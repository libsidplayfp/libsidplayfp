/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012 Leando Nini <drfiemost@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MUS_H
#define MUS_H

#include "SidTuneBase.h"

class MUS : public SidTuneBase
{
 private:
    //uint_least32_t fileOffset;

 private:
    bool resolveAddrs(const uint_least8_t *c64data);
    bool checkRelocInfo(void);

    void tryLoad(Buffer_sidtt<const uint_least8_t>& dataBuf);

    static bool detect(const uint_least8_t* buffer, const uint_least32_t bufLen,
                         uint_least32_t& voice3Index);

    static bool fileSupport(Buffer_sidtt<const uint_least8_t>& musBuf,
                                             Buffer_sidtt<const uint_least8_t>& strBuf);

    bool mergeParts(Buffer_sidtt<const uint_least8_t>& musBuf,
                             Buffer_sidtt<const uint_least8_t>& strBuf);

    void tryLoad(Buffer_sidtt<const uint_least8_t>& musBuf,
                                       Buffer_sidtt<const uint_least8_t>& strBuf,
                                       SmartPtr_sidtt<const uint8_t> &spPet,
                                       uint_least32_t voice3Index,
                                       bool init);

 protected:
    MUS() {}

    void installPlayer(uint_least8_t *c64buf);

    void setPlayerAddress();

    virtual bool acceptSidTune(const char* dataFileName, const char* infoFileName,
                       Buffer_sidtt<const uint_least8_t>& buf);

 public:
    virtual ~MUS() {}

    static SidTuneBase* load(Buffer_sidtt<const uint_least8_t>& dataBuf, bool init = false);
    static SidTuneBase* load(Buffer_sidtt<const uint_least8_t>& musBuf,
                                       Buffer_sidtt<const uint_least8_t>& strBuf,
                                       const uint_least32_t fileOffset,
                                       bool init = false);

    virtual bool placeSidTuneInC64mem(uint_least8_t* c64buf);
};

#endif // PSID_H
