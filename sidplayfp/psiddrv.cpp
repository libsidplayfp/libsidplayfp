/***************************************************************************
                          psiddrv.cpp  -  PSID Driver Installtion
                             -------------------
    begin                : Fri Jul 27 2001
    copyright            : (C) 2001 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


// --------------------------------------------------------
// The code here is use to support the PSID Version 2NG
// (proposal B) file format for player relocation support.
// --------------------------------------------------------
#include <string.h>

#include "sidendian.h"
#include "player.h"

#include "c64/CPU/opcodes.h"


SIDPLAY2_NAMESPACE_START

const char Player::ERR_PSIDDRV_NO_SPACE[]  = "ERROR: No space to install psid driver in C64 ram";
const char Player::ERR_PSIDDRV_RELOC[]     = "ERROR: Failed whilst relocating psid driver";

extern "C" int reloc65(unsigned char** buf, int* fsize, int addr);

// Input: A 16-bit effective address
// Output: A default bank-select value for $01.
uint8_t Player::iomap (const uint_least16_t addr)
{
    // Force Real C64 Compatibility
    switch (m_tuneInfo.compatibility)
    {
    case SIDTUNE_COMPATIBILITY_R64:
    case SIDTUNE_COMPATIBILITY_BASIC:
        return 0;     // Special case, converted to 0x37 later
    }

    if (addr == 0)
        return 0;     // Special case, converted to 0x37 later
    if (addr < 0xa000)
        return 0x37;  // Basic-ROM, Kernal-ROM, I/O
    if (addr  < 0xd000)
        return 0x36;  // Kernal-ROM, I/O
    if (addr >= 0xe000)
        return 0x35;  // I/O only

    return 0x34;  // RAM only (special I/O in PlaySID mode)
}

int Player::psidDrvReloc (SidTuneInfo &tuneInfo, sid2_info_t &info)
{
    const int startlp = tuneInfo.loadAddr >> 8;
    const int endlp   = (tuneInfo.loadAddr + (tuneInfo.c64dataLen - 1)) >> 8;

    // Will get done later if can't now
    m_c64.getMmu()->writeMemByte(0x02a6, (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_PAL) ? 1 : 0);

    if (tuneInfo.compatibility == SIDTUNE_COMPATIBILITY_BASIC)
    {   // The psiddrv is only used for initialisation and to
        // autorun basic tunes as running the kernel falls
        // into a manual load/run mode
        tuneInfo.relocStartPage = 0x04;
        tuneInfo.relocPages     = 0x03;
    }

    // Check for free space in tune
    if (tuneInfo.relocStartPage == 0xff)
        tuneInfo.relocPages = 0;
    // Check if we need to find the reloc addr
    else if (tuneInfo.relocStartPage == 0)
    {
        tuneInfo.relocPages = 0;
        /* find area where to dump the driver in.
        * It's only 1 block long, so any free block we can find will do. */
        for (int i = 4; i < 0xd0; i ++)
        {
            if (i >= startlp && i <= endlp)
                continue;

            if (i >= 0xa0 && i <= 0xbf)
                continue;

            tuneInfo.relocStartPage = i;
            tuneInfo.relocPages = 1;
            break;
        }
    }

    if (tuneInfo.relocPages < 1)
    {
        m_errorString = ERR_PSIDDRV_NO_SPACE;
        return -1;
    }

    // Place psid driver into ram
    uint_least16_t relocAddr = tuneInfo.relocStartPage << 8;

    uint8_t psid_driver[] = {
#      include "psiddrv.bin"
    };
    uint8_t *reloc_driver = psid_driver;
    int      reloc_size   = sizeof (psid_driver);

    if (!reloc65 (&reloc_driver, &reloc_size, relocAddr - 10))
    {
        m_errorString = ERR_PSIDDRV_RELOC;
        return -1;
    }

    // Adjust size to not included initialisation data.
    reloc_size -= 10;
    info.driverAddr   = relocAddr;
    info.driverLength = (uint_least16_t) reloc_size;
    // Round length to end of page
    info.driverLength += 0xff;
    info.driverLength &= 0xff00;

    m_c64.getMmu()->installResetHook(endian_little16(reloc_driver));

    // If not a basic tune then the psiddrv must install
    // interrupt hooks and trap programs trying to restart basic
    if (tuneInfo.compatibility == SIDTUNE_COMPATIBILITY_BASIC)
    {   // Install hook to set subtune number for basic
        m_c64.getMmu()->setBasicSubtune((uint8_t) (tuneInfo.currentSong-1));
        m_c64.getMmu()->installBasicTrap(0xbf53);
    }
    else
    {   // Only install irq handle for RSID tunes
        m_c64.getMmu()->fillRam(0x0314, &reloc_driver[2], tuneInfo.compatibility == SIDTUNE_COMPATIBILITY_R64 ? 2 : 6);

        // Experimental restart basic trap
        const uint_least16_t addr = endian_little16(&reloc_driver[8]);
        m_c64.getMmu()->installBasicTrap(0xffe1);
        m_c64.getMmu()->writeMemWord(0x0328, addr);
    }

    int pos = relocAddr;

    // Install driver to ram
    m_c64.getMmu()->fillRam(pos, &reloc_driver[10], reloc_size);

    // Tell C64 about song
    m_c64.getMmu()->writeMemByte(pos, (uint8_t) (tuneInfo.currentSong-1));
    pos++;
    m_c64.getMmu()->writeMemByte(pos, tuneInfo.songSpeed == SIDTUNE_SPEED_VBI ? 0 : 1);
    pos++;
    m_c64.getMmu()->writeMemWord(pos, tuneInfo.compatibility == SIDTUNE_COMPATIBILITY_BASIC ?
                     0xbf55 /*Was 0xa7ae, see above*/ : tuneInfo.initAddr);
    pos += 2;
    m_c64.getMmu()->writeMemWord(pos, tuneInfo.playAddr);
    pos += 2;
    // Initialise random number generator
    info.powerOnDelay = m_cfg.powerOnDelay;
    // Delays above MAX result in random delays
    if (info.powerOnDelay > SID2_MAX_POWER_ON_DELAY)
    {   // Limit the delay to something sensible.
        info.powerOnDelay = (uint_least16_t) (m_rand >> 3) &
                            SID2_MAX_POWER_ON_DELAY;
    }
    m_c64.getMmu()->writeMemWord(pos, info.powerOnDelay);
    pos += 2;
    m_rand  = m_rand * 13 + 1;
    m_c64.getMmu()->writeMemByte(pos, iomap (m_tuneInfo.initAddr));
    pos++;
    m_c64.getMmu()->writeMemByte(pos, iomap (m_tuneInfo.playAddr));
    pos++;
    const uint8_t flag = m_c64.getMmu()->readMemByte(0x02a6); // PAL/NTSC flag
    m_c64.getMmu()->writeMemByte(pos, flag);
    pos++;

    // Add the required tune speed
    switch ((m_tune->getInfo()).clockSpeed)
    {
    case SIDTUNE_CLOCK_PAL:
        m_c64.getMmu()->writeMemByte(pos, 1);
        break;
    case SIDTUNE_CLOCK_NTSC:
        m_c64.getMmu()->writeMemByte(pos, 0);
        break;
    default: // UNKNOWN or ANY
        m_c64.getMmu()->writeMemByte(pos, flag);
        break;
    }
    pos++;

    // Default processor register flags on calling init
    m_c64.getMmu()->writeMemByte(pos, tuneInfo.compatibility >= SIDTUNE_COMPATIBILITY_R64 ? 0 : 1 << MOS6510::SR_INTERRUPT);

    return 0;
}

SIDPLAY2_NAMESPACE_STOP
