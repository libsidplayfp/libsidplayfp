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

#ifndef SIDLITE_SID_H
#define SIDLITE_SID_H

//#include "ADSR.h"
//#include "OutputStage.h"
//#include "Waveforms.h"

namespace SIDLite
{

class SID
{
    friend class ADSR;
    friend class OutputStage;
    friend class Waveforms;

private:
    //ADSR adsr;
    //Waveforms waves;
    //OutputStage output;

    //SID-chip data:
    unsigned short     ChipModel;     //values: 8580 / 6581
    unsigned char      Channel;       //1:left, 2:right, 3:both(middle)
    unsigned short     BaseAddress;   //SID-baseaddress location in C64-memory (IO)
    unsigned char*     BasePtr;       //SID-baseaddress location in host's memory (for writing to SID)
    unsigned char*     BasePtrRD;     //SID-baseaddress location in host's memory (SID OSC3/ENV3 written here for readback)

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
    unsigned char      Volume; //pre-calculated once, used by oversampled/HQ output-emulation many times
    int                Digi; //pre-calculated once, used by oversampled/HQ output-emulation many times
    int                Resonance, Cutoff; //pre-calculated once, used by oversampled/HQ-filter many times
    unsigned char      HighPassBit, BandPassBit, LowPassBit; //pre-calculated once, used by oversampled/HQ-filter many times
    //Output-stage:
    int                NonFiltedSample;
    int                FilterInputSample;
    int                PrevNonFiltedSample;
    int                PrevFilterInputSample;
    signed int         PrevVolume; //lowpass-filtered version of Volume-band register
    int                Output;     //not attenuated (range:0..0xFFFFF depending on SID's main-volume)
    int                Level;      //filtered version, good for VU-meter display

    unsigned char regs[0x40];

public:
    SID();
    ~SID();

    void setChipModel(unsigned short model);
    void setSamplingRate(unsigned int samplerate);
    void reset();

    int clock(int cycles);

    void write(int addr, unsigned char val);
    unsigned char read(int addr) const;
};

}

#endif
