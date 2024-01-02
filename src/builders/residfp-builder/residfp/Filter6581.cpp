/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "Filter6581.h"

#include "Integrator6581.h"

namespace reSIDfp
{

unsigned short Filter6581::clock(int voice1, int voice2, int voice3)
{
    voice1 = (voice1 * voiceScaleS11 >> 15) + voiceDC;
    voice2 = (voice2 * voiceScaleS11 >> 15) + voiceDC;
    // Voice 3 is silenced by voice3off if it is not routed through the filter.
    voice3 = (filt3 || !voice3off) ? (voice3 * voiceScaleS11 >> 15) + voiceDC : 0;

    int Vi = 0;
    int Vo = 0;

    (filt1 ? Vi : Vo) += voice1;
    (filt2 ? Vi : Vo) += voice2;
    (filt3 ? Vi : Vo) += voice3;
    (filtE ? Vi : Vo) += ve;

    Vhp = currentSummer[currentResonance[Vbp] + Vlp + Vi];
    Vbp = hpIntegrator->solve(Vhp);
    Vlp = bpIntegrator->solve(Vbp);

    if (lp) Vo += Vlp;
    if (bp) Vo += Vbp;
    if (hp) Vo += Vhp;

    return currentGain[currentMixer[Vo]];
}

Filter6581::~Filter6581()
{
    delete [] f0_dac;

    delete hpIntegrator;
    delete bpIntegrator;
}

void Filter6581::updatedCenterFrequency()
{
    const unsigned short Vw = f0_dac[fc];
    hpIntegrator->setVw(Vw);
    bpIntegrator->setVw(Vw);
}

void Filter6581::updatedMixing()
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

void Filter6581::setFilterCurve(double curvePosition)
{
    delete [] f0_dac;
    f0_dac = FilterModelConfig6581::getInstance()->getDAC(curvePosition);
    updatedCenterFrequency();
}

} // namespace reSIDfp
