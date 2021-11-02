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
#include "names_r.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

inline static void hud_drawpal(double x, double y, int tilenum, int shade, int orientation, int p, int scale = 32768)
{
	hud_drawsprite(x, y, scale, 0, tilenum, shade, p, 2 | orientation);
}

inline static void rdmyospal(double x, double y, int tilenum, int shade, int orientation, int p)
{
	hud_drawpal(x, y, tilenum, shade, orientation, p, 36700);
}

inline static void rd2myospal(double x, double y, int tilenum, int shade, int orientation, int p)
{
	hud_drawpal(x, y, tilenum, shade, orientation, p, 44040);
}

inline static void rd3myospal(double x, double y, int tilenum, int shade, int orientation, int p)
{
	hud_drawpal(x, y, tilenum, shade, orientation, p, 47040);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displaymasks_r(int snum, int p, double smoothratio)
{
	if (ps[snum].scuba_on)
	{
		//int pin = 0;
		// to get the proper clock value with regards to interpolation we have add a smoothratio based offset to the value.
		double interpclock = PlayClock + (+TICSPERFRAME/65536.) * smoothratio;
		int pin = RS_STRETCH;
		hud_drawsprite((320 - (tileWidth(SCUBAMASK) >> 1) - 15), (200 - (tileHeight(SCUBAMASK) >> 1) + bsinf(interpclock, -10)), 49152, 0, SCUBAMASK, 0, p, 2 + 16 + pin);
		hud_drawsprite((320 - tileWidth(SCUBAMASK + 4)), (200 - tileHeight(SCUBAMASK + 4)), 65536, 0, SCUBAMASK + 4, 0, p, 2 + 16 + pin);
		hud_drawsprite(tileWidth(SCUBAMASK + 4), (200 - tileHeight(SCUBAMASK + 4)), 65536, 0, SCUBAMASK + 4, 0, p, 2 + 4 + 16 + pin);
		hud_drawsprite(35, (-1), 65536, 0, SCUBAMASK + 3, 0, p, 2 + 16 + pin);
		hud_drawsprite(285, 200, 65536, 1024, SCUBAMASK + 3, 0, p, 2 + 16 + pin);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ShowMotorcycle(double x, double y, int tilenum, int shade, int orientation, int p, double a)
{
	hud_drawsprite(x, y, 34816, a, tilenum, shade, p, 2 | orientation);
}


void ShowBoat(double x, double y, int tilenum, int shade, int orientation, int p, double a)
{
	hud_drawsprite(x, y, 66048, a, tilenum, shade, p, 2 | orientation);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displayweapon_r(int snum, double smoothratio)
{
	int cw;
	int i, j;
	double weapon_sway, weapon_xoffset, gun_pos, looking_arc, look_anghalf, hard_landing, TiltStatus;
	char o,pal;
	signed char shade;

	auto p = &ps[snum];
	auto kb = &p->kickback_pic;

	o = 0;

	if (cl_hudinterpolation)
	{
		weapon_sway = interpolatedvaluef(p->oweapon_sway, p->weapon_sway, smoothratio);
		hard_landing = interpolatedvaluef(p->ohard_landing, p->hard_landing, smoothratio);
		gun_pos = 80 - interpolatedvaluef(p->oweapon_pos * p->oweapon_pos, p->weapon_pos * p->weapon_pos, smoothratio);
		TiltStatus = !SyncInput() ? p->TiltStatus : interpolatedvaluef(p->oTiltStatus, p->TiltStatus, smoothratio);
	}
	else
	{
		weapon_sway = p->weapon_sway;
		hard_landing = p->hard_landing;
		gun_pos = 80 - (p->weapon_pos * p->weapon_pos);
		TiltStatus = p->TiltStatus;
	}

	look_anghalf = p->angle.look_anghalf(smoothratio);
	looking_arc = p->angle.looking_arc(smoothratio);
	hard_landing *= 8.;

	gun_pos -= fabs(p->GetActor()->s->xrepeat < 8 ? bsinf(weapon_sway * 4., -9) : bsinf(weapon_sway * 0.5, -10));
	gun_pos -= hard_landing;

	weapon_xoffset = (160)-90;
	weapon_xoffset -= bcosf(weapon_sway * 0.5) * (1. / 1536.);
	weapon_xoffset -= 58 + p->weapon_ang;

	if (shadedsector[p->cursectnum] == 1)
		shade = 16;
	else
		shade = p->GetActor()->s->shade;
	if(shade > 24) shade = 24;

	pal = p->GetActor()->s->pal == 1 ? 1 : pal = sector[p->cursectnum].floorpal;

	if(p->newOwner != nullptr || ud.cameraactor != nullptr || p->over_shoulder_on > 0 || (p->GetActor()->s->pal != 1 && p->GetActor()->s->extra <= 0))
		return;

	if(p->last_weapon >= 0)
		cw = p->last_weapon;
	else cw = p->curr_weapon;

	j = 14-p->quick_kick;
	if(j != 14)
	{
		if(p->GetActor()->s->pal == 1)
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
				shade = 0;
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
				shade = 0;
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

		ShowMotorcycle(160-look_anghalf, 174, temp_kb, shade, 0, pal, TiltStatus*5);
		return;
	}
	if (p->OnBoat)
	{
		int temp2, temp_kb, temp3;
		temp2 = 0;
		if (TiltStatus > 0)
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
		else if (TiltStatus < 0)
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

		if (p->NotOnWater)
			temp3 = 170;
		else
			temp3 = 170 + (*kb>>2);

		if (temp2)
			shade = -96;

		ShowBoat(160-look_anghalf, temp3, temp_kb, shade, 0, pal, TiltStatus);
		return;
	}

	if (p->GetActor()->s->xrepeat < 8)
	{
		static int fistsign;
		if (p->jetpack_on == 0)
		{
			i = p->GetActor()->s->xvel;
			looking_arc += 32 - (i >> 1);
			fistsign += i >> 1;
		}
		double owo = weapon_xoffset;
		weapon_xoffset += bsinf(fistsign, -10);
		hud_draw(weapon_xoffset + 250 - look_anghalf, looking_arc + 258 - abs(bsinf(fistsign, -8)),	FIST, shade, o);
		weapon_xoffset = owo;
		weapon_xoffset -= bsinf(fistsign, -10);
		hud_draw(weapon_xoffset + 40 - look_anghalf, looking_arc + 200 + abs(bsinf(fistsign, -8)), FIST, shade, o | 4);
	}
	else
	{
		int pin = 0;


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
			double x;
			short y;
			x = weapon_xoffset + ((kb_ox[kb_frames[*kb]] >> 1) - 12);
			y = 200 - (244 - kb_oy[kb_frames[*kb]]);
			hud_drawpal(x - look_anghalf, looking_arc + y - gun_pos,
				KNEE + kb_frames[*kb], shade, 0, pal);
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
			double x;
			short y;
			x = weapon_xoffset + ((kb_ox[kb_frames[*kb]] >> 1) - 12);
			y = 210 - (244 - kb_oy[kb_frames[*kb]]);
			hud_drawpal(x - look_anghalf + 20, looking_arc + y - gun_pos - 80,
				SLINGBLADE + kb_frames[*kb], shade, 0, pal);
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
				hud_drawpal(weapon_xoffset + 162 - look_anghalf,
					looking_arc + 214 - gun_pos + (*kb << 3), BOWLINGBALLH, shade, o, pal);
			}
			else
			{
				rdmyospal(weapon_xoffset + 162 - look_anghalf,
					looking_arc + 214 - gun_pos, HANDTHROW + 5, shade, o, pal);
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
				rdmyospal(weapon_xoffset + 180 - look_anghalf,
					looking_arc + 214 - gun_pos + (*kb << 3), POWDERH, shade, o, pal);
				rdmyospal(weapon_xoffset + 90 - look_anghalf,
					looking_arc + 214 - gun_pos + (*kb << 3), POWDERH, shade, o | 4, pal);
			}
			else
			{
				rdmyospal(weapon_xoffset + 162 - look_anghalf,
					looking_arc + 214 - gun_pos, HANDTHROW + 5, shade, o, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaycrossbow = [&]
		{
			if (!(gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) pin = RS_ALIGN_R;
			static const uint8_t kb_frames[] = { 0,1,1,2,2,3,2,3,2,3,2,2,2,2,2,2,2,2,2,4,4,4,4,5,5,5,5,6,6,6,6,6,6,7,7,7,7,7,7 };
			if (kb_frames[*kb] == 2 || kb_frames[*kb] == 3)
			{
				rdmyospal((weapon_xoffset + 200) - look_anghalf,
					looking_arc + 250 - gun_pos, RPGGUN + kb_frames[*kb], shade, o | pin, pal);
			}
			else if (kb_frames[*kb] == 1)
			{
				rdmyospal((weapon_xoffset + 200) - look_anghalf,
					looking_arc + 250 - gun_pos, RPGGUN + kb_frames[*kb], 0, o | pin, pal);
			}
			else
			{
				rdmyospal((weapon_xoffset + 210) - look_anghalf,
					looking_arc + 255 - gun_pos, RPGGUN + kb_frames[*kb], shade, o | pin, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaychicken = [&]
		{
			if (!(gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) pin = RS_ALIGN_R;
			if (*kb)
			{
				static const uint8_t kb_frames[] = { 0,1,1,2,2,3,2,3,2,3,2,2,2,2,2,2,2,2,2,4,4,4,4,5,5,5,5,6,6,6,6,6,6,7,7,7,7,7,7 };
				if (kb_frames[*kb] == 2 || kb_frames[*kb] == 3)
				{
					rdmyospal((weapon_xoffset + 200) - look_anghalf,
						looking_arc + 250 - gun_pos, RPGGUN2 + kb_frames[*kb], shade, o |  pin, pal);
				}
				else if (kb_frames[*kb] == 1)
				{
					rdmyospal((weapon_xoffset + 200) - look_anghalf,
						looking_arc + 250 - gun_pos, RPGGUN2 + kb_frames[*kb], 0, o |  pin, pal);
				}
				else
				{
					rdmyospal((weapon_xoffset + 210) - look_anghalf,
						looking_arc + 255 - gun_pos, RPGGUN2 + kb_frames[*kb], shade, o |  pin, pal);
				}
			}
			else
			{
				if (ud.multimode < 2)
				{
					if (chickenphase)
					{
						rdmyospal((weapon_xoffset + 210) - look_anghalf,
							looking_arc + 222 - gun_pos, RPGGUN2 + 7, shade, o |  pin, pal);
					}
					else if ((krand() & 15) == 5)
					{
						S_PlayActorSound(327, p->GetActor());
						rdmyospal((weapon_xoffset + 210) - look_anghalf,
							looking_arc + 222 - gun_pos, RPGGUN2 + 7, shade, o |  pin, pal);
						chickenphase = 6;
					}
					else
					{
						rdmyospal((weapon_xoffset + 210) - look_anghalf,
							looking_arc + 225 - gun_pos, RPGGUN2, shade, o |  pin, pal);
					}
				}
				else
				{
					rdmyospal((weapon_xoffset + 210) - look_anghalf,
						looking_arc + 225 - gun_pos, RPGGUN2, shade, o |  pin, pal);
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

			{
				double x;
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
							shade = 0;
						x = weapon_xoffset + ((kb_ox[kb_frames[*kb]] >> 1) - 12);
						y = tm - (244 - kb_oy[kb_frames[*kb]]);
						hud_drawpal(x + 64 - look_anghalf,
							y + looking_arc - gun_pos, SHOTGUN + kb_frames[*kb], shade, 0, pal);
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
						hud_drawpal(x + 64 - look_anghalf, y + looking_arc - gun_pos, SHOTGUN + kb_frames[*kb], shade, 0, pal);
						if (kb_frames[*kb] == 21)
							hud_drawpal(x + 96 - look_anghalf, y + looking_arc - gun_pos, SHOTGUNSHELLS, shade, 0, pal);
					}
				}
				else
				{
					if ((*kb) < 16)
					{
						if (p->shotgun_state[0])
						{
							if (kb_frames2[*kb] == 3 || kb_frames2[*kb] == 4)
								shade = 0;
							x = weapon_xoffset + ((kb_ox[kb_frames2[*kb]] >> 1) - 12);
							y = tm - (244 - kb_oy[kb_frames2[*kb]]);
							hud_drawpal(x + 64 - look_anghalf,
								y + looking_arc - gun_pos, SHOTGUN + kb_frames2[*kb], shade, 0, pal);
						}
						else
						{
							if (kb_frames3[*kb] == 1 || kb_frames3[*kb] == 2)
								shade = 0;
							x = weapon_xoffset + ((kb_ox[kb_frames3[*kb]] >> 1) - 12);
							y = tm - (244 - kb_oy[kb_frames3[*kb]]);
							hud_drawpal(x + 64 - look_anghalf,
								y + looking_arc - gun_pos, SHOTGUN + kb_frames3[*kb], shade, 0, pal);
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
						hud_drawpal(x + 64 - look_anghalf, y + looking_arc - gun_pos, SHOTGUN + kb_frames2[*kb], shade, 0, pal);
						if (kb_frames2[*kb] == 21)
							hud_drawpal(x + 96 - look_anghalf, y + looking_arc - gun_pos, SHOTGUNSHELLS, shade, 0, pal);
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
				gun_pos -= bsinf((*kb) << 7, -12);

			if (*kb > 0 && p->GetActor()->s->pal != 1) weapon_xoffset += 1 - (rand() & 3);

			switch (*kb)
			{
			case 0:
				hud_drawpal(weapon_xoffset + 178 - look_anghalf + 30, looking_arc + 233 - gun_pos + 5,
					CHAINGUN, shade, o, pal);
				break;
			default:
				shade = 0;
				if (*kb < 8)
				{
					i = rand() & 7;
					hud_drawpal(weapon_xoffset + 178 - look_anghalf + 30, looking_arc + 233 - gun_pos + 5,
						CHAINGUN + 1, shade, o, pal);
				}
				else hud_drawpal(weapon_xoffset + 178 - look_anghalf + 30, looking_arc + 233 - gun_pos + 5,
					CHAINGUN + 2, shade, o, pal);
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
				double x;
				short y;

				x = weapon_xoffset + (kb_ox[kb_frames[*kb]] - 12);
				y = 244 - (244 - kb_oy[kb_frames[*kb]]);

				if (kb_frames[*kb])
					shade = 0;

				rdmyospal(x - look_anghalf,
					y + looking_arc - gun_pos, FIRSTGUN + kb_frames[*kb], shade, 0, pal);
			}
			else
			{
				static const short kb_frames[] = { 0,0,1,1,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0 };
				static const short kb_ox[] = { 244,244,244 };
				static const short kb_oy[] = { 256,249,248 };
				double x;
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
				rdmyospal(x - look_anghalf - dx,
					y + looking_arc - gun_pos + dy, FIRSTGUNRELOAD + kb_frames[(*kb) - 22], shade, 0, pal);
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

			rdmyospal(weapon_xoffset + 190 - look_anghalf, looking_arc + 260 - gun_pos, HANDTHROW, shade, o, pal);
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
				if (!(gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) pin = RS_ALIGN_R;
				static const int8_t remote_frames[] = { 1,1,1,1,1,2,2,2,2,3,3,3,4,4,4,5,5,5,5,5,6,6,6 };

				if (*kb)
				{
					if ((*kb) < 5)
					{
						rdmyospal(weapon_xoffset + x + 190 - look_anghalf - dx,
							looking_arc + 258 - gun_pos - 64 + p->detonate_count - dy, RRTILE1752, 0, o |  pin, pal);
					}
					rdmyospal(weapon_xoffset + x + 190 - look_anghalf,
						looking_arc + 258 - gun_pos - dy, HANDTHROW + remote_frames[*kb], shade, o |  pin, pal);
				}
				else
				{
					if ((*kb) < 5)
					{
						rdmyospal(weapon_xoffset + x + 190 - look_anghalf - dx,
							looking_arc + 258 - gun_pos - 64 + p->detonate_count - dy, RRTILE1752, 0, o |  pin, pal);
					}
					rdmyospal(weapon_xoffset + x + 190 - look_anghalf,
						looking_arc + 258 - gun_pos - dy, HANDTHROW + 1, shade, o |  pin, pal);
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
				shade = 0;
				rd3myospal(150 + (weapon_xoffset / 2.) - look_anghalf, 266 + (looking_arc / 2.) - gun_pos, DEVISTATOR, shade, o, pal);
			}
			else
				rd3myospal(150 + (weapon_xoffset / 2.) - look_anghalf, 266 + (looking_arc / 2.) - gun_pos, DEVISTATOR + 1, shade, o, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayblaster = [&]
		{
			if (!(gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) pin = RS_ALIGN_R;
			if ((*kb))
			{
				char cat_frames[] = { 0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
				rdmyospal(weapon_xoffset + 260 - look_anghalf, looking_arc + 215 - gun_pos, FREEZE + cat_frames[*kb], -32, o |  pin, pal);
			}
			else rdmyospal(weapon_xoffset + 260 - look_anghalf, looking_arc + 215 - gun_pos, FREEZE, shade, o |  pin, pal);
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
				rd2myospal(weapon_xoffset + 188 - look_anghalf,
					looking_arc + 240 - gun_pos, SHRINKER, shade, o, pal);
			}
			else
			{
				if (p->GetActor()->s->pal != 1)
				{
					weapon_xoffset += rand() & 3;
					gun_pos += (rand() & 3);
				}

				if (cw == BUZZSAW_WEAPON)
				{
					rd2myospal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, GROWSPARK + ((*kb) & 2), shade, o, 0);
				}
				else
				{
					signed char kb_frames[] = { 1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0 };
					short frm = kb_frames[*kb];
					rd2myospal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + frm, shade, o, 0);
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
