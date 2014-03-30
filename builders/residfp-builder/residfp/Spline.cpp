/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
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

#include "Spline.h"

#include <cassert>
#include <limits>

namespace reSIDfp
{

double Spline::slope(const Point &a, const Point &b)
{
    return (b.y - a.y) / (b.x - a.x);
}

Spline::Spline(const Point input[], int inputLength) :
    paramsLength(inputLength),
    params(new double[paramsLength][6])
{
    assert(inputLength > 2);

    double dxs[inputLength - 1];
    double ms[inputLength - 1];

    // Get consecutive differences and slopes
    for (int i = 0; i < inputLength - 1; i++)
    {
        assert(input[i].x < input[i + 1].x);

        const double dx = input[i + 1].x - input[i].x;
        const double dy = input[i + 1].y - input[i].y;
        dxs[i] = dx;
        ms[i] = dy/dx;
    }

    // Get degree-1 coefficients
    params[0][4] = ms[0];
    for (int i = 1; i < inputLength - 1; i++)
    {
        const double m = ms[i - 1];
        const double mNext = ms[i];
        if (m * mNext <= 0) {
            params[i][4] = 0.0;
        } else {
            const double dx = dxs[i - 1];
            const double dxNext = dxs[i];
            const double common = dx + dxNext;
            params[i][4] = 3.0 * common / ((common + dxNext) / m + (common + dx) / mNext);
        }
    }
    params[inputLength - 1][4] = ms[inputLength - 2];

    // Get degree-2 and degree-3 coefficients
    for (int i = 0; i < inputLength - 1; i++)
    {
        params[i][0] = input[i].x;
        params[i][1] = input[i + 1].x;
        params[i][5] = input[i].y;

        const double c1 = params[i][4];
        const double m = ms[i];
        const double invDx = 1.0 / dxs[i];
        const double common = c1 + params[i + 1][4] - m - m;
        params[i][3] = (m - c1 - common) * invDx;
        params[i][2] = common * invDx * invDx;
    }

    // Fix the value ranges, because we interpolate outside original bounds if necessary.
    params[0][0] = std::numeric_limits<double>::min();
    params[inputLength - 2][1] = std::numeric_limits<double>::max();

    c = params[0];
}

void Spline::evaluate(double x, Point &out)
{
    if (x < c[0] || x > c[1])
    {
        for (int i = 0; i < paramsLength; i++)
        {
            if (x <= params[i][1])
            {
                c = params[i];
                break;
            }
        }
    }

    // Interpolate
    const double diff = x - c[0];

    // y = a*x^3 + b*x^2 + c*x + d
    out.x =  ((c[2] * diff + c[3]) * diff + c[4]) * diff + c[5];

    // dy = 3*a*x^2 + 2*b*x + c
    out.y = (3.0 * c[2] * diff + 2.0 * c[3]) * diff + c[4];
}

} // namespace reSIDfp
