/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

#include "player.h"

#include "SidConfig.h"
#include "sidbuilder.h"

SIDPLAYFP_NAMESPACE_START

const char  TXT_PAL_VBI[]        = "50 Hz VBI (PAL)";
const char  TXT_PAL_VBI_FIXED[]  = "60 Hz VBI (PAL FIXED)";
const char  TXT_PAL_CIA[]        = "CIA (PAL)";
const char  TXT_PAL_UNKNOWN[]    = "UNKNOWN (PAL)";
const char  TXT_NTSC_VBI[]       = "60 Hz VBI (NTSC)";
const char  TXT_NTSC_VBI_FIXED[] = "50 Hz VBI (NTSC FIXED)";
const char  TXT_NTSC_CIA[]       = "CIA (NTSC)";
const char  TXT_NTSC_UNKNOWN[]   = "UNKNOWN (NTSC)";

// Error Strings
const char  ERR_UNSUPPORTED_FREQ[]      = "SIDPLAYER ERROR: Unsupported sampling frequency.";
const char  ERR_UNSUPPORTED_PRECISION[] = "SIDPLAYER ERROR: Unsupported sample precision.";

// An instance of this structure is used to transport emulator settings
// to and from the interface class.

bool Player::config (const SidConfig &cfg)
{
    const SidTuneInfo* tuneInfo = 0;

    // Check for base sampling frequency
    if (cfg.frequency < 4000)
    {
        m_errorString = ERR_UNSUPPORTED_FREQ;
        return false;
    }

    // Only do these if we have a loaded tune
    if (m_tune)
    {
        tuneInfo = m_tune->getInfo();

        // Determine clock speed
        const double cpuFreq = clockSpeed (cfg.clockDefault, cfg.clockForced);

        // SID emulation setup (must be performed before the
        // environment setup call)
        if (!sidCreate(cfg.sidEmulation, cfg.sidDefault, cfg.forceModel,
            cfg.playback == SidConfig::STEREO ? 2 : 1, cpuFreq, cfg.frequency,
            cfg.samplingMethod, cfg.fastSampling))
        {
            m_errorString = cfg.sidEmulation->error();
            m_cfg.sidEmulation = 0;
            if (&m_cfg != &cfg)
                config (m_cfg);
            return false;
        }

        m_c64.setMainCpuSpeed(cpuFreq);

        // Configure, setup and install C64 environment/events
        if (!initialise())
        {
            return false;
        }
    }

    m_c64.resetSIDMapper();
    if (m_tune && tuneInfo->sidChipBase2())
    {
        // Assumed to be in d4xx-d7xx range
        m_c64.setSecondSIDAddress(tuneInfo->sidChipBase2());
        m_info.m_channels = 2;
    }
    else if (cfg.forceDualSids)
    {
        /* Tune didn't tell us where; let's put the second SID
         * in every slot apart from 0xd400 - 0xd420. */
        for (int i = 0xd420; i < 0xd7ff; i += 0x20)
            m_c64.setSecondSIDAddress(i);
        m_info.m_channels = 2;
    }
    else
    {
        m_info.m_channels = 1;
        // without stereo SID mode, we don't emulate the second chip!
        m_c64.setSid(1, 0);
    }

    m_mixer.setSids(m_c64.getSid(0), m_c64.getSid(1));
    m_mixer.setStereo(cfg.playback == SidConfig::STEREO);
    m_mixer.setVolume(cfg.leftVolume, cfg.rightVolume);

    // Update Configuration
    m_cfg = cfg;

    return true;
}

// Clock speed changes due to loading a new song
double Player::clockSpeed (const SidConfig::clock_t defaultClock, const bool forced)
{
    const SidTuneInfo* tuneInfo = m_tune->getInfo();

    SidTuneInfo::clock_t clockSpeed = tuneInfo->clockSpeed();

    // Use preferred speed if forced or if song speed is unknown
    if (forced || clockSpeed == SidTuneInfo::CLOCK_UNKNOWN || clockSpeed == SidTuneInfo::CLOCK_ANY)
    {
        switch (defaultClock)
        {
        case SidConfig::CLOCK_PAL:
            clockSpeed = SidTuneInfo::CLOCK_PAL;
            break;
        case SidConfig::CLOCK_NTSC:
            clockSpeed = SidTuneInfo::CLOCK_NTSC;
            break;
        }
    }

    double cpuFreq;

    switch (clockSpeed)
    {
    case SidTuneInfo::CLOCK_PAL:
        cpuFreq = c64::CLOCK_FREQ_PAL;
        if (tuneInfo->songSpeed() == SidTuneInfo::SPEED_CIA_1A)
            m_info.m_speedString = TXT_PAL_CIA;
        else if (tuneInfo->clockSpeed() == SidTuneInfo::CLOCK_NTSC)
            m_info.m_speedString = TXT_PAL_VBI_FIXED;
        else
            m_info.m_speedString = TXT_PAL_VBI;
        break;
    case SidTuneInfo::CLOCK_NTSC:
        cpuFreq = c64::CLOCK_FREQ_NTSC;
        if (tuneInfo->songSpeed() == SidTuneInfo::SPEED_CIA_1A)
            m_info.m_speedString = TXT_NTSC_CIA;
        else if (tuneInfo->clockSpeed() == SidTuneInfo::CLOCK_PAL)
            m_info.m_speedString = TXT_NTSC_VBI_FIXED;
        else
            m_info.m_speedString = TXT_NTSC_VBI;
        break;
    }

    return cpuFreq;
}

SidConfig::model_t Player::getModel(const SidTuneInfo::model_t sidModel, const SidConfig::model_t defaultModel, const bool forced)
{
    SidTuneInfo::model_t tuneModel = sidModel;

    // Use preferred speed if forced or if song speed is unknown
    if (forced || tuneModel == SidTuneInfo::SIDMODEL_UNKNOWN || tuneModel == SidTuneInfo::SIDMODEL_ANY)
    {
        switch (defaultModel)
        {
        case SidConfig::MOS6581:
            tuneModel = SidTuneInfo::SIDMODEL_6581;
            break;
        case SidConfig::MOS8580:
            tuneModel = SidTuneInfo::SIDMODEL_8580;
            break;
        }
    }

    SidConfig::model_t newModel;

    switch (tuneModel)
    {
    case SidTuneInfo::CLOCK_PAL:
        newModel = SidConfig::MOS6581;
        break;
    case SidTuneInfo::CLOCK_NTSC:
        newModel = SidConfig::MOS8580;
        break;
    }

    return newModel;
}

bool Player::sidCreate (sidbuilder *builder, const SidConfig::model_t defaultModel,
                       const bool forced, const int channels,
                       const double cpuFreq, const int frequency,
                       const SidConfig::sampling_method_t sampling, const bool fastSampling)
{
    for (unsigned int i = 0; i < SidBank::MAX_SIDS; i++)
    {
        sidemu *s = m_c64.getSid(i);
        if (s)
        {
            sidbuilder *b = s->builder ();
            if (b)
                b->unlock (s);
            m_c64.setSid(i, 0);
        }
    }

    if (builder)
    {   // Detect the Correct SID model
        // Determine model when unknown
        SidConfig::model_t userModels[SidBank::MAX_SIDS];

        const SidTuneInfo* tuneInfo = m_tune->getInfo();

        userModels[0] = getModel(tuneInfo->sidModel1(), defaultModel, forced);
        // If bits 6-7 are set to Unknown then the second SID will be set to the same SID
        // model as the first SID.
        userModels[1] = getModel(tuneInfo->sidModel2(), userModels[0], forced);

        for (int i = 0; i < channels; i++)
        {   // Get first SID emulation
            sidemu *s = builder->lock (m_c64.getEventScheduler(), userModels[i]);
            if ((i == 0) && !builder->getStatus())
                return false;
            if (s)
                s->sampling((long)cpuFreq, frequency, sampling, fastSampling);
            m_c64.setSid(i, s);
        }
    }

    return true;
}

SIDPLAYFP_NAMESPACE_STOP
