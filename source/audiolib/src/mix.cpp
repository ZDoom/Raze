/*
 Copyright (C) 2009 Jonathon Fowler <jf@jonof.id.au>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */

#include "_multivc.h"

template uint32_t MV_MixMono<uint8_t, int16_t>(struct VoiceNode * const voice, uint32_t length);
template uint32_t MV_MixStereo<uint8_t, int16_t>(struct VoiceNode * const voice, uint32_t length);
template uint32_t MV_MixMono<int16_t, int16_t>(struct VoiceNode * const voice, uint32_t length);
template uint32_t MV_MixStereo<int16_t, int16_t>(struct VoiceNode * const voice, uint32_t length);
template void MV_Reverb<int16_t>(char const *src, char * const dest, const float volume, int count);

/*
 length = count of samples to mix
 position = offset of starting sample in source
 rate = resampling increment
 volume = direct volume adjustment, 1.0 = no change
 */

// mono source, mono output
template <typename S, typename D>
uint32_t MV_MixMono(struct VoiceNode * const voice, uint32_t length)
{
    auto const source = (S const *)voice->sound;
    auto       dest   = (D *)MV_MixDestination;

    uint32_t       position = voice->position;
    uint32_t const rate     = voice->RateScale;
    float const    volume   = voice->volume*MV_GlobalVolume;

    do
    {
        auto const isample0 = CONVERT_LE_SAMPLE_TO_SIGNED<S, D>(source[position >> 16]);

        position += rate;

        *dest = MIX_SAMPLES<D>(SCALE_SAMPLE(isample0, volume*voice->LeftVolume), *dest);
        dest++;

        voice->LeftVolume = SMOOTH_VOLUME(voice->LeftVolume, voice->LeftVolumeDest);
    }
    while (--length);

    MV_MixDestination = (char *)dest;

    return position;
}

// mono source, stereo output
template <typename S, typename D>
uint32_t MV_MixStereo(struct VoiceNode * const voice, uint32_t length)
{
    auto const source = (S const *)voice->sound;
    auto       dest   = (D *)MV_MixDestination;

    uint32_t       position = voice->position;
    uint32_t const rate     = voice->RateScale;
    float const    volume   = voice->volume*MV_GlobalVolume;

    do
    {
        auto const isample0 = CONVERT_LE_SAMPLE_TO_SIGNED<S, D>(source[position >> 16]);

        position += rate;

        *dest = MIX_SAMPLES<D>(SCALE_SAMPLE(isample0, volume*voice->LeftVolume), *dest);
        *(dest + (MV_RightChannelOffset / sizeof(*dest)))
            = MIX_SAMPLES<D>(SCALE_SAMPLE(isample0, volume*voice->RightVolume), *(dest + (MV_RightChannelOffset / sizeof(*dest))));
        dest += 2;

        voice->LeftVolume = SMOOTH_VOLUME(voice->LeftVolume, voice->LeftVolumeDest);
        voice->RightVolume = SMOOTH_VOLUME(voice->RightVolume, voice->RightVolumeDest);
    }
    while (--length);

    MV_MixDestination = (char *)dest;

    return position;
}

template <typename T>
void MV_Reverb(char const *src, char * const dest, const float volume, int count)
{
    auto input  = (T const *)src;
    auto output = (T *)dest;

    do
    {
        auto const isample0 = CONVERT_SAMPLE_TO_SIGNED<T>(*input++);
        *output++ = CONVERT_SAMPLE_FROM_SIGNED<T>(SCALE_SAMPLE(isample0, volume));
    }
    while (--count > 0);
}
