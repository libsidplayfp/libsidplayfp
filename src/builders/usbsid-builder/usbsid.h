#ifndef  USBSID_H
#define  USBSID_H

#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

#define USBSID_MAXSID 4

class SID_EXTERN USBSIDBuilder : public sidbuilder
{
private:
    static bool m_initialised;

    static unsigned int m_count;

public:
    USBSIDBuilder(const char * const name);
    ~USBSIDBuilder();

    bool m_isthreaded;

    /**
     * Available sids.
     *
     * @return the number of available sids, 0 = endless.
     */
    unsigned int availDevices() const;

    const char *credits() const;
    void flush();

    /**
     * enable/disable filter.
     */
    void filter(bool enable);

    /**
     * Create the sid emu.
     *
     * @param sids the number of required sid emu
     */
    unsigned int create(unsigned int sids);
};

#endif // USBSID_H
