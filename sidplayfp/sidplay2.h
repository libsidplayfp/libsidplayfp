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

#include "sidtypes.h"
#include "sid2types.h"
#include "sidversion.h"

class SidTune;

// Private Sidplayer
namespace SIDPLAY2_NAMESPACE
{
    class Player;
}

/**
* sidplay2
*/
class SID_EXTERN sidplay2
{
private:
    SIDPLAY2_NAMESPACE::Player &sidplayer;

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
    * Pause the engine
    */
    void           pause        (void);

    /**
    * Produce samples to play
    *
    * @param buffer pointer to the buffer to fill with samples.
    * @param count the size of the buffer.
    * @return the number of produced samples.
    */
    uint_least32_t play         (short *buffer, uint_least32_t count);

    /**
    * Check the state of the engine.
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
    SID_DEPRECATED uint_least32_t timebase (void) const { return 1; }
    uint_least32_t time     (void) const;
    uint_least32_t mileage  (void) const;
    //@}

    SID_DEPRECATED operator bool()  const { return (&sidplayer ? true: false); }
    SID_DEPRECATED bool operator!() const { return (&sidplayer ? false: true); }
};

#endif // _sidplay2_h_
