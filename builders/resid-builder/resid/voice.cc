//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2004  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#include "voice.h"

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
Voice::Voice()
{
  set_chip_model(MOS6581);
}

// ----------------------------------------------------------------------------
// Set chip model.
// ----------------------------------------------------------------------------
void Voice::set_chip_model(chip_model model)
{
  wave.set_chip_model(model);

  if (model == MOS6581) {
    /* there is some level from each voice even if the env is down and osc
     * is stopped. You can hear this by routing a voice into filter (filter
     * should be kept disabled for this) as the master level changes. This
     * tunable affects the volume of digis. */
    voice_DC = static_cast<float>(0x800 * 0xff);
  }
  else {
    /* In 8580 the waveforms seem well centered, but there is a slight offset
     * because sample music is not completely inaudible even though all envelopes
     * are muted. */
    voice_DC = static_cast<float>(-0x100 * 0xff);
  }
}

// ----------------------------------------------------------------------------
// Register functions.
// ----------------------------------------------------------------------------
void Voice::writeCONTROL_REG(WaveformGenerator& source, reg8 control)
{
  wave.writeCONTROL_REG(source, control);
  envelope.writeCONTROL_REG(control);
}

// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void Voice::reset()
{
  wave.reset();
  envelope.reset();
}


// ----------------------------------------------------------------------------
// Voice mute.
// ----------------------------------------------------------------------------
void Voice::mute(bool enable)
{
  envelope.mute(enable);
}
