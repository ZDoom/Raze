//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

#pragma once

#ifndef keyboard_public_h_
#define keyboard_public_h_

#include "baselayer.h"	// for the keyboard stuff
#include "scancodes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t kb_scancode;

#define MAXKEYBOARDSCAN  256

#define KB_KeyDown keystatus
extern kb_scancode KB_LastScan;

#define KB_GetLastScanCode() (KB_LastScan)
#define KB_SetLastScanCode(scancode)                                                                                                       \
    {                                                                                                                                      \
        KB_LastScan = (scancode);                                                                                                          \
    }
#define KB_ClearLastScanCode()                                                                                                             \
    {                                                                                                                                      \
        KB_SetLastScanCode(sc_None);                                                                                                       \
    }
#define KB_KeyPressed(scan) (keystatus[(scan)] != 0)
#define KB_ClearKeyDown(scan)                                                                                                              \
    {                                                                                                                                      \
        keystatus[(scan)] = FALSE;                                                                                                         \
    }
#define KB_UnBoundKeyPressed(scan) (keystatus[(scan)] != 0 && !CONTROL_KeyBinds[scan].cmdstr)
#define KB_GetCh bgetchar
#define KB_KeyWaiting bkbhit
#define KB_FlushKeyboardQueue bflushchars

static inline void KB_ClearKeysDown(void)
{
    KB_LastScan = 0;
    Bmemset(keystatus, 0, sizeof(keystatus));
}

static inline void KB_KeyEvent(int32_t scancode, int32_t keypressed)
{
    if (keypressed)
        KB_LastScan = scancode;
}

static inline void KB_Startup(void) { setkeypresscallback(KB_KeyEvent); }
static inline void KB_Shutdown(void) { setkeypresscallback((void (*)(int32_t, int32_t))NULL); }
const char *  KB_ScanCodeToString( kb_scancode scancode ); // convert scancode into a string
kb_scancode KB_StringToScanCode( const char * string );  // convert a string into a scancode

#ifdef __cplusplus
}
#endif
#endif
