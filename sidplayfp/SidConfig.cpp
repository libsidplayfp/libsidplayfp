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

#include "SidConfig.h"

#include "mixer.h"

SidConfig::SidConfig() :
    clockDefault(SID2_CLOCK_PAL),
    clockForced(false),
    forceDualSids(false),
    frequency(DEFAULT_SAMPLING_FREQ),
    playback(sid2_mono),
    sidDefault(SID2_MOS6581),
    forceModel(false),
    sidEmulation(0),
    leftVolume(Mixer::VOLUME_MAX),
    rightVolume(Mixer::VOLUME_MAX),
    powerOnDelay(DEFAULT_POWER_ON_DELAY),
    samplingMethod(SID2_RESAMPLE_INTERPOLATE),
    fastSampling(false)
{}
