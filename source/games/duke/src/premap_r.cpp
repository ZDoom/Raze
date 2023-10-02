//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

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
#include "mapinfo.h"
#include "dukeactor.h"
#include "precache.h"

BEGIN_DUKE_NS 

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void prelevel_r(int g, TArray<DDukeActor*>& actors)
{
	DukePlayer* p;
	int j;
	int lotaglist;
	TArray<short> lotags;
	int speed = 0;
	int dist;
	int sound;
	sound = 0;

	prelevel_common(g);
	p = getPlayer(screenpeek);

	if (currentLevel->gameflags & LEVEL_RR_CLEARMOONSHINE)
		getPlayer(myconnectindex)->steroids_amount = 0;

	if (isRRRA())
	{
		for(auto actor : actors)
		{
			if (!actor->exists()) continue;
			if (actor->spr.pal == 100)
			{
				if (numplayers > 1)
					actor->Destroy();
				else
					actor->spr.pal = 0;
			}
			else if (actor->spr.pal == 101)
			{
				actor->spr.extra = 0;
				actor->spr.hitag = 1;
				actor->spr.pal = 0;
				ChangeActorStat(actor, STAT_BOBBING);
			}
		}
	}

	for (auto&sect: sector)
	{
		auto sectp = &sect;

		switch (sectp->lotag)
		{
		case ST_41_JAILDOOR:
		{
			DukeSectIterator it(sectp);
			dist = 0;
			while (auto act = it.Next())
			{
				if (act->GetClass() == RedneckJaildoorDefClass)
				{
					dist = act->spr.lotag;
					speed = act->spr.hitag;
					act->Destroy();
				}
				if (act->GetClass() == RedneckJaildoorSoundClass)
				{
					sound = act->spr.lotag;
					act->Destroy();
				}
			}
			if (dist == 0)
			{
				// Oh no, we got an incomplete definition.
				if (sectindex(sectp) == 534 && currentLevel->levelNumber == 2007) // fix for bug in RR E2L7 Beaudry Mansion.
				{
					dist = 48;
					speed = 32;
				}
			}
			for(auto& osect: sector)
			{
				if (sectp->hitag == osect.hitag && &osect != sectp)
				{
					// & 32767 to avoid some ordering issues here. 
					// Other code assumes that the lotag is always a sector effector type and can mask the high bit in.
					addjaildoor(dist, speed, sectp->hitag, osect.lotag & 32767, sound, &osect);
				}
			}
			break;
		}
		case ST_42_MINECART:
		{
			sectortype* childsectnum = nullptr;
			dist = 0;
			speed = 0;
			DukeSectIterator it(sectp);
			while (auto act = it.Next())
			{
				if (act->GetClass() == RedneckMinecartDefClass)
				{
					dist = act->spr.lotag;
					speed = act->spr.hitag;
					DukeSpriteIterator itt;
					while(auto act1 = itt.Next())
					{
						if (act1->GetClass() == RedneckMinecartInnerClass)
							if (act1->spr.lotag == act->sectno()) // bad map format design... Should have used a tag instead...
							{
								childsectnum = act1->sector();
								act1->Destroy();
							}
					}
					act->Destroy();
				}
				if (act->GetClass() == RedneckMinecartSoundClass)
				{
					sound = act->spr.lotag;
					act->Destroy();
				}
			}
			addminecart(dist, speed, sectp, sectp->hitag, sound, childsectnum);
			break;
		}
		}
	}

	DukeStatIterator it(STAT_DEFAULT);
	while (auto ac = it.Next())
	{
		LoadActor(ac, -1, -1);

		if (ac->spr.lotag == -1 && (ac->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
		{
			getPlayer(0)->Exit = ac->spr.pos.XY();
		}
		else
		{
			premapcontroller(ac);
		}
	}

	for (auto actor : actors)
	{
		if (!actor->exists()) continue;
		if (actor->GetClass() == RedneckGeometryEffectClass)
		{
			if (geocnt >= MAXGEOSECTORS)
				I_Error("Too many geometry effects");
			if (actor->spr.hitag == 0)
			{
				geosector[geocnt] = actor->sector();
				for (auto actor2 : actors)
				{
					if (actor && actor->spr.lotag == actor2->spr.lotag && actor2 != actor && actor2->GetClass() == actor->GetClass())
					{
						if (actor2->spr.hitag == 1)
						{
							geosectorwarp[geocnt] = actor2->sector();
							geox[geocnt] = actor->spr.pos.X - actor2->spr.pos.X;
							geoy[geocnt] = actor->spr.pos.Y - actor2->spr.pos.Y;
							//geoz[geocnt] = actor->spr.z - actor2->spr.z;
						}
						if (actor2->spr.hitag == 2)
						{
							geosectorwarp2[geocnt] = actor2->sector();
							geox2[geocnt] = actor->spr.pos.X - actor2->spr.pos.X;
							geoy2[geocnt] = actor->spr.pos.Y - actor2->spr.pos.Y;
							//geoz2[geocnt] = actor->spr.z - actor2->spr.z;
						}
					}
				}
				geocnt++;
			}
		}
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
			if (actor->GetClass() == RedneckGeometryEffectClass)
				actor->Destroy();
			if (actor->GetClass() == RedneckKeyinfoSetterClass)
			{
				actor->sector()->lockinfo = uint8_t(actor->spr.lotag);
				actor->Destroy();
			}
		}
	}

	lotaglist = 0;

	it.Reset(STAT_DEFAULT);
	while (auto ac = it.Next())
	{
		auto ext = GetExtInfo(ac->spr.spritetexture());
		if (ext.switchphase == 1 && switches[ext.switchindex].type == SwitchDef::Regular)
		{
			j = lotags.Find(ac->spr.lotag);
			if (j == lotags.Size()) 
			{
				lotags.Push(ac->spr.lotag);
				DukeStatIterator it1(STAT_EFFECTOR);
				while (auto actj = it1.Next())
				{
					if (actj->spr.lotag == SE_12_LIGHT_SWITCH && actj->spr.hitag == ac->spr.lotag)
						actj->counter = 1;
				}
			}
		}
	}
 }


END_DUKE_NS
