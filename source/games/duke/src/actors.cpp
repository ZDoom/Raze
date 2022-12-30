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
#include "vm.h"
#include "global.h"
#include "names.h"
#include "stats.h"
#include "constants.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

double adjustfall(DDukeActor* actor, double c);


//---------------------------------------------------------------------------
//
// this is the implementation of DDukeActor::Tick. It is native so that
// its internally needed accesss does not have to be made public.
//
//---------------------------------------------------------------------------

void TickActor(DDukeActor* self)
{
	if (self->spr.statnum == STAT_ACTOR || self->spr.statnum == STAT_PLAYER || (self->flags3 & SFLAG3_FORCERUNCON))
	{
		double pdist;
		int p;
		
		if (self->spr.statnum != STAT_PLAYER)
		{
			p = findplayer(self, &pdist);
		}
		else
		{
			if (ud.multimode > 1)
				p = findotherplayer(self->PlayerIndex(), &pdist);
			else
			{
				p = self->PlayerIndex();
				pdist = 0;
			}
		}

		// Run sprite animations.
		if (self->curAction->name != NAME_None)
		{
			// This code was utterly cryptic in the original source.
			int numframes = self->curAction->numframes;
			int increment = self->curAction->increment;
			int delay = self->curAction->delay;

			self->spr.lotag += TICSPERFRAME;
			if (self->spr.lotag > delay)
			{
				self->actioncounter++;
				self->spr.lotag = 0;
				self->curframe += increment;
			}
			if (abs(self->curframe) >= abs(numframes * increment))
				self->curframe = 0;
		}

		if (!execute(self, p, pdist))
		{
			self->state_player = &ps[p];
			self->state_dist = pdist;
			IFVIRTUALPTR(self, DDukeActor, RunState)
			{
				VMValue val[] = { self };
				VMCall(func, val, 1, nullptr, 0);
			}
			self->state_player = nullptr;
			self->state_dist = -1;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void respawnhitag(DDukeActor* actor)
{
	if (actor->flags2 & SFLAG2_TRIGGERRESPAWN)
	{
		if (actor->spr.yint) operaterespawns(actor->spr.yint);
	}
	else
	{
		if (actor->spr.hitag >= 0) operaterespawns(actor->spr.hitag);
	}
}

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

	auto spawned = CreateActor(origin->sector(), origin->spr.pos + offset, DukeScrapClass, -8, DVector2(v, v), a, vel, zvel, origin, STAT_MISC);
	if (spawned)
	{
		spawned->spriteextra = (r4 & 15);
	}
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
			fi.addweapon(player, weap, true);
			return;
		}
	}

	weap = player->curr_weapon;
	if (player->gotweapon[weap])
	{
		if (player->ammo_amount[weap] > 0 || (WeaponSwitch(player - ps) & 2) == 0)
			return;
	}

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
	ps->GetActor()->restoreloc();
	updatesector(ps->GetActor()->getPosWithOffsetZ(), &ps->cursector);

	DukeStatIterator it(STAT_ACTOR);
	while (auto k = it.Next())
	{
		if (k->flags2 & SFLAG2_CAMERA)
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

	return movesprite_ex(actor, DVector3(actor->spr.Angles.Yaw.ToVector() * actor->vel.X, actor->vel.Z), cliptype, c) == kHitNone;
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
			spriteq[spriteqloc]->Destroy();
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

void lotsofstuff(DDukeActor* actor, int n, PClassActor* spawntype)
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
	actor->spr.pos.XY() += actor->spr.Angles.Yaw.ToVector() * actor->vel.X;

	for(auto& wal : actor->sector()->walls)
	{
		dragpoint(&wal, actor->spr.pos.XY() + mspos[msindex].Rotated(rotation));
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
	for (int q = cyclers.Size() - 1; q >= 0; q--)
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
			for (auto& wal : sect->walls)
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

void addcycler(sectortype* sector, int lotag, int shade, int shade2, int hitag, int state)
{
	cyclers.Reserve(1);
	cyclers.Last().sector = sector;
	cyclers.Last().lotag = lotag;
	cyclers.Last().shade1 = shade;
	cyclers.Last().shade2 = shade2;
	cyclers.Last().hitag = hitag;
	cyclers.Last().state = state;
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
			act->Destroy();
			continue;
		}
		else
		{
			if (ps[p].on_ground && ps[p].on_warping_sector == 1 && ps[p].cursector->lotag == 1)
			{
				act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
				act->spr.pos.Z = act->sector()->ceilingz + 27;
				act->spr.Angles.Yaw = ps[p].GetActor()->spr.Angles.Yaw;
				if (act->counter == 8)
					act->counter = 0;
				else act->counter++;
			}
			else
			{
				if (act->sector()->lotag != 2) act->spr.pos.Z = act->sector()->floorz;
				act->spr.cstat = CSTAT_SPRITE_INVISIBLE;
			}
		}

		act->spr.pos.XY() += ps[p].GetActor()->spr.pos.XY() - ps[p].GetActor()->opos.XY();
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
				act->restorepos();
				act->backupz();
				act->spr.Angles.Yaw = p->GetActor()->PrevAngles.Yaw;
				SetActor(act, act->spr.pos);
			}
			else
			{
				TickActor(act);
				if (ud.multimode > 1)
				{
					otherp = findotherplayer(pn, &other);
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
					p->GetActor()->spr.Angles.Yaw += deltaangle(p->GetActor()->spr.Angles.Yaw, (p->actorsqu->spr.pos.XY() - p->GetActor()->spr.pos.XY()).Angle()) * 0.25;
				}

				if (act->spr.extra > 0)
				{
					// currently alive...

					act->SetHitOwner(act);

					if (ud.god == 0)
						if (ceilingspace(act->sector()) || floorspace(act->sector()))
							quickkill(p);
				}
				else
				{
					p->GetActor()->spr.pos.Z += 20;
					p->newOwner = nullptr;

					if (p->wackedbyactor != nullptr && p->wackedbyactor->spr.statnum < MAXSTATUS)
					{
						p->GetActor()->spr.Angles.Yaw += deltaangle(p->GetActor()->spr.Angles.Yaw, (p->wackedbyactor->spr.pos.XY() - p->GetActor()->spr.pos.XY()).Angle()) * 0.5;
					}
				}
			}
		}
		else
		{
			if (p->holoduke_on == nullptr)
			{
				act->Destroy();
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
				act->spr.scale.Y += (0.0625);
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
				act->spr.extra++;
				ssp(act, CLIPMASK0);
			}
			else
			{
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

void movefallers(void)
{
	DukeStatIterator iti(STAT_FALLER);
	while (auto act = iti.Next())
	{
		auto sectp = act->sector();

		if (act->counter == 0)
		{
			act->spr.pos.Z -= 16;
			DAngle saved_angle = act->spr.Angles.Yaw;
			int x = act->spr.extra;
			int j = fi.ifhitbyweapon(act);
			if (j >= 0)
			{
				if (j == 2) // explosive
				{
					if (act->spr.extra <= 0)
					{
						act->counter = 1;
						DukeStatIterator itj(STAT_FALLER);
						while (auto a2 = itj.Next())
						{
							if (a2->spr.hitag == act->spr.hitag)
							{
								a2->counter = 1;
								a2->spr.cstat &= ~CSTAT_SPRITE_ONE_SIDE;
								if (a2->IsKindOf(DukeSteamBaseClass))
									a2->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
							}
						}
					}
				}
				else
				{
					act->hitextra = 0;
					act->spr.extra = x;
				}
			}
			act->spr.Angles.Yaw = saved_angle;
			act->spr.pos.Z += 16;
		}
		else if (act->counter == 1)
		{
			if (act->spr.lotag > 0)
			{
				act->spr.lotag -= 3;
				if (act->spr.lotag <= 0 || isRR())
				{
					act->vel.X = 2 + krandf(4);
					act->vel.Z = -4 + krandf(4);
				}
			}
			else
			{
				if (act->vel.X > 0)
				{
					act->vel.X -= 0.5;
					ssp(act, CLIPMASK0);
				}

				double grav;
				if (floorspace(act->sector())) grav = 0;
				else
				{
					if (ceilingspace(act->sector()))
						grav = gs.gravity / 6;
					else
						grav = gs.gravity;
				}

				if (act->spr.pos.Z < sectp->floorz - 1)
				{
					act->vel.Z += grav;
					if (act->vel.Z > 24)
						act->vel.Z = 24;
					act->spr.pos.Z += act->vel.Z;
				}
				if ((sectp->floorz - act->spr.pos.Z) < 16)
				{
					int j = 1 + (krand() & 7);
					for (int x = 0; x < j; x++) RANDOMSCRAP(act);
					act->Destroy();
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

void tickstat(int stat, bool deleteinvalid)
{
	DukeStatIterator iti(stat);
	while (auto act = iti.Next())
	{
		if ((act->flags2 & SFLAG2_DIENOW) || act->sector() == nullptr || (deleteinvalid && act->spr.scale.X == 0))
		{
			addkill(act);
			act->Destroy();
		}
		else if (stat != STAT_ACTOR || !badguy(act) || !monsterCheatCheck(act))
		{
			CallTick(act);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operaterespawns(int low)
{
	DukeStatIterator it(STAT_FX);
	while (auto act = it.Next())
	{
		CallOnRespawn(act, low);
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void detonate(DDukeActor *actor, PClassActor* explosion)
{
	ud.earthquaketime = 16;

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
				if (effector->counter == 0)
					effector->counter = 1;
			}
			else if (effector->spr.lotag == SE_21_DROP_FLOOR)
				effector->counter = 1;
		}
	}

	actor->spr.pos.Z -= 32;

	if ((actor->temp_data[3] == 1 && actor->spr.scale.X != 0) || actor->spr.lotag == -99)
	{
		int x = actor->spr.extra;
		spawn(actor, explosion);
		hitradius(actor, gs.seenineblastradius, x >> 2, x - (x >> 1), x - (x >> 2), x);
		S_PlayActorSound(PIPEBOMB_EXPLODE, actor);
	}

	if (actor->spr.scale.X != 0)
		for (int x = 0; x < 8; x++) RANDOMSCRAP(actor);

	actor->Destroy();

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void gutsdir(DDukeActor* actor, PClassActor* gtype, int n, int p)
{
	double scale;

	if (badguy(actor) && actor->spr.scale.X < 0.25)
		scale = 0.125;
	else scale = 0.5;

	double gutz = actor->spr.pos.Z - 8;
	double floorz = getflorzofslopeptr(actor->sector(), actor->spr.pos);

	if (gutz > floorz - 8)
		gutz = floorz - 8;

	gutz += actor->IntVar(NAME_gutsoffset);

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
// 
//
//---------------------------------------------------------------------------

void blastceiling(DDukeActor* actor, double radius)
{
	BFSSectorSearch search(actor->sector());

	while (auto dasectp = search.GetNext())
	{
		if ((dasectp->ceilingz - actor->spr.pos.Z) < radius * 16) // what value range is this supposed to be? The check that was here did not multiply correctly
		{
			auto wal = dasectp->walls.Data();
			double d = (wal->pos - actor->spr.pos.XY()).Sum();
			if (d < radius)
				checkhitceiling(dasectp);
			else
			{
				auto thirdpoint = wal->point2Wall()->point2Wall();
				d = (thirdpoint->pos - actor->spr.pos.XY()).Sum();
				if (d < radius)
					checkhitceiling(dasectp);
			}
		}

		for (auto& wal : dasectp->walls)
		{
			if ((wal.pos - actor->spr.pos.XY()).Sum() < radius)
			{
				if (wal.twoSided())
				{
					search.Add(wal.nextSector());
				}
				DVector3 w1(((wal.pos + wal.point2Wall()->pos) * 0.5 + actor->spr.pos) * 0.5, actor->spr.pos.Z); // half way between the actor and the wall's center.
				sectortype* sect = wal.sectorp();
				updatesector(w1, &sect);

				if (sect && cansee(w1, sect, actor->spr.pos, actor->sector()))
					checkhitwall(actor, &wal, DVector3(wal.pos, actor->spr.pos.Z));
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void hitradius(DDukeActor* actor, int  r, int  hp1, int  hp2, int  hp3, int  hp4)
{
	double radius = r * inttoworld;
	static const uint8_t statlist[] = { STAT_DEFAULT, STAT_ACTOR, STAT_STANDABLE, STAT_PLAYER, STAT_FALLER, STAT_ZOMBIEACTOR, STAT_MISC };

	if (!(actor->flags3 & SFLAG3_NOCEILINGBLAST))
	{
		blastceiling(actor, radius);
	}

	double q = zrand(32) - (isRR() ? 24 : 16);

	auto Owner = actor->GetOwner();
	for (int x = 0; x < 7; x++)
	{
		DukeStatIterator itj(statlist[x]);
		while (auto act2 = itj.Next())
		{
			if (Owner)
			{
				if (Owner->isPlayer() && act2->isPlayer() && ud.coop != 0 && ud.ffire == 0 && Owner != act2 /* && (dmflags & NOFRIENDLYRADIUSDMG)*/)
				{
					continue;
				}

				if (actor->flags3 & SFLAG3_HITRADIUS_DONTHURTSPECIES && !Owner->isPlayer() && Owner->GetClass() == act2->GetClass())
				{
					continue;
				}
			}

			if (x == 0 || x >= 5 || (act2->flags1 & SFLAG_HITRADIUS_CHECKHITONLY))
			{
				if (!(actor->flags3 & SFLAG3_HITRADIUS_NODAMAGE) || (act2->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
					if ((actor->spr.pos - act2->spr.pos).Length() < radius)
					{
						if (badguy(act2) && !cansee(act2->spr.pos.plusZ(q), act2->sector(), actor->spr.pos.plusZ(q), actor->sector()))
							continue;
						checkhitsprite(act2, actor);
					}
			}
			else if (act2->spr.extra >= 0 && act2 != actor && ((act2->flags1 & SFLAG_HITRADIUS_FORCEEFFECT) || badguy(act2) || (act2->spr.cstat & CSTAT_SPRITE_BLOCK_ALL)))
			{
				// this should be a damage type check, not a projectile type check.
				// It's also quite broken because it doesn't check for being shrunk but tries to guess it from the size.
				// Unfortunately, with CON there is no way to retrieve proper shrunk state in any way.
				if (actor->GetClass() == DukeShrinkSparkClass && (act2->spr.scale.X < 0.375)) 
				{
					continue;
				}
				if (actor->flags3 & SFLAG3_HITRADIUS_DONTHURTSHOOTER && act2 == Owner)
				{
					continue;
				}
				if (actor->flags3 & SFLAG3_HITRADIUS_NOEFFECT)
				{
					continue;
				}

				double dist = (act2->getPosWithOffsetZ() - actor->spr.pos).Length();

				if (dist < radius && cansee(act2->spr.pos.plusZ(-8), act2->sector(), actor->spr.pos.plusZ(-12), actor->sector()))
				{
					act2->hitang = (act2->spr.pos - actor->spr.pos).Angle();
					act2->attackertype = CallGetRadiusDamageType(actor, act2->spr.extra);

					if (!(actor->flags3 & SFLAG3_HITRADIUS_NODAMAGE))
					{
						if (dist < radius / 3)
						{
							if (hp4 == hp3) hp4++;
							act2->hitextra = hp3 + (krand() % (hp4 - hp3));
						}
						else if (dist < 2 * radius / 3)
						{
							if (hp3 == hp2) hp3++;
							act2->hitextra = hp2 + (krand() % (hp3 - hp2));
						}
						else if (dist < radius)
						{
							if (hp2 == hp1) hp2++;
							act2->hitextra = hp1 + (krand() % (hp2 - hp1));
						}

						if (!(act2->flags2 & SFLAG2_NORADIUSPUSH) && !bossguy(act2))
						{
							if (act2->vel.X < 0) act2->vel.X = 0;
							act2->vel.X += ((actor->spr.extra / 4.));
						}

						if ((act2->flags1 & SFLAG_HITRADIUSCHECK))
							checkhitsprite(act2, actor);
					}
					else if (actor->spr.extra == 0) act2->hitextra = 0;

					if (act2->GetClass() != DukeRadiusExplosionClass && Owner && Owner->spr.statnum < MAXSTATUS)
					{
						if (act2->isPlayer())
						{
							int p = act2->PlayerIndex();

							if (act2->attackertype == DukeFlamethrowerFlameClass && Owner->isPlayer())
							{
								ps[p].numloogs = -1 - actor->spr.yint;
							}

							if (ps[p].newOwner != nullptr)
							{
								clearcamera(&ps[p]);
							}
						}
						act2->SetHitOwner(actor->GetOwner());
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


int movesprite_ex(DDukeActor* actor, const DVector3& change, unsigned int cliptype, Collision& result)
{
	int bg = badguy(actor);

	if (actor->spr.statnum == STAT_MISC || (bg && actor->spr.scale.X < 0.0625))
	{
		actor->spr.pos += change;
		if (bg)
			SetActor(actor, actor->spr.pos);
		return result.setNone();
	}

	auto dasectp = actor->sector();

	auto ppos = actor->spr.pos;

	auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
	ppos.Z -= tex->GetDisplayHeight() * actor->spr.scale.Y * 0.5;

	if (bg)
	{
		if (actor->spr.scale.X > 0.9375)
			clipmove(ppos, &dasectp, change * 0.5, 64., 4., 4., cliptype, result);
		else
		{
			// todo: move this mess to the actor definitions once we have them all available.
			double clipdist = actor->FloatVar(NAME_moveclipdist);
			if (clipdist == 0)
			{
				if ((actor->flags1 & SFLAG_BADGUY) && !isRR())
					clipdist = actor->clipdist;
				else
					clipdist = 12;
			}

			clipmove(ppos, &dasectp, change * 0.5, clipdist, 4., 4., cliptype, result);
		}

		// conditional code from hell...
		if (dasectp == nullptr || (dasectp != nullptr &&
			((actor->actorstayput != nullptr && actor->actorstayput != dasectp) ||
				((actor->flags3 & SFLAG3_ST3CONFINED) && actor->spr.pal == 0 && dasectp->lotag != ST_3_BOSS2) ||
				((actor->flags3 & SFLAG3_DONTENTERWATER) && dasectp->lotag == ST_1_ABOVE_WATER) ||
				((actor->flags3 & SFLAG3_DONTENTERWATERONGROUND) && actor->vel.Z == 0 && dasectp->lotag == ST_1_ABOVE_WATER))
			)
			)
		{
			if (dasectp && dasectp->lotag == ST_1_ABOVE_WATER && (actor->flags3 & SFLAG3_RANDOMANGLEONWATER))
				actor->spr.Angles.Yaw = randomAngle();
			else if ((actor->counter & 3) == 1 && !(actor->flags3 & SFLAG3_NORANDOMANGLEWHENBLOCKED))
				actor->spr.Angles.Yaw = randomAngle();
			SetActor(actor, actor->spr.pos);
			if (dasectp == nullptr) dasectp = &sector[0];
			return result.setSector(dasectp);
		}
		if ((result.type == kHitWall || result.type == kHitSprite) && (actor->cgg == 0)) actor->spr.Angles.Yaw += DAngle90 + DAngle45;
	}
	else
	{
		if (actor->spr.statnum == STAT_PROJECTILE)
			clipmove(ppos, &dasectp, change * 0.5, 0.5, 4., 4., cliptype, result);
		else
			clipmove(ppos, &dasectp, change * 0.5, actor->clipdist, 4., 4., cliptype, result);
	}
	actor->spr.pos.XY() = ppos.XY();

	if (dasectp != nullptr && dasectp != actor->sector())
		ChangeActorSect(actor, dasectp);

	double daz = actor->spr.pos.Z + change.Z * 0.5;
	if (daz > actor->ceilingz && daz <= actor->floorz)
		actor->spr.pos.Z = daz;
	else if (result.type == kHitNone)
		return result.setSector(dasectp);

	return result.type;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void move(DDukeActor* actor, int pnum, double pdist)
{
	DAngle goalang, angdif;
	double daxvel;

	int a = actor->spr.hitag;

	if (a == -1) a = 0;

	actor->counter++;

	if (a & face_player)
	{
		if (ps[pnum].newOwner != nullptr)
			goalang = (ps[pnum].GetActor()->opos.XY() - actor->spr.pos.XY()).Angle();
		else goalang = (ps[pnum].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Angle();
		angdif = deltaangle(actor->spr.Angles.Yaw, goalang) * 0.25;
		if (angdif > -DAngle22_5 / 16 && angdif < nullAngle) angdif = nullAngle;
		actor->spr.Angles.Yaw += angdif;
	}

	if (a & spin)
		actor->spr.Angles.Yaw += DAngle45 * BobVal(actor->counter << 3);

	if (a & face_player_slow)
	{
		if (ps[pnum].newOwner != nullptr)
			goalang = (ps[pnum].GetActor()->opos.XY() - actor->spr.pos.XY()).Angle();
		else goalang = (ps[pnum].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Angle();
		angdif = DAngle22_5 * 0.25 * Sgn(deltaangle(actor->spr.Angles.Yaw, goalang).Degrees()); // this looks very wrong...
		actor->spr.Angles.Yaw += angdif;
	}

	if (a & antifaceplayerslow)
	{
		if (ps[pnum].newOwner != nullptr)
			goalang = ((ps[pnum].GetActor()->opos.XY() - actor->spr.pos.XY()).Angle() + DAngle180);
		else goalang = ((ps[pnum].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Angle() + DAngle180);
		angdif = DAngle22_5 * 0.25 * Sgn(deltaangle(actor->spr.Angles.Yaw, goalang).Degrees()); // this looks very wrong...
		actor->spr.Angles.Yaw += angdif;
	}

	if ((a & jumptoplayer) == jumptoplayer)
	{
		if (actor->counter < 16)
			actor->vel.Z -= BobVal(512 + (actor->counter << 4)) * actor->FloatVar(NAME_jumptoplayer_factor);
	}
	else if (a & justjump1)
	{
		if (actor->counter < 8)
			actor->vel.Z -= BobVal(512 + (actor->counter << 4)) * actor->FloatVar(NAME_justjump1_factor);
	}
	if (a & justjump2)
	{
		if (actor->counter < 8)
			actor->vel.Z -= BobVal(512 + (actor->counter << 4)) * actor->FloatVar(NAME_justjump2_factor);
	}
	if (a & windang)
	{
		if (actor->counter < 8)
			actor->vel.Z -= BobVal(512 + (actor->counter << 4)) * actor->FloatVar(NAME_windang_factor);
	}


	if (a & face_player_smart)
	{
		DVector2 newpos = ps[pnum].GetActor()->spr.pos.XY() + (ps[pnum].vel.XY() * (4. / 3.));
		goalang = (newpos - actor->spr.pos.XY()).Angle();
		angdif = deltaangle(actor->spr.Angles.Yaw, goalang) * 0.25;
		if (angdif > -DAngle22_5 / 16 && angdif < nullAngle) angdif = nullAngle;
		actor->spr.Angles.Yaw += angdif;
	}

	if (actor->curMove->name == NAME_None || a == 0)
	{
		if ((badguy(actor) && actor->spr.extra <= 0) || (actor->opos.X != actor->spr.pos.X) || (actor->opos.Y != actor->spr.pos.Y))
		{
			if (!actor->isPlayer()) actor->backupvec2();
			SetActor(actor, actor->spr.pos);
		}
		if (badguy(actor) && actor->spr.extra <= 0)
		{
			if (actor->sector()->ceilingstat & CSTAT_SECTOR_SKY)
			{
				if (actor->sector()->shadedsector == 1)
				{
					actor->spr.shade += (16 - actor->spr.shade) >> 1;
				}
				else
				{
					actor->spr.shade += (actor->sector()->ceilingshade - actor->spr.shade) >> 1;
				}
			}
			else
			{
				actor->spr.shade += (actor->sector()->floorshade - actor->spr.shade) >> 1;
			}
		}
		return;
	}

	if (a & geth) actor->vel.X += (actor->curMove->movex - actor->vel.X) * 0.5;
	if (a & getv) actor->vel.Z += (actor->curMove->movez - actor->vel.Z) * 0.5;

	if (a & dodgebullet)
		dodge(actor);

	if (!actor->isPlayer())
		alterang(a, actor, pnum);

	if (abs(actor->vel.X) < 6 / 16.) actor->vel.X = 0;

	a = badguy(actor);

	if (actor->vel.X != 0 || actor->vel.Z != 0)
	{
		if (a && !(actor->flags3 & SFLAG3_NOVERTICALMOVE))
		{
			if ((actor->flags2 & SFLAG2_FLOATING) && actor->spr.extra > 0)
			{
				double fdist = actor->FloatVar(NAME_floating_floordist);
				double cdist = actor->FloatVar(NAME_floating_ceilingdist);
				double c, f;
				calcSlope(actor->sector(), actor->spr.pos.X, actor->spr.pos.Y, &c, &f);
				actor->floorz = f;
				actor->ceilingz = c;

				if (actor->spr.pos.Z > f - fdist)
				{
					actor->spr.pos.Z = f - fdist;
					actor->vel.Z = 0;
				}

				if (actor->spr.pos.Z < c + cdist)
				{
					actor->spr.pos.Z = c + cdist;
					actor->vel.Z = 0;
				}
			}
			else
			{
				if (actor->vel.Z > 0 && actor->floorz < actor->spr.pos.Z)
					actor->spr.pos.Z = actor->floorz;
				if (actor->vel.Z < 0)
				{
					double c = getceilzofslopeptr(actor->sector(), actor->spr.pos.X, actor->spr.pos.Y);
					if (actor->spr.pos.Z < c + 66)
					{
						actor->spr.pos.Z = c + 66;
						actor->vel.Z *= 0.5;
					}
				}
			}
		}

		daxvel = actor->vel.X;
		angdif = actor->spr.Angles.Yaw;

		if (a && !(actor->flags3 & SFLAG3_MOVE_NOPLAYERINTERACT))
		{
			if (pdist < 60 && actor->spr.scale.X > 0.25)
			{

				daxvel = -(64 - pdist);
				angdif = (ps[pnum].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Angle();

				if (pdist < 32)
				{
					ps[pnum].vel.X = 0;
					ps[pnum].vel.Y = 0;
				}
				else
				{
					ps[pnum].vel.XY() *= gs.playerfriction - 0.125;
				}
			}
			else if (!(actor->flags2 & SFLAG2_FLOATING))
			{
				if (actor->curMove->movez == 0)
				{
					if (actor->opos.Z != actor->spr.pos.Z || (ud.multimode < 2 && ud.player_skill < 2))
					{
						if ((actor->counter & 1) || ps[pnum].actorsqu == actor) return;
						else daxvel *= 2;
					}
					else
					{
						if ((actor->counter & 3) || ps[pnum].actorsqu == actor) return;
						else daxvel *= 4;
					}
				}
			}
		}
		if (actor->sector()->lotag != ST_1_ABOVE_WATER)
		{
			daxvel *= actor->FloatVar(NAME_landmovefactor);
		}
		else
		{
			daxvel *= actor->FloatVar(NAME_watermovefactor);
		}

		Collision coll;
		actor->movflag = movesprite_ex(actor, DVector3(angdif.ToVector() * daxvel, actor->vel.Z), CLIPMASK0, coll);
	}

	if (a)
	{
		if (actor->sector()->ceilingstat & CSTAT_SECTOR_SKY)
		{
			if (actor->sector()->shadedsector == 1)
			{
				actor->spr.shade += (16 - actor->spr.shade) >> 1;
			}
			else
			{
				actor->spr.shade += (actor->sector()->ceilingshade - actor->spr.shade) >> 1;
			}
		}
		else actor->spr.shade += (actor->sector()->floorshade - actor->spr.shade) >> 1;

		if (actor->sector()->floortexture == mirrortex)
			actor->Destroy();
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
		actor->Destroy();
		return;
	}

	DAngle ang_amount = DAngle::fromQ16(sect->extra << 2);
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
				if (actor->spr.detail) direction = 1;
				else direction = -1;
			}
			else actor->tempval = 256;

			if (sect->floorz > actor->spr.pos.Z) //z's are touching
			{
				sect->addfloorz(-2);
				zchange =  -2;
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
				if (actor->spr.detail) direction = -1;
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

		actor->spr.Angles.Yaw += ang_amount * direction;
		actor->temp_angle += ang_amount * direction;
	}
	else
	{
		if (Owner->counter == 0) return;
		if (Owner->counter == 2)
		{
			actor->Destroy();
			return;
		}

		if (Owner->spr.Angles.Yaw.Normalized360() > DAngle180)
			direction = -1;
		else direction = 1;
		if (actor->temp_pos.Y == 0)
			actor->temp_pos.Y = (actor->spr.pos.XY() - Owner->spr.pos.XY()).Length();
		actor->vel.X = actor->temp_pos.Y;
		actor->spr.pos.XY() = Owner->spr.pos.XY();
		actor->spr.Angles.Yaw += ang_amount * direction;
		actor->temp_angle += ang_amount * direction;
	}

	if (direction && (sect->floorstat & CSTAT_SECTOR_ALIGN))
	{
		int p;
		for (p = connecthead; p >= 0; p = connectpoint2[p])
		{
			if (ps[p].cursector == actor->sector() && ps[p].on_ground == 1)
			{
				ps[p].GetActor()->spr.Angles.Yaw += ang_amount * direction;

				ps[p].GetActor()->spr.pos.Z += zchange;

				auto result = rotatepoint(Owner->spr.pos, ps[p].GetActor()->spr.pos.XY(), ang_amount * direction);

				ps[p].bobpos += (result - ps[p].GetActor()->spr.pos.XY());

				ps[p].GetActor()->spr.pos.XY() = result;
			}
		}
		DukeSectIterator itp(actor->sector());
		while (auto act2 = itp.Next())
		{
			if (act2->spr.statnum != STAT_EFFECTOR && act2->spr.statnum != STAT_PROJECTILE && !(act2->flags2 & SFLAG2_NOROTATEWITHSECTOR))
			{
				if (act2->isPlayer() && act2->GetOwner())
				{
					continue;
				}

				act2->spr.Angles.Yaw += ang_amount * direction;
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
				actor->counter = 0;
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

void handle_se14(DDukeActor* actor, bool checkstat, PClassActor* RPG)
{
	auto sc = actor->sector();
	int st = actor->spr.lotag;

	if (actor->GetOwner() == nullptr)
	{
		auto NewOwner = LocateTheLocator(actor->temp_data[3], &sector[actor->counter]);

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
		auto NewOwner = LocateTheLocator(actor->temp_data[3], &sector[actor->counter]);
		if (NewOwner == nullptr)
		{
			actor->temp_data[3] = 0;
			NewOwner = LocateTheLocator(0, &sector[actor->counter]);
		}
		if (NewOwner) actor->SetOwner(NewOwner);
	}

	Owner = actor->GetOwner();
	if(actor->vel.X != 0)
	{
		auto curangle = (Owner->spr.pos.XY() - actor->spr.pos.XY()).Angle();
		auto diffangle = deltaangle(actor->spr.Angles.Yaw, curangle) * 0.125;

		actor->temp_angle += diffangle;
		actor->spr.Angles.Yaw += diffangle;

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
					auto saved_angle = actor->spr.Angles.Yaw;
					actor->spr.Angles.Yaw = (actor->spr.pos.XY() - ps[p].GetActor()->spr.pos.XY()).Angle();
					shoot(actor, RPG);
					actor->spr.Angles.Yaw = saved_angle;
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
						updatesector(ps[p].GetActor()->getPosWithOffsetZ(), &sect);
						if ((sect == nullptr && ud.clipping == 0) || (sect == actor->sector() && ps[p].cursector != actor->sector()))
						{
							ps[p].GetActor()->spr.pos.XY() = actor->spr.pos.XY();
							ps[p].setCursector(actor->sector());

							SetActor(ps[p].GetActor(), actor->spr.pos);
							quickkill(&ps[p]);
						}
					}
				}
		}

		auto vec = actor->spr.Angles.Yaw.ToVector() * actor->vel.X;

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
					auto result = rotatepoint(actor->spr.pos.XY(), ps[p].GetActor()->spr.pos.XY(), diffangle);

					ps[p].GetActor()->spr.pos.XY() = result + vec;

					ps[p].bobpos += vec;

					ps[p].GetActor()->spr.Angles.Yaw += diffangle;

					if (numplayers > 1)
					{
						ps[p].GetActor()->backupvec2();
					}
					if (psp->spr.extra <= 0)
					{
						psp->spr.pos.XY() = ps[p].GetActor()->spr.pos.XY();
					}
				}
			}
		}
		DukeSectIterator it(actor->sector());
		while (auto a2 = it.Next())
		{
			if (a2->spr.statnum != STAT_PLAYER && a2->sector()->lotag != 2 && 
				(!iseffector(a2) || a2->spr.lotag == SE_49_POINT_LIGHT || a2->spr.lotag == SE_50_SPOT_LIGHT) &&
				!islocator(a2))
			{
				a2->spr.pos.XY() = rotatepoint(actor->spr.pos.XY(), a2->spr.pos.XY(), diffangle) + vec;
				a2->spr.Angles.Yaw += diffangle;

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
						updatesector(ps[p].GetActor()->getPosWithOffsetZ(), &k);
						if ((k == nullptr && ud.clipping == 0) || (k == actor->sector() && ps[p].cursector != actor->sector()))
						{
							ps[p].GetActor()->spr.pos.XY() = actor->spr.pos.XY();
							ps[p].GetActor()->backupvec2();
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
					if (a2->spr.statnum == 1 && badguy(a2) && !iseffector(a2) && !islocator(a2))
					{
						auto k = a2->sector();
						updatesector(a2->spr.pos, &k);
						if (a2->spr.extra >= 0 && k == actor->sector())
						{
							gutsdir(a2, DukeJibs6Class, 72, myconnectindex);
							S_PlayActorSound(SQUISHED, actor);
							a2->Destroy();
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

void handle_se30(DDukeActor *actor)
{
	auto sc = actor->sector();

	auto Owner = actor->GetOwner();
	if (Owner == nullptr)
	{
		actor->temp_data[3] = !actor->temp_data[3];
		Owner = LocateTheLocator(actor->temp_data[3], &sector[actor->counter]);
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
					operateactivators(actor->spr.hitag + (!actor->temp_data[3]), nullptr);
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
				operateactivators(actor->spr.hitag + (short)actor->temp_data[3], nullptr);
				actor->SetOwner(nullptr);
				actor->spr.Angles.Yaw += DAngle180;
				actor->temp_data[4] = 0;
				operateforcefields(actor, actor->spr.hitag);
			}
		}
	}

	if(actor->vel.X != 0)
	{
		auto vect = actor->spr.Angles.Yaw.ToVector() * actor->vel.X;

		if ((sc->floorz - sc->ceilingz) < 108)
			if (ud.clipping == 0)
				for (int p = connecthead; p >= 0; p = connectpoint2[p])
					{
					auto psp = ps[p].GetActor();
					if (psp->spr.extra > 0)
					{
						auto k = ps[p].cursector;
						updatesector(ps[p].GetActor()->getPosWithOffsetZ(), &k);
						if ((k == nullptr && ud.clipping == 0) || (k == actor->sector() && ps[p].cursector != actor->sector()))
						{
							ps[p].GetActor()->spr.pos.XY() = actor->spr.pos.XY();
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
				ps[p].GetActor()->spr.pos.XY() += vect;

				if (numplayers > 1)
				{
					ps[p].GetActor()->backupvec2();
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
			if (!iseffector(a2) && !islocator(a2) && !a2->isPlayer())
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
						updatesector(ps[p].GetActor()->getPosWithOffsetZ(), &k);
						if ((k == nullptr && ud.clipping == 0) || (k == actor->sector() && ps[p].cursector != actor->sector()))
						{
							ps[p].GetActor()->spr.pos.XY() = actor->spr.pos.XY();
							ps[p].GetActor()->backupvec2();

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
					if (a2->spr.statnum == STAT_ACTOR && badguy(a2) && !iseffector(a2) && !islocator(a2))
					{
						//					if(a2->spr.sector != actor->spr.sector)
						{
							auto k = a2->sector();
							updatesector(a2->spr.pos, &k);
							if (a2->spr.extra >= 0 && k == actor->sector())
							{
								gutsdir(a2, DukeJibs6Class, 24, myconnectindex);
								S_PlayActorSound(SQUISHED, a2);
								a2->Destroy();
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

	if (actor->temp_data[4] > 0 && actor->counter == 0)
	{
		if (actor->temp_data[4] < sh)
			actor->temp_data[4]++;
		else actor->counter = 1;
	}

	if (actor->counter > 0)
	{
		actor->counter++;

		actor->vel.X = 3 / 16.;

		if (actor->counter > 96)
		{
			actor->counter = -1; //Stop the quake
			actor->temp_data[4] = -1;
			actor->Destroy();
			return;
		}
		else
		{
			if ((actor->counter & 31) == 8)
			{
				ud.earthquaketime = 48;
				S_PlayActorSound(EARTHQUAKE, ps[screenpeek].GetActor());
			}

			if (abs(sc->floorheinum - actor->temp_data[0]) < 8)
				sc->setfloorslope(actor->temp_data[0]);
			else sc->setfloorslope(sc->getfloorslope() + (Sgn(actor->temp_data[0] - sc->getfloorslope()) << 4));
		}

		auto vect = actor->spr.Angles.Yaw.ToVector() * actor->vel.X;

		for (int p = connecthead; p >= 0; p = connectpoint2[p])
			if (ps[p].cursector == actor->sector() && ps[p].on_ground)
			{
				ps[p].GetActor()->spr.pos.XY() += vect;
				ps[p].bobpos += vect;
			}

		DukeSectIterator it(actor->sector());
		while (auto a2 = it.Next())
		{
			if (!iseffector(a2))
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
		actor->counter = actor->spr.shade + (global_random & 15);
	}
	else
	{
		sc->ceilingpal = actor->spr.pal;
		sc->floorpal = actor->spr.pal;
		actor->counter = actor->temp_data[3];
	}

	sc->ceilingshade = actor->counter;
	sc->floorshade = actor->counter;

	for(auto& wal : sc->walls)
	{
		if (wal.hitag != 1)
		{
			wal.shade = actor->counter;
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
		actor->counter = actor->spr.shade + (global_random & 15);
		sc->ceilingpal = palvals >> 8;
		sc->floorpal = palvals & 0xff;
		j = 1;
	}
	else
	{
		actor->temp_data[1] = actor->temp_data[2];
		actor->counter = actor->temp_data[3];

		sc->ceilingpal = actor->spr.pal;
		sc->floorpal = actor->spr.pal;

		j = 0;
	}

	sc->floorshade = actor->temp_data[1];
	sc->ceilingshade = actor->temp_data[1];

	for (auto& wal : sc->walls)
	{
		if (j) wal.pal = (palvals & 0xff);
		else wal.pal = actor->spr.pal;

		if (wal.hitag != 1)
		{
			wal.shade = actor->counter;
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
		actor->Destroy();

}

//---------------------------------------------------------------------------
//
// boss
//
//---------------------------------------------------------------------------

void handle_se05(DDukeActor* actor)
{
	auto sc = actor->sector();
	int j;

	double x;
	int p = findplayer(actor, &x);
	if (x < 512)
	{
		auto ang = actor->spr.Angles.Yaw;
		actor->spr.Angles.Yaw = (actor->spr.pos.XY() - ps[p].GetActor()->spr.pos.XY()).Angle();
		shoot(actor, isRR()? RedneckFirelaserClass : DukeFirelaserClass);
		actor->spr.Angles.Yaw = ang;
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
	auto angdiff = deltaangle(actor->spr.Angles.Yaw, ang) / 8;
	actor->spr.Angles.Yaw += angdiff;

	if (rnd(32))
	{
		actor->temp_angle += angdiff;
		sc->ceilingshade = 127;
	}
	else
	{
		actor->temp_angle +=
			deltaangle(actor->temp_angle + DAngle90, (ps[p].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Angle()) * 0.25;
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
	sector[actor->counter].setceilingz(actor->vel.Z);
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
			actor->Destroy();
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

				for (auto& wal : sect->walls)
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
				else if (sect->floorshade > ac->counter)
					sect->floorshade = ac->counter;

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
			if (actor->counter > sh)
			{
				if (specialtags) for (int i = 0; specialtags[i]; i++)
				{
					if (actor->sector()->lotag == specialtags[i] && getanimationindex(anim_ceilingz, actor->sector()) >= 0)
					{
						return;
					}
				}
				fi.activatebysector(actor->sector(), actor);
				actor->counter = 0;
			}
			else actor->counter++;
		}
	}
	else actor->counter = 0;
}

//---------------------------------------------------------------------------
//
// swinging door
//
//---------------------------------------------------------------------------

void handle_se11(DDukeActor *actor)
{
	auto sc = actor->sector();
	if (actor->temp_data[0] > 0)
	{
		actor->temp_data[0]--;
		return;
	}

	if (actor->temp_data[4])
	{
		for(auto& wal : sc->walls)
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

		for(auto& wal : sc->walls)
		{
			DukeStatIterator it(STAT_PLAYER);
			while (auto ac = it.Next())
			{
				if (ac->GetOwner() && IsCloseToWall(ac->spr.pos.XY(), &wal, 9) == EClose::InFront)
				{
					actor->temp_data[0] = 8; // Delay
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
	if (actor->counter == 3 || actor->temp_data[3] == 1) //Lights going off
	{
		sc->floorpal = 0;
		sc->ceilingpal = 0;

		for (auto& wal : sc->walls)
		{
			if (wal.hitag != 1)
			{
				wal.shade = actor->temp_data[1];
				wal.pal = 0;
			}
		}
		sc->floorshade = actor->temp_data[1];
		sc->ceilingshade = actor->temp_data[2];
		actor->counter = 0;

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
			actor->Destroy();
			return;
		}
	}
	if (actor->counter == 1) //Lights flickering on
	{
		// planeonly 1 is RRRA SE47, planeonly 2 is SE48
		int compshade = planeonly == 2 ? sc->ceilingshade : sc->floorshade;
		if (compshade > actor->spr.shade)
		{
			if (planeonly != 2) sc->floorpal = actor->spr.pal;
			if (planeonly != 1) sc->ceilingpal = actor->spr.pal;

			if (planeonly != 2) sc->floorshade -= 2;
			if (planeonly != 1) sc->ceilingshade -= 2;

			for (auto& wal : sc->walls)
			{
				if (wal.hitag != 1)
				{
					wal.pal = actor->spr.pal;
					wal.shade -= 2;
				}
			}
		}
		else actor->counter = 2;

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
				for (auto& wal : sc->walls)
					wal.shade = actor->spr.shade;

				sc->floorshade = actor->spr.shade;

				if (ps[0].one_parallax_sectnum != nullptr)
				{
					sc->setceilingtexture(ps[0].one_parallax_sectnum->ceilingtexture);
					sc->ceilingshade = ps[0].one_parallax_sectnum->ceilingshade;
				}
			}
		}
		actor->temp_data[2]++;
		if (actor->temp_data[2] > 256)
		{
			actor->Destroy();
			return;
		}
	}


	if (actor->temp_data[2] == 4 && actor->spr.Angles.Yaw != DAngle90)
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

void handle_se16(DDukeActor* actor)
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
			if (a2->IsKindOf(DukeReactorClass) && a2->spritesetindex == 0)
				return;
		}
		if (a2 == nullptr)
		{
			actor->Destroy();
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

	double q = actor->counter * refheight * 4;

	sc->addceilingz(q);
	sc->addfloorz(q);

	DukeSectIterator it(actor->sector());
	while (auto act1 = it.Next())
	{
		if (act1->spr.statnum == STAT_PLAYER && act1->GetOwner())
		{
			int p = act1->spr.yint;
			ps[p].GetActor()->spr.pos.Z += q;
			ps[p].truefz += q;
			ps[p].truecz += q;
		}
		if (act1->spr.statnum != STAT_EFFECTOR && act1->spr.statnum != STAT_PLAYER)
		{
			act1->spr.pos.Z += q;
		}

		act1->floorz = sc->floorz;
		act1->ceilingz = sc->ceilingz;
	}

	if (actor->counter) //If in motion
	{
		if (abs(sc->floorz - actor->temp_pos.X) <= refheight)
		{
			activatewarpelevators(actor, 0);
			return;
		}

		if (actor->counter == -1)
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
				if ((sc->hitag - actor->counter) == (act2->sector()->hitag) && sh == (act2->spr.hitag))
					break;
		}

		if (act2 == nullptr) return;

		DukeSectIterator its(actor->sector());
		while (auto act3 = its.Next())
		{
			if (act3->spr.statnum == STAT_PLAYER && act3->GetOwner())
			{
				int p = act3->PlayerIndex();

				act3->opos -= act3->spr.pos;
				act3->spr.pos.XY() += act2->spr.pos.XY() - actor->spr.pos.XY();
				act3->spr.pos.Z += act2->sector()->floorz - sc->floorz;
				act3->opos += act3->spr.pos;

				if (q > 0) ps[p].GetActor()->backupz();

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
	if (actor->counter)
	{
		if (actor->spr.pal)
		{
			if (actor->spr.intangle == 512)
			{
				sc->addceilingz(-extra);
				if (sc->ceilingz <= goal)
				{
					sc->setceilingz(goal);
					actor->Destroy();
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
							if (ps[a2->PlayerIndex()].on_ground == 1) ps[a2->PlayerIndex()].GetActor()->spr.pos.Z += extra;
						}
						if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && a2->spr.statnum != STAT_PROJECTILE)
						{
							if (!a2->isPlayer()) a2->spr.pos.Z += extra;
							a2->floorz = sc->floorz;
						}
					}
				}
				if (sc->floorz >= goal)
				{
					sc->setfloorz(goal);
					actor->Destroy();
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
					actor->Destroy();
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
							if (ps[a2->PlayerIndex()].on_ground == 1) ps[a2->PlayerIndex()].GetActor()->spr.pos.Z -= extra;
						}
						if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && a2->spr.statnum != STAT_PROJECTILE)
						{
							if (!a2->isPlayer()) a2->spr.pos.Z -= extra;
							a2->floorz = sc->floorz;
						}
					}
				}
				if (sc->floorz <= actor->spr.pos.Z)
				{
					sc->setfloorz(actor->spr.pos.Z);
					actor->Destroy();
					return;
				}
			}
		}

		actor->temp_data[2]++;
		if (actor->temp_data[2] >= actor->spr.hitag)
		{
			actor->temp_data[2] = 0;
			actor->counter = 0;
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
		if ((a1->flags1 & SFLAG_TRIGGER_IFHITSECTOR) && sect == a1->sector())
			return a1;
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se19(DDukeActor *actor)
{
	auto sc = actor->sector();
	int sh = actor->spr.hitag;

	if (actor->counter)
	{
		if (actor->counter == 1)
		{
			actor->counter++;
			for (auto& wal : sc->walls)
			{
				if (tileflags(wal.overtexture) & TFLAG_FORCEFIELD)
				{
					wal.cstat &= (CSTAT_WALL_TRANSLUCENT | CSTAT_WALL_1WAY | CSTAT_WALL_XFLIP | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_BOTTOM_SWAP);
					wal.setovertexture(FNullTextureID());
					auto nextwal = wal.nextWall();
					if (nextwal != nullptr)
					{
						nextwal->setovertexture(FNullTextureID());
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
					a2Owner->counter = 2;
				}
			}
			actor->Destroy();
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
						if (ac->counter == 0)
						{
							ac->counter = 1; //Shut them all on
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

	if (actor->counter == 0) return;

	//if(actor->vel.X != 0) //Moving
	{
		auto vec = actor->spr.Angles.Yaw.ToVector() * (actor->counter == 1 ? 0.5 : -0.5);

		actor->temp_data[3] += actor->counter == 1? 8 :- 8;

		actor->spr.pos += vec;

		if (actor->temp_data[3] <= 0 || (actor->temp_data[3] >> 6) >= (actor->spr.yint >> 6))
		{
			actor->spr.pos -= vec;
			actor->counter = 0;
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
				ps[p].GetActor()->spr.pos.XY() += vec;
				ps[p].GetActor()->backupvec2();

				SetActor(ps[p].GetActor(), ps[p].GetActor()->spr.pos);
			}

		sc->addfloorxpan(-(float)vec.X * 2);
		sc->addfloorypan(-(float)vec.Y * 2);

		sc->addceilingxpan(-(float)vec.X * 2);
		sc->addceilingypan(-(float)vec.Y * 2);
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

	if (actor->counter == 0) return;

	if (actor->spr.intangle == 1536)
		lp = sc->ceilingz;
	else
		lp = sc->floorz;

	if (actor->counter == 1) //Decide if the sector should go up or down
	{
		actor->vel.Z = (Sgn(actor->spr.pos.Z - lp) * (actor->spr.yint << 4) * zmaptoworld);
		actor->counter++;
	}

	if (sc->extra == 0)
	{
		lp += actor->vel.Z;

		if (abs(lp - actor->spr.pos.Z) < 4)
		{
			lp = actor->spr.pos.Z;
			actor->Destroy();
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
		if (getanimationindex(anim_ceilingz, &sector[actor->counter]) >= 0)
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
	DVector2 vect = 2 * actor->spr.Angles.Yaw.ToVector(); // was: (32 * b sin) >> 14

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
			ps[p].GetActor()->spr.pos.Z += zvel;
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

	actor->temp_angle = actor->spr.Angles.Yaw;

	p = findplayer(actor, &xx);
	if (ps[p].GetActor()->spr.extra > 0 && myconnectindex == screenpeek)
	{
		if (actor->counter < 0)
		{
			ud.cameraactor = actor;
			actor->counter++;
		}
		else if (ud.recstat == 2 && ps[p].newOwner == nullptr)
		{
			if (cansee(actor->spr.pos, actor->sector(), ps[p].GetActor()->getPosWithOffsetZ(), ps[p].cursector))
			{
				if (xx < sh * maptoworld)
				{
					ud.cameraactor = actor;
					actor->counter = 999;
					actor->spr.Angles.Yaw += deltaangle(actor->spr.Angles.Yaw, (ps[p].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Angle()) * 0.125;
					actor->spr.yint = 100 + int((actor->spr.pos.Z - ps[p].GetActor()->getOffsetZ()) * (256. / 257.));

				}
				else if (actor->counter == 999)
				{
					if (ud.cameraactor == actor)
						actor->counter = 0;
					else actor->counter = -10;
					ud.cameraactor = actor;

				}
			}
			else
			{
				actor->spr.Angles.Yaw = (ps[p].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Angle();

				if (actor->counter == 999)
				{
					if (ud.cameraactor == actor)
						actor->counter = 0;
					else actor->counter = -20;
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

	auto vec = actor->spr.Angles.Yaw.ToVector() * actor->spr.yint / 256.;

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
				if (a2->flags1 & SFLAG_SE24_REMOVE)
				{
					a2->spr.scale = DVector2(0, 0);
					continue;
				}

				if ((a2->flags1 & SFLAG_SE24_NOCARRY) || wallswitchcheck(a2) || GetExtInfo(a2->spr.spritetexture()).switchindex > 0)
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
			if (abs(ps[p].GetActor()->getOffsetZ() - ps[p].truefz) < gs.playerheight + 9)
			{
				ps[p].fric += vec * (1. / 8.); // keeping the original velocity. to match the animation it should be ~1/24.
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
	auto add = actor->spr.yint * (1 / 16.);

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
			if (ud.pistonsound && snd1 >= 0)
				S_PlayActorSound(snd1, actor);
		}
	}
	else
	{
		sec->addceilingz(-add);
		if (sec->ceilingz < actor->temp_pos.Z)
		{
			sec->setceilingz(actor->temp_pos.Z);
			if (ud.pistonsound && snd2 >= 0)
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

	if (actor->counter == 1)
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
					actor->counter = 0;
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
					actor->counter = 0;
				}
				else sc->addceilingz(Sgn(actor->temp_pos.Z - sc->ceilingz) * targetval);
			}
			return;
		}

		if ((actor->spr.intangle & 2047) == 1536)
		{
			if (abs(sc->ceilingz - actor->spr.pos.Z) < targetval * 2)
			{
				actor->counter = 0;
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
				actor->counter = 0;
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

void handle_se35(DDukeActor *actor)
{
	auto sc = actor->sector();

	if (sc->ceilingz > actor->spr.pos.Z)
		for (int j = 0; j < 8; j++)
		{
			actor->spr.Angles.Yaw = randomAngle(90);
			auto spawned = spawn(actor, DukeSmallSmokeClass);
			if (spawned)
			{
				spawned->vel.X = 6 + krandf(8);
				ssp(spawned, CLIPMASK0);
				SetActor(spawned, spawned->spr.pos);
				if (rnd(16))
					spawn(actor, DukeExplosion2Class);
			}
		}


	double targetval = actor->spr.yint * zmaptoworld;
	switch (actor->counter)
	{
	case 0:
		sc->addceilingz(targetval);
		if (sc->ceilingz > sc->floorz)
			sc->setfloorz(sc->ceilingz);
		if (sc->ceilingz > actor->spr.pos.Z + 32)
			actor->counter++;
		break;
	case 1:
		sc->addceilingz(-targetval * 4);
		if (sc->ceilingz < actor->temp_pos.Y)
		{
			sc->setceilingz(actor->temp_pos.Y);
			actor->counter = 0;
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se36(DDukeActor* actor)
{
	if (actor->counter)
	{
		if (actor->counter == 1)
			shoot(actor, GetSpawnType(actor->sector()->extra));
		else if (actor->counter == 26 * 5)
			actor->counter = 0;
		actor->counter++;
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

	auto data = breakWallMap.CheckKey(wal->overtexture.GetIndex());
	FTextureID newtex = data? data->brokentex : FNullTextureID();
	wal->setovertexture(newtex);
	auto nextwal = wal->nextWall();
	if (nextwal)
		nextwal->setovertexture(newtex);

	if (actor->counter < actor->temp_data[1]) actor->counter++;
	else
	{
		wal->cstat &= (CSTAT_WALL_TRANSLUCENT | CSTAT_WALL_1WAY | CSTAT_WALL_XFLIP | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_BOTTOM_SWAP);
		if (nextwal)
			nextwal->cstat &= (CSTAT_WALL_TRANSLUCENT | CSTAT_WALL_1WAY | CSTAT_WALL_XFLIP | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_BOTTOM_SWAP);
		actor->Destroy();
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se130(DDukeActor *actor, int countmax)
{
	auto sc = actor->sector();

	if (actor->counter > countmax)
	{
		actor->Destroy();
		return;
	}
	else actor->counter++;

	double x = sc->floorz - sc->ceilingz;

	if (rnd(64))
	{
		auto k = spawn(actor, DukeExplosion2Class);
		if (k)
		{
			double s = 0.03125 + (krand() & 7) * REPEAT_SCALE;
			k->spr.scale = DVector2(s, s);
			k->spr.pos.Z = sc->floorz + krandf(x);
			k->spr.Angles.Yaw += DAngle45 - randomAngle(90);
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

	if (actor->counter == 1)
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
					actor->counter = 0;
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
								ps[a2->PlayerIndex()].GetActor()->spr.pos.Z += l;
						if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && (!choosedir || a2->spr.statnum != STAT_PROJECTILE))
						{
							if (!a2->isPlayer()) a2->spr.pos.Z += l;
							a2->floorz = sec->floorz;
						}
					}
				}
			}
			else
			{
				if (abs(sec->floorz - actor->temp_pos.Y) < actor->temp_pos.Z)
				{
					sec->setfloorz(actor->temp_pos.Y);
					callsound(actor->sector(), actor);
					actor->temp_data[2] = 0;
					actor->counter = 0;
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
								ps[a2->PlayerIndex()].GetActor()->spr.pos.Z += l;
						if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && (!choosedir || a2->spr.statnum != STAT_PROJECTILE))
						{
							if (!a2->isPlayer()) a2->spr.pos.Z += l;
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
				actor->counter = 0;
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
							ps[a2->PlayerIndex()].GetActor()->spr.pos.Z += l;
					if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && (!choosedir || a2->spr.statnum != STAT_PROJECTILE))
					{
						if (!a2->isPlayer()) a2->spr.pos.Z += l;
						a2->floorz = sec->floorz;
					}
				}
			}
		}
		else
		{
			if (abs(sec->floorz - actor->temp_pos.Y) < actor->temp_pos.Z)
			{
				actor->counter = 0;
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
							ps[a2->PlayerIndex()].GetActor()->spr.pos.Z -= l;
					if (a2->vel.Z == 0 && a2->spr.statnum != STAT_EFFECTOR && (!choosedir || a2->spr.statnum != STAT_PROJECTILE))
					{
						if (!a2->isPlayer()) a2->spr.pos.Z -= l;
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
	if (actor->flags3 & SFLAG3_NOGRAVITY) return;

	double grav;

	if( floorspace(actor->sector()) )
		grav = 0;
	else
	{
		if( ceilingspace(actor->sector()) || actor->sector()->lotag == ST_2_UNDERWATER)
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
		if( actor->sector()->lotag == ST_2_UNDERWATER && actor->vel.Z > 3122/256.)
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
		auto bvect = ac->spr.Angles.Yaw.ToVector() * 1024;

		if (actor->spr.Angles.Yaw.ToVector().dot(delta) >= 0)
		{
			if (bvect.dot(delta) < 0)
			{
				double d = bvect.X * delta.Y - bvect.Y * delta.X;
				if (abs(d) < 256 * 64)
				{
					actor->spr.Angles.Yaw -= DAngle90 + randomAngle(180);
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
		if ((actor->counter & 63) > 2) return(actor->spr.Angles.Yaw + DAngle180);

	for (DAngle j = actor->spr.Angles.Yaw; j < DAngle360 + actor->spr.Angles.Yaw; j += angincs)
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

	if ((actor->counter & 63)) return -1;

	if (ud.multimode < 2 && ud.player_skill < 3)
		angincs = DAngle180;
	else angincs = DAngle360 / (1 + (krand() & 1));

	for (auto j = tosee->spr.Angles.Yaw; j < tosee->spr.Angles.Yaw + DAngle360; j += (angincs - randomAngle(90)))
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

	ticselapsed = (actor->counter) & 31;

	aang = actor->spr.Angles.Yaw;

	actor->vel.X += (actor->curMove->movex - actor->vel.X) / 5;
	if (actor->vel.Z < (648 / 256.))
	{
		actor->vel.Z += (actor->curMove->movez - actor->vel.Z) / 5;
	}

	if (isRRRA() && (ang & windang))
		actor->spr.Angles.Yaw = WindDir;
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

		if (actor->vel.X != 0 && !(actor->flags3 & SFLAG3_QUICKALTERANG))
		{
			angdif = deltaangle(aang, goalang);

			if (ticselapsed < 2)
			{
				if (abs(angdif) < DAngle45)
				{
					DAngle add = DAngle22_5 - randomAngle(DAngle45);
					actor->spr.Angles.Yaw += add;
					if (hits(actor) < 52.75)
						actor->spr.Angles.Yaw -= add;
				}
			}
			else if (ticselapsed > 18 && ticselapsed < 26) // choose
			{
				if (abs(angdif) < DAngle90) actor->spr.Angles.Yaw = goalang;
				else actor->spr.Angles.Yaw += angdif * 0.25;
			}
		}
		else actor->spr.Angles.Yaw = goalang;
	}

	if (ticselapsed < 1)
	{
		j = 2;
		if (ang & furthestdir)
		{
			goalang = furthestangle(actor, j);
			actor->spr.Angles.Yaw = goalang;
			actor->SetOwner(ps[playernum].GetActor());
		}

		if (ang & fleeenemy)
		{
			goalang = furthestangle(actor, j);
			actor->spr.Angles.Yaw = goalang;
		}
	}
}

//---------------------------------------------------------------------------
//
// the indirections here are to keep this core function free of game references
//
//---------------------------------------------------------------------------

void fall_common(DDukeActor *actor, int playernum, int(*fallspecial)(DDukeActor*, int))
{
	actor->spr.xoffset = 0;
	actor->spr.yoffset = 0;
	//			  if(!gotz)
	{
		double grav;

		int sphit = fallspecial? fallspecial(actor, playernum) : 0;
		if (floorspace(actor->sector()))
			grav = 0;
		else
		{
			if (ceilingspace(actor->sector()) || actor->sector()->lotag == 2)
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
					if (actor->spr.pal != 1 && !(actor->flags3 & SFLAG3_NOJIBS))
					{
						if (actor->isPlayer() && actor->spr.extra > 0)
							goto SKIPJIBS;
						if (sphit)
						{
							spawnguts(actor, DukeJibs6Class, 5);
							S_PlayActorSound(SQUISHED, actor);
						}
						else
						{
							spawnguts(actor, DukeJibs6Class, 15);
							S_PlayActorSound(SQUISHED, actor);
							spawn(actor, DukeBloodPoolClass);
						}
					}

				SKIPJIBS:

					actor->attackertype = DukeShotSparkClass;
					actor->hitextra = 1;
					actor->vel.Z = 0;
				}
				else if (actor->vel.Z > 8 && actor->sector()->lotag != 1)
				{

					auto sect = actor->sector();
					pushmove(actor->spr.pos, &sect, 8., 4., 4., CLIPMASK0);
					if (sect != actor->sector() && sect != nullptr)
						ChangeActorSect(actor, sect);

					S_PlayActorSound(THUD, actor);
				}
			}
			if (actor->sector()->lotag == 1)
				actor->spr.pos.Z += actor->IntVar(NAME_falladjustz);
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


void movefta(void)
{
	double xx;
	int canseeme, p;
	sectortype* psect, * ssect;

	auto check_fta_sounds = [](DDukeActor* act)
	{
		if (act->spr.extra > 0)
			CallPlayFTASound(act);
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
						double px = ps[p].GetActor()->opos.X - xyrand();
						double py = ps[p].GetActor()->opos.Y - xyrand();
						updatesector(DVector3(px, py, 0), &psect);
						if (psect == nullptr)
						{
							continue;
						}
						double sx = act->spr.pos.X - xyrand();
						double sy = act->spr.pos.Y - xyrand();
						// The second updatesector call here used px and py again and was redundant as coded.

						// SFLAG_LOOKALLAROUND is set for all actors in Duke.
						if (act->spr.pal == 33 || (act->flags1 & SFLAG_LOOKALLAROUND) ||
							((act->flags1 & SFLAG_LOOKALLAROUNDWITHPAL8) && act->spr.pal == 8) ||
							(act->spr.Angles.Yaw.Cos() * (px - sx) + act->spr.Angles.Yaw.Sin() * (py - sy) >= 0))
						{
							double r1 = zrand(32);
							double r2 = zrand(52);
							canseeme = cansee({ sx, sy, act->spr.pos.Z - r2 }, act->sector(), { px, py, ps[p].GetActor()->getPrevOffsetZ() - r1 }, ps[p].cursector);
						}
					}
					else
					{
						int r1 = krand();
						int r2 = krand();
						canseeme = cansee(act->spr.pos.plusZ(-(r2 & 31)), act->sector(), ps[p].GetActor()->getPrevPosWithOffsetZ().plusZ(-(r1 & 31)), ps[p].cursector);
					}


					if (canseeme)
					{
						if ((act->flags1 & SFLAG_MOVEFTA_MAKESTANDABLE))
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

				// wakeup is an RR feature, this flag will allow it to use in Duke, too.
				if ((act->flags1 & SFLAG_MOVEFTA_WAKEUPCHECK))
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void actorsizeto(DDukeActor* actor, double x, double y)
{
	// JBF 20030805: As I understand it, if repeat becomes 0 it basically kills the
	// sprite, which is why the "sizeto 0 41" calls in 1.3d became "sizeto 4 41" in
	// 1.4, so instead of patching the CONs I'll surruptitiously patch the code here
	//if (!isPlutoPak() && *insptr == 0) *insptr = 4;

	double siz = (x - actor->spr.scale.X);
	actor->spr.scale.X = (clamp(actor->spr.scale.X + Sgn(siz) * REPEAT_SCALE, 0., 4.));

	auto scale = actor->spr.scale.Y;
	auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
	if ((actor->isPlayer() && scale < 0.5626) || y < scale || (scale * (tex->GetDisplayHeight() + 8)) < actor->floorz - actor->ceilingz)
	{
		siz = (y - actor->spr.scale.Y);
		actor->spr.scale.Y = (clamp(actor->spr.scale.Y + Sgn(siz) * REPEAT_SCALE, 0., 4.));
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void spawndebris(DDukeActor* g_ac, int dnum, int count)
{
	if (dnum < 0 || dnum >= ScrapMax) return;	// this code only works with scrap and nothing else.
	bool weap = (g_ac->flags3 & SFLAG3_SPAWNWEAPONDEBRIS);

	if (g_ac->insector())
		for (int j = count; j >= 0; j--)
		{
			int s;
			if (weap)
				s = 0;
			else s = (krand() % 3);
			DVector3 offs;
			offs.X = krandf(16) - 8;
			offs.Y = krandf(16) - 8;
			offs.Z = -krandf(16) - 8;

			auto a = randomAngle();
			auto vel = krandf(8) + 2;
			auto zvel = -krandf(8);
			DVector2 scale(0.5 + (krand() & 15) * REPEAT_SCALE, 0.5 + (krand() & 15) * REPEAT_SCALE);

			auto spawned = CreateActor(g_ac->sector(), g_ac->spr.pos + offs, DukeScrapClass, g_ac->spr.shade, scale, a, vel, zvel, g_ac, STAT_MISC);
			if (spawned)
			{
				spawned->spriteextra = dnum + s;
				if (weap)
					spawned->spr.yint = (j % 15) + 1;
				else spawned->spr.yint = -1;
				spawned->spr.pal = g_ac->spr.pal;
			}
		}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void actoroperate(DDukeActor* g_ac)
{
	if (g_ac->sector()->lotag == 0)
	{
		HitInfo hit{};
		neartag(g_ac->spr.pos.plusZ(-32), g_ac->sector(), g_ac->spr.Angles.Yaw, hit, 48, NT_Lotag | NT_NoSpriteCheck);
		auto sectp = hit.hitSector;
		if (sectp)
		{
			if (isanearoperator(sectp->lotag))
				if ((sectp->lotag & 0xff) == ST_23_SWINGING_DOOR || sectp->floorz == sectp->ceilingz)
					if ((sectp->lotag & 16384) == 0 && (sectp->lotag & 32768) == 0)
					{
						DukeSectIterator it(sectp);
						DDukeActor* a2;
						while ((a2 = it.Next()))
						{
							if (isactivator(a2))
								break;
						}
						if (a2 == nullptr)
							operatesectors(sectp, g_ac);
					}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void garybanjo(DDukeActor* g_ac)
{
	if (banjosound == 0)
	{
		int rnum = (krand() & 3) + 1;
		if (rnum == 4)
		{
			banjosound = 262;
		}
		else if (rnum == 1)
		{
			banjosound = 272;
		}
		else if (rnum == 2)
		{
			banjosound = 273;
		}
		else
		{
			banjosound = 273;
		}
		S_PlayActorSound(banjosound, g_ac, CHAN_WEAPON);
	}
	else if (!S_CheckActorSoundPlaying(g_ac, banjosound))
		S_PlayActorSound(banjosound, g_ac, CHAN_WEAPON);
}
END_DUKE_NS
