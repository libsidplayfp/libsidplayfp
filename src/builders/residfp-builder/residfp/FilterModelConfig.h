/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include <algorithm>
#include <cassert>

#include "OpAmp.h"
#include "Spline.h"

#include "siddefs-fp.h"

#include "sidcxx11.h"

namespace reSIDfp
{

class Integrator;

class FilterModelConfig
{
protected:
    /// Capacitor value.
    const double C;

    /// Transistor parameters.
    //@{
    const double Vdd;
    const double Vth;           ///< Threshold voltage
    const double Ut;            ///< Thermal voltage: Ut = kT/q = 8.61734315e-5*T ~ 26mV
    double uCox;                ///< Transconductance coefficient: u*Cox
    const double Vddt;          ///< Vdd - Vth
    //@}

    // Derived stuff
    const double vmin, vmax;
    const double denorm, norm;

    /// Fixed point scaling for 16 bit op-amp output.
    const double N16;

    /// Current factor coefficient for op-amp integrators.
    double currFactorCoeff;

    const double voice_voltage_range;
    const double voice_DC_voltage;

    /// Lookup tables for gain and summer op-amps in output stage / filter.
    //@{
    unsigned short* mixer[8];       //-V730_NOINIT this is initialized in the derived class constructor
    float* summer[5];               //-V730_NOINIT this is initialized in the derived class constructor
    float* volume[16];              //-V730_NOINIT this is initialized in the derived class constructor
    float* resonance[16];             //-V730_NOINIT this is initialized in the derived class constructor
    //@}

    /// Reverse op-amp transfer function.
    float opamp_rev[1 << 16]; //-V730_NOINIT this is initialized in the derived class constructor

private:
    FilterModelConfig (const FilterModelConfig&) DELETE;
    FilterModelConfig& operator= (const FilterModelConfig&) DELETE;

protected:
    /**
     * @param vvr voice voltage range
     * @param vdv voice DC voltage
     * @param c   capacitor value
     * @param vdd Vdd
     * @param vth threshold voltage
     * @param ucox u*Cox
     * @param ominv opamp min voltage
     * @param omaxv opamp max voltage
     */
    FilterModelConfig(
        double vvr,
        double vdv,
        double c,
        double vdd,
        double vth,
        double ucox,
        const Spline::Point *opamp_voltage,
        int opamp_size
    );

    ~FilterModelConfig();

    void setUCox(double new_uCox);

    /**
     * The filter summer operates at n ~ 1, and has 5 fundamentally different
     * input configurations (2 - 6 input "resistors").
     *
     * Note that all "on" transistors are modeled as one. This is not
     * entirely accurate, since the input for each transistor is different,
     * and transistors are not linear components. However modeling all
     * transistors separately would be extremely costly.
     */
    inline void buildSummerTable(const OpAmp& opampModel)
    {
        for (int i = 0; i < 5; i++)
        {
            const int size = 1 << 16;
            const double n = 2. + i;        // 2 - 6 input "resistors".
            opampModel.reset();
            summer[i] = new float[size];

            for (int vi = 0; vi < size; vi++)
            {
                const double vin = vmin + vi / N16; /* vmin .. vmax */
                summer[i][vi] = opampModel.solve(n, vin);
            }
        }
    }

    /**
     * The audio mixer operates at n ~ 8/6 (6581) or 8/5 (8580),
     * and has 8 fundamentally different input configurations
     * (0 - 7 input "resistors").
     *
     * All "on", transistors are modeled as one - see comments above for
     * the filter summer.
     */
    inline void buildMixerTable(const OpAmp& opampModel, double nRatio)
    {
        for (int i = 0; i < 8; i++)
        {
            const int size = 1 << 16;
            const double n = i * nRatio;
            opampModel.reset();
            mixer[i] = new unsigned short[size];

            for (int vi = 0; vi < size; vi++)
            {
                const double vin = vmin + vi / N16; /* vmin .. vmax */
                mixer[i][vi] = getNormalizedValue(opampModel.solve(n, vin));
            }
        }
    }

    /**
     * 4 bit "resistor" ladders in the audio output gain
     * necessitate 16 gain tables.
     * From die photographs of the volume "resistor" ladders
     * it follows that gain ~ vol/12 (6581) or vol/16 (8580)
     * (assuming ideal op-amps and ideal "resistors").
     */
    inline void buildVolumeTable(const OpAmp& opampModel, double nDivisor)
    {
        for (int n8 = 0; n8 < 16; n8++)
        {
            const int size = 1 << 16;
            const double n = n8 / nDivisor;
            opampModel.reset();
            volume[n8] = new float[size];

            for (int vi = 0; vi < size; vi++)
            {
                const double vin = vmin + vi / N16; /* vmin .. vmax */
                volume[n8][vi] = opampModel.solve(n, vin);
            }
        }
    }

    /**
     * 4 bit "resistor" ladders in the bandpass resonance gain
     * necessitate 16 gain tables.
     * From die photographs of the bandpass "resistor" ladders
     * it follows that 1/Q ~ ~res/8 (6581) or 2^((4 - res)/8) (8580)
     * (assuming ideal op-amps and ideal "resistors").
     */
    inline void buildResonanceTable(const OpAmp& opampModel, const double resonance_n[16])
    {
        for (int n8 = 0; n8 < 16; n8++)
        {
            const int size = 1 << 16;
            opampModel.reset();
            resonance[n8] = new float[size];

            for (int vi = 0; vi < size; vi++)
            {
                const double vin = vmin + vi / N16; /* vmin .. vmax */
                resonance[n8][vi] = opampModel.solve(resonance_n[n8], vin);
            }
        }
    }

public:
    unsigned short** getMixer() { return mixer; }
    float** getSummer() { return summer; }
    float** getVolume() { return volume; }
    float** getResonance() { return resonance; }

    virtual Integrator* buildIntegrator() = 0;

    inline double getVddt() const { return Vddt; }
    inline double getVth() const { return Vth; }
    inline double getVmin() const { return vmin; }

    inline float getOpampRev(double Vc) const
    {
        const double tmp = 32768. + N16 * Vc/2.;
        assert(tmp > -0.5 && tmp < 65535.5);
        const int i = static_cast<int>(tmp + 0.5);
        return opamp_rev[i];
    }

    // helper functions
    inline unsigned short getNormalizedValue(double value) const
    {
        if (unlikely(value == 0.)) return 0;
        const double tmp = N16 * (value - vmin);
        assert(tmp > -0.5 && tmp < 65535.5);
        return static_cast<unsigned short>(tmp + 0.5);
    }

    inline double getCurrentFactor(double wl) const
    {
        return currFactorCoeff * wl;
    }

    inline double getVoiceVoltage(float value) const
    {
        return value * voice_voltage_range + voice_DC_voltage;
    }
};

} // namespace reSIDfp

#endif
