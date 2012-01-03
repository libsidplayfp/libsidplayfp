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
#include "c64/c64cia.h"
#include "c64/c64vic.h"
#include "c64/mmu.h"
#include "mixer.h"

#include "mos6510/mos6510.h"


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

    static const char  ERR_UNSUPPORTED_FREQ[];
    static const char  ERR_UNSUPPORTED_PRECISION[];
    static const char  ERR_UNKNOWN_ROM[];
    static const char  ERR_PSIDDRV_NO_SPACE[];
    static const char  ERR_PSIDDRV_RELOC[];

    static const char  *credit[10]; // 10 credits max

    EventScheduler m_scheduler;

    MOS6510 cpu;
    // Sid objects to use.
    c64cia1 cia;
    c64cia2 cia2;
    c64vic  vic;
    sidemu *sid[SID2_MAX_SIDS];
    int     m_sidmapper[32]; // Mapping table in d4xx-d7xx

    Mixer   m_mixer;

    // User Configuration Settings
    SidTuneInfo   m_tuneInfo;
    SidTune      *m_tune;
    sid2_info_t   m_info;
    sid2_config_t m_cfg;

    const char     *m_errorString;
    uint_least32_t  m_mileage;
    volatile sid2_player_t m_playerState;
    int             m_rand;

    float64_t m_cpuFreq;

    bool           m_status;

    // C64 environment settings
    MMU mmu;

private:
    float64_t clockSpeed     (sid2_clock_t clock, sid2_clock_t defaultClock,
                              const bool forced);
    int       initialise     (void);
    int       sidCreate      (sidbuilder *builder, sid2_model_t model,
                              sid2_model_t defaultModel);
    void      reset          ();
    uint8_t   iomap          (const uint_least16_t addr);
    sid2_model_t getModel(const int sidModel,
                          sid2_model_t userModel,
                          sid2_model_t defaultModel);

    uint8_t readMemByte_io   (const uint_least16_t addr);
    void    writeMemByte_io  (const uint_least16_t addr, const uint8_t data);


    uint8_t m_readMemByte    (const uint_least16_t);
    void    m_writeMemByte   (const uint_least16_t, const uint8_t);

    uint16_t getChecksum(const uint8_t* rom, const int size);

    // Environment Function entry Points
    uint8_t cpuRead  (const uint_least16_t addr) { return m_readMemByte (addr); }
    void    cpuWrite (const uint_least16_t addr, const uint8_t data) { m_writeMemByte (addr, data); }

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
    inline void interruptNMI (const bool state);
    inline void interruptRST (void);
    void signalAEC (const bool state) { cpu.setRDY (state); }
    void lightpen  () { vic.lightpen (); }

    // PSID driver
    int  psidDrvReloc   (SidTuneInfo &tuneInfo, sid2_info_t &info);
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

    SidTuneInfo *getTuneInfo() { return m_tune ? &m_tuneInfo : 0; }

    EventContext *getEventScheduler() {return &m_scheduler; }
};

/**
* CPU IRQ line control. IRQ is asserted if any source asserts IRQ.
* (Maintains a counter of calls with state=true vs. state=false.)
*/
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

/**
* CPU NMI line control. NMI is asserted if any source asserts NMI.
* (Maintains a counter of calls with state=true vs. state=false.)
*/
void Player::interruptNMI (const bool state)
{
    if (state)
    {
        cpu.triggerNMI ();
    }
    else
    {
        cpu.clearNMI ();
    }
}

void Player::interruptRST ()
{
    stop ();
}

SIDPLAY2_NAMESPACE_STOP

#endif // _player_h_
