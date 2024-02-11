/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2014 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "Integrator6581.h"

#ifdef SLOPE_FACTOR
#  include <cmath>
#endif

namespace reSIDfp
{

float Integrator6581::solve(float Vi) const
{
    // Make sure Vgst>0 so we're not in subthreshold mode
    assert(Vx < Vddt);

    // Check that transistor is actually in triode mode
    // Vds < Vgs - Vth
    assert(Vi < Vddt);

    // "Snake" voltages for triode mode calculation.
    const double Vgst = Vddt - Vx;
    const double Vgdt = Vddt - Vi;

    const double Vgst_2 = Vgst * Vgst;
    const double Vgdt_2 = Vgdt * Vgdt;

    // "Snake" current
    const double I_snake = fmc->getCurrentFactor(wlSnake) * (Vgst_2 - Vgdt_2);

    // VCR gate voltage.
    // Vg = Vddt - sqrt(((Vddt - Vw)^2 + Vgdt^2)/2)
    const double Vg = fmc->getVcr_Vg((Vddt_Vw_2 + Vgdt_2)/2.);
#ifdef SLOPE_FACTOR
    const double kVgt = (Vg - Vt) / n; // Pinch-off voltage
#else
    const double kVgt = Vg - Vt;
#endif

    // VCR current
    const double If = fmc->getVcr_Ids_term(kVgt - Vx);
    const double Ir = fmc->getVcr_Ids_term(kVgt - Vi);

#ifdef SLOPE_FACTOR
    const double I_vcr = (If - Ir) * n;
#else
    const double I_vcr = If - Ir;
#endif

#ifdef SLOPE_FACTOR
    // estimate new slope factor based on gate voltage
    const double gamma = 1.0;   // body effect factor
    const double phi = 0.8;     // bulk Fermi potential
    n = 1. + (gamma / (2. * sqrt(kVgt + phi + 4. * fmc->getUt())));
    assert((n > 1.2) && (n < 1.8));
#endif

    // Change in capacitor charge.
    Vc += I_snake + I_vcr;

    // Vx = g(Vc)
    Vx = fmc->getOpampRev(Vc);

    // Return Vo.
    return Vx - Vc;
}

} // namespace reSIDfp
