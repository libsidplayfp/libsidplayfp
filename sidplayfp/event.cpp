/***************************************************************************
                          event->cpp  -  Event schdeduler (based on alarm
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
/***************************************************************************
 *  $Log: event->cpp,v $
 *  Revision 1.12  2004/06/26 10:52:29  s_a_white
 *  Support new event template class and add minor code improvements.
 *
 *  Revision 1.11  2003/02/20 19:10:16  s_a_white
 *  Code simplification.
 *
 *  Revision 1.10  2003/01/24 19:30:39  s_a_white
 *  Made code slightly more efficient.  Changes to this code greatly effect
 *  sidplay2s performance.  Somehow need to speed up the schedule routine.
 *
 *  Revision 1.9  2003/01/23 17:32:37  s_a_white
 *  Redundent code removal.
 *
 *  Revision 1.8  2003/01/17 08:35:46  s_a_white
 *  Event scheduler phase support.
 *
 *  Revision 1.7  2002/11/21 19:55:38  s_a_white
 *  We now jump to next event directly instead on clocking by a number of
 *  cycles.
 *
 *  Revision 1.6  2002/07/17 19:20:03  s_a_white
 *  More efficient event handling code.
 *
 *  Revision 1.5  2001/10/02 18:24:09  s_a_white
 *  Updated to support safe scheduler interface.
 *
 *  Revision 1.4  2001/09/17 19:00:28  s_a_white
 *  Constructor moved out of line.
 *
 *  Revision 1.3  2001/09/15 13:03:50  s_a_white
 *  timeWarp now zeros m_eventClk instead of m_pendingEventClk which
 *  fixes a inifinite loop problem when driving libsidplay1.
 *
 ***************************************************************************/

#include "event.h"


void EventScheduler::reset (void)
{
    Event *scan = firstEvent;
    while (scan) {
        scan->m_pending = false;
        scan = scan->next;
    }
    currentTime = 0;
    firstEvent = 0;
}

void EventScheduler::cancel   (Event &event)
{
    event.m_pending = false;
    Event *scan = firstEvent;
    Event *prev = 0;
    while (scan) {
        if (&event == scan) {
            if (prev)
                prev->next = scan->next;
            else
                firstEvent = scan->next;
            break;
        }
        prev = scan;
        scan = scan->next;
    }
}

void EventScheduler::schedule (Event &event, const event_clock_t cycles,
                               const event_phase_t phase)
{
    if (event.m_pending)
        cancel(event);

    const event_clock_t tt = event.triggerTime = (cycles << 1) + currentTime + ((currentTime & 1) ^ (phase == EVENT_CLOCK_PHI1 ? 0 : 1));

    event.m_pending = true;

    /* find the right spot where to tuck this new event */
    Event **scan = &firstEvent;
    for (;;) {
        if (*scan == 0 || (*scan)->triggerTime > tt) {
             event.next = *scan;
             *scan = &event;
             break;
         }
         scan = &((*scan)->next);
     }

}
