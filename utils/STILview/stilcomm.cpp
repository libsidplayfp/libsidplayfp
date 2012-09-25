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
// STIL - Common stuff
//

//
// Common functions used for STIL handling.
// See stilcomm.h for prologs.
//

#ifndef _STILCOMM
#define _STILCOMM

#include "stildefs.h"
#include "stil.h"

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

void convertSlashes(char *str);
void convertToSlashes(char *str);

void convertSlashes(char *str)
{
    while (*str) {
        if (*str == '/') {
            *str = SLASH;
        }
        str++;
    }
}

void convertToSlashes(char *str)
{
    while (*str) {
        if (*str == SLASH) {
            *str = '/' ;
        }
        str++;
    }
}


#endif //_STILCOMM
