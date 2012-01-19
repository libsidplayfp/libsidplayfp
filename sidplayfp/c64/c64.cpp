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

#include "c64.h"
#include "sidplayfp/sidbuilder.h"

SIDPLAY2_NAMESPACE_START

const float64_t c64::CLOCK_FREQ_NTSC = 1022727.14;
const float64_t c64::CLOCK_FREQ_PAL  = 985248.4;

const float64_t c64::VIC_FREQ_PAL    = 50.0;
const float64_t c64::VIC_FREQ_NTSC   = 60.0;

c64::c64()
:c64env  (&m_scheduler),
 cpu     (this),
 cia1    (this),
 cia2    (this),
 vic     (this),
 m_cpuFreq(CLOCK_FREQ_PAL)
{
    // SID Initialise
    for (int i = 0; i < MAX_SIDS; i++)
        sid[i] = 0;

    // Setup sid mapping table
    for (int i = 0; i < MAPPER_SIZE; i++)
        sidmapper[i] = 0;
}

uint8_t c64::readMemByte_io (const uint_least16_t addr)
{
    switch (endian_16hi8 (addr))
    {
        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xd3:
            return vic.read(addr & 0x3f);
        case 0xd4:
        case 0xd5:
        case 0xd6:
        case 0xd7:
        case 0xd8:
        case 0xd9:
        case 0xda:
        case 0xdb:
        {
            // Read real sid for these
            const int i = sidmapper[(addr >> 5) & (MAPPER_SIZE - 1)];
            return sid[i]->read(addr & 0x1f);
        }
        case 0xdc:
            return cia1.read(addr & 0x0f);
        case 0xdd:
            return cia2.read(addr & 0x0f);
        case 0xde:
        case 0xdf:
            return mmu.readRomByte(addr);
        }
}

uint8_t c64::m_readMemByte (const uint_least16_t addr)
{
    if (((addr >> 12) == 0xd) && mmu.isIoArea())
        return readMemByte_io(addr);

    return mmu.cpuRead (addr);
}

void c64::m_writeMemByte (const uint_least16_t addr, const uint8_t data)
{
    if (((addr >> 12) == 0xd) && mmu.isIoArea())
        writeMemByte_io (addr, data);
    else
        mmu.cpuWrite (addr, data);
}

void c64::writeMemByte_io (const uint_least16_t addr, const uint8_t data)
{
    switch (endian_16hi8 (addr))
    {
        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xd3:
            vic.write(addr & 0x3f, data);
            break;
        case 0xd4:
        case 0xd5:
        case 0xd6:
        case 0xd7:
        case 0xd8:
        case 0xd9:
        case 0xda:
        case 0xdb:
        {
            const int i = sidmapper[(addr >> 5) & (MAPPER_SIZE - 1)];
            // Convert address to that acceptable by resid
            sid[i]->write(addr & 0x1f, data);
        }
            break;
        case 0xdc:
            cia1.write(addr & 0x0f, data);
            break;
        case 0xdd:
            cia2.write(addr & 0x0f, data);
            break;
        case 0xde:
        case 0xdf:
            mmu.cpuWrite (addr, data);
            break;
    }
}

void c64::reset()
{
    m_scheduler.reset();

    //cpu.reset  ();
    cia1.reset ();
    cia2.reset ();
    vic.reset  ();

    for (int i = 0; i < MAX_SIDS; i++)
    {
        if (sid[i])
            sid[i]->reset (0x0f);
    }

    mmu.reset();

    irqCount = 0;
    oldBAState = true;
}

void c64::setMainCpuSpeed(const double cpuFreq)
{
    m_cpuFreq = cpuFreq;
    if (m_cpuFreq == CLOCK_FREQ_PAL)
    {
        const float64_t clockPAL = m_cpuFreq / VIC_FREQ_PAL;
        cia1.clock (clockPAL);
        cia2.clock (clockPAL);
        vic.chip   (MOS6569);
    }
    else
    {
        const float64_t clockNTSC = m_cpuFreq / VIC_FREQ_NTSC;
        cia1.clock (clockNTSC);
        cia2.clock (clockNTSC);
        vic.chip   (MOS6567R8);
    }
}

void c64::resetSIDMapper()
{
    for (int i = 0; i < MAPPER_SIZE; i++)
        sidmapper[i] = 0;
}

void c64::setSecondSIDAddress(const int sidChipBase2)
{
    sidmapper[sidChipBase2 >> 5 & (MAPPER_SIZE - 1)] = 1;
}

SIDPLAY2_NAMESPACE_STOP
