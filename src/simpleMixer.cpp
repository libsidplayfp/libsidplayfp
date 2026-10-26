/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "simpleMixer.h"

#include <cassert>
#include <climits>
#include <cstring>

#ifdef __cpp_lib_unreachable
#  include <utility>
#endif

#include "sidemu.h"


namespace libsidplayfp
{

unsigned int SimpleMixer::doMix(short *buffer, unsigned int samples)
{
    unsigned int j = 0;
    for (unsigned int i=0; i<samples; i++)
    {
        for (size_t k=0; k<m_buffers.size(); k++)
        {
            const short *buf = m_buffers[k];
            m_iSamples[k] = buf[i];
        }
        if (m_oplBuffer)
            m_oplSample = m_oplBuffer[i];

        for (auto mix: m_mix)
        {
            const int_least32_t tmp = (this->*(mix))();
            assert((tmp >= SHRT_MIN) && (tmp <= SHRT_MAX));
            buffer[j++] = static_cast<short>(tmp);
        }
    }

    return j;
}

int_least32_t SimpleMixer::invCount(unsigned int n)
{
    return SCALE_FACTOR / static_cast<int_least32_t>(n);
}

void SimpleMixer::setupGeneric(bool stereo, const SidTuneInfo::pan_t *pans, int chips)
{
    if (!stereo)
    {
        m_monoScale = invCount(chips);
        m_mix.push_back(&SimpleMixer::monoGeneric);
        return;
    }

    // Chips feeding each channel, a centered chip feeds both
    unsigned int countL = 0;
    unsigned int countR = 0;
    for (int i = 0; i < chips; i++)
    {
        const SidTuneInfo::pan_t pan = pans ? pans[i] : SidTuneInfo::PAN_CENTER;
        if (pan != SidTuneInfo::PAN_RIGHT) countL++;
        if (pan != SidTuneInfo::PAN_LEFT) countR++;
    }

    // Scale by chips in channel, empty channel stays silent
    const int_least32_t scaleL = countL ? invCount(countL) : 0;
    const int_least32_t scaleR = countR ? invCount(countR) : 0;

    for (int i = 0; i < chips; i++)
    {
        const SidTuneInfo::pan_t pan = pans ? pans[i] : SidTuneInfo::PAN_CENTER;
        m_gainL.push_back(pan != SidTuneInfo::PAN_RIGHT ? scaleL : 0);
        m_gainR.push_back(pan != SidTuneInfo::PAN_LEFT ? scaleR : 0);
    }

    m_mix.push_back(&SimpleMixer::stereo_ch1_Generic);
    m_mix.push_back(&SimpleMixer::stereo_ch2_Generic);
}

SimpleMixer::SimpleMixer(bool stereo, short** buffers, int chips, bool opl, const SidTuneInfo::pan_t *pans)
{
    if ((stereo && pans) || (chips > 3))
    {
        setupGeneric(stereo, pans, chips);
    }
    else
    {
        switch (chips)
        {
        case 1:
            m_mix.push_back(stereo ? &SimpleMixer::stereo_OneChip : &SimpleMixer::template mono<1>);
            if (stereo) m_mix.push_back(&SimpleMixer::stereo_OneChip);
            break;
        case 2:
            m_mix.push_back(stereo ? &SimpleMixer::stereo_ch1_TwoChips : &SimpleMixer::template mono<2>);
            if (stereo) m_mix.push_back(&SimpleMixer::stereo_ch2_TwoChips);
            break;
        case 3:
            m_mix.push_back(stereo ? &SimpleMixer::stereo_ch1_ThreeChips : &SimpleMixer::template mono<3>);
            if (stereo) m_mix.push_back(&SimpleMixer::stereo_ch2_ThreeChips);
            break;
#ifdef __cpp_lib_unreachable
        default:
            std::unreachable();
#endif
        }
    }

    m_iSamples.resize(chips);
    for (int i=0; i<chips; i++)
        m_buffers.push_back(buffers[i]);

    if (opl)
        m_oplBuffer = buffers[chips];
}

}
