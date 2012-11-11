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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ZERORAMBANK_H
#define ZERORAMBANK_H

#include "Bank.h"
#include "sidplayfp/event.h"

#include <string.h>
#include <stdint.h>

class PLA
{
public:
    virtual void setCpuPort(int state) =0;
    virtual uint8_t getLastReadByte() const =0;
    virtual event_clock_t getPhi2Time() const =0;
};

/** @internal
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
    /** $01 bits 6 and 7 fall-off cycles (1->0), average is about 350 msec */
    static const event_clock_t C64_CPU_DATA_PORT_FALL_OFF_CYCLES = 350000;

    static const bool tape_sense = false;

private:
    PLA* pla;

    /** C64 RAM area */
    Bank* ramBank;

    /** cycle that should invalidate the unused bits of the data port. */
    //@{
    event_clock_t dataSetClkBit6;
    event_clock_t dataSetClkBit7;
    //@}

    /**
     * indicates if the unused bits of the data port are still
     * valid or should be read as 0, 1 = unused bits valid,
     * 0 = unused bits should be 0
     */
    //@{
    bool dataSetBit6;
    bool dataSetBit7;
    //@}

    /** indicates if the unused bits are in the process of falling off. */
    bool dataFalloffBit6;
    bool dataFalloffBit7;

    /** Value written to processor port.  */
    //@{
    uint8_t dir;
    uint8_t data;
    //@}

    /** Value read from processor port.  */
    uint8_t dataRead;

    /** State of processor port pins.  */
    uint8_t dataOut;

    /** Tape motor status.  */
    uint8_t oldPortDataOut;

    /** Tape write line status.  */
    uint8_t oldPortWriteBit;

private:
    void updateCpuPort()
    {
        dataOut = (dataOut & ~dir) | (data & dir);
        dataRead = (data | ~dir) & (dataOut | 0x17);
        pla->setCpuPort(dataRead);

        if ((dir & 0x20) == 0)
        {
            dataRead &= 0xdf;
        }
        if ((dir & 0x10) == 0 && tape_sense)
        {
            dataRead &= 0xef;
        }

        if ((dir & data & 0x20) != oldPortDataOut)
        {
            oldPortDataOut = dir & data & 0x20;
            //C64.this.setMotor(0 == oldPortDataOut);
        }

        if (((~dir | data) & 0x8) != oldPortWriteBit)
        {
            oldPortWriteBit = (~dir | data) & 0x8;
            //C64.this.toggleWriteBit(((~dir | data) & 0x8) != 0);
        }
    }

public:
    ZeroRAMBank(PLA* pla, Bank* ramBank) :
        pla(pla),
        ramBank(ramBank) {}

    void reset()
    {
        oldPortDataOut = 0xff;
        oldPortWriteBit = 0xff;
        data = 0x3f;
        dataOut = 0x3f;
        dataRead = 0x3f;
        dir = 0;
        dataSetBit6 = false;
        dataSetBit7 = false;
        dataFalloffBit6 = false;
        dataFalloffBit7 = false;
        updateCpuPort();
    }

    uint8_t read(uint_least16_t address)
    {
        if (address == 0)
        {
            return dir;
        }
        else if (address == 1)
        {
            if (dataFalloffBit6 || dataFalloffBit7)
            {
                const event_clock_t phi2time = pla->getPhi2Time();

                if (dataSetClkBit6 < phi2time)
                {
                    dataFalloffBit6 = false;
                    dataSetBit6 = false;
                }

                if (dataSetClkBit7 < phi2time)
                {
                    dataFalloffBit7 = false;
                    dataSetBit7 = false;
                }
            }
            return dataRead & (0xff - (((!dataSetBit6?1:0)<<6) + ((!dataSetBit7?1:0)<<7)));
        }
        else
        {
            return ramBank->read(address);
        }
    }

    void write(uint_least16_t address, uint8_t value)
    {
        if (address == 0)
        {
            if (dataSetBit7 && (value & 0x80) == 0 && !dataFalloffBit7)
            {
                dataFalloffBit7 = true;
                dataSetClkBit7 = pla->getPhi2Time() + C64_CPU_DATA_PORT_FALL_OFF_CYCLES;
            }
            if (dataSetBit6 && (value & 0x40) == 0 && !dataFalloffBit6)
            {
                dataFalloffBit6 = true;
                dataSetClkBit6 = pla->getPhi2Time() + C64_CPU_DATA_PORT_FALL_OFF_CYCLES;
            }
            if (dataSetBit7 && (value & 0x80) != 0 && dataFalloffBit7)
            {
                dataFalloffBit7 = false;
            }
            if (dataSetBit6 && (value & 0x40) != 0 && dataFalloffBit6)
            {
                dataFalloffBit6 = false;
            }
            dir = value;
            updateCpuPort();
            value = pla->getLastReadByte();
        }
        else if (address == 1)
        {
            if ((dir & 0x80) != 0 && (value & 0x80) != 0)
            {
                dataSetBit7 = true;
            }
            if ((dir & 0x40) != 0 && (value & 0x40) != 0)
            {
                dataSetBit6 = true;
            }
            data = value;
            updateCpuPort();
            value = pla->getLastReadByte();
        }

        ramBank->write(address, value);
    }
};

#endif
