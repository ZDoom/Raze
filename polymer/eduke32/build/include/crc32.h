#ifndef __crc32_h__
#define __crc32_h__

#include "compat.h"

#ifdef EXTERNC
extern "C" {
#endif

#define POLY 0xEDB88320

#ifdef BITNESS64
extern uint32_t crc32table[8][256];
#else
extern uint32_t crc32table[4][256];
#endif

extern uint32_t crc32(const void* data, size_t length, uint32_t crc);
extern void initcrc32table(void);

#ifdef EXTERNC
}
#endif

#endif
