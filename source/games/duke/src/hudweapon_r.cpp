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
// Note: Scaling factors in here are very weird numbers that are better left in Q16.16.
//
//---------------------------------------------------------------------------

inline static void hud_drawpal(double x, double y, int tilenum, int shade, int orientation, int p, DAngle angle, int scale = 32768)
{
	hud_drawsprite(x, y, scale, angle.Degrees(), tilenum, shade, p, 2 | orientation);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displaymasks_r(int snum, int p, double interpfrac)
{
	if (ps[snum].scuba_on)
	{
		auto scuba0 = TexMan.CheckForTexture("SCUBAMASK0", ETextureType::Any);
		auto scuba3 = TexMan.CheckForTexture("SCUBAMASK3", ETextureType::Any);
		auto scuba4 = TexMan.CheckForTexture("SCUBAMASK4", ETextureType::Any);
		auto tex0 = TexMan.GetGameTexture(scuba0);
		auto tex4 = TexMan.GetGameTexture(scuba4);
		//int pin = 0;
		// to get the proper clock value with regards to interpolation we have add a interpfrac based offset to the value.
		double interpclock = PlayClock + TICSPERFRAME * interpfrac;
		int pin = RS_STRETCH;
		hud_drawsprite((320 - (tex0->GetDisplayWidth() * 0.5) - 15), (200 - (tex0->GetDisplayHeight() * 0.5) + BobVal(interpclock) * 16), 0.75, 0, scuba0, 0, p, 2 + 16 + pin);
		hud_drawsprite((320 - tex4->GetDisplayWidth()), (200 - tex4->GetDisplayHeight()), 1., 0, scuba4, 0, p, 2 + 16 + pin);
		hud_drawsprite(tex4->GetDisplayWidth(), (200 - tex4->GetDisplayHeight()), 1., 0, scuba4, 0, p, 2 + 4 + 16 + pin);
		hud_drawsprite(35, (-1), 1., 0, scuba3, 0, p, 2 + 16 + pin);
		hud_drawsprite(285, 200, 1., -180, scuba3, 0, p, 2 + 16 + pin);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawMotorCycle(int const kb, DVector2 offsets, DAngle angle, int shade, int pal)
{
	const char* temp_kb;
	if (numplayers == 1)
	{
		if (kb)
		{
			shade = 0;
			if (kb == 1)
			{
				if ((krand() & 1) == 1)
					temp_kb = "MOTOHIT1";
				else
					temp_kb = "MOTOHIT2";
			}
			else if (kb == 4)
			{
				if ((krand() & 1) == 1)
					temp_kb = "MOTOHIT3";
				else
					temp_kb = "MOTOHIT4";
			}
			else
				temp_kb = "MOTOHIT0";

		}
		else
			temp_kb = "MOTOHIT0";
	}
	else
	{
		if (kb)
		{
			shade = 0;
			if (kb == 1)
				temp_kb = "MOTOHIT1";
			else if (kb == 2)
				temp_kb = "MOTOHIT2";
			else if (kb == 3)
				temp_kb = "MOTOHIT3";
			else if (kb == 4)
				temp_kb = "MOTOHIT4";
			else
				temp_kb = "MOTOHIT0";

		}
		else
			temp_kb = "MOTOHIT0";
	}
	hud_drawsprite(160 + offsets.X, 174, 0.53125, -angle.Degrees(), TexMan.CheckForTexture(temp_kb, ETextureType::Any), shade, pal, 0);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawBoat(int const kb, DVector2 offsets, DAngle angle, int shade, int pal, bool notonwater)
{
	const char* temp_kb;
	int temp2, temp3;
	temp2 = 0;
	if (angle > nullAngle)
	{
		if (kb == 0)
			temp_kb = "BOATHIT1";
		else if (kb <= 3)
		{
			temp_kb = "BOATHIT5";
			temp2 = 1;
		}
		else if (kb <= 6)
		{
			temp_kb = "BOATHIT6";
			temp2 = 1;
		}
		else
			temp_kb = "BOATHIT1";
	}
	else if (angle < nullAngle)
	{
		if (kb == 0)
			temp_kb = "BOATHIT2";
		else if (kb <= 3)
		{
			temp_kb = "BOATHIT7";
			temp2 = 1;
		}
		else if (kb <= 6)
		{
			temp_kb = "BOATHIT8";
			temp2 = 1;
		}
		else
			temp_kb = "BOATHIT2";
	}
	else
	{
		if (kb == 0)
			temp_kb = "BOATHIT0";
		else if (kb <= 3)
		{
			temp_kb = "BOATHIT3";
			temp2 = 1;
		}
		else if (kb <= 6)
		{
			temp_kb = "BOATHIT4";
			temp2 = 1;
		}
		else
			temp_kb = "BOATHIT0";
	}

	if (notonwater)
		temp3 = 170;
	else
		temp3 = 170 + (kb >> 2);

	if (temp2)
		shade = -96;

	hud_drawsprite(160 + offsets.X, temp3, 1.0078125, -angle.Degrees(), TexMan.CheckForTexture(temp_kb, ETextureType::Any), shade, pal, 0);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------
void animateshrunken(player_struct* p, double xoffset, double yoffset, int8_t shade, int o, double interpfrac);

void displayweapon_r(int snum, double interpfrac)
{
	double weapon_sway, gun_pos, hard_landing;
	DAngle TiltStatus;

	auto p = &ps[snum];
	auto kb = &p->kickback_pic;

	int o = 0;

	if (cl_hudinterpolation)
	{
		weapon_sway = interpolatedvalue<double>(p->oweapon_sway, p->weapon_sway, interpfrac);
		hard_landing = interpolatedvalue<double>(p->ohard_landing, p->hard_landing, interpfrac);
		gun_pos = 80 - interpolatedvalue<double>(p->oweapon_pos * p->oweapon_pos, p->weapon_pos * p->weapon_pos, interpfrac);
		TiltStatus = interpolatedvalue(p->oTiltStatus, p->TiltStatus, interpfrac);
	}
	else
	{
		weapon_sway = p->weapon_sway;
		hard_landing = p->hard_landing;
		gun_pos = 80 - (p->weapon_pos * p->weapon_pos);
		TiltStatus = p->TiltStatus;
	}

	hard_landing *= 8.;
	gun_pos -= fabs(p->GetActor()->spr.scale.X < 0.125 ? BobVal(weapon_sway * 4.) * 32 : BobVal(weapon_sway * 0.5) * 16) + hard_landing;

	auto offpair = p->Angles.getWeaponOffsets(interpfrac);
	auto offsets = offpair.first;
	auto angle = offpair.second;
	auto weapon_xoffset = 160 - 90 - (BobVal(512 + weapon_sway * 0.5) * (16384. / 1536.)) - 58 - p->weapon_ang;
	auto shade = min(p->insector() && p->cursector->shadedsector == 1 ? 16 : p->GetActor()->spr.shade, 24);
	auto pal = !p->insector()? 0 : p->GetActor()->spr.pal == 1? 1 : p->cursector->floorpal;
	auto cw = p->last_weapon >= 0 ? p->last_weapon : p->curr_weapon;

	if (p->newOwner != nullptr || ud.cameraactor != nullptr || p->over_shoulder_on > 0 || (p->GetActor()->spr.pal != 1 && p->GetActor()->spr.extra <= 0))
		return;

	if ((14 - p->quick_kick) != 14)
	{
		pal = p->GetActor()->spr.pal == 1 ? 1 : p->palookup;
	}

	if (p->OnMotorcycle)
	{
		DrawMotorCycle(*kb, offsets, TiltStatus, shade, pal);
		return;
	}
	if (p->OnBoat)
	{
		DrawBoat(*kb, offsets, TiltStatus, shade, pal, p->NotOnWater);
		return;
	}

	offsets.X += weapon_xoffset;
	offsets.Y -= gun_pos;

	if (p->GetActor()->spr.scale.X < 0.125)
	{
		animateshrunken(p, offsets.X, offsets.Y + gun_pos, shade, o, interpfrac);
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
			static const uint8_t kb_frames[] = { 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 };
			static const uint16_t kb_ox[] = { 310,342,364,418,350,316,282,288,0,0 };
			static const uint16_t kb_oy[] = { 300,362,320,268,248,248,277,420,0,0 };

			double x = ((kb_ox[kb_frames[*kb]] >> 1) - 12) + offsets.X;
			double y = 200 - (244 - kb_oy[kb_frames[*kb]]) + offsets.Y;
			hud_drawpal(x, y, RTILE_KNEE + kb_frames[*kb], shade, 0, pal, angle);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayslingblade = [&]
		{
			static const uint8_t kb_frames[] = { 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 };
			static const uint16_t kb_ox[] = { 580,676,310,491,356,210,310,614 };
			static const uint16_t kb_oy[] = { 369,363,300,323,371,400,300,440 };

			double x = ((kb_ox[kb_frames[*kb]] >> 1) - 12) + 20 + offsets.X;
			double y = 210 - (244 - kb_oy[kb_frames[*kb]]) - 80 + offsets.Y;
			hud_drawpal(x, y, RTILE_SLINGBLADE + kb_frames[*kb], shade, 0, pal, angle);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaybowlingball = [&]
		{
			offsets.X += 8;
			offsets.Y += 10;

			if (p->ammo_amount[BOWLING_WEAPON])
			{
				hud_drawpal(162 + offsets.X, 214 + offsets.Y + (*kb << 3), RTILE_BOWLINGBALLHUD, shade, o, pal, angle);
			}
			else
			{
				hud_drawpal(162 + offsets.X, 214 + offsets.Y, RTILE_HANDTHROW + 5, shade, o, pal, angle, 36700);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaypowderkeg = [&]
		{
			offsets.X += 8;
			offsets.Y += 10;

			if (p->ammo_amount[POWDERKEG_WEAPON])
			{
				hud_drawpal(180 + offsets.X, 214 + offsets.Y + (*kb << 3), RTILE_POWDERH, shade, o, pal, angle, 36700);
				hud_drawpal(90 + offsets.X, 214 + offsets.Y + (*kb << 3), RTILE_POWDERH, shade, o | 4, pal, angle, 36700);
			}
			else
			{
				hud_drawpal(162 + offsets.X, 214 + offsets.Y, RTILE_HANDTHROW + 5, shade, o, pal, angle, 36700);
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
				hud_drawpal(200 + offsets.X, 250 + offsets.Y, RTILE_RPGGUN + kb_frames[*kb], shade, o | pin, pal, angle, 36700);
			}
			else if (kb_frames[*kb] == 1)
			{
				hud_drawpal(200 + offsets.X, 250 + offsets.Y, RTILE_RPGGUN + kb_frames[*kb], 0, o | pin, pal, angle, 36700);
			}
			else
			{
				hud_drawpal(210 + offsets.X, 255 + offsets.Y, RTILE_RPGGUN + kb_frames[*kb], shade, o | pin, pal, angle, 36700);
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
					hud_drawpal(200 + offsets.X, 250 + offsets.Y, RTILE_RPGGUN2 + kb_frames[*kb], shade, o |  pin, pal, angle, 36700);
				}
				else if (kb_frames[*kb] == 1)
				{
					hud_drawpal(200 + offsets.X, 250 + offsets.Y, RTILE_RPGGUN2 + kb_frames[*kb], 0, o |  pin, pal, angle, 36700);
				}
				else
				{
					hud_drawpal(210 + offsets.X, 255 + offsets.Y, RTILE_RPGGUN2 + kb_frames[*kb], shade, o |  pin, pal, angle, 36700);
				}
			}
			else
			{
				if (ud.multimode < 2)
				{
					if (chickenphase)
					{
						hud_drawpal(210 + offsets.X, 222 + offsets.Y, RTILE_RPGGUN2 + 7, shade, o |  pin, pal, angle, 36700);
					}
					else if ((krand() & 15) == 5)
					{
						S_PlayActorSound(327, p->GetActor());
						hud_drawpal(210 + offsets.X, 222 + offsets.Y, RTILE_RPGGUN2 + 7, shade, o |  pin, pal, angle, 36700);
						chickenphase = 6;
					}
					else
					{
						hud_drawpal(210 + offsets.X, 225 + offsets.Y, RTILE_RPGGUN2, shade, o |  pin, pal, angle, 36700);
					}
				}
				else
				{
					hud_drawpal(210 + offsets.X, 225 + offsets.Y, RTILE_RPGGUN2, shade, o |  pin, pal, angle, 36700);
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
			offsets.X -= 8;

			double x;
			double y;

			static const uint8_t kb_frames3[] = { 0,0,1,1,2,2,5,5,6,6,7,7,8,8,0,0,0,0,0,0,0 };
			static const uint8_t kb_frames2[] = { 0,0,3,3,4,4,5,5,6,6,7,7,8,8,0,0,20,20,21,21,21,21,20,20,20,20,0,0 };
			static const uint8_t kb_frames[] = { 0,0,1,1,2,2,3,3,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,0,0,20,20,21,21,21,21,20,20,20,20,0,0 };
			static const uint16_t kb_ox[] = { 300,300,300,300,300,330,320,310,305,306,302 };
			static const uint16_t kb_oy[] = { 315,300,302,305,302,302,303,306,302,404,384 };
			int tm = 180;

			if (p->shotgun_state[1])
			{
				if ((*kb) < 26)
				{
					if (kb_frames[*kb] == 3 || kb_frames[*kb] == 4)
						shade = 0;
					x = ((kb_ox[kb_frames[*kb]] >> 1) - 12) + offsets.X;
					y = tm - (244 - kb_oy[kb_frames[*kb]]) + offsets.Y;
					hud_drawpal(x + 64,	y, RTILE_SHOTGUN + kb_frames[*kb], shade, 0, pal, angle);
				}
				else
				{
					if (kb_frames[*kb] > 0)
					{
						x = ((kb_ox[kb_frames[(*kb) - 11]] >> 1) - 12) + offsets.X;
						y = tm - (244 - kb_oy[kb_frames[(*kb) - 11]]) + offsets.Y;
					}
					else
					{
						x = ((kb_ox[kb_frames[*kb]] >> 1) - 12) + offsets.X;
						y = tm - (244 - kb_oy[kb_frames[*kb]]) + offsets.Y;
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
					hud_drawpal(x + 64, y, RTILE_SHOTGUN + kb_frames[*kb], shade, 0, pal, angle);
					if (kb_frames[*kb] == 21)
						hud_drawpal(x + 96, y, RTILE_SHOTGUNSHELLS, shade, 0, pal, angle);
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
						x = ((kb_ox[kb_frames2[*kb]] >> 1) - 12) + offsets.X;
						y = tm - (244 - kb_oy[kb_frames2[*kb]]) + offsets.Y;
						hud_drawpal(x + 64, y, RTILE_SHOTGUN + kb_frames2[*kb], shade, 0, pal, angle);
					}
					else
					{
						if (kb_frames3[*kb] == 1 || kb_frames3[*kb] == 2)
							shade = 0;
						x = ((kb_ox[kb_frames3[*kb]] >> 1) - 12) + offsets.X;
						y = tm - (244 - kb_oy[kb_frames3[*kb]]) + offsets.Y;
						hud_drawpal(x + 64, y, RTILE_SHOTGUN + kb_frames3[*kb], shade, 0, pal, angle);
					}
				}
				else if (p->shotgun_state[0])
				{
					if (kb_frames2[*kb] > 0)
					{
						x = ((kb_ox[kb_frames2[(*kb) - 11]] >> 1) - 12) + offsets.X;
						y = tm - (244 - kb_oy[kb_frames2[(*kb) - 11]]) + offsets.Y;
					}
					else
					{
						x = ((kb_ox[kb_frames2[*kb]] >> 1) - 12) + offsets.X;
						y = tm - (244 - kb_oy[kb_frames2[*kb]]) + offsets.Y;
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
					hud_drawpal(x + 64, y, RTILE_SHOTGUN + kb_frames2[*kb], shade, 0, pal, angle);
					if (kb_frames2[*kb] == 21)
						hud_drawpal(x + 96, y, RTILE_SHOTGUNSHELLS, shade, 0, pal, angle);
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
				offsets.Y += BobVal((*kb) << 7) * 4;

			if (*kb > 0 && p->GetActor()->spr.pal != 1) offsets.X += 1 - (rand() & 3);

			switch (*kb)
			{
			case 0:
				hud_drawpal(208 + offsets.X, 238 + offsets.Y, RTILE_CHAINGUN, shade, o, pal, angle);
				break;
			default:
				shade = 0;
				if (*kb < 8)
				{
					hud_drawpal(208 + offsets.X, 238 + offsets.Y, RTILE_CHAINGUN + 1, shade, o, pal, angle);
				}
				else hud_drawpal(208 + offsets.X, 238 + offsets.Y, RTILE_CHAINGUN + 2, shade, o, pal, angle);
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
			double x, y;

			if ((*kb) < 22)
			{
				static const uint8_t kb_frames[] = { 0,0,1,1,2,2,3,3,4,4,6,6,6,6,5,5,4,4,3,3,0,0 };
				static const uint16_t kb_ox[] = { 194,190,185,208,215,215,216,216,201,170 };
				static const uint16_t kb_oy[] = { 256,249,248,238,228,218,208,256,245,258 };

				x = (kb_ox[kb_frames[*kb]] - 12) + offsets.X;
				y = 244 - (244 - kb_oy[kb_frames[*kb]]) + offsets.Y;

				if (kb_frames[*kb])
					shade = 0;

				hud_drawpal(x, y, RTILE_FIRSTGUN + kb_frames[*kb], shade, 0, pal, angle, 36700);
			}
			else
			{
				static const uint8_t kb_frames[] = { 0,0,1,1,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0 };
				static const uint16_t kb_ox[] = { 244,244,244 };
				static const uint16_t kb_oy[] = { 256,249,248 };

				x = (kb_ox[kb_frames[(*kb) - 22]] - 12) + offsets.X;
				y = 244 - (244 - kb_oy[kb_frames[(*kb) - 22]]) + offsets.Y;

				int dx, dy;

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
				hud_drawpal(x - dx, y + dy, RTILE_FIRSTGUNRELOAD + kb_frames[(*kb) - 22], shade, 0, pal, angle, 36700);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaydynamite = [&]
		{
			offsets.Y += 9 * (*kb);

			hud_drawpal(190 + offsets.X, 260 + offsets.Y, RTILE_HANDTHROW, shade, o, pal, angle, 36700);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaythrowingdynamite = [&]
		{
			int dx = 25;
			int dy = 20;

			if ((*kb) < 20)
			{
				if (!(gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) pin = RS_ALIGN_R;
				static const int8_t remote_frames[] = { 1,1,1,1,1,2,2,2,2,3,3,3,4,4,4,5,5,5,5,5,6,6,6 };

				if (*kb)
				{
					if ((*kb) < 5)
					{
						hud_drawpal(290 + offsets.X - dx, 258 + offsets.Y - 64 + p->detonate_count - dy, RTILE_RRTILE1752, 0, o |  pin, pal, angle, 36700);
					}
					hud_drawpal(290 + offsets.X, 258 + offsets.Y - dy, RTILE_HANDTHROW + remote_frames[*kb], shade, o |  pin, pal, angle, 36700);
				}
				else
				{
					if ((*kb) < 5)
					{
						hud_drawpal(290 + offsets.X - dx, 258 + offsets.Y - 64 + p->detonate_count - dy, RTILE_RRTILE1752, 0, o |  pin, pal, angle, 36700);
					}
					hud_drawpal(290 + offsets.X, 258 + offsets.Y - dy, RTILE_HANDTHROW + 1, shade, o |  pin, pal, angle, 36700);
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
			offsets.X -= weapon_xoffset * 0.5;

			if (*kb)
			{
				shade = 0;
				hud_drawpal(150 + offsets.X, 266 + offsets.Y, RTILE_DEVISTATOR, shade, o, pal, angle, 47040);
			}
			else
				hud_drawpal(150 + offsets.X, 266 + offsets.Y, RTILE_DEVISTATOR + 1, shade, o, pal, angle, 47040);
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
				static const uint8_t cat_frames[] = { 0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
				hud_drawpal(260 + offsets.X, 215 + offsets.Y, RTILE_FREEZE + cat_frames[*kb], -32, o |  pin, pal, angle, 36700);
			}
			else hud_drawpal(260 + offsets.X, 215 + offsets.Y, RTILE_FREEZE, shade, o |  pin, pal, angle, 36700);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaysaw = [&]
		{
			offsets.X += 28;
			offsets.Y += 18;

			if ((*kb) == 0)
			{
				hud_drawpal(188 + offsets.X, 240 + offsets.Y, RTILE_SHRINKER, shade, o, pal, angle, 44040);
			}
			else
			{
				if (p->GetActor()->spr.pal != 1)
				{
					offsets.X += rand() & 3;
					offsets.Y -= rand() & 3;
				}

				if (cw == BUZZSAW_WEAPON)
				{
					hud_drawpal(184 + offsets.X, 240 + offsets.Y, RTILE_GROWSPARK + ((*kb) & 2), shade, o, 0, angle, 44040);
				}
				else
				{
					static const int8_t kb_frames[] = { 1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0 };
					hud_drawpal(184 + offsets.X, 240 + offsets.Y, RTILE_SHRINKER + kb_frames[*kb], shade, o, 0, angle, 44040);
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
