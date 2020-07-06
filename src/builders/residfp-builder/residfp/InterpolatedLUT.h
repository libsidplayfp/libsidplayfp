/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2020 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef INPERPOLATEDLUT_H
#define INPERPOLATEDLUT_H

#include <cstring>
#include <cstdint>
#include <cassert>

#include "sidcxx11.h"

namespace reSIDfp
{

class LUT
{
public:
    virtual float output(float input) const = 0;

    virtual ~LUT() {}
};

class InterpolatedLUT final : public LUT
{
private:
    const unsigned int size;
    const float min;
    const float range;

    float* table;

public:
    InterpolatedLUT(unsigned int size, float min, float max, const float* tab) :
        size(size),
        min(min),
        range(max-min),
        table(new float[size+1])
    {
        memcpy(table, tab, (size+1)*sizeof(float));
    }

    ~InterpolatedLUT() { delete [] table; }

    float output(float input) const
    {
        const float scaledInput = ((input - min) / range) * size;
        const unsigned int index = static_cast<unsigned int>(scaledInput + 0.5f);
        const float dist = scaledInput - index;
        assert(index <= size);
        return table[index] + (dist * (table[index+1] - table[index]));
    }
};

} // namespace reSIDfp

#endif
