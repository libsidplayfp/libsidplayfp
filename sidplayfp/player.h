/***************************************************************************
                          player.h  -  description
                             -------------------
    begin                : Fri Jun 9 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _player_h_
#define _player_h_

#include "sid2types.h"
#include "SidTune.h"
#include "sidbuilder.h"

#include "sidenv.h"
#include "c64env.h"
#include "c64/c64xsid.h"
#include "c64/c64cia.h"
#include "c64/c64vic.h"
#include "c64/mmu.h"

#include "mos6510/mos6510.h"
#include "nullsid.h"


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef PC64_TESTSUITE
#  include <string.h>
#endif

#define  SID2_MAX_SIDS 2
#define  SID2_MAPPER_SIZE 32

SIDPLAY2_NAMESPACE_START

class Player: public C64Environment, private c64env
{
private:
    static const double CLOCK_FREQ_NTSC;
    static const double CLOCK_FREQ_PAL;
    static const double VIC_FREQ_PAL;
    static const double VIC_FREQ_NTSC;

    static const char  TXT_PAL_VBI[];
    static const char  TXT_PAL_VBI_FIXED[];
    static const char  TXT_PAL_CIA[];
    static const char  TXT_PAL_UNKNOWN[];
    static const char  TXT_NTSC_VBI[];
    static const char  TXT_NTSC_VBI_FIXED[];
    static const char  TXT_NTSC_CIA[];
    static const char  TXT_NTSC_UNKNOWN[];
    static const char  TXT_NA[];

    static const char  ERR_CONF_WHILST_ACTIVE[];
    static const char  ERR_UNSUPPORTED_FREQ[];
    static const char  ERR_UNSUPPORTED_PRECISION[];
    static const char  ERR_MEM_ALLOC[];
    static const char  ERR_UNSUPPORTED_MODE[];
    static const char  ERR_UNKNOWN_ROM[];
    static const char  *credit[10]; // 10 credits max

    static const char  ERR_PSIDDRV_NO_SPACE[];
    static const char  ERR_PSIDDRV_RELOC[];

    EventScheduler m_scheduler;

    MOS6510 cpu;
    // Sid objects to use.
    NullSID nullsid;
    c64xsid xsid;
    c64cia1 cia;
    c64cia2 cia2;
    c64vic  vic;
    sidemu *sid[SID2_MAX_SIDS];
    int     m_sidmapper[32]; // Mapping table in d4xx-d7xx

    EventCallback<Player> m_mixerEvent;

    // User Configuration Settings
    SidTuneInfo   m_tuneInfo;
    SidTune      *m_tune;
    sid2_info_t   m_info;
    sid2_config_t m_cfg;

    const char     *m_errorString;
    int             m_fastForwardFactor;
    uint_least32_t  m_mileage;
    int_least32_t   m_leftVolume;
    int_least32_t   m_rightVolume;
    volatile sid2_player_t m_playerState;
    volatile bool   m_running;
    int             m_rand;

    float64_t m_cpuFreq;

    bool           m_status;

    // Mixer settings
    uint_least32_t m_sampleCount;
    uint_least32_t m_sampleIndex;
    short         *m_sampleBuffer;

    // RTC clock - Based of mixer sample period
    // to make sure time is right when we ask
    // for n samples
    event_clock_t  m_rtcClock;
    event_clock_t  m_rtcPeriod;

    // C64 environment settings
    MMU mmu;

private:
    float64_t clockSpeed     (sid2_clock_t clock, sid2_clock_t defaultClock,
                              const bool forced);
    int       initialise     (void);
    void      nextSequence   (void);
    void      mixer          (void);
    void      mixerReset     (void);
    void      mileageCorrect (void);
    int       sidCreate      (sidbuilder *builder, sid2_model_t model,
                              sid2_model_t defaultModel);
    void      sidSamples     (const bool enable);
    void      reset          ();
    uint8_t   iomap          (const uint_least16_t addr);
    sid2_model_t getModel(const int sidModel,
                          sid2_model_t userModel,
                          sid2_model_t defaultModel);

    uint8_t readMemByte_plain     (const uint_least16_t addr);
    uint8_t readMemByte_io        (const uint_least16_t addr);
    void    writeMemByte_plain    (const uint_least16_t addr, const uint8_t data);
    void    writeMemByte_playsid  (const uint_least16_t addr, const uint8_t data);


    uint8_t m_readMemByte    (const uint_least16_t);
    void    m_writeMemByte   (const uint_least16_t, const uint8_t);

    uint8_t  readMemRamByte (const uint_least16_t addr)
    { return mmu.readMemByte(addr); }

    uint16_t getChecksum(const uint8_t* rom, const int size);

    // Environment Function entry Points
    void           envReset           (void);
    inline uint8_t envReadRomByte     (const uint_least16_t addr);
    inline uint8_t envReadMemByte     (const uint_least16_t addr);
    inline void    envWriteMemByte    (const uint_least16_t addr, const uint8_t data);
    inline uint8_t envReadMemDataByte (const uint_least16_t addr);

#ifdef PC64_TESTSUITE
    void   envLoadFile (const char *file)
    {
        char name[0x100] = PC64_TESTSUITE;
        strcat (name, file);
        strcat (name, ".prg");

        m_tune->load (name);
        stop ();
    }
#endif

    // Rev 2.0.3 Added - New Mixer Routines
    uint_least32_t (Player::*output) (char *buffer);

    inline void interruptIRQ (const bool state);
    inline void interruptNMI (void);
    inline void interruptRST (void);
    void signalAEC (const bool state) { cpu.aecSignal (state); }
    void lightpen  () { vic.lightpen (); }

    // PSID driver
    int  psidDrvReloc   (SidTuneInfo &tuneInfo, sid2_info_t &info);
    void psidDrvInstall (sid2_info_t &info);
    void psidRelocAddr  (SidTuneInfo &tuneInfo, int startp, int endp);

public:
    Player ();
    ~Player () {}

    const sid2_config_t &config (void) const { return m_cfg; }
    const sid2_info_t   &info   (void) const { return m_info; }

    int            config       (const sid2_config_t &cfg);
    int            fastForward  (uint percent);
    int            load         (SidTune *tune);
    uint_least32_t mileage      (void) const { return m_mileage + time(); }
    float64_t      cpuFreq      (void) const { return m_cpuFreq; }
    void           pause        (void);
    uint_least32_t play         (short *buffer, uint_least32_t samples);
    sid2_player_t  state        (void) const { return m_playerState; }
    void           stop         (void);
    uint_least32_t time         (void) const {return (uint_least32_t)(context().getTime(EVENT_CLOCK_PHI1) / m_cpuFreq); }
    void           debug        (bool enable, FILE *out)
                                { cpu.debug (enable, out); }
    void           mute         (int voice, bool enable);
    const char    *error        (void) const { return m_errorString; }

    bool           getStatus() const { return m_status; }

    void setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character);
};


uint8_t Player::envReadMemByte (const uint_least16_t addr)
{   // Read from plain only to prevent execution of rom code
    return m_readMemByte (addr);
}

void Player::envWriteMemByte (const uint_least16_t addr, uint8_t data)
{   // Writes must be passed to env version.
    m_writeMemByte (addr, data);
}

uint8_t Player::envReadMemDataByte (const uint_least16_t addr)
{   // Read from plain only to prevent execution of rom code
    return m_readMemByte (addr);
}

void Player::interruptIRQ (const bool state)
{
    if (state)
    {
        cpu.triggerIRQ ();
    }
    else
    {
        cpu.clearIRQ ();
    }
}

void Player::interruptNMI ()
{
    cpu.triggerNMI ();
}

void Player::interruptRST ()
{
    stop ();
}

SIDPLAY2_NAMESPACE_STOP

#endif // _player_h_
