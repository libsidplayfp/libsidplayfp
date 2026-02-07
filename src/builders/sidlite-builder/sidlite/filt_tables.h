/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2025-2026 Leandro Nini
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


// Based on cRSID lightweight RealSID by Hermit (Mihaly Horvath)

#include <array>
#include <cstddef>

#if __cplusplus > 202302L
#  include <cmath>

    constexpr int CRSID_FILTERTABLE_RESOLUTION = 12;
    constexpr int Magnitude = (1 << CRSID_FILTERTABLE_RESOLUTION);

    // https://joelfilho.com/blog/2020/compile_time_lookup_tables_in_cpp/
    template<std::size_t Length, typename Generator>
    constexpr auto lut(Generator&& f)
    {
        using content_type = decltype(f(std::size_t{0}));
        std::array<content_type, Length> arr {};

        for (std::size_t i=0; i<Length; i++)
        {
            arr[i] = f(i);
        }

        return arr;
    }

    //8580 Resonance-DAC (1/Q) curve
    constexpr unsigned short res8580(int n)
    {
        return (std::pow(2, (4 - n) / 8.0)) * Magnitude;
    }

    //6581 Resonance-DAC (1/Q) curve
    constexpr unsigned short res6581(int n)
    {
        return (n>5 ? 8.0 / n : 1.41) * Magnitude;
    }

    static constexpr auto Resonances8580 = lut<0x10>(res8580);
    static constexpr auto Resonances6581 = lut<0x10>(res6581);
#else

    static const unsigned short Resonances8580[] =
    { //8580 Resonance-DAC (1/Q) curve: Magnitude:$1000
        //  0     1      2      3      4      5      6      7      8      9      A      B      C      D      E      F
        0x16A0,0x14BF,0x1306,0x1172,0x1000,0x0EAC,0x0D74,0x0C56,0x0B50,0x0A5F,0x0983,0x08B9,0x0800,0x0756,0x06BA,0x062B,
        //0xFF,  0xFE,  0xFC,  0xF8,  0xF0,  0xE8,  0xD5,  0xC5,  0xB3,  0xA4,  0x97,  0x8A,  0x80,  0x77,  0x6E,  0x66 <- calculated then refined manually to sound best
    };

    static const unsigned short Resonances6581[] =
    { //6581 Resonance-DAC (1/Q) curve: Magnitude:$1000
        //  0     1      2      3      4      5      6      7      8      9      A      B      C      D      E      F
        0x168F,0x168F,0x168F,0x168F,0x168F,0x168F,0x1555,0x1249,0x1000,0x0E38,0x0CCC,0x0BA2,0x0AAA,0x09D8,0x0924,0x0888,
        //0xFF,  0xFE,  0xFD,  0xFC,  0xFB,  0xF9,  0xF6,  0xF2,  0xEC,  0xE4,  0xCD,  0xBA,  0xAB,  0x9E,  0x92,  0x86 <- calculated then refined manually to sound best
    };

#endif
