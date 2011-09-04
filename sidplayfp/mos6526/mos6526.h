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

#ifndef MOS6526_H
#define MOS6526_H

#include "sidplayfp/component.h"
#include "sidplayfp/event.h"

class MOS6526;

/** @internal
* This class implements a Timer A or B of a MOS6526 chip.
*
* @author Ken Händel
*
*/
class Timer : private Event {

protected:
	static const int_least32_t CIAT_CR_START   = 0x01;
	static const int_least32_t CIAT_STEP       = 0x04;
	static const int_least32_t CIAT_CR_ONESHOT = 0x08;
	static const int_least32_t CIAT_CR_FLOAD   = 0x10;
	static const int_least32_t CIAT_PHI2IN     = 0x20;
	static const int_least32_t CIAT_CR_MASK    = CIAT_CR_START | CIAT_CR_ONESHOT | CIAT_CR_FLOAD | CIAT_PHI2IN;

	static const int_least32_t CIAT_COUNT2     = 0x100;
	static const int_least32_t CIAT_COUNT3     = 0x200;

	static const int_least32_t CIAT_ONESHOT0   = 0x08 << 8;
	static const int_least32_t CIAT_ONESHOT    = 0x08 << 16;
	static const int_least32_t CIAT_LOAD1      = 0x10 << 8;
	static const int_least32_t CIAT_LOAD       = 0x10 << 16;

	static const int_least32_t CIAT_OUT        = 0x80000000;

private:
	/**
	* Event context.
	*/
	EventContext &event_context;

	/**
	* This is a tri-state:
	*
	* when -1: cia is completely stopped
	* when 0: cia 1-clock events are ticking.
	* otherwise: cycleskipevent is ticking, and the value is the first
	* phi1 clock of skipping.
	*/
	event_clock_t ciaEventPauseTime;

	/**
	* Copy of regs[CRA/B]
	*/
	uint8_t lastControlValue;

	/**
	* Current timer value.
	*/
	uint_least16_t timer;

	/**
	* Timer start value (Latch).
	*/
	uint_least16_t latch;

	/**
	* PB6/PB7 Flipflop to signal underflows.
	*/
	bool pbToggle;

	EventCallback<Timer> m_cycleSkippingEvent;

protected:
	/**
	* Pointer to the MOS6526 which this Timer belongs to.
	*/
	MOS6526* parent;

	/**
	* CRA/CRB control register / state.
	*/
	int_least32_t state;

private:
	/**
	* Perform scheduled cycle skipping, and resume.
	*/
	void cycleSkippingEvent(void);

	/**
	* Execute one CIA state transition.
	*/
	void clock();

	/**
	* Reschedule CIA event at the earliest interesting time.
	* If CIA timer is stopped or is programmed to just count down,
	* the events are paused.
	*/
	inline void reschedule();

	/**
	* Timer ticking event.
	*/
	void event();

	/**
	* Signal timer underflow.
	*/
	virtual void underFlow() =0;

	/**
	* Handle the serial port.
	*/
	virtual void serialPort() {};

protected:
	/**
	* Create a new timer.
	*
	* @param name component name
	* @param context event context
	* @param parent the MOS6526 which this Timer belongs to
	*/
	Timer(const char* name, EventContext *context, MOS6526* parent) :
		Event(name),
		event_context(*context),
		lastControlValue(0),
		timer(0),
		latch(0),
		pbToggle(false),
		m_cycleSkippingEvent("Skip CIA clock decrement cycles", *this, &Timer::cycleSkippingEvent),
		parent(parent),
		state(0) {}

public:
	/**
	* Set CRA/CRB control register.
	*
	* @param cr
	*            control register value
	*/
	void setControlRegister(const uint8_t cr);

	/**
	* Perform cycle skipping manually.
	*
	* Clocks the CIA up to the state it should be in, and stops all events.
	*/
	void syncWithCpu();

	/**
	* Counterpart of syncWithCpu(),
	* starts the event ticking if it is needed.
	* No clock() call or anything such is permissible here!
	*/
	void wakeUpAfterSyncWithCpu();

	/**
	* Reset timer.
	*/
	void reset();

	/**
	* Set low byte of Timer start value (Latch).
	*
	* @param data
	*            low byte of latch
	*/
	void latchLo(const uint8_t data);

	/**
	* Set high byte of Timer start value (Latch).
	*
	* @param data
	*            high byte of latch
	*/
	void latchHi(const uint8_t data);

	/**
	* Set PB6/PB7 Flipflop state.
	*
	* @param state
	*            PB6/PB7 flipflop state
	*/
	inline void setPbToggle(const bool state) { pbToggle = state; }

	/**
	* Get current state value.
	*
	* @return current state value
	*/
	inline int_least32_t getState() const { return state; }

	/**
	* Get current timer value.
	*
	* @return current timer value
	*/
	inline uint_least16_t getTimer() const { return timer; }

	/**
	* Get PB6/PB7 Flipflop state.
	*
	* @param reg value of the control register
	* @return PB6/PB7 flipflop state
	*/
	inline bool getPb(const uint8_t reg) const { return (reg & 0x04) ? pbToggle : (state & CIAT_OUT); }
};

/** @internal
* This is the timer A of this CIA.
*
* @author Ken Händel
*
*/
class TimerA : public Timer {
private:
	/**
	* Signal underflows of Timer A to Timer B.
	*/
	void underFlow();

	void serialPort();

public:
	/**
	* Create timer A.
	*/
	TimerA(EventContext *context, MOS6526* parent) :
		Timer("CIA Timer A", context, parent) {}
};

/** @internal
* This is the timer B of this CIA.
*
* @author Ken Händel
*
*/
class TimerB : public Timer {
private:
	void underFlow();

public:
	/**
	* Create timer B.
	*/
	TimerB(EventContext *context, MOS6526* parent) :
		Timer("CIA Timer B", context, parent) {}

	/**
	* Receive an underflow from Timer A.
	*/
	void cascade() {
		/* we pretend that we are CPU doing a write to ctrl register */
		syncWithCpu();
		state |= CIAT_STEP;
		wakeUpAfterSyncWithCpu();
	}

	/**
	* Check if start flag is set.
	*
	* @return true if start flag is set, false otherwise
	*/
	bool started() const { return (state & CIAT_CR_START) != 0; }
};

/** @internal
 * This class is heavily based on the ciacore/ciatimer source code from VICE.
 * The CIA state machine is lifted as-is. Big thanks to VICE project!
 *
 * @author alankila
 */
class MOS6526: public component
{
	friend class TimerA;
	friend class TimerB;

private:
	static const char *credit;

protected:
	/**
	* These are all CIA registers.
	*/
	uint8_t regs[0x10];

	// Ports
	uint8_t &pra, &prb, &ddra, &ddrb;

	/**
	* Timers A and B.
	*/
	TimerA timerA;
	TimerB timerB;

	// Serial Data Registers
	uint8_t sdr_out;
	bool    sdr_buffered;
	int     sdr_count;

	/** Interrupt control register */
	uint8_t icr;

	/** Interrupt data register */
	uint8_t idr;

	/**
	* Event context.
	*/
	EventContext &event_context;

	bool    m_todlatched;
	bool    m_todstopped;
	uint8_t m_todclock[4], m_todalarm[4], m_todlatch[4];
	event_clock_t m_todCycles, m_todPeriod;

	/** Have we already scheduled CIA->CPU interrupt transition? */
	bool triggerScheduled;


	EventCallback<MOS6526> bTickEvent;
	EventCallback<MOS6526> todEvent;
	EventCallback<MOS6526> triggerEvent;

protected:
	/**
	* Create a new CIA.
	* @param ctx the event context
	*/
	MOS6526(EventContext *context);

	/**
	* This event exists solely to break the ambiguity of what scheduling on
	* top of PHI1 causes, because there is no ordering between events on
	* same phase. Thus it is scheduled in PHI2 to ensure the b.event() is
	* run once before the value changes.
	* <UL>
	* <LI>PHI1 a.event() (which calls underFlow())
	* <LI>PHI1 b.event()
	* <LI>PHI2 bTick.event()
	* <LI>PHI1 a.event()
	* <LI>PHI1 b.event()
	* </UL>
	*/
	void bTick(void);

	void tod(void);

	/**
	* Signal interrupt to CPU.
	*/
	void trigger(void);

	/**
	* Timer A underflow 
	*/
	void underflowA(void);

	/**
	* Timer B underflow.
	*/
	void underflowB(void);

	/**
	* Trigger an interrupt.
	*
	* @param interruptMask Interrupt flag number
	*/
	void trigger(const uint8_t interruptMask);

	void serialPort();

	/**
	* Signal interrupt.
	*
	* @param state
	*            interrupt state
	*/
	virtual void interrupt(bool state) = 0;

	virtual void portA() {}
	virtual void portB() {}

public:
	/**
	* Reset CIA.
	*/
	virtual void reset(void);

	/**
	* Read CIA register.
	*
	* @param addr
	*            register address to read (lowest 4 bits)
	*/
	uint8_t read(uint_least8_t addr);

	/**
	* Write CIA register.
	*
	* @param addr
	*            register address to write (lowest 4 bits)
	* @param data
	*            value to write
	*/
	void write(uint_least8_t addr, uint8_t data);

	/**
	* Get the credits.
	*
	* @return the credits
	*/
	const char *credits(void) { return credit; }

	void clear(void);

	/**
	* Set day-of-time event occurence of rate.
	*
	* @param clock
	*/
	void clock(float64_t clock);
};

#endif // MOS6526_H
