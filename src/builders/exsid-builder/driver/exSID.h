//
//  exSID.h
//	A simple I/O library for exSID USB - header file
//
//  (C) 2015 Thibaut VARENE
//  License: GPLv2 - http://www.gnu.org/licenses/gpl-2.0.html
//

#ifndef exSID_h
#define exSID_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#undef DEBUG
//#define DEBUG
#undef	__TARGET_16F88X			///< define if targetting the 16F88X PIC family

#define	XS_VERSION	"1.0"

// CLOCK_FREQ_NTSC = 1022727.14;
// CLOCK_FREQ_PAL  = 985248.4;

#ifdef	__TARGET_16F88X
#define	XS_BDRATE	1000000		///< 1Mpbs
#define	XS_BUFFSZ	1024		///< ~10ms buffer @1Mbps, multiple of 64. Penalty if too big (introduces delay in libsidplay) or too small (controller can't keep up)
#define XS_ADJMLT	1			///< 1-to-1 cycle adjustement (max resolution: 1 cycle)
#else
#define	XS_BDRATE	750000		///< 750kpbs
#define	XS_BUFFSZ	768			///< ~10ms buffer @750kbps, multiple of 64. Penalty if too big (introduces delay in libsidplay) or too small (controller can't keep up)
#define	XS_ADJMLT	2			///< 2-to-1 cycle adjustement (max resolution: 2 cycles).
#endif

#define	XS_SIDCLK	1000000		///< 1MHz (for computation only, currently hardcoded in hardware)
#define	XS_CYCCHR	XS_SIDCLK/(XS_BDRATE/10)	///< SID cycles between two consecutive chars
#define XS_USBLAT	2			///< FTDI latency: 2-255ms in 1ms increments

#define XS_MINDEL	(XS_CYCCHR)	///< Smallest possible delay (with IOCTD1).
#define	XS_CYCIO	(2*XS_CYCCHR)	///< minimum cycles between two consecutive I/Os
#define	XS_MAXADJ	7			///< maximum post write clock adjustment: must fit on 3 bits

#define XS_AD_IOCTD1	0x9D	///< shortest delay (XS_MINDEL SID cycles)
#define	XS_AD_IOCTLD	0x9E	///< polled delay, amount of SID cycles to wait must be given in data

#define	XS_AD_IOCTS0	0xBD	///< select chip 0
#define XS_AD_IOCTS1	0xBE	///< select chip 1
#define XS_AD_IOCTSB	0xBF	///< select both (invalid for reads, only chip 0 will be read from)

#define	XS_AD_IOCTFV	0xFD	///< Firmware version query
#define	XS_AD_IOCTHV	0xFE	///< Hardware version query
#define XS_AD_IOCTRS	0xFF	///< SID reset

#define	XS_CS_CHIP0	0			///< 6581
#define	XS_CS_CHIP1	1			///< 8580
#define	XS_CS_BOTH	2

#ifdef DEBUG
 #define dbg(format, ...)	printf("(%s) " format, __func__, ## __VA_ARGS__)
#else
 #define dbg(format, ...)	/* nothing */
#endif

#define	error(format, ...)	printf("(%s) ERROR " format, __func__, ## __VA_ARGS__)

#ifdef C_HAS_BUILTIN_EXPECT
 #define likely(x)       __builtin_expect(!!(x), 1)
 #define unlikely(x)     __builtin_expect(!!(x), 0)
#else
 #define likely(x)      (x)
 #define unlikely(x)    (x)
#endif

// public interface
int exSID_init(void);
void exSID_exit(void);
void exSID_reset(uint_least8_t volume);
uint16_t exSID_version(void);
void exSID_chipselect(int chip);
void exSID_delay(uint_fast32_t cycles);
void exSID_polldelay(uint_fast32_t cycles);
void exSID_clkdwrite(uint_fast32_t cycles, uint_least8_t addr, uint8_t data);
uint8_t exSID_clkdread(uint_fast32_t cycles, uint_least8_t addr);

#define exSID_write(addr, data)	exSID_clkdwrite(0, addr, data)
#define exSID_read(addr)		exSID_clkdread(0, addr)

#ifdef __cplusplus
}
#endif
#endif /* exSID_h */
