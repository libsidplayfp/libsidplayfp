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

#ifndef SID_H
#define SID_H

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

private:
    unsigned char regs[0x20];

    //SID-chip data:
    unsigned short     ChipModel;     //values: 8580 / 6581
    //ADSR-related:
    unsigned char      ADSRstate[15];
    unsigned short     RateCounter[15];
    unsigned char      EnvelopeCounter[15];
    unsigned char      ExponentCounter[15];
    //Wave-related:
    int                PhaseAccu[15];       //28bit precision instead of 24bit
    int                PrevPhaseAccu[15];   //(integerized ClockRatio fractionals, WebSID has similar solution)
    unsigned char      SyncSourceMSBrise;
    unsigned int       RingSourceMSB;
    unsigned int       NoiseLFSR[15];
    signed char        PrevSounDemonDigiWF[15];
    unsigned int       PrevWavGenOut[15];
    unsigned char      PrevWavData[15];
    //Filter-related:
    int                PrevLowPass;
    int                PrevBandPass;
    //Output-stage:
    signed int         PrevVolume; //lowpass-filtered version of Volume-band register

    //C64-machine related:
    unsigned int      CPUfrequency;
    unsigned short    SampleClockRatio; //ratio of CPU-clock and samplerate
    unsigned short    Attenuation;
    bool              RealSIDmode = true;
    int               Digi;
    //PSID-playback related:
    short             SampleCycleCnt;
    unsigned short    SampleRate;

    unsigned char oscReg;
    unsigned char envReg;

private:
    void emulateADSRs(char cycles);
    int emulateWaves();
    inline int emulateSIDoutputStage(int FilterInput, int NonFiltered);

    int generateSound(short* buf, unsigned int cycles);
    inline signed short generateSample(unsigned int &cycles);
    int emulateC64(unsigned int &cycles);
};

}

#endif // SIDLITE_H
