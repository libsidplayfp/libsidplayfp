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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SIDBUILDER_H
#define SIDBUILDER_H

#include "sid2types.h"
#include "component.h"
#include "event.h"


// Inherit this class to create a new SID emulation.
class sidbuilder;

class sidemu: public component
{
private:
    sidbuilder *m_builder;

protected:
    int         m_bufferpos;
    short      *m_buffer;

public:
    sidemu (sidbuilder *builder)
        : m_builder (builder) {;}
    virtual ~sidemu () {;}

    // Standard component functions
    void            reset () { reset (0); }
    virtual void    reset (uint8_t volume) = 0;
    virtual uint8_t read  (uint_least8_t addr) = 0;
    virtual void    write (uint_least8_t addr, uint8_t data) = 0;
    virtual void    clock() = 0;
    virtual const   char *credits (void) = 0;

    // Standard SID functions
    virtual void    voice   (uint_least8_t num, bool mute) = 0;
    sidbuilder     *builder (void) const { return m_builder; }

    virtual int bufferpos() const { return m_bufferpos; }
    virtual void bufferpos(const int pos) { m_bufferpos = pos; }
    virtual short *buffer() const { return m_buffer; }

    virtual void sampling(float systemfreq, float outputfreq,
    const sampling_method_t method, const bool fast) { return; }
};

class sidbuilder
{
private:
    const char * const m_name;

protected:
    bool m_status;

public:
    sidbuilder(const char * const name)
        : m_name(name), m_status (true) {;}
    virtual ~sidbuilder() {;}

    virtual  sidemu      *lock    (EventContext *env, const sid2_model_t model) = 0;
    virtual  void         unlock  (sidemu *device) = 0;
    const    char        *name    (void) const { return m_name; }
    virtual  const  char *error   (void) const = 0;
    virtual  const  char *credits (void) = 0;
    virtual  void         filter  (bool enable) = 0;

    // Determine current state of object (true = okay, false = error).
    bool     getStatus() const { return m_status; }
};

#endif // SIDBUILDER_H
