/*
** buildtexture.cpp
** Handling Build textures
**
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
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
**
*/

#include "files.h"
#include "zstring.h"
#include "buildtiles.h"
#include "image.h"

#include "palette.h"
#include "m_crc32.h"
#include "build.h"
#include "gamecontrol.h"
#include "palettecontainer.h"
#include "texturemanager.h"
#include "c_dispatch.h"
#include "sc_man.h"
#include "gamestruct.h"
#include "hw_voxels.h"

#include "hw_renderstate.h"

enum
{
	MAXARTFILES_BASE = 200,
	MAXARTFILES_TOTAL = 220
};


BuildTiles TileFiles;

int tileSetHightileReplacement(int picnum, int palnum, const char* filename, float alphacut, float xscale, float yscale, float specpower, float specfactor, bool indexed = false);

//==========================================================================
//
// 
//
//==========================================================================

picanm_t tileConvertAnimFormat(int32_t const picanimraw, int* lo, int* to)
{
	// Unpack a 4 byte packed anim descriptor into something more accessible
	picanm_t anm;
	anm.num = picanimraw & 63;
	*lo = (int8_t)((picanimraw >> 8) & 255);
	*to = (int8_t)((picanimraw >> 16) & 255);
	anm.sf = ((picanimraw >> 24) & 15) | (picanimraw & 192);
	anm.extra = (picanimraw >> 28) & 15;
	return anm;
}


//==========================================================================
//
// Base class for Build tile textures
// This needs a few subclasses for different use cases.
//
//==========================================================================

int FTileTexture::CopyPixels(FBitmap* bmp, int conversion)
{
	TArray<uint8_t> buffer;
	auto ppix = GetRawData();
	if (ppix)
	{
		bmp->CopyPixelData(0, 0, ppix, Width, Height, Height, 1, 0, GPalette.BaseColors);
	}
	return 0;
}

TArray<uint8_t> FTileTexture::CreatePalettedPixels(int conversion)
{
	TArray<uint8_t> buffer(Width * Height, true);
	auto p = GetRawData();
	if (p) memcpy(buffer.Data(), p, buffer.Size());
	else memset(buffer.Data(), 0, buffer.Size());
	return buffer;
}

//==========================================================================
//
// 
//
//==========================================================================

static FGameTexture* GetTileTexture(const char* name, const TArray<uint8_t>& backingstore, uint32_t offset, int width, int height)
{
	auto tex = new FArtTile(backingstore, offset, width, height);
	auto p = &backingstore[offset];
	auto siz = width * height;
	for (int i = 0; i < siz; i++, p++)
	{
		// move transparent color to index 0 to get in line with the rest of the texture management.
		if (*p == 0) *p = 255;
		else if (*p == 255) *p = 0;
	}

	if (tex)
	{
		return MakeGameTexture(new FImageTexture(tex), name, ETextureType::Any);
	}
	return nullptr;
}


void BuildTiles::Init()
{
	Placeholder = TexMan.GameByIndex(0);
	for (auto& tile : tiledata)
	{
		tile.texture = Placeholder;
		tile.backup = Placeholder;
		tile.picanm = {};
		tile.RotTile = { -1,-1 };
		tile.replacement = ReplacementType::Art;
		tile.alphaThreshold = 0.5;
		tile.hiofs = {};
	}

}
//==========================================================================
//
// 
//
//==========================================================================

void BuildTiles::AddTile(int tilenum, FGameTexture* tex, bool permap)
{
	if (!tex->GetID().isValid())
		TexMan.AddGameTexture(tex);
	tiledata[tilenum].texture = tex;
	if (!permap) tiledata[tilenum].backup = tex;
}

//===========================================================================
//
// AddTiles
//
// Adds all the tiles in an artfile to the texture manager.
//
//===========================================================================

void BuildTiles::AddTiles (int firsttile, TArray<uint8_t>& RawData, const char *mapname)
{

	const uint8_t *tiles = RawData.Data();
//	int numtiles = LittleLong(((uint32_t *)tiles)[1]);	// This value is not reliable
	int tilestart = LittleLong(((int *)tiles)[2]);
	int tileend = LittleLong(((int *)tiles)[3]);
	const uint16_t *tilesizx = &((const uint16_t *)tiles)[8];
	const uint16_t *tilesizy = &tilesizx[tileend - tilestart + 1];
	const uint32_t *picanm = (const uint32_t *)&tilesizy[tileend - tilestart + 1];
	const uint8_t *tiledata = (const uint8_t *)&picanm[tileend - tilestart + 1];

	if (firsttile != -1)
	{
		tileend = tileend - tilestart + firsttile;
		tilestart = firsttile;
	}

	for (int i = tilestart; i <= tileend; ++i)
	{
		int pic = i - tilestart;
		int width = LittleShort(tilesizx[pic]);
		int height = LittleShort(tilesizy[pic]);
		uint32_t anm = LittleLong(picanm[pic]);
		int size = width*height;

		if (width <= 0 || height <= 0) continue;

		FString texname;
		if (mapname) texname.Format("maptile_%s_%05d", mapname, i);
		else texname.Format("#%05d", i);
		auto tex = GetTileTexture(texname, RawData, uint32_t(tiledata - tiles), width, height);
		AddTile(i, tex);
		int leftoffset, topoffset;
		this->tiledata[i].picanmbackup = this->tiledata[i].picanm = tileConvertAnimFormat(anm, &leftoffset, &topoffset);
		tex->SetOffsets(leftoffset, topoffset);

		tiledata += size;
	}
}

//===========================================================================
//
// CountTiles
//
// Returns the number of tiles provided by an artfile
//
//===========================================================================

int CountTiles (const char *fn, const uint8_t *RawData)
{
	int version = LittleLong(*(uint32_t *)RawData);
	if (version != 1)
	{
		Printf("%s: Invalid art file version.  Must be 1, got %d\n", fn, version);
		return 0;
	}

	int tilestart = LittleLong(((uint32_t *)RawData)[2]);
	int tileend = LittleLong(((uint32_t *)RawData)[3]);

	if ((unsigned)tilestart >= MAXUSERTILES || (unsigned)tileend >= MAXUSERTILES)
	{
		Printf("%s: Invalid tilestart or tileend\n", fn);
		return 0;
	}
	if (tileend < tilestart)
	{
		Printf("%s: tileend < tilestart\n", fn);
		return 0;
	}

	return tileend >= tilestart ? tileend - tilestart + 1 : 0;
}

//===========================================================================
//
// InvalidateTile
//
//===========================================================================

void BuildTiles::InvalidateTile(int num)
{
	if ((unsigned) num < MAXTILES)
	{
		auto tex = tiledata[num].texture;
		tex->CleanHardwareData();
		tiledata[num].rawCache.data.Clear();
	}
}

//===========================================================================
//
// MakeCanvas
//
// Turns texture into a canvas (i.e. camera texture)
//
//===========================================================================

void BuildTiles::MakeCanvas(int tilenum, int width, int height)
{
	auto canvas = ValidateCustomTile(tilenum, ReplacementType::Canvas);
	canvas->SetSize(width*4, height*4);
	canvas->SetDisplaySize((float)width, (float)height);
	canvas->GetTexture()->SetSize(width * 4, height * 4);
	static_cast<FCanvasTexture*>(canvas->GetTexture())->aspectRatio = (float)width / height;
}

//===========================================================================
//
// LoadArtFile
//
// Returns the number of tiles found.
//
// let's load everything into memory on startup.
//
//===========================================================================

int BuildTiles::LoadArtFile(const char *fn, const char *mapname, int firsttile)
{
	unsigned old = FindFile(fn);
	if (old >= ArtFiles.Size())	// Do not process if already loaded.
	{
		FileReader fr = fileSystem.OpenFileReader(fn);
		if (fr.isOpen())
		{
			auto artdata = fr.Read();
			const uint8_t *artptr = artdata.Data();
			if (artdata.Size() > 16)
			{
				if (memcmp(artptr, "BUILDART", 8) == 0)
				{
					artdata.Delete(0, 8);
				}
				// Only load the data if the header is present
				if (CountTiles(fn, artptr) > 0)
				{
					auto file = new BuildArtFile;
					ArtFiles.Push(file);
					file->filename = fn;
					file->RawData = std::move(artdata);
					AddTiles(firsttile, file->RawData, mapname);
				}
			}
		}
		else
		{
			//Printf("%s: file not found\n", fn);
			return -1;
		}
	}
	return 0;
}

//==========================================================================
//
// 
//
//==========================================================================

void BuildTiles::LoadArtSet(const char* filename)
{
	for (int index = 0; index < MAXARTFILES_BASE; index++)
	{
		FStringf fn(filename, index);
		LoadArtFile(fn, nullptr);
	}
	for (auto& addart : addedArt)
	{
		LoadArtFile(addart, nullptr);
	}
}


//==========================================================================
//
// Checks if a custom tile has alredy been added to the list.
// For each tile index there may only be one replacement and its
// type may never change!
//
// All these uses will need some review further down the line so that the texture manager's content is immutable.
//
//==========================================================================

FGameTexture* BuildTiles::ValidateCustomTile(int tilenum, ReplacementType type)
{
	if (tilenum < 0 || tilenum >= MAXTILES) return nullptr;
	auto &td = tiledata[tilenum];
	if (td.texture != td.backup) return nullptr;	// no mucking around with map tiles.
	auto tile = td.texture;
	auto reptype = td.replacement;
	if (reptype == type) return tile;		// already created
	if (reptype > ReplacementType::Art) return nullptr;		// different custom type - cannot replace again.
	FTexture* replacement = nullptr;
	td.replacement = type;
	if (type == ReplacementType::Writable)
	{
		// Creates an empty writable tile.
		// Current use cases are:
		// Camera textures (should be made to be creatable by the hardware renderer instead of falling back on the software renderer.)
		// thumbnails for savegame and loadgame (should bypass the texture manager entirely.)
		// view tilting in the software renderer (this should just use a local buffer instead of relying on the texture manager.)
		// Movie playback (like thumbnails this should bypass the texture manager entirely.)
		// Blood's 'lens' effect (apparently MP only) - combination of a camera texture with a distortion map - should be made a shader effect to be applied to the camera texture.
		replacement = new FImageTexture(new FWritableTile);
	}
	else if (type == ReplacementType::Restorable)
	{
		// This is for modifying an existing tile.
		// It only gets used for the crosshair and a few specific effects:
		// A) the fire in Blood.
		// B) the pin display in Redneck Rampage's bowling lanes.
		// C) Exhumed's menu plus one special effect tile.
		// All of these effects should probably be redone without actual texture hacking...
		if (tile->GetTexelWidth() == 0 || tile->GetTexelHeight() == 0) return nullptr;	// The base must have a size for this to work.
		// todo: invalidate hardware textures for tile.
		replacement = new FImageTexture(new FRestorableTile(tile->GetTexture()->GetImage()));
	}
	else if (type == ReplacementType::Canvas)
	{
		// necessary fuckery thanks to SW considering camera textures the same as mirrors and deleting them if opportune.
		// If we do not remember them this would create new ones over and over again.
		auto check = cameratextures.CheckKey(tilenum);
		if (check)
		{
			// if this once was a camera texture but now isn't anymore, grab that camera texture from the cache and reuse it.
			AddTile(tilenum, *check);
			return *check;
		}

		replacement = new FCanvasTexture(1, 1);
	}
	else return nullptr;
	auto rep = MakeGameTexture(replacement, tile->GetName(), ETextureType::Override);
	if (type == ReplacementType::Canvas)
	{
		cameratextures.Insert(tilenum, rep);
	}
	AddTile(tilenum, rep);
	return rep;
}

//==========================================================================
//
//  global interface
//
//==========================================================================

int32_t BuildTiles::artLoadFiles(const char* filename)
{
	TileFiles.LoadArtSet(filename);
	memset(gotpic, 0, sizeof(gotpic));
	return 0;
}

//==========================================================================
//
// Creates a tile for displaying custom content
//
//==========================================================================

uint8_t* BuildTiles::tileCreate(int tilenum, int width, int height)
{
	if (width <= 0 || height <= 0) return nullptr;
	auto tex = ValidateCustomTile(tilenum, ReplacementType::Writable);
	if (tex == nullptr) return nullptr;
	auto wtex = static_cast<FWritableTile*>(tex->GetTexture()->GetImage());
	if (!wtex->ResizeImage(width, height)) return nullptr;
	tex->SetSize(width, height);
	return wtex->GetRawData();
}

//==========================================================================
//
// Makes a tile writable - only used for a handful of special cases
// (todo: Investigate how to get rid of this)
//
//==========================================================================

uint8_t* BuildTiles::tileMakeWritable(int num)
{
	auto tex = ValidateCustomTile(num, ReplacementType::Restorable);
	auto wtex = static_cast<FWritableTile*>(tex->GetTexture()->GetImage());
	return wtex ? wtex->GetRawData() : nullptr;
}

//==========================================================================
//
// Returns checksum for a given tile
//
//==========================================================================

int32_t tileGetCRC32(int tileNum)
{
	if ((unsigned)tileNum >= (unsigned)MAXTILES) return 0;
	auto tile = dynamic_cast<FArtTile*>(TileFiles.tiledata[tileNum].texture->GetTexture()->GetImage());	// only consider original ART tiles.
	if (!tile) return 0;
	auto pixels = tile->GetRawData();
	if (!pixels) return 0;

	auto size = tile->GetWidth() * tile->GetHeight();
	if (size == 0) return 0;

	// Temporarily revert the data to its original form with 255 being transparent. Otherwise the CRC won't match.
	auto p = pixels;
	for (int i = 0; i < size; i++, p++)
	{
		// move transparent color to index 0 to get in line with the rest of the texture management.
		if (*p == 0) *p = 255;
		else if (*p == 255) *p = 0;
	}

	auto crc = crc32(0, (const Bytef*)pixels, size);

	// ... and back again.
	p = pixels;
	for (int i = 0; i < size; i++, p++)
	{
		// move transparent color to index 0 to get in line with the rest of the texture management.
		if (*p == 0) *p = 255;
		else if (*p == 255) *p = 0;
	}
	return crc;
}

CCMD(tilecrc)
{
	if (argv.argc() > 1)
	{
		int tile = atoi(argv[1]);
		Printf("%d\n", tileGetCRC32(tile));
	}
}

//==========================================================================
//
// Import a tile from an external image.
// This has been signifcantly altered so it may not cover everything yet.
//
//==========================================================================

int tileImportFromTexture(const char* fn, int tilenum, int alphacut, int istexture)
{
	FTextureID texid = TexMan.CheckForTexture(fn, ETextureType::Any, FTextureManager::TEXMAN_ForceLookup);
	if (!texid.isValid()) return -1;
	auto tex = TexMan.GetGameTexture(texid);

	int32_t xsiz = tex->GetTexelWidth(), ysiz = tex->GetTexelHeight();

	if (xsiz <= 0 || ysiz <= 0)
		return -2;

	// create a new game texture here - we want to give it a different name!
	tex = MakeGameTexture(tex->GetTexture(), FStringf("#%05d", tilenum), ETextureType::Override);
	TexMan.AddGameTexture(tex);
	TileFiles.tiledata[tilenum].backup = TileFiles.tiledata[tilenum].texture = tex;
	if (istexture)
		tileSetHightileReplacement(tilenum, 0, fn, (float)(255 - alphacut) * (1.f / 255.f), 1.0f, 1.0f, 1.0, 1.0);
	return 0;

}

//==========================================================================
//
// Copies a tile into another and optionally translates its palette.
//
//==========================================================================

void tileCopy(int tile, int source, int pal, int xoffset, int yoffset, int flags)
{
	picanm_t* picanm = nullptr;
	picanm_t* sourceanm = nullptr;
	int srcxo, srcyo;
	FGameTexture* tex;

	if (pal == -1 && tile == source)
	{
		// Only modify the picanm info.
		tex = tileGetTexture(tile);
		if (!tex) return;
		picanm = &TileFiles.tiledata[tile].picanm;
		sourceanm = picanm;
		srcxo = tex->GetTexelLeftOffset(0);
		srcyo = tex->GetTexelTopOffset(0);
	}
	else
	{
		if (source == -1) source = tile;
		tex = tileGetTexture(source);
		if (!tex) return;
		sourceanm = &TileFiles.tiledata[source].picanm;
		srcxo = tex->GetTexelLeftOffset(0);
		srcyo = tex->GetTexelTopOffset(0);

		TArray<uint8_t> buffer = tex->GetTexture()->Get8BitPixels(false);

		if (pal != -1)
		{
			auto remap = lookups.getTable(pal);
			for (auto& pixel : buffer)
			{
				pixel = remap[pixel];
			}
		}
		tex = MakeGameTexture(new FImageTexture(new FLooseTile(buffer, tex->GetTexelWidth(), tex->GetTexelHeight())), "", ETextureType::Any);
		picanm = &TileFiles.tiledata[tile].picanm;
		TileFiles.AddTile(tile, tex);
	}

	if (xoffset != -1024) srcxo = clamp(xoffset, -128, 127);
	if (yoffset != -1024) srcyo = clamp(yoffset, -128, 127);
	tex->SetOffsets(srcxo, srcyo);
	picanm->sf = (picanm->sf & ~PICANM_MISC_MASK) | (sourceanm->sf & PICANM_MISC_MASK) | flags;
}

//==========================================================================
//
// Clear map specific ART
//
//==========================================================================
static FString currentMapArt;

void artClearMapArt(void)
{
	for (auto& td : TileFiles.tiledata)
	{
		td.texture = td.backup;
		td.picanm = td.picanmbackup;
	}
	currentMapArt = "";
}

//==========================================================================
//
// Load map specficied ART
//
//==========================================================================

void artSetupMapArt(const char* filename)
{
	FString lcfilename = StripExtension(filename);
	lcfilename.MakeLower();

	if (currentMapArt.CompareNoCase(lcfilename) == 0) return;
	artClearMapArt();
	currentMapArt = lcfilename;


	// Re-get from the texture manager if this map's tiles have already been created.
	if (TileFiles.maptilesadded.Find(lcfilename) < TileFiles.maptilesadded.Size())
	{
		for (int i = 0; i < MAXTILES; i++)
		{
			FStringf name("maptile_%s_%05d", lcfilename.GetChars(), i);
			auto texid = TexMan.CheckForTexture(name, ETextureType::Any);
			if (texid.isValid())
			{
				TileFiles.tiledata[i].texture = TexMan.GetGameTexture(texid);
			}
		}
		return;
	}

	TileFiles.maptilesadded.Push(lcfilename);


	FStringf firstname("%s_00.art", lcfilename.GetChars());
	auto fr = fileSystem.OpenFileReader(firstname);
	if (!fr.isOpen()) return;
	for (auto& td : TileFiles.tiledata)
	{
		td.picanmbackup = td.picanm;
	}


	for (int i = 0; i < MAXARTFILES_TOTAL - MAXARTFILES_BASE; i++)
	{
		FStringf fullname("%s_%02d.art", lcfilename.GetChars(), i);
		TileFiles.LoadArtFile(fullname, filename);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void tileDelete(int tile)
{
	TileFiles.tiledata[tile].texture = TileFiles.tiledata[tile].backup = TexMan.GameByIndex(0);
	TileFiles.tiledata[tile].replacement = ReplacementType::Art; // whatever this was, now it isn't anymore. (SW tries to nuke camera textures with this, :( )
	tiletovox[tile] = -1; // clear the link but don't clear the voxel. It may be in use for another tile.
	md_undefinetile(tile);
}

//==========================================================================
//
//
//
//==========================================================================

void tileSetDummy(int tile, int width, int height)
{
	if (width == 0 || height == 0)
	{
		tileDelete(tile);
	}
	else if (width > 0 && height > 0)
	{
		FStringf texname("#%05d", tile);
		auto dtile = MakeGameTexture(new FImageTexture(new FDummyTile(width, height)), texname, ETextureType::Any);
		TileFiles.AddTile(tile, dtile);
	}
}

//==========================================================================
//
//
//
//==========================================================================

int BuildTiles::findUnusedTile(void)
{
	static int lastUnusedTile = MAXUSERTILES - 1;

	for (; lastUnusedTile >= 0; --lastUnusedTile)
	{
		auto tex = tileGetTexture(lastUnusedTile);
		if (!tex || !tex->isValid()) return lastUnusedTile;
	}
	return -1;
}

//==========================================================================
//
// fixme: This *really* needs to be done by rotating the texture coordinates,
// not by creating an entirely new texture.
// Unfortunately it's in all the wrong place in the rendering code so it
// has to wait for later.
//
//==========================================================================

int BuildTiles::tileCreateRotated(int tileNum)
{
	if ((unsigned)tileNum >= MAXTILES) return tileNum;
	auto tex = tileGetTexture(tileNum);
	if (!tex || tex->GetTexelWidth() <= 0 || tex->GetTexelHeight() <= 0) return tileNum;
	TArray<uint8_t> buffer = tex->GetTexture()->Get8BitPixels(false);
	TArray<uint8_t> dbuffer(tex->GetTexelWidth() * tex->GetTexelHeight(), true);

	auto src = buffer.Data();
	auto dst = dbuffer.Data();

	auto width = tex->GetTexelWidth();
	auto height = tex->GetTexelHeight();
	for (int x = 0; x < width; ++x)
	{
		int xofs = width - x - 1;
		int yofs = height * x;

		for (int y = 0; y < height; ++y)
			*(dst + y * width + xofs) = *(src + y + yofs);
	}

	auto dtex = MakeGameTexture(new FImageTexture(new FLooseTile(dbuffer, tex->GetTexelHeight(), tex->GetTexelWidth())), "", ETextureType::Override);
	int index = findUnusedTile();
	bool mapart = TileFiles.tiledata[tileNum].texture != TileFiles.tiledata[tileNum].backup;
	TileFiles.AddTile(index, dtex, mapart);
	return index;
}

//==========================================================================
//
//
//
//==========================================================================

void BuildTiles::CloseAll()
{
	ArtFiles.DeleteAndClear();
}

//==========================================================================
//
//  Copy a block of a tile.
//  Only used by RR's bowling lane.
//
//==========================================================================

void tileCopySection(int tilenum1, int sx1, int sy1, int xsiz, int ysiz, int tilenum2, int sx2, int sy2)
{
	int xsiz1 = tileWidth(tilenum1);
	int ysiz1 = tileHeight(tilenum1);
	int xsiz2 = tileWidth(tilenum2);
	int ysiz2 = tileHeight(tilenum2);
	if (xsiz1 > 0 && ysiz1 > 0 && xsiz2 > 0 && ysiz2 > 0)
	{
		auto p1 = tilePtr(tilenum1);
		auto p2 = tileData(tilenum2);
		if (p2 == nullptr) return;	// Error: Destination is not writable.
		
		int x1 = sx1;
		int x2 = sx2;
		for (int i=0; i<xsiz; i++)
		{
			int y1 = sy1;
			int y2 = sy2;
			for (int j=0; j<ysiz; j++)
			{
				if (x2 >= 0 && y2 >= 0 && x2 < xsiz2 && y2 < ysiz2)
				{
					auto src = p1[x1 * ysiz1 + y1];
					if (src != TRANSPARENT_INDEX)
						p2[x2 * ysiz2 + y2] = src;
				}
				
				y1++;
				y2++;
				if (y1 >= ysiz1) y1 = 0;
			}
			x1++;
			x2++;
			if (x1 >= xsiz1) x1 = 0;
		}
	}
	TileFiles.InvalidateTile(tilenum2);
}

//==========================================================================
//
//  Check if two tiles are the same
//
//==========================================================================

bool tileEqualTo(int me, int other)
{
	auto tile = tileGetTexture(me);
	auto tile2 = tileGetTexture(other);
	int tilew1 = tile->GetTexelWidth();
	int tileh1 = tile->GetTexelHeight();
	int tilew2 = tile2->GetTexelWidth();
	int tileh2 = tile2->GetTexelHeight();
	if (tilew1 == tilew2 && tileh1 == tileh2)
	{
		auto p1 = tileRawData(me);
		auto p2 = tileRawData(other);
		if (p1 && p2 && !memcmp(p1, p2, tilew1 * tileh2))
			return true;
	}
	return false;
}

//===========================================================================
// 
//	Picks a texture for rendering for a given tilenum/palette combination
//
//===========================================================================

void tileUpdateAnimations()
{
	for (int i = 0; i < MAXTILES; i++)
	{
		if (TileFiles.tiledata[i].picanm.sf & PICANM_ANIMTYPE_MASK)
		{
			int j = i + animateoffs(i, 0);

			auto id1 = TileFiles.tiledata[i].texture->GetID();
			auto id2 = TileFiles.tiledata[j].texture->GetID();
			TexMan.SetTranslation(id1, id2);
		}
	}
}

//===========================================================================
// 
//	Parsing stuff for tile data comes below.
//
//===========================================================================

//===========================================================================
// 
//	Helpers for tile parsing
//
//===========================================================================

bool ValidateTileRange(const char* cmd, int &begin, int& end, FScriptPosition pos, bool allowswap)
{
	if (end < begin)
	{
		pos.Message(MSG_WARNING, "%s: tile range [%d..%d] is backwards. Indices were swapped.", cmd, begin, end);
		std::swap(begin, end);
	}

	if ((unsigned)begin >= MAXUSERTILES || (unsigned)end >= MAXUSERTILES)
	{
		pos.Message(MSG_ERROR, "%s: Invalid tile range [%d..%d]", cmd, begin, end);
		return false;
	}

	return true;
}

bool ValidateTilenum(const char* cmd, int tile, FScriptPosition pos)
{
	if ((unsigned)tile >= MAXUSERTILES)
	{
		pos.Message(MSG_ERROR, "%s: Invalid tile number %d", cmd, tile);
		return false;
	}

	return true;
}

//===========================================================================
// 
//	Internal worker for tileImportTexture
//
//===========================================================================

void processTileImport(const char *cmd, FScriptPosition& pos, TileImport& imp)
{
	if (!ValidateTilenum(cmd, imp.tile, pos))
		return;

	if (imp.crc32 != INT64_MAX && int(imp.crc32) != tileGetCRC32(imp.tile))
		return;

	if (imp.sizex != INT_MAX && tileWidth(imp.tile) != imp.sizex && tileHeight(imp.tile) != imp.sizey)
		return;

	imp.alphacut = clamp(imp.alphacut, 0, 255);

	gi->SetTileProps(imp.tile, imp.surface, imp.vox, imp.shade);

	if (imp.fn.IsNotEmpty() && tileImportFromTexture(imp.fn, imp.tile, imp.alphacut, imp.istexture) < 0) return;

	TileFiles.tiledata[imp.tile].picanm.sf |= imp.flags;
	// This is not quite the same as originally, for two reasons:
	// 1: Since these are texture properties now, there's no need to clear them.
	// 2: The original code assumed that an imported texture cannot have an offset. But this can import Doom patches and PNGs with grAb, so the situation is very different.
	if (imp.xoffset == INT_MAX) imp.xoffset = tileLeftOffset(imp.tile);
	else imp.xoffset = clamp(imp.xoffset, -128, 127);
	if (imp.yoffset == INT_MAX) imp.yoffset = tileTopOffset(imp.tile);
	else imp.yoffset = clamp(imp.yoffset, -128, 127);

	auto tex = tileGetTexture(imp.tile);
	if (tex)
	{
		tex->SetOffsets(imp.xoffset, imp.yoffset);
		if (imp.flags & PICANM_NOFULLBRIGHT_BIT)
		{
			tex->SetDisableBrightmap();
		}
	}
	if (imp.extra != INT_MAX) TileFiles.tiledata[imp.tile].picanm.extra = imp.extra;
}

//===========================================================================
// 
//	Internal worker for tileSetAnim
//
//===========================================================================

void processSetAnim(const char* cmd, FScriptPosition& pos, SetAnim& imp) 
{
	if (!ValidateTilenum(cmd, imp.tile1, pos) ||
	    !ValidateTilenum(cmd, imp.tile2, pos))
		return;

	if (imp.type < 0 || imp.type > 3)
	{
		pos.Message(MSG_ERROR, "%s: animation type must be 0-3, got %d", cmd, imp.type);
		return;
	}

	int count = imp.tile2 - imp.tile1;
	if (imp.type == (PICANM_ANIMTYPE_BACK >> PICANM_ANIMTYPE_SHIFT) && imp.tile1 > imp.tile2)
		count = -count;

	TileFiles.setAnim(imp.tile1, imp.type, imp.speed, count);
}

PicAnm picanm;

#if 0 // this only gets in if unavoidable. It'd be preferable if the script side can solely operate on texture names.
#include "vm.h"

static int GetTexture(int tile, int anim)
{
	if (tile < 0 || tile >= MAXTILES) return 0;
	auto tex = tileGetTexture(tile, anim);
	return tex ? tex->GetID().GetIndex() : 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_TileFiles, GetTexture, GetTexture)
{
	PARAM_PROLOGUE;
	PARAM_INT(tile);
	PARAM_BOOL(animate);
	ACTION_RETURN_INT(GetTexture(tile, animate));
}
#endif


