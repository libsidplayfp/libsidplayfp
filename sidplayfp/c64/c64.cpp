/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
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

#include "c64.h"

#include "sidplayfp/c64/VIC_II/mos656x.h"

typedef struct
{
    unsigned int crystalFreq;
    unsigned int divider;
    MOS656X::model_t vicModel;
} model_data_t;

const model_data_t modelData[] =
{
    {17734472u, 18u, MOS656X::MOS6569},     // PAL-B
    {14318181u, 14u, MOS656X::MOS6567R8},   // NTSC-M
    {14318181u, 14u, MOS656X::MOS6567R56A}, // Old NTSC-M
    {14328224u, 14u, MOS656X::MOS6572},     // PAL-N
};

double c64::getCpuFreq(model_t model)
{
    return (double)modelData[model].crystalFreq/(double)modelData[model].divider;
}

c64::c64() :
    c64env(&m_scheduler),
    m_cpuFreq(getCpuFreq(PAL_B)),
    cpu(this),
    cia1(this),
    cia2(this),
    vic(this),
    mmu(&m_scheduler, &ioBank)
{
    resetIoBank();
}


void c64::resetIoBank()
{
    ioBank.setBank(0x0, &vic);
    ioBank.setBank(0x1, &vic);
    ioBank.setBank(0x2, &vic);
    ioBank.setBank(0x3, &vic);
    ioBank.setBank(0x4, &sidBank);
    ioBank.setBank(0x5, &sidBank);
    ioBank.setBank(0x6, &sidBank);
    ioBank.setBank(0x7, &sidBank);
    ioBank.setBank(0x8, &colorRAMBank);
    ioBank.setBank(0x9, &colorRAMBank);
    ioBank.setBank(0xa, &colorRAMBank);
    ioBank.setBank(0xb, &colorRAMBank);
    ioBank.setBank(0xc, &cia1);
    ioBank.setBank(0xd, &cia2);
    ioBank.setBank(0xe, &disconnectedBusBank);
    ioBank.setBank(0xf, &disconnectedBusBank);
}

void c64::reset()
{
    m_scheduler.reset();

    //cpu.reset  ();
    cia1.reset();
    cia2.reset();
    vic.reset();
    sidBank.reset();
    colorRAMBank.reset();
    mmu.reset();

    irqCount = 0;
    oldBAState = true;
}

void c64::setModel(model_t model)
{
    m_cpuFreq = getCpuFreq(model);
    vic.chip(modelData[model].vicModel);

    const unsigned int rate = vic.getCyclesPerLine() * vic.getRasterLines();
    cia1.setDayOfTimeRate(rate);
    cia2.setDayOfTimeRate(rate);
}

void c64::setSid(unsigned int i, c64sid *s)
{
    switch (i)
    {
    case 0:
        sidBank.setSID(s);
        break;
    case 1:
        extraSidBank.setSID(s);
        break;
    default:
        break;
    }
}

void c64::setSecondSIDAddress(int sidChipBase2)
{
    resetIoBank();

    // Check for valid address in the IO area range ($dxxx)
    if (sidChipBase2 == 0 || ((sidChipBase2 & 0xf000) != 0xd000))
        return;

    const int idx = (sidChipBase2 >> 8) & 0xf;
    /*
    * Only allow second SID chip in SID area ($d400-$d7ff)
    * or IO Area ($de00-$dfff)
    */
    if (idx < 0x4 || (idx > 0x7 && idx < 0xe))
        return;

    extraSidBank.resetSIDMapper(ioBank.getBank(idx));
    ioBank.setBank(idx, &extraSidBank);
    extraSidBank.setSIDMapping(sidChipBase2);
}
