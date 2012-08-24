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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MMU_H
#define MMU_H

#include "sidplayfp/sid2types.h"
#include "sidplayfp/sidendian.h"
#include "sidplayfp/sidconfig.h"

#include "Banks/Bank.h"
#include "Banks/IOBank.h"
#include "Banks/SystemRAMBank.h"
#include "Banks/SystemROMBanks.h"
#include "Banks/ZeroRAMBank.h"

#include <string.h>

/** @internal
 * The C64 MMU chip.
*/
class MMU : public PLA
{
private:
    EventContext &context;

    /** CPU port signals */
    bool loram, hiram, charen;

    /** CPU read memory mapping in 4k chunks */
    Bank* cpuReadMap[16];

    /** CPU write memory mapping in 4k chunks */
    Bank* cpuWriteMap[16];

    /** IO region handler */
    Bank* ioBank;

    /** Kernal ROM */
    KernalRomBank kernalRomBank;

    /** BASIC ROM */
    BasicRomBank basicRomBank;

    /** Character ROM */
    CharacterRomBank characterRomBank;

    /** RAM */
    SystemRAMBank ramBank;

    ZeroRAMBank zeroRAMBank;

private:
    void setCpuPort(const int state);
    void updateMappingPHI2();
    uint8_t getLastReadByte() const { return 0; }
    event_clock_t getPhi2Time() const { return context.getTime(EVENT_CLOCK_PHI2); }

public:
    MMU(EventContext *context, Bank* ioBank);
    ~MMU () {}

    void reset();

    void setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
    {
        kernalRomBank.set(kernal);
        basicRomBank.set(basic);
        characterRomBank.set(character);
    }

    // RAM access methods
    uint8_t* getMem() { return ramBank.array(); }

    uint8_t readMemByte(const uint_least16_t addr) { return ramBank.read(addr); }
    uint_least16_t readMemWord(const uint_least16_t addr) { return endian_little16(ramBank.array()+addr); }

    void writeMemByte(const uint_least16_t addr, const uint8_t value) { ramBank.write(addr, value); }
    void writeMemWord(const uint_least16_t addr, const uint_least16_t value) { endian_little16(ramBank.array()+addr, value); }

    void fillRam(const uint_least16_t start, const uint8_t value, const int size)
    {
        memset(ramBank.array()+start, value, size);
    }
    void fillRam(const uint_least16_t start, const uint8_t* source, const int size)
    {
        memcpy(ramBank.array()+start, source, size);
    }

    // SID specific hacks
    void installResetHook(const uint_least16_t addr) { kernalRomBank.installResetHook(addr); }

    void installBasicTrap(const uint_least16_t addr) { basicRomBank.installTrap(addr); }

    void setBasicSubtune(const uint8_t tune) { basicRomBank.setSubtune(tune); }

    /**
     * Access memory as seen by CPU
     *
     * @param address
     * @return value at address
     */
    uint8_t cpuRead(const uint_least16_t addr) const { return cpuReadMap[addr >> 12]->read(addr); }

    /**
     * Access memory as seen by CPU.
     *
     * @param address
     * @param value
     */
    void cpuWrite(const uint_least16_t addr, const uint8_t data) { cpuWriteMap[addr >> 12]->write(addr, data); }
};

#endif
