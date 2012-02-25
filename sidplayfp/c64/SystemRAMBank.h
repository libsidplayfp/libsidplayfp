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

#ifndef SYSTEMRAMBANK_H
#define SYSTEMRAMBANK_H

#include "Bank.h"

#include <string.h>
#include <stdint.h>

/**
 * Area backed by RAM
 *
 * @author Antti Lankila
 */
class SystemRAMBank : public Bank
{
private:
    /** C64 RAM area */
    uint8_t ram[65536];

public:
    void reset()
    {
        // Initialize RAM with powerup pattern
        memset(ram, 0, 65536);
        for (int i = 0x07c0; i < 0x10000; i += 128)
        {
            memset(ram+i, 0xff, 64);
        }
    }

    uint8_t read(const uint_least16_t address)
    {
        return ram[address];
    }

    void write(const uint_least16_t address, const uint8_t value)
    {
        ram[address] = value;
    }

    uint8_t* array()
    {
        return ram;
    }
};

#endif
