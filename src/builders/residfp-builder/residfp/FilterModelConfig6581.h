/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2020 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004,2010 Dag Lem
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

#ifndef FILTERMODELCONFIG_H
#define FILTERMODELCONFIG_H

#include <memory>

#include "Dac.h"
#include "Spline.h"

#include "sidcxx11.h"

namespace reSIDfp
{

class Integrator6581;
class LUT;

/**
 * Calculate parameters for 6581 filter emulation.
 */
class FilterModelConfig6581
{
private:
    static const unsigned int DAC_BITS = 11;

private:
    static std::unique_ptr<FilterModelConfig6581> instance;
    // This allows access to the private constructor
#ifdef HAVE_CXX11
    friend std::unique_ptr<FilterModelConfig6581>::deleter_type;
#else
    friend class std::auto_ptr<FilterModelConfig6581>;
#endif

    const double voice_voltage_range;
    const double voice_DC_voltage;

    /// Capacitor value.
    const double C;

    /// Transistor parameters.
    //@{
    const double Vdd;
    const double Vth;           ///< Threshold voltage
    const double Ut;            ///< Thermal voltage: Ut = kT/q = 8.61734315e-5*T ~ 26mV
    const double uCox;          ///< Transconductance coefficient: u*Cox
    const double WL_vcr;        ///< W/L for VCR
    const double WL_snake;      ///< W/L for "snake"
    const double Vddt;          ///< Vdd - Vth
    //@}

    /// DAC parameters.
    //@{
    const double dac_zero;
    const double dac_scale;
    //@}

    // Derived stuff
    const double vmin, vmax;
    const double denorm, norm;

    /// Lookup tables for gain and summer op-amps in output stage / filter.
    //@{
    LUT* mixer[8];
    LUT* summer[5];
    LUT* gain[16];
    //@}

    /// DAC lookup table
    Dac dac;

    /// VCR - 6581 only.
    //@{
    LUT* vcr_Vg;
    LUT* vcr_n_Ids_term;
    //@}

    /// Reverse op-amp transfer function.
    LUT* opamp_rev_lut;

private:
    double getDacZero(double adjustment) const { return dac_zero + (1. - adjustment); }

    FilterModelConfig6581();
    ~FilterModelConfig6581();

public:
    static FilterModelConfig6581* getInstance();

    /**
     * The digital range of one voice is 20 bits; create a scaling term
     * for multiplication which fits in 11 bits.
     */
    float getVoiceScale() const { return norm * voice_voltage_range; }

    /**
     * The "zero" output level of the voices.
     */
    float getVoiceDC() const { return norm * (voice_DC_voltage - vmin); }

    LUT** getGain() { return gain; }

    LUT** getSummer() { return summer; }

    LUT** getMixer() { return mixer; }

    /**
     * Construct an 11 bit cutoff frequency DAC output voltage table.
     * Ownership is transferred to the requester which becomes responsible
     * of freeing the object when done.
     *
     * @param adjustment
     * @return the DAC table
     */
    float* getDAC(double adjustment) const;

    /**
     * Construct an integrator solver.
     *
     * @return the integrator
     */
    std::unique_ptr<Integrator6581> buildIntegrator();
};

} // namespace reSIDfp

#endif
