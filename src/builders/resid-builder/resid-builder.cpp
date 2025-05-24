/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "resid.h"

#include <algorithm>
#include <new>

#include "properties.h"
#include "resid-emu.h"

struct ReSIDBuilder::config
{
    Property<double> bias;
    Property<bool> filterEnabled;
};

ReSIDBuilder::ReSIDBuilder(const char * const name) :
    sidbuilder(name),
    m_config(new config) {}

ReSIDBuilder::~ReSIDBuilder()
{   // Remove all SID emulations
    remove();

    delete m_config;
}

// Create a new sid emulation.
unsigned int ReSIDBuilder::create(unsigned int sids)
{
    m_status = true;

    // Check available devices
    unsigned int count = availDevices();

    if (count && (count < sids))
        sids = count;

    for (count = 0; count < sids; count++)
    {
        try
        {
            auto sid = new libsidplayfp::ReSID(this);
            if (m_config->bias.has_value())
                sid->bias(m_config->bias.value());
            if (m_config->filterEnabled.has_value())
                sid->filter(m_config->filterEnabled.value());
            sidobjs.insert(sid);
        }
        // Memory alloc failed?
        catch (std::bad_alloc const &)
        {
            m_errorBuffer.assign(name()).append(" ERROR: Unable to create ReSID object");
            m_status = false;
            break;
        }
    }
    return count;
}

const char *ReSIDBuilder::credits() const
{
    return libsidplayfp::ReSID::getCredits();
}

void ReSIDBuilder::filter(bool enable)
{
    m_config->filterEnabled = enable;
    for (libsidplayfp::sidemu* e: sidobjs)
        static_cast<libsidplayfp::ReSID*>(e)->filter(enable);
}

void ReSIDBuilder::bias(double dac_bias)
{
    m_config->bias = dac_bias;
    for (libsidplayfp::sidemu* e: sidobjs)
        static_cast<libsidplayfp::ReSID*>(e)->bias(dac_bias);
}
