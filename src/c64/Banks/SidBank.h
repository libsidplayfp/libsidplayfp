/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2013 Leandro Nini <drfiemost@users.sourceforge.net>
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
#include "c64/c64sid.h"

#include "sidcxx11.h"

#include "NullSid.h"
#include "xsid/xsid.h"

namespace libsidplayfp
{

/**
 * SID
 *
 * Located at $D400-$D7FF, mirrored each 32 bytes
 */
class SidBank final : public Bank
{
private:
    /// SID chip
    c64sid *sid;
    XSID* playSid = nullptr;

  public:
    SidBank()
      : sid(NullSid::getInstance())
    {}

    void reset()
    {
        sid->reset();
    }

    uint8_t peek(uint_least16_t addr) override
    {
        return sid->peek(addr);
    }

    void poke(uint_least16_t addr, uint8_t data) override
    {
        if (playSid)
        {
            if ((addr & 0x1f) == 0x18)
            {
                playSid->storeSidData0x18(data);
                return;
            }
            playSid->write(static_cast<uint_least16_t>(addr & 0x1ff), data);
            if ((addr & 0x1f) >= 0x1d)
                return;
        }
        sid->poke(addr, data);
    }
    void pokeHardware(uint8_t value)
    {
        sid->poke(0x18, value);
    }
    void setPlaySid(XSID* value)
    {
        playSid = value;
    }

    /**
     * Set SID emulation.
     *
     * @param s the emulation, nullptr to remove current sid
     */
    void setSID(c64sid *s) { sid = (s != nullptr) ? s : NullSid::getInstance(); }
};

}

#endif
