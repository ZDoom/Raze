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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#ifndef __actors_inline_h__
#define __actors_inline_h__

#if KRANDDEBUG
# define ACTOR_INLINE
# define ACTOR_INLINE_HEADER extern
#else
# define ACTOR_INLINE EXTERN_INLINE
# define ACTOR_INLINE_HEADER EXTERN_INLINE_HEADER
#endif

ACTOR_INLINE_HEADER int32_t A_SetSprite(int32_t i,uint32_t cliptype);

EXTERN_INLINE_HEADER void G_UpdateInterpolations(void);
EXTERN_INLINE_HEADER void G_RestoreInterpolations(void);

EXTERN_INLINE_HEADER int32_t G_CheckForSpaceCeiling(int32_t sectnum);
EXTERN_INLINE_HEADER int32_t G_CheckForSpaceFloor(int32_t sectnum);

EXTERN_INLINE_HEADER int32_t A_CheckEnemySprite(const spritetype *s);

#ifndef DISABLE_INLINING
#include "actors_inline.c"
#endif

#endif
