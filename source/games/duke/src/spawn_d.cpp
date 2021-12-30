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
	auto sectp = act->sector();

	if (isWorldTour())
	{
		switch (act->spr.picnum)
		{
		case BOSS2STAYPUT:
		case BOSS3STAYPUT:
		case BOSS5STAYPUT:
			act->actorstayput = act->sector();
			[[fallthrough]];
		case FIREFLY:
		case BOSS5:
			if (act->spr.picnum != FIREFLY)
			{
				if (actj && actj->spr.picnum == RESPAWN)
					act->spr.pal = actj->spr.pal;
				if (act->spr.pal != 0)
				{
					act->spr.clipdist = 80;
					act->spr.xrepeat = 40;
					act->spr.yrepeat = 40;
				}
				else
				{
					act->spr.xrepeat = 80;
					act->spr.yrepeat = 80;
					act->spr.clipdist = 164;
				}
			}
			else
			{
				act->spr.xrepeat = 40;
				act->spr.yrepeat = 40;
				act->spr.clipdist = 80;
			}

			if (actj)
				act->spr.lotag = 0;

			if ((act->spr.lotag > ud.player_skill) || ud.monsters_off)
			{
				act->spr.xrepeat = act->spr.yrepeat = 0;
				ChangeActorStat(act, STAT_MISC);
				break;
			}
			else
			{
				makeitfall(act);

				act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
				ps[connecthead].max_actors_killed++;

				if (actj) {
					act->timetosleep = 0;
					check_fta_sounds_d(act);
					ChangeActorStat(act, 1);
				}
				else
					ChangeActorStat(act, 2);
			}
			return act;
		case FIREFLYFLYINGEFFECT:
			act->SetOwner(actj);
			ChangeActorStat(act, STAT_MISC);
			act->spr.xrepeat = 16;
			act->spr.yrepeat = 16;
			return act;
		case LAVAPOOLBUBBLE:
			if (actj->spr.xrepeat < 30)
				return act;
			act->SetOwner(actj);
			ChangeActorStat(act, STAT_MISC);
			act->spr.pos.X += krand() % 512 - 256;
			act->spr.pos.Y += krand() % 512 - 256;
			act->spr.xrepeat = 16;
			act->spr.yrepeat = 16;
			return act;
		case WHISPYSMOKE:
			ChangeActorStat(act, STAT_MISC);
			act->spr.pos.X += krand() % 256 - 128;
			act->spr.pos.Y += krand() % 256 - 128;
			act->spr.xrepeat = 20;
			act->spr.yrepeat = 20;
			return act;
		case SERIOUSSAM:
			ChangeActorStat(act, 2);
			act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
			act->spr.extra = 150;
			return act;
		}
	}

	switch (act->spr.picnum)
	{
	default:
		spawninitdefault(actj, act);
		break;
	case FOF:
		act->spr.xrepeat = act->spr.yrepeat = 0;
		ChangeActorStat(act, STAT_MISC);
		break;
	case WATERSPLASH2:
		if (actj)
		{
			SetActor(act, actj->spr.pos);
			act->spr.xrepeat = act->spr.yrepeat = 8 + (krand() & 7);
		}
		else act->spr.xrepeat = act->spr.yrepeat = 16 + (krand() & 15);

		act->spr.shade = -16;
		act->spr.cstat |= CSTAT_SPRITE_YCENTER;
		if (actj)
		{
			if (actj->sector()->lotag == 2)
			{
				act->spr.pos.Z = getceilzofslopeptr(act->sector(), act->spr.pos.X, act->spr.pos.Y) + (16 << 8);
				act->spr.cstat |= CSTAT_SPRITE_YFLIP;
			}
			else if (actj->sector()->lotag == 1)
				act->spr.pos.Z = getflorzofslopeptr(act->sector(), act->spr.pos.X, act->spr.pos.Y);
		}

		if (sectp->floorpicnum == FLOORSLIME ||
			sectp->ceilingpicnum == FLOORSLIME)
			act->spr.pal = 7;
		[[fallthrough]];
	case NEON1:
	case NEON2:
	case NEON3:
	case NEON4:
	case NEON5:
	case NEON6:
	case DOMELITE:
		if (act->spr.picnum != WATERSPLASH2)
			act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		[[fallthrough]];
	case NUKEBUTTON:
		if (act->spr.picnum == DOMELITE)
			act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		[[fallthrough]];
	case JIBS1:
	case JIBS2:
	case JIBS3:
	case JIBS4:
	case JIBS5:
	case JIBS6:
	case HEADJIB1:
	case ARMJIB1:
	case LEGJIB1:
	case LIZMANHEAD1:
	case LIZMANARM1:
	case LIZMANLEG1:
	case DUKETORSO:
	case DUKEGUN:
	case DUKELEG:
		ChangeActorStat(act, STAT_MISC);
		break;
	case TONGUE:
		if (actj)
			act->spr.ang = actj->spr.ang;
		act->spr.pos.Z -= PHEIGHT_DUKE;
		act->spr.zvel = 256 - (krand() & 511);
		act->spr.xvel = 64 - (krand() & 127);
		ChangeActorStat(act, 4);
		break;
	case NATURALLIGHTNING:
		act->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		break;
	case TRANSPORTERSTAR:
	case TRANSPORTERBEAM:
		spawntransporter(actj, act, act->spr.picnum == TRANSPORTERBEAM);
		break;

	case FRAMEEFFECT1:
		if (actj)
		{
			act->spr.xrepeat = actj->spr.xrepeat;
			act->spr.yrepeat = actj->spr.yrepeat;
			act->temp_data[1] = actj->spr.picnum;
		}
		else act->spr.xrepeat = act->spr.yrepeat = 0;

		ChangeActorStat(act, STAT_MISC);

		break;

	case LASERLINE:
		act->spr.yrepeat = 6;
		act->spr.xrepeat = 32;

		if (gs.lasermode == 1)
			act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_TRANSLUCENT;
		else if (gs.lasermode == 0 || gs.lasermode == 2)
			act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_WALL;
		else
		{
			act->spr.xrepeat = 0;
			act->spr.yrepeat = 0;
		}

		if (actj) act->spr.ang = actj->temp_data[5] + 512;
		ChangeActorStat(act, STAT_MISC);
		break;

	case FORCESPHERE:
		if (!actj)
		{
			act->spr.cstat = CSTAT_SPRITE_INVISIBLE;
			ChangeActorStat(act, 2);
		}
		else
		{
			act->spr.xrepeat = act->spr.yrepeat = 1;
			ChangeActorStat(act, STAT_MISC);
		}
		break;

	case BLOOD:
		act->spr.xrepeat = act->spr.yrepeat = 16;
		act->spr.pos.Z -= (26 << 8);
		if (actj && actj->spr.pal == 6)
			act->spr.pal = 6;
		ChangeActorStat(act, STAT_MISC);
		break;
	case LAVAPOOL:
		if (!isWorldTour()) // Twentieth Anniversary World Tour
			return act;
		[[fallthrough]];

	case BLOODPOOL:
	case PUKE:
		if (spawnbloodpoolpart1(act)) break;

		if (actj && act->spr.picnum != PUKE)
		{
			if (actj->spr.pal == 1)
				act->spr.pal = 1;
			else if (actj->spr.pal != 6 && actj->spr.picnum != NUKEBARREL && actj->spr.picnum != TIRE)
			{
				if (actj->spr.picnum == FECES)
					act->spr.pal = 7; // Brown
				else act->spr.pal = 2; // Red
			}
			else act->spr.pal = 0;  // green

			if (actj->spr.picnum == TIRE)
				act->spr.shade = 127;
		}
		act->spr.cstat |= CSTAT_SPRITE_ALIGNMENT_FLOOR;
		if (act->spr.picnum == LAVAPOOL)  // Twentieth Anniversary World Tour
		{
			int fz = getflorzofslopeptr(act->sector(), act->spr.pos.X, act->spr.pos.Y);
			if (fz != act->spr.pos.Z)
				act->spr.pos.Z = fz;
			act->spr.pos.Z -= 200;
		}
		[[fallthrough]];

	case FECES:
		if (actj)
			act->spr.xrepeat = act->spr.yrepeat = 1;
		ChangeActorStat(act, STAT_MISC);
		break;

	case BLOODSPLAT1:
	case BLOODSPLAT2:
	case BLOODSPLAT3:
	case BLOODSPLAT4:
		act->spr.cstat |= CSTAT_SPRITE_ALIGNMENT_WALL;
		act->spr.xrepeat = 7 + (krand() & 7);
		act->spr.yrepeat = 7 + (krand() & 7);
		act->spr.pos.Z -= (16 << 8);
		if (actj && actj->spr.pal == 6)
			act->spr.pal = 6;
		insertspriteq(act);
		ChangeActorStat(act, STAT_MISC);
		break;

	case TRIPBOMB:
		if (act->spr.lotag > ud.player_skill)
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
			break;
		}

		act->spr.xrepeat = 4;
		act->spr.yrepeat = 5;

		act->SetOwner(act);
		ud.bomb_tag = (ud.bomb_tag + 1) & 32767;
		act->spr.hitag = ud.bomb_tag;

		act->spr.xvel = 16;
		ssp(act, CLIPMASK0);
		act->temp_data[0] = 17;
		act->temp_data[2] = 0;
		act->temp_data[5] = act->spr.ang;
		[[fallthrough]];

	case SPACEMARINE:
		if (act->spr.picnum == SPACEMARINE)
		{
			act->spr.extra = 20;
			act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		}
		ChangeActorStat(act, 2);
		break;

	case HYDRENT:
	case PANNEL1:
	case PANNEL2:
	case SATELITE:
	case FUELPOD:
	case SOLARPANNEL:
	case ANTENNA:
	case GRATE1:
	case CHAIR1:
	case CHAIR2:
	case CHAIR3:
	case BOTTLE1:
	case BOTTLE2:
	case BOTTLE3:
	case BOTTLE4:
	case BOTTLE5:
	case BOTTLE6:
	case BOTTLE7:
	case BOTTLE8:
	case BOTTLE10:
	case BOTTLE11:
	case BOTTLE12:
	case BOTTLE13:
	case BOTTLE14:
	case BOTTLE15:
	case BOTTLE16:
	case BOTTLE17:
	case BOTTLE18:
	case BOTTLE19:
	case OCEANSPRITE1:
	case OCEANSPRITE2:
	case OCEANSPRITE3:
	case OCEANSPRITE5:
	case MONK:
	case INDY:
	case LUKE:
	case JURYGUY:
	case SCALE:
	case VACUUM:
	case FANSPRITE:
	case CACTUS:
	case CACTUSBROKE:
	case HANGLIGHT:
	case FETUS:
	case FETUSBROKE:
	case CAMERALIGHT:
	case MOVIECAMERA:
	case IVUNIT:
	case POT1:
	case POT2:
	case POT3:
	case TRIPODCAMERA:
	case SUSHIPLATE1:
	case SUSHIPLATE2:
	case SUSHIPLATE3:
	case SUSHIPLATE4:
	case SUSHIPLATE5:
	case WAITTOBESEATED:
	case VASE:
	case PIPE1:
	case PIPE2:
	case PIPE3:
	case PIPE4:
	case PIPE5:
	case PIPE6:
		act->spr.clipdist = 32;
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		[[fallthrough]];
	case OCEANSPRITE4:
		ChangeActorStat(act, 0);
		break;
	case FEMMAG1:
	case FEMMAG2:
		act->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		ChangeActorStat(act, 0);
		break;
	case DUKETAG:
	case SIGN1:
	case SIGN2:
		if (ud.multimode < 2 && act->spr.pal)
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
		}
		else act->spr.pal = 0;
		break;
	case MASKWALL1:
	case MASKWALL2:
	case MASKWALL3:
	case MASKWALL4:
	case MASKWALL5:
	case MASKWALL6:
	case MASKWALL7:
	case MASKWALL8:
	case MASKWALL9:
	case MASKWALL10:
	case MASKWALL11:
	case MASKWALL12:
	case MASKWALL13:
	case MASKWALL14:
	case MASKWALL15:
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

	case FEM1:
	case FEM2:
	case FEM3:
	case FEM4:
	case FEM5:
	case FEM6:
	case FEM7:
	case FEM8:
	case FEM9:
	case FEM10:
	case PODFEM1:
	case NAKED1:
	case STATUE:
	case TOUGHGAL:
		act->spr.yvel = act->spr.hitag;
		act->spr.hitag = -1;
		if (act->spr.picnum == PODFEM1) act->spr.extra <<= 1;
		[[fallthrough]];

	case BLOODYPOLE:

	case QUEBALL:
	case STRIPEBALL:

		if (act->spr.picnum == QUEBALL || act->spr.picnum == STRIPEBALL)
		{
			act->spr.cstat = CSTAT_SPRITE_BLOCK_HITSCAN;
			act->spr.clipdist = 8;
		}
		else
		{
			act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
			act->spr.clipdist = 32;
		}

		ChangeActorStat(act, 2);
		break;

	case DUKELYINGDEAD:
		if (actj && actj->spr.picnum == APLAYER)
		{
			act->spr.xrepeat = actj->spr.xrepeat;
			act->spr.yrepeat = actj->spr.yrepeat;
			act->spr.shade = actj->spr.shade;
			act->spr.pal = ps[actj->spr.yvel].palookup;
		}
		[[fallthrough]];
	case DUKECAR:
	case HELECOPT:
		//                if(act->spr.picnum == HELECOPT || act->spr.picnum == DUKECAR) act->spr.xvel = 1024;
		act->spr.cstat = 0;
		act->spr.extra = 1;
		act->spr.xvel = 292;
		act->spr.zvel = 360;
		[[fallthrough]];
	case RESPAWNMARKERRED:
	case BLIMP:

		if (act->spr.picnum == RESPAWNMARKERRED)
		{
			act->spr.xrepeat = act->spr.yrepeat = 24;
			if (actj) act->spr.pos.Z = actj->floorz; // -(1<<4);
		}
		else
		{
			act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
			act->spr.clipdist = 128;
		}
		[[fallthrough]];
	case MIKE:
		if (act->spr.picnum == MIKE)
			act->spr.yvel = act->spr.hitag;
		[[fallthrough]];
	case WEATHERWARN:
		ChangeActorStat(act, 1);
		break;

	case SPOTLITE:
		act->temp_data[0] = act->spr.pos.X;
		act->temp_data[1] = act->spr.pos.Y;
		break;
	case BULLETHOLE:
		act->spr.xrepeat = act->spr.yrepeat = 3;
		act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_WALL | randomFlip();
		insertspriteq(act);
		[[fallthrough]];
	case MONEY:
	case MAIL:
	case PAPER:
		if (act->spr.picnum == MONEY || act->spr.picnum == MAIL || act->spr.picnum == PAPER)
		{
			act->temp_data[0] = krand() & 2047;
			act->spr.cstat = randomFlip();
			act->spr.xrepeat = act->spr.yrepeat = 8;
			act->spr.ang = krand() & 2047;
		}
		ChangeActorStat(act, STAT_MISC);
		break;

	case VIEWSCREEN:
	case VIEWSCREEN2:
		act->SetOwner(act);
		act->spr.lotag = 1;
		act->spr.extra = 1;
		ChangeActorStat(act, 6);
		break;

	case SHELL: //From the player
	case SHOTGUNSHELL:
		initshell(actj, act, act->spr.picnum == SHELL);
		break;

	case RESPAWN:
		act->spr.extra = 66 - 13;
		[[fallthrough]];
	case MUSICANDSFX:
		if (ud.multimode < 2 && act->spr.pal == 1)
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
			break;
		}
		act->spr.cstat = CSTAT_SPRITE_INVISIBLE;
		ChangeActorStat(act, 11);
		break;

	case ONFIRE:
		// Twentieth Anniversary World Tour
		if (!isWorldTour())
			break;
		[[fallthrough]];
	case EXPLOSION2:
	case EXPLOSION2BOT:
	case BURNING:
	case BURNING2:
	case SMALLSMOKE:
	case SHRINKEREXPLOSION:
	case COOLEXPLOSION1:

		if (actj)
		{
			act->spr.ang = actj->spr.ang;
			act->spr.shade = -64;
			act->spr.cstat = CSTAT_SPRITE_YCENTER | randomXFlip();
		}

		if (act->spr.picnum == EXPLOSION2 || act->spr.picnum == EXPLOSION2BOT)
		{
			act->spr.xrepeat = 48;
			act->spr.yrepeat = 48;
			act->spr.shade = -127;
			act->spr.cstat |= CSTAT_SPRITE_YCENTER;
		}
		else if (act->spr.picnum == SHRINKEREXPLOSION)
		{
			act->spr.xrepeat = 32;
			act->spr.yrepeat = 32;
		}
		else if (act->spr.picnum == SMALLSMOKE || act->spr.picnum == ONFIRE)
		{
			// 64 "money"
			act->spr.xrepeat = 24;
			act->spr.yrepeat = 24;
		}
		else if (act->spr.picnum == BURNING || act->spr.picnum == BURNING2)
		{
			act->spr.xrepeat = 4;
			act->spr.yrepeat = 4;
		}

		if (actj)
		{
			int x = getflorzofslopeptr(act->sector(), act->spr.pos.X, act->spr.pos.Y);
			if (act->spr.pos.Z > x - (12 << 8))
				act->spr.pos.Z = x - (12 << 8);
		}

		if (act->spr.picnum == ONFIRE)
		{
			act->spr.pos.X += krand() % 256 - 128;
			act->spr.pos.Y += krand() % 256 - 128;
			act->spr.pos.Z -= krand() % 10240;
			act->spr.cstat |= CSTAT_SPRITE_YCENTER;
		}

		ChangeActorStat(act, STAT_MISC);

		break;

	case PLAYERONWATER:
		if (actj)
		{
			act->spr.xrepeat = actj->spr.xrepeat;
			act->spr.yrepeat = actj->spr.yrepeat;
			act->spr.zvel = 128;
			if (act->sector()->lotag != 2)
				act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		}
		ChangeActorStat(act, STAT_DUMMYPLAYER);
		break;

	case APLAYER:
	{
		act->spr.xrepeat = act->spr.yrepeat = 0;
		int j = ud.coop;
		if (j == 2) j = 0;

		if (ud.multimode < 2 || (ud.multimode > 1 && j != act->spr.lotag))
			ChangeActorStat(act, STAT_MISC);
		else
			ChangeActorStat(act, STAT_PLAYER);
		break;
	}
	case WATERBUBBLE:
		if (actj && actj->spr.picnum == APLAYER)
			act->spr.pos.Z -= (16 << 8);
		if (act->spr.picnum == WATERBUBBLE)
		{
			if (actj)
				act->spr.ang = actj->spr.ang;
			act->spr.xrepeat = act->spr.yrepeat = 4;
		}
		else act->spr.xrepeat = act->spr.yrepeat = 32;

		ChangeActorStat(act, STAT_MISC);
		break;

	case CRANE:
		initcrane(actj, act, CRANEPOLE);
		break;

	case WATERDRIP:
		initwaterdrip(actj, act);
		break;
	case TRASH:

		if (act->spr.picnum != WATERDRIP)
			act->spr.ang = krand() & 2047;
		[[fallthrough]];

	case WATERDRIPSPLASH:

		act->spr.xrepeat = 24;
		act->spr.yrepeat = 24;


		ChangeActorStat(act, 6);
		break;

	case PLUG:
		act->spr.lotag = 9999;
		ChangeActorStat(act, 6);
		break;
	case TOUCHPLATE:
		act->temp_data[2] = sectp->floorz;
		if (sectp->lotag != 1 && sectp->lotag != 2)
			sectp->setfloorz(act->spr.pos.Z);
		if (!isWorldTour())
		{
			if (act->spr.pal && ud.multimode > 1)
			{
				act->spr.xrepeat = act->spr.yrepeat = 0;
				ChangeActorStat(act, STAT_MISC);
				break;
			}
		}
		else { // Twentieth Anniversary World Tour addition
			if ((act->spr.pal == 1 && ud.multimode > 1) // Single-game Only
				|| (act->spr.pal == 2 && (ud.multimode == 1 || (ud.multimode > 1 && ud.coop != 1))) // Co-op Only
				|| (act->spr.pal == 3 && (ud.multimode == 1 || (ud.multimode > 1 && ud.coop == 1)))) // Dukematch Only
			{
				act->spr.xrepeat = act->spr.yrepeat = 0;
				ChangeActorStat(act, STAT_MISC);
				break;
			}
		}
		[[fallthrough]];
	case WATERBUBBLEMAKER:
		if (act->spr.hitag && act->spr.picnum == WATERBUBBLEMAKER)
		{	// JBF 20030913: Pisses off move(), eg. in bobsp2
			Printf(TEXTCOLOR_YELLOW "WARNING: WATERBUBBLEMAKER %d @ %d,%d with hitag!=0. Applying fixup.\n", act->GetIndex(), act->spr.pos.X, act->spr.pos.Y);
			act->spr.hitag = 0;
		}
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		ChangeActorStat(act, 6);
		break;
	case BOLT1:
	case BOLT1 + 1:
	case BOLT1 + 2:
	case BOLT1 + 3:
	case SIDEBOLT1:
	case SIDEBOLT1 + 1:
	case SIDEBOLT1 + 2:
	case SIDEBOLT1 + 3:
		act->temp_data[0] = act->spr.xrepeat;
		act->temp_data[1] = act->spr.yrepeat;
		[[fallthrough]];
	case MASTERSWITCH:
		if (act->spr.picnum == MASTERSWITCH)
			act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		act->spr.yvel = 0;
		ChangeActorStat(act, 6);
		break;
	case TARGET:
	case DUCK:
	case LETTER:
		act->spr.extra = 1;
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		ChangeActorStat(act, 1);
		break;
	case OCTABRAINSTAYPUT:
	case LIZTROOPSTAYPUT:
	case PIGCOPSTAYPUT:
	case LIZMANSTAYPUT:
	case BOSS1STAYPUT:
	case PIGCOPDIVE:
	case COMMANDERSTAYPUT:
	case BOSS4STAYPUT:
		act->actorstayput = act->sector();
		[[fallthrough]];
	case BOSS1:
	case BOSS2:
	case BOSS3:
	case BOSS4:
	case ROTATEGUN:
	case GREENSLIME:
		if (act->spr.picnum == GREENSLIME)
			act->spr.extra = 1;
		[[fallthrough]];
	case DRONE:
	case LIZTROOPONTOILET:
	case LIZTROOPJUSTSIT:
	case LIZTROOPSHOOT:
	case LIZTROOPJETPACK:
	case LIZTROOPDUCKING:
	case LIZTROOPRUNNING:
	case LIZTROOP:
	case OCTABRAIN:
	case COMMANDER:
	case PIGCOP:
	case LIZMAN:
	case LIZMANSPITTING:
	case LIZMANFEEDING:
	case LIZMANJUMP:
	case ORGANTIC:
	case RAT:
	case SHARK:

		if (act->spr.pal == 0)
		{
			switch (act->spr.picnum)
			{
			case LIZTROOPONTOILET:
			case LIZTROOPSHOOT:
			case LIZTROOPJETPACK:
			case LIZTROOPDUCKING:
			case LIZTROOPRUNNING:
			case LIZTROOPSTAYPUT:
			case LIZTROOPJUSTSIT:
			case LIZTROOP:
				act->spr.pal = 22;
				break;
			}
		}

		if (bossguy(act))
		{
			if (actj && actj->spr.picnum == RESPAWN)
				act->spr.pal = actj->spr.pal;
			if (act->spr.pal && (!isWorldTour() || !(currentLevel->flags & LEVEL_WT_BOSSSPAWN) || act->spr.pal != 22))
			{
				act->spr.clipdist = 80;
				act->spr.xrepeat = 40;
				act->spr.yrepeat = 40;
			}
			else
			{
				act->spr.xrepeat = 80;
				act->spr.yrepeat = 80;
				act->spr.clipdist = 164;
			}
		}
		else
		{
			if (act->spr.picnum != SHARK)
			{
				act->spr.xrepeat = 40;
				act->spr.yrepeat = 40;
				act->spr.clipdist = 80;
			}
			else
			{
				act->spr.xrepeat = 60;
				act->spr.yrepeat = 60;
				act->spr.clipdist = 40;
			}
		}

		if (actj) act->spr.lotag = 0;

		if ((act->spr.lotag > ud.player_skill) || ud.monsters_off == 1)
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
			break;
		}
		else
		{
			makeitfall(act);

			if (act->spr.picnum == RAT)
			{
				act->spr.ang = krand() & 2047;
				act->spr.xrepeat = act->spr.yrepeat = 48;
				act->spr.cstat = 0;
			}
			else
			{
				act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;

				if (act->spr.picnum != SHARK)
					ps[myconnectindex].max_actors_killed++;
			}

			if (act->spr.picnum == ORGANTIC) act->spr.cstat |= CSTAT_SPRITE_YCENTER;

			if (actj)
			{
				act->timetosleep = 0;
				check_fta_sounds_d(act);
				ChangeActorStat(act, 1);
			}
			else ChangeActorStat(act, 2);
		}

		if (act->spr.picnum == ROTATEGUN)
			act->spr.zvel = 0;

		break;

	case LOCATORS:
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		ChangeActorStat(act, 7);
		break;

	case ACTIVATORLOCKED:
	case ACTIVATOR:
		act->spr.cstat = CSTAT_SPRITE_INVISIBLE;
		if (act->spr.picnum == ACTIVATORLOCKED)
			act->sector()->lotag |= 16384;
		ChangeActorStat(act, 8);
		break;

	case DOORSHOCK:
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		act->spr.shade = -12;
		ChangeActorStat(act, 6);
		break;

	case OOZ:
	case OOZ2:
	{
		act->spr.shade = -12;

		if (actj)
		{
			if (actj->spr.picnum == NUKEBARREL)
				act->spr.pal = 8;
			insertspriteq(act);
		}

		ChangeActorStat(act, 1);

		getglobalz(act);

		int j = (act->floorz - act->ceilingz) >> 9;

		act->spr.yrepeat = j;
		act->spr.xrepeat = 25 - (j >> 1);
		if (krand() & 4) act->spr.cstat |= CSTAT_SPRITE_XFLIP;

		break;
	}
	case HEAVYHBOMB:
		if (actj) act->SetOwner(actj);
		else act->SetOwner(act);

		act->spr.xrepeat = act->spr.yrepeat = 9;
		act->spr.yvel = 4;
		[[fallthrough]];
	case REACTOR2:
	case REACTOR:
	case RECON:
		if (initreactor(actj, act, act->spr.picnum == RECON)) return act;
		break;

	case FLAMETHROWERSPRITE:
	case FLAMETHROWERAMMO: // Twentieth Anniversary World Tour
		if (!isWorldTour())
			break;
		[[fallthrough]];

	case ATOMICHEALTH:
	case STEROIDS:
	case HEATSENSOR:
	case SHIELD:
	case AIRTANK:
	case TRIPBOMBSPRITE:
	case JETPACK:
	case HOLODUKE:

	case FIRSTGUNSPRITE:
	case CHAINGUNSPRITE:
	case SHOTGUNSPRITE:
	case RPGSPRITE:
	case SHRINKERSPRITE:
	case FREEZESPRITE:
	case DEVISTATORSPRITE:

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
	case COLA:
	case FIRSTAID:
	case SIXPAK:
		if (actj)
		{
			act->spr.lotag = 0;
			act->spr.pos.Z -= (32 << 8);
			act->spr.zvel = -1024;
			ssp(act, CLIPMASK0);
			if (krand() & 4) act->spr.cstat |= CSTAT_SPRITE_XFLIP;
		}
		else
		{
			act->SetOwner(act);
			act->spr.cstat = 0;
		}

		if ((ud.multimode < 2 && act->spr.pal != 0) || (act->spr.lotag > ud.player_skill))
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
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
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
			break;
		}
		else
		{
			if (act->spr.picnum == AMMO)
				act->spr.xrepeat = act->spr.yrepeat = 16;
			else act->spr.xrepeat = act->spr.yrepeat = 32;
		}

		act->spr.shade = -17;

		if (actj) ChangeActorStat(act, STAT_ACTOR);
		else
		{
			ChangeActorStat(act, STAT_ZOMBIEACTOR);
			makeitfall(act);
		}
		break;

	case WATERFOUNTAIN:
		act->spr.lotag = 1;
		[[fallthrough]];

	case TREE1:
	case TREE2:
	case TIRE:
	case CONE:
	case BOX:
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL; // Make it hitable
		act->spr.extra = 1;
		ChangeActorStat(act, STAT_STANDABLE);
		break;

	case FLOORFLAME:
		act->spr.shade = -127;
		ChangeActorStat(act, STAT_STANDABLE);
		break;

	case BOUNCEMINE:
		act->SetOwner(act);
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL; //Make it hitable
		act->spr.xrepeat = act->spr.yrepeat = 24;
		act->spr.shade = -127;
		act->spr.extra = gs.impact_damage << 2;
		ChangeActorStat(act, STAT_ZOMBIEACTOR);
		break;

	case CAMERA1:
	case CAMERA1 + 1:
	case CAMERA1 + 2:
	case CAMERA1 + 3:
	case CAMERA1 + 4:
	case CAMERAPOLE:
		act->spr.extra = 1;

		if (gs.camerashitable) act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		else act->spr.cstat = 0;
		[[fallthrough]];

	case GENERICPOLE:

		if (ud.multimode < 2 && act->spr.pal != 0)
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
			break;
		}
		else act->spr.pal = 0;
		if (act->spr.picnum == CAMERAPOLE || act->spr.picnum == GENERICPOLE) break;
		act->spr.picnum = CAMERA1;
		ChangeActorStat(act, 1);
		break;
	case STEAM:
		if (actj)
		{
			act->spr.ang = actj->spr.ang;
			act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
			act->spr.xrepeat = act->spr.yrepeat = 1;
			act->spr.xvel = -8;
			ssp(act, CLIPMASK0);
		}
		[[fallthrough]];
	case CEILINGSTEAM:
		ChangeActorStat(act, STAT_STANDABLE);
		break;

	case SECTOREFFECTOR:
		spawneffector(act, actors);

		break;


	case SEENINE:
	case OOZFILTER:

		act->spr.shade = -16;
		if (act->spr.xrepeat <= 8)
		{
			act->spr.cstat = CSTAT_SPRITE_INVISIBLE;
			act->spr.xrepeat = act->spr.yrepeat = 0;
		}
		else act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->spr.extra = gs.impact_damage << 2;
		act->SetOwner(act);
		ChangeActorStat(act, STAT_STANDABLE);
		break;

	case CRACK1:
	case CRACK2:
	case CRACK3:
	case CRACK4:
	case FIREEXT:
		if (act->spr.picnum == FIREEXT)
		{
			act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
			act->spr.extra = gs.impact_damage << 2;
		}
		else
		{
			act->spr.cstat |= (act->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) ? CSTAT_SPRITE_BLOCK : (CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_ALIGNMENT_WALL);
			act->spr.extra = 1;
		}

		if (ud.multimode < 2 && act->spr.pal != 0)
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
			break;
		}

		act->spr.pal = 0;
		act->SetOwner(act);
		ChangeActorStat(act, STAT_STANDABLE);
		act->spr.xvel = 8;
		ssp(act, CLIPMASK0);
		break;

	case TOILET:
	case STALL:
		act->spr.lotag = 1;
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		act->spr.clipdist = 8;
		act->SetOwner(act);
		break;
	case CANWITHSOMETHING:
	case CANWITHSOMETHING2:
	case CANWITHSOMETHING3:
	case CANWITHSOMETHING4:
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
			act->spr.xrepeat = act->spr.yrepeat = 32;
		act->spr.clipdist = 72;
		makeitfall(act);
		if (actj) act->SetOwner(actj);
		else act->SetOwner(act);
		[[fallthrough]];

	case EGG:
		if (ud.monsters_off == 1 && act->spr.picnum == EGG)
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
		}
		else
		{
			if (act->spr.picnum == EGG)
			{
				act->spr.clipdist = 24;
				ps[connecthead].max_actors_killed++;
			}
			act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL | randomXFlip();
			ChangeActorStat(act, STAT_ZOMBIEACTOR);
		}
		break;
	case TOILETWATER:
		act->spr.shade = -16;
		ChangeActorStat(act, STAT_STANDABLE);
		break;
	}
	return act;
}

END_DUKE_NS
