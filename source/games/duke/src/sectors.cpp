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
		if (act->spr.picnum == MUSICANDSFX && act->spr.lotag < 1000)
		{
			if (whatsprite == nullptr) whatsprite = act;

			int snum = act->spr.lotag;
			auto flags = S_GetUserFlags(snum);

			// Reset if the desired actor isn't playing anything.
			bool hival = S_IsSoundValid(act->spr.hitag);
			if (act->temp_data[0] == 1 && !hival && !endstate)
			{
				if (!S_CheckActorSoundPlaying(act->temp_actor, snum))
					act->temp_data[0] = 0;
			}

			if (act->temp_data[0] == 0)
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
						act->temp_data[0] = 1;
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
				act->temp_data[0] = 0;
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
			for (int j = animatecnt - 1; j >= 0; j--)
				if (act->sector() == animatesect[j])
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
						if (act2->temp_data[0])
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

int findplayer(const DDukeActor* actor, int* d)
{
	int j, closest_player;
	int x, closest;
	const auto s = actor->int_pos();

	if (ud.multimode < 2)
	{
		if (d) *d = abs(ps[myconnectindex].player_int_opos().X - s.X) + abs(ps[myconnectindex].player_int_opos().Y - s.Y) + ((abs(ps[myconnectindex].player_int_opos().Z - s.Z + (28 << 8))) >> 4);
		return myconnectindex;
	}

	closest = 0x7fffffff;
	closest_player = 0;

	for (j = connecthead; j >= 0; j = connectpoint2[j])
	{
		x = abs(ps[j].player_int_opos().X - s.X) + abs(ps[j].player_int_opos().Y - s.Y) + ((abs(ps[j].player_int_opos().Z - s.Z + (28 << 8))) >> 4);
		if (x < closest && ps[j].GetActor()->spr.extra > 0)
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

int findotherplayer(int p, int* d)
{
	int j, closest_player;
	int x, closest;

	closest = 0x7fffffff;
	closest_player = p;

	for (j = connecthead; j >= 0; j = connectpoint2[j])
		if (p != j && ps[j].GetActor()->spr.extra > 0)
		{
			x = abs(ps[j].player_int_opos().X - ps[p].player_int_pos().X) + abs(ps[j].player_int_opos().Y - ps[p].player_int_pos().Y) + (abs(ps[j].player_int_opos().Z - ps[p].player_int_pos().Z) >> 4);

			if (x < closest)
			{
				closest_player = j;
				closest = x;
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
		return sector[index].int_floorz();
	case anim_ceilingz:
		return sector[index].int_ceilingz();
	case anim_vertexx:
		return wall[index].wall_int_pos().X;
	case anim_vertexy:
		return wall[index].wall_int_pos().Y;
	default:
		assert(false);
		return 0;
	}
}

double getanimatevalue(int i)
{
	return getanimatevalue(animatetype[i], animatetarget[i]);
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
		sector[index].set_int_floorz((int)value);
		break;
	case anim_ceilingz:
		sector[index].set_int_ceilingz((int)value);
		break;
	case anim_vertexx:

		wall[index].pos.X = value * inttoworld;
		wall[index].moved();
		break;
	case anim_vertexy:

		wall[index].pos.Y = value * inttoworld;
		wall[index].moved();
		break;
	default:
		assert(false);
	}
}

void setanimatevalue(int i, double value)
{
	return setanimatevalue(animatetype[i], animatetarget[i], value);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void doanimations(void)
{
	int i, a, p, v;

	for (i = animatecnt - 1; i >= 0; i--)
	{
		a = (int)getanimatevalue(i);
		v = animatevel[i] * TICSPERFRAME;
		auto dasectp = animatesect[i];

		if (a == animategoal[i])
		{
			StopInterpolation(animatetarget[i], interptype[animatetype[i]]);

			animatecnt--;
			animatetype[i] = animatetype[animatecnt];
			animatetarget[i] = animatetarget[animatecnt];
			animategoal[i] = animategoal[animatecnt];
			animatevel[i] = animatevel[animatecnt];
			animatesect[i] = animatesect[animatecnt];
			dasectp = animatesect[i];
			if (dasectp->lotag == ST_18_ELEVATOR_DOWN || dasectp->lotag == ST_19_ELEVATOR_UP)
				if (animatetype[i] == anim_ceilingz)
					continue;

			if ((dasectp->lotag & 0xff) != ST_22_SPLITTING_DOOR)
				callsound(dasectp, nullptr, true);

			continue;
		}

		if (v > 0) { a = min(a + v, animategoal[i]); }
		else { a = max(a + v, animategoal[i]); }

		if (animatetype[i] == anim_floorz)
		{
			for (p = connecthead; p >= 0; p = connectpoint2[p])
				if (ps[p].cursector == dasectp)
					if ((dasectp->int_floorz() - ps[p].player_int_pos().Z) < (64 << 8))
						if (ps[p].GetActor()->GetOwner() != nullptr)
						{
							ps[p].player_add_int_z(v);
							ps[p].vel.Z = 0;
						}

			DukeSectIterator it(dasectp);
			while (auto act = it.Next())
			{
				if (act->spr.statnum != STAT_EFFECTOR)
				{
					act->backupz();
					act->add_int_z(v);
					act->floorz = dasectp->floorz + v * zinttoworld;
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

int getanimationgoal(int animtype, sectortype* animtargetp)
{
	int i, j;

	j = -1;
	int animtarget = sectnum(animtargetp);
	for (i = animatecnt - 1; i >= 0; i--)
		if (animtype == animatetype[i] && animtarget == animatetarget[i])
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

static int dosetanimation(sectortype* animsect, int animtype, int animtarget, int thegoal, int thevel)
{
	int i, j;

	if (animatecnt >= MAXANIMATES - 1)
		return(-1);

	j = animatecnt;
	for (i = 0; i < animatecnt; i++)
		if (animtype == animatetype[i] && animtarget == animatetarget[i])
		{
			j = i;
			break;
		}

	auto animval = (int)getanimatevalue(animtype, animtarget);
	animatesect[j] = animsect;
	animatetype[j] = animtype;
	animatetarget[j] = animtarget;
	animategoal[j] = thegoal;
	if (thegoal >= animval)
		animatevel[j] = thevel;
	else
		animatevel[j] = -thevel;

	if (j == animatecnt) animatecnt++;

	StartInterpolation(animatetarget[i], interptype[animatetype[i]]);
	return(j);
}

int setanimation(sectortype* animsect, int animtype, walltype* animtarget, int thegoal, int thevel)
{
	assert(animtype == anim_vertexx || animtype == anim_vertexy);
	return dosetanimation(animsect, animtype, wallnum(animtarget), thegoal, thevel);
}

int setanimation(sectortype* animsect, int animtype, sectortype* animtarget, int thegoal, int thevel)
{
	assert(animtype == anim_ceilingz || animtype == anim_floorz);
	return dosetanimation(animsect, animtype, sectnum(animtarget), thegoal, thevel);
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
				if ((abs(sect->int_floorz() - actor->temp_data[2]) > act2->spr.yvel) ||
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
				act2->temp_data[0] = d;
				act2->temp_data[1] = d; //Make all check warp
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
	int dax, day, dax2, day2, sp;
	walltype* wallfind[2];

	sp = sptr->extra >> 4;

	//first find center point by averaging all points
	dax = 0L, day = 0L;
	for (auto& wal : wallsofsector(sptr))
	{
		dax += wal.wall_int_pos().X;
		day += wal.wall_int_pos().Y;
	}
	dax /= sptr->wallnum;
	day /= sptr->wallnum;

	//find any points with either same x or same y coordinate
	//  as center (dax, day) - should be 2 points found.
	wallfind[0] = nullptr;
	wallfind[1] = nullptr;
	for (auto& wal : wallsofsector(sptr))
		if ((wal.wall_int_pos().X == dax) || (wal.wall_int_pos().Y == day))
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
		if (prevwall < sptr->firstWall()) prevwall += sptr->wallnum;

		if ((wal->wall_int_pos().X == dax) && (wal->wall_int_pos().Y == day))
		{
			dax2 = ((prevwall->wall_int_pos().X + wal->point2Wall()->wall_int_pos().X) >> 1) - wal->wall_int_pos().X;
			day2 = ((prevwall->wall_int_pos().Y + wal->point2Wall()->wall_int_pos().Y) >> 1) - wal->wall_int_pos().Y;
			if (dax2 != 0)
			{
				dax2 = wal->point2Wall()->point2Wall()->wall_int_pos().X;
				dax2 -= wal->point2Wall()->wall_int_pos().X;
				setanimation(sptr, anim_vertexx, wal, wal->wall_int_pos().X + dax2, sp);
				setanimation(sptr, anim_vertexx, prevwall, prevwall->wall_int_pos().X + dax2, sp);
				setanimation(sptr, anim_vertexx, wal->point2Wall(), wal->point2Wall()->wall_int_pos().X + dax2, sp);
				callsound(sptr, actor);
			}
			else if (day2 != 0)
			{
				day2 = wal->point2Wall()->point2Wall()->wall_int_pos().Y;
				day2 -= wal->point2Wall()->wall_int_pos().Y;
				setanimation(sptr, anim_vertexy, wal, wal->wall_int_pos().Y + day2, sp);
				setanimation(sptr, anim_vertexy, prevwall, prevwall->wall_int_pos().Y + day2, sp);
				setanimation(sptr, anim_vertexy, wal->point2Wall(), wal->point2Wall()->wall_int_pos().Y + day2, sp);
				callsound(sptr, actor);
			}
		}
		else
		{
			dax2 = ((prevwall->wall_int_pos().X + wal->point2Wall()->wall_int_pos().X) >> 1) - wal->wall_int_pos().X;
			day2 = ((prevwall->wall_int_pos().Y + wal->point2Wall()->wall_int_pos().Y) >> 1) - wal->wall_int_pos().Y;
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
		if (a2->spr.picnum == SECTOREFFECTOR && a2->spr.lotag == ST_17_PLATFORM_UP) break;
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
		if (sptr->int_floorz() > a2->int_pos().Z)
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
	int i = getanimationgoal(anim_floorz, sptr);
	sectortype* sectp;

	if (i == -1)
	{
		sectp = nextsectorneighborzptr(sptr, sptr->int_floorz(), Find_FloorDown);
		if (sectp == nullptr)
		{
			sectp = nextsectorneighborzptr(sptr, sptr->int_floorz(), Find_FloorUp);
			if (sectp == nullptr) return;
			setanimation(sptr, anim_floorz, sptr, sectp->int_floorz(), sptr->extra);
		}
		else
		{
			setanimation(sptr, anim_floorz, sptr, sectp->int_floorz(), sptr->extra);
		}
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
	int i = getanimationgoal(anim_floorz, sptr);

	if (i == -1)
	{
		auto sectp = nextsectorneighborzptr(sptr, sptr->int_floorz(), Find_FloorUp);
		if (sectp == nullptr) sectp = nextsectorneighborzptr(sptr, sptr->int_floorz(), Find_FloorDown);
		if (sectp == nullptr) return;
		int j = sectp->int_floorz();
		int q = sptr->extra;
		int l = sptr->int_ceilingz() - sptr->int_floorz();
		setanimation(sptr, anim_floorz, sptr, j, q);
		setanimation(sptr, anim_ceilingz, sptr, j + l, q);
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
	int j;

	if (sptr->lotag & 0x8000)
		j = nextsectorneighborzptr(sptr, sptr->int_ceilingz(), Find_FloorDown | Find_Safe)->int_floorz();
	else
		j = nextsectorneighborzptr(sptr, sptr->int_ceilingz(), Find_CeilingUp | Find_Safe)->int_ceilingz();

	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act2 = it.Next())
	{
		if ((act2->spr.lotag == 22) &&
			(act2->spr.hitag == sptr->hitag))
		{
			act2->sector()->extra = -act2->sector()->extra;

			act2->temp_data[0] = sectnum(sptr);
			act2->temp_data[1] = 1;
		}
	}

	sptr->lotag ^= 0x8000;

	setanimation(sptr, anim_ceilingz, sptr, j, sptr->extra);

	callsound(sptr, actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st20(sectortype* sptr, DDukeActor* actor)
{
	int j = 0;
REDODOOR:

	if (sptr->lotag & 0x8000)
	{
		DDukeActor* a2;
		DukeSectIterator it(sptr);
		while ((a2 = it.Next()))
		{
			if (a2->spr.statnum == 3 && a2->spr.lotag == 9)
			{
				j = a2->int_pos().Z;
				break;
			}
		}
		if (a2 == nullptr)
			j = sptr->int_floorz();
	}
	else
	{
		auto sectp = nextsectorneighborzptr(sptr, sptr->int_ceilingz(), Find_CeilingUp);

		if (sectp) j = sectp->int_ceilingz();
		else
		{
			sptr->lotag |= 32768;
			goto REDODOOR;
		}
	}

	sptr->lotag ^= 0x8000;

	setanimation(sptr, anim_ceilingz, sptr, j, sptr->extra);
	callsound(sptr, actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_st21(sectortype* sptr, DDukeActor* actor)
{
	int i = getanimationgoal(anim_floorz, sptr);
	int j;
	if (i >= 0)
	{
		if (animategoal[i] == sptr->int_ceilingz())
			animategoal[i] = nextsectorneighborzptr(sptr, sptr->int_ceilingz(), Find_FloorDown | Find_Safe)->int_floorz();
		else animategoal[i] = sptr->int_ceilingz();
		j = animategoal[i];
	}
	else
	{
		if (sptr->int_ceilingz() == sptr->int_floorz())
			j = nextsectorneighborzptr(sptr, sptr->int_ceilingz(), Find_FloorDown | Find_Safe)->int_floorz();
		else j = sptr->int_ceilingz();

		sptr->lotag ^= 0x8000;

		if (setanimation(sptr, anim_floorz, sptr, j, sptr->extra) >= 0)
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
	int j, q;
	if ((sptr->lotag & 0x8000))
	{
		q = (sptr->int_ceilingz() + sptr->int_floorz()) >> 1;
		j = setanimation(sptr, anim_floorz, sptr, q, sptr->extra);
		j = setanimation(sptr, anim_ceilingz, sptr, q, sptr->extra);
	}
	else
	{
		q = nextsectorneighborzptr(sptr, sptr->int_floorz(), Find_FloorDown | Find_Safe)->int_floorz();
		j = setanimation(sptr, anim_floorz, sptr, q, sptr->extra);
		q = nextsectorneighborzptr(sptr, sptr->int_ceilingz(), Find_CeilingUp | Find_Safe)->int_ceilingz();
		j = setanimation(sptr, anim_ceilingz, sptr, q, sptr->extra);
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
				act3->add_int_ang(1024);
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
				act2->temp_data[0] = 1;
			else act2->temp_data[0] = 2;
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
		if ((act3->spr.lotag & 0xff) == 21 && !act3->temp_data[0] &&
			(act3->spr.hitag) == j)
			act3->temp_data[0] = 1;
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
		for (auto& wal : wallsofsector(sptr))
		{
			setanimation(sptr, anim_vertexx, &wal, wal.wall_int_pos().X + 1024, 4);
			if (wal.twoSided()) setanimation(sptr, anim_vertexx, wal.nextWall(), wal.nextWall()->wall_int_pos().X + 1024, 4);
		}
		break;

	case ST_30_ROTATE_RISE_BRIDGE:
	{
		auto act = barrier_cast<DDukeActor*>(sptr->hitagactor);
		if (!act) break;
		if (act->tempang == 0 || act->tempang == 256) callsound(sptr, actor);
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
		i = getanimationgoal(anim_ceilingz, sptr);
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

void operateactivators(int low, int plnum)
{
	int i, j, k;
	Cycler * p;
	walltype* wal;

	for (i = numcyclers - 1; i >= 0; i--)
	{
		p = &cyclers[i];


		if (p->hitag == low)
		{
			auto sect = p->sector;
			p->state = !p->state;

			sect->floorshade = sect->ceilingshade = (int8_t)p->shade2;
			wal = sect->firstWall();
			for (j = sect->wallnum; j > 0; j--, wal++)
				wal->shade = (int8_t)p->shade2;
		}
	}

	k = -1;
	DukeStatIterator it(STAT_ACTIVATOR);
	while (auto act = it.Next())
	{
		if (act->spr.lotag == low)
		{
			if (act->spr.picnum == ACTIVATORLOCKED)
			{
				act->sector()->lotag ^= 16384;

				if (plnum >= 0)
				{
					if (act->sector()->lotag & 16384)
						FTA(4, &ps[plnum]);
					else FTA(8, &ps[plnum]);
				}
			}
			else
			{
				switch (act->spr.hitag)
				{
				case 0:
					break;
				case 1:
					if (act->sector()->int_floorz() != act->sector()->int_ceilingz())
					{
						continue;
					}
					break;
				case 2:
					if (act->sector()->int_floorz() == act->sector()->int_ceilingz())
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
							a2->temp_data[0] = 1 - a2->temp_data[0];
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

	fi.operaterespawns(low);
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
		if (act2->spr.picnum == MASTERSWITCH && act2->spr.lotag == low && act2->spr.yvel == 0)
			act2->spr.yvel = 1;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operateforcefields_common(DDukeActor *effector, int low, const std::initializer_list<int> &tiles)
{
	for (int p = numanimwalls-1; p >= 0; p--)
	{
		auto wal = animwall[p].wall;

		if (low == wal->lotag || low == -1)
			if (isIn(wal->overpicnum, tiles))
			{
				animwall[p].tag = 0;

				if (wal->cstat)
				{
					wal->cstat = 0;

					if (effector && effector->spr.picnum == SECTOREFFECTOR && effector->spr.lotag == 30)
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

void breakwall(int newpn, DDukeActor* spr, walltype* wal)
{
	wal->picnum = newpn;
	S_PlayActorSound(VENT_BUST, spr);
	S_PlayActorSound(GLASS_HEAVYBREAK, spr);
	lotsofglass(spr, wal, 10);
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
					act2->sector()->set_int_floorz(act->sector()->int_floorz());
					act2->sector()->set_int_ceilingz(act->sector()->int_ceilingz());
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

void moveclouds(double smoothratio)
{
	// The math here is very messy.. :(
	int myclock = smoothratio < 32768? PlayClock-2 : PlayClock;
	if (myclock > cloudclock || myclock < (cloudclock - 7))
	{
		cloudclock = myclock + 6;

		// cloudx/y were an array, but all entries were always having the same value so a single pair is enough.
		cloudx += (float)ps[screenpeek].angle.ang.Cos() * 0.5f;
		cloudy += (float)ps[screenpeek].angle.ang.Sin() * 0.5f;
		for (int i = 0; i < numclouds; i++)
		{
			// no clamping here!
			clouds[i]->ceilingxpan_ = cloudx;
			clouds[i]->ceilingypan_ = cloudy;
			clouds[i]->exflags |= SECTOREX_CLOUDSCROLL;
		}
	}
}




END_DUKE_NS
