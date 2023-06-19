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
#include "serializer.h"
#include "savegamehelp.h"
#include "dukeactor.h"
#include "interpolate.h"

#include "buildtiles.h"

BEGIN_DUKE_NS

static int torchcnt;

static sectortype* torchsector[64];
static short torchsectorshade[64];
static short torchtype[64];

struct jaildoor
{
	sectortype* sect;
	double dist;
	double drag;
	int speed;
	int16_t direction;
	int16_t sound;
	int16_t open;
	int16_t hitag;
};

struct minecart
{
	sectortype* sect;
	sectortype* childsect;
	double dist;
	double drag;
	int speed;
	int16_t direction;
	int16_t sound;
	int16_t open;
};

struct WindowLightning
{
	sectortype* sect;
	int shade;
};
static TArray<jaildoor> jaildoors;
static TArray<minecart> minecarts;
static TArray<WindowLightning> windowlightning;
static TArray<sectortype*> thundersectors;

static uint8_t brightness;

static int thunderflash;
static int thundertime;
static int windowflash;
static int windowtime;


void lava_cleararrays()
{
	jaildoors.Clear();
	minecarts.Clear();
	windowlightning.Clear();
	thundersectors.Clear();
	torchcnt = 0;
}

FSerializer& Serialize(FSerializer& arc, const char* key, jaildoor& c, jaildoor* def)
{
	if (arc.BeginObject(key))
	{
		arc("sect", c.sect)
			("hitag", c.hitag)
			("speed", c.speed)
			("dist", c.dist)
			("drag", c.drag)
			("dir", c.direction)
			("sound", c.sound)
			("open", c.open)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* key, minecart& c, minecart* def)
{
	if (arc.BeginObject(key))
	{
		arc("sect", c.sect)
			("childsect", c.childsect)
			("speed", c.speed)
			("dist", c.dist)
			("drag", c.drag)
			("dir", c.direction)
			("sound", c.sound)
			("open", c.open)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* key, WindowLightning& c, WindowLightning* def)
{
	if (arc.BeginObject(key))
	{
		arc("sect", c.sect)
			("shade", c.shade)
			.EndObject();
	}
	return arc;
}

void lava_serialize(FSerializer& arc)
{
	arc("torchcnt", torchcnt)
		("jaildoors", jaildoors)
		("minecarts", minecarts)
		("thundersectors", thundersectors)
		("windowlightning", windowlightning);

	if (torchcnt)
		arc.Array("torchsector", torchsector, torchcnt)
		.Array("torchsectorshade", torchsectorshade, torchcnt)
		.Array("torchtype", torchtype, torchcnt);

	arc("brightness", brightness)
		("thunderflash", thunderflash)
		("thundertime", thundertime)
		("winderflash", windowflash)
		("windertime", windowtime);
}

void addthundersector(sectortype* sect)
{
	thundersectors.Push(sect);
}

void addtorch(sectortype* sect, int shade, int lotag)
{
	if (torchcnt >= 64)
		I_Error("Too many torch effects");

	torchsector[torchcnt] = sect;
	torchsectorshade[torchcnt] = shade;
	torchtype[torchcnt] = lotag;
	torchcnt++;
}

void addlightning(sectortype* sect, int shade)
{
	windowlightning.Push({ sect, shade });
}

void addjaildoor(int p1, int p2, int iht, int jlt, int p3, sectortype* j)
{
	if (jlt != 10 && jlt != 20 && jlt != 30 && jlt != 40)
	{
		Printf(PRINT_HIGH, "Bad direction %d for jail door with tag %d\n", jlt, iht);
		return;	// wouldn't work so let's skip it.
	}

	jaildoors.Reserve(1);
	auto& jd = jaildoors.Last();

	jd.dist = p1;
	jd.speed = p2;
	jd.hitag = iht;
	jd.sect = j;
	jd.drag = 0;
	jd.open = 0;
	jd.direction = jlt;
	jd.sound = p3;
	setsectinterpolate(j);
}

void addminecart(int p1, int p2, sectortype* i, int iht, int p3, sectortype* childsectnum)
{
	minecarts.Reserve(1);
	auto& mc = minecarts.Last();
	mc.dist = p1;
	mc.speed = p2;
	mc.sect = i;
	mc.direction = i->hitag;
	mc.drag = p1;
	mc.open = 1;
	mc.sound = p3;
	mc.childsect = childsectnum;
	setsectinterpolate(i);
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
		for (auto& wal : sect->walls)
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
	for(auto& jd : jaildoors)
	{
		auto sectp = jd.sect;
		if (!sectp) continue; // this is only for allowing old, broken savegames to work, this would crash otherwise.
		double speed = max(2, jd.speed) * maptoworld;

		if (jd.open == 1 || jd.open == 3)
		{
			jd.drag -= speed;
			if (jd.drag <= 0)
			{
				jd.drag = 0;
				jd.open ^= 3;
				switch (jd.direction)
				{
					case 10:
						jd.direction = 30;
						break;
					case 20:
						jd.direction = 40;
						break;
					case 30:
						jd.direction = 10;
						break;
					case 40:
						jd.direction = 20;
						break;
				}
			}
			else
			{
				for (auto& wal : sectp->walls)
				{
					DVector2 vec = wal.pos;
					switch (jd.direction)
					{
						case 10:
							vec.Y += speed;
							break;
						case 20:
							vec.X -= speed;
							break;
						case 30:
							vec.Y -= speed;
							break;
						case 40:
							vec.X += speed;
							break;
					}
					dragpoint(&wal, vec);
				}
			}
		}
	}
}

void operatejaildoors(int hitag)
{
	for (auto& jd : jaildoors)
	{
		if (jd.hitag == hitag)
		{
			if (jd.open == 0)
			{
				jd.open = 1;
				jd.drag = jd.dist;
				if (!isRRRA() || jd.sound != 0)
					S_PlayActorSound(jd.sound, ps[screenpeek].GetActor());
			}
			if (jd.open == 2)
			{
				jd.open = 3;
				jd.drag = jd.dist;
				if (!isRRRA() || jd.sound != 0)
					S_PlayActorSound(jd.sound, ps[screenpeek].GetActor());
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
	for(auto& mc : minecarts)
	{
		auto sectp = mc.sect;
		double speed = max(2, mc.speed) * maptoworld;

		if (mc.open == 1 || mc.open == 2)
		{
			mc.drag -= speed;
			if (mc.drag <= 0)
			{
				mc.drag = mc.dist;
				mc.open ^= 3;
				switch (mc.direction)
				{
					case 10:
						mc.direction = 30;
						break;
					case 20:
						mc.direction = 40;
						break;
					case 30:
						mc.direction = 10;
						break;
					case 40:
						mc.direction = 20;
						break;
				}
			}
			else
			{
				for (auto& wal : sectp->walls)
				{
					auto pos = wal.pos;
					switch (mc.direction)
					{
						default: // make case of bad parameters well defined.
							break;
						case 10:
							pos.Y += speed;
							break;
						case 20:
							pos.X -= speed;
							break;
						case 30:
							pos.Y -= speed;
							break;
						case 40:
							pos.X += speed;
							break;
					}
					dragpoint(&wal, pos);
				}
			}
		}

		auto csect = mc.childsect;
		double max_x = INT32_MIN, max_y = INT32_MIN, min_x = INT32_MAX, min_y = INT32_MAX;
		for (auto& wal : csect->walls)
		{
			double x = wal.pos.X;
			double y = wal.pos.Y;
			if (x > max_x)
				max_x = x;
			if (y > max_y)
				max_y = y;
			if (x < min_x)
				min_x = x;
			if (y < min_y)
				min_y = y;
		}
		double cx = (max_x + min_x) * 0.5;
		double cy = (max_y + min_y) * 0.5;
		DukeSectIterator it(csect);
		while (auto a2 = it.Next())
		{
			if (badguy(a2))
				SetActor(a2, DVector3(cx, cy, a2->spr.pos.Z));
		}
	}
}

void thunder(void)
{
	player_struct* p;
	int r1, r2;
	int i = 0;
	uint8_t shade;

	p = &ps[screenpeek];

	if (!thunderflash)
	{
		bool seen = false;
		for (auto& sectp : thundersectors)
		{
			if (sectp->exflags & SECTOREX_SEEN)
			{
				seen = true;
				sectp->exflags &= ~SECTOREX_SEEN;
			}
		}

		if (seen)
		{
			if (ps[screenpeek].actor->sector()->ceilingstat & CSTAT_SECTOR_SKY)
			{
				g_relvisibility = 0;
				if (krand() > 65000)
				{
					thunderflash = 1;
					thundertime = 256;
					S_PlaySound(soundEngine->FindSound("THUNDER"));
				}
			}
			else
			{
				brightness = ud.brightness >> 2;
			}
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
		}
	}

	if (!windowflash)
	{
		bool seen = false;
		for (auto& window : windowlightning)
		{
			auto sectp = window.sect;
			if (sectp->exflags & SECTOREX_SEEN)
			{
				seen = true;
				sectp->exflags &= ~SECTOREX_SEEN;
			}
		}

		if (seen)
		{
			if (krand() > 65000)
			{
				windowflash = 1;
				windowtime = 128;
				S_PlaySound(soundEngine->FindSound("THUNDER"));
			}
		}
	}
	else
	{
		windowtime -= 4;
		if (windowtime < 0)
		{
			windowflash = 0;
			for (auto& window : windowlightning)
			{
				auto sectp = window.sect;
				sectp->floorshade = 
				sectp->ceilingshade = (int8_t)window.shade;
				for (auto& wal : sectp->walls)
					wal.shade = (int8_t)window.shade;
			}
		}
	}
	if (thunderflash == 1)
	{
		r1 = krand() & 4;
		brightness += r1;
		if (brightness > 8)
			brightness = 0;
		thunder_brightness = brightness;
	}
	if (windowflash == 1)
	{
		r2 = krand() & 8;
		shade = torchsectorshade[i] + r2;
		for (auto& window : windowlightning)
		{
			auto sectp = window.sect;
			sectp->floorshade = 
			sectp->ceilingshade = window.shade - shade;
			for (auto& wal : sectp->walls)
				wal.shade = window.shade - shade;
		}
	}
}

int addambient(int hitag, int lotag)
{

	ambienttags.Reserve(1);
	ambienttags.Last().lo = lotag;
	ambienttags.Last().hi = hitag;
	return ambienttags.Size() - 1;
}

END_DUKE_NS
