//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers

This file is part of Raze

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "dukeactor.h"
#include "gamefuncs.h"
#include "models/modeldata.h"

BEGIN_DUKE_NS

void drawshadows(tspriteArray& tsprites, tspritetype* t, DDukeActor* h)
{
	if (r_shadows && !(h->spr.cstat2 & CSTAT2_SPRITE_NOSHADOW))
	{
		auto sectp = t->sectp;
		double floorz;

		if ((sectp->lotag & 0xff) > 2 || h->spr.statnum == STAT_PROJECTILE || h->spr.statnum == STAT_MISC || actorflag(h, SFLAG2_FLOATING))
			floorz = sectp->floorz;
		else
			floorz = h->floorz;


		if (h->spr.pos.Z - floorz < 8 && ps[screenpeek].GetActor()->getOffsetZ() < floorz)
		{
			auto shadowspr = tsprites.newTSprite();
			*shadowspr = *t;

			shadowspr->statnum = STAT_TEMP;

			shadowspr->scale.Y = (max(t->scale.Y * 0.125, 0.0625));
			shadowspr->shade = 127;
			shadowspr->cstat |= CSTAT_SPRITE_TRANSLUCENT;

			shadowspr->pos.Z = floorz;
			shadowspr->pal = 4;

			if (hw_models && modelManager.CheckModel(t->picnum, t->pal))
			{
				shadowspr->scale.Y = (0);
				// 512:trans reverse
				//1024:tell MD2SPRITE.C to use Z-buffer hacks to hide overdraw issues
				shadowspr->clipdist |= TSPR_FLAGS_MDHACK;
				shadowspr->cstat |= CSTAT_SPRITE_TRANS_FLIP;
			}
			else
			{
				// Alter the shadow's position so that it appears behind the sprite itself.
				auto look = (shadowspr->pos.XY() - ps[screenpeek].GetActor()->spr.pos.XY()).Angle();
				shadowspr->pos.XY() += look.ToVector() * 2;
			}
		}
	}
}
END_DUKE_NS
