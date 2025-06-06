/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2023 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include <stdint.h>
#include <cstdio>

#include <map>

#include "Banks/IOBank.h"
#include "Banks/ColorRAMBank.h"
#include "Banks/DisconnectedBusBank.h"
#include "Banks/SidBank.h"
#include "Banks/ExtraSidBank.h"

#include "EventScheduler.h"

#include "c64/c64env.h"
#include "c64/c64cpu.h"
#include "c64/c64cia.h"
#include "c64/c64vic.h"
#include "c64/mmu.h"

#include "sidcxx11.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

namespace libsidplayfp
{

class c64sid;
class sidmemory;

/**
 * Commodore 64 emulation core.
 *
 * It consists of the following chips:
 * - CPU 6510
 * - VIC-II 6567/6569/6572/6573
 * - CIA 6526/8521
 * - SID 6581/8580
 * - PLA 7700/82S100
 * - Color RAM 2114
 * - System RAM 4164-20/50464-150
 * - Character ROM 2332
 * - Basic ROM 2364
 * - Kernal ROM 2364
 */
class c64 final : private c64env
{
public:
    using model_t = enum
    {
        PAL_B = 0     ///< PAL C64
        ,NTSC_M       ///< NTSC C64
        ,OLD_NTSC_M   ///< Old NTSC C64
        ,PAL_N        ///< C64 Drean
        ,PAL_M        ///< C64 Brasil
    };

    using cia_model_t = enum
    {
        OLD = 0     ///< Old CIA
        ,NEW        ///< New CIA
        ,OLD_4485   ///< Old CIA, special batch labeled 4485
    };

private:
    using sidBankMap_t = std::map<int, ExtraSidBank*>;

private:
    /// System clock frequency
    double cpuFrequency;

    /// Number of sources asserting IRQ
    int irqCount;

    /// BA state
    bool oldBAState;

    /// System event context
    EventScheduler eventScheduler;

    /// CIA1
    c64cia1 cia1;

    /// CIA2
    c64cia2 cia2;

    /// VIC II
    c64vic vic;

    /// Color RAM
    ColorRAMBank colorRAMBank;

    /// SID
    SidBank sidBank;

    /// Extra SIDs
    sidBankMap_t extraSidBanks;

    /// I/O Area #1 and #2
    DisconnectedBusBank disconnectedBusBank;

    /// I/O Area
    IOBank ioBank;

    /// MMU chip
    MMU mmu;

    /// CPUBus
    c64cpubus cpubus;

    /// CPU
    MOS6510 cpu;

private:
    static double getCpuFreq(model_t model);

    static void deleteSids(sidBankMap_t &extraSidBanks);

private:
    /**
     * IRQ trigger signal.
     *
     * Calls permitted any time, but normally originated by chips at PHI1.
     *
     * @param state
     */
    inline void interruptIRQ(bool state) override;

    /**
     * NMI trigger signal.
     *
     * Calls permitted any time, but normally originated by chips at PHI1.
     */
    inline void interruptNMI() override { cpu.triggerNMI(); }

    /**
     * Reset signal.
     */
    inline void interruptRST() override { cpu.triggerRST(); }

    /**
     * BA signal.
     *
     * Calls permitted during PHI1.
     *
     * @param state
     */
    inline void setBA(bool state) override;

    /**
     * @param state fire pressed, active low
     */
    inline void lightpen(bool state) override;

    void resetIoBank();

public:
    c64();
    ~c64();

    /**
     * Get C64's event scheduler
     *
     * @return the scheduler
     */
    EventScheduler *getEventScheduler() { return &eventScheduler; }

    uint_least32_t getTimeMs() const
    {
        return static_cast<uint_least32_t>((eventScheduler.getTime(EVENT_CLOCK_PHI1) * 1000) / cpuFrequency);
    }

    /**
     * Clock the emulation.
     *
     * @throws haltInstruction
     */
    void clock() { eventScheduler.clock(); }

    void debug(bool enable, FILE *out) { cpu.debug(enable, out); }

    void reset();
    void resetCpu() { cpu.reset(); }

    /**
     * Set the c64 model.
     */
    void setModel(model_t model);

    /**
     * Set the cia model.
     */
    void setCiaModel(cia_model_t model);

    /**
     * Get the CPU clock speed.
     *
     * @return the speed in Hertz
     */
    double getMainCpuSpeed() const { return cpuFrequency; }

    /**
     * Set the base SID.
     *
     * @param s the sid emu to set
     */
    void setBaseSid(c64sid *s);

    /**
     * Add an extra SID.
     *
     * @param s the sid emu to set
     * @param sidAddress
     *            base address (e.g. 0xd420)
     *
     * @return false if address is unsupported
     */
    bool addExtraSid(c64sid *s, int address);

    /**
     * Remove all the SIDs.
     */
    void clearSids();

    /**
     * Get the components credits
     */
    //@{
    const char* cpuCredits() const { return cpu.credits(); }
    const char* ciaCredits() const { return cia1.credits(); }
    const char* vicCredits() const { return vic.credits(); }
    //@}

    sidmemory& getMemInterface() { return mmu; }

    uint_least16_t getCia1TimerA() const { return cia1.getTimerA(); }
};

void c64::interruptIRQ(bool state)
{
    if (state)
    {
        if (irqCount == 0)
            cpu.triggerIRQ();

        irqCount ++;
    }
    else
    {
        irqCount --;
        if (irqCount == 0)
             cpu.clearIRQ();
    }
}

void c64::setBA(bool state)
{
    // only react to changes in state
    if (state == oldBAState)
        return;

    oldBAState = state;

    // Signal changes in BA to interested parties
    cpu.setRDY(state);
}

void c64::lightpen(bool state)
{
    if (!state)
        vic.triggerLightpen();
    else
        vic.clearLightpen();
}

}

#endif // C64_H
