/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2013 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef SIDBANK_H
#define SIDBANK_H

#include "Bank.h"
#include "sidplayfp/sidemu.h"

/**
* SID
* located at $D400-$D7FF, mirrored each 32 bytes
*/
class SidBank : public Bank
{
private:
    /** SID chip */
    sidemu *sid;

public:
    SidBank()
      : sid(0)
    {}

    void reset()
    {
        if (sid)
            sid->reset(0xf);
    }

    uint8_t peek(uint_least16_t addr)
    {
        return sid ? sid->peek(addr) : 0xff;
    }

    void poke(uint_least16_t addr, uint8_t data)
    {
        if (sid)
            sid->poke(addr, data);
    }

    /**
    * Set SID emulation.
    *
    * @param s the emulation
    */
    void setSID(sidemu *s) { sid = s; }

    /**
    * Get SID emulation.
    *
    * @ratuen the emulation
    */
    sidemu *getSID() const { return sid; }
};

#endif
