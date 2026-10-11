/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2026 LouD
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "PSID.h"

#include <cstring>
#include <string>
#include <memory>
#include <vector>

#include "sidplayfp/SidTuneInfo.h"

#include "sidendian.h"
#include "sidmd5.h"

namespace libsidplayfp
{

constexpr int PSID_MAXSTRLEN = 32;

// Version 5 name, author and released share one area
constexpr int PSID_V5_METADATALEN = 3 * PSID_MAXSTRLEN;


// Header has been extended for 'RSID' format
// The following changes are present:
//     id = 'RSID'
//     version = 2, 3, 4 or 5
//     play, load and speed reserved 0
//     psidspecific flag is called C64BASIC flag
//     init cannot be under ROMS/IO memory area
//     load address cannot be less than $07E8
//     info strings may be 32 characters long without trailing zero

struct psidHeader           // all values are big-endian
{
    uint32_t id;                   // 'PSID' or 'RSID' (ASCII)
    uint16_t version;              // 1, 2, 3, 4 or 5
    uint16_t data;                 // 16-bit offset to binary data in file
    uint16_t load;                 // 16-bit C64 address to load file to
    uint16_t init;                 // 16-bit C64 address of init subroutine
    uint16_t play;                 // 16-bit C64 address of play subroutine
    uint16_t songs;                // number of songs
    uint16_t start;                // start song out of [1..256]
    uint32_t speed;                // 32-bit speed info
                                   // bit: 0=50 Hz, 1=CIA 1 Timer A (default: 60 Hz)
    char name[PSID_MAXSTRLEN];     // ASCII strings, 31 characters long and
    char author[PSID_MAXSTRLEN];   // terminated by a trailing zero
    char released[PSID_MAXSTRLEN]; //

    uint16_t flags;                // only version >= 2
    uint8_t relocStartPage;        // only version >= 2ng
    uint8_t relocPages;            // only version >= 2ng
    uint8_t sidChipBase2;          // only version >= 3
    uint8_t sidChipBase3;          // only version >= 4
                                   // version 5: multiSidConfig and sidAddressConfigStart

    uint8_t metadata[PSID_V5_METADATALEN]; // version 5: name, author and released
                                           // as one area, same bytes as the fields above
};

enum
{
    PSID_MUS       = 1 << 0,
    PSID_SPECIFIC  = 1 << 1, // These two are mutally exclusive
    PSID_BASIC     = 1 << 1,
    PSID_CLOCK     = 3 << 2,
    PSID_SIDMODEL  = 3 << 4,
    PSID_PANLAYOUT = 3 << 6, // version 5 only
    PSID_PANMODE   = 3 << 8, // version 5 only
    PSID_SONGLENGTHS = 1 << 10, // version 5 only
    PSID_FMOPL       = 1 << 11  // version 5 only
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
    PSID_SIDMODEL_UNKNOWN = 0,
    PSID_SIDMODEL_6581    = 1,
    PSID_SIDMODEL_8580    = 2,
    PSID_SIDMODEL_ANY     = PSID_SIDMODEL_6581 | PSID_SIDMODEL_8580
};

// Format strings
const char TXT_FORMAT_PSID[]  = "PlaySID one-file format (PSID)";
const char TXT_FORMAT_RSID[]  = "Real C64 one-file format (RSID)";
const char TXT_UNKNOWN_PSID[] = "Unsupported PSID version";
const char TXT_UNKNOWN_RSID[] = "Unsupported RSID version";
const char TXT_V5_RESERVED_SID_CONFIG[]   = "Reserved SID address configuration";
const char TXT_V5_TOO_MANY_SIDS[]         = "SID count exceeds the generated SID addresses";
const char TXT_V5_FMOPL_OVERLAP[]         = "SID address overlaps the FM OPL range";
const char TXT_V5_SONGLENGTHS_TRUNCATED[] = "Song length table does not fit in the file";

constexpr int psid_headerSize = 118;
constexpr int psidv2_headerSize = psid_headerSize + 6;

// Magic fields
constexpr uint32_t PSID_ID = 0x50534944;
constexpr uint32_t RSID_ID = 0x52534944;

/**
 * Decode SID model flags.
 */
SidTuneInfo::model_t getSidModel(uint_least16_t modelFlag)
{
    if ((modelFlag & PSID_SIDMODEL_ANY) == PSID_SIDMODEL_ANY)
        return SidTuneInfo::SIDMODEL_ANY;

    if (modelFlag & PSID_SIDMODEL_6581)
        return SidTuneInfo::SIDMODEL_6581;

    if (modelFlag & PSID_SIDMODEL_8580)
        return SidTuneInfo::SIDMODEL_8580;

    return SidTuneInfo::SIDMODEL_UNKNOWN;
}

/**
 * Check if extra SID addres is valid for PSID specs.
 */
bool validateAddress(uint_least8_t address)
{
    // Only even values are valid.
    if (address & 1)
        return false;

    // Ranges $00-$41 ($D000-$D410) and $80-$DF ($D800-$DDF0) are invalid.
    // Any invalid value means that no second SID is used, like $00.
    if (address <= 0x41
            || (address >= 0x80 && address <= 0xdf))
        return false;

    return true;
}

/**
 * Generate the SID addresses of a version 5 file.
 *
 * @param multiSidConfig bits 0-3 SID count, bits 4-7 address configuration
 * @param startByte address of SID #2 as in the version 3 fields, 0 for the standard start
 * @param fmOpl the FM OPL range $DF00-$DFFF is reserved
 * @param addresses in: SID #1, out: SID #1 and the generated addresses
 * @throw loadError
 */
void generateSidAddresses(uint_least8_t multiSidConfig, uint_least8_t startByte,
                          bool fmOpl, std::vector<uint_least16_t> &addresses)
{
    const unsigned int count = multiSidConfig & 0x0f;
    if (count < 2)
        return;

    const unsigned int config = multiSidConfig >> 4;
    if (config > 6)
        throw loadError(TXT_V5_RESERVED_SID_CONFIG);

    static const unsigned int steps[7][2] =
    {
        { 0x0020, 0x0020 },
        { 0x0040, 0x0040 },
        { 0x0080, 0x0080 },
        { 0x0100, 0x0100 },
        { 0x0020, 0x0060 },
        { 0x0020, 0x00e0 },
        { 0x0100, 0x0300 }
    };

    // An invalid start byte means the standard start address
    const bool hasStart = validateAddress(startByte);

    unsigned int address = 0xd400;
    for (unsigned int i = 1; i < count; i++)
    {
        if (i == 1 && hasStart)
        {
            address = 0xd000 | (startByte << 4);
        }
        else
        {
            // With a start byte the first step goes from SID #2 to SID #3
            const unsigned int stepIdx = (i - (hasStart ? 2 : 1)) & 1;
            address += steps[config][stepIdx];

            // Resume at $DE00 keeping the overshoot past $D7E0
            if (address >= 0xd800 && address < 0xde00)
                address += 0xde00 - 0xd800;
        }

        if (address > 0xdfe0)
            break;

        if (fmOpl && address >= 0xdf00)
            throw loadError(TXT_V5_FMOPL_OVERLAP);

        addresses.push_back(address);
    }

    if (addresses.size() < count)
        throw loadError(TXT_V5_TOO_MANY_SIDS);
}

/**
 * Decode the version 5 metadata area.
 *
 * @param area the 96 bytes at offset 22
 * @param strings receives name, author and released on success
 * @return false if the area does not hold three valid strings
 */
bool parseMetadataV5(const uint8_t *area, std::vector<std::string> &strings)
{
    constexpr int len = PSID_V5_METADATALEN;

    // Name
    int nameEnd = 0;
    while (nameEnd < len && area[nameEnd] != 0)
        nameEnd++;
    if (nameEnd == 0 || nameEnd == len)
        return false;

    // Author, skip the padding
    int authorStart = nameEnd + 1;
    while (authorStart < len && area[authorStart] == 0)
        authorStart++;
    int authorEnd = authorStart;
    while (authorEnd < len && area[authorEnd] != 0)
        authorEnd++;
    if (authorStart == len || authorEnd == len)
        return false;

    // Released, a terminator is not needed if it fills the area
    int releasedStart = authorEnd + 1;
    while (releasedStart < len && area[releasedStart] == 0)
        releasedStart++;
    int releasedEnd = releasedStart;
    while (releasedEnd < len && area[releasedEnd] != 0)
        releasedEnd++;
    if (releasedStart == len)
        return false;

    const char *chars = reinterpret_cast<const char*>(area);
    strings.push_back(std::string(chars, nameEnd));
    strings.push_back(std::string(chars + authorStart, authorEnd - authorStart));
    strings.push_back(std::string(chars + releasedStart, releasedEnd - releasedStart));
    return true;
}

/**
 * Compute the version 5 panning of every SID.
 *
 * @param layout flags bits 6-7
 * @param mode flags bits 8-9
 * @param n the number of SIDs
 */
std::vector<SidTuneInfo::pan_t> computePanning(unsigned int layout, unsigned int mode, unsigned int n)
{
    typedef SidTuneInfo::pan_t pan_t;

    std::vector<pan_t> pan(n, SidTuneInfo::PAN_CENTER);

    if (n < 2 || layout == 3)
        return pan;

    const unsigned int centerFirstCount = n - (n / 3) * 2;

    switch (mode)
    {
    case 0: // Direct
    case 1: // Reverse
        for (unsigned int i = 0; i < n; i++)
        {
            switch (layout)
            {
            case 0:
                pan[i] = (i % 2 == 0) ? SidTuneInfo::PAN_LEFT : SidTuneInfo::PAN_RIGHT;
                break;
            case 1:
                pan[i] = (i % 3 == 0) ? SidTuneInfo::PAN_LEFT
                    : (i % 3 == 1) ? SidTuneInfo::PAN_CENTER : SidTuneInfo::PAN_RIGHT;
                break;
            default:
                pan[i] = (i < centerFirstCount) ? SidTuneInfo::PAN_CENTER
                    : ((i - centerFirstCount) % 2 == 0) ? SidTuneInfo::PAN_LEFT : SidTuneInfo::PAN_RIGHT;
                break;
            }
        }
        if (mode == 1)
        {
            // Swap left and right
            for (unsigned int i = 0; i < n; i++)
            {
                if (pan[i] == SidTuneInfo::PAN_LEFT)
                    pan[i] = SidTuneInfo::PAN_RIGHT;
                else if (pan[i] == SidTuneInfo::PAN_RIGHT)
                    pan[i] = SidTuneInfo::PAN_LEFT;
            }
            // Center first: move the center assignments to the end
            if (layout == 2)
            {
                std::vector<pan_t> moved;
                for (unsigned int i = 0; i < n; i++)
                    if (pan[i] != SidTuneInfo::PAN_CENTER)
                        moved.push_back(pan[i]);
                moved.resize(n, SidTuneInfo::PAN_CENTER);
                pan.swap(moved);
            }
        }
        break;

    case 2: // Group
        if (layout == 0)
        {
            // Groups of two, alternating left and right, two SIDs are split
            const unsigned int groupSize = (n == 2) ? 1 : 2;
            for (unsigned int i = 0; i < n; i++)
                pan[i] = ((i / groupSize) % 2 == 0) ? SidTuneInfo::PAN_LEFT : SidTuneInfo::PAN_RIGHT;
        }
        else if (layout == 1)
        {
            // One group of up to 9 SIDs or a group of 6 and the remaining ones
            const unsigned int first = (n <= 9) ? n : 6;
            unsigned int pos = 0;
            for (unsigned int g = 0; g < 2; g++)
            {
                const unsigned int size = (g == 0) ? first : n - first;
                const unsigned int sides = (size + 1) / 3;
                for (unsigned int i = 0; i < size; i++, pos++)
                {
                    pan[pos] = (i < sides) ? SidTuneInfo::PAN_LEFT
                        : (i < size - sides) ? SidTuneInfo::PAN_CENTER : SidTuneInfo::PAN_RIGHT;
                }
            }
        }
        else
        {
            const unsigned int sides = n / 3;
            const unsigned int centers = n - sides * 2;
            unsigned int pos = centers;
            if (sides == 1)
            {
                pan[pos++] = SidTuneInfo::PAN_LEFT;
                pan[pos++] = SidTuneInfo::PAN_RIGHT;
            }
            else if (sides > 1)
            {
                unsigned int remaining = sides;
                if (sides % 2 != 0)
                {
                    // Odd: three on each side first
                    for (unsigned int i = 0; i < 3; i++)
                        pan[pos++] = SidTuneInfo::PAN_LEFT;
                    for (unsigned int i = 0; i < 3; i++)
                        pan[pos++] = SidTuneInfo::PAN_RIGHT;
                    remaining -= 3;
                }
                // Alternate pairs, left first, remaining is even
                for (bool left = true; remaining > 0; left = !left)
                {
                    const SidTuneInfo::pan_t side = left ? SidTuneInfo::PAN_LEFT : SidTuneInfo::PAN_RIGHT;
                    pan[pos++] = side;
                    pan[pos++] = side;
                    if (!left)
                        remaining -= 2;
                }
            }
        }
        break;

    default: // Spread
        if (layout == 0)
        {
            // An odd total adds one to the left
            const unsigned int left = (n + 1) / 2;
            for (unsigned int i = 0; i < n; i++)
                pan[i] = (i < left) ? SidTuneInfo::PAN_LEFT : SidTuneInfo::PAN_RIGHT;
        }
        else
        {
            const unsigned int sides = n / 3;
            const unsigned int centers = n - sides * 2;
            for (unsigned int i = 0; i < n; i++)
            {
                if (layout == 1) // left, center, right
                {
                    pan[i] = (i < sides) ? SidTuneInfo::PAN_LEFT
                        : (i < sides + centers) ? SidTuneInfo::PAN_CENTER : SidTuneInfo::PAN_RIGHT;
                }
                else // center, left, right
                {
                    pan[i] = (i < centers) ? SidTuneInfo::PAN_CENTER
                        : (i < centers + sides) ? SidTuneInfo::PAN_LEFT : SidTuneInfo::PAN_RIGHT;
                }
            }
        }
        break;
    }

    return pan;
}

/**
 * Decode an embedded song length entry to milliseconds.
 * Two BCD words, MM:SS and 0MMM.
 *
 * @return 0 if the entry is not valid
 */
uint_least32_t decodeSongLength(const uint8_t *entry)
{
    // Every nibble must be a decimal digit and the milliseconds
    // most significant digit must be zero
    for (int i = 0; i < 4; i++)
    {
        if ((entry[i] >> 4) > 9 || (entry[i] & 0x0f) > 9)
            return 0;
    }

    if ((entry[2] >> 4) != 0)
        return 0;

    const unsigned int minutes = (entry[0] >> 4) * 10 + (entry[0] & 0x0f);
    const unsigned int seconds = (entry[1] >> 4) * 10 + (entry[1] & 0x0f);
    if (seconds > 59)
        return 0;

    const unsigned int millis = (entry[2] & 0x0f) * 100 + (entry[3] >> 4) * 10 + (entry[3] & 0x0f);

    return (minutes * 60 + seconds) * 1000 + millis;
}

SidTuneBase* PSID::load(buffer_t& dataBuf)
{
    // File format check
    if (dataBuf.size() < 4)
    {
        return nullptr;
    }

    const uint32_t magic = endian_big32(&dataBuf[0]);
    if ((magic != PSID_ID)
        && (magic != RSID_ID))
    {
        return nullptr;
    }

    psidHeader pHeader;
    readHeader(dataBuf, pHeader);

    std::unique_ptr<PSID> tune(new PSID());
    tune->tryLoad(pHeader, dataBuf);

    return tune.release();
}

void PSID::readHeader(const buffer_t &dataBuf, psidHeader &hdr)
{
    // Due to security concerns, input must be at least as long as version 1
    // header plus 16-bit C64 load address. That is the area which will be
    // accessed.
    if (dataBuf.size() < (psid_headerSize + 2))
    {
        throw loadError(ERR_TRUNCATED);
    }

    // Read v1 fields
    hdr.id               = endian_big32(&dataBuf[0]);
    hdr.version          = endian_big16(&dataBuf[4]);
    hdr.data             = endian_big16(&dataBuf[6]);
    hdr.load             = endian_big16(&dataBuf[8]);
    hdr.init             = endian_big16(&dataBuf[10]);
    hdr.play             = endian_big16(&dataBuf[12]);
    hdr.songs            = endian_big16(&dataBuf[14]);
    hdr.start            = endian_big16(&dataBuf[16]);
    hdr.speed            = endian_big32(&dataBuf[18]);

    std::memcpy(hdr.name,     &dataBuf[22], PSID_MAXSTRLEN);
    std::memcpy(hdr.author,   &dataBuf[54], PSID_MAXSTRLEN);
    std::memcpy(hdr.released, &dataBuf[86], PSID_MAXSTRLEN);
    std::memcpy(hdr.metadata, &dataBuf[22], PSID_V5_METADATALEN);

    if (hdr.version >= 2)
    {
        if (dataBuf.size() < (psidv2_headerSize + 2))
        {
            throw loadError(ERR_TRUNCATED);
        }

        // Read v2/3/4 fields
        hdr.flags            = endian_big16(&dataBuf[118]);
        hdr.relocStartPage   = dataBuf[120];
        hdr.relocPages       = dataBuf[121];
        hdr.sidChipBase2     = dataBuf[122];
        hdr.sidChipBase3     = dataBuf[123];
    }
}

void PSID::tryLoad(const psidHeader &pHeader, const buffer_t &dataBuf)
{
    SidTuneInfo::compatibility_t compatibility = SidTuneInfo::COMPATIBILITY_C64;

    // Require a valid ID and version number.
    if (pHeader.id == PSID_ID)
    {
       switch (pHeader.version)
       {
       case 1:
           compatibility = SidTuneInfo::COMPATIBILITY_PSID;
           break;
       case 2:
       case 3:
       case 4:
       case 5:
           break;
       default:
           throw loadError(TXT_UNKNOWN_PSID);
       }
       info->m_formatString = TXT_FORMAT_PSID;
    }
    else if (pHeader.id == RSID_ID)
    {
       switch (pHeader.version)
       {
       case 2:
       case 3:
       case 4:
       case 5:
           break;
       default:
           throw loadError(TXT_UNKNOWN_RSID);
       }
       info->m_formatString = TXT_FORMAT_RSID;
       compatibility = SidTuneInfo::COMPATIBILITY_R64;
    }

    m_fileOffset           = pHeader.data;
    info->m_loadAddr       = pHeader.load;
    info->m_initAddr       = pHeader.init;
    info->m_playAddr       = pHeader.play;
    info->m_songs          = pHeader.songs;
    info->m_startSong      = pHeader.start;
    info->m_compatibility  = compatibility;
    info->m_relocPages     = 0;
    info->m_relocStartPage = 0;

    uint_least32_t speed = pHeader.speed;
    SidTuneInfo::clock_t clock = SidTuneInfo::CLOCK_UNKNOWN;

    bool musPlayer = false;

    if (pHeader.version >= 2)
    {
        const uint_least16_t flags = pHeader.flags;

        // Check clock
        if (flags & PSID_MUS)
        {   // MUS tunes run at any speed
            clock = SidTuneInfo::CLOCK_ANY;
            musPlayer = true;
        }
        else
        {
            switch (flags & PSID_CLOCK)
            {
            case PSID_CLOCK_ANY:
                clock = SidTuneInfo::CLOCK_ANY;
                break;
            case PSID_CLOCK_PAL:
                clock = SidTuneInfo::CLOCK_PAL;
                break;
            case PSID_CLOCK_NTSC:
                clock = SidTuneInfo::CLOCK_NTSC;
                break;
            default:
                break;
            }
        }

        // These flags are only available for the appropriate
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
        default:
            break;
        }

        info->m_clockSpeed = clock;

        info->m_sidModels[0] = getSidModel(flags >> 4);

        info->m_relocStartPage = pHeader.relocStartPage;
        info->m_relocPages     = pHeader.relocPages;

        if (pHeader.version == 5)
        {
            // The bits 6-9 and the offsets 122 and 123 have another meaning
            // in version 5, all SIDs use the model of the first one
            generateSidAddresses(pHeader.sidChipBase2, pHeader.sidChipBase3,
                                 (flags & PSID_FMOPL) != 0, info->m_sidChipAddresses);

            const unsigned int sids = info->m_sidChipAddresses.size();
            const SidTuneInfo::model_t model = info->m_sidModels[0];
            info->m_sidModels.assign(sids, model);

            info->m_hasPanning = true;
            info->m_panLayout = static_cast<SidTuneInfo::panlayout_t>((flags & PSID_PANLAYOUT) >> 6);
            info->m_panMode = static_cast<SidTuneInfo::panmode_t>((flags & PSID_PANMODE) >> 8);
            info->m_sidPannings = computePanning((flags & PSID_PANLAYOUT) >> 6, (flags & PSID_PANMODE) >> 8, sids);

            info->m_fmOpl = (flags & PSID_FMOPL) != 0;

            if (flags & PSID_SONGLENGTHS)
            {
                // The table has one entry per song and ends the file
                const size_t songs = (pHeader.songs != 0) ? pHeader.songs : 1;
                const size_t tableLen = songs * 4;
                if (dataBuf.size() < pHeader.data || (dataBuf.size() - pHeader.data) < tableLen)
                {
                    throw loadError(TXT_V5_SONGLENGTHS_TRUNCATED);
                }

                const uint8_t *table = &dataBuf[dataBuf.size() - tableLen];
                const size_t entries = (songs < MAX_SONGS) ? songs : MAX_SONGS;
                for (size_t i = 0; i < entries; i++)
                {
                    info->m_songLengths.push_back(decodeSongLength(table + i * 4));
                }

                m_trailerLen = tableLen;
            }
        }
        else if (pHeader.version >= 3)
        {
            if (validateAddress(pHeader.sidChipBase2))
            {
                info->m_sidChipAddresses.push_back(0xd000 | (pHeader.sidChipBase2 << 4));

                info->m_sidModels.push_back(getSidModel(flags >> 6));
            }

            if (pHeader.version >= 4)
            {
                if (pHeader.sidChipBase3 != pHeader.sidChipBase2
                    && validateAddress(pHeader.sidChipBase3))
                {
                    info->m_sidChipAddresses.push_back(0xd000 | (pHeader.sidChipBase3 << 4));

                    info->m_sidModels.push_back(getSidModel(flags >> 8));
                }
            }
        }
    }

    // Check reserved fields to force real c64 compliance
    // as required by the RSID specification
    if (compatibility == SidTuneInfo::COMPATIBILITY_R64)
    {
        if ((info->m_loadAddr != 0)
            || (info->m_playAddr != 0)
            || (speed != 0))
        {
            throw loadError(ERR_INVALID);
        }

        // Real C64 tunes appear as CIA
        speed = ~0;
    }

    // Create the speed/clock setting table.
    convertOldStyleSpeedToTables(speed, clock);

    // Copy info strings.
    // Version 5 falls back to the fixed size fields if the
    // variable length ones are not valid.
    if (pHeader.version != 5 || !parseMetadataV5(pHeader.metadata, info->m_infoString))
    {
        info->m_infoString.push_back(std::string(pHeader.name, PSID_MAXSTRLEN));
        info->m_infoString.push_back(std::string(pHeader.author, PSID_MAXSTRLEN));
        info->m_infoString.push_back(std::string(pHeader.released, PSID_MAXSTRLEN));
    }

    if (musPlayer)
        throw loadError("Compute!'s Sidplayer MUS data is not supported yet"); // TODO
}

const char *PSID::createMD5(char *md5)
{
    if (md5 == nullptr)
        md5 = m_md5;

    *md5 = '\0';

    // Include C64 data.
    sidmd5 myMD5;
    myMD5.append(&m_cache[m_fileOffset], info->m_c64dataLen);

    uint8_t tmp[2];
    // Include INIT and PLAY address.
    endian_little16(tmp, info->m_initAddr);
    myMD5.append(tmp, sizeof(tmp));
    endian_little16(tmp, info->m_playAddr);
    myMD5.append(tmp, sizeof(tmp));

    // Include number of songs.
    endian_little16(tmp, info->m_songs);
    myMD5.append(tmp, sizeof(tmp));

    {
        // Include song speed for each song.
        const unsigned int currentSong = info->m_currentSong;
        for (unsigned int s = 1; s <= info->m_songs; s++)
        {
            selectSong(s);
            const uint8_t songSpeed = static_cast<uint8_t>(info->m_songSpeed);
            myMD5.append(&songSpeed, sizeof(songSpeed));
        }
        // Restore old song
        selectSong(currentSong);
    }

    // Deal with PSID v2NG clock speed flags: Let only NTSC
    // clock speed change the MD5 fingerprint. That way the
    // fingerprint of a PAL-speed sidtune in PSID v1, v2, and
    // PSID v2NG format is the same.
    if (info->m_clockSpeed == SidTuneInfo::CLOCK_NTSC)
    {
        const uint8_t ntsc_val = 2;
        myMD5.append(&ntsc_val, sizeof(ntsc_val));
    }

    // NB! If the fingerprint is used as an index into a
    // song-lengths database or cache, modify above code to
    // allow for PSID v2NG files which have clock speed set to
    // SIDTUNE_CLOCK_ANY. If the SID player program fully
    // supports the SIDTUNE_CLOCK_ANY setting, a sidtune could
    // either create two different fingerprints depending on
    // the clock speed chosen by the player, or there could be
    // two different values stored in the database/cache.

    // Get fingerprint.
    myMD5.getDigest().copy(md5, SidTune::MD5_LENGTH);
    md5[SidTune::MD5_LENGTH] = '\0';

    return md5;
}

const char *PSID::createMD5New(char *md5)
{
    if (md5 == nullptr)
        md5 = m_md5;

    *md5 = '\0';

    // The calculation is now simplified
    // All the header + all the data
    sidmd5 myMD5;
    myMD5.append(&m_cache[0], m_cache.size());

    // Get fingerprint.
    myMD5.getDigest().copy(md5, SidTune::MD5_LENGTH);
    md5[SidTune::MD5_LENGTH] = '\0';

    return md5;
}

}
