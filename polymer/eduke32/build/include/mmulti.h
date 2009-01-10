// mmulti.h

#ifndef __mmulti_h__
#define __mmulti_h__

#define MAXMULTIPLAYERS 16

extern int32_t myconnectindex, numplayers;
extern int32_t connecthead, connectpoint2[MAXMULTIPLAYERS];
extern char syncstate;
extern int32_t natfree; //Addfaz NatFree

int32_t initmultiplayersparms(int32_t argc, char **argv);
int32_t initmultiplayerscycle(void);

void mmulti_initmultiplayers(int32_t argc, char **argv);
void mmulti_setpackettimeout(int32_t datimeoutcount, int32_t daresendagaincount);
void mmulti_uninitmultiplayers(void);
void mmulti_sendlogon(void);
void mmulti_sendlogoff(void);
int32_t mmulti_getoutputcirclesize(void);
void mmulti_sendpacket(int32_t other, char *bufptr, int32_t messleng);
int32_t mmulti_getpacket(int32_t *other, char *bufptr);
void mmulti_flushpackets(void);
void mmulti_generic(int32_t other, char *bufptr, int32_t messleng, int32_t command);
int32_t isvalidipaddress(char *st);

void nfIncCP(void); //Addfaz NatFree
int32_t nfCheckHF (int32_t other); //Addfaz NatFree
int32_t nfCheckCP(int32_t other); //Addfaz NatFree

#endif	// __mmulti_h__

