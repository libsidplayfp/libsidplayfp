/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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
: sidplayer (*(new(std::nothrow) SIDPLAYFP_NAMESPACE::Player))
#else
: sidplayer (*(new SIDPLAYFP_NAMESPACE::Player))
#endif
{
}

sidplay2::~sidplay2 ()
{   if (&sidplayer) delete &sidplayer; }

bool sidplay2::config (const SidConfig &cfg)
{   return sidplayer.config (cfg); }

const SidConfig &sidplay2::config (void) const
{   return sidplayer.config (); }

void sidplay2::stop (void)
{   sidplayer.stop (); }

uint_least32_t sidplay2::play (short *buffer, uint_least32_t count)
{   return sidplayer.play (buffer, count); }

bool sidplay2::load (SidTune *tune)
{   return sidplayer.load (tune); }

const SidInfo &sidplay2::info () const
{   return sidplayer.info (); }

uint_least32_t sidplay2::time (void) const
{   return sidplayer.time (); }

const char *sidplay2::error (void) const
{   return sidplayer.error (); }

bool  sidplay2::fastForward  (unsigned int percent)
{   return sidplayer.fastForward (percent); }

void sidplay2::mute(const unsigned int sidNum, const unsigned int voice, const bool enable)
{   sidplayer.mute(sidNum, voice, enable); }

void sidplay2::debug (bool enable, FILE *out)
{   sidplayer.debug (enable, out); }

bool sidplay2::isPlaying (void) const
{   return sidplayer.isPlaying (); }

void sidplay2::setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
{   sidplayer.setRoms(kernal, basic, character); }

EventContext *sidplay2::getEventContext()
{   return sidplayer.getEventScheduler(); }

bool sidplay2::getStatus() const
{ return sidplayer.getStatus(); }

uint_least16_t sidplay2::getCia1TimerA() const
{ return sidplayer.getCia1TimerA(); }
