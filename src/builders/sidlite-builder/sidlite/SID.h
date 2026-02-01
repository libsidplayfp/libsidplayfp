// cRSID lightweight RealSID (integer-only) library-header (with API-calls) by Hermit (Mihaly Horvath)

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
    unsigned char regs[0xff];

    //SID-chip data:
    unsigned short     ChipModel;     //values: 8580 / 6581
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

    //C64-machine related:
    unsigned int      CPUfrequency;
    unsigned short    SampleClockRatio; //ratio of CPU-clock and samplerate
    unsigned short    Attenuation;
    bool              RealSIDmode;
    bool              PSIDdigiMode = false;
    //PSID-playback related:
    //int               FrameCycles;
    //int               FrameCycleCnt; //this is a substitution in PSID-mode for CIA/VIC counters
    short             SampleCycleCnt;
    unsigned short    SampleRate;

    unsigned char oscReg;
    unsigned char envReg;

enum cRSID_MemAddresses {
 CRSID_C64_MEMBANK_SIZE = 0x10000, CRSID_MEMBANK_SAFETY_ZONE_SIZE = 0x100, CRSID_SID_SAFETY_ZONE_SIZE = 0x100,
 CRSID_MEMBANK_SIZE = (CRSID_C64_MEMBANK_SIZE + CRSID_MEMBANK_SAFETY_ZONE_SIZE + CRSID_SID_SAFETY_ZONE_SIZE),
 CRSID_SID_SAFE_ADDRESS = (CRSID_C64_MEMBANK_SIZE + CRSID_MEMBANK_SAFETY_ZONE_SIZE)
};

    unsigned char RAMbank[CRSID_MEMBANK_SIZE];

private:
    void emulateADSRs(char cycles);
    int emulateWaves();

    int generateSound(short* buf, unsigned int cycles);
    inline signed short generateSample(unsigned int &cycles);
    int emulateC64(unsigned int &cycles);
    inline short playPSIDdigi();
};

}

#endif // SIDLITE_H
