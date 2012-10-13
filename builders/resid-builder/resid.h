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

#ifndef RESID_H
#define RESID_H

#include <vector>
#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

class sidemu;

/**
* ReSID Builder Class
*/
class SID_EXTERN ReSIDBuilder: public sidbuilder
{
protected:
    std::vector<sidemu *> sidobjs;

private:
    const char *m_error;
    char        m_errorBuffer[100];

public:
    ReSIDBuilder  (const char * const name);
    ~ReSIDBuilder (void);

    /**
    * true will give you the number of used devices.
    *    return values: 0 none, positive is used sids
    * false will give you all available sids.
    *    return values: 0 endless, positive is available sids.
    */
    unsigned int        devices (const bool created);
    unsigned int        create  (unsigned int sids);
    sidemu     *lock    (EventContext *env, const sid2_model_t model);
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
    * The bias is given in millivolts, and a maximum reasonable
    * control range is approximately -500 to 500.
    */
    void bias (const double dac_bias);
    //@}
};

#endif // RESID_H
