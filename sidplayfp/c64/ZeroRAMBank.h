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

#ifndef ZERORAMBANK_H
#define ZERORAMBANK_H

#include "Bank.h"
#include "sidplayfp/event.h"

#include <string.h>
#include <stdint.h>

class PLA
{
public:
    virtual void setCpuPort(const int state) =0;
    virtual uint8_t getLastReadByte() const =0;
    virtual event_clock_t getPhi2Time() const =0;
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
    /** $01 bits 6 and 7 fall-off cycles (1->0), average is about 350 msec */
    static const long C64_CPU_DATA_PORT_FALL_OFF_CYCLES = 350000;

    static const bool tape_sense = false;

private:
    PLA* pla;

    /** C64 RAM area */
    Bank* ramBank;

    /** cycle that should invalidate the unused bits of the data port. */
    //@{
    long data_set_clk_bit6;
    long data_set_clk_bit7;
    //@}

    /** indicates if the unused bits of the data port are still
    valid or should be read as 0, 1 = unused bits valid,
    0 = unused bits should be 0 */
    //@{
    bool data_set_bit6;
    bool data_set_bit7;
    //@}

    /** Value written to processor port.  */
    //@{
    uint8_t dir;
    uint8_t data;
    //@}

    /** Value read from processor port.  */
    //@{
    uint8_t dir_read;
    uint8_t data_read;
    //@}

    /** State of processor port pins.  */
    uint8_t data_out;

    /** indicated if the unused bits are in the process of falling off. */
    uint8_t data_falloff_bit6;
    uint8_t data_falloff_bit7;

    /** Tape motor status.  */
    uint8_t old_port_data_out;

    /** Tape write line status.  */
    uint8_t old_port_write_bit;

private:
    void updateCpuPort()
    {
        data_out = (data_out & ~dir) | (data & dir);
        data_read = (data | ~dir) & (data_out | 0x17);
        pla->setCpuPort(data_read);

        if ((dir & 0x20) == 0)
        {
            data_read &= 0xdf;
        }
        if ((dir & 0x10) == 0 && tape_sense)
        {
            data_read &= 0xef;
        }

        if ((dir & data & 0x20) != old_port_data_out)
        {
            old_port_data_out = dir & data & 0x20;
            //C64.this.setMotor(0 == old_port_data_out);
        }

        if (((~dir | data) & 0x8) != old_port_write_bit)
        {
            old_port_write_bit = (~dir | data) & 0x8;
            //C64.this.toggleWriteBit(((~dir | data) & 0x8) != 0);
        }
    }

public:
    ZeroRAMBank(PLA* pla, Bank* ramBank) :
        pla(pla),
        ramBank(ramBank) {}

    void reset()
    {
        old_port_data_out = 0xff;
        old_port_write_bit = 0xff;
        data = 0x3f;
        data_out = 0x3f;
        data_read = 0x3f;
        dir = 0;
        data_set_bit6 = false;
        data_set_bit7 = false;
        data_falloff_bit6 = 0;
        data_falloff_bit7 = 0;
        updateCpuPort();
    }

    uint8_t read(const uint_least16_t address)
    {
        if (address == 0)
        {
            return dir;
        }
        else if (address == 1)
        {
            if (data_falloff_bit6 != 0 || data_falloff_bit7 != 0)
            {
                const event_clock_t phi2time = pla->getPhi2Time();

                if (data_set_clk_bit6 < phi2time)
                {
                    data_falloff_bit6 = 0;
                    data_set_bit6 = false;
                }

                if (data_set_clk_bit7 < phi2time)
                {
                    data_falloff_bit7 = 0;
                    data_set_bit7 = false;
                }
            }
            return data_read & 0xff - (((!data_set_bit6?1:0)<<6)+((!data_set_bit7?1:0)<<7));
        }
        else
        {
            return ramBank->read(address);
        }
    }

    void write(const uint_least16_t address, const uint8_t value)
    {
        uint8_t v;
        if (address == 0)
        {
            if (data_set_bit7 && (value & 0x80) == 0 && data_falloff_bit7 == 0)
            {
                data_falloff_bit7 = 1;
                data_set_clk_bit7 = pla->getPhi2Time() + C64_CPU_DATA_PORT_FALL_OFF_CYCLES;
            }
            if (data_set_bit6 && (value & 0x40) == 0 && data_falloff_bit6 == 0)
            {
                data_falloff_bit6 = 1;
                data_set_clk_bit6 = pla->getPhi2Time() + C64_CPU_DATA_PORT_FALL_OFF_CYCLES;
            }
            if (data_set_bit7 && (value & 0x80) != 0 && data_falloff_bit7 != 0)
            {
                data_falloff_bit7 = 0;
            }
            if (data_set_bit6 && (value & 0x40) != 0 && data_falloff_bit6 != 0)
            {
                data_falloff_bit6 = 0;
            }
            dir = value;
            updateCpuPort();
            v = pla->getLastReadByte();
        }
        else if (address == 1)
        {
            if ((dir & 0x80) != 0 && (value & 0x80) != 0)
            {
                data_set_bit7 = true;
            }
            if ((dir & 0x40) != 0 && (value & 0x40) != 0)
            {
                data_set_bit6 = true;
            }
            data = value;
            updateCpuPort();
            v = pla->getLastReadByte();
        }
        else
            v = value;

        ramBank->write(address, v);
    }
};

#endif
