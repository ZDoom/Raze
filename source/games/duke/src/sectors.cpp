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
#include "dukeactor.h"
#include "interpolate.h"

// PRIMITIVE
BEGIN_DUKE_NS

static int interptype[] = { Interp_Sect_Floorz, Interp_Sect_Ceilingz, Interp_Wall_X, Interp_Wall_Y };

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ceilingspace(sectortype* sectp)
{
	return (sectp && (sectp->ceilingstat & CSTAT_SECTOR_SKY) && sectp->ceilingpal == 0 && (tilesurface(sectp->ceilingtexture) == TSURF_OUTERSPACE));
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool floorspace(sectortype* sectp)
{
	// Yes, ceilingpal in this check is correct...
	return (sectp && (sectp->floorstat & CSTAT_SECTOR_SKY) && sectp->ceilingpal == 0 && (tilesurface(sectp->floortexture) == TSURF_OUTERSPACE));
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------
static bool haltsoundhack;

int callsound(sectortype* sn, DDukeActor* whatsprite, bool endstate)
{
	if (!isRRRA() && haltsoundhack)
	{
		haltsoundhack = 0;
		return -1;
	}

	DukeSectIterator it(sn);
	while (auto act = it.Next())
	{
		if (issoundcontroller(act) && act->spr.lotag < 1000)
		{
			if (whatsprite == nullptr) whatsprite = act;

			int snum = act->spr.lotag;
			auto flags = S_GetUserFlags(snum);

			// Reset if the desired actor isn't playing anything.
			bool hival = S_IsSoundValid(act->spr.hitag);
			if (act->counter == 1 && !hival && !endstate)
			{
				if (!S_CheckActorSoundPlaying(act->temp_actor, snum))
					act->counter = 0;
			}

			if (act->counter == 0)
			{
				if ((flags & (SF_GLOBAL | SF_DTAG)) != SF_GLOBAL)
				{
					if (snum)
					{
						if (act->spr.hitag && snum != act->spr.hitag)
							S_StopSound(act->spr.hitag, act->temp_actor);
						S_PlayActorSound(snum, whatsprite);
						act->temp_actor = whatsprite;
					}

					if ((act->sector()->lotag & 0xff) != ST_22_SPLITTING_DOOR)
						act->counter = 1;
				}
			}
			else if (act->spr.hitag < 1000)
			{
				// The original code performed these two actions in reverse order which in case of a looped sound being stopped
				// being the same as the sound about to be started, the newly started sound would fall through some cracks in the sound system and be rejected.
				// Here this case needs to be simulated.
				bool stopped = false;
				if ((flags & SF_LOOP) || (act->spr.hitag && act->spr.hitag != act->spr.lotag))
				{
					S_StopSound(act->spr.lotag, act->temp_actor);
					if (act->spr.hitag == act->spr.lotag) stopped = true;
				}
				if (act->spr.hitag && !stopped) S_PlayActorSound(act->spr.hitag, whatsprite);
				act->counter = 0;
				act->temp_actor = whatsprite;
			}
			return act->spr.lotag;
		}
	}
	return -1;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int check_activator_motion(int lotag)
{
	DukeStatIterator it(STAT_ACTIVATOR);
	while (auto act = it.Next())
	{
		if (act->spr.lotag == lotag)
		{
			for (int j = animates.Size() - 1; j >= 0; j--)
				if (act->sector() == animates[j].sect)
					return(1);

			DukeStatIterator it1(STAT_EFFECTOR);
			while (auto act2 = it1.Next())
			{
				if (act->sector() == act2->sector())
					switch (act2->spr.lotag)
					{
					case SE_11_SWINGING_DOOR:
					case SE_30_TWO_WAY_TRAIN:
						if (act2->temp_data[4])
							return(1);
						break;
					case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
						if (isRRRA()) break;
						[[fallthrough]];
					case SE_20_STRETCH_BRIDGE:
					case SE_31_FLOOR_RISE_FALL:
					case SE_32_CEILING_RISE_FALL:
						if (act2->counter)
							return(1);
						break;
					}

			}
		}
	}
	return(0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool isanunderoperator(int lotag)
{
	switch (lotag & 0xff)
	{
	case ST_15_WARP_ELEVATOR:
	case ST_16_PLATFORM_DOWN:
	case ST_17_PLATFORM_UP:
	case ST_18_ELEVATOR_DOWN:
	case ST_19_ELEVATOR_UP:
	case ST_26_SPLITTING_ST_DOOR:
		return true;
	case ST_22_SPLITTING_DOOR:
		return !isRR();
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool isanearoperator(int lotag)
{
	switch (lotag & 0xff)
	{
	case ST_9_SLIDING_ST_DOOR:
	case ST_15_WARP_ELEVATOR:
	case ST_16_PLATFORM_DOWN:
	case ST_17_PLATFORM_UP:
	case ST_18_ELEVATOR_DOWN:
	case ST_19_ELEVATOR_UP:
	case ST_20_CEILING_DOOR:
	case ST_21_FLOOR_DOOR:
	case ST_22_SPLITTING_DOOR:
	case ST_23_SWINGING_DOOR:
	case ST_25_SLIDING_DOOR:
	case ST_26_SPLITTING_ST_DOOR:
	case ST_29_TEETH_DOOR:
		return true;
	case 41:
		return isRR();
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int findplayer(const DDukeActor* actor, double* d)
{
	int j, closest_player;
	const auto s = actor->spr.pos;

	if (ud.multimode < 2)
	{
		if (d) *d = (getPlayer(myconnectindex)->GetActor()->getPrevPosWithOffsetZ() - s).plusZ(28).Sum();
		return myconnectindex;
	}

	double closest = 0x7fffffff;
	closest_player = 0;

	for (j = connecthead; j >= 0; j = connectpoint2[j])
	{
		const auto jact = getPlayer(j)->GetActor();
		double x = (jact->getPrevPosWithOffsetZ() - s).plusZ(28).Sum();
		if (x < closest && jact->spr.extra > 0)
		{
			closest_player = j;
			closest = x;
		}
	}

	if (d) *d = closest;
	return closest_player;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int findotherplayer(int p, double* d)
{
	const auto ppos = getPlayer(p)->GetActor()->getPosWithOffsetZ();
	int j, closest_player;

	double closest = 0x7fffffff;
	closest_player = p;

	for (j = connecthead; j >= 0; j = connectpoint2[j])
	{
		const auto jact = getPlayer(j)->GetActor();

		if (p != j && jact->spr.extra > 0)
		{
			double x = (jact->getPrevPosWithOffsetZ() - ppos).Sum();

			if (x < closest)
			{
				closest_player = j;
				closest = x;
			}
		}
	}

	*d = closest;
	return closest_player;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

double getanimatevalue(int type, int index)
{
	switch (type)
	{
	case anim_floorz:
		return sector[index].floorz;
	case anim_ceilingz:
		return sector[index].ceilingz;
	case anim_vertexx:
		return wall[index].pos.X;
	case anim_vertexy:
		return wall[index].pos.Y;
	default:
		assert(false);
		return 0;
	}
}

double getanimatevalue(int i)
{
	return getanimatevalue(animates[i].type, animates[i].target);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void setanimatevalue(int type, int index, double value)
{
	static int scratch;
	switch (type)
	{
	case anim_floorz:
		sector[index].setfloorz(value);
		break;
	case anim_ceilingz:
		sector[index].setceilingz(value);
		break;
	case anim_vertexx:

		wall[index].pos.X = value;
		wall[index].moved();
		break;
	case anim_vertexy:

		wall[index].pos.Y = value;
		wall[index].moved();
		break;
	default:
		assert(false);
	}
}

void setanimatevalue(int i, double value)
{
	return setanimatevalue(animates[i].type, animates[i].target, value);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void doanimations(void)
{
	for (int i = animates.Size() - 1; i >= 0; i--)
	{
		double a = getanimatevalue(i);
		double const v = animates[i].vel * TICSPERFRAME;
		auto dasectp = animates[i].sect;
		int type = animates[i].type;

		if (a == animates[i].goal)
		{
			StopInterpolation(animates[i].target, interptype[animates[i].type]);

			animates[i] = animates.Last();
			animates.Pop();
			if (dasectp->lotag == ST_18_ELEVATOR_DOWN || dasectp->lotag == ST_19_ELEVATOR_UP)
				if (type == anim_ceilingz)
					continue;

			if ((dasectp->lotag & 0xff) != ST_22_SPLITTING_DOOR)
				callsound(dasectp, nullptr, true);

			continue;
		}

		if (v > 0) { a = min(a + v, animates[i].goal); }
		else { a = max(a + v, animates[i].goal); }

		if (type == anim_floorz)
		{
			for (auto j = connecthead; j >= 0; j = connectpoint2[j])
			{
				const auto p = getPlayer(j);
				const auto pact = p->GetActor();

				if ((p->cursector == dasectp) && ((dasectp->floorz - pact->getOffsetZ()) < 64) && (pact->GetOwner() != nullptr))
				{
					pact->spr.pos.Z += v;
					p->vel.Z = 0;
				}
			}

			DukeSectIterator it(dasectp);
			while (auto act = it.Next())
			{
				if (act->spr.statnum != STAT_EFFECTOR)
				{
					if (act->spr.statnum != STAT_PLAYER)
					{
						act->backupz();
						act->spr.pos.Z += v;
					}
					act->floorz = dasectp->floorz + v;
				}
			}
		}
		setanimatevalue(i, a);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int getanimationindex(int animtype, sectortype* animtargetp)
{
	int i, j;

	j = -1;
	int animtarget = sectindex(animtargetp);
	for (i = animates.Size() - 1; i >= 0; i--)
		if (animtype == animates[i].type && animtarget == animates[i].target)
		{
			j = i;
			break;
		}
	return(j);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static int dosetanimation(sectortype* animsect, int animtype, int animtarget, double thegoal, double thevel)
{
	int j = -1;

	for (unsigned i = 0; i < animates.Size(); i++)
		if (animtype == animates[i].type && animtarget == animates[i].target)
		{
			j = i;
			break;
		}

	if (j == -1) j = animates.Reserve(1);

	auto animval = getanimatevalue(animtype, animtarget);
	animates[j].sect = animsect;
	animates[j].type = animtype;
	animates[j].target = animtarget;
	animates[j].goal = thegoal;
	if (thegoal >= animval)
		animates[j].vel = thevel;
	else
		animates[j].vel = -thevel;

	StartInterpolation(animates[j].target, interptype[animates[j].type]);
	return(j);
}

int setanimation(sectortype* animsect, int animtype, walltype* animtarget, double thegoal, double thevel)
{
	assert(animtype == anim_vertexx || animtype == anim_vertexy);
	return dosetanimation(animsect, animtype, wallindex(animtarget), thegoal, thevel);
}

int setanimation(sectortype* animsect, int animtype, sectortype* animtarget, double thegoal, double thevel)
{
	assert(animtype == anim_ceilingz || animtype == anim_floorz);
	return dosetanimation(animsect, animtype, sectindex(animtarget), thegoal, thevel);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool activatewarpelevators(DDukeActor* actor, int d) //Parm = sectoreffectornum
{
	auto sect = actor->sector();

	// See if the sector exists

	DukeStatIterator it(STAT_EFFECTOR);
	DDukeActor *act2;
	while ((act2 = it.Next()))
	{
		if (act2->spr.lotag == SE_17_WARP_ELEVATOR || (isRRRA() && act2->spr.lotag == SE_18_INCREMENTAL_SECTOR_RISE_FALL))
			if (act2->spr.hitag == actor->spr.hitag)
				if ((abs(sect->floorz - actor->temp_pos.X) > act2->spr.yint * maptoworld) ||
					(act2->sector()->hitag == (sect->hitag - d)))
					break;
	}

	if (act2 == nullptr)
	{
		d = 0;
		return 1; // No find
	}
	else
	{
		if (d == 0)
			S_PlayActorSound(ELEVATOR_OFF, actor);
		else S_PlayActorSound(ELEVATOR_ON, actor);
	}


	it.Reset(STAT_EFFECTOR);
	while ((act2 = it.Next()))
	{
		if (act2->spr.lotag == SE_17_WARP_ELEVATOR || (isRRRA() && act2->spr.lotag == SE_18_INCREMENTAL_SECTOR_RISE_FALL))
			if (act2->spr.hitag == actor->spr.hitag)
			{
				act2->counter = d;
				if (act2->spr.lotag == SE_17_WARP_ELEVATOR) act2->temp_data[1] = d; //Make all check warp (only SE17, in SE18 this is a coordinate)
			}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st09(sectortype* sptr, DDukeActor* actor)
{
	double dax, day, dax2, day2, sp;
	walltype* wallfind[2];

	sp = (sptr->extra >> 4) / 16.;

	//first find center point by averaging all points
	dax = 0, day = 0;
	for (auto& wal : sptr->walls)
	{
		dax += wal.pos.X;
		day += wal.pos.Y;
	}
	dax /= sptr->walls.Size();
	day /= sptr->walls.Size();

	//find any points with either same x or same y coordinate
	//  as center (dax, day) - should be 2 points found.
	wallfind[0] = nullptr;
	wallfind[1] = nullptr;
	for (auto& wal : sptr->walls)
		// more precise checks won't work here.
		if (abs(wal.pos.X - dax) <= (1 / 32.) || abs(wal.pos.Y - day) <= (1 / 32.))
		{
			if (wallfind[0] == nullptr)
				wallfind[0] = &wal;
			else wallfind[1] = &wal;
		}

	for (int j = 0; j < 2; j++)
	{
		auto wal = wallfind[j];

		//find what direction door should open by averaging the
		//  2 neighboring points of wallfind[0] & wallfind[1].
		auto prevwall = wal - 1;
		if (prevwall < sptr->walls.Data()) prevwall += sptr->walls.Size();

		if ((wal->pos.X == dax) && (wal->pos.Y == day))
		{
			dax2 = ((prevwall->pos.X + wal->point2Wall()->pos.X) * 0.5) - wal->pos.X;
			day2 = ((prevwall->pos.Y + wal->point2Wall()->pos.Y) * 0.5) - wal->pos.Y;
			if (dax2 != 0)
			{
				dax2 = wal->point2Wall()->point2Wall()->pos.X;
				dax2 -= wal->point2Wall()->pos.X;
				setanimation(sptr, anim_vertexx, wal, wal->pos.X + dax2, sp);
				setanimation(sptr, anim_vertexx, prevwall, prevwall->pos.X + dax2, sp);
				setanimation(sptr, anim_vertexx, wal->point2Wall(), wal->point2Wall()->pos.X + dax2, sp);
				callsound(sptr, actor);
			}
			else if (day2 != 0)
			{
				day2 = wal->point2Wall()->point2Wall()->pos.Y;
				day2 -= wal->point2Wall()->pos.Y;
				setanimation(sptr, anim_vertexy, wal, wal->pos.Y + day2, sp);
				setanimation(sptr, anim_vertexy, prevwall, prevwall->pos.Y + day2, sp);
				setanimation(sptr, anim_vertexy, wal->point2Wall(), wal->point2Wall()->pos.Y + day2, sp);
				callsound(sptr, actor);
			}
		}
		else
		{
			dax2 = ((prevwall->pos.X + wal->point2Wall()->pos.X) * 0.5) - wal->pos.X;
			day2 = ((prevwall->pos.Y + wal->point2Wall()->pos.Y) * 0.5) - wal->pos.Y;
			if (dax2 != 0)
			{
				setanimation(sptr, anim_vertexx, wal, dax, sp);
				setanimation(sptr, anim_vertexx, prevwall, dax + dax2, sp);
				setanimation(sptr, anim_vertexx, wal->point2Wall(), dax + dax2, sp);
				callsound(sptr, actor);
			}
			else if (day2 != 0)
			{
				setanimation(sptr, anim_vertexy, wal, day, sp);
				setanimation(sptr, anim_vertexy, prevwall, day + day2, sp);
				setanimation(sptr, anim_vertexy, wal->point2Wall(), day + day2, sp);
				callsound(sptr, actor);
			}
		}
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st15(sectortype* sptr, DDukeActor* actor)
{
	if (!actor->isPlayer()) return;

	DukeSectIterator it(sptr);
	DDukeActor* a2;
	while ((a2 = it.Next()))
	{
		if (iseffector(a2) && a2->spr.lotag == ST_17_PLATFORM_UP) break;
	}
	if (!a2) return;

	if (actor->sector() == sptr)
	{
		if (activatewarpelevators(a2, -1))
			activatewarpelevators(a2, 1);
		else if (activatewarpelevators(a2, 1))
			activatewarpelevators(a2, -1);
		return;
	}
	else
	{
		if (sptr->floorz > a2->spr.pos.Z)
			activatewarpelevators(a2, -1);
		else
			activatewarpelevators(a2, 1);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st16(sectortype* sptr, DDukeActor* actor)
{
	int i = getanimationindex(anim_floorz, sptr);
	sectortype* sectp;

	if (i == -1)
	{
		sectp = nextsectorneighborzptr(sptr, sptr->floorz, Find_FloorDown);
		if (sectp == nullptr)
		{
			sectp = nextsectorneighborzptr(sptr, sptr->floorz, Find_FloorUp);
			if (sectp == nullptr) return;
		}
		setanimation(sptr, anim_floorz, sptr, sectp->floorz, sptr->extra / 256.);
		callsound(sptr, actor);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st18(sectortype* sptr, DDukeActor* actor)
{
	int i = getanimationindex(anim_floorz, sptr);

	if (i == -1)
	{
		auto sectp = nextsectorneighborzptr(sptr, sptr->floorz, Find_FloorUp);
		if (sectp == nullptr) sectp = nextsectorneighborzptr(sptr, sptr->floorz, Find_FloorDown);
		if (sectp == nullptr) return;
		double speed = sptr->extra / 256.;
		setanimation(sptr, anim_floorz, sptr, sectp->floorz, speed);
		setanimation(sptr, anim_ceilingz, sptr, sectp->floorz + sptr->ceilingz - sptr->floorz, speed);
		callsound(sptr, actor);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st29(sectortype* sptr, DDukeActor* actor)
{
	double j;

	if (sptr->lotag & 0x8000)
		j = nextsectorneighborzptr(sptr, sptr->ceilingz, Find_FloorDown | Find_Safe)->floorz;
	else
		j = nextsectorneighborzptr(sptr, sptr->ceilingz, Find_CeilingUp | Find_Safe)->ceilingz;

	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act2 = it.Next())
	{
		if ((act2->spr.lotag == 22) &&
			(act2->spr.hitag == sptr->hitag))
		{
			act2->sector()->extra = -act2->sector()->extra;

			act2->counter = sectindex(sptr);
			act2->temp_data[1] = 1;
		}
	}

	sptr->lotag ^= 0x8000;

	setanimation(sptr, anim_ceilingz, sptr, j, sptr->extra / 256.);

	callsound(sptr, actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st20(sectortype* sptr, DDukeActor* actor)
{
	double j = 0;
REDODOOR:

	if (sptr->lotag & 0x8000)
	{
		DDukeActor* a2;
		DukeSectIterator it(sptr);
		while ((a2 = it.Next()))
		{
			if (a2->spr.statnum == STAT_EFFECTOR && a2->spr.lotag == SE_9_DOWN_OPEN_DOOR_LIGHTS)
			{
				j = a2->spr.pos.Z;
				break;
			}
		}
		if (a2 == nullptr)
			j = sptr->floorz;
	}
	else
	{
		auto sectp = nextsectorneighborzptr(sptr, sptr->ceilingz, Find_CeilingUp);

		if (sectp) j = sectp->ceilingz;
		else
		{
			sptr->lotag |= 32768;
			goto REDODOOR;
		}
	}

	sptr->lotag ^= 0x8000;

	setanimation(sptr, anim_ceilingz, sptr, j, sptr->extra / 256.);
	callsound(sptr, actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st21(sectortype* sptr, DDukeActor* actor)
{
	int i = getanimationindex(anim_floorz, sptr);
	double j;
	if (i >= 0)
	{
		if (animates[i].goal == sptr->ceilingz)
			animates[i].goal = nextsectorneighborzptr(sptr, sptr->ceilingz, Find_FloorDown | Find_Safe)->floorz;
		else animates[i].goal = sptr->ceilingz;
		j = animates[i].goal;
	}
	else
	{
		if (sptr->ceilingz == sptr->floorz)
			j = nextsectorneighborzptr(sptr, sptr->ceilingz, Find_FloorDown | Find_Safe)->floorz;
		else j = sptr->ceilingz;

		sptr->lotag ^= 0x8000;

		if (setanimation(sptr, anim_floorz, sptr, j, sptr->extra / 256.) >= 0)
			callsound(sptr, actor);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st22(sectortype* sptr, DDukeActor* actor)
{
	int j;
	double z;
	double speed = sptr->extra / 256.;
	if ((sptr->lotag & 0x8000))
	{
		z = (sptr->ceilingz + sptr->floorz) * 0.5;
		j = setanimation(sptr, anim_floorz, sptr, z, speed);
		j = setanimation(sptr, anim_ceilingz, sptr, z, speed);
	}
	else
	{
		z = nextsectorneighborzptr(sptr, sptr->floorz, Find_FloorDown | Find_Safe)->floorz;
		j = setanimation(sptr, anim_floorz, sptr, z, speed);
		z = nextsectorneighborzptr(sptr, sptr->ceilingz, Find_CeilingUp | Find_Safe)->ceilingz;
		j = setanimation(sptr, anim_ceilingz, sptr, z, speed);
	}

	sptr->lotag ^= 0x8000;

	callsound(sptr, actor);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st23(sectortype* sptr, DDukeActor* actor)
{
	int q = 0;

	DukeStatIterator it(STAT_EFFECTOR);
	DDukeActor* act2;
	while ((act2 = it.Next()))
	{
		if (act2->spr.lotag == SE_11_SWINGING_DOOR && act2->sector() == sptr && !act2->temp_data[4])
		{
			break;
		}
	}
	if (!act2) return;

	int l = act2->sector()->lotag & 0x8000;

	if (act2)
	{
		DukeStatIterator itr(STAT_EFFECTOR);

		while (auto act3 = itr.Next())
		{
			if (l == (act3->sector()->lotag & 0x8000) && act3->spr.lotag == SE_11_SWINGING_DOOR && act2->spr.hitag == act3->spr.hitag && act3->temp_data[4])
			{
				return;
			}
		}

		itr.Reset(STAT_EFFECTOR);
		while (auto act3 = itr.Next())
		{
			if (l == (act3->sector()->lotag & 0x8000) && act3->spr.lotag == SE_11_SWINGING_DOOR && act2->spr.hitag == act3->spr.hitag)
			{
				if (act3->sector()->lotag & 0x8000) act3->sector()->lotag &= 0x7fff;
				else act3->sector()->lotag |= 0x8000;
				act3->temp_data[4] = 1;
				act3->temp_data[3] = -act3->temp_data[3];
				if (q == 0)
				{
					callsound(sptr, act3);
					q = 1;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st25(sectortype* sptr, DDukeActor* actor)
{
	DukeStatIterator it(STAT_EFFECTOR);
	DDukeActor* act2;
	while ((act2 = it.Next()))
	{
		if (act2->spr.lotag == 15 && act2->sector() == sptr)
		{
			break;
		}
	}

	if (act2 == nullptr)
		return;

	it.Reset(STAT_EFFECTOR);
	while (auto act3 = it.Next())
	{
		if (act3->spr.hitag == act2->spr.hitag)
		{
			if (act3->spr.lotag == 15)
			{
				act3->sector()->lotag ^= 0x8000; // Toggle the open or close
				act3->spr.Angles.Yaw += DAngle180;
				if (act3->temp_data[4]) callsound(act3->sector(), act3);
				callsound(act3->sector(), act3);
				if (act3->sector()->lotag & 0x8000) act3->temp_data[4] = 1;
				else act3->temp_data[4] = 2;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st27(sectortype* sptr, DDukeActor* actor)
{
	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act2 = it.Next())
	{
		if ((act2->spr.lotag & 0xff) == 20 && act2->sector() == sptr) //Bridge
		{

			sptr->lotag ^= 0x8000;
			if (sptr->lotag & 0x8000) //OPENING
				act2->counter = 1;
			else act2->counter = 2;
			callsound(sptr, actor);
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st28(sectortype* sptr, DDukeActor* actor)
{
	//activate the rest of them
	int j = -1;
	DukeSectIterator it(sptr);
	while (auto a2 = it.Next())
	{
		if (a2->spr.statnum == 3 && (a2->spr.lotag & 0xff) == 21)
		{
			j = a2->spr.hitag;
			break; //Found it
		}
	}

	if (j == -1) return;
	DukeStatIterator it1(STAT_EFFECTOR);
	while (auto act3 = it.Next())
	{
		if ((act3->spr.lotag & 0xff) == 21 && !act3->counter &&
			(act3->spr.hitag) == j)
			act3->counter = 1;
	}
	callsound(sptr, actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operatesectors(sectortype* sptr, DDukeActor *actor)
{
	int j=0;
	int i;

	switch (sptr->lotag & (0x3fff))
	{

	case 41:
		if (isRR()) operatejaildoors(sptr->hitag);
		break;

	case 7:
		if (!isRR()) break;
		for (auto& wal : sptr->walls)
		{
			setanimation(sptr, anim_vertexx, &wal, wal.pos.X + 64, 1 / 4.);
			if (wal.twoSided()) setanimation(sptr, anim_vertexx, wal.nextWall(), wal.nextWall()->pos.X + 64, 1 / 4.);
		}
		break;

	case ST_30_ROTATE_RISE_BRIDGE:
	{
		auto act = barrier_cast<DDukeActor*>(sptr->hitagactor);
		if (!act) break;
		if (act->tempval == 0 || act->tempval == 256) callsound(sptr, actor);
		if (act->spr.extra == 1) act->spr.extra = 3;
		else act->spr.extra = 1;
		break;
	}

	case ST_31_TWO_WAY_TRAIN:
	{
		auto act = barrier_cast<DDukeActor*>(sptr->hitagactor);
		if (!act) break;
		if (act->temp_data[4] == 0)
			act->temp_data[4] = 1;

		callsound(sptr, actor);
		break;
	}
	case ST_26_SPLITTING_ST_DOOR: //The split doors
		i = getanimationindex(anim_ceilingz, sptr);
		if (i == -1) //if the door has stopped
		{
			haltsoundhack = 1;
			sptr->lotag &= 0xff00;
			sptr->lotag |= ST_22_SPLITTING_DOOR;
			operatesectors(sptr, actor);
			sptr->lotag &= 0xff00;
			sptr->lotag |= ST_9_SLIDING_ST_DOOR;
			operatesectors(sptr, actor);
			sptr->lotag &= 0xff00;
			sptr->lotag |= ST_26_SPLITTING_ST_DOOR;
		}
		return;

	case ST_9_SLIDING_ST_DOOR:
		handle_st09(sptr, actor);
		return;

	case ST_15_WARP_ELEVATOR://Warping elevators
		handle_st15(sptr, actor);
		return;

	case ST_16_PLATFORM_DOWN:
	case ST_17_PLATFORM_UP:
		handle_st16(sptr, actor);
		return;

	case ST_18_ELEVATOR_DOWN:
	case ST_19_ELEVATOR_UP:
		handle_st18(sptr, actor);
		return;

	case ST_29_TEETH_DOOR:
		handle_st29(sptr, actor);
		return;
	case ST_20_CEILING_DOOR:
		handle_st20(sptr, actor);
		return;

	case ST_21_FLOOR_DOOR:
		handle_st21(sptr, actor);
		return;

	case ST_22_SPLITTING_DOOR:
		handle_st22(sptr, actor);
		return;

	case ST_23_SWINGING_DOOR: //Swingdoor
		handle_st23(sptr, actor);
		return;

	case ST_25_SLIDING_DOOR: //Subway type sliding doors
	{
		handle_st25(sptr, actor);
		return;
	}
	case ST_27_STRETCH_BRIDGE:  //Extended bridge
		handle_st27(sptr, actor);
		return;

	case ST_28_DROP_FLOOR:
		handle_st28(sptr, actor);
		return;
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operateactivators(int low, DDukePlayer* plr)
{
	int i, j, k;
	Cycler * p;
	walltype* wal;

	for (i = cyclers.Size() - 1; i >= 0; i--)
	{
		p = &cyclers[i];


		if (p->hitag == low)
		{
			auto sect = p->sector;
			p->state = !p->state;

			sect->floorshade = sect->ceilingshade = (int8_t)p->shade2;
			wal = sect->walls.Data();
			for (j = sect->walls.Size(); j > 0; j--, wal++)
				wal->shade = (int8_t)p->shade2;
		}
	}

	k = -1;
	DukeStatIterator it(STAT_ACTIVATOR);
	while (auto act = it.Next())
	{
		if (act->spr.lotag == low)
		{
			if (islockedactivator(act))
			{
				act->sector()->lotag ^= 16384;

				if (plr)
				{
					if (act->sector()->lotag & 16384)
						FTA(4, plr);
					else FTA(8, plr);
				}
			}
			else
			{
				switch (act->spr.hitag)
				{
				case 0:
					break;
				case 1:
					if (act->sector()->floorz != act->sector()->ceilingz)
					{
						continue;
					}
					break;
				case 2:
					if (act->sector()->floorz == act->sector()->ceilingz)
					{
						continue;
					}
					break;
				}

				if (act->sector()->lotag < 3)
				{
					DukeSectIterator itr(act->sector());
					while (auto a2 = itr.Next())
					{
						if (a2->spr.statnum == 3) switch (a2->spr.lotag)
						{
						case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
							if (isRRRA()) break;
							[[fallthrough]];
						case SE_36_PROJ_SHOOTER:
						case SE_31_FLOOR_RISE_FALL:
						case SE_32_CEILING_RISE_FALL:
							a2->counter = 1 - a2->counter;
							callsound(act->sector(), a2);
							break;
						}
					}
				}

				if (k == -1 && (act->sector()->lotag & 0xff) == SE_22_TEETH_DOOR)
					k = callsound(act->sector(), act);

				operatesectors(act->sector(), act);
			}
		}
	}

	operaterespawns(low);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operatemasterswitches(int low)
{
	DukeStatIterator it(STAT_STANDABLE);
	while (auto act2 = it.Next())
	{
		if (ismasterswitch(act2) && act2->spr.lotag == low && act2->spr.yint == 0)
		{
			act2->spr.yint = 1;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operateforcefields(DDukeActor *effector, int low)
{
	for (int p = numanimwalls-1; p >= 0; p--)
	{
		auto wal = animwall[p].wall;

		if (low == wal->lotag || low == -1)
			if (tileflags(wal->overtexture) & (TFLAG_FORCEFIELD | TFLAG_ANIMFORCEFIELD))
			{
				animwall[p].tag = 0;

				if (wal->cstat)
				{
					wal->cstat = 0;

					if (effector && iseffector(effector) && effector->spr.lotag == SE_30_TWO_WAY_TRAIN)
						wal->lotag = 0;
				}
				else
					wal->cstat = (CSTAT_WALL_BLOCK | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_MASKED | CSTAT_WALL_BLOCK_HITSCAN);
			}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkhitwall(DDukeActor* spr, walltype* wal, const DVector3& pos)
{
	if (wal->overtexture == mirrortex && (spr->flags2 & SFLAG2_BREAKMIRRORS))
	{
		lotsofglass(spr, wal, 70);
		wal->cstat &= ~CSTAT_WALL_MASKED;
		wal->setovertexture(TexMan.CheckForTexture("MIRRORBROKE", ETextureType::Any));
		wal->portalflags = 0;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	}

	auto handler = [=](const BreakWallRec* data) ->bool
	{
		if (!data->handler)
		{
			S_PlayActorSound(data->breaksound, spr);
			return true;
		}
		else
		{
			VMValue args[7] = { wal, data->brokentex.GetIndex(), data->breaksound.index(), spr, pos.X, pos.Y, pos.Z};
			VMCall(data->handler, args, 7, nullptr, 0);
		}
		return false;
	};


	if (wal->twoSided() && wal->nextSector()->floorz > pos.Z && wal->nextSector()->floorz - wal->nextSector()->ceilingz)
	{
		auto data = breakWallMap.CheckKey(wal->overtexture.GetIndex());
		if (data && (data->flags & 1) && (!(data->flags & 2) || wal->cstat & CSTAT_WALL_MASKED))
		{
			if (handler(data)) wal->setovertexture(data->brokentex);
		}
	}

	auto data = breakWallMap.CheckKey(wal->walltexture.GetIndex());
	if (data && !(data->flags & 1))
	{
		if (handler(data)) wal->setwalltexture(data->brokentex);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool checkhitceiling(sectortype* sectp)
{
	auto data = breakCeilingMap.CheckKey(sectp->ceilingtexture.GetIndex());
	if (data && !(data->flags & 1))
	{
		if (!data->handler)
		{
			sectp->setceilingtexture(data->brokentex);
			S_PlayActorSound(data->breaksound, getPlayer(screenpeek)->GetActor());	// this is nonsense but what the original code did.
		}
		else
		{
			VMValue args[7] = { sectp, data->brokentex.GetIndex(), data->breaksound.index()};
			VMCall(data->handler, args, 3, nullptr, 0);
		}
		if (data->flags & 1)
		{
			if (!sectp->hitag)
			{
				DukeSectIterator it(sectp);
				while (auto act = it.Next())
				{
					if (iseffector(act) && act->spr.lotag == SE_12_LIGHT_SWITCH)
					{
						DukeStatIterator it1(STAT_EFFECTOR);
						while (auto act2 = it1.Next())
						{
							if (act2->spr.hitag == act->spr.hitag)
								act2->temp_data[3] = 1;
						}
						break;
					}
				}
			}

			int j = krand() & 1;
			DukeStatIterator it(STAT_EFFECTOR);
			while (auto act = it.Next())
			{
				if (act->spr.hitag == (sectp->hitag) && act->spr.lotag == SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT)
				{
					act->temp_data[2] = j;
					act->temp_data[4] = 1;
				}
			}
		}
		if (data->flags & 2)
		{
			ceilingglass(getPlayer(myconnectindex)->GetActor(), sectp, 10);
		}
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkhitdefault(DDukeActor* targ, DDukeActor* proj)
{
	if ((targ->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && targ->spr.hitag == 0 && targ->spr.lotag == 0 && targ->spr.statnum == STAT_DEFAULT)
		return;

	if (((proj->flags3 & SFLAG3_CANHURTSHOOTER) || proj->GetOwner() != targ) && targ->spr.statnum != STAT_PROJECTILE)
	{
		if (badguy(targ))
		{
			if (targ->spr.scale.X < targ->FloatVar(NAME_minhitscale))
				return;

			if (proj->flags4 & SFLAG4_DOUBLEHITDAMAGE) proj->spr.extra <<= 1;

			if (!(targ->flags3 & SFLAG3_NOHITJIBS) && !(proj->flags3 & SFLAG3_NOHITJIBS))
			{
				auto spawned = spawn(proj, DukeJibs6Class);
				if (spawned)
				{
					if (proj->spr.pal == 6)
						spawned->spr.pal = 6;
					spawned->spr.pos.Z += 4;
					spawned->vel.X = 1;
					spawned->spr.scale = DVector2(0.375, 0.375);
					spawned->spr.Angles.Yaw = DAngle22_5 / 4 - randomAngle(22.5 / 2);
				}
			}

			auto Owner = proj->GetOwner();

			if (Owner && Owner->isPlayer() && !(targ->flags3 & SFLAG3_NOSHOTGUNBLOOD))
				if (getPlayer(Owner->PlayerIndex())->curr_weapon == SHOTGUN_WEAPON)
				{
					shoot(targ, DukeBloodSplat3Class);
					shoot(targ, DukeBloodSplat1Class);
					shoot(targ, DukeBloodSplat2Class);
					shoot(targ, DukeBloodSplat4Class);
				}

			if (!(targ->flags2 & SFLAG2_NODAMAGEPUSH))
			{
				if ((targ->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == 0)
					targ->spr.Angles.Yaw = proj->spr.Angles.Yaw + DAngle180;

				targ->vel.X = -proj->spr.extra * 0.25;
				auto sp = targ->sector();
				pushmove(targ->spr.pos, &sp, 8, 4, 4, CLIPMASK0);
				if (sp != targ->sector() && sp != nullptr)
					ChangeActorSect(targ, sp);
			}

			if (targ->spr.statnum == STAT_ZOMBIEACTOR)
			{
				ChangeActorStat(targ, STAT_ACTOR);
				targ->timetosleep = SLEEPTIME;
			}
			if (!shrinkersizecheck(proj->GetClass(), targ)) return;
		}

		if (targ->spr.statnum != STAT_ZOMBIEACTOR)
		{
			if ((proj->flags2 & SFLAG2_FREEZEDAMAGE) && ((targ->isPlayer() && targ->spr.pal == 1) || (gs.freezerhurtowner == 0 && proj->GetOwner() == targ)))
				return;

			auto hitpic = static_cast<PClassActor*>(proj->GetClass());
			auto Owner = proj->GetOwner();
			if (Owner && Owner->isPlayer())
			{
				if (targ->isPlayer() && ud.coop != 0 && ud.ffire == 0)
					return;

				auto tOwner = targ->GetOwner();
				if (hitpic == DukeFireballClass && tOwner && tOwner->GetClass() != DukeFireballClass) // hack alert! Even with damage types this special check needs to stay.
					hitpic = DukeFlamethrowerFlameClass;
			}

			targ->attackertype = hitpic;
			targ->hitextra += proj->spr.extra;
			if (!(targ->flags4 & SFLAG4_NODAMAGETURN))
				targ->hitang = proj->spr.Angles.Yaw;
			targ->SetHitOwner(Owner);
		}

		if (targ->spr.statnum == STAT_PLAYER)
		{
			const auto p = getPlayer(targ->PlayerIndex());
			const auto pact = p->GetActor();

			if (p->newOwner != nullptr)
			{
				p->newOwner = nullptr;
				pact->restoreloc();

				updatesector(pact->getPosWithOffsetZ(), &p->cursector);

				DukeStatIterator it(STAT_ACTOR);
				while (auto itActor = it.Next())
				{
					if (itActor->flags2 & SFLAG2_CAMERA) itActor->spr.yint = 0;
				}
			}

			if (!shrinkersizecheck(proj->GetClass(), targ))
				return;

			auto hitowner = targ->GetHitOwner();
			if (!hitowner || !hitowner->isPlayer())
				if (ud.player_skill >= 3)
					proj->spr.extra += (proj->spr.extra >> 1);
		}

	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void allignwarpelevators(void)
{
	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act = it.Next())
	{
		if (act->spr.lotag == SE_17_WARP_ELEVATOR && act->spr.shade > 16)
		{
			DukeStatIterator it1(STAT_EFFECTOR);
			while (auto act2 = it1.Next())
			{
				if ((act2->spr.lotag) == SE_17_WARP_ELEVATOR && act != act2 && act->spr.hitag == act2->spr.hitag)
				{
					act2->sector()->setfloorz(act->sector()->floorz);
					act2->sector()->setceilingz(act->sector()->ceilingz);
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveclouds(double interpfrac)
{
	// The math here is very messy.. :(
	int myclock = interpfrac < 0.5 ? PlayClock-2 : PlayClock;
	if (myclock > cloudclock || myclock < (cloudclock - 7))
	{
		const auto pact = getPlayer(screenpeek)->GetActor();
		cloudclock = myclock + 6;

		// cloudx/y were an array, but all entries were always having the same value so a single pair is enough.
		cloudx += (float)pact->spr.Angles.Yaw.Cos() * 0.5f;
		cloudy += (float)pact->spr.Angles.Yaw.Sin() * 0.5f;
		for (int i = 0; i < numclouds; i++)
		{
			// no clamping here!
			clouds[i]->ceilingxpan_ = cloudx;
			clouds[i]->ceilingypan_ = cloudy;
			clouds[i]->exflags |= SECTOREX_CLOUDSCROLL;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetswitch(int tag)
{
	DukeStatIterator it2(STAT_DEFAULT);
	while (auto act2 = it2.Next())
	{
		auto& ext = GetExtInfo(act2->spr.spritetexture());
		if (ext.switchindex > 0 && ext.switchphase == 1 && act2->spr.hitag == tag)
		{
			auto& swdef = switches[ext.switchindex];
			if (swdef.type == SwitchDef::Regular && swdef.flags & SwitchDef::resettable)
			{
				act2->spr.setspritetexture(swdef.states[0]);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void tag10000specialswitch(DDukePlayer* const p, DDukeActor* act, const DVector3& v)
{
	DDukeActor* switches[3];
	int switchcount = 0, j;
	S_PlaySound3D(SWITCH_ON, act, v);
	DukeSpriteIterator itr;
	while (auto actt = itr.Next())
	{
		int jht = actt->spr.hitag;
		auto ext = GetExtInfo(actt->spr.spritetexture());
		if (jht == 10000 && ext.switchphase == 0 && ::switches[ext.switchindex].type == SwitchDef::Multi)
		{
			if (switchcount < 3)
			{
				switches[switchcount] = actt;
				switchcount++;
			}
		}
	}
	if (switchcount == 3)
	{
		// This once was a linear search over sprites[] so bring things back in order, just to be safe.
		if (switches[0]->GetIndex() > switches[1]->GetIndex()) std::swap(switches[0], switches[1]);
		if (switches[0]->GetIndex() > switches[2]->GetIndex()) std::swap(switches[0], switches[2]);
		if (switches[1]->GetIndex() > switches[2]->GetIndex()) std::swap(switches[1], switches[2]);

		S_PlaySound3D(78, act, v);
		for (j = 0; j < switchcount; j++)
		{
			switches[j]->spr.hitag = 0;
			switches[j]->spr.setspritetexture(::switches[GetExtInfo(switches[j]->spr.spritetexture()).switchindex].states[3]);
			checkhitswitch(p, nullptr, switches[j]);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void togglespriteswitches(DDukeActor* act, const TexExtInfo& ext, int lotag, int& correctdips, int& numdips)
{
	auto& swdef = switches[ext.switchindex];

	DukeStatIterator it(STAT_DEFAULT);
	while (auto other = it.Next())
	{
		if (lotag != other->spr.lotag) continue;

		auto& other_ext = GetExtInfo(other->spr.spritetexture());
		auto& other_swdef = switches[other_ext.switchindex];

		switch (other_swdef.type)
		{
		case SwitchDef::Combo:
			if (other_ext.switchphase == 0)
			{
				if (act == other) other->spr.setspritetexture(other_swdef.states[1]);
				else if (other->spr.hitag == 0) correctdips++;
				numdips++;
			}
			else
			{
				if (act == other) other->spr.setspritetexture(other_swdef.states[0]);
				else if (other->spr.hitag == 1) correctdips++;
				numdips++;
			}
			break;

		case SwitchDef::Multi:
			other->spr.setspritetexture(other_swdef.states[(other_ext.switchphase + 1) & 3]);
			break;

		case SwitchDef::Access:
		case SwitchDef::Regular:
			if (other->spr.hitag != 999 || other_ext.switchphase != 1 || !(other_swdef.flags & SwitchDef::resettable))
			{
				other->spr.setspritetexture(other_swdef.states[1 - other_ext.switchphase]);
			}
			// one of RR's ugly hacks.
			if (other->spr.hitag == 999 && other_ext.switchphase == 0 && (other_swdef.flags & SwitchDef::resettable))
			{
				DukeStatIterator it1(STAT_LUMBERMILL);
				while (auto other2 = it1.Next())
				{
					CallOnUse(other2, nullptr);
				}
			}
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void togglewallswitches(walltype* wwal, const TexExtInfo& ext, int lotag, int& correctdips, int& numdips)
{
	for (auto& wal : wall)
	{
		if (lotag != wal.lotag) continue;

		auto& other_ext = GetExtInfo(wal.walltexture);
		auto& other_swdef = switches[other_ext.switchindex];

		switch (other_swdef.type)
		{
		case SwitchDef::Combo:
			if (other_ext.switchphase == 0)
			{
				if (&wal == wwal) wal.setwalltexture(other_swdef.states[1]);
				else if (wal.hitag == 0) correctdips++;
				numdips++;
			}
			else
			{
				if (&wal == wwal) wal.setwalltexture(other_swdef.states[0]);
				else if (wal.hitag == 1) correctdips++;
				numdips++;
			}
			break;

		case SwitchDef::Multi:
			wal.setwalltexture(other_swdef.states[(other_ext.switchphase + 1) & 3]);
			break;

		case SwitchDef::Access:
		case SwitchDef::Regular:
			wal.setwalltexture(other_swdef.states[1 - other_ext.switchphase]);
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
// how NOT to implement switches...
// (even after cleaning up the hard coded texture checks it's still a disaster)
//
//---------------------------------------------------------------------------

bool checkhitswitch(DDukePlayer* const p, walltype* wwal, DDukeActor* act)
{
	uint8_t switchpal;
	int lotag, hitag, correctdips, numdips;
	DVector2 spos;
	FTextureID texid;
	int swresult = 0;

	if (wwal == nullptr && act == nullptr) return 0;
	correctdips = 1;
	numdips = 0;

	if (act)
	{
		lotag = act->spr.lotag;
		if (lotag == 0) return 0;
		hitag = act->spr.hitag;
		spos = act->spr.pos.XY();
		texid = act->spr.spritetexture();
		switchpal = act->spr.pal;

		// custom switches that maintain themselves can immediately abort.
		swresult = CallTriggerSwitch(act, p);
		if (swresult == 1) return true;
	}
	else
	{
		lotag = wwal->lotag;
		if (lotag == 0) return 0;
		hitag = wwal->hitag;
		spos = wwal->pos;
		texid = wwal->walltexture;
		switchpal = wwal->pal;
	}
	auto& ext = GetExtInfo(texid);
	auto& swdef = switches[ext.switchindex];

	if (swresult == 0)
	{
		// check if the switch may be activated.
		switch (swdef.type)
		{
		case SwitchDef::Combo:
			break;

		case SwitchDef::Access:
			if (fi.checkaccessswitch(p, switchpal, act, wwal))
				return 0;
			[[fallthrough]];

		case SwitchDef::Regular:
		case SwitchDef::Multi:
			if (check_activator_motion(lotag)) return 0;
			break;

		default:
			if (isadoorwall(texid) == 0) return 0;
			break;
		}

		togglespriteswitches(act, ext, lotag, correctdips, numdips);
		togglewallswitches(wwal, ext, lotag, correctdips, numdips);

		if (lotag == -1)
		{
			setnextmap(false);
			return 1;
		}

		// Yet another crude RRRA hack that cannot be fully generalized.
		if (hitag == 10001 && swdef.flags & SwitchDef::oneway && isRRRA())
		{
			act->spr.setspritetexture(swdef.states[1]);
			if (p->SeaSick == 0)
				p->SeaSick = 350;
			operateactivators(668, p);
			operatemasterswitches(668);
			S_PlayActorSound(328, p->GetActor());
			return 1;
		}
	}
	DVector3 v(spos, p->GetActor()->getOffsetZ());

	if (swdef.type != SwitchDef::None || isadoorwall(texid))
	{
		if (swresult == 0)
		{
			if (swdef.type == SwitchDef::Combo)
			{
				FSoundID sound = swdef.soundid != NO_SOUND ? swdef.soundid : S_FindSoundByResID(SWITCH_ON);
				if (act) S_PlaySound3D(sound, act, v);
				else S_PlaySound3D(sound, p->GetActor(), v);
				if (numdips != correctdips) return 0;
				S_PlaySound3D(END_OF_LEVEL_WARN, p->GetActor(), v);
			}
			if (swdef.type == SwitchDef::Multi)
			{
				lotag += ext.switchphase;
				if (hitag == 10000 && act && isRRRA())	// no idea if the game check is really needed for something this far off the beaten path...
				{
					tag10000specialswitch(p, act, v);
					return 1;
				}
			}
		}

		DukeStatIterator itr(STAT_EFFECTOR);
		while (auto other = itr.Next())
		{
			if (other->spr.hitag == lotag)
			{
				switch (other->spr.lotag)
				{
				case 46:
				case SE_47_LIGHT_SWITCH:
				case SE_48_LIGHT_SWITCH:
					if (!isRRRA()) break;
					[[fallthrough]];

				case SE_12_LIGHT_SWITCH:
					other->sector()->floorpal = 0;
					other->counter++;
					if (other->counter == 2)
						other->counter++;

					break;
				case SE_24_CONVEYOR:
				case SE_34:
				case SE_25_PISTON:
					other->temp_data[4] = !other->temp_data[4];
					if (other->temp_data[4])
						FTA(15, p);
					else FTA(2, p);
					break;
				case SE_21_DROP_FLOOR:
					FTA(2, getPlayer(screenpeek));
					break;
				}
			}
		}

		operateactivators(lotag, p);
		operateforcefields(p->GetActor(), lotag);
		operatemasterswitches(lotag);

		if (swdef.type == SwitchDef::Combo) return 1;

		if (hitag == 0 && isadoorwall(texid) == 0)
		{
			FSoundID sound = swdef.soundid != NO_SOUND ? swdef.soundid : S_FindSoundByResID(SWITCH_ON);
			if (act) S_PlaySound3D(sound, act, v);
			else S_PlaySound3D(sound, p->GetActor(), v);
		}
		else if (hitag != 0)
		{
			auto flags = S_GetUserFlags(hitag);

			if (act && (flags & SF_TALK) == 0)
				S_PlaySound3D(hitag, act, v);
			else
				S_PlayActorSound(hitag, p->GetActor());
		}

		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void animatewalls(void)
{
	static FTextureID noise, ff1, ff2;

	// all that was done here is to open the system up sufficiently to allow replacing the textures being used without having to use ART files.
	// Custom animated textures are better done with newly written controller actors.
	if (!noise.isValid()) noise = TexMan.CheckForTexture("SCREENBREAK6", ETextureType::Any);
	if (!ff1.isValid()) ff1 = TexMan.CheckForTexture("W_FORCEFIELD", ETextureType::Any);
	if (!ff2.isValid()) ff2 = TexMan.CheckForTexture("W_FORCEFIELD2", ETextureType::Any);

	if (getPlayer(screenpeek)->sea_sick_stat == 1)
	{
		for (auto& wal : wall)
		{
			if (tileflags(wal.walltexture) & TFLAG_SEASICKWALL)
				wal.addxpan(6);
		}
	}

	int t;

	for (int p = 0; p < numanimwalls; p++)
	{
		auto wal = animwall[p].wall;
		auto texid = wal->walltexture;

		if (!animwall[p].overpic)
		{
			if (tileflags(wal->walltexture) & TFLAG_ANIMSCREEN)
			{
				if ((krand() & 255) < 16)
				{
					wal->setwalltexture(noise);
				}
			}
			else if (tileflags(wal->walltexture) & TFLAG_ANIMSCREENNOISE)
			{
				if (animwall[p].origtex.isValid())
					wal->setwalltexture(animwall[p].origtex);
				else
				{
					texid = texid + 1;
					if (texid.GetIndex() > noise.GetIndex() + 3 || texid.GetIndex() < noise.GetIndex()) texid = noise;
					wal->setwalltexture(texid);
				}
			}
		}
		else
		{
			if (tileflags(wal->overtexture) & TFLAG_ANIMFORCEFIELD && wal->cstat & CSTAT_WALL_MASKED)
			{

				t = animwall[p].tag;

				if (wal->cstat & CSTAT_WALL_ANY_EXCEPT_BLOCK)
				{
					wal->addxpan(-t / 4096.f);
					wal->addypan(-t / 4096.f);

					if (wal->extra == 1)
					{
						wal->extra = 0;
						animwall[p].tag = 0;
					}
					else
						animwall[p].tag += 128;

					if (animwall[p].tag < (128 << 4))
					{
						if (animwall[p].tag & 128)
							wal->setovertexture(ff1);
						else wal->setovertexture(ff2);
					}
					else
					{
						if ((krand() & 255) < 32)
							animwall[p].tag = 128 << (krand() & 3);
						else wal->setovertexture(ff2);
					}
				}
			}

		}
	}

}

END_DUKE_NS
