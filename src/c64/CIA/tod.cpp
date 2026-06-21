/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2009-2024 VICE Project
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

    std::memset(m_clock, 0, sizeof(m_clock));
    m_clock[HOURS] = 1; // the most common value
    std::memcpy(m_latch, m_clock, sizeof(m_latch));
    std::memset(m_alarm, 0, sizeof(m_alarm));

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
        std::memcpy(m_latch, m_clock, sizeof(m_latch));

    if (reg == TENTHS)
        isLatched = false;
    else if (reg == HOURS)
        isLatched = true;

    return m_latch[reg];
}

void Tod::write(uint_least8_t reg, uint8_t data)
{
    switch (reg)
    {
    case TENTHS: // Time Of Day clock 1/10 s
        data &= 0x0f;
        break;
    case SECONDS: // Time Of Day clock sec
    case MINUTES: // Time Of Day clock min
        data &= 0x7f;
        break;
    case HOURS:  // Time Of Day clock hour
        data &= 0x9f; // force bits 6-5 = 0
        // Flip AM/PM on hour 12 on the rising edge of the comparator
        if (((data & 0x1f) == 0x12) && !(crb & 0x80))
            data ^= 0x80;
        break;
    }

    bool changed = false;
    if (crb & 0x80)
    {
        // set alarm
        if (m_alarm[reg] != data)
        {
            changed = true;
            m_alarm[reg] = data;
        }
    }
    else
    {
        // set time
        if (reg == TENTHS)
        {
            // the tickcounter is kept clear while the clock
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

        if (m_clock[reg] != data)
        {
            // see https://sourceforge.net/p/vice-emu/bugs/1988/
            // Flip AM/PM on hour 12 on the rising edge of the comparator
            //if ((reg == HOURS) && ((data & 0x1f) == 0x12))
            //    data ^= 0x80;

            changed = true;
            m_clock[reg] = data;
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
        /*
         * The divider which divides the 50 or 60 Hz power supply ticks into
         * 10 Hz uses a 3-bit ring counter, which goes 000, 001, 011, 111, 110,
         * 100.
         * For 50 Hz: matches at 110 (like "4")
         * For 60 Hz: matches at 100 (like "5")
         * (the middle bit of the match value is CRA7)
         * After a match there is a 1 tick delay (until the next power supply
         * tick) and then the 1/10 seconds counter increases, and the ring
         * resets to 000.
         */
        // todtickcounter bits are mirrored to save an ANDing
        if (todtickcounter == (0x1 | ((cra & 0x80) >> 6)))
        {
            // reset the counter and update the timer
            todtickcounter = 0;
            updateCounters();
        }
        else
        {
            // Count 50/60 Hz power supply ticks
            todtickcounter = (todtickcounter >> 1) | ((~todtickcounter << 2) & 0x4);
        }
    }
}

void Tod::updateCounters()
{
    // advance the counters.
    // - individual counters are 4 bit
    //   except for sh and mh which are 3 bits
    uint8_t ts = m_clock[TENTHS] & 0x0f;
    uint8_t sl = m_clock[SECONDS] & 0x0f;
    uint8_t sh = (m_clock[SECONDS] >> 4) & 0x07;
    uint8_t ml = m_clock[MINUTES] & 0x0f;
    uint8_t mh = (m_clock[MINUTES] >> 4) & 0x07;
    uint8_t hl = m_clock[HOURS] & 0x0f;
    uint8_t hh = (m_clock[HOURS] >> 4) & 0x01;
    uint8_t pm = m_clock[HOURS] & 0x80;

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
                        // flip from 09:59:59 to 10:00:00
                        // or from 12:59:59 to 01:00:00
                        if (((hl == 2) && (hh == 1))
                            || ((hl == 9) && (hh == 0)))
                        {
                            hl = hh;
                            hh ^= 1;
                        }
                        else
                        {
                            hl = (hl + 1) & 0x0f;
                            // toggle the am/pm flag when reaching 12
                            if ((hl == 2) && (hh == 1))
                            {
                                pm ^= 0x80;
                            }
                        }
                    }
                }
            }
        }
    }

    m_clock[TENTHS]  = ts;
    m_clock[SECONDS] = sl | (sh << 4);
    m_clock[MINUTES] = ml | (mh << 4);
    m_clock[HOURS]   = hl | (hh << 4) | pm;

    checkAlarm();
}

void Tod::checkAlarm()
{
    if (!std::memcmp(m_alarm, m_clock, sizeof(m_alarm)))
    {
        m_parent.todInterrupt();
    }
}

}
