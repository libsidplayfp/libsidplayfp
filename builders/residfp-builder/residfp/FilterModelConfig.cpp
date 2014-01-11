/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2010 Dag Lem
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

#include "FilterModelConfig.h"

#include <cmath>

#include "Dac.h"
#include "Integrator.h"
#include "OpAmp.h"
#include "Spline.h"

namespace reSIDfp
{

// This is the SID 6581 op-amp voltage transfer function, measured on
// CAP1B/CAP1A on a chip marked MOS 6581R4AR 0687 14.
// All measured chips have op-amps with output voltages (and thus input
// voltages) within the range of 0.81V - 10.31V.
const double FilterModelConfig::opamp_voltage[OPAMP_SIZE][2] =
{
  {  0.81, 10.31 },  // Approximate start of actual range
  {  2.40, 10.31 },
  {  2.60, 10.30 },
  {  2.70, 10.29 },
  {  2.80, 10.26 },
  {  2.90, 10.17 },
  {  3.00, 10.04 },
  {  3.10,  9.83 },
  {  3.20,  9.58 },
  {  3.30,  9.32 },
  {  3.50,  8.69 },
  {  3.70,  8.00 },
  {  4.00,  6.89 },
  {  4.40,  5.21 },
  {  4.54,  4.54 },  // Working point (vi = vo)
  {  4.60,  4.19 },
  {  4.80,  3.00 },
  {  4.90,  2.30 },  // Change of curvature
  {  4.95,  2.03 },
  {  5.00,  1.88 },
  {  5.05,  1.77 },
  {  5.10,  1.69 },
  {  5.20,  1.58 },
  {  5.40,  1.44 },
  {  5.60,  1.33 },
  {  5.80,  1.26 },
  {  6.00,  1.21 },
  {  6.40,  1.12 },
  {  7.00,  1.02 },
  {  7.50,  0.97 },
  {  8.50,  0.89 },
  { 10.00,  0.81 },
  { 10.31,  0.81 },  // Approximate end of actual range
};

std::auto_ptr<FilterModelConfig> FilterModelConfig::instance(0);

FilterModelConfig* FilterModelConfig::getInstance()
{
    if (!instance.get())
    {
        instance.reset(new FilterModelConfig());
    }

    return instance.get();
}

FilterModelConfig::FilterModelConfig() :
    voice_voltage_range(1.5),
    voice_DC_voltage(5.0),
    C(470e-12),
    Vdd(12.18),
    Vth(1.31),
    uCox_vcr(20e-6),
    WL_vcr(9.0 / 1.0),
    uCox_snake(20e-6),
    WL_snake(1.0 / 115.0),
    dac_zero(6.65),
    dac_scale(2.63),
    vmin(opamp_voltage[0][0]),
    norm(1.0 / ((Vdd - Vth) - vmin))
{
    // Convert op-amp voltage transfer to 16 bit values.

    Dac::kinkedDac(dac, DAC_BITS, 2.2, false);

    const double N16 = norm * ((1 << 16) - 1);

    // Create lookup table mapping capacitor voltage to op-amp input voltage:
    // vc -> vx
    double scaled_voltage[OPAMP_SIZE][2];

    for (unsigned int i = 0; i < OPAMP_SIZE; i++)
    {
        scaled_voltage[i][0] = (N16 * (opamp_voltage[i][0] - opamp_voltage[i][1]) + (1 << 16)) / 2.;
        scaled_voltage[i][1] = N16 * opamp_voltage[i][0];
    }

    Spline s(scaled_voltage, OPAMP_SIZE);
    double out[2];

    for (int x = 0; x < (1 << 16); x ++)
    {
        s.evaluate(x, out);
        opamp_rev[x] = (int)(out[0] + 0.5);
    }

    // The filter summer operates at n ~ 1, and has 5 fundamentally different
    // input configurations (2 - 6 input "resistors").
    //
    // Note that all "on" transistors are modeled as one. This is not
    // entirely accurate, since the input for each transistor is different,
    // and transistors are not linear components. However modeling all
    // transistors separately would be extremely costly.
    OpAmp opampModel(opamp_voltage, OPAMP_SIZE, Vdd - Vth);

    for (int i = 0; i < 5; i++)
    {
        const int idiv = 2 + i;        // 2 - 6 input "resistors".
        const int size = idiv << 16;
        opampModel.reset();
        summer[i] = new unsigned short[size];

        for (int vi = 0; vi < size; vi++)
        {
            const double vin = vmin + vi / N16 / idiv; /* vmin .. vmax */
            summer[i][vi] = (unsigned short)((opampModel.solve(idiv, vin) - vmin) * N16 + 0.5);
        }
    }

    // The audio mixer operates at n ~ 8/6, and has 8 fundamentally different
    // input configurations (0 - 7 input "resistors").
    //
    // All "on", transistors are modeled as one - see comments above for
    // the filter summer.
    for (int i = 0; i < 8; i++)
    {
        const int size = (i == 0) ? 1 : i << 16;
        opampModel.reset();
        mixer[i] = new unsigned short[size];

        for (int vi = 0; vi < size; vi++)
        {
            const double vin = vmin + vi / N16 / (i == 0 ? 1 : i); /* vmin .. vmax */
            mixer[i][vi] = (unsigned short)((opampModel.solve(i * 8.0 / 6.0, vin) - vmin) * N16 + 0.5);
        }
    }

    // 4 bit "resistor" ladders in the bandpass resonance gain and the audio
    // output gain necessitate 16 gain tables.
    // From die photographs of the bandpass and volume "resistor" ladders
    // it follows that gain ~ vol/8 and 1/Q ~ ~res/8 (assuming ideal
    // op-amps and ideal "resistors").
    for (int n8 = 0; n8 < 16; n8++)
    {
        opampModel.reset();
        gain[n8] = new unsigned short[1 << 16];

        for (int vi = 0; vi < (1 << 16); vi++)
        {
            const double vin = vmin + vi / N16; /* vmin .. vmax */
            gain[n8][vi] = (unsigned short)((opampModel.solve(n8 / 8.0, vin) - vmin) * N16 + 0.5);
        }
    }

    const int Vddt = (int)(N16 * (Vdd - Vth) + 0.5);

    for (int i = 0; i < (1 << 16); i++)
    {
        // The table index is right-shifted 16 times in order to fit in
        // 16 bits; the argument to sqrt is thus multiplied by (1 << 16).
        int Vg = Vddt - (int)(sqrt((double) i * (1 << 16)) + 0.5);

        if (Vg >= (1 << 16))
        {
            // Clamp to 16 bits.
            // FIXME: If the DAC output voltage exceeds the max op-amp output
            // voltage while the input voltage is at the max op-amp output
            // voltage, Vg will not fit in 16 bits.
            // Check whether this can happen, and if so, change the lookup table
            // to a plain sqrt.
            Vg = (1 << 16) - 1;
        }

        vcr_Vg[i] = (unsigned short) Vg;
    }

    /*
      EKV model:

      Ids = Is*(if - ir)
      Is = 2*u*Cox*Ut^2/k*W/L
      if = ln^2(1 + e^((k*(Vg - Vt) - Vs)/(2*Ut))
      ir = ln^2(1 + e^((k*(Vg - Vt) - Vd)/(2*Ut))
    */
    const double Ut = 26.0e-3;  // Thermal voltage: Ut = k*T/q = 8.61734315e-5*T ~ 26mV
    const double k = 1.0;       // Gate coupling coefficient: K = Cox/(Cox+Cdep) ~ 0.7
    const double kVt = k * Vth;
    const double Is = 2. * uCox_vcr * Ut * Ut / k * WL_vcr;

    // Normalized current factor for 1 cycle at 1MHz.
    const double N15 = norm * ((1L << 15) - 1);
    const double n_Is = N15 * 1.0e-6 / C * Is;

    // kVg_Vx = k*Vg - Vx
    // I.e. if k != 1.0, Vg must be scaled accordingly.
    for (int kVg_Vx = 0; kVg_Vx < (1 << 16); kVg_Vx++)
    {
        const double log_term = log(1. + exp((kVg_Vx / N16 - kVt) / (2. * Ut)));
        // Scaled by m*2^15
        vcr_n_Ids_term[kVg_Vx] = (unsigned short)(n_Is * log_term * log_term + 0.5);
    }
}

FilterModelConfig::~FilterModelConfig()
{
    for (int i = 0; i < 5; i++)
    {
        delete [] summer[i];
    }

    for (int i = 0; i < 8; i++)
    {
        delete [] mixer[i];
    }

    for (int i = 0; i < 16; i++)
    {
        delete [] gain[i];
    }
}

unsigned int* FilterModelConfig::getDAC(double adjustment) const
{
    const double dac_zero = getDacZero(adjustment);

    const double N16 = norm * ((1L << 16) - 1);
    unsigned int* f0_dac = new unsigned int[1 << DAC_BITS];

    for (int i = 0; i < (1 << DAC_BITS); i++)
    {
        double fcd = 0.;

        for (unsigned int j = 0; j < DAC_BITS; j ++)
        {
            if ((i & (1 << j)) != 0)
            {
                fcd += dac[j];
            }
        }

        f0_dac[i] = (unsigned int)(N16 * (dac_zero + fcd * dac_scale / (1 << DAC_BITS)) + 0.5);
    }

    return f0_dac;
}

Integrator* FilterModelConfig::buildIntegrator()
{
    const double N16 = norm * ((1 << 16) - 1);
    const int Vddt = (int)(N16 * (Vdd - Vth) + 0.5);
    const int n_snake = (int)((1 << 13) / norm * (uCox_snake / 2. * WL_snake * 1.0e-6 / C) + 0.5);
    return new Integrator(vcr_Vg, vcr_n_Ids_term, opamp_rev, Vddt, n_snake);
}

} // namespace reSIDfp
