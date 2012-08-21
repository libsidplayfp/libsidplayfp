/*
 * This file is part of reSID, a MOS6581 SID emulator engine.
 * Copyright (C) 2004  Dag Lem <resid@nimrod.no>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * @author Ken Händel
 *
 */

#ifndef WAVEFORMCALCULATOR_h
#define WAVEFORMCALCULATOR_h

#include "siddefs-fp.h"
#include "array.h"

#include <map>


namespace reSIDfp
{

typedef struct {
    float bias;
    float pulsestrength;
    float topbit;
    float distance;
    float stmix;
} CombinedWaveformConfig;

/** @internal
 * Combined waveform calculator for WaveformGenerator.
 *
 * @author Antti Lankila
 */
class WaveformCalculator {

private:
	std::map<const CombinedWaveformConfig*, array<short> > CACHE;

	/*
	 * the "bits wrong" figures below are not directly comparable. 0 bits are very easy to predict, and waveforms that are mostly zero have low scores. More comparable scores would be found by
	 * dividing with the count of 1-bits, or something.
	 */
	static const CombinedWaveformConfig config[2][4];

	/**
	 * Generate bitstate based on emulation of combined waves.
	 *
	 * @param config
	 * @param waveform the waveform to emulate, 1 .. 7
	 * @param accumulator the accumulator value
	 */
	short calculateCombinedWaveform(CombinedWaveformConfig config, const int waveform, const int accumulator) const;

	WaveformCalculator() {}

public:
	static WaveformCalculator* getInstance();

	/**
	 * Build waveform tables for use by WaveformGenerator.
	 *
	 * @param model Chip model to use
	 * @return Waveform table
	 */
	array<short>* buildTable(const ChipModel model);
};

} // namespace reSIDfp

#endif
