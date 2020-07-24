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
#include "sounds.h"
#include "names_d.h"
#include "mapinfo.h"

// PRIMITIVE
BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool isadoorwall_d(int dapic)
{
	switch(dapic)
	{
		case DOORTILE1:
		case DOORTILE2:
		case DOORTILE3:
		case DOORTILE4:
		case DOORTILE5:
		case DOORTILE6:
		case DOORTILE7:
		case DOORTILE8:
		case DOORTILE9:
		case DOORTILE10:
		case DOORTILE11:
		case DOORTILE12:
		case DOORTILE14:
		case DOORTILE15:
		case DOORTILE16:
		case DOORTILE17:
		case DOORTILE18:
		case DOORTILE19:
		case DOORTILE20:
		case DOORTILE21:
		case DOORTILE22:
		case DOORTILE23:
			return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void animatewalls_d(void)
{
	int i, j, p, t;

	for (p = 0; p < numanimwalls; p++)
	{
		i = animwall[p].wallnum;
		j = wall[i].picnum;

		switch (j)
		{
		case SCREENBREAK1:
		case SCREENBREAK2:
		case SCREENBREAK3:
		case SCREENBREAK4:
		case SCREENBREAK5:

		case SCREENBREAK9:
		case SCREENBREAK10:
		case SCREENBREAK11:
		case SCREENBREAK12:
		case SCREENBREAK13:
		case SCREENBREAK14:
		case SCREENBREAK15:
		case SCREENBREAK16:
		case SCREENBREAK17:
		case SCREENBREAK18:
		case SCREENBREAK19:

			if ((krand() & 255) < 16)
			{
				animwall[p].tag = wall[i].picnum;
				wall[i].picnum = SCREENBREAK6;
			}

			continue;

		case SCREENBREAK6:
		case SCREENBREAK7:
		case SCREENBREAK8:

			if (animwall[p].tag >= 0 && wall[i].extra != FEMPIC2 && wall[i].extra != FEMPIC3)
				wall[i].picnum = animwall[p].tag;
			else
			{
				wall[i].picnum++;
				if (wall[i].picnum == (SCREENBREAK6 + 3))
					wall[i].picnum = SCREENBREAK6;
			}
			continue;

		}

		if (wall[i].cstat & 16)
			switch (wall[i].overpicnum)
			{
			case W_FORCEFIELD:
			case W_FORCEFIELD + 1:
			case W_FORCEFIELD + 2:

				t = animwall[p].tag;

				if (wall[i].cstat & 254)
				{
					wall[i].xpanning -= t >> 10; // sintable[(t+512)&2047]>>12;
					wall[i].ypanning -= t >> 10; // sintable[t&2047]>>12;

					if (wall[i].extra == 1)
					{
						wall[i].extra = 0;
						animwall[p].tag = 0;
					}
					else
						animwall[p].tag += 128;

					if (animwall[p].tag < (128 << 4))
					{
						if (animwall[p].tag & 128)
							wall[i].overpicnum = W_FORCEFIELD;
						else wall[i].overpicnum = W_FORCEFIELD + 1;
					}
					else
					{
						if ((krand() & 255) < 32)
							animwall[p].tag = 128 << (krand() & 3);
						else wall[i].overpicnum = W_FORCEFIELD + 1;
					}
				}

				break;
			}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operaterespawns_d(int low)
{
	short i, j, nexti;

	i = headspritestat[11];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		if (sprite[i].lotag == low) switch (sprite[i].picnum)
		{
		case RESPAWN:
			if (badguypic(sprite[i].hitag) && ud.monsters_off) break;

			j = fi.spawn(i, TRANSPORTERSTAR);
			sprite[j].z -= (32 << 8);

			sprite[i].extra = 66 - 12;   // Just a way to killit
			break;
		}
		i = nexti;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operateforcefields_d(int s, int low)
{
	operateforcefields_common(s, low, { W_FORCEFIELD, W_FORCEFIELD + 1, W_FORCEFIELD + 2, BIGFORCE });
}

//---------------------------------------------------------------------------
//
// how NOT to implement switch animations...
//
//---------------------------------------------------------------------------

bool checkhitswitch_d(int snum, int w, int switchtype)
{
	uint8_t switchpal;
	int i, x, lotag, hitag, picnum, correctdips, numdips;
	int sx, sy;

	if (w < 0) return 0;
	correctdips = 1;
	numdips = 0;

	if (switchtype == SWITCH_SPRITE) // A wall sprite
	{
		lotag = sprite[w].lotag;
		if (lotag == 0) return 0;
		hitag = sprite[w].hitag;
		sx = sprite[w].x;
		sy = sprite[w].y;
		picnum = sprite[w].picnum;
		switchpal = sprite[w].pal;
	}
	else
	{
		lotag = wall[w].lotag;
		if (lotag == 0) return 0;
		hitag = wall[w].hitag;
		sx = wall[w].x;
		sy = wall[w].y;
		picnum = wall[w].picnum;
		switchpal = wall[w].pal;
	}

	switch (picnum)
	{
	case DIPSWITCH:
	case DIPSWITCH + 1:
	case TECHSWITCH:
	case TECHSWITCH + 1:
	case ALIENSWITCH:
	case ALIENSWITCH + 1:
		break;
	case DEVELOPERCOMMENTARY + 1: //Twentieth Anniversary World Tour
		if (switchtype == 1)
		{
			sprite[w].picnum--;
			StopCommentary();
			return true;
		}
		return false;
	case DEVELOPERCOMMENTARY: //Twentieth Anniversary World Tour
		if (switchtype == 1)
		{
			if (StartCommentary(lotag, w))
				sprite[w].picnum++;
			return true;
		}
		return false;
	case ACCESSSWITCH:
	case ACCESSSWITCH2:
		if (ps[snum].access_incs == 0)
		{
			if (switchpal == 0)
			{
				if ((ps[snum].got_access & 1))
					ps[snum].access_incs = 1;
				else FTA(70, &ps[snum]);
			}

			else if (switchpal == 21)
			{
				if (ps[snum].got_access & 2)
					ps[snum].access_incs = 1;
				else FTA(71, &ps[snum]);
			}

			else if (switchpal == 23)
			{
				if (ps[snum].got_access & 4)
					ps[snum].access_incs = 1;
				else FTA(72, &ps[snum]);
			}

			if (ps[snum].access_incs == 1)
			{
				if (switchtype == SWITCH_WALL)
					ps[snum].access_wallnum = w;
				else
					ps[snum].access_spritenum = w;
			}

			return 0;
		}
	case DIPSWITCH2:
	case DIPSWITCH2 + 1:
	case DIPSWITCH3:
	case DIPSWITCH3 + 1:
	case MULTISWITCH:
	case MULTISWITCH + 1:
	case MULTISWITCH + 2:
	case MULTISWITCH + 3:
	case PULLSWITCH:
	case PULLSWITCH + 1:
	case HANDSWITCH:
	case HANDSWITCH + 1:
	case SLOTDOOR:
	case SLOTDOOR + 1:
	case LIGHTSWITCH:
	case LIGHTSWITCH + 1:
	case SPACELIGHTSWITCH:
	case SPACELIGHTSWITCH + 1:
	case SPACEDOORSWITCH:
	case SPACEDOORSWITCH + 1:
	case FRANKENSTINESWITCH:
	case FRANKENSTINESWITCH + 1:
	case LIGHTSWITCH2:
	case LIGHTSWITCH2 + 1:
	case POWERSWITCH1:
	case POWERSWITCH1 + 1:
	case LOCKSWITCH1:
	case LOCKSWITCH1 + 1:
	case POWERSWITCH2:
	case POWERSWITCH2 + 1:
		if (check_activator_motion(lotag)) return 0;
		break;
	default:
		if (fi.isadoorwall(picnum) == 0) return 0;
		break;
	}

	i = headspritestat[0];
	while (i >= 0)
	{
		if (lotag == sprite[i].lotag) switch (sprite[i].picnum)
		{
		case DIPSWITCH:
		case TECHSWITCH:
		case ALIENSWITCH:
			if (switchtype == SWITCH_SPRITE && w == i) sprite[i].picnum++;
			else if (sprite[i].hitag == 0) correctdips++;
			numdips++;
			break;
		case TECHSWITCH + 1:
		case DIPSWITCH + 1:
		case ALIENSWITCH + 1:
			if (switchtype == SWITCH_SPRITE && w == i) sprite[i].picnum--;
			else if (sprite[i].hitag == 1) correctdips++;
			numdips++;
			break;
		case MULTISWITCH:
		case MULTISWITCH + 1:
		case MULTISWITCH + 2:
		case MULTISWITCH + 3:
			sprite[i].picnum++;
			if (sprite[i].picnum > (MULTISWITCH + 3))
				sprite[i].picnum = MULTISWITCH;
			break;
		case ACCESSSWITCH:
		case ACCESSSWITCH2:
		case SLOTDOOR:
		case LIGHTSWITCH:
		case SPACELIGHTSWITCH:
		case SPACEDOORSWITCH:
		case FRANKENSTINESWITCH:
		case LIGHTSWITCH2:
		case POWERSWITCH1:
		case LOCKSWITCH1:
		case POWERSWITCH2:
		case HANDSWITCH:
		case PULLSWITCH:
		case DIPSWITCH2:
		case DIPSWITCH3:
			sprite[i].picnum++;
			break;
		case PULLSWITCH + 1:
		case HANDSWITCH + 1:
		case LIGHTSWITCH2 + 1:
		case POWERSWITCH1 + 1:
		case LOCKSWITCH1 + 1:
		case POWERSWITCH2 + 1:
		case SLOTDOOR + 1:
		case LIGHTSWITCH + 1:
		case SPACELIGHTSWITCH + 1:
		case SPACEDOORSWITCH + 1:
		case FRANKENSTINESWITCH + 1:
		case DIPSWITCH2 + 1:
		case DIPSWITCH3 + 1:
			sprite[i].picnum--;
			break;
		}
		i = nextspritestat[i];
	}

	for (i = 0; i < numwalls; i++)
	{
		x = i;
		if (lotag == wall[x].lotag)
			switch (wall[x].picnum)
			{
			case DIPSWITCH:
			case TECHSWITCH:
			case ALIENSWITCH:
				if (switchtype == SWITCH_WALL && i == w) wall[x].picnum++;
				else if (wall[x].hitag == 0) correctdips++;
				numdips++;
				break;
			case DIPSWITCH + 1:
			case TECHSWITCH + 1:
			case ALIENSWITCH + 1:
				if (switchtype == SWITCH_WALL && i == w) wall[x].picnum--;
				else if (wall[x].hitag == 1) correctdips++;
				numdips++;
				break;
			case MULTISWITCH:
			case MULTISWITCH + 1:
			case MULTISWITCH + 2:
			case MULTISWITCH + 3:
				wall[x].picnum++;
				if (wall[x].picnum > (MULTISWITCH + 3))
					wall[x].picnum = MULTISWITCH;
				break;
			case ACCESSSWITCH:
			case ACCESSSWITCH2:
			case SLOTDOOR:
			case LIGHTSWITCH:
			case SPACELIGHTSWITCH:
			case SPACEDOORSWITCH:
			case LIGHTSWITCH2:
			case POWERSWITCH1:
			case LOCKSWITCH1:
			case POWERSWITCH2:
			case PULLSWITCH:
			case HANDSWITCH:
			case DIPSWITCH2:
			case DIPSWITCH3:
				wall[x].picnum++;
				break;
			case HANDSWITCH + 1:
			case PULLSWITCH + 1:
			case LIGHTSWITCH2 + 1:
			case POWERSWITCH1 + 1:
			case LOCKSWITCH1 + 1:
			case POWERSWITCH2 + 1:
			case SLOTDOOR + 1:
			case LIGHTSWITCH + 1:
			case SPACELIGHTSWITCH + 1:
			case SPACEDOORSWITCH + 1:
			case DIPSWITCH2 + 1:
			case DIPSWITCH3 + 1:
				wall[x].picnum--;
				break;
			}
	}

	if (lotag == (short)65535)
	{
		setnextmap(false);
		return 1;
	}

	vec3_t v = { sx, sy, ps[snum].posz };
	switch (picnum)
	{
	default:
		if (fi.isadoorwall(picnum) == 0) break;
	case DIPSWITCH:
	case DIPSWITCH + 1:
	case TECHSWITCH:
	case TECHSWITCH + 1:
	case ALIENSWITCH:
	case ALIENSWITCH + 1:
		if (picnum == DIPSWITCH || picnum == DIPSWITCH + 1 ||
			picnum == ALIENSWITCH || picnum == ALIENSWITCH + 1 ||
			picnum == TECHSWITCH || picnum == TECHSWITCH + 1)
		{
			if (picnum == ALIENSWITCH || picnum == ALIENSWITCH + 1)
			{
				if (switchtype == SWITCH_SPRITE)
					S_PlaySound3D(ALIEN_SWITCH1, w, &v);
				else S_PlaySound3D(ALIEN_SWITCH1, ps[snum].i, &v);
			}
			else
			{
				if (switchtype == SWITCH_SPRITE)
					S_PlaySound3D(SWITCH_ON, w, &v);
				else S_PlaySound3D(SWITCH_ON, ps[snum].i, &v);
			}
			if (numdips != correctdips) break;
			S_PlaySound3D(END_OF_LEVEL_WARN, ps[snum].i, &v);
		}
	case DIPSWITCH2:
	case DIPSWITCH2 + 1:
	case DIPSWITCH3:
	case DIPSWITCH3 + 1:
	case MULTISWITCH:
	case MULTISWITCH + 1:
	case MULTISWITCH + 2:
	case MULTISWITCH + 3:
	case ACCESSSWITCH:
	case ACCESSSWITCH2:
	case SLOTDOOR:
	case SLOTDOOR + 1:
	case LIGHTSWITCH:
	case LIGHTSWITCH + 1:
	case SPACELIGHTSWITCH:
	case SPACELIGHTSWITCH + 1:
	case SPACEDOORSWITCH:
	case SPACEDOORSWITCH + 1:
	case FRANKENSTINESWITCH:
	case FRANKENSTINESWITCH + 1:
	case LIGHTSWITCH2:
	case LIGHTSWITCH2 + 1:
	case POWERSWITCH1:
	case POWERSWITCH1 + 1:
	case LOCKSWITCH1:
	case LOCKSWITCH1 + 1:
	case POWERSWITCH2:
	case POWERSWITCH2 + 1:
	case HANDSWITCH:
	case HANDSWITCH + 1:
	case PULLSWITCH:
	case PULLSWITCH + 1:

		if (picnum == MULTISWITCH || picnum == (MULTISWITCH + 1) ||
			picnum == (MULTISWITCH + 2) || picnum == (MULTISWITCH + 3))
			lotag += picnum - MULTISWITCH;

		x = headspritestat[3];
		while (x >= 0)
		{
			if (sprite[x].hitag == lotag)
			{
				switch (sprite[x].lotag)
				{
				case SE_12_LIGHT_SWITCH:
					sector[sprite[x].sectnum].floorpal = 0;
					hittype[x].temp_data[0]++;
					if (hittype[x].temp_data[0] == 2)
						hittype[x].temp_data[0]++;

					break;
				case SE_24_CONVEYOR:
				case SE_34:
				case SE_25_PISTON:
					hittype[x].temp_data[4] = !hittype[x].temp_data[4];
					if (hittype[x].temp_data[4])
						FTA(15, &ps[snum]);
					else FTA(2, &ps[snum]);
					break;
				case SE_21_DROP_FLOOR:
					FTA(2, &ps[screenpeek]);
					break;
				}
			}
			x = nextspritestat[x];
		}

		operateactivators(lotag, snum);
		fi.operateforcefields(ps[snum].i, lotag);
		operatemasterswitches(lotag);

		if (picnum == DIPSWITCH || picnum == DIPSWITCH + 1 ||
			picnum == ALIENSWITCH || picnum == ALIENSWITCH + 1 ||
			picnum == TECHSWITCH || picnum == TECHSWITCH + 1) return 1;

		if (hitag == 0 && fi.isadoorwall(picnum) == 0)
		{
			if (switchtype == SWITCH_SPRITE)
				S_PlaySound3D(SWITCH_ON, w, &v);
			else S_PlaySound3D(SWITCH_ON, ps[snum].i, &v);
		}
		else if (hitag != 0)
		{
			auto flags = S_GetUserFlags(hitag);

			if (switchtype == SWITCH_SPRITE && (flags & SF_TALK) == 0)
				S_PlaySound3D(hitag, w, &v);
			else
				A_PlaySound(hitag, ps[snum].i);
		}

		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void activatebysector_d(int sect, int j)
{
	short i, didit;

	didit = 0;

	i = headspritesect[sect];
	while (i >= 0)
	{
		if (sprite[i].picnum == ACTIVATOR)
		{
			operateactivators(sprite[i].lotag, -1);
			didit = 1;
			//			return;
		}
		i = nextspritesect[i];
	}

	if (didit == 0)
		operatesectors(sect, j);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkhitwall_d(int spr, int dawallnum, int x, int y, int z, int atwith)
{
	short j, i, sn = -1, darkestwall;
	walltype* wal;

	wal = &wall[dawallnum];

	if (wal->overpicnum == MIRROR)
	{
		switch (atwith)
		{
		case HEAVYHBOMB:
		case RADIUSEXPLOSION:
		case RPG:
		case HYDRENT:
		case SEENINE:
		case OOZFILTER:
		case EXPLODINGBARREL:
			lotsofglass(spr, dawallnum, 70);
			wal->cstat &= ~16;
			wal->overpicnum = MIRRORBROKE;
			spritesound(GLASS_HEAVYBREAK, spr);
			return;
		}
	}

	if (((wal->cstat & 16) || wal->overpicnum == BIGFORCE) && wal->nextsector >= 0)
		if (sector[wal->nextsector].floorz > z)
			if (sector[wal->nextsector].floorz - sector[wal->nextsector].ceilingz)
				switch (wal->overpicnum)
				{
				case W_FORCEFIELD:
				case W_FORCEFIELD + 1:
				case W_FORCEFIELD + 2:
					wal->extra = 1; // tell the forces to animate
				case BIGFORCE:
					updatesector(x, y, &sn);
					if (sn < 0) return;

					if (atwith == -1)
						i = EGS(sn, x, y, z, FORCERIPPLE, -127, 8, 8, 0, 0, 0, spr, 5);
					else
					{
						if (atwith == CHAINGUN)
							i = EGS(sn, x, y, z, FORCERIPPLE, -127, 16 + sprite[spr].xrepeat, 16 + sprite[spr].yrepeat, 0, 0, 0, spr, 5);
						else i = EGS(sn, x, y, z, FORCERIPPLE, -127, 32, 32, 0, 0, 0, spr, 5);
					}

					sprite[i].cstat |= 18 + 128;
					sprite[i].ang = getangle(wal->x - wall[wal->point2].x,
						wal->y - wall[wal->point2].y) - 512;

					spritesound(SOMETHINGHITFORCE, i);

					return;

				case FANSPRITE:
					wal->overpicnum = FANSPRITEBROKE;
					wal->cstat &= 65535 - 65;
					if (wal->nextwall >= 0)
					{
						wall[wal->nextwall].overpicnum = FANSPRITEBROKE;
						wall[wal->nextwall].cstat &= 65535 - 65;
					}
					spritesound(VENT_BUST, spr);
					spritesound(GLASS_BREAKING, spr);
					return;

				case GLASS:
					updatesector(x, y, &sn); if (sn < 0) return;
					wal->overpicnum = GLASS2;
					lotsofglass(spr, dawallnum, 10);
					wal->cstat = 0;

					if (wal->nextwall >= 0)
						wall[wal->nextwall].cstat = 0;

					i = EGS(sn, x, y, z, SECTOREFFECTOR, 0, 0, 0, ps[0].getang(), 0, 0, spr, 3);
					sprite[i].lotag = 128; hittype[i].temp_data[1] = 5; hittype[i].temp_data[2] = dawallnum;
					spritesound(GLASS_BREAKING, i);
					return;
				case STAINGLASS1:
					updatesector(x, y, &sn); if (sn < 0) return;
					lotsofcolourglass(spr, dawallnum, 80);
					wal->cstat = 0;
					if (wal->nextwall >= 0)
						wall[wal->nextwall].cstat = 0;
					spritesound(VENT_BUST, spr);
					spritesound(GLASS_BREAKING, spr);
					return;
				}

	switch (wal->picnum)
	{
	case COLAMACHINE:
	case VENDMACHINE:
		breakwall(wal->picnum + 2, spr, dawallnum);
		spritesound(VENT_BUST, spr);
		return;

	case OJ:
	case FEMPIC2:
	case FEMPIC3:

	case SCREENBREAK6:
	case SCREENBREAK7:
	case SCREENBREAK8:

	case SCREENBREAK1:
	case SCREENBREAK2:
	case SCREENBREAK3:
	case SCREENBREAK4:
	case SCREENBREAK5:

	case SCREENBREAK9:
	case SCREENBREAK10:
	case SCREENBREAK11:
	case SCREENBREAK12:
	case SCREENBREAK13:
	case SCREENBREAK14:
	case SCREENBREAK15:
	case SCREENBREAK16:
	case SCREENBREAK17:
	case SCREENBREAK18:
	case SCREENBREAK19:
	case BORNTOBEWILDSCREEN:

		lotsofglass(spr, dawallnum, 30);
		wal->picnum = W_SCREENBREAK + (krand() % 3);
		spritesound(GLASS_HEAVYBREAK, spr);
		return;

	case W_TECHWALL5:
	case W_TECHWALL6:
	case W_TECHWALL7:
	case W_TECHWALL8:
	case W_TECHWALL9:
		breakwall(wal->picnum + 1, spr, dawallnum);
		return;
	case W_MILKSHELF:
		breakwall(W_MILKSHELFBROKE, spr, dawallnum);
		return;

	case W_TECHWALL10:
		breakwall(W_HITTECHWALL10, spr, dawallnum);
		return;

	case W_TECHWALL1:
	case W_TECHWALL11:
	case W_TECHWALL12:
	case W_TECHWALL13:
	case W_TECHWALL14:
		breakwall(W_HITTECHWALL1, spr, dawallnum);
		return;

	case W_TECHWALL15:
		breakwall(W_HITTECHWALL15, spr, dawallnum);
		return;

	case W_TECHWALL16:
		breakwall(W_HITTECHWALL16, spr, dawallnum);
		return;

	case W_TECHWALL2:
		breakwall(W_HITTECHWALL2, spr, dawallnum);
		return;

	case W_TECHWALL3:
		breakwall(W_HITTECHWALL3, spr, dawallnum);
		return;

	case W_TECHWALL4:
		breakwall(W_HITTECHWALL4, spr, dawallnum);
		return;

	case ATM:
		wal->picnum = ATMBROKE;
		fi.lotsofmoney(&sprite[spr], 1 + (krand() & 7));
		spritesound(GLASS_HEAVYBREAK, spr);
		break;

	case WALLLIGHT1:
	case WALLLIGHT2:
	case WALLLIGHT3:
	case WALLLIGHT4:
	case TECHLIGHT2:
	case TECHLIGHT4:

		if (rnd(128))
			spritesound(GLASS_HEAVYBREAK, spr);
		else spritesound(GLASS_BREAKING, spr);
		lotsofglass(spr, dawallnum, 30);

		if (wal->picnum == WALLLIGHT1)
			wal->picnum = WALLLIGHTBUST1;

		if (wal->picnum == WALLLIGHT2)
			wal->picnum = WALLLIGHTBUST2;

		if (wal->picnum == WALLLIGHT3)
			wal->picnum = WALLLIGHTBUST3;

		if (wal->picnum == WALLLIGHT4)
			wal->picnum = WALLLIGHTBUST4;

		if (wal->picnum == TECHLIGHT2)
			wal->picnum = TECHLIGHTBUST2;

		if (wal->picnum == TECHLIGHT4)
			wal->picnum = TECHLIGHTBUST4;

		if (!wal->lotag) return;

		sn = wal->nextsector;
		if (sn < 0) return;
		darkestwall = 0;

		wal = &wall[sector[sn].wallptr];
		for (i = sector[sn].wallnum; i > 0; i--, wal++)
			if (wal->shade > darkestwall)
				darkestwall = wal->shade;

		j = krand() & 1;
		i = headspritestat[3];
		while (i >= 0)
		{
			if (sprite[i].hitag == wall[dawallnum].lotag && sprite[i].lotag == 3)
			{
				hittype[i].temp_data[2] = j;
				hittype[i].temp_data[3] = darkestwall;
				hittype[i].temp_data[4] = 1;
			}
			i = nextspritestat[i];
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkplayerhurt_d(struct player_struct* p, int j)
{
	if ((j & 49152) == 49152)
	{
		j &= (MAXSPRITES - 1);

		switch (sprite[j].picnum)
		{
		case CACTUS:
			if (p->hurt_delay < 8)
			{
				sprite[p->i].extra -= 5;
				p->hurt_delay = 16;
				SetPlayerPal(p, PalEntry(32, 32, 0, 0));
				spritesound(DUKE_LONGTERM_PAIN, p->i);
			}
			break;
		}
		return;
	}

	if ((j & 49152) != 32768) return;
	j &= (MAXWALLS - 1);

	if (p->hurt_delay > 0) p->hurt_delay--;
	else if (wall[j].cstat & 85) switch (wall[j].overpicnum)
	{
	case W_FORCEFIELD:
	case W_FORCEFIELD + 1:
	case W_FORCEFIELD + 2:
		sprite[p->i].extra -= 5;

		p->hurt_delay = 16;
		SetPlayerPal(p, PalEntry(32, 32, 0, 0));

		p->posxv = -(sintable[(p->getang() + 512) & 2047] << 8);
		p->posyv = -(sintable[(p->getang()) & 2047] << 8);
		spritesound(DUKE_LONGTERM_PAIN, p->i);

		fi.checkhitwall(p->i, j,
			p->posx + (sintable[(p->getang() + 512) & 2047] >> 9),
			p->posy + (sintable[p->getang() & 2047] >> 9),
			p->posz, -1);

		break;

	case BIGFORCE:
		p->hurt_delay = 26;
		fi.checkhitwall(p->i, j,
			p->posx + (sintable[(p->getang() + 512) & 2047] >> 9),
			p->posy + (sintable[p->getang() & 2047] >> 9),
			p->posz, -1);
		break;

	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool checkhitceiling_d(int sn)
{
	int i, j;

	switch (sector[sn].ceilingpicnum)
	{
	case WALLLIGHT1:
	case WALLLIGHT2:
	case WALLLIGHT3:
	case WALLLIGHT4:
	case TECHLIGHT2:
	case TECHLIGHT4:

		ceilingglass(ps[myconnectindex].i, sn, 10);
		spritesound(GLASS_BREAKING, ps[screenpeek].i);

		if (sector[sn].ceilingpicnum == WALLLIGHT1)
			sector[sn].ceilingpicnum = WALLLIGHTBUST1;

		if (sector[sn].ceilingpicnum == WALLLIGHT2)
			sector[sn].ceilingpicnum = WALLLIGHTBUST2;

		if (sector[sn].ceilingpicnum == WALLLIGHT3)
			sector[sn].ceilingpicnum = WALLLIGHTBUST3;

		if (sector[sn].ceilingpicnum == WALLLIGHT4)
			sector[sn].ceilingpicnum = WALLLIGHTBUST4;

		if (sector[sn].ceilingpicnum == TECHLIGHT2)
			sector[sn].ceilingpicnum = TECHLIGHTBUST2;

		if (sector[sn].ceilingpicnum == TECHLIGHT4)
			sector[sn].ceilingpicnum = TECHLIGHTBUST4;


		if (!sector[sn].hitag)
		{
			i = headspritesect[sn];
			while (i >= 0)
			{
				if (sprite[i].picnum == SECTOREFFECTOR && sprite[i].lotag == 12)
				{
					j = headspritestat[3];
					while (j >= 0)
					{
						if (sprite[j].hitag == sprite[i].hitag)
							hittype[j].temp_data[3] = 1;
						j = nextspritestat[j];
					}
					break;
				}
				i = nextspritesect[i];
			}
		}

		i = headspritestat[3];
		j = krand() & 1;
		while (i >= 0)
		{
			if (sprite[i].hitag == (sector[sn].hitag) && sprite[i].lotag == 3)
			{
				hittype[i].temp_data[2] = j;
				hittype[i].temp_data[4] = 1;
			}
			i = nextspritestat[i];
		}

		return 1;
	}

	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkhitsprite_d(int i, int sn)
{
	int j, k, p;
	spritetype* s;

	i &= (MAXSPRITES - 1);

	switch (sprite[i].picnum)
	{
	case WTGLASS1:
	case WTGLASS2:
		if (!isWorldTour())
			break;
		A_PlaySound(GLASS_BREAKING, i);
		lotsofglass(i, -1, 10);
		deletesprite(i);
		return;

	case OCEANSPRITE1:
	case OCEANSPRITE2:
	case OCEANSPRITE3:
	case OCEANSPRITE4:
	case OCEANSPRITE5:
		fi.spawn(i, SMALLSMOKE);
		deletesprite(i);
		break;
	case QUEBALL:
	case STRIPEBALL:
		if (sprite[sn].picnum == QUEBALL || sprite[sn].picnum == STRIPEBALL)
		{
			sprite[sn].xvel = (sprite[i].xvel >> 1) + (sprite[i].xvel >> 2);
			sprite[sn].ang -= (sprite[i].ang << 1) + 1024;
			sprite[i].ang = getangle(sprite[i].x - sprite[sn].x, sprite[i].y - sprite[sn].y) - 512;
			if (S_CheckSoundPlaying(POOLBALLHIT) < 2)
				spritesound(POOLBALLHIT, i);
		}
		else
		{
			if (krand() & 3)
			{
				sprite[i].xvel = 164;
				sprite[i].ang = sprite[sn].ang;
			}
			else
			{
				lotsofglass(i, -1, 3);
				deletesprite(i);
			}
		}
		break;
	case TREE1:
	case TREE2:
	case TIRE:
	case CONE:
	case BOX:
		switch (sprite[sn].picnum)
		{
		case RADIUSEXPLOSION:
		case RPG:
		case FIRELASER:
		case HYDRENT:
		case HEAVYHBOMB:
			if (hittype[i].temp_data[0] == 0)
			{
				sprite[i].cstat &= ~257;
				hittype[i].temp_data[0] = 1;
				fi.spawn(i, BURNING);
			}
			break;
		}
		break;
	case CACTUS:
		//		case CACTUSBROKE:
		switch (sprite[sn].picnum)
		{
		case RADIUSEXPLOSION:
		case RPG:
		case FIRELASER:
		case HYDRENT:
		case HEAVYHBOMB:
			for (k = 0; k < 64; k++)
			{
				j = EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (krand() % (48 << 8)), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (sprite[i].zvel >> 2), i, 5);
				sprite[j].pal = 8;
			}

			if (sprite[i].picnum == CACTUS)
				sprite[i].picnum = CACTUSBROKE;
			sprite[i].cstat &= ~257;
			//	   else deletesprite(i);
			break;
		}
		break;

	case HANGLIGHT:
	case GENERICPOLE2:
		for (k = 0; k < 6; k++)
			EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (8 << 8), SCRAP1 + (krand() & 15), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (sprite[i].zvel >> 2), i, 5);
		spritesound(GLASS_HEAVYBREAK, i);
		deletesprite(i);
		break;


	case FANSPRITE:
		sprite[i].picnum = FANSPRITEBROKE;
		sprite[i].cstat &= (65535 - 257);
		if (sector[sprite[i].sectnum].floorpicnum == FANSHADOW)
			sector[sprite[i].sectnum].floorpicnum = FANSHADOWBROKE;

		spritesound(GLASS_HEAVYBREAK, i);
		s = &sprite[i];
		for (j = 0; j < 16; j++) RANDOMSCRAP(s, i);

		break;
	case WATERFOUNTAIN:
	case WATERFOUNTAIN + 1:
	case WATERFOUNTAIN + 2:
	case WATERFOUNTAIN + 3:
		sprite[i].picnum = WATERFOUNTAINBROKE;
		fi.spawn(i, TOILETWATER);
		break;
	case SATELITE:
	case FUELPOD:
	case SOLARPANNEL:
	case ANTENNA:
		if (actorinfo[SHOTSPARK1].scriptaddress && sprite[sn].extra != ScriptCode[actorinfo[SHOTSPARK1].scriptaddress])
		{
			for (j = 0; j < 15; j++)
				EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sector[sprite[i].sectnum].floorz - (12 << 8) - (j << 9), SCRAP1 + (krand() & 15), -8, 64, 64,
					krand() & 2047, (krand() & 127) + 64, -(krand() & 511) - 256, i, 5);
			fi.spawn(i, EXPLOSION2);
			deletesprite(i);
		}
		break;
	case BOTTLE1:
	case BOTTLE2:
	case BOTTLE3:
	case BOTTLE4:
	case BOTTLE5:
	case BOTTLE6:
	case BOTTLE8:
	case BOTTLE10:
	case BOTTLE11:
	case BOTTLE12:
	case BOTTLE13:
	case BOTTLE14:
	case BOTTLE15:
	case BOTTLE16:
	case BOTTLE17:
	case BOTTLE18:
	case BOTTLE19:
	case WATERFOUNTAINBROKE:
	case DOMELITE:
	case SUSHIPLATE1:
	case SUSHIPLATE2:
	case SUSHIPLATE3:
	case SUSHIPLATE4:
	case SUSHIPLATE5:
	case WAITTOBESEATED:
	case VASE:
	case STATUEFLASH:
	case STATUE:
		if (sprite[i].picnum == BOTTLE10)
			fi.lotsofmoney(&sprite[i], 4 + (krand() & 3));
		else if (sprite[i].picnum == STATUE || sprite[i].picnum == STATUEFLASH)
		{
			lotsofcolourglass(i, -1, 40);
			spritesound(GLASS_HEAVYBREAK, i);
		}
		else if (sprite[i].picnum == VASE)
			lotsofglass(i, -1, 40);

		spritesound(GLASS_BREAKING, i);
		sprite[i].ang = krand() & 2047;
		lotsofglass(i, -1, 8);
		deletesprite(i);
		break;
	case FETUS:
		sprite[i].picnum = FETUSBROKE;
		spritesound(GLASS_BREAKING, i);
		lotsofglass(i, -1, 10);
		break;
	case FETUSBROKE:
		for (j = 0; j < 48; j++)
		{
			fi.shoot(i, BLOODSPLAT1);
			sprite[i].ang += 333;
		}
		spritesound(GLASS_HEAVYBREAK, i);
		spritesound(SQUISHED, i);
	case BOTTLE7:
		spritesound(GLASS_BREAKING, i);
		lotsofglass(i, -1, 10);
		deletesprite(i);
		break;
	case HYDROPLANT:
		sprite[i].picnum = BROKEHYDROPLANT;
		spritesound(GLASS_BREAKING, i);
		lotsofglass(i, -1, 10);
		break;

	case FORCESPHERE:
		sprite[i].xrepeat = 0;
		hittype[sprite[i].owner].temp_data[0] = 32;
		hittype[sprite[i].owner].temp_data[1] = !hittype[sprite[i].owner].temp_data[1];
		hittype[sprite[i].owner].temp_data[2] ++;
		fi.spawn(i, EXPLOSION2);
		break;

	case BROKEHYDROPLANT:
		if (sprite[i].cstat & 1)
		{
			spritesound(GLASS_BREAKING, i);
			sprite[i].z += 16 << 8;
			sprite[i].cstat = 0;
			lotsofglass(i, -1, 5);
		}
		break;

	case TOILET:
		sprite[i].picnum = TOILETBROKE;
		sprite[i].cstat |= (krand() & 1) << 2;
		sprite[i].cstat &= ~257;
		fi.spawn(i, TOILETWATER);
		spritesound(GLASS_BREAKING, i);
		break;

	case STALL:
		sprite[i].picnum = STALLBROKE;
		sprite[i].cstat |= (krand() & 1) << 2;
		sprite[i].cstat &= ~257;
		fi.spawn(i, TOILETWATER);
		spritesound(GLASS_HEAVYBREAK, i);
		break;

	case HYDRENT:
		sprite[i].picnum = BROKEFIREHYDRENT;
		fi.spawn(i, TOILETWATER);

		//			for(k=0;k<5;k++)
		  //		  {
			//			j = EGS(sprite[i].sectnum,sprite[i].x,sprite[i].y,sprite[i].z-(krand()%(48<<8)),SCRAP3+(krand()&3),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(sprite[i].zvel>>2),i,5);
			  //		  sprite[j].pal = 2;
				//	}
		spritesound(GLASS_HEAVYBREAK, i);
		break;

	case GRATE1:
		sprite[i].picnum = BGRATE1;
		sprite[i].cstat &= (65535 - 256 - 1);
		spritesound(VENT_BUST, i);
		break;

	case CIRCLEPANNEL:
		sprite[i].picnum = CIRCLEPANNELBROKE;
		sprite[i].cstat &= (65535 - 256 - 1);
		spritesound(VENT_BUST, i);
		break;
	case PANNEL1:
	case PANNEL2:
		sprite[i].picnum = BPANNEL1;
		sprite[i].cstat &= (65535 - 256 - 1);
		spritesound(VENT_BUST, i);
		break;
	case PANNEL3:
		sprite[i].picnum = BPANNEL3;
		sprite[i].cstat &= (65535 - 256 - 1);
		spritesound(VENT_BUST, i);
		break;
	case PIPE1:
	case PIPE2:
	case PIPE3:
	case PIPE4:
	case PIPE5:
	case PIPE6:
		switch (sprite[i].picnum)
		{
		case PIPE1:sprite[i].picnum = PIPE1B; break;
		case PIPE2:sprite[i].picnum = PIPE2B; break;
		case PIPE3:sprite[i].picnum = PIPE3B; break;
		case PIPE4:sprite[i].picnum = PIPE4B; break;
		case PIPE5:sprite[i].picnum = PIPE5B; break;
		case PIPE6:sprite[i].picnum = PIPE6B; break;
		}

		j = fi.spawn(i, STEAM);
		sprite[j].z = sector[sprite[i].sectnum].floorz - (32 << 8);
		break;

	case MONK:
	case LUKE:
	case INDY:
	case JURYGUY:
		spritesound(sprite[i].lotag, i);
		fi.spawn(i, sprite[i].hitag);
	case SPACEMARINE:
		sprite[i].extra -= sprite[sn].extra;
		if (sprite[i].extra > 0) break;
		sprite[i].ang = krand() & 2047;
		fi.shoot(i, BLOODSPLAT1);
		sprite[i].ang = krand() & 2047;
		fi.shoot(i, BLOODSPLAT2);
		sprite[i].ang = krand() & 2047;
		fi.shoot(i, BLOODSPLAT3);
		sprite[i].ang = krand() & 2047;
		fi.shoot(i, BLOODSPLAT4);
		sprite[i].ang = krand() & 2047;
		fi.shoot(i, BLOODSPLAT1);
		sprite[i].ang = krand() & 2047;
		fi.shoot(i, BLOODSPLAT2);
		sprite[i].ang = krand() & 2047;
		fi.shoot(i, BLOODSPLAT3);
		sprite[i].ang = krand() & 2047;
		fi.shoot(i, BLOODSPLAT4);
		fi.guts(&sprite[i], JIBS1, 1, myconnectindex);
		fi.guts(&sprite[i], JIBS2, 2, myconnectindex);
		fi.guts(&sprite[i], JIBS3, 3, myconnectindex);
		fi.guts(&sprite[i], JIBS4, 4, myconnectindex);
		fi.guts(&sprite[i], JIBS5, 1, myconnectindex);
		fi.guts(&sprite[i], JIBS3, 6, myconnectindex);
		sound(SQUISHED);
		deletesprite(i);
		break;
	case CHAIR1:
	case CHAIR2:
		sprite[i].picnum = BROKENCHAIR;
		sprite[i].cstat = 0;
		break;
	case CHAIR3:
	case MOVIECAMERA:
	case SCALE:
	case VACUUM:
	case CAMERALIGHT:
	case IVUNIT:
	case POT1:
	case POT2:
	case POT3:
	case TRIPODCAMERA:
		spritesound(GLASS_HEAVYBREAK, i);
		s = &sprite[i];
		for (j = 0; j < 16; j++) RANDOMSCRAP(s, i);
		deletesprite(i);
		break;
	case PLAYERONWATER:
		i = sprite[i].owner;
	default:
		if ((sprite[i].cstat & 16) && sprite[i].hitag == 0 && sprite[i].lotag == 0 && sprite[i].statnum == 0)
			break;

		if ((sprite[sn].picnum == FREEZEBLAST || sprite[sn].owner != i) && sprite[i].statnum != 4)
		{
			if (badguy(&sprite[i]) == 1)
			{
				if (sprite[sn].picnum == RPG) sprite[sn].extra <<= 1;

				if ((sprite[i].picnum != DRONE) && (sprite[i].picnum != ROTATEGUN) && (sprite[i].picnum != COMMANDER) && (sprite[i].picnum < GREENSLIME || sprite[i].picnum > GREENSLIME + 7))
					if (sprite[sn].picnum != FREEZEBLAST)
						//if (actortype[sprite[i].picnum] == 0) //TRANSITIONAL. Cannot be done right with EDuke mess backing the engine. 
						{
							j = fi.spawn(sn, JIBS6);
							if (sprite[sn].pal == 6)
								sprite[j].pal = 6;
							sprite[j].z += (4 << 8);
							sprite[j].xvel = 16;
							sprite[j].xrepeat = sprite[j].yrepeat = 24;
							sprite[j].ang += 32 - (krand() & 63);
						}

				j = sprite[sn].owner;

				if (j >= 0 && sprite[j].picnum == APLAYER && sprite[i].picnum != ROTATEGUN && sprite[i].picnum != DRONE)
					if (ps[sprite[j].yvel].curr_weapon == SHOTGUN_WEAPON)
					{
						fi.shoot(i, BLOODSPLAT3);
						fi.shoot(i, BLOODSPLAT1);
						fi.shoot(i, BLOODSPLAT2);
						fi.shoot(i, BLOODSPLAT4);
					}

				if (sprite[i].picnum != TANK && sprite[i].picnum != BOSS1 && sprite[i].picnum != BOSS4 && sprite[i].picnum != BOSS2 && sprite[i].picnum != BOSS3 && sprite[i].picnum != RECON && sprite[i].picnum != ROTATEGUN)
				{
					if ((sprite[i].cstat & 48) == 0)
						sprite[i].ang = (sprite[sn].ang + 1024) & 2047;
					sprite[i].xvel = -(sprite[sn].extra << 2);
					short j = sprite[i].sectnum;
					pushmove(&sprite[i].x, &sprite[i].y, &sprite[i].z, &j, 128L, (4L << 8), (4L << 8), CLIPMASK0);
					if (j != sprite[i].sectnum && j >= 0 && j < MAXSECTORS)
						changespritesect(i, j);
				}

				if (sprite[i].statnum == 2)
				{
					changespritestat(i, 1);
					hittype[i].timetosleep = SLEEPTIME;
				}
				if ((sprite[i].xrepeat < 24 || sprite[i].picnum == SHARK) && sprite[sn].picnum == SHRINKSPARK) return;
			}

			if (sprite[i].statnum != 2)
			{
				if (sprite[sn].picnum == FREEZEBLAST && ((sprite[i].picnum == APLAYER && sprite[i].pal == 1) || (freezerhurtowner == 0 && sprite[sn].owner == i)))
					return;

				hittype[i].picnum = sprite[sn].picnum;
				hittype[i].extra += sprite[sn].extra;
				hittype[i].ang = sprite[sn].ang;
				hittype[i].owner = sprite[sn].owner;
			}

			if (sprite[i].statnum == 10)
			{
				p = sprite[i].yvel;
				if (ps[p].newowner >= 0)
				{
					ps[p].newowner = -1;
					ps[p].posx = ps[p].oposx;
					ps[p].posy = ps[p].oposy;
					ps[p].posz = ps[p].oposz;
					ps[p].q16ang = ps[p].oq16ang;

					updatesector(ps[p].posx, ps[p].posy, &ps[p].cursectnum);
					setpal(&ps[p]);

					j = headspritestat[1];
					while (j >= 0)
					{
						if (sprite[j].picnum == CAMERA1) sprite[j].yvel = 0;
						j = nextspritestat[j];
					}
				}

				if (sprite[i].xrepeat < 24 && sprite[sn].picnum == SHRINKSPARK)
					return;

				if (sprite[hittype[i].owner].picnum != APLAYER)
					if (ud.player_skill >= 3)
						sprite[sn].extra += (sprite[sn].extra >> 1);
			}

		}
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checksectors_d(int snum)
{
	int i = -1, oldz;
	struct player_struct* p;
	int j, hitscanwall;
	short neartagsector, neartagwall, neartagsprite;
	int neartaghitdist;

	p = &ps[snum];

	switch (sector[p->cursectnum].lotag)
	{

	case 32767:
		sector[p->cursectnum].lotag = 0;
		FTA(9, p);
		p->secret_rooms++;
		return;
	case -1:
		sector[p->cursectnum].lotag = 0;
		setnextmap(false);
		return;
	case -2:
		sector[p->cursectnum].lotag = 0;
		p->timebeforeexit = 26 * 8;
		p->customexitsound = sector[p->cursectnum].hitag;
		return;
	default:
		if (sector[p->cursectnum].lotag >= 10000 && sector[p->cursectnum].lotag < 16383)
		{
			if (snum == screenpeek || ud.coop == 1)
				spritesound(sector[p->cursectnum].lotag - 10000, p->i);
			sector[p->cursectnum].lotag = 0;
		}
		break;

	}

	//After this point the the player effects the map with space

	if (chatmodeon || sprite[p->i].extra <= 0) return;

	if (ud.cashman && PlayerInput(snum, SKB_OPEN))
		fi.lotsofmoney(&sprite[p->i], 2);

	if (p->newowner >= 0)
	{
		Printf("%d, %d\n", PlayerInputSideVel(snum), PlayerInputForwardVel(snum));
		if (abs(PlayerInputSideVel(snum)) > 768 || abs(PlayerInputForwardVel(snum)) > 768)
		{
			i = -1;
			goto CLEARCAMERAS;
		}
	}

	if (!(PlayerInput(snum, SKB_OPEN)) && !PlayerInput(snum, SKB_ESCAPE))
		p->toggle_key_flag = 0;

	else if (!p->toggle_key_flag)
	{

		if (PlayerInput(snum, SKB_ESCAPE))
		{
			if (p->newowner >= 0)
			{
				i = -1;
				goto CLEARCAMERAS;
			}
			return;
		}

		neartagsprite = -1;
		p->toggle_key_flag = 1;
		hitscanwall = -1;

		i = hitawall(p, &hitscanwall);

		if (i < 1280 && hitscanwall >= 0 && wall[hitscanwall].overpicnum == MIRROR)
			if (wall[hitscanwall].lotag > 0 && S_CheckSoundPlaying(wall[hitscanwall].lotag) == 0 && snum == screenpeek)
			{
				spritesound(wall[hitscanwall].lotag, p->i);
				return;
			}

		if (hitscanwall >= 0 && (wall[hitscanwall].cstat & 16))
			if (wall[hitscanwall].lotag)
				return;

		if (p->newowner >= 0)
			neartag(p->oposx, p->oposy, p->oposz, sprite[p->i].sectnum, p->getoang(), &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1280L, 1);
		else
		{
			neartag(p->posx, p->posy, p->posz, sprite[p->i].sectnum, p->getoang(), &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1280L, 1);
			if (neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
				neartag(p->posx, p->posy, p->posz + (8 << 8), sprite[p->i].sectnum, p->getoang(), &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1280L, 1);
			if (neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
				neartag(p->posx, p->posy, p->posz + (16 << 8), sprite[p->i].sectnum, p->getoang(), &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1280L, 1);
			if (neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
			{
				neartag(p->posx, p->posy, p->posz + (16 << 8), sprite[p->i].sectnum, p->getoang(), &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1280L, 3);
				if (neartagsprite >= 0)
				{
					switch (sprite[neartagsprite].picnum)
					{
					case FEM1:
					case FEM2:
					case FEM3:
					case FEM4:
					case FEM5:
					case FEM6:
					case FEM7:
					case FEM8:
					case FEM9:
					case FEM10:
					case PODFEM1:
					case NAKED1:
					case STATUE:
					case TOUGHGAL:
						return;
					}
				}

				neartagsprite = -1;
				neartagwall = -1;
				neartagsector = -1;
			}
		}

		if (p->newowner == -1 && neartagsprite == -1 && neartagsector == -1 && neartagwall == -1)
			if (isanunderoperator(sector[sprite[p->i].sectnum].lotag))
				neartagsector = sprite[p->i].sectnum;

		if (neartagsector >= 0 && (sector[neartagsector].lotag & 16384))
			return;

		if (neartagsprite == -1 && neartagwall == -1)
			if (sector[p->cursectnum].lotag == 2)
			{
				oldz = hitasprite(p->i, &neartagsprite);
				if (oldz > 1280) neartagsprite = -1;
			}

		if (neartagsprite >= 0)
		{
			if (fi.checkhitswitch(snum, neartagsprite, 1)) return;

			switch (sprite[neartagsprite].picnum)
			{
			case TOILET:
			case STALL:
				if (p->last_pissed_time == 0)
				{
					if (adult_lockout == 0) spritesound(DUKE_URINATE, p->i);

					p->last_pissed_time = 26 * 220;
					p->transporter_hold = 29 * 2;
					if (p->holster_weapon == 0)
					{
						p->holster_weapon = 1;
						p->weapon_pos = -1;
					}
					if (sprite[p->i].extra <= (max_player_health - (max_player_health / 10)))
					{
						sprite[p->i].extra += max_player_health / 10;
						p->last_extra = sprite[p->i].extra;
					}
					else if (sprite[p->i].extra < max_player_health)
						sprite[p->i].extra = max_player_health;
				}
				else if (S_CheckSoundPlaying(neartagsprite, FLUSH_TOILET) == 0)
					spritesound(FLUSH_TOILET, neartagsprite);
				return;

			case NUKEBUTTON:

				hitawall(p, &j);
				if (j >= 0 && wall[j].overpicnum == 0)
					if (hittype[neartagsprite].temp_data[0] == 0)
					{
						hittype[neartagsprite].temp_data[0] = 1;
						sprite[neartagsprite].owner = p->i;
						p->buttonpalette = sprite[neartagsprite].pal;
						if (p->buttonpalette)
							ud.secretlevel = sprite[neartagsprite].lotag;
						else ud.secretlevel = 0;
					}
				return;
			case WATERFOUNTAIN:
				if (hittype[neartagsprite].temp_data[0] != 1)
				{
					hittype[neartagsprite].temp_data[0] = 1;
					sprite[neartagsprite].owner = p->i;

					if (sprite[p->i].extra < max_player_health)
					{
						sprite[p->i].extra++;
						spritesound(DUKE_DRINKING, p->i);
					}
				}
				return;
			case PLUG:
				spritesound(SHORT_CIRCUIT, p->i);
				sprite[p->i].extra -= 2 + (krand() & 3);
				SetPlayerPal(p, PalEntry(32, 48, 48, 64));
				break;
			case VIEWSCREEN:
			case VIEWSCREEN2:
			{
				i = headspritestat[1];

				while (i >= 0)
				{
					if (sprite[i].picnum == CAMERA1 && sprite[i].yvel == 0 && sprite[neartagsprite].hitag == sprite[i].lotag)
					{
						sprite[i].yvel = 1; //Using this camera
						spritesound(MONITOR_ACTIVE, neartagsprite);

						sprite[neartagsprite].owner = i;
						sprite[neartagsprite].yvel = 1;
						camsprite = neartagsprite;


						j = p->cursectnum;
						p->cursectnum = sprite[i].sectnum;
						setpal(p);
						p->cursectnum = j;

						// parallaxtype = 2;
						p->newowner = i;
						return;
					}
					i = nextspritestat[i];
				}
			}

		CLEARCAMERAS:

			if (i < 0)
			{
				p->posx = p->oposx;
				p->posy = p->oposy;
				p->posz = p->oposz;
				p->q16ang = p->oq16ang;
				p->newowner = -1;

				updatesector(p->posx, p->posy, &p->cursectnum);
				setpal(p);


				i = headspritestat[1];
				while (i >= 0)
				{
					if (sprite[i].picnum == CAMERA1) sprite[i].yvel = 0;
					i = nextspritestat[i];
				}
			}
			else if (p->newowner >= 0)
				p->newowner = -1;

			return;
			}
		}

		if (!PlayerInput(snum, SKB_OPEN)) return;
		else if (p->newowner >= 0) { i = -1; goto CLEARCAMERAS; }

		if (neartagwall == -1 && neartagsector == -1 && neartagsprite == -1)
			if (abs(hits(p->i)) < 512)
			{
				if ((krand() & 255) < 16)
					spritesound(DUKE_SEARCH2, p->i);
				else spritesound(DUKE_SEARCH, p->i);
				return;
			}

		if (neartagwall >= 0)
		{
			if (wall[neartagwall].lotag > 0 && fi.isadoorwall(wall[neartagwall].picnum))
			{
				if (hitscanwall == neartagwall || hitscanwall == -1)
					fi.checkhitswitch(snum, neartagwall, 0);
				return;
			}
			else if (p->newowner >= 0)
			{
				i = -1;
				goto CLEARCAMERAS;
			}
		}

		if (neartagsector >= 0 && (sector[neartagsector].lotag & 16384) == 0 && isanearoperator(sector[neartagsector].lotag))
		{
			i = headspritesect[neartagsector];
			while (i >= 0)
			{
				if (sprite[i].picnum == ACTIVATOR || sprite[i].picnum == MASTERSWITCH)
					return;
				i = nextspritesect[i];
			}
			operatesectors(neartagsector, p->i);
		}
		else if ((sector[sprite[p->i].sectnum].lotag & 16384) == 0)
		{
			if (isanunderoperator(sector[sprite[p->i].sectnum].lotag))
			{
				i = headspritesect[sprite[p->i].sectnum];
				while (i >= 0)
				{
					if (sprite[i].picnum == ACTIVATOR || sprite[i].picnum == MASTERSWITCH) return;
					i = nextspritesect[i];
				}
				operatesectors(sprite[p->i].sectnum, p->i);
			}
			else fi.checkhitswitch(snum, neartagwall, 0);
		}
	}
}





END_DUKE_NS
