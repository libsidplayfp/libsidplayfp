/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2026 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef OPLBANK_H
#define OPLBANK_H

#include "Bank.h"
#include "c64/c64opl.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

/**
 * OPL
 *
 * Located at $DF00-$DFFF
 */
class OplBank final : public Bank
{
private:
    /// OPL chip
    c64opl *m_opl;

public:
    OplBank(c64opl *opl)
      : m_opl(opl)
    {}

    void reset()
    {
        m_opl->reset();
    }

    uint8_t peek(uint_least16_t addr) override
    {
        return m_opl->peek(addr);
    }

    void poke(uint_least16_t addr, uint8_t data) override
    {
        m_opl->poke(addr, data);
    }
};

}

#endif
