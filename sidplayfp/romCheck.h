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
#include <map>

/** @internal
*/
class romCheck
{
protected:
    /**
     * Maps checksums to respective ROM description.
     * Must be filled by derived class.
     */
    std::map<int, const char*> m_checksums;

    /**
     * Pointer to the ROM buffer
     */ 
    const uint8_t* m_rom;

private:
    /**
     * Size of the ROM buffer.
     */
    unsigned int m_size;

private:
    romCheck();

    /**
     * Calculate the checksum.
     * For now it's a simple sum of all the bytes
     * should be changed to an md5 digest.
     */
    uint16_t checksum() const
    {
        uint16_t checksum = 0;
        for (int i=0; i<m_size; i++)
            checksum += m_rom[i];

        return checksum;
    }

protected:
    /**
     * Construct the class.
     *
     * @param rom pointer to the ROM buffer
     * @param size size of the ROM buffer
     */
    romCheck(const uint8_t* rom, int size) :
      m_rom(rom),
      m_size(size) {}

public:
    /**
     * Get ROM description.
     *
     * @return the ROM description or "Unknown Rom".
     */
    const char* info() const
    {
        std::map<int, const char*>::const_iterator res = m_checksums.find(checksum());
        return (res!=m_checksums.end())?res->second:"Unknown Rom";
    }
};

/** @internal
* romCheck implementation specific for kernal ROM.
*/
class kernalCheck : public romCheck
{
private:
    /**
     * Get Kernal revision ID which is located at 0xff80.
     */
    uint8_t revision() const { return m_rom[0xff80 - 0xe000]; }

public:
    kernalCheck(const uint8_t* kernal) :
      romCheck(kernal, 0x2000)
    {
        switch (revision())
        {
        case 0x00:
            m_checksums.insert(std::pair<int, const char*>(0xc70b, "C64 KERNAL second revision"));
            m_checksums.insert(std::pair<int, const char*>(0xd183, "C64 KERNAL second revision (Japanese)"));
            break;
        case 0x03:
            m_checksums.insert(std::pair<int, const char*>(0xc70a, "C64 KERNAL third revision"));
            m_checksums.insert(std::pair<int, const char*>(0xc5c9, "C64 KERNAL third revision (Swedish)"));
            break;
        case 0xaa:
            m_checksums.insert(std::pair<int, const char*>(0xd4fd, "C64 KERNAL first revision"));
            break;
        case 0x43: // Commodore SX-64 + Swedish version
            m_checksums.insert(std::pair<int, const char*>(0xc70b, "Commodore SX-64 KERNAL"));
            m_checksums.insert(std::pair<int, const char*>(0xc788, "Commodore SX-64 KERNAL (Swedish)"));
            break;
        }
    }
};

/** @internal
* romCheck implementation specific for basic ROM.
*/
class basicCheck : public romCheck
{
public:
    basicCheck(const uint8_t* basic) :
      romCheck(basic, 0x2000)
    {
        m_checksums.insert(std::pair<int, const char*>(0x3d56, "C64 BASIC V2"));
    }
};

/** @internal
* romCheck implementation specific for character generator ROM.
*/
class chargenCheck : public romCheck
{
public:
    chargenCheck(const uint8_t* chargen) :
      romCheck(chargen, 0x1000)
    {
        m_checksums.insert(std::pair<int, const char*>(0xf7f8, "C64 character generator"));
        m_checksums.insert(std::pair<int, const char*>(0xf800, "C64 character generator (Japanese)"));
    }
};

#endif // ROMCHECK_H
