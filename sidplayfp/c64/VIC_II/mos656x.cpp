/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

// References below are from:
//     The MOS 6567/6569 video controller (VIC-II)
//     and its application in the Commodore 64
//     http://www.uni-mainz.de/~bauec002/VIC-Article.gz


#include <string.h>
#if defined(HAVE_CONFIG_H) || defined(_WIN32)
#  include "config.h"
#endif
#include "sidplayfp/sidendian.h"
#include "mos656x.h"

#define MOS6567R56A_SCREEN_HEIGHT  262
#define MOS6567R56A_SCREEN_WIDTH   64

#define MOS6567R8_SCREEN_HEIGHT    263
#define MOS6567R8_SCREEN_WIDTH     65

#define MOS6569_SCREEN_HEIGHT      312
#define MOS6569_SCREEN_WIDTH       63

const char *MOS656X::credit =
{   // Optional information
    "MOS656X (VICII) Emulation:\n"
    "\tCopyright (C) 2001 Simon White\n"
};


MOS656X::MOS656X (EventContext *context)
:Event("VIC Raster"),
 event_context(*context),
 sprite_enable(regs[0x15]),
 sprite_y_expansion(regs[0x17]),
 badLineStateChangeEvent("Update AEC signal", *this, &MOS656X::badLineStateChange)
{
    chip (MOS6569);
}

void MOS656X::reset ()
{
    irqFlags     = 0;
    irqMask      = 0;
    raster_irq   = 0;
    yscroll      = 0;
    rasterY      = maxRasters - 1;
    lineCycle     = 0;
    areBadLinesEnabled = false;
    m_rasterClk  = 0;
    vblanking    = lp_triggered = false;
    lpx          = lpy = 0;
    sprite_dma   = 0;
    sprite_expand_y = 0xff;
    memset (regs, 0, sizeof (regs));
    memset (sprite_mc_base, 0, sizeof (sprite_mc_base));
    event_context.cancel(*this);
    event_context.schedule (*this, 0, EVENT_CLOCK_PHI1);
}

void MOS656X::chip (mos656x_model_t model)
{
    switch (model)
    {
    // Seems to be an older NTSC chip
    case MOS6567R56A:
        maxRasters    = MOS6567R56A_SCREEN_HEIGHT;
        cyclesPerLine = MOS6567R56A_SCREEN_WIDTH;
    break;

    // NTSC Chip
    case MOS6567R8:
        maxRasters    = MOS6567R8_SCREEN_HEIGHT;
        cyclesPerLine = MOS6567R8_SCREEN_WIDTH;
    break;

    // PAL Chip
    case MOS6569:
        maxRasters    = MOS6569_SCREEN_HEIGHT;
        cyclesPerLine = MOS6569_SCREEN_WIDTH;
    break;
    }

    reset ();
}

uint8_t MOS656X::read (uint_least8_t addr)
{
    addr &= 0x3f;

    // Sync up timers
    clock ();

    switch (addr)
    {
    case 0x11:
        // Control register 1
        return (regs[addr] & 0x7f) | ((rasterY & 0x100) >> 1);
    case 0x12:
        // Raster counter
        return rasterY & 0xFF;
    case 0x13:
        return lpx;
    case 0x14:
        return lpy;
    case 0x19:
        // Interrupt Pending Register
        return irqFlags | 0x70;
    case 0x1a:
        // Interrupt Mask Register
        return irqMask | 0xf0;
    default:
        // for addresses < $20 read from register directly, when < $2f set
        // bits of high nibble to 1, for >= $2f return $ff
        if (addr < 0x20)
            return regs[addr];
        if (addr < 0x2f)
            return regs[addr] | 0xf0;
        return 0xff;
    }
}

void MOS656X::write (uint_least8_t addr, uint8_t data)
{
    addr &= 0x3f;

    regs[addr] = data;

    // Sync up timers
    clock ();

    switch (addr)
    {
    case 0x11: // Control register 1
    {
        endian_16hi8 (raster_irq, data >> 7);
        yscroll = data & 7;

        if (lineCycle < 11)
            break;

        /* display enabled at any cycle of line 48 enables badlines */
        if (rasterY == FIRST_DMA_LINE)
            areBadLinesEnabled |= readDEN();

        /* Re-evaluate badline condition */
        isBadLine = evaluateIsBadLine();

        // Start bad dma line now
        if (isBadLine && (lineCycle < 53))
            event_context.schedule(badLineStateChangeEvent, 0, EVENT_CLOCK_PHI1);
        break;
    }

    case 0x12: // Raster counter
        endian_16lo8 (raster_irq, data);
        break;

    case 0x17:
        sprite_expand_y |= ~data; // 3.8.1-1
        break;

    case 0x19:
        // VIC Interrupt Flag Register
        irqFlags &= (~data & 0x0f) | 0x80;
        handleIrqState();
        break;

    case 0x1a:
        // IRQ Mask Register
        irqMask = data & 0x0f;
        handleIrqState();
        break;
    }
}

void MOS656X::handleIrqState()
{
    /* signal an IRQ unless we already signaled it */
    if ((irqFlags & irqMask & 0x0f) != 0)
    {
        if ((irqFlags & 0x80) == 0)
        {
            interrupt(true);
            irqFlags |= 0x80;
        }
    }
    else if ((irqFlags & 0x80) != 0)
    {
        interrupt(false);
        irqFlags &= 0x7f;
    }
}

void MOS656X::event (void)
{
    const event_clock_t delay = clock();
    event_context.schedule (*this, delay - event_context.phase(), EVENT_CLOCK_PHI1);
}

event_clock_t MOS656X::clock (void)
{
    event_clock_t delay = 1;

    const event_clock_t cycles = event_context.getTime (m_rasterClk, event_context.phase());

    // Cycle already executed check
    if (!cycles)
        return delay;

    // Update x raster
    m_rasterClk += cycles;
    lineCycle    += cycles;
    const uint_least16_t cycle = (lineCycle + 9) % cyclesPerLine;
    lineCycle    %= cyclesPerLine;

    switch (cycle)
    {
    case 0:
    {   // Calculate sprite DMA
        const uint8_t y = rasterY & 0xff;
        sprite_expand_y ^= sprite_y_expansion; // 3.8.1-2
        uint8_t mask = 1;
        for (unsigned int i=1; i<0x10; i+=2, mask<<=1)
        {   // 3.8.1-3
            if ((sprite_enable & mask) && (y == regs[i]))
            {
                sprite_dma |= mask;
                sprite_mc_base[i >> 1] = 0;
                sprite_expand_y &= ~(sprite_y_expansion & mask);
            }
        }

        if (sprite_dma & 0x01)
        {
            setBA (false);
            delay = 2;
        }
        else
        {
            setBA (true);
            // No sprites before next compulsory cycle
            delay = (sprite_dma & 0x1f) ? 2 : 9;
        }
        break;
    }

    case 1:
        break;

    case 2:
        if (sprite_dma & 0x02)
            setBA (false);
        break;

    case 3:
        if (!(sprite_dma & 0x03))
            setBA (true);
        break;

    case 4:
        if (sprite_dma & 0x04)
            setBA (false);
        break;

    case 5:
        if (!(sprite_dma & 0x06))
            setBA (true);
        break;

    case 6:
        if (sprite_dma & 0x08)
            setBA (false);
        break;

    case 7:
        if (!(sprite_dma & 0x0c))
            setBA (true);
        break;

    case 8:
        if (sprite_dma & 0x10)
            setBA (false);
        break;

    case 9:  // IRQ occurred (xraster != 0)
        if (rasterY == (maxRasters - 1))
            vblanking = true;
        else
        {
            rasterY++;
            // Trigger raster IRQ if IRQ line reached
            if (rasterY == raster_irq)
                activateIRQFlag(IRQ_RASTER);
        }
        if (!(sprite_dma & 0x18))
            setBA (true);
        break;

    case 10:  // Vertical blank (line 0)
        if (vblanking)
        {
            vblanking = lp_triggered = false;
            rasterY = 0;
            // Trigger raster IRQ if IRQ in line 0
            if (raster_irq == 0)
                activateIRQFlag(IRQ_RASTER);
        }
        if (sprite_dma & 0x20)
            setBA (false);
        // No sprites before next compulsory cycle
        else if (!(sprite_dma & 0xf8))
           delay = 10;
        break;

    case 11:
        if (!(sprite_dma & 0x30))
            setBA (true);
        break;

    case 12:
        if (sprite_dma & 0x40)
            setBA (false);
        break;

    case 13:
        if (!(sprite_dma & 0x60))
            setBA (true);
        break;

    case 14:
        if (sprite_dma & 0x80)
            setBA (false);
        break;

    case 15:
        if (!(sprite_dma & 0xc0))
        {
            setBA (true);
            delay = 5;
        } else
            delay = 2;
        break;

    case 16:
        break;

    case 17:
        if (!(sprite_dma & 0x80))
        {
            setBA (true);
            delay = 3;
        } else
            delay = 2;
        break;

    case 18:
        break;

    case 19:
        setBA (true);
        break;

    case 20: // Start bad line
    {   // In line $30, the DEN bit controls if Bad Lines can occur
        if (rasterY == FIRST_DMA_LINE)
            areBadLinesEnabled = readDEN();

        // Test for bad line condition
        isBadLine = evaluateIsBadLine();

        if (isBadLine)
        {   // DMA starts on cycle 23
            setBA (false);
        }
        delay = 3;
        break;
    }

    case 23:
    {   // 3.8.1-7
        for (unsigned int i=0; i<8; i++)
        {
            if (sprite_expand_y & (1 << i))
                sprite_mc_base[i] += 2;
        }
        break;
    }

    case 24:
    {
        uint8_t mask = 1;
        for (unsigned int i=0; i<8; i++, mask<<=1)
        {   // 3.8.1-8
            if (sprite_expand_y & mask)
                sprite_mc_base[i]++;
            if ((sprite_mc_base[i] & 0x3f) == 0x3f)
                sprite_dma &= ~mask;
        }
        delay = 39;
        break;
    }

    case 63: // End DMA - Only get here for non PAL
        setBA (true);
        delay = cyclesPerLine - cycle;
        break;

    default:
        if (cycle < 23)
            delay = 23 - cycle;
        else if (cycle < 63)
            delay = 63 - cycle;
        else
            delay = cyclesPerLine - cycle;
    }

    return delay;
}

// Handle light pen trigger
void MOS656X::lightpen ()
{   // Synchronise simulation
    clock ();

    if (!lp_triggered)
    {   // Latch current coordinates
        lpx = lineCycle << 2;
        lpy = (uint8_t) rasterY & 0xff;
        activateIRQFlag(IRQ_LIGHTPEN);
    }
}
