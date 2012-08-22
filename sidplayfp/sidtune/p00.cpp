/*
 * C64 P00 file format support.
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

#include "p00.h"

#include "SidTuneCfg.h"
#include "SidTuneTools.h"
#include "SidTuneInfoImpl.h"

#define X00_ID_LEN   8
#define X00_NAME_LEN 17

// File format from PC64.  PC64 automatically generates
// the filename from the cbm name (16 to 8 conversion)
// but we only need to worry about that when writing files
// should we want pc64 compatibility.  The extension numbers
// are just an index to try to avoid repeats.  Name conversion
// works by creating an initial filename from alphanumeric
// and ' ', '-' characters only with the later two being
// converted to '_'.  Then it parses the filename
// from end to start removing characters stopping as soon
// as the filename becomes <= 8.  The removal of characters
// occurs in three passes, the first removes all '_', then
// vowels and finally numerics.  If the filename is still
// greater than 8 it is trucated. struct X00Header
struct X00Header
{
    char    id[X00_ID_LEN];     // C64File
    char    name[X00_NAME_LEN]; // C64 name
    uint8_t length;             // Rel files only (Bytes/Record),
                                // should be 0 for all other types
};

typedef enum
{
    X00_UNKNOWN,
    X00_DEL,
    X00_SEQ,
    X00_PRG,
    X00_USR,
    X00_REL
} X00Format;

const char TXT_FORMAT_DEL[] = "Unsupported tape image file (DEL)";
const char TXT_FORMAT_SEQ[] = "Unsupported tape image file (SEQ)";
const char TXT_FORMAT_PRG[] = "Tape image file (PRG)";
const char TXT_FORMAT_USR[] = "Unsupported USR file (USR)";
const char TXT_FORMAT_REL[] = "Unsupported tape image file (REL)";

const char ERR_TRUNCATED[]  = "ERROR: File is most likely truncated";

const char P00_ID[]         = "C64File";


SidTuneBase* p00::load(const char *fileName, Buffer_sidtt<const uint_least8_t>& dataBuf)
{
    const char      *ext     = SidTuneTools::fileExtOfPath(const_cast<char *>(fileName));
    const char      *format  = 0;
    const X00Header *pHeader = reinterpret_cast<const X00Header*>(dataBuf.get());
    const uint_least32_t bufLen  = dataBuf.len ();

    // Combined extension & magic field identification
    if (strlen (ext) != 4)
        return false;
    if (!isdigit(ext[2]) || !isdigit(ext[3]))
        return false;

    X00Format type = X00_UNKNOWN;
    switch (toupper(ext[1]))
    {
    case 'D':
        type   = X00_DEL;
        format = TXT_FORMAT_DEL;
        break;
    case 'S':
        type   = X00_SEQ;
        format = TXT_FORMAT_SEQ;
        break;
    case 'P':
        type   = X00_PRG;
        format = TXT_FORMAT_PRG;
        break;
    case 'U':
        type   = X00_USR;
        format = TXT_FORMAT_USR;
        break;
    case 'R':
        type   = X00_REL;
        format = TXT_FORMAT_REL;
        break;
    }

    if (type == X00_UNKNOWN)
        return 0;

    // Verify the file is what we think it is
    if (bufLen < X00_ID_LEN)
        return 0;

    if (strcmp (pHeader->id, P00_ID))
        return 0;

    // File types current supported
    if (type != X00_PRG)
        throw loadError("Not a PRG inside X00");

    if (bufLen < sizeof(X00Header)+2)
        throw loadError(ERR_TRUNCATED);

    p00 *tune = new p00();
    try
    {
        tune->load(format, pHeader);
    }
    catch (loadError& e)
    {
        delete tune;
        throw e;
    }

    return tune;
}

void p00::load(const char* format, const X00Header* pHeader)
{
    info->m_formatString = format;

    {   // Decode file name
        SmartPtr_sidtt<const uint8_t> spPet((const uint8_t*)pHeader->name, X00_NAME_LEN);
        convertPetsciiToAscii(spPet, infoString[0]);
    }

    // Automatic settings
    fileOffset            = sizeof(X00Header);
    info->m_songs         = 1;
    info->m_startSong     = 1;
    info->m_compatibility = SidTuneInfo::COMPATIBILITY_BASIC;
    info->m_numberOfInfoStrings = 1;
    info->m_infoString[0] = infoString[0];

    // Create the speed/clock setting table.
    convertOldStyleSpeedToTables(~0, info->m_clockSpeed);
}
