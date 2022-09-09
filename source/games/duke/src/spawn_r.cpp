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
	auto sectp = act->sector();

	switch (act->spr.picnum)
	{
	default:
	default_case:
		spawninitdefault(actj, act);
		break;
	case BOWLINGPINSPOT:
	case RRTILE281:
	case BOWLINGBALLSPOT:
	case RRTILE283:
	case RRTILE2025:
	case RRTILE2026:
	case RRTILE2027:
	case RRTILE2028:
		act->spr.cstat = 0;
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		act->spr.xrepeat = 0;
		act->spr.yrepeat = 0;
		act->set_const_clipdist(0);
		act->spr.extra = 0;
		ChangeActorStat(act, STAT_BOWLING);
		break;
	case RRTILE3410:
		act->spr.extra = 0;
		ChangeActorStat(act, 107);
		break;
	case RRTILE8450:
		if (!isRRRA()) goto default_case;
		act->spr.xrepeat = 64;
		act->spr.yrepeat = 64;
		act->spr.extra = act->spr.lotag;
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		ChangeActorStat(act, 116);
		break;
	case PIG + 11:
		if (!isRRRA()) goto default_case;
		act->spr.xrepeat = 16;
		act->spr.yrepeat = 16;
		act->set_const_clipdist(0);
		act->spr.extra = 0;
		act->spr.cstat = 0;
		ChangeActorStat(act, 121);
		break;
	case RRTILE8487:
	case RRTILE8489:
		if (!isRRRA()) goto default_case;
		act->spr.xrepeat = 32;
		act->spr.yrepeat = 32;
		act->spr.extra = 0;
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		act->spr.hitag = 0;
		ChangeActorStat(act, 117);
		break;
	case RRTILE7424:
		if (!isRRRA()) goto default_case;
		act->spr.extra = 0;
		act->spr.xrepeat = 0;
		act->spr.yrepeat = 0;
		ChangeActorStat(act, 11);
		break;
	case RRTILE7936:
		if (!isRRRA()) goto default_case;
		act->spr.xrepeat = 0;
		act->spr.yrepeat = 0;
		fogactive = 1;
		break;
	case RRTILE6144:
		if (!isRRRA()) goto default_case;
		act->spr.xrepeat = 0;
		act->spr.yrepeat = 0;
		ps[screenpeek].sea_sick_stat = 1;
		break;
	case RRTILE8448:
		if (!isRRRA()) goto default_case;
		act->spr.lotag = 1;
		act->set_const_clipdist(0);
		break;
	case RRTILE8099:
		if (!isRRRA()) goto default_case;
		act->spr.lotag = 5;
		act->set_const_clipdist(0);
		ChangeActorStat(act, 123);
		break;
	case RRTILE8704:
		if (!isRRRA()) goto default_case;
		act->spr.lotag = 1;
		act->set_const_clipdist(0);
		break;
	case RRTILE8192:
		if (!isRRRA()) goto default_case;
		act->spr.xrepeat = 0;
		act->spr.yrepeat = 0;
		ufospawnsminion = 1;
		break;
	case RRTILE8193:
		if (!isRRRA()) goto default_case;
		act->spr.xrepeat = 0;
		act->spr.yrepeat = 0;
		pistonsound = 1;
		break;
	case RRTILE8165:
		if (!isRRRA()) goto default_case;
		act->spr.lotag = 1;
		act->set_const_clipdist(0);
		act->SetOwner(act);
		act->spr.extra = 0;
		ChangeActorStat(act, 115);
		break;
	case RRTILE8593:
		if (!isRRRA()) goto default_case;
		act->spr.lotag = 1;
		act->set_const_clipdist(0);
		act->SetOwner(act);
		act->spr.extra = 0;
		ChangeActorStat(act, 122);
		break;
	case RRTILE285:
	case RRTILE286:
	case RRTILE287:
	case RRTILE288:
	case RRTILE289:
	case RRTILE290:
	case RRTILE291:
	case RRTILE292:
	case RRTILE293:
		act->spr.cstat = 0;
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		act->spr.xrepeat = 0;
		act->spr.yrepeat = 0;
		act->set_const_clipdist(0);
		act->spr.lotag = 0;
		ChangeActorStat(act, 106);
		break;

	case RRTILE2460:
	case RRTILE2465:
	case BIKEJIBA:
	case BIKEJIBB:
	case BIKEJIBC:
	case BIKERJIBA:
	case BIKERJIBB:
	case BIKERJIBC:
	case BIKERJIBD:
	case CHEERJIBA:
	case CHEERJIBB:
	case CHEERJIBC:
	case CHEERJIBD:
	case FBOATJIBA:
	case FBOATJIBB:
	case RABBITJIBA:
	case RABBITJIBB:
	case RABBITJIBC:
	case MAMAJIBA:
	case MAMAJIBB:
		// impossible to do with better methods outside of redoing the entire switch/case block
		if (isRRRA()) goto rrra_badguy;
		else goto default_case;

	case WATERSPLASH2:
	case MUD:
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
				act->set_int_z(getceilzofslopeptr(act->sector(), act->int_pos().X, act->int_pos().Y) + (16 << 8));
				act->spr.cstat |= CSTAT_SPRITE_YFLIP;
			}
			else if (actj->sector()->lotag == 1)
				act->set_int_z(getflorzofslopeptr(act->sector(), act->int_pos().X, act->int_pos().Y));
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
		if (act->spr.picnum == DOMELITE)
			act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		[[fallthrough]];
	case JIBS1:
	case JIBS2:
	case JIBS3:
	case JIBS4:
	case JIBS5:
	case JIBS6:
	case DUKETORSO:
	case DUKEGUN:
	case DUKELEG:
	case BILLYJIBA:
	case BILLYJIBB:
	case HULKJIBA:
	case HULKJIBB:
	case HULKJIBC:
	case MINJIBA:
	case MINJIBB:
	case MINJIBC:
	case COOTJIBA:
	case COOTJIBB:
	case COOTJIBC:
	rrra_badguy:
		if (act->spr.picnum == JIBS6)
		{
			act->spr.xrepeat >>= 1;
			act->spr.yrepeat >>= 1;
		}
		else if (isRRRA())
		{
			if (act->spr.picnum == RABBITJIBA)
			{
				act->spr.xrepeat = 18;
				act->spr.yrepeat = 18;
			}
			else if (act->spr.picnum == RABBITJIBB)
			{
				act->spr.xrepeat = 36;
				act->spr.yrepeat = 36;
			}
			else if (act->spr.picnum == RABBITJIBC)
			{
				act->spr.xrepeat = 54;
				act->spr.yrepeat = 54;
			}
		}
		ChangeActorStat(act, STAT_MISC);
		break;
	case TONGUE:
		if (actj)
			act->spr.angle = actj->spr.angle;
		act->spr.pos.Z -= gs.playerheight;
		act->set_int_zvel(256 - (krand() & 511));
		act->set_int_xvel(64 - (krand() & 127));
		ChangeActorStat(act, 4);
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
			if (actj->spr.picnum == APLAYER)
				act->temp_data[1] = SMALLSMOKE;
			else
				act->temp_data[1] = actj->spr.picnum;
		}
		else act->spr.xrepeat = act->spr.yrepeat = 0;

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
		act->spr.xrepeat = act->spr.yrepeat = 4;
		act->spr.pos.Z -= 26;
		ChangeActorStat(act, STAT_MISC);
		break;
	case BLOODPOOL:
		if (spawnbloodpoolpart1(act)) break;

		if (actj)
		{
			if (actj->spr.pal == 1)
				act->spr.pal = 1;
			else if (actj->spr.pal != 6 && actj->spr.picnum != NUKEBARREL && actj->spr.picnum != TIRE)
			{
				act->spr.pal = 2; // Red
			}
			else act->spr.pal = 0;  // green

			if (actj->spr.picnum == TIRE)
				act->spr.shade = 127;
		}
		act->spr.cstat |= CSTAT_SPRITE_ALIGNMENT_FLOOR;
		[[fallthrough]];

	case BLOODSPLAT1:
	case BLOODSPLAT2:
	case BLOODSPLAT3:
	case BLOODSPLAT4:
		act->spr.cstat |= CSTAT_SPRITE_ALIGNMENT_WALL;
		act->spr.xrepeat = 7 + (krand() & 7);
		act->spr.yrepeat = 7 + (krand() & 7);
		act->spr.pos.Z -= 16;
		if (actj && actj->spr.pal == 6)
			act->spr.pal = 6;
		insertspriteq(act);
		ChangeActorStat(act, STAT_MISC);
		break;

	case HYDRENT:
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
	case SCALE:
	case VACUUM:
	case FANSPRITE:
	case CACTUS:
	case CACTUSBROKE:
	case CAMERALIGHT:
	case MOVIECAMERA:
	case IVUNIT:
	case POT1:
	case POT2:
	case POT3:
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
		act->set_const_clipdist(32);
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
		act->spr.yint = act->spr.hitag;
		act->spr.hitag = -1;
		[[fallthrough]];
	case QUEBALL:
	case STRIPEBALL:
		if (act->spr.picnum == QUEBALL || act->spr.picnum == STRIPEBALL)
		{
			act->spr.cstat = CSTAT_SPRITE_BLOCK_HITSCAN;
			act->set_const_clipdist(8);
		}
		else
		{
			act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
			act->set_const_clipdist(32);
		}
		ChangeActorStat(act, 2);
		break;
	case BOWLINGBALL:
		act->spr.cstat = CSTAT_SPRITE_BLOCK_HITSCAN;
		act->set_const_clipdist(64);
		act->spr.xrepeat = 11;
		act->spr.yrepeat = 9;
		ChangeActorStat(act, 2);
		break;
	case HENSTAND:
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->set_const_clipdist(48);
		act->spr.xrepeat = 21;
		act->spr.yrepeat = 15;
		ChangeActorStat(act, 2);
		break;
	case RRTILE295:
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		ChangeActorStat(act, 107);
		break;
	case RRTILE296:
	case RRTILE297:
		act->spr.xrepeat = 64;
		act->spr.yrepeat = 64;
		act->set_const_clipdist(64);
		ChangeActorStat(act, 108);
		break;
	case RRTILE3190:
	case RRTILE3191:
	case RRTILE3192:
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->set_const_clipdist(8);
		act->spr.xrepeat = 32;
		act->spr.yrepeat = 26;
		act->vel.X = 2;
		ChangeActorStat(act, 1);
		break;
	case RRTILE3120:
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->set_const_clipdist(8);
		act->spr.xrepeat = 12;
		act->spr.yrepeat = 10;
		act->vel.X = 2;
		ChangeActorStat(act, 1);
		break;
	case RRTILE3122:
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->set_const_clipdist(2);
		act->spr.xrepeat = 8;
		act->spr.yrepeat = 6;
		act->vel.X = 1;
		ChangeActorStat(act, 1);
		break;
	case RRTILE3123:
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->set_const_clipdist(8);
		act->spr.xrepeat = 13;
		act->spr.yrepeat = 13;
		act->vel.X = 1;
		ChangeActorStat(act, 1);
		break;
	case RRTILE3124:
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->set_const_clipdist(8);
		act->spr.xrepeat = 17;
		act->spr.yrepeat = 12;
		act->vel.X = 2;
		ChangeActorStat(act, 1);
		break;
	case RRTILE3132:
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->set_const_clipdist(8);
		act->spr.xrepeat = 13;
		act->spr.yrepeat = 10;
		act->vel.X = 0;
		ChangeActorStat(act, 1);
		break;
	case BOWLINGPIN:
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->set_const_clipdist(48);
		act->spr.xrepeat = 23;
		act->spr.yrepeat = 23;
		ChangeActorStat(act, 2);
		break;
	case DUKELYINGDEAD:
		if (actj && actj->spr.picnum == APLAYER)
		{
			act->spr.xrepeat = actj->spr.xrepeat;
			act->spr.yrepeat = actj->spr.yrepeat;
			act->spr.shade = actj->spr.shade;
			act->spr.pal = ps[actj->PlayerIndex()].palookup;
		}
		act->spr.cstat = 0;
		act->spr.extra = 1;
		act->set_int_xvel(292);
		act->set_int_zvel(360);
		[[fallthrough]];
	case RESPAWNMARKERRED:
		if (act->spr.picnum == RESPAWNMARKERRED)
		{
			act->spr.xrepeat = act->spr.yrepeat = 8;
			if (actj) act->spr.pos.Z = actj->floorz;
		}
		else
		{
			act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
			act->set_const_clipdist(128);
		}
		[[fallthrough]];
	case MIKE:
		if (act->spr.picnum == MIKE)
			act->spr.yint = act->spr.hitag;
		ChangeActorStat(act, 1);
		break;

	case SPOTLITE:
		act->temp_data[0] = act->int_pos().X;
		act->temp_data[1] = act->int_pos().Y;
		break;
	case BULLETHOLE:
		act->spr.xrepeat = act->spr.yrepeat = 3;
		act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_WALL | randomFlip();
		insertspriteq(act);
		[[fallthrough]];
	case MONEY:
		if (act->spr.picnum == MONEY)
		{
			act->temp_data[0] = krand() & 2047;
			act->spr.cstat = randomFlip();
			act->spr.xrepeat = act->spr.yrepeat = 8;
			act->set_int_ang(krand() & 2047);
		}
		ChangeActorStat(act, STAT_MISC);
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
		ChangeActorStat(act, STAT_FX);
		break;
	case SOUNDFX:
	{
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		ChangeActorStat(act, STAT_ZOMBIEACTOR);
	}
	break;
	case EXPLOSION2:
	case EXPLOSION3:
	case BURNING:
	case SMALLSMOKE:
		if (actj)
		{
			act->spr.angle = actj->spr.angle;
			act->spr.shade = -64;
			act->spr.cstat = CSTAT_SPRITE_YCENTER | randomXFlip();
		}

		if (act->spr.picnum == EXPLOSION2)
		{
			act->spr.xrepeat = 48;
			act->spr.yrepeat = 48;
			act->spr.shade = -127;
			act->spr.cstat |= CSTAT_SPRITE_YCENTER;
		}
		else if (act->spr.picnum == EXPLOSION3)
		{
			act->spr.xrepeat = 128;
			act->spr.yrepeat = 128;
			act->spr.shade = -127;
			act->spr.cstat |= CSTAT_SPRITE_YCENTER;
		}
		else if (act->spr.picnum == SMALLSMOKE)
		{
			act->spr.xrepeat = 12;
			act->spr.yrepeat = 12;
		}
		else if (act->spr.picnum == BURNING)
		{
			act->spr.xrepeat = 4;
			act->spr.yrepeat = 4;
		}

		if (actj)
		{
			int x = getflorzofslopeptr(act->sector(), act->int_pos().X, act->int_pos().Y);
			if (act->int_pos().Z > x - (12 << 8))
				act->set_int_z(x - (12 << 8));
		}

		ChangeActorStat(act, STAT_MISC);

		break;

	case PLAYERONWATER:
		if (actj)
		{
			act->spr.xrepeat = actj->spr.xrepeat;
			act->spr.yrepeat = actj->spr.yrepeat;
			act->vel.Z = 0.5;
			if (act->sector()->lotag != 2)
				act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		}
		ChangeActorStat(act, 13);
		break;

	case APLAYER:
	{
		act->spr.xrepeat = act->spr.yrepeat = 0;
		int j = ud.coop;
		if (j == 2) j = 0;

		if (ud.multimode < 2 || (ud.multimode > 1 && j != act->spr.lotag))
			ChangeActorStat(act, STAT_MISC);
		else
			ChangeActorStat(act, 10);
		break;
	}
	case WATERBUBBLE:
		if (actj && actj->spr.picnum == APLAYER)
			act->spr.pos.Z -= 16;
		if (act->spr.picnum == WATERBUBBLE)
		{
			if (actj)
				act->spr.angle = actj->spr.angle;
			act->spr.xrepeat = act->spr.yrepeat = 1 + (krand() & 7);
		}
		else
			act->spr.xrepeat = act->spr.yrepeat = 32;
		ChangeActorStat(act, STAT_MISC);
		break;
	case CRANE:
		initcrane(actj, act, CRANEPOLE);
		break;
	case WATERDRIP:
		initwaterdrip(actj, act);
		break;
	case TRASH:

		if (act->spr.picnum != WATERDRIP) act->set_int_ang(krand() & 2047);

		act->spr.xrepeat = 24;
		act->spr.yrepeat = 24;
		ChangeActorStat(act, 6);
		break;

	case PLUG:
		act->spr.lotag = 9999;
		ChangeActorStat(act, 6);
		break;
	case TOUCHPLATE:
		act->temp_data[2] = sectp->int_floorz();
		if (sectp->lotag != 1 && sectp->lotag != 2)
			sectp->setfloorz(act->spr.pos.Z);
		if (act->spr.pal && ud.multimode > 1)
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
			break;
		}
		[[fallthrough]];
	case WATERBUBBLEMAKER:
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		ChangeActorStat(act, 6);
		break;
	case BOLT1:
	case BOLT1 + 1:
	case BOLT1 + 2:
	case BOLT1 + 3:
		act->temp_data[0] = act->spr.xrepeat;
		act->temp_data[1] = act->spr.yrepeat;
		[[fallthrough]];
	case MASTERSWITCH:
		if (act->spr.picnum == MASTERSWITCH)
			act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		act->spr.yint = 0;
		ChangeActorStat(act, 6);
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
	case UFO1_RRRA:
		if (isRRRA()) goto rrra_badguy2;
		else goto default_case;

	case UFO1_RR:
		if (!isRRRA()) goto rrra_badguy2;
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
	case RAT:
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
	case UFO2:
	case UFO3:
	case UFO4:
	case UFO5:
	case COW:
	case COOT:
	case SHARK:
	case VIXEN:
	rrra_badguy2:
		act->spr.xrepeat = 40;
		act->spr.yrepeat = 40;
		// Note: All inappropriate tiles have already been weeded out by the outer switch block so this does not need game type checks anymore.
		switch (act->spr.picnum)
		{
		case VIXEN:
			if (act->spr.pal == 34)
			{
				act->spr.xrepeat = 22;
				act->spr.yrepeat = 21;
			}
			else
			{
				act->spr.xrepeat = 22;
				act->spr.yrepeat = 20;
			}
			act->setClipDistFromTile();
			break;
		case HULKHANG:
		case HULKHANGDEAD:
		case HULKJUMP:
		case HULK:
		case HULKSTAYPUT:
			act->spr.xrepeat = 32;
			act->spr.yrepeat = 32;
			act->setClipDistFromTile();
			break;
		case COOTPLAY:
		case COOT:
		case COOTSTAYPUT:
			act->spr.xrepeat = 24;
			act->spr.yrepeat = 18;
			act->setClipDistFromTile();
			act->spr.clipdist <<= 2;
			break;
		case DRONE:
			act->spr.xrepeat = 14;
			act->spr.yrepeat = 7;
			act->set_const_clipdist(128);
			break;
		case SBSWIPE:
		case BILLYPLAY:
		case BILLYCOCK:
		case BILLYRAY:
		case BILLYRAYSTAYPUT:
		case BRAYSNIPER:
		case BUBBASTAND:
			act->spr.xrepeat = 25;
			act->spr.yrepeat = 21;
			act->setClipDistFromTile();
			break;
		case COW:
			act->spr.xrepeat = 32;
			act->spr.yrepeat = 32;
			act->setClipDistFromTile();
			break;
		case HEN:
		case HENSTAYPUT:
		case HENSTAND:
			if (act->spr.pal == 35)
			{
				act->spr.xrepeat = 42;
				act->spr.yrepeat = 30;
				act->setClipDistFromTile();
			}
			else
			{
				act->spr.xrepeat = 21;
				act->spr.yrepeat = 15;
				act->set_const_clipdist(64);
			}
			break;
		case MINION:
		case MINIONSTAYPUT:
			act->spr.xrepeat = 16;
			act->spr.yrepeat = 16;
			act->setClipDistFromTile();
			if (isRRRA() && ufospawnsminion)
				act->spr.pal = 8;
			break;
		case DOGRUN:
		case PIG:
			act->spr.xrepeat = 16;
			act->spr.yrepeat = 16;
			act->setClipDistFromTile();
			break;
		case RABBIT:
			act->spr.xrepeat = 18;
			act->spr.yrepeat = 18;
			act->setClipDistFromTile();
			break;
		case MAMACLOUD:
			act->spr.xrepeat = 64;
			act->spr.yrepeat = 64;
			act->spr.cstat = CSTAT_SPRITE_TRANSLUCENT;
			act->spr.cstat |= CSTAT_SPRITE_TRANS_FLIP;
			act->add_int_pos({ (krand() & 2047) - 1024, (krand() & 2047) - 1024, (krand() & 2047) - 1024 });
			break;
		case MAMA:
			if (act->spr.pal == 30)
			{
				act->spr.xrepeat = 26;
				act->spr.yrepeat = 26;
				act->set_const_clipdist(75);
			}
			else if (act->spr.pal == 31)
			{
				act->spr.xrepeat = 36;
				act->spr.yrepeat = 36;
				act->set_const_clipdist(100);
			}
			else if (act->spr.pal == 32)
			{
				act->spr.xrepeat = 50;
				act->spr.yrepeat = 50;
				act->set_const_clipdist(100);
			}
			else
			{
				act->spr.xrepeat = 50;
				act->spr.yrepeat = 50;
				act->set_const_clipdist(100);
			}
			break;
		case BIKERB:
			act->spr.xrepeat = 28;
			act->spr.yrepeat = 22;
			act->set_const_clipdist(72);
			break;
		case BIKERBV2:
			act->spr.xrepeat = 28;
			act->spr.yrepeat = 22;
			act->set_const_clipdist(72);
			break;
		case BIKER:
			act->spr.xrepeat = 28;
			act->spr.yrepeat = 22;
			act->setClipDistFromTile();
			break;
		case CHEERB:
			act->spr.xrepeat = 28;
			act->spr.yrepeat = 22;
			act->set_const_clipdist(72);
			break;
		case CHEER:
		case CHEERSTAYPUT:
			act->spr.xrepeat = 20;
			act->spr.yrepeat = 20;
			act->setClipDistFromTile();
			break;
		case MAKEOUT:
			act->spr.xrepeat = 26;
			act->spr.yrepeat = 26;
			act->setClipDistFromTile();
			break;
		case MINIONBOAT:
			act->spr.xrepeat = 16;
			act->spr.yrepeat = 16;
			act->setClipDistFromTile();
			break;
		case HULKBOAT:
			act->spr.xrepeat = 48;
			act->spr.yrepeat = 48;
			act->setClipDistFromTile();
			break;
		case CHEERBOAT:
			act->spr.xrepeat = 32;
			act->spr.yrepeat = 32;
			act->setClipDistFromTile();
			break;

		case TORNADO:
			act->spr.xrepeat = 64;
			act->spr.yrepeat = 128;
			act->setClipDistFromTile();
			act->spr.clipdist >>= 2;
			act->spr.cstat = CSTAT_SPRITE_TRANSLUCENT;
			break;
		case LTH:
			act->spr.xrepeat = 24;
			act->spr.yrepeat = 22;
			act->setClipDistFromTile();
			break;
		case ROCK:
		case ROCK2:
			act->spr.xrepeat = 64;
			act->spr.yrepeat = 64;
			act->setClipDistFromTile();
			break;

		case UFO1_RRRA:
		case UFO1_RR:
		case UFO2:
		case UFO3:
		case UFO4:
		case UFO5:
			act->spr.xrepeat = 32;
			act->spr.yrepeat = 32;
			act->setClipDistFromTile();
			act->spr.extra = 50;
			break;
		case SBMOVE:
			act->spr.xrepeat = 48;
			act->spr.yrepeat = 48;
			act->setClipDistFromTile();
			break;

		default:
			break;
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
				act->set_int_ang(krand() & 2047);
				act->spr.xrepeat = act->spr.yrepeat = 48;
				act->spr.cstat = 0;
			}
			else
			{
				act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;

				if (act->spr.picnum != 5501)
					if (actorfella(act))
						ps[myconnectindex].max_actors_killed++;
			}

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
	case LOCATORS:
		//                act->spr.xrepeat=act->spr.yrepeat=0;
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		ChangeActorStat(act, STAT_LOCATOR);
		break;

	case ACTIVATORLOCKED:
	case ACTIVATOR:
		//                act->spr.xrepeat=act->spr.yrepeat=0;
		act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		if (act->spr.picnum == ACTIVATORLOCKED)
			sectp->lotag ^= 16384;
		ChangeActorStat(act, STAT_ACTIVATOR);
		break;
	case DOORSHOCK:
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		act->spr.shade = -12;

		ChangeActorStat(act, STAT_STANDABLE);
		break;

	case OOZ:
	{
		act->spr.shade = -12;

		if (actj)
			if (actj->spr.picnum == NUKEBARREL)
				act->spr.pal = 8;

		ChangeActorStat(act, STAT_STANDABLE);

		getglobalz(act);

		int j = int((act->floorz - act->ceilingz) * 0.5);

		act->spr.yrepeat = j;
		act->spr.xrepeat = 25 - (j >> 1);
		if(krand() & 4) act->spr.cstat |= CSTAT_SPRITE_XFLIP;
		break;
	}
	case HEAVYHBOMB:
		act->SetOwner(act);
		act->spr.xrepeat = act->spr.yrepeat = 9;
		act->spr.yint = 4;
		[[fallthrough]];
	case REACTOR2:
	case REACTOR:
	case RECON:
		if (initreactor(actj, act, act->spr.picnum == RECON)) return act;
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
				act->set_int_zvel(-(4 << 8));
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
		switch (act->spr.picnum)
		{
		case FIRSTGUNSPRITE:
			act->spr.xrepeat = 16;
			act->spr.yrepeat = 16;
			break;
		case SHOTGUNAMMO:
			act->spr.xrepeat = 18;
			act->spr.yrepeat = 17;
			if (isRRRA()) act->spr.cstat = CSTAT_SPRITE_BLOCK_HITSCAN;
			break;
		case SIXPAK:
			act->spr.xrepeat = 13;
			act->spr.yrepeat = 9;
			if (isRRRA()) act->spr.cstat = CSTAT_SPRITE_BLOCK_HITSCAN;
			break;
		case FIRSTAID:
			act->spr.xrepeat = 8;
			act->spr.yrepeat = 8;
			break;
		case BEER:
			act->spr.xrepeat = 5;
			act->spr.yrepeat = 4;
			break;
		case AMMO:
			act->spr.xrepeat = 9;
			act->spr.yrepeat = 9;
			break;
		case MOTOAMMO:
			if (!isRRRA()) goto default_case;
			act->spr.xrepeat = 23;
			act->spr.yrepeat = 23;
			break;
		case BOATAMMO:
			if (!isRRRA()) goto default_case;
			act->spr.xrepeat = 16;
			act->spr.yrepeat = 16;
			break;
		case COWPIE:
			act->spr.xrepeat = 8;
			act->spr.yrepeat = 6;
			break;
		case STEROIDS:
			act->spr.xrepeat = 13;
			act->spr.yrepeat = 9;
			break;
		case ACCESSCARD:
			act->spr.xrepeat = 11;
			act->spr.yrepeat = 12;
			break;
		case HEATSENSOR:
			act->spr.xrepeat = 6;
			act->spr.yrepeat = 4;
			break;
		case AIRTANK:
			act->spr.xrepeat = 19;
			act->spr.yrepeat = 16;
			break;
		case BATTERYAMMO:
			act->spr.xrepeat = 15;
			act->spr.yrepeat = 15;
			break;
		case BOWLINGBALLSPRITE:
			act->spr.xrepeat = 11;
			act->spr.yrepeat = 11;
			break;
		case POWDERKEG:
			act->spr.xrepeat = 11;
			act->spr.yrepeat = 11;
			act->spr.yint = 4;
			act->vel.X = 2;
			break;
		case RPGSPRITE:
			act->spr.xrepeat = 16;
			act->spr.yrepeat = 14;
			break;
		case RPG2SPRITE:
			if (!isRRRA()) goto default_case;
			act->spr.xrepeat = 20;
			act->spr.yrepeat = 20;
			break;
		case SHRINKERSPRITE:
			act->spr.xrepeat = 22;
			act->spr.yrepeat = 13;
			break;
		case DEVISTATORSPRITE:
			act->spr.xrepeat = 18;
			act->spr.yrepeat = 17;
			break;
		case SAWAMMO:
			act->spr.xrepeat = 12;
			act->spr.yrepeat = 7;
			break;
		case GROWSPRITEICON:
			act->spr.xrepeat = 10;
			act->spr.yrepeat = 9;
			break;
		case DEVISTATORAMMO:
			act->spr.xrepeat = 10;
			act->spr.yrepeat = 9;
			break;
		case ATOMICHEALTH:
			act->spr.xrepeat = 8;
			act->spr.yrepeat = 8;
			break;
		case FREEZESPRITE:
			act->spr.xrepeat = 17;
			act->spr.yrepeat = 16;
			break;
		}
		act->spr.shade = act->sector()->floorshade;
		break;
	case WATERFOUNTAIN:
		act->spr.lotag = 1;
		[[fallthrough]];
	case TREE1:
	case TREE2:
	case TIRE:
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL; // Make it hitable
		act->spr.extra = 1;
		ChangeActorStat(act, 6);
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

		if (ud.multimode < 2 && act->spr.pal != 0)
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
			break;
		}
		else act->spr.pal = 0;
		if (act->spr.picnum == CAMERAPOLE) break;
		act->spr.picnum = CAMERA1;
		ChangeActorStat(act, 1);
		break;
	case STEAM:
		if (actj)
		{
			act->spr.angle = actj->spr.angle;
			act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
			act->spr.xrepeat = act->spr.yrepeat = 1;
			act->set_int_xvel(-8);
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
		act->spr.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_ALIGNMENT_WALL;
		act->spr.extra = 1;
		if (ud.multimode < 2 && act->spr.pal != 0)
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
			break;
		}

		act->spr.pal = 0;
		act->SetOwner(act);
		ChangeActorStat(act, STAT_STANDABLE);
		act->set_int_xvel(8);
		ssp(act, CLIPMASK0);
		break;

	case EMPTYBIKE:
		if (!isRRRA()) goto default_case;
		if (ud.multimode < 2 && act->spr.pal == 1)
		{
			act->spr.xrepeat = act->spr.yrepeat = 0;
			break;
		}
		act->spr.pal = 0;
		act->spr.xrepeat = 18;
		act->spr.yrepeat = 18;
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
			act->spr.xrepeat = act->spr.yrepeat = 0;
			break;
		}
		act->spr.pal = 0;
		act->spr.xrepeat = 32;
		act->spr.yrepeat = 32;
		act->setClipDistFromTile();
		act->saved_ammo = 20;
		act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		act->spr.lotag = 1;
		ChangeActorStat(act, 1);
		break;

	case TOILET:
	case STALL:
	case RRTILE2121:
	case RRTILE2122:
		act->spr.lotag = 1;
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		act->set_const_clipdist(8);
		act->SetOwner(act);
		break;
	case CANWITHSOMETHING:
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
		act->set_const_clipdist(72);
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
				act->set_const_clipdist(24);
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
		act->spr.xrepeat = 1;
		act->spr.yrepeat = 1;
		act->set_const_clipdist(1);
		ChangeActorStat(act, 100);
		break;
	}
	return act;
}

END_DUKE_NS
