#include "multivoc.h"
// #include "_multivc.h"

void MV_16BitReverb(char *src, char *dest, VOLUME16 *volume, int count)
{
    int i;

    short *pdest = (short *)dest;

    for (i = 0; i < count; i++)
    {
#if PLATFORM_BIGENDIAN
        int sl = src[i*2+1];
        int sh = src[i*2+0] ^ 0x80;
#else
        int sl = src[i*2+0];
        int sh = src[i*2+1] ^ 0x80;
#endif

        sl = (*volume)[sl] >> 8;
        sh = (*volume)[sh];

        pdest[i] = (short)(sl + sh + 0x80);
    }
}

void MV_8BitReverb(signed char *src, signed char *dest, VOLUME16 *volume, int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        unsigned char s = (unsigned char) src[i];

        s = (*volume)[s] & 0xff;

        dest[i] = (char)(s + 0x80);
    }
}

void MV_16BitReverbFast(char *src, char *dest, int count, int shift)
{
    int i;

    short *pdest = (short *)dest;
    const short *psrc = (const short *)src;

    for (i = 0; i < count; i++)
    {
        pdest[i] = psrc[i] >> shift;
    }
}

void MV_8BitReverbFast(signed char *src, signed char *dest, int count, int shift)
{
    int i;

    unsigned char sh = 0x80 - (0x80 >> shift);

    for (i = 0; i < count; i++)
    {
        unsigned char a = ((unsigned char) src[i]) >> shift;
        unsigned char c = (((unsigned char) src[i]) ^ 0x80) >> 7;

        dest[i] = (signed char)(a + sh + c);
    }
}
