//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "getopt.h"

int margc;
char const * const *margv;

const char *OptArgv[16];
int OptArgc;
const char *OptFull;
const char *SwitchChars = "-/";

int GetOptions(SWITCH *switches)
{
    static const char *pChar = NULL;
    static int OptIndex = 1;
    if (!pChar)
    {
        if (OptIndex >= margc)
            return -1;
        pChar = margv[OptIndex++];
        if (!pChar)
            return -1;
    }
    OptFull = pChar;
    if (!strchr(SwitchChars, *pChar))
    {
        pChar = NULL;
        return -2;
    }
    pChar++;
    int i;
    int vd;
    for (i = 0; true; i++)
    {
        if (!switches[i].name)
            return -3;
        int nLength = strlen(switches[i].name);
        if (!Bstrncasecmp(pChar, switches[i].name, nLength) && (pChar[nLength]=='=' || pChar[nLength]==0))
        {
            pChar += nLength;
            if (*pChar=='=')
            {
                pChar++;
            }
            else
            {
                pChar = NULL;
            }
            break;
        }
    }
    vd = switches[i].at4;
    OptArgc = 0;
    while (OptArgc < switches[i].at8)
    {
        if (!pChar)
        {
            if (OptIndex >= margc)
                break;
            pChar = margv[OptIndex++];
            if (strchr(SwitchChars, *pChar) != 0)
                break;
        }
        OptArgv[OptArgc++] = pChar;
        pChar = NULL;
    }
    return vd;
}
