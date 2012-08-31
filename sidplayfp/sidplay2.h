/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2012 Leando Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
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

#ifndef SIDPLAY2_H
#define SIDPLAY2_H

#include <stdio.h>

#include "sid2types.h"
#include "sidconfig.h"
#include "sidversion.h"

class  SidTune;
class  EventContext;
struct SidTuneInfo;

// Private Sidplayer
namespace SIDPLAYFP_NAMESPACE
{
    class Player;
}

/**
* sidplay2
*/
class SID_EXTERN sidplay2
{
private:
    SIDPLAYFP_NAMESPACE::Player &sidplayer;

public:
    sidplay2 ();
    virtual ~sidplay2 ();

    /**
    * Get the current engine configuration.
    *
    * @return a const reference to the current configuration.
    */
    const sid2_config_t &config(void) const;

    /**
    * Get the current player informations.
    *
    * @return a const reference to the current info.
    */
    const sid2_info_t &info(void) const;

    /**
    * Configure the engine.
    *
    * @param cfg the new configuration
    * @return true on sucess, false otherwise.
    */
    bool config(const sid2_config_t &cfg);

    /**
    * Error message.
    *
    * @return string error message.
    */
    const char *error(void) const;

    /**
    * 
    *
    * @param percent
    */
    bool fastForward(uint percent);

    /**
    * Load a tune.
    *
    * @param tune the SidTune to load, 0 unloads current tune.
    * @return true on sucess, false otherwise.
    */
    bool load(SidTune *tune);

    /**
    * Produce samples to play.
    *
    * @param buffer pointer to the buffer to fill with samples.
    * @param count the size of the buffer.
    * @return the number of produced samples.
    */
    uint_least32_t play(short *buffer, uint_least32_t count);

    /**
    * Check if the engine is playing or stopped.
    *
    * @return true if playing, false otherwise.
    */
    bool isPlaying(void) const;

    /**
    * Stop engine
    */
    void stop(void);

    /**
    * Control debugging.
    *
    * @param enable enable/disable debugging.
    * @param out the file where to redirect the debug info.
    */
    void debug(bool enable, FILE *out);

    /**
    * Mute/unmute a SID channel.
    *
    * @param sidNum the SID chip, 0 for the first one, 1 for the second.
    * @param voice the channel to mute/unmute.
    * @param enable true unmutes the channel, false mutes it.
    */
    void mute(const unsigned int sidNum, const unsigned int voice, const bool enable);

    /**
    * Get the current playing time with respect to resolution returned by timebase.
    *
    * @return the current playing time.
    */
    uint_least32_t time(void) const;

    /**
    * Set ROMs
    * The ROMs are validate against konwn ones.
    * The Kernal ROM is the only one mandatory.
    *
    * @param kernal pointer to Kernal ROM.
    * @param basic pointer to Basic ROM, optional, generally needed only for BASIC tunes.
    * @param character pointer to character generator ROM, optional.
    */
    void setRoms(const uint8_t* kernal, const uint8_t* basic=0, const uint8_t* character=0);

    /**
    * Get the event scheduler.
    *
    * @return the scheduler.
    */
    EventContext *getEventContext();

    /**
    * Check the status of the engine.
    *
    * @return true if the engine is correctly initialized.
    */
    bool getStatus() const;

    /**
    * Get the CIA 1 Timer A programmed value
    */
    uint_least16_t getCia1TimerA() const;
};

#endif // SIDPLAY2_H
