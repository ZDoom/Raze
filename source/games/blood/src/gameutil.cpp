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

#include "ns.h"	// Must come before everything else!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build.h"
#include "blood.h"


BEGIN_BLD_NS

HitInfo gHitInfo;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool FindSector(int nX, int nY, int nZ, sectortype** pSector)
{
	int32_t nZFloor, nZCeil;
	assert(pSector);
	if (inside(nX, nY, *pSector))
	{
		getzsofslopeptr(*pSector, nX, nY, &nZCeil, &nZFloor);
		if (nZ >= nZCeil && nZ <= nZFloor)
		{
			return 1;
		}
	}
	for (auto& wal : wallsofsector(*pSector))
	{
		auto pOSector = wal.nextSector();
		if (pOSector != nullptr && inside(nX, nY, pOSector))
		{
			getzsofslopeptr(pOSector, nX, nY, &nZCeil, &nZFloor);
			if (nZ >= nZCeil && nZ <= nZFloor)
			{
				*pSector = pOSector;
				return 1;
			}
		}
	}
	for (auto& sec : sector)
	{
		if (inside(nX, nY, &sec))
		{
			getzsofslopeptr(&sec, nX, nY, &nZCeil, &nZFloor);
			if (nZ >= nZCeil && nZ <= nZFloor)
			{
				*pSector = &sec;
				return 1;
			}
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool FindSector(int nX, int nY, sectortype** pSector)
{
	assert(*pSector);
	if (inside(nX, nY, *pSector))
	{
		return 1;
	}
	for (auto& wal : wallsofsector(*pSector))
	{
		auto pOSector = wal.nextSector();
		if (pOSector != nullptr && inside(nX, nY, pOSector))
		{
			*pSector = pOSector;
			return 1;
		}
	}
	for (auto& sec : sector)
	{
		if (inside(nX, nY, &sec))
		{
			*pSector = &sec;
			return 1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool CheckProximity(DBloodActor* actor, int nX, int nY, int nZ, sectortype* pSector, int nDist)
{
	assert(actor != nullptr);
	int oX = abs(nX - actor->spr.pos.X) >> 4;
	if (oX >= nDist) return 0;

	int oY = abs(nY - actor->spr.pos.Y) >> 4;
	if (oY >= nDist) return 0;

	int oZ = abs(nZ - actor->spr.pos.Z) >> 8;
	if (oZ >= nDist) return 0;

	if (approxDist(oX, oY) >= nDist) return 0;

	int bottom, top;
	GetActorExtents(actor, &top, &bottom);
	if (cansee(actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z, actor->sector(), nX, nY, nZ, pSector))
		return 1;
	if (cansee(actor->spr.pos.X, actor->spr.pos.Y, bottom, actor->sector(), nX, nY, nZ, pSector))
		return 1;
	if (cansee(actor->spr.pos.X, actor->spr.pos.Y, top, actor->sector(), nX, nY, nZ, pSector))
		return 1;
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool CheckProximityPoint(int nX1, int nY1, int nZ1, int nX2, int nY2, int nZ2, int nDist)
{
	int oX = abs(nX2 - nX1) >> 4;
	if (oX >= nDist)
		return 0;
	int oY = abs(nY2 - nY1) >> 4;
	if (oY >= nDist)
		return 0;
	if (nZ2 != nZ1)
	{
		int oZ = abs(nZ2 - nZ1) >> 8;
		if (oZ >= nDist)
			return 0;
	}
	if (approxDist(oX, oY) >= nDist) return 0;
	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool CheckProximityWall(walltype* pWall, int x, int y, int nDist)
{
	int x1 = pWall->wall_int_pos().X;
	int y1 = pWall->wall_int_pos().Y;
	int x2 = pWall->point2Wall()->wall_int_pos().X;
	int y2 = pWall->point2Wall()->wall_int_pos().Y;
	nDist <<= 4;
	if (x1 < x2)
	{
		if (x <= x1 - nDist || x >= x2 + nDist)
		{
			return 0;
		}
	}
	else
	{
		if (x <= x2 - nDist || x >= x1 + nDist)
		{
			return 0;
		}
		if (x1 == x2)
		{
			int px1 = x - x1;
			int py1 = y - y1;
			int px2 = x - x2;
			int py2 = y - y2;
			int dist1 = px1 * px1 + py1 * py1;
			int dist2 = px2 * px2 + py2 * py2;
			if (y1 < y2)
			{
				if (y <= y1 - nDist || y >= y2 + nDist)
				{
					return 0;
				}
				if (y < y1)
				{
					return dist1 < nDist* nDist;
				}
				if (y > y2)
				{
					return dist2 < nDist* nDist;
				}
			}
			else
			{
				if (y <= y2 - nDist || y >= y1 + nDist)
				{
					return 0;
				}
				if (y < y2)
				{
					return dist2 < nDist* nDist;
				}
				if (y > y1)
				{
					return dist1 < nDist* nDist;
				}
			}
			return 1;
		}
	}
	if (y1 < y2)
	{
		if (y <= y1 - nDist || y >= y2 + nDist)
		{
			return 0;
		}
	}
	else
	{
		if (y <= y2 - nDist || y >= y1 + nDist)
		{
			return 0;
		}
		if (y1 == y2)
		{
			int px1 = x - x1;
			int py1 = y - y1;
			int px2 = x - x2;
			int py2 = y - y2;
			int check1 = px1 * px1 + py1 * py1;
			int check2 = px2 * px2 + py2 * py2;
			if (x1 < x2)
			{
				if (x <= x1 - nDist || x >= x2 + nDist)
				{
					return 0;
				}
				if (x < x1)
				{
					return check1 < nDist* nDist;
				}
				if (x > x2)
				{
					return check2 < nDist* nDist;
				}
			}
			else
			{
				if (x <= x2 - nDist || x >= x1 + nDist)
				{
					return 0;
				}
				if (x < x2)
				{
					return check2 < nDist* nDist;
				}
				if (x > x1)
				{
					return check1 < nDist* nDist;
				}
			}
		}
	}

	int dx = x2 - x1;
	int dy = y2 - y1;
	int px = x - x2;
	int py = y - y2;
	int side = px * dx + dy * py;
	if (side >= 0)
	{
		return px * px + py * py < nDist* nDist;
	}
	px = x - x1;
	py = y - y1;
	side = px * dx + dy * py;
	if (side <= 0)
	{
		return px * px + py * py < nDist* nDist;
	}
	int check1 = px * dy - dx * py;
	int check2 = dy * dy + dx * dx;
	return check1 * check1 < check2* nDist* nDist;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int GetWallAngle(walltype* pWall)
{
	auto delta = pWall->delta();
	return getangle(delta.X, delta.Y);
}

void GetWallNormal(walltype* pWall, int* pX, int* pY)
{

	auto delta = pWall->delta();
	int dX = -delta.Y >> 4;
	int dY = delta.X >> 4;

	int nLength = ksqrt(dX * dX + dY * dY);
	if (nLength <= 0)
		nLength = 1;
	*pX = DivScale(dX, nLength, 16);
	*pY = DivScale(dY, nLength, 16);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool IntersectRay(int wx, int wy, int wdx, int wdy, int x1, int y1, int z1, int x2, int y2, int z2, int* ix, int* iy, int* iz)
{
	int dX = x1 - x2;
	int dY = y1 - y2;
	int dZ = z1 - z2;
	int side = wdx * dY - wdy * dX;
	int dX2 = x1 - wx;
	int dY2 = y1 - wy;
	int check1 = dX2 * dY - dY2 * dX;
	int check2 = wdx * dY2 - wdy * dX2;
	if (side >= 0)
	{
		if (!side)
			return 0;
		if (check1 < 0)
			return 0;
		if (check2 < 0 || check2 >= side)
			return 0;
	}
	else
	{
		if (check1 > 0)
			return 0;
		if (check2 > 0 || check2 <= side)
			return 0;
	}
	int nScale = DivScale(check2, side, 16);
	*ix = x1 + MulScale(dX, nScale, 16);
	*iy = y1 + MulScale(dY, nScale, 16);
	*iz = z1 + MulScale(dZ, nScale, 16);
	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int HitScan(DBloodActor* actor, int z, int dx, int dy, int dz, unsigned int nMask, int nRange)
{
	assert(actor != nullptr);
	assert(dx != 0 || dy != 0);
	gHitInfo.clearObj();
	int x = actor->spr.pos.X;
	int y = actor->spr.pos.Y;
	auto bakCstat = actor->spr.cstat;
	actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_HITSCAN;
	if (nRange)
	{
		hitscangoal.X = x + MulScale(nRange << 4, Cos(actor->spr.ang), 30);
		hitscangoal.Y = y + MulScale(nRange << 4, Sin(actor->spr.ang), 30);
	}
	else
	{
		hitscangoal.X = hitscangoal.Y = 0x1ffffff;
	}
	hitscan({ x, y, z }, actor->sector(), { dx, dy, dz << 4 }, gHitInfo, nMask);

	hitscangoal.X = hitscangoal.Y = 0x1ffffff;
	actor->spr.cstat = bakCstat;
	if (gHitInfo.actor() != nullptr)
		return 3;
	if (gHitInfo.hitWall != nullptr)
	{
		auto pWall = gHitInfo.hitWall;

		if (!pWall->twoSided())
			return 0;
		int nZCeil, nZFloor;
		getzsofslopeptr(pWall->nextSector(), gHitInfo.hitpos.X, gHitInfo.hitpos.Y, &nZCeil, &nZFloor);
		if (gHitInfo.hitpos.Z <= nZCeil || gHitInfo.hitpos.Z >= nZFloor)
			return 0;
		return 4;
	}
	if (gHitInfo.hitSector != nullptr)
		return 1 + (z < gHitInfo.hitpos.Z);
	return -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int VectorScan(DBloodActor* actor, int nOffset, int nZOffset, int dx, int dy, int dz, int nRange, int ac)
{
	assert(actor != nullptr);

	int nNum = 256;
	gHitInfo.clearObj();
	int x1 = actor->spr.pos.X + MulScale(nOffset, Cos(actor->spr.ang + 512), 30);
	int y1 = actor->spr.pos.Y + MulScale(nOffset, Sin(actor->spr.ang + 512), 30);
	int z1 = actor->spr.pos.Z + nZOffset;
	auto bakCstat = actor->spr.cstat;
	actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_HITSCAN;
	if (nRange)
	{
		hitscangoal.X = x1 + MulScale(nRange << 4, Cos(actor->spr.ang), 30);
		hitscangoal.Y = y1 + MulScale(nRange << 4, Sin(actor->spr.ang), 30);
	}
	else
	{
		hitscangoal.X = hitscangoal.Y = 0x1fffffff;
	}
	vec3_t pos = { x1, y1, z1 };
	hitscan(pos, actor->sector(), { dx, dy, dz << 4 }, gHitInfo, CLIPMASK1);

	hitscangoal.X = hitscangoal.Y = 0x1ffffff;
	actor->spr.cstat = bakCstat;
	while (nNum--)
	{
		if (nRange && approxDist(gHitInfo.hitpos.X - actor->spr.pos.X, gHitInfo.hitpos.Y - actor->spr.pos.Y) > nRange)
			return -1;
		auto other = gHitInfo.actor();
		if (other != nullptr)
		{
			if ((other->spr.flags & 8) && !(ac & 1))
				return 3;
			if ((other->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != 0)
				return 3;
			int nPicnum = other->spr.picnum;
			if (tileWidth(nPicnum) == 0 || tileHeight(nPicnum) == 0)
				return 3;
			int height = (tileHeight(nPicnum) * other->spr.yrepeat) << 2;
			int otherZ = other->spr.pos.Z;
			if (other->spr.cstat & CSTAT_SPRITE_YCENTER)
				otherZ += height / 2;
			int nTopOfs = tileTopOffset(nPicnum);
			if (nTopOfs)
				otherZ -= (nTopOfs * other->spr.yrepeat) << 2;
			assert(height > 0);
			int height2 = Scale(otherZ - gHitInfo.hitpos.Z, tileHeight(nPicnum), height);
			if (!(other->spr.cstat & CSTAT_SPRITE_YFLIP))
				height2 = tileHeight(nPicnum) - height2;
			if (height2 >= 0 && height2 < tileHeight(nPicnum))
			{
				int width = (tileWidth(nPicnum) * other->spr.xrepeat) >> 2;
				width = (width * 3) / 4;
				int check1 = ((y1 - other->spr.pos.Y) * dx - (x1 - other->spr.pos.X) * dy) / ksqrt(dx * dx + dy * dy);
				assert(width > 0);
				int width2 = Scale(check1, tileWidth(nPicnum), width);
				int nLeftOfs = tileLeftOffset(nPicnum);
				width2 += nLeftOfs + tileWidth(nPicnum) / 2;
				if (width2 >= 0 && width2 < tileWidth(nPicnum))
				{
					auto pData = tilePtr(nPicnum);
					if (pData[width2 * tileHeight(nPicnum) + height2] != TRANSPARENT_INDEX)
						return 3;
				}
			}
			bakCstat = other->spr.cstat;
			other->spr.cstat &= ~CSTAT_SPRITE_BLOCK_HITSCAN;
			gHitInfo.clearObj();
			pos = gHitInfo.hitpos; // must make a copy!
			hitscan(pos, other->sector(), { dx, dy, dz << 4 }, gHitInfo, CLIPMASK1);
			other->spr.cstat = bakCstat;
			continue;
		}
		if (gHitInfo.hitWall != nullptr)
		{
			walltype* pWall = gHitInfo.hitWall;
			if (!pWall->twoSided())
				return 0;
			sectortype* pSector = gHitInfo.hitSector;
			sectortype* pSectorNext = pWall->nextSector();
			int nZCeil, nZFloor;
			getzsofslopeptr(pWall->nextSector(), gHitInfo.hitpos.X, gHitInfo.hitpos.Y, &nZCeil, &nZFloor);
			if (gHitInfo.hitpos.Z <= nZCeil)
				return 0;
			if (gHitInfo.hitpos.Z >= nZFloor)
			{
				if (!(pSector->floorstat & CSTAT_SECTOR_SKY) || !(pSectorNext->floorstat & CSTAT_SECTOR_SKY))
					return 0;
				return 2;
			}
			if (!(pWall->cstat & (CSTAT_WALL_MASKED | CSTAT_WALL_1WAY)))
				return 0;
			int nOfs;
			if (pWall->cstat & CSTAT_WALL_ALIGN_BOTTOM)
				nOfs = ClipHigh(pSector->floorz, pSectorNext->floorz);
			else
				nOfs = ClipLow(pSector->ceilingz, pSectorNext->ceilingz);
			nOfs = (gHitInfo.hitpos.Z - nOfs) >> 8;
			if (pWall->cstat & CSTAT_WALL_YFLIP)
				nOfs = -nOfs;

			int nPicnum = pWall->overpicnum;
			int nSizX = tileWidth(nPicnum);
			int nSizY = tileHeight(nPicnum);
			if (!nSizX || !nSizY)
				return 0;

			nOfs = (nOfs * pWall->yrepeat) / 8;
			nOfs += int((nSizY * pWall->ypan_) / 256);
			int nLength = approxDist(pWall->wall_int_pos().X - pWall->point2Wall()->wall_int_pos().X, pWall->wall_int_pos().Y - pWall->point2Wall()->wall_int_pos().Y);
			int nHOffset;
			if (pWall->cstat & CSTAT_WALL_XFLIP)
				nHOffset = approxDist(gHitInfo.hitpos.X - pWall->point2Wall()->wall_int_pos().X, gHitInfo.hitpos.Y - pWall->point2Wall()->wall_int_pos().Y);
			else
				nHOffset = approxDist(gHitInfo.hitpos.X - pWall->wall_int_pos().X, gHitInfo.hitpos.Y - pWall->wall_int_pos().Y);

			nHOffset = pWall->xpan() + ((nHOffset * pWall->xrepeat) << 3) / nLength;
			nHOffset %= nSizX;
			nOfs %= nSizY;
			auto pData = tilePtr(nPicnum);
			int nPixel;
			nPixel = nHOffset * nSizY + nOfs;

			if (pData[nPixel] == TRANSPARENT_INDEX)
			{
				auto bakCstat1 = pWall->cstat;
				pWall->cstat &= ~CSTAT_WALL_BLOCK_HITSCAN;
				auto bakCstat2 = pWall->nextWall()->cstat;
				pWall->nextWall()->cstat &= ~CSTAT_WALL_BLOCK_HITSCAN;
				gHitInfo.clearObj();
				pos = gHitInfo.hitpos;
				hitscan(pos, pWall->nextSector(), { dx, dy, dz << 4 }, gHitInfo, CLIPMASK1);

				pWall->cstat = bakCstat1;
				pWall->nextWall()->cstat = bakCstat2;
				continue;
			}
			return 4;
		}
		if (gHitInfo.hitSector != nullptr)
		{
			if (dz > 0)
			{
				auto upper = barrier_cast<DBloodActor*>(gHitInfo.hitSector->upperLink);
				if (!upper) return 2;
				auto link = upper->GetOwner();
				gHitInfo.clearObj();
				x1 = gHitInfo.hitpos.X + link->spr.pos.X - upper->spr.pos.X;
				y1 = gHitInfo.hitpos.Y + link->spr.pos.Y - upper->spr.pos.Y;
				z1 = gHitInfo.hitpos.Z + link->spr.pos.Z - upper->spr.pos.Z;
				pos = { x1, y1, z1 };
				hitscan(pos, link->sector(), { dx, dy, dz << 4 }, gHitInfo, CLIPMASK1);

				continue;
			}
			else
			{
				auto lower = barrier_cast<DBloodActor*>(gHitInfo.hitSector->lowerLink);
				if (!lower) return 1;
				auto link = lower->GetOwner();
				gHitInfo.clearObj();
				x1 = gHitInfo.hitpos.X + link->spr.pos.X - lower->spr.pos.X;
				y1 = gHitInfo.hitpos.Y + link->spr.pos.Y - lower->spr.pos.Y;
				z1 = gHitInfo.hitpos.Z + link->spr.pos.Z - lower->spr.pos.Z;
				pos = { x1, y1, z1 };
				hitscan(pos, link->sector(), { dx, dy, dz << 4 }, gHitInfo, CLIPMASK1);
				continue;
			}
		}
		return -1;
	}
	return -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GetZRange(DBloodActor* actor, int* ceilZ, Collision* ceilColl, int* floorZ, Collision* floorColl, int nDist, unsigned int nMask, unsigned int nClipParallax)
{
	assert(actor != nullptr);
	Collision scratch;

	auto bakCstat = actor->spr.cstat;
	int32_t nTemp1;
	actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	getzrange(actor->spr.pos, actor->sector(), (int32_t*)ceilZ, *ceilColl, (int32_t*)floorZ, *floorColl, nDist, nMask);
	if (floorColl->type == kHitSector)
	{
		auto pSector = floorColl->hitSector;
		if ((nClipParallax & PARALLAXCLIP_FLOOR) == 0 && (pSector->floorstat & CSTAT_SECTOR_SKY))
			*floorZ = 0x7fffffff;
		if (pSector->hasX())
		{
			XSECTOR* pXSector = &pSector->xs();
			*floorZ += pXSector->Depth << 10;
		}
		auto linkActor = barrier_cast<DBloodActor*>(pSector->upperLink);
		if (linkActor)
		{
			auto linkOwner = linkActor->GetOwner();
			vec3_t lpos = actor->spr.pos + linkOwner->spr.pos - linkActor->spr.pos;
			getzrange(lpos, linkOwner->sector(), &nTemp1, scratch, (int32_t*)floorZ, *floorColl, nDist, nMask);
			*floorZ -= linkOwner->spr.pos.Z - linkActor->spr.pos.Z;
		}
	}
	if (ceilColl->type == kHitSector)
	{
		auto pSector = ceilColl->hitSector;
		if ((nClipParallax & PARALLAXCLIP_CEILING) == 0 && (pSector->ceilingstat & CSTAT_SECTOR_SKY))
			*ceilZ = 0x80000000;
		auto linkActor = barrier_cast<DBloodActor*>(pSector->lowerLink);
		if (linkActor)
		{
			auto linkOwner = linkActor->GetOwner();
			vec3_t lpos = actor->spr.pos + linkOwner->spr.pos - linkActor->spr.pos;
			getzrange(lpos, linkOwner->sector(), (int32_t*)ceilZ, *ceilColl, &nTemp1, scratch, nDist, nMask);
			*ceilZ -= linkOwner->spr.pos.Z - linkActor->spr.pos.Z;
		}
	}
	actor->spr.cstat = bakCstat;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GetZRangeAtXYZ(int x, int y, int z, sectortype* pSector, int* ceilZ, Collision* ceilColl, int* floorZ, Collision* floorColl, int nDist, unsigned int nMask, unsigned int nClipParallax)
{
	Collision scratch;
	int32_t nTemp1;
	vec3_t lpos = { x, y, z };
	getzrange(lpos, pSector, (int32_t*)ceilZ, *ceilColl, (int32_t*)floorZ, *floorColl, nDist, nMask);
	if (floorColl->type == kHitSector)
	{
		auto pHitSect = floorColl->hitSector;
		if ((nClipParallax & PARALLAXCLIP_FLOOR) == 0 && (pHitSect->floorstat & CSTAT_SECTOR_SKY))
			*floorZ = 0x7fffffff;
		if (pHitSect->hasX())
		{
			XSECTOR* pXSector = &pHitSect->xs();
			*floorZ += pXSector->Depth << 10;
		}
		auto actor = barrier_cast<DBloodActor*>(pHitSect->upperLink);
		if (actor)
		{
			auto link = actor->GetOwner();
			vec3_t newpos = lpos + link->spr.pos - actor->spr.pos;
			getzrange(newpos, link->sector(), &nTemp1, scratch, (int32_t*)floorZ, *floorColl, nDist, nMask);
			*floorZ -= link->spr.pos.Z - actor->spr.pos.Z;
		}
	}
	if (ceilColl->type == kHitSector)
	{
		auto pHitSect = ceilColl->hitSector;
		if ((nClipParallax & PARALLAXCLIP_CEILING) == 0 && (pHitSect->ceilingstat & CSTAT_SECTOR_SKY))
			*ceilZ = 0x80000000;
		auto actor = barrier_cast<DBloodActor*>(pHitSect->lowerLink);
		if (actor)
		{
			auto link = actor->GetOwner();
			vec3_t newpos = lpos + link->spr.pos - actor->spr.pos;
			getzrange(newpos, link->sector(), (int32_t*)ceilZ, *ceilColl, &nTemp1, scratch, nDist, nMask);
			*ceilZ -= link->spr.pos.Z - actor->spr.pos.Z;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int GetDistToLine(int x1, int y1, int x2, int y2, int x3, int y3)
{
	int check = (y1 - y3) * (x3 - x2);
	int check2 = (x1 - x2) * (y3 - y2);
	if (check2 > check)
		return -1;
	int v8 = DMulScale(x1 - x2, x3 - x2, y1 - y3, y3 - y2, 4);
	int vv = DMulScale(x3 - x2, x3 - x2, y3 - y2, y3 - y2, 4);
	int t1, t2;
	if (v8 <= 0)
	{
		t1 = x2;
		t2 = y2;
	}
	else if (vv > v8)
	{
		t1 = x2 + Scale(x3 - x2, v8, vv);
		t2 = y2 + Scale(y3 - y2, v8, vv);
	}
	else
	{
		t1 = x3;
		t2 = y3;
	}
	return approxDist(t1 - x1, t2 - y1);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ClipMove(vec3_t& pos, sectortype** pSector, int xv, int yv, int wd, int cd, int fd, unsigned int nMask, CollisionBase& hit, int tracecount)
{
	auto opos = pos;
	sectortype* bakSect = *pSector;
	clipmove(pos, &bakSect, xv << 14, yv << 14, wd, cd, fd, nMask, hit, tracecount);
	if (bakSect == nullptr)
	{
		pos = opos;
	}
	else
	{
		*pSector = bakSect;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

BitArray GetClosestSpriteSectors(sectortype* pSector, int x, int y, int nDist, TArray<walltype*>* pWalls, bool newSectCheckMethod)
{
	// by default this function fails with sectors that linked with wide spans, or there was more than one link to the same sector. for example...
	// E6M1: throwing TNT on the stone footpath while standing on the brown road will fail due to the start/end points of the span being too far away. it'll only do damage at one end of the road
	// E1M2: throwing TNT at the double doors while standing on the train platform
	// by setting newSectCheckMethod to true these issues will be resolved

	BitArray sectorMap(sector.Size()); // this gets returned to the caller.
	sectorMap.Zero();
	sectorMap.Set(sectnum(pSector));
	double nDist4sq = nDist * nDist;    // (nDist * 16)^2 - * 16 to account for Build's 28.4 fixed point format.

	BFSSectorSearch search(pSector);

	while (auto pCurSector = search.GetNext())
	{
		for (auto& wal : wallsofsector(pCurSector))
		{
			if (!wal.twoSided()) continue;
			const auto pNextSector = wal.nextSector();

			bool withinRange = false;
			if (!newSectCheckMethod) // original method
			{
				if (search.Check(pNextSector)) // if we've already checked this sector, skip. This is bad, therefore only in compat mode.
					continue;
				withinRange = CheckProximityWall(wal.point2Wall(), x, y, nDist);
			}
			else // new method using proper math and no bad shortcut.
			{
				double dist1 = SquareDistToWall(x * inttoworld, y * inttoworld, &wal);
				withinRange = dist1 <= nDist4sq;
			}
			if (withinRange) // if new sector is within range, add it to the processing queue
			{
				sectorMap.Set(sectnum(pNextSector));
				search.Add(pNextSector);
				if (pWalls && wal.hasX())
				{
					XWALL* pXWall = &wal.xw();
					if (pXWall->triggerVector && !pXWall->isTriggered && !pXWall->state)
						pWalls->Push(&wal);
				}
			}
		}
	}
	return sectorMap;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int picWidth(int nPic, int repeat) {
	return ClipLow((tileWidth(nPic) * repeat) << 2, 0);
}

int picHeight(int nPic, int repeat) {
	return ClipLow((tileHeight(nPic) * repeat) << 2, 0);
}



END_BLD_NS
