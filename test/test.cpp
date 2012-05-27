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

#include <sidplayfp/sidplay2.h>
#include <sidplayfp/SidTune.h>
#include <builders/residfp-builder/residfp.h>

int main(int argc, char* argv[])
{
    sidplay2 m_engine;
    ReSIDfpBuilder* rs = new ReSIDfpBuilder("Test");

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
        printf("Error: %s\n", tune->getInfo()->statusString());
        goto error;
    }

    rs->create(2);

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
        const int temp = 48000;
        short buffer[temp];

        for (;;)
        {
            m_engine.play(buffer, temp);
        }
    }

error:
    delete tune;
    delete rs;
}
