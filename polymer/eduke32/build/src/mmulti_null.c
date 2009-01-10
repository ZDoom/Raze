// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jonof@edgenetwk.com)


#include "mmulti.h"


int32_t myconnectindex, numplayers;
int32_t connecthead, connectpoint2[MAXMULTIPLAYERS];
char syncstate = 0;

int32_t isvalidipaddress(char *st)
{
    return 0;
}

int32_t initmultiplayersparms(int32_t argc, char **argv)
{
    return 0;
}

int32_t initmultiplayerscycle(void)
{
    return 0;
}

void mmulti_initmultiplayers(int32_t argc, char **argv, char damultioption, char dacomrateoption, char dapriority)
{
    numplayers = 1; myconnectindex = 0;
    connecthead = 0; connectpoint2[0] = -1;
}

void mmulti_setpackettimeout(int32_t datimeoutcount, int32_t daresendagaincount)
{
}

void mmulti_uninitmultiplayers(void)
{
}

void mmulti_sendlogon(void)
{
}

void mmulti_sendlogoff(void)
{
}

int32_t mmulti_getoutputcirclesize(void)
{
    return 0;
}

void mmulti_sendpacket(int32_t other, char *bufptr, int32_t messleng)
{
}

int32_t mmulti_getpacket(int32_t *other, char *bufptr)
{
    return 0;
}

void mmulti_flushpackets(void)
{
}

void mmulti_generic(int32_t other, char *bufptr, int32_t messleng, int32_t command)
{
}


