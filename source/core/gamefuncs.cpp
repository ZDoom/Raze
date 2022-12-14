//-------------------------------------------------------------------------
/*
Copyright (C) 2021 Christoph Oelckers & Mitchell Richters

This is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
//------------------------------------------------------------------------- 

#include "gamefuncs.h"
#include "gamestruct.h"
#include "intvec.h"
#include "coreactor.h"
#include "interpolate.h"
#include "hw_voxels.h"

IntRect viewport3d;
constexpr double MAXCLIPDISTF = 64;

//---------------------------------------------------------------------------
//
// Unified chasecam function for all games.
//
//---------------------------------------------------------------------------

double cameradist, cameraclock;

bool calcChaseCamPos(DVector3& ppos, DCoreActor* act, sectortype** psect, DAngle ang, DAngle horiz, double const interpfrac, double const backamp)
{
	if (!*psect) return false;

	// Calculate new pos to shoot backwards
	DVector3 npos = -DVector3(ang.ToVector() * horiz.Cos(), horiz.Sin()) * backamp;

	HitInfoBase hitinfo;
	auto bakcstat = act->spr.cstat;
	act->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	updatesectorz(ppos, psect);
	hitscan(ppos, *psect, npos, hitinfo, CLIPMASK1);
	act->spr.cstat = bakcstat;
	auto hpos = hitinfo.hitpos - ppos;

	if (!*psect) return false;

	// If something is in the way, make cameradist lower if necessary
	if (npos.XY().Sum() > hpos.XY().Sum())
	{
		double DVector3::* c = fabs(npos.X) > fabs(npos.Y) ? &DVector3::X : &DVector3::Y;

		if (hitinfo.hitWall != nullptr)
		{
			// Push you a little bit off the wall
			*psect = hitinfo.hitSector;
			hpos.*c -= npos.*c * npos.XY().dot(hitinfo.hitWall->delta().Angle().ToVector().Rotated90CW()) * (1. / 1024.);
		}
		else if (hitinfo.hitActor == nullptr)		
		{
			// Push you off the ceiling/floor
			*psect = hitinfo.hitSector;
			hpos.*c -= npos.*c * (1. / 32.);
		}
		else
		{
			// If you hit a sprite that's not a wall sprite - try again.
			if (!(hitinfo.hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
			{
				bakcstat = hitinfo.hitActor->spr.cstat;
				hitinfo.hitActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
				calcChaseCamPos(ppos, act, psect, ang, horiz, interpfrac, backamp);
				hitinfo.hitActor->spr.cstat = bakcstat;
				return false;
			}
			else
			{
				// same as wall calculation.
				hpos.*c -= npos.*c * npos.XY().dot((act->spr.angle - DAngle90).ToVector().Rotated90CW()) * (1. / 1024.);
			}
		}

		double newdist = hpos.*c / npos.*c;
		if (newdist < cameradist) cameradist = newdist;
	}

	// Actually move you! (camerdist is 1 if nothing is in the way)
	ppos += npos * cameradist;

	// Calculate clock using GameTicRate so it increases the same rate on all speed computers.
	double myclock = PlayClock + 120 / GameTicRate * interpfrac;
	if (cameraclock == INT_MIN)
	{
		// Third person view was just started.
		cameraclock = myclock;
	}

	// Slowly increase cameradist until it reaches 1.
	cameradist = min(cameradist + ((myclock - cameraclock) * (1. / 64.)), 1.);
	cameraclock = myclock;

	// Make sure psectnum is correct.
	updatesectorz(ppos, psect);

	return true;
}

//==========================================================================
//
// consolidated slope calculation
//
//==========================================================================

void calcSlope(const sectortype* sec, double xpos, double ypos, double* pceilz, double* pflorz)
{
	int bits = 0;
	if (pceilz)
	{
		bits |= sec->ceilingstat;
		*pceilz = sec->ceilingz;
	}
	if (pflorz)
	{
		bits |= sec->floorstat;
		*pflorz = sec->floorz;
	}

	if ((bits & CSTAT_SECTOR_SLOPE) == CSTAT_SECTOR_SLOPE)
	{
		auto wal = sec->walls.Data();
		double len = wal->Length();
		if (len != 0)
		{
			DVector2 wd = DVector2(xpos, ypos) - wal->pos;
			double fac = wal->delta().dot(wd.Rotated90CW()) / len * (1. / SLOPEVAL_FACTOR);
			if (pceilz && sec->ceilingstat & CSTAT_SECTOR_SLOPE) *pceilz += (sec->ceilingheinum * fac);
			if (pflorz && sec->floorstat & CSTAT_SECTOR_SLOPE) *pflorz += (sec->floorheinum * fac);
		}
	}
}

//==========================================================================
//
// 
//
//==========================================================================

int getslopeval(sectortype* sect, const DVector3& pos, double basez)
{
	auto wal = sect->walls.Data();
	double i = (pos.XY() - wal->pos).dot(wal->delta().Rotated90CCW());
	return i == 0? 0 : int(SLOPEVAL_FACTOR * (pos.Z - basez) * wal->Length() / i);
}

//==========================================================================
//
// Calculate the distance to the closest point in the given sector
//
//==========================================================================

double SquareDistToSector(double px, double py, const sectortype* sect, DVector2* point)
{
	if (inside(px, py, sect))
	{
		if (point)
			*point = { px, py };
		return 0;
	}

	double bestdist = DBL_MAX;
	DVector2 bestpt = { px, py };
	for (auto& wal : sect->walls)
	{
		DVector2 pt;
		auto dist = SquareDistToWall(px, py, &wal, &pt);
		if (dist < bestdist)
		{
			bestdist = dist;
			bestpt = pt;
		}
	}
	if (point) *point = bestpt;
	return bestdist;
}

//==========================================================================
//
// Calculate the position of a wall sprite in the world
//
//==========================================================================

void GetWallSpritePosition(const spritetypebase* spr, const DVector2& pos, DVector2* out, bool render)
{
	auto tex = tileGetTexture(spr->picnum);

	double width, xoffset;
	if (render && hw_hightile && TileFiles.tiledata[spr->picnum].hiofs.xsize)
	{
		width = TileFiles.tiledata[spr->picnum].hiofs.xsize;
		xoffset = (TileFiles.tiledata[spr->picnum].hiofs.xoffs + spr->xoffset);
	}
	else
	{
		width = tex->GetDisplayWidth();
		xoffset = tex->GetDisplayLeftOffset() + spr->xoffset;
	}

	auto p = spr->angle.ToVector().Rotated90CW() * spr->scale.X;

	if (spr->cstat & CSTAT_SPRITE_XFLIP) xoffset = -xoffset;
	double origin = (width * 0.5) + xoffset;

	out[0] = pos - p * origin;
	out[1] = out[0] + p * width;
}


//==========================================================================
//
// Calculate the position of a floor sprite in the world
//
//==========================================================================

void TGetFlatSpritePosition(const spritetypebase* spr, const DVector2& pos, DVector2* out, double* outz, int heinum, bool render)
{
	auto tex = tileGetTexture(spr->picnum);

	double width, height, leftofs, topofs;
	double sloperatio = sqrt(heinum * heinum + SLOPEVAL_FACTOR * SLOPEVAL_FACTOR) * (1. / SLOPEVAL_FACTOR);
	double xrepeat = spr->scale.X;
	double yrepeat = spr->scale.Y;

	int xo = heinum ? 0 : spr->xoffset;
	int yo = heinum ? 0 : spr->yoffset;

	if (render && hw_hightile && TileFiles.tiledata[spr->picnum].hiofs.xsize)
	{
		width = TileFiles.tiledata[spr->picnum].hiofs.xsize * xrepeat;
		height = TileFiles.tiledata[spr->picnum].hiofs.ysize * yrepeat;
		leftofs = (TileFiles.tiledata[spr->picnum].hiofs.xoffs + xo) * xrepeat;
		topofs = (TileFiles.tiledata[spr->picnum].hiofs.yoffs + yo) * yrepeat;
	}
	else
	{
		width = (int)tex->GetDisplayWidth() * xrepeat;
		height = (int)tex->GetDisplayHeight() * yrepeat;
		leftofs = ((int)tex->GetDisplayLeftOffset() + xo) * xrepeat;
		topofs = ((int)tex->GetDisplayTopOffset() + yo) * yrepeat;
	}

	if (spr->cstat & CSTAT_SPRITE_XFLIP) leftofs = -leftofs;
	if (spr->cstat & CSTAT_SPRITE_YFLIP) topofs = -topofs;

	double sprcenterx = (width * 0.5) + leftofs;
	double sprcentery = (height * 0.5) + topofs;

	double cosang = spr->angle.Cos();
	double sinang = spr->angle.Sin();
	double cosangslope = cosang / sloperatio;
	double sinangslope = sinang / sloperatio;

	out[0].X = pos.X + sinang * sprcenterx + cosangslope * sprcentery;
	out[0].Y = pos.Y + sinangslope * sprcentery - cosang * sprcenterx;

	out[1].X = out[0].X - sinang * width;
	out[1].Y = out[0].Y + cosang * width;

	DVector2 sub = { cosangslope * height, sinangslope * height };
	out[2] = out[1] - sub;
	out[3] = out[0] - sub;
	if (outz)
	{
		if (!heinum) outz[3] = outz[2] = outz[1] = outz[0] = 0;
		else
		{
			for (int i = 0; i < 4; i++)
			{
				outz[i] = (sinang * (out[i].Y - pos.Y) + cosang * (out[i].X - pos.X)) * heinum * (1. / SLOPEVAL_FACTOR);
			}
		}
	}
}


void GetFlatSpritePosition(DCoreActor* actor, const DVector2& pos, DVector2* out, double* outz, bool render)
{
	TGetFlatSpritePosition(&actor->spr, pos, out, outz, spriteGetSlope(actor), render);
}

void GetFlatSpritePosition(const tspritetype* spr, const DVector2& pos, DVector2* out, double* outz, bool render)
{
	TGetFlatSpritePosition(spr, pos, out, outz, tspriteGetSlope(spr), render);
}

//==========================================================================
//
// checks if the given point is sufficiently close to the given line segment.
//
//==========================================================================

EClose IsCloseToLine(const DVector2& point, const DVector2& start, const DVector2& end, double maxdist)
{
	auto const v1 = start - point;
	auto const v2 = end - point;

	// trivially outside the box.
	if (
		((v1.X < -maxdist) && (v2.X < -maxdist)) || // fully to the left
		((v1.Y < -maxdist) && (v2.Y < -maxdist)) || // fully below
		((v1.X >= maxdist) && (v2.X >= maxdist)) || // fully to the right
		((v1.Y >= maxdist) && (v2.Y >= maxdist)))   // fully above
		return EClose::Outside;

	auto waldelta = end - start;

	if (waldelta.X * v1.Y <= waldelta.Y * v1.X)
	{
		// is it in front?
		waldelta.X *= waldelta.X > 0 ? v1.Y + maxdist : v1.Y - maxdist;
		waldelta.Y *= waldelta.Y > 0 ? v1.X - maxdist : v1.X + maxdist;
		return waldelta.X > waldelta.Y ? EClose::InFront : EClose::Outside;
	}
	else
	{
		// or behind?
		waldelta.X *= waldelta.X > 0 ? v1.Y - maxdist : v1.Y + maxdist;
		waldelta.Y *= waldelta.Y > 0 ? v1.X + maxdist : v1.X - maxdist;
		return (waldelta.X <= waldelta.Y) ? EClose::Behind : EClose::Outside;
	}
}

EClose IsCloseToWall(const DVector2& point, walltype* wal, double maxdist)
{
	return IsCloseToLine(point, wal->pos, wal->point2Wall()->pos, maxdist);
}

//==========================================================================
//
// Check if some walls are set to use rotated textures.
// Ideally this should just have been done with texture rotation,
// but the effects on the render code would be too severe due to the alignment mess.
//
//==========================================================================

void checkRotatedWalls()
{
	for (auto& w : wall)
	{
		if (w.cstat & CSTAT_WALL_ROTATE_90)
		{
			int picnum = w.picnum;
			tileUpdatePicnum(&picnum);
			auto& tile = RotTile(picnum);

			if (tile.newtile == -1 && tile.owner == -1)
			{
				tile.newtile = TileFiles.tileCreateRotated(picnum);
				assert(tile.newtile != -1);

				RotTile(tile.newtile).owner = picnum;

			}
		}
	}
}

//==========================================================================
//
// check if two sectors share a wall connection
//
//==========================================================================

bool sectorsConnected(int sect1, int sect2)
{
	for (auto& wal : sector[sect1].walls)
	{
		if (wal.nextsector == sect2) return true;
	}
	return false;
}

//==========================================================================
//
//
//
//==========================================================================

int64_t checkforinside(double x, double y, const DVector2& pt1, const DVector2& pt2)
{
	// Perform the checks here in 48.16 fixed point.
	// Doing it directly with floats and multiplications does not work reliably due to underflows.
	// Unfortunately, due to the conversions, this is a bit slower. :(
	int64_t xs = int64_t(0x10000 * (pt1.X - x));
	int64_t ys = int64_t(0x10000 * (pt1.Y - y));
	int64_t xe = int64_t(0x10000 * (pt2.X - x));
	int64_t ye = int64_t(0x10000 * (pt2.Y - y));

	if ((ys ^ ye) < 0)
	{
		int64_t val;

		if ((xs ^ xe) >= 0) val = xs;
		else val = ((xs * ye) - xe * ys) ^ ye;
		return val;
	}
	return 0;
}

//==========================================================================
//
// 
//
//==========================================================================

int inside(double x, double y, const sectortype* sect)
{
	if (sect)
	{
		int64_t acc = 1;
		for (auto& wal : sect->walls)
		{
			acc ^= checkforinside(x, y, wal.pos, wal.point2Wall()->pos);
		}
		return acc < 0;
	}
	return -1;
}

//==========================================================================
//
//
//
//==========================================================================

int insidePoly(double x, double y, const DVector2* points, int count)
{
	int64_t acc = 1;
	for (int i = 0; i < count; i++)
	{
		int j = (i + 1) % count;
		acc ^= checkforinside(x, y, points[i], points[j]);
	}
	return acc < 0;	
}

//==========================================================================
//
// find the closest neighboring sector plane in the given direction.
// Does not consider slopes, just like the original!
//
//==========================================================================

sectortype* nextsectorneighborzptr(sectortype* sectp, double startz, int flags)
{
	double factor = (flags & Find_Up)? -1 : 1;
	double bestz = INT_MAX;
	sectortype* bestsec = (flags & Find_Safe)? sectp : nullptr;
	const auto planez = (flags & Find_Ceiling)? &sectortype::ceilingz : &sectortype::floorz;

	startz *= factor;
	for(auto& wal : sectp->walls)
	{
		if (wal.twoSided())
		{
			auto nextsec = wal.nextSector();
			auto nextz = factor * nextsec->*planez;
			
			if (startz < nextz && nextz < bestz)
			{
				bestz = nextz;
				bestsec = nextsec;
			}
		}
	}
	return bestsec;
}


//==========================================================================
//
//
//
//==========================================================================

bool cansee(const DVector3& start, sectortype* sect1, const DVector3& end, sectortype* sect2)
{
	if (!sect1 || !sect2) return false;

	auto delta = end - start;

	if (delta.XY().isZero())
		return (sect1 == sect2);

	BFSSectorSearch search(sect1);

	while (auto sec = search.GetNext())
	{
		for (auto& wal : sec->walls)
		{
			double factor = InterceptLineSegments(start.X, start.Y, delta.X, delta.Y, wal.pos.X, wal.pos.Y, wal.delta().X, wal.delta().Y, nullptr, true);
			if (factor < 0 || factor >= 1) continue;

			if (!wal.twoSided() || wal.cstat & CSTAT_WALL_1WAY)
				return false;

			auto spot = start + delta * factor;
			double floorz, ceilz;

			for (auto isec : { sec, wal.nextSector() })
			{
				calcSlope(isec, spot, &ceilz, &floorz);

				if (spot.Z <= ceilz || spot.Z >= floorz)
					return false;
			}
			search.Add(wal.nextSector());
		}
	}
	return search.Check(sect2);
}

//---------------------------------------------------------------------------
//
// taken out of SW.
//
//---------------------------------------------------------------------------

int testpointinquad(const DVector2& pt, const DVector2* quad)
{
	for (int i = 0; i < 4; i++)
	{
		double dist = PointOnLineSide(pt.X, pt.Y, quad[i].X, quad[i].Y, quad[(i + 1) & 3].X - quad[i].X, quad[(i + 1) & 3].Y - quad[i].Y);
		if (dist > 0) return false;
	}
	return true;
}

//==========================================================================
//
// 
//
//==========================================================================

double intersectSprite(DCoreActor* actor, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor)
{
	auto end = start + direction;
	if (direction.isZero()) return false;

	// get point on trace that is closest to the sprite
	double factor = NearestPointOnLineFast(actor->spr.pos.X, actor->spr.pos.Y, start.X, start.Y, end.X, end.Y);
	if (factor < 0 || factor > maxfactor) return -1;


	auto sprwidth = tileWidth(actor->spr.picnum) * actor->spr.scale.X * 0.5;
	auto point = start + direction * factor;

	// Using proper distance here, Build originally used the sum of x- and y-distance
	if ((point.XY() - actor->spr.pos.XY()).LengthSquared() > sprwidth * sprwidth * 0.5) return -1; // too far away

	double siz, hitz = actor->spr.pos.Z + actor->GetOffsetAndHeight(siz);

	if (point.Z < hitz - siz || point.Z > hitz)
		return -1;

	result = point;
	return factor;
}

//==========================================================================
//
// 
//
//==========================================================================

double intersectWallSprite(DCoreActor* actor, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor, bool checktex)
{
	DVector2 points[2];

	GetWallSpritePosition(&actor->spr, actor->spr.pos, points, false);

	points[1] -= points[0];
	if ((actor->spr.cstat & CSTAT_SPRITE_ONE_SIDE))   //check for back side of one way sprite
	{
		if (PointOnLineSide(start.X, start.Y, points[0].X, points[0].Y, points[1].X, points[1].Y) > 0)
			return -1;
	}

	// the wall factor is needed for doing a texture check.
	double factor2, factor = InterceptLineSegments(start.X, start.Y, direction.X, direction.Y, points[0].X, points[0].Y, points[1].X, points[1].Y, &factor2);
	if (factor < 0 || factor > maxfactor) return -1;

	result = start + factor * direction;

	double height, position = actor->spr.pos.Z + actor->GetOffsetAndHeight(height);
	if (result.Z <= position - height || result.Z >= position) return -1;

	if (checktex)
	{
		int tilenum = actor->spr.picnum;
		tileUpdatePicnum(&tilenum);

		if (tileLoad(tilenum))
		{
			double zfactor = 1. - (position - result.Z) / height;

			// all other flags have been taken care of already by GetWallSpritePosition and GetOffsetAndHeight
			// - but we have to handle the flip flags here to fetch the correct texel.
			if (actor->spr.cstat & CSTAT_SPRITE_XFLIP) factor2 = 1 - factor2;
			if (actor->spr.cstat & CSTAT_SPRITE_YFLIP) zfactor = 1 - zfactor;

			int xtex = int(factor2 * tileWidth(tilenum));
			int ytex = int(zfactor * tileHeight(tilenum));

			auto texel = (tilePtr(tilenum) + tileHeight(tilenum) * xtex + ytex);
			if (*texel == TRANSPARENT_INDEX)
				return -1;
		}
	}
	return factor;
}

//==========================================================================
//
// 
//
//==========================================================================

double intersectFloorSprite(DCoreActor* actor, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor)
{
	if (actor->spr.cstat & CSTAT_SPRITE_ONE_SIDE)
	{
		if ((start.Z > actor->spr.pos.Z) == ((actor->spr.cstat & CSTAT_SPRITE_YFLIP) == 0)) return -1;
	}

	DVector2 points[4];
	GetFlatSpritePosition(actor, actor->spr.pos, points, nullptr, false);
	double factor = (actor->spr.pos.Z - start.Z) / direction.Z;
	if (factor <= 0 || factor > maxfactor) return -1;
	result = start + factor * direction;
	if (!testpointinquad(result.XY(), points)) return -1;
	return factor;
}

//==========================================================================
//
// 
//
//==========================================================================

double intersectSlopeSprite(DCoreActor* actor, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor)
{
	DVector2 points[4];
	double ptz[4];
	GetFlatSpritePosition(actor, actor->spr.pos, points, ptz, false);
	DVector3 pt1(points[0], ptz[0]);
	DVector3 pt2(points[1], ptz[1]);
	DVector3 pt3(points[2], ptz[2]);
	double factor = LinePlaneIntersect(start, direction, pt1, pt2 - pt1, pt3 - pt1);
	if (factor <= 0 || factor > maxfactor) return -1;
	result = start + factor * direction;

	// we can only do this after calculating the actual intersection spot...
	if (actor->spr.cstat & CSTAT_SPRITE_ONE_SIDE)
	{
		double checkz = spriteGetZOfSlopef(&actor->spr, start.XY(), spriteGetSlope(actor));
		if ((start.Z > checkz) == ((actor->spr.cstat & CSTAT_SPRITE_YFLIP) == 0)) return -1;
	}
	if (!testpointinquad(result.XY(), points)) return -1;
	return factor;
}

//==========================================================================
//
// 
//
//==========================================================================

double checkWallHit(walltype* wal, EWallFlags flagmask, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor)
{
	if (PointOnLineSide(start.XY(), wal) > 0) return -1;

	double factor = InterceptLineSegments(start.X, start.Y, direction.X, direction.Y, wal->pos.X, wal->pos.Y, wal->delta().X, wal->delta().Y);
	if (factor < 0 || factor > maxfactor) return -1;	// did not connect.

	result = start + factor * direction;
	if (wal->twoSided() && !(wal->cstat & flagmask))
	{
		// check if the trace passes this wall or hits the upper or lower tier.
		double cz, fz;
		calcSlope(wal->nextSector(), result, &cz, &fz);
		if (result.Z > cz && result.Z < fz) return -2; // trace will pass this wall, i.e. no hit. Return -2 to tell the caller to go on.
	}
	return factor;
}

//==========================================================================
//
// 
//
//==========================================================================

double checkSectorPlaneHit(sectortype* sec, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor)
{
	if (sec->walls.Size() < 3) return -1;
	auto wal = sec->walls.Data();
	double len = wal->Length();

	DVector3 pt1, pt2, pt3;
	double startcz, startfz;
	double p3cz, p3fz;

	pt1.XY() = wal->pos;
	pt2.XY() = wal->point2Wall()->pos;
	pt3.X = pt1.X + pt2.Y - pt1.Y;
	pt3.Y = pt1.Y + pt2.X - pt1.X; // somewhere off the first line.

	calcSlope(sec, start.X, start.Y, &startcz, &startfz);
	calcSlope(sec, pt3.X, pt3.Y, &p3cz, &p3fz);
	double factor;

	for (int i = 0; i < 2; i++)
	{
		bool sloped;

		if (i == 0)
		{
			if (start.Z <= startcz) continue;
			sloped = (sec->ceilingstat & CSTAT_SECTOR_SLOPE) && len > 0;
			pt1.Z = pt2.Z = sec->ceilingz;
			pt3.Z = p3cz;
		}
		else
		{
			if (start.Z >= startfz) continue;
			sloped = (sec->floorstat & CSTAT_SECTOR_SLOPE && len > 0);
			pt1.Z = pt2.Z = sec->floorz;
			pt3.Z = p3fz;
		}

		if (sloped)
		{
			factor = LinePlaneIntersect(start, direction, pt1, pt2 - pt1, pt3 - pt1);
		}
		else
		{
			factor = (pt1.Z - start.Z) / direction.Z;
		}
		if (factor > 0 && factor <= maxfactor)
		{
			result = start + factor * direction;
			return inside(result.X, result.Y, sec) ? factor : -1;
		}
	}

	return -1;
}

//==========================================================================
//
// 
// 
//==========================================================================

int hitscan(const DVector3& start, const sectortype* startsect, const DVector3& vect, HitInfoBase& hitinfo, unsigned cliptype, double maxrange)
{
	double hitfactor = DBL_MAX;

	const auto wallflags = EWallFlags::FromInt(cliptype & 65535);
	const auto spriteflags = ESpriteFlags::FromInt(cliptype >> 16);

	hitinfo.clearObj();
	if (startsect == nullptr)
		return -1;

	if (maxrange > 0)
	{
		hitfactor = maxrange / vect.Length();
		hitinfo.hitpos = start + hitfactor * vect;
	}
	else hitinfo.hitpos.X = hitinfo.hitpos.Y = DBL_MAX;

	BFSSectorSearch search(startsect);
	while (auto sec = search.GetNext())
	{
		DVector3 v;
		double hit = checkSectorPlaneHit(sec, start, vect, v, hitfactor);
		if (hit > 0)
		{
			hitfactor = hit;
			hitinfo.set(sec, nullptr, nullptr, v);
		}

		// check all walls in this sector
		for (auto& w : sec->walls)
		{
			hit = checkWallHit(&w, EWallFlags::FromInt(wallflags), start, vect, v, hitfactor);
			if (hit > 0)
			{
				hitfactor = hit;
				hitinfo.set(sec, &w, nullptr, v);
			}
			else if (hit == -2)
				search.Add(w.nextSector());
		}

		if (!spriteflags)
			continue;

		//Check all sprites in this sector
		TSectIterator<DCoreActor> it(sec);
		while (auto actor = it.Next())
		{
			if (actor->spr.cstat2 & CSTAT2_SPRITE_NOFIND)
				continue;

			if (!(actor->spr.cstat & spriteflags))
				continue;

			hit = -1;
			// we pass hitfactor to the workers because it can shortcut their calculations a lot.
			switch (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK)
			{
			case CSTAT_SPRITE_ALIGNMENT_FACING:
				hit = intersectSprite(actor, start, vect, v, hitfactor);
				break;

			case CSTAT_SPRITE_ALIGNMENT_WALL:
				hit = intersectWallSprite(actor, start, vect, v, hitfactor, (picanm[actor->spr.picnum].sf & PICANM_TEXHITSCAN_BIT));
				break;

			case CSTAT_SPRITE_ALIGNMENT_FLOOR:
				hit = intersectFloorSprite(actor, start, vect, v, hitfactor);
				break;

			case CSTAT_SPRITE_ALIGNMENT_SLOPE:
				hit = intersectSlopeSprite(actor, start, vect, v, hitfactor);
				break;
			}
			if (hit > 0)
			{
				hitfactor = hit;
				hitinfo.set(sec, nullptr, actor, v);
			}
		}
	}

	return 0;
}


//==========================================================================
//
// 
// 
//==========================================================================

bool checkRangeOfWall(walltype* wal, EWallFlags flagmask, const DVector3& pos, double maxdist, double* theZs)
{
	if (!wal->twoSided()) return false;
	if (wal->cstat & flagmask) return false;
	if (PointOnLineSide(pos.XY(), wal) > 0) return false;

	auto nextsect = wal->nextSector();

	// Rather pointless sky check that needs to be kept...
	if (((nextsect->ceilingstat & CSTAT_SECTOR_SKY) == 0) && (pos.Z <= nextsect->ceilingz + 3)) return false;
	if (((nextsect->floorstat & CSTAT_SECTOR_SKY) == 0) && (pos.Z >= nextsect->floorz - 3)) return false;

	auto pos1 = wal->pos;
	auto pos2 = wal->point2Wall()->pos;

	// Checks borrowed from GZDoom.
	DVector2 boxtl = pos - DVector2(maxdist, maxdist);
	DVector2 boxbr = pos + DVector2(maxdist, maxdist);
	if (!BoxInRange(boxtl, boxbr, pos1, pos2)) return false;
	if (BoxOnLineSide(boxtl, boxbr, pos1, pos2 - pos1) != -1) return false;

	auto closest = pos.XY();
	if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE)	// todo: need to check if it makes sense to have this always on.
		SquareDistToSector(closest.X, closest.Y, nextsect, &closest);

	calcSlope(nextsect, closest.X, closest.Y, &theZs[0], &theZs[1]);
	return true;
}

//==========================================================================
//
// 
// 
//==========================================================================

bool checkRangeOfFaceSprite(DCoreActor* itActor, const DVector3& pos, double maxdist, double* theZs)
{
	double dist = maxdist + itActor->clipdist;
	if (abs(pos.X - itActor->spr.pos.X) > dist || abs(pos.Y - itActor->spr.pos.Y) > dist) return false; // Just like Doom: actors are square...
	double h;
	theZs[0] = itActor->spr.pos.Z + itActor->GetOffsetAndHeight(h);
	theZs[1] = theZs[0] - h;
	return true;
}

//==========================================================================
//
// 
// 
//==========================================================================

bool checkRangeOfWallSprite(DCoreActor* itActor, const DVector3& pos, double maxdist, double* theZs)
{
	int t = itActor->time;
	if ((t >= 485 && t <= 487) || t == 315)
	{
		int a = 0;
	}
	DVector2 verts[2];
	GetWallSpritePosition(&itActor->spr, itActor->spr.pos.XY(), verts);
	if (IsCloseToLine(pos.XY(), verts[0], verts[1], maxdist) == EClose::Outside) return false;
	double h;
	theZs[0] = itActor->spr.pos.Z + itActor->GetOffsetAndHeight(h);
	theZs[1] = theZs[0] - h;
	return true;
}

//==========================================================================
//
// 
// 
//==========================================================================

bool checkRangeOfFloorSprite(DCoreActor* itActor, const DVector3& pos, double maxdist, double& theZ)
{
	int cstat = itActor->spr.cstat;
	if ((cstat & (CSTAT_SPRITE_ALIGNMENT_MASK)) < (CSTAT_SPRITE_ALIGNMENT_FLOOR))
		return false;

	double fdaz = spriteGetZOfSlopef(&itActor->spr, pos.XY(), spriteGetSlope(itActor));

	// Only check if sprite's 2-sided or your on the 1-sided side
	if (((cstat & CSTAT_SPRITE_ONE_SIDE) != 0) && ((pos.Z > fdaz) == ((cstat & CSTAT_SPRITE_YFLIP) == 0)))
		return false;

	DVector2 out[4];
	GetFlatSpritePosition(itActor, itActor->spr.pos.XY(), out);

	// expand the area to cover 'maxdist' units more on each side. (i.e. move the edges out)
	auto expand = (itActor->spr.angle - DAngle45).ToVector() * (maxdist + 0.25);	// that's surely not accurate but here we must match Build's original value.
	out[0] += expand; 
	out[1] += expand.Rotated90CCW();
	out[2] -= expand;
	out[3] += expand.Rotated90CW();

	if (!insidePoly(pos.X, pos.Y, out, 4)) return false;

	theZ = fdaz;
	return true;
}

//==========================================================================
//
// 
// 
//==========================================================================

void getzrange(const DVector3& pos, sectortype* sect, double* ceilz, CollisionBase& ceilhit, double* florz, CollisionBase& florhit, double maxdist, uint32_t cliptype)
{
	if (sect == nullptr)
	{
		*ceilz = -FLT_MAX; ceilhit.setVoid();
		*florz = FLT_MAX; florhit.setVoid();
		return;
	}

	const EWallFlags dawalclipmask = EWallFlags::FromInt(cliptype & 65535);
	const ESpriteFlags dasprclipmask = ESpriteFlags::FromInt(cliptype >> 16);

	auto closest = pos.XY();
	if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE)
		SquareDistToSector(closest.X, closest.Y, sect, &closest);

	calcSlope(sect, closest, ceilz, florz);
	ceilhit.setSector(sect);
	florhit.setSector(sect);

	double theZs[2];

	BFSSectorSearch search(sect);
	while (auto sec = search.GetNext())
	{
		for (auto& wal : sec->walls)
		{
			if (checkRangeOfWall(&wal, EWallFlags::FromInt(dawalclipmask), pos, maxdist + 1 / 16., theZs))
			{
				auto nsec = wal.nextSector();
				search.Add(nsec);
				if (/*(pos.Z > theZs[0]) &&*/ (theZs[0] > *ceilz))
				{
					*ceilz = theZs[0];
					ceilhit.setSector(nsec);
				}

				if (/*(pos.Z < theZs[1]) &&*/ (theZs[1] < *florz))
				{
					*florz = theZs[1];
					florhit.setSector(nsec);
				}
			}
			else if (checkRangeOfWall(&wal, EWallFlags::FromInt(dawalclipmask), pos, maxdist + 64., theZs))
			{
				// extend the search distance for neighboring sectors a bit further so that we can find sprites outside the search range extending to our position.
				auto nsec = wal.nextSector();
				search.Add(nsec);
			}
		}
	}

	if (dasprclipmask)
	{
		search.Rewind();
		while (auto sec = search.GetNext())
		{
			TSectIterator<DCoreActor> it(sec);
			while (auto actor = it.Next())
			{
				const int32_t cstat = actor->spr.cstat;

				if (actor->spr.cstat2 & CSTAT2_SPRITE_NOFIND) continue;
				if (cstat & dasprclipmask)
				{
					switch (cstat & CSTAT_SPRITE_ALIGNMENT_MASK)
					{
					case CSTAT_SPRITE_ALIGNMENT_FACING:
						if (!checkRangeOfFaceSprite(actor, pos, maxdist + 1 / 16., theZs)) continue;
						break;

					case CSTAT_SPRITE_ALIGNMENT_WALL:
						if (!checkRangeOfWallSprite(actor, pos, maxdist + 1 / 16., theZs)) continue;
						break;

					case CSTAT_SPRITE_ALIGNMENT_FLOOR:
					case CSTAT_SPRITE_ALIGNMENT_SLOPE:
						if (!checkRangeOfFloorSprite(actor, pos, maxdist, theZs[0])) continue;
						theZs[1] = theZs[0];
						break;
					}

					// Clipping time!
					if ((pos.Z > theZs[0]) && (theZs[0] > *ceilz))
					{
						*ceilz = theZs[0];
						ceilhit.setSprite(actor);
					}

					if ((pos.Z < theZs[1]) && (theZs[1] < *florz))
					{
						*florz = theZs[1];
						florhit.setSprite(actor);
					}
				}
			}
		}
	}
}


//==========================================================================
//
// 
// 
//==========================================================================

void neartag(const DVector3& pos, sectortype* startsect, DAngle angle, HitInfoBase& result, double range, int flags)
{
	auto checkTag = [=](const auto* object)
	{
		return (((flags & NT_Lotag) && object->lotag) || ((flags & NT_Hitag) && object->hitag));
	};

	auto v = DVector3(angle.ToVector() * range * 1.000001, 0); // extend the range a tiny bit so that we really find everything we need.

	result.clearObj();
	result.hitpos.X = result.hitpos.Y = 0;

	if (!startsect || (flags & (NT_Lotag | NT_Hitag)) == 0)
		return;

	BFSSectorSearch search(startsect);

	while (auto sect = search.GetNext())
	{
		for (auto& wal : sect->walls)
		{
			const auto nextsect = wal.nextSector();

			if (PointOnLineSide(pos.XY(), &wal) > 0) continue;

			double factor = InterceptLineSegments(pos.X, pos.Y, v.X, v.Y, wal.pos.X, wal.pos.Y, wal.delta().X, wal.delta().Y);
			if (factor > 0 && factor < 1)
			{
				bool foundsector = (wal.twoSided() && checkTag(nextsect));
				bool foundwall = checkTag(&wal);
#if 0	// does not work if the trace goes right through the vertex between two walls.
				if (!wal.twoSided() && !foundwall && !foundsector)
				{
					// this case was not handled by Build:
					// If we hit an untagged one-sided wall it should both shorten the scan trace and clear all hits beyond.
					// Otherwise this may cause problems with some weirdly shaped sectors.
					result.hitSector = nullptr;
					result.hitWall = nullptr;
					result.hitpos.X = 0;
					v *= factor;
					continue;
				}
#endif
				if (foundsector) result.hitSector = nextsect;
				if (foundwall) result.hitWall = &wal;

				if (foundwall || foundsector)
				{
					v *= factor;
					result.hitpos.X = v.XY().Length();
				}

				if (wal.twoSided())
				{
					search.Add(nextsect);
				}
			}
		}

		if (!(flags & NT_NoSpriteCheck))
		{
			double factor = 1;
			TSectIterator<DCoreActor> it(sect);
			while (auto actor = it.Next())
			{
				if (actor->spr.cstat2 & CSTAT2_SPRITE_NOFIND)
					continue;

				if (checkTag(&actor->spr))
				{
					DVector3 spot;
					double newfactor = intersectSprite(actor, pos, v, spot, factor - 1. / 65536.);
					if (newfactor > 0)
					{
						factor = newfactor;
						result.hitActor = actor;
						v = spot - pos;
						// return distance to sprite in a separate variable because there is 
						// no means to determine what it is for if both a sprite and wall are found.
						// Only SW's NearTagList actually uses it.
						result.hitpos.Y = v.XY().Length();
					}
				}
			}
		}
	}
}

//==========================================================================
//
// 
// 
//==========================================================================

bool checkOpening(const DVector2& inpos, double z, const sectortype* sec, const sectortype* nextsec, double ceilingdist, double floordist, bool precise)
{
	DVector2 pos;
	if (precise) SquareDistToSector(inpos.X, inpos.Y, nextsec, &pos);
	else pos = inpos;

	double c1, c2, f1, f2;
	calcSlope(sec, pos.X, pos.Y, &c1, &f1);
	calcSlope(nextsec, pos.X, pos.Y, &c2, &f2);

	if (precise)
	{
		double sech = abs(f1 - c1);
		double nextsech = abs(f2 - c2);
		if (sech > nextsech && nextsech < ceilingdist + 2) return 1;
	}

	return ((f2 < f1 - 1 && (nextsec->floorstat & CSTAT_SECTOR_SKY) == 0 && z >= f2 - (floordist - zmaptoworld)) ||
		(c2 > c1 + 1 && (nextsec->ceilingstat & CSTAT_SECTOR_SKY) == 0 && z <= c2 + (ceilingdist - zmaptoworld)));
}

//==========================================================================
//
//
//
//==========================================================================

int pushmove(DVector3& pos, sectortype** pSect, double walldist, double ceildist, double floordist, unsigned cliptype)
{
	auto wallflags = EWallFlags::FromInt(cliptype & 65535);
	int maxhitwalls = 0;
	bool pushed = true;

	for(int direction = 1; pushed; direction = -direction)
	{
		pushed = false;

		if (*pSect == nullptr)
			return -1;

		BFSSectorSearch search(*pSect);

		while (auto sec = search.GetNext())
		{
			// this must go both forward and backward so we cannot use iterators. Pity
			for (unsigned i = 0; i < sec->walls.Size(); i++)
			{
				auto wal = direction > 0 ? &sec->walls[i] : &sec->walls[sec->walls.Size() - 1 - i];

				if (IsCloseToWall(pos.XY(), wal, walldist - 0.25) == EClose::InFront)
				{
					bool blocked = false;
					if (!wal->twoSided() || wal->cstat & wallflags) blocked = true;
					else
					{
						auto pvect = NearestPointOnWall(pos.X, pos.Y, wal);
						blocked = checkOpening(pvect, pos.Z, sec, wal->nextSector(), ceildist, floordist);
					}

					if (blocked)
					{
						auto dv = wal->delta().Rotated90CCW().Unit() * 0.5;
						for (int t = 0; t < 16; t++)
						{
							pos += dv;
							if (IsCloseToWall(pos, wal, (walldist - 0.25)) == EClose::Outside) break;
						}
						pushed = true;

						updatesector(pos, pSect);
						if (++maxhitwalls >= 32) return -1;
						if (*pSect == nullptr) return -1;
					}
					else
						search.Add(wal->nextSector());
				}
			}
		}
		if (!pushed) return 0;
	}
	return -1;
}

//==========================================================================
//
//
//
//==========================================================================

int FindBestSector(const DVector3& pos)
{
	int bestnum = -1;
	double bestdist = FLT_MAX;
	for (int secnum = (int)sector.Size() - 1; secnum >= 0; secnum--)
	{
		auto sect = &sector[secnum];
		if (inside(pos.X, pos.Y, sect))
		{
			double ceilz, floorz;
			calcSlope(sect, pos.X, pos.Y, &ceilz, &floorz);

			if (pos.Z < ceilz)
			{
				// above ceiling
				double dist = ceilz - pos.Z;
				if (dist < bestdist)
				{
					bestnum = secnum;
					bestdist = dist;
				}
			}
			else if (pos.Z > floorz)
			{
				// below floor
				double dist = pos.Z - floorz;
				if (dist < bestdist)
				{
					bestnum = secnum;
					bestdist = dist;
				}
			}
			else
			{
				// inside sector
				return secnum;
			}
		}
	}
	return bestnum;
}

//==========================================================================
//
//
//
//==========================================================================

bool isAwayFromWall(DCoreActor* ac, double delta)
{
	sectortype* s1;

	updatesector(ac->spr.pos + DVector2(delta, delta), &s1);
	if (s1 == ac->sector())
	{
		updatesector(ac->spr.pos - DVector2(delta, delta), &s1);
		if (s1 == ac->sector())
		{
			updatesector(ac->spr.pos + DVector2(delta, -delta), &s1);
			if (s1 == ac->sector())
			{
				updatesector(ac->spr.pos + DVector2(-delta, delta), &s1);
				if (s1 == ac->sector())
					return true;
			}
		}
	}
	return false;
}

//==========================================================================
//
// 
//
//==========================================================================

tspritetype* renderAddTsprite(tspriteArray& tsprites, DCoreActor* actor)
{
	auto tspr = tsprites.newTSprite();

	tspr->pos = actor->spr.pos;
	tspr->cstat = actor->spr.cstat;
	tspr->picnum = actor->spr.picnum;
	tspr->shade = actor->spr.shade;
	tspr->pal = actor->spr.pal;
	tspr->clipdist = 0;
	tspr->blend = actor->spr.blend;
	tspr->scale = actor->spr.scale;
	tspr->xoffset = actor->spr.xoffset;
	tspr->yoffset = actor->spr.yoffset;
	tspr->sectp = actor->spr.sectp;
	tspr->statnum = actor->spr.statnum;
	tspr->angle = actor->spr.angle;
	tspr->xint = actor->spr.xint;
	tspr->yint = actor->spr.yint;
	tspr->inittype = actor->spr.inittype; // not used by tsprites.
	tspr->lotag = actor->spr.lotag;
	tspr->hitag = actor->spr.hitag;
	tspr->extra = actor->spr.extra;
	tspr->time = actor->time;
	tspr->cstat2 = actor->spr.cstat2;
	tspr->ownerActor = actor;

	// need to copy the slope sprite flag around because for tsprites the bit combination means 'voxel'.
	if ((tspr->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_SLOPE)
	{
		tspr->cstat &= ~CSTAT_SPRITE_ALIGNMENT_WALL;
		tspr->clipdist |= TSPR_SLOPESPRITE;
	}

	return tspr;
}

//==========================================================================
//
// vector serializers
//
//==========================================================================

FSerializer& Serialize(FSerializer& arc, const char* key, vec2_t& c, vec2_t* def)
{
	if (arc.isWriting() && def && !memcmp(&c, def, sizeof(c))) return arc;
	if (arc.BeginObject(key))
	{
		arc("x", c.X, def ? &def->X : nullptr)
			("y", c.Y, def ? &def->Y : nullptr)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* key, vec3_t& c, vec3_t* def)
{
	if (arc.isWriting() && def && !memcmp(&c, def, sizeof(c))) return arc;
	if (arc.BeginObject(key))
	{
		arc("x", c.X, def ? &def->X : nullptr)
			("y", c.Y, def ? &def->Y : nullptr)
			("z", c.Z, def ? &def->Z : nullptr)
			.EndObject();
	}
	return arc;
}
