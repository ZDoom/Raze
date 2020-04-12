//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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

#ifndef __engine_h__
#define __engine_h__

#include "compat.h"
#include "build.h"
#include "pragmas.h"
#include "typedefs.h"
#include "trigdat.h"

BEGIN_PS_NS

#define kMaxTiles	6144
#define kMaxSprites 4096
#define kMaxSectors 1024
#define kMaxWalls   8192
#define kMaxTiles   6144
#define kMaxVoxels	4096

enum
{
	kStatIgnited = 404
};

#define kMaxSpritesOnscreen		1024

#define kMaxPalookups 256
#define kMaxStatus   1024
//#define MAXPSKYTILES 256

inline int Sin(int angle)
{
    return sintable[angle & kAngleMask];
}

inline int Cos(int angle)
{
    return sintable[(angle + 512) & kAngleMask];
}

int movesprite(short spritenum, int dx, int dy, int dz, int ceildist, int flordist, unsigned int clipmask);
void overwritesprite(int thex, int they, short tilenum, signed char shade, char stat, char dapalnum, int basepal = 0);
void precache();
void resettiming();
void printext(int x, int y, const char* buffer, short tilenum, char invisiblecol);

END_PS_NS

#endif
