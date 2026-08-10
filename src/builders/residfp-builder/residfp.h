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

#ifndef RESIDFP_BUILDER_H
#define RESIDFP_BUILDER_H

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
     * Set 6581 filter curve type.
     *
     * @param filterCurve sets center frequency from 0.0 (dark) to 1.0 (bright). (default 0.5)
     */
    void filter6581Curve(double filterCurve);

    /**
     * Set 6581 filter offset and range.
     *
     * @param filterRange sets center frequency from 0.0 (dark) to 1.0 (bright).
     *                    This also affects the range. (default 0.5)
     */
    void filter6581Range(double filterRange);

    /**
     * Set 8580 filter curve type.
     *
     * @param filterCurve sets center frequency from 0.0 (dark) to 1.0 (bright). (default 0.5)
     */
    void filter8580Curve(double filterCurve);

    /**
     * Enable/disable old caps for 6581 model.
     *
     * @param enable true to enable old 2200pF caps used on ASSY 326298
     *               false to use the standard 470pF caps. (default off)
     */
    void enableOld6581caps(bool enable);

    /**
     * Set combined waveforms strength.
     *
     * @param cws 
     */
    void combinedWaveformsStrength(SidConfig::sid_cw_t cws);
    //@}

    /**
     * Set the DAC leakage level.
     * Affects the envelope and waveforms.
     *
     * @param level the leakage level, between 0 (no leakage) and 1 (standard leakage) (default 1.0)
     * @since 3.1
     */
    void dacLeakage(double level);

    /**
     * Set the 6581 wave offset.
     * Affects the volume of digi samples.
     *
     * @param offset the waveform offset, between 0 (average digis) and 1 (loud digis) (default 1.0)
     * @since 3.1
     */
    void offset6581(double offset);

    /**
     * Set the DC-Blocker resistance.
     * Affects the highpass cutoff frequency.
     *
     * @param res the resistance value, between 0 (10KOhm => ~1.6Hz) and 1 (1KOhm => ~16Hz) (default 0.0)
     * @since 3.1
     */
    void dcbRes(double res);

private:
    struct config;
    config *m_config;
};

#endif // RESIDFP_BUILDER_H
