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


#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "player.h"

#include "psiddrv.h"

#ifndef PACKAGE_NAME
#   define PACKAGE_NAME PACKAGE
#endif

#ifndef PACKAGE_VERSION
#   define PACKAGE_VERSION VERSION
#endif


SIDPLAYFP_NAMESPACE_START


const char  TXT_NA[]             = "NA";

// Error Strings
const char  ERR_MISSING_ROM[]    = "SIDPLAYER ERROR: Roms have not been loaded.";
const char  ERR_UNKNOWN_ROM[]    = "SIDPLAYER ERROR: Unknown Rom.";

const char  *Player::credit[];


Player::Player (void)
// Set default settings for system
:m_mixer (m_c64.getEventScheduler()),
 m_tune (0),
 m_errorString(ERR_MISSING_ROM),
 m_status(false),
 m_isPlaying(false),
 m_rand((int)::time(0))
{
#ifdef PC64_TESTSUITE
    m_c64.setTestEnv(this);
#endif

    // Setup exported info
    m_info.name            = PACKAGE_NAME;
    m_info.version         = PACKAGE_VERSION;
    m_info.credits         = credit;
    m_info.maxsids         = SidBank::MAX_SIDS;
    m_info.channels        = 1;
    m_info.driverAddr      = 0;
    m_info.driverLength    = 0;
    m_info.speedString     = TXT_NA;

    // Configure default settings
    m_cfg.clockDefault    = SID2_CLOCK_PAL;
    m_cfg.clockForced     = false;
    m_cfg.forceDualSids   = false;
    m_cfg.frequency       = SID2_DEFAULT_SAMPLING_FREQ;
    m_cfg.playback        = sid2_mono;
    m_cfg.sidDefault      = SID2_MOS6581;
    m_cfg.forceModel      = false;
    m_cfg.sidEmulation    = 0;
    m_cfg.leftVolume      = Mixer::VOLUME_MAX;
    m_cfg.rightVolume     = Mixer::VOLUME_MAX;
    m_cfg.powerOnDelay    = DEFAULT_POWER_ON_DELAY;
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
    m_errorString = TXT_NA;
    m_status = true;
}

bool Player::fastForward (const uint percent)
{
    if (!m_mixer.setFastForward(percent / 100)) {
        m_errorString = "SIDPLAYER ERROR: Percentage value out of range.";
        return false;
    }

    return true;
}

bool Player::initialise ()
{
    if (!m_status)
        return false;

    m_isPlaying  = false;

    m_c64.reset ();

    const SidTuneInfo* tuneInfo = m_tune->getInfo();

    {
        const uint_least32_t page = (uint_least32_t) tuneInfo->loadAddr() + tuneInfo->c64dataLen() - 1;
        if (page > 0xffff)
        {
            m_errorString = "SIDPLAYER ERROR: Size of music data exceeds C64 memory.";
            return false;
        }
    }

    uint_least16_t powerOnDelay = m_cfg.powerOnDelay;
    // Delays above MAX result in random delays
    if (powerOnDelay > MAX_POWER_ON_DELAY)
    {   // Limit the delay to something sensible.
        powerOnDelay = (uint_least16_t) (m_rand.next() >> 3) & MAX_POWER_ON_DELAY;
    }

    psiddrv driver(m_tune->getInfo());
    driver.powerOnDelay(powerOnDelay);
    if (!driver.drvReloc (m_c64.getMmu()))
    {
        m_errorString = driver.errorString();
        return false;
    }

    m_info.driverAddr = driver.driverAddr();
    m_info.driverLength = driver.driverLength();
    m_info.powerOnDelay = powerOnDelay;

    if (!m_tune->placeSidTuneInC64mem (m_c64.getMmu()))
    {
        m_errorString = m_tune->statusString();
        return false;
    }

    m_c64.getMmu()->cpuWrite(0, 0x2F);
    m_c64.getMmu()->cpuWrite(1, 0x37);

    m_c64.resetCpu();

    m_mixer.reset ();

    return true;
}

bool Player::load (SidTune *tune)
{
    m_tune = tune;
    if (!tune)
    {   // Unload tune
        return true;
    }

    {   // Must re-configure on fly for stereo support!
        const int ret = config (m_cfg);
        // Failed configuration with new tune, reject it
        if (ret < 0)
        {
            m_tune = 0;
            return false;
        }
    }
    return true;
}

void Player::mute(const unsigned int sidNum, const unsigned int voice, const bool enable) {
    sidemu *s = m_c64.getSid(sidNum);
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
    m_isPlaying = true;

    while (m_isPlaying && m_mixer.notFinished())
        m_c64.getEventScheduler()->clock();

    if (!m_isPlaying)
        initialise ();

    return m_mixer.samplesGenerated();
}

void Player::stop (void)
{   // Re-start song
    if (m_tune && m_isPlaying)
    {
        m_isPlaying = false;
    }
}

SIDPLAYFP_NAMESPACE_STOP
