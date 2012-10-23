/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "resid/sid.h"
#include "sidplayfp/sidemu.h"
#include "sidplayfp/EventScheduler.h"

#include <string>

//FIXME
#define RESID_NAMESPACE reSID

#ifdef RESID_NAMESPACE
#   define RESID_NS ::RESID_NAMESPACE
#else
#   define RESID_NS
#endif

enum {
    OUTPUTBUFFERSIZE = 32768
};

class ReSID: public sidemu
{
private:
    EventContext *m_context;
    event_phase_t m_phase;
    RESID_NS::SID &m_sid;
    event_clock_t m_accessClk;
    const  char  *m_error;
    bool          m_status;
    bool          m_locked;
    uint8_t       m_voiceMask;

    static std::string   m_credit;

public:
    static const char* getCredits();

public:
    ReSID  (sidbuilder *builder);
    ~ReSID (void);

    // Standard component functions
    const char   *credits () const { return getCredits(); }

    void          reset   () { sidemu::reset (); }
    void          reset   (uint8_t volume);
    uint8_t       read    (uint_least8_t addr);
    void          write   (uint_least8_t addr, uint8_t data);
    const char   *error   (void) const {return m_error;}

    // Standard SID functions
    void          clock   ();
    void          filter  (bool enable);
    void          voice   (const unsigned int num, const bool mute);

    bool          getStatus() const { return m_status; }

    // Specific to resid
    void sampling (float systemclock, float freq,
        const SidConfig::sampling_method_t method, const bool fast);

    void bias     (const double dac_bias);
    void model    (SidConfig::model_t model);
    // Must lock the SID before using the standard functions.
    bool lock     (EventContext *env);
};
