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

#ifndef rts_private__
#define rts_private__

#include "cache1d.h"
#include "vfs.h"

//===============
//   TYPES
//===============

typedef struct
{
    char name[8];
    buildvfs_kfd handle;
    int32_t position, size;
} lumpinfo_t;

typedef struct
{
    char identification[4];  // should be "IWAD"
    int32_t numlumps;
    int32_t infotableofs;
} wadinfo_t;

typedef struct
{
    int32_t filepos;
    int32_t size;
    char name[8];
} filelump_t;

#endif
