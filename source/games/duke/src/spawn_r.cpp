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

#include <utility>
#include "ns.h"
#include "global.h"
#include "sounds.h"
#include "names_r.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

DDukeActor* spawninit_r(DDukeActor* actj, DDukeActor* act, TArray<DDukeActor*>* actors)
{
	if (actorflag(act, SFLAG2_TRIGGERRESPAWN))
	{
		act->spr.yint = act->spr.hitag;
		act->spr.hitag = -1;
	}

	if (act->GetClass() != RUNTIME_CLASS(DDukeActor))
	{
		CallInitialize(act);
		return act;
	}
	auto sectp = act->sector();

	switch (act->spr.picnum)
	{
	default:
	default_case:
		spawninitdefault(actj, act);
		break;
	case RRTILE7936:
		if (!isRRRA()) goto default_case;
		act->spr.scale = DVector2(0, 0);
		fogactive = 1;
		break;
	case RRTILE6144:
		if (!isRRRA()) goto default_case;
		act->spr.scale = DVector2(0, 0);
		ps[screenpeek].sea_sick_stat = 1;
		break;
	case MUSICNOTES:
		if (!isRRRA()) goto default_case;
		act->spr.lotag = 1;
		act->clipdist = 0;
		break;
	case JOE9000:
		if (!isRRRA()) goto default_case;
		act->spr.lotag = 1;
		act->clipdist = 0;
		break;
	case RRTILE8193:
		if (!isRRRA()) goto default_case;
		act->spr.scale = DVector2(0, 0);
		pistonsound = 1;
		break;

	case TRANSPORTERSTAR:
	case TRANSPORTERBEAM:
		spawntransporter(actj, act, act->spr.picnum == TRANSPORTERBEAM);
		break;

	case BLOOD:
		act->spr.scale = DVector2(0.0625, 0.0625);
		act->spr.pos.Z -= 26;
		ChangeActorStat(act, STAT_MISC);
		break;
	case GRATE1:
		act->clipdist = 8;
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		ChangeActorStat(act, 0);
		break;
	case FEMMAG1:
	case FEMMAG2:
		act->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		ChangeActorStat(act, 0);
		break;

	case MASKWALL7:
	{
		auto j = act->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_MASK | CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP);
		act->spr.cstat = j | CSTAT_SPRITE_BLOCK;
		ChangeActorStat(act, 0);
		break;
	}
	case FOOTPRINTS:
	case FOOTPRINTS2:
	case FOOTPRINTS3:
	case FOOTPRINTS4:
		initfootprint(actj, act);
		break;
	case FEM10:
	case NAKED1:
	case STATUE:
	case TOUGHGAL:
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		act->clipdist = 8;
		ChangeActorStat(act, STAT_ZOMBIEACTOR);
		break;
	case DUKELYINGDEAD:
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
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		act->clipdist = 32;
		[[fallthrough]];
	case MIKE:
		if (act->spr.picnum == MIKE)
			act->spr.yint = act->spr.hitag;
		ChangeActorStat(act, STAT_ACTOR);
		break;

	case SPOTLITE:
		break;
	case BULLETHOLE:
		act->spr.scale = DVector2(0.046875, 0.046875);
		act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_WALL | randomFlip();
		insertspriteq(act);
		ChangeActorStat(act, STAT_MISC);
		break;

	case EXPLOSION2:
	case EXPLOSION3:
	case BURNING:
	case SMALLSMOKE:
		if (actj)
		{
			act->spr.Angles.Yaw = actj->spr.Angles.Yaw;
			act->spr.shade = -64;
			act->spr.cstat = CSTAT_SPRITE_YCENTER | randomXFlip();
		}

		if (act->spr.picnum == EXPLOSION2)
		{
			act->spr.scale = DVector2(0.75, 0.75);
			act->spr.shade = -127;
			act->spr.cstat |= CSTAT_SPRITE_YCENTER;
		}
		else if (act->spr.picnum == EXPLOSION3)
		{
			act->spr.scale = DVector2(2, 2);
			act->spr.shade = -127;
			act->spr.cstat |= CSTAT_SPRITE_YCENTER;
		}
		else if (act->spr.picnum == SMALLSMOKE)
		{
			act->spr.scale = DVector2(0.1875, 0.1875);
		}
		else if (act->spr.picnum == BURNING)
		{
			act->spr.scale = DVector2(0.0625, 0.0625);
		}

		if (actj)
		{
			double x = getflorzofslopeptr(act->sector(), act->spr.pos);
			if (act->spr.pos.Z > x - 12)
				act->spr.pos.Z = x - 12;
		}

		ChangeActorStat(act, STAT_MISC);

		break;

	case PLAYERONWATER:
		if (actj)
		{
			act->spr.scale = actj->spr.scale;
			act->vel.Z = 0.5;
			if (act->sector()->lotag != 2)
				act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		}
		ChangeActorStat(act, STAT_DUMMYPLAYER);
		break;

	case APLAYER:
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
	case WATERBUBBLE:
		if (actj && actj->isPlayer())
			act->spr.pos.Z -= 16;
		if (act->spr.picnum == WATERBUBBLE)
		{
			if (actj)
				act->spr.Angles.Yaw = actj->spr.Angles.Yaw;
			double s = REPEAT_SCALE + (krand() & 7) * REPEAT_SCALE;
			act->spr.scale = DVector2(s, s);
		}
		else
			act->spr.scale = DVector2(0.5, 0.5);
		ChangeActorStat(act, STAT_MISC);
		break;

	case PLUG:
		act->spr.lotag = 9999;
		ChangeActorStat(act, STAT_STANDABLE);
		break;
	case WATERBUBBLEMAKER:
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		ChangeActorStat(act, STAT_STANDABLE);
		break;

		// this is not really nice...
	case BIKERB:
	case BIKERBV2:
	case BIKER:
	case MAKEOUT:
	case CHEERB:
	case CHEER:
	case COOTPLAY:
	case BILLYPLAY:
	case MINIONBOAT:
	case HULKBOAT:
	case CHEERBOAT:
	case RABBIT:
	case ROCK:
	case ROCK2:
	case MAMACLOUD:
	case MAMA:
		if (isRRRA()) goto rrra_badguy2;
		else goto default_case;

	case SBSWIPE:
	case CHEERSTAYPUT:
		if (isRRRA()) goto rrra_stayput;
		else goto default_case;
	case SBMOVE:
		if (isRRRA()) goto default_case;
		[[fallthrough]];

	case BILLYRAYSTAYPUT:
	case BRAYSNIPER:
	case BUBBASTAND:
	case HULKSTAYPUT:
	case HENSTAYPUT:
	case PIGSTAYPUT:
	case MINIONSTAYPUT:
	case COOTSTAYPUT:
	rrra_stayput:
		act->actorstayput = act->sector();
		[[fallthrough]];
	case BOULDER:
	case BOULDER1:
	case TORNADO:
	case BILLYCOCK:
	case BILLYRAY:
	case DOGRUN:
	case LTH:
	case HULK:
	case HEN:
	case DRONE:
	case PIG:
	case MINION:
	case COW:
	case COOT:
	case VIXEN:
	rrra_badguy2:
		act->spr.scale = DVector2(0.625, 0.625);
		// Note: All inappropriate tiles have already been weeded out by the outer switch block so this does not need game type checks anymore.
		switch (act->spr.picnum)
		{
		case VIXEN:
			if (act->spr.pal == 34)
			{
				act->spr.scale = DVector2(0.34375, 0.328125);
			}
			else
			{
				act->spr.scale = DVector2(0.34375, 0.3125);
			}
			act->setClipDistFromTile();
			break;
		case HULKHANG:
		case HULKHANGDEAD:
		case HULKJUMP:
		case HULK:
		case HULKSTAYPUT:
			act->spr.scale = DVector2(0.5, 0.5);
			act->setClipDistFromTile();
			break;
		case COOTPLAY:
		case COOT:
		case COOTSTAYPUT:
			act->spr.scale = DVector2(0.375, 0.28125);
			act->setClipDistFromTile();
			act->clipdist *= 4;
			break;
		case DRONE:
			act->spr.scale = DVector2(0.21875, 0.109375);
			act->clipdist = 32;
			break;
		case SBSWIPE:
		case BILLYPLAY:
		case BILLYCOCK:
		case BILLYRAY:
		case BILLYRAYSTAYPUT:
		case BRAYSNIPER:
		case BUBBASTAND:
			act->spr.scale = DVector2(0.390625, 0.328125);
			act->setClipDistFromTile();
			break;
		case COW:
			act->spr.scale = DVector2(0.5, 0.5);
			act->setClipDistFromTile();
			break;
		case HEN:
		case HENSTAYPUT:
			if (act->spr.pal == 35)
			{
				act->spr.scale = DVector2(0.65625, 0.46875);
				act->setClipDistFromTile();
			}
			else
			{
				act->spr.scale = DVector2(0.328125, 0.234375);
				act->clipdist = 16;
			}
			break;
		case MINION:
		case MINIONSTAYPUT:
			act->spr.scale = DVector2(0.25, 0.25);
			act->setClipDistFromTile();
			if (isRRRA() && ud.ufospawnsminion)
				act->spr.pal = 8;
			break;
		case DOGRUN:
		case PIG:
			act->spr.scale = DVector2(0.25, 0.25);
			act->setClipDistFromTile();
			break;
		case RABBIT:
			act->spr.scale = DVector2(0.28125, 0.28125);
			act->setClipDistFromTile();
			break;
		case MAMACLOUD:
			act->spr.scale = DVector2(1, 1);
			act->spr.cstat = CSTAT_SPRITE_TRANSLUCENT;
			act->spr.cstat |= CSTAT_SPRITE_TRANS_FLIP;
			act->spr.pos.X += krandf(128) - 64;
			act->spr.pos.Y += krandf(128) - 64;
			act->spr.pos.Z += krandf(8) - 4;
			break;
		case MAMA:
			if (actj && isrespawncontroller(actj))
				act->spr.pal = actj->spr.pal;

			if (act->spr.pal == 30)
			{
				act->spr.scale = DVector2(0.40625, 0.40625);
				act->clipdist = 18.75;
			}
			else if (act->spr.pal == 31)
			{
				act->spr.scale = DVector2(0.5625, 0.5625);
				act->clipdist = 25;
			}
			else if (act->spr.pal == 32)
			{
				act->spr.scale = DVector2(0.78125, 0.78125);
				act->clipdist = 25;
			}
			else
			{
				act->spr.scale = DVector2(0.78125, 0.78125);
				act->clipdist = 25;
			}
			break;
		case BIKERB:
			act->spr.scale = DVector2(0.4375, 0.34375);
			act->clipdist = 18;
			break;
		case BIKERBV2:
			act->spr.scale = DVector2(0.4375, 0.34375);
			act->clipdist = 18;
			break;
		case BIKER:
			act->spr.scale = DVector2(0.4375, 0.34375);
			act->setClipDistFromTile();
			break;
		case CHEERB:
			act->spr.scale = DVector2(0.4375, 0.34375);
			act->clipdist = 18;
			break;
		case CHEER:
		case CHEERSTAYPUT:
			act->spr.scale = DVector2(0.34375, 0.3125);
			act->setClipDistFromTile();
			break;
		case MAKEOUT:
			act->spr.scale = DVector2(0.40625, 0.40625);
			act->setClipDistFromTile();
			break;
		case MINIONBOAT:
			act->spr.scale = DVector2(0.25, 0.25);
			act->setClipDistFromTile();
			break;
		case HULKBOAT:
			act->spr.scale = DVector2(0.75, 0.75);
			act->setClipDistFromTile();
			break;
		case CHEERBOAT:
			act->spr.scale = DVector2(0.5, 0.5);
			act->setClipDistFromTile();
			break;

		case TORNADO:
			act->spr.scale = DVector2(1, 2);
			act->setClipDistFromTile();
			act->clipdist *= 0.25;
			act->spr.cstat = CSTAT_SPRITE_TRANSLUCENT;
			break;
		case LTH:
			act->spr.scale = DVector2(0.375, 0.34375);
			act->setClipDistFromTile();
			break;
		case ROCK:
		case ROCK2:
			act->spr.scale = DVector2(1, 1);
			act->setClipDistFromTile();
			break;

		case SBMOVE:
			act->spr.scale = DVector2(0.75, 0.75);
			act->setClipDistFromTile();
			break;

		default:
			break;
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

			if (act->spr.picnum != 5501)
				if (actorfella(act))
					ps[myconnectindex].max_actors_killed++;

			if (actj)
			{
				act->timetosleep = 0;
				check_fta_sounds_r(act);
				ChangeActorStat(act, STAT_ACTOR);
				act->spr.shade = actj->spr.shade;
			}
			else ChangeActorStat(act, STAT_ZOMBIEACTOR);

		}

		break;

	case RPG2SPRITE:
	case MOTOAMMO:
	case BOATAMMO:
		if (!isRRRA()) goto default_case;
		[[fallthrough]];

	case ATOMICHEALTH:
	case STEROIDS:
	case HEATSENSOR:
	case SHIELD:
	case AIRTANK:
	case POWDERKEG:
	case COWPIE:
	case HOLODUKE:

	case FIRSTGUNSPRITE:
	case RIFLEGUNSPRITE:
	case SHOTGUNSPRITE:
	case CROSSBOWSPRITE:
	case RIPSAWSPRITE:
	case TITSPRITE:
	case ALIENBLASTERSPRITE:

	case SHOTGUNAMMO:
	case FREEZEAMMO:
	case HBOMBAMMO:
	case CRYSTALAMMO:
	case GROWAMMO:
	case BATTERYAMMO:
	case DEVISTATORAMMO:
	case RPGAMMO:
	case BOOTS:
	case AMMO:
	case AMMOLOTS:
	case BEER:
	case FIRSTAID:
	case SIXPAK:

	case SAWAMMO:
	case BOWLINGBALLSPRITE:
		if (actj)
		{
			act->spr.lotag = 0;
			if (act->spr.picnum != BOWLINGBALLSPRITE)
			{
				act->spr.pos.Z -= 32;
				act->vel.Z = -4;
			}
			else
			{
				act->vel.Z = 0;
			}
			ssp(act, CLIPMASK0);
			act->spr.cstat = randomXFlip();
		}
		else
		{
			act->SetOwner(act);
			act->spr.cstat = 0;
		}

		if ((ud.multimode < 2 && act->spr.pal != 0) || (act->spr.lotag > ud.player_skill))
		{
			act->spr.scale = DVector2(0, 0);
			ChangeActorStat(act, STAT_MISC);
			break;
		}

		act->spr.pal = 0;
		[[fallthrough]];

	case ACCESSCARD:

		if (act->spr.picnum == ATOMICHEALTH)
			act->spr.cstat |= CSTAT_SPRITE_YCENTER;

		if (ud.multimode > 1 && ud.coop != 1 && act->spr.picnum == ACCESSCARD)
		{
			act->spr.scale = DVector2(0, 0);
			ChangeActorStat(act, STAT_MISC);
			break;
		}
		else
		{
			if (act->spr.picnum == AMMO)
				act->spr.scale = DVector2(0.25, 0.25);
			else act->spr.scale = DVector2(0.5, 0.5);
		}

		act->spr.shade = -17;

		if (actj) ChangeActorStat(act, STAT_ACTOR);
		else
		{
			ChangeActorStat(act, STAT_ZOMBIEACTOR);
			makeitfall(act);
		}
		switch (act->spr.picnum)
		{
		case FIRSTGUNSPRITE:
			act->spr.scale = DVector2(0.25, 0.25);
			break;
		case SHOTGUNAMMO:
			act->spr.scale = DVector2(0.28125, 0.265625);
			if (isRRRA()) act->spr.cstat = CSTAT_SPRITE_BLOCK_HITSCAN;
			break;
		case SIXPAK:
			act->spr.scale = DVector2(0.203125, 0.140625);
			if (isRRRA()) act->spr.cstat = CSTAT_SPRITE_BLOCK_HITSCAN;
			break;
		case FIRSTAID:
			act->spr.scale = DVector2(0.125, 0.125);
			break;
		case BEER:
			act->spr.scale = DVector2(0.078125, 0.0625);
			break;
		case AMMO:
			act->spr.scale = DVector2(0.140625, 0.140625);
			break;
		case MOTOAMMO:
			if (!isRRRA()) goto default_case;
			act->spr.scale = DVector2(0.359375, 0.359375);
			break;
		case BOATAMMO:
			if (!isRRRA()) goto default_case;
			act->spr.scale = DVector2(0.25, 0.25);
			break;
		case COWPIE:
			act->spr.scale = DVector2(0.125, 0.09375);
			break;
		case STEROIDS:
			act->spr.scale = DVector2(0.203125, 0.140625);
			break;
		case ACCESSCARD:
			act->spr.scale = DVector2(0.171875, 0.1875);
			break;
		case HEATSENSOR:
			act->spr.scale = DVector2(0.09375, 0.0625);
			break;
		case AIRTANK:
			act->spr.scale = DVector2(0.296875, 0.25);
			break;
		case BATTERYAMMO:
			act->spr.scale = DVector2(0.234375, 0.234375);
			break;
		case BOWLINGBALLSPRITE:
			act->spr.scale = DVector2(0.171875, 0.171875);
			break;
		case POWDERKEG:
			act->spr.scale = DVector2(0.171875, 0.171875);
			act->spr.yint = 4;
			act->vel.X = 2;
			break;
		case CROSSBOWSPRITE:
			act->spr.scale = DVector2(0.25, 0.21875);
			break;
		case RPG2SPRITE:
			if (!isRRRA()) goto default_case;
			act->spr.scale = DVector2(0.34375, 0.3125);
			break;
		case RIPSAWSPRITE:
			act->spr.scale = DVector2(0.34375, 0.203125);
			break;
		case ALIENBLASTERSPRITE:
			act->spr.scale = DVector2(0.28125, 0.265625);
			break;
		case SAWAMMO:
			act->spr.scale = DVector2(0.1875, 0.109375);
			break;
		case GROWSPRITEICON:
			act->spr.scale = DVector2(0.15625, 0.140625);
			break;
		case DEVISTATORAMMO:
			act->spr.scale = DVector2(0.15625, 0.140625);
			break;
		case ATOMICHEALTH:
			act->spr.scale = DVector2(0.125, 0.125);
			break;
		case TITSPRITE:
			act->spr.scale = DVector2(0.265625, 0.25);
			break;
		}
		act->spr.shade = act->sector()->floorshade;
		break;
	case CAMERAPOLE:
		act->spr.extra = 1;

		if (gs.camerashitable) act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		else act->spr.cstat = 0;

		if (ud.multimode < 2 && act->spr.pal != 0)
		{
			act->spr.scale = DVector2(0, 0);
			ChangeActorStat(act, STAT_MISC);
			break;
		}
		else act->spr.pal = 0;
		break;
	case STEAM:
		if (actj)
		{
			act->spr.Angles.Yaw = actj->spr.Angles.Yaw;
			act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
			act->spr.scale = DVector2(REPEAT_SCALE, REPEAT_SCALE);
			act->vel.X = -0.5;
			ssp(act, CLIPMASK0);
		}
		[[fallthrough]];
	case CEILINGSTEAM:
		ChangeActorStat(act, STAT_STANDABLE);
		break;
	case SECTOREFFECTOR:
		spawneffector(act, actors);
		break;

	case EMPTYBIKE:
		if (!isRRRA()) goto default_case;
		if (ud.multimode < 2 && act->spr.pal == 1)
		{
			act->spr.scale = DVector2(0, 0);
			break;
		}
		act->spr.pal = 0;
		act->spr.scale = DVector2(0.28125, 0.28125);
		act->setClipDistFromTile();
		act->saved_ammo = 100;
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->spr.lotag = 1;
		ChangeActorStat(act, STAT_ACTOR);
		break;
	case EMPTYBOAT:
		if (!isRRRA()) goto default_case;
		if (ud.multimode < 2 && act->spr.pal == 1)
		{
			act->spr.scale = DVector2(0, 0);
			break;
		}
		act->spr.pal = 0;
		act->spr.scale = DVector2(0.5, 0.5);
		act->setClipDistFromTile();
		act->saved_ammo = 20;
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->spr.lotag = 1;
		ChangeActorStat(act, STAT_ACTOR);
		break;

	case RUBBERCAN:
		act->spr.extra = 0;
		[[fallthrough]];
	case EXPLODINGBARREL:
	case HORSEONSIDE:
	case FIREBARREL:
	case NUKEBARREL:
	case FIREVASE:
	case NUKEBARRELDENTED:
	case NUKEBARRELLEAKED:
	case WOODENHORSE:

		if (actj)
			act->spr.scale = DVector2(0.5, 0.5);
		act->clipdist = 18;
		makeitfall(act);
		if (actj) act->SetOwner(actj);
		else act->SetOwner(act);
		[[fallthrough]];

	case EGG:
		if (ud.monsters_off == 1 && act->spr.picnum == EGG)
		{
			act->spr.scale = DVector2(0, 0);
			ChangeActorStat(act, STAT_MISC);
		}
		else
		{
			if (act->spr.picnum == EGG)
				act->clipdist = 6;
			act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL | randomXFlip();
			ChangeActorStat(act, STAT_ZOMBIEACTOR);
		}
		break;
	case TOILETWATER:
		act->spr.shade = -16;
		ChangeActorStat(act, STAT_STANDABLE);
		break;
	case RRTILE63:
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		act->spr.scale = DVector2(REPEAT_SCALE, REPEAT_SCALE);
		act->clipdist = 0.25;
		ChangeActorStat(act, 100);
		break;
	}
	return act;
}

END_DUKE_NS
