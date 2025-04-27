/***************************************************************************
      exsid-builder.cpp - exSID builder class for creating/controlling
                               exSIDs.
                               -------------------
   Based on hardsid-builder.cpp (C) 2001 Simon White

    copyright            : (C) 2015 Thibaut VARENE
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <sstream>
#include <string>

#include "properties.h"
#include "exsid.h"
#include "exsid-emu.h"


struct exSIDBuilder::config
{
    Property<bool> filterEnabled;
};

bool exSIDBuilder::m_initialised = false;
unsigned int exSIDBuilder::m_count = 0;

exSIDBuilder::exSIDBuilder(const char * const name) :
    sidbuilder(name),
    m_config(new config)
{
    if (!m_initialised)
    {
        m_count = 1;
        m_initialised = true;
    }
}

exSIDBuilder::~exSIDBuilder()
{
    // Remove all SID emulations
    remove();

    delete m_config;
}

// Create a new sid emulation.  Called by libsidplay2 only
unsigned int exSIDBuilder::create(unsigned int sids)
{
    m_status = true;

    // Check available devices
    unsigned int count = availDevices();
    if (count == 0)
    {
        m_errorBuffer.assign(name()).append(" ERROR: No devices found");
        goto exSIDBuilder_create_error;
    }

    if (count < sids)
        sids = count;

    for (count = 0; count < sids; count++)
    {
        try
        {
            std::unique_ptr<libsidplayfp::exSID> sid(new libsidplayfp::exSID(this));

            // SID init failed?
            if (!sid->getStatus())
            {
                m_errorBuffer = sid->error();
                goto exSIDBuilder_create_error;
            }

            if (m_config->filterEnabled.has_value())
                sid->filter(m_config->filterEnabled.value());
            sidobjs.insert(sid.release());
        }
        // Memory alloc failed?
        catch (std::bad_alloc const &)
        {
            m_errorBuffer.assign(name()).append(" ERROR: Unable to create exSID object");
            goto exSIDBuilder_create_error;
        }
    }

exSIDBuilder_create_error:
    if (count == 0)
        m_status = false;
    return count;
}

unsigned int exSIDBuilder::availDevices() const
{
    return m_count;
}

const char *exSIDBuilder::credits() const
{
    return libsidplayfp::exSID::getCredits();
}

void exSIDBuilder::flush()
{
    for (libsidplayfp::sidemu* e: sidobjs)
        static_cast<libsidplayfp::exSID*>(e)->flush();
}

void exSIDBuilder::filter (bool enable)
{
    m_config->filterEnabled = enable;
    for (libsidplayfp::sidemu* e: sidobjs)
        static_cast<libsidplayfp::exSID*>(e)->filter(enable);
}
