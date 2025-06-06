/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2023 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "c64/CIA/mos652x.h"
#include "c64/VIC_II/mos656x.h"

namespace libsidplayfp
{

using model_data_t = struct
{
    double colorBurst;         ///< Colorburst frequency in Herz
    double divider;            ///< Clock frequency divider
    double powerFreq;          ///< Power line frequency in Herz
    MOS656X::model_t vicModel; ///< Video chip model
};

using cia_model_data_t = struct
{
    MOS652X::model_t ciaModel; ///< CIA chip model
};

/*
 * Color burst frequencies:
 *
 * NTSC  - 3.579545455 MHz = 315/88 MHz
 * PAL-B - 4.43361875 MHz = 283.75 * 15625 Hz + 25 Hz.
 * PAL-M - 3.57561149 MHz
 * PAL-N - 3.58205625 MHz
 */

const model_data_t modelData[] =
{
    {4433618.75,  18., 50., MOS656X::model_t::MOS6569},      // PAL-B
    {3579545.455, 14., 60., MOS656X::model_t::MOS6567R8},    // NTSC-M
    {3579545.455, 14., 60., MOS656X::model_t::MOS6567R56A},  // Old NTSC-M
    {3582056.25,  14., 50., MOS656X::model_t::MOS6572},      // PAL-N
    {3575611.49,  14., 50., MOS656X::model_t::MOS6573},      // PAL-M
};

const cia_model_data_t ciaModelData[] =
{
    {MOS652X::model_t::MOS6526},      // Old
    {MOS652X::model_t::MOS8521},      // New
    {MOS652X::model_t::MOS6526W4485}, // Old week 4485
};

double c64::getCpuFreq(model_t model)
{
    // The crystal clock that drives the VIC II chip is four times
    // the color burst frequency
    const double crystalFreq = modelData[model].colorBurst * 4.;

    // The VIC II produces the two-phase system clock
    // by running the input clock through a divider
    return crystalFreq / modelData[model].divider;
}

c64::c64() :
    c64env(eventScheduler),
    cpuFrequency(getCpuFreq(PAL_B)),
    cia1(*this),
    cia2(*this),
    vic(*this),
    disconnectedBusBank(mmu),
    mmu(eventScheduler, &ioBank),
    cpubus(mmu),
    cpu(eventScheduler, cpubus)
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
    eventScheduler.reset();

    //cpu.reset();
    cia1.reset();
    cia2.reset();
    vic.reset();
    sidBank.reset();
    colorRAMBank.reset();
    mmu.reset();

    for (auto sidBank: extraSidBanks)
        sidBank.second->reset();

    irqCount = 0;
    oldBAState = true;
}

void c64::setModel(model_t model)
{
    cpuFrequency = getCpuFreq(model);
    vic.chip(modelData[model].vicModel);

    const unsigned int rate = cpuFrequency / modelData[model].powerFreq;
    cia1.setDayOfTimeRate(rate);
    cia2.setDayOfTimeRate(rate);
}

void c64::setCiaModel(cia_model_t model)
{
    cia1.setModel(ciaModelData[model].ciaModel);
    cia2.setModel(ciaModelData[model].ciaModel);
}

void c64::setBaseSid(c64sid *s)
{
    sidBank.setSID(s);
}

bool c64::addExtraSid(c64sid *s, int address)
{
    // Check for valid address in the IO area range ($dxxx)
    if ((address & 0xf000) != 0xd000) UNLIKELY
        return false;

    const int idx = (address >> 8) & 0xf;

    // Only allow second SID chip in SID area ($d400-$d7ff)
    // or IO Area ($de00-$dfff)
    if ((idx < 0x4) || ((idx > 0x7) && (idx < 0xe))) UNLIKELY
        return false;

    // Add new SID bank
    sidBankMap_t::iterator it = extraSidBanks.find(idx);
    if (it != extraSidBanks.end())
    {
         ExtraSidBank *extraSidBank = it->second;
         extraSidBank->addSID(s, address);
    }
    else
    {
        ExtraSidBank *extraSidBank = extraSidBanks.insert(it, sidBankMap_t::value_type(idx, new ExtraSidBank()))->second;
        extraSidBank->resetSIDMapper(ioBank.getBank(idx));
        ioBank.setBank(idx, extraSidBank);
        extraSidBank->addSID(s, address);
    }

    return true;
}

void c64::deleteSids(sidBankMap_t &extraSidBanks)
{
    for (auto sidBank: extraSidBanks)
        delete sidBank.second;

    extraSidBanks.clear();
}

c64::~c64()
{
    deleteSids(extraSidBanks);
}

void c64::clearSids()
{
    sidBank.setSID(nullptr);

    resetIoBank();

    deleteSids(extraSidBanks);
}

}
