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

void fireweapon_ww(DDukePlayer* const p);
void operateweapon_ww(DDukePlayer* const p, ESyncBits actions);

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void incur_damage(DDukePlayer* p)
{
	const auto pact = p->GetActor();
	int damage = 0L, shield_damage = 0L;

	pact->spr.extra -= p->extra_extra8 >> 8;

	damage = pact->spr.extra - p->last_extra;

	if (damage < 0)
	{
		p->extra_extra8 = 0;

		if (p->shield_amount > 0)
		{
			shield_damage = damage * (20 + (rand() % 30)) / 100;
			damage -= shield_damage;

			p->shield_amount += shield_damage;

			if (p->shield_amount < 0)
			{
				damage += p->shield_amount;
				p->shield_amount = 0;
			}
		}

		pact->spr.extra = p->last_extra + damage;
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void selectweapon_d(DDukePlayer* const p, int weap) // playernum, weaponnum
{
	const auto pact = p->GetActor();
	int i, j, k;
	if (p->last_pissed_time <= (26 * 218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && pact->spr.scale.X > 0.5  && p->access_incs == 0 && p->knee_incs == 0)
	{
		if ((p->weapon_pos == 0 || (p->holster_weapon && p->weapon_pos == -9)))
		{
			if (weap == WeaponSel_Alt)
			{
				j = p->curr_weapon;
				switch (p->curr_weapon)
				{
					case SHRINKER_WEAPON:
						if (p->ammo_amount[GROW_WEAPON] > 0 && p->gotweapon[GROW_WEAPON] && isPlutoPak())
						{
							j = GROW_WEAPON;
							p->subweapon |= (1 << GROW_WEAPON);
						}
						break;
					case GROW_WEAPON:
						if (p->ammo_amount[SHRINKER_WEAPON] > 0 && p->gotweapon[SHRINKER_WEAPON])
						{
							j = SHRINKER_WEAPON;
							p->subweapon &= ~(1 << GROW_WEAPON);
						}
						break;
					case FREEZE_WEAPON:
						if (p->ammo_amount[FLAMETHROWER_WEAPON] > 0 && p->gotweapon[FLAMETHROWER_WEAPON] && isWorldTour())
						{
							j = FLAMETHROWER_WEAPON;
							p->subweapon |= (1 << FLAMETHROWER_WEAPON);
						}
						break;
					case FLAMETHROWER_WEAPON:
						if (p->ammo_amount[FREEZE_WEAPON] > 0 && p->gotweapon[FREEZE_WEAPON])
						{
							j = FREEZE_WEAPON;
							p->subweapon &= ~(1 << FLAMETHROWER_WEAPON);
						}
						break;
					default:
						break;
				}
			}
			else if (weap == WeaponSel_Next || weap == WeaponSel_Prev)
			{
				k = p->curr_weapon;
				j = (weap == WeaponSel_Prev ? -1 : 1);	// JBF: prev (-1) or next (1) weapon choice
				i = 0;

				while ((k >= 0 && k < 10) || (isPlutoPak() && k == GROW_WEAPON && (p->subweapon & (1 << GROW_WEAPON)) != 0)
					|| (isWorldTour() && k == FLAMETHROWER_WEAPON && (p->subweapon & (1 << FLAMETHROWER_WEAPON)) != 0))
				{
					if (k == FLAMETHROWER_WEAPON) //Twentieth Anniversary World Tour
					{
						if (j == -1) k = TRIPBOMB_WEAPON;
						else k = PISTOL_WEAPON;
					}
					else if (k == GROW_WEAPON)	// JBF: this is handling next/previous with the grower selected
					{
						if (j == -1)
							k = 5;
						else k = 7;

					}
					else
					{
						k += j;
						// JBF 20040116: so we don't select grower with v1.3d
						if (isPlutoPak() && k == SHRINKER_WEAPON && (p->subweapon & (1 << GROW_WEAPON)))	// JBF: activates grower
							k = GROW_WEAPON;							// if enabled
						if (isWorldTour() && k == FREEZE_WEAPON && (p->subweapon & (1 << FLAMETHROWER_WEAPON)) != 0)
							k = FLAMETHROWER_WEAPON;
					}

					if (k == -1) k = 9;
					else if (k == 10) k = 0;

					if (p->gotweapon[k] && p->ammo_amount[k] > 0)
					{
						if (isPlutoPak())	// JBF 20040116: so we don't select grower with v1.3d
						{
							if (k == SHRINKER_WEAPON && (p->subweapon & (1 << GROW_WEAPON)))
								k = GROW_WEAPON;
							if (isWorldTour() && k == FREEZE_WEAPON && (p->subweapon & (1 << FLAMETHROWER_WEAPON)) != 0)
								k = FLAMETHROWER_WEAPON;
						}
						j = k;
						break;
					}
					else if (isPlutoPak() && k == GROW_WEAPON && p->ammo_amount[GROW_WEAPON] == 0 && p->gotweapon[SHRINKER_WEAPON] && p->ammo_amount[SHRINKER_WEAPON] > 0)	// JBF 20040116: added isPlutoPak() so we don't select grower with v1.3d
					{
						j = SHRINKER_WEAPON;
						p->subweapon &= ~(1 << GROW_WEAPON);
						break;
					}
					else if (isPlutoPak() && k == SHRINKER_WEAPON && p->ammo_amount[SHRINKER_WEAPON] == 0 && p->gotweapon[SHRINKER_WEAPON] && p->ammo_amount[GROW_WEAPON] > 0)	// JBF 20040116: added isPlutoPak() so we don't select grower with v1.3d
					{
						j = GROW_WEAPON;
						p->subweapon |= (1 << GROW_WEAPON);
						break;
					}
					//Twentieth Anniversary World Tour
					else if (isWorldTour() && k == FLAMETHROWER_WEAPON && p->ammo_amount[FLAMETHROWER_WEAPON] == 0 && p->gotweapon[FREEZE_WEAPON] && p->ammo_amount[FREEZE_WEAPON] > 0)
					{
						j = FREEZE_WEAPON;
						p->subweapon &= ~(1 << FLAMETHROWER_WEAPON);
						break;
					}
					else if (isWorldTour() && k == FREEZE_WEAPON && p->ammo_amount[FREEZE_WEAPON] == 0 && p->gotweapon[FLAMETHROWER_WEAPON] && p->ammo_amount[FLAMETHROWER_WEAPON] > 0)
					{
						j = FLAMETHROWER_WEAPON;
						p->subweapon |= (1 << FLAMETHROWER_WEAPON);
						break;
					}

					i++;	// absolutely no weapons, so use foot
					if (i == 10)
					{
						fi.addweapon(p, KNEE_WEAPON, true);
						break;
					}
				}
			}
			else j = weap - 1;

			k = -1;

			if (j == HANDBOMB_WEAPON && p->ammo_amount[HANDBOMB_WEAPON] == 0)
			{
				DukeStatIterator it(STAT_ACTOR);
				while (auto act = it.Next())
				{
					if (act->GetClass() == DukePipeBombClass && act->GetOwner() == pact)
					{
						p->gotweapon[HANDBOMB_WEAPON] = true;
						j = HANDREMOTE_WEAPON;
						break;
					}
				}
			}

			//Twentieth Anniversary World Tour
			if (j == FREEZE_WEAPON && isWorldTour())
			{
				if (p->curr_weapon != FLAMETHROWER_WEAPON && p->curr_weapon != FREEZE_WEAPON)
				{
					if (p->ammo_amount[FLAMETHROWER_WEAPON] > 0)
					{
						if ((p->subweapon & (1 << FLAMETHROWER_WEAPON)) == (1 << FLAMETHROWER_WEAPON))
							j = FLAMETHROWER_WEAPON;
						else if (p->ammo_amount[FREEZE_WEAPON] == 0)
						{
							j = FLAMETHROWER_WEAPON;
							p->subweapon |= (1 << FLAMETHROWER_WEAPON);
						}
					}
					else if (p->ammo_amount[FREEZE_WEAPON] > 0)
						p->subweapon &= ~(1 << FLAMETHROWER_WEAPON);
				}
				else if (p->curr_weapon == FREEZE_WEAPON)
				{
					p->subweapon |= (1 << FLAMETHROWER_WEAPON);
					j = FLAMETHROWER_WEAPON;
				}
				else
					p->subweapon &= ~(1 << FLAMETHROWER_WEAPON);
			}

			if (j == SHRINKER_WEAPON && isPlutoPak())	// JBF 20040116: so we don't select the grower with v1.3d
			{
				if (p->curr_weapon != GROW_WEAPON && p->curr_weapon != SHRINKER_WEAPON)
				{
					if (p->ammo_amount[GROW_WEAPON] > 0)
					{
						if ((p->subweapon & (1 << GROW_WEAPON)) == (1 << GROW_WEAPON))
							j = GROW_WEAPON;
						else if (p->ammo_amount[SHRINKER_WEAPON] == 0)
						{
							j = GROW_WEAPON;
							p->subweapon |= (1 << GROW_WEAPON);
						}
					}
					else if (p->ammo_amount[SHRINKER_WEAPON] > 0)
						p->subweapon &= ~(1 << GROW_WEAPON);
				}
				else if (p->curr_weapon == SHRINKER_WEAPON)
				{
					p->subweapon |= (1 << GROW_WEAPON);
					j = GROW_WEAPON;
				}
				else
					p->subweapon &= ~(1 << GROW_WEAPON);
			}

			if (p->holster_weapon)
			{
				p->cmd.ucmd.actions |= SB_HOLSTER;
				p->oweapon_pos = p->weapon_pos = -9;
			}
			else if (j >= MIN_WEAPON && p->gotweapon[j] && p->curr_weapon != j) switch (j)
			{
			case KNEE_WEAPON:
				fi.addweapon(p, KNEE_WEAPON, true);
				break;
			case PISTOL_WEAPON:
			case SHOTGUN_WEAPON:
			case CHAINGUN_WEAPON:
			case RPG_WEAPON:
			case DEVISTATOR_WEAPON:
			case FREEZE_WEAPON:
			case FLAMETHROWER_WEAPON:
			case GROW_WEAPON:
			case SHRINKER_WEAPON:

				if (p->ammo_amount[j] == 0 && p->show_empty_weapon == 0)
				{
					p->show_empty_weapon = 32;
					p->last_full_weapon = p->curr_weapon;
				}

				fi.addweapon(p, j, true);
				break;
			case HANDREMOTE_WEAPON:
				if (k >= 0) // Found in list of [1]'s
				{
					p->curr_weapon = HANDREMOTE_WEAPON;
					p->last_weapon = -1;
					p->oweapon_pos = p->weapon_pos = 10;
				}
				break;
			case HANDBOMB_WEAPON:
				if (p->ammo_amount[HANDBOMB_WEAPON] > 0 && p->gotweapon[HANDBOMB_WEAPON])
					fi.addweapon(p, HANDBOMB_WEAPON, true);
				break;
			case TRIPBOMB_WEAPON:
				if (p->ammo_amount[TRIPBOMB_WEAPON] > 0 && p->gotweapon[TRIPBOMB_WEAPON])
					fi.addweapon(p, TRIPBOMB_WEAPON, true);
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

int doincrements_d(DDukePlayer* p)
{
	auto pact = p->GetActor();

	p->player_par++;

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

		if (p->last_pissed_time == (26 * 219))
		{
			S_PlayActorSound(FLUSH_TOILET, pact);
			if (p->pnum == screenpeek || ud.coop == 1)
				S_PlayActorSound(DUKE_PISSRELIEF, pact);
		}

		if (p->last_pissed_time == (26 * 218))
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
			checkavailinven(p);
		if (!(p->steroids_amount & 7))
			if (p->pnum == screenpeek || ud.coop == 1)
				S_PlayActorSound(DUKE_HARTBEAT, pact);
	}

	if (p->heat_on && p->heat_amount > 0)
	{
		p->heat_amount--;
		if (p->heat_amount == 0)
		{
			p->heat_on = 0;
			checkavailinven(p);
			S_PlayActorSound(NITEVISION_ONOFF, pact);
		}
	}

	if (p->holoduke_on != nullptr)
	{
		p->holoduke_amount--;
		if (p->holoduke_amount <= 0)
		{
			S_PlayActorSound(TELEPORTER, pact);
			p->holoduke_on = nullptr;
			checkavailinven(p);
		}
	}

	if (p->jetpack_on && p->jetpack_amount > 0)
	{
		p->jetpack_amount--;
		if (p->jetpack_amount <= 0)
		{
			p->jetpack_on = 0;
			checkavailinven(p);
			S_PlayActorSound(DUKE_JETPACK_OFF, pact);
			S_StopSound(DUKE_JETPACK_IDLE, pact);
			S_StopSound(DUKE_JETPACK_ON, pact);
		}
	}

	if (p->quick_kick > 0 && pact->spr.pal != 1)
	{
		p->last_quick_kick = p->quick_kick + 1;
		p->quick_kick--;
		if (p->quick_kick == 8)
			shoot(pact, DukeMeleeAttackClass);
	}
	else if (p->last_quick_kick > 0)
		p->last_quick_kick--;

	if (p->access_incs && pact->spr.pal != 1)
	{
		p->oaccess_incs = p->access_incs;
		p->access_incs++;
		if (pact->spr.extra <= 0)
			p->access_incs = 12;
		if (p->access_incs == 12)
		{
			if (p->access_spritenum != nullptr)
			{
				checkhitswitch(p, nullptr, p->access_spritenum);
				switch (p->access_spritenum->spr.pal)
				{
				case 0:p->got_access &= (0xffff - 0x1); break;
				case 21:p->got_access &= (0xffff - 0x2); break;
				case 23:p->got_access &= (0xffff - 0x4); break;
				}
				p->access_spritenum = nullptr;
			}
			else
			{
				checkhitswitch(p, p->access_wall, nullptr);
				switch (p->access_wall->pal)
				{
				case 0:p->got_access &= (0xffff - 0x1); break;
				case 21:p->got_access &= (0xffff - 0x2); break;
				case 23:p->got_access &= (0xffff - 0x4); break;
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
		if (p->knuckle_incs == 10 && !isWW2GI())
		{
			if (PlayClock > 1024)
				if (p->pnum == screenpeek || ud.coop == 1)
				{
					if (rand() & 1)
						S_PlayActorSound(DUKE_CRACK, pact);
					else S_PlayActorSound(DUKE_CRACK2, pact);
				}
			S_PlayActorSound(DUKE_CRACK_FIRST, pact);
		}
		else if (p->knuckle_incs == 22 || !!(p->cmd.ucmd.actions & SB_FIRE))
			p->knuckle_incs = 0;

		return 1;
	}
	return 0;
}


//---------------------------------------------------------------------------NAM
//
//
//
//---------------------------------------------------------------------------

void checkweapons_d(DDukePlayer* p)
{
	static PClassActor* const * const weapon_sprites[MAX_WEAPONS] = { &DukeMeleeAttackClass, &DukeFirstgunSpriteClass, &DukeShotgunSpriteClass,
			&DukeChaingunSpriteClass, &DukeRPGSpriteClass, &DukePipeBombClass, &DukeShrinkerSpriteClass, &DukeDevastatorSpriteClass,
			&DukeTripBombSpriteClass, &DukeFreezeSpriteClass, &DukePipeBombClass, &DukeShrinkerSpriteClass };

	int cw;

	if (isWW2GI())
	{
		cw = aplWeaponWorksLike(p->curr_weapon, p);
	}
	else 
		cw = p->curr_weapon;


	if (cw < 1 || cw >= MAX_WEAPONS) return;

	if (cw)
	{
		const auto pact = p->GetActor();

		if (krand() & 1)
			spawn(pact, *weapon_sprites[cw]);
		else switch (cw)
		{
		case RPG_WEAPON:
		case HANDBOMB_WEAPON:
			spawn(pact, DukeExplosion2Class);
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void operateJetpack(DDukePlayer* const p, ESyncBits actions, int psectlotag, double floorz, double ceilingz, int shrunk)
{
	const auto pact = p->GetActor();
	const auto kbdDir = !!(actions & SB_JUMP) - !!(actions & SB_CROUCH);
	const double dist = shrunk ? 2 : 8;
	const double velZ = clamp(dist * kbdDir + dist * p->cmd.ucmd.vel.Z, -dist, dist);

	p->on_ground = 0;
	p->jumping_counter = 0;
	p->hard_landing = 0;
	p->falling_counter = 0;
	p->pycount += 32;
	p->pycount &= 2047;
	p->pyoff = BobVal(p->pycount);

	if (p->jetpack_on < 11)
	{
		p->jetpack_on++;
		pact->spr.pos.Z -= p->jetpack_on * 0.5; //Goin up
	}
	else if (p->jetpack_on == 11 && !S_CheckActorSoundPlaying(pact, DUKE_JETPACK_IDLE))
	{
		S_PlayActorSound(DUKE_JETPACK_IDLE, pact);
	}

	if (velZ > 0) //A (soar high)
	{
		// jump
		SetGameVarID(g_iReturnVarID, 0, pact, p->pnum);
		OnEvent(EVENT_SOARUP, p->pnum, pact, -1);
		if (GetGameVarID(g_iReturnVarID, pact, p->pnum).value() == 0)
		{
			pact->spr.pos.Z -= velZ;
			p->crack_time = CRACK_TIME;
		}
	}

	if (velZ < 0) //Z (soar low)
	{
		// crouch
		SetGameVarID(g_iReturnVarID, 0, pact, p->pnum);
		OnEvent(EVENT_SOARDOWN, p->pnum, pact, -1);
		if (GetGameVarID(g_iReturnVarID, pact, p->pnum).value() == 0)
		{
			pact->spr.pos.Z -= velZ;
			p->crack_time = CRACK_TIME;
		}
	}

	const int k = shrunk == 0 && (psectlotag == ST_0_NO_EFFECT || psectlotag == ST_2_UNDERWATER) ? 32 : 16;

	if (psectlotag != 2 && p->scuba_on == 1)
		p->scuba_on = 0;

	if (pact->getOffsetZ() > floorz - k)
		pact->spr.pos.Z += ((floorz - k) - pact->getOffsetZ()) * 0.5;
	if (pact->getOffsetZ() < pact->ceilingz + 18)
		pact->spr.pos.Z = pact->ceilingz + 18 + gs.playerheight;

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void movement(DDukePlayer* const p, ESyncBits actions, sectortype* psect, double floorz, double ceilingz, int shrunk, double truefdist, int psectlotag)
{
	int j;
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
					p->footprintpal = 8;
				else p->footprintpal = 0;
				p->footprintshade = 0;
			}
		}
	}
	else
	{
		footprints(p);
	}

	if (pact->getOffsetZ() < floorz - i) //falling
	{

		// not jumping or crouching
		if ((actions & (SB_JUMP|SB_CROUCH)) == 0 && p->on_ground && (psect->floorstat & CSTAT_SECTOR_SLOPE) && pact->getOffsetZ() >= (floorz - i - 16))
			pact->spr.pos.Z = floorz - i + gs.playerheight;
		else
		{
			p->on_ground = 0;
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
				if (!p->insector() || p->cursector->lotag != ST_1_ABOVE_WATER)
				{
					if (p->falling_counter > 62 && !ud.god) quickkill(p);

					else if (p->falling_counter > 9)
					{
						j = p->falling_counter;
						pact->spr.extra -= j - (krand() & 3);
						if (pact->spr.extra <= 0)
						{
							S_PlayActorSound(SQUISHED, pact);
							SetPlayerPal(p, PalEntry(63, 63, 0, 0));
						}
						else
						{
							S_PlayActorSound(DUKE_LAND, pact);
							S_PlayActorSound(DUKE_LAND_HURT, pact);
						}

						SetPlayerPal(p, PalEntry(32, 16, 0, 0));
					}
					else if (p->vel.Z > 8) S_PlayActorSound(DUKE_LAND, pact);
				}
			}
		}
	}

	else
	{
		p->falling_counter = 0;
		S_StopSound(DUKE_SCREAM, pact);

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

		if ((actions & SB_CROUCH) || p->cmd.ucmd.vel.Z < 0)
		{
			playerCrouch(p);
		}

		// jumping
		if ((actions & SB_JUMP) == 0 && p->jumping_toggle == 1)
			p->jumping_toggle = 0;

		else if ((actions & SB_JUMP))
		{
			playerJump(p, floorz, ceilingz);
		}

		if (p->jumping_counter && (actions & SB_JUMP) == 0)
			p->jumping_toggle = 0;
	}

	if (p->jumping_counter)
	{
		if ((actions & SB_JUMP) == 0 && p->jumping_toggle == 1)
			p->jumping_toggle = 0;

		if (p->jumping_counter < (1024 + 256))
		{
			if (psectlotag == 1 && p->jumping_counter > 768)
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

int operateTripbomb(DDukePlayer* const p)
{
	const auto pact = p->GetActor();
	HitInfo hit{};
	double vel = 1024, zvel = 0;
	setFreeAimVelocity(vel, zvel, p->getPitchWithView(), 16.);

	hitscan(pact->getPosWithOffsetZ(), p->cursector, DVector3(pact->spr.Angles.Yaw.ToVector() * vel, zvel), hit, CLIPMASK1);

	if (hit.hitSector == nullptr || hit.actor())
		return 0;

	if (hit.hitWall != nullptr && hit.hitSector->lotag > 2)
		return 0;

	if (hit.hitWall != nullptr)
		if (tileflags(hit.hitWall->overtexture) & TFLAG_FORCEFIELD)
			return 0;

	DDukeActor* act;
	DukeSectIterator it(hit.hitSector);
	while ((act = it.Next()))
	{
		if (!(act->flags1 & SFLAG_BLOCK_TRIPBOMB))
		{
			auto delta = act->spr.pos - hit.hitpos;
			if (abs(delta.Z) < 12 && delta.XY().LengthSquared() < (18.125 * 18.125))
				return 0;
		}
	}

	if (act == nullptr && hit.hitWall != nullptr && (hit.hitWall->cstat & CSTAT_WALL_MASKED) == 0)
		if ((hit.hitWall->twoSided() && hit.hitWall->nextSector()->lotag <= 2) || (!hit.hitWall->twoSided() && hit.hitSector->lotag <= 2))
		{
			auto delta = hit.hitpos.XY() - pact->spr.pos.XY();
			if (delta.LengthSquared() < (18.125 * 18.125))
			{
				pact->restorez();
				p->vel.Z = 0;
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

static void fireweapon(DDukePlayer* const p)
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
	else switch (p->curr_weapon)
	{
	case HANDBOMB_WEAPON:
		p->hbomb_hold_delay = 0;
		if (p->ammo_amount[HANDBOMB_WEAPON] > 0)
			p->kickback_pic = 1;
		break;
	case HANDREMOTE_WEAPON:
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


	case CHAINGUN_WEAPON:
		if (p->ammo_amount[CHAINGUN_WEAPON] > 0) // && p->random_club_frame == 0)
			p->kickback_pic = 1;
		break;

	case SHOTGUN_WEAPON:
		if (p->ammo_amount[SHOTGUN_WEAPON] > 0 && p->random_club_frame == 0)
			p->kickback_pic = 1;
		break;
	case TRIPBOMB_WEAPON:
		if (p->ammo_amount[TRIPBOMB_WEAPON] > 0)
		{
			if (operateTripbomb(p))
				p->kickback_pic = 1;
		}
		break;

	case SHRINKER_WEAPON:
	case GROW_WEAPON:
		if (p->curr_weapon == GROW_WEAPON)
		{
			if (p->ammo_amount[GROW_WEAPON] > 0)
			{
				p->kickback_pic = 1;
				S_PlayActorSound(EXPANDERSHOOT, pact);
			}
		}
		else if (p->ammo_amount[SHRINKER_WEAPON] > 0)
		{
			p->kickback_pic = 1;
			S_PlayActorSound(SHRINKER_FIRE, pact);
		}
		break;

	case FREEZE_WEAPON:
		if (p->ammo_amount[FREEZE_WEAPON] > 0)
		{
			p->kickback_pic = 1;
			S_PlayActorSound(CAT_FIRE, pact);
		}
		break;
	case DEVISTATOR_WEAPON:
		if (p->ammo_amount[DEVISTATOR_WEAPON] > 0)
		{
			p->kickback_pic = 1;
			p->hbomb_hold_delay = !p->hbomb_hold_delay;
			S_PlayActorSound(CAT_FIRE, pact);
		}
		break;

	case RPG_WEAPON:
		if (p->ammo_amount[RPG_WEAPON] > 0)
		{
			p->kickback_pic = 1;
		}
		break;

	case FLAMETHROWER_WEAPON: // Twentieth Anniversary World Tour
		if (isWorldTour() && p->ammo_amount[FLAMETHROWER_WEAPON] > 0) 
		{
			p->kickback_pic = 1;
			if (p->cursector->lotag != 2)
				S_PlayActorSound(FLAMETHROWER_INTRO, pact);
		}
		break;

	case KNEE_WEAPON:
		if (p->quick_kick == 0)
		{
			p->kickback_pic = 1;
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void operateweapon(DDukePlayer* const p, ESyncBits actions)
{
	auto pact = p->GetActor();

	// already firing...

	switch (p->curr_weapon)
	{
	case HANDBOMB_WEAPON:	// grenade in NAM
		if (p->kickback_pic == 6 && (actions & SB_FIRE))
		{
			p->rapid_fire_hold = 1;
			break;
		}
		p->kickback_pic++;
		if (p->kickback_pic == 12)
		{
			double zvel, vel;

			p->ammo_amount[HANDBOMB_WEAPON]--;

			if (p->on_ground && (actions & SB_CROUCH))
			{
				vel = 15/16.;
				zvel = p->getPitchWithView().Sin() * 10.;
			}
			else
			{
				vel = 140/16.;
				setFreeAimVelocity(vel, zvel, p->getPitchWithView(), 10.);
				zvel -= 4;
			}

			auto spawned = CreateActor(p->cursector, pact->getPosWithOffsetZ() + pact->spr.Angles.Yaw.ToVector() * 16, DukePipeBombClass, -16, DVector2(0.140625, 0.140625),
				pact->spr.Angles.Yaw, vel + p->hbomb_hold_delay * 2, zvel, pact, STAT_ACTOR);

			if (isNam())
			{
				spawned->spr.extra = MulScale(krand(), NAM_GRENADE_LIFETIME_VAR, 14);
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
		else if (p->kickback_pic < 12 && (actions & SB_FIRE))
			p->hbomb_hold_delay++;
		else if (p->kickback_pic > 19)
		{
			p->okickback_pic = p->kickback_pic = 0;
			// don't change to remote when in NAM: grenades are timed
			if (isNam()) checkavailweapon(p);
			else
			{
				p->curr_weapon = HANDREMOTE_WEAPON;
				p->last_weapon = -1;
				p->oweapon_pos = p->weapon_pos = 10;
			}
		}

		break;


	case HANDREMOTE_WEAPON:	// knife in NAM

		p->kickback_pic++;

		if (p->kickback_pic == 2)
		{
			p->hbomb_on = 0;
		}

		if (p->kickback_pic == 10)
		{
			p->okickback_pic = p->kickback_pic = 0;
			int weapon = isNam() ? TRIPBOMB_WEAPON : HANDBOMB_WEAPON;

			if (p->ammo_amount[weapon] > 0)
				fi.addweapon(p, weapon, true);
			else
				checkavailweapon(p);
		}
		break;

	case PISTOL_WEAPON:	// m-16 in NAM
		if (p->kickback_pic == 1)
		{
			shoot(pact, DukeShotSparkClass);
			S_PlayActorSound(PISTOL_FIRE, pact);
			lastvisinc = PlayClock + 32;
			p->visibility = 0;
		}

		else if (p->kickback_pic == 2)
			spawn(pact, DukeShellClass);

		p->kickback_pic++;

		if (p->kickback_pic >= 5)
		{
			if (p->ammo_amount[PISTOL_WEAPON] <= 0 || (p->ammo_amount[PISTOL_WEAPON] % (isNam() ? 20 : 12)))
			{
				p->okickback_pic = p->kickback_pic = 0;
				checkavailweapon(p);
			}
			else
			{
				switch (p->kickback_pic)
				{
				case 5:
					S_PlayActorSound(EJECT_CLIP, pact);
					break;
					//#ifdef NAM								
					//     case WEAPON2_RELOAD_TIME - 15:
					//#else
				case 8:
					//#endif
					S_PlayActorSound(INSERT_CLIP, pact);
					break;
				}
			}
		}

		// 3 second re-load time
		if (p->kickback_pic == (isNam() ? 50 : 27))
		{
			p->okickback_pic = p->kickback_pic = 0;
			checkavailweapon(p);
		}
		break;

	case SHOTGUN_WEAPON:

		p->kickback_pic++;

		if (p->kickback_pic == 4)
		{
			for(int ii = 0; ii < 7; ii++)
				shoot(pact, DukeShotgunShotClass);
			p->ammo_amount[SHOTGUN_WEAPON]--;

			S_PlayActorSound(SHOTGUN_FIRE, pact);

			lastvisinc = PlayClock + 32;
			p->visibility = 0;

		}

		switch(p->kickback_pic)
		{
		case 13:
			checkavailweapon(p);
			break;
		case 15:
			S_PlayActorSound(SHOTGUN_COCK, pact);
			break;
		case 17:
		case 20:
			p->kickback_pic++;
			break;
		case 24:
		{
			auto j = spawn(pact, DukeShotgunShellClass);
			if (j)
			{
				j->spr.Angles.Yaw += DAngle180;
				ssp(j, CLIPMASK0);
				j->spr.Angles.Yaw -= DAngle180;
			}
			p->kickback_pic++;
			break;
		}
		case 31:
			p->okickback_pic = p->kickback_pic = 0;
			return;
		}
		break;

	case CHAINGUN_WEAPON:	// m-60 in NAM 

		p->kickback_pic++;

		if (p->kickback_pic <= 12)
		{
			if (((p->kickback_pic) % 3) == 0)
			{
				if (p->ammo_amount[CHAINGUN_WEAPON] <= 0)
				{
					p->ammo_amount[CHAINGUN_WEAPON] = 0;
					p->okickback_pic = p->kickback_pic = 0;
					break;
				}
				p->ammo_amount[CHAINGUN_WEAPON]--;

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
				lastvisinc = PlayClock + 32;
				p->visibility = 0;
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
			if (actions & SB_FIRE) p->okickback_pic = p->kickback_pic = 1;
			else p->okickback_pic = p->kickback_pic = 0;
		}

		break;

	case GROW_WEAPON:		// m-14 with scope (sniper rifle)

		bool check;
		if (isNam())
		{
			p->kickback_pic++;
			check = (p->kickback_pic == 3);
		}
		else
		{
			check = (p->kickback_pic > 3);
		}
		if (check)
		{
			// fire now, but don't reload right away...
			if (isNam())
			{
				p->kickback_pic++;
				if (p->ammo_amount[p->curr_weapon] <= 1)
					p->okickback_pic = p->kickback_pic = 0;
			}
			else
				p->okickback_pic = p->kickback_pic = 0;
			p->ammo_amount[p->curr_weapon]--;
			shoot(pact, DukeGrowSparkClass);

			if (!isNam())
			{
				// make them visible if not set...
				p->visibility = 0;
				lastvisinc = PlayClock + 32;
				checkavailweapon(p);
			}
		}
		else if (!isNam()) p->kickback_pic++;
		if (isNam() && p->kickback_pic > 30)
		{
			// reload now...
			p->okickback_pic = p->kickback_pic = 0;
			// make them visible if not set...
			p->visibility = 0;
			lastvisinc = PlayClock + 32;
			checkavailweapon(p);
		}
		break;

	case SHRINKER_WEAPON:	// m-79 in NAM (Grenade launcher)
		if ((!isNam() && p->kickback_pic > 10) || (isNam() && p->kickback_pic == 10))
		{
			if (isNam()) p->kickback_pic++; // fire now, but wait for reload...
			else p->okickback_pic = p->kickback_pic = 0;

			p->ammo_amount[SHRINKER_WEAPON]--;
			shoot(pact, DukeShrinkerClass);

			if (!isNam())
			{
				p->visibility = 0;
				//flashColor = 176 + (252 << 8) + (120 << 16);
				lastvisinc = PlayClock + 32;
				checkavailweapon(p);
			}
		}
		else if (isNam() && p->kickback_pic > 30)
		{
			p->okickback_pic = p->kickback_pic = 0;
			p->visibility = 0;
			lastvisinc = PlayClock + 32;
			checkavailweapon(p);
		}
		else p->kickback_pic++;
		break;

	case DEVISTATOR_WEAPON:
		if (p->kickback_pic)
		{
			p->kickback_pic++;

			if (isNam())
			{
				if ((p->kickback_pic >= 2) &&
					p->kickback_pic < 5 &&
					(p->kickback_pic & 1))
				{
					p->visibility = 0;
					lastvisinc = PlayClock + 32;
					shoot(pact, DukeRPGClass);
					p->ammo_amount[DEVISTATOR_WEAPON]--;
					checkavailweapon(p);
				}
				if (p->kickback_pic > 5) p->okickback_pic = p->kickback_pic = 0;
			}
			else if (p->kickback_pic & 1)
			{
				p->visibility = 0;
				lastvisinc = PlayClock + 32;
				shoot(pact, DukeRPGClass);
				p->ammo_amount[DEVISTATOR_WEAPON]--;
				checkavailweapon(p);
				if (p->ammo_amount[DEVISTATOR_WEAPON] <= 0) p->okickback_pic = p->kickback_pic = 0;
			}
			if (p->kickback_pic > 5) p->okickback_pic = p->kickback_pic = 0;
		}
		break;
	case FREEZE_WEAPON:
		// flame thrower in NAM

		if (p->kickback_pic < 4)
		{
			p->kickback_pic++;
			if (p->kickback_pic == 3)
			{
				p->ammo_amount[p->curr_weapon]--;

				p->visibility = 0;
				lastvisinc = PlayClock + 32;
				shoot(pact, DukeFreezeBlastClass);
				checkavailweapon(p);
			}
			if (pact->spr.scale.X < 0.5)
			{
				p->okickback_pic = p->kickback_pic = 0; break;
			}
		}
		else
		{
			if (actions & SB_FIRE)
			{
				p->okickback_pic = p->kickback_pic = 1;
				S_PlayActorSound(CAT_FIRE, pact);
			}
			else p->okickback_pic = p->kickback_pic = 0;
		}
		break;

	case FLAMETHROWER_WEAPON:
		if (!isWorldTour()) // Twentieth Anniversary World Tour
			break;
		p->kickback_pic++;
		if (p->kickback_pic == 2) 
		{
			if (p->cursector->lotag != 2) 
			{
				p->ammo_amount[FLAMETHROWER_WEAPON]--;
				shoot(pact, DukeFireballClass);
			}
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 16) 
		{
			if ((actions & SB_FIRE) != 0)
			{
				p->okickback_pic = p->kickback_pic = 1;
				S_PlayActorSound(FLAMETHROWER_INTRO, pact);
			}
			else
				p->okickback_pic = p->kickback_pic = 0;
		}
		break;

	case TRIPBOMB_WEAPON:	// Claymore in NAM
		if (p->kickback_pic < 4)
		{
			pact->restorez();
			p->vel.Z = 0;
			if (p->kickback_pic == 3)
				shoot(pact, DukeHandHoldingLaserClass);
		}
		if (p->kickback_pic == 16)
		{
			p->okickback_pic = p->kickback_pic = 0;
			checkavailweapon(p);
			p->oweapon_pos = p->weapon_pos = -9;
		}
		else p->kickback_pic++;
		break;
	case KNEE_WEAPON:
		p->kickback_pic++;

		if (p->kickback_pic == 7) shoot(pact, DukeMeleeAttackClass);
		else if (p->kickback_pic == 14)
		{
			if (actions & SB_FIRE)
				p->okickback_pic = p->kickback_pic = 1 + (krand() & 3);
			else p->okickback_pic = p->kickback_pic = 0;
		}

		if (p->wantweaponfire >= 0)
			checkavailweapon(p);
		break;

	case RPG_WEAPON:	// m-72 in NAM (LAW)
		p->kickback_pic++;
		if (p->kickback_pic == 4)
		{
			p->ammo_amount[RPG_WEAPON]--;
			lastvisinc = PlayClock + 32;
			p->visibility = 0;
			shoot(pact, DukeRPGClass);
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 20)
			p->okickback_pic = p->kickback_pic = 0;
		break;
		}
}

//---------------------------------------------------------------------------
//
// this function exists because gotos suck. :P
//
//---------------------------------------------------------------------------

static void processweapon(DDukePlayer* const p, ESyncBits actions)
{
	auto pact = p->GetActor();
	int shrunk = (pact->spr.scale.Y < 0.5);

	if (isNamWW2GI() && (actions & SB_HOLSTER)) // 'Holster Weapon
	{
		if (isWW2GI())
		{
			SetGameVarID(g_iReturnVarID, 0, pact, p->pnum);
			SetGameVarID(g_iWeaponVarID, p->curr_weapon, pact, p->pnum);
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike(p->curr_weapon, p), pact, p->pnum);
			OnEvent(EVENT_HOLSTER, p->pnum, pact, -1);
			if (GetGameVarID(g_iReturnVarID, pact, p->pnum).value() == 0)
			{
				// now it uses the game definitions...
				if (aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_HOLSTER_CLEARS_CLIP)
				{
					if (p->ammo_amount[p->curr_weapon] > aplWeaponClip(p->curr_weapon, p)
						&& (p->ammo_amount[p->curr_weapon] % aplWeaponClip(p->curr_weapon, p)) != 0)
					{
						// throw away the remaining clip
						p->ammo_amount[p->curr_weapon] -=
							p->ammo_amount[p->curr_weapon] % aplWeaponClip(p->curr_weapon, p);
						p->kickback_pic = aplWeaponTotalTime(p->curr_weapon, p) + 1;	// animate, but don't shoot...
						actions &= ~SB_FIRE; // not firing...
					}
					return;
				}
			}
		}
		else if (p->curr_weapon == PISTOL_WEAPON)
		{
			if (p->ammo_amount[PISTOL_WEAPON] > 20)
			{
				// throw away the remaining clip
				p->ammo_amount[PISTOL_WEAPON] -= p->ammo_amount[PISTOL_WEAPON] % 20;
				p->kickback_pic = 3;	// animate, but don't shoot...
				actions &= ~SB_FIRE; // not firing...
			}
			return;
		}
	}


	if (isWW2GI() && (aplWeaponFlags(p->curr_weapon, p) & WEAPON_FLAG_GLOWS))
		p->random_club_frame += 64; // Glowing

	if (!isWW2GI() && (p->curr_weapon == SHRINKER_WEAPON || p->curr_weapon == GROW_WEAPON))
		p->random_club_frame += 64; // Glowing

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
		if (!isWW2GI()) fireweapon(p);
		else fireweapon_ww(p);
	}
	else if (p->kickback_pic)
	{
		if (!isWW2GI()) operateweapon(p, actions);
		else operateweapon_ww(p, actions);
	}
}
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void processinput_d(DDukePlayer* const p)
{
	int k, doubvel;
	double floorz, ceilingz, truefdist;
	Collision chz, clz;
	bool shrunk;
	int psectlotag;

	auto pact = p->GetActor();

	ESyncBits& actions = p->cmd.ucmd.actions;

	// Get strafe value before it's rotated by the angle.
	const auto RollVel = p->cmd.ucmd.vel.Y;

	rotateInputVel(p);

	auto psectp = p->cursector;
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
	p->spritebridge = 0;

	shrunk = (pact->spr.scale.Y < 0.5);
	getzrange(pact->getPosWithOffsetZ(), psectp, &ceilingz, chz, &floorz, clz, 10.1875, CLIPMASK0);

	setPlayerActorViewZOffset(pact);

	p->truefz = getflorzofslopeptr(psectp, pact->getPosWithOffsetZ());
	p->truecz = getceilzofslopeptr(psectp, pact->getPosWithOffsetZ());

	truefdist = abs(pact->getOffsetZ() - p->truefz);
	if (clz.type == kHitSector && psectlotag == 1 && truefdist > gs.playerheight + 16)
		psectlotag = 0;

	pact->floorz = floorz;
	pact->ceilingz = ceilingz;

	p->doViewPitch();

	if (chz.type == kHitSprite)
	{
		if (chz.actor()->spr.statnum == 1 && chz.actor()->spr.extra >= 0)
		{
			chz.setNone();
			ceilingz = p->truecz;
		}
	}

	if (clz.type == kHitSprite)
	{
		if ((clz.actor()->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_BLOCK)) == (CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_BLOCK))
		{
			psectlotag = 0;
			p->footprintcount = 0;
			p->spritebridge = 1;
		}
		else if (badguy(clz.actor()) && clz.actor()->spr.scale.X > 0.375 && abs(pact->spr.pos.Z - clz.actor()->spr.pos.Z) < 84)
		{
			auto ang = (clz.actor()->spr.pos.XY() - pact->spr.pos.XY()).Angle();
			p->vel -= ang.ToVector();
		}
		CallStandingOn(clz.actor(), p);
	}


	if (pact->spr.extra > 0) incur_damage(p);
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

	if (pact->spr.scale.X < 0.625 && p->jetpack_on == 0)
	{
		p->ofistsign = p->fistsign;
		p->fistsign += int(pact->vel.X * 16);
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
		gameInput.ForceInputSync(p->pnum);
		p->vel.X = p->vel.Y = 0;
		pact->vel.X = 0;

		fi.doincrements(p);

		if (isWW2GI() && aplWeaponWorksLike(p->curr_weapon, p) == HANDREMOTE_WEAPON) processweapon(p, actions);
		if (!isWW2GI() && p->curr_weapon == HANDREMOTE_WEAPON) processweapon(p, actions);
		return;
	}

	doubvel = TICSPERFRAME;

	checklook(p, actions);
	p->doViewYaw();

	p->updatecentering();

	if (p->on_crane != nullptr)
	{
		gameInput.ForceInputSync(p->pnum);
		goto HORIZONLY;
	}

	p->playerweaponsway(pact->vel.X);

	pact->vel.X = (pact->spr.pos.XY() - p->bobpos).Length();
	if (p->on_ground) p->bobcounter += int(pact->vel.X * 8);

	p->backuppos(ud.clipping == 0 && ((p->insector() && p->cursector->floortexture == mirrortex) || !p->insector()));

	// Shrinking code

	if (psectlotag == ST_2_UNDERWATER)
	{
		underwater(p, actions, floorz, ceilingz);
	}
	else if (p->jetpack_on)
	{
		operateJetpack(p, actions, psectlotag, floorz, ceilingz, shrunk);
	}
	else if (psectlotag != ST_2_UNDERWATER)
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
		gameInput.ForceInputSync(p->pnum);
	}

	p->doYawInput();

	purplelavacheck(p);

	if (p->spritebridge == 0 && pact->insector())
	{
		auto sect = pact->sector();
		k = 0;

		if (p->on_ground && truefdist <= gs.playerheight + 16)
		{
			int surface = tilesurface(sect->floortexture);
			int whichsound = surface == TSURF_ELECTRIC? 0 : surface == TSURF_SLIME? 1 : surface == TSURF_PLASMA? 2 : -1;
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
						S_PlayActorSound(DUKE_WALKINDUCTS, pact);
						p->walking_snd_toggle = 1;
					}
					break;
				case ST_1_ABOVE_WATER:
					if ((krand() & 1) == 0)
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

		p->vel += p->cmd.ucmd.vel.XY() * doubvel * (5. / 16.);
		p->RollVel += RollVel * doubvel * (5. / 16.);

		bool check;

		if (!isWW2GI()) check = ((p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 10 && p->on_ground) || (p->on_ground && (actions & SB_CROUCH)));
		else check = ((aplWeaponWorksLike(p->curr_weapon, p) == KNEE_WEAPON && p->kickback_pic > 10 && p->on_ground) || (p->on_ground && (actions & SB_CROUCH)));
		if (check)
		{
			p->vel.SetXY(p->vel.XY() * (gs.playerfriction - 0.125));
			p->RollVel *= gs.playerfriction - 0.125;
		}
		else
		{
			if (psectlotag == ST_2_UNDERWATER)
			{
				p->vel.SetXY(p->vel.XY() * (gs.playerfriction - FixedToFloat(0x1400)));
				p->RollVel *= gs.playerfriction - FixedToFloat(0x1400);
			}
			else
			{
				p->vel.SetXY(p->vel.XY() * gs.playerfriction);
				p->RollVel *= gs.playerfriction;
			}
		}

		if (abs(p->vel.X) < 1/128. && abs(p->vel.Y) < 1 / 128.)
		{
			p->vel.X = p->vel.Y = 0;
			p->RollVel = 0;
		}

		if (shrunk)
		{
			p->vel.SetXY(p->vel.XY() * (gs.playerfriction * 0.75));
			p->RollVel *= gs.playerfriction * 0.75;
		}
	}

	p->doRollInput((psectlotag == 1) || (psectlotag == 2));

HORIZONLY:

	double iif = (psectlotag == 1 || p->spritebridge == 1) ? 4 : 20;

	if (p->insector() && p->cursector->lotag == 2) k = 0;
	else k = 1;

	Collision clip{};
	if (ud.clipping)
	{
		pact->spr.pos += p->vel.XY();
		updatesector(pact->getPosWithOffsetZ(), &p->cursector);
		ChangeActorSect(pact, p->cursector);
	}
	else
		clipmove(pact->spr.pos, pact->getOffsetZ(), &p->cursector, p->vel.XY(), 10.25, 4., iif, CLIPMASK0, clip);

	if (p->jetpack_on == 0 && psectlotag != 2 && psectlotag != 1 && shrunk)
		pact->spr.pos.Z += 32;

	if (clip.type != kHitNone)
		checkplayerhurt_d(p, clip);

	if (p->jetpack_on == 0)
	{
		if (pact->vel.X > 1)
		{
			if (psectlotag != 1 && psectlotag != 2 && p->on_ground)
			{
				p->pycount += 52;
				p->pycount &= 2047;
				const double factor = 1024. / 1596; // What is 1596?
				p->pyoff = abs(pact->vel.X * BobVal(p->pycount)) * factor;
			}
		}
		else if (psectlotag != 2 && psectlotag != 1)
			p->pyoff = 0;
	}

	// RBG***
	SetActor(pact, pact->spr.pos);

	if (psectlotag < 3)
	{
		psectp = pact->sector();
		if (ud.clipping == 0 && psectp->lotag == 31)
		{
			auto secact = barrier_cast<DDukeActor*>(psectp->hitagactor);
			if (secact && secact->vel.X != 0 && secact->counter == 0)
			{
				quickkill(p);
				return;
			}
		}
	}

	if (truefdist < gs.playerheight && p->on_ground && psectlotag != 1 && shrunk == 0 && p->insector() && p->cursector->lotag == 1)
		if (!S_CheckActorSoundPlaying(pact, DUKE_ONWATER))
			S_PlayActorSound(DUKE_ONWATER, pact);

	if (p->cursector != pact->sector())
		ChangeActorSect(pact, p->cursector);

	auto oldpos = pact->opos;
	int retry = 0;
	while (ud.clipping == 0)
	{
		int blocked;
		blocked = (pushmove(pact->spr.pos, pact->getOffsetZ(), &p->cursector, 10.25, 4, 4, CLIPMASK0) < 0 && furthestangle(pact, 8) < DAngle90);

		if (fabs(pact->floorz - pact->ceilingz) < 48 || blocked)
		{
			if (!(pact->sector()->lotag & 0x8000) && (isanunderoperator(pact->sector()->lotag) ||
				isanearoperator(pact->sector()->lotag)))
				fi.activatebysector(pact->sector(), pact);
			if (blocked)
			{
				if (!retry++)
				{
					pact->spr.pos = oldpos;
					pact->backuppos();
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

	// center_view
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
	else if ((actions & SB_LOOK_UP) == SB_AIM_UP)
	{
		playerAimUp(p, actions);
	}
	else if ((actions & SB_LOOK_DOWN) == SB_AIM_DOWN)
	{	// aim_down
		playerAimDown(p, actions);
	}

	p->doPitchInput();

	p->checkhardlanding();

	//Shooting code/changes

	if (p->show_empty_weapon > 0)
	{
		p->show_empty_weapon--;
		if (p->show_empty_weapon == 0 && (WeaponSwitch(p->pnum) & 2))
		{
			if (p->last_full_weapon == GROW_WEAPON)
				p->subweapon |= (1 << GROW_WEAPON);
			else if (p->last_full_weapon == SHRINKER_WEAPON)
				p->subweapon &= ~(1 << GROW_WEAPON);
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

	// HACKS
	processweapon(p, actions);
}

END_DUKE_NS

