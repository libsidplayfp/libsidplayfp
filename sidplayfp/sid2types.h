/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SID2TYPES_H
#define SID2TYPES_H

#include <stdint.h>

class sidbuilder;

typedef unsigned int uint;
typedef double   float64_t;

#ifndef SIDPLAY2_DEFAULTS
#define SIDPLAY2_DEFAULTS
    // Default settings
    const uint_least32_t SID2_DEFAULT_SAMPLING_FREQ  = 44100;
#endif // SIDPLAY2_DEFAULTS

typedef enum {sid2_mono = 1,  sid2_stereo} sid2_playback_t;
typedef enum {SID2_MOS6581, SID2_MOS8580} sid2_model_t;
typedef enum {SID2_CLOCK_PAL, SID2_CLOCK_NTSC} sid2_clock_t;
typedef enum {SID2_INTERPOLATE, SID2_RESAMPLE_INTERPOLATE} sampling_method_t;

/**
* sid2_config_t
*/
struct sid2_config_t
{
    /**
    * Intended tune speed when unknown or forced
    * - SID2_CLOCK_PAL
    * - SID2_CLOCK_NTSC
    */
    sid2_clock_t        clockDefault;
    bool                clockForced;

    bool                forceDualSids;
    bool                emulateStereo;

    /// Sampling frequency
    uint_least32_t      frequency;

    /**
    * Playbak mode
    * - sid2_mono
    * - sid2_stereo
    */
    sid2_playback_t     playback;

    /**
    * Intended sid model when unknown or forced
    * - SID2_MOS6581
    * - SID2_MOS8580
    */
    sid2_model_t        sidDefault;
    bool                forceModel;

    sidbuilder         *sidEmulation;

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

#endif // SID2TYPES_H
