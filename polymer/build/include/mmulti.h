// mmulti.h

#ifndef __mmulti_h__
#define __mmulti_h__

#define MAXMULTIPLAYERS 16

extern long myconnectindex, numplayers;
extern long connecthead, connectpoint2[MAXMULTIPLAYERS];
extern char syncstate;

long initmultiplayersparms(long argc, char **argv);
long initmultiplayerscycle(void);

void initmultiplayers(long argc, char **argv, char damultioption, char dacomrateoption, char dapriority);
void setpackettimeout(long datimeoutcount, long daresendagaincount);
void uninitmultiplayers(void);
void sendlogon(void);
void sendlogoff(void);
long getoutputcirclesize(void);
void setsocket(short newsocket);
void sendpacket(long other, char *bufptr, long messleng);
long getpacket(long *other, char *bufptr);
void flushpackets(void);
void genericmultifunction(long other, char *bufptr, long messleng, long command);
long isvalidipaddress(char *st);

#endif	// __mmulti_h__

