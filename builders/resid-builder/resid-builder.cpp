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

#include <stdio.h>
#include <cstring>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#include "resid.h"
#include "resid-emu.h"

ReSIDBuilder::ReSIDBuilder (const char * const name)
:sidbuilder (name)
{
    m_error = "N/A";
}

ReSIDBuilder::~ReSIDBuilder (void)
{   // Remove all are SID emulations
    remove ();
}

// Create a new sid emulation.  Called by libsidplay2 only
uint ReSIDBuilder::create (uint sids)
{
    ReSID *sid = 0;
    m_status   = true;

    // Check available devices
    uint count = devices (false);
    if (!m_status)
        goto ReSIDBuilder_create_error;
    if (count && (count < sids))
        sids = count;

    for (count = 0; count < sids; count++)
    {
#   ifdef HAVE_EXCEPTIONS
        sid = new(std::nothrow) ReSID(this);
#   else
        sid = new ReSID(this);
#   endif

        // Memory alloc failed?
        if (!sid)
        {
            sprintf (m_errorBuffer, "%s ERROR: Unable to create ReSID object", name ());
            m_error = m_errorBuffer;
            goto ReSIDBuilder_create_error;
        }

        // SID init failed?
        if (!sid->getStatus())
        {
            m_error = sid->error ();
            goto ReSIDBuilder_create_error;
        }
        sidobjs.push_back (sid);
    }
    return count;

ReSIDBuilder_create_error:
    m_status = false;
    delete sid;
    return count;
}

const char *ReSIDBuilder::credits ()
{
    m_status = true;

    // Available devices
    if (!sidobjs.empty ())
    {
        ReSID *sid = static_cast<ReSID*>(sidobjs[0]);
        return sid->credits ();
    }

    {   // Create an emulation to obtain credits
        ReSID sid(this);
        if (!sid.getStatus())
        {
            m_status = false;
            strcpy (m_errorBuffer, sid.error ());
            return 0;
        }
        return sid.credits ();
    }
}


uint ReSIDBuilder::devices (const bool created)
{
    m_status = true;
    if (created)
        return sidobjs.size ();
    else // Available devices
        return 0;
}

void ReSIDBuilder::filter (const bool enable)
{
    const int size = sidobjs.size ();
    m_status = true;
    for (int i = 0; i < size; i++)
    {
        ReSID *sid = static_cast<ReSID*>(sidobjs[i]);
        sid->filter (enable);
    }
}

void ReSIDBuilder::bias (const double dac_bias)
{
    const int size = sidobjs.size ();
    m_status = true;
    for (int i = 0; i < size; i++)
    {
        ReSID *sid = static_cast<ReSID*>(sidobjs[i]);
        sid->bias (dac_bias);
    }
}

// Find a free SID of the required specs
sidemu *ReSIDBuilder::lock (EventContext *env, const sid2_model_t model)
{
    const int size = sidobjs.size ();
    m_status = true;

    for (int i = 0; i < size; i++)
    {
        ReSID *sid = static_cast<ReSID*>(sidobjs[i]);
        if (sid->lock (env))
        {
            sid->model (model);
            return sid;
        }
    }
    // Unable to locate free SID
    m_status = false;
    sprintf (m_errorBuffer, "%s ERROR: No available SIDs to lock", name ());
    return 0;
}

// Allow something to use this SID
void ReSIDBuilder::unlock (sidemu *device)
{
    const int size = sidobjs.size ();
    // Maek sure this is our SID
    for (int i = 0; i < size; i++)
    {
        ReSID *sid = static_cast<ReSID*>(sidobjs[i]);
        if (sid == device)
        {   // Unlock it
            sid->lock (0);
            break;
        }
    }
}

// Remove all SID emulations.
void ReSIDBuilder::remove ()
{
    const int size = sidobjs.size ();
    for (int i = 0; i < size; i++)
        delete sidobjs[i];
    sidobjs.clear();
}
