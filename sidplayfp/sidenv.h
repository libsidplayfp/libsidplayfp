/***************************************************************************
                          environment.h - This is the environment file which
                                          defines all the standard functions
                                          to be inherited by the ICs.
                             -------------------
    begin                : Thu May 11 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _environment_h_
#define _environment_h_

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sidtypes.h"

class C64Environment
{
public:
    // Eniviroment functions
    virtual void    envReset        (void) =0;
    virtual uint8_t envReadMemByte  (const uint_least16_t addr) =0;
    virtual void    envWriteMemByte (const uint_least16_t addr, const uint8_t data) =0;

    // Sidplay compatibily funtions
    virtual bool    envCheckBankJump   (const uint_least16_t addr) =0;
    virtual uint8_t envReadMemDataByte (const uint_least16_t addr) =0;
    virtual void    envSleep           (void) =0;
#ifdef PC64_TESTSUITE
    virtual void    envLoadFile        (const char *file) =0;
#endif
};

#endif // _environment_h_
