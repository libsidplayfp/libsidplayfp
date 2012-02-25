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

/**
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
    }

    uint8_t read(const uint_least16_t address)
    {
        return rom[address & 0x1fff];
    }

    void write(const uint_least16_t address, const uint8_t value)
    {
        rom[address & 0x1fff] = value;
    }
};

/**
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

    void write(const uint_least16_t address, const uint8_t value)
    {
        rom[address & 0x1fff] = value;
    }
};

/**
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
