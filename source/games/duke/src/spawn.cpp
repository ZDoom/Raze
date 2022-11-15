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

DDukeActor* CreateActor(sectortype* whatsectp, const DVector3& pos, PClassActor* clstype, int s_pn, int8_t s_shd, const DVector2& scale, DAngle s_ang, double s_vel, double s_zvel, DDukeActor* s_ow, int8_t s_stat)
{
	// sector pointer must be strictly validated here or the engine will crash.
	if (whatsectp == nullptr || !validSectorIndex(sectnum(whatsectp))) return nullptr;
	// spawning out of range sprites will also crash.
	if (clstype == nullptr && (s_pn < 0 || s_pn >= MAXTILES)) return nullptr;

	int basepicnum = -1;
	if (!clstype)
	{
		auto info = spawnMap.CheckKey(s_pn);
		if (info)
		{
			clstype = static_cast<PClassActor*>(info->Class());
			basepicnum = info->param;
		}
	}

	auto act = static_cast<DDukeActor*>(InsertActor(clstype? clstype : RUNTIME_CLASS(DDukeActor), whatsectp, s_stat));

	if (act == nullptr) return nullptr;
	SetupGameVarsForActor(act);

	act->basepicnum = basepicnum;
	act->spr.pos = pos;
	if (s_pn != -1) act->spr.picnum = s_pn;	// if -1 use the class default.
	act->spr.shade = s_shd;
	if (!scale.isZero()) act->spr.scale = DVector2(scale.X, scale.Y);

	act->spr.angle = s_ang;
	act->vel.X = s_vel;
	act->vel.Z = s_zvel;
	act->backuploc();

	act->hitextra = -1;
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

DDukeActor* CreateActor(sectortype* whatsectp, const DVector3& pos, int s_pn, int8_t s_shd, const DVector2& scale, DAngle s_ang, double s_vel, double s_zvel, DDukeActor* s_ow, int8_t s_stat)
{
	return CreateActor(whatsectp, pos, nullptr, s_pn, s_shd, scale, s_ang, s_vel, s_zvel, s_ow, s_stat);
}

DDukeActor* CreateActor(sectortype* whatsectp, const DVector3& pos, PClassActor* cls, int8_t s_shd, const DVector2& scale, DAngle s_ang, double s_vel, double s_zvel, DDukeActor* s_ow, int8_t s_stat)
{
	return CreateActor(whatsectp, pos, cls, -1, s_shd, scale, s_ang, s_vel, s_zvel, s_ow, s_stat);
}

DDukeActor* SpawnActor(sectortype* whatsectp, const DVector3& pos, PClassActor* cls, int8_t s_shd, const DVector2& scale, DAngle s_ang, double s_vel, double s_zvel, DDukeActor* s_ow, int8_t s_stat)
{
	auto actor = CreateActor(whatsectp, pos, cls, s_shd, scale, s_ang, s_vel, s_zvel, s_ow, s_stat);
	if (actor) fi.spawninit(s_ow, actor, nullptr);
	return actor;
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
	act->tempval = 0;
	act->dispicnum = 0;
	act->floorz = act->sector()->floorz;
	act->ceilingz = act->sector()->ceilingz;

	act->ovel.Zero();
	act->actorstayput = nullptr;

	act->temp_data[0] = act->temp_data[1] = act->temp_data[2] = act->temp_data[3] = act->temp_data[4] = act->temp_data[5] = 0;
	act->temp_actor = nullptr;
	act->temp_angle = nullAngle;
	act->temp_pos = DVector3(0, 0, 0);

	if (wallswitchcheck(act) && (act->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
	{
		if (act->spr.picnum != TILE_ACCESSSWITCH && act->spr.picnum != TILE_ACCESSSWITCH2 && act->spr.pal)
		{
			if ((ud.multimode < 2) || (ud.multimode > 1 && ud.coop == 1))
			{
				act->spr.scale = DVector2(0, 0);
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
		auto spawned = CreateActor(actj->sector(), actj->spr.pos, pn, 0, DVector2(0, 0), nullAngle, 0., 0., actj, 0);
		if (spawned)
		{
			spawned->attackertype = actj->spr.picnum;
			return fi.spawninit(actj, spawned, nullptr);
		}
	}
	return nullptr;
}

DDukeActor* spawn(DDukeActor* actj, PClassActor * cls)
{
	if (actj && cls)
	{
		auto spawned = CreateActor(actj->sector(), actj->spr.pos, cls, 0, DVector2(0, 0), nullAngle, 0., 0., actj, 0);
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
			act->spr.scale = DVector2(0, 0);
			ChangeActorStat(act, STAT_MISC);
			return;
		}

		//  Init the size
		if (act->spr.scale.X == 0 || act->spr.scale.Y == 0)
			act->spr.scale = DVector2(REPEAT_SCALE, REPEAT_SCALE);

		if (actorflag(act, SFLAG_BADGUY))
		{
			if (ud.monsters_off == 1)
			{
				act->spr.scale = DVector2(0, 0);
				ChangeActorStat(act, STAT_MISC);
				return;
			}

			makeitfall(act);

			if (actorflag(act, SFLAG_BADGUYSTAYPUT))
				act->actorstayput = act->sector();

			if (!isRR() || actorflag(act, SFLAG_KILLCOUNT))	// Duke is just like Doom - Bad guys always count as kill.
				ps[myconnectindex].max_actors_killed++;

			act->clipdist = 20;
			if (actj)
			{
				if (actj->spr.picnum == RESPAWN)
					act->tempval = act->spr.pal = actj->spr.pal;
				ChangeActorStat(act, STAT_ACTOR);
			}
			else ChangeActorStat(act, STAT_ZOMBIEACTOR);
		}
		else
		{
			act->clipdist = 10;
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
		act->spr.scale = DVector2(0.484375, REPEAT_SCALE);
		act->spr.pos.Z = actj->sector()->floorz - gs.playerheight;
	}
	else
	{
		if (actj->spr.statnum == 4)
		{
			act->spr.scale = DVector2(0.125, 0.125);
		}
		else
		{
			act->spr.scale = DVector2(0.75, 1);
			if (actj->spr.statnum == 10 || badguy(actj))
				act->spr.pos.Z -= 32;
		}
	}

	act->spr.shade = -127;
	act->spr.cstat = CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
	act->spr.angle = actj->spr.angle;

	act->vel.X = 8;
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
	bool away = isAwayFromWall(act, 6.75);
	
	if (!away)
	{
		act->spr.scale = DVector2(0, 0); 
		ChangeActorStat(act, STAT_MISC); 
		return true;
	}

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
		bool away = isAwayFromWall(act, 5.25);
		if (!away)
		{
			act->spr.scale = DVector2(0, 0);
			return;
		}

		act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_FLOOR;
		if ((ps[actj->PlayerIndex()].footprintcount & 1)) act->spr.cstat |= CSTAT_SPRITE_XFLIP;
		act->spr.angle = actj->spr.angle;
	}

	act->spr.pos.Z = sect->floorz;
	if (sect->lotag != 1 && sect->lotag != 2)
		act->spr.scale = DVector2(0.5, 0.5);

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
		int snum;
		DAngle ang;

		if (actj->isPlayer())
		{
			snum = actj->PlayerIndex();
			ang = ps[snum].angle.ang - mapangle((krand() & 63) + 8);  //Fine tune

			act->temp_data[0] = krand() & 1;
			act->spr.pos.Z = 3 + ps[snum].pos.Z + ps[snum].pyoff + (ps[snum].horizon.sum().Tan() * 8.) + (!isshell ? 3 : 0);
			act->vel.Z = -krandf(1);
		}
		else
		{
			ang = act->spr.angle;
			act->spr.pos.Z = actj->spr.pos.Z - gs.playerheight + 3;
		}

		act->spr.pos.XY() = actj->spr.pos.XY() + ang.ToVector() * 8;

		act->spr.shade = -8;

		if (isNamWW2GI())
		{
			// to the right, with feeling
			act->spr.angle = ang + DAngle90;
			act->vel.X = 1.875;
		}
		else
		{
			act->spr.angle = ang - DAngle90;
			act->vel.X = 1.25;
		}

		double scale = isRR() && isshell ? 0.03125 : 0.0625;
		act->spr.scale = DVector2(scale, scale);

		ChangeActorStat(act, STAT_MISC);
	}
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
		actor->spr.angle = (ps[connecthead].pos.XY() - actor->spr.pos.XY()).Angle();
		actor->vel.X = 3 - krandf(2);
		ssp(actor, CLIPMASK0);
	}
	else if (!actj)
	{
		actor->spr.pos.Z += 4;
		actor->temp_pos.Z = actor->spr.pos.Z;
		if (!isRR()) actor->temp_data[1] = krand() & 127;
	}
	actor->spr.scale = DVector2(0.375, 0.375);
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
			actor->spr.scale = DVector2(0, 0);
			ChangeActorStat(actor, STAT_MISC);
			return true;
		}
		if (!isRR() || actorflag(actor, SFLAG_KILLCOUNT))	// Duke is just like Doom - Bad guys always count as kill.
			ps[myconnectindex].max_actors_killed++;
		actor->temp_data[5] = 0;
		if (ud.monsters_off == 1)
		{
			actor->spr.scale = DVector2(0, 0);
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
		actor->spr.scale = DVector2(0, 0);
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
	int clostest = 0;

	actor->spr.yint = sectp->extra;
	actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
	actor->spr.scale = DVector2(0, 0);

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
			actor->temp_pos.Z = sectp->ceilingz;
			actor->temp_data[4] = 1;
			sectp->setceilingz(actor->spr.pos.Z);
			StartInterpolation(sectp, Interp_Sect_Ceilingz);
			break;
		case SE_35:
			sectp->setceilingz(actor->spr.pos.Z);
			break;
		case SE_27_DEMO_CAM:
			if (ud.recstat == 1)
			{
				actor->spr.scale = DVector2(1, 1);
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
			actor->temp_pos.Y = sectp->ceilingz;
			actor->temp_pos.Z = sectp->floorz;

			bool ceiling = (abs(sectp->ceilingz - actor->spr.pos.Z) < abs(sectp->floorz - actor->spr.pos.Z));
			actor->spriteextra = ceiling;

			if (actor->spr.intangle == 512)
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

				if (!ceiling && actor->spr.intangle == 512)
				{
					sectp->ceilingstat ^= CSTAT_SECTOR_SKY;
					actor->temp_data[3] = 0;
				}

				sectp->ceilingshade =
					sectp->floorshade;

				if (actor->spr.intangle == 512)
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
			actor->temp_pos.X = sectp->floorz; //Stopping loc
			actor->temp_pos.Y = nextsectorneighborzptr(sectp, sectp->floorz, Find_CeilingUp | Find_Safe)->ceilingz;
			actor->temp_pos.Z = nextsectorneighborzptr(sectp, sectp->ceilingz, Find_FloorDown | Find_Safe)->floorz;

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
			actor->spr.yint <<= 1;
		case SE_36_PROJ_SHOOTER:
			break;

		case SE_20_STRETCH_BRIDGE:
		{
			//find the two most clostest wall x's and y's
			for (unsigned i = 0; i < 2; i++)
			{
				walltype* closewall = nullptr;
				double maxdist = 0x7fffffff;

				for (auto& wal : wallsofsector(sectp))
				{
					double dist = (actor->spr.pos.XY() - wal.pos).LengthSquared();
					if (dist < maxdist && &wal != actor->temp_walls[0])
					{
						maxdist = dist;
						closewall = &wal;
					}
				}

				actor->temp_walls[i] = closewall;

				vertexscan(actor->temp_walls[i], [=](walltype* w)
					{
						StartInterpolation(w, Interp_Wall_X);
						StartInterpolation(w, Interp_Wall_Y);
					});
			}
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
			actor->temp_pos.Z = actor->spr.yint * zmaptoworld;
			actor->temp_pos.Y = sectp->floorz;
			//	actor->temp_data[2] = actor->spr.hitag;
			if (actor->spr.intangle != 1536) sectp->setfloorz(actor->spr.pos.Z);

			for (auto& wal : wallsofsector(sectp))
				if (wal.hitag == 0) wal.hitag = 9999;

			StartInterpolation(sectp, Interp_Sect_Floorz);

			break;
		case SE_32_CEILING_RISE_FALL:
			actor->temp_pos.Z = sectp->ceilingz;
			actor->temp_data[2] = actor->spr.hitag;
			if (actor->spr.intangle != 1536) sectp->setceilingz(actor->spr.pos.Z);

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
				abs(sectp->ceilingz - actor->spr.pos.Z) > 4)
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
			if (actor->spr.intangle > 1024) actor->temp_data[3] = 2;
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
			if (actor->spr.lotag == SE_0_ROTATING_SECTOR)
			{
				if (sectp->lotag == 30)
				{
					if (actor->spr.pal) actor->spr.clipdist = 1; // notreallyclipdist
					else actor->spr.clipdist = 0;
					actor->temp_pos.Z = sectp->floorz;
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
							if (actor->spr.angle == DAngle90)
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
					Printf("Found lonely Sector Effector (lotag 0) at (%d,%d)\n", int(actor->spr.pos.X), int(actor->spr.pos.Y));
					return;
				}
			}

			actor->temp_data[1] = mspos.Size();
			for (auto& wal : wallsofsector(sectp))
			{
				mspos.Push(wal.pos - actor->spr.pos);
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
				actor->temp_pos.Z = sectp->ceilingz;

			else if (actor->spr.lotag == SE_26)
			{
				actor->temp_pos.XY() = actor->spr.pos.XY();
				if (actor->spr.shade == sectp->floorshade) //UP
					actor->vel.Z = -1;
				else
					actor->vel.Z = 1;

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
	int j;
	sectortype* sect = nullptr;

	if (wal == nullptr)
	{
		for (j = n - 1; j >= 0; j--)
		{
			auto a = actor->spr.angle - DAngle45 + DAngle180 + randomAngle(90);
			auto vel = krandf(4) + 2;
			auto zvel = 4 - krandf(4);

			CreateActor(actor->sector(), actor->spr.pos, TILE_GLASSPIECES + (j % 3), -32, DVector2(0.5625, 0.5625), a, vel, zvel, actor, 5);
		}
		return;
	}
	

	auto pos = wal->pos;
	auto delta = wal->delta() / (n + 1);

	pos.X -= Sgn(delta.Y) * maptoworld;
	pos.Y += Sgn(delta.X) * maptoworld;


	for (j = n; j > 0; j--)
	{
		pos += delta;
		sect = wal->sectorp();
		updatesector(DVector3(pos, sect->floorz), &sect);
		if (sect)
		{
			double z = sect->floorz - krandf(abs(sect->ceilingz - sect->floorz));
			if (fabs(z) > 32)
				z = actor->spr.pos.Z - 32 + krandf(64);
			DAngle angl = actor->spr.angle - DAngle180;
			auto vel = krandf(4) + 2;
			auto zvel = 4 - krandf(4);

			CreateActor(actor->sector(), DVector3(pos, z), TILE_GLASSPIECES + (j % 3), -32, DVector2(0.5625, 0.5625), angl, vel, zvel, actor, 5);
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
		auto a = randomAngle();
		auto vel = krandf(4) + 2;
		auto zvel = -2 - krandf(8);

		auto k = CreateActor(actor->sector(), actor->spr.pos.plusZ(-(krand() & 16)), TILE_GLASSPIECES + (j % 3), krand() & 15, DVector2(0.5625, 0.5625), a, vel, zvel, actor, 5);
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
	for (auto& wal : wallsofsector(sectp))
	{
		auto pos = wal.pos;
		auto delta = wal.delta() / (n + 1);

		for (int j = n; j > 0; j--)
		{
			pos += delta;
			DAngle a = randomAngle();
			auto vel = krandf(2);

			double z = sectp->ceilingz + krandf(16);
			CreateActor(sectp, DVector3(pos, z), TILE_GLASSPIECES + (j % 3), -32, DVector2(0.5625, 0.5625), a, vel, 0, actor, 5);
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
	int j;
	sectortype* sect = nullptr;

	if (wal == nullptr)
	{
		for (j = n - 1; j >= 0; j--)
		{
			DAngle a = randomAngle();
			auto vel = krandf(4) + 2;
			auto zvel = 4 - krandf(4);

			auto k = CreateActor(actor->sector(), actor->spr.pos.plusZ(-(krand() & 63)), TILE_GLASSPIECES + (j % 3), -32, DVector2(0.5625, 0.5625), a, vel, zvel, actor, 5);
			if (k) k->spr.pal = krand() & 15;
		}
		return;
	}

	auto pos = wal->pos;
	auto delta = wal->delta() / (n + 1);

	for (j = n; j > 0; j--)
	{
		pos += delta;

		sect = wal->sectorp();
		updatesector(DVector3(pos, sect->floorz), &sect);
		if (!sect) continue;
		double z = sect->floorz - krandf(abs(sect->ceilingz - sect->floorz));
		if (abs(z) > 32)
			z = actor->spr.pos.Z - 32 + krandf(64);

		DAngle a = actor->spr.angle - DAngle180;
		auto vel = krandf(4) + 2;
		auto zvel = - krandf(8);

		auto k = CreateActor(actor->sector(), DVector3(pos, z), TILE_GLASSPIECES + (j % 3), -32, DVector2(0.5625, 0.5625), a, vel, zvel, actor, 5);
		if (k) k->spr.pal = krand() & 7;
	}
}



END_DUKE_NS
