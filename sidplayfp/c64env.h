/***************************************************************************
                          c64env.h  -  The C64 environment interface.
                             -------------------
    begin                : Fri Apr 4 2001
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

#ifndef _c64env_h_
#define _c64env_h_

#include "event.h"
 
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

/* An implementation of of this class can be created to perform the C64
   specifics.  A pointer to this child class can then be passed to
   each of the conponents so they can interact with it.
*/

class c64env
{
private:
    EventContext &m_context;

public:
    c64env (EventContext *context)
        :m_context (*context) {}

    EventContext &context (void) const { return m_context; }

    virtual uint8_t cpuRead(const uint_least16_t addr) =0;
    virtual void cpuWrite(const uint_least16_t addr, const uint8_t data) =0;

#ifdef PC64_TESTSUITE
    virtual void   loadFile (const char *file) =0;
#endif

    virtual void interruptIRQ (const bool state) = 0;
    virtual void interruptNMI () = 0;
    virtual void interruptRST () = 0;

    virtual void signalAEC    (const bool state) = 0;
    virtual void lightpen     () = 0;
};

#endif // _c64env_h_
