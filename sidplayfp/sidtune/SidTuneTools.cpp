/*
 * /home/ms/files/source/libsidtune/RCS/SidTuneTools.cpp,v
 *
 * Copyright (C) Michael Schwendt <mschwendt@yahoo.com>
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

#include "SidTuneTools.h"

#include <string.h>

// Return pointer to file name position in complete path.
size_t SidTuneTools::fileNameWithoutPath(const char* s)
{
    size_t last_slash_pos = -1;
    for ( size_t pos = 0; pos < strlen(s); pos++ )
    {
#if defined(SID_FS_IS_COLON_AND_BACKSLASH_AND_SLASH)
        if ( s[pos] == ':' || s[pos] == '\\' ||
             s[pos] == '/' )
#elif defined(SID_FS_IS_COLON_AND_SLASH)
        if ( s[pos] == ':' || s[pos] == '/' )
#elif defined(SID_FS_IS_SLASH)
        if ( s[pos] == '/' )
#elif defined(SID_FS_IS_BACKSLASH)
        if ( s[pos] == '\\' )
#elif defined(SID_FS_IS_COLON)
        if ( s[pos] == ':' )
#else
#error Missing file/path separator definition.
#endif
        {
            last_slash_pos = pos;
        }
    }
    return last_slash_pos + 1;
}

// Return pointer to file name position in complete path.
// Special version: file separator = forward slash.
size_t SidTuneTools::slashedFileNameWithoutPath(const char* s)
{
    size_t last_slash_pos = -1;
    for ( size_t pos = 0; pos < strlen(s); pos++ )
    {
        if ( s[pos] == '/' )
        {
            last_slash_pos = pos;
        }
    }
    return last_slash_pos + 1;
}

// Return pointer to file name extension in path.
// The backwards-version.
char* SidTuneTools::fileExtOfPath(char* s)
{
    uint_least32_t last_dot_pos = strlen(s);  // assume no dot and append
    for ( int pos = last_dot_pos; pos >= 0; --pos )
    {
        if ( s[pos] == '.' )
        {
            last_dot_pos = pos;
            break;
        }
    }
    return( &s[last_dot_pos] );
}
