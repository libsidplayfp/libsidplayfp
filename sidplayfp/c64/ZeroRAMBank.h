/*
 *  Copyright (C) 2012 Leandro Nini
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef ZERORAMBANK_H
#define ZERORAMBANK_H

#include "Bank.h"

#include <string.h>
#include <stdint.h>

class PLA
{
public:
    virtual uint8_t getDirRead() const =0;
    virtual uint8_t getDataRead() const =0;
    virtual void setData(const uint8_t value) =0;
    virtual void setDir(const uint8_t value) =0;
};

/**
 * Area backed by RAM, including cpu port addresses 0 and 1.
*
* This is bit of a fake. We know that the CPU port is an internal
* detail of the CPU, and therefore CPU should simply pay the price
* for reading/writing to 0/1.
*
* However, that would slow down all accesses, which is suboptimal. Therefore
* we install this little hook to the 4k 0 region to deal with this.
*
 * @author Antti Lankila
 */
class ZeroRAMBank : public Bank
{
private:
    PLA* pla;

    /** C64 RAM area */
    Bank* ramBank;

public:
    ZeroRAMBank(PLA* pla, Bank* ramBank) :
        pla(pla),
        ramBank(ramBank) {}

    void reset()
    {
    }

    uint8_t read(const uint_least16_t address)
    {
        // Bank Select Register Value DOES NOT get to ram
        switch (address)
        {
        case 0:
            return pla->getDirRead();
        case 1:
            return pla->getDataRead();
        default:
            return ramBank->read(address);
        }
    }

    void write(const uint_least16_t address, const uint8_t value)
    {
        switch (address)
        {
        case 0:
            pla->setDir(value);
            break;
        case 1:
            pla->setData(value);
            break;
        default:
            ramBank->write(address, value);
        }
    }
};

#endif
