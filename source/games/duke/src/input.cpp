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

BEGIN_DUKE_NS

static int WeaponToSend;
static ESyncBits BitsToSend;

// State timer counters. 
static int nonsharedtimer;
static int turnheldtime;
static int lastcontroltime;
static double lastCheck;

//---------------------------------------------------------------------------
//
// handles UI side input not handled via CCMDs or CVARs.
// Most of what's in here needs to be offloaded to CCMDs
//
//---------------------------------------------------------------------------

void nonsharedkeys(void)
{
	if (ud.recstat == 2)
	{
		ControlInfo noshareinfo;
		CONTROL_GetInput(&noshareinfo);
	}

	if (System_WantGuiCapture())
		return;

	if (!ALT_IS_PRESSED && ud.overhead_on == 0)
	{
		if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
		{
			buttonMap.ClearButton(gamefunc_Enlarge_Screen);

			if (!SHIFTS_IS_PRESSED)
			{
				if (G_ChangeHudLayout(1))
				{
					gi->PlayHudSound();
				}
			}
			else
			{
				hud_scale = hud_scale + 4;
			}
		}

		if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
		{
			buttonMap.ClearButton(gamefunc_Shrink_Screen);

			if (!SHIFTS_IS_PRESSED)
			{
				if (G_ChangeHudLayout(-1))
				{
					gi->PlayHudSound();
				}
			}
			else
			{
				hud_scale = hud_scale - 4;
			}
		}
	}

	if (buttonMap.ButtonDown(gamefunc_See_Coop_View) && (ud.coop || ud.recstat == 2))
	{
		buttonMap.ClearButton(gamefunc_See_Coop_View);
		screenpeek = connectpoint2[screenpeek];
		if (screenpeek == -1) screenpeek = 0;
	}

	if ((ud.multimode > 1) && buttonMap.ButtonDown(gamefunc_Show_Opponents_Weapon))
	{
		buttonMap.ClearButton(gamefunc_Show_Opponents_Weapon);
		ud.showweapons = 1 - ud.showweapons;
		cl_showweapon = ud.showweapons;
		FTA(QUOTE_WEAPON_MODE_OFF - ud.showweapons, &ps[screenpeek]);
	}

	if (buttonMap.ButtonDown(gamefunc_Toggle_Crosshair))
	{
		buttonMap.ClearButton(gamefunc_Toggle_Crosshair);
		cl_crosshair = !cl_crosshair;
		FTA(QUOTE_CROSSHAIR_OFF - cl_crosshair, &ps[screenpeek]);
	}

	if (ud.overhead_on && buttonMap.ButtonDown(gamefunc_Map_Follow_Mode))
	{
		buttonMap.ClearButton(gamefunc_Map_Follow_Mode);
		ud.scrollmode = 1 - ud.scrollmode;
		if (ud.scrollmode)
		{
			ud.folx = ps[screenpeek].oposx;
			ud.foly = ps[screenpeek].oposy;
			ud.fola = ps[screenpeek].getoang();
		}
		FTA(QUOTE_MAP_FOLLOW_OFF + ud.scrollmode, &ps[myconnectindex]);
	}

	// Fixme: This really should be done via CCMD, not via hard coded key checks - but that needs alternative Shift and Alt bindings.
	if (SHIFTS_IS_PRESSED || ALT_IS_PRESSED)
	{
		int taunt = 0;

		// NOTE: sc_F1 .. sc_F10 are contiguous. sc_F11 is not sc_F10+1.
		for (int j = sc_F1; j <= sc_F10; j++)
			if (inputState.UnboundKeyPressed(j))
			{
				inputState.ClearKeyStatus(j);
				taunt = j - sc_F1 + 1;
				break;
			}

		if (taunt)
		{
			if (SHIFTS_IS_PRESSED)
			{
				Printf(PRINT_NOTIFY, "%s", **CombatMacros[taunt - 1]);
				//Net_SendTaunt(taunt);
				return;
			}

			if (startrts(taunt, 1))
			{
				//Net_SendRTS(taunt);
				return;
			}
		}
	}

	if (!ALT_IS_PRESSED && !SHIFTS_IS_PRESSED)
	{
		if (buttonMap.ButtonDown(gamefunc_Third_Person_View))
		{
			buttonMap.ClearButton(gamefunc_Third_Person_View);

			if (!isRRRA() || (!ps[myconnectindex].OnMotorcycle && !ps[myconnectindex].OnBoat))
			{
				if (ps[myconnectindex].over_shoulder_on)
					ps[myconnectindex].over_shoulder_on = 0;
				else
				{
					ps[myconnectindex].over_shoulder_on = 1;
					cameradist = 0;
					cameraclock = (int)totalclock;
				}
				FTA(QUOTE_VIEW_MODE_OFF + ps[myconnectindex].over_shoulder_on, &ps[myconnectindex]);
			}
		}

		if (ud.overhead_on != 0)
		{
			int j;
			if (nonsharedtimer > 0 || totalclock < nonsharedtimer)
			{
				j = (int)totalclock - nonsharedtimer;
				nonsharedtimer += j;
			}
			else
			{
				j = 0;
				nonsharedtimer = (int)totalclock;
			}

			if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
				ps[myconnectindex].zoom += mulscale6(j, max(ps[myconnectindex].zoom, 256));
			if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
				ps[myconnectindex].zoom -= mulscale6(j, max(ps[myconnectindex].zoom, 256));

			ps[myconnectindex].zoom = clamp(ps[myconnectindex].zoom, 48, 2048);
		}
	}

	if (buttonMap.ButtonDown(gamefunc_Map))
	{
		buttonMap.ClearButton(gamefunc_Map);
		if (ud.last_overhead != ud.overhead_on && ud.last_overhead)
		{
			ud.overhead_on = ud.last_overhead;
			ud.last_overhead = 0;
		}
		else
		{
			ud.overhead_on++;
			if (ud.overhead_on == 3) ud.overhead_on = 0;
			ud.last_overhead = ud.overhead_on;
		}
	}
}

//---------------------------------------------------------------------------
//
// handles all HUD related input, i.e. inventory item selection and activation plus weapon selection.
//
// Note: This doesn't restrict the events to WW2GI - since the other games do 
// not define any by default there is no harm done keeping the code clean.
//
//---------------------------------------------------------------------------

void hud_input(int snum)
{
	int i, k;
	uint8_t dainv;
	unsigned int j;
	struct player_struct* p;
	short unk;

	unk = 0;
	p = &ps[snum];

	i = p->aim_mode;
	p->aim_mode = PlayerInput(snum, SKB_AIMMODE);
	if (p->aim_mode < i)
		p->return_to_center = 9;

	// Backup weapon here as hud_input() is the first function where any one of the weapon variables can change.
	backupweapon(p);

	if (isRR())
	{
		if (PlayerInput(snum, SKB_QUICK_KICK) && p->last_pissed_time == 0)
		{
			if (!isRRRA() || sprite[p->i].extra > 0)
			{
				p->last_pissed_time = 4000;
				S_PlayActorSound(437, p->i);
				if (sprite[p->i].extra <= max_player_health - max_player_health / 10)
				{
					sprite[p->i].extra += 2;
					p->last_extra = sprite[p->i].extra;
				}
				else if (sprite[p->i].extra < max_player_health)
					sprite[p->i].extra = max_player_health;
			}
		}
	}
	else
	{
		if (PlayerInput(snum, SKB_QUICK_KICK) && p->quick_kick == 0 && (p->curr_weapon != KNEE_WEAPON || p->kickback_pic == 0))
		{
			SetGameVarID(g_iReturnVarID, 0, -1, snum);
			OnEvent(EVENT_QUICKKICK, -1, snum, -1);
			if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
			{
				p->quick_kick = 14;
				if (!p->quick_kick_msg && snum == screenpeek) FTA(QUOTE_MIGHTY_FOOT, p);
				p->quick_kick_msg = true;
			}
		}
	}
	if (!PlayerInput(snum, SKB_QUICK_KICK)) p->quick_kick_msg = false;

	if (!PlayerInputBits(snum, SKB_INTERFACE_BITS))
		p->interface_toggle_flag = 0;
	else if (p->interface_toggle_flag == 0)
	{
		p->interface_toggle_flag = 1;

		// Don't go on if paused or dead.
		if (paused) return;
		if (sprite[p->i].extra <= 0) return;

		// Activate an inventory item. This just forwards to the other inventory bits. If the inventory selector was taken out of the playsim this could be removed.
		if (PlayerInput(snum, SKB_INVENTORY) && p->newowner == -1)
		{
			SetGameVarID(g_iReturnVarID, 0, -1, snum);
			OnEvent(EVENT_INVENTORY, -1, snum, -1);
			if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
			{
				switch (p->inven_icon)
				{
					// Yet another place where no symbolic constants were used. :(
				case ICON_JETPACK: PlayerSetInput(snum, SKB_JETPACK); break;
				case ICON_HOLODUKE: PlayerSetInput(snum, SKB_HOLODUKE); break;
				case ICON_HEATS: PlayerSetInput(snum, SKB_NIGHTVISION); break;
				case ICON_FIRSTAID: PlayerSetInput(snum, SKB_MEDKIT); break;
				case ICON_STEROIDS: PlayerSetInput(snum, SKB_STEROIDS); break;
				}
			}
		}

		if (!isRR() && PlayerInput(snum, SKB_NIGHTVISION))
		{
			SetGameVarID(g_iReturnVarID, 0, -1, snum);
			OnEvent(EVENT_USENIGHTVISION, -1, snum, -1);
			if (GetGameVarID(g_iReturnVarID, -1, snum) == 0 && p->heat_amount > 0)
			{
				p->heat_on = !p->heat_on;
				setpal(p);
				p->inven_icon = 5;
				S_PlayActorSound(NITEVISION_ONOFF, p->i);
				FTA(106 + (!p->heat_on), p);
			}
		}

		if (PlayerInput(snum, SKB_STEROIDS))
		{
			SetGameVarID(g_iReturnVarID, 0, -1, snum);
			OnEvent(EVENT_USESTEROIDS, -1, snum, -1);
			if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
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

		if (PlayerInput(snum, SKB_INV_LEFT) || PlayerInput(snum, SKB_INV_RIGHT))
		{
			p->invdisptime = 26 * 2;

			if (PlayerInput(snum, SKB_INV_RIGHT)) k = 1;
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
			if (PlayerInput(snum, SKB_INV_LEFT))
			{
				SetGameVarID(g_iReturnVarID, dainv, -1, snum);
				OnEvent(EVENT_INVENTORYLEFT, -1, snum, -1);
				dainv = GetGameVarID(g_iReturnVarID, -1, snum);
			}
			if (PlayerInput(snum, SKB_INV_RIGHT))
			{
				SetGameVarID(g_iReturnVarID, dainv, -1, snum);
				OnEvent(EVENT_INVENTORYRIGHT, -1, snum, -1);
				dainv = GetGameVarID(g_iReturnVarID, -1, snum);
			}
			p->inven_icon = dainv;
			// Someone must have really hated constant data, doing this with a switch/case (and of course also with literal numbers...)
			static const uint8_t invquotes[] = { QUOTE_MEDKIT, QUOTE_STEROIDS, QUOTE_HOLODUKE, QUOTE_JETPACK, QUOTE_NVG, QUOTE_SCUBA, QUOTE_BOOTS };
			if (dainv >= 1 && dainv < 8) FTA(invquotes[dainv - 1], p);
		}

		j = (PlayerInputBits(snum, SKB_WEAPONMASK_BITS) / SKB_FIRST_WEAPON_BIT) - 1;
		if (j >= 0)
		{
			int a = 0;
		}
		if (j > 0 && p->kickback_pic > 0)
			p->wantweaponfire = j;

		// Here we have to be extra careful that the weapons do not get mixed up, so let's keep the code for Duke and RR completely separate.
		fi.selectweapon(snum, j);

		if (PlayerInput(snum, SKB_HOLSTER))
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

		if (PlayerInput(snum, SKB_HOLODUKE) && (isRR() || p->newowner == -1))
		{
			SetGameVarID(g_iReturnVarID, 0, -1, snum);
			OnEvent(EVENT_HOLODUKEON, -1, snum, -1);
			if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
			{
				if (!isRR())
				{
					if (p->holoduke_on == -1)
					{
						if (p->holoduke_amount > 0)
						{
							p->inven_icon = 3;

							p->holoduke_on = i =
								EGS(p->cursectnum,
									p->posx,
									p->posy,
									p->posz + (30 << 8), TILE_APLAYER, -64, 0, 0, p->getang(), 0, 0, -1, 10);
							hittype[i].temp_data[3] = hittype[i].temp_data[4] = 0;
							sprite[i].yvel = snum;
							sprite[i].extra = 0;
							FTA(47, p);
						}
						else FTA(QUOTE_HOLODUKE_ON, p);
						S_PlayActorSound(TELEPORTER, p->holoduke_on);

					}
					else
					{
						S_PlayActorSound(TELEPORTER, p->holoduke_on);
						p->holoduke_on = -1;
						FTA(QUOTE_HOLODUKE_NOT_FOUND, p);
					}
				}
				else // In RR this means drinking whiskey.
				{
					if (p->holoduke_amount > 0 && sprite[p->i].extra < max_player_health)
					{
						p->holoduke_amount -= 400;
						sprite[p->i].extra += 5;
						if (sprite[p->i].extra > max_player_health)
							sprite[p->i].extra = max_player_health;

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

		if (isRR() && PlayerInput(snum, SKB_NIGHTVISION) && p->newowner == -1)
		{
			SetGameVarID(g_iReturnVarID, 0, -1, snum);
			OnEvent(EVENT_USENIGHTVISION, -1, snum, -1);
			if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
			{
				if (p->yehaa_timer == 0)
				{
					p->yehaa_timer = 126;
					S_PlayActorSound(390, p->i);
					p->noise_radius = 16384;
					madenoise(snum);
					if (sector[p->cursectnum].lotag == 857)
					{
						if (sprite[p->i].extra <= max_player_health)
						{
							sprite[p->i].extra += 10;
							if (sprite[p->i].extra >= max_player_health)
								sprite[p->i].extra = max_player_health;
						}
					}
					else
					{
						if (sprite[p->i].extra + 1 <= max_player_health)
						{
							sprite[p->i].extra++;
						}
					}
				}
			}
		}

		if (PlayerInput(snum, SKB_MEDKIT))
		{
			SetGameVarID(g_iReturnVarID, 0, -1, snum);
			OnEvent(EVENT_USEMEDKIT, -1, snum, -1);
			if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
			{
				if (p->firstaid_amount > 0 && sprite[p->i].extra < max_player_health)
				{
					if (!isRR())
					{
						j = max_player_health - sprite[p->i].extra;

						if ((unsigned int)p->firstaid_amount > j)
						{
							p->firstaid_amount -= j;
							sprite[p->i].extra = max_player_health;
							p->inven_icon = 1;
						}
						else
						{
							sprite[p->i].extra += p->firstaid_amount;
							p->firstaid_amount = 0;
							checkavailinven(p);
						}
						S_PlayActorSound(DUKE_USEMEDKIT, p->i);
					}
					else
					{
						j = 10;
						if (p->firstaid_amount > j)
						{
							p->firstaid_amount -= j;
							sprite[p->i].extra += j;
							if (sprite[p->i].extra > max_player_health)
								sprite[p->i].extra = max_player_health;
							p->inven_icon = 1;
						}
						else
						{
							sprite[p->i].extra += p->firstaid_amount;
							p->firstaid_amount = 0;
							checkavailinven(p);
						}
						if (sprite[p->i].extra > max_player_health)
							sprite[p->i].extra = max_player_health;
						p->drink_amt += 10;
						if (p->drink_amt <= 100 && !S_CheckActorSoundPlaying(p->i, DUKE_USEMEDKIT))
							S_PlayActorSound(DUKE_USEMEDKIT, p->i);
					}
				}
			}
		}

		if (PlayerInput(snum, SKB_JETPACK) && (isRR() || p->newowner == -1))
		{
			SetGameVarID(g_iReturnVarID, 0, -1, snum);
			OnEvent(EVENT_USEJETPACK, -1, snum, -1);
			if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
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
					if (p->jetpack_amount > 0 && sprite[p->i].extra < max_player_health)
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

						sprite[p->i].extra += 5;

						p->inven_icon = 4;

						if (sprite[p->i].extra > max_player_health)
							sprite[p->i].extra = max_player_health;

						if (p->jetpack_amount <= 0)
							checkavailinven(p);
					}
				}
			}
		}

		if (PlayerInput(snum, SKB_TURNAROUND) && p->one_eighty_count == 0)
		{
			SetGameVarID(g_iReturnVarID, 0, -1, snum);
			OnEvent(EVENT_TURNAROUND, -1, snum, -1);
			if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
			{
				p->one_eighty_count = -F16(1024);
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
	TURBOTURNTIME =  (TICRATE/8), // 7
	NORMALTURN    = 15,
	PREAMBLETURN  = 5,
	NORMALKEYMOVE = 40,
	MAXVEL        = ((NORMALKEYMOVE*2)+10),
	MAXSVEL       = ((NORMALKEYMOVE*2)+10),
	MAXANGVEL     = 1024,
	MAXHORIZVEL   = 256,
	ONEEIGHTYSCALE = 4,

	MOTOTURN      = 20,
	MAXVELMOTO    = 120,
};

//---------------------------------------------------------------------------
//
// handles the input bits
//
//---------------------------------------------------------------------------

static void processInputBits(player_struct *p, ControlInfo &info)
{
	bool onVehicle = p->OnMotorcycle || p->OnBoat;
	if (buttonMap.ButtonDown(gamefunc_Fire)) loc.bits |= SKB_FIRE;
	if (buttonMap.ButtonDown(gamefunc_Open)) loc.bits |= SKB_OPEN;

	// These 3 bits are only available when not riding a bike or boat.
	if (onVehicle) BitsToSend &= ~(SKB_HOLSTER|SKB_TURNAROUND|SKB_CENTER_VIEW);
	loc.bits |= BitsToSend;
	BitsToSend = 0;

	if (buttonMap.ButtonDown(gamefunc_Dpad_Select))
	{
		if (info.dx < 0 || info.dyaw < 0) loc.bits |= SKB_INV_LEFT;
		if (info.dx > 0 || info.dyaw < 0) loc.bits |= SKB_INV_RIGHT;
	}

	if (gamequit) loc.bits |= SKB_GAMEQUIT;

	if (!onVehicle)
	{
		if (buttonMap.ButtonDown(gamefunc_Jump)) loc.bits |= SKB_JUMP;
		if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Toggle_Crouch) || p->crouch_toggle)
		{
			loc.bits |= SKB_CROUCH;
			if (isRR()) loc.bits &= ~SKB_JUMP;
		}
		if (buttonMap.ButtonDown(gamefunc_Aim_Up) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && info.dz > 0)) loc.bits |= SKB_AIM_UP;
		if ((buttonMap.ButtonDown(gamefunc_Aim_Down) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && info.dz < 0))) loc.bits |= SKB_AIM_DOWN;
		if (G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run))) loc.bits |= SKB_RUN;
		if (buttonMap.ButtonDown(gamefunc_Look_Left) || (isRR() && p->drink_amt > 88)) loc.bits |= SKB_LOOK_LEFT;
		if (buttonMap.ButtonDown(gamefunc_Look_Right)) loc.bits |= SKB_LOOK_RIGHT;
		if (buttonMap.ButtonDown(gamefunc_Look_Up)) loc.bits |= SKB_LOOK_UP;
		if (buttonMap.ButtonDown(gamefunc_Look_Down) || (isRR() && p->drink_amt > 99)) loc.bits |= SKB_LOOK_DOWN;
		if (buttonMap.ButtonDown(gamefunc_Quick_Kick)) loc.bits |= SKB_QUICK_KICK;
		if (in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming)) loc.bits |= SKB_AIMMODE;

		int j = WeaponToSend;
		WeaponToSend = 0;
		if (VOLUMEONE && (j >= 7 && j <= 10)) j = 0;

		if (buttonMap.ButtonDown(gamefunc_Dpad_Select) && info.dz > 0) j = 11;
		if (buttonMap.ButtonDown(gamefunc_Dpad_Select) && info.dz < 0) j = 12;

		if (j && (loc.bits & SKB_WEAPONMASK_BITS) == 0)
			loc.bits |= ESyncBits::FromInt(j * SKB_FIRST_WEAPON_BIT);

	}

	if (buttonMap.ButtonDown(gamefunc_Dpad_Select))
	{
		// This eats the controller input for regular use
		info.dx = 0;
		info.dz = 0;
		info.dyaw = 0;
	}

	if (buttonMap.ButtonDown(gamefunc_Dpad_Aiming))
		info.dz = 0;
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
	int  tics = lastcontroltime == 0 || (int)totalclock < lastcontroltime ? 0 : (int)totalclock - lastcontroltime;
	lastcontroltime = (int)totalclock;
	return tics;
}

//---------------------------------------------------------------------------
//
// handles movement
//
//---------------------------------------------------------------------------

static void processMovement(player_struct *p, input_t &input, ControlInfo &info, double scaleFactor)
{
	bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);

	// JBF: Run key behaviour is selectable
	int running = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
	int turnamount = NORMALTURN << running;
	int keymove = NORMALKEYMOVE << running;

	if (buttonMap.ButtonDown(gamefunc_Strafe))
		input.svel -= info.mousex * 4.f + scaleFactor * info.dyaw * keymove;
	else
		input.q16avel += fix16_from_float(info.mousex + scaleFactor * info.dyaw);

	if (mouseaim)
		input.q16horz += fix16_from_float(info.mousey);
	else
		input.fvel -= info.mousey * 8.f;

	if (!in_mouseflip) input.q16horz = -input.q16horz;

	input.q16horz -= fix16_from_dbl(scaleFactor * (info.dpitch));
	input.svel -= scaleFactor * (info.dx * keymove);
	input.fvel -= scaleFactor * (info.dz * keymove);

	if (buttonMap.ButtonDown(gamefunc_Strafe))
	{
		if (!loc.svel)
		{
			if (buttonMap.ButtonDown(gamefunc_Turn_Left))
				input.svel = keymove;

			if (buttonMap.ButtonDown(gamefunc_Turn_Right))
				input.svel = -keymove;
		}
	}
	else
	{
		int tics = getticssincelastupdate();

		if (buttonMap.ButtonDown(gamefunc_Turn_Left))
		{
			turnheldtime += tics;
			input.q16avel -= fix16_from_dbl(2 * scaleFactor * (turnheldtime >= TURBOTURNTIME ? turnamount : PREAMBLETURN));
		}
		else if (buttonMap.ButtonDown(gamefunc_Turn_Right))
		{
			turnheldtime += tics;
			input.q16avel += fix16_from_dbl(2 * scaleFactor * (turnheldtime >= TURBOTURNTIME ? turnamount : PREAMBLETURN));
		}
		else
		{
			turnheldtime = 0;
			lastcontroltime = 0;
		}

	}

	if (abs(loc.svel) < keymove)
	{
		if (buttonMap.ButtonDown(gamefunc_Strafe_Left))
			input.svel += keymove;

		if (buttonMap.ButtonDown(gamefunc_Strafe_Right))
			input.svel += -keymove;
	}

	if (abs(loc.fvel) < keymove)
	{
		if (isRR() && p->drink_amt >= 66 && p->drink_amt <= 87)
		{
			if (buttonMap.ButtonDown(gamefunc_Move_Forward))
			{
				input.fvel += keymove;
				if (p->drink_amt & 1)
					input.svel += keymove;
				else
					input.svel -= keymove;
			}

			if (buttonMap.ButtonDown(gamefunc_Move_Backward))
			{
				input.fvel += -keymove;
				if (p->drink_amt & 1)
					input.svel -= keymove;
				else
					input.svel += keymove;
			}
		}
		else
		{
			if (buttonMap.ButtonDown(gamefunc_Move_Forward))
				input.fvel += keymove;

			if (buttonMap.ButtonDown(gamefunc_Move_Backward))
				input.fvel += -keymove;
		}
	}
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

static void processVehicleInput(player_struct *p, ControlInfo& info, input_t& input, double scaleAdjust)
{
	auto turnspeed = info.mousex + scaleAdjust * info.dyaw * (1. / 32); // originally this was 64, not 32. Why the change?
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
			loc.bits |= SKB_JUMP;
		if (buttonMap.ButtonDown(gamefunc_Move_Backward))
			loc.bits |= SKB_AIM_UP;
		if (buttonMap.ButtonDown(gamefunc_Run))
			loc.bits |= SKB_CROUCH;
	}

	if (turnl)
		loc.bits |= SKB_AIM_DOWN;
	if (turnr)
		loc.bits |= SKB_LOOK_LEFT;

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
	input.q16avel = fix16_from_dbl(turnvel);
}

//---------------------------------------------------------------------------
//
// finalizes the input and passes it to the global input buffer
//
//---------------------------------------------------------------------------

static void FinalizeInput(int playerNum, input_t& input, bool vehicle)
{
	auto p = &ps[playerNum];
	bool blocked = movementBlocked(playerNum) || sprite[p->i].extra <= 0 || (p->dead_flag && !ud.god);

	if ((ud.scrollmode && ud.overhead_on) || blocked)
	{
		if (ud.scrollmode && ud.overhead_on)
		{
			ud.folfvel = input.fvel;
			ud.folavel = fix16_to_int(input.q16avel);
		}

		loc.fvel = loc.svel = 0;
		loc.q16avel = loc.q16horz = 0;
		input.q16avel = input.q16horz = 0;
	}
	else
	{
		if (p->on_crane < 0)
		{
			if (!vehicle)
			{
				loc.fvel = clamp(loc.fvel + input.fvel, -MAXVEL, MAXVEL);
				loc.svel = clamp(loc.svel + input.svel, -MAXSVEL, MAXSVEL);
			}
			else
				loc.fvel = clamp(input.fvel, -(MAXVELMOTO / 8), MAXVELMOTO);
		}

		if (p->on_crane < 0 && p->newowner == -1)
		{
			loc.q16avel += input.q16avel;
			if (!cl_syncinput && input.q16avel)
			{
				p->one_eighty_count = 0;
			}
		}

		if (p->newowner == -1 && p->return_to_center <= 0)
		{
			loc.q16horz = fix16_clamp(loc.q16horz + input.q16horz, F16(-MAXHORIZVEL), F16(MAXHORIZVEL));
		}
	}
}

//---------------------------------------------------------------------------
//
// main input handler routine
//
//---------------------------------------------------------------------------

void GetInput()
{
	double elapsedInputTicks;
	auto const p = &ps[myconnectindex];

	auto now = I_msTimeF();
	// do not let this become too large - it would create overflows resulting in undefined behavior. The very first tic must not use the timer difference at all because the timer has not been set yet.
	// This really needs to have the timer fixed to be robust, doing it ad-hoc here is not really safe.
	if (lastCheck > 0) elapsedInputTicks = min(now - lastCheck, 1000.0 / REALGAMETICSPERSEC);
	else elapsedInputTicks = 1;
	lastCheck = now;

	if (paused)
	{
		loc = {};
		if (gamequit) loc.bits |= SKB_GAMEQUIT;
		return;
	}

	if (numplayers == 1)
	{
		setlocalplayerinput(p);
	}

	double scaleAdjust = !cl_syncinput ? elapsedInputTicks * REALGAMETICSPERSEC / 1000.0 : 1;
	ControlInfo info;
	CONTROL_GetInput(&info);
	input_t input{};

	if (isRRRA() && (p->OnMotorcycle || p->OnBoat))
	{
		p->crouch_toggle = 0;
		processInputBits(p, info);
		processVehicleInput(p, info, input, scaleAdjust);
		FinalizeInput(myconnectindex, input, true);

		if (!cl_syncinput && sprite[p->i].extra > 0)
		{
			apply_seasick(p, scaleAdjust);
		}
	}
	else
	{
		processMovement(p, input, info, scaleAdjust);
		checkCrouchToggle(p);
		processInputBits(p, info);
		FinalizeInput(myconnectindex, input, false);
	}

	if (!cl_syncinput)
	{
		// Do these in the same order as the old code.
		calcviewpitch(p, scaleAdjust);
		applylook(myconnectindex, scaleAdjust, input.q16avel);
		sethorizon(myconnectindex, loc.bits, scaleAdjust, true, input.q16horz);
	}
}

//---------------------------------------------------------------------------
//
// CCMD based input. The basics are from Randi's ZDuke but this uses dynamic
// registration to only have the commands active when this game module runs.
//
//---------------------------------------------------------------------------

static int ccmd_slot(CCmdFuncPtr parm)
{
	if (parm->numparms != 1) return CCMD_SHOWHELP;

	auto slot = atoi(parm->parms[0]);
	if (slot >= 1 && slot <= 10)
	{
		WeaponToSend = slot;
		return CCMD_OK;
	}
	return CCMD_SHOWHELP;
}

void registerinputcommands()
{
	C_RegisterFunction("slot", "slot <weaponslot>: select a weapon from the given slot (1-10)", ccmd_slot);
	C_RegisterFunction("weapprev", nullptr, [](CCmdFuncPtr)->int { WeaponToSend = 11; return CCMD_OK; });
	C_RegisterFunction("weapnext", nullptr, [](CCmdFuncPtr)->int { WeaponToSend = 12; return CCMD_OK; });
	C_RegisterFunction("pause", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_PAUSE; return CCMD_OK; });
	C_RegisterFunction("steroids", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_STEROIDS; return CCMD_OK; });
	C_RegisterFunction("nightvision", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_NIGHTVISION; return CCMD_OK; });
	C_RegisterFunction("medkit", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_MEDKIT; return CCMD_OK; });
	C_RegisterFunction("centerview", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_CENTER_VIEW; return CCMD_OK; });
	C_RegisterFunction("holsterweapon", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_HOLSTER; return CCMD_OK; });
	C_RegisterFunction("invprev", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_INV_LEFT; return CCMD_OK; });
	C_RegisterFunction("invnext", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_INV_RIGHT; return CCMD_OK; });
	C_RegisterFunction("holoduke", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_HOLODUKE; return CCMD_OK; });
	C_RegisterFunction("jetpack", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_JETPACK; return CCMD_OK; });
	C_RegisterFunction("turnaround", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_TURNAROUND; return CCMD_OK; });
	C_RegisterFunction("invuse", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_INVENTORY; return CCMD_OK; });
	C_RegisterFunction("backoff", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_ESCAPE; return CCMD_OK; });
}

// This is called from ImputState::ClearAllInput and resets all static state being used here.
void GameInterface::clearlocalinputstate()
{
	WeaponToSend = 0;
	BitsToSend = 0;
	nonsharedtimer = 0;
	turnheldtime = 0;
	lastcontroltime = 0;
	lastCheck = 0;

}

END_DUKE_NS
