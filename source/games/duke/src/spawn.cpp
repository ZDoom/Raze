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
#include "vm.h"

BEGIN_DUKE_NS


void setFromSpawnRec(DDukeActor* act, SpawnRec* info)
{
	act->curMove = &moves[0];
	act->curAction = &actions[0];
	if (info)
	{
		if (info->basetex.isValid() && act->IsKindOf(DukeGenericDestructibleClass))
		{
			// allow defining simple destructibles without actual actor definitions.
			act->IntVar(NAME_spawnstate) = info->basetex.GetIndex();
			act->IntVar(NAME_brokenstate) = info->brokentex.GetIndex();
			act->IntVar(NAME_breaksound) = info->breaksound.index();
			act->IntVar(NAME_fullbright) = info->fullbright;
			act->spr.inittype = info->flags;
			if (info->clipdist > 0) act->spr.clipdist = info->clipdist;
		}
		else
		{
			// same for simple sprite replacements with existing implementations.
			if (info->basetex.isValid()) act->spr.setspritetexture(info->basetex);
			if (info->fullbright & 1) act->spr.cstat2 |= CSTAT2_SPRITE_FULLBRIGHT;
		}
	}
}

//---------------------------------------------------------------------------
//
// set default pointers for scripted animation data
//
//---------------------------------------------------------------------------
EXTERN_CVAR(Bool, overridecon)	// This is for debugging the CON replacement code only.

static void initanimations(DDukeActor* act)
{
	auto coninf = act->conInfo();

	if ((!coninf || overridecon) && (act->flags4 & SFLAG4_CONOVERRIDE))
	{
		auto ainf = static_cast<PClassActor*>(act->GetClass())->ActorInfo();
		int ndx = LookupAction(act->GetClass(),ainf->DefaultAction);
		act->curAction = &actions[ndx];
		act->actioncounter = act->curframe = 0;

		ndx = LookupMove(act->GetClass(), ainf->DefaultMove);
		act->curMove = &moves[ndx >= 0 && ndx < moves.SSize() ? ndx : 0];
		if (ainf->DefaultMoveflags && act->spr.hitag == 0)
			act->spr.hitag = ainf->DefaultMoveflags;
	}
	else if (coninf)
	{
		auto sa = &ScriptCode[coninf->scriptaddress];
		act->curAction = &actions[sa[1]];
		act->curMove = &moves[sa[2] >= 0 && sa[2] < moves.SSize() ? sa[2] : 0];
		int s3 = sa[3];
		if (s3 && act->spr.hitag == 0)
			act->spr.hitag = s3;
	}
}

//---------------------------------------------------------------------------
//
// this creates a new actor but does not run any init code on it
// direct calls should only be done for very simple things.
//
//---------------------------------------------------------------------------

DDukeActor* CreateActor(sectortype* whatsectp, const DVector3& pos, PClassActor* clstype, int8_t s_shd, const DVector2& scale, DAngle s_ang, double s_vel, double s_zvel, DDukeActor* s_ow, int8_t s_stat)
{
	// sector pointer must be strictly validated here or the engine will crash.
	if (whatsectp == nullptr || !validSectorIndex(sectindex(whatsectp))) return nullptr;
	// spawning out of range sprites will also crash.
	if (clstype == nullptr) return nullptr;

	if (s_stat < 0) s_stat = clstype ? GetDefaultByType(clstype)->spr.statnum : 0;

	auto act = static_cast<DDukeActor*>(InsertActor(clstype? clstype : RUNTIME_CLASS(DDukeActor), whatsectp, s_stat));
	if (act == nullptr) return nullptr;
	SetupGameVarsForActor(act);

	setFromSpawnRec(act, nullptr);
	act->spr.pos = pos;
	act->spr.shade = s_shd;
	if (!scale.isZero()) act->spr.scale = DVector2(scale.X, scale.Y);

	act->spr.Angles.Yaw = s_ang;
	act->vel.X = s_vel;
	act->vel.Z = s_zvel;
	act->backuploc();

	act->hitextra = -1;
	act->SetHitOwner(s_ow);
	act->SetOwner(s_ow);

	if (s_ow)
	{
		act->attackertype = static_cast<PClassActor*>(s_ow->GetClass());
		act->floorz = s_ow->floorz;
		act->ceilingz = s_ow->ceilingz;
	}
	else
	{

	}

	memset(act->temp_data, 0, sizeof(act->temp_data));
	initanimations(act);
	act->spr.extra = act->IntVar(NAME_strength);

	if (show2dsector[act->sectno()]) act->spr.cstat2 |= CSTAT2_SPRITE_MAPPED;
	else act->spr.cstat2 &= ~CSTAT2_SPRITE_MAPPED;

	return act;

}

DDukeActor* SpawnActor(sectortype* whatsectp, const DVector3& pos, PClassActor* cls, int8_t s_shd, const DVector2& scale, DAngle s_ang, double s_vel, double s_zvel, DDukeActor* s_ow, int8_t s_stat)
{
	auto actor = CreateActor(whatsectp, pos, cls, s_shd, scale, s_ang, s_vel, s_zvel, s_ow, s_stat);
	if (actor)
	{
		actor->flags3 |= SFLAG3_SIMPLEINIT; // at this point we only want to run custom Initialize code, but not the default for scripted actors, even if this one has scripts.
		spawninit(s_ow, actor, nullptr);
		actor->flags3 &= ~SFLAG3_SIMPLEINIT; // from now on act normally.
	}
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
	act->attackertype = static_cast<PClassActor*>(act->GetClass());
	act->hitextra = -1;

	act->backuppos();

	act->SetOwner(act);
	act->SetHitOwner(act);
	act->floorz = act->sector()->floorz;
	act->ceilingz = act->sector()->ceilingz;

	auto ext = GetExtInfo(act->spr.spritetexture());
	bool overrideswitch = false;

	// The STAT_FALLER code below would render any switch actor inoperable so we must also include everything that overrides TriggerSwitch, 
	// even if it got no other hint for being a switch. A bit dirty but it makes it unnecessary to explicitly mark such switches.

	IFVIRTUALPTR(act, DDukeActor, TriggerSwitch)
	{
		if (stricmp(func->PrintableName, "DukeActor.TriggerSwitch") != 0)
			overrideswitch = true;
	}

	if (act->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL && (wallswitchcheck(act) || ext.switchindex > 0 || overrideswitch))
	{
		// this is a bit more complicated than needed thanks to some bugs in the original code that must be retained for the multiplayer filter. 
		// Not all switches were properly included here.
		bool shouldfilter = wallswitchcheck(act) || !(switches[ext.switchindex].flags & SwitchDef::nofilter);

		if (shouldfilter && act->spr.pal > 0)
		{
			if ((ud.multimode < 2) || (ud.multimode > 1 && ud.coop == 1))
			{
				act->spr.scale = DVector2(0, 0);
				act->spr.cstat = 0;
				act->spr.lotag = act->spr.hitag = 0;
				return false;
			}
			act->spr.pal = 0;
		}
		act->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		return true;

	}

	if (!(act->flags1 & SFLAG_NOFALLER) && (act->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK))
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

	if (act->spr.cstat & CSTAT_SPRITE_BLOCK) act->spr.cstat |= CSTAT_SPRITE_BLOCK_HITSCAN;

	initanimations(act);
	act->spr.extra = act->IntVar(NAME_strength);

	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DDukeActor* spawn(DDukeActor* actj, PClassActor * cls)
{
	if (actj && cls)
	{
		auto spawned = CreateActor(actj->sector(), actj->spr.pos, cls, 0, DVector2(0, 0), nullAngle, 0., 0., actj, 0);
		if (spawned)
		{
			spawned->attackertype = static_cast<PClassActor*>(actj->GetClass());
			return spawninit(actj, spawned, nullptr);
		}
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
// This spawns an actor from a spawnclasses type ID.
//
//---------------------------------------------------------------------------

DDukeActor* spawnsprite(DDukeActor* origin, int typeId)
{
	auto srec = spawnMap.CheckKey(typeId);
	if (srec && !srec->basetex.isValid()) return spawn(origin, srec->cls);

	PClassActor* cls = srec ? srec->cls : (PClassActor*)RUNTIME_CLASS(DDukeActor);
	auto spawned = spawn(origin, cls);
	if (!spawned) return nullptr;
	setFromSpawnRec(spawned, srec);
	if (!srec) spawned->spr.setspritetexture(tileGetTextureID(typeId));
	return spawned;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void commonEnemySetup(DDukeActor* self, DDukeActor* owner)
{
	if (!self->mapSpawned) self->spr.lotag = 0;

	if (self->flags1 & SFLAG_BADGUYSTAYPUT)
	{
		self->actorstayput = self->spr.sectp;
	}

	if (gs.actorinfo[self->spr.picnum].scriptaddress)	// default scale only applies to actors with a CON part. Note: needs fixing later!
	{
		//  Init the size. This is different for internal and user enemies.
		self->clipdist = 20;
		if (self->flags1 & SFLAG_INTERNAL_BADGUY)
		{
			self->spr.scale = DVector2(0.625, 0.625);
		}
		else if (self->spr.scale.X == 0 || self->spr.scale.Y == 0)
		{
			self->spr.scale = DVector2(REPEAT_SCALE, REPEAT_SCALE);
		}
	}

	makeitfall(self);

	self->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;

	addtokills(self);

	self->timetosleep = 0;
	if (!self->mapSpawned)
	{
		CallPlayFTASound(self);
		ChangeActorStat(self, STAT_ACTOR);
		if (owner && !(self->flags1 & SFLAG_INTERNAL_BADGUY)) self->spr.Angles.Yaw = owner->spr.Angles.Yaw;
	}
	else ChangeActorStat(self, STAT_ZOMBIEACTOR);
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
			if (!isRR()) actor->temp_data[0] = 65;// Delay for lightning
			break;
		case SE_7_TELEPORT: // Transporters!!!!
		case SE_23_ONE_WAY_TELEPORT:// XPTR END
			if (actor->spr.lotag != SE_23_ONE_WAY_TELEPORT && actors)
			{

				for(auto act2 : *actors)
				{
					if (act2->spr.statnum < MAXSTATUS && iseffector(act2) && (act2->spr.lotag == SE_7_TELEPORT || act2->spr.lotag == SE_23_ONE_WAY_TELEPORT) && 
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
			actor->counter = 1;
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
					for (auto& wl : sectp->walls)
					{
						if (wl.twoSided())
						{
							auto nsec = wl.nextSector();
							if (!(nsec->ceilingstat & CSTAT_SECTOR_SKY))
							{
								sectp->setceilingtexture(nsec->ceilingtexture);
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
			auto FindDistance2D = [](int x, int y)
			{
				x = abs(x);
				y = abs(y);

				if (x < y)
					std::swap(x, y);

				int t = y + (y >> 1);

				return (x - (x >> 5) - (x >> 7) + (t >> 2) + (t >> 6));

			};
			//find the two most closest wall x's and y's

			for (unsigned i = 0; i < 2; i++)
			{
				walltype* closewall = nullptr;
				int maxdist = 0x7fffffff;

				for (auto& wal : sectp->walls)
				{
					int x = int(actor->spr.pos.X - wal.pos.X) * 16;
					int y = int(actor->spr.pos.Y - wal.pos.Y) * 16;

					int dist = FindDistance2D(x, y);
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

			for (auto& wal : sectp->walls)
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

			for (auto& wal : sectp->walls)
				if (wal.hitag == 0) wal.hitag = 9999;

			StartInterpolation(sectp, Interp_Sect_Floorz);

			break;
		case SE_32_CEILING_RISE_FALL:
			actor->temp_pos.Z = sectp->ceilingz;
			actor->temp_data[2] = actor->spr.hitag;
			if (actor->spr.intangle != 1536) sectp->setceilingz(actor->spr.pos.Z);

			for (auto& wal : sectp->walls)
				if (wal.hitag == 0) wal.hitag = 9999;

			StartInterpolation(sectp, Interp_Sect_Ceilingz);

			break;

		case SE_4_RANDOM_LIGHTS: //Flashing lights

			actor->temp_data[2] = sectp->floorshade;

			actor->palvals = (sectp->ceilingpal << 8) | sectp->floorpal;

			for (auto& wal : sectp->walls)
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

			actor->counter = sectp->floorshade;
			actor->temp_data[1] = sectp->ceilingshade;

			for (auto& wal : sectp->walls)
				if (wal.shade > actor->temp_data[2])
					actor->temp_data[2] = wal.shade;

			actor->temp_data[3] = 1; //Take Out;

			break;

		case 88:
			//First, get the ceiling-floor shade
			if (!isRR()) break;

			actor->counter = sectp->floorshade;
			actor->temp_data[1] = sectp->ceilingshade;

			for (auto& wal : sectp->walls)
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
					if (actor->spr.pal) actor->spr.detail = 1;
					else actor->spr.detail = 0;
					actor->temp_pos.Z = sectp->floorz;
					sectp->hitagactor = actor;
				}


				bool found = false;
				if (actors) for (auto act2 : *actors)
				{
					if (act2->spr.statnum < MAXSTATUS)
						if (iseffector(act2) &&
							act2->spr.lotag == SE_1_PIVOT &&
							act2->spr.hitag == actor->spr.hitag)
						{
							if (actor->spr.Angles.Yaw == DAngle90)
							{
								actor->spr.pos.SetXY(act2->spr.pos.XY());
							}
							found = true;
							actor->SetOwner(act2);
							break;
						}
				}
				if (!found)
				{
					actor->spr.setspritetexture(FNullTextureID());
					actor->spr.cstat2 = CSTAT2_SPRITE_NOFIND;
					actor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
					ChangeActorStat(actor, STAT_REMOVED);
					Printf("Found lonely Sector Effector (lotag 0) at (%d,%d)\n", int(actor->spr.pos.X), int(actor->spr.pos.Y));
					return;
				}
			}

			actor->temp_data[1] = mspos.Size();
			for (auto& wal : sectp->walls)
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
				for (auto& wal : sectp->walls)
				{
					if (wal.twoSided() &&
						wal.nextSector()->hitag == 0 &&
						(wal.nextSector()->lotag < 3 || ((ud.mapflags & MFLAG_ALLSECTORTYPES) && wal.nextSector()->lotag == ST_160_FLOOR_TELEPORT)))
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
				actor->counter = sectindex(s);

				if (actor->spr.lotag != SE_30_TWO_WAY_TRAIN)
					actor->temp_data[3] = actor->spr.hitag;
			}

			else if (actor->spr.lotag == SE_16_REACTOR)
				actor->temp_pos.Z = sectp->ceilingz;

			else if (actor->spr.lotag == SE_26)
			{
				actor->temp_pos.SetXY(actor->spr.pos.XY());
				if (actor->spr.shade == sectp->floorshade) //UP
					actor->vel.Z = -1;
				else
					actor->vel.Z = 1;

				actor->spr.shade = 0;
			}
			else if (actor->spr.lotag == SE_2_EARTHQUAKE)
			{
				actor->temp_data[0] = actor->sector()->getfloorslope();
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
				if (isactivator(itActor) || islockedactivator(itActor))
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
			if(actor->spr.lotag == SE_0_ROTATING_SECTOR) StartInterpolation(actor->sector(), Interp_Sect_Floorz);
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

DDukeActor* spawninit(DDukeActor* actj, DDukeActor* act, TArray<DDukeActor*>* actors)
{
	if ((act->flags2 & SFLAG2_TRIGGERRESPAWN))
	{
		act->spr.yint = act->spr.hitag;
		act->spr.hitag = -1;
	}

	if (iseffector(act))
	{
		// for in-game spawned SE's the init code must not run. The only type ever being spawned that way is SE128 - 
		// but we cannot check that here as the number has not been set yet.
		if (actj == 0) spawneffector(act, actors);
	}
	else if (!act->isPlayer())
	{
		if (act->flags1 & SFLAG_SKILLFILTER)
		{
			if (act->spr.lotag > ud.player_skill)
			{
				act->spr.scale.Zero();
				ChangeActorStat(act, STAT_MISC);
				return nullptr;
			}
		}
		if (badguy(act))
		{
			if (ud.monsters_off == 1)
			{
				act->spr.scale.Zero();
				ChangeActorStat(act, STAT_MISC);
				return nullptr;
			}
			commonEnemySetup(act, actj);
		}

		CallInitialize(act, actj);
	}
	else
	{
		act->spr.scale = DVector2(0, 0);
		int j = ud.coop;
		if (j == 2) j = 0;

		if (ud.multimode < 2 || (ud.multimode > 1 && j != act->spr.lotag))
			ChangeActorStat(act, STAT_MISC);
		else
			ChangeActorStat(act, STAT_PLAYER);
		CallInitialize(act, nullptr);
	}
	return act;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void spriteinit(DDukeActor* actor, TArray<DDukeActor*>& actors)
{
	actor->mapSpawned = true;
	bool res = initspriteforspawn(actor);
	if (res) spawninit(nullptr, actor, &actors);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

inline PClassActor* GlassClass(int j)
{
	static PClassActor* const* glasses[] = { &DukeGlassPiecesClass, &DukeGlassPieces1Class, &DukeGlassPieces2Class };
	return *glasses[j % 3];
}

void lotsofglass(DDukeActor *actor, walltype* wal, int n)
{
	int j;
	sectortype* sect = nullptr;

	if (wal == nullptr)
	{
		for (j = n - 1; j >= 0; j--)
		{
			auto a = actor->spr.Angles.Yaw - DAngle45 + DAngle180 + randomAngle(90);
			auto vel = krandf(4) + 2;
			auto zvel = 4 - krandf(4);

			CreateActor(actor->sector(), actor->spr.pos, GlassClass(j), -32, DVector2(0.5625, 0.5625), a, vel, zvel, actor, 5);
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
			DAngle angl = actor->spr.Angles.Yaw - DAngle180;
			auto vel = krandf(4) + 2;
			auto zvel = 4 - krandf(4);

			CreateActor(actor->sector(), DVector3(pos, z), GlassClass(j), -32, DVector2(0.5625, 0.5625), angl, vel, zvel, actor, 5);
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

		auto k = CreateActor(actor->sector(), actor->spr.pos.plusZ(-(krand() & 16)), GlassClass(j), krand() & 15, DVector2(0.5625, 0.5625), a, vel, zvel, actor, 5);
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
	for (auto& wal : sectp->walls)
	{
		auto pos = wal.pos;
		auto delta = wal.delta() / (n + 1);

		for (int j = n; j > 0; j--)
		{
			pos += delta;
			DAngle a = randomAngle();
			auto vel = krandf(2);

			double z = sectp->ceilingz + krandf(16);
			CreateActor(sectp, DVector3(pos, z), GlassClass(j), -32, DVector2(0.5625, 0.5625), a, vel, 0, actor, 5);
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

			auto k = CreateActor(actor->sector(), actor->spr.pos.plusZ(-(krand() & 63)), GlassClass(j), -32, DVector2(0.5625, 0.5625), a, vel, zvel, actor, 5);
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

		DAngle a = actor->spr.Angles.Yaw - DAngle180;
		auto vel = krandf(4) + 2;
		auto zvel = - krandf(8);

		auto k = CreateActor(actor->sector(), DVector3(pos, z), GlassClass(j), -32, DVector2(0.5625, 0.5625), a, vel, zvel, actor, 5);
		if (k) k->spr.pal = krand() & 7;
	}
}

END_DUKE_NS
