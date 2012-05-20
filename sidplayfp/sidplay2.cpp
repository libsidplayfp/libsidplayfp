/***************************************************************************
                          sidplayer.cpp  -  Wrapper to hide private
                                            header files (see below)
                             -------------------
    begin                : Fri Jun 9 2000
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


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// Redirection to private version of sidplayer (This method is called Cheshire Cat)
// [ms: which is J. Carolan's name for a degenerate 'bridge']
// This interface can be directly replaced with a libsidplay1 or C interface wrapper.
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

#include <stdio.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "player.h"
#include "sidplay2.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

sidplay2::sidplay2 ()
#ifdef HAVE_EXCEPTIONS
: sidplayer (*(new(std::nothrow) SIDPLAY2_NAMESPACE::Player))
#else
: sidplayer (*(new SIDPLAY2_NAMESPACE::Player))
#endif
{
}

sidplay2::~sidplay2 ()
{   if (&sidplayer) delete &sidplayer; }

int sidplay2::config (const sid2_config_t &cfg)
{   return sidplayer.config (cfg); }

const sid2_config_t &sidplay2::config (void) const
{   return sidplayer.config (); }

void sidplay2::stop (void)
{   sidplayer.stop (); }

uint_least32_t sidplay2::play (short *buffer, uint_least32_t count)
{   return sidplayer.play (buffer, count); }

int sidplay2::load (SidTune *tune)
{   return sidplayer.load (tune); }

const sid2_info_t &sidplay2::info () const
{   return sidplayer.info (); }

uint_least32_t sidplay2::time (void) const
{   return sidplayer.time (); }

uint_least32_t sidplay2::mileage (void) const
{   return sidplayer.mileage (); }

const char *sidplay2::error (void) const
{   return sidplayer.error (); }

int  sidplay2::fastForward  (uint percent)
{   return sidplayer.fastForward (percent); }

void sidplay2::mute(int voice, bool enable)
{   sidplayer.mute(voice, enable); }

void sidplay2::debug (bool enable, FILE *out)
{   sidplayer.debug (enable, out); }

sid2_player_t sidplay2::state (void) const
{   return sidplayer.state (); }

void sidplay2::setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
{   sidplayer.setRoms(kernal, basic, character); }

EventContext *sidplay2::getEventContext()
{   return sidplayer.getEventScheduler(); }

bool sidplay2::getStatus() const
{ return sidplayer.getStatus(); }
