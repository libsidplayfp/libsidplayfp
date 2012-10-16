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

#include "residfp.h"
#include "residfp-emu.h"

ReSIDfpBuilder::ReSIDfpBuilder (const char * const name)
:sidbuilder (name)
{
    m_error = "N/A";
}

ReSIDfpBuilder::~ReSIDfpBuilder (void)
{   // Remove all are SID emulations
    remove ();
}

// Create a new sid emulation.  Called by libsidplay2 only
unsigned int ReSIDfpBuilder::create (unsigned int sids)
{
    ReSIDfp *sid = 0;
    m_status   = true;

    // Check available devices
    unsigned int count = devices (false);
    if (!m_status)
        goto ReSIDfpBuilder_create_error;
    if (count && (count < sids))
        sids = count;

    for (count = 0; count < sids; count++)
    {
#   ifdef HAVE_EXCEPTIONS
        sid = new(std::nothrow) ReSIDfp(this);
#   else
        sid = new ReSIDfp(this);
#   endif

        // Memory alloc failed?
        if (!sid)
        {
            sprintf (m_errorBuffer, "%s ERROR: Unable to create ReSIDfp object", name ());
            m_error = m_errorBuffer;
            goto ReSIDfpBuilder_create_error;
        }

        // SID init failed?
        if (!sid->getStatus())
        {
            m_error = sid->error ();
            goto ReSIDfpBuilder_create_error;
        }
        sidobjs.push_back (sid);
    }
    return count;

ReSIDfpBuilder_create_error:
    m_status = false;
    delete sid;
    return count;
}

const char *ReSIDfpBuilder::credits ()
{
    m_status = true;

    // Available devices
    if (!sidobjs.empty ())
    {
        ReSIDfp *sid = static_cast<ReSIDfp*>(sidobjs[0]);
        return sid->credits ();
    }

    {   // Create an emulation to obtain credits
        ReSIDfp sid(this);
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


unsigned int ReSIDfpBuilder::devices (const bool created)
{
    m_status = true;
    if (created)
        return sidobjs.size ();
    else // Available devices
        return 0;
}

void ReSIDfpBuilder::filter (const bool enable)
{
    const int size = sidobjs.size ();
    m_status = true;
    for (int i = 0; i < size; i++)
    {
        ReSIDfp *sid = static_cast<ReSIDfp*>(sidobjs[i]);
        sid->filter (enable);
    }
}

void ReSIDfpBuilder::filter6581Curve (const double filterCurve)
{
    const int size = sidobjs.size ();
    m_status = true;
    for (int i = 0; i < size; i++)
    {
        ReSIDfp *sid = static_cast<ReSIDfp*>(sidobjs[i]);
        sid->filter6581Curve (filterCurve);
    }
}

void ReSIDfpBuilder::filter8580Curve (const double filterCurve)
{
    const int size = sidobjs.size ();
    m_status = true;
    for (int i = 0; i < size; i++)
    {
        ReSIDfp *sid = static_cast<ReSIDfp*>(sidobjs[i]);
        sid->filter8580Curve (filterCurve);
    }
}

// Find a free SID of the required specs
sidemu *ReSIDfpBuilder::lock (EventContext *env, const SidConfig::model_t model)
{
    const int size = sidobjs.size ();
    m_status = true;

    for (int i = 0; i < size; i++)
    {
        ReSIDfp *sid = static_cast<ReSIDfp*>(sidobjs[i]);
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
void ReSIDfpBuilder::unlock (sidemu *device)
{
    const int size = sidobjs.size ();
    // Maek sure this is our SID
    for (int i = 0; i < size; i++)
    {
        ReSIDfp *sid = static_cast<ReSIDfp*>(sidobjs[i]);
        if (sid == device)
        {   // Unlock it
            sid->lock (0);
            break;
        }
    }
}

// Remove all SID emulations.
void ReSIDfpBuilder::remove ()
{
    const int size = sidobjs.size ();
    for (int i = 0; i < size; i++)
        delete sidobjs[i];
    sidobjs.clear();
}
