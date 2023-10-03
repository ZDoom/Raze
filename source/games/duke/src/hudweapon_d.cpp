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
#include "dukeactor.h"
#include "buildtiles.h"

BEGIN_DUKE_NS 

inline static double getavel(DDukePlayer* const p)
{
	return p->cmd.ucmd.avel * (2048. / 360.);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

inline static void hud_drawpal(double x, double y, const char* tilenum, int shade, int orientation, int p, DAngle angle)
{
	hud_drawsprite(x, y, 1., angle.Degrees(), TexMan.CheckForTexture(tilenum, ETextureType::Any), shade, p, orientation);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void displayloogie(DDukePlayer* p, double const interpfrac)
{
	if (p->loogcnt == 0) return;

	const double loogi = interpolatedvalue<double>(p->oloogcnt, p->loogcnt, interpfrac);
	const double y = loogi * 4.;

	for (int i = 0; i < p->numloogs; i++)
	{
		const double a = fabs(BobVal((loogi + i) * 32.) * 90);
		const double z = 4096. + ((loogi + i) * 512.);
		const double x = -getavel(p) + BobVal((loogi + i) * 64.) * 16;

		hud_drawsprite((p->loogie[i].X + x), (200 + p->loogie[i].Y - y), (z - (i << 8)) / 65536., a - 22.5, TexMan.CheckForTexture("LOOGIE", ETextureType::Any), 0, 0, 0);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool animatefist(int gs, DDukePlayer* p, double xoffset, double yoffset, int fistpal, double const interpfrac)
{
	const double fisti = min(interpolatedvalue<double>(p->ofist_incs, p->fist_incs, interpfrac), 32.);
	if (fisti <= 0) return false;

	hud_drawsprite(
		(-fisti + 222 + xoffset),
		(yoffset + 194 + BobVal((6 + fisti) * 128.) * 32),
		clamp(65536. - 65536. * BobVal(512 + fisti * 64.), 40920., 90612.) / 65536., 0, TexMan.CheckForTexture("FIST", ETextureType::Any), gs, fistpal, 0);

	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool animateknee(int gs, DDukePlayer* p, double xoffset, double yoffset, int pal, double const interpfrac, DAngle angle)
{
	if (p->knee_incs > 11 || p->knee_incs == 0 || p->GetActor()->spr.extra <= 0) return false;

	static const int8_t knee_y[] = { 0,-8,-16,-32,-64,-84,-108,-108,-108,-72,-32,-8 };
	const double kneei = interpolatedvalue<double>(knee_y[p->oknee_incs], knee_y[p->knee_incs], interpfrac);

	hud_drawpal(105 + (kneei * 0.25) + xoffset, 280 + kneei + yoffset, "KNEE", gs, 4, pal, angle);

	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool animateknuckles(int gs, DDukePlayer* p, double xoffset, double yoffset, int pal, DAngle angle)
{
	if (isWW2GI() || p->over_shoulder_on != 0 || p->knuckle_incs == 0 || p->GetActor()->spr.extra <= 0) return false;
	static const char* const frames[] = { "CRACKKNUCKLES0", "CRACKKNUCKLES1", "CRACKKNUCKLES2", "CRACKKNUCKLES3" };
	static const uint8_t knuckle_frames[] = { 0,1,2,2,3,3,3,2,2,1,0 };

	hud_drawpal(160 + xoffset, 180 + yoffset, frames[knuckle_frames[p->knuckle_incs >> 1]], gs, 4, pal, angle);

	return true;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displaymasks_d(int snum, int p, double interpfrac)
{
	if (getPlayer(snum)->scuba_on)
	{
		auto scuba0 = TexMan.CheckForTexture("SCUBAMASK", ETextureType::Any);
		auto tex0 = TexMan.GetGameTexture(scuba0);

		double y = 200 - tex0->GetDisplayHeight();
		hud_drawsprite(44, y, 1., 0, scuba0, 0, p, 2 + 16);
		hud_drawsprite((320 - 43), y, 1, 0, scuba0, 0, p, 2 + 4 + 16);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool animatetip(int gs, DDukePlayer* p, double xoffset, double yoffset, int pal, double const interpfrac, DAngle angle)
{
	if (p->tipincs == 0) return false;

	static const char* frames[] = { "TIP0", "TIP1" };
	static const int8_t tip_y[] = { 0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16 };
	const double tipi = interpolatedvalue<double>(tip_y[p->otipincs], tip_y[p->tipincs], interpfrac) * 0.5;

	hud_drawpal(170 + xoffset, 240 + tipi + yoffset, frames[((26 - p->tipincs) >> 4)], gs, 0, pal, angle);

	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool animateaccess(int gs, DDukePlayer* p, double xoffset, double yoffset, double const interpfrac, DAngle angle)
{
	if (p->access_incs == 0 || p->GetActor()->spr.extra <= 0) return false;

	static const char* const frames[] = { "HANDHOLDINGLASER0", "HANDHOLDINGLASER1", "HANDHOLDINGLASER2" };
	static const int8_t access_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16};
	const double accessi = interpolatedvalue<double>(access_y[p->oaccess_incs], access_y[p->access_incs], interpfrac);

	const int pal = p->access_spritenum != nullptr ? p->access_spritenum->spr.pal : 0;

	if ((p->access_incs-3) > 0 && (p->access_incs-3)>>3)
		hud_drawpal(170 + (accessi * 0.25) + xoffset, 266 + accessi + yoffset, frames[(p->access_incs >> 3)], gs, 0, pal, angle);
	else
		hud_drawpal(170 + (accessi * 0.25) + xoffset, 266 + accessi + yoffset, "HANDHOLDINGACCESS", gs, 4, pal, angle);

	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void animateshrunken(DDukePlayer* p, double xoffset, double yoffset, int8_t shade, int o, double interpfrac)
{
	const double fistsign = BobVal(interpolatedvalue<double>(p->ofistsign, p->fistsign, interpfrac)) * 16;
	int pal = getPlayer(screenpeek)->cursector->floorpal;
	if (p->jetpack_on == 0)	yoffset += 32 - (p->GetActor()->vel.X * 8);
	hud_drawpal(250 + fistsign + xoffset, 258 - fabs(fistsign * 4) + yoffset, "FIST", shade, o, pal, nullAngle);
	hud_drawpal(40 - fistsign + xoffset, 200 + fabs(fistsign * 4) + yoffset, "FIST", shade, o | 4, pal, nullAngle);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displayweapon_d(int snum, double interpfrac)
{
	int pal, pal2;
	DDukePlayer* p = getPlayer(snum);

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

	auto offpair = p->Angles.getWeaponOffsets(interpfrac);
	auto offsets = offpair.first;
	auto pitchoffset = 16. * (p->Angles.getRenderAngles(interpfrac).Pitch / DAngle90);
	auto yawinput = getavel(p) * (1. / 16.);
	auto angle = offpair.second;
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

	offsets.X += weapon_xoffset;
	offsets.Y -= gun_pos;

	int cw = p->last_weapon >= 0 ? p->last_weapon : p->curr_weapon;
	if (isWW2GI()) cw = aplWeaponWorksLike(cw, snum);

	// onevent should go here..
	// rest of code should be moved to CON..
	int quick_kick = 14 - p->quick_kick;

	if (quick_kick != 14 || p->last_quick_kick)
	{
		if (quick_kick < 5 || quick_kick > 9)
		{
			hud_drawpal(80 + offsets.X, 250 + offsets.Y, "KNEE", shade, o | 4, pal2, angle);
		}
		else
		{
			hud_drawpal(160 - 16 + offsets.X, 214 + offsets.Y, "KNEE1", shade, o | 4, pal2, angle);
		}
	}

	if (p->GetActor()->spr.scale.X < 0.625)
	{
		//shrunken..
		animateshrunken(p, offsets.X, offsets.Y + gun_pos, shade, o, interpfrac);
	}
	else
	{
		int weapTotalTime = 0, weapFireDelay = 0, weapReload = 0;
		if (isWW2GI())
		{
			weapTotalTime = aplWeaponTotalTime(p->curr_weapon, snum);
			weapFireDelay = aplWeaponFireDelay(p->curr_weapon, snum);
			weapReload = aplWeaponReload(p->curr_weapon, snum);
		}

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayknee = [&]()
		{
			if (*kb > 0)
			{
				if (*kb < 5 || *kb > 9)
				{
					hud_drawpal(220 + offsets.X, 250 + offsets.Y, "KNEE", shade, o, pal2, angle);
				}
				else
				{
					hud_drawpal(160 + offsets.X, 214 + offsets.Y, "KNEE1", shade, o, pal2, angle);
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
			static const char* const frames[] = { "HANDHOLDINGLASER0", "HANDHOLDINGLASER1", "HANDHOLDINGLASER2" };
			offsets.X += 8;
			offsets.Y -= 10;

			if (*kb > 6)
				offsets.Y += kickback_pic * 8.;
			else if (*kb < 4)
				hud_drawpal(142 + offsets.X, 234 + offsets.Y, "TRIPBOMB", shade, o, pal, angle);

			int i = (*kb >> 2);
			if (i < 3)
			{
				hud_drawpal(130 + offsets.X, 249 + offsets.Y, frames[i], shade, o, pal, angle);
				hud_drawpal(152 + offsets.X, 249 + offsets.Y, frames[i], shade, o | 4, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayrpg = [&]()
		{
			const int pin = ((gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;

			offsets -= BobVal(512 + (min(kickback_pic, 16.) * 128.)) * 8;

			if (*kb > 0)
			{
				if (*kb < (isWW2GI() ? weapTotalTime : 8))
				{
					int frame = (*kb >> 1);
					if (frame >= 1 && frame <= 3)
					{
						static const char* const muzzleflash[] = { "RPGMUZZLEFLASH1", "RPGMUZZLEFLASH2", "RPGMUZZLEFLASH3" };
						hud_drawpal(164 + offsets.X, 176 + offsets.Y, muzzleflash[frame - 1], shade, o | pin, pal, angle);
					}
				}
				else if (isWW2GI())
				{
					// else we are in 'reload time'
					if (*kb < ((weapReload - weapTotalTime) / 2 + weapTotalTime))
					{
						// down 
						offsets.Y += 10 * (kickback_pic - weapTotalTime); //D
					}
					else
					{
						// up and left
						offsets.Y += 10 * (weapReload - kickback_pic); //U
					}
				}
			}

			hud_drawpal(164 + offsets.X, 176 + offsets.Y, "RPGGUN", shade, o | pin, pal, angle);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshotgun_ww = [&]()
		{
			offsets.X -= 8;

			if (*kb > 0)
				offsets.Y += BobVal(kickback_pic * 128.) * 4;

			if (*kb > 0 && p->GetActor()->spr.pal != 1)
				offsets.X += 1 - (rand() & 3);

			const char* pic = "SHOTGUN";

			if (*kb == 0)
			{
				// Just fall through here.
			}
			else if (*kb <= weapTotalTime)
			{
				pic = "SHOTGUN1";
			}
			// else we are in 'reload time'
			else if (*kb < ((weapReload - weapTotalTime) / 2 + weapTotalTime))
			{
				// down 
				offsets.Y += 10 * (kickback_pic - weapTotalTime); //D
			}
			else
			{
				// up and left
				offsets.Y += 10 * (weapReload - kickback_pic); //U
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
			offsets.X -= 8;

			switch(*kb)
			{
				case 1:
				case 2:
					hud_drawpal(168 + offsets.X, 201 + offsets.Y, "SHOTGUN2", -128, o, pal, angle);
					[[fallthrough]];
				case 0:
				case 6:
				case 7:
				case 8:
					hud_drawpal(146 + offsets.X, 202 + offsets.Y, "SHOTGUN", shade, o, pal, angle);
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
						// this was screwed. *kb can only be 3 and 4 here as case 2 is elsewhere, so ((*(kb)-1) >> 1) will always be 1, meaning only SHOTGUN2 can be drawn.
						hud_drawpal(178 + offsets.X, 194 + offsets.Y, "SHOTGUN2", -128, o, pal, angle);
					}
					hud_drawpal(158 + offsets.X, 220 + offsets.Y, "SHOTGUN3", shade, o, pal, angle);
					break;
				case 13:
				case 14:
				case 15:
					hud_drawpal(198 + offsets.X, 210 + offsets.Y, "SHOTGUN4", shade, o, pal, angle);
					break;
				case 16:
				case 17:
				case 18:
				case 19:
					hud_drawpal(234 + offsets.X, 196 + offsets.Y, "SHOTGUN5", shade, o, pal, angle);
					break;
				case 20:
				case 21:
				case 22:
				case 23:
					hud_drawpal(240 + offsets.X, 196 + offsets.Y, "SHOTGUN6", shade, o, pal, angle);
					break;
				case 24:
				case 25:
				case 26:
				case 27:
					hud_drawpal(234 + offsets.X, 196 + offsets.Y, "SHOTGUN5", shade, o, pal, angle);
					break;
				case 28:
				case 29:
				case 30:
					hud_drawpal(188 + offsets.X, 206 + offsets.Y, "SHOTGUN4", shade, o, pal, angle);
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
			if (*kb > 0)
				offsets.Y += BobVal(kickback_pic * 128.) * 4;

			if (*kb > 0 && p->GetActor()->spr.pal != 1)
				offsets.X += 1 - (rand() & 3);

			if (*kb == 0)
			{
				hud_drawpal(178 + offsets.X, 233 + offsets.Y, "CHAINGUNF1", shade, o, pal, angle);
			}
			else if (*kb <= weapTotalTime)
			{
				hud_drawpal(188 + offsets.X, 243 + offsets.Y, "CHAINGUNF2", shade, o, pal, angle);
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
				const char* pic;
				const int iFifths = max((weapReload - weapTotalTime) / 5, 1);

				if (*kb < (iFifths + weapTotalTime))
				{
					// first segment
					pic = "WW2CGRELOAD3";
					adj = 80 - (10 * (weapTotalTime + iFifths - kickback_pic));
				}
				else if (*kb < (iFifths * 2 + weapTotalTime))
				{
					// second segment (down)
					pic = "WW2SCRELOAD2";
					adj = 80;
				}
				else if (*kb < (iFifths * 3 + weapTotalTime))
				{
					// third segment (up)
					pic = "WW2CGRELOAD1";
					adj = 80;
				}
				else if (*kb < (iFifths * 4 + weapTotalTime))
				{
					// fourth segment (down)
					pic = "WW2CGRELOAD2";
					adj = 80;
				}
				else
				{
					// up and left
					pic = "WW2CGRELOAD3";
					adj = 10 * (weapReload - kickback_pic);
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
			static const char* const fireframes[] = { "CHAINGUNF1", "CHAINGUNF2", "CHAINGUNF3", "CHAINGUNF4" };
			static const char* const flashframes[] = { "CHAINGUNFLASH1", "CHAINGUNFLASH2", "CHAINGUNFLASH3" };
			if (*kb > 0)
				offsets.Y += BobVal(kickback_pic * 128.) * 4;

			if (*kb > 0 && p->GetActor()->spr.pal != 1)
				offsets.X += 1 - (rand() & 3);

			hud_drawpal(168 + offsets.X, 260 + offsets.Y, "CHAINGUN", shade, o, pal, angle);

			switch(*kb)
			{
				case 0:
					hud_drawpal(178 + offsets.X, 233 + offsets.Y, "CHAINGUNF1", shade, o, pal, angle);
					break;
				default:
					if (*kb > 4 && *kb < 12)
					{
						auto rnd = p->GetActor()->spr.pal != 1 ? rand() & 7 : 0;
						hud_drawpal(136 + offsets.X + rnd, 208 + offsets.Y + rnd - (kickback_pic * 0.5), flashframes[((*kb - 4) / 5)], shade, o, pal, angle);

						if (p->GetActor()->spr.pal != 1) rnd = rand() & 7;
						hud_drawpal(180 + offsets.X + rnd, 208 + offsets.Y + rnd - (kickback_pic * 0.5), flashframes[((*kb - 4) / 5)], shade, o, pal, angle);
					}

					if (*kb < 8)
					{
						auto rnd = rand() & 7;
						hud_drawpal(158 + offsets.X + rnd, 208 + offsets.Y + rnd - (kickback_pic * 0.5), flashframes[((*kb - 2) / 5)], shade, o, pal, angle);
						hud_drawpal(178 + offsets.X, 233 + offsets.Y, fireframes[(*kb >> 1)], shade, o, pal, angle);
					}
					else
					{
						hud_drawpal(178 + offsets.X, 233 + offsets.Y, "CHAINGUNF1", shade, o, pal, angle);
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
			static const char* const frames[] = { "FIRSTGUN0", "FIRSTGUN1", "FIRSTGUN2" };
			if (*kb < 5)
			{
				static constexpr uint8_t kb_frames[] = { 0,1,2,0,0 };
				hud_drawpal((195 - 12 - (*kb == 2) * 3) + offsets.X, 244 + offsets.Y, frames[kb_frames[*kb]], shade, 2, pal, angle);
			}
			else
			{
				const int pin = (isWW2GI() || (gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;
				const int WEAPON2_RELOAD_TIME = 50;
				const int reload_time = isWW2GI() ? weapReload : WEAPON2_RELOAD_TIME;

				offsets.X -= weapon_xoffset;

				if (*kb < 10)
				{
					hud_drawpal(194 + offsets.X, 230 + offsets.Y, "FIRSTGUNRELOAD0", shade, o | pin, pal, angle);
				}
				else if (*kb < 15)
				{
					hud_drawpal(244 + offsets.X - (kickback_pic * 8.), 130 + offsets.Y + (kickback_pic * 16.), "FIRSTGUNMAG0", shade, o | pin, pal, angle);
					hud_drawpal(224 + offsets.X, 220 + offsets.Y, "FIRSTGUNRELOAD1", shade, o | pin, pal, angle);
				}
				else if (*kb < 20)
				{
					hud_drawpal(124 + offsets.X + (kickback_pic * 2.), 430 + offsets.Y - (kickback_pic * 8.), "FIRSTGUNMAG0", shade, o | pin, pal, angle);
					hud_drawpal(224 + offsets.X, 220 + offsets.Y, "FIRSTGUNRELOAD1", shade, o | pin, pal, angle);
				}
				else if (*kb < (isNamWW2GI()? (reload_time - 12) : 23))
				{
					hud_drawpal(184 + offsets.X, 235 + offsets.Y, "FIRSTGUNHAND", shade, o | pin, pal, angle);
					hud_drawpal(224 + offsets.X, 210 + offsets.Y, "FIRSTGUNRELOAD1", shade, o | pin, pal, angle);
				}
				else if (*kb < (isNamWW2GI()? (reload_time - 6) : 25))
				{
					hud_drawpal(164 + offsets.X, 245 + offsets.Y, "FIRSTGUNHAND", shade, o | pin, pal, angle);
					hud_drawpal(224 + offsets.X, 220 + offsets.Y, "FIRSTGUNRELOAD1", shade, o | pin, pal, angle);
				}
				else if (*kb < (isNamWW2GI()? reload_time : 27))
				{
					hud_drawpal(194 + offsets.X, 235 + offsets.Y, "FIRSTGUNRELOAD1", shade, o, pal, angle);
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
			static const char* const frames[] = { "HANDTHROW0", "HANDTHROW1", "HANDTHROW2" };

			int pic = 0;
			if (*kb)
			{
				static constexpr uint8_t throw_frames[] = { 0,0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2 };

				if (isWW2GI())
				{
					if (*kb <= weapFireDelay)
					{
						// it holds here
						offsets.Y += 5 * kickback_pic; //D
					}
					else if (*kb < ((weapTotalTime - weapFireDelay) / 2 + weapFireDelay))
					{
						// up and left
						offsets.Y -= 10 * (kickback_pic - weapFireDelay); //U
						offsets.X += 80 * (kickback_pic - weapFireDelay);
					}
					else if (*kb < weapTotalTime)
					{
						// move left
						offsets.Y -= 240; // start high
						offsets.Y += 12 * (kickback_pic - weapFireDelay); //D
						weapon_xoffset += 90 - (5 * (weapTotalTime - kickback_pic));
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

				pic = throw_frames[*kb];
				offsets.Y -= 10;
			}

			hud_drawpal(190 + offsets.X, 260 + offsets.Y, frames[pic], shade, o, pal, angle);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayhandremote = [&]()
		{
			static const char* const frames[] = { "HANDREMOTE0", "HANDREMOTE1", "HANDREMOTE2" };
			static constexpr uint8_t remote_frames[] = { 0,1,1,2,1,1,0,0,0,0,0 };
			hud_drawpal(102 + offsets.X, 258 + offsets.Y, frames[(*kb ? remote_frames[*kb] : 0)], shade, o, pal, angle);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaydevastator_ww = [&]
		{
			const char* const frames[] = { "DEVISTATOR", "DEVISTATORFIRE" };
			if (*kb)
			{
				if (*kb < weapTotalTime)
				{
					const int i = Sgn(*kb >> 2);

					if (p->ammo_amount[p->curr_weapon] & 1)
					{
						hud_drawpal(30 + offsets.X, 240 + offsets.Y, "DEVISTATOR", shade, o | 4, pal, angle);
						hud_drawpal(268 + offsets.X, 238 + offsets.Y, frames[i], -32, o, pal, angle);
					}
					else
					{
						hud_drawpal(30 + offsets.X, 240 + offsets.Y, frames[i], -32, o | 4, pal, angle);
						hud_drawpal(268 + offsets.X, 238 + offsets.Y, "DEVISTATOR", shade, o, pal, angle);
					}
				}
				// else we are in 'reload time'
				else if (*kb < ((weapReload - weapTotalTime) / 2 + weapTotalTime))
				{
					// down 
					offsets.Y += 10 * (kickback_pic - weapTotalTime); //D
					hud_drawpal(268 + offsets.X, 238 + offsets.Y, "DEVISTATOR", shade, o, pal, angle);
					hud_drawpal(30 + offsets.X, 240 + offsets.Y, "DEVISTATOR", shade, o | 4, pal, angle);
				}
				else
				{
					// up and left
					offsets.Y += 10 * (weapReload - kickback_pic); //U
					hud_drawpal(268 + offsets.X, 238 + offsets.Y, "DEVISTATOR", shade, o, pal, angle);
					hud_drawpal(30 + offsets.X, 240 + offsets.Y, "DEVISTATOR", shade, o | 4, pal, angle);
				}
			}
			else
			{
				hud_drawpal(268 + offsets.X, 238 + offsets.Y, "DEVISTATOR", shade, o, pal, angle);
				hud_drawpal(30 + offsets.X, 240 + offsets.Y, "DEVISTATOR", shade, o | 4, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaydevastator = [&]
		{
			const char* const frames[] = { "DEVISTATOR", "DEVISTATORFIRE" };
			if (*kb)
			{
				static constexpr uint8_t cycloidy[] = { 0,4,12,24,12,4,0 };
				const int i = Sgn(*kb >> 2);

				if (p->hbomb_hold_delay)
				{
					hud_drawpal(268 + offsets.X + (cycloidy[*kb] >> 1), 238 + offsets.Y + cycloidy[*kb], frames[i], -32, o, pal, angle);
					hud_drawpal(30 + offsets.X, 240 + offsets.Y, "DEVISTATOR", shade, o | 4, pal, angle);
				}
				else
				{
					hud_drawpal(30 + offsets.X - (cycloidy[*kb] >> 1), 240 + offsets.Y + cycloidy[*kb], frames[i], -32, o | 4, pal, angle);
					hud_drawpal(268 + offsets.X, 238 + offsets.Y, "DEVISTATOR", shade, o, pal, angle);
				}
			}
			else
			{
				hud_drawpal(268 + offsets.X, 238 + offsets.Y, "DEVISTATOR", shade, o, pal, angle);
				hud_drawpal(30 + offsets.X, 240 + offsets.Y, "DEVISTATOR", shade, o | 4, pal, angle);
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

			if (*kb)
			{
				static const char* const cat[] = { "FREEZECAT0", "FREEZECAT1", "FREEZECAT2" };
				static constexpr uint8_t cat_frames[] = { 0,0,1,1,2,2 };

				if (p->GetActor()->spr.pal != 1)
				{
					offsets.X += rand() & 3;
					offsets.Y += rand() & 3;
				}

				offsets.Y += 16;

				hud_drawpal(210 + offsets.X, 261 + offsets.Y, "FREEZEFIRE", -32, o | pin, pal, angle);
				hud_drawpal(210 + offsets.X, 235 + offsets.Y, cat[cat_frames[*kb % 6]], -32, o | pin, pal, angle);
			}
			else
			{
				hud_drawpal(210 + offsets.X, 261 + offsets.Y, "FREEZE", shade, o | pin, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------
		static const char* const coilfireframes[] = { "SHRINKERFIRE1", "SHRINKERFIRE2", "SHRINKERFIRE3", "SHRINKERFIRE4" };

		auto displayshrinker_ww = [&]
		{
			offsets.X += 28;
			offsets.Y += 18;

			if (*kb == 0)
			{
				// the 'at rest' display
				if (p->ammo_amount[cw] <= 0) //p->last_weapon >= 0)
				{
					hud_drawpal(184 + offsets.X, 240 + offsets.Y, coilfireframes[(*kb & 3)], -32, o, 0, angle);
					hud_drawpal(188 + offsets.X, 240 + offsets.Y, "SHRINKERLITE", shade, o, pal, angle);
				}
				else
				{
					hud_drawpal(184 + offsets.X, 240 + offsets.Y, "SHRINKERCOIL", 16 - int(BobVal(random_club_frame) * 16), o, 0, angle);
					hud_drawpal(188 + offsets.X, 240 + offsets.Y, "SHRINKER", shade, o, pal, angle);
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


				if (*kb < weapTotalTime)
				{
					if (!(*kb < weapFireDelay))
					{
						// lower weapon to reload cartridge (not clip)
						offsets.Y += 10 * (weapTotalTime - kickback_pic);
					}
				}
				// else we are in 'reload time'
				else if (*kb < ((weapReload - weapTotalTime) / 2 + weapTotalTime))
				{
					// down 
					offsets.Y += 10 * (kickback_pic - weapTotalTime); //D
				}
				else
				{
					// up
					offsets.Y += 10 * (weapReload - kickback_pic); //U
				}

				// draw weapon
				hud_drawpal(184 + offsets.X, 240 + offsets.Y, coilfireframes[(*kb & 3)], -32, o, 0, angle);
				hud_drawpal(188 + offsets.X, 240 + offsets.Y, "SHRINKERLITE", shade, o, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaygrower_ww = [&]
		{
			offsets.X += 28;
			offsets.Y += 18;

			if (*kb == 0)
			{
				hud_drawpal(188 + offsets.X, 240 + offsets.Y, "GROWER", shade, o, pal, angle);
			}
			else
			{
				if (p->GetActor()->spr.pal != 1)
				{
					offsets.X += rand() & 3;
					offsets.Y -= rand() & 3;
				}

				if (*kb < weapTotalTime)
				{
					if (!(*kb < weapFireDelay))
					{
						// lower weapon to reload cartridge (not clip)
						offsets.Y += 15 * (weapTotalTime - kickback_pic);
					}
				}
				// else we are in 'reload time'
				else if (*kb < ((weapReload - weapTotalTime) / 2 + weapTotalTime))
				{
					// down 
					offsets.Y += 5 * (kickback_pic - weapTotalTime);
				}
				else
				{
					// up
					offsets.Y += 10 * (weapReload - kickback_pic);
				}

				// display weapon
				hud_drawpal(184 + offsets.X, 240 + offsets.Y, coilfireframes[(*kb & 3)], -32, o, 2, angle);
				hud_drawpal(188 + offsets.X, 240 + offsets.Y, "GROWERLITE", shade, o, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshrinker = [&]
		{
			offsets.X += 28;
			offsets.Y += 18;

			if (*kb == 0)
			{
				hud_drawpal(184 + offsets.X, 240 + offsets.Y, "SHRINKERCOIL", 16 - int(BobVal(random_club_frame) * 16), o, 0, angle);
				hud_drawpal(188 + offsets.X, 240 + offsets.Y, "SHRINKER", shade, o, pal, angle);
			}
			else
			{
				if (p->GetActor()->spr.pal != 1)
				{
					offsets.X += rand() & 3;
					offsets.Y -= rand() & 3;
				}

				hud_drawpal(184 + offsets.X, 240 + offsets.Y, coilfireframes[(*kb & 3)], -32, o, 0, angle);
				hud_drawpal(188 + offsets.X, 240 + offsets.Y, "SHRINKERLITE", shade, o, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaygrower = [&]
		{
			offsets.X += 28;
			offsets.Y += 18;

			if (*kb == 0)
			{
				hud_drawpal(184 + offsets.X, 240 + offsets.Y, "SHRINKERCOIL", 16 - int(BobVal(random_club_frame) * 16), o, 2, angle);
				hud_drawpal(188 + offsets.X, 240 + offsets.Y, "GROWER", shade, o, pal, angle);
			}
			else
			{
				if (p->GetActor()->spr.pal != 1)
				{
					offsets.X += rand() & 3;
					offsets.Y -= rand() & 3;
				}

				hud_drawpal(184 + offsets.X, 240 + offsets.Y, coilfireframes[(*kb & 3)], -32, o, 2, angle);
				hud_drawpal(188 + offsets.X, 240 + offsets.Y, "GROWERLITE", shade, o, pal, angle);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayflamethrower = [&]()
		{
			if (*kb < 1 || p->cursector->lotag == 2)
			{
				hud_drawpal(210 + offsets.X, 261 + offsets.Y, "FLAMETHROWER", shade, o, pal, angle);
				hud_drawpal(210 + offsets.X, 261 + offsets.Y, "FLAMETHROWERPILOT", shade, o, pal, angle);
			}
			else
			{
				static const char* const cat[] = { "FLAMETHROWERCAT0", "FLAMETHROWERCAT1", "FLAMETHROWERCAT2" };
				static constexpr uint8_t cat_frames[] = { 0, 0, 1, 1, 2, 2 };

				if (p->GetActor()->spr.pal != 1)
				{
					offsets.X += krand() & 1;
					offsets.Y += krand() & 1;
				}

				offsets.Y += 16;

				hud_drawpal(210 + offsets.X, 261 + offsets.Y, "FLAMETHROWERFIRE", -32, o, pal, angle);
				hud_drawpal(210 + offsets.X, 235 + offsets.Y, cat[cat_frames[*kb % 6]], -32, o, pal, angle);
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
			else displaygrower();
			break;

		case FLAMETHROWER_WEAPON:
			displayflamethrower();
			break;
		}
	}

	displayloogie(p, interpfrac);
}

END_DUKE_NS
