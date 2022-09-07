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

IntRect viewport3d;

//---------------------------------------------------------------------------
//
// Unified chasecam function for all games.
//
//---------------------------------------------------------------------------

double cameradist, cameraclock;

bool calcChaseCamPos(DVector3& ppos, DCoreActor* act, sectortype** psect, DAngle ang, fixedhoriz horiz, double const interpfrac)
{
	HitInfoBase hitinfo;
	DAngle daang;
	double newdist;

	if (!*psect) return false;

	// Calculate new pos to shoot backwards
	DVector3 npos = gi->chaseCamPos(ang, horiz);

	auto bakcstat = act->spr.cstat;
	act->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	updatesectorz(ppos, psect);
	hitscan(ppos, *psect, npos, hitinfo, CLIPMASK1);
	act->spr.cstat = bakcstat;
	auto hpos = hitinfo.hitpos.XY() - ppos.XY();

	if (!*psect) return false;

	// If something is in the way, make cameradist lower if necessary
	if (fabs(npos.X) + fabs(npos.Y) > fabs(hpos.X) + fabs(hpos.Y))
	{
		if (hitinfo.hitWall != nullptr)
		{
			// Push you a little bit off the wall
			*psect = hitinfo.hitSector;
			daang = hitinfo.hitWall->delta().Angle();
			newdist = (npos.X * daang.Sin() + npos.Y * -daang.Cos()) * (1. / 1024.);

			if (fabs(npos.X) > fabs(npos.Y))
				hpos.X -= npos.X * newdist;
			else
				hpos.Y -= npos.Y * newdist;
		}
		else if (hitinfo.hitActor == nullptr)		
		{
			// Push you off the ceiling/floor
			*psect = hitinfo.hitSector;

			if (fabs(npos.X) > fabs(npos.Y))
				hpos.X -= npos.X * (1. / 32.);
			else
				hpos.Y -= npos.Y * (1. / 32.);
		}
		else
		{
			// If you hit a sprite that's not a wall sprite - try again.
			auto hit = hitinfo.hitActor;

			if (!(hit->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
			{
				bakcstat = hit->spr.cstat;
				hit->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
				calcChaseCamPos(ppos, act, psect, ang, horiz, interpfrac);
				hit->spr.cstat = bakcstat;
				return false;
			}
			else
			{
				// same as wall calculation.
				daang = act->spr.angle - DAngle90;
				newdist = (npos.X * daang.Sin() + npos.Y * -daang.Cos()) * (1. / 1024.);

				if (fabs(npos.X) > fabs(npos.Y))
					hpos.X -= npos.X * newdist;
				else
					hpos.Y -= npos.Y * newdist;
			}
		}

		newdist = fabs(npos.X) > fabs(npos.Y) ? hpos.X / npos.X : hpos.Y / npos.Y;
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

void calcSlope(const sectortype* sec, float xpos, float ypos, float* pceilz, float* pflorz)
{
	int bits = 0;
	if (pceilz)
	{
		bits |= sec->ceilingstat;
		*pceilz = float(sec->int_ceilingz());
	}
	if (pflorz)
	{
		bits |= sec->floorstat;
		*pflorz = float(sec->int_floorz());
	}

	if ((bits & CSTAT_SECTOR_SLOPE) == CSTAT_SECTOR_SLOPE)
	{
		auto wal = sec->firstWall();
		int len = wal->Length();
		if (len != 0)
		{
			float fac = (wal->int_delta().X * (float(ypos - wal->wall_int_pos().Y)) - wal->int_delta().Y * (float(xpos - wal->wall_int_pos().X))) * (1.f / 256.f) / len;
			if (pceilz && sec->ceilingstat & CSTAT_SECTOR_SLOPE) *pceilz += (sec->ceilingheinum * fac);
			if (pflorz && sec->floorstat & CSTAT_SECTOR_SLOPE) *pflorz += (sec->floorheinum * fac);
		}
	}
}

//==========================================================================
//
// for the renderer
//
//==========================================================================

void PlanesAtPoint(const sectortype* sec, float dax, float day, float* pceilz, float* pflorz)
{
	calcSlope(sec, dax * worldtoint, day * worldtoint, pceilz, pflorz);
	if (pceilz) *pceilz *= -(1 / 256.f);
	if (pflorz) *pflorz *= -(1 / 256.f);
}

//==========================================================================
//
// for the games (these are not inlined so that they can inline calcSlope)
//
//==========================================================================

int getceilzofslopeptr(const sectortype* sec, int dax, int day)
{
	float z;
	calcSlope(sec, dax, day, &z, nullptr);
	return int(z);
}

int getflorzofslopeptr(const sectortype* sec, int dax, int day)
{
	float z;
	calcSlope(sec, dax, day, nullptr, &z);
	return int(z);
}

void getzsofslopeptr(const sectortype* sec, int dax, int day, int* ceilz, int* florz)
{
	float c, f;
	calcSlope(sec, dax, day, &c, &f);
	*ceilz = int(c);
	*florz = int(f);
}

void getzsofslopeptr(const sectortype* sec, double dax, double day, double* ceilz, double* florz)
{
	float c, f;
	calcSlope(sec, dax * worldtoint, day * worldtoint, &c, &f);
	*ceilz = c * zinttoworld;
	*florz = f * zinttoworld;
}

void getcorrectzsofslope(int sectnum, int dax, int day, int* ceilz, int* florz)
{
	DVector2 closestv;
	SquareDistToSector(dax * inttoworld, day * inttoworld, &sector[sectnum], &closestv);
	float ffloorz, fceilz;
	calcSlope(&sector[sectnum], closestv.X * worldtoint, closestv.Y * worldtoint, &fceilz, &ffloorz);
	if (ceilz) *ceilz = int(fceilz);
	if (florz) *florz = int(ffloorz);
}

//==========================================================================
//
// 
//
//==========================================================================

int getslopeval(sectortype* sect, int x, int y, int z, int basez)
{
	auto wal = sect->firstWall();
	auto delta = wal->int_delta();
	int i = (y - wal->wall_int_pos().Y) * delta.X - (x - wal->wall_int_pos().X) * delta.Y;
	return i == 0? 0 : Scale((z - basez) << 8, wal->Length(), i);
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
	for (auto& wal : wallsofsector(sect))
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

	double x = spr->angle.Sin() * spr->xrepeat * (1. / 64.);
	double y = -spr->angle.Cos() * spr->xrepeat * (1. / 64.);

	if (spr->cstat & CSTAT_SPRITE_XFLIP) xoffset = -xoffset;
	double origin = (width * 0.5) + xoffset;

	out[0].X = pos.X - x * origin;
	out[0].Y = pos.Y - y * origin;
	out[1].X = out[0].X + x * width;
	out[1].Y = out[0].Y + y * width;
}


//==========================================================================
//
// Calculate the position of a wall sprite in the world
//
//==========================================================================

void TGetFlatSpritePosition(const spritetypebase* spr, const DVector2& pos, DVector2* out, double* outz, int heinum, bool render)
{
	auto tex = tileGetTexture(spr->picnum);

	double width, height, leftofs, topofs;
	double sloperatio = sqrt(heinum * heinum + 4096 * 4096) * (1. / 4096.);
	double xrepeat = spr->xrepeat * (1. / 64.);
	double yrepeat = spr->yrepeat * (1. / 64.);

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
				outz[i] = (sinang * (out[i].Y - pos.Y) + cosang * (out[i].X - pos.X)) * heinum * (1. / 4096);
			}
		}
	}
}


void GetFlatSpritePosition(DCoreActor* actor, const DVector2& pos, DVector2* out, bool render)
{
	TGetFlatSpritePosition(&actor->spr, pos, out, nullptr, spriteGetSlope(actor), render);
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
	for (auto& wal : wallsofsector(sect1))
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

void dragpoint(walltype* startwall, int newx, int newy)
{
	vertexscan(startwall, [&](walltype* wal)
	{
		wal->movexy(newx, newy);
		wal->sectorp()->exflags |= SECTOREX_DRAGGED;
	});
}

void dragpoint(walltype* startwall, const DVector2& pos)
{
	vertexscan(startwall, [&](walltype* wal)
		{
			wal->move(pos);
			wal->sectorp()->exflags |= SECTOREX_DRAGGED;
		});
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
		for (auto& wal : wallsofsector(sect))
		{
			// Perform the checks here in 48.16 fixed point.
			// Doing it directly with floats and multiplications does not work reliably.
			// Unfortunately, due to the conversions, this is a bit slower. :(
			int64_t xs = int64_t(0x10000 * (wal.pos.X - x));
			int64_t ys = int64_t(0x10000 * (wal.pos.Y - y));
			auto wal2 = wal.point2Wall();
			int64_t xe = int64_t(0x10000 * (wal2->pos.X - x));
			int64_t ye = int64_t(0x10000 * (wal2->pos.Y - y));

			if ((ys ^ ye) < 0)
			{
				int64_t val;

				if ((xs ^ xe) >= 0) val = xs;
				else val = ((xs * ye) - xe * ys) ^ ye;
				acc ^= val;
			}
		}
		return acc < 0;
	}
	return -1;
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
	for(auto& wal : wallsofsector(sectp))
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
	tspr->xrepeat = actor->spr.xrepeat;
	tspr->yrepeat = actor->spr.yrepeat;
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
