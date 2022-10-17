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
aint with this program; if not, write to the Free Software
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
#include "names_d.h"
#include "dukeactor.h"

BEGIN_DUKE_NS 

inline static double getavel(int snum)
{
	return PlayerInputAngVel(snum) * (2048. / 360.);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

inline static void hud_drawpal(double x, double y, int tilenum, int shade, int orientation, int p, DAngle angle)
{
	hud_drawsprite(x, y, 65536, angle.Buildfang(), tilenum, shade, p, 2 | orientation);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void displayloogie(player_struct* p, double const interpfrac)
{
	if (p->loogcnt == 0) return;

	const double loogi = interpolatedvalue<double>(p->oloogcnt, p->loogcnt, interpfrac);
	const double y = loogi * 4.;

	for (int i = 0; i < p->numloogs; i++)
	{
		const double a = fabs(BobVal((loogi + i) * 32.) * 512);
		const double z = 4096. + ((loogi + i) * 512.);
		const double x = -getavel(p->GetPlayerNum()) + BobVal((loogi + i) * 64.) * 16;

		hud_drawsprite((p->loogie[i].X + x), (200 + p->loogie[i].Y - y), z - (i << 8), 256 - a, LOOGIE, 0, 0, 2);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool animatefist(int gs, player_struct* p, double xoffset, double yoffset, int fistpal, double const interpfrac)
{
	const double fisti = min(interpolatedvalue<double>(p->ofist_incs, p->fist_incs, interpfrac), 32.);
	if (fisti <= 0) return false;

	hud_drawsprite(
		(-fisti + 222 + xoffset),
		(yoffset + 194 + BobVal((6 + fisti) * 128.) * 32),
		clamp(65536. - 65536. * BobVal(512 + fisti * 64.), 40920., 90612.), 0, FIST, gs, fistpal, 2);

	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool animateknee(int gs, player_struct* p, double xoffset, double yoffset, int pal, double const interpfrac, DAngle angle)
{
	if (p->knee_incs > 11 || p->knee_incs == 0 || p->GetActor()->spr.extra <= 0) return false;

	static const int8_t knee_y[] = { 0,-8,-16,-32,-64,-84,-108,-108,-108,-72,-32,-8 };
	const double kneei = interpolatedvalue<double>(knee_y[p->oknee_incs], knee_y[p->knee_incs], interpfrac);

	hud_drawpal(105 + (kneei * 0.25) + xoffset, 280 + kneei + yoffset, KNEE, gs, 4, pal, angle);

	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool animateknuckles(int gs, player_struct* p, double xoffset, double yoffset, int pal, DAngle angle)
{
	if (isWW2GI() || p->over_shoulder_on != 0 || p->knuckle_incs == 0 || p->GetActor()->spr.extra <= 0) return false;

	static const uint8_t knuckle_frames[] = { 0,1,2,2,3,3,3,2,2,1,0 };

	hud_drawpal(160 + xoffset, 180 + yoffset, CRACKKNUCKLES + knuckle_frames[p->knuckle_incs >> 1], gs, 4, pal, angle);

	return true;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displaymasks_d(int snum, int p, double interpfrac)
{
	if (ps[snum].scuba_on)
	{
		int y = 200 - tileHeight(SCUBAMASK);
		hud_drawsprite(44, y, 65536, 0, SCUBAMASK, 0, p, 2 + 16);
		hud_drawsprite((320 - 43), y, 65536, 0, SCUBAMASK, 0, p, 2 + 4 + 16);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool animatetip(int gs, player_struct* p, double xoffset, double yoffset, int pal, double const interpfrac, DAngle angle)
{
	if (p->tipincs == 0) return false;

	static const int8_t tip_y[] = { 0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16 };
	const double tipi = interpolatedvalue<double>(tip_y[p->otipincs], tip_y[p->tipincs], interpfrac) * 0.5;

	hud_drawpal(170 + xoffset, 240 + tipi + yoffset, TIP + ((26 - p->tipincs) >> 4), gs, 0, pal, angle);

	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool animateaccess(int gs, player_struct* p, double xoffset, double yoffset, double const interpfrac, DAngle angle)
{
	if (p->access_incs == 0 || p->GetActor()->spr.extra <= 0) return false;

	static const int8_t access_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16};
	const double accessi = interpolatedvalue<double>(access_y[p->oaccess_incs], access_y[p->access_incs], interpfrac);

	const int pal = p->access_spritenum != nullptr ? p->access_spritenum->spr.pal : 0;

	if ((p->access_incs-3) > 0 && (p->access_incs-3)>>3)
		hud_drawpal(170 + (accessi * 0.25) + xoffset, 266 + accessi + yoffset, HANDHOLDINGLASER + (p->access_incs >> 3), gs, 0, pal, angle);
	else
		hud_drawpal(170 + (accessi * 0.25) + xoffset, 266 + accessi + yoffset, HANDHOLDINGACCESS, gs, 4, pal, angle);

	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displayweapon_d(int snum, double interpfrac)
{
	int pal, pal2;
	player_struct* p = &ps[snum];

	if (p->newOwner != nullptr || ud.cameraactor != nullptr || p->over_shoulder_on > 0 || (p->GetActor()->spr.pal != 1 && p->GetActor()->spr.extra <= 0))
		return;

	double weapon_sway, gun_pos, kickback_pic, random_club_frame, hard_landing;
	auto kb = &p->kickback_pic;
	int o = 0;

	if (cl_hudinterpolation)
	{
		weapon_sway = interpolatedvalue<double>(p->oweapon_sway, p->weapon_sway, interpfrac);
		kickback_pic = interpolatedvalue<double>(p->okickback_pic, p->kickback_pic, interpfrac);
		random_club_frame = interpolatedvalue<double>(p->orandom_club_frame, p->random_club_frame, interpfrac);
		hard_landing = interpolatedvalue<double>(p->ohard_landing, p->hard_landing, interpfrac);
		gun_pos = 80 - interpolatedvalue<double>(p->oweapon_pos * p->oweapon_pos, p->weapon_pos * p->weapon_pos, interpfrac);
	}
	else
	{
		weapon_sway = p->weapon_sway;
		kickback_pic = p->kickback_pic;
		random_club_frame = p->random_club_frame;
		hard_landing = p->hard_landing;
		gun_pos = 80 - (p->weapon_pos * p->weapon_pos);
	}

	hard_landing *= 8.;
	gun_pos -= fabs(p->GetActor()->spr.scale.X < 0.5 ? BobVal(weapon_sway * 4.) * 32 : BobVal(weapon_sway * 0.5) * 16) + hard_landing;

	auto offsets = p->angle.weaponoffsets(interpfrac);
	auto horiz = !SyncInput() ? p->horizon.sum() : p->horizon.interpolatedsum(interpfrac);
	auto pitchoffset = interpolatedvalue(0., 16., horiz / DAngle90);
	auto yawinput = getavel(snum) * (1. / 16.);
	auto angle = p->angle.renderrotscrn(interpfrac);
	auto weapon_xoffset = 160 - 90 - (BobVal(512 + weapon_sway * 0.5) * (16384. / 1536.)) - 58 - p->weapon_ang;
	auto shade = min(p->GetActor()->spr.shade, (int8_t)24);

	pal2 = pal = !p->insector() ? 0 : p->GetActor()->spr.pal == 1 ? 1 : p->cursector->floorpal;
	if (pal2 == 0) pal2 = p->palookup;

	auto animoffs = offsets + DVector2(yawinput, -hard_landing + pitchoffset);

	if (animatefist(shade, p, yawinput, offsets.Y, pal, interpfrac))
		return;
	if (animateknuckles(shade, p, animoffs.X, animoffs.Y, pal, angle))
		return;
	if (animatetip(shade, p, animoffs.X, animoffs.Y, pal, interpfrac, angle))
		return;
	if (animateaccess(shade, p, animoffs.X, animoffs.Y, interpfrac, angle))
		return;

	animateknee(shade, p, animoffs.X, animoffs.Y, pal2, interpfrac, angle);

	int cw = p->last_weapon >= 0 ? p->last_weapon : p->curr_weapon;
	if (isWW2GI()) cw = aplWeaponWorksLike(cw, snum);

	// onevent should go here..
	// rest of code should be moved to CON..
	int quick_kick = 14 - p->quick_kick;

	if (quick_kick != 14 || p->last_quick_kick)
	{
		if (quick_kick < 5 || quick_kick > 9)
		{
			hud_drawpal(weapon_xoffset + 80 + offsets.X, 250 - gun_pos + offsets.Y, KNEE, shade, o | 4, pal2, angle);
		}
		else
		{
			hud_drawpal(weapon_xoffset + 160 - 16 + offsets.X, 214 - gun_pos + offsets.Y, KNEE + 1, shade, o | 4, pal2, angle);
		}
	}

	if (p->GetActor()->spr.scale.X < 0.625)
	{
		//shrunken..
		animateshrunken(p, weapon_xoffset, offsets.Y, -offsets.X, FIST, shade, o, interpfrac);
	}
	else
	{
		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayknee = [&]()
		{
			offsets.X += weapon_xoffset;
			offsets.Y -= gun_pos;

			if (*kb > 0)
			{
				if (*kb < 5 || *kb > 9)
				{
					hud_drawpal(220 + offsets.X, 250 + offsets.Y, KNEE, shade, o, pal2, angle);
				}
				else
				{
					hud_drawpal(160 + offsets.X, 214 + offsets.Y, KNEE + 1, shade, o, pal2, angle);
				}
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaytripbomb = [&]()
		{
			offsets.X += weapon_xoffset + 8;
			offsets.Y -= gun_pos + 10;

			if (*kb > 6)
				offsets.Y += kickback_pic * 8.;
			else if (*kb < 4)
				hud_drawpal(142 + offsets.X, 234 + offsets.Y, HANDHOLDINGLASER + 3, shade, o, pal, angle);

			hud_drawpal(130 + offsets.X, 249 + offsets.Y, HANDHOLDINGLASER + (*kb >> 2), shade, o, pal, angle);
			hud_drawpal(152 + offsets.X, 249 + offsets.Y, HANDHOLDINGLASER + (*kb >> 2), shade, o | 4, pal, angle);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayrpg = [&]()
		{
			const int pin = ((gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;

			const auto xyoffset = BobVal(768 + (kickback_pic * 128.)) * 8;
			offsets.X += weapon_xoffset - xyoffset;
			offsets.Y -= gun_pos + xyoffset;

			if (*kb > 0)
			{
				if (*kb < (isWW2GI() ? aplWeaponTotalTime(RPG_WEAPON, snum) : 8))
				{
					hud_drawpal(164 + offsets.X, 176 + offsets.Y, RPGGUN + (*kb >> 1), shade, o | pin, pal, angle);
				}
				else if (isWW2GI())
				{
					// else we are in 'reload time'
					if (*kb < ((aplWeaponReload(p->curr_weapon, snum) - aplWeaponTotalTime(p->curr_weapon, snum)) / 2 + aplWeaponTotalTime(p->curr_weapon, snum)))
					{
						// down 
						offsets.Y += 10 * (kickback_pic - aplWeaponTotalTime(p->curr_weapon, snum)); //D
					}
					else
					{
						// up and left
						offsets.Y += 10 * (aplWeaponReload(p->curr_weapon, snum) - kickback_pic); //U
					}
				}
			}

			hud_drawpal(164 + offsets.X, 176 + offsets.Y, RPGGUN, shade, o | pin, pal, angle);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshotgun_ww = [&]()
		{
			offsets.X += weapon_xoffset - 8;
			offsets.Y -= gun_pos;

			if (*kb > 0)
				offsets.Y += BobVal(kickback_pic * 128.) * 4;

			if (*kb > 0 && p->GetActor()->spr.pal != 1)
				offsets.X += 1 - (rand() & 3);

			int pic = SHOTGUN;

			if (*kb == 0)
			{
				// Just fall through here.
			}
			else if (*kb <= aplWeaponTotalTime(SHOTGUN_WEAPON, snum))
			{
				pic += 1;
			}
			// else we are in 'reload time'
			else if (*kb < ((aplWeaponReload(p->curr_weapon, snum) - aplWeaponTotalTime(p->curr_weapon, snum)) / 2 + aplWeaponTotalTime(p->curr_weapon, snum)))
			{
				// down 
				offsets.Y += 10 * (kickback_pic - aplWeaponTotalTime(p->curr_weapon, snum)); //D
			}
			else
			{
				// up and left
				offsets.Y += 10 * (aplWeaponReload(p->curr_weapon, snum) - kickback_pic); //U
			}

			hud_drawpal(146 + offsets.X, 202 + offsets.Y, pic, shade, o, pal, angle);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshotgun = [&]()
		{
			offsets.X += weapon_xoffset - 8;
			offsets.Y -= gun_pos;

			switch(*kb)
			{
				case 1:
				case 2:
					hud_drawpal(168 + offsets.X, 201 + offsets.Y, SHOTGUN + 2, -128, o, pal, angle);
					[[fallthrough]];
				case 0:
				case 6:
				case 7:
				case 8:
					hud_drawpal(146 + offsets.X, 202 + offsets.Y, SHOTGUN, shade, o, pal, angle);
					break;
				case 3:
				case 4:
				case 5:
				case 9:
				case 10:
				case 11:
				case 12:
					if (*kb > 1 && *kb < 5)
					{
						offsets.Y += 40;
						offsets.X += 20;

						hud_drawpal(178 + offsets.X, 194 + offsets.Y, SHOTGUN + 1 + ((*(kb)-1) >> 1), -128, o, pal, angle);
					}
					hud_drawpal(158 + offsets.X, 220 + offsets.Y, SHOTGUN + 3, shade, o, pal, angle);
					break;
				case 13:
				case 14:
				case 15:
					hud_drawpal(198 + offsets.X, 210 + offsets.Y, SHOTGUN + 4, shade, o, pal, angle);
					break;
				case 16:
				case 17:
				case 18:
				case 19:
					hud_drawpal(234 + offsets.X, 196 + offsets.Y, SHOTGUN + 5, shade, o, pal, angle);
					break;
				case 20:
				case 21:
				case 22:
				case 23:
					hud_drawpal(240 + offsets.X, 196 + offsets.Y, SHOTGUN + 6, shade, o, pal, angle);
					break;
				case 24:
				case 25:
				case 26:
				case 27:
					hud_drawpal(234 + offsets.X, 196 + offsets.Y, SHOTGUN + 5, shade, o, pal, angle);
					break;
				case 28:
				case 29:
				case 30:
					hud_drawpal(188 + offsets.X, 206 + offsets.Y, SHOTGUN + 4, shade, o, pal, angle);
					break;
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaychaingun_ww = [&]()
		{
			offsets.X += weapon_xoffset;
			offsets.Y -= gun_pos;

			if (*kb > 0)
				offsets.Y += BobVal(kickback_pic * 128.) * 4;

			if (*kb > 0 && p->GetActor()->spr.pal != 1)
				offsets.X += 1 - (rand() & 3);

			if (*kb == 0)
			{
				hud_drawpal(178 + offsets.X, 233 + offsets.Y, CHAINGUN + 1, shade, o, pal, angle);
			}
			else if (*kb <= aplWeaponTotalTime(CHAINGUN_WEAPON, snum))
			{
				hud_drawpal(188 + offsets.X, 243 + offsets.Y, CHAINGUN + 2, shade, o, pal, angle);
			}
			else
			{
				// else we are in 'reload time', divide reload time into fifths.
				// 1) move weapon up/right, hand on clip (2519)
				// 2) move weapon up/right, hand removing clip (2518)
				// 3) hold weapon up/right, hand removed clip (2517)
				// 4) hold weapon up/right, hand inserting clip (2518)
				// 5) move weapon down/left, clip inserted (2519)

				double adj;
				int pic;
				const int iFifths = max((aplWeaponReload(p->curr_weapon, snum) - aplWeaponTotalTime(p->curr_weapon, snum)) / 5, 1);

				if (*kb < (iFifths + aplWeaponTotalTime(p->curr_weapon, snum)))
				{
					// first segment
					pic = 2519;
					adj = 80 - (10 * (aplWeaponTotalTime(p->curr_weapon, snum) + iFifths - kickback_pic));
				}
				else if (*kb < (iFifths * 2 + aplWeaponTotalTime(p->curr_weapon, snum)))
				{
					// second segment (down)
					pic = 2518;
					adj = 80;
				}
				else if (*kb < (iFifths * 3 + aplWeaponTotalTime(p->curr_weapon, snum)))
				{
					// third segment (up)
					pic = 2517;
					adj = 80;
				}
				else if (*kb < (iFifths * 4 + aplWeaponTotalTime(p->curr_weapon, snum)))
				{
					// fourth segment (down)
					pic = 2518;
					adj = 80;
				}
				else
				{
					// up and left
					pic = 2519;
					adj = 10 * (aplWeaponReload(p->curr_weapon, snum) - kickback_pic);
				}

				hud_drawpal(168 + offsets.X + adj, 260 + offsets.Y - adj, pic, shade, o, pal, angle);
			}

		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaychaingun = [&]
		{
			offsets.X += weapon_xoffset;
			offsets.Y -= gun_pos;

			if (*kb > 0)
				offsets.Y += BobVal(kickback_pic * 128.) * 4;

			if (*kb > 0 && p->GetActor()->spr.pal != 1)
				offsets.X += 1 - (rand() & 3);

			hud_drawpal(168 + offsets.X, 260 + offsets.Y, CHAINGUN, shade, o, pal, angle);

			switch(*kb)
			{
				case 0:
					hud_drawpal(178 + offsets.X, 233 + offsets.Y, CHAINGUN + 1, shade, o, pal, angle);
					break;
				default:
					if (*kb > 4 && *kb < 12)
					{
						auto rnd = p->GetActor()->spr.pal != 1 ? rand() & 7 : 0;
						hud_drawpal(136 + offsets.X + rnd, 208 + offsets.Y + rnd - (kickback_pic * 0.5), CHAINGUN + 5 + ((*kb - 4) / 5), shade, o, pal, angle);

						if (p->GetActor()->spr.pal != 1) rnd = rand() & 7;
						hud_drawpal(180 + offsets.X + rnd, 208 + offsets.Y + rnd - (kickback_pic * 0.5), CHAINGUN + 5 + ((*kb - 4) / 5), shade, o, pal, angle);
					}

					if (*kb < 8)
					{
						auto rnd = rand() & 7;
						hud_drawpal(158 + offsets.X + rnd, 208 + offsets.Y + rnd - (kickback_pic * 0.5), CHAINGUN + 5 + ((*kb - 2) / 5), shade, o, pal, angle);
						hud_drawpal(178 + offsets.X, 233 + offsets.Y, CHAINGUN + 1 + (*kb >> 1), shade, o, pal, angle);
					}
					else
					{
						hud_drawpal(178 + offsets.X, 233 + offsets.Y, CHAINGUN + 1, shade, o, pal, angle);
					}
					break;
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaypistol = [&]()
		{
			offsets.Y -= gun_pos;

			if (*kb < 5)
			{
				static constexpr uint8_t kb_frames[] = { 0,1,2,0,0 };
				const double l = 195 - 12 + weapon_xoffset - (*kb == 2) * 3;

				hud_drawpal(l + offsets.X, 244 + offsets.Y, FIRSTGUN + kb_frames[*kb], shade, 2, pal, angle);
			}
			else
			{
				const int pin = (isWW2GI() || (gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;
				const int pic_5 = FIRSTGUN+5;
				const int WEAPON2_RELOAD_TIME = 50;
				const int reload_time = isWW2GI() ? aplWeaponReload(PISTOL_WEAPON, snum) : WEAPON2_RELOAD_TIME;

				if (*kb < 10)
				{
					hud_drawpal(194 + offsets.X, 230 + offsets.Y, FIRSTGUN + 4, shade, o | pin, pal, angle);
				}
				else if (*kb < 15)
				{
					hud_drawpal(244 + offsets.X - (kickback_pic * 8.), 130 + offsets.Y + (kickback_pic * 16.), FIRSTGUN + 6, shade, o | pin, pal, angle);
					hud_drawpal(224 + offsets.X, 220 + offsets.Y, pic_5, shade, o | pin, pal, angle);
				}
				else if (*kb < 20)
				{
					hud_drawpal(124 + offsets.X + (kickback_pic * 2.), 430 + offsets.Y - (kickback_pic * 8.), FIRSTGUN + 6, shade, o | pin, pal, angle);
					hud_drawpal(224 + offsets.X, 220 + offsets.Y, pic_5, shade, o | pin, pal, angle);
				}
				else if (*kb < (isNamWW2GI()? (reload_time - 12) : 23))
				{
					hud_drawpal(184 + offsets.X, 235 + offsets.Y, FIRSTGUN + 8, shade, o | pin, pal, angle);
					hud_drawpal(224 + offsets.X, 210 + offsets.Y, pic_5, shade, o | pin, pal, angle);
				}
				else if (*kb < (isNamWW2GI()? (reload_time - 6) : 25))
				{
					hud_drawpal(164 + offsets.X, 245 + offsets.Y, FIRSTGUN + 8, shade, o | pin, pal, angle);
					hud_drawpal(224 + offsets.X, 220 + offsets.Y, pic_5, shade, o | pin, pal, angle);
				}
				else if (*kb < (isNamWW2GI()? reload_time : 27))
				{
					hud_drawpal(194 + offsets.X, 235 + offsets.Y, pic_5, shade, o, pal, angle);
				}
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayhandbomb = [&]()
		{
			int pic = HANDTHROW;
			offsets.X += weapon_xoffset;
			offsets.Y -= gun_pos;

			if (*kb)
			{
				static constexpr uint8_t throw_frames[] = { 0,0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2 };

				if (isWW2GI())
				{
					if (*kb <= aplWeaponFireDelay(HANDBOMB_WEAPON, snum))
					{
						// it holds here
						offsets.Y += 5 * kickback_pic; //D
					}
					else if (*kb < ((aplWeaponTotalTime(HANDBOMB_WEAPON, snum) - aplWeaponFireDelay(HANDBOMB_WEAPON, snum)) / 2 + aplWeaponFireDelay(HANDBOMB_WEAPON, snum)))
					{
						// up and left
						offsets.Y -= 10 * (kickback_pic - aplWeaponFireDelay(HANDBOMB_WEAPON, snum)); //U
						offsets.X += 80 * (kickback_pic - aplWeaponFireDelay(HANDBOMB_WEAPON, snum));
					}
					else if (*kb < aplWeaponTotalTime(HANDBOMB_WEAPON, snum))
					{
						// move left
						offsets.Y -= 240; // start high
						offsets.Y += 12 * (kickback_pic - aplWeaponFireDelay(HANDBOMB_WEAPON, snum)); //D
						weapon_xoffset += 90 - (5 * (aplWeaponTotalTime(HANDBOMB_WEAPON, snum) - kickback_pic));
					}
				}
				else
				{
					if (*kb < 7)
						offsets.Y += 10 * kickback_pic;        //D
					else if (*kb < 12)
						offsets.Y -= 20 * (kickback_pic - 10); //U
					else if (*kb < 20)
						offsets.Y += 9 * (kickback_pic - 14);  //D
				}

				pic += throw_frames[*kb];
				offsets.Y -= 10;
			}

			hud_drawpal(190 + offsets.X, 260 + offsets.Y, pic, shade, o, pal, angle);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayhandremote = [&]()
		{
			static constexpr uint8_t remote_frames[] = { 0,1,1,2,1,1,0,0,0,0,0 };
			hud_drawpal(102 + offsets.X, 258 + offsets.Y, HANDREMOTE + (*kb ? remote_frames[*kb] : 0), shade, o, pal, angle);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaydevastator_ww = [&]
		{
			offsets.X += weapon_xoffset;
			offsets.Y -= gun_pos;

			if (*kb)
			{
				if (*kb < aplWeaponTotalTime(p->curr_weapon, snum))
				{
					const int i = Sgn(*kb >> 2);

					if (p->ammo_amount[p->curr_weapon] & 1)
					{
						hud_drawpal(30 + offsets.X, 240 + offsets.Y, DEVISTATOR, shade, o | 4, pal, angle);
						hud_drawpal(268 + offsets.X, 238 + offsets.Y, DEVISTATOR + i, -32, o, pal, angle);
					}
					else
					{
						hud_drawpal(30 + offsets.X, 240 + offsets.Y, DEVISTATOR + i, -32, o | 4, pal, angle);
						hud_drawpal(268 + offsets.X, 238 + offsets.Y, DEVISTATOR, shade, o, pal, angle);
					}
				}
				// else we are in 'reload time'
				else if (*kb < ((aplWeaponReload(p->curr_weapon, snum) - aplWeaponTotalTime(p->curr_weapon, snum)) / 2 + aplWeaponTotalTime(p->curr_weapon, snum)))
				{
					// down 
					offsets.Y += 10 * (kickback_pic - aplWeaponTotalTime(p->curr_weapon, snum)); //D
					// offsets.X += 80 * (*kb - aplWeaponTotalTime[cw][snum]);
					hud_drawpal(268 + offsets.X, 238 + offsets.Y, DEVISTATOR, shade, o, pal, angle);
					hud_drawpal(30 + offsets.X, 240 + offsets.Y, DEVISTATOR, shade, o | 4, pal, angle);
				}
				else
				{
					// up and left
					offsets.Y += 10 * (aplWeaponReload(p->curr_weapon, snum) - kickback_pic); //U
					// offsets.X += 80 * (*kb - aplWeaponTotalTime[cw][snum]);
					hud_drawpal(268 + offsets.X, 238 + offsets.Y, DEVISTATOR, shade, o, pal, angle);
					hud_drawpal(30 + offsets.X, 240 + offsets.Y, DEVISTATOR, shade, o | 4, pal, angle);
				}
			}
			else
			{
				hud_drawpal(268 + offsets.X, 238 + offsets.Y, DEVISTATOR, shade, o, pal, angle);
				hud_drawpal(30 + offsets.X, 240 + offsets.Y, DEVISTATOR, shade, o | 4, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaydevastator = [&]
		{
			offsets.X += weapon_xoffset;
			offsets.Y -= gun_pos;

			if (*kb)
			{
				static constexpr uint8_t cycloidy[] = { 0,4,12,24,12,4,0 };
				const int i = Sgn(*kb >> 2);

				if (p->hbomb_hold_delay)
				{
					hud_drawpal(268 + offsets.X + (cycloidy[*kb] >> 1), 238 + offsets.Y + cycloidy[*kb], DEVISTATOR + i, -32, o, pal, angle);
					hud_drawpal(30 + offsets.X, 240 + offsets.Y, DEVISTATOR, shade, o | 4, pal, angle);
				}
				else
				{
					hud_drawpal(30 + offsets.X - (cycloidy[*kb] >> 1), 240 + offsets.Y + cycloidy[*kb], DEVISTATOR + i, -32, o | 4, pal, angle);
					hud_drawpal(268 + offsets.X, 238 + offsets.Y, DEVISTATOR, shade, o, pal, angle);
				}
			}
			else
			{
				hud_drawpal(268 + offsets.X, 238 + offsets.Y, DEVISTATOR, shade, o, pal, angle);
				hud_drawpal(30 + offsets.X, 240 + offsets.Y, DEVISTATOR, shade, o | 4, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayfreezer = [&]
		{
			const int pin = (isWW2GI() || (gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;

			offsets.X += weapon_xoffset;
			offsets.Y -= gun_pos;

			if (*kb)
			{
				static constexpr uint8_t cat_frames[] = { 0,0,1,1,2,2 };

				if (p->GetActor()->spr.pal != 1)
				{
					offsets.X += rand() & 3;
					offsets.Y += rand() & 3;
				}

				offsets.Y += 16;

				hud_drawpal(210 + offsets.X, 261 + offsets.Y, FREEZE + 2, -32, o | pin, pal, angle);
				hud_drawpal(210 + offsets.X, 235 + offsets.Y, FREEZE + 3 + cat_frames[*kb % 6], -32, o | pin, pal, angle);
			}
			else
			{
				hud_drawpal(210 + offsets.X, 261 + offsets.Y, FREEZE, shade, o | pin, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshrinker_ww = [&]
		{
			offsets.X += weapon_xoffset + 28;
			offsets.Y -= gun_pos - 18;

			if (*kb == 0)
			{
				// the 'at rest' display
				if (p->ammo_amount[cw] <= 0) //p->last_weapon >= 0)
				{
					hud_drawpal(184 + offsets.X, 240 + offsets.Y, SHRINKER + 3 + (*kb & 3), -32, o, 0, angle);
					hud_drawpal(188 + offsets.X, 240 + offsets.Y, SHRINKER + 1, shade, o, pal, angle);
				}
				else
				{
					hud_drawpal(184 + offsets.X, 240 + offsets.Y, SHRINKER + 2, 16 - int(BobVal(random_club_frame) * 16), o, 0, angle);
					hud_drawpal(188 + offsets.X, 240 + offsets.Y, SHRINKER, shade, o, pal, angle);
				}
			}
			else
			{
				// the 'active' display.
				if (p->GetActor()->spr.pal != 1)
				{
					offsets.X += rand() & 3;
					offsets.Y -= rand() & 3;
				}


				if (*kb < aplWeaponTotalTime(p->curr_weapon, snum))
				{
					if (!(*kb < aplWeaponFireDelay(p->curr_weapon, snum)))
					{
						// lower weapon to reload cartridge (not clip)
						offsets.Y += 10 * (aplWeaponTotalTime(p->curr_weapon, snum) - kickback_pic);
					}
				}
				// else we are in 'reload time'
				else if (*kb < ((aplWeaponReload(p->curr_weapon, snum) - aplWeaponTotalTime(p->curr_weapon, snum)) / 2 + aplWeaponTotalTime(p->curr_weapon, snum)))
				{
					// down 
					offsets.Y += 10 * (kickback_pic - aplWeaponTotalTime(p->curr_weapon, snum)); //D
				}
				else
				{
					// up
					offsets.Y += 10 * (aplWeaponReload(p->curr_weapon, snum) - kickback_pic); //U
				}

				// draw weapon
				hud_drawpal(184 + offsets.X, 240 + offsets.Y, SHRINKER + 3 + (*kb & 3), -32, o, 0, angle);
				hud_drawpal(188 + offsets.X, 240 + offsets.Y, SHRINKER + 1, shade, o, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaygrower_ww = [&]
		{
			offsets.X += weapon_xoffset + 28;
			offsets.Y -= gun_pos - 18;

			if (*kb == 0)
			{
				hud_drawpal(188 + offsets.X, 240 + offsets.Y, SHRINKER - 2, shade, o, pal, angle);
			}
			else
			{
				if (p->GetActor()->spr.pal != 1)
				{
					offsets.X += rand() & 3;
					offsets.Y -= rand() & 3;
				}

				if (*kb < aplWeaponTotalTime(p->curr_weapon, snum))
				{
					if (!(*kb < aplWeaponFireDelay(p->curr_weapon, snum)))
					{
						// lower weapon to reload cartridge (not clip)
						offsets.Y += 15 * (aplWeaponTotalTime(p->curr_weapon, snum) - kickback_pic);
					}
				}
				// else we are in 'reload time'
				else if (*kb < ((aplWeaponReload(p->curr_weapon, snum) - aplWeaponTotalTime(p->curr_weapon, snum)) / 2 + aplWeaponTotalTime(p->curr_weapon, snum)))
				{
					// down 
					offsets.Y += 5 * (kickback_pic - aplWeaponTotalTime(p->curr_weapon, snum));
				}
				else
				{
					// up
					offsets.Y += 10 * (aplWeaponReload(p->curr_weapon, snum) - kickback_pic);
				}

				// display weapon
				hud_drawpal(184 + offsets.X, 240 + offsets.Y, SHRINKER + 3 + (*kb & 3), -32, o, 2, angle);
				hud_drawpal(188 + offsets.X, 240 + offsets.Y, SHRINKER - 1, shade, o, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshrinker = [&]
		{
			auto shrinker = /*isWorldTour() ? SHRINKERWIDE :*/ SHRINKER;
			offsets.X += weapon_xoffset + 28;
			offsets.Y -= gun_pos - 18;

			if (*kb == 0)
			{
				if (cw == GROW_WEAPON)
				{
					hud_drawpal(184 + offsets.X, 240 + offsets.Y, SHRINKER + 2, 16 - int(BobVal(random_club_frame) * 16), o, 2, angle);
					hud_drawpal(188 + offsets.X, 240 + offsets.Y, shrinker - 2, shade, o, pal, angle);
				}
				else
				{
					hud_drawpal(184 + offsets.X, 240 + offsets.Y, SHRINKER + 2, 16 - int(BobVal(random_club_frame) * 16), o, 0, angle);
					hud_drawpal(188 + offsets.X, 240 + offsets.Y, shrinker, shade, o, pal, angle);
				}
			}
			else
			{
				if (p->GetActor()->spr.pal != 1)
				{
					offsets.X += rand() & 3;
					offsets.Y -= rand() & 3;
				}

				if (cw == GROW_WEAPON)
				{
					hud_drawpal(184 + offsets.X, 240 + offsets.Y, SHRINKER + 3 + (*kb & 3), -32, o, 2, angle);
					hud_drawpal(188 + offsets.X, 240 + offsets.Y, shrinker - 1, shade, o, pal, angle);
				}
				else
				{
					hud_drawpal(184 + offsets.X, 240 + offsets.Y, SHRINKER + 3 + (*kb & 3), -32, o, 0, angle);
					hud_drawpal(188 + offsets.X, 240 + offsets.Y, shrinker + 1, shade, o, pal, angle);
				}
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayflamethrower = [&]()
		{
			offsets.X += weapon_xoffset;
			offsets.Y -= gun_pos;

			if (*kb < 1 || p->cursector->lotag == 2)
			{
				hud_drawpal(210 + offsets.X, 261 + offsets.Y, FLAMETHROWER, shade, o, pal, angle);
				hud_drawpal(210 + offsets.X, 261 + offsets.Y, FLAMETHROWERPILOT, shade, o, pal, angle);
			}
			else
			{
				static constexpr uint8_t cat_frames[] = { 0, 0, 1, 1, 2, 2 };

				if (p->GetActor()->spr.pal != 1)
				{
					offsets.X += krand() & 1;
					offsets.Y += krand() & 1;
				}

				offsets.Y += 16;

				hud_drawpal(210 + offsets.X, 261 + offsets.Y, FLAMETHROWER + 1, -32, o, pal, angle);
				hud_drawpal(210 + offsets.X, 235 + offsets.Y, FLAMETHROWER + 2 + cat_frames[*kb % 6], -32, o, pal, angle);
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
			displayknee();
			break;

		case TRIPBOMB_WEAPON:
			displaytripbomb();
			break;

		case RPG_WEAPON:
			displayrpg();
			break;

		case SHOTGUN_WEAPON:
			if (isWW2GI()) displayshotgun_ww();
			else displayshotgun();
			break;

		case CHAINGUN_WEAPON:
			if (isWW2GI()) displaychaingun_ww();
			else displaychaingun();
			break;

		case PISTOL_WEAPON:
			displaypistol();
			break;

		case HANDBOMB_WEAPON:
			displayhandbomb();
			break;

		case HANDREMOTE_WEAPON:
			displayhandremote();
			break;

		case DEVISTATOR_WEAPON:
			if (isWW2GI()) displaydevastator_ww();
			else displaydevastator();
			break;

		case FREEZE_WEAPON:
			displayfreezer();
			break;

		case SHRINKER_WEAPON:
			if (isWW2GI()) displayshrinker_ww();
			else displayshrinker();
			break;

		case GROW_WEAPON:
			if (isWW2GI()) displaygrower_ww();
			else displayshrinker();
			break;

		case FLAMETHROWER_WEAPON:
			displayflamethrower();
			break;
		}
	}

	displayloogie(p, interpfrac);
}

END_DUKE_NS
