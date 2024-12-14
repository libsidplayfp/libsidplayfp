
#include "usbsid.h"
#include "usbsid-emu.h"

#include <cstdio>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>

#include "driver/USBSID.h"
// #include "driver/USBSIDInterface.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

unsigned int USBSID::m_sidFree[4] = {0,0,0,0};
unsigned int USBSID::sid = 0;
unsigned int USBSID::num = USBSID_MAXSID;
unsigned int USBSID::m_instance = 0;

const char* USBSID::getCredits()
{
    return
        "USBSID V" VERSION " Engine:\n"
        "\t(C) 2024 LouD\n";
}

USBSID::USBSID(sidbuilder *builder, bool threaded) :
    sidemu(builder),
    Event("USBSID Delay"),
    m_status(false),
    m_isthreaded(false),
    readflag(false),
    busValue(0),
    m_handle(0)
    // m_instance(sid++)
{
    /* sid++; */
    printf("[%s %s %d]A sid:%d num:%d m_sidFree:%d%d%d%d m_instance:%d m_status:%d\n",
        __func__, builder->name(), builder->usedDevices(), sid, num,
        m_sidFree[0], m_sidFree[1], m_sidFree[2], m_sidFree[3],
        this->m_instance, this->m_status);
    /* unsigned int num = 4; */
    for (int i = 0; i < 4; i++)
    {
        printf("[%s]A num:%d i:%d m_sidFree[i]:%d sid:%d\n", __func__, num, i, m_sidFree[i], sid);
        if (m_sidFree[i] == 0)
        {
            sid = i;
            m_sidFree[i] = 1;
            num = i;
            break;
        }
        printf("[%s]B num:%d i:%d m_sidFree[i]:%d sid:%d\n", __func__, num, i, m_sidFree[i], sid);
    }

    // All sids in use?!?
    if (num == 4)
        return;

    /* m_handle = num; */
    printf("[%s]B sid:%d num:%d m_sidFree:%d%d%d%d m_instance:%d m_status:%d\n",
        __func__, sid, num,
        m_sidFree[0], m_sidFree[1], m_sidFree[2], m_sidFree[3],
        this->m_instance, this->m_status);
    /* usbsid driver is initialized in usbsid-emu.h */
    if (!usbsid.us_Initialised)
    {
        m_error = "out of memory";
        return;
    }

    {
        /* Only start the object driver once */
        m_handle = sid;
        if (sid == 0 && m_instance == 0) {
            /* Set threaded option */
            m_isthreaded = threaded;
            /* Start the fucker */
            if (usbsid.USBSID_Init(m_isthreaded) < 0)
            {
                m_error = "USBSID init failed";
                return;
            }
            m_instance = 1;  /* TODO: Per USB devoce */
        }
    }
    printf("[%s]C m_handle:%d sid:%d num:%d m_sidFree:%d%d%d%d m_instance:%d m_status:%d\n",
        __func__, m_handle, sid, num,
        m_sidFree[0], m_sidFree[1], m_sidFree[2], m_sidFree[3],
        this->m_instance, this->m_status);
    m_status = true;
    sidemu::reset();
}

USBSID::~USBSID()
{
    printf("[%s] BREAKDOWN POO!\n", __func__);
    reset(0);
    /* sid--; */
    /* m_sidFree[m_instance] = 0; */
}

void USBSID::reset(uint8_t volume)
{

    (void)volume;
    usbsid.USBSID_Pause();
    usbsid.USBSID_Reset();

    m_accessClk = 0;
    readflag = false;
}

event_clock_t USBSID::delay()
{

    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
    m_accessClk += cycles;
    while (cycles > 0xffff)
    {
        if (cycles > 0) {
            usbsid.USBSID_WaitForCycle(0xffff);
            // usbsid.WaitForCycle(cycles);
        }
        cycles -= 0xffff;
    }
    return static_cast<unsigned int>(cycles);
}

void USBSID::clock()
{
    const unsigned int cycles = delay();
    if (cycles)
        usbsid.USBSID_WaitForCycle(cycles);
}

uint8_t USBSID::read(uint_least8_t addr)
{
    if ((addr < 0x19) || (addr > 0x1C))
    {
        return busValue;
    }

    if (!readflag)
    {
#ifdef DEBUG
        printf("\nWARNING: Read support is limited. This file may not play correctly!\n");
#endif
        /* NOTE: NOT USED OR IMPLEMENTED YET! */
        readflag = true;
        // Here we implement the "safe" detection routine return values
        // if (0x1b == addr) {	// we could implement a commandline-chosen return byte here
            // return (SidConfig::MOS8580 == runmodel) ? 0x02 : 0x03;
        // }
    }

    const unsigned int cycles = delay();

    if (readflag && !m_isthreaded) {

        /* printf("R%02X\n", addr); */
        // USBSID_clkdread(usbsid, cycles, addr, &busValue);	// busValue is updated on valid reads
        // return busValue;
    }
    return 0x0;
}

void USBSID::write(uint_least8_t addr, uint8_t data)
{
    busValue = data;
    /* printf("[W]$%04X$%02X:%d:%d:%d\n", addr, data, sid, m_instance, m_handle); */

    if (addr > 0x18)
        return;

    const unsigned int cycles = delay();
    if (cycles) {
        usbsid.USBSID_WaitForCycle(cycles);
    }

    if (!m_isthreaded) usbsid.USBSID_Write(addr, data, cycles);
    if (m_isthreaded) usbsid.USBSID_RingPush(addr, data, cycles);
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
    usbsid.USBSID_SetClockRate((long)systemclock);
    printf("[%s] m_handle:%d m_handle:%d sid:%d num:%d m_instance:%d m_status:%d m_sidFree:%d%d%d%d\n",
        __func__, m_handle, this->m_handle, this->sid, this->num,
        this->m_instance, this->m_status,
        m_sidFree[0], m_sidFree[1], m_sidFree[2], m_sidFree[3]);
}

void USBSID::event() /* Afaik only gets called from Sidplay2!? */
{
    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
    printf("[%s] dafuq is this? %lu\n", __func__, cycles);
    if (cycles < USBSID_DELAY_CYCLES)
    {
        eventScheduler->schedule(*this, USBSID_DELAY_CYCLES - cycles, EVENT_CLOCK_PHI1);
    }
    else
    {
        m_accessClk += cycles;
        usbsid.USBSID_WaitForCycle(cycles);
        eventScheduler->schedule(*this, USBSID_DELAY_CYCLES, EVENT_CLOCK_PHI1);
    }
}

void USBSID::flush() /* Only gets call on player exit!? */
{
    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
    printf("[%s] dafuq is this? %lu\n", __func__, cycles);  // TODO: REMOVE ME
}

bool USBSID::lock(EventScheduler* env)
{
    sidemu::lock(env);
    eventScheduler->schedule(*this, USBSID_DELAY_CYCLES, EVENT_CLOCK_PHI1);

    return true;
}

void USBSID::unlock()
{
    eventScheduler->cancel(*this);
    sidemu::unlock();
}

}
