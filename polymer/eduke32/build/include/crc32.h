#ifndef __crc32_h__
#define __crc32_h__

#include "compat.h"

#ifdef EXTERNC
extern "C" {
#endif

extern uint32_t crc32table[256];

void initcrc32table(void);

uint32_t crc32once(uint8_t *blk, uint32_t len);

static inline void crc32init(uint32_t *crcvar)
{
    if (!crcvar) return;
    *crcvar = 0xffffffffl;
}

static inline void crc32block(uint32_t *crcvar, uint8_t *blk, uint32_t len)
{
    uint32_t crc = *crcvar;
    while (len--) crc = crc32table[(crc ^ *(blk++)) & 0xffl] ^(crc >> 8);
    *crcvar = crc;
}

static inline uint32_t crc32finish(uint32_t *crcvar)
{
    *crcvar = *crcvar ^ 0xffffffffl;
    return *crcvar;
}

#ifdef EXTERNC
}
#endif

#endif
