/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2012-2026 Leandro Nini
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

#ifndef C64CPU_H
#define C64CPU_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "c64/mmu.h"
#include "CPU/mos6510.h"

#ifdef VICE_TESTSUITE
#  include <iostream>
#  include <utility>
#  include <cstdlib>
#endif

#define PRINTSCREENCODES

#include "sidcxx11.h"

namespace libsidplayfp
{
#ifdef PRINTSCREENCODES
/**
 * Screen codes conversion table (0x01 = no output)
 */
static const char CHRtab[128] =
{
  0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
  0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x01,0x01,0x01,0x01,
  0x20,0x21,0x01,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
  0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
  0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x24,0x5d,0x20,0x20,
  // alternative: CHR$(92=0x5c) => ISO Latin-1(0xa3)
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c
};
#endif
class c64cpubus final : public CPUDataBus
{
private:
    MMU &m_mmu;

protected:
    uint8_t cpuRead(uint_least16_t addr) override { return m_mmu.cpuRead(addr); }

    void cpuWrite(uint_least16_t addr, uint8_t data) override
    {
#ifdef VICE_TESTSUITE
#  ifdef PRINTSCREENCODES
        if (addr >= 0x0400 && addr <= 0x07ff)
        {
            uint8_t fg_color = m_mmu.readMemByte(addr - 0x0400 + 0xd800) & 0xf;
            uint8_t bg_color = m_mmu.readMemByte(0xd021) & 0xf;

            const char* colodore[16][3] = {
                "0",   "0",   "0",   // black
                "15",  "15",  "15",  // white
                "150", "40",  "46",  // red
                "91",  "214", "206", // cyan
                "159", "45",  "173", // purple
                "65",  "185", "54",  // green
                "39",  "36",  "196", // blue
                "239", "243", "71",  // yellow
                "159", "72",  "21",  // orange
                "94",  "53",  "0",   // brown
                "218", "95",  "102", // light red
                "71",  "71",  "71",  // dark grey
                "120", "120", "120", // grey
                "145", "255", "132", // light green
                "104", "100", "255", // light blue
                "174", "174", "174"  // light grey
            };
            uint8_t chr = data;
            if (chr & 0x80)
            {
                chr >>= 1;
                std::swap(fg_color, bg_color);
            }
            std::cout << "\x1b[38;2;"
                << colodore[fg_color][0] << ';'
                << colodore[fg_color][1] << ';'
                << colodore[fg_color][2] << 'm';
            std::cout << "\x1b[48;2;"
                << colodore[bg_color][0] << ';'
                << colodore[bg_color][1] << ';'
                << colodore[bg_color][2] << 'm';
            std::cout << CHRtab[chr];
        }
#  endif
        // for VICE tests
        if (addr == 0xd7ff)
        {
            if (data == 0)
            {
                std::cout << std::endl << "\x1b[0;32m" << "OK" << std::endl;
                exit(EXIT_SUCCESS);
            }
            else if (data == 0xff)
            {
                std::cout << std::endl << "\x1b[0;31m" << "KO" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
#endif
        m_mmu.cpuWrite(addr, data);
    }

public:
    c64cpubus (MMU &mmu) :
        m_mmu(mmu) {}
};

}

#endif // C64CPU_H
