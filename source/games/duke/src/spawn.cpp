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
// this creates a new actor but does not run any init code on it
// direct calls should only be done for very simple things.
//
//---------------------------------------------------------------------------

DDukeActor* CreateActor(sectortype* whatsectp, const DVector3& pos, int s_pn, int8_t s_s, int8_t s_xr, int8_t s_yr, int s_a, int s_ve, int s_zv, DDukeActor* s_ow, int8_t s_ss)
{
	// sector pointer must be strictly validated here or the engine will crash.
	if (whatsectp == nullptr || !validSectorIndex(sectnum(whatsectp))) return nullptr;
	// spawning out of range sprites will also crash.
	if (s_pn < 0 || s_pn >= MAXTILES) return nullptr;
	auto act = static_cast<DDukeActor*>(::InsertActor(RUNTIME_CLASS(DDukeActor), whatsectp, s_ss));

	if (act == nullptr) return nullptr;
	SetupGameVarsForActor(act);


	act->spr.pos = pos;
	act->spr.cstat = 0;
	act->spr.picnum = s_pn;
	act->spr.shade = s_s;
	act->spr.xrepeat = s_xr;
	act->spr.yrepeat = s_yr;
	act->spr.pal = 0;

	act->set_int_ang(s_a);
	act->spr.xvel = s_ve;
	act->spr.zvel = s_zv;
	act->spr.xoffset = 0;
	act->spr.yoffset = 0;
	act->spr.yvel = 0;
	act->spr.clipdist = 0;
	act->spr.pal = 0;
	act->spr.lotag = 0;
	act->backuploc();

	act->ovel.X = 0;
	act->ovel.Y = 0;

	act->timetosleep = 0;
	act->actorstayput = nullptr;
	act->hitextra = -1;
	act->cgg = 0;
	act->movflag = 0;
	act->tempang = 0;
	act->dispicnum = 0;
	act->SetHitOwner(s_ow);
	act->SetOwner(s_ow);

	if (s_ow)
	{
		act->attackertype = s_ow->spr.picnum;
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
		act->spr.extra = sa[0];
		act->temp_data[4] = sa[1];
		act->temp_data[1] = sa[2];
		act->spr.hitag = sa[3];
	}
	else
	{
		act->spr.extra = 0;
		act->spr.hitag = 0;
	}

	if (show2dsector[act->sectno()]) act->spr.cstat2 |= CSTAT2_SPRITE_MAPPED;
	else act->spr.cstat2 &= ~CSTAT2_SPRITE_MAPPED;

	act->sprext = {};
	act->spsmooth = {};

	return act;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool initspriteforspawn(DDukeActor* act)
{
	SetupGameVarsForActor(act);
	act->attackertype = act->spr.picnum;
	act->timetosleep = 0;
	act->hitextra = -1;

	act->backuppos();

	act->SetOwner(act);
	act->SetHitOwner(act);
	act->cgg = 0;
	act->movflag = 0;
	act->tempang = 0;
	act->dispicnum = 0;
	act->floorz = act->sector()->floorz;
	act->ceilingz = act->sector()->ceilingz;

	act->ovel.X = 0;
	act->ovel.Y = 0;
	act->actorstayput = nullptr;

	act->temp_data[0] = act->temp_data[1] = act->temp_data[2] = act->temp_data[3] = act->temp_data[4] = act->temp_data[5] = 0;
	act->temp_actor = nullptr;

	if (wallswitchcheck(act) && (act->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
	{
		if (act->spr.picnum != TILE_ACCESSSWITCH && act->spr.picnum != TILE_ACCESSSWITCH2 && act->spr.pal)
		{
			if ((ud.multimode < 2) || (ud.multimode > 1 && ud.coop == 1))
			{
				act->spr.xrepeat = act->spr.yrepeat = 0;
				act->spr.cstat = 0;
				act->spr.lotag = act->spr.hitag = 0;
				return false;
			}
		}
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		if (act->spr.pal && act->spr.picnum != TILE_ACCESSSWITCH && act->spr.picnum != TILE_ACCESSSWITCH2)
			act->spr.pal = 0;
		return false;
	}

	if (!actorflag(act, SFLAG_NOFALLER) && (act->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK))
	{
		if (act->spr.shade == 127) return false;

		if (act->spr.hitag)
		{
			ChangeActorStat(act, STAT_FALLER);
			act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
			act->spr.extra = gs.impact_damage;
			return false;
		}
	}

	int s = act->spr.picnum;

	if (act->spr.cstat & CSTAT_SPRITE_BLOCK) act->spr.cstat |= CSTAT_SPRITE_BLOCK_HITSCAN;

	if (gs.actorinfo[s].scriptaddress)
	{
		act->spr.extra = ScriptCode[gs.actorinfo[s].scriptaddress];
		act->temp_data[4] = ScriptCode[gs.actorinfo[s].scriptaddress+1];
		act->temp_data[1] = ScriptCode[gs.actorinfo[s].scriptaddress+2];
		int s3 = ScriptCode[gs.actorinfo[s].scriptaddress+3];
		if (s3 && act->spr.hitag == 0)
			act->spr.hitag = s3;
	}
	else act->temp_data[1] = act->temp_data[4] = 0;
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DDukeActor* spawn(DDukeActor* actj, int pn)
{
	if (actj)
	{
		auto spawned = EGS(actj->sector(), actj->int_pos().X, actj->int_pos().Y, actj->int_pos().Z, pn, 0, 0, 0, 0, 0, 0, actj, 0);
		if (spawned)
		{
			spawned->attackertype = actj->spr.picnum;
			return fi.spawninit(actj, spawned, nullptr);
		}
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void spawninitdefault(DDukeActor* actj, DDukeActor *act)
{
	if (gs.actorinfo[act->spr.picnum].scriptaddress)
	{
		if (actj == nullptr && act->spr.lotag > ud.player_skill)
		{
			// make it go away...
			act->spr.xrepeat = act->spr.yrepeat = 0;
			ChangeActorStat(act, STAT_MISC);
			return;
		}

		//  Init the size
		if (act->spr.xrepeat == 0 || act->spr.yrepeat == 0)
			act->spr.xrepeat = act->spr.yrepeat = 1;

		if (actorflag(act, SFLAG_BADGUY))
		{
			if (ud.monsters_off == 1)
			{
				act->spr.xrepeat = act->spr.yrepeat = 0;
				ChangeActorStat(act, STAT_MISC);
				return;
			}

			makeitfall(act);

			if (actorflag(act, SFLAG_BADGUYSTAYPUT))
				act->actorstayput = act->sector();

			if (!isRR() || actorflag(act, SFLAG_KILLCOUNT))	// Duke is just like Doom - Bad guys always count as kill.
				ps[myconnectindex].max_actors_killed++;

			act->spr.clipdist = 80;
			if (actj)
			{
				if (actj->spr.picnum == RESPAWN)
					act->tempang = act->spr.pal = actj->spr.pal;
				ChangeActorStat(act, STAT_ACTOR);
			}
			else ChangeActorStat(act, STAT_ZOMBIEACTOR);
		}
		else
		{
			act->spr.clipdist = 40;
			act->SetOwner(act);
			ChangeActorStat(act, STAT_ACTOR);
		}

		act->timetosleep = 0;

		if (actj)
			act->spr.angle = actj->spr.angle;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void spawntransporter(DDukeActor *actj, DDukeActor* act, bool beam)
{
	if (actj == nullptr) return;
	if (beam)
	{
		act->spr.xrepeat = 31;
		act->spr.yrepeat = 1;
		act->spr.pos.Z = actj->sector()->floorz - gs.playerheight;
	}
	else
	{
		if (actj->spr.statnum == 4)
		{
			act->spr.xrepeat = 8;
			act->spr.yrepeat = 8;
		}
		else
		{
			act->spr.xrepeat = 48;
			act->spr.yrepeat = 64;
			if (actj->spr.statnum == 10 || badguy(actj))
				act->spr.pos.Z -= 32;
		}
	}

	act->spr.shade = -127;
	act->spr.cstat = CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
	act->spr.angle = actj->spr.angle;

	act->spr.xvel = 128;
	ChangeActorStat(act, STAT_MISC);
	ssp(act, CLIPMASK0);
	SetActor(act, act->spr.pos);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int spawnbloodpoolpart1(DDukeActor* act)
{
	auto s1 = act->sector();

	updatesector(act->int_pos().X + 108, act->int_pos().Y + 108, &s1);
	if (s1 && s1->floorz == act->sector()->floorz)
	{
		updatesector(act->int_pos().X - 108, act->int_pos().Y - 108, &s1);
		if (s1 && s1->floorz == act->sector()->floorz)
		{
			updatesector(act->int_pos().X + 108, act->int_pos().Y - 108, &s1);
			if (s1 && s1->floorz == act->sector()->floorz)
			{
				updatesector(act->int_pos().X - 108, act->int_pos().Y + 108, &s1);
				if (s1 && s1->floorz != act->sector()->floorz)
				{
					act->spr.xrepeat = act->spr.yrepeat = 0; ChangeActorStat(act, STAT_MISC); return true;
				}
			}
			else { act->spr.xrepeat = act->spr.yrepeat = 0; ChangeActorStat(act, STAT_MISC); return true; }
		}
		else { act->spr.xrepeat = act->spr.yrepeat = 0; ChangeActorStat(act, STAT_MISC); return true; }
	}
	else { act->spr.xrepeat = act->spr.yrepeat = 0; ChangeActorStat(act, STAT_MISC); return true; }

	if (act->sector()->lotag == 1)
	{
		ChangeActorStat(act, STAT_MISC);
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void initfootprint(DDukeActor* actj, DDukeActor* act)
{
	auto sect = act->sector();
	if (actj)
	{
		auto s1 = act->sector();

		updatesector(act->int_pos().X + 84, act->int_pos().Y + 84, &s1);
		if (s1 && s1->floorz == act->sector()->floorz)
		{
			updatesector(act->int_pos().X - 84, act->int_pos().Y - 84, &s1);
			if (s1 && s1->floorz == act->sector()->floorz)
			{
				updatesector(act->int_pos().X + 84, act->int_pos().Y - 84, &s1);
				if (s1 && s1->floorz == act->sector()->floorz)
				{
					updatesector(act->int_pos().X - 84, act->int_pos().Y + 84, &s1);
					if (s1 && s1->floorz != act->sector()->floorz)
					{
						act->spr.xrepeat = act->spr.yrepeat = 0; ChangeActorStat(act, STAT_MISC); return;
					}
				}
				else { act->spr.xrepeat = act->spr.yrepeat = 0; return; }
			}
			else { act->spr.xrepeat = act->spr.yrepeat = 0; return; }
		}
		else { act->spr.xrepeat = act->spr.yrepeat = 0; return; }

		act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_FLOOR;
		if ((ps[actj->spr.yvel].footprintcount & 1)) act->spr.cstat |= CSTAT_SPRITE_XFLIP;
		act->spr.angle = actj->spr.angle;
	}

	act->set_int_z(sect->int_floorz());
	if (sect->lotag != 1 && sect->lotag != 2)
		act->spr.xrepeat = act->spr.yrepeat = 32;

	insertspriteq(act);
	ChangeActorStat(act, STAT_MISC);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void initshell(DDukeActor* actj, DDukeActor* act, bool isshell)
{
	if (actj)
	{
		int snum, a;

		if (actj->isPlayer())
		{
			snum = actj->spr.yvel;
			a = ps[snum].angle.ang.Buildang() - (krand() & 63) + 8;  //Fine tune

			act->temp_data[0] = krand() & 1;
			act->set_int_z((3 << 8) + ps[snum].pyoff + ps[snum].player_int_pos().Z - (ps[snum].horizon.sum().asq16() >> 12) + (!isshell ? (3 << 8) : 0));
			act->spr.zvel = -(krand() & 255);
		}
		else
		{
			a = act->int_ang();
			act->spr.pos.Z = actj->spr.pos.Z - gs.playerheight + 3;
		}

		act->set_int_xy(actj->int_pos().X + bcos(a, -7), actj->int_pos().Y + bsin(a, -7));

		act->spr.shade = -8;

		if (isNamWW2GI())
		{
			// to the right, with feeling
			act->set_int_ang(a + 512);
			act->spr.xvel = 30;
		}
		else
		{
			act->set_int_ang(a - 512);
			act->spr.xvel = 20;
		}

		act->spr.xrepeat = act->spr.yrepeat = isRR() && isshell? 2 : 4;

		ChangeActorStat(act, STAT_MISC);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void initcrane(DDukeActor* actj, DDukeActor* act, int CRANEPOLE)
{
	auto sect = act->sector();
	act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL | CSTAT_SPRITE_ONE_SIDE;

	act->spr.picnum += 2;
	act->spr.pos.Z = sect->ceilingz + 48;
	act->temp_data[4] = cranes.Reserve(1);

	auto& apt = cranes[act->temp_data[4]];
	apt.pos = act->spr.pos;
	apt.poleactor = nullptr;

	DukeStatIterator it(STAT_DEFAULT);
	while (auto actk = it.Next())
	{
		if (actk->spr.picnum == CRANEPOLE && act->spr.hitag == actk->spr.hitag)
		{
			apt.poleactor = actk;

			act->temp_sect = actk->sector();

			actk->spr.xrepeat = 48;
			actk->spr.yrepeat = 128;

			apt.pole = actk->spr.pos.XY();

			actk->spr.pos = act->spr.pos;
			actk->spr.shade = act->spr.shade;

			SetActor(actk, actk->spr.pos);
			break;
		}
	}

	act->SetOwner(nullptr);
	act->spr.extra = 8;
	ChangeActorStat(act, STAT_STANDABLE);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void initwaterdrip(DDukeActor* actj, DDukeActor* actor)
{
	if (actj && (actj->spr.statnum == 10 || actj->spr.statnum == 1))
	{
		actor->spr.shade = 32;
		if (actj->spr.pal != 1)
		{
			actor->spr.pal = 2;
			actor->spr.pos.Z -= 18;
		}
		else actor->spr.pos.Z -= 13;
		actor->spr.angle = VecToAngle(ps[connecthead].pos.XY() - actor->spr.pos.XY());
		actor->spr.xvel = 48 - (krand() & 31);
		ssp(actor, CLIPMASK0);
	}
	else if (!actj)
	{
		actor->spr.pos.Z += 4;
		actor->temp_data[0] = actor->int_pos().Z;
		if (!isRR()) actor->temp_data[1] = krand() & 127;
	}
	actor->spr.xrepeat = 24;
	actor->spr.yrepeat = 24;
	ChangeActorStat(actor, STAT_STANDABLE);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int initreactor(DDukeActor* actj, DDukeActor* actor, bool isrecon)
{
	if (isrecon)
	{
		if (actor->spr.lotag > ud.player_skill)
		{
			actor->spr.xrepeat = actor->spr.yrepeat = 0;
			ChangeActorStat(actor, STAT_MISC);
			return true;
		}
		if (!isRR() || actorflag(actor, SFLAG_KILLCOUNT))	// Duke is just like Doom - Bad guys always count as kill.
			ps[myconnectindex].max_actors_killed++;
		actor->temp_data[5] = 0;
		if (ud.monsters_off == 1)
		{
			actor->spr.xrepeat = actor->spr.yrepeat = 0;
			ChangeActorStat(actor, STAT_MISC);
			return false;
		}
		actor->spr.extra = 130;
	}
	else
		actor->spr.extra = gs.impact_damage;

	actor->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL; // Make it hitable

	if (ud.multimode < 2 && actor->spr.pal != 0)
	{
		actor->spr.xrepeat = actor->spr.yrepeat = 0;
		ChangeActorStat(actor, STAT_MISC);
		return false;
	}
	actor->spr.pal = 0;
	actor->spr.shade = -17;

	ChangeActorStat(actor, 2);
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void spawneffector(DDukeActor* actor, TArray<DDukeActor*>* actors)
{
	auto sectp = actor->sector();
	int d, clostest = 0;

	actor->spr.yvel = sectp->extra;
	actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
	actor->spr.xrepeat = actor->spr.yrepeat = 0;

	switch (actor->spr.lotag)
	{
		case SE_28_LIGHTNING:
			if (!isRR()) actor->temp_data[5] = 65;// Delay for lightning
			break;
		case SE_7_TELEPORT: // Transporters!!!!
		case SE_23_ONE_WAY_TELEPORT:// XPTR END
			if (actor->spr.lotag != SE_23_ONE_WAY_TELEPORT && actors)
			{

				for(auto act2 : *actors)
				{
					if (act2->spr.statnum < MAXSTATUS && act2->spr.picnum == SECTOREFFECTOR && (act2->spr.lotag == SE_7_TELEPORT || act2->spr.lotag == SE_23_ONE_WAY_TELEPORT) && 
						actor != act2 && act2->spr.hitag == actor->spr.hitag)
					{
						actor->SetOwner(act2);
						break;
					}
				}
			}
			else actor->SetOwner(actor);

			actor->temp_data[4] = sectp->floorz == actor->spr.pos.Z;
			actor->spr.cstat = 0;
			ChangeActorStat(actor, STAT_TRANSPORT);
			return;
		case SE_1_PIVOT:
			actor->SetOwner(nullptr);
			actor->temp_data[0] = 1;
			break;
		case SE_18_INCREMENTAL_SECTOR_RISE_FALL:

			if (actor->spr.intangle == 512)
			{
				actor->temp_data[1] = FloatToFixed<8>(sectp->ceilingz);
				if (actor->spr.pal)
					sectp->setceilingz(actor->spr.pos.Z);
			}
			else
			{
				actor->temp_data[1] = FloatToFixed<8>(sectp->floorz);
				if (actor->spr.pal)
					sectp->setfloorz(actor->spr.pos.Z);
			}

			actor->spr.hitag <<= 2;
			break;

		case SE_19_EXPLOSION_LOWERS_CEILING:
			actor->SetOwner(nullptr);
			break;
		case SE_25_PISTON: // Pistons
			if (!isRR())
			{
				actor->temp_data[3] = sectp->int_ceilingz();
				actor->temp_data[4] = 1;
			}
			else
				actor->temp_data[4] = sectp->int_ceilingz();

			sectp->setceilingz(actor->spr.pos.Z);
			StartInterpolation(sectp, Interp_Sect_Ceilingz);
			break;
		case SE_35:
			sectp->setceilingz(actor->spr.pos.Z);
			break;
		case SE_27_DEMO_CAM:
			if (ud.recstat == 1)
			{
				actor->spr.xrepeat = actor->spr.yrepeat = 64;
				actor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
			}
			break;
		case SE_47_LIGHT_SWITCH:
		case SE_48_LIGHT_SWITCH:
			if (!isRRRA()) break;
			[[fallthrough]];
		case SE_12_LIGHT_SWITCH:

			actor->temp_data[1] = sectp->floorshade;
			actor->temp_data[2] = sectp->ceilingshade;
			break;

		case SE_13_EXPLOSIVE:
		{
			actor->temp_data[0] = sectp->int_ceilingz();
			actor->temp_data[1] = sectp->int_floorz();

			bool ceiling = (abs(actor->temp_data[0] - actor->int_pos().Z) < abs(actor->temp_data[1] - actor->int_pos().Z));
			actor->spriteextra = ceiling;

			if (actor->int_ang() == 512)
			{
				if (ceiling)
					sectp->setceilingz(actor->spr.pos.Z);
				else
					sectp->setfloorz(actor->spr.pos.Z);
			}
			else
			{
				sectp->setceilingz(actor->spr.pos.Z);
				sectp->setfloorz(actor->spr.pos.Z);
			}

			if (sectp->ceilingstat & CSTAT_SECTOR_SKY)
			{
				sectp->ceilingstat ^= CSTAT_SECTOR_SKY;
				actor->temp_data[3] = 1;

				if (!ceiling && actor->int_ang() == 512)
				{
					sectp->ceilingstat ^= CSTAT_SECTOR_SKY;
					actor->temp_data[3] = 0;
				}

				sectp->ceilingshade =
					sectp->floorshade;

				if (actor->int_ang() == 512)
				{
					for (auto& wl : wallsofsector(sectp))
					{
						if (wl.twoSided())
						{
							auto nsec = wl.nextSector();
							if (!(nsec->ceilingstat & CSTAT_SECTOR_SKY))
							{
								sectp->ceilingpicnum = nsec->ceilingpicnum;
								sectp->ceilingshade = nsec->ceilingshade;
								break; //Leave early
							}
						}
					}
				}
			}

			break;
		}
		case SE_17_WARP_ELEVATOR:
		{
			actor->temp_data[2] = sectp->int_floorz(); //Stopping loc
			actor->temp_data[3] = nextsectorneighborzptr(sectp, sectp->int_floorz(), Find_CeilingUp | Find_Safe)->int_ceilingz();
			actor->temp_data[4] = nextsectorneighborzptr(sectp, sectp->int_ceilingz(), Find_FloorDown | Find_Safe)->int_floorz();

			if (numplayers < 2)
			{
				StartInterpolation(sectp, Interp_Sect_Floorz);
				StartInterpolation(sectp, Interp_Sect_Ceilingz);
			}

			break;
		}
		case 156:
			break;

		case 34:
			StartInterpolation(sectp, Interp_Sect_FloorPanX);
			break;

		case SE_24_CONVEYOR:
			StartInterpolation(sectp, Interp_Sect_FloorPanX);
			actor->spr.yvel <<= 1;
		case SE_36_PROJ_SHOOTER:
			break;

		case SE_20_STRETCH_BRIDGE:
		{
			int q;
			walltype* closewall = nullptr;

			//find the two most clostest wall x's and y's
			q = 0x7fffffff;

			for (auto& wal : wallsofsector(sectp))
			{
				d = FindDistance2D(actor->int_pos().vec2 - wal.wall_int_pos());
				if (d < q)
				{
					q = d;
					closewall = &wal;
				}
			}

			actor->temp_walls[0] = closewall;

			q = 0x7fffffff;

			for (auto& wal : wallsofsector(sectp))
			{
				d = FindDistance2D(actor->int_pos().vec2 - wal.wall_int_pos());
				if (d < q && &wal != actor->temp_walls[0])
				{
					q = d;
					closewall = &wal;
				}
			}

			actor->temp_walls[1] = closewall;
			StartInterpolation(sectp, Interp_Sect_FloorPanX);
			StartInterpolation(sectp, Interp_Sect_FloorPanY);
			break;
		}


		case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:

			actor->temp_data[3] = sectp->floorshade;

			sectp->floorshade = actor->spr.shade;
			sectp->ceilingshade = actor->spr.shade;

			actor->palvals = (sectp->ceilingpal << 8) | sectp->floorpal;

			//fix all the walls;

			for (auto& wal : wallsofsector(sectp))
			{
				if (!(wal.hitag & 1))
					wal.shade = actor->spr.shade;
				if ((wal.cstat & CSTAT_WALL_BOTTOM_SWAP) && wal.twoSided())
					wal.nextWall()->shade = actor->spr.shade;
			}
			break;

		case SE_31_FLOOR_RISE_FALL:
			actor->temp_data[1] = sectp->int_floorz();
			//	actor->temp_data[2] = actor->spr.hitag;
			if (actor->int_ang() != 1536) sectp->setfloorz(actor->spr.pos.Z);

			for (auto& wal : wallsofsector(sectp))
				if (wal.hitag == 0) wal.hitag = 9999;

			StartInterpolation(sectp, Interp_Sect_Floorz);

			break;
		case SE_32_CEILING_RISE_FALL:
			actor->temp_data[1] = sectp->int_ceilingz();
			actor->temp_data[2] = actor->spr.hitag;
			if (actor->int_ang() != 1536) sectp->setceilingz(actor->spr.pos.Z);

			for (auto& wal : wallsofsector(sectp))
				if (wal.hitag == 0) wal.hitag = 9999;

			StartInterpolation(sectp, Interp_Sect_Ceilingz);

			break;

		case SE_4_RANDOM_LIGHTS: //Flashing lights

			actor->temp_data[2] = sectp->floorshade;

			actor->palvals = (sectp->ceilingpal << 8) | sectp->floorpal;

			for (auto& wal : wallsofsector(sectp))
				if (wal.shade > actor->temp_data[3])
					actor->temp_data[3] = wal.shade;

			break;

		case SE_9_DOWN_OPEN_DOOR_LIGHTS:
			if (sectp->lotag &&
				labs(sectp->int_ceilingz() - actor->int_pos().Z) > 1024)
				sectp->lotag |= 32768; //If its open
			[[fallthrough]];
		case SE_8_UP_OPEN_DOOR_LIGHTS:
			//First, get the ceiling-floor shade

			actor->temp_data[0] = sectp->floorshade;
			actor->temp_data[1] = sectp->ceilingshade;

			for (auto& wal : wallsofsector(sectp))
				if (wal.shade > actor->temp_data[2])
					actor->temp_data[2] = wal.shade;

			actor->temp_data[3] = 1; //Take Out;

			break;

		case 88:
			//First, get the ceiling-floor shade
			if (!isRR()) break;

			actor->temp_data[0] = sectp->floorshade;
			actor->temp_data[1] = sectp->ceilingshade;

			for (auto& wal : wallsofsector(sectp))
				if (wal.shade > actor->temp_data[2])
					actor->temp_data[2] = wal.shade;

			actor->temp_data[3] = 1; //Take Out;
			break;

		case SE_11_SWINGING_DOOR://Pivitor rotater
			if (actor->int_ang() > 1024) actor->temp_data[3] = 2;
			else actor->temp_data[3] = -2;
			[[fallthrough]];
		case SE_0_ROTATING_SECTOR:
		case SE_2_EARTHQUAKE://Earthquakemakers
		case SE_5_BOSS://Boss Creature
		case SE_6_SUBWAY://Subway
		case SE_14_SUBWAY_CAR://Caboos
		case SE_15_SLIDING_DOOR://Subwaytype sliding door
		case SE_16_REACTOR://That rotating blocker reactor thing
		case SE_26://ESCELATOR
		case SE_30_TWO_WAY_TRAIN://No rotational subways
		{
			if (actor->spr.lotag == 0)
			{
				if (sectp->lotag == 30)
				{
					if (actor->spr.pal) actor->spr.clipdist = 1;
					else actor->spr.clipdist = 0;
					actor->temp_data[3] = sectp->int_floorz();
					sectp->hitagactor = actor;
				}


				bool found = false;
				if (actors) for (auto act2 : *actors)
				{
					if (act2->spr.statnum < MAXSTATUS)
						if (act2->spr.picnum == SECTOREFFECTOR &&
							act2->spr.lotag == SE_1_PIVOT &&
							act2->spr.hitag == actor->spr.hitag)
						{
							if (actor->int_ang() == 512)
							{
								actor->spr.pos.XY() = act2->spr.pos.XY();
							}
							found = true;
							actor->SetOwner(act2);
							break;
						}
				}
				if (!found)
				{
					actor->spr.picnum = 0;
					actor->spr.cstat2 = CSTAT2_SPRITE_NOFIND;
					actor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
					ChangeActorStat(actor, STAT_REMOVED);
					Printf("Found lonely Sector Effector (lotag 0) at (%d,%d)\n", actor->int_pos().X, actor->int_pos().Y);
					return;
				}
			}

			actor->temp_data[1] = tempwallptr;
			for (auto& wal : wallsofsector(sectp))
			{
				msx[tempwallptr] = wal.wall_int_pos().X - actor->int_pos().X;
				msy[tempwallptr] = wal.wall_int_pos().Y - actor->int_pos().Y;
				tempwallptr++;
				if (tempwallptr > 2047)
				{
					I_Error("Too many moving sectors at (%d,%d).\n", int(wal.pos.X), int(wal.pos.Y));
				}
			}
			if (actor->spr.lotag == SE_30_TWO_WAY_TRAIN || actor->spr.lotag == SE_6_SUBWAY || actor->spr.lotag == SE_14_SUBWAY_CAR || actor->spr.lotag == SE_5_BOSS)
			{

				if (sectp->hitag == -1)
					actor->spr.extra = 0;
				else actor->spr.extra = 1;

				sectp->hitagactor = actor;

				sectortype* s = nullptr;
				for (auto& wal : wallsofsector(sectp))
				{
					if (wal.twoSided() &&
						wal.nextSector()->hitag == 0 &&
						(wal.nextSector()->lotag < 3 || (isRRRA() && wal.nextSector()->lotag == 160)))
					{
						s = wal.nextSector();
						break;
					}
				}

				if (s == nullptr)
				{
					I_Error("Subway found no zero'd sectors with locators\nat (%d,%d).\n", int(actor->spr.pos.X), int(actor->spr.pos.Y));
				}

				actor->SetOwner(nullptr);
				actor->temp_data[0] = sectnum(s);

				if (actor->spr.lotag != SE_30_TWO_WAY_TRAIN)
					actor->temp_data[3] = actor->spr.hitag;
			}

			else if (actor->spr.lotag == SE_16_REACTOR)
				actor->temp_data[3] = sectp->int_ceilingz();

			else if (actor->spr.lotag == SE_26)
			{
				actor->temp_data[3] = actor->int_pos().X;
				actor->temp_data[4] = actor->int_pos().Y;
				if (actor->spr.shade == sectp->floorshade) //UP
					actor->spr.zvel = -256;
				else
					actor->spr.zvel = 256;

				actor->spr.shade = 0;
			}
			else if (actor->spr.lotag == SE_2_EARTHQUAKE)
			{
				actor->temp_data[5] = actor->sector()->getfloorslope();
				actor->sector()->setfloorslope(0);
			}
			break;
		}
		case SE_49_POINT_LIGHT:
		case SE_50_SPOT_LIGHT:
		{
			DukeSectIterator it(actor->sector());
			while (auto itActor = it.Next())
			{
				if (itActor->spr.picnum == ACTIVATOR || itActor->spr.picnum == ACTIVATORLOCKED)
					actor->flags2 |= SFLAG2_USEACTIVATOR;
			}
			ChangeActorStat(actor, STAT_LIGHT);
			break;
		}

	}

	switch (actor->spr.lotag)
	{
		case SE_6_SUBWAY:
		case SE_14_SUBWAY_CAR:
		{
			int j = callsound(sectp, actor);
			if (j == -1)
			{
				if (!isRR()) j = SUBWAY;	// Duke
				else if (actor->sector()->floorpal == 7) j = 456;
				else j = 75;
			}
			actor->tempsound = j;
		}
		[[fallthrough]];
		case SE_30_TWO_WAY_TRAIN:
			if (numplayers > 1) break;
			[[fallthrough]];
		case SE_0_ROTATING_SECTOR:
		case SE_1_PIVOT:
		case SE_5_BOSS:
		case SE_11_SWINGING_DOOR:
		case SE_15_SLIDING_DOOR:
		case SE_16_REACTOR:
		case SE_26:
			setsectinterpolate(actor->sector());
			break;

		case SE_29_WAVES:
			StartInterpolation(actor->sector(), Interp_Sect_Floorheinum);
			StartInterpolation(actor->sector(), Interp_Sect_Floorz);
			break;
	}

	if ((!isRR() && actor->spr.lotag >= 40 && actor->spr.lotag <= 45) ||
		(isRRRA() && actor->spr.lotag >= 150 && actor->spr.lotag <= 155))
		ChangeActorStat(actor, STAT_RAROR);
	else
		ChangeActorStat(actor, STAT_EFFECTOR);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void lotsofglass(DDukeActor *actor, walltype* wal, int n)
{
	int j, z, a;
	sectortype* sect = nullptr;

	if (wal == nullptr)
	{
		for (j = n - 1; j >= 0; j--)
		{
			a = actor->int_ang() - 256 + (krand() & 511) + 1024;
			CreateActor(actor->sector(), actor->spr.pos, TILE_GLASSPIECES + (j % 3), -32, 36, 36, a, 32 + (krand() & 63), 1024 - (krand() & 1023), actor, 5);
		}
		return;
	}

	int x1 = wal->wall_int_pos().X;
	int y1 = wal->wall_int_pos().Y;
	auto delta = wal->delta() / (n + 1);

	x1 -= Sgn(delta.Y);
	y1 += Sgn(delta.X);


	for (j = n; j > 0; j--)
	{
		x1 += delta.X;
		y1 += delta.Y;

		updatesector(x1, y1, &sect);
		if (sect)
		{
			z = sect->int_floorz() - (krand() & (abs(sect->int_ceilingz() - sect->int_floorz())));
			if (z < -(32 << 8) || z >(32 << 8))
				z = actor->int_pos().Z - (32 << 8) + (krand() & ((64 << 8) - 1));
			a = actor->int_ang() - 1024;
			EGS(actor->sector(), x1, y1, z, TILE_GLASSPIECES + (j % 3), -32, 36, 36, a, 32 + (krand() & 63), -(krand() & 1023), actor, 5);
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
	for (int j = n; j > 0; j--)
	{
		int a = krand() & 2047;
		int z = actor->int_pos().Z - ((krand() & 16) << 8);
		auto k = EGS(actor->sector(), actor->int_pos().X, actor->int_pos().Y, z, TILE_GLASSPIECES + (j % 3), krand() & 15, 36, 36, a, 32 + (krand() & 63), -512 - (krand() & 2047), actor, 5);
		if (k) k->spr.pal = actor->spr.pal;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ceilingglass(DDukeActor* actor, sectortype* sectp, int n)
{
	int j, z;
	int a;

	for (auto& wal : wallsofsector(sectp))
	{
		int x1 = wal.wall_int_pos().X;
		int y1 = wal.wall_int_pos().Y;

		auto delta = wal.delta() / (n + 1);

		for (j = n; j > 0; j--)
		{
			x1 += delta.X;
			y1 += delta.Y;
			a = krand() & 2047;
			z = sectp->int_ceilingz() + ((krand() & 15) << 8);
			EGS(sectp, x1, y1, z, TILE_GLASSPIECES + (j % 3), -32, 36, 36, a, (krand() & 31), 0, actor, 5);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void lotsofcolourglass(DDukeActor* actor, walltype* wal, int n)
{
	int j, z;
	sectortype* sect = nullptr;
	int a;;

	if (wal == nullptr)
	{
		for (j = n - 1; j >= 0; j--)
		{
			a = krand() & 2047;
			auto k = EGS(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - (krand() & (63 << 8)), TILE_GLASSPIECES + (j % 3), -32, 36, 36, a, 32 + (krand() & 63), 1024 - (krand() & 2047), actor, 5);
			if (k) k->spr.pal = krand() & 15;
		}
		return;
	}

	int x1 = wal->wall_int_pos().X;
	int y1 = wal->wall_int_pos().Y;

	auto delta = wal->delta() / (n + 1);

	for (j = n; j > 0; j--)
	{
		x1 += delta.X;
		y1 += delta.Y;

		updatesector(x1, y1, &sect);
		z = sect->int_floorz() - (krand() & (abs(sect->int_ceilingz() - sect->int_floorz())));
		if (z < -(32 << 8) || z >(32 << 8))
			z = actor->int_pos().Z - (32 << 8) + (krand() & ((64 << 8) - 1));
		a = actor->int_ang() - 1024;
		auto k = EGS(actor->sector(), x1, y1, z, TILE_GLASSPIECES + (j % 3), -32, 36, 36, a, 32 + (krand() & 63), -(krand() & 2047), actor, 5);
		if (k) k->spr.pal = krand() & 7;
	}
}



END_DUKE_NS
