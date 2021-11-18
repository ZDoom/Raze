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
#include "savegamehelp.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

static int torchcnt;
static int jaildoorcnt;
static int minecartcnt;
static int lightnincnt;

static sectortype* torchsector[64];
static short torchsectorshade[64];
static short torchtype[64];

static short jaildoorsound[32];
static int jaildoordrag[32];
static int jaildoorspeed[32];
static short jaildoorsecthtag[32];
static int jaildoordist[32];
static short jaildoordir[32];
static short jaildooropen[32];
static sectortype* jaildoorsect[32];

static short minecartdir[16];
static int minecartspeed[16];
static sectortype* minecartchildsect[16];
static short minecartsound[16];
static int minecartdist[16];
static int minecartdrag[16];
static short minecartopen[16];
static sectortype* minecartsect[16];

static sectortype* lightninsector[64];
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

void addtorch(spritetype* s)
{
	if (torchcnt >= 64)
		I_Error("Too many torch effects");

	torchsector[torchcnt] = s->sector();
	torchsectorshade[torchcnt] = s->sector()->floorshade;
	torchtype[torchcnt] = s->lotag;
	torchcnt++;
}

void addlightning(spritetype* s)
{
	if (lightnincnt >= 64)
		I_Error("Too many lightnin effects");

	lightninsector[lightnincnt] = s->sector();
	lightninsectorshade[lightnincnt] = s->sector()->floorshade;
	lightnincnt++;
}

void addjaildoor(int p1, int p2, int iht, int jlt, int p3, sectortype* j)
{
	if (jaildoorcnt >= 32)
		I_Error("Too many jaildoor sectors");

	if (jlt != 10 && jlt != 20 && jlt != 30 && jlt != 40)
	{
		Printf(PRINT_HIGH, "Bad direction %d for jail door with tag %d\n", jlt, iht);
		return;	// wouldn't work so let's skip it.
	}

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

void addminecart(int p1, int p2, sectortype* i, int iht, int p3, sectortype* childsectnum)
{
	if (minecartcnt >= 16)
		I_Error("Too many minecart sectors");
	minecartdist[minecartcnt] = p1;
	minecartspeed[minecartcnt] = p2;
	minecartsect[minecartcnt] = i;
	minecartdir[minecartcnt] = i->hitag;
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
	uint8_t shade;
	ds = krand()&8;
	for (int i = 0; i < torchcnt; i++)
	{
		auto sect = torchsector[i];
		shade = torchsectorshade[i] - ds;
		switch (torchtype[i])
		{
			case 0:
				sect->floorshade = shade;
				sect->ceilingshade = shade;
				break;
			case 1:
				sect->ceilingshade = shade;
				break;
			case 2:
				sect->floorshade = shade;
				break;
			case 4:
				sect->ceilingshade = shade;
				break;
			case 5:
				sect->floorshade = shade;
				break;
		}
		for (auto& wal : wallsofsector(sect))
		{
			if (wal.lotag != 1)
			{
				switch (torchtype[i])
				{
					case 0:
						wal.shade = shade;
						break;
					case 1:
						wal.shade = shade;
						break;
					case 2:
						wal.shade = shade;
						break;
					case 3:
						wal.shade = shade;
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
	for (int i = 0; i < jaildoorcnt; i++)
	{
		int speed;
		auto sectp = jaildoorsect[i];
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
				for (auto& wal : wallsofsector(sectp))
				{
					int x = wal.x;
					int y = wal.y;
					switch (jaildoordir[i])
					{
						case 10:
							y += speed;
							break;
						case 20:
							x -= speed;
							break;
						case 30:
							y -= speed;
							break;
						case 40:
							x += speed;
							break;
					}
					dragpoint(&wal, x, y);
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
				for (auto& wal : wallsofsector(sectp))
				{
					int x, y;
					switch (jaildoordir[i])
					{
						default: // make case of bad parameters well defined.
							x = wal.x;
							y = wal.y;
							break;
						case 10:
							x = wal.x;
							y = wal.y + speed;
							break;
						case 20:
							x = wal.x - speed;
							y = wal.y;
							break;
						case 30:
							x = wal.x;
							y = wal.y - speed;
							break;
						case 40:
							x = wal.x + speed;
							y = wal.y;
							break;
					}
					dragpoint(&wal, x, y);
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
	int i;
	int speed;
	int y;
	int x;
	int cx;
	int cy;
	int max_x;
	int min_y;
	int max_y;
	int min_x;
	for (i = 0; i < minecartcnt; i++)
	{
		auto sectp = minecartsect[i];
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
				for (auto& wal : wallsofsector(sectp))
				{
					switch (minecartdir[i])
					{
						default: // make case of bad parameters well defined.
							x = wal.x;
							y = wal.y;
							break;
						case 10:
							x = wal.x;
							y = wal.y + speed;
							break;
						case 20:
							x = wal.x - speed;
							y = wal.y;
							break;
						case 30:
							x = wal.x;
							y = wal.y - speed;
							break;
						case 40:
							x = wal.x + speed;
							y = wal.y;
							break;
					}
					dragpoint(&wal, x, y);
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
				for (auto& wal : wallsofsector(sectp))
				{
					switch (minecartdir[i])
					{
						default: // make case of bad parameters well defined.
							x = wal.x;
							y = wal.y;
							break;
						case 10:
							x = wal.x;
							y = wal.y + speed;
							break;
						case 20:
							x = wal.x - speed;
							y = wal.y;
							break;
						case 30:
							x = wal.x;
							y = wal.y - speed;
							break;
						case 40:
							x = wal.x + speed;
							y = wal.y;
							break;
					}
					dragpoint(&wal, x, y);
				}
			}
		}
		auto csect = minecartchildsect[i];
		max_x = max_y = -0x20000;
		min_x = min_y = 0x20000;
		for (auto& wal : wallsofsector(csect))
		{
			x = wal.x;
			y = wal.y;
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
		DukeSectIterator it(csect);
		while (auto a2 = it.Next())
		{
			auto sj = a2->s;
			if (badguy(sj))
				setsprite(a2, cx, cy, sj->z);
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
					S_PlayActorSound(jaildoorsound[i], ps[screenpeek].GetActor());
			}
			if (jaildooropen[i] == 2)
			{
				jaildooropen[i] = 3;
				jaildoordrag[i] = jaildoordist[i];
				if (!isRRRA() || jaildoorsound[i] != 0)
					S_PlayActorSound(jaildoorsound[i], ps[screenpeek].GetActor());
			}
		}
	}
}

void thunder(void)
{
	struct player_struct* p;
	int r1, r2;
	int i = 0;
	uint8_t shade;

	p = &ps[screenpeek];

	if (!thunderflash)
	{
		if (testgotpic(RRTILE2577, true))
		{
			g_visibility = 256; // this is an engine variable
			if (krand() > 65000)
			{
				thunderflash = 1;
				thundertime = 256;
				S_PlaySound(351 + (rand() % 3));
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
			thunder_brightness = brightness;
			g_visibility = p->visibility;
		}
	}
	if (!winderflash)
	{
		if (testgotpic(RRTILE2562, true))
		{
			if (krand() > 65000)
			{
				winderflash = 1;
				windertime = 128;
				S_PlaySound(351 + (rand() % 3));
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
				auto sectp = lightninsector[i];
				sectp->floorshade = (int8_t)lightninsectorshade[i];
				sectp->ceilingshade = (int8_t)lightninsectorshade[i];
				for (auto& wal : wallsofsector(sectp))
					wal.shade = (int8_t)lightninsectorshade[i];
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
		thunder_brightness = brightness;
	}
	if (winderflash == 1)
	{
		r2 = krand() & 8;
		shade = torchsectorshade[i] + r2;
		for (i = 0; i < lightnincnt; i++)
		{
			auto sectp = lightninsector[i];
			sectp->floorshade = lightninsectorshade[i] - shade;
			sectp->ceilingshade = lightninsectorshade[i] - shade;
			for (auto& wal : wallsofsector(sectp))
				wal.shade = lightninsectorshade[i] - shade;
		}
	}
}

END_DUKE_NS
