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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 */

#include "_multivc.h"


void ClearBuffer_DW( void *ptr, unsigned data, int32_t length )
{
    unsigned *ptrdw = (unsigned *)ptr;
    while (length--) {
        *(ptrdw++) = data;
    }
}

/*
 JBF:
 
 position = offset of starting sample in start
 rate = resampling increment
 start = sound data
 length = count of samples to mix
 */

// 8-bit mono source, 8-bit mono output
void MV_Mix8BitMono( uint32_t position, uint32_t rate,
                    const char *start, uint32_t length )
{
    uint8_t *source = (uint8_t *) start;
    uint8_t *dest = (uint8_t *) MV_MixDestination;
    int32_t sample0;
    
    while (length--) {
        sample0 = source[position >> 16];
        position += rate;
        
        sample0 = MV_LeftVolume[sample0] + *dest;
        sample0 = MV_HarshClipTable[sample0 + 128];
        
        *dest = sample0 & 255;
        
        dest += MV_SampleSize;
    }
    
    MV_MixPosition = position;
    MV_MixDestination = (char *) dest;
}

// 8-bit mono source, 8-bit stereo output
void MV_Mix8BitStereo( uint32_t position, uint32_t rate,
                      const char *start, uint32_t length )
{
    uint8_t *source = (uint8_t *) start;
    uint8_t *dest = (uint8_t *) MV_MixDestination;
    int32_t sample0, sample1;
    
    while (length--) {
        sample0 = source[position >> 16];
        sample1 = sample0;
        position += rate;
        
        sample0 = MV_LeftVolume[sample0] + *dest;
        sample1 = MV_RightVolume[sample1] + *(dest + MV_RightChannelOffset);
        sample0 = MV_HarshClipTable[sample0 + 128];
        sample1 = MV_HarshClipTable[sample1 + 128];
        
        *dest = sample0 & 255;
        *(dest + MV_RightChannelOffset) = sample1 & 255;
        
        dest += MV_SampleSize;
    }
    
    MV_MixPosition = position;
    MV_MixDestination = (char *) dest;
}

// 8-bit mono source, 16-bit mono output
void MV_Mix16BitMono( uint32_t position, uint32_t rate,
                     const char *start, uint32_t length )
{
    uint8_t *source = (uint8_t *) start;
    int16_t *dest = (int16_t *) MV_MixDestination;
    int32_t sample0;
    
    while (length--) {
        sample0 = source[position >> 16];
        position += rate;
        
        sample0 = MV_LeftVolume[sample0] + *dest;
        if (sample0 < -32768) sample0 = -32768;
        else if (sample0 > 32767) sample0 = 32767;
        
        *dest = (int16_t) sample0;
        
        dest += MV_SampleSize / 2;
    }
    
    MV_MixPosition = position;
    MV_MixDestination = (char *) dest;
}

// 8-bit mono source, 16-bit stereo output
void MV_Mix16BitStereo( uint32_t position, uint32_t rate,
                       const char *start, uint32_t length )
{
    uint8_t *source = (uint8_t *) start;
    int16_t *dest = (int16_t *) MV_MixDestination;
    int32_t sample0, sample1;
    
    while (length--) {
        sample0 = source[position >> 16];
        sample1 = sample0;
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

// 16-bit mono source, 16-bit mono output
void MV_Mix16BitMono16( uint32_t position, uint32_t rate,
                       const char *start, uint32_t length )
{
    uint16_t *source = (uint16_t *) start;
    int16_t *dest = (int16_t *) MV_MixDestination;
    int32_t sample0l, sample0h, sample0;
    
    while (length--) {
        sample0 = source[position >> 16];
#ifdef BIGENDIAN
        sample0l = sample0 >> 8;
        sample0h = (sample0 & 255) ^ 128;
#else
        sample0l = sample0 & 255;
        sample0h = (sample0 >> 8) ^ 128;
#endif
        position += rate;
        
        sample0l = MV_LeftVolume[sample0l] >> 8;
        sample0h = MV_LeftVolume[sample0h];
        sample0 = sample0l + sample0h + 128 + *dest;
        if (sample0 < -32768) sample0 = -32768;
        else if (sample0 > 32767) sample0 = 32767;
        
        *dest = (int16_t) sample0;
        
        dest += MV_SampleSize / 2;
    }
    
    MV_MixPosition = position;
    MV_MixDestination = (char *) dest;
}

// 16-bit mono source, 8-bit mono output
void MV_Mix8BitMono16( uint32_t position, uint32_t rate,
                      const char *start, uint32_t length )
{
    int8_t *source = (int8_t *) start + 1;
    uint8_t *dest = (uint8_t *) MV_MixDestination;
    int32_t sample0;
    
    while (length--) {
        sample0 = source[(position >> 16) << 1];
        position += rate;
        
        sample0 = MV_LeftVolume[sample0 + 128] + *dest;
        sample0 = MV_HarshClipTable[sample0 + 128];
        
        *dest = sample0 & 255;
        
        dest += MV_SampleSize;
    }
    
    MV_MixPosition = position;
    MV_MixDestination = (char *) dest;
}

// 16-bit mono source, 8-bit stereo output
void MV_Mix8BitStereo16( uint32_t position, uint32_t rate,
                        const char *start, uint32_t length )
{
    int8_t *source = (int8_t *) start + 1;
    uint8_t *dest = (uint8_t *) MV_MixDestination;
    int32_t sample0, sample1;
    
    while (length--) {
        sample0 = source[(position >> 16) << 1];
        sample1 = sample0;
        position += rate;
        
        sample0 = MV_LeftVolume[sample0 + 128] + *dest;
        sample1 = MV_RightVolume[sample1 + 128] + *(dest + MV_RightChannelOffset);
        sample0 = MV_HarshClipTable[sample0 + 128];
        sample1 = MV_HarshClipTable[sample1 + 128];
        
        *dest = sample0 & 255;
        *(dest + MV_RightChannelOffset) = sample1 & 255;
        
        dest += MV_SampleSize;
    }
    
    MV_MixPosition = position;
    MV_MixDestination = (char *) dest;
}

// 16-bit mono source, 16-bit stereo output
void MV_Mix16BitStereo16( uint32_t position, uint32_t rate,
                         const char *start, uint32_t length )
{
    uint16_t *source = (uint16_t *) start;
    int16_t *dest = (int16_t *) MV_MixDestination;
    int32_t sample0l, sample0h, sample0;
    int32_t sample1l, sample1h, sample1;
    
    while (length--) {
        sample0 = source[position >> 16];
#ifdef BIGENDIAN
        sample0l = sample0 >> 8;
        sample0h = (sample0 & 255) ^ 128;
#else
        sample0l = sample0 & 255;
        sample0h = (sample0 >> 8) ^ 128;
#endif
        sample1l = sample0l;
        sample1h = sample0h;
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

void MV_16BitReverb( char *src, char *dest, VOLUME16 *volume, int32_t count )
{
    uint16_t * input = (uint16_t *) src;
    int16_t * output = (int16_t *) dest;
    int16_t sample0l, sample0h, sample0;
    
    do {
        sample0 = *input;
#if 0 //def BIGENDIAN
        sample0l = sample0 >> 8;
        sample0h = (sample0 & 255) ^ 128;
#else
        sample0l = sample0 & 255;
        sample0h = (sample0 >> 8) ^ 128;
#endif
        
        sample0l = ((int16_t *) volume)[sample0l] >> 8;
        sample0h = ((int16_t *) volume)[sample0h];
        *output = (int16_t) (sample0l + sample0h + 128);
        
        input++;
        output++;
    } while (--count > 0);
}

void MV_8BitReverb( int8_t *src, int8_t *dest, VOLUME16 *volume, int32_t count )
{
    uint8_t * input = (uint8_t *) src;
    uint8_t * output = (uint8_t *) dest;
    
    do {
        *output = ((int16_t *) volume)[*input] + 128;
        
        input++;
        output++;
    } while (--count > 0);
}

void MV_16BitReverbFast( char *src, char *dest, int32_t count, int32_t shift )
{
    int16_t * input = (int16_t *) src;
    int16_t * output = (int16_t *) dest;
    
    do {
        *output = *input >> shift;
        
        input++;
        output++;
    } while (--count > 0);
}

void MV_8BitReverbFast( int8_t *src, int8_t *dest, int32_t count, int32_t shift )
{
    uint8_t sample0, c;
    
    c = 128 - (128 >> shift);
    
    do {
        sample0 = *((uint8_t *) src) >> shift;
        *dest = sample0 + c + ((sample0 ^ 128) >> 7);
        
        src++;
        dest++;
    } while (--count > 0);
}

