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

#ifndef game_inline_h_
#define game_inline_h_

#ifdef __cplusplus
extern "C" {
#endif

EXTERN_INLINE_HEADER void G_SetStatusBarScale(int32_t sc);

EXTERN_INLINE_HEADER void SetIfGreater(int32_t *variable, int32_t potentialValue);

#ifdef __cplusplus
}
#endif

#ifndef DISABLE_INLINING
#include "game_inline.c"
#endif

#endif
