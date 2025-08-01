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

#ifndef RESIDFP_H
#define RESIDFP_H

#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

/**
 * ReSIDfp Builder Class
 */
class SID_EXTERN ReSIDfpBuilder: public sidbuilder
{
protected:
    /**
     * Create the sid emu.
     */
    libsidplayfp::sidemu* create();

public:
    ReSIDfpBuilder(const char * const name);
    ~ReSIDfpBuilder();


    const char *getCredits() const;

    /// @name global settings
    /// Settings that affect all SIDs.
    //@{
    /**
     * Set 6581 filter curve.
     *
     * @param filterCurve from 0.0 (light) to 1.0 (dark) (default 0.5)
     */
    void filter6581Curve(double filterCurve);

    /**
     * Set 6581 filter curve.
     *
     * @param filterCurve from 0.0 (dark) to 1.0 (light) (default 0.5)
     */
    void filter6581Range(double filterRange);

    /**
     * Set 8580 filter curve.
     *
     * @param filterCurve curve center frequency (default 12500)
     */
    void filter8580Curve(double filterCurve);

    /**
     * Set combined waveforms strength.
     *
     * @param cws 
     */
    void combinedWaveformsStrength(SidConfig::sid_cw_t cws);
    //@}
private:
    struct config;
    config *m_config;
};

#endif // RESIDFP_H
