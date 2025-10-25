
#include "usbsid.h"
#include "usbsid-emu.h"

#include <cstdio>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>

#include "driver/src/USBSID.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

static long raster_rate = 19950; /* Start on PAL */

const char* USBSID::getCredits()
{
    return
        "USBSID V" VERSION " Engine:\n"
        "\t(C) 2024-2025 LouD\n";
}

USBSID::USBSID(sidbuilder *builder) :
    sidemu(builder),
    Event("USBSID Delay"),
    m_handle(-1),
    m_sid(*(new USBSID_NS::USBSID_Class)),
    busValue(0),
    sidno(0)
{
    sidno = m_sid.us_InstanceID;

    if (!m_sid.us_Initialised)
    {
        m_error = "out of memory";
        return;
    }

    /* Start the fucker */
    m_handle = m_sid.USBSID_Init(true, true);
    if (m_handle < 0)
    {
        m_error = "USBSID init failed";
        return;
    }

    if (sidno == 0) {
        m_sid.USBSID_Mute();
        m_sid.USBSID_ClearBus();
        raster_rate = m_sid.USBSID_GetRasterRate();    /* Get the USBSID internal raster rate */
        m_sid.USBSID_SetBufferSize(8192);
        m_sid.USBSID_SetDiffSize(64);
        m_sid.USBSID_RestartRingBuffer();
    }
}

USBSID::~USBSID()
{
    if (sidno == 0) {
        reset(0);
        delete &m_sid;
    }
}

void USBSID::reset(uint8_t volume)
{
    using namespace std;

    m_accessClk = 0;
    if (sidno == 0) {
        m_sid.USBSID_Reset();
        if (eventScheduler != nullptr) {
            eventScheduler->schedule(*this, raster_rate, EVENT_CLOCK_PHI1);
        }
    }
}

event_clock_t USBSID::delay()
{
    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - (m_accessClk - 1);
    m_accessClk += cycles;
    while (cycles > 0xffff)
    {
        if (sidno == 0) m_sid.USBSID_WaitForCycle(0xffff);
        cycles -= 0xffff;
    }
    if (sidno == 0) m_sid.USBSID_WaitForCycle(cycles);
    return cycles;
}

uint8_t USBSID::read(uint_least8_t addr)
{
    return busValue;  /* Always return the busValue */
}

void USBSID::write(uint_least8_t addr, uint8_t data)
{
    busValue = data;
    if (addr > 0x18)
        return;

    const unsigned int cycles = delay();
    uint_least8_t address = ((0x20 * sidno) + addr);
    m_sid.USBSID_WriteRingCycled(address, data, cycles);
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
        m_sid.USBSID_SetClockRate((long)systemclock, true);  /* Set the USBSID internal oscillator speed */
        raster_rate = m_sid.USBSID_GetRasterRate();    /* Get the USBSID internal raster rate */
    }
}

void USBSID::event()
{
    if (sidno == 0) {
        event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
        if (cycles < raster_rate)
        {
            eventScheduler->schedule(*this, raster_rate - cycles, EVENT_CLOCK_PHI1);
        }
        else
        {
            m_accessClk += cycles;
            if (sidno == 0) {
                m_sid.USBSID_Flush();
                m_sid.USBSID_WaitForCycle(cycles);
            }
            eventScheduler->schedule(*this, raster_rate, EVENT_CLOCK_PHI1);
        }
    }
}

void USBSID::filter(bool enable)
{
 (void) enable;
}

void USBSID::flush() /* Only gets call on player exit!? */
{
    m_sid.USBSID_Flush();
}

} /* libsidplayfp */
