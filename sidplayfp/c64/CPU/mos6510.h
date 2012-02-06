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


#ifndef MOS6510DEBUG_H
#define MOS6510DEBUG_H

#include "cycle_based/mos6510c.h"

#ifdef DEBUG

class MOS6510Debug
{
public:
    static void DumpState (const event_clock_t time, MOS6510 &cpu);
};

#endif

#endif // MOS6510DEBUG_H

