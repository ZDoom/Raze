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

TArray<int> GlobalSectorList; //This is a shared list. Every client must leave it in the same state as it was when it started.

//---------------------------------------------------------------------------
//
// Unified chasecam function for all games.
//
//---------------------------------------------------------------------------

int cameradist, cameraclock;

bool calcChaseCamPos(int* px, int* py, int* pz, spritetype* pspr, int *psectnum, binangle ang, fixedhoriz horiz, double const smoothratio)
{
	hitdata_t hitinfo;
	binangle daang;
	short bakcstat;
	int newdist;

	assert(*psectnum >= 0 && *psectnum < MAXSECTORS);

	// Calculate new pos to shoot backwards, using averaged values from the big three.
	int nx = gi->chaseCamX(ang);
	int ny = gi->chaseCamY(ang);
	int nz = gi->chaseCamZ(horiz);

	vec3_t pvect = { *px, *py, *pz };
	bakcstat = pspr->cstat;
	pspr->cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
	updatesectorz(*px, *py, *pz, psectnum);
	hitscan(&pvect, *psectnum, nx, ny, nz, &hitinfo, CLIPMASK1);
	pspr->cstat = bakcstat;

	int hx = hitinfo.pos.x - *px;
	int hy = hitinfo.pos.y - *py;

	if (*psectnum < 0)
	{
		return false;
	}

	assert(*psectnum >= 0 && *psectnum < MAXSECTORS);

	// If something is in the way, make pp->camera_dist lower if necessary
	if (abs(nx) + abs(ny) > abs(hx) + abs(hy))
	{
		if (hitinfo.wall >= 0)
		{
			// Push you a little bit off the wall
			*psectnum = hitinfo.sect;
			daang = bvectangbam(wall[wall[hitinfo.wall].point2].x - wall[hitinfo.wall].x,
								wall[wall[hitinfo.wall].point2].y - wall[hitinfo.wall].y);
			newdist = nx * daang.bsin() + ny * -daang.bcos();

			if (abs(nx) > abs(ny))
				hx -= MulScale(nx, newdist, 28);
			else
				hy -= MulScale(ny, newdist, 28);
		}
		else if (hitinfo.sprite < 0)		
		{
			// Push you off the ceiling/floor
			*psectnum = hitinfo.sect;

			if (abs(nx) > abs(ny))
				hx -= (nx >> 5);
			else
				hy -= (ny >> 5);
		}
		else
		{
			// If you hit a sprite that's not a wall sprite - try again.
			spritetype* hspr = &sprite[hitinfo.sprite];

			if (!(hspr->cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
			{
				bakcstat = hspr->cstat;
				hspr->cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
				calcChaseCamPos(px, py, pz, pspr, psectnum, ang, horiz, smoothratio);
				hspr->cstat = bakcstat;
				return false;
			}
			else
			{
				// same as wall calculation.
				daang = buildang(pspr->ang - 512);
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
	updatesectorz(*px, *py, *pz, psectnum);

	return true;
}

//==========================================================================
//
// note that this returns values in renderer coordinate space with inverted sign!
//
//==========================================================================

void PlanesAtPoint(const sectortype* sec, int dax, int day, float* pceilz, float* pflorz)
{
	float ceilz = float(sec->ceilingz);
	float florz = float(sec->floorz);

	if (((sec->ceilingstat | sec->floorstat) & CSTAT_SECTOR_SLOPE) == CSTAT_SECTOR_SLOPE)
	{
		auto wal = &wall[sec->wallptr];
		auto wal2 = &wall[wal->point2];

		float dx = float(wal2->x - wal->x);
		float dy = float(wal2->y - wal->y);

		int i = (int)sqrt(dx * dx + dy * dy) << 5; // length of sector's first wall.
		if (i != 0)
		{
			float const j = (dx * (float(day - wal->y)) - dy * (float(dax - wal->x))) * (1.f / 8.f);
			if (sec->ceilingstat & CSTAT_SECTOR_SLOPE) ceilz += (sec->ceilingheinum * j) / i;
			if (sec->floorstat & CSTAT_SECTOR_SLOPE) florz += (sec->floorheinum * j) / i;
		}
	}
	// Scale to render coordinates.
	if (pceilz) *pceilz = ceilz * -(1.f / 256.f);
	if (pflorz) *pflorz = florz * -(1.f / 256.f);
}

//==========================================================================
//
// Calculate the position of a wall sprite in the world
//
//==========================================================================

void GetWallSpritePosition(const spritetype* spr, vec2_t pos, vec2_t* out, bool render)
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

	out[0].x = pos.x - MulScale(x, origin, 16);
	out[0].y = pos.y - MulScale(y, origin, 16);
	out[1].x = out[0].x + MulScale(x, width, 16);
	out[1].y = out[0].y + MulScale(y, width, 16);
}


//==========================================================================
//
// Calculate the position of a wall sprite in the world
//
//==========================================================================

void GetFlatSpritePosition(const spritetype* spr, vec2_t pos, vec2_t* out, bool render)
{
	auto tex = tileGetTexture(spr->picnum);

	int width, height, leftofs, topofs;
	if (render && hw_hightile && TileFiles.tiledata[spr->picnum].hiofs.xsize)
	{
		width = TileFiles.tiledata[spr->picnum].hiofs.xsize * spr->xrepeat;
		height = TileFiles.tiledata[spr->picnum].hiofs.ysize * spr->yrepeat;
		leftofs = (TileFiles.tiledata[spr->picnum].hiofs.xoffs + spr->xoffset) * spr->xrepeat;
		topofs = (TileFiles.tiledata[spr->picnum].hiofs.yoffs + spr->yoffset) * spr->yrepeat;
	}
	else
	{
		width = (int)tex->GetDisplayWidth() * spr->xrepeat;
		height = (int)tex->GetDisplayHeight() * spr->yrepeat;
		leftofs = ((int)tex->GetDisplayLeftOffset() + spr->xoffset) * spr->xrepeat;
		topofs = ((int)tex->GetDisplayTopOffset() + spr->yoffset) * spr->yrepeat;
	}

	if (spr->cstat & CSTAT_SPRITE_XFLIP) leftofs = -leftofs;
	if (spr->cstat & CSTAT_SPRITE_YFLIP) topofs = -topofs;

	int sprcenterx = (width >> 1) + leftofs;
	int sprcentery = (height >> 1) + topofs;

	int cosang = bcos(spr->ang);
	int sinang = bsin(spr->ang);

	out[0].x = pos.x + DMulScale(sinang, sprcenterx, cosang, sprcentery, 16);
	out[0].y = pos.y + DMulScale(sinang, sprcentery, -cosang, sprcenterx, 16);

	out[1].x = out[0].x - MulScale(sinang, width, 16);
	out[1].y = out[0].y + MulScale(cosang, width, 16);

	vec2_t sub = { MulScale(cosang, height, 16), MulScale(sinang, height, 16) };
	out[2] = out[1] - sub;
	out[3] = out[0] - sub;
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
	for (int i = 0; i < numwalls; ++i)
	{
		if (wall[i].cstat & CSTAT_WALL_ROTATE_90)
		{
			auto& w = wall[i];
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
// vector serializers
//
//==========================================================================

FSerializer& Serialize(FSerializer& arc, const char* key, vec2_t& c, vec2_t* def)
{
	if (arc.isWriting() && def && !memcmp(&c, def, sizeof(c))) return arc;
	if (arc.BeginObject(key))
	{
		arc("x", c.x, def ? &def->x : nullptr)
			("y", c.y, def ? &def->y : nullptr)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* key, vec3_t& c, vec3_t* def)
{
	if (arc.isWriting() && def && !memcmp(&c, def, sizeof(c))) return arc;
	if (arc.BeginObject(key))
	{
		arc("x", c.x, def ? &def->x : nullptr)
			("y", c.y, def ? &def->y : nullptr)
			("z", c.z, def ? &def->z : nullptr)
			.EndObject();
	}
	return arc;
}
