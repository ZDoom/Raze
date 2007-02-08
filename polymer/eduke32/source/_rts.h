//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2004, 2007 - EDuke32 developers

This file is part of EDuke32

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

#ifndef __rts_private__
#define __rts_private__

//===============
//   TYPES
//===============

typedef struct
   {
   char name[8];
   int32 handle,position,size;
   } lumpinfo_t;

typedef struct
   {
   char identification[4];              // should be IWAD
   int32 numlumps;
   int32 infotableofs;
   } wadinfo_t;

typedef struct
   {
   int32 filepos;
   int32 size;
   char name[8];
   } filelump_t;

#endif
