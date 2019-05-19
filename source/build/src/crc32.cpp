// based on http://create.stephan-brumme.com/crc32/Crc32.cpp, zlib license

#include "compat.h"
#include "crc32.h"

uint32_t Bcrc32(const void* data, int length, uint32_t crc)
{
    const uint32_t* current = (const uint32_t*) data;
    uint8_t const * currentChar;
    crc = ~crc;

#ifdef BITNESS64
    // process eight bytes at once (Slicing-by-8)
    while (length >= 8)
    {
#if B_BIG_ENDIAN != 0
        uint32_t one = *current ^ B_SWAP32(crc);
        uint32_t two = *(current+1);
        crc  = crc32table[0][two       & 0xFF] ^
               crc32table[1][(two>> 8) & 0xFF] ^
               crc32table[2][(two>>16) & 0xFF] ^
               crc32table[3][(two>>24) & 0xFF] ^
               crc32table[4][one       & 0xFF] ^
               crc32table[5][(one>> 8) & 0xFF] ^
               crc32table[6][(one>>16) & 0xFF] ^
               crc32table[7][(one>>24) & 0xFF];
#else
        uint32_t one = *current ^ crc;
        uint32_t two = *(current+1);
        crc  = crc32table[0][(two>>24) & 0xFF] ^
               crc32table[1][(two>>16) & 0xFF] ^
               crc32table[2][(two>> 8) & 0xFF] ^
               crc32table[3][two       & 0xFF] ^
               crc32table[4][(one>>24) & 0xFF] ^
               crc32table[5][(one>>16) & 0xFF] ^
               crc32table[6][(one>> 8) & 0xFF] ^
               crc32table[7][one       & 0xFF];
#endif
        current += 2;
        length -= 8;
    }
#else
    // process four bytes at once (Slicing-by-4)
    while (length >= 4)
    {
#if B_BIG_ENDIAN != 0
        uint32_t one = *current++ ^ B_SWAP32(crc);
        crc  = crc32table[0][one       & 0xFF] ^
               crc32table[1][(one>> 8) & 0xFF] ^
               crc32table[2][(one>>16) & 0xFF] ^
               crc32table[3][(one>>24) & 0xFF];
#else
        uint32_t one = *current++ ^ crc;
        crc  = crc32table[0][(one>>24) & 0xFF] ^
               crc32table[1][(one>>16) & 0xFF] ^
               crc32table[2][(one>> 8) & 0xFF] ^
               crc32table[3][one       & 0xFF];
#endif

        length -= 4;
    }
#endif

    currentChar = (uint8_t const *) current;
    // remaining 1 to 7 bytes (standard algorithm)
    while (length-- > 0)
        crc = (crc >> 8) ^ crc32table[0][(crc & 0xFF) ^ *currentChar++];

    return ~crc;
}

#ifdef BITNESS64
uint32_t crc32table[8][256];
#else
uint32_t crc32table[4][256];
#endif

void initcrc32table(void)
{
    int i;
    for (i = 0; i <= 0xFF; i++)
    {
      uint32_t j, crc = i;
      for (j = 0; j < 8; j++)
        crc = (crc >> 1) ^ ((crc & 1) * POLY);
      crc32table[0][i] = crc;
    }

    for (i = 0; i <= 0xFF; i++)
    {
      crc32table[1][i] = (crc32table[0][i] >> 8) ^ crc32table[0][crc32table[0][i] & 0xFF];
      crc32table[2][i] = (crc32table[1][i] >> 8) ^ crc32table[0][crc32table[1][i] & 0xFF];
      crc32table[3][i] = (crc32table[2][i] >> 8) ^ crc32table[0][crc32table[2][i] & 0xFF];

#ifdef BITNESS64
      crc32table[4][i] = (crc32table[3][i] >> 8) ^ crc32table[0][crc32table[3][i] & 0xFF];
      crc32table[5][i] = (crc32table[4][i] >> 8) ^ crc32table[0][crc32table[4][i] & 0xFF];
      crc32table[6][i] = (crc32table[5][i] >> 8) ^ crc32table[0][crc32table[5][i] & 0xFF];
      crc32table[7][i] = (crc32table[6][i] >> 8) ^ crc32table[0][crc32table[6][i] & 0xFF];
#endif
    }
}
