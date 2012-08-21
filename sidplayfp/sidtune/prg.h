/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012 Leando Nini <drfiemost@users.sourceforge.net>
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

#ifndef PRG_H
#define PRG_H

#include "SidTuneBase.h"

class prg : public SidTuneBase
{
protected:
    prg() {}

private:
    void load();

public:
    static SidTuneBase* load(const char *fileName,
                             Buffer_sidtt<const uint_least8_t>& dataBuf);

    virtual ~prg() {}

private:    // prevent copying
    prg(const prg&);
    prg& operator=(prg&);
};

#endif
