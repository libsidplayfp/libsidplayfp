/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2010 Antti Lankila
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

#ifndef DISCONNECTEDBUSBANK_H
#define DISCONNECTEDBUSBANK_H

#include "Bank.h"

/** @internal
* IO1/IO2
*/
class DisconnectedBusBank : public Bank
{
    void write(const uint_least16_t addr, const uint8_t data) {}

    // FIXME this should actually return last byte read from VIC
    uint8_t read(const uint_least16_t addr) { return 0; }
};

#endif
