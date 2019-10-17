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
#include "textures.h"
#include "image.h"
#include "cache1d.h"
#include "baselayer.h"
#include "palette.h"
#include "m_crc32.h"
#include "build.h"

enum
{
	MAXARTFILES_BASE = 200,
	MAXARTFILES_TOTAL = 220
};

extern char* palookup[];

BuildFiles TileFiles;

//==========================================================================
//
// 
//
//==========================================================================

picanm_t tileConvertAnimFormat(int32_t const picanimraw)
{
	// Unpack a 4 byte packed anim descriptor into the internal 5 byte format.
	picanm_t anm;
	anm.num = picanimraw & 63;
	anm.xofs = (picanimraw >> 8) & 255;
	anm.yofs = (picanimraw >> 16) & 255;
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

FBitmap FTileTexture::GetBgraBitmap(PalEntry* remap, int* ptrans)
{
	FBitmap bmp;
	TArray<uint8_t> buffer;
	bmp.Create(Size.x, Size.y);
	const uint8_t* ppix = Get8BitPixels();
	if (!ppix)
	{
		// This is needed for tiles with a palette remap.
		buffer.Resize(Size.x * Size.y);
		Create8BitPixels(buffer.Data());
		ppix = buffer.Data();
	}
	if (ppix) bmp.CopyPixelData(0, 0, ppix, Size.x, Size.y, Size.y, 1, 0, remap);
	return bmp;
}

void FTileTexture::Create8BitPixels(uint8_t* buffer)
{
	auto pix = Get8BitPixels();
	if (pix) memcpy(buffer, pix, Size.x * Size.y);
}


//==========================================================================
//
// Tile textures are owned by their containing file object.
//
//==========================================================================

FArtTile* GetTileTexture(const char* name, const TArray<uint8_t>& backingstore, uint32_t offset, int width, int height, int picanm)
{
	auto tex = new FArtTile(backingstore, offset, width, height, picanm);
	if (tex)
	{
		tex->SetName(name);
	}
	return tex;
}

//==========================================================================
//
// 
//
//==========================================================================

void BuildFiles::AddTile(int tilenum, FTexture* tex, bool permap)
{
	assert(AllTiles.Find(tex) == AllTiles.Size() && AllMapTiles.Find(tex) == AllMapTiles.Size());
	auto& array = permap ? AllMapTiles : AllTiles;
	array.Push(tex);
	tiles[tilenum] = tex;
	if (!permap) tilesbak[tilenum] = tex;
}

//===========================================================================
//
// AddTiles
//
// Adds all the tiles in an artfile to the texture manager.
//
//===========================================================================

void BuildFiles::AddTiles (int firsttile, TArray<uint8_t>& RawData, bool permap)
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

		auto tex = GetTileTexture("", RawData, uint32_t(tiledata - tiles), width, height, anm);
		AddTile(i, tex);
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
		initprintf("%s: Invalid art file version.  Must be 1, got %d\n", fn, version);
		return 0;
	}

	int tilestart = LittleLong(((uint32_t *)RawData)[2]);
	int tileend = LittleLong(((uint32_t *)RawData)[3]);

	if ((unsigned)tilestart >= MAXUSERTILES || (unsigned)tileend >= MAXUSERTILES)
	{
		initprintf("%s: Invalid tilestart or tileend\n", fn);
		return 0;
	}
	if (tileend < tilestart)
	{
		initprintf("%s: tileend < tilestart\n", fn);
		return 0;
	}

	return tileend >= tilestart ? tileend - tilestart + 1 : 0;
}

//===========================================================================
//
// CloseAllMapArt
//
// Closes all per-map ART files
//
//===========================================================================

void BuildFiles::CloseAllMapArt()
{
	AllMapTiles.DeleteAndClear();
	PerMapArtFiles.DeleteAndClear();
}

//===========================================================================
//
// LoadArtFile
//
// Returns the number of tiles found.
//
// let's load everything into memory on startup.
// Even for Ion Fury this will merely add 80 MB, because the engine already needs to cache the data, albeit in a compressed-per-lump form,
// so its 100MB art file will only have a partial impact on memory.
//
//===========================================================================

int BuildFiles::LoadArtFile(const char *fn, bool mapart, int firsttile)
{
	auto old = FindFile(fn);
	if (old >= ArtFiles.Size())	// Do not process if already loaded.
	{
		FileReader fr = kopenFileReader(fn, 0);
		if (fr.isOpen())
		{
			auto artdata = fr.Read();
			const uint8_t *artptr = artdata.Data();
			if (artdata.Size() > 16)
			{
				if (memcmp(artptr, "BUILDART", 8) == 0) artptr += 8;
				// Only load the data if the header is present
				if (CountTiles(fn, artptr) > 0)
				{
					auto& descs = mapart ? PerMapArtFiles : ArtFiles;
					auto file = new BuildArtFile;
					descs.Push(file);
					file->filename = fn;
					file->RawData = std::move(artdata);
					AddTiles(firsttile, file->RawData, mapart);
				}
			}
		}
		else
		{
			//initprintf("%s: file not found\n", fn);
			return -1;
		}
	}
	else
	{
		// Reuse the old one but move it to the top. (better not.)
		//auto fd = std::move(ArtFiles[old]);
		//ArtFiles.Delete(old);
		//ArtFiles.Push(std::move(fd));
	}
	return 0;
}

//==========================================================================
//
// 
//
//==========================================================================

void BuildFiles::LoadArtSet(const char* filename)
{
	for (int index = 0; index < MAXARTFILES_BASE; index++)
	{
		FStringf fn(filename, index);
		LoadArtFile(fn, false);
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

FTexture* BuildFiles::ValidateCustomTile(int tilenum, int type)
{
	if (tilenum < 0 || tilenum >= MAXTILES) return nullptr;
	if (tiles[tilenum] != tilesbak[tilenum]) return nullptr;	// no mucking around with map tiles.
	auto tile = tiles[tilenum];
	if (tile && tile->GetUseType() == type) return tile;		// already created
	if (tile->GetUseType() > FTexture::Art) return nullptr;		// different custom type - cannot replace again.
	FTexture* replacement = nullptr;
	if (type == FTexture::Writable)
	{
		// Creates an empty writable tile.
		// Current use cases are:
		// Camera textures (should be made to be creatable by the hardware renderer instead of falling back on the software renderer.)
		// thumbnails for savegame and loadgame (should bypass the texture manager entirely.)
		// view tilting in the software renderer (this should just use a local buffer instead of relying on the texture manager.)
		// Movie playback (like thumbnails this should bypass the texture manager entirely.)
		// Blood's 'lens' effect (apparently MP only) - combination of a camera texture with a distortion map - should be made a shader effect to be applied to the camera texture.
		replacement = new FWritableTile;
	}
	else if (type == FTexture::Restorable)
	{
		// This is for modifying an existing tile.
		// It only gets used for the crosshair and two specific effects:
		// A) the fire in Blood.
		// B) the pin display in Redneck Rampage's bowling lanes.
		// All of these effects should probably be redone without actual texture hacking...
		if (tile->GetWidth() == 0 || tile->GetHeight() == 0) return nullptr;	// The base must have a size for this to work.
		// todo: invalidate hardware textures for tile.
		replacement = new FRestorableTile(tile);
	}
	else return nullptr;
	AddTile(tilenum, replacement);
	return replacement;
}

//==========================================================================
//
//  global interface
//
//==========================================================================

int32_t BuildFiles::artLoadFiles(const char* filename)
{
	TileFiles.LoadArtSet(filename);
	memset(gotpic, 0, sizeof(gotpic));
	cacheInitBuffer(MAXCACHE1DSIZE);
	return 0;
}

//==========================================================================
//
// Creates a tile for displaying custom content
//
//==========================================================================

uint8_t* BuildFiles::tileCreate(int tilenum, int width, int height)
{
	if (width <= 0 || height <= 0) return nullptr;
	auto tex = ValidateCustomTile(tilenum, FTexture::Writable);
	if (tex == nullptr) return nullptr;
	auto wtex = static_cast<FWritableTile*>(tex);
	if (!wtex->Resize(width, height)) return nullptr;
	return tex->GetWritableBuffer();
}

//==========================================================================
//
// Makes a tile writable - only used for a handful of special cases
// (todo: Investigate how to get rid of this)
//
//==========================================================================

uint8_t * BuildFiles::tileMakeWritable(int num)
{
	auto tex = ValidateCustomTile(num, FTexture::Restorable);
	return tex ? tex->GetWritableBuffer() : nullptr;
}

//==========================================================================
//
// Sets user content for a tile.
// This must copy the buffer to make sure that the renderer has the data, 
// even if processing is deferred.
//
// Only used by the movie players.
// 
//==========================================================================

void BuildFiles::tileSetExternal(int tilenum, int width, int height, uint8_t* data)
{
	uint8_t* buffer = tileCreate(tilenum, width, height);
	if (buffer) memcpy(buffer, data, width * height);
}

//==========================================================================
//
// Returns checksum for a given tile
//
//==========================================================================

int32_t tileCRC(int tileNum)
{
	if ((unsigned)tileNum >= (unsigned)MAXTILES) return 0;
	auto tile = TileFiles.tiles[tileNum];
	if (!tile ||tile->GetUseType() != FTexture::Art) return 0;	// only consider original ART tiles.
	auto pixels = tile->Get8BitPixels();
	if (!pixels) return 0;
	int size = tile->GetWidth() * tile->GetHeight();
	if (size == 0) return 0;
	return crc32(0, (const Bytef*)pixels, size);
}


//==========================================================================
//
// Import a tile from an external image.
// This has been signifcantly altered so it may not cover everything yet.
//
//==========================================================================

int tileImportFromTexture(const char* fn, int tilenum, int alphacut, int istexture)
{
	FTexture* tex = TileFiles.GetTexture(fn);
	if (tex == nullptr) return -1;
	tex->alphaThreshold = 255 - alphacut;

	int32_t xsiz = tex->GetWidth(), ysiz = tex->GetHeight();

	if (xsiz <= 0 || ysiz <= 0)
		return -2;

	TileFiles.tiles[tilenum] = tex;
#pragma message("tileImportFromTexture needs rework!")	// Reminder so that this place isn't forgotten.
//#if 0
	// Does this make any difference when the texture gets *properly* inserted into the tile array?
	//if (istexture)
		hicsetsubsttex(tilenum, 0, fn, (float)(255 - alphacut) * (1.f / 255.f), 1.0f, 1.0f, 1.0, 1.0, 0);	// At the moment this is the only way to load the texture. The texture creation code is not ready yet for downconverting an image.
//#endif
	return 0;

}

//==========================================================================
//
// Copies a tile into another and optionally translates its palette.
//
//==========================================================================

void tileCopy(int tile, int source, int pal, int xoffset, int yoffset, int flags)
{
	// Todo. Since I do not know if some mod needs this it's of low priority now.
	// Let's get things working first.
	picanm_t* picanm = nullptr;
	picanm_t* sourceanm = nullptr;

	if (pal == -1 && tile == source)
	{
		// Only modify the picanm info.
		FTexture* tex = TileFiles.tiles[tile];
		if (!tex) return;
		picanm = &tex->PicAnim;
		sourceanm = picanm;
	}
	else
	{
		if (source == -1) source = tile;
		FTexture* tex = TileFiles.tiles[source];
		if (!tex) return;
		sourceanm = &tex->PicAnim;

		TArray<uint8_t> buffer(tex->GetWidth() * tex->GetHeight(), true);
		tex->Create8BitPixels(buffer.Data());

		if (pal != -1)
		{
			auto remap = palookup[pal];
			for (auto& pixel : buffer)
			{
				pixel = palookup[pal][pixel];
			}
		}
		tex = new FLooseTile(buffer, tex->GetWidth(), tex->GetHeight());
		picanm = &tex->PicAnim;
		TileFiles.AddTile(tile, tex);
	}

	picanm->xofs = xoffset != -1024 ? clamp(xoffset, -128, 127) : sourceanm->xofs;
	picanm->yofs = yoffset != -1024 ? clamp(yoffset, -128, 127) : sourceanm->yofs;
	picanm->sf = (picanm->sf & ~PICANM_MISC_MASK) | (sourceanm->sf & PICANM_MISC_MASK) | flags;
}

//==========================================================================
//
// Clear map specific ART
//
//==========================================================================

void artClearMapArt(void)
{
	TileFiles.CloseAllMapArt();
	memcpy(TileFiles.tiles, TileFiles.tilesbak, sizeof(TileFiles.tiles));
}

//==========================================================================
//
// Load map specfici ART
//
//==========================================================================

void artSetupMapArt(const char* filename)
{
	artClearMapArt();

	FStringf firstname("%s_00.art", filename);
	auto fr = kopenFileReader(firstname, 0);
	if (!fr.isOpen()) return;

	for (bssize_t i = 0; i < MAXARTFILES_TOTAL - MAXARTFILES_BASE; i++)
	{
		FStringf fullname("%s_%02d.art", filename, i);
		TileFiles.LoadArtFile(fullname, true);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void tileDelete(int tile)
{
	TileFiles.tiles[tile] = TileFiles.tilesbak[tile] = TileFiles.Placeholder;
	vox_undefine(tile);
	md_undefinetile(tile);
	for (ssize_t i = MAXPALOOKUPS - 1; i >= 0; --i)
		hicclearsubst(tile, i);

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
		auto dtile = new FDummyTile(width, height);
		TileFiles.AddTile(tile, dtile);
	}
}

//==========================================================================
//
//
//
//==========================================================================

bool tileLoad(int tileNum)
{
	if ((unsigned)tileNum >= MAXTILES) return false;
	auto tex = TileFiles.tiles[tileNum];
	if (!tex || tex->GetWidth() <= 0 || tex->GetHeight() <= 0) return false;
	if (tex->Get8BitPixels()) return true;
	tex->CacheLock = 199;

	if (!tex->CacheHandle)
	{
		// Allocate storage if necessary.
		int size = tex->GetWidth() * tex->GetHeight();
		cacheAllocateBlock(&tex->CacheHandle, size, &tex->CacheLock);
		tex->Create8BitPixels((uint8_t*)tex->CacheHandle);
	}
	return true;
	}


//==========================================================================
//
//
//
//==========================================================================

int BuildFiles::findUnusedTile(void)
{
	static int lastUnusedTile = MAXUSERTILES - 1;

	for (; lastUnusedTile >= 0; --lastUnusedTile)
	{
		auto tex = TileFiles.tiles[lastUnusedTile];
		if (!tex || tex->GetWidth() <= 0 || tex->GetHeight() <= 0) return lastUnusedTile;
	}
	return -1;
}

int BuildFiles::tileCreateRotated(int tileNum)
{
	if ((unsigned)tileNum >= MAXTILES) return tileNum;
	auto tex = TileFiles.tiles[tileNum];
	if (!tex || tex->GetWidth() <= 0 || tex->GetHeight() <= 0) return tileNum;
	TArray<uint8_t> buffer(tex->GetWidth() * tex->GetHeight(), true);
	tex->Create8BitPixels(buffer.Data());
	TArray<uint8_t> dbuffer(tex->GetWidth() * tex->GetHeight(), true);

	auto src = buffer.Data();
	auto dst = dbuffer.Data();

	// the engine has a squarerotatetile() we could call, but it mirrors at the same time
	auto siz = tex->GetSize();
	for (int x = 0; x < siz.x; ++x)
	{
		int xofs = siz.x - x - 1;
		int yofs = siz.y * x;

		for (int y = 0; y < siz.y; ++y)
			*(dst + y * siz.x + xofs) = *(src + y + yofs);
	}

	auto dtex = new FLooseTile(dbuffer, tex->GetHeight(), tex->GetWidth());
	int index = findUnusedTile();
	bool mapart = TileFiles.tiles[tileNum] != TileFiles.tilesbak[tileNum];
	TileFiles.AddTile(index, dtex, mapart);
	return index;
}

void tileSetAnim(int tile, const picanm_t& anm)
{

}

//==========================================================================
//
//
//
//==========================================================================

FTexture* BuildFiles::GetTexture(const char* path)
{
	auto res = textures.CheckKey(path);
	if (res) return *res;
	auto tex = FTexture::CreateTexture(path);
	if (tex) textures.Insert(path, tex);
	return tex;
}

//==========================================================================
//
//
//
//==========================================================================

void BuildFiles::CloseAll()
{
	decltype(textures)::Iterator it(textures);
	decltype(textures)::Pair* pair;
	while (it.NextPair(pair)) delete pair->Value;
	textures.Clear();
	CloseAllMapArt();
	ArtFiles.DeleteAndClear();
	AllTiles.DeleteAndClear();
	if (Placeholder) delete Placeholder;
	Placeholder = nullptr;
}


TileSiz tilesiz;
PicAnm picanm;
PicSiz picsiz;
