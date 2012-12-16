/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012 Leando Nini <drfiemost@users.sourceforge.net>
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

#ifndef EXTRASIDBANK_H
#define EXTRASIDBANK_H

#include "Bank.h"
#include "sidplayfp/sidemu.h"

/** @internal
* SID emu wrapper
*/
class sidwrapper : public Bank
{
private:
    sidemu *sid;

public:
    void setSID(sidemu *s) { sid = s; }
    sidemu *getSID() const { return sid; }

    void reset() { if (sid) sid->reset(); }

    void write(uint_least16_t address, uint8_t value) { if (sid) sid->write(address & 0x1f, value); }
    uint8_t read(uint_least16_t address) { return sid ? sid->read(address & 0x1f) : 0xff; }
};

/** @internal
* Extra SID
*/
class ExtraSidBank : public Bank
{
private:
    /**
    * Size of mapping table. Each 32 bytes another SID chip base address
    * can be assigned to.
    */
    static const int MAPPER_SIZE = 8;

private:
    Bank *bank;

    /**
    * SID mapping table.
    * Maps a SID chip base address to a SID
    * chip number.
    */
    Bank *mapper[MAPPER_SIZE];

    sidwrapper sid;

private:
    static unsigned int mapperIndex(int address) { return address >> 5 & (MAPPER_SIZE - 1); }

public:
    void reset()
    {
        sid.reset();
    }

    void resetSIDMapper(Bank *bank)
    {
        for (int i = 0; i < MAPPER_SIZE; i++)
            mapper[i] = bank;
    }

    /**
    * Put a SID at desired location.
    *
    * @param address the address
    */
    void setSIDMapping(int address)
    {
        mapper[mapperIndex(address)] = &sid;
    }

    uint8_t read(uint_least16_t addr)
    {
        return mapper[mapperIndex(addr)]->read(addr);
    }

    void write(uint_least16_t addr, uint8_t data)
    {
        mapper[mapperIndex(addr)]->write(addr, data);
    }

    /**
    * Set SID emulation.
    *
    * @param s the emulation
    */
    void setSID(sidemu *s) { sid.setSID(s); }

    /**
    * Get SID emulation.
    *
    * @ratuen the emulation
    */
    sidemu *getSID() const { return sid.getSID(); }
};

#endif
