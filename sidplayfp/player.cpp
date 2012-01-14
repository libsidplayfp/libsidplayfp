/***************************************************************************
                          player.cpp  -  Main Library Code
                             -------------------
    begin                : Fri Jun 9 2000
    copyright            : (C) 2000 by Simon White
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


#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sidendian.h"
#include "player.h"

#ifndef PACKAGE_NAME
#   define PACKAGE_NAME PACKAGE
#endif

#ifndef PACKAGE_VERSION
#   define PACKAGE_VERSION VERSION
#endif


SIDPLAY2_NAMESPACE_START


// These texts are used to override the sidtune settings.
const char  Player::TXT_PAL_VBI[]        = "50 Hz VBI (PAL)";
const char  Player::TXT_PAL_VBI_FIXED[]  = "60 Hz VBI (PAL FIXED)";
const char  Player::TXT_PAL_CIA[]        = "CIA (PAL)";
const char  Player::TXT_PAL_UNKNOWN[]    = "UNKNOWN (PAL)";
const char  Player::TXT_NTSC_VBI[]       = "60 Hz VBI (NTSC)";
const char  Player::TXT_NTSC_VBI_FIXED[] = "50 Hz VBI (NTSC FIXED)";
const char  Player::TXT_NTSC_CIA[]       = "CIA (NTSC)";
const char  Player::TXT_NTSC_UNKNOWN[]   = "UNKNOWN (NTSC)";
const char  Player::TXT_NA[]             = "NA";

// Error Strings
const char  Player::ERR_UNSUPPORTED_FREQ[]      = "SIDPLAYER ERROR: Unsupported sampling frequency.";
const char  Player::ERR_UNSUPPORTED_PRECISION[] = "SIDPLAYER ERROR: Unsupported sample precision.";
const char  Player::ERR_UNKNOWN_ROM[]           = "SIDPLAYER ERROR: Unknown Rom.";

const char  *Player::credit[];


Player::Player (void)
// Set default settings for system
:m_mixer (m_c64.getEventScheduler()),
 m_tune (NULL),
 m_errorString(TXT_NA),
 m_mileage(0),
 m_playerState(sid2_stopped),
#if EMBEDDED_ROMS
 m_status            (true)
#else
 m_status            (false)
#endif
{
    srand ((uint) ::time(NULL));
    m_rand = (uint_least32_t) rand ();

    // Setup exported info
    m_info.credits         = credit;
    m_info.channels        = 1;
    m_info.driverAddr      = 0;
    m_info.driverLength    = 0;
    m_info.name            = PACKAGE_NAME;
    m_info.version         = PACKAGE_VERSION;
    // Number of SIDs support by this library
    m_info.maxsids         = c64::MAX_SIDS;

    // Configure default settings
    m_cfg.clockDefault    = SID2_CLOCK_CORRECT;
    m_cfg.clockForced     = false;
    m_cfg.clockSpeed      = SID2_CLOCK_CORRECT;
    m_cfg.forceDualSids   = false;
    m_cfg.frequency       = SID2_DEFAULT_SAMPLING_FREQ;
    m_cfg.playback        = sid2_mono;
    m_cfg.sidDefault      = SID2_MODEL_CORRECT;
    m_cfg.sidEmulation    = NULL;
    m_cfg.sidModel        = SID2_MODEL_CORRECT;
    m_cfg.leftVolume      = 255;
    m_cfg.rightVolume     = 255;
    m_cfg.powerOnDelay    = SID2_DEFAULT_POWER_ON_DELAY;
    m_cfg.samplingMethod  = SID2_RESAMPLE_INTERPOLATE;
    m_cfg.fastSampling    = false;

    config (m_cfg);

    // Get component credits
    credit[0] = PACKAGE_NAME " V" PACKAGE_VERSION " Engine:\n"
                "\tCopyright (C) 2000 Simon White\n"
                "\tCopyright (C) 2007-2010 Antti Lankila\n"
                "\tCopyright (C) 2010-2012 Leandro Nini\n"
                "\thttp://sourceforge.net/projects/sidplay-residfp/\n";
    credit[1] = m_c64.cpuCredits ();
    credit[2] = m_c64.ciaCredits ();
    credit[3] = m_c64.vicCredits ();
    credit[4] = 0;
}

uint16_t Player::getChecksum(const uint8_t* rom, const int size)
{
    uint16_t checksum = 0;
    for (int i=0; i<size; i++)
        checksum += rom[i];

    return checksum;
}

void Player::setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
{
    // kernal is mandatory
    if (!kernal)
        return;

    // Verify rom checksums and accept only known ones

    // Kernal revision is located at 0xff80
    const uint8_t rev = kernal[0xff80 - 0xe000];

    const uint16_t kChecksum = getChecksum(kernal, 0x2000);
    // second revision + Japanese version
    if ((rev == 0x00 && (kChecksum != 0xc70b && kChecksum != 0xd183))
        // third revision
        || (rev == 0x03 && kChecksum != 0xc70a)
        // first revision
        || (rev == 0xaa && kChecksum != 0xd4fd)
        // Commodore SX-64
        || (rev == 0x43 && kChecksum != 0xc70b))
    {
        m_errorString = ERR_UNKNOWN_ROM;
        return;
    }

    if (basic)
    {
        const uint16_t checksum = getChecksum(basic, 0x2000);
        // Commodore 64 BASIC V2
        if (checksum != 0x3d56)
        {
            m_errorString = ERR_UNKNOWN_ROM;
            return;
        }
    }

    if (character)
    {
        const uint16_t checksum = getChecksum(character, 0x1000);
        if (checksum != 0xf7f8
            && checksum != 0xf800)
        {
            m_errorString = ERR_UNKNOWN_ROM;
            return;
        }
    }

    m_c64.getMmu()->setRoms(kernal, basic, character);
    m_status = true;
}

int Player::fastForward (const uint percent)
{
    if (!m_mixer.setFastForward(percent / 100)) {
        m_errorString = "SIDPLAYER ERROR: Percentage value out of range.";
        return -1;
    }

    return 0;
}

int Player::initialise ()
{
    if (!m_status)
        return -1;

    // Calculate 1 bit below the timebase so we can round the
    // mileage count
    if (((m_mixer.sampleCount() * 2) / m_cfg.frequency) & 1)
        m_mileage++;

    // Fix the mileage counter if just finished another song.
    m_mileage += time ();

    m_playerState  = sid2_stopped;

    m_c64.reset ();

    {
        const uint_least32_t page = (uint_least32_t) m_tuneInfo.loadAddr + m_tuneInfo.c64dataLen - 1;
        if (page > 0xffff)
        {
            m_errorString = "SIDPLAYER ERROR: Size of music data exceeds C64 memory.";
            return -1;
        }
    }

    if (psidDrvReloc (m_tuneInfo, m_info) < 0)
        return -1;

    if (!m_tune->placeSidTuneInC64mem (m_c64.getMmu()->getMem()))
    {   // Rev 1.6 (saw) - Allow loop through errors
        m_errorString = (m_tune->getInfo()).statusString;
        return -1;
    }

    m_c64.getMmu()->cpuWrite(0, 0x2F);
    m_c64.getMmu()->cpuWrite(1, 0x37);

    m_c64.resetCpu();

    m_mixer.reset ();

    return 0;
}

int Player::load (SidTune *tune)
{
    m_tune = tune;
    if (!tune)
    {   // Unload tune
        return 0;
    }

    {   // Must re-configure on fly for stereo support!
        const int ret = config (m_cfg);
        // Failed configuration with new tune, reject it
        if (ret < 0)
        {
            m_tune = NULL;
            return -1;
        }
    }
    return 0;
}

void Player::mute(int voice, bool enable) {
    sidemu *s = m_c64.getSid(0);
    if (s)
        s->voice(voice, enable);
}

uint_least32_t Player::play (short *buffer, uint_least32_t count)
{
    // Make sure a _tune is loaded
    if (!m_tune)
        return 0;

    m_mixer.begin(buffer, count);

    // Start the player loop
    m_playerState = sid2_playing;

    while (m_mixer.notFinished())
        m_c64.getEventScheduler()->clock();

    if (m_playerState == sid2_stopped)
        initialise ();

    return m_mixer.samplesGenerated();
}

void Player::stop (void)
{   // Re-start song
    if (m_tune && (m_playerState != sid2_stopped))
    {
        m_playerState = sid2_stopped;
    }
}

SIDPLAY2_NAMESPACE_STOP
