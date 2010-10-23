/***************************************************************************
                          SidFilter.cpp  -  read filter
                             -------------------
    begin                : Sun Mar 11 2001
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "SidFilter.h"
#include "iniParser.h"


SidFilter::SidFilter ()
:m_status(false),
m_errorString("SID Filter: No filter loaded")
{}

void SidFilter::read (const char *filename)
{
    iniParser m_parser;
    m_status = false;

    if (!m_parser.open (filename))
    {
        m_errorString = "SID Filter: Unable to open filter file";
        return;
    }

    if (!m_parser.setSection ("Filter")) {
        m_errorString = "SID Filter: Unable to locate filter section in input file";
        return;
    }

    struct parampair {
        const char* name;
        float*      address;
    };

    struct parampair sidparams[] = {
        { "DistortionAttenuation",	&m_filter.attenuation           },
        { "DistortionNonlinearity",	&m_filter.distortion_nonlinearity },
        { "VoiceNonlinearity",		&m_filter.voice_nonlinearity    },
        { "Type3BaseResistance",	&m_filter.baseresistance        },
        { "Type3Offset",		&m_filter.offset                },
        { "Type3Steepness",		&m_filter.steepness             },
        { "Type3MinimumFETResistance",	&m_filter.minimumfetresistance  },
        { "Type4K",			&m_filter.k                     },
        { "Type4B",			&m_filter.b                     },
        { NULL,				NULL                            }
    };

    for (int i = 0; sidparams[i].name != NULL; i ++) {
        /* Ensure that all parameters are zeroed, if missing. */
        float tmp = 0.;
        const char* value = m_parser.getValue (sidparams[i].name);
        if (value) {
            tmp = iniParser::parseFloat (value);
            /*if ( < 0) {
                std::string msg = "Invalid parameter: " + std::string(sidparams[i].name) + ": int expected";
                m_errorString = (char *) msg.c_str();
                m_status = false;
                return;
            }*/
        }

        *sidparams[i].address = tmp;
    }

    m_status = true;
}

// Get filter
const sid_filter_t *SidFilter::provide () const
{
    if (!m_status)
        return NULL;
    return &m_filter;
}

// Copy filter from another SidFilter class
const SidFilter &SidFilter::operator= (const SidFilter &filter)
{
    (void) operator= (filter.provide ());
    return filter;
}

// Copy sidplay2 sid_filter_t object
const sid_filter_t &SidFilter::operator= (const sid_filter_t &filter)
{
    m_filter = filter;
    m_status = true;
    return filter;
}

const sid_filter_t *SidFilter::operator= (const sid_filter_t *filter)
{
    m_status = false;
    if (filter)
        (void) operator= (*filter);
    return filter;
}
