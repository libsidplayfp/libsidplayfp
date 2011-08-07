/*
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

#ifndef SIDTUNEMOD_H
#define SIDTUNEMOD_H

#include "sidplayfp/SidTune.h"

#define SIDTUNE_MD5_LENGTH 32

/**
* SidTuneMod
* A wrapper around SidTune class which provides the ability
* to calculate the tune's md5 hash to match against songlenth DB.
*/
class SID_EXTERN SidTuneMod : public SidTune
{
 private:
    char m_md5[SIDTUNE_MD5_LENGTH+1];

 public:  // --------------------------------------------------------- public

    SidTuneMod(const char* fileName) : SidTune(fileName)
    { ; }

    /**
    * Calculates the MD5 hash of the tune.
    * Not providing an md5 buffer will cause the internal one to be used.
    * If provided, buffer must be SIDTUNE_MD5_LENGTH + 1
    * @return a pointer to the buffer containing the md5 string.
    */
    const char *createMD5(char *md5 = 0);
};

#endif  /* SIDTUNEMOD_H */
