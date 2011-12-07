/***************************************************************************
                          sid2types.h  -  sidplay2 specific types
                             -------------------
    begin                : Fri Aug 10 2001
    copyright            : (C) 2001 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _sid2types_h_
#define _sid2types_h_

#include <stdint.h>

class   sidbuilder;
class   EventContext;
struct  SidTuneInfo;

typedef unsigned int uint;
typedef double   float64_t;

#ifndef SIDPLAY2_DEFAULTS
#define SIDPLAY2_DEFAULTS
    // Maximum values
    // Delays <= MAX produce constant results.
    // Delays >  MAX produce random results
    const uint_least16_t SID2_MAX_POWER_ON_DELAY = 0x1FFF;
    // Default settings
    const uint_least32_t SID2_DEFAULT_SAMPLING_FREQ  = 44100;
    const bool           SID2_DEFAULT_SID_SAMPLES    = true; // Samples through sid
    const uint_least16_t SID2_DEFAULT_POWER_ON_DELAY = SID2_MAX_POWER_ON_DELAY + 1;
#endif // SIDPLAY2_DEFAULTS

typedef enum {sid2_playing = 0, sid2_paused, sid2_stopped} sid2_player_t;
typedef enum {sid2_mono = 1,  sid2_stereo} sid2_playback_t;
typedef enum {SID2_MODEL_CORRECT, SID2_MOS6581, SID2_MOS8580} sid2_model_t;
typedef enum {SID2_CLOCK_CORRECT, SID2_CLOCK_PAL, SID2_CLOCK_NTSC} sid2_clock_t;
typedef enum {SID2_INTERPOLATE, SID2_RESAMPLE_INTERPOLATE} sampling_method_t;

/**
* sid2_config_t
*/
struct sid2_config_t
{
    /// Intended tune speed when unknown
    sid2_clock_t        clockDefault;
    bool                clockForced;
    /// User requested emulation speed
    sid2_clock_t        clockSpeed;
    bool                forceDualSids;
    bool                emulateStereo;
    uint_least32_t      frequency;
    /**
    * Playbak mode
    * - sid2_mono
    * - sid2_stereo
    */
    sid2_playback_t     playback;
    /**
    * Intended sid model when unknown
    * - SID2_MODEL_CORRECT
    * - SID2_MOS6581
    * - SID2_MOS8580
    */
    sid2_model_t        sidDefault;
    sidbuilder         *sidEmulation;
    /// User requested sid model
    sid2_model_t        sidModel;
    uint_least32_t      leftVolume;
    uint_least32_t      rightVolume;
    uint_least16_t      powerOnDelay;

    /**
    * Sampling method
    * - SID2_INTERPOLATE
    * - SID2_RESAMPLE_INTERPOLATE
    */
    sampling_method_t   samplingMethod;
    /**
    * Faster low-quality emulation
    * available only for reSID
    */
    bool                fastSampling;
};

/**
* sid2_info_t
*/
struct sid2_info_t
{
    const char       **credits;
    uint               channels;
    uint_least16_t     driverAddr;
    uint_least16_t     driverLength;
    const char        *name;
    const SidTuneInfo *tuneInfo; // May not need this
    const char        *version;
    // load, config and stop calls will reset this
    // and remove all pending events! 10th sec resolution.
    EventContext      *eventContext;
    uint               maxsids;
    uint_least16_t     powerOnDelay;
};

#endif // _sid2types_h_
