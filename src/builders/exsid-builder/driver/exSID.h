//
//  exSID.h
//	A simple I/O library for exSID USB - interface header file
//
//  (C) 2015-2017 Thibaut VARENE
//  License: GPLv2 - http://www.gnu.org/licenses/gpl-2.0.html
//

/**
 * @file
 * libexsid interface header file.
 */

#ifndef exSID_h
#define exSID_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define	XS_VERSION	"2.0pre"

/** Chip selection values for exSID_chipselect() */
enum {
	XS_CS_CHIP0,	///< 6581
	XS_CS_CHIP1,	///< 8580
	XS_CS_BOTH,	///< Both chips. @warning Invalid for reads: undefined behaviour!
};

/** Audio output operations for exSID_audio_op() */
enum {
	XS_AU_6581_8580,	///< mix: 6581 L / 8580 R
	XS_AU_8580_6581,	///< mix: 8580 L / 6581 R
	XS_AU_8580_8580,	///< mix: 8580 L and R
	XS_AU_6581_6581,	///< mix: 6581 L and R
	XS_AU_MUTE,		///< mute output
	XS_AU_UNMUTE,		///< unmute output
};

/** Clock selection values for exSID_clockselect() */
enum {
	XS_CL_PAL,		///< select PAL clock
	XS_CL_NTSC,		///< select NTSC clock
	XS_CL_1MHZ,		///< select 1MHz clock
};

/** Hardware model return values for exSID_hwmodel() */
enum {
	XS_MD_STD,		///< exSID USB
	XS_MD_PLUS,		///< exSID+ USB
};

// public interface
int exSID_init(void);
void exSID_exit(void);

void exSID_reset(uint_least8_t volume);
int exSID_hwmodel(void);
uint16_t exSID_hwversion(void);
int exSID_clockselect(int clock);
int exSID_audio_op(int operation);
void exSID_chipselect(int chip);
void exSID_delay(uint_fast32_t cycles);
void exSID_clkdwrite(uint_fast32_t cycles, uint_least8_t addr, uint8_t data);
uint8_t exSID_clkdread(uint_fast32_t cycles, uint_least8_t addr);
const char * exSID_error_str(void);

#define exSID_write(addr, data)	exSID_clkdwrite(0, addr, data)
#define exSID_read(addr)	exSID_clkdread(0, addr)

#ifdef __cplusplus
}
#endif
#endif /* exSID_h */
