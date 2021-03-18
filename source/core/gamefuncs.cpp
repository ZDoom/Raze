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


//---------------------------------------------------------------------------
//
// Unified chasecam function for all games.
//
//---------------------------------------------------------------------------

int cameradist, cameraclock;

bool calcChaseCamPos(int* px, int* py, int* pz, spritetype* pspr, short *psectnum, binangle ang, fixedhoriz horiz, double const smoothratio)
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
	int myclock = PlayClock + MulScale(120 / GameTicRate, smoothratio, 16);
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool spriteIsModelOrVoxel(const spritetype * tspr)
{
	if ((unsigned)tspr->owner < MAXSPRITES && spriteext[tspr->owner].flags & SPREXT_NOTMD)
		return false;

	if (hw_models)
	{
		auto& mdinfo = tile2model[Ptile2tile(tspr->picnum, tspr->pal)];
		if (mdinfo.modelid >= 0 && mdinfo.framenum >= 0) return true;
	}

	auto slabalign = (tspr->cstat & CSTAT_SPRITE_ALIGNMENT) == CSTAT_SPRITE_ALIGNMENT_SLAB;
	if (r_voxels && !slabalign && tiletovox[tspr->picnum] >= 0 && voxmodels[tiletovox[tspr->picnum]]) return true;
	return (slabalign && voxmodels[tspr->picnum]);
}

//==========================================================================
//
// note that this returns values in renderer coordinate space with inverted sign!
//
//==========================================================================

void PlanesAtPoint(usectorptr_t sec, float dax, float day, float* pceilz, float* pflorz)
{
	float ceilz = float(sec->ceilingz);
	float florz = float(sec->floorz);

	if (((sec->ceilingstat | sec->floorstat) & CSTAT_SECTOR_SLOPE) == CSTAT_SECTOR_SLOPE)
	{
		auto wal = &wall[sec->wallptr];
		auto wal2 = &wall[wal->point2];

		float dx = wal2->x - wal->x;
		float dy = wal2->y - wal->y;

		int i = (int)sqrt(dx * dx + dy * dy) << 5; // length of sector's first wall.
		if (i != 0)
		{
			float const j = (dx * (day - wal->y) - dy * (dax - wal->x)) * (1.f / 8.f);
			if (sec->ceilingstat & CSTAT_SECTOR_SLOPE) ceilz += (sec->ceilingheinum * j) / i;
			if (sec->floorstat & CSTAT_SECTOR_SLOPE) florz += (sec->floorheinum * j) / i;
		}
	}
	// Scale to render coordinates.
	if (pceilz) *pceilz = ceilz * -(1.f / 256.f);
	if (pflorz) *pflorz = florz * -(1.f / 256.f);
}

