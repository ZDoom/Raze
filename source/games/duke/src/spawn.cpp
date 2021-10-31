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
#include "automap.h"
#include "dukeactor.h"
#include "interpolate.h"

BEGIN_DUKE_NS


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DDukeActor* EGS(short whatsect, int s_x, int s_y, int s_z, short s_pn, signed char s_s, signed char s_xr, signed char s_yr, short s_a, short s_ve, int s_zv, DDukeActor* s_ow, signed char s_ss) 
{
	int const i = insertsprite(whatsect, s_ss);

	if (i < 0)
		I_Error(" Too many sprites spawned.");

	auto act = &hittype[i];
	auto s = act->s;

	s->x = s_x;
	s->y = s_y;
	s->z = s_z;
	s->cstat = 0;
	s->picnum = s_pn;
	s->shade = s_s;
	s->xrepeat = s_xr;
	s->yrepeat = s_yr;
	s->pal = 0;

	s->ang = s_a;
	s->xvel = s_ve;
	s->zvel = s_zv;
	s->xoffset = 0;
	s->yoffset = 0;
	s->yvel = 0;
	s->clipdist = 0;
	s->pal = 0;
	s->lotag = 0;
	s->backuploc();

	act->lastvx = 0;
	act->lastvy = 0;

	act->timetosleep = 0;
	act->actorstayput = -1;
	act->extra = -1;
	act->cgg = 0;
	act->movflag = 0;
	act->tempang = 0;
	act->dispicnum = 0;
	act->SetHitOwner(s_ow);
	act->SetOwner(s_ow);

	if (s_ow)
	{
		act->picnum = s_ow->s->picnum;
		act->floorz = s_ow->floorz;
		act->ceilingz = s_ow->ceilingz;
	}
	else
	{

	}

	memset(act->temp_data, 0, sizeof(act->temp_data));
	if (gs.actorinfo[s_pn].scriptaddress)
	{
		auto sa = &ScriptCode[gs.actorinfo[s_pn].scriptaddress];
		s->extra = sa[0];
		act->temp_data[4] = sa[1];
		act->temp_data[1] = sa[2];
		s->hitag = sa[3];
	}
	else
	{
		s->extra = 0;
		s->hitag = 0;
	}

	if (show2dsector[s->sectnum]) act->s->cstat2 |= CSTAT2_SPRITE_MAPPED;
	else act->s->cstat2 &= ~CSTAT2_SPRITE_MAPPED;

	spriteext[i] = {};
	spritesmooth[i] = {};

	return act;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int initspriteforspawn(DDukeActor* actj, int pn, const std::initializer_list<int> &excludes)
{
	spritetype* sp;
	int* t;
	int i;

	if (actj)
	{
		auto spawned = EGS(actj->s->sectnum, actj->s->x, actj->s->y, actj->s->z, pn, 0, 0, 0, 0, 0, 0, actj, 0);
		spawned->picnum = actj->s->picnum;
		sp = spawned->s;
		t = spawned->temp_data;
		i = spawned->GetSpriteIndex();
	}
	else
	{
		i = pn;
		auto act = &hittype[i];
		sp = act->s;
		t = act->temp_data;

		act->picnum = sp->picnum;
		act->timetosleep = 0;
		act->extra = -1;

		sp->backuppos();

		act->SetOwner(act);
		act->SetHitOwner(act);
		act->cgg = 0;
		act->movflag = 0;
		act->tempang = 0;
		act->dispicnum = 0;
		act->floorz = sector[sp->sectnum].floorz;
		act->ceilingz = sector[sp->sectnum].ceilingz;

		act->lastvx = 0;
		act->lastvy = 0;
		act->actorstayput = -1;

		t[0] = t[1] = t[2] = t[3] = t[4] = t[5] = 0;
		act->temp_actor = nullptr;

		if (sp->cstat & 48)
			if (!isIn(sp->picnum, excludes) && (sp->cstat & 48))
			{
				if (sp->shade == 127) return i;
				if (wallswitchcheck(act) && (sp->cstat & 16))
				{
					if (sp->picnum != TILE_ACCESSSWITCH && sp->picnum != TILE_ACCESSSWITCH2 && sp->pal)
					{
						if ((ud.multimode < 2) || (ud.multimode > 1 && ud.coop == 1))
						{
							sp->xrepeat = sp->yrepeat = 0;
							sp->cstat = sp->lotag = sp->hitag = 0;
							return i;
						}
					}
					sp->cstat |= 257;
					if (sp->pal && sp->picnum != TILE_ACCESSSWITCH && sp->picnum != TILE_ACCESSSWITCH2)
						sp->pal = 0;
					return i;
				}

				if (sp->hitag)
				{
					changespritestat(i, 12);
					sp->cstat |= 257;
					sp->extra = gs.impact_damage;
					return i;
				}
			}

		int s = sp->picnum;

		if (sp->cstat & 1) sp->cstat |= 256;

		if (gs.actorinfo[s].scriptaddress)
		{
			sp->extra = ScriptCode[gs.actorinfo[s].scriptaddress];
			t[4] = ScriptCode[gs.actorinfo[s].scriptaddress+1];
			t[1] = ScriptCode[gs.actorinfo[s].scriptaddress+2];
			int s3 = ScriptCode[gs.actorinfo[s].scriptaddress+3];
			if (s3 && sp->hitag == 0)
				sp->hitag = s3;
		}
		else t[1] = t[4] = 0;
	}
	return i | 0x1000000;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void spawninitdefault(DDukeActor* actj, DDukeActor *act)
{
	auto sp = act->s;
	auto sect = sp->sectnum;

	if (gs.actorinfo[sp->picnum].scriptaddress)
	{
		if (actj == nullptr && sp->lotag > ud.player_skill)
		{
			// make it go away...
			sp->xrepeat = sp->yrepeat = 0;
			changeactorstat(act, STAT_MISC);
			return;
		}

		//  Init the size
		if (sp->xrepeat == 0 || sp->yrepeat == 0)
			sp->xrepeat = sp->yrepeat = 1;

		if (actorflag(act, SFLAG_BADGUY))
		{
			if (ud.monsters_off == 1)
			{
				sp->xrepeat = sp->yrepeat = 0;
				changeactorstat(act, STAT_MISC);
				return;
			}

			makeitfall(act);

			if (actorflag(act, SFLAG_BADGUYSTAYPUT))
				act->actorstayput = sp->sectnum;

			if (!isRR() || actorflag(act, SFLAG_KILLCOUNT))	// Duke is just like Doom - Bad guys always count as kill.
				ps[myconnectindex].max_actors_killed++;

			sp->clipdist = 80;
			if (actj)
			{
				if (actj->s->picnum == RESPAWN)
					act->tempang = sp->pal = actj->s->pal;
				changeactorstat(act, STAT_ACTOR);
			}
			else changeactorstat(act, STAT_ZOMBIEACTOR);
		}
		else
		{
			sp->clipdist = 40;
			act->SetOwner(act);
			changeactorstat(act, STAT_ACTOR);
		}

		act->timetosleep = 0;

		if (actj)
			sp->ang = actj->s->ang;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void spawntransporter(DDukeActor *actj, DDukeActor* acti, bool beam)
{
	if (actj == nullptr) return;
	auto sp = acti->s;
	auto spj = actj->s;
	if (beam)
	{
		sp->xrepeat = 31;
		sp->yrepeat = 1;
		sp->z = sector[spj->sectnum].floorz - (isRR() ? PHEIGHT_RR : PHEIGHT_DUKE);
	}
	else
	{
		if (spj->statnum == 4)
		{
			sp->xrepeat = 8;
			sp->yrepeat = 8;
		}
		else
		{
			sp->xrepeat = 48;
			sp->yrepeat = 64;
			if (spj->statnum == 10 || badguy(spj))
				sp->z -= (32 << 8);
		}
	}

	sp->shade = -127;
	sp->cstat = 128 | 2;
	sp->ang = spj->ang;

	sp->xvel = 128;
	changeactorstat(acti, STAT_MISC);
	ssp(acti, CLIPMASK0);
	setsprite(acti, sp->x, sp->y, sp->z);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int spawnbloodpoolpart1(DDukeActor *actj, DDukeActor* acti)
{
	auto sp = acti->s;
	short s1 = sp->sectnum;

	updatesector(sp->x + 108, sp->y + 108, &s1);
	if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
	{
		updatesector(sp->x - 108, sp->y - 108, &s1);
		if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
		{
			updatesector(sp->x + 108, sp->y - 108, &s1);
			if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
			{
				updatesector(sp->x - 108, sp->y + 108, &s1);
				if (s1 >= 0 && sector[s1].floorz != sector[sp->sectnum].floorz)
				{
					sp->xrepeat = sp->yrepeat = 0; changeactorstat(acti, STAT_MISC); return true;
				}
			}
			else { sp->xrepeat = sp->yrepeat = 0; changeactorstat(acti, STAT_MISC); return true; }
		}
		else { sp->xrepeat = sp->yrepeat = 0; changeactorstat(acti, STAT_MISC); return true; }
	}
	else { sp->xrepeat = sp->yrepeat = 0; changeactorstat(acti, STAT_MISC); return true; }

	if (sector[sp->sectnum].lotag == 1)
	{
		changeactorstat(acti, STAT_MISC);
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void initfootprint(DDukeActor* actj, DDukeActor* acti)
{
	auto sp = acti->s;
	int sect = sp->sectnum;
	if (actj)
	{
		short s1;
		s1 = sp->sectnum;

		updatesector(sp->x + 84, sp->y + 84, &s1);
		if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
		{
			updatesector(sp->x - 84, sp->y - 84, &s1);
			if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
			{
				updatesector(sp->x + 84, sp->y - 84, &s1);
				if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
				{
					updatesector(sp->x - 84, sp->y + 84, &s1);
					if (s1 >= 0 && sector[s1].floorz != sector[sp->sectnum].floorz)
					{
						sp->xrepeat = sp->yrepeat = 0; changeactorstat(acti, STAT_MISC); return;
					}
				}
				else { sp->xrepeat = sp->yrepeat = 0; return; }
			}
			else { sp->xrepeat = sp->yrepeat = 0; return; }
		}
		else { sp->xrepeat = sp->yrepeat = 0; return; }

		sp->cstat = 32 + ((ps[actj->s->yvel].footprintcount & 1) << 2);
		sp->ang = actj->s->ang;
	}

	sp->z = sector[sect].floorz;
	if (sector[sect].lotag != 1 && sector[sect].lotag != 2)
		sp->xrepeat = sp->yrepeat = 32;

	insertspriteq(acti);
	changeactorstat(acti, STAT_MISC);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void initshell(DDukeActor* actj, DDukeActor* acti, bool isshell)
{
	auto sp = acti->s;
	int sect = sp->sectnum;
	auto t = acti->temp_data;
	if (actj)
	{
		auto spj = actj->s;
		short snum, a;

		if (spj->picnum == TILE_APLAYER)
		{
			snum = spj->yvel;
			a = ps[snum].angle.ang.asbuild() - (krand() & 63) + 8;  //Fine tune

			t[0] = krand() & 1;
			sp->z = (3 << 8) + ps[snum].pyoff + ps[snum].pos.z - (ps[snum].horizon.sum().asq16() >> 12) + (!isshell ? (3 << 8) : 0);
			sp->zvel = -(krand() & 255);
		}
		else
		{
			a = sp->ang;
			sp->z = spj->z - gs.playerheight + (3 << 8);
		}

		sp->x = spj->x + bcos(a, -7);
		sp->y = spj->y + bsin(a, -7);

		sp->shade = -8;

		if (isNamWW2GI())
		{
			// to the right, with feeling
			sp->ang = a + 512;
			sp->xvel = 30;
		}
		else
		{
			sp->ang = a - 512;
			sp->xvel = 20;
		}

		sp->xrepeat = sp->yrepeat = isRR() && isshell? 2 : 4;

		changeactorstat(acti, STAT_MISC);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void initcrane(DDukeActor* actj, DDukeActor* acti, int CRANEPOLE)
{
	auto sp = acti->s;
	int sect = sp->sectnum;
	auto t = acti->temp_data;
	sp->cstat |= 64 | 257;

	sp->picnum += 2;
	sp->z = sector[sect].ceilingz + (48 << 8);
	t[4] = tempwallptr;

	msx[tempwallptr] = sp->x;
	msy[tempwallptr] = sp->y;
	msx[tempwallptr + 2] = sp->z;

	DukeStatIterator it(STAT_DEFAULT);
	while (auto act = it.Next())
	{
		auto ss = act->s;
		if (ss->picnum == CRANEPOLE && sp->hitag == (ss->hitag))
		{
			msy[tempwallptr + 2] = ActorToScriptIndex(act);

			t[1] = ss->sectnum;

			ss->xrepeat = 48;
			ss->yrepeat = 128;

			msx[tempwallptr + 1] = ss->x;
			msy[tempwallptr + 1] = ss->y;

			ss->x = sp->x;
			ss->y = sp->y;
			ss->z = sp->z;
			ss->shade = sp->shade;

			setsprite(act, ss->pos);
			break;
		}
	}

	tempwallptr += 3;
	acti->SetOwner(nullptr);
	sp->extra = 8;
	changeactorstat(acti, STAT_STANDABLE);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void initwaterdrip(DDukeActor* actj, DDukeActor* actor)
{
	auto sp = actor->s;
	int sect = sp->sectnum;
	auto t = actor->temp_data;
	if (actj && (actj->s->statnum == 10 || actj->s->statnum == 1))
	{
		sp->shade = 32;
		if (actj->s->pal != 1)
		{
			sp->pal = 2;
			sp->z -= (18 << 8);
		}
		else sp->z -= (13 << 8);
		sp->ang = getangle(ps[connecthead].pos.x - sp->x, ps[connecthead].pos.y - sp->y);
		sp->xvel = 48 - (krand() & 31);
		ssp(actor, CLIPMASK0);
	}
	else if (!actj)
	{
		sp->z += (4 << 8);
		t[0] = sp->z;
		if (!isRR()) t[1] = krand() & 127;
	}
	sp->xrepeat = 24;
	sp->yrepeat = 24;
	changeactorstat(actor, STAT_STANDABLE);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int initreactor(DDukeActor* actj, DDukeActor* actor, bool isrecon)
{
	auto sp = actor->s;
	int sect = sp->sectnum;
	auto t = actor->temp_data;
	if (isrecon)
	{
		if (sp->lotag > ud.player_skill)
		{
			sp->xrepeat = sp->yrepeat = 0;
			changeactorstat(actor, STAT_MISC);
			return true;
		}
		if (!isRR() || actorflag(actor, SFLAG_KILLCOUNT))	// Duke is just like Doom - Bad guys always count as kill.
			ps[myconnectindex].max_actors_killed++;
		actor->temp_data[5] = 0;
		if (ud.monsters_off == 1)
		{
			sp->xrepeat = sp->yrepeat = 0;
			changeactorstat(actor, STAT_MISC);
			return false;
		}
		sp->extra = 130;
	}
	else
		sp->extra = gs.impact_damage;

	sp->cstat |= 257; // Make it hitable

	if (ud.multimode < 2 && sp->pal != 0)
	{
		sp->xrepeat = sp->yrepeat = 0;
		changeactorstat(actor, STAT_MISC);
		return false;
	}
	sp->pal = 0;
	sp->shade = -17;

	changeactorstat(actor, 2);
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void spawneffector(DDukeActor* actor)
{
	auto sp = actor->s;
	int sect = sp->sectnum;
	auto t = actor->temp_data;
	int startwall, endwall, x, y, d, s, clostest;

	sp->yvel = sector[sect].extra;
	sp->cstat |= 32768;
	sp->xrepeat = sp->yrepeat = 0;

	switch (sp->lotag)
	{
		case SE_28_LIGHTNING:
			if (!isRR()) t[5] = 65;// Delay for lightning
			break;
		case SE_7_TELEPORT: // Transporters!!!!
		case SE_23_ONE_WAY_TELEPORT:// XPTR END
			if (sp->lotag != SE_23_ONE_WAY_TELEPORT)
			{
				DukeLinearSpriteIterator it;
				while (auto act2 = it.Next())
					{
					if (act2->s->statnum < MAXSTATUS && act2->s->picnum == SECTOREFFECTOR && (act2->s->lotag == SE_7_TELEPORT || act2->s->lotag == SE_23_ONE_WAY_TELEPORT) && 
						actor != act2 && act2->s->hitag == sp->hitag)
					{
						actor->SetOwner(act2);
						break;
					}
			}
			}
			else actor->SetOwner(actor);

			t[4] = sector[sect].floorz == sp->z;
			sp->cstat = 0;
			changeactorstat(actor, STAT_TRANSPORT);
			return;
		case SE_1_PIVOT:
			actor->SetOwner(nullptr);
			t[0] = 1;
			break;
		case SE_18_INCREMENTAL_SECTOR_RISE_FALL:

			if (sp->ang == 512)
			{
				t[1] = sector[sect].ceilingz;
				if (sp->pal)
					sector[sect].ceilingz = sp->z;
			}
			else
			{
				t[1] = sector[sect].floorz;
				if (sp->pal)
					sector[sect].floorz = sp->z;
			}

			sp->hitag <<= 2;
			break;

		case SE_19_EXPLOSION_LOWERS_CEILING:
			actor->SetOwner(nullptr);
			break;
		case SE_25_PISTON: // Pistons
			if (!isRR())
			{
				t[3] = sector[sect].ceilingz;
				t[4] = 1;
			}
			else
				t[4] = sector[sect].ceilingz;

			sector[sect].ceilingz = sp->z;
			StartInterpolation(sect, Interp_Sect_Ceilingz);
			break;
		case SE_35:
			sector[sect].ceilingz = sp->z;
			break;
		case SE_27_DEMO_CAM:
			if (ud.recstat == 1)
			{
				sp->xrepeat = sp->yrepeat = 64;
				sp->cstat &= 32767;
			}
			break;
		case SE_47_LIGHT_SWITCH:
		case SE_48_LIGHT_SWITCH:
			if (!isRRRA()) break;
		case SE_12_LIGHT_SWITCH:

			t[1] = sector[sect].floorshade;
			t[2] = sector[sect].ceilingshade;
			break;

		case SE_13_EXPLOSIVE:
		{
			t[0] = sector[sect].ceilingz;
			t[1] = sector[sect].floorz;

			bool ceiling = (abs(t[0] - sp->z) < abs(t[1] - sp->z));
			actor->spriteextra = ceiling;

			if (sp->ang == 512)
			{
				if (ceiling)
					sector[sect].ceilingz = sp->z;
				else
					sector[sect].floorz = sp->z;
			}
			else
				sector[sect].ceilingz = sector[sect].floorz = sp->z;

			if (sector[sect].ceilingstat & 1)
			{
				sector[sect].ceilingstat ^= 1;
				t[3] = 1;

				if (!ceiling && sp->ang == 512)
				{
					sector[sect].ceilingstat ^= 1;
					t[3] = 0;
				}

				sector[sect].ceilingshade =
					sector[sect].floorshade;

				if (sp->ang == 512)
				{
					startwall = sector[sect].wallptr;
					endwall = startwall + sector[sect].wallnum;
					for (int j = startwall; j < endwall; j++)
					{
						int x = wall[j].nextsector;
						if (x >= 0)
							if (!(sector[x].ceilingstat & 1))
							{
								sector[sect].ceilingpicnum =
									sector[x].ceilingpicnum;
								sector[sect].ceilingshade =
									sector[x].ceilingshade;
								break; //Leave earily
							}
					}
				}
			}

			break;
		}
		case SE_17_WARP_ELEVATOR:
		{
			t[2] = sector[sect].floorz; //Stopping loc

			int j = nextsectorneighborz(sect, sector[sect].floorz, -1, -1);
			t[3] = sector[j].ceilingz;

			j = nextsectorneighborz(sect, sector[sect].ceilingz, 1, 1);
			t[4] = sector[j].floorz;

			if (numplayers < 2)
			{
				StartInterpolation(sect, Interp_Sect_Floorz);
				StartInterpolation(sect, Interp_Sect_Ceilingz);
			}

			break;
		}
		case 156:
			break;

		case 34:
			StartInterpolation(sect, Interp_Sect_FloorPanX);
			break;

		case SE_24_CONVEYOR:
			StartInterpolation(sect, Interp_Sect_FloorPanX);
			sp->yvel <<= 1;
		case SE_36_PROJ_SHOOTER:
			break;

		case SE_20_STRETCH_BRIDGE:
		{
			int q;

			startwall = sector[sect].wallptr;
			endwall = startwall + sector[sect].wallnum;

			//find the two most clostest wall x's and y's
			q = 0x7fffffff;

			for (s = startwall; s < endwall; s++)
			{
				x = wall[s].x;
				y = wall[s].y;

				d = FindDistance2D(sp->x - x, sp->y - y);
				if (d < q)
				{
					q = d;
					clostest = s;
				}
			}

			t[1] = clostest;

			q = 0x7fffffff;

			for (s = startwall; s < endwall; s++)
			{
				x = wall[s].x;
				y = wall[s].y;

				d = FindDistance2D(sp->x - x, sp->y - y);
				if (d < q && s != t[1])
				{
					q = d;
					clostest = s;
				}
			}

			t[2] = clostest;
			StartInterpolation(sect, Interp_Sect_FloorPanX);
			StartInterpolation(sect, Interp_Sect_FloorPanY);
			break;
		}


		case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:

			t[3] = sector[sect].floorshade;

			sector[sect].floorshade = sp->shade;
			sector[sect].ceilingshade = sp->shade;

			actor->palvals = (sector[sect].ceilingpal << 8) | sector[sect].floorpal;

			//fix all the walls;

			startwall = sector[sect].wallptr;
			endwall = startwall + sector[sect].wallnum;

			for (s = startwall; s < endwall; s++)
			{
				if (!(wall[s].hitag & 1))
					wall[s].shade = sp->shade;
				if ((wall[s].cstat & 2) && wall[s].nextwall >= 0)
					wall[wall[s].nextwall].shade = sp->shade;
			}
			break;

		case SE_31_FLOOR_RISE_FALL:
			t[1] = sector[sect].floorz;
			//	t[2] = sp->hitag;
			if (sp->ang != 1536) sector[sect].floorz = sp->z;

			startwall = sector[sect].wallptr;
			endwall = startwall + sector[sect].wallnum;

			for (s = startwall; s < endwall; s++)
				if (wall[s].hitag == 0) wall[s].hitag = 9999;

			StartInterpolation(sect, Interp_Sect_Floorz);

			break;
		case SE_32_CEILING_RISE_FALL:
			t[1] = sector[sect].ceilingz;
			t[2] = sp->hitag;
			if (sp->ang != 1536) sector[sect].ceilingz = sp->z;

			startwall = sector[sect].wallptr;
			endwall = startwall + sector[sect].wallnum;

			for (s = startwall; s < endwall; s++)
				if (wall[s].hitag == 0) wall[s].hitag = 9999;

			StartInterpolation(sect, Interp_Sect_Ceilingz);

			break;

		case SE_4_RANDOM_LIGHTS: //Flashing lights

			t[2] = sector[sect].floorshade;

			startwall = sector[sect].wallptr;
			endwall = startwall + sector[sect].wallnum;

			actor->palvals = (sector[sect].ceilingpal << 8) | sector[sect].floorpal;

			for (s = startwall; s < endwall; s++)
				if (wall[s].shade > t[3])
					t[3] = wall[s].shade;

			break;

		case SE_9_DOWN_OPEN_DOOR_LIGHTS:
			if (sector[sect].lotag &&
				labs(sector[sect].ceilingz - sp->z) > 1024)
				sector[sect].lotag |= 32768; //If its open
		case SE_8_UP_OPEN_DOOR_LIGHTS:
			//First, get the ceiling-floor shade

			t[0] = sector[sect].floorshade;
			t[1] = sector[sect].ceilingshade;

			startwall = sector[sect].wallptr;
			endwall = startwall + sector[sect].wallnum;

			for (s = startwall; s < endwall; s++)
				if (wall[s].shade > t[2])
					t[2] = wall[s].shade;

			t[3] = 1; //Take Out;

			break;

		case 88:
			//First, get the ceiling-floor shade
			if (!isRR()) break;

			t[0] = sector[sect].floorshade;
			t[1] = sector[sect].ceilingshade;

			startwall = sector[sect].wallptr;
			endwall = startwall + sector[sect].wallnum;

			for (s = startwall; s < endwall; s++)
				if (wall[s].shade > t[2])
					t[2] = wall[s].shade;

			t[3] = 1; //Take Out;
			break;

		case SE_11_SWINGING_DOOR://Pivitor rotater
			if (sp->ang > 1024) t[3] = 2;
			else t[3] = -2;
		case SE_0_ROTATING_SECTOR:
		case SE_2_EARTHQUAKE://Earthquakemakers
		case SE_5_BOSS://Boss Creature
		case SE_6_SUBWAY://Subway
		case SE_14_SUBWAY_CAR://Caboos
		case SE_15_SLIDING_DOOR://Subwaytype sliding door
		case SE_16_REACTOR://That rotating blocker reactor thing
		case SE_26://ESCELATOR
		case SE_30_TWO_WAY_TRAIN://No rotational subways

			if (sp->lotag == 0)
			{
				if (sector[sect].lotag == 30)
				{
					if (sp->pal) sp->clipdist = 1;
					else sp->clipdist = 0;
					t[3] = sector[sect].floorz;
					sector[sect].hitag = ActorToScriptIndex(actor);
				}

				DukeLinearSpriteIterator it;
				bool found = false;
				while (auto act2 = it.Next())
				{
					auto spr = act2->s;
					if (spr->statnum < MAXSTATUS)
						if (spr->picnum == SECTOREFFECTOR &&
							spr->lotag == SE_1_PIVOT &&
							spr->hitag == sp->hitag)
						{
							if (sp->ang == 512)
							{
								sp->x = spr->x;
								sp->y = spr->y;
							}
							found = true;
							actor->SetOwner(act2);
							break;
						}
				}
				if (!found)
				{
					sp->picnum = 0;
					sp->cstat2 = CSTAT2_SPRITE_NOFIND;
					sp->cstat = CSTAT_SPRITE_INVISIBLE;
					changeactorsect(actor, STAT_REMOVED);
					Printf("Found lonely Sector Effector (lotag 0) at (%d,%d)\n", sp->x, sp->y);
					return;
				}
			}

			startwall = sector[sect].wallptr;
			endwall = startwall + sector[sect].wallnum;

			t[1] = tempwallptr;
			for (s = startwall; s < endwall; s++)
			{
				msx[tempwallptr] = wall[s].x - sp->x;
				msy[tempwallptr] = wall[s].y - sp->y;
				tempwallptr++;
				if (tempwallptr > 2047)
				{
					I_Error("Too many moving sectors at (%d,%d).\n", wall[s].x, wall[s].y);
				}
			}
			if (sp->lotag == SE_30_TWO_WAY_TRAIN || sp->lotag == SE_6_SUBWAY || sp->lotag == SE_14_SUBWAY_CAR || sp->lotag == SE_5_BOSS)
			{

				startwall = sector[sect].wallptr;
				endwall = startwall + sector[sect].wallnum;

				if (sector[sect].hitag == -1)
					sp->extra = 0;
				else sp->extra = 1;

				sector[sect].hitag = ActorToScriptIndex(actor);

				int j = 0;

				for (s = startwall; s < endwall; s++)
				{
					if (wall[s].nextsector >= 0 &&
						sector[wall[s].nextsector].hitag == 0 &&
						(sector[wall[s].nextsector].lotag < 3 || (isRRRA() && sector[wall[s].nextsector].lotag == 160)))
					{
						s = wall[s].nextsector;
						j = 1;
						break;
					}
				}

				if (j == 0)
				{
					I_Error("Subway found no zero'd sectors with locators\nat (%d,%d).\n", sp->x, sp->y);
				}

				actor->SetOwner(nullptr);
				t[0] = s;

				if (sp->lotag != SE_30_TWO_WAY_TRAIN)
					t[3] = sp->hitag;
			}

			else if (sp->lotag == SE_16_REACTOR)
				t[3] = sector[sect].ceilingz;

			else if (sp->lotag == SE_26)
			{
				t[3] = sp->x;
				t[4] = sp->y;
				if (sp->shade == sector[sect].floorshade) //UP
					sp->zvel = -256;
				else
					sp->zvel = 256;

				sp->shade = 0;
			}
			else if (sp->lotag == SE_2_EARTHQUAKE)
			{
				t[5] = sector[sp->sectnum].floorheinum;
				sector[sp->sectnum].floorheinum = 0;
			}
	}

	switch (sp->lotag)
	{
		case SE_6_SUBWAY:
		case SE_14_SUBWAY_CAR:
		{
			int j = callsound(sect, actor);
			if (j == -1)
			{
				if (!isRR()) j = SUBWAY;	// Duke
				else if (sector[sp->sectnum].floorpal == 7) j = 456;
				else j = 75;
			}
			actor->lastvx = j;
		}
		case SE_30_TWO_WAY_TRAIN:
			if (numplayers > 1) break;
		case SE_0_ROTATING_SECTOR:
		case SE_1_PIVOT:
		case SE_5_BOSS:
		case SE_11_SWINGING_DOOR:
		case SE_15_SLIDING_DOOR:
		case SE_16_REACTOR:
		case SE_26:
			setsectinterpolate(actor->s->sectnum);
			break;

		case SE_29_WAVES:
			StartInterpolation(actor->s->sectnum, Interp_Sect_Floorheinum);
			StartInterpolation(actor->s->sectnum, Interp_Sect_Floorz);
			break;
	}

	if ((!isRR() && actor->s->lotag >= 40 && actor->s->lotag <= 45) ||
		(isRRRA() && actor->s->lotag >= 150 && actor->s->lotag <= 155))
		changeactorstat(actor, STAT_RAROR);
	else
		changeactorstat(actor, STAT_EFFECTOR);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void lotsofglass(DDukeActor *actor, int wallnum, int n)
{
	int j, xv, yv, z, x1, y1, a;
	short sect;
	auto sp = actor->s;

	sect = -1;

	if (wallnum < 0)
	{
		for (j = n - 1; j >= 0; j--)
		{
			a = sp->ang - 256 + (krand() & 511) + 1024;
			EGS(sp->sectnum, sp->x, sp->y, sp->z, TILE_GLASSPIECES + (j % 3), -32, 36, 36, a, 32 + (krand() & 63), 1024 - (krand() & 1023), &hittype[j], 5);
		}
		return;
	}

	j = n + 1;

	x1 = wall[wallnum].x;
	y1 = wall[wallnum].y;

	xv = wall[wall[wallnum].point2].x - x1;
	yv = wall[wall[wallnum].point2].y - y1;

	x1 -= Sgn(yv);
	y1 += Sgn(xv);

	xv /= j;
	yv /= j;

	for (j = n; j > 0; j--)
	{
		x1 += xv;
		y1 += yv;

		updatesector(x1, y1, &sect);
		if (sect >= 0)
		{
			z = sector[sect].floorz - (krand() & (abs(sector[sect].ceilingz - sector[sect].floorz)));
			if (z < -(32 << 8) || z >(32 << 8))
				z = sp->z - (32 << 8) + (krand() & ((64 << 8) - 1));
			a = sp->ang - 1024;
			EGS(sp->sectnum, x1, y1, z, TILE_GLASSPIECES + (j % 3), -32, 36, 36, a, 32 + (krand() & 63), -(krand() & 1023), actor, 5);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void spriteglass(DDukeActor* actor, int n)
{
	auto sp = actor->s;

	for (int j = n; j > 0; j--)
	{
		int a = krand() & 2047;
		int z = sp->z - ((krand() & 16) << 8);
		auto k = EGS(sp->sectnum, sp->x, sp->y, z, TILE_GLASSPIECES + (j % 3), krand() & 15, 36, 36, a, 32 + (krand() & 63), -512 - (krand() & 2047), actor, 5);
		k->s->pal = sp->pal;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ceilingglass(DDukeActor* actor, int sectnum, int n)
{
	int j, xv, yv, z, x1, y1;
	int a, s, startwall, endwall;
	auto sp = actor->s;

	startwall = sector[sectnum].wallptr;
	endwall = startwall + sector[sectnum].wallnum;

	for (s = startwall; s < (endwall - 1); s++)
	{
		x1 = wall[s].x;
		y1 = wall[s].y;

		xv = (wall[s + 1].x - x1) / (n + 1);
		yv = (wall[s + 1].y - y1) / (n + 1);

		for (j = n; j > 0; j--)
		{
			x1 += xv;
			y1 += yv;
			a = krand() & 2047;
			z = sector[sectnum].ceilingz + ((krand() & 15) << 8);
			EGS(sectnum, x1, y1, z, TILE_GLASSPIECES + (j % 3), -32, 36, 36, a, (krand() & 31), 0, actor, 5);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void lotsofcolourglass(DDukeActor* actor, int wallnum, int n)
{
	int j, xv, yv, z, x1, y1;
	short sect = -1;
	int a;;
	auto sp = actor->s;

	if (wallnum < 0)
	{
		for (j = n - 1; j >= 0; j--)
		{
			a = krand() & 2047;
			auto k = EGS(sp->sectnum, sp->x, sp->y, sp->z - (krand() & (63 << 8)), TILE_GLASSPIECES + (j % 3), -32, 36, 36, a, 32 + (krand() & 63), 1024 - (krand() & 2047), actor, 5);
			k->s->pal = krand() & 15;
		}
		return;
	}

	j = n + 1;
	x1 = wall[wallnum].x;
	y1 = wall[wallnum].y;

	xv = (wall[wall[wallnum].point2].x - wall[wallnum].x) / j;
	yv = (wall[wall[wallnum].point2].y - wall[wallnum].y) / j;

	for (j = n; j > 0; j--)
	{
		x1 += xv;
		y1 += yv;

		updatesector(x1, y1, &sect);
		z = sector[sect].floorz - (krand() & (abs(sector[sect].ceilingz - sector[sect].floorz)));
		if (z < -(32 << 8) || z >(32 << 8))
			z = sp->z - (32 << 8) + (krand() & ((64 << 8) - 1));
		a = sp->ang - 1024;
		auto k = EGS(sp->sectnum, x1, y1, z, TILE_GLASSPIECES + (j % 3), -32, 36, 36, a, 32 + (krand() & 63), -(krand() & 2047), actor, 5);
		k->s->pal = krand() & 7;
	}
}



END_DUKE_NS
