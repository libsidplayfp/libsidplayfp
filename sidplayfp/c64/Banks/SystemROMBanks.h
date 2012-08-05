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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <stdint.h>

#include "sidplayfp/c64/CPU/opcodes.h"

#if EMBEDDED_ROMS

static const uint8_t KERNAL[] = {
#include "kernal.bin"
};

static const uint8_t BASIC[] = {
#include "basic.bin"
};

static const uint8_t CHARACTER[] = {
#include "char.bin"
};

#else
#  define KERNAL 0
#  define BASIC 0
#  define CHARACTER 0
#endif

/** @internal
 * Kernal ROM
 */
class KernalRomBank : public Bank
{
private:
    const uint8_t* kernalRom;

    uint8_t rom[8192];

public:
    KernalRomBank() :
        kernalRom(KERNAL) {}

    void set(const uint8_t* kernal)
    {
        kernalRom = kernal;
    }

    void reset()
    {
        if (kernalRom)
            memcpy(rom, kernalRom, 8192);

        rom[0xfd69 & 0x1fff] = 0x9f; // Bypass memory check
        rom[0xe55f & 0x1fff] = 0x00; // Bypass screen clear
        rom[0xfdc4 & 0x1fff] = 0xea; // Ignore sid volume reset to avoid DC
        rom[0xfdc5 & 0x1fff] = 0xea; //   click (potentially incompatibility)!!
        rom[0xfdc6 & 0x1fff] = 0xea;
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
    const uint8_t* basicRom;

    uint8_t rom[8192];

public:
    BasicRomBank() :
        basicRom(BASIC) {}

    void set(const uint8_t* basic)
    {
        basicRom = basic;
    }

    void reset()
    {
        if (basicRom)
            memcpy(rom, basicRom, 8192);
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
    const uint8_t* characterRom;

    uint8_t rom[4096];

public:
    CharacterRomBank() :
        characterRom(CHARACTER) {}

    void set(const uint8_t* character)
    {
        characterRom = character;
    }

    void reset()
    {
        if (characterRom)
            memcpy(rom, characterRom, 4096);
    }

    uint8_t read(const uint_least16_t address)
    {
        return rom[address & 0x0fff];
    }

    void write(const uint_least16_t address, const uint8_t value) {}
};

#endif
