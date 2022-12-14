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
#include <utility>
#include "ns.h"
#include "global.h"
#include "sounds.h"
#include "names_d.h"
#include "dukeactor.h"

BEGIN_DUKE_NS


DDukeActor* spawninit_d(DDukeActor* actj, DDukeActor* act, TArray<DDukeActor*>* actors)
{
	if (actorflag(act, SFLAG2_TRIGGERRESPAWN))
	{
		act->spr.yint = act->spr.hitag;
		act->spr.hitag = -1;
	}

	if (iseffector(act))
	{
		// for in-game spawned SE's the init code must not run. The only type ever being spawned that way is SE128 - 
		// but we cannot check that here as the number has not been set yet.
		if (actj == 0) spawneffector(act, actors);
		return act;
	}

	if (act->GetClass() != RUNTIME_CLASS(DDukeActor))
	{
		if (!badguy(act) || commonEnemySetup(act, actj))
			CallInitialize(act);
		return act;
	}
	auto sectp = act->sector();


	if (isWorldTour())
	{
		switch (act->spr.picnum)
		{
		case DTILE_WHISPYSMOKE:
			ChangeActorStat(act, STAT_MISC);
			act->spr.pos.X += krandf(16) - 8;
			act->spr.pos.Y += krandf(16) - 8;
			act->spr.scale = DVector2(0.3125, 0.3125);
			return act;
		case DTILE_SERIOUSSAM:
			ChangeActorStat(act, STAT_ZOMBIEACTOR);
			act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
			act->spr.extra = 150;
			return act;
		}
	}

	switch (act->spr.picnum)
	{
	default:
		if (!badguy(act) || commonEnemySetup(act, actj))
			CallInitialize(act);
		break;
	case DTILE_BLOOD:
		act->spr.scale = DVector2(0.25, 0.25);
		act->spr.pos.Z -= 26;
		if (actj && actj->spr.pal == 6)
			act->spr.pal = 6;
		ChangeActorStat(act, STAT_MISC);
		break;
	case DTILE_FECES:
		if (actj)
			act->spr.scale = DVector2(REPEAT_SCALE, REPEAT_SCALE);
		ChangeActorStat(act, STAT_MISC);
		break;

	case DTILE_DUKELYINGDEAD:
		if (actj && actj->isPlayer())
		{
			act->spr.scale = actj->spr.scale;
			act->spr.shade = actj->spr.shade;
			act->spr.pal = ps[actj->PlayerIndex()].palookup;
		}
		act->spr.cstat = 0;
		act->spr.extra = 1;
		act->vel.X = 292 / 16.;
		act->vel.Z = 360 / 256.;
		[[fallthrough]];
	case DTILE_BLIMP:
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		act->clipdist = 32;
		[[fallthrough]];
	case DTILE_MIKE:
		if (act->spr.picnum == DTILE_MIKE)
			act->spr.yint = act->spr.hitag;
		ChangeActorStat(act, 1);
		break;
	case DTILE_ONFIRE:
		// Twentieth Anniversary World Tour
		if (!isWorldTour())
			break;

		if (actj)
		{
			act->spr.Angles.Yaw = actj->spr.Angles.Yaw;
			act->spr.shade = -64;
			act->spr.cstat = CSTAT_SPRITE_YCENTER | randomXFlip();
		}
		act->spr.scale = DVector2(0.375, 0.375);

		if (actj)
		{
			double x = getflorzofslopeptr(act->sector(), act->spr.pos);
			if (act->spr.pos.Z > x - 12)
				act->spr.pos.Z = x - 12;
		}

		act->spr.pos.X += krandf(32) - 16;
		act->spr.pos.Y += krandf(32) - 16;
		act->spr.pos.Z -= krandf(40);
		act->spr.cstat |= CSTAT_SPRITE_YCENTER;

		ChangeActorStat(act, STAT_MISC);
		break;

	case DTILE_PLAYERONWATER:
		if (actj)
		{
			act->spr.scale = actj->spr.scale;
			act->vel.Z = 0.5;
			if (act->sector()->lotag != 2)
				act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		}
		ChangeActorStat(act, STAT_DUMMYPLAYER);
		break;

	case DTILE_APLAYER:
	{
		act->spr.scale = DVector2(0, 0);
		int j = ud.coop;
		if (j == 2) j = 0;

		if (ud.multimode < 2 || (ud.multimode > 1 && j != act->spr.lotag))
			ChangeActorStat(act, STAT_MISC);
		else
			ChangeActorStat(act, STAT_PLAYER);
		break;
	}
	case DTILE_WATERDRIPSPLASH: // ok
		act->spr.scale = DVector2(0.375, 0.375);
		ChangeActorStat(act, STAT_STANDABLE);
		break;

	case DTILE_WATERBUBBLEMAKER:
		if (act->spr.hitag && act->spr.picnum == DTILE_WATERBUBBLEMAKER)
		{	// JBF 20030913: Pisses off move(), eg. in bobsp2
			Printf(TEXTCOLOR_YELLOW "WARNING: DTILE_WATERBUBBLEMAKER %d @ %d,%d with hitag!=0. Applying fixup.\n", act->GetIndex(), int(act->spr.pos.X), int(act->spr.pos.Y));
			act->spr.hitag = 0;
		}
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		ChangeActorStat(act, STAT_STANDABLE);
		break;
	case DTILE_OCTABRAINSTAYPUT:
	case DTILE_COMMANDERSTAYPUT:
		act->actorstayput = act->sector();
		[[fallthrough]];
	case DTILE_ROTATEGUN:
	case DTILE_DRONE:
	case DTILE_OCTABRAIN:
	case DTILE_COMMANDER:
	case DTILE_ORGANTIC:
	case DTILE_SHARK:

		if (act->spr.picnum != DTILE_SHARK)
		{
			act->spr.scale = DVector2(0.625, 0.625);
			act->clipdist = 20;
		}
		else
		{
			act->spr.scale = DVector2(0.9375, 0.9375);
			act->clipdist = 10;
		}

		if (actj) act->spr.lotag = 0;

		if ((act->spr.lotag > ud.player_skill) || ud.monsters_off == 1)
		{
			act->spr.scale = DVector2(0, 0);
			ChangeActorStat(act, STAT_MISC);
			break;
		}
		else
		{
			makeitfall(act);

			act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;

			if (act->spr.picnum == DTILE_ORGANTIC) act->spr.cstat |= CSTAT_SPRITE_YCENTER;

			if (actj)
			{
				act->timetosleep = 0;
				check_fta_sounds_d(act);
				ChangeActorStat(act, STAT_ACTOR);
			}
			else ChangeActorStat(act, STAT_ZOMBIEACTOR);
		}

		if (act->spr.picnum == DTILE_ROTATEGUN)
			act->vel.Z = 0;

		break;


	case DTILE_FLOORFLAME:
		act->spr.shade = -127;
		ChangeActorStat(act, STAT_STANDABLE);
		break;

	case DTILE_CEILINGSTEAM:
		ChangeActorStat(act, STAT_STANDABLE);
		break;

	case DTILE_RUBBERCAN:
		act->spr.extra = 0;
		[[fallthrough]];
	case DTILE_EXPLODINGBARREL:
	case DTILE_HORSEONSIDE:
	case DTILE_FIREBARREL:
	case DTILE_NUKEBARREL:
	case DTILE_FIREVASE:
	case DTILE_NUKEBARRELDENTED:
	case DTILE_NUKEBARRELLEAKED:
	case DTILE_WOODENHORSE:

		if (actj)
			act->spr.scale = DVector2(0.5, 0.5);
		act->clipdist = 18;
		makeitfall(act);
		if (actj) act->SetOwner(actj);
		else act->SetOwner(act);
		[[fallthrough]];

	case DTILE_EGG:
		if (ud.monsters_off == 1 && act->spr.picnum == DTILE_EGG)
		{
			act->spr.scale = DVector2(0, 0);
			ChangeActorStat(act, STAT_MISC);
		}
		else
		{
			if (act->spr.picnum == DTILE_EGG)
			{
				act->clipdist = 6;
			}
			act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL | randomXFlip();
			ChangeActorStat(act, STAT_ZOMBIEACTOR);
		}
		break;
	}
	return act;
}

END_DUKE_NS
