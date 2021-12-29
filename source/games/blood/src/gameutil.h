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

BEGIN_BLD_NS

enum {
	PARALLAXCLIP_CEILING = 1,
	PARALLAXCLIP_FLOOR = 2,
};


bool FindSector(int nX, int nY, int nZ, sectortype** ppSector);
bool FindSector(int nX, int nY, sectortype** ppSector);

bool CheckProximity(DBloodActor* pSprite, int nX, int nY, int nZ, sectortype* pSector, int nDist);
bool CheckProximityPoint(int nX1, int nY1, int nZ1, int nX2, int nY2, int nZ2, int nDist);
bool CheckProximityWall(walltype* pWall, int x, int y, int nDist);
int GetWallAngle(walltype* pWall);
void GetWallNormal(walltype* pWall, int* pX, int* pY);
bool IntersectRay(int wx, int wy, int wdx, int wdy, int x1, int y1, int z1, int x2, int y2, int z2, int* ix, int* iy, int* iz);
int HitScan(DBloodActor* pSprite, int z, int dx, int dy, int dz, unsigned int nMask, int a8);
int VectorScan(DBloodActor* pSprite, int nOffset, int nZOffset, int dx, int dy, int dz, int nRange, int ac);
void GetZRange(DBloodActor* pSprite, int* ceilZ, Collision* ceilHit, int* floorZ, Collision* floorHit, int nDist, unsigned int nMask, unsigned int nClipParallax = 0);
void GetZRangeAtXYZ(int x, int y, int z, sectortype* pSector, int* ceilZ, Collision* ceilHit, int* floorZ, Collision* floorHit, int nDist, unsigned int nMask, unsigned int nClipParallax = 0);
int GetDistToLine(int x1, int y1, int x2, int y2, int x3, int y3);
void ClipMove(vec3_t& pos, sectortype** pSector, int xv, int yv, int wd, int cd, int fd, unsigned int nMask, CollisionBase& hit, int tracecount = 3);
BitArray GetClosestSpriteSectors(sectortype* pSector, int x, int y, int nDist, TArray<walltype*>* pWalls, bool newSectCheckMethod = false);
int picWidth(int nPic, int repeat);
int picHeight(int nPic, int repeat);

END_BLD_NS
