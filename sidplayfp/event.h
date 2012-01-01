/***************************************************************************
                          event.h  -  Event scheduler (based on alarm
                                      from Vice)
                             -------------------
    begin                : Wed May 9 2001
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

#ifndef _event_h_
#define _event_h_

#include <stdint.h>

#include "sidconfig.h"

typedef int_fast64_t event_clock_t;

/**
* C64 system runs actions at system clock high and low
* states. The PHI1 corresponds to the auxiliary chip activity
* and PHI2 to CPU activity. For any clock, PHI1s are before
* PHI2s.
*/
typedef enum {EVENT_CLOCK_PHI1 = 0, EVENT_CLOCK_PHI2 = 1} event_phase_t;


/**
* Event scheduler (based on alarm from Vice). Created in 2001 by Simon A.
* White.
*
* Optimized EventScheduler and corresponding Event class by Antti S. Lankila
* in 2009.
*
* @author Antti Lankila
*/
class SID_EXTERN Event
{
    friend class EventScheduler;

private:
    /** Describe event for humans. */
    const char * const m_name;

    /** The clock this event fires */
    event_clock_t triggerTime;

    /**
    * This variable is set by the event context
    * when it is scheduled
    */
    bool m_pending;

    /** The next event in sequence */
    Event *next;

public:
    /**
    * Events are used for delayed execution. Name is
    * not used by code, but is useful for debugging.
    *
    * @param name Descriptive string of the event.
    */
    Event(const char * const name)
        : m_name(name),
          m_pending(false) {}
    ~Event() {}

    /**
    * Event code to be executed. Events are allowed to safely
    * reschedule themselves with the EventScheduler during
    * invocations.
    */
    virtual void event (void) = 0;

    /** Is Event scheduled? */
    bool    pending () const { return m_pending; }
};

/**
* Fast EventScheduler, which maintains a linked list of Events.
* This scheduler takes neglible time even when it is used to
* schedule events for nearly every clock.
*
* Events occur on an internal clock which is 2x the visible clock.
* The visible clock is divided to two phases called phi1 and phi2.
*
* The phi1 clocks are used by VIC and CIA chips, phi2 clocks by CPU.
*
* Scheduling an event for a phi1 clock when system is in phi2 causes the
* event to be moved to the next phi1 cycle. Correspondingly, requesting 
* a phi1 time when system is in phi2 returns the value of the next phi1.
*
* @author Antti S. Lankila
*/
class EventContext
{
public:
    /** Cancel the specified event.
    *
    * @param event the event to cancel
    */
    virtual void cancel   (Event &event) = 0;

    /** Add event to pending queue.
    *
    * At PHI2, specify cycles=0 and Phase=PHI1 to fire on the very next PHI1.
    *
    * @param event the event to add
    * @param cycles how many cycles from now to fire
    * @param phase the phase when to fire the event
    */
    virtual void schedule (Event &event, const event_clock_t cycles,
                           const event_phase_t phase) = 0;

    /** Add event to pending queue in the same phase as current event.
    *
    * @param event the event to add
    * @param cycles how many cycles from now to fire
    */
    virtual void schedule (Event &event, const event_clock_t cycles) = 0;

    /** Get time with respect to a specific clock phase
    *
    * @param phase the phase
    * @return the time according to specified phase.
    */
    virtual event_clock_t getTime (const event_phase_t phase) const = 0;

    /** Get clocks since specified clock in given phase.
    *
    * @param clock the time to compare to
    * @param phase the phase to comapre to
    * @return the time between specified clock and now
    */
    virtual event_clock_t getTime (const event_clock_t clock, const event_phase_t phase) const = 0;

    /** Return current clock phase
    *
    * @return The current phase
    */
    virtual event_phase_t phase () const = 0;
};

#endif // _event_h_
