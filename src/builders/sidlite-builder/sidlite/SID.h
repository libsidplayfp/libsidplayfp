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

#ifndef SIDLITE_SID_H
#define SIDLITE_SID_H

#include "ADSR.h"

#include <array>

namespace SIDLite
{

class SID
{
public:
    SID();
    void reset();
    void write(int addr, int value);
    int read(int addr);
    int clock(unsigned int cycles, short* buf);
    void setChipModel(int model);

    void setSamplingParameters(unsigned int clockFrequency, unsigned short samplingFrequency);

    int getLevel() const { return Level; }

private:
    unsigned char regs[0x20] = {0};

    //SID-chip data:
    unsigned short     ChipModel;     //values: 8580 / 6581
    ADSR               adsr;
    //Wave-related:
    int                PhaseAccu[3];       //28bit precision instead of 24bit
    int                PrevPhaseAccu[3];   //(integerized ClockRatio fractionals, WebSID has similar solution)
    unsigned int       NoiseLFSR[3];
    unsigned int       PrevWavGenOut[3];
    unsigned char      PrevWavData[3];
    signed char        PrevSounDemonDigiWF[3];
    unsigned int       RingSourceMSB;
    unsigned char      SyncSourceMSBrise;
    //Filter-related:
    int                PrevLowPass;
    int                PrevBandPass;
    unsigned short     *CutoffMul8580;
    unsigned short     *CutoffMul6581;
    //Output-stage:
    signed int         PrevVolume; //lowpass-filtered version of Volume-band register

    //C64-machine related:
    unsigned int      CPUfrequency = 0;
    unsigned short    SampleClockRatio = 0; //ratio of CPU-clock and samplerate
    unsigned short    Attenuation;
    bool              RealSIDmode = true;
    int               Digi;
    //PSID-playback related:
    short             SampleCycleCnt;
    unsigned short    SampleRate = 0;

    unsigned char oscReg;
    unsigned char envReg;

    int                Level;      //filtered version, good for VU-meter display
    unsigned char VUmeterUpdateCounter;

private:
    int emulateWaves();
    inline int emulateSIDoutputStage(int FilterInput, int NonFiltered);

    int generateSound(short* buf, unsigned int cycles);
    inline signed short generateSample(unsigned int &cycles);
    int emulateC64(unsigned int &cycles);

    void rebuildCutoffTables(unsigned short samplerate);
};

}

#endif // SIDLITE_SID_H
