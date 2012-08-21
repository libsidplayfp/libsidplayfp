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

#define ENVELOPEGENERATOR_CPP

#include "EnvelopeGenerator.h"

#include "Dac.h"

namespace reSIDfp
{

const int EnvelopeGenerator::adsrtable[16] = {
	0x7F00,
	0x0006,
	0x003C,
	0x0330,
	0x20C0,
	0x6755,
	0x3800,
	0x500E,
	0x1212,
	0x0222,
	0x1848,
	0x59B8,
	0x3840,
	0x77E2,
	0x7625,
	0x0A93
};

void EnvelopeGenerator::set_exponential_counter() {
	// Check for change of exponential counter period.
	//
	// For a detailed description see:
	// http://ploguechipsounds.blogspot.it/2010/03/sid-6581r3-adsr-tables-up-close.html
	switch (envelope_counter) {
	case 0xff:
		exponential_counter_period = 1;
		break;
	case 0x5d:
		exponential_counter_period = 2;
		break;
	case 0x36:
		exponential_counter_period = 4;
		break;
	case 0x1a:
		exponential_counter_period = 8;
		break;
	case 0x0e:
		exponential_counter_period = 16;
		break;
	case 0x06:
		exponential_counter_period = 30;
		break;
	case 0x00:
		// FIXME: Check whether 0x00 really changes the period.
		// E.g. set R = 0xf, gate on to 0x06, gate off to 0x00, gate on to 0x04,
		// gate off, sample.
		exponential_counter_period = 1;

		// When the envelope counter is changed to zero, it is frozen at zero.
		// This has been verified by sampling ENV3.
		hold_zero = true;
		break;
	}
}

void EnvelopeGenerator::setChipModel(const ChipModel chipModel) {
	const int dacBitsLength = 8;
	double dacBits[dacBitsLength];
	Dac::kinkedDac(dacBits, dacBitsLength, chipModel == MOS6581 ? 2.30 : 2.00, chipModel == MOS8580);
	for (int i = 0; i < 256; i++) {
		double dacValue = 0.;
		for (int j = 0; j < dacBitsLength; j ++) {
			if ((i & (1 << j)) != 0) {
				dacValue += dacBits[j];
			}
		}
		dac[i] = (short) (dacValue + 0.5);
	}
}

void EnvelopeGenerator::reset() {
	envelope_counter = 0;
	envelope_pipeline = false;

	attack = 0;
	decay = 0;
	sustain = 0;
	release = 0;

	gate = false;

	lfsr = 0x7fff;
	exponential_counter = 0;
	exponential_counter_period = 1;

	state = RELEASE;
	rate = adsrtable[release];
	hold_zero = true;
}

void EnvelopeGenerator::writeCONTROL_REG(const unsigned char control) {
	const bool gate_next = (control & 0x01) != 0;

	// The rate counter is never reset, thus there will be a delay before the
	// envelope counter starts counting up (attack) or down (release).

	// Gate bit on: Start attack, decay, sustain.
	if (!gate && gate_next) {
		state = ATTACK;
		rate = adsrtable[attack];

		// Switching to attack state unlocks the zero freeze and aborts any
		// pipelined envelope decrement.
		hold_zero = false;
		// FIXME: This is an assumption which should be checked using cycle exact
		// envelope sampling.
		envelope_pipeline = false;
	}
	// Gate bit off: Start release.
	else if (gate && !gate_next) {
		state = RELEASE;
		rate = adsrtable[release];
	}

	gate = gate_next;
}

void EnvelopeGenerator::writeATTACK_DECAY(const unsigned char attack_decay) {
	attack = (attack_decay >> 4) & 0x0f;
	decay = attack_decay & 0x0f;
	if (state == ATTACK) {
		rate = adsrtable[attack];
	} else if (state == DECAY_SUSTAIN) {
		rate = adsrtable[decay];
	}
}

void EnvelopeGenerator::writeSUSTAIN_RELEASE(const unsigned char sustain_release) {
	sustain = (sustain_release >> 4) & 0x0f;
	release = sustain_release & 0x0f;
	if (state == RELEASE) {
		rate = adsrtable[release];
	}
}

} // namespace reSIDfp
