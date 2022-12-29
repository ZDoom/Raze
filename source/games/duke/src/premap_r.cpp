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
#include "names_r.h"
#include "mapinfo.h"
#include "dukeactor.h"
#include "precache.h"

BEGIN_DUKE_NS 

static inline void tloadtile(int tilenum, int palnum = 0)
{
	assert(tilenum < MAXTILES);
	markTextureForPrecache(tileGetTextureID(tilenum), palnum);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void cachespritenum(DDukeActor* actor)
{
	int maxc;
	int j;
	int pal = actor->spr.pal;

	if (ud.monsters_off && badguy(actor)) return;

	maxc = 1;

	switch (actor->spr.picnum)
	{
	case RTILE_HYDRENT:
		tloadtile(RTILE_BROKEFIREHYDRENT);
		for (j = RTILE_TOILETWATER; j < (RTILE_TOILETWATER + 4); j++)
			tloadtile(j, pal);
		break;
	case RTILE_TOILETSEAT:
	case RTILE_TOILET2:
		tloadtile(actor->spr.picnum, pal);
		break;
	case RTILE_TOILET:
		tloadtile(RTILE_TOILETBROKE);
		for (j = RTILE_TOILETWATER; j < (RTILE_TOILETWATER + 4); j++)
			tloadtile(j, pal);
		break;
	case RTILE_STALL:
		tloadtile(RTILE_STALLBROKE);
		for (j = RTILE_TOILETWATER; j < (RTILE_TOILETWATER + 4); j++)
			tloadtile(j, pal);
		break;
	case RTILE_FORCERIPPLE:
		maxc = 9;
		break;
	case RTILE_TOILETWATER:
		maxc = 4;
		break;
	case RTILE_BUBBASTAND:
		for (j = RTILE_BUBBASCRATCH; j <= (RTILE_BUBBASCRATCH + 47); j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case RTILE_BUBBAELVIS:
		if (isRRRA())
			for (j = RTILE_BUBBAELVIS; j <= (RTILE_BUBBAELVIS + 29); j++)
				tloadtile(j, pal);
		maxc = 0;
		break;

	case RTILE_COOT:
		for (j = RTILE_COOT; j <= (RTILE_COOT + 217); j++)
			tloadtile(j, pal);
		for (j = RTILE_COOTJIBA; j < RTILE_COOTJIBC + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case RTILE_LTH:
		maxc = 105;
		for (j = RTILE_LTH; j < (RTILE_LTH + maxc); j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case RTILE_BILLYRAY:
		maxc = 144;
		for (j = RTILE_BILLYWALK; j < (RTILE_BILLYWALK + maxc); j++)
			tloadtile(j, pal);
		for (j = RTILE_BILLYJIBA; j <= RTILE_BILLYJIBB + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case RTILE_COW:
		maxc = 56;
		for (j = actor->spr.picnum; j < (actor->spr.picnum + maxc); j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case RTILE_DOGRUN:
		for (j = RTILE_DOGATTACK; j <= RTILE_DOGATTACK + 35; j++)
			tloadtile(j, pal);
		for (j = RTILE_DOGRUN; j <= RTILE_DOGRUN + 80; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case RTILE_RABBIT:
		if (isRRRA())
		{
			for (j = RTILE_RABBIT; j <= RTILE_RABBIT + 54; j++)
				tloadtile(j, pal);
			for (j = RTILE_RABBIT + 56; j <= RTILE_RABBIT + 56 + 49; j++)
				tloadtile(j, pal);
			for (j = RTILE_RABBIT + 56; j <= RTILE_RABBIT + 56 + 49; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case RTILE_BIKERB:
	case RTILE_BIKERBV2:
		if (isRRRA())
		{
			for (j = RTILE_BIKERB; j <= RTILE_BIKERB + 104; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case RTILE_BIKER:
		if (isRRRA())
		{
			for (j = RTILE_BIKER; j <= RTILE_BIKER + 116; j++)
				tloadtile(j, pal);
			for (j = RTILE_BIKER + 150; j <= RTILE_BIKER + 150 + 104; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case RTILE_CHEER:
		if (isRRRA())
		{
			for (j = RTILE_CHEER; j <= RTILE_CHEER + 44; j++)
				tloadtile(j, pal);
			for (j = RTILE_CHEER + 47; j <= RTILE_CHEER + 47 + 211; j++)
				tloadtile(j, pal);
			for (j = RTILE_CHEER + 262; j <= RTILE_CHEER + 262 + 72; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case RTILE_CHEERB:
		if (isRRRA())
		{
			for (j = RTILE_CHEERB; j <= RTILE_CHEERB + 83; j++)
				tloadtile(j, pal);
			for (j = RTILE_CHEERB + 157; j <= RTILE_CHEERB + 157 + 83; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case RTILE_MAMA:
		if (isRRRA())
		{
			for (j = RTILE_MAMA; j <= RTILE_MAMA + 78; j++)
				tloadtile(j, pal);
			for (j = RTILE_MAMA + 80; j <= RTILE_MAMA + 80 + 7; j++)
				tloadtile(j, pal);
			for (j = RTILE_MAMA + 90; j <= RTILE_MAMA + 90 + 94; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case RTILE_CHEERBOAT:
		if (isRRRA())
		{
			tloadtile(RTILE_CHEERBOAT);
			maxc = 0;
		}
		break;
	case RTILE_HULKBOAT:
		if (isRRRA())
		{
			tloadtile(RTILE_HULKBOAT);
			maxc = 0;
		}
		break;
	case RTILE_MINIONBOAT:
		if (isRRRA())
		{
			tloadtile(RTILE_MINIONBOAT);
			maxc = 0;
		}
		break;
	case RTILE_BILLYPLAY:
		if (isRRRA())
		{
			for (j = RTILE_BILLYPLAY; j <= RTILE_BILLYPLAY + 2; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case RTILE_COOTPLAY:
		if (isRRRA())
		{
			for (j = RTILE_COOTPLAY; j <= RTILE_COOTPLAY + 4; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case RTILE_PIG:
	case RTILE_PIGSTAYPUT:
		maxc = 68;
		break;
	case RTILE_TORNADO:
		maxc = 7;
		break;
	case RTILE_HEN:
	case RTILE_HENSTAND:
		maxc = 34;
		break;
	case RTILE_APLAYER:
		maxc = 0;
		if (ud.multimode > 1)
		{
			maxc = 5;
			for (j = RTILE_APLAYER; j < RTILE_APLAYER + 220; j++)
				tloadtile(j, pal);
			for (j = RTILE_DUKEGUN; j < RTILE_DUKELEG + 4; j++)
				tloadtile(j, pal);
		}
		break;
	case RTILE_ATOMICHEALTH:
		maxc = 14;
		break;
	case RTILE_DRONE:
		maxc = 6;
		break;
	case RTILE_VIXEN:
		maxc = 214;
		for (j = actor->spr.picnum; j < actor->spr.picnum + maxc; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case RTILE_SBMOVE:
		if (!isRRRA())
		{

			maxc = 54;
			for (j = actor->spr.picnum; j < actor->spr.picnum + maxc; j++)
				tloadtile(j, pal);
			maxc = 100;
			for (j = RTILE_SBMOVE; j < RTILE_SBMOVE + maxc; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case RTILE_HULK:
		maxc = 40;
		for (j = actor->spr.picnum - 41; j < actor->spr.picnum + maxc - 41; j++)
			tloadtile(j, pal);
		for (j = RTILE_HULKJIBA; j <= RTILE_HULKJIBC + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case RTILE_MINION:
		maxc = 141;
		for (j = actor->spr.picnum; j < actor->spr.picnum + maxc; j++)
			tloadtile(j, pal);
		for (j = RTILE_MINJIBA; j <= RTILE_MINJIBC + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;


	}

	for (j = actor->spr.picnum; j < (actor->spr.picnum + maxc); j++)
		tloadtile(j, pal);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void cachegoodsprites(void)
{
	int i;

	for (i = RTILE_FOOTPRINTS; i < RTILE_FOOTPRINTS + 3; i++)
		tloadtile(i);

	for (i = RTILE_BURNING; i < RTILE_BURNING + 14; i++)
		tloadtile(i);

	for (i = RTILE_FIRSTGUN; i < RTILE_FIRSTGUN + 10; i++)
		tloadtile(i);

	for (i = RTILE_EXPLOSION2; i < RTILE_EXPLOSION2 + 21; i++)
		tloadtile(i);

	tloadtile(RTILE_BULLETHOLE);

	for (i = RTILE_SHOTGUN; i < RTILE_SHOTGUN + 8; i++)
		tloadtile(i);

	tloadtile(RTILE_FOOTPRINTS);

	for (i = RTILE_JIBS1; i < (RTILE_JIBS5 + 5); i++)
		tloadtile(i);

	for (i = RTILE_SCRAP1; i < (RTILE_SCRAP1 + 19); i++)
		tloadtile(i);

	for (i = RTILE_SMALLSMOKE; i < (RTILE_SMALLSMOKE + 4); i++)
		tloadtile(i);

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void cacheit_r(void)
{
	cachegoodsprites();

	DukeSpriteIterator it;
	while (auto act = it.Next())
	{
		if (act->spr.scale.X != 0 && act->spr.scale.Y != 0 && (act->spr.cstat & CSTAT_SPRITE_INVISIBLE) == 0)
			cachespritenum(act);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void prelevel_r(int g, TArray<DDukeActor*>& actors)
{
	player_struct* p;
	int j;
	int lotaglist;
	TArray<short> lotags;
	int speed = 0;
	int dist;
	int sound;
	sound = 0;

	prelevel_common(g);
	p = &ps[screenpeek];

	if (currentLevel->gameflags & LEVEL_RR_CLEARMOONSHINE)
		ps[myconnectindex].steroids_amount = 0;

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
			ps[0].Exit = ac->spr.pos.XY();
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
				actor->sector()->keyinfo = uint8_t(actor->spr.lotag);
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
