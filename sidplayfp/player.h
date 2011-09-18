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
/***************************************************************************
 *  $Log: player.h,v $
 *  Revision 1.52  2004/06/26 11:02:55  s_a_white
 *  Changes to support new calling convention for event scheduler.
 *  Removed unnecessary cpu emulation.
 *
 *  Revision 1.51  2004/05/03 22:42:56  s_a_white
 *  Change how port handling is dealt with to provide better C64 compatiiblity.
 *  Add character rom support.
 *
 *  Revision 1.50  2004/04/13 07:40:47  s_a_white
 *  Add lightpen support.
 *
 *  Revision 1.49  2004/03/18 20:15:15  s_a_white
 *  Added sidmapper (so support more the 2 sids).
 *
 *  Revision 1.48  2004/01/31 17:01:44  s_a_white
 *  Add ability to specify the maximum number of sid writes forming the sid2crc.
 *
 *  Revision 1.47  2003/12/15 23:50:37  s_a_white
 *  Fixup use of inline functions and correct mileage calculations.
 *
 *  Revision 1.46  2003/10/16 07:42:49  s_a_white
 *  Allow redirection of debug information to file.
 *
 *  Revision 1.45  2003/06/27 21:15:25  s_a_white
 *  Tidy up mono to stereo sid conversion, only allowing it theres sufficient
 *  support from the emulation.  Allow user to override whether they want this
 *  to happen.
 *
 *  Revision 1.44  2003/05/28 20:11:19  s_a_white
 *  Support the psiddrv overlapping unused parts of the tunes load image.
 *
 *  Revision 1.43  2003/02/20 19:11:48  s_a_white
 *  sid2crc support.
 *
 *  Revision 1.42  2003/01/23 17:32:39  s_a_white
 *  Redundent code removal.
 *
 *  Revision 1.41  2003/01/20 18:37:08  s_a_white
 *  Stealing update.  Apparently the cpu does a memory read from any non
 *  write cycle (whether it needs to or not) resulting in those cycles
 *  being stolen.
 *
 *  Revision 1.40  2003/01/17 08:35:46  s_a_white
 *  Event scheduler phase support.
 *
 *  Revision 1.39  2002/12/03 23:25:53  s_a_white
 *  Prevent PSID digis from playing in real C64 mode.
 *
 *  Revision 1.38  2002/11/27 00:16:51  s_a_white
 *  Make sure driver info gets reset and exported properly.
 *
 *  Revision 1.37  2002/11/20 21:44:34  s_a_white
 *  Initial support for external DMA to steal cycles away from the CPU.
 *
 *  Revision 1.36  2002/11/19 22:55:50  s_a_white
 *  PSIDv2NG/RSID changes to deal with spec updates for recommended
 *  implementation.
 *
 *  Revision 1.35  2002/11/01 17:36:01  s_a_white
 *  Frame based support for old sidplay1 modes.
 *
 *  Revision 1.34  2002/10/02 19:45:23  s_a_white
 *  RSID support.
 *
 *  Revision 1.33  2002/09/12 21:01:31  s_a_white
 *  Added support for simulating the random delay before the user loads a
 *  program on a real C64.
 *
 *  Revision 1.32  2002/09/09 18:01:30  s_a_white
 *  Prevent m_info driver details getting modified when C64 crashes.
 *
 *  Revision 1.31  2002/07/21 19:39:41  s_a_white
 *  Proper error handling of reloc info overlapping load image.
 *
 *  Revision 1.30  2002/07/20 08:34:52  s_a_white
 *  Remove unnecessary and pointless conts.
 *
 *  Revision 1.29  2002/07/17 21:48:10  s_a_white
 *  PSIDv2NG reloc exclude region extension.
 *
 *  Revision 1.28  2002/04/14 21:46:50  s_a_white
 *  PlaySID reads fixed to come from RAM only.
 *
 *  Revision 1.27  2002/03/12 18:43:59  s_a_white
 *  Tidy up handling of envReset on illegal CPU instructions.
 *
 *  Revision 1.26  2002/03/03 22:01:58  s_a_white
 *  New clock speed & sid model interface.
 *
 *  Revision 1.25  2002/01/29 21:50:33  s_a_white
 *  Auto switching to a better emulation mode.  m_tuneInfo reloaded after a
 *  config.  Initial code added to support more than two sids.
 *
 *  Revision 1.24  2002/01/28 19:32:01  s_a_white
 *  PSID sample improvements.
 *
 *  Revision 1.23  2001/12/13 08:28:08  s_a_white
 *  Added namespace support to fix problems with xsidplay.
 *
 *  Revision 1.22  2001/10/18 22:34:04  s_a_white
 *  GCC3 fixes.
 *
 *  Revision 1.21  2001/10/02 18:26:36  s_a_white
 *  Removed ReSID support and updated for new scheduler.
 *
 *  Revision 1.20  2001/09/20 19:32:39  s_a_white
 *  Support for a null sid emulation to use when a builder create call fails.
 *
 *  Revision 1.19  2001/09/17 19:02:38  s_a_white
 *  Now uses fixed point maths for sample output and rtc.
 *
 *  Revision 1.18  2001/09/01 11:15:46  s_a_white
 *  Fixes sidplay1 environment modes.
 *
 *  Revision 1.17  2001/08/10 21:01:06  s_a_white
 *  Fixed RTC initialisation order warning.
 *
 *  Revision 1.16  2001/08/10 20:03:19  s_a_white
 *  Added RTC reset.
 *
 *  Revision 1.15  2001/07/25 17:01:13  s_a_white
 *  Support for new configuration interface.
 *
 *  Revision 1.14  2001/07/14 16:46:16  s_a_white
 *  Sync with sidbuilder class project.
 *
 *  Revision 1.13  2001/07/14 12:50:58  s_a_white
 *  Support for credits and debuging.  External filter selection removed.  RTC
 *  and samples obtained in a more efficient way.  Support for component
 *  and sidbuilder classes.
 *
 *  Revision 1.12  2001/04/23 17:09:56  s_a_white
 *  Fixed video speed selection using unforced/forced and NTSC clockSpeeds.
 *
 *  Revision 1.11  2001/03/22 22:45:20  s_a_white
 *  Re-ordered initialisations to match defintions.
 *
 *  Revision 1.10  2001/03/21 23:28:12  s_a_white
 *  Support new component names.
 *
 *  Revision 1.9  2001/03/21 22:32:55  s_a_white
 *  Filter redefinition support.  VIC & NMI support added.
 *
 *  Revision 1.8  2001/03/08 22:48:33  s_a_white
 *  Sid reset on player destruction removed.  Now handled locally by the sids.
 *
 *  Revision 1.7  2001/03/01 23:46:37  s_a_white
 *  Support for sample mode to be selected at runtime.
 *
 *  Revision 1.6  2001/02/28 18:52:55  s_a_white
 *  Removed initBank* related stuff.
 *
 *  Revision 1.5  2001/02/21 21:41:51  s_a_white
 *  Added seperate ram bank to hold C64 player.
 *
 *  Revision 1.4  2001/02/07 20:56:46  s_a_white
 *  Samples now delayed until end of simulated frame.
 *
 *  Revision 1.3  2001/01/23 21:26:28  s_a_white
 *  Only way to load a tune now is by passing in a sidtune object.  This is
 *  required for songlength database support.
 *
 *  Revision 1.2  2001/01/07 15:58:37  s_a_white
 *  SID2_LIB_API now becomes a core define (SID_API).
 *
 *  Revision 1.1  2000/12/12 19:15:40  s_a_white
 *  Renamed from sidplayer
 *
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

#include "mos6510/mos6510.h"
#include "sid6526/sid6526.h"
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

class MMU
{
private:
    bool kernal;
    bool basic;
    bool ioArea;
    bool character;

    /* Value written to processor port.  */
    uint8_t dir;
    uint8_t data;

    /* Value read from processor port.  */
    uint8_t dir_read;
    uint8_t data_read;

    /* State of processor port pins.  */
    uint8_t data_out;

    // TODO some wired stuff with data_set_bit6 and data_set_bit7

private:
    void mem_pla_config_changed();
    void c64pla_config_changed(const bool tape_sense, const bool caps_sense, const uint8_t pullup);

public:
    void reset()
    {
        data = 0x3f;
        data_out = 0x3f;
        data_read = 0x3f;
        dir = 0;
        dir_read = 0;
    }

    void setData(const uint8_t value)
    {
        if (data != value)
        {
            data = value;
            mem_pla_config_changed();
        }
    }

    void setDir(const uint8_t value)
    {
        if (dir != value)
        {
            dir = value;
            mem_pla_config_changed();
        }
    }

    bool isKernal() const { return kernal; }
    bool isBasic() const { return basic; }
    bool isIoArea() const { return ioArea; }
    bool isCharacter() const { return character; }

    uint8_t getDirRead() const { return dir_read; }
    uint8_t getDataRead() const { return data_read; }
};


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
    static const char  *credit[10]; // 10 credits max

    static const char  ERR_PSIDDRV_NO_SPACE[];
    static const char  ERR_PSIDDRV_RELOC[];

    EventScheduler m_scheduler;

    //SID6510  cpu(6510, "Main CPU");
    SID6510 cpu;
    // Sid objects to use.
    NullSID nullsid;
    c64xsid xsid;
    c64cia1 cia;
    c64cia2 cia2;
    SID6526 sid6526;
    c64vic  vic;
    sidemu *sid[SID2_MAX_SIDS];
    int     m_sidmapper[32]; // Mapping table in d4xx-d7xx

    EventCallback<Player> m_mixerEvent;

    // User Configuration Settings
    SidTuneInfo   m_tuneInfo;
    SidTune      *m_tune;
    uint8_t      *m_ram, *m_rom;
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
    uint_least32_t  m_sid2crc;
    uint_least32_t  m_sid2crcCount;

    float64_t m_cpuFreq;

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
    MMU m_port;

    uint8_t m_playBank;

private:
    float64_t clockSpeed     (sid2_clock_t clock, sid2_clock_t defaultClock,
                              const bool forced);
    int       environment    (sid2_env_t env);
    void      fakeIRQ        (void);
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
    uint8_t readMemByte_sidplaytp (const uint_least16_t addr);
    uint8_t readMemByte_sidplaybs (const uint_least16_t addr);
    void    writeMemByte_plain    (const uint_least16_t addr, const uint8_t data);
    void    writeMemByte_playsid  (const uint_least16_t addr, const uint8_t data);
    void    writeMemByte_sidplay  (const uint_least16_t addr, const uint8_t data);

    // Use pointers to please requirements of all the provided
    // environments.
    uint8_t (Player::*m_readMemByte)    (const uint_least16_t);
    void    (Player::*m_writeMemByte)   (const uint_least16_t, const uint8_t);
    uint8_t (Player::*m_readMemDataByte)(const uint_least16_t);

    uint8_t  readMemRamByte (const uint_least16_t addr)
    {   return m_ram[addr]; }
    void sid2crc (const uint8_t data);

    // Environment Function entry Points
    void           envReset           (const bool safe);
    void           envReset           (void) { envReset (true); }
    inline uint8_t envReadRomByte     (const uint_least16_t addr);
    inline uint8_t envReadMemByte     (const uint_least16_t addr);
    inline void    envWriteMemByte    (const uint_least16_t addr, const uint8_t data);
    bool           envCheckBankJump   (const uint_least16_t addr);
    inline uint8_t envReadMemDataByte (const uint_least16_t addr);
    inline void    envSleep           (void);

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
    ~Player ();

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
};


uint8_t Player::envReadRomByte (const uint_least16_t addr)
{
    return m_rom[addr];
}

uint8_t Player::envReadMemByte (const uint_least16_t addr)
{   // Read from plain only to prevent execution of rom code
    return (this->*(m_readMemByte)) (addr);
}

void Player::envWriteMemByte (const uint_least16_t addr, uint8_t data)
{   // Writes must be passed to env version.
    (this->*(m_writeMemByte)) (addr, data);
}

uint8_t Player::envReadMemDataByte (const uint_least16_t addr)
{   // Read from plain only to prevent execution of rom code
    return (this->*(m_readMemDataByte)) (addr);
}

void Player::envSleep (void)
{
    if (m_info.environment != sid2_envR)
    {   // Start the sample sequence
        xsid.suppress (false);
        xsid.suppress (true);
    }
}

void Player::interruptIRQ (const bool state)
{
    if (state)
    {
        if (m_info.environment == sid2_envR)
            cpu.triggerIRQ ();
        else
            fakeIRQ ();
    }
    else
        cpu.clearIRQ ();
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
