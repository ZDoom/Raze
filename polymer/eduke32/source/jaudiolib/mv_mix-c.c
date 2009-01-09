#include "multivoc.h"

extern char  *MV_MixDestination;
extern uint32_t MV_MixPosition;

extern char *MV_LeftVolume;
extern char *MV_RightVolume;

extern char *MV_HarshClipTable;

extern int32_t MV_RightChannelOffset;
extern int32_t MV_SampleSize;

void MV_Mix8BitMono(uint32_t position, uint32_t rate,
                    const char *start, uint32_t length)
{
    const char *src;
    char *dest;
    uint32_t i;

    src = (const char *)start;
    dest = (char *)MV_MixDestination;

    for (i = 0; i < length; i++)
    {
        int32_t s = src[position >> 16];
        int32_t d = *dest;

        s = MV_LeftVolume[s * 2];

        s += d;

        s = MV_HarshClipTable[s + 0x80];

        *dest = (s & 0xff);

        position += rate;
        dest += MV_SampleSize;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *)dest;
}

void MV_Mix8BitStereo(uint32_t position,
                      uint32_t rate, const char *start, uint32_t length)
{
    const char *src;
    char *dest;
    uint32_t i;

    src = (const char *)start;
    dest = (char *)MV_MixDestination;

    for (i = 0; i < length; i++)
    {
        int32_t s = src[(position >> 16)];
        int32_t dl = dest[0];
        int32_t dr = dest[MV_RightChannelOffset];

        dl += MV_LeftVolume[s * 2];
        dr += MV_RightVolume[s * 2];

        dl = MV_HarshClipTable[dl + 0x80];
        dr = MV_HarshClipTable[dr + 0x80];

        dest[0] = (dl & 0xff);
        dest[MV_RightChannelOffset] = (dr & 0xff);

        position += rate;
        dest += MV_SampleSize;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *)dest;
}

void MV_Mix16BitMono(uint32_t position,
                     uint32_t rate, const char *start, uint32_t length)
{
    const int16_t *MV_LeftVolumeS;
    const char *src;
    int16_t *dest;
    uint32_t i;

    src = (const char *)start;
    dest = (int16_t *)MV_MixDestination;

    MV_LeftVolumeS = (const int16_t *)MV_LeftVolume;

    for (i = 0; i < length; i++)
    {
        int32_t s = src[position >> 16];
        int32_t d = dest[0];

        s = MV_LeftVolumeS[s];

        s += d;

        if (s < -32768) s = -32768;
        if (s >  32767) s =  32767;

        *dest = (int16_t) s;

        position += rate;
        dest += MV_SampleSize/2;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *)dest;
}

void MV_Mix16BitStereo(uint32_t position,
                       uint32_t rate, const char *start, uint32_t length)
{
    const int16_t *MV_LeftVolumeS;
    const int16_t *MV_RightVolumeS;
    const char *src;
    int16_t *dest;
    uint32_t i;

    src = (char *)start;
    dest = (int16_t *)MV_MixDestination;

    MV_LeftVolumeS = (const int16_t *)MV_LeftVolume;
    MV_RightVolumeS = (const int16_t *)MV_RightVolume;

    for (i = 0; i < length; i++)
    {
        int32_t s = src[position >> 16];
        int32_t dl = dest[0];
        int32_t dr = dest[MV_RightChannelOffset/2];

        dl += MV_LeftVolumeS[s];
        dr += MV_RightVolumeS[s];

        if (dl < -32768) dl = -32768;
        if (dl >  32767) dl =  32767;
        if (dr < -32768) dr = -32768;
        if (dr >  32767) dr =  32767;

        dest[0] = (int16_t) dl;
        dest[MV_RightChannelOffset/2] = (int16_t) dr;

        position += rate;
        dest += MV_SampleSize/2;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *)dest;
}

void MV_Mix8BitMono16(uint32_t position, uint32_t rate,
                      const char *start, uint32_t length)
{
    const char *src;
    char *dest;
    uint32_t i;

    src = (const char *)start + 1;
    dest = (char *)MV_MixDestination;

    for (i = 0; i < length; i++)
    {
        int32_t s = (int32_t)src[(position >> 16) * 2] + 0x80;
        int32_t d = *dest;

        s = MV_LeftVolume[s * 2];

        s += d;

        s = MV_HarshClipTable[s + 0x80];

        *dest = (s & 0xff);

        position += rate;
        dest += MV_SampleSize;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *)dest;
}

void MV_Mix8BitStereo16(uint32_t position,
                        uint32_t rate, const char *start, uint32_t length)
{
    const char *src;
    char *dest;
    uint32_t i;

    src = (const char *)start + 1;
    dest = (char *)MV_MixDestination;

    for (i = 0; i < length; i++)
    {
        int32_t s = src[(position >> 16) * 2] + 0x80;
        int32_t dl = dest[0];
        int32_t dr = dest[MV_RightChannelOffset];

        dl += MV_LeftVolume[s * 2];
        dr += MV_RightVolume[s * 2];

        dl = MV_HarshClipTable[dl + 0x80];
        dr = MV_HarshClipTable[dr + 0x80];

        dest[0] = (dl & 0xff);
        dest[MV_RightChannelOffset] = (dr & 0xff);

        position += rate;
        dest += MV_SampleSize;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *)dest;
}

void MV_Mix16BitMono16(uint32_t position,
                       uint32_t rate, const char *start, uint32_t length)
{
    const int16_t *MV_LeftVolumeS;
    const char *src;
    int16_t *dest;
    uint32_t i;

    src = (const char *)start;
    dest = (int16_t *)MV_MixDestination;

    MV_LeftVolumeS = (const int16_t *)MV_LeftVolume;

    for (i = 0; i < length; i++)
    {
        int32_t sl = src[(position >> 16) * 2 + 0];
        int32_t sh = src[(position >> 16) * 2 + 1] ^ 0x80;

        int32_t d = *dest;

        sl = MV_LeftVolume[sl * 2 + 1];
        sh = MV_LeftVolumeS[sh];

        d = sl + sh + 0x80 + d;

        if (d < -32768) d = -32768;
        if (d >  32767) d =  32767;

        *dest = (int16_t) d;

        position += rate;
        dest += MV_SampleSize/2;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *)dest;
}

void MV_Mix16BitStereo16(uint32_t position,
                         uint32_t rate, const char *start, uint32_t length)
{
    const int16_t *MV_LeftVolumeS;
    const int16_t *MV_RightVolumeS;
    const char *src;
    int16_t *dest;
    uint32_t i;

    src = (const char *)start;
    dest = (int16_t *)MV_MixDestination;

    MV_LeftVolumeS = (const int16_t *)MV_LeftVolume;
    MV_RightVolumeS = (const int16_t *)MV_RightVolume;

    for (i = 0; i < length; i++)
    {
        int32_t sl = src[(position >> 16) * 2 + 0];
        int32_t sh = src[(position >> 16) * 2 + 1] ^ 0x80;

        int32_t dl = dest[0];
        int32_t dr = dest[MV_RightChannelOffset/2];

        int32_t sll = MV_LeftVolume[sl * 2 + 1];
        int32_t slh = MV_LeftVolumeS[sh];

        int32_t srl = MV_RightVolume[sl * 2 + 1];
        int32_t srh = MV_RightVolumeS[sh];

        dl = sll + slh + 0x80 + dl;
        dr = srl + srh + 0x80 + dr;

        if (dl < -32768) dl = -32768;
        if (dl >  32767) dl =  32767;
        if (dr < -32768) dr = -32768;
        if (dr >  32767) dr =  32767;

        dest[0] = (int16_t) dl;
        dest[MV_RightChannelOffset/2] = (int16_t) dr;

        position += rate;
        dest += MV_SampleSize/2;
    }

    MV_MixPosition = position;
    MV_MixDestination = (char *)dest;
}
