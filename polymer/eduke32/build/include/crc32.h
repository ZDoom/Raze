#ifndef __crc32_h__
#define __crc32_h__

#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif

void initcrc32table(void);

uint32_t crc32once(uint8_t *blk, uint32_t len);

void crc32init(uint32_t *crcvar);
void crc32block(uint32_t *crcvar, uint8_t *blk, uint32_t len);
uint32_t crc32finish(uint32_t *crcvar);

#ifdef __cplusplus
}
#endif

#endif
