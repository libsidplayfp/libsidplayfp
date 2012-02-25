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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _c64cia_h_
#define _c64cia_h_

// The CIA emulations are very generic and here we need to effectively
// wire them into the computer (like adding a chip to a PCB).

#include "Bank.h"
#include "sidplayfp/c64/c64env.h"
#include "sidplayfp/sidendian.h"
#include "CIA/mos6526.h"

/* CIA 1 specifics:
   Generates IRQs
*/
class c64cia1: public MOS6526, public Bank
{
private:
    c64env &m_env;
    uint8_t lp;

protected:
    void write(const uint_least16_t address, const uint8_t value)
    {
        MOS6526::write(endian_16lo8(address), value);
    }

    uint8_t read(const uint_least16_t address)
    {
        return MOS6526::read(endian_16lo8(address));
    }

    void interrupt (const bool state)
    {
        m_env.interruptIRQ (state);
    }

    void portB ()
    {
        const uint8_t lp = (prb | ~ddrb) & 0x10;
        if (lp != this->lp)
        {
            m_env.lightpen();
            this->lp = lp;
        }
    }

public:
    c64cia1 (c64env *env)
    :MOS6526(&(env->context ())),
     m_env(*env) {}
    const char *error (void) const {return "";}

    void reset ()
    {
        lp = 0x10;
        MOS6526::reset ();
    }
};

/* CIA 2 specifics:
   Generates NMIs
*/
class c64cia2: public MOS6526, public Bank
{
private:
    c64env &m_env;

protected:
    void write(const uint_least16_t address, const uint8_t value)
    {
        MOS6526::write(address, value);
    }

    uint8_t read(const uint_least16_t address)
    {
        return MOS6526::read(address);
    }

    void interrupt (const bool state)
    {
        if (state)
            m_env.interruptNMI ();
    }

public:
    c64cia2 (c64env *env)
    :MOS6526(&(env->context ())),
     m_env(*env) {}
    const char *error (void) const {return "";}
};

#endif // _c64cia_h_
