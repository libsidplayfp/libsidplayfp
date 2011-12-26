/***************************************************************************
                          resid.h  -  ReSid Builder
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

#ifndef _residfp_h_
#define _residfp_h_

/* Since ReSID is not part of this project we are actually
 * creating a wrapper instead of implementing a SID emulation
 */

#include <vector>
#include "sidplayfp/sidbuilder.h"

/**
* ReSIDfp Builder Class
*/
class SID_EXTERN ReSIDfpBuilder: public sidbuilder
{
protected:
    std::vector<sidemu *> sidobjs;

private:
    static const char  *ERR_FILTER_DEFINITION;
    const char *m_error;
    char        m_errorBuffer[100];

public:
    ReSIDfpBuilder  (const char * const name);
    ~ReSIDfpBuilder (void);

    /**
    * true will give you the number of used devices.
    *    return values: 0 none, positive is used sids
    * false will give you all available sids.
    *    return values: 0 endless, positive is available sids.
    */
    uint        devices (bool used);
    uint        create  (uint sids);
    sidemu     *lock    (EventContext *env, sid2_model_t model);
    void        unlock  (sidemu *device);
    void        remove  (void);
    const char *error   (void) const { return m_error; }
    const char *credits (void);

    /// @name global settings
    /// Settings that affect all SIDs
    //@{
    /// enable/disable filter
    void filter   (bool enable);

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

#endif // _resid_h_
