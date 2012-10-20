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
unsigned int ReSIDBuilder::create (unsigned int sids)
{
    ReSID *sid = 0;
    m_status   = true;

    // Check available devices
    unsigned int count = devices (false);
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
            m_error = m_errorBuffer;
            return 0;
        }
        return sid.credits ();
    }
}


unsigned int ReSIDBuilder::devices (const bool created)
{
    m_status = true;
    if (created)
        return sidobjs.size ();
    else // Available devices
        return 0;
}

void ReSIDBuilder::filter (const bool enable)
{
    m_status = true;

    for (std::vector<sidemu *>::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
    {
        ReSID *sid = static_cast<ReSID*>(*it);
        sid->filter (enable);
    }
}

void ReSIDBuilder::bias (const double dac_bias)
{
    m_status = true;

    for (std::vector<sidemu *>::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
    {
        ReSID *sid = static_cast<ReSID*>(*it);
        sid->bias (dac_bias);
    }
}

// Find a free SID of the required specs
sidemu *ReSIDBuilder::lock (EventContext *env, const SidConfig::model_t model)
{
    m_status = true;

    for (std::vector<sidemu *>::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
    {
        ReSID *sid = static_cast<ReSID*>(*it);
        if (sid->lock (env))
        {
            sid->model (model);
            return sid;
        }
    }
    // Unable to locate free SID
    m_status = false;
    sprintf (m_errorBuffer, "%s ERROR: No available SIDs to lock", name ());
    m_error = m_errorBuffer;
    return 0;
}

// Allow something to use this SID
void ReSIDBuilder::unlock (sidemu *device)
{
    // Make sure this is our SID
    for (std::vector<sidemu *>::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
    {
        ReSID *sid = static_cast<ReSID*>(*it);
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
    for (std::vector<sidemu *>::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
        delete (*it);

    sidobjs.clear();
}
