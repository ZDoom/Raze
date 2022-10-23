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


#include "automap.h"
#include "savegamehelp.h"

#include "blood.h" 
#include "render.h"

BEGIN_BLD_NS


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitMirrors(void)
{
	portalClear();

	for (int i = (int)wall.Size() - 1; i >= 0; i--)
	{
		auto pWalli = &wall[i];
		if (pWalli->overtexture == mirrortile)
		{
			if (pWalli->extra > 0 && pWalli->type == kWallStack)
			{
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
						break;
					}
				}
				if (j < 0)
				{
					Printf(PRINT_HIGH, "wall[%d] has no matching wall link! (data=%d)\n", i, tmp);
				}
				else
				{
					pWalli->portalflags = PORTAL_WALL_VIEW;
					pWalli->portalnum = j;
				}
			}
			continue;
		}
		if (pWalli->walltexture == mirrortile)
		{
			pWalli->cstat |= CSTAT_WALL_1WAY;
			pWalli->portalflags = PORTAL_WALL_MIRROR;
			continue;
		}
	}
	for (int i = (int)sector.Size() - 1; i >= 0; i--)
	{
		auto secti = &sector[i];
		if (secti->floortexture == mirrortile)
		{
			auto link = barrier_cast<DBloodActor*>(secti->upperLink);
			if (link == nullptr)
				continue;
			auto link2 = link->GetOwner();
			if (link2 == nullptr)
				continue;

			auto sectj = link2->sector();
			int j = sectindex(sectj);
			if (sectj->ceilingtexture != mirrortile)
				I_Error("Lower link sector %d doesn't have mirror pic\n", j);
			auto diff = link2->spr.pos - link->spr.pos;
			secti->portalflags = PORTAL_SECTOR_FLOOR;
			secti->portalnum = portalAdd(PORTAL_SECTOR_FLOOR, j, diff);
			diff = link->spr.pos - link2->spr.pos;
			sectj->portalflags = PORTAL_SECTOR_CEILING;
			sectj->portalnum = portalAdd(PORTAL_SECTOR_CEILING, i, diff);
		}
	}
	mergePortals();
}

END_BLD_NS
