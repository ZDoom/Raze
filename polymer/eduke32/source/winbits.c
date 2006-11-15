//-------------------------------------------------------------------------
/*
Duke Nukem Copyright (C) 1996, 2003 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Windows-specific hooks for JonoF's Duke3D port.
*/
//-------------------------------------------------------------------------

#ifdef RENDERTYPEWIN

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "winlayer.h"


int Win_YesNoBox(char *name, char *fmt, ...)
{
    char buf[1000];
    va_list va;
    int r;

    va_start(va,fmt);
    vsprintf(buf,fmt,va);
    va_end(va);

    r = MessageBox((HWND)win_gethwnd(),buf,name,MB_YESNO|MB_TASKMODAL);
    if (r==IDYES) return 'y';
    return 'n';
}

int Win_MsgBox(char *name, char *fmt, ...)
{
    char buf[1000];
    va_list va;

    va_start(va,fmt);
    vsprintf(buf,fmt,va);
    va_end(va);

    MessageBox((HWND)win_gethwnd(),buf,name,MB_OK|MB_TASKMODAL);
    return 'y';
}


// this replaces the Error() function in jmact/util_lib.c
extern void Shutdown(void);	// game.c
void Error(char *error, ...)
{
    char buf[1000];
    va_list va;

    Shutdown();

    if (error)
    {
        va_start(va, error);
        vsprintf(buf, error, va);
        va_end(va);

        MessageBox((HWND)win_gethwnd(),buf,"Fatal Error",MB_OK|MB_TASKMODAL);
    }

    exit((error != NULL));
}


#endif
