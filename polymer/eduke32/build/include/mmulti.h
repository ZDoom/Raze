// mmulti.h

#ifndef __mmulti_h__
#define __mmulti_h__

#define MAXMULTIPLAYERS 16

extern int myconnectindex, numplayers;
extern int connecthead, connectpoint2[MAXMULTIPLAYERS];
extern char syncstate;
extern int natfree; //Addfaz NatFree

int initmultiplayersparms(int argc, char **argv);
int initmultiplayerscycle(void);

void initmultiplayers(int argc, char **argv);
void setpackettimeout(int datimeoutcount, int daresendagaincount);
void uninitmultiplayers(void);
void sendlogon(void);
void sendlogoff(void);
int getoutputcirclesize(void);
void setsocket(short newsocket);
void sendpacket(int other, char *bufptr, int messleng);
int getpacket(int *other, char *bufptr);
void flushpackets(void);
void genericmultifunction(int other, char *bufptr, int messleng, int command);
int isvalidipaddress(char *st);

void nfIncCP(void); //Addfaz NatFree
int nfCheckHF (int other); //Addfaz NatFree
int nfCheckCP(int other); //Addfaz NatFree

#endif	// __mmulti_h__

