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

inline static void hud_drawpal(double x, double y, int tilenum, int shade, int orientation, int p)
{
	hud_drawsprite(x, y, 65536, 0, tilenum, shade, p, 2 | orientation);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displayloogie(player_struct* p)
{
	double a, y;
	int z;
	double x;

	if (p->loogcnt == 0) return;

	y = (p->loogcnt << 2);
	for (int i = 0; i < p->numloogs; i++)
	{
		a = fabs(bsinf((p->loogcnt + i) << 5, -5));
		z = 4096 + ((p->loogcnt + i) << 9);
		x = -getavel(p->GetPlayerNum()) + bsinf((p->loogcnt + i) << 6, -10);

		hud_drawsprite((p->loogiex[i] + x), (200 + p->loogiey[i] - y), z - (i << 8), 256 - a, LOOGIE, 0, 0, 2);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int animatefist(int gs, player_struct* p, double look_anghalf, double looking_arc, double plravel, int fistpal)
{
	int fisti;
	double fistzoom;
	double fistz;

	fisti = p->fist_incs;
	if (fisti > 32) fisti = 32;
	if (fisti <= 0) return 0;

	fistzoom = 65536 - bcosf(fisti << 6, 2);
	if (fistzoom > 90612)
		fistzoom = 90612;
	if (fistzoom < 40920)
		fistzoom = 40290;
	fistz = 194 + bsinf((6 + fisti) << 7, -9);

	hud_drawsprite(
		(-fisti + 222 + plravel),
		(looking_arc + fistz),
		int(fistzoom), 0, FIST, gs, fistpal, 2);

	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int animateknee(int gs, player_struct* p, double look_anghalf, double looking_arc, double horiz16th, double plravel, int pal)
{
	if (p->knee_incs > 11 || p->knee_incs == 0 || p->GetActor()->s->extra <= 0) return 0;

	static const int8_t knee_y[] = { 0,-8,-16,-32,-64,-84,-108,-108,-108,-72,-32,-8 };

	looking_arc += knee_y[p->knee_incs];

	hud_drawpal(105 + plravel - look_anghalf + (knee_y[p->knee_incs] >> 2), looking_arc + 280 - horiz16th, KNEE, gs, 4, pal);

	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int animateknuckles(int gs, player_struct* p, double look_anghalf, double looking_arc, double horiz16th, double plravel, int pal)
{
	if (isWW2GI() || p->over_shoulder_on != 0 || p->knuckle_incs == 0 || p->GetActor()->s->extra <= 0) return 0;

	static const uint8_t knuckle_frames[] = { 0,1,2,2,3,3,3,2,2,1,0 };

	hud_drawpal(160 + plravel - look_anghalf, looking_arc + 180 - horiz16th, CRACKKNUCKLES + knuckle_frames[p->knuckle_incs >> 1], gs, 4, pal);

	return 1;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displaymasks_d(int snum, int p, double)
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

static int animatetip(int gs, player_struct* p, double look_anghalf, double looking_arc, double horiz16th, double plravel, int pal)
{
	if (p->tipincs == 0) return 0;

	static const int8_t tip_y[] = { 0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16 };

	hud_drawpal(170 + plravel - look_anghalf,
		(tip_y[p->tipincs] >> 1) + looking_arc + 240 - horiz16th, TIP + ((26 - p->tipincs) >> 4), gs, 0, pal);

	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int animateaccess(int gs, player_struct* p, double look_anghalf, double looking_arc, double horiz16th, double plravel)
{
	if(p->access_incs == 0 || p->GetActor()->s->extra <= 0) return 0;

	static const int8_t access_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16};

	looking_arc += access_y[p->access_incs];

	int pal;
	if (p->access_spritenum != nullptr)
		pal = p->access_spritenum->s->pal;
	else pal = 0;

	if((p->access_incs-3) > 0 && (p->access_incs-3)>>3)
		hud_drawpal(170 + plravel - look_anghalf + (access_y[p->access_incs] >> 2), looking_arc + 266 - horiz16th, HANDHOLDINGLASER + (p->access_incs >> 3), gs, 0, pal);
	else
		hud_drawpal(170 + plravel - look_anghalf + (access_y[p->access_incs] >> 2), looking_arc + 266 - horiz16th, HANDHOLDINGACCESS, gs, 4, pal);

	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displayweapon_d(int snum, double smoothratio)
{
	int cw;
	int i, j;
	int o, pal;
	double weapon_sway, weapon_xoffset, gun_pos, looking_arc, kickback_pic, random_club_frame, hard_landing, look_anghalf, horiz16th, plravel;
	int8_t shade;
	struct player_struct *p;

	p = &ps[snum];
	auto kb = &p->kickback_pic;
	int pin = 0;

	o = 0;

	if (cl_hudinterpolation)
	{
		weapon_sway = interpolatedvaluef(p->oweapon_sway, p->weapon_sway, smoothratio);
		kickback_pic = interpolatedvaluef(p->okickback_pic, p->kickback_pic, smoothratio);
		random_club_frame = interpolatedvaluef(p->orandom_club_frame, p->random_club_frame, smoothratio);
		hard_landing = interpolatedvaluef(p->ohard_landing, p->hard_landing, smoothratio);
		gun_pos = 80 - interpolatedvaluef(p->oweapon_pos * p->oweapon_pos, p->weapon_pos * p->weapon_pos, smoothratio);
	}
	else
	{
		weapon_sway = p->weapon_sway;
		kickback_pic = p->kickback_pic;
		random_club_frame = p->random_club_frame;
		hard_landing = p->hard_landing;
		gun_pos = 80 - (p->weapon_pos * p->weapon_pos);
	}

	plravel = getavel(snum) * (1. / 16.);
	horiz16th = p->horizon.horizsumfrac(smoothratio);
	look_anghalf = p->angle.look_anghalf(smoothratio);
	looking_arc = p->angle.looking_arc(smoothratio);
	hard_landing *= 8.;

	gun_pos -= fabs(p->GetActor()->s->xrepeat < 32 ? bsinf(weapon_sway * 4., -9) : bsinf(weapon_sway * 0.5, -10));
	gun_pos -= hard_landing;

	weapon_xoffset = (160)-90;
	weapon_xoffset -= bcosf(weapon_sway * 0.5) * (1. / 1536.);
	weapon_xoffset -= 58 + p->weapon_ang;

	shade = p->GetActor()->s->shade;
	if(shade > 24) shade = 24;

	pal = p->GetActor()->s->pal == 1 ? 1 : p->cursector()->floorpal;
	if (pal == 0)
		pal = p->palookup;

	auto adjusted_arc = looking_arc - hard_landing;
	bool playerVars  = p->newOwner != nullptr || ud.cameraactor != nullptr || p->over_shoulder_on > 0 || (p->GetActor()->s->pal != 1 && p->GetActor()->s->extra <= 0);
	bool playerAnims = animatefist(shade, p, look_anghalf, looking_arc, plravel, pal) || animateknuckles(shade, p, look_anghalf, adjusted_arc, horiz16th, plravel, pal) ||
					   animatetip(shade, p, look_anghalf, adjusted_arc, horiz16th, plravel, pal) || animateaccess(shade, p, look_anghalf, adjusted_arc, horiz16th, plravel);

	if(playerVars || playerAnims)
		return;

	animateknee(shade, p, look_anghalf, adjusted_arc, horiz16th, plravel, pal);

	if (isWW2GI())
	{
		if (p->last_weapon >= 0)
		{
			cw = aplWeaponWorksLike[p->last_weapon][snum];
		}
		else
		{
			cw = aplWeaponWorksLike[p->curr_weapon][snum];
		}
	}
	else
	{
		if (p->last_weapon >= 0)
			cw = p->last_weapon;
		else cw = p->curr_weapon;
	}

	// onevent should go here..

	// rest of code should be moved to CON..

	j = 14-p->quick_kick;
	if (j != 14 || p->last_quick_kick)
	{
		if (j < 5 || j > 9)
		{
			hud_drawpal(weapon_xoffset + 80 - look_anghalf, looking_arc + 250 - gun_pos, KNEE, shade, o | 4, pal);
		}
		else
		{
			hud_drawpal(weapon_xoffset + 160 - 16 - look_anghalf, looking_arc + 214 - gun_pos, KNEE + 1, shade, o | 4, pal);
		}
	}

	if (p->GetActor()->s->xrepeat < 40)
	{
		static int fistsign;
		//shrunken..
		if (p->jetpack_on == 0)
		{
			i = p->GetActor()->s->xvel;
			looking_arc += 32 - (i >> 1);
			fistsign += i >> 1;
		}
		double owo = weapon_xoffset;
		weapon_xoffset += bsinf(fistsign, -10);
		hud_draw(weapon_xoffset + 250 - look_anghalf,
			looking_arc + 258 - fabs(bsinf(fistsign, -8)),
			FIST, shade, o);
		weapon_xoffset = owo;
		weapon_xoffset -= bsinf(fistsign, -10);
		hud_draw(weapon_xoffset + 40 - look_anghalf,
			looking_arc + 200 + fabs(bsinf(fistsign, -8)),
			FIST, shade, o | 4);
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
			if (*kb > 0)
			{
				if (*kb < 5 || *kb > 9)
				{
					hud_drawpal(weapon_xoffset + 220 - look_anghalf,
						looking_arc + 250 - gun_pos, KNEE, shade, o, pal);
				}
				else
				{
					hud_drawpal(weapon_xoffset + 160 - look_anghalf,
						looking_arc + 214 - gun_pos, KNEE + 1, shade, o, pal);
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
			weapon_xoffset += 8;
			gun_pos -= 10;

			if (*kb > 6)
				looking_arc += kickback_pic * 8.;
			else if (*kb < 4)
				hud_drawpal(weapon_xoffset + 142 - look_anghalf,
					looking_arc + 234 - gun_pos, HANDHOLDINGLASER + 3, shade, o, pal);

			hud_drawpal(weapon_xoffset + 130 - look_anghalf,
				looking_arc + 249 - gun_pos,
				HANDHOLDINGLASER + (*kb >> 2), shade, o, pal);
			hud_drawpal(weapon_xoffset + 152 - look_anghalf,
				looking_arc + 249 - gun_pos,
				HANDHOLDINGLASER + (*kb >> 2), shade, o | 4, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayrpg = [&]()
		{
			pin = ((gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;
			auto rpgpic = RPGGUN;

			weapon_xoffset -= bsinf(768 + (kickback_pic * 128.), -11);
			gun_pos += bsinf(768 + (kickback_pic * 128.), -11);

			if (*kb > 0)
			{
				if (*kb < (isWW2GI() ? aplWeaponTotalTime[RPG_WEAPON][snum] : 8))
				{
					hud_drawpal(weapon_xoffset + 164, (looking_arc * 2.) + 176 - gun_pos,
						RPGGUN + (*kb >> 1), shade, o | pin, pal);
				}
				else if (isWW2GI())
				{
					// else we are in 'reload time'
					if (*kb <
						(
							(aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum]) / 2
							+ aplWeaponTotalTime[p->curr_weapon][snum]
							)
						)
					{
						// down 
						gun_pos -= 10 * (kickback_pic - aplWeaponTotalTime[p->curr_weapon][snum]); //D
					}
					else
					{
						// move back down

						// up and left
						gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - kickback_pic); //U
					}
				}
			}

			hud_drawpal(weapon_xoffset + 164, (looking_arc * 2.) + 176 - gun_pos, rpgpic, shade, o | pin, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshotgun_ww = [&]()
		{
			if (*kb > 0)
			{
				gun_pos -= bsinf(kickback_pic * 128., -12);
			}

			if (*kb > 0 && p->GetActor()->s->pal != 1)
			{
				weapon_xoffset += 1 - (rand() & 3);
			}

			weapon_xoffset -= 8;

			if (*kb == 0)
			{
				hud_drawpal(weapon_xoffset + 146 - look_anghalf, looking_arc + 202 - gun_pos, SHOTGUN, shade, o, pal);
			}
			else if (*kb <= aplWeaponTotalTime[SHOTGUN_WEAPON][snum])
			{
				hud_drawpal(weapon_xoffset + 146 - look_anghalf, looking_arc + 202 - gun_pos, SHOTGUN + 1, shade, o, pal);
			}
			// else we are in 'reload time'
			else if (*kb <
				(
					(aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum]) / 2
					+ aplWeaponTotalTime[p->curr_weapon][snum]
					)
				)
			{
				// down 
				gun_pos -= 10 * (kickback_pic - aplWeaponTotalTime[p->curr_weapon][snum]); //D
//					weapon_xoffset+=80*(*kb-aplWeaponTotalTime[cw][snum]);
				hud_drawpal(weapon_xoffset + 146 - look_anghalf, looking_arc + 202 - gun_pos, SHOTGUN, shade, o, pal);
			}
			else
			{
				// move back down

				// up and left
				gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - kickback_pic); //U
//					weapon_xoffset+=80*(*kb-aplWeaponTotalTime[cw][snum]);
				hud_drawpal(weapon_xoffset + 146 - look_anghalf, looking_arc + 202 - gun_pos, SHOTGUN, shade, o, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshotgun = [&]()
		{
			weapon_xoffset -= 8;

			switch(*kb)
			{
				case 1:
				case 2:
					hud_drawpal(weapon_xoffset + 168 - look_anghalf,looking_arc + 201 - gun_pos, SHOTGUN + 2,-128,o,pal);
				case 0:
				case 6:
				case 7:
				case 8:
					hud_drawpal(weapon_xoffset + 146 - look_anghalf,looking_arc + 202 - gun_pos, SHOTGUN,shade,o,pal);
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
						gun_pos -= 40;
						weapon_xoffset += 20;

						hud_drawpal(weapon_xoffset + 178 - look_anghalf,looking_arc + 194 - gun_pos, SHOTGUN + 1 + ((*(kb)-1) >> 1),-128,o,pal);
					}

					hud_drawpal(weapon_xoffset + 158 - look_anghalf,looking_arc + 220 - gun_pos, SHOTGUN + 3,shade,o,pal);

					break;
				case 13:
				case 14:
				case 15:
					hud_drawpal(32 + weapon_xoffset + 166 - look_anghalf,looking_arc + 210 - gun_pos, SHOTGUN + 4,shade,o,pal);
					break;
				case 16:
				case 17:
				case 18:
				case 19:
					hud_drawpal(64 + weapon_xoffset + 170 - look_anghalf,looking_arc + 196 - gun_pos, SHOTGUN + 5,shade,o,pal);
					break;
				case 20:
				case 21:
				case 22:
				case 23:
					hud_drawpal(64 + weapon_xoffset + 176 - look_anghalf,looking_arc + 196 - gun_pos, SHOTGUN + 6,shade,o,pal);
					break;
				case 24:
				case 25:
				case 26:
				case 27:
					hud_drawpal(64 + weapon_xoffset + 170 - look_anghalf,looking_arc + 196 - gun_pos, SHOTGUN + 5,shade,o,pal);
					break;
				case 28:
				case 29:
				case 30:
					hud_drawpal(32 + weapon_xoffset + 156 - look_anghalf,looking_arc + 206 - gun_pos, SHOTGUN + 4,shade,o,pal);
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
				gun_pos -= bsinf(kickback_pic * 128., -12);

			if (*kb > 0 && p->GetActor()->s->pal != 1) weapon_xoffset += 1 - (rand() & 3);

			if (*kb == 0)
			{
				//				hud_drawpal(weapon_xoffset+168-look_anghalf,looking_arc+260-gun_pos,
				//						CHAINGUN,gs,o,pal);
				hud_drawpal(weapon_xoffset + 178 - look_anghalf, looking_arc + 233 - gun_pos, CHAINGUN + 1, shade, o, pal);
			}
			else if (*kb <= aplWeaponTotalTime[CHAINGUN_WEAPON][snum])
			{
				hud_drawpal(weapon_xoffset + 188 - look_anghalf, looking_arc + 243 - gun_pos, CHAINGUN + 2, shade, o, pal);
			}
			// else we are in 'reload time'
			// divide reload time into fifths.
			// 1) move weapon up/right, hand on clip (2519)
			// 2) move weapon up/right, hand removing clip (2518)
			// 3) hold weapon up/right, hand removed clip (2517)
			// 4) hold weapon up/right, hand inserting clip (2518)
			// 5) move weapon down/left, clip inserted (2519)

			else
			{
				int iFifths = (aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum]) / 5;
				if (iFifths < 1)
				{
					iFifths = 1;
				}
				if (*kb <
					(iFifths
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// first segment
					// 
					gun_pos += 80 - (10 * (aplWeaponTotalTime[p->curr_weapon][snum]	+ iFifths - kickback_pic));
					weapon_xoffset += 80 - (10 * (aplWeaponTotalTime[p->curr_weapon][snum] + iFifths - kickback_pic));
					hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos, 2519, shade, o, pal);
				}
				else if (*kb <
					(iFifths * 2
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// second segment
					// down 
					gun_pos += 80; //5*(iFifthsp->kickback_pic-aplWeaponTotalTime[p->curr_weapon][snum]); //D
					weapon_xoffset += 80; //80*(*kb-aplWeaponTotalTime[p->curr_weapon][snum]);
					hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos, 2518, shade, o, pal);
				}
				else if (*kb <
					(iFifths * 3
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// third segment
					// up 
					gun_pos += 80;//5*(iFifths*2);
					weapon_xoffset += 80; //80*(*kb-aplWeaponTotalTime[p->curr_weapon][snum]);
					hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos, 2517, shade, o, pal);
				}
				else if (*kb <
					(iFifths * 4
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// fourth segment
					// down 
					gun_pos += 80; //5*(aplWeaponTotalTime[p->curr_weapon][snum]- p->kickback_pic); //D
					weapon_xoffset += 80; //80*(*kb-aplWeaponTotalTime[p->curr_weapon][snum]);
					hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos, 2518, shade, o, pal);
				}
				else
				{
					// move back down

					// up and left
					gun_pos += 10 * (aplWeaponReload[p->curr_weapon][snum] - kickback_pic);
					//5*(aplWeaponReload[p->curr_weapon][snum]- p->kickback_pic); //U
					weapon_xoffset += 10 * (aplWeaponReload[p->curr_weapon][snum] - kickback_pic);
					//80*(*kb-aplWeaponTotalTime[cw][snum]);
					hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos, 2519, shade, o, pal);
				}
			}

		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaychaingun = [&]
		{
			if (*kb > 0)
				gun_pos -= bsinf(kickback_pic * 128., -12);

			if (*kb > 0 && p->GetActor()->s->pal != 1) weapon_xoffset += 1 - (rand() & 3);

			hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos, CHAINGUN, shade, o, pal);
			switch(*kb)
			{
				case 0:
					hud_drawpal(weapon_xoffset + 178 - look_anghalf,looking_arc + 233 - gun_pos, CHAINGUN + 1,shade,o,pal);
					break;
				default:
					if (*kb > 4 && *kb < 12)
					{
						i = 0;
						if (p->GetActor()->s->pal != 1) i = rand() & 7;
						hud_drawpal(i + weapon_xoffset - 4 + 140 - look_anghalf,i + looking_arc - (kickback_pic / 2.) + 208 - gun_pos, CHAINGUN + 5 + ((*kb - 4) / 5),shade,o,pal);
						if (p->GetActor()->s->pal != 1) i = rand() & 7;
						hud_drawpal(i + weapon_xoffset - 4 + 184 - look_anghalf,i + looking_arc - (kickback_pic / 2.) + 208 - gun_pos, CHAINGUN + 5 + ((*kb - 4) / 5),shade,o,pal);
					}
					if (*kb < 8)
					{
						i = rand() & 7;
						hud_drawpal(i + weapon_xoffset - 4 + 162 - look_anghalf,i + looking_arc - (kickback_pic / 2.) + 208 - gun_pos, CHAINGUN + 5 + ((*kb - 2) / 5),shade,o,pal);
						hud_drawpal(weapon_xoffset + 178 - look_anghalf,looking_arc + 233 - gun_pos, CHAINGUN + 1 + (*kb >> 1),shade,o,pal);
					}
					else hud_drawpal(weapon_xoffset + 178 - look_anghalf,looking_arc + 233 - gun_pos, CHAINGUN + 1,shade,o,pal);
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
			if (*kb < 5)
			{
				static const uint8_t kb_frames[] = { 0,1,2,0,0 };

				double l = 195 - 12 + weapon_xoffset;

				if (*kb == 2)
					l -= 3;
				{
					hud_drawpal(
						(l - look_anghalf),
						(looking_arc + 244 - gun_pos),
						FIRSTGUN + kb_frames[*kb],
						shade, 2, pal);
				}
			}
			else
			{
				pin = (isWW2GI() || (gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;
				auto pic_5 = FIRSTGUN+5;

				const int WEAPON2_RELOAD_TIME = 50;
				auto reload_time = isWW2GI() ? aplWeaponReload[PISTOL_WEAPON][snum] : WEAPON2_RELOAD_TIME;
				if (*kb < 10)
					hud_drawpal(194 - look_anghalf, looking_arc + 230 - gun_pos, FIRSTGUN + 4, shade, o|pin, pal);
				else if (*kb < 15)
				{
					hud_drawpal(244 - (kickback_pic * 8.) - look_anghalf, looking_arc + 130 - gun_pos + (kickback_pic * 16.), FIRSTGUN + 6, shade, o | pin, pal);
					hud_drawpal(224 - look_anghalf, looking_arc + 220 - gun_pos, pic_5, shade, o | pin, pal);
				}
				else if (*kb < 20)
				{
					hud_drawpal(124 + (kickback_pic * 2.) - look_anghalf, looking_arc + 430 - gun_pos - (kickback_pic * 8.), FIRSTGUN + 6, shade, o | pin, pal);
					hud_drawpal(224 - look_anghalf, looking_arc + 220 - gun_pos, pic_5, shade, o | pin, pal);
				}
				else if (*kb < (isNamWW2GI()? (reload_time - 12) : 23))
				{
					hud_drawpal(184 - look_anghalf, looking_arc + 235 - gun_pos, FIRSTGUN + 8, shade, o | pin, pal);
					hud_drawpal(224 - look_anghalf, looking_arc + 210 - gun_pos, pic_5, shade, o | pin, pal);
				}
				else if (*kb < (isNamWW2GI()? (reload_time - 6) : 25))
				{
					hud_drawpal(164 - look_anghalf, looking_arc + 245 - gun_pos, FIRSTGUN + 8, shade, o | pin, pal);
					hud_drawpal(224 - look_anghalf, looking_arc + 220 - gun_pos, pic_5, shade, o | pin, pal);
				}
				else if (*kb < (isNamWW2GI()? reload_time : 27))
					hud_drawpal(194 - look_anghalf, looking_arc + 235 - gun_pos, pic_5, shade, o, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayhandbomb = [&]()
		{
			if (*kb)
			{
				static const uint8_t throw_frames[]
					= { 0,0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2 };

				if (isWW2GI())
				{
					if (*kb <= aplWeaponFireDelay[HANDBOMB_WEAPON][snum])
					{
						// it holds here
						gun_pos -= 5 * kickback_pic; //D
					}
					else if (*kb <
						(
							(aplWeaponTotalTime[HANDBOMB_WEAPON][snum] - aplWeaponFireDelay[HANDBOMB_WEAPON][snum]) / 2
							+ aplWeaponFireDelay[HANDBOMB_WEAPON][snum]
							)
						)
					{
						// up and left
						gun_pos += 10 * (kickback_pic - aplWeaponFireDelay[HANDBOMB_WEAPON][snum]); //U
						weapon_xoffset += 80 * (kickback_pic - aplWeaponFireDelay[HANDBOMB_WEAPON][snum]);
					}
					else if (*kb < aplWeaponTotalTime[HANDBOMB_WEAPON][snum])
					{
						gun_pos += 240;	// start high
						gun_pos -= 12 * (kickback_pic - aplWeaponFireDelay[HANDBOMB_WEAPON][snum]);  //D
						// move left
						weapon_xoffset += 90 - (5 * (aplWeaponTotalTime[HANDBOMB_WEAPON][snum] - kickback_pic));
					}
				}
				else
				{
					if (*kb < 7)
						gun_pos -= 10 * kickback_pic;        //D
					else if (*kb < 12)
						gun_pos += 20 * (kickback_pic - 10); //U
					else if (*kb < 20)
						gun_pos -= 9 * (kickback_pic - 14);  //D
				}
				hud_drawpal(weapon_xoffset + 190 - look_anghalf, looking_arc + 250 - gun_pos, HANDTHROW + throw_frames[*kb], shade, o, pal);
			}
			else
				hud_drawpal(weapon_xoffset + 190 - look_anghalf, looking_arc + 260 - gun_pos, HANDTHROW, shade, o, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayhandremote = [&]()
		{
			int8_t remote_frames[] = { 0,1,1,2,1,1,0,0,0,0,0 };

			weapon_xoffset = -48;

			if (*kb)
				hud_drawpal(weapon_xoffset + 150 - look_anghalf, looking_arc + 258 - gun_pos, HANDREMOTE + remote_frames[*kb], shade, o, pal);
			else
				hud_drawpal(weapon_xoffset + 150 - look_anghalf, looking_arc + 258 - gun_pos, HANDREMOTE, shade, o, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaydevastator_ww = [&]
		{
			if (*kb)
			{
				if (*kb < aplWeaponTotalTime[p->curr_weapon][snum])
				{
					i = Sgn(*kb >> 2);
					if (p->ammo_amount[p->curr_weapon] & 1)
					{
						hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, shade, o | 4, pal);
						hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR + i, -32, o, pal);
					}
					else
					{
						hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR + i, -32, o | 4, pal);
						hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, shade, o, pal);
					}
				}
				// else we are in 'reload time'
				else if (*kb <
					(
						(aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum]) / 2
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// down 
					gun_pos -= 10 * (kickback_pic - aplWeaponTotalTime[p->curr_weapon][snum]); //D
//					weapon_xoffset+=80*(*kb-aplWeaponTotalTime[cw][snum]);
					hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, shade, o, pal);
					hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, shade, o | 4, pal);
				}
				else
				{
					// move back down

					// up and left
					gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - kickback_pic); //U
//					weapon_xoffset+=80*(*kb-aplWeaponTotalTime[cw][snum]);
					hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, shade, o, pal);
					hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, shade, o | 4, pal);
				}
			}
			else
			{
				hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, shade, o, pal);
				hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, shade, o | 4, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaydevastator = [&]
		{
			if (*kb)
			{
				static const uint8_t cycloidy[] = { 0,4,12,24,12,4,0 };

				i = Sgn(*kb >> 2);

				if (p->hbomb_hold_delay)
				{
					hud_drawpal((cycloidy[*kb] >> 1) + weapon_xoffset + 268 - look_anghalf, cycloidy[*kb] + looking_arc + 238 - gun_pos, DEVISTATOR + i, -32, o, pal);
					hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, shade, o | 4, pal);
				}
				else
				{
					hud_drawpal(-(cycloidy[*kb] >> 1) + weapon_xoffset + 30 - look_anghalf, cycloidy[*kb] + looking_arc + 240 - gun_pos, DEVISTATOR + i, -32, o | 4, pal);
					hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, shade, o, pal);
				}
			}
			else
			{
				hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, shade, o, pal);
				hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, shade, o | 4, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayfreezer = [&]
		{
			pin = (isWW2GI() || (gs.displayflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;
			auto pic = FREEZE;

			if (*kb)
			{
				static const uint8_t cat_frames[] = { 0,0,1,1,2,2 };

				if (p->GetActor()->s->pal != 1)
				{
					weapon_xoffset += rand() & 3;
					looking_arc += rand() & 3;
				}
				gun_pos -= 16;
				hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 261 - gun_pos, /*isWorldTour() ? FREEZEFIREWIDE :*/ FREEZE + 2, -32, o|pin, pal);
				hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 235 - gun_pos, FREEZE + 3 + cat_frames[*kb % 6], -32, o | pin, pal);
			}
			else hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 261 - gun_pos, /*isWorldTour() ? FREEZEWIDE :*/ FREEZE, shade, o | pin, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshrinker_ww = [&]
		{
			weapon_xoffset += 28;
			looking_arc += 18;

			if (*kb == 0)
			{
				// the 'at rest' display
				if (p->ammo_amount[cw] <= 0) //p->last_weapon >= 0)
				{
					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (*kb & 3), -32,
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 1, shade, o, pal);
				}
				else
				{

					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 2,
						16 - xs_CRoundToInt(bsinf(random_club_frame, -10)),
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER, shade, o, pal);
				}
			}
			else
			{
				// the 'active' display.
				if (p->GetActor()->s->pal != 1)
				{
					weapon_xoffset += rand() & 3;
					gun_pos += (rand() & 3);
				}


				if (*kb < aplWeaponTotalTime[p->curr_weapon][snum])
				{
					if (*kb < aplWeaponFireDelay[p->curr_weapon][snum])
					{
						// before fire time.
						// nothing to modify

					}
					else
					{
						// after fire time.

						// lower weapon to reload cartridge (not clip)
						gun_pos -= 10 * (aplWeaponTotalTime[p->curr_weapon][snum] - kickback_pic);
					}
				}
				// else we are in 'reload time'
				else if (*kb <
					(
						(aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum]) / 2
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// down 
					gun_pos -= 10 * (kickback_pic - aplWeaponTotalTime[p->curr_weapon][snum]); //D
				}
				else
				{
					// up
					gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - kickback_pic); //U
				}

				// draw weapon
				{
					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (*kb & 3), -32,
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 1, shade, o, pal);
				}
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaygrower_ww = [&]
		{
			weapon_xoffset += 28;
			looking_arc += 18;

			if (*kb == 0)
			{
				{
					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER - 2, shade, o, pal);
				}
			}
			else
			{
				if (p->GetActor()->s->pal != 1)
				{
					weapon_xoffset += rand() & 3;
					gun_pos += (rand() & 3);
				}

				if (*kb < aplWeaponTotalTime[p->curr_weapon][snum])
				{
					if (*kb < aplWeaponFireDelay[p->curr_weapon][snum])
					{
						// before fire time.
						// nothing to modify

					}
					else
					{
						// after fire time.

						// lower weapon to reload cartridge (not clip)
						gun_pos -= 15 * (aplWeaponTotalTime[p->curr_weapon][snum] - kickback_pic);
					}
				}
				// else we are in 'reload time'
				else if (*kb <
					(
						(aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum]) / 2
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// down 
					gun_pos -= 5 * (kickback_pic - aplWeaponTotalTime[p->curr_weapon][snum]); //D
				}
				else
				{
					// up
					gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - kickback_pic); //U
				}

				// display weapon
				{
					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (*kb & 3), -32,
						o, 2);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER - 1, shade, o, pal);

				}
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
			weapon_xoffset += 28;
			looking_arc += 18;

			if (*kb == 0)
			{
				if (cw == GROW_WEAPON)
				{
					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 2,
						16 - xs_CRoundToInt(bsinf(random_club_frame, -10)),
						o, 2);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, shrinker - 2, shade, o, pal);
				}
				else
				{
					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 2,
						16 - xs_CRoundToInt(bsinf(random_club_frame, -10)),
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, shrinker, shade, o, pal);
				}
			}
			else
			{
				if (p->GetActor()->s->pal != 1)
				{
					weapon_xoffset += rand() & 3;
					gun_pos += (rand() & 3);
				}

				if (cw == GROW_WEAPON)
				{
					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (*kb & 3), -32,
						o, 2);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, shrinker - 1, shade, o, pal);

				}
				else
				{
					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (*kb & 3), -32,
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, shrinker + 1, shade, o, pal);
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
			if (*kb < 1 || p->cursector()->lotag == 2)
			{
				hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 261 - gun_pos, FLAMETHROWER, shade, o, pal);
				hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 261 - gun_pos, FLAMETHROWERPILOT, shade, o, pal);
			}
			else
			{
				static const uint8_t cat_frames[] = { 0, 0, 1, 1, 2, 2 };
				if (p->GetActor()->s->pal != 1)
				{
					weapon_xoffset += krand() & 1;
					looking_arc += krand() & 1;
				}
				gun_pos -= 16;
				hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 261 - gun_pos, FLAMETHROWER + 1, -32, o, pal);
				hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 235 - gun_pos, FLAMETHROWER + 2 + cat_frames[*kb % 6], -32, o, pal);
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

	displayloogie(p);

}

END_DUKE_NS
