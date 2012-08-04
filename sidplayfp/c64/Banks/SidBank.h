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

#ifndef SIDBANK_H
#define SIDBANK_H

#include "Bank.h"

#include "sidplayfp/sidemu.h"

/** @internal
*
*/
class SidBank : public Bank
{
public:
    /** Maximum number of supported SIDs (mono and stereo) */
    static const int MAX_SIDS = 2;

private:
    /**
    * Size of mapping table. Each 32 bytes another SID chip base address
    * can be assigned to.
    */
    static const int MAPPER_SIZE = 32;

private:
    /** SID chips */
    sidemu *sid[MAX_SIDS];

    /**
    * SID mapping table in d4xx-d7xx.
    * Maps a SID chip base address to a SID
    * chip number.
    */
    int sidmapper[32];

public:
    SidBank()
    {
        for (int i = 0; i < MAX_SIDS; i++)
            sid[i] = 0;

        resetSIDMapper();
    }

    void reset()
    {
        for (int i = 0; i < MAX_SIDS; i++)
        {
            if (sid[i])
                sid[i]->reset(0xf);
        }
    }

    void resetSIDMapper()
    {
        for (int i = 0; i < MAPPER_SIZE; i++)
            sidmapper[i] = 0;
    }

    void setSIDMapping(const int address, const int chipNum)
    {
        sidmapper[address >> 5 & (MAPPER_SIZE - 1)] = chipNum;
    }

    uint8_t read(const uint_least16_t addr)
    {
        const int i = sidmapper[addr >> 5 & (MAPPER_SIZE - 1)];
        return sid[i] ? sid[i]->read(addr & 0x1f) : 0xff;
    }

    void write(const uint_least16_t addr, const uint8_t data)
    {
        const int i = sidmapper[addr >> 5 & (MAPPER_SIZE - 1)];
        if (sid[i])
            sid[i]->write(addr & 0x1f, data);
    }

    void setSID(const unsigned int i, sidemu *s) { sid[i] = s; }

    sidemu *getSID(const unsigned int i) const { return (i < MAX_SIDS)?sid[i]:0; }
};

#endif
