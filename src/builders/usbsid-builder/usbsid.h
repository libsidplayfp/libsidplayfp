#ifndef  USBSID_H
#define  USBSID_H

#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

#define USBSID_MAXSID 4

class SID_EXTERN USBSIDBuilder : public sidbuilder
{
public:
    USBSIDBuilder(const char * const name);
    ~USBSIDBuilder();

    /**
     * Available sids.
     *
     * @return the number of available sids, 0 = endless.
     */
    unsigned int availDevices() const { return USBSID_MAXSID; }

    /**
     * Create the sid emu.
     *
     * @param sids the number of required sid emu
     */
    unsigned int create(unsigned int sids);

    const char *credits() const;

    void flush();

    /**
     * enable/disable filter.
     */
    void filter(bool enable);

};

#endif // USBSID_H
