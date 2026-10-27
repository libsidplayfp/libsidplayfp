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

#include "libs/emu8950/emu8950.h"

#include <cmath>
#include <string>

namespace libsidplayfp
{

class oplemu : public c64opl
{
private:
    static constexpr uint32_t OPL_CLOCK = 3579545;

protected:
    static const char ERR_UNSUPPORTED_FREQ[];
    static const char ERR_INVALID_CHIP[];

protected:
    EventScheduler *eventScheduler = nullptr;

    OPL *m_opl = nullptr;

    event_clock_t m_accessClk = 0;

    /// The sample buffer
    short *m_buffer = nullptr;

    float m_clockRatio;
    float m_err = 0.f;

    /// Current position in buffer
    int m_bufferpos = 0;

    std::string m_error;

protected:
    void write(uint_least8_t addr, uint8_t data) override final
    {
        clock();
        if (addr == 0x40)
            OPL_writeIO(m_opl, 0, data); // register select
        else if (addr == 0x50)
            OPL_writeIO(m_opl, 1, data); // register write
    }

    uint8_t read(uint_least8_t addr) override final
    {
        clock();
        if (addr == 0x60)
            return OPL_readIO(m_opl); // register read
        return 0;
    }

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
        float integral;
        m_err = std::modf(cycles * m_clockRatio + m_err, &integral);
        int bytes = integral;
        short *buf = m_buffer + m_bufferpos;
        for (int i=0; i<bytes; i++)
            buf[i] = OPL_calc(m_opl);
        m_bufferpos += bytes;
    }

    void reset() override final
    {
        OPL_reset(m_opl);
    }

    /**
     * Set execution environment and lock opl to it.
     */
    void lock(EventScheduler *scheduler, uint32_t rate)
    {
        m_opl = OPL_new(OPL_CLOCK, rate);
        OPL_setChipType(m_opl, 2); // 1:YM3526, 2:YM3812
        eventScheduler = scheduler;
    }

    /**
     * Unlock opl.
     */
    void unlock() { if (m_opl) OPL_delete(m_opl); eventScheduler = nullptr; }

    bool locked() const { return eventScheduler != nullptr; }

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
        delete [] m_buffer;
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
