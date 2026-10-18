/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2026 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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

#ifndef OPLEMU_H
#define OPLEMU_H

#include "sidplayfp/SidConfig.h"
#include "sidplayfp/siddefs.h"
#include "Event.h"
#include "EventScheduler.h"

#include "c64/c64opl.h"

#include "sidcxx11.h"

#include <string>

namespace libsidplayfp
{

class oplemu : public c64opl
{
protected:
    static const char ERR_UNSUPPORTED_FREQ[];
    static const char ERR_INVALID_CHIP[];

protected:
    EventScheduler *eventScheduler = nullptr;

    event_clock_t m_accessClk = 0;

    /// The sample buffer
    short *m_buffer = nullptr;

    float m_clockRatio;

    /// Current position in buffer
    int m_bufferpos = 0;

    bool m_status = true;
    bool isLocked = false;

    std::string m_error;

public:
    oplemu() :
        m_error("N/A")
    {}

    ~oplemu() override
    {
        delete [] m_buffer;
    }

    /**
     * Clock the OPL chip.
     */
    void clock()
    {
        const event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
        m_accessClk += cycles;
        m_bufferpos += clock(cycles * m_clockRatio, m_buffer+m_bufferpos);
    }

    /**
     * Set execution environment and lock opl to it.
     */
    bool lock(EventScheduler *scheduler) { eventScheduler = scheduler; return true; }

    /**
     * Unlock opl.
     */
    void unlock() { eventScheduler = nullptr; }

    /**
     * Set OPL model.
     */
    //void model(SidConfig::opl_model_t model) {}

    /**
     * Set the sampling frequency.
     *
     * @param systemfreq
     * @param outputfreq
     */
    void sampling(float systemfreq, float outputfreq)
    {
        // 20ms buffer
        const int buffersize = std::ceil(20.f * outputfreq / 1000.f);
        m_buffer = new short[buffersize];
        m_clockRatio = outputfreq / systemfreq;
    }

    /**
     * Get a detailed error message.
     */
    const char* error() const { return m_error.c_str(); }

    /**
     * Get the current position in buffer.
     */
    int bufferpos() const { return m_bufferpos; }

    /**
     * Set the position in buffer.
     */
    void bufferpos(int pos) { m_bufferpos = pos; }

    /**
     * Get the buffer.
     */
    short *buffer() const { return m_buffer; }
};

}

#endif // SIDEMU_H
