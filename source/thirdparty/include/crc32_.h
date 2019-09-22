#ifndef crc32_h_
#define crc32_h_

#include "compat.h"

#define POLY 0xEDB88320

#ifdef BITNESS64
extern uint32_t crc32table[8][256];
#else
extern uint32_t crc32table[4][256];
#endif

extern uint32_t Bcrc32(const void* data, int length, uint32_t crc);
extern void initcrc32table(void);

#endif
