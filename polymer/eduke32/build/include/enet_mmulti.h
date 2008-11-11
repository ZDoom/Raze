#ifndef _MMULTI_STABLE_H_
#define _MMULTI_STABLE_H_

#define MAXMULTIPLAYERS 16

void callcommit(void);
void initcrc(void);
long getcrc(char *buffer, int bufleng);
void initmultiplayers(int argc, char **argv);
void sendpacket(long other, char *bufptr, long messleng);
void setpackettimeout(long datimeoutcount, long daresendagaincount);
void uninitmultiplayers(void);
void sendlogon(void);
void sendlogoff(void);
int  getoutputcirclesize(void);
void setsocket(int newsocket);
int getpacket(int *other, char *bufptr);
void flushpackets(void);
void genericmultifunction(long other, char *bufptr, long messleng, long command);

extern int natfree;

#endif
