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
	p->aim_mode = PlayerInput(snum, SK_AIMMODE);
	if (p->aim_mode < i)
		p->return_to_center = 9;

	if (isRR())
	{
		if (PlayerInput(snum, SK_QUICK_KICK) && p->last_pissed_time == 0)
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
		if (PlayerInput(snum, SK_QUICK_KICK) && p->quick_kick == 0 && (p->curr_weapon != KNEE_WEAPON || p->kickback_pic == 0))
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

	// WTF??? In the original source this was a soup of numeric literals, i.e. totally incomprehensible.
	// The bit mask has been exported to the bit type enum.
	if (!PlayerInputBits(snum, SK_INTERFACE_BITS))
		p->interface_toggle_flag = 0;
	else if (p->interface_toggle_flag == 0)
	{
		p->interface_toggle_flag = 1;

		if (PlayerInput(snum, SK_PAUSE))
		{
			ud.pause_on = !ud.pause_on;
			if (ud.pause_on == 1 && PlayerInput(snum, SK_RUN)) ud.pause_on = 2; // Mode 2 is silent, i.e. prints no notification.
			Mus_SetPaused(ud.pause_on);
			S_PauseSounds(ud.pause_on);
		}

		// Don't go on if paused or dead.
		if (ud.pause_on) return;
		if (sprite[p->i].extra <= 0) return;

		// Activate an inventory item. This just forwards to the other inventory bits. If the inventory selector was taken out of the playsim this could be removed.
		if (PlayerInput(snum, SK_INVENTORY) && p->newowner == -1)
		{
			SetGameVarID(g_iReturnVarID, 0, -1, snum);
			OnEvent(EVENT_INVENTORY, -1, snum, -1);
			if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
			{
				switch (p->inven_icon)
				{
					// Yet another place where no symbolic constants were used. :(
				case ICON_JETPACK: PlayerSetInput(snum, SK_JETPACK); break;
				case ICON_HOLODUKE: PlayerSetInput(snum, SK_HOLODUKE); break;
				case ICON_HEATS: PlayerSetInput(snum, SK_NIGHTVISION); break;
				case ICON_FIRSTAID: PlayerSetInput(snum, SK_MEDKIT); break;
				case ICON_STEROIDS: PlayerSetInput(snum, SK_STEROIDS); break;
				}
			}
		}

		if (!isRR() && PlayerInput(snum, SK_NIGHTVISION))
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

		if (PlayerInput(snum, SK_STEROIDS))
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

		if (PlayerInput(snum, SK_INV_LEFT) || PlayerInput(snum, SK_INV_RIGHT))
		{
			p->invdisptime = 26 * 2;

			if (PlayerInput(snum, SK_INV_RIGHT)) k = 1;
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
			if (PlayerInput(snum, SK_INV_LEFT))
			{
				SetGameVarID(g_iReturnVarID, dainv, -1, snum);
				OnEvent(EVENT_INVENTORYLEFT, -1, snum, -1);
				dainv = GetGameVarID(g_iReturnVarID, -1, snum);
			}
			if (PlayerInput(snum, SK_INV_RIGHT))
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

		j = (PlayerInputBits(snum, SK_WEAPONMASK_BITS) >> SK_WEAPON_BITS) - 1;
		if (j > 0 && p->kickback_pic > 0)
			p->wantweaponfire = j;

		// Here we have to be extra careful that the weapons do not get mixed up, so let's keep the code for Duke and RR completely separate.
		fi.selectweapon(snum, j);

		if (PlayerInput(snum, SK_HOLSTER))
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

		if (PlayerInput(snum, SK_HOLODUKE) && (isRR() || p->newowner == -1))
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

		if (isRR() && PlayerInput(snum, SK_NIGHTVISION) && p->newowner == -1)
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

		if (PlayerInput(snum, SK_MEDKIT))
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

		if (PlayerInput(snum, SK_JETPACK) && (isRR() || p->newowner == -1))
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

		if (PlayerInput(snum, SK_TURNAROUND) && p->one_eighty_count == 0)
		{
			SetGameVarID(g_iReturnVarID, 0, -1, snum);
			OnEvent(EVENT_TURNAROUND, -1, snum, -1);
			if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
			{
				p->one_eighty_count = -1024;
			}
		}
	}
}
END_DUKE_NS
