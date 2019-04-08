// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#ifndef mmulti_h_
#define mmulti_h_

#define MAXMULTIPLAYERS 16

extern int myconnectindex, numplayers;
extern int connecthead, connectpoint2[MAXMULTIPLAYERS];
extern unsigned char syncstate;

#if 0
int initmultiplayersparms(int argc, char const * const * argv);
int initmultiplayerscycle(void);
void initmultiplayers(int argc, char const * const * argv, unsigned char damultioption, unsigned char dacomrateoption, unsigned char dapriority);
#else
static inline int initmultiplayersparms(int argc, char const * const * argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    return 0;
}
static inline int initmultiplayerscycle(void)
{
    return 0;
}
static inline void initmultiplayers(int argc, char const * const * argv, unsigned char damultioption, unsigned char dacomrateoption, unsigned char dapriority)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(damultioption);
    UNREFERENCED_PARAMETER(dacomrateoption);
    UNREFERENCED_PARAMETER(dapriority);
}
#endif

void setpackettimeout(int datimeoutcount, int daresendagaincount);
void uninitmultiplayers(void);
void sendlogon(void);
void sendlogoff(void);
int getoutputcirclesize(void);
void setsocket(int newsocket);
void sendpacket(int other, const unsigned char *bufptr, int messleng);
int getpacket(const int *other, const unsigned char *bufptr);
void flushpackets(void);
void genericmultifunction(int other, const unsigned char *bufptr, int messleng, int command);
int isvalidipaddress(const char *st);

#endif  // mmulti_h_

