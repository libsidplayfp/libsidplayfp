/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2026 LouD
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

#include "utpp/utpp.h"

#include "../src/sidplayfp/SidTune.h"
#include "../src/sidplayfp/SidTuneInfo.h"

#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#define HEADERSIZE 124
#define METADATA    22
#define METADATALEN 96
#define FLAGS_HI   118
#define FLAGS_LO   119
#define MULTISIDCONFIG    122
#define SIDADDRESSSTART   123

#define FLAG_FMOPL       0x0800
#define FLAG_SONGLENGTHS 0x0400

using namespace UnitTest;

// Compare C strings by content, CHECK_EQUAL on char pointers compares the first character only
#define CHECK_STR(expected, actual) CHECK_EQUAL(std::string(expected), std::string(actual))

namespace
{

typedef std::vector<uint8_t> Buffer;

const char ERR_RESERVED[]  = "Reserved SID address configuration";
const char ERR_TOOMANY[]   = "SID count exceeds the generated SID addresses";
const char ERR_FMOPL[]     = "SID address overlaps the FM OPL range";
const char ERR_TRUNCATED[] = "Song length table does not fit in the file";

/*
 * A version 5 header with four bytes of C64 data.
 * The metadata area holds the fixed size fields "N", "A" and "R".
 */
Buffer makeHeader(unsigned int version, unsigned int flags, unsigned int multiSidConfig,
                  unsigned int sidAddressStart, unsigned int songs = 1)
{
    const uint8_t header[HEADERSIZE] = {
        'P', 'S', 'I', 'D',
        0x00, static_cast<uint8_t>(version),
        0x00, 0x7C,             // dataOffset
        0x10, 0x00,             // loadAddress
        0x10, 0x00,             // initAddress
        0x10, 0x03,             // playAddress
        static_cast<uint8_t>(songs >> 8), static_cast<uint8_t>(songs),
        0x00, 0x01,             // startSong
        0x00, 0x00, 0x00, 0x00, // speed
    };

    Buffer data(header, header + 22);
    data.resize(HEADERSIZE, 0);
    data[METADATA] = 'N';
    data[METADATA + 32] = 'A';
    data[METADATA + 64] = 'R';
    data[FLAGS_HI] = static_cast<uint8_t>(flags >> 8);
    data[FLAGS_LO] = static_cast<uint8_t>(flags);
    data[MULTISIDCONFIG] = static_cast<uint8_t>(multiSidConfig);
    data[SIDADDRESSSTART] = static_cast<uint8_t>(sidAddressStart);

    // C64 data
    data.push_back(0xea);
    data.push_back(0xea);
    data.push_back(0xea);
    data.push_back(0x60);

    return data;
}

Buffer makeV5(unsigned int flags, unsigned int multiSidConfig, unsigned int sidAddressStart)
{
    return makeHeader(5, flags, multiSidConfig, sidAddressStart);
}

/*
 * Put the three strings in the metadata area.
 * The area is cleared first, each string is followed by padding bytes.
 */
void setMetadata(Buffer &data, const std::string &name, const std::string &author,
                 const std::string &released, size_t padding = 0)
{
    std::memset(&data[METADATA], 0, METADATALEN);
    size_t pos = 0;
    const std::string *strings[3] = { &name, &author, &released };
    for (int i = 0; i < 3; i++)
    {
        std::memcpy(&data[METADATA + pos], strings[i]->data(), strings[i]->size());
        pos += strings[i]->size() + 1 + padding;
    }
}

/*
 * Append a song length table.
 */
void appendLengths(Buffer &data, const uint8_t (*entries)[4], size_t count)
{
    for (size_t i = 0; i < count; i++)
        data.insert(data.end(), entries[i], entries[i] + 4);
}

/*
 * Panning as a string of L, C and R.
 */
std::string panning(const SidTuneInfo *info)
{
    std::string result;
    for (int i = 0; i < info->sidChips(); i++)
    {
        switch (info->sidPanning(i))
        {
        case SidTuneInfo::PAN_LEFT:  result += 'L'; break;
        case SidTuneInfo::PAN_RIGHT: result += 'R'; break;
        default:                     result += 'C'; break;
        }
    }
    return result;
}

/*
 * SID addresses as a space separated list of hexadecimal values.
 */
std::string addresses(const SidTuneInfo *info)
{
    std::string result;
    char buf[16];
    for (int i = 0; i < info->sidChips(); i++)
    {
        std::snprintf(buf, sizeof(buf), i ? " %04x" : "%04x", info->sidChipBase(i));
        result += buf;
    }
    return result;
}

// Addresses of a file with count SIDs
std::string addrOf(unsigned int flags, unsigned int config, unsigned int count, unsigned int start)
{
    const Buffer data = makeV5(flags, (config << 4) | count, start);
    SidTune tune(&data[0], data.size());
    return addresses(tune.getInfo());
}

std::string status(const Buffer &data)
{
    SidTune tune(&data[0], data.size());
    return tune.getStatus() ? "ok" : tune.statusString();
}

// multiSidConfig from count and configuration
unsigned int multi(unsigned int config, unsigned int count)
{
    return (config << 4) | count;
}

// flags from panning layout and mode
unsigned int pan(unsigned int layout, unsigned int mode)
{
    return (layout << 6) | (mode << 8);
}

// Panning of a file with n SIDs
std::string panOf(unsigned int layout, unsigned int mode, unsigned int n)
{
    const Buffer data = makeV5(pan(layout, mode), multi(0, n), 0);
    SidTune tune(&data[0], data.size());
    return panning(tune.getInfo());
}

}

SUITE(PSIDv5)
{

/*
 * A version 5 file without extra SIDs loads like a version 2 file.
 */
TEST(TestLoadOk)
{
    const Buffer data = makeV5(0, 0, 0);
    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_STR("No errors", tune.statusString());

    const SidTuneInfo *info = tune.getInfo();
    CHECK_EQUAL(1, info->sidChips());
    CHECK_EQUAL(0xd400, info->sidChipBase(0));
    CHECK_EQUAL(4u, info->c64dataLen());
    CHECK_EQUAL(static_cast<uint_least32_t>(data.size()), info->dataFileLen());
    CHECK_STR("N", info->infoString(0));
    CHECK_STR("A", info->infoString(1));
    CHECK_STR("R", info->infoString(2));
    CHECK(info->hasPanning());
    CHECK_STR("C", panning(info));
    CHECK(!info->fmOpl());
    CHECK_EQUAL(0u, info->songLength(1));
}

/*
 * RSID accepts version 5 too.
 */
TEST(TestLoadRsidOk)
{
    Buffer data = makeV5(0, multi(0, 2), 0);
    std::memcpy(&data[0], "RSID", 4);
    data[8] = data[9] = 0;      // loadAddress
    data[10] = data[11] = 0;    // initAddress
    data[12] = data[13] = 0;    // playAddress
    data[HEADERSIZE] = 0xe8;    // actual load address
    data[HEADERSIZE + 1] = 0x07;

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_EQUAL(2, tune.getInfo()->sidChips());
    CHECK_EQUAL(0xd420, tune.getInfo()->sidChipBase(1));
}

/*
 * Version 6 is not supported.
 */
TEST(TestUnsupportedVersion)
{
    CHECK_STR("Unsupported PSID version", status(makeHeader(6, 0, 0, 0)));
}

///////////////
// METADATA  //
///////////////

/*
 * Strings longer than 32 characters use the variable length layout.
 */
TEST(TestLongMetadata)
{
    Buffer data = makeV5(0, 0, 0);
    const std::string name(40, 'n');
    const std::string author(35, 'a');
    const std::string released(10, 'r');
    setMetadata(data, name, author, released);

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_EQUAL(3u, tune.getInfo()->numberOfInfoStrings());
    CHECK_STR(name.c_str(), tune.getInfo()->infoString(0));
    CHECK_STR(author.c_str(), tune.getInfo()->infoString(1));
    CHECK_STR(released.c_str(), tune.getInfo()->infoString(2));
}

/*
 * Padding after the terminators is skipped.
 */
TEST(TestMetadataPadding)
{
    Buffer data = makeV5(0, 0, 0);
    setMetadata(data, std::string(33, 'n'), "Author", "Released", 7);

    SidTune tune(&data[0], data.size());
    CHECK_STR(std::string(33, 'n').c_str(), tune.getInfo()->infoString(0));
    CHECK_STR("Author", tune.getInfo()->infoString(1));
    CHECK_STR("Released", tune.getInfo()->infoString(2));
}

/*
 * The released string needs no terminator if it fills the area.
 */
TEST(TestMetadataReleasedFillsArea)
{
    Buffer data = makeV5(0, 0, 0);
    const std::string name(30, 'n');
    const std::string author(30, 'a');
    // 96 - 31 - 31 = 34
    const std::string released(34, 'r');
    setMetadata(data, name, author, released);

    SidTune tune(&data[0], data.size());
    CHECK_STR(released.c_str(), tune.getInfo()->infoString(2));
}

/*
 * Three fields of 32 bytes without terminator for the name fall back
 * to the fixed layout, the example of the specification.
 */
TEST(TestMetadataFallback)
{
    Buffer data = makeV5(0, 0, 0);
    std::memset(&data[METADATA], 0, METADATALEN);
    std::memcpy(&data[METADATA], "TitleThatIsExactly32OfCharacters", 32);
    std::memcpy(&data[METADATA + 32], "Author", 6);
    std::memcpy(&data[METADATA + 64], "Released", 8);

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_STR("TitleThatIsExactly32OfCharacters", tune.getInfo()->infoString(0));
    CHECK_STR("Author", tune.getInfo()->infoString(1));
    CHECK_STR("Released", tune.getInfo()->infoString(2));
}

/*
 * An empty field makes the variable length layout invalid, the file
 * loads with the fixed layout and is never rejected.
 */
TEST(TestMetadataFallbackEmptyField)
{
    Buffer data = makeV5(0, 0, 0);
    std::memset(&data[METADATA], 0, METADATALEN);
    std::memcpy(&data[METADATA], "Name", 4);
    std::memcpy(&data[METADATA + 64], "Released", 8);

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_STR("Name", tune.getInfo()->infoString(0));
    CHECK_STR("", tune.getInfo()->infoString(1));
    CHECK_STR("Released", tune.getInfo()->infoString(2));
}

/*
 * An unterminated author makes the variable length layout invalid.
 */
TEST(TestMetadataFallbackUnterminatedAuthor)
{
    Buffer data = makeV5(0, 0, 0);
    std::memset(&data[METADATA], 0, METADATALEN);
    std::memcpy(&data[METADATA], "Name", 4);
    std::memset(&data[METADATA + 5], 'a', METADATALEN - 5);

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_STR("Name", tune.getInfo()->infoString(0));
    CHECK_STR(std::string(32, 'a').c_str(), tune.getInfo()->infoString(1));
}

/*
 * An all zero area gives empty strings.
 */
TEST(TestMetadataAllZero)
{
    Buffer data = makeV5(0, 0, 0);
    std::memset(&data[METADATA], 0, METADATALEN);

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_EQUAL(3u, tune.getInfo()->numberOfInfoStrings());
    CHECK_STR("", tune.getInfo()->infoString(0));
}

///////////////
// ADDRESSES //
///////////////

/*
 * Single step configurations.
 */
TEST(TestAddressConfigsSingleStep)
{
    CHECK_STR("d400 d420 d440 d460", addrOf(0, 0, 4, 0));
    CHECK_STR("d400 d440 d480 d4c0", addrOf(0, 1, 4, 0));
    CHECK_STR("d400 d480 d500 d580", addrOf(0, 2, 4, 0));
    CHECK_STR("d400 d500 d600 d700 de00 df00", addrOf(0, 3, 6, 0));
}

/*
 * Alternating step configurations.
 */
TEST(TestAddressConfigsAlternating)
{
    CHECK_STR("d400 d420 d480 d4a0 d500", addrOf(0, 4, 5, 0));
    CHECK_STR("d400 d420 d500 d520 d600", addrOf(0, 5, 5, 0));
    CHECK_STR("d400 d500 de00 df00", addrOf(0, 6, 4, 0));
}

/*
 * Maximum SID counts of the configurations, one more is rejected.
 */
TEST(TestAddressMaxCounts)
{
    const unsigned int maxCount[7] = { 15, 15, 12, 6, 15, 12, 4 };
    for (unsigned int config = 0; config < 7; config++)
    {
        CHECK_STR("ok", status(makeV5(0, multi(config, maxCount[config]), 0)));
        if (maxCount[config] < 15)
        {
            CHECK_STR(ERR_TOOMANY, status(makeV5(0, multi(config, maxCount[config] + 1), 0)));
        }
    }
}

/*
 * The last of 15 SIDs in configuration 0 is at $D5C0.
 */
TEST(TestAddressFifteenSids)
{
    const Buffer data = makeV5(0, multi(0, 15), 0);
    SidTune tune(&data[0], data.size());
    CHECK_EQUAL(15, tune.getInfo()->sidChips());
    CHECK_EQUAL(0xd5c0, tune.getInfo()->sidChipBase(14));
    CHECK_EQUAL(0, tune.getInfo()->sidChipBase(15));
}

/*
 * A valid start byte replaces the address of SID #2.
 */
TEST(TestAddressStartOverride)
{
    // Examples of the specification
    CHECK_STR("d400 d600 d620 d640", addrOf(0, 0, 4, 0x60));
    CHECK_STR("d400 de00 df00", addrOf(0, 3, 3, 0xe0));
    CHECK_STR(ERR_TOOMANY, status(makeV5(0, multi(3, 4), 0xe0)));
    // Lowest and highest valid values
    CHECK_STR("d400 d420", addrOf(0, 0, 2, 0x42));
    CHECK_STR("d400 dfe0", addrOf(0, 0, 2, 0xfe));
}

/*
 * With a start byte the first step of an alternating configuration
 * goes from SID #2 to SID #3.
 */
TEST(TestAddressStartOverrideAlternating)
{
    CHECK_STR("d400 d600 d620 d680 d6a0", addrOf(0, 4, 5, 0x60));
    CHECK_STR("d400 d600 d620 d700 d720", addrOf(0, 5, 5, 0x60));
    CHECK_STR("d400 d500 d600 df00", addrOf(0, 6, 4, 0x50));
}

/*
 * A start byte outside the valid ranges or odd is the standard start.
 */
TEST(TestAddressInvalidStartByte)
{
    const unsigned int invalid[] = { 0x01, 0x40, 0x41, 0x43, 0x80, 0xa0, 0xdf, 0xe1, 0xff };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
    {
        const Buffer data = makeV5(0, multi(0, 3), invalid[i]);
        SidTune tune(&data[0], data.size());
        CHECK_STR("d400 d420 d440", addresses(tune.getInfo()));
    }
}

/*
 * A step past $D7E0 resumes at $DE00 keeping the overshoot.
 */
TEST(TestAddressWrap)
{
    // $D700 + $0100 = $D800
    CHECK_STR("d400 d500 d600 d700 de00", addrOf(0, 3, 5, 0));
    // $D7E0 + $0080 = $D860
    CHECK_STR("d400 d7e0 de60", addrOf(0, 2, 3, 0x7e));
    // $D7E0 + $0040 = $D820
    CHECK_STR("d400 d7e0 de20 de60", addrOf(0, 1, 4, 0x7e));
    // $D7E0 + $0020 = $D800
    CHECK_STR("d400 d7e0 de00 de20", addrOf(0, 0, 4, 0x7e));
}

/*
 * No addresses are generated past $DFE0.
 */
TEST(TestAddressPastEnd)
{
    CHECK_STR("d400 dfe0", addrOf(0, 0, 2, 0xfe));
    CHECK_STR(ERR_TOOMANY, status(makeV5(0, multi(0, 3), 0xfe)));
    CHECK_STR(ERR_TOOMANY, status(makeV5(0, multi(3, 3), 0xfe)));
}

/*
 * A count of 0 or 1 is a single SID whatever the configuration.
 */
TEST(TestAddressSingleSid)
{
    for (unsigned int count = 0; count < 2; count++)
    {
        const Buffer data = makeV5(0, multi(3, count), 0x60);
        SidTune tune(&data[0], data.size());
        CHECK(tune.getStatus());
        CHECK_EQUAL(1, tune.getInfo()->sidChips());
        CHECK_STR("d400", addresses(tune.getInfo()));
    }
}

/*
 * Configurations 7 to 15 must not be used with more than one SID.
 */
TEST(TestAddressReservedConfig)
{
    for (unsigned int config = 7; config < 16; config++)
    {
        CHECK_STR(ERR_RESERVED, status(makeV5(0, multi(config, 2), 0)));
        CHECK_STR(ERR_RESERVED, status(makeV5(0, multi(config, 15), 0)));
        CHECK_STR("ok", status(makeV5(0, multi(config, 0), 0)));
        CHECK_STR("ok", status(makeV5(0, multi(config, 1), 0)));
    }
}

///////////
// MODEL //
///////////

/*
 * All SIDs use the model of bits 4-5, the version 3 and 4 bits 6-9 are ignored.
 */
TEST(TestSidModels)
{
    const SidTuneInfo::model_t models[4] = {
        SidTuneInfo::SIDMODEL_UNKNOWN,
        SidTuneInfo::SIDMODEL_6581,
        SidTuneInfo::SIDMODEL_8580,
        SidTuneInfo::SIDMODEL_ANY
    };

    for (unsigned int i = 0; i < 4; i++)
    {
        for (unsigned int others = 0; others < 16; others++)
        {
            const Buffer data = makeV5((i << 4) | (others << 6), multi(0, 5), 0);
            SidTune tune(&data[0], data.size());
            for (unsigned int chip = 0; chip < 5; chip++)
            {
                CHECK_EQUAL(models[i], tune.getInfo()->sidModel(chip));
            }
        }
    }
}

/*
 * Flags other than the model do not change the SID count and addresses,
 * the reserved bits 12-15 are ignored.
 */
TEST(TestReservedFlagsIgnored)
{
    const Buffer data = makeV5(0xf000, multi(0, 3), 0);
    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_STR("d400 d420 d440", addresses(tune.getInfo()));
    CHECK(!tune.getInfo()->fmOpl());
    CHECK_EQUAL(4u, tune.getInfo()->c64dataLen());
}

/////////////
// PANNING //
/////////////

TEST(TestPanningFields)
{
    for (unsigned int layout = 0; layout < 4; layout++)
    {
        for (unsigned int mode = 0; mode < 4; mode++)
        {
            const Buffer data = makeV5(pan(layout, mode), multi(0, 3), 0);
            SidTune tune(&data[0], data.size());
            CHECK(tune.getInfo()->hasPanning());
            CHECK_EQUAL(layout, static_cast<unsigned int>(tune.getInfo()->panLayout()));
            CHECK_EQUAL(mode, static_cast<unsigned int>(tune.getInfo()->panMode()));
        }
    }
}

/*
 * Rows of the tables of the specification.
 */
TEST(TestPanningTables)
{
    // Layout 0 standard, 1 L/C/R, 2 center first, 3 fully centered
    // Mode 0 direct, 1 reverse, 2 group, 3 spread
    CHECK_STR("C", panOf(0, 0, 1));
    CHECK_STR("LRL", panOf(0, 0, 3));
    CHECK_STR("LCRL", panOf(1, 0, 4));
    CHECK_STR("CCLRLR", panOf(2, 0, 6));
    CHECK_STR("CCCLRLR", panOf(2, 0, 7));
    CHECK_STR("CCCCC", panOf(3, 0, 5));

    CHECK_STR("RLR", panOf(0, 1, 3));
    CHECK_STR("RCLRC", panOf(1, 1, 5));
    CHECK_STR("RLC", panOf(2, 1, 3));
    CHECK_STR("RLRLCC", panOf(2, 1, 6));
    CHECK_STR("RLRLRLRLRLCCCCC", panOf(2, 1, 15));
    CHECK_STR("CCCC", panOf(3, 1, 4));

    CHECK_STR("LR", panOf(0, 2, 2));
    CHECK_STR("LLRRL", panOf(0, 2, 5));
    CHECK_STR("LCR", panOf(1, 2, 3));
    CHECK_STR("LLCCRRLCCR", panOf(1, 2, 10));
    CHECK_STR("LLCCRRLLLCCCRRR", panOf(1, 2, 15));
    CHECK_STR("CLR", panOf(2, 2, 3));
    CHECK_STR("CCLLRR", panOf(2, 2, 6));
    CHECK_STR("CCCLLLRRR", panOf(2, 2, 9));
    CHECK_STR("CCCCLLRRLLRR", panOf(2, 2, 12));
    CHECK_STR("CCCCCLLLRRRLLRR", panOf(2, 2, 15));
    CHECK_STR("CCC", panOf(3, 2, 3));

    CHECK_STR("LR", panOf(0, 3, 2));
    CHECK_STR("LLLRR", panOf(0, 3, 5));
    CHECK_STR("CC", panOf(1, 3, 2));
    CHECK_STR("LLCCRR", panOf(1, 3, 6));
    CHECK_STR("LLLLLCCCCCRRRRR", panOf(1, 3, 15));
    CHECK_STR("CCCCLLLLRRRR", panOf(2, 3, 12));
    CHECK_STR("CCCCCCCC", panOf(3, 3, 8));
}

/*
 * The position of a SID that does not exist is center.
 */
TEST(TestPanningOutOfRange)
{
    const Buffer data = makeV5(pan(0, 0), multi(0, 2), 0);
    SidTune tune(&data[0], data.size());
    CHECK_EQUAL(SidTuneInfo::PAN_CENTER, tune.getInfo()->sidPanning(2));
    CHECK_EQUAL(SidTuneInfo::PAN_CENTER, tune.getInfo()->sidPanning(100));
}

/////////////
// FM OPL  //
/////////////

TEST(TestFmOpl)
{
    const Buffer data = makeV5(FLAG_FMOPL, multi(0, 2), 0);
    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK(tune.getInfo()->fmOpl());

    // SID chips below $DF00 are fine
    CHECK_STR("ok", status(makeV5(FLAG_FMOPL, multi(3, 5), 0)));
    CHECK_STR("ok", status(makeV5(FLAG_FMOPL, multi(0, 2), 0xe0)));
}

/*
 * A SID chip inside $DF00-$DFFF is an invalid configuration.
 */
TEST(TestFmOplOverlap)
{
    CHECK_STR(ERR_FMOPL, status(makeV5(FLAG_FMOPL, multi(3, 6), 0)));
    CHECK_STR(ERR_FMOPL, status(makeV5(FLAG_FMOPL, multi(6, 4), 0)));
    CHECK_STR(ERR_FMOPL, status(makeV5(FLAG_FMOPL, multi(0, 2), 0xfe)));
    CHECK_STR(ERR_FMOPL, status(makeV5(FLAG_FMOPL, multi(0, 2), 0xf0)));
    // Without the flag the same files load
    CHECK_STR("ok", status(makeV5(0, multi(3, 6), 0)));
}

/////////////////
// SONGLENGTHS //
/////////////////

/*
 * The table is stripped from the C64 data and decoded.
 */
TEST(TestSongLengths)
{
    Buffer data = makeHeader(5, FLAG_SONGLENGTHS, 0, 0, 3);
    const uint8_t table[3][4] = {
        { 0x02, 0x34, 0x05, 0x00 },  // 2:34.500
        { 0x99, 0x59, 0x09, 0x99 },  // 99:59.999
        { 0x00, 0x00, 0x00, 0x00 }   // unknown
    };
    appendLengths(data, table, 3);

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());

    const SidTuneInfo *info = tune.getInfo();
    CHECK_EQUAL(4u, info->c64dataLen());
    CHECK_EQUAL(static_cast<uint_least32_t>(data.size()), info->dataFileLen());
    CHECK_EQUAL(3u, info->songs());
    CHECK_EQUAL(154500u, info->songLength(1));
    CHECK_EQUAL(5999999u, info->songLength(2));
    CHECK_EQUAL(0u, info->songLength(3));
    CHECK_EQUAL(0u, info->songLength(0));
    CHECK_EQUAL(0u, info->songLength(4));
    CHECK(tune.getStatus());
}

/*
 * The example of the specification.
 */
TEST(TestSongLengthsExample)
{
    Buffer data = makeHeader(5, FLAG_SONGLENGTHS, 0, 0, 6);
    const uint8_t table[6][4] = {
        { 0x02, 0x44, 0x05, 0x54 }, { 0x01, 0x16, 0x08, 0x10 },
        { 0x01, 0x04, 0x01, 0x58 }, { 0x02, 0x59, 0x04, 0x22 },
        { 0x01, 0x43, 0x04, 0x23 }, { 0x02, 0x09, 0x06, 0x17 }
    };
    appendLengths(data, table, 6);

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_EQUAL(164554u, tune.getInfo()->songLength(1));
    CHECK_EQUAL(76810u, tune.getInfo()->songLength(2));
    CHECK_EQUAL(64158u, tune.getInfo()->songLength(3));
    CHECK_EQUAL(179422u, tune.getInfo()->songLength(4));
    CHECK_EQUAL(103423u, tune.getInfo()->songLength(5));
    CHECK_EQUAL(129617u, tune.getInfo()->songLength(6));
    CHECK_EQUAL(4u, tune.getInfo()->c64dataLen());
}

/*
 * An invalid entry is unknown, the file still loads.
 */
TEST(TestSongLengthsBadBcd)
{
    const uint8_t bad[][4] = {
        { 0x0a, 0x00, 0x00, 0x00 },  // minutes digit
        { 0xa0, 0x00, 0x00, 0x00 },
        { 0x00, 0x0a, 0x00, 0x00 },  // seconds digit
        { 0x00, 0x60, 0x00, 0x00 },  // seconds above 59
        { 0x00, 0x99, 0x00, 0x00 },
        { 0x01, 0x00, 0x10, 0x00 },  // milliseconds most significant digit
        { 0x01, 0x00, 0x0a, 0x00 },  // milliseconds digits
        { 0x01, 0x00, 0x00, 0x0a },
        { 0x01, 0x00, 0x00, 0xa0 }
    };

    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
    {
        Buffer data = makeHeader(5, FLAG_SONGLENGTHS, 0, 0, 1);
        appendLengths(data, &bad[i], 1);

        SidTune tune(&data[0], data.size());
        CHECK(tune.getStatus());
        CHECK_EQUAL(0u, tune.getInfo()->songLength(1));
        CHECK_EQUAL(4u, tune.getInfo()->c64dataLen());
    }
}

/*
 * 59 seconds and 999 milliseconds are valid.
 */
TEST(TestSongLengthsLimits)
{
    Buffer data = makeHeader(5, FLAG_SONGLENGTHS, 0, 0, 1);
    const uint8_t table[1][4] = { { 0x00, 0x59, 0x09, 0x99 } };
    appendLengths(data, table, 1);

    SidTune tune(&data[0], data.size());
    CHECK_EQUAL(59999u, tune.getInfo()->songLength(1));
}

/*
 * The old MD5 skips the table, the new MD5 covers the whole file.
 */
TEST(TestSongLengthsMd5)
{
    Buffer plain = makeHeader(5, 0, 0, 0, 2);
    Buffer withTable = makeHeader(5, FLAG_SONGLENGTHS, 0, 0, 2);
    const uint8_t table[2][4] = {
        { 0x00, 0x30, 0x00, 0x00 },
        { 0x01, 0x00, 0x05, 0x00 }
    };
    appendLengths(withTable, table, 2);

    SidTune plainTune(&plain[0], plain.size());
    SidTune tableTune(&withTable[0], withTable.size());

    char md5Plain[SidTune::MD5_LENGTH + 1];
    char md5Table[SidTune::MD5_LENGTH + 1];

    plainTune.createMD5(md5Plain);
    tableTune.createMD5(md5Table);
    CHECK_STR(md5Plain, md5Table);

    plainTune.createMD5New(md5Plain);
    tableTune.createMD5New(md5Table);
    CHECK(std::strcmp(md5Plain, md5Table) != 0);
}

/*
 * A table that does not fit is invalid.
 */
TEST(TestSongLengthsTruncated)
{
    // No table at all
    CHECK_STR(ERR_TRUNCATED, status(makeHeader(5, FLAG_SONGLENGTHS, 0, 0, 2)));

    // Two entries for four songs
    Buffer data = makeHeader(5, FLAG_SONGLENGTHS, 0, 0, 4);
    const uint8_t table[2][4] = { { 0x00, 0x30, 0x00, 0x00 }, { 0x00, 0x30, 0x00, 0x00 } };
    appendLengths(data, table, 2);
    CHECK_STR(ERR_TRUNCATED, status(data));
}

/*
 * The table is never part of the C64 data: a file that is a header
 * plus a table only has no C64 data.
 */
TEST(TestSongLengthsNoData)
{
    Buffer data = makeHeader(5, FLAG_SONGLENGTHS, 0, 0, 1);
    data.resize(HEADERSIZE);
    const uint8_t table[1][4] = { { 0x00, 0x30, 0x00, 0x00 } };
    appendLengths(data, table, 1);

    CHECK_STR("SIDTUNE ERROR: No data to load", status(data));
}

/*
 * Without the flag the bytes after the data are C64 data.
 */
TEST(TestSongLengthsFlagClear)
{
    Buffer data = makeHeader(5, 0, 0, 0, 1);
    const uint8_t table[1][4] = { { 0x00, 0x30, 0x00, 0x00 } };
    appendLengths(data, table, 1);

    SidTune tune(&data[0], data.size());
    CHECK_EQUAL(8u, tune.getInfo()->c64dataLen());
    CHECK_EQUAL(0u, tune.getInfo()->songLength(1));
}

/*
 * The table of 256 songs fits in a buffer.
 */
TEST(TestSongLengthsMaxSongs)
{
    Buffer data = makeHeader(5, FLAG_SONGLENGTHS, 0, 0, 256);
    for (int i = 0; i < 256; i++)
    {
        const uint8_t entry[1][4] = { { 0x00, 0x01, 0x00, 0x00 } };
        appendLengths(data, entry, 1);
    }

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_EQUAL(256u, tune.getInfo()->songs());
    CHECK_EQUAL(1000u, tune.getInfo()->songLength(256));
    CHECK_EQUAL(4u, tune.getInfo()->c64dataLen());
}

}

SUITE(PSIDv4Unchanged)
{

/*
 * Version 4 ignores the bits and fields that only exist in version 5.
 */
TEST(TestV4IgnoresV5Fields)
{
    // Bits 6-9 are the models of SID #2 and #3, the song length
    // and FM OPL bits are not defined.
    Buffer data = makeHeader(4, FLAG_SONGLENGTHS | FLAG_FMOPL | (1 << 4) | (2 << 6) | (1 << 8), 0x42, 0x50);
    const uint8_t table[1][4] = { { 0x00, 0x30, 0x00, 0x00 } };
    appendLengths(data, table, 1);

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());

    const SidTuneInfo *info = tune.getInfo();
    CHECK_EQUAL(3, info->sidChips());
    CHECK_STR("d400 d420 d500", addresses(info));
    CHECK_EQUAL(SidTuneInfo::SIDMODEL_6581, info->sidModel(0));
    CHECK_EQUAL(SidTuneInfo::SIDMODEL_8580, info->sidModel(1));
    CHECK_EQUAL(SidTuneInfo::SIDMODEL_6581, info->sidModel(2));
    CHECK_EQUAL(8u, info->c64dataLen());
    CHECK_EQUAL(0u, info->songLength(1));
    CHECK(!info->hasPanning());
    CHECK(!info->fmOpl());
    CHECK_EQUAL(SidTuneInfo::PANLAYOUT_STANDARD, info->panLayout());
    CHECK_EQUAL(SidTuneInfo::PANMODE_DIRECT, info->panMode());
    CHECK_EQUAL(SidTuneInfo::PAN_CENTER, info->sidPanning(0));
}

/*
 * Version 4 keeps the fixed size string fields and does not parse
 * the variable length layout.
 */
TEST(TestV4FixedStrings)
{
    Buffer data = makeHeader(4, 0, 0, 0);
    setMetadata(data, std::string(40, 'n'), "Author", "Released");

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_STR(std::string(32, 'n').c_str(), tune.getInfo()->infoString(0));
    CHECK_STR(std::string(8, 'n').c_str(), tune.getInfo()->infoString(1));
}

/*
 * The multi SID bytes of version 4 are addresses, not a configuration.
 */
TEST(TestV4MultiSidBytes)
{
    // 0x23 would be the reserved configuration 2 with 3 SIDs in version 5
    // and an invalid address in version 4
    Buffer data = makeHeader(4, 0, 0x23, 0x00);

    SidTune tune(&data[0], data.size());
    CHECK(tune.getStatus());
    CHECK_EQUAL(1, tune.getInfo()->sidChips());

    // Configuration 7 has no meaning in version 3
    CHECK_STR("ok", status(makeHeader(3, 0, 0x72, 0x00)));
}

/*
 * A version 4 file longer than the limit for a buffer is still rejected.
 */
TEST(TestV4LongBuffer)
{
    Buffer data = makeHeader(4, 0, 0, 0);
    data.resize(65536 + 2 + 0x7C + 1, 0x60);

    CHECK_STR("SIDTUNE ERROR: Input data too long", status(data));
}

}
