/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2020 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004 Dag Lem <resid@nimrod.no>
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

#ifndef EXTERNALFILTER_H
#define EXTERNALFILTER_H

#include "siddefs-fp.h"

namespace reSIDfp
{

/**
 * The audio output stage in a Commodore 64 consists of two STC networks, a
 * low-pass filter with 3 dB frequency 16kHz followed by a DC-blocker which
 * acts as a high-pass filter with a cutoff dependent on the attached audio
 * equipment impedance. Here we suppose an impedance of 1kOhm resulting
 * in a 3 dB attenuation at 16Hz.
 *
 * The STC networks are connected with a [BJT] supposedly meant to act
 * as a unity gain buffer, which is not really how it works.
 * A more elaborate model would include the BJT, however DC circuit analysis
 * yields BJT base-emitter and emitter-base impedances sufficiently low
 * to produce additional low-pass and high-pass 3dB-frequencies
 * in the order of hundreds of kHz. This calls for a sampling frequency
 * of several MHz, which is far too high for practical use.
 *
 * [BJT]: https://en.wikipedia.org/wiki/Bipolar_junction_transistor
 */
class ExternalFilter
{
private:
    /// Lowpass filter voltage
    float Vlp;

    /// Highpass filter voltage
    float Vhp;

    float w0lp_1_s7;

    float w0hp_1_s17;

public:
    /**
     * SID clocking.
     *
     * @param input
     */
    float clock(unsigned short input);

    /**
     * Constructor.
     */
    ExternalFilter();

    /**
     * Setup of the external filter sampling parameters.
     *
     * @param frequency
     */
    void setClockFrequency(double frequency);

    /**
     * SID reset.
     */
    void reset();
};

} // namespace reSIDfp

#if RESID_INLINING || defined(EXTERNALFILTER_CPP)

namespace reSIDfp
{

RESID_INLINE
float ExternalFilter::clock(unsigned short input)
{
    const float Vi = static_cast<float>(input);
    const float dVlp = w0lp_1_s7 * (Vi - Vlp);
    const float dVhp = w0hp_1_s17 * (Vlp - Vhp);
    Vlp += dVlp;
    Vhp += dVhp;
    return Vlp - Vhp;
}

} // namespace reSIDfp

#endif

#endif
