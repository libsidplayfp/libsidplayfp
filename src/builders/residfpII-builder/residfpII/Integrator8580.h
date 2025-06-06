/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2023 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004, 2010 Dag Lem <resid@nimrod.no>
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

#ifndef INTEGRATOR8580_H
#define INTEGRATOR8580_H

#include "Integrator.h"
#include "FilterModelConfig8580.h"

#include <stdint.h>
#include <cassert>

#include "siddefs-fpII.h"

namespace reSIDfpII
{

/**
 * 8580 integrator
 *
 *                   +---C---+
 *                   |       |
 *     vi -----Rfc---o--[A>--o-- vo
 *                   vx
 *
 *     IRfc + ICr = 0
 *     IRfc + C*(vc - vc0)/dt = 0
 *     dt/C*(IRfc) + vc - vc0 = 0
 *     vc = vc0 - n*(IRfc(vi,vx))
 *     vc = vc0 - n*(IRfc(vi,g(vc)))
 *
 * IRfc = K*W/L*(Vgst^2 - Vgdt^2) = n*((Vddt - vx)^2 - (Vddt - vi)^2)
 *
 * Rfc gate voltage is generated by an OP Amp and depends on chip temperature.
 */
class Integrator8580 : public Integrator
{
private:
    unsigned short nVgt;
    unsigned short n_dac;

    FilterModelConfig8580& fmc;

public:
    Integrator8580(FilterModelConfig8580& fmc) :
        fmc(fmc)
    {
        setV(1.5);
    }

    /**
     * Set Filter Cutoff resistor ratio.
     */
    void setFc(double wl)
    {
        // Normalized current factor, 1 cycle at 1MHz.
        n_dac = fmc.getNormalizedCurrentFactor<17>(wl);
    }

    /**
     * Set FC gate voltage multiplier.
     */
    void setV(double v)
    {
        // Gate voltage is controlled by the switched capacitor voltage divider
        // Ua = Ue * v = 4.75v  1<v<2
        assert(v > 1.0 && v < 2.0);
        const double Vg = fmc.getVref() * v;
        const double Vgt = Vg - fmc.getVth();

        // Vg - Vth, normalized so that translated values can be subtracted:
        // Vgt - x = (Vgt - t) - (x - t)
        nVgt = fmc.getNormalizedValue(Vgt);
    }

    int solve(int vi) const override;
};

} // namespace reSIDfpII

#endif
