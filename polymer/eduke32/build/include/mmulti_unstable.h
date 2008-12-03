#ifndef _MMULTI_UNSTABLE_H_
#define _MMULTI_UNSTABLE_H_

void callcommit(void);
void initcrc(void);
int getcrc(char *buffer, int bufleng);
void initmultiplayers(int argc, char **argv);
void sendpacket(int other, char *bufptr, int messleng);
void setpackettimeout(int datimeoutcount, int daresendagaincount);
void uninitmultiplayers(void);
void sendlogon(void);
void sendlogoff(void);
int  getoutputcirclesize(void);
void setsocket(int newsocket);
int getpacket(int *other, char *bufptr);
void flushpackets(void);
void genericmultifunction(int other, char *bufptr, int messleng, int command);

extern int natfree;

#endif

