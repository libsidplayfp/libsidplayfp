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

#include "sidcxx11.h"

namespace reSIDfp
{

class LUT
{
public:
    virtual unsigned short output(unsigned short input) const = 0;

    virtual ~LUT() {}
};

template <unsigned int N>
class InterpolatedLUT final : public LUT
{
private:
    const unsigned short min;
    const unsigned short range;

    unsigned short table[N+1];

public:
    InterpolatedLUT(unsigned short min, unsigned short max, const unsigned short tab[N+1]) :
        min(min),
        range(max-min)
    {
        memcpy(table, tab, (N+1)*sizeof(unsigned short));
    }

    unsigned short output(unsigned short input) const
    {
        const unsigned int scaledInput = ((static_cast<unsigned int>(input - min)<<16) / range) * N;
        const unsigned int index = scaledInput >> 16;
        const unsigned int dist = (scaledInput & ((1 << 16)-1));
        return table[index] + (dist * (table[index+1] - table[index])) / (1 << 16);
    }
};

template <>
class InterpolatedLUT<1> final : public LUT
{
private:
    const unsigned short data;

public:
    InterpolatedLUT(unsigned short data) :
        data(data) {}

    unsigned short output(unsigned short input) const { return data; }
};

} // namespace reSIDfp

#endif
