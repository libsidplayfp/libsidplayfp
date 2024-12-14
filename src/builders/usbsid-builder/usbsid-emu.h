
#ifndef USBSID_EMU_H
#define USBSID_EMU_H

#include "sidemu.h"
#include "Event.h"
#include "EventScheduler.h"
#include "sidplayfp/siddefs.h"

#include "sidcxx11.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

// #include "usbsid.h"
#include "driver/USBSID.h"
// #include "driver/USBSIDInterface.h"

namespace libsidplayfp
{

// Approx 60ms
#define USBSID_DELAY_CYCLES 60000

/***************************************************************************
 * USBSID SID Specialisation
 ***************************************************************************/
class USBSID final : public sidemu, private Event//, public USBSIDBuilder
{
private:
    friend class USBSIDBuilder;

    // USBSID specific data
    USBSID_NS::USBSID_Class usbsid;
    static unsigned int sid;
    static unsigned int m_sidFree[4];
    static unsigned int num;
    static unsigned int m_instance;
    int m_handle;

    bool m_status;
    bool readflag;

    uint8_t busValue;

    SidConfig::sid_model_t runmodel;  /* Read model type */

private:
    // unsigned int delay();
    event_clock_t delay();

public:
    static const char* getCredits();

public:
    USBSID(sidbuilder *builder, bool threaded);
    ~USBSID() override;
    bool m_isthreaded;

    bool getStatus() const { return m_status; }

    uint8_t read(uint_least8_t addr) override;
    void write(uint_least8_t addr, uint8_t data) override;

    /* c64sid functions */
    void reset(uint8_t volume) override;

    /* Standard SID functions */
    void clock() override;
    void model(SidConfig::sid_model_t model, MAYBE_UNUSED bool digiboost) override;
    void sampling(float systemclock, MAYBE_UNUSED float freq,
        MAYBE_UNUSED SidConfig::sampling_method_t method, bool) override;

    /* USBSID specific */
    void flush(void);
    void filter(bool) {}

    // Must lock the SID before using the standard functions.
    bool lock(EventScheduler *env) override;
    void unlock() override;

private:
    // Fixed interval timer delay to prevent sidplay2
    // shoot to 100% CPU usage when song no longer
    // writes to SID.
    void event() override;
};

}

#endif // USBSID_EMU_H
