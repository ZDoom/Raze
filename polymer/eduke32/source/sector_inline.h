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

#ifndef sector_inline_h_
#define sector_inline_h_

#ifdef __cplusplus
extern "C" {
#endif

EXTERN_INLINE_HEADER int32_t G_CheckPlayerInSector(int32_t sect);

#ifdef __cplusplus
}
#endif

#ifndef DISABLE_INLINING
#include "sector_inline.c"
#endif

#endif
