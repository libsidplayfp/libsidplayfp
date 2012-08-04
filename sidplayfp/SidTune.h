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

#ifndef SIDTUNE_H
#define SIDTUNE_H

#include "sidconfig.h"
#include "SidTuneInfo.h"

#include <stdint.h>
#include <fstream>

class SidTuneInfoImpl;
class SidTuneBase;

/**
* SidTune
*/
class SID_EXTERN SidTune
{
 public:
    static const int MD5_LENGTH = 32;

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
    SidTune(const char* fileName, const char **fileNameExt = 0,
            const bool separatorIsSlash = false);

    /**
    * Load a single-file sidtune from a memory buffer.
    * Currently supported: PSID format
    */
    SidTune(const uint_least8_t* oneFileFormatSidtune, const uint_least32_t sidtuneLength);

    virtual ~SidTune();

    /**
    * The sidTune class does not copy the list of file name extensions,
    * so make sure you keep it. If the provided pointer is 0, the
    * default list will be activated. This is a static list which
    *
    * is used by all SidTune objects.
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
    bool getStatus() const;

    /**
    * Error/status message of last operation
    */
    const char* statusString() const;

    /**
    * Copy sidtune into C64 memory (64 KB).
    */
    bool placeSidTuneInC64mem(uint_least8_t* c64buf);

    // --- file save & format conversion ---
#if 0
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
#endif
    /**
    * Calculates the MD5 hash of the tune.
    * Not providing an md5 buffer will cause the internal one to be used.
    * If provided, buffer must be MD5_LENGTH + 1
    * @return a pointer to the buffer containing the md5 string.
    */
    const char *createMD5(char *md5 = 0);

 protected:  // -------------------------------------------------------------
    SidTuneBase *tune;

 private:    // prevent copying
    SidTune(const SidTune&);
    SidTune& operator=(SidTune&);
};

#endif  /* SIDTUNE_H */
