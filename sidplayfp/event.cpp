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

void EventScheduler::cancel (Event &event)
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
