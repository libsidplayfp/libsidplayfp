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

#ifndef RESIDFP_H
#define RESIDFP_H

#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

class sidemu;

/**
* ReSIDfp Builder Class
*/
class SID_EXTERN ReSIDfpBuilder: public sidbuilder
{
private:
    const char *m_error;

public:
    ReSIDfpBuilder  (const char * const name);
    ~ReSIDfpBuilder (void);

    /**
    * The number of used devices.
    *    return values: 0 none, positive is used sids
    */
    unsigned int        usedDevices () const { return sidobjs.size (); }

    /** All available sids.
    *    return values: 0 endless, positive is available sids.
    */
    unsigned int        availDevices () const { return 0; }

    unsigned int        create  (unsigned int sids);
    sidemu     *lock    (EventContext *env, const SidConfig::model_t model);
    void        unlock  (sidemu *device);
    void        remove  (void);
    const char *error   (void) const { return m_error; }
    const char *credits (void);

    /// @name global settings
    /// Settings that affect all SIDs
    //@{
    /// enable/disable filter
    void filter (const bool enable);

    /**
    * Set 6581 filter curve
    *
    * @param filterCurve from 0.0 (light) to 1.0 (dark) (default 0.5)
    */
    void filter6581Curve (const double filterCurve);

    /**
    * Set 8580 filter curve
    *
    * @param filterCurve curve center frequency (default 12500)
    */
    void filter8580Curve (const double filterCurve);
    //@}
};

#endif // RESIDFP_H
