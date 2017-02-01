// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)

#include "compat.h"
#include "mmulti.h"


int myconnectindex, numplayers;
int connecthead, connectpoint2[MAXMULTIPLAYERS] = { -1 };
unsigned char syncstate = 0;

int isvalidipaddress (const char *st)
{
    UNREFERENCED_PARAMETER(st);
    return 0;
}

int initmultiplayersparms(int argc, char const * const argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    return 0;
}

int initmultiplayerscycle(void)
{
    return 0;
}

void initmultiplayers(int argc, char const * const argv[], unsigned char damultioption, unsigned char dacomrateoption, unsigned char dapriority)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(damultioption);
    UNREFERENCED_PARAMETER(dacomrateoption);
    UNREFERENCED_PARAMETER(dapriority);

    numplayers = 1; myconnectindex = 0;
    connecthead = 0; connectpoint2[0] = -1;
}

void setpackettimeout(int datimeoutcount, int daresendagaincount)
{
    UNREFERENCED_PARAMETER(datimeoutcount);
    UNREFERENCED_PARAMETER(daresendagaincount);
}

void uninitmultiplayers(void)
{
}

void sendlogon(void)
{
}

void sendlogoff(void)
{
}

int getoutputcirclesize(void)
{
    return 0;
}

void setsocket(int newsocket)
{
    UNREFERENCED_PARAMETER(newsocket);
}

void sendpacket(int other, unsigned char *bufptr, int messleng)
{
    UNREFERENCED_PARAMETER(other);
    UNREFERENCED_PARAMETER(bufptr);
    UNREFERENCED_PARAMETER(messleng);
}

int getpacket (int *other, unsigned char *bufptr)
{
    UNREFERENCED_PARAMETER(other);
    UNREFERENCED_PARAMETER(bufptr);

    return 0;
}

void flushpackets(void)
{
}

void genericmultifunction(int other, unsigned char *bufptr, int messleng, int command)
{
    UNREFERENCED_PARAMETER(other);
    UNREFERENCED_PARAMETER(bufptr);
    UNREFERENCED_PARAMETER(messleng);
    UNREFERENCED_PARAMETER(command);
}


