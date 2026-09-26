
#ifndef  USBSID_H
#define  USBSID_H

#include <string>
#include <vector>

#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

/// SIDs addressable on a single board
#define USBSID_MAXSID 4

/// USBSIDBuilder::boards() and USBSIDBuilder::listBoards() are available
#define USBSID_MULTIBOARD 1

namespace libsidplayfp
{
class USBSIDSession;
}

class SID_EXTERN USBSIDBuilder : public sidbuilder
{
private:
    libsidplayfp::USBSIDSession *m_session;

protected:
    /**
     * Create the sid emu.
     */
    libsidplayfp::sidemu* create();

public:
    USBSIDBuilder(const char * const name);
    ~USBSIDBuilder();

    USBSIDBuilder(const USBSIDBuilder &) = delete;
    USBSIDBuilder &operator=(const USBSIDBuilder &) = delete;

    const char *getCredits() const;
    void flush();

    /**
     * enable/disable filter.
     */
    void filter(bool enable);

    /**
     * Select the boards to play on, by serial number.
     *
     * Empty (default) opens the first board in USB bus/port order, and
     * its four register blocks take up to USBSID_MAXSID SIDs.
     * With several boards, SIDs are assigned in list order, each board
     * in the SID order of its own config. A single listed board behaves
     * as the default. Takes effect before the first SID is created.
     *
     * @param serials serial numbers, from listBoards()
     */
    void boards(const std::vector<std::string> &serials);

    /**
     * List the serial numbers of every attached board.
     *
     * @return serial numbers in USB bus/port order
     */
    static std::vector<std::string> listBoards();
};

#endif // USBSID_H
