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

#include "sidbuilder.h"

#include "sidemu.h"
#include <cstdio>

sidemu *sidbuilder::lock (EventContext *env, const SidConfig::model_t model)
{
    m_status = true;

    for (std::set<sidemu *>::iterator it=sidobjs.begin(); it != sidobjs.end(); ++it)
    {
        sidemu *sid = (*it);
        if (sid->lock (env))
        {
            sid->model (model);
            return sid;
        }
    }

    // Unable to locate free SID
    m_status = false;
    m_errorBuffer.assign(name ()).append("ERROR: No available SIDs to lock");
    return 0;
}

void sidbuilder::unlock (sidemu *device)
{
    std::set<sidemu *>::iterator it = sidobjs.find(device);
    if (it != sidobjs.end())
    {
        (*it)->unlock ();
    }
}

void sidbuilder::remove ()
{
    sidobjs.clear();
}
