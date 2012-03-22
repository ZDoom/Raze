#include "crc32.h"

uint32_t crc32table[256];

void initcrc32table(void)
{
    uint32_t i,j,k;

    // algorithm and polynomial same as that used by infozip's zip
    for (i=0; i<256; i++)
    {
        j = i;
        for (k=8; k; k--)
            j = (j&1) ? (0xedb88320L^(j>>1)) : (j>>1);
        crc32table[i] = j;
    }
}

uint32_t crc32once(uint8_t *blk, uint32_t len)
{
    uint32_t crc;

    crc32init(&crc);
    crc32block(&crc, blk, len);
    return crc32finish(&crc);
}
