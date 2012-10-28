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

#include "residfp.h"
#include "residfp-emu.h"

ReSIDfpBuilder::ReSIDfpBuilder (const char * const name)
:sidbuilder (name)
{
    strcpy (m_errorBuffer, "N/A");
}

ReSIDfpBuilder::~ReSIDfpBuilder (void)
{   // Remove all are SID emulations
    remove ();
}

// Create a new sid emulation.
unsigned int ReSIDfpBuilder::create (unsigned int sids)
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
            sidobjs.push_back (new ReSIDfp(this));
        }
        // Memory alloc failed?
        catch (std::bad_alloc&)
        {
            sprintf (m_errorBuffer, "%s ERROR: Unable to create ReSIDfp object", name ());
            m_status = false;
            break;
        }
    }
    return count;

}

const char *ReSIDfpBuilder::credits () const
{
    return ReSIDfp::getCredits ();
}

void ReSIDfpBuilder::filter (const bool enable)
{
    for (std::vector<sidemu *>::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
    {
        ReSIDfp *sid = static_cast<ReSIDfp*>(*it);
        sid->filter (enable);
    }
}

void ReSIDfpBuilder::filter6581Curve (const double filterCurve)
{
    for (std::vector<sidemu *>::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
    {
        ReSIDfp *sid = static_cast<ReSIDfp*>(*it);
        sid->filter6581Curve (filterCurve);
    }
}

void ReSIDfpBuilder::filter8580Curve (const double filterCurve)
{
    for (std::vector<sidemu *>::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
    {
        ReSIDfp *sid = static_cast<ReSIDfp*>(*it);
        sid->filter8580Curve (filterCurve);
    }
}
