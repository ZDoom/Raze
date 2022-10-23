//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

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

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"

#include "dukeactor.h"
#include "precache.h"

BEGIN_DUKE_NS 

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void prelevel_d(int g, TArray<DDukeActor*>& actors)
{
	unsigned j;
	TArray<short> lotags;

	prelevel_common(g);

	DukeStatIterator it(STAT_DEFAULT);
	while (auto ac = it.Next())
	{
		LoadActor(ac, -1, -1);

		if (ac->spr.lotag == -1 && (ac->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
		{
			getPlayer(0)->Exit = ac->spr.pos.XY();
		}
		else
			premapcontroller(ac);
	}


	for (auto actor : actors)
	{
		if (actor->exists())
		{
			if (iseffector(actor) && actor->spr.lotag == SE_14_SUBWAY_CAR)
				continue;
			spriteinit(actor, actors);
		}
	}

	for (auto actor : actors)
	{
		if (actor->exists())
		{
			if (iseffector(actor) && actor->spr.lotag == SE_14_SUBWAY_CAR)
				spriteinit(actor, actors);
		}
	}

	it.Reset(STAT_DEFAULT);
	while (auto actor = it.Next())
	{
		auto ext = GetExtInfo(actor->spr.spritetexture());
		if (ext.switchphase == 1 && switches[ext.switchindex].type == SwitchDef::Regular)
		{
			j = lotags.Find(actor->spr.lotag);
			if (j == lotags.Size())
			{
				lotags.Push(actor->spr.lotag);
				DukeStatIterator it1(STAT_EFFECTOR);
				while (auto ac = it1.Next())
				{
					if (ac->spr.lotag == SE_12_LIGHT_SWITCH && ac->spr.hitag == actor->spr.lotag)
						ac->counter = 1;
				}
			}
		}
	}
}

END_DUKE_NS
