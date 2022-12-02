/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2018 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2009-2014 VICE Project
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
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

#include "tod.h"

#include <cstring>

#include "mos652x.h"

namespace libsidplayfp
{

void Tod::reset()
{
    cycles = 0;
    todtickcounter = 0;

    memset(clock, 0, sizeof(clock));
    clock[HOURS] = 1; // the most common value
    memcpy(latch, clock, sizeof(latch));
    memset(alarm, 0, sizeof(alarm));

    isLatched = false;
    isStopped = true;

    eventScheduler.schedule(*this, 0, EVENT_CLOCK_PHI1);
}

uint8_t Tod::read(uint_least8_t reg)
{
    // TOD clock is latched by reading Hours, and released
    // upon reading Tenths of Seconds. The counter itself
    // keeps ticking all the time.
    // Also note that this latching is different from the input one.
    if (!isLatched)
        memcpy(latch, clock, sizeof(latch));

    if (reg == TENTHS)
        isLatched = false;
    else if (reg == HOURS)
        isLatched = true;

    return latch[reg];
}

void Tod::write(uint_least8_t reg, uint8_t data)
{
    switch (reg)
    {
    case TENTHS: // Time Of Day clock 1/10 s
        data &= 0x0f;
        break;
    case SECONDS: // Time Of Day clock sec
        // fallthrough
    case MINUTES: // Time Of Day clock min
        data &= 0x7f;
        break;
    case HOURS:  // Time Of Day clock hour
        // force bits 6-5 = 0
        data &= 0x9f;
        // Flip AM/PM on hour 12
        // Flip AM/PM only when writing time, not when writing alarm
        if ((data & 0x1f) == 0x12 && !(crb & 0x80))
            data ^= 0x80;
        break;
    }

    bool changed = false;
    if (crb & 0x80)
    {
        // set alarm
        if (alarm[reg] != data)
        {
            changed = true;
            alarm[reg] = data;
        }
    }
    else
    {
        // set time
        if (reg == TENTHS)
        {
            // apparently the tickcounter is reset to 0 when the clock
            // is not running and then restarted by writing to the 10th
            // seconds register.
            if (isStopped)
            {
                todtickcounter = 0;
                isStopped = false;
            }
        }
        else if (reg == HOURS)
        {
            isStopped = true;
        }

        if (clock[reg] != data)
        {
            changed = true;
            clock[reg] = data;
        }
    }

    // check alarm
    if (changed)
    {
        checkAlarm();
    }
}

void Tod::event()
{
    cycles += period;

    // Fixed precision 25.7
    eventScheduler.schedule(*this, cycles >> 7);
    cycles &= 0x7F; // Just keep the decimal part

    if (!isStopped)
    {
        // count 50/60 hz ticks
        todtickcounter++;
        // wild assumption: counter is 3 bits and is not reset elsewhere
        // FIXME: this doesnt seem to be 100% correct - apparently it is reset
        //        in some cases
        todtickcounter &= 7;
        // if the counter matches the TOD frequency ...
        if (todtickcounter == ((cra & 0x80) ? 5 : 6))
        {
            // reset the counter and update the timer
            todtickcounter = 0;
            updateCounters();
        }
    }
}

void Tod::updateCounters()
{
    // advance the counters.
    // - individual counters are all 4 bit
    uint8_t ts = clock[TENTHS] & 0x0f;
    uint8_t sl = clock[SECONDS] & 0x0f;
    uint8_t sh = (clock[SECONDS] >> 4) & 0x0f;
    uint8_t ml = clock[MINUTES] & 0x0f;
    uint8_t mh = (clock[MINUTES] >> 4) & 0x0f;
    uint8_t hl = clock[HOURS] & 0x0f;
    uint8_t hh = (clock[HOURS] >> 4) & 0x01;
    uint8_t pm = clock[HOURS] & 0x80;

    // tenth seconds (0-9)
    ts = (ts + 1) & 0x0f;
    if (ts == 10)
    {
        ts = 0;
        // seconds (0-59)
        sl = (sl + 1) & 0x0f; // x0...x9
        if (sl == 10)
        {
            sl = 0;
            sh = (sh + 1) & 0x07; // 0x...5x
            if (sh == 6)
            {
                sh = 0;
                // minutes (0-59)
                ml = (ml + 1) & 0x0f; // x0...x9
                if (ml == 10)
                {
                    ml = 0;
                    mh = (mh + 1) & 0x07; // 0x...5x
                    if (mh == 6)
                    {
                        mh = 0;
                        // hours (1-12)
                        hl = (hl + 1) & 0x0f;
                        if (hh)
                        {
                            // toggle the am/pm flag when going from 11 to 12 (!)
                            if (hl == 2)
                            {
                                pm ^= 0x80;
                            }
                            // wrap 12h -> 1h (FIXME: when hour became x3 ?)
                            else if (hl == 3)
                            {
                                hl = 1;
                                hh = 0;
                            }
                        }
                        else
                        {
                            if (hl == 10)
                            {
                                hl = 0;
                                hh = 1;
                            }
                        }
                    }
                }
            }
        }
    }

    clock[TENTHS]  = ts;
    clock[SECONDS] = sl | (sh << 4);
    clock[MINUTES] = ml | (mh << 4);
    clock[HOURS]   = hl | (hh << 4) | pm;

    checkAlarm();
}

void Tod::checkAlarm()
{
    if (!memcmp(alarm, clock, sizeof(alarm)))
    {
        parent.todInterrupt();
    }
}

}
