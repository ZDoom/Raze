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

bool CheckProximity(DBloodActor* actor, const DVector3& pos, sectortype* pSector, int nDist)
{
	assert(actor != nullptr);
	auto vec = pos - actor->spr.pos;
	if (abs(vec.Z) >= nDist) return false;

	if (vec.LengthSquared() >= nDist * nDist) return false;

	double bottom, top;
	GetActorExtents(actor, &top, &bottom);
	if (cansee(actor->spr.pos, actor->sector(), pos, pSector))
		return 1;
	if (cansee(DVector3(actor->spr.pos.XY(), bottom), actor->sector(), pos, pSector))
		return 1;
	if (cansee(DVector3(actor->spr.pos.XY(), top), actor->sector(), pos, pSector))
		return 1;
	return 0;
}

//---------------------------------------------------------------------------
//
// Note: This function features some very bad math.
// It cannot be redone because some game functionality
// depends on the math being broken.
//
//---------------------------------------------------------------------------

bool CheckProximityWall(walltype* pWall, const DVector2& pos, int nDist)
{
	int x = pos.X * (1. / maptoworld);
	int y = pos.Y * (1. / maptoworld);
	int x1 = pWall->pos.X * (1./maptoworld);
	int y1 = pWall->pos.Y * (1./maptoworld);
	int x2 = pWall->point2Wall()->pos.X * (1./maptoworld);
	int y2 = pWall->point2Wall()->pos.Y * (1./maptoworld);
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

int HitScan(DBloodActor* actor, double z, const DVector3& vect, unsigned int nMask, double nRange)
{
	assert(actor != nullptr);
	assert(!vect.XY().isZero());
	gHitInfo.clearObj();
	auto bakCstat = actor->spr.cstat;
	actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_HITSCAN;
	hitscan(DVector3(actor->spr.pos.XY(), z), actor->sector(), vect, gHitInfo, nMask, nRange);

	actor->spr.cstat = bakCstat;
	if (gHitInfo.actor() != nullptr)
		return 3;
	if (gHitInfo.hitWall != nullptr)
	{
		auto pWall = gHitInfo.hitWall;

		if (!pWall->twoSided())
			return 0;
		double nZCeil, nZFloor;
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

int VectorScan(DBloodActor* actor, double nOffset, double nZOffset, const DVector3& vel, double nRange, int ac)
{
	assert(actor != nullptr);

	int nNum = 256;
	gHitInfo.clearObj();
	auto pos = actor->spr.pos.plusZ(nZOffset) + (actor->spr.angle + DAngle90).ToVector() * nOffset;
	auto bakCstat = actor->spr.cstat;
	actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_HITSCAN;

	hitscan(pos, actor->sector(), vel, gHitInfo, CLIPMASK1, nRange);

	actor->spr.cstat = bakCstat;
	while (nNum--)
	{
		if (nRange && (gHitInfo.hitpos.XY() - actor->spr.pos.XY()).Length() > nRange)
			return -1;
		auto other = gHitInfo.actor();
		if (other != nullptr)
		{
			if ((other->spr.flags & 8) && !(ac & 1))
				return SS_SPRITE;
			if ((other->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != 0)
				return SS_SPRITE;

			int nPicnum = other->spr.picnum;
			if (tileWidth(nPicnum) == 0 || tileHeight(nPicnum) == 0)
				return SS_SPRITE;

			double height = (tileHeight(nPicnum) * other->spr.ScaleY());
			double otherZ = other->spr.pos.Z;
			if (other->spr.cstat & CSTAT_SPRITE_YCENTER)
				otherZ += height / 2;

			int nTopOfs = tileTopOffset(nPicnum);
			if (nTopOfs)
				otherZ -= (nTopOfs * other->spr.ScaleY());
			assert(height > 0);

			double height2 = (otherZ - gHitInfo.hitpos.Z) * tileHeight(nPicnum) / height;
			if (!(other->spr.cstat & CSTAT_SPRITE_YFLIP))
				height2 = tileHeight(nPicnum) - height2;

			if (height2 >= 0 && height2 < tileHeight(nPicnum))
			{
				double width = (tileWidth(nPicnum) * other->spr.ScaleX()) * 0.75; // should actually be 0.8 to match the renderer!
				double check1 = ((pos.Y - other->spr.pos.Y) * vel.X - (pos.X - other->spr.pos.X) * vel.Y) / vel.XY().Length();
				assert(width > 0);

				double width2 = check1 * tileWidth(nPicnum) / width;
				int nLeftOfs = tileLeftOffset(nPicnum);
				width2 += nLeftOfs + tileWidth(nPicnum) / 2;
				if (width2 >= 0 && width2 < tileWidth(nPicnum))
				{
					auto pData = tilePtr(nPicnum);
					if (pData[int(width2) * tileHeight(nPicnum) + int(height2)] != TRANSPARENT_INDEX)
						return SS_SPRITE;
				}
			}
			bakCstat = other->spr.cstat;
			other->spr.cstat &= ~CSTAT_SPRITE_BLOCK_HITSCAN;
			gHitInfo.clearObj();
			pos = gHitInfo.hitpos; // must make a copy!
			hitscan(pos, other->sector(), vel, gHitInfo, CLIPMASK1);
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
			double nZCeil, nZFloor;
			getzsofslopeptr(pWall->nextSector(), gHitInfo.hitpos, &nZCeil, &nZFloor);
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
			double nOfs;
			if (pWall->cstat & CSTAT_WALL_ALIGN_BOTTOM)
				nOfs = min(pSector->floorz, pSectorNext->floorz);
			else
				nOfs = max(pSector->ceilingz, pSectorNext->ceilingz);
			nOfs = (gHitInfo.hitpos.Z - nOfs);
			if (pWall->cstat & CSTAT_WALL_YFLIP)
				nOfs = -nOfs;

			int nPicnum = pWall->overpicnum;
			int nSizX = tileWidth(nPicnum);
			int nSizY = tileHeight(nPicnum);
			if (!nSizX || !nSizY)
				return 0;

			int nnOfs = int((nOfs * pWall->_yrepeat) / 8);
			nnOfs += int((nSizY * pWall->ypan_) / 256);
			double nLength = (pWall->pos - pWall->point2Wall()->pos).Length();
			double fHOffset;
			if (pWall->cstat & CSTAT_WALL_XFLIP)
				fHOffset = (gHitInfo.hitpos.XY() - pWall->point2Wall()->pos).Length();
			else
				fHOffset = (gHitInfo.hitpos.XY() - pWall->pos).Length();

			int nHOffset = int(pWall->xpan_ + ((fHOffset * pWall->_xrepeat) * 8) / nLength) % nSizX;
			nnOfs %= nSizY;
			auto pData = tilePtr(nPicnum);
			int nPixel = nHOffset * nSizY + nnOfs;

			if (pData[nPixel] == TRANSPARENT_INDEX)
			{
				auto bakCstat1 = pWall->cstat;
				pWall->cstat &= ~CSTAT_WALL_BLOCK_HITSCAN;
				auto bakCstat2 = pWall->nextWall()->cstat;
				pWall->nextWall()->cstat &= ~CSTAT_WALL_BLOCK_HITSCAN;
				gHitInfo.clearObj();
				pos = gHitInfo.hitpos;
				hitscan(pos, pWall->nextSector(), vel, gHitInfo, CLIPMASK1);

				pWall->cstat = bakCstat1;
				pWall->nextWall()->cstat = bakCstat2;
				continue;
			}
			return 4;
		}
		if (gHitInfo.hitSector != nullptr)
		{
			if (vel.Z > 0)
			{
				auto upper = barrier_cast<DBloodActor*>(gHitInfo.hitSector->upperLink);
				if (!upper) return SS_FLOOR;
				auto link = upper->GetOwner();
				gHitInfo.clearObj();
				pos = gHitInfo.hitpos + link->spr.pos - upper->spr.pos;
				hitscan(pos, link->sector(), vel, gHitInfo, CLIPMASK1);

				continue;
			}
			else
			{
				auto lower = barrier_cast<DBloodActor*>(gHitInfo.hitSector->lowerLink);
				if (!lower) return SS_CEILING;
				auto link = lower->GetOwner();
				gHitInfo.clearObj();
				pos = gHitInfo.hitpos + link->spr.pos - lower->spr.pos;
				hitscan(pos, link->sector(), vel, gHitInfo, CLIPMASK1);
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

void GetZRange(DBloodActor* actor, double* ceilZ, Collision* ceilColl, double* floorZ, Collision* floorColl, double nDist, unsigned int nMask, unsigned int nClipParallax)
{
	assert(actor != nullptr);
	Collision scratch;

	auto bakCstat = actor->spr.cstat;
	double nTemp1;
	actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	getzrange(actor->spr.pos, actor->sector(), ceilZ, *ceilColl, floorZ, *floorColl, nDist, nMask);
	if (floorColl->type == kHitSector)
	{
		auto pSector = floorColl->hitSector;
		if ((nClipParallax & PARALLAXCLIP_FLOOR) == 0 && (pSector->floorstat & CSTAT_SECTOR_SKY))
			*floorZ = 0x800000;
		if (pSector->hasX())
		{
			XSECTOR* pXSector = &pSector->xs();
			*floorZ += pXSector->Depth << 2;
		}
		auto linkActor = barrier_cast<DBloodActor*>(pSector->upperLink);
		if (linkActor)
		{
			auto linkOwner = linkActor->GetOwner();
			auto lpos = actor->spr.pos + linkOwner->spr.pos - linkActor->spr.pos;
			getzrange(lpos, linkOwner->sector(), &nTemp1, scratch, floorZ, *floorColl, nDist, nMask);
			*floorZ -= linkOwner->spr.pos.Z - linkActor->spr.pos.Z;
		}
	}
	if (ceilColl->type == kHitSector)
	{
		auto pSector = ceilColl->hitSector;
		if ((nClipParallax & PARALLAXCLIP_CEILING) == 0 && (pSector->ceilingstat & CSTAT_SECTOR_SKY))
			*ceilZ = -(int)0x800000;
		auto linkActor = barrier_cast<DBloodActor*>(pSector->lowerLink);
		if (linkActor)
		{
			auto linkOwner = linkActor->GetOwner();
			auto lpos = actor->spr.pos + linkOwner->spr.pos - linkActor->spr.pos;
			getzrange(lpos, linkOwner->sector(), ceilZ, *ceilColl, &nTemp1, scratch, nDist, nMask);
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

void GetZRangeAtXYZ(const DVector3& pos, sectortype* pSector, double* ceilZ, Collision* ceilColl, double* floorZ, Collision* floorColl, double nDist, unsigned int nMask, unsigned int nClipParallax)
{
	Collision scratch;
	double nTemp1;
	getzrange(pos, pSector, ceilZ, *ceilColl, floorZ, *floorColl, nDist, nMask);
	if (floorColl->type == kHitSector)
	{
		auto pHitSect = floorColl->hitSector;
		if ((nClipParallax & PARALLAXCLIP_FLOOR) == 0 && (pHitSect->floorstat & CSTAT_SECTOR_SKY))
			*floorZ = 0x800000;
		if (pHitSect->hasX())
		{
			XSECTOR* pXSector = &pHitSect->xs();
			*floorZ += pXSector->Depth << 2;
		}
		auto actor = barrier_cast<DBloodActor*>(pHitSect->upperLink);
		if (actor)
		{
			auto link = actor->GetOwner();
			auto newpos = pos + link->spr.pos - actor->spr.pos;
			getzrange(newpos, link->sector(), &nTemp1, scratch, floorZ, *floorColl, nDist, nMask);
			*floorZ -= link->spr.pos.Z - actor->spr.pos.Z;
		}
	}
	if (ceilColl->type == kHitSector)
	{
		auto pHitSect = ceilColl->hitSector;
		if ((nClipParallax & PARALLAXCLIP_CEILING) == 0 && (pHitSect->ceilingstat & CSTAT_SECTOR_SKY))
			*ceilZ = -0x800000;
		auto actor = barrier_cast<DBloodActor*>(pHitSect->lowerLink);
		if (actor)
		{
			auto link = actor->GetOwner();
			auto newpos = pos + link->spr.pos - actor->spr.pos;
			getzrange(newpos, link->sector(), ceilZ, *ceilColl, &nTemp1, scratch, nDist, nMask);
			*ceilZ -= link->spr.pos.Z - actor->spr.pos.Z;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ClipMove(DVector3& pos, sectortype** pSector, const DVector2& vect, double wd, double cd, double fd, unsigned int nMask, CollisionBase& hit, int tracecount)
{
	auto opos = pos;
	sectortype* bakSect = *pSector;
	
	// Due to the low precision of original Build coordinates this code is susceptible to shifts from negative values being off by one, 
	// so we have to replicate the imprecision here. Gross...
	DVector2 vel;
	vel.X = (FloatToFixed(vect.X) >> 12) / 16.;
	vel.Y = (FloatToFixed(vect.Y) >> 12) / 16.;
	
	clipmove(pos, &bakSect, vel, wd, cd, fd, nMask, hit, tracecount);
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

BitArray GetClosestSpriteSectors(sectortype* pSector, const DVector2& pos, int nDist, TArray<walltype*>* pWalls, bool newSectCheckMethod)
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
				withinRange = CheckProximityWall(wal.point2Wall(), pos, nDist);
			}
			else // new method using proper math and no bad shortcut.
			{
				double dist1 = SquareDistToWall(pos.X, pos.Y, &wal);
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


END_BLD_NS
