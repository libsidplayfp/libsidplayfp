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
#include <string>
#include <utility>

#include "sidplayfp/sidmd5.h"

/** @internal
*/
class romCheck
{
private:
    typedef std::map<std::string, const char*> md5map;

private:
    /**
     * Maps checksums to respective ROM description.
     * Must be filled by derived class.
     */
    md5map m_checksums;

    /**
     * Pointer to the ROM buffer
     */ 
    const uint8_t* m_rom;

    /**
     * Size of the ROM buffer.
     */
    unsigned int m_size;

private:
    romCheck();

    /**
     * Calculate the md5 digest.
     */
    std::string checksum() const
    {
        sidmd5 md5;
        md5.append (m_rom, m_size);
        md5.finish();

        return md5.getDigest();
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

    void add(const char* md5, const char* desc)
    {
        m_checksums.insert(std::make_pair(md5, desc));
    }

public:
    /**
     * Get ROM description.
     *
     * @return the ROM description or "Unknown Rom".
     */
    const char* info() const
    {
        md5map::const_iterator res = m_checksums.find(checksum());
        return (res!=m_checksums.end())?res->second:"Unknown Rom";
    }
};

/** @internal
* romCheck implementation specific for kernal ROM.
*/
class kernalCheck : public romCheck
{
public:
    kernalCheck(const uint8_t* kernal) :
      romCheck(kernal, 0x2000)
    {
        add("1ae0ea224f2b291dafa2c20b990bb7d4", "C64 KERNAL first revision");
        add("7360b296d64e18b88f6cf52289fd99a1", "C64 KERNAL second revision");
        add("479553fd53346ec84054f0b1c6237397", "C64 KERNAL second revision (Japanese)");
        add("39065497630802346bce17963f13c092", "C64 KERNAL third revision");
        add("27e26dbb267c8ebf1cd47105a6ca71e7", "C64 KERNAL third revision (Swedish)");
        add("187b8c713b51931e070872bd390b472a", "Commodore SX-64 KERNAL");
        add("3abc938cac3d622e1a7041c15b928707", "Commodore SX-64 KERNAL (Swedish)");
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
        add("57af4ae21d4b705c2991d98ed5c1f7b8", "C64 BASIC V2");
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
        add("12a4202f5331d45af846af6c58fba946", "C64 character generator");
        add("cf32a93c0a693ed359a4f483ef6db53d", "C64 character generator (Japanese)");
    }
};

#endif // ROMCHECK_H
