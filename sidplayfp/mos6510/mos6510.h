/***************************************************************************
                          mos6510.h  -  description
                             -------------------
    begin                : Thu May 11 2000
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
 *  $Log: mos6510.h,v $
 *  Revision 1.6  2004/05/24 23:11:10  s_a_white
 *  Fixed email addresses displayed to end user.
 *
 *  Revision 1.5  2001/07/14 16:47:21  s_a_white
 *  Sync with sidbuilder class project.
 *
 *  Revision 1.4  2001/07/14 13:04:34  s_a_white
 *  Accumulator is now unsigned, which improves code readability.
 *
 *  Revision 1.3  2000/12/11 19:03:16  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#ifndef _mos6510_h_
#define _mos6510_h_

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sidplayfp/component.h"
#include "sidplayfp/EventScheduler.h"

#include "opcodes.h"
#include "conf6510.h"

#include "cycle_based/mos6510c.h"

#ifdef MOS6510_DEBUG

class MOS6510Debug
{
public:
    static void DumpState (const event_clock_t time, const MOS6510 &cpu);
};

#endif

#endif // _mos6510_h_

