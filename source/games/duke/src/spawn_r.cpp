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

DDukeActor* spawninit_r(DDukeActor* actj, DDukeActor* act)
{
	auto sp = act->s;
	auto spj = actj == nullptr ? nullptr : actj->s;
	auto t = act->temp_data;
	auto sectp = sp->sector();

	switch (sp->picnum)
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
		sp->cstat = 0;
		sp->cstat |= 32768;
		sp->xrepeat = 0;
		sp->yrepeat = 0;
		sp->clipdist = 0;
		sp->extra = 0;
		changeactorstat(act, STAT_BOWLING);
		break;
	case RRTILE3410:
		sp->extra = 0;
		changeactorstat(act, 107);
		break;
	case RRTILE8450:
		if (!isRRRA()) goto default_case;
		sp->xrepeat = 64;
		sp->yrepeat = 64;
		sp->extra = sp->lotag;
		sp->cstat |= 257;
		changeactorstat(act, 116);
		break;
	case PIG + 11:
		if (!isRRRA()) goto default_case;
		sp->xrepeat = 16;
		sp->yrepeat = 16;
		sp->clipdist = 0;
		sp->extra = 0;
		sp->cstat = 0;
		changeactorstat(act, 121);
		break;
	case RRTILE8487:
	case RRTILE8489:
		if (!isRRRA()) goto default_case;
		sp->xrepeat = 32;
		sp->yrepeat = 32;
		sp->extra = 0;
		sp->cstat |= 257;
		sp->hitag = 0;
		changeactorstat(act, 117);
		break;
	case RRTILE7424:
		if (!isRRRA()) goto default_case;
		sp->extra = 0;
		sp->xrepeat = 0;
		sp->yrepeat = 0;
		changeactorstat(act, 11);
		break;
	case RRTILE7936:
		if (!isRRRA()) goto default_case;
		sp->xrepeat = 0;
		sp->yrepeat = 0;
		fogactive = 1;
		break;
	case RRTILE6144:
		if (!isRRRA()) goto default_case;
		sp->xrepeat = 0;
		sp->yrepeat = 0;
		ps[screenpeek].sea_sick_stat = 1;
		break;
	case RRTILE8448:
		if (!isRRRA()) goto default_case;
		sp->lotag = 1;
		sp->clipdist = 0;
		break;
	case RRTILE8099:
		if (!isRRRA()) goto default_case;
		sp->lotag = 5;
		sp->clipdist = 0;
		changeactorstat(act, 123);
		break;
	case RRTILE8704:
		if (!isRRRA()) goto default_case;
		sp->lotag = 1;
		sp->clipdist = 0;
		break;
	case RRTILE8192:
		if (!isRRRA()) goto default_case;
		sp->xrepeat = 0;
		sp->yrepeat = 0;
		ufospawnsminion = 1;
		break;
	case RRTILE8193:
		if (!isRRRA()) goto default_case;
		sp->xrepeat = 0;
		sp->yrepeat = 0;
		pistonsound = 1;
		break;
	case RRTILE8165:
		if (!isRRRA()) goto default_case;
		sp->lotag = 1;
		sp->clipdist = 0;
		act->SetOwner(act);
		sp->extra = 0;
		changeactorstat(act, 115);
		break;
	case RRTILE8593:
		if (!isRRRA()) goto default_case;
		sp->lotag = 1;
		sp->clipdist = 0;
		act->SetOwner(act);
		sp->extra = 0;
		changeactorstat(act, 122);
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
		sp->cstat = 0;
		sp->cstat |= 32768;
		sp->xrepeat = 0;
		sp->yrepeat = 0;
		sp->clipdist = 0;
		sp->lotag = 0;
		changeactorstat(act, 106);
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
		if (spj)
		{
			setsprite(act, spj->x, spj->y, spj->z);
			sp->xrepeat = sp->yrepeat = 8 + (krand() & 7);
		}
		else sp->xrepeat = sp->yrepeat = 16 + (krand() & 15);

		sp->shade = -16;
		sp->cstat |= 128;
		if (spj)
		{
			if (spj->sector()->lotag == 2)
			{
				sp->z = getceilzofslope(sp->sectnum, sp->x, sp->y) + (16 << 8);
				sp->cstat |= 8;
			}
			else if (spj->sector()->lotag == 1)
				sp->z = getflorzofslope(sp->sectnum, sp->x, sp->y);
		}

		if (sectp->floorpicnum == FLOORSLIME ||
			sectp->ceilingpicnum == FLOORSLIME)
			sp->pal = 7;
		[[fallthrough]];
	case NEON1:
	case NEON2:
	case NEON3:
	case NEON4:
	case NEON5:
	case NEON6:
	case DOMELITE:
		if (sp->picnum != WATERSPLASH2)
			sp->cstat |= 257;
		if (sp->picnum == DOMELITE)
			sp->cstat |= 257;
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
		if (sp->picnum == JIBS6)
		{
			sp->xrepeat >>= 1;
			sp->yrepeat >>= 1;
		}
		else if (isRRRA())
		{
			if (sp->picnum == RABBITJIBA)
			{
				sp->xrepeat = 18;
				sp->yrepeat = 18;
			}
			else if (sp->picnum == RABBITJIBB)
			{
				sp->xrepeat = 36;
				sp->yrepeat = 36;
			}
			else if (sp->picnum == RABBITJIBC)
			{
				sp->xrepeat = 54;
				sp->yrepeat = 54;
			}
		}
		changeactorstat(act, STAT_MISC);
		break;
	case TONGUE:
		if (spj)
			sp->ang = spj->ang;
		sp->z -= PHEIGHT_RR;
		sp->zvel = 256 - (krand() & 511);
		sp->xvel = 64 - (krand() & 127);
		changeactorstat(act, 4);
		break;
	case TRANSPORTERSTAR:
	case TRANSPORTERBEAM:
		spawntransporter(actj, act, sp->picnum == TRANSPORTERBEAM);
		break;

	case FRAMEEFFECT1:
		if (spj)
		{
			sp->xrepeat = spj->xrepeat;
			sp->yrepeat = spj->yrepeat;
			if (spj->picnum == APLAYER)
				t[1] = SMALLSMOKE;
			else
				t[1] = spj->picnum;
		}
		else sp->xrepeat = sp->yrepeat = 0;

		changeactorstat(act, STAT_MISC);
		break;

	case FORCESPHERE:
		if (!spj)
		{
			sp->cstat = 32768;
			changeactorstat(act, 2);
		}
		else
		{
			sp->xrepeat = sp->yrepeat = 1;
			changeactorstat(act, STAT_MISC);
		}
		break;

	case BLOOD:
		sp->xrepeat = sp->yrepeat = 4;
		sp->z -= (26 << 8);
		changeactorstat(act, STAT_MISC);
		break;
	case BLOODPOOL:
		if (spawnbloodpoolpart1(act)) break;

		if (spj)
		{
			if (spj->pal == 1)
				sp->pal = 1;
			else if (spj->pal != 6 && spj->picnum != NUKEBARREL && spj->picnum != TIRE)
			{
				sp->pal = 2; // Red
			}
			else sp->pal = 0;  // green

			if (spj->picnum == TIRE)
				sp->shade = 127;
		}
		sp->cstat |= 32;
		[[fallthrough]];

	case BLOODSPLAT1:
	case BLOODSPLAT2:
	case BLOODSPLAT3:
	case BLOODSPLAT4:
		sp->cstat |= 16;
		sp->xrepeat = 7 + (krand() & 7);
		sp->yrepeat = 7 + (krand() & 7);
		sp->z -= (16 << 8);
		if (spj && spj->pal == 6)
			sp->pal = 6;
		insertspriteq(act);
		changeactorstat(act, STAT_MISC);
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
		sp->clipdist = 32;
		sp->cstat |= 257;
		changeactorstat(act, 0);
		break;
	case FEMMAG1:
	case FEMMAG2:
		sp->cstat &= ~257;
		changeactorstat(act, 0);
		break;

	case MASKWALL7:
	{
		int j = sp->cstat & 60;
		sp->cstat = j | 1;
		changeactorstat(act, 0);
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
		sp->yvel = sp->hitag;
		sp->hitag = -1;
		[[fallthrough]];
	case QUEBALL:
	case STRIPEBALL:
		if (sp->picnum == QUEBALL || sp->picnum == STRIPEBALL)
		{
			sp->cstat = 256;
			sp->clipdist = 8;
		}
		else
		{
			sp->cstat |= 257;
			sp->clipdist = 32;
		}
		changeactorstat(act, 2);
		break;
	case BOWLINGBALL:
		sp->cstat = 256;
		sp->clipdist = 64;
		sp->xrepeat = 11;
		sp->yrepeat = 9;
		changeactorstat(act, 2);
		break;
	case HENSTAND:
		sp->cstat = 257;
		sp->clipdist = 48;
		sp->xrepeat = 21;
		sp->yrepeat = 15;
		changeactorstat(act, 2);
		break;
	case RRTILE295:
		sp->cstat |= 32768;
		changeactorstat(act, 107);
		break;
	case RRTILE296:
	case RRTILE297:
		sp->xrepeat = 64;
		sp->yrepeat = 64;
		sp->clipdist = 64;
		changeactorstat(act, 108);
		break;
	case RRTILE3190:
	case RRTILE3191:
	case RRTILE3192:
		sp->cstat = 257;
		sp->clipdist = 8;
		sp->xrepeat = 32;
		sp->yrepeat = 26;
		sp->xvel = 32;
		changeactorstat(act, 1);
		break;
	case RRTILE3120:
		sp->cstat = 257;
		sp->clipdist = 8;
		sp->xrepeat = 12;
		sp->yrepeat = 10;
		sp->xvel = 32;
		changeactorstat(act, 1);
		break;
	case RRTILE3122:
		sp->cstat = 257;
		sp->clipdist = 2;
		sp->xrepeat = 8;
		sp->yrepeat = 6;
		sp->xvel = 16;
		changeactorstat(act, 1);
		break;
	case RRTILE3123:
		sp->cstat = 257;
		sp->clipdist = 8;
		sp->xrepeat = 13;
		sp->yrepeat = 13;
		sp->xvel = 16;
		changeactorstat(act, 1);
		break;
	case RRTILE3124:
		sp->cstat = 257;
		sp->clipdist = 8;
		sp->xrepeat = 17;
		sp->yrepeat = 12;
		sp->xvel = 32;
		changeactorstat(act, 1);
		break;
	case RRTILE3132:
		sp->cstat = 257;
		sp->clipdist = 8;
		sp->xrepeat = 13;
		sp->yrepeat = 10;
		sp->xvel = 0;
		changeactorstat(act, 1);
		break;
	case BOWLINGPIN:
		sp->cstat = 257;
		sp->clipdist = 48;
		sp->xrepeat = 23;
		sp->yrepeat = 23;
		changeactorstat(act, 2);
		break;
	case DUKELYINGDEAD:
		if (spj && spj->picnum == APLAYER)
		{
			sp->xrepeat = spj->xrepeat;
			sp->yrepeat = spj->yrepeat;
			sp->shade = spj->shade;
			sp->pal = ps[spj->yvel].palookup;
		}
		sp->cstat = 0;
		sp->extra = 1;
		sp->xvel = 292;
		sp->zvel = 360;
		[[fallthrough]];
	case RESPAWNMARKERRED:
		if (sp->picnum == RESPAWNMARKERRED)
		{
			sp->xrepeat = sp->yrepeat = 8;
			if (spj) sp->z = actj->floorz;
		}
		else
		{
			sp->cstat |= 257;
			sp->clipdist = 128;
		}
		[[fallthrough]];
	case MIKE:
		if (sp->picnum == MIKE)
			sp->yvel = sp->hitag;
		changeactorstat(act, 1);
		break;

	case SPOTLITE:
		t[0] = sp->x;
		t[1] = sp->y;
		break;
	case BULLETHOLE:
		sp->xrepeat = sp->yrepeat = 3;
		sp->cstat = 16 + (krand() & 12);
		insertspriteq(act);
		[[fallthrough]];
	case MONEY:
		if (sp->picnum == MONEY)
		{
			act->temp_data[0] = krand() & 2047;
			sp->cstat = krand() & 12;
			sp->xrepeat = sp->yrepeat = 8;
			sp->ang = krand() & 2047;
		}
		changeactorstat(act, STAT_MISC);
		break;

	case SHELL: //From the player
	case SHOTGUNSHELL:
		initshell(actj, act, sp->picnum == SHELL);
		break;
	case RESPAWN:
		sp->extra = 66 - 13;
		[[fallthrough]];
	case MUSICANDSFX:
		if (ud.multimode < 2 && sp->pal == 1)
		{
			sp->xrepeat = sp->yrepeat = 0;
			changeactorstat(act, STAT_MISC);
			break;
		}
		sp->cstat = 32768;
		changeactorstat(act, 11);
		break;
	case SOUNDFX:
	{
		sp->cstat |= 32768;
		changeactorstat(act, 2);
	}
	break;
	case EXPLOSION2:
	case EXPLOSION3:
	case BURNING:
	case SMALLSMOKE:
		if (spj)
		{
			sp->ang = spj->ang;
			sp->shade = -64;
			sp->cstat = 128 | (krand() & 4);
		}

		if (sp->picnum == EXPLOSION2)
		{
			sp->xrepeat = 48;
			sp->yrepeat = 48;
			sp->shade = -127;
			sp->cstat |= 128;
		}
		else if (sp->picnum == EXPLOSION3)
		{
			sp->xrepeat = 128;
			sp->yrepeat = 128;
			sp->shade = -127;
			sp->cstat |= 128;
		}
		else if (sp->picnum == SMALLSMOKE)
		{
			sp->xrepeat = 12;
			sp->yrepeat = 12;
		}
		else if (sp->picnum == BURNING)
		{
			sp->xrepeat = 4;
			sp->yrepeat = 4;
		}

		if (spj)
		{
			int x = getflorzofslope(sp->sectnum, sp->x, sp->y);
			if (sp->z > x - (12 << 8))
				sp->z = x - (12 << 8);
		}

		changeactorstat(act, STAT_MISC);

		break;

	case PLAYERONWATER:
		if (spj)
		{
			sp->xrepeat = spj->xrepeat;
			sp->yrepeat = spj->yrepeat;
			sp->zvel = 128;
			if (sp->sector()->lotag != 2)
				sp->cstat |= 32768;
		}
		changeactorstat(act, 13);
		break;

	case APLAYER:
	{
		sp->xrepeat = sp->yrepeat = 0;
		int j = ud.coop;
		if (j == 2) j = 0;

		if (ud.multimode < 2 || (ud.multimode > 1 && j != sp->lotag))
			changeactorstat(act, STAT_MISC);
		else
			changeactorstat(act, 10);
		break;
	}
	case WATERBUBBLE:
		if (spj && spj->picnum == APLAYER)
			sp->z -= (16 << 8);
		if (sp->picnum == WATERBUBBLE)
		{
			if (spj)
				sp->ang = spj->ang;
			sp->xrepeat = sp->yrepeat = 1 + (krand() & 7);
		}
		else
			sp->xrepeat = sp->yrepeat = 32;
		changeactorstat(act, STAT_MISC);
		break;
	case CRANE:
		initcrane(actj, act, CRANEPOLE);
		break;
	case WATERDRIP:
		initwaterdrip(actj, act);
		break;
	case TRASH:

		if (sp->picnum != WATERDRIP) sp->ang = krand() & 2047;

		sp->xrepeat = 24;
		sp->yrepeat = 24;
		changeactorstat(act, 6);
		break;

	case PLUG:
		sp->lotag = 9999;
		changeactorstat(act, 6);
		break;
	case TOUCHPLATE:
		t[2] = sectp->floorz;
		if (sectp->lotag != 1 && sectp->lotag != 2)
			sectp->floorz = sp->z;
		if (sp->pal && ud.multimode > 1)
		{
			sp->xrepeat = sp->yrepeat = 0;
			changeactorstat(act, STAT_MISC);
			break;
		}
		[[fallthrough]];
	case WATERBUBBLEMAKER:
		sp->cstat |= 32768;
		changeactorstat(act, 6);
		break;
	case BOLT1:
	case BOLT1 + 1:
	case BOLT1 + 2:
	case BOLT1 + 3:
		t[0] = sp->xrepeat;
		t[1] = sp->yrepeat;
		[[fallthrough]];
	case MASTERSWITCH:
		if (sp->picnum == MASTERSWITCH)
			sp->cstat |= 32768;
		sp->yvel = 0;
		changeactorstat(act, 6);
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
		act->actorstayput = sp->sector();
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
		sp->xrepeat = 40;
		sp->yrepeat = 40;
		// Note: All inappropriate tiles have already been weeded out by the outer switch block so this does not need game type checks anymore.
		switch (sp->picnum)
		{
		case VIXEN:
			if (sp->pal == 34)
			{
				sp->xrepeat = 22;
				sp->yrepeat = 21;
			}
			else
			{
				sp->xrepeat = 22;
				sp->yrepeat = 20;
			}
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case HULKHANG:
		case HULKHANGDEAD:
		case HULKJUMP:
		case HULK:
		case HULKSTAYPUT:
			sp->xrepeat = 32;
			sp->yrepeat = 32;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case COOTPLAY:
		case COOT:
		case COOTSTAYPUT:
			sp->xrepeat = 24;
			sp->yrepeat = 18;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			sp->clipdist <<= 2;
			break;
		case DRONE:
			sp->xrepeat = 14;
			sp->yrepeat = 7;
			sp->clipdist = 128;
			break;
		case SBSWIPE:
		case BILLYPLAY:
		case BILLYCOCK:
		case BILLYRAY:
		case BILLYRAYSTAYPUT:
		case BRAYSNIPER:
		case BUBBASTAND:
			sp->xrepeat = 25;
			sp->yrepeat = 21;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case COW:
			sp->xrepeat = 32;
			sp->yrepeat = 32;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case HEN:
		case HENSTAYPUT:
		case HENSTAND:
			if (sp->pal == 35)
			{
				sp->xrepeat = 42;
				sp->yrepeat = 30;
				sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			}
			else
			{
				sp->xrepeat = 21;
				sp->yrepeat = 15;
				sp->clipdist = 64;
			}
			break;
		case MINION:
		case MINIONSTAYPUT:
			sp->xrepeat = 16;
			sp->yrepeat = 16;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			if (isRRRA() && ufospawnsminion)
				sp->pal = 8;
			break;
		case DOGRUN:
		case PIG:
			sp->xrepeat = 16;
			sp->yrepeat = 16;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case RABBIT:
			sp->xrepeat = 18;
			sp->yrepeat = 18;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case MAMACLOUD:
			sp->xrepeat = 64;
			sp->yrepeat = 64;
			sp->cstat = 2;
			sp->cstat |= 512;
			sp->x += (krand() & 2047) - 1024;
			sp->y += (krand() & 2047) - 1024;
			sp->z += (krand() & 2047) - 1024;
			break;
		case MAMA:
			if (sp->pal == 30)
			{
				sp->xrepeat = 26;
				sp->yrepeat = 26;
				sp->clipdist = 75;
			}
			else if (sp->pal == 31)
			{
				sp->xrepeat = 36;
				sp->yrepeat = 36;
				sp->clipdist = 100;
			}
			else if (sp->pal == 32)
			{
				sp->xrepeat = 50;
				sp->yrepeat = 50;
				sp->clipdist = 100;
			}
			else
			{
				sp->xrepeat = 50;
				sp->yrepeat = 50;
				sp->clipdist = 100;
			}
			break;
		case BIKERB:
			sp->xrepeat = 28;
			sp->yrepeat = 22;
			sp->clipdist = 72;
			break;
		case BIKERBV2:
			sp->xrepeat = 28;
			sp->yrepeat = 22;
			sp->clipdist = 72;
			break;
		case BIKER:
			sp->xrepeat = 28;
			sp->yrepeat = 22;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case CHEERB:
			sp->xrepeat = 28;
			sp->yrepeat = 22;
			sp->clipdist = 72;
			break;
		case CHEER:
		case CHEERSTAYPUT:
			sp->xrepeat = 20;
			sp->yrepeat = 20;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case MAKEOUT:
			sp->xrepeat = 26;
			sp->yrepeat = 26;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case MINIONBOAT:
			sp->xrepeat = 16;
			sp->yrepeat = 16;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case HULKBOAT:
			sp->xrepeat = 48;
			sp->yrepeat = 48;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case CHEERBOAT:
			sp->xrepeat = 32;
			sp->yrepeat = 32;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;

		case TORNADO:
			sp->xrepeat = 64;
			sp->yrepeat = 128;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			sp->clipdist >>= 2;
			sp->cstat = 2;
			break;
		case LTH:
			sp->xrepeat = 24;
			sp->yrepeat = 22;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;
		case ROCK:
		case ROCK2:
			sp->xrepeat = 64;
			sp->yrepeat = 64;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;

		case UFO1_RRRA:
		case UFO1_RR:
		case UFO2:
		case UFO3:
		case UFO4:
		case UFO5:
			sp->xrepeat = 32;
			sp->yrepeat = 32;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			sp->extra = 50;
			break;
		case SBMOVE:
			sp->xrepeat = 48;
			sp->yrepeat = 48;
			sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
			break;

		default:
			break;
		}

		if (spj) sp->lotag = 0;

		if ((sp->lotag > ud.player_skill) || ud.monsters_off == 1)
		{
			sp->xrepeat = sp->yrepeat = 0;
			changeactorstat(act, STAT_MISC);
			break;
		}
		else
		{
			makeitfall(act);

			if (sp->picnum == RAT)
			{
				sp->ang = krand() & 2047;
				sp->xrepeat = sp->yrepeat = 48;
				sp->cstat = 0;
			}
			else
			{
				sp->cstat |= 257;

				if (sp->picnum != 5501)
					if (actorfella(act))
						ps[myconnectindex].max_actors_killed++;
			}

			if (spj)
			{
				act->timetosleep = 0;
				check_fta_sounds_r(act);
				changeactorstat(act, STAT_ACTOR);
				sp->shade = spj->shade;
			}
			else changeactorstat(act, STAT_ZOMBIEACTOR);

		}

		break;
	case LOCATORS:
		//                sp->xrepeat=sp->yrepeat=0;
		sp->cstat |= 32768;
		changeactorstat(act, STAT_LOCATOR);
		break;

	case ACTIVATORLOCKED:
	case ACTIVATOR:
		//                sp->xrepeat=sp->yrepeat=0;
		sp->cstat |= 32768;
		if (sp->picnum == ACTIVATORLOCKED)
			sectp->lotag ^= 16384;
		changeactorstat(act, STAT_ACTIVATOR);
		break;
	case DOORSHOCK:
		sp->cstat |= 1 + 256;
		sp->shade = -12;

		changeactorstat(act, STAT_STANDABLE);
		break;

	case OOZ:
	{
		sp->shade = -12;

		if (spj)
			if (spj->picnum == NUKEBARREL)
				sp->pal = 8;

		changeactorstat(act, STAT_STANDABLE);

		getglobalz(act);

		int j = (act->floorz - act->ceilingz) >> 9;

		sp->yrepeat = j;
		sp->xrepeat = 25 - (j >> 1);
		sp->cstat |= (krand() & 4);
		break;
	}
	case HEAVYHBOMB:
		act->SetOwner(act);
		sp->xrepeat = sp->yrepeat = 9;
		sp->yvel = 4;
		[[fallthrough]];
	case REACTOR2:
	case REACTOR:
	case RECON:
		if (initreactor(actj, act, sp->picnum == RECON)) return act;
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
	case TRIPBOMBSPRITE:
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
		if (spj)
		{
			sp->lotag = 0;
			if (sp->picnum != BOWLINGBALLSPRITE)
			{
				sp->z -= (32 << 8);
				sp->zvel = -(4 << 8);
			}
			else
			{
				sp->zvel = 0;
			}
			ssp(act, CLIPMASK0);
			sp->cstat = krand() & 4;
		}
		else
		{
			act->SetOwner(act);
			sp->cstat = 0;
		}

		if ((ud.multimode < 2 && sp->pal != 0) || (sp->lotag > ud.player_skill))
		{
			sp->xrepeat = sp->yrepeat = 0;
			changeactorstat(act, STAT_MISC);
			break;
		}

		sp->pal = 0;
		[[fallthrough]];

	case ACCESSCARD:

		if (sp->picnum == ATOMICHEALTH)
			sp->cstat |= 128;

		if (ud.multimode > 1 && ud.coop != 1 && sp->picnum == ACCESSCARD)
		{
			sp->xrepeat = sp->yrepeat = 0;
			changeactorstat(act, STAT_MISC);
			break;
		}
		else
		{
			if (sp->picnum == AMMO)
				sp->xrepeat = sp->yrepeat = 16;
			else sp->xrepeat = sp->yrepeat = 32;
		}

		sp->shade = -17;

		if (spj) changeactorstat(act, STAT_ACTOR);
		else
		{
			changeactorstat(act, STAT_ZOMBIEACTOR);
			makeitfall(act);
		}
		switch (sp->picnum)
		{
		case FIRSTGUNSPRITE:
			sp->xrepeat = 16;
			sp->yrepeat = 16;
			break;
		case SHOTGUNAMMO:
			sp->xrepeat = 18;
			sp->yrepeat = 17;
			if (isRRRA()) sp->cstat = 256;
			break;
		case SIXPAK:
			sp->xrepeat = 13;
			sp->yrepeat = 9;
			if (isRRRA()) sp->cstat = 256;
			break;
		case FIRSTAID:
			sp->xrepeat = 8;
			sp->yrepeat = 8;
			break;
		case BEER:
			sp->xrepeat = 5;
			sp->yrepeat = 4;
			break;
		case AMMO:
			sp->xrepeat = 9;
			sp->yrepeat = 9;
			break;
		case MOTOAMMO:
			if (!isRRRA()) goto default_case;
			sp->xrepeat = 23;
			sp->yrepeat = 23;
			break;
		case BOATAMMO:
			if (!isRRRA()) goto default_case;
			sp->xrepeat = 16;
			sp->yrepeat = 16;
			break;
		case COWPIE:
			sp->xrepeat = 8;
			sp->yrepeat = 6;
			break;
		case STEROIDS:
			sp->xrepeat = 13;
			sp->yrepeat = 9;
			break;
		case ACCESSCARD:
			sp->xrepeat = 11;
			sp->yrepeat = 12;
			break;
		case HEATSENSOR:
			sp->xrepeat = 6;
			sp->yrepeat = 4;
			break;
		case AIRTANK:
			sp->xrepeat = 19;
			sp->yrepeat = 16;
			break;
		case BATTERYAMMO:
			sp->xrepeat = 15;
			sp->yrepeat = 15;
			break;
		case BOWLINGBALLSPRITE:
			sp->xrepeat = 11;
			sp->yrepeat = 11;
			break;
		case TRIPBOMBSPRITE:
			sp->xrepeat = 11;
			sp->yrepeat = 11;
			sp->yvel = 4;
			sp->xvel = 32;
			break;
		case RPGSPRITE:
			sp->xrepeat = 16;
			sp->yrepeat = 14;
			break;
		case RPG2SPRITE:
			if (!isRRRA()) goto default_case;
			sp->xrepeat = 20;
			sp->yrepeat = 20;
			break;
		case SHRINKERSPRITE:
			sp->xrepeat = 22;
			sp->yrepeat = 13;
			break;
		case DEVISTATORSPRITE:
			sp->xrepeat = 18;
			sp->yrepeat = 17;
			break;
		case SAWAMMO:
			sp->xrepeat = 12;
			sp->yrepeat = 7;
			break;
		case GROWSPRITEICON:
			sp->xrepeat = 10;
			sp->yrepeat = 9;
			break;
		case DEVISTATORAMMO:
			sp->xrepeat = 10;
			sp->yrepeat = 9;
			break;
		case ATOMICHEALTH:
			sp->xrepeat = 8;
			sp->yrepeat = 8;
			break;
		case FREEZESPRITE:
			sp->xrepeat = 17;
			sp->yrepeat = 16;
			break;
		}
		sp->shade = sp->sector()->floorshade;
		break;
	case WATERFOUNTAIN:
		sp->lotag = 1;
		[[fallthrough]];
	case TREE1:
	case TREE2:
	case TIRE:
		sp->cstat = 257; // Make it hitable
		sp->extra = 1;
		changeactorstat(act, 6);
		break;

	case CAMERA1:
	case CAMERA1 + 1:
	case CAMERA1 + 2:
	case CAMERA1 + 3:
	case CAMERA1 + 4:
	case CAMERAPOLE:
		sp->extra = 1;

		if (gs.camerashitable) sp->cstat = 257;
		else sp->cstat = 0;

		if (ud.multimode < 2 && sp->pal != 0)
		{
			sp->xrepeat = sp->yrepeat = 0;
			changeactorstat(act, STAT_MISC);
			break;
		}
		else sp->pal = 0;
		if (sp->picnum == CAMERAPOLE) break;
		sp->picnum = CAMERA1;
		changeactorstat(act, 1);
		break;
	case STEAM:
		if (spj)
		{
			sp->ang = spj->ang;
			sp->cstat = 16 + 128 + 2;
			sp->xrepeat = sp->yrepeat = 1;
			sp->xvel = -8;
			ssp(act, CLIPMASK0);
		}
		[[fallthrough]];
	case CEILINGSTEAM:
		changeactorstat(act, STAT_STANDABLE);
		break;
	case SECTOREFFECTOR:
		spawneffector(act);
		break;

	case SEENINE:
	case OOZFILTER:

		sp->shade = -16;
		if (sp->xrepeat <= 8)
		{
			sp->cstat = 32768;
			sp->xrepeat = sp->yrepeat = 0;
		}
		else sp->cstat = 1 + 256;
		sp->extra = gs.impact_damage << 2;
		act->SetOwner(act);
		changeactorstat(act, STAT_STANDABLE);
		break;

	case CRACK1:
	case CRACK2:
	case CRACK3:
	case CRACK4:
		sp->cstat |= 17;
		sp->extra = 1;
		if (ud.multimode < 2 && sp->pal != 0)
		{
			sp->xrepeat = sp->yrepeat = 0;
			changeactorstat(act, STAT_MISC);
			break;
		}

		sp->pal = 0;
		act->SetOwner(act);
		changeactorstat(act, STAT_STANDABLE);
		sp->xvel = 8;
		ssp(act, CLIPMASK0);
		break;

	case EMPTYBIKE:
		if (!isRRRA()) goto default_case;
		if (ud.multimode < 2 && sp->pal == 1)
		{
			sp->xrepeat = sp->yrepeat = 0;
			break;
		}
		sp->pal = 0;
		sp->xrepeat = 18;
		sp->yrepeat = 18;
		sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
		act->saved_ammo = 100;
		sp->cstat = 257;
		sp->lotag = 1;
		changeactorstat(act, STAT_ACTOR);
		break;
	case EMPTYBOAT:
		if (!isRRRA()) goto default_case;
		if (ud.multimode < 2 && sp->pal == 1)
		{
			sp->xrepeat = sp->yrepeat = 0;
			break;
		}
		sp->pal = 0;
		sp->xrepeat = 32;
		sp->yrepeat = 32;
		sp->clipdist = MulScale(sp->xrepeat, tileWidth(sp->picnum), 7);
		act->saved_ammo = 20;
		sp->cstat = 257;
		sp->lotag = 1;
		changeactorstat(act, 1);
		break;

	case TOILET:
	case STALL:
	case RRTILE2121:
	case RRTILE2122:
		sp->lotag = 1;
		sp->cstat |= 257;
		sp->clipdist = 8;
		act->SetOwner(act);
		break;
	case CANWITHSOMETHING:
	case RUBBERCAN:
		sp->extra = 0;
		[[fallthrough]];
	case EXPLODINGBARREL:
	case HORSEONSIDE:
	case FIREBARREL:
	case NUKEBARREL:
	case FIREVASE:
	case NUKEBARRELDENTED:
	case NUKEBARRELLEAKED:
	case WOODENHORSE:

		if (spj)
			sp->xrepeat = sp->yrepeat = 32;
		sp->clipdist = 72;
		makeitfall(act);
		if (spj) act->SetOwner(actj);
		else act->SetOwner(act);
		[[fallthrough]];

	case EGG:
		if (ud.monsters_off == 1 && sp->picnum == EGG)
		{
			sp->xrepeat = sp->yrepeat = 0;
			changeactorstat(act, STAT_MISC);
		}
		else
		{
			if (sp->picnum == EGG)
				sp->clipdist = 24;
			sp->cstat = 257 | (krand() & 4);
			changeactorstat(act, STAT_ZOMBIEACTOR);
		}
		break;
	case TOILETWATER:
		sp->shade = -16;
		changeactorstat(act, STAT_STANDABLE);
		break;
	case RRTILE63:
		sp->cstat |= 32768;
		sp->xrepeat = 1;
		sp->yrepeat = 1;
		sp->clipdist = 1;
		changeactorstat(act, 100);
		break;
	}
	return act;
}

END_DUKE_NS
