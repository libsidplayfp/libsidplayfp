
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <sstream>
#include <string>

#include "usbsid.h"
#include "usbsid-emu.h"


USBSIDBuilder::USBSIDBuilder(const char * const name) :
    sidbuilder(name)
{}

USBSIDBuilder::~USBSIDBuilder()
{
    /* Remove all SID objects */
    remove();
}

// Create a new sid emulation.
unsigned int USBSIDBuilder::create(unsigned int sids)
{
    m_status = true;

    // Check available devices
    unsigned int count = availDevices();

    if (count && (count < sids))
        sids = count;

    for (count = 0; count < sids; count++)
    {
        try
        {
            std::unique_ptr<libsidplayfp::USBSID> sid(new libsidplayfp::USBSID(this));

            // SID init failed?
            if (!sid->getStatus())
            {
                m_errorBuffer = sid->error();
                m_status = false;
                return count;
            }
            sidobjs.insert(sid.release());
        }
        // Memory alloc failed?
        catch (std::bad_alloc const &)
        {
            m_errorBuffer.assign(name()).append(" ERROR: Unable to create USBSID object");
            m_status = false;
            break;
        }
    }
    return count;

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
