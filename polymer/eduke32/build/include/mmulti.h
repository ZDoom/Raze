// mmulti.h

#ifndef mmulti_h_
#define mmulti_h_

#define MAXMULTIPLAYERS 16

extern int myconnectindex, numplayers;
extern int connecthead, connectpoint2[MAXMULTIPLAYERS];
extern unsigned char syncstate;

int initmultiplayersparms(int argc, char const * const argv[]);
int initmultiplayerscycle(void);

void initmultiplayers(int argc, char const * const argv[], unsigned char damultioption, unsigned char dacomrateoption, unsigned char dapriority);
void setpackettimeout(int datimeoutcount, int daresendagaincount);
void uninitmultiplayers(void);
void sendlogon(void);
void sendlogoff(void);
int getoutputcirclesize(void);
void setsocket(int newsocket);
void sendpacket(int other, unsigned char *bufptr, int messleng);
int getpacket(int *other, unsigned char *bufptr);
void flushpackets(void);
void genericmultifunction(int other, unsigned char *bufptr, int messleng, int command);
int isvalidipaddress(const char *st);

#endif  // mmulti_h_

