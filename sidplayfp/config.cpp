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

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

SIDPLAY2_NAMESPACE_START

// An instance of this structure is used to transport emulator settings
// to and from the interface class.

int Player::config (const sid2_config_t &cfg)
{
    if (m_running)
    {
        m_errorString = ERR_CONF_WHILST_ACTIVE;
        goto Player_configure_error;
    }

    // Check for base sampling frequency
    if (cfg.frequency < 4000)
    {   // Rev 1.6 (saw) - Added descriptive error
        m_errorString = ERR_UNSUPPORTED_FREQ;
        goto Player_configure_error;
    }

    // Only do these if we have a loaded tune
    if (m_tune)
    {
        if (m_playerState != sid2_paused)
            m_tune->getInfo(m_tuneInfo);

        // SID emulation setup (must be performed before the
        // environment setup call)
        if (sidCreate(cfg.sidEmulation, cfg.sidModel, cfg.sidDefault) < 0) {
            m_errorString = cfg.sidEmulation->error();
            m_cfg.sidEmulation = NULL;
            goto Player_configure_restore;
        }

        if (m_playerState != sid2_paused)
        {
            // Must be this order:
            // Determine clock speed
            m_cpuFreq = clockSpeed (cfg.clockSpeed, cfg.clockDefault,
                                  cfg.clockForced);

            const float64_t clockPAL = m_cpuFreq / VIC_FREQ_PAL;
            const float64_t clockNTSC = m_cpuFreq / VIC_FREQ_NTSC;

            // Setup fake cia
            if (m_tuneInfo.songSpeed  == SIDTUNE_SPEED_CIA_1A ||
                m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_NTSC)
            {
                sid6526.clock ((uint_least16_t)(clockNTSC + 0.5));
            }
            else
            {
                sid6526.clock ((uint_least16_t)(clockPAL + 0.5));
            }

            // @FIXME@ see mos6526.h for details. Setup TOD clock
            if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_PAL)
            {
                cia.clock  (clockPAL);
                cia2.clock (clockPAL);
                vic.chip   (MOS6569);
            }
            else
            {
                cia.clock  (clockNTSC);
                cia2.clock (clockNTSC);
                vic.chip   (MOS6567R8);
            }

            // Configure, setup and install C64 environment/events
            if (environment(cfg.environment) < 0) {
                goto Player_configure_error;
            }

            /* inform ReSID of the desired sampling rate */
            for (int i = 0; i < SID2_MAX_SIDS; i += 1) {
                sid[i]->sampling((long)m_cpuFreq, cfg.frequency, cfg.samplingMethod, false);
            }
        }
    }
    sidSamples (cfg.sidSamples);

    // Setup sid mapping table
    // Note this should be based on m_tuneInfo.sidChipBase1
    // but this is only temporary code anyway
    for (int i = 0; i < SID2_MAPPER_SIZE; i++)
        m_sidmapper[i] = 0;

    if (m_tune && m_tuneInfo.sidChipBase2) {
        // Assumed to be in d4xx-d7xx range
        m_sidmapper[(m_tuneInfo.sidChipBase2 >> 5) &
                    (SID2_MAPPER_SIZE - 1)] = 1;
        m_info.channels = 2;
    } else if (cfg.forceDualSids) {
        /* Tune didn't tell us where; let's put the second SID
         * in every slot apart from 0xd400 - 0xd420. */
        for (int i = 0xd420; i < 0xd7ff; i += 0x20)
            m_sidmapper[(i >> 5) & (SID2_MAPPER_SIZE - 1)] = 1;
        m_info.channels = 2;
    } else
        m_info.channels = 1;

    m_leftVolume  = cfg.leftVolume;
    m_rightVolume = cfg.rightVolume;

    /* without stereo SID mode, we don't emulate the second chip! */
    if (m_info.channels == 1)
        sid[1] = &nullsid;

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
float64_t Player::clockSpeed (sid2_clock_t userClock, sid2_clock_t defaultClock,
                              bool forced)
{
    float64_t cpuFreq = CLOCK_FREQ_PAL;

    // Detect the Correct Song Speed
    // Determine song speed when unknown
    if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_UNKNOWN)
    {
        switch (defaultClock)
        {
        case SID2_CLOCK_PAL:
            m_tuneInfo.clockSpeed = SIDTUNE_CLOCK_PAL;
            break;
        case SID2_CLOCK_NTSC:
            m_tuneInfo.clockSpeed = SIDTUNE_CLOCK_NTSC;
            break;
        case SID2_CLOCK_CORRECT:
            // No default so base it on emulation clock
            m_tuneInfo.clockSpeed = SIDTUNE_CLOCK_ANY;
        }
    }

    // Since song will run correct at any clock speed
    // set tune speed to the current emulation
    if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_ANY)
    {
        if (userClock == SID2_CLOCK_CORRECT)
            userClock  = defaultClock;

        m_tuneInfo.clockSpeed = (userClock == SID2_CLOCK_NTSC) ?
            SIDTUNE_CLOCK_NTSC : SIDTUNE_CLOCK_PAL;
    }

    if (userClock == SID2_CLOCK_CORRECT)
    {
        switch (m_tuneInfo.clockSpeed)
        {
        case SIDTUNE_CLOCK_NTSC:
            userClock = SID2_CLOCK_NTSC;
            break;
        case SIDTUNE_CLOCK_PAL:
            userClock = SID2_CLOCK_PAL;
            break;
        }
    }

    if (forced)
    {
        m_tuneInfo.clockSpeed = (userClock == SID2_CLOCK_NTSC) ?
            SIDTUNE_CLOCK_NTSC : SIDTUNE_CLOCK_PAL;
    }

    if (userClock == SID2_CLOCK_PAL)
    {
        cpuFreq = CLOCK_FREQ_PAL;
        m_tuneInfo.speedString = TXT_PAL_VBI;
        if (m_tuneInfo.songSpeed == SIDTUNE_SPEED_CIA_1A)
            m_tuneInfo.speedString = TXT_PAL_CIA;
        else if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_NTSC)
            m_tuneInfo.speedString = TXT_PAL_VBI_FIXED;
    }
    else // if (userClock == SID2_CLOCK_NTSC)
    {
        cpuFreq = CLOCK_FREQ_NTSC;
        m_tuneInfo.speedString = TXT_NTSC_VBI;
        if (m_tuneInfo.songSpeed == SIDTUNE_SPEED_CIA_1A)
            m_tuneInfo.speedString = TXT_NTSC_CIA;
        else if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_PAL)
            m_tuneInfo.speedString = TXT_NTSC_VBI_FIXED;
    }
    return cpuFreq;
}

int Player::environment (sid2_env_t env)
{
    switch (m_tuneInfo.compatibility)
    {
    case SIDTUNE_COMPATIBILITY_R64:
    case SIDTUNE_COMPATIBILITY_BASIC:
        env = sid2_envR;
        break;
    case SIDTUNE_COMPATIBILITY_PSID:
        if (env == sid2_envR)
            env  = sid2_envBS;
    }

    // Environment not set?
    if (mmu.getRom() == 0 || m_info.environment != env)
    {   // Setup new player environment
        m_info.environment = env;
        mmu.setEnv(env);
        // Setup the access functions to the environment
        // and the properties the memory has.
        if (m_info.environment == sid2_envPS)
        {   // Playsid has no roms and SID exists in ram space
            m_readMemByte     = &Player::readMemByte_plain;
            m_writeMemByte    = &Player::writeMemByte_playsid;
            m_readMemDataByte = &Player::readMemByte_plain;
        }
        else
        {
            switch (m_info.environment)
            {
            case sid2_envTP:
                m_readMemByte     = &Player::readMemByte_plain;
                m_writeMemByte    = &Player::writeMemByte_sidplay;
                m_readMemDataByte = &Player::readMemByte_sidplaytp;
            break;

            //case sid2_envTR:
            case sid2_envBS:
                m_readMemByte     = &Player::readMemByte_plain;
                m_writeMemByte    = &Player::writeMemByte_sidplay;
                m_readMemDataByte = &Player::readMemByte_sidplaybs;
            break;

            case sid2_envR:
            default: // <-- Just to please compiler
                m_readMemByte     = &Player::readMemByte_sidplaybs;
                m_writeMemByte    = &Player::writeMemByte_sidplay;
                m_readMemDataByte = &Player::readMemByte_sidplaybs;
            break;
            }
        }
    }

    return initialise ();
}

sid2_model_t Player::getModel(int sidModel,
                              sid2_model_t userModel,
                              sid2_model_t defaultModel)
{
    if (sidModel == SIDTUNE_SIDMODEL_UNKNOWN)
    {
        switch (defaultModel)
        {
        case SID2_MOS6581:
            sidModel = SIDTUNE_SIDMODEL_6581;
            break;
        case SID2_MOS8580:
            sidModel = SIDTUNE_SIDMODEL_8580;
            break;
        case SID2_MODEL_CORRECT:
            // No default so base it on emulation clock
            sidModel = SIDTUNE_SIDMODEL_ANY;
        }
    }

    // Since song will run correct on any sid model
    // set it to the current emulation
    if (sidModel == SIDTUNE_SIDMODEL_ANY)
    {
        if (userModel == SID2_MODEL_CORRECT)
            userModel  = defaultModel;

        switch (userModel)
        {
        case SID2_MOS8580:
            sidModel = SIDTUNE_SIDMODEL_8580;
            break;
        case SID2_MOS6581:
        default:
            sidModel = SIDTUNE_SIDMODEL_6581;
            break;
        }
    }

    switch (userModel)
    {
    case SID2_MODEL_CORRECT:
        switch (sidModel)
        {
        case SIDTUNE_SIDMODEL_8580:
            userModel = SID2_MOS8580;
            break;
        case SIDTUNE_SIDMODEL_6581:
            userModel = SID2_MOS6581;
            break;
        }
    break;
    // Fixup tune information if model is forced
    case SID2_MOS6581:
        sidModel = SIDTUNE_SIDMODEL_6581;
        break;
    case SID2_MOS8580:
        sidModel = SIDTUNE_SIDMODEL_8580;
        break;
    }

    return userModel;
}

// Integrate SID emulation from the builder class into
// libsidplay2
int Player::sidCreate (sidbuilder *builder, sid2_model_t userModel,
                       sid2_model_t defaultModel)
{
    // Release old sids
    for (int i = 0; i < SID2_MAX_SIDS; i++)
    {
        sidbuilder *b = sid[i]->builder ();
        if (b)
            b->unlock (sid[i]);
    }

    if (!builder)
    {   // No sid
        for (int i = 0; i < SID2_MAX_SIDS; i++)
            sid[i] = &nullsid;
    }
    else
    {   // Detect the Correct SID model
        // Determine model when unknown
        sid2_model_t userModels[SID2_MAX_SIDS];

        userModels[0] = getModel(m_tuneInfo.sidModel1, userModel, defaultModel);
        // If bits 6-7 are set to Unknown then the second SID will be set to the same SID
        // model as the first SID.
        userModels[1] = getModel(m_tuneInfo.sidModel2, userModel, userModels[0]);

        for (int i = 0; i < SID2_MAX_SIDS; i++)
        {   // Get first SID emulation
            sid[i] = builder->lock (this, userModels[i]);
            if (!sid[i])
                sid[i] = &nullsid;
            if ((i == 0) && !builder->getStatus())
                return -1;
        }
    }

    /* put XSID as sid #0 in case we aren't in a real c64 environment */
    if (m_tuneInfo.compatibility == SIDTUNE_COMPATIBILITY_PSID) {
        xsid.emulation (sid[0]);
        sid[0] = &xsid;
    }

    return 0;
}

void Player::sidSamples (bool enable)
{
    //int_least8_t gain = 0;
    xsid.sidSamples (enable);
}

SIDPLAY2_NAMESPACE_STOP
