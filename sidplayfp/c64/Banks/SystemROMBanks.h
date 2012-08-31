/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2010 Antti Lankila
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

#ifndef SYSTEMROMBANKS_H
#define SYSTEMROMBANKS_H

#include "Bank.h"

#include <string.h>
#include <stdint.h>

#include "sidplayfp/c64/CPU/opcodes.h"

/** @internal
 * Kernal ROM
 */
class KernalRomBank : public Bank
{
private:
    uint8_t rom[8192];

    uint8_t resetVectorLo;  // 0xfffc
    uint8_t resetVectorHi;  // 0xfffd

public:
    void set(const uint8_t* kernal)
    {
        if (kernal)
            memcpy(rom, kernal, 8192);

        // Backup Reset Vector
        resetVectorLo = rom[0xfffc & 0x1fff];
        resetVectorHi = rom[0xfffd & 0x1fff];

        // Apply Kernal hacks
        rom[0xfd69 & 0x1fff] = 0x9f; // Bypass memory check
        rom[0xe55f & 0x1fff] = 0x00; // Bypass screen clear
        rom[0xfdc4 & 0x1fff] = 0xea; // Ignore sid volume reset to avoid DC
        rom[0xfdc5 & 0x1fff] = 0xea; //   click (potentially incompatibility)!!
        rom[0xfdc6 & 0x1fff] = 0xea;
    }

    void reset()
    {
        // Restore original Reset Vector
        rom[0xfffc & 0x1fff] = resetVectorLo;
        rom[0xfffd & 0x1fff] = resetVectorHi;
    }

    uint8_t read(const uint_least16_t address)
    {
        return rom[address & 0x1fff];
    }

    void write(const uint_least16_t address, const uint8_t value) {}

    /**
    * Change the RESET vector
    *
    * @param addr the new addres to point to
    */
    void installResetHook(const uint_least16_t addr)
    {
        rom[0xfffc & 0x1fff] = endian_16lo8(addr);
        rom[0xfffd & 0x1fff] = endian_16hi8(addr);
    }
};

/** @internal
 * BASIC ROM
 */
class BasicRomBank : public Bank
{
private:
    uint8_t rom[8192];

    uint8_t trap[3];
    uint8_t subTune[11];

public:
    BasicRomBank() {}

    void set(const uint8_t* basic)
    {
        if (basic)
            memcpy(rom, basic, 8192);

        // Backup BASIC Warm Start
        memcpy(trap, &rom[0xa7ae & 0x1fff], 3);

        memcpy(subTune, &rom[0xbf53 & 0x1fff], 1);
    }

    void reset()
    {
        // Restore original BASIC Warm Start
        memcpy(&rom[0xa7ae & 0x1fff], trap, 3);

        memcpy(&rom[0xbf53 & 0x1fff], subTune, 11);
    }

    uint8_t read(const uint_least16_t address)
    {
        return rom[address & 0x1fff];
    }

    void write(const uint_least16_t address, const uint8_t value) {}

    /**
    * Set BASIC Warm Start address
    *
    * @param addr
    */
    void installTrap(const uint_least16_t addr)
    {
        rom[0xa7ae & 0x1fff] = JMPw;
        rom[0xa7af & 0x1fff] = endian_16lo8(addr);
        rom[0xa7b0 & 0x1fff] = endian_16hi8(addr);
    }

    void setSubtune(const uint8_t tune)
    {
        rom[0xbf53 & 0x1fff] = LDAb;
        rom[0xbf54 & 0x1fff] = tune;
        rom[0xbf55 & 0x1fff] = STAa;
        rom[0xbf56 & 0x1fff] = 0x0c;
        rom[0xbf57 & 0x1fff] = 0x03;
        rom[0xbf58 & 0x1fff] = JSRw;
        rom[0xbf59 & 0x1fff] = 0x2c;
        rom[0xbf5a & 0x1fff] = 0xa8;
        rom[0xbf5b & 0x1fff] = JMPw;
        rom[0xbf5c & 0x1fff] = 0xb1;
        rom[0xbf5d & 0x1fff] = 0xa7;
    }
};

/** @internal
 * Character ROM
 */
class CharacterRomBank : public Bank
{
private:
    uint8_t rom[4096];

public:
    CharacterRomBank() {}

    void set(const uint8_t* character)
    {
        if (character)
            memcpy(rom, character, 4096);
    }

    void reset() {}

    uint8_t read(const uint_least16_t address)
    {
        return rom[address & 0x0fff];
    }

    void write(const uint_least16_t address, const uint8_t value) {}
};

#endif
