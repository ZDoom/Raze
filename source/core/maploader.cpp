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

TArray<sectortype> sector;
TArray<walltype> wall;

// for differential savegames.
TArray<sectortype> sectorbackup;
TArray<walltype> wallbackup;

void walltype::calcLength()
{
	lengthflags &= ~1;
	point2Wall()->lengthflags &= ~2;
	auto d = int_delta();
	length = (int)sqrt(d.X * d.X + d.Y * d.Y);
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
	int c = fr.ReadInt32();
	int f = fr.ReadInt32();
	sect.setzfrommap(c, f);
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
	int c = fr.ReadInt32();
	int f = fr.ReadInt32();
	sect.setzfrommap(c, f);
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
	int c = fr.ReadInt32();
	int f = fr.ReadInt32();
	sect.setzfrommap(c, f);
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
	int x = fr.ReadInt32();
	int y = fr.ReadInt32();
	wall.setPosFromMap(x, y);
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
	int x = fr.ReadInt32();
	int y = fr.ReadInt32();
	wall.setPosFromMap(x, y);
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
	int x = fr.ReadInt32();
	int y = fr.ReadInt32();
	wall.setPosFromMap(x, y);
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
	for (unsigned i = 0; i < sector.Size(); i++)
	{
		int startwall = sector[i].wallptr;
		int endwall = startwall + sector[i].wallnum;
		for (int w = startwall; w < endwall; w++)
		{
			wall[w].pal = sector[i].floorpal;
		}
	}
}

void validateSprite(spritetype& spri, int sectnum, int index)
{
	auto pos = spri.int_pos();
	bool bugged = false;
	if ((unsigned)spri.statnum >= MAXSTATUS)
	{
		Printf("Sprite #%d (%d,%d) has invalid statnum %d.\n", index, pos.X, pos.Y, spri.statnum);
		bugged = true;
	}
	else if ((unsigned)spri.picnum >= MAXTILES)
	{
		Printf("Sprite #%d (%d,%d) has invalid picnum %d.\n", index, pos.X, pos.Y, spri.picnum);
		bugged = true;
	}
	else if (!validSectorIndex(sectnum))
	{
		sectnum = -1;
		updatesector(pos.X, pos.Y, &sectnum);
		bugged = sectnum < 0;

		if (!DPrintf(DMSG_WARNING, "Sprite #%d (%d,%d) with invalid sector %d was corrected to sector %d\n", index, pos.X, pos.Y, sectnum, sectnum))
		{
			if (bugged) Printf("Sprite #%d (%d,%d) with invalid sector %d\n", index, pos.X, pos.Y, sectnum);
		}
	}
	if (bugged)
	{
		spri = {};
		spri.statnum = MAXSTATUS;
		sectnum = -1;
	}
	if (sectnum >= 0) spri.sectp = &sector[sectnum];
	else spri.sectp = nullptr;
}

static void ReadSpriteV7(FileReader& fr, spritetype& spr, int& secno)
{
	int x = fr.ReadInt32();
	int y = fr.ReadInt32();
	int z = fr.ReadInt32();
	spr.SetMapPos(x, y, z);
	spr.cstat = ESpriteFlags::FromInt(fr.ReadUInt16());
	spr.picnum = fr.ReadInt16();
	spr.shade = fr.ReadInt8();
	spr.pal = fr.ReadUInt8();
	spr. clipdist = fr.ReadUInt8();
	spr.blend = fr.ReadUInt8();
	spr.xrepeat = fr.ReadUInt8();
	spr.yrepeat = fr.ReadUInt8();
	spr.xoffset = fr.ReadInt8();
	spr.yoffset = fr.ReadInt8();
	secno = fr.ReadInt16();
	spr.statnum = fr.ReadInt16();
	spr.intangle = fr.ReadInt16();
	spr.angle = DAngle::fromBuild(spr.intangle);
	spr.intowner = fr.ReadInt16();
	spr.xint = fr.ReadInt16();
	spr.yint = fr.ReadInt16();
	spr.inittype = fr.ReadInt16();
	spr.lotag = fr.ReadInt16();
	spr.hitag = fr.ReadInt16();
	spr.extra = fr.ReadInt16();
	spr.detail = 0;
}

static void ReadSpriteV6(FileReader& fr, spritetype& spr, int& secno)
{
	int x = fr.ReadInt32();
	int y = fr.ReadInt32();
	int z = fr.ReadInt32();
	spr.SetMapPos(x, y, z);
	spr.cstat = ESpriteFlags::FromInt(fr.ReadUInt16());
	spr.shade = fr.ReadInt8();
	spr.pal = fr.ReadUInt8();
	spr. clipdist = fr.ReadUInt8();
	spr.xrepeat = fr.ReadUInt8();
	spr.yrepeat = fr.ReadUInt8();
	spr.xoffset = fr.ReadInt8();
	spr.yoffset = fr.ReadInt8();
	spr.picnum = fr.ReadInt16();
	spr.intangle = fr.ReadInt16();
	spr.angle = DAngle::fromBuild(spr.intangle);
	spr.xint = fr.ReadInt16();
	spr.yint = fr.ReadInt16();
	spr.inittype = fr.ReadInt16();
	spr.intowner = fr.ReadInt16();
	secno = fr.ReadInt16();
	spr.statnum = fr.ReadInt16();
	spr.lotag = fr.ReadInt16();
	spr.hitag = fr.ReadInt16();
	spr.extra = fr.ReadInt16();
	spr.blend = 0;
	spr.detail = 0;
	if ((spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_SLOPE)
		spr.cstat &= ~CSTAT_SPRITE_ALIGNMENT_MASK;
}

static void ReadSpriteV5(FileReader& fr, spritetype& spr, int& secno)
{
	int x = fr.ReadInt32();
	int y = fr.ReadInt32();
	int z = fr.ReadInt32();
	spr.SetMapPos(x, y, z);
	spr.cstat = ESpriteFlags::FromInt(fr.ReadUInt16());
	spr.shade = fr.ReadInt8();
	spr.xrepeat = fr.ReadUInt8();
	spr.yrepeat = fr.ReadUInt8();
	spr.picnum = fr.ReadInt16();
	spr.intangle = fr.ReadInt16();
	spr.angle = DAngle::fromBuild(spr.intangle);
	spr.xint = fr.ReadInt16();
	spr.yint = fr.ReadInt16();
	spr.inittype = fr.ReadInt16();
	spr.intowner = fr.ReadInt16();
	secno = fr.ReadInt16();
	spr.statnum = fr.ReadInt16();
	spr.lotag = fr.ReadInt16();
	spr.hitag = fr.ReadInt16();
	spr.extra = fr.ReadInt16();
	if ((spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_SLOPE)
		spr.cstat &= ~CSTAT_SPRITE_ALIGNMENT_MASK;

	auto sec = spr.sectp;
	if ((sec->ceilingstat & CSTAT_SECTOR_SKY) > 0)
		spr.pal = sec->ceilingpal;
	else
		spr.pal = sec->floorpal;

	spr.blend = 0;
	spr. clipdist = 32;
	spr.xoffset = 0;
	spr.yoffset = 0;
	spr.detail = 0;
}


// allocates global map storage. Blood will also call this.
void allocateMapArrays(int numwall, int numsector, int numsprites)
{
	ClearInterpolations();


	show2dsector.Resize(numsector);
	show2dwall.Resize(numwall);
	gotsector.Resize(numsector);
	clipsectormap.Resize(numsector);

	mapDataArena.FreeAll();
	sector.Resize(numsector);
	memset(sector.Data(), 0, sizeof(sectortype) * numsector);
	wall.Resize(numwall);
	memset(wall.Data(), 0, sizeof(walltype) * wall.Size());

	ClearAutomap();
}

void fixSectors()
{
	for(auto& sect: sector)
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

void validateStartSector(const char* filename, const DVector3& pos, int* cursectnum, unsigned numsectors, bool noabort)
{

	if ((unsigned)(*cursectnum) >= numsectors)
	{
		sectortype* sect = nullptr;
		updatesectorz(pos, &sect);
		if (!sect) updatesector(pos, &sect);
		if (sect || noabort)
		{
			Printf(PRINT_HIGH, "Error in map %s: Start sector %d out of range. Max. sector is %d\n", filename, *cursectnum, numsectors);
			*cursectnum = sect? sectnum(sect) : 0;
		}
		else
		{
			I_Error("Unable to start map %s: Start sector %d out of range. Max. sector is %d. No valid location at start spot\n", filename, *cursectnum, numsectors);
		}
	}


}

void loadMap(const char* filename, int flags, DVector3* pos, int16_t* ang, int* cursectnum, SpawnSpriteDef& sprites)
{
	inputState.ClearAllInput();

	FileReader fr = fileSystem.OpenFileReader(filename);
	if (!fr.isOpen()) I_Error("Unable to open map %s", filename);
	int mapversion = fr.ReadInt32();
	if (mapversion < 5 || mapversion > 9) // 9 is most likely useless but let's try anyway.
	{
		I_Error("%s: Invalid map format, expected 5-9, got %d", filename, mapversion);
	}

	pos->X = fr.ReadInt32() * maptoworld;
	pos->Y = fr.ReadInt32() * maptoworld;
	pos->Z = fr.ReadInt32() * zmaptoworld;
	*ang = fr.ReadInt16() & 2047;
	*cursectnum = fr.ReadUInt16();

	// Get the basics out before loading the data so that we can set up the global storage.
	unsigned numsectors = fr.ReadUInt16();
	auto sectorpos = fr.Tell();
	fr.Seek((mapversion == 5 ? sectorsize5 : mapversion == 6 ? sectorsize6 : sectorsize7) * numsectors, FileReader::SeekCur);
	unsigned numwalls = fr.ReadUInt16();
	auto wallpos = fr.Tell();
	fr.Seek((mapversion == 5 ? wallsize5 : mapversion == 6 ? wallsize6 : wallsize7)* numwalls, FileReader::SeekCur);
	int numsprites = fr.ReadUInt16();
	auto spritepos = fr.Tell();

	// Now that we know the map's size, set up the globals.
	allocateMapArrays(numwalls, numsectors, numsprites);
	sprites.sprites.Resize(numsprites);
	memset(sprites.sprites.Data(), 0, numsprites * sizeof(spritetype));

	// Now load the actual data.
	fr.Seek(sectorpos, FileReader::SeekSet);
	for (unsigned i = 0; i < sector.Size(); i++)
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
	for (unsigned i = 0; i < wall.Size(); i++)
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
	updatesector(*pos, cursectnum);
	guniqhudid = 0;
	fr.Seek(0, FileReader::SeekSet);
	auto buffer = fr.Read();
	uint8_t md4[16];
	md4once(buffer.Data(), buffer.Size(), md4);
	loadMapHack(filename, md4, sprites);
	setWallSectors();
	hw_CreateSections();
	sectionGeometry.SetSize(sections.Size());


	wallbackup = wall;
	sectorbackup = sector;
	validateStartSector(filename, *pos, cursectnum, numsectors);
}


//==========================================================================
//
// Decrypt
//
// Note that this is different from the general RFF encryption.
//
//==========================================================================

static void Decrypt(void* to_, const void* from_, int len, int key)
{
	uint8_t* to = (uint8_t*)to_;
	const uint8_t* from = (const uint8_t*)from_;

	for (int i = 0; i < len; ++i, ++key)
	{
		to[i] = from[i] ^ key;
	}
}


//==========================================================================
//
// P_LoadBloodMap
// 
// This was adapted from ZDoom's old Build map loader.
//
//==========================================================================

static void P_LoadBloodMapWalls(uint8_t* data, size_t len, TArray<walltype>& lwalls)
{
	uint8_t infoBlock[37];
	int mapver = data[5];
	uint32_t matt;
	int i;
	int k;

	if (mapver != 6 && mapver != 7)
	{
		return;
	}

	matt = *(uint32_t*)(data + 28);
	if (matt != 0 &&
		matt != MAKE_ID('M', 'a', 't', 't') &&
		matt != MAKE_ID('t', 't', 'a', 'M'))
	{
		Decrypt(infoBlock, data + 6, 37, 0x7474614d);
	}
	else
	{
		memcpy(infoBlock, data + 6, 37);
	}
	int numRevisions = *(uint32_t*)(infoBlock + 27);
	int numSectors = *(uint16_t*)(infoBlock + 31);
	int numWalls = *(uint16_t*)(infoBlock + 33);
	int numSprites = *(uint16_t*)(infoBlock + 35);
	int skyLen = 2 << *(uint16_t*)(infoBlock + 16);

	if (mapver == 7)
	{
		// Version 7 has some extra stuff after the info block. This
		// includes a copyright, and I have no idea what the rest of
		// it is.
		data += 171;
	}
	else
	{
		data += 43;
	}

	// Skip the sky info.
	data += skyLen;

	lwalls.Reserve(numWalls);

	// Read sectors
	k = numRevisions * sizeof(sectortypedisk);
	for (i = 0; i < numSectors; ++i)
	{
		sectortypedisk bsec;
		if (mapver == 7)
		{
			Decrypt(&bsec, data, sizeof(sectortypedisk), k);
		}
		else
		{
			memcpy(&bsec, data, sizeof(sectortypedisk));
		}
		data += sizeof(sectortypedisk);
		if (bsec.extra > 0)	// skip Xsector
		{
			data += 60;
		}
	}

	// Read walls
	k |= 0x7474614d;
	for (i = 0; i < numWalls; ++i)
	{
		walltypedisk load;
		if (mapver == 7)
		{
			Decrypt(&load, data, sizeof(walltypedisk), k);
		}
		else
		{
			memcpy(&load, data, sizeof(walltypedisk));
		}
		// only copy what we need to draw the map preview.

		auto pWall = &lwalls[i];

		int x = LittleLong(load.x);
		int y = LittleLong(load.y);
		pWall->setPosFromMap(x, y);
		pWall->point2 = LittleShort(load.point2);
		pWall->nextwall = LittleShort(load.nextwall);
		pWall->nextsector = LittleShort(load.nextsector);

		data += sizeof(walltypedisk);
		if (load.extra > 0)	// skip Xwall
		{
			data += 24;
		}
	}

}

TArray<walltype> loadMapWalls(const char* filename)
{
	TArray<walltype> lwalls;
	FileReader fr = fileSystem.OpenFileReader(filename);
	if (!fr.isOpen()) return lwalls;

	if (isBlood())
	{
		auto data = fr.Read();
		P_LoadBloodMapWalls(data.Data(), data.Size(), lwalls);
		return lwalls;
	}


	int mapversion = fr.ReadInt32();
	if (mapversion < 5 || mapversion > 9) return lwalls;

	fr.Seek(16, FileReader::SeekCur);

	// Get the basics out before loading the data so that we can set up the global storage.
	unsigned numsectors = fr.ReadUInt16();
	fr.Seek((mapversion == 5 ? sectorsize5 : mapversion == 6 ? sectorsize6 : sectorsize7) * numsectors, FileReader::SeekCur);
	unsigned numwalls = fr.ReadUInt16();

	lwalls.Resize(numwalls);
	for (unsigned i = 0; i < lwalls.Size(); i++)
	{
		switch (mapversion)
		{
		case 5: ReadWallV5(fr, lwalls[i]); break;
		case 6: ReadWallV6(fr, lwalls[i]); break;
		default: ReadWallV7(fr, lwalls[i]); break;
		}
	}
	return lwalls;
}


void qloadboard(const char* filename, uint8_t flags, DVector3* dapos, int16_t* daang);


// loads a map into the backup buffer.
void loadMapBackup(const char* filename)
{
	DVector3 fpos;
	int16_t scratch;
	int scratch2;
	SpawnSpriteDef scratch3;

	if (isBlood())
	{
		qloadboard(filename, 0, &fpos, &scratch);
	}
	else
	{
		loadMap(filename, 0, &fpos, &scratch, &scratch2, scratch3);
	}
}

// Sets the sector reference for each wall. We need this for the triangulation cache.
void setWallSectors()
{
	for (auto& wal : wall)
	{
		wal.sector = -1;
		wal.lengthflags = 3;
	}

	for (unsigned i = 0; i < sector.Size() - 1; i++)
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

	int i = 0;
	for(auto& sect: sector)
	{
		sect.dirty = EDirty::AllDirty;
		for (auto& wal : wallsofsector(&sect))
		{
			if (wal.sector == -1)
				wal.sector = i;
		}
		i++;
	}

	// 
	for (unsigned ii = 1; ii < wall.Size() - 1; ii++)
	{
		// two maps in RRRA have this error. Delete one of those 2 walls.
		if (wall[ii].point2 == wall[ii + 1].point2)
		{
			auto w1 = wall[ii].lastWall(false);
			auto w2 = wall[ii + 1].lastWall(false);
			// Neutralize the bad one of the two walls.
			if (w1 == nullptr)
			{
				wall[ii].nextwall = -1;
				wall[ii].nextsector = -1;
				wall[ii].point2 = ii;
			}
			else if (w2 == nullptr)
			{
				wall[ii+1].nextwall = -1;
				wall[ii+1].nextsector = -1;
				wall[ii+1].point2 = ii;
			}
		}
	}

	// validate 'nextsector' fields. Some maps have these wrong which can cause render glitches and occasionally even crashes.
	for (auto& wal : wall)
	{
		if (validWallIndex(wal.nextwall))
		{
			if (wal.nextsector != wal.nextWall()->sector)
			{
				DPrintf(DMSG_ERROR, "Bad 'nextsector' reference %d on wall %d\n", wal.nextsector, wall.IndexOf(&wal));
				wal.nextsector = wal.nextWall()->sector;
			}
		}
		else
		{
			wal.nextwall = -1;
			wal.nextsector = -1;
		}
	}

}

void MarkMap()
{
	for (auto& stat : statList)
	{
		GC::Mark(stat.firstEntry);
		GC::Mark(stat.lastEntry);
	}
	for (auto& sect: sector)
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
