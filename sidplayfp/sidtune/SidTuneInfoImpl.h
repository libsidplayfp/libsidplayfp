/*
 *  Copyright 2011-2012 Leandro Nini
 *  Copyright 2007-2010 Antti Lankila
 *  Copyright 2000 Simon White
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SIDTUNEINFOIMPL_H
#define SIDTUNEINFOIMPL_H

#include <stdint.h>
#include "sidplayfp/SidTuneInfo.h"

/** @internal
* The implementation of the SidTuneInfo interface.
*/
class SidTuneInfoImpl : public SidTuneInfo
{
    friend class SidTune;

private:
    const char* m_formatString;

    const char* m_statusString;

    const char* m_speedString;

    char* m_path;

    char* m_dataFileName;

    char* m_infoFileName;

    unsigned int m_numberOfInfoStrings;
    char* m_infoString[MAX_CREDIT_STRINGS];

    unsigned int m_numberOfCommentStrings;
    char** m_commentString;

    unsigned int m_songs;
    unsigned int m_startSong;
    unsigned int m_currentSong;

    int m_songSpeed;

    bool m_musPlayer;

    bool m_fixLoad;

    uint_least32_t m_dataFileLen;

    uint_least32_t m_c64dataLen;

    uint_least16_t m_loadAddr;
    uint_least16_t m_initAddr;
    uint_least16_t m_playAddr;

    uint_least16_t m_sidChipBase1;
    uint_least16_t m_sidChipBase2;

    clock_t m_clockSpeed;

    model_t m_sidModel1;
    model_t m_sidModel2;

    compatibility_t m_compatibility;

    uint_least8_t m_relocStartPage;

    uint_least8_t m_relocPages;

private:    // prevent copying
    SidTuneInfoImpl(const SidTuneInfoImpl&);
    SidTuneInfoImpl& operator=(SidTuneInfoImpl&);

public:
    SidTuneInfoImpl() :
        m_formatString("N/A"),
        m_statusString("N/A"),
        m_path(0),
        m_dataFileName(0),
        m_infoFileName(0),
        m_songs(0),
        m_startSong(0),
        m_currentSong(0),
        m_songSpeed(SPEED_VBI),
        m_musPlayer(false),
        m_fixLoad(false),
        m_dataFileLen(0),
        m_c64dataLen(0),
        m_loadAddr(0),
        m_initAddr(0),
        m_playAddr(0),
        m_sidChipBase1(0xd400),
        m_sidChipBase2(0),
        m_clockSpeed(CLOCK_UNKNOWN),
        m_sidModel1(SIDMODEL_UNKNOWN),
        m_sidModel2(SIDMODEL_UNKNOWN),
        m_compatibility(COMPATIBILITY_C64),
        m_relocStartPage(0),
        m_relocPages(0) {}

    uint_least16_t loadAddr() const { return m_loadAddr; }

    uint_least16_t initAddr() const { return m_initAddr; }

    uint_least16_t playAddr() const { return m_playAddr; }

    unsigned int songs() const { return m_songs; }

    unsigned int startSong() const { return m_startSong; }

    unsigned int currentSong() const { return m_currentSong; }

    uint_least16_t sidChipBase1() const { return m_sidChipBase1; }
    uint_least16_t sidChipBase2() const { return m_sidChipBase2; }

    bool isStereo() const { return (m_sidChipBase1!=0 && m_sidChipBase2!=0); }

    int songSpeed() const { return m_songSpeed; }

    uint_least8_t relocStartPage() const { return m_relocStartPage; }

    uint_least8_t relocPages() const { return m_relocPages; }

    model_t sidModel1() const { return m_sidModel1; }
    model_t sidModel2() const { return m_sidModel2; }

    compatibility_t compatibility() const { return m_compatibility; }

    unsigned int numberOfInfoStrings() const { return m_numberOfInfoStrings; }
    const char* infoString(const unsigned int i) const { return i<m_numberOfInfoStrings?m_infoString[i]:""; }

    unsigned int numberOfCommentStrings() const { return m_numberOfCommentStrings; } 
    const char* commentString(const unsigned int i) const { return i<m_numberOfCommentStrings?m_commentString[i]:""; }

    uint_least32_t dataFileLen() const { return m_dataFileLen; }

    uint_least32_t c64dataLen() const { return m_c64dataLen; }

    clock_t clockSpeed() const { return m_clockSpeed; }

    const char* statusString() const { return m_statusString; }

    const char* formatString() const { return m_formatString; }

    const char* speedString() const { return m_speedString; }

    bool musPlayer() const { return m_musPlayer; }

    bool fixLoad() const { return m_fixLoad; }

    const char* path() const { return m_path; }

    const char* dataFileName() const { return m_dataFileName; }

    const char* infoFileName() const { return m_infoFileName; }
};

#endif  /* SIDTUNEINFOIMPL_H */
