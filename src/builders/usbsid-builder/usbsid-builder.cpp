
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <sstream>
#include <string>

#include "usbsid.h"
#include "usbsid-emu.h"

bool USBSIDBuilder::m_initialised = false;
unsigned int USBSIDBuilder::m_count = 0;

USBSIDBuilder::USBSIDBuilder(const char * const name) :
    sidbuilder(name)
{
    if (!m_initialised)
    {
        m_count = USBSID_MAXSID;  /* Should refer to max sids per device! */
        m_initialised = true;
    }
    printf("[%s %s] m_initialised:%d m_count:%d m_status:%d m_isthreaded:%d\n",
        __func__, name,
        this->m_initialised, this->m_count, this->m_status, this->m_isthreaded);
}

USBSIDBuilder::~USBSIDBuilder()
{
    // Remove all SID emulations
    remove();
}

// Create a new sid emulation.  Called by libsidplay2 only
unsigned int USBSIDBuilder::create(unsigned int sids)  /* Always uses the maximum amount of available SIDS */
{
    printf("[%s]A sids:%d count:- m_status:%d m_count:%d m_initialised:%d\n",
        __func__,
         sids, this->m_status, this->m_count, this->m_initialised);
    m_status = true;

    // Check available devices
    unsigned int count = availDevices();

    printf("[%s]B sids:%d count:%d m_status:%d m_count:%d m_initialised:%d\n",
            __func__,
            sids, count, this->m_status, this->m_count, this->m_initialised);

    if (count == 0)
    {
        m_errorBuffer.assign(name()).append(" ERROR: No devices found");
        goto USBSIDBuilder_create_error;
    }

    if (count < sids)
        sids = count;

    for (count = 0; count < sids; count++)
    {
        try
        {
            /* Always inits a new Object */
            // this->m_name;
            std::unique_ptr<libsidplayfp::USBSID> sid(new libsidplayfp::USBSID(this, m_isthreaded));
            // SID init failed?
            if (!sid->getStatus())
            {
                m_errorBuffer = sid->error();
                goto USBSIDBuilder_create_error;
            }

            sidobjs.insert(sid.release());
            int c = (int)count;
            auto val = sidobjs.find(0);
            printf("[%s]C %d sids:%d count:%d m_status:%d m_count:%d m_initialised:%d\n",
            __func__, val,
            sids, count, this->m_status, this->m_count, this->m_initialised);
        }
        // Memory alloc failed?
        catch (std::bad_alloc const &)
        {
            m_errorBuffer.assign(name()).append(" ERROR: Unable to create USBSID object");
            goto USBSIDBuilder_create_error;
        }
    }
    printf("[%s]D sids:%d count:%d m_status:%d m_count:%d m_initialised:%d\n",
            __func__,
            sids, count, this->m_status, this->m_count, this->m_initialised);

USBSIDBuilder_create_error:
    if (count == 0)
        m_status = false;
    return count;
}

unsigned int USBSIDBuilder::availDevices() const
{
    return m_count;
}

const char *USBSIDBuilder::credits() const
{
    return libsidplayfp::USBSID::getCredits();
}

void USBSIDBuilder::flush()
{
    for (libsidplayfp::sidemu* e: sidobjs)
        static_cast<libsidplayfp::USBSID*>(e)->flush();
}

void USBSIDBuilder::filter (bool enable)
{
    for (libsidplayfp::sidemu* e: sidobjs)
        static_cast<libsidplayfp::USBSID*>(e)->filter(enable);
}
