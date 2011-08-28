/***************************************************************************
                          mos6526.cpp  -  CIA Timer
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

#include "mos6526.h"

#include <string.h>

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
#include <stdio.h>
void Timer::setControlRegister(const uint8_t cr) {
	state &= ~CIAT_CR_MASK;
	state |= cr & CIAT_CR_MASK ^ CIAT_PHI2IN;
	lastControlValue = cr;
}

void Timer::syncWithCpu() {
	if (ciaEventPauseTime > 0) {
		event_context.cancel(m_cycleSkippingEvent);
		const event_clock_t elapsed = event_context.getTime(EVENT_CLOCK_PHI2) - ciaEventPauseTime;
		/* It's possible for CIA to determine that it wants to go to sleep starting from the next
		* cycle, and then have its plans aborted by CPU. Thus, we must avoid modifying
		* the CIA state if the first sleep clock was still in the future.
		*/
		if (elapsed >= 0) {
			timer -= elapsed;
			clock();
		}
	}
	if (ciaEventPauseTime == 0) {
		event_context.cancel(*this);
	}
	ciaEventPauseTime = -1;
}

void Timer::wakeUpAfterSyncWithCpu() {
	ciaEventPauseTime = 0;
	event_context.schedule(*this, 0, EVENT_CLOCK_PHI1);
}

void Timer::event() {
	clock();
	reschedule();
}

void Timer::cycleSkippingEvent(void) {
	const event_clock_t elapsed = event_context.getTime(EVENT_CLOCK_PHI1) - ciaEventPauseTime;
	ciaEventPauseTime = 0;
	timer -= elapsed;
	event();
}

void Timer::clock() {
	if (timer != 0 && (state & CIAT_COUNT3) != 0) {
		timer --;
	}

	/* ciatimer.c block start */
	int_least32_t adj = state & (CIAT_CR_START | CIAT_CR_ONESHOT | CIAT_PHI2IN);
	if ((state & (CIAT_CR_START | CIAT_PHI2IN)) == (CIAT_CR_START | CIAT_PHI2IN)) {
		adj |= CIAT_COUNT2;
	}
	if ((state & CIAT_COUNT2) != 0
			|| (state & (CIAT_STEP | CIAT_CR_START)) == (CIAT_STEP | CIAT_CR_START)) {
		adj |= CIAT_COUNT3;
	}
	/* CR_FLOAD -> LOAD1, CR_ONESHOT -> ONESHOT0, LOAD1 -> LOAD, ONESHOT0 -> ONESHOT */
	adj |= (state & (CIAT_CR_FLOAD | CIAT_CR_ONESHOT | CIAT_LOAD1 | CIAT_ONESHOT0)) << 8;
	state = adj;
	/* ciatimer.c block end */

	if (timer == 0 && (state & CIAT_COUNT3) != 0) {
		state |= CIAT_LOAD | CIAT_OUT;

		if ((state & (CIAT_ONESHOT | CIAT_ONESHOT0)) != 0) {
			state &= ~(CIAT_CR_START | CIAT_COUNT2);
		}

		// By setting bits 2&3 of the control register,
		// PB6/PB7 will be toggled between high and low at each underflow.
		const bool toggle = (lastControlValue & 0x06) == 6;
		pbToggle = toggle && !pbToggle;

		// Implementation of the serial port
		serialPort();

		// Timer A signals underflow handling: IRQ/B-count
		underFlow();
	}

	if ((state & CIAT_LOAD) != 0) {
		timer = latch;
		state &= ~CIAT_COUNT3;
	}
}

void Timer::reschedule() {
	/* There are only two subcases to consider.
	*
	* - are we counting, and if so, are we going to
	*   continue counting?
	* - have we stopped, and are there no conditions to force a new beginning?
	*
	* Additionally, there are numerous flags that are present only in passing manner,
	* but which we need to let cycle through the CIA state machine.
	*/
	const int_least32_t unwanted = CIAT_OUT | CIAT_CR_FLOAD | CIAT_LOAD1 | CIAT_LOAD;
	if ((state & unwanted) != 0) {
		event_context.schedule(*this, 1);
		return;
	}

	if ((state & CIAT_COUNT3) != 0) {
		/* Test the conditions that keep COUNT2 and thus COUNT3 alive, and also
		* ensure that all of them are set indicating steady state operation. */

		const int_least32_t wanted = CIAT_CR_START | CIAT_PHI2IN | CIAT_COUNT2 | CIAT_COUNT3;
		if (timer > 2 && (state & wanted) == wanted) {
			/* we executed this cycle, therefore the pauseTime is +1. If we are called
			* to execute on the very next clock, we need to get 0 because there's
			* another timer-- in it. */
			ciaEventPauseTime = event_context.getTime(EVENT_CLOCK_PHI1) + 1;
			/* execute event slightly before the next underflow. */
			event_context.schedule(m_cycleSkippingEvent, timer - 1);
			return;
		}

		/* play safe, keep on ticking. */
		event_context.schedule(*this, 1);
	} else {
		/* Test conditions that result in CIA activity in next clocks.
		* If none, stop. */
		const int_least32_t unwanted1 = CIAT_CR_START | CIAT_PHI2IN;
		const int_least32_t unwanted2 = CIAT_CR_START | CIAT_STEP;

		if ((state & unwanted1) == unwanted1
				|| (state & unwanted2) == unwanted2) {
			event_context.schedule(*this, 1);
			return;
		}

		ciaEventPauseTime = -1;
	}
}

void Timer::reset() {
	event_context.cancel(*this);
	timer = latch = 0xffff;
	pbToggle = false;
	state = 0;
	lastControlValue = 0;
	ciaEventPauseTime = 0;
	event_context.schedule(*this, 1, EVENT_CLOCK_PHI1);
}

void Timer::latchLo(const uint8_t data) {
	endian_16lo8(latch, data);
	if (state & CIAT_LOAD)
		endian_16lo8(timer, data);
}

void Timer::latchHi(const uint8_t data) {
	endian_16hi8 (latch, data);
	if ((state & CIAT_LOAD) || !(state & CIAT_CR_START)) // Reload timer if stopped
		timer = latch;
}

void TimerA::underFlow() {
	parent->underflowA();
}

void TimerA::serialPort() {
	parent->serialPort();
}

void TimerB::underFlow() {
	parent->underflowB();
}

const char *MOS6526::credit = {
	"*MOS6526 (CIA) Emulation:\0"
	"\tCopyright (C) 2001-2004 Simon White <sidplay2@yahoo.com>\0"
	"\tCopyright (C) 2009 VICE Project\0"
	"\tCopyright (C) 2009-2010 Antti S. Lankila\0"
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
	triggerEvent("Trigger Interrupt", *this, &MOS6526::trigger) {

	reset();
}

void MOS6526::serialPort() {
	if (regs[CRA] & 0x40) {
		if (sdr_count) {
			if (--sdr_count == 0) {
				trigger(INTERRUPT_SP);
			}
		}
		if (sdr_count == 0 && sdr_buffered) {
			sdr_out = regs[SDR];
			sdr_buffered = false;
			sdr_count = 16;
			// Output rate 8 bits at ta / 2
		}
	}
}

void MOS6526::clear(void) {
	if (idr & INTERRUPT_REQUEST)
		interrupt(false);
	idr = 0;
}


void MOS6526::clock(float64_t clock) {
	m_todPeriod = (event_clock_t) (clock * (float64_t) (1 << 7));
}

void MOS6526::reset(void) {
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
	memset(m_todalarm, 0, sizeof(m_todalarm));
	memset(m_todlatch, 0, sizeof(m_todlatch));
	m_todlatched = false;
	m_todstopped = true;
	m_todclock[TOD_HR-TOD_TEN] = 1; // the most common value
	m_todCycles = 0;

	triggerScheduled = false;

	event_context.cancel(bTickEvent);
	event_context.cancel(triggerEvent);
	event_context.schedule(todEvent, 0, EVENT_CLOCK_PHI1);
}

uint8_t MOS6526::read(uint_least8_t addr) {
	addr &= 0x0f;

	timerA.syncWithCpu();
	timerA.wakeUpAfterSyncWithCpu();
	timerB.syncWithCpu();
	timerB.wakeUpAfterSyncWithCpu();

	switch (addr) {
	case PRA: // Simulate a serial port
		return (regs[PRA] | ~regs[DDRA]);
	case PRB:{
		uint8_t data = regs[PRB] | ~regs[DDRB];
		// Timers can appear on the port
		if (regs[CRA] & 0x02) {
			data &= 0xbf;
			if (timerA.getPb(regs[CRA]))
				data |= 0x40;
		}
		if (regs[CRB] & 0x02) {
			data &= 0x7f;
			if (timerB.getPb(regs[CRB]))
				data |= 0x80;
		}
		return data;}
	case TAL:
		return endian_16lo8 (timerA.getTimer());
	case TAH:
		return endian_16hi8 (timerA.getTimer());
	case TBL:
		return endian_16lo8 (timerB.getTimer());
	case TBH:
		return endian_16hi8 (timerB.getTimer());

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
		if (triggerScheduled) {
			event_context.cancel(triggerEvent);
			triggerScheduled = false;
		}
		// Clear IRQs, and return interrupt
		// data register
		const uint8_t ret = idr;
		clear();
		return ret;}
	case CRA:
		return regs[CRA] & 0xee | timerA.getState() & 1;
	case CRB:
		return regs[CRB] & 0xee | timerB.getState() & 1;
	default:
		return regs[addr];
	}
}

void MOS6526::write(uint_least8_t addr, uint8_t data) {
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
		// Flip AM/PM on hour 12
		//   (Andreas Boose <viceteam@t-online.de> 1997/10/11).
		// Flip AM/PM only when writing time, not when writing alarm
		// (Alexander Bluhm <mam96ehy@studserv.uni-leipzig.de> 2000/09/17).
		data &= 0x9f;
		if ((data & 0x1f) == 0x12 && !(regs[CRB] & 0x80))
			data ^= 0x80;
		// deliberate run on
	case TOD_TEN: // Time Of Day clock 1/10 s
	case TOD_SEC: // Time Of Day clock sec
	case TOD_MIN: // Time Of Day clock min
		if (regs[CRB] & 0x80)
			m_todalarm[addr - TOD_TEN] = data;
		else {
			if (addr == TOD_TEN)
				m_todstopped = false;
			if (addr == TOD_HR)
				m_todstopped = true;
			m_todclock[addr - TOD_TEN] = data;
		}
		// check alarm
		if (!m_todstopped && !memcmp(m_todalarm, m_todclock, sizeof(m_todalarm))) {
			trigger(INTERRUPT_ALARM);
		}
		break;
	case SDR:
		if (regs[CRA] & 0x40)
			sdr_buffered = true;
		break;
	case ICR:
		if (data & 0x80) {
			icr |= data & ~INTERRUPT_REQUEST;
			trigger(INTERRUPT_NONE);
		} else {
			icr &= ~data;
		}
		break;
	case CRA:{
		if ((data & 1) && !(oldData & 1)) {
			// Reset the underflow flipflop for the data port
			timerA.setPbToggle(true);
		}
		timerA.setControlRegister(data);
		break;}
	case CRB:{
		if ((data & 1) && !(oldData & 1)) {
			// Reset the underflow flipflop for the data port
			timerB.setPbToggle(true);
		}
		timerB.setControlRegister(data | (data & 0x40) >> 1);
		break;}
	}

	timerA.wakeUpAfterSyncWithCpu();
	timerB.wakeUpAfterSyncWithCpu();
}

void MOS6526::trigger(void) {
	idr |= INTERRUPT_REQUEST;
	interrupt(true);
	triggerScheduled = false;
}

void MOS6526::trigger(const uint8_t interruptMask) {
	idr |= interruptMask;
	if ((icr & idr) && !(idr & INTERRUPT_REQUEST)) {
		if (!triggerScheduled) {
			// Schedules an IRQ asserting state transition for next cycle.
			event_context.schedule(triggerEvent, 1, EVENT_CLOCK_PHI1);
			triggerScheduled = true;
		}
	}
}

void MOS6526::bTick(void) {
	timerB.cascade();
}

void MOS6526::underflowA(void) {
	trigger(INTERRUPT_UNDERFLOW_A);
	if ((regs[CRB] & 0x41) == 0x41) {
		if (timerB.started()) {
			event_context.schedule(bTickEvent, 0, EVENT_CLOCK_PHI2);
		}
	}
}

void MOS6526::underflowB(void) {
	trigger(INTERRUPT_UNDERFLOW_B);
}

// TOD implementation taken from Vice
#define byte2bcd(byte) (((((byte) / 10) << 4) + ((byte) % 10)) & 0xff)
#define bcd2byte(bcd)  (((10*(((bcd) & 0xf0) >> 4)) + ((bcd) & 0xf)) & 0xff)

void MOS6526::tod(void) {
	// Reload divider according to 50/60 Hz flag
	// Only performed on expiry according to Frodo
	if (regs[CRA] & 0x80)
		m_todCycles += (m_todPeriod * 5);
	else
		m_todCycles += (m_todPeriod * 6);

	// Fixed precision 25.7
	event_context.schedule(todEvent, m_todCycles >> 7);
	m_todCycles &= 0x7F; // Just keep the decimal part

	if (!m_todstopped) {
		// inc timer
		uint8_t *tod = m_todclock;
		uint8_t t = bcd2byte(*tod) + 1;
		*tod++ = byte2bcd(t % 10);
		if (t >= 10) {
			t = bcd2byte(*tod) + 1;
			*tod++ = byte2bcd(t % 60);
			if (t >= 60) {
				t = bcd2byte(*tod) + 1;
				*tod++ = byte2bcd(t % 60);
				if (t >= 60) {
					uint8_t pm = *tod & 0x80;
					t = *tod & 0x1f;
					if (t == 0x11)
						pm ^= 0x80; // toggle am/pm on 0:59->1:00 hr
					if (t == 0x12)
						t = 1;
					else if (++t == 10)
						t = 0x10;   // increment, adjust bcd
					t &= 0x1f;
					*tod = t | pm;
				}
			}
		}
		// check alarm
		if (!memcmp(m_todalarm, m_todclock, sizeof(m_todalarm))) {
			trigger(INTERRUPT_ALARM);
		}
	}
}
