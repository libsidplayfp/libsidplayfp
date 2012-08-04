 /*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012 Leando Nini <drfiemost@users.sourceforge.net>
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

#include "SidTune.h"

#include "sidtune/SidTuneBase.h"

SidTune::SidTune(const char* fileName, const char **fileNameExt,
            const bool separatorIsSlash) :
    tune(new SidTuneBase(fileName, fileNameExt, separatorIsSlash)) {}

SidTune::SidTune(const uint_least8_t* oneFileFormatSidtune, const uint_least32_t sidtuneLength) :
    tune(new SidTuneBase(oneFileFormatSidtune, sidtuneLength)) {}

SidTune::~SidTune()
{
    delete tune;
}

void SidTune::setFileNameExtensions(const char **fileNameExt)
{
    tune->setFileNameExtensions(fileNameExt);
}

bool SidTune::load(const char* fileName, const bool separatorIsSlash)
{
    return tune->load(fileName, separatorIsSlash);
}

bool SidTune::read(const uint_least8_t* sourceBuffer, const uint_least32_t bufferLen)
{
    return tune->read(sourceBuffer, bufferLen);
}

unsigned int SidTune::selectSong(const unsigned int songNum){
    return tune->selectSong(songNum);
}

const SidTuneInfo* SidTune::getInfo() const
{
    return tune->getInfo();
}

const SidTuneInfo* SidTune::getInfo(const unsigned int songNum)
{
    return tune->getInfo(songNum);
}

bool SidTune::getStatus() const { return tune->getStatus(); }

const char* SidTune::statusString() const { return tune->statusString(); }

bool SidTune::placeSidTuneInC64mem(uint_least8_t* c64buf)
{
    return tune->placeSidTuneInC64mem(c64buf);
}
#if 0
bool SidTune::saveC64dataFile(const char* destFileName, const bool overWriteFlag)
{
    return tune->saveC64dataFile(destFileName, overWriteFlag);
}

bool SidTune::savePSIDfile(const char* destFileName, const bool overWriteFlag)
{
    return tune->savePSIDfile(destFileName, overWriteFlag);
}

//bool SidTune::loadFile(const char* fileName, Buffer_sidtt<const uint_least8_t>& bufferRef);

bool SidTune::saveToOpenFile(std::ofstream& toFile, const uint_least8_t* buffer, uint_least32_t bufLen)
{
    return tune->saveToOpenFile(toFile, buffer, bufLen);
}
#endif
const char* SidTune::createMD5(char *md5)
{
    return tune->createMD5(md5);
}
