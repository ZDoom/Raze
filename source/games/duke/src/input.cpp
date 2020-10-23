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

// State timer counters. 
static int turnheldtime;
static int lastcontroltime;
static InputPacket loc; // input accumulation buffer.

//---------------------------------------------------------------------------
//
// handles all HUD related input, i.e. inventory item selection and activation plus weapon selection.
//
// Note: This doesn't restrict the events to WW2GI - since the other games do 
// not define any by default there is no harm done keeping the code clean.
//
//---------------------------------------------------------------------------

void hud_input(int plnum)
{
	int i, k;
	uint8_t dainv;
	struct player_struct* p;
	short unk;

	unk = 0;
	p = &ps[plnum];

	i = p->aim_mode;
	p->aim_mode = !PlayerInput(plnum, SB_AIMMODE);
	if (p->aim_mode < i)
		p->sync.actions |= SB_CENTERVIEW;

	// Backup weapon here as hud_input() is the first function where any one of the weapon variables can change.
	backupweapon(p);

	if (isRR())
	{
		if (PlayerInput(plnum, SB_QUICK_KICK) && p->last_pissed_time == 0)
		{
			if (!isRRRA() || p->GetActor()->s.extra > 0)
			{
				p->last_pissed_time = 4000;
				S_PlayActorSound(437, p->i);
				if (p->GetActor()->s.extra <= max_player_health - max_player_health / 10)
				{
					p->GetActor()->s.extra += 2;
					p->last_extra = p->GetActor()->s.extra;
				}
				else if (p->GetActor()->s.extra < max_player_health)
					p->GetActor()->s.extra = max_player_health;
			}
		}
	}
	else
	{
		if (PlayerInput(plnum, SB_QUICK_KICK) && p->quick_kick == 0 && (p->curr_weapon != KNEE_WEAPON || p->kickback_pic == 0))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, plnum);
			OnEvent(EVENT_QUICKKICK, plnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, plnum) == 0)
			{
				p->quick_kick = 14;
				if (!p->quick_kick_msg && plnum == screenpeek) FTA(QUOTE_MIGHTY_FOOT, p);
				p->quick_kick_msg = true;
			}
		}
	}
	if (!PlayerInput(plnum, SB_QUICK_KICK)) p->quick_kick_msg = false;

	if (!PlayerInputBits(plnum, SB_INTERFACE_BITS))
		p->interface_toggle_flag = 0;
	else if (p->interface_toggle_flag == 0)
	{
		p->interface_toggle_flag = 1;

		// Don't go on if paused or dead.
		if (paused) return;
		if (p->GetActor()->s.extra <= 0) return;

		// Activate an inventory item. This just forwards to the other inventory bits. If the inventory selector was taken out of the playsim this could be removed.
		if (PlayerInput(plnum, SB_INVUSE) && p->newowner == -1)
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, plnum);
			OnEvent(EVENT_INVENTORY, plnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, plnum) == 0)
			{
				if (p->inven_icon > ICON_NONE && p->inven_icon <= ICON_HEATS) PlayerSetItemUsed(plnum, p->inven_icon);
			}
		}

		if (!isRR() && PlayerUseItem(plnum, ICON_HEATS))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, plnum);
			OnEvent(EVENT_USENIGHTVISION, plnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, plnum) == 0 && p->heat_amount > 0)
			{
				p->heat_on = !p->heat_on;
				setpal(p);
				p->inven_icon = 5;
				S_PlayActorSound(NITEVISION_ONOFF, p->i);
				FTA(106 + (!p->heat_on), p);
			}
		}

		if (PlayerUseItem(plnum, ICON_STEROIDS))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, plnum);
			OnEvent(EVENT_USESTEROIDS, plnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, plnum) == 0)
			{
				if (p->steroids_amount == 400)
				{
					p->steroids_amount--;
					S_PlayActorSound(DUKE_TAKEPILLS, p->i);
					p->inven_icon = ICON_STEROIDS;
					FTA(12, p);
				}
			}
			return;
		}

		if (PlayerInput(plnum, SB_INVPREV) || PlayerInput(plnum, SB_INVNEXT))
		{
			p->invdisptime = 26 * 2;

			if (PlayerInput(plnum, SB_INVNEXT)) k = 1;
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
			if (PlayerInput(plnum, SB_INVPREV))
			{
				SetGameVarID(g_iReturnVarID, dainv, nullptr, plnum);
				OnEvent(EVENT_INVENTORYLEFT, plnum, nullptr, -1);
				dainv = GetGameVarID(g_iReturnVarID, nullptr, plnum);
			}
			if (PlayerInput(plnum, SB_INVNEXT))
			{
				SetGameVarID(g_iReturnVarID, dainv, nullptr, plnum);
				OnEvent(EVENT_INVENTORYRIGHT, plnum, nullptr, -1);
				dainv = GetGameVarID(g_iReturnVarID, nullptr, plnum);
			}
			p->inven_icon = dainv;
			// Someone must have really hated constant data, doing this with a switch/case (and of course also with literal numbers...)
			static const uint8_t invquotes[] = { QUOTE_MEDKIT, QUOTE_STEROIDS, QUOTE_HOLODUKE, QUOTE_JETPACK, QUOTE_NVG, QUOTE_SCUBA, QUOTE_BOOTS };
			if (dainv >= 1 && dainv < 8) FTA(invquotes[dainv - 1], p);
		}

		int weap = PlayerNewWeapon(plnum);
		if (weap > 1 && p->kickback_pic > 0)
			p->wantweaponfire = weap - 1;

		// Here we have to be extra careful that the weapons do not get mixed up, so let's keep the code for Duke and RR completely separate.
		fi.selectweapon(plnum, weap);

		if (PlayerInput(plnum, SB_HOLSTER))
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

		if (PlayerUseItem(plnum, ICON_HOLODUKE) && (isRR() || p->newowner == -1))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, plnum);
			OnEvent(EVENT_HOLODUKEON, plnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, plnum) == 0)
			{
				if (!isRR())
				{
					if (p->holoduke_on == nullptr)
					{
						if (p->holoduke_amount > 0)
						{
							p->inven_icon = 3;

							auto pactor =
								EGS(p->cursectnum,
									p->posx,
									p->posy,
									p->posz + (30 << 8), TILE_APLAYER, -64, 0, 0, p->angle.ang.asbuild(), 0, 0, nullptr, 10);
							pactor->temp_data[3] = pactor->temp_data[4] = 0;
							p->holoduke_on = pactor;
							pactor->s.yvel = plnum;
							pactor->s.extra = 0;
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
					if (p->holoduke_amount > 0 && p->GetActor()->s.extra < max_player_health)
					{
						p->holoduke_amount -= 400;
						p->GetActor()->s.extra += 5;
						if (p->GetActor()->s.extra > max_player_health)
							p->GetActor()->s.extra = max_player_health;

						p->drink_amt += 5;
						p->inven_icon = 3;
						if (p->holoduke_amount == 0)
							checkavailinven(p);

						if (p->drink_amt < 99 && !S_CheckActorSoundPlaying(p->i, 425))
							S_PlayActorSound(425, p->i);
					}
				}
			}
		}

		if (isRR() && PlayerUseItem(plnum, ICON_HEATS) && p->newowner == -1)
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, plnum);
			OnEvent(EVENT_USENIGHTVISION, plnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, plnum) == 0)
			{
				if (p->yehaa_timer == 0)
				{
					p->yehaa_timer = 126;
					S_PlayActorSound(390, p->i);
					p->noise_radius = 16384;
					madenoise(plnum);
					if (sector[p->cursectnum].lotag == 857)
					{
						if (p->GetActor()->s.extra <= max_player_health)
						{
							p->GetActor()->s.extra += 10;
							if (p->GetActor()->s.extra >= max_player_health)
								p->GetActor()->s.extra = max_player_health;
						}
					}
					else
					{
						if (p->GetActor()->s.extra + 1 <= max_player_health)
						{
							p->GetActor()->s.extra++;
						}
					}
				}
			}
		}

		if (PlayerUseItem(plnum, ICON_FIRSTAID))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, plnum);
			OnEvent(EVENT_USEMEDKIT, plnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, plnum) == 0)
			{
				if (p->firstaid_amount > 0 && p->GetActor()->s.extra < max_player_health)
				{
					if (!isRR())
					{
						int j = max_player_health - p->GetActor()->s.extra;

						if ((unsigned int)p->firstaid_amount > j)
						{
							p->firstaid_amount -= j;
							p->GetActor()->s.extra = max_player_health;
							p->inven_icon = 1;
						}
						else
						{
							p->GetActor()->s.extra += p->firstaid_amount;
							p->firstaid_amount = 0;
							checkavailinven(p);
						}
						S_PlayActorSound(DUKE_USEMEDKIT, p->i);
					}
					else
					{
						int j = 10;
						if (p->firstaid_amount > j)
						{
							p->firstaid_amount -= j;
							p->GetActor()->s.extra += j;
							if (p->GetActor()->s.extra > max_player_health)
								p->GetActor()->s.extra = max_player_health;
							p->inven_icon = 1;
						}
						else
						{
							p->GetActor()->s.extra += p->firstaid_amount;
							p->firstaid_amount = 0;
							checkavailinven(p);
						}
						if (p->GetActor()->s.extra > max_player_health)
							p->GetActor()->s.extra = max_player_health;
						p->drink_amt += 10;
						if (p->drink_amt <= 100 && !S_CheckActorSoundPlaying(p->i, DUKE_USEMEDKIT))
							S_PlayActorSound(DUKE_USEMEDKIT, p->i);
					}
				}
			}
		}

		if (PlayerUseItem(plnum, ICON_JETPACK) && (isRR() || p->newowner == -1))
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, plnum);
			OnEvent(EVENT_USEJETPACK, plnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, plnum) == 0)
			{
				if (!isRR())
				{
					if (p->jetpack_amount > 0)
					{
						p->jetpack_on = !p->jetpack_on;
						if (p->jetpack_on)
						{
							p->inven_icon = 4;

							S_StopSound(-1, p->i, CHAN_VOICE);	// this will stop the falling scream
							S_PlayActorSound(DUKE_JETPACK_ON, p->i);
							FTA(QUOTE_JETPACK_ON, p);
						}
						else
						{
							p->hard_landing = 0;
							p->poszv = 0;
							S_PlayActorSound(DUKE_JETPACK_OFF, p->i);
							S_StopSound(DUKE_JETPACK_IDLE, p->i);
							S_StopSound(DUKE_JETPACK_ON, p->i);
							FTA(QUOTE_JETPACK_OFF, p);
						}
					}
					else FTA(QUOTE_JETPACK_NOT_FOUND, p);
				}
				else
				{
					// eat cow pie
					if (p->jetpack_amount > 0 && p->GetActor()->s.extra < max_player_health)
					{
						if (!S_CheckActorSoundPlaying(p->i, 429))
							S_PlayActorSound(429, p->i);

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

						p->GetActor()->s.extra += 5;

						p->inven_icon = 4;

						if (p->GetActor()->s.extra > max_player_health)
							p->GetActor()->s.extra = max_player_health;

						if (p->jetpack_amount <= 0)
							checkavailinven(p);
					}
				}
			}
		}

		if (PlayerInput(plnum, SB_TURNAROUND) && p->angle.spin.asbam() == 0 && p->on_crane == nullptr)
		{
			SetGameVarID(g_iReturnVarID, 0, nullptr, plnum);
			OnEvent(EVENT_TURNAROUND, plnum, nullptr, -1);
			if (GetGameVarID(g_iReturnVarID, nullptr, plnum) != 0)
			{
				p->sync.actions &= ~SB_TURNAROUND;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// Main input routine.
// This includes several input improvements from EDuke32, but this code
// has been mostly rewritten completely to make it clearer and reduce redundancy.
//
//---------------------------------------------------------------------------

enum
{
	TURBOTURNTIME = (TICRATE/8), // 7
	NORMALTURN    = 15,
	PREAMBLETURN  = 5,
	NORMALKEYMOVE = 40,
	MAXVEL        = ((NORMALKEYMOVE*2)+10),
	MAXSVEL       = ((NORMALKEYMOVE*2)+10),
	MAXANGVEL     = 1024, // 127
	MAXHORIZVEL   = 256,  // 127

	MAXVELMOTO    = 120
};

//---------------------------------------------------------------------------
//
// handles the input bits
//
//---------------------------------------------------------------------------

static void processInputBits(player_struct *p, ControlInfo* const hidInput)
{
	ApplyGlobalInput(loc, hidInput);
	if (isRR() && (loc.actions & SB_CROUCH)) loc.actions &= ~SB_JUMP;

	if (p->OnMotorcycle || p->OnBoat)
	{
		// mask out all actions not compatible with vehicles.
		loc.actions &= ~(SB_WEAPONMASK_BITS | SB_TURNAROUND | SB_CENTERVIEW | SB_HOLSTER | SB_JUMP | SB_CROUCH | SB_RUN | 
			SB_AIM_UP | SB_AIM_DOWN | SB_AIMMODE | SB_LOOK_UP | SB_LOOK_DOWN | SB_LOOK_LEFT | SB_LOOK_RIGHT);
	}
	else
	{
		if (buttonMap.ButtonDown(gamefunc_Quick_Kick)) // this shares a bit with another function so cannot be in the common code.
			loc.actions |= SB_QUICK_KICK;

		if (buttonMap.ButtonDown(gamefunc_Toggle_Crouch) || p->crouch_toggle)
		{
			loc.actions |= SB_CROUCH;
		}
		if ((isRR() && p->drink_amt > 88)) loc.actions |= SB_LOOK_LEFT;
		if ((isRR() && p->drink_amt > 99)) loc.actions |= SB_LOOK_DOWN;
	}
}

//---------------------------------------------------------------------------
//
// split off so that it can later be integrated into the other games more easily.
//
//---------------------------------------------------------------------------

static void checkCrouchToggle(player_struct* p)
{
	int const sectorLotag = p->cursectnum != -1 ? sector[p->cursectnum].lotag : 0;
	int const crouchable = sectorLotag != ST_2_UNDERWATER && (sectorLotag != ST_1_ABOVE_WATER || p->spritebridge);

	if (buttonMap.ButtonDown(gamefunc_Toggle_Crouch))
	{
		p->crouch_toggle = !p->crouch_toggle && crouchable;

		if (crouchable)
			buttonMap.ClearButton(gamefunc_Toggle_Crouch);
	}

	if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Jump) || p->jetpack_on || (!crouchable && p->on_ground))
		p->crouch_toggle = 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int getticssincelastupdate()
{
	int  tics = lastcontroltime == 0 || ud.levelclock < lastcontroltime ? 0 : ud.levelclock - lastcontroltime;
	lastcontroltime = ud.levelclock;
	return tics;
}

//---------------------------------------------------------------------------
//
// split out for readability
//
//---------------------------------------------------------------------------

static double motoApplyTurn(player_struct* p, int turnl, int turnr, int bike_turn, bool goback, double factor)
{
	int turnvel = 0;
	p->oTiltStatus = p->TiltStatus;

	if (p->MotoSpeed == 0 || !p->on_ground)
	{
		turnheldtime = 0;
		lastcontroltime = 0;
		if (turnl)
		{
			p->TiltStatus -= (float)factor;
			if (p->TiltStatus < -10)
				p->TiltStatus = -10;
		}
		else if (turnr)
		{
			p->TiltStatus += (float)factor;
			if (p->TiltStatus > 10)
				p->TiltStatus = 10;
		}
	}
	else
	{
		int tics = getticssincelastupdate();
		if (turnl || turnr || p->moto_drink != 0)
		{
			if (turnl || p->moto_drink < 0)
			{
				turnheldtime += tics;
				p->TiltStatus -= (float)factor;
				if (p->TiltStatus < -10)
					p->TiltStatus = -10;
				if (turnheldtime >= TURBOTURNTIME && p->MotoSpeed > 0)
				{
					if (goback) turnvel += bike_turn ? 40 : 20;
					else turnvel += bike_turn ? -40 : -20;
				}
				else
				{
					if (goback) turnvel += bike_turn ? 20 : 6;
					else turnvel += bike_turn ? -20 : -6;
				}
			}

			if (turnr || p->moto_drink > 0)
			{
				turnheldtime += tics;
				p->TiltStatus += (float)factor;
				if (p->TiltStatus > 10)
					p->TiltStatus = 10;
				if (turnheldtime >= TURBOTURNTIME && p->MotoSpeed > 0)
				{
					if (goback) turnvel += bike_turn ? -40 : -20;
					else turnvel += bike_turn ? 40 : 20;
				}
				else
				{
					if (goback) turnvel += bike_turn ? -20 : -6;
					else turnvel += bike_turn ? 20 : 6;
				}
			}
		}
		else
		{
			turnheldtime = 0;
			lastcontroltime = 0;

			if (p->TiltStatus > 0)
				p->TiltStatus -= (float)factor;
			else if (p->TiltStatus < 0)
				p->TiltStatus += (float)factor;
		}
	}

	if (fabs(p->TiltStatus) < factor)
		p->TiltStatus = 0;

	return turnvel * factor;
}

//---------------------------------------------------------------------------
//
// same for the boat
//
//---------------------------------------------------------------------------

static double boatApplyTurn(player_struct *p, int turnl, int turnr, int boat_turn, double factor)
{
	int turnvel = 0;
	int tics = getticssincelastupdate();

	if (p->MotoSpeed)
	{
		if (turnl || turnr || p->moto_drink != 0)
		{
			if (turnl || p->moto_drink < 0)
			{
				turnheldtime += tics;
				if (!p->NotOnWater)
				{
					p->TiltStatus -= (float)factor;
					if (p->TiltStatus < -10)
						p->TiltStatus = -10;
				}
				if (turnheldtime >= TURBOTURNTIME)
				{
					if (p->NotOnWater) turnvel += boat_turn ? -12 : -6;
					else turnvel += boat_turn ? -40 : -20;
				}
				else
				{
					if (p->NotOnWater) turnvel += boat_turn ? -4 : -2;
					else turnvel += boat_turn ? -12 : -6;
				}
			}

			if (turnr || p->moto_drink > 0)
			{
				turnheldtime += tics;
				if (!p->NotOnWater)
				{
					p->TiltStatus += (float)factor;
					if (p->TiltStatus > 10)
						p->TiltStatus = 10;
				}
				if (turnheldtime >= TURBOTURNTIME)
				{
					if (p->NotOnWater) turnvel += boat_turn ? 12 : 6;
					else turnvel += boat_turn ? 40 : 20;
				}
				else
				{
					if (p->NotOnWater) turnvel += boat_turn ? 4 : 2;
					else turnvel += boat_turn ? 12 : 6;
				}
			}
		}
		else if (!p->NotOnWater)
		{
			turnheldtime = 0;
			lastcontroltime = 0;

			if (p->TiltStatus > 0)
				p->TiltStatus -= (float)factor;
			else if (p->TiltStatus < 0)
				p->TiltStatus += (float)factor;
		}
	}
	else if (!p->NotOnWater)
	{
		turnheldtime = 0;
		lastcontroltime = 0;

		if (p->TiltStatus > 0)
			p->TiltStatus -= (float)factor;
		else if (p->TiltStatus < 0)
			p->TiltStatus += (float)factor;
	}

	if (fabs(p->TiltStatus) < factor)
		p->TiltStatus = 0;

	return turnvel * factor;
}

//---------------------------------------------------------------------------
//
// much of this was rewritten from scratch to make the logic easier to follow.
//
//---------------------------------------------------------------------------

static void processVehicleInput(player_struct *p, ControlInfo* const hidInput, InputPacket& input, double scaleAdjust)
{
	auto turnspeed = hidInput->mouseturnx + scaleAdjust * hidInput->dyaw * (1. / 32); // originally this was 64, not 32. Why the change?
	int turnl = buttonMap.ButtonDown(gamefunc_Turn_Left) || buttonMap.ButtonDown(gamefunc_Strafe_Left);
	int turnr = buttonMap.ButtonDown(gamefunc_Turn_Right) || buttonMap.ButtonDown(gamefunc_Strafe_Right);

	// Cancel out micro-movement
	const double turn_threshold = 1 / 65536.;
	if (turnspeed < -turn_threshold)
		turnl = 1;
	else if (turnspeed > turn_threshold)
		turnr = 1;
	else
		turnspeed = 0;

	if (p->OnBoat || !p->moto_underwater)
	{
		if (buttonMap.ButtonDown(gamefunc_Move_Forward) || buttonMap.ButtonDown(gamefunc_Strafe))
			loc.actions |= SB_JUMP;
		if (buttonMap.ButtonDown(gamefunc_Move_Backward))
			p->vehicle_backwards = true;
		if (loc.actions & SB_RUN)
			loc.actions |= SB_CROUCH;
	}

	if (turnl) p->vehicle_turnl = true;
	if (turnr) p->vehicle_turnr = true;

	double turnvel;

	if (p->OnMotorcycle)
	{
		bool backward = buttonMap.ButtonDown(gamefunc_Move_Backward) && p->MotoSpeed <= 0;

		turnvel = motoApplyTurn(p, turnl, turnr, turnspeed, backward, scaleAdjust);
		if (p->moto_underwater) p->MotoSpeed = 0;
	}
	else
	{
		turnvel = boatApplyTurn(p, turnl, turnr, turnspeed != 0, scaleAdjust);
	}

	// What is this? Optimization for playing with a mouse which the original did not have?
	if (turnspeed)
		turnvel *= clamp(turnspeed * turnspeed, 0., 1.);

	input.fvel = p->MotoSpeed;
	input.avel = turnvel * (45. / 256.);
	loc.avel = clamp(loc.avel + input.avel, -MAXANGVEL, MAXANGVEL);
}

//---------------------------------------------------------------------------
//
// finalizes the input and passes it to the global input buffer
//
//---------------------------------------------------------------------------

static void FinalizeInput(int playerNum, InputPacket& input, bool vehicle)
{
	auto p = &ps[playerNum];
	bool blocked = movementBlocked(playerNum) || p->GetActor()->s.extra <= 0 || (p->dead_flag && !ud.god);

	if (blocked && ps[playerNum].newowner < 0)
	{
		// neutralize all movement when blocked or in automap follow mode
		loc.fvel = loc.svel = 0;
		loc.avel = loc.horz = 0;
		input.avel = input.horz = 0;
	}
	else
	{
		if (p->on_crane == nullptr)
		{
			if (!vehicle)
			{
				loc.fvel = clamp(loc.fvel + input.fvel, -MAXVEL, MAXVEL);
				loc.svel = clamp(loc.svel + input.svel, -MAXSVEL, MAXSVEL);
			}
			else
				loc.fvel = clamp(input.fvel, -(MAXVELMOTO >> 3), MAXVELMOTO);
		}
		else
		{
			loc.fvel = input.fvel = 0;
			loc.svel = input.svel = 0;
		}

		if (p->on_crane == nullptr && p->newowner == -1)
		{
			// input.avel already added to loc in processMovement()
			loc.avel = clamp(loc.avel, -MAXANGVEL, MAXANGVEL);
			if (!cl_syncinput && input.avel)
			{
				p->angle.spin = bamlook(0);
			}
		}
		else
		{
			loc.avel = input.avel = 0;
		}

		if (p->newowner == -1 && !(p->sync.actions & SB_CENTERVIEW))
		{
			// input.horz already added to loc in processMovement()
			loc.horz = clamp(loc.horz, -MAXHORIZVEL, MAXHORIZVEL);
		}
		else
		{
			loc.horz = input.horz = 0;
		}
	}
}


//---------------------------------------------------------------------------
//
// External entry point
//
//---------------------------------------------------------------------------

void GameInterface::GetInput(InputPacket* packet, ControlInfo* const hidInput)
{
	if (paused)
	{
		loc = {};
		return;
	}

	auto const p = &ps[myconnectindex];

	if (numplayers == 1)
	{
		setlocalplayerinput(p);
	}

	double const scaleAdjust = InputScale();
	InputPacket input{};

	if (isRRRA() && (p->OnMotorcycle || p->OnBoat))
	{
		p->crouch_toggle = 0;
		processInputBits(p, hidInput);
		processVehicleInput(p, hidInput, input, scaleAdjust);
		FinalizeInput(myconnectindex, input, true);

		if (!cl_syncinput && p->GetActor()->s.extra > 0)
		{
			apply_seasick(p, scaleAdjust);
		}
	}
	else
	{
		processInputBits(p, hidInput);
		processMovement(&input, &loc, hidInput, scaleAdjust, p->drink_amt);
		checkCrouchToggle(p);
		FinalizeInput(myconnectindex, input, false);
	}

	if (!cl_syncinput)
	{
		if (p->GetActor()->s.extra > 0)
		{
			// Do these in the same order as the old code.
			calcviewpitch(p, scaleAdjust);
			processavel(p, &input.avel);
			applylook(&p->angle, input.avel, &p->sync.actions, scaleAdjust, p->crouch_toggle || p->sync.actions & SB_CROUCH);
			sethorizon(&p->horizon.horiz, input.horz, &p->sync.actions, scaleAdjust);
		}

		p->angle.processhelpers(scaleAdjust);
		p->horizon.processhelpers(scaleAdjust);
	}

	if (packet)
	{
		auto const pPlayer = &ps[myconnectindex];
		auto const ang = pPlayer->angle.ang.asbuild();

		*packet = loc;
		auto fvel = loc.fvel;
		auto svel = loc.svel;
		packet->fvel = mulscale9(fvel, sintable[(ang + 2560) & 2047]) +
			mulscale9(svel, sintable[(ang + 2048) & 2047]) +
			pPlayer->fric.x;
		packet->svel = mulscale9(fvel, sintable[(ang + 2048) & 2047]) +
			mulscale9(svel, sintable[(ang + 1536) & 2047]) +
			pPlayer->fric.y;
		loc = {};
	}
}

//---------------------------------------------------------------------------
//
// This is called from InputState::ClearAllInput and resets all static state being used here.
//
//---------------------------------------------------------------------------

void GameInterface::clearlocalinputstate()
{
	loc = {};
	turnheldtime = 0;
	lastcontroltime = 0;
}


END_DUKE_NS
