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

#ifndef mouse_h_
#define mouse_h_
#ifdef __cplusplus
extern "C" {
#endif

#define LEFT_MOUSE      1
#define RIGHT_MOUSE     2
#define MIDDLE_MOUSE    4
#define THUMB_MOUSE     8
#define WHEELUP_MOUSE   16
#define WHEELDOWN_MOUSE 32

#define LEFT_MOUSE_PRESSED(button)      (((button)&LEFT_MOUSE) != 0)
#define RIGHT_MOUSE_PRESSED(button)     (((button)&RIGHT_MOUSE) != 0)
#define MIDDLE_MOUSE_PRESSED(button)    (((button)&MIDDLE_MOUSE) != 0)

#include "baselayer.h"

static inline bool Mouse_Init(void)
{
    mouseInit();
    return ((inputdevices & 2) == 2);
}

static inline void MOUSE_Shutdown(void) { mouseUninit(); }
static inline int32_t MOUSE_GetButtons(void) { return mouseReadButtons(); }
static inline void MOUSE_ClearButton(int32_t b) { g_mouseBits &= ~b; }
static inline void MOUSE_ClearAllButtons(void) { g_mouseBits = 0; }

#ifdef __cplusplus
}
#endif
#endif /* __mouse_h */
