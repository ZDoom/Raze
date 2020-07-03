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
#include "game.h"
#include "names_r.h"

BEGIN_DUKE_NS

#define CRECT windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

inline static void hud_drawpal(int x, int y, int tilenum, int shade, int orientation, int p, int scale = 32768)
{
	hud_rotatesprite(x << 16, y << 16, scale, (orientation & 4) ? 1024 : 0, tilenum, shade, p, 2 | orientation, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
}

inline static void rdmyospal(int x, int y, int tilenum, int shade, int orientation, int p)
{
	hud_drawpal(x, y, tilenum, shade, orientation, p, 36700);
}

inline static void rd2myospal(int x, int y, int tilenum, int shade, int orientation, int p)
{
	hud_drawpal(x, y, tilenum, shade, orientation, p, 44040);
}

inline static void rd3myospal(int x, int y, int tilenum, int shade, int orientation, int p)
{
	hud_drawpal(x, y, tilenum, shade, orientation, p, 47040);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

inline static void hud_draw(int x, int y, int tilenum, int shade, int orientation)
{
	int p = sector[ps[screenpeek].cursectnum].floorpal;
	rotatesprite(x << 16, y << 16, 65536L, (orientation & 4) ? 1024 : 0, tilenum, shade, p, 2 | orientation, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displaymasks_r(int snum)
{
	short p;

	if (sprite[ps[snum].i].pal == 1)
		p = 1;
	else
		p = sector[ps[snum].cursectnum].floorpal;

	if (ps[snum].scuba_on)
	{
		int pin = 0;
		if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING)) pin = RS_STRETCH;
		hud_rotatesprite((320 - (tilesiz[SCUBAMASK].x >> 1) - 15) << 16, (200 - (tilesiz[SCUBAMASK].y >> 1) + (sintable[(int)totalclock & 2047] >> 10)) << 16, 49152, 0, SCUBAMASK, 0, p, 2 + 16 + pin, CRECT);
		hud_rotatesprite((320 - tilesiz[SCUBAMASK + 4].x) << 16, (200 - tilesiz[SCUBAMASK + 4].y) << 16, 65536, 0, SCUBAMASK + 4, 0, p, 2 + 16 + pin, CRECT);
		hud_rotatesprite(tilesiz[SCUBAMASK + 4].x << 16, (200 - tilesiz[SCUBAMASK + 4].y) << 16, 65536, 1024, SCUBAMASK + 4, 0, p, 2 + 4 + 16 + pin, CRECT);
		hud_rotatesprite(35 << 16, (-1) << 16, 65536, 0, SCUBAMASK + 3, 0, p, 2 + 16 + pin, CRECT);
		hud_rotatesprite(285 << 16, 200 << 16, 65536, 1024, SCUBAMASK + 3, 0, p, 2 + 16 + pin, CRECT);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ShowMotorcycle(int x, int y, short tilenum, signed char shade, char orientation, char p, short a)
{
	hud_rotatesprite(x << 16, y << 16, 34816L, a, tilenum, shade, p, 2 | orientation, CRECT);
}


void ShowBoat(int x, int y, short tilenum, signed char shade, char orientation, char p, short a)
{
	hud_rotatesprite(x << 16, y << 16, 66048L, a, tilenum, shade, p, 2 | orientation, CRECT);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displayweapon_r(int snum)
{
	int gun_pos, looking_arc, cw;
	int weapon_xoffset, i, j;
	char o,pal;
	signed char gs;

	auto p = &ps[snum];
	auto kb = &p->kickback_pic;

	o = 0;

	looking_arc = abs(p->look_ang)/9;

	if (shadedsector[p->cursectnum] == 1)
		gs = 16;
	else
		gs = sprite[p->i].shade;
	if(gs > 24) gs = 24;

	if(p->newowner >= 0 || ud.camerasprite >= 0 || (sprite[p->i].pal != 1 && sprite[p->i].extra <= 0))
		return;

	gun_pos = 80-(p->weapon_pos*p->weapon_pos);

	weapon_xoffset =  (160)-90;
	weapon_xoffset -= (sintable[((p->weapon_sway>>1)+512)&2047]/(1024+512));
	weapon_xoffset -= 58 + p->weapon_ang;
	if( sprite[p->i].xrepeat < 8 )
		gun_pos -= abs(sintable[(p->weapon_sway<<2)&2047]>>9);
	else gun_pos -= abs(sintable[(p->weapon_sway>>1)&2047]>>10);

	gun_pos -= (p->hard_landing<<3);

	if(p->last_weapon >= 0)
		cw = p->last_weapon;
	else cw = p->curr_weapon;

	j = 14-p->quick_kick;
	if(j != 14)
	{
		if(sprite[p->i].pal == 1)
			pal = 1;
		else
			pal = p->palookup;
	}

	if (p->OnMotorcycle)
	{
		int temp_kb;
		if (numplayers == 1)
		{
			if (*kb)
			{
				gs = 0;
				if (*kb == 1)
				{
					if ((krand()&1) == 1)
						temp_kb = MOTOHIT+1;
					else
						temp_kb = MOTOHIT+2;
				}
				else if (*kb == 4)
				{
					if ((krand()&1) == 1)
						temp_kb = MOTOHIT+3;
					else
						temp_kb = MOTOHIT+4;
				}
				else
					temp_kb = MOTOHIT;

			}
			else
				temp_kb = MOTOHIT;
		}
		else
		{
			if (*kb)
			{
				gs = 0;
				if (*kb == 1)
					temp_kb = MOTOHIT+1;
				else if (*kb == 2)
					temp_kb = MOTOHIT+2;
				else if (*kb == 3)
					temp_kb = MOTOHIT+3;
				else if (*kb == 4)
					temp_kb = MOTOHIT+4;
				else
					temp_kb = MOTOHIT;

			}
			else
				temp_kb = MOTOHIT;
		}
		if (sprite[p->i].pal == 1)
			pal = 1;
		else
			pal = sector[p->cursectnum].floorpal;

		if (p->TiltStatus >= 0)
			ShowMotorcycle(160-(p->look_ang>>1), 174, temp_kb, gs, 0, pal, p->TiltStatus*5);
		else if (p->TiltStatus < 0)
			ShowMotorcycle(160-(p->look_ang>>1), 174, temp_kb, gs, 0, pal, p->TiltStatus*5+2047);
		return;
	}
	if (p->OnBoat)
	{
		int temp2, temp_kb, temp3;
		temp2 = 0;
		if (p->TiltStatus > 0)
		{
			if (*kb == 0)
				temp_kb = BOATHIT+1;
			else if (*kb <= 3)
			{
				temp_kb = BOATHIT+5;
				temp2 = 1;
			}
			else if (*kb <= 6)
			{
				temp_kb = BOATHIT+6;
				temp2 = 1;
			}
			else
				temp_kb = BOATHIT+1;
		}
		else if (p->TiltStatus < 0)
		{
			if (*kb == 0)
				temp_kb = BOATHIT+2;
			else if (*kb <= 3)
			{
				temp_kb = BOATHIT+7;
				temp2 = 1;
			}
			else if (*kb <= 6)
			{
				temp_kb = BOATHIT+8;
				temp2 = 1;
			}
			else
				temp_kb = BOATHIT+2;
		}
		else
		{
			if (*kb == 0)
				temp_kb = BOATHIT;
			else if (*kb <= 3)
			{
				temp_kb = BOATHIT+3;
				temp2 = 1;
			}
			else if (*kb <= 6)
			{
				temp_kb = BOATHIT+4;
				temp2 = 1;
			}
			else
				temp_kb = BOATHIT;
		}

		if (sprite[p->i].pal == 1)
			pal = 1;
		else
			pal = sector[p->cursectnum].floorpal;

		if (p->NotOnWater)
			temp3 = 170;
		else
			temp3 = 170 + (*kb>>2);

		if (temp2)
			gs = -96;

		if (p->TiltStatus >= 0)
			ShowBoat(160-(p->look_ang>>1), temp3, temp_kb, gs, 0, pal, p->TiltStatus);
		else if (p->TiltStatus < 0)
			ShowBoat(160-(p->look_ang>>1), temp3, temp_kb, gs, 0, pal, p->TiltStatus+2047);
		return;
	}

	if( sprite[p->i].xrepeat < 8 )
	{
		static int fistsign;
		if(p->jetpack_on == 0 )
		{
			i = sprite[p->i].xvel;
			looking_arc += 32-(i>>1);
			fistsign += i>>1;
		}
		cw = weapon_xoffset;
		weapon_xoffset += sintable[(fistsign)&2047]>>10;
		hud_draw(weapon_xoffset+250-(p->look_ang>>1),
			 looking_arc+258-(abs(sintable[(fistsign)&2047]>>8)),
			 FIST,gs,o);
		weapon_xoffset = cw;
		weapon_xoffset -= sintable[(fistsign)&2047]>>10;
		hud_draw(weapon_xoffset+40-(p->look_ang>>1),
			 looking_arc+200+(abs(sintable[(fistsign)&2047]>>8)),
			 FIST,gs,o|4);
	}
	else
	{
		int pin = 0;

		if (sprite[p->i].pal == 1)
			pal = 1;
		else
			pal = sector[p->cursectnum].floorpal;


		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaycrowbar = [&]
		{
			static const short kb_frames[] = { 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 };
			static const short kb_ox[] = { 310,342,364,418,350,316,282,288,0,0 };
			static const short kb_oy[] = { 300,362,320,268,248,248,277,420,0,0 };
			short x;
			short y;
			x = weapon_xoffset + ((kb_ox[kb_frames[*kb]] >> 1) - 12);
			y = 200 - (244 - kb_oy[kb_frames[*kb]]);
			hud_drawpal(x - (p->look_ang >> 1), looking_arc + y - gun_pos,
				KNEE + kb_frames[*kb], gs, 0, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayslingblade = [&]
		{
			static const short kb_frames[] = { 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 };
			static const short kb_ox[] = { 580,676,310,491,356,210,310,614 };
			static const short kb_oy[] = { 369,363,300,323,371,400,300,440 };
			short x, y;
			x = weapon_xoffset + ((kb_ox[kb_frames[*kb]] >> 1) - 12);
			y = 210 - (244 - kb_oy[kb_frames[*kb]]);
			hud_drawpal(x - (p->look_ang >> 1) + 20, looking_arc + y - gun_pos - 80,
				SLINGBLADE + kb_frames[*kb], gs, 0, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaybowlingball = [&]
		{
			weapon_xoffset += 8;
			gun_pos -= 10;
			if (p->ammo_amount[BOWLING_WEAPON])
			{
				hud_drawpal(weapon_xoffset + 162 - (p->look_ang >> 1),
					looking_arc + 214 - gun_pos + (*kb << 3), BOWLINGBALLH, gs, o, pal);
			}
			else
			{
				rdmyospal(weapon_xoffset + 162 - (p->look_ang >> 1),
					looking_arc + 214 - gun_pos, HANDTHROW + 5, gs, o, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaypowderkeg = [&]
		{
			weapon_xoffset += 8;
			gun_pos -= 10;
			if (p->ammo_amount[POWDERKEG_WEAPON])
			{
				rdmyospal(weapon_xoffset + 180 - (p->look_ang >> 1),
					looking_arc + 214 - gun_pos + (*kb << 3), POWDERH, gs, o, pal);
				rdmyospal(weapon_xoffset + 90 - (p->look_ang >> 1),
					looking_arc + 214 - gun_pos + (*kb << 3), POWDERH, gs, o | 4, pal);
			}
			else
			{
				rdmyospal(weapon_xoffset + 162 - (p->look_ang >> 1),
					looking_arc + 214 - gun_pos, HANDTHROW + 5, gs, o, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaycrossbow = [&]
		{
			if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING)) pin = RS_ALIGN_R;
			static const uint8_t kb_frames[] = { 0,1,1,2,2,3,2,3,2,3,2,2,2,2,2,2,2,2,2,4,4,4,4,5,5,5,5,6,6,6,6,6,6,7,7,7,7,7,7 };
			if (kb_frames[*kb] == 2 || kb_frames[*kb] == 3)
			{
				rdmyospal((weapon_xoffset + 200) - (p->look_ang >> 1),
					looking_arc + 250 - gun_pos, RPGGUN + kb_frames[*kb], gs, o | pin, pal);
			}
			else if (kb_frames[*kb] == 1)
			{
				rdmyospal((weapon_xoffset + 200) - (p->look_ang >> 1),
					looking_arc + 250 - gun_pos, RPGGUN + kb_frames[*kb], 0, o | pin, pal);
			}
			else
			{
				rdmyospal((weapon_xoffset + 210) - (p->look_ang >> 1),
					looking_arc + 255 - gun_pos, RPGGUN + kb_frames[*kb], gs, o | pin, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaychicken = [&]
		{
			if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING)) pin = RS_ALIGN_R;
			if (*kb)
			{
				static const uint8_t kb_frames[] = { 0,1,1,2,2,3,2,3,2,3,2,2,2,2,2,2,2,2,2,4,4,4,4,5,5,5,5,6,6,6,6,6,6,7,7,7,7,7,7 };
				if (kb_frames[*kb] == 2 || kb_frames[*kb] == 3)
				{
					rdmyospal((weapon_xoffset + 200) - (p->look_ang >> 1),
						looking_arc + 250 - gun_pos, RPGGUN2 + kb_frames[*kb], gs, o |  pin, pal);
				}
				else if (kb_frames[*kb] == 1)
				{
					rdmyospal((weapon_xoffset + 200) - (p->look_ang >> 1),
						looking_arc + 250 - gun_pos, RPGGUN2 + kb_frames[*kb], 0, o |  pin, pal);
				}
				else
				{
					rdmyospal((weapon_xoffset + 210) - (p->look_ang >> 1),
						looking_arc + 255 - gun_pos, RPGGUN2 + kb_frames[*kb], gs, o |  pin, pal);
				}
			}
			else
			{
				if (ud.multimode < 2)
				{
					if (chickenphase)
					{
						rdmyospal((weapon_xoffset + 210) - (p->look_ang >> 1),
							looking_arc + 222 - gun_pos, RPGGUN2 + 7, gs, o |  pin, pal);
					}
					else if ((krand() & 15) == 5)
					{
						spritesound(327, p->i);
						rdmyospal((weapon_xoffset + 210) - (p->look_ang >> 1),
							looking_arc + 222 - gun_pos, RPGGUN2 + 7, gs, o |  pin, pal);
						chickenphase = 6;
					}
					else
					{
						rdmyospal((weapon_xoffset + 210) - (p->look_ang >> 1),
							looking_arc + 225 - gun_pos, RPGGUN2, gs, o |  pin, pal);
					}
				}
				else
				{
					rdmyospal((weapon_xoffset + 210) - (p->look_ang >> 1),
						looking_arc + 225 - gun_pos, RPGGUN2, gs, o |  pin, pal);
				}
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshotgun = [&]
		{
			weapon_xoffset -= 8;

			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;

			{
				short x;
				short y;
				static const short kb_frames3[] = { 0,0,1,1,2,2,5,5,6,6,7,7,8,8,0,0,0,0,0,0,0 };
				static const short kb_frames2[] = { 0,0,3,3,4,4,5,5,6,6,7,7,8,8,0,0,20,20,21,21,21,21,20,20,20,20,0,0 };
				static const short kb_frames[] = { 0,0,1,1,2,2,3,3,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,0,0,20,20,21,21,21,21,20,20,20,20,0,0 };
				static const short kb_ox[] = { 300,300,300,300,300,330,320,310,305,306,302 };
				static const short kb_oy[] = { 315,300,302,305,302,302,303,306,302,404,384 };
				short tm;
				tm = 180;
				if (p->shotgun_state[1])
				{
					if ((*kb) < 26)
					{
						if (kb_frames[*kb] == 3 || kb_frames[*kb] == 4)
							gs = 0;
						x = weapon_xoffset + ((kb_ox[kb_frames[*kb]] >> 1) - 12);
						y = tm - (244 - kb_oy[kb_frames[*kb]]);
						hud_drawpal(x + 64 - (p->look_ang >> 1),
							y + looking_arc - gun_pos, SHOTGUN + kb_frames[*kb], gs, 0, pal);
					}
					else
					{
						if (kb_frames[*kb] > 0)
						{
							x = weapon_xoffset + ((kb_ox[kb_frames[(*kb) - 11]] >> 1) - 12);
							y = tm - (244 - kb_oy[kb_frames[(*kb) - 11]]);
						}
						else
						{
							x = weapon_xoffset + ((kb_ox[kb_frames[*kb]] >> 1) - 12);
							y = tm - (244 - kb_oy[kb_frames[*kb]]);
						}
						switch (*kb)
						{
						case 23:
							y += 60;
							break;
						case 24:
							y += 30;
							break;
						}
						hud_drawpal(x + 64 - (p->look_ang >> 1), y + looking_arc - gun_pos, SHOTGUN + kb_frames[*kb], gs, 0, pal);
						if (kb_frames[*kb] == 21)
							hud_drawpal(x + 96 - (p->look_ang >> 1), y + looking_arc - gun_pos, SHOTGUNSHELLS, gs, 0, pal);
					}
				}
				else
				{
					if ((*kb) < 16)
					{
						if (p->shotgun_state[0])
						{
							if (kb_frames2[*kb] == 3 || kb_frames2[*kb] == 4)
								gs = 0;
							x = weapon_xoffset + ((kb_ox[kb_frames2[*kb]] >> 1) - 12);
							y = tm - (244 - kb_oy[kb_frames2[*kb]]);
							hud_drawpal(x + 64 - (p->look_ang >> 1),
								y + looking_arc - gun_pos, SHOTGUN + kb_frames2[*kb], gs, 0, pal);
						}
						else
						{
							if (kb_frames3[*kb] == 1 || kb_frames3[*kb] == 2)
								gs = 0;
							x = weapon_xoffset + ((kb_ox[kb_frames3[*kb]] >> 1) - 12);
							y = tm - (244 - kb_oy[kb_frames3[*kb]]);
							hud_drawpal(x + 64 - (p->look_ang >> 1),
								y + looking_arc - gun_pos, SHOTGUN + kb_frames3[*kb], gs, 0, pal);
						}
					}
					else if (p->shotgun_state[0])
					{
						if (kb_frames2[*kb] > 0)
						{
							x = weapon_xoffset + ((kb_ox[kb_frames2[(*kb) - 11]] >> 1) - 12);
							y = tm - (244 - kb_oy[kb_frames2[(*kb) - 11]]);
						}
						else
						{
							x = weapon_xoffset + ((kb_ox[kb_frames2[*kb]] >> 1) - 12);
							y = tm - (244 - kb_oy[kb_frames2[*kb]]);
						}
						switch (*kb)
						{
						case 23:
							y += 60;
							break;
						case 24:
							y += 30;
							break;
						}
						hud_drawpal(x + 64 - (p->look_ang >> 1), y + looking_arc - gun_pos, SHOTGUN + kb_frames2[*kb], gs, 0, pal);
						if (kb_frames2[*kb] == 21)
							hud_drawpal(x + 96 - (p->look_ang >> 1), y + looking_arc - gun_pos, SHOTGUNSHELLS, gs, 0, pal);
					}
				}
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayrifle = [&]
		{
			if (*kb > 0)
				gun_pos -= sintable[(*kb) << 7] >> 12;

			if (*kb > 0 && sprite[p->i].pal != 1) weapon_xoffset += 1 - (rand() & 3);

			switch (*kb)
			{
			case 0:
				hud_drawpal(weapon_xoffset + 178 - (p->look_ang >> 1) + 30, looking_arc + 233 - gun_pos + 5,
					CHAINGUN, gs, o, pal);
				break;
			default:
				gs = 0;
				if (*kb < 8)
				{
					i = rand() & 7;
					hud_drawpal(weapon_xoffset + 178 - (p->look_ang >> 1) + 30, looking_arc + 233 - gun_pos + 5,
						CHAINGUN + 1, gs, o, pal);
				}
				else hud_drawpal(weapon_xoffset + 178 - (p->look_ang >> 1) + 30, looking_arc + 233 - gun_pos + 5,
					CHAINGUN + 2, gs, o, pal);
				break;
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaypistol = [&]
		{
			if ((*kb) < 22)
			{
				static const uint8_t kb_frames[] = { 0,0,1,1,2,2,3,3,4,4,6,6,6,6,5,5,4,4,3,3,0,0 };
				static const short kb_ox[] = { 194,190,185,208,215,215,216,216,201,170 };
				static const short kb_oy[] = { 256,249,248,238,228,218,208,256,245,258 };
				short x;
				short y;

				x = weapon_xoffset + (kb_ox[kb_frames[*kb]] - 12);
				y = 244 - (244 - kb_oy[kb_frames[*kb]]);

				if (kb_frames[*kb])
					gs = 0;

				rdmyospal(x - (p->look_ang >> 1),
					y + looking_arc - gun_pos, FIRSTGUN + kb_frames[*kb], gs, 0, pal);
			}
			else
			{
				static const short kb_frames[] = { 0,0,1,1,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0 };
				static const short kb_ox[] = { 244,244,244 };
				static const short kb_oy[] = { 256,249,248 };
				short x;
				short dx;
				short y;
				short dy;

				x = weapon_xoffset + (kb_ox[kb_frames[(*kb) - 22]] - 12);
				y = 244 - (244 - kb_oy[kb_frames[(*kb) - 22]]);
				switch (*kb)
				{
				case 28:
					dy = 10;
					dx = 5;
					break;
				case 29:
					dy = 20;
					dx = 10;
					break;
				case 30:
					dy = 30;
					dx = 15;
					break;
				case 31:
					dy = 40;
					dx = 20;
					break;
				case 32:
					dy = 50;
					dx = 25;
					break;
				case 33:
					dy = 40;
					dx = 20;
					break;
				case 34:
					dy = 30;
					dx = 15;
					break;
				case 35:
					dy = 20;
					dx = 10;
					break;
				case 36:
					dy = 10;
					dx = 5;
					break;
				default:
					dy = 0;
					dx = 0;
					break;
				}
				rdmyospal(x - (p->look_ang >> 1) - dx,
					y + looking_arc - gun_pos + dy, FIRSTGUNRELOAD + kb_frames[(*kb) - 22], gs, 0, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaydynamite = [&]
		{
			gun_pos -= 9 * (*kb);

			rdmyospal(weapon_xoffset + 190 - (p->look_ang >> 1), looking_arc + 260 - gun_pos, HANDTHROW, gs, o, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaythrowingdynamite = [&]
		{
			int dx;
			int x;
			int dy;
			dx = 25;
			x = 100;
			dy = 20;
			if ((*kb) < 20)
			{
				if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING)) pin = RS_ALIGN_R;
				static const int8_t remote_frames[] = { 1,1,1,1,1,2,2,2,2,3,3,3,4,4,4,5,5,5,5,5,6,6,6 };

				if (*kb)
				{
					if ((*kb) < 5)
					{
						rdmyospal(weapon_xoffset + x + 190 - (p->look_ang >> 1) - dx,
							looking_arc + 258 - gun_pos - 64 + p->at57e - dy, RRTILE1752, 0, o |  pin, pal);
					}
					rdmyospal(weapon_xoffset + x + 190 - (p->look_ang >> 1),
						looking_arc + 258 - gun_pos - dy, HANDTHROW + remote_frames[*kb], gs, o |  pin, pal);
				}
				else
				{
					if ((*kb) < 5)
					{
						rdmyospal(weapon_xoffset + x + 190 - (p->look_ang >> 1) - dx,
							looking_arc + 258 - gun_pos - 64 + p->at57e - dy, RRTILE1752, 0, o |  pin, pal);
					}
					rdmyospal(weapon_xoffset + x + 190 - (p->look_ang >> 1),
						looking_arc + 258 - gun_pos - dy, HANDTHROW + 1, gs, o |  pin, pal);
				}
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaytits = [&]
		{
			if (*kb)
			{
				gs = 0;
				rd3myospal(150 + (weapon_xoffset >> 1) - (p->look_ang >> 1), 266 + (looking_arc >> 1) - gun_pos, DEVISTATOR, gs, o, pal);
			}
			else
				rd3myospal(150 + (weapon_xoffset >> 1) - (p->look_ang >> 1), 266 + (looking_arc >> 1) - gun_pos, DEVISTATOR + 1, gs, o, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayblaster = [&]
		{
			if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING)) pin = RS_ALIGN_R;
			if ((*kb))
			{
				char cat_frames[] = { 0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
				rdmyospal(weapon_xoffset + 260 - (p->look_ang >> 1), looking_arc + 215 - gun_pos, FREEZE + cat_frames[*kb], -32, o |  pin, pal);
			}
			else rdmyospal(weapon_xoffset + 260 - (p->look_ang >> 1), looking_arc + 215 - gun_pos, FREEZE, gs, o |  pin, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaysaw = [&]
		{
			weapon_xoffset += 28;
			looking_arc += 18;
			if ((*kb) == 0)
			{
				rd2myospal(weapon_xoffset + 188 - (p->look_ang >> 1),
					looking_arc + 240 - gun_pos, SHRINKER, gs, o, pal);
			}
			else
			{
				if (sprite[p->i].pal != 1)
				{
					weapon_xoffset += rand() & 3;
					gun_pos += (rand() & 3);
				}

				if (cw == BUZZSAW_WEAPON)
				{
					rd2myospal(weapon_xoffset + 184 - (p->look_ang >> 1),
						looking_arc + 240 - gun_pos, GROWSPARK + ((*kb) & 2), gs, o, 0);
				}
				else
				{
					signed char kb_frames[] = { 1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0 };
					short frm = kb_frames[*kb];
					rd2myospal(weapon_xoffset + 184 - (p->look_ang >> 1),
						looking_arc + 240 - gun_pos, SHRINKER + frm, gs, o, 0);
				}
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		switch (cw)
		{
		case KNEE_WEAPON:
			displaycrowbar();
			break;

		case SLINGBLADE_WEAPON:
			displayslingblade();
			break;

		case POWDERKEG_WEAPON:
			displaypowderkeg();
			break;

		case BOWLING_WEAPON:
			displaybowlingball();
			break;

		case CROSSBOW_WEAPON:
			displaycrossbow();
			break;

		case CHICKEN_WEAPON:
			displaychicken();
			break;

		case SHOTGUN_WEAPON:
			displayshotgun();
			break;

		case RIFLEGUN_WEAPON:
			displayrifle();
			break;

		case PISTOL_WEAPON:
			displaypistol();
			break;

		case DYNAMITE_WEAPON:
			displaydynamite();
			break;

		case THROWINGDYNAMITE_WEAPON:
			displaythrowingdynamite();
			break;

		case TIT_WEAPON:
			displaytits();
			break;

		case MOTORCYCLE_WEAPON:
		case BOAT_WEAPON:
			break;

		case ALIENBLASTER_WEAPON:
			displayblaster();
			break;

		case THROWSAW_WEAPON:
		case BUZZSAW_WEAPON:
			displaysaw();
			break;
		}
	}
}

END_DUKE_NS