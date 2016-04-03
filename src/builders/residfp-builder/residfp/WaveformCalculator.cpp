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
const CombinedWaveformConfig config[2][4] =
{
    { // kevtris chip J (6581 R2)
        {0.979544f,  0.f,       0.f,       3.98271f,  0.775023f},  // Error   148  (61)
        {0.91186f,   1.78531f,  0.f,       1.10833f,  0.f      },  // Error  1562 (110)
        {0.9f,       2.f,       0.f,       1.f,       0.f      },  // Error     0   (0)
        {0.950095f,  1.48367f,  0.f,       1.079012f, 1.f      },  // Error     2   (2)
    },
    { // kevtris chip V (8580 R5)
        {0.954174f,  0.f,       0.977581f, 2.11887f,  0.971231f},  // Error  1595 (268)
        {0.9017799f, 1.616059f,  0.f,      1.436651f, 0.f      },  // Error 19241 (784)
        {0.912565f,  1.84866f,  0.965f,    1.41917f,  0.f      },  // Error 17839 (668)
        {0.970001f,  0.955653f, 1.00323f,  1.97354f,  1.f      },  // Error  2671 (161)
    },
};

/**
 * Generate bitstate based on emulation of combined waves.
 *
 * @param config model parameters matrix
 * @param waveform the waveform to emulate, 1 .. 7
 * @param accumulator the high bits of the accumulator value
 */
short calculateCombinedWaveform(CombinedWaveformConfig config, int waveform, int accumulator)
{
    float o[12];

    // S with strong top bit for 6581
    for (unsigned int i = 0; i < 12; i++)
    {
        o[i] = (accumulator & (1 << i)) != 0 ? 1.f : 0.f;
    }

    // convert to T
    if ((waveform & 3) == 1)
    {
        const bool top = (accumulator & 0x800) != 0;

        for (int i = 11; i > 0; i--)
        {
            o[i] = top ? 1.0f - o[i - 1] : o[i - 1];
        }

        o[0] = 0.f;
    }

    // convert to ST
    else if ((waveform & 3) == 3)
    {
        // bottom bit is grounded via T waveform selector
        o[0] *= config.stmix;

        for (int i = 1; i < 12; i++)
        {
            o[i] = o[i - 1] * (1.f - config.stmix) + o[i] * config.stmix;
        }
    }

    // topbit for Saw
    if ((waveform & 2) == 2)
    {
        o[11] *= config.topbit;
    }

    // ST, P* waveform?
    if (waveform == 3 || waveform > 4)
    {
        float distancetable[12 * 2 + 1];

        for (int i = 0; i <= 12; i++)
        {
            distancetable[12 + i] = distancetable[12 - i] = 1.f / pow(config.distance, i);
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

            // pulse control bit
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

    for (unsigned int i = 0; i < 12; i++)
    {
        if (o[i] > config.bias)
        {
            value |= 1 << i;
        }
    }

    return value;
}

matrix_t* WaveformCalculator::buildTable(ChipModel model)
{
    const CombinedWaveformConfig* cfgArray = config[model == MOS6581 ? 0 : 1];

    cw_cache_t::iterator lb = CACHE.lower_bound(cfgArray);

    if (lb != CACHE.end() && !(CACHE.key_comp()(cfgArray, lb->first)))
    {
        return &(lb->second);
    }

    matrix_t wftable(8, 4096);

    for (unsigned int idx = 0; idx < 1 << 12; idx++)
    {
        wftable[0][idx] = 0xfff;
        wftable[1][idx] = static_cast<short>((idx & 0x800) == 0 ? idx << 1 : (idx ^ 0xfff) << 1);
        wftable[2][idx] = static_cast<short>(idx);
        wftable[3][idx] = calculateCombinedWaveform(cfgArray[0], 3, idx);
        wftable[4][idx] = 0xfff;
        wftable[5][idx] = calculateCombinedWaveform(cfgArray[1], 5, idx);
        wftable[6][idx] = calculateCombinedWaveform(cfgArray[2], 6, idx);
        wftable[7][idx] = calculateCombinedWaveform(cfgArray[3], 7, idx);
    }

    return &(CACHE.insert(lb, cw_cache_t::value_type(cfgArray, wftable))->second);
}

} // namespace reSIDfp
