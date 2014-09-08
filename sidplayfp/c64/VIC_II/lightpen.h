/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2009-2014 VICE Project
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

#ifndef LIGHTPEN_H
#define LIGHTPEN_H

/**
 * Lightpen.
 */
class Lightpen
{
private:
    /// Horizontal coordinate
    uint8_t lpx;

    /// Vertical coordinate
    uint8_t lpy;

    /// Has light pen IRQ been triggered in this frame already?
    bool isTriggered;

public:
    /**
     * Reset lightpen status.
     */
    void reset()
    {
        isTriggered  = false;

        lpx = 0;
        lpy = 0;
    }

    /**
     * Reset IRQ status.
     */
    void untrigger() { isTriggered = false; }

    /**
     * Check if an IRQ is triggered.
     *
     * @param lineCycle
     * @param rasterY
     */
    bool trigger(unsigned int lineCycle, unsigned int rasterY)
    {
        if (!isTriggered)
        {
            isTriggered = true;

            // Latch current coordinates
            lpx = lineCycle << 2;
            lpy = (uint8_t)rasterY & 0xff;

            return true;
        }
        return false;
    }

    /**
     * Get X coordinete.
     */
    uint8_t getX() const { return lpx; }

    /**
     * Get Y coordinete.
     */
    uint8_t getY() const { return lpy; }
};

#endif // LIGHTPEN_H
