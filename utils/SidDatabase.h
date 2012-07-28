/***************************************************************************
                          SidDatabase.h  -  songlength database support
                             -------------------
    begin                : Sun Mar 11 2001
    copyright            : (C) 2001 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _siddatabase_h_
#define _siddatabase_h_

#include <stdint.h>

#include "sidplayfp/sidconfig.h"

class SidTune;
class iniParser;

/**
* SidDatabase
* An utility class to deal with the songlength DataBase.
*/
class SID_EXTERN SidDatabase
{
private:
    iniParser  *m_parser;
    const char *errorString;

    class parseError {};

    static const char* parseTime(const char* str, long &result);

public:
    SidDatabase  ();
    ~SidDatabase ();

    /**
    * Open the songlength DataBase.
    *
    * @param filename songlengthDB file name with full path.
    * @return -1 in case of errors, 0 otherwise.
    */
    int           open   (const char *filename);

    /**
    * Close the songlength DataBase.
    */
    void          close  ();

    /**
    * Get the length of the current subtune.
    *
    * @param tune
    * @return tune length in seconds, -1 in case of errors.
    */
    int_least32_t length (SidTune &tune);

    /**
    * Get the length of the selected subtune.
    *
    * @param md5 the md5 hash of the tune.
    * @param song the subtune.
    * @return tune length in seconds, -1 in case of errors.
    */
    int_least32_t length (const char *md5, uint_least16_t song);

    /// Get descriptive error message.
    const char *  error  (void) { return errorString; }
};

#endif // _siddatabase_h_
