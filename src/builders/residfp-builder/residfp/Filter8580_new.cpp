/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2016 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004,2010 Dag Lem <resid@nimrod.no>
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

#define FILTER8580_CPP

#include "Filter8580_new.h"

#include "Integrator8580.h"

namespace reSIDfp
{

/**
 * Filter cutoff DAC resistances.
 */
const double dacWL[11] =
{
    (1./64.)*(1./(5./2. + 1./27.5)),
    (1./32.)*(1./(5./2. + 1./27.5)),
    (1./16.)*(1./(5./2. + 1./27.5)),
    (1./8.) *(1./(5./2. + 1./27.5)),
    (1./4.) *(1./(5./2. + 1./27.5)),
    (1./2.) *(1./(5./2. + 1./27.5)),
     1.     *(1./(5./2. + 1./27.5)),
     2.     *(1./(5./2. + 1./27.5)),
     4.     *(1./(5./2. + 1./27.5)),
     8.     *(1./(5./2. + 1./27.5)),
    16.     *(1./(5./2. + 1./27.5))
};

Filter8580::~Filter8580() {}

void Filter8580::updatedCenterFrequency()
{
    double wl;
    if (fc)
    {
        wl = 0.;
        for (unsigned int i = 0; i < 11; i++)
        {
            if (fc & (1 << i))
                wl += dacWL[i] * cp;
        }
    }
    else
        wl = (1./128.)*(1./(5./2. + 1./27.5)) * cp;

    bpIntegrator->setFc(wl);
    lpIntegrator->setFc(wl);
}

void Filter8580::updatedMixing()
{
    currentGain = gain_vol[vol];

    unsigned int ni = 0;
    unsigned int no = 0;

    (filt1 ? ni : no)++;
    (filt2 ? ni : no)++;

    if (filt3) ni++;
    else if (!voice3off) no++;

    (filtE ? ni : no)++;

    currentSummer = summer[ni];

    if (lp) no++;
    if (bp) no++;
    if (hp) no++;

    currentMixer = mixer[no];
}

void Filter8580::setFilterCurve(double curvePosition)
{
    cp = curvePosition;
    updatedCenterFrequency();
}

} // namespace reSIDfp
