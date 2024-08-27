/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "Filter8580.h"

#include "Integrator8580.h"

namespace reSIDfp
{

unsigned short Filter8580::clock(float voice1, float voice2, float voice3)
{
    const int V1 = fmc->getNormalizedVoice(voice1);
    const int V2 = fmc->getNormalizedVoice(voice2);
    // Voice 3 is silenced by voice3off if it is not routed through the filter.
    const int V3 = (filt3 || !voice3off) ? fmc->getNormalizedVoice(voice3) : 0;

    int Vsum = 0;
    int Vmix = 0;

    (filt1 ? Vsum : Vmix) += V1;
    (filt2 ? Vsum : Vmix) += V2;
    (filt3 ? Vsum : Vmix) += V3;
    (filtE ? Vsum : Vmix) += Ve;

    Vhp = currentSummer[currentResonance[Vbp] + Vlp + Vsum];
    Vbp = hpIntegrator->solve(Vhp);
    Vlp = bpIntegrator->solve(Vbp);

    if (lp) Vmix += Vlp;
    if (bp) Vmix += Vbp;
    if (hp) Vmix += Vhp;

    return currentVolume[currentMixer[Vmix]];
}

/**
 * W/L ratio of frequency DAC bit 0,
 * other bit are proportional.
 * When no bit are selected a resistance with half
 * W/L ratio is selected.
 */
const double DAC_WL0 = 0.00615;

Filter8580::~Filter8580()
{
    delete hpIntegrator;
    delete bpIntegrator;
}

void Filter8580::updateCenterFrequency()
{
    double wl;
    double dacWL = DAC_WL0;
    if (getFC())
    {
        wl = 0.;
        for (unsigned int i = 0; i < 11; i++)
        {
            if (getFC() & (1 << i))
            {
                wl += dacWL;
            }
            dacWL *= 2.;
        }
    }
    else
    {
        wl = dacWL/2.;
    }

    static_cast<Integrator8580*>(hpIntegrator)->setFc(wl);
    static_cast<Integrator8580*>(bpIntegrator)->setFc(wl);
}

void Filter8580::setFilterCurve(double curvePosition)
{
    // Adjust cp
    // 1.2 <= cp <= 1.8
    cp = 1.8 - curvePosition * 3./5.;

    static_cast<Integrator8580*>(hpIntegrator)->setV(cp);
    static_cast<Integrator8580*>(bpIntegrator)->setV(cp);
}

} // namespace reSIDfp
