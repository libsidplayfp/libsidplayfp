/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012 Leando Nini <drfiemost@users.sourceforge.net>
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

#ifndef ROMCHECK_H
#define ROMCHECK_H

#include <stdint.h>

class romCheck
{
protected:
    static const char MSG_UNKNOWN_ROM[];

protected:
    const uint8_t* m_rom;
    unsigned int m_size;
    uint16_t m_checksum;

private:
    romCheck();

    uint16_t checksum() const
    {
        uint16_t checksum = 0;
        for (int i=0; i<m_size; i++)
            checksum += m_rom[i];

        return checksum;
    }

protected:
    romCheck(const uint8_t* rom, int size) :
      m_rom(rom),
      m_size(size),
      m_checksum(checksum()) {}
};

const char romCheck::MSG_UNKNOWN_ROM[] = "Unknown Rom.";

class kernalCheck : public romCheck
{
private:
    // Kernal revision is located at 0xff80
    uint8_t revision() const { return m_rom[0xff80 - 0xe000]; }

public:
    kernalCheck(const uint8_t* kernal) :
      romCheck(kernal, 0x2000) {}

    const char* info() const
    {
        switch (revision())
        {
        case 0x00:
            if (m_checksum == 0xc70b)
            {
                return "C64 KERNAL second revision";
            }
            if (m_checksum == 0xd183)
            {
                return "C64 KERNAL second revision (Japanese).";
            }
            break;
        case 0x03:
            if (m_checksum == 0xc70a)
            {
                return "C64 KERNAL third revision";
            }
            if (m_checksum == 0xc5c9)
            {
                return "C64 KERNAL third revision (Swedish)";
            }
            break;
        case 0xaa:
            if (m_checksum == 0xd4fd)
            {
                return "C64 KERNAL first revision";
            }
            break;
        case 0x43: // Commodore SX-64 + Swedish version
            if (m_checksum == 0xc70b)
            {
                return "Commodore SX-64 KERNAL";
            }
            if (m_checksum == 0xc788)
            {
                return "Commodore SX-64 KERNAL (Swedish)";
            }
            break;
        }

        return MSG_UNKNOWN_ROM;
    }
};

class basicCheck : public romCheck
{
public:
    basicCheck(const uint8_t* basic) : romCheck(basic, 0x2000) {}

    const char* info() const
    {
        if (m_checksum == 0x3d56)
        {
            return "C64 BASIC V2";
        }

        return MSG_UNKNOWN_ROM;
    }
};

class chargenCheck : public romCheck
{
public:
    chargenCheck(const uint8_t* chargen) : romCheck(chargen, 0x1000) {}

    const char* info() const
    {
        if (m_checksum == 0xf7f8)
        {
            return "C64 character generator";
        }
        if (m_checksum == 0xf800)
        {
            return "C64 character generator (Japanese)";
        }

        return MSG_UNKNOWN_ROM;
    }
};

#endif // ROMCHECK_H
