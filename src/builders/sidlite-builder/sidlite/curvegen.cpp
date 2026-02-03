/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2026 Leandro Nini
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

#include <cmath>

int Resonance8580[0xF];
int Resonance6581[0xF];
int Cutoff8580[0x800];
int Cutoff6581[0x800];

void curvegen(int samplerate)
{
    //generate resonance-multipliers

    //8580 Resonance-DAC (1/Q) curve
    for (int i=0; i<=0xF; ++i)
    {
        Resonance8580[i] = (std::pow(2, (4 - i) / 8.0)) * 0x1000;
    }

    //6581 Resonance-DAC (1/Q) curve
    for (int i=0; i<=0xF; ++i)
    {
        Resonance6581[i] = (i>5 ? 8.0 / i : 1.41) * 0x1000;
    }

    //generate cutoff value curves

    float const cutoff_ratio_8580 = -2 * 3.14 * (12500 / 2048) / samplerate;

    //8580 Cutoff-curve (for samplerate)
    for (int i=0; i<0x800; ++i)
    {
        Cutoff8580[i] = (1 - std::exp((i+2) * cutoff_ratio_8580)) * 0x1000; //linear curve by resistor-ladder VCR (with a little leakage)
    }

    //6581
    constexpr float VCR_SHUNT_6581 = 1500.f; //kOhm //cca 1.5 MOhm Rshunt across VCR FET drain and source (causing 220Hz bottom cutoff with 470pF integrator capacitors in old C64)
    constexpr int VCR_FET_TRESHOLD = 192; //Vth (on cutoff numeric range 0..2048) for the VCR cutoff-frequency control FET below which it doesn't conduct
    constexpr float CAP_6581 = 0.470f; //nF //filter capacitor value for 6581
    constexpr float FILTER_DARKNESS_6581 = 22.0f; //the bigger the value, the darker the filter control is (that is, cutoff frequency increases less with the same cutoff-value)
    constexpr float FILTER_DISTORTION_6581 = 0.0016f; //the bigger the value the more of resistance-modulation (filter distortion) is applied for 6581 cutoff-control

    float cap_6581_reciprocal = -1000000.f/CAP_6581;
    float cutoff_steepness_6581 = FILTER_DARKNESS_6581*(2048.0-VCR_FET_TRESHOLD); //pre-scale for 0...2048 cutoff-value range

    // 6581 Cutoff-curve: (for samplerate)
    for (int i=0; i<0x800; ++i)
    {
        float rDS_VCR_FET = i<=VCR_FET_TRESHOLD ? 100000000.0f //below Vth treshold Vgs control-voltage FET presents an open circuit
            : cutoff_steepness_6581/(i-VCR_FET_TRESHOLD);  // rDS ~ (-Vth*rDSon) / (Vgs-Vth)  //above Vth FET drain-source resistance is proportional to reciprocal of cutoff-control voltage

        Cutoff6581[i] = (1.f - std::exp(cap_6581_reciprocal / (VCR_SHUNT_6581*rDS_VCR_FET/(VCR_SHUNT_6581+rDS_VCR_FET)) / samplerate)) * 0x1000; //curve with 1.5MOhm VCR parallel Rshunt emulation
    }
}
