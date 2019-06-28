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

#include "compat.h"
#include "common_game.h"
#include "globals.h"
#include "resource.h"


ud_setup_t gSetup;
bool bVanilla = false;
int gFrameClock;
int gFrameTicks;
int gFrame;
int volatile gGameClock;
int gFrameRate;
int gGamma;

char *gVersionString;
char gVersionStringBuf[16];

Resource gSysRes;

static const char *_module;
static int _line;

void _SetErrorLoc(const char *pzFile, int nLine)
{
    _module = pzFile;
    _line = nLine;
}

void _ThrowError(const char *pzFormat, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, pzFormat);
    vsprintf(buffer, pzFormat, args);
    initprintf("%s(%i): %s\n", _module, _line, buffer);

    char titlebuf[256];
    Bsprintf(titlebuf, APPNAME " %s", s_buildRev);
    wm_msgbox(titlebuf, "%s(%i): %s\n", _module, _line, buffer);

    Bfflush(NULL);
    QuitGame();
}

void __dassert(const char * pzExpr, const char * pzFile, int nLine)
{
    initprintf("Assertion failed: %s in file %s at line %i\n", pzExpr, pzFile, nLine);

    char titlebuf[256];
    Bsprintf(titlebuf, APPNAME " %s", s_buildRev);
    wm_msgbox(titlebuf, "Assertion failed: %s in file %s at line %i\n", pzExpr, pzFile, nLine);

    Bfflush(NULL);
    exit(0);
}

const char *GetVersionString(void)
{
    if (!gVersionString)
    {
        gVersionString = gVersionStringBuf;
        if (!gVersionString)
            return NULL;
        sprintf(gVersionString, "%d.%02d", EXEVERSION / 100, EXEVERSION % 100);
    }
    return gVersionString;
}