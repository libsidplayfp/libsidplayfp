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

#include <stdint.h>

/** @internal
* Interface to PLA functions.
*/
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
* Implementation based on VICE code.
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
     * indicates if the unused bits of the data port are in the process of falling off
     */
    //@{
    bool dataFalloffBit6;
    bool dataFalloffBit7;
    //@}

    /** Value of the unused bit of the processor port.  */
    //@{
    uint8_t dataSetBit6;
    uint8_t dataSetBit7;
    //@}

    /** Value written to processor port.  */
    //@{
    uint8_t dir;
    uint8_t data;
    //@}

    /** Value read from processor port.  */
    uint8_t dataRead;

    /** State of processor port pins.  */
    uint8_t procPortPins;

    /** Tape motor status.  */
    uint8_t tapeMotor;

    /** Tape write line status.  */
    uint8_t tapeDataOutput;

private:
    void updateCpuPort()
    {
        // Update data pins for which direction is OUTPUT
        procPortPins = (procPortPins & ~dir) | (data & dir);

        dataRead = (data | ~dir) & (procPortPins | 0x17);

        pla->setCpuPort((data | ~dir) & 0x07);

        if ((dir & 0x20) == 0)
        {
            dataRead &= ~0x20;
        }
        if (tape_sense && (dir & 0x10) == 0)
        {
            dataRead &= ~0x10;
        }

        if (((dir & data) & 0x20) != tapeMotor)
        {
            tapeMotor = dir & data & 0x20;
            //C64.setMotor(tapeMotor == 0);
        }

        if (((~dir | data) & 0x08) != tapeDataOutput)
        {
            tapeDataOutput = (~dir | data) & 0x08;
            //C64.toggleWriteBit(tapeDataOutput != 0);
        }
    }

private:    // prevent copying
    ZeroRAMBank(const ZeroRAMBank&);
    ZeroRAMBank& operator=(const ZeroRAMBank&);

public:
    ZeroRAMBank(PLA* pla, Bank* ramBank) :
        pla(pla),
        ramBank(ramBank) {}

    void reset()
    {
        dataFalloffBit6 = false;
        dataFalloffBit7 = false;
        dir = 0;
        data = 0x3f;
        dataRead = 0x3f;
        procPortPins = 0x3f;
        tapeMotor = 0xff;
        tapeDataOutput = 0xff;
        updateCpuPort();
    }

    /* $00/$01 unused bits emulation, as investigated by groepaz:
    
       it actually seems to work like this... somewhat unexpected indeed
    
       a) unused bits of $00 (DDR) are actually implemented and working. any value
          written to them can be read back and also affects $01 (DATA) the same as
          with the used bits.
       b) unused bits of $01 are also implemented and working. when a bit is
          programmed as output, any value written to it can be read back. when a bit
          is programmed as input it will read back as 0, if 1 is written to it then
          it will read back as 1 for some time and drop back to 0 (the already
          emulated case of when bitfading occurs)
    
       educated guess on why this happens: on the CPU actually the full 8 bit of
       the i/o port are implemented. what is missing for bit 6 and bit 7 are the
       actual input/output driver stages only - which (imho) completely explains
       the above described behavior :)
    */

    uint8_t peek(uint_least16_t address)
    {
        switch (address)
        {
        case 0:
            return dir;
        case 1:
            if (dataFalloffBit6 || dataFalloffBit7)
            {
                const event_clock_t phi2time = pla->getPhi2Time();

                if (dataFalloffBit6 && dataSetClkBit6 < phi2time)
                {
                    dataFalloffBit6 = false;
                    dataSetBit6 = 0;
                }

                if (dataFalloffBit7 && dataSetClkBit7 < phi2time)
                {
                    dataFalloffBit7 = false;
                    dataSetBit7 = 0;
                }
            }

            return (dataRead & 0x3f) | dataSetBit6 | dataSetBit7;
        default:
            return ramBank->peek(address);
        }
    }

    void poke(uint_least16_t address, uint8_t value)
    {
        switch (address)
        {
        case 0:
            // check if bit 6 has flipped
            if ((dir ^ value) & 0x40)
            {
                if (value & 0x40)
                {
                    /* output, update according to last write, cancel falloff */
                    dataSetBit6 = data & 0x40;
                    dataFalloffBit6 = false;
                }
                else
                {
                    /* input, start falloff if bit was set */
                    dataFalloffBit6 = dataSetBit6 != 0;
                    dataSetClkBit6 = pla->getPhi2Time() + C64_CPU_DATA_PORT_FALL_OFF_CYCLES;
                }
            }

            // check if bit 7 has flipped
            if ((dir ^ value) & 0x80)
            {
                if (value & 0x80)
                {
                    /* output, update according to last write, cancel falloff */
                    dataSetBit7 = data & 0x80;
                    dataFalloffBit7 = false;
                }
                else
                {
                    /* input, start falloff if bit was set */
                    dataFalloffBit7 = dataSetBit7 != 0;
                    dataSetClkBit7 = pla->getPhi2Time() + C64_CPU_DATA_PORT_FALL_OFF_CYCLES;
                }
            }

            if (dir != value)
            {
                dir = value;
                updateCpuPort();
            }
            value = pla->getLastReadByte();
            break;
        case 1:
             /* update value if output, otherwise don't touch */
            if (dir & 0x40)
            {
                dataSetBit6 = value & 0x40;
            }

             /* update value if output, otherwise don't touch */
            if (dir & 0x80)
            {
                dataSetBit7 = value & 0x80;
            }

            if (data != value)
            {
                data = value;
                updateCpuPort();
            }
            value = pla->getLastReadByte();
            break;
        default:
            break;
        }

        ramBank->poke(address, value);
    }
};

#endif
