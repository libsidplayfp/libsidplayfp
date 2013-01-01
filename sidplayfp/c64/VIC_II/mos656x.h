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

#ifndef MOS656X_H
#define MOS656X_H

#include "sidplayfp/component.h"
#include "sidplayfp/EventScheduler.h"


class MOS656X: public component, private Event
{
public:
    typedef enum
    {
        MOS6567R56A = 0  /* OLD NTSC CHIP */
        ,MOS6567R8       /* NTSC-M */
        ,MOS6569         /* PAL-B */
        ,MOS6572         /* PAL-N */
    } model_t;

private:
    static const char *credit;

private:
    /** raster IRQ flag */
    static const int IRQ_RASTER = 1 << 0;

    /** Light-Pen IRQ flag */
    static const int IRQ_LIGHTPEN = 1 << 3;

protected:
    /** First line when we check for bad lines */
    static const int FIRST_DMA_LINE = 0x30;

    /** Last line when we check for bad lines */
    static const int LAST_DMA_LINE = 0xf7;

protected:
    event_clock_t m_rasterClk;

    /** CPU's event context. */
    EventContext &event_context;

    /** Number of cycles per line. */
    uint_least16_t cyclesPerLine;

    uint_least16_t maxRasters;

    uint_least16_t raster_irq;

    /** Current visible line */
    uint_least16_t lineCycle;

    /** current raster line */
    uint_least16_t rasterY;

    /** vertical scrolling value */
    uint_least16_t yscroll;

    /** are bad lines enabled for this frame? */
    bool areBadLinesEnabled;

    /** is the current line a bad line */
    bool isBadLine;

    /** Set when new frame starts. */
    bool vblanking;

    /** Has light pen IRQ been triggered in this frame already? */
    bool lp_triggered;

    /** internal IRQ flags */
    uint8_t irqFlags;

    /** masks for the IRQ flags */
    uint8_t irqMask;

    /** Light pen coordinates */
    uint8_t lpx, lpy;

    /** the 8 sprites data*/
    uint8_t &sprite_enable, &sprite_y_expansion;
    uint8_t sprite_dma, sprite_expand_y;
    uint8_t sprite_mc_base[8];

    /** memory for chip registers */
    uint8_t regs[0x40];

private:
    event_clock_t  clock   (void);

    /** Signal CPU interrupt if requested by VIC. */
    void handleIrqState();

protected:
    MOS656X(EventContext *context);
    ~MOS656X() {}

    void    event       (void);

    EventCallback<MOS656X> badLineStateChangeEvent;

    /** AEC state was updated. */
    void badLineStateChange() { setBA(false); }

    /**
    * Set an IRQ flag and trigger an IRQ if the corresponding IRQ mask is set.
    * The IRQ only gets activated, i.e. flag 0x80 gets set, if it was not active before.
    */
    void activateIRQFlag(int flag)
    {
        irqFlags |= flag;
        handleIrqState();
    }

    /**
    * Read the DEN flag which tells whether the display is enabled
    *
    * @return true if DEN is set, otherwise false
    */
    bool readDEN() const { return (regs[0x11] & 0x10) != 0; }

    bool evaluateIsBadLine() const
    {
        return areBadLinesEnabled
            && rasterY >= FIRST_DMA_LINE
            && rasterY <= LAST_DMA_LINE
            && (rasterY & 7) == yscroll;
    }

    // Environment Interface
    virtual void interrupt (bool state) = 0;
    virtual void setBA     (bool state) = 0;

    /**
    * Read VIC register.
    *
    * @param register
    *            Register to read.
    */
    uint8_t read  (uint_least8_t addr);

    /**
    * Write to VIC register.
    *
    * @param register
    *            Register to write to.
    * @param data
    *            Data byte to write.
    */
    void    write (uint_least8_t addr, uint8_t data);

public:
    void    chip  (model_t model);
    void    lightpen ();

    // Component Standard Calls
    void    reset (void);

    const   char *credits (void) const { return credit; }

    uint_least16_t getCyclesPerLine() const { return cyclesPerLine; }

    uint_least16_t getRasterLines() const { return maxRasters; }
};

#endif // MOS656X_H
