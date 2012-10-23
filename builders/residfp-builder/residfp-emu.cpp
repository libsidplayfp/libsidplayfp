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

#include <cstring>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#include "residfp/Filter6581.h"
#include "residfp/Filter8580.h"


#include "residfp/siddefs-fp.h"
#include "residfp.h"
#include "residfp-emu.h"

#include <sstream>

std::string ReSIDfp::m_credit;

const char* ReSIDfp::getCredits()
{
    if (m_credit.empty())
    {
        // Setup credits
        std::ostringstream ss;
        ss << "ReSIDfp V" << VERSION << " Engine:\n";
        ss << "\t(C) 1999-2002 Simon White\n";
        ss << "MOS6581 (SID) Emulation (ReSIDfp V" << residfp_version_string << "):\n";
        ss << "\t(C) 1999-2002 Dag Lem\n";
        ss << "\t(C) 2005-2011 Antti S. Lankila\n";
        m_credit = ss.str();
    }

    return m_credit.c_str();
}

ReSIDfp::ReSIDfp (sidbuilder *builder)
:sidemu(builder),
 m_context(0),
#ifdef HAVE_EXCEPTIONS
 m_sid(*(new(std::nothrow) RESID_NAMESPACE::SID)),
#else
 m_sid(*(new RESID_NAMESPACE::SID)),
#endif
 m_status(true),
 m_locked(false)
{
    m_error = "N/A";

    if (!&m_sid)
    {
        m_error  = "RESID ERROR: Unable to create sid object";
        m_status = false;
        return;
    }

    m_buffer = new short[OUTPUTBUFFERSIZE];
    m_bufferpos = 0;
    reset (0);
}

ReSIDfp::~ReSIDfp ()
{
    if (&m_sid)
        delete &m_sid;
    delete[] m_buffer;
}

void ReSIDfp::filter6581Curve (const double filterCurve)
{
   m_sid.getFilter6581()->setFilterCurve(filterCurve);
}

void ReSIDfp::filter8580Curve (const double filterCurve)
{
   m_sid.getFilter8580()->setFilterCurve(filterCurve);
}

// Standard component options
void ReSIDfp::reset (uint8_t volume)
{
    m_accessClk = 0;
    m_sid.reset ();
    m_sid.write (0x18, volume);
}

uint8_t ReSIDfp::read (uint_least8_t addr)
{
    clock();
    return m_sid.read (addr);
}

void ReSIDfp::write (uint_least8_t addr, uint8_t data)
{
    clock();
    m_sid.write (addr, data);
}

void ReSIDfp::clock()
{
    const int cycles = m_context->getTime(m_accessClk, EVENT_CLOCK_PHI1);
    m_accessClk += cycles;
    m_bufferpos += m_sid.clock(cycles, m_buffer+m_bufferpos);
}

void ReSIDfp::filter (bool enable)
{
      m_sid.getFilter6581()->enable(enable);
      m_sid.getFilter8580()->enable(enable);
}

void ReSIDfp::sampling (float systemclock, float freq,
        const SidConfig::sampling_method_t method, const bool fast)
{
    reSIDfp::SamplingMethod sampleMethod;
    switch (method)
    {
    case SidConfig::INTERPOLATE:
        sampleMethod = reSIDfp::DECIMATE;
        break;
    case SidConfig::RESAMPLE_INTERPOLATE:
        sampleMethod = reSIDfp::RESAMPLE;
        break;
    default:
        m_status = false;
        m_error = "Invalid sampling method.";
        return;
    }

    try
    {
      const int halfFreq = 5000*((int)freq/10000);
      m_sid.setSamplingParameters (systemclock, sampleMethod, freq, halfFreq>20000?20000:halfFreq);
    }
    catch (RESID_NAMESPACE::SIDError& e)
    {
        m_status = false;
        m_error = "Unable to set desired output frequency.";
    }
}

// Set execution environment and lock sid to it
bool ReSIDfp::lock (EventContext *env)
{
    if (!env)
    {
        if (!m_locked)
            return false;
        m_locked  = false;
        m_context = 0;
    }
    else
    {
        if (m_locked)
            return false;
        m_locked  = true;
        m_context = env;
    }
    return true;
}

// Set the emulated SID model
void ReSIDfp::model (SidConfig::model_t model)
{
    if (model == SidConfig::MOS8580)
        m_sid.setChipModel (reSIDfp::MOS8580);
    else
        m_sid.setChipModel (reSIDfp::MOS6581);
}
