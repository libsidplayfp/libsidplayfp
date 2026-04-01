/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2026 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef SYSTEMROMBANKS_H
#define SYSTEMROMBANKS_H

#include <algorithm>
#include <iterator>
#include <cstdint>
#include <cstring>

#include "Bank.h"
#include "c64/CPU/opcodes.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

/**
 * ROM bank base class.
 */
template <int N>
class romBank : public Bank
{
    static_assert((N > 0) && ((N & (N - 1)) == 0), "N must be a power of two");

protected:
    /// The ROM array
    uint8_t rom[N];

protected:
    /**
     * Set value at memory address.
     */
    inline void setVal(uint_least16_t address, uint8_t val) { rom[address & (N-1)] = val; }
    inline void setVal16(uint_least16_t address, uint_least16_t val)
    {
        endian_little16(getPtr(address), val);
    }

    /**
     * Return value from memory address.
     */
    inline uint8_t getVal(uint_least16_t address) { return rom[address & (N-1)]; }
    inline uint_least16_t getVal16(uint_least16_t address)
    {
        return endian_little16(getPtr(address));
    }

    /**
     * Return pointer to memory address.
     */
    inline uint8_t* getPtr(uint_least16_t address) { return &rom[address & (N-1)]; }

public:
    /**
     * Copy content from source buffer.
     */
    void set(const uint8_t* source) { if (source != nullptr) std::memcpy(rom, source, N); }

    /**
     * Writing to ROM is a no-op.
     */
    void poke(uint_least16_t, uint8_t) override {}

    /**
     * Read from ROM.
     */
    uint8_t peek(uint_least16_t address) override { return rom[address & (N-1)]; }
};

/**
 * Kernal ROM
 *
 * Located at $E000-$FFFF
 */
class KernalRomBank final : public romBank<0x2000>
{
private:
    uint_least16_t resetVector;  // 0xfffc-0xfffd

private:
    uint8_t save_regs[5] =
    {
        PHAn,
        TXAn,
        PHAn,
        TYAn,
        PHAn
    };

    uint8_t restore_regs[5] =
    {
        PLAn,
        TAYn,
        PLAn,
        TAXn,
        PLAn
    };

    // https://sta.c64.org/cbm64krnfunc.html
    uint16_t kernal_functions[78] =
    {
    //  address real address
        0xFF81, 0xFF5B, // SCINIT
        0xFF84, 0xFDA3, // IOINIT
        0xFF87, 0xFD50, // RAMTAS
        0xFF8A, 0xFD15, // RESTOR
        0xFF8D, 0xFD1A, // VECTOR
        0xFF90, 0xFE18, // SETMSG
        0xFF93, 0xEDB9, // LSTNSA
        0xFF96, 0xEDC7, // TALKSA
        0xFF99, 0xFE25, // MEMTOP
        0xFF9C, 0xFE34, // MEMBOT
        0xFF9F, 0xEA87, // SCNKEY
        0xFFA2, 0xFE21, // SETTMO
        0xFFA5, 0xEE13, // IECIN.
        0xFFA8, 0xEDDD, // IECOUT
        0xFFAB, 0xEDEF, // UNTALK
        0xFFAE, 0xEDFE, // UNLSTN
        0xFFB1, 0xED0C, // LISTEN
        0xFFB4, 0xED09, // TALK
        0xFFB7, 0xFE07, // READST
        0xFFBA, 0xFE00, // SETLFS
        0xFFBD, 0xFDF9, // SETNAM
        0xFFC0, 0xF34A, // OPEN
        0xFFC3, 0xF291, // CLOSE
        0xFFC6, 0xF20E, // CHKIN
        0xFFC9, 0xF250, // CHKOUT
        0xFFCC, 0xF333, // CLRCHN
        0xFFCF, 0xF157, // CHRIN
        0xFFD2, 0xF1CA, // CHROUT
        0xFFD5, 0xF49E, // LOAD
        0xFFD8, 0xF5DD, // SAVE
        0xFFDB, 0xF6E4, // SETTIM
        0xFFDE, 0xF6DD, // RDTIM
        0xFFE1, 0xF6ED, // STOP
        0xFFE4, 0xF13E, // GETIN
        0xFFE7, 0xF32F, // CLALL
        0xFFEA, 0xF69B, // UDTIM
        0xFFED, 0xE505, // SCREEN
        0xFFF0, 0xE50A, // PLOT
        0xFFF3, 0xE500, // IOBASE
    };

    void fill(uint_least16_t address, uint8_t data[5])
    {
        std::memcpy(getPtr(address), data, 5);
    }

public:
    void set(const uint8_t* kernal)
    {
        romBank<0x2000>::set(kernal);

        if (kernal == nullptr)
        {
            std::fill(std::begin(rom), std::end(rom), NOPn);

            // IRQ routine
            setVal(0xea31, JMPw);
            setVal16(0xea32, 0xea7e);

            setVal(0xea7e, NOPa);  // Clear IRQ
            setVal16(0xea7f, 0xdc0d);
            fill(0xea81, restore_regs);
            setVal(0xea86, RTIn); // Return from interrupt

            // Reset
            setVal(0xfce2, 0x02); // Halt

            // NMI entry point
            setVal(0xfe43, SEIn);
            setVal(0xfe44, JMPi); // Jump to NMI routine (Default: $FE47)
            setVal16(0xfe45, 0x0318);

            // NMI routine
            fill(0xfe47, save_regs);

            fill(0xfebc, restore_regs);
            setVal(0xfec1, RTIn);

            // IRQ entry point
            fill(0xff48, save_regs);
            setVal(0xff4d, JMPi); // Jump to IRQ routine (Default: $EA31)
            setVal16(0xff4e, 0x0314);

            // Hardware vectors
            setVal16(0xfffa, 0xfe43); // NMI vector
            setVal16(0xfffc, 0xfce2); // RESET vector
            setVal16(0xfffe, 0xff48); // IRQ/BRK vector

            // Standard KERNAL functions called by some unclean rips
            for (auto addr: kernal_functions)
                setVal(addr, RTSn);
        }

        // Backup Reset Vector
        resetVector = getVal16(0xfffc);
    }

    void reset()
    {
        // Restore original Reset Vector
        setVal16(0xfffc, resetVector);
    }

    /**
     * Change the RESET vector.
     *
     * @param addr the new addres to point to
     */
    void installResetHook(uint_least16_t addr)
    {
        setVal16(0xfffc, addr);
    }
};

/**
 * BASIC ROM
 *
 * Located at $A000-$BFFF
 */
class BasicRomBank final : public romBank<0x2000>
{
private:
    uint8_t trap[3];
    uint8_t subTune[11];

public:
    void set(const uint8_t* basic)
    {
        romBank<0x2000>::set(basic);

        // Backup BASIC Warm Start
        std::memcpy(trap, getPtr(0xa7ae), sizeof(trap));

        std::memcpy(subTune, getPtr(0xbf53), sizeof(subTune));
    }

    void reset()
    {
        // Restore original BASIC Warm Start
        std::memcpy(getPtr(0xa7ae), trap, sizeof(trap));

        std::memcpy(getPtr(0xbf53), subTune, sizeof(subTune));
    }

    /**
     * Set BASIC Warm Start address.
     *
     * @param addr
     */
    void installTrap(uint_least16_t addr)
    {
        setVal(0xa7ae, JMPw);
        setVal16(0xa7af, addr);
    }

    void setSubtune(uint8_t tune)
    {
        setVal(0xbf53, LDAb);
        setVal(0xbf54, tune);
        setVal(0xbf55, STAa);
        setVal16(0xbf56, 0x030c);
        setVal(0xbf58, JSRw);
        setVal16(0xbf59, 0xa82c);
        setVal(0xbf5b, JMPw);
        setVal16(0xbf5c, 0xa7b1);
    }
};

/**
 * Character ROM
 *
 * Located at $D000-$DFFF
 */
class CharacterRomBank final : public romBank<0x1000> {};

}

#endif
