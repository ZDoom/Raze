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
 JBF:

 position = offset of starting sample in start
 rate = resampling increment
 start = sound data
 length = count of samples to mix
 */

// 8-bit stereo source, 16-bit mono output
void MV_Mix16BitMono8Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length)
{
    uint8_t const * const source = (uint8_t const *) start;
    int16_t *dest = (int16_t *) MV_MixDestination;
    int32_t sample0, sample1;

    while (length--) {
        sample0 = source[(position >> 16) << 1];
        sample1 = source[((position >> 16) << 1) + 1];
        position += rate;

        sample0 = (MV_LeftVolume[sample0] + MV_LeftVolume[sample1]) / 2 + *dest;
        if (sample0 < -32768) sample0 = -32768;
        else if (sample0 > 32767) sample0 = 32767;

        *dest = (int16_t) sample0;

        dest += MV_SampleSize / 2;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *) dest;
}

// 8-bit stereo source, 16-bit stereo output
void MV_Mix16BitStereo8Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length)
{
    uint8_t const * const source = (uint8_t const *) start;
    int16_t *dest = (int16_t *) MV_MixDestination;
    int32_t sample0, sample1;

    while (length--) {
        sample0 = source[(position >> 16) << 1];
        sample1 = source[((position >> 16) << 1) + 1];
        position += rate;

        sample0 = MV_LeftVolume[sample0] + *dest;
        sample1 = MV_RightVolume[sample1] + *(dest + MV_RightChannelOffset / 2);
        if (sample0 < -32768) sample0 = -32768;
        else if (sample0 > 32767) sample0 = 32767;
        if (sample1 < -32768) sample1 = -32768;
        else if (sample1 > 32767) sample1 = 32767;

        *dest = (int16_t) sample0;
        *(dest + MV_RightChannelOffset/2) = (int16_t) sample1;

        dest += MV_SampleSize / 2;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *) dest;
}

// 16-bit stereo source, 16-bit mono output
void MV_Mix16BitMono16Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length)
{
    uint16_t const * const source = (uint16_t const *) start;
    int16_t *dest = (int16_t *) MV_MixDestination;
    int32_t sample0l, sample0h, sample0;
    int32_t sample1l, sample1h, sample1;

    while (length--) {
        sample0 = source[(position >> 16) << 1];
        sample1 = source[((position >> 16) << 1) + 1];
#ifdef BIGENDIAN
        sample0l = sample0 >> 8;
        sample0h = (sample0 & 255) ^ 128;
        sample1l = sample1 >> 8;
        sample1h = (sample1 & 255) ^ 128;
#else
        sample0l = sample0 & 255;
        sample0h = (sample0 >> 8) ^ 128;
        sample1l = sample1 & 255;
        sample1h = (sample1 >> 8) ^ 128;
#endif
        position += rate;

        sample0l = MV_LeftVolume[sample0l] >> 8;
        sample0h = MV_LeftVolume[sample0h];
        sample0 = sample0l + sample0h + 128;
        sample1l = MV_LeftVolume[sample1l] >> 8;
        sample1h = MV_LeftVolume[sample1h];
        sample1 = sample1l + sample1h + 128;

        sample0 = (sample0 + sample1) / 2 + *dest;
        if (sample0 < -32768) sample0 = -32768;
        else if (sample0 > 32767) sample0 = 32767;

        *dest = (int16_t) sample0;

        dest += MV_SampleSize / 2;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *) dest;
}

// 16-bit stereo source, 16-bit stereo output
void MV_Mix16BitStereo16Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length)
{
    uint16_t const * const source = (uint16_t const *) start;
    int16_t *dest = (int16_t *) MV_MixDestination;
    int32_t sample0l, sample0h, sample0;
    int32_t sample1l, sample1h, sample1;

    while (length--) {
        sample0 = source[(position >> 16) << 1];
        sample1 = source[((position >> 16) << 1) + 1];
#ifdef BIGENDIAN
        sample0l = sample0 >> 8;
        sample0h = (sample0 & 255) ^ 128;
        sample1l = sample1 >> 8;
        sample1h = (sample1 & 255) ^ 128;
#else
        sample0l = sample0 & 255;
        sample0h = (sample0 >> 8) ^ 128;
        sample1l = sample1 & 255;
        sample1h = (sample1 >> 8) ^ 128;
#endif
        position += rate;

        sample0l = MV_LeftVolume[sample0l] >> 8;
        sample0h = MV_LeftVolume[sample0h];
        sample1l = MV_RightVolume[sample1l] >> 8;
        sample1h = MV_RightVolume[sample1h];
        sample0 = sample0l + sample0h + 128 + *dest;
        sample1 = sample1l + sample1h + 128 + *(dest + MV_RightChannelOffset/2);
        if (sample0 < -32768) sample0 = -32768;
        else if (sample0 > 32767) sample0 = 32767;
        if (sample1 < -32768) sample1 = -32768;
        else if (sample1 > 32767) sample1 = 32767;

        *dest = (int16_t) sample0;
        *(dest + MV_RightChannelOffset/2) = (int16_t) sample1;

        dest += MV_SampleSize / 2;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *) dest;
}
