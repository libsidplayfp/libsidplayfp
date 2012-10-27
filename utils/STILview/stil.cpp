/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 1998, 2002 by LaLa <LaLa@C64.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

//
// STIL class - Implementation file
//
// AUTHOR: LaLa
// Email : LaLa@C64.org
// Copyright (C) 1998, 2002 by LaLa
//

#include <iostream>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <cstdio>      // For snprintf() and NULL
#include <sstream>

using namespace std;

#include "stil.h"

#define STILopenFlags (ios::in | ios::binary)

const float VERSION_NO = 2.17;

#define CERR_STIL_DEBUG if (STIL_DEBUG) cerr << "Line #" << __LINE__ << " STIL::"

// These are the hardcoded STIL/BUG field names.
const char    _NAME_STR[]="   NAME: ";
const char  _AUTHOR_STR[]=" AUTHOR: ";
const char   _TITLE_STR[]="  TITLE: ";
const char  _ARTIST_STR[]=" ARTIST: ";
const char _COMMENT_STR[]="COMMENT: ";
const char     _BUG_STR[]="BUG: ";

const char *STIL::STIL_ERROR_STR[] = {
    "No error.",
    "Failed to open BUGlist.txt.",
    "Base dir path is not the HVSC base dir path.",
    "The entry was not found in STIL.txt.",
    "The entry was not found in BUGlist.txt.",
    "A section-global comment was asked for in the wrong way.",
    "",
    "",
    "",
    "",
    "CRITICAL ERROR",
    "Incorrect HVSC base dir length!",
    "Failed to open STIL.txt!",
    "Failed to determine EOL from STIL.txt!",
    "No STIL sections were found in STIL.txt!",
    "No STIL sections were found in BUGlist.txt!"
};

// CONSTRUCTOR
STIL::STIL(const char* stilPath, const char* bugsPath) :
  PATH_TO_STIL(stilPath),
  PATH_TO_BUGLIST(bugsPath)
{
    setVersionString();

    STILVersion = 0.0;
    memset((void *)entrybuf, 0, sizeof(entrybuf));
    memset((void *)globalbuf, 0, sizeof(globalbuf));
    memset((void *)bugbuf, 0, sizeof(bugbuf));
    memset((void *)resultEntry, 0, sizeof(resultEntry));
    memset((void *)resultBug, 0, sizeof(resultBug));

    STIL_EOL = '\n';
    STIL_EOL2 = '\0';

    STIL_DEBUG = false;
    lastError = NO_STIL_ERROR;
}

void STIL::setVersionString()
{
    ostringstream ss;
    ss << fixed << setw(4) << setprecision(2);
    ss << "STILView v" << VERSION_NO << ", (C) 1998, 2002 by LaLa (LaLa@C64.org)" << endl;
    versionString = ss.str();
}

const char *
STIL::getVersion()
{
    lastError = NO_STIL_ERROR;
    return versionString.c_str();
}

float
STIL::getVersionNo()
{
    lastError = NO_STIL_ERROR;
    return VERSION_NO;
}

float
STIL::getSTILVersionNo()
{
    lastError = NO_STIL_ERROR;
    return STILVersion;
}

bool
STIL::setBaseDir(const char *pathToHVSC)
{
    // Temporary placeholder for STIL.txt's version number.
    float tempSTILVersion = STILVersion;

    // Temporary placeholders for lists of sections.
    std::map<std::string, std::streampos> tempStilDirs;
    std::map<std::string, std::streampos> tempBugDirs;

    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "setBaseDir() called, pathToHVSC=" << pathToHVSC << endl;

    string tempBaseDir(pathToHVSC);

    // Sanity check the length.
    if (tempBaseDir.empty()) {
        CERR_STIL_DEBUG << "setBaseDir() has problem with the size of pathToHVSC" << endl;
        lastError = BASE_DIR_LENGTH;
        return false;
    }

    // Chop the trailing slash
    const std::string::iterator lastChar = tempBaseDir.end() - 1;
    if (*lastChar == SLASH) {
        tempBaseDir.erase(lastChar);
    }

    // Attempt to open STIL

    // Create the full path+filename
    string tempName = tempBaseDir;
    tempName.append(PATH_TO_STIL);
    convertSlashes(tempName);

    stilFile.clear();
    stilFile.open(tempName.c_str(), STILopenFlags);

    if (stilFile.fail()) {
        stilFile.close();
        CERR_STIL_DEBUG << "setBaseDir() open failed for " << tempName << endl;
        lastError = STIL_OPEN;
        return false;
    }

    CERR_STIL_DEBUG << "setBaseDir(): open succeeded for " << tempName << endl;

    // Attempt to open BUGlist

    // Create the full path+filename
    tempName = tempBaseDir;
    tempName.append(PATH_TO_BUGLIST);
    convertSlashes(tempName);

    bugFile.clear();
    bugFile.open(tempName.c_str(), STILopenFlags);

    if (bugFile.fail()) {

        // This is not a critical error - some earlier versions of HVSC did
        // not have a BUGlist.txt file at all.

        bugFile.close();
        CERR_STIL_DEBUG << "setBaseDir() open failed for " << tempName << endl;
        lastError = BUG_OPEN;
    }
    else
    {
        CERR_STIL_DEBUG << "setBaseDir(): open succeeded for " << tempName << endl;
    }

    // Find out what the EOL really is
    if (determineEOL() != true) {
        stilFile.close();
        bugFile.close();
        CERR_STIL_DEBUG << "determinEOL() failed" << endl;
        lastError = NO_EOL;
        return false;
    }

    // Save away the current string so we can restore it if needed.
    string tempVersionString(versionString);

    setVersionString();

    // This is necessary so the version number gets scanned in from the new
    // file, too.
    STILVersion = 0.0;

    // These will populate the tempStilDirs and tempBugDirs arrays (or not :)

    if (getDirs(stilFile, tempStilDirs, true) != true) {
        stilFile.close();
        bugFile.close();
        CERR_STIL_DEBUG << "getDirs() failed for stilFile" << endl;
        lastError = NO_STIL_DIRS;

        // Clean up and restore things.
        STILVersion = tempSTILVersion;
        versionString = tempVersionString;
        return false;
    }

    if (bugFile.good()) {
        if (getDirs(bugFile, tempBugDirs, false) != true) {

            // This is not a critical error - it is possible that the
            // BUGlist.txt file has no entries in it at all (in fact, that's
            // good!).

            CERR_STIL_DEBUG << "getDirs() failed for bugFile" << endl;
            lastError = BUG_OPEN;
        }
    }

    stilFile.close();
    bugFile.close();

    // Now we can copy the stuff into private data.
    // NOTE: At this point, STILVersion and the versionString should contain
    // the new info!

    // Copy.
    baseDir = tempBaseDir;

    // Now proceed with copy.

    stilDirs = tempStilDirs;
    bugDirs = tempBugDirs;

    // Clear the buffers (caches).
    memset((void *)entrybuf, 0, sizeof(entrybuf));
    memset((void *)globalbuf, 0, sizeof(globalbuf));
    memset((void *)bugbuf, 0, sizeof(bugbuf));

    CERR_STIL_DEBUG << "setBaseDir() succeeded" << endl;

    return true;
}

char *
STIL::getAbsEntry(const char *absPathToEntry, int tuneNo, STILField field)
{
    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getAbsEntry() called, absPathToEntry=" << absPathToEntry << endl;

    if (baseDir.empty()) {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = STIL_OPEN;
        return NULL;
    }

    // Determine if the baseDir is in the given pathname.

    if (MYSTRNICMP(absPathToEntry, baseDir.c_str(), baseDir.size()) != 0) {
        CERR_STIL_DEBUG << "getAbsEntry() failed: baseDir=" << baseDir << ", absPath=" << absPathToEntry << endl;
        lastError = WRONG_DIR;
        return NULL;
    }


    string tempDir(absPathToEntry+baseDir.size());
    convertToSlashes(tempDir);

    return getEntry(tempDir.c_str(), tuneNo, field);
}

char *
STIL::getEntry(const char *relPathToEntry, int tuneNo, STILField field)
{
    char *tempName;
    size_t tempNameLength;

    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getEntry() called, relPath=" << relPathToEntry << ", rest=" << tuneNo << "," << field << endl;

    if (baseDir.empty()) {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = STIL_OPEN;
        return NULL;
    }

    // Fail if a section-global comment was asked for.

    if (*(relPathToEntry+strlen(relPathToEntry)-1) == '/') {
        CERR_STIL_DEBUG << "getEntry() section-global comment was asked for - failed" << endl;
        lastError = WRONG_ENTRY;
        return NULL;
    }

    if (STILVersion < 2.59) {

        // Older version of STIL is detected.

        tuneNo = 0;
        field = all;
    }

    // Find out whether we have this entry in the buffer.

    if ((MYSTRNICMP(entrybuf, relPathToEntry, strlen(relPathToEntry)) != 0) ||
        ((( (size_t) (strchr(entrybuf, '\n')-entrybuf)) != strlen(relPathToEntry))
            && (STILVersion > 2.59))) {

        // The relative pathnames don't match or they're not the same length:
        // we don't have it in the buffer, so pull it in.

        CERR_STIL_DEBUG << "getEntry(): entry not in buffer" << endl;

        // Create the full path+filename
        string tempName(baseDir);
        tempName.append(PATH_TO_STIL);
        convertSlashes(tempName);

        stilFile.clear();
        stilFile.open(tempName.c_str(), STILopenFlags);

        if (stilFile.fail()) {
            stilFile.close();
            CERR_STIL_DEBUG << "getEntry() open failed for stilFile" << endl;
            lastError = STIL_OPEN;
            return NULL;
        }

        CERR_STIL_DEBUG << "getEntry() open succeeded for stilFile" << endl;

        if (positionToEntry(relPathToEntry, stilFile, stilDirs) == false) {
            // Copy the entry's name to the buffer.
            strncpy(entrybuf, relPathToEntry, STIL_MAX_ENTRY_SIZE-1);
            strncat(entrybuf, "\n", 2);
            entrybuf[STIL_MAX_ENTRY_SIZE-1] = '\0';
            CERR_STIL_DEBUG << "getEntry() posToEntry() failed" << endl;
            lastError = NOT_IN_STIL;
        }
        else {
            *entrybuf= '\0';
            readEntry(stilFile, entrybuf);
            CERR_STIL_DEBUG << "getEntry() entry read" << endl;
        }

        stilFile.close();
    }

    // Put the requested field into the result string.

    if (getField(resultEntry, entrybuf, tuneNo, field) != true) {
        return NULL;
    }
    else {
        return resultEntry;
    }
}

char *
STIL::getAbsBug(const char *absPathToEntry, int tuneNo)
{
    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getAbsBug() called, absPathToEntry=" << absPathToEntry << endl;

    if (baseDir.empty()) {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = BUG_OPEN;
        return NULL;
    }

    // Determine if the baseDir is in the given pathname.

    if (MYSTRNICMP(absPathToEntry, baseDir.c_str(), baseDir.size()) != 0) {
        CERR_STIL_DEBUG << "getAbsBug() failed: baseDir=" << baseDir << ", absPath=" << absPathToEntry << endl;
        lastError = WRONG_DIR;
        return NULL;
    }

    string tempDir(absPathToEntry+baseDir.size());
    convertToSlashes(tempDir);

    return getBug(tempDir.c_str(), tuneNo);
}

char *
STIL::getBug(const char *relPathToEntry, int tuneNo)
{
    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getBug() called, relPath=" << relPathToEntry << ", rest=" << tuneNo << endl;

    if (baseDir.empty()) {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = BUG_OPEN;
        return NULL;
    }

    // Older version of STIL is detected.

    if (STILVersion < 2.59) {
        tuneNo = 0;
    }

    // Find out whether we have this bug entry in the buffer.
    // If the baseDir was changed, we'll have to read it in again,
    // even if it might be in the buffer already.

    if ((MYSTRNICMP(bugbuf, relPathToEntry, strlen(relPathToEntry)) != 0) ||
        ((( (size_t) (strchr(bugbuf, '\n')-bugbuf)) != strlen(relPathToEntry)) &&
            (STILVersion > 2.59))) {

        // The relative pathnames don't match or they're not the same length:
        // we don't have it in the buffer, so pull it in.

        CERR_STIL_DEBUG << "getBug(): entry not in buffer" << endl;

        // Create the full path+filename
        string tempName(baseDir);
        tempName.append(PATH_TO_BUGLIST);
        convertSlashes(tempName);

        bugFile.clear();
        bugFile.open(tempName.c_str(), STILopenFlags);

        if (bugFile.fail()) {
            bugFile.close();
            CERR_STIL_DEBUG << "getBug() open failed for bugFile" << endl;
            lastError = BUG_OPEN;
            return NULL;
        }

        CERR_STIL_DEBUG << "getBug() open succeeded for bugFile" << endl;

        if (positionToEntry(relPathToEntry, bugFile, bugDirs) == false) {
            // Copy the entry's name to the buffer.
            strncpy(bugbuf, relPathToEntry, STIL_MAX_ENTRY_SIZE-1);
            strncat(bugbuf, "\n", 2);
            bugbuf[STIL_MAX_ENTRY_SIZE-1] = '\0';
            CERR_STIL_DEBUG << "getBug() posToEntry() failed" << endl;
            lastError = NOT_IN_BUG;
        }
        else {
            *bugbuf = '\0';
            readEntry(bugFile, bugbuf);
            CERR_STIL_DEBUG << "getBug() entry read" << endl;
        }

        bugFile.close();
    }

    // Put the requested field into the result string.

    if (getField(resultBug, bugbuf, tuneNo) != true) {
        return NULL;
    }
    else {
        return resultBug;
    }
}

char *
STIL::getAbsGlobalComment(const char *absPathToEntry)
{
    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getAbsGC() called, absPathToEntry=" << absPathToEntry << endl;

    if (baseDir.empty()) {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = STIL_OPEN;
        return NULL;
    }

    // Determine if the baseDir is in the given pathname.

    if (MYSTRNICMP(absPathToEntry, baseDir.c_str(), baseDir.size()) != 0) {
        CERR_STIL_DEBUG << "getAbsGC() failed: baseDir=" << baseDir << ", absPath=" << absPathToEntry << endl;
        lastError = WRONG_DIR;
        return NULL;
    }

    string tempDir(absPathToEntry+baseDir.size());
    convertToSlashes(tempDir);

    return getGlobalComment(tempDir.c_str());
}

char *
STIL::getGlobalComment(const char *relPathToEntry)
{
    size_t pathLen;
    char *temp;
    char *lastSlash;

    lastError = NO_STIL_ERROR;

    CERR_STIL_DEBUG << "getGC() called, relPath=" << relPathToEntry << endl;

    if (baseDir.empty()) {
        CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
        lastError = STIL_OPEN;
        return NULL;
    }

    // Save the dirpath.

    lastSlash = (char *)strrchr(relPathToEntry, '/');

    if (lastSlash == NULL) {
        lastError = WRONG_DIR;
        return NULL;
    }

    pathLen = lastSlash-relPathToEntry+1;
    string dir(relPathToEntry, pathLen);

    // Find out whether we have this global comment in the buffer.
    // If the baseDir was changed, we'll have to read it in again,
    // even if it might be in the buffer already.

    if ((MYSTRNICMP(globalbuf, dir.c_str(), pathLen) != 0) ||
        ((( (size_t) (strchr(globalbuf, '\n')-globalbuf)) != pathLen) &&
            (STILVersion > 2.59))) {

        // The relative pathnames don't match or they're not the same length:
        // we don't have it in the buffer, so pull it in.

        CERR_STIL_DEBUG << "getGC(): entry not in buffer" << endl;

        // Create the full path+filename
        string tempName(baseDir);
        tempName.append(PATH_TO_STIL);
        convertSlashes(tempName);

        stilFile.clear();
        stilFile.open(tempName.c_str(), STILopenFlags);

        if (stilFile.fail()) {
            stilFile.close();
            CERR_STIL_DEBUG << "getGC() open failed for stilFile" << endl;
            lastError = STIL_OPEN;
            return NULL;
        }

        if (positionToEntry(dir.c_str(), stilFile, stilDirs) == false) {
            // Copy the dirname to the buffer.
            strncpy(globalbuf, dir.c_str(), STIL_MAX_ENTRY_SIZE-1);
            strncat(globalbuf, "\n", 2);
            globalbuf[STIL_MAX_ENTRY_SIZE-1] = '\0';
            CERR_STIL_DEBUG << "getGC() posToEntry() failed" << endl;
            lastError = NOT_IN_STIL;
        }
        else {
            *globalbuf = '\0';
            readEntry(stilFile, globalbuf);
            CERR_STIL_DEBUG << "getGC() entry read" << endl;
        }

        stilFile.close();
    }

    CERR_STIL_DEBUG << "getGC() globalbuf=" << globalbuf << endl;
    CERR_STIL_DEBUG << "-=END=-" << endl;

    // Position pointer to the global comment field.

    temp = strchr(globalbuf, '\n');
    temp++;

    // Check whether this is a NULL entry or not.

    if (*temp == '\0') {
        return NULL;
    }
    else {
        return temp;
    }
}

//////// PRIVATE

bool
STIL::determineEOL()
{
    char line[STIL_MAX_LINE_SIZE+5];
    int i=0;

    CERR_STIL_DEBUG << "detEOL() called" << endl;

    if (stilFile.fail()) {
        CERR_STIL_DEBUG << "detEOL() open failed" << endl;
        return false;
    }

    stilFile.seekg(0);

    // Read in the first line from stilFile to determine what the
    // EOL character is (it can be different from OS to OS).

    stilFile.read(line, sizeof(line)-1);
    line[sizeof(line)-1] = '\0';

    CERR_STIL_DEBUG << "detEOL() line=" << line << endl;

    // Now find out what the EOL char is (or are).

    STIL_EOL = '\0';
    STIL_EOL2 = '\0';

    while (line[i] != '\0') {
        if ((line[i] == 0x0d) || (line[i] == 0x0a)) {
            if (STIL_EOL == '\0') {
                STIL_EOL = line[i];
            }
            else {
                if (line[i] != STIL_EOL) {
                    STIL_EOL2 = line[i];
                }
            }
        }
        i++;
    }

    if (STIL_EOL == '\0') {
        // Something is wrong - no EOL-like char was found.
        CERR_STIL_DEBUG << "detEOL() no EOL found" << endl;
        return false;
    }

    CERR_STIL_DEBUG << "detEOL() EOL1=0x" << hex << (int) STIL_EOL << " EOL2=0x" << hex << (int) STIL_EOL2 << dec << endl;

    return true;
}

bool
STIL::getDirs(ifstream& inFile, dirList &dirs, bool isSTILFile)
{
    char line[STIL_MAX_LINE_SIZE];
    size_t j;
    bool newDir;

    if (isSTILFile) {
        newDir = false;
    }
    else {
        newDir = true;
    }

    CERR_STIL_DEBUG << "getDirs() called" << endl;

    inFile.seekg(0);

    while (inFile.good()) {
        getStilLine(inFile, line);

        if (!isSTILFile) CERR_STIL_DEBUG << line << '\n';

        // Try to extract STIL's version number if it's not done, yet.

        if (isSTILFile && (STILVersion == 0.0)) {
            if (strncmp(line, "#  STIL v", 9) == 0) {

                // Get the version number
                STILVersion = atof(line+9);

                // Put it into the string, too.
                snprintf(line, STIL_MAX_LINE_SIZE-1, "SID Tune Information List (STIL) v%4.2f\n", STILVersion);
                line[STIL_MAX_LINE_SIZE-1] = '\0';
                versionString.append(line);

                CERR_STIL_DEBUG << "getDirs() STILVersion=" << STILVersion << endl;

                continue;
            }
        }

        // Search for the start of a dir separator first.

        if (isSTILFile && !newDir && (MYSTRNICMP(line, "### ", 4) == 0)) {
            newDir = true;
            continue;
        }

        // Is this the start of an entry immediately following a dir separator?

        if (newDir && (*line == '/')) {

            // Get the directory only
            j = strrchr(line,'/')-line+1;

            string dirName(line, j);

            if (!isSTILFile) {
                // Compare it to the stored dirnames
                if (dirs.count(dirName) == 0) {
                    newDir = true;
                }
                else {
                    newDir = false;
                }
            }

            // Store the info
            if (newDir) {

                streampos position = inFile.tellg()-(streampos)strlen(line)-1L;

                CERR_STIL_DEBUG << "getDirs() dirName=" << dirName << ", pos=" << position <<  endl;

                dirs.insert(pair<string, streampos>(dirName, position));
            }

            if (isSTILFile) {
                newDir = false;
            }
            else {
                newDir = true;
            }
        }
    }

    if (dirs.empty()) {
        // No entries found - something is wrong.
        // NOTE: It's perfectly valid to have a BUGlist.txt file with no
        // entries in it!
        CERR_STIL_DEBUG << "getDirs() no dirs found" << endl;
        return false;
    }

    CERR_STIL_DEBUG << "getDirs() successful" << endl;

    return true;
}

bool
STIL::positionToEntry(const char *entryStr, ifstream& inFile, dirList &dirs)
{
    size_t pathLen;
    size_t entryStrLen;
    char line[STIL_MAX_LINE_SIZE];
    int temp;
    bool globComm = false;
    char *chrptr;

    CERR_STIL_DEBUG << "pos2Entry() called, entryStr=" << entryStr << endl;

    inFile.seekg(0);

    // Get the dirpath.

    chrptr = strrchr((char *)entryStr, '/');

    // If no slash was found, something is screwed up in the entryStr.

    if (chrptr == NULL) {
        return false;
    }

    pathLen = chrptr-entryStr+1;

    // Determine whether a section-global comment is asked for.

    entryStrLen = strlen(entryStr);
    if (pathLen == entryStrLen) {
        globComm = true;
    }

    // Find it in the table.
    string entry(entryStr, pathLen);
    dirList::iterator elem = dirs.find(entry);

    if (elem == dirs.end()) {
        // The directory was not found.
        CERR_STIL_DEBUG << "pos2Entry() did not find the dir" << endl;
        return false;
    }

    // Jump to the first entry of this section.
    inFile.seekg(elem->second);
    bool foundIt = false;

    // Now find the desired entry

    do {
        getStilLine(inFile, line);
        if (inFile.eof()) {
            break;
        }

        // Check if it is the start of an entry

        if (*line == '/') {

            if (MYSTRNICMP(elem->first.c_str(), line, pathLen) != 0) {
                // We are outside the section - get out of the loop,
                // which will fail the search.
                break;
            }

            // Check whether we need to find a section-global comment or
            // a specific entry.

            if (globComm || (STILVersion > 2.59)) {
                temp = MYSTRICMP(line, entryStr);
            }
            else {
                // To be compatible with older versions of STIL, which may have
                // the tune designation on the first line of a STIL entry
                // together with the pathname.
                temp = MYSTRNICMP(line, entryStr, entryStrLen);
            }

            CERR_STIL_DEBUG << "pos2Entry() line=" << line << endl;

            if (temp == 0) {
                // Found it!
                foundIt = true;
            }

        }
    } while (!foundIt);

    if (foundIt) {
        // Reposition the file pointer back to the start of the entry.
        inFile.seekg(inFile.tellg()-(streampos)strlen(line)-1L);
        CERR_STIL_DEBUG << "pos2Entry() entry found" << endl;
        return true;
    }
    else {
        CERR_STIL_DEBUG << "pos2Entry() entry not found" << endl;
        return false;
    }
}

void
STIL::readEntry(ifstream& inFile, char *buffer)
{
    char line[STIL_MAX_LINE_SIZE];

    do {
        getStilLine(inFile, line);
        strcat(buffer, line);
        if (*line != '\0') {
            strncat(buffer, "\n", 2);
        }
    } while (*line != '\0');
}

bool
STIL::getField(char *result, char *buffer, int tuneNo, STILField field)
{
    char *start, *firstTuneNo, *temp, *temp2 = NULL;

    CERR_STIL_DEBUG << "getField() called, buffer=" << buffer << ", rest=" << tuneNo << "," << field << endl;

    // Clean out the result buffer first.
    *result = '\0';

    // Position pointer to the first char beyond the file designation.

    start = strchr(buffer, '\n');
    start++;

    // Check whether this is a NULL entry or not.

    if (*start == '\0') {
        CERR_STIL_DEBUG << "getField() null entry" << endl;
        return false;
    }

    // Is this a multitune entry?
    firstTuneNo = strstr(start, "(#");

    // This is a tune designation only if the previous char was
    // a newline (ie. if the "(#" is on the beginning of a line).
    if ((firstTuneNo != NULL) && (*(firstTuneNo-1) != '\n')) {
        firstTuneNo = NULL;
    }

    if (firstTuneNo == NULL) {

        //-------------------//
        // SINGLE TUNE ENTRY //
        //-------------------//

        // Is the first thing in this STIL entry the COMMENT?

        temp = strstr(start, _COMMENT_STR);

        // Search for other potential fields beyond the COMMENT.
        if (temp == start) {
            temp2 = strstr(start, _NAME_STR);
            if (temp2 == NULL) {
                temp2 = strstr(start, _AUTHOR_STR);
                if (temp2 == NULL) {
                    temp2 = strstr(start, _TITLE_STR);
                    if (temp2 == NULL) {
                        temp2 = strstr(start, _ARTIST_STR);
                    }
                }
            }
        }

        if (temp == start) {

            // Yes. So it's assumed to be a file-global comment.

            CERR_STIL_DEBUG << "getField() single-tune entry, COMMENT only" << endl;

            if ((tuneNo == 0) && ((field == all) || ((field == comment) && (temp2 == NULL))) ) {

                // Simply copy the stuff in.

                strncpy(result, start, STIL_MAX_ENTRY_SIZE-1);
                result[STIL_MAX_ENTRY_SIZE-1] = '\0';
                CERR_STIL_DEBUG << "getField() copied to resultbuf" << endl;
                return true;
            }

            else if ((tuneNo == 0) && (field == comment)) {

                // Copy just the comment.

                strncpy(result, start, temp2-start);
                *(result+(temp2-start)) = '\0';
                CERR_STIL_DEBUG << "getField() copied to just the COMMENT to resultbuf" << endl;
                return true;
            }

            else if ((tuneNo == 1) && (temp2 != NULL)) {

               // A specific field was asked for.

                CERR_STIL_DEBUG << "getField() copying COMMENT to resultbuf" << endl;
                return getOneField(result, temp2, temp2+strlen(temp2), field);
            }

            else {

                // Anything else is invalid as of v2.00.

                CERR_STIL_DEBUG << "getField() invalid parameter combo: single tune, tuneNo=" << tuneNo << ", field=" << field << endl;
                return false;
            }
        }
        else {

            // No. Handle it as a regular entry.

            CERR_STIL_DEBUG << "getField() single-tune regular entry" << endl;

            if ((field == all) && ((tuneNo == 0) || (tuneNo == 1))) {

                // The complete entry was asked for. Simply copy the stuff in.

                strncpy(result, start, STIL_MAX_ENTRY_SIZE-1);
                result[STIL_MAX_ENTRY_SIZE-1] = '\0';
                CERR_STIL_DEBUG << "getField() copied to resultbuf" << endl;
                return true;
            }

            else if (tuneNo == 1) {

               // A specific field was asked for.

                CERR_STIL_DEBUG << "getField() copying COMMENT to resultbuf" << endl;
                return getOneField(result, start, start+strlen(start), field);
            }

            else {

                // Anything else is invalid as of v2.00.

                CERR_STIL_DEBUG << "getField() invalid parameter combo: single tune, tuneNo=" << tuneNo << ", field=" << field << endl;
                return false;
            }
        }
    }
    else {

        //-------------------//
        // MULTITUNE ENTRY
        //-------------------//

        CERR_STIL_DEBUG << "getField() multitune entry" << endl;

        // Was the complete entry asked for?

        if (tuneNo == 0) {

            switch (field) {

                case all:

                    // Yes. Simply copy the stuff in.

                    strncpy(result, start, STIL_MAX_ENTRY_SIZE-1);
                    result[STIL_MAX_ENTRY_SIZE-1] = '\0';
                    CERR_STIL_DEBUG << "getField() copied all to resultbuf" << endl;
                    return true;
                    break;

                case comment:

                    // Only the file-global comment field was asked for.

                    if (firstTuneNo != start) {
                        CERR_STIL_DEBUG << "getField() copying file-global comment to resultbuf" << endl;
                        return getOneField(result, start, firstTuneNo, comment);
                    }
                    else {
                        CERR_STIL_DEBUG << "getField() no file-global comment" << endl;
                        return false;
                    }
                    break;

                default:

                    // If a specific field other than a comment is
                    // asked for tuneNo=0, this is illegal.

                    CERR_STIL_DEBUG << "getField() invalid parameter combo: multitune, tuneNo=" << tuneNo << ", field=" << field << endl;
                    return false;
                    break;
            }
        }

        char *myTuneNo, *nextTuneNo;
        char tuneNoStr[8];

        // Search for the requested tune number.

        snprintf(tuneNoStr, 7, "(#%d)", tuneNo);
        tuneNoStr[7] = '\0';
        myTuneNo = strstr(start, tuneNoStr);

        if (myTuneNo != NULL) {

            // We found the requested tune number.
            // Set the pointer beyond it.
            myTuneNo = strchr(myTuneNo, '\n') + 1;

            // Where is the next one?

            nextTuneNo = strstr(myTuneNo, "\n(#");
            if (nextTuneNo == NULL) {
                // There is no next one - set pointer to end of entry.
                nextTuneNo = start+strlen(start);
            }
            else {
                // The search included the \n - go beyond it.
                nextTuneNo++;
            }

            // Put the desired fields into the result (which may be 'all').

            CERR_STIL_DEBUG << "getField() myTuneNo=" << myTuneNo << ", nextTuneNo=" << nextTuneNo << endl;
            return getOneField(result+strlen(result), myTuneNo, nextTuneNo, field);
        }

        else {
            CERR_STIL_DEBUG << "getField() nothing found" << endl;
            return false;
        }
    }
}

bool
STIL::getOneField(char *result, char *start, char *end, STILField field)
{
    char *temp = NULL;

    // Sanity checking

    if ((end < start) || (*(end-1) != '\n')) {
        *result = '\0';
        CERR_STIL_DEBUG << "getOneField() illegal parameters" << endl;
        return false;
    }

    CERR_STIL_DEBUG << "getOneField() called, start=" << start << ", rest=" << field << endl;

    switch (field) {

        case all:

            strncat(result, start, end-start);
            return true;
            break;

        case name:

            temp = strstr(start, _NAME_STR);
            break;

        case author:

            temp = strstr(start, _AUTHOR_STR);
            break;

        case title:

            temp = strstr(start, _TITLE_STR);
            break;

        case artist:

            temp = strstr(start, _ARTIST_STR);
            break;

        case comment:

            temp = strstr(start, _COMMENT_STR);
            break;

        default:

            break;
    }

    // If the field was not found or it is not in between 'start'
    // and 'end', it is declared a failure.

    if ((temp == NULL) || (temp < start) || (temp > end)) {
        *result = '\0';
        return false;
    }

    // Search for the end of this field. This is done by finding
    // where the next field starts.

    char *nextName, *nextAuthor, *nextTitle, *nextArtist, *nextComment, *nextField;

    nextName = strstr(temp+1, _NAME_STR);
    nextAuthor = strstr(temp+1, _AUTHOR_STR);
    nextTitle = strstr(temp+1, _TITLE_STR);
    nextArtist = strstr(temp+1, _ARTIST_STR);
    nextComment = strstr(temp+1, _COMMENT_STR);

    // If any of these fields is beyond 'end', they are ignored.

    if ((nextName != NULL) && (nextName >= end)) {
        nextName = NULL;
    }

    if ((nextAuthor != NULL) && (nextAuthor >= end)) {
        nextAuthor = NULL;
    }

    if ((nextTitle != NULL) && (nextTitle >= end)) {
        nextTitle = NULL;
    }

    if ((nextArtist != NULL) && (nextArtist >= end)) {
        nextArtist = NULL;
    }

    if ((nextComment != NULL) && (nextComment >= end)) {
        nextComment = NULL;
    }

    // Now determine which one is the closest to our field - that one
    // will mark the end of the required field.

    nextField = nextName;

    if (nextField == NULL) {
        nextField = nextAuthor;
    }
    else if ((nextAuthor != NULL) && (nextAuthor < nextField)) {
        nextField = nextAuthor;
    }

    if (nextField == NULL) {
        nextField = nextTitle;
    }
    else if ((nextTitle != NULL) && (nextTitle < nextField)) {
        nextField = nextTitle;
    }

    if (nextField == NULL) {
        nextField = nextArtist;
    }
    else if ((nextArtist != NULL) && (nextArtist < nextField)) {
        nextField = nextArtist;
    }

    if (nextField == NULL) {
        nextField = nextComment;
    }
    else if ((nextComment != NULL) && (nextComment < nextField)) {
        nextField = nextComment;
    }

    if (nextField == NULL) {
        nextField = end;
    }

    // Now nextField points to the last+1 char that should be copied to
    // result. Do that.

    strncat(result, temp, nextField-temp);
    return true;
}

void
STIL::getStilLine(ifstream& infile, char *line)
{
    char temp;

    if (STIL_EOL2 != '\0') {

        // If there was a remaining EOL char from the previous read, eat it up.

        temp = infile.peek();

        if ((temp == 0x0d) || (temp == 0x0a)) {
            infile.get(temp);
        }
    }

    infile.getline(line, STIL_MAX_LINE_SIZE, STIL_EOL);
}
