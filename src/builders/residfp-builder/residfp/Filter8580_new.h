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

#ifndef FILTER8580_H
#define FILTER8580_H

#include "siddefs-fp.h"

#include <memory>

#include "Filter.h"
#include "FilterModelConfig8580.h"
#include "Integrator8580.h"

#include "sidcxx11.h"

namespace reSIDfp
{

class Integrator8580;

/**
 * Filter for 8580 chip.
 * ~~~
 *
 *                ---------------------------------------------------
 *               |    $17       ----Rf-                              |
 *               |             |       |                             |
 *               |      D4&!D5 |- \-R3-|                             |
 *               |             |       |                    $17      |
 *               |     !D4&!D5 |- \-R2-|                             |
 *               |             |       |   ---R8-- \--   !D6&D7      |
 *               |      D4&!D5 |- \-R1-|  |           |              |
 *               |             |       |  |---RC-- \--|   D6&D7      |
 *               |    ------------<A]-----|           |              |
 *               |   |                    |---R4-- \--|  D6&!D7      |
 *               |   |                    |           |              |
 *               |   |                     ---Ri-- \--| !D6&!D7      |
 *               |   |                                |              |
 * $17           |   |                    (CAP2B)     |  (CAP1B)     |
 * 0=to mixer    |    --R8--    ---R8--        ---C---|       ---C---|
 * 1=to filter   |          |  |       |      |       |      |       |
 *                ------R8--|-----[A>--|--Rfc----[A>--|--Rfc----[A>--|
 *     ve (EXT IN)          |          |              |              |
 * D3  \ ---------------R8--|          |              | (CAP2A)      | (CAP1A)
 *     |   v3               |          | vhp          | vbp          | vlp
 * D2  |   \ -----------R8--|     -----               |              |
 *     |   |   v2           |    |                    |              |
 * D1  |   |   \ -------R8--|    |    ----------------               |
 *     |   |   |   v1       |    |   |                               |
 * D0  |   |   |   \ ---R8--     |   |    ---------------------------
 *     |   |   |   |             |   |   |
 *     R6  R6  R6  R6            R6  R6  R6
 *     |   |   |   | $18         |   |   |  $18
 *     |    \  |   | D7: 1=open   \   \   \ D6 - D4: 0=open
 *     |   |   |   |             |   |   |
 *      ---------------------------------
 *                 |
 *                 |               D3  --/ --1R2--
 *                 |    ---R8--       |           |   ---R2--
 *                 |   |       |   D2 |--/ --2R2--|  |       |
 *                  ------[A>---------|           |-----[A>----- vo (AUDIO OUT)
 *                                 D1 |--/ --4R2--| (4.25R2)
 *                        $18         |           |
 *                        0=open   D0  --/ --8R2--  (8.75R2)
 *
 *
 *
 *
 * R1 = 15.3*Ri
 * R2 =  7.3*Ri
 * R3 =  4.7*Ri
 * Rf =  1.4*Ri
 * R4 =  1.4*Ri
 * R8 =  2.0*Ri
 * RC =  2.8*Ri
 *
 * Rfc - freq control DAC resistance
 */

class Filter8580 final : public Filter
{
private:
    /// Current volume amplifier setting.
    unsigned short* currentGain;

    /// Current filter/voice mixer setting.
    unsigned short* currentMixer;

    /// Filter input summer setting.
    unsigned short* currentSummer;

    /// Filter resonance value.
    unsigned short* currentResonance;

    unsigned short** mixer;
    unsigned short** summer;
    unsigned short** gain_res;
    unsigned short** gain_vol;

    /// Filter highpass state.
    int Vhp;

    /// Filter bandpass state.
    int Vbp;

    /// Filter lowpass state.
    int Vlp;

    /// Filter external input.
    int ve;

    const int voiceScaleS14;
    const int voiceDC;

    double cp;

    /// VCR + associated capacitor connected to lowpass output.
    std::unique_ptr<Integrator8580> const lpIntegrator;

    /// VCR + associated capacitor connected to bandpass output.
    std::unique_ptr<Integrator8580> const bpIntegrator;

public:
    Filter8580() :
        currentGain(nullptr),
        currentMixer(nullptr),
        currentSummer(nullptr),
        currentResonance(nullptr),
        mixer(FilterModelConfig8580::getInstance()->getMixer()),
        summer(FilterModelConfig8580::getInstance()->getSummer()),
        gain_res(FilterModelConfig8580::getInstance()->getGainRes()),
        gain_vol(FilterModelConfig8580::getInstance()->getGainVol()),
        Vhp(0),
        Vbp(0),
        Vlp(0),
        ve(0),
        voiceScaleS14(FilterModelConfig8580::getInstance()->getVoiceScaleS14()),
        voiceDC(FilterModelConfig8580::getInstance()->getVoiceDC()),
        cp(3.6), // FIXME find a good default
        lpIntegrator(FilterModelConfig8580::getInstance()->buildIntegrator()),
        bpIntegrator(FilterModelConfig8580::getInstance()->buildIntegrator())
    {
        input(0);
    }

    ~Filter8580();

    int clock(int voice1, int voice2, int voice3) override;

    void input(int sample) override { ve = (sample * voiceScaleS14 * 3 >> 14) + mixer[0][0]; }

    /**
     * Set filter cutoff frequency.
     */
    void updatedCenterFrequency() override;

    /**
     * Set filter resonance.
     */
    void updateResonance(unsigned char res) override { currentResonance = gain_res[~res & 0x0f]; } // FIXME why is res reversed ???

    void updatedMixing() override;

public:
    /**
     * Set filter curve type based on single parameter.
     *
     * FIXME find a reasonable range of values
     * @param curvePosition
     */
    void setFilterCurve(double curvePosition);
};

} // namespace reSIDfp

#if RESID_INLINING || defined(FILTER8580_CPP)

namespace reSIDfp
{

RESID_INLINE
int Filter8580::clock(int voice1, int voice2, int voice3)
{
    voice1 = (voice1 * voiceScaleS14 >> 18) + voiceDC;
    voice2 = (voice2 * voiceScaleS14 >> 18) + voiceDC;
    voice3 = (voice3 * voiceScaleS14 >> 18) + voiceDC;

    int Vi = 0;
    int Vo = 0;

    (filt1 ? Vi : Vo) += voice1;
    (filt2 ? Vi : Vo) += voice2;

    // NB! Voice 3 is not silenced by voice3off if it is routed
    // through the filter.
    if (filt3) Vi += voice3;
    else if (!voice3off) Vo += voice3;

    (filtE ? Vi : Vo) += ve;

    Vhp = currentSummer[currentResonance[Vbp] + Vlp + Vi];
    Vbp = bpIntegrator->solve(Vhp);
    Vlp = lpIntegrator->solve(Vbp);

    if (lp) Vo += Vlp;
    if (bp) Vo += Vbp;
    if (hp) Vo += Vhp;

    return currentGain[currentMixer[Vo]] - (1 << 15);
}

} // namespace reSIDfp

#endif

#endif
