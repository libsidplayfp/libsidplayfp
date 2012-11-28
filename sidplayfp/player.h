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

#include "SidConfig.h"
#include "SidTune.h"
#include "SidTuneInfo.h"
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
    c64     m_c64;

    Mixer   m_mixer;

    // User Configuration Settings
    SidTune      *m_tune;
    SidInfoImpl   m_info;
    SidConfig     m_cfg;

    const char   *m_errorString;

    bool          m_status;
    volatile bool m_isPlaying;

    sidrandom     m_rand;

private:
    double    clockSpeed     (SidConfig::clock_t defaultClock, bool forced);
    bool      initialise     (void);
    bool      sidCreate(sidbuilder *builder, SidConfig::model_t defaultModel,
                       bool forced, int channels,
                       double cpuFreq, int frequency,
                       SidConfig::sampling_method_t sampling, bool fastSampling);

    static SidConfig::model_t getModel (SidTuneInfo::model_t sidModel, SidConfig::model_t defaultModel, bool forced);

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

    const SidConfig &config (void) const { return m_cfg; }
    const SidInfo   &info   (void) const { return m_info; }

    bool           config       (const SidConfig &cfg);
    bool           fastForward  (unsigned int percent);
    bool           load         (SidTune *tune);
    double         cpuFreq      (void) const { return m_c64.getMainCpuSpeed(); }
    uint_least32_t play         (short *buffer, uint_least32_t samples);
    bool           isPlaying    (void) const { return m_isPlaying; }
    void           stop         (void);
    uint_least32_t time         (void) const { return (uint_least32_t)(m_c64.getEventScheduler().getTime(EVENT_CLOCK_PHI1) / cpuFreq()); }
    void           debug        (const bool enable, FILE *out) { m_c64.debug (enable, out); }
    void           mute         (unsigned int sidNum, unsigned int voice, bool enable);

    const char    *error        (void) const { return m_errorString; }
    bool           getStatus    () const { return m_status; }

    void setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character);

    EventContext *getEventScheduler() { return m_c64.getEventScheduler(); }

    uint_least16_t getCia1TimerA() const { return m_c64.getCia1TimerA(); }
};

SIDPLAYFP_NAMESPACE_STOP

#endif // PLAYER_H
