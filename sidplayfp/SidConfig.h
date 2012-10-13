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

#ifndef SIDCONFIG_H
#define SIDCONFIG_H

#include "siddefs.h"

#include <stdint.h>


class sidbuilder;

typedef enum {sid2_mono = 1,  sid2_stereo} sid2_playback_t;
typedef enum {SID2_MOS6581, SID2_MOS8580} sid2_model_t;
typedef enum {SID2_CLOCK_PAL, SID2_CLOCK_NTSC} sid2_clock_t;
typedef enum {SID2_INTERPOLATE, SID2_RESAMPLE_INTERPOLATE} sampling_method_t;

/**
* SidConfig
*/
class SID_EXTERN SidConfig
{
public:
    /**
    * Maximum power on delay
    * Delays <= MAX produce constant results.
    * Delays >  MAX produce random results
    */
    static const uint_least16_t MAX_POWER_ON_DELAY = 0x1FFF;
    static const uint_least16_t DEFAULT_POWER_ON_DELAY = MAX_POWER_ON_DELAY + 1;

    static const uint_least32_t DEFAULT_SAMPLING_FREQ  = 44100;

public:
    /**
    * Intended tune speed when unknown or forced
    * - SID2_CLOCK_PAL
    * - SID2_CLOCK_NTSC
    */
    sid2_clock_t        clockDefault;

    /**
    * Force the clock to clockDefault
    */
    bool                clockForced;

    /**
    * Force an environment with two SID chips installed
    */
    bool                forceDualSids;

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

    /**
    * Force the sid model to sidDefault
    */
    bool                forceModel;

    sidbuilder         *sidEmulation;

    /**
    * Left channel volume
    */
    uint_least32_t      leftVolume;

    /**
    * Right channel volume
    */
    uint_least32_t      rightVolume;

    /**
    * Power on delay cycles
    */
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

public:
    SidConfig();
};

#endif // SIDCONFIG_H
