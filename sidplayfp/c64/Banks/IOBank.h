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

#ifndef IOBANK_H
#define IOBANK_H

#include "Bank.h"

#include <stdint.h>

/** @internal
 * IO region handler. 4k region, 16 chips, 256b banks.
 *
 * @author Antti Lankila
 */
class IOBank : public Bank
{
private:
    Bank* map[16];

public:
    void setBank(const int num, Bank* bank)
    {
        map[num] = bank;
    }

    uint8_t read(const uint_least16_t addr)
    {
        return map[addr >> 8 & 0xf]->read(addr);
    }

    void write(const uint_least16_t addr, const uint8_t data)
    {
        map[addr >> 8 & 0xf]->write(addr, data);
    }
};

#endif
