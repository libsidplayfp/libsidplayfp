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
    sidbuilder(name),
    m_session(new libsidplayfp::USBSIDSession)
{}

USBSIDBuilder::~USBSIDBuilder()
{
    /* Remove all SID objects before the session they use */
    remove();
    delete m_session;
}

libsidplayfp::sidemu* USBSIDBuilder::create()
{
    /* Always init a new Object */
    try
    {
        /* Boards open on the first SID and stay open for the builder's lifetime */
        if (!m_session->open(m_errorBuffer))
            return nullptr;

        std::unique_ptr<libsidplayfp::USBSID> sid(new libsidplayfp::USBSID(this, *m_session));

        // SID init failed?
        if (!sid->getStatus())
        {
            m_errorBuffer = sid->error();
            return nullptr;
        }
        return sid.release();
    }
    /* Memory alloc failed? */
    catch (std::bad_alloc const &)
    {
        m_errorBuffer.assign(name()).append(" ERROR: Unable to create USBSID object");
        return nullptr;
    }
}

const char *USBSIDBuilder::getCredits() const
{
    return libsidplayfp::USBSID::getCredits();
}

void USBSIDBuilder::flush()
{
    m_session->flush();
}

void USBSIDBuilder::filter (bool enable)
{
    for (libsidplayfp::sidemu* e: sidobjs)
        static_cast<libsidplayfp::USBSID*>(e)->filter(enable);
}

void USBSIDBuilder::boards(const std::vector<std::string> &serials)
{
    m_session->serials(serials);
}

std::vector<std::string> USBSIDBuilder::listBoards()
{
    std::vector<std::string> serials;
    for (const USBSID_NS::USBSID_DeviceInfo &info: USBSID_Manager::Enumerate())
        serials.push_back(info.serial);
    return serials;
}
