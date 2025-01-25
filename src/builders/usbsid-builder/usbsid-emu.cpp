
#include "usbsid.h"
#include "usbsid-emu.h"

#include <cstdio>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>

#include "driver/USBSID.h"

#include "sidcxx11.h"

/* #define USWRITE_DEBUG */
#ifdef USWRITE_DEBUG
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...) ((void)0)
#endif

namespace libsidplayfp
{

unsigned int USBSID::m_sidFree[4] = {0,0,0,0};
unsigned int USBSID::m_sidsUsed = 0;
bool USBSID::m_sidInitDone = false;
static long refresh_rate;

const char* USBSID::getCredits()
{
    return
        "USBSID V" VERSION " Engine:\n"
        "\t(C) 2024 LouD\n";
}

USBSID::USBSID(sidbuilder *builder, bool threaded, bool cycled, unsigned int count) :
    sidemu(builder),
    Event("USBSID Delay"),
    m_sid(*(new USBSID_NS::USBSID_Class)),
    m_isthreaded(false),
    m_iscycled(false),
    readflag(false),
    busValue(0),
    sidno(count)
{
    if (sidno < USBSID_MAXSID) {  /* Any sids available? */
        if (m_sidFree[sidno] == 0) m_sidFree[sidno] = 1;
    } else { /* All sids in use */
        return;
    }
    DBG("[%s] threaded:%d cycled:%d sidno:%d m_sidFree[all]:[%d%d%d%d]\n",
        __func__, threaded, cycled, sidno, m_sidFree[0], m_sidFree[1], m_sidFree[2], m_sidFree[3]);

    if (!m_sid.us_Initialised)
    {
        m_error = "out of memory";
        return;
    }

    /* Set threaded option ~ overruled by cycled */
    m_isthreaded = (threaded == 1) ? 1 : 0;
    /* Set cycled option */
    m_iscycled = (cycled == 1) ? 1 : 0;
    /* Only start the object driver once */
    if (sidno == 0) {
        /* Start the fucker */
        if (m_sid.USBSID_Init(m_isthreaded, m_iscycled) < 0)
        {
            m_error = "USBSID init failed";
            return;
        }
        // m_instance = 1;  /* USB device access ~ finish this later */
    }

    /* NASTY WORKAROUND */
    if(USBSID::m_sidInitDone == false) {
        m_sid.USBSID_Reset();
        USBSID::m_sidInitDone = true; // update the static member here
    }
}

USBSID::~USBSID()
{
    DBG("[%s] BREAKDOWN POO! sidno:%d\n", __func__, sidno);
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
    readflag = false;
    if (sidno == 0 && m_sidInitDone == true) {
        if (volume > 0) m_sid.USBSID_Reset();
        if (volume == 0) m_sid.USBSID_ResetAll();
        if (eventScheduler != nullptr)
            eventScheduler->schedule(*this, refresh_rate, EVENT_CLOCK_PHI1);
    }
}

event_clock_t USBSID::delay()
{
    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
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
    const event_clock_t cycles = delay();
    if (cycles) {
        m_sid.USBSID_WaitForCycle(cycles);
    }
}

uint8_t USBSID::read(uint_least8_t addr)
{
    return busValue;  /* NOTICE: Reading is disabled for now! */
    /* if ((addr < 0x19) || (addr > 0x1C))
    {
        return busValue;
    } */

    if (!readflag)
    {
        readflag = true;
        // Here we can implement any "safe" detection routine return values if we want to
        // if (0x1b == addr) {	// we could implement a commandline-chosen return byte here
            // return (SidConfig::MOS8580 == runmodel) ? 0x02 : 0x03;
        // }
    }
    uint8_t sid = (sidno > (USBSID::m_sidsUsed - 1)) ? (USBSID::m_sidsUsed - 1) : sidno;
    uint_least8_t address = ((0x20 * sid) + addr);
    DBG("READ: $%02X:%02X ", address, 0xFF);
    event_clock_t cycles = delay();
    if (cycles) {
        m_sid.USBSID_WaitForCycle(cycles);
        // if (m_isthreaded == 1) m_sid.USBSID_RingPushCycled(0xFF, 0xFF, cycles);
    }

    // if (readflag && !m_isthreaded) {
    // }
    return busValue;  // always return the busValue for now ~ need to fix and finish this!
}

void USBSID::write(uint_least8_t addr, uint8_t data)
{
    busValue = data;
    /* NASTY WORKAROUND */
    uint8_t sid = (sidno > (USBSID::m_sidsUsed - 1)) ? (USBSID::m_sidsUsed - 1) : sidno;
    uint_least8_t address = ((0x20 * sid) + addr);

    if (addr > 0x18)
        return;

    event_clock_t cycles = delay();
    if (cycles) {
        m_sid.USBSID_WaitForCycle(cycles);
    }
    /* WOWZERS! THAT'S A LOT OF DIFFERENT WRITE TYPES! */
    /* TODO: CLEAN THIS SHIT UP */
    if (m_isthreaded == 1 && m_iscycled == 1) {
        m_sid.USBSID_WriteRingCycled(address, data, cycles);
        DBG("WRITE THREADED CYCLED: $%02X:%02X %lu\n", address, data, cycles);
    };
    if (m_isthreaded != 1 && m_iscycled == 1) {
        m_sid.USBSID_WriteCycled(address, data, cycles);
        DBG("WRITE CYCLED: $%02X:%02X %lu\n", address, data, cycles);
    };
    if (m_isthreaded == 1 && m_iscycled != 1) {
        m_sid.USBSID_WriteRing(address, data);
        DBG("WRITE THREADED: $%02X:%02X\n", address, data);
    };
    if (m_isthreaded != 1 && m_iscycled != 1) {
        m_sid.USBSID_Write(address, data);
        DBG("WRITE: $%02X:%02X\n", address, data);
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
        refresh_rate = m_sid.USBSID_GetRefreshRate();  /* Get the USBSID internal refresh rate */
    }
}

void USBSID::event()
{
    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
    if (cycles < refresh_rate)
    {
        eventScheduler->schedule(*this, refresh_rate - cycles, EVENT_CLOCK_PHI1);
    }
    else
    {
        m_accessClk += cycles;
        m_sid.USBSID_Flush();
        eventScheduler->schedule(*this, refresh_rate, EVENT_CLOCK_PHI1);
    }
}

void USBSID::filter(bool enable)
{
    DBG("[%s] dafuq is this?\n", __func__);  // TODO: UPDATE ME
}

void USBSID::flush() /* Only gets call on player exit!? */
{
    m_sid.USBSID_Flush();
}

/* bool USBSID::lock(EventScheduler* env)
{
    sidemu::lock(env);
    eventScheduler->schedule(*this, refresh_rate, EVENT_CLOCK_PHI1);

    return true;
} */

/* void USBSID::unlock()
{
    eventScheduler->cancel(*this);
    sidemu::unlock();
} */

} /* libsidplayfp */
