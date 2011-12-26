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


#ifndef MMU_H
#define MMU_H

#include "sidplayfp/sid2types.h"
#include "sidplayfp/sidendian.h"
#include "sidplayfp/sidconfig.h"

#include <string.h>

SIDPLAY2_NAMESPACE_START

class MMU
{
private:
	const uint8_t* kernalRom;
	const uint8_t* basicRom;
	const uint8_t* characterRom;

	bool kernal;
	bool basic;
	bool ioArea;
	bool character;

	/* Value written to processor port.  */
	uint8_t dir;
	uint8_t data;

	/* Value read from processor port.  */
	uint8_t dir_read;
	uint8_t data_read;

	/* State of processor port pins.  */
	uint8_t data_out;

	// TODO some wired stuff with data_set_bit6 and data_set_bit7

	uint8_t m_rom[65536];
	uint8_t m_ram[65536];

private:
	void mem_pla_config_changed();
	void c64pla_config_changed(const bool tape_sense, const bool caps_sense, const uint8_t pullup);

	uint8_t getDirRead() const { return dir_read; }
	uint8_t getDataRead() const { return data_read; }

public:
	MMU();
	~MMU () {}

	void reset();

	void setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character) {
		kernalRom=kernal;
		basicRom=basic;
		characterRom=character;
	}

	void setData(const uint8_t value) {
		if (data != value) {
			data = value;
			mem_pla_config_changed();
		}
	}

	void setDir(const uint8_t value) {
		if (dir != value) {
			dir = value;
			mem_pla_config_changed();
		}
	}

	bool isKernal() const { return kernal; }
	bool isBasic() const { return basic; }
	bool isIoArea() const { return ioArea; }
	bool isCharacter() const { return character; }

	// RAM access methods
	uint8_t* getMem() { return m_ram; }

	uint8_t readMemByte(const uint_least16_t addr) const { return m_ram[addr]; }
	uint_least16_t readMemWord(const uint_least16_t addr) const { return endian_little16(&m_ram[addr]); }

	void writeMemByte(const uint_least16_t addr, const uint8_t value) { m_ram[addr] = value; }
	void writeMemWord(const uint_least16_t addr, const uint_least16_t value) { endian_little16(&m_ram[addr], value); }

	void fillRam(const uint_least16_t start, const uint8_t value, const int size) {
		memset(m_ram+start, value, size);
	}
	void fillRam(const uint_least16_t start, const uint8_t* source, const int size) {
		memcpy(m_ram+start, source, size);
	}

	// ROM access methods
	uint8_t* getRom() { return m_rom; }

	uint8_t readRomByte(const uint_least16_t addr) const { return m_rom[addr]; }

	void writeRomByte(const uint_least16_t addr, const uint8_t value) { m_rom[addr] = value; }
	void writeRomWord(const uint_least16_t addr, const uint_least16_t value) { endian_little16(&m_rom[addr], value); }

	void fillRom(const uint_least16_t start, const uint8_t* source, const int size) {
		memcpy(m_rom+start, source, size);
	}

	//
	uint8_t cpuRead(const uint_least16_t addr);
	void cpuWrite(const uint_least16_t addr, const uint8_t data);
};

SIDPLAY2_NAMESPACE_STOP

#endif
