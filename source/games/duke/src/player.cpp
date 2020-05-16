//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)

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

BEGIN_DUKE_NS 

//---------------------------------------------------------------------------
//
// setpal
//
//---------------------------------------------------------------------------

void setpal_(struct player_struct* p) // cannot be activated yet.
{
	int palette;;
	if (p->DrugMode) palette = DRUGPAL;
	else if (p->heat_on || (sector[p->cursectnum].ceilingpicnum >= TILE_FLOORSLIME && sector[p->cursectnum].ceilingpicnum <= TILE_FLOORSLIME+2)) palette = SLIMEPAL;
	else if (sector[p->cursectnum].lotag == 2) palette = WATERPAL;
	else palette = BASEPAL;
	videoSetPalette(palette, 0);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void quickkill(struct player_struct* p)
{
	SetPlayerPal(p, PalEntry(48, 48, 48, 48));

	sprite[p->i].extra = 0;
	sprite[p->i].cstat |= 32768;
	if (ud.god == 0) fi.guts(&sprite[p->i], TILE_JIBS6, 8, myconnectindex);
	return;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void forceplayerangle(struct player_struct* p)
{
	int n;

	n = 128 - (krand() & 255);

	p->q16horiz += 64 << FRACBITS;
	p->return_to_center = 9;
	p->look_ang = n >> 1;
	p->rotscrnang = n >> 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void tracers(int x1, int y1, int z1, int x2, int y2, int z2, int n)
{
	int i, xv, yv, zv;
	short sect = -1;

	i = n + 1;
	xv = (x2 - x1) / i;
	yv = (y2 - y1) / i;
	zv = (z2 - z1) / i;

	if ((abs(x1 - x2) + abs(y1 - y2)) < 3084)
		return;

	for (i = n; i > 0; i--)
	{
		x1 += xv;
		y1 += yv;
		z1 += zv;
		updatesector(x1, y1, &sect);
		if (sect >= 0)
		{
			if (sector[sect].lotag == 2)
				EGS(sect, x1, y1, z1, TILE_WATERBUBBLE, -32, 4 + (krand() & 3), 4 + (krand() & 3), krand() & 2047, 0, 0, ps[0].i, 5);
			else
				EGS(sect, x1, y1, z1, TILE_SMALLSMOKE, -32, 14, 14, 0, 0, 0, ps[0].i, 5);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int hits(int i)
{
	auto sp = &sprite[i];
	int sx, sy, sz;
	short sect;
	short hw, hs;
	int zoff;

	if (sp->picnum == TILE_APLAYER) zoff = (40 << 8);
	else zoff = 0;

	hitscan(sp->x, sp->y, sp->z - zoff, sp->sectnum, sintable[(sp->ang + 512) & 2047], sintable[sp->ang & 2047], 0, &sect, &hw, &hs, &sx, &sy, &sz, CLIPMASK1);

	return (FindDistance2D(sx - sp->x, sy - sp->y));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int hitasprite(int i, short* hitsp)
{
	auto sp = &sprite[i];
	int sx, sy, sz, zoff;
	short sect, hw;

	if (badguy(&sprite[i]))
		zoff = (42 << 8);
	else if (sp->picnum == TILE_APLAYER) zoff = (39 << 8);
	else zoff = 0;

	hitscan(sp->x, sp->y, sp->z - zoff, sp->sectnum, sintable[(sp->ang + 512) & 2047], sintable[sp->ang & 2047], 0, &sect, &hw, hitsp, &sx, &sy, &sz, CLIPMASK1);

	if (hw >= 0 && (wall[hw].cstat & 16) && badguy(&sprite[i]))
		return((1 << 30));

	return (FindDistance2D(sx - sp->x, sy - sp->y));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int hitawall(struct player_struct* p, int* hitw)
{
	int sx, sy, sz;
	short sect, hs, hitw1;

	hitscan(p->posx, p->posy, p->posz, p->cursectnum,
		sintable[(p->getang() + 512) & 2047], sintable[p->getang() & 2047], 0, &sect, &hitw1, &hs, &sx, &sy, &sz, CLIPMASK0);
	*hitw = hitw1;

	return (FindDistance2D(sx - p->posx, sy - p->posy));
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int aim(spritetype* s, int aang)
{
	char gotshrinker, gotfreezer;
	int i, j, a, k, cans;
	int aimstats[] = { 10,13,1,2 };
	int dx1, dy1, dx2, dy2, dx3, dy3, smax, sdist;
	int xv, yv;

	a = s->ang;

	// Autoaim from DukeGDX.
	if (s->picnum == TILE_APLAYER && ps[s->yvel].auto_aim == 0)
		return -1;

	j = -1;
	//    if(s->picnum == TILE_APLAYER && ps[s->yvel].aim_mode) return -1;

	if (isRR())
	{
		gotshrinker = 0;
		gotfreezer = 0;
	}
	else if (isWW2GI())
	{
		gotshrinker = s->picnum == TILE_APLAYER && aplWeaponWorksLike[ps[s->yvel].curr_weapon][s->yvel] == SHRINKER_WEAPON;
		gotfreezer = s->picnum == TILE_APLAYER && aplWeaponWorksLike[ps[s->yvel].curr_weapon][s->yvel] == FREEZE_WEAPON;
	}
	else
	{
		gotshrinker = s->picnum == TILE_APLAYER && ps[s->yvel].curr_weapon == SHRINKER_WEAPON;
		gotfreezer = s->picnum == TILE_APLAYER && ps[s->yvel].curr_weapon == FREEZE_WEAPON;
	}

	smax = 0x7fffffff;

	dx1 = sintable[(a + 512 - aang) & 2047];
	dy1 = sintable[(a - aang) & 2047];
	dx2 = sintable[(a + 512 + aang) & 2047];
	dy2 = sintable[(a + aang) & 2047];

	dx3 = sintable[(a + 512) & 2047];
	dy3 = sintable[a & 2047];

	for (k = 0; k < 4; k++)
	{
		if (j >= 0)
			break;
		for (i = headspritestat[aimstats[k]]; i >= 0; i = nextspritestat[i])
			if (sprite[i].xrepeat > 0 && sprite[i].extra >= 0 && (sprite[i].cstat & (257 + 32768)) == 257)
				if (badguy(&sprite[i]) || k < 2)
				{
					auto sp = &sprite[i];
					if (badguy(&sprite[i]) || sp->picnum == TILE_APLAYER)
					{
						if (sp->picnum == TILE_APLAYER &&
							(isRR() && ud.ffire == 0) &&
							ud.coop == 1 &&
							s->picnum == TILE_APLAYER &&
							s != &sprite[i])
							continue;

						if (gotshrinker && sprite[i].xrepeat < 30 && !(actorinfo[sp->picnum].flags & SFLAG_SHRINKAUTOAIM)) continue;
						if (gotfreezer && sprite[i].pal == 1) continue;
					}

					xv = (sp->x - s->x);
					yv = (sp->y - s->y);

					if ((dy1 * xv) <= (dx1 * yv))
						if ((dy2 * xv) >= (dx2 * yv))
						{
							sdist = mulscale(dx3, xv, 14) + mulscale(dy3, yv, 14);
							if (sdist > 512 && sdist < smax)
							{
								if (s->picnum == TILE_APLAYER)
									a = (abs(scale(sp->z - s->z, 10, sdist) - (ps[s->yvel].gethorizdiff() - 100)) < 100);
								else a = 1;

								cans = cansee(sp->x, sp->y, sp->z - (32 << 8) + actorinfo[sp->picnum].aimoffset, sp->sectnum, s->x, s->y, s->z - (32 << 8), s->sectnum);

								if (a && cans)
								{
									smax = sdist;
									j = i;
								}
							}
						}
				}
	}

	return j;
}



END_DUKE_NS
