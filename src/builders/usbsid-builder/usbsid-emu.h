
#ifndef USBSID_EMU_H
#define USBSID_EMU_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "sidplayfp/SidConfig.h"
#include "sidemu.h"
#include "Event.h"
#include "EventScheduler.h"
#include "sidplayfp/siddefs.h"
#include "driver/src/USBSID.h"
#include "driver/src/USBSID_Manager.h"

#include "sidcxx11.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

namespace libsidplayfp
{

class USBSID;

/***************************************************************************
 * USBSID board session, shared by every SID of one USBSIDBuilder
 ***************************************************************************/
class USBSIDSession
{
public:
    typedef std::chrono::steady_clock wallclock_t;

private:
    USBSID_Manager m_manager;

    /// Requested board serials, empty opens the first board only
    std::vector<std::string> m_serials;

    bool m_open = false;

    /// One board: SIDs map onto its four register blocks
    bool m_single = true;

    /// Owner of each SID slot, nullptr when free
    std::vector<USBSID*> m_slots;

    /// Emulated time of the last write sent to each board
    std::vector<event_clock_t> m_lastWrite;

    /// SID that drives pacing and flushing
    USBSID *m_pacer = nullptr;

    /// Board stopped draining its ring, writes are dropped until it recovers
    bool m_stalled = false;

    /// Pacing anchor: wall time `m_start` equals emulated cycle `m_baseClk`
    wallclock_t::time_point m_start;
    event_clock_t m_baseClk = 0;

    /// Emulated time of the last pace() call
    event_clock_t m_lastPace = 0;
    double m_cycleNs = 1000.0;
    unsigned int m_frameCycles = USBSID_NS::R_EU;

public:
    USBSIDSession() = default;
    ~USBSIDSession();

    USBSIDSession(const USBSIDSession &) = delete;
    USBSIDSession &operator=(const USBSIDSession &) = delete;

    /**
     * @brief Set board serials to open. Ignored once open.
     *
     * @param serials serial numbers in logical SID order
     */
    void serials(const std::vector<std::string> &serials) { m_serials = serials; }

    /**
     * @brief Open the requested boards.
     *
     * @param error receives the reason on failure
     * @return true when every requested board is open
     */
    bool open(std::string &error);

    /**
     * @brief Reset, release and close every board.
     */
    void close();

    bool isOpen() const { return m_open; }

    /**
     * @brief Claim the lowest free SID slot.
     *
     * @param sid SID claiming the slot
     * @return slot number, -1 when all slots are in use
     */
    int acquire(USBSID *sid);

    /**
     * @brief Free a slot claimed by acquire().
     *
     * @param slot slot number
     * @param sid SID releasing the slot
     */
    void release(int slot, USBSID *sid);

    /// Number of SID slots
    int capacity() const { return (int)m_slots.size(); }

    bool isPacer(const USBSID *sid) const { return m_pacer == sid; }

    /**
     * @brief Resolve a slot to board, logical SID and register block.
     *
     * @param slot slot number
     * @param board receives the board index
     * @param logical receives the manager logical SID
     * @param regBase receives the board local register base
     */
    void route(int slot, int &board, int &logical, uint8_t &regBase) const;

    /**
     * @brief Send a register write to a board.
     *
     * @param board board index from route()
     * @param logical logical SID from route()
     * @param reg board local register
     * @param data register value
     * @param now emulated time of the write
     */
    void write(int board, int logical, uint8_t reg, uint8_t data, event_clock_t now);

    /**
     * @brief Reset boards, drop queued writes, restart the timeline.
     */
    void reset();

    /**
     * @brief Set the board clock and the pacing rate.
     *
     * @param systemclock emulated CPU clock in Hz
     */
    void clockRate(float systemclock);

    /// Pacing and flush interval in cycles
    unsigned int frameCycles() const { return m_frameCycles; }

    /**
     * @brief Flush all boards, then hold emulation to real time.
     *
     * @param now current emulated time
     */
    void pace(event_clock_t now);

    /**
     * @brief Flush every board.
     */
    void flush();
};

/***************************************************************************
 * USBSID SID Specialisation
 ***************************************************************************/
class USBSID final : public sidemu, private Event
{
private:
    USBSIDSession &m_session;

    int m_slot;
    int m_board = 0;
    int m_logical = 0;
    uint8_t m_regBase = 0;

    uint8_t busValue;  /* Return value on read */

    SidConfig::sid_model_t runmodel;  /* Read model type */

public:
    static const char* getCredits();

public:
    USBSID(sidbuilder *builder, USBSIDSession &session);
    ~USBSID() override;

    bool getStatus() const { return m_status; }

    uint8_t read(uint_least8_t addr) override;
    void write(uint_least8_t addr, uint8_t data) override;

    /* c64sid functions */
    void reset(uint8_t volume) override;

    /* Standard SID functions */
    void clock() override {}

    void sampling(float systemclock, float freq,
        SidConfig::sampling_method_t method) override;

    void model(SidConfig::sid_model_t model, MAYBE_UNUSED bool digiboost) override;

    void unlock() override;

private:
    /**
     * @brief Pace emulation to real time and flush, once per frame.
     */
    void event() override;
};

}

#endif // USBSID_EMU_H
