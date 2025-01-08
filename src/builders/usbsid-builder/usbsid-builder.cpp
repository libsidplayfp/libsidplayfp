
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

USBSIDBuilder::USBSIDBuilder(const char * const name) :
    sidbuilder(name)
{
    if (!m_initialised)
    {
        m_initialised = true;
    }
}

USBSIDBuilder::~USBSIDBuilder()
{
    /* Remove all SID objects */
    remove();
}

unsigned int USBSIDBuilder::create(unsigned int sids)  /* Always uses the maximum amount of available SIDS */
{
    m_status = true;

    /* Check available devices */
    unsigned int count = availDevices();

    if (count && (count < sids))
        sids = count;

    for (count = 0; count < sids; count++)
    {
        /* Always init a new Object */
        try
        {
            /* sidobjs.insert(new libsidplayfp::USBSID(this, m_isthreaded, count)); */
            std::unique_ptr<libsidplayfp::USBSID> sid(new libsidplayfp::USBSID(this, m_isthreaded, m_iscycled, count));

            // SID init failed?
            if (!sid->getStatus())
            {
                m_errorBuffer = sid->error();
                m_status = false;
                return count;
            }
            sidobjs.insert(sid.release());
        }
        /* Memory alloc failed? */
        catch (std::bad_alloc const &)
        {
            m_errorBuffer.assign(name()).append(" ERROR: Unable to create USBSID object");
            m_status = false;
            break;
        }
    }
    return count;
}

unsigned int USBSIDBuilder::availDevices() const
{
    return 0;  /* 0 means endless devices left */
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
