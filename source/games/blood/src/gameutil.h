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


bool CheckProximity(DBloodActor* pSprite, const DVector3& pos, sectortype* pSector, int nDist);
bool CheckProximityPoint(int nX1, int nY1, int nZ1, int nX2, int nY2, int nZ2, int nDist);
int GetWallAngle(walltype* pWall);
bool IntersectRay(int wx, int wy, int wdx, int wdy, int x1, int y1, int z1, int x2, int y2, int z2, int* ix, int* iy, int* iz);
int HitScan(DBloodActor* pSprite, int z, int dx, int dy, int dz, unsigned int nMask, int a8);
inline int HitScan(DBloodActor* pSprite, double z, int dx, int dy, int dz, unsigned int nMask, int a8)
{
	return HitScan(pSprite, int(z * zworldtoint), dx, dy, dz, nMask, a8);
}
int VectorScan(DBloodActor* pSprite, int nOffset, int nZOffset, int dx, int dy, int dz, int nRange, int ac);
void GetZRange(DBloodActor* pSprite, int* ceilZ, Collision* ceilHit, int* floorZ, Collision* floorHit, int nDist, unsigned int nMask, unsigned int nClipParallax = 0);
void GetZRangeAtXYZ(int x, int y, int z, sectortype* pSector, int* ceilZ, Collision* ceilHit, int* floorZ, Collision* floorHit, int nDist, unsigned int nMask, unsigned int nClipParallax = 0);
void GetZRangeAtXYZ(const DVector3& pos, sectortype* pSector, double* ceilZ, Collision* ceilHit, double* floorZ, Collision* floorHit, int nDist, unsigned int nMask, unsigned int nClipParallax = 0)
{
	vec3_t ipos = { int(pos.X * worldtoint), int(pos.Y * worldtoint), int(pos.Z * zworldtoint)};
	int cz, fz;
	GetZRangeAtXYZ(ipos.X, ipos.Y, ipos.Z, pSector, &cz, ceilHit, &fz, floorHit, nDist, nMask);
	*ceilZ = cz * zinttoworld;
	*floorZ = fz * zinttoworld;
}
int GetDistToLine(int x1, int y1, int x2, int y2, int x3, int y3);
void ClipMove(vec3_t& pos, sectortype** pSector, int xv, int yv, int wd, int cd, int fd, unsigned int nMask, CollisionBase& hit, int tracecount = 3);
inline void ClipMove(DVector3& pos, sectortype** pSector, int xv, int yv, int wd, int cd, int fd, unsigned int nMask, CollisionBase& hit, int tracecount = 3)
{
	// this uses floats only partially.
	vec3_t ipos = { int(pos.X * worldtoint), int(pos.Y * worldtoint), int(pos.Z * zworldtoint)};
	ClipMove(ipos, pSector, xv, yv, wd, cd, fd, nMask, hit, tracecount);
	pos = { ipos.X * inttoworld, ipos.Y * inttoworld, ipos.Z * zinttoworld };
}
inline void ClipMove(DVector3& pos, sectortype** pSector, const DVector2& vect, int wd, double cd, double fd, unsigned int nMask, CollisionBase& hit, int tracecount = 3)
{
	// this uses floats only partially.
	vec3_t ipos = { int(pos.X * worldtoint), int(pos.Y * worldtoint), int(pos.Z * zworldtoint)};
	ClipMove(ipos, pSector, int(vect.X * worldtoint), int(vect.Y * worldtoint), wd, int(cd * zworldtoint), int(fd * zworldtoint), nMask, hit, tracecount);
	pos = { ipos.X * inttoworld, ipos.Y * inttoworld, ipos.Z * zinttoworld };
}
BitArray GetClosestSpriteSectors(sectortype* pSector, const DVector2& pos, int nDist, TArray<walltype*>* pWalls, bool newSectCheckMethod = false);
int picWidth(int nPic, int repeat);
int picHeight(int nPic, int repeat);

END_BLD_NS
