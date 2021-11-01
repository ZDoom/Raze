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

int adjustfall(DDukeActor* s, int c);


//---------------------------------------------------------------------------
//
// this was once a macro
//
//---------------------------------------------------------------------------

void RANDOMSCRAP(DDukeActor* origin)
{
	int r1 = krand(), r2 = krand(), r3 = krand(), r4 = krand(), r5 = krand(), r6 = krand(), r7 = krand();
	int v = isRR() ? 16 : 48;
	EGS(origin->s->sectnum, 
		origin->s->x + (r7 & 255) - 128, origin->s->y + (r6 & 255) - 128, origin->s->z - (8 << 8) - (r5 & 8191), 
		TILE_SCRAP6 + (r4 & 15), -8, v, v, r3 & 2047, (r2 & 63) + 64, -512 - (r1 & 2047), origin, 5); 
}

//---------------------------------------------------------------------------
//
// wrapper to ensure that if a sound actor is killed, the sound is stopped as well.
//
//---------------------------------------------------------------------------

void deletesprite(DDukeActor *const actor)
{
	if (actor->s->picnum == MUSICANDSFX && actor->temp_data[0] == 1)
		S_StopSound(actor->s->lotag, actor);
	else
		S_RelinkActorSound(actor, nullptr);
	::deletesprite(actor->GetSpriteIndex());
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addammo(int weapon, struct player_struct* player, int amount)
{
	player->ammo_amount[weapon] += amount;

	if (player->ammo_amount[weapon] > gs.max_ammo_amount[weapon])
		player->ammo_amount[weapon] = gs.max_ammo_amount[weapon];
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
		SetGameVarID(g_iWeaponVarID, player->curr_weapon, player->GetActor(), snum); // snum is player index!
		if (player->curr_weapon >= 0)
		{
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[player->curr_weapon][snum], player->GetActor(), snum);
		}
		else
		{
			SetGameVarID(g_iWorksLikeVarID, -1, player->GetActor(), snum);
		}
		OnEvent(EVENT_CHANGEWEAPON, snum, player->GetActor(), -1);
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
	ps->newOwner = nullptr;
	ps->pos.x = ps->oposx;
	ps->pos.y = ps->oposy;
	ps->pos.z = ps->oposz;
	ps->angle.restore();
	updatesector(ps->pos.x, ps->pos.y, &ps->cursectnum);

	DukeStatIterator it(STAT_ACTOR);
	while (auto k = it.Next())
	{
		if (k->s->picnum == TILE_CAMERA1)
			k->s->yvel = 0;
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
		MulScale(actor->s->xvel, bcos(actor->s->ang), 14),
		MulScale(actor->s->xvel, bsin(actor->s->ang), 14), actor->s->zvel,
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
			spriteq[spriteqloc]->s->xrepeat = 0;
			// deletesprite(spriteq[spriteqloc]);
		}
		spriteq[spriteqloc] = actor;
		spriteqloc = (spriteqloc + 1) % spriteqamount;
	}
	else actor->s->xrepeat = actor->s->yrepeat = 0;
}

//---------------------------------------------------------------------------
//
// consolidation of several nearly identical functions
//
//---------------------------------------------------------------------------

void lotsofstuff(DDukeActor* actor, int n, int spawntype)
{
	auto s = actor->s;
	for (int i = n; i > 0; i--)
	{
		int r1 = krand(), r2 = krand();	// using the RANDCORRECT version from RR.
		auto j = EGS(s->sectnum, s->x, s->y, s->z - (r2 % (47 << 8)), spawntype, -32, 8, 8, r1 & 2047, 0, 0, actor, 5);
		j->s->cstat = krand() & 12;
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
	auto s = actor->s;

	s->x += MulScale(s->xvel, bcos(s->ang), 14);
	s->y += MulScale(s->xvel, bsin(s->ang), 14);

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
	for (int q = numcyclers - 1; q >= 0; q--)
	{
		short* c = &cyclers[q][0];
		int s = c[0];

		int t = c[3];
		int j = t + bsin(c[1], -10);
		int cshade = c[2];

		if (j < cshade) j = cshade;
		else if (j > t)  j = t;

		c[1] += sector[s].extra;
		if (c[5])
		{
			auto wal = &wall[sector[s].wallptr];
			for (int x = sector[s].wallnum; x > 0; x--, wal++)
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
		auto spri = act->s;

		if ((!isRR() && ps[p].on_crane != nullptr) || sector[ps[p].cursectnum].lotag != 1 || ps->GetActor()->s->extra <= 0)
		{
			ps[p].dummyplayersprite = nullptr;
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

		spri->x += (ps[p].pos.x - ps[p].oposx);
		spri->y += (ps[p].pos.y - ps[p].oposy);
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
		auto spri = act->s;

		if (act->GetOwner())
		{
			if (p->newOwner != nullptr) //Looking thru the camera
			{
				spri->x = p->oposx;
				spri->y = p->oposy;
				spri->z = p->oposz + gs.playerheight;
				spri->backupz();
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
					if (psp->s->extra > 0)
					{
						if (spri->yrepeat > 32 && psp->s->yrepeat < 32)
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
					spri->extra = gs.max_player_health;
					spri->cstat = 257;
					if (!isWW2GI() && !isRR())
						p->jetpack_amount = 1599;
				}

				if (p->actorsqu != nullptr)
				{
					p->angle.addadjustment(getincanglebam(p->angle.ang, bvectangbam(p->actorsqu->s->x - p->pos.x, p->actorsqu->s->y - p->pos.y)) >> 2);
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
					p->pos.x = spri->x;
					p->pos.y = spri->y;
					p->pos.z = spri->z - (20 << 8);

					p->newOwner = nullptr;

					if (p->wackedbyactor != nullptr && p->wackedbyactor->s->statnum < MAXSTATUS)
					{
						p->angle.addadjustment(getincanglebam(p->angle.ang, bvectangbam(p->wackedbyactor->s->x - p->pos.x, p->wackedbyactor->s->y - p->pos.y)) >> 1);
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
		auto spri = act->s;
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
					FX_SetReverb(spri->lotag - 1100);
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
	auto spri = actor->s;
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
			switch (a2->s->statnum)
			{
			case STAT_ACTOR:
			case STAT_ZOMBIEACTOR:
			case STAT_STANDABLE:
			case STAT_PLAYER:
				spri->ang = getangle(msx[t[4] + 1] - spri->x, msy[t[4] + 1] - spri->y);
				setsprite(a2, msx[t[4] + 1], msy[t[4] + 1], a2->s->z);
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
					actor->SetActiveCrane(false);
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
				ps[p].angle.settarget(spri->ang + 1024);
			}
			else
			{
				DukeSectIterator it(t[1]);
				while (auto a2 = it.Next())
				{
					switch (a2->s->statnum)
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
		ssp(actor, CLIPMASK0);
		if (((spri->x - msx[t[4]]) * (spri->x - msx[t[4]]) + (spri->y - msy[t[4]]) * (spri->y - msy[t[4]])) < (128 * 128))
			t[0]++;
	}

	else if (t[0] == 9)
		t[0] = 0;

	setsprite(ScriptIndexToActor(msy[t[4] + 2]), spri->x, spri->y, spri->z - (34 << 8));

	auto Owner = actor->GetOwner();
	if (Owner != nullptr || actor->IsActiveCrane())
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

		if (Owner != nullptr)
		{
			setsprite(Owner, spri->pos);

			Owner->s->opos = spri->pos;

			spri->zvel = 0;
		}
		else if (actor->IsActiveCrane())
		{
			auto ang = ps[p].angle.ang.asbuild();
			ps[p].oposx = ps[p].pos.x;
			ps[p].oposy = ps[p].pos.y;
			ps[p].oposz = ps[p].pos.z;
			ps[p].pos.x = spri->x - bcos(ang, -6);
			ps[p].pos.y = spri->y - bsin(ang, -6);
			ps[p].pos.z = spri->z + (2 << 8);
			setsprite(ps[p].GetActor(), ps[p].pos.x, ps[p].pos.y, ps[p].pos.z);
			ps[p].cursectnum = ps[p].GetActor()->s->sectnum;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefountain(DDukeActor *actor, int fountain)
{
	int* t = &actor->temp_data[0];
	int x;
	if (t[0] > 0)
	{
		if (t[0] < 20)
		{
			t[0]++;

			actor->s->picnum++;

			if (actor->s->picnum == fountain + 3)
				actor->s->picnum = fountain + 1;
		}
		else
		{
			findplayer(actor, &x);

			if (x > 512)
			{
				t[0] = 0;
				actor->s->picnum = fountain;
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

void moveflammable(DDukeActor* actor, int tire, int box, int pool)
{
	auto spri = actor->s;
	int j;
	if (actor->temp_data[0] == 1)
	{
		actor->temp_data[1]++;
		if ((actor->temp_data[1] & 3) > 0) return;

		if (!isRR() && spri->picnum == tire && actor->temp_data[1] == 32)
		{
			spri->cstat = 0;
			auto spawned = spawn(actor, pool);
			spawned->s->shade = 127;
		}
		else
		{
			if (spri->shade < 64) spri->shade++;
			else
			{
				deletesprite(actor);
				return;
			}
		}

		j = spri->xrepeat - (krand() & 7);
		if (j < 10)
		{
			deletesprite(actor);
			return;
		}

		spri->xrepeat = j;

		j = spri->yrepeat - (krand() & 7);
		if (j < 4)
		{
			deletesprite(actor);
			return;
		}
		spri->yrepeat = j;
	}
	if (box >= 0 && spri->picnum == box)
	{
		makeitfall(actor);
		actor->ceilingz = sector[spri->sectnum].ceilingz;
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void detonate(DDukeActor *actor, int explosion)
{
	auto spri = actor->s;
	int* t = &actor->temp_data[0];
	earthquaketime = 16;

	DukeStatIterator itj(STAT_EFFECTOR);
	while (auto effector = itj.Next())
	{
		auto sj = effector->s;
		if (actor->s->hitag == sj->hitag)
		{
			if (sj->lotag == SE_13_EXPLOSIVE)
			{
				if (effector->temp_data[2] == 0)
					effector->temp_data[2] = 1;
			}
			else if (sj->lotag == SE_8_UP_OPEN_DOOR_LIGHTS)
				effector->temp_data[4] = 1;
			else if (sj->lotag == SE_18_INCREMENTAL_SECTOR_RISE_FALL)
			{
				if (effector->temp_data[0] == 0)
					effector->temp_data[0] = 1;
			}
			else if (sj->lotag == SE_21_DROP_FLOOR)
				effector->temp_data[0] = 1;
		}
	}

	spri->z -= (32 << 8);

	if ((t[3] == 1 && spri->xrepeat) || spri->lotag == -99)
	{
		int x = spri->extra;
		spawn(actor, explosion);
		fi.hitradius(actor, gs.seenineblastradius, x >> 2, x - (x >> 1), x - (x >> 2), x);
		S_PlayActorSound(PIPEBOMB_EXPLODE, actor);
	}

	if (spri->xrepeat)
		for (int x = 0; x < 8; x++) RANDOMSCRAP(actor);

	deletesprite(actor);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movemasterswitch(DDukeActor *actor, int spectype1, int spectype2)
{
	auto spri = actor->s;
	if (spri->yvel == 1)
	{
		spri->hitag--;
		if (spri->hitag <= 0)
		{
			operatesectors(spri->sectnum, actor);

			DukeSectIterator it(spri->sectnum);
			while (auto effector = it.Next())
			{
				auto sj = effector->s;
				if (sj->statnum == STAT_EFFECTOR)
				{
					switch (sj->lotag)
					{
					case SE_2_EARTHQUAKE:
					case SE_21_DROP_FLOOR:
					case SE_31_FLOOR_RISE_FALL:
					case SE_32_CEILING_RISE_FALL:
					case SE_36_PROJ_SHOOTER:
						effector->temp_data[0] = 1;
						break;
					case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
						effector->temp_data[4] = 1;
						break;
					}
				}
				else if (sj->statnum == STAT_STANDABLE)
				{
					if (sj->picnum == spectype1 || sj->picnum == spectype2) // SEENINE and OOZFILTER
					{
						sj->shade = -31;
					}
				}
			}
			// we cannot delete this because it may be used as a sound source.
			// This originally depended on undefined behavior as the deleted sprite was still used for the sound
			// with no checking if it got reused in the mean time.
			spri->picnum = 0;	// give it a picnum without any behavior attached, just in case
			spri->cstat |= CSTAT_SPRITE_INVISIBLE;
			spri->cstat2 |= CSTAT2_SPRITE_NOFIND;
			changeactorstat(actor, STAT_REMOVED);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movetrash(DDukeActor *actor)
{
	auto s = actor->s;
	if (s->xvel == 0) s->xvel = 1;
	if (ssp(actor, CLIPMASK0))
	{
		makeitfall(actor);
		if (krand() & 1) s->zvel -= 256;
		if (abs(s->xvel) < 48)
			s->xvel += (krand() & 3);
	}
	else deletesprite(actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movewaterdrip(DDukeActor *actor, int drip)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	int sect = s->sectnum;

	if (t[1])
	{
		t[1]--;
		if (t[1] == 0)
			s->cstat &= 32767;
	}
	else
	{
		makeitfall(actor);
		ssp(actor, CLIPMASK0);
		if (s->xvel > 0) s->xvel -= 2;

		if (s->zvel == 0)
		{
			s->cstat |= 32768;

			if (s->pal != 2 && (isRR() || s->hitag == 0))
				S_PlayActorSound(SOMETHING_DRIPPING, actor);

			auto Owner = actor->GetOwner();
			if (!Owner || Owner->s->picnum != drip)
			{
				deletesprite(actor);
			}
			else
			{
				s->z = t[0];
				s->backupz();
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

void movedoorshock(DDukeActor* actor)
{
	auto s = actor->s;
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

void movetouchplate(DDukeActor* actor, int plate)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
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
				if (p >= 0) ps[p].pos.z += sector[sect].extra;
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
					ps[p].pos.z -= sector[sect].extra;
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
		DukeStatIterator it(STAT_STANDABLE);
		while (auto act2 = it.Next())
		{
			if (act2 != actor && act2->s->picnum == plate && act2->s->lotag == s->lotag)
			{
				act2->temp_data[1] = 1;
				act2->temp_data[3] = t[3];
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveooz(DDukeActor* actor, int seenine, int seeninedead, int ooz, int explosion)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	int j;
	if (s->shade != -32 && s->shade != -33)
	{
		if (s->xrepeat)
			j = (fi.ifhitbyweapon(actor) >= 0);
		else
			j = 0;

		if (j || s->shade == -31)
		{
			if (j) s->lotag = 0;

			t[3] = 1;

			DukeStatIterator it(STAT_STANDABLE);
			while (auto act2 = it.Next())
			{
				auto ss = act2->s;
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
				actor->temp_data[2]++;
				if (actor->temp_data[2] == 3)
				{
					if (s->picnum == ooz)
					{
						actor->temp_data[2] = 0;
						detonate(actor, explosion);
						return;
					}
					if (s->picnum != (seeninedead + 1))
					{
						actor->temp_data[2] = 0;

						if (s->picnum == seeninedead) s->picnum++;
						else if (s->picnum == seenine)
							s->picnum = seeninedead;
					}
					else
					{
						detonate(actor, explosion);
						return;
					}
				}
				return;
			}
			detonate(actor, explosion);
			return;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movecanwithsomething(DDukeActor* actor)
{
	makeitfall(actor);
	int j = fi.ifhitbyweapon(actor);
	if (j >= 0)
	{
		S_PlayActorSound(VENT_BUST, actor);
		for (j = 0; j < 10; j++)
			RANDOMSCRAP(actor);

		if (actor->s->lotag) spawn(actor, actor->s->lotag);
		deletesprite(actor);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void bounce(DDukeActor* actor)
{
	auto s = actor->s;
	int xvect = MulScale(s->xvel, bcos(s->ang), 10);
	int yvect = MulScale(s->xvel, bsin(s->ang), 10);
	int zvect = s->zvel;

	int hitsect = s->sectnum;

	int k = sector[hitsect].wallptr; 
	int l = wall[k].point2;
	int daang = getangle(wall[l].x - wall[k].x, wall[l].y - wall[k].y);

	if (s->z < (actor->floorz + actor->ceilingz) >> 1)
		k = sector[hitsect].ceilingheinum;
	else
		k = sector[hitsect].floorheinum;

	int dax = MulScale(k, bsin(daang), 14);
	int day = MulScale(k, -bcos(daang), 14);
	int daz = 4096;

	k = xvect * dax + yvect * day + zvect * daz;
	l = dax * dax + day * day + daz * daz;
	if ((abs(k) >> 14) < l)
	{
		k = DivScale(k, l, 17);
		xvect -= MulScale(dax, k, 16);
		yvect -= MulScale(day, k, 16);
		zvect -= MulScale(daz, k, 16);
	}

	s->zvel = zvect;
	s->xvel = ksqrt(DMulScale(xvect, xvect, yvect, yvect, 8));
	s->ang = getangle(xvect, yvect);
}

//---------------------------------------------------------------------------
//
// taken out of moveweapon
//
//---------------------------------------------------------------------------

void movetongue(DDukeActor *actor, int tongue, int jaw)
{
	auto s = actor->s;
	actor->temp_data[0] = bsin(actor->temp_data[1], -9);
	actor->temp_data[1] += 32;
	if (actor->temp_data[1] > 2047)
	{
		deletesprite(actor);
		return;
	}

	auto Owner = actor->GetOwner();
	if (!Owner) return;

	if (Owner->s->statnum == MAXSTATUS)
		if (badguy(Owner) == 0)
		{
			deletesprite(actor);
			return;
		}

	s->ang = Owner->s->ang;
	s->x = Owner->s->x;
	s->y = Owner->s->y;
	if (Owner->s->picnum == TILE_APLAYER)
		s->z = Owner->s->z - (34 << 8);
	for (int k = 0; k < actor->temp_data[0]; k++)
	{
		auto q = EGS(s->sectnum,
			s->x + MulScale(k, bcos(s->ang), 9),
			s->y + MulScale(k, bsin(s->ang), 9),
			s->z + ((k * Sgn(s->zvel)) * abs(s->zvel / 12)), tongue, -40 + (k << 1),
			8, 8, 0, 0, 0, actor, 5);
		if (q)
		{
			q->s->cstat = 128;
			q->s->pal = 8;
	}
	}
	int k = actor->temp_data[0];	// do not depend on the above loop counter.
	auto spawned = EGS(s->sectnum,
		s->x + MulScale(k, bcos(s->ang), 9),
		s->y + MulScale(k, bsin(s->ang), 9),
		s->z + ((k * Sgn(s->zvel)) * abs(s->zvel / 12)), jaw, -40,
		32, 32, 0, 0, 0, actor, 5);
	if (spawned)
	{
		spawned->s->cstat = 128;
		if (actor->temp_data[1] > 512 && actor->temp_data[1] < (1024))
			spawned->s->picnum = jaw + 1;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void rpgexplode(DDukeActor *actor, int hit, const vec3_t &pos, int EXPLOSION2, int EXPLOSION2BOT, int newextra, int playsound)
{
	auto s = actor->s;
	auto explosion = spawn(actor, EXPLOSION2);
	explosion->s->pos = pos;

	if (s->xrepeat < 10)
	{
		explosion->s->xrepeat = 6;
		explosion->s->yrepeat = 6;
	}
	else if (hit == kHitSector)
	{
		if (s->zvel > 0 && EXPLOSION2BOT >= 0)
			spawn(actor, EXPLOSION2BOT);
		else
		{
			explosion->s->cstat |= 8;
			explosion->s->z += (48 << 8);
		}
	}
	if (newextra > 0) s->extra = newextra;
	S_PlayActorSound(playsound, actor);

	if (s->xrepeat >= 10)
	{
		int x = s->extra;
		fi.hitradius(actor, gs.rpgblastradius, x >> 2, x >> 1, x - (x >> 2), x);
	}
	else
	{
		int x = s->extra + (global_random & 3);
		fi.hitradius(actor, (gs.rpgblastradius >> 1), x >> 2, x >> 1, x - (x >> 2), x);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool respawnmarker(DDukeActor *actor, int yellow, int green)
{
	actor->temp_data[0]++;
	if (actor->temp_data[0] > gs.respawnitemtime)
	{
		deletesprite(actor);
		return false;
	}
	if (actor->temp_data[0] >= (gs.respawnitemtime >> 1) && actor->temp_data[0] < ((gs.respawnitemtime >> 1) + (gs.respawnitemtime >> 2)))
		actor->s->picnum = yellow;
	else if (actor->temp_data[0] > ((gs.respawnitemtime >> 1) + (gs.respawnitemtime >> 2)))
		actor->s->picnum = green;
	makeitfall(actor);
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool rat(DDukeActor* actor, bool makesound)
{
	auto s = actor->s;
	makeitfall(actor);
	if (ssp(actor, CLIPMASK0))
	{
		if (makesound && (krand() & 255) == 0) S_PlayActorSound(RATTY, actor);
		s->ang += (krand() & 31) - 15 + bsin(actor->temp_data[0] << 8, -11);
	}
	else
	{
		actor->temp_data[0]++;
		if (actor->temp_data[0] > 1)
		{
			deletesprite(actor);
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

bool queball(DDukeActor *actor, int pocket, int queball, int stripeball)
{
	auto s = actor->s;
	if (s->xvel)
	{
		DukeStatIterator it(STAT_DEFAULT);
		while (auto aa = it.Next())
		{
			if (aa->s->picnum == pocket && ldist(aa, actor) < 52)
			{
				deletesprite(actor);
				return false;
			}
		}

		Collision coll;
		int j = clipmove_ex(&s->x, &s->y, &s->z, &s->sectnum,
			(MulScale(s->xvel, bcos(s->ang), 14) * TICSPERFRAME) << 11,
			(MulScale(s->xvel, bsin(s->ang), 14) * TICSPERFRAME) << 11,
			24L, (4 << 8), (4 << 8), CLIPMASK1, coll);

		if (j == kHitWall)
		{
			j = coll.index;
				int k = getangle(
					wall[wall[j].point2].x - wall[j].x,
					wall[wall[j].point2].y - wall[j].y);
				s->ang = ((k << 1) - s->ang) & 2047;
			}
		else if (j == kHitSprite)
		{
			fi.checkhitsprite(actor, coll.actor);
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
		int p = findplayer(actor, &x);

		if (x < 1596)
		{
			//						if(s->pal == 12)
			{
				int j = getincangle(ps[p].angle.ang.asbuild(), getangle(s->x - ps[p].pos.x, s->y - ps[p].pos.y));
				if (j > -64 && j < 64 && PlayerInput(p, SB_OPEN))
					if (ps[p].toggle_key_flag == 1)
					{
						DukeStatIterator it(STAT_ACTOR);
						DDukeActor *act2;
						while ((act2 = it.Next()))
						{
							auto sa = act2->s;
							if (sa->picnum == queball || sa->picnum == stripeball)
							{
								j = getincangle(ps[p].angle.ang.asbuild(), getangle(sa->x - ps[p].pos.x, sa->y - ps[p].pos.y));
								if (j > -64 && j < 64)
								{
									int l;
									findplayer(act2, &l);
									if (x > l) break;
								}
							}
						}
						if (act2 == nullptr)
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
			s->ang = getangle(s->x - ps[p].pos.x, s->y - ps[p].pos.y);
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

void forcesphere(DDukeActor* actor, int forcesphere)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	int sect = s->sectnum;
	if (s->yvel == 0)
	{
		s->yvel = 1;

		for (int l = 512; l < (2048 - 512); l += 128)
			for (int j = 0; j < 2048; j += 128)
			{
				auto k = spawn(actor, forcesphere);
				k->s->cstat = 257 + 128;
				k->s->clipdist = 64;
				k->s->ang = j;
				k->s->zvel = bsin(l, -5);
				k->s->xvel = bcos(l, -9);
				k->SetOwner(actor);
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
			deletesprite(actor);
			return;
		}
		else if (t[2] > 10)
		{
			DukeStatIterator it(STAT_MISC);
			while (auto aa = it.Next())
			{
				if (aa->GetOwner() == actor && aa->s->picnum == forcesphere)
					aa->temp_data[1] = 1 + (krand() & 63);
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

void recon(DDukeActor *actor, int explosion, int firelaser, int attacksnd, int painsnd, int roamsnd, int shift, int (*getspawn)(DDukeActor* i))
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	int sect = s->sectnum;
	int a;

	getglobalz(actor);

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
	if (fi.ifhitbyweapon(actor) >= 0)
	{
		if (s->extra < 0 && t[0] != -1)
		{
			t[0] = -1;
			s->extra = 0;
		}
		if (painsnd >= 0) S_PlayActorSound(painsnd, actor);
		RANDOMSCRAP(actor);
	}

	if (t[0] == -1)
	{
		s->z += 1024;
		t[2]++;
		if ((t[2] & 3) == 0) spawn(actor, explosion);
		getglobalz(actor);
		s->ang += 96;
		s->xvel = 128;
		int j = ssp(actor, CLIPMASK0);
		if (j != 1 || s->z > actor->floorz)
		{
			for (int l = 0; l < 16; l++)
				RANDOMSCRAP(actor);
			S_PlayActorSound(LASERTRIP_EXPLODE, actor);
			int sp = getspawn(actor);
			if (sp >= 0) spawn(actor, sp);
			ps[myconnectindex].actors_killed++;
			deletesprite(actor);
		}
		return;
	}
	else
	{
		if (s->z > actor->floorz - (48 << 8))
			s->z = actor->floorz - (48 << 8);
	}

	int x;
	int p = findplayer(actor, &x);
	auto Owner = actor->GetOwner();

	// 3 = findplayerz, 4 = shoot

	if (t[0] >= 4)
	{
		t[2]++;
		if ((t[2] & 15) == 0)
		{
			a = s->ang;
			s->ang = actor->tempang;
			if (attacksnd >= 0) S_PlayActorSound(attacksnd, actor);
			fi.shoot(actor, firelaser);
			s->ang = a;
		}
		if (t[2] > (26 * 3) || !cansee(s->x, s->y, s->z - (16 << 8), s->sectnum, ps[p].pos.x, ps[p].pos.y, ps[p].pos.z, ps[p].cursectnum))
		{
			t[0] = 0;
			t[2] = 0;
		}
		else actor->tempang +=
			getincangle(actor->tempang, getangle(ps[p].pos.x - s->x, ps[p].pos.y - s->y)) / 3;
	}
	else if (t[0] == 2 || t[0] == 3)
	{
		t[3] = 0;
		if (s->xvel > 0) s->xvel -= 16;
		else s->xvel = 0;

		if (t[0] == 2)
		{
			int l = ps[p].pos.z - s->z;
			if (abs(l) < (48 << 8)) t[0] = 3;
			else s->z += Sgn(ps[p].pos.z - s->z) << shift; // The shift here differs between Duke and RR.
		}
		else
		{
			t[2]++;
			if (t[2] > (26 * 3) || !cansee(s->x, s->y, s->z - (16 << 8), s->sectnum, ps[p].pos.x, ps[p].pos.y, ps[p].pos.z, ps[p].cursectnum))
			{
				t[0] = 1;
				t[2] = 0;
			}
			else if ((t[2] & 15) == 0 && attacksnd >= 0)
			{
				S_PlayActorSound(attacksnd, actor);
				fi.shoot(actor, firelaser);
			}
		}
		s->ang += getincangle(s->ang, getangle(ps[p].pos.x - s->x, ps[p].pos.y - s->y)) >> 2;
	}

	if (t[0] != 2 && t[0] != 3 && Owner)
	{
		int l = ldist(Owner, actor);
		if (l <= 1524)
		{
			a = s->ang;
			s->xvel >>= 1;
		}
		else a = getangle(Owner->s->x - s->x, Owner->s->y - s->y);

		if (t[0] == 1 || t[0] == 4) // Found a locator and going with it
		{
			l = dist(Owner, actor);

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
				actor->tempang = s->ang;
			}
		}

		if (t[0] == 0 || t[0] == 5)
		{
			if (t[0] == 0)
				t[0] = 1;
			else t[0] = 4;
			auto NewOwner = LocateTheLocator(s->hitag, -1);
			if (!NewOwner)
			{
				s->hitag = actor->temp_data[5];
				NewOwner = LocateTheLocator(s->hitag, -1);
				if (!NewOwner)
				{
					deletesprite(actor);
					return;
				}
			}
			else s->hitag++;
			actor->SetOwner(NewOwner);
		}

		t[3] = getincangle(s->ang, a);
		s->ang += t[3] >> 3;

		if (s->z < Owner->s->z)
			s->z += 1024;
		else s->z -= 1024;
	}

	if (roamsnd >= 0 && S_CheckActorSoundPlaying(actor, roamsnd) < 1)
		S_PlayActorSound(roamsnd, actor);

	ssp(actor, CLIPMASK0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ooz(DDukeActor *actor)
{
	getglobalz(actor);

	int j = (actor->floorz - actor->ceilingz) >> 9;
	if (j > 255) j = 255;

	int x = 25 - (j >> 1);
	if (x < 8) x = 8;
	else if (x > 48) x = 48;

	actor->s->yrepeat = j;
	actor->s->xrepeat = x;
	actor->s->z = actor->floorz;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void reactor(DDukeActor* actor, int REACTOR, int REACTOR2, int REACTORBURNT, int REACTOR2BURNT, int REACTORSPARK, int REACTOR2SPARK)
{
	spritetype* s = actor->s;
	int* t = &actor->temp_data[0];
	int sect = actor->s->sectnum;

	if (t[4] == 1)
	{
		DukeSectIterator it(sect);
		while (auto act2 = it.Next())
		{
			auto sprj = act2->s;
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
		}		
		return;
	}

	if (t[1] >= 20)
	{
		t[4] = 1;
		return;
	}

	int x;
	int p = findplayer(actor, &x);

	t[2]++;
	if (t[2] == 4) t[2] = 0;

	if (x < 4096)
	{
		if ((krand() & 255) < 16)
		{
			if (!S_CheckSoundPlaying(DUKE_LONGTERM_PAIN))
				S_PlayActorSound(DUKE_LONGTERM_PAIN, ps[p].GetActor());

			S_PlayActorSound(SHORT_CIRCUIT, actor);

			ps[p].GetActor()->s->extra--;
			SetPlayerPal(&ps[p], PalEntry(32, 32, 0, 0));
		}
		t[0] += 128;
		if (t[3] == 0)
			t[3] = 1;
	}
	else t[3] = 0;

	if (t[1])
	{
		t[1]++;

		t[4] = s->z;
		s->z = sector[sect].floorz - (krand() % (sector[sect].floorz - sector[sect].ceilingz));

		switch (t[1])
		{
		case 3:
		{
			//Turn on all of those flashing sectoreffector.
			fi.hitradius(actor, 4096,
				gs.impact_damage << 2,
				gs.impact_damage << 2,
				gs.impact_damage << 2,
				gs.impact_damage << 2);
			DukeStatIterator it(STAT_STANDABLE);
			while (auto act2 = it.Next())
			{
				auto sj = act2->s;
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
			DukeSectIterator it(sect);
			while (auto a2 = it.Next())
			{
				if (a2 != actor)
				{
					deletesprite(a2);
					break;
				}
			}
			break;
		}
		}
		for (x = 0; x < 16; x++)
			RANDOMSCRAP(actor);

		s->z = t[4];
		t[4] = 0;

	}
	else
	{
		int j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{
			for (x = 0; x < 32; x++)
				RANDOMSCRAP(actor);
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

void camera(DDukeActor *actor)
{
	spritetype* s = actor->s;
	int* t = &actor->temp_data[0];
	if (t[0] == 0)
	{
		if (gs.camerashitable)
		{
			int j = fi.ifhitbyweapon(actor);
			if (j >= 0)
			{
				t[0] = 1; // static
				s->cstat = (short)32768;
				for (int x = 0; x < 5; x++)
					RANDOMSCRAP(actor);
				return;
			}
		}
		
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

void forcesphereexplode(DDukeActor *actor)
{
	int* t = &actor->temp_data[0];
	int l = actor->s->xrepeat;
	if (t[1] > 0)
	{
		t[1]--;
		if (t[1] == 0)
		{
			deletesprite(actor);
			return;
		}
	}
	auto Owner = actor->GetOwner();
	if (!Owner) return;
	if (Owner->temp_data[1] == 0)
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

	actor->s->x = Owner->s->x;
	actor->s->y = Owner->s->y;
	actor->s->z = Owner->s->z;
	actor->s->ang += Owner->temp_data[0];

	if (l > 64) l = 64;
	else if (l < 1) l = 1;

	actor->s->xrepeat = l;
	actor->s->yrepeat = l;
	actor->s->shade = (l >> 1) - 48;

	for (int j = t[0]; j > 0; j--)
		ssp(actor, CLIPMASK0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void watersplash2(DDukeActor* actor)
{
	int sect = actor->s->sectnum;
	int* t = &actor->temp_data[0];
	t[0]++;
	if (t[0] == 1)
	{
		if (sector[sect].lotag != 1 && sector[sect].lotag != 2)
		{
			deletesprite(actor);
			return;
		}
		if (!S_CheckSoundPlaying(ITEM_SPLASH))
			S_PlayActorSound(ITEM_SPLASH, actor);
	}
	if (t[0] == 3)
	{
		t[0] = 0;
		t[1]++;
	}
	if (t[1] == 5)
		deletesprite(actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void frameeffect1(DDukeActor *actor)
{
	int* t = &actor->temp_data[0];
	auto Owner = actor->GetOwner();
	if (Owner)
	{
		t[0]++;

		if (t[0] > 7)
		{
			deletesprite(actor);
			return;
		}
		else if (t[0] > 4) actor->s->cstat |= 512 + 2;
		else if (t[0] > 2) actor->s->cstat |= 2;
		actor->s->xoffset = Owner->s->xoffset;
		actor->s->yoffset = Owner->s->yoffset;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool money(DDukeActor* actor, int BLOODPOOL)
{
	auto s = actor->s;
	int sect = s->sectnum;
	int* t = &actor->temp_data[0];

	s->xvel = (krand() & 7) + bsin(actor->temp_data[0], -9);
	actor->temp_data[0] += (krand() & 63);
	if ((actor->temp_data[0] & 2047) > 512 && (actor->temp_data[0] & 2047) < 1596)
	{
		if (sector[sect].lotag == 2)
		{
			if (s->zvel < 64)
				s->zvel += (gs.gravity >> 5) + (krand() & 7);
		}
		else
			if (s->zvel < 144)
				s->zvel += (gs.gravity >> 5) + (krand() & 7);
	}

	ssp(actor, CLIPMASK0);

	if ((krand() & 3) == 0)
		setsprite(actor, s->pos);

	if (s->sectnum == -1)
	{
		deletesprite(actor);
		return false;
	}
	int l = getflorzofslope(s->sectnum, s->x, s->y);

	if (s->z > l)
	{
		s->z = l;

		insertspriteq(actor);
		s->picnum++;

		DukeStatIterator it(STAT_MISC);
		while (auto aa = it.Next())
		{
			if (aa->s->picnum == BLOODPOOL)
				if (ldist(actor, aa) < 348)
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

bool jibs(DDukeActor *actor, int JIBS6, bool timeout, bool callsetsprite, bool floorcheck, bool zcheck1, bool zcheck2)
{
	spritetype* s = actor->s;
	int sect = s->sectnum;
	int* t = &actor->temp_data[0];

	if (s->xvel > 0) s->xvel--;
	else s->xvel = 0;

	if (timeout)
	{
		if (t[5] < 30 * 10)
			t[5]++;
		else
		{
			deletesprite(actor);
			return false;
		}
	}

	if (s->zvel > 1024 && s->zvel < 1280)
	{
		setsprite(actor, s->pos);
		sect = s->sectnum;
	}

	if (callsetsprite) setsprite(actor, s->pos);

	int l = getflorzofslope(sect, s->x, s->y);
	int x = getceilzofslope(sect, s->x, s->y);
	if (x == l || sect < 0 || sect >= MAXSECTORS)
	{
		deletesprite(actor);
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
			else s->zvel += gs.gravity - 50;
		}

		s->x += MulScale(s->xvel, bcos(s->ang), 14);
		s->y += MulScale(s->xvel, bsin(s->ang), 14);
		s->z += s->zvel;

		if (floorcheck && s->z >= sector[s->sectnum].floorz)
		{
			deletesprite(actor);
			return false;
		}
	}
	else
	{
		if (zcheck2)
		{
			deletesprite(actor);
			return false;
		}
		if (t[2] == 0)
		{
			if (s->sectnum == -1)
			{
				deletesprite(actor);
				return false;
			}
			if ((sector[s->sectnum].floorstat & 2))
			{
				deletesprite(actor);
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
				deletesprite(actor);
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

bool bloodpool(DDukeActor* actor, bool puke, int TIRE)
{
	spritetype* s = actor->s;
	int sect = s->sectnum;
	int* t = &actor->temp_data[0];

	if (t[0] == 0)
	{
		t[0] = 1;
		if (sector[sect].floorstat & 2)
		{
			deletesprite(actor);
			return false;
		}
		else insertspriteq(actor);
	}

	makeitfall(actor);

	int x;
	int p = findplayer(actor, &x);

	s->z = actor->floorz - (FOURSLEIGHT);

	if (t[2] < 32)
	{
		t[2]++;
		if (actor->picnum == TIRE)
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
					S_PlayActorSound(DUKE_LONGTERM_PAIN, ps[p].GetActor());
				ps[p].GetActor()->s->extra--;
				SetPlayerPal(&ps[p], PalEntry(32, 16, 0, 0));
			}
		}

		if (t[1] == 1) return false;
		t[1] = 1;

		if (actor->picnum == TIRE)
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

void shell(DDukeActor* actor, bool morecheck)
{
	spritetype* s = actor->s;
	int sect = s->sectnum;
	int* t = &actor->temp_data[0];

	ssp(actor, CLIPMASK0);

	if (sect < 0 || morecheck)
	{
		deletesprite(actor);
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
		if (s->zvel < 128) s->zvel += (gs.gravity / 13); // 8
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
		if (s->zvel < 512) s->zvel += (gs.gravity / 3); // 52;
		if (s->xvel > 0)
			s->xvel--;
		else
		{
			deletesprite(actor);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void glasspieces(DDukeActor* actor)
{
	spritetype* s = actor->s;
	int sect = s->sectnum;
	int* t = &actor->temp_data[0];

	makeitfall(actor);

	if (s->zvel > 4096) s->zvel = 4096;
	if (sect < 0)
	{
		deletesprite(actor);
		return;
	}

	if (s->z == actor->floorz - (FOURSLEIGHT) && t[0] < 3)
	{
		s->zvel = -((3 - t[0]) << 8) - (krand() & 511);
		if (sector[sect].lotag == 2)
			s->zvel >>= 1;
		s->xrepeat >>= 1;
		s->yrepeat >>= 1;
		if (rnd(96))
			setsprite(actor, s->pos);
		t[0]++;//Number of bounces
	}
	else if (t[0] == 3)
	{
		deletesprite(actor);
		return;
	}

	if (s->xvel > 0)
	{
		s->xvel -= 2;
		s->cstat = ((s->xvel & 3) << 2);
	}
	else s->xvel = 0;

	ssp(actor, CLIPMASK0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void scrap(DDukeActor* actor, int SCRAP1, int SCRAP6)
{
	spritetype* s = actor->s;
	int sect = s->sectnum;
	int* t = &actor->temp_data[0];

	if (s->xvel > 0)
		s->xvel--;
	else s->xvel = 0;

	if (s->zvel > 1024 && s->zvel < 1280)
	{
		setsprite(actor, s->pos);
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
		if (s->zvel < 4096) s->zvel += gs.gravity - 50;
		s->x += MulScale(s->xvel, bcos(s->ang), 14);
		s->y += MulScale(s->xvel, bsin(s->ang), 14);
		s->z += s->zvel;
	}
	else
	{
		if (s->picnum == SCRAP1 && s->yvel > 0)
		{
			auto spawned = spawn(actor, s->yvel);
			setsprite(spawned, s->pos);
			getglobalz(spawned);
			spawned->s->hitag = spawned->s->lotag = 0;
		}
		deletesprite(actor);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void gutsdir(DDukeActor* actor, short gtype, short n, short p)
{
	int sx, sy;

	if (badguy(actor) && actor->s->xrepeat < 16)
		sx = sy = 8;
	else sx = sy = 32;

	int gutz = actor->s->z - (8 << 8);
	int floorz = getflorzofslope(actor->s->sectnum, actor->s->x, actor->s->y);

	if (gutz > (floorz - (8 << 8)))
		gutz = floorz - (8 << 8);

	gutz += gs.actorinfo[actor->s->picnum].gutsoffset;

	for (int j = 0; j < n; j++)
	{
		int a = krand() & 2047;
		int r1 = krand();
		int r2 = krand();
		// TRANSITIONAL: owned by a player???
		EGS(actor->s->sectnum, actor->s->x, actor->s->y, gutz, gtype, -32, sx, sy, a, 256 + (r2 & 127), -512 - (r1 & 2047), ps[p].GetActor(), 5);
	}
}

//---------------------------------------------------------------------------
//
// taken out of moveeffectors
//
//---------------------------------------------------------------------------

void handle_se00(DDukeActor* actor, int LASERLINE)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	sectortype *sect = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	int zchange = 0;

	auto Owner = actor->GetOwner();

	if (!Owner || Owner->s->lotag == (short)65535)
	{
		deletesprite(actor);
		return;
	}

	int q = sect->extra >> 3;
	int l = 0;

	if (sect->lotag == 30)
	{
		q >>= 2;

		if (s->extra == 1)
		{
			if (actor->tempang < 256)
			{
				actor->tempang += 4;
				if (actor->tempang >= 256)
					callsound(s->sectnum, actor);
				if (s->clipdist) l = 1;
				else l = -1;
			}
			else actor->tempang = 256;

			if (sect->floorz > s->z) //z's are touching
			{
				sect->floorz -= 512;
				zchange = -512;
				if (sect->floorz < s->z)
					sect->floorz = s->z;
			}

			else if (sect->floorz < s->z) //z's are touching
			{
				sect->floorz += 512;
				zchange = 512;
				if (sect->floorz > s->z)
					sect->floorz = s->z;
			}
		}
		else if (s->extra == 3)
		{
			if (actor->tempang > 0)
			{
				actor->tempang -= 4;
				if (actor->tempang <= 0)
					callsound(s->sectnum, actor);
				if (s->clipdist) l = -1;
				else l = 1;
			}
			else actor->tempang = 0;

			if (sect->floorz > actor->temp_data[3]) //z's are touching
			{
				sect->floorz -= 512;
				zchange = -512;
				if (sect->floorz < actor->temp_data[3])
					sect->floorz = actor->temp_data[3];
			}

			else if (sect->floorz < actor->temp_data[3]) //z's are touching
			{
				sect->floorz += 512;
				zchange = 512;
				if (sect->floorz > actor->temp_data[3])
					sect->floorz = actor->temp_data[3];
			}
		}

		s->ang += (l * q);
		t[2] += (l * q);
	}
	else
	{
		if (Owner->temp_data[0] == 0) return;
		if (Owner->temp_data[0] == 2)
		{
			deletesprite(actor);
			return;
		}

		if (Owner->s->ang > 1024)
			l = -1;
		else l = 1;
		if (t[3] == 0)
			t[3] = ldist(actor, Owner);
		s->xvel = t[3];
		s->x = Owner->s->x;
		s->y = Owner->s->y;
		s->ang += (l * q);
		t[2] += (l * q);
	}

	if (l && (sect->floorstat & 64))
	{
		int p;
		for (p = connecthead; p >= 0; p = connectpoint2[p])
		{
			if (ps[p].cursectnum == s->sectnum && ps[p].on_ground == 1)
			{
				ps[p].angle.addadjustment(l * q);

				ps[p].pos.z += zchange;

				int m, x;
				rotatepoint(Owner->s->x, Owner->s->y,	ps[p].pos.x, ps[p].pos.y, (q * l), &m, &x);

				ps[p].bobposx += m - ps[p].pos.x;
				ps[p].bobposy += x - ps[p].pos.y;

				ps[p].pos.x = m;
				ps[p].pos.y = x;

				auto psp = ps[p].GetActor();
				if (psp->s->extra <= 0)
				{
					psp->s->x = m;
					psp->s->y = x;
				}
			}
		}
		DukeSectIterator itp(s->sectnum);
		while (auto ap = itp.Next())
		{
			auto sprp = ap->s;
			if (sprp->statnum != 3 && sprp->statnum != 4)
				if (LASERLINE < 0 || sprp->picnum != LASERLINE)
				{
					if (sprp->picnum == TILE_APLAYER && ap->GetOwner())
					{
						continue;
					}

					sprp->ang += (l * q);
					sprp->ang &= 2047;

					sprp->z += zchange;
					rotatepoint(Owner->s->x, Owner->s->y, ap->s->x, ap->s->y, (q* l), &ap->s->x, &ap->s->y);
				}
		}

	}
	ms(actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se01(DDukeActor *actor)
{
	int* t = &actor->temp_data[0];
	int sh = actor->s->hitag;
	if (actor->GetOwner() == nullptr) //Init
	{
		actor->SetOwner(actor);

		DukeStatIterator it(STAT_EFFECTOR);
		while (auto ac = it.Next())
		{
			if (ac->s->lotag == 19 && ac->s->hitag == sh)
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

void handle_se14(DDukeActor* actor, bool checkstat, int RPG, int JIBS6)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	if (actor->GetOwner() == nullptr)
	{
		auto NewOwner = LocateTheLocator(t[3], t[0]);

		if (NewOwner == nullptr)
		{
			I_Error("Could not find any locators for SE# 6 and 14 with a hitag of %d.", t[3]);
		}
		actor->SetOwner(NewOwner);
	}

	auto Owner = actor->GetOwner();
	int j = ldist(Owner, actor);

	if (j < 1024)
	{
		if (st == 6)
			if (Owner->s->hitag & 1)
				t[4] = sc->extra; //Slow it down
		t[3]++;
		auto NewOwner = LocateTheLocator(t[3], t[0]);
		if (NewOwner == nullptr)
		{
			t[3] = 0;
			NewOwner = LocateTheLocator(0, t[0]);
		}
		if (NewOwner) actor->SetOwner(NewOwner);
	}

	Owner = actor->GetOwner();
	if (s->xvel)
	{
		int x = getangle(Owner->s->x - s->x, Owner->s->y - s->y);
		int q = getincangle(s->ang, x) >> 3;

		t[2] += q;
		s->ang += q;

		bool statstate = (!checkstat || ((sc->floorstat & 1) == 0 && (sc->ceilingstat & 1) == 0));
		if (s->xvel == sc->extra)
		{
			if (statstate)
			{
				if (!S_CheckSoundPlaying(actor->lastvx))
					S_PlayActorSound(actor->lastvx, actor);
			}
			if ((!checkstat || !statstate) && (ud.monsters_off == 0 && sc->floorpal == 0 && (sc->floorstat & 1) && rnd(8)))
			{
				int p = findplayer(actor, &x);
				if (x < 20480)
				{
					j = s->ang;
					s->ang = getangle(s->x - ps[p].pos.x, s->y - ps[p].pos.y);
					fi.shoot(actor, RPG);
					s->ang = j;
				}
			}
		}

		if (s->xvel <= 64 && statstate)
			S_StopSound(actor->lastvx, actor);

		if ((sc->floorz - sc->ceilingz) < (108 << 8))
		{
			if (ud.clipping == 0 && s->xvel >= 192)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
				{
					auto psp = ps[p].GetActor();
					if (psp->s->extra > 0)
					{
						short k = ps[p].cursectnum;
						updatesector(ps[p].pos.x, ps[p].pos.y, &k);
						if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && ps[p].cursectnum != s->sectnum))
						{
							ps[p].pos.x = s->x;
							ps[p].pos.y = s->y;
							ps[p].cursectnum = s->sectnum;

							setsprite(ps[p].GetActor(), s->pos);
							quickkill(&ps[p]);
						}
					}
				}
		}

		int m = MulScale(s->xvel, bcos(s->ang), 14);
		x = MulScale(s->xvel, bsin(s->ang), 14);

		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			auto psp = ps[p].GetActor();
			if (sector[ps[p].cursectnum].lotag != 2)
			{
				if (po[p].os == s->sectnum)
				{
					po[p].ox += m;
					po[p].oy += x;
				}

				if (s->sectnum == psp->s->sectnum)
				{
					rotatepoint(s->x, s->y, ps[p].pos.x, ps[p].pos.y, q, &ps[p].pos.x, &ps[p].pos.y);

					ps[p].pos.x += m;
					ps[p].pos.y += x;

					ps[p].bobposx += m;
					ps[p].bobposy += x;

					ps[p].angle.addadjustment(q);

					if (numplayers > 1)
					{
						ps[p].oposx = ps[p].pos.x;
						ps[p].oposy = ps[p].pos.y;
					}
					if (psp->s->extra <= 0)
					{
						psp->s->x = ps[p].pos.x;
						psp->s->y = ps[p].pos.y;
					}
				}
			}
		}
		DukeSectIterator it(s->sectnum);
		while (auto a2 = it.Next())
		{
			auto sj = a2->s;
			if (sj->statnum != 10 && sector[sj->sectnum].lotag != 2 && sj->picnum != SECTOREFFECTOR && sj->picnum != LOCATORS)
			{
				rotatepoint(s->x, s->y, sj->x, sj->y, q, &sj->x, &sj->y);

				sj->x += m;
				sj->y += x;

				sj->ang += q;

				if (numplayers > 1)
				{
					sj->backupvec2();
				}
			}
		}

		ms(actor);
		setsprite(actor, s->pos);

		if ((sc->floorz - sc->ceilingz) < (108 << 8))
		{
			if (ud.clipping == 0 && s->xvel >= 192)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
				{
					if (ps[p].GetActor()->s->extra > 0)
					{
						short k = ps[p].cursectnum;
						updatesector(ps[p].pos.x, ps[p].pos.y, &k);
						if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && ps[p].cursectnum != s->sectnum))
						{
							ps[p].oposx = ps[p].pos.x = s->x;
							ps[p].oposy = ps[p].pos.y = s->y;
							ps[p].cursectnum = s->sectnum;

							setsprite(ps[p].GetActor(), s->pos);
							quickkill(&ps[p]);
						}
					}
				}

			auto Owner = actor->GetOwner();
			if (Owner)
			{
				DukeSectIterator itr(Owner->s->sectnum);
				while (auto a2 = itr.Next())
				{
					if (a2->s->statnum == 1 && badguy(a2) && a2->s->picnum != SECTOREFFECTOR && a2->s->picnum != LOCATORS)
					{
						short k = a2->s->sectnum;
						updatesector(a2->s->x, a2->s->y, &k);
						if (a2->s->extra >= 0 && k == s->sectnum)
						{
							gutsdir(a2, JIBS6, 72, myconnectindex);
							S_PlayActorSound(SQUISHED, actor);
							deletesprite(a2);
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

void handle_se30(DDukeActor *actor, int JIBS6)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	auto Owner = actor->GetOwner();
	if (Owner == nullptr)
	{
		t[3] = !t[3];
		Owner = LocateTheLocator(t[3], t[0]);
		actor->SetOwner(Owner);
	}
	else
	{
		if (t[4] == 1) // Starting to go
		{
			if (ldist(Owner, actor) < (2048 - 128))
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
			int l = FindDistance2D(Owner->s->x - s->x, Owner->s->y - s->y);

			if (l <= 128)
				s->xvel = 0;

			if (s->xvel > 0)
				s->xvel -= 16;
			else
			{
				s->xvel = 0;
				operateactivators(s->hitag + (short)t[3], -1);
				actor->SetOwner(nullptr);
				s->ang += 1024;
				t[4] = 0;
				fi.operateforcefields(actor, s->hitag);
			}
		}
	}

	if (s->xvel)
	{
		int l = MulScale(s->xvel, bcos(s->ang), 14);
		int x = MulScale(s->xvel, bsin(s->ang), 14);

		if ((sc->floorz - sc->ceilingz) < (108 << 8))
			if (ud.clipping == 0)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
					{
					auto psp = ps[p].GetActor();
					if (psp->s->extra > 0)
					{
						short k = ps[p].cursectnum;
						updatesector(ps[p].pos.x, ps[p].pos.y, &k);
						if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && ps[p].cursectnum != s->sectnum))
						{
							ps[p].pos.x = s->x;
							ps[p].pos.y = s->y;
							ps[p].cursectnum = s->sectnum;

							setsprite(ps[p].GetActor(), s->pos);
							quickkill(&ps[p]);
						}
					}
				}
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			auto psp = ps[p].GetActor();
			if (psp->s->sectnum == s->sectnum)
			{
				ps[p].pos.x += l;
				ps[p].pos.y += x;

				if (numplayers > 1)
				{
					ps[p].oposx = ps[p].pos.x;
					ps[p].oposy = ps[p].pos.y;
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

		DukeSectIterator its(s->sectnum);
		while (auto a2 = its.Next())
		{
			auto spa2 = a2->s;
			if (spa2->picnum != SECTOREFFECTOR && spa2->picnum != LOCATORS)
			{
				spa2->x += l;
				spa2->y += x;

				if (numplayers > 1)
				{
					spa2->backupvec2();
				}
			}
		}

		ms(actor);
		setsprite(actor, s->pos);

		if ((sc->floorz - sc->ceilingz) < (108 << 8))
		{
			if (ud.clipping == 0)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
					if (ps[p].GetActor()->s->extra > 0)
					{
						short k = ps[p].cursectnum;
						updatesector(ps[p].pos.x, ps[p].pos.y, &k);
						if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && ps[p].cursectnum != s->sectnum))
						{
							ps[p].pos.x = s->x;
							ps[p].pos.y = s->y;

							ps[p].oposx = ps[p].pos.x;
							ps[p].oposy = ps[p].pos.y;

							ps[p].cursectnum = s->sectnum;

							setsprite(ps[p].GetActor(), s->pos);
							quickkill(&ps[p]);
						}
					}

			if (Owner)
			{
				DukeSectIterator it(Owner->s->sectnum);
				while (auto a2 = it.Next())
				{
					if (a2->s->statnum == 1 && badguy(a2) && a2->s->picnum != SECTOREFFECTOR && a2->s->picnum != LOCATORS)
					{
						//					if(a2->s.sectnum != s->sectnum)
						{
							short k = a2->s->sectnum;
							updatesector(a2->s->x, a2->s->y, &k);
							if (a2->s->extra >= 0 && k == s->sectnum)
							{
								gutsdir(a2, JIBS6, 24, myconnectindex);
								S_PlayActorSound(SQUISHED, a2);
								deletesprite(a2);
						}
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

void handle_se02(DDukeActor *actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
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
			deletesprite(actor);
			return;
		}
		else
		{
			if ((t[0] & 31) == 8)
			{
				earthquaketime = 48;
				S_PlayActorSound(EARTHQUAKE, ps[screenpeek].GetActor());
			}

			if (abs(sc->floorheinum - t[5]) < 8)
				sc->floorheinum = t[5];
			else sc->floorheinum += (Sgn(t[5] - sc->floorheinum) << 4);
		}

		int m = MulScale(s->xvel, bcos(s->ang), 14);
		int x = MulScale(s->xvel, bsin(s->ang), 14);


		for (int p = connecthead; p >= 0; p = connectpoint2[p])
			if (ps[p].cursectnum == s->sectnum && ps[p].on_ground)
			{
				ps[p].pos.x += m;
				ps[p].pos.y += x;

				ps[p].bobposx += m;
				ps[p].bobposy += x;
			}

		DukeSectIterator it(s->sectnum);
		while (auto a2 = it.Next())
		{
			auto sj = a2->s;
			if (sj->picnum != SECTOREFFECTOR)
			{
				sj->x += m;
				sj->y += x;
				setsprite(a2, sj->pos);
			}
		}
		ms(actor);
		setsprite(actor, s->pos);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se03(DDukeActor *actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	if (t[4] == 0) return;
	int x, p = findplayer(actor, &x);

	int palvals = actor->palvals;

	//	if(t[5] > 0) { t[5]--; break; }

	if ((global_random / (sh + 1) & 31) < 4 && !t[2])
	{
		//	   t[5] = 4+(global_random&7);
		sc->ceilingpal = palvals >> 8;
		sc->floorpal = palvals & 0xff;
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

void handle_se04(DDukeActor *actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	int j;

	int palvals = actor->palvals;

	if ((global_random / (sh + 1) & 31) < 4)
	{
		t[1] = s->shade + (global_random & 15);//Got really bright
		t[0] = s->shade + (global_random & 15);
		sc->ceilingpal = palvals >> 8;
		sc->floorpal = palvals & 0xff;
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
		if (j) wal->pal = (palvals & 0xff);
		else wal->pal = s->pal;

		if (wal->hitag != 1)
		{
			wal->shade = t[0];
			if ((wal->cstat & 2) && wal->nextwall >= 0)
				wall[wal->nextwall].shade = wal->shade;
		}
	}

	DukeSectIterator it(s->sectnum);
	while (auto a2 = it.Next())
	{
		auto sj = a2->s;
		if (sj->cstat & 16)
		{
			if (sc->ceilingstat & 1)
				sj->shade = sc->ceilingshade;
			else sj->shade = sc->floorshade;
		}
	}

	if (t[4])
		deletesprite(actor);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se05(DDukeActor* actor, int FIRELASER)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	int j, l, m;

	int x, p = findplayer(actor, &x);
	if (x < 8192)
	{
		j = s->ang;
		s->ang = getangle(s->x - ps[p].pos.x, s->y - ps[p].pos.y);
		fi.shoot(actor, FIRELASER);
		s->ang = j;
	}

	auto Owner = actor->GetOwner();
	if (Owner == nullptr) //Start search
	{
		t[4] = 0;
		l = 0x7fffffff;
		while (1) //Find the shortest dist
		{
			auto NewOwner = LocateTheLocator(t[4], -1); //t[0] hold sectnum
			if (NewOwner == nullptr) break;

			m = ldist(ps[p].GetActor(), NewOwner);

			if (l > m)
			{
				Owner = NewOwner;
				l = m;
			}

			t[4]++;
		}

		actor->SetOwner(Owner);
		if (!Owner) return; // Undefined case - was not checked.
		s->zvel = Sgn(Owner->s->z - s->z) << 4;
	}

	if (ldist(Owner, actor) < 1024)
	{
		short ta;
		ta = s->ang;
		s->ang = getangle(ps[p].pos.x - s->x, ps[p].pos.y - s->y);
		s->ang = ta;
		actor->SetOwner(nullptr);
		return;

	}
	else s->xvel = 256;

	x = getangle(Owner->s->x - s->x, Owner->s->y - s->y);
	int q = getincangle(s->ang, x) >> 3;
	s->ang += q;

	if (rnd(32))
	{
		t[2] += q;
		sc->ceilingshade = 127;
	}
	else
	{
		t[2] +=
			getincangle(t[2] + 512, getangle(ps[p].pos.x - s->x, ps[p].pos.y - s->y)) >> 2;
		sc->ceilingshade = 0;
	}
	j = fi.ifhitbyweapon(actor);
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
	ms(actor);
	setsprite(actor, s->pos);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se08(DDukeActor *actor, bool checkhitag1)
{
	// work only if its moving
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	int x, j = -1;

	if (actor->temp_data[4])
	{
		actor->temp_data[4]++;
		if (actor->temp_data[4] > 8)
		{
			deletesprite(actor);
			return;
		}
		j = 1;
	}
	else j = getanimationgoal(anim_ceilingz, s->sectnum);

	if (j >= 0)
	{
		short sn;

		if ((sc->lotag & 0x8000) || actor->temp_data[4])
			x = -t[3];
		else
			x = t[3];

		if (st == 9) x = -x;

		DukeStatIterator it(STAT_EFFECTOR);
		while (auto ac = it.Next())
		{
			if (((ac->s->lotag) == st) && (ac->s->hitag) == sh)
			{
				sn = ac->s->sectnum;
				auto sect = &sector[sn];
				int m = ac->s->shade;

				auto wal = &wall[sect->wallptr];

				for (int l = sect->wallnum; l > 0; l--, wal++)
				{
					if (wal->hitag != 1)
					{
						wal->shade += x;

						if (wal->shade < m)
							wal->shade = m;
						else if (wal->shade > ac->temp_data[2])
							wal->shade = ac->temp_data[2];

						if (wal->nextwall >= 0)
							if (wall[wal->nextwall].hitag != 1)
								wall[wal->nextwall].shade = wal->shade;
					}
				}

				sect->floorshade += x;
				sect->ceilingshade += x;

				if (sect->floorshade < m)
					sect->floorshade = m;
				else if (sect->floorshade > ac->temp_data[0])
					sect->floorshade = ac->temp_data[0];

				if (sect->ceilingshade < m)
					sect->ceilingshade = m;
				else if (sect->ceilingshade > ac->temp_data[1])
					sect->ceilingshade = ac->temp_data[1];

				if (checkhitag1 && sect->hitag == 1)
					sect->ceilingshade = ac->temp_data[1];

			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se10(DDukeActor* actor, const int* specialtags)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	if ((sc->lotag & 0xff) == 27 || (sc->floorz > sc->ceilingz && (sc->lotag & 0xff) != 23) || sc->lotag == (short)32791)
	{
		int j = 1;

		if ((sc->lotag & 0xff) != 27)
			for (int p = connecthead; p >= 0; p = connectpoint2[p])
				if (sc->lotag != 30 && sc->lotag != 31 && sc->lotag != 0)
					if (s->sectnum == ps[p].GetActor()->s->sectnum)
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
				fi.activatebysector(s->sectnum, actor);
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

void handle_se11(DDukeActor *actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
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

		DukeStatIterator it(STAT_ACTOR);

		for (int j = startwall; j < endwall; j++)
		{
			while (auto ac = it.Next())
			{
				auto sk = ac->s;
				if (sk->extra > 0 && badguy(ac) && clipinsidebox(sk->x, sk->y, j, 256L) == 1)
					return;
			}
		}

		int k = (s->yvel >> 3) * t[3];
		t[2] += k;
		t[4] += k;
		ms(actor);
		setsprite(actor, s->pos);

		for (int j = startwall; j < endwall; j++)
		{
			it.Reset(STAT_PLAYER);
			while (auto ac = it.Next())
			{
				auto sk = ac->s;
				if (ac->GetOwner() && clipinsidebox(sk->x, sk->y, j, 144L) == 1)
				{
					t[5] = 8; // Delay
					t[2] -= k;
					t[4] -= k;
					ms(actor);
					setsprite(actor, s->pos);
					return;
				}
			}
		}

		if (t[4] <= -511 || t[4] >= 512)
		{
			t[4] = 0;
			t[2] &= 0xffffff00;
			ms(actor);
			setsprite(actor, s->pos);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se12(DDukeActor *actor, int planeonly)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
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

		DukeSectIterator it(s->sectnum);
		while (auto a2 = it.Next())
		{
			if (a2->s->cstat & 16)
			{
				if (sc->ceilingstat & 1)
					a2->s->shade = sc->ceilingshade;
				else a2->s->shade = sc->floorshade;
			}
		}

		if (t[3] == 1)
		{
			deletesprite(actor);
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

		DukeSectIterator it(s->sectnum);
		while (auto a2 = it.Next())
		{
			if (a2->s->cstat & 16)
			{
				if (sc->ceilingstat & 1)
					a2->s->shade = sc->ceilingshade;
				else a2->s->shade = sc->floorshade;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se13(DDukeActor* actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	if (t[2])
	{
		int j = (s->yvel << 5) | 1;

		if (s->ang == 512)
		{
			if (actor->spriteextra)
			{
				if (abs(t[0] - sc->ceilingz) >= j)
					sc->ceilingz += Sgn(t[0] - sc->ceilingz) * j;
				else sc->ceilingz = t[0];
			}
			else
			{
				if (abs(t[1] - sc->floorz) >= j)
					sc->floorz += Sgn(t[1] - sc->floorz) * j;
				else sc->floorz = t[1];
			}
		}
		else
		{
			if (abs(t[1] - sc->floorz) >= j)
				sc->floorz += Sgn(t[1] - sc->floorz) * j;
			else sc->floorz = t[1];
			if (abs(t[0] - sc->ceilingz) >= j)
				sc->ceilingz += Sgn(t[0] - sc->ceilingz) * j;
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
					sc->ceilingpicnum = sector[ps[0].one_parallax_sectnum].ceilingpicnum;
					sc->ceilingshade = sector[ps[0].one_parallax_sectnum].ceilingshade;
				}
			}
		}
		t[2]++;
		if (t[2] > 256)
		{
			deletesprite(actor);
			return;
		}
	}


	if (t[2] == 4 && s->ang != 512)
		for (int x = 0; x < 7; x++) RANDOMSCRAP(actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se15(DDukeActor* actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	if (t[4])
	{
		s->xvel = 16;

		if (t[4] == 1) //Opening
		{
			if (t[3] >= (s->yvel >> 3))
			{
				t[4] = 0; //Turn off the sliders
				callsound(s->sectnum, actor);
				return;
			}
			t[3]++;
		}
		else if (t[4] == 2)
		{
			if (t[3] < 1)
			{
				t[4] = 0;
				callsound(s->sectnum, actor);
				return;
			}
			t[3]--;
		}

		ms(actor);
		setsprite(actor, s->pos);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se16(DDukeActor* actor, int REACTOR, int REACTOR2)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];

	t[2] += 32;
	if (sc->floorz < sc->ceilingz) s->shade = 0;

	else if (sc->ceilingz < t[3])
	{

		//The following code check to see if
		//there is any other sprites in the sector.
		//If there isn't, then kill this sectoreffector
		//itself.....

		DukeSectIterator it(s->sectnum);
		DDukeActor* a2;
		while ((a2 = it.Next()))
		{
			if (a2->s->picnum == REACTOR || a2->s->picnum == REACTOR2)
				return;
		}
		if (a2 == nullptr)
		{
			deletesprite(actor);
			return;
		}
		else s->shade = 1;
	}

	if (s->shade) sc->ceilingz += 1024;
	else sc->ceilingz -= 512;

	ms(actor);
	setsprite(actor, s->pos);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se17(DDukeActor* actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	int q = t[0] * (s->yvel << 2);

	sc->ceilingz += q;
	sc->floorz += q;

	DukeSectIterator it(s->sectnum);
	while (auto act1 = it.Next())
	{
		if (act1->s->statnum == STAT_PLAYER && act1->GetOwner())
		{
			int p = act1->s->yvel;
			if (numplayers < 2) ps[p].oposz = ps[p].pos.z;
			ps[p].pos.z += q;
			ps[p].truefz += q;
			ps[p].truecz += q;
			if (numplayers > 1)	ps[p].oposz = ps[p].pos.z;
		}
		if (act1->s->statnum != STAT_EFFECTOR)
		{
			act1->s->backupz();
			act1->s->z += q;
		}

		act1->floorz = sc->floorz;
		act1->ceilingz = sc->ceilingz;
	}

	if (t[0]) //If in motion
	{
		if (abs(sc->floorz - t[2]) <= s->yvel)
		{
			activatewarpelevators(actor, 0);
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

		DDukeActor* act2;
		DukeStatIterator it(STAT_EFFECTOR);
		while ((act2 = it.Next()))
		{
			if (actor != act2 && (act2->s->lotag) == 17)
				if ((sc->hitag - t[0]) == (sector[act2->s->sectnum].hitag) && sh == (act2->s->hitag))
					break;
		}

		if (act2 == nullptr) return;
		auto spr2 = act2->s;

		DukeSectIterator its(s->sectnum);
		while (auto act3 = its.Next())
		{
			auto spr3 = act3->s;
			if (spr3->statnum == STAT_PLAYER && act3->GetOwner())
			{
				int p = spr3->yvel;

				ps[p].pos.x += spr2->x - s->x;
				ps[p].pos.y += spr2->y - s->y;
				ps[p].pos.z = sector[spr2->sectnum].floorz - (sc->floorz - ps[p].pos.z);

				act3->floorz = sector[spr2->sectnum].floorz;
				act3->ceilingz = sector[spr2->sectnum].ceilingz;

				ps[p].bobposx = ps[p].oposx = ps[p].pos.x;
				ps[p].bobposy = ps[p].oposy = ps[p].pos.y;
				ps[p].oposz = ps[p].pos.z;

				ps[p].truefz = act3->floorz;
				ps[p].truecz = act3->ceilingz;
				ps[p].bobcounter = 0;

				changeactorsect(act3, spr2->sectnum);
				ps[p].cursectnum = spr2->sectnum;
			}
			else if (spr3->statnum != STAT_EFFECTOR)
			{
				spr3->x += spr2->x - s->x;
				spr3->y += spr2->y - s->y;
				spr3->z = sector[spr2->sectnum].floorz - (sc->floorz - spr3->z);

				spr3->backupz();

				changeactorsect(act3, spr2->sectnum);
				setsprite(act3, spr3->pos);

				act3->floorz = sector[spr2->sectnum].floorz;
				act3->ceilingz = sector[spr2->sectnum].ceilingz;

			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se18(DDukeActor *actor, bool morecheck)
{
	int* t = &actor->temp_data[0];
	auto sc = &sector[actor->s->sectnum];
	int st = actor->s->lotag;
	int sh = actor->s->hitag;

	if (t[0])
	{
		if (actor->s->pal)
		{
			if (actor->s->ang == 512)
			{
				sc->ceilingz -= sc->extra;
				if (sc->ceilingz <= t[1])
				{
					sc->ceilingz = t[1];
					deletesprite(actor);
					return;
				}
			}
			else
			{
				sc->floorz += sc->extra;
				if (morecheck)
				{
					DukeSectIterator it(actor->s->sectnum);
					while (auto a2 = it.Next())
					{
						if (a2->s->picnum == TILE_APLAYER && a2->GetOwner())
							if (ps[a2->PlayerIndex()].on_ground == 1) ps[a2->PlayerIndex()].pos.z += sc->extra;
						if (a2->s->zvel == 0 && a2->s->statnum != STAT_EFFECTOR && a2->s->statnum != STAT_PROJECTILE)
						{
							a2->s->z += sc->extra;
							a2->floorz = sc->floorz;
						}
					}
				}
				if (sc->floorz >= t[1])
				{
					sc->floorz = t[1];
					deletesprite(actor);
					return;
				}
			}
		}
		else
		{
			if (actor->s->ang == 512)
			{
				sc->ceilingz += sc->extra;
				if (sc->ceilingz >= actor->s->z)
				{
					sc->ceilingz = actor->s->z;
					deletesprite(actor);
					return;
				}
			}
			else
			{
				sc->floorz -= sc->extra;
				if (morecheck)
				{
					DukeSectIterator it(actor->s->sectnum);
					while (auto a2 = it.Next())
					{
						if (a2->s->picnum == TILE_APLAYER && a2->GetOwner())
							if (ps[a2->PlayerIndex()].on_ground == 1) ps[a2->PlayerIndex()].pos.z -= sc->extra;
						if (a2->s->zvel == 0 && a2->s->statnum != STAT_EFFECTOR && a2->s->statnum != STAT_PROJECTILE)
						{
							a2->s->z -= sc->extra;
							a2->floorz = sc->floorz;
						}
					}
				}
				if (sc->floorz <= actor->s->z)
				{
					sc->floorz = actor->s->z;
					deletesprite(actor);
					return;
				}
			}
		}

		t[2]++;
		if (t[2] >= actor->s->hitag)
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

void handle_se19(DDukeActor *actor, int BIGFORCE)
{
	int* t = &actor->temp_data[0];
	auto sc = &sector[actor->s->sectnum];
	int st = actor->s->lotag;
	int sh = actor->s->hitag;
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
			sc->ceilingz += actor->s->yvel;
		else
		{
			sc->ceilingz = sc->floorz;

			DukeStatIterator it(STAT_EFFECTOR);
			while (auto a2 = it.Next())
			{
				auto a2Owner = a2->GetOwner();
				if (a2->s->lotag == 0 && a2->s->hitag == sh && a2Owner)
				{
					q = a2Owner->s->sectnum; 
					sector[a2->s->sectnum].floorpal = sector[a2->s->sectnum].ceilingpal =	sector[q].floorpal;
					sector[a2->s->sectnum].floorshade = sector[a2->s->sectnum].ceilingshade = sector[q].floorshade;
					a2Owner->temp_data[0] = 2;
				}
			}
			deletesprite(actor);
			return;
		}
	}
	else //Not hit yet
	{
		auto hitter = fi.ifhitsectors(actor->s->sectnum);
		if (hitter)
		{
			FTA(8, &ps[myconnectindex]);

			DukeStatIterator it(STAT_EFFECTOR);
			while (auto ac = it.Next())
			{
				x = ac->s->lotag & 0x7fff;
				switch (x)
				{
				case 0:
					if (ac->s->hitag == sh && ac->GetOwner())
					{
						q = ac->s->sectnum;
						sector[q].floorshade = sector[q].ceilingshade =	ac->GetOwner()->s->shade;
						sector[q].floorpal = sector[q].ceilingpal =	ac->GetOwner()->s->pal;
					}
					break;

				case 1:
				case 12:
					//case 18:
				case 19:

					if (sh == ac->s->hitag)
						if (ac->temp_data[0] == 0)
						{
							ac->temp_data[0] = 1; //Shut them all on
							ac->SetOwner(actor);
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

void handle_se20(DDukeActor* actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;

	if (t[0] == 0) return;
	if (t[0] == 1) s->xvel = 8;
	else s->xvel = -8;

	if (s->xvel) //Moving
	{
		int x = MulScale(s->xvel, bcos(s->ang), 14);
		int l = MulScale(s->xvel, bsin(s->ang), 14);

		t[3] += s->xvel;

		s->x += x;
		s->y += l;

		if (t[3] <= 0 || (t[3] >> 6) >= (s->yvel >> 6))
		{
			s->x -= x;
			s->y -= l;
			t[0] = 0;
			callsound(s->sectnum, actor);
			return;
		}

		DukeSectIterator it(s->sectnum);
		while (auto a2 = it.Next())
		{
			if (a2->s->statnum != 3 && a2->s->zvel == 0)
			{
				a2->s->x += x;
				a2->s->y += l;
				setsprite(a2, a2->s->pos);
				if (sector[a2->s->sectnum].floorstat & 2)
					if (a2->s->statnum == 2)
						makeitfall(a2);
			}
		}

		dragpoint((short)t[1], wall[t[1]].x + x, wall[t[1]].y + l);
		dragpoint((short)t[2], wall[t[2]].x + x, wall[t[2]].y + l);

		for (int p = connecthead; p >= 0; p = connectpoint2[p])
			if (ps[p].cursectnum == s->sectnum && ps[p].on_ground)
			{
				ps[p].pos.x += x;
				ps[p].pos.y += l;

				ps[p].oposx = ps[p].pos.x;
				ps[p].oposy = ps[p].pos.y;

				setsprite(ps[p].GetActor(), ps[p].pos.x, ps[p].pos.y, ps[p].pos.z + gs.playerheight);
			}

		sc->addfloorxpan(-x / 8.f);
		sc->addfloorypan(-l / 8.f);

		sc->addceilingxpan(-x / 8.f);
		sc->addceilingypan(-l / 8.f);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se21(DDukeActor* actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
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
		s->zvel = Sgn(s->z - *lp) * (s->yvel << 4);
		t[0]++;
	}

	if (sc->extra == 0)
	{
		*lp += s->zvel;

		if (abs(*lp - s->z) < 1024)
		{
			*lp = s->z;
			deletesprite(actor);
		}
	}
	else sc->extra--;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se22(DDukeActor* actor)
{
	int* t = &actor->temp_data[0];
	auto sc = &sector[actor->s->sectnum];
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

void handle_se26(DDukeActor* actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int x, l;

	s->xvel = 32;
	l = MulScale(s->xvel, bcos(s->ang), 14);
	x = MulScale(s->xvel, bsin(s->ang), 14);

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

	DukeSectIterator it(s->sectnum);
	while (auto a2 = it.Next())
	{
		if (a2->s->statnum != 3 && a2->s->statnum != 10)
		{
			a2->s->x += l;
			a2->s->y += x;
			a2->s->z += s->zvel;
			setsprite(a2, a2->s->pos);
		}
	}

	for (int p = connecthead; p >= 0; p = connectpoint2[p])
		if (ps[p].GetActor()->s->sectnum == s->sectnum && ps[p].on_ground)
		{
			ps[p].fric.x += l << 5;
			ps[p].fric.y += x << 5;
			ps[p].pos.z += s->zvel;
		}

	ms(actor);
	setsprite(actor, s->pos);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se27(DDukeActor* actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];
	int st = s->lotag;
	int sh = s->hitag;
	int x, p;

	if (ud.recstat == 0) return;

	actor->tempang = s->ang;

	p = findplayer(actor, &x);
	if (ps[p].GetActor()->s->extra > 0 && myconnectindex == screenpeek)
	{
		if (t[0] < 0)
		{
			ud.cameraactor = actor;
			t[0]++;
		}
		else if (ud.recstat == 2 && ps[p].newOwner == nullptr)
		{
			if (cansee(s->x, s->y, s->z, s->sectnum, ps[p].pos.x, ps[p].pos.y, ps[p].pos.z, ps[p].cursectnum))
			{
				if (x < sh)
				{
					ud.cameraactor = actor;
					t[0] = 999;
					s->ang += getincangle(s->ang, getangle(ps[p].pos.x - s->x, ps[p].pos.y - s->y)) >> 3;
					s->yvel = 100 + ((s->z - ps[p].pos.z) / 257);

				}
				else if (t[0] == 999)
				{
					if (ud.cameraactor == actor)
						t[0] = 0;
					else t[0] = -10;
					ud.cameraactor = actor;

				}
			}
			else
			{
				s->ang = getangle(ps[p].pos.x - s->x, ps[p].pos.y - s->y);

				if (t[0] == 999)
				{
					if (ud.cameraactor == actor)
						t[0] = 0;
					else t[0] = -20;
					ud.cameraactor = actor;
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

void handle_se24(DDukeActor *actor, int16_t *list1, int16_t *list2, bool scroll, int TRIPBOMB, int LASERLINE, int CRANE, int shift)
{
	int* t = &actor->temp_data[0];

	auto testlist = [](int16_t* list, int val) { for (int i = 0; list[i] > 0; i++) if (list[i] == val) return true; return false; };

	if (t[4]) return;

	int x = MulScale(actor->s->yvel, bcos(actor->s->ang), 18);
	int l = MulScale(actor->s->yvel, bsin(actor->s->ang), 18);

	DukeSectIterator it(actor->s->sectnum);
	while (auto a2 = it.Next())
	{
		auto s2 = a2->s;
		if (s2->zvel >= 0)
		{
			switch (s2->statnum)
			{
			case 5:
				if (testlist(list1, s2->picnum))
				{
					s2->xrepeat = s2->yrepeat = 0;
					continue;
				}
				if (s2->picnum == LASERLINE)
				{
					continue;
				}

				//[[fallthrough]]
			case 6:
				if (s2->picnum == TRIPBOMB) break;
				//[[fallthrough]]
			case 1:
			case 0:
				if (testlist(list2, s2->picnum) ||
					wallswitchcheck(a2))
					break;

				if (!(s2->picnum >= CRANE && s2->picnum <= (CRANE + 3)))
				{
					if (s2->z > (a2->floorz - (16 << 8)))
					{
						s2->x += x >> shift;
						s2->y += l >> shift;

						setsprite(a2, s2->pos);

						if (sector[s2->sectnum].floorstat & 2)
							if (s2->statnum == 2)
								makeitfall(a2);
					}
				}
				break;
			}
		}
	}

	for (auto p = connecthead; p >= 0; p = connectpoint2[p])
	{
		if (ps[p].cursectnum == actor->s->sectnum && ps[p].on_ground)
		{
			if (abs(ps[p].pos.z - ps[p].truefz) < gs.playerheight + (9 << 8))
			{
				ps[p].fric.x += x << 3;
				ps[p].fric.y += l << 3;
			}
		}
	}
	if (scroll) sector[actor->s->sectnum].addfloorxpan(actor->s->yvel / 128.f);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se25(DDukeActor* actor, int t_index, int snd1, int snd2)
{
	int* t = &actor->temp_data[0];
	auto sec = &sector[actor->s->sectnum];

	if (sec->floorz <= sec->ceilingz)
		actor->s->shade = 0;
	else if (sec->ceilingz <= t[t_index])
		actor->s->shade = 1;

	if (actor->s->shade)
	{
		sec->ceilingz += actor->s->yvel << 4;
		if (sec->ceilingz > sec->floorz)
		{
			sec->ceilingz = sec->floorz;
			if (pistonsound && snd1 >= 0)
				S_PlayActorSound(snd1, actor);
		}
	}
	else
	{
		sec->ceilingz -= actor->s->yvel << 4;
		if (sec->ceilingz < t[t_index])
		{
			sec->ceilingz = t[t_index];
			if (pistonsound && snd2 >= 0)
				S_PlayActorSound(snd2, actor);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se32(DDukeActor *actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];

	if (t[0] == 1)
	{
		// Choose dir

		if (t[2] == 1) // Retract
		{
			if (s->ang != 1536)
			{
				if (abs(sc->ceilingz - s->z) < (s->yvel << 1))
				{
					sc->ceilingz = s->z;
					callsound(s->sectnum, actor);
					t[2] = 0;
					t[0] = 0;
				}
				else sc->ceilingz += Sgn(s->z - sc->ceilingz) * s->yvel;
			}
			else
			{
				if (abs(sc->ceilingz - t[1]) < (s->yvel << 1))
				{
					sc->ceilingz = t[1];
					callsound(s->sectnum, actor);
					t[2] = 0;
					t[0] = 0;
				}
				else sc->ceilingz += Sgn(t[1] - sc->ceilingz) * s->yvel;
			}
			return;
		}

		if ((s->ang & 2047) == 1536)
		{
			if (abs(sc->ceilingz - s->z) < (s->yvel << 1))
			{
				t[0] = 0;
				t[2] = !t[2];
				callsound(s->sectnum, actor);
				sc->ceilingz = s->z;
			}
			else sc->ceilingz += Sgn(s->z - sc->ceilingz) * s->yvel;
		}
		else
		{
			if (abs(sc->ceilingz - t[1]) < (s->yvel << 1))
			{
				t[0] = 0;
				t[2] = !t[2];
				callsound(s->sectnum, actor);
			}
			else sc->ceilingz -= Sgn(s->z - t[1]) * s->yvel;
		}
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se35(DDukeActor *actor, int SMALLSMOKE, int EXPLOSION2)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sc = &sector[s->sectnum];

	if (sc->ceilingz > s->z)
		for (int j = 0; j < 8; j++)
		{
			s->ang += krand() & 511;
			auto spawned = spawn(actor, SMALLSMOKE);
			spawned->s->xvel = 96 + (krand() & 127);
			ssp(spawned, CLIPMASK0);
			setsprite(spawned, spawned->s->pos);
			if (rnd(16))
				spawn(actor, EXPLOSION2);
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

void handle_se128(DDukeActor *actor)
{
	int* t = &actor->temp_data[0];
	auto sc = &sector[actor->s->sectnum];

	auto wal = &wall[t[2]];

//	if (wal->cstat | 32) // this has always been bugged, the condition can never be false.
	{
		wal->cstat &= (255 - 32);
		wal->cstat |= 16;
		if (wal->nextwall >= 0)
		{
			wall[wal->nextwall].cstat &= (255 - 32);
			wall[wal->nextwall].cstat |= 16;
		}
	}
//	else return;

	wal->overpicnum++;
	if (wal->nextwall >= 0)
		wall[wal->nextwall].overpicnum++;

	if (t[0] < t[1]) t[0]++;
	else
	{
		wal->cstat &= (128 + 32 + 8 + 4 + 2);
		if (wal->nextwall >= 0)
			wall[wal->nextwall].cstat &= (128 + 32 + 8 + 4 + 2);
		deletesprite(actor);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se130(DDukeActor *actor, int countmax, int EXPLOSION2)
{
	int* t = &actor->temp_data[0];
	auto sc = &sector[actor->s->sectnum];

	if (t[0] > countmax)
	{
		deletesprite(actor);
		return;
	}
	else t[0]++;

	int x = sc->floorz - sc->ceilingz;

	if (rnd(64))
	{
		auto k = spawn(actor, EXPLOSION2);
		k->s->xrepeat = k->s->yrepeat = 2 + (krand() & 7);
		k->s->z = sc->floorz - (krand() % x);
		k->s->ang += 256 - (krand() % 511);
		k->s->xvel = krand() & 127;
		ssp(k, CLIPMASK0);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se31(DDukeActor* actor, bool choosedir)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sec = &sector[s->sectnum];

	if (t[0] == 1)
	{
		// Choose dir

		if (choosedir && t[3] > 0)
		{
			t[3]--;
			return;
		}

		if (t[2] == 1) // Retract
		{
			if (s->ang != 1536)
			{
				if (abs(sec->floorz - s->z) < s->yvel)
				{
					sec->floorz = s->z;
					t[2] = 0;
					t[0] = 0;
					if (choosedir) t[3] = s->hitag;
					callsound(s->sectnum, actor);
				}
				else
				{
					int l = Sgn(s->z - sec->floorz) * s->yvel;
					sec->floorz += l;

					DukeSectIterator it(s->sectnum);
					while (auto a2 = it.Next())
					{
						if (a2->s->picnum == TILE_APLAYER && a2->GetOwner())
							if (ps[a2->PlayerIndex()].on_ground == 1)
								ps[a2->PlayerIndex()].pos.z += l;
						if (a2->s->zvel == 0 && a2->s->statnum != STAT_EFFECTOR && (!choosedir || a2->s->statnum != STAT_PROJECTILE))
						{
							a2->s->z += l;
							a2->floorz = sec->floorz;
						}
					}
				}
			}
			else
			{
				if (abs(sec->floorz - t[1]) < s->yvel)
				{
					sec->floorz = t[1];
					callsound(s->sectnum, actor);
					t[2] = 0;
					t[0] = 0;
					if (choosedir) t[3] = s->hitag;
				}
				else
				{
					int l = Sgn(t[1] - sec->floorz) * s->yvel;
					sec->floorz += l;

					DukeSectIterator it(s->sectnum);
					while (auto a2 = it.Next())
					{
						if (a2->s->picnum == TILE_APLAYER && a2->GetOwner())
							if (ps[a2->PlayerIndex()].on_ground == 1)
								ps[a2->PlayerIndex()].pos.z += l;
						if (a2->s->zvel == 0 && a2->s->statnum != STAT_EFFECTOR && (!choosedir || a2->s->statnum != STAT_PROJECTILE))
						{
							a2->s->z += l;
							a2->floorz = sec->floorz;
						}
					}
				}
			}
			return;
		}

		if ((s->ang & 2047) == 1536)
		{
			if (abs(s->z - sec->floorz) < s->yvel)
			{
				callsound(s->sectnum, actor);
				t[0] = 0;
				t[2] = 1;
				if (choosedir) t[3] = s->hitag;
			}
			else
			{
				int l = Sgn(s->z - sec->floorz) * s->yvel;
				sec->floorz += l;

				DukeSectIterator it(s->sectnum);
				while (auto a2 = it.Next())
				{
					if (a2->s->picnum == TILE_APLAYER && a2->GetOwner())
						if (ps[a2->PlayerIndex()].on_ground == 1)
							ps[a2->PlayerIndex()].pos.z += l;
					if (a2->s->zvel == 0 && a2->s->statnum != STAT_EFFECTOR && (!choosedir || a2->s->statnum != STAT_PROJECTILE))
					{
						a2->s->z += l;
						a2->floorz = sec->floorz;
					}
				}
			}
		}
		else
		{
			if (abs(sec->floorz - t[1]) < s->yvel)
			{
				t[0] = 0;
				callsound(s->sectnum, actor);
				t[2] = 1;
				t[3] = s->hitag;
			}
			else
			{
				int l = Sgn(s->z - t[1]) * s->yvel;
				sec->floorz -= l;

				DukeSectIterator it(s->sectnum);
				while (auto a2 = it.Next())
				{
					if (a2->s->picnum ==TILE_APLAYER && a2->GetOwner())
						if (ps[a2->PlayerIndex()].on_ground == 1)
							ps[a2->PlayerIndex()].pos.z -= l;
					if (a2->s->zvel == 0 && a2->s->statnum != STAT_EFFECTOR && (!choosedir || a2->s->statnum != STAT_PROJECTILE))
					{
						a2->s->z -= l;
						a2->floorz = sec->floorz;
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

void getglobalz(DDukeActor* actor)
{
	auto s = actor->s;
	int zr;
	Collision hz, lz;

	if( s->statnum == STAT_PLAYER || s->statnum == STAT_STANDABLE || s->statnum == STAT_ZOMBIEACTOR || s->statnum == STAT_ACTOR || s->statnum == STAT_PROJECTILE)
	{
		if(s->statnum == STAT_PROJECTILE)
			zr = 4;
		else zr = 127;

		auto cc = s->cstat2;
		s->cstat2 |= CSTAT2_SPRITE_NOFIND; // don't clip against self. getzrange cannot detect this because it only receives a coordinate.
		getzrange_ex(s->x, s->y, s->z - (FOURSLEIGHT), s->sectnum, &actor->ceilingz, hz, &actor->floorz, lz, zr, CLIPMASK0);
		s->cstat2 = cc;

		if( lz.type == kHitSprite && (lz.actor->s->cstat&48) == 0 )
		{
			if( badguy(lz.actor) && lz.actor->s->pal != 1)
			{
				if( s->statnum != STAT_PROJECTILE)
				{
					actor->aflags |= SFLAG_NOFLOORSHADOW; 
					//actor->dispicnum = -4; // No shadows on actors
					s->xvel = -256;
					ssp(actor, CLIPMASK0);
				}
			}
			else if(lz.actor->s->picnum == TILE_APLAYER && badguy(actor) )
			{
				actor->aflags |= SFLAG_NOFLOORSHADOW; 
				//actor->dispicnum = -4; // No shadows on actors
				s->xvel = -256;
				ssp(actor, CLIPMASK0);
			}
			else if(s->statnum == STAT_PROJECTILE && lz.actor->s->picnum == TILE_APLAYER && actor->GetOwner() == actor)
			{
				actor->ceilingz = sector[s->sectnum].ceilingz;
				actor->floorz	= sector[s->sectnum].floorz;
			}
		}
	}
	else
	{
		actor->ceilingz = sector[s->sectnum].ceilingz;
		actor->floorz	= sector[s->sectnum].floorz;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void makeitfall(DDukeActor* actor)
{
	auto s = actor->s;
	int c;

	if( fi.floorspace(s->sectnum) )
		c = 0;
	else
	{
		if( fi.ceilingspace(s->sectnum) || sector[s->sectnum].lotag == ST_2_UNDERWATER)
			c = gs.gravity/6;
		else c = gs.gravity;
	}
	
	if (isRRRA())
	{
		c = adjustfall(actor, c); // this accesses sprite indices and cannot be in shared code. Should be done better.
	}

	if ((s->statnum == STAT_ACTOR || s->statnum == STAT_PLAYER || s->statnum == STAT_ZOMBIEACTOR || s->statnum == STAT_STANDABLE))
	{
		Collision c;
		getzrange_ex(s->x, s->y, s->z - (FOURSLEIGHT), s->sectnum, &actor->ceilingz, c, &actor->floorz, c, 127, CLIPMASK0);
	}
	else
	{
		actor->ceilingz = sector[s->sectnum].ceilingz;
		actor->floorz	= sector[s->sectnum].floorz;
	}

	if( s->z < actor->floorz-(FOURSLEIGHT) )
	{
		if( sector[s->sectnum].lotag == 2 && s->zvel > 3122 )
			s->zvel = 3144;
		if(s->zvel < 6144)
			s->zvel += c;
		else s->zvel = 6144;
		s->z += s->zvel;
	}
	if( s->z >= actor->floorz-(FOURSLEIGHT) )
	{
		s->z = actor->floorz - FOURSLEIGHT;
		s->zvel = 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int dodge(DDukeActor* actor)
{
	auto s = actor->s;
	int bx, by, mx, my, bxvect, byvect, d;

	mx = s->x;
	my = s->y;

	DukeStatIterator it(STAT_PROJECTILE);
	while (auto ac = it.Next())
	{
		auto si = ac->s;
		if (ac->GetOwner() == ac || si->sectnum != s->sectnum)
			continue;

		bx = si->x - mx;
		by = si->y - my;
		bxvect = bcos(si->ang);
		byvect = bsin(si->ang);

		if (bcos(s->ang) * bx + bsin(s->ang) * by >= 0)
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

int furthestangle(DDukeActor *actor, int angs)
{
	auto s = actor->s;
	short j, hitsect, hitwall, furthest_angle, angincs;
	int hx, hy, hz, d, greatestd;
	DDukeActor* dd;

	greatestd = -(1 << 30);
	angincs = 2048 / angs;

	if (s->picnum != TILE_APLAYER)
		if ((actor->temp_data[0] & 63) > 2) return(s->ang + 1024);

	for (j = s->ang; j < (2048 + s->ang); j += angincs)
	{
		hitscan(s->x, s->y, s->z - (8 << 8), s->sectnum, bcos(j), bsin(j), 0, &hitsect, &hitwall, &dd, &hx, &hy, &hz, CLIPMASK1);

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

int furthestcanseepoint(DDukeActor *actor, DDukeActor* tosee, int* dax, int* day)
{
	auto s = actor->s;
	short j, hitsect, hitwall, angincs;
	int hx, hy, hz, d, da;//, d, cd, ca,tempx,tempy,cx,cy;
	DDukeActor* dd;

	if ((actor->temp_data[0] & 63)) return -1;

	if (ud.multimode < 2 && ud.player_skill < 3)
		angincs = 2048 / 2;
	else angincs = 2048 / (1 + (krand() & 1));

	auto ts = tosee->s;
	for (j = ts->ang; j < (2048 + ts->ang); j += (angincs - (krand() & 511)))
	{
		hitscan(ts->x, ts->y, ts->z - (16 << 8), ts->sectnum, bcos(j), bsin(j), 16384 - (krand() & 32767), 
			&hitsect, &hitwall, &dd, &hx, &hy, &hz, CLIPMASK1);

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

void alterang(int ang, DDukeActor* actor, int playernum)
{
	auto s = actor->s;
	short aang, angdif, goalang, j;
	int ticselapsed;
	int* t = actor->temp_data;

	auto moveptr = &ScriptCode[t[1]];

	ticselapsed = (t[0]) & 31;

	aang = s->ang;

	s->xvel += (*moveptr - s->xvel) / 5;
	if (s->zvel < 648) s->zvel += ((*(moveptr + 1) << 4) - s->zvel) / 5;

	if (isRRRA() && (ang & windang))
		s->ang = WindDir;
	else if (ang & seekplayer)
	{
		auto holoduke = !isRR()? ps[playernum].holoduke_on : nullptr;

		// NOTE: looks like 'Owner' is set to target sprite ID...

		if (holoduke && cansee(holoduke->s->x, holoduke->s->y, holoduke->s->z, holoduke->s->sectnum, s->x, s->y, s->z, s->sectnum))
			actor->SetOwner(holoduke);
		else actor->SetOwner(ps[playernum].GetActor());

		auto Owner = actor->GetOwner();
		if (Owner->s->picnum == TILE_APLAYER)
			goalang = getangle(actor->lastvx - s->x, actor->lastvy - s->y);
		else
			goalang = getangle(Owner->s->x - s->x, Owner->s->y - s->y);

		if (s->xvel && s->picnum != TILE_DRONE)
		{
			angdif = getincangle(aang, goalang);

			if (ticselapsed < 2)
			{
				if (abs(angdif) < 256)
				{
					j = 128 - (krand() & 256);
					s->ang += j;
					if (hits(actor) < 844)
						s->ang -= j;
				}
			}
			else if (ticselapsed > 18 && ticselapsed < 26) // choose
			{
				if (abs(angdif >> 2) < 128) s->ang = goalang;
				else s->ang += angdif >> 2;
			}
		}
		else s->ang = goalang;
	}

	if (ticselapsed < 1)
	{
		j = 2;
		if (ang & furthestdir)
		{
			goalang = furthestangle(actor, j);
			s->ang = goalang;
			actor->SetOwner(ps[playernum].GetActor());
		}

		if (ang & fleeenemy)
		{
			goalang = furthestangle(actor, j);
			s->ang = goalang; // += angdif; //  = getincangle(aang,goalang)>>1;
		}
	}
}

//---------------------------------------------------------------------------
//
// the indirections here are to keep this core function free of game references
//
//---------------------------------------------------------------------------

void fall_common(DDukeActor *actor, int playernum, int JIBS6, int DRONE, int BLOODPOOL, int SHOTSPARK1, int squished, int thud, int(*fallspecial)(DDukeActor*, int))
{
	auto s = actor->s;
	s->xoffset = 0;
	s->yoffset = 0;
	//			  if(!gotz)
	{
		int c;

		int sphit = fallspecial? fallspecial(actor, playernum) : 0;
		if (fi.floorspace(s->sectnum))
			c = 0;
		else
		{
			if (fi.ceilingspace(s->sectnum) || sector[s->sectnum].lotag == 2)
				c = gs.gravity / 6;
			else c = gs.gravity;
		}

		if (actor->cgg <= 0 || (sector[s->sectnum].floorstat & 2))
		{
			getglobalz(actor);
			actor->cgg = 6;
		}
		else actor->cgg--;

		if (s->z < (actor->floorz - FOURSLEIGHT))
		{
			s->zvel += c;
			s->z += s->zvel;

			if (s->zvel > 6144) s->zvel = 6144;
		}
		else
		{
			s->z = actor->floorz - FOURSLEIGHT;

			if (badguy(actor) || (s->picnum == TILE_APLAYER && actor->GetOwner()))
			{

				if (s->zvel > 3084 && s->extra <= 1)
				{
					if (s->pal != 1 && s->picnum != DRONE)
					{
						if (s->picnum == TILE_APLAYER && s->extra > 0)
							goto SKIPJIBS;
						if (sphit)
						{
							fi.guts(actor, JIBS6, 5, playernum);
							S_PlayActorSound(squished, actor);
						}
						else
						{
							fi.guts(actor, JIBS6, 15, playernum);
							S_PlayActorSound(squished, actor);
							spawn(actor, BLOODPOOL);
						}
					}

				SKIPJIBS:

					actor->picnum = SHOTSPARK1;
					actor->extra = 1;
					s->zvel = 0;
				}
				else if (s->zvel > 2048 && sector[s->sectnum].lotag != 1)
				{

					short j = s->sectnum;
					int x = s->x, y = s->y, z = s->z;
					pushmove(&x, &y, &z, &j, 128, (4 << 8), (4 << 8), CLIPMASK0);
					s->x = x; s->y = y; s->z = z;
					if (j != s->sectnum && j >= 0 && j < MAXSECTORS)
						changeactorsect(actor, j);

					S_PlayActorSound(thud, actor);
				}
			}
			if (sector[s->sectnum].lotag == 1)
				s->z += gs.actorinfo[s->picnum].falladjustz;
			else s->zvel = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DDukeActor *LocateTheLocator(int n, int sectnum)
{
	DukeStatIterator it(STAT_LOCATOR);
	while (auto ac = it.Next())
	{
		if ((sectnum == -1 || sectnum == ac->s->sectnum) && n == ac->s->lotag)
			return ac;
	}
	return nullptr;
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
		DukeStatIterator it(statNum);
		while (auto ac = it.Next())
		{
			ac->s->backuploc();
		}
	}
}



END_DUKE_NS
