/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "sidplayfp/sidplay2.h"
#include "sidplayfp/SidTune.h"
#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/sidemu.h"

class NullSID: public sidemu
{
public:
    NullSID () : sidemu(0) { m_buffer = 0; }

    void    reset (uint8_t) {}
    uint8_t read  (uint_least8_t) { return 0; }
    void    write (uint_least8_t, uint8_t) {}
    void    clock() {}
    const   char *credits (void) { return ""; }
    void    voice (const unsigned int num, const bool mute) {}
    const   char *error   (void) const { return ""; }
};

class FakeBuilder: public sidbuilder
{
private:
    NullSID sidobj;

public:
    FakeBuilder  (const char * const name) : sidbuilder(name) {}
    ~FakeBuilder (void) {}

    sidemu     *lock    (EventContext *env, const sid2_model_t model) { return &sidobj; }
    void        unlock  (sidemu *device) {}
    const char *error   (void) const { return ""; }
    const char *credits (void) { return ""; }
    void        filter (const bool enable) {}
};

int main(int argc, char* argv[])
{
    sidplay2 m_engine;
    FakeBuilder* rs = new FakeBuilder("Test");

    char name[0x100] = PC64_TESTSUITE;

    if (argc > 1)
    {
        strcat (name, argv[1]);
        strcat (name, ".prg");
    }
    else
    {
        strcat (name, " start.prg");
    }

    SidTune* tune = new SidTune(name);

    if (!tune->getStatus())
    {
        printf("Error: %s\n", tune->statusString());
        goto error;
    }

    sid2_config_t cfg;
    cfg = m_engine.config();
    cfg.clockForced = false;
    cfg.frequency = 48000;
    cfg.samplingMethod = SID2_INTERPOLATE;
    cfg.fastSampling = false;
    cfg.playback = sid2_stereo;
    cfg.sidEmulation = rs;
    cfg.sidDefault = SID2_MOS6581;
    m_engine.config(cfg);

    tune->selectSong(0);

    m_engine.load(tune);

    {
        for (;;)
        {
            m_engine.play(0, 48000);
        }
    }

error:
    delete tune;
    delete rs;
}
