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
#include "actors.h"
#include "names_rr.h"

BEGIN_DUKE_NS

short pinsectorresetdown(short sect);


void ballreturn(short spr)
{
	short j, i, nexti, nextj;
	i = headspritestat[105];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		if (sprite[i].picnum == RRTILE281)
			if (sprite[spr].sectnum == sprite[i].sectnum)
		{
			j = headspritestat[105];
			while (j >= 0)
			{
				nextj = nextspritestat[j];
				if (sprite[j].picnum == RRTILE282)
					if (sprite[i].hitag == sprite[j].hitag)
					spawn(j, BOWLINGBALLSPRITE);
				if (sprite[j].picnum == RRTILE280)
					if (sprite[i].hitag == sprite[j].hitag)
						if (sprite[j].lotag == 0)
				{
					sprite[j].lotag = 100;
					sprite[j].extra++;
					pinsectorresetdown(sprite[j].sectnum);
				}
				j = nextj;
			}
		}

		i = nexti;
	}
}

short pinsectorresetdown(short sect)
{
	int vel, j;
	
	j = getanimationgoal(&sector[sect].ceilingz);

	if (j == -1)
	{
		j = sector[sect].floorz;
		vel = 64;
		setanimation(sect,&sector[sect].ceilingz,j,vel);
		return 1;
	}
	return 0;
}

short pinsectorresetup(short sect)
{
	int vel, j;
	
	j = getanimationgoal(&sector[sect].ceilingz);

	if (j == -1)
	{
		j = sector[nextsectorneighborz(sect,sector[sect].ceilingz,-1,-1)].ceilingz;
		vel = 64;
		setanimation(sect,&sector[sect].ceilingz,j,vel);
		return 1;
	}
	return 0;
}

short checkpins(short sect)
{
	short i, pin;
	int  x, y;
	short pins[10];
	short nexti, tag;
	
	pin = 0;
	for(i=0;i<10;i++) pins[i] = 0;

	i = headspritesect[sect];

	while (i >= 0)
	{
		nexti = nextspritesect[i];

		if (sprite[i].picnum == RRTILE3440)
		{
			pin++;
			pins[sprite[i].lotag] = 1;
		}
		if (sprite[i].picnum == RRTILE280)
		{
			tag = sprite[i].hitag;
		}

		i = nexti;
	}

	if (tag)
	{
		tag += 2024;
		tileCopySection(2024,0,0,128,64,tag,0,0);
		for(i=0;i<10;i++)
		{
			if (pins[i] == 1)
			{
				switch (i)
				{
					case 0:
						x = 64;
						y = 48;
						break;
					case 1:
						x = 56;
						y = 40;
						break;
					case 2:
						x = 72;
						y = 40;
						break;
					case 3:
						x = 48;
						y = 32;
						break;
					case 4:
						x = 64;
						y = 32;
						break;
					case 5:
						x = 80;
						y = 32;
						break;
					case 6:
						x = 40;
						y = 24;
						break;
					case 7:
						x = 56;
						y = 24;
						break;
					case 8:
						x = 72;
						y = 24;
						break;
					case 9:
						x = 88;
						y = 24;
						break;
				}
				tileCopySection(2023,0,0,8,8,tag,x-4,y-10);
			}
		}
	}

	return pin;
}

void resetpins(short sect)
{
	short i, j, nexti, tag;
	int x, y;
	i = headspritesect[sect];
	while (i >= 0)
	{
		nexti = headspritesect[i];
		if (sprite[i].picnum == 3440)
			deletesprite(i);
		i = nexti;
	}
	i = headspritesect[sect];
	while (i >= 0)
	{
		nexti = nextspritesect[i];
		if (sprite[i].picnum == 283)
		{
			j = spawn(i,3440);
			sprite[j].lotag = sprite[i].lotag;
			if (sprite[j].lotag == 3 || sprite[j].lotag == 5)
			{
				sprite[j].clipdist = (1+(krand()%1))*16+32;
			}
			else
			{
				sprite[j].clipdist = (1+(krand()%1))*16+32;
			}
			sprite[j].ang -= ((krand()&32)-(krand()&64))&2047;
		}
		if (sprite[i].picnum == 280)
			tag = sprite[i].hitag;
		i = nexti;
	}
	if (tag)
	{
		tag += LANEPICS+1;
		tileCopySection(LANEPICS+1,0,0,128,64,tag,0,0);
		for(i=0;i<10;i++)
		{
			switch (i)
			{
				case 0:
					x = 64;
					y = 48;
					break;
				case 1:
					x = 56;
					y = 40;
					break;
				case 2:
					x = 72;
					y = 40;
					break;
				case 3:
					x = 48;
					y = 32;
					break;
				case 4:
					x = 64;
					y = 32;
					break;
				case 5:
					x = 80;
					y = 32;
					break;
				case 6:
					x = 40;
					y = 24;
					break;
				case 7:
					x = 56;
					y = 24;
					break;
				case 8:
					x = 72;
					y = 24;
					break;
				case 9:
					x = 88;
					y = 24;
					break;
			}
			tileCopySection(LANEPICS,0,0,8,8,tag,x-4,y-10);
		}
	}
}

void resetlanepics(void)
{
	int x, y;
	short i;
	short tag, pic;
	for(tag=0;tag<4;tag++)
	{
		pic = tag + 1;
		if (pic == 0) continue;
		pic += LANEPICS+1;
		tileCopySection(LANEPICS+1,0,0,128,64, pic,0,0);
		for(i=0;i<10;i++)
		{
			switch (i)
			{
				case 0:
					x = 64;
					y = 48;
					break;
				case 1:
					x = 56;
					y = 40;
					break;
				case 2:
					x = 72;
					y = 40;
					break;
				case 3:
					x = 48;
					y = 32;
					break;
				case 4:
					x = 64;
					y = 32;
					break;
				case 5:
					x = 80;
					y = 32;
					break;
				case 6:
					x = 40;
					y = 24;
					break;
				case 7:
					x = 56;
					y = 24;
					break;
				case 8:
					x = 72;
					y = 24;
					break;
				case 9:
					x = 88;
					y = 24;
					break;
			}
			tileCopySection(LANEPICS,0,0,8,8,pic,x-4,y-10);
		}
	}
}

END_DUKE_NS

