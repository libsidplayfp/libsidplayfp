
#include "usbsid.h"
#include "usbsid-emu.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "driver/src/USBSID.h"
#include "driver/src/USBSID_Manager.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

namespace
{

/// Bus time of one write on the board, taken off every write delta
constexpr event_clock_t ACCESS_OVERHEAD = 1;

/// Largest delta a cycled write carries
constexpr event_clock_t MAX_DELTA = 0xffff;

/// Lag past which the pacer drops the schedule instead of catching up
constexpr std::chrono::milliseconds RESYNC_LAG(250);

/// Free ring bytes a write waits for, one write takes 4. The driver thread
/// measures its backlog without wrap-around: a ring filled to within
/// 64 bytes reads as empty and stops draining. Stay well clear of that.
constexpr int RING_LOW_WATER = 1024;

/// Longest wait for ring space before writing anyway
constexpr std::chrono::milliseconds RING_WAIT_MAX(500);

}

/***************************************************************************
 * USBSIDSession
 ***************************************************************************/

USBSIDSession::~USBSIDSession()
{
    close();
}

bool USBSIDSession::open(std::string &error)
{
    if (m_open)
        return true;

    for (size_t i = 0; i < m_serials.size(); i++)
    {
        if (m_serials[i].empty())
        {
            error = "USBSID ERROR: empty board serial";
            return false;
        }
        if (std::find(m_serials.begin(), m_serials.begin() + i, m_serials[i]) != m_serials.begin() + i)
        {
            error = "USBSID ERROR: board " + m_serials[i] + " listed twice";
            return false;
        }
    }

    // One empty serial opens the first board in bus/port order
    const std::vector<std::string> targets = m_serials.empty()
        ? std::vector<std::string>{ std::string() }
        : m_serials;

    if (!m_manager.OpenAll(targets, true, true))
    {
        error = m_serials.empty()
            ? "USBSID ERROR: no USBSID-Pico board found"
            : "USBSID ERROR: none of the requested boards could be opened";
        return false;
    }

    // OpenAll() skips a board it cannot open, a missing board shifts every SID after it
    for (const std::string &serial: m_serials)
    {
        bool found = false;
        for (const USBSID_Manager::BoardInfo &board: m_manager.Boards())
            found |= (board.serial == serial);
        if (!found)
        {
            error = "USBSID ERROR: board " + serial + " not found or busy";
            m_manager.CloseAll();
            return false;
        }
    }

    m_single = (m_manager.BoardCount() == 1);
    m_slots.assign(m_single ? USBSID_MAXSID : m_manager.TotalSIDs(), nullptr);
    m_lastWrite.assign(m_manager.BoardCount(), 0);
    m_pacer = nullptr;
    m_open = true;

    m_manager.ResetAllRegistersAll();
    m_manager.StartAll();
    return true;
}

void USBSIDSession::close()
{
    if (!m_open)
        return;

    m_manager.FlushAll();
    m_manager.ResetAllRegistersAll();
    m_manager.StartAll();
    m_manager.CloseAll();

    m_slots.clear();
    m_lastWrite.clear();
    m_pacer = nullptr;
    m_open = false;
}

int USBSIDSession::acquire(USBSID *sid)
{
    for (size_t slot = 0; slot < m_slots.size(); slot++)
    {
        if (m_slots[slot] == nullptr)
        {
            m_slots[slot] = sid;
            if (m_pacer == nullptr)
                m_pacer = sid;
            return (int)slot;
        }
    }
    return -1;
}

void USBSIDSession::release(int slot, USBSID *sid)
{
    if ((slot < 0) || ((size_t)slot >= m_slots.size()) || (m_slots[slot] != sid))
        return;

    m_slots[slot] = nullptr;

    if (m_pacer == sid)
    {
        // Hand pacing to the lowest remaining SID
        m_pacer = nullptr;
        for (USBSID *owner: m_slots)
        {
            if (owner != nullptr)
            {
                m_pacer = owner;
                break;
            }
        }
    }
}

void USBSIDSession::route(int slot, int &board, int &logical, uint8_t &regBase) const
{
    if (m_single)
    {
        // Raw register blocks of the one board, whatever its socket config says
        board = 0;
        logical = 0;
        regBase = (uint8_t)(slot * 0x20);
        return;
    }

    const USBSID_Manager::LogicalSlot &entry = m_manager.LogicalMap()[slot];
    board = entry.board_index;
    logical = slot;
    regBase = (uint8_t)(entry.local_slot * 0x20);
}

void USBSIDSession::write(int board, int logical, uint8_t reg, uint8_t data, event_clock_t now)
{
    if (!m_open)
        return;

    // Deltas are per board, each board plays its own write stream
    event_clock_t delta = now - m_lastWrite[board];
    m_lastWrite[board] = now;

    // The board sits idle after a long gap, the write is due this far into the paced frame
    if (delta > MAX_DELTA)
        delta = std::min(now - m_lastPace, MAX_DELTA);

    const uint16_t cycles = (uint16_t)((delta > ACCESS_OVERHEAD) ? (delta - ACCESS_OVERHEAD) : 0);

    // The driver ring has no overflow guard, hold off while the board drains it
    if (m_manager.RingFreeBytes(logical) < RING_LOW_WATER)
    {
        // A stalled board gets no second wait, emulation keeps running
        if (m_stalled)
            return;

        const wallclock_t::time_point limit = wallclock_t::now() + RING_WAIT_MAX;
        while ((m_manager.RingFreeBytes(logical) < RING_LOW_WATER) && (wallclock_t::now() < limit))
            std::this_thread::sleep_for(std::chrono::microseconds(200));

        if (m_manager.RingFreeBytes(logical) < RING_LOW_WATER)
        {
            // Drop the write, overwriting unsent ring data corrupts the stream
            m_stalled = true;
            fprintf(stderr, "USBSID: board stopped accepting writes, dropping writes\n");
            return;
        }
    }
    m_stalled = false;

    m_manager.WriteRingCycled(logical, reg, data, cycles);
}

void USBSIDSession::reset()
{
    if (!m_open)
        return;

    // Drop writes still queued for the previous tune before the reset overtakes them
    m_manager.ResetRingBufferAll();
    m_manager.ResetAllRegistersAll();
    m_manager.StartAll();
    m_manager.UnMuteAll();

    std::fill(m_lastWrite.begin(), m_lastWrite.end(), 0);
    m_stalled = false;
    m_lastPace = 0;
    m_baseClk = 0;
    m_start = wallclock_t::now();
}

void USBSIDSession::clockRate(float systemclock)
{
    if (!m_open || (systemclock <= 0.f))
        return;

    m_cycleNs = 1e9 / systemclock;
    m_frameCycles = (systemclock < 1000000.f) ? USBSID_NS::R_EU : USBSID_NS::R_US;

    // Board takes a fixed set of rates, pick the nearest one
    const long hz = std::lround(systemclock);
    long best = 0;
    for (const USBSID_NS::clock_speeds rate: USBSID_NS::clockSpeed)
    {
        if ((best == 0) || (std::labs(rate - hz) < std::labs(best - hz)))
            best = rate;
    }
    m_manager.SetClockRateAll(best, true);

    m_baseClk = m_lastPace;
    m_start = wallclock_t::now();
}

void USBSIDSession::pace(event_clock_t now)
{
    if (!m_open)
        return;

    // Send the frame emulated since the last call while waiting for its due time
    m_manager.FlushAll();
    m_lastPace = now;

    // Deadline counts from a fixed anchor, a late frame does not delay later ones
    const wallclock_t::time_point due = m_start + std::chrono::nanoseconds(
        (long long)((double)(now - m_baseClk) * m_cycleNs));

    const wallclock_t::time_point t = wallclock_t::now();
    if (t >= due)
    {
        // Stalled or paused: move the anchor, a catch-up burst overflows the ring
        if ((t - due) > RESYNC_LAG)
        {
            m_start = t;
            m_baseClk = now;
        }
        return;
    }

    for (;;)
    {
        const wallclock_t::duration left = due - wallclock_t::now();
        if (left <= wallclock_t::duration::zero())
            break;
        if (left > std::chrono::milliseconds(1))
            std::this_thread::sleep_for(left - std::chrono::microseconds(500));
        else
            std::this_thread::yield();
    }
}

void USBSIDSession::flush()
{
    if (m_open)
        m_manager.FlushAll();
}

/***************************************************************************
 * USBSID
 ***************************************************************************/

const char* USBSID::getCredits()
{
    return
        "USBSID V" VERSION " Engine:\n"
        "\t(C) 2024-2026 LouD\n";
}

USBSID::USBSID(sidbuilder *builder, USBSIDSession &session) :
    sidemu(builder),
    Event("USBSID Pacer"),
    m_session(session),
    m_slot(-1),
    busValue(0),
    runmodel(SidConfig::MOS6581)
{
    m_slot = m_session.acquire(this);
    if (m_slot < 0)
    {
        m_error = "USBSID ERROR: all " + std::to_string(m_session.capacity()) + " SIDs in use";
        m_status = false;
        return;
    }

    m_session.route(m_slot, m_board, m_logical, m_regBase);
}

USBSID::~USBSID()
{
    m_session.release(m_slot, this);
}

void USBSID::reset(uint8_t)
{
    if (m_session.isPacer(this))
        m_session.reset();

    if (eventScheduler != nullptr)
    {
        // Prevent a double insert when the scheduler was not reset
        eventScheduler->cancel(*this);
        eventScheduler->schedule(*this, m_session.frameCycles(), EVENT_CLOCK_PHI1);
    }
}

uint8_t USBSID::read(uint_least8_t)
{
    return busValue;  /* Always return the busValue */
}

void USBSID::write(uint_least8_t addr, uint8_t data)
{
    busValue = data;
    if (addr > 0x18)
        return;

    m_session.write(m_board, m_logical, (uint8_t)(m_regBase + addr), data,
        eventScheduler->getTime(EVENT_CLOCK_PHI1));
}

void USBSID::model(SidConfig::sid_model_t model, MAYBE_UNUSED bool digiboost)
{
    /* Not used for USBSID (yet) */
    runmodel = model;
}

void USBSID::sampling(float systemclock, float freq,
        SidConfig::sampling_method_t method)
{
    (void)freq; /* Audio frequency is not used for USBSID-Pico */
    (void)method; /* Interpolation method is not used for USBSID-Pico */
    if (m_session.isPacer(this))
        m_session.clockRate(systemclock);
}

void USBSID::unlock()
{
    // Scheduler outlives this SID, drop the pending event
    if (eventScheduler != nullptr)
        eventScheduler->cancel(*this);
    sidemu::unlock();
}

void USBSID::event()
{
    // Every SID keeps its event running, the pacer role can move on release
    if (m_session.isPacer(this))
        m_session.pace(eventScheduler->getTime(EVENT_CLOCK_PHI1));

    eventScheduler->schedule(*this, m_session.frameCycles(), EVENT_CLOCK_PHI1);
}

} /* libsidplayfp */
