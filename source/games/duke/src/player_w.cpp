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
#include "gamevar.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

int operateTripbomb(DDukePlayer* const p);

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void DoFire(DDukePlayer* const p)
{
	const auto pact = p->GetActor();

	if (aplWeaponWorksLike(p->curr_weapon, p) != KNEE_WEAPON)
	{
		p->ammo_amount[p->curr_weapon]--;
	}

	if (aplWeaponFireSound(p->curr_weapon, p))
	{
		S_PlayActorSound(aplWeaponFireSound(p->curr_weapon, p), pact);
	}

	SetGameVarID(g_iWeaponVarID, p->curr_weapon, pact, p->pnum);
	SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike(p->curr_weapon, p), pact, p->pnum);
	shoot(pact, GetSpawnType(aplWeaponShoots(p->curr_weapon, p)));
	for (int i = 1; i < aplWeaponShotsPerBurst(p->curr_weapon, p); i++)
	{
		shoot(pact, GetSpawnType(aplWeaponShoots(p->curr_weapon, p)));
		if (aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_AMMOPERSHOT)
		{
			p->ammo_amount[p->curr_weapon]--;
		}
	}

	if (!(aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_NOVISIBLE))
	{
		// make them visible if not set...
		lastvisinc = PlayClock + 32;
		p->visibility = 0;
	}

	if ( //!(aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_CHECKATRELOAD) &&
		aplWeaponReload(p->curr_weapon, p) > aplWeaponTotalTime(p->curr_weapon, p)
		&& p->ammo_amount[p->curr_weapon] > 0
		&& (aplWeaponClip(p->curr_weapon, p))
		&& ((p->ammo_amount[p->curr_weapon] % (aplWeaponClip(p->curr_weapon, p))) == 0)
		)
	{
		// do clip check...
		p->kickback_pic = aplWeaponTotalTime(p->curr_weapon, p);
		// is same as p->kickback_pic....
	}

	if (aplWeaponWorksLike(p->curr_weapon, p) != KNEE_WEAPON)
	{
		checkavailweapon(p);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void DoSpawn(DDukePlayer* const p)
{
	if(!aplWeaponSpawn(p->curr_weapon, p))
		return;

	auto j = spawn(p->GetActor(), GetSpawnType(aplWeaponSpawn(p->curr_weapon, p)));
	if (!j) return;

	if((aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_SPAWNTYPE2 ) )
	{
		// like shotgun shells
		j->spr.Angles.Yaw += DAngle180;
		ssp(j,CLIPMASK0);
		j->spr.Angles.Yaw += DAngle180;
//		p->kickback_pic++;
	}
	else if((aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_SPAWNTYPE3 ) )
	{
		// like chaingun shells
		j->spr.Angles.Yaw += DAngle90;
		j->vel.X += 2.;
		j->spr.pos.Z += 3;
		ssp(j,CLIPMASK0);
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fireweapon_ww(DDukePlayer* const p)
{
	auto pact = p->GetActor();

	p->crack_time = CRACK_TIME;

	if (p->holster_weapon == 1)
	{
		if (p->last_pissed_time <= (26 * 218) && p->weapon_pos == -9)
		{
			p->holster_weapon = 0;
			p->oweapon_pos = p->weapon_pos = 10;
			FTA(74, p);
		}
	}
	else
	{
		SetGameVarID(g_iReturnVarID, 0, pact, p->pnum);
		SetGameVarID(g_iWeaponVarID, p->curr_weapon, pact, p->pnum);
		SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike(p->curr_weapon, p), pact, p->pnum);
		OnEvent(EVENT_FIRE, p->pnum, pact, -1);
		if (GetGameVarID(g_iReturnVarID, pact, p->pnum).value() == 0)
		{
			switch (aplWeaponWorksLike(p->curr_weapon, p))
			{
			case HANDBOMB_WEAPON:
				p->hbomb_hold_delay = 0;
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound(p->curr_weapon, p))
					{
						S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
					}
				}
				break;
			case HANDREMOTE_WEAPON:
				p->hbomb_hold_delay = 0;
				p->kickback_pic = 1;
				if (aplWeaponInitialSound(p->curr_weapon, p))
				{
					S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
				}
				break;

			case PISTOL_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					//p->ammo_amount[p->curr_weapon]--;
					p->kickback_pic = 1;
					if (aplWeaponInitialSound(p->curr_weapon, p))
					{
						S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
					}
				}
				break;


			case CHAINGUN_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound(p->curr_weapon, p))
					{
						S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
					}
				}
				break;

			case SHOTGUN_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0 && p->random_club_frame == 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound(p->curr_weapon, p))
					{
						S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
					}
				}
				break;
			case TRIPBOMB_WEAPON:
				if (operateTripbomb(p))
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound(p->curr_weapon, p))
					{
						S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
					}
				}
				break;

			case SHRINKER_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound(p->curr_weapon, p))
					{
						S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
					}
				}
				break;

			case GROW_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound(p->curr_weapon, p))
					{
						S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
					}
				}
				break;

			case FREEZE_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound(p->curr_weapon, p))
					{
						S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
					}
				}
				break;
			case DEVISTATOR_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					p->hbomb_hold_delay = !p->hbomb_hold_delay;
					if (aplWeaponInitialSound(p->curr_weapon, p))
					{
						S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
					}
				}
				break;

			case RPG_WEAPON:
				if (p->ammo_amount[RPG_WEAPON] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound(p->curr_weapon, p))
					{
						S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
					}
				}
				break;

			case KNEE_WEAPON:
				if (p->quick_kick == 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound(p->curr_weapon, p))
					{
						S_PlayActorSound(aplWeaponInitialSound(p->curr_weapon, p), pact);
					}
				}
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

void operateweapon_ww(DDukePlayer* const p, ESyncBits actions)
{
	auto pact = p->GetActor();

	// already firing...
	if (aplWeaponWorksLike(p->curr_weapon, p) == HANDBOMB_WEAPON)
	{
		if (aplWeaponHoldDelay(p->curr_weapon, p)	// there is a hold delay
			&& (p->kickback_pic == aplWeaponFireDelay(p->curr_weapon, p))	// and we are 'at' hold
			&& (actions & SB_FIRE)	// and 'fire' button is still down
			)
			// just hold here...
		{
			p->rapid_fire_hold = 1;
			return;
		}
		p->kickback_pic++;
		if (p->kickback_pic == aplWeaponHoldDelay(p->curr_weapon, p))
		{
			double zvel, vel;

			p->ammo_amount[p->curr_weapon]--;

			if (p->on_ground && (actions & SB_CROUCH))
			{
				vel = 15 / 16.;
				setFreeAimVelocity(vel, zvel, p->Angles.getPitchWithView(), 10.);
			}
			else
			{
				vel = 140 / 16.;
				setFreeAimVelocity(vel, zvel, p->Angles.getPitchWithView(), 10.);
				zvel -= 4;
			}

			auto spawned = CreateActor(p->cursector, pact->getPosWithOffsetZ() + pact->spr.Angles.Yaw.ToVector() * 16, DukePipeBombClass, -16, DVector2(0.140625, 0.140625),
				pact->spr.Angles.Yaw, vel + p->hbomb_hold_delay * 2, zvel, pact, 1);

			if (spawned)
			{
				{
					int lGrenadeLifetime = GetGameVar("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, nullptr, p->pnum).value();
					int lGrenadeLifetimeVar = GetGameVar("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, nullptr, p->pnum).value();
					// set timer.  blows up when at zero....
					spawned->spr.extra = lGrenadeLifetime
						+ MulScale(krand(), lGrenadeLifetimeVar, 14)
						- lGrenadeLifetimeVar;
				}

				if (vel == 15 / 16.)
				{
					spawned->spr.yint = 3;
					spawned->spr.pos.Z += 8;
				}

				double hd = hits(pact);
				if (hd < 32)
				{
					spawned->spr.Angles.Yaw += DAngle180;
					spawned->vel *= 1./3.;
				}

				p->hbomb_on = 1;
			}

		}
		else if (p->kickback_pic < aplWeaponHoldDelay(p->curr_weapon, p) &&
			(actions & SB_CROUCH))
		{
			p->hbomb_hold_delay++;
		}
		else if (p->kickback_pic > aplWeaponTotalTime(p->curr_weapon, p))
		{
			p->okickback_pic = p->kickback_pic = 0;
			// don't change to remote when in NAM: grenades are timed
			checkavailweapon(p);
		}
	}
	else if (aplWeaponWorksLike(p->curr_weapon, p) == HANDREMOTE_WEAPON)
	{
		p->kickback_pic++;

		if (p->kickback_pic == aplWeaponFireDelay(p->curr_weapon, p))
		{
			if (aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_BOMB_TRIGGER)
			{
				p->hbomb_on = 0;
			}
			if (aplWeaponShoots(p->curr_weapon, p) != 0)
			{
				if (!(aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_NOVISIBLE))
				{
					// make them visible if not set...
					lastvisinc = PlayClock + 32;
					p->visibility = 0;
				}
				SetGameVarID(g_iWeaponVarID, p->curr_weapon, pact, p->pnum);
				SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike(p->curr_weapon, p), pact, p->pnum);
				shoot(pact, GetSpawnType(aplWeaponShoots(p->curr_weapon, p)));
			}
		}

		if (p->kickback_pic >= aplWeaponTotalTime(p->curr_weapon, p))
		{
			p->okickback_pic = p->kickback_pic = 0;
			/// WHAT THE HELL DOES THIS DO....?????????????
			if (p->ammo_amount[TRIPBOMB_WEAPON] > 0)
				fi.addweapon(p, TRIPBOMB_WEAPON, true);
			else
				checkavailweapon(p);
		}

	}
	else
	{

		// the basic weapon...
		p->kickback_pic++;

		if (aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_CHECKATRELOAD)
		{
			if (p->kickback_pic == aplWeaponReload(p->curr_weapon, p))
			{
				checkavailweapon(p);
			}
		}
		if (aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_STANDSTILL
			&& p->kickback_pic < (aplWeaponFireDelay(p->curr_weapon, p) + 1))
		{
			pact->restorez();
			p->vel.Z = 0;
		}
		if (p->kickback_pic == aplWeaponSound2Time(p->curr_weapon, p))
		{
			if (aplWeaponSound2Sound(p->curr_weapon, p))
			{
				S_PlayActorSound(aplWeaponSound2Sound(p->curr_weapon, p), pact);
			}
		}
		if (p->kickback_pic == aplWeaponSpawnTime(p->curr_weapon, p))
		{
			DoSpawn(p);
		}
		if (p->kickback_pic == aplWeaponFireDelay(p->curr_weapon, p))
		{
			DoFire(p);
		}

		if (p->kickback_pic > aplWeaponFireDelay(p->curr_weapon, p)
			&& p->kickback_pic < aplWeaponTotalTime(p->curr_weapon, p))
		{

			if (aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_AUTOMATIC)
			{ // an 'automatic'
				if ((actions & SB_FIRE) == 0)
				{
					p->kickback_pic = aplWeaponTotalTime(p->curr_weapon, p);
				}

				if (aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_FIREEVERYTHIRD)
				{
					if (((p->kickback_pic) % 3) == 0)
					{
						DoFire(p);
						DoSpawn(p);
					}

				}
				if (aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_FIREEVERYOTHER)
				{
					// fire every other...
					DoFire(p);
					DoSpawn(p);
				}

			} // 'automatic
		}
		else if (p->kickback_pic >= aplWeaponTotalTime(p->curr_weapon, p))
		{
			if ( //!(aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_CHECKATRELOAD) &&
				aplWeaponReload(p->curr_weapon, p) > aplWeaponTotalTime(p->curr_weapon, p)
				&& p->ammo_amount[p->curr_weapon] > 0
				&& (aplWeaponClip(p->curr_weapon, p))
				&& ((p->ammo_amount[p->curr_weapon] % (aplWeaponClip(p->curr_weapon, p))) == 0)
				)
			{
				// reload in progress...
				int timer = aplWeaponReload(p->curr_weapon, p) - aplWeaponTotalTime(p->curr_weapon, p);
				// time for 'reload'

				if (p->kickback_pic == (aplWeaponTotalTime(p->curr_weapon, p) + 1))
				{ // eject shortly after 'total time'
					S_PlayActorSound(EJECT_CLIP, pact);
				}
				else if (p->kickback_pic == (aplWeaponReload(p->curr_weapon, p) - (timer / 3)))
				{
					// insert occurs 2/3 of way through reload delay
					S_PlayActorSound(INSERT_CLIP, pact);
				}

				if (p->kickback_pic >= (aplWeaponReload(p->curr_weapon, p)))
				{
					p->okickback_pic = p->kickback_pic = 0;
				}

			}
			else
			{
				if (aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_AUTOMATIC)
				{ // an 'automatic'
					if (actions & SB_FIRE)
					{
						// we are an AUTOMATIC.  Fire again...
						if (aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_RANDOMRESTART)
						{
							p->kickback_pic = 1 + (krand() & 3);
						}
						else
						{
							p->kickback_pic = 1;
						}
					}
					else
					{
						p->okickback_pic = p->kickback_pic = 0;
					}
				}
				else
				{ // not 'automatic' and >totaltime
					p->okickback_pic = p->kickback_pic = 0;
				}
			}
		}
	} // process the event ourselves if no handler provided.
}


END_DUKE_NS
