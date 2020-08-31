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

using std::min;
using std::max;
// PRIMITIVE
BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------
static bool haltsoundhack;

int callsound(int sn, int whatsprite)
{
	if (!isRRRA() && haltsoundhack)
	{
		haltsoundhack = 0;
		return -1;
	}

	int i = headspritesect[sn];
	while (i >= 0)
	{
		if (sprite[i].picnum == MUSICANDSFX && sprite[i].lotag < 1000)
		{
			if (whatsprite == -1) whatsprite = i;

			int snum = sprite[i].lotag;
			auto flags = S_GetUserFlags(snum);

			// Reset if the desired actor isn't playing anything.
			bool hival = S_IsSoundValid(sprite[i].hitag);
			if (hittype[i].temp_data[0] == 1 && !hival)
			{
				if (!S_CheckActorSoundPlaying(hittype[i].temp_data[5], snum))
					hittype[i].temp_data[0] = 0;
			}


			if (hittype[i].temp_data[0] == 0)
			{
				if ((flags & (SF_GLOBAL | SF_DTAG)) != SF_GLOBAL)
				{
					if (snum)
					{
						if (sprite[i].hitag && snum != sprite[i].hitag)
							S_StopSound(sprite[i].hitag, hittype[i].temp_data[5]);
						S_PlayActorSound(snum, whatsprite);
						hittype[i].temp_data[5] = whatsprite;
					}

					if ((sector[sprite[i].sectnum].lotag & 0xff) != ST_22_SPLITTING_DOOR)
						hittype[i].temp_data[0] = 1;
				}
			}
			else if (hival)
			{
				if ((flags & SF_LOOP) || (sprite[i].hitag && sprite[i].hitag != sprite[i].lotag))
					S_StopSound(sprite[i].lotag, hittype[i].temp_data[5]);
				if (sprite[i].hitag) S_PlayActorSound(sprite[i].hitag, whatsprite);
				hittype[i].temp_data[0] = 0;
				hittype[i].temp_data[5] = whatsprite;
			}
			return sprite[i].lotag;
		}
		i = nextspritesect[i];
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
	int i, j;
	spritetype* s;

	i = headspritestat[STAT_ACTIVATOR];
	while (i >= 0)
	{
		if (sprite[i].lotag == lotag)
		{
			s = &sprite[i];

			for (j = animatecnt - 1; j >= 0; j--)
				if (s->sectnum == animatesect[j])
					return(1);

			j = headspritestat[STAT_EFFECTOR];
			while (j >= 0)
			{
				if (s->sectnum == sprite[j].sectnum)
					switch (sprite[j].lotag)
					{
					case SE_11_SWINGING_DOOR:
					case SE_30_TWO_WAY_TRAIN:
						if (hittype[j].temp_data[4])
							return(1);
						break;
					case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
						if (isRRRA()) break;
					case SE_20_STRETCH_BRIDGE:
					case SE_31_FLOOR_RISE_FALL:
					case SE_32_CEILING_RISE_FALL:
						if (hittype[j].temp_data[0])
							return(1);
						break;
					}

				j = nextspritestat[j];
			}
		}
		i = nextspritestat[i];
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

int findplayer(const spritetype* s, int* d)
{
	short j, closest_player;
	int x, closest;

	if (ud.multimode < 2)
	{
		if (d) *d = abs(ps[myconnectindex].oposx - s->x) + abs(ps[myconnectindex].oposy - s->y) + ((abs(ps[myconnectindex].oposz - s->z + (28 << 8))) >> 4);
		return myconnectindex;
	}

	closest = 0x7fffffff;
	closest_player = 0;

	for (j = connecthead; j >= 0; j = connectpoint2[j])
	{
		x = abs(ps[j].oposx - s->x) + abs(ps[j].oposy - s->y) + ((abs(ps[j].oposz - s->z + (28 << 8))) >> 4);
		if (x < closest && sprite[ps[j].i].extra > 0)
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
	short j, closest_player;
	int x, closest;

	closest = 0x7fffffff;
	closest_player = p;

	for (j = connecthead; j >= 0; j = connectpoint2[j])
		if (p != j && sprite[ps[j].i].extra > 0)
		{
			x = abs(ps[j].oposx - ps[p].posx) + abs(ps[j].oposy - ps[p].posy) + (abs(ps[j].oposz - ps[p].posz) >> 4);

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

int* animateptr(int type, int index)
{
	static int scratch;
	switch (type)
	{
	case anim_floorz:
		return &sector[index].floorz;
	case anim_ceilingz:
		return &sector[index].ceilingz;
	case anim_vertexx:
		return &wall[index].x;
	case anim_vertexy:
		return &wall[index].y;
	default:
		assert(false);
		return &scratch;
	}
}

int* animateptr(int i)
{
	return animateptr(animatetype[i], animatetarget[i]);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void doanimations(void)
{
	int i, j, a, p, v, dasect;

	for (i = animatecnt - 1; i >= 0; i--)
	{
		a = *animateptr(i);
		v = animatevel[i] * TICSPERFRAME;
		dasect = animatesect[i];

		if (a == animategoal[i])
		{
			stopinterpolation(animateptr(i));

			animatecnt--;
			animatetype[i] = animatetype[animatecnt];
			animatetarget[i] = animatetarget[animatecnt];
			animategoal[i] = animategoal[animatecnt];
			animatevel[i] = animatevel[animatecnt];
			animatesect[i] = animatesect[animatecnt];
			if (sector[animatesect[i]].lotag == ST_18_ELEVATOR_DOWN || sector[animatesect[i]].lotag == ST_19_ELEVATOR_UP)
				if (animatetype[i] == anim_ceilingz)
					continue;

			if ((sector[dasect].lotag & 0xff) != ST_22_SPLITTING_DOOR)
				callsound(dasect, -1);

			continue;
		}

		if (v > 0) { a = min(a + v, animategoal[i]); }
		else { a = max(a + v, animategoal[i]); }

		if (animatetype[i] == anim_floorz)
		{
			for (p = connecthead; p >= 0; p = connectpoint2[p])
				if (ps[p].cursectnum == dasect)
					if ((sector[dasect].floorz - ps[p].posz) < (64 << 8))
						if (sprite[ps[p].i].owner >= 0)
						{
							ps[p].posz += v;
							ps[p].poszv = 0;
						}

			for (j = headspritesect[dasect]; j >= 0; j = nextspritesect[j])
				if (sprite[j].statnum != STAT_EFFECTOR)
				{
					hittype[j].bposz = sprite[j].z;
					sprite[j].z += v;
					hittype[j].floorz = sector[dasect].floorz + v;
				}
		}

		*animateptr(i) = a;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int getanimationgoal(int animtype, int animtarget)
{
	int i, j;

	j = -1;
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

int setanimation(short animsect, int animtype, int animtarget, int thegoal, int thevel)
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

	auto animptr = animateptr(animtype, animtarget);
	animatesect[j] = animsect;
	animatetype[j] = animtype;
	animatetarget[j] = animtarget;
	animategoal[j] = thegoal;
	if (thegoal >= *animptr)
		animatevel[j] = thevel;
	else
		animatevel[j] = -thevel;

	if (j == animatecnt) animatecnt++;

	setinterpolation(animptr);

	return(j);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool activatewarpelevators(int s, int d) //Parm = sectoreffectornum
{
	short i, sn;

	sn = sprite[s].sectnum;

	// See if the sector exists

	i = headspritestat[3];
	while (i >= 0)
	{
		if (sprite[i].lotag == SE_17_WARP_ELEVATOR || (isRRRA() && sprite[i].lotag == SE_18_INCREMENTAL_SECTOR_RISE_FALL))
			if (sprite[i].hitag == sprite[s].hitag)
				if ((abs(sector[sn].floorz - hittype[s].temp_data[2]) > sprite[i].yvel) ||
					(sector[sprite[i].sectnum].hitag == (sector[sn].hitag - d)))
					break;
		i = nextspritestat[i];
	}

	if (i == -1)
	{
		d = 0;
		return 1; // No find
	}
	else
	{
		if (d == 0)
			S_PlayActorSound(ELEVATOR_OFF, s);
		else S_PlayActorSound(ELEVATOR_ON, s);
	}


	i = headspritestat[3];
	while (i >= 0)
	{
		if (sprite[i].lotag == SE_17_WARP_ELEVATOR || (isRRRA() && sprite[i].lotag == SE_18_INCREMENTAL_SECTOR_RISE_FALL))
			if (sprite[i].hitag == sprite[s].hitag)
			{
				hittype[i].temp_data[0] = d;
				hittype[i].temp_data[1] = d; //Make all check warp
			}
		i = nextspritestat[i];
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operatesectors(int sn, int ii)
{
	int j=0, l, q, startwall, endwall;
	int i;
	char sect_error;
	sectortype* sptr;

	sect_error = 0;
	sptr = &sector[sn];

	switch (sptr->lotag & (0xffff - 49152))
	{

	case 41:
		if (isRR()) operatejaildoors(sptr->hitag);
		break;

	case 7:
		if (!isRR()) break;
		startwall = sptr->wallptr;
		endwall = startwall + sptr->wallnum;
		for (j = startwall; j < endwall; j++)
		{
			setanimation(sn, anim_vertexx, j, wall[j].x + 1024, 4);
			setanimation(sn, anim_vertexx, wall[j].nextwall, wall[wall[j].nextwall].x + 1024, 4);
		}
		break;

	case ST_30_ROTATE_RISE_BRIDGE:
		j = sector[sn].hitag;
		if (hittype[j].tempang == 0 ||
			hittype[j].tempang == 256)
			callsound(sn, ii);
		if (sprite[j].extra == 1)
			sprite[j].extra = 3;
		else sprite[j].extra = 1;
		break;

	case ST_31_TWO_WAY_TRAIN:

		j = sector[sn].hitag;
		if (hittype[j].temp_data[4] == 0)
			hittype[j].temp_data[4] = 1;

		callsound(sn, ii);
		break;

	case ST_26_SPLITTING_ST_DOOR: //The split doors
		i = getanimationgoal(anim_ceilingz, sn);
		if (i == -1) //if the door has stopped
		{
			haltsoundhack = 1;
			sptr->lotag &= 0xff00;
			sptr->lotag |= ST_22_SPLITTING_DOOR;
			operatesectors(sn, ii);
			sptr->lotag &= 0xff00;
			sptr->lotag |= ST_9_SLIDING_ST_DOOR;
			operatesectors(sn, ii);
			sptr->lotag &= 0xff00;
			sptr->lotag |= ST_26_SPLITTING_ST_DOOR;
		}
		return;

	case ST_9_SLIDING_ST_DOOR:
	{
		int dax, day, dax2, day2, sp;
		int wallfind[2];

		startwall = sptr->wallptr;
		endwall = startwall + sptr->wallnum - 1;

		sp = sptr->extra >> 4;

		//first find center point by averaging all points
		dax = 0L, day = 0L;
		for (i = startwall; i <= endwall; i++)
		{
			dax += wall[i].x;
			day += wall[i].y;
		}
		dax /= (endwall - startwall + 1);
		day /= (endwall - startwall + 1);

		//find any points with either same x or same y coordinate
		//  as center (dax, day) - should be 2 points found.
		wallfind[0] = -1;
		wallfind[1] = -1;
		for (i = startwall; i <= endwall; i++)
			if ((wall[i].x == dax) || (wall[i].y == day))
			{
				if (wallfind[0] == -1)
					wallfind[0] = i;
				else wallfind[1] = i;
			}

		for (j = 0; j < 2; j++)
		{
			if ((wall[wallfind[j]].x == dax) && (wall[wallfind[j]].y == day))
			{
				//find what direction door should open by averaging the
				//  2 neighboring points of wallfind[0] & wallfind[1].
				i = wallfind[j] - 1; if (i < startwall) i = endwall;
				dax2 = ((wall[i].x + wall[wall[wallfind[j]].point2].x) >> 1) - wall[wallfind[j]].x;
				day2 = ((wall[i].y + wall[wall[wallfind[j]].point2].y) >> 1) - wall[wallfind[j]].y;
				if (dax2 != 0)
				{
					dax2 = wall[wall[wall[wallfind[j]].point2].point2].x;
					dax2 -= wall[wall[wallfind[j]].point2].x;
					setanimation(sn, anim_vertexx, wallfind[j], wall[wallfind[j]].x + dax2, sp);
					setanimation(sn, anim_vertexx, i, wall[i].x + dax2, sp);
					setanimation(sn, anim_vertexx, wall[wallfind[j]].point2, wall[wall[wallfind[j]].point2].x + dax2, sp);
					callsound(sn, ii);
				}
				else if (day2 != 0)
				{
					day2 = wall[wall[wall[wallfind[j]].point2].point2].y;
					day2 -= wall[wall[wallfind[j]].point2].y;
					setanimation(sn, anim_vertexy, wallfind[j], wall[wallfind[j]].y + day2, sp);
					setanimation(sn, anim_vertexy, i, wall[i].y + day2, sp);
					setanimation(sn, anim_vertexy, wall[wallfind[j]].point2, wall[wall[wallfind[j]].point2].y + day2, sp);
					callsound(sn, ii);
				}
			}
			else
			{
				i = wallfind[j] - 1; if (i < startwall) i = endwall;
				dax2 = ((wall[i].x + wall[wall[wallfind[j]].point2].x) >> 1) - wall[wallfind[j]].x;
				day2 = ((wall[i].y + wall[wall[wallfind[j]].point2].y) >> 1) - wall[wallfind[j]].y;
				if (dax2 != 0)
				{
					setanimation(sn, anim_vertexx, wallfind[j], dax, sp);
					setanimation(sn, anim_vertexx, i, dax + dax2, sp);
					setanimation(sn, anim_vertexx, wall[wallfind[j]].point2, dax + dax2, sp);
					callsound(sn, ii);
				}
				else if (day2 != 0)
				{
					setanimation(sn, anim_vertexy, wallfind[j], day, sp);
					setanimation(sn, anim_vertexy, i, day + day2, sp);
					setanimation(sn, anim_vertexy, wall[wallfind[j]].point2, day + day2, sp);
					callsound(sn, ii);
				}
			}
		}

	}
	return;

	case ST_15_WARP_ELEVATOR://Warping elevators

		if (sprite[ii].picnum != TILE_APLAYER) return;
		//			if(ps[sprite[ii].yvel].select_dir == 1) return;

		i = headspritesect[sn];
		while (i >= 0)
		{
			if (sprite[i].picnum == SECTOREFFECTOR && sprite[i].lotag == 17) break;
			i = nextspritesect[i];
		}

		if (sprite[ii].sectnum == sn)
		{
			if (activatewarpelevators(i, -1))
				activatewarpelevators(i, 1);
			else if (activatewarpelevators(i, 1))
				activatewarpelevators(i, -1);
			return;
		}
		else
		{
			if (sptr->floorz > sprite[i].z)
				activatewarpelevators(i, -1);
			else
				activatewarpelevators(i, 1);
		}

		return;

	case ST_16_PLATFORM_DOWN:
	case ST_17_PLATFORM_UP:

		i = getanimationgoal(anim_floorz, sn);

		if (i == -1)
		{
			i = nextsectorneighborz(sn, sptr->floorz, 1, 1);
			if (i == -1)
			{
				i = nextsectorneighborz(sn, sptr->floorz, 1, -1);
				if (i == -1) return;
				j = sector[i].floorz;
				setanimation(sn, anim_floorz, sn, j, sptr->extra);
			}
			else
			{
				j = sector[i].floorz;
				setanimation(sn, anim_floorz, sn, j, sptr->extra);
			}
			callsound(sn, ii);
		}

		return;

	case ST_18_ELEVATOR_DOWN:
	case ST_19_ELEVATOR_UP:

		i = getanimationgoal(anim_floorz, sn);

		if (i == -1)
		{
			i = nextsectorneighborz(sn, sptr->floorz, 1, -1);
			if (i == -1) i = nextsectorneighborz(sn, sptr->floorz, 1, 1);
			if (i == -1) return;
			j = sector[i].floorz;
			q = sptr->extra;
			l = sptr->ceilingz - sptr->floorz;
			setanimation(sn, anim_floorz, sn, j, q);
			setanimation(sn, anim_ceilingz, sn, j + l, q);
			callsound(sn, ii);
		}
		return;

	case ST_29_TEETH_DOOR:

		if (sptr->lotag & 0x8000)
			j = sector[nextsectorneighborz(sn, sptr->ceilingz, 1, 1)].floorz;
		else
			j = sector[nextsectorneighborz(sn, sptr->ceilingz, -1, -1)].ceilingz;

		i = headspritestat[3]; //Effectors
		while (i >= 0)
		{
			if ((sprite[i].lotag == 22) &&
				(sprite[i].hitag == sptr->hitag))
			{
				sector[sprite[i].sectnum].extra = -sector[sprite[i].sectnum].extra;

				hittype[i].temp_data[0] = sn;
				hittype[i].temp_data[1] = 1;
			}
			i = nextspritestat[i];
		}

		sptr->lotag ^= 0x8000;

		setanimation(sn, anim_ceilingz, sn, j, sptr->extra);

		callsound(sn, ii);

		return;

	case ST_20_CEILING_DOOR:
	REDODOOR:

		if (sptr->lotag & 0x8000)
		{
			i = headspritesect[sn];
			while (i >= 0)
			{
				if (sprite[i].statnum == 3 && sprite[i].lotag == 9)
				{
					j = sprite[i].z;
					break;
				}
				i = nextspritesect[i];
			}
			if (i == -1)
				j = sptr->floorz;
		}
		else
		{
			j = nextsectorneighborz(sn, sptr->ceilingz, -1, -1);

			if (j >= 0) j = sector[j].ceilingz;
			else
			{
				sptr->lotag |= 32768;
				goto REDODOOR;
			}
		}

		sptr->lotag ^= 0x8000;

		setanimation(sn, anim_ceilingz, sn, j, sptr->extra);
		callsound(sn, ii);

		return;

	case ST_21_FLOOR_DOOR:
		i = getanimationgoal(anim_floorz, sn);
		if (i >= 0)
		{
			if (animategoal[sn] == sptr->ceilingz)
				animategoal[i] = sector[nextsectorneighborz(sn, sptr->ceilingz, 1, 1)].floorz;
			else animategoal[i] = sptr->ceilingz;
			j = animategoal[i];
		}
		else
		{
			if (sptr->ceilingz == sptr->floorz)
				j = sector[nextsectorneighborz(sn, sptr->ceilingz, 1, 1)].floorz;
			else j = sptr->ceilingz;

			sptr->lotag ^= 0x8000;

			if (setanimation(sn, anim_floorz, sn, j, sptr->extra) >= 0)
				callsound(sn, ii);
		}
		return;

	case ST_22_SPLITTING_DOOR:

		if ((sptr->lotag & 0x8000))
		{
			q = (sptr->ceilingz + sptr->floorz) >> 1;
			j = setanimation(sn, anim_floorz, sn, q, sptr->extra);
			j = setanimation(sn, anim_ceilingz, sn, q, sptr->extra);
		}
		else
		{
			q = sector[nextsectorneighborz(sn, sptr->floorz, 1, 1)].floorz;
			j = setanimation(sn, anim_floorz, sn, q, sptr->extra);
			q = sector[nextsectorneighborz(sn, sptr->ceilingz, -1, -1)].ceilingz;
			j = setanimation(sn, anim_ceilingz, sn, q, sptr->extra);
		}

		sptr->lotag ^= 0x8000;

		callsound(sn, ii);

		return;

	case ST_23_SWINGING_DOOR: //Swingdoor

		j = -1;
		q = 0;

		i = headspritestat[3];
		while (i >= 0)
		{
			if (sprite[i].lotag == 11 && sprite[i].sectnum == sn && !hittype[i].temp_data[4])
			{
				j = i;
				break;
			}
			i = nextspritestat[i];
		}

		l = sector[sprite[i].sectnum].lotag & 0x8000;

		if (j >= 0)
		{
			i = headspritestat[3];
			while (i >= 0)
			{
				if (l == (sector[sprite[i].sectnum].lotag & 0x8000) && sprite[i].lotag == 11 && sprite[j].hitag == sprite[i].hitag && !hittype[i].temp_data[4])
				{
					if (sector[sprite[i].sectnum].lotag & 0x8000) sector[sprite[i].sectnum].lotag &= 0x7fff;
					else sector[sprite[i].sectnum].lotag |= 0x8000;
					hittype[i].temp_data[4] = 1;
					hittype[i].temp_data[3] = -hittype[i].temp_data[3];
					if (q == 0)
					{
						callsound(sn, i);
						q = 1;
					}
				}
				i = nextspritestat[i];
			}
		}
		return;

	case ST_25_SLIDING_DOOR: //Subway type sliding doors

		j = headspritestat[3];
		while (j >= 0)//Find the sprite
		{
			if ((sprite[j].lotag) == 15 && sprite[j].sectnum == sn)
				break; //Found the sectoreffector.
			j = nextspritestat[j];
		}

		if (j < 0)
			return;

		i = headspritestat[3];
		while (i >= 0)
		{
			if (sprite[i].hitag == sprite[j].hitag)
			{
				if (sprite[i].lotag == 15)
				{
					sector[sprite[i].sectnum].lotag ^= 0x8000; // Toggle the open or close
					sprite[i].ang += 1024;
					if (hittype[i].temp_data[4]) callsound(sprite[i].sectnum, i);
					callsound(sprite[i].sectnum, i);
					if (sector[sprite[i].sectnum].lotag & 0x8000) hittype[i].temp_data[4] = 1;
					else hittype[i].temp_data[4] = 2;
				}
			}
			i = nextspritestat[i];
		}
		return;

	case ST_27_STRETCH_BRIDGE:  //Extended bridge

		j = headspritestat[3];
		while (j >= 0)
		{
			if ((sprite[j].lotag & 0xff) == 20 && sprite[j].sectnum == sn) //Bridge
			{

				sector[sn].lotag ^= 0x8000;
				if (sector[sn].lotag & 0x8000) //OPENING
					hittype[j].temp_data[0] = 1;
				else hittype[j].temp_data[0] = 2;
				callsound(sn, ii);
				break;
			}
			j = nextspritestat[j];
		}
		return;


	case ST_28_DROP_FLOOR:
		//activate the rest of them

		j = headspritesect[sn];
		while (j >= 0)
		{
			if (sprite[j].statnum == 3 && (sprite[j].lotag & 0xff) == 21)
				break; //Found it
			j = nextspritesect[j];
		}

		j = sprite[j].hitag;

		l = headspritestat[3];
		while (l >= 0)
		{
			if ((sprite[l].lotag & 0xff) == 21 && !hittype[l].temp_data[0] &&
				(sprite[l].hitag) == j)
				hittype[l].temp_data[0] = 1;
			l = nextspritestat[l];
		}
		callsound(sn, ii);

		return;
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operateactivators(int low, int snum)
{
	int i, j, k;
	short * p;
	walltype* wal;

	for (i = numcyclers - 1; i >= 0; i--)
	{
		p = &cyclers[i][0];

		if (p[4] == low)
		{
			p[5] = !p[5];

			sector[p[0]].floorshade = sector[p[0]].ceilingshade = p[3];
			wal = &wall[sector[p[0]].wallptr];
			for (j = sector[p[0]].wallnum; j > 0; j--, wal++)
				wal->shade = p[3];
		}
	}

	i = headspritestat[8];
	k = -1;
	while (i >= 0)
	{
		if (sprite[i].lotag == low)
		{
			if (sprite[i].picnum == ACTIVATORLOCKED)
			{
				sector[sprite[i].sectnum].lotag ^= 16384;

				if (snum >= 0)
				{
					if (sector[sprite[i].sectnum].lotag & 16384)
						FTA(4, &ps[snum]);
					else FTA(8, &ps[snum]);
				}
			}
			else
			{
				switch (sprite[i].hitag)
				{
				case 0:
					break;
				case 1:
					if (sector[sprite[i].sectnum].floorz != sector[sprite[i].sectnum].ceilingz)
					{
						i = nextspritestat[i];
						continue;
					}
					break;
				case 2:
					if (sector[sprite[i].sectnum].floorz == sector[sprite[i].sectnum].ceilingz)
					{
						i = nextspritestat[i];
						continue;
					}
					break;
				}

				if (sector[sprite[i].sectnum].lotag < 3)
				{
					j = headspritesect[sprite[i].sectnum];
					while (j >= 0)
					{
						if (sprite[j].statnum == 3) switch (sprite[j].lotag)
						{
						case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
							if (isRRRA()) break;
						case SE_36_PROJ_SHOOTER:
						case SE_31_FLOOR_RISE_FALL:
						case SE_32_CEILING_RISE_FALL:
							hittype[j].temp_data[0] = 1 - hittype[j].temp_data[0];
							callsound(sprite[i].sectnum, j);
							break;
						}
						j = nextspritesect[j];
					}
				}

				if (k == -1 && (sector[sprite[i].sectnum].lotag & 0xff) == 22)
					k = callsound(sprite[i].sectnum, i);

				operatesectors(sprite[i].sectnum, i);
			}
		}
		i = nextspritestat[i];
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
	short i;

	i = headspritestat[6];
	while (i >= 0)
	{
		if (sprite[i].picnum == MASTERSWITCH && sprite[i].lotag == low && sprite[i].yvel == 0)
			sprite[i].yvel = 1;
		i = nextspritestat[i];
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operateforcefields_common(int s, int low, const std::initializer_list<int> &tiles)
{
	int i, p;

	for (p = numanimwalls; p >= 0; p--)
	{
		i = animwall[p].wallnum;

		if (low == wall[i].lotag || low == -1)
			if (isIn(wall[i].overpicnum, tiles))
			{
				animwall[p].tag = 0;

				if (wall[i].cstat)
				{
					wall[i].cstat = 0;

					if (s >= 0 && sprite[s].picnum == SECTOREFFECTOR &&
						sprite[s].lotag == 30)
						wall[i].lotag = 0;
				}
				else
					wall[i].cstat = 85;
			}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void breakwall(short newpn, short spr, short dawallnum)
{
	wall[dawallnum].picnum = newpn;
	S_PlayActorSound(VENT_BUST, spr);
	S_PlayActorSound(GLASS_HEAVYBREAK, spr);
	lotsofglass(spr, dawallnum, 10);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void allignwarpelevators(void)
{
	short i, j;

	i = headspritestat[STAT_EFFECTOR];
	while (i >= 0)
	{
		if (sprite[i].lotag == SE_17_WARP_ELEVATOR && sprite[i].shade > 16)
		{
			j = headspritestat[STAT_EFFECTOR];
			while (j >= 0)
			{
				if ((sprite[j].lotag) == SE_17_WARP_ELEVATOR && i != j &&
					(sprite[i].hitag) == (sprite[j].hitag))
				{
					sector[sprite[j].sectnum].floorz = sector[sprite[i].sectnum].floorz;
					sector[sprite[j].sectnum].ceilingz = sector[sprite[i].sectnum].ceilingz;
				}

				j = nextspritestat[j];
			}
		}
		i = nextspritestat[i];
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveclouds(double smoothratio)
{
	// The math here is very messy... :(
	int myclock = smoothratio < 32768? ud.levelclock-2 : ud.levelclock;
	if (myclock > cloudclock || myclock < (cloudclock - 7))
	{
		cloudclock = myclock + 6;

		// cloudx/y were an array, but all entries were always having the same value so a single pair is enough.
		cloudx += (sintable[(ps[screenpeek].getang() + 512) & 2047] >> 9);
		cloudy += (sintable[ps[screenpeek].getang() & 2047] >> 9);
		for (int i = 0; i < numclouds; i++)
		{
			sector[clouds[i]].ceilingxpanning = cloudx >> 6;
			sector[clouds[i]].ceilingypanning = cloudy >> 6;
		}
	}
}




END_DUKE_NS
