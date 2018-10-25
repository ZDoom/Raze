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
 position = offset of starting sample in start
 rate = resampling increment
 start = sound data
 length = count of samples to mix
 volume = direct volume adjustment, 1.0 = no change
 */

// 8-bit mono source, 16-bit mono output
void MV_Mix16BitMono(struct VoiceNode *voice, uint32_t length)
{
    auto const     source   = (uint8_t const *)voice->sound;
    int16_t *      dest     = (int16_t *)MV_MixDestination;
    uint32_t       position = voice->position;
    uint32_t const rate     = voice->RateScale;
    float const    volume   = voice->volume;

    while (length--)
    {
        uint8_t const usample0 = SCALE_SAMPLE(source[position >> 16], volume);

        position += rate;

        *dest = (int16_t)clamp(SCALE_SAMPLE(MV_LeftVolume[usample0], MV_GlobalVolume) + *dest, INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

// 8-bit mono source, 16-bit stereo output
void MV_Mix16BitStereo(struct VoiceNode *voice, uint32_t length)
{
    auto const     source   = (uint8_t const *)voice->sound;
    int16_t *      dest     = (int16_t *)MV_MixDestination;
    uint32_t       position = voice->position;
    uint32_t const rate     = voice->RateScale;
    float const    volume   = voice->volume;

    while (length--)
    {
        uint8_t const usample0 = SCALE_SAMPLE(source[position >> 16], volume);

        position += rate;

        *dest = (int16_t)clamp(SCALE_SAMPLE(MV_LeftVolume[usample0], MV_GlobalVolume) + *dest, INT16_MIN, INT16_MAX);
        *(dest + (MV_RightChannelOffset >> 1))
            = (int16_t)clamp(SCALE_SAMPLE(MV_RightVolume[usample0], MV_GlobalVolume) + *(dest + (MV_RightChannelOffset >> 1)), INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

// 16-bit mono source, 16-bit mono output
void MV_Mix16BitMono16(struct VoiceNode *voice, uint32_t length)
{
    auto const     source   = (int16_t const *)voice->sound;
    int16_t *      dest     = (int16_t *)MV_MixDestination;
    uint32_t       position = voice->position;
    uint32_t const rate     = voice->RateScale;
    float const    volume   = voice->volume;

    while (length--)
    {
        int16_t const isample0 = B_LITTLE16(source[position >> 16]);
        split16_t const usample0{FLIP_SIGN(SCALE_SAMPLE(isample0, volume))};

        position += rate;

        int32_t const sample0 = (SCALE_SAMPLE(MV_LeftVolume[usample0.l()], MV_GlobalVolume) >> 8) +
                                 SCALE_SAMPLE(MV_LeftVolume[usample0.h()], MV_GlobalVolume) + 128;

        *dest = (int16_t)clamp(sample0 + *dest, INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

// 16-bit mono source, 16-bit stereo output
void MV_Mix16BitStereo16(struct VoiceNode *voice, uint32_t length)
{
    auto const     source   = (int16_t const *)voice->sound;
    int16_t *      dest     = (int16_t *)MV_MixDestination;
    uint32_t       position = voice->position;
    uint32_t const rate     = voice->RateScale;
    float const    volume   = voice->volume;

    while (length--)
    {
        int16_t const isample0 = B_LITTLE16(source[position >> 16]);
        split16_t const usample0{FLIP_SIGN(SCALE_SAMPLE(isample0, volume))};

        position += rate;

        int32_t const sample0 = (SCALE_SAMPLE(MV_LeftVolume[usample0.l()], MV_GlobalVolume) >> 8) +
                                 SCALE_SAMPLE(MV_LeftVolume[usample0.h()], MV_GlobalVolume) + 128;
        int32_t const sample1 = (SCALE_SAMPLE(MV_RightVolume[usample0.l()], MV_GlobalVolume) >> 8) +
                                 SCALE_SAMPLE(MV_RightVolume[usample0.h()], MV_GlobalVolume) + 128;

        *dest = (int16_t)clamp(sample0 + *dest, INT16_MIN, INT16_MAX);
        *(dest + (MV_RightChannelOffset >> 1))
            = (int16_t)clamp(sample1 + *(dest + (MV_RightChannelOffset >> 1)), INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

void MV_16BitReverb(char const *src, char *dest, int16_t *volume, int32_t count)
{
    auto     input  = (uint16_t const *)src;
    int16_t *output = (int16_t *)dest;

    do
    {
        int const sample0 = *input++;
#if 0  // def BIGENDIAN
        int sample0l = sample0 >> 8;
        int sample0h = (sample0 & 255) ^ 128;
#else
        int sample0l = sample0 & 255;
        int sample0h = (sample0 >> 8) ^ 128;
#endif

        sample0l  = ((int16_t *)volume)[sample0l] >> 8;
        sample0h  = ((int16_t *)volume)[sample0h];
        *output++ = (int16_t)(sample0l + sample0h + 128);
    } while (--count > 0);
}
