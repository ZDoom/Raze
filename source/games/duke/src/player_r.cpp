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
#include "mapinfo.h"
#include "dukeactor.h"

EXTERN_CVAR(Float, cl_viewtiltscale);
CVAR(Bool, cl_rrvehicletilting, false, CVAR_ARCHIVE);

BEGIN_DUKE_NS 


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void incur_damage_r(DDukePlayer* p)
{
	int  damage = 0, shield_damage = 0;
	int gut = 0;

	p->GetActor()->spr.extra -= p->extra_extra8 >> 8;

	damage = p->GetActor()->spr.extra - p->last_extra;
	if (damage < 0)
	{
		p->extra_extra8 = 0;

		if (p->steroids_amount > 0 && p->steroids_amount < 400)
		{
			shield_damage = damage * (20 + (krand() % 30)) / 100;
			damage -= shield_damage;
		}
		if (p->drink_amt > 31 && p->drink_amt < 65)
			gut++;
		if (p->eat > 31 && p->eat < 65)
			gut++;

		switch (gut)
		{
		case 1:
			damage = damage * 3 / 4;
			break;
		case 2:
			damage /= 4;
			break;
		}

		p->GetActor()->spr.extra = p->last_extra + damage;
	}
}

//---------------------------------------------------------------------------
//
// this is one lousy hack job...
//
//---------------------------------------------------------------------------

void selectweapon_r(DDukePlayer* const p, int weap)
{
	int i, j, k;
	if (p->last_pissed_time <= (26 * 218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && p->GetActor()->spr.scale.X > 0.125  && p->access_incs == 0 && p->knee_incs == 0)
	{
		if ((p->weapon_pos == 0 || (p->holster_weapon && p->weapon_pos == -9)))
		{
			if (weap == WeaponSel_Alt)
			{
				j = p->curr_weapon;
				switch (p->curr_weapon)
				{
					case THROWSAW_WEAPON:
						if (p->ammo_amount[BUZZSAW_WEAPON] > 0)
						{
							j = BUZZSAW_WEAPON;
							p->subweapon = 1 << BUZZSAW_WEAPON;
						}
						break;
					case BUZZSAW_WEAPON:
						if (p->ammo_amount[THROWSAW_WEAPON] > 0)
						{
							j = THROWSAW_WEAPON;
							p->subweapon = 0;
						}
						break;
					case POWDERKEG_WEAPON:
						if (p->ammo_amount[BOWLING_WEAPON] > 0)
						{
							j = BOWLING_WEAPON;
							p->subweapon = 1 << BOWLING_WEAPON;
						}
						break;
					case BOWLING_WEAPON:
						if (p->ammo_amount[POWDERKEG_WEAPON] > 0)
						{
							j = POWDERKEG_WEAPON;
							p->subweapon = 0;
						}
						break;
					case KNEE_WEAPON:
						if (isRRRA())
						{
							j = SLINGBLADE_WEAPON;
							p->subweapon = 2;
						}
						break;
					case SLINGBLADE_WEAPON:
						j = KNEE_WEAPON;
						p->subweapon = 0;
						break;
					case CROSSBOW_WEAPON:
						if (p->ammo_amount[CHICKEN_WEAPON] > 0 && isRRRA())
						{
							j = CHICKEN_WEAPON;
							p->subweapon = 4;
						}
						break;
					case CHICKEN_WEAPON:
						if (p->ammo_amount[CROSSBOW_WEAPON] > 0)
						{
							j = CROSSBOW_WEAPON;
							p->subweapon = 0;
						}
						break;
					default:
						break;
				}
			}
			else if (weap == WeaponSel_Next || weap == WeaponSel_Prev)
			{
				k = p->curr_weapon;
				if (isRRRA())
				{
					if (k == CHICKEN_WEAPON) k = CROSSBOW_WEAPON;
					else if (k == BUZZSAW_WEAPON) k = THROWSAW_WEAPON;
					else if (k == SLINGBLADE_WEAPON) k = KNEE_WEAPON;
				}
				j = (weap == WeaponSel_Prev ? -1 : 1);	// JBF: prev (-1) or next (1) weapon choice
				i = 0;

				while (k >= 0 && k < 10)
				{
					k += j;
					if (k == -1) k = 9;
					else if (k == 10) k = 0;

					if (p->gotweapon[k] && p->ammo_amount[k] > 0)
					{
						j = k;
						break;
					}

					i++;
					if (i == 10)
					{
						fi.addweapon(p, KNEE_WEAPON, true);
						break;
					}
				}
			}
			else j = weap - 1;

			k = -1;

			if (j == DYNAMITE_WEAPON && p->ammo_amount[DYNAMITE_WEAPON] == 0)
			{
				DukeStatIterator it(STAT_ACTOR);
				while (auto act = it.Next())
				{
					if (act->GetClass() == RedneckDynamiteClass && act->GetOwner() == p->GetActor())
					{
						p->gotweapon[DYNAMITE_WEAPON] = true;
						j = THROWINGDYNAMITE_WEAPON;
						break;
					}
				}
			}
			else if (j == KNEE_WEAPON && isRRRA())
			{
				if (p->curr_weapon == KNEE_WEAPON)
				{
					p->subweapon = 2;
					j = SLINGBLADE_WEAPON;
				}
				else if (p->subweapon & 2)
				{
					p->subweapon = 0;
					j = KNEE_WEAPON;
				}
			}
			else if (j == CROSSBOW_WEAPON && isRRRA())
			{
				if (p->curr_weapon == CROSSBOW_WEAPON || p->ammo_amount[CROSSBOW_WEAPON] == 0)
				{
					if (p->ammo_amount[CHICKEN_WEAPON] == 0)
						return;
					p->subweapon = 4;
					j = CHICKEN_WEAPON;
				}
				else if ((p->subweapon & 4) || p->ammo_amount[CHICKEN_WEAPON] == 0)
				{
					p->subweapon = 0;
					j = CROSSBOW_WEAPON;
				}
			}
			else if (j == THROWSAW_WEAPON)
			{
				if (p->curr_weapon == THROWSAW_WEAPON || p->ammo_amount[THROWSAW_WEAPON] == 0)
				{
					p->subweapon = (1 << BUZZSAW_WEAPON);
					j = BUZZSAW_WEAPON;
				}
				else if ((p->subweapon & (1 << BUZZSAW_WEAPON)) || p->ammo_amount[BUZZSAW_WEAPON] == 0)
				{
					p->subweapon = 0;
					j = THROWSAW_WEAPON;
				}
			}
			else if (j == POWDERKEG_WEAPON)
			{
				if (p->curr_weapon == POWDERKEG_WEAPON || p->ammo_amount[POWDERKEG_WEAPON] == 0)
				{
					p->subweapon = (1 << BOWLING_WEAPON);
					j = BOWLING_WEAPON;
				}
				else if ((p->subweapon & (1 << BOWLING_WEAPON)) || p->ammo_amount[BOWLING_WEAPON] == 0)
				{
					p->subweapon = 0;
					j = POWDERKEG_WEAPON;
				}
			}


			if (p->holster_weapon)
			{
				p->cmd.ucmd.actions |= SB_HOLSTER;
				p->oweapon_pos = p->weapon_pos = -9;
			}
			else if (j >= MIN_WEAPON && p->gotweapon[j] && p->curr_weapon != j) switch (j)
			{
			case KNEE_WEAPON:
				fi.addweapon(p, j, true);
				break;
			case SLINGBLADE_WEAPON:
				if (isRRRA())
				{
					S_PlayActorSound(496, getPlayer(screenpeek)->GetActor());
					fi.addweapon(p, j, true);
				}
				break;

			case PISTOL_WEAPON:
				if (p->ammo_amount[PISTOL_WEAPON] == 0)
					if (p->show_empty_weapon == 0)
					{
						p->last_full_weapon = p->curr_weapon;
						p->show_empty_weapon = 32;
					}
				fi.addweapon(p, PISTOL_WEAPON, true);
				break;

			case CHICKEN_WEAPON:
				if (!isRRRA()) break;
				[[fallthrough]];
			case SHOTGUN_WEAPON:
			case RIFLEGUN_WEAPON:
			case CROSSBOW_WEAPON:
			case TIT_WEAPON:
			case ALIENBLASTER_WEAPON:
			case THROWSAW_WEAPON:
			case BUZZSAW_WEAPON:
			case POWDERKEG_WEAPON:
			case BOWLING_WEAPON:
				if (p->ammo_amount[j] == 0 && p->show_empty_weapon == 0)
				{
					p->last_full_weapon = p->curr_weapon;
					p->show_empty_weapon = 32;
				}
				fi.addweapon(p, j, true);
				break;

			case MOTORCYCLE_WEAPON:
			case BOAT_WEAPON:
				if (isRRRA())
				{
					if (p->ammo_amount[j] == 0 && p->show_empty_weapon == 0)
					{
						p->show_empty_weapon = 32;
					}
					fi.addweapon(p, j, true);
				}
				break;

			case THROWINGDYNAMITE_WEAPON:
				if (k >= 0) // Found in list of [1]'s
				{
					p->curr_weapon = THROWINGDYNAMITE_WEAPON;
					p->last_weapon = -1;
					p->oweapon_pos = p->weapon_pos = 10;
				}
				break;
			case DYNAMITE_WEAPON:
				if (p->ammo_amount[DYNAMITE_WEAPON] > 0 && p->gotweapon[DYNAMITE_WEAPON])
					fi.addweapon(p, DYNAMITE_WEAPON, true);
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

int doincrements_r(DDukePlayer* p)
{
	int snum;
	auto pact = p->GetActor();

	if (isRRRA())
	{
		if (WindTime > 0)
			WindTime--;
		else if ((krand() & 127) == 8)
		{
			WindTime = 120 + ((krand() & 63) << 2);
			WindDir = randomAngle();
		}

		if (chickenphase > 0)
			chickenphase--;
		if (p->SeaSick)
		{
			p->SeaSick--;
			if (p->SeaSick == 0)
				p->sea_sick_stat = 0;
		}
	}

	snum = p->GetActor()->PlayerIndex();

	p->player_par++;
	if (p->yehaa_timer)
		p->yehaa_timer--;


	if (p->detonate_count > 0)
	{
		p->detonate_count++;
		p->detonate_time--;
	}
	p->drink_timer--;
	if (p->drink_timer <= 0)
	{
		p->drink_timer = 1024;
		if (p->drink_amt)
		{
			p->drink_amt--;
		}
	}
	p->eat_timer--;
	if (p->eat_timer <= 0)
	{
		p->eat_timer = 1024;
		if (p->eat)
			p->eat--;
	}
	if (p->drink_amt >= 100)
	{
		if (!S_CheckActorSoundPlaying(pact, 420))
			S_PlayActorSound(420, pact);
		p->drink_amt -= 9;
		p->eat >>= 1;
	}
	p->eatang = (1647 + p->eat * 8) & 2047;

	if (p->eat >= 100)
		p->eat = 100;

	if (p->eat >= 31 && krand() < p->eat)
	{
		switch (krand() & 3)
		{
		case 0:
			S_PlayActorSound(404, pact);
			break;
		case 1:
			S_PlayActorSound(422, pact);
			break;
		case 2:
			S_PlayActorSound(423, pact);
			break;
		case 3:
			S_PlayActorSound(424, pact);
			break;
		}
		if (numplayers < 2)
		{
			p->noise_radius = 1024;
			madenoise(getPlayer(screenpeek));
			p->vel.XY() += p->GetActor()->spr.Angles.Yaw.ToVector();
		}
		p->eat -= 4;
		if (p->eat < 0)
			p->eat = 0;
	}

	if (p->invdisptime > 0)
		p->invdisptime--;

	if (p->tipincs > 0)
	{
		p->otipincs = p->tipincs;
		p->tipincs--;
	}

	if (p->last_pissed_time > 0)
	{
		p->last_pissed_time--;

		if (p->drink_amt > 66 && (p->last_pissed_time % 26) == 0)
			p->drink_amt--;

		{
			if (p->last_pissed_time == 5662)
				S_PlayActorSound(434, pact);
			else if (p->last_pissed_time == 5567)
				S_PlayActorSound(434, pact);
			else if (p->last_pissed_time == 5472)
				S_PlayActorSound(433, pact);
			else if (p->last_pissed_time == 5072)
				S_PlayActorSound(435, pact);
			else if (p->last_pissed_time == 5014)
				S_PlayActorSound(434, pact);
			else if (p->last_pissed_time == 4919)
				S_PlayActorSound(433, pact);
		}

		if (p->last_pissed_time == 5668)
		{
			p->holster_weapon = 0;
			p->oweapon_pos = p->weapon_pos = 10;
		}
	}

	if (p->crack_time > 0)
	{
		p->crack_time--;
		if (p->crack_time == 0)
		{
			p->knuckle_incs = 1;
			p->crack_time = CRACK_TIME;
		}
	}

	if (p->steroids_amount > 0 && p->steroids_amount < 400)
	{
		p->steroids_amount--;
		if (p->steroids_amount == 0)
		{
			checkavailinven(p);
			p->eat = p->drink_amt = 0;
			p->eatang = p->drunkang = 1647;
		}
		if (!(p->steroids_amount & 14))
			if (snum == screenpeek || ud.coop == 1)
				S_PlayActorSound(DUKE_TAKEPILLS, pact);
	}

	if (p->access_incs && p->GetActor()->spr.pal != 1)
	{
		p->oaccess_incs = p->access_incs;
		p->access_incs++;
		if (p->GetActor()->spr.extra <= 0)
			p->access_incs = 12;
		if (p->access_incs == 12)
		{
			if (p->access_spritenum != nullptr)
			{
				checkhitswitch(snum, nullptr, p->access_spritenum);
				switch (p->access_spritenum->spr.pal)
				{
				case 0:p->keys[1] = 1; break;
				case 21:p->keys[2] = 1; break;
				case 23:p->keys[3] = 1; break;
				}
				p->access_spritenum = nullptr;
			}
			else
			{
				checkhitswitch(snum, p->access_wall, nullptr);
				switch (p->access_wall->pal)
				{
				case 0:p->keys[1] = 1; break;
				case 21:p->keys[2] = 1; break;
				case 23:p->keys[3] = 1; break;
				}
			}
		}

		if (p->access_incs > 20)
		{
			p->oaccess_incs = p->access_incs = 0;
			p->oweapon_pos = p->weapon_pos = 10;
			p->okickback_pic = p->kickback_pic = 0;
		}
	}

	if (p->scuba_on == 0 && p->insector() && p->cursector->lotag == 2)
	{
		if (p->scuba_amount > 0)
		{
			p->scuba_on = 1;
			p->inven_icon = 6;
			FTA(76, p);
		}
		else
		{
			if (p->airleft > 0)
				p->airleft--;
			else
			{
				p->extra_extra8 += 32;
				if (p->last_extra < (gs.max_player_health >> 1) && (p->last_extra & 3) == 0)
					S_PlayActorSound(DUKE_LONGTERM_PAIN, pact);
			}
		}
	}
	else if (p->scuba_amount > 0 && p->scuba_on)
	{
		p->scuba_amount--;
		if (p->scuba_amount == 0)
		{
			p->scuba_on = 0;
			checkavailinven(p);
		}
	}

	if (p->knuckle_incs)
	{
		p->knuckle_incs++;
		if (p->knuckle_incs == 10)
		{
			if (!wupass)
			{
				int snd = currentLevel->rr_startsound ? currentLevel->rr_startsound : 391;
				wupass = 1;
				S_PlayActorSound(snd, pact);
			}
			else if (PlayClock > 1024)
				if (snum == screenpeek || ud.coop == 1)
				{
					if (rand() & 1)
						S_PlayActorSound(DUKE_CRACK, pact);
					else S_PlayActorSound(DUKE_CRACK2, pact);
				}
		}
		else if (p->knuckle_incs == 22 || !!(p->cmd.ucmd.actions & SB_FIRE))
			p->knuckle_incs = 0;

		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void checkweapons_r(DDukePlayer* p)
{
	static  PClassActor* const* const weapon_sprites[MAX_WEAPONS] = { &DukeMeleeAttackClass, &RedneckRevolverClass, &RedneckShotgunClass,
		&RedneckRiflegunClass, &RedneckDynamiteClass, &RedneckCrossbowClass, &RedneckRipsawClass, &RedneckBlasterClass,
			&RedneckPowderKegClass, &RedneckTitgunClass, &RedneckDynamiteClass, &RedneckRipsawClass, &RedneckBowlingBallClass, 
			nullptr, nullptr, nullptr, &RedneckCrossbowClass };

	if (isRRRA())
	{
		if (p->OnMotorcycle && numplayers > 1)
		{
			auto j = spawn(p->GetActor(), RedneckEmptyBikeClass);
			if (j)
			{
				j->spr.Angles.Yaw = p->GetActor()->spr.Angles.Yaw;
				j->saved_ammo = p->ammo_amount[MOTORCYCLE_WEAPON];
			}
			p->OnMotorcycle = 0;
			p->gotweapon[MOTORCYCLE_WEAPON] = false;
			p->GetActor()->spr.Angles.Pitch = nullAngle;
			p->moto_do_bump = 0;
			p->MotoSpeed = 0;
			p->TiltStatus = nullAngle;
			p->moto_drink = 0;
			p->VBumpTarget = 0;
			p->VBumpNow = 0;
			p->TurbCount = 0;
		}
		else if (p->OnBoat && numplayers > 1)
		{
			auto j = spawn(p->GetActor(), RedneckEmptyBoatClass);
			if (j)
			{
				j->spr.Angles.Yaw = p->GetActor()->spr.Angles.Yaw;
				j->saved_ammo = p->ammo_amount[BOAT_WEAPON];
			}
			p->OnBoat = 0;
			p->gotweapon[BOAT_WEAPON] = false;
			p->GetActor()->spr.Angles.Pitch = nullAngle;
			p->moto_do_bump = 0;
			p->MotoSpeed = 0;
			p->TiltStatus = nullAngle;
			p->moto_drink = 0;
			p->VBumpTarget = 0;
			p->VBumpNow = 0;
			p->TurbCount = 0;
		}
	}

	if (p->curr_weapon > 0)
	{
		if (krand() & 1)
		{
			auto weap = weapon_sprites[p->curr_weapon];
			if (weap && *weap) spawn(p->GetActor(), *weap);
		}
		else switch (p->curr_weapon)
		{
		case CHICKEN_WEAPON:
			if (!isRRRA()) break;
			[[fallthrough]];
		case DYNAMITE_WEAPON:
		case CROSSBOW_WEAPON:
			spawn(p->GetActor(), DukeExplosion2Class);
			break;
		}
	}

	for (int i = 0; i < 5; i++)
	{
		if (p->keys[i] == 1)
		{
			auto j = spawn(p->GetActor(), RedneckDoorkeyClass);
			if (j) switch (i)
			{
			case 1:
				j->spr.lotag = 100;
				break;
			case 2:
				j->spr.lotag = 101;
				break;
			case 3:
				j->spr.lotag = 102;
				break;
			case 4:
				j->spr.lotag = 103;
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

enum : unsigned
{
	VEH_FORWARD = 1,
	VEH_REVERSE = 2,
	VEH_TURNLEFT = 4,
	VEH_TURNRIGHT = 8,
	VEH_TURNING = VEH_TURNLEFT|VEH_TURNRIGHT,
	VEH_BRAKING = 16,
	VEH_FWDBRAKING = VEH_FORWARD|VEH_BRAKING,
};

static unsigned outVehicleFlags(DDukePlayer* p, ESyncBits& actions)
{
	unsigned flags = 0;
	flags += VEH_FORWARD * (p->cmd.ucmd.vel.X > 0);
	flags += VEH_REVERSE * (p->cmd.ucmd.vel.X < 0);
	flags += VEH_TURNLEFT * (p->cmd.ucmd.ang.Yaw.Degrees() < 0);
	flags += VEH_TURNRIGHT * (p->cmd.ucmd.ang.Yaw.Degrees() > 0);
	flags += VEH_BRAKING * (!!(actions & SB_CROUCH) || (p->cmd.ucmd.vel.X < 0 && p->MotoSpeed > 0));
	actions &= ~SB_CROUCH;
	return flags;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doVehicleTilting(DDukePlayer* const p, const bool canTilt)
{
	auto adj = p->cmd.ucmd.ang.Yaw * (545943. / 3200000.) * canTilt;
	if (p->OnMotorcycle) adj *= 5 * Sgn(p->MotoSpeed);
	if (cl_rrvehicletilting) adj *= cl_viewtiltscale;
	p->oTiltStatus = p->TiltStatus;

	scaletozero(p->TiltStatus, 10.);
	p->GetActor()->spr.Angles.Roll = (p->TiltStatus += adj) * cl_rrvehicletilting;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doVehicleBumping(DDukePlayer* p, DDukeActor* pact, unsigned flags, bool bumptest, int bumpscale)
{
	if (p->MotoSpeed != 0 && p->on_ground == 1)
	{
		if (!p->VBumpNow && bumptest)
			p->VBumpTarget = p->MotoSpeed * (1. / 16.) * bumpscale;

		if ((flags & VEH_TURNLEFT) || p->moto_drink < 0)
		{
			if (p->moto_drink < 0)
				p->moto_drink++;
		}
		else if ((flags & VEH_TURNRIGHT) || p->moto_drink > 0)
		{
			if (p->moto_drink > 0)
				p->moto_drink--;
		}
	}

	if (p->TurbCount)
	{
		if (p->TurbCount <= 1)
		{
			p->TurbCount = 0;
			p->VBumpTarget = 0;
			p->VBumpNow = 0;
		}
		else
		{
			p->TurbCount--;
			p->moto_drink = (krand() & 3) - 2;
			pact->spr.Angles.Pitch = -maphoriz((krand() & 15) - 7);
		}
	}
	else if (p->VBumpTarget > p->VBumpNow)
	{
		p->VBumpNow += p->moto_bump_fast ? 6 : 1;
		if (p->VBumpTarget < p->VBumpNow)
			p->VBumpNow = p->VBumpTarget;
		pact->spr.Angles.Pitch = -maphoriz(p->VBumpNow * (1. / 3.));
	}
	else if (p->VBumpTarget < p->VBumpNow)
	{
		p->VBumpNow -= p->moto_bump_fast ? 6 : 1;
		if (p->VBumpTarget > p->VBumpNow)
			p->VBumpNow = p->VBumpTarget;
		pact->spr.Angles.Pitch = -maphoriz(p->VBumpNow * (1. / 3.));
	}
	else
	{
		p->VBumpTarget = 0;
		p->moto_bump_fast = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doVehicleDrunk(DDukePlayer* const p)
{
	if (p->drink_amt > 88 && p->moto_drink == 0)
	{
		const int rng = krand() & 63;
		if (rng == 1)
			p->moto_drink = -10;
		else if (rng == 2)
			p->moto_drink = 10;
	}
	else if (p->drink_amt > 99 && p->moto_drink == 0)
	{
		const int rng = krand() & 31;
		if (rng == 1)
			p->moto_drink = -20;
		else if (rng == 2)
			p->moto_drink = 20;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doVehicleSounds(DDukePlayer* p, DDukeActor* pact, unsigned flags, unsigned sound1, unsigned sound2, unsigned sound3, unsigned sound4)
{
	if ((p->OnBoat && (flags & VEH_FWDBRAKING) == VEH_FORWARD) || flags & VEH_FORWARD)
	{
		if (p->OnBoat || p->on_ground)
		{
			if (p->OnMotorcycle && p->MotoSpeed == 0 && (flags & VEH_BRAKING))
			{
				if (!S_CheckActorSoundPlaying(pact, sound1))
					S_PlayActorSound(sound1, pact);
			}
			else if (p->MotoSpeed == 0 && !S_CheckActorSoundPlaying(pact, sound3))
			{
				if (S_CheckActorSoundPlaying(pact, sound1))
					S_StopSound(sound1, pact);
				S_PlayActorSound(sound3, pact);
			}
			else if (p->MotoSpeed >= 50 && !S_CheckActorSoundPlaying(pact, sound2))
			{
				S_PlayActorSound(sound2, pact);
			}
			else if (!S_CheckActorSoundPlaying(pact, sound2) && !S_CheckActorSoundPlaying(pact, sound3))
			{
				S_PlayActorSound(sound2, pact);
			}
		}
	}
	else
	{
		if (S_CheckActorSoundPlaying(pact, sound3))
		{
			S_StopSound(sound3, pact);
			if (!S_CheckActorSoundPlaying(pact, sound4))
				S_PlayActorSound(sound4, pact);
		}
		if (S_CheckActorSoundPlaying(pact, sound2))
		{
			S_StopSound(sound2, pact);
			if (!S_CheckActorSoundPlaying(pact, sound4))
				S_PlayActorSound(sound4, pact);
		}
		if (!S_CheckActorSoundPlaying(pact, sound4) && !S_CheckActorSoundPlaying(pact, sound1))
			S_PlayActorSound(sound1, pact);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doVehicleThrottling(DDukePlayer* p, DDukeActor* pact, unsigned& flags, int fwdSpeed, int revSpeed, int brakeSpeed, int vBmpFwd, int vBmpBrake)
{
	if (p->on_ground == 1)
	{
		if (p->OnBoat && (flags & VEH_FWDBRAKING) == VEH_FWDBRAKING)
		{
			if (p->MotoSpeed <= 25)
			{
				p->MotoSpeed++;
				if (!S_CheckActorSoundPlaying(pact, 182))
					S_PlayActorSound(182, pact);
			}
			else
			{
				p->MotoSpeed -= brakeSpeed;
				if (p->MotoSpeed < 0)
					p->MotoSpeed = 0;
				p->VBumpTarget = vBmpBrake;
				p->moto_do_bump = 1;
			}
		}
		else if ((flags & VEH_BRAKING) && p->MotoSpeed > 0)
		{
			const auto kbdBraking = brakeSpeed * !!(flags & VEH_BRAKING);
			const auto hidBraking = brakeSpeed * p->cmd.ucmd.vel.X * (p->cmd.ucmd.vel.X < 0);
			p->MotoSpeed -= clamp<double>(kbdBraking - hidBraking, -brakeSpeed, brakeSpeed);
			if (p->MotoSpeed < 0)
				p->MotoSpeed = 0;
			p->VBumpTarget = vBmpBrake;
			p->moto_do_bump = 1;
		}
		else if ((flags & VEH_FORWARD) && (p->OnBoat || !(flags & VEH_BRAKING)))
		{
			if (p->MotoSpeed < 40 && (p->OnMotorcycle || !p->NotOnWater))
			{
				p->VBumpTarget = vBmpFwd;
				p->moto_bump_fast = 1;
			}

			p->MotoSpeed += fwdSpeed * p->cmd.ucmd.vel.X;
			flags &= ~VEH_FORWARD;

			if (p->MotoSpeed > 120)
				p->MotoSpeed = 120;

			if (p->OnMotorcycle && !p->NotOnWater && p->MotoSpeed > 80)
				p->MotoSpeed = 80;
		}
		else if (p->MotoSpeed > 0)
			p->MotoSpeed--;

		if (p->moto_do_bump && (!(flags & VEH_BRAKING) || p->MotoSpeed == 0))
		{
			p->VBumpTarget = 0;
			p->moto_do_bump = 0;
		}

		if ((flags & VEH_REVERSE) && p->MotoSpeed <= 0 && !(flags & VEH_BRAKING))
		{
			if (flags & VEH_TURNLEFT)
			{
				flags &= ~VEH_TURNLEFT;
				flags |= VEH_TURNRIGHT;
			}
			else
			{
				flags &= ~VEH_TURNRIGHT;
				flags |= VEH_TURNLEFT;
			}
			p->MotoSpeed = revSpeed * p->cmd.ucmd.vel.X;
			flags &= ~VEH_REVERSE;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void onMotorcycle(int snum, ESyncBits &actions)
{
	auto p = getPlayer(snum);
	auto pact = p->GetActor();

	unsigned flags = outVehicleFlags(p, actions);
	doVehicleTilting(p, !p->on_ground || p->cmd.ucmd.ang.Yaw.Degrees());

	if (p->MotoSpeed < 0 || p->moto_underwater)
		p->MotoSpeed = 0;

	doVehicleSounds(p, pact, flags, 187, 188, 214, 189);
	doVehicleDrunk(p);
	doVehicleThrottling(p, pact, flags, 2, 15, p->moto_on_oil ? 2 : 4, 70, -30);
	doVehicleBumping(p, pact, flags, (krand() & 3) == 2, (krand() & 7) - 4);

	constexpr DAngle adjust = mapangle(-510);
	DAngle velAdjustment;

	int currSpeed = int(p->MotoSpeed);
	if (p->MotoSpeed >= 20 && p->on_ground == 1 && (flags & VEH_TURNING))
	{
		velAdjustment = (flags & VEH_TURNLEFT) ? -adjust : adjust;
		auto angAdjustment = (350 << 21) * velAdjustment.Sgn();

		if (p->moto_on_mud || p->moto_on_oil || !p->NotOnWater)
		{
			currSpeed <<= p->moto_on_oil ? 3 : 2;

			if (p->moto_do_bump)
			{
				currSpeed >>= 5;
				angAdjustment >>= 2;
			}
			else
			{
				currSpeed >>= 7;
				angAdjustment >>= 6;
			}

			p->moto_on_mud = 0;
			p->moto_on_oil = 0;
		}
		else
		{
			if (p->moto_do_bump)
			{
				currSpeed >>= 5;
				angAdjustment >>= 4;
				if (!S_CheckActorSoundPlaying(pact, 220))
					S_PlayActorSound(220, pact);
			}
			else
			{
				currSpeed >>= 7;
				angAdjustment >>= 7;
			}
		}

		p->vel.XY() += (pact->spr.Angles.Yaw + velAdjustment).ToVector() * currSpeed;
		pact->spr.Angles.Yaw -= DAngle::fromBam(angAdjustment);
	}
	else if (p->MotoSpeed >= 20 && p->on_ground == 1 && (p->moto_on_mud || p->moto_on_oil))
	{
		velAdjustment = krand() & 1 ? adjust : -adjust;
		currSpeed = MulScale(currSpeed, p->moto_on_oil ? 10 : 5, 7);
		p->vel.XY() += (pact->spr.Angles.Yaw + velAdjustment).ToVector() * currSpeed;
	}

	p->moto_on_mud = p->moto_on_oil = 0;
	p->cmd.ucmd.vel.X = clamp<float>((float)p->MotoSpeed, -15.f, 120.f) * (1.f / 40.f);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void onBoat(int snum, ESyncBits &actions)
{
	auto p = getPlayer(snum);
	auto pact = p->GetActor();

	if (p->NotOnWater)
	{
		if (p->MotoSpeed > 0)
		{
			if (!S_CheckActorSoundPlaying(pact, 88))
				S_PlayActorSound(88, pact);
		}
		else
		{
			if (!S_CheckActorSoundPlaying(pact, 87))
				S_PlayActorSound(87, pact);
		}
	}

	if (p->MotoSpeed < 0)
		p->MotoSpeed = 0;

	unsigned flags = outVehicleFlags(p, actions);
	doVehicleTilting(p, (p->MotoSpeed != 0 && (p->cmd.ucmd.ang.Yaw.Degrees() || p->moto_drink)) || !p->NotOnWater);
	doVehicleSounds(p, pact, flags, 87, 88, 89, 90);

	if (!p->NotOnWater)
	{
		if ((flags & VEH_TURNING) && !S_CheckActorSoundPlaying(pact, 91) && p->MotoSpeed > 30)
			S_PlayActorSound(91, pact);

		doVehicleDrunk(p);
	}

	doVehicleThrottling(p, pact, flags, 1, !p->NotOnWater ? 25 : 20, 2, -30, 30);
	doVehicleBumping(p, pact, flags, (krand() & 15) == 14, (krand() & 7) - 4);

	if (p->MotoSpeed > 0 && p->on_ground == 1 && (flags & VEH_TURNING))
	{
		int currSpeed = int(p->MotoSpeed * 4.);
		constexpr DAngle adjust = mapangle(-510);
		DAngle velAdjustment = (flags & VEH_TURNLEFT) ? -adjust : adjust;
		auto angAdjustment = (350 << 21) * velAdjustment.Sgn();

		if (p->moto_do_bump)
		{
			currSpeed >>= 6;
			angAdjustment >>= 5;
		}
		else
		{
			currSpeed >>= 7;
			angAdjustment >>= 6;
		}

		p->vel.XY() += (pact->spr.Angles.Yaw + velAdjustment).ToVector() * currSpeed;
		pact->spr.Angles.Yaw -= DAngle::fromBam(angAdjustment);
	}
	if (p->NotOnWater && p->MotoSpeed > 50)
		p->MotoSpeed *= 0.5;

	p->cmd.ucmd.vel.X = clamp<float>((float)p->MotoSpeed, -15.f, 120.f) * (1.f / 40.f);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void movement(DDukePlayer* const p, ESyncBits actions, sectortype* psect, double floorz, double ceilingz, int shrunk, double truefdist, int psectlotag)
{
	auto pact = p->GetActor();

	if (p->airleft != 15 * 26)
		p->airleft = 15 * 26; //Aprox twenty seconds.

	if (p->scuba_on == 1)
		p->scuba_on = 0;

	double i = gs.playerheight;
	if (psectlotag == ST_1_ABOVE_WATER && p->spritebridge == 0)
	{
		if (shrunk == 0)
		{
			i = 34;
			p->pycount += 32;
			p->pycount &= 2047;
			p->pyoff = BobVal(p->pycount) * 2;
		}
		else i = 12;

		if (shrunk == 0 && truefdist <= gs.playerheight)
		{
			if (p->on_ground == 1)
			{
				if (p->dummyplayersprite == nullptr)
					p->dummyplayersprite = spawn(pact, DukePlayerOnWaterClass);

				p->footprintcount = 6;
				if (tilesurface(p->cursector->floortexture) == TSURF_SLIME)
				{
					p->footprintpal = 8;
					p->footprintshade = 0;
				}
				else if (tilesurface(p->cursector->floortexture) == TSURF_OIL)
				{
					p->footprintpal = 0;
					p->footprintshade = 40;
				}
				else
				{
					p->footprintpal = 0;
					p->footprintshade = 0;
				}
			}
		}
	}
	else if (!p->OnMotorcycle)
	{
		footprints(p);
	}

	if (pact->getOffsetZ() < floorz - i) //falling
	{
		if ((actions & (SB_JUMP|SB_CROUCH)) == 0 && p->on_ground && (psect->floorstat & CSTAT_SECTOR_SLOPE) && pact->getOffsetZ() >= (floorz - i - 16))
			pact->spr.pos.Z = floorz - i + gs.playerheight;
		else
		{
			p->on_ground = 0;

			if ((p->OnMotorcycle || p->OnBoat) && floorz - i * 2 > pact->getOffsetZ())
			{
				if (p->MotoOnGround)
				{
					p->VBumpTarget = 80;
					p->moto_bump_fast = 1;
					p->vel.Z -= (gs.gravity * p->MotoSpeed * (1. / 16.));
					p->MotoOnGround = 0;
					if (S_CheckActorSoundPlaying(pact, 188))
						S_StopSound(188, pact);
					S_PlayActorSound(189, pact);
				}
				else
				{
					p->vel.Z += (gs.gravity - 5/16. + (int(120 - p->MotoSpeed) / 256.));
					if (!S_CheckActorSoundPlaying(pact, 189) && !S_CheckActorSoundPlaying(pact, 190))
						S_PlayActorSound(190, pact);
				}
			}
			else
				p->vel.Z += (gs.gravity + 5/16.); // (TICSPERFRAME<<6);

			if (p->vel.Z >= (16 + 8)) p->vel.Z = (16 + 8);
			if (p->vel.Z > 2400 / 256 && p->falling_counter < 255)
			{
				p->falling_counter++;
				if (p->falling_counter == 38 && !S_CheckActorSoundPlaying(pact, DUKE_SCREAM))
					S_PlayActorSound(DUKE_SCREAM, pact);
			}

			if (pact->getOffsetZ() + p->vel.Z  >= floorz - i) // hit the ground
			{
				S_StopSound(DUKE_SCREAM, pact);
				if (!p->insector() || p->cursector->lotag != 1)
				{
					if (isRRRA()) p->MotoOnGround = 1;
					if (p->falling_counter > 62 || (isRRRA() && p->falling_counter > 2 && p->insector() && p->cursector->lotag == 802))
						quickkill(p);

					else if (p->falling_counter > 9)
					{
						int j = p->falling_counter;
						pact->spr.extra -= j - (krand() & 3);
						if (pact->spr.extra <= 0)
						{
							S_PlayActorSound(SQUISHED, pact);
						}
						else
						{
							S_PlayActorSound(DUKE_LAND, pact);
							S_PlayActorSound(DUKE_LAND_HURT, pact);
						}

						SetPlayerPal(p, PalEntry(32, 16, 0, 0));
					}
					else if (p->vel.Z > 8)
					{
						if (p->OnMotorcycle)
						{
							if (S_CheckActorSoundPlaying(pact, 190))
								S_StopSound(190, pact);
							S_PlayActorSound(191, pact);
							p->TurbCount = 12;
						}
						else S_PlayActorSound(DUKE_LAND, pact);
					}
					else if (p->vel.Z > 4 && p->OnMotorcycle)
					{
						S_PlayActorSound(DUKE_LAND, pact);
						p->TurbCount = 12;
					}
				}
			}
		}
	}

	else
	{
		p->falling_counter = 0;
		S_StopSound(-1, pact, CHAN_VOICE);

		if (psectlotag != ST_1_ABOVE_WATER && psectlotag != ST_2_UNDERWATER && p->on_ground == 0 && p->vel.Z > 12)
			p->hard_landing = uint8_t(p->vel.Z / 4. );

		p->on_ground = 1;

		if (i == gs.playerheight)
		{
			//Smooth on the ground

			double k = (floorz - i - pact->getOffsetZ()) * 0.5;
			pact->spr.pos.Z += k;
			p->vel.Z -= 3;
			if (p->vel.Z < 0) p->vel.Z = 0;
		}
		else if (p->jumping_counter == 0)
		{
			pact->spr.pos.Z += ((floorz - i * 0.5) - pact->getOffsetZ()) * 0.5; //Smooth on the water
			if (p->on_warping_sector == 0 && pact->getOffsetZ() > floorz - 16)
			{
				pact->spr.pos.Z = floorz - 16 + gs.playerheight;
				p->vel.Z *= 0.5;
			}
		}

		p->on_warping_sector = 0;

		if (((actions & SB_CROUCH) || p->cmd.ucmd.vel.Z < 0) && !p->OnMotorcycle)
		{
			playerCrouch(p);
		}

		if ((actions & SB_JUMP) == 0 && !p->OnMotorcycle && p->jumping_toggle == 1)
			p->jumping_toggle = 0;

		else if ((actions & SB_JUMP) && !p->OnMotorcycle && p->jumping_toggle == 0)
		{
			playerJump(p, floorz, ceilingz);
		}
	}

	if (p->jumping_counter)
	{
		if ((actions & SB_JUMP) == 0 && !p->OnMotorcycle && p->jumping_toggle == 1)
			p->jumping_toggle = 0;

		if (p->jumping_counter < 768)
		{
			if (psectlotag == ST_1_ABOVE_WATER && p->jumping_counter > 768)
			{
				p->jumping_counter = 0;
				p->vel.Z = -2;
			}
			else
			{
				p->vel.Z -= BobVal(2048 - 128 + p->jumping_counter) * (64. / 12);
				p->jumping_counter += 180;
				p->on_ground = 0;
			}
		}
		else
		{
			p->jumping_counter = 0;
			p->vel.Z = 0;
		}
	}

	pact->spr.pos.Z += p->vel.Z;

	if (pact->getOffsetZ() < ceilingz + 4)
	{
		p->jumping_counter = 0;
		if (p->vel.Z < 0)
			p->vel.X = p->vel.Y = 0;
		p->vel.Z = 0.5;
		pact->spr.pos.Z = ceilingz + 4 + gs.playerheight;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onMotorcycleMove(int snum, walltype* wal)
{
	auto p = getPlayer(snum);
	auto pact = p->GetActor();
	double angleDelta = absangle(p->GetActor()->spr.Angles.Yaw, wal->delta().Angle()).Degrees();
	double damageAmount = p->MotoSpeed * p->MotoSpeed;

	const double scale = (180. / 2048.);
	p->GetActor()->spr.Angles.Yaw += DAngle::fromDeg(p->MotoSpeed * (krand() & 1 ? -scale : scale));

	// That's some very weird angles here...
	if (angleDelta >= 77.51 && angleDelta <= 102.13)
	{
		damageAmount *= (1. / 256.);
		p->MotoSpeed = 0;
		if (S_CheckActorSoundPlaying(pact, 238) == 0)
			S_PlayActorSound(238, pact);
	}
	else if (angleDelta >= 54.66 && angleDelta <= 125)
	{
		damageAmount *= (1. / 2048.);
		p->MotoSpeed -= (p->MotoSpeed / 2.) + (p->MotoSpeed / 4.);
		if (S_CheckActorSoundPlaying(pact, 238) == 0)
			S_PlayActorSound(238, pact);
	}
	else if (angleDelta >= 19.51 && angleDelta <= 160.14)
	{
		damageAmount *= (1. / 16384.);
		p->MotoSpeed -= p->MotoSpeed / 2.;
		if (S_CheckActorSoundPlaying(pact, 239) == 0)
			S_PlayActorSound(239, pact);
	}
	else
	{
		damageAmount *= (1. / 32768.);
		p->MotoSpeed -= p->MotoSpeed / 8.;
		if (S_CheckActorSoundPlaying(pact, 240) == 0)
			S_PlayActorSound(240, pact);
	}
	pact->spr.extra -= int(damageAmount);
	if (pact->spr.extra <= 0)
	{
		S_PlayActorSound(SQUISHED, pact);
		SetPlayerPal(p, PalEntry(63, 63, 0, 0));
	}
	else if (damageAmount)
		S_PlayActorSound(DUKE_LAND_HURT, pact);

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onBoatMove(int snum, int psectlotag, walltype* wal)
{
	auto p = getPlayer(snum);
	auto pact = p->GetActor();
	double angleDelta = absangle(p->GetActor()->spr.Angles.Yaw, wal->delta().Angle()).Degrees();

	const double scale = (90. / 2048.);
	p->GetActor()->spr.Angles.Yaw += DAngle::fromDeg(p->MotoSpeed * (krand() & 1 ? -scale : scale));

	if (angleDelta >= 77.51 && angleDelta <= 102.13)
	{
		p->MotoSpeed = ((p->MotoSpeed / 2.) + (p->MotoSpeed / 4.)) / 4.;
		if (psectlotag == 1 && S_CheckActorSoundPlaying(pact, 178) == 0)
			S_PlayActorSound(178, pact);
	}
	else if (angleDelta >= 54.66 && angleDelta <= 125)
	{
		p->MotoSpeed -= ((p->MotoSpeed / 2.) + (p->MotoSpeed / 4.)) / 8.;
		if (psectlotag == 1 && S_CheckActorSoundPlaying(pact, 179) == 0)
			S_PlayActorSound(179, pact);
	}
	else if (angleDelta >= 19.51 && angleDelta <= 160.14)
	{
		p->MotoSpeed -= p->MotoSpeed / 16.;
		if (psectlotag == 1 && S_CheckActorSoundPlaying(pact, 180) == 0)
			S_PlayActorSound(180, pact);
	}
	else
	{
		p->MotoSpeed -= p->MotoSpeed / 64.;
		if (psectlotag == 1 && S_CheckActorSoundPlaying(pact, 181) == 0)
			S_PlayActorSound(181, pact);
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onMotorcycleHit(int snum, DDukeActor* victim)
{
	auto p = getPlayer(snum);
	if (badguy(victim) || victim->isPlayer())
	{
		if (!victim->isPlayer())
		{
			if (numplayers == 1)
			{
				Collision coll;
				DAngle ang = p->TiltStatus * 20 + p->GetActor()->spr.Angles.Yaw;
				movesprite_ex(victim, DVector3(ang.ToVector() * 4, victim->vel.Z), CLIPMASK0, coll);
			}
		}
		else
			victim->SetHitOwner(p->GetActor());
		victim->attackertype = RedneckMotoHitClass;
		victim->hitextra = int(p->MotoSpeed * 0.5);
		p->MotoSpeed -= p->MotoSpeed / 4.;
		p->TurbCount = 6;
	}
	else if (p->MotoSpeed > 45)
	{
		CallOnMotoSmash(victim, p);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onBoatHit(int snum, DDukeActor* victim)
{
	auto p = getPlayer(snum);

	if (badguy(victim) || victim->isPlayer())
	{
		if (!victim->isPlayer())
		{
			if (numplayers == 1)
			{
				Collision coll;
				DAngle ang = p->TiltStatus * 20 + p->GetActor()->spr.Angles.Yaw;
				movesprite_ex(victim, DVector3(ang.ToVector() * 2, victim->vel.Z), CLIPMASK0, coll);
			}
		}
		else
			victim->SetHitOwner(p->GetActor());
		victim->attackertype = RedneckMotoHitClass;
		victim->hitextra = int(p->MotoSpeed * 0.25);
		p->MotoSpeed -= p->MotoSpeed / 4.;
		p->TurbCount = 6;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void fireweapon(DDukePlayer* const p)
{
	const auto pact = p->GetActor();
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
		if (!isRRRA() && p->curr_weapon >= MOTORCYCLE_WEAPON) return;
		switch (p->curr_weapon)
		{
		case DYNAMITE_WEAPON:
			p->hbomb_hold_delay = 0;
			if (p->ammo_amount[DYNAMITE_WEAPON] > 0)
				p->kickback_pic = 1;
			break;
		case THROWINGDYNAMITE_WEAPON:
			p->hbomb_hold_delay = 0;
			p->kickback_pic = 1;
			break;

		case PISTOL_WEAPON:
			if (p->ammo_amount[PISTOL_WEAPON] > 0)
			{
				p->ammo_amount[PISTOL_WEAPON]--;
				p->kickback_pic = 1;
			}
			break;

		case RIFLEGUN_WEAPON:
			if (p->ammo_amount[RIFLEGUN_WEAPON] > 0) // && p->random_club_frame == 0)
				p->kickback_pic = 1;
			break;

		case SHOTGUN_WEAPON:
			if (p->ammo_amount[SHOTGUN_WEAPON] > 0 && p->random_club_frame == 0)
				p->kickback_pic = 1;
			break;

		case BOWLING_WEAPON:
			if (p->ammo_amount[BOWLING_WEAPON] > 0)
				p->kickback_pic = 1;
			break;
		case POWDERKEG_WEAPON:
			if (p->ammo_amount[POWDERKEG_WEAPON] > 0)
				p->kickback_pic = 1;
			break;

		case BUZZSAW_WEAPON:
		case THROWSAW_WEAPON:
			if (p->curr_weapon == BUZZSAW_WEAPON)
			{
				if (p->ammo_amount[BUZZSAW_WEAPON] > 0)
				{
					p->kickback_pic = 1;
					S_PlayActorSound(431, pact);
				}
			}
			else if (p->ammo_amount[THROWSAW_WEAPON] > 0)
			{
				p->kickback_pic = 1;
				S_PlayActorSound(SHRINKER_FIRE, pact);
			}
			break;

		case ALIENBLASTER_WEAPON:
			if (p->ammo_amount[ALIENBLASTER_WEAPON] > 0)
				p->kickback_pic = 1;
			break;

		case TIT_WEAPON:
			if (p->ammo_amount[TIT_WEAPON] > 0)
			{
				p->kickback_pic = 1;
				p->hbomb_hold_delay = !p->hbomb_hold_delay;
			}
			break;

		case MOTORCYCLE_WEAPON:
			if (p->ammo_amount[MOTORCYCLE_WEAPON] > 0)
			{
				p->kickback_pic = 1;
				p->hbomb_hold_delay = !p->hbomb_hold_delay;
			}
			break;

		case BOAT_WEAPON:
			if (p->ammo_amount[BOAT_WEAPON] > 0)
				p->kickback_pic = 1;
			break;

		case CROSSBOW_WEAPON:
			if (p->ammo_amount[CROSSBOW_WEAPON] > 0)
				p->kickback_pic = 1;
			break;

		case CHICKEN_WEAPON:
			if (p->ammo_amount[CHICKEN_WEAPON] > 0)
				p->kickback_pic = 1;
			break;

		case KNEE_WEAPON:
		case SLINGBLADE_WEAPON:
			if (p->curr_weapon == SLINGBLADE_WEAPON)
			{
				if (p->ammo_amount[SLINGBLADE_WEAPON] > 0)
					if (p->quick_kick == 0)
						p->kickback_pic = 1;
			}
			else if (!isRRRA() || p->ammo_amount[KNEE_WEAPON] > 0)
				if (p->quick_kick == 0)
					p->kickback_pic = 1;
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void operateweapon(DDukePlayer* const p, ESyncBits actions, sectortype* psectp)
{
	auto pact = p->GetActor();
	int psectlotag = psectp ? psectp->lotag : 857;

	if (!isRRRA() && p->curr_weapon >= MOTORCYCLE_WEAPON) return;
	switch (p->curr_weapon)
	{
	case DYNAMITE_WEAPON:

		if (p->kickback_pic == 1)
			S_PlaySound(401);
		if (p->kickback_pic == 6 && (actions & SB_FIRE))
			p->rapid_fire_hold = 1;
		p->kickback_pic++;
		if (p->kickback_pic > 19)
		{
			p->okickback_pic = p->kickback_pic = 0;
			p->curr_weapon = THROWINGDYNAMITE_WEAPON;
			p->last_weapon = -1;
			p->oweapon_pos = p->weapon_pos = 10;
			p->detonate_time = 45;
			p->detonate_count = 1;
			S_PlaySound(402);
		}

		break;


	case THROWINGDYNAMITE_WEAPON:

		p->kickback_pic++;

		if (p->detonate_time < 0)
		{
			p->hbomb_on = 0;
		}

		if (p->kickback_pic == 39)
		{
			p->hbomb_on = 0;
			p->noise_radius = 512;
			madenoise(p);
		}
		if (p->kickback_pic == 12)
		{
			double vel, zvel;

			p->ammo_amount[DYNAMITE_WEAPON]--;
			if (p->ammo_amount[CROSSBOW_WEAPON])
				p->ammo_amount[CROSSBOW_WEAPON]--;

			if (p->on_ground && (actions & SB_CROUCH))
			{
				vel = 15 / 16.;
				zvel = p->Angles.getPitchWithView().Sin() * 10.;
			}
			else
			{
				vel = 140 / 16.;
				setFreeAimVelocity(vel, zvel, p->Angles.getPitchWithView(), 10.);
				zvel -= 4;
			}

			auto spawned = CreateActor(p->cursector, pact->getPosWithOffsetZ() + pact->spr.Angles.Yaw.ToVector() * 16, RedneckDynamiteClass, -16, DVector2(0.140625, 0.140625),
				pact->spr.Angles.Yaw, (vel + p->hbomb_hold_delay * 2) * 2, zvel, pact, 1);

			if (spawned)
			{
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
		else if (p->kickback_pic < 12 && (actions & SB_FIRE))
			p->hbomb_hold_delay++;

		if (p->kickback_pic == 40)
		{
			p->okickback_pic = p->kickback_pic = 0;
			p->curr_weapon = DYNAMITE_WEAPON;
			p->last_weapon = -1;
			p->detonate_count = 0;
			p->detonate_time = 45;
			if (p->ammo_amount[DYNAMITE_WEAPON] > 0)
			{
				fi.addweapon(p, DYNAMITE_WEAPON, true);
				p->oweapon_pos = p->weapon_pos = -9;
			}
			else checkavailweapon(p);
		}
		break;

	case PISTOL_WEAPON:
		if (p->kickback_pic == 1)
		{
			shoot(pact, DukeShotSparkClass);
			S_PlayActorSound(PISTOL_FIRE, pact);
			p->noise_radius = 512;
			madenoise(p);

			lastvisinc = PlayClock + 32;
			p->visibility = 0;
			if (psectlotag != 857)
			{
				p->vel.XY() -= pact->spr.Angles.Yaw.ToVector();
			}
		}
		else if (p->kickback_pic == 2)
			if (p->ammo_amount[PISTOL_WEAPON] <= 0)
			{
				p->okickback_pic = p->kickback_pic = 0;
				checkavailweapon(p);
			}

		p->kickback_pic++;

		if (p->kickback_pic >= 22)
		{
			if (p->ammo_amount[PISTOL_WEAPON] <= 0)
			{
				p->okickback_pic = p->kickback_pic = 0;
				checkavailweapon(p);
				break;
			}
			else if ((p->ammo_amount[PISTOL_WEAPON] % 6) == 0)
			{
				switch (p->kickback_pic)
				{
				case 24:
					S_PlayActorSound(EJECT_CLIP, pact);
					break;
				case 30:
					S_PlayActorSound(INSERT_CLIP, pact);
					break;
				}
			}
			else
				p->kickback_pic = 38;
		}

		if (p->kickback_pic == 38)
		{
			p->okickback_pic = p->kickback_pic = 0;
			checkavailweapon(p);
		}

		break;

	case SHOTGUN_WEAPON:

		p->kickback_pic++;

		if (p->kickback_pic == 6)
			if (p->shotgun_state[0] == 0)
				if (p->ammo_amount[SHOTGUN_WEAPON] > 1)
					if (actions & SB_FIRE)
						p->shotgun_state[1] = 1;

		if (p->kickback_pic == 4)
		{
			for (int ii = 0; ii < 10; ii++)
				shoot(pact, RedneckShotgunShotClass);

			p->ammo_amount[SHOTGUN_WEAPON]--;

			S_PlayActorSound(SHOTGUN_FIRE, pact);

			p->noise_radius = 512;
			madenoise(p);

			lastvisinc = PlayClock + 32;
			p->visibility = 0;
		}

		if (p->kickback_pic == 7)
		{
			if (p->shotgun_state[1])
			{
				for (int ii = 0; ii < 10; ii++)
					shoot(pact, RedneckShotgunShotClass);

				p->ammo_amount[SHOTGUN_WEAPON]--;

				S_PlayActorSound(SHOTGUN_FIRE, pact);

				if (psectlotag != 857)
				{
					p->vel.XY() -= pact->spr.Angles.Yaw.ToVector() * 2;
				}
			}
			else if (psectlotag != 857)
			{
				p->vel.XY() -= pact->spr.Angles.Yaw.ToVector();
			}
		}

		if (p->shotgun_state[0])
		{
			switch (p->kickback_pic)
			{
			case 16:
				checkavailweapon(p);
				break;
			case 17:
				S_PlayActorSound(SHOTGUN_COCK, pact);
				break;
			case 28:
				p->okickback_pic = p->kickback_pic = 0;
				p->shotgun_state[0] = 0;
				p->shotgun_state[1] = 0;
				return;
			}
		}
		else if (p->shotgun_state[1])
		{
			switch (p->kickback_pic)
			{
			case 26:
				checkavailweapon(p);
				break;
			case 27:
				S_PlayActorSound(SHOTGUN_COCK, pact);
				break;
			case 38:
				p->okickback_pic = p->kickback_pic = 0;
				p->shotgun_state[0] = 0;
				p->shotgun_state[1] = 0;
				return;
			}
		}
		else
		{
			switch (p->kickback_pic)
			{
			case 16:
				checkavailweapon(p);
				p->okickback_pic = p->kickback_pic = 0;
				p->shotgun_state[0] = 1;
				p->shotgun_state[1] = 0;
				return;
			}
		}
		break;

	case RIFLEGUN_WEAPON:

		p->kickback_pic++;
		pact->spr.Angles.Pitch -= DAngle::fromDeg(0.4476);
		p->recoil++;

		if (p->kickback_pic <= 12)
		{
			if ((p->kickback_pic % 3) == 0)
			{
				p->ammo_amount[RIFLEGUN_WEAPON]--;

				if ((p->kickback_pic % 3) == 0)
				{
					auto j = spawn(pact, DukeShellClass);
					if (j)
					{

						j->spr.Angles.Yaw += DAngle180;
						j->vel.X += 2.;
						j->spr.pos.Z += 3;
						ssp(j, CLIPMASK0);
					}
				}

				S_PlayActorSound(CHAINGUN_FIRE, pact);
				shoot(pact, DukeChaingunShotClass);
				p->noise_radius = 512;
				madenoise(p);
				lastvisinc = PlayClock + 32;
				p->visibility = 0;

				if (psectlotag != 857)
				{
					p->vel.XY() -= pact->spr.Angles.Yaw.ToVector();
				}
				checkavailweapon(p);

				if ((actions & SB_FIRE) == 0)
				{
					p->okickback_pic = p->kickback_pic = 0;
					break;
				}
			}
		}
		else if (p->kickback_pic > 10)
		{
			if (actions & SB_FIRE) p->kickback_pic = 1;
			else p->okickback_pic = p->kickback_pic = 0;
		}

		break;

	case BUZZSAW_WEAPON:

		if (p->kickback_pic > 3)
		{
			p->okickback_pic = p->kickback_pic = 0;
			shoot(pact, RedneckBuzzSawClass);
			p->noise_radius = 64;
			madenoise(p);
			checkavailweapon(p);
		}
		else p->kickback_pic++;
		break;

	case THROWSAW_WEAPON:

		if (p->kickback_pic == 1)
		{
			p->ammo_amount[THROWSAW_WEAPON]--;
			shoot(pact, RedneckSawbladeClass);
			checkavailweapon(p);
		}
		p->kickback_pic++;
		if (p->kickback_pic > 20)
			p->okickback_pic = p->kickback_pic = 0;
		break;

	case TIT_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 2 || p->kickback_pic == 4)
		{
			p->visibility = 0;
			lastvisinc = PlayClock + 32;
			S_PlayActorSound(CHAINGUN_FIRE, pact);
			shoot(pact, DukeShotSparkClass);
			p->noise_radius = 1024;
			madenoise(p);
			p->ammo_amount[TIT_WEAPON]--;
			checkavailweapon(p);
		}
		if (p->kickback_pic == 2)
		{
			pact->spr.Angles.Yaw += mapangle(16);
		}
		else if (p->kickback_pic == 4)
		{
			pact->spr.Angles.Yaw -= mapangle(16);
		}
		if (p->kickback_pic > 4)
			p->kickback_pic = 1;
		if (!(actions & SB_FIRE))
			p->okickback_pic = p->kickback_pic = 0;
		break;

	case MOTORCYCLE_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 2 || p->kickback_pic == 4)
		{
			p->visibility = 0;
			lastvisinc = PlayClock + 32;
			S_PlayActorSound(CHAINGUN_FIRE, pact);
			shoot(pact, DukeChaingunShotClass);
			p->noise_radius = 1024;
			madenoise(p);
			p->ammo_amount[MOTORCYCLE_WEAPON]--;
			if (p->ammo_amount[MOTORCYCLE_WEAPON] <= 0)
				p->okickback_pic = p->kickback_pic = 0;
			else
				checkavailweapon(p);
		}
		if (p->kickback_pic == 2)
		{
			pact->spr.Angles.Yaw += mapangle(4);
		}
		else if (p->kickback_pic == 4)
		{
			pact->spr.Angles.Yaw -= mapangle(4);
		}
		if (p->kickback_pic > 4)
			p->kickback_pic = 1;
		if (!(actions & SB_FIRE))
			p->okickback_pic = p->kickback_pic = 0;
		break;
	case BOAT_WEAPON:
		if (p->kickback_pic == 3)
		{
			p->MotoSpeed -= 20;
			p->ammo_amount[BOAT_WEAPON]--;
			shoot(pact, RedneckBoatGrenadeClass);
		}
		p->kickback_pic++;
		if (p->kickback_pic > 20)
		{
			p->okickback_pic = p->kickback_pic = 0;
			checkavailweapon(p);
		}
		if (p->ammo_amount[BOAT_WEAPON] <= 0)
			p->okickback_pic = p->kickback_pic = 0;
		else
			checkavailweapon(p);
		break;

	case ALIENBLASTER_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic >= 7 && p->kickback_pic <= 11)
			shoot(pact, RedneckFirelaserClass);

		if (p->kickback_pic == 5)
		{
			S_PlayActorSound(CAT_FIRE, pact);
			p->noise_radius = 128;
			madenoise(p);
		}
		else if (p->kickback_pic == 9)
		{
			p->ammo_amount[ALIENBLASTER_WEAPON]--;
			p->visibility = 0;
			lastvisinc = PlayClock + 32;
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 12)
		{
			p->vel.XY() -= pact->spr.Angles.Yaw.ToVector();
			pact->spr.Angles.Pitch -= DAngle::fromDeg(8.88);
			p->recoil += 20;
		}
		if (p->kickback_pic > 20)
			p->okickback_pic = p->kickback_pic = 0;
		break;

	case POWDERKEG_WEAPON:
		if (p->kickback_pic == 3)
		{
			double vel, zvel;
			p->ammo_amount[POWDERKEG_WEAPON]--;
			p->gotweapon[POWDERKEG_WEAPON] = false;
			if (p->on_ground && (actions & SB_CROUCH) && !p->OnMotorcycle)
			{
				vel = 15 / 16.;
				setFreeAimVelocity(vel, zvel, p->Angles.getPitchWithView(), 10.);
			}
			else
			{
				vel = 2.;
				setFreeAimVelocity(vel, zvel, p->Angles.getPitchWithView(), 10.);
				zvel -= 4;
			}

			CreateActor(p->cursector, pact->getPosWithOffsetZ() + pact->spr.Angles.Yaw.ToVector() * 16, RedneckPowderKegClass, -16, DVector2(0.140625, 0.140625), p->GetActor()->spr.Angles.Yaw, vel * 2, zvel, pact, 1);
		}
		p->kickback_pic++;
		if (p->kickback_pic > 20)
		{
			p->okickback_pic = p->kickback_pic = 0;
			checkavailweapon(p);
		}
		break;

	case BOWLING_WEAPON:
		if (p->kickback_pic == 30)
		{
			p->ammo_amount[BOWLING_WEAPON]--;
			S_PlayActorSound(354, pact);
			shoot(pact, RedneckBowlingBallClass);
			p->noise_radius = 64;
			madenoise(p);
		}
		if (p->kickback_pic < 30)
		{
			p->vel.XY() += pact->spr.Angles.Yaw.ToVector();
		}
		p->kickback_pic++;
		if (p->kickback_pic > 40)
		{
			p->okickback_pic = p->kickback_pic = 0;
			p->gotweapon[BOWLING_WEAPON] = false;
			checkavailweapon(p);
		}
		break;

	case KNEE_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 3)
			S_PlayActorSound(426, pact);
		if (p->kickback_pic == 12)
		{
			shoot(pact, DukeMeleeAttackClass);
			p->noise_radius = 64;
			madenoise(p);
		}
		else if (p->kickback_pic == 16)
			p->okickback_pic = p->kickback_pic = 0;

		if (p->wantweaponfire >= 0)
			checkavailweapon(p);
		break;


	case SLINGBLADE_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 3)
			S_PlayActorSound(252, pact);
		if (p->kickback_pic == 8)
		{
			shoot(pact, RedneckSlingbladeAttackClass);
			p->noise_radius = 64;
			madenoise(p);
		}
		else if (p->kickback_pic == 16)
			p->okickback_pic = p->kickback_pic = 0;

		if (p->wantweaponfire >= 0)
			checkavailweapon(p);
		break;

	case CROSSBOW_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 4)
		{
			p->ammo_amount[CROSSBOW_WEAPON]--;
			if (p->ammo_amount[DYNAMITE_WEAPON])
				p->ammo_amount[DYNAMITE_WEAPON]--;
			lastvisinc = PlayClock + 32;
			p->visibility = 0;
			shoot(pact, RedneckDynamiteArrowClass);
			p->noise_radius = 2048;
			madenoise(p);
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 16)
			S_PlayActorSound(450, pact);
		else if (p->kickback_pic == 34)
			p->okickback_pic = p->kickback_pic = 0;
		break;

	case CHICKEN_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 4)
		{
			p->ammo_amount[CHICKEN_WEAPON]--;
			lastvisinc = PlayClock + 32;
			p->visibility = 0;
			shoot(pact, RedneckChickenArrowClass);
			p->noise_radius = 2048;
			madenoise(p);
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 16)
			S_PlayActorSound(450, pact);
		else if (p->kickback_pic == 34)
			p->okickback_pic = p->kickback_pic = 0;
		break;

	}

}

//---------------------------------------------------------------------------
//
// this function exists because gotos suck. :P
//
//---------------------------------------------------------------------------

static void processweapon(DDukePlayer* const p, ESyncBits actions, sectortype* psectp)
{
	auto pact = p->GetActor();
	int shrunk = (pact->spr.scale.Y < 0.125);

	if (p->detonate_count > 0)
	{
		if (ud.god)
		{
			p->detonate_time = 45;
			p->detonate_count = 0;
		}
		else if (p->detonate_time <= 0 && p->kickback_pic < 5)
		{
			S_PlaySound(14);
			quickkill(p);
		}
	}


	if (isRRRA() && (p->curr_weapon == KNEE_WEAPON || p->curr_weapon == SLINGBLADE_WEAPON))
		p->random_club_frame += 64;

	if (p->curr_weapon == THROWSAW_WEAPON || p->curr_weapon == BUZZSAW_WEAPON)
		p->random_club_frame += 64; // Glowing

	if (p->curr_weapon == TRIPBOMB_WEAPON || p->curr_weapon == BOWLING_WEAPON)
		p->random_club_frame += 64;

	if (p->rapid_fire_hold == 1)
	{
		if (actions & SB_FIRE) return;
		p->rapid_fire_hold = 0;
	}

	if (shrunk || p->tipincs || p->access_incs)
		actions &= ~SB_FIRE;
	else if (shrunk == 0 && (actions & SB_FIRE) && p->kickback_pic == 0 && p->fist_incs == 0 &&
		p->last_weapon == -1 && (p->weapon_pos == 0 || p->holster_weapon == 1))
	{
		fireweapon(p);
	}
	else if (p->kickback_pic)
	{
		operateweapon(p, actions, psectp);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void processinput_r(int snum)
{
	int k, doubvel;
	Collision chz, clz;
	bool shrunk;
	int psectlotag;
	double floorz = 0, ceilingz = 0;

	auto p = getPlayer(snum);
	auto pact = p->GetActor();

	ESyncBits& actions = p->cmd.ucmd.actions;

	// Get strafe value before it's rotated by the angle.
	const auto strafeVel = p->cmd.ucmd.vel.Y;
	constexpr auto maxVel = (117351124. / 10884538.);

	auto psectp = p->cursector;
	if (p->OnMotorcycle && pact->spr.extra > 0)
	{
		onMotorcycle(snum, actions);
	}
	else if (p->OnBoat && pact->spr.extra > 0)
	{
		onBoat(snum, actions);
	}

	processinputvel(snum);

	if (psectp == nullptr)
	{
		if (pact->spr.extra > 0 && ud.clipping == 0)
		{
			quickkill(p);
			S_PlayActorSound(SQUISHED, pact);
		}
		psectp = &sector[0];
	}
	psectlotag = psectp->lotag;

	if (psectlotag == 867)
	{
		DukeSectIterator it(psectp);
		while (auto act2 = it.Next())
		{
			if (act2->GetClass() == RedneckWaterSurfaceClass)
				if (act2->spr.pos.Z - 8 < p->GetActor()->getOffsetZ())
					psectlotag = ST_2_UNDERWATER;
		}
	}
	else if (psectlotag == 7777 && (currentLevel->gameflags & LEVEL_RR_HULKSPAWN))
			lastlevel = 1;

	if (psectlotag == 848 && tilesurface(psectp->floortexture) == TSURF_SPECIALWATER)
		psectlotag = ST_1_ABOVE_WATER;

	if (psectlotag == 857)
		pact->clipdist = 0.25;
	else
		pact->clipdist = 16;

	p->spritebridge = 0;

	shrunk = (pact->spr.scale.Y < 0.125);
	if (pact->clipdist == 16)
	{
		getzrange(p->GetActor()->getPosWithOffsetZ(), psectp, &ceilingz, chz, &floorz, clz, 10.1875, CLIPMASK0);
	}
	else
	{
		getzrange(p->GetActor()->getPosWithOffsetZ(), psectp, &ceilingz, chz, &floorz, clz, 0.25, CLIPMASK0);
	}

	setPlayerActorViewZOffset(pact);

	p->truefz = getflorzofslopeptr(psectp, p->GetActor()->getPosWithOffsetZ());
	p->truecz = getceilzofslopeptr(psectp, p->GetActor()->getPosWithOffsetZ());

	double truefdist = abs(p->GetActor()->getOffsetZ() - p->truefz);
	if (clz.type == kHitSector && psectlotag == 1 && truefdist > gs.playerheight + 16)
		psectlotag = 0;

	pact->floorz = floorz;
	pact->ceilingz = ceilingz;

	p->doslopetilting();

	if (chz.type == kHitSprite)
	{
		if (chz.actor()->spr.statnum == 1 && chz.actor()->spr.extra >= 0)
		{
			chz.setNone();
			ceilingz = p->truecz;
		}
		else if (chz.actor()->GetClass() == RedneckLadderClass)
		{
			if (!p->stairs)
			{
				p->stairs = 10;
				if ((actions & SB_JUMP) && !p->OnMotorcycle)
				{
					chz.setNone();
					ceilingz = p->truecz;
				}
			}
			else
				p->stairs--;
		}
	}

	if (clz.type == kHitSprite)
	{
		auto doVehicleHit = [&]()
		{
			if (badguy(clz.actor()))
			{
				clz.actor()->attackertype = RedneckMotoHitClass;
				clz.actor()->hitextra = int(2 + (p->MotoSpeed * 0.5));
				p->MotoSpeed -= p->MotoSpeed * (1. / 16.);
			}
		};

		if ((clz.actor()->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_FLOOR| CSTAT_SPRITE_BLOCK)) == (CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_BLOCK))
		{
			psectlotag = 0;
			p->footprintcount = 0;
			p->spritebridge = 1;
		}
		if (p->OnMotorcycle)
			doVehicleHit();
		if (p->OnBoat)
		{
			doVehicleHit();
		}
		else if (badguy(clz.actor()) && clz.actor()->spr.scale.X > 0.375 && abs(pact->spr.pos.Z - clz.actor()->spr.pos.Z) < 84)
		{
			auto ang = (clz.actor()->spr.pos.XY() - p->GetActor()->spr.pos.XY()).Angle();
			p->vel.XY() -= ang.ToVector();
		}
		if (clz.actor()->GetClass() == RedneckLadderClass)
		{
			if (!p->stairs)
			{
				p->stairs = 10;
				if ((actions & SB_CROUCH) && !p->OnMotorcycle)
				{
					ceilingz = clz.actor()->spr.pos.Z;
					chz.setNone();
					floorz = clz.actor()->spr.pos.Z + 4;
				}
			}
			else
				p->stairs--;
		}
		else CallStandingOn(clz.actor(), p);
	}


	if (pact->spr.extra > 0) fi.incur_damage(p);
	else
	{
		pact->spr.extra = 0;
		p->shield_amount = 0;
	}

	p->last_extra = pact->spr.extra;

	if (p->loogcnt > 0)
	{
		p->oloogcnt = p->loogcnt;
		p->loogcnt--;
	}
	else
	{
		p->oloogcnt = p->loogcnt = 0;
	}

	if (p->fist_incs)
	{
		if (endoflevel(p)) return;
	}

	if (p->timebeforeexit > 1 && p->last_extra > 0)
	{
		if (timedexit(p))
			return;
	}

	if (pact->spr.extra <= 0 && !ud.god)
	{
		playerisdead(p, psectlotag, floorz, ceilingz);
		return;
	}

	if (p->GetActor()->spr.scale.X < 0.125 && p->jetpack_on == 0)
	{
		p->ofistsign = p->fistsign;
		p->fistsign += int(p->GetActor()->vel.X * 16);
	}

	if (p->transporter_hold > 0)
	{
		p->transporter_hold--;
		if (p->transporter_hold == 0 && p->on_warping_sector)
			p->transporter_hold = 2;
	}
	if (p->transporter_hold < 0)
		p->transporter_hold++;

	if (p->newOwner != nullptr)
	{
		setForcedSyncInput(snum);
		p->vel.X = p->vel.Y = 0;
		pact->vel.X = 0;

		fi.doincrements(p);

		if (p->curr_weapon == THROWINGDYNAMITE_WEAPON) processweapon(p, actions, psectp);
		return;
	}

	doubvel = TICSPERFRAME;

	checklook(p, actions);
	p->Angles.doViewYaw(&p->cmd.ucmd);
	p->apply_seasick();

	p->updatecentering(snum);

	if (p->on_crane != nullptr)
	{
		setForcedSyncInput(snum);
		goto HORIZONLY;
	}

	p->playerweaponsway(pact->vel.X);

	pact->vel.X = clamp((p->GetActor()->spr.pos.XY() - p->bobpos).Length(), 0., 32.);
	if (p->on_ground) p->bobcounter += int(p->GetActor()->vel.X * 8);

	p->backuppos(ud.clipping == 0 && ((p->insector() && p->cursector->floortexture == mirrortex) || !p->insector()));

	// Shrinking code

	if (psectlotag == ST_17_PLATFORM_UP || (isRRRA() && psectlotag == ST_18_ELEVATOR_DOWN))
	{
		int tmp;
		tmp = getanimationindex(anim_floorz, p->cursector);
		if (tmp >= 0)
		{
			if (!S_CheckActorSoundPlaying(pact, 432))
				S_PlayActorSound(432, pact);
		}
		else
			S_StopSound(432);
	}

	if (isRRRA() && p->sea_sick_stat)
	{
		p->pycount += 32;
		p->pycount &= 2047;
		p->pyoff = BobVal(p->pycount) * (p->SeaSick? 32 : 1);
	}
	if (psectlotag == ST_2_UNDERWATER)
	{
		underwater(p, actions, floorz, ceilingz);
	}
	else
	{
		movement(p, actions, psectp, floorz, ceilingz, shrunk, truefdist, psectlotag);
	}

	p->psectlotag = psectlotag;

	if (movementBlocked(p))
	{
		doubvel = 0;
		p->vel.X = 0;
		p->vel.Y = 0;
		p->cmd.ucmd.ang.Yaw = nullAngle;
		setForcedSyncInput(snum);
	}

	p->Angles.doYawInput(&p->cmd.ucmd);

	purplelavacheck(p);

	if (p->spritebridge == 0 && pact->insector())
	{
		auto sect = pact->sector();
		k = 0;

		if (p->on_ground && truefdist <= gs.playerheight + 16)
		{
			int surface = tilesurface(sect->floortexture);
			int whichsound = surface == TSURF_ELECTRIC ? 0 : surface == TSURF_SLIME ? 1 : surface == TSURF_PLASMA ? 2 : surface == TSURF_MAGMA ? 3 : -1;
			k = makepainsounds(p, whichsound);
		}

		if (k)
		{
			FTA(75, p);
			p->boot_amount -= 2;
			if (p->boot_amount <= 0)
				checkavailinven(p);
		}
	}

	if (p->vel.X || p->vel.Y || !p->cmd.ucmd.vel.XY().isZero())
	{
		p->crack_time = CRACK_TIME;

		k = int(BobVal(p->bobcounter) * 4);

		if (isRRRA() && p->spritebridge == 0 && p->on_ground)
		{
			if (psectlotag == ST_1_ABOVE_WATER)
				p->NotOnWater = 0;
			else if (p->OnBoat)
			{
				if (psectlotag == 1234)
					p->NotOnWater = 0;
				else
					p->NotOnWater = 1;
			}
			else
				p->NotOnWater = 1;
		}

		if (truefdist < gs.playerheight + 8 && (k == 1 || k == 3))
		{
			if (p->spritebridge == 0 && p->walking_snd_toggle == 0 && p->on_ground)
			{
				FTextureID j;
				switch (psectlotag)
				{
				case 0:
					if (clz.type == kHitSprite)
						j = clz.actor()->spr.spritetexture();
					else
						j = psectp->floortexture;

					if (tilesurface(j) == TSURF_METALDUCTS)
					{
						S_PlayActorSound(S_FindSound("PLAYER_WALKINDUCTS"), pact); // Duke's sound slot is not available here.
						p->walking_snd_toggle = 1;
					}
					break;
				case 1:
					if ((krand() & 1) == 0)
						if  (!isRRRA() || (!p->OnBoat && !p->OnMotorcycle && p->cursector->hitag != 321))
							S_PlayActorSound(DUKE_ONWATER, pact);
					p->walking_snd_toggle = 1;
					break;
				}
			}
		}
		else if (p->walking_snd_toggle > 0)
			p->walking_snd_toggle--;

		if (p->jetpack_on == 0 && p->steroids_amount > 0 && p->steroids_amount < 400)
			doubvel <<= 1;

		p->vel.XY() += p->cmd.ucmd.vel.XY() * doubvel * (5. / 16.);
		p->Angles.StrafeVel += strafeVel * doubvel * (5. / 16.);

		if (!isRRRA() && ((p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 10 && p->on_ground) || (p->on_ground && (actions & SB_CROUCH))))
		{
			p->vel.XY() *= gs.playerfriction - 0.125;
			p->Angles.StrafeVel *= gs.playerfriction - 0.125;
		}
		else
		{
			if (psectlotag == 2)
			{
				p->vel.XY() *= gs.playerfriction - FixedToFloat(0x1400);
				p->Angles.StrafeVel *= gs.playerfriction - FixedToFloat(0x1400);
			}
			else
			{
				p->vel.XY() *= gs.playerfriction;
				p->Angles.StrafeVel *= gs.playerfriction;
			}
		}

		if (tilesurface(psectp->floortexture) == TSURF_OIL)
		{
			if (p->OnMotorcycle)
				if (p->on_ground)
					p->moto_on_oil = 1;
		}
		else if (tilesurface(psectp->floortexture) == TSURF_DEEPMUD)
		{
			if (p->OnMotorcycle)
			{
				if (p->on_ground)
					p->moto_on_mud = 1;
			}
			else if (p->boot_amount > 0)
				p->boot_amount--;
			else
			{
				p->vel.XY() *= gs.playerfriction;
				p->Angles.StrafeVel *= gs.playerfriction;
			}
		}
		else if (tilesurface(psectp->floortexture) == TSURF_MUDDY)
		{
			if (p->OnMotorcycle)
			{
				if (p->on_ground)
				{
					p->vel.XY() *= gs.playerfriction - FixedToFloat(0x1800);
					p->Angles.StrafeVel *= gs.playerfriction - FixedToFloat(0x1800);
				}
			}
			else
				if (p->boot_amount > 0)
					p->boot_amount--;
				else
				{
					p->vel.XY() *= gs.playerfriction - FixedToFloat(0x1800);
					p->Angles.StrafeVel *= gs.playerfriction - FixedToFloat(0x1800);
				}
		}

		if (abs(p->vel.X) < 1 / 128. && abs(p->vel.Y) < 1 / 128.)
		{
			p->vel.X = p->vel.Y = 0;
			p->Angles.StrafeVel = 0;
		}

		if (shrunk)
		{
			p->vel.XY() *= gs.playerfriction * 0.75;
			p->Angles.StrafeVel *= gs.playerfriction * 0.75;
		}
	}

	if (!p->OnMotorcycle && !p->OnBoat)
	{
		p->Angles.doRollInput(&p->cmd.ucmd, p->vel.XY(), maxVel, (psectlotag == 1) || (psectlotag == 2));
	}

HORIZONLY:

	double iif = (psectlotag == 1 || p->spritebridge == 1) ? 4 : 20;

	if (p->insector() && p->cursector->lotag == 2) k = 0;
	else k = 1;

	Collision clip{};
	if (ud.clipping)
	{
		p->GetActor()->spr.pos.XY() += p->vel.XY() ;
		updatesector(p->GetActor()->getPosWithOffsetZ(), &p->cursector);
		ChangeActorSect(pact, p->cursector);
	}
	else
		clipmove(p->GetActor()->spr.pos.XY(), p->GetActor()->getOffsetZ(), &p->cursector, p->vel.XY(), 10.25, 4., iif, CLIPMASK0, clip);

	if (p->jetpack_on == 0 && psectlotag != 2 && psectlotag != 1 && shrunk)
		p->GetActor()->spr.pos.Z += 32;

	if (clip.type != kHitNone)
		checkplayerhurt_r(p, clip);
	else if (isRRRA() && p->hurt_delay2 > 0)
		p->hurt_delay2--;


	if (clip.type == kHitWall)
	{
		auto wal = clip.hitWall;
		if (p->OnMotorcycle)
		{
			onMotorcycleMove(snum, wal);
		}
		else if (p->OnBoat)
		{
			onBoatMove(snum, psectlotag, wal);
		}
		else
		{
			if (wal->lotag >= 40 && wal->lotag <= 44)
			{
				if (wal->lotag < 44)
				{
					dofurniture(clip.hitWall, p->cursector, snum);
					pushmove(p->GetActor()->spr.pos.XY(), p->GetActor()->getOffsetZ(), &p->cursector, 10.75, 4, 4, CLIPMASK0);
				}
				else
					pushmove(p->GetActor()->spr.pos.XY(), p->GetActor()->getOffsetZ(), &p->cursector, 10.75, 4, 4, CLIPMASK0);
			}
		}
	}

	if (clip.type == kHitSprite)
	{
		if (p->OnMotorcycle)
		{
			onMotorcycleHit(snum, clip.actor());
		}
		else if (p->OnBoat)
		{
			onBoatHit(snum, clip.actor());
		}
		else if (badguy(clip.actor()))
		{
			if (clip.actor()->spr.statnum != STAT_ACTOR)
			{
				clip.actor()->timetosleep = 0;
				CallPlayFTASound(clip.actor(), 1);
				ChangeActorStat(clip.actor(), STAT_ACTOR);
			}
		}
		CallOnTouch(clip.actor(), p);
	}

	if (p->jetpack_on == 0)
	{
		if (pact->vel.X > 1)
		{
			if (psectlotag != ST_1_ABOVE_WATER && psectlotag != ST_2_UNDERWATER && p->on_ground && (!isRRRA() || !p->sea_sick_stat))
			{
				p->pycount += 52;
				p->pycount &= 2047;
				const double factor = 1024. / 1596; // What is 1596?
				p->pyoff = abs(pact->vel.X * BobVal(p->pycount)) * factor;
			}
		}
		else if (psectlotag != ST_2_UNDERWATER && psectlotag != 1 && (!isRRRA() || !p->sea_sick_stat))
			p->pyoff = 0;
	}

	// RBG***
	SetActor(pact, pact->spr.pos);

	if (psectlotag == ST_800_KILLSTUFF && (!isRRRA() || !p->lotag800kill))
	{
		if (isRRRA()) p->lotag800kill = 1;
		quickkill(p);
		return;
	}

	if (psectlotag < 3)
	{
		psectp = pact->sector();
		if (ud.clipping == 0 && psectp->lotag == ST_31_TWO_WAY_TRAIN)
		{
			auto act = barrier_cast<DDukeActor*>(psectp->hitagactor);
			if (act && act->vel.X != 0 && act->counter == 0)
			{
				quickkill(p);
				return;
			}
		}
	}

	if (truefdist < gs.playerheight && p->on_ground && psectlotag != 1 && shrunk == 0 && p->insector() && p->cursector->lotag == 1)
		if (!S_CheckActorSoundPlaying(pact, DUKE_ONWATER))
			if (!isRRRA() || (!p->OnBoat && !p->OnMotorcycle && p->cursector->hitag != 321))
				S_PlayActorSound(DUKE_ONWATER, pact);

	if (p->cursector != pact->sector())
		ChangeActorSect(pact, p->cursector);

	auto oldpos = p->GetActor()->opos;
	int retry = 0;
	while (ud.clipping == 0)
	{
		int blocked;
		if (pact->clipdist == 16)
			blocked = (pushmove(p->GetActor()->spr.pos.XY(), p->GetActor()->getOffsetZ(), &p->cursector, 8, 4, 4, CLIPMASK0) < 0 && furthestangle(p->GetActor(), 8) < DAngle90);
		else
			blocked = (pushmove(p->GetActor()->spr.pos.XY(), p->GetActor()->getOffsetZ(), &p->cursector, 1, 4, 4, CLIPMASK0) < 0 && furthestangle(p->GetActor(), 8) < DAngle90);

		if (fabs(pact->floorz - pact->ceilingz) < 48 || blocked)
		{
			if (!(pact->sector()->lotag & 0x8000) && (isanunderoperator(pact->sector()->lotag) ||
				isanearoperator(pact->sector()->lotag)))
				fi.activatebysector(pact->sector(), pact);
			if (blocked)
			{
				if (!retry++)
				{
					p->GetActor()->spr.pos = oldpos;
					p->GetActor()->backuppos();
					continue;
				}
				quickkill(p);
				return;
			}
		}
		else if (abs(floorz - ceilingz) < 32 && isanunderoperator(psectp->lotag))
			fi.activatebysector(psectp, pact);
		break;
	}

	if (ud.clipping == 0 && (!p->cursector || (p->cursector && p->cursector->ceilingz > (p->cursector->floorz - 12))))
	{
		quickkill(p);
		return;
	}

	if (actions & SB_CENTERVIEW || (p->hard_landing && (cl_dukepitchmode & kDukePitchLandingRecenter)))
	{
		playerCenterView(p);
	}
	else if ((actions & SB_LOOK_UP) == SB_LOOK_UP)
	{
		playerLookUp(p, actions);
	}
	else if ((actions & SB_LOOK_DOWN) == SB_LOOK_DOWN)
	{
		playerLookDown(p, actions);
	}
	else if ((actions & SB_LOOK_UP) == SB_AIM_UP && !p->OnMotorcycle)
	{
		playerAimUp(p, actions);
	}
	else if ((actions & SB_LOOK_DOWN) == SB_AIM_DOWN && !p->OnMotorcycle)
	{
		playerAimDown(p, actions);
	}
	if (p->recoil && p->kickback_pic == 0)
	{
		int d = p->recoil >> 1;
		if (!d)
			d = 1;
		p->recoil -= d;
		p->GetActor()->spr.Angles.Pitch += maphoriz(d);
	}

	p->Angles.doPitchInput(&p->cmd.ucmd);

	p->checkhardlanding();

	//Shooting code/changes

	if (p->show_empty_weapon > 0)
	{
		p->show_empty_weapon--;

		if (p->show_empty_weapon == 0 && (WeaponSwitch(p->pnum) & 2))
		{
			fi.addweapon(p, p->last_full_weapon, true);
			return;
		}
	}
	dokneeattack(p);


	if (fi.doincrements(p)) return;

	if (p->weapon_pos != 0)
	{
		if (p->weapon_pos == -9)
		{
			if (p->last_weapon >= 0)
			{
				p->oweapon_pos = p->weapon_pos = 10;
				// if(p->curr_weapon == KNEE_WEAPON) p->kickback_pic = 1;
				p->last_weapon = -1;
			}
			else if (p->holster_weapon == 0)
				p->oweapon_pos = p->weapon_pos = 10;
		}
		else p->weapon_pos--;
	}

	processweapon(p, actions, psectp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void OnMotorcycle(DDukePlayer *p)
{
	if (!p->OnMotorcycle && p->cursector->lotag != ST_2_UNDERWATER)
	{
		p->over_shoulder_on = 0;
		p->OnMotorcycle = 1;
		p->last_full_weapon = p->curr_weapon;
		p->curr_weapon = MOTORCYCLE_WEAPON;
		p->gotweapon[MOTORCYCLE_WEAPON] = true;
		p->vel.X = 0;
		p->vel.Y = 0;
		p->GetActor()->spr.Angles.Pitch = nullAngle;
	}
	if (!S_CheckActorSoundPlaying(p->GetActor(),186))
		S_PlayActorSound(186, p->GetActor());
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void OffMotorcycle(DDukePlayer *p)
{
	auto pact = p->GetActor();
	if (p->OnMotorcycle)
	{
		if (S_CheckActorSoundPlaying(pact,188))
			S_StopSound(188,pact);
		if (S_CheckActorSoundPlaying(pact,187))
			S_StopSound(187,pact);
		if (S_CheckActorSoundPlaying(pact,186))
			S_StopSound(186,pact);
		if (S_CheckActorSoundPlaying(pact,214))
			S_StopSound(214,pact);
		if (!S_CheckActorSoundPlaying(pact,42))
			S_PlayActorSound(42, pact);
		p->OnMotorcycle = 0;
		p->gotweapon[MOTORCYCLE_WEAPON] = false;
		p->curr_weapon = p->last_full_weapon;
		checkavailweapon(p);
		p->GetActor()->spr.Angles.Pitch = nullAngle;
		p->moto_do_bump = 0;
		p->MotoSpeed = 0;
		p->TiltStatus = nullAngle;
		p->moto_drink = 0;
		p->VBumpTarget = 0;
		p->VBumpNow = 0;
		p->TurbCount = 0;
		p->vel.XY() = p->GetActor()->spr.Angles.Yaw.ToVector() / 2048.;
		p->moto_underwater = 0;
		auto spawned = spawn(p->GetActor(), RedneckEmptyBikeClass);
		if (spawned)
		{
			spawned->spr.Angles.Yaw = p->GetActor()->spr.Angles.Yaw;
			spawned->saved_ammo = p->ammo_amount[MOTORCYCLE_WEAPON];
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void OnBoat(DDukePlayer *p)
{
	if (!p->OnBoat)
	{
		p->over_shoulder_on = 0;
		p->OnBoat = 1;
		p->last_full_weapon = p->curr_weapon;
		p->curr_weapon = BOAT_WEAPON;
		p->gotweapon[BOAT_WEAPON] = true;
		p->vel.X = 0;
		p->vel.Y = 0;
		p->GetActor()->spr.Angles.Pitch = nullAngle;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void OffBoat(DDukePlayer *p)
{
	if (p->OnBoat)
	{
		p->OnBoat = 0;
		p->gotweapon[BOAT_WEAPON] = false;
		p->curr_weapon = p->last_full_weapon;
		checkavailweapon(p);
		p->GetActor()->spr.Angles.Pitch = nullAngle;
		p->moto_do_bump = 0;
		p->MotoSpeed = 0;
		p->TiltStatus = nullAngle;
		p->moto_drink = 0;
		p->VBumpTarget = 0;
		p->VBumpNow = 0;
		p->TurbCount = 0;
		p->vel.XY() = p->GetActor()->spr.Angles.Yaw.ToVector() / 2048.;
		p->moto_underwater = 0;
		auto spawned = spawn(p->GetActor(), RedneckEmptyBoatClass);
		if (spawned)
		{
			spawned->spr.Angles.Yaw = p->GetActor()->spr.Angles.Yaw;
			spawned->saved_ammo = p->ammo_amount[BOAT_WEAPON];
		}
	}
}


END_DUKE_NS
