/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012 Leando Nini <drfiemost@users.sourceforge.net>
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
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <fstream>
#include <memory>
#include <vector>
#include <iostream>

#include "sidplayfp/sidplayfp.h"
#include "sidplayfp/SidTune.h"
#include <sidplayfp/SidInfo.h>
#include "sidplayfp/builders/residfp.h"
#include "sidplayfp/sidemu.h"


/*
* Adjust these paths to point to existing ROM dumps
*/
#define KERNAL_PATH "/usr/local/lib/vice/C64/kernal"
#define BASIC_PATH "/usr/local/lib/vice/C64/basic"
#define CHARGEN_PATH "/usr/local/lib/vice/C64/chargen"

#define SAMPLERATE 48000

void loadRom(const char* path, char* buffer)
{
    std::ifstream is(path, std::ios::binary);
    is.read(buffer, 8192);
    is.close();
}

/*
* Sample application that shows how to use libsidplayfp
* to play a SID tune from a file
* It uses OSS for audio output
*/
int main(int argc, char* argv[])
{
    sidplayfp m_engine;

    { // Load ROM files
    char kernal[8192];
    char basic[8192];
    char chargen[4096];

    loadRom(KERNAL_PATH, kernal);
    loadRom(BASIC_PATH, basic);
    loadRom(CHARGEN_PATH, chargen);

    m_engine.setRoms((const uint8_t*)kernal, (const uint8_t*)basic, (const uint8_t*)chargen);
    }

    // Set up a SID builder
    std::auto_ptr<ReSIDfpBuilder> rs(new ReSIDfpBuilder("Demo"));
    // Get the number of SIDs supported by the engine
    unsigned int maxsids=(m_engine.info ()).maxsids();
    // Create SID emulators
    rs->create(maxsids);

    if (!rs->getStatus())
    {
        std::cerr << rs->error() << std::endl;
        return -1;
    }

    // Load tune from file
    std::auto_ptr<SidTune> tune(new SidTune(argv[1]));

    if (!tune->getStatus())
    {
        std::cerr << tune->statusString() << std::endl;
        return -1;
    }

    // Select default song
    tune->selectSong(0);

    // Configure the engine
    SidConfig cfg;
    cfg.clockForced = false;
    cfg.frequency = SAMPLERATE;
    cfg.samplingMethod = SidConfig::INTERPOLATE;
    cfg.fastSampling = false;
    cfg.playback = SidConfig::MONO;
    cfg.sidEmulation = rs.get();
    cfg.sidDefault = SidConfig::MOS6581;
    m_engine.config(cfg);

    // Load tune into engine
    if (!m_engine.load(tune.get()))
    {
        std::cerr <<  m_engine.error() << std::endl;
        return -1;
    }

    // Setup audio device
    int handle=::open("/dev/dsp", O_WRONLY, 0);
    int format=AFMT_S16_LE;
    ioctl(handle, SNDCTL_DSP_SETFMT, &format);
    int chn=1;
    ioctl(handle, SNDCTL_DSP_CHANNELS, &chn);
    int sampleRate=SAMPLERATE;
    ioctl(handle, SNDCTL_DSP_SPEED, &sampleRate);
    int bufferSize;
    ioctl(handle, SNDCTL_DSP_GETBLKSIZE, &bufferSize);

    // Play
    std::vector<short> buffer(bufferSize);
    for (int i=0; i<1000; i++)
    {
        m_engine.play(&buffer.front(), bufferSize/sizeof(short));
        ::write(handle, &buffer.front(), bufferSize);
    }

    ::close(handle);
}
