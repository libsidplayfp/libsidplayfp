/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
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

#ifndef C64_H
#define C64_H

#include "Banks/Bank.h"
#include "Banks/IOBank.h"
#include "Banks/ColorRAMBank.h"
#include "Banks/DisconnectedBusBank.h"
#include "Banks/SidBank.h"

#include "sidplayfp/c64/c64env.h"
#include "sidplayfp/c64/c64cpu.h"
#include "sidplayfp/c64/c64cia.h"
#include "sidplayfp/c64/c64vic.h"
#include "sidplayfp/c64/mmu.h"


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif


#ifdef PC64_TESTSUITE
class testEnv
{
public:
    virtual void load(const char *) =0;
};
#endif

/** @internal
* Commodore 64 emulation core.
*
* It consists of the following chips: PLA, MOS6510, MOS6526(a), VIC
* 6569(PAL)/6567(NTSC), RAM/ROM.<BR>
*
* @author Antti Lankila
* @author Ken Händel
* @author Leando Nini
*
*/
class c64: private c64env
{
public:
    static const double CLOCK_FREQ_NTSC;
    static const double CLOCK_FREQ_PAL;

private:
    static const double VIC_FREQ_PAL;
    static const double VIC_FREQ_NTSC;

private:
    /** System clock frequency */
    double m_cpuFreq;

    /** Number of sources asserting IRQ */
    int   irqCount;

    /** BA state */
    bool oldBAState;

    /** System event context */
    EventScheduler m_scheduler;

    /** CPU */
    c64cpu  cpu;

    /** CIA1 */
    c64cia1 cia1;

    /** CIA2 */
    c64cia2 cia2;

    /** VIC */
    c64vic  vic;

    /** Color RAM */
    ColorRAMBank colorRAMBank;

    /** SID */
    SidBank sidBank;

    /** I/O Area #1 and #2 */
    DisconnectedBusBank disconnectedBusBank;

    /** I/O Area */
    IOBank ioBank;

    /** MMU chip */
    MMU     mmu;

private:
    /**
    * Access memory as seen by CPU.
    *
    * @param address
    * @return value at address
    */
    uint8_t cpuRead (const uint_least16_t addr) { return mmu.cpuRead(addr); }

    /**
    * Access memory as seen by CPU.
    *
    * @param address
    * @param value
    */
    void cpuWrite (const uint_least16_t addr, const uint8_t data) { mmu.cpuWrite(addr, data); }

    /**
    * IRQ trigger signal.
    *
    * Calls permitted any time, but normally originated by chips at PHI1.
    *
    * @param state
    */
    inline void interruptIRQ (const bool state);

    /**
    * NMI trigger signal.
    *
    * Calls permitted any time, but normally originated by chips at PHI1.
    *
    * @param state
    */
    inline void interruptNMI (void) { cpu.triggerNMI (); }

    inline void interruptRST (void) { /*stop ();*/ }

    /**
    * BA signal.
    *
    * Calls permitted during PHI1.
    *
    * @param state
    */
    inline void setBA (const bool state);

    inline void lightpen () { vic.lightpen (); }

#ifdef PC64_TESTSUITE
    testEnv *m_env;

    void loadFile(const char *file)
    {
        m_env->load(file);
    }
#endif

public:
    c64();
    ~c64() {}

#ifdef PC64_TESTSUITE
    void setTestEnv(testEnv *env)
    {
        m_env = env;
    }
#endif

    /**
    * Get C64's event scheduler
    *
    * @return the scheduler
    */
    EventScheduler *getEventScheduler() { return &m_scheduler; }
    //const EventScheduler &getEventScheduler() const { return m_scheduler; }

    void debug(const bool enable, FILE *out) { cpu.debug (enable, out); }

    void reset();
    void resetCpu() { cpu.reset(); }

    void setMainCpuSpeed(const double cpuFreq);
    double getMainCpuSpeed() const { return m_cpuFreq; }

    /**
    * Set the requested SID
    *
    * @param i sid number to set
    * @param sidemu the sid to set
    */
    void setSid(const unsigned int i, sidemu *s) { sidBank.setSID(i, s); }

    /**
    * Return the requested SID
    *
    * @param i sid number to get
    * @return the SID
    */
    sidemu *getSid(const unsigned int i) const { return sidBank.getSID(i); }

    void resetSIDMapper() { sidBank.resetSIDMapper(); }

    /**
    * Set the base address of a stereo SID chip
    *
    * @param secondSidChipBase
    *            base address (e.g. 0xd420)
    */
    void setSecondSIDAddress(const int sidChipBase2) { sidBank.setSIDMapping(sidChipBase2, 1); }

    /**
    * Get the components credits
    */
    //@{
    const char* cpuCredits () { return cpu.credits(); }
    const char* ciaCredits () { return cia1.credits(); }
    const char* vicCredits () { return vic.credits(); }
    //@}

    MMU *getMmu() { return &mmu; } //FIXME

    uint_least16_t getCia1TimerA() const { return cia1.getTimerA(); }
};

void c64::interruptIRQ (const bool state)
{
    if (state)
    {
        if (irqCount == 0)
            cpu.triggerIRQ ();

        irqCount ++;
    }
    else
    {
        irqCount --;
        if (irqCount == 0)
             cpu.clearIRQ ();
    }
}

void c64::setBA (const bool state)
{
    /* only react to changes in state */
    if ((state ^ oldBAState) == false)
        return;

    oldBAState = state;

    /* Signal changes in BA to interested parties */
    cpu.setRDY (state);
}

#endif // C64_H
