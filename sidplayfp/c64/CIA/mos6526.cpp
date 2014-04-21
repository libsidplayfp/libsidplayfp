/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2009-2014 VICE Project
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

#include "mos6526.h"

#include <cstring>

#include "sidplayfp/sidendian.h"

enum
{
    INTERRUPT_NONE         = 0,
    INTERRUPT_UNDERFLOW_A  = 1 << 0,
    INTERRUPT_UNDERFLOW_B  = 1 << 1,
    INTERRUPT_ALARM        = 1 << 2,
    INTERRUPT_SP           = 1 << 3,
    INTERRUPT_FLAG         = 1 << 4,
    INTERRUPT_REQUEST      = 1 << 7
};

enum
{
    PRA     = 0,
    PRB     = 1,
    DDRA    = 2,
    DDRB    = 3,
    TAL     = 4,
    TAH     = 5,
    TBL     = 6,
    TBH     = 7,
    TOD_TEN = 8,
    TOD_SEC = 9,
    TOD_MIN = 10,
    TOD_HR  = 11,
    SDR     = 12,
    ICR     = 13,
    IDR     = 13,
    CRA     = 14,
    CRB     = 15
};

void TimerA::underFlow()
{
    parent->underflowA();
}

void TimerA::serialPort()
{
    parent->serialPort();
}

void TimerB::underFlow()
{
    parent->underflowB();
}

const char *MOS6526::credit =
{
    "MOS6526 (CIA) Emulation:\n"
    "\tCopyright (C) 2001-2004 Simon White\n"
    "\tCopyright (C) 2009-2014 VICE Project\n"
    "\tCopyright (C) 2009-2010 Antti S. Lankila\n"
    "\tCopyright (C) 2011-2014 Leandro Nini\n"
};

MOS6526::MOS6526(EventContext *context) :
    pra(regs[PRA]),
    prb(regs[PRB]),
    ddra(regs[DDRA]),
    ddrb(regs[DDRB]),
    timerA(context, this),
    timerB(context, this),
    idr(0),
    event_context(*context),
    m_todPeriod(~0), // Dummy
    bTickEvent("CIA B counts A", *this, &MOS6526::bTick),
    todEvent("CIA Time of Day", *this, &MOS6526::tod),
    triggerEvent("Trigger Interrupt", *this, &MOS6526::trigger)
{
    reset();
}

void MOS6526::serialPort()
{
    if (regs[CRA] & 0x40)
    {
        if (sdr_count)
        {
            if (--sdr_count == 0)
            {
                trigger(INTERRUPT_SP);
            }
        }
        if (sdr_count == 0 && sdr_buffered)
        {
            sdr_out = regs[SDR];
            sdr_buffered = false;
            sdr_count = 16;
            // Output rate 8 bits at ta / 2
        }
    }
}

void MOS6526::clear()
{
    if (idr & INTERRUPT_REQUEST)
        interrupt(false);
    idr = 0;
}


void MOS6526::setDayOfTimeRate(unsigned int clock)
{
    m_todPeriod = (event_clock_t) clock * (1 << 7);
}

void MOS6526::reset()
{
    sdr_out = 0;
    sdr_count = 0;
    sdr_buffered = false;
    // Clear off any IRQs
    clear();
    icr = idr = 0;
    memset(regs, 0, sizeof(regs));

    // Reset timers
    timerA.reset();
    timerB.reset();

    // Reset tod
    memset(m_todclock, 0, sizeof(m_todclock));
    m_todclock[TOD_HR-TOD_TEN] = 1; // the most common value
    memcpy(m_todlatch, m_todclock, sizeof(m_todlatch));
    memset(m_todalarm, 0, sizeof(m_todalarm));
    m_todlatched = false;
    m_todstopped = true;
    m_todCycles = 0;

    triggerScheduled = false;

    event_context.cancel(bTickEvent);
    event_context.cancel(triggerEvent);
    event_context.schedule(todEvent, 0, EVENT_CLOCK_PHI1);
}

uint8_t MOS6526::read(uint_least8_t addr)
{
    addr &= 0x0f;

    timerA.syncWithCpu();
    timerA.wakeUpAfterSyncWithCpu();
    timerB.syncWithCpu();
    timerB.wakeUpAfterSyncWithCpu();

    switch (addr)
    {
    case PRA: // Simulate a serial port
        return (regs[PRA] | ~regs[DDRA]);
    case PRB:{
        uint8_t data = regs[PRB] | ~regs[DDRB];
        // Timers can appear on the port
        if (regs[CRA] & 0x02)
        {
            data &= 0xbf;
            if (timerA.getPb(regs[CRA]))
                data |= 0x40;
        }
        if (regs[CRB] & 0x02)
        {
            data &= 0x7f;
            if (timerB.getPb(regs[CRB]))
                data |= 0x80;
        }
        return data;}
    case TAL:
        return endian_16lo8(timerA.getTimer());
    case TAH:
        return endian_16hi8(timerA.getTimer());
    case TBL:
        return endian_16lo8(timerB.getTimer());
    case TBH:
        return endian_16hi8(timerB.getTimer());

    // TOD implementation taken from Vice
    // TOD clock is latched by reading Hours, and released
    // upon reading Tenths of Seconds. The counter itself
    // keeps ticking all the time.
    // Also note that this latching is different from the input one.
    case TOD_TEN: // Time Of Day clock 1/10 s
    case TOD_SEC: // Time Of Day clock sec
    case TOD_MIN: // Time Of Day clock min
    case TOD_HR:  // Time Of Day clock hour
        if (!m_todlatched)
            memcpy(m_todlatch, m_todclock, sizeof(m_todlatch));
        if (addr == TOD_TEN)
            m_todlatched = false;
        if (addr == TOD_HR)
            m_todlatched = true;
        return m_todlatch[addr - TOD_TEN];
    case IDR:{
        if (triggerScheduled)
        {
            event_context.cancel(triggerEvent);
            triggerScheduled = false;
        }
        // Clear IRQs, and return interrupt
        // data register
        const uint8_t ret = idr;
        clear();
        return ret;}
    case CRA:
        return (regs[CRA] & 0xee) | (timerA.getState() & 1);
    case CRB:
        return (regs[CRB] & 0xee) | (timerB.getState() & 1);
    default:
        return regs[addr];
    }
}

void MOS6526::setTodReg(uint_least8_t addr, uint8_t data)
{
    if (regs[CRB] & 0x80)
    {
        /* set alarm */
        m_todalarm[addr - TOD_TEN] = data;
    }
    else
    {
        /* set time */
        if (addr == TOD_TEN)
            m_todstopped = false;
        if (addr == TOD_HR)
            m_todstopped = true;
        m_todclock[addr - TOD_TEN] = data;
    }

    // check alarm
    if (!m_todstopped && !memcmp(m_todalarm, m_todclock, sizeof(m_todalarm)))
    {
        trigger(INTERRUPT_ALARM);
    }
}

void MOS6526::write(uint_least8_t addr, uint8_t data)
{
    addr &= 0x0f;

    timerA.syncWithCpu();
    timerB.syncWithCpu();

    const uint8_t oldData = regs[addr];
    regs[addr] = data;

    switch (addr)
    {
    case PRA:
    case DDRA:
        portA();
        break;
    case PRB:
    case DDRB:
        portB();
        break;
    case TAL:
        timerA.latchLo(data);
        break;
    case TAH:
        timerA.latchHi(data);
        break;
    case TBL:
        timerB.latchLo(data);
        break;
    case TBH:
        timerB.latchHi(data);
        break;

    // TOD implementation taken from Vice
    case TOD_HR:  // Time Of Day clock hour
        /* force bits 6-5 = 0 */
        data &= 0x9f;
        /* Flip AM/PM on hour 12  */
        /* Flip AM/PM only when writing time, not when writing alarm */
        if ((data & 0x1f) == 0x12 && !(regs[CRB] & 0x80))
            data ^= 0x80;

        setTodReg(addr, data);
        break;
    case TOD_TEN: // Time Of Day clock 1/10 s
        data &= 0x0f;
        setTodReg(addr, data);
        break;
    case TOD_SEC: // Time Of Day clock sec
        // deliberate run on
    case TOD_MIN: // Time Of Day clock min
        data &= 0x7f;
        setTodReg(addr, data);
        break;
    case SDR:
        if (regs[CRA] & 0x40)
            sdr_buffered = true;
        break;
    case ICR:
        if (data & 0x80)
        {
            icr |= data & ~INTERRUPT_REQUEST;
            trigger(INTERRUPT_NONE);
        }
        else
        {
            icr &= ~data;
        }
        break;
    case CRA:{
        if ((data & 1) && !(oldData & 1))
        {
            // Reset the underflow flipflop for the data port
            timerA.setPbToggle(true);
        }
        timerA.setControlRegister(data);
        break;}
    case CRB:{
        if ((data & 1) && !(oldData & 1))
        {
            // Reset the underflow flipflop for the data port
            timerB.setPbToggle(true);
        }
        timerB.setControlRegister(data | (data & 0x40) >> 1);
        break;}
    }

    timerA.wakeUpAfterSyncWithCpu();
    timerB.wakeUpAfterSyncWithCpu();
}

void MOS6526::trigger()
{
    idr |= INTERRUPT_REQUEST;
    interrupt(true);
    triggerScheduled = false;
}

void MOS6526::trigger(uint8_t interruptMask)
{
    idr |= interruptMask;
    if ((icr & idr) && !(idr & INTERRUPT_REQUEST))
    {
        if (!triggerScheduled)
        {
            // Schedules an IRQ asserting state transition for next cycle.
            event_context.schedule(triggerEvent, 1, EVENT_CLOCK_PHI1);
            triggerScheduled = true;
        }
    }
}

void MOS6526::bTick()
{
    timerB.cascade();
}

void MOS6526::underflowA()
{
    trigger(INTERRUPT_UNDERFLOW_A);
    if ((regs[CRB] & 0x41) == 0x41)
    {
        if (timerB.started())
        {
            event_context.schedule(bTickEvent, 0, EVENT_CLOCK_PHI2);
        }
    }
}

void MOS6526::underflowB()
{
    trigger(INTERRUPT_UNDERFLOW_B);
}

void MOS6526::tod()
{
    // Reload divider according to 50/60 Hz flag
    // Only performed on expiry according to Frodo
    if (regs[CRA] & 0x80)
        m_todCycles += (m_todPeriod * 5);
    else
        m_todCycles += (m_todPeriod * 6);

    // Fixed precision 25.7
    event_context.schedule(todEvent, m_todCycles >> 7);
    m_todCycles &= 0x7F; // Just keep the decimal part

    if (!m_todstopped)
    {
        /* advance the counters.
         * - individual counters are all 4 bit
         */
        uint8_t t0 = m_todclock[TOD_TEN-TOD_TEN] & 0x0f;
        uint8_t t1 = m_todclock[TOD_SEC-TOD_TEN] & 0x0f;
        uint8_t t2 = (m_todclock[TOD_SEC-TOD_TEN] >> 4) & 0x0f;
        uint8_t t3 = m_todclock[TOD_MIN-TOD_TEN] & 0x0f;
        uint8_t t4 = (m_todclock[TOD_MIN-TOD_TEN] >> 4) & 0x0f;
        uint8_t t5 = m_todclock[TOD_HR-TOD_TEN] & 0x0f;
        uint8_t t6 = (m_todclock[TOD_HR-TOD_TEN] >> 4) & 0x01;
        uint8_t pm = m_todclock[TOD_HR-TOD_TEN] & 0x80;

        /* tenth seconds (0-9) */
        t0 = (t0 + 1) & 0x0f;
        if (t0 == 10)
        {
            t0 = 0;
            /* seconds (0-59) */
            t1 = (t1 + 1) & 0x0f; /* x0...x9 */
            if (t1 == 10)
            {
                t1 = 0;
                t2 = (t2 + 1) & 0x07; /* 0x...5x */
                if (t2 == 6)
                {
                    t2 = 0;
                    /* minutes (0-59) */
                    t3 = (t3 + 1) & 0x0f; /* x0...x9 */
                    if (t3 == 10)
                    {
                        t3 = 0;
                        t4 = (t4 + 1) & 0x07; /* 0x...5x */
                        if (t4 == 6)
                        {
                            t4 = 0;
                            /* hours (1-12) */
                            t5 = (t5 + 1) & 0x0f;
                            if (t6)
                            {
                                /* toggle the am/pm flag when going from 11 to 12 (!) */
                                if (t5 == 2)
                                {
                                    pm ^= 0x80;
                                }
                                /* wrap 12h -> 1h (FIXME: when hour became x3 ?) */
                                if (t5 == 3)
                                {
                                    t5 = 1;
                                    t6 = 0;
                                }
                            }
                            else
                            {
                                if (t5 == 10)
                                {
                                    t5 = 0;
                                    t6 = 1;
                                }
                            }
                        }
                    }
                }
            }
        }

        m_todclock[TOD_TEN-TOD_TEN] = t0;
        m_todclock[TOD_SEC-TOD_TEN] = t1 | (t2 << 4);
        m_todclock[TOD_MIN-TOD_TEN] = t3 | (t4 << 4);
        m_todclock[TOD_HR-TOD_TEN] = t5 | (t6 << 4) | pm;

        // check alarm
        if (!memcmp(m_todalarm, m_todclock, sizeof(m_todalarm)))
        {
            trigger(INTERRUPT_ALARM);
        }
    }
}
