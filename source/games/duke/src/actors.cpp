//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2017-2019 - Nuke.YKT
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

This file is a combination of code from the following sources:
- EDuke 2 by Matt Saettler
- JFDuke by Jonathon Fowler (jf@jonof.id.au),
- DukeGDX and RedneckGDX by Alexander Makarov-[M210] (m210-2007@mail.ru)
- Redneck Rampage reconstructed source by Nuke.YKT

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names.h"
#include "stats.h"
#include "constants.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

int adjustfall(spritetype* s, int c);


//---------------------------------------------------------------------------
//
// this was once a macro
//
//---------------------------------------------------------------------------

void RANDOMSCRAP(DDukeActor* origin)
{
	int r1 = krand(), r2 = krand(), r3 = krand(), r4 = krand(), r5 = krand(), r6 = krand(), r7 = krand();
	int v = isRR() ? 16 : 48;
	EGS(origin->s.sectnum, 
		origin->s.x + (r7 & 255) - 128, origin->s.y + (r6 & 255) - 128, origin->s.z - (8 << 8) - (r5 & 8191), 
		TILE_SCRAP6 + (r4 & 15), -8, v, v, r3 & 2047, (r2 & 63) + 64, -512 - (r1 & 2047), origin, 5); 
}

//---------------------------------------------------------------------------
//
// wrapper to ensure that if a sound actor is killed, the sound is stopped as well.
//
//---------------------------------------------------------------------------

void deletesprite(DDukeActor *const actor)
{
	if (actor->s.picnum == MUSICANDSFX && actor->temp_data[0] == 1)
		S_StopSound(actor->s.lotag, actor);
	else
		S_RelinkActorSound(actor, nullptr);
	::deletesprite(actor->GetIndex());
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addammo(int weapon, struct player_struct* player, int amount)
{
	player->ammo_amount[weapon] += amount;

	if (player->ammo_amount[weapon] > max_ammo_amount[weapon])
		player->ammo_amount[weapon] = max_ammo_amount[weapon];
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkavailinven(struct player_struct* player)
{

	if (player->firstaid_amount > 0)
		player->inven_icon = ICON_FIRSTAID;
	else if (player->steroids_amount > 0)
		player->inven_icon = ICON_STEROIDS;
	else if (player->holoduke_amount > 0)
		player->inven_icon = ICON_HOLODUKE;
	else if (player->jetpack_amount > 0)
		player->inven_icon = ICON_JETPACK;
	else if (player->heat_amount > 0)
		player->inven_icon = ICON_HEATS;
	else if (player->scuba_amount > 0)
		player->inven_icon = ICON_SCUBA;
	else if (player->boot_amount > 0)
		player->inven_icon = ICON_BOOTS;
	else player->inven_icon = ICON_NONE;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkavailweapon(struct player_struct* player)
{
	short i, snum;
	int weap;

	if (player->wantweaponfire >= 0)
	{
		weap = player->wantweaponfire;
		player->wantweaponfire = -1;

		if (weap == player->curr_weapon) return;
		else if (player->gotweapon[weap] && player->ammo_amount[weap] > 0)
		{
			fi.addweapon(player, weap);
			return;
		}
	}

	weap = player->curr_weapon;
	if (player->gotweapon[weap] && player->ammo_amount[weap] > 0)
		return;

	snum = player->GetPlayerNum();

	int max = MAX_WEAPON;
	for (i = 0; i <= max; i++)
	{
		weap = ud.wchoice[snum][i];
		if ((g_gameType & GAMEFLAG_SHAREWARE) && weap > 6) continue;

		if (weap == 0) weap = max;
		else weap--;

		if (weap == MIN_WEAPON || (player->gotweapon[weap] && player->ammo_amount[weap] > 0))
			break;
	}

	if (i == MAX_WEAPON) weap = MIN_WEAPON;

	// Found the weapon

	player->last_weapon = player->curr_weapon;
	player->random_club_frame = 0;
	player->curr_weapon = weap;
	if (isWW2GI())
	{
		SetGameVarID(g_iWeaponVarID, player->curr_weapon, player->GetActor(), snum); // snum is playerindex!
		if (player->curr_weapon >= 0)
		{
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[player->curr_weapon][snum], player->GetActor(), snum);
		}
		else
		{
			SetGameVarID(g_iWorksLikeVarID, -1, player->GetActor(), snum);
		}
		OnEvent(EVENT_CHANGEWEAPON, snum, player->i, -1);
	}

	player->okickback_pic = player->kickback_pic = 0;
	if (player->holster_weapon == 1)
	{
		player->holster_weapon = 0;
		player->weapon_pos = 10;
	}
	else player->weapon_pos = -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void clearcamera(player_struct* ps)
{
	ps->newowner = -1;
	ps->posx = ps->oposx;
	ps->posy = ps->oposy;
	ps->posz = ps->oposz;
	ps->angle.restore();
	updatesector(ps->posx, ps->posy, &ps->cursectnum);
	setpal(ps);

	DukeStatIterator it(STAT_ACTOR);
	while (auto k = it.Next())
	{
		if (k->s.picnum == TILE_CAMERA1)
			k->s.yvel = 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ssp(DDukeActor* const actor, unsigned int cliptype) //The set sprite function
{
	Collision c;

	return movesprite_ex(actor,
		(actor->s.xvel * (sintable[(actor->s.ang + 512) & 2047])) >> 14,
		(actor->s.xvel * (sintable[actor->s.ang & 2047])) >> 14, actor->s.zvel,
		cliptype, c) == kHitNone;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void insertspriteq(DDukeActor* const actor)
{
	if (spriteqamount > 0)
	{
		if (spriteq[spriteqloc] != nullptr)
		{
			// Why is this not deleted here?
			// Also todo: Make list size a CVAR.
			spriteq[spriteqloc]->s.xrepeat = 0;
			// deletesprite(spriteq[spriteqloc]);
		}
		spriteq[spriteqloc] = actor;
		spriteqloc = (spriteqloc + 1) % spriteqamount;
	}
	else actor->s.xrepeat = actor->s.yrepeat = 0;
}

//---------------------------------------------------------------------------
//
// consolidation of several nearly identical functions
//
//---------------------------------------------------------------------------

void lotsofstuff(DDukeActor* actor, int n, int spawntype)
{
	auto s = &actor->s;
	for (int i = n; i > 0; i--)
	{
		int r1 = krand(), r2 = krand();	// using the RANDCORRECT version from RR.
		auto j = EGS(s->sectnum, s->x, s->y, s->z - (r2 % (47 << 8)), spawntype, -32, 8, 8, r1 & 2047, 0, 0, actor, 5);
		j->s.cstat = krand() & 12;
	}
}

//---------------------------------------------------------------------------
//
// movesector - why is this in actors.cpp?
//
//---------------------------------------------------------------------------

void ms(DDukeActor* const actor)
{
	//T1,T2 and T3 are used for all the sector moving stuff!!!

	short startwall, endwall, x;
	int tx, ty;
	auto s = &actor->s;

	s->x += (s->xvel * (sintable[(s->ang + 512) & 2047])) >> 14;
	s->y += (s->xvel * (sintable[s->ang & 2047])) >> 14;

	int j = actor->temp_data[1];
	int k = actor->temp_data[2];

	startwall = sector[s->sectnum].wallptr;
	endwall = startwall + sector[s->sectnum].wallnum;
	for (x = startwall; x < endwall; x++)
	{
		rotatepoint(
			0, 0,
			msx[j], msy[j],
			k & 2047, &tx, &ty);

		dragpoint(x, s->x + tx, s->y + ty);

		j++;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movecyclers(void)
{
	short q, j, x, t, s, * c;
	walltype* wal;
	char cshade;

	for (q = numcyclers - 1; q >= 0; q--)
	{

		c = &cyclers[q][0];
		s = c[0];

		t = c[3];
		j = t + (sintable[c[1] & 2047] >> 10);
		cshade = c[2];

		if (j < cshade) j = cshade;
		else if (j > t)  j = t;

		c[1] += sector[s].extra;
		if (c[5])
		{
			wal = &wall[sector[s].wallptr];
			for (x = sector[s].wallnum; x > 0; x--, wal++)
				if (wal->hitag != 1)
				{
					wal->shade = j;

					if ((wal->cstat & CSTAT_WALL_BOTTOM_SWAP) && wal->nextwall >= 0)
						wall[wal->nextwall].shade = j;

				}
			sector[s].floorshade = sector[s].ceilingshade = j;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movedummyplayers(void)
{
	int p;

	DukeStatIterator iti(STAT_DUMMYPLAYER);
	while (auto act = iti.Next())
	{
		if (!act->GetOwner()) continue;
		p = act->GetOwner()->PlayerIndex();
		auto spri = &act->s;

		if ((!isRR() && ps[p].on_crane != nullptr) || sector[ps[p].cursectnum].lotag != 1 || ps->GetActor()->s.extra <= 0)
		{
			ps[p].dummyplayersprite = -1;
			deletesprite(act);
			continue;
		}
		else
		{
			if (ps[p].on_ground && ps[p].on_warping_sector == 1 && sector[ps[p].cursectnum].lotag == 1)
			{
				spri->cstat = CSTAT_SPRITE_BLOCK_ALL;
				spri->z = sector[spri->sectnum].ceilingz + (27 << 8);
				spri->ang = ps[p].angle.ang.asbuild();
				if (act->temp_data[0] == 8)
					act->temp_data[0] = 0;
				else act->temp_data[0]++;
			}
			else
			{
				if (sector[spri->sectnum].lotag != 2) spri->z = sector[spri->sectnum].floorz;
				spri->cstat = (short)32768;
			}
		}

		spri->x += (ps[p].posx - ps[p].oposx);
		spri->y += (ps[p].posy - ps[p].oposy);
		setsprite(act, spri->pos);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveplayers(void)
{
	int otherx;

	DukeStatIterator iti(STAT_PLAYER);
	while (auto act = iti.Next())
	{
		int pn = act->PlayerIndex();
		auto p = &ps[pn];
		auto spri = &act->s;
		if (act->GetOwner())
		{
			if (p->newowner >= 0) //Looking thru the camera
			{
				spri->x = p->oposx;
				spri->y = p->oposy;
				act->bposz = spri->z = p->oposz + PHEIGHT;
				spri->ang = p->angle.oang.asbuild();
				setsprite(act, spri->pos);
			}
			else
			{
				if (ud.multimode > 1)
					otherp = findotherplayer(pn, &otherx);
				else
				{
					otherp = pn;
					otherx = 0;
				}

				execute(act, pn, otherx);

				if (ud.multimode > 1)
				{
					auto psp = ps[otherp].GetActor();
					if (psp->s.extra > 0)
					{
						if (spri->yrepeat > 32 && psp->s.yrepeat < 32)
						{
							if (otherx < 1400 && p->knee_incs == 0)
							{
								p->knee_incs = 1;
								p->weapon_pos = -1;
								p->actorsqu = ps[otherp].GetActor();
							}
						}
					}
				}
				if (ud.god)
				{
					spri->extra = max_player_health;
					spri->cstat = 257;
					if (!isWW2GI() && !isRR())
						p->jetpack_amount = 1599;
				}

				if (p->actorsqu != nullptr)
				{
					p->angle.addadjustment(FixedToFloat(getincangleq16(p->angle.ang.asq16(), gethiq16angle(p->actorsqu->s.x - p->posx, p->actorsqu->s.y - p->posy)) >> 2));
				}

				if (spri->extra > 0)
				{
					// currently alive...

					act->SetHitOwner(act);

					if (ud.god == 0)
						if (fi.ceilingspace(spri->sectnum) || fi.floorspace(spri->sectnum))
							quickkill(p);
				}
				else
				{
					p->posx = spri->x;
					p->posy = spri->y;
					p->posz = spri->z - (20 << 8);

					p->newowner = -1;

					if (p->wackedbyactor != nullptr && p->wackedbyactor->s.statnum < MAXSTATUS)
					{
						p->angle.addadjustment(FixedToFloat(getincangleq16(p->angle.ang.asq16(), gethiq16angle(p->wackedbyactor->s.x - p->posx, p->wackedbyactor->s.y - p->posy)) >> 1));
					}
				}
				spri->ang = p->angle.ang.asbuild();
			}
		}
		else
		{
			if (p->holoduke_on == nullptr)
			{
				deletesprite(act);
				continue;
			}

			act->bposx = spri->x;
			act->bposy = spri->y;
			act->bposz = spri->z;

			spri->cstat = 0;

			if (spri->xrepeat < 42)
			{
				spri->xrepeat += 4;
				spri->cstat |= 2;
			}
			else spri->xrepeat = 42;
			if (spri->yrepeat < 36)
				spri->yrepeat += 4;
			else
			{
				spri->yrepeat = 36;
				if (sector[spri->sectnum].lotag != ST_2_UNDERWATER)
					makeitfall(act);
				if (spri->zvel == 0 && sector[spri->sectnum].lotag == ST_1_ABOVE_WATER)
					spri->z += (32 << 8);
			}

			if (spri->extra < 8)
			{
				spri->xvel = 128;
				spri->ang = p->angle.ang.asbuild();
				spri->extra++;
				ssp(act, CLIPMASK0);
			}
			else
			{
				spri->ang = 2047 - (p->angle.ang.asbuild());
				setsprite(act, spri->pos);
			}
		}

		if (sector[spri->sectnum].ceilingstat & 1)
			spri->shade += (sector[spri->sectnum].ceilingshade - spri->shade) >> 1;
		else
			spri->shade += (sector[spri->sectnum].floorshade - spri->shade) >> 1;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefx(void)
{
	int p;
	int x, ht;

	DukeStatIterator iti(STAT_FX);
	while (auto act = iti.Next())
	{
		auto spri = &act->s;
		switch (spri->picnum)
		{
		case RESPAWN:
			if (spri->extra == 66)
			{
				auto j = spawn(act, spri->hitag);
				if (isRRRA())
				{
					respawn_rrra(act, j);
				}
				else
				{
					deletesprite(act);
				}
			}
			else if (spri->extra > (66 - 13))
				spri->extra++;
			break;

		case MUSICANDSFX:

			ht = spri->hitag;

			if (act->temp_data[1] != (int)SoundEnabled())
			{
				act->temp_data[1] = SoundEnabled();
				act->temp_data[0] = 0;
			}

			if (spri->lotag >= 1000 && spri->lotag < 2000)
			{
				x = ldist(ps[screenpeek].GetActor(), act);
				if (x < ht && act->temp_data[0] == 0)
				{
					FX_SetReverb(spri->lotag - 1000);
					act->temp_data[0] = 1;
				}
				if (x >= ht && act->temp_data[0] == 1)
				{
					FX_SetReverb(0);
					FX_SetReverbDelay(0);
					act->temp_data[0] = 0;
				}
			}
			else if (spri->lotag < 999 && (unsigned)sector[spri->sectnum].lotag < ST_9_SLIDING_ST_DOOR && snd_ambience && sector[spri->sectnum].floorz != sector[spri->sectnum].ceilingz)
			{
				int flags = S_GetUserFlags(spri->lotag);
				if (flags & SF_MSFX)
				{
					int x = dist(ps[screenpeek].GetActor(), act);

					if (x < ht && act->temp_data[0] == 0)
					{
						// Start playing an ambience sound.
						S_PlayActorSound(spri->lotag, act, CHAN_AUTO, CHANF_LOOP);
						act->temp_data[0] = 1;  // AMBIENT_SFX_PLAYING
					}
					else if (x >= ht && act->temp_data[0] == 1)
					{
						// Stop playing ambience sound because we're out of its range.
						S_StopSound(spri->lotag, act);
					}
				}

				if ((flags & (SF_GLOBAL | SF_DTAG)) == SF_GLOBAL)
				{
					if (act->temp_data[4] > 0) act->temp_data[4]--;
					else for (p = connecthead; p >= 0; p = connectpoint2[p])
						if (p == myconnectindex && ps[p].cursectnum == spri->sectnum)
						{
							S_PlaySound(spri->lotag + (unsigned)global_random % (spri->hitag + 1));
							act->temp_data[4] = 26 * 40 + (global_random % (26 * 40));
						}
				}
			}
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
// split out of movestandables
//
//---------------------------------------------------------------------------

void movecrane(DDukeActor *actor, int crane)
{
	int* t = &actor->temp_data[0];
	auto spri = &actor->s;
	int sect = spri->sectnum;
	int x;

	//t[0] = state
	//t[1] = checking sector number

	if (spri->xvel) getglobalz(actor);

	if (t[0] == 0) //Waiting to check the sector
	{
		DukeSectIterator it(t[1]);
		while (auto a2 = it.Next())
		{
			switch (a2->s.statnum)
			{
			case STAT_ACTOR:
			case STAT_ZOMBIEACTOR:
			case STAT_STANDABLE:
			case STAT_PLAYER:
				spri->ang = getangle(msx[t[4] + 1] - spri->x, msy[t[4] + 1] - spri->y);
				setsprite(a2, msx[t[4] + 1], msy[t[4] + 1], a2->s.z);
				t[0]++;
				return;
			}
		}
	}

	else if (t[0] == 1)
	{
		if (spri->xvel < 184)
		{
			spri->picnum = crane + 1;
			spri->xvel += 8;
		}
		//IFMOVING;	// JBF 20040825: see my rant above about this
		ssp(actor, CLIPMASK0);
		if (sect == t[1])
			t[0]++;
	}
	else if (t[0] == 2 || t[0] == 7)
	{
		spri->z += (1024 + 512);

		if (t[0] == 2)
		{
			if ((sector[sect].floorz - spri->z) < (64 << 8))
				if (spri->picnum > crane) spri->picnum--;

			if ((sector[sect].floorz - spri->z) < (4096 + 1024))
				t[0]++;
		}
		if (t[0] == 7)
		{
			if ((sector[sect].floorz - spri->z) < (64 << 8))
			{
				if (spri->picnum > crane) spri->picnum--;
				else
				{
					if (actor->IsActiveCrane())
					{
						int p = findplayer(actor, &x);
						S_PlayActorSound(isRR() ? 390 : DUKE_GRUNT, ps[p].GetActor());
						if (ps[p].on_crane == actor)
							ps[p].on_crane = nullptr;
					}
					t[0]++;
					actor->SetOwner(nullptr);
				}
			}
		}
	}
	else if (t[0] == 3)
	{
		spri->picnum++;
		if (spri->picnum == (crane + 2))
		{
			int p = checkcursectnums(t[1]);
			if (p >= 0 && ps[p].on_ground)
			{
				actor->SetActiveCrane(true);
				ps[p].on_crane = actor;
				S_PlayActorSound(isRR() ? 390 : DUKE_GRUNT, ps[p].GetActor());
				ps[p].angle.addadjustment(spri->ang + 1024);
			}
			else
			{
				DukeSectIterator it(t[1]);
				while (auto a2 = it.Next())
				{
					switch (a2->s.statnum)
					{
					case 1:
					case 6:
						actor->SetOwner(a2);
						break;
					}
				}
			}

			t[0]++;//Grabbed the sprite
			t[2] = 0;
			return;
		}
	}
	else if (t[0] == 4) //Delay before going up
	{
		t[2]++;
		if (t[2] > 10)
			t[0]++;
	}
	else if (t[0] == 5 || t[0] == 8)
	{
		if (t[0] == 8 && spri->picnum < (crane + 2))
			if ((sector[sect].floorz - spri->z) > 8192)
				spri->picnum++;

		if (spri->z < msx[t[4] + 2])
		{
			t[0]++;
			spri->xvel = 0;
		}
		else
			spri->z -= (1024 + 512);
	}
	else if (t[0] == 6)
	{
		if (spri->xvel < 192)
			spri->xvel += 8;
		spri->ang = getangle(msx[t[4]] - spri->x, msy[t[4]] - spri->y);
		//IFMOVING;	// JBF 20040825: see my rant above about this
		ssp(actor, CLIPMASK0);
		if (((spri->x - msx[t[4]]) * (spri->x - msx[t[4]]) + (spri->y - msy[t[4]]) * (spri->y - msy[t[4]])) < (128 * 128))
			t[0]++;
	}

	else if (t[0] == 9)
		t[0] = 0;

	setsprite(msy[t[4] + 2], spri->x, spri->y, spri->z - (34 << 8));

	auto Owner = actor->GetOwner();
	if (Owner != nullptr)
	{
		int p = findplayer(actor, &x);

		int j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{
			if (actor->IsActiveCrane())
				if (ps[p].on_crane == actor)
					ps[p].on_crane = nullptr;
			actor->SetActiveCrane(false);
			spri->picnum = crane;
			return;
		}

		auto a_owner = actor->GetOwner();
		if (a_owner != nullptr)
		{
			setsprite(a_owner, spri->pos);

			a_owner->bposx = spri->x;
			a_owner->bposy = spri->y;
			a_owner->bposz = spri->z;

			spri->zvel = 0;
		}
		else if (actor->IsActiveCrane())
		{
			auto ang = ps[p].angle.ang.asbuild();
			ps[p].oposx = ps[p].posx;
			ps[p].oposy = ps[p].posy;
			ps[p].oposz = ps[p].posz;
			ps[p].posx = spri->x - (sintable[(ang + 512) & 2047] >> 6);
			ps[p].posy = spri->y - (sintable[ang & 2047] >> 6);
			ps[p].posz = spri->z + (2 << 8);
			setsprite(ps[p].GetActor(), ps[p].posx, ps[p].posy, ps[p].posz);
			ps[p].cursectnum = ps[p].GetActor()->s.sectnum;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefountain(int i, int fountain)
{
	auto t = &hittype[i].temp_data[0];
	auto s = &sprite[i];
	int x;
	if (t[0] > 0)
	{
		if (t[0] < 20)
		{
			t[0]++;

			s->picnum++;

			if (s->picnum == fountain + 3)
				s->picnum = fountain + 1;
		}
		else
		{
			findplayer(s, &x);

			if (x > 512)
			{
				t[0] = 0;
				s->picnum = fountain;
			}
			else t[0] = 1;
		}
	}
}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveflammable(int i, int tire, int box, int pool)
{
	auto s = &sprite[i];
	auto ht = &hittype[i];
	int j;
	if (ht->temp_data[0] == 1)
	{
		ht->temp_data[1]++;
		if ((ht->temp_data[1] & 3) > 0) return;

		if (!isRR() && s->picnum == tire && ht->temp_data[1] == 32)
		{
			s->cstat = 0;
			j = fi.spawn(i, pool);
			sprite[j].shade = 127;
		}
		else
		{
			if (s->shade < 64) s->shade++;
			else
			{
				deletesprite(i);
				return;
			}
		}

		j = s->xrepeat - (krand() & 7);
		if (j < 10)
		{
			deletesprite(i);
			return;
		}

		s->xrepeat = j;

		j = s->yrepeat - (krand() & 7);
		if (j < 4)
		{
			deletesprite(i);
			return;
		}
		s->yrepeat = j;
	}
	if (box >= 0 && s->picnum == box)
	{
		makeitfall(i);
		ht->ceilingz = sector[s->sectnum].ceilingz;
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void detonate(int i, int explosion)
{
	auto s = &sprite[i];
	auto ht = &hittype[i];
	auto t = &hittype[i].temp_data[0];
	earthquaketime = 16;

	int j;
	StatIterator itj(STAT_EFFECTOR);
	while ((j = itj.NextIndex()) >= 0)
	{
		auto sj = &sprite[j];
		auto htj = &hittype[j];
		if (s->hitag == sj->hitag)
		{
			if (sj->lotag == SE_13_EXPLOSIVE)
			{
				if (htj->temp_data[2] == 0)
					htj->temp_data[2] = 1;
			}
			else if (sj->lotag == SE_8_UP_OPEN_DOOR_LIGHTS)
				htj->temp_data[4] = 1;
			else if (sj->lotag == SE_18_INCREMENTAL_SECTOR_RISE_FALL)
			{
				if (htj->temp_data[0] == 0)
					htj->temp_data[0] = 1;
			}
			else if (sj->lotag == SE_21_DROP_FLOOR)
				htj->temp_data[0] = 1;
		}
	}

	s->z -= (32 << 8);

	if ((t[3] == 1 && s->xrepeat) || s->lotag == -99)
	{
		int x = s->extra;
		fi.spawn(i, explosion);
		fi.hitradius(i, seenineblastradius, x >> 2, x - (x >> 1), x - (x >> 2), x);
		S_PlayActorSound(PIPEBOMB_EXPLODE, i);
	}

	if (s->xrepeat)
		for (int x = 0; x < 8; x++) RANDOMSCRAP(s, i);

	deletesprite(i);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movemasterswitch(int i, int spectype1, int spectype2)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	if (s->yvel == 1)
	{
		s->hitag--;
		if (s->hitag <= 0)
		{
			operatesectors(s->sectnum, i);

			SectIterator it(s->sectnum);
			int j;
			while ((j = it.NextIndex()) >= 0)
			{
				auto sj = &sprite[j];
				if (sj->statnum == 3)
				{
					switch (sj->lotag)
					{
					case SE_2_EARTHQUAKE:
					case SE_21_DROP_FLOOR:
					case SE_31_FLOOR_RISE_FALL:
					case SE_32_CEILING_RISE_FALL:
					case SE_36_PROJ_SHOOTER:
						hittype[j].temp_data[0] = 1;
						break;
					case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
						hittype[j].temp_data[4] = 1;
						break;
					}
				}
				else if (sj->statnum == 6)
				{
					if (sj->picnum == spectype1 || sj->picnum == spectype2) // SEENINE and OOZFILTER
					{
						sj->shade = -31;
					}
				}
			}
			deletesprite(i);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movetrash(int i)
{
	auto s = &sprite[i];
	if (s->xvel == 0) s->xvel = 1;
	if (ssp(i, CLIPMASK0))
	{
		makeitfall(i);
		if (krand() & 1) s->zvel -= 256;
		if (abs(s->xvel) < 48)
			s->xvel += (krand() & 3);
	}
	else deletesprite(i);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movewaterdrip(int i, int drip)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;

	if (t[1])
	{
		t[1]--;
		if (t[1] == 0)
			s->cstat &= 32767;
	}
	else
	{
		makeitfall(i);
		ssp(i, CLIPMASK0);
		if (s->xvel > 0) s->xvel -= 2;

		if (s->zvel == 0)
		{
			s->cstat |= 32768;

			if (s->pal != 2 && (isRR() || s->hitag == 0))
				S_PlayActorSound(SOMETHING_DRIPPING, i);

			if (sprite[s->owner].picnum != drip)
			{
				deletesprite(i);
			}
			else
			{
				hittype[i].bposz = s->z = t[0];
				t[1] = 48 + (krand() & 31);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movedoorshock(int i)
{
	auto s = &sprite[i];
	int sect = s->sectnum;
	int j = abs(sector[sect].ceilingz - sector[sect].floorz) >> 9;
	s->yrepeat = j + 4;
	s->xrepeat = 16;
	s->z = sector[sect].floorz;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movetouchplate(int i, int plate)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	int x;
	int p;

	if (t[1] == 1 && s->hitag >= 0) //Move the sector floor
	{
		x = sector[sect].floorz;

		if (t[3] == 1)
		{
			if (x >= t[2])
			{
				sector[sect].floorz = x;
				t[1] = 0;
			}
			else
			{
				sector[sect].floorz += sector[sect].extra;
				p = checkcursectnums(sect);
				if (p >= 0) ps[p].posz += sector[sect].extra;
			}
		}
		else
		{
			if (x <= s->z)
			{
				sector[sect].floorz = s->z;
				t[1] = 0;
			}
			else
			{
				sector[sect].floorz -= sector[sect].extra;
				p = checkcursectnums(sect);
				if (p >= 0)
					ps[p].posz -= sector[sect].extra;
			}
		}
		return;
	}

	if (t[5] == 1) return;

	p = checkcursectnums(sect);
	if (p >= 0 && (ps[p].on_ground || s->ang == 512))
	{
		if (t[0] == 0 && !check_activator_motion(s->lotag))
		{
			t[0] = 1;
			t[1] = 1;
			t[3] = !t[3];
			operatemasterswitches(s->lotag);
			operateactivators(s->lotag, p);
			if (s->hitag > 0)
			{
				s->hitag--;
				if (s->hitag == 0) t[5] = 1;
			}
		}
	}
	else t[0] = 0;

	if (t[1] == 1)
	{
		StatIterator it(STAT_STANDABLE);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			if (j != i && sprite[j].picnum == plate && sprite[j].lotag == s->lotag)
			{
				hittype[j].temp_data[1] = 1;
				hittype[j].temp_data[3] = t[3];
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveooz(int i, int seenine, int seeninedead, int ooz, int explosion)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int j;
	if (s->shade != -32 && s->shade != -33)
	{
		if (s->xrepeat)
			j = (fi.ifhitbyweapon(&hittype[i]) >= 0);
		else
			j = 0;

		if (j || s->shade == -31)
		{
			if (j) s->lotag = 0;

			t[3] = 1;

			StatIterator it(STAT_STANDABLE);
			int j;
			while ((j = it.NextIndex()) >= 0)
			{
				auto ss = &sprite[j];
				if (s->hitag == ss->hitag && (ss->picnum == seenine || ss->picnum == ooz))
					ss->shade = -32;
			}
		}
	}
	else
	{
		if (s->shade == -32)
		{
			if (s->lotag > 0)
			{
				s->lotag -= 3;
				if (s->lotag <= 0) s->lotag = -99;
			}
			else
				s->shade = -33;
		}
		else
		{
			if (s->xrepeat > 0)
			{
				hittype[i].temp_data[2]++;
				if (hittype[i].temp_data[2] == 3)
				{
					if (s->picnum == ooz)
					{
						hittype[i].temp_data[2] = 0;
						detonate(i, explosion);
						return;
					}
					if (s->picnum != (seeninedead + 1))
					{
						hittype[i].temp_data[2] = 0;

						if (s->picnum == seeninedead) s->picnum++;
						else if (s->picnum == seenine)
							s->picnum = seeninedead;
					}
					else
					{
						detonate(i, explosion);
						return;
					}
				}
				return;
			}
			detonate(i, explosion);
			return;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movecanwithsomething(int i)
{
	auto s = &sprite[i];
	makeitfall(i);
	int j = fi.ifhitbyweapon(&hittype[i]);
	if (j >= 0)
	{
		S_PlayActorSound(VENT_BUST, i);
		for (j = 0; j < 10; j++)
			RANDOMSCRAP(s, i);

		if (s->lotag) fi.spawn(i, s->lotag);

		deletesprite(i);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void bounce(int i)
{
	int k, l, daang, dax, day, daz, xvect, yvect, zvect;
	int hitsect;
	spritetype* s = &sprite[i];

	xvect = mulscale10(s->xvel, sintable[(s->ang + 512) & 2047]);
	yvect = mulscale10(s->xvel, sintable[s->ang & 2047]);
	zvect = s->zvel;

	hitsect = s->sectnum;

	k = sector[hitsect].wallptr; l = wall[k].point2;
	daang = getangle(wall[l].x - wall[k].x, wall[l].y - wall[k].y);

	if (s->z < (hittype[i].floorz + hittype[i].ceilingz) >> 1)
		k = sector[hitsect].ceilingheinum;
	else
		k = sector[hitsect].floorheinum;

	dax = mulscale14(k, sintable[(daang) & 2047]);
	day = mulscale14(k, sintable[(daang + 1536) & 2047]);
	daz = 4096;

	k = xvect * dax + yvect * day + zvect * daz;
	l = dax * dax + day * day + daz * daz;
	if ((abs(k) >> 14) < l)
	{
		k = divscale17(k, l);
		xvect -= mulscale16(dax, k);
		yvect -= mulscale16(day, k);
		zvect -= mulscale16(daz, k);
	}

	s->zvel = zvect;
	s->xvel = ksqrt(dmulscale8(xvect, xvect, yvect, yvect));
	s->ang = getangle(xvect, yvect);
}

//---------------------------------------------------------------------------
//
// taken out of moveweapon
//
//---------------------------------------------------------------------------

void movetongue(int i, int tongue, int jaw)
{
	spritetype* s = &sprite[i];

	hittype[i].temp_data[0] = sintable[(hittype[i].temp_data[1]) & 2047] >> 9;
	hittype[i].temp_data[1] += 32;
	if (hittype[i].temp_data[1] > 2047)
	{
		deletesprite(i);
		return;
	}

	if (sprite[s->owner].statnum == MAXSTATUS)
		if (badguy(&sprite[s->owner]) == 0)
		{
			deletesprite(i);
			return;
		}

	s->ang = sprite[s->owner].ang;
	s->x = sprite[s->owner].x;
	s->y = sprite[s->owner].y;
	if (sprite[s->owner].picnum == TILE_APLAYER)
		s->z = sprite[s->owner].z - (34 << 8);
	for (int k = 0; k < hittype[i].temp_data[0]; k++)
	{
		int q = EGS(s->sectnum,
			s->x + ((k * sintable[(s->ang + 512) & 2047]) >> 9),
			s->y + ((k * sintable[s->ang & 2047]) >> 9),
			s->z + ((k * ksgn(s->zvel)) * abs(s->zvel / 12)), tongue, -40 + (k << 1),
			8, 8, 0, 0, 0, i, 5);
		sprite[q].cstat = 128;
		sprite[q].pal = 8;
	}
	int k = hittype[i].temp_data[0];	// do not depend on the above loop counter.
	int q = EGS(s->sectnum,
		s->x + ((k * sintable[(s->ang + 512) & 2047]) >> 9),
		s->y + ((k * sintable[s->ang & 2047]) >> 9),
		s->z + ((k * ksgn(s->zvel)) * abs(s->zvel / 12)), jaw, -40,
		32, 32, 0, 0, 0, i, 5);
	sprite[q].cstat = 128;
	if (hittype[i].temp_data[1] > 512 && hittype[i].temp_data[1] < (1024))
		sprite[q].picnum = jaw + 1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void rpgexplode(int i, int j, const vec3_t &pos, int EXPLOSION2, int EXPLOSION2BOT, int newextra, int playsound)
{
	auto act = &hittype[i];
	auto s = &act->s;
	auto k = spawn(act, EXPLOSION2);
	k->s.pos = pos;

	if (s->xrepeat < 10)
	{
		k->s.xrepeat = 6;
		k->s.yrepeat = 6;
	}
	else if ((j & kHitTypeMask) == kHitSector)
	{
		if (s->zvel > 0 && EXPLOSION2BOT >= 0)
			fi.spawn(i, EXPLOSION2BOT);
		else
		{
			k->s.cstat |= 8;
			k->s.z += (48 << 8);
		}
	}
	if (newextra > 0) s->extra = newextra;
	S_PlayActorSound(playsound, i);

	if (s->xrepeat >= 10)
	{
		int x = s->extra;
		fi.hitradius(i, rpgblastradius, x >> 2, x >> 1, x - (x >> 2), x);
	}
	else
	{
		int x = s->extra + (global_random & 3);
		fi.hitradius(i, (rpgblastradius >> 1), x >> 2, x >> 1, x - (x >> 2), x);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool respawnmarker(int i, int yellow, int green)
{
	hittype[i].temp_data[0]++;
	if (hittype[i].temp_data[0] > respawnitemtime)
	{
		deletesprite(i);
		return false;
	}
	if (hittype[i].temp_data[0] >= (respawnitemtime >> 1) && hittype[i].temp_data[0] < ((respawnitemtime >> 1) + (respawnitemtime >> 2)))
		sprite[i].picnum = yellow;
	else if (hittype[i].temp_data[0] > ((respawnitemtime >> 1) + (respawnitemtime >> 2)))
		sprite[i].picnum = green;
	makeitfall(i);
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool rat(int i, bool makesound)
{
	spritetype* s = &sprite[i];
	makeitfall(i);
	if (ssp(i, CLIPMASK0))
	{
		if (makesound && (krand() & 255) == 0) S_PlayActorSound(RATTY, i);
		s->ang += (krand() & 31) - 15 + (sintable[(hittype[i].temp_data[0] << 8) & 2047] >> 11);
	}
	else
	{
		hittype[i].temp_data[0]++;
		if (hittype[i].temp_data[0] > 1)
		{
			deletesprite(i);
			return false;
		}
		else s->ang = (krand() & 2047);
	}
	if (s->xvel < 128)
		s->xvel += 2;
	s->ang += (krand() & 3) - 6;
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool queball(int i, int pocket, int queball, int stripeball)
{
	spritetype* s = &sprite[i];
	if (s->xvel)
	{
		StatIterator it(STAT_DEFAULT);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			if (sprite[j].picnum == pocket && ldist(&sprite[j], s) < 52)
			{
				deletesprite(i);
				return false;
			}
		}

		j = clipmove(&s->x, &s->y, &s->z, &s->sectnum,
			(((s->xvel * (sintable[(s->ang + 512) & 2047])) >> 14) * TICSPERFRAME) << 11,
			(((s->xvel * (sintable[s->ang & 2047])) >> 14) * TICSPERFRAME) << 11,
			24L, (4 << 8), (4 << 8), CLIPMASK1);

		if (j & 49152)
		{
			if ((j & 49152) == 32768)
			{
				j &= (MAXWALLS - 1);
				int k = getangle(
					wall[wall[j].point2].x - wall[j].x,
					wall[wall[j].point2].y - wall[j].y);
				s->ang = ((k << 1) - s->ang) & 2047;
			}
			else if ((j & 49152) == 49152)
			{
				j &= (MAXSPRITES - 1);
				fi.checkhitsprite(i, j);
			}
		}
		s->xvel--;
		if (s->xvel < 0) s->xvel = 0;
		if (s->picnum == stripeball)
		{
			s->cstat = 257;
			s->cstat |= 4 & s->xvel;
			s->cstat |= 8 & s->xvel;
		}
	}
	else
	{
		int x;
		int p = findplayer(s, &x);

		if (x < 1596)
		{

			//						if(s->pal == 12)
			{
				int j = getincangle(ps[p].angle.ang.asbuild(), getangle(s->x - ps[p].posx, s->y - ps[p].posy));
				if (j > -64 && j < 64 && PlayerInput(p, SB_OPEN))
					if (ps[p].toggle_key_flag == 1)
					{
						StatIterator it(STAT_ACTOR);
						int a;
						while ((a = it.NextIndex()) >= 0)
						{
							auto sa = &sprite[a];
							if (sa->picnum == queball || sa->picnum == stripeball)
							{
								j = getincangle(ps[p].angle.ang.asbuild(), getangle(sa->x - ps[p].posx, sa->y - ps[p].posy));
								if (j > -64 && j < 64)
								{
									int l;
									findplayer(sa, &l);
									if (x > l) break;
								}
							}
						}
						if (a == -1)
						{
							if (s->pal == 12)
								s->xvel = 164;
							else s->xvel = 140;
							s->ang = ps[p].angle.ang.asbuild();
							ps[p].toggle_key_flag = 2;
						}
					}
			}
		}
		if (x < 512 && s->sectnum == ps[p].cursectnum)
		{
			s->ang = getangle(s->x - ps[p].posx, s->y - ps[p].posy);
			s->xvel = 48;
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void forcesphere(int i, int forcesphere)
{
	auto act = &hittype[i];
	auto s = &act->s;
	auto t = &act->temp_data[0];
	int sect = s->sectnum;
	if (s->yvel == 0)
	{
		s->yvel = 1;

		for (int l = 512; l < (2048 - 512); l += 128)
			for (int j = 0; j < 2048; j += 128)
			{
				auto k = spawn(act, forcesphere);
				k->s.cstat = 257 + 128;
				k->s.clipdist = 64;
				k->s.ang = j;
				k->s.zvel = sintable[l & 2047] >> 5;
				k->s.xvel = sintable[(l + 512) & 2047] >> 9;
				k->s.owner = i;
			}
	}

	if (t[3] > 0)
	{
		if (s->zvel < 6144)
			s->zvel += 192;
		s->z += s->zvel;
		if (s->z > sector[sect].floorz)
			s->z = sector[sect].floorz;
		t[3]--;
		if (t[3] == 0)
		{
			deletesprite(i);
			return;
		}
		else if (t[2] > 10)
		{
			StatIterator it(STAT_MISC);
			int j;
			while ((j = it.NextIndex()) >= 0)
			{
				if (sprite[j].owner == i && sprite[j].picnum == forcesphere)
					hittype[j].temp_data[1] = 1 + (krand() & 63);
			}
			t[3] = 64;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void recon(int i, int explosion, int firelaser, int attacksnd, int painsnd, int roamsnd, int shift, int (*getspawn)(int i))
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	int j, a;

	getglobalz(i);

	if (sector[s->sectnum].ceilingstat & 1)
		s->shade += (sector[s->sectnum].ceilingshade - s->shade) >> 1;
	else s->shade += (sector[s->sectnum].floorshade - s->shade) >> 1;

	if (s->z < sector[sect].ceilingz + (32 << 8))
		s->z = sector[sect].ceilingz + (32 << 8);

	if (ud.multimode < 2)
	{
		if (actor_tog == 1)
		{
			s->cstat = (short)32768;
			return;
		}
		else if (actor_tog == 2) s->cstat = 257;
	}
	j = fi.ifhitbyweapon(&hittype[i]); if (j >= 0)
	{
		if (s->extra < 0 && t[0] != -1)
		{
			t[0] = -1;
			s->extra = 0;
		}
		if (painsnd >= 0) S_PlayActorSound(painsnd, i);
		RANDOMSCRAP(s, i);
	}

	if (t[0] == -1)
	{
		s->z += 1024;
		t[2]++;
		if ((t[2] & 3) == 0) fi.spawn(i, explosion);
		getglobalz(i);
		s->ang += 96;
		s->xvel = 128;
		j = ssp(i, CLIPMASK0);
		if (j != 1 || s->z > hittype[i].floorz)
		{
			for (int l = 0; l < 16; l++)
				RANDOMSCRAP(s, i);
			S_PlayActorSound(LASERTRIP_EXPLODE, i);
			int sp = getspawn(i);
			if (sp >= 0) fi.spawn(i, sp);
			ps[myconnectindex].actors_killed++;
			deletesprite(i);
		}
		return;
	}
	else
	{
		if (s->z > hittype[i].floorz - (48 << 8))
			s->z = hittype[i].floorz - (48 << 8);
	}

	int x;
	int p = findplayer(s, &x);
	j = s->owner;

	// 3 = findplayerz, 4 = shoot

	if (t[0] >= 4)
	{
		t[2]++;
		if ((t[2] & 15) == 0)
		{
			a = s->ang;
			s->ang = hittype[i].tempang;
			if (attacksnd >= 0) S_PlayActorSound(attacksnd, i);
			fi.shoot(i, firelaser);
			s->ang = a;
		}
		if (t[2] > (26 * 3) || !cansee(s->x, s->y, s->z - (16 << 8), s->sectnum, ps[p].posx, ps[p].posy, ps[p].posz, ps[p].cursectnum))
		{
			t[0] = 0;
			t[2] = 0;
		}
		else hittype[i].tempang +=
			getincangle(hittype[i].tempang, getangle(ps[p].posx - s->x, ps[p].posy - s->y)) / 3;
	}
	else if (t[0] == 2 || t[0] == 3)
	{
		t[3] = 0;
		if (s->xvel > 0) s->xvel -= 16;
		else s->xvel = 0;

		if (t[0] == 2)
		{
			int l = ps[p].posz - s->z;
			if (abs(l) < (48 << 8)) t[0] = 3;
			else s->z += sgn(ps[p].posz - s->z) << shift; // The shift here differs between Duke and RR.
		}
		else
		{
			t[2]++;
			if (t[2] > (26 * 3) || !cansee(s->x, s->y, s->z - (16 << 8), s->sectnum, ps[p].posx, ps[p].posy, ps[p].posz, ps[p].cursectnum))
			{
				t[0] = 1;
				t[2] = 0;
			}
			else if ((t[2] & 15) == 0 && attacksnd >= 0)
			{
				S_PlayActorSound(attacksnd, i);
				fi.shoot(i, firelaser);
			}
		}
		s->ang += getincangle(s->ang, getangle(ps[p].posx - s->x, ps[p].posy - s->y)) >> 2;
	}

	if (t[0] != 2 && t[0] != 3)
	{
		int l = ldist(&sprite[j], s);
		if (l <= 1524)
		{
			a = s->ang;
			s->xvel >>= 1;
		}
		else a = getangle(sprite[j].x - s->x, sprite[j].y - s->y);

		if (t[0] == 1 || t[0] == 4) // Found a locator and going with it
		{
			l = dist(&sprite[j], s);

			if (l <= 1524) { if (t[0] == 1) t[0] = 0; else t[0] = 5; }
			else
			{
				// Control speed here
				if (l > 1524) { if (s->xvel < 256) s->xvel += 32; }
				else
				{
					if (s->xvel > 0) s->xvel -= 16;
					else s->xvel = 0;
				}
			}

			if (t[0] < 2) t[2]++;

			if (x < 6144 && t[0] < 2 && t[2] > (26 * 4))
			{
				t[0] = 2 + (krand() & 2);
				t[2] = 0;
				hittype[i].tempang = s->ang;
			}
		}

		if (t[0] == 0 || t[0] == 5)
		{
			if (t[0] == 0)
				t[0] = 1;
			else t[0] = 4;
			j = s->owner = LocateTheLocator(s->hitag, -1);
			if (j == -1)
			{
				s->hitag = j = hittype[i].temp_data[5];
				s->owner = LocateTheLocator(j, -1);
				j = s->owner;
				if (j == -1)
				{
					deletesprite(i);
					return;
				}
			}
			else s->hitag++;
		}

		t[3] = getincangle(s->ang, a);
		s->ang += t[3] >> 3;

		if (s->z < sprite[j].z)
			s->z += 1024;
		else s->z -= 1024;
	}

	if (roamsnd >= 0 && S_CheckSoundPlaying(i, roamsnd) < 1)
		S_PlayActorSound(roamsnd, i);

	ssp(i, CLIPMASK0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ooz(int i)
{
	getglobalz(i);

	int j = (hittype[i].floorz - hittype[i].ceilingz) >> 9;
	if (j > 255) j = 255;

	int x = 25 - (j >> 1);
	if (x < 8) x = 8;
	else if (x > 48) x = 48;

	spritetype* s = &sprite[i];
	s->yrepeat = j;
	s->xrepeat = x;
	s->z = hittype[i].floorz;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void reactor(int i, int REACTOR, int REACTOR2, int REACTORBURNT, int REACTOR2BURNT, int REACTORSPARK, int REACTOR2SPARK)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;

	if (t[4] == 1)
	{
		SectIterator it(sect);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			auto sprj = &sprite[j];
			if (sprj->picnum == SECTOREFFECTOR)
			{
				if (sprj->lotag == 1)
				{
					sprj->lotag = (short)65535;
					sprj->hitag = (short)65535;
				}
			}
			else if (sprj->picnum == REACTOR)
			{
				sprj->picnum = REACTORBURNT;
			}
			else if (sprj->picnum == REACTOR2)
			{
				sprj->picnum = REACTOR2BURNT;
			}
			else if (sprj->picnum == REACTORSPARK || sprj->picnum == REACTOR2SPARK)
			{
				sprj->cstat = (short)32768;
			}
		}		return;
	}

	if (t[1] >= 20)
	{
		t[4] = 1;
		return;
	}

	int x;
	int p = findplayer(s, &x);

	t[2]++;
	if (t[2] == 4) t[2] = 0;

	if (x < 4096)
	{
		if ((krand() & 255) < 16)
		{
			if (!S_CheckSoundPlaying(DUKE_LONGTERM_PAIN))
				S_PlayActorSound(DUKE_LONGTERM_PAIN, ps[p].i);

			S_PlayActorSound(SHORT_CIRCUIT, i);

			sprite[ps[p].i].extra--;
			SetPlayerPal(&ps[p], PalEntry(32, 32, 0, 0));
		}
		t[0] += 128;
		if (t[3] == 0)
			t[3] = 1;
	}
	else t[3] = 0;

	if (t[1])
	{
		int j;
		t[1]++;

		t[4] = s->z;
		s->z = sector[sect].floorz - (krand() % (sector[sect].floorz - sector[sect].ceilingz));

		switch (t[1])
		{
		case 3:
		{
			//Turn on all of those flashing sectoreffector.
			fi.hitradius(i, 4096,
				impact_damage << 2,
				impact_damage << 2,
				impact_damage << 2,
				impact_damage << 2);
			StatIterator it(STAT_STANDABLE);
			int j;
			while ((j = it.NextIndex()) >= 0)
			{
				auto sj = &sprite[j];
				if (sj->picnum == MASTERSWITCH)
					if (sj->hitag == s->hitag)
						if (sj->yvel == 0)
							sj->yvel = 1;
			}
			break;
		}
		case 4:
		case 7:
		case 10:
		case 15:
		{
			SectIterator it(sect);
			while ((j = it.NextIndex()) >= 0)
			{
				if (j != i)
				{
					deletesprite(j);
					break;
				}
			}
			break;
		}
		}
		for (x = 0; x < 16; x++)
			RANDOMSCRAP(s, i);

		s->z = t[4];
		t[4] = 0;

	}
	else
	{
		int j = fi.ifhitbyweapon(&hittype[i]);
		if (j >= 0)
		{
			for (x = 0; x < 32; x++)
				RANDOMSCRAP(s, i);
			if (s->extra < 0)
				t[1] = 1;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void camera(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	if (t[0] == 0)
	{
		if (camerashitable)
		{
			int j = fi.ifhitbyweapon(&hittype[i]);
			if (j >= 0)
			{
				t[0] = 1; // static
				s->cstat = (short)32768;
				for (int x = 0; x < 5; x++)
					RANDOMSCRAP(s, i);
				return;
			}
		}

		// backup current angle for interpolating camera angle.
		hittype[i].tempang = s->ang;
		
		if (s->hitag > 0)
		{
			// alias our temp_data array indexes.
			auto& increment = t[1];
			auto& minimum = t[2];
			auto& maximum = t[3];
			auto& setupflag = t[4];

			// set up camera if already not.
			if (setupflag != 1)
			{
				increment = 8;
				minimum = s->ang - s->hitag - increment;
				maximum = s->ang + s->hitag - increment;
				setupflag = 1;
			}

			// update angle accordingly.
			if (s->ang == minimum || s->ang == maximum)
			{
				increment = -increment;
				s->ang += increment;
			}
			else if (s->ang + increment < minimum)
			{
				s->ang = minimum;
			}
			else if (s->ang + increment > maximum)
			{
				s->ang = maximum;
			}
			else
			{
				s->ang += increment;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// taken out of moveexplosion
//
//---------------------------------------------------------------------------

void forcesphere(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int l = s->xrepeat;
	if (t[1] > 0)
	{
		t[1]--;
		if (t[1] == 0)
		{
			deletesprite(i);
			return;
		}
	}
	if (hittype[s->owner].temp_data[1] == 0)
	{
		if (t[0] < 64)
		{
			t[0]++;
			l += 3;
		}
	}
	else
		if (t[0] > 64)
		{
			t[0]--;
			l -= 3;
		}

	s->x = sprite[s->owner].x;
	s->y = sprite[s->owner].y;
	s->z = sprite[s->owner].z;
	s->ang += hittype[s->owner].temp_data[0];

	if (l > 64) l = 64;
	else if (l < 1) l = 1;

	s->xrepeat = l;
	s->yrepeat = l;
	s->shade = (l >> 1) - 48;

	for (int j = t[0]; j > 0; j--)
		ssp(i, CLIPMASK0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void watersplash2(int i)
{
	spritetype* s = &sprite[i];
	int sect = s->sectnum;
	auto t = &hittype[i].temp_data[0];
	t[0]++;
	if (t[0] == 1)
	{
		if (sector[sect].lotag != 1 && sector[sect].lotag != 2)
		{
			deletesprite(i);
			return;
		}
		if (!S_CheckSoundPlaying(ITEM_SPLASH))
			S_PlayActorSound(ITEM_SPLASH, i);
	}
	if (t[0] == 3)
	{
		t[0] = 0;
		t[1]++;
	}
	if (t[1] == 5)
		deletesprite(i);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void frameeffect1(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	if (s->owner >= 0)
	{
		t[0]++;

		if (t[0] > 7)
		{
			deletesprite(i);
			return;
		}
		else if (t[0] > 4)
			s->cstat |= 512 + 2;
		else if (t[0] > 2)
			s->cstat |= 2;
		s->xoffset = sprite[s->owner].xoffset;
		s->yoffset = sprite[s->owner].yoffset;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool money(int i, int BLOODPOOL)
{
	spritetype* s = &sprite[i];
	int sect = s->sectnum;
	auto t = &hittype[i].temp_data[0];

	s->xvel = (krand() & 7) + (sintable[hittype[i].temp_data[0] & 2047] >> 9);
	hittype[i].temp_data[0] += (krand() & 63);
	if ((hittype[i].temp_data[0] & 2047) > 512 && (hittype[i].temp_data[0] & 2047) < 1596)
	{
		if (sector[sect].lotag == 2)
		{
			if (s->zvel < 64)
				s->zvel += (gc >> 5) + (krand() & 7);
		}
		else
			if (s->zvel < 144)
				s->zvel += (gc >> 5) + (krand() & 7);
	}

	ssp(i, CLIPMASK0);

	if ((krand() & 3) == 0)
		setsprite(i, s->x, s->y, s->z);

	if (s->sectnum == -1)
	{
		deletesprite(i);
		return false;
	}
	int l = getflorzofslope(s->sectnum, s->x, s->y);

	if (s->z > l)
	{
		s->z = l;

		insertspriteq(&hittype[i]);
		sprite[i].picnum++;

		StatIterator it(STAT_MISC);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			if (sprite[j].picnum == BLOODPOOL)
				if (ldist(s, &sprite[j]) < 348)
				{
					s->pal = 2;
					break;
				}
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool jibs(int i, int JIBS6, bool timeout, bool callsetsprite, bool floorcheck, bool zcheck1, bool zcheck2)
{
	spritetype* s = &sprite[i];
	int sect = s->sectnum;
	auto t = &hittype[i].temp_data[0];

	if (s->xvel > 0) s->xvel--;
	else s->xvel = 0;

	if (timeout)
	{
		if (t[5] < 30 * 10)
			t[5]++;
		else
		{
			deletesprite(i);
			return false;
		}
	}

	if (s->zvel > 1024 && s->zvel < 1280)
	{
		setsprite(i, s->x, s->y, s->z);
		sect = s->sectnum;
	}

	if (callsetsprite) setsprite(i, s->x, s->y, s->z);

	int l = getflorzofslope(sect, s->x, s->y);
	int x = getceilzofslope(sect, s->x, s->y);
	if (x == l || sect < 0 || sect >= MAXSECTORS)
	{
		deletesprite(i);
		return false;
	}

	if (s->z < l - (2 << 8))
	{
		if (t[1] < 2) t[1]++;
		else if (sector[sect].lotag != 2)
		{
			t[1] = 0;
			if (zcheck1)
			{
				if (t[0] > 6) t[0] = 0;
				else t[0]++;
			}
			else
			{
				if (t[0] > 2)
					t[0] = 0;
				else t[0]++;
			}
		}

		if (s->zvel < 6144)
		{
			if (sector[sect].lotag == 2)
			{
				if (s->zvel < 1024)
					s->zvel += 48;
				else s->zvel = 1024;
			}
			else s->zvel += gc - 50;
		}

		s->x += (s->xvel * sintable[(s->ang + 512) & 2047]) >> 14;
		s->y += (s->xvel * sintable[s->ang & 2047]) >> 14;
		s->z += s->zvel;

		if (floorcheck && s->z >= sector[s->sectnum].floorz)
		{
			deletesprite(i);
			return false;
		}
	}
	else
	{
		if (zcheck2)
		{
			deletesprite(i);
			return false;
		}
		if (t[2] == 0)
		{
			if (s->sectnum == -1)
			{
				deletesprite(i);
				return false;
			}
			if ((sector[s->sectnum].floorstat & 2))
			{
				deletesprite(i);
				return false;
			}
			t[2]++;
		}
		l = getflorzofslope(s->sectnum, s->x, s->y);

		s->z = l - (2 << 8);
		s->xvel = 0;

		if (s->picnum == JIBS6)
		{
			t[1]++;
			if ((t[1] & 3) == 0 && t[0] < 7)
				t[0]++;
			if (t[1] > 20)
			{
				deletesprite(i);
				return false;
			}
		}
		else { s->picnum = JIBS6; t[0] = 0; t[1] = 0; }
	}
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool bloodpool(int i, bool puke, int TIRE)
{
	spritetype* s = &sprite[i];
	int sect = s->sectnum;
	auto t = &hittype[i].temp_data[0];

	if (t[0] == 0)
	{
		t[0] = 1;
		if (sector[sect].floorstat & 2)
		{
			deletesprite(i);
			return false;
		}
		else insertspriteq(&hittype[i]);
	}

	makeitfall(i);

	int x;
	int p = findplayer(s, &x);

	s->z = hittype[i].floorz - (FOURSLEIGHT);

	if (t[2] < 32)
	{
		t[2]++;
		if (hittype[i].picnum == TIRE)
		{
			if (s->xrepeat < 64 && s->yrepeat < 64)
			{
				s->xrepeat += krand() & 3;
				s->yrepeat += krand() & 3;
			}
		}
		else
		{
			if (s->xrepeat < 32 && s->yrepeat < 32)
			{
				s->xrepeat += krand() & 3;
				s->yrepeat += krand() & 3;
			}
		}
	}

	if (x < 844 && s->xrepeat > 6 && s->yrepeat > 6)
	{
		if (s->pal == 0 && (krand() & 255) < 16 && !puke)
		{
			if (ps[p].boot_amount > 0)
				ps[p].boot_amount--;
			else
			{
				if (!S_CheckSoundPlaying(DUKE_LONGTERM_PAIN))
					S_PlayActorSound(DUKE_LONGTERM_PAIN, ps[p].i);
				sprite[ps[p].i].extra--;
				SetPlayerPal(&ps[p], PalEntry(32, 16, 0, 0));
			}
		}

		if (t[1] == 1) return false;
		t[1] = 1;

		if (hittype[i].picnum == TIRE)
			ps[p].footprintcount = 10;
		else ps[p].footprintcount = 3;

		ps[p].footprintpal = s->pal;
		ps[p].footprintshade = s->shade;

		if (t[2] == 32)
		{
			s->xrepeat -= 6;
			s->yrepeat -= 6;
		}
	}
	else t[1] = 0;
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void shell(int i, bool morecheck)
{
	spritetype* s = &sprite[i];
	int sect = s->sectnum;
	auto t = &hittype[i].temp_data[0];

	ssp(i, CLIPMASK0);

	if (sect < 0 || morecheck)
	{
		deletesprite(i);
		return;
	}

	if (sector[sect].lotag == 2)
	{
		t[1]++;
		if (t[1] > 8)
		{
			t[1] = 0;
			t[0]++;
			t[0] &= 3;
		}
		if (s->zvel < 128) s->zvel += (gc / 13); // 8
		else s->zvel -= 64;
		if (s->xvel > 0)
			s->xvel -= 4;
		else s->xvel = 0;
	}
	else
	{
		t[1]++;
		if (t[1] > 3)
		{
			t[1] = 0;
			t[0]++;
			t[0] &= 3;
		}
		if (s->zvel < 512) s->zvel += (gc / 3); // 52;
		if (s->xvel > 0)
			s->xvel--;
		else
		{
			deletesprite(i);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void glasspieces(int i)
{
	spritetype* s = &sprite[i];
	int sect = s->sectnum;
	auto t = &hittype[i].temp_data[0];

	makeitfall(i);

	if (s->zvel > 4096) s->zvel = 4096;
	if (sect < 0)
	{
		deletesprite(i);
		return;
	}

	if (s->z == hittype[i].floorz - (FOURSLEIGHT) && t[0] < 3)
	{
		s->zvel = -((3 - t[0]) << 8) - (krand() & 511);
		if (sector[sect].lotag == 2)
			s->zvel >>= 1;
		s->xrepeat >>= 1;
		s->yrepeat >>= 1;
		if (rnd(96))
			setsprite(i, s->x, s->y, s->z);
		t[0]++;//Number of bounces
	}
	else if (t[0] == 3)
	{
		deletesprite(i);
		return;
	}

	if (s->xvel > 0)
	{
		s->xvel -= 2;
		s->cstat = ((s->xvel & 3) << 2);
	}
	else s->xvel = 0;

	ssp(i, CLIPMASK0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void scrap(int i, int SCRAP1, int SCRAP6)
{
	spritetype* s = &sprite[i];
	int sect = s->sectnum;
	auto t = &hittype[i].temp_data[0];

	if (s->xvel > 0)
		s->xvel--;
	else s->xvel = 0;

	if (s->zvel > 1024 && s->zvel < 1280)
	{
		setsprite(i, s->x, s->y, s->z);
		sect = s->sectnum;
	}

	if (s->z < sector[sect].floorz - (2 << 8))
	{
		if (t[1] < 1) t[1]++;
		else
		{
			t[1] = 0;

			if (s->picnum < SCRAP6 + 8)
			{
				if (t[0] > 6)
					t[0] = 0;
				else t[0]++;
			}
			else
			{
				if (t[0] > 2)
					t[0] = 0;
				else t[0]++;
			}
		}
		if (s->zvel < 4096) s->zvel += gc - 50;
		s->x += (s->xvel * sintable[(s->ang + 512) & 2047]) >> 14;
		s->y += (s->xvel * sintable[s->ang & 2047]) >> 14;
		s->z += s->zvel;
	}
	else
	{
		if (s->picnum == SCRAP1 && s->yvel > 0)
		{
			int j = fi.spawn(i, s->yvel);
			setsprite(j, s->x, s->y, s->z);
			getglobalz(j);
			sprite[j].hitag = sprite[j].lotag = 0;
		}
		deletesprite(i);
	}
}

//---------------------------------------------------------------------------
//
// taken out of moveeffectors
//
//---------------------------------------------------------------------------

void handle_se00(int i, int LASERLINE)
{
	auto s = &sprite[i];
	auto ht = &hittype[i];
	int sect = s->sectnum;
	auto t = &ht->temp_data[0];
	sectortype *sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	int zchange = 0;

	int j = s->owner;
	auto sprowner = &sprite[j];

	if (sprowner->lotag == (short)65535)
	{
		deletesprite(i);
		return;
	}

	int q = sc->extra >> 3;
	int l = 0;

	if (sc->lotag == 30)
	{
		q >>= 2;

		if (sprite[i].extra == 1)
		{
			if (ht->tempang < 256)
			{
				ht->tempang += 4;
				if (ht->tempang >= 256)
					callsound(s->sectnum, i);
				if (s->clipdist) l = 1;
				else l = -1;
			}
			else ht->tempang = 256;

			if (sc->floorz > s->z) //z's are touching
			{
				sc->floorz -= 512;
				zchange = -512;
				if (sc->floorz < s->z)
					sc->floorz = s->z;
			}

			else if (sc->floorz < s->z) //z's are touching
			{
				sc->floorz += 512;
				zchange = 512;
				if (sc->floorz > s->z)
					sc->floorz = s->z;
			}
		}
		else if (sprite[i].extra == 3)
		{
			if (ht->tempang > 0)
			{
				ht->tempang -= 4;
				if (ht->tempang <= 0)
					callsound(s->sectnum, i);
				if (s->clipdist) l = -1;
				else l = 1;
			}
			else ht->tempang = 0;

			if (sc->floorz > ht->temp_data[3]) //z's are touching
			{
				sc->floorz -= 512;
				zchange = -512;
				if (sc->floorz < ht->temp_data[3])
					sc->floorz = ht->temp_data[3];
			}

			else if (sc->floorz < ht->temp_data[3]) //z's are touching
			{
				sc->floorz += 512;
				zchange = 512;
				if (sc->floorz > ht->temp_data[3])
					sc->floorz = ht->temp_data[3];
			}
		}

		s->ang += (l * q);
		t[2] += (l * q);
	}
	else
	{
		if (hittype[j].temp_data[0] == 0) return;
		if (hittype[j].temp_data[0] == 2)
		{
			deletesprite(i);
			return;
		}

		if (sprowner->ang > 1024)
			l = -1;
		else l = 1;
		if (t[3] == 0)
			t[3] = ldist(s, &sprite[j]);
		s->xvel = t[3];
		s->x = sprowner->x;
		s->y = sprowner->y;
		s->ang += (l * q);
		t[2] += (l * q);
	}

	if (l && (sc->floorstat & 64))
	{
		int p;
		for (p = connecthead; p >= 0; p = connectpoint2[p])
		{
			if (ps[p].cursectnum == s->sectnum && ps[p].on_ground == 1)
			{
				ps[p].angle.addadjustment(l * q);

				ps[p].posz += zchange;

				int m, x;
				rotatepoint(sprowner->x, sprowner->y,
					ps[p].posx, ps[p].posy, (q * l),
					&m, &x);

				ps[p].bobposx += m - ps[p].posx;
				ps[p].bobposy += x - ps[p].posy;

				ps[p].posx = m;
				ps[p].posy = x;

				if (sprite[ps[p].i].extra <= 0)
				{
					sprite[ps[p].i].x = m;
					sprite[ps[p].i].y = x;
				}
			}
		}
		SectIterator itp(s->sectnum);
		while ((p = itp.NextIndex()) >= 0)
		{
			auto sprp = &sprite[p];
			if (sprp->statnum != 3 && sprp->statnum != 4)
				if (LASERLINE < 0 || sprp->picnum != LASERLINE)
				{
					if (sprp->picnum == TILE_APLAYER && sprp->owner >= 0)
					{
						continue;
					}

					sprp->ang += (l * q);
					sprp->ang &= 2047;

					sprp->z += zchange;

					rotatepoint(sprowner->x, sprowner->y,
						sprp->x, sprp->y, (q * l),
						&sprp->x, &sprp->y);

				}
		}

	}
	ms(i);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se01(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sh = s->hitag;
	if (s->owner == -1) //Init
	{
		s->owner = i;

		StatIterator it(STAT_EFFECTOR);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			auto ss = &sprite[j];
			if (ss->lotag == 19 && ss->hitag == sh)
			{
				t[0] = 0;
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se14(int i, bool checkstat, int RPG, int JIBS6)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	if (s->owner == -1)
		s->owner = LocateTheLocator(t[3], t[0]);

	if (s->owner == -1)
	{
		I_Error("Could not find any locators for SE# 6 and 14 with a hitag of %d.", t[3]);
	}

	int j = ldist(&sprite[s->owner], s);

	if (j < 1024L)
	{
		if (st == 6)
			if (sprite[s->owner].hitag & 1)
				t[4] = sc->extra; //Slow it down
		t[3]++;
		s->owner = LocateTheLocator(t[3], t[0]);
		if (s->owner == -1)
		{
			t[3] = 0;
			s->owner = LocateTheLocator(0, t[0]);
		}
	}

	if (s->xvel)
	{
		int x = getangle(sprite[s->owner].x - s->x, sprite[s->owner].y - s->y);
		int q = getincangle(s->ang, x) >> 3;

		t[2] += q;
		s->ang += q;

		bool statstate = (!checkstat || ((sc->floorstat & 1) == 0 && (sc->ceilingstat & 1) == 0));
		if (s->xvel == sc->extra)
		{
			if (statstate)
			{
				if (!S_CheckSoundPlaying(hittype[i].lastvx))
					S_PlayActorSound(hittype[i].lastvx, i);
			}
			if ((!checkstat || !statstate) && (ud.monsters_off == 0 && sc->floorpal == 0 && (sc->floorstat & 1) && rnd(8)))
			{
				int p = findplayer(s, &x);
				if (x < 20480)
				{
					j = s->ang;
					s->ang = getangle(s->x - ps[p].posx, s->y - ps[p].posy);
					fi.shoot(i, RPG);
					s->ang = j;
				}
			}
		}

		if (s->xvel <= 64 && statstate)
			S_StopSound(hittype[i].lastvx, i);

		if ((sc->floorz - sc->ceilingz) < (108 << 8))
		{
			if (ud.clipping == 0 && s->xvel >= 192)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
					if (sprite[ps[p].i].extra > 0)
					{
						short k = ps[p].cursectnum;
						updatesector(ps[p].posx, ps[p].posy, &k);
						if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && ps[p].cursectnum != s->sectnum))
						{
							ps[p].posx = s->x;
							ps[p].posy = s->y;
							ps[p].cursectnum = s->sectnum;

							setsprite(ps[p].i, s->x, s->y, s->z);
							quickkill(&ps[p]);
						}
					}
		}

		int m = (s->xvel * sintable[(s->ang + 512) & 2047]) >> 14;
		x = (s->xvel * sintable[s->ang & 2047]) >> 14;

		for (int p = connecthead; p >= 0; p = connectpoint2[p])
			if (sector[ps[p].cursectnum].lotag != 2)
			{
				if (po[p].os == s->sectnum)
				{
					po[p].ox += m;
					po[p].oy += x;
				}

				if (s->sectnum == sprite[ps[p].i].sectnum)
				{
					rotatepoint(s->x, s->y, ps[p].posx, ps[p].posy, q, &ps[p].posx, &ps[p].posy);

					ps[p].posx += m;
					ps[p].posy += x;

					ps[p].bobposx += m;
					ps[p].bobposy += x;

					ps[p].angle.addadjustment(q);

					if (numplayers > 1)
					{
						ps[p].oposx = ps[p].posx;
						ps[p].oposy = ps[p].posy;
					}
					if (sprite[ps[p].i].extra <= 0)
					{
						sprite[ps[p].i].x = ps[p].posx;
						sprite[ps[p].i].y = ps[p].posy;
					}
				}
			}
		SectIterator it(s->sectnum);
		while ((j = it.NextIndex()) >= 0)
		{
			auto sj = &sprite[j];
			if (sj->statnum != 10 && sector[sj->sectnum].lotag != 2 && sj->picnum != SECTOREFFECTOR && sj->picnum != LOCATORS)
			{
				rotatepoint(s->x, s->y,
					sj->x, sj->y, q,
					&sj->x, &sj->y);

				sj->x += m;
				sj->y += x;

				sj->ang += q;

				if (numplayers > 1)
				{
					hittype[j].bposx = sj->x;
					hittype[j].bposy = sj->y;
				}
			}
		}

		ms(i);
		setsprite(i, s->x, s->y, s->z);

		if ((sc->floorz - sc->ceilingz) < (108 << 8))
		{
			if (ud.clipping == 0 && s->xvel >= 192)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
					if (sprite[ps[p].i].extra > 0)
					{
						short k = ps[p].cursectnum;
						updatesector(ps[p].posx, ps[p].posy, &k);
						if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && ps[p].cursectnum != s->sectnum))
						{
							ps[p].oposx = ps[p].posx = s->x;
							ps[p].oposy = ps[p].posy = s->y;
							ps[p].cursectnum = s->sectnum;

							setsprite(ps[p].i, s->x, s->y, s->z);
							quickkill(&ps[p]);
						}
					}

			SectIterator itr(sprite[s->owner].sectnum);
			while ((j = itr.NextIndex()) >= 0)
			{
				auto spj = &sprite[j];

				if (spj->statnum == 1 && badguy(spj) && spj->picnum != SECTOREFFECTOR && spj->picnum != LOCATORS)
				{
					short k = spj->sectnum;
					updatesector(spj->x, spj->y, &k);
					if (spj->extra >= 0 && k == s->sectnum)
					{
						fi.gutsdir(&sprite[j], JIBS6, 72, myconnectindex);
						S_PlayActorSound(SQUISHED, i);
						deletesprite(j);
					}
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

void handle_se30(int i, int JIBS6)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	if (s->owner == -1)
	{
		t[3] = !t[3];
		s->owner = LocateTheLocator(t[3], t[0]);
	}
	else
	{

		if (t[4] == 1) // Starting to go
		{
			if (ldist(&sprite[s->owner], s) < (2048 - 128))
				t[4] = 2;
			else
			{
				if (s->xvel == 0)
					operateactivators(s->hitag + (!t[3]), -1);
				if (s->xvel < 256)
					s->xvel += 16;
			}
		}
		if (t[4] == 2)
		{
			int l = FindDistance2D(sprite[s->owner].x - s->x, sprite[s->owner].y - s->y);

			if (l <= 128)
				s->xvel = 0;

			if (s->xvel > 0)
				s->xvel -= 16;
			else
			{
				s->xvel = 0;
				operateactivators(s->hitag + (short)t[3], -1);
				s->owner = -1;
				s->ang += 1024;
				t[4] = 0;
				fi.operateforcefields(i, s->hitag);

				SectIterator it(s->sectnum);
				int j;
				while ((j = it.NextIndex()) >= 0)
				{
					auto sj = &sprite[j];
					if (sj->picnum != SECTOREFFECTOR && sj->picnum != LOCATORS)
					{
						hittype[j].bposx = sj->x;
						hittype[j].bposy = sj->y;
					}
				}

			}
		}
	}

	if (s->xvel)
	{
		int l = (s->xvel * sintable[(s->ang + 512) & 2047]) >> 14;
		int x = (s->xvel * sintable[s->ang & 2047]) >> 14;

		if ((sc->floorz - sc->ceilingz) < (108 << 8))
			if (ud.clipping == 0)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
					if (sprite[ps[p].i].extra > 0)
					{
						short k = ps[p].cursectnum;
						updatesector(ps[p].posx, ps[p].posy, &k);
						if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && ps[p].cursectnum != s->sectnum))
						{
							ps[p].posx = s->x;
							ps[p].posy = s->y;
							ps[p].cursectnum = s->sectnum;

							setsprite(ps[p].i, s->x, s->y, s->z);
							quickkill(&ps[p]);
						}
					}

		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			if (sprite[ps[p].i].sectnum == s->sectnum)
			{
				ps[p].posx += l;
				ps[p].posy += x;

				if (numplayers > 1)
				{
					ps[p].oposx = ps[p].posx;
					ps[p].oposy = ps[p].posy;
				}

				ps[p].bobposx += l;
				ps[p].bobposy += x;
			}

			if (po[p].os == s->sectnum)
			{
				po[p].ox += l;
				po[p].oy += x;
			}
		}

		SectIterator its(s->sectnum);
		int j;
		while ((j = its.NextIndex()) >= 0)
		{
			auto sprj = &sprite[j];
			auto htj = &hittype[j];
			if (sprj->picnum != SECTOREFFECTOR && sprj->picnum != LOCATORS)
			{
				if (numplayers < 2)
				{
					htj->bposx = sprj->x;
					htj->bposy = sprj->y;
				}

				sprj->x += l;
				sprj->y += x;

				if (numplayers > 1)
				{
					htj->bposx = sprj->x;
					htj->bposy = sprj->y;
				}
			}
		}

		ms(i);
		setsprite(i, s->x, s->y, s->z);

		if ((sc->floorz - sc->ceilingz) < (108 << 8))
		{
			if (ud.clipping == 0)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
					if (sprite[ps[p].i].extra > 0)
					{
						short k = ps[p].cursectnum;
						updatesector(ps[p].posx, ps[p].posy, &k);
						if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && ps[p].cursectnum != s->sectnum))
						{
							ps[p].posx = s->x;
							ps[p].posy = s->y;

							ps[p].oposx = ps[p].posx;
							ps[p].oposy = ps[p].posy;

							ps[p].cursectnum = s->sectnum;

							setsprite(ps[p].i, s->x, s->y, s->z);
							quickkill(&ps[p]);
						}
					}

			SectIterator it(sprite[sprite[i].owner].sectnum);
			while ((j = it.NextIndex()) >= 0)
			{
				auto sj = &sprite[j];
				if (sj->statnum == 1 && badguy(&sprite[j]) && sj->picnum != SECTOREFFECTOR && sj->picnum != LOCATORS)
				{
					//					if(sj->sectnum != s->sectnum)
					{
						short k = sj->sectnum;
						updatesector(sj->x, sj->y, &k);
						if (sj->extra >= 0 && k == s->sectnum)
						{
							fi.gutsdir(&sprite[j], JIBS6, 24, myconnectindex);
							S_PlayActorSound(SQUISHED, j);
							deletesprite(j);
						}
					}

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

void handle_se02(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	if (t[4] > 0 && t[0] == 0)
	{
		if (t[4] < sh)
			t[4]++;
		else t[0] = 1;
	}

	if (t[0] > 0)
	{
		t[0]++;

		s->xvel = 3;

		if (t[0] > 96)
		{
			t[0] = -1; //Stop the quake
			t[4] = -1;
			deletesprite(i);
			return;
		}
		else
		{
			if ((t[0] & 31) == 8)
			{
				earthquaketime = 48;
				S_PlayActorSound(EARTHQUAKE, ps[screenpeek].i);
			}

			if (abs(sc->floorheinum - t[5]) < 8)
				sc->floorheinum = t[5];
			else sc->floorheinum += (sgn(t[5] - sc->floorheinum) << 4);
		}

		int m = (s->xvel * sintable[(s->ang + 512) & 2047]) >> 14;
		int x = (s->xvel * sintable[s->ang & 2047]) >> 14;


		for (int p = connecthead; p >= 0; p = connectpoint2[p])
			if (ps[p].cursectnum == s->sectnum && ps[p].on_ground)
			{
				ps[p].posx += m;
				ps[p].posy += x;

				ps[p].bobposx += m;
				ps[p].bobposy += x;
			}

		SectIterator it(s->sectnum);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			auto sj = &sprite[j];
			if (sj->picnum != SECTOREFFECTOR)
			{
				sj->x += m;
				sj->y += x;
				setsprite(j, sj->x, sj->y, sj->z);
			}
		}
		ms(i);
		setsprite(i, s->x, s->y, s->z);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se03(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	if (t[4] == 0) return;
	int x, p = findplayer(s, &x);

	//	if(t[5] > 0) { t[5]--; break; }

	if ((global_random / (sh + 1) & 31) < 4 && !t[2])
	{
		//	   t[5] = 4+(global_random&7);
		sc->ceilingpal = s->owner >> 8;
		sc->floorpal = s->owner & 0xff;
		t[0] = s->shade + (global_random & 15);
	}
	else
	{
		//	   t[5] = 4+(global_random&3);
		sc->ceilingpal = s->pal;
		sc->floorpal = s->pal;
		t[0] = t[3];
	}

	sc->ceilingshade = t[0];
	sc->floorshade = t[0];

	auto wal = &wall[sc->wallptr];

	for (x = sc->wallnum; x > 0; x--, wal++)
	{
		if (wal->hitag != 1)
		{
			wal->shade = t[0];
			if ((wal->cstat & 2) && wal->nextwall >= 0)
			{
				wall[wal->nextwall].shade = wal->shade;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se04(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	int j;

	if ((global_random / (sh + 1) & 31) < 4)
	{
		t[1] = s->shade + (global_random & 15);//Got really bright
		t[0] = s->shade + (global_random & 15);
		sc->ceilingpal = s->owner >> 8;
		sc->floorpal = s->owner & 0xff;
		j = 1;
	}
	else
	{
		t[1] = t[2];
		t[0] = t[3];

		sc->ceilingpal = s->pal;
		sc->floorpal = s->pal;

		j = 0;
	}

	sc->floorshade = t[1];
	sc->ceilingshade = t[1];

	auto wal = &wall[sc->wallptr];

	for (int x = sc->wallnum; x > 0; x--, wal++)
	{
		if (j) wal->pal = (s->owner & 0xff);
		else wal->pal = s->pal;

		if (wal->hitag != 1)
		{
			wal->shade = t[0];
			if ((wal->cstat & 2) && wal->nextwall >= 0)
				wall[wal->nextwall].shade = wal->shade;
		}
	}

	SectIterator it(s->sectnum);
	while ((j = it.NextIndex()) >= 0)
	{
		auto sj = &sprite[j];
		if (sj->cstat & 16)
		{
			if (sc->ceilingstat & 1)
				sj->shade = sc->ceilingshade;
			else sj->shade = sc->floorshade;
		}
	}

	if (t[4])
		deletesprite(i);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se05(int i, int FIRELASER)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	int j, l, q, m;

	int x, p = findplayer(s, &x);
	if (x < 8192)
	{
		j = s->ang;
		s->ang = getangle(s->x - ps[p].posx, s->y - ps[p].posy);
		fi.shoot(i, FIRELASER);
		s->ang = j;
	}

	if (s->owner == -1) //Start search
	{
		t[4] = 0;
		l = 0x7fffffff;
		while (1) //Find the shortest dist
		{
			s->owner = LocateTheLocator((short)t[4], -1); //t[0] hold sectnum

			if (s->owner == -1) break;

			m = ldist(&sprite[ps[p].i], &sprite[s->owner]);

			if (l > m)
			{
				q = s->owner;
				l = m;
			}

			t[4]++;
		}

		s->owner = q;
		s->zvel = ksgn(sprite[q].z - s->z) << 4;
	}

	if (ldist(&sprite[s->owner], s) < 1024)
	{
		short ta;
		ta = s->ang;
		s->ang = getangle(ps[p].posx - s->x, ps[p].posy - s->y);
		s->ang = ta;
		s->owner = -1;
		return;

	}
	else s->xvel = 256;

	x = getangle(sprite[s->owner].x - s->x, sprite[s->owner].y - s->y);
	q = getincangle(s->ang, x) >> 3;
	s->ang += q;

	if (rnd(32))
	{
		t[2] += q;
		sc->ceilingshade = 127;
	}
	else
	{
		t[2] +=
			getincangle(t[2] + 512, getangle(ps[p].posx - s->x, ps[p].posy - s->y)) >> 2;
		sc->ceilingshade = 0;
	}
	j = fi.ifhitbyweapon(&hittype[i]);
	if (j >= 0)
	{
		t[3]++;
		if (t[3] == 5)
		{
			s->zvel += 1024;
			FTA(7, &ps[myconnectindex]);
		}
	}

	s->z += s->zvel;
	sc->ceilingz += s->zvel;
	sector[t[0]].ceilingz += s->zvel;
	ms(i);
	setsprite(i, s->x, s->y, s->z);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se08(int i, bool checkhitag1)
{
	// work only if its moving
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	int x, j = -1;

	if (hittype[i].temp_data[4])
	{
		hittype[i].temp_data[4]++;
		if (hittype[i].temp_data[4] > 8)
		{
			deletesprite(i);
			return;
		}
		j = 1;
	}
	else j = getanimationgoal(anim_ceilingz, s->sectnum);

	if (j >= 0)
	{
		short sn;

		if ((sc->lotag & 0x8000) || hittype[i].temp_data[4])
			x = -t[3];
		else
			x = t[3];

		if (st == 9) x = -x;

		StatIterator it(STAT_EFFECTOR);
		while ((j = it.NextIndex()) >= 0)
		{
			auto sj = &sprite[j];
			if (((sj->lotag) == st) && (sj->hitag) == sh)
			{
				sn = sj->sectnum;
				auto sect = &sector[sn];
				int m = sj->shade;

				auto wal = &wall[sect->wallptr];

				for (int l = sect->wallnum; l > 0; l--, wal++)
				{
					if (wal->hitag != 1)
					{
						wal->shade += x;

						if (wal->shade < m)
							wal->shade = m;
						else if (wal->shade > hittype[j].temp_data[2])
							wal->shade = hittype[j].temp_data[2];

						if (wal->nextwall >= 0)
							if (wall[wal->nextwall].hitag != 1)
								wall[wal->nextwall].shade = wal->shade;
					}
				}

				sect->floorshade += x;
				sect->ceilingshade += x;

				if (sect->floorshade < m)
					sect->floorshade = m;
				else if (sect->floorshade > hittype[j].temp_data[0])
					sect->floorshade = hittype[j].temp_data[0];

				if (sect->ceilingshade < m)
					sect->ceilingshade = m;
				else if (sect->ceilingshade > hittype[j].temp_data[1])
					sect->ceilingshade = hittype[j].temp_data[1];

				if (checkhitag1 && sect->hitag == 1)
					sect->ceilingshade = hittype[j].temp_data[1];

			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se10(int i, const int* specialtags)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	if ((sc->lotag & 0xff) == 27 || (sc->floorz > sc->ceilingz && (sc->lotag & 0xff) != 23) || sc->lotag == (short)32791)
	{
		int j = 1;

		if ((sc->lotag & 0xff) != 27)
			for (int p = connecthead; p >= 0; p = connectpoint2[p])
				if (sc->lotag != 30 && sc->lotag != 31 && sc->lotag != 0)
					if (s->sectnum == sprite[ps[p].i].sectnum)
						j = 0;

		if (j == 1)
		{
			if (t[0] > sh)
			{
				if (specialtags) for (int i = 0; specialtags[i]; i++)
				{
					if (sector[s->sectnum].lotag == specialtags[i] && getanimationgoal(anim_ceilingz, s->sectnum) >= 0)
					{
						return;
					}
				}
				fi.activatebysector(s->sectnum, i);
				t[0] = 0;
			}
			else t[0]++;
		}
	}
	else t[0] = 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se11(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	if (t[5] > 0)
	{
		t[5]--;
		return;
	}

	if (t[4])
	{
		int startwall, endwall;

		startwall = sc->wallptr;
		endwall = startwall + sc->wallnum;

		for (int j = startwall; j < endwall; j++)
		{
			StatIterator it(STAT_ACTOR);
			int k;
			while ((k = it.NextIndex()) >= 0)
			{
				auto sk = &sprite[k];
				if (sk->extra > 0 && badguy(&sprite[k]) && clipinsidebox(sk->x, sk->y, j, 256L) == 1)
					return;
			}

			it.Reset(STAT_PLAYER);
			while ((k = it.NextIndex()) >= 0)
			{
				auto sk = &sprite[k];
				if (sk->owner >= 0 && clipinsidebox(sk->x, sk->y, j, 144L) == 1)
				{
					t[5] = 8; // Delay
					k = (s->yvel >> 3) * t[3];
					t[2] -= k;
					t[4] -= k;
					ms(i);
					setsprite(i, s->x, s->y, s->z);
					return;
				}
			}
		}

		int k = (s->yvel >> 3) * t[3];
		t[2] += k;
		t[4] += k;
		ms(i);
		setsprite(i, s->x, s->y, s->z);

		if (t[4] <= -511 || t[4] >= 512)
		{
			t[4] = 0;
			t[2] &= 0xffffff00;
			ms(i);
			setsprite(i, s->x, s->y, s->z);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se12(int i, int planeonly)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	if (t[0] == 3 || t[3] == 1) //Lights going off
	{
		sc->floorpal = 0;
		sc->ceilingpal = 0;

		auto wal = &wall[sc->wallptr];
		for (int j = sc->wallnum; j > 0; j--, wal++)
			if (wal->hitag != 1)
			{
				wal->shade = t[1];
				wal->pal = 0;
			}

		sc->floorshade = t[1];
		sc->ceilingshade = t[2];
		t[0] = 0;

		SectIterator it(s->sectnum);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			auto sj = &sprite[j];
			if (sj->cstat & 16)
			{
				if (sc->ceilingstat & 1)
					sj->shade = sc->ceilingshade;
				else sj->shade = sc->floorshade;
			}
		}

		if (t[3] == 1)
		{
			deletesprite(i);
			return;
		}
	}
	if (t[0] == 1) //Lights flickering on
	{
		// planeonly 1 is RRRA SE47, planeonly 2 is SE48
		int compshade = planeonly == 2 ? sc->ceilingshade : sc->floorshade;
		if (compshade > s->shade)
		{
			if (planeonly != 2) sc->floorpal = s->pal;
			if (planeonly != 1) sc->ceilingpal = s->pal;

			if (planeonly != 2) sc->floorshade -= 2;
			if (planeonly != 1) sc->ceilingshade -= 2;

			auto wal = &wall[sc->wallptr];
			for (int j = sc->wallnum; j > 0; j--, wal++)
				if (wal->hitag != 1)
				{
					wal->pal = s->pal;
					wal->shade -= 2;
				}
		}
		else t[0] = 2;

		SectIterator it(s->sectnum);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			auto sj = &sprite[j];
			if (sj->cstat & 16)
			{
				if (sc->ceilingstat & 1)
					sj->shade = sc->ceilingshade;
				else sj->shade = sc->floorshade;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se13(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	if (t[2])
	{
		int j = (sprite[i].yvel << 5) | 1;

		if (s->ang == 512)
		{
			if (s->owner)
			{
				if (abs(t[0] - sc->ceilingz) >= j)
					sc->ceilingz += sgn(t[0] - sc->ceilingz) * j;
				else sc->ceilingz = t[0];
			}
			else
			{
				if (abs(t[1] - sc->floorz) >= j)
					sc->floorz += sgn(t[1] - sc->floorz) * j;
				else sc->floorz = t[1];
			}
		}
		else
		{
			if (abs(t[1] - sc->floorz) >= j)
				sc->floorz += sgn(t[1] - sc->floorz) * j;
			else sc->floorz = t[1];
			if (abs(t[0] - sc->ceilingz) >= j)
				sc->ceilingz += sgn(t[0] - sc->ceilingz) * j;
			sc->ceilingz = t[0];
		}

		if (t[3] == 1)
		{
			//Change the shades

			t[3]++;
			sc->ceilingstat ^= 1;

			if (s->ang == 512)
			{
				auto wal = &wall[sc->wallptr];
				for (j = sc->wallnum; j > 0; j--, wal++)
					wal->shade = s->shade;

				sc->floorshade = s->shade;

				if (ps[0].one_parallax_sectnum >= 0)
				{
					sc->ceilingpicnum =
						sector[ps[0].one_parallax_sectnum].ceilingpicnum;
					sc->ceilingshade =
						sector[ps[0].one_parallax_sectnum].ceilingshade;
				}
			}
		}
		t[2]++;
		if (t[2] > 256)
		{
			deletesprite(i);
			return;
		}
	}


	if (t[2] == 4 && s->ang != 512)
		for (int x = 0; x < 7; x++) RANDOMSCRAP(s, i);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se15(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	if (t[4])
	{
		s->xvel = 16;

		if (t[4] == 1) //Opening
		{
			if (t[3] >= (sprite[i].yvel >> 3))
			{
				t[4] = 0; //Turn off the sliders
				callsound(s->sectnum, i);
				return;
			}
			t[3]++;
		}
		else if (t[4] == 2)
		{
			if (t[3] < 1)
			{
				t[4] = 0;
				callsound(s->sectnum, i);
				return;
			}
			t[3]--;
		}

		ms(i);
		setsprite(i, s->x, s->y, s->z);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se16(int i, int REACTOR, int REACTOR2)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];

	t[2] += 32;
	if (sc->floorz < sc->ceilingz) s->shade = 0;

	else if (sc->ceilingz < t[3])
	{

		//The following code check to see if
		//there is any other sprites in the sector.
		//If there isn't, then kill this sectoreffector
		//itself.....

		SectIterator it(s->sectnum);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			auto sj = &sprite[j];
			if (sj->picnum == REACTOR || sj->picnum == REACTOR2)
				return;
		}
		if (j == -1)
		{
			deletesprite(i);
			return;
		}
		else s->shade = 1;
	}

	if (s->shade) sc->ceilingz += 1024;
	else sc->ceilingz -= 512;

	ms(i);
	setsprite(i, s->x, s->y, s->z);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se17(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	int q = t[0] * (sprite[i].yvel << 2);

	sc->ceilingz += q;
	sc->floorz += q;

	SectIterator it(s->sectnum);
	int j;
	while ((j = it.NextIndex()) >= 0)
	{
		auto sj = &sprite[j];
		if (sj->statnum == 10 && sj->owner >= 0)
		{
			int p = sj->yvel;
			if (numplayers < 2)
				ps[p].oposz = ps[p].posz;
			ps[p].posz += q;
			ps[p].truefz += q;
			ps[p].truecz += q;
			if (numplayers > 1)
				ps[p].oposz = ps[p].posz;
		}
		if (sj->statnum != 3)
		{
			hittype[j].bposz = sj->z;
			sj->z += q;
		}

		hittype[j].floorz = sc->floorz;
		hittype[j].ceilingz = sc->ceilingz;
	}

	if (t[0]) //If in motion
	{
		if (abs(sc->floorz - t[2]) <= sprite[i].yvel)
		{
			activatewarpelevators(i, 0);
			return;
		}

		if (t[0] == -1)
		{
			if (sc->floorz > t[3])
				return;
		}
		else if (sc->ceilingz < t[4]) return;

		if (t[1] == 0) return;
		t[1] = 0;

		StatIterator it(STAT_EFFECTOR);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			if (i != j && (sprite[j].lotag) == 17)
				if ((sc->hitag - t[0]) ==
					(sector[sprite[j].sectnum].hitag)
					&& sh == (sprite[j].hitag))
					break;
		}

		if (j == -1) return;

		SectIterator its(s->sectnum);
		int k;
		while ((k = its.NextIndex()) >= 0)
		{
			auto sk = &sprite[k];
			auto htk = &hittype[k];
			if (sk->statnum == 10 && sk->owner >= 0)
			{
				int p = sk->yvel;

				ps[p].posx += sprite[j].x - s->x;
				ps[p].posy += sprite[j].y - s->y;
				ps[p].posz = sector[sprite[j].sectnum].floorz - (sc->floorz - ps[p].posz);

				htk->floorz = sector[sprite[j].sectnum].floorz;
				htk->ceilingz = sector[sprite[j].sectnum].ceilingz;

				ps[p].bobposx = ps[p].oposx = ps[p].posx;
				ps[p].bobposy = ps[p].oposy = ps[p].posy;
				ps[p].oposz = ps[p].posz;

				ps[p].truefz = htk->floorz;
				ps[p].truecz = htk->ceilingz;
				ps[p].bobcounter = 0;

				changespritesect(k, sprite[j].sectnum);
				ps[p].cursectnum = sprite[j].sectnum;
			}
			else if (sk->statnum != 3)
			{
				sk->x +=
					sprite[j].x - s->x;
				sk->y +=
					sprite[j].y - s->y;
				sk->z = sector[sprite[j].sectnum].floorz -
					(sc->floorz - sk->z);

				htk->bposx = sk->x;
				htk->bposy = sk->y;
				htk->bposz = sk->z;

				changespritesect(k, sprite[j].sectnum);
				setsprite(k, sk->x, sk->y, sk->z);

				htk->floorz = sector[sprite[j].sectnum].floorz;
				htk->ceilingz = sector[sprite[j].sectnum].ceilingz;

			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se18(int i, bool morecheck)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	if (t[0])
	{
		if (s->pal)
		{
			if (s->ang == 512)
			{
				sc->ceilingz -= sc->extra;
				if (sc->ceilingz <= t[1])
				{
					sc->ceilingz = t[1];
					deletesprite(i);
					return;
				}
			}
			else
			{
				sc->floorz += sc->extra;
				if (morecheck)
				{
					SectIterator it(s->sectnum);
					int j;
					while ((j = it.NextIndex()) >= 0)
					{
						auto sj = &sprite[j];
						if (sj->picnum == TILE_APLAYER && sj->owner >= 0)
							if (ps[sj->yvel].on_ground == 1)
								ps[sj->yvel].posz += sc->extra;
						if (sj->zvel == 0 && sj->statnum != STAT_EFFECTOR && sj->statnum != STAT_PROJECTILE)
						{
							hittype[j].bposz = sj->z += sc->extra;
							hittype[j].floorz = sc->floorz;
						}
					}
				}
				if (sc->floorz >= t[1])
				{
					sc->floorz = t[1];
					deletesprite(i);
					return;
				}
			}
		}
		else
		{
			if (s->ang == 512)
			{
				sc->ceilingz += sc->extra;
				if (sc->ceilingz >= s->z)
				{
					sc->ceilingz = s->z;
					deletesprite(i);
					return;
				}
			}
			else
			{
				sc->floorz -= sc->extra;
				if (morecheck)
				{
					SectIterator it(s->sectnum);
					int j;
					while ((j = it.NextIndex()) >= 0)
					{
						auto sj = &sprite[j];
						if (sj->picnum == TILE_APLAYER && sj->owner >= 0)
							if (ps[sj->yvel].on_ground == 1)
								ps[sj->yvel].posz -= sc->extra;
						if (sj->zvel == 0 && sj->statnum != STAT_EFFECTOR && sj->statnum != STAT_PROJECTILE)
						{
							hittype[j].bposz = sj->z -= sc->extra;
							hittype[j].floorz = sc->floorz;
						}
					}
				}
				if (sc->floorz <= s->z)
				{
					sc->floorz = s->z;
					deletesprite(i);
					return;
				}
			}
		}

		t[2]++;
		if (t[2] >= s->hitag)
		{
			t[2] = 0;
			t[0] = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se19(int i, int BIGFORCE)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	int j, x, q;

	if (t[0])
	{
		if (t[0] == 1)
		{
			t[0]++;
			x = sc->wallptr;
			q = x + sc->wallnum;
			for (j = x; j < q; j++)
				if (wall[j].overpicnum == BIGFORCE)
				{
					wall[j].cstat &= (128 + 32 + 8 + 4 + 2);
					wall[j].overpicnum = 0;
					if (wall[j].nextwall >= 0)
					{
						wall[wall[j].nextwall].overpicnum = 0;
						wall[wall[j].nextwall].cstat &= (128 + 32 + 8 + 4 + 2);
					}
				}
		}

		if (sc->ceilingz < sc->floorz)
			sc->ceilingz += sprite[i].yvel;
		else
		{
			sc->ceilingz = sc->floorz;

			StatIterator it(STAT_EFFECTOR);
			int j;
			while ((j = it.NextIndex()) >= 0)
			{
				auto sj = &sprite[j];
				if (sj->lotag == 0 && sj->hitag == sh)
				{
					q = sprite[sj->owner].sectnum;
					sector[sj->sectnum].floorpal = sector[sj->sectnum].ceilingpal =
						sector[q].floorpal;
					sector[sj->sectnum].floorshade = sector[sj->sectnum].ceilingshade =
						sector[q].floorshade;

					hittype[sj->owner].temp_data[0] = 2;
				}
			}
			deletesprite(i);
			return;
		}
	}
	else //Not hit yet
	{
		j = fi.ifhitsectors(s->sectnum);
		if (j >= 0)
		{
			FTA(8, &ps[myconnectindex]);

			StatIterator it(STAT_EFFECTOR);
			int l;
			while ((l = it.NextIndex()) >= 0)
			{
				auto sl = &sprite[l];
				x = sl->lotag & 0x7fff;
				switch (x)
				{
				case 0:
					if (sl->hitag == sh)
					{
						q = sl->sectnum;
						sector[q].floorshade =
							sector[q].ceilingshade =
							sprite[sl->owner].shade;
						sector[q].floorpal =
							sector[q].ceilingpal =
							sprite[sl->owner].pal;
					}
					break;

				case 1:
				case 12:
					//case 18:
				case 19:

					if (sh == sl->hitag)
						if (hittype[l].temp_data[0] == 0)
						{
							hittype[l].temp_data[0] = 1; //Shut them all on
							sl->owner = i;
						}

					break;
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

void handle_se20(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	if (t[0] == 0) return;
	if (t[0] == 1) s->xvel = 8;
	else s->xvel = -8;

	if (s->xvel) //Moving
	{
		int x = (s->xvel * sintable[(s->ang + 512) & 2047]) >> 14;
		int l = (s->xvel * sintable[s->ang & 2047]) >> 14;

		t[3] += s->xvel;

		s->x += x;
		s->y += l;

		if (t[3] <= 0 || (t[3] >> 6) >= (sprite[i].yvel >> 6))
		{
			s->x -= x;
			s->y -= l;
			t[0] = 0;
			callsound(s->sectnum, i);
			return;
		}

		SectIterator it(s->sectnum);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			auto sj = &sprite[j];
			if (sj->statnum != 3 && sj->zvel == 0)
			{
				sj->x += x;
				sj->y += l;
				setsprite(j, sj->x, sj->y, sj->z);
				if (sector[sj->sectnum].floorstat & 2)
					if (sj->statnum == 2)
						makeitfall(j);
			}
		}

		dragpoint((short)t[1], wall[t[1]].x + x, wall[t[1]].y + l);
		dragpoint((short)t[2], wall[t[2]].x + x, wall[t[2]].y + l);

		for (int p = connecthead; p >= 0; p = connectpoint2[p])
			if (ps[p].cursectnum == s->sectnum && ps[p].on_ground)
			{
				ps[p].posx += x;
				ps[p].posy += l;

				ps[p].oposx = ps[p].posx;
				ps[p].oposy = ps[p].posy;

				setsprite(ps[p].i, ps[p].posx, ps[p].posy, ps[p].posz + PHEIGHT);
			}

		sc->floorxpanning -= x >> 3;
		sc->floorypanning -= l >> 3;

		sc->ceilingxpanning -= x >> 3;
		sc->ceilingypanning -= l >> 3;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se21(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	int* lp;

	if (t[0] == 0) return;

	if (s->ang == 1536)
		lp = &sc->ceilingz;
	else
		lp = &sc->floorz;

	if (t[0] == 1) //Decide if the s->sectnum should go up or down
	{
		s->zvel = ksgn(s->z - *lp) * (sprite[i].yvel << 4);
		t[0]++;
	}

	if (sc->extra == 0)
	{
		lp += s->zvel;

		if (abs(*lp - s->z) < 1024)
		{
			*lp = s->z;
			deletesprite(i);
		}
	}
	else sc->extra--;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se22(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	if (t[1])
	{
		if (getanimationgoal(anim_ceilingz, t[0]) >= 0)
			sc->ceilingz += sc->extra * 9;
		else t[1] = 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se26(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int x, j, l;

	s->xvel = 32;
	l = (s->xvel * sintable[(s->ang + 512) & 2047]) >> 14;
	x = (s->xvel * sintable[s->ang & 2047]) >> 14;

	s->shade++;
	if (s->shade > 7)
	{
		s->x = t[3];
		s->y = t[4];
		sc->floorz -= ((s->zvel * s->shade) - s->zvel);
		s->shade = 0;
	}
	else
		sc->floorz += s->zvel;

	SectIterator it(s->sectnum);
	while ((j = it.NextIndex()) >= 0)
	{
		auto sj = &sprite[j];
		if (sj->statnum != 3 && sj->statnum != 10)
		{
			hittype[j].bposx = sj->x;
			hittype[j].bposy = sj->y;

			sj->x += l;
			sj->y += x;

			sj->z += s->zvel;
			setsprite(j, sj->x, sj->y, sj->z);
		}
	}

	for (int p = connecthead; p >= 0; p = connectpoint2[p])
		if (sprite[ps[p].i].sectnum == s->sectnum && ps[p].on_ground)
		{
			ps[p].fric.x += l << 5;
			ps[p].fric.y += x << 5;
			ps[p].posz += s->zvel;
		}

	ms(i);
	setsprite(i, s->x, s->y, s->z);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se27(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	int x, p;

	if (ud.recstat == 0) return;

	hittype[i].tempang = s->ang;

	p = findplayer(s, &x);
	if (sprite[ps[p].i].extra > 0 && myconnectindex == screenpeek)
	{
		if (t[0] < 0)
		{
			ud.camerasprite = i;
			t[0]++;
		}
		else if (ud.recstat == 2 && ps[p].newowner == -1)
		{
			if (cansee(s->x, s->y, s->z, sprite[i].sectnum, ps[p].posx, ps[p].posy, ps[p].posz, ps[p].cursectnum))
			{
				if (x < (unsigned)sh)
				{
					ud.camerasprite = i;
					t[0] = 999;
					s->ang += getincangle(s->ang, getangle(ps[p].posx - s->x, ps[p].posy - s->y)) >> 3;
					sprite[i].yvel = 100 + ((s->z - ps[p].posz) / 257);

				}
				else if (t[0] == 999)
				{
					if (ud.camerasprite == i)
						t[0] = 0;
					else t[0] = -10;
					ud.camerasprite = i;

				}
			}
			else
			{
				s->ang = getangle(ps[p].posx - s->x, ps[p].posy - s->y);

				if (t[0] == 999)
				{
					if (ud.camerasprite == i)
						t[0] = 0;
					else t[0] = -20;
					ud.camerasprite = i;
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

void handle_se32(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];

	if (t[0] == 1)
	{
		// Choose dir

		if (t[2] == 1) // Retract
		{
			if (sprite[i].ang != 1536)
			{
				if (abs(sc->ceilingz - s->z) <
					(sprite[i].yvel << 1))
				{
					sc->ceilingz = s->z;
					callsound(s->sectnum, i);
					t[2] = 0;
					t[0] = 0;
				}
				else sc->ceilingz +=
					sgn(s->z - sc->ceilingz) * sprite[i].yvel;
			}
			else
			{
				if (abs(sc->ceilingz - t[1]) <
					(sprite[i].yvel << 1))
				{
					sc->ceilingz = t[1];
					callsound(s->sectnum, i);
					t[2] = 0;
					t[0] = 0;
				}
				else sc->ceilingz +=
					sgn(t[1] - sc->ceilingz) * sprite[i].yvel;
			}
			return;
		}

		if ((s->ang & 2047) == 1536)
		{
			if (abs(sc->ceilingz - s->z) <
				(sprite[i].yvel << 1))
			{
				t[0] = 0;
				t[2] = !t[2];
				callsound(s->sectnum, i);
				sc->ceilingz = s->z;
			}
			else sc->ceilingz +=
				sgn(s->z - sc->ceilingz) * sprite[i].yvel;
		}
		else
		{
			if (abs(sc->ceilingz - t[1]) < (sprite[i].yvel << 1))
			{
				t[0] = 0;
				t[2] = !t[2];
				callsound(s->sectnum, i);
			}
			else sc->ceilingz -= sgn(s->z - t[1]) * sprite[i].yvel;
		}
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se35(int i, int SMALLSMOKE, int EXPLOSION2)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];

	if (sc->ceilingz > s->z)
		for (int j = 0; j < 8; j++)
		{
			s->ang += krand() & 511;
			int k = fi.spawn(i, SMALLSMOKE);
			sprite[k].xvel = 96 + (krand() & 127);
			ssp(k, CLIPMASK0);
			setsprite(k, sprite[k].x, sprite[k].y, sprite[k].z);
			if (rnd(16))
				fi.spawn(i, EXPLOSION2);
		}

	switch (t[0])
	{
	case 0:
		sc->ceilingz += s->yvel;
		if (sc->ceilingz > sc->floorz)
			sc->floorz = sc->ceilingz;
		if (sc->ceilingz > s->z + (32 << 8))
			t[0]++;
		break;
	case 1:
		sc->ceilingz -= (s->yvel << 2);
		if (sc->ceilingz < t[4])
		{
			sc->ceilingz = t[4];
			t[0] = 0;
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se128(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	auto sc = &sector[s->sectnum];

	auto wal = &wall[t[2]];

	if (wal->cstat | 32)
	{
		wal->cstat &= (255 - 32);
		wal->cstat |= 16;
		if (wal->nextwall >= 0)
		{
			wall[wal->nextwall].cstat &= (255 - 32);
			wall[wal->nextwall].cstat |= 16;
		}
	}
	else return;

	wal->overpicnum++;
	if (wal->nextwall >= 0)
		wall[wal->nextwall].overpicnum++;

	if (t[0] < t[1]) t[0]++;
	else
	{
		wal->cstat &= (128 + 32 + 8 + 4 + 2);
		if (wal->nextwall >= 0)
			wall[wal->nextwall].cstat &= (128 + 32 + 8 + 4 + 2);
		deletesprite(i);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se130(int i, int countmax, int EXPLOSION2)
{
	auto act = &hittype[i];
	spritetype* s = &act->s;
	auto t = &act->temp_data[0];
	auto sc = &sector[s->sectnum];

	if (t[0] > countmax)
	{
		deletesprite(i);
		return;
	}
	else t[0]++;

	int x = sc->floorz - sc->ceilingz;

	if (rnd(64))
	{
		auto k = spawn(act, EXPLOSION2);
		k->s.xrepeat = k->s.yrepeat = 2 + (krand() & 7);
		k->s.z = sc->floorz - (krand() % x);
		k->s.ang += 256 - (krand() % 511);
		k->s.xvel = krand() & 127;
		ssp(k, CLIPMASK0);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void getglobalz(int i)
{
	int hz,lz,zr;

	spritetype *s = &sprite[i];

	if( s->statnum == STAT_PLAYER || s->statnum == STAT_STANDABLE || s->statnum == STAT_ZOMBIEACTOR || s->statnum == STAT_ACTOR || s->statnum == STAT_PROJECTILE)
	{
		if(s->statnum == STAT_PROJECTILE)
			zr = 4;
		else zr = 127;

		getzrange(s->x,s->y,s->z-(FOURSLEIGHT),s->sectnum,&hittype[i].ceilingz,&hz,&hittype[i].floorz,&lz,zr,CLIPMASK0);

		if( (lz&49152) == 49152 && (sprite[lz&(MAXSPRITES-1)].cstat&48) == 0 )
		{
			lz &= (MAXSPRITES-1);
			if( badguy(&sprite[lz]) && sprite[lz].pal != 1)
			{
				if( s->statnum != 4 )
				{
					hittype[i].aflags |= SFLAG_NOFLOORSHADOW; 
					//hittype[i].dispicnum = -4; // No shadows on actors
					s->xvel = -256;
					ssp(i,CLIPMASK0);
				}
			}
			else if(sprite[lz].picnum == TILE_APLAYER && badguy(s) )
			{
				hittype[i].aflags |= SFLAG_NOFLOORSHADOW; 
				//hittype[i].dispicnum = -4; // No shadows on actors
				s->xvel = -256;
				ssp(i,CLIPMASK0);
			}
			else if(s->statnum == 4 && sprite[lz].picnum == TILE_APLAYER)
				if(s->owner == lz)
			{
				hittype[i].ceilingz = sector[s->sectnum].ceilingz;
				hittype[i].floorz	= sector[s->sectnum].floorz;
			}
		}
	}
	else
	{
		hittype[i].ceilingz = sector[s->sectnum].ceilingz;
		hittype[i].floorz	= sector[s->sectnum].floorz;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void makeitfall(int i)
{
	spritetype *s = &sprite[i];
	int hz,lz,c;

	if( fi.floorspace(s->sectnum) )
		c = 0;
	else
	{
		if( fi.ceilingspace(s->sectnum) || sector[s->sectnum].lotag == ST_2_UNDERWATER)
			c = gc/6;
		else c = gc;
	}
	
	if (isRRRA())
	{
		c = adjustfall(s, c); // this accesses sprite indices and cannot be in shared code. Should be done better.
	}

	if( ( s->statnum == STAT_ACTOR || s->statnum == STAT_PLAYER || s->statnum == STAT_ZOMBIEACTOR || s->statnum == STAT_STANDABLE ) )
		getzrange(s->x,s->y,s->z-(FOURSLEIGHT),s->sectnum,&hittype[i].ceilingz,&hz,&hittype[i].floorz,&lz,127L,CLIPMASK0);
	else
	{
		hittype[i].ceilingz = sector[s->sectnum].ceilingz;
		hittype[i].floorz	= sector[s->sectnum].floorz;
	}

	if( s->z < hittype[i].floorz-(FOURSLEIGHT) )
	{
		if( sector[s->sectnum].lotag == 2 && s->zvel > 3122 )
			s->zvel = 3144;
		if(s->zvel < 6144)
			s->zvel += c;
		else s->zvel = 6144;
		s->z += s->zvel;
	}
	if( s->z >= hittype[i].floorz-(FOURSLEIGHT) )
	{
		s->z = hittype[i].floorz - FOURSLEIGHT;
		s->zvel = 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int dodge(spritetype* s)
{
	int i;
	int bx, by, mx, my, bxvect, byvect, mxvect, myvect, d;

	mx = s->x;
	my = s->y;
	mxvect = sintable[(s->ang + 512) & 2047]; myvect = sintable[s->ang & 2047];

	StatIterator it(STAT_PROJECTILE);
	while ((i = it.NextIndex()) >= 0)
	{
		auto si = &sprite[i];
		if (si->owner == i || si->sectnum != s->sectnum)
			continue;

		bx = si->x - mx;
		by = si->y - my;
		bxvect = sintable[(si->ang + 512) & 2047]; byvect = sintable[si->ang & 2047];

		if (mxvect * bx + myvect * by >= 0)
			if (bxvect * bx + byvect * by < 0)
			{
				d = bxvect * by - byvect * bx;
				if (abs(d) < 65536 * 64)
				{
					s->ang -= 512 + (krand() & 1024);
					return 1;
				}
			}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int furthestangle(int i, int angs)
{
	short j, hitsect, hitwall, hitspr, furthest_angle, angincs;
	int hx, hy, hz, d, greatestd;
	spritetype* s = &sprite[i];

	greatestd = -(1 << 30);
	angincs = 2048 / angs;

	if (s->picnum != TILE_APLAYER)
		if ((hittype[i].temp_data[0] & 63) > 2) return(s->ang + 1024);

	for (j = s->ang; j < (2048 + s->ang); j += angincs)
	{
		hitscan(s->x, s->y, s->z - (8 << 8), s->sectnum,
			sintable[(j + 512) & 2047],
			sintable[j & 2047], 0,
			&hitsect, &hitwall, &hitspr, &hx, &hy, &hz, CLIPMASK1);

		d = abs(hx - s->x) + abs(hy - s->y);

		if (d > greatestd)
		{
			greatestd = d;
			furthest_angle = j;
		}
	}
	return (furthest_angle & 2047);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int furthestcanseepoint(int i, spritetype* ts, int* dax, int* day)
{
	short j, hitsect, hitwall, hitspr, angincs;
	int hx, hy, hz, d, da;//, d, cd, ca,tempx,tempy,cx,cy;
	spritetype* s = &sprite[i];

	if ((hittype[i].temp_data[0] & 63)) return -1;

	if (ud.multimode < 2 && ud.player_skill < 3)
		angincs = 2048 / 2;
	else angincs = 2048 / (1 + (krand() & 1));

	for (j = ts->ang; j < (2048 + ts->ang); j += (angincs - (krand() & 511)))
	{
		hitscan(ts->x, ts->y, ts->z - (16 << 8), ts->sectnum,
			sintable[(j + 512) & 2047],
			sintable[j & 2047], 16384 - (krand() & 32767),
			&hitsect, &hitwall, &hitspr, &hx, &hy, &hz, CLIPMASK1);

		d = abs(hx - ts->x) + abs(hy - ts->y);
		da = abs(hx - s->x) + abs(hy - s->y);

		if (d < da)
			if (cansee(hx, hy, hz, hitsect, s->x, s->y, s->z - (16 << 8), s->sectnum))
			{
				*dax = hx;
				*day = hy;
				return hitsect;
			}
	}
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void alterang(int a, int g_i, int g_p)
{
	short aang, angdif, goalang, j;
	int ticselapsed;
	int* g_t = hittype[g_i].temp_data;
	auto* g_sp = &sprite[g_i];

	auto moveptr = &ScriptCode[g_t[1]];

	ticselapsed = (g_t[0]) & 31;

	aang = g_sp->ang;

	g_sp->xvel += (*moveptr - g_sp->xvel) / 5;
	if (g_sp->zvel < 648) g_sp->zvel += ((*(moveptr + 1) << 4) - g_sp->zvel) / 5;

	if (isRRRA() && (a & windang))
		g_sp->ang = WindDir;
	else if (a & seekplayer)
	{
		j = !isRR()? ps[g_p].holoduke_on->GetIndex() : -1;

		// NOTE: looks like 'owner' is set to target sprite ID...

		if (j >= 0 && cansee(sprite[j].x, sprite[j].y, sprite[j].z, sprite[j].sectnum, g_sp->x, g_sp->y, g_sp->z, g_sp->sectnum))
			g_sp->owner = j;
		else g_sp->owner = ps[g_p].i;

		if (sprite[g_sp->owner].picnum == TILE_APLAYER)
			goalang = getangle(hittype[g_i].lastvx - g_sp->x, hittype[g_i].lastvy - g_sp->y);
		else
			goalang = getangle(sprite[g_sp->owner].x - g_sp->x, sprite[g_sp->owner].y - g_sp->y);

		if (g_sp->xvel && g_sp->picnum != TILE_DRONE)
		{
			angdif = getincangle(aang, goalang);

			if (ticselapsed < 2)
			{
				if (abs(angdif) < 256)
				{
					j = 128 - (krand() & 256);
					g_sp->ang += j;
					if (hits(g_i) < 844)
						g_sp->ang -= j;
				}
			}
			else if (ticselapsed > 18 && ticselapsed < 26) // choose
			{
				if (abs(angdif >> 2) < 128) g_sp->ang = goalang;
				else g_sp->ang += angdif >> 2;
			}
		}
		else g_sp->ang = goalang;
	}

	if (ticselapsed < 1)
	{
		j = 2;
		if (a & furthestdir)
		{
			goalang = furthestangle(g_i, j);
			g_sp->ang = goalang;
			g_sp->owner = ps[g_p].i;
		}

		if (a & fleeenemy)
		{
			goalang = furthestangle(g_i, j);
			g_sp->ang = goalang; // += angdif; //  = getincangle(aang,goalang)>>1;
		}
	}
}

//---------------------------------------------------------------------------
//
// the indirections here are to keep this core function free of game references
//
//---------------------------------------------------------------------------

void fall_common(int g_i, int g_p, int JIBS6, int DRONE, int BLOODPOOL, int SHOTSPARK1, int squished, int thud, int(*fallspecial)(int, int), void (*falladjustz)(spritetype*))
{
	auto g_sp = &sprite[g_i];
	g_sp->xoffset = 0;
	g_sp->yoffset = 0;
	//			  if(!gotz)
	{
		int c;

		int sphit = fallspecial? fallspecial(g_i, g_p) : 0;
		if (fi.floorspace(g_sp->sectnum))
			c = 0;
		else
		{
			if (fi.ceilingspace(g_sp->sectnum) || sector[g_sp->sectnum].lotag == 2)
				c = gc / 6;
			else c = gc;
		}

		if (hittype[g_i].cgg <= 0 || (sector[g_sp->sectnum].floorstat & 2))
		{
			getglobalz(g_i);
			hittype[g_i].cgg = 6;
		}
		else hittype[g_i].cgg--;

		if (g_sp->z < (hittype[g_i].floorz - FOURSLEIGHT))
		{
			g_sp->zvel += c;
			g_sp->z += g_sp->zvel;

			if (g_sp->zvel > 6144) g_sp->zvel = 6144;
		}
		else
		{
			g_sp->z = hittype[g_i].floorz - FOURSLEIGHT;

			if (badguy(g_sp) || (g_sp->picnum == TILE_APLAYER && g_sp->owner >= 0))
			{

				if (g_sp->zvel > 3084 && g_sp->extra <= 1)
				{
					if (g_sp->pal != 1 && g_sp->picnum != DRONE)
					{
						if (g_sp->picnum == TILE_APLAYER && g_sp->extra > 0)
							goto SKIPJIBS;
						if (sphit)
						{
							fi.guts(g_sp, JIBS6, 5, g_p);
							S_PlayActorSound(squished, g_i);
						}
						else
						{
							fi.guts(g_sp, JIBS6, 15, g_p);
							S_PlayActorSound(squished, g_i);
							fi.spawn(g_i, BLOODPOOL);
						}
					}

				SKIPJIBS:

					hittype[g_i].picnum = SHOTSPARK1;
					hittype[g_i].extra = 1;
					g_sp->zvel = 0;
				}
				else if (g_sp->zvel > 2048 && sector[g_sp->sectnum].lotag != 1)
				{

					short j = g_sp->sectnum;
					int x = g_sp->x, y = g_sp->y, z = g_sp->z;
					pushmove(&x, &y, &z, &j, 128, (4 << 8), (4 << 8), CLIPMASK0);
					setspritepos(g_i, x, y, z);	// wrap this for safety. The renderer may need processing of the new position.
					if (j != g_sp->sectnum && j >= 0 && j < MAXSECTORS)
						changespritesect(g_i, j);

					S_PlayActorSound(thud, g_i);
				}
			}
			if (sector[g_sp->sectnum].lotag == 1)
				falladjustz(g_sp);
			else g_sp->zvel = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int LocateTheLocator(int n, int sn)
{
	int i;

	StatIterator it(STAT_LOCATOR);
	while ((i = it.NextIndex()) >= 0)
	{
		if ((sn == -1 || sn == sprite[i].sectnum) && n == sprite[i].lotag)
			return i;
	}
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void recordoldspritepos()
{
	for (int statNum = 0; statNum < MAXSTATUS; statNum++)
	{
		StatIterator it(statNum);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			auto s = &sprite[j];
			auto h = &hittype[j];
			h->bposx = s->x;
			h->bposy = s->y;
			h->bposz = s->z;
		}
	}
}



END_DUKE_NS
