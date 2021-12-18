/*
** maploader.cpp
**
** Map loader for non-Blood maps
**
**---------------------------------------------------------------------------
** Copyright 2020 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <stdint.h>
#include "build.h"
#include "files.h"
#include "automap.h"
#include "printf.h"
#include "inputstate.h"
#include "md4.h"
#include "coreactor.h"
#include "gamecontrol.h"
#include "gamefuncs.h"
#include "sectorgeometry.h"
#include "render.h"
#include "hw_sections.h"
#include "interpolate.h"
#include "games/blood/src/mapstructs.h"

extern BitArray clipsectormap;

int numsectors, numwalls;	// not really needed anymore, need to be refactored out (58x numsectors, 48x numwalls)
TArray<sectortype> sector;
TArray<walltype> wall;

// for differential savegames.
TArray<sectortype> sectorbackup;
TArray<walltype> wallbackup;

void walltype::calcLength()
{
	lengthflags &= ~1;
	point2Wall()->lengthflags &= ~2;
	auto d = delta();
	length = (int)sqrt(d.x * d.x + d.y * d.y);
}

// needed for skipping over to get the map size first.
enum
{
	sectorsize5 = 37,
	sectorsize6 = 37,
	sectorsize7 = 40,
	wallsize5 = 35,
	wallsize6 = 32,
	wallsize7 = 32,
};

// This arena stores the larger allocated game-specific extension data. Since this can be freed in bulk a memory arena is better suited than malloc.
static FMemArena mapDataArena;

void walltype::allocX()
{
	using XWALL = BLD_NS::XWALL;
	_xw = (XWALL*)mapDataArena.Alloc(sizeof(XWALL));
	memset(_xw, 0, sizeof(XWALL));
}

void sectortype::allocX()
{
	using XSECTOR = BLD_NS::XSECTOR;
	_xs = (XSECTOR*)mapDataArena.Alloc(sizeof(XSECTOR));
	memset(_xs, 0, sizeof(XSECTOR));
}

static void ReadSectorV7(FileReader& fr, sectortype& sect)
{
	sect.wallptr = fr.ReadInt16();
	sect.wallnum = fr.ReadInt16();
	sect.ceilingz = fr.ReadInt32();
	sect.floorz = fr.ReadInt32();
	sect.ceilingstat = ESectorFlags::FromInt(fr.ReadUInt16());
	sect.floorstat = ESectorFlags::FromInt(fr.ReadUInt16());
	sect.ceilingpicnum = fr.ReadUInt16();
	sect.ceilingheinum = fr.ReadInt16();
	sect.ceilingshade = fr.ReadInt8();
	sect.ceilingpal = fr.ReadUInt8();
	sect.ceilingxpan_ = fr.ReadUInt8();
	sect.ceilingypan_ = fr.ReadUInt8();
	sect.floorpicnum = fr.ReadUInt16();
	sect.floorheinum = fr.ReadInt16();
	sect.floorshade = fr.ReadInt8();
	sect.floorpal = fr.ReadUInt8();
	sect.floorxpan_ = fr.ReadUInt8();
	sect.floorypan_ = fr.ReadUInt8();
	sect.visibility = fr.ReadUInt8();
	sect.fogpal = fr.ReadUInt8(); // note: currently unused, except for Blood.
	sect.lotag = fr.ReadInt16();
	sect.hitag = fr.ReadInt16();
	sect.extra = fr.ReadInt16();
}

static void ReadSectorV6(FileReader& fr, sectortype& sect)
{
	sect.wallptr = fr.ReadUInt16();
	sect.wallnum = fr.ReadUInt16();
	sect.ceilingpicnum = fr.ReadUInt16();
	sect.floorpicnum = fr.ReadUInt16();
	sect.ceilingheinum = clamp(fr.ReadInt16() << 5, -32768, 32767);
	sect.floorheinum = clamp(fr.ReadInt16() << 5, -32768, 32767);
	sect.ceilingz = fr.ReadInt32();
	sect.floorz = fr.ReadInt32();
	sect.ceilingshade = fr.ReadInt8();
	sect.floorshade = fr.ReadInt8();
	sect.ceilingxpan_ = fr.ReadUInt8();
	sect.floorxpan_ = fr.ReadUInt8();
	sect.ceilingypan_ = fr.ReadUInt8();
	sect.floorypan_ = fr.ReadUInt8();
	sect.ceilingstat = ESectorFlags::FromInt(fr.ReadUInt8());
	sect.floorstat = ESectorFlags::FromInt(fr.ReadUInt8());
	sect.ceilingpal = fr.ReadUInt8();
	sect.floorpal = fr.ReadUInt8();
	sect.visibility = fr.ReadUInt8();
	sect.lotag = fr.ReadInt16();
	sect.hitag = fr.ReadInt16();
	sect.extra = fr.ReadInt16();
}


static void ReadSectorV5(FileReader& fr, sectortype& sect)
{
	sect.wallptr = fr.ReadInt16();
	sect.wallnum = fr.ReadInt16();
	sect.ceilingpicnum = fr.ReadUInt16();
	sect.floorpicnum = fr.ReadUInt16();
	sect.ceilingheinum = clamp(fr.ReadInt16() << 5, -32768, 32767);
	sect.floorheinum = clamp(fr.ReadInt16() << 5, -32768, 32767);
	sect.ceilingz = fr.ReadInt32();
	sect.floorz = fr.ReadInt32();
	sect.ceilingshade = fr.ReadInt8();
	sect.floorshade = fr.ReadInt8();
	sect.ceilingxpan_ = fr.ReadUInt8();
	sect.floorxpan_ = fr.ReadUInt8();
	sect.ceilingypan_ = fr.ReadUInt8();
	sect.floorypan_ = fr.ReadUInt8();
	sect.ceilingstat = ESectorFlags::FromInt(fr.ReadUInt8());
	sect.floorstat = ESectorFlags::FromInt(fr.ReadUInt8());
	sect.ceilingpal = fr.ReadUInt8();
	sect.floorpal = fr.ReadUInt8();
	sect.visibility = fr.ReadUInt8();
	sect.lotag = fr.ReadInt16();
	sect.hitag = fr.ReadInt16();
	sect.extra = fr.ReadInt16();
	if ((sect.ceilingstat & CSTAT_SECTOR_SLOPE) == 0) sect.ceilingheinum = 0;
	if ((sect.floorstat & CSTAT_SECTOR_SLOPE) == 0) sect.floorheinum = 0;
}

static void ReadWallV7(FileReader& fr, walltype& wall)
{
	wall.pos.x = fr.ReadInt32();
	wall.pos.y = fr.ReadInt32();
	wall.point2 = fr.ReadInt16();
	wall.nextwall = fr.ReadInt16();
	wall.nextsector = fr.ReadInt16();
	wall.cstat = EWallFlags::FromInt(fr.ReadUInt16());
	wall.picnum = fr.ReadInt16();
	wall.overpicnum = fr.ReadInt16();
	wall.shade = fr.ReadInt8();
	wall.pal = fr.ReadUInt8();
	wall.xrepeat = fr.ReadUInt8();
	wall.yrepeat = fr.ReadUInt8();
	wall.xpan_ = fr.ReadUInt8();
	wall.ypan_ = fr.ReadUInt8();
	wall.lotag = fr.ReadInt16();
	wall.hitag = fr.ReadInt16();
	wall.extra = fr.ReadInt16();
}

static void ReadWallV6(FileReader& fr, walltype& wall)
{
	wall.pos.x = fr.ReadInt32();
	wall.pos.y = fr.ReadInt32();
	wall.point2 = fr.ReadInt16();
	wall.nextsector = fr.ReadInt16();
	wall.nextwall = fr.ReadInt16();
	wall.picnum = fr.ReadInt16();
	wall.overpicnum = fr.ReadInt16();
	wall.shade = fr.ReadInt8();
	wall.pal = fr.ReadUInt8();
	wall.cstat = EWallFlags::FromInt(fr.ReadUInt16());
	wall.xrepeat = fr.ReadUInt8();
	wall.yrepeat = fr.ReadUInt8();
	wall.xpan_ = fr.ReadUInt8();
	wall.ypan_ = fr.ReadUInt8();
	wall.lotag = fr.ReadInt16();
	wall.hitag = fr.ReadInt16();
	wall.extra = fr.ReadInt16();
}

static void ReadWallV5(FileReader& fr, walltype& wall)
{
	wall.pos.x = fr.ReadInt32();
	wall.pos.y = fr.ReadInt32();
	wall.point2 = fr.ReadInt16();
	wall.picnum = fr.ReadInt16();
	wall.overpicnum = fr.ReadInt16();
	wall.shade = fr.ReadInt8();
	wall.cstat = EWallFlags::FromInt(fr.ReadUInt16());
	wall.xrepeat = fr.ReadUInt8();
	wall.yrepeat = fr.ReadUInt8();
	wall.xpan_ = fr.ReadUInt8();
	wall.ypan_ = fr.ReadUInt8();

	wall.nextsector = fr.ReadInt16();
	wall.nextwall = fr.ReadInt16();
	fr.Seek(4, FileReader::SeekSet); // skip over 2 unused 16 bit values

	wall.lotag = fr.ReadInt16();
	wall.hitag = fr.ReadInt16();
	wall.extra = fr.ReadInt16();
}

static void SetWallPalV5()
{
	for (int i = 0; i < numsectors; i++)
	{
		int startwall = sector[i].wallptr;
		int endwall = startwall + sector[i].wallnum;
		for (int w = startwall; w < endwall; w++)
		{
			wall[w].pal = sector[i].floorpal;
		}
	}
}

void validateSprite(spritetype& spr, int sectnum, int index)
{
	bool bugged = false;
	if ((unsigned)spr.statnum >= MAXSTATUS)
	{
		Printf("Sprite #%d (%d,%d) has invalid statnum %d.\n", index, spr.x, spr.y, spr.statnum);
		bugged = true;
	}
	else if ((unsigned)spr.picnum >= MAXTILES)
	{
		Printf("Sprite #%d (%d,%d) has invalid picnum %d.\n", index, spr.x, spr.y, spr.picnum);
		bugged = true;
	}
	else if (!validSectorIndex(sectnum))
	{
		sectnum = -1;
		updatesector(spr.x, spr.y, &sectnum);
		bugged = sectnum < 0;

		if (!DPrintf(DMSG_WARNING, "Sprite #%d (%d,%d) with invalid sector %d was corrected to sector %d\n", index, spr.x, spr.y, sectnum, sectnum))
		{
			if (bugged) Printf("Sprite #%d (%d,%d) with invalid sector %d\n", index, spr.x, spr.y, sectnum);
		}
	}
	if (bugged)
	{
		spr.clear();
		spr.statnum = MAXSTATUS;
		sectnum = -1;
	}
	spr.setsector(sectnum);
}

static void ReadSpriteV7(FileReader& fr, spritetype& spr, int& secno)
{
	spr.pos.x = fr.ReadInt32();
	spr.pos.y = fr.ReadInt32();
	spr.pos.z = fr.ReadInt32();
	spr.cstat = ESpriteFlags::FromInt(fr.ReadUInt16());
	spr.picnum = fr.ReadInt16();
	spr.shade = fr.ReadInt8();
	spr.pal = fr.ReadUInt8();
	spr.clipdist = fr.ReadUInt8();
	spr.blend = fr.ReadUInt8();
	spr.xrepeat = fr.ReadUInt8();
	spr.yrepeat = fr.ReadUInt8();
	spr.xoffset = fr.ReadInt8();
	spr.yoffset = fr.ReadInt8();
	secno = fr.ReadInt16();
	spr.statnum = fr.ReadInt16();
	spr.ang = fr.ReadInt16();
	spr.owner = fr.ReadInt16();
	spr.xvel = fr.ReadInt16();
	spr.yvel = fr.ReadInt16();
	spr.zvel = fr.ReadInt16();
	spr.lotag = fr.ReadInt16();
	spr.hitag = fr.ReadInt16();
	spr.extra = fr.ReadInt16();
	spr.detail = 0;
}

static void ReadSpriteV6(FileReader& fr, spritetype& spr, int& secno)
{
	spr.pos.x = fr.ReadInt32();
	spr.pos.y = fr.ReadInt32();
	spr.pos.z = fr.ReadInt32();
	spr.cstat = ESpriteFlags::FromInt(fr.ReadUInt16());
	spr.shade = fr.ReadInt8();
	spr.pal = fr.ReadUInt8();
	spr.clipdist = fr.ReadUInt8();
	spr.xrepeat = fr.ReadUInt8();
	spr.yrepeat = fr.ReadUInt8();
	spr.xoffset = fr.ReadInt8();
	spr.yoffset = fr.ReadInt8();
	spr.picnum = fr.ReadInt16();
	spr.ang = fr.ReadInt16();
	spr.xvel = fr.ReadInt16();
	spr.yvel = fr.ReadInt16();
	spr.zvel = fr.ReadInt16();
	spr.owner = fr.ReadInt16();
	secno = fr.ReadInt16();
	spr.statnum = fr.ReadInt16();
	spr.lotag = fr.ReadInt16();
	spr.hitag = fr.ReadInt16();
	spr.extra = fr.ReadInt16();
	spr.blend = 0;
	spr.detail = 0;
}

static void ReadSpriteV5(FileReader& fr, spritetype& spr, int& secno)
{
	spr.pos.x = fr.ReadInt32();
	spr.pos.y = fr.ReadInt32();
	spr.pos.z = fr.ReadInt32();
	spr.cstat = ESpriteFlags::FromInt(fr.ReadUInt16());
	spr.shade = fr.ReadInt8();
	spr.xrepeat = fr.ReadUInt8();
	spr.yrepeat = fr.ReadUInt8();
	spr.picnum = fr.ReadInt16();
	spr.ang = fr.ReadInt16();
	spr.xvel = fr.ReadInt16();
	spr.yvel = fr.ReadInt16();
	spr.zvel = fr.ReadInt16();
	spr.owner = fr.ReadInt16();
	secno = fr.ReadInt16();
	spr.statnum = fr.ReadInt16();
	spr.lotag = fr.ReadInt16();
	spr.hitag = fr.ReadInt16();
	spr.extra = fr.ReadInt16();

	auto sec = spr.sector();
	if ((sec->ceilingstat & CSTAT_SECTOR_SKY) > 0)
		spr.pal = sec->ceilingpal;
	else
		spr.pal = sec->floorpal;

	spr.blend = 0;
	spr.clipdist = 32;
	spr.xoffset = 0;
	spr.yoffset = 0;
	spr.detail = 0;
}


void addBlockingPairs();

// allocates global map storage. Blood will also call this.
void allocateMapArrays(int numsprites)
{
	ClearInterpolations();


	show2dsector.Resize(numsectors);
	show2dwall.Resize(numwalls);
	gotsector.Resize(numsectors);
	clipsectormap.Resize(numsectors);

	mapDataArena.FreeAll();
	sector.Resize(numsectors);
	memset(sector.Data(), 0, sizeof(sectortype) * numsectors);
	wall.Resize(numwalls);
	memset(wall.Data(), 0, sizeof(walltype) * numwalls);

	ClearAutomap();
}

void fixSectors()
{
	for(auto& sect : sectors())
	{
		// Fix maps which do not set their wallptr to the first wall of the sector. Lo Wang In Time's map 11 is such a case.
		auto wp = sect.firstWall();
		// Note: we do not have the 'sector' index initialized here, it would not be helpful anyway for this fix.
		while (wp != wall.Data() && wp[-1].twoSided() && wp[-1].nextWall()->nextWall() == &wp[-1] && wp[-1].nextWall()->nextSector() == &sect)
		{
			sect.wallptr--;
			sect.wallnum++;
			wp--;
		}
	}
}

void loadMap(const char* filename, int flags, vec3_t* pos, int16_t* ang, int* cursectnum, SpawnSpriteDef& sprites)
{
	inputState.ClearAllInput();

	FileReader fr = fileSystem.OpenFileReader(filename);
	if (!fr.isOpen()) I_Error("Unable to open map %s", filename);
	int mapversion = fr.ReadInt32();
	if (mapversion < 5 || mapversion > 9) // 9 is most likely useless but let's try anyway.
	{
		I_Error("%s: Invalid map format, expected 5-9, got %d", filename, mapversion);
	}

	pos->x = fr.ReadInt32();
	pos->y = fr.ReadInt32();
	pos->z = fr.ReadInt32();
	*ang = fr.ReadInt16() & 2047;
	*cursectnum = fr.ReadUInt16();

	// Get the basics out before loading the data so that we can set up the global storage.
	numsectors = fr.ReadUInt16();
	auto sectorpos = fr.Tell();
	fr.Seek((mapversion == 5 ? sectorsize5 : mapversion == 6 ? sectorsize6 : sectorsize7) * numsectors, FileReader::SeekCur);
	numwalls = fr.ReadUInt16();
	auto wallpos = fr.Tell();
	fr.Seek((mapversion == 5 ? wallsize5 : mapversion == 6 ? wallsize6 : wallsize7)* numwalls, FileReader::SeekCur);
	int numsprites = fr.ReadUInt16();
	auto spritepos = fr.Tell();

	// Now that we know the map's size, set up the globals.
	allocateMapArrays(numsprites);
	sprites.sprites.Resize(numsprites);
	memset(sprites.sprites.Data(), 0, numsprites * sizeof(spritetype));

	// Now load the actual data.
	fr.Seek(sectorpos, FileReader::SeekSet);
	for (int i = 0; i < numsectors; i++)
	{
		switch (mapversion)
		{
		case 5: ReadSectorV5(fr, sector[i]); break;
		case 6: ReadSectorV6(fr, sector[i]); break;
		default: ReadSectorV7(fr, sector[i]); break;
		}
		// If we do not do this here, we need to do a lot more contortions to exclude this default from getting written out to savegames.
		if (isExhumed())
		{
			sector[i].Sound = -1;
		}
	}

	fr.Seek(wallpos, FileReader::SeekSet);
	for (int i = 0; i < numwalls; i++)
	{
		switch (mapversion)
		{
		case 5: ReadWallV5(fr, wall[i]); break;
		case 6: ReadWallV6(fr, wall[i]); break;
		default: ReadWallV7(fr, wall[i]); break;
		}
	}

	fr.Seek(spritepos, FileReader::SeekSet);
	for (int i = 0; i < numsprites; i++)
	{
		int secno = -1;
		switch (mapversion)
		{
		case 5: ReadSpriteV5(fr, sprites.sprites[i], secno); break;
		case 6: ReadSpriteV6(fr, sprites.sprites[i], secno); break;
		default: ReadSpriteV7(fr, sprites.sprites[i], secno); break;
		}
		validateSprite(sprites.sprites[i], secno, i);

	}

	artSetupMapArt(filename);

	//Must be last.
	fixSectors();
	updatesector(pos->x, pos->y, cursectnum);
	guniqhudid = 0;
	fr.Seek(0, FileReader::SeekSet);
	auto buffer = fr.Read();
	unsigned char md4[16];
	md4once(buffer.Data(), buffer.Size(), md4);
	loadMapHack(filename, md4, sprites);
	setWallSectors();
	hw_CreateSections();
	sectionGeometry.SetSize(sections.Size());


	wallbackup = wall;
	sectorbackup = sector;
}


void qloadboard(const char* filename, char flags, vec3_t* dapos, int16_t* daang);


// loads a map into the backup buffer.
void loadMapBackup(const char* filename)
{
	vec3_t pos;
	int16_t scratch;
	int scratch2;
	SpawnSpriteDef scratch3;

	if (isBlood())
	{
		qloadboard(filename, 0, &pos, &scratch);
	}
	else
	{
		loadMap(filename, 0, &pos, &scratch, &scratch2, scratch3);
	}
}

// Sets the sector reference for each wall. We need this for the triangulation cache.
void setWallSectors()
{
	int i = 0;
	for (auto& wal : walls())
	{
		wal.sector = -1;
		wal.lengthflags = 3;
	}

	for (int i = 0; i < numsectors - 1; i++)
	{
		auto sect = &sector[i];
		auto nextsect = &sector[i + 1];

		if (sect->wallptr < nextsect->wallptr && sect->wallptr + sect->wallnum > nextsect->wallptr)
		{
			// We have overlapping wall ranges for two sectors. Do some analysis to see where these walls belong
			int checkstart = nextsect->wallptr;
			int checkend = sect->wallptr + sect->wallnum;

			// for now assign the walls to the first sector. Final decisions are made below.
			nextsect->wallnum -= checkend - checkstart;
			nextsect->wallptr = checkend;

			auto belongs = [](int wal, int first, int last, int firstwal)
			{
				bool point2ok = wall[wal].point2 >= first && wall[wal].point2 < last;
				bool refok = false;
				for (int i = first; i < last; i++) 
					if (wall[i].point2 >= firstwal && wall[i].point2 <= wal) 
					{
						refok = true; break; 
					}
				return refok && point2ok;
			};
			while (checkstart < checkend && belongs(checkstart, sect->wallptr, checkstart, checkstart))
				checkstart++;

			sect->wallnum = checkstart - sect->wallptr;

			while (checkstart < checkend && belongs(checkend - 1, checkend, nextsect->wallptr + nextsect->wallnum, checkstart))
				checkend--;

			nextsect->wallnum += nextsect->wallptr - checkend;
			nextsect->wallptr = checkend;

			if (nextsect->wallptr > sect->wallptr + sect->wallnum)
			{
				// If there's a gap, assign to the first sector. In this case we may only guess.
				Printf("Wall range %d - %d referenced by sectors %d and %d\n", sect->wallptr + sect->wallnum, nextsect->wallptr - 1, i, i + 1);
				sect->wallnum = nextsect->wallptr - sect->wallptr;
			}
		}
	}

	for(auto& sect : sectors())
	{
		sect.dirty = EDirty::AllDirty;
		sect.exflags = 0;
		for (auto& wal : wallsofsector(&sect))
		{
			if (wal.sector == -1)
				wal.sector = i;
		}
		i++;
	}

	for (unsigned i = 1; i < wall.Size() - 1; i++)
	{
		// two maps in RRRA have this error. Delete one of those 2 walls.
		if (wall[i].point2 == wall[i + 1].point2)
		{
			wall[i].nextwall = -1;
			wall[i].nextsector = -1;
			wall[i].point2 = i;
		}
	}

	// validate 'nextsector' fields. Some maps have these wrong which can cause render glitches and occasionally even crashes.
	for (auto& wal : walls())
	{
		if (validWallIndex(wal.nextwall))
		{
			if (wal.nextsector != wal.nextWall()->sector)
			{
				DPrintf(DMSG_ERROR, "Bad 'nextsector' reference %d on wall %d\n", wal.nextsector, i);
				wal.nextsector = wal.nextWall()->sector;
			}
		}
		else wal.nextwall = -1;
	}

}

void MarkMap()
{
	for (auto& stat : statList)
	{
		GC::Mark(stat.firstEntry);
		GC::Mark(stat.lastEntry);
	}
	for (auto& sect : sectors())
	{
		GC::Mark(sect.firstEntry);
		GC::Mark(sect.lastEntry);
		if (isDukeLike()) GC::Mark(sect.hitagactor);
		else if (isBlood())
		{
			GC::Mark(sect.upperLink);
			GC::Mark(sect.lowerLink);
		}
	}
}
