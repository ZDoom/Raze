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
void MV_Mix16BitMono(uint32_t position, uint32_t rate, const char *start, uint32_t length, float volume)
{
    auto const source = (uint8_t const *)start;
    int16_t *  dest   = (int16_t *)MV_MixDestination;

    while (length--)
    {
        int const sample0 = MV_VOLUME(source[position >> 16]);
        position += rate;

        *dest = (int16_t)clamp(MV_LeftVolume[sample0] + *dest, INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

// 8-bit mono source, 16-bit stereo output
void MV_Mix16BitStereo(uint32_t position, uint32_t rate, const char *start, uint32_t length, float volume)
{
    auto const source = (uint8_t const *)start;
    int16_t *  dest   = (int16_t *)MV_MixDestination;

    while (length--)
    {
        int const sample0 = MV_VOLUME(source[position >> 16]);
        position += rate;

        *dest = (int16_t)clamp(MV_LeftVolume[sample0] + *dest, INT16_MIN, INT16_MAX);
        *(dest + (MV_RightChannelOffset >> 1))
        = (int16_t)clamp(MV_RightVolume[sample0] + *(dest + (MV_RightChannelOffset >> 1)), INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

// 16-bit mono source, 16-bit mono output
void MV_Mix16BitMono16(uint32_t position, uint32_t rate, const char *start, uint32_t length, float volume)
{
    auto const source = (uint16_t const *)start;
    int16_t *  dest   = (int16_t *)MV_MixDestination;

    while (length--)
    {
        int const sample0 = MV_VOLUME(source[position >> 16]);
#ifdef BIGENDIAN
        int sample0l = sample0 >> 8;
        int sample0h = (sample0 & 255) ^ 128;
#else
        int sample0l = sample0 & 255;
        int sample0h = (sample0 >> 8) ^ 128;
#endif
        position += rate;

        sample0l = MV_LeftVolume[sample0l] >> 8;
        sample0h = MV_LeftVolume[sample0h];

        *dest = (int16_t)clamp(sample0l + sample0h + 128 + *dest, INT16_MIN, INT16_MAX);

        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

// 16-bit mono source, 16-bit stereo output
void MV_Mix16BitStereo16(uint32_t position, uint32_t rate, const char *start, uint32_t length, float volume)
{
    auto const source = (uint16_t const *)start;
    int16_t *  dest   = (int16_t *)MV_MixDestination;

    while (length--)
    {
        int const sample0 = MV_VOLUME(source[position >> 16]);
        position += rate;

#ifdef BIGENDIAN
        int sample0l = sample0 >> 8;
        int sample0h = (sample0 & 255) ^ 128;
#else
        int sample0l = sample0 & 255;
        int sample0h = (sample0 >> 8) ^ 128;
#endif

        int const sample1l = MV_RightVolume[sample0l] >> 8;
        int const sample1h = MV_RightVolume[sample0h];

        sample0l = MV_LeftVolume[sample0l] >> 8;
        sample0h = MV_LeftVolume[sample0h];

        *dest = (int16_t)clamp(sample0l + sample0h + 128 + *dest, INT16_MIN, INT16_MAX);
        *(dest + (MV_RightChannelOffset >> 1))
        = (int16_t)clamp(sample1l + sample1h + 128 + *(dest + (MV_RightChannelOffset >> 1)), INT16_MIN, INT16_MAX);

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
