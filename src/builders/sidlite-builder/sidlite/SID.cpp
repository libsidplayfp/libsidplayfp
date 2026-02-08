/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2025-2026 Leandro Nini
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Based on cRSID lightweight RealSID by Hermit (Mihaly Horvath)

#include "SID.h"

#include "sl_defs.h"
#include "sl_constants.h"
#include "filt_tables.h"
#include "cw_tables.h"

#include <algorithm>
#include <iterator>

namespace SIDLite
{

int SID::clock(unsigned int cycles, short* buf)
{
    int i = 0;
    while (cycles > 0)
    {
        buf[i] = generateSample(cycles);
        i++;
    }
    return i;
}

inline signed short SID::generateSample(unsigned int &cycles)
{
    // call this from custom buffer-filler
    int Output = emulateC64(cycles);
    // saturation logic on overflow
    if (Output > 32767)
        Output = 32767;
    else if (Output < -32768)
        Output = -32768;
    return static_cast<signed short>(Output);
}


inline int SID::emulateC64(unsigned int &cycles)
{
    // Cycle-based part of emulations:

    while ((SampleCycleCnt <= s.SampleClockRatio) && cycles)
    {
        unsigned char InstructionCycles = std::min(7u, cycles);
        SampleCycleCnt += InstructionCycles << 4;
        cycles -= InstructionCycles;

        adsr.clock(InstructionCycles);
    }

    SampleCycleCnt -= s.SampleClockRatio;

    // Samplerate-based part of emulations:

    wg_output_t output = wavgen.clock(&adsr);
    return filter.clock(output.first, output.second);
}

void SID::write(int addr, int value)
{
    regs[addr] = value;
}

int SID::read(int addr)
{
    if (addr == 0x1B)
        return wavgen.getOsc3();
    if (addr == 0x1C)
        return wavgen.getEnv3();
    return 0;
}

SID::SID() :
    adsr(regs),
    filter(&s, regs),
    wavgen(&s, regs)
{
    setChipModel(8580);
    reset();
}

void SID::reset()
{
    SampleCycleCnt = 0;

    std::fill(std::begin(regs), std::end(regs), 0);
}

void SID::setSamplingParameters(unsigned int clockFrequency, unsigned short samplingFrequency)
{
    filter.rebuildCutoffTables(samplingFrequency);

    // shifting (multiplication) enhances SampleClockRatio precision
    s.SampleClockRatio = (clockFrequency << 4) / samplingFrequency;
}

void SID::setChipModel(int model)
{
    s.ChipModel = model;
}

void SID::setRealSIDmode(bool mode)
{
    s.RealSIDmode = mode;
}

}
