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
// STIL Common header
//

#ifndef STILCOMM_H
#define STILCOMM_H

#include <string>
#include <algorithm>

#include "stildefs.h"

/**
* Contains some definitions common to STIL
*/
class stilcomm
{
public:
    /**
    * Converts slashes to the one the OS uses to access files.
    *
    * @param
    *      str - what to convert
    */
    static void convertSlashes(std::string& str) { std::replace(str.begin(), str.end(), '/', SLASH); }

    /**
    * Converts OS specific dir separators to slashes.
    *
    * @param
    *      str - what to convert
    */
    static void convertToSlashes(std::string& str) { std::replace(str.begin(), str.end(), SLASH, '/'); }
};

#endif // STILCOMM_H
