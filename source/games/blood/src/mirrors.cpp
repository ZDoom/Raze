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

#include "build.h"
#include "automap.h"
#include "savegamehelp.h"

#include "blood.h" 
#include "render.h"

BEGIN_BLD_NS

int mirrorcnt, mirrorsector, mirrorwall[4];

MIRROR mirror[16]; // only needed by Polymost.

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitMirrors(void)
{
	mirrorcnt = 0;
	tileDelete(504);
	portalClear();

	for (int i = 0; i < 16; i++)
	{
		tileDelete(4080 + i);
	}
	for (int i = (int)wall.Size() - 1; i >= 0; i--)
	{
		auto pWalli = &wall[i];
		if (mirrorcnt == 16)
			break;
		int nTile = 4080 + mirrorcnt;
		if (pWalli->overpicnum == 504)
		{
			if (pWalli->extra > 0 && pWalli->type == kWallStack)
			{
				pWalli->overpicnum = nTile;

				mirror[mirrorcnt].wallnum = i;
				mirror[mirrorcnt].type = 0;
				pWalli->cstat |= CSTAT_WALL_1WAY;
				int tmp = pWalli->xw().data;
				int j;
				for (j = (int)wall.Size() - 1; j >= 0; j--)
				{
					if (j == i)
						continue;
					auto pWallj = &wall[j];
					if (pWallj->extra > 0 && pWallj->type == kWallStack)
					{
						if (tmp != pWallj->xw().data)
							continue;
						pWalli->hitag = j; // hitag is only used by Polymost, the new renderer uses external links.
						pWallj->hitag = i;
						mirror[mirrorcnt].link = j;
						break;
					}
				}
				if (j < 0)
				{
					Printf(PRINT_HIGH, "wall[%d] has no matching wall link! (data=%d)\n", i, tmp);
				}
				else
				{
					mirrorcnt++;
					pWalli->portalflags = PORTAL_WALL_VIEW;
					pWalli->portalnum = j;
				}
			}
			continue;
		}
		if (pWalli->picnum == 504)
		{
			mirror[mirrorcnt].link = i;
			mirror[mirrorcnt].wallnum = i;
			pWalli->picnum = nTile;
			mirror[mirrorcnt].type = 0;
			pWalli->cstat |= CSTAT_WALL_1WAY;
			pWalli->portalflags = PORTAL_WALL_MIRROR;
			mirrorcnt++;
			continue;
		}
	}
	for (int i = (int)sector.Size() - 1; i >= 0; i--)
	{
		if (mirrorcnt >= 15)
			break;

		auto secti = &sector[i];
		if (secti->floorpicnum == 504)
		{
			auto link = barrier_cast<DBloodActor*>(secti->upperLink);
			if (link == nullptr)
				continue;
			auto link2 = link->GetOwner();
			if (link2 == nullptr)
				continue;

			auto sectj = link2->sector();
			int j = sectnum(sectj);
			if (sectj->ceilingpicnum != 504)
				I_Error("Lower link sector %d doesn't have mirror picnum\n", j);
			mirror[mirrorcnt].type = 2;
			mirror[mirrorcnt].diff = link2->spr.pos - link->spr.pos;
			mirror[mirrorcnt].wallnum = i;
			mirror[mirrorcnt].link = j;
			secti->floorpicnum = 4080 + mirrorcnt;
			secti->portalflags = PORTAL_SECTOR_FLOOR;
			secti->portalnum = portalAdd(PORTAL_SECTOR_FLOOR, j, mirror[mirrorcnt].diff);
			mirrorcnt++;
			mirror[mirrorcnt].type = 1;
			mirror[mirrorcnt].diff = link->spr.pos - link2->spr.pos;
			mirror[mirrorcnt].wallnum = j;
			mirror[mirrorcnt].link = i;
			sectj->ceilingpicnum = 4080 + mirrorcnt;
			sectj->portalflags = PORTAL_SECTOR_CEILING;
			sectj->portalnum = portalAdd(PORTAL_SECTOR_CEILING, i, mirror[mirrorcnt].diff);
			mirrorcnt++;
		}
	}
	mirrorsector = sector.Size();
	mergePortals();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, MIRROR& w, MIRROR* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("type", w.type)
			("link", w.link)
			("diff", w.diff)
			("wallnum", w.wallnum)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SerializeMirrors(FSerializer& arc)
{
	if (arc.BeginObject("mirror"))
	{
		arc("mirrorcnt", mirrorcnt)
			.Array("mirror", mirror, countof(mirror))
			.EndObject();
	}

	if (arc.isReading())
	{

		tileDelete(504);

		for (int i = 0; i < 16; i++)
		{
			tileDelete(4080 + i);
		}
	}
}

END_BLD_NS
