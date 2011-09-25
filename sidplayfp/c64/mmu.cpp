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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "mmu.h"

#include "sidplayfp/mos6510/opcodes.h"

SIDPLAY2_NAMESPACE_START

static const uint8_t KERNAL[] = {
#include "kernal.bin"
};

static const uint8_t CHARACTER[] = {
#include "char.bin"
};

static const uint8_t BASIC[] = {
#include "basic.bin"
};

static const uint8_t POWERON[] = {
#include "poweron.bin"
};

void MMU::mem_pla_config_changed () {

	const uint8_t mem_config = (data | ~dir) & 0x7;
	/*  B I K C
	* 0 . . . .
	* 1 . . . *
	* 2 . . * *
	* 3 * . * *
	* 4 . . . .
	* 5 . * . .
	* 6 . * * .
	* 7 * * * .
	*/
	basic      = ((mem_config & 3) == 3);
	ioArea     = (mem_config >  4);
	kernal     = ((mem_config & 2) != 0);
	character  = ((mem_config ^ 4) > 4);

	c64pla_config_changed(false, true, 0x17);
}

void MMU::c64pla_config_changed(const bool tape_sense, const bool caps_sense, const uint8_t pullup) {

	data_out = (data_out & ~dir) | (data & dir);
	data_read = (data | ~dir) & (data_out | pullup);

	if ((pullup & 0x40) != 0 && !caps_sense) {
		data_read &= 0xbf;
	}
	if ((dir & 0x20) == 0) {
		data_read &= 0xdf;
	}
	if (tape_sense && (dir & 0x10) == 0) {
		data_read &= 0xef;
	}

	dir_read = dir;
}

void MMU::reset(const sid2_env_t env, const bool compatibility) {
	data = 0x3f;
	data_out = 0x3f;
	data_read = 0x3f;
	dir = 0;
	dir_read = 0;
	memset(m_ram, 0, 65536);

	if (env == sid2_envR) {
		// Initialize RAM with powerup pattern
		for (int i=0x07c0; i<0x10000; i+=128)
			memset(m_ram+i, 0xff, 64);

		memset(m_rom, 0, 0x10000);

		memcpy(&m_rom[0xe000], KERNAL, sizeof (KERNAL));
		// ROM should be at 0xd000 but have internally relocated
		// it here to unused ROM space (does not affect C64 progs)
		memcpy(&m_rom[0x4000], CHARACTER, sizeof (CHARACTER));
		m_rom[0xfd69] = 0x9f; // Bypass memory check
		m_rom[0xe55f] = 0x00; // Bypass screen clear
		m_rom[0xfdc4] = 0xea; // Ingore sid volume reset to avoid DC
		m_rom[0xfdc5] = 0xea; // click (potentially incompatibility)!!
		m_rom[0xfdc6] = 0xea;
		if (compatibility)
			memcpy(&m_rom[0xa000], BASIC, sizeof (BASIC));

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
						m_ram[addr++] = data;
				}
				// Extract uncompressed data
				else
				{
					while (count-- > 0)
						m_ram[addr++] = POWERON[i++];
				}
			}
		}
	}
	else // !sid2_envR
	{
		if (env == sid2_envPS)
		{
			m_ram[0xff48] = JMPi;
			endian_little16(&m_ram[0xff49], 0x0314);
		}
		else
		{
			memset(m_rom, 0, 0x10000);
			memset(m_rom + 0xA000, RTSn, 0x2000); // BASIC ROM
		}

		memset(m_rom + 0xE000, RTSn, 0x2000); // KERNAL ROM
		// fake VBI-interrupts that do $D019, BMI ...
		m_rom[0x0d019] = 0xff;

		// Software vectors
		endian_little16(&m_ram[0x0314], 0xEA31); // IRQ
		endian_little16(&m_ram[0x0316], 0xFE66); // BRK
		endian_little16(&m_ram[0x0318], 0xFE47); // NMI

		// Hardware vectors
		endian_little16(&m_rom[0xfffa], (env == sid2_envPS) ? 0xFFFA : 0xFE43); // NMI
		endian_little16(&m_rom[0xfffc], 0xFCE2); // RESET
		endian_little16(&m_rom[0xfffe], 0xFF48); // IRQ
		memcpy(&m_ram[0xfffa], &m_rom[0xfffa], 6);
	}
}

void MMU::setEnv(const sid2_env_t env) {
	if (m_rom != m_ram)
		delete [] m_rom;
	m_rom = (env == sid2_envPS) ? m_ram : new uint8_t[65536];
}

SIDPLAY2_NAMESPACE_STOP
