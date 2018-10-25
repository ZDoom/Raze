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
        uint8_t const usample0 = MV_VOLUME(source[(position >> 16) << 1], volume);
        uint8_t const usample1 = MV_VOLUME(source[((position >> 16) << 1) + 1], volume);

        position += rate;

        *dest = (int16_t)clamp(((MV_LeftVolume[usample0] + MV_LeftVolume[usample1]) >> 1) + *dest, INT16_MIN, INT16_MAX);
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
        uint8_t const usample0 = MV_VOLUME(source[(position >> 16) << 1], volume);
        uint8_t const usample1 = MV_VOLUME(source[((position >> 16) << 1) + 1], volume);

        position += rate;

        *dest = (int16_t)clamp(MV_LeftVolume[usample0] + *dest, INT16_MIN, INT16_MAX);
        *(dest + (MV_RightChannelOffset >> 1))
            = (int16_t)clamp(MV_RightVolume[usample1] + *(dest + (MV_RightChannelOffset >> 1)), INT16_MIN, INT16_MAX);

        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

// 16-bit stereo source, 16-bit mono output
void MV_Mix16BitMono16Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length, float volume)
{
    auto const source = (int16_t const *)start;
    int16_t *  dest   = (int16_t *)MV_MixDestination;

    while (length--)
    {
        int16_t const isample0 = B_LITTLE16(source[(position >> 16) << 1]);
        int16_t const isample1 = B_LITTLE16(source[((position >> 16) << 1) + 1]);
        split16_t const usample0{MV_FLIP_SIGNEDNESS(MV_VOLUME(isample0, volume))};
        split16_t const usample1{MV_FLIP_SIGNEDNESS(MV_VOLUME(isample1, volume))};

        position += rate;

        int32_t const sample0 = (MV_LeftVolume[usample0.l()] >> 8) + MV_LeftVolume[usample0.h()] + 128;
        int32_t const sample1 = (MV_LeftVolume[usample1.l()] >> 8) + MV_LeftVolume[usample1.h()] + 128;
        *dest = (int16_t)clamp(((sample0 + sample1) >> 1) + *dest, INT16_MIN, INT16_MAX);

        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}

// 16-bit stereo source, 16-bit stereo output
void MV_Mix16BitStereo16Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length, float volume)
{
    auto const source = (int16_t const *)start;
    int16_t *  dest   = (int16_t *)MV_MixDestination;

    while (length--)
    {
        int16_t const isample0 = B_LITTLE16(source[(position >> 16) << 1]);
        int16_t const isample1 = B_LITTLE16(source[((position >> 16) << 1) + 1]);
        split16_t const usample0{MV_FLIP_SIGNEDNESS(MV_VOLUME(isample0, volume))};
        split16_t const usample1{MV_FLIP_SIGNEDNESS(MV_VOLUME(isample1, volume))};

        position += rate;

        int32_t const sample0 = (MV_LeftVolume[usample0.l()] >> 8) + MV_LeftVolume[usample0.h()] + 128;
        int32_t const sample1 = (MV_RightVolume[usample1.l()] >> 8) + MV_RightVolume[usample1.h()] + 128;
        *dest = (int16_t)clamp(sample0 + *dest, INT16_MIN, INT16_MAX);
        *(dest + (MV_RightChannelOffset >> 1))
            = (int16_t)clamp(sample1 + *(dest + (MV_RightChannelOffset >> 1)), INT16_MIN, INT16_MAX);

        dest += MV_SampleSize >> 1;
    }

    MV_MixPosition    = position;
    MV_MixDestination = (char *)dest;
}
