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
#include "mapinfo.h"

BEGIN_DUKE_NS 

void addjaildoor(int p1, int p2, int iht, int jlt, int p3, int h);
void addminecart(int p1, int p2, int i, int iht, int p3, int childsectnum);
void addtorch(int i);
void addlightning(int i);


static inline void tloadtile(int tilenum, int palnum = 0)
{
	markTileForPrecache(tilenum, palnum);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void cachespritenum(short i)
{
	char maxc;
	short j;
	int pal = sprite[i].pal;

	if (ud.monsters_off && badguy(&sprite[i])) return;

	maxc = 1;

	switch (sprite[i].picnum)
	{
	case HYDRENT:
		tloadtile(BROKEFIREHYDRENT);
		for (j = TOILETWATER; j < (TOILETWATER + 4); j++)
			tloadtile(j, pal);
		break;
	case RRTILE2121:
	case RRTILE2122:
		tloadtile(sprite[i].picnum, pal);
		break;
	case TOILET:
		tloadtile(TOILETBROKE);
		for (j = TOILETWATER; j < (TOILETWATER + 4); j++)
			tloadtile(j, pal);
		break;
	case STALL:
		tloadtile(STALLBROKE);
		for (j = TOILETWATER; j < (TOILETWATER + 4); j++)
			tloadtile(j, pal);
		break;
	case FORCERIPPLE:
		maxc = 9;
		break;
	case RUBBERCAN:
		maxc = 2;
		break;
	case TOILETWATER:
		maxc = 4;
		break;
	case BUBBASTAND:
		for (j = BUBBASCRATCH; j <= (BUBBASCRATCH + 47); j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case SBSWIPE:
		if (isRRRA())
			for (j = SBSWIPE; j <= (SBSWIPE + 29); j++)
				tloadtile(j, pal);
		maxc = 0;
		break;

	case COOT:
		for (j = COOT; j <= (COOT + 217); j++)
			tloadtile(j, pal);
		for (j = COOTJIBA; j < COOTJIBC + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case LTH:
		maxc = 105;
		for (j = LTH; j < (LTH + maxc); j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case BILLYRAY:
		maxc = 144;
		for (j = BILLYWALK; j < (BILLYWALK + maxc); j++)
			tloadtile(j, pal);
		for (j = BILLYJIBA; j <= BILLYJIBB + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case COW:
		maxc = 56;
		for (j = sprite[i].picnum; j < (sprite[i].picnum + maxc); j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case DOGRUN:
		for (j = DOGATTACK; j <= DOGATTACK + 35; j++)
			tloadtile(j, pal);
		for (j = DOGRUN; j <= DOGRUN + 80; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case RABBIT:
		if (isRRRA())
		{
			for (j = RABBIT; j <= RABBIT + 54; j++)
				tloadtile(j, pal);
			for (j = RABBIT + 56; j <= RABBIT + 56 + 49; j++)
				tloadtile(j, pal);
			for (j = RABBIT + 56; j <= RABBIT + 56 + 49; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case BIKERB:
	case BIKERBV2:
		if (isRRRA())
		{
			for (j = BIKERB; j <= BIKERB + 104; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case BIKER:
		if (isRRRA())
		{
			for (j = BIKER; j <= BIKER + 116; j++)
				tloadtile(j, pal);
			for (j = BIKER + 150; j <= BIKER + 150 + 104; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case CHEER:
		if (isRRRA())
		{
			for (j = CHEER; j <= CHEER + 44; j++)
				tloadtile(j, pal);
			for (j = CHEER + 47; j <= CHEER + 47 + 211; j++)
				tloadtile(j, pal);
			for (j = CHEER + 262; j <= CHEER + 262 + 72; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case CHEERB:
		if (isRRRA())
		{
			for (j = CHEERB; j <= CHEERB + 83; j++)
				tloadtile(j, pal);
			for (j = CHEERB + 157; j <= CHEERB + 157 + 83; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case MAMA:
		if (isRRRA())
		{
			for (j = MAMA; j <= MAMA + 78; j++)
				tloadtile(j, pal);
			for (j = MAMA + 80; j <= MAMA + 80 + 7; j++)
				tloadtile(j, pal);
			for (j = MAMA + 90; j <= MAMA + 90 + 94; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case CHEERBOAT:
		if (isRRRA())
		{
			tloadtile(CHEERBOAT);
			maxc = 0;
		}
		break;
	case HULKBOAT:
		if (isRRRA())
		{
			tloadtile(HULKBOAT);
			maxc = 0;
		}
		break;
	case MINIONBOAT:
		if (isRRRA())
		{
			tloadtile(MINIONBOAT);
			maxc = 0;
		}
		break;
	case BILLYPLAY:
		if (isRRRA())
		{
			for (j = BILLYPLAY; j <= BILLYPLAY + 2; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case COOTPLAY:
		if (isRRRA())
		{
			for (j = COOTPLAY; j <= COOTPLAY + 4; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case PIG:
	case PIGSTAYPUT:
		maxc = 68;
		break;
	case TORNADO:
		maxc = 7;
		break;
	case HEN:
	case HENSTAND:
		maxc = 34;
		break;
	case APLAYER:
		maxc = 0;
		if (ud.multimode > 1)
		{
			maxc = 5;
			for (j = APLAYER; j < APLAYER + 220; j++)
				tloadtile(j, pal);
			for (j = DUKEGUN; j < DUKELEG + 4; j++)
				tloadtile(j, pal);
		}
		break;
	case ATOMICHEALTH:
		maxc = 14;
		break;
	case DRONE:
		maxc = 6;
		break;
	case EXPLODINGBARREL:
	case SEENINE:
	case OOZFILTER:
		maxc = 3;
		break;
	case NUKEBARREL:
	case CAMERA1:
		maxc = 5;
		break;
	case VIXEN:
		maxc = 214;
		for (j = sprite[i].picnum; j < sprite[i].picnum + maxc; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case SBMOVE:
		if (!isRRRA())
		{

			maxc = 54;
			for (j = sprite[i].picnum; j < sprite[i].picnum + maxc; j++)
				tloadtile(j, pal);
			maxc = 100;
			for (j = SBMOVE; j < SBMOVE + maxc; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case HULK:
		maxc = 40;
		for (j = sprite[i].picnum - 41; j < sprite[i].picnum + maxc - 41; j++)
			tloadtile(j, pal);
		for (j = HULKJIBA; j <= HULKJIBC + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case MINION:
		maxc = 141;
		for (j = sprite[i].picnum; j < sprite[i].picnum + maxc; j++)
			tloadtile(j, pal);
		for (j = MINJIBA; j <= MINJIBC + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;


	}

	for (j = sprite[i].picnum; j < (sprite[i].picnum + maxc); j++)
		tloadtile(j, pal);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void cachegoodsprites(void)
{
	short i;

	tloadtile(BOTTOMSTATUSBAR);
	if (ud.multimode > 1)
	{
		tloadtile(FRAGBAR);
	}

	//tloadtile(VIEWSCREEN);

	for (i = FOOTPRINTS; i < FOOTPRINTS + 3; i++)
		tloadtile(i);

	for (i = BURNING; i < BURNING + 14; i++)
		tloadtile(i);

	for (i = FIRSTGUN; i < FIRSTGUN + 10; i++)
		tloadtile(i);

	for (i = EXPLOSION2; i < EXPLOSION2 + 21; i++)
		tloadtile(i);

	tloadtile(BULLETHOLE);

	for (i = SHOTGUN; i < SHOTGUN + 8; i++)
		tloadtile(i);

	tloadtile(FOOTPRINTS);

	for (i = JIBS1; i < (JIBS5 + 5); i++)
		tloadtile(i);

	for (i = SCRAP1; i < (SCRAP1 + 19); i++)
		tloadtile(i);

	for (i = SMALLSMOKE; i < (SMALLSMOKE + 4); i++)
		tloadtile(i);

	if (isRRRA() && currentLevel->levelNumber == levelnum(0, 4))
	{
		tloadtile(RRTILE2577);
	}
	if (!isRRRA() && currentLevel->levelNumber == levelnum(1, 2))
	{
		tloadtile(RRTILE3190);
		tloadtile(RRTILE3191);
		tloadtile(RRTILE3192);
		tloadtile(RRTILE3144);
		tloadtile(RRTILE3139);
		tloadtile(RRTILE3132);
		tloadtile(RRTILE3120);
		tloadtile(RRTILE3121);
		tloadtile(RRTILE3122);
		tloadtile(RRTILE3123);
		tloadtile(RRTILE3124);
	}
	if (lastlevel)
	{
		i = isRRRA() ? UFO1_RRRA : UFO1_RR;
		tloadtile(i);
		i = UFO2;
		tloadtile(i);
		i = UFO3;
		tloadtile(i);
		i = UFO4;
		tloadtile(i);
		i = UFO5;
		tloadtile(i);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void cacheit_r(void)
{
	short i,j;

	cachegoodsprites();

	for(i=0;i<numwalls;i++)
	{
			tloadtile(wall[i].picnum, wall[i].pal);
		if(wall[i].overpicnum >= 0)
			tloadtile(wall[i].overpicnum, wall[i].pal);
	}

	for (i = 0; i < numsectors; i++)
	{
		tloadtile(sector[i].floorpicnum, sector[i].floorpal);
		tloadtile(sector[i].ceilingpicnum, sector[i].ceilingpal);
		if (sector[i].ceilingpicnum == LA)
		{
			tloadtile(LA + 1);
			tloadtile(LA + 2);
		}
	}

	j = headspritesect[i];
	while(j >= 0)
	{
		if(sprite[j].xrepeat != 0 && sprite[j].yrepeat != 0 && (sprite[j].cstat&32768) == 0)
				cachespritenum(j);
		j = nextspritesect[j];
	}
	precacheMarkedTiles();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void prelevel_r(int g)
{
	struct player_struct* p;
	short i;
	short nexti;
	short j;
	short startwall;
	short endwall;
	short lotaglist;
	short k;
	short lotags[65];
	int speed;
	int dist;
	short sound;
	sound = 0;

	prelevel_common(g);
	p = &ps[screenpeek];


	if (isRRRA())
	{
		if (currentLevel->levelNumber == levelnum(1, 4))
			ps[myconnectindex].steroids_amount = 0;

		for (j = 0; j < MAXSPRITES; j++)
		{
			if (sprite[j].pal == 100)
			{
				if (numplayers > 1)
					deletesprite(j);
				else
					sprite[j].pal = 0;
			}
			else if (sprite[j].pal == 101)
			{
				sprite[j].extra = 0;
				sprite[j].hitag = 1;
				sprite[j].pal = 0;
				changespritestat(j, 118);
			}
		}
	}

	for (i = 0; i < numsectors; i++)
	{
		if (sector[i].ceilingpicnum == RRTILE2577)
			thunderon = 1;

		switch (sector[i].lotag)
		{
		case 41:
			k = headspritesect[i];
			while (k != -1)
			{
				nexti = nextspritesect[k];
				if (sprite[k].picnum == RRTILE11)
				{
					dist = sprite[k].lotag << 4;
					speed = sprite[k].hitag;
					deletesprite(k);
				}
				if (sprite[k].picnum == RRTILE38)
				{
					sound = sprite[k].lotag;
					deletesprite(k);
				}
				k = nexti;
			}
			for (j = 0; j < numsectors; j++)
			{
				if (sector[i].hitag == sector[j].hitag && j != i)
				{
					addjaildoor(dist, speed, sector[i].hitag, sector[j].lotag, sound, j);
				}
			}
			break;
		case 42:
		{
			short ii;
			int childsectnum = -1;
			k = headspritesect[i];
			while (k != -1)
			{
				nexti = nextspritesect[k];
				if (sprite[k].picnum == RRTILE64)
				{
					dist = sprite[k].lotag << 4;
					speed = sprite[k].hitag;
					for (ii = 0; ii < MAXSPRITES; ii++)
					{
						if (sprite[ii].picnum == RRTILE66)
							if (sprite[ii].lotag == sprite[k].sectnum)
							{
								childsectnum = sprite[ii].sectnum;
								deletesprite(ii);
							}
					}
					deletesprite(k);
				}
				if (sprite[k].picnum == RRTILE65)
				{
					sound = sprite[k].lotag;
					deletesprite(k);
				}
				k = nexti;
			}
			addminecart(dist, speed, i, sector[i].hitag, sound, childsectnum);
			break;
		}
		}
	}

	i = headspritestat[0];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		LoadActor(i, -1, -1);

		if (sprite[i].lotag == -1 && (sprite[i].cstat & 16))
		{
			ps[0].exitx = sprite[i].x;
			ps[0].exity = sprite[i].y;
		}
		else switch (sprite[i].picnum)
		{
		case NUKEBUTTON:
			chickenplant = 1;
			break;

		case GPSPEED:
			sector[sprite[i].sectnum].extra = sprite[i].lotag;
			deletesprite(i);
			break;

		case CYCLER:
			if (numcyclers >= MAXCYCLERS)
				I_Error("Too many cycling sectors.");
			cyclers[numcyclers][0] = sprite[i].sectnum;
			cyclers[numcyclers][1] = sprite[i].lotag;
			cyclers[numcyclers][2] = sprite[i].shade;
			cyclers[numcyclers][3] = sector[sprite[i].sectnum].floorshade;
			cyclers[numcyclers][4] = sprite[i].hitag;
			cyclers[numcyclers][5] = (sprite[i].ang == 1536);
			numcyclers++;
			deletesprite(i);
			break;

		case RRTILE18:
			addtorch(i);
			deletesprite(i);
			break;

		case RRTILE35:
			addlightning(i);
			deletesprite(i);
			break;

		case RRTILE68:
			shadedsector[sprite[i].sectnum] = 1;
			deletesprite(i);
			break;

		case RRTILE67:
			sprite[i].cstat |= 32768;
			break;

		case SOUNDFX:
			if (ambientfx >= 64)
				I_Error("Too many ambient effects");
			else
			{
				ambienthitag[ambientfx] = sprite[i].hitag;
				ambientlotag[ambientfx] = sprite[i].lotag;
				sprite[i].ang = ambientfx;
				ambientfx++;
				sprite[i].lotag = 0;
				sprite[i].hitag = 0;
			}
			break;
		}
		i = nexti;
	}

	for (i = 0; i < MAXSPRITES; i++)
	{
		if (sprite[i].picnum == RRTILE19)
		{
			if (geocnt > 64)
				I_Error("Too many geometry effects");
			if (sprite[i].hitag == 0)
			{
				geosector[geocnt] = sprite[i].sectnum;
				for (j = 0; j < MAXSPRITES; j++)
				{
					if (sprite[i].lotag == sprite[j].lotag && j != i && sprite[j].picnum == RRTILE19)
					{
						if (sprite[j].hitag == 1)
						{
							geosectorwarp[geocnt] = sprite[j].sectnum;
							geox[geocnt] = sprite[i].x - sprite[j].x;
							geoy[geocnt] = sprite[i].y - sprite[j].y;
							//geoz[geocnt] = sprite[i].z - sprite[j].z;
						}
						if (sprite[j].hitag == 2)
						{
							geosectorwarp2[geocnt] = sprite[j].sectnum;
							geox2[geocnt] = sprite[i].x - sprite[j].x;
							geoy2[geocnt] = sprite[i].y - sprite[j].y;
							//geoz2[geocnt] = sprite[i].z - sprite[j].z;
						}
					}
				}
				geocnt++;
			}
		}
	}

	for (i = 0; i < MAXSPRITES; i++)
	{
		if (sprite[i].statnum < MAXSTATUS)
		{
			if (sprite[i].picnum == SECTOREFFECTOR && sprite[i].lotag == 14)
				continue;
			fi.spawn(-1, i);
		}
	}

	for (i = 0; i < MAXSPRITES; i++)
	{
		if (sprite[i].statnum < MAXSTATUS)
		{
			if (sprite[i].picnum == SECTOREFFECTOR && sprite[i].lotag == 14)
				fi.spawn(-1, i);
		}
		if (sprite[i].picnum == RRTILE19)
			deletesprite(i);
		if (sprite[i].picnum == RRTILE34)
		{
			sectorextra[sprite[i].sectnum] = sprite[i].lotag;
			deletesprite(i);
		}
	}

	lotaglist = 0;

	i = headspritestat[0];
	while (i >= 0)
	{
		switch (sprite[i].picnum)
		{
		case RRTILE8464 + 1:
			if (!isRRRA()) break;
		case DIPSWITCH + 1:
		case DIPSWITCH2 + 1:
		case PULLSWITCH + 1:
		case HANDSWITCH + 1:
		case SLOTDOOR + 1:
		case LIGHTSWITCH + 1:
		case SPACELIGHTSWITCH + 1:
		case SPACEDOORSWITCH + 1:
		case FRANKENSTINESWITCH + 1:
		case LIGHTSWITCH2 + 1:
		case POWERSWITCH1 + 1:
		case LOCKSWITCH1 + 1:
		case POWERSWITCH2 + 1:
		case NUKEBUTTON:
		case NUKEBUTTON + 1:

			for (j = 0; j < lotaglist; j++)
				if (sprite[i].lotag == lotags[j])
					break;

			if (j == lotaglist)
			{
				lotags[lotaglist] = sprite[i].lotag;
				lotaglist++;
				if (lotaglist > 64)
					I_Error("Too many switches (64 max).");

				j = headspritestat[3];
				while (j >= 0)
				{
					if (sprite[j].lotag == 12 && sprite[j].hitag == sprite[i].lotag)
						hittype[j].temp_data[0] = 1;
					j = nextspritestat[j];
				}
			}
			break;
		}
		i = nextspritestat[i];
	}

	mirrorcnt = 0;

	for (i = 0; i < numwalls; i++)
	{
		walltype* wal;
		wal = &wall[i];

		if (wal->overpicnum == MIRROR && (wal->cstat & 32) != 0)
		{
			j = wal->nextsector;

			if (mirrorcnt > 63)
				I_Error("Too many mirrors (64 max.)");
			if ((j >= 0) && sector[j].ceilingpicnum != MIRROR)
			{
				sector[j].ceilingpicnum = MIRROR;
				sector[j].floorpicnum = MIRROR;
				mirrorwall[mirrorcnt] = i;
				mirrorsector[mirrorcnt] = j;
				mirrorcnt++;
				continue;
			}
		}

		if (numanimwalls >= MAXANIMWALLS)
			I_Error("Too many 'anim' walls (max 512.)");

		animwall[numanimwalls].tag = 0;
		animwall[numanimwalls].wallnum = 0;

		switch (wal->overpicnum)
		{
		case FANSPRITE:
			wall->cstat |= 65;
			animwall[numanimwalls].wallnum = i;
			numanimwalls++;
			break;
		case BIGFORCE:
			animwall[numanimwalls].wallnum = i;
			numanimwalls++;
			continue;
		}

		wal->extra = -1;

		switch (wal->picnum)
		{
		case SCREENBREAK6:
		case SCREENBREAK7:
		case SCREENBREAK8:
			for (j = SCREENBREAK6; j <= SCREENBREAK8; j++)
				tloadtile(j);
			animwall[numanimwalls].wallnum = i;
			animwall[numanimwalls].tag = -1;
			numanimwalls++;
			break;
		}
	}

	//Invalidate textures in sector behind mirror
	for (i = 0; i < mirrorcnt; i++)
	{
		startwall = sector[mirrorsector[i]].wallptr;
		endwall = startwall + sector[mirrorsector[i]].wallnum;
		for (j = startwall; j < endwall; j++)
		{
			wall[j].picnum = MIRROR;
			wall[j].overpicnum = MIRROR;
		}
	}
	thunder_brightness = 0;
	if (!thunderon)
	{
		g_visibility = p->visibility;
	}
 }


END_DUKE_NS
