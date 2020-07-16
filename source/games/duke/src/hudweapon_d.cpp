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
  
int getavel(int snum)
{
	return PlayerInputAngVel(screenpeek) >> FRACBITS;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

inline static void hud_drawpal(double x, double y, int tilenum, int shade, int orientation, int p)
{
	hud_drawsprite(x, y, 65536, (orientation & 4) ? 1024 : 0, tilenum, shade, p, 2 | orientation);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displayloogie(short snum)
{
	int i, a, x, y, z;

	if (ps[snum].loogcnt == 0) return;

	y = (ps[snum].loogcnt << 2);
	for (i = 0; i < ps[snum].numloogs; i++)
	{
		a = abs(sintable[((ps[snum].loogcnt + i) << 5) & 2047]) >> 5;
		z = 4096 + ((ps[snum].loogcnt + i) << 9);
		x = (-getavel(snum)) + (sintable[((ps[snum].loogcnt + i) << 6) & 2047] >> 10);

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

int animatefist(int gs, int snum)
{
	short looking_arc, fisti, fistpal;
	int fistzoom, fistz;

	fisti = ps[snum].fist_incs;
	if (fisti > 32) fisti = 32;
	if (fisti <= 0) return 0;

	looking_arc = abs(ps[snum].getlookang()) / 9;

	fistzoom = 65536L - (sintable[(512 + (fisti << 6)) & 2047] << 2);
	if (fistzoom > 90612L)
		fistzoom = 90612L;
	if (fistzoom < 40920)
		fistzoom = 40290;
	fistz = 194 + (sintable[((6 + fisti) << 7) & 2047] >> 9);

	if (sprite[ps[snum].i].pal == 1)
		fistpal = 1;
	else
		fistpal = sector[ps[snum].cursectnum].floorpal;

	hud_drawsprite(
		(-fisti + 222 + (getavel(snum) >> 4)),
		(looking_arc + fistz),
		fistzoom, 0, FIST, gs, fistpal, 2);

	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int animateknee(int gs, int snum)
{
	static const short knee_y[] = { 0,-8,-16,-32,-64,-84,-108,-108,-108,-72,-32,-8 };
	short looking_arc, pal;

	if (ps[snum].knee_incs > 11 || ps[snum].knee_incs == 0 || sprite[ps[snum].i].extra <= 0) return 0;

	looking_arc = knee_y[ps[snum].knee_incs] + abs(ps[snum].getlookang()) / 9;

	looking_arc -= (ps[snum].hard_landing << 3);

	if (sprite[ps[snum].i].pal == 1)
		pal = 1;
	else
	{
		pal = sector[ps[snum].cursectnum].floorpal;
		if (pal == 0)
			pal = ps[snum].palookup;
	}

	hud_drawpal(105 + (getavel(snum) >> 4) - (ps[snum].getlookang() >> 1) + (knee_y[ps[snum].knee_incs] >> 2), looking_arc + 280 - ((ps[snum].gethoriz() - ps[snum].gethorizof()) >> 4), KNEE, gs, 4, pal);

	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int animateknuckles(int gs, int snum)
{
	static const short knuckle_frames[] = { 0,1,2,2,3,3,3,2,2,1,0 };
	short looking_arc, pal;

	if (isWW2GI()) return 0;

	if (ps[snum].knuckle_incs == 0 || sprite[ps[snum].i].extra <= 0) return 0;

	looking_arc = abs(ps[snum].getlookang()) / 9;

	looking_arc -= (ps[snum].hard_landing << 3);

	if (sprite[ps[snum].i].pal == 1)
		pal = 1;
	else
		pal = sector[ps[snum].cursectnum].floorpal;

	auto pic = isWorldTour() ? CRACKKNUCKLESWIDE : CRACKKNUCKLES;
	hud_drawpal(160 + (getavel(snum) >> 4) - (ps[snum].getlookang() >> 1), looking_arc + 180 - ((ps[snum].gethoriz() - ps[snum].gethorizof()) >> 4), pic + knuckle_frames[ps[snum].knuckle_incs >> 1], gs, 4, pal);

	return 1;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displaymasks_d(int snum)
{
	int p;

	if (sprite[ps[snum].i].pal == 1)
		p = 1;
	else
		p = sector[ps[snum].cursectnum].floorpal;

	if (ps[snum].scuba_on)
	{
		if (ud.screen_size > 4)
		{
			hud_drawsprite(44, (200 - 8 - tilesiz[SCUBAMASK].y), 65536, 0, SCUBAMASK, 0, p, 2 + 16);
			hud_drawsprite((320 - 43), (200 - 8 - tilesiz[SCUBAMASK].y), 65536, 1024, SCUBAMASK, 0, p, 2 + 4 + 16);
		}
		else
		{
			hud_drawsprite(44 << 16, (200 - tilesiz[SCUBAMASK].y) << 16, 65536, 0, SCUBAMASK, 0, p, 2 + 16);
			hud_drawsprite((320 - 43), (200 - tilesiz[SCUBAMASK].y), 65536, 1024, SCUBAMASK, 0, p, 2 + 4 + 16);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int animatetip(int gs, int snum)
{
	int p, looking_arc;
	static const short tip_y[] = { 0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16 };

	if (ps[snum].tipincs == 0) return 0;

	looking_arc = abs(ps[snum].getlookang()) / 9;
	looking_arc -= (ps[snum].hard_landing << 3);

	if (sprite[ps[snum].i].pal == 1)
		p = 1;
	else
		p = sector[ps[snum].cursectnum].floorpal;

	/*    if(ps[snum].access_spritenum >= 0)
			p = sprite[ps[snum].access_spritenum].pal;
		else
			p = wall[ps[snum].access_wallnum].pal;
	  */
	hud_drawpal(170 + (getavel(snum) >> 4) - (ps[snum].getlookang() >> 1),
		(tip_y[ps[snum].tipincs] >> 1) + looking_arc + 240 - ((ps[snum].gethoriz() - ps[snum].gethorizof()) >> 4), TIP + ((26 - ps[snum].tipincs) >> 4), gs, 0, p);

	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int animateaccess(int gs,int snum)
{
	static const short access_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16};
	short looking_arc;
	char p;

	if(ps[snum].access_incs == 0 || sprite[ps[snum].i].extra <= 0) return 0;

	looking_arc = access_y[ps[snum].access_incs] + abs(ps[snum].getlookang())/9;
	looking_arc -= (ps[snum].hard_landing<<3);

	if(ps[snum].access_spritenum >= 0)
		p = sprite[ps[snum].access_spritenum].pal;
	else p = 0;
//    else
//        p = wall[ps[snum].access_wallnum].pal;

	if((ps[snum].access_incs-3) > 0 && (ps[snum].access_incs-3)>>3)
		hud_drawpal(170+(getavel(snum)>>4)-(ps[snum].getlookang()>>1)+(access_y[ps[snum].access_incs]>>2),looking_arc+266-((ps[snum].gethoriz()-ps[snum].gethorizof())>>4),HANDHOLDINGLASER+(ps[snum].access_incs>>3),gs,0,p);
	else
		hud_drawpal(170+(getavel(snum)>>4)-(ps[snum].getlookang()>>1)+(access_y[ps[snum].access_incs]>>2),looking_arc+266-((ps[snum].gethoriz()-ps[snum].gethorizof())>>4),HANDHOLDINGACCESS,gs,4,p);

	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displayweapon_d(int snum)
{
	int gun_pos, looking_arc, cw;
	int weapon_xoffset, i, j;
	int o,pal;
	signed char gs;
	struct player_struct *p;

	p = &ps[snum];
	auto kb = &p->kickback_pic;
	int pin = 0;

	o = 0;

	looking_arc = abs(p->getlookang())/9;

	gs = sprite[p->i].shade;
	if(gs > 24) gs = 24;

	if(p->newowner >= 0 || ud.camerasprite >= 0 || p->over_shoulder_on > 0 || (sprite[p->i].pal != 1 && sprite[p->i].extra <= 0) || animatefist(gs,snum) || animateknuckles(gs,snum) || animatetip(gs,snum) || animateaccess(gs,snum) )
		return;

	animateknee(gs,snum);

	gun_pos = 80-(p->weapon_pos*p->weapon_pos);

	weapon_xoffset =  (160)-90;
	weapon_xoffset -= (sintable[((p->weapon_sway>>1)+512)&2047]/(1024+512));
	weapon_xoffset -= 58 + p->weapon_ang;
	if( sprite[p->i].xrepeat < 32 )
		gun_pos -= abs(sintable[(p->weapon_sway<<2)&2047]>>9);
	else gun_pos -= abs(sintable[(p->weapon_sway>>1)&2047]>>10);

	gun_pos -= (p->hard_landing<<3);

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
	if (j != 14)
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
			hud_drawpal(weapon_xoffset + 80 - (p->getlookang() >> 1),
				looking_arc + 250 - gun_pos, KNEE, gs, o | 4, pal);
		else hud_drawpal(weapon_xoffset + 160 - 16 - (p->getlookang() >> 1),
			looking_arc + 214 - gun_pos, KNEE + 1, gs, o | 4, pal);
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
		weapon_xoffset += sintable[(fistsign) & 2047] >> 10;
		hud_draw(weapon_xoffset + 250 - (p->getlookang() >> 1),
			looking_arc + 258 - (abs(sintable[(fistsign) & 2047] >> 8)),
			FIST, gs, o);
		weapon_xoffset = cw;
		weapon_xoffset -= sintable[(fistsign) & 2047] >> 10;
		hud_draw(weapon_xoffset + 40 - (p->getlookang() >> 1),
			looking_arc + 200 + (abs(sintable[(fistsign) & 2047] >> 8)),
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
			if (p->kickback_pic > 0)
			{
				if (sprite[p->i].pal == 1)
					pal = 1;
				else
				{
					pal = sector[p->cursectnum].floorpal;
					if (pal == 0)
						pal = p->palookup;
				}

				if (p->kickback_pic < 5 || p->kickback_pic > 9)
				{
					hud_drawpal(weapon_xoffset + 220 - (p->getlookang() >> 1),
						looking_arc + 250 - gun_pos, KNEE, gs, o, pal);
				}
				else
				{
					hud_drawpal(weapon_xoffset + 160 - (p->getlookang() >> 1),
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

			if (p->kickback_pic > 6)
				looking_arc += (p->kickback_pic << 3);
			else if (p->kickback_pic < 4)
				hud_drawpal(weapon_xoffset + 142 - (p->getlookang() >> 1),
					looking_arc + 234 - gun_pos, HANDHOLDINGLASER + 3, gs, o, pal);

			hud_drawpal(weapon_xoffset + 130 - (p->getlookang() >> 1),
				looking_arc + 249 - gun_pos,
				HANDHOLDINGLASER + (p->kickback_pic >> 2), gs, o, pal);
			hud_drawpal(weapon_xoffset + 152 - (p->getlookang() >> 1),
				looking_arc + 249 - gun_pos,
				HANDHOLDINGLASER + (p->kickback_pic >> 2), gs, o | 4, pal);
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayrpg = [&]()
		{
			pin = (isWorldTour() || (duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;
			auto rpgpic = isWorldTour() ? RPGGUNWIDE : RPGGUN;

			if (sprite[p->i].pal == 1)
				pal = 1;
			else pal = sector[p->cursectnum].floorpal;

			weapon_xoffset -= sintable[(768 + (p->kickback_pic << 7)) & 2047] >> 11;
			gun_pos += sintable[(768 + (p->kickback_pic << 7) & 2047)] >> 11;

			if (*kb > 0)
			{
				if (*kb < (isWW2GI() ? aplWeaponTotalTime[RPG_WEAPON][snum] : 8))
				{
					hud_drawpal(weapon_xoffset + 164, (looking_arc << 1) + 176 - gun_pos,
						rpgpic + (p->kickback_pic >> 1), gs, o | pin, pal);
				}
				else if (isWW2GI())
				{
					// else we are in 'reload time'
					if (p->kickback_pic <
						(
							(aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum]) / 2
							+ aplWeaponTotalTime[p->curr_weapon][snum]
							)
						)
					{
						// down 
						gun_pos -= 10 * (p->kickback_pic - aplWeaponTotalTime[p->curr_weapon][snum]); //D
					}
					else
					{
						// move back down

						// up and left
						gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - p->kickback_pic); //U
					}
				}
			}

			hud_drawpal(weapon_xoffset + 164, (looking_arc << 1) + 176 - gun_pos, rpgpic, gs, o | pin, pal);
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
				gun_pos -= sintable[p->kickback_pic << 7] >> 12;
			}

			if (*kb > 0 && sprite[p->i].pal != 1)
			{
				weapon_xoffset += 1 - (rand() & 3);
			}

			weapon_xoffset -= 8;

			if (*kb == 0)
			{
				hud_drawpal(weapon_xoffset + 146 - (p->getlookang() >> 1), looking_arc + 202 - gun_pos,
					SHOTGUN, gs, o, pal);
			}
			else if (*kb <= aplWeaponTotalTime[SHOTGUN_WEAPON][snum])
			{
				hud_drawpal(weapon_xoffset + 146 - (p->getlookang() >> 1), looking_arc + 202 - gun_pos,
					SHOTGUN + 1, gs, o, pal);
			}
			// else we are in 'reload time'
			else if (p->kickback_pic <
				(
					(aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum]) / 2
					+ aplWeaponTotalTime[p->curr_weapon][snum]
					)
				)
			{
				// down 
				gun_pos -= 10 * (p->kickback_pic - aplWeaponTotalTime[p->curr_weapon][snum]); //D
//					weapon_xoffset+=80*(*kb-aplWeaponTotalTime[cw][snum]);
				hud_drawpal(weapon_xoffset + 146 - (p->getlookang() >> 1), looking_arc + 202 - gun_pos,
					SHOTGUN, gs, o, pal);
			}
			else
			{
				// move back down

				// up and left
				gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - p->kickback_pic); //U
//					weapon_xoffset+=80*(*kb-aplWeaponTotalTime[cw][snum]);
				hud_drawpal(weapon_xoffset + 146 - (p->getlookang() >> 1), looking_arc + 202 - gun_pos,
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

			switch(p->kickback_pic)
			{
				case 1:
				case 2:
					hud_drawpal(weapon_xoffset + 168 - (p->getlookang() >> 1),looking_arc + 201 - gun_pos,
					   SHOTGUN + 2,-128,o,pal);
				case 0:
				case 6:
				case 7:
				case 8:
					hud_drawpal(weapon_xoffset + 146 - (p->getlookang() >> 1),looking_arc + 202 - gun_pos,
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

						hud_drawpal(weapon_xoffset + 178 - (p->getlookang() >> 1),looking_arc + 194 - gun_pos,
							SHOTGUN + 1 + ((*(kb)-1) >> 1),-128,o,pal);
					}

					hud_drawpal(weapon_xoffset + 158 - (p->getlookang() >> 1),looking_arc + 220 - gun_pos,
						SHOTGUN + 3,gs,o,pal);

					break;
				case 13:
				case 14:
				case 15:
					hud_drawpal(32 + weapon_xoffset + 166 - (p->getlookang() >> 1),looking_arc + 210 - gun_pos,
						SHOTGUN + 4,gs,o,pal);
					break;
				case 16:
				case 17:
				case 18:
				case 19:
					hud_drawpal(64 + weapon_xoffset + 170 - (p->getlookang() >> 1),looking_arc + 196 - gun_pos,
						SHOTGUN + 5,gs,o,pal);
					break;
				case 20:
				case 21:
				case 22:
				case 23:
					hud_drawpal(64 + weapon_xoffset + 176 - (p->getlookang() >> 1),looking_arc + 196 - gun_pos,
						SHOTGUN + 6,gs,o,pal);
					break;
				case 24:
				case 25:
				case 26:
				case 27:
					hud_drawpal(64 + weapon_xoffset + 170 - (p->getlookang() >> 1),looking_arc + 196 - gun_pos,
						SHOTGUN + 5,gs,o,pal);
					break;
				case 28:
				case 29:
				case 30:
					hud_drawpal(32 + weapon_xoffset + 156 - (p->getlookang() >> 1),looking_arc + 206 - gun_pos,
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
				gun_pos -= sintable[p->kickback_pic << 7] >> 12;

			if (*kb > 0 && sprite[p->i].pal != 1) weapon_xoffset += 1 - (rand() & 3);

			if (*kb == 0)
			{
				//				hud_drawpal(weapon_xoffset+168-(p->getlookang()>>1),looking_arc+260-gun_pos,
				//						CHAINGUN,gs,o,pal);
				hud_drawpal(weapon_xoffset + 178 - (p->getlookang() >> 1), looking_arc + 233 - gun_pos,
					CHAINGUN + 1, gs, o, pal);
			}
			else if (*kb <= aplWeaponTotalTime[CHAINGUN_WEAPON][snum])
			{
				hud_drawpal(weapon_xoffset + 188 - (p->getlookang() >> 1), looking_arc + 243 - gun_pos,
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
				if (p->kickback_pic <
					(iFifths
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// first segment
					// 
					gun_pos += 80 - (10 * (aplWeaponTotalTime[p->curr_weapon][snum]
						+ iFifths - p->kickback_pic));
					weapon_xoffset += 80 - (10 * (aplWeaponTotalTime[p->curr_weapon][snum]
						+ iFifths - p->kickback_pic));
					hud_drawpal(weapon_xoffset + 168 - (p->getlookang() >> 1), looking_arc + 260 - gun_pos,
						2519, gs, o, pal);
				}
				else if (p->kickback_pic <
					(iFifths * 2
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// second segment
					// down 
					gun_pos += 80; //5*(iFifthsp->kickback_pic-aplWeaponTotalTime[p->curr_weapon][snum]); //D
					weapon_xoffset += 80; //80*(*kb-aplWeaponTotalTime[p->curr_weapon][snum]);
					hud_drawpal(weapon_xoffset + 168 - (p->getlookang() >> 1), looking_arc + 260 - gun_pos,
						2518, gs, o, pal);
				}
				else if (p->kickback_pic <
					(iFifths * 3
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// third segment
					// up 
					gun_pos += 80;//5*(iFifths*2);
					weapon_xoffset += 80; //80*(*kb-aplWeaponTotalTime[p->curr_weapon][snum]);
					hud_drawpal(weapon_xoffset + 168 - (p->getlookang() >> 1), looking_arc + 260 - gun_pos,
						2517, gs, o, pal);
				}
				else if (p->kickback_pic <
					(iFifths * 4
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// fourth segment
					// down 
					gun_pos += 80; //5*(aplWeaponTotalTime[p->curr_weapon][snum]- p->kickback_pic); //D
					weapon_xoffset += 80; //80*(*kb-aplWeaponTotalTime[p->curr_weapon][snum]);
					hud_drawpal(weapon_xoffset + 168 - (p->getlookang() >> 1), looking_arc + 260 - gun_pos, 2518, gs, o, pal);
				}
				else
				{
					// move back down

					// up and left
					gun_pos += 10 * (aplWeaponReload[p->curr_weapon][snum] - p->kickback_pic);
					//5*(aplWeaponReload[p->curr_weapon][snum]- p->kickback_pic); //U
					weapon_xoffset += 10 * (aplWeaponReload[p->curr_weapon][snum] - p->kickback_pic);
					//80*(*kb-aplWeaponTotalTime[cw][snum]);
					hud_drawpal(weapon_xoffset + 168 - (p->getlookang() >> 1), looking_arc + 260 - gun_pos, 2519, gs, o, pal);
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
				gun_pos -= sintable[p->kickback_pic << 7] >> 12;

			if (*kb > 0 && sprite[p->i].pal != 1) weapon_xoffset += 1 - (rand() & 3);

			hud_drawpal(weapon_xoffset + 168 - (p->getlookang() >> 1), looking_arc + 260 - gun_pos, CHAINGUN, gs, o, pal);
			switch(p->kickback_pic)
			{
				case 0:
					hud_drawpal(weapon_xoffset + 178 - (p->getlookang() >> 1),looking_arc + 233 - gun_pos, CHAINGUN + 1,gs,o,pal);
					break;
				default:
					if (*kb > 4 && *kb < 12)
					{
						i = 0;
						if (sprite[p->i].pal != 1) i = rand() & 7;
						hud_drawpal(i + weapon_xoffset - 4 + 140 - (p->getlookang() >> 1),i + looking_arc - (p->kickback_pic >> 1) + 208 - gun_pos, CHAINGUN + 5 + ((*kb - 4) / 5),gs,o,pal);
						if (sprite[p->i].pal != 1) i = rand() & 7;
						hud_drawpal(i + weapon_xoffset - 4 + 184 - (p->getlookang() >> 1),i + looking_arc - (p->kickback_pic >> 1) + 208 - gun_pos, CHAINGUN + 5 + ((*kb - 4) / 5),gs,o,pal);
					}
					if (*kb < 8)
					{
						i = rand() & 7;
						hud_drawpal(i + weapon_xoffset - 4 + 162 - (p->getlookang() >> 1),i + looking_arc - (p->kickback_pic >> 1) + 208 - gun_pos, CHAINGUN + 5 + ((*kb - 2) / 5),gs,o,pal);
						hud_drawpal(weapon_xoffset + 178 - (p->getlookang() >> 1),looking_arc + 233 - gun_pos, CHAINGUN + 1 + (p->kickback_pic >> 1),gs,o,pal);
					}
					else hud_drawpal(weapon_xoffset + 178 - (p->getlookang() >> 1),looking_arc + 233 - gun_pos, CHAINGUN + 1,gs,o,pal);
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

			if (p->kickback_pic < 5)
			{
				short kb_frames[] = { 0,1,2,0,0 }, l;

				l = 195 - 12 + weapon_xoffset;

				if (p->kickback_pic == 2)
					l -= 3;
				{
					int x, y;
					short tilenum;
					signed char shade;
					char orientation;
					x = (l - (p->getlookang() >> 1));
					y = (looking_arc + 244 - gun_pos);
					tilenum = FIRSTGUN + kb_frames[*kb];
					shade = gs;
					orientation = 2;


					hud_drawpal(
						x,
						y,
						tilenum,
						gs, 2, pal);
				}
			}
			else
			{
				pin = (isWW2GI() || isWorldTour() || (duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;
				auto pic = isWorldTour() ? FIRSTGUNRELOAD - 4 : FIRSTGUN; // I do not want to edit all code below

				const int WEAPON2_RELOAD_TIME = 50;
				auto reload_time = isWW2GI() ? aplWeaponReload[PISTOL_WEAPON][snum] : WEAPON2_RELOAD_TIME;
				if (p->kickback_pic < 10)
					hud_drawpal(194 - (p->getlookang() >> 1), looking_arc + 230 - gun_pos, pic + 4, gs, o|pin, pal);
				else if (p->kickback_pic < 15)
				{
					hud_drawpal(244 - (p->kickback_pic << 3) - (p->getlookang() >> 1), looking_arc + 130 - gun_pos + (p->kickback_pic << 4), pic + 6, gs, o | pin, pal);
					hud_drawpal(224 - (p->getlookang() >> 1), looking_arc + 220 - gun_pos, pic + 5, gs, o | pin, pal);
				}
				else if (p->kickback_pic < 20)
				{
					hud_drawpal(124 + (p->kickback_pic << 1) - (p->getlookang() >> 1), looking_arc + 430 - gun_pos - (p->kickback_pic << 3), pic + 6, gs, o | pin, pal);
					hud_drawpal(224 - (p->getlookang() >> 1), looking_arc + 220 - gun_pos, pic + 5, gs, o | pin, pal);
				}
				else if (p->kickback_pic < (reload_time - 12))
				{
					hud_drawpal(184 - (p->getlookang() >> 1), looking_arc + 235 - gun_pos, pic + 8, gs, o | pin, pal);
					hud_drawpal(224 - (p->getlookang() >> 1), looking_arc + 210 - gun_pos, pic + 5, gs, o | pin, pal);
				}
				else if (p->kickback_pic < (reload_time - 6))
				{
					hud_drawpal(164 - (p->getlookang() >> 1), looking_arc + 245 - gun_pos, pic + 8, gs, o | pin, pal);
					hud_drawpal(224 - (p->getlookang() >> 1), looking_arc + 220 - gun_pos, pic + 5, gs, o | pin, pal);
				}
				else if (p->kickback_pic < (reload_time))
					hud_drawpal(194 - (p->getlookang() >> 1), looking_arc + 235 - gun_pos, pic + 5, gs, o, pal);
				else if (p->kickback_pic < 23)
				{
					hud_drawpal(184 - (p->getlookang() >> 1), looking_arc + 235 - gun_pos, pic + 8, gs, o | pin, pal);
					hud_drawpal(224 - (p->getlookang() >> 1), looking_arc + 210 - gun_pos, pic + 5, gs, o | pin, pal);
				}
				else if (p->kickback_pic < 25)
				{
					hud_drawpal(164 - (p->getlookang() >> 1), looking_arc + 245 - gun_pos, pic + 8, gs, o | pin, pal);
					hud_drawpal(224 - (p->getlookang() >> 1), looking_arc + 220 - gun_pos, pic + 5, gs, o | pin, pal);
				}
				else if (p->kickback_pic < 27)
					hud_drawpal(194 - (p->getlookang() >> 1), looking_arc + 235 - gun_pos, pic + 5, gs, o | pin, pal);
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

			if (p->kickback_pic)
			{
				static const uint8_t throw_frames[]
					= { 0,0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2 };

				if (isWW2GI())
				{
					if (p->kickback_pic <= aplWeaponFireDelay[HANDBOMB_WEAPON][snum])
					{
						// it holds here
						gun_pos -= 5 * p->kickback_pic;        //D
					}
					else if (p->kickback_pic <
						(
							(aplWeaponTotalTime[HANDBOMB_WEAPON][snum] - aplWeaponFireDelay[HANDBOMB_WEAPON][snum]) / 2
							+ aplWeaponFireDelay[HANDBOMB_WEAPON][snum]
							)
						)
					{
						// up and left
						gun_pos += 10 * (p->kickback_pic - aplWeaponFireDelay[HANDBOMB_WEAPON][snum]); //U
						weapon_xoffset += 80 * (*kb - aplWeaponFireDelay[HANDBOMB_WEAPON][snum]);
					}
					else if (p->kickback_pic < aplWeaponTotalTime[HANDBOMB_WEAPON][snum])
					{
						gun_pos += 240;	// start high
						gun_pos -= 12 * (p->kickback_pic - aplWeaponFireDelay[HANDBOMB_WEAPON][snum]);  //D
						// move left
						weapon_xoffset += 90 - (5 * (aplWeaponTotalTime[HANDBOMB_WEAPON][snum] - p->kickback_pic));
					}
				}
				else
				{
					if (p->kickback_pic < 7)
						gun_pos -= 10 * p->kickback_pic;        //D
					else if (p->kickback_pic < 12)
						gun_pos += 20 * (p->kickback_pic - 10); //U
					else if (p->kickback_pic < 20)
						gun_pos -= 9 * (p->kickback_pic - 14);  //D
				}
				hud_drawpal(weapon_xoffset + 190 - (p->getlookang() >> 1), looking_arc + 250 - gun_pos, HANDTHROW + throw_frames[p->kickback_pic], gs, o, pal);
			}
			else
				hud_drawpal(weapon_xoffset + 190 - (p->getlookang() >> 1), looking_arc + 260 - gun_pos, HANDTHROW, gs, o, pal);
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

			if (p->kickback_pic)
				hud_drawpal(weapon_xoffset + 150 - (p->getlookang() >> 1), looking_arc + 258 - gun_pos, HANDREMOTE + remote_frames[p->kickback_pic], gs, o, pal);
			else
				hud_drawpal(weapon_xoffset + 150 - (p->getlookang() >> 1), looking_arc + 258 - gun_pos, HANDREMOTE, gs, o, pal);
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

			if (p->kickback_pic)
			{
				if (p->kickback_pic < aplWeaponTotalTime[p->curr_weapon][snum])
				{
					i = sgn(p->kickback_pic >> 2);
					if (p->ammo_amount[p->curr_weapon] & 1)
					{
						hud_drawpal(weapon_xoffset + 30 - (p->getlookang() >> 1), looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
						hud_drawpal(weapon_xoffset + 268 - (p->getlookang() >> 1), looking_arc + 238 - gun_pos, DEVISTATOR + i, -32, o, pal);
					}
					else
					{
						hud_drawpal(weapon_xoffset + 30 - (p->getlookang() >> 1), looking_arc + 240 - gun_pos, DEVISTATOR + i, -32, o | 4, pal);
						hud_drawpal(weapon_xoffset + 268 - (p->getlookang() >> 1), looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
					}
				}
				// else we are in 'reload time'
				else if (p->kickback_pic <
					(
						(aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum]) / 2
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// down 
					gun_pos -= 10 * (p->kickback_pic - aplWeaponTotalTime[p->curr_weapon][snum]); //D
//					weapon_xoffset+=80*(*kb-aplWeaponTotalTime[cw][snum]);
					hud_drawpal(weapon_xoffset + 268 - (p->getlookang() >> 1), looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
					hud_drawpal(weapon_xoffset + 30 - (p->getlookang() >> 1), looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
				}
				else
				{
					// move back down

					// up and left
					gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - p->kickback_pic); //U
//					weapon_xoffset+=80*(*kb-aplWeaponTotalTime[cw][snum]);
					hud_drawpal(weapon_xoffset + 268 - (p->getlookang() >> 1), looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
					hud_drawpal(weapon_xoffset + 30 - (p->getlookang() >> 1), looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
				}
			}
			else
			{
				hud_drawpal(weapon_xoffset + 268 - (p->getlookang() >> 1), looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
				hud_drawpal(weapon_xoffset + 30 - (p->getlookang() >> 1), looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
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

			if (p->kickback_pic)
			{
				char cycloidy[] = { 0,4,12,24,12,4,0 };

				i = sgn(p->kickback_pic >> 2);

				if (p->hbomb_hold_delay)
				{
					hud_drawpal((cycloidy[*kb] >> 1) + weapon_xoffset + 268 - (p->getlookang() >> 1), cycloidy[*kb] + looking_arc + 238 - gun_pos, DEVISTATOR + i, -32, o, pal);
					hud_drawpal(weapon_xoffset + 30 - (p->getlookang() >> 1), looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
				}
				else
				{
					hud_drawpal(-(cycloidy[*kb] >> 1) + weapon_xoffset + 30 - (p->getlookang() >> 1), cycloidy[*kb] + looking_arc + 240 - gun_pos, DEVISTATOR + i, -32, o | 4, pal);
					hud_drawpal(weapon_xoffset + 268 - (p->getlookang() >> 1), looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
				}
			}
			else
			{
				hud_drawpal(weapon_xoffset + 268 - (p->getlookang() >> 1), looking_arc + 238 - gun_pos, DEVISTATOR, gs, o, pal);
				hud_drawpal(weapon_xoffset + 30 - (p->getlookang() >> 1), looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal);
			}
		};

		//---------------------------------------------------------------------------
		//
		//
		//
		//---------------------------------------------------------------------------

		auto displayfreezer = [&]
		{
			pin = (isWW2GI() || isWorldTour() || (duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING)) ? 0 : RS_ALIGN_R;
			auto pic = isWorldTour() ? FREEZEWIDE : FREEZE;

			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;

			if (p->kickback_pic)
			{
				char cat_frames[] = { 0,0,1,1,2,2 };

				if (sprite[p->i].pal != 1)
				{
					weapon_xoffset += rand() & 3;
					looking_arc += rand() & 3;
				}
				gun_pos -= 16;
				hud_drawpal(weapon_xoffset + 210 - (p->getlookang() >> 1), looking_arc + 261 - gun_pos, pic + 2, -32, o|pin, pal);
				hud_drawpal(weapon_xoffset + 210 - (p->getlookang() >> 1), looking_arc + 235 - gun_pos, pic + 3 + cat_frames[*kb % 6], -32, o | pin, pal);
			}
			else hud_drawpal(weapon_xoffset + 210 - (p->getlookang() >> 1), looking_arc + 261 - gun_pos, pic, gs, o | pin, pal);
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
			if (p->kickback_pic == 0)
			{
				// the 'at rest' display
				if (ps[snum].ammo_amount[cw] <= 0) //p->last_weapon >= 0)
				{
					hud_drawpal(weapon_xoffset + 184 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (p->kickback_pic & 3), -32,
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER + 1, gs, o, pal);
				}
				else
				{

					hud_drawpal(weapon_xoffset + 184 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER + 2,
						16 - (sintable[p->random_club_frame & 2047] >> 10),
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - (p->getlookang() >> 1),
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
						gun_pos -= 10 * (aplWeaponTotalTime[p->curr_weapon][snum] - p->kickback_pic);
					}
				}
				// else we are in 'reload time'
				else if (p->kickback_pic <
					(
						(aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum]) / 2
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// down 
					gun_pos -= 10 * (p->kickback_pic - aplWeaponTotalTime[p->curr_weapon][snum]); //D
				}
				else
				{
					// up
					gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - p->kickback_pic); //U
				}

				// draw weapon
				{
					hud_drawpal(weapon_xoffset + 184 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (p->kickback_pic & 3), -32,
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - (p->getlookang() >> 1),
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
			if (p->kickback_pic == 0)
			{
				{
					hud_drawpal(weapon_xoffset + 188 - (p->getlookang() >> 1),
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
						gun_pos -= 15 * (aplWeaponTotalTime[p->curr_weapon][snum] - p->kickback_pic);
					}
				}
				// else we are in 'reload time'
				else if (p->kickback_pic <
					(
						(aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum]) / 2
						+ aplWeaponTotalTime[p->curr_weapon][snum]
						)
					)
				{
					// down 
					gun_pos -= 5 * (p->kickback_pic - aplWeaponTotalTime[p->curr_weapon][snum]); //D
				}
				else
				{
					// up
					gun_pos -= 10 * (aplWeaponReload[p->curr_weapon][snum] - p->kickback_pic); //U
				}

				// display weapon
				{
					hud_drawpal(weapon_xoffset + 184 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (p->kickback_pic & 3), -32,
						o, 2);

					hud_drawpal(weapon_xoffset + 188 - (p->getlookang() >> 1),
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
			weapon_xoffset += 28;
			looking_arc += 18;
			if (sprite[p->i].pal == 1)
				pal = 1;
			else
				pal = sector[p->cursectnum].floorpal;
			if (p->kickback_pic == 0)
			{
				if (cw == GROW_WEAPON)
				{
					hud_drawpal(weapon_xoffset + 184 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER + 2,
						16 - (sintable[p->random_club_frame & 2047] >> 10),
						o, 2);

					hud_drawpal(weapon_xoffset + 188 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER - 2, gs, o, pal);
				}
				else
				{
					hud_drawpal(weapon_xoffset + 184 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER + 2,
						16 - (sintable[p->random_club_frame & 2047] >> 10),
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER, gs, o, pal);
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
					hud_drawpal(weapon_xoffset + 184 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (p->kickback_pic & 3), -32,
						o, 2);

					hud_drawpal(weapon_xoffset + 188 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER - 1, gs, o, pal);

				}
				else
				{
					hud_drawpal(weapon_xoffset + 184 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER + 3 + (p->kickback_pic & 3), -32,
						o, 0);

					hud_drawpal(weapon_xoffset + 188 - (p->getlookang() >> 1),
						looking_arc + 240 - gun_pos, SHRINKER + 1, gs, o, pal);
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
				hud_drawpal(weapon_xoffset + 210 - (p->getlookang() >> 1), looking_arc + 261 - gun_pos, FLAMETHROWER, gs, o, pal);
				hud_drawpal(weapon_xoffset + 210 - (p->getlookang() >> 1), looking_arc + 261 - gun_pos, FLAMETHROWERPILOT, gs, o, pal);
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
				hud_drawpal(weapon_xoffset + 210 - (p->getlookang() >> 1), looking_arc + 261 - gun_pos, FLAMETHROWER + 1, 32, o, pal);
				hud_drawpal(weapon_xoffset + 210 - (p->getlookang() >> 1), looking_arc + 235 - gun_pos, FLAMETHROWER + 2 + cat_frames[*kb % 6], -32, o, pal);
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
