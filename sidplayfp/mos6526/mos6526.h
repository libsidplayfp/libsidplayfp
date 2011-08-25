/***************************************************************************
                          mos6526.h  -  CIA timer to produce interrupts
                             -------------------
    begin                : Wed Jun 7 2000
    copyright            : (C) 2000 by Simon White
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
/***************************************************************************
 *  $Log: mos6526.h,v $
 *  Revision 1.17  2004/06/26 11:09:13  s_a_white
 *  Changes to support new calling convention for event scheduler.
 *
 *  Revision 1.16  2004/04/13 07:39:32  s_a_white
 *  Add lightpen support.
 *
 *  Revision 1.15  2004/03/09 20:26:15  s_a_white
 *  Keep track of timer A/B underflows for the I/O ports.
 *
 *  Revision 1.14  2004/02/29 14:30:18  s_a_white
 *  Serial port emulation.
 *
 *  Revision 1.13  2004/01/06 21:28:27  s_a_white
 *  Initial TOD support (code taken from vice)
 *
 *  Revision 1.12  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.11  2002/12/16 22:12:27  s_a_white
 *  Simulate serial input from data port A to prevent kernel lockups.
 *
 *  Revision 1.10  2002/10/02 19:49:22  s_a_white
 *  Revert previous change as was incorrect.
 *
 *  Revision 1.9  2002/09/11 22:30:47  s_a_white
 *  Counter interval writes now go to a new register call prescaler.  This is
 *  copied to the timer latch/counter as appropriate.
 *
 *  Revision 1.8  2002/07/20 08:34:52  s_a_white
 *  Remove unnecessary and pointless conts.
 *
 *  Revision 1.7  2001/10/18 22:35:45  s_a_white
 *  GCC3 fixes.
 *
 *  Revision 1.6  2001/07/14 16:46:59  s_a_white
 *  Sync with sidbuilder class project.
 *
 *  Revision 1.5  2001/07/14 13:03:33  s_a_white
 *  Now uses new component classes and event generation.
 *
 *  Revision 1.4  2001/03/25 19:50:16  s_a_white
 *  Timer B counts timer Aer a underflows correc
 *
 *  Revision 1.3  2001/03/23 23:20:29  s_a_white
 *  Removed redundant reset prototype.
 *
 *  Revision 1.2  2001/03/22 22:41:45  s_a_white
 *  Replaced tab characters
 *
 *  Revision 1.1  2001/03/21 22:41:45  s_a_white
 *  Non faked CIA emulation with NMI support.  Removal of Hacked VIC support
 *  off CIA timer.
 *
 *  Revision 1.7  2001/03/09 23:44:30  s_a_white
 *  Integrated more 6526 features.  All timer modes and interrupts correctly
 *  supported.
 *
 *  Revision 1.6  2001/02/21 22:07:10  s_a_white
 *  Prevent re-triggering of interrupt if it's already active.
 *
 *  Revision 1.5  2001/02/13 21:00:01  s_a_white
 *  Support for real interrupts.
 *
 *  Revision 1.3  2000/12/11 18:52:12  s_a_white
 *  Conversion to AC99
 *
 ***************************************************************************/

#ifndef _mos6526_h_
#define _mos6526_h_

#include "sidplayfp/component.h"
#include "sidplayfp/event.h"

class Timer {

private:
    static const int CIAT_CR_START   = 0x01;
    static const int CIAT_STEP       = 0x04;
    static const int CIAT_CR_ONESHOT = 0x08;
    static const int CIAT_CR_FLOAD   = 0x10;

private:
    uint8_t        m_cr;
    uint_least16_t m_timer;
    uint_least16_t m_latch;
    bool           m_underflow;
    bool           m_pulse;

public:
    Timer() :
    m_cr(0),
    m_timer(0),
    m_latch(0),
    m_underflow(false),
    m_pulse(false) {}

    inline void reset();
    inline bool cr(const uint8_t data);
    inline void pulseDown() { m_pulse = false; }
    inline void latchLo(const uint8_t data);
    inline void latchHi(const uint8_t data);
    inline void underflow();

    inline void decTimer(const event_clock_t cycles) { m_timer -= cycles; }
    inline void load() { m_timer = m_latch; }

    inline uint8_t cr() const { return m_cr; }
    inline uint_least16_t timer() const { return m_timer; }
    inline bool pb() const { return m_cr & CIAT_STEP ? m_underflow : m_pulse; }
    inline uint8_t timerLo() const;
    inline uint8_t timerHi() const;
};

class MOS6526: public component
{
private:
    static const char *credit;

protected:
    uint8_t regs[0x10];
    bool    cnt_high;

    // Ports
    uint8_t &pra, &prb, &ddra, &ddrb;

    // Timers
    Timer timerA;
    Timer timerB;

    // Serial Data Registers
    uint8_t sdr_out;
    bool    sdr_buffered;
    int     sdr_count;

    uint8_t icr, idr; // Interrupt Control Register
    event_clock_t m_accessClk;
    EventContext &event_context;

    bool    m_todlatched;
    bool    m_todstopped;
    uint8_t m_todclock[4], m_todalarm[4], m_todlatch[4];
    event_clock_t m_todCycles, m_todPeriod;

    EventCallback<MOS6526> m_taEvent;
    EventCallback<MOS6526> m_tbEvent;
    EventCallback<MOS6526> m_todEvent;
    EventCallback<MOS6526> m_tapulse;
    EventCallback<MOS6526> m_tbpulse;
    EventCallback<MOS6526> m_trigger;
    EventCallback<MOS6526> m_taload;
    EventCallback<MOS6526> m_tastart;
    EventCallback<MOS6526> m_tacontinue;
    EventCallback<MOS6526> m_tastop;
    EventCallback<MOS6526> m_tbload;
    EventCallback<MOS6526> m_tbstart;
    EventCallback<MOS6526> m_tbcontinue;
    EventCallback<MOS6526> m_tbstop;
    EventCallback<MOS6526> m_tabEvent;

protected:
    MOS6526 (EventContext *context);
    void ta_event  (void);
    void tb_event  (void);
    void tab_event  (void);
    void ta_pulse_down (void);
    void tb_pulse_down (void);
    void ta_load (void);
    void ta_start (void);
    void ta_continue(void);
    void ta_stop(void);
    void tb_load (void);
    void tb_start (void);
    void tb_continue(void);
    void tb_stop(void);
    void tod_event (void);
    void trigger   (void);

    // Environment Interface
    virtual void interrupt (bool state) = 0;
    virtual void portA () {}
    virtual void portB () {}

public:
    // Component Standard Calls
    virtual void reset (void);
    uint8_t read  (uint_least8_t addr);
    void    write (uint_least8_t addr, uint8_t data);
    const   char *credits (void) {return credit;}
    void    update_timers(void);
    void    clear(void);

    // @FIXME@ This is not correct!  There should be
    // muliple schedulers running at different rates
    // that are passed into different function calls.
    // This is the same as have different clock freqs
    // connected to pins on the IC.
    void clock (float64_t clock);
};

#endif // _mos6526_h_
