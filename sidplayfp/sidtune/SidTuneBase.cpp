/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "SidTuneCfg.h"
#include "SidTuneInfoImpl.h"
#include "SidTuneBase.h"
#include "SidTuneTools.h"
#include "sidplayfp/sidendian.h"

#include "MUS.h"
#include "p00.h"
#include "prg.h"
#include "PSID.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#include <iostream>
#include <iomanip>
#include <string.h>
#include <limits.h>

#ifdef HAVE_IOS_OPENMODE
    typedef std::ios::openmode openmode;
#else
    typedef int openmode;
#endif

// Error and status message strings.
const char ERR_SONG_NUMBER_EXCEED[]  = "SIDTUNE WARNING: Selected song number was too high";
const char ERR_EMPTY[]               = "SIDTUNE ERROR: No data to load";
const char ERR_UNRECOGNIZED_FORMAT[] = "SIDTUNE ERROR: Could not determine file format";
const char ERR_NO_DATA_FILE[]        = "SIDTUNE ERROR: Did not find the corresponding data file";
const char ERR_NOT_ENOUGH_MEMORY[]   = "SIDTUNE ERROR: Not enough free memory";
const char ERR_CANT_LOAD_FILE[]      = "SIDTUNE ERROR: Could not load input file";
const char ERR_CANT_OPEN_FILE[]      = "SIDTUNE ERROR: Could not open file for binary input";
const char ERR_FILE_TOO_LONG[]       = "SIDTUNE ERROR: Input data too long";
const char ERR_DATA_TOO_LONG[]       = "SIDTUNE ERROR: Size of music data exceeds C64 memory";
const char ERR_CANT_CREATE_FILE[]    = "SIDTUNE ERROR: Could not create output file";
const char ERR_FILE_IO_ERROR[]       = "SIDTUNE ERROR: File I/O error";
const char ERR_BAD_ADDR[]            = "SIDTUNE ERROR: Bad address data";
const char ERR_BAD_RELOC[]           = "SIDTUNE ERROR: Bad reloc data";
const char ERR_CORRUPT[]             = "SIDTUNE ERROR: File is incomplete or corrupt";

// Petscii to Ascii conversion table
static const char _sidtune_CHRtab[256] =  // CHR$ conversion table (0x01 = no output)
{
   0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0xd, 0x1, 0x1,
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
  0x20,0x21, 0x1,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
  0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
  0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x24,0x5d,0x20,0x20,
  // alternative: CHR$(92=0x5c) => ISO Latin-1(0xa3)
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  // 0x80-0xFF
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23,
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23
};

SidTuneBase* SidTuneBase::load(const char* fileName, const char **fileNameExt,
                 const bool separatorIsSlash)
{
    if (!fileName)
        return 0;

    //isSlashedFileName = separatorIsSlash;
    //setFileNameExtensions(fileNameExt);
#if !defined(SIDTUNE_NO_STDIN_LOADER)
    // Filename ``-'' is used as a synonym for standard input.
    if ( strcmp(fileName,"-")==0 )
        return getFromStdIn();
#endif
    return getFromFiles(fileName, fileNameExt);
}

SidTuneBase* SidTuneBase::read(const uint_least8_t* sourceBuffer, const uint_least32_t bufferLen)
{
    return getFromBuffer(sourceBuffer, bufferLen);
}

const SidTuneInfo* SidTuneBase::getInfo() const
{
    return info;
}

const SidTuneInfo* SidTuneBase::getInfo(const unsigned int songNum)
{
    selectSong(songNum);
    return info;
}

// First check, whether a song is valid. Then copy any song-specific
// variable information such a speed/clock setting to the info structure.
unsigned int SidTuneBase::selectSong(const unsigned int selectedSong)
{
    unsigned int song = selectedSong;
    // Determine and set starting song number.
    if (selectedSong == 0)
        song = info->m_startSong;
    if (selectedSong>info->m_songs || selectedSong>MAX_SONGS)
    {
        song = info->m_startSong;
        //m_statusString = ERR_SONG_NUMBER_EXCEED; FIXME
    }
    info->m_currentSong = song;
    // Retrieve song speed definition.
    if (info->m_compatibility == SidTuneInfo::COMPATIBILITY_R64)
        info->m_songSpeed = SidTuneInfo::SPEED_CIA_1A;
    else if (info->m_compatibility == SidTuneInfo::COMPATIBILITY_PSID)
    {   // This does not take into account the PlaySID bug upon evaluating the
        // SPEED field. It would most likely break compatibility to lots of
        // sidtunes, which have been converted from .SID format and vice versa.
        // The .SID format does the bit-wise/song-wise evaluation of the SPEED
        // value correctly, like it is described in the PlaySID documentation.
        info->m_songSpeed = songSpeed[(song-1)&31];
    }
    else
        info->m_songSpeed = songSpeed[song-1];
    info->m_clockSpeed = clockSpeed[song-1];

    return info->m_currentSong;
}

// ------------------------------------------------- private member functions

bool SidTuneBase::placeSidTuneInC64mem(uint_least8_t* c64buf)
{
    if (c64buf != 0)
    {
        // The Basic ROM sets these values on loading a file.
        // Program end address
        const uint_least16_t start = info->m_loadAddr;
        const uint_least16_t end   = start + info->m_c64dataLen;
        endian_little16(c64buf + 0x2d, end); // Variables start
        endian_little16(c64buf + 0x2f, end); // Arrays start
        endian_little16(c64buf + 0x31, end); // Strings start
        endian_little16(c64buf + 0xac, start);
        endian_little16(c64buf + 0xae, end);

        // Copy data from cache to the correct destination.
        memcpy(c64buf+info->m_loadAddr, cache.get()+fileOffset, info->m_c64dataLen);

        return true;
    }
    return false;
}

void SidTuneBase::loadFile(const char* fileName, Buffer_sidtt<const uint_least8_t>& bufferRef)
{
    // This sucks big time
    openmode createAttr = std::ios::in;
#ifdef HAVE_IOS_NOCREATE
    createAttr |= std::ios::nocreate;
#endif
    // Open binary input file stream at end of file.
#if defined(HAVE_IOS_BIN)
    createAttr |= std::ios::bin;
#else
    createAttr |= std::ios::binary;
#endif

    std::fstream myIn(fileName, createAttr);

    if ( !myIn.is_open() )
    {
        throw loadError(ERR_CANT_OPEN_FILE);
    }

    myIn.seekg(0, std::ios::end);
    const uint_least32_t fileLen = (uint_least32_t)myIn.tellg();

    if ( fileLen == 0 )
    {
         throw loadError(ERR_EMPTY);
    }

    Buffer_sidtt<const uint_least8_t> fileBuf;

    if ( !fileBuf.assign(new uint_least8_t[fileLen], fileLen) ) //FIXME catch bad_alloc exceptions?
    {
        throw loadError(ERR_NOT_ENOUGH_MEMORY);
    }

    myIn.seekg(0, std::ios::beg);

    myIn.read((char*)fileBuf.get(), fileLen);

    if ( myIn.bad() )
    {
        throw loadError(ERR_CANT_LOAD_FILE);
    }

    myIn.close();

    bufferRef.assign(fileBuf.xferPtr(), fileBuf.xferLen());
}

void SidTuneBase::deleteFileNameCopies()
{
    // When will it be fully safe to call delete[](0) on every system?
    if ( info->m_dataFileName != 0 )
        delete[] info->m_dataFileName;
    if ( info->m_infoFileName != 0 )
        delete[] info->m_infoFileName;
    if ( info->m_path != 0 )
        delete[] info->m_path;
    info->m_dataFileName = 0;
    info->m_infoFileName = 0;
    info->m_path = 0;
}

SidTuneBase::SidTuneBase()
{
    // Initialize the object with some safe defaults.
    info = new SidTuneInfoImpl();

    for ( unsigned int si = 0; si < MAX_SONGS; si++ )
    {
        songSpeed[si] = info->m_songSpeed;
        clockSpeed[si] = info->m_clockSpeed;
    }

    fileOffset = 0;

    for ( unsigned int sNum = 0; sNum < SidTuneInfo::MAX_CREDIT_STRINGS; sNum++ )
    {
        for ( unsigned int sPos = 0; sPos < SidTuneInfo::MAX_CREDIT_STRLEN; sPos++ )
        {
            infoString[sNum][sPos] = 0;
        }
    }
    info->m_numberOfInfoStrings = 0;

    // Not used!!! TODO remove
    info->m_numberOfCommentStrings = 1;
#ifdef HAVE_EXCEPTIONS
    info->m_commentString = new(std::nothrow) char* [info->m_numberOfCommentStrings];
#else
    info->m_commentString = new char* [info->m_numberOfCommentStrings];
#endif
    if (info->m_commentString != 0)
        info->m_commentString[0] = SidTuneTools::myStrDup("--- SAVED WITH SIDPLAY ---");
}

SidTuneBase::~SidTuneBase()
{
    // Remove copy of comment field.
    unsigned int strNum = 0;
    // Check and remove every available line.
    while (info->m_numberOfCommentStrings-- > 0)
    {
        if (info->m_commentString[strNum] != 0)
        {
            delete[] info->m_commentString[strNum];
            info->m_commentString[strNum] = 0;
        }
        strNum++;  // next string
    };
    delete[] info->m_commentString;  // free the array pointer

    deleteFileNameCopies();

    delete info;
}

#if !defined(SIDTUNE_NO_STDIN_LOADER)

SidTuneBase* SidTuneBase::getFromStdIn()
{
    uint_least8_t* fileBuf = new uint_least8_t[MAX_FILELEN]; //FIXME catch bad_alloc exceptions?

    // We only read as much as fits in the buffer.
    // This way we avoid choking on huge data.
    uint_least32_t i = 0;
    char datb;
    while (std::cin.get(datb) && i<MAX_FILELEN)
        fileBuf[i++] = (uint_least8_t) datb;
    //info->m_dataFileLen = i; //FIXME
    getFromBuffer(fileBuf, i);
    delete[] fileBuf;
}

#endif

SidTuneBase* SidTuneBase::getFromBuffer(const uint_least8_t* const buffer, const uint_least32_t bufferLen)
{
    if (buffer==0 || bufferLen==0)
    {
        throw loadError(ERR_EMPTY);
    }

    if (bufferLen > MAX_FILELEN)
    {
        throw loadError(ERR_FILE_TOO_LONG);
    }

    uint_least8_t* tmpBuf = new uint_least8_t[bufferLen]; //FIXME catch bad_alloc exceptions?
    /*if ( tmpBuf == 0 )
    {
        throw loadError(ERR_NOT_ENOUGH_MEMORY);
    }*/
    memcpy(tmpBuf,buffer,bufferLen);

    Buffer_sidtt<const uint_least8_t> buf1(tmpBuf, bufferLen);

    bool foundFormat = false;
    // Here test for the possible single file formats. --------------
    SidTuneBase* s = PSID::load(buf1);
    if (!s)
    {
        Buffer_sidtt<const uint_least8_t> buf2;  // empty
        s = MUS::load(buf1, buf2, 0, true);
    }

    if (s)
    {
        if (s->acceptSidTune("-","-",buf1)) //FIXME catch loadError
            return s;
        delete s;
        //throw loadError(m_statusString);
    }

    throw loadError(ERR_UNRECOGNIZED_FORMAT);
}

bool SidTuneBase::acceptSidTune(const char* dataFileName, const char* infoFileName,
                            Buffer_sidtt<const uint_least8_t>& buf)
{
    // @FIXME@ - MUS
    if ( info->m_numberOfInfoStrings == 3 )
    {   // Add <?> (HVSC standard) to missing title, author, release fields
        for (int i = 0; i < 3; i++)
        {
            if (infoString[i][0] == '\0')
            {
                strcpy (&infoString[i][0], "<?>");
                info->m_infoString[i] = &infoString[i][0];
            }
        }
    }

    deleteFileNameCopies();
    // Make a copy of the data file name and path, if available.
    if ( dataFileName != 0 )
    {
        info->m_path = SidTuneTools::myStrDup(dataFileName);
        if (isSlashedFileName)
        {
            info->m_dataFileName = SidTuneTools::myStrDup(SidTuneTools::slashedFileNameWithoutPath(info->m_path));
            *SidTuneTools::slashedFileNameWithoutPath(info->m_path) = 0;  // path only
        }
        else
        {
            info->m_dataFileName = SidTuneTools::myStrDup(SidTuneTools::fileNameWithoutPath(info->m_path));
            *SidTuneTools::fileNameWithoutPath(info->m_path) = 0;  // path only
        }
        if ((info->m_path==0) || (info->m_dataFileName==0))
        {
            throw loadError(ERR_NOT_ENOUGH_MEMORY);
        }
    }
    else
    {
        // Provide empty strings.
        info->m_path = SidTuneTools::myStrDup("");
        info->m_dataFileName = SidTuneTools::myStrDup("");
    }
    // Make a copy of the info file name, if available.
    if ( infoFileName != 0 )
    {
        char* tmp = SidTuneTools::myStrDup(infoFileName);
        if (isSlashedFileName)
            info->m_infoFileName = SidTuneTools::myStrDup(SidTuneTools::slashedFileNameWithoutPath(tmp));
        else
            info->m_infoFileName = SidTuneTools::myStrDup(SidTuneTools::fileNameWithoutPath(tmp));
        if ((tmp==0) || (info->m_infoFileName==0))
        {
            throw loadError(ERR_NOT_ENOUGH_MEMORY);
        }
        delete[] tmp;
    }
    else
    {
        // Provide empty string.
        info->m_infoFileName = SidTuneTools::myStrDup("");
    }
    // Fix bad sidtune set up.
    if (info->m_songs > MAX_SONGS)
        info->m_songs = MAX_SONGS;
    else if (info->m_songs == 0)
        info->m_songs++;
    if (info->m_startSong > info->m_songs)
        info->m_startSong = 1;
    else if (info->m_startSong == 0)
        info->m_startSong++;

    info->m_dataFileLen = buf.len();
    info->m_c64dataLen = buf.len() - fileOffset;

    // Calculate any remaining addresses and then
    // confirm all the file details are correct
    if ( resolveAddrs(buf.get() + fileOffset) == false )
        return false;
    if ( checkRelocInfo() == false )
        return false;
    if ( checkCompatibility() == false )
        return false;

    if (info->m_dataFileLen >= 2)
    {
        // We only detect an offset of two. Some position independent
        // sidtunes contain a load address of 0xE000, but are loaded
        // to 0x0FFE and call player at 0x1000.
        info->m_fixLoad = (endian_little16(buf.get()+fileOffset)==(info->m_loadAddr+2));
    }

    // Check the size of the data.

    if ( info->m_c64dataLen > MAX_MEMORY )
    {
        throw loadError(ERR_DATA_TOO_LONG);
    }
    else if ( info->m_c64dataLen == 0 )
    {
        throw loadError(ERR_EMPTY);
    }

    cache.assign(buf.xferPtr(),buf.xferLen());

    return true;
}

bool SidTuneBase::createNewFileName(Buffer_sidtt<char>& destString,
                                const char* sourceName,
                                const char* sourceExt)
{
    Buffer_sidtt<char> newBuf;
    uint_least32_t newLen = strlen(sourceName)+strlen(sourceExt)+1;
    // Get enough memory, so we can appended the extension.

    try
    {
        newBuf.assign(new char[newLen], newLen);
    }
    catch (std::bad_alloc &e)
    {
        //m_statusString = ERR_NOT_ENOUGH_MEMORY;
        return false;
    }
    strcpy(newBuf.get(),sourceName);
    strcpy(SidTuneTools::fileExtOfPath(newBuf.get()),sourceExt);
    destString.assign(newBuf.xferPtr(),newBuf.xferLen());
    return true;
}

// Initializing the object based upon what we find in the specified file.

SidTuneBase* SidTuneBase::getFromFiles(const char* fileName, const char **fileNameExtensions)
{
    Buffer_sidtt<const uint_least8_t> fileBuf1;

    loadFile(fileName, fileBuf1);

    // File loaded. Now check if it is in a valid single-file-format.
    SidTuneBase* s = PSID::load(fileBuf1);
    if (!s)
    {
        Buffer_sidtt<const uint_least8_t> fileBuf2;

        // Try some native C64 file formats
        s = MUS::load(fileBuf1, fileBuf2, 0, true);
        if (s)
        {
            // Try to find second file.
            Buffer_sidtt<char> fileName2;
            int n = 0;
            while (fileNameExtensions[n] != 0)
            {
                if ( !createNewFileName(fileName2, fileName, fileNameExtensions[n]) )
                    return 0;
                // 1st data file was loaded into ``fileBuf1'',
                // so we load the 2nd one into ``fileBuf2''.
                // Do not load the first file again if names are equal.
                if (MYSTRICMP(fileName, fileName2.get()) != 0)
                {
                    try
                    {
                        loadFile(fileName2.get(), fileBuf2);
                    // Check if tunes in wrong order and therefore swap them here
                    if (MYSTRICMP (fileNameExtensions[n], ".mus") == 0)
                    {
                        SidTuneBase* s2 = MUS::load(fileBuf2, fileBuf1, 0, true);
                        if (s2)
                        {
                            if (s2->acceptSidTune(fileName2.get(), fileName, fileBuf2)) //FIXME catch loadError
                            {
                                delete s;
                                return s2;
                            }
                            delete s2;
                        }
                    }
                    else
                    {
                        SidTuneBase* s2 = MUS::load(fileBuf1, fileBuf2, 0, true);
                        if (s2)
                        {
                            if (s2->acceptSidTune(fileName, fileName2.get(), fileBuf1)) //FIXME catch loadError
                            {
                                delete s;
                                return s2;
                            }
                            delete s2;
                        }
                    }
                    // The first tune loaded ok, so ignore errors on the
                    // second tune, may find an ok one later
                    }
                    catch (loadError& e) {}
                }
                n++;
            }
            // No (suitable) second file, so reload first without second
            /*fileBuf2.erase();
            s = MUS::load(fileBuf1, fileBuf2, true);*/

            if (s->acceptSidTune(fileName, 0, fileBuf1)) //FIXME catch loadError
                return s;
            delete s;
            return 0;
        }
    }
    if (!s) s = p00::load(fileName, fileBuf1);
    if (!s) s = prg::load(fileName, fileBuf1);

    if (s)
    {
        if (s->acceptSidTune(fileName, 0, fileBuf1)) //FIXME catch loadError
            return s;
        delete s;
        //throw loadError(m_statusString);
    }

    throw loadError(ERR_UNRECOGNIZED_FORMAT);
}

void SidTuneBase::convertOldStyleSpeedToTables(uint_least32_t speed, SidTuneInfo::clock_t clock)
{
    // Create the speed/clock setting tables.
    //
    // This routine implements the PSIDv2NG compliant speed conversion.  All tunes
    // above 32 use the same song speed as tune 32
    const unsigned int toDo = ((info->m_songs <= MAX_SONGS) ? info->m_songs : MAX_SONGS);
    for (unsigned int s = 0; s < toDo; s++)
    {
        clockSpeed[s] = clock;
        if (speed & 1)
            songSpeed[s] = SidTuneInfo::SPEED_CIA_1A;
        else
            songSpeed[s] = SidTuneInfo::SPEED_VBI;
        if (s < 31)
            speed >>= 1;
    }
}

bool SidTuneBase::checkRelocInfo (void)
{
    // Fix relocation information
    if (info->m_relocStartPage == 0xFF)
    {
        info->m_relocPages = 0;
        return true;
    }
    else if (info->m_relocPages == 0)
    {
        info->m_relocStartPage = 0;
        return true;
    }

    // Calculate start/end page
    const uint_least8_t startp = info->m_relocStartPage;
    const uint_least8_t endp   = (startp + info->m_relocPages - 1) & 0xff;
    if (endp < startp)
    {
        //m_statusString = ERR_BAD_RELOC; FIXME
        return false;
    }

    {    // Check against load range
        const uint_least8_t startlp = (uint_least8_t) (info->m_loadAddr >> 8);
        const uint_least8_t endlp   = startlp + (uint_least8_t) ((info->m_c64dataLen - 1) >> 8);

        if ( ((startp <= startlp) && (endp >= startlp)) ||
             ((startp <= endlp)   && (endp >= endlp)) )
        {
            //m_statusString = ERR_BAD_RELOC; FIXME
            return false;
        }
    }

    // Check that the relocation information does not use the following
    // memory areas: 0x0000-0x03FF, 0xA000-0xBFFF and 0xD000-0xFFFF
    if ((startp < 0x04)
        || ((0xa0 <= startp) && (startp <= 0xbf))
        || (startp >= 0xd0)
        || ((0xa0 <= endp) && (endp <= 0xbf))
        || (endp >= 0xd0))
    {
        //m_statusString = ERR_BAD_RELOC; FIXME
        return false;
    }
    return true;
}

bool SidTuneBase::resolveAddrs (const uint_least8_t *c64data)
{   // Originally used as a first attempt at an RSID
    // style format. Now reserved for future use
    if ( info->m_playAddr == 0xffff )
        info->m_playAddr  = 0;

    // loadAddr = 0 means, the address is stored in front of the C64 data.
    if ( info->m_loadAddr == 0 )
    {
        if ( info->m_c64dataLen < 2 )
        {
            //m_statusString = ERR_CORRUPT; FIXME
            return false;
        }
        info->m_loadAddr = endian_16( *(c64data+1), *c64data );
        fileOffset += 2;
        c64data += 2;
        info->m_c64dataLen -= 2;
    }

    if ( info->m_compatibility == SidTuneInfo::COMPATIBILITY_BASIC )
    {
        if ( info->m_initAddr != 0 )
        {
            //m_statusString = ERR_BAD_ADDR; FIXME
            return false;
        }
    }
    else if ( info->m_initAddr == 0 )
        info->m_initAddr = info->m_loadAddr;
    return true;
}

bool SidTuneBase::checkCompatibility (void)
{
    switch ( info->m_compatibility )
    {
    case SidTuneInfo::COMPATIBILITY_R64:
        // Check valid init address
        switch (info->m_initAddr >> 12)
        {
        case 0x0F:
        case 0x0E:
        case 0x0D:
        case 0x0B:
        case 0x0A:
            //m_statusString = ERR_BAD_ADDR; FIXME
            return false;
        default:
            if ( (info->m_initAddr < info->m_loadAddr) ||
                 (info->m_initAddr > (info->m_loadAddr + info->m_c64dataLen - 1)) )
            {
                //m_statusString = ERR_BAD_ADDR; FIXME
                return false;
            }
        }
        // deliberate run on

    case SidTuneInfo::COMPATIBILITY_BASIC:
        // Check tune is loadable on a real C64
        if ( info->m_loadAddr < SIDTUNE_R64_MIN_LOAD_ADDR )
        {
            //m_statusString = ERR_BAD_ADDR; FIXME
            return false;
        }
        break;
    }
    return true;
}

int SidTuneBase::convertPetsciiToAscii(SmartPtr_sidtt<const uint8_t>& spPet, char* dest)
{
    int count = 0;
    char c;
    if (dest)
    {
        do
        {
            c = _sidtune_CHRtab[*spPet];  // ASCII CHR$ conversion
            if ((c>=0x20) && (count<=31))
                dest[count++] = c;  // copy to info string

            // If character is 0x9d (left arrow key) then move back.
            if ((*spPet==0x9d) && (count>0))
                count--;
            spPet++;
        }
        while ( !((c==0x0D)||(c==0x00)||spPet.fail()) );
    }
    else
    {   // Just find end of string
        do
        {
            c = _sidtune_CHRtab[*spPet];  // ASCII CHR$ conversion
            spPet++;
        }
        while ( !((c==0x0D)||(c==0x00)||spPet.fail()) );
    }
    return count;
}
