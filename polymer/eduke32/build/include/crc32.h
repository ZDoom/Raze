#ifndef __crc32_h__
#define __crc32_h__

#ifdef __cplusplus
extern "C" {
#endif

void initcrc32table(void);

unsigned int crc32once(unsigned char *blk, unsigned int len);

void crc32init(unsigned int *crcvar);
void crc32block(unsigned int *crcvar, unsigned char *blk, unsigned int len);
unsigned int crc32finish(unsigned int *crcvar);

#ifdef __cplusplus
}
#endif

#endif
