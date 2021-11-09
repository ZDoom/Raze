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
#include "names_d.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

int operateTripbomb(int snum);

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoFire(struct player_struct* p, int snum)
{
	int i;

	if (aplWeaponWorksLike[p->curr_weapon][snum] != KNEE_WEAPON)
	{
		p->ammo_amount[p->curr_weapon]--;
	}

	if (aplWeaponFireSound[p->curr_weapon][snum])
	{
		S_PlayActorSound(aplWeaponFireSound[p->curr_weapon][snum], p->GetActor());
	}

	SetGameVarID(g_iWeaponVarID, p->curr_weapon, p->GetActor(), snum);
	SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[p->curr_weapon][snum], p->GetActor(), snum);
	fi.shoot(p->GetActor(), aplWeaponShoots[p->curr_weapon][snum]);
	for (i = 1; i < aplWeaponShotsPerBurst[p->curr_weapon][snum]; i++)
	{
		fi.shoot(p->GetActor(), aplWeaponShoots[p->curr_weapon][snum]);
		if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_AMMOPERSHOT)
		{
			p->ammo_amount[p->curr_weapon]--;
		}
	}

	if (!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_NOVISIBLE))
	{
		// make them visible if not set...
		lastvisinc = PlayClock + 32;
		p->visibility = 0;
	}

	if ( //!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_CHECKATRELOAD) &&
		aplWeaponReload[p->curr_weapon][snum] > aplWeaponTotalTime[p->curr_weapon][snum]
		&& p->ammo_amount[p->curr_weapon] > 0
		&& (aplWeaponClip[p->curr_weapon][snum])
		&& ((p->ammo_amount[p->curr_weapon] % (aplWeaponClip[p->curr_weapon][snum])) == 0)
		)
	{
		// do clip check...
		p->kickback_pic = aplWeaponTotalTime[p->curr_weapon][snum];
		// is same as p->kickback_pic....
	}

	if (aplWeaponWorksLike[p->curr_weapon][snum] != KNEE_WEAPON)
	{
		checkavailweapon(p);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoSpawn(struct player_struct *p, int snum)
{
	if(!aplWeaponSpawn[p->curr_weapon][snum])
		return;
		
	auto j = spawn(p->GetActor(), aplWeaponSpawn[p->curr_weapon][snum]);
	
	if((aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_SPAWNTYPE2 ) )
	{
		// like shotgun shells
		j->s->ang += 1024;
		ssp(j,CLIPMASK0);
		j->s->ang += 1024;
//		p->kickback_pic++;
	}
	else if((aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_SPAWNTYPE3 ) )
	{
		// like chaingun shells
		j->s->ang += 1024;
		j->s->ang &= 2047;
		j->s->xvel += 32;
		j->s->z += (3<<8);
		ssp(j,CLIPMASK0);
	}
		
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fireweapon_ww(int snum)
{
	auto p = &ps[snum];
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
		SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
		SetGameVarID(g_iWeaponVarID, p->curr_weapon, p->GetActor(), snum);
		SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[p->curr_weapon][snum], p->GetActor(), snum);
		OnEvent(EVENT_FIRE, snum, p->GetActor(), -1);
		if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum) == 0)
		{
			switch (aplWeaponWorksLike[p->curr_weapon][snum])
			{
			case HANDBOMB_WEAPON:
				p->hbomb_hold_delay = 0;
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
					}
				}
				break;
			case HANDREMOTE_WEAPON:
				p->hbomb_hold_delay = 0;
				p->kickback_pic = 1;
				if (aplWeaponInitialSound[p->curr_weapon][snum])
				{
					S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
				}
				break;

			case PISTOL_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					//p->ammo_amount[p->curr_weapon]--;
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
					}
				}
				break;


			case CHAINGUN_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
					}
				}
				break;

			case SHOTGUN_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0 && p->random_club_frame == 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
					}
				}
				break;
			case TRIPBOMB_WEAPON:
				if (operateTripbomb(snum))
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
					}
				}
				break;

			case SHRINKER_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
					}
				}
				break;

			case GROW_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
					}
				}
				break;

			case FREEZE_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
					}
				}
				break;
			case DEVISTATOR_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					p->hbomb_hold_delay = !p->hbomb_hold_delay;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
					}
				}
				break;

			case RPG_WEAPON:
				if (p->ammo_amount[RPG_WEAPON] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
					}
				}
				break;

			case KNEE_WEAPON:
				if (p->quick_kick == 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pact);
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

void operateweapon_ww(int snum, ESyncBits actions, int psect)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();
	int i, k;
	int psectlotag = sector[psect].lotag;

	// already firing...
	if (aplWeaponWorksLike[p->curr_weapon][snum] == HANDBOMB_WEAPON)
	{
		if (aplWeaponHoldDelay[p->curr_weapon][snum]	// there is a hold delay
			&& (p->kickback_pic == aplWeaponFireDelay[p->curr_weapon][snum])	// and we are 'at' hold
			&& (actions & SB_FIRE)	// and 'fire' button is still down
			)
			// just hold here...
		{
			p->rapid_fire_hold = 1;
			return;
		}
		p->kickback_pic++;
		if (p->kickback_pic == aplWeaponHoldDelay[p->curr_weapon][snum])
		{
			p->ammo_amount[p->curr_weapon]--;

			if (p->on_ground && (actions & SB_CROUCH))
			{
				k = 15;
				i = MulScale(p->horizon.sum().asq16(), 20, 16);
			}
			else
			{
				k = 140;
				i = -512 - MulScale(p->horizon.sum().asq16(), 20, 16);
			}

			auto j = EGS(p->cursectnum,
				p->pos.x + p->angle.ang.bcos(-6),
				p->pos.y + p->angle.ang.bsin(-6),
				p->pos.z, HEAVYHBOMB, -16, 9, 9,
				p->angle.ang.asbuild(), (k + (p->hbomb_hold_delay << 5)), i, p->GetActor(), 1);

			{
				int lGrenadeLifetime = GetGameVar("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, nullptr, snum);
				int lGrenadeLifetimeVar = GetGameVar("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, nullptr, snum);
				// set timer.  blows up when at zero....
				j->s->extra = lGrenadeLifetime
					+ MulScale(krand(), lGrenadeLifetimeVar, 14)
					- lGrenadeLifetimeVar;
			}

			if (k == 15)
			{
				j->s->yvel = 3;
				j->s->z += (8 << 8);
			}

			k = hits(p->GetActor());
			if (k < 512)
			{
				j->s->ang += 1024;
				j->s->zvel /= 3;
				j->s->xvel /= 3;
			}

			p->hbomb_on = 1;

		}
		else if (p->kickback_pic < aplWeaponHoldDelay[p->curr_weapon][snum] &&
			(actions & SB_CROUCH))
		{
			p->hbomb_hold_delay++;
		}
		else if (p->kickback_pic > aplWeaponTotalTime[p->curr_weapon][snum])
		{
			p->okickback_pic = p->kickback_pic = 0;
			// don't change to remote when in NAM: grenades are timed
			checkavailweapon(p);
		}
	}
	else if (aplWeaponWorksLike[p->curr_weapon][snum] == HANDREMOTE_WEAPON)
	{
		p->kickback_pic++;

		if (p->kickback_pic == aplWeaponFireDelay[p->curr_weapon][snum])
		{
			if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_BOMB_TRIGGER)
			{
				p->hbomb_on = 0;
			}
			if (aplWeaponShoots[p->curr_weapon][snum] != 0)
			{
				if (!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_NOVISIBLE))
				{
					// make them visible if not set...
					lastvisinc = PlayClock + 32;
					p->visibility = 0;
				}
				SetGameVarID(g_iWeaponVarID, p->curr_weapon, p->GetActor(), snum);
				SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[p->curr_weapon][snum], p->GetActor(), snum);
				fi.shoot(p->GetActor(), aplWeaponShoots[p->curr_weapon][snum]);
			}
		}

		if (p->kickback_pic >= aplWeaponTotalTime[p->curr_weapon][snum])
		{
			p->okickback_pic = p->kickback_pic = 0;
			/// WHAT THE HELL DOES THIS DO....?????????????
			if (p->ammo_amount[TRIPBOMB_WEAPON] > 0)
				fi.addweapon(p, TRIPBOMB_WEAPON);
			else
				checkavailweapon(p);
		}

	}
	else
	{

		// the basic weapon...
		p->kickback_pic++;

		if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_CHECKATRELOAD)
		{
			if (p->kickback_pic == aplWeaponReload[p->curr_weapon][snum])
			{
				checkavailweapon(p);
			}
		}
		if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_STANDSTILL
			&& p->kickback_pic < (aplWeaponFireDelay[p->curr_weapon][snum] + 1))
		{
			p->pos.z = p->oposz;
			p->poszv = 0;
		}
		if (p->kickback_pic == aplWeaponSound2Time[p->curr_weapon][snum])
		{
			if (aplWeaponSound2Sound[p->curr_weapon][snum])
			{
				S_PlayActorSound(aplWeaponSound2Sound[p->curr_weapon][snum], pact);
			}
		}
		if (p->kickback_pic == aplWeaponSpawnTime[p->curr_weapon][snum])
		{
			DoSpawn(p, snum);
		}
		if (p->kickback_pic == aplWeaponFireDelay[p->curr_weapon][snum])
		{
			DoFire(p, snum);
		}

		if (p->kickback_pic > aplWeaponFireDelay[p->curr_weapon][snum]
			&& p->kickback_pic < aplWeaponTotalTime[p->curr_weapon][snum])
		{

			if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_AUTOMATIC)
			{ // an 'automatic'
				if ((actions & SB_FIRE) == 0)
				{
					p->kickback_pic = aplWeaponTotalTime[p->curr_weapon][snum];
				}

				if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_FIREEVERYTHIRD)
				{
					if (((p->kickback_pic) % 3) == 0)
					{
						DoFire(p, snum);
						DoSpawn(p, snum);
					}

				}
				if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_FIREEVERYOTHER)
				{
					// fire every other...
					DoFire(p, snum);
					DoSpawn(p, snum);
				}

			} // 'automatic
		}
		else if (p->kickback_pic >= aplWeaponTotalTime[p->curr_weapon][snum])
		{
			if ( //!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_CHECKATRELOAD) &&
				aplWeaponReload[p->curr_weapon][snum] > aplWeaponTotalTime[p->curr_weapon][snum]
				&& p->ammo_amount[p->curr_weapon] > 0
				&& (aplWeaponClip[p->curr_weapon][snum])
				&& ((p->ammo_amount[p->curr_weapon] % (aplWeaponClip[p->curr_weapon][snum])) == 0)
				)
			{
				// reload in progress...
				int i;
				i = aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum];
				// time for 'reload'

				if (p->kickback_pic == (aplWeaponTotalTime[p->curr_weapon][snum] + 1))
				{ // eject shortly after 'total time'
					S_PlayActorSound(EJECT_CLIP, pact);
				}
				else if (p->kickback_pic == (aplWeaponReload[p->curr_weapon][snum] - (i / 3)))
				{
					// insert occurs 2/3 of way through reload delay
					S_PlayActorSound(INSERT_CLIP, pact);
				}

				if (p->kickback_pic >= (aplWeaponReload[p->curr_weapon][snum]))
				{
					p->okickback_pic = p->kickback_pic = 0;
				}

			}
			else
			{
				if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_AUTOMATIC)
				{ // an 'automatic'
					if (actions & SB_FIRE)
					{
						// we are an AUTOMATIC.  Fire again...
						if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_RANDOMRESTART)
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
