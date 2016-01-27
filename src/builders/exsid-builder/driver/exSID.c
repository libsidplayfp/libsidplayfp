//
//  exSID.c
//	A simple I/O library for exSID USB
//
//  (C) 2015-2016 Thibaut VARENE
//  License: GPLv2 - http://www.gnu.org/licenses/gpl-2.0.html
//
//  Builds with -lftdi1

/**
 * @file
 * exSID USB I/O library
 * @author Thibaut VARENE
 * @date 2015-2016
 * @version 1.1
 */

#include "exSID.h"
#include <ftdi.h>
#include <stdio.h>
#include <unistd.h>

#ifdef	DEBUG
static long accdrift = 0;
static long accioops = 0;
static long accdelay = 0;
static long acccycle = 0;
#endif

static struct ftdi_context *ftdi = NULL;
static struct ftdi_version_info version;
static int ftdi_status;
/**
 * cycles is uint_fast32_t. Technically, clkdrift should be int_fast64_t though
 * overflow should not happen under normal conditions.
 * negative values mean we're lagging, positive mean we're ahead.
 * See it as a number of SID clocks queued to be spent.
 */
static int_fast32_t clkdrift = 0;

static inline void _exSID_write(uint_least8_t addr, uint8_t data, int flush);

/**
 * Write routine to send data to the device. BLOCKING.
 * @param buff pointer to a byte array of data to send
 * @param size number of bytes to send
 */
static inline void _xSwrite(const unsigned char *buff, int size)
{
	ftdi_status = ftdi_write_data(ftdi, buff, size);
#ifdef	DEBUG
	if (unlikely(ftdi_status < 0)) {
		error("Error ftdi_write_data(%d): %s\n", ftdi_status, ftdi_get_error_string(ftdi));
	}
	if (unlikely(ftdi_status != size)) {
		error("ftdi_write_data only wrote %d (of %d) bytes\n",
			   ftdi_status,
			   size);
	}
#endif
}

/**
 * Read routine to get data from the device. BLOCKING.
 * @param buff pointer to a byte array that will be filled with read data
 * @param size number of bytes to read
 */
static inline void _xSread(unsigned char *buff, int size)
{
	ftdi_status = ftdi_read_data(ftdi, buff, size);
#ifdef	DEBUG
	if (unlikely(ftdi_status < 0)) {
	        error("Error ftdi_read_data(%d): %s\n", ftdi_status, ftdi_get_error_string(ftdi));
	}
	if (unlikely(ftdi_status != size)) {
		error("ftdi_read_data only read %d (of %d) bytes\n",
			   ftdi_status,
			   size);
	}
#endif
}

/**
 * Single byte output routine.
 * Fills a static buffer with bytes to send to the device until the buffer is
 * full or a forced write is triggered. Compensates for drift if XS_BDRATE isn't
 * a multiple of of XS_SIDCLK.
 * @note No drift compensation is performed on read operations.
 * @param byte byte to send
 * @param flush force write flush if non-zero
 */
static void _xSoutb(uint8_t byte, int flush)
{
	static unsigned char bufchar[XS_BUFFSZ];
	static int i = 0;

	bufchar[i++] = (unsigned char)byte;

	/* if XS_BDRATE isn't a multiple of XS_SIDCLK we will drift:
	   every XS_BDRATE/(remainder of XS_SIDCLK/XS_BDRATE) we lose one SID cycle.
	   Compensate here */
	if (XS_SIDCLK % XS_BDRATE) {
		if (!(i % (XS_BDRATE/(XS_SIDCLK%XS_BDRATE))))
			clkdrift--;
	}


	if (likely(!flush && (i < XS_BUFFSZ)))
		return;

	_xSwrite(bufchar, i);
	i = 0;
}

/**
 * Device init routine.
 * Must be called once before any operation is attempted on the device.
 * Opens the named device, and sets various parameters: baudrate, parity, flow
 * control and USB latency, and finally clears the RX and TX buffers.
 * @return 0 on success, !0 otherwise.
 */
int exSID_init(void)
{
	if (ftdi) {
		error("Device already open!");
		return -1;
	}

	ftdi = ftdi_new();
	if (ftdi == NULL) {
		error("ftdi_new failed\n");
		return -1;
	}

	ftdi_status = ftdi_usb_open_desc(ftdi, 0x0403, 0x6001, "exSIDUSB", NULL);
	if (ftdi_status < 0) {
		error("Failed to open device: %d (%s)\n", ftdi_status, ftdi_get_error_string(ftdi));
		ftdi_free(ftdi);
		ftdi = NULL;
		return -1;
	}

	// success - device with device description "exSIDUSB" is open
	dbg("Device opened\n");

	ftdi_status = ftdi_set_baudrate(ftdi, XS_BDRATE);
	if (ftdi_status < 0)
		error("BR error\n");

	ftdi_status = ftdi_set_line_property(ftdi, BITS_8 , STOP_BIT_1, NONE);
	if (ftdi_status < 0)
		error("SDC error\n");

	ftdi_status = ftdi_setflowctrl(ftdi, SIO_DISABLE_FLOW_CTRL);
	if (ftdi_status < 0)
		error("SFC error\n");

	ftdi_status = ftdi_set_latency_timer(ftdi, XS_USBLAT);
	if (ftdi_status < 0)
		error("SLT error\n");

#ifdef	DEBUG
	exSID_version();
#endif
	
	ftdi_status = ftdi_usb_purge_buffers(ftdi); // Purge both Rx and Tx buffers

	return 0;
}

/**
 * Device exit routine.
 * Must be called to release the device.
 * Resets the SIDs and clears RX/TX buffers.
 */
void exSID_exit(void)
{
	if (ftdi == NULL)
		return;

	exSID_reset(0);

	ftdi_status = ftdi_usb_purge_buffers(ftdi); // Purge both Rx and Tx buffers
	if ((ftdi_status = ftdi_usb_close(ftdi)) < 0)
		error("unable to close ftdi device: %d (%s)\n", ftdi_status, ftdi_get_error_string(ftdi));

	ftdi_free(ftdi);
	ftdi = NULL;

#ifdef	DEBUG
	printf("mean drift: %d cycles over %ld I/O ops\n", (accdrift/accioops), accioops);
	printf("time spent in delays: %d%% (approx)\n", (accdelay*100/acccycle));
	accdrift = accioops = accdelay = acccycle = 0;
#endif

	clkdrift = 0;
}


/**
 * SID reset routine.
 * Performs a hardware reset on the SIDs.
 * @note since the reset procedure in firmware will stall the device for more than
 * XS_CYCCHR, reset forcefully waits for enough time before resuming execution
 * via a call to usleep();
 * @param volume volume to set the SIDs to after reset.
 */
void exSID_reset(uint_least8_t volume)
{
	dbg("rvol: %hhx\n", volume);

	_xSoutb(XS_AD_IOCTRS, 0);
	_exSID_write(0x18, volume, 1);

	clkdrift = 0;
	usleep(1000);	// sleep for 1ms
}

/**
 * SID chipselect routine.
 * Selects which SID will play the tunes. If neither CHIP0 or CHIP1 is chosen,
 * both SIDs will operate together.
 * @param chip SID selector value, see exSID.h.
 */
void exSID_chipselect(int chip)
{
	if (XS_CS_CHIP0 == chip)
		_xSoutb(XS_AD_IOCTS0, 0);
	else if (XS_CS_CHIP1 == chip)
		_xSoutb(XS_AD_IOCTS1, 0);
	else
		_xSoutb(XS_AD_IOCTSB, 0);
}

/**
 * Hardware and firmware version of the device.
 * Queries the device for the hardware revision and current firmware version
 * and returns both in the form of a 16bit integer: MSB is an ASCII
 * character representing the hardware revision (e.g. 0x42 = "B"), and LSB
 * is a number representing the firmware version x10 in decimal (e.g. 10 = "1.0").
 * @return version information as described above.
 */
uint16_t exSID_version(void)
{
	unsigned char inbuf[2];
	uint16_t out = 0;

	_xSoutb(XS_AD_IOCTHV, 0);
	_xSoutb(XS_AD_IOCTFV, 1);
	_xSread(inbuf, 2);
	out = inbuf[0] << 8 | inbuf[1];	// ensure proper order regardless of endianness

	dbg("HV: %c, FV: %hhd\n", inbuf[0], inbuf[1]);

	return out;
}

/**
 * Poll-based blocking (long) delay.
 * Calls to IOCTLD polled delay, for "very" long delays (thousands of SID clocks).
 * Total delay should be 3*CYCCHR + WAITCNT(500 + 1) (see PIC firmware).
 * @warning NOT CYCLE ACCURATE
 * @param cycles how many SID clocks to wait for.
 */
void exSID_polldelay(uint_fast32_t cycles)
{
#define	SBPDOFFSET	(3*XS_CYCCHR)
#define	SBPDMULT	501
	int delta;
	int multiple;	// minimum 1 full loop
	unsigned char dummy;

	multiple = cycles - SBPDOFFSET;
	delta = multiple % SBPDMULT;
	multiple /= SBPDMULT;

	//dbg("ldelay: %d, multiple: %d, delta: %d\n", cycles, multiple, delta);

	if (unlikely((multiple <=0) || (multiple > 255)))
		error("Wrong delay!\n");

	// send delay command and flush queue
	_exSID_write(XS_AD_IOCTLD, (unsigned char)multiple, 1);

	// wait for answer with blocking read
	_xSread(&dummy, 1);

	// deal with remainder
	exSID_delay(delta);

#ifdef	DEBUG
	acccycle += (cycles - delta);
	accdelay += (cycles - delta);
#endif
}

/**
 * Private delay loop.
 * @note will block every time a device write is triggered, blocking time will be
 * equal to the number of bytes written times XS_MINDEL.
 * @param cycles how many SID clocks to loop for.
 */
static inline void _xSdelay(uint_fast32_t cycles)
{
#ifdef	DEBUG
	accdelay += cycles;
#endif

	while (likely(cycles >= XS_MINDEL)) {
		_xSoutb(XS_AD_IOCTD1, 0);
		cycles -= XS_MINDEL;
		clkdrift -= XS_MINDEL;
	}
}

/**
 * Write-based delay.
 * Calls _xSdelay() while leaving enough lead time for an I/O operation.
 * @param cycles how many SID clocks to loop for.
 */
void exSID_delay(uint_fast32_t cycles)
{
	clkdrift += cycles;

#ifdef	DEBUG
	acccycle += cycles;
#endif

	if (unlikely(clkdrift <= XS_CYCIO))	// never delay for less than a full write would need
		return;	// too short

	_xSdelay(clkdrift - XS_CYCIO);
}

/**
 * Private write routine for a tuple address + data.
 * @param addr target address to write to.
 * @param data data to write at that address.
 * @param flush if non-zero, force immediate flush to device.
 */
static inline void _exSID_write(uint_least8_t addr, uint8_t data, int flush)
{
	_xSoutb((unsigned char)addr, 0);
	_xSoutb((unsigned char)data, flush);
}

/**
 * Timed write routine, attempts cycle-accurate writes.
 * This function will be cycle-accurate provided that no two consecutive reads or writes
 * are less than XS_CYCIO apart and the leftover delay is <= 7 SID clock cycles.
 * @param cycles how many SID clocks to wait before the actual data write.
 * @param addr target address.
 * @param data data to write at that address.
 */
void exSID_clkdwrite(uint_fast32_t cycles, uint_least8_t addr, uint8_t data)
{
	static int adj = 0;

#ifdef	DEBUG
	if (unlikely(addr > 0x18)) {
		dbg("Invalid write: %.2hxx\n", addr);
		exSID_delay(cycles);
		return;
	}
#endif

	// actual write will cost XS_CYCIO. Delay for cycles - XS_CYCIO then account for the write
	clkdrift += cycles;
	if (clkdrift > XS_CYCIO)
		_xSdelay(clkdrift - XS_CYCIO);

	clkdrift -= XS_CYCIO;	// write is going to consume XS_CYCIO clock ticks

	// if we are still going to be early, delay actual write by up to XS_MAXADJ ticks
	if (clkdrift > 0) {
		adj = clkdrift % (XS_MAXADJ*XS_ADJMLT+1);	// modulo gives much better results by spreading/evening jitter
		//adj = (clkdrift < XS_MAXADJ*XS_ADJMLT ? clkdrift : XS_MAXADJ*XS_ADJMLT);
		adj /= XS_ADJMLT;
		clkdrift -= adj*XS_ADJMLT;
		addr = (unsigned char)(addr | (adj << 5));	// final delay encoded in top 3 bits of address
		//dbg("adj: %d, addr: %.2hhx, data: %.2hhx\n", adj*XS_ADJMLT, (char)(addr | (adj << 5)), data);
	}

#ifdef	DEBUG
	acccycle += cycles;
	accdrift += clkdrift;
	accioops++;
#endif

	//dbg("delay: %d, clkdrift: %d\n", cycles, clkdrift);
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

	_xSoutb(addr, flush);	// XXX
	_xSread(&data, flush);	// blocking

	dbg("addr: %.2hhx, data: %.2hhx\n", addr, data);
	return data;
}

/**
 * Timed read routine, attempts cycle-accurate reads. BLOCKING.
 * This function will be cycle-accurate provided that no two consecutive reads or writes
 * are less than XS_CYCIO apart and leftover delay is <= 7 SID clock cycles.
 * Read result will only be available after a full XS_CYCIO, giving clkdread() the same
 * run time as clkdwrite().
 * @param cycles how many SID clocks to wait before the actual data read.
 * @param addr target address.
 * @return data read from address.
 */
uint8_t exSID_clkdread(uint_fast32_t cycles, uint_least8_t addr)
{
	static int adj = 0;

#ifdef	DEBUG
	if (unlikely((addr < 0x19) || (addr > 0x1C))) {
		dbg("Invalid read: %.2hxx\n", addr);
		exSID_delay(cycles);
		return 0xFF;
	}
#endif

	// actual read will cost XS_MINDEL. Delay for cycles - XS_MINDEL then account for the read
	clkdrift += cycles;
	if (clkdrift > XS_MINDEL)
		_xSdelay(clkdrift - XS_MINDEL);

	clkdrift -= XS_MINDEL;	// read is going to consume XS_MINDEL clock ticks

	// if we are still going to be early, delay actual read by up to XS_MAXADJ ticks
	if (clkdrift > 0) {
		adj = clkdrift % (XS_MAXADJ*XS_ADJMLT+1);	// modulo gives much better results by spreading/evening jitter
		//adj = (clkdrift < XS_MAXADJ*XS_ADJMLT ? clkdrift : XS_MAXADJ*XS_ADJMLT);
		adj /= XS_ADJMLT;
		clkdrift -= adj*XS_ADJMLT;
		addr = (unsigned char)(addr | (adj << 5));	// final delay encoded in top 3 bits of address
		//dbg("adj: %d, addr: %.2hhx\n", adj, (char)(addr | (adj << 5)));
	}

#ifdef	DEBUG
	acccycle += cycles;
	accdrift += clkdrift;
	accioops++;
#endif

	//dbg("delay: %d, clkdrift: %d\n", cycles, clkdrift);
	return _exSID_read(addr, 1);
}
