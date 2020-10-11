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

BEGIN_DUKE_NS 
  
double getavel(int snum)
{
	return FixedToFloat(PlayerInputAngVel(screenpeek));
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

void displayloogie(short snum)
{
	int i, a, y, z;
	double x;

	if (ps[snum].loogcnt == 0) return;

	y = (ps[snum].loogcnt << 2);
	for (i = 0; i < ps[snum].numloogs; i++)
	{
		a = abs(calcSinTableValue((ps[snum].loogcnt + i) << 5)) / 32;
		z = 4096 + ((ps[snum].loogcnt + i) << 9);
		x = (-getavel(snum)) + (calcSinTableValue((ps[snum].loogcnt + i) << 6) / 1024.);

		hud_drawsprite(
			(ps[snum].loogiex[i] + x), (200 + ps[snum].loogiey[i] - y), z - (i << 8), 256 - a,
			LOOGIE, 0, 0, 2);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int animatefist(int gs, int snum, double look_anghalf)
{
	short fisti, fistpal;
	int fistzoom;
	double looking_arc, fistz;

	fisti = ps[snum].fist_incs;
	if (fisti > 32) fisti = 32;
	if (fisti <= 0) return 0;

	looking_arc = fabs(look_anghalf) / 4.5;

	fistzoom = 65536L - (calcSinTableValue(512 + (fisti << 6)) * 4);
	if (fistzoom > 90612L)
		fistzoom = 90612L;
	if (fistzoom < 40920)
		fistzoom = 40290;
	fistz = 194 + (calcSinTableValue(((6 + fisti) << 7) & 2047) / 512.);

	if (sprite[ps[snum].i].pal == 1)
		fistpal = 1;
	else
		fistpal = sector[ps[snum].cursectnum].floorpal;

	hud_drawsprite(
		(-fisti + 222 + (getavel(snum) / 16.)),
		(looking_arc + fistz),
		fistzoom, 0, FIST, gs, fistpal, 2);

	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int animateknee(int gs, int snum, double hard_landing, double look_anghalf, double horiz16th)
{
	static const short knee_y[] = { 0,-8,-16,-32,-64,-84,-108,-108,-108,-72,-32,-8 };
	short pal;
	double looking_arc;

	if (ps[snum].knee_incs > 11 || ps[snum].knee_incs == 0 || sprite[ps[snum].i].extra <= 0) return 0;

	looking_arc = knee_y[ps[snum].knee_incs] + (fabs(look_anghalf) / 4.5);

	looking_arc -= hard_landing * 8.;

	if (sprite[ps[snum].i].pal == 1)
		pal = 1;
	else
	{
		pal = sector[ps[snum].cursectnum].floorpal;
		if (pal == 0)
			pal = ps[snum].palookup;
	}

	hud_drawpal(105 + (getavel(snum) / 16.) - look_anghalf + (knee_y[ps[snum].knee_incs] >> 2), looking_arc + 280 - horiz16th, KNEE, gs, 4, pal);

	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int animateknuckles(int gs, int snum, double hard_landing, double look_anghalf, double horiz16th)
{
	static const short knuckle_frames[] = { 0,1,2,2,3,3,3,2,2,1,0 };
	short pal;
	double looking_arc;

	if (isWW2GI() || ps[snum].over_shoulder_on != 0 || ps[snum].knuckle_incs == 0 || sprite[ps[snum].i].extra <= 0) return 0;

	looking_arc = fabs(look_anghalf) / 4.5;

	looking_arc -= hard_landing * 8.;

	if (sprite[ps[snum].i].pal == 1)
		pal = 1;
	else
		pal = sector[ps[snum].cursectnum].floorpal;

	hud_drawpal(160 + (getavel(snum) / 16.) - look_anghalf, looking_arc + 180 - horiz16th, CRACKKNUCKLES + knuckle_frames[ps[snum].knuckle_incs >> 1], gs, 4, pal);

	return 1;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displaymasks_d(int snum, double)
{
	int p;

	if (sprite[ps[snum].i].pal == 1)
		p = 1;
	else
		p = sector[ps[snum].cursectnum].floorpal;

	if (ps[snum].scuba_on)
	{
		hud_drawsprite(44, (200 - tilesiz[SCUBAMASK].y), 65536, 0, SCUBAMASK, 0, p, 2 + 16);
		hud_drawsprite((320 - 43), (200 - tilesiz[SCUBAMASK].y), 65536, 0, SCUBAMASK, 0, p, 2 + 4 + 16);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int animatetip(int gs, int snum, double hard_landing, double look_anghalf, double horiz16th)
{
	int p;
	double looking_arc;
	static const short tip_y[] = { 0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16 };

	if (ps[snum].tipincs == 0) return 0;

	looking_arc = fabs(look_anghalf) / 4.5;
	looking_arc -= hard_landing * 8.;

	if (sprite[ps[snum].i].pal == 1)
		p = 1;
	else
		p = sector[ps[snum].cursectnum].floorpal;

	/*    if(ps[snum].access_spritenum >= 0)
			p = sprite[ps[snum].access_spritenum].pal;
		else
			p = wall[ps[snum].access_wallnum].pal;
	  */
	hud_drawpal(170 + (getavel(snum) / 16.) - look_anghalf,
		(tip_y[ps[snum].tipincs] >> 1) + looking_arc + 240 - horiz16th, TIP + ((26 - ps[snum].tipincs) >> 4), gs, 0, p);

	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int animateaccess(int gs,int snum,double hard_landing,double look_anghalf,double horiz16th)
{
	static const short access_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16};
	double looking_arc;
	char p;

	if(ps[snum].access_incs == 0 || sprite[ps[snum].i].extra <= 0) return 0;

	looking_arc = access_y[ps[snum].access_incs] + (fabs(look_anghalf) / 4.5);
	looking_arc -= hard_landing * 8.;

	if(ps[snum].access_spritenum >= 0)
		p = sprite[ps[snum].access_spritenum].pal;
	else p = 0;
//    else
//        p = wall[ps[snum].access_wallnum].pal;

	if((ps[snum].access_incs-3) > 0 && (ps[snum].access_incs-3)>>3)
		hud_drawpal(170+(getavel(snum)/16.)-look_anghalf+(access_y[ps[snum].access_incs]>>2),looking_arc+266-horiz16th,HANDHOLDINGLASER+(ps[snum].access_incs>>3),gs,0,p);
	else
		hud_drawpal(170+(getavel(snum)/16.)-look_anghalf+(access_y[ps[snum].access_incs]>>2),looking_arc+266-horiz16th,HANDHOLDINGACCESS,gs,4,p);

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
	int o,pal;
	double weapon_sway, weapon_xoffset, gun_pos, looking_arc, kickback_pic, random_club_frame, hard_landing, look_anghalf, horiz16th;
	signed char gs;
	struct player_struct *p;

	p = &ps[snum];
	auto kb = &p->kickback_pic;
	int pin = 0;

	o = 0;

	horiz16th = get16thOfHoriz(snum, cl_syncinput, smoothratio);
	look_anghalf = getHalfLookAng(p->angle.olook_ang.asq16(), p->angle.look_ang.asq16(), cl_syncinput, smoothratio);
	looking_arc = fabs(look_anghalf) / 4.5;
	weapon_sway = p->oweapon_sway + fmulscale16(p->weapon_sway - p->oweapon_sway, smoothratio);
	kickback_pic = p->okickback_pic + fmulscale16(*kb - p->okickback_pic, smoothratio);
	random_club_frame = p->orandom_club_frame + fmulscale16(p->random_club_frame - p->orandom_club_frame, smoothratio);
	hard_landing = p->ohard_landing + fmulscale16(p->hard_landing - p->ohard_landing, smoothratio);

	gs = sprite[p->i].shade;
	if(gs > 24) gs = 24;

	bool playerVars  = p->newowner >= 0 || ud.camerasprite >= 0 || p->over_shoulder_on > 0 || (sprite[p->i].pal != 1 && sprite[p->i].extra <= 0);
	bool playerAnims = animatefist(gs,snum,look_anghalf) || animateknuckles(gs,snum,hard_landing,look_anghalf,horiz16th) ||
					   animatetip(gs,snum,hard_landing,look_anghalf,horiz16th) || animateaccess(gs,snum,hard_landing,look_anghalf,horiz16th);

	if(playerVars || playerAnims)
		return;

	animateknee(gs,snum,hard_landing,look_anghalf,horiz16th);

	int opos = p->oweapon_pos * p->oweapon_pos;
	int npos = p->weapon_pos * p->weapon_pos;
	gun_pos = 80 - (opos + fmulscale16(npos - opos, smoothratio));

	weapon_xoffset =  (160)-90;
	weapon_xoffset -= calcSinTableValue((weapon_sway / 2.) + 512) / (1024. + 512.);
	weapon_xoffset -= 58 + p->weapon_ang;
	if( sprite[p->i].xrepeat < 32 )
		gun_pos -= fabs(calcSinTableValue(weapon_sway * 4.) / 512.);
	else gun_pos -= fabs(calcSinTableValue(weapon_sway / 2.) / 1024.);

	gun_pos -= hard_landing * 8.;

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
		if (sprite[p->i].pal == 1)
			pal = 1;
		else
		{
			pal = sector[p->cursectnum].floorpal;
			if (pal == 0)
				pal = p->palookup;
		}


		if (j < 5 || j > 9)
		{
			hud_drawpal(weapon_xoffset + 80 - look_anghalf, looking_arc + 250 - gun_pos, KNEE, gs, o | 4, pal);
		}
		else
		{
			hud_drawpal(weapon_xoffset + 160 - 16 - look_anghalf, looking_arc + 214 - gun_pos, KNEE + 1, gs, o | 4, pal);
		}
	}

	if (sprite[p->i].xrepeat < 40)
	{
		static int fistsign;
		//shrunken..
		if (p->jetpack_on == 0)
		{
			i = sprite[p->i].xvel;
			looking_arc += 32 - (i >> 1);
			fistsign += i >> 1;
		}
		cw = weapon_xoffset;
		weapon_xoffset += calcSinTableValue(fistsign) / 1024.;
		hud_draw(weapon_xoffset + 250 - look_anghalf,
			looking_arc + 258 - (fabs(calcSinTableValue(fistsign) / 256.)),
			FIST, gs, o);
		weapon_xoffset = cw;
		weapon_xoffset -= calcSinTableValue(fistsign) / 1024.;
		hud_draw(weapon_xoffset + 40 - look_anghalf,
			looking_arc + 200 + (fabs(calcSinTableValue(fistsign) / 256.)),
			FIST, gs, o | 4);
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
				if (sprite[p->i].pal == 1)
					pal = 1;
				else
				{
					pal = sector[p->cursectnum].floorpal;
					if (pal == 0)
						pal = p->palookup;
				}

				if (*kb < 5 || *kb > 9)
				{
					hud_drawpal(weapon_xoffset + 220 - look_anghalf,
						looking_arc + 250 - gun_pos, KNEE, gs, o, pal);
				}
				else
				{
					hud_drawpal(weapon_xoffset + 160 - look_anghalf,
						looking_arc + 214 - gun_pos, KNEE + 1, gs, o, pal);
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

			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;

			weapon_xoffset += 8;
			gun_pos -= 10;

			if (*kb > 6)
				looking_arc += kickback_pic * 8.;
			else if (*kb < 4)
				hud_drawpal(weapon_xoffset + 142 - look_anghalf,
					looking_arc + 234 - gun_pos, HANDHOLDINGLASER + 3, gs, o, pal);

			hud_drawpal(weapon_xoffset + 130 - look_anghalf,
				looking_arc + 249 - gun_pos,
				HANDHOLDINGLASER + (*kb >> 2), gs, o, pal);
			hud_drawpal(weapon_xoffset + 152 - look_anghalf,
				looking_arc + 249 - gun_pos,
				HANDHOLDINGLASER + (*kb >> 2), gs, o | 4, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayrpg = [&]()
		{
			//pin = (isWorldTour() || (duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;
			auto rpgpic = /*isWorldTour() ? RPGGUNWIDE :*/ RPGGUN;

			if (sprite[p->i].pal == 1)
				pal = 1;
			else pal = sector[p->cursectnum].floorpal;

			weapon_xoffset -= calcSinTableValue(768 + (kickback_pic * 128.)) / 2048.;
			gun_pos += calcSinTableValue(768 + (kickback_pic * 128.)) / 2048.;

			if (*kb > 0)
			{
				if (*kb < (isWW2GI() ? aplWeaponTotalTime[RPG_WEAPON][snum] : 8))
				{
					hud_drawpal(weapon_xoffset + 164, (looking_arc * 2.) + 176 - gun_pos,
						RPGGUN + (*kb >> 1), gs, o | pin, pal);
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

			hud_drawpal(weapon_xoffset + 164, (looking_arc * 2.) + 176 - gun_pos, rpgpic, gs, o | pin, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshotgun_ww = [&]()
		{
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;


			if (*kb > 0)
			{
				gun_pos -= calcSinTableValue(kickback_pic * 128.) / 4096.;
			}

			if (*kb > 0 && sprite[p->i].pal != 1)
			{
				weapon_xoffset += 1 - (rand() & 3);
			}

			weapon_xoffset -= 8;

			if (*kb == 0)
			{
				hud_drawpal(weapon_xoffset + 146 - look_anghalf, looking_arc + 202 - gun_pos,
					SHOTGUN, gs, o, pal);
			}
			else if (*kb <= aplWeaponTotalTime[SHOTGUN_WEAPON][snum])
			{
				hud_drawpal(weapon_xoffset + 146 - look_anghalf, looking_arc + 202 - gun_pos,
					SHOTGUN + 1, gs, o, pal);
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
				hud_drawpal(weapon_xoffset + 146 - look_anghalf, looking_arc + 202 - gun_pos,
					SHOTGUN, gs, o, pal);
			}
			else
			{
				// move back down

				// up and left
				gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - kickback_pic); //U
//					weapon_xoffset+=80*(*kb-aplWeaponTotalTime[cw][snum]);
				hud_drawpal(weapon_xoffset + 146 - look_anghalf, looking_arc + 202 - gun_pos,
					SHOTGUN, gs, o, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayshotgun = [&]()
		{
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;


			weapon_xoffset -= 8;

			switch(*kb)
			{
				case 1:
				case 2:
					hud_drawpal(weapon_xoffset + 168 - look_anghalf,looking_arc + 201 - gun_pos,
					   SHOTGUN + 2,-128,o,pal);
				case 0:
				case 6:
				case 7:
				case 8:
					hud_drawpal(weapon_xoffset + 146 - look_anghalf,looking_arc + 202 - gun_pos,
						SHOTGUN,gs,o,pal);
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

						hud_drawpal(weapon_xoffset + 178 - look_anghalf,looking_arc + 194 - gun_pos,
							SHOTGUN + 1 + ((*(kb)-1) >> 1),-128,o,pal);
					}

					hud_drawpal(weapon_xoffset + 158 - look_anghalf,looking_arc + 220 - gun_pos,
						SHOTGUN + 3,gs,o,pal);

					break;
				case 13:
				case 14:
				case 15:
					hud_drawpal(32 + weapon_xoffset + 166 - look_anghalf,looking_arc + 210 - gun_pos,
						SHOTGUN + 4,gs,o,pal);
					break;
				case 16:
				case 17:
				case 18:
				case 19:
					hud_drawpal(64 + weapon_xoffset + 170 - look_anghalf,looking_arc + 196 - gun_pos,
						SHOTGUN + 5,gs,o,pal);
					break;
				case 20:
				case 21:
				case 22:
				case 23:
					hud_drawpal(64 + weapon_xoffset + 176 - look_anghalf,looking_arc + 196 - gun_pos,
						SHOTGUN + 6,gs,o,pal);
					break;
				case 24:
				case 25:
				case 26:
				case 27:
					hud_drawpal(64 + weapon_xoffset + 170 - look_anghalf,looking_arc + 196 - gun_pos,
						SHOTGUN + 5,gs,o,pal);
					break;
				case 28:
				case 29:
				case 30:
					hud_drawpal(32 + weapon_xoffset + 156 - look_anghalf,looking_arc + 206 - gun_pos,
						SHOTGUN + 4,gs,o,pal);
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
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;

			if (*kb > 0)
				gun_pos -= calcSinTableValue(kickback_pic * 128.) / 4096.;

			if (*kb > 0 && sprite[p->i].pal != 1) weapon_xoffset += 1 - (rand() & 3);

			if (*kb == 0)
			{
				//				hud_drawpal(weapon_xoffset+168-look_anghalf,looking_arc+260-gun_pos,
				//						CHAINGUN,gs,o,pal);
				hud_drawpal(weapon_xoffset + 178 - look_anghalf, looking_arc + 233 - gun_pos,
					CHAINGUN + 1, gs, o, pal);
			}
			else if (*kb <= aplWeaponTotalTime[CHAINGUN_WEAPON][snum])
			{
				hud_drawpal(weapon_xoffset + 188 - look_anghalf, looking_arc + 243 - gun_pos,
					CHAINGUN + 2, gs, o, pal);
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
					gun_pos += 80 - (10 * (aplWeaponTotalTime[p->curr_weapon][snum]
						+ iFifths - kickback_pic));
					weapon_xoffset += 80 - (10 * (aplWeaponTotalTime[p->curr_weapon][snum]
						+ iFifths - kickback_pic));
					hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos,
						2519, gs, o, pal);
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
					hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos,
						2518, gs, o, pal);
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
					hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos,
						2517, gs, o, pal);
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
					hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos, 2518, gs, o, pal);
				}
				else
				{
					// move back down

					// up and left
					gun_pos += 10 * (aplWeaponReload[p->curr_weapon][snum] - kickback_pic);
					//5*(aplWeaponReload[p->curr_weapon][snum]- p->kickback_pic); //U
					weapon_xoffset += 10 * (aplWeaponReload[p->curr_weapon][snum] - kickback_pic);
					//80*(*kb-aplWeaponTotalTime[cw][snum]);
					hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos, 2519, gs, o, pal);
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
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;

			if (*kb > 0)
				gun_pos -= calcSinTableValue(kickback_pic * 128.) / 4096.;

			if (*kb > 0 && sprite[p->i].pal != 1) weapon_xoffset += 1 - (rand() & 3);

			hud_drawpal(weapon_xoffset + 168 - look_anghalf, looking_arc + 260 - gun_pos, CHAINGUN, gs, o, pal);
			switch(*kb)
			{
				case 0:
					hud_drawpal(weapon_xoffset + 178 - look_anghalf,looking_arc + 233 - gun_pos, CHAINGUN + 1,gs,o,pal);
					break;
				default:
					if (*kb > 4 && *kb < 12)
					{
						i = 0;
						if (sprite[p->i].pal != 1) i = rand() & 7;
						hud_drawpal(i + weapon_xoffset - 4 + 140 - look_anghalf,i + looking_arc - (kickback_pic / 2.) + 208 - gun_pos, CHAINGUN + 5 + ((*kb - 4) / 5),gs,o,pal);
						if (sprite[p->i].pal != 1) i = rand() & 7;
						hud_drawpal(i + weapon_xoffset - 4 + 184 - look_anghalf,i + looking_arc - (kickback_pic / 2.) + 208 - gun_pos, CHAINGUN + 5 + ((*kb - 4) / 5),gs,o,pal);
					}
					if (*kb < 8)
					{
						i = rand() & 7;
						hud_drawpal(i + weapon_xoffset - 4 + 162 - look_anghalf,i + looking_arc - (kickback_pic / 2.) + 208 - gun_pos, CHAINGUN + 5 + ((*kb - 2) / 5),gs,o,pal);
						hud_drawpal(weapon_xoffset + 178 - look_anghalf,looking_arc + 233 - gun_pos, CHAINGUN + 1 + (*kb >> 1),gs,o,pal);
					}
					else hud_drawpal(weapon_xoffset + 178 - look_anghalf,looking_arc + 233 - gun_pos, CHAINGUN + 1,gs,o,pal);
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
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;

			if (*kb < 5)
			{
				short kb_frames[] = { 0,1,2,0,0 };

				double l = 195 - 12 + weapon_xoffset;

				if (*kb == 2)
					l -= 3;
				{
					hud_drawpal(
						(l - look_anghalf),
						(looking_arc + 244 - gun_pos),
						FIRSTGUN + kb_frames[*kb],
						gs, 2, pal);
				}
			}
			else
			{
				//pin = 0; (isWW2GI() || isWorldTour() || (duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;
				auto pic_5 = /*isWorldTour() ? FIRSTGUNRELOADWIDE :*/ FIRSTGUN+5;

				const int WEAPON2_RELOAD_TIME = 50;
				auto reload_time = isWW2GI() ? aplWeaponReload[PISTOL_WEAPON][snum] : WEAPON2_RELOAD_TIME;
				if (*kb < 10)
					hud_drawpal(194 - look_anghalf, looking_arc + 230 - gun_pos, FIRSTGUN + 4, gs, o|pin, pal);
				else if (*kb < 15)
				{
					hud_drawpal(244 - (kickback_pic * 8.) - look_anghalf, looking_arc + 130 - gun_pos + (kickback_pic * 16.), FIRSTGUN + 6, gs, o | pin, pal);
					hud_drawpal(224 - look_anghalf, looking_arc + 220 - gun_pos, pic_5, gs, o | pin, pal);
				}
				else if (*kb < 20)
				{
					hud_drawpal(124 + (kickback_pic * 2.) - look_anghalf, looking_arc + 430 - gun_pos - (kickback_pic * 8.), FIRSTGUN + 6, gs, o | pin, pal);
					hud_drawpal(224 - look_anghalf, looking_arc + 220 - gun_pos, pic_5, gs, o | pin, pal);
				}
				else if (*kb < (isNamWW2GI()? (reload_time - 12) : 23))
				{
					hud_drawpal(184 - look_anghalf, looking_arc + 235 - gun_pos, FIRSTGUN + 8, gs, o | pin, pal);
					hud_drawpal(224 - look_anghalf, looking_arc + 210 - gun_pos, pic_5, gs, o | pin, pal);
				}
				else if (*kb < (isNamWW2GI()? (reload_time - 6) : 25))
				{
					hud_drawpal(164 - look_anghalf, looking_arc + 245 - gun_pos, FIRSTGUN + 8, gs, o | pin, pal);
					hud_drawpal(224 - look_anghalf, looking_arc + 220 - gun_pos, pic_5, gs, o | pin, pal);
				}
				else if (*kb < (isNamWW2GI()? reload_time : 27))
					hud_drawpal(194 - look_anghalf, looking_arc + 235 - gun_pos, pic_5, gs, o, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayhandbomb = [&]()
		{
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;

			if (*kb)
			{
				static const uint8_t throw_frames[]
					= { 0,0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2 };

				if (isWW2GI())
				{
					if (*kb <= aplWeaponFireDelay[HANDBOMB_WEAPON][snum])
					{
						// it holds here
						gun_pos -= 5 * kickback_pic;        //D
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
				hud_drawpal(weapon_xoffset + 190 - look_anghalf, looking_arc + 250 - gun_pos, HANDTHROW + throw_frames[*kb], gs, o, pal);
			}
			else
				hud_drawpal(weapon_xoffset + 190 - look_anghalf, looking_arc + 260 - gun_pos, HANDTHROW, gs, o, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayhandremote = [&]()
		{
			signed char remote_frames[] = { 0,1,1,2,1,1,0,0,0,0,0 };
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;

			weapon_xoffset = -48;

			if (*kb)
				hud_drawpal(weapon_xoffset + 150 - look_anghalf, looking_arc + 258 - gun_pos, HANDREMOTE + remote_frames[*kb], gs, o, pal);
			else
				hud_drawpal(weapon_xoffset + 150 - look_anghalf, looking_arc + 258 - gun_pos, HANDREMOTE, gs, o, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaydevastator_ww = [&]
		{
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;

			if (*kb)
			{
				if (*kb < aplWeaponTotalTime[p->curr_weapon][snum])
				{
					i = sgn(*kb >> 2);
					if (p->ammo_amount[p->curr_weapon] & 1)
					{
						hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
						hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR + i, -32, o, pal);
					}
					else
					{
						hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR + i, -32, o | 4, pal);
						hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
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
					hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
					hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
				}
				else
				{
					// move back down

					// up and left
					gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - kickback_pic); //U
//					weapon_xoffset+=80*(*kb-aplWeaponTotalTime[cw][snum]);
					hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
					hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
				}
			}
			else
			{
				hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
				hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displaydevastator = [&]
		{
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;

			if (*kb)
			{
				char cycloidy[] = { 0,4,12,24,12,4,0 };

				i = sgn(*kb >> 2);

				if (p->hbomb_hold_delay)
				{
					hud_drawpal((cycloidy[*kb] >> 1) + weapon_xoffset + 268 - look_anghalf, cycloidy[*kb] + looking_arc + 238 - gun_pos, DEVISTATOR + i, -32, o, pal);
					hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
				}
				else
				{
					hud_drawpal(-(cycloidy[*kb] >> 1) + weapon_xoffset + 30 - look_anghalf, cycloidy[*kb] + looking_arc + 240 - gun_pos, DEVISTATOR + i, -32, o | 4, pal);
					hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
				}
			}
			else
			{
				hud_drawpal(weapon_xoffset + 268 - look_anghalf, looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
				hud_drawpal(weapon_xoffset + 30 - look_anghalf, looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayfreezer = [&]
		{
			//pin = (isWW2GI() || isWorldTour() || (duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;
			auto pic = /*isWorldTour() ? FREEZEWIDE :*/ FREEZE;

			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;

			if (*kb)
			{
				char cat_frames[] = { 0,0,1,1,2,2 };

				if (sprite[p->i].pal != 1)
				{
					weapon_xoffset += rand() & 3;
					looking_arc += rand() & 3;
				}
				gun_pos -= 16;
				hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 261 - gun_pos, /*isWorldTour() ? FREEZEFIREWIDE :*/ FREEZE + 2, -32, o|pin, pal);
				hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 235 - gun_pos, FREEZE + 3 + cat_frames[*kb % 6], -32, o | pin, pal);
			}
			else hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 261 - gun_pos, /*isWorldTour() ? FREEZEWIDE :*/ FREEZE, gs, o | pin, pal);
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
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;
			if (*kb == 0)
			{
				// the 'at rest' display
				if (ps[snum].ammo_amount[cw] <= 0) //p->last_weapon >= 0)
				{
					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (*kb & 3), -32,
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 1, gs, o, pal);
				}
				else
				{

					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 2,
						16 - xs_CRoundToInt(calcSinTableValue(random_club_frame) / 1024.),
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER, gs, o, pal);
				}
			}
			else
			{
				// the 'active' display.
				if (sprite[p->i].pal != 1)
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
						looking_arc + 240 - gun_pos, SHRINKER + 1, gs, o, pal);
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
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;
			if (*kb == 0)
			{
				{
					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER - 2, gs, o, pal);
				}
			}
			else
			{
				if (sprite[p->i].pal != 1)
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
						looking_arc + 240 - gun_pos, SHRINKER - 1, gs, o, pal);

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
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;
			if (*kb == 0)
			{
				if (cw == GROW_WEAPON)
				{
					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 2,
						16 - xs_CRoundToInt(calcSinTableValue(random_club_frame) / 1024.),
						o, 2);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, shrinker - 2, gs, o, pal);
				}
				else
				{
					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 2,
						16 - xs_CRoundToInt(calcSinTableValue(random_club_frame) / 1024.),
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, shrinker, gs, o, pal);
				}
			}
			else
			{
				if (sprite[p->i].pal != 1)
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
						looking_arc + 240 - gun_pos, shrinker - 1, gs, o, pal);

				}
				else
				{
					hud_drawpal(weapon_xoffset + 184 - look_anghalf,
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (*kb & 3), -32,
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - look_anghalf,
						looking_arc + 240 - gun_pos, shrinker + 1, gs, o, pal);
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
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
			{
				if (p->cursectnum < 0)
					pal = 0;
				else
					pal = sector[p->cursectnum].floorpal;
			}

			if (*kb < 1 || sector[p->cursectnum].lotag == 2)
			{
				hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 261 - gun_pos, FLAMETHROWER, gs, o, pal);
				hud_drawpal(weapon_xoffset + 210 - look_anghalf, looking_arc + 261 - gun_pos, FLAMETHROWERPILOT, gs, o, pal);
			}
			else
			{
				static const uint8_t cat_frames[] = { 0, 0, 1, 1, 2, 2 };
				if (sprite[p->i].pal != 1)
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

	displayloogie(snum);

}

END_DUKE_NS
