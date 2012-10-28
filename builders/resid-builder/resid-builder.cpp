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

// Create a new sid emulation.
unsigned int ReSIDBuilder::create (unsigned int sids)
{
    m_status = true;

    // Check available devices
    unsigned int count = availDevices ();

    if (count && (count < sids))
        sids = count;

    for (count = 0; count < sids; count++)
    {
        try
        {
            sidobjs.push_back (new ReSID(this));
        }
        // Memory alloc failed?
        catch (std::bad_alloc&)
        {
            sprintf (m_errorBuffer, "%s ERROR: Unable to create ReSID object", name ());
            m_error = m_errorBuffer;
            m_status = false;
            break;
        }
    }
    return count;
}

const char *ReSIDBuilder::credits () const
{
    return ReSID::getCredits ();
}

void ReSIDBuilder::filter (const bool enable)
{
    for (std::vector<sidemu *>::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
    {
        ReSID *sid = static_cast<ReSID*>(*it);
        sid->filter (enable);
    }
}

void ReSIDBuilder::bias (const double dac_bias)
{
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
            sid->unlock ();
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
