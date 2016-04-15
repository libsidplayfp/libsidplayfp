/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2016 Leandro Nini <drfiemost@users.sourceforge.net>
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
 * In parentheses the number of mispredicted bits
 * on a total of 32768.
 *
 * [1] http://svn.code.sf.net/p/sidplay-residfp/code/trunk/combined-waveforms/
 */
const CombinedWaveformConfig WaveformCalculator::config[2][4] =
{
#if 1
    { /* kevtris chip G (6581 R2) */
        {0.90251f, 0.f,        0.f,       1.9147f,    1.6747f,  0.62376f }, // error  1689 (280)
        {0.93088f, 2.4843f,    0.f,       1.0353f,    1.1484f,  0.f      }, // error  6128 (130)
        {0.90988f, 2.26303f,   1.13126f,  1.0035f,    1.13801f, 0.f      }, // error 14243 (632)
        {0.91f,    1.192f,     0.f,       1.0169f,    1.2f,     0.637f   }, // error    64   (2)
    },
#else
    { /* kevtris chip J (6581 R2) */
        {0.979544f, 0.f,       0.f,       3.98271f,   0.f,      0.775023f}, // error   148  (61)
        {0.9079f,   1.72749f,  0.f,       1.12017f,   1.10793f, 0.f      }, // error  1540 (102)
        {0.9f,      2.f,       0.f,       1.f,        1.f,      0.f      }, // error     0
        {0.95248f,  1.51f,     0.f,       1.07153f,   1.09353f, 1.f      }, // error     0
    },
#endif
    { /* kevtris chip V (8580 R5) */
        {0.9632f,   0.f,       0.975f,    1.7467f,    2.36132f, 0.975395f}, // error  1380 (169)
        {0.93303f,  1.7025f,   0.f,       1.0868f,    1.43527f, 0.f      }, // error  7981 (204)
        {0.94043f,  1.7937f,   0.981f,    1.1213f,    1.4259f,  0.f      }, // error 11957 (362)
        {0.95865f,  1.0106f,   1.0017f,   1.3707f,    1.8647f,  0.76826f }, // error  2347  (81)
    },
};

matrix_t* WaveformCalculator::buildTable(ChipModel model)
{
    const CombinedWaveformConfig* cfgArray = config[model == MOS6581 ? 0 : 1];

    cw_cache_t::iterator lb = CACHE.lower_bound(cfgArray);

    if (lb != CACHE.end() && !(CACHE.key_comp()(cfgArray, lb->first)))
    {
        return &(lb->second);
    }

    matrix_t wftable(8, 4096);

    for (unsigned int accumulator = 0; accumulator < 1 << 24; accumulator += 1 << 12)
    {
        const int unsigned idx = (accumulator >> 12);
        wftable[0][idx] = 0xfff;
        wftable[1][idx] = (short)((accumulator & 0x800000) == 0 ? idx << 1 : (idx ^ 0xfff) << 1);
        wftable[2][idx] = (short) idx;
        wftable[3][idx] = calculateCombinedWaveform(cfgArray[0], 3, accumulator);
        wftable[4][idx] = 0xfff;
        wftable[5][idx] = calculateCombinedWaveform(cfgArray[1], 5, accumulator);
        wftable[6][idx] = calculateCombinedWaveform(cfgArray[2], 6, accumulator);
        wftable[7][idx] = calculateCombinedWaveform(cfgArray[3], 7, accumulator);
    }

    return &(CACHE.insert(lb, cw_cache_t::value_type(cfgArray, wftable))->second);
}

short WaveformCalculator::calculateCombinedWaveform(CombinedWaveformConfig config, int waveform, int accumulator) const
{
    float o[12];

    // Saw
    for (unsigned int i = 0; i < 12; i++)
    {
        o[i] = ((accumulator >> 12) & (1 << i)) != 0 ? 1.f : 0.f;
    }

    // convert to Triangle
    if ((waveform & 3) == 1)
    {
        const bool top = (accumulator & 0x800000) != 0;

        for (int i = 11; i > 0; i--)
        {
            o[i] = top ? 1.0f - o[i - 1] : o[i - 1];
        }

        o[0] = 0.f;
    }

    // or to Saw+Triangle
    else if ((waveform & 3) == 3)
    {
        /* bottom bit is grounded via T waveform selector */
        o[0] *= config.stmix;

        for (int i = 1; i < 12; i++)
        {
            /*
             * Enabling the S waveform pulls the XOR circuit selector transistor down
             * (which would normally make the descending ramp of the triangle waveform),
             * so ST does not actually have a sawtooth and triangle waveform combined,
             * but merely combines two sawtooths, one rising double the speed the other.
             *
             * http://www.lemon64.com/forum/viewtopic.php?t=25442&postdays=0&postorder=asc&start=165
             */
            o[i] = o[i - 1] * (1.f - config.stmix) + o[i] * config.stmix;
        }
    }

    // topbit for Saw
    if ((waveform & 2) == 2)
    {
        o[11] *= config.topbit;
    }

    // ST, P* waveforms
    if (waveform == 3 || waveform > 4)
    {
        float distancetable[12 * 2 + 1];
        distancetable[12] = 1.f;
        for (int i = 12; i > 0; i--)
        {
            distancetable[12-i] = 1.0f / pow(config.distance1, i);
            distancetable[12+i] = 1.0f / pow(config.distance2, i);
        }

        float tmp[12];

        for (int i = 0; i < 12; i++)
        {
            float avg = 0.f;
            float n = 0.f;

            for (int j = 0; j < 12; j++)
            {
                const float weight = distancetable[i - j + 12];
                avg += o[j] * weight;
                n += weight;
            }

            /* pulse control bit */
            if (waveform > 4)
            {
                const float weight = distancetable[i - 12 + 12];
                avg += config.pulsestrength * weight;
                n += weight;
            }

            tmp[i] = (o[i] + avg / n) * 0.5f;
        }

        for (int i = 0; i < 12; i++)
        {
            o[i] = tmp[i];
        }
    }

    short value = 0;

    for (int i = 0; i < 12; i++)
    {
        if (o[i] > config.bias)
        {
            value |= 1 << i;
        }
    }

    return value;
}

} // namespace reSIDfp
