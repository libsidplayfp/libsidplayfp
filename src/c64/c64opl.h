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
#include "libs/emu8950/emu8950.h"

#include "sidcxx11.h"

#include <algorithm>
#include <iterator>
#include <cstring>
#include <cstdint>

namespace libsidplayfp
{

/**
 * OPL interface.
 */
class c64opl : public Bank
{
private:
    //uint8_t lastpoke[0x20];

    OPL *m_opl;

public:
    c64opl()
    {
        m_opl = OPL_new(1, 1); // FIXME
        OPL_setChipType(m_opl, 1); // 1:YM3526, 2:YM3812
    }

    ~c64opl()
    {
        OPL_delete(m_opl);
    }

    void reset()
    {
        //std::fill(std::begin(lastpoke), std::end(lastpoke), 0);
        OPL_reset(m_opl);
    }

    // Bank functions
    void poke(uint_least16_t address, uint8_t value) override
    {
        //lastpoke[address & 0x1f] = value;
        OPL_writeIO(m_opl, 0, address & 0x1f);
        OPL_writeIO(m_opl, 1, value);
    }
    uint8_t peek(uint_least16_t address) override
    {
        OPL_writeIO(m_opl, 0, address & 0x1f);
        return OPL_readIO(m_opl);
    }

    //void getStatus(uint8_t regs[0x20]) const { std::memcpy(regs, lastpoke, 0x20); }
};

}

#endif // C64SID_H
