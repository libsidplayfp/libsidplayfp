
#include "usbsid.h"
#include "usbsid-emu.h"

#include <cstdio>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>
#include <iostream>

#include "driver/USBSID.h"
// #include "driver/USBSIDInterface.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

unsigned int USBSID::m_sidFree[4] = {0,0,0,0};
unsigned int USBSID::m_sidsUsed = 0;
bool USBSID::m_sidInitDone = false;

const char* USBSID::getCredits()
{
    return
        "USBSID V" VERSION " Engine:\n"
        "\t(C) 2024 LouD\n";
}

USBSID::USBSID(sidbuilder *builder, bool threaded, unsigned int count) :
    sidemu(builder),
    Event("USBSID Delay"),
    m_sid(*(new USBSID_NS::USBSID_Class)),
    m_isthreaded(false),
    readflag(false),
    busValue(0),
    sidno(count)
{
    if (sidno < USBSID_MAXSID) {  /* Any sids available? */
        if (m_sidFree[sidno] == 0) m_sidFree[sidno] = 1;
    } else { /* All sids in use */
        return;
    }
    printf("[%s] sidno:%d m_sidFree[all]:[%d%d%d%d]\n",
        __func__, sidno, m_sidFree[0], m_sidFree[1], m_sidFree[2], m_sidFree[3]);

    if (!m_sid.us_Initialised)
    {
        m_error = "out of memory";
        return;
    }

    /* Only start the object driver once */
    if (sidno == 0) {
        /* Set threaded option */
        m_isthreaded = threaded;
        /* Start the fucker */
        if (m_sid.USBSID_Init(m_isthreaded) < 0)
        {
            m_error = "USBSID init failed";
            return;
        }
        // m_instance = 1;  /* USB device access ~ finish this later */
    }

    /* NASTY WORKAROUND */
    if(USBSID::m_sidInitDone == false) {
        reset(0);
        USBSID::m_sidInitDone = true; // update the static member here
    }
}

USBSID::~USBSID()
{
    printf("[%s] BREAKDOWN POO! sidno:%d\n", __func__, sidno);
    m_sidFree[sidno] = 0;
    if (sidno == 0) {
        reset(0);
        delete &m_sid;
    }
}

void USBSID::reset(uint8_t volume)
{
    using namespace std;
    (void)volume;

    /* NASTY WORKAROUND */
    if (USBSID::m_sidInitDone == true) {
        USBSID::m_sidsUsed++;
    }
    uint8_t sid = (sidno > (USBSID::m_sidsUsed - 1)) ? (USBSID::m_sidsUsed - 1) : sidno;
    printf("[%s] m_sidInitDone:%d m_sidsUsed:%d sid:%d sidno:%d volume:%X", __func__, USBSID::m_sidInitDone, USBSID::m_sidsUsed, sid, sidno, volume);
    std::cout << " address of `this`: " << this << '\n';

    m_accessClk = 0;
    readflag = false;
    m_sid.USBSID_Reset();
    m_sid.USBSID_Write(0x18, volume, 0); /* Testing volume writes */
}

event_clock_t USBSID::delay()
{

    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
    m_accessClk += cycles;
    while (cycles > 0xffff)
    {
        if (cycles > 0) {
            m_sid.USBSID_WaitForCycle(0xffff);
        }
        cycles -= 0xffff;
    }
    return static_cast<unsigned int>(cycles);
}

void USBSID::clock()
{
    const unsigned int cycles = delay();
    if (cycles) m_sid.USBSID_WaitForCycle(cycles);
}

uint8_t USBSID::read(uint_least8_t addr)
{
    if ((addr < 0x19) || (addr > 0x1C))
    {
        return busValue;
    }

    if (!readflag)
    {
        readflag = true;
        // Here we can implement any "safe" detection routine return values if we want to
        // if (0x1b == addr) {	// we could implement a commandline-chosen return byte here
            // return (SidConfig::MOS8580 == runmodel) ? 0x02 : 0x03;
        // }
    }

    /* const unsigned int cycles = delay();
    if (cycles) {  //TODO: change this to be only needed if there is to be an external cycle delayer (lol)
        m_sid.USBSID_WaitForCycle(cycles);
    } */

    clock();

    if (readflag && !m_isthreaded) {

        /* printf("R%02X\n", addr); */
        // USBSID_clkdread(usbsid, cycles, addr, &busValue);	// busValue is updated on valid reads
        // return busValue;
    }
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

    const unsigned int cycles = delay();
    if (cycles) {  /* TODO: change this to be only needed if there is to be an external cycle delayer (lol) */
        m_sid.USBSID_WaitForCycle(cycles);
    }

    /* clock(); */
    if (!m_isthreaded) m_sid.USBSID_Write(address, data, cycles);
    if (m_isthreaded) m_sid.USBSID_RingPush(address, data, cycles);
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
    if (sidno == 0) m_sid.USBSID_SetClockRate((long)systemclock);  /* Set the USBSID internal oscillator speed */
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
        m_sid.USBSID_WaitForCycle(cycles);
        eventScheduler->schedule(*this, USBSID_DELAY_CYCLES, EVENT_CLOCK_PHI1);
    }
}

void USBSID::flush() /* Only gets call on player exit!? */
{
    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
    printf("[%s] dafuq is this? %lu\n", __func__, cycles);  // TODO: REMOVE ME
}

}
