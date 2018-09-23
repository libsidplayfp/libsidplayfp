//
//  exSID.c
//	A simple I/O library for exSID/exSID+ USB
//
//  (C) 2015-2018 Thibaut VARENE
//  License: GPLv2 - http://www.gnu.org/licenses/gpl-2.0.html

/**
 * @file
 * exSID/exSID+ USB I/O library
 * @author Thibaut VARENE
 * @date 2015-2018
 * @version 2.0
 *
 * This driver will control the first exSID device available.
 * All public API functions are only valid after a successful call to exSID_init().
 * To release the device and resources, exSID_exit() must be called.
 */

#include "exSID.h"
#include "exSID_defs.h"
#include "exSID_ftdiwrap.h"
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>

#ifdef	EXSID_THREADED
 #if defined(HAVE_THREADS_H)	// Native C11 threads support
  #include <threads.h>
 #elif defined(HAVE_PTHREAD_H)	// Trivial C11 over pthreads support
  #include "c11threads.h"
 #else
  #error "No thread model available"
 #endif
#endif	// EXSID_TRHREADED

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof(x[0]))

char xSerrstr[];

#ifdef	DEBUG
static long accdrift = 0;
static unsigned long accioops = 0;
static unsigned long accdelay = 0;
static unsigned long acccycle = 0;	// ensures no overflow with exSID+ up to ~1h of continuous playback
#endif

static int ftdi_status;
static void * ftdi = NULL;

#ifdef	EXSID_THREADED
// Global variables for flip buffering
static unsigned char bufchar0[XS_BUFFSZ];
static unsigned char bufchar1[XS_BUFFSZ];
static unsigned char * restrict frontbuf = bufchar0, * restrict backbuf = bufchar1;
static int frontbuf_idx = 0, backbuf_idx = 0;
static mtx_t frontbuf_mtx;	///< mutex protecting access to frontbuf
static cnd_t frontbuf_ready_cnd, frontbuf_done_cnd;
static thrd_t thread_output;
#endif	// EXSID_THREADED

/**
 * cycles is uint_fast32_t. Technically, clkdrift should be int_fast64_t though
 * overflow should not happen under normal conditions.
 */
typedef int_fast32_t clkdrift_t;

/**
 * negative values mean we're lagging, positive mean we're ahead.
 * See it as a number of SID clocks queued to be spent.
 */
static clkdrift_t clkdrift = 0;

/**
 * This private structure holds hardware-dependent constants.
 */
struct xSpriv_s {
	unsigned int	model;			///< exSID device model in use
	clkdrift_t	write_cycles;		///< number of SID clocks spent in write ops
	clkdrift_t	read_pre_cycles;	///< number of SID clocks spent in read op before data is actually read
	clkdrift_t	read_post_cycles;	///< number of SID clocks spent in read op after data is actually read
	clkdrift_t	read_offset_cycles;	///< read offset adjustment to align with writes (see function documentation)
	clkdrift_t	csioctl_cycles;		///< number of SID clocks spent in chip select ioctl
	clkdrift_t	mindel_cycles;		///< lowest number of SID clocks that can be accounted for in delay
	clkdrift_t	max_adj;		///< maximum number of SID clocks that can be encoded in final delay for read()/write()
	clkdrift_t	ldelay_offs;		///< long delay SID clocks offset
};

/** Array of supported devices */
const struct {
	const char *		desc;
	const int		pid;
	const int		vid;
	const struct xSpriv_s	xsp;
} xSsupported[] = {
	{
		/* exSID USB */
		.desc = XS_USBDSC,
		.pid = XS_USBPID,
		.vid = XS_USBVID,
		.xsp = (struct xSpriv_s){
			.model = XS_MODEL_STD,
			.write_cycles = XS_CYCIO,
			.read_pre_cycles = XS_CYCCHR,
			.read_post_cycles = XS_CYCCHR,
			.read_offset_cycles = -2,	// see exSID_clkdread() documentation
			.csioctl_cycles = XS_CYCCHR,
			.mindel_cycles = XS_MINDEL,
			.max_adj = XS_MAXADJ,
			.ldelay_offs = XS_LDOFFS,
		},
	}, {
		/* exSID+ USB */
		.desc = XSP_USBDSC,
		.pid = XSP_USBPID,
		.vid = XSP_USBVID,
		.xsp = (struct xSpriv_s){
			.model = XS_MODEL_PLUS,
			.write_cycles = XSP_CYCIO,
			.read_pre_cycles = XSP_PRE_RD,
			.read_post_cycles = XSP_POSTRD,
			.read_offset_cycles = 0,
			.csioctl_cycles = XSP_CYCCS,
			.mindel_cycles = XSP_MINDEL,
			.max_adj = XSP_MAXADJ,
			.ldelay_offs = XSP_LDOFFS,
		},
	}
};

/** Global pointer used by all the hardware access routines */
const struct xSpriv_s * restrict xSpriv;

static inline void _exSID_write(uint_least8_t addr, uint8_t data, int flush);

/**
 * Returns a string describing the last recorded error.
 * @return error message (max 256 bytes long).
 */
const char * exSID_error_str(void)
{
	return (xSerrstr);
}


/**
 * Write routine to send data to the device.
 * @note BLOCKING.
 * @param buff pointer to a byte array of data to send
 * @param size number of bytes to send
 */
static inline void xSwrite(const unsigned char * buff, int size)
{
	ftdi_status = xSfw_write_data(ftdi, buff, size);
#ifdef	DEBUG
	if (unlikely(ftdi_status < 0)) {
		xsdbg("Error ftdi_write_data(%d): %s\n",
			ftdi_status, xSfw_get_error_string(ftdi));
	}
	if (unlikely(ftdi_status != size)) {
		xsdbg("ftdi_write_data only wrote %d (of %d) bytes\n",
			ftdi_status, size);
	}
#endif
}

/**
 * Read routine to get data from the device.
 * @note BLOCKING.
 * @param buff pointer to a byte array that will be filled with read data
 * @param size number of bytes to read
 */
static void xSread(unsigned char * buff, int size)
{
#ifdef	EXSID_THREADED
	mtx_lock(&frontbuf_mtx);
	while (frontbuf_idx)
		cnd_wait(&frontbuf_done_cnd, &frontbuf_mtx);
#endif
	ftdi_status = xSfw_read_data(ftdi, buff, size);
#ifdef	EXSID_THREADED
	mtx_unlock(&frontbuf_mtx);
#endif

#ifdef	DEBUG
	if (unlikely(ftdi_status < 0)) {
		xsdbg("Error ftdi_read_data(%d): %s\n",
			ftdi_status, xSfw_get_error_string(ftdi));
	}
	if (unlikely(ftdi_status != size)) {
		xsdbg("ftdi_read_data only read %d (of %d) bytes\n",
			ftdi_status, size);
	}
#endif
}


#ifdef	EXSID_THREADED
/**
 * Writer thread. ** consummer **
 * This thread consumes buffer prepared in xSoutb().
 * Since writes to the FTDI subsystem are blocking, this thread blocks when it's
 * writing to the chip, and also while it's waiting for the front buffer to be ready.
 * This ensures execution time consistency as xSoutb() periodically waits for
 * the front buffer to be ready before flipping buffers.
 * @note BLOCKING.
 * @param arg ignored
 * @return DOES NOT RETURN, exits when frontbuf_idx is negative.
 */
static int _exSID_thread_output(void *arg)
{
	xsdbg("thread started\n");
	while (1) {
		mtx_lock(&frontbuf_mtx);

		// wait for frontbuf ready (not empty)
		while (!frontbuf_idx)
			cnd_wait(&frontbuf_ready_cnd, &frontbuf_mtx);

		if (unlikely(frontbuf_idx < 0))	{	// exit condition
			xsdbg("thread exiting!\n");
			mtx_unlock(&frontbuf_mtx);
			thrd_exit(0);
		}

		xSwrite(frontbuf, frontbuf_idx);
		frontbuf_idx = 0;

		// xSread() and xSoutb() are in the same thread of execution
		// so it can only be one or the other waiting.
		cnd_signal(&frontbuf_done_cnd);
		mtx_unlock(&frontbuf_mtx);
	}
	return 0;	// make the compiler happy
}
#endif	// EXSID_THREADED

/**
 * Single byte output routine. ** producer **
 * Fills a static buffer with bytes to send to the device until the buffer is
 * full or a forced write is triggered.
 * @note No drift compensation is performed on read operations.
 * @param byte byte to send
 * @param flush force write flush if positive, trigger thread exit if negative
 */
static void xSoutb(uint8_t byte, int flush)
{
#ifndef	EXSID_THREADED
	static unsigned char backbuf[XS_BUFFSZ];
	static int backbuf_idx = 0;
#else
	unsigned char * bufptr = NULL;
#endif

	backbuf[backbuf_idx++] = (unsigned char)byte;

	if (likely((backbuf_idx < XS_BUFFSZ) && !flush))
		return;

#ifdef	EXSID_THREADED
	// buffer dance
	mtx_lock(&frontbuf_mtx);

	// wait for frontbuf available (empty). Only triggers if previous
	// write buffer hasn't been consummed before we get here again.
	while (unlikely(frontbuf_idx))
		cnd_wait(&frontbuf_done_cnd, &frontbuf_mtx);

	if (unlikely(flush < 0))	// indicate exit request
		frontbuf_idx = -1;
	else {				// flip buffers
		bufptr = frontbuf;
		frontbuf = backbuf;
		frontbuf_idx = backbuf_idx;
		backbuf = bufptr;
		backbuf_idx = 0;
	}

	cnd_signal(&frontbuf_ready_cnd);
	mtx_unlock(&frontbuf_mtx);
#else	// unthreaded
	xSwrite(backbuf, backbuf_idx);
	backbuf_idx = 0;
#endif
}

/**
 * Device init routine.
 * Must be called once before any operation is attempted on the device.
 * Opens first available device, and sets various parameters: baudrate, parity, flow
 * control and USB latency, and finally clears the RX and TX buffers.
 * @return 0 on success, !0 otherwise.
 */
int exSID_init(void)
{
	unsigned char dummy;
	int i, found, ret;

	if (ftdi) {
		xserror("Device already open!");
		return -1;
	}

	if (xSfw_dlopen()) {
		//xserror("Failed to open dynamic loader"); // already sets its own error message
		return -1;
	}

	/* Attempt to open all supported devices until first success.
	 * Cleanup ftdi after each try to avoid passing garbage around as we don't know what
	 * the FTDI open routines do with the pointer.
	 * FIXME: despite that, some combinations still don't seem to work */
	found = 0;
	for (i = 0; i < ARRAY_SIZE(xSsupported); i++) {
		if (xSfw_new) {
			ftdi = xSfw_new();
			if (!ftdi) {
				xserror("ftdi_new failed");
				return -1;
			}
		}

		xsdbg("Trying %s...\n", xSsupported[i].desc);
		xSpriv = &xSsupported[i].xsp;	// setting unconditionnally avoids segfaults if user code does the wrong thing.
		ftdi_status = xSfw_usb_open_desc(&ftdi, xSsupported[i].vid, xSsupported[i].pid, xSsupported[i].desc, NULL);
		if (ftdi_status >= 0) {
			xsdbg("Opened!\n");
			found = 1;
			break;
		}
		else {
			xsdbg("Failed: %d (%s)\n", ftdi_status, xSfw_get_error_string(ftdi));
			if (xSfw_free)
				xSfw_free(ftdi);
			ftdi = NULL;
		}
	}

	if (!found) {
		xserror("No device could be opened");
		return -1;
	}

	ftdi_status = xSfw_usb_setup(ftdi, XS_BDRATE, XS_USBLAT);
	if (ftdi_status < 0) {
		// xserror("Failed to setup device"); // already sets its own error message
		return -1;
	}

	// success - device is ready
	xsdbg("Device ready\n");
	

#ifdef	EXSID_THREADED
	xsdbg("Thread setup\n");
	ret = mtx_init(&frontbuf_mtx, mtx_plain);
	ret |= cnd_init(&frontbuf_ready_cnd);
	ret |= cnd_init(&frontbuf_done_cnd);
	backbuf_idx = frontbuf_idx = 0;
	ret |= thrd_create(&thread_output, _exSID_thread_output, NULL);
	if (ret) {
		xserror("Thread setup failed");
		return -1;
	}
#endif

	xSfw_usb_purge_buffers(ftdi); // Purge both Rx and Tx buffers

	// Wait for device ready by trying to read FV and wait for the answer
	// XXX Broken with libftdi due to non-blocking read :-/
	xSoutb(XS_AD_IOCTFV, 1);
	xSread(&dummy, 1);

	xsdbg("Rock'n'roll!\n");

#ifdef	DEBUG
	exSID_hwversion();
	xsdbg("XS_BUFFSZ: %d bytes\n", XS_BUFFSZ);
#endif

	return 0;
}

/**
 * Device exit routine.
 * Must be called to release the device.
 * Resets the SIDs and clears RX/TX buffers, releases all resources allocated
 * in exSID_init().
 */
void exSID_exit(void)
{
	if (ftdi) {
		exSID_reset(0);

#ifdef	EXSID_THREADED
		xSoutb(XS_AD_IOCTFV, -1);	// signal end of thread
		cnd_destroy(&frontbuf_ready_cnd);
		mtx_destroy(&frontbuf_mtx);
		thrd_join(thread_output, NULL);
#endif

		xSfw_usb_purge_buffers(ftdi); // Purge both Rx and Tx buffers

		ftdi_status = xSfw_usb_close(ftdi);
		if (ftdi_status < 0)
			xserror("Unable to close ftdi device: %d (%s)",
				ftdi_status, xSfw_get_error_string(ftdi));

		if (xSfw_free)
			xSfw_free(ftdi);
		ftdi = NULL;

#ifdef	DEBUG
		xsdbg("mean jitter: %.2f cycle(s) over %lu I/O ops\n",
			((float)accdrift/accioops), accioops);
		xsdbg("bandwidth used for I/O ops: %lu%% (approx)\n",
			100-(accdelay*100/acccycle));
		accdrift = accioops = accdelay = acccycle = 0;
#endif
	}

	clkdrift = 0;
	xSfw_dlclose();
}


/**
 * SID reset routine.
 * Performs a hardware reset on the SIDs.
 * @note since the reset procedure in firmware will stall the device,
 * reset forcefully waits for enough time before resuming execution
 * via a call to usleep();
 * @param volume volume to set the SIDs to after reset.
 */
void exSID_reset(uint_least8_t volume)
{
	xsdbg("rvol: %" PRIxLEAST8 "\n", volume);

	xSoutb(XS_AD_IOCTRS, 1);	// this will stall
	usleep(100);	// sleep for 100us
	_exSID_write(0x18, volume, 1);	// this only needs 2 bytes which matches the input buffer of the PIC so all is well

	clkdrift = 0;
}


/**
 * exSID+ clock selection routine.
 * Selects between PAL, NTSC and 1MHz clocks.
 * @note upon clock change the hardware resync itself and resets the SIDs, which
 * takes approximately 50us: this function waits for enough time before resuming
 * execution via a call to usleep();
 * Output should be muted before execution
 * @param clock clock selector value, see exSID.h.
 * @return execution status
 */
int exSID_clockselect(int clock)
{
	xsdbg("clk: %d\n", clock);

	if (XS_MODEL_PLUS != xSpriv->model)
		return -1;

	switch (clock) {
		case XS_CL_PAL:
			xSoutb(XSP_AD_IOCTCP, 1);
			break;
		case XS_CL_NTSC:
			xSoutb(XSP_AD_IOCTCN, 1);
			break;
		case XS_CL_1MHZ:
			xSoutb(XSP_AD_IOCTC1, 1);
			break;
		default:
			return -1;
	}

	usleep(100);	// sleep for 100us

	clkdrift = 0;	// reset drift

	return 0;
}

/**
 * exSID+ audio operations routine.
 * Selects the audio mixing / muting option. Only implemented in exSID+ devices.
 * @warning all these operations (excepting unmuting obviously) will mute the
 * output by default.
 * @note no accounting for SID cycles consumed.
 * @param operation audio operation value, see exSID.h.
 * @return execution status
 */
int exSID_audio_op(int operation)
{
	xsdbg("auop: %d\n", operation);

	if (XS_MODEL_PLUS != xSpriv->model)
		return -1;

	switch (operation) {
		case XS_AU_6581_8580:
			xSoutb(XSP_AD_IOCTA0, 0);
			break;
		case XS_AU_8580_6581:
			xSoutb(XSP_AD_IOCTA1, 0);
			break;
		case XS_AU_8580_8580:
			xSoutb(XSP_AD_IOCTA2, 0);
			break;
		case XS_AU_6581_6581:
			xSoutb(XSP_AD_IOCTA3, 0);
			break;
		case XS_AU_MUTE:
			xSoutb(XSP_AD_IOCTAM, 0);
			break;
		case XS_AU_UNMUTE:
			xSoutb(XSP_AD_IOCTAU, 0);
			break;
		default:
			return -1;
	}

	return 0;
}

/**
 * SID chipselect routine.
 * Selects which SID will play the tunes. If neither CHIP0 or CHIP1 is chosen,
 * both SIDs will operate together. Accounts for elapsed cycles.
 * @param chip SID selector value, see exSID.h.
 */
void exSID_chipselect(int chip)
{
	clkdrift -= xSpriv->csioctl_cycles;

	xsdbg("cs: %d\n", chip);

	if (XS_CS_CHIP0 == chip)
		xSoutb(XS_AD_IOCTS0, 0);
	else if (XS_CS_CHIP1 == chip)
		xSoutb(XS_AD_IOCTS1, 0);
	else
		xSoutb(XS_AD_IOCTSB, 0);
}

/**
 * Device hardware model.
 * Queries the driver for the hardware model currently identified.
 * @return hardware model as enumerated in exSID.h, negative value on error.
 */
int exSID_hwmodel(void)
{
	int model;

	switch (xSpriv->model) {
		case XS_MODEL_STD:
			model = XS_MD_STD;
			break;
		case XS_MODEL_PLUS:
			model = XS_MD_PLUS;
			break;
		default:
			model = -1;
			break;
	}

	xsdbg("HW model: %d\n", model);

	return model;
}

/**
 * Hardware and firmware version of the device.
 * Queries the device for the hardware revision and current firmware version
 * and returns both in the form of a 16bit integer: MSB is an ASCII
 * character representing the hardware revision (e.g. 0x42 = "B"), and LSB
 * is a number representing the firmware version in decimal integer.
 * Does NOT account for elapsed cycles.
 * @return version information as described above.
 */
uint16_t exSID_hwversion(void)
{
	unsigned char inbuf[2];
	uint16_t out = 0;

	xSoutb(XS_AD_IOCTHV, 0);
	xSoutb(XS_AD_IOCTFV, 1);
	xSread(inbuf, 2);
	out = inbuf[0] << 8 | inbuf[1];	// ensure proper order regardless of endianness

	xsdbg("HV: %c, FV: %hhu\n", inbuf[0], inbuf[1]);

	return out;
}

/**
 * Private busy delay loop.
 * @note will block every time a device write is triggered, blocking time will be
 * equal to the number of bytes written times mindel_cycles.
 * @param cycles how many SID clocks to loop for.
 */
static inline void xSdelay(uint_fast32_t cycles)
{
#ifdef	DEBUG
	accdelay += cycles;
#endif
	while (likely(cycles >= xSpriv->mindel_cycles)) {
		xSoutb(XS_AD_IOCTD1, 0);
		cycles -= xSpriv->mindel_cycles;
		clkdrift -= xSpriv->mindel_cycles;
	}
#ifdef	DEBUG
	accdelay -= cycles;
#endif
}

/**
 * Private long delay loop.
 * Calls to IOCTLD delay, for "very" long delays (thousands of SID clocks).
 * Requested delay @b MUST be > ldelay_offs, and for better performance,
 * the requested delay time should ideally be several XS_LDMULT and be close to
 * a multiple of XS_USBLAT milliseconds (on the exSID).
 * @warning polling and NOT CYCLE ACCURATE on exSID
 * @param cycles how many SID clocks to wait for.
 */
static void xSlongdelay(uint_fast32_t cycles)
{
	int multiple, flush;
	uint_fast32_t delta;
	unsigned char dummy;

	flush = (XS_MODEL_STD == xSpriv->model);

	multiple = cycles - xSpriv->ldelay_offs;
	delta = multiple % XS_LDMULT;
	multiple /= XS_LDMULT;

	//xsdbg("ldelay: %" PRIdFAST32 ", multiple: %d, delta: %" PRIdFAST32 "\n", cycles, multiple, delta);

	if (unlikely(multiple < 0)) {
		xsdbg("Wrong delay!\n");
		return;
	}

#ifdef	DEBUG
	accdelay += (cycles - delta);
#endif

	while (multiple >= 255) {
		_exSID_write(XS_AD_IOCTLD, 255, flush);
		if (flush)
			xSread(&dummy, 1);	// wait for answer with blocking read
		multiple -= 255;
	}

	if (multiple) {
		_exSID_write(XS_AD_IOCTLD, (unsigned char)multiple, flush);
		if (flush)
			xSread(&dummy, 1);	// wait for answer with blocking read
	}

	// deal with remainder
	xSdelay(delta);
}

/**
 * Cycle accurate delay routine.
 * Applies the most efficient strategy to delay for cycles SID clocks
 * while leaving enough lead time for an I/O operation.
 * @param cycles how many SID clocks to loop for.
 */
void exSID_delay(uint_fast32_t cycles)
{
	uint_fast32_t delay;

	clkdrift += cycles;
#ifdef	DEBUG
	acccycle += cycles;
#endif

	if (unlikely(clkdrift <= xSpriv->write_cycles))	// never delay for less than a full write would need
		return;	// too short

	delay = clkdrift - xSpriv->write_cycles;

	switch (xSpriv->model) {
#if 0	// currently breaks sidplayfp - REVIEW
		case XS_MODEL_PLUS:
			if (delay > XS_LDMULT) {
				xSlongdelay(delay);
				break;
			}
#endif
		default:
			xSdelay(delay);
	}
}

/**
 * Private write routine for a tuple address + data.
 * @param addr target address to write to.
 * @param data data to write at that address.
 * @param flush if non-zero, force immediate flush to device.
 */
static inline void _exSID_write(uint_least8_t addr, uint8_t data, int flush)
{
	xSoutb((unsigned char)addr, 0);
	xSoutb((unsigned char)data, flush);
}

/**
 * Timed write routine, attempts cycle-accurate writes.
 * This function will be cycle-accurate provided that no two consecutive reads or writes
 * are less than write_cycles apart and the leftover delay is <= max_adj SID clock cycles.
 * @param cycles how many SID clocks to wait before the actual data write.
 * @param addr target address.
 * @param data data to write at that address.
 */
void exSID_clkdwrite(uint_fast32_t cycles, uint_least8_t addr, uint8_t data)
{
	static int adj = 0;

#ifdef	DEBUG
	if (unlikely(addr > 0x18)) {
		xsdbg("Invalid write: %.2" PRIxLEAST8 "\n", addr);
		exSID_delay(cycles);
		return;
	}
#endif

	// actual write will cost write_cycles. Delay for cycles - write_cycles then account for the write
	clkdrift += cycles;
	if (clkdrift > xSpriv->write_cycles)
		xSdelay(clkdrift - xSpriv->write_cycles);

	clkdrift -= xSpriv->write_cycles;	// write is going to consume write_cycles clock ticks

#ifdef	DEBUG
	if (clkdrift >= xSpriv->mindel_cycles)
		xsdbg("Impossible drift adjustment! %" PRIdFAST32 " cycles\n", clkdrift);
	else if (clkdrift < 0)
		accdrift += clkdrift;
#endif

	/* if we are still going to be early, delay actual write by up to XS_MAXAD ticks
	At this point it is guaranted that clkdrift will be < mindel_cycles. */
	if (likely(clkdrift >= 0)) {
		adj = clkdrift % (xSpriv->max_adj+1);
		/* if max_adj is >= clkdrift, modulo will give the same results
		   as the correct test:
		   adj = (clkdrift < max_adj ? clkdrift : max_adj)
		   but without an extra conditional branch. If is is < max_adj, then it
		   seems to provide better results by evening jitter accross writes. So
		   it's the preferred solution for all cases. */
		addr = (unsigned char)(addr | (adj << 5));	// final delay encoded in top 3 bits of address
#ifdef	DEBUG
		accdrift += (clkdrift - adj);
#endif
		//xsdbg("drft: %d, adj: %d, addr: %.2hhx, data: %.2hhx\n", clkdrift, adj, (char)(addr | (adj << 5)), data);
	}

#ifdef	DEBUG
	acccycle += cycles;
	accioops++;
#endif

	//xsdbg("delay: %d, clkdrift: %d\n", cycles, clkdrift);
	_exSID_write(addr, data, 0);
}

/**
 * Private read routine for a given address.
 * @param addr target address to read from.
 * @param flush if non-zero, force immediate flush to device.
 * @return data read from address.
 */
static inline uint8_t _exSID_read(uint_least8_t addr, int flush)
{
	static unsigned char data;

	xSoutb(addr, flush);	// XXX
	xSread(&data, 1);	// blocking

	xsdbg("addr: %.2" PRIxLEAST8 ", data: %.2hhx\n", addr, data);
	return data;
}

/**
 * BLOCKING Timed read routine, attempts cycle-accurate reads.
 * The following description is based on exSID (standard).
 * This function will be cycle-accurate provided that no two consecutive reads or writes
 * are less than XS_CYCIO apart and leftover delay is <= max_adj SID clock cycles.
 * Read result will only be available after a full XS_CYCIO, giving clkdread() the same
 * run time as clkdwrite(). There's a 2-cycle negative adjustment in the code because
 * that's the actual offset from the write calls ('/' denotes falling clock edge latch),
 * which the following ASCII tries to illustrate: <br />
 * Write looks like this in firmware:
 * > ...|_/_|...
 * ...end of data byte read | cycle during which write is enacted / next cycle | etc... <br />
 * Read looks like this in firmware:
 * > ...|_|_|_/_|_|...
 * ...end of address byte read | 2 cycles for address processing | cycle during which SID is read /
 *	then half a cycle later the CYCCHR-long data TX starts, cycle completes | another cycle | etc... <br />
 * This explains why reads happen a relative 2-cycle later than then should with
 * respect to writes.
 * @note The actual time the read will take to complete depends
 * on the USB bus activity and settings. It *should* complete in XS_USBLAT ms, but
 * not less, meaning that read operations are bound to introduce timing inaccuracy.
 * As such, this function is only really provided as a proof of concept but SHOULD
 * BETTER BE AVOIDED.
 * @param cycles how many SID clocks to wait before the actual data read.
 * @param addr target address.
 * @return data read from address.
 */
uint8_t exSID_clkdread(uint_fast32_t cycles, uint_least8_t addr)
{
	static int adj = 0;

#ifdef	DEBUG
	if (unlikely((addr < 0x19) || (addr > 0x1C))) {
		xsdbg("Invalid read: %.2" PRIxLEAST8 "\n", addr);
		exSID_delay(cycles);
		return 0xFF;
	}
#endif

	// actual read will happen after read_pre_cycles. Delay for cycles - read_pre_cycles then account for the read
	clkdrift += xSpriv->read_offset_cycles;		// 2-cycle offset adjustement, see function documentation.
	clkdrift += cycles;
	if (clkdrift > xSpriv->read_pre_cycles)
		xSdelay(clkdrift - xSpriv->read_pre_cycles);

	clkdrift -= xSpriv->read_pre_cycles;	// read request is going to consume read_pre_cycles clock ticks

#ifdef	DEBUG
	if (clkdrift > xSpriv->mindel_cycles)
		xsdbg("Impossible drift adjustment! %" PRIdFAST32 " cycles", clkdrift);
	else if (clkdrift < 0) {
		accdrift += clkdrift;
		xsdbg("Late read request! %" PRIdFAST32 " cycles\n", clkdrift);
	}
#endif

	// if we are still going to be early, delay actual read by up to max_adj ticks
	if (likely(clkdrift >= 0)) {
		adj = clkdrift % (xSpriv->max_adj+1);	// see clkdwrite()
		addr = (unsigned char)(addr | (adj << 5));	// final delay encoded in top 3 bits of address
#ifdef	DEBUG
		accdrift += (clkdrift - adj);
#endif
		//xsdbg("drft: %d, adj: %d, addr: %.2hhx, data: %.2hhx\n", clkdrift, adj, (char)(addr | (adj << 5)), data);
	}

#ifdef	DEBUG
	acccycle += cycles;
	accioops++;
#endif

	// after read has completed, at least another read_post_cycles will have been spent
	clkdrift -= xSpriv->read_post_cycles;

	//xsdbg("delay: %d, clkdrift: %d\n", cycles, clkdrift);
	return _exSID_read(addr, 1);
}
