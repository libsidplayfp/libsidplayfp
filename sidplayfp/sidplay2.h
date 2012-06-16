/***************************************************************************
                          sidplay2.h  -  Public sidplay header
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

#ifndef _sidplay2_h_
#define _sidplay2_h_

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

    const sid2_config_t &config (void) const;
    const sid2_info_t   &info   (void) const;

    /**
    * Configure the engine.
    *
    * @param cfg
    * @return 0 on sucess, -1 otherwise.
    */
    int            config       (const sid2_config_t &cfg);

    /**
    * Error message
    *
    * @return string error message.
    */
    const char    *error        (void) const;

    /**
    * 
    *
    * @param percent
    */
    int            fastForward  (uint percent);

    /**
    * Load a tune
    *
    * @param tune the SidTune to load, 0 unloads current tune
    * @return 0 on sucess, -1 otherwise
    */
    int            load         (SidTune *tune);

    /**
    * Produce samples to play
    *
    * @param buffer pointer to the buffer to fill with samples.
    * @param count the size of the buffer.
    * @return the number of produced samples.
    */
    uint_least32_t play         (short *buffer, uint_least32_t count);

    /**
    * Check if the engine is playing, paused or stopped.
    *
    * @return
    */
    sid2_player_t  state        (void) const;

    /**
    * Stop engine
    */
    void           stop         (void);

    /**
    * Control debugging
    *
    * @param enable enable/disable debugging.
    * @param out the file where to redirect the debug info.
    */
    void           debug        (bool enable, FILE *out);
    void           mute         (int voice, bool enable);

    //@{
    /// Timer functions with respect to resolution returned by timebase
    uint_least32_t time     (void) const;
    uint_least32_t mileage  (void) const;
    //@}

    /**
    * Set ROMs
    *
    * @param kernal pointer to Kernal ROM.
    * @param basic pointer to Basic ROM, optional, generally needed only for BASIC tunes.
    * @param character pointer to character generator ROM, optional.
    */
    void setRoms(const uint8_t* kernal, const uint8_t* basic=0, const uint8_t* character=0);

    /**
    * Get the event scheduler
    *
    * @return the scheduler
    */
    EventContext *getEventContext();

    /**
    * Check the status of the engine.
    *
    * @return true if the engine is correctly initialized.
    */
    bool           getStatus() const;
};

#endif // _sidplay2_h_
