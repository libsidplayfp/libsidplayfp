/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2013-2026 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef C64OPL_H
#define C64OPL_H

#include "Banks/Bank.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

/**
 * OPL interface.
 */
class c64opl : public Bank
{
protected:
    virtual ~c64opl() = default;

    virtual uint8_t read(uint_least8_t addr) = 0;
    virtual void write(uint_least8_t addr, uint8_t data) = 0;

public:
    virtual void reset() = 0;

    // Bank functions
    void poke(uint_least16_t address, uint8_t value) override
    {
        write(address & 0x7f, value);
    }

    uint8_t peek(uint_least16_t address) override
    {
        return read(address & 0x7f);
    }
};

}

#endif // C64SID_H
