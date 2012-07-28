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


SIDPLAYFP_NAMESPACE_START

// Error Strings
const char ERR_PSIDDRV_NO_SPACE[]  = "ERROR: No space to install psid driver in C64 ram";
const char ERR_PSIDDRV_RELOC[]     = "ERROR: Failed whilst relocating psid driver";

extern "C" int reloc65(unsigned char** buf, int* fsize, int addr);

// Input: A 16-bit effective address
// Output: A default bank-select value for $01.
uint8_t Player::iomap (const uint_least16_t addr)
{
    const SidTuneInfo* tuneInfo = m_tune->getInfo();

    // Force Real C64 Compatibility
    switch (tuneInfo->compatibility())
    {
    case SidTuneInfo::COMPATIBILITY_R64:
    case SidTuneInfo::COMPATIBILITY_BASIC:
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

int Player::psidDrvReloc (MMU *mmu)
{
    const SidTuneInfo* tuneInfo = m_tune->getInfo();

    const int startlp = tuneInfo->loadAddr() >> 8;
    const int endlp   = (tuneInfo->loadAddr() + (tuneInfo->c64dataLen() - 1)) >> 8;

    uint_least8_t relocStartPage = tuneInfo->relocStartPage();
    uint_least8_t relocPages = tuneInfo->relocPages();

    // Will get done later if can't now
    mmu->writeMemByte(0x02a6, (tuneInfo->clockSpeed() == SidTuneInfo::CLOCK_PAL) ? 1 : 0);

    if (tuneInfo->compatibility() == SidTuneInfo::COMPATIBILITY_BASIC)
    {   // The psiddrv is only used for initialisation and to
        // autorun basic tunes as running the kernel falls
        // into a manual load/run mode
        relocStartPage = 0x04;
        relocPages     = 0x03;
    }

    // Check for free space in tune
    if (relocStartPage == 0xff)
        relocPages = 0;
    // Check if we need to find the reloc addr
    else if (relocStartPage == 0)
    {
        relocPages = 0;
        /* find area where to dump the driver in.
        * It's only 1 block long, so any free block we can find will do. */
        for (int i = 4; i < 0xd0; i ++)
        {
            if (i >= startlp && i <= endlp)
                continue;

            if (i >= 0xa0 && i <= 0xbf)
                continue;

            relocStartPage = i;
            relocPages = 1;
            break;
        }
    }

    if (relocPages < 1)
    {
        m_errorString = ERR_PSIDDRV_NO_SPACE;
        return -1;
    }

    // Place psid driver into ram
    uint_least16_t relocAddr = relocStartPage << 8;

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
    m_info.driverAddr   = relocAddr;
    m_info.driverLength = (uint_least16_t) reloc_size;
    // Round length to end of page
    m_info.driverLength += 0xff;
    m_info.driverLength &= 0xff00;

    mmu->installResetHook(endian_little16(reloc_driver));

    // If not a basic tune then the psiddrv must install
    // interrupt hooks and trap programs trying to restart basic
    if (tuneInfo->compatibility() == SidTuneInfo::COMPATIBILITY_BASIC)
    {   // Install hook to set subtune number for basic
        mmu->setBasicSubtune((uint8_t) (tuneInfo->currentSong() - 1));
        mmu->installBasicTrap(0xbf53);
    }
    else
    {   // Only install irq handle for RSID tunes
        mmu->fillRam(0x0314, &reloc_driver[2], tuneInfo->compatibility() == SidTuneInfo::COMPATIBILITY_R64 ? 2 : 6);

        // Experimental restart basic trap
        const uint_least16_t addr = endian_little16(&reloc_driver[8]);
        mmu->installBasicTrap(0xffe1);
        mmu->writeMemWord(0x0328, addr);
    }

    int pos = relocAddr;

    // Install driver to ram
    mmu->fillRam(pos, &reloc_driver[10], reloc_size);

    // Tell C64 about song
    mmu->writeMemByte(pos, (uint8_t) (tuneInfo->currentSong() - 1));
    pos++;
    mmu->writeMemByte(pos, tuneInfo->songSpeed() == SidTuneInfo::SPEED_VBI ? 0 : 1);
    pos++;
    mmu->writeMemWord(pos, tuneInfo->compatibility() == SidTuneInfo::COMPATIBILITY_BASIC ?
                     0xbf55 /*Was 0xa7ae, see above*/ : tuneInfo->initAddr());
    pos += 2;
    mmu->writeMemWord(pos, tuneInfo->playAddr());
    pos += 2;
    // Initialise random number generator
    m_info.powerOnDelay = m_cfg.powerOnDelay;
    // Delays above MAX result in random delays
    if (m_info.powerOnDelay > SID2_MAX_POWER_ON_DELAY)
    {   // Limit the delay to something sensible.
        m_info.powerOnDelay = (uint_least16_t) (m_rand >> 3) &
                            SID2_MAX_POWER_ON_DELAY;
    }
    mmu->writeMemWord(pos, m_info.powerOnDelay);
    pos += 2;
    m_rand  = m_rand * 13 + 1;
    mmu->writeMemByte(pos, iomap (tuneInfo->initAddr()));
    pos++;
    mmu->writeMemByte(pos, iomap (tuneInfo->playAddr()));
    pos++;
    const uint8_t flag = mmu->readMemByte(0x02a6); // PAL/NTSC flag
    mmu->writeMemByte(pos, flag);
    pos++;

    // Add the required tune speed
    switch (tuneInfo->clockSpeed())
    {
    case SidTuneInfo::CLOCK_PAL:
        mmu->writeMemByte(pos, 1);
        break;
    case SidTuneInfo::CLOCK_NTSC:
        mmu->writeMemByte(pos, 0);
        break;
    default: // UNKNOWN or ANY
        mmu->writeMemByte(pos, flag);
        break;
    }
    pos++;

    // Default processor register flags on calling init
    mmu->writeMemByte(pos, tuneInfo->compatibility() >= SidTuneInfo::COMPATIBILITY_R64 ? 0 : 1 << MOS6510::SR_INTERRUPT);

    return 0;
}

SIDPLAYFP_NAMESPACE_STOP
