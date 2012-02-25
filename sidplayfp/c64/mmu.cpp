/*
 * This file is part of libsidplayfp, a SID player engine.
 * Copyright (C) 2011 Leando Nini <drfiemost@users.sourceforge.net>
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

#include "mmu.h"

SIDPLAY2_NAMESPACE_START

static const uint8_t POWERON[] =
{
#include "poweron.bin"
};

MMU::MMU(EventContext *context, Bank* ioBank)
:context(*context),
 loram(false),
 hiram(false),
 charen(false),
 ioBank(ioBank),
 zeroRAMBank(this, &ramBank)
{
    cpuReadMap[0] = &zeroRAMBank;
    cpuWriteMap[0] = &zeroRAMBank;

    for (int i = 1; i < 16; i++)
    {
        cpuReadMap[i] = &ramBank;
        cpuWriteMap[i] = &ramBank;
    }
}

void MMU::setCpuPort (const int state)
{
    loram = (state & 1) != 0;
    hiram = (state & 2) != 0;
    charen = (state & 4) != 0;
    updateMappingPHI2();
}

void MMU::updateMappingPHI2()
{
    cpuReadMap[0xe] = cpuReadMap[0xf] = hiram ? (Bank*)&kernalRomBank : &ramBank;
    cpuReadMap[0xa] = cpuReadMap[0xb] = (loram && hiram) ? (Bank*)&basicRomBank : &ramBank;

    if (charen && (loram || hiram))
    {
        cpuReadMap[0xd] = cpuWriteMap[0xd] = ioBank;
    }
    else
    {
        cpuReadMap[0xd] = (!charen && (loram || hiram)) ? (Bank*)&characterRomBank : &ramBank;
        cpuWriteMap[0xd] = &ramBank;
    }
}

void MMU::reset()
{
    ramBank.reset();
    kernalRomBank.reset();
    basicRomBank.reset();
    characterRomBank.reset();

    kernalRomBank.write(0xfd69, 0x9f); // Bypass memory check
    kernalRomBank.write(0xe55f, 0x00); // Bypass screen clear
    kernalRomBank.write(0xfdc4, 0xea); // Ingore sid volume reset to avoid DC
    kernalRomBank.write(0xfdc5, 0xea); // click (potentially incompatibility)!!
    kernalRomBank.write(0xfdc6, 0xea);

    updateMappingPHI2();

    // Copy in power on settings.  These were created by running
    // the kernel reset routine and storing the usefull values
    // from $0000-$03ff.  Format is:
    // -offset byte (bit 7 indicates presence rle byte)
    // -rle count byte (bit 7 indicates compression used)
    // data (single byte) or quantity represented by uncompressed count
    // -all counts and offsets are 1 less than they should be
    //if (m_tuneInfo.compatibility >= SIDTUNE_COMPATIBILITY_R64)
    {
        uint_least16_t addr = 0;
        for (int i = 0; i < sizeof (POWERON);)
        {
            uint8_t off   = POWERON[i++];
            uint8_t count = 0;
            bool compressed = false;

            // Determine data count/compression
            if (off & 0x80)
            {   // fixup offset
                off  &= 0x7f;
                count = POWERON[i++];
                if (count & 0x80)
                {   // fixup count
                count &= 0x7f;
                compressed = true;
                }
            }

            // Fix count off by ones (see format details)
            count++;
            addr += off;

            // Extract compressed data
            if (compressed)
            {
                const uint8_t data = POWERON[i++];
                while (count-- > 0)
                    ramBank.write(addr++, data);
            }
            // Extract uncompressed data
            else
            {
                while (count-- > 0)
                    ramBank.write(addr++, POWERON[i++]);
            }
        }
    }
}

SIDPLAY2_NAMESPACE_STOP
