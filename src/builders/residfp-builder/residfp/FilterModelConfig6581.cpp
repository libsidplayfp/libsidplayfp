/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2020 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "FilterModelConfig6581.h"

#include <cmath>
#include <cassert>

#include "Integrator6581.h"
#include "OpAmp.h"

namespace reSIDfp
{

#ifndef HAVE_CXX11
/**
 * Compute log(1+x) without losing precision for small values of x
 *
 * @note when compiling with -ffastm-math the compiler will
 * optimize the expression away leaving a plain log(1. + x)
 */
inline double log1p(double x)
{
    return log(1. + x) - (((1. + x) - 1.) - x) / (1. + x);
}
#endif

const unsigned int OPAMP_SIZE = 33;

/**
 * This is the SID 6581 op-amp voltage transfer function, measured on
 * CAP1B/CAP1A on a chip marked MOS 6581R4AR 0687 14.
 * All measured chips have op-amps with output voltages (and thus input
 * voltages) within the range of 0.81V - 10.31V.
 */
const Spline::Point opamp_voltage[OPAMP_SIZE] =
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

std::unique_ptr<FilterModelConfig6581> FilterModelConfig6581::instance(nullptr);

FilterModelConfig6581* FilterModelConfig6581::getInstance()
{
    if (!instance.get())
    {
        instance.reset(new FilterModelConfig6581());
    }

    return instance.get();
}

FilterModelConfig6581::FilterModelConfig6581() :
    voice_voltage_range(1.5),
    voice_DC_voltage(5.0),
    C(470e-12),
    Vdd(12.18),
    Vth(1.31),
    Ut(26.0e-3),
    k(1.0),
    uCox(20e-6),
    WL_vcr(9.0 / 1.0),
    WL_snake(1.0 / 115.0),
    Vddt(Vdd - Vth),
    dac_zero(6.65),
    dac_scale(2.63),
    vmin(opamp_voltage[0].x),
    vmax(Vddt < opamp_voltage[0].y ? opamp_voltage[0].y : Vddt),
    denorm(vmax - vmin),
    norm(1.0 / denorm),
    dac(DAC_BITS)
{
    dac.kinkedDac(MOS6581);

    // Convert op-amp voltage transfer to 16 bit values.

    Spline::Point scaled_voltage[OPAMP_SIZE];

    for (unsigned int i = 0; i < OPAMP_SIZE; i++)
    {
        scaled_voltage[i].x = norm * (opamp_voltage[i].x - opamp_voltage[i].y);
        scaled_voltage[i].y = norm * (opamp_voltage[i].x - vmin);
    }

    float temp_tab[(7 << 8)+1];

    // Create lookup table mapping capacitor voltage to op-amp input voltage:

    Spline s(scaled_voltage, OPAMP_SIZE);

    for (unsigned int x = 0; x <= (1 << 8); x++)
    {
        const Spline::Point out = s.evaluate(static_cast<double>(x)*2./256. - 1.);
        double tmp = out.x;
        //assert(tmp > -0.5 && tmp < 65535.5);
        temp_tab[x] = static_cast<float>(tmp);
    }
    opamp_rev_lut = new InterpolatedLUT(256, -1.f, 1.f, temp_tab);

    // Create lookup tables for gains / summers.

    OpAmp opampModel(opamp_voltage, OPAMP_SIZE, Vddt);

    // The filter summer operates at n ~ 1, and has 5 fundamentally different
    // input configurations (2 - 6 input "resistors").
    //
    // Note that all "on" transistors are modeled as one. This is not
    // entirely accurate, since the input for each transistor is different,
    // and transistors are not linear components. However modeling all
    // transistors separately would be extremely costly.
    for (unsigned int i = 0; i < 5; i++)
    {
        const unsigned int idiv = 2 + i;        // 2 - 6 input "resistors".
        const unsigned int size = idiv << 8;
        const double n = idiv;
        opampModel.reset();

        for (unsigned int vi = 0; vi <= size; vi++)
        {
            const double vin = vmin + static_cast<double>(vi)/256. * denorm / idiv; /* vmin .. vmax */
            const double tmp = (opampModel.solve(n, vin) - vmin) * norm;
            assert(tmp > -0.5 && tmp < 65535.5);
            temp_tab[vi] = static_cast<float>(tmp);
        }
        summer[i] = new InterpolatedLUT(size, 0.f, static_cast<float>(size)/256.f, temp_tab);
    }

    // The audio mixer operates at n ~ 8/6, and has 8 fundamentally different
    // input configurations (0 - 7 input "resistors").
    //
    // All "on", transistors are modeled as one - see comments above for
    // the filter summer.
    for (unsigned int i = 0; i < 8; i++)
    {
        const unsigned int idiv = (i == 0) ? 1 : i;
        const unsigned int size = (i == 0) ? 1 : i << 8;
        const double n = i * 8.0 / 6.0;
        opampModel.reset();

        for (unsigned int vi = 0; vi <= size; vi++)
        {
            const double vin = vmin + static_cast<double>(vi)/256. * denorm / idiv; /* vmin .. vmax */
            const double tmp = (opampModel.solve(n, vin) - vmin) * norm;
            assert(tmp > -0.5 && tmp < 65535.5);
            temp_tab[vi] = static_cast<float>(tmp);
        }
        mixer[i] = new InterpolatedLUT(size, 0.f, static_cast<float>(size)/256.f, temp_tab);
    }

    // 4 bit "resistor" ladders in the bandpass resonance gain and the audio
    // output gain necessitate 16 gain tables.
    // From die photographs of the bandpass and volume "resistor" ladders
    // it follows that gain ~ vol/8 and 1/Q ~ ~res/8 (assuming ideal
    // op-amps and ideal "resistors").
    for (unsigned int n8 = 0; n8 < 16; n8++)
    {
        const unsigned int size = 1 << 8;
        const double n = n8 / 8.0;
        opampModel.reset();

        for (unsigned int vi = 0; vi <= size; vi++)
        {
            const double vin = vmin + static_cast<double>(vi)/256. * denorm; /* vmin .. vmax */
            const double tmp = (opampModel.solve(n, vin) - vmin) * norm;
            //assert(tmp > -0.5 && tmp < 65535.5);
            temp_tab[vi] = static_cast<float>(tmp);
        }
        gain[n8] = new InterpolatedLUT(size, 0.f, 1.f, temp_tab);
    }

    const double nVddt = norm * Vddt;
    const double nVmin = norm * vmin;

    for (unsigned int i = 0; i <= (1 << 8); i++)
    {
        // The returned value must be corrected for translation. Vg always
        // takes part in a subtraction as follows:
        //
        //   k*Vg - Vx = (k*Vg - t) - (Vx - t)
        //
        // I.e. k*Vg - t must be returned.
        const double Vg = nVddt - sqrt(static_cast<double>(i)/256.);
        const double tmp = k * Vg - nVmin;
        //assert(tmp > -0.5 && tmp < 65535.5);
        temp_tab[i] = static_cast<float>(tmp);
    }
    vcr_kVg = new InterpolatedLUT(256, 0.f, 1.f, temp_tab);

    //  EKV model:
    //
    //  Ids = Is * (if - ir)
    //  Is = ((2 * u*Cox * Ut^2)/k) * W/L
    //  if = ln^2(1 + e^((k*(Vg - Vt) - Vs)/(2*Ut))
    //  ir = ln^2(1 + e^((k*(Vg - Vt) - Vd)/(2*Ut))

    const double kVt = k * Vth;
    // moderate inversion characteristic current
    const double Is = ((2. * uCox * Ut * Ut) / k) * WL_vcr;

    // Normalized current factor for 1 cycle at 1MHz.
    const double n_Is = norm * 1.0e-6 / C * Is;

    // kVg_Vx = k*Vg - Vx
    // I.e. if k != 1.0, Vg must be scaled accordingly.
    for (unsigned int kVg_Vx = 0; kVg_Vx <= (1 << 8); kVg_Vx++)
    {
        const double log_term = log1p(exp(((static_cast<double>(kVg_Vx)/256.) * denorm - kVt) / (2. * Ut)));
        const double tmp = n_Is * log_term * log_term;
        //assert(tmp > -0.5 && tmp < 65535.5);
        temp_tab[kVg_Vx] = static_cast<float>(tmp);
    }
    vcr_n_Ids_term = new InterpolatedLUT(256, 0.f, 1.f, temp_tab);
}

FilterModelConfig6581::~FilterModelConfig6581()
{
    for (int i = 0; i < 5; i++)
    {
        delete summer[i];
    }

    for (int i = 0; i < 8; i++)
    {
        delete mixer[i];
    }

    for (int i = 0; i < 16; i++)
    {
        delete gain[i];
    }
}

float* FilterModelConfig6581::getDAC(double adjustment) const
{
    const double dac_zero = getDacZero(adjustment);

    float* f0_dac = new float[1 << DAC_BITS];

    for (unsigned int i = 0; i < (1 << DAC_BITS); i++)
    {
        const double fcd = dac.getOutput(i);
        const double tmp = norm * (dac_zero + fcd * dac_scale / (1 << DAC_BITS) - vmin);
        //assert(tmp > -0.5 && tmp < 65535.5);
        f0_dac[i] = static_cast<float>(tmp);
    }

    return f0_dac;
}

std::unique_ptr<Integrator6581> FilterModelConfig6581::buildIntegrator()
{
    // Vdd - Vth, normalized so that translated values can be subtracted:
    // Vddt - x = (Vddt - t) - (x - t)
    double tmp = norm * (Vddt - vmin);
    //assert(tmp > -0.5 && tmp < 65535.5);
    const float nVddt = static_cast<float>(tmp);

    // Normalized snake current factor, 1 cycle at 1MHz.
    tmp = denorm * (uCox / (2. * k) * WL_snake * 1.0e-6 / C);
    //assert(tmp > -0.5 && tmp < 65535.5);
    const float n_snake = static_cast<float>(tmp);

    return std::unique_ptr<Integrator6581>(new Integrator6581(vcr_kVg, vcr_n_Ids_term, opamp_rev_lut, nVddt, n_snake));
}

} // namespace reSIDfp
