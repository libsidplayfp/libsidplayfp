/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright (C) 2000 Simon White
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef MIXER_H
#define MIXER_H

#include <stdlib.h>

#include "event.h"
#include "sidbuilder.h"

/**
* This class implements the mixer.
*/
class Mixer : private Event
{
private:
    /**
    * Scheduling time for next sample mixing event. 5000 is roughly 5 ms
    */
    static const int MIXER_EVENT_RATE = 5000;

public:
    /** Maximum allowed volume, must be a power of 2 */
    static const int_least32_t VOLUME_MAX = 1024;

private:
    /**
    * Event context.
    */
    EventContext &event_context;

    sidemu *m_chip1;
    sidemu *m_chip2;

    int oldRandomValue;
    int m_fastForwardFactor;

    int_least32_t  m_leftVolume;
    int_least32_t  m_rightVolume;
    bool           m_stereo;

    // Mixer settings
    uint_least32_t m_sampleCount;
    uint_least32_t m_sampleIndex;
    short         *m_sampleBuffer;

private:
    int triangularDithering()
    {
        const int prevValue = oldRandomValue;
        oldRandomValue = rand() & (VOLUME_MAX-1);
        return oldRandomValue - prevValue;
    }

public:
    /**
    * Create a new mixer.
    *
    * @param context event context
    */
    Mixer(EventContext *context) :
        Event("Mixer"),
        event_context(*context),
        oldRandomValue(0),
        m_fastForwardFactor(1),
        m_sampleCount(0) {}

    /**
    * Timer ticking event.
    */
    void event();

    void reset() { event_context.schedule(*this, MIXER_EVENT_RATE, EVENT_CLOCK_PHI2); }

    void begin(short *buffer, uint_least32_t count);
    void setSids(sidemu *chip1, sidemu *chip2);
    bool setFastForward(int ff);
    void setVolume(int_least32_t left, int_least32_t right);
    void setStereo(bool stereo) { m_stereo = stereo; }

    bool notFinished() const { return m_sampleIndex != m_sampleCount; }
    uint_least32_t samplesGenerated() const { return m_sampleIndex; }
    uint_least32_t sampleCount() const { return m_sampleCount; }
};

#endif // MIXER_H
