/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2022 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "WaveformCalculator.h"

#include <cmath>

namespace reSIDfp
{

WaveformCalculator* WaveformCalculator::getInstance()
{
    static WaveformCalculator instance;
    return &instance;
}

/**
 * Parameters derived with the Monte Carlo method based on
 * samplings by kevtris. Code and data available in the project repository [1].
 *
 * The score here reported is the acoustic error
 * calculated XORing the estimated and the sampled values.
 * In parentheses the number of mispredicted bits.
 *
 * [1] https://github.com/libsidplayfp/combined-waveforms
 */
const CombinedWaveformConfig config[2][4] =
{
    { /* kevtris chip G (6581 R2) */
        {0.862147212f, 0.f,          10.8962431f,    2.50848103f }, // TS  error  1941 (327/28672)
        {0.932746708f, 2.07508397f,   1.03668225f,   1.14876997f }, // PT  error  5992 (126/32768)
        {0.785892785f, 1.68656933f,   0.913057923f,  1.09173143f }, // PS  error  3795 (575/28672)
        {0.741343081f, 0.0452554375f, 1.1439606f,    1.05711341f }, // PTS error   338 ( 29/28672)
    },
    { /* kevtris chip V (8580 R5) */
        {0.715788841f, 0.f,           1.32999945f,   2.2172699f  }, // TS  error   928 (135/32768)
        {0.93500334f,  1.05977178f,   1.08629429f,   1.43518543f }, // PT  error  9113 (198/32768)
        {0.920648575f, 0.943601072f,  1.13034654f,   1.41881108f }, // PS  error 12566 (394/32768)
        {0.90921098f,  0.979807794f,  0.942194462f,  1.40958893f }, // PTS error  2092 ( 60/32768)
    },
};

typedef float (*distance_t)(float, int);

// Distance functions
static float exponentialDistance(float distance, int i)
{
    return pow(distance, -i);
}

static float linearDistance(float distance, int i)
{
    return 1.f / (1.f + i * distance);
}

static float quadraticDistance(float distance, int i)
{
    return 1.f / (1.f + (i*i) * distance);
}

/// Calculate triangle waveform
static unsigned int triXor(unsigned int val)
{
    return (((val & 0x800) == 0) ? val : (val ^ 0xfff)) << 1;
}

/**
 * Generate bitstate based on emulation of combined waves.
 *
 * @param config model parameters matrix
 * @param waveform the waveform to emulate, 1 .. 7
 * @param accumulator the high bits of the accumulator value
 */
short calculatePulldown(const CombinedWaveformConfig& config, int waveform, int accumulator)
{
    // saw/tri: if saw is not selected the bits are XORed
    unsigned int osc = (waveform & 2) ? accumulator : triXor(accumulator);

    // If both Saw and Triangle are selected the bits are interconnected
    if ((waveform & 3) == 3)
    {
        /*
        * Enabling the S waveform pulls the XOR circuit selector transistor down
        * (which would normally make the descending ramp of the triangle waveform),
        * so ST does not actually have a sawtooth and triangle waveform combined,
        * but merely combines two sawtooths, one rising double the speed the other.
        *
        * http://www.lemon64.com/forum/viewtopic.php?t=25442&postdays=0&postorder=asc&start=165
        */
        osc &= osc << 1;
    }

    float o[12];

    for (unsigned int i = 0; i < 12; i++)
    {
        o[i] = (accumulator & (1u << i)) != 0 ? 1.f : 0.f;
    }

    // ST, P* waveforms

    // TODO move out of the loop
    const distance_t distFunc = exponentialDistance;

    float distancetable[12 * 2 + 1];
    distancetable[12] = 1.f;
    for (int i = 12; i > 0; i--)
    {
        distancetable[12-i] = distFunc(config.distance1, i);
        distancetable[12+i] = distFunc(config.distance2, i);
    }

    float pulldown[12];

    for (int sb = 0; sb < 12; sb++)
    {
        float avg = 0.f;
        float n = 0.f;

        for (int cb = 0; cb < 12; cb++)
        {
            if (cb == sb)
                continue;
            const float weight = distancetable[sb - cb + 12];
            avg += (1.f - o[cb]) * weight;
            n += weight;
        }

        // pulse control bit
        if (waveform > 4)
        {
            avg -= config.pulsestrength;
        }

        pulldown[sb] = avg / n;
    }

    for (int i = 0; i < 12; i++)
    {
        if (o[i] != 0.f)
            o[i] = 1.f - pulldown[i];
    }

    // Get the predicted value
    short value = 0;

    for (unsigned int i = 0; i < 12; i++)
    {
        if (o[i] > config.threshold)
        {
            value |= 1 << i;
        }
    }

    return value;
}

matrix_t* WaveformCalculator::buildWaveTable()
{
    const CombinedWaveformConfig* cfgArray = config[0];

    cw_cache_t::iterator lb = WAVEFORM_CACHE.lower_bound(cfgArray);

    if (lb != WAVEFORM_CACHE.end() && !(WAVEFORM_CACHE.key_comp()(cfgArray, lb->first)))
    {
        return &(lb->second);
    }

    matrix_t wftable(4, 4096);

    for (unsigned int idx = 0; idx < 1 << 12; idx++)
    {
        short const saw = static_cast<short>(idx);
        short const tri = static_cast<short>((idx & 0x800) == 0 ? idx << 1 : (idx ^ 0xfff) << 1);

        wftable[0][idx] = 0xfff;
        wftable[1][idx] = tri;
        wftable[2][idx] = saw;
        wftable[3][idx] = saw & (saw << 1);
    }
#ifdef HAVE_CXX11
    return &(WAVEFORM_CACHE.emplace_hint(lb, cw_cache_t::value_type(cfgArray, wftable))->second);
#else
    return &(WAVEFORM_CACHE.insert(lb, cw_cache_t::value_type(cfgArray, wftable))->second);
#endif
}

matrix_t* WaveformCalculator::buildPulldownTable(ChipModel model)
{
    const bool is8580 = model != MOS6581;

    const CombinedWaveformConfig* cfgArray = config[is8580 ? 1 : 0];

    cw_cache_t::iterator lb = PULLDOWN_CACHE.lower_bound(cfgArray);

    if (lb != PULLDOWN_CACHE.end() && !(PULLDOWN_CACHE.key_comp()(cfgArray, lb->first)))
    {
        return &(lb->second);
    }

    matrix_t wftable(8, 4096);

    for (unsigned int idx = 0; idx < 1 << 12; idx++)
    {
        wftable[0][idx] = 0;
        wftable[1][idx] = 0;
        wftable[2][idx] = 0;
        wftable[3][idx] = calculatePulldown(cfgArray[0], 3, idx);
        wftable[4][idx] = 0;
        wftable[5][idx] = calculatePulldown(cfgArray[1], 5, idx);
        wftable[6][idx] = calculatePulldown(cfgArray[2], 6, idx);
        wftable[7][idx] = calculatePulldown(cfgArray[3], 7, idx);
    }
#ifdef HAVE_CXX11
    return &(PULLDOWN_CACHE.emplace_hint(lb, cw_cache_t::value_type(cfgArray, wftable))->second);
#else
    return &(PULLDOWN_CACHE.insert(lb, cw_cache_t::value_type(cfgArray, wftable))->second);
#endif
}

} // namespace reSIDfp
