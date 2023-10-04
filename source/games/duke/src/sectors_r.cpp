//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

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
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "sounds.h"
#include "mapinfo.h"
#include "dukeactor.h"
#include "secrets.h"
#include "vm.h"

// PRIMITIVE
BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool checkaccessswitch_r(DDukePlayer* const p, int switchpal, DDukeActor* act, walltype* wwal)
{
	const auto pact = p->GetActor();

	if (p->access_incs == 0)
	{
		if (switchpal == 0)
		{
			if (p->keys[1])
				p->access_incs = 1;
			else
			{
				FTA(70, p);
				if (isRRRA()) S_PlayActorSound(99, act ? act : pact);
			}
		}

		else if (switchpal == 21)
		{
			if (p->keys[2])
				p->access_incs = 1;
			else
			{
				FTA(71, p);
				if (isRRRA()) S_PlayActorSound(99, act ? act : pact);
			}
		}

		else if (switchpal == 23)
		{
			if (p->keys[3])
				p->access_incs = 1;
			else
			{
				FTA(72, p);
				if (isRRRA()) S_PlayActorSound(99, act ? act : pact);
			}
		}

		if (p->access_incs == 1)
		{
			if (!act)
				p->access_wall = wwal;
			else
				p->access_spritenum = act;
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

void activatebysector_r(sectortype* sect, DDukeActor* activator)
{
	DukeSectIterator it(sect);
	while (auto act = it.Next())
	{
		if (isactivator(act))
		{
			operateactivators(act->spr.lotag, nullptr);
			//			return;
		}
	}

	if (sect->lotag != 22)
		operatesectors(sect, activator);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkplayerhurt_r(DDukePlayer* p, const Collision &coll)
{
	if (coll.type == kHitSprite)
	{
		CallOnHurt(coll.actor(), p);
		return;
	}

	if (coll.type == kHitWall)
	{
		auto wal = coll.hitWall;

		if (p->hurt_delay > 0) p->hurt_delay--;
		else if (wal->cstat & (CSTAT_WALL_BLOCK | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_MASKED | CSTAT_WALL_BLOCK_HITSCAN))
		{
			int tf = tileflags(wal->overtexture);
			if (tf & TFLAG_FORCEFIELD)
			{
				p->hurt_delay = 26;
				checkhitwall(p->GetActor(), wal, p->GetActor()->getPosWithOffsetZ() + p->GetActor()->spr.Angles.Yaw.ToVector() * 2);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checksectors_r(DDukePlayer* const p)
{
	walltype* hitscanwall;
	HitInfo near;

	auto pact = p->GetActor();

	if (!p->insector()) return;

	switch (p->cursector->lotag)
	{

	case 32767:
		p->cursector->lotag = 0;
		FTA(9, p);
		Level.addSecret(p->pnum);
		return;
	case -1:
		p->cursector->lotag = 0;
		if (!isRRRA() || !RRRA_ExitedLevel)
		{
			setnextmap(false);
			RRRA_ExitedLevel = 1;
		}
		return;
	case -2:
		p->cursector->lotag = 0;
		p->timebeforeexit = 26 * 8;
		p->customexitsound = p->cursector->hitag;
		return;
	default:
		if (p->cursector->lotag >= 10000)
		{
			if (p->pnum == screenpeek || ud.coop == 1)
				S_PlayActorSound(p->cursector->lotag - 10000, pact);
			p->cursector->lotag = 0;
		}
		break;

	}

	//After this point the the player effects the map with space

	if (chatmodeon || pact->spr.extra <= 0) return;

	if (ud.cashman && !!(p->cmd.ucmd.actions & SB_OPEN))
		lotsofstuff(pact, 2, DukeMailClass);


	if (!(!!(p->cmd.ucmd.actions & SB_OPEN)))
		p->toggle_key_flag = 0;

	else if (!p->toggle_key_flag)
	{
		near.hitActor = nullptr;
		p->toggle_key_flag = 1;
		hitscanwall = nullptr;

		hitawall(p, &hitscanwall);

		if (hitscanwall != nullptr)
		{
			if (isRRRA())
			{
				if (hitscanwall->overtexture == mirrortex && p->pnum == screenpeek)
					if (numplayers == 1)
					{
						if (S_CheckActorSoundPlaying(pact, 27) == 0 && S_CheckActorSoundPlaying(pact, 28) == 0 && S_CheckActorSoundPlaying(pact, 29) == 0
							&& S_CheckActorSoundPlaying(pact, 257) == 0 && S_CheckActorSoundPlaying(pact, 258) == 0)
						{
							int snd = krand() % 5;
							if (snd == 0)
								S_PlayActorSound(27, pact);
							else if (snd == 1)
								S_PlayActorSound(28, pact);
							else if (snd == 2)
								S_PlayActorSound(29, pact);
							else if (snd == 3)
								S_PlayActorSound(257, pact);
							else if (snd == 4)
								S_PlayActorSound(258, pact);
						}
						return;
					}
			}
			else
			{
				if (hitscanwall->overtexture == mirrortex)
					if (hitscanwall->lotag > 0 && S_CheckActorSoundPlaying(pact, hitscanwall->lotag) == 0 && p->pnum == screenpeek)
					{
						S_PlayActorSound(hitscanwall->lotag, pact);
						return;
					}
			}

			if ((hitscanwall->cstat & CSTAT_WALL_MASKED))
				if (hitscanwall->lotag)
					return;

		}
		if (isRRRA())
		{
			if (p->OnMotorcycle)
			{
				if (p->MotoSpeed < 20)
				{
					OffMotorcycle(p);
						return;
				}
				return;
			}
			if (p->OnBoat)
			{
				if (p->MotoSpeed < 20)
				{
					OffBoat(p);
					return;
				}
				return;
			}
			neartag(pact->getPosWithOffsetZ(), pact->sector(), pact->PrevAngles.Yaw, near , 80., NT_Lotag | NT_Hitag);
		}

		if (p->newOwner != nullptr)
			neartag(pact->getPrevPosWithOffsetZ(), pact->sector(), pact->PrevAngles.Yaw, near, 80., NT_Lotag);
		else
		{
			neartag(pact->getPosWithOffsetZ(), pact->sector(), pact->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag(pact->getPosWithOffsetZ().plusZ(8), pact->sector(), pact->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag(pact->getPosWithOffsetZ().plusZ(16), pact->sector(), pact->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
			{
				neartag(pact->getPosWithOffsetZ().plusZ(16), pact->sector(), pact->PrevAngles.Yaw, near, 80., NT_Lotag | NT_Hitag);
				if (near.actor() != nullptr)
				{
					if (near.actor()->flags2 & SFLAG2_TRIGGERRESPAWN)
						return;

					if (near.actor()->IsKindOf(RedneckCowClass)) // This is for a VERY poorly implemented CON hack. See 'iftipcow'.
					{
						near.actor()->spriteextra = 1;
						return;
					}
				}

				near.clearObj();
			}
		}

		if (p->newOwner == nullptr && near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
			if (isanunderoperator(pact->sector()->lotag))
				near.hitSector = pact->sector();

		if (near.hitSector && (near.hitSector->lotag & 16384))
			return;

		if (near.actor() == nullptr && near.hitWall == nullptr)
			if (p->cursector->lotag == ST_2_UNDERWATER)
			{
				DDukeActor* hit;
				double dist = hitasprite(pact, &hit);
				if (hit) near.hitActor = hit;
				if (dist > 80) near.hitActor = nullptr;
			}

		auto const neartagsprite = near.actor();
		if (neartagsprite != nullptr)
		{
			if (checkhitswitch(p, nullptr, neartagsprite)) return;

			if (neartagsprite->GetClass() != RUNTIME_CLASS(DDukeActor))
			{
				if (CallOnUse(neartagsprite, p))
					return;
			}
		}

		if (!!!(p->cmd.ucmd.actions & SB_OPEN)) return;

		if (near.hitWall == nullptr && near.hitSector == nullptr && near.actor() == nullptr)
			if (hits(pact) < 32)
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
					checkhitswitch(p, near.hitWall, nullptr);
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
			if (haslock(near.hitSector, p))
				operatesectors(near.hitSector, pact);
			else
			{
				if (neartagsprite && neartagsprite->spriteextra > 3)
					S_PlayActorSound(99, pact);
				else
					S_PlayActorSound(419, pact);
				FTA(41, p);
			}
		}
		else if ((pact->sector()->lotag & 16384) == 0)
		{
			if (isanunderoperator(pact->sector()->lotag))
			{
				DukeSectIterator it(pact->sector());
				while (auto act = it.Next())
				{
					if (isactivator(act) || ismasterswitch(act))
						return;
				}
				if (haslock(near.hitSector, p))
					operatesectors(pact->sector(), pact);
				else
				{
					if (neartagsprite && neartagsprite->spriteextra > 3)
						S_PlayActorSound(99, pact);
					else
						S_PlayActorSound(419, pact);
					FTA(41, p);
				}
			}
			else checkhitswitch(p, near.hitWall, nullptr);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void dofurniture(DDukePlayer* const p, walltype* wlwal)
{
	assert(wlwal->twoSided());
	auto sectp = p->cursector;
	auto nextsect = wlwal->nextSector();

	double movestep = min(sectp->hitag * maptoworld, 1.);
	if (movestep == 0) movestep = 4 * maptoworld;

	double max_x = INT32_MIN, max_y = INT32_MIN, min_x = INT32_MAX, min_y = INT32_MAX;
	for (auto& wal : nextsect->walls)
	{
		double x = wal.pos.X;
		double y = wal.pos.Y;
		if (x > max_x)
			max_x = x;
		if (y > max_y)
			max_y = y;
		if (x < min_x)
			min_x = x;
		if (y < min_y)
			min_y = y;
	}

	double margin = movestep + maptoworld;
	max_x += margin;
	max_y += margin;
	min_x -= margin;
	min_y -= margin;
	int pos_ok = 1;
	if (!inside(max_x, max_y, sectp) ||
		!inside(max_x, min_y, sectp) ||
		!inside(min_x, min_y, sectp) ||
		!inside(min_x, max_y, sectp))
		pos_ok = 0;

	for (auto& wal : nextsect->walls)
	{
		switch (wlwal->lotag)
		{
		case 42:
		case 41:
		case 40:
		case 43:
			vertexscan(&wal, [=](walltype* w)
				{
					StartInterpolation(w, wlwal->lotag == 41 || wlwal->lotag == 43 ? Interp_Wall_X : Interp_Wall_Y);
				});
			break;
		}
	}

	if (pos_ok)
	{
		const auto pact = p->GetActor();

		if (S_CheckActorSoundPlaying(pact, 389) == 0)
			S_PlayActorSound(389, pact);
		for(auto& wal : nextsect->walls)
		{
			auto vec = wal.pos;
			switch (wlwal->lotag)
			{
			case 42:
				vec.Y += movestep;
				dragpoint(&wal, vec);
				break;
			case 41:
				vec.X -= movestep;
				dragpoint(&wal, vec);
				break;
			case 40:
				vec.Y -= movestep;
				dragpoint(&wal, vec);
				break;
			case 43:
				vec.X += movestep;
				dragpoint(&wal, vec);
				break;
			}
		}
	}
	else
	{
		movestep -= 2 * maptoworld;
		for(auto& wal : nextsect->walls)
		{
			auto vec = wal.pos;
			switch (wlwal->lotag)
			{
			case 42:
				vec.Y -= movestep;
				dragpoint(&wal, vec);
				break;
			case 41:
				vec.X += movestep;
				dragpoint(&wal, vec);
				break;
			case 40:
				vec.Y += movestep;
				dragpoint(&wal, vec);
				break;
			case 43:
				vec.X -= movestep;
				dragpoint(&wal, vec);
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

void tearitup(sectortype* sect)
{
	DukeSectIterator it(sect);
	while (auto act = it.Next())
	{
		if (act->GetClass() == RedneckDestructoClass)
		{
			act->attackertype = DukeShotSparkClass;
			act->hitextra = 1;
		}
	}
}
END_DUKE_NS
