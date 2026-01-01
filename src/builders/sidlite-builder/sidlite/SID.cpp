/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2025 Leandro Nini <drfiemost@users.sourceforge.net>
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

// Based on cRSID-1.56 by Hermit (Mihaly Horvath)

#include "SID.h"

#include "sidlite_defs.h"

namespace SIDLite
{

constexpr unsigned int CRSID_CLOCK_FRACTIONAL_BITS = 4;
constexpr unsigned int CRSID_RESAMPLER_FRACTIONAL_BITS = 12;

int SID::clock(int cycles)
{
    adsr.clock(cycles);
/*
    if (C64->HighQualitySID) { //oversampled waveform-generation
        HQsampleCount=0;
        NonFiltedSample = FilterInputSample = 0;

        while (C64->OverSampleCycleCnt <= C64->SampleClockRatio) {
            SIDwavOutput = cRSID_emulateHQwaves (&C64->SID[1], OVERSAMPLING_CYCLES);
            C64->SID[1].NonFiltedSample += SIDwavOutput.NonFilted; C64->SID[1].FilterInputSample += SIDwavOutput.FilterInput;
            ++HQsampleCount;
            C64->OverSampleCycleCnt += (OVERSAMPLING_CYCLES<<4);
        }
        C64->OverSampleCycleCnt -= C64->SampleClockRatio;
    }
*/
    return 0;// waves.clock();
}

void SID::write(int addr, unsigned char val)
{
    regs[addr] = val;
}

unsigned char SID::read(int addr) const
{
    return regs[addr];
}

void SID::setChipModel(unsigned short model) {
    ChipModel = model;
}

void SID::setSamplingRate(double clockFrequency, double samplerate) {
    unsigned int cpuclock = static_cast<unsigned int>(clockFrequency);
    SampleClockRatio = (cpuclock << CRSID_CLOCK_FRACTIONAL_BITS) / samplerate; //shifting (multiplication) enhances SampleClockRatio precision
}

SID::SID() :
        adsr(this)
        /*waves(this)*/ {
}

void SID::reset() {
    adsr.reset();

    for (int Channel = 0; Channel < SID_CHANNELS_RANGE; Channel += SID_CHANNEL_SPACING) {
        PhaseAccu[Channel] = 0;
        PrevPhaseAccu[Channel] = 0;
        NoiseLFSR[Channel] = 0x7FFFFF;
        PrevWavGenOut[Channel] = 0;
        PrevWavData[Channel] = 0;
    }
    SyncSourceMSBrise = 0;
    RingSourceMSB = 0;
    PrevLowPass = PrevBandPass = PrevVolume = 0;
}

}
