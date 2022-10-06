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

int HitScan(DBloodActor* pSprite, double z, const DVector3& pos, unsigned int nMask, double range = 0);
int VectorScan(DBloodActor* pSprite, double nOffset, double nZOffset, const DVector3& vel, double nRange, int ac);
void GetZRange(DBloodActor* pSprite, double* ceilZ, Collision* ceilHit, double* floorZ, Collision* floorHit, double nDist, unsigned int nMask, unsigned int nClipParallax = 0);
void GetZRangeAtXYZ(const DVector3& pos, sectortype* pSector, double* ceilZ, Collision* ceilHit, double* floorZ, Collision* floorHit, double nDist, unsigned int nMask, unsigned int nClipParallax = 0);

void ClipMove(DVector3& pos, sectortype** pSector, const DVector2& vect, double wd, double cd, double fd, unsigned int nMask, CollisionBase& hit, int tracecount = 3);
BitArray GetClosestSpriteSectors(sectortype* pSector, const DVector2& pos, int nDist, TArray<walltype*>* pWalls, bool newSectCheckMethod = false);

END_BLD_NS
