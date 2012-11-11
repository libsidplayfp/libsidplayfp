/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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

#ifndef SIDBUILDER_H
#define SIDBUILDER_H

#include "SidConfig.h"

#include <set>
#include <string>

class sidemu;
class EventContext;

class sidbuilder
{
private:
    const char * const m_name;

protected:
    bool m_status;

    std::string m_errorBuffer;

    std::set<sidemu *> sidobjs;

protected:
    template<class Temu, typename Tparam>
    class applyParameter
    {
    protected:
        Tparam m_param;
        void (Temu::*m_method)(Tparam);

    public:
        applyParameter(void (Temu::*method)(Tparam), Tparam param)
          : m_param(param),
            m_method(method) {}
        void operator() (sidemu *e) { (static_cast<Temu*>(e)->*m_method)(m_param); }
    };

public:
    sidbuilder(const char * const name)
      : m_name(name),
        m_status (true),
        m_errorBuffer("N/A") {}
    virtual ~sidbuilder() {}

    /**
    * The number of used devices.
    *    return values: 0 none, positive is used sids
    */
    unsigned int usedDevices () const { return sidobjs.size (); }

    /// Find a free SID of the required specs
    sidemu      *lock    (EventContext *env, SidConfig::model_t model);

    /// Release this SID
    void         unlock  (sidemu *device);

    /// Remove all SID emulations.
    void         remove  (void);

    const    char        *name    (void) const { return m_name; }
    const  char *error   (void) const { return m_errorBuffer.c_str(); }

    virtual  const  char *credits (void) const = 0;
    virtual  void         filter  (bool enable) = 0;

    // Determine current state of object (true = okay, false = error).
    bool     getStatus() const { return m_status; }
};

#endif // SIDBUILDER_H
