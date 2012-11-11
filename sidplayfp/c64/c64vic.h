/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

#ifndef C64VIC_H
#define C64VIC_H

// The VIC emulation is very generic and here we need to effectively
// wire it into the computer (like adding a chip to a PCB).

#include "Banks/Bank.h"
#include "sidplayfp/c64/c64env.h"
#include "sidplayfp/sidendian.h"
#include "VIC_II/mos656x.h"

/** @internal
* VIC-II
* located at $D000-$D3FF
*/
class c64vic: public MOS656X, public Bank
{
private:
    c64env &m_env;

protected:
    void write(uint_least16_t address, uint8_t value)
    {
        MOS656X::write(endian_16lo8(address), value);
    }

    uint8_t read(uint_least16_t address)
    {
        return MOS656X::read(endian_16lo8(address));
    }

    void interrupt (bool state)
    {
        m_env.interruptIRQ (state);
    }

    void setBA (bool state)
    {
        m_env.setBA (state);
    }

public:
    c64vic (c64env *env)
    :MOS656X(&(env->context ())),
     m_env(*env) {}

    const char *error (void) const {return "";}
};

#endif // C64VIC_H
