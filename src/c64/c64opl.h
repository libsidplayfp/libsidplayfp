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
#include <cmath>
#include <cstdint>

namespace libsidplayfp
{

/**
 * OPL interface.
 */
class c64opl : public Bank
{
private:
    //uint8_t lastpoke[0x80];

    OPL *m_opl;

    /// The sample buffer
    short *m_buffer = nullptr;

    static constexpr uint32_t OPL_CLOCK = 3579545;

public:
    c64opl(uint32_t rate)
    {
        m_opl = OPL_new(OPL_CLOCK, rate);
        OPL_setChipType(m_opl, 2); // 1:YM3526, 2:YM3812
        // 20ms buffer
        const int buffersize = std::ceil((rate / 1000.f) * 20.f);
        m_buffer = new short[buffersize];
    }

    ~c64opl()
    {
        OPL_delete(m_opl);
        delete [] m_buffer;
    }

    void reset()
    {
        //std::fill(std::begin(lastpoke), std::end(lastpoke), 0);
        OPL_reset(m_opl);
    }

    // Bank functions
    void poke(uint_least16_t address, uint8_t value) override
    {
        //lastpoke[address & 0x7f] = value;
        if ((address & 0x7f) == 0x40)
            OPL_writeIO(m_opl, 0, value); // register select
        if ((address & 0x7f) == 0x50)
            OPL_writeIO(m_opl, 1, value); // register write
    }

    uint8_t peek(uint_least16_t address) override
    {
        if ((address & 0x7f) == 0x60)
            return OPL_readIO(m_opl); // register read
        return 0;
    }

    //void getStatus(uint8_t regs[0x80]) const { std::memcpy(regs, lastpoke, 0x80); }
};

}

#endif // C64SID_H
