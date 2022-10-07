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

double adjustfall(DDukeActor* actor, double c);


//---------------------------------------------------------------------------
//
// this was once a macro
//
//---------------------------------------------------------------------------

void RANDOMSCRAP(DDukeActor* origin)
{
	int r1 = krand(), r2 = krand(), r3 = krand(), r4 = krand();
	DVector3 offset;
	offset.X = krandf(16) - 8;
	offset.Y = krandf(16) - 8;
	offset.Z = krandf(16) - 8;

	double v = isRR() ? 0.125 : 0.375;

	auto a = randomAngle();
	auto vel = krandf(4) + 4;
	auto zvel = -krandf(8) - 2;

	CreateActor(origin->sector(), origin->spr.pos + offset, TILE_SCRAP6 + (r4 & 15), -8, DVector2(v, v), a, vel, zvel, origin, 5);
}

//---------------------------------------------------------------------------
//
// wrapper to ensure that if a sound actor is killed, the sound is stopped as well.
//
//---------------------------------------------------------------------------

void deletesprite(DDukeActor *const actor)
{
	if (actor->spr.picnum == MUSICANDSFX && actor->temp_data[0] == 1)
		S_StopSound(actor->spr.lotag, actor);

	actor->Destroy();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addammo(int weapon, player_struct* player, int amount)
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

void checkavailinven(player_struct* player)
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

void checkavailweapon(player_struct* player)
{
	int i, snum;
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
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike(player->curr_weapon, snum), player->GetActor(), snum);
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
	ps->restorexyz();
	ps->angle.restore();
	updatesector(ps->pos, &ps->cursector);

	DukeStatIterator it(STAT_ACTOR);
	while (auto k = it.Next())
	{
		if (actorflag(k, SFLAG2_CAMERA))
			k->spr.yint = 0;
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

	return movesprite_ex(actor, DVector3(actor->spr.angle.ToVector() * actor->vel.X, actor->vel.Z), cliptype, c) == kHitNone;
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
			// todo: Make list size a CVAR.
			deletesprite(spriteq[spriteqloc]);
		}
		spriteq[spriteqloc] = actor;
		spriteqloc = (spriteqloc + 1) % spriteqamount;
	}
	else actor->spr.scale = DVector2(0, 0);
}

//---------------------------------------------------------------------------
//
// consolidation of several nearly identical functions
//
//---------------------------------------------------------------------------

void lotsofstuff(DDukeActor* actor, int n, int spawntype)
{
	for (int i = n; i > 0; i--)
	{
		DAngle r1 = randomAngle();
		double r2 = zrand(47);
		auto j = CreateActor(actor->sector(), actor->spr.pos.plusZ(-r2), spawntype, -32, DVector2(0.125, 0.125), r1, 0., 0., actor, 5);
		if (j) j->spr.cstat = randomFlip();
	}
}

//---------------------------------------------------------------------------
//
// movesector - used by sector effectors
//
//---------------------------------------------------------------------------

void movesector(DDukeActor* const actor, int msindex, DAngle rotation)
{
	//T1,T2 and T3 are used for all the sector moving stuff!!!
	actor->spr.pos.X += actor->vel.X * actor->spr.angle.Cos();
	actor->spr.pos.Y += actor->vel.X * actor->spr.angle.Sin();

	for(auto& wal : wallsofsector(actor->sector()))
	{
		auto t = rotatepoint({ 0, 0 }, mspos[msindex], rotation);

		dragpoint(&wal, actor->spr.pos.XY() + t);
		msindex++;
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
		Cycler* c = &cyclers[q];
		auto sect = c->sector;

		int shade = c->shade2;
		int j = shade + int(BobVal(c->lotag) * 16);
		int cshade = c->shade1;

		if (j < cshade) j = cshade;
		else if (j > shade)  j = shade;

		c->lotag += sect->extra;
		if (c->state)
		{
			for (auto& wal : wallsofsector(sect))
			{
				if (wal.hitag != 1)
				{
					wal.shade = j;

					if ((wal.cstat & CSTAT_WALL_BOTTOM_SWAP) && wal.twoSided())
						wal.nextWall()->shade = j;

				}
			}
			sect->floorshade = sect->ceilingshade = j;
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

		if ((!isRR() && ps[p].on_crane != nullptr) || !ps[p].insector() || ps[p].cursector->lotag != 1 || ps->GetActor()->spr.extra <= 0)
		{
			ps[p].dummyplayersprite = nullptr;
			deletesprite(act);
			continue;
		}
		else
		{
			if (ps[p].on_ground && ps[p].on_warping_sector == 1 && ps[p].cursector->lotag == 1)
			{
				act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
				act->spr.pos.Z = act->sector()->ceilingz + 27;
				act->spr.angle = ps[p].angle.ang;
				if (act->temp_data[0] == 8)
					act->temp_data[0] = 0;
				else act->temp_data[0]++;
			}
			else
			{
				if (act->sector()->lotag != 2) act->spr.pos.Z = act->sector()->floorz;
				act->spr.cstat = CSTAT_SPRITE_INVISIBLE;
			}
		}

		act->spr.pos.X += (ps[p].pos.X - ps[p].opos.X);
		act->spr.pos.Y += (ps[p].pos.Y - ps[p].opos.Y);
		SetActor(act, act->spr.pos);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveplayers(void)
{
	double other;

	DukeStatIterator iti(STAT_PLAYER);
	while (auto act = iti.Next())
	{
		int pn = act->PlayerIndex();
		auto p = &ps[pn];

		if (act->GetOwner())
		{
			if (p->newOwner != nullptr) //Looking thru the camera
			{
				act->spr.pos = p->opos.plusZ(gs.playerheight);
				act->backupz();
				act->spr.angle = p->angle.oang;
				SetActor(act, act->spr.pos);
			}
			else
			{
				if (ud.multimode > 1)
					otherp = findotherplayer(pn, &other);
				else
				{
					otherp = pn;
					other = 0;
				}

				execute(act, pn, other);

				if (ud.multimode > 1)
				{
					auto psp = ps[otherp].GetActor();
					if (psp->spr.extra > 0)
					{
						if (act->spr.scale.Y > 0.5 && psp->spr.scale.Y < 0.5)
						{
							if (other < 1400/16. && p->knee_incs == 0)
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
					act->spr.extra = gs.max_player_health;
					act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
					if (!isWW2GI() && !isRR())
						p->jetpack_amount = 1599;
				}

				if (p->actorsqu != nullptr)
				{
					p->angle.addadjustment(deltaangle(p->angle.ang, (p->actorsqu->spr.pos - p->pos).Angle()) * 0.25);
				}

				if (act->spr.extra > 0)
				{
					// currently alive...

					act->SetHitOwner(act);

					if (ud.god == 0)
						if (fi.ceilingspace(act->sector()) || fi.floorspace(act->sector()))
							quickkill(p);
				}
				else
				{
					p->pos = act->spr.pos.plusZ(-28);
					p->newOwner = nullptr;

					if (p->wackedbyactor != nullptr && p->wackedbyactor->spr.statnum < MAXSTATUS)
					{
						p->angle.addadjustment(deltaangle(p->angle.ang, (p->wackedbyactor->spr.pos - p->pos).Angle()) * 0.5);
					}
				}
				act->spr.angle = p->angle.ang;
			}
		}
		else
		{
			if (p->holoduke_on == nullptr)
			{
				deletesprite(act);
				continue;
			}

			act->spr.cstat = 0;

			if (act->spr.scale.X < 0.65625)
			{
				act->spr.scale.X += (0.0625);
				act->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT;
			}
			else act->spr.scale.X = (0.65625);

			if (act->spr.scale.Y < 0.5625)
				act->spr.scale.Y += (4);
			else
			{
				act->spr.scale.Y = (0.5625);
				if (act->sector()->lotag != ST_2_UNDERWATER)
					makeitfall(act);
				if (act->vel.Z == 0 && act->sector()->lotag == ST_1_ABOVE_WATER)
					act->spr.pos.Z += 32;
			}

			if (act->spr.extra < 8)
			{
				act->vel.X = 8;
				act->spr.angle = p->angle.ang;
				act->spr.extra++;
				ssp(act, CLIPMASK0);
			}
			else
			{
				act->spr.angle = DAngle360 - minAngle - p->angle.ang;
				SetActor(act, act->spr.pos);
			}
		}

		if (act->insector())
		{
			if (act->sector()->ceilingstat & CSTAT_SECTOR_SKY)
				act->spr.shade += (act->sector()->ceilingshade - act->spr.shade) >> 1;
			else
				act->spr.shade += (act->sector()->floorshade - act->spr.shade) >> 1;
		}
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

	DukeStatIterator iti(STAT_FX);
	while (auto act = iti.Next())
	{
		switch (act->spr.picnum)
		{
		case RESPAWN:
			if (act->spr.extra == 66)
			{
				auto j = spawn(act, act->spr.hitag);
				if (isRRRA() && j)
				{
					respawn_rrra(act, j);
				}
				else
				{
					deletesprite(act);
				}
			}
			else if (act->spr.extra > (66 - 13))
				act->spr.extra++;
			break;

		case MUSICANDSFX:
			{
			double maxdist = act->spr.hitag * maptoworld;

			if (act->temp_data[1] != (int)SoundEnabled())
			{
				act->temp_data[1] = SoundEnabled();
				act->temp_data[0] = 0;
			}

			if (act->spr.lotag >= 1000 && act->spr.lotag < 2000)
			{
				double dist = (ps[screenpeek].GetActor()->spr.pos.XY() - act->spr.pos.XY()).Length();
				if (dist < maxdist && act->temp_data[0] == 0)
				{
					FX_SetReverb(act->spr.lotag - 1100);
					act->temp_data[0] = 1;
				}
				if (dist >= maxdist && act->temp_data[0] == 1)
				{
					FX_SetReverb(0);
					FX_SetReverbDelay(0);
					act->temp_data[0] = 0;
				}
			}
			else if (act->spr.lotag < 999 && (unsigned)act->sector()->lotag < ST_9_SLIDING_ST_DOOR && snd_ambience && act->sector()->floorz != act->sector()->ceilingz)
			{
				int flags = S_GetUserFlags(act->spr.lotag);
				if (flags & SF_MSFX)
				{
					double distance = (ps[screenpeek].GetActor()->spr.pos - act->spr.pos).Length();

					if (distance < maxdist && act->temp_data[0] == 0)
					{
						// Start playing an ambience sound.
						S_PlayActorSound(act->spr.lotag, act, CHAN_AUTO, CHANF_LOOP);
						act->temp_data[0] = 1;  // AMBIENT_SFX_PLAYING
					}
					else if (distance >= maxdist && act->temp_data[0] == 1)
					{
						// Stop playing ambience sound because we're out of its range.
						S_StopSound(act->spr.lotag, act);
					}
				}

				if ((flags & (SF_GLOBAL | SF_DTAG)) == SF_GLOBAL)
				{
					if (act->temp_data[4] > 0) act->temp_data[4]--;
					else for (p = connecthead; p >= 0; p = connectpoint2[p])
						if (p == myconnectindex && ps[p].cursector == act->sector())
						{
							S_PlaySound(act->spr.lotag + (unsigned)global_random % (act->spr.hitag + 1));
							act->temp_data[4] = 26 * 40 + (global_random % (26 * 40));
						}
				}
			}
			break;
			}
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
	const double CRANE_STEP = 16.;
	auto sectp = actor->sector();
	double xx;
	auto& cpt = cranes[actor->temp_data[4]];

	//actor->temp_data[0] = state
	//actor->temp_data[1] = checking sector number

	if(actor->vel.X != 0) getglobalz(actor);

	if (actor->temp_data[0] == 0) //Waiting to check the sector
	{
		DukeSectIterator it(actor->temp_sect);
		while (auto a2 = it.Next())
		{
			switch (a2->spr.statnum)
			{
			case STAT_ACTOR:
			case STAT_ZOMBIEACTOR:
			case STAT_STANDABLE:
			case STAT_PLAYER:
				actor->spr.angle = (cpt.pole - actor->spr.pos.XY()).Angle();
				SetActor(a2, DVector3( cpt.pole.X, cpt.pole.Y, a2->spr.pos.Z ));
				actor->temp_data[0]++;
				return;
			}
		}
	}

	else if (actor->temp_data[0] == 1)
	{
		if (actor->vel.X < 11.5)
		{
			actor->spr.picnum = crane + 1;
			actor->vel.X += 0.5;
		}
		//IFMOVING;	// JBF 20040825: see my rant above about this
		ssp(actor, CLIPMASK0);
		if (actor->sector() == actor->temp_sect)
			actor->temp_data[0]++;
	}
	else if (actor->temp_data[0] == 2 || actor->temp_data[0] == 7)
	{
		actor->spr.pos.Z += 6;

		if (actor->temp_data[0] == 2)
		{
			if ((sectp->floorz - actor->spr.pos.Z) < 64)
				if (actor->spr.picnum > crane) actor->spr.picnum--;

			if ((sectp->floorz - actor->spr.pos.Z) < 20)
				actor->temp_data[0]++;
		}
		if (actor->temp_data[0] == 7)
		{
			if ((sectp->floorz - actor->spr.pos.Z) < 64)
			{
				if (actor->spr.picnum > crane) actor->spr.picnum--;
				else
				{
					if (actor->IsActiveCrane())
					{
						int p = findplayer(actor, &xx);
						S_PlayActorSound(isRR() ? 390 : DUKE_GRUNT, ps[p].GetActor());
						if (ps[p].on_crane == actor)
							ps[p].on_crane = nullptr;
					}
					actor->temp_data[0]++;
					actor->SetActiveCrane(false);
				}
			}
		}
	}
	else if (actor->temp_data[0] == 3)
	{
		actor->spr.picnum++;
		if (actor->spr.picnum == (crane + 2))
		{
			int p = checkcursectnums(actor->temp_sect);
			if (p >= 0 && ps[p].on_ground)
			{
				actor->SetActiveCrane(true);
				ps[p].on_crane = actor;
				S_PlayActorSound(isRR() ? 390 : DUKE_GRUNT, ps[p].GetActor());
				ps[p].angle.settarget(actor->spr.angle + DAngle180);
			}
			else
			{
				DukeSectIterator it(actor->temp_sect);
				while (auto a2 = it.Next())
				{
					switch (a2->spr.statnum)
					{
					case 1:
					case 6:
						actor->SetOwner(a2);
						break;
					}
				}
			}

			actor->temp_data[0]++;//Grabbed the sprite
			actor->temp_data[2] = 0;
			return;
		}
	}
	else if (actor->temp_data[0] == 4) //Delay before going up
	{
		actor->temp_data[2]++;
		if (actor->temp_data[2] > 10)
			actor->temp_data[0]++;
	}
	else if (actor->temp_data[0] == 5 || actor->temp_data[0] == 8)
	{
		if (actor->temp_data[0] == 8 && actor->spr.picnum < (crane + 2))
			if ((sectp->floorz - actor->spr.pos.Z) > 32)
				actor->spr.picnum++;

		if (actor->spr.pos.Z < cpt.pos.Z)
		{
			actor->temp_data[0]++;
			actor->vel.X = 0;
		}
		else
			actor->spr.pos.Z -= 6;
	}
	else if (actor->temp_data[0] == 6)
	{
		if (actor->vel.X < 12)
			actor->vel.X += 0.5;
		actor->spr.angle = (cpt.pos.XY() - actor->spr.pos.XY()).Angle();
		ssp(actor, CLIPMASK0);
		if (((actor->spr.pos.X - cpt.pos.X) * (actor->spr.pos.X - cpt.pos.X) + (actor->spr.pos.Y - cpt.pos.Y) * (actor->spr.pos.Y - cpt.pos.Y)) < (8 * 8))
			actor->temp_data[0]++;
	}

	else if (actor->temp_data[0] == 9)
		actor->temp_data[0] = 0;

	if (cpt.poleactor)
		SetActor(cpt.poleactor, actor->spr.pos.plusZ(-34));

	auto Owner = actor->GetOwner();
	if (Owner != nullptr || actor->IsActiveCrane())
	{
		int p = findplayer(actor, &xx);

		int j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{
			if (actor->IsActiveCrane())
				if (ps[p].on_crane == actor)
					ps[p].on_crane = nullptr;
			actor->SetActiveCrane(false);
			actor->spr.picnum = crane;
			return;
		}

		if (Owner != nullptr)
		{
			SetActor(Owner, actor->spr.pos);

			Owner->opos = actor->spr.pos;

			actor->vel.Z = 0;
		}
		else if (actor->IsActiveCrane())
		{
			auto ang = ps[p].angle.ang;
			ps[p].backupxyz();
			ps[p].pos.XY() = actor->spr.pos.XY() - CRANE_STEP * ang.ToVector();
			ps[p].pos.Z = actor->spr.pos.Z + 2;
			SetActor(ps[p].GetActor(), ps[p].pos);
			ps[p].setCursector(ps[p].GetActor()->sector());
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
	if (actor->temp_data[0] > 0)
	{
		if (actor->temp_data[0] < 20)
		{
			actor->temp_data[0]++;

			actor->spr.picnum++;

			if (actor->spr.picnum == fountain + 3)
				actor->spr.picnum = fountain + 1;
		}
		else
		{
			double x;
			findplayer(actor, &x);

			if (x > 32)
			{
				actor->temp_data[0] = 0;
				actor->spr.picnum = fountain;
			}
			else actor->temp_data[0] = 1;
		}
	}
}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveflammable(DDukeActor* actor, int pool)
{
	double scale;
	if (actor->temp_data[0] == 1)
	{
		actor->temp_data[1]++;
		if ((actor->temp_data[1] & 3) > 0) return;

		if (actorflag(actor, SFLAG_FLAMMABLEPOOLEFFECT) && actor->temp_data[1] == 32)
		{
			actor->spr.cstat = 0;
			auto spawned = spawn(actor, pool);
			if (spawned) 
			{
				spawned->spr.pal = 2;
				spawned->spr.shade = 127;
			}
		}
		else
		{
			if (actor->spr.shade < 64) actor->spr.shade++;
			else
			{
				deletesprite(actor);
				return;
			}
		}

		scale = actor->spr.scale.X - (krand() & 7) * REPEAT_SCALE;
		if (scale < 0.15625)
		{
			deletesprite(actor);
			return;
		}

		actor->spr.scale.X = (scale);

		scale = actor->spr.scale.Y - (krand() & 7) * REPEAT_SCALE;
		if (scale < 0.0625)
		{
			deletesprite(actor);
			return;
		}
		actor->spr.scale.Y = (scale);
	}
	if (actorflag(actor, SFLAG_FALLINGFLAMMABLE))
	{
		makeitfall(actor);
		actor->ceilingz = actor->sector()->ceilingz;
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void detonate(DDukeActor *actor, int explosion)
{
	earthquaketime = 16;

	DukeStatIterator itj(STAT_EFFECTOR);
	while (auto effector = itj.Next())
	{
		if (actor->spr.hitag == effector->spr.hitag)
		{
			if (effector->spr.lotag == SE_13_EXPLOSIVE)
			{
				if (effector->temp_data[2] == 0)
					effector->temp_data[2] = 1;
			}
			else if (effector->spr.lotag == SE_8_UP_OPEN_DOOR_LIGHTS)
				effector->temp_data[4] = 1;
			else if (effector->spr.lotag == SE_18_INCREMENTAL_SECTOR_RISE_FALL)
			{
				if (effector->temp_data[0] == 0)
					effector->temp_data[0] = 1;
			}
			else if (effector->spr.lotag == SE_21_DROP_FLOOR)
				effector->temp_data[0] = 1;
		}
	}

	actor->spr.pos.Z -= 32;

	if ((actor->temp_data[3] == 1 && actor->spr.scale.X != 0) || actor->spr.lotag == -99)
	{
		int x = actor->spr.extra;
		spawn(actor, explosion);
		fi.hitradius(actor, gs.seenineblastradius, x >> 2, x - (x >> 1), x - (x >> 2), x);
		S_PlayActorSound(PIPEBOMB_EXPLODE, actor);
	}

	if (actor->spr.scale.X != 0)
		for (int x = 0; x < 8; x++) RANDOMSCRAP(actor);

	deletesprite(actor);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movemasterswitch(DDukeActor *actor)
{
	if (actor->spr.yint == 1)
	{
		actor->spr.hitag--;
		if (actor->spr.hitag <= 0)
		{
			operatesectors(actor->sector(), actor);

			DukeSectIterator it(actor->sector());
			while (auto effector = it.Next())
			{
				if (effector->spr.statnum == STAT_EFFECTOR)
				{
					switch (effector->spr.lotag)
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
				else if (effector->spr.statnum == STAT_STANDABLE)
				{
					if (actorflag(effector, SFLAG2_BRIGHTEXPLODE)) // _SEENINE_ and _OOZFILTER_
					{
						effector->spr.shade = -31;
					}
				}
			}
			// we cannot delete this because it may be used as a sound source.
			// This originally depended on undefined behavior as the deleted sprite was still used for the sound
			// with no checking if it got reused in the mean time.
			actor->spr.picnum = 0;	// give it a picnum without any behavior attached, just in case
			actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
			actor->spr.cstat2 |= CSTAT2_SPRITE_NOFIND;
			ChangeActorStat(actor, STAT_REMOVED);
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
	if (actor->vel.X == 0) actor->vel.X = 1 / 16.;
	if (ssp(actor, CLIPMASK0))
	{
		makeitfall(actor);
		if (krand() & 1) actor->vel.Z -= 1;
		if (abs(actor->vel.X) < 3)
			actor->vel.X += (krand() & 3) / 16.;
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
	if (actor->temp_data[1])
	{
		actor->temp_data[1]--;
		if (actor->temp_data[1] == 0)
			actor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
	}
	else
	{
		makeitfall(actor);
		ssp(actor, CLIPMASK0);
		if(actor->vel.X > 0) actor->vel.X -= 1/8.;

		if (actor->vel.Z == 0)
		{
			actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;

			if (actor->spr.pal != 2 && (isRR() || actor->spr.hitag == 0))
				S_PlayActorSound(SOMETHING_DRIPPING, actor);

			auto Owner = actor->GetOwner();
			if (!Owner || Owner->spr.picnum != drip)
			{
				deletesprite(actor);
			}
			else
			{
				actor->spr.pos.Z = actor->temp_pos.Z;
				actor->backupz();
				actor->temp_data[1] = 48 + (krand() & 31);
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
	auto sectp = actor->sector();
	double j = abs(sectp->ceilingz - sectp->floorz) / 128.;
	actor->spr.scale = DVector2(0.25, 0.0625 + j);
	actor->spr.pos.Z = sectp->floorz;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movetouchplate(DDukeActor* actor, int plate)
{
	auto sectp = actor->sector();
	int p;

	if (actor->temp_data[1] == 1 && actor->spr.hitag >= 0) //Move the sector floor
	{
		double X = sectp->floorz;
		double add = sectp->extra * zmaptoworld;

		if (actor->temp_data[3] == 1)
		{
			if (X >= actor->temp_pos.Z)
			{
				sectp->setfloorz(X);
				actor->temp_data[1] = 0;
			}
			else
			{
				sectp->addfloorz(add);
				p = checkcursectnums(actor->sector());
				if (p >= 0) ps[p].pos.Z += add;
			}
		}
		else
		{
			if (X <= actor->spr.pos.Z)
			{
				sectp->setfloorz(actor->spr.pos.Z);
				actor->temp_data[1] = 0;
			}
			else
			{
				sectp->floorz -= add;
				p = checkcursectnums(actor->sector());
				if (p >= 0)
					ps[p].pos.Z -= add;
			}
		}
		return;
	}

	if (actor->temp_data[5] == 1) return;

	p = checkcursectnums(actor->sector());
	if (p >= 0 && (ps[p].on_ground || actor->spr.intangle == 512))
	{
		if (actor->temp_data[0] == 0 && !check_activator_motion(actor->spr.lotag))
		{
			actor->temp_data[0] = 1;
			actor->temp_data[1] = 1;
			actor->temp_data[3] = !actor->temp_data[3];
			operatemasterswitches(actor->spr.lotag);
			operateactivators(actor->spr.lotag, p);
			if (actor->spr.hitag > 0)
			{
				actor->spr.hitag--;
				if (actor->spr.hitag == 0) actor->temp_data[5] = 1;
			}
		}
	}
	else actor->temp_data[0] = 0;

	if (actor->temp_data[1] == 1)
	{
		DukeStatIterator it(STAT_STANDABLE);
		while (auto act2 = it.Next())
		{
			if (act2 != actor && act2->spr.picnum == plate && act2->spr.lotag == actor->spr.lotag)
			{
				act2->temp_data[1] = 1;
				act2->temp_data[3] = actor->temp_data[3];
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
	int j;
	if (actor->spr.shade != -32 && actor->spr.shade != -33)
	{
		if (actor->spr.scale.X != 0)
			j = (fi.ifhitbyweapon(actor) >= 0);
		else
			j = 0;

		if (j || actor->spr.shade == -31)
		{
			if (j) actor->spr.lotag = 0;

			actor->temp_data[3] = 1;

			DukeStatIterator it(STAT_STANDABLE);
			while (auto act2 = it.Next())
			{
				if (actor->spr.hitag == act2->spr.hitag && actorflag(act2, SFLAG2_BRIGHTEXPLODE))
					act2->spr.shade = -32;
			}
		}
	}
	else
	{
		if (actor->spr.shade == -32)
		{
			if (actor->spr.lotag > 0)
			{
				actor->spr.lotag -= 3;
				if (actor->spr.lotag <= 0) actor->spr.lotag = -99;
			}
			else
				actor->spr.shade = -33;
		}
		else
		{
			if (actor->spr.scale.X > 0)
			{
				actor->temp_data[2]++;
				if (actor->temp_data[2] == 3)
				{
					if (actor->spr.picnum == ooz)
					{
						actor->temp_data[2] = 0;
						detonate(actor, explosion);
						return;
					}
					if (actor->spr.picnum != (seeninedead + 1))
					{
						actor->temp_data[2] = 0;

						if (actor->spr.picnum == seeninedead) actor->spr.picnum++;
						else if (actor->spr.picnum == seenine)
							actor->spr.picnum = seeninedead;
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

		if (actor->spr.lotag) spawn(actor, actor->spr.lotag);
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
	DVector3 vect(actor->spr.angle.ToVector() * actor->vel.X, actor->vel.Z);

	auto sectp = actor->sector();

	DAngle daang = sectp->firstWall()->delta().Angle();

	double k;
	if (actor->spr.pos.Z < (actor->floorz + actor->ceilingz) * 0.5)
		k = sectp->ceilingheinum / 16.;
	else
		k = sectp->floorheinum / 16.;

	DVector3 davec(daang.Sin() * k, -daang.Cos() * k, 16);

	double dot = vect.dot(davec);
	double l = davec.LengthSquared();

	const double scale = 1;	// still need to figure out.
	if ((abs(dot) * scale) < l)
	{
		k = k * l / 8.; // Guessed by '<< (17-14)'
		vect -= davec * k;
	}

	actor->vel.Z = vect.Z;
	actor->vel.X = vect.XY().Length();
	actor->spr.angle = vect.Angle();
}

//---------------------------------------------------------------------------
//
// taken out of moveweapon
//
//---------------------------------------------------------------------------

void movetongue(DDukeActor *actor, int tongue, int jaw)
{
	actor->temp_data[0] = int(BobVal(actor->temp_data[1]) * 32);
	actor->temp_data[1] += 32;
	if (actor->temp_data[1] > 2047)
	{
		deletesprite(actor);
		return;
	}

	auto Owner = actor->GetOwner();
	if (!Owner) return;

	if (Owner->spr.statnum == MAXSTATUS)
		if (badguy(Owner) == 0)
		{
			deletesprite(actor);
			return;
		}

	actor->spr.angle = Owner->spr.angle;
	actor->spr.pos = Owner->spr.pos.plusZ(Owner->isPlayer() ? -34 : 0);

	for (int k = 0; k < actor->temp_data[0]; k++)
	{
		auto pos = actor->spr.pos + actor->spr.angle.ToVector() * 2 * k;
		pos.Z += k * Sgn(actor->vel.Z) * abs(actor->vel.Z / 12);

		auto q = CreateActor(actor->sector(), pos, tongue, -40 + (k << 1), DVector2(0.125, 0.125), nullAngle, 0., 0., actor, 5);
		if (q)
		{
			q->spr.cstat = CSTAT_SPRITE_YCENTER;
			q->spr.pal = 8;
		}
	}
	int k = actor->temp_data[0];	// do not depend on the above loop counter.
	auto pos = actor->spr.pos + actor->spr.angle.ToVector() * 2 * k;
	pos.Z += k * Sgn(actor->vel.Z) * abs(actor->vel.Z / 12);
	auto spawned = CreateActor(actor->sector(), pos, jaw, -40, DVector2(0.5, 0.5), nullAngle, 0., 0., actor, 5);
	if (spawned)
	{
		spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
		if (actor->temp_data[1] > 512 && actor->temp_data[1] < (1024))
			spawned->spr.picnum = jaw + 1;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void rpgexplode(DDukeActor *actor, int hit, const DVector3 &pos, int EXPLOSION2, int EXPLOSION2BOT, int newextra, int playsound)
{
	auto explosion = spawn(actor, EXPLOSION2);
	if (!explosion) return;
	explosion->spr.pos = pos;

	if (actor->spr.scale.X < 0.15625)
	{
		explosion->spr.scale = DVector2(0.09375, 0.09375);
	}
	else if (hit == kHitSector)
	{
		if (actor->vel.Z > 0 && EXPLOSION2BOT >= 0)
			spawn(actor, EXPLOSION2BOT);
		else
		{
			explosion->spr.cstat |= CSTAT_SPRITE_YFLIP;
			explosion->spr.pos.Z += 48;
		}
	}
	if (newextra > 0) actor->spr.extra = newextra;
	S_PlayActorSound(playsound, actor);

	if (actor->spr.scale.X >= 0.15625)
	{
		int x = actor->spr.extra;
		fi.hitradius(actor, gs.rpgblastradius, x >> 2, x >> 1, x - (x >> 2), x);
	}
	else
	{
		int x = actor->spr.extra + (global_random & 3);
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
		actor->spr.picnum = yellow;
	else if (actor->temp_data[0] > ((gs.respawnitemtime >> 1) + (gs.respawnitemtime >> 2)))
		actor->spr.picnum = green;
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
	makeitfall(actor);
	if (ssp(actor, CLIPMASK0))
	{
		if (makesound && (krand() & 255) == 0) S_PlayActorSound(RATTY, actor);
		actor->spr.angle += mapangle((krand() & 31) - 15 + BobVal(actor->temp_data[0] << 8) * 8);
	}
	else
	{
		actor->temp_data[0]++;
		if (actor->temp_data[0] > 1)
		{
			deletesprite(actor);
			return false;
		}
		else actor->spr.angle = randomAngle();
	}
	if (actor->vel.X < 8)
		actor->vel.X += 1/8.;
	actor->spr.angle += mapangle((krand() & 3) - 6);
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool queball(DDukeActor *actor, int pocket, int queball, int stripeball)
{
	if(actor->vel.X != 0)
	{
		DukeStatIterator it(STAT_DEFAULT);
		while (auto aa = it.Next())
		{
			double dist = (aa->spr.pos.XY() - actor->spr.pos.XY()).Length();
			if (aa->spr.picnum == pocket && dist < 3.25)
			{
				deletesprite(actor);
				return false;
			}
		}

		Collision coll;
		auto sect = actor->sector();
		auto pos = actor->spr.pos;
		auto move = actor->spr.angle.ToVector() * actor->vel.X * 0.5;
		int j = clipmove(pos, &sect, move, 1.5, 4., 4., CLIPMASK1, coll);
		actor->spr.pos = pos;;
		actor->setsector(sect);

		if (j == kHitWall)
		{
			auto ang = coll.hitWall->delta().Angle();
			actor->spr.angle = ang * 2 - actor->spr.angle;
		}
		else if (j == kHitSprite)
		{
			fi.checkhitsprite(actor, coll.actor());
		}

		actor->vel.X -= 1/16.;
		if(actor->vel.X < 0) actor->vel.X = 0;
		if (actor->spr.picnum == stripeball)
		{
			actor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
			actor->spr.cstat |= (CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP) & ESpriteFlags::FromInt(int(actor->vel.X / 16.)); // special hack edition...
		}
	}
	else
	{
		double x;
		int p = findplayer(actor, &x);

		if (x < 99.75)
		{
			//						if(actor->spr.pal == 12)
			{
				auto delta = absangle(ps[p].angle.ang, (actor->spr.pos.XY() - ps[p].pos.XY()).Angle());
				if (delta < DAngle22_5 / 2 && PlayerInput(p, SB_OPEN))
					if (ps[p].toggle_key_flag == 1)
					{
						DukeStatIterator it(STAT_ACTOR);
						DDukeActor *act2;
						while ((act2 = it.Next()))
						{
							if (act2->spr.picnum == queball || act2->spr.picnum == stripeball)
							{
								delta = absangle(ps[p].angle.ang, (act2->spr.pos.XY() - ps[p].pos.XY()).Angle());
								if (delta < DAngle22_5 / 2)
								{
									double l;
									findplayer(act2, &l);
									if (x > l) break;
								}
							}
						}
						if (act2 == nullptr)
						{
							if (actor->spr.pal == 12)
								actor->vel.X = 10.25;
							else actor->vel.Z = 8.75;
							actor->spr.angle = ps[p].angle.ang;
							ps[p].toggle_key_flag = 2;
						}
					}
			}
		}
		if (x < 32 && actor->sector() == ps[p].cursector)
		{
			actor->spr.angle = (actor->spr.pos.XY() - ps[p].pos.XY()).Angle();
			actor->vel.X = 3;
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
	auto sectp = actor->sector();
	if (actor->spr.yint == 0)
	{
		actor->spr.yint = 1;

		for (DAngle l = DAngle90; l < DAngle270; l += DAngle22_5)
			for (DAngle j = nullAngle; j < DAngle360; j += DAngle22_5)
			{
				auto k = spawn(actor, forcesphere);
				if (k)
				{
					k->spr.cstat = CSTAT_SPRITE_BLOCK_ALL | CSTAT_SPRITE_YCENTER;
					k->clipdist = 16;
					k->spr.angle = j;
					k->vel.Z = l.Sin() * 2;
					k->vel.X = l.Cos() * 2;
					k->SetOwner(actor);
				}
			}
	}

	if (actor->temp_data[3] > 0)
	{
		if (actor->vel.Z < 24)
			actor->vel.Z += 0.75;
		actor->spr.pos.Z += actor->vel.Z;
		if (actor->spr.pos.Z > sectp->floorz)
			actor->spr.pos.Z = sectp->floorz;
		actor->temp_data[3]--;
		if (actor->temp_data[3] == 0)
		{
			deletesprite(actor);
			return;
		}
		else if (actor->temp_data[2] > 10)
		{
			DukeStatIterator it(STAT_MISC);
			while (auto aa = it.Next())
			{
				if (aa->GetOwner() == actor && aa->spr.picnum == forcesphere)
					aa->temp_data[1] = 1 + (krand() & 63);
			}
			actor->temp_data[3] = 64;
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
	auto sectp = actor->sector();
	DAngle a;

	getglobalz(actor);

	if (sectp->ceilingstat & CSTAT_SECTOR_SKY)
		actor->spr.shade += (sectp->ceilingshade - actor->spr.shade) >> 1;
	else actor->spr.shade += (sectp->floorshade - actor->spr.shade) >> 1;

	if (actor->spr.pos.Z < sectp->ceilingz + 32)
		actor->spr.pos.Z = sectp->ceilingz + 32;

	if (ud.multimode < 2)
	{
		if (actor_tog == 1)
		{
			actor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
			return;
		}
		else if (actor_tog == 2) actor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
	}
	if (fi.ifhitbyweapon(actor) >= 0)
	{
		if (actor->spr.extra < 0 && actor->temp_data[0] != -1)
		{
			actor->temp_data[0] = -1;
			actor->spr.extra = 0;
		}
		if (painsnd >= 0) S_PlayActorSound(painsnd, actor);
		RANDOMSCRAP(actor);
	}

	if (actor->temp_data[0] == -1)
	{
		actor->spr.pos.Z += 4;
		actor->temp_data[2]++;
		if ((actor->temp_data[2] & 3) == 0) spawn(actor, explosion);
		getglobalz(actor);
		actor->spr.angle += DAngle22_5 * 0.75;
		actor->vel.X = 8;
		int j = ssp(actor, CLIPMASK0);
		if (j != 1 || actor->spr.pos.Z > actor->floorz)
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
		if (actor->spr.pos.Z > actor->floorz - 48)
			actor->spr.pos.Z = actor->floorz - 48;
	}

	double xx;
	int p = findplayer(actor, &xx);
	auto Owner = actor->GetOwner();

	// 3 = findplayerz, 4 = shoot

	if (actor->temp_data[0] >= 4)
	{
		actor->temp_data[2]++;
		if ((actor->temp_data[2] & 15) == 0)
		{
			a = actor->spr.angle;
			actor->spr.angle = actor->temp_angle;
			if (attacksnd >= 0) S_PlayActorSound(attacksnd, actor);
			fi.shoot(actor, firelaser);
			actor->spr.angle = a;
		}
		if (actor->temp_data[2] > (26 * 3) || !cansee(actor->spr.pos.plusZ(-16), actor->sector(), ps[p].pos, ps[p].cursector))
		{
			actor->temp_data[0] = 0;
			actor->temp_data[2] = 0;
		}
		else actor->temp_angle +=
			deltaangle(actor->temp_angle, (ps[p].pos.XY() - actor->spr.pos.XY()).Angle()) / 3;
	}
	else if (actor->temp_data[0] == 2 || actor->temp_data[0] == 3)
	{
		if(actor->vel.X > 0) actor->vel.X -= 1;
		else actor->vel.X = 0;

		if (actor->temp_data[0] == 2)
		{
			double l = ps[p].pos.Z - actor->spr.pos.Z;
			if (fabs(l) < 48) actor->temp_data[0] = 3;
			else actor->spr.pos.Z += (Sgn(ps[p].pos.Z - actor->spr.pos.Z) * shift); // The shift here differs between Duke and RR.
		}
		else
		{
			actor->temp_data[2]++;
			if (actor->temp_data[2] > (26 * 3) || !cansee(actor->spr.pos.plusZ(-16), actor->sector(), ps[p].pos, ps[p].cursector))
			{
				actor->temp_data[0] = 1;
				actor->temp_data[2] = 0;
			}
			else if ((actor->temp_data[2] & 15) == 0 && attacksnd >= 0)
			{
				S_PlayActorSound(attacksnd, actor);
				fi.shoot(actor, firelaser);
			}
		}
		actor->spr.angle += deltaangle(actor->spr.angle, (ps[p].pos.XY() - actor->spr.pos.XY()).Angle()) * 0.25;
	}

	if (actor->temp_data[0] != 2 && actor->temp_data[0] != 3 && Owner)
	{
		double dist = (Owner->spr.pos.XY() - actor->spr.pos.XY()).Length();
		if (dist <= 96)
		{
			a = actor->spr.angle;
			actor->vel.X *= 0.5;
		}
		else a = (Owner->spr.pos.XY() - actor->spr.pos.XY()).Angle();

		if (actor->temp_data[0] == 1 || actor->temp_data[0] == 4) // Found a locator and going with it
		{
			dist = (Owner->spr.pos - actor->spr.pos).Length();

			if (dist <= 96) { if (actor->temp_data[0] == 1) actor->temp_data[0] = 0; else actor->temp_data[0] = 5; }
			else
			{
				// Control speed here
				if (dist > 96) { if (actor->vel.X < 16) actor->vel.X += 2.; }
				else
				{
					if(actor->vel.X > 0) actor->vel.X -= 1;
					else actor->vel.X = 0;
				}
			}

			if (actor->temp_data[0] < 2) actor->temp_data[2]++;

			if (xx < 384 && actor->temp_data[0] < 2 && actor->temp_data[2] > (26 * 4))
			{
				actor->temp_data[0] = 2 + (krand() & 2);
				actor->temp_data[2] = 0;
				actor->temp_angle = actor->spr.angle;
			}
		}

		if (actor->temp_data[0] == 0 || actor->temp_data[0] == 5)
		{
			if (actor->temp_data[0] == 0)
				actor->temp_data[0] = 1;
			else actor->temp_data[0] = 4;
			auto NewOwner = LocateTheLocator(actor->spr.hitag, nullptr);
			if (!NewOwner)
			{
				actor->spr.hitag = actor->temp_data[5];
				NewOwner = LocateTheLocator(actor->spr.hitag, nullptr);
				if (!NewOwner)
				{
					deletesprite(actor);
					return;
				}
			}
			else actor->spr.hitag++;
			actor->SetOwner(NewOwner);
		}

		auto ang = deltaangle(actor->spr.angle, a);
		actor->spr.angle += ang * 0.125;

        if (actor->spr.pos.Z < Owner->spr.pos.Z - 2)
            actor->spr.pos.Z += 2;
        else if (actor->spr.pos.Z > Owner->spr.pos.Z + 2)
            actor->spr.pos -= 2;
        else actor->spr.pos.Z = Owner->spr.pos.Z; 
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

	double y = min((actor->floorz - actor->ceilingz) / 128, 4.);
	double x = clamp(0.390625 - y * 0.5, 0.125, 0.75);

	actor->spr.scale = DVector2(x, y);
	actor->spr.pos.Z = actor->floorz;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void reactor(DDukeActor* const actor, int REACTOR, int REACTOR2, int REACTORBURNT, int REACTOR2BURNT, int REACTORSPARK, int REACTOR2SPARK)
{
	auto sectp = actor->sector();

	if (actor->temp_data[4] == 1)
	{
		DukeSectIterator it(actor->sector());
		while (auto a2 = it.Next())
		{
			if (a2->spr.picnum == SECTOREFFECTOR)
			{
				if (a2->spr.lotag == 1)
				{
					a2->spr.lotag = -1;
					a2->spr.hitag = -1;
				}
			}
			else if (a2->spr.picnum == REACTOR)
			{
				a2->spr.picnum = REACTORBURNT;
			}
			else if (a2->spr.picnum == REACTOR2)
			{
				a2->spr.picnum = REACTOR2BURNT;
			}
			else if (a2->spr.picnum == REACTORSPARK || a2->spr.picnum == REACTOR2SPARK)
			{
				a2->spr.cstat = CSTAT_SPRITE_INVISIBLE;
			}
		}		
		return;
	}

	if (actor->temp_data[1] >= 20)
	{
		actor->temp_data[4] = 1;
		return;
	}

	double xx;
	int p = findplayer(actor, &xx);

	actor->temp_data[2]++;
	if (actor->temp_data[2] == 4) actor->temp_data[2] = 0;

	if (xx < 256)
	{
		if ((krand() & 255) < 16)
		{
			if (!S_CheckSoundPlaying(DUKE_LONGTERM_PAIN))
				S_PlayActorSound(DUKE_LONGTERM_PAIN, ps[p].GetActor());

			S_PlayActorSound(SHORT_CIRCUIT, actor);

			ps[p].GetActor()->spr.extra--;
			SetPlayerPal(&ps[p], PalEntry(32, 32, 0, 0));
		}
		actor->temp_data[0] += 128;
		if (actor->temp_data[3] == 0)
			actor->temp_data[3] = 1;
	}
	else actor->temp_data[3] = 0;

	if (actor->temp_data[1])
	{
		actor->temp_data[1]++;

		actor->temp_data[4] = FloatToFixed<8>(actor->spr.pos.Z);
		actor->spr.pos.Z = sectp->floorz - zrand(sectp->floorz - sectp->ceilingz);

		switch (actor->temp_data[1])
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
			while (auto a2 = it.Next())
			{
				if (a2->spr.picnum == MASTERSWITCH)
					if (a2->spr.hitag == actor->spr.hitag)
						if (a2->spr.yint == 0)
							a2->spr.yint = 1;
			}
			break;
		}
		case 4:
		case 7:
		case 10:
		case 15:
		{
			DukeSectIterator it(actor->sector());
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
		for (int x = 0; x < 16; x++)
			RANDOMSCRAP(actor);

		actor->spr.pos.Z = FixedToFloat<8>(actor->temp_data[4]);
		actor->temp_data[4] = 0;

	}
	else
	{
		int j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{
			for (int x = 0; x < 32; x++)
				RANDOMSCRAP(actor);
			if (actor->spr.extra < 0)
				actor->temp_data[1] = 1;
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
	if (actor->temp_data[0] == 0)
	{
		actor->temp_data[1] += 8;

		if (gs.camerashitable)
		{
			if (fi.ifhitbyweapon(actor) >= 0)
			{
				actor->temp_data[0] = 1; // static
				actor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
				for (int x = 0; x < 5; x++)
					RANDOMSCRAP(actor);
				return;
			}
		}

		if (actor->spr.hitag > 0)
		{
			auto const angle = mapangle(8);

			if (actor->temp_data[1] < actor->spr.hitag)
				actor->spr.angle += angle;
			else if (actor->temp_data[1] < (actor->spr.hitag * 3))
				actor->spr.angle -= angle;
			else if (actor->temp_data[1] < (actor->spr.hitag << 2))
				actor->spr.angle += angle;
			else
			{
				actor->spr.angle += angle;
				actor->temp_data[1] = 0;
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
	double size = actor->spr.scale.X;
	if (actor->temp_data[1] > 0)
	{
		actor->temp_data[1]--;
		if (actor->temp_data[1] == 0)
		{
			deletesprite(actor);
			return;
		}
	}
	auto Owner = actor->GetOwner();
	if (!Owner) return;
	if (Owner->temp_data[1] == 0)
	{
		if (actor->temp_data[0] < 64)
		{
			actor->temp_data[0]++;
			size += 0.046875;
		}
	}
	else
		if (actor->temp_data[0] > 64)
		{
			actor->temp_data[0]--;
			size -= 0.046875;
		}

	actor->spr.pos = Owner->spr.pos;;
	actor->spr.angle += mapangle(Owner->temp_data[0]);

	size = clamp(size, REPEAT_SCALE, 1.);

	actor->spr.scale = DVector2(size, size);
	actor->spr.shade = int8_t((size * 32) - 48);

	for (int j = actor->temp_data[0]; j > 0; j--)
		ssp(actor, CLIPMASK0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void watersplash2(DDukeActor* actor)
{
	auto sectp = actor->sector();
	actor->temp_data[0]++;
	if (actor->temp_data[0] == 1)
	{
		if (sectp->lotag != 1 && sectp->lotag != 2)
		{
			deletesprite(actor);
			return;
		}
		if (!S_CheckSoundPlaying(ITEM_SPLASH))
			S_PlayActorSound(ITEM_SPLASH, actor);
	}
	if (actor->temp_data[0] == 3)
	{
		actor->temp_data[0] = 0;
		actor->temp_data[1]++;
	}
	if (actor->temp_data[1] == 5)
		deletesprite(actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void frameeffect1(DDukeActor *actor)
{
	auto Owner = actor->GetOwner();
	if (Owner)
	{
		actor->temp_data[0]++;

		if (actor->temp_data[0] > 7)
		{
			deletesprite(actor);
			return;
		}
		else if (actor->temp_data[0] > 4) actor->spr.cstat |= CSTAT_SPRITE_TRANS_FLIP | CSTAT_SPRITE_TRANSLUCENT;
		else if (actor->temp_data[0] > 2) actor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT;
		actor->spr.xoffset = Owner->spr.xoffset;
		actor->spr.yoffset = Owner->spr.yoffset;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool money(DDukeActor* actor, int BLOODPOOL)
{
	auto sectp = actor->sector();

	actor->vel.X = krandf(0.5) + BobVal(actor->temp_data[0]) * 2;
	actor->temp_data[0] += (krand() & 63);
	if ((actor->temp_data[0] & 2047) > 512 && (actor->temp_data[0] & 2047) < 1596)
	{
		if (sectp->lotag == 2)
		{
			if (actor->vel.Z < 0.25)
				actor->vel.Z += gs.gravity / 32. + krandf(1/32.);
		}
		else
			if (actor->vel.Z < 0.5625)
				actor->vel.Z += gs.gravity / 32. + krandf(1 / 32.);
	}

	ssp(actor, CLIPMASK0);

	if ((krand() & 3) == 0)
		SetActor(actor, actor->spr.pos);

	if (!actor->insector())
	{
		deletesprite(actor);
		return false;
	}
	double l = getflorzofslopeptr(actor->sector(), actor->spr.pos);

	if (actor->spr.pos.Z > l)
	{
		actor->spr.pos.Z = l;

		insertspriteq(actor);
		actor->spr.picnum++;

		DukeStatIterator it(STAT_MISC);
		while (auto aa = it.Next())
		{
			if (aa->spr.picnum == BLOODPOOL)
			{
				double dist = (aa->spr.pos.XY() - actor->spr.pos.XY()).Length();
				if (dist < 348/16.)
				{
					actor->spr.pal = 2;
					break;
				}
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
	auto sectp = actor->sector();

	if(actor->vel.X > 0) actor->vel.X -= 1/16.;
	else actor->vel.X = 0;

	if (timeout)
	{
		if (actor->temp_data[5] < 30 * 10)
			actor->temp_data[5]++;
		else
		{
			deletesprite(actor);
			return false;
		}
	}

	if (actor->vel.Z > 4 && actor->vel.Z < 5)
	{
		SetActor(actor, actor->spr.pos);
		sectp = actor->sector();
	}

	if (callsetsprite) SetActor(actor, actor->spr.pos);

	// this was after the slope calls, but we should avoid calling that for invalid sectors.
	if (!actor->insector())
	{
		deletesprite(actor);
		return false;
	}

	double fz = getflorzofslopeptr(sectp, actor->spr.pos);
	double cz = getceilzofslopeptr(sectp, actor->spr.pos);
	if (cz == fz)
	{
		deletesprite(actor);
		return false;
	}

	if (actor->spr.pos.Z < fz - 2)
	{
		if (actor->temp_data[1] < 2) actor->temp_data[1]++;
		else if (sectp->lotag != 2)
		{
			actor->temp_data[1] = 0;
			if (zcheck1)
			{
				if (actor->temp_data[0] > 6) actor->temp_data[0] = 0;
				else actor->temp_data[0]++;
			}
			else
			{
				if (actor->temp_data[0] > 2)
					actor->temp_data[0] = 0;
				else actor->temp_data[0]++;
			}
		}

		if (actor->vel.Z < 24)
		{
			if (sectp->lotag == 2)
			{
				if (actor->vel.Z < 4)
					actor->vel.Z += 3 / 16.;
				else actor->vel.Z = 4;
			}
			else actor->vel.Z += ( gs.gravity - 50/256.);
		}

		actor->spr.pos += actor->spr.angle.ToVector() * actor->vel.X;
		actor->spr.pos.Z += actor->vel.Z;

		if (floorcheck && actor->spr.pos.Z >= actor->sector()->floorz)
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
		if (actor->temp_data[2] == 0)
		{
			if (!actor->insector())
			{
				deletesprite(actor);
				return false;
			}
			if ((actor->sector()->floorstat & CSTAT_SECTOR_SLOPE))
			{
				deletesprite(actor);
				return false;
			}
			actor->temp_data[2]++;
		}
		double ll = getflorzofslopeptr(actor->sector(), actor->spr.pos);

		actor->spr.pos.Z = ll - 2;
		actor->vel.X = 0;

		if (actor->spr.picnum == JIBS6)
		{
			actor->temp_data[1]++;
			if ((actor->temp_data[1] & 3) == 0 && actor->temp_data[0] < 7)
				actor->temp_data[0]++;
			if (actor->temp_data[1] > 20)
			{
				deletesprite(actor);
				return false;
			}
		}
		else { actor->spr.picnum = JIBS6; actor->temp_data[0] = 0; actor->temp_data[1] = 0; }
	}
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool bloodpool(DDukeActor* actor, bool puke)
{
	auto sectp = actor->sector();

	if (actor->temp_data[0] == 0)
	{
		actor->temp_data[0] = 1;
		if (sectp->floorstat & CSTAT_SECTOR_SLOPE)
		{
			deletesprite(actor);
			return false;
		}
		else insertspriteq(actor);
	}

	makeitfall(actor);

	double xx;
	int p = findplayer(actor, &xx);

	actor->spr.pos.Z = actor->floorz - FOURSLEIGHT_F;

	if (actor->temp_data[2] < 32)
	{
		actor->temp_data[2]++;
		if (attackerflag(actor, SFLAG_FLAMMABLEPOOLEFFECT))
		{
			if (actor->spr.scale.X < 1 && actor->spr.scale.Y < 1)
			{
				actor->spr.scale.X += ((krand() & 3) * REPEAT_SCALE);
				actor->spr.scale.Y += ((krand() & 3) * REPEAT_SCALE);
			}
		}
		else
		{
			if (actor->spr.scale.X < 0.5 && actor->spr.scale.Y < 0.5)
			{
				actor->spr.scale.X += ((krand() & 3) * REPEAT_SCALE);
				actor->spr.scale.Y += ((krand() & 3) * REPEAT_SCALE);
			}
		}
	}

	if (xx < 844 / 16. && actor->spr.scale.X > 0.09375 && actor->spr.scale.Y > 0.09375)
	{
		if (actor->spr.pal == 0 && (krand() & 255) < 16 && !puke)
		{
			if (ps[p].boot_amount > 0)
				ps[p].boot_amount--;
			else
			{
				if (!S_CheckSoundPlaying(DUKE_LONGTERM_PAIN))
					S_PlayActorSound(DUKE_LONGTERM_PAIN, ps[p].GetActor());
				ps[p].GetActor()->spr.extra--;
				SetPlayerPal(&ps[p], PalEntry(32, 16, 0, 0));
			}
		}

		if (actor->temp_data[1] == 1) return false;
		actor->temp_data[1] = 1;

		if (attackerflag(actor, SFLAG_FLAMMABLEPOOLEFFECT))
			ps[p].footprintcount = 10;
		else ps[p].footprintcount = 3;

		ps[p].footprintpal = actor->spr.pal;
		ps[p].footprintshade = actor->spr.shade;

		if (actor->temp_data[2] == 32)
		{
			actor->spr.scale.X += (-6);
			actor->spr.scale.Y += (-6);
		}
	}
	else actor->temp_data[1] = 0;
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void shell(DDukeActor* actor, bool morecheck)
{
	auto sectp = actor->sector();

	ssp(actor, CLIPMASK0);

	if (!actor->insector() || morecheck)
	{
		deletesprite(actor);
		return;
	}

	if (sectp->lotag == 2)
	{
		actor->temp_data[1]++;
		if (actor->temp_data[1] > 8)
		{
			actor->temp_data[1] = 0;
			actor->temp_data[0]++;
			actor->temp_data[0] &= 3;
		}
		if (actor->vel.Z < 0.5) actor-> vel.Z += (gs.gravity / 13); // 8
		else actor->vel.Z -= 0.25;
		if (actor->vel.X > 0)
			actor->vel.X -= 0.25;
		else actor->vel.X = 0;
	}
	else
	{
		actor->temp_data[1]++;
		if (actor->temp_data[1] > 3)
		{
			actor->temp_data[1] = 0;
			actor->temp_data[0]++;
			actor->temp_data[0] &= 3;
		}
		if (actor->vel.Z < 2) actor->vel.Z += (gs.gravity / 3); // 52;
		if(actor->vel.X > 0)
			actor->vel.X -= 1/16.;
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
	auto sectp = actor->sector();

	makeitfall(actor);

	if (actor->vel.Z > 16) actor->vel.Z = 16;
	if (!actor->insector())
	{
		deletesprite(actor);
		return;
	}

	if (actor->spr.pos.Z == actor->floorz - FOURSLEIGHT_F && actor->temp_data[0] < 3)
	{
		actor->vel.Z = -(3 - actor->temp_data[0]) - krandf(2);
		if (sectp->lotag == 2)
			actor->vel.Z *= 0.5;

		actor->spr.scale *= 0.5;
		if (rnd(96))
			SetActor(actor, actor->spr.pos);
		actor->temp_data[0]++;//Number of bounces
	}
	else if (actor->temp_data[0] == 3)
	{
		deletesprite(actor);
		return;
	}

	if(actor->vel.X > 0)
	{
		actor->vel.X -= 1/8.;
		static const ESpriteFlags flips[] = { 0, CSTAT_SPRITE_XFLIP, CSTAT_SPRITE_YFLIP, CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP };
		actor->spr.cstat = flips[int(actor->vel.X * 16) & 3];
	}
	else actor->vel.X = 0;

	ssp(actor, CLIPMASK0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void scrap(DDukeActor* actor, int SCRAP1, int SCRAP6)
{
	auto sectp = actor->sector();

	if(actor->vel.X > 0)
		actor->vel.X -= 1/16.;
	else actor->vel.X = 0;

	if (actor->vel.Z > 4 && actor->vel.Z < 5)
	{
		SetActor(actor, actor->spr.pos);
		sectp = actor->sector();
	}

	if (actor->spr.pos.Z < sectp->floorz - 2)
	{
		if (actor->temp_data[1] < 1) actor->temp_data[1]++;
		else
		{
			actor->temp_data[1] = 0;

			if (actor->spr.picnum < SCRAP6 + 8)
			{
				if (actor->temp_data[0] > 6)
					actor->temp_data[0] = 0;
				else actor->temp_data[0]++;
			}
			else
			{
				if (actor->temp_data[0] > 2)
					actor->temp_data[0] = 0;
				else actor->temp_data[0]++;
			}
		}
		if (actor->vel.Z < 16) actor->vel.Z += (gs.gravity - 50 / 256.);
		actor->spr.pos += actor->spr.angle.ToVector() * actor->vel.X;
		actor->spr.pos.Z += actor->vel.Z;
	}
	else
	{
		if (actor->spr.picnum == SCRAP1 && actor->spr.yint > 0)
		{
			auto spawned = spawn(actor, actor->spr.yint);
			if (spawned)
			{
				SetActor(spawned, actor->spr.pos);
				getglobalz(spawned);
				spawned->spr.hitag = spawned->spr.lotag = 0;
			}
		}
		deletesprite(actor);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void gutsdir(DDukeActor* actor, int gtype, int n, int p)
{
	double scale;

	if (badguy(actor) && actor->spr.scale.X < 0.25)
		scale = 0.125;
	else scale = 0.5;

	double gutz = actor->spr.pos.Z - 8;
	double floorz = getflorzofslopeptr(actor->sector(), actor->spr.pos);

	if (gutz > floorz - 8)
		gutz = floorz - 8;

	gutz += gs.actorinfo[actor->spr.picnum].gutsoffset;

	for (int j = 0; j < n; j++)
	{
		auto a = randomAngle();
		auto vel = krandf(8) + 16;
		auto zvel = -krandf(8) - 2;

		// TRANSITIONAL: owned by a player???
		CreateActor(actor->sector(), DVector3(actor->spr.pos.XY(), gutz), gtype, -32, DVector2(scale, scale), a, vel, zvel, ps[p].GetActor(), 5);
	}
}

//---------------------------------------------------------------------------
//
// Rotating sector
// 
// temp_data[1]: mspos index
// temp_angle: current angle
// temp_data[3]: checkz / acceleration, depending on mode.
//
//---------------------------------------------------------------------------

void handle_se00(DDukeActor* actor)
{
	sectortype *sect = actor->sector();

	double zchange = 0;

	auto Owner = actor->GetOwner();

	if (!Owner || Owner->spr.lotag == -1)
	{
		deletesprite(actor);
		return;
	}

	DAngle ang_amount = mapangle(sect->extra >> 3);
	double direction = 0;

	if (sect->lotag == 30)
	{
		ang_amount *= 0.25;

		if (actor->spr.extra == 1)
		{
			if (actor->tempval < 256)
			{
				actor->tempval += 4;
				if (actor->tempval >= 256)
					callsound(actor->sector(), actor, true);
				if (actor->spr.clipdist) direction = 1; // notreallyclipdist
				else direction = -1;
			}
			else actor->tempval = 256;

			if (sect->floorz > actor->spr.pos.Z) //z's are touching
			{
				sect->addfloorz(-2);
				zchange =  2;
				if (sect->floorz < actor->spr.pos.Z)
					sect->setfloorz(actor->spr.pos.Z);
			}

			else if (sect->floorz < actor->spr.pos.Z) //z's are touching
			{
				sect->addfloorz(2);
				zchange = 2;
				if (sect->floorz > actor->spr.pos.Z)
					sect->setfloorz(actor->spr.pos.Z);
			}
		}
		else if (actor->spr.extra == 3)
		{
			if (actor->tempval > 0)
			{
				actor->tempval -= 4;
				if (actor->tempval <= 0)
					callsound(actor->sector(), actor, true);
				if (actor->spr.clipdist) direction = -1; // notreallyclipdist
				else direction = 1;
			}
			else actor->tempval = 0;

			double checkz = actor->temp_pos.Z;
			if (sect->floorz > checkz) //z's are touching
			{
				sect->addfloorz(-2);
				zchange = -2;
				if (sect->floorz < checkz)
					sect->setfloorz(checkz);
			}

			else if (sect->floorz < checkz) //z's are touching
			{
				sect->addfloorz(2);
				zchange = 2;
				if (sect->floorz > checkz)
					sect->setfloorz(checkz);
			}
		}

		actor->spr.angle += ang_amount * direction;
		actor->temp_angle += ang_amount * direction;
	}
	else
	{
		if (Owner->temp_data[0] == 0) return;
		if (Owner->temp_data[0] == 2)
		{
			deletesprite(actor);
			return;
		}

		if (Owner->spr.angle.Normalized360() > DAngle180)
			direction = -1;
		else direction = 1;
		if (actor->temp_pos.Y == 0)
			actor->temp_pos.Y = (actor->spr.pos.XY() - Owner->spr.pos.XY()).Length();
		actor->vel.X = actor->temp_pos.Y;
		actor->spr.pos.XY() = Owner->spr.pos.XY();
		actor->spr.angle += ang_amount * direction;
		actor->temp_angle += ang_amount * direction;
	}

	if (direction && (sect->floorstat & CSTAT_SECTOR_ALIGN))
	{
		int p;
		for (p = connecthead; p >= 0; p = connectpoint2[p])
		{
			if (ps[p].cursector == actor->sector() && ps[p].on_ground == 1)
			{
				ps[p].angle.addadjustment(ang_amount * direction);

				ps[p].pos.Z += zchange;

				auto result = rotatepoint(Owner->spr.pos, ps[p].pos.XY(), ang_amount * direction);

				ps[p].bobpos += (result - ps[p].pos.XY());

				ps[p].pos.X = result.X;
				ps[p].pos.Y = result.Y;

				auto psp = ps[p].GetActor();
				if (psp->spr.extra <= 0)
				{
					psp->spr.pos.X = result.X;
					psp->spr.pos.Y = result.Y;
				}
			}
		}
		DukeSectIterator itp(actor->sector());
		while (auto act2 = itp.Next())
		{
			if (act2->spr.statnum != STAT_EFFECTOR && act2->spr.statnum != STAT_PROJECTILE && !actorflag(act2, SFLAG2_NOROTATEWITHSECTOR))
			{
				if (act2->isPlayer() && act2->GetOwner())
				{
					continue;
				}

				act2->spr.angle += ang_amount * direction;
				act2->norm_ang();

				act2->spr.pos.Z += zchange;

				auto pos = rotatepoint(Owner->spr.pos, act2->spr.pos.XY(), ang_amount * direction);
				act2->spr.pos.X = pos.X;
				act2->spr.pos.Y = pos.Y;
			}
 		}

	}
	movesector(actor, actor->temp_data[1], actor->temp_angle);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se01(DDukeActor *actor)
{
	int sh = actor->spr.hitag;
	if (actor->GetOwner() == nullptr) //Init
	{
		actor->SetOwner(actor);

		DukeStatIterator it(STAT_EFFECTOR);
		while (auto ac = it.Next())
		{
			if (ac->spr.lotag == SE_19_EXPLOSION_LOWERS_CEILING && ac->spr.hitag == sh)
			{
				actor->temp_data[0] = 0;
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// Subway car
// 
// temp_data[1]: mspos index
// temp_angle: rotation angle
//
//---------------------------------------------------------------------------

void handle_se14(DDukeActor* actor, bool checkstat, int RPG, int JIBS6)
{
	auto sc = actor->sector();
	int st = actor->spr.lotag;

	if (actor->GetOwner() == nullptr)
	{
		auto NewOwner = LocateTheLocator(actor->temp_data[3], &sector[actor->temp_data[0]]);

		if (NewOwner == nullptr)
		{
			I_Error("Could not find any locators for SE# 6 and 14 with a hitag of %d.", actor->temp_data[3]);
		}
		actor->SetOwner(NewOwner);
	}

	auto Owner = actor->GetOwner();
	double dist = (Owner->spr.pos.XY() - actor->spr.pos.XY()).LengthSquared();

	if (dist < 64*64)
	{
		if (st == 6)
			if (Owner->spr.hitag & 1)
				actor->temp_data[4] = sc->extra; //Slow it down
		actor->temp_data[3]++;
		auto NewOwner = LocateTheLocator(actor->temp_data[3], &sector[actor->temp_data[0]]);
		if (NewOwner == nullptr)
		{
			actor->temp_data[3] = 0;
			NewOwner = LocateTheLocator(0, &sector[actor->temp_data[0]]);
		}
		if (NewOwner) actor->SetOwner(NewOwner);
	}

	Owner = actor->GetOwner();
	if(actor->vel.X != 0)
	{
		auto curangle = (Owner->spr.pos.XY() - actor->spr.pos.XY()).Angle();
		auto diffangle = deltaangle(actor->spr.angle, curangle) * 0.125;

		actor->temp_angle += diffangle;
		actor->spr.angle += diffangle;

		bool statstate = (!checkstat || ((sc->floorstat & CSTAT_SECTOR_SKY) == 0 && (sc->ceilingstat & CSTAT_SECTOR_SKY) == 0));
		if (actor->vel.X == sc->extra * maptoworld)
		{
			if (statstate)
			{
				if (!S_CheckSoundPlaying(actor->tempsound))
					S_PlayActorSound(actor->tempsound, actor);
			}
			if ((!checkstat || !statstate) && (ud.monsters_off == 0 && sc->floorpal == 0 && (sc->floorstat & CSTAT_SECTOR_SKY) && rnd(8)))
			{
				double dist2;
				int p = findplayer(actor, &dist2);
				if (dist2 < 1280)//20480)
				{
					auto saved_angle = actor->spr.angle;
					actor->spr.angle = (actor->spr.pos.XY() - ps[p].pos.XY()).Angle();
					fi.shoot(actor, RPG);
					actor->spr.angle = saved_angle;
				}
			}
		}

		if (actor->vel.X <= 4 && statstate)
			S_StopSound(actor->tempsound, actor);

		if ((sc->floorz - sc->ceilingz) < 108)
		{
			if (ud.clipping == 0 && actor->vel.X >=  12)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
				{
					auto psp = ps[p].GetActor();
					if (psp->spr.extra > 0)
					{
						auto sect = ps[p].cursector;
						updatesector(ps[p].pos, &sect);
						if ((sect == nullptr && ud.clipping == 0) || (sect == actor->sector() && ps[p].cursector != actor->sector()))
						{
							ps[p].pos.XY() = actor->spr.pos.XY();
							ps[p].setCursector(actor->sector());

							SetActor(ps[p].GetActor(), actor->spr.pos);
							quickkill(&ps[p]);
						}
					}
				}
		}

		auto vec = actor->spr.angle.ToVector() * actor->vel.X;

		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			auto psp = ps[p].GetActor();
			if (ps[p].insector() && ps[p].cursector->lotag != 2)
			{
				if (po[p].os == actor->sector())
				{
					po[p].opos += vec;
				}

				if (actor->sector() == psp->sector())
				{
					auto result = rotatepoint(actor->spr.pos.XY(), ps[p].pos.XY(), diffangle);

					ps[p].pos.XY() = result + vec;

					ps[p].bobpos += vec;

					ps[p].angle.addadjustment(diffangle);

					if (numplayers > 1)
					{
						ps[p].backupxy();
					}
					if (psp->spr.extra <= 0)
					{
						psp->spr.pos.XY() = ps[p].pos.XY();
					}
				}
			}
		}
		DukeSectIterator it(actor->sector());
		while (auto a2 = it.Next())
		{
			if (a2->spr.statnum != STAT_PLAYER && a2->sector()->lotag != 2 && 
				(a2->spr.picnum != SECTOREFFECTOR || a2->spr.lotag == SE_49_POINT_LIGHT || a2->spr.lotag == SE_50_SPOT_LIGHT) &&
					a2->spr.picnum != LOCATORS)
			{
				a2->spr.pos.XY() = rotatepoint(actor->spr.pos.XY(), a2->spr.pos.XY(), diffangle) + vec;
				a2->spr.angle += diffangle;

				if (numplayers > 1)
				{
					a2->backupvec2();
				}
			}
		}

		movesector(actor, actor->temp_data[1], actor->temp_angle);
		// I have no idea why this is here, but the SE's sector must never, *EVER* change, or the map will corrupt.
		//SetActor(actor, actor->spr.pos);

		if ((sc->floorz - sc->ceilingz) < 108)
		{
			if (ud.clipping == 0 && actor->vel.X >=  12)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
				{
					if (ps[p].GetActor()->spr.extra > 0)
					{
						auto k = ps[p].cursector;
						updatesector(ps[p].pos, &k);
						if ((k == nullptr && ud.clipping == 0) || (k == actor->sector() && ps[p].cursector != actor->sector()))
						{
							ps[p].pos.XY() = actor->spr.pos.XY();
							ps[p].backupxy();
							ps[p].setCursector(actor->sector());

							SetActor(ps[p].GetActor(), actor->spr.pos);
							quickkill(&ps[p]);
						}
					}
				}

			auto actOwner = actor->GetOwner();
			if (actOwner)
			{
				DukeSectIterator itr(actOwner->sector());
				while (auto a2 = itr.Next())
				{
					if (a2->spr.statnum == 1 && badguy(a2) && a2->spr.picnum != SECTOREFFECTOR && a2->spr.picnum != LOCATORS)
					{
						auto k = a2->sector();
						updatesector(a2->spr.pos, &k);
						if (a2->spr.extra >= 0 && k == actor->sector())
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
// Two way train
// 
// temp_data[1]: mspos index
// temp_angle: rotation angle
// temp_data[3]: locator tag
//
//---------------------------------------------------------------------------

void handle_se30(DDukeActor *actor, int JIBS6)
{
	auto sc = actor->sector();

	auto Owner = actor->GetOwner();
	if (Owner == nullptr)
	{
		actor->temp_data[3] = !actor->temp_data[3];
		Owner = LocateTheLocator(actor->temp_data[3], &sector[actor->temp_data[0]]);
		actor->SetOwner(Owner);
	}
	else
	{
		auto dist = (Owner->spr.pos.XY() - actor->spr.pos.XY()).Length();
		if (actor->temp_data[4] == 1) // Starting to go
		{
			if (dist < (128 - 8))
				actor->temp_data[4] = 2;
			else
			{
				if (actor->vel.X == 0)
					operateactivators(actor->spr.hitag + (!actor->temp_data[3]), -1);
				if (actor->vel.X < 16)
					actor->vel.X += 1;
			}
		}
		if (actor->temp_data[4] == 2)
		{
			if (dist <= 8)
				actor->vel.X = 0;

			if(actor->vel.X > 0)
				actor->vel.X -= 1;
			else
			{
				actor->vel.X = 0;
				operateactivators(actor->spr.hitag + (short)actor->temp_data[3], -1);
				actor->SetOwner(nullptr);
				actor->spr.angle += DAngle180;
				actor->temp_data[4] = 0;
				fi.operateforcefields(actor, actor->spr.hitag);
			}
		}
	}

	if(actor->vel.X != 0)
	{
		auto vect = actor->spr.angle.ToVector() * actor->vel.X;

		if ((sc->floorz - sc->ceilingz) < 108)
			if (ud.clipping == 0)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
					{
					auto psp = ps[p].GetActor();
					if (psp->spr.extra > 0)
					{
						auto k = ps[p].cursector;
						updatesector(ps[p].pos, &k);
						if ((k == nullptr && ud.clipping == 0) || (k == actor->sector() && ps[p].cursector != actor->sector()))
						{
							ps[p].pos.XY() = actor->spr.pos.XY();
							ps[p].setCursector(actor->sector());

							SetActor(ps[p].GetActor(), actor->spr.pos);
							quickkill(&ps[p]);
						}
					}
				}
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			auto psp = ps[p].GetActor();
			if (psp->sector() == actor->sector())
			{
				ps[p].pos += vect;

				if (numplayers > 1)
				{
					ps[p].backupxy();
				}

				ps[p].bobpos += vect;
			}

			if (po[p].os == actor->sector())
			{
				po[p].opos += vect;
			}
		}

		DukeSectIterator its(actor->sector());
		while (auto a2 = its.Next())
		{
			if (a2->spr.picnum != SECTOREFFECTOR && a2->spr.picnum != LOCATORS)
			{
				a2->spr.pos += vect;

				if (numplayers > 1)
				{
					a2->backupvec2();
				}
			}
		}

		movesector(actor, actor->temp_data[1], actor->temp_angle);
		//SetActor(actor, actor->spr.pos);

		if ((sc->floorz - sc->ceilingz) < 108)
		{
			if (ud.clipping == 0)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
					if (ps[p].GetActor()->spr.extra > 0)
					{
						auto k = ps[p].cursector;
						updatesector(ps[p].pos, &k);
						if ((k == nullptr && ud.clipping == 0) || (k == actor->sector() && ps[p].cursector != actor->sector()))
						{
							ps[p].pos.XY() = actor->spr.pos.XY();
							ps[p].backupxy();

							ps[p].setCursector(actor->sector());

							SetActor(ps[p].GetActor(), actor->spr.pos);
							quickkill(&ps[p]);
						}
					}

			if (Owner)
			{
				DukeSectIterator it(Owner->sector());
				while (auto a2 = it.Next())
				{
					if (a2->spr.statnum == STAT_ACTOR && badguy(a2) && a2->spr.picnum != SECTOREFFECTOR && a2->spr.picnum != LOCATORS)
					{
						//					if(a2->spr.sector != actor->spr.sector)
						{
							auto k = a2->sector();
							updatesector(a2->spr.pos, &k);
							if (a2->spr.extra >= 0 && k == actor->sector())
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
// Earthquake
//
//---------------------------------------------------------------------------

void handle_se02(DDukeActor* actor)
{
	auto sc = actor->sector();
	int sh = actor->spr.hitag;

	if (actor->temp_data[4] > 0 && actor->temp_data[0] == 0)
	{
		if (actor->temp_data[4] < sh)
			actor->temp_data[4]++;
		else actor->temp_data[0] = 1;
	}

	if (actor->temp_data[0] > 0)
	{
		actor->temp_data[0]++;

		actor->vel.X = 3 / 16.;

		if (actor->temp_data[0] > 96)
		{
			actor->temp_data[0] = -1; //Stop the quake
			actor->temp_data[4] = -1;
			deletesprite(actor);
			return;
		}
		else
		{
			if ((actor->temp_data[0] & 31) == 8)
			{
				earthquaketime = 48;
				S_PlayActorSound(EARTHQUAKE, ps[screenpeek].GetActor());
			}

			if (abs(sc->floorheinum - actor->temp_data[5]) < 8)
				sc->setfloorslope(actor->temp_data[5]);
			else sc->setfloorslope(sc->getfloorslope() + (Sgn(actor->temp_data[5] - sc->getfloorslope()) << 4));
		}

		auto vect = actor->spr.angle.ToVector() * actor->vel.X;

		for (int p = connecthead; p >= 0; p = connectpoint2[p])
			if (ps[p].cursector == actor->sector() && ps[p].on_ground)
			{
				ps[p].pos += vect;
				ps[p].bobpos += vect;
			}

		DukeSectIterator it(actor->sector());
		while (auto a2 = it.Next())
		{
			if (a2->spr.picnum != SECTOREFFECTOR)
			{
				a2->spr.pos += vect;
				SetActor(a2, a2->spr.pos);
			}
		}
		movesector(actor, actor->temp_data[1], nullAngle);
		//SetActor(actor, actor->spr.pos);
	}
}

//---------------------------------------------------------------------------
//
// lights off
//
//---------------------------------------------------------------------------

void handle_se03(DDukeActor *actor)
{
	auto sc = actor->sector();
	int sh = actor->spr.hitag;

	if (actor->temp_data[4] == 0) return;
	double xx;

	findplayer(actor, &xx);

	int palvals = actor->palvals;

	if ((global_random / (sh + 1) & 31) < 4 && !actor->temp_data[2])
	{
		sc->ceilingpal = palvals >> 8;
		sc->floorpal = palvals & 0xff;
		actor->temp_data[0] = actor->spr.shade + (global_random & 15);
	}
	else
	{
		sc->ceilingpal = actor->spr.pal;
		sc->floorpal = actor->spr.pal;
		actor->temp_data[0] = actor->temp_data[3];
	}

	sc->ceilingshade = actor->temp_data[0];
	sc->floorshade = actor->temp_data[0];

	for(auto& wal : wallsofsector(sc))
	{
		if (wal.hitag != 1)
		{
			wal.shade = actor->temp_data[0];
			if ((wal.cstat & CSTAT_WALL_BOTTOM_SWAP) && wal.twoSided())
			{
				wal.nextWall()->shade = wal.shade;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// lights
//
//---------------------------------------------------------------------------

void handle_se04(DDukeActor *actor)
{
	auto sc = actor->sector();
	int sh = actor->spr.hitag;
	int j;

	int palvals = actor->palvals;

	if ((global_random / (sh + 1) & 31) < 4)
	{
		actor->temp_data[1] = actor->spr.shade + (global_random & 15);//Got really bright
		actor->temp_data[0] = actor->spr.shade + (global_random & 15);
		sc->ceilingpal = palvals >> 8;
		sc->floorpal = palvals & 0xff;
		j = 1;
	}
	else
	{
		actor->temp_data[1] = actor->temp_data[2];
		actor->temp_data[0] = actor->temp_data[3];

		sc->ceilingpal = actor->spr.pal;
		sc->floorpal = actor->spr.pal;

		j = 0;
	}

	sc->floorshade = actor->temp_data[1];
	sc->ceilingshade = actor->temp_data[1];

	for (auto& wal : wallsofsector(sc))
	{
		if (j) wal.pal = (palvals & 0xff);
		else wal.pal = actor->spr.pal;

		if (wal.hitag != 1)
		{
			wal.shade = actor->temp_data[0];
			if ((wal.cstat & CSTAT_WALL_BOTTOM_SWAP) && wal.twoSided())
				wal.nextWall()->shade = wal.shade;
		}
	}

	DukeSectIterator it(actor->sector());
	while (auto a2 = it.Next())
	{
		if (a2->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL)
		{
			if (sc->ceilingstat & CSTAT_SECTOR_SKY)
				a2->spr.shade = sc->ceilingshade;
			else a2->spr.shade = sc->floorshade;
		}
	}

	if (actor->temp_data[4])
		deletesprite(actor);

}

//---------------------------------------------------------------------------
//
// boss
//
//---------------------------------------------------------------------------

void handle_se05(DDukeActor* actor, int FIRELASER)
{
	auto sc = actor->sector();
	int j;

	double x;
	int p = findplayer(actor, &x);
	if (x < 512)
	{
		auto ang = actor->spr.angle;
		actor->spr.angle = (actor->spr.pos.XY() - ps[p].pos).Angle();
		fi.shoot(actor, FIRELASER);
		actor->spr.angle = ang;
	}

	auto Owner = actor->GetOwner();
	if (Owner == nullptr) //Start search
	{
		actor->temp_data[4] = 0;
		double maxdist = 0x7fffffff;
		while (1) //Find the shortest dist
		{
			auto NewOwner = LocateTheLocator(actor->temp_data[4], nullptr);
			if (NewOwner == nullptr) break;

			double dist = (ps[p].GetActor()->spr.pos.XY() - NewOwner->spr.pos.XY()).LengthSquared();

			if (maxdist > dist)
			{
				Owner = NewOwner;
				maxdist = dist;
			}

			actor->temp_data[4]++;
		}

		actor->SetOwner(Owner);
		if (!Owner) return; // Undefined case - was not checked.
		actor->vel.Z = (Sgn(Owner->spr.pos.Z - actor->spr.pos.Z) / 16);
	}

	if ((Owner->spr.pos.XY() - actor->spr.pos.XY()).LengthSquared() < 64 * 64)
	{
		// Huh?
		//auto ta = actor->spr.angle;
		//actor->spr.angle = (ps[p].pos.XY() - actor->spr.pos.XY()).Angle();
		//actor->spr.angle = ta;
		actor->SetOwner(nullptr);
		return;

	}
	else actor->vel.X = 16;

	auto ang = (Owner->spr.pos.XY() - actor->spr.pos.XY()).Angle();
	auto angdiff = deltaangle(actor->spr.angle, ang) / 8;
	actor->spr.angle += angdiff;

	if (rnd(32))
	{
		actor->temp_angle += angdiff;
		sc->ceilingshade = 127;
	}
	else
	{
		actor->temp_angle +=
			deltaangle(actor->temp_angle + DAngle90, (ps[p].pos.XY() - actor->spr.pos.XY()).Angle()) * 0.25;
		sc->ceilingshade = 0;
	}
	j = fi.ifhitbyweapon(actor);
	if (j >= 0)
	{
		actor->temp_data[3]++;
		if (actor->temp_data[3] == 5)
		{
			actor->vel.Z += 4;
			FTA(7, &ps[myconnectindex]);
		}
	}

	actor->spr.pos.Z += actor->vel.Z;
	sc->setceilingz(actor->vel.Z);
	sector[actor->temp_data[0]].setceilingz(actor->vel.Z);
	movesector(actor, actor->temp_data[1], actor->temp_angle);
	//SetActor(actor, actor->spr.pos);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se08(DDukeActor *actor, bool checkhitag1)
{
	// work only if its moving
	auto sc = actor->sector();
	int st = actor->spr.lotag;
	int sh = actor->spr.hitag;

	int change, goal = -1;

	if (actor->temp_data[4])
	{
		actor->temp_data[4]++;
		if (actor->temp_data[4] > 8)
		{
			deletesprite(actor);
			return;
		}
		goal = 1;
	}
	else goal = getanimationindex(anim_ceilingz, actor->sector());

	if (goal >= 0)
	{
		if ((sc->lotag & 0x8000) || actor->temp_data[4])
			change = -actor->temp_data[3];
		else
			change = actor->temp_data[3];

		if (st == 9) change = -change;

		DukeStatIterator it(STAT_EFFECTOR);
		while (auto ac = it.Next())
		{
			if (((ac->spr.lotag) == st) && (ac->spr.hitag) == sh)
			{
				auto sect = ac->sector();
				int minshade = ac->spr.shade;

				for (auto& wal : wallsofsector(sect))
				{
					if (wal.hitag != 1)
					{
						wal.shade += change;

						if (wal.shade < minshade)
							wal.shade = minshade;
						else if (wal.shade > ac->temp_data[2])
							wal.shade = ac->temp_data[2];

						if (wal.twoSided())
							if (wal.nextWall()->hitag != 1)
								wal.nextWall()->shade = wal.shade;
					}
				}

				sect->floorshade += change;
				sect->ceilingshade += change;

				if (sect->floorshade < minshade)
					sect->floorshade = minshade;
				else if (sect->floorshade > ac->temp_data[0])
					sect->floorshade = ac->temp_data[0];

				if (sect->ceilingshade < minshade)
					sect->ceilingshade = minshade;
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
// door auto close
//
//---------------------------------------------------------------------------

void handle_se10(DDukeActor* actor, const int* specialtags)
{
	auto sc = actor->sector();
	int sh = actor->spr.hitag;

	if ((sc->lotag & 0xff) == 27 || (sc->floorz > sc->ceilingz && (sc->lotag & 0xff) != 23) || sc->lotag == 32791 - 65536)
	{
		int j = 1;

		if ((sc->lotag & 0xff) != 27)
			for (int p = connecthead; p >= 0; p = connectpoint2[p])
				if (sc->lotag != 30 && sc->lotag != 31 && sc->lotag != 0)
					if (actor->sector() == ps[p].GetActor()->sector())
						j = 0;

		if (j == 1)
		{
			if (actor->temp_data[0] > sh)
			{
				if (specialtags) for (int i = 0; specialtags[i]; i++)
				{
					if (actor->sector()->lotag == specialtags[i] && getanimationindex(anim_ceilingz, actor->sector()) >= 0)
					{
						return;
					}
				}
				fi.activatebysector(actor->sector(), actor);
				actor->temp_data[0] = 0;
			}
			else actor->temp_data[0]++;
		}
	}
	else actor->temp_data[0] = 0;
}

//---------------------------------------------------------------------------
//
// swinging door
//
//---------------------------------------------------------------------------

void handle_se11(DDukeActor *actor)
{
	auto sc = actor->sector();
	if (actor->temp_data[5] > 0)
	{
		actor->temp_data[5]--;
		return;
	}

	if (actor->temp_data[4])
	{
		for(auto& wal : wallsofsector(sc))
		{
			DukeStatIterator it(STAT_ACTOR);
			while (auto ac = it.Next())
			{
				if (ac->spr.extra > 0 && badguy(ac) && IsCloseToWall(ac->spr.pos.XY(), &wal, 16) == EClose::InFront)
					return;
			}
		}

		int k = (actor->spr.yint >> 3) * actor->temp_data[3];
		actor->temp_angle += mapangle(k);
		actor->temp_data[4] += k;
		movesector(actor, actor->temp_data[1], actor->temp_angle);
		//SetActor(actor, actor->spr.pos);

		for(auto& wal : wallsofsector(sc))
		{
			DukeStatIterator it(STAT_PLAYER);
			while (auto ac = it.Next())
			{
				if (ac->GetOwner() && IsCloseToWall(ac->spr.pos.XY(), &wal, 9) == EClose::InFront)
				{
					actor->temp_data[5] = 8; // Delay
					actor->temp_angle -= mapangle(k);
					actor->temp_data[4] -= k;
					movesector(actor, actor->temp_data[1], actor->temp_angle);
					//SetActor(actor, actor->spr.pos);
					return;
				}
			}
		}

		if (actor->temp_data[4] <= -511 || actor->temp_data[4] >= 512)
		{
			actor->temp_data[4] = 0;
			actor->temp_angle = mapangle(actor->temp_angle.Buildang() & 0xffffff00); // Gross hack! What is this supposed to do?
			movesector(actor, actor->temp_data[1], actor->temp_angle);
			//SetActor(actor, actor->spr.pos);
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
	auto sc = actor->sector();
	if (actor->temp_data[0] == 3 || actor->temp_data[3] == 1) //Lights going off
	{
		sc->floorpal = 0;
		sc->ceilingpal = 0;

		for (auto& wal : wallsofsector(sc))
		{
			if (wal.hitag != 1)
			{
				wal.shade = actor->temp_data[1];
				wal.pal = 0;
			}
		}
		sc->floorshade = actor->temp_data[1];
		sc->ceilingshade = actor->temp_data[2];
		actor->temp_data[0] = 0;

		DukeSectIterator it(sc);
		while (auto a2 = it.Next())
		{
			if (a2->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL)
			{
				if (sc->ceilingstat & CSTAT_SECTOR_SKY)
					a2->spr.shade = sc->ceilingshade;
				else a2->spr.shade = sc->floorshade;
			}
		}

		if (actor->temp_data[3] == 1)
		{
			deletesprite(actor);
			return;
		}
	}
	if (actor->temp_data[0] == 1) //Lights flickering on
	{
		// planeonly 1 is RRRA SE47, planeonly 2 is SE48
		int compshade = planeonly == 2 ? sc->ceilingshade : sc->floorshade;
		if (compshade > actor->spr.shade)
		{
			if (planeonly != 2) sc->floorpal = actor->spr.pal;
			if (planeonly != 1) sc->ceilingpal = actor->spr.pal;

			if (planeonly != 2) sc->floorshade -= 2;
			if (planeonly != 1) sc->ceilingshade -= 2;

			for (auto& wal : wallsofsector(sc))
			{
				if (wal.hitag != 1)
				{
					wal.pal = actor->spr.pal;
					wal.shade -= 2;
				}
			}
		}
		else actor->temp_data[0] = 2;

		DukeSectIterator it(actor->sector());
		while (auto a2 = it.Next())
		{
			if (a2->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL)
			{
				if (sc->ceilingstat & CSTAT_SECTOR_SKY)
					a2->spr.shade = sc->ceilingshade;
				else a2->spr.shade = sc->floorshade;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// explosive
//
//---------------------------------------------------------------------------

void handle_se13(DDukeActor* actor)
{
	auto sc = actor->sector();
	if (actor->temp_data[2])
	{
		double amt = ((actor->spr.yint << 5) | 1) * zmaptoworld;

		if (actor->spr.intangle == 512)
		{
			if (actor->spriteextra)
			{
				if (abs(actor->temp_pos.Y - sc->ceilingz) >= amt)
					sc->addceilingz(Sgn(actor->temp_pos.Y - sc->ceilingz) * amt);
				else sc->setceilingz(actor->temp_pos.Y);
			}
			else
			{
				if (abs(actor->temp_pos.Z - sc->floorz) >= amt)
					sc->addfloorz(Sgn(actor->temp_pos.Z - sc->floorz) * amt);
				else sc->setfloorz(actor->temp_pos.Z);
			}
		}
		else
		{
			if (abs(actor->temp_pos.Z - sc->floorz) >= amt)
				sc->addfloorz(Sgn(actor->temp_pos.Z - sc->floorz) * amt);
			else sc->setfloorz(actor->temp_pos.Z);

			if (abs(actor->temp_pos.Y - sc->ceilingz) >= amt)
				sc->addceilingz(Sgn(actor->temp_pos.Y - sc->ceilingz) * amt);
			sc->setceilingz(actor->temp_pos.Y);
		}

		if (actor->temp_data[3] == 1)
		{
			//Change the shades

			actor->temp_data[3]++;
			sc->ceilingstat ^= CSTAT_SECTOR_SKY;

			if (actor->spr.intangle == 512)
			{
				for (auto& wal : wallsofsector(sc))
					wal.shade = actor->spr.shade;

				sc->floorshade = actor->spr.shade;

				if (ps[0].one_parallax_sectnum != nullptr)
				{
					sc->ceilingpicnum = ps[0].one_parallax_sectnum->ceilingpicnum;
					sc->ceilingshade = ps[0].one_parallax_sectnum->ceilingshade;
				}
			}
		}
		actor->temp_data[2]++;
		if (actor->temp_data[2] > 256)
		{
			deletesprite(actor);
			return;
		}
	}


	if (actor->temp_data[2] == 4 && actor->spr.angle != DAngle90)
		for (int x = 0; x < 7; x++) RANDOMSCRAP(actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se15(DDukeActor* actor)
{
	if (actor->temp_data[4])
	{
		actor->vel.X = 1;

		if (actor->temp_data[4] == 1) //Opening
		{
			if (actor->temp_data[3] >= (actor->spr.yint >> 3))
			{
				actor->temp_data[4] = 0; //Turn off the sliders
				callsound(actor->sector(), actor);
				return;
			}
			actor->temp_data[3]++;
		}
		else if (actor->temp_data[4] == 2)
		{
			if (actor->temp_data[3] < 1)
			{
				actor->temp_data[4] = 0;
				callsound(actor->sector(), actor);
				return;
			}
			actor->temp_data[3]--;
		}

		movesector(actor, actor->temp_data[1], nullAngle);
		//SetActor(actor, actor->spr.pos);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se16(DDukeActor* actor, int REACTOR, int REACTOR2)
{
	auto sc = actor->sector();

	actor->temp_angle += DAngle22_5 / 4;
	if (sc->floorz < sc->ceilingz) actor->spr.shade = 0;

	else if (sc->ceilingz < actor->temp_pos.Z)
	{

		//The following code check to see if
		//there is any other sprites in the sector.
		//If there isn't, then kill this sectoreffector
		//itself.....

		DukeSectIterator it(actor->sector());
		DDukeActor* a2;
		while ((a2 = it.Next()))
		{
			if (a2->spr.picnum == REACTOR || a2->spr.picnum == REACTOR2)
				return;
		}
		if (a2 == nullptr)
		{
			deletesprite(actor);
			return;
		}
		else actor->spr.shade = 1;
	}

	if (actor->spr.shade) sc->addceilingz(4);
	else sc->addceilingz(-2);

	movesector(actor, actor->temp_data[1], actor->temp_angle);
	//SetActor(actor, actor->spr.pos);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se17(DDukeActor* actor)
{
	auto sc = actor->sector();
	int sh = actor->spr.hitag;
	double refheight = actor->spr.yint * zmaptoworld;

	double q = actor->temp_data[0] * refheight * 4;

	sc->addceilingz(q);
	sc->addfloorz(q);

	DukeSectIterator it(actor->sector());
	while (auto act1 = it.Next())
	{
		if (act1->spr.statnum == STAT_PLAYER && act1->GetOwner())
		{
			int p = act1->spr.yint;
			ps[p].pos.Z += q;
			ps[p].truefz += q;
			ps[p].truecz += q;
		}
		if (act1->spr.statnum != STAT_EFFECTOR)
		{
			act1->spr.pos.Z += q;
		}

		act1->floorz = sc->floorz;
		act1->ceilingz = sc->ceilingz;
	}

	if (actor->temp_data[0]) //If in motion
	{
		if (abs(sc->floorz - actor->temp_pos.X) <= refheight)
		{
			activatewarpelevators(actor, 0);
			return;
		}

		if (actor->temp_data[0] == -1)
		{
			if (sc->floorz > actor->temp_pos.Y)
				return;
		}
		else if (sc->ceilingz < actor->temp_pos.Z) return;

		if (actor->temp_data[1] == 0) return;
		actor->temp_data[1] = 0;

		DDukeActor* act2;
		DukeStatIterator itr(STAT_EFFECTOR);
		while ((act2 = itr.Next()))
		{
			if (actor != act2 && (act2->spr.lotag) == 17)
				if ((sc->hitag - actor->temp_data[0]) == (act2->sector()->hitag) && sh == (act2->spr.hitag))
					break;
		}

		if (act2 == nullptr) return;

		DukeSectIterator its(actor->sector());
		while (auto act3 = its.Next())
		{
			if (act3->spr.statnum == STAT_PLAYER && act3->GetOwner())
			{
				int p = act3->PlayerIndex();

				ps[p].opos -= ps[p].pos;
				ps[p].pos.XY() += act2->spr.pos.XY() - actor->spr.pos.XY();
				ps[p].pos.Z += act2->sector()->floorz - sc->floorz;
				ps[p].opos += ps[p].pos;

				if (q > 0) ps[p].backupz();

				act3->floorz = act2->sector()->floorz;
				act3->ceilingz = act2->sector()->ceilingz;

				ps[p].setbobpos();

				ps[p].truefz = act3->floorz;
				ps[p].truecz = act3->ceilingz;
				ps[p].bobcounter = 0;

				ChangeActorSect(act3, act2->sector());
				ps[p].setCursector(act2->sector());
			}
			else if (act3->spr.statnum != STAT_EFFECTOR)
			{
				act3->opos -= act3->spr.pos;
				act3->spr.pos.XY() += act2->spr.pos.XY() - actor->spr.pos.XY();
				act3->spr.pos.Z += act2->sector()->floorz - sc->floorz;
				act3->opos += act3->spr.pos;

				if (q > 0) act3->backupz();

				ChangeActorSect(act3, act2->sector());
				SetActor(act3, act3->spr.pos);

				act3->floorz = act2->sector()->floorz;
				act3->ceilingz = act2->sector()->ceilingz;

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
	auto sc = actor->sector();

	double extra = sc->extra * zmaptoworld;
	double goal = FixedToFloat<8>(actor->temp_data[1]);
	if (actor->temp_data[0])
	{
		if (actor->spr.pal)
		{
			if (actor->spr.intangle == 512)
			{
				sc->addceilingz(-extra);
				if (sc->ceilingz <= goal)
				{
					sc->setceilingz(goal);
					deletesprite(actor);
					return;
				}
			}
			else
			{
				sc->addfloorz(extra);
				if (morecheck)
				{
					DukeSectIterator it(actor->sector());
					while (auto a2 = it.Next())
					{
						if (a2->isPlayer() && a2->GetOwner())
						{
							if (ps[a2->PlayerIndex()].on_ground == 1) ps[a2->PlayerIndex()].pos.Z += extra;
						}
						if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && a2->spr.statnum != STAT_PROJECTILE)
						{
							a2->spr.pos.Z += extra;
							a2->floorz = sc->floorz;
						}
					}
				}
				if (sc->floorz >= goal)
				{
					sc->setfloorz(goal);
					deletesprite(actor);
					return;
				}
			}
		}
		else
		{
			if (actor->spr.intangle == 512)
			{
				sc->addceilingz(extra);
				if (sc->ceilingz >= actor->spr.pos.Z)
				{
					sc->setceilingz(actor->spr.pos.Z);
					deletesprite(actor);
					return;
				}
			}
			else
			{
				sc->addfloorz(-extra);
				if (morecheck)
				{
					DukeSectIterator it(actor->sector());
					while (auto a2 = it.Next())
					{
						if (a2->isPlayer() && a2->GetOwner())
						{
							if (ps[a2->PlayerIndex()].on_ground == 1) ps[a2->PlayerIndex()].pos.Z -= extra;
						}
						if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && a2->spr.statnum != STAT_PROJECTILE)
						{
							a2->spr.pos.Z -= extra;
							a2->floorz = sc->floorz;
						}
					}
				}
				if (sc->floorz <= actor->spr.pos.Z)
				{
					sc->setfloorz(actor->spr.pos.Z);
					deletesprite(actor);
					return;
				}
			}
		}

		actor->temp_data[2]++;
		if (actor->temp_data[2] >= actor->spr.hitag)
		{
			actor->temp_data[2] = 0;
			actor->temp_data[0] = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DDukeActor* ifhitsectors(sectortype* sect)
{
	DukeStatIterator it(STAT_MISC);
	while (auto a1 = it.Next())
	{
		if (actorflag(a1, SFLAG_TRIGGER_IFHITSECTOR) && sect == a1->sector())
			return a1;
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se19(DDukeActor *actor, int BIGFORCE)
{
	auto sc = actor->sector();
	int sh = actor->spr.hitag;

	if (actor->temp_data[0])
	{
		if (actor->temp_data[0] == 1)
		{
			actor->temp_data[0]++;
			for (auto& wal : wallsofsector(sc))
			{
				if (wal.overpicnum == BIGFORCE)
				{
					wal.cstat &= (CSTAT_WALL_TRANSLUCENT | CSTAT_WALL_1WAY | CSTAT_WALL_XFLIP | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_BOTTOM_SWAP);
					wal.overpicnum = 0;
					auto nextwal = wal.nextWall();
					if (nextwal != nullptr)
					{
						nextwal->overpicnum = 0;
						nextwal->cstat &= (CSTAT_WALL_TRANSLUCENT | CSTAT_WALL_1WAY | CSTAT_WALL_XFLIP | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_BOTTOM_SWAP);
					}
				}
			}
		}

		if (sc->ceilingz < sc->floorz)
			sc->addceilingz(actor->spr.yint * zmaptoworld);
		else
		{
			sc->setceilingz(sc->floorz);

			DukeStatIterator it(STAT_EFFECTOR);
			while (auto a2 = it.Next())
			{
				auto a2Owner = a2->GetOwner();
				if (a2->spr.lotag == 0 && a2->spr.hitag == sh && a2Owner)
				{
					auto sectp = a2Owner->sector(); 
					a2->sector()->floorpal = a2->sector()->ceilingpal =	sectp->floorpal;
					a2->sector()->floorshade = a2->sector()->ceilingshade = sectp->floorshade;
					a2Owner->temp_data[0] = 2;
				}
			}
			deletesprite(actor);
			return;
		}
	}
	else //Not hit yet
	{
		auto hitter = ifhitsectors(actor->sector());
		if (hitter)
		{
			FTA(8, &ps[myconnectindex]);

			DukeStatIterator it(STAT_EFFECTOR);
			while (auto ac = it.Next())
			{
				int x = ac->spr.lotag & 0x7fff;
				switch (x)
				{
				case 0:
					if (ac->spr.hitag == sh && ac->GetOwner())
					{
						auto sectp = ac->sector();
						sectp->floorshade = sectp->ceilingshade =	ac->GetOwner()->spr.shade;
						sectp->floorpal = sectp->ceilingpal =	ac->GetOwner()->spr.pal;
					}
					break;

				case 1:
				case 12:
					//case 18:
				case 19:

					if (sh == ac->spr.hitag)
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
	auto sc = actor->sector();

	if (actor->temp_data[0] == 0) return;

	//if(actor->vel.X != 0) //Moving
	{
		auto vec = actor->spr.angle.ToVector() * (actor->temp_data[0] == 1 ? 0.5 : -0.5);

		actor->temp_data[3] += actor->temp_data[0] == 1? 8 :- 8;

		actor->spr.pos += vec;

		if (actor->temp_data[3] <= 0 || (actor->temp_data[3] >> 6) >= (actor->spr.yint >> 6))
		{
			actor->spr.pos -= vec;
			actor->temp_data[0] = 0;
			callsound(actor->sector(), actor);
			return;
		}

		DukeSectIterator it(actor->sector());
		while (auto a2 = it.Next())
		{
			if (a2->spr.statnum != STAT_EFFECTOR && a2->vel.Z == 0)
			{
				actor->spr.pos += vec;
				if (a2->sector()->floorstat & CSTAT_SECTOR_SLOPE)
					if (a2->spr.statnum == 2)
						makeitfall(a2);
			}
		}

		auto& wal = actor->temp_walls;
		dragpoint(wal[0], wal[0]->pos + vec);
		dragpoint(wal[1], wal[1]->pos + vec);

		for (int p = connecthead; p >= 0; p = connectpoint2[p])
			if (ps[p].cursector == actor->sector() && ps[p].on_ground)
			{
				ps[p].pos += vec;
				ps[p].backupxy();

				SetActor(ps[p].GetActor(), ps[p].pos.plusZ(gs.playerheight));
			}

		sc->addfloorxpan(-vec.X * 2);
		sc->addfloorypan(-vec.Y * 2);

		sc->addceilingxpan(-vec.X * 2);
		sc->addceilingypan(-vec.Y * 2);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se21(DDukeActor* actor)
{
	auto sc = actor->sector();
	double lp;

	if (actor->temp_data[0] == 0) return;

	if (actor->spr.intangle == 1536)
		lp = sc->ceilingz;
	else
		lp = sc->floorz;

	if (actor->temp_data[0] == 1) //Decide if the sector should go up or down
	{
		actor->vel.Z = (Sgn(actor->spr.pos.Z - lp) * (actor->spr.yint << 4) * zmaptoworld);
		actor->temp_data[0]++;
	}

	if (sc->extra == 0)
	{
		lp += actor->vel.Z;

		if (abs(lp - actor->spr.pos.Z) < 4)
		{
			lp = actor->spr.pos.Z;
			deletesprite(actor);
		}

		if (actor->spr.intangle == 1536)
			sc->setceilingz(lp);
		else
			sc->setfloorz(lp);

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
	auto sc = actor->sector();
	if (actor->temp_data[1])
	{
		if (getanimationindex(anim_ceilingz, &sector[actor->temp_data[0]]) >= 0)
			sc->addceilingz(sc->extra * 9 * zmaptoworld);
		else actor->temp_data[1] = 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se26(DDukeActor* actor)
{
	auto sc = actor->sector();
	double zvel = actor->vel.Z;

	actor->vel.X = 2;
	DVector2 vect = 2 * actor->spr.angle.ToVector(); // was: (32 * b sin) >> 14

	actor->spr.shade++;
	if (actor->spr.shade > 7)
	{
		actor->spr.pos.XY() = actor->temp_pos.XY();
		sc->addfloorz(-((zvel * actor->spr.shade) - zvel));
		actor->spr.shade = 0;
	}
	else
		sc->addfloorz(zvel);

	DukeSectIterator it(actor->sector());
	while (auto a2 = it.Next())
	{
		if (a2->spr.statnum != 3 && a2->spr.statnum != 10)
		{
			a2->spr.pos = DVector3(vect, zvel);
			SetActor(a2, a2->spr.pos);
		}
	}

	for (int p = connecthead; p >= 0; p = connectpoint2[p])
		if (ps[p].GetActor()->sector() == actor->sector() && ps[p].on_ground)
		{
			ps[p].fric.X += vect.X;
			ps[p].fric.Y += vect.Y;
			ps[p].pos.Z += zvel;
		}

	movesector(actor, actor->temp_data[1], nullAngle);
	//SetActor(actor, actor->spr.pos);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se27(DDukeActor* actor)
{
	int sh = actor->spr.hitag;
	int p;
	double xx;

	if (ud.recstat == 0) return;

	actor->temp_angle = actor->spr.angle;

	p = findplayer(actor, &xx);
	if (ps[p].GetActor()->spr.extra > 0 && myconnectindex == screenpeek)
	{
		if (actor->temp_data[0] < 0)
		{
			ud.cameraactor = actor;
			actor->temp_data[0]++;
		}
		else if (ud.recstat == 2 && ps[p].newOwner == nullptr)
		{
			if (cansee(actor->spr.pos, actor->sector(), ps[p].pos, ps[p].cursector))
			{
				if (xx < sh * maptoworld)
				{
					ud.cameraactor = actor;
					actor->temp_data[0] = 999;
					actor->spr.angle += deltaangle(actor->spr.angle, (ps[p].pos.XY() - actor->spr.pos.XY()).Angle()) * 0.125;
					actor->spr.yint = 100 + int((actor->spr.pos.Z - ps[p].pos.Z) * (256. / 257.));

				}
				else if (actor->temp_data[0] == 999)
				{
					if (ud.cameraactor == actor)
						actor->temp_data[0] = 0;
					else actor->temp_data[0] = -10;
					ud.cameraactor = actor;

				}
			}
			else
			{
				actor->spr.angle = (ps[p].pos.XY() - actor->spr.pos.XY()).Angle();

				if (actor->temp_data[0] == 999)
				{
					if (ud.cameraactor == actor)
						actor->temp_data[0] = 0;
					else actor->temp_data[0] = -20;
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

void handle_se24(DDukeActor *actor, bool scroll, double mult)
{
	if (actor->temp_data[4]) return;

	auto vec = actor->spr.angle.ToVector() * actor->spr.yint / 256.;

	DukeSectIterator it(actor->sector());
	while (auto a2 = it.Next())
	{
		if (a2->vel.Z >= 0)
		{
			switch (a2->spr.statnum)
			{
			case STAT_MISC:
			case STAT_STANDABLE:
			case STAT_ACTOR:
			case STAT_DEFAULT:
				if (actorflag(a2, SFLAG_SE24_REMOVE))
				{
					a2->spr.scale = DVector2(0, 0);
					continue;
				}

				if (actorflag(a2, SFLAG_SE24_NOCARRY) ||
					wallswitchcheck(a2))
					continue;

				if (a2->spr.pos.Z > a2->floorz - 16)
				{
					a2->spr.pos += vec * mult;

					SetActor(a2, a2->spr.pos);

					if (a2->sector()->floorstat & CSTAT_SECTOR_SLOPE)
						if (a2->spr.statnum == STAT_ZOMBIEACTOR)
							makeitfall(a2);
				}
				break;
			}
		}
	}

	for (auto p = connecthead; p >= 0; p = connectpoint2[p])
	{
		if (ps[p].cursector == actor->sector() && ps[p].on_ground)
		{
			if (abs(ps[p].pos.Z - ps[p].truefz) < gs.playerheight + 9)
			{
				ps[p].fric += vec * (1. / 4.);
			}
		}
	}
	if (scroll) actor->sector()->addfloorxpan(actor->spr.yint / 128.f);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se25(DDukeActor* actor, int snd1, int snd2)
{
	auto sec = actor->sector();
	auto add = actor->spr.yint * zmaptoworld;

	if (sec->floorz <= sec->ceilingz)
		actor->spr.shade = 0;
	else if (sec->ceilingz <= actor->temp_pos.Z)
		actor->spr.shade = 1;

	if (actor->spr.shade)
	{
		sec->addceilingz(add);
		if (sec->ceilingz > sec->floorz)
		{
			sec->setceilingz(sec->floorz);
			if (pistonsound && snd1 >= 0)
				S_PlayActorSound(snd1, actor);
		}
	}
	else
	{
		sec->addceilingz(-add);
		if (sec->ceilingz < actor->temp_pos.Z)
		{
			sec->setceilingz(actor->temp_pos.Z);
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
	auto sc = actor->sector();

	if (actor->temp_data[0] == 1)
	{
		// Choose dir

		double targetval = actor->spr.yint * zmaptoworld;

		if (actor->temp_data[2] == 1) // Retract
		{
			if (actor->spr.intangle != 1536)
			{
				if (abs(sc->ceilingz - actor->spr.pos.Z) < targetval * 2)
				{
					sc->setceilingz(actor->spr.pos.Z);
					callsound(actor->sector(), actor);
					actor->temp_data[2] = 0;
					actor->temp_data[0] = 0;
				}
				else sc->addceilingz(Sgn(actor->spr.pos.Z - sc->ceilingz) * targetval);
			}
			else
			{
				if (abs(sc->ceilingz - actor->temp_pos.Z) < targetval * 2)
				{
					sc->setceilingz(actor->temp_pos.Z);
					callsound(actor->sector(), actor);
					actor->temp_data[2] = 0;
					actor->temp_data[0] = 0;
				}
				else sc->addceilingz(Sgn(actor->temp_pos.Z - sc->ceilingz) * targetval);
			}
			return;
		}

		if ((actor->spr.intangle & 2047) == 1536)
		{
			if (abs(sc->ceilingz - actor->spr.pos.Z) < targetval * 2)
			{
				actor->temp_data[0] = 0;
				actor->temp_data[2] = !actor->temp_data[2];
				callsound(actor->sector(), actor);
				sc->setceilingz(actor->spr.pos.Z);
			}
			else sc->addceilingz(Sgn(actor->spr.pos.Z - sc->ceilingz) * targetval);
		}
		else
		{
			if (abs(sc->ceilingz - actor->temp_pos.Z) < targetval * 2)
			{
				actor->temp_data[0] = 0;
				actor->temp_data[2] = !actor->temp_data[2];
				callsound(actor->sector(), actor);
			}
			else sc->addceilingz(-Sgn(actor->spr.pos.Z - actor->temp_pos.Z) * targetval);
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
	auto sc = actor->sector();

	if (sc->ceilingz > actor->spr.pos.Z)
		for (int j = 0; j < 8; j++)
		{
			actor->spr.angle = randomAngle(90);
			auto spawned = spawn(actor, SMALLSMOKE);
			if (spawned)
			{
				spawned->vel.X = 6 + krandf(8);
				ssp(spawned, CLIPMASK0);
				SetActor(spawned, spawned->spr.pos);
				if (rnd(16))
					spawn(actor, EXPLOSION2);
			}
		}


	double targetval = actor->spr.yint * zmaptoworld;
	switch (actor->temp_data[0])
	{
	case 0:
		sc->addceilingz(targetval);
		if (sc->ceilingz > sc->floorz)
			sc->setfloorz(sc->ceilingz);
		if (sc->ceilingz > actor->spr.pos.Z + 32)
			actor->temp_data[0]++;
		break;
	case 1:
		sc->addceilingz(-targetval * 4);
		if (sc->ceilingz < actor->temp_pos.Y)
		{
			sc->setceilingz(actor->temp_pos.Y);
			actor->temp_data[0] = 0;
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
	auto wal = actor->temp_walls[0]; 
	if (!wal) return; // E4L1 contains an uninitialized SE128 which would crash without this.

	//if (wal->cstat | 32) // this has always been bugged, the condition can never be false.
	{
		wal->cstat &= ~CSTAT_WALL_1WAY;
		wal->cstat |= CSTAT_WALL_MASKED;
		if (wal->twoSided())
		{
			wal->nextWall()->cstat &= ~CSTAT_WALL_1WAY;
			wal->nextWall()->cstat |= CSTAT_WALL_MASKED;
		}
	}
//	else return;

	wal->overpicnum++;
	auto nextwal = wal->nextWall();
	if (nextwal)
		nextwal->overpicnum++;

	if (actor->temp_data[0] < actor->temp_data[1]) actor->temp_data[0]++;
	else
	{
		wal->cstat &= (CSTAT_WALL_TRANSLUCENT | CSTAT_WALL_1WAY | CSTAT_WALL_XFLIP | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_BOTTOM_SWAP);
		if (nextwal)
			nextwal->cstat &= (CSTAT_WALL_TRANSLUCENT | CSTAT_WALL_1WAY | CSTAT_WALL_XFLIP | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_BOTTOM_SWAP);
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
	auto sc = actor->sector();

	if (actor->temp_data[0] > countmax)
	{
		deletesprite(actor);
		return;
	}
	else actor->temp_data[0]++;

	double x = sc->floorz - sc->ceilingz;

	if (rnd(64))
	{
		auto k = spawn(actor, EXPLOSION2);
		if (k)
		{
			double s = 0.03125 + (krand() & 7) * REPEAT_SCALE;
			k->spr.scale = DVector2(s, s);
			k->spr.pos.Z = sc->floorz + krandf(x);
			k->spr.angle += DAngle45 - randomAngle(90);
			k->vel.X = krandf(8);
			ssp(k, CLIPMASK0);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se29(DDukeActor* actor)
{
	auto sc = actor->sector();
	actor->spr.hitag += 64;
	double val = actor->spr.yint * BobVal(actor->spr.hitag) / 64.;
	sc->setfloorz(actor->spr.pos.Z + val);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se31(DDukeActor* actor, bool choosedir)
{
	auto sec = actor->sector();

	if (actor->temp_data[0] == 1)
	{
		// Choose dir

		if (choosedir && actor->temp_data[3] > 0)
		{
			actor->temp_data[3]--;
			return;
		}

		if (actor->temp_data[2] == 1) // Retract
		{
			if (actor->spr.intangle != 1536)
			{
				if (abs(sec->floorz- actor->spr.pos.Z) < actor->temp_pos.Z)
				{
					sec->setfloorz(actor->spr.pos.Z);
					actor->temp_data[2] = 0;
					actor->temp_data[0] = 0;
					if (choosedir) actor->temp_data[3] = actor->spr.hitag;
					callsound(actor->sector(), actor);
				}
				else
				{
					double l = Sgn(actor->spr.pos.Z - sec->floorz) * actor->temp_pos.Z;
					sec->addfloorz(l);

					DukeSectIterator it(actor->sector());
					while (auto a2 = it.Next())
					{
						if (a2->isPlayer() && a2->GetOwner())
							if (ps[a2->PlayerIndex()].on_ground == 1)
								ps[a2->PlayerIndex()].pos.Z +=l;
						if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && (!choosedir || a2->spr.statnum != STAT_PROJECTILE))
						{
							a2->spr.pos.Z += l;
							a2->floorz = sec->floorz;
						}
					}
				}
			}
			else
			{
				if (abs(sec->floorz - actor->temp_pos.Y) < actor->temp_pos.Z)
				{
					sec->floorz = actor->temp_pos.Y;
					callsound(actor->sector(), actor);
					actor->temp_data[2] = 0;
					actor->temp_data[0] = 0;
					if (choosedir) actor->temp_data[3] = actor->spr.hitag;
				}
				else
				{
					double l = Sgn(actor->temp_pos.Y - sec->floorz) * actor->temp_pos.Z;
					sec->addfloorz(l);

					DukeSectIterator it(actor->sector());
					while (auto a2 = it.Next())
					{
						if (a2->isPlayer() && a2->GetOwner())
							if (ps[a2->PlayerIndex()].on_ground == 1)
								ps[a2->PlayerIndex()].pos.Z += l;
						if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && (!choosedir || a2->spr.statnum != STAT_PROJECTILE))
						{
							a2->spr.pos.Z += l;
							a2->floorz = sec->floorz;
						}
					}
				}
			}
			return;
		}

		if ((actor->spr.intangle & 2047) == 1536)
		{
			if (abs(actor->spr.pos.Z - sec->floorz) < actor->temp_pos.Z)
			{
				callsound(actor->sector(), actor);
				actor->temp_data[0] = 0;
				actor->temp_data[2] = 1;
				if (choosedir) actor->temp_data[3] = actor->spr.hitag;
			}
			else
			{
				double l = Sgn(actor->spr.pos.Z - sec->floorz) * actor->temp_pos.Z;
				sec->addfloorz(l);

				DukeSectIterator it(actor->sector());
				while (auto a2 = it.Next())
				{
					if (a2->isPlayer() && a2->GetOwner())
						if (ps[a2->PlayerIndex()].on_ground == 1)
							ps[a2->PlayerIndex()].pos.Z += l;
					if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && (!choosedir || a2->spr.statnum != STAT_PROJECTILE))
					{
						a2->spr.pos.Z += l;
						a2->floorz = sec->floorz;
					}
				}
			}
		}
		else
		{
			if (abs(sec->floorz - actor->temp_pos.Y) < actor->temp_pos.Z)
			{
				actor->temp_data[0] = 0;
				callsound(actor->sector(), actor);
				actor->temp_data[2] = 1;
				actor->temp_data[3] = actor->spr.hitag;
			}
			else
			{
				double l = Sgn(actor->spr.pos.Z - actor->temp_pos.Y) * actor->temp_pos.Z;
				sec->addfloorz(-l);

				DukeSectIterator it(actor->sector());
				while (auto a2 = it.Next())
				{
					if (a2->isPlayer() && a2->GetOwner())
						if (ps[a2->PlayerIndex()].on_ground == 1)
							ps[a2->PlayerIndex()].pos.Z -= l;
					if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && (!choosedir || a2->spr.statnum != STAT_PROJECTILE))
					{
						a2->spr.pos.Z -= l;
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
	double zr;
	Collision hz, lz;

	if( actor->spr.statnum == STAT_PLAYER || actor->spr.statnum == STAT_STANDABLE || actor->spr.statnum == STAT_ZOMBIEACTOR || actor->spr.statnum == STAT_ACTOR || actor->spr.statnum == STAT_PROJECTILE)
	{
		if(actor->spr.statnum == STAT_PROJECTILE)
			zr = 0.25;
		else zr = 7.9375;

		auto cc = actor->spr.cstat2;
		actor->spr.cstat2 |= CSTAT2_SPRITE_NOFIND; // don't clip against self. getzrange cannot detect this because it only receives a coordinate.
		getzrange(actor->spr.pos.plusZ(-1), actor->sector(), &actor->ceilingz, hz, &actor->floorz, lz, zr, CLIPMASK0);
		actor->spr.cstat2 = cc;

		actor->spr.cstat2 &= ~CSTAT2_SPRITE_NOSHADOW;
		if( lz.type == kHitSprite && (lz.actor()->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == 0 )
		{
			if( badguy(lz.actor()) && lz.actor()->spr.pal != 1)
			{
				if( actor->spr.statnum != STAT_PROJECTILE)
				{
					actor->spr.cstat2 |= CSTAT2_SPRITE_NOSHADOW; // No shadows on actors
					actor->vel.X = -16;
					ssp(actor, CLIPMASK0);
				}
			}
			else if(lz.actor()->isPlayer() && badguy(actor) )
			{
				actor->spr.cstat2 |= CSTAT2_SPRITE_NOSHADOW; // No shadows on actors
				actor->vel.X = -16;
				ssp(actor, CLIPMASK0);
			}
			else if(actor->spr.statnum == STAT_PROJECTILE && lz.actor()->isPlayer() && actor->GetOwner() == actor)
			{
				actor->ceilingz = actor->sector()->ceilingz;
				actor->floorz = actor->sector()->floorz;
			}
		}
	}
	else
	{
		actor->ceilingz = actor->sector()->ceilingz;
		actor->floorz = actor->sector()->floorz;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void makeitfall(DDukeActor* actor)
{
	double grav;

	if( fi.floorspace(actor->sector()) )
		grav = 0;
	else
	{
		if( fi.ceilingspace(actor->sector()) || actor->sector()->lotag == ST_2_UNDERWATER)
			grav = gs.gravity/6;
		else grav = gs.gravity;
	}

	if (isRRRA())
	{
		grav = adjustfall(actor, grav); // this accesses sprite indices and cannot be in shared code. Should be done better. (todo: turn into actor flags)
	}

	if ((actor->spr.statnum == STAT_ACTOR || actor->spr.statnum == STAT_PLAYER || actor->spr.statnum == STAT_ZOMBIEACTOR || actor->spr.statnum == STAT_STANDABLE))
	{
		Collision coll;
		getzrange(actor->spr.pos.plusZ(-1), actor->sector(), &actor->ceilingz, coll, &actor->floorz, coll, 7.9375, CLIPMASK0);
	}
	else
	{
		actor->ceilingz = actor->sector()->ceilingz;
		actor->floorz = actor->sector()->floorz;
	}

	if( actor->spr.pos.Z < actor->floorz - FOURSLEIGHT_F)
	{
		if( actor->sector()->lotag == 2 && actor->vel.Z > 3122/256.)
			actor->vel.Z = 3144 / 256.;
		if (actor->vel.Z < 24)
			actor->vel.Z += grav;
		else actor->vel.Z = 24;
		actor->spr.pos.Z += actor->vel.Z;
	}
	if (actor->spr.pos.Z >= actor->floorz - FOURSLEIGHT_F)
	{
 		actor->spr.pos.Z = actor->floorz - FOURSLEIGHT_F;
		actor->vel.Z = 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int dodge(DDukeActor* actor)
{
	auto oldpos = actor->spr.pos.XY();

	DukeStatIterator it(STAT_PROJECTILE);
	while (auto ac = it.Next())
	{
		if (ac->GetOwner() == ac || ac->sector() != actor->sector())
			continue;

		auto delta = ac->spr.pos.XY() - oldpos;
		auto bvect = ac->spr.angle.ToVector() * 1024;

		if (actor->spr.angle.ToVector().dot(delta) >= 0)
		{
			if (bvect.dot(delta) < 0)
			{
				double d = bvect.X * delta.Y - bvect.Y * delta.X;
				if (abs(d) < 256 * 64)
				{
					actor->spr.angle -= DAngle90 + randomAngle(180);
					return 1;
				}
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

DAngle furthestangle(DDukeActor *actor, int angs)
{
	double d, greatestd;
	DAngle furthest_angle = DAngle360;
	HitInfo hit{};

	greatestd = -(1 << 30);
	DAngle angincs = DAngle360 / angs;

	if (!actor->isPlayer())
		if ((actor->temp_data[0] & 63) > 2) return(actor->spr.angle + DAngle180);

	for (DAngle j = actor->spr.angle; j < DAngle360 + actor->spr.angle; j += angincs)
	{
		hitscan(actor->spr.pos.plusZ(-8), actor->sector(), DVector3(j.ToVector() * 1024, 0), hit, CLIPMASK1);

		d = (hit.hitpos.XY() - actor->spr.pos.XY()).Sum();

		if (d > greatestd)
		{
			greatestd = d;
			furthest_angle = j;
		}
	}
	return furthest_angle.Normalized360();
}

//---------------------------------------------------------------------------
//
// return value was changed to what its only caller really expects
//
//---------------------------------------------------------------------------

int furthestcanseepoint(DDukeActor *actor, DDukeActor* tosee, DVector2& pos)
{
	DAngle angincs;
	HitInfo hit{};

	if ((actor->temp_data[0] & 63)) return -1;

	if (ud.multimode < 2 && ud.player_skill < 3)
		angincs = DAngle180;
	else angincs = DAngle360 / (1 + (krand() & 1));

	for (auto j = tosee->spr.angle; j < tosee->spr.angle + DAngle360; j += (angincs - randomAngle(90)))
	{
		hitscan(tosee->spr.pos.plusZ(-16), tosee->sector(), DVector3(j.ToVector() * 1024, 64 - krandf(128)), hit, CLIPMASK1);

		double d = (hit.hitpos.XY() - tosee->spr.pos.XY()).Sum();
		double da = (hit.hitpos.XY() - actor->spr.pos.XY()).Sum();

		if (d < da && hit.hitSector)
			if (cansee(hit.hitpos, hit.hitSector, actor->spr.pos.plusZ(-16), actor->sector()))
			{
				pos = hit.hitpos.XY();
				return 1;
			}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void alterang(int ang, DDukeActor* actor, int playernum)
{
	DAngle goalang, aang, angdif;
	int j;
	int ticselapsed;

	auto moveptr = &ScriptCode[actor->temp_data[1]];

	ticselapsed = (actor->temp_data[0]) & 31;

	aang = actor->spr.angle;

	actor->vel.X += (moveptr[0] / 16 - actor->vel.X) / 5;
	if (actor->vel.Z < (648 / 256.))
	{
		actor->vel.Z += (moveptr[1] / 16 - actor->vel.Z) / 5;
	}

	if (isRRRA() && (ang & windang))
		actor->spr.angle = WindDir;
	else if (ang & seekplayer)
	{
		DDukeActor* holoduke = !isRR()? ps[playernum].holoduke_on.Get() : nullptr;

		// NOTE: looks like 'Owner' is set to target sprite ID...

		if (holoduke && cansee(holoduke->spr.pos, holoduke->sector(), actor->spr.pos, actor->sector()))
			actor->SetOwner(holoduke);
		else actor->SetOwner(ps[playernum].GetActor());

		auto Owner = actor->GetOwner();
		if (Owner->isPlayer())
			goalang = (actor->ovel - actor->spr.pos.XY()).Angle();
		else
			goalang = (Owner->spr.pos.XY() - actor->spr.pos.XY()).Angle();

		if (actor->vel.X != 0 && actor->spr.picnum != TILE_DRONE)
		{
			angdif = absangle(aang, goalang);

			if (ticselapsed < 2)
			{
				if (angdif < DAngle45)
				{
					DAngle add = DAngle22_5 * ((krand() & 256)? 1 : -1);
					actor->spr.angle += add;
					if (hits(actor) < 51.25)
						actor->spr.angle -= add;
				}
			}
			else if (ticselapsed > 18 && ticselapsed < 26) // choose
			{
				if (angdif < DAngle90) actor->spr.angle = goalang;
				else actor->spr.angle += angdif * 0.25;
			}
		}
		else actor->spr.angle = goalang;
	}

	if (ticselapsed < 1)
	{
		j = 2;
		if (ang & furthestdir)
		{
			goalang = furthestangle(actor, j);
			actor->spr.angle = goalang;
			actor->SetOwner(ps[playernum].GetActor());
		}

		if (ang & fleeenemy)
		{
			goalang = furthestangle(actor, j);
			actor->spr.angle = goalang;
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
	actor->spr.xoffset = 0;
	actor->spr.yoffset = 0;
	//			  if(!gotz)
	{
		double grav;

		int sphit = fallspecial? fallspecial(actor, playernum) : 0;
		if (fi.floorspace(actor->sector()))
			grav = 0;
		else
		{
			if (fi.ceilingspace(actor->sector()) || actor->sector()->lotag == 2)
				grav = gs.gravity / 6;
			else grav = gs.gravity;
		}

		if (actor->cgg <= 0 || (actor->sector()->floorstat & CSTAT_SECTOR_SLOPE))
		{
			getglobalz(actor);
			actor->cgg = 6;
		}
		else actor->cgg--;

		if (actor->spr.pos.Z < actor->floorz - FOURSLEIGHT_F)
		{
			actor->vel.Z += grav;
			actor->spr.pos.Z += actor->vel.Z;

			if (actor->vel.Z > 24) actor->vel.Z = 24;
		}
		else
		{
			actor->spr.pos.Z = actor->floorz - FOURSLEIGHT_F;

			if (badguy(actor) || (actor->isPlayer() && actor->GetOwner()))
			{

				if (actor->vel.Z > (3084/256.) && actor->spr.extra <= 1)
				{
					if (actor->spr.pal != 1 && actor->spr.picnum != DRONE)
					{
						if (actor->isPlayer() && actor->spr.extra > 0)
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

					actor->attackertype = SHOTSPARK1;
					actor->hitextra = 1;
					actor->vel.Z = 0;
				}
				else if (actor->vel.Z > 8 && actor->sector()->lotag != 1)
				{

					auto sect = actor->sector();
					pushmove(actor->spr.pos, &sect, 8., 4., 4., CLIPMASK0);
					if (sect != actor->sector() && sect != nullptr)
						ChangeActorSect(actor, sect);

					S_PlayActorSound(thud, actor);
				}
			}
			if (actor->sector()->lotag == 1)
				actor->spr.pos.Z += gs.actorinfo[actor->spr.picnum].falladjustz;
			else actor->vel.Z = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DDukeActor *LocateTheLocator(int n, sectortype* sect)
{
	DukeStatIterator it(STAT_LOCATOR);
	while (auto ac = it.Next())
	{
		if ((sect == nullptr || sect == ac->sector()) && n == ac->spr.lotag)
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
			ac->backuploc();
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------


void movefta(void)
{
	double xx;
	int canseeme, p;
	sectortype* psect, * ssect;

	auto check_fta_sounds = [](DDukeActor* act)
	{
		if (isRR()) check_fta_sounds_r(act);
		else check_fta_sounds_d(act);
	};

	DukeStatIterator it(STAT_ZOMBIEACTOR);
	while (auto act = it.Next())
	{
		p = findplayer(act, &xx);
		canseeme = 0;

		ssect = psect = act->sector();

		if (ps[p].GetActor()->spr.extra > 0)
		{
			if (xx < 30000 / 16.)
			{
				act->timetosleep++;
				if (act->timetosleep >= int(xx / 16.))
				{
					if (badguy(act))
					{
						auto xyrand = []() -> double { return (64 - (krand() & 127)) * maptoworld; };
						double px = ps[p].opos.X - xyrand();
						double py = ps[p].opos.Y - xyrand();
						updatesector(DVector3(px, py, 0), &psect);
						if (psect == nullptr)
						{
							continue;
						}
						double sx = act->spr.pos.X - xyrand();
						double sy = act->spr.pos.Y - xyrand();
						// The second updatesector call here used px and py again and was redundant as coded.

						// SFLAG_MOVEFTA_CHECKSEE is set for all actors in Duke.
						if (act->spr.pal == 33 || actorflag(act, SFLAG_MOVEFTA_CHECKSEE) ||
							(actorflag(act, SFLAG_MOVEFTA_CHECKSEEWITHPAL8) && act->spr.pal == 8) ||
							(act->spr.angle.Cos() * (px - sx) + act->spr.angle.Sin() * (py - sy) >= 0))
						{
							double r1 = zrand(32);
							double r2 = zrand(52);
							canseeme = cansee({ sx, sy, act->spr.pos.Z - r2 }, act->sector(), { px, py, ps[p].opos.Z - r1 }, ps[p].cursector);
						}
					}
					else
					{
						int r1 = krand();
						int r2 = krand();
						canseeme = cansee(act->spr.pos.plusZ(-(r2 & 31)), act->sector(), ps[p].opos.plusZ(-(r1 & 31)), ps[p].cursector);
					}


					if (canseeme)
					{
						if (actorflag(act, SFLAG_MOVEFTA_MAKESTANDABLE))
						{
							if (act->sector()->ceilingstat & CSTAT_SECTOR_SKY)
								act->spr.shade = act->sector()->ceilingshade;
							else act->spr.shade = act->sector()->floorshade;

							act->timetosleep = 0;
							ChangeActorStat(act, STAT_STANDABLE);
						}
						else
						{
							act->timetosleep = 0;
							check_fta_sounds(act);
							ChangeActorStat(act, STAT_ACTOR);
						}
					}
					else act->timetosleep = 0;
				}
			}
			if (badguy(act))
			{
				if (act->sector()->ceilingstat & CSTAT_SECTOR_SKY)
					act->spr.shade = act->sector()->ceilingshade;
				else act->spr.shade = act->sector()->floorshade;

				// wakeup is an RR feature, this flag will later allow it to use in Duke, too.
				if (actorflag(act, SFLAG_MOVEFTA_WAKEUPCHECK))
				{
					if (wakeup(act, p))
					{
						act->timetosleep = 0;
						check_fta_sounds(act);
						ChangeActorStat(act, STAT_ACTOR);
					}
				}
			}
		}
	}
}


END_DUKE_NS
