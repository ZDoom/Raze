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

/*
 length = count of samples to mix
 position = offset of starting sample in source
 rate = resampling increment
 volume = direct volume adjustment, 1.0 = no change
 */

// 8-bit mono source, 16-bit mono output
uint32_t MV_Mix16BitMono(struct VoiceNode * const voice, uint32_t length)
{
    auto const source = (uint8_t const *)voice->sound;
    auto       dest   = (int16_t *)MV_MixDestination;

    uint32_t       position = voice->position;
    uint32_t const rate     = voice->RateScale;
    float const    volume   = voice->volume*MV_GlobalVolume;

    do
    {
        int32_t const isample0 = FLIP_SIGN(source[position >> 16]) << 8;

        position += rate;

        *dest = (int16_t)clamp(SCALE_SAMPLE(isample0, volume*voice->LeftVolume) + *dest, INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;

        voice->LeftVolume = SMOOTH_VOLUME(voice->LeftVolume, voice->LeftVolumeDest);
    }
    while (--length);

    MV_MixDestination = (char *)dest;

    return position;
}

// 8-bit mono source, 16-bit stereo output
uint32_t MV_Mix16BitStereo(struct VoiceNode * const voice, uint32_t length)
{
    auto const source = (uint8_t const *)voice->sound;
    auto       dest   = (int16_t *)MV_MixDestination;

    uint32_t       position = voice->position;
    uint32_t const rate     = voice->RateScale;
    float const    volume   = voice->volume*MV_GlobalVolume;

    do
    {
        int32_t const isample0 = FLIP_SIGN(source[position >> 16]) << 8;

        position += rate;

        *dest = (int16_t)clamp(SCALE_SAMPLE(isample0, volume*voice->LeftVolume) + *dest, INT16_MIN, INT16_MAX);
        *(dest + (MV_RightChannelOffset >> 1))
            = (int16_t)clamp(SCALE_SAMPLE(isample0, volume*voice->RightVolume) + *(dest + (MV_RightChannelOffset >> 1)), INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;

        voice->LeftVolume = SMOOTH_VOLUME(voice->LeftVolume, voice->LeftVolumeDest);
        voice->RightVolume = SMOOTH_VOLUME(voice->RightVolume, voice->RightVolumeDest);
    }
    while (--length);

    MV_MixDestination = (char *)dest;

    return position;
}

// 16-bit mono source, 16-bit mono output
uint32_t MV_Mix16BitMono16(struct VoiceNode * const voice, uint32_t length)
{
    auto const source = (int16_t const *)voice->sound;
    auto       dest   = (int16_t *)MV_MixDestination;

    uint32_t       position = voice->position;
    uint32_t const rate     = voice->RateScale;
    float const    volume   = voice->volume*MV_GlobalVolume;

    do
    {
        int32_t const isample0 = B_LITTLE16(source[position >> 16]);

        position += rate;

        int32_t const sample0 = SCALE_SAMPLE(isample0, volume*voice->LeftVolume);

        *dest = (int16_t)clamp(sample0 + *dest, INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;

        voice->LeftVolume = SMOOTH_VOLUME(voice->LeftVolume, voice->LeftVolumeDest);
    }
    while (--length);

    MV_MixDestination = (char *)dest;

    return position;
}

// 16-bit mono source, 16-bit stereo output
uint32_t MV_Mix16BitStereo16(struct VoiceNode * const voice, uint32_t length)
{
    auto const source = (int16_t const *)voice->sound;
    auto       dest   = (int16_t *)MV_MixDestination;

    uint32_t       position = voice->position;
    uint32_t const rate     = voice->RateScale;
    float const    volume   = voice->volume*MV_GlobalVolume;

    do
    {
        int32_t const isample0 = B_LITTLE16(source[position >> 16]);

        position += rate;

        int32_t const sample0 = SCALE_SAMPLE(isample0, volume*voice->LeftVolume);
        int32_t const sample1 = SCALE_SAMPLE(isample0, volume*voice->RightVolume);

        *dest = (int16_t)clamp(sample0 + *dest, INT16_MIN, INT16_MAX);
        *(dest + (MV_RightChannelOffset >> 1))
            = (int16_t)clamp(sample1 + *(dest + (MV_RightChannelOffset >> 1)), INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;

        voice->LeftVolume = SMOOTH_VOLUME(voice->LeftVolume, voice->LeftVolumeDest);
        voice->RightVolume = SMOOTH_VOLUME(voice->RightVolume, voice->RightVolumeDest);
    }
    while (--length);

    MV_MixDestination = (char *)dest;

    return position;
}

void MV_16BitReverb(char const *src, char *dest, const float volume, int32_t count)
{
    auto input  = (uint16_t const *)src;
    auto output = (int16_t *)dest;

    do
    {
        int16_t const isample0 = (int16_t)*input++;
        *output++ = SCALE_SAMPLE(isample0, volume);
    }
    while (--count > 0);
}
