/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2002 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef  HARDSID_H
#define  HARDSID_H

#include <vector>
#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

class sidemu;

/**
* HardSID Builder Class
*/
class SID_EXTERN HardSIDBuilder: public sidbuilder
{
private:
    static bool m_initialised;
    char   m_errorBuffer[100];
    std::vector<sidemu *> sidobjs;

#ifndef _WIN32
    static uint m_count;
#endif

    int init (void);

public:
    HardSIDBuilder  (const char * const name);
    ~HardSIDBuilder (void);

    /**
    * true will give you the number of used devices.
    *    return values: 0 none, positive is used sids
    * false will give you all available sids.
    *    return values: 0 endless, positive is available sids.
    */
    uint        devices (const bool created);
    sidemu     *lock    (EventContext *env, const sid2_model_t model);
    void        unlock  (sidemu *device);
    void        remove  (void);
    const char *error   (void) const { return m_errorBuffer; }
    const char *credits (void);
    void        flush   (void);
    void        filter  (const bool enable);

    uint        create  (uint sids);
};

#endif // HARDSID_H
