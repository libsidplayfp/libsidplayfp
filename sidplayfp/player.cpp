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


#include "player.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "psiddrv.h"
#include "romCheck.h"

SIDPLAYFP_NAMESPACE_START


const char  TXT_NA[]             = "NA";


Player::Player (void)
// Set default settings for system
:m_mixer (m_c64.getEventScheduler()),
 m_tune (0),
 m_errorString(TXT_NA),
 m_status(true),
 m_isPlaying(false),
 m_rand((int)::time(0))
{
#ifdef PC64_TESTSUITE
    m_c64.setTestEnv(this);
#endif
    config (m_cfg);

    // Get component credits
    m_info.m_credits.push_back(m_c64.cpuCredits());
    m_info.m_credits.push_back(m_c64.ciaCredits());
    m_info.m_credits.push_back(m_c64.vicCredits());
}

void Player::setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
{
    if (kernal)
    {
        kernalCheck k(kernal);
        m_info.m_kernalDesc = k.info();
    }

    if (basic)
    {
        basicCheck b(basic);
        m_info.m_basicDesc = b.info();
    }

    if (character)
    {
        chargenCheck c(character);
        m_info.m_chargenDesc = c.info();
    }

    m_c64.setRoms(kernal, basic, character);
}

bool Player::fastForward (unsigned int percent)
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
        const uint_least32_t size = (uint_least32_t) tuneInfo->loadAddr() + tuneInfo->c64dataLen() - 1;
        if (size > 0xffff)
        {
            m_errorString = "SIDPLAYER ERROR: Size of music data exceeds C64 memory.";
            return false;
        }
    }

    uint_least16_t powerOnDelay = m_cfg.powerOnDelay;
    // Delays above MAX result in random delays
    if (powerOnDelay > SidConfig::MAX_POWER_ON_DELAY)
    {   // Limit the delay to something sensible.
        powerOnDelay = (uint_least16_t) ((m_rand.next() >> 3) & SidConfig::MAX_POWER_ON_DELAY);
    }

    psiddrv driver(m_tune->getInfo());
    driver.powerOnDelay(powerOnDelay);
    if (!driver.drvReloc (m_c64.getMemInterface()))
    {
        m_errorString = driver.errorString();
        return false;
    }

    m_info.m_driverAddr = driver.driverAddr();
    m_info.m_driverLength = driver.driverLength();
    m_info.m_powerOnDelay = powerOnDelay;

    if (!m_tune->placeSidTuneInC64mem (m_c64.getMemInterface()))
    {
        m_errorString = m_tune->statusString();
        return false;
    }

    driver.install (m_c64.getMemInterface());

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
        if (!config (m_cfg))
        {
            // Failed configuration with new tune, reject it
            m_tune = 0;
            return false;
        }
    }
    return true;
}

void Player::mute(unsigned int sidNum, unsigned int voice, bool enable) {
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
