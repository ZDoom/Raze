/*
 * util_lib.c
 * Utility functions to emulate MACT
 *
 * by Jonathon Fowler
 *
 * Since we weren't given the source for MACT386.LIB so I've had to do some
 * creative interpolation here.
 *
 */
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
*/
//-------------------------------------------------------------------------

#include "compat.h"

#include "util_lib.h"
#include "baselayer.h"

//#define MOTOROLA


static void (*ShutDown)(void) = NULL;	// this is defined by whoever links us


void RegisterShutdownFunction(void (* sh)(void))
{
    ShutDown = sh;
}

#ifndef RENDERTYPEWIN
void Error(char *error, ...)
{
    va_list va;

    if (ShutDown) ShutDown();

    if (error)
    {
        va_start(va, error);
        vprintf(error, va);
        va_end(va);
        printf("\n\n");
    }

    exit((error != NULL));
}
#endif

char CheckParm(char *check)
{
    int32_t c;

    for (c=1; c<_buildargc; c++)
    {
        if (_buildargv[c][0] == '/' || _buildargv[c][0] == '-')
            if (!Bstrcasecmp(&_buildargv[c][1], check)) return c;
    }

    return 0;
}

int32_t ParseHex(char *hex)
{
    return strtol(hex, NULL, 16);
}

int32_t ParseNum(char *str)
{
    return strtol(str, NULL, 10);
}

int16_t MotoShort(int16_t l)
{
#if B_LITTLE_ENDIAN != 0
    return l;
#else
    return ((l & 0x00ff) << 8) | ((l & 0xff00) >> 8);
#endif
}

int16_t  IntelShort(int16_t l)
{
#if B_BIG_ENDIAN != 0
    return ((l & 0x00ff) << 8) | ((l & 0xff00) >> 8);
#else
    return l;
#endif
}

int32_t  MotoLong(int32_t l)
{
#if B_LITTLE_ENDIAN != 0
    return l;
#else
    int32_t t = ((l & 0x00ff00ffl) << 8) | ((l & 0xff00ff00l) >> 8);
    return ((t & 0x0000ffffl) << 16) | ((t & 0xffff0000l) >> 16);
#endif
}

int32_t  IntelLong(int32_t l)
{
#if B_BIG_ENDIAN != 0
    int32_t t = ((l & 0x00ff00ffl) << 8) | ((l & 0xff00ff00l) >> 8);
    return ((t & 0x0000ffffl) << 16) | ((t & 0xffff0000l) >> 16);
#else
    return l;
#endif
}

