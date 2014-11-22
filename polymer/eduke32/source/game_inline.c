//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#ifndef game_inline_c_
#define game_inline_c_

#include "compat.h"
#include "duke3d.h"
#include "premap.h"
#include "game.h"
#include "game_inline.h"

EXTERN_INLINE void G_SetStatusBarScale(int32_t sc)
{
    ud.statusbarscale = clamp(sc, 36, 100);
    G_UpdateScreenArea();
}

// the point of this is to prevent re-running a function or calculation passed to potentialValue
// without making a new variable under each individual circumstance
EXTERN_INLINE void SetIfGreater(int32_t *variable, int32_t potentialValue)
{
    if (potentialValue > *variable)
        *variable = potentialValue;
}

#endif
