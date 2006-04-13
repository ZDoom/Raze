// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jonof@edgenetwk.com)


#include "mmulti.h"


long myconnectindex, numplayers;
long connecthead, connectpoint2[MAXMULTIPLAYERS];
char syncstate = 0;

long isvalidipaddress (char *st)
{
	return 0;
}

long initmultiplayersparms(long argc, char **argv)
{
	return 0;
}

long initmultiplayerscycle(void)
{
	return 0;
}

void initmultiplayers(long argc, char **argv, char damultioption, char dacomrateoption, char dapriority)
{
	numplayers = 1; myconnectindex = 0;
	connecthead = 0; connectpoint2[0] = -1;
}

void setpackettimeout(long datimeoutcount, long daresendagaincount)
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

long getoutputcirclesize(void)
{
	return 0;
}

void setsocket(short newsocket)
{
}

void sendpacket(long other, char *bufptr, long messleng)
{
}

long getpacket (long *other, char *bufptr)
{
	return 0;
}

void flushpackets(void)
{
}

void genericmultifunction(long other, char *bufptr, long messleng, long command)
{
}


