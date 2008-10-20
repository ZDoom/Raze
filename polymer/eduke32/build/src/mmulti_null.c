// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jonof@edgenetwk.com)


#include "mmulti.h"


int myconnectindex, numplayers;
int connecthead, connectpoint2[MAXMULTIPLAYERS];
char syncstate = 0;

int isvalidipaddress(char *st)
{
    return 0;
}

int initmultiplayersparms(int argc, char **argv)
{
    return 0;
}

int initmultiplayerscycle(void)
{
    return 0;
}

void initmultiplayers(int argc, char **argv, char damultioption, char dacomrateoption, char dapriority)
{
    numplayers = 1; myconnectindex = 0;
    connecthead = 0; connectpoint2[0] = -1;
}

void setpackettimeout(int datimeoutcount, int daresendagaincount)
{
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

void setsocket(short newsocket)
{
}

void sendpacket(int other, char *bufptr, int messleng)
{
}

int getpacket(int *other, char *bufptr)
{
    return 0;
}

void flushpackets(void)
{
}

void genericmultifunction(int other, char *bufptr, int messleng, int command)
{
}


