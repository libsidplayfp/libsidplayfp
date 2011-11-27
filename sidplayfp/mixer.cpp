/***************************************************************************
                          mixer.cpp  -  Sids Mixer Routines
                             -------------------
    begin                : Sun Jul 9 2000
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


#include "player.h"

const int_least32_t VOLUME_MAX = 255;

SIDPLAY2_NAMESPACE_START

/* Scheduling time for next sample event. 20000 is roughly 20 ms and
 * gives us about 1k samples per mixing event on typical settings. */
const int Player::MIXER_EVENT_RATE = 20000;

void Player::mixer (void)
{
    short *buf = m_sampleBuffer + m_sampleIndex;
    sidemu *chip1 = sid[0];
    sidemu *chip2 = sid[1];

    /* this clocks the SID to the present moment, if it isn't already. */
    chip1->clock();
    if (chip2)
        chip2->clock();

    /* extract buffer info now that the SID is updated.
     * clock() may update bufferpos. */
    short *buf1 = chip1->buffer();
    short *buf2 = chip2?chip2->buffer():0;
    int samples = chip1->bufferpos();
    /* NB: if chip2 exists, its bufferpos is identical to chip1's. */

    int i = 0;
    while (i < samples) {
        /* Handle whatever output the sid has generated so far */
        if (m_sampleIndex >= m_sampleCount) {
            m_running = false;
            break;
        }
        /* Are there enough samples to generate the next one? */
        if (i + m_fastForwardFactor >= samples)
            break;

        /* This is a crude boxcar low-pass filter to
         * reduce aliasing during fast forward, something I commonly do. */
        int sample1 = 0;
        int sample2 = 0;
        int j;
        for (j = 0; j < m_fastForwardFactor; j += 1) {
            sample1 += buf1[i + j];
            if (buf2 != NULL)
                sample2 += buf2[i + j];
        }
        /* increment i to mark we ate some samples, finish the boxcar thing. */
        i += j;
        sample1 = sample1 * m_leftVolume / VOLUME_MAX;
        sample1 /= j;
        sample2 = sample2 * m_rightVolume / VOLUME_MAX;
        sample2 /= j;

        /* mono mix. */
        if (buf2 != NULL && m_cfg.playback != sid2_stereo)
            sample1 = (sample1 + sample2) / 2;
        /* stereo clone, for people who keep stereo on permanently. */
        if (buf2 == NULL && m_cfg.playback == sid2_stereo)
            sample2 = sample1;

        *buf++ = (short int)sample1;
        m_sampleIndex ++;
        if (m_cfg.playback == sid2_stereo) {
            *buf++ = (short int)sample2;
            m_sampleIndex ++;
        }
    }

    /* move the unhandled data to start of buffer, if any. */
    int j = 0;
    for (j = 0; j < samples - i; j += 1) {
        buf1[j] = buf1[i + j];
        if (buf2 != NULL)
            buf2[j] = buf2[i + j];
    }
    chip1->bufferpos(j);
    if (chip2)
        chip2->bufferpos(j);

    /* Post a callback to ourselves. */
    context().schedule(m_mixerEvent, MIXER_EVENT_RATE, EVENT_CLOCK_PHI1);
}

SIDPLAY2_NAMESPACE_STOP
