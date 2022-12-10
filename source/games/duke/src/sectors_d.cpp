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

#include "ns.h"
#include "global.h"
#include "sounds.h"
#include "names_d.h"
#include "mapinfo.h"
#include "dukeactor.h"
#include "secrets.h"

// PRIMITIVE
BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool checkaccessswitch_d(int snum, int switchpal, DDukeActor* act, walltype* wwal)
{
	if (ps[snum].access_incs == 0)
	{
		if (switchpal == 0)
		{
			if ((ps[snum].got_access & 1))
				ps[snum].access_incs = 1;
			else FTA(70, &ps[snum]);
		}

		else if (switchpal == 21)
		{
			if (ps[snum].got_access & 2)
				ps[snum].access_incs = 1;
			else FTA(71, &ps[snum]);
		}

		else if (switchpal == 23)
		{
			if (ps[snum].got_access & 4)
				ps[snum].access_incs = 1;
			else FTA(72, &ps[snum]);
		}

		if (ps[snum].access_incs == 1)
		{
			if (!act)
				ps[snum].access_wall = wwal;
			else
				ps[snum].access_spritenum = act;
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

void activatebysector_d(sectortype* sect, DDukeActor* activator)
{
	int didit = 0;

	DukeSectIterator it(sect);
	while (auto act = it.Next())
	{
		if (isactivator(act))
		{
			operateactivators(act->spr.lotag, nullptr);
			didit = 1;
			//			return;
		}
	}

	if (didit == 0)
		operatesectors(sect, activator);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkplayerhurt_d(player_struct* p, const Collision& coll)
{
	if (coll.type == kHitSprite)
	{
		CallOnHurt(coll.actor(), p);
		return;
	}

	if (coll.type != kHitWall) return;
	auto wal = coll.hitWall;

	if (p->hurt_delay > 0) p->hurt_delay--;
	else if (wal->cstat & (CSTAT_WALL_BLOCK | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_MASKED | CSTAT_WALL_BLOCK_HITSCAN))
	{
		int tf = tileflags(wal->overtexture);
		if (tf & TFLAG_ANIMFORCEFIELD)
		{
			p->GetActor()->spr.extra -= 5;

			p->hurt_delay = 16;
			SetPlayerPal(p, PalEntry(32, 32, 0, 0));

			p->vel.XY() = -p->GetActor()->spr.Angles.Yaw.ToVector() * 16;
			S_PlayActorSound(DUKE_LONGTERM_PAIN, p->GetActor());

			checkhitwall(p->GetActor(), wal, p->GetActor()->getPosWithOffsetZ() + p->GetActor()->spr.Angles.Yaw.ToVector() * 2);
		}
		else if (tf & TFLAG_FORCEFIELD)
		{
			p->hurt_delay = 26;
			checkhitwall(p->GetActor(), wal, p->GetActor()->getPosWithOffsetZ() + p->GetActor()->spr.Angles.Yaw.ToVector() * 2);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkhitdefault_d(DDukeActor* targ, DDukeActor* proj)
{
	if ((targ->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && targ->spr.hitag == 0 && targ->spr.lotag == 0 && targ->spr.statnum == STAT_DEFAULT)
		return;

	if ((proj->spr.picnum == DTILE_FREEZEBLAST || proj->GetOwner() != targ) && targ->spr.statnum != STAT_PROJECTILE)
	{
		if (badguy(targ) == 1)
		{
			if (isWorldTour() && targ->spr.picnum == DTILE_FIREFLY && targ->spr.scale.X < 0.75)
				return;

			if (proj->spr.picnum == DTILE_RPG) proj->spr.extra <<= 1;

			if ((targ->spr.picnum != DTILE_DRONE) && (targ->spr.picnum != DTILE_ROTATEGUN) && (targ->spr.picnum != DTILE_COMMANDER) && targ->spr.picnum != DTILE_GREENSLIME)
				if (proj->spr.picnum != DTILE_FREEZEBLAST)
					//if (actortype[targ->spr.picnum] == 0) //TRANSITIONAL.
				{
					auto spawned = spawn(proj, DTILE_JIBS6);
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

			if (Owner && Owner->isPlayer() && targ->spr.picnum != DTILE_ROTATEGUN && targ->spr.picnum != DTILE_DRONE)
				if (ps[Owner->PlayerIndex()].curr_weapon == SHOTGUN_WEAPON)
				{
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat3"));
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat1"));
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat2"));
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat4"));
				}

			if (!actorflag(targ, SFLAG2_NODAMAGEPUSH) && !bossguy(targ)) // RR does not have this.
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
			if ((targ->spr.scale.X < 0.375 || targ->spr.picnum == DTILE_SHARK) && proj->spr.picnum == DTILE_SHRINKSPARK) return;
		}

		if (targ->spr.statnum != STAT_ZOMBIEACTOR)
		{
			if (proj->spr.picnum == DTILE_FREEZEBLAST && ((targ->isPlayer() && targ->spr.pal == 1) || (gs.freezerhurtowner == 0 && proj->GetOwner() == targ)))
				return;

			int hitpic = proj->spr.picnum;
			auto Owner = proj->GetOwner();
			if (Owner && Owner->isPlayer())
			{
				if (targ->isPlayer() && ud.coop != 0 && ud.ffire == 0)
					return;

				auto tOwner = targ->GetOwner();
				if (isWorldTour() && hitpic == DTILE_FIREBALL && tOwner && tOwner->spr.picnum != DTILE_FIREBALL)
					hitpic = DTILE_FLAMETHROWERFLAME;
			}

			targ->attackertype = hitpic;
			targ->hitextra += proj->spr.extra;
			targ->hitang = proj->spr.Angles.Yaw;
			targ->SetHitOwner(Owner);
		}

		if (targ->spr.statnum == STAT_PLAYER)
		{
			auto p = targ->spr.yint;
			if (ps[p].newOwner != nullptr)
			{
				ps[p].newOwner = nullptr;
				ps[p].GetActor()->restoreloc();

				updatesector(ps[p].GetActor()->getPosWithOffsetZ(), &ps[p].cursector);

				DukeStatIterator it(STAT_ACTOR);
				while (auto itActor = it.Next())
				{
					if (actorflag(itActor, SFLAG2_CAMERA)) itActor->spr.yint = 0;
				}
			}

			if (targ->spr.scale.X < 0.375 && proj->spr.picnum == DTILE_SHRINKSPARK)
				return;

			auto hitowner = targ->GetHitOwner();
			if (!hitowner || !hitowner->isPlayer())
				if (ud.player_skill >= 3)
					proj->spr.extra += (proj->spr.extra >> 1);
		}

	}
}

void checkhitsprite_d(DDukeActor* targ, DDukeActor* proj)
{
	if (targ->GetClass() != RUNTIME_CLASS(DDukeActor))
	{
		CallOnHit(targ, proj);
		return;
	}


	if (targ->spr.picnum == DTILE_PLAYERONWATER)
	{
		targ = targ->GetOwner();
		if (!targ) return;
	}
	checkhitdefault_d(targ, proj);
}

//---------------------------------------------------------------------------
//
// taken out of checksectors to eliminate some gotos.
//
//---------------------------------------------------------------------------

void clearcameras(player_struct* p)
{
	p->GetActor()->restorepos();
	p->newOwner = nullptr;

	updatesector(p->GetActor()->getPosWithOffsetZ(), &p->cursector);

	DukeStatIterator it(STAT_ACTOR);
	while (auto act = it.Next())
	{
		if (actorflag(act, SFLAG2_CAMERA)) act->spr.yint = 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checksectors_d(int snum)
{
	int i = -1;
	player_struct* p;
	walltype* hitscanwall;
	HitInfo near;

	p = &ps[snum];
	auto pact = p->GetActor();

	if (!p->insector()) return;

	switch (p->cursector->lotag)
	{

	case 32767:
		p->cursector->lotag = 0;
		FTA(9, p);
		p->secret_rooms++;
		SECRET_Trigger(sectindex(p->cursector));
		return;
	case -1:
		p->cursector->lotag = 0;
		setnextmap(false);
		return;
	case -2:
		p->cursector->lotag = 0;
		p->timebeforeexit = 26 * 8;
		p->customexitsound = p->cursector->hitag;
		return;
	default:
		if (p->cursector->lotag >= 10000 && p->cursector->lotag < 16383)
		{
			if (snum == screenpeek || ud.coop == 1)
				S_PlayActorSound(p->cursector->lotag - 10000, pact);
			p->cursector->lotag = 0;
		}
		break;

	}

	//After this point the the player effects the map with space

	if (chatmodeon || p->GetActor()->spr.extra <= 0) return;

	if (ud.cashman && PlayerInput(snum, SB_OPEN))
		fi.lotsofmoney(p->GetActor(), 2);

	if (p->newOwner != nullptr)
	{
		if (abs(PlayerInputSideVel(snum)) > 0.75 || abs(PlayerInputForwardVel(snum)) > 0.75)
		{
			clearcameras(p);
			return;
		}
		else if (PlayerInput(snum, SB_ESCAPE))
		{
			clearcameras(p);
			return;
		}
	}

	if (!(PlayerInput(snum, SB_OPEN)))
		p->toggle_key_flag = 0;

	else if (!p->toggle_key_flag)
	{
		near.hitActor = nullptr;
		p->toggle_key_flag = 1;
		hitscanwall = nullptr;

		double dist = hitawall(p, &hitscanwall);

		if (hitscanwall != nullptr)
		{
			if (dist < 80 && hitscanwall->overtexture == mirrortex)
				if (hitscanwall->lotag > 0 && S_CheckSoundPlaying(hitscanwall->lotag) == 0 && snum == screenpeek)
				{
					S_PlayActorSound(hitscanwall->lotag, pact);
					return;
				}

			if (hitscanwall != nullptr && (hitscanwall->cstat & CSTAT_WALL_MASKED))
				if (hitscanwall->lotag)
					return;
		}
		if (p->newOwner != nullptr)
			neartag(p->GetActor()->getPrevPosWithOffsetZ(), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
		else
		{
			neartag(p->GetActor()->getPosWithOffsetZ(), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag(p->GetActor()->getPosWithOffsetZ().plusZ(8), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag(p->GetActor()->getPosWithOffsetZ().plusZ(16), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
			{
				neartag(p->GetActor()->getPosWithOffsetZ().plusZ(16), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag | NT_Hitag);
				if (near.actor() != nullptr)
				{
					if (actorflag(near.actor(), SFLAG2_TRIGGERRESPAWN))
						return;
				}

				near.clearObj();
			}
		}

		if (p->newOwner == nullptr && near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
			if (isanunderoperator(p->GetActor()->sector()->lotag))
				near.hitSector = p->GetActor()->sector();

		if (near.hitSector && (near.hitSector->lotag & 16384))
			return;

		if (near.actor() == nullptr && near.hitWall == nullptr)
			if (p->cursector->lotag == 2)
			{
				DDukeActor* hit;
				dist = hitasprite(p->GetActor(), &hit);
				if (hit) near.hitActor = hit;
				if (dist > 80) near.hitActor = nullptr;

			}

		auto const neartagsprite = near.actor();
		if (neartagsprite != nullptr)
		{
			if (checkhitswitch(snum, nullptr, neartagsprite)) return;

			if (CallOnUse(neartagsprite, p))
				return;
		}

		if (!PlayerInput(snum, SB_OPEN)) return;
		else if (p->newOwner != nullptr)
		{
			clearcameras(p);
			return;
		}

		if (near.hitWall == nullptr && near.hitSector == nullptr && near.actor() == nullptr)
			if (hits(p->GetActor()) < 32)
			{
				if ((krand() & 255) < 16)
					S_PlayActorSound(DUKE_SEARCH2, pact);
				else S_PlayActorSound(DUKE_SEARCH, pact);
				return;
			}

		if (near.hitWall)
		{
			if (near.hitWall->lotag > 0 && isadoorwall(near.hitWall->walltexture))
			{
				if (hitscanwall == near.hitWall || hitscanwall == nullptr)
					checkhitswitch(snum, near.hitWall, nullptr);
				return;
			}
			else if (p->newOwner != nullptr)
			{
				clearcameras(p);
				return;
			}
		}

		if (near.hitSector && (near.hitSector->lotag & 16384) == 0 && isanearoperator(near.hitSector->lotag))
		{
			DukeSectIterator it(near.hitSector);
			while (auto act = it.Next())
			{
				if (isactivator(act) || ismasterswitch(act))
					return;
			}
			operatesectors(near.hitSector, p->GetActor());
		}
		else if ((p->GetActor()->sector()->lotag & 16384) == 0)
		{
			if (isanunderoperator(p->GetActor()->sector()->lotag))
			{
				DukeSectIterator it(p->GetActor()->sector());
				while (auto act = it.Next())
				{
					if (isactivator(act) || ismasterswitch(act)) return;
				}
				operatesectors(p->GetActor()->sector(), p->GetActor());
			}
			else checkhitswitch(snum, near.hitWall, nullptr);
		}
	}
}





END_DUKE_NS
