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

// 8-bit stereo source, 16-bit mono output
void MV_Mix16BitMono8Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length, float volume)
{
    auto const source = (uint8_t const *)start;
    int16_t *  dest   = (int16_t *)MV_MixDestination;

    while (length--)
    {
        int const sample0 = MV_VOLUME(source[(position >> 16) << 1]);
        int const sample1 = MV_VOLUME(source[((position >> 16) << 1) + 1]);
        position += rate;

        *dest = (int16_t)clamp(((MV_LeftVolume[sample0] + MV_LeftVolume[sample1]) >> 1) + *dest, INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

// 8-bit stereo source, 16-bit stereo output
void MV_Mix16BitStereo8Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length, float volume)
{
    auto const source = (uint8_t const *)start;
    int16_t *  dest   = (int16_t *)MV_MixDestination;

    while (length--)
    {
        int const sample0 = MV_VOLUME(source[(position >> 16) << 1]);
        int const sample1 = MV_VOLUME(source[((position >> 16) << 1) + 1]);
        position += rate;

        *dest = (int16_t)clamp(MV_LeftVolume[sample0] + *dest, INT16_MIN, INT16_MAX);
        *(dest + (MV_RightChannelOffset >> 1))
        = (int16_t)clamp(MV_RightVolume[sample1] + *(dest + (MV_RightChannelOffset >> 1)), INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

// 16-bit stereo source, 16-bit mono output
void MV_Mix16BitMono16Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length, float volume)
{
    auto const source = (uint16_t const *)start;
    int16_t *  dest   = (int16_t *)MV_MixDestination;

    while (length--)
    {
        int sample0 = MV_VOLUME(source[(position >> 16) << 1]);
        int sample1 = MV_VOLUME(source[((position >> 16) << 1) + 1]);
#ifdef BIGENDIAN
        int sample0l = sample0 >> 8;
        int sample0h = (sample0 & 255) ^ 128;
        int sample1l = sample1 >> 8;
        int sample1h = (sample1 & 255) ^ 128;
#else
        int sample0l = sample0 & 255;
        int sample0h = (sample0 >> 8) ^ 128;
        int sample1l = sample1 & 255;
        int sample1h = (sample1 >> 8) ^ 128;
#endif
        position += rate;

        sample0l = MV_LeftVolume[sample0l] >> 8;
        sample0h = MV_LeftVolume[sample0h];
        sample0  = sample0l + sample0h + 128;

        sample1l = MV_LeftVolume[sample1l] >> 8;
        sample1h = MV_LeftVolume[sample1h];
        sample1  = sample1l + sample1h + 128;

        *dest = (int16_t)clamp(((sample0 + sample1) >> 1) + *dest, INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

// 16-bit stereo source, 16-bit stereo output
void MV_Mix16BitStereo16Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length, float volume)
{
    auto const source = (uint16_t const *)start;
    int16_t *  dest   = (int16_t *)MV_MixDestination;

    while (length--)
    {
        int sample0 = MV_VOLUME(source[(position >> 16) << 1]);
        int sample1 = MV_VOLUME(source[((position >> 16) << 1) + 1]);
#ifdef BIGENDIAN
        int sample0l = sample0 >> 8;
        int sample0h = (sample0 & 255) ^ 128;
        int sample1l = sample1 >> 8;
        int sample1h = (sample1 & 255) ^ 128;
#else
        int sample0l = sample0 & 255;
        int sample0h = (sample0 >> 8) ^ 128;
        int sample1l = sample1 & 255;
        int sample1h = (sample1 >> 8) ^ 128;
#endif
        position += rate;

        sample0l = MV_LeftVolume[sample0l] >> 8;
        sample0h = MV_LeftVolume[sample0h];

        sample1l = MV_RightVolume[sample1l] >> 8;
        sample1h = MV_RightVolume[sample1h];

        *dest = (int16_t)clamp(sample0l + sample0h + 128 + *dest, INT16_MIN, INT16_MAX);
        *(dest + (MV_RightChannelOffset >> 1))
        = (int16_t)clamp(sample1l + sample1h + 128 + *(dest + (MV_RightChannelOffset >> 1)), INT16_MIN, INT16_MAX);
        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}
