/*
 * /home/ms/files/source/libsidtune/RCS/PSID.cpp,v
 *
 * PlaySID one-file format support.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "SidTuneCfg.h"
#include "SidTuneInfoImpl.h"
#include "sidplayfp/SidTune.h"
#include "sidplayfp/sidendian.h"

#define PSID_ID 0x50534944
#define RSID_ID 0x52534944

// Header has been extended for 'RSID' format
// The following changes are present:
//     id = 'RSID'
//     version = 2 only
//     play, load and speed reserved 0
//     psidspecific flag reserved 0
//     init cannot be under ROMS/IO
//     load cannot be less than 0x0801 (start of basic)

struct psidHeader           // all values big-endian
{
    char id[4];             // 'PSID' (ASCII)
    uint8_t version[2];     // 0x0001 or 0x0002
    uint8_t data[2];        // 16-bit offset to binary data in file
    uint8_t load[2];        // 16-bit C64 address to load file to
    uint8_t init[2];        // 16-bit C64 address of init subroutine
    uint8_t play[2];        // 16-bit C64 address of play subroutine
    uint8_t songs[2];       // number of songs
    uint8_t start[2];       // start song out of [1..256]
    uint8_t speed[4];       // 32-bit speed info
                            // bit: 0=50 Hz, 1=CIA 1 Timer A (default: 60 Hz)
    char name[32];          // ASCII strings, 31 characters long and
    char author[32];        // terminated by a trailing zero
    char released[32];      //
    uint8_t flags[2];       // only version >= 0x0002
    uint8_t relocStartPage; // only version >= 0x0002B
    uint8_t relocPages;     // only version >= 0x0002B
    char sidChipBase2;      // only version >= 0x0003
    char reserved;          // only version >= 0x0002
};

enum
{
    PSID_MUS       = 1 << 0,
    PSID_SPECIFIC  = 1 << 1, // These two are mutally exclusive
    PSID_BASIC     = 1 << 1,
    PSID_CLOCK     = 3 << 2,
    PSID_SIDMODEL  = 3 << 4
};

enum
{
    PSID_CLOCK_UNKNOWN = 0,
    PSID_CLOCK_PAL     = 1 << 2,
    PSID_CLOCK_NTSC    = 1 << 3,
    PSID_CLOCK_ANY     = PSID_CLOCK_PAL | PSID_CLOCK_NTSC
};

enum
{
    PSID_SIDMODEL1_UNKNOWN = 0,
    PSID_SIDMODEL1_6581    = 1 << 4,
    PSID_SIDMODEL1_8580    = 1 << 5,
    PSID_SIDMODEL1_ANY     = PSID_SIDMODEL1_6581 | PSID_SIDMODEL1_8580
};

enum
{
    PSID_SIDMODEL2_UNKNOWN = 0,
    PSID_SIDMODEL2_6581    = 1 << 6,
    PSID_SIDMODEL2_8580    = 1 << 7,
    PSID_SIDMODEL2_ANY     = PSID_SIDMODEL2_6581 | PSID_SIDMODEL2_8580
};

static const char _sidtune_format_psid[] = "PlaySID one-file format (PSID)";
static const char _sidtune_format_rsid[] = "Real C64 one-file format (RSID)";
static const char _sidtune_unknown_psid[] = "Unsupported PSID version";
static const char _sidtune_unknown_rsid[] = "Unsupported RSID version";
static const char _sidtune_truncated[] = "ERROR: File is most likely truncated";
static const char _sidtune_invalid[] = "ERROR: File contains invalid data";

static const int _sidtune_psid_maxStrLen = 32;


SidTune::LoadStatus SidTune::PSID_fileSupport(Buffer_sidtt<const uint_least8_t>& dataBuf)
{
    const uint_least32_t bufLen = dataBuf.len();

    // File format check
    if (bufLen<6)
        return LOAD_NOT_MINE;

    SidTuneInfo::clock_t clock = SidTuneInfo::CLOCK_UNKNOWN;
    SidTuneInfo::compatibility_t compatibility = SidTuneInfo::COMPATIBILITY_C64;

    // Require minimum size to allow access to the first few bytes.
    // Require a valid ID and version number.
    const psidHeader* pHeader = (const psidHeader*)dataBuf.get();

    if (endian_big32((const uint_least8_t*)pHeader->id)==PSID_ID)
    {
       switch (endian_big16(pHeader->version))
       {
       case 1:
           compatibility = SidTuneInfo::COMPATIBILITY_PSID;
           // Deliberate run on
       case 2:
       case 3:
           break;
       default:
           info->m_formatString = _sidtune_unknown_psid;
           return LOAD_ERROR;
       }
       info->m_formatString = _sidtune_format_psid;
    }
    else if (endian_big32((const uint_least8_t*)pHeader->id)==RSID_ID)
    {
       switch (endian_big16(pHeader->version))
       {
       case 2:
       case 3:
           break;
       default:
           info->m_formatString = _sidtune_unknown_rsid;
           return LOAD_ERROR;
       }
       info->m_formatString = _sidtune_format_rsid;
       compatibility = SidTuneInfo::COMPATIBILITY_R64;
    }
    else
    {
        return LOAD_NOT_MINE;
    }

    // Due to security concerns, input must be at least as long as version 1
    // header plus 16-bit C64 load address. That is the area which will be
    // accessed.
    if ( bufLen < (sizeof(psidHeader)+2) )
    {
        info->m_formatString = _sidtune_truncated;
        return LOAD_ERROR;
    }

    fileOffset         = endian_big16(pHeader->data);
    info->m_loadAddr      = endian_big16(pHeader->load);
    info->m_initAddr      = endian_big16(pHeader->init);
    info->m_playAddr      = endian_big16(pHeader->play);
    info->m_songs         = endian_big16(pHeader->songs);
    info->m_startSong     = endian_big16(pHeader->start);
    info->m_sidChipBase1  = 0xd400;
    info->m_sidChipBase2  = 0;
    info->m_compatibility = compatibility;

    uint_least32_t speed = endian_big32(pHeader->speed);

    if (info->m_songs > MAX_SONGS)
    {
        info->m_songs = MAX_SONGS;
    }

    info->m_musPlayer      = false;
    info->m_sidModel1      = SidTuneInfo::SIDMODEL_UNKNOWN;
    info->m_sidModel2      = SidTuneInfo::SIDMODEL_UNKNOWN;
    info->m_relocPages     = 0;
    info->m_relocStartPage = 0;
    if ( endian_big16(pHeader->version) >= 2 )
    {
        const uint_least16_t flags = endian_big16(pHeader->flags);
        if (flags & PSID_MUS)
        {   // MUS tunes run at any speed
            clock = SidTuneInfo::CLOCK_ANY;
            info->m_musPlayer = true;
        }

        // This flags is only available for the appropriate
        // file formats
        switch (compatibility)
        {
        case SidTuneInfo::COMPATIBILITY_C64:
            if (flags & PSID_SPECIFIC)
                info->m_compatibility = SidTuneInfo::COMPATIBILITY_PSID;
            break;
        case SidTuneInfo::COMPATIBILITY_R64:
            if (flags & PSID_BASIC)
                info->m_compatibility = SidTuneInfo::COMPATIBILITY_BASIC;
            break;
        }

        if ((flags & PSID_CLOCK_ANY) == PSID_CLOCK_ANY)
            clock = SidTuneInfo::CLOCK_ANY;
        else if (flags & PSID_CLOCK_PAL)
            clock = SidTuneInfo::CLOCK_PAL;
        else if (flags & PSID_CLOCK_NTSC)
            clock = SidTuneInfo::CLOCK_NTSC;
        info->m_clockSpeed = clock;

        if ((flags & PSID_SIDMODEL1_ANY) == PSID_SIDMODEL1_ANY)
            info->m_sidModel1 = SidTuneInfo::SIDMODEL_ANY;
        else if (flags & PSID_SIDMODEL1_6581)
            info->m_sidModel1 = SidTuneInfo::SIDMODEL_6581;
        else if (flags & PSID_SIDMODEL1_8580)
            info->m_sidModel1 = SidTuneInfo::SIDMODEL_8580;
        else
            info->m_sidModel1 = SidTuneInfo::SIDMODEL_UNKNOWN;

        if ((flags & PSID_SIDMODEL2_ANY) == PSID_SIDMODEL2_ANY)
            info->m_sidModel2 = SidTuneInfo::SIDMODEL_ANY;
        else if (flags & PSID_SIDMODEL2_6581)
            info->m_sidModel2 = SidTuneInfo::SIDMODEL_6581;
        else if (flags & PSID_SIDMODEL2_8580)
            info->m_sidModel2 = SidTuneInfo::SIDMODEL_8580;
        else
            info->m_sidModel2 = SidTuneInfo::SIDMODEL_UNKNOWN;

        info->m_relocStartPage = pHeader->relocStartPage;
        info->m_relocPages     = pHeader->relocPages;

        if ( endian_big16(pHeader->version) >= 3 )
        {
            info->m_sidChipBase2 = 0xd000 | (pHeader->sidChipBase2<<4);
        }
    }

    // Check reserved fields to force real c64 compliance
    // as required by the RSID specification
    if (compatibility == SidTuneInfo::COMPATIBILITY_R64)
    {
        if ((info->m_loadAddr != 0) ||
            (info->m_playAddr != 0) ||
            (speed != 0))
        {
            info->m_formatString = _sidtune_invalid;
            return LOAD_ERROR;
        }
        // Real C64 tunes appear as CIA
        speed = ~0;
    }
    // Create the speed/clock setting table.
    convertOldStyleSpeedToTables(speed, clock);

    // Copy info strings, so they will not get lost.
    info->m_numberOfInfoStrings = 3;
    // Name
    strncpy(&infoString[0][0],pHeader->name,_sidtune_psid_maxStrLen);
    info->m_infoString[0] = &infoString[0][0];
    // Author
    strncpy(&infoString[1][0],pHeader->author,_sidtune_psid_maxStrLen);
    info->m_infoString[1] = &infoString[1][0];
    // Released
    strncpy(&infoString[2][0],pHeader->released,_sidtune_psid_maxStrLen);
    info->m_infoString[2] = &infoString[2][0];

    if ( info->m_musPlayer )
        return MUS_load (dataBuf);
    return LOAD_OK;
}


bool SidTune::PSID_fileSupportSave(std::ofstream& fMyOut, const uint_least8_t* dataBuffer)
{
    psidHeader myHeader;
    endian_big32((uint_least8_t*)myHeader.id,PSID_ID);
    endian_big16(myHeader.version,2);
    endian_big16(myHeader.data,sizeof(psidHeader));
    endian_big16(myHeader.songs,info->m_songs);
    endian_big16(myHeader.start,info->m_startSong);

    uint_least32_t speed = 0, check = 0;
    uint_least32_t maxBugSongs = ((info->m_songs <= 32) ? info->m_songs : 32);
    for (uint_least32_t s = 0; s < maxBugSongs; s++)
    {
        if (songSpeed[s] == SidTuneInfo::SPEED_CIA_1A)
            speed |= (1<<s);
        check |= (1<<s);
    }
    endian_big32(myHeader.speed,speed);

    uint_least16_t tmpFlags = 0;
    if ( info->m_musPlayer )
    {
        endian_big16(myHeader.load,0);
        endian_big16(myHeader.init,0);
        endian_big16(myHeader.play,0);
        myHeader.relocStartPage = 0;
        myHeader.relocPages     = 0;
        tmpFlags |= PSID_MUS;
    }
    else
    {
        endian_big16(myHeader.load,0);
        endian_big16(myHeader.init,info->m_initAddr);
        myHeader.relocStartPage = info->m_relocStartPage;
        myHeader.relocPages     = info->m_relocPages;

        switch (info->m_compatibility)
        {
        case SidTuneInfo::COMPATIBILITY_BASIC:
            tmpFlags |= PSID_BASIC;
        case SidTuneInfo::COMPATIBILITY_R64:
            endian_big32((uint_least8_t*)myHeader.id,RSID_ID);
            endian_big16(myHeader.play,0);
            endian_big32(myHeader.speed,0);
            break;
        case SidTuneInfo::COMPATIBILITY_PSID:
            tmpFlags |= PSID_SPECIFIC;
        default:
            endian_big16(myHeader.play,info->m_playAddr);
            break;
        }
    }

    for ( unsigned int i = 0; i < 32; i++ )
    {
        myHeader.name[i] = 0;
        myHeader.author[i] = 0;
        myHeader.released[i] = 0;
    }

    // @FIXME@ Need better solution.  Make it possible to override MUS strings
    if ( info->m_numberOfInfoStrings == 3 )
    {
        strncpy( myHeader.name, info->m_infoString[0], _sidtune_psid_maxStrLen);
        strncpy( myHeader.author, info->m_infoString[1], _sidtune_psid_maxStrLen);
        strncpy( myHeader.released, info->m_infoString[2], _sidtune_psid_maxStrLen);
    }

    tmpFlags |= (info->m_clockSpeed << 2);
    tmpFlags |= (info->m_sidModel1 << 4);
    tmpFlags |= (info->m_sidModel2 << 6);
    endian_big16(myHeader.flags,tmpFlags);
    myHeader.sidChipBase2 = info->m_sidChipBase2;
    myHeader.reserved = 0;

    fMyOut.write( (char*)&myHeader, sizeof(psidHeader) );

    if (info->m_musPlayer)
        fMyOut.write( (const char*)dataBuffer, info->m_dataFileLen );  // !cast!
    else
    {   // Save C64 lo/hi load address (little-endian).
        uint_least8_t saveAddr[2];
        saveAddr[0] = info->m_loadAddr & 255;
        saveAddr[1] = info->m_loadAddr >> 8;
        fMyOut.write( (char*)saveAddr, 2 );  // !cast!

        // Data starts at: bufferaddr + fileoffset
        // Data length: datafilelen - fileoffset
        fMyOut.write( (const char*)dataBuffer + fileOffset, info->m_dataFileLen - fileOffset );  // !cast!
    }

    if ( !fMyOut )
        return false;
    else
        return true;
}
