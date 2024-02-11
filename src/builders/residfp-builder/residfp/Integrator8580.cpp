/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2014-2016 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "Integrator8580.h"

namespace reSIDfp
{

float Integrator8580::solve(float Vi) const
{
    // Make sure we're not in subthreshold mode
    assert(Vx < Vgt);

    // DAC voltages
    const double Vgst = Vgt - Vx;
    const double Vgdt = (Vi < Vgt) ? Vgt - Vi : 0.;  // triode/saturation mode

    const double Vgst_2 = Vgst * Vgst;
    const double Vgdt_2 = Vgdt * Vgdt;

    // DAC current
    const double I_dac = n_dac * (Vgst_2 - Vgdt_2);

    // Change in capacitor charge.
    Vc += I_dac;

    // Vx = g(Vc)
    Vx = fmc->getOpampRev(Vc);

    // Return Vo.
    return Vx - Vc;
}

} // namespace reSIDfp
