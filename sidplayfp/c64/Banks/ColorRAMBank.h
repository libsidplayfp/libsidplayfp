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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef COLORRAMBANK_H
#define COLORRAMBANK_H

#include "Bank.h"

#include <stdint.h>
#include <string.h>

class ColorRAMBank : public Bank
{
private:
    uint8_t ram[0x400];

public:
    void reset()
    {
         memset(ram, 0, 0x400);
    }

    void write(const uint_least16_t address, const uint8_t value)
    {
        ram[address & 0x3ff] = value & 0xf;
    }

    uint8_t read(const uint_least16_t address)
    {
        return ram[address & 0x3ff];
    }
};

#endif
