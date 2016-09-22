/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2016 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include <stdint.h>
#include <cassert>

#include "siddefs-fp.h"

namespace reSIDfp
{

/**
 * 8580 integrator
 *
 *                    ---C---
 *                   |       |
 *     vi -----Rfc------[A>----- vo
 *                   vx
 *
 *     IRfc + ICr = 0
 *     IRfc + C*(vc - vc0)/dt = 0
 *     dt/C*(IRfc) + vc - vc0 = 0
 *     vc = vc0 - n*(IRfc(vi,vx))
 *     vc = vc0 - n*(IRfc(vi,g(vc)))
 *
 * IRfc = K*W/L*(Vgst^2 - Vgdt^2) = n*((Vddt - vx)^2 - (Vddt - vi)^2)
 */
class Integrator8580
{
private:
    const unsigned short* opamp_rev;

    mutable int vx;
    mutable int vc;

    const unsigned short kVddt;
    unsigned short n_snake;

    const double denorm;
    const double C;
    const double k;
    const double uCox;

public:
    Integrator8580(const unsigned short* opamp_rev, unsigned short kVddt, double denorm, double C, double k, double uCox) :
        opamp_rev(opamp_rev),
        vx(0),
        vc(0),
        kVddt(kVddt),
        denorm(denorm),
        C(C),
        k(k),
        uCox(uCox) {}

    void setFc(double wl)
    {
        // Normalized current factor, 1 cycle at 1MHz.
        // Fit in 5 bits.
        const double tmp = denorm * (1 << 13) * (uCox / (2. * k) * wl * 1.0e-6 / C);
        assert(tmp > -0.5 && tmp < 65535.5);
        n_snake = static_cast<unsigned short>(tmp + 0.5);
    }

    int solve(int vi) const;
};

} // namespace reSIDfp

#if RESID_INLINING || defined(INTEGRATOR8580_CPP)

namespace reSIDfp
{

RESID_INLINE
int Integrator8580::solve(int vi) const
{
    // "Snake" voltages for triode mode calculation.
    const unsigned int Vgst = kVddt - vx;
    const unsigned int Vgdt = kVddt - vi;

    const unsigned int Vgst_2 = Vgst * Vgst;
    const unsigned int Vgdt_2 = Vgdt * Vgdt;

    // "Snake" current, scaled by (1/m)*2^13*m*2^16*m*2^16*2^-15 = m*2^30
    const int n_I_snake = n_snake * (static_cast<int>(Vgst_2 - Vgdt_2) >> 15);

    // Change in capacitor charge.
    vc += n_I_snake;

    // vx = g(vc)
    const int tmp = (vc >> 15) + (1 << 15);
    assert(tmp < (1 << 16));
    vx = opamp_rev[tmp];

    // Return vo.
    return vx - (vc >> 14);
}

} // namespace reSIDfp

#endif

#endif
