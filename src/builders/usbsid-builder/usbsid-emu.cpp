
#include "usbsid.h"
#include "usbsid-emu.h"

#include <cstdio>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>

#include "driver/USBSID.h"

#include "sidcxx11.h"

/* #define US_DEBUG */
/* #define USWRITE_DEBUG */
#ifdef US_DEBUG
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...) ((void)0)
#endif
#ifdef USWRITE_DEBUG
#define WDBG(...) printf(__VA_ARGS__)
#else
#define WDBG(...) ((void)0)
#endif

namespace libsidplayfp
{

unsigned int USBSID::m_sidFree[4] = {0,0,0,0};
unsigned int USBSID::m_sidsUsed = 0;
bool USBSID::m_sidInitDone = false;
static long raster_rate;

const char* USBSID::getCredits()
{
    return
        "USBSID V" VERSION " Engine:\n"
        "\t(C) 2024-2025 LouD\n";
}

USBSID::USBSID(sidbuilder *builder, bool cycled, unsigned int count) :
    sidemu(builder),
    Event("USBSID Delay"),
    m_handle(-1),
    m_sid(*(new USBSID_NS::USBSID_Class)),
    m_iscycled(false),
    busValue(0),
    sidno(count)
{
    if (sidno < USBSID_MAXSID) {  /* Any sids available? */
        if (m_sidFree[sidno] == 0) m_sidFree[sidno] = 1;
    } else { /* All sids in use */
        return;
    }
    DBG("[%s] cycled:%d sidno:%d m_sidFree[all]:[%d%d%d%d]\n",
        __func__, cycled, sidno, m_sidFree[0], m_sidFree[1], m_sidFree[2], m_sidFree[3]);

    if (!m_sid.us_Initialised)
    {
        m_error = "out of memory";
        return;
    }

    /* Set cycled option */
    m_iscycled = (cycled == 1) ? 1 : 0;
    /* Only start the object driver once */
    if (sidno == 0) {
        /* Start the fucker */
        m_handle = m_sid.USBSID_Init(m_iscycled, m_iscycled);
        if (m_handle < 0)
        {
            m_error = "USBSID init failed";
            return;
        }
        // m_instance = 1;  /* USB device access ~ finish this later */
    }

    /* NASTY WORKAROUND */
    if(sidno == 0 && USBSID::m_sidInitDone == false) {
        m_sid.USBSID_Mute();
        m_sid.USBSID_ClearBus();
        USBSID::m_sidInitDone = true; // update the static member here
    }
}

USBSID::~USBSID()
{
    DBG("[%s] De-init sidno:%d\n", __func__, sidno);
    m_sidFree[sidno] = 0;
    if (sidno == 0) {
        USBSID::m_sidInitDone = false;
        USBSID::m_sidsUsed = 0;
        reset(0);
        delete &m_sid;
    }
}

void USBSID::reset(uint8_t volume)
{
    using namespace std;

    /* NASTY WORKAROUND */
    if (USBSID::m_sidInitDone == true) {
        USBSID::m_sidsUsed++;
    }
    uint8_t sid = (sidno > (USBSID::m_sidsUsed - 1)) ? (USBSID::m_sidsUsed - 1) : sidno;
    DBG("[%s] m_sidInitDone:%d m_sidsUsed:%d sid:%d sidno:%d volume:%X", __func__, USBSID::m_sidInitDone, USBSID::m_sidsUsed, sid, sidno, volume);
    DBG(" address of `this`: %p\n", this);

    m_accessClk = 0;
    if (sidno == 0 && m_sidInitDone == true) {
        m_sid.USBSID_Mute();
        m_sid.USBSID_ClearBus();
        if (eventScheduler != nullptr) {
            eventScheduler->schedule(*this, raster_rate, EVENT_CLOCK_PHI1);
        }
    }
}

event_clock_t USBSID::delay()
{
    // TODO: This needs more testing which is better!
    // event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk - 1;
    m_accessClk += cycles;
    while (cycles > 0xffff)
    {
        m_sid.USBSID_WaitForCycle(0xffff);
        cycles -= 0xffff;
    }
    return cycles;
}

void USBSID::clock()
{
    if (!m_handle || sidno != 0) return;
    const event_clock_t cycles = delay();
    if (cycles) {
        m_sid.USBSID_WaitForCycle(cycles);
    }
}

uint8_t USBSID::read(uint_least8_t addr)
{   /* NOTICE: Reading is blocking and not needed so it is disabled */
    return busValue;  /* Always return the busValue */
}

void USBSID::write(uint_least8_t addr, uint8_t data)
{
    busValue = data;
    if (addr > 0x18)
        return;

    /* Nasty workaround to get the correct sid address */
    uint8_t sid = (sidno > (USBSID::m_sidsUsed - 1)) ? (USBSID::m_sidsUsed - 1) : sidno;
    uint_least8_t address = ((0x20 * sid) + addr);

    event_clock_t cycles = delay();
    if (cycles) {
        m_sid.USBSID_WaitForCycle(cycles);
    }

    if (m_iscycled == 1) {  /* Digitunes and most other SID tunes */
        m_sid.USBSID_WriteRingCycled(address, data, cycles);
        WDBG("WRITE THREADED CYCLED: $%02X:%02X (c:%lu)\n", address, data, cycles);
    } else {  /* PSID tunes like Spy vs Spy and Commando */
        m_sid.USBSID_Write(address, data);
        WDBG("WRITE: $%02X:%02X (%lu)\n", address, data, cycles);
    };
}

void USBSID::model(SidConfig::sid_model_t model, MAYBE_UNUSED bool digiboost)
{
    /* Not used for USBSID (yet) */
    runmodel = model;
}

void USBSID::sampling(float systemclock, MAYBE_UNUSED float freq,
        MAYBE_UNUSED SidConfig::sampling_method_t method, bool)
{
    (void)freq; /* Audio frequency is not used for USBSID-Pico */
    (void)method; /* Interpolation method is not used for USBSID-Pico */
    if (sidno == 0) {
        m_sid.USBSID_SetClockRate((long)systemclock);  /* Set the USBSID internal oscillator speed */
        raster_rate = m_sid.USBSID_GetRasterRate();    /* Get the USBSID internal raster rate */
    }
}

void USBSID::event()
{
    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
    if (cycles < raster_rate)
    {
        eventScheduler->schedule(*this, raster_rate - cycles, EVENT_CLOCK_PHI1);
    }
    else
    {
        m_accessClk += cycles;
        m_sid.USBSID_Flush();
        eventScheduler->schedule(*this, raster_rate, EVENT_CLOCK_PHI1);
    }
}

void USBSID::filter(bool enable)
{
    DBG("[%s] what is this?\n", __func__);
}

void USBSID::flush() /* Only gets call on player exit!? */
{
    m_sid.USBSID_Flush();
}

} /* libsidplayfp */
