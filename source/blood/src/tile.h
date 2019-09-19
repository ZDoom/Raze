//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#pragma once
#include "build.h"
#include "common_game.h"
#include "blood.h"

enum SurfaceType {
    kSurf0 = 0,
    kSurf1,
    kSurf2,
    kSurf3,
    kSurf4,
    kSurf5,
    kSurf6,
    kSurf7,
    kSurf8,
    kSurf9,
    kSurf10,
    kSurf11,
    kSurf12,
    kSurf13,
    kSurf14,
    kSurfMax
};

extern char surfType[kMaxTiles];
extern signed char tileShade[kMaxTiles];
extern short voxelIndex[kMaxTiles];

void qloadvoxel(int32_t nVoxel);
void CalcPicsiz(int a1, int a2, int a3);
int tileInit(char a1, const char *a2);
#ifdef USE_OPENGL
void tileProcessGLVoxels(void);
#endif
char * tileLoadTile(int nTile);
char * tileAllocTile(int nTile, int x, int y, int ox, int oy);
void tilePreloadTile(int nTile);
void tilePrecacheTile(int nTile, int nType = 1);
char tileGetSurfType(int hit);
