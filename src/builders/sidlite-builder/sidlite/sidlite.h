// cRSID lightweight RealSID (integer-only) library-header (with API-calls) by Hermit (Mihaly Horvath)

#ifndef SIDLITE_H
#define SIDLITE_H


namespace SIDLite
{

class SID
{
    SID::SID();
    void reset();
    void write(int addr, int value);
    int read(int addr);
    void clock(int cycles, void* buf);
    void setChipModel(int model);
};

}
/*
typedef struct cRSID_C64instance cRSID_C64instance;
typedef struct cRSID_SIDinstance cRSID_SIDinstance;

extern cRSID_C64instance cRSID_C64; //the only global object (for faster & simpler access than with struct-pointers, in some places)


// Main API functions (mainly in libcRSID.c)
cRSID_C64instance* cRSID_init           (unsigned short samplerate, unsigned short buflen); //init emulation objects and sound

static inline signed short cRSID_generateSample (cRSID_C64instance* C64); //in host/audio.c, calculate a single sample


//Internal functions

// C64/C64.c
cRSID_C64instance*  cRSID_createC64     (cRSID_C64instance* C64, unsigned short samplerate);
int                 cRSID_emulateC64    (cRSID_C64instance* C64);
static inline short cRSID_playPSIDdigi  (cRSID_C64instance* C64);

// C64/SID.c
void               cRSID_createSIDchip (cRSID_C64instance* C64, cRSID_SIDinstance* SID, unsigned short model);
void               cRSID_initSIDchip   (cRSID_SIDinstance* SID);
void               cRSID_emulateADSRs  (cRSID_SIDinstance *SID, char cycles);
int                cRSID_emulateWaves  (cRSID_SIDinstance* SID);

// host/audio.c
void               cRSID_generateSound (cRSID_C64instance* C64, unsigned char* buf, unsigned short len);


struct cRSID_SIDinstance {
 //SID-chip data:
 cRSID_C64instance* C64;           //reference to the containing C64
 unsigned short     ChipModel;     //values: 8580 / 6581
 unsigned short     BaseAddress;   //SID-baseaddress location in C64-memory (IO)
 unsigned char*     BasePtr;       //SID-baseaddress location in host's memory
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
 unsigned int       PrevWavGenOut[15];
 unsigned char      PrevWavData[15];
 //Filter-related:
 int                PrevLowPass;
 int                PrevBandPass;
 //Output-stage:
 signed int         PrevVolume; //lowpass-filtered version of Volume-band register
};


struct cRSID_C64instance {
 //platform-related:
 unsigned short    SampleRate;
 //C64-machine related:
 unsigned char     VideoStandard; //0:NTSC, 1:PAL (based on the SID-header field)
 unsigned int      CPUfrequency;
 unsigned short    SampleClockRatio; //ratio of CPU-clock and samplerate
 unsigned short    Attenuation;
 char              RealSIDmode;
 char              PSIDdigiMode;
 //PSID-playback related:
 int               FrameCycles;
 int               FrameCycleCnt; //this is a substitution in PSID-mode for CIA/VIC counters
 short             SampleCycleCnt;
};
*/

#endif // SIDLITE_H
