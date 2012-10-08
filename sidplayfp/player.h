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


#ifndef PLAYER_H
#define PLAYER_H

#include "sid2types.h"
#include "SidTune.h"
#include "SidInfoImpl.h"

#include "sidrandom.h"
#include "mixer.h"

#include "c64/c64.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef PC64_TESTSUITE
#  include <string.h>
#endif


SIDPLAYFP_NAMESPACE_START

class Player
#ifdef PC64_TESTSUITE
: public testEnv
#endif
{
private:
    /**
    * Maximum power on delay
    * Delays <= MAX produce constant results.
    * Delays >  MAX produce random results
    */
    static const uint_least16_t MAX_POWER_ON_DELAY = 0x1FFF;

    static const uint_least16_t DEFAULT_POWER_ON_DELAY = MAX_POWER_ON_DELAY + 1;

private:
    c64     m_c64;

    Mixer   m_mixer;

    // User Configuration Settings
    SidTune      *m_tune;
    SidInfoImpl   m_info;
    sid2_config_t m_cfg;

    const char     *m_errorString;

    bool            m_status;
    volatile bool   m_isPlaying;

    sidrandom       m_rand;

private:
    float64_t clockSpeed     (const sid2_clock_t defaultClock, const bool forced);
    bool      initialise     (void);
    bool      sidCreate(sidbuilder *builder, const sid2_model_t defaultModel,
                       const bool forced, const int channels,
                       const float64_t cpuFreq, const int frequency,
                       const sampling_method_t sampling, const bool fastSampling);

    static sid2_model_t getModel (const SidTuneInfo::model_t sidModel, const sid2_model_t defaultModel, const bool forced);

    uint16_t getChecksum(const uint8_t* rom, const int size);

    uint_least32_t (Player::*output) (char *buffer);

#ifdef PC64_TESTSUITE
    void load (const char *file)
    {
        char name[0x100] = PC64_TESTSUITE;
        strcat (name, file);
        strcat (name, ".prg");

        m_tune->load (name);
        m_tune->selectSong(0);
        initialise();
    }
#endif

public:
    Player ();
    ~Player () {}

    const sid2_config_t &config (void) const { return m_cfg; }
    const SidInfo   &info   (void) const { return m_info; }

    bool           config       (const sid2_config_t &cfg);
    bool           fastForward  (uint percent);
    bool           load         (SidTune *tune);
    float64_t      cpuFreq      (void) const { return m_c64.getMainCpuSpeed(); }
    uint_least32_t play         (short *buffer, uint_least32_t samples);
    bool           isPlaying    (void) const { return m_isPlaying; }
    void           stop         (void);
    uint_least32_t time         (void) { return (uint_least32_t)(m_c64.getEventScheduler()->getTime(EVENT_CLOCK_PHI1) / cpuFreq()); }
    void           debug        (const bool enable, FILE *out)
                                { m_c64.debug (enable, out); }
    void           mute         (const unsigned int sidNum, const unsigned int voice, const bool enable);

    const char    *error        (void) const { return m_errorString; }
    bool           getStatus() const { return m_status; }

    void setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character);

    EventContext *getEventScheduler() { return m_c64.getEventScheduler(); }

    uint_least16_t getCia1TimerA() const { return m_c64.getCia1TimerA(); }
};

SIDPLAYFP_NAMESPACE_STOP

#endif // PLAYER_H
