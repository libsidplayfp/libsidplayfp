/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef ZEROORDER_RESAMPLER_H
#define ZEROORDER_RESAMPLER_H

#include "Resampler.h"

#include "sidcxx11.h"

namespace reSIDfpII
{

/**
 * Return sample with linear interpolation.
 *
 * @author Antti Lankila
 */
class ZeroOrderResampler final : public Resampler
{

private:
    /// Last sample
    float cachedSample = 0.f;

    /// Calculated sample
    float outputValue = 0.f;

    /// Number of cycles per sample
    const int cyclesPerSample;

    int sampleOffset = 0;

public:
    ZeroOrderResampler(double clockFrequency, double samplingFrequency) :
        cyclesPerSample(static_cast<int>(clockFrequency / samplingFrequency * 1024.)) {}

    bool input(float sample) override
    {
        bool ready = false;

        if (sampleOffset < 1024)
        {
            outputValue = cachedSample + (sampleOffset * (sample - cachedSample) / 1024.f);
            ready = true;
            sampleOffset += cyclesPerSample;
        }

        sampleOffset -= 1024;

        cachedSample = sample;

        return ready;
    }

    float output() const override { return outputValue; }

    void reset() override
    {
        sampleOffset = 0;
        cachedSample = 0.f;
    }
};

} // namespace reSIDfpII

#endif
