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

BEGIN_DUKE_NS

int operateTripbomb(int snum);

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoFire(struct player_struct *p, short snum)
{
	int i;

	if(aplWeaponWorksLike[p->curr_weapon][snum]!=KNEE_WEAPON)
	{
		p->ammo_amount[p->curr_weapon]--;
	}
	
	if(aplWeaponFireSound[p->curr_weapon][snum])
	{
		S_PlayActorSound(aplWeaponFireSound[p->curr_weapon][snum],p->i);
	}
	
	SetGameVarID(g_iWeaponVarID,p->curr_weapon,p->i,snum);
	SetGameVarID(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum], p->i, snum);
	fi.shoot(p->i,aplWeaponShoots[p->curr_weapon][snum]);
	for(i=1;i<aplWeaponShotsPerBurst[p->curr_weapon][snum];i++)
	{
		fi.shoot(p->i,aplWeaponShoots[p->curr_weapon][snum]);
		if( aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_AMMOPERSHOT)
		{
			p->ammo_amount[p->curr_weapon]--;
		}
	}
						
	if(! (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_NOVISIBLE ))
	{
		// make them visible if not set...
		lastvisinc = gameclock+32;
		p->visibility = 0;
	}
	
	if( //!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_CHECKATRELOAD) &&
	   aplWeaponReload[p->curr_weapon][snum] > aplWeaponTotalTime[p->curr_weapon][snum]
	   && p->ammo_amount[p->curr_weapon] > 0
	   && (aplWeaponClip[p->curr_weapon][snum])
	   && ((p->ammo_amount[p->curr_weapon]%(aplWeaponClip[p->curr_weapon][snum]))==0) 
	  )
	{
		// do clip check...
		p->kickback_pic=aplWeaponTotalTime[p->curr_weapon][snum];
		// is same as p->kickback_pic....
	}

	if(aplWeaponWorksLike[p->curr_weapon][snum]!=KNEE_WEAPON)
	{
		checkavailweapon(p);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoSpawn(struct player_struct *p, short snum)
{
	int j;
	if(!aplWeaponSpawn[p->curr_weapon][snum])
		return;
		
	j = fi.spawn(p->i, aplWeaponSpawn[p->curr_weapon][snum]);
	
	if((aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_SPAWNTYPE2 ) )
	{
		// like shotgun shells
		sprite[j].ang += 1024;
		ssp(j,CLIPMASK0);
		sprite[j].ang += 1024;
//		p->kickback_pic++;
	}
	else if((aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_SPAWNTYPE3 ) )
	{
		// like chaingun shells
		sprite[j].ang += 1024;
		sprite[j].ang &= 2047;
		sprite[j].xvel += 32;
		sprite[j].z += (3<<8);
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
	int pi = p->i;

	p->crack_time = 777;

	if (p->holster_weapon == 1)
	{
		if (p->last_pissed_time <= (26 * 218) && p->weapon_pos == -9)
		{
			p->holster_weapon = 0;
			p->weapon_pos = 10;
			FTA(74, p);
		}
	}
	else
	{
		SetGameVarID(g_iReturnVarID, 0, pi, snum);
		SetGameVarID(g_iWeaponVarID, p->curr_weapon, pi, snum);
		SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[p->curr_weapon][snum], pi, snum);
		OnEvent(EVENT_FIRE, pi, snum, -1);
		if (GetGameVarID(g_iReturnVarID, pi, snum) == 0)
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
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
					}
				}
				break;
			case HANDREMOTE_WEAPON:
				p->hbomb_hold_delay = 0;
				p->kickback_pic = 1;
				if (aplWeaponInitialSound[p->curr_weapon][snum])
				{
					S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
				}
				break;

			case PISTOL_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					//                    p->ammo_amount[p->curr_weapon]--;
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
					}
				}
				break;


			case CHAINGUN_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
					}
				}
				break;

			case SHOTGUN_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0 && p->random_club_frame == 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
					}
				}
				break;
			case TRIPBOMB_WEAPON:
				if (operateTripbomb(snum))
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
					}
				}
				break;

			case SHRINKER_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
					}
				}
				break;

			case GROW_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
					}
				}
				break;

			case FREEZE_WEAPON:
				if (p->ammo_amount[p->curr_weapon] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
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
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
					}
				}
				break;

			case RPG_WEAPON:
				if (p->ammo_amount[RPG_WEAPON] > 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
					}
				}
				break;

			case KNEE_WEAPON:
				if (p->quick_kick == 0)
				{
					p->kickback_pic = 1;
					if (aplWeaponInitialSound[p->curr_weapon][snum])
					{
						S_PlayActorSound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
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

void operateweapon_ww(int snum, ESyncBits sb_snum, int psect)
{
	auto p = &ps[snum];
	int pi = p->i;
	int i, j, k;
	int psectlotag = sector[psect].lotag;

	// already firing...
	if (aplWeaponWorksLike[p->curr_weapon][snum] == HANDBOMB_WEAPON)
	{
		if (aplWeaponHoldDelay[p->curr_weapon][snum]	// there is a hold delay
			&& (p->kickback_pic == aplWeaponFireDelay[p->curr_weapon][snum])	// and we are 'at' hold
			&& (sb_snum & SKB_FIRE)	// and 'fire' button is still down
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

			if (p->on_ground && (sb_snum & SKB_CROUCH))
			{
				k = 15;
				i = ((p->gethorizsum() - 100) * 20);
			}
			else
			{
				k = 140;
				i = -512 - ((p->gethorizsum() - 100) * 20);
			}

			j = EGS(p->cursectnum,
				p->posx + (sintable[(p->getang() + 512) & 2047] >> 6),
				p->posy + (sintable[p->getang() & 2047] >> 6),
				p->posz, HEAVYHBOMB, -16, 9, 9,
				p->getang(), (k + (p->hbomb_hold_delay << 5)), i, pi, 1);

			{
				long lGrenadeLifetime = GetGameVar("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, -1, snum);
				long lGrenadeLifetimeVar = GetGameVar("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, -1, snum);
				// set timer.  blows up when at zero....
				sprite[j].extra = lGrenadeLifetime
					+ mulscale(krand(), lGrenadeLifetimeVar, 14)
					- lGrenadeLifetimeVar;
			}

			if (k == 15)
			{
				sprite[j].yvel = 3;
				sprite[j].z += (8 << 8);
			}

			k = hits(pi);
			if (k < 512)
			{
				sprite[j].ang += 1024;
				sprite[j].zvel /= 3;
				sprite[j].xvel /= 3;
			}

			p->hbomb_on = 1;

		}
		else if (p->kickback_pic < aplWeaponHoldDelay[p->curr_weapon][snum] &&
			(sb_snum & SKB_CROUCH))
		{
			p->hbomb_hold_delay++;
		}
		else if (p->kickback_pic > aplWeaponTotalTime[p->curr_weapon][snum])
		{
			p->kickback_pic = 0;
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
					lastvisinc = gameclock + 32;
					p->visibility = 0;
				}
				SetGameVarID(g_iWeaponVarID, p->curr_weapon, p->i, snum);
				SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[p->curr_weapon][snum], p->i, snum);
				fi.shoot(pi, aplWeaponShoots[p->curr_weapon][snum]);
			}
		}

		if (p->kickback_pic >= aplWeaponTotalTime[p->curr_weapon][snum])
		{
			p->kickback_pic = 0;
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
			p->posz = p->oposz;
			p->poszv = 0;
		}
		if (p->kickback_pic == aplWeaponSound2Time[p->curr_weapon][snum])
		{
			if (aplWeaponSound2Sound[p->curr_weapon][snum])
			{
				S_PlayActorSound(aplWeaponSound2Sound[p->curr_weapon][snum], pi);
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
				if ((sb_snum & SKB_FIRE) == 0)
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
					S_PlayActorSound(EJECT_CLIP, pi);
				}
				else if (p->kickback_pic == (aplWeaponReload[p->curr_weapon][snum] - (i / 3)))
				{
					// insert occurs 2/3 of way through reload delay
					S_PlayActorSound(INSERT_CLIP, pi);
				}

				if (p->kickback_pic >= (aplWeaponReload[p->curr_weapon][snum]))
				{
					p->kickback_pic = 0;
				}

			}
			else
			{
				if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_AUTOMATIC)
				{ // an 'automatic'
					if (sb_snum & SKB_FIRE)
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
						p->kickback_pic = 0;
					}
				}
				else
				{ // not 'automatic' and >totaltime
					p->kickback_pic = 0;
				}
			}
		}
	} // process the event ourselves if no handler provided.
}


END_DUKE_NS
