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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------


#include "ns.h"
#include "global.h"
#include "gamecontrol.h"
#include "v_video.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// handles all HUD related input, i.e. inventory item selection and activation plus weapon selection.
//
// Note: This doesn't restrict the events to WW2GI - since the other games do 
// not define any by default there is no harm done keeping the code clean.
//
//---------------------------------------------------------------------------

void hud_input(DDukePlayer* const p)
{
	int i, k;
	uint8_t dainv;

	auto pact = p->GetActor();

	i = p->aim_mode;
	p->aim_mode = !(p->cmd.ucmd.actions & SB_AIMMODE);
	if (p->aim_mode < i)
		p->cmd.ucmd.actions |= SB_CENTERVIEW;

	// Backup weapon here as hud_input() is the first function where any one of the weapon variables can change.
	p->backupweapon();

	if (isRR() && (p->cmd.ucmd.actions & SB_CROUCH)) p->cmd.ucmd.actions &= ~SB_JUMP;

	if ((isRR() && p->drink_amt > 88))
		p->cmd.ucmd.actions |= SB_LOOK_LEFT;
	if ((isRR() && p->drink_amt > 99))
		p->cmd.ucmd.actions |= SB_LOOK_DOWN;

	if (isRR())
	{
		if (!!(p->cmd.ucmd.actions & SB_QUICK_KICK) && p->last_pissed_time == 0)
		{
			if (!isRRRA() || pact->spr.extra > 0)
			{
				p->last_pissed_time = 4000;
				S_PlayActorSound(437, pact);
				if (pact->spr.extra <= gs.max_player_health - gs.max_player_health / 10)
				{
					pact->spr.extra += 2;
					p->last_extra = pact->spr.extra;
				}
				else if (pact->spr.extra < gs.max_player_health)
					pact->spr.extra = gs.max_player_health;
			}
		}
	}
	else
	{
		if (!!(p->cmd.ucmd.actions & SB_QUICK_KICK) && p->quick_kick == 0 && (p->curr_weapon != KNEE_WEAPON || p->kickback_pic == 0))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, p->pnum);
			OnEvent(EVENT_QUICKKICK, p->pnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, p->pnum).value() == 0)
			{
				p->quick_kick = 14;
				if (!p->quick_kick_msg && p->pnum == screenpeek) FTA(QUOTE_MIGHTY_FOOT, p);
				p->quick_kick_msg = true;
			}
		}
	}
	if (!(p->cmd.ucmd.actions & SB_QUICK_KICK)) p->quick_kick_msg = false;

	if (!(p->cmd.ucmd.actions & SB_INTERFACE_BITS))
		p->interface_toggle_flag = 0;
	else if (p->interface_toggle_flag == 0)
	{
		p->interface_toggle_flag = 1;

		// Don't go on if paused or dead.
		if (paused) return;
		if (pact->spr.extra <= 0) return;

		// Activate an inventory item. This just forwards to the other inventory bits. If the inventory selector was taken out of the playsim this could be removed.
		if (!!(p->cmd.ucmd.actions & SB_INVUSE) && p->newOwner == nullptr)
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, p->pnum);
			OnEvent(EVENT_INVENTORY, p->pnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, p->pnum).value() == 0 && p->inven_icon > ICON_NONE && p->inven_icon <= ICON_HEATS)
			{
				p->useItem(p->inven_icon);
			}
		}

		if (!isRR() && p->itemUsed(ICON_HEATS))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, p->pnum);
			OnEvent(EVENT_USENIGHTVISION, p->pnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, p->pnum).value() == 0 && p->heat_amount > 0)
			{
				p->heat_on = !p->heat_on;
				p->inven_icon = 5;
				S_PlayActorSound(NITEVISION_ONOFF, pact);
				FTA(106 + (!p->heat_on), p);
			}
		}

		if (p->itemUsed(ICON_STEROIDS))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, p->pnum);
			OnEvent(EVENT_USESTEROIDS, p->pnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, p->pnum).value() == 0)
			{
				if (p->steroids_amount == 400)
				{
					p->steroids_amount--;
					S_PlayActorSound(DUKE_TAKEPILLS, pact);
					p->inven_icon = ICON_STEROIDS;
					FTA(12, p);
				}
			}
			return;
		}

		if (!!(p->cmd.ucmd.actions & SB_INVPREV) || !!(p->cmd.ucmd.actions & SB_INVNEXT))
		{
			p->invdisptime = 26 * 2;

			if (!!(p->cmd.ucmd.actions & SB_INVNEXT)) k = 1;
			else k = 0;

			dainv = p->inven_icon;

			i = 0;
		CHECKINV1:

			if (i < 9)
			{
				i++;

				switch (dainv)
				{
				case 4:
					if (p->jetpack_amount > 0 && i > 1)
						break;
					if (k) dainv = 5;
					else dainv = 3;
					goto CHECKINV1;
				case 6:
					if (p->scuba_amount > 0 && i > 1)
						break;
					if (k) dainv = 7;
					else dainv = 5;
					goto CHECKINV1;
				case 2:
					if (p->steroids_amount > 0 && i > 1)
						break;
					if (k) dainv = 3;
					else dainv = 1;
					goto CHECKINV1;
				case 3:
					if (p->holoduke_amount > 0 && i > 1)
						break;
					if (k) dainv = 4;
					else dainv = 2;
					goto CHECKINV1;
				case 0:
				case 1:
					if (p->firstaid_amount > 0 && i > 1)
						break;
					if (k) dainv = 2;
					else dainv = 7;
					goto CHECKINV1;
				case 5:
					if (p->heat_amount > 0 && i > 1)
						break;
					if (k) dainv = 6;
					else dainv = 4;
					goto CHECKINV1;
				case 7:
					if (p->boot_amount > 0 && i > 1)
						break;
					if (k) dainv = 1;
					else dainv = 6;
					goto CHECKINV1;
				}
			}
			else dainv = 0;

			// These events force us to keep the inventory selector in the playsim as opposed to the UI where it really belongs.
			if (!!(p->cmd.ucmd.actions & SB_INVPREV))
			{
				SetGameVarID(g_iReturnVarID, dainv, nullptr, p->pnum);
				OnEvent(EVENT_INVENTORYLEFT, p->pnum, nullptr, -1);
				dainv = GetGameVarID(g_iReturnVarID, nullptr, p->pnum).safeValue();
			}
			if (!!(p->cmd.ucmd.actions & SB_INVNEXT))
			{
				SetGameVarID(g_iReturnVarID, dainv, nullptr, p->pnum);
				OnEvent(EVENT_INVENTORYRIGHT, p->pnum, nullptr, -1);
				dainv = GetGameVarID(g_iReturnVarID, nullptr, p->pnum).safeValue();
			}
			p->inven_icon = dainv;
			// Someone must have really hated constant data, doing this with a switch/case (and of course also with literal numbers...)
			static const uint8_t invquotes[] = { QUOTE_MEDKIT, QUOTE_STEROIDS, QUOTE_HOLODUKE, QUOTE_JETPACK, QUOTE_NVG, QUOTE_SCUBA, QUOTE_BOOTS };
			if (dainv >= 1 && dainv < 8) FTA(invquotes[dainv - 1], p);
		}

		int weap = p->cmd.ucmd.getNewWeapon();
		if (weap > 1 && p->kickback_pic > 0)
			p->wantweaponfire = weap - 1;

		// Here we have to be extra careful that the weapons do not get mixed up, so let's keep the code for Duke and RR completely separate.
		fi.selectweapon(p, weap);

		if (!!(p->cmd.ucmd.actions & SB_HOLSTER))
		{
			if (p->curr_weapon > KNEE_WEAPON)
			{
				if (p->holster_weapon == 0 && p->weapon_pos == 0)
				{
					p->holster_weapon = 1;
					p->weapon_pos = -1;
					FTA(QUOTE_WEAPON_LOWERED, p);
				}
				else if (p->holster_weapon == 1 && p->weapon_pos == -9)
				{
					p->holster_weapon = 0;
					p->weapon_pos = 10;
					FTA(QUOTE_WEAPON_RAISED, p);
				}
			}
		}

		if (p->itemUsed(ICON_HOLODUKE) && (isRR() || p->newOwner == nullptr))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, p->pnum);
			OnEvent(EVENT_HOLODUKEON, p->pnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, p->pnum).value() == 0)
			{
				if (!isRR())
				{
					if (p->holoduke_on == nullptr)
					{
						if (p->holoduke_amount > 0)
						{
							p->inven_icon = 3;

							auto pactor =
								CreateActor(p->cursector, pact->getPosWithOffsetZ().plusZ(30), DukePlayerPawnClass, -64, DVector2(0, 0), pact->spr.Angles.Yaw, 0., 0., nullptr, 10);
							pactor->temp_data[3] = pactor->temp_data[4] = 0;
							p->holoduke_on = pactor;
							pactor->spr.yint = p->pnum;
							pactor->spr.extra = 0;
							FTA(QUOTE_HOLODUKE_ON, p);
							S_PlayActorSound(TELEPORTER, p->holoduke_on);
						}
						else FTA(QUOTE_HOLODUKE_NOT_FOUND, p);

					}
					else
					{
						S_PlayActorSound(TELEPORTER, p->holoduke_on);
						p->holoduke_on = nullptr;
						FTA(QUOTE_HOLODUKE_OFF, p);
					}
				}
				else // In RR this means drinking whiskey.
				{
					if (p->holoduke_amount > 0 && pact->spr.extra < gs.max_player_health)
					{
						p->holoduke_amount -= 400;
						pact->spr.extra += 5;
						if (pact->spr.extra > gs.max_player_health)
							pact->spr.extra = gs.max_player_health;

						p->drink_amt += 5;
						p->inven_icon = 3;
						if (p->holoduke_amount == 0)
							checkavailinven(p);

						if (p->drink_amt < 99 && !S_CheckActorSoundPlaying(pact, 425))
							S_PlayActorSound(425, pact);
					}
				}
			}
		}

		if (isRR() && p->itemUsed(ICON_HEATS) && p->newOwner == nullptr)
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, p->pnum);
			OnEvent(EVENT_USENIGHTVISION, p->pnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, p->pnum).value() == 0)
			{
				if (p->yehaa_timer == 0)
				{
					p->yehaa_timer = 126;
					S_PlayActorSound(390, pact);
					p->noise_radius = 1024;
					madenoise(p);
					if (p->cursector->lotag == 857)
					{
						if (pact->spr.extra <= gs.max_player_health)
						{
							pact->spr.extra += 10;
							if (pact->spr.extra >= gs.max_player_health)
								pact->spr.extra = gs.max_player_health;
						}
					}
					else
					{
						if (pact->spr.extra + 1 <= gs.max_player_health)
						{
							pact->spr.extra++;
						}
					}
				}
			}
		}

		if (p->itemUsed(ICON_FIRSTAID))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, p->pnum);
			OnEvent(EVENT_USEMEDKIT, p->pnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, p->pnum).value() == 0)
			{
				if (p->firstaid_amount > 0 && pact->spr.extra < gs.max_player_health)
				{
					if (!isRR())
					{
						int j = gs.max_player_health - pact->spr.extra;

						if (p->firstaid_amount > j)
						{
							p->firstaid_amount -= j;
							pact->spr.extra = gs.max_player_health;
							p->inven_icon = 1;
						}
						else
						{
							pact->spr.extra += p->firstaid_amount;
							p->firstaid_amount = 0;
							checkavailinven(p);
						}
						S_PlayActorSound(DUKE_USEMEDKIT, pact);
					}
					else
					{
						int j = 10;
						if (p->firstaid_amount > j)
						{
							p->firstaid_amount -= j;
							pact->spr.extra += j;
							if (pact->spr.extra > gs.max_player_health)
								pact->spr.extra = gs.max_player_health;
							p->inven_icon = 1;
						}
						else
						{
							pact->spr.extra += p->firstaid_amount;
							p->firstaid_amount = 0;
							checkavailinven(p);
						}
						if (pact->spr.extra > gs.max_player_health)
							pact->spr.extra = gs.max_player_health;
						p->drink_amt += 10;
						if (p->drink_amt <= 100 && !S_CheckActorSoundPlaying(pact, DUKE_USEMEDKIT))
							S_PlayActorSound(DUKE_USEMEDKIT, pact);
					}
				}
			}
		}

		if (p->itemUsed(ICON_JETPACK) && (isRR() || p->newOwner == nullptr))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, p->pnum);
			OnEvent(EVENT_USEJETPACK, p->pnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, p->pnum).value() == 0)
			{
				if (!isRR())
				{
					if (p->jetpack_amount > 0)
					{
						p->jetpack_on = !p->jetpack_on;
						if (p->jetpack_on)
						{
							p->inven_icon = 4;

							S_StopSound(DUKE_SCREAM, pact);	// this will stop the falling scream
							S_PlayActorSound(DUKE_JETPACK_ON, pact);
							FTA(QUOTE_JETPACK_ON, p);
						}
						else
						{
							p->hard_landing = 0;
							p->vel.Z = 0;
							S_PlayActorSound(DUKE_JETPACK_OFF, pact);
							S_StopSound(DUKE_JETPACK_IDLE, pact);
							S_StopSound(DUKE_JETPACK_ON, pact);
							FTA(QUOTE_JETPACK_OFF, p);
						}
					}
					else FTA(QUOTE_JETPACK_NOT_FOUND, p);
				}
				else
				{
					// eat cow pie
					if (p->jetpack_amount > 0 && pact->spr.extra < gs.max_player_health)
					{
						if (!S_CheckActorSoundPlaying(pact, 429))
							S_PlayActorSound(429, pact);

						p->jetpack_amount -= 100;
						if (p->drink_amt > 0)
						{
							p->drink_amt -= 5;
							if (p->drink_amt < 0)
								p->drink_amt = 0;
						}

						if (p->eat < 100)
						{
							p->eat += 5;
							if (p->eat > 100)
								p->eat = 100;
						}

						pact->spr.extra += 5;

						p->inven_icon = 4;

						if (pact->spr.extra > gs.max_player_health)
							pact->spr.extra = gs.max_player_health;

						if (p->jetpack_amount <= 0)
							checkavailinven(p);
					}
				}
			}
		}

		if (!!(p->cmd.ucmd.actions & SB_TURNAROUND) && p->YawSpin == nullAngle && p->on_crane == nullptr)
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, p->pnum);
			OnEvent(EVENT_TURNAROUND, p->pnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, p->pnum).value() != 0)
			{
				p->cmd.ucmd.actions &= ~SB_TURNAROUND;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// External entry point
//
//---------------------------------------------------------------------------

void GameInterface::doPlayerMovement()
{
	const auto p = getPlayer(myconnectindex);

	if (isRRRA() && (p->OnMotorcycle || p->OnBoat))
	{
		static constexpr double VEHICLETURN = mapangle(20).Degrees();
		double baseVel, velScale; unsigned vehFlags;

		if (p->OnMotorcycle)
		{
			vehFlags = VEH_CANTURN | (VEH_CANMOVE * !p->moto_underwater) | (VEH_SCALETURN * (p->MotoSpeed <= 0));
			velScale = (3. / 10.);
			baseVel = VEHICLETURN * Sgn(p->MotoSpeed);
		}
		else
		{
			vehFlags = VEH_CANMOVE | (VEH_CANTURN * (p->MotoSpeed || p->moto_drink));
			velScale = !p->NotOnWater? 1. : (6. / 19.);
			baseVel = VEHICLETURN * velScale;
		}

		gameInput.processVehicle(baseVel, velScale, vehFlags);
	}
	else
	{
		gameInput.processMovement((p->psectlotag != ST_2_UNDERWATER) ? 1 : 0.875, true, p->drink_amt);
	}
}


END_DUKE_NS
