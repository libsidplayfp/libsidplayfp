
#include "SID.h"


namespace SIDLite
{


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
    }
    SyncSourceMSBrise = 0;
    RingSourceMSB = 0;
    PrevLowPass = PrevBandPass = PrevVolume = 0;
}

void SID::write(int addr, int value)
{
}

int SID::read(int addr)
{
    return 0;
}

int SID::clock(unsigned int cycles, short* buf)
{
    return 0;
}

void SID::setChipModel(int model)
{
    ChipModel = model;
}

void SID::setSamplingParameters(unsigned int CPUfrequency, unsigned short SampleRate)
{
    SampleClockRatio = ( CPUfrequency << 4 ) / SampleRate; //shifting (multiplication) enhances SampleClockRatio precision
    Attenuation = 26;
}

}

/*


void cRSID_generateSound(cRSID_C64instance* C64instance, unsigned char *buf, unsigned short len) {
 static unsigned short i;
 static int Output;
 for(i=0;i<len;i+=2) {
  Output=cRSID_generateSample(C64instance); //cRSID_emulateC64(C64instance);
  //if (Output>=32767) Output=32767; else if (Output<=-32768) Output=-32768; //saturation logic on overflow
  buf[i]=Output&0xFF; buf[i+1]=Output>>8;
 }
}

static inline signed short cRSID_generateSample (cRSID_C64instance* C64) { //call this from custom buffer-filler
 static int Output;
 Output=cRSID_emulateC64(C64);
 if (C64->PSIDdigiMode) Output += cRSID_playPSIDdigi(C64);
 if (Output>=32767) Output=32767; else if (Output<=-32768) Output=-32768; //saturation logic on overflow
 return (signed short) Output;
}

int cRSID_emulateC64 (cRSID_C64instance *C64) {
 static unsigned char InstructionCycles;
 static int Output;


 //Cycle-based part of emulations:


 while (C64->SampleCycleCnt <= C64->SampleClockRatio) {

  if (!C64->RealSIDmode) {
   if (C64->FrameCycleCnt >= C64->FrameCycles) {
    C64->FrameCycleCnt -= C64->FrameCycles;
    if (C64->Finished) { //some tunes (e.g. Barbarian, A-Maze-Ing) doesn't always finish in 1 frame
     cRSID_initCPU ( &C64->CPU, C64->PlayAddress ); //(PSID docs say bank-register should always be set for each call's region)
     C64->Finished=0; //C64->SampleCycleCnt=0; //PSID workaround for some tunes (e.g. Galdrumway):
     if (C64->TimerSource==0) C64->IObankRD[0xD019] = 0x81; //always simulate to player-calls that VIC-IRQ happened
     else C64->IObankRD[0xDC0D] = 0x83; //always simulate to player-calls that CIA TIMERA/TIMERB-IRQ happened
   }}
   if (C64->Finished==0) {
    if ( (InstructionCycles = cRSID_emulateCPU()) >= 0xFE ) { InstructionCycles=6; C64->Finished=1; }
   }
   else InstructionCycles=7; //idle between player-calls
   C64->FrameCycleCnt += InstructionCycles;
   C64->IObankRD[0xDC04] += InstructionCycles; //very simple CIA1 TimerA simulation for PSID (e.g. Delta-Mix_E-Load_loader)
  }

  else { //RealSID emulations:
   if ( cRSID_handleCPUinterrupts(&C64->CPU) ) { C64->Finished=0; InstructionCycles=7; }
   else if (C64->Finished==0) {
    if ( (InstructionCycles = cRSID_emulateCPU()) >= 0xFE ) {
     InstructionCycles=6; C64->Finished=1;
    }
   }
   else InstructionCycles=7; //idle between IRQ-calls
   C64->IRQ = C64->NMI = 0; //prepare for collecting IRQ sources
   C64->IRQ |= cRSID_emulateCIA (&C64->CIA[1], InstructionCycles);
   C64->NMI |= cRSID_emulateCIA (&C64->CIA[2], InstructionCycles);
   C64->IRQ |= cRSID_emulateVIC (&C64->VIC, InstructionCycles);
  }

  C64->SampleCycleCnt += (InstructionCycles<<4);

  cRSID_emulateADSRs (&C64->SID[1], InstructionCycles);
  if ( C64->SID[2].BaseAddress != 0 ) cRSID_emulateADSRs (&C64->SID[2], InstructionCycles);
  if ( C64->SID[3].BaseAddress != 0 ) cRSID_emulateADSRs (&C64->SID[3], InstructionCycles);

 }
 C64->SampleCycleCnt -= C64->SampleClockRatio;


 //Samplerate-based part of emulations:


 Output = cRSID_emulateWaves (&C64->SID[1]);
 if ( C64->SID[2].BaseAddress != 0 ) Output += cRSID_emulateWaves (&C64->SID[2]);
 if ( C64->SID[3].BaseAddress != 0 ) Output += cRSID_emulateWaves (&C64->SID[3]);

 return Output;
}


static inline short cRSID_playPSIDdigi(cRSID_C64instance* C64) {
 enum PSIDdigiSpecs { DIGI_VOLUME = 1200 }; //80 };
 static unsigned char PlaybackEnabled=0, NybbleCounter=0, RepeatCounter=0, Shifts;
 static unsigned short SampleAddress, RatePeriod;
 static short Output=0;
 static int PeriodCounter;

 if (C64->IObankWR[0xD41D]) {
  PlaybackEnabled = (C64->IObankWR[0xD41D] >= 0xFE);
  PeriodCounter = 0; NybbleCounter = 0;
  SampleAddress = C64->IObankWR[0xD41E] + (C64->IObankWR[0xD41F]<<8);
  RepeatCounter = C64->IObankWR[0xD43F];
 }
 C64->IObankWR[0xD41D] = 0;

 if (PlaybackEnabled) {
  RatePeriod = C64->IObankWR[0xD45D] + (C64->IObankWR[0xD45E]<<8);
  if (RatePeriod) PeriodCounter += C64->CPUfrequency / RatePeriod;
  if ( PeriodCounter >= C64->SampleRate ) {
   PeriodCounter -= C64->SampleRate;

   if ( SampleAddress < C64->IObankWR[0xD43D] + (C64->IObankWR[0xD43E]<<8) ) {
    if (NybbleCounter) {
     Shifts = C64->IObankWR[0xD47D] ? 4:0;
     ++SampleAddress;
    }
    else Shifts = C64->IObankWR[0xD47D] ? 0:4;
    Output = ( ( (C64->RAMbank[SampleAddress]>>Shifts) & 0xF) - 8 ) * DIGI_VOLUME; //* (C64->IObankWR[0xD418]&0xF);
    NybbleCounter^=1;
   }
   else if (RepeatCounter) {
    SampleAddress = C64->IObankWR[0xD47F] + (C64->IObankWR[0xD47E]<<8);
    RepeatCounter--;
   }

  }
 }

 return Output;
}

---------------------


//cRSID SID emulation engine



void cRSID_emulateADSRs (cRSID_SIDinstance *SID, char cycles) {

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

 static unsigned char Channel, PrevGate, AD, SR;
 static unsigned short PrescalePeriod;
 static unsigned char *ChannelPtr, *ADSRstatePtr, *EnvelopeCounterPtr, *ExponentCounterPtr;
 static unsigned short *RateCounterPtr;


 for (Channel=0; Channel<21; Channel+=7) {

  ChannelPtr=&SID->BasePtr[Channel]; AD=ChannelPtr[5]; SR=ChannelPtr[6];
  ADSRstatePtr = &(SID->ADSRstate[Channel]);
  RateCounterPtr = &(SID->RateCounter[Channel]);
  EnvelopeCounterPtr = &(SID->EnvelopeCounter[Channel]);
  ExponentCounterPtr = &(SID->ExponentCounter[Channel]);

  PrevGate = (*ADSRstatePtr & GATE_BITVAL);
  if ( PrevGate != (ChannelPtr[4] & GATE_BITVAL) ) { //gatebit-change?
   if (PrevGate) *ADSRstatePtr &= ~ (GATE_BITVAL | ATTACK_BITVAL | DECAYSUSTAIN_BITVAL); //falling edge
   else *ADSRstatePtr = (GATE_BITVAL | ATTACK_BITVAL | DECAYSUSTAIN_BITVAL | HOLDZEROn_BITVAL); //rising edge
  }

  if (*ADSRstatePtr & ATTACK_BITVAL) PrescalePeriod = ADSRprescalePeriods[ AD >> 4 ];
  else if (*ADSRstatePtr & DECAYSUSTAIN_BITVAL) PrescalePeriod = ADSRprescalePeriods[ AD & 0x0F ];
  else PrescalePeriod = ADSRprescalePeriods[ SR & 0x0F ];

  *RateCounterPtr += cycles; if (*RateCounterPtr >= 0x8000) *RateCounterPtr -= 0x8000; //*RateCounterPtr &= 0x7FFF; //can wrap around (ADSR delay-bug: short 1st frame)

  if (PrescalePeriod <= *RateCounterPtr && *RateCounterPtr < PrescalePeriod+cycles) { //ratecounter shot (matches rateperiod) (in genuine SID ratecounter is LFSR)
   *RateCounterPtr -= PrescalePeriod; //reset rate-counter on period-match
   if ( (*ADSRstatePtr & ATTACK_BITVAL) || ++(*ExponentCounterPtr) == ADSRexponentPeriods[*EnvelopeCounterPtr] ) {
    *ExponentCounterPtr = 0;
    if (*ADSRstatePtr & HOLDZEROn_BITVAL) {
     if (*ADSRstatePtr & ATTACK_BITVAL) {
      ++(*EnvelopeCounterPtr);
      if (*EnvelopeCounterPtr==0xFF) *ADSRstatePtr &= ~ATTACK_BITVAL;
     }
     else if ( !(*ADSRstatePtr & DECAYSUSTAIN_BITVAL) || *EnvelopeCounterPtr != (SR&0xF0)+(SR>>4) ) {
      --(*EnvelopeCounterPtr); //resid adds 1 cycle delay, we omit that mechanism here
      if (*EnvelopeCounterPtr==0) *ADSRstatePtr &= ~HOLDZEROn_BITVAL;
 }}}}}

}



int cRSID_emulateWaves (cRSID_SIDinstance *SID) {

 enum SIDspecs { CHANNELS=3+1, VOLUME_MAX=0xF, D418_DIGI_VOLUME=2 }; //digi-channel is counted too
 enum WaveFormBits { NOISE_BITVAL=0x80, PULSE_BITVAL=0x40, SAW_BITVAL=0x20, TRI_BITVAL=0x10 };
 enum ControlBits { TEST_BITVAL=0x08, RING_BITVAL=0x04, SYNC_BITVAL=0x02, GATE_BITVAL=0x01 };
 enum FilterBits { OFF3_BITVAL=0x80, HIGHPASS_BITVAL=0x40, BANDPASS_BITVAL=0x20, LOWPASS_BITVAL=0x10 };

 #include "SID.h"
 static const unsigned char FilterSwitchVal[] = {1,1,1,1,1,1,1,2,2,2,2,2,2,2,4};

 static char MainVolume;
 static unsigned char Channel, WF, TestBit, Envelope, FilterSwitchReso, VolumeBand;
 static unsigned int Utmp, PhaseAccuStep, MSB, WavGenOut, PW;
 static int Tmp, Feedback, Steepness, PulsePeak;
 static int FilterInput, Cutoff, Resonance, FilterOutput, NonFilted, Output;
 static unsigned char *ChannelPtr;
 static int *PhaseAccuPtr;


 inline unsigned short combinedWF( const unsigned char* WFarray, unsigned short oscval) {
  static unsigned char Pitch;
  static unsigned short Filt;
  if (SID->ChipModel==6581 && WFarray!=PulseTriangle) oscval &= 0x7FFF;
  Pitch = ChannelPtr[1] ? ChannelPtr[1] : 1; //avoid division by zero
  Filt = 0x7777 + (0x8888/Pitch);
  SID->PrevWavData[Channel] = ( WFarray[oscval>>4]*Filt + SID->PrevWavData[Channel]*(0xFFFF-Filt) ) >> 16;
  return SID->PrevWavData[Channel] << 8;
 }


 FilterInput = NonFilted = 0;
 FilterSwitchReso = SID->BasePtr[0x17]; VolumeBand=SID->BasePtr[0x18];


 //Waveform-generator //(phase accumulator and waveform-selector)


 for (Channel=0; Channel<21; Channel+=7) {
  ChannelPtr=&(SID->BasePtr[Channel]);

  WF = ChannelPtr[4]; TestBit = ( (WF & TEST_BITVAL) != 0 );
  PhaseAccuPtr = &(SID->PhaseAccu[Channel]);


  PhaseAccuStep = ( (ChannelPtr[1]<<8) + ChannelPtr[0] ) * SID->C64->SampleClockRatio;
  if (TestBit || ((WF & SYNC_BITVAL) && SID->SyncSourceMSBrise)) *PhaseAccuPtr = 0;
  else { //stepping phase-accumulator (oscillator)
   *PhaseAccuPtr += PhaseAccuStep;
   if (*PhaseAccuPtr >= 0x10000000) *PhaseAccuPtr -= 0x10000000;
  }
  *PhaseAccuPtr &= 0xFFFFFFF;
  MSB = *PhaseAccuPtr & 0x8000000;
  SID->SyncSourceMSBrise = (MSB > (SID->PrevPhaseAccu[Channel] & 0x8000000)) ? 1 : 0;


  if (WF & NOISE_BITVAL) { //noise waveform
   Tmp = SID->NoiseLFSR[Channel]; //clock LFSR all time if clockrate exceeds observable at given samplerate (last term):
   if ( ((*PhaseAccuPtr & 0x1000000) != (SID->PrevPhaseAccu[Channel] & 0x1000000)) || PhaseAccuStep >= 0x1000000 ) {
    Feedback = ( (Tmp & 0x400000) ^ ((Tmp & 0x20000) << 5) ) != 0;
    Tmp = ( (Tmp << 1) | Feedback|TestBit ) & 0x7FFFFF; //TEST-bit turns all bits in noise LFSR to 1 (on real SID slowly, in approx. 8000 microseconds ~ 300 samples)
    SID->NoiseLFSR[Channel] = Tmp;
   } //we simply zero output when other waveform is mixed with noise. On real SID LFSR continuously gets filled by zero and locks up. ($C1 waveform with pw<8 can keep it for a while.)
   WavGenOut = (WF & 0x70) ? 0 : ((Tmp & 0x100000) >> 5) | ((Tmp & 0x40000) >> 4) | ((Tmp & 0x4000) >> 1) | ((Tmp & 0x800) << 1)
                                 | ((Tmp & 0x200) << 2) | ((Tmp & 0x20) << 5) | ((Tmp & 0x04) << 7) | ((Tmp & 0x01) << 8);
  }

  else if (WF & PULSE_BITVAL) { //simple pulse
   PW = ( ((ChannelPtr[3]&0xF) << 8) + ChannelPtr[2] ) << 4; //PW=0000..FFF0 from SID-register
   Utmp = (int) (PhaseAccuStep >> 13); if (0 < PW && PW < Utmp) PW = Utmp; //Too thin pulsewidth? Correct...
   Utmp ^= 0xFFFF;  if (PW > Utmp) PW = Utmp; //Too thin pulsewidth? Correct it to a value representable at the current samplerate
   Utmp = *PhaseAccuPtr >> 12;

   if ( (WF&0xF0) == PULSE_BITVAL ) { //simple pulse, most often used waveform, make it sound as clean as possible (by making it trapezoid)
    Steepness = (PhaseAccuStep>=4096) ? 0xFFFFFFF/PhaseAccuStep : 0xFFFF; //rising/falling-edge steepness (add/sub at samples)
    if (TestBit) WavGenOut = 0xFFFF;
    else if (Utmp<PW) { //rising edge (interpolation)
     PulsePeak = (0xFFFF-PW) * Steepness; //very thin pulses don't make a full swing between 0 and max but make a little spike
     if (PulsePeak>0xFFFF) PulsePeak=0xFFFF; //but adequately thick trapezoid pulses reach the maximum level
     Tmp = PulsePeak - (PW-Utmp)*Steepness; //draw the slope from the peak
     WavGenOut = (Tmp<0)? 0:Tmp;           //but stop at 0-level
    }
    else { //falling edge (interpolation)
     PulsePeak = PW*Steepness; //very thin pulses don't make a full swing between 0 and max but make a little spike
     if (PulsePeak>0xFFFF) PulsePeak=0xFFFF; //adequately thick trapezoid pulses reach the maximum level
     Tmp = (0xFFFF-Utmp)*Steepness - PulsePeak; //draw the slope from the peak
     WavGenOut = (Tmp>=0)? 0xFFFF:Tmp;         //but stop at max-level
   }}

   else { //combined pulse
    WavGenOut = (Utmp >= PW || TestBit) ? 0xFFFF:0;
    if (WF & TRI_BITVAL) {
     if (WF & SAW_BITVAL) { //pulse+saw+triangle (waveform nearly identical to tri+saw)
      if (WavGenOut) WavGenOut = combinedWF( PulseSawTriangle, Utmp);
     }
     else { //pulse+triangle
      Tmp = *PhaseAccuPtr ^ ( (WF&RING_BITVAL)? SID->RingSourceMSB : 0 );
      if (WavGenOut) WavGenOut = combinedWF( PulseTriangle, Tmp >> 12);
    }}
    else if (WF & SAW_BITVAL) { //pulse+saw
     if(WavGenOut) WavGenOut = combinedWF( PulseSawtooth, Utmp);
   }}
  }

  else if (WF & SAW_BITVAL) { //sawtooth
   WavGenOut = *PhaseAccuPtr >> 12; //saw (this row would be enough for simple but aliased-at-high-pitch saw)
   if (WF & TRI_BITVAL) WavGenOut = combinedWF( SawTriangle, WavGenOut); //saw+triangle
   else { //simple cleaned (bandlimited) saw
    Steepness = (PhaseAccuStep>>4)/288; if(Steepness==0) Steepness=1; //avoid division by zero in next steps
    WavGenOut += (WavGenOut * Steepness) >> 16; //1st half (rising edge) of asymmetric triangle-like saw waveform
    if (WavGenOut>0xFFFF) WavGenOut = 0xFFFF - ( ((WavGenOut-0x10000)<<16) / Steepness ); //2nd half (falling edge, reciprocal steepness)
  }}

  else if (WF & TRI_BITVAL) { //triangle (this waveform has no harsh edges, so it doesn't suffer from strong aliasing at high pitches)
   Tmp = *PhaseAccuPtr ^ ( WF&RING_BITVAL? SID->RingSourceMSB : 0 );
   WavGenOut = ( Tmp ^ (Tmp&0x8000000? 0xFFFFFFF:0) ) >> 11;
  }


  WavGenOut &= 0xFFFF;
  if (WF&0xF0) SID->PrevWavGenOut[Channel] = WavGenOut; //emulate waveform 00 floating wave-DAC (utilized by SounDemon digis)
  else WavGenOut = SID->PrevWavGenOut[Channel];  //(on real SID waveform00 decays, we just simply keep the value to avoid clicks)
  SID->PrevPhaseAccu[Channel] = *PhaseAccuPtr;
  SID->RingSourceMSB = MSB;

  //routing the channel signal to either the filter or the unfiltered master output depending on filter-switch SID-registers
  Envelope = SID->ChipModel==8580 ?  SID->EnvelopeCounter[Channel] : ADSR_DAC_6581[SID->EnvelopeCounter[Channel]];
  if (FilterSwitchReso & FilterSwitchVal[Channel]) {
   FilterInput += ( ((int)WavGenOut-0x8000) * Envelope ) >> 8;
  }
  else if ( Channel!=14 || !(VolumeBand & OFF3_BITVAL) ) {
   NonFilted += ( ((int)WavGenOut-0x8000) * Envelope ) >> 8;
  }

 }
 //update readable SID1-registers (some SID tunes might use 3rd channel ENV3/OSC3 value as control)
 SID->C64->IObankRD[SID->BaseAddress+0x1B] = WavGenOut>>8; //OSC3, ENV3 (some players rely on it, unfortunately even for timing)
 SID->C64->IObankRD[SID->BaseAddress+0x1C] = SID->EnvelopeCounter[14]; //Envelope


 //Filter


 Cutoff = (SID->BasePtr[0x16] << 3) + (SID->BasePtr[0x15] & 7);
 Resonance = FilterSwitchReso >> 4;
 if (SID->ChipModel == 8580) {
  Cutoff = CutoffMul8580_44100Hz[Cutoff];
  Resonance = Resonances8580[Resonance];
 }
 else { //6581
  Cutoff += (FilterInput*105)>>16; if (Cutoff>0x7FF) Cutoff=0x7FF; else if (Cutoff<0) Cutoff=0; //MOSFET-VCR control-voltage-modulation
  Cutoff = CutoffMul6581_44100Hz[Cutoff]; //(resistance-modulation aka 6581 filter distortion) emulation
  Resonance = Resonances6581[Resonance];
 }

 FilterOutput=0;
 Tmp = FilterInput + ((SID->PrevBandPass * Resonance)>>12) + SID->PrevLowPass;
 if (VolumeBand & HIGHPASS_BITVAL) FilterOutput -= Tmp;
 Tmp = SID->PrevBandPass - ( (Tmp * Cutoff) >> 12 );
 SID->PrevBandPass = Tmp;
 if (VolumeBand & BANDPASS_BITVAL) FilterOutput -= Tmp;
 Tmp = SID->PrevLowPass + ( (Tmp * Cutoff) >> 12 );
 SID->PrevLowPass = Tmp;
 if (VolumeBand & LOWPASS_BITVAL) FilterOutput += Tmp;


 //Output stage
 //For $D418 volume-register digi playback: an AC / DC separation for $D418 value at low (20Hz or so) cutoff-frequency,
 //sending AC (highpass) value to a 4th 'digi' channel mixed to the master output, and set ONLY the DC (lowpass) value to the volume-control.
 //This solved 2 issues: Thanks to the lowpass filtering of the volume-control, SID tunes where digi is played together with normal SID channels,
 //won't sound distorted anymore, and the volume-clicks disappear when setting SID-volume. (This is useful for fade-in/out tunes like Hades Nebula, where clicking ruins the intro.)
 if (SID->C64->RealSIDmode) {
  Tmp = (signed int) ( (VolumeBand&0xF) << 12 );
  NonFilted += (Tmp - SID->PrevVolume) * D418_DIGI_VOLUME; //highpass is digi, adding it to output must be before digifilter-code
  SID->PrevVolume += (Tmp - SID->PrevVolume) >> 10; //arithmetic shift amount determines digi lowpass-frequency
  MainVolume = SID->PrevVolume >> 12; //lowpass is main volume
 }
 else MainVolume = VolumeBand & 0xF;

 Output = ((NonFilted+FilterOutput) * MainVolume) / ( (CHANNELS*VOLUME_MAX) + SID->C64->Attenuation );

 return Output; // master output

}


*/