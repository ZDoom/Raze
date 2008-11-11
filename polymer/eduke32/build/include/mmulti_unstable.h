#ifndef _MMULTI_UNSTABLE_H_
#define _MMULTI_UNSTABLE_H_

void callcommit(void);
void initcrc(void);
long getcrc(char *buffer, short bufleng);
void initmultiplayers(int argc, char **argv, char damultioption, char dacomrateoption, char dapriority);
void sendpacket(long other, char *bufptr, long messleng);
void setpackettimeout(long datimeoutcount, long daresendagaincount);
void uninitmultiplayers(void);
void sendlogon(void);
void sendlogoff(void);
int  getoutputcirclesize(void);
void setsocket(short newsocket);
short getpacket(short *other, char *bufptr);
void flushpackets(void);
void genericmultifunction(long other, char *bufptr, long messleng, long command);

#endif