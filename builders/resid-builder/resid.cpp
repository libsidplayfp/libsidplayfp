/***************************************************************************
                          c64sid.h  -  ReSid Wrapper for redefining the
                                       filter
                             -------------------
    begin                : Fri Apr 4 2001
    copyright            : (C) 2001 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cstring>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#include "resid/siddefs-fp.h"
#include "resid.h"
#include "resid-emu.h"

char ReSID::m_credit[];

ReSID::ReSID (sidbuilder *builder)
:sidemu(builder),
 m_context(NULL),
 m_phase(EVENT_CLOCK_PHI1),
#ifdef HAVE_EXCEPTIONS
 m_sid(*(new(std::nothrow) RESID::SIDFP)),
#else
 m_sid(*(new RESID::SIDFP)),
#endif
 m_status(true),
 m_locked(false)
{
    char *p = m_credit;
    m_error = "N/A";

    // Setup credits
    sprintf (p, "ReSID V%s Engine:", VERSION);
    p += strlen (p) + 1;
    strcpy  (p, "\t(C) 1999-2002 Simon White <sidplay2@yahoo.com>");
    p += strlen (p) + 1;
    sprintf (p, "MOS6581 (SID) Emulation (ReSID V%s):", RESID::resid_version_string);
    p += strlen (p) + 1;
    sprintf (p, "\t(C) 1999-2002 Dag Lem <resid@nimrod.no>");
    p += strlen (p) + 1;
    *p = '\0';

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

ReSID::~ReSID ()
{
    if (&m_sid)
        delete &m_sid;
    delete[] m_buffer;
}

bool ReSID::filter (const sid_filter_t *filter)
{
    /* Set sensible defaults, will override them if new ones provided. */
    m_sid.get_filter().set_distortion_properties(0.5f, 3.3e6f);
    m_sid.get_filter().set_type3_properties(1.37e6f, 1.70e8f, 1.006f, 1.55e4f);
    m_sid.get_filter().set_type4_properties(5.5f, 20.f);
    m_sid.set_voice_nonlinearity(1.f);

    /* Set whatever data is provided in the filter def.
     * XXX: we should check that if one param in set is provided,
     * all are provided. */
    if (filter != NULL) {
        if (filter->baseresistance != 0.f)
            m_sid.get_filter().set_type3_properties(
                filter->baseresistance, filter->offset, filter->steepness, filter->minimumfetresistance
            );

        if (filter->k != 0.f)
            m_sid.get_filter().set_type4_properties(filter->k, filter->b);

        if (filter->attenuation != 0.f)
            m_sid.get_filter().set_distortion_properties(
                filter->attenuation, filter->distortion_nonlinearity
            );

        if (filter->voice_nonlinearity != 0.f)
            m_sid.set_voice_nonlinearity(filter->voice_nonlinearity);
    }

    return true;
}

// Standard component options
void ReSID::reset (uint8_t volume)
{
    m_accessClk = 0;
    m_sid.reset ();
    m_sid.write (0x18, volume);
}

uint8_t ReSID::read (uint_least8_t addr)
{
    clock();
    return m_sid.read (addr);
}

void ReSID::write (uint_least8_t addr, uint8_t data)
{
    clock();
    m_sid.write (addr, data);
}

void ReSID::clock()
{
    cycle_count cycles = m_context->getTime(m_accessClk, m_phase);
    m_accessClk += cycles;
    m_bufferpos += m_sid.clock(cycles, (short *) m_buffer + m_bufferpos, OUTPUTBUFFERSIZE - m_bufferpos, 1);
}

void ReSID::filter (bool enable)
{
    m_sid.enable_filter (enable);
}

void ReSID::sampling (float systemclock, float freq,
        const sampling_method_t method, const bool fast)
{
    sampling_method sampleMethod;
    switch (method)
    {
    case SID2_INTERPOLATE:
        sampleMethod = RESID::SAMPLE_INTERPOLATE;
        break;
    case SID2_RESAMPLE_INTERPOLATE:
        sampleMethod = RESID::SAMPLE_RESAMPLE_INTERPOLATE;
        break;
    default:
        m_status = false;
        m_error = "Invalid sampling method.";
        return;
    }

    if (! m_sid.set_sampling_parameters (systemclock, sampleMethod, freq)) {
        m_status = false;
        m_error = "Unable to set desired output frequency.";
    }
}

// Set execution environment and lock sid to it
bool ReSID::lock (c64env *env)
{
    if (env == NULL)
    {
        if (!m_locked)
            return false;
        m_locked  = false;
        m_context = NULL;
    }
    else
    {
        if (m_locked)
            return false;
        m_locked  = true;
        m_context = &env->context ();
    }
    return true;
}

// Set the emulated SID model
void ReSID::model (sid2_model_t model)
{
    if (model == SID2_MOS8580)
        m_sid.set_chip_model (RESID::MOS8580FP);
    else
        m_sid.set_chip_model (RESID::MOS6581FP);
}
