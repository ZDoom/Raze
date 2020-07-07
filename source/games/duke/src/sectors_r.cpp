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
#include "sounds.h"
#include "names_r.h"
#include "mapinfo.h"

// PRIMITIVE
BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool isadoorwall_r(int dapic)
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
		case RRTILE1856:
		case RRTILE1877:
			return 1;
	}
	return 0;
}

bool isablockdoor(int dapic)
{
	switch (dapic)
	{
		case RRTILE1792:
		case RRTILE1801:
		case RRTILE1805:
		case RRTILE1807:
		case RRTILE1808:
		case RRTILE1812:
		case RRTILE1821:
		case RRTILE1826:
		case RRTILE1850:
		case RRTILE1851:
		case RRTILE1856:
		case RRTILE1877:
		case RRTILE1938:
		case RRTILE1942:
		case RRTILE1944:
		case RRTILE1945:
		case RRTILE1951:
		case RRTILE1961:
		case RRTILE1964:
		case RRTILE1985:
		case RRTILE1995:
		case RRTILE2022:
		case RRTILE2052:
		case RRTILE2053:
		case RRTILE2060:
		case RRTILE2074:
		case RRTILE2132:
		case RRTILE2136:
		case RRTILE2139:
		case RRTILE2150:
		case RRTILE2178:
		case RRTILE2186:
		case RRTILE2319:
		case RRTILE2321:
		case RRTILE2326:
		case RRTILE2329:
		case RRTILE2578:
		case RRTILE2581:
		case RRTILE2610:
		case RRTILE2613:
		case RRTILE2621:
		case RRTILE2622:
		case RRTILE2676:
		case RRTILE2732:
		case RRTILE2831:
		case RRTILE2832:
		case RRTILE2842:
		case RRTILE2940:
		case RRTILE2970:
		case RRTILE3083:
		case RRTILE3100:
		case RRTILE3155:
		case RRTILE3195:
		case RRTILE3232:
		case RRTILE3600:
		case RRTILE3631:
		case RRTILE3635:
		case RRTILE3637:
		case RRTILE3643+2:
		case RRTILE3643+3:
		case RRTILE3647:
		case RRTILE3652:
		case RRTILE3653:
		case RRTILE3671:
		case RRTILE3673:
		case RRTILE3684:
		case RRTILE3708:
		case RRTILE3714:
		case RRTILE3716:
		case RRTILE3723:
		case RRTILE3725:
		case RRTILE3737:
		case RRTILE3754:
		case RRTILE3762:
		case RRTILE3763:
		case RRTILE3764:
		case RRTILE3765:
		case RRTILE3767:
		case RRTILE3793:
		case RRTILE3814:
		case RRTILE3815:
		case RRTILE3819:
		case RRTILE3827:
		case RRTILE3837:
			return true;
			
		case RRTILE1996:
		case RRTILE2382:
		case RRTILE2961:
		case RRTILE3804:
		case RRTILE7430:
		case RRTILE7467:
		case RRTILE7469:
		case RRTILE7470:
		case RRTILE7475:
		case RRTILE7566:
		case RRTILE7576:
		case RRTILE7716:
		case RRTILE8063:
		case RRTILE8067:
		case RRTILE8076:
		case RRTILE8106:
		case RRTILE8379:
		case RRTILE8380:
		case RRTILE8565:
		case RRTILE8605:
			return isRRRA();
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void animatewalls_r(void)
{
	int i, j, p, t;

	if (isRRRA() &&ps[screenpeek].sea_sick_stat == 1)
	{
		for (i = 0; i < MAXWALLS; i++)
		{
			if (wall[i].picnum == RRTILE7873)
				wall[i].xpanning += 6;
			else if (wall[i].picnum == RRTILE7870)
				wall[i].xpanning += 6;
		}
	}

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

			if ((krand() & 255) < 16)
			{
				animwall[p].tag = wall[i].picnum;
				wall[i].picnum = SCREENBREAK6;
			}

			continue;

		case SCREENBREAK6:
		case SCREENBREAK7:
		case SCREENBREAK8:

			if (animwall[p].tag >= 0)
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

void operaterespawns_r(int low)
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
		case RRTILE7424:
			if (isRRRA() && !ud.monsters_off)
				changespritestat(i, 119);
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

void operateforcefields_r(int s, int low)
{
	operateforcefields_common(s, low, { BIGFORCE });
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool checkhitswitch_r(int snum, int w, int switchtype)
{
	char switchpal;
	short i, x, lotag, hitag, picnum, correctdips, numdips;
	int sx, sy;

	if (w < 0) return 0;
	correctdips = 1;
	numdips = 0;

	if (switchtype == 1) // A wall sprite
	{
		lotag = sprite[w].lotag; if (lotag == 0) return 0;
		hitag = sprite[w].hitag;
		sx = sprite[w].x;
		sy = sprite[w].y;
		picnum = sprite[w].picnum;
		switchpal = sprite[w].pal;
	}
	else
	{
		lotag = wall[w].lotag; if (lotag == 0) return 0;
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
	case ACCESSSWITCH:
	case ACCESSSWITCH2:
		if (ps[snum].access_incs == 0)
		{
			if (switchpal == 0)
			{
				if (ps[snum].keys[1])
					ps[snum].access_incs = 1;
				else
				{
					FTA(70, &ps[snum]);
					if (isRRRA()) spritesound(99, w);
				}
			}

			else if (switchpal == 21)
			{
				if (ps[snum].keys[2])
					ps[snum].access_incs = 1;
				else
				{
					FTA(71, &ps[snum]);
					if (isRRRA()) spritesound(99, w);
				}
			}

			else if (switchpal == 23)
			{
				if (ps[snum].keys[3])
					ps[snum].access_incs = 1;
				else
				{
					FTA(72, &ps[snum]);
					if (isRRRA()) spritesound(99, w);
				}
			}

			if (ps[snum].access_incs == 1)
			{
				if (switchtype == 0)
					ps[snum].access_wallnum = w;
				else
					ps[snum].access_spritenum = w;
			}

			return 0;
		}
	case MULTISWITCH2:
	case MULTISWITCH2 + 1:
	case MULTISWITCH2 + 2:
	case MULTISWITCH2 + 3:
	case RRTILE8464:
	case RRTILE8660:
		if (isRRRA()) break;
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
	case NUKEBUTTON:
	case NUKEBUTTON + 1:
	case RRTILE2214:
	case RRTILE2697:
	case RRTILE2697 + 1:
	case RRTILE2707:
	case RRTILE2707 + 1:
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
			if (switchtype == 1 && w == i) sprite[i].picnum++;
			else if (sprite[i].hitag == 0) correctdips++;
			numdips++;
			break;
		case TECHSWITCH + 1:
		case DIPSWITCH + 1:
		case ALIENSWITCH + 1:
			if (switchtype == 1 && w == i) sprite[i].picnum--;
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
		case MULTISWITCH2:
		case MULTISWITCH2 + 1:
		case MULTISWITCH2 + 2:
		case MULTISWITCH2 + 3:
			if (!isRRRA()) break;
			sprite[i].picnum++;
			if (sprite[i].picnum > (MULTISWITCH2 + 3))
				sprite[i].picnum = MULTISWITCH2;
			break;

		case RRTILE2214:
			//if (ud.level_numbe r > 6) ud.level_numbe r = 0; ??? Looks like some leftover garbage.
			sprite[i].picnum++;
			break;
		case RRTILE8660:
			if (!isRRRA()) break;
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
		case NUKEBUTTON:
		case RRTILE2697:
		case RRTILE2707:
			if (sprite[i].picnum == DIPSWITCH3)
				if (sprite[i].hitag == 999)
				{
					short j, nextj;
					j = headspritestat[107];
					while (j >= 0)
					{
						nextj = nextspritestat[j];
						if (sprite[j].picnum == RRTILE3410)
						{
							sprite[j].picnum++;
							sprite[j].hitag = 100;
							sprite[j].extra = 0;
							spritesound(474, j);
						}
						else if (sprite[j].picnum == RRTILE295)
							deletesprite(j);
						j = nextj;
					}
					sprite[i].picnum++;
					break;
				}
			if (sprite[i].picnum == NUKEBUTTON)
				chickenplant = 0;
			if (sprite[i].picnum == RRTILE8660)
			{
				BellTime = 132;
				word_119BE0 = i;
			}
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
		case NUKEBUTTON + 1:
		case RRTILE2697 + 1:
		case RRTILE2707 + 1:
			if (sprite[i].picnum == NUKEBUTTON + 1)
				chickenplant = 1;
			if (sprite[i].hitag != 999)
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
				if (switchtype == 0 && i == w) wall[x].picnum++;
				else if (wall[x].hitag == 0) correctdips++;
				numdips++;
				break;
			case DIPSWITCH + 1:
			case TECHSWITCH + 1:
			case ALIENSWITCH + 1:
				if (switchtype == 0 && i == w) wall[x].picnum--;
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
			case MULTISWITCH2:
			case MULTISWITCH2 + 1:
			case MULTISWITCH2 + 2:
			case MULTISWITCH2 + 3:
				if (!isRRRA()) break;
				wall[x].picnum++;
				if (wall[x].picnum > (MULTISWITCH2 + 3))
					wall[x].picnum = MULTISWITCH2;
				break;
			case RRTILE8660:
				if (!isRRRA()) break;
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
			case RRTILE2697:
			case RRTILE2707:
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
			case RRTILE2697 + 1:
			case RRTILE2707 + 1:
				wall[x].picnum--;
				break;
			}
	}

	if (lotag == (short)65535)
	{
		setnextmap(false);
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
	case MULTISWITCH2:
	case MULTISWITCH2 + 1:
	case MULTISWITCH2 + 2:
	case MULTISWITCH2 + 3:
	case RRTILE8464:
	case RRTILE8660:
		if (!isRRRA()) break;
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
	case RRTILE2697:
	case RRTILE2697 + 1:
	case RRTILE2707:
	case RRTILE2707 + 1:
		if (isRRRA())
		{
			if (picnum == RRTILE8660)
			{
				BellTime = 132;
				word_119BE0 = w;
				sprite[w].picnum++;
			}
			else if (picnum == RRTILE8464)
			{
				sprite[w].picnum = sprite[w].picnum + 1;
				if (hitag == 10001)
				{
					if (ps[snum].SeaSick == 0)
						ps[snum].SeaSick = 350;
					operateactivators(668, ps[snum].i);
					operatemasterswitches(668);
					spritesound(328, ps[snum].i);
					return 1;
				}
			}
			else if (hitag == 10000)
			{
				if (picnum == MULTISWITCH || picnum == (MULTISWITCH + 1) ||
					picnum == (MULTISWITCH + 2) || picnum == (MULTISWITCH + 3) ||
					picnum == MULTISWITCH2 || picnum == (MULTISWITCH2 + 1) ||
					picnum == (MULTISWITCH2 + 2) || picnum == (MULTISWITCH2 + 3))
				{
					int var6c[3], var54, j;
					short jpn, jht;
					var54 = 0;
					S_PlaySound3D(SWITCH_ON, w, &v);
					for (j = 0; j < MAXSPRITES; j++)
					{
						jpn = sprite[j].picnum;
						jht = sprite[j].hitag;
						if ((jpn == MULTISWITCH || jpn == MULTISWITCH2) && jht == 10000)
						{
							if (var54 < 3)
							{
								var6c[var54] = j;
								var54++;
							}
						}
					}
					if (var54 == 3)
					{
						S_PlaySound3D(78, w, &v);
						for (j = 0; j < var54; j++)
						{
							sprite[var6c[j]].hitag = 0;
							if (picnum >= MULTISWITCH2)
								sprite[var6c[j]].picnum = MULTISWITCH2 + 3;
							else
								sprite[var6c[j]].picnum = MULTISWITCH + 3;
							checkhitswitch_r(snum, var6c[j], 1);
						}
					}
					return 1;
				}
			}
		}
		if (picnum == MULTISWITCH || picnum == (MULTISWITCH + 1) ||
			picnum == (MULTISWITCH + 2) || picnum == (MULTISWITCH + 3))
			lotag += picnum - MULTISWITCH;
		if (isRRRA())
		{
			if (picnum == MULTISWITCH2 || picnum == (MULTISWITCH2 + 1) ||
				picnum == (MULTISWITCH2 + 2) || picnum == (MULTISWITCH2 + 3))
				lotag += picnum - MULTISWITCH2;
		}

		x = headspritestat[3];
		while (x >= 0)
		{
			if (((sprite[x].hitag) == lotag))
			{
				switch (sprite[x].lotag)
				{
				case 46:
				case 47:
				case 48:
					if (!isRRRA()) break;
				case 12:
					sector[sprite[x].sectnum].floorpal = 0;
					hittype[x].temp_data[0]++;
					if (hittype[x].temp_data[0] == 2)
						hittype[x].temp_data[0]++;

					break;
				case 24:
				case 34:
				case 25:
					hittype[x].temp_data[4] = !hittype[x].temp_data[4];
					if (hittype[x].temp_data[4])
						FTA(15, &ps[snum]);
					else FTA(2, &ps[snum]);
					break;
				case 21:
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

void activatebysector_r(int sect, int j)
{
	short i;

	i = headspritesect[sect];
	while (i >= 0)
	{
		if (sprite[i].picnum == ACTIVATOR)
		{
			operateactivators(sprite[i].lotag, -1);
			//			return;
		}
		i = nextspritesect[i];
	}

	if (sector[sect].lotag != 22)
		operatesectors(sect, j);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void lotsofpopcorn(short i, short wallnum, short n)
{
	long j, xv, yv, z, x1, y1;
	short sect, a;

	sect = -1;
	auto sp = &sprite[i];

	if (wallnum < 0)
	{
		for (j = n - 1; j >= 0; j--)
		{
			a = sp->ang - 256 + (krand() & 511) + 1024;
			EGS(sp->sectnum, sp->x, sp->y, sp->z, POPCORN, -32, 36, 36, a, 32 + (krand() & 63), 1024 - (krand() & 1023), i, 5);
		}
		return;
	}

	j = n + 1;

	x1 = wall[wallnum].x;
	y1 = wall[wallnum].y;

	xv = wall[wall[wallnum].point2].x - x1;
	yv = wall[wall[wallnum].point2].y - y1;

	x1 -= ksgn(yv);
	y1 += ksgn(xv);

	xv /= j;
	yv /= j;

	for (j = n; j > 0; j--)
	{
		x1 += xv;
		y1 += yv;

		updatesector(x1, y1, &sect);
		if (sect >= 0)
		{
			z = sector[sect].floorz - (krand() & (abs(sector[sect].ceilingz - sector[sect].floorz)));
			if (z < -(32 << 8) || z >(32 << 8))
				z = sp->z - (32 << 8) + (krand() & ((64 << 8) - 1));
			a = sp->ang - 1024;
			EGS(sp->sectnum, x1, y1, z, POPCORN, -32, 36, 36, a, 32 + (krand() & 63), -(krand() & 1023), i, 5);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkhitwall_r(int spr, int dawallnum, int x, int y, int z, int atwith)
{
	short j, i, sn = -1, darkestwall;
	walltype* wal;
	spritetype* s;

	wal = &wall[dawallnum];

	if (wal->overpicnum == MIRROR)
	{
		switch (atwith)
		{
		case RPG2:
			if (!isRRRA()) break;
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

				case RRTILE1973:
					updatesector(x, y, &sn); if (sn < 0) return;
					wal->overpicnum = GLASS2;
					lotsofpopcorn(spr, dawallnum, 64);
					wal->cstat = 0;

					if (wal->nextwall >= 0)
						wall[wal->nextwall].cstat = 0;

					i = EGS(sn, x, y, z, SECTOREFFECTOR, 0, 0, 0, ps[0].getang(), 0, 0, spr, 3);
					sprite[i].lotag = 128; hittype[i].temp_data[1] = 2; hittype[i].temp_data[2] = dawallnum;
					spritesound(GLASS_BREAKING, i);
					return;

				case GLASS:
					updatesector(x, y, &sn); if (sn < 0) return;
					wal->overpicnum = GLASS2;
					lotsofglass(spr, dawallnum, 10);
					wal->cstat = 0;

					if (wal->nextwall >= 0)
						wall[wal->nextwall].cstat = 0;

					i = EGS(sn, x, y, z, SECTOREFFECTOR, 0, 0, 0, ps[0].getang(), 0, 0, spr, 3);
					sprite[i].lotag = 128; hittype[i].temp_data[1] = 2; hittype[i].temp_data[2] = dawallnum;
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
	case RRTILE8464:
		if (isRRRA()) break;
		break;
	case RRTILE3643:
	case RRTILE3643 + 1:
	case RRTILE3643 + 2:
	case RRTILE3643 + 3:
	{
		short sect;
		short unk = 0;
		short jj;
		short nextjj;
		short startwall, endwall;
		sect = wall[wal->nextwall].nextsector;
		jj = headspritesect[sect];
		while (jj != -1)
		{
			nextjj = nextspritesect[jj];
			s = &sprite[jj];
			if (s->lotag == 6)
			{
				for (j = 0; j < 16; j++) RANDOMSCRAP(s, -1);
				g_spriteExtra[jj]++; // TRANSITIONAL move to sprite or actor
				if (g_spriteExtra[jj] == 25)
				{
					startwall = sector[s->sectnum].wallptr;
					endwall = startwall + sector[s->sectnum].wallnum;
					for (i = startwall; i < endwall; i++)
						sector[wall[i].nextsector].lotag = 0;
					sector[s->sectnum].lotag = 0;
					stopsound(sprite[jj].lotag);
					spritesound(400, jj);
					deletesprite(jj);
				}
			}
			jj = nextjj;
		}
		return;
	}
	case RRTILE7555:
		if (!isRRRA()) break;
		wal->picnum = SBMOVE;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7441:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5016;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7559:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5017;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7433:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5018;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7557:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5019;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7553:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5020;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7552:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5021;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7568:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5022;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7540:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5023;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7558:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5024;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7554:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5025;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7579:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5026;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7561:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5027;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7580:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5037;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8227:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5070;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8503:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5079;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8567:
	case RRTILE8568:
	case RRTILE8569:
	case RRTILE8570:
	case RRTILE8571:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5082;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7859:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5081;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8496:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5061;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8617:
		if (!isRRRA()) break;
		if (numplayers < 2)
		{
			wal->picnum = RRTILE8618;
			spritesound(47, spr);
		}
		return;
	case RRTILE8620:
		if (!isRRRA()) break;
		wal->picnum = RRTILE8621;
		spritesound(47, spr);
		return;
	case RRTILE8622:
		if (!isRRRA()) break;
		wal->picnum = RRTILE8623;
		spritesound(495, spr);
		return;
	case RRTILE7657:
		if (!isRRRA()) break;
		wal->picnum = RRTILE7659;
		spritesound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8497:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5076;
		spritesound(495, spr);
		return;
	case RRTILE7533:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5035;
		spritesound(495, spr);
		return;

	case COLAMACHINE:
	case VENDMACHINE:
		breakwall(wal->picnum + 2, spr, dawallnum);
		spritesound(GLASS_BREAKING, spr);
		return;

	case OJ:

	case SCREENBREAK6:
	case SCREENBREAK7:
	case SCREENBREAK8:

		lotsofglass(spr, dawallnum, 30);
		wal->picnum = W_SCREENBREAK + (krand() % (isRRRA() ? 2 : 3));
		spritesound(GLASS_HEAVYBREAK, spr);
		return;

	case ATM:
		wal->picnum = ATMBROKE;
		fi.lotsofmoney(&sprite[spr], 1 + (krand() & 7));
		spritesound(GLASS_HEAVYBREAK, spr);
		break;

	case WALLLIGHT1:
	case WALLLIGHT3:
	case WALLLIGHT4:
	case TECHLIGHT2:
	case TECHLIGHT4:
	case RRTILE1814:
	case RRTILE1939:
	case RRTILE1986:
	case RRTILE1988:
	case RRTILE2123:
	case RRTILE2125:
	case RRTILE2636:
	case RRTILE2878:
	case RRTILE2898:
	case RRTILE3200:
	case RRTILE3202:
	case RRTILE3204:
	case RRTILE3206:
	case RRTILE3208:

		if (rnd(128))
			spritesound(GLASS_HEAVYBREAK, spr);
		else spritesound(GLASS_BREAKING, spr);
		lotsofglass(spr, dawallnum, 30);

		if (wal->picnum == RRTILE1814)
			wal->picnum = RRTILE1817;

		if (wal->picnum == RRTILE1986)
			wal->picnum = RRTILE1987;

		if (wal->picnum == RRTILE1939)
			wal->picnum = RRTILE2004;

		if (wal->picnum == RRTILE1988)
			wal->picnum = RRTILE2005;

		if (wal->picnum == RRTILE2898)
			wal->picnum = RRTILE2899;

		if (wal->picnum == RRTILE2878)
			wal->picnum = RRTILE2879;

		if (wal->picnum == RRTILE2123)
			wal->picnum = RRTILE2124;

		if (wal->picnum == RRTILE2125)
			wal->picnum = RRTILE2126;

		if (wal->picnum == RRTILE3200)
			wal->picnum = RRTILE3201;

		if (wal->picnum == RRTILE3202)
			wal->picnum = RRTILE3203;

		if (wal->picnum == RRTILE3204)
			wal->picnum = RRTILE3205;

		if (wal->picnum == RRTILE3206)
			wal->picnum = RRTILE3207;

		if (wal->picnum == RRTILE3208)
			wal->picnum = RRTILE3209;

		if (wal->picnum == RRTILE2636)
			wal->picnum = RRTILE2637;

		if (wal->picnum == WALLLIGHT1)
			wal->picnum = WALLLIGHTBUST1;

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

void checkplayerhurt_r(struct player_struct* p, int j)
{
	if ((j & 49152) == 49152)
	{
		j &= (MAXSPRITES - 1);

		switch (sprite[j].picnum)
		{
		case RRTILE2430:
		case RRTILE2431:
		case RRTILE2432:
		case RRTILE2443:
		case RRTILE2446:
		case RRTILE2451:
		case RRTILE2455:
			if (isRRRA() && p->hurt_delay2 < 8)
			{
				sprite[p->i].extra -= 2;
				p->hurt_delay2 = 16;
				SetPlayerPal(p, PalEntry(32, 32, 0, 0));
				spritesound(DUKE_LONGTERM_PAIN, p->i);
			}
			break;
		case CACTUS:
			if (!isRRRA() && p->hurt_delay < 8)
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

bool checkhitceiling_r(int sn)
{
	short i, j;

	switch (sector[sn].ceilingpicnum)
	{
	case WALLLIGHT1:
	case WALLLIGHT3:
	case WALLLIGHT4:
	case TECHLIGHT2:
	case TECHLIGHT4:
	case RRTILE1939:
	case RRTILE1986:
	case RRTILE1988:
	case RRTILE2123:
	case RRTILE2125:
	case RRTILE2878:
	case RRTILE2898:


		ceilingglass(ps[myconnectindex].i, sn, 10);
		spritesound(GLASS_BREAKING, ps[screenpeek].i);

		if (sector[sn].ceilingpicnum == WALLLIGHT1)
			sector[sn].ceilingpicnum = WALLLIGHTBUST1;

		if (sector[sn].ceilingpicnum == WALLLIGHT3)
			sector[sn].ceilingpicnum = WALLLIGHTBUST3;

		if (sector[sn].ceilingpicnum == WALLLIGHT4)
			sector[sn].ceilingpicnum = WALLLIGHTBUST4;

		if (sector[sn].ceilingpicnum == TECHLIGHT2)
			sector[sn].ceilingpicnum = TECHLIGHTBUST2;

		if (sector[sn].ceilingpicnum == TECHLIGHT4)
			sector[sn].ceilingpicnum = TECHLIGHTBUST4;

		if (sector[sn].ceilingpicnum == RRTILE1986)
			sector[sn].ceilingpicnum = RRTILE1987;

		if (sector[sn].ceilingpicnum == RRTILE1939)
			sector[sn].ceilingpicnum = RRTILE2004;

		if (sector[sn].ceilingpicnum == RRTILE1988)
			sector[sn].ceilingpicnum = RRTILE2005;

		if (sector[sn].ceilingpicnum == RRTILE2898)
			sector[sn].ceilingpicnum = RRTILE2899;

		if (sector[sn].ceilingpicnum == RRTILE2878)
			sector[sn].ceilingpicnum = RRTILE2879;

		if (sector[sn].ceilingpicnum == RRTILE2123)
			sector[sn].ceilingpicnum = RRTILE2124;

		if (sector[sn].ceilingpicnum == RRTILE2125)
			sector[sn].ceilingpicnum = RRTILE2126;


		if (!sector[sn].hitag)
		{
			i = headspritesect[sn];
			while (i >= 0)
			{
				if (sprite[i].picnum == SECTOREFFECTOR && (sprite[i].lotag == 12 || (isRRRA() && (sprite[i].lotag == 47 || sprite[i].lotag == 48))))
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

void checkhitsprite_r(int i, int sn)
{
	int j, k, p;
	spritetype* s;

	i &= (MAXSPRITES - 1);

	if (isRRRA()) switch (sprite[i].picnum)
	{
	case RRTILE8464:
		break;
	case RRTILE8487:
	case RRTILE8489:
		spritesound(471, i);
		sprite[i].picnum++;
		break;
	case RRTILE7638:
	case RRTILE7644:
	case RRTILE7646:
	case RRTILE7650:
	case RRTILE7653:
	case RRTILE7655:
	case RRTILE7691:
	case RRTILE7876:
	case RRTILE7881:
	case RRTILE7883:
		sprite[i].picnum++;
		spritesound(VENT_BUST, i);
		break;
	case RRTILE7879:
		sprite[i].picnum++;
		spritesound(495, i);
		fi.hitradius(i, 10, 0, 0, 1, 1);
		break;
	case RRTILE7648:
	case RRTILE7694:
	case RRTILE7700:
	case RRTILE7702:
	case RRTILE7711:
		sprite[i].picnum++;
		spritesound(47, i);
		break;
	case RRTILE7636:
		sprite[i].picnum += 3;
		spritesound(VENT_BUST, i);
		break;
	case RRTILE7875:
		sprite[i].picnum += 3;
		spritesound(VENT_BUST, i);
		break;
	case RRTILE7640:
		sprite[i].picnum += 2;
		spritesound(VENT_BUST, i);
		break;
	case RRTILE7595:
	case RRTILE7704:
		sprite[i].picnum = RRTILE7705;
		spritesound(495, i);
		break;
	case RRTILE8579:
		sprite[i].picnum = RRTILE5014;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7441:
		sprite[i].picnum = RRTILE5016;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7534:
		sprite[i].picnum = RRTILE5029;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7545:
		sprite[i].picnum = RRTILE5030;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7547:
		sprite[i].picnum = RRTILE5031;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7574:
		sprite[i].picnum = RRTILE5032;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7575:
		sprite[i].picnum = RRTILE5033;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7578:
		sprite[i].picnum = RRTILE5034;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7478:
		sprite[i].picnum = RRTILE5035;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8525:
		sprite[i].picnum = RRTILE5036;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8537:
		sprite[i].picnum = RRTILE5062;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8215:
		sprite[i].picnum = RRTILE5064;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8216:
		sprite[i].picnum = RRTILE5065;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8217:
		sprite[i].picnum = RRTILE5066;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8218:
		sprite[i].picnum = RRTILE5067;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8220:
		sprite[i].picnum = RRTILE5068;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8221:
		sprite[i].picnum = RRTILE5069;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8312:
		sprite[i].picnum = RRTILE5071;
		spritesound(472, i);
		break;
	case RRTILE8395:
		sprite[i].picnum = RRTILE5072;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8423:
		sprite[i].picnum = RRTILE5073;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE3462:
		sprite[i].picnum = RRTILE5074;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case UWHIP:
		sprite[i].picnum = RRTILE5075;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8608:
		sprite[i].picnum = RRTILE5083;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8609:
		sprite[i].picnum = RRTILE5084;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8567:
	case RRTILE8568:
	case RRTILE8569:
	case RRTILE8570:
	case RRTILE8571:
		sprite[i].picnum = RRTILE5082;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8640:
		sprite[i].picnum = RRTILE5085;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8611:
		sprite[i].picnum = RRTILE5086;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case TECHLIGHTBUST2:
		sprite[i].picnum = TECHLIGHTBUST4;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8497:
		sprite[i].picnum = RRTILE5076;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8162:
	case RRTILE8163:
	case RRTILE8164:
	case RRTILE8165:
	case RRTILE8166:
	case RRTILE8167:
	case RRTILE8168:
		changespritestat(i, 5);
		sprite[i].picnum = RRTILE5063;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8589:
	case RRTILE8590:
	case RRTILE8591:
	case RRTILE8592:
	case RRTILE8593:
	case RRTILE8594:
	case RRTILE8595:
		changespritestat(i, 5);
		sprite[i].picnum = RRTILE8588;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE3497:
		sprite[i].picnum = RRTILE5076;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE3498:
		sprite[i].picnum = RRTILE5077;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE3499:
		sprite[i].picnum = RRTILE5078;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8503:
		sprite[i].picnum = RRTILE5079;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7901:
		sprite[i].picnum = RRTILE5080;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7696:
		sprite[i].picnum = RRTILE7697;
		spritesound(DUKE_SHUCKS, i);
		break;
	case RRTILE7806:
		sprite[i].picnum = RRTILE5043;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7885:
	case RRTILE7890:
		sprite[i].picnum = RRTILE5045;
		spritesound(495, i);
		fi.hitradius(i, 10, 0, 0, 1, 1);
		break;
	case RRTILE7886:
		sprite[i].picnum = RRTILE5046;
		spritesound(495, i);
		fi.hitradius(i, 10, 0, 0, 1, 1);
		break;
	case RRTILE7887:
		sprite[i].picnum = RRTILE5044;
		spritesound(GLASS_HEAVYBREAK, i);
		fi.hitradius(i, 10, 0, 0, 1, 1);
		break;
	case RRTILE7900:
		sprite[i].picnum = RRTILE5047;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7906:
		sprite[i].picnum = RRTILE5048;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7912:
	case RRTILE7913:
		sprite[i].picnum = RRTILE5049;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8047:
		sprite[i].picnum = RRTILE5050;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8596:
		sprite[i].picnum = RRTILE8598;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8059:
		sprite[i].picnum = RRTILE5051;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8060:
		sprite[i].picnum = RRTILE5052;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8222:
		sprite[i].picnum = RRTILE5053;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8223:
		sprite[i].picnum = RRTILE5054;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8224:
		sprite[i].picnum = RRTILE5055;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8370:
		sprite[i].picnum = RRTILE5056;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8371:
		sprite[i].picnum = RRTILE5057;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8372:
		sprite[i].picnum = RRTILE5058;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8373:
		sprite[i].picnum = RRTILE5059;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8396:
		sprite[i].picnum = RRTILE5038;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8397:
		sprite[i].picnum = RRTILE5039;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8398:
		sprite[i].picnum = RRTILE5040;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8399:
		sprite[i].picnum = RRTILE5041;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8385:
		sprite[i].picnum = RRTILE8386;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8387:
		sprite[i].picnum = RRTILE8388;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8389:
		sprite[i].picnum = RRTILE8390;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8391:
		sprite[i].picnum = RRTILE8392;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE7553:
		sprite[i].picnum = RRTILE5035;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8475:
		sprite[i].picnum = RRTILE5075;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8498:
		sprite[i].picnum = RRTILE5077;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8499:
		sprite[i].picnum = RRTILE5078;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE2445:
		sprite[i].picnum = RRTILE2450;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE2123:
		sprite[i].picnum = RRTILE2124;
		spritesound(GLASS_BREAKING, i);
		lotsofglass(i, -1, 10);
		break;
	case RRTILE3773:
		sprite[i].picnum = RRTILE8651;
		spritesound(GLASS_BREAKING, i);
		lotsofglass(i, -1, 10);
		break;
	case RRTILE7533:
		sprite[i].picnum = RRTILE5035;
		spritesound(495, i);
		fi.hitradius(i, 10, 0, 0, 1, 1);
		break;
	case RRTILE8394:
		sprite[i].picnum = RRTILE5072;
		spritesound(495, i);
		break;
	case RRTILE8461:
	case RRTILE8462:
		sprite[i].picnum = RRTILE5074;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8679:
		sprite[i].picnum = RRTILE8680;
		spritesound(DUKE_SHUCKS, i);
		fi.hitradius(i, 10, 0, 0, 1, 1);
		if (sprite[i].lotag != 0)
		{
			short j;
			for (j = 0; j < MAXSPRITES; j++)
			{
				if (sprite[j].picnum == RRTILE8679 && sprite[j].pal == 4)
				{
					if (sprite[j].lotag == sprite[i].lotag)
						sprite[j].picnum = RRTILE8680;
				}
			}
		}
		break;
	case RRTILE3584:
		sprite[i].picnum = RRTILE8681;
		spritesound(495, i);
		fi.hitradius(i, 250, 0, 0, 1, 1);
		break;
	case RRTILE8682:
		sprite[i].picnum = RRTILE8683;
		spritesound(GLASS_HEAVYBREAK, i);
		break;
	case RRTILE8099:
		if (sprite[i].lotag == 5)
		{
			short j;
			sprite[i].lotag = 0;
			sprite[i].picnum = RRTILE5087;
			spritesound(340, i);
			for (j = 0; j < MAXSPRITES; j++)
			{
				if (sprite[j].picnum == RRTILE8094)
					sprite[j].picnum = RRTILE5088;
			}
		}
		break;
	case RRTILE2431:
		if (sprite[i].pal != 4)
		{
			sprite[i].picnum = RRTILE2451;
			if (sprite[i].lotag != 0)
			{
				short j;
				for (j = 0; j < MAXSPRITES; j++)
				{
					if (sprite[j].picnum == RRTILE2431 && sprite[j].pal == 4)
					{
						if (sprite[i].lotag == sprite[j].lotag)
							sprite[j].picnum = RRTILE2451;
					}
				}
			}
		}
		break;
	case RRTILE2443:
		if (sprite[i].pal != 19)
			sprite[i].picnum = RRTILE2455;
		break;
	case RRTILE2455:
		spritesound(SQUISHED, i);
		fi.guts(&sprite[i], RRTILE2465, 3, myconnectindex);
		deletesprite(i);
		break;
	case RRTILE2451:
		if (sprite[i].pal != 4)
		{
			spritesound(SQUISHED, i);
			if (sprite[i].lotag != 0)
			{
				short j;
				for (j = 0; j < MAXSPRITES; j++)
				{
					if (sprite[j].picnum == RRTILE2451 && sprite[j].pal == 4)
					{
						if (sprite[i].lotag == sprite[j].lotag)
						{
							fi.guts(&sprite[i], RRTILE2460, 12, myconnectindex);
							fi.guts(&sprite[i], RRTILE2465, 3, myconnectindex);
							sprite[j].xrepeat = 0;
							sprite[j].yrepeat = 0;
							sprite[i].xrepeat = 0;
							sprite[i].yrepeat = 0;
						}
					}
				}
			}
			else
			{
				fi.guts(&sprite[i], RRTILE2460, 12, myconnectindex);
				fi.guts(&sprite[i], RRTILE2465, 3, myconnectindex);
				sprite[i].xrepeat = 0;
				sprite[i].yrepeat = 0;
			}
		}
		break;
	case RRTILE2437:
		spritesound(439, i);
		break;
	}

	switch (sprite[i].picnum)
	{
	case RRTILE3114:
		sprite[i].picnum = RRTILE3117;
		break;
	case RRTILE2876:
		sprite[i].picnum = RRTILE2990;
		break;
	case RRTILE3152:
		sprite[i].picnum = RRTILE3218;
		break;
	case RRTILE3153:
		sprite[i].picnum = RRTILE3219;
		break;
	case RRTILE2030:
		sprite[i].picnum = RRTILE2034;
		spritesound(GLASS_BREAKING, i);
		lotsofglass(i, -1, 10);
		break;
	case RRTILE2893:
	case RRTILE2915:
	case RRTILE3115:
	case RRTILE3171:
		switch (sprite[i].picnum)
		{
		case RRTILE2915:
			sprite[i].picnum = RRTILE2977;
			break;
		case RRTILE2893:
			sprite[i].picnum = RRTILE2978;
			break;
		case RRTILE3115:
			sprite[i].picnum = RRTILE3116;
			break;
		case RRTILE3171:
			sprite[i].picnum = RRTILE3216;
			break;
		}
		spritesound(GLASS_BREAKING, i);
		lotsofglass(i, -1, 10);
		break;
	case RRTILE2156:
	case RRTILE2158:
	case RRTILE2160:
	case RRTILE2175:
		sprite[i].picnum++;
		spritesound(GLASS_BREAKING, i);
		lotsofglass(i, -1, 10);
		break;
	case RRTILE2137:
	case RRTILE2151:
	case RRTILE2152:
		spritesound(GLASS_BREAKING, i);
		lotsofglass(i, -1, 10);
		sprite[i].picnum++;
		for (k = 0; k < 6; k++)
			EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (8 << 8), SCRAP6 + (krand() & 15), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (sprite[i].zvel >> 2), i, 5);
		break;
	case BOWLINGBALL:
		sprite[sn].xvel = (sprite[i].xvel >> 1) + (sprite[i].xvel >> 2);
		sprite[sn].ang -= (krand() & 16);
		spritesound(355, i);
		break;

	case STRIPEBALL:
	case QUEBALL:
	case RRTILE3440:
	case RRTILE3440 + 1:
	case HENSTAND:
	case HENSTAND + 1:
		if (sprite[sn].picnum == QUEBALL || sprite[sn].picnum == STRIPEBALL)
		{
			sprite[sn].xvel = (sprite[i].xvel >> 1) + (sprite[i].xvel >> 2);
			sprite[sn].ang -= (sprite[i].ang << 1) + 1024;
			sprite[i].ang = getangle(sprite[i].x - sprite[sn].x, sprite[i].y - sprite[sn].y) - 512;
			if (S_CheckSoundPlaying(POOLBALLHIT) < 2)
				spritesound(POOLBALLHIT, i);
		}
		else if (sprite[sn].picnum == RRTILE3440 || sprite[sn].picnum == RRTILE3440 + 1)
		{
			sprite[sn].xvel = (sprite[i].xvel >> 1) + (sprite[i].xvel >> 2);
			sprite[sn].ang -= ((sprite[i].ang << 1) + krand()) & 64;
			sprite[i].ang = (sprite[i].ang + krand()) & 16;
			spritesound(355, i);
		}
		else if (sprite[sn].picnum == HENSTAND || sprite[sn].picnum == HENSTAND + 1)
		{
			sprite[sn].xvel = (sprite[i].xvel >> 1) + (sprite[i].xvel >> 2);
			sprite[sn].ang -= ((sprite[i].ang << 1) + krand()) & 16;
			sprite[i].ang = (sprite[i].ang + krand()) & 16;
			spritesound(355, i);
		}
		else
		{
			if (krand() & 3)
			{
				sprite[i].xvel = 164;
				sprite[i].ang = sprite[sn].ang;
			}
		}
		break;

	case TREE1:
	case TREE2:
	case TIRE:
	case BOX:
		switch (sprite[sn].picnum)
		{
		case RPG2:
			if (!isRRRA()) break;
		case RADIUSEXPLOSION:
		case RPG:
		case FIRELASER:
		case HYDRENT:
		case HEAVYHBOMB:
		case TRIPBOMBSPRITE:
		case COOLEXPLOSION1:
		case OWHIP:
		case UWHIP:
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
		case RPG2:
			if (!isRRRA()) break;
		case RADIUSEXPLOSION:
		case RPG:
		case FIRELASER:
		case HYDRENT:
		case HEAVYHBOMB:
		case TRIPBOMBSPRITE:
		case COOLEXPLOSION1:
		case OWHIP:
		case UWHIP:
			for (k = 0; k < 64; k++)
			{
				j = EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (krand() % (48 << 8)), SCRAP6 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (sprite[i].zvel >> 2), i, 5);
				sprite[j].pal = 8;
			}

			if (sprite[i].picnum == CACTUS)
				sprite[i].picnum = CACTUSBROKE;
			sprite[i].cstat &= ~257;
			//	   else deletesprite(i);
			break;
		}
		break;


	case FANSPRITE:
		sprite[i].picnum = FANSPRITEBROKE;
		sprite[i].cstat &= (65535 - 257);
		spritesound(GLASS_HEAVYBREAK, i);
		s = &sprite[i];
		for (j = 0; j < 16; j++) RANDOMSCRAP(s, i);

		break;
	case WATERFOUNTAIN:
	case WATERFOUNTAIN + 1:
	case WATERFOUNTAIN + 2:
	case WATERFOUNTAIN + 3:
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
	case RRTILE1824:
		if (!isRRRA()) break;
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
	case RRTILE2654:
	case RRTILE2656:
	case RRTILE3172:
		if (!isRRRA()) break;
	case BOTTLE7:
		spritesound(GLASS_BREAKING, i);
		lotsofglass(i, -1, 10);
		deletesprite(i);
		break;
	case FORCESPHERE:
		sprite[i].xrepeat = 0;
		hittype[sprite[i].owner].temp_data[0] = 32;
		hittype[sprite[i].owner].temp_data[1] = !hittype[sprite[i].owner].temp_data[1];
		hittype[sprite[i].owner].temp_data[2] ++;
		fi.spawn(i, EXPLOSION2);
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

		if ((sprite[sn].picnum == SHRINKSPARK || sprite[sn].picnum == FREEZEBLAST || sprite[sn].owner != i) && sprite[i].statnum != 4)
		{
			if (badguy(&sprite[i]) == 1)
			{
				if (sprite[sn].picnum == RPG) sprite[sn].extra <<= 1;
				else if (isRRRA() && sprite[sn].picnum == RPG2) sprite[sn].extra <<= 1;

				if ((sprite[i].picnum != DRONE))
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

				if (j >= 0 && sprite[j].picnum == APLAYER && sprite[i].picnum != DRONE)
					if (ps[sprite[j].yvel].curr_weapon == SHOTGUN_WEAPON)
					{
						fi.shoot(i, BLOODSPLAT3);
						fi.shoot(i, BLOODSPLAT1);
						fi.shoot(i, BLOODSPLAT2);
						fi.shoot(i, BLOODSPLAT4);
					}

				if (sprite[i].statnum == 2)
				{
					changespritestat(i, 1);
					hittype[i].timetosleep = SLEEPTIME;
				}
			}

			if (sprite[i].statnum != 2)
			{
				if (sprite[sn].picnum == FREEZEBLAST && ((sprite[i].picnum == APLAYER && sprite[i].pal == 1) || (freezerhurtowner == 0 && sprite[sn].owner == i)))
					return;

				hittype[i].picnum = sprite[sn].picnum;
				hittype[i].extra += sprite[sn].extra;
				if (sprite[i].picnum != COW)
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

void checksectors_r(int snum)
{
	int i = -1, oldz;
	struct player_struct* p;
	int hitscanwall;
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
		if (!isRRRA() || !RRRA_ExitedLevel)
		{
			setnextmap(false);
			RRRA_ExitedLevel = 1;
		}
		return;
	case -2:
		sector[p->cursectnum].lotag = 0;
		p->timebeforeexit = 26 * 8;
		p->customexitsound = sector[p->cursectnum].hitag;
		return;
	default:
		if (sector[p->cursectnum].lotag >= 10000)
		{
			if (snum == screenpeek || ud.coop == 1)
				spritesound(sector[p->cursectnum].lotag - 10000, p->i);
			sector[p->cursectnum].lotag = 0;
		}
		break;

	}

	//After this point the the player effects the map with space

	if (p->gm & MODE_TYPE || sprite[p->i].extra <= 0) return;

	if (ud.cashman && PlayerInput(snum, SKB_OPEN))
		fi.lotsofmoney(&sprite[p->i], 2);


	if (!(PlayerInput(snum, SKB_OPEN)) && !PlayerInput(snum, SKB_ESCAPE))
		p->toggle_key_flag = 0;

	else if (!p->toggle_key_flag)
	{
		neartagsprite = -1;
		p->toggle_key_flag = 1;
		hitscanwall = -1;

		hitawall(p, &hitscanwall);

		if (isRRRA())
		{
			if (hitscanwall >= 0 && wall[hitscanwall].overpicnum == MIRROR && snum == screenpeek)
				if (numplayers == 1)
				{
					if (A_CheckSoundPlaying(p->i, 27) == 0 && A_CheckSoundPlaying(p->i, 28) == 0 && A_CheckSoundPlaying(p->i, 29) == 0
						&& A_CheckSoundPlaying(p->i, 257) == 0 && A_CheckSoundPlaying(p->i, 258) == 0)
					{
						short snd = krand() % 5;
						if (snd == 0)
							spritesound(27, p->i);
						else if (snd == 1)
							spritesound(28, p->i);
						else if (snd == 2)
							spritesound(29, p->i);
						else if (snd == 3)
							spritesound(257, p->i);
						else if (snd == 4)
							spritesound(258, p->i);
					}
					return;
				}
		}
		else
		{
			if (hitscanwall >= 0 && wall[hitscanwall].overpicnum == MIRROR)
		  		if (wall[hitscanwall].lotag > 0 && A_CheckSoundPlaying(p->i, wall[hitscanwall].lotag) == 0 && snum == screenpeek)
				{
					spritesound(wall[hitscanwall].lotag, p->i);
					return;
				}
		}

		if (hitscanwall >= 0 && (wall[hitscanwall].cstat & 16))
			if (wall[hitscanwall].lotag)
				return;

		if (isRRRA())
		{
			if (p->OnMotorcycle)
			{
				if (p->MotoSpeed < 20)
				{
					OffMotorcycle(p);
						return;
				}
				return;
			}
			if (p->OnBoat)
			{
				if (p->MotoSpeed < 20)
				{
					OffBoat(p);
					return;
				}
				return;
			}
			neartag(p->posx, p->posy, p->posz, sprite[p->i].sectnum, p->getoang(), &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1280L, 3);
		}

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
					case FEM10:
					case NAKED1:
					case STATUE:
					case TOUGHGAL:
						return;
					case COW:
						g_spriteExtra[neartagsprite] = 1; // TRANSITIONAL move to sprite or actor
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
			case RRTILE8448:
				if (!isRRRA()) return;
				if (!A_CheckSoundPlaying(neartagsprite, 340))
					A_PlaySound(340, neartagsprite);
				return;
			case RRTILE8704:
				if (!isRRRA()) return;
				if (numplayers == 1)
				{
					static bool alreadydone; // what is this supposed to do? Looks broken.
					// This is from RedneckGDX - the version in RR Reconstruction looked like broken nonsense.
					if (S_CheckSoundPlaying(neartagsprite, 445) || alreadydone != 0)
					{
						if (!S_CheckSoundPlaying(neartagsprite, 445) && !S_CheckSoundPlaying(neartagsprite, 446) && !S_CheckSoundPlaying(neartagsprite, 447) && alreadydone != 0)
						{
							if ((krand() % 2) == 1)
								spritesound(446, neartagsprite);
							else
								spritesound(447, neartagsprite);
						}
					}
					else
					{
						spritesound(445, neartagsprite);
						alreadydone = 1;
					}
				}
				return;
			case EMPTYBIKE:
				if (!isRRRA()) return;
				OnMotorcycle(p, neartagsprite);
				return;
			case EMPTYBOAT:
				if (!isRRRA()) return;
				OnBoat(p, neartagsprite);
				return;
			case RRTILE8164:
			case RRTILE8165:
			case RRTILE8166:
			case RRTILE8167:
			case RRTILE8168:
			case RRTILE8591:
			case RRTILE8592:
			case RRTILE8593:
			case RRTILE8594:
			case RRTILE8595:
				if (!isRRRA()) return;
				sprite[neartagsprite].extra = 60;
				spritesound(235, neartagsprite);
				return;

			case TOILET:
			case STALL:
			case RRTILE2121:
			case RRTILE2122:
				if (p->last_pissed_time == 0)
				{
					if (ud.lockout == 0) spritesound(435, p->i);

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
				else if (A_CheckSoundPlaying(p->i, DUKE_GRUNT) == 0)
					spritesound(DUKE_GRUNT, p->i);
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
			}
		}

		if (!PlayerInput(snum, SKB_OPEN)) return;

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
		}

		if (neartagsector >= 0 && (sector[neartagsector].lotag & 16384) == 0 && isanearoperator(sector[neartagsector].lotag))
		{
			short unk = 0;
			i = headspritesect[neartagsector];
			while (i >= 0)
			{
				if (sprite[i].picnum == ACTIVATOR || sprite[i].picnum == MASTERSWITCH)
					return;
				i = nextspritesect[i];
			}
			if (haskey(neartagsector, snum))
				operatesectors(neartagsector, p->i);
			else
			{
				if (g_spriteExtra[neartagsprite] > 3) // TRANSITIONAL move to sprite or actor
					spritesound(99, p->i);
				else
					spritesound(419, p->i);
				FTA(41, p);
			}
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
				if (haskey(neartagsector, snum))
					operatesectors(sprite[p->i].sectnum, p->i);
				else
				{
					if (g_spriteExtra[neartagsprite] > 3) // TRANSITIONAL move to sprite or actor
						spritesound(99, p->i);
					else
						spritesound(419, p->i);
					FTA(41, p);
				}
			}
			else fi.checkhitswitch(snum, neartagwall, 0);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void dofurniture(int wl, int sect, int snum)
{
	int startwall;
	int endwall;
	int i;
	int var_C;
	int x;
	int y;
	int min_x;
	int min_y;
	int max_x;
	int max_y;
	int ins;
	int var_cx;

	startwall = sector[wall[wl].nextsector].wallptr;;
	endwall = startwall + sector[wall[wl].nextsector].wallnum;
	var_C = 1;
	max_x = max_y = -0x20000;
	min_x = min_y = 0x20000;
	var_cx = sector[sect].hitag;
	if (var_cx > 16)
		var_cx = 16;
	else if (var_cx == 0)
		var_cx = 4;
	for (i = startwall; i < endwall; i++)
	{
		x = wall[i].x;
		y = wall[i].y;
		if (x > max_x)
			max_x = x;
		if (y > max_y)
			max_y = y;
		if (x < min_x)
			min_x = x;
		if (y < min_y)
			min_y = y;
	}
	max_x += var_cx + 1;
	max_y += var_cx + 1;
	min_x -= var_cx + 1;
	min_y -= var_cx + 1;
	ins = inside(max_x, max_y, sect);
	if (!ins)
		var_C = 0;
	ins = inside(max_x, min_y, sect);
	if (!ins)
		var_C = 0;
	ins = inside(min_x, min_y, sect);
	if (!ins)
		var_C = 0;
	ins = inside(min_x, max_y, sect);
	if (!ins)
		var_C = 0;
	if (var_C)
	{
		if (A_CheckSoundPlaying(ps[snum].i, 389) == 0)
			spritesound(389, ps[snum].i);
		for (i = startwall; i < endwall; i++)
		{
			x = wall[i].x;
			y = wall[i].y;
			switch (wall[wl].lotag)
			{
			case 42:
				y = wall[i].y + var_cx;
				dragpoint(i, x, y);
				break;
			case 41:
				x = wall[i].x - var_cx;
				dragpoint(i, x, y);
				break;
			case 40:
				y = wall[i].y - var_cx;
				dragpoint(i, x, y);
				break;
			case 43:
				x = wall[i].x + var_cx;
				dragpoint(i, x, y);
				break;
			}
		}
	}
	else
	{
		for (i = startwall; i < endwall; i++)
		{
			x = wall[i].x;
			y = wall[i].y;
			switch (wall[wl].lotag)
			{
			case 42:
				y = wall[i].y - (var_cx - 2);
				dragpoint(i, x, y);
				break;
			case 41:
				x = wall[i].x + (var_cx - 2);
				dragpoint(i, x, y);
				break;
			case 40:
				y = wall[i].y + (var_cx - 2);
				dragpoint(i, x, y);
				break;
			case 43:
				x = wall[i].x - (var_cx - 2);
				dragpoint(i, x, y);
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void tearitup(int sect)
{
	int j = headspritesect[sect];
	while (j != -1)
	{
		int nextj = nextspritesect[j];
		if (sprite[j].picnum == DESTRUCTO)
		{
			hittype[j].picnum = SHOTSPARK1;
			hittype[j].extra = 1;
		}
		j = nextj;
	}
}
END_DUKE_NS
