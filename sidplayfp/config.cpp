/***************************************************************************
                          config.cpp  -  Library Configuration Code
                             -------------------
    begin                : Fri Jul 27 2001
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


#include "sid2types.h"
#include "player.h"
#include "sidbuilder.h"

SIDPLAY2_NAMESPACE_START

// An instance of this structure is used to transport emulator settings
// to and from the interface class.

int Player::config (const sid2_config_t &cfg)
{
    const SidTuneInfo* tuneInfo = 0;

    // Check for base sampling frequency
    if (cfg.frequency < 4000)
    {   // Rev 1.6 (saw) - Added descriptive error
        m_errorString = ERR_UNSUPPORTED_FREQ;
        goto Player_configure_error;
    }

    // Only do these if we have a loaded tune
    if (m_tune)
    {
        tuneInfo = m_tune->getInfo();

        // Determine clock speed
        const float64_t cpuFreq = clockSpeed (cfg.clockDefault, cfg.clockForced);

        // SID emulation setup (must be performed before the
        // environment setup call)
        if (sidCreate(cfg.sidEmulation, cfg.sidDefault, cfg.forceModel,
            cfg.playback == sid2_stereo ? 2 : 1, cpuFreq, cfg.frequency, cfg.samplingMethod) < 0) {
            m_errorString = cfg.sidEmulation->error();
            m_cfg.sidEmulation = NULL;
            goto Player_configure_restore;
        }

        m_c64.setMainCpuSpeed(cpuFreq);

        // Configure, setup and install C64 environment/events
        if (initialise() < 0) {
            goto Player_configure_error;
        }
    }

    m_c64.resetSIDMapper();
    if (m_tune && tuneInfo->sidChipBase2()) {
        // Assumed to be in d4xx-d7xx range
        m_c64.setSecondSIDAddress(tuneInfo->sidChipBase2());
        m_info.channels = 2;
    } else if (cfg.forceDualSids) {
        /* Tune didn't tell us where; let's put the second SID
         * in every slot apart from 0xd400 - 0xd420. */
        for (int i = 0xd420; i < 0xd7ff; i += 0x20)
            m_c64.setSecondSIDAddress(i);
        m_info.channels = 2;
    } else
        m_info.channels = 1;

    /* without stereo SID mode, we don't emulate the second chip! */
    if (m_info.channels == 1)
        m_c64.setSid(1, 0);

    m_mixer.setSids(m_c64.getSid(0), m_c64.getSid(1));
    m_mixer.setStereo(cfg.playback == sid2_stereo);
    m_mixer.setVolume(cfg.leftVolume, cfg.rightVolume);

    // Update Configuration
    m_cfg = cfg;

    return 0;

Player_configure_restore:
    // Try restoring old configuration
    if (&m_cfg != &cfg)
        config (m_cfg);
Player_configure_error:
    return -1;
}

// Clock speed changes due to loading a new song
float64_t Player::clockSpeed (const sid2_clock_t defaultClock, const bool forced)
{
    const SidTuneInfo* tuneInfo = m_tune->getInfo();

    SidTuneInfo::clock_t clockSpeed = tuneInfo->clockSpeed();

    // Use preferred speed if forced or if song speed is unknown
    if (forced || clockSpeed == SidTuneInfo::CLOCK_UNKNOWN || clockSpeed == SidTuneInfo::CLOCK_ANY)
    {
        switch (defaultClock)
        {
        case SID2_CLOCK_PAL:
            clockSpeed = SidTuneInfo::CLOCK_PAL;
            break;
        case SID2_CLOCK_NTSC:
            clockSpeed = SidTuneInfo::CLOCK_NTSC;
            break;
        }
    }

    float64_t cpuFreq;

    switch (clockSpeed)
    {
    case SidTuneInfo::CLOCK_PAL:
        cpuFreq = c64::CLOCK_FREQ_PAL;
        if (tuneInfo->songSpeed() == SidTuneInfo::SPEED_CIA_1A)
            m_info.speedString = TXT_PAL_CIA;
        else if (tuneInfo->clockSpeed() == SidTuneInfo::CLOCK_NTSC)
            m_info.speedString = TXT_PAL_VBI_FIXED;
        else
            m_info.speedString = TXT_PAL_VBI;
        break;
    case SidTuneInfo::CLOCK_NTSC:
        cpuFreq = c64::CLOCK_FREQ_NTSC;
        if (tuneInfo->songSpeed() == SidTuneInfo::SPEED_CIA_1A)
            m_info.speedString = TXT_NTSC_CIA;
        else if (tuneInfo->clockSpeed() == SidTuneInfo::CLOCK_PAL)
            m_info.speedString = TXT_NTSC_VBI_FIXED;
        else
            m_info.speedString = TXT_NTSC_VBI;
        break;
    }

    return cpuFreq;
}

sid2_model_t Player::getModel(const SidTuneInfo::model_t sidModel, const sid2_model_t defaultModel, const bool forced)
{
    SidTuneInfo::model_t tuneModel = sidModel;

    // Use preferred speed if forced or if song speed is unknown
    if (forced || tuneModel == SidTuneInfo::SIDMODEL_UNKNOWN || tuneModel == SidTuneInfo::SIDMODEL_ANY)
    {
        switch (defaultModel)
        {
        case SID2_MOS6581:
            tuneModel = SidTuneInfo::SIDMODEL_6581;
            break;
        case SID2_MOS8580:
            tuneModel = SidTuneInfo::SIDMODEL_8580;
            break;
        }
    }

    sid2_model_t newModel;

    switch (tuneModel)
    {
    case SidTuneInfo::CLOCK_PAL:
        newModel = SID2_MOS6581;
        break;
    case SidTuneInfo::CLOCK_NTSC:
        newModel = SID2_MOS8580;
        break;
    }

    return newModel;
}

int Player::sidCreate (sidbuilder *builder, const sid2_model_t defaultModel,
                       const bool forced, const int channels,
                       const float64_t cpuFreq, const int frequency,
                       const sampling_method_t sampling)
{
    for (int i = 0; i < SidBank::MAX_SIDS; i++)
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
        sid2_model_t userModels[SidBank::MAX_SIDS];

        const SidTuneInfo* tuneInfo = m_tune->getInfo();

        userModels[0] = getModel(tuneInfo->sidModel1(), defaultModel, forced);
        // If bits 6-7 are set to Unknown then the second SID will be set to the same SID
        // model as the first SID.
        userModels[1] = getModel(tuneInfo->sidModel2(), userModels[0], forced);

        for (int i = 0; i < channels; i++)
        {   // Get first SID emulation
            sidemu *s = builder->lock (m_c64.getEventScheduler(), userModels[i]);
            if ((i == 0) && !builder->getStatus())
                return -1;
            if (s)
                s->sampling((long)cpuFreq, frequency, sampling, false);
            m_c64.setSid(i, s);
        }
    }

    return 0;
}

SIDPLAY2_NAMESPACE_STOP
