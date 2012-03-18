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

#include "sidtypes.h"

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
    class EventContext *m_context SID_DEPRECATED;

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

    /**
    * Cancel the specified event.
    *
    * @deprecated use EventContext::cancel
    */
    SID_DEPRECATED void    cancel  ();

    /**
    * Add event to pending queue.
    *
    * @deprecated use EventContext::schedule
    */
    SID_DEPRECATED void    schedule(EventContext &context, event_clock_t cycles,
                     event_phase_t phase);
};


template< class This >
class EventCallback: public Event
{
private:
    typedef void (This::*Callback) ();
    This          &m_this;
    Callback const m_callback;
    void event(void) { (m_this.*m_callback) (); }

public:
    EventCallback (const char * const name, This &_this, Callback callback)
      : Event(name), m_this(_this),
        m_callback(callback) {}
};


// Public Event Context
class EventContext
{
    friend class Event;

public:
    virtual void cancel   (Event &event) = 0;
    virtual void schedule (Event &event, const event_clock_t cycles,
                           const event_phase_t phase) = 0;
    virtual void schedule (Event &event, const event_clock_t cycles) = 0;
    virtual event_clock_t getTime (const event_phase_t phase) const = 0;
    virtual event_clock_t getTime (const event_clock_t clock, const event_phase_t phase) const = 0;
    virtual event_phase_t phase () const = 0;
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
* event to be moved to the next cycle. (This behavior may be a bug of the
* original EventScheduler specification. Likely reason is to avoid
* scheduling an event into past.)
*
* Getting a phi1 time when system is in phi2 causes the next phi1 clock
* to be returned.
*
* To make scheduling even faster, I am considering making event cancellation
* before rescheduling mandatory, as caller is generally in position to
* automatically know if the event needs to be canceled first.
*
* @author Antti S. Lankila
*/
class EventScheduler: public EventContext
{
private:
    event_clock_t  currentTime;
    Event *firstEvent;

private:
    void event (void);

    /**
    * Scan the event queue and schedule event for execution.
    *
    * @param event The event to add
    */
    void schedule(Event &event) {

        if (event.m_pending)
            cancel(event);

        event.m_pending = true;

        /* find the right spot where to tuck this new event */
        Event **scan = &firstEvent;
        for (;;) {
            if (*scan == 0 || (*scan)->triggerTime > event.triggerTime) {
                 event.next = *scan;
                 *scan = &event;
                 break;
             }
             scan = &((*scan)->next);
         }
    }

protected:
    /** Add event to pending queue.
    *
    * @param event the event to add
    * @param cycles the clock to fire
    * @param phase when to fire the event
    */
    void schedule (Event &event, const event_clock_t cycles,
                   const event_phase_t phase) {
        // this strange formulation always selects the next available slot regardless of specified phase.
        event.triggerTime = (cycles << 1) + currentTime + ((currentTime & 1) ^ phase);
        schedule(event);
    }

    /** Add event to pending queue in the same phase as current event.
    *
    * @param event the event to add
    * @param cycles how many cycles from now to fire
    */
    void schedule(Event &event, const event_clock_t cycles) {
        event.triggerTime = (cycles << 1) + currentTime;
        schedule(event);
    }

    /** Cancel the specified event.
    *
    * @param event the event to cancel
    */
    void cancel   (Event &event);

public:
    EventScheduler ()
        : currentTime(0),
          firstEvent(0) {}

    /** Cancel all pending events and reset time. */
    void reset     (void);

    /** Fire next event, advance system time to that event */
    void clock (void)
    {
        Event &event = *firstEvent;
        firstEvent = firstEvent->next;
        event.m_pending = false;
        currentTime = event.triggerTime;
        event.event();
    }

    /** Get time with respect to a specific clock phase
    *
    * @param phase the phase
    * @return the time according to specified phase.
    */
    event_clock_t getTime (const event_phase_t phase) const
    {   return (currentTime + (phase ^ 1)) >> 1; }

    /** Get clocks since specified clock in given phase.
    *
    * @param clock the time to compare to
    * @param phase the phase to comapre to
    * @return the time between specified clock and now
    */
    event_clock_t getTime (const event_clock_t clock, const event_phase_t phase) const
    {   return getTime (phase) - clock; }

    /** Return current clock phase
    *
    * @return The current phase
    */
    event_phase_t phase () const { return (event_phase_t) (currentTime & 1); }
};


inline void Event::schedule (EventContext &context, event_clock_t cycles,
                             event_phase_t phase)
{
    m_context = &context;
    m_context->schedule (*this, cycles, phase);
}

inline void Event::cancel ()
{
    if (m_pending)
        m_context->cancel (*this);
}

#endif // _event_h_
