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
#include "tables.h"

#include <algorithm>

namespace SIDLite
{

constexpr int SID_CHANNEL_SPACING = 7;
constexpr int SID_CHANNEL_COUNT = 3;
constexpr int CHANNEL2_INDEX = 2*SID_CHANNEL_SPACING;
constexpr int SID_CHANNELS_RANGE = SID_CHANNEL_SPACING * SID_CHANNEL_COUNT;

SID::SID()
{
    setChipModel(8580);
    reset();
}

void SID::reset()
{
    for (int Channel = 0; Channel < 21; Channel+=7)
    {
        ADSRstate[Channel] = 0;
        RateCounter[Channel] = 0;
        EnvelopeCounter[Channel] = 0;
        ExponentCounter[Channel] = 0;
        PhaseAccu[Channel] = 0;
        PrevPhaseAccu[Channel] = 0;
        NoiseLFSR[Channel] = 0x7FFFFF;
        PrevWavGenOut[Channel] = 0;
        PrevWavData[Channel] = 0;
        PrevSounDemonDigiWF[Channel] = 0x00;
    }
    SyncSourceMSBrise = 0;
    RingSourceMSB = 0;
    PrevLowPass = PrevBandPass = PrevVolume = 0;
    SampleCycleCnt = 0;

    oscReg = envReg = 0;
}

void SID::write(int addr, int value)
{
    regs[addr] = value;
}

int SID::read(int addr)
{
    if (addr == 0x1B)
        return oscReg;
    if (addr == 0x1C)
        return envReg;
    return 0;
}

int SID::clock(unsigned int cycles, short* buf)
{
    return generateSound(buf, cycles);
}

void SID::setChipModel(int model)
{
    ChipModel = model;
}

void SID::setSamplingParameters(unsigned int clockFrequency, unsigned short samplingFrequency)
{
    CPUfrequency = clockFrequency;
    SampleRate = samplingFrequency;
    SampleClockRatio = ( CPUfrequency << 4 ) / SampleRate; //shifting (multiplication) enhances SampleClockRatio precision
    Attenuation = 26;
}

static const unsigned char ADSR_DAC_6581[] = {
    0x00,0x02,0x03,0x04,0x05,0x06,0x07,0x09,0x09,0x0B,0x0B,0x0D,0x0D,0x0F,0x10,0x12,
    0x11,0x13,0x14,0x15,0x16,0x17,0x18,0x1A,0x1A,0x1C,0x1C,0x1E,0x1E,0x20,0x21,0x23, //used at output of ADSR envelope generator
    0x21,0x23,0x24,0x25,0x26,0x27,0x28,0x2A,0x2A,0x2C,0x2C,0x2E,0x2E,0x30,0x31,0x33, //(not used for wave-generator because of 8bit-only resolution)
    0x32,0x34,0x35,0x36,0x37,0x38,0x39,0x3B,0x3B,0x3D,0x3D,0x3F,0x3F,0x41,0x42,0x44,
    0x40,0x42,0x42,0x44,0x44,0x46,0x47,0x49,0x49,0x4A,0x4B,0x4D,0x4D,0x4F,0x50,0x52,
    0x51,0x53,0x53,0x55,0x55,0x57,0x58,0x5A,0x5A,0x5B,0x5C,0x5E,0x5E,0x60,0x61,0x63,
    0x61,0x62,0x63,0x65,0x65,0x67,0x68,0x6A,0x69,0x6B,0x6C,0x6E,0x6E,0x70,0x71,0x73,
    0x72,0x73,0x74,0x76,0x76,0x78,0x79,0x7B,0x7A,0x7C,0x7D,0x7F,0x7F,0x81,0x82,0x84,
    0x7B,0x7D,0x7E,0x80,0x80,0x82,0x83,0x85,0x84,0x86,0x87,0x89,0x89,0x8B,0x8C,0x8D,
    0x8C,0x8E,0x8F,0x91,0x91,0x93,0x94,0x96,0x95,0x97,0x98,0x9A,0x9A,0x9C,0x9D,0x9E,
    0x9C,0x9E,0x9F,0xA1,0xA1,0xA3,0xA4,0xA5,0xA5,0xA7,0xA8,0xAA,0xAA,0xAC,0xAC,0xAE,
    0xAD,0xAF,0xB0,0xB2,0xB2,0xB4,0xB5,0xB6,0xB6,0xB8,0xB9,0xBB,0xBB,0xBD,0xBD,0xBF,
    0xBB,0xBD,0xBE,0xC0,0xC0,0xC2,0xC2,0xC4,0xC4,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCD,
    0xCC,0xCE,0xCF,0xD1,0xD1,0xD3,0xD3,0xD5,0xD5,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDE,
    0xDC,0xDE,0xDF,0xE1,0xE1,0xE3,0xE3,0xE5,0xE5,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xEE,
    0xED,0xEF,0xF0,0xF2,0xF2,0xF4,0xF4,0xF6,0xF6,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFF
};

void SID::emulateADSRs(char cycles)
{
    enum ADSRstateBits { GATE_BITVAL=0x01, ATTACK_BITVAL=0x80, DECAYSUSTAIN_BITVAL=0x40, HOLDZEROn_BITVAL=0x10 };

    static const short ADSRprescalePeriods[16] = {
        9, 32, 63, 95, 149, 220, 267, 313, 392, 977, 1954, 3126, 3907, 11720, 19532, 31251
    };

    static const unsigned char ADSRexponentPeriods[256] = {
        1, 30, 30, 30, 30, 30, 30, 16, 16, 16, 16, 16, 16, 16, 16,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, 4, 4, 4, 4, //pos0:1  pos6:30  pos14:16  pos26:8
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, //pos54:4 //pos93:2
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };

    for (int Channel=0; Channel<SID_CHANNELS_RANGE; Channel+=SID_CHANNEL_SPACING)
    {
        unsigned char *ChannelPtr = &regs[Channel];
        unsigned char AD = ChannelPtr[5];
        unsigned char SR = ChannelPtr[6];
        unsigned char *ADSRstatePtr = &(ADSRstate[Channel]);
        unsigned short *RateCounterPtr = &(RateCounter[Channel]);
        unsigned char *EnvelopeCounterPtr = &(EnvelopeCounter[Channel]);
        unsigned char *ExponentCounterPtr = &(ExponentCounter[Channel]);

        unsigned char PrevGate = (*ADSRstatePtr & GATE_BITVAL);
        if (UNLIKELY(PrevGate != (ChannelPtr[4] & GATE_BITVAL)))
        { //gatebit-change?
            if (PrevGate)
                *ADSRstatePtr &= ~(GATE_BITVAL | ATTACK_BITVAL | DECAYSUSTAIN_BITVAL); //falling edge
            else
                *ADSRstatePtr = (GATE_BITVAL | ATTACK_BITVAL | DECAYSUSTAIN_BITVAL | HOLDZEROn_BITVAL); //rising edge
        }

        unsigned short PrescalePeriod;
        if (*ADSRstatePtr & ATTACK_BITVAL)
            PrescalePeriod = ADSRprescalePeriods[ AD >> 4 ];
        else if (*ADSRstatePtr & DECAYSUSTAIN_BITVAL)
            PrescalePeriod = ADSRprescalePeriods[ AD & 0x0F ];
        else
            PrescalePeriod = ADSRprescalePeriods[ SR & 0x0F ];

        *RateCounterPtr += cycles;
        if (UNLIKELY(*RateCounterPtr >= 0x8000))
            *RateCounterPtr -= 0x8000; //*RateCounterPtr &= 0x7FFF; //can wrap around (ADSR delay-bug: short 1st frame)

        if (UNLIKELY(PrescalePeriod <= *RateCounterPtr && *RateCounterPtr < PrescalePeriod+cycles))
        { //ratecounter shot (matches rateperiod) (in genuine SID ratecounter is LFSR)
            *RateCounterPtr -= PrescalePeriod; //reset rate-counter on period-match
            if ((*ADSRstatePtr & ATTACK_BITVAL) || ++(*ExponentCounterPtr) == ADSRexponentPeriods[*EnvelopeCounterPtr])
            {
                *ExponentCounterPtr = 0;
                if (*ADSRstatePtr & HOLDZEROn_BITVAL)
                {
                    if (*ADSRstatePtr & ATTACK_BITVAL)
                    {
                        ++(*EnvelopeCounterPtr);
                        if (*EnvelopeCounterPtr==0xFF)
                            *ADSRstatePtr &= ~ATTACK_BITVAL;
                    }
                    else if (!(*ADSRstatePtr & DECAYSUSTAIN_BITVAL) || *EnvelopeCounterPtr != (SR&0xF0)+(SR>>4))
                    {
                        --(*EnvelopeCounterPtr); //resid adds 1 cycle delay, we omit that mechanism here
                        if (*EnvelopeCounterPtr==0)
                            *ADSRstatePtr &= ~HOLDZEROn_BITVAL;
                    }
                }
            }
        }
    }
}

constexpr int CRSID_CLOCK_FRACTIONAL_BITS = 4;
constexpr int CRSID_WAVE_RESOLUTION = 16;
constexpr int OSC3_WAVE_RESOLUTION = 8;
constexpr int CRSID_WAVGEN_PRESHIFT = 3;
constexpr int CRSID_WAVGEN_PREDIV = 1 << CRSID_WAVGEN_PRESHIFT;

constexpr int SID_PHASEACCU_RESOLUTION = 24;
constexpr int COMBINEDWF_SAMPLE_RESOLUTION = 12; //bits
constexpr int PHASEACCU_SID__RANGE = 1 << SID_PHASEACCU_RESOLUTION;
constexpr int PHASEACCU_RANGE = (PHASEACCU_SID__RANGE << CRSID_CLOCK_FRACTIONAL_BITS);
constexpr int PHASEACCU_MAX = PHASEACCU_RANGE - 1;
constexpr int PHASEACCU_ANDMASK = PHASEACCU_MAX;
constexpr int PHASEACCU_MSB_BITVAL = (PHASEACCU_SID__RANGE >> 1) << CRSID_CLOCK_FRACTIONAL_BITS;
constexpr int SID_NOISE_CLOCK_BITVAL = 0x100000;
constexpr int NOISE_CLOCK = (SID_NOISE_CLOCK_BITVAL << CRSID_CLOCK_FRACTIONAL_BITS);
constexpr int CRSID_WAVE_RANGE = 1 << CRSID_WAVE_RESOLUTION;
constexpr int CRSID_WAVE_MID = CRSID_WAVE_RANGE / 2;
constexpr int CRSID_WAVE_MIN = 0x0000;
constexpr int CRSID_WAVE_MAX = CRSID_WAVE_RANGE - 1;
constexpr int CRSID_WAVE_MASK = CRSID_WAVE_MAX;
constexpr int CRSID_WAVE_SHIFTS = (SID_PHASEACCU_RESOLUTION - CRSID_WAVE_RESOLUTION) + CRSID_CLOCK_FRACTIONAL_BITS;
constexpr int STEEPNESS_FRACTION_SHIFTS = 16;
constexpr int STEEPNESS_STEPLIMIT = 256 << CRSID_CLOCK_FRACTIONAL_BITS;
constexpr int COMBINEDWF_SAMPLE_SHIFTS =
    (SID_PHASEACCU_RESOLUTION - COMBINEDWF_SAMPLE_RESOLUTION) + CRSID_CLOCK_FRACTIONAL_BITS; // 16
constexpr int WAVE_OSC3_SHIFTS = CRSID_WAVE_RESOLUTION - OSC3_WAVE_RESOLUTION; //8
constexpr int SOUNDEMON_DIGI_SEEK_WAVEFORM = 0x01;
constexpr int SOUNDEMON_DIGI_RESOLUTION = 8;
constexpr int SOUNDEMON_DIGI_SHIFTS = (CRSID_WAVE_RESOLUTION - SOUNDEMON_DIGI_RESOLUTION);
constexpr int SOUNDEMON_CARRIER_ELIMINATION_SAMPLECOUNT = 2;
constexpr int SID_ENVELOPE_RESOLUTION = 8;
constexpr int SID_ENVELOPE_MAGNITUDE = 1 << SID_ENVELOPE_RESOLUTION; //256
constexpr int ENVELOPE_MAGNITUDE_DIV = SID_ENVELOPE_MAGNITUDE * CRSID_WAVGEN_PREDIV; //256 * 1..16 = 256..4096

static inline unsigned short getPW(unsigned char* channelptr)
{
    return (((channelptr[3]&0xF) << 8) | channelptr[2]) << 4; //PW=0000..FFF0 from SID-register (000..FFF)
}

static inline unsigned short getCombinedPW(unsigned char* channelptr)
{
    return ((channelptr[3]&0xF) << 8) | channelptr[2]; //PW=000..FFF (range for combined-waveform lookup) from SID-register (000..FFF)
}

int SID::emulateWaves()
{
    enum SIDspecs { CHANNELS=3+1, VOLUME_MAX=0xF, D418_DIGI_VOLUME=2 }; //digi-channel is counted too
    enum WaveFormBits { NOISE_BITVAL=0x80, PULSE_BITVAL=0x40, SAW_BITVAL=0x20, TRI_BITVAL=0x10,
               PULSAWTRI_VAL=0x70, PULSAW_VAL=0x60, PULTRI_VAL=0x50, SAWTRI_VAL=0x30 };
    enum ControlBits { TEST_BITVAL=0x08, RING_BITVAL=0x04, SYNC_BITVAL=0x02, GATE_BITVAL=0x01 };
    enum FilterBits { OFF3_BITVAL=0x80, HIGHPASS_BITVAL=0x40, BANDPASS_BITVAL=0x20, LOWPASS_BITVAL=0x10 };

    const unsigned char FilterSwitchVal[] = {1,1,1,1,1,1,1,2,2,2,2,2,2,2,4};

    //Waveform-generator //(phase accumulator and waveform-selector)

    unsigned int WavGenOut = 0;
    int FilterInput = 0;
    int NonFilted = 0;
    unsigned char FilterSwitchReso = regs[0x17];
    unsigned char VolumeBand = regs[0x18];

    for (int Channel=0; Channel<SID_CHANNELS_RANGE; Channel+=SID_CHANNEL_SPACING)
    {
        unsigned char *ChannelPtr = &(regs[Channel]);

        auto combinedWF = [&](const unsigned char* WFarray, unsigned short oscval) {
            unsigned char Pitch;
            unsigned short Filt;
            if (ChipModel==6581 && WFarray!=PulseTriangle)
                oscval &= 0x7FFF;
            Pitch = ChannelPtr[1] ? ChannelPtr[1] : 1; //avoid division by zero
            Filt = 0x7777 + (0x8888/Pitch);
            PrevWavData[Channel] = ( WFarray[oscval>>4]*Filt + PrevWavData[Channel]*(0xFFFF-Filt) ) >> 16;
            return PrevWavData[Channel] << 8;
        };

        unsigned char WF = ChannelPtr[4];
        unsigned char TestBit = (UNLIKELY((WF & TEST_BITVAL) != 0));
        int *PhaseAccuPtr = &(PhaseAccu[Channel]);

        unsigned int PhaseAccuStep = ((ChannelPtr[1]<<8) | ChannelPtr[0]) * SampleClockRatio;
        if (UNLIKELY(TestBit || ((WF & SYNC_BITVAL) && SyncSourceMSBrise)))
            *PhaseAccuPtr = 0;
        else
        { //stepping phase-accumulator (oscillator)
            *PhaseAccuPtr += PhaseAccuStep;
            if (UNLIKELY(*PhaseAccuPtr >= PHASEACCU_RANGE))
                *PhaseAccuPtr -= PHASEACCU_RANGE; // 0x10000000
        }
        *PhaseAccuPtr &= PHASEACCU_ANDMASK; // 0xFFFFFFF
        unsigned int MSB = *PhaseAccuPtr & PHASEACCU_MSB_BITVAL; // 0x8000000
        SyncSourceMSBrise = (UNLIKELY(MSB > (PrevPhaseAccu[Channel] & PHASEACCU_MSB_BITVAL))) ? 1 : 0;

        switch (WF & 0xF0)
        { //switch-case encourages computed-goto compiler-optimization
            case NOISE_BITVAL:
            { //noise waveform
                int Tmp = NoiseLFSR[Channel]; //clock LFSR all time if clockrate exceeds observable at given samplerate (last term):
                if (UNLIKELY(((*PhaseAccuPtr & NOISE_CLOCK) != (PrevPhaseAccu[Channel] & NOISE_CLOCK)) || PhaseAccuStep >= NOISE_CLOCK))
                {
                    int Feedback = ((Tmp & 0x400000) ^ ((Tmp & 0x20000) << 5)) != 0;
                    Tmp = ((Tmp << 1) | Feedback|TestBit) & 0x7FFFFF; //TEST-bit turns all bits in noise LFSR to 1 (on real SID slowly, in approx. 8000 microseconds ~ 300 samples)
                    NoiseLFSR[Channel] = Tmp;
                } //we simply zero output when other waveform is mixed with noise. On real SID LFSR continuously gets filled by zero and locks up. ($C1 waveform with pw<8 can keep it for a while.)
                WavGenOut = /*(WF & 0x70) ? 0 :*/ ((Tmp & 0x100000) >> 5) | ((Tmp & 0x40000) >> 4) | ((Tmp & 0x4000) >> 1) | ((Tmp & 0x800) << 1)
                            | ((Tmp & 0x200) << 2) | ((Tmp & 0x20) << 5) | ((Tmp & 0x04) << 7) | ((Tmp & 0x01) << 8);
            } break;
            case PULSE_BITVAL:
            { //simple pulse
                unsigned int PW = getPW(ChannelPtr); //PW=0000..FFF0 from SID-register
                unsigned int Utmp = (int)(PhaseAccuStep >> (CRSID_WAVE_SHIFTS+1));
                if (UNLIKELY(0 < PW && PW < Utmp))
                    PW = Utmp; //Too thin pulsewidth? Correct...
                Utmp ^= CRSID_WAVE_MAX;
                if (UNLIKELY(PW > Utmp))
                    PW = Utmp; //Too thin pulsewidth? Correct it to a value representable at the current samplerate
                Utmp = *PhaseAccuPtr >> CRSID_WAVE_SHIFTS; // 12

                int Steepness = (PhaseAccuStep>=STEEPNESS_STEPLIMIT) ? PHASEACCU_MAX/PhaseAccuStep : CRSID_WAVE_MAX; //rising/falling-edge steepness (add/sub at samples)
                if (UNLIKELY(TestBit))
                    WavGenOut = CRSID_WAVE_MAX; //0xFFFF;
                else if (Utmp<PW)
                { //rising edge (interpolation)
                    int PulsePeak = (CRSID_WAVE_MAX-PW) * Steepness; //very thin pulses don't make a full swing between 0 and max but make a little spike
                    if (PulsePeak>CRSID_WAVE_MAX)
                        PulsePeak=CRSID_WAVE_MAX; //but adequately thick trapezoid pulses reach the maximum level
                    int Tmp = PulsePeak - (PW-Utmp)*Steepness; //draw the slope from the peak
                    WavGenOut = (LIKELY(Tmp<CRSID_WAVE_MIN)) ? CRSID_WAVE_MIN : Tmp; //but stop at 0-level
                }
                else
                { //falling edge (interpolation)
                    int PulsePeak = PW*Steepness; //very thin pulses don't make a full swing between 0 and max but make a little spike
                    if (PulsePeak>CRSID_WAVE_MAX)
                        PulsePeak=CRSID_WAVE_MAX; //adequately thick trapezoid pulses reach the maximum level
                    int Tmp = (CRSID_WAVE_MAX-Utmp)*Steepness - PulsePeak; //draw the slope from the peak
                    WavGenOut = (LIKELY(Tmp>=0)) ? CRSID_WAVE_MAX : Tmp;         //but stop at max-level
                }
            } break;
            case PULSAWTRI_VAL:
            { //pulse+saw+triangle (waveform nearly identical to tri+saw)
                unsigned int Utmp = *PhaseAccuPtr >> COMBINEDWF_SAMPLE_SHIFTS; //16; //12;
                WavGenOut = Utmp >= getCombinedPW(ChannelPtr) || UNLIKELY(TestBit)
                    ? combinedWF( PulseSawTriangle, Utmp) : CRSID_WAVE_MIN; //0;
            } break;
            case PULSAW_VAL:
            { //pulse+saw
                unsigned int Utmp = *PhaseAccuPtr >> COMBINEDWF_SAMPLE_SHIFTS; //16; //12;
                WavGenOut = Utmp >= getCombinedPW(ChannelPtr) //|| UNLIKELY(TestBit)
                            ? combinedWF( PulseSawtooth, Utmp) : CRSID_WAVE_MIN; //0;
            } break;
            case PULTRI_VAL:
            { //pulse+triangle
                int Tmp = *PhaseAccuPtr ^ ((WF&RING_BITVAL)? RingSourceMSB : 0);
                WavGenOut = (*PhaseAccuPtr >> COMBINEDWF_SAMPLE_SHIFTS) >= getCombinedPW(ChannelPtr) || UNLIKELY(TestBit)
                ? combinedWF(PulseTriangle, Tmp >> COMBINEDWF_SAMPLE_SHIFTS) : CRSID_WAVE_MIN; //0;
            } break;
            case SAWTRI_VAL:
            { //saw+triangle
                WavGenOut = combinedWF(SawTriangle, *PhaseAccuPtr >> COMBINEDWF_SAMPLE_SHIFTS);
            } break;
            case SAW_BITVAL:
            { //sawtooth
                WavGenOut = *PhaseAccuPtr >> CRSID_WAVE_SHIFTS; //12; //saw (this row would be enough for simple but aliased-at-high-pitch saw)
                int Steepness = (PhaseAccuStep>>CRSID_CLOCK_FRACTIONAL_BITS) / 288;
                if (UNLIKELY(Steepness==0))
                    Steepness = 1; //avoid division by zero in next steps
                WavGenOut += (WavGenOut * Steepness) >> STEEPNESS_FRACTION_SHIFTS; //16; //1st half (rising edge) of asymmetric triangle-like saw waveform
                if (UNLIKELY(WavGenOut>CRSID_WAVE_MAX))
                    WavGenOut = CRSID_WAVE_MAX - (((WavGenOut-CRSID_WAVE_RANGE)<<STEEPNESS_FRACTION_SHIFTS) / Steepness); //2nd half (falling edge, reciprocal steepness
            } break;
            case TRI_BITVAL:
            { //triangle (this waveform has no harsh edges, so it doesn't suffer from strong aliasing at high pitches)
                if (LIKELY(!RealSIDmode || PrevSounDemonDigiWF[Channel] <= 0))
                {
                    int Tmp = *PhaseAccuPtr ^ (UNLIKELY(WF&RING_BITVAL) ? RingSourceMSB : 0);
                    WavGenOut = (Tmp ^ (Tmp&PHASEACCU_MSB_BITVAL ? PHASEACCU_MAX : 0)) >> (CRSID_WAVE_SHIFTS-1); //11;
                }  //SounDemon digi hack: if previous waveform was 01, don't modify output in this round:
                else
                {
                    WavGenOut = PrevWavGenOut[Channel];
                    --PrevSounDemonDigiWF[Channel];
                } //(so carrier noise won't be heard due to non 1MHz emulation
            } break;
            case 0x00: //emulate waveform 00 floating wave-DAC (utilized by SounDemon digis) (on real SID waveform00 decays after about 5 seconds, here we just simply keep the value to avoid clicks)
                //(Our jittery 'seeking' waveform=$01 part of SounDemon-digi is substituted directly by frequency-high register's value (as in SwinSID))
                if (RealSIDmode && WF == SOUNDEMON_DIGI_SEEK_WAVEFORM)
                {
                    WavGenOut = ChannelPtr[1] << SOUNDEMON_DIGI_SHIFTS;
                    PrevSounDemonDigiWF[Channel] = SOUNDEMON_CARRIER_ELIMINATION_SAMPLECOUNT;
                }
                else
                    WavGenOut = PrevWavGenOut[Channel];
            break;
            default:
            {
                WavGenOut = CRSID_WAVE_MIN;
            } break; //noise plus pulse/saw/triangle mostly yields silence
        }

        WavGenOut &= CRSID_WAVE_MASK; //0xFFFF;
        //if (WF&0xF0)
            PrevWavGenOut[Channel] = WavGenOut; //emulate waveform 00 floating wave-DAC (utilized by SounDemon digis)
        //else
        //    WavGenOut = PrevWavGenOut[Channel];  //(on real SID waveform00 decays, we just simply keep the value to avoid clicks)
        PrevPhaseAccu[Channel] = *PhaseAccuPtr;
        RingSourceMSB = MSB;

        //routing the channel signal to either the filter or the unfiltered master output depending on filter-switch SID-registers
        unsigned char Envelope = (LIKELY(ChipModel==8580)) ? EnvelopeCounter[Channel] : ADSR_DAC_6581[EnvelopeCounter[Channel]];
        if (UNLIKELY(FilterSwitchReso & FilterSwitchVal[Channel]))
        {
            FilterInput += (((int)WavGenOut-CRSID_WAVE_MID) * Envelope) >> 8;// / ENVELOPE_MAGNITUDE_DIV;
        }
        else if (LIKELY(Channel!=14 || !(VolumeBand & OFF3_BITVAL)))
        {
            NonFilted += (((int)WavGenOut-CRSID_WAVE_MID) * Envelope) >> 8;// / ENVELOPE_MAGNITUDE_DIV;
        }
    }
    //update readable SID1-registers (some SID tunes might use 3rd channel ENV3/OSC3 value as control)
    oscReg = WavGenOut >> WAVE_OSC3_SHIFTS; //OSC3, ENV3 (some players rely on it, unfortunately even for timing)
    envReg = EnvelopeCounter[CHANNEL2_INDEX]; //Envelope

    //Filter

    int Cutoff = (regs[0x16] << 3) + (regs[0x15] & 7);
    int Resonance = FilterSwitchReso >> 4;
    if (ChipModel == 8580)
    {
        Cutoff = CutoffMul8580_44100Hz[Cutoff];
        Resonance = Resonances8580[Resonance];
    }
    else
    { //6581
        Cutoff += (FilterInput*105)>>16;
        if (Cutoff>0x7FF)
            Cutoff=0x7FF;
        else if (Cutoff<0)
            Cutoff=0; //MOSFET-VCR control-voltage-modulation
        Cutoff = CutoffMul6581_44100Hz[Cutoff]; //(resistance-modulation aka 6581 filter distortion) emulation
        Resonance = Resonances6581[Resonance];
    }

    int FilterOutput = 0;
    int Tmp = FilterInput + ((PrevBandPass * Resonance)>>12) + PrevLowPass;
    if (VolumeBand & HIGHPASS_BITVAL)
        FilterOutput -= Tmp;
    Tmp = PrevBandPass - ((Tmp * Cutoff) >> 12);
    PrevBandPass = Tmp;
    if (VolumeBand & BANDPASS_BITVAL)
        FilterOutput -= Tmp;
    Tmp = PrevLowPass + ((Tmp * Cutoff) >> 12);
    PrevLowPass = Tmp;
    if (VolumeBand & LOWPASS_BITVAL)
        FilterOutput += Tmp;

    //Output stage
    //For $D418 volume-register digi playback: an AC / DC separation for $D418 value at low (20Hz or so) cutoff-frequency,
    //sending AC (highpass) value to a 4th 'digi' channel mixed to the master output, and set ONLY the DC (lowpass) value to the volume-control.
    //This solved 2 issues: Thanks to the lowpass filtering of the volume-control, SID tunes where digi is played together with normal SID channels,
    //won't sound distorted anymore, and the volume-clicks disappear when setting SID-volume. (This is useful for fade-in/out tunes like Hades Nebula, where clicking ruins the intro.)
    char MainVolume;
    if (RealSIDmode)
    {
        Tmp = (signed int)((VolumeBand&0xF) << 12);
        NonFilted += (Tmp - PrevVolume) * D418_DIGI_VOLUME; //highpass is digi, adding it to output must be before digifilter-code
        PrevVolume += (Tmp - PrevVolume) >> 10; //arithmetic shift amount determines digi lowpass-frequency
        MainVolume = PrevVolume >> 12; //lowpass is main volume
    }
    else
        MainVolume = VolumeBand & 0xF;

    int Output = ((NonFilted+FilterOutput) * MainVolume) / ((CHANNELS*VOLUME_MAX) + Attenuation);

    return Output; // master output
}

int SID::generateSound(short *buf, unsigned int cycles)
{
    int i = 0;
    while (cycles>0)
    {
        buf[i] = generateSample(cycles);
        i++;
    }
    return i;
}

inline signed short SID::generateSample(unsigned int &cycles)
{
    //call this from custom buffer-filler
    int Output = emulateC64(cycles);
    if (Output>=32767)
        Output=32767;
    else if (Output<=-32768)
        Output=-32768; //saturation logic on overflow
    return (signed short) Output;
}


int SID::emulateC64(unsigned int &cycles)
{
    //Cycle-based part of emulations:

    while (SampleCycleCnt <= SampleClockRatio && cycles)
    {
        unsigned char InstructionCycles = std::min(7u, cycles);
        SampleCycleCnt += (InstructionCycles<<4);
        cycles -= InstructionCycles;

        emulateADSRs(InstructionCycles);
    }

    SampleCycleCnt -= SampleClockRatio;

    //Samplerate-based part of emulations:

    return emulateWaves();
}

}
