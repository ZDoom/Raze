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
#include "game.h"

BEGIN_DUKE_NS

static int WeaponToSend, BitsToSend;

extern double elapsedInputTicks;

//---------------------------------------------------------------------------
//
// handles UI side input not handled via CCMDs or CVARs.
// Most of what's in here needs to be offloaded to CCMDs
//
//---------------------------------------------------------------------------

void nonsharedkeys(void)
{
	static int nonsharedtimer;

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
					S_PlaySound(isRR() ? 341 : THUD, CHAN_AUTO, CHANF_UI);
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
					S_PlaySound(isRR() ? 341 : THUD, CHAN_AUTO, CHANF_UI);
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
				Printf(PRINT_NOTIFY, *CombatMacros[taunt - 1]);
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
			int j = (int)totalclock - nonsharedtimer; 
			nonsharedtimer += j;

			if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
				ps[myconnectindex].zoom += mulscale6(j, max(ps[myconnectindex].zoom, 256));
			if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
				ps[myconnectindex].zoom -= mulscale6(j, max(ps[myconnectindex].zoom, 256));

			ps[myconnectindex].zoom = clamp(ps[myconnectindex].zoom, 48, 2048);
		}
	}

#if 0 // ESC is blocked by the menu, this function is not particularly useful anyway.
	if (inputState.GetKeyStatus(sc_Escape) && ud.overhead_on && ps[myconnectindex].newowner == -1)
	{
		inputState.ClearKeyStatus(sc_Escape);
		ud.last_overhead = ud.overhead_on;
		ud.overhead_on = 0;
		ud.scrollmode = 0;
	}
#endif

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

	if (isRR())
	{
		if (PlayerInput(snum, SKB_QUICK_KICK) && p->last_pissed_time == 0)
		{
			if (!isRRRA() || sprite[p->i].extra > 0)
			{
				p->last_pissed_time = 4000;
				if (!ud.lockout)
					spritesound(437, p->i);
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
				FTA(QUOTE_MIGHTY_FOOT, p);
			}
		}
	}

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
				spritesound(NITEVISION_ONOFF, p->i);
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
					spritesound(DUKE_TAKEPILLS, p->i);
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
						spritesound(TELEPORTER, p->holoduke_on);

					}
					else
					{
						spritesound(TELEPORTER, p->holoduke_on);
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

						if (p->drink_amt < 99 && !A_CheckSoundPlaying(p->i, 425))
							spritesound(425, p->i);
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
					spritesound(390, p->i);
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
						spritesound(DUKE_USEMEDKIT, p->i);
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
						if (p->drink_amt <= 100 && !A_CheckSoundPlaying(p->i, DUKE_USEMEDKIT))
							spritesound(DUKE_USEMEDKIT, p->i);
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

							S_StopEnvSound(-1, p->i, CHAN_VOICE);	// this will stop the falling scream
							A_PlaySound(DUKE_JETPACK_ON, p->i);
							FTA(QUOTE_JETPACK_ON, p);
						}
						else
						{
							p->hard_landing = 0;
							p->poszv = 0;
							spritesound(DUKE_JETPACK_OFF, p->i);
							S_StopEnvSound(DUKE_JETPACK_IDLE, p->i);
							S_StopEnvSound(DUKE_JETPACK_ON, p->i);
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
						if (!A_CheckSoundPlaying(p->i, 429))
							A_PlaySound(429, p->i);

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


enum
{
	TURBOTURNTIME =  (TICRATE/8) // 7
};

//---------------------------------------------------------------------------
//
// This one's from VoidSW, not EDuke32
//
//---------------------------------------------------------------------------

fix16_t GetDeltaQ16Angle(fix16_t ang1, fix16_t ang2)
{
	// Look at the smaller angle if > 1024 (180 degrees)
	if (fix16_abs(ang1 - ang2) > fix16_from_int(1024))
	{
		if (ang1 <= fix16_from_int(1024))
			ang1 += fix16_from_int(2048);

		if (ang2 <= fix16_from_int(1024))
			ang2 += fix16_from_int(2048);
	}

	//if (ang1 - ang2 == -fix16_from_int(1024))
	//    return(fix16_from_int(1024));

	return ang1 - ang2;
}

//---------------------------------------------------------------------------
//
// common handler for all 3 input methods.
//
//---------------------------------------------------------------------------

void processCommonInput(ControlInfo &info, bool onVehicle)
{
	if (buttonMap.ButtonDown(gamefunc_Fire)) loc.bits |= SKB_FIRE;
	if (buttonMap.ButtonDown(gamefunc_Open)) loc.bits |= SKB_OPEN;

#if 0
	// todo: handle these with CCMDs instead.
	if (buttonMap.ButtonDown(gamefunc_Inventory)) loc.bits |= SKB_INVENTORY;
	if (buttonMap.ButtonDown(gamefunc_MedKit)) loc.bits |= SKB_MEDKIT;
	if (buttonMap.ButtonDown(gamefunc_Steroids)) loc.bits |= SKB_STEROIDS;
	if (buttonMap.ButtonDown(gamefunc_NightVision)) loc.bits |= SKB_NIGHTVISION;
	if (buttonMap.ButtonDown(gamefunc_Holo_Duke)) loc.bits |= SKB_HOLODUKE;
	if (buttonMap.ButtonDown(gamefunc_Jetpack)) loc.bits |= SKB_JETPACK;
	//if (inputState.CheckPause()) loc.bits |= SKB_PAUSE;
	if (buttonMap.ButtonDown(gamefunc_Inventory_Left)) loc.bits |= SKB_INV_LEFT;
	if (buttonMap.ButtonDown(gamefunc_Inventory_Right)) loc.bits |= SKB_INV_RIGHT;

	/*
	loc.bits |= (buttonMap.ButtonDown(gamefunc_Center_View) << SK_CENTER_VIEW);
	loc.bits |= buttonMap.ButtonDown(gamefunc_Holster_Weapon) << SK_HOLSTER;
	loc.bits |= buttonMap.ButtonDown(gamefunc_TurnAround) << SK_TURNAROUND;
	*/

#else
	if (onVehicle) BitsToSend &= ~(SKB_HOLSTER|SKB_TURNAROUND|SKB_CENTER_VIEW);
#endif

	if (buttonMap.ButtonDown(gamefunc_Dpad_Select))
	{
		if (info.dx < 0 || info.dyaw < 0) loc.bits |= SKB_INV_LEFT;
		if (info.dx > 0 || info.dyaw < 0) loc.bits |= SKB_INV_RIGHT;
		// This eats the controller input for regular use
		info.dx = 0;
		info.dz = 0;
		info.dyaw = 0;
	}

	if (g_gameQuit) loc.bits |= SKB_GAMEQUIT;
	//if (inputState.GetKeyStatus(sc_Escape))  loc.bits |= SKB_ESCAPE; fixme. This never gets here because the menu eats the escape key.


	if (buttonMap.ButtonDown(gamefunc_Dpad_Aiming))
		info.dz = 0;
}

//---------------------------------------------------------------------------
//
// weapon selection bits.
// This should all be remapped to CCMDs, except for the controller check
// For the next and prev weapon functions this is particularly necessary 
// due to how the mouse wheel works.
//
//---------------------------------------------------------------------------

void processSelectWeapon(input_t& input)
{
	int j = WeaponToSend;
	WeaponToSend = 0;
	if (VOLUMEONE && (j >= 7 && j <= 10)) j = 0;

#if 0 // must be removed once the CCMDs are hooked up
	if (buttonMap.ButtonPressed(gamefunc_Weapon_1)) j = 1;
	if (buttonMap.ButtonPressed(gamefunc_Weapon_2))	j = 2;
	if (buttonMap.ButtonPressed(gamefunc_Weapon_3))	j = 3;
	if (buttonMap.ButtonPressed(gamefunc_Weapon_4))	j = 4;
	if (buttonMap.ButtonPressed(gamefunc_Weapon_5))	j = 5;
	if (buttonMap.ButtonPressed(gamefunc_Weapon_6))	j = 6;

	if (!VOLUMEONE)
	{
		if (buttonMap.ButtonPressed(gamefunc_Weapon_7))	j = 7;
		if (buttonMap.ButtonPressed(gamefunc_Weapon_8))	j = 8;
		if (buttonMap.ButtonPressed(gamefunc_Weapon_9))	j = 9;
		if (buttonMap.ButtonPressed(gamefunc_Weapon_10)) j = 10;
	}
	if (buttonMap.ButtonPressed(gamefunc_Previous_Weapon)) j = 11;
	if (buttonMap.ButtonPressed(gamefunc_Next_Weapon)) j = 12;
#endif

	if (buttonMap.ButtonDown(gamefunc_Dpad_Select) && input.fvel < 0) j = 11;
	if (buttonMap.ButtonDown(gamefunc_Dpad_Select) && input.fvel < 0) j = 12;

	if (j && (loc.bits & SKB_WEAPONMASK_BITS) == 0)
		loc.bits |= ESyncBits::FromInt(j * SKB_FIRST_WEAPON_BIT);

}

//---------------------------------------------------------------------------
//
// split out of playerinputmotocycle for readability purposes and condensed using ?: operators
//
//---------------------------------------------------------------------------

int motoApplyTurn(player_struct* p, int turnl, int turnr, int bike_turn, bool goback, double factor)
{
	static int turnheldtime;
	static int lastcontroltime;

	int tics = totalclock - lastcontroltime;
	lastcontroltime = totalclock;

	if (p->MotoSpeed == 0 || !p->on_ground)
	{
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
		if (turnl || p->moto_drink < 0)
		{
			turnheldtime += tics;
			p->TiltStatus -= (float)factor;
			if (p->TiltStatus < -10)
				p->TiltStatus = -10;
			if (turnheldtime >= TURBOTURNTIME && p->MotoSpeed > 0)
			{
				if (goback) return bike_turn ? 20 : 10;
				else return bike_turn ? -20 : -10;
			}
			else
			{
				if (goback) return bike_turn ? 10 : 3;
				else return bike_turn ? -10 : -3;
			}
		}
		else if (turnr || p->moto_drink > 0)
		{
			turnheldtime += tics;
			p->TiltStatus += (float)factor;
			if (p->TiltStatus > 10)
				p->TiltStatus = 10;
			if (turnheldtime >= TURBOTURNTIME && p->MotoSpeed > 0)
			{
				if (goback) return bike_turn ? -20 : -10;
				else return bike_turn ? 20 : 10;
			}
			else
			{
				if (goback) return bike_turn ? -10 : -3;
				else return bike_turn ? 10 : 3;
			}
		}
		else
		{
			turnheldtime = 0;

			if (p->TiltStatus > 0)
				p->TiltStatus -= (float)factor;
			else if (p->TiltStatus < 0)
				p->TiltStatus += (float)factor;

			if (fabs(p->TiltStatus) < 0.025)
				p->TiltStatus = 0;

		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// same for the boat
//
//---------------------------------------------------------------------------

static int boatApplyTurn(player_struct *p, int turnl, int turnr, int bike_turn, double factor)
{
	static int turnheldtime;
	static int lastcontroltime;

	int tics = totalclock - lastcontroltime;
	lastcontroltime = totalclock;

	if (p->MotoSpeed)
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
			if (turnheldtime >= TURBOTURNTIME && p->MotoSpeed != 0)
			{
				if (p->NotOnWater) return bike_turn ? -6 : -3;
				else return bike_turn ? -20 : -10;
			}
			else if (turnheldtime < TURBOTURNTIME && p->MotoSpeed != 0)
			{
				if (p->NotOnWater) return bike_turn ? -2 : -1;
				else return bike_turn ? -6 : -3;
			}
		}
		else if (turnr || p->moto_drink > 0)
		{
			turnheldtime += tics;
			if (!p->NotOnWater)
			{
				p->TiltStatus += (float)factor;
				if (p->TiltStatus > 10)
					p->TiltStatus = 10;
			}
			if (turnheldtime >= TURBOTURNTIME && p->MotoSpeed != 0)
			{
				if (p->NotOnWater) return bike_turn ? 6 : 3;
				else return bike_turn ? 20 : 10;
			}
			else if (turnheldtime < TURBOTURNTIME && p->MotoSpeed != 0)
			{
				if (p->NotOnWater) return bike_turn ? 2 : 1;
				else return bike_turn ? 6 : 3;
			}
		}
		else if (!p->NotOnWater)
		{
			turnheldtime = 0;

			if (p->TiltStatus > 0)
				p->TiltStatus -= (float)factor;
			else if (p->TiltStatus < 0)
				p->TiltStatus += (float)factor;

			if (fabs(p->TiltStatus) < 0.025)
				p->TiltStatus = 0;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// much of this was rewritten from scratch to make the logic easier to follow.
//
//---------------------------------------------------------------------------

void processBoatInput(player_struct *p, ControlInfo& info, input_t& input, double scaleAdjust)
{
	auto boat_turn = info.mousex + scaleAdjust * info.dyaw * (1. / 32); // originally this was 64, not 32. Why the change?
	int turnl = buttonMap.ButtonDown(gamefunc_Turn_Left) || buttonMap.ButtonDown(gamefunc_Strafe_Left);
	int turnr = buttonMap.ButtonDown(gamefunc_Turn_Right) || buttonMap.ButtonDown(gamefunc_Strafe_Right);

	// Cancel out micro-movement
	const double turn_threshold = 1 / 65536.;
	if (boat_turn < -turn_threshold)
		turnl = 1;
	else if (boat_turn > turn_threshold)
		turnr = 1;
	else
		boat_turn = 0;

	if (buttonMap.ButtonDown(gamefunc_Move_Forward) || buttonMap.ButtonDown(gamefunc_Strafe))
		loc.bits |= SKB_JUMP;
	if (buttonMap.ButtonDown(gamefunc_Move_Backward))
		loc.bits |= SKB_AIM_UP;
	if (buttonMap.ButtonDown(gamefunc_Run))
		loc.bits |= SKB_CROUCH;

	if (turnl)
		loc.bits |= SKB_AIM_DOWN;
	if (turnr)
		loc.bits |= SKB_LOOK_LEFT;

	double turnvel = boatApplyTurn(p, turnl, turnr, boat_turn != 0, scaleAdjust) * scaleAdjust * 2;

	// What is this? Optimization for playing with a mouse which the original did not have?
	if (boat_turn)
		turnvel *= clamp(boat_turn * boat_turn, 0., 1.);

	input.fvel = p->MotoSpeed;
	input.q16avel = fix16_from_dbl(turnvel);
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
	C_RegisterFunction("pause", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_PAUSE; return CCMD_OK; });
	C_RegisterFunction("steroids", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_STEROIDS; return CCMD_OK; });
	C_RegisterFunction("nightvision", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_NIGHTVISION; return CCMD_OK; });
	C_RegisterFunction("medkit", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_MEDKIT; return CCMD_OK; });
	C_RegisterFunction("centerview", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_CENTER_VIEW; return CCMD_OK; });
	C_RegisterFunction("holsterweapon", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_HOLSTER; return CCMD_OK; });
	C_RegisterFunction("invprev", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_INV_LEFT; return CCMD_OK; });
	C_RegisterFunction("invnext", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_INV_RIGHT; return CCMD_OK; });
	C_RegisterFunction("holoduke", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_HOLODUKE; return CCMD_OK; });
	C_RegisterFunction("jetpack", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_JETPACK; return CCMD_OK; });
	C_RegisterFunction("turnaround", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_TURNAROUND; return CCMD_OK; });
	C_RegisterFunction("invuse", nullptr, [](CCmdFuncPtr)->int { BitsToSend = SKB_INVENTORY; return CCMD_OK; });
}

// This is called from ImputState::ClearAllInput
void GameInterface::clearlocalinputstate()
{
	WeaponToSend = 0;
	BitsToSend = 0;
}

END_DUKE_NS
