#ifndef _MMULTI_UNSTABLE_H_
#define _MMULTI_UNSTABLE_H_

#include "compat.h"

void callcommit(void);
void initcrc(void);
int32_t getcrc(char *buffer, int32_t bufleng);
void mmulti_initmultiplayers(int32_t argc, char **argv);
void mmulti_sendpacket(int32_t other, char *bufptr, int32_t messleng);
void mmulti_setpackettimeout(int32_t datimeoutcount, int32_t daresendagaincount);
void mmulti_uninitmultiplayers(void);
void sendlogon(void);
void sendlogoff(void);
int32_t  getoutputcirclesize(void);
void setsocket(int32_t newsocket);
int32_t mmulti_getpacket(int32_t *other, char *bufptr);
void mmulti_flushpackets(void);
void genericmultifunction(int32_t other, char *bufptr, int32_t messleng, int32_t command);

extern int32_t natfree;

#endif

