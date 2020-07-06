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
aint with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "global.h"
#include "names_r.h"
#include "serializer.h"

BEGIN_DUKE_NS

static int torchcnt;
static int jaildoorcnt;
static int minecartcnt;
static int lightnincnt;

static short torchsector[64];
static short torchsectorshade[64];
static short torchtype[64];

static short jaildoorsound[32];
static int jaildoordrag[32];
static int jaildoorspeed[32];
static short jaildoorsecthtag[32];
static int jaildoordist[32];
static short jaildoordir[32];
static short jaildooropen[32];
static short jaildoorsect[32];

static short minecartdir[16];
static int minecartspeed[16];
static short minecartchildsect[16];
static short minecartsound[16];
static int minecartdist[16];
static int minecartdrag[16];
static short minecartopen[16];
static short minecartsect[16];

static short lightninsector[64];
static short lightninsectorshade[64];

static uint8_t brightness;

static int thunderflash;
static int thundertime;
static int winderflash;
static int windertime;


void lava_cleararrays()
{
	jaildoorcnt = 0;
	minecartcnt = 0;
	torchcnt = 0;
	lightnincnt = 0;
}

void lava_serialize(FSerializer& arc)
{
	arc("torchcnt", torchcnt)
		("jaildoorcnt", jaildoorcnt)
		("minecartcnt", minecartcnt)
		("lightnincnt", lightnincnt);

	if (torchcnt)
		arc.Array("torchsector", torchsector, torchcnt)
		.Array("torchsectorshade", torchsectorshade, torchcnt)
		.Array("torchtype", torchtype, torchcnt);

	if (jaildoorcnt)
		arc.Array("jaildoorsound", jaildoorsound, jaildoorcnt)
		.Array("jaildoordrag", jaildoordrag, jaildoorcnt)
		.Array("jaildoorspeed", jaildoorspeed, jaildoorcnt)
		.Array("jaildoorsecthtag", jaildoorsecthtag, jaildoorcnt)
		.Array("jaildoordist", jaildoordist, jaildoorcnt)
		.Array("jaildoordir", jaildoordir, jaildoorcnt)
		.Array("jaildooropen", jaildooropen, jaildoorcnt)
		.Array("jaildoorsect", jaildoorsect, jaildoorcnt);

	if (minecartcnt)
		arc.Array("minecartdir", minecartdir, minecartcnt)
		.Array("minecartspeed", minecartspeed, minecartcnt)
		.Array("minecartchildsect", minecartchildsect, minecartcnt)
		.Array("minecartsound", minecartsound, minecartcnt)
		.Array("minecartdist", minecartdist, minecartcnt)
		.Array("minecartdrag", minecartdrag, minecartcnt)
		.Array("minecartopen", minecartopen, minecartcnt)
		.Array("minecartsect", minecartsect, minecartcnt);

	if (lightnincnt)
		arc.Array("lightninsector", lightninsector, lightnincnt)
		.Array("lightninsectorshade", lightninsectorshade, lightnincnt);

	arc("brightness", brightness)
		("thunderflash", thunderflash)
		("thundertime", thundertime)
		("winderflash", winderflash)
		("windertime", windertime);
}

void addtorch(int i)
{
	if (torchcnt >= 64)
		I_Error("Too many torch effects");

	torchsector[torchcnt] = sprite[i].sectnum;
	torchsectorshade[torchcnt] = sector[sprite[i].sectnum].floorshade;
	torchtype[torchcnt] = sprite[i].lotag;
	torchcnt++;
}

void addlightning(int i)
{
	if (lightnincnt >= 64)
		I_Error("Too many lightnin effects");

	lightninsector[lightnincnt] = sprite[i].sectnum;
	lightninsectorshade[lightnincnt] = sector[sprite[i].sectnum].floorshade;
	lightnincnt++;
}

void addjaildoor(int p1, int p2, int iht, int jlt, int p3, int j)
{
	if (jaildoorcnt >= 32)
		I_Error("Too many jaildoor sectors");

	jaildoordist[jaildoorcnt] = p1;
	jaildoorspeed[jaildoorcnt] = p2;
	jaildoorsecthtag[jaildoorcnt] = iht;
	jaildoorsect[jaildoorcnt] = j;
	jaildoordrag[jaildoorcnt] = 0;
	jaildooropen[jaildoorcnt] = 0;
	jaildoordir[jaildoorcnt] = jlt;
	jaildoorsound[jaildoorcnt] = p3;
	jaildoorcnt++;
}

void addminecart(int p1, int p2, int i, int iht, int p3, int childsectnum)
{
	if (minecartcnt >= 16)
		I_Error("Too many minecart sectors");
	minecartdist[minecartcnt] = p1;
	minecartspeed[minecartcnt] = p2;
	minecartsect[minecartcnt] = i;
	minecartdir[minecartcnt] = sector[i].hitag;
	minecartdrag[minecartcnt] = p1;
	minecartopen[minecartcnt] = 1;
	minecartsound[minecartcnt] = p3;
	minecartchildsect[minecartcnt] = childsectnum;
	minecartcnt++;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void dotorch(void)
{
	int ds;
	short j;
	short startwall, endwall;
	char shade;
	ds = krand()&8;
	for (int i = 0; i < torchcnt; i++)
	{
		shade = torchsectorshade[i] - ds;
		switch (torchtype[i])
		{
			case 0:
				sector[torchsector[i]].floorshade = shade;
				sector[torchsector[i]].ceilingshade = shade;
				break;
			case 1:
				sector[torchsector[i]].ceilingshade = shade;
				break;
			case 2:
				sector[torchsector[i]].floorshade = shade;
				break;
			case 4:
				sector[torchsector[i]].ceilingshade = shade;
				break;
			case 5:
				sector[torchsector[i]].floorshade = shade;
				break;
		}
		startwall = sector[torchsector[i]].wallptr;
		endwall = startwall + sector[torchsector[i]].wallnum;
		for (j = startwall; j < endwall; j++)
		{
			if (wall[j].lotag != 1)
			{
				switch (torchtype[i])
				{
					case 0:
						wall[j].shade = shade;
						break;
					case 1:
						wall[j].shade = shade;
						break;
					case 2:
						wall[j].shade = shade;
						break;
					case 3:
						wall[j].shade = shade;
						break;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void dojaildoor(void)
{
	int j;
	int startwall, endwall;
	int x, y;
	int speed;
	for (int i = 0; i < jaildoorcnt; i++)
	{
		if (numplayers > 2)
			speed = jaildoorspeed[i];
		else
			speed = jaildoorspeed[i];
		if (speed < 2)
			speed = 2;
		if (jaildooropen[i] == 1)
		{
			jaildoordrag[i] -= speed;
			if (jaildoordrag[i] <= 0)
			{
				jaildoordrag[i] = 0;
				jaildooropen[i] = 2;
				switch (jaildoordir[i])
				{
					case 10:
						jaildoordir[i] = 30;
						break;
					case 20:
						jaildoordir[i] = 40;
						break;
					case 30:
						jaildoordir[i] = 10;
						break;
					case 40:
						jaildoordir[i] = 20;
						break;
				}
			}
			else
			{
				startwall = sector[jaildoorsect[i]].wallptr;
				endwall = startwall + sector[jaildoorsect[i]].wallnum;
				for (j = startwall; j < endwall; j++)
				{
					switch (jaildoordir[i])
					{
						case 10:
							x = wall[j].x;
							y = wall[j].y + speed;
							break;
						case 20:
							x = wall[j].x - speed;
							y = wall[j].y;
							break;
						case 30:
							x = wall[j].x;
							y = wall[j].y - speed;
							break;
						case 40:
							x = wall[j].x + speed;
							y = wall[j].y;
							break;
					}
					dragpoint(j,x,y);
				}
			}
		}
		if (jaildooropen[i] == 3)
		{
			jaildoordrag[i] -= speed;
			if (jaildoordrag[i] <= 0)
			{
				jaildoordrag[i] = 0;
				jaildooropen[i] = 0;
				switch (jaildoordir[i])
				{
					case 10:
						jaildoordir[i] = 30;
						break;
					case 20:
						jaildoordir[i] = 40;
						break;
					case 30:
						jaildoordir[i] = 10;
						break;
					case 40:
						jaildoordir[i] = 20;
						break;
				}
			}
			else
			{
				startwall = sector[jaildoorsect[i]].wallptr;
				endwall = startwall + sector[jaildoorsect[i]].wallnum;
				for (j = startwall; j < endwall; j++)
				{
					switch (jaildoordir[i])
					{
						case 10:
							x = wall[j].x;
							y = wall[j].y + speed;
							break;
						case 20:
							x = wall[j].x - speed;
							y = wall[j].y;
							break;
						case 30:
							x = wall[j].x;
							y = wall[j].y - speed;
							break;
						case 40:
							x = wall[j].x + speed;
							y = wall[j].y;
							break;
					}
					dragpoint(j,x,y);
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveminecart(void)
{
	short i;
	short j;
	short csect;
	short startwall;
	short endwall;
	int speed;
	int y;
	int x;
	short nextj;
	int cx;
	int cy;
	int max_x;
	int min_y;
	int max_y;
	int min_x;
	for (i = 0; i < minecartcnt; i++)
	{
		speed = minecartspeed[i];
		if (speed < 2)
			speed = 2;

		if (minecartopen[i] == 1)
		{
			minecartdrag[i] -= speed;
			if (minecartdrag[i] <= 0)
			{
				minecartdrag[i] = minecartdist[i];
				minecartopen[i] = 2;
				switch (minecartdir[i])
				{
					case 10:
						minecartdir[i] = 30;
						break;
					case 20:
						minecartdir[i] = 40;
						break;
					case 30:
						minecartdir[i] = 10;
						break;
					case 40:
						minecartdir[i] = 20;
						break;
				}
			}
			else
			{
				startwall = sector[minecartsect[i]].wallptr;
				endwall = startwall + sector[minecartsect[i]].wallnum;
				for (j = startwall; j < endwall; j++)
				{
					switch (minecartdir[i])
					{
						case 10:
							x = wall[j].x;
							y = wall[j].y + speed;
							break;
						case 20:
							x = wall[j].x - speed;
							y = wall[j].y;
							break;
						case 30:
							x = wall[j].x;
							y = wall[j].y - speed;
							break;
						case 40:
							x = wall[j].x + speed;
							y = wall[j].y;
							break;
					}
					dragpoint(j,x,y);
				}
			}
		}
		if (minecartopen[i] == 2)
		{
			minecartdrag[i] -= speed;
			if (minecartdrag[i] <= 0)
			{
				minecartdrag[i] = minecartdist[i];
				minecartopen[i] = 1;
				switch (minecartdir[i])
				{
					case 10:
						minecartdir[i] = 30;
						break;
					case 20:
						minecartdir[i] = 40;
						break;
					case 30:
						minecartdir[i] = 10;
						break;
					case 40:
						minecartdir[i] = 20;
						break;
				}
			}
			else
			{
				startwall = sector[minecartsect[i]].wallptr;
				endwall = startwall + sector[minecartsect[i]].wallnum;
				for (j = startwall; j < endwall; j++)
				{
					switch (minecartdir[i])
					{
						case 10:
							x = wall[j].x;
							y = wall[j].y + speed;
							break;
						case 20:
							x = wall[j].x - speed;
							y = wall[j].y;
							break;
						case 30:
							x = wall[j].x;
							y = wall[j].y - speed;
							break;
						case 40:
							x = wall[j].x + speed;
							y = wall[j].y;
							break;
					}
					dragpoint(j,x,y);
				}
			}
		}
		csect = minecartchildsect[i];
		startwall = sector[csect].wallptr;
		endwall = startwall + sector[csect].wallnum;
		max_x = max_y = -0x20000;
		min_x = min_y = 0x20000;
		for (j = startwall; j < endwall; j++)
		{
			x = wall[j].x;
			y = wall[j].y;
			if (x > max_x)
				max_x = x;
			if (y > max_y)
				max_y = y;
			if (x < min_x)
				min_x = x;
			if (y < min_y)
				min_y = y;
		}
		cx = (max_x + min_x) >> 1;
		cy = (max_y + min_y) >> 1;
		j = headspritesect[csect];
		while (j != -1)
		{
			nextj = nextspritesect[j];
			if (badguy(&sprite[j]))
				setsprite(j,cx,cy,sprite[j].z);
			j = nextj;
		}
	}
}

void operatejaildoors(int hitag)
{
	for (int i = 0; i < jaildoorcnt; i++)
	{
		if (jaildoorsecthtag[i] == hitag)
		{
			if (jaildooropen[i] == 0)
			{
				jaildooropen[i] = 1;
				jaildoordrag[i] = jaildoordist[i];
				if (!isRRRA() || jaildoorsound[i] != 0)
					spritesound(jaildoorsound[i], ps[screenpeek].i);
			}
			if (jaildooropen[i] == 2)
			{
				jaildooropen[i] = 3;
				jaildoordrag[i] = jaildoordist[i];
				if (!isRRRA() || jaildoorsound[i] != 0)
					spritesound(jaildoorsound[i], ps[screenpeek].i);
			}
		}
	}
}

void thunder(void)
{
	struct player_struct* p;
	int r1, r2;
	short startwall, endwall, i, j;
	unsigned char shade;

	p = &ps[screenpeek];

	if (!thunderflash)
	{
		if ((gotpic[RRTILE2577 >> 3] & (1 << (RRTILE2577 & 7))) > 0)
		{
			gotpic[RRTILE2577 >> 3] &= ~(1 << (RRTILE2577 & 7));
			g_visibility = 256; // this is an engine variable
			if (krand() > 65000)
			{
				thunderflash = 1;
				thundertime = 256;
				sound(351 + (rand() % 3));
			}
		}
		else
		{
			g_visibility = p->visibility;
			brightness = ud.brightness >> 2;
		}
	}
	else
	{
		thundertime -= 4;
		if (thundertime < 0)
		{
			thunderflash = 0;
			brightness = ud.brightness >> 2;
			videoSetBrightness(brightness);
			g_visibility = p->visibility;
		}
	}
	if (!winderflash)
	{
		if ((gotpic[RRTILE2562 >> 3] & (1 << (RRTILE2562 & 7))) > 0)
		{
			gotpic[RRTILE2562 >> 3] &= ~(1 << (RRTILE2562 & 7));
			if (krand() > 65000)
			{
				winderflash = 1;
				windertime = 128;
				sound(351 + (rand() % 3));
			}
		}
	}
	else
	{
		windertime -= 4;
		if (windertime < 0)
		{
			winderflash = 0;
			for (i = 0; i < lightnincnt; i++)
			{
				startwall = sector[lightninsector[i]].wallptr;
				endwall = startwall + sector[lightninsector[i]].wallnum;
				sector[lightninsector[i]].floorshade = lightninsectorshade[i];
				sector[lightninsector[i]].ceilingshade = lightninsectorshade[i];
				for (j = startwall; j < endwall; j++)
					wall[j].shade = lightninsectorshade[i];
			}
		}
	}
	if (thunderflash == 1)
	{
		r1 = krand() & 4;
		brightness += r1;
		switch (r1)
		{
		case 0:
			g_visibility = 2048;
			break;
		case 1:
			g_visibility = 1024;
			break;
		case 2:
			g_visibility = 512;
			break;
		case 3:
			g_visibility = 256;
			break;
		default:
			g_visibility = 4096;
			break;
		}
		if (brightness > 8)
			brightness = 0;
		videoSetBrightness(brightness);
	}
	if (winderflash == 1)
	{
		r2 = krand() & 8;
		shade = torchsectorshade[i] + r2;
		for (i = 0; i < lightnincnt; i++)
		{
			startwall = sector[lightninsector[i]].wallptr;
			endwall = startwall + sector[lightninsector[i]].wallnum;
			sector[lightninsector[i]].floorshade = lightninsectorshade[i] - shade;
			sector[lightninsector[i]].ceilingshade = lightninsectorshade[i] - shade;
			for (j = startwall; j < endwall; j++)
				wall[j].shade = lightninsectorshade[i] - shade;
		}
	}
}

END_DUKE_NS
