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

#include "sidcxx11.h"

namespace reSIDfp
{

class LUT
{
public:
    virtual unsigned short output(unsigned int input) const = 0;

    virtual ~LUT() {}
};

class InterpolatedLUT final : public LUT
{
private:
    const unsigned short size;
    const unsigned int min;
    const unsigned int range;

    unsigned short* table;

public:
    InterpolatedLUT(unsigned short size, unsigned int min, unsigned int max, const unsigned short* tab) :
        size(size),
        min(min),
        range(max-min),
        table(new unsigned short[size+1])
    {
        memcpy(table, tab, (size+1)*sizeof(unsigned short));
    }

    ~InterpolatedLUT() { delete [] table; }

    unsigned short output(unsigned int input) const
    {
        const uint64_t scaledInput = ((static_cast<uint64_t>(input - min)<<16) / range) * size;
        const uint64_t index = scaledInput >> 16;
        const uint64_t dist = (scaledInput & ((1 << 16)-1));
        return table[index] + (dist * (table[index+1] - table[index])) / (1 << 16);
    }
};

} // namespace reSIDfp

#endif
