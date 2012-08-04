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

#ifndef SIDTUNEBASE_H
#define SIDTUNEBASE_H

#include "sidplayfp/Buffer.h"
#include "sidplayfp/SmartPtr.h"

#include "sidplayfp/SidTuneInfo.h"
#include "sidplayfp/SidTune.h"

#include <stdint.h>
#include <fstream>

template class Buffer_sidtt<const uint_least8_t>;

class SidTuneInfoImpl;

/** @internal
* SidTuneBaseBase
*/
class SidTuneBase
{
 private:
    /// Also PSID file format limit.
    static const unsigned int MAX_SONGS = 256;

    /// C64KB+LOAD+PSID
    static const uint_least32_t MAX_FILELEN = 65536+2+0x7C;

    static const uint_least32_t MAX_MEMORY = 65536;

 private:
    char m_md5[SidTune::MD5_LENGTH+1];

 public:  // ----------------------------------------------------------------

    /**
    * Load a sidtune from a file.
    *
    * To retrieve data from standard input pass in filename "-".
    * If you want to override the default filename extensions use this
    * contructor. Please note, that if the specified ``sidTuneFileName''
    * does exist and the loader is able to determine its file format,
    * this function does not try to append any file name extension.
    * See ``sidtune.cpp'' for the default list of file name extensions.
    * You can specific ``sidTuneFileName = 0'', if you do not want to
    * load a sidtune. You can later load one with open().
    */
    SidTuneBase(const char* fileName, const char **fileNameExt = 0,
            const bool separatorIsSlash = false);

    /**
    * Load a single-file sidtune from a memory buffer.
    * Currently supported: PSID format
    */
    SidTuneBase(const uint_least8_t* oneFileFormatSidtune, const uint_least32_t sidtuneLength);

    virtual ~SidTuneBase();

    /**
    * The sidTune class does not copy the list of file name extensions,
    * so make sure you keep it. If the provided pointer is 0, the
    * default list will be activated. This is a static list which
    *
    * is used by all SidTuneBase objects.
    */
    void setFileNameExtensions(const char **fileNameExt);

    /**
    * Load a sidtune into an existing object.
    * From a file.
    */
    bool load(const char* fileName, const bool separatorIsSlash = false);

    /**
    * From a buffer.
    */
    bool read(const uint_least8_t* sourceBuffer, const uint_least32_t bufferLen);

    /**
    * Select sub-song (0 = default starting song)
    * and return active song number out of [1,2,..,SIDTUNE_MAX_SONGS].
    */
    unsigned int selectSong(const unsigned int songNum);

    /**
    * Retrieve sub-song specific information.
    */
    const SidTuneInfo* getInfo() const;

    /**
    * Select sub-song (0 = default starting song)
    * and retrieve active song information.
    */
    const SidTuneInfo* getInfo(const unsigned int songNum);

    /**
    * Determine current state of object (true = okay, false = error).
    * Upon error condition use ``getInfo'' to get a descriptive
    * text string in ``SidTuneInfo.statusString''.
    */
    bool getStatus() const { return status; }

    /**
    * Error/status message of last operation
    */
    const char* statusString() const  { return m_statusString; }

    /**
    * Copy sidtune into C64 memory (64 KB).
    */
    bool placeSidTuneInC64mem(uint_least8_t* c64buf);

    // --- file save & format conversion ---

    /**
    * These functions work for any successfully created object.
    * overWriteFlag: true  = Overwrite existing file.
    *                  false = Default, return error when file already
    *                          exists.
    * One could imagine an "Are you sure ?"-checkbox before overwriting
    * any file.
    * returns: true = Successful, false = Error condition.
    */
    bool saveC64dataFile( const char* destFileName, const bool overWriteFlag = false );
    bool savePSIDfile( const char* destFileName, const bool overWriteFlag = false );

    /**
    * Does not affect status of object, and therefore can be used
    * to load files. Error string is put into info.statusString, though.
    */
    bool loadFile(const char* fileName, Buffer_sidtt<const uint_least8_t>& bufferRef);

    bool saveToOpenFile( std::ofstream& toFile, const uint_least8_t* buffer, uint_least32_t bufLen );

    /**
    * Calculates the MD5 hash of the tune.
    * Not providing an md5 buffer will cause the internal one to be used.
    * If provided, buffer must be MD5_LENGTH + 1
    * @return a pointer to the buffer containing the md5 string.
    */
    const char *createMD5(char *md5 = 0);

 protected:  // -------------------------------------------------------------

    SidTuneInfoImpl *info;

    bool status;

    const char* m_statusString;

    uint_least8_t songSpeed[MAX_SONGS];
    SidTuneInfo::clock_t clockSpeed[MAX_SONGS];

    /// holds text info from the format headers etc.
    char infoString[SidTuneInfo::MAX_CREDIT_STRINGS][SidTuneInfo::MAX_CREDIT_STRLEN];

    /**
    * If your opendir() and readdir()->d_name return path names
    * that contain the forward slash (/) as file separator, but
    * your operating system uses a different character, there are
    * extra functions that can deal with this special case. Set
    * separatorIsSlash to true if you like path names to be split
    * correctly.
    * You do not need these extra functions if your systems file
    * separator is the forward slash.
    */
    bool isSlashedFileName;

    /// For files with header: offset to real data
    uint_least32_t fileOffset;

    /// Needed for MUS/STR player installation.
    uint_least16_t musDataLen;

    Buffer_sidtt<const uint_least8_t> cache;

    /// Filename extensions to append for various file types.
    static const char** fileNameExtensions;

    // --- protected member functions ---

    /// Convert 32-bit PSID-style speed word to internal tables.
    void convertOldStyleSpeedToTables(uint_least32_t speed,
         SidTuneInfo::clock_t clock = SidTuneInfo::CLOCK_PAL);

    static int convertPetsciiToAscii (SmartPtr_sidtt<const uint_least8_t>&, char*);

    /// Check compatibility details are sensible
    bool checkCompatibility(void);
    /// Check for valid relocation information
    bool checkRelocInfo(void);
    /// Common address resolution procedure
    bool resolveAddrs(const uint_least8_t* c64data);

    // Support for various file formats.

    class loadError {
    private:
        const char* m_msg;
    public:
        loadError(const char* msg) : m_msg(msg) {}
        const char* message() const {return m_msg; }
    };

    virtual bool PSID_fileSupport    (Buffer_sidtt<const uint_least8_t>& dataBuf);
    virtual bool PSID_fileSupportSave(std::ofstream& toFile, const uint_least8_t* dataBuffer);

    virtual bool MUS_fileSupport     (Buffer_sidtt<const uint_least8_t>& musBuf,
                                            Buffer_sidtt<const uint_least8_t>& strBuf);
    bool         MUS_load            (Buffer_sidtt<const uint_least8_t>& musBuf,
                                            bool init = false);
    bool         MUS_load            (Buffer_sidtt<const uint_least8_t>& musBuf,
                                            Buffer_sidtt<const uint_least8_t>& strBuf,
                                            bool init = false);
    virtual bool MUS_detect          (const void* buffer, const uint_least32_t bufLen,
                                            uint_least32_t& voice3Index);
    virtual bool MUS_mergeParts      (Buffer_sidtt<const uint_least8_t>& musBuf,
                                      Buffer_sidtt<const uint_least8_t>& strBuf);
    virtual void MUS_setPlayerAddress();
    virtual void MUS_installPlayer   (uint_least8_t *c64buf);

    virtual bool PRG_fileSupport     (const char* fileName,
                                            Buffer_sidtt<const uint_least8_t>& dataBuf);
    virtual bool X00_fileSupport     (const char* fileName,
                                            Buffer_sidtt<const uint_least8_t>& dataBuf);

 private:  // ---------------------------------------------------------------

    void init();
    void cleanup();
#if !defined(SIDTUNE_NO_STDIN_LOADER)
    void getFromStdIn();
#endif
    void getFromFiles(const char* name);

    void deleteFileNameCopies();

    /// Try to retrieve single-file sidtune from specified buffer.
    void getFromBuffer(const uint_least8_t* const buffer, const uint_least32_t bufferLen);

    /**
    * Cache the data of a single-file or two-file sidtune and its
    * corresponding file names.
    */
    bool acceptSidTune(const char* dataFileName, const char* infoFileName,
                       Buffer_sidtt<const uint_least8_t>& buf);

    bool createNewFileName(Buffer_sidtt<char>& destString,
                           const char* sourceName, const char* sourceExt);

 private:    // prevent copying
    SidTuneBase(const SidTuneBase&);
    SidTuneBase& operator=(SidTuneBase&);
};

#endif  /* SIDTUNEBASE_H */
