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

//---------------------------------------------------------------------------
//
// Unified chasecam function for all games.
//
//---------------------------------------------------------------------------

int cameradist, cameraclock;

bool calcChaseCamPos(int* px, int* py, int* pz, DCoreActor* act, sectortype** psect, binangle ang, fixedhoriz horiz, double const smoothratio)
{
	HitInfoBase hitinfo;
	binangle daang;
	int newdist;

	if (!*psect) return false;
	// Calculate new pos to shoot backwards, using averaged values from the big three.
	int nx = gi->chaseCamX(ang);
	int ny = gi->chaseCamY(ang);
	int nz = gi->chaseCamZ(horiz);

	auto bakcstat = act->spr.cstat;
	act->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	updatesectorz(*px, *py, *pz, psect);
	hitscan({ *px, *py, *pz }, *psect, { nx, ny, nz }, hitinfo, CLIPMASK1);
	act->spr.cstat = bakcstat;

	int hx = hitinfo.hitpos.X - *px;
	int hy = hitinfo.hitpos.Y - *py;

	if (*psect == nullptr)
	{
		return false;
	}

	// If something is in the way, make pp->camera_dist lower if necessary
	if (abs(nx) + abs(ny) > abs(hx) + abs(hy))
	{
		if (hitinfo.hitWall != nullptr)
		{
			// Push you a little bit off the wall
			*psect = hitinfo.hitSector;
			daang = bvectangbam(hitinfo.hitWall->point2Wall()->pos.X - hitinfo.hitWall->pos.X,
								hitinfo.hitWall->point2Wall()->pos.Y - hitinfo.hitWall->pos.Y);
			newdist = nx * daang.bsin() + ny * -daang.bcos();

			if (abs(nx) > abs(ny))
				hx -= MulScale(nx, newdist, 28);
			else
				hy -= MulScale(ny, newdist, 28);
		}
		else if (hitinfo.hitActor == nullptr)		
		{
			// Push you off the ceiling/floor
			*psect = hitinfo.hitSector;

			if (abs(nx) > abs(ny))
				hx -= (nx >> 5);
			else
				hy -= (ny >> 5);
		}
		else
		{
			// If you hit a sprite that's not a wall sprite - try again.
			auto hit = hitinfo.hitActor;

			if (!(hit->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
			{
				bakcstat = hit->spr.cstat;
				hit->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
				calcChaseCamPos(px, py, pz, act, psect, ang, horiz, smoothratio);
				hit->spr.cstat = bakcstat;
				return false;
			}
			else
			{
				// same as wall calculation.
				daang = buildang(act->spr.ang - 512);
				newdist = nx * daang.bsin() + ny * -daang.bcos();

				if (abs(nx) > abs(ny))
					hx -= MulScale(nx, newdist, 28);
				else
					hy -= MulScale(ny, newdist, 28);
			}
		}

		if (abs(nx) > abs(ny))
			newdist = DivScale(hx, nx, 16);
		else
			newdist = DivScale(hy, ny, 16);

		if (newdist < cameradist)
			cameradist = newdist;
	}

	// Actually move you! (Camerdist is 65536 if nothing is in the way)
	*px += MulScale(nx, cameradist, 16);
	*py += MulScale(ny, cameradist, 16);
	*pz += MulScale(nz, cameradist, 16);

	// Caculate clock using GameTicRate so it increases the same rate on all speed computers.
	int myclock = PlayClock + MulScale(120 / GameTicRate, int(smoothratio), 16);
	if (cameraclock == INT_MIN)
	{
		// Third person view was just started.
		cameraclock = myclock;
	}

	// Slowly increase cameradist until it reaches 65536.
	cameradist = min(cameradist + ((myclock - cameraclock) << 10), 65536);
	cameraclock = myclock;

	// Make sure psectnum is correct.
	updatesectorz(*px, *py, *pz, psect);

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
		*pceilz = float(sec->ceilingz);
	}
	if (pflorz)
	{
		bits |= sec->floorstat;
		*pflorz = float(sec->floorz);
	}

	if ((bits & CSTAT_SECTOR_SLOPE) == CSTAT_SECTOR_SLOPE)
	{
		auto wal = sec->firstWall();
		int len = wal->Length();
		if (len != 0)
		{
			float fac = (wal->deltax() * (float(ypos - wal->pos.Y)) - wal->deltay() * (float(xpos - wal->pos.X))) * (1.f / 256.f) / len;
			if (pceilz && sec->ceilingstat & CSTAT_SECTOR_SLOPE) *pceilz += (sec->ceilingheinum * fac);
			if (pflorz && sec->floorstat & CSTAT_SECTOR_SLOPE) *pflorz += (sec->floorheinum * fac);
		}
	}
}

//==========================================================================
//
// for the renderer (Polymost variants are in polymost.cpp)
//
//==========================================================================

void PlanesAtPoint(const sectortype* sec, float dax, float day, float* pceilz, float* pflorz)
{
	calcSlope(sec, dax, day, pceilz, pflorz);
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

//==========================================================================
//
// 
//
//==========================================================================

int getslopeval(sectortype* sect, int x, int y, int z, int basez)
{
	auto wal = sect->firstWall();
	auto delta = wal->delta();
	int i = (y - wal->pos.Y) * delta.X - (x - wal->pos.X) * delta.Y;
	return i == 0? 0 : Scale((z - basez) << 8, wal->Length(), i);
}


//==========================================================================
//
// Calculate the position of a wall sprite in the world
//
//==========================================================================

void GetWallSpritePosition(const tspritetype* spr, vec2_t pos, vec2_t* out, bool render)
{
	auto tex = tileGetTexture(spr->picnum);

	int width, leftofs;
	if (render && hw_hightile && TileFiles.tiledata[spr->picnum].hiofs.xsize)
	{
		width = TileFiles.tiledata[spr->picnum].hiofs.xsize;
		leftofs = (TileFiles.tiledata[spr->picnum].hiofs.xoffs + spr->xoffset);
	}
	else
	{
		width = (int)tex->GetDisplayWidth();
		leftofs = ((int)tex->GetDisplayLeftOffset() + spr->xoffset);
	}

	int x = bsin(spr->ang) * spr->xrepeat;
	int y = -bcos(spr->ang) * spr->xrepeat;

	int xoff = leftofs;
	if (spr->cstat & CSTAT_SPRITE_XFLIP) xoff = -xoff;
	int origin = (width >> 1) + xoff;

	out[0].X = pos.X - MulScale(x, origin, 16);
	out[0].Y = pos.Y - MulScale(y, origin, 16);
	out[1].X = out[0].X + MulScale(x, width, 16);
	out[1].Y = out[0].Y + MulScale(y, width, 16);
}


//==========================================================================
//
// Calculate the position of a wall sprite in the world
//
//==========================================================================

void TGetFlatSpritePosition(const spritetypebase* spr, vec2_t pos, vec2_t* out, int* outz, int heinum, bool render)
{
	auto tex = tileGetTexture(spr->picnum);

	int width, height, leftofs, topofs;
	int ratio = ksqrt(heinum * heinum + 4096 * 4096);

	int xo = heinum ? 0 : spr->xoffset;
	int yo = heinum ? 0 : spr->yoffset;

	if (render && hw_hightile && TileFiles.tiledata[spr->picnum].hiofs.xsize)
	{
		width = TileFiles.tiledata[spr->picnum].hiofs.xsize * spr->xrepeat;
		height = TileFiles.tiledata[spr->picnum].hiofs.ysize * spr->yrepeat;
		leftofs = (TileFiles.tiledata[spr->picnum].hiofs.xoffs + xo) * spr->xrepeat;
		topofs = (TileFiles.tiledata[spr->picnum].hiofs.yoffs + yo) * spr->yrepeat;
	}
	else
	{
		width = (int)tex->GetDisplayWidth() * spr->xrepeat;
		height = (int)tex->GetDisplayHeight() * spr->yrepeat;
		leftofs = ((int)tex->GetDisplayLeftOffset() + xo) * spr->xrepeat;
		topofs = ((int)tex->GetDisplayTopOffset() + yo) * spr->yrepeat;
	}

	if (spr->cstat & CSTAT_SPRITE_XFLIP) leftofs = -leftofs;
	if (spr->cstat & CSTAT_SPRITE_YFLIP) topofs = -topofs;

	int sprcenterx = (width >> 1) + leftofs;
	int sprcentery = (height >> 1) + topofs;

	int cosang = bcos(spr->ang);
	int sinang = bsin(spr->ang);
	int cosangslope = DivScale(cosang, ratio, 12);
	int sinangslope = DivScale(sinang, ratio, 12);

	out[0].X = pos.X + DMulScale(sinang, sprcenterx, cosangslope, sprcentery, 16);
	out[0].Y = pos.Y + DMulScale(sinangslope, sprcentery, -cosang, sprcenterx, 16);

	out[1].X = out[0].X - MulScale(sinang, width, 16);
	out[1].Y = out[0].Y + MulScale(cosang, width, 16);

	vec2_t sub = { MulScale(cosangslope, height, 16), MulScale(sinangslope, height, 16) };
	out[2] = out[1] - sub;
	out[3] = out[0] - sub;
	if (outz)
	{
		if (!heinum) outz[3] = outz[2] = outz[1] = outz[0] = 0;
		else
		{
			for (int i = 0; i < 4; i++)
			{
				int spos = DMulScale(-sinang, out[i].Y - spr->pos.Y, -cosang, out[i].X - spr->pos.X, 4);
				outz[i] = MulScale(heinum, spos, 18);
			}
		}
	}
}

void GetFlatSpritePosition(DCoreActor* actor, vec2_t pos, vec2_t* out, bool render)
{
	TGetFlatSpritePosition(&actor->spr, pos, out, nullptr, spriteGetSlope(actor), render);
}

void GetFlatSpritePosition(const tspritetype* spr, vec2_t pos, vec2_t* out, int* outz, bool render)
{
	TGetFlatSpritePosition(spr, pos, out, outz, tspriteGetSlope(spr), render);
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
			auto& tile = RotTile(w.picnum + animateoffs(w.picnum, 16384));

			if (tile.newtile == -1 && tile.owner == -1)
			{
				auto owner = w.picnum + animateoffs(w.picnum, 16384);

				tile.newtile = TileFiles.tileCreateRotated(owner);
				assert(tile.newtile != -1);

				RotTile(tile.newtile).owner = w.picnum + animateoffs(w.picnum, 16384);

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
		wal->move(newx, newy);
		wal->sectorp()->exflags |= SECTOREX_DRAGGED;
	});
}

//==========================================================================
//
// 
//
//==========================================================================

tspritetype* renderAddTsprite(tspritetype* tsprite, int& spritesortcnt, DCoreActor* actor)
{
	validateTSpriteSize(tsprite, spritesortcnt);

	if (spritesortcnt >= MAXSPRITESONSCREEN) return nullptr;
	auto tspr = &tsprite[spritesortcnt++];

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
	tspr->ang = actor->spr.ang;
	tspr->xvel = actor->spr.xvel;
	tspr->yvel = actor->spr.yvel;
	tspr->zvel = actor->spr.zvel;
	tspr->lotag = actor->spr.lotag;
	tspr->hitag = actor->spr.hitag;
	tspr->extra = actor->spr.extra;
	tspr->time = actor->time;
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
