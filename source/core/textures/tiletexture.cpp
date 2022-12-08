/*
** tiletexture.cpp
** Image sources and file management for Build tile textures
**
**---------------------------------------------------------------------------
** Copyright 2019-2022 Christoph Oelckers
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
#include "i_time.h"
#include "zstring.h"
#include "tiletexture.h"
#include "texturemanager.h"
#include "filesystem.h"
#include "printf.h"


class FTileTexture : public FImageSource
{
public:
	FTileTexture() noexcept
	{
		bUseGamePalette = true;
		bTranslucent = false;
	}
	virtual uint8_t* GetRawData() = 0;
	virtual TArray<uint8_t> CreatePalettedPixels(int conversion);
	virtual int CopyPixels(FBitmap* bmp, int conversion);			// This will always ignore 'luminance'.
};

//==========================================================================
//
// A tile coming from an ART file.
//
//==========================================================================

class FArtTile : public FTileTexture
{
	const TArray<uint8_t>& RawPixels;
	const uint32_t Offset;
public:
	FArtTile(const TArray<uint8_t>& backingstore, uint32_t offset, int width, int height) noexcept
		: RawPixels(backingstore), Offset(offset)
	{
		Width = width;
		Height = height;
	}
	uint8_t* GetRawData() override final
	{
		return &RawPixels[Offset];
	}
};

//==========================================================================
//
// A non-existent tile
//
//==========================================================================

class FDummyTile : public FTileTexture
{
public:
	FDummyTile(int width, int height)
	{
		Width = width;
		Height = height;
	}

	uint8_t* GetRawData() override
	{
		return nullptr;
	}
};

//==========================================================================
//
// A tile with a writable surface
//
//==========================================================================

class FWritableTile : public FTileTexture
{
protected:
	TArray<uint8_t> buffer;

public:
	FWritableTile()
	{
		//useType = Writable;
	}

	uint8_t* GetRawData() override
	{
		return buffer.Data();
	}

	bool ResizeImage(int w, int h)
	{
		if (w <= 0 || h <= 0)
		{
			buffer.Reset();
			return false;
		}
		else
		{
			Width = w;
			Height = h;
			buffer.Resize(w * h);
			return true;
		}
	}

	virtual void Reload() {}

};

//==========================================================================
//
// A tile with a writable surface
//
//==========================================================================

class FRestorableTile : public FWritableTile
{
	FImageSource* Base;

public:
	FRestorableTile(FImageSource* base)
	{
		Base = base;

		CopySize(*base);
		ResizeImage(Width, Height);
		Reload();
	}

	void Reload() override
	{
		buffer = Base->GetPalettedPixels(0);
	}
};


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
// raw pixel cache. This is for accessing pixel data in the game code,
// not for rendering.
//
//==========================================================================

struct RawCacheNode
{
	TArray<uint8_t> data;
	uint64_t lastUseTime;

	RawCacheNode() = default;
	RawCacheNode(const RawCacheNode& other) = default;
	RawCacheNode& operator=(const RawCacheNode& other) = default;

	RawCacheNode(RawCacheNode&& other) noexcept
	{
		data = std::move(other.data);
		lastUseTime = other.lastUseTime;
	}

	RawCacheNode& operator=(RawCacheNode&& other) noexcept
	{
		data = std::move(other.data);
		lastUseTime = other.lastUseTime;
		return *this;
	}
};

//==========================================================================
//
// access raw pixel data for in-game checking. Works for all texture types.
//
//==========================================================================
static TMap<int, RawCacheNode> CacheNodes;

const uint8_t* GetRawPixels(FTextureID texid)
{
	if (!texid.isValid()) return nullptr;
	auto gtex = TexMan.GetGameTexture(texid);
	auto tex = dynamic_cast<FImageTexture*>(gtex->GetTexture());

	if (!tex || !tex->GetImage()) return nullptr;
	auto img = tex->GetImage();
	auto timg = dynamic_cast<FTileTexture*>(img);
	if (!timg || !timg->GetRawData())
	{
		auto cache = CacheNodes.CheckKey(texid.GetIndex());
		if (cache)
		{
			cache->lastUseTime = I_nsTime();
			return cache->data.Data();
		}
		RawCacheNode newnode;
		newnode.data = img->GetPalettedPixels(0);
		newnode.lastUseTime = I_nsTime();
		auto retval =newnode.data.Data();
		CacheNodes.Insert(texid.GetIndex(), std::move(newnode));
		return retval;
	}
	else
	{
		return timg->GetRawData();
	}
}

//==========================================================================
//
//To use this the texture must have been made writable during texture init.
//
//==========================================================================

uint8_t* GetWritablePixels(FTextureID texid, bool reload)
{
	if (!texid.isValid()) return nullptr;
	auto gtex = TexMan.GetGameTexture(texid);
	auto tex = dynamic_cast<FImageTexture*>(gtex->GetTexture());

	if (!tex || !tex->GetImage()) return nullptr;
	auto timg = dynamic_cast<FWritableTile*>(tex->GetImage());
	if (!timg) return nullptr;

	if (reload)
		timg->Reload();

	gtex->CleanHardwareData();	// we can safely assume that this only gets called when the texture is about to be changed.
	return timg->GetRawData();
}


//==========================================================================
//
// 
//
//==========================================================================

static FImageSource* GetTileImage(const TArray<uint8_t>& backingstore, uint32_t offset, int width, int height, TArray<void*> freelist)
{
	FImageSource* tex;

	void* mem = nullptr;
	if (freelist.Size() > 0) freelist.Pop(mem);
	// recycle discarded image sources if available. They are all the same type so this is safe.
	if (mem)
	{
		memset(mem, 0, sizeof(FArtTile));
		tex = new(mem) FArtTile(backingstore, offset, width, height);
	}
	else tex = new FArtTile(backingstore, offset, width, height);
	auto p = &backingstore[offset];
	auto siz = width * height;
	for (int i = 0; i < siz; i++, p++)
	{
		// move transparent color to index 0 to get in line with the rest of the texture management.
		if (*p == 0) *p = 255;
		else if (*p == 255) *p = 0;
	}
	return tex;
}

//===========================================================================
//
// Creates image sources for all valid tiles in one ART file.
// In case this overwrites older tiles, these get added to a freelist
// and reused later
//
//===========================================================================

static void GetImagesFromFile(TArray<FImageSource*>& array, TArray<unsigned>& picanmarray, TArray<void*> freelist, TArray<uint8_t>& RawData)
{
	const uint8_t* tiles = RawData.Data();
	//	int numtiles = LittleLong(((uint32_t *)tiles)[1]);	// This value is not reliable
	unsigned tilestart = LittleLong(((unsigned*)tiles)[2]);
	unsigned tileend = LittleLong(((unsigned*)tiles)[3]);
	const uint16_t* tilesizx = &((const uint16_t*)tiles)[8];
	const uint16_t* tilesizy = &tilesizx[tileend - tilestart + 1];
	const uint32_t* picanmraw = (const uint32_t*)&tilesizy[tileend - tilestart + 1];
	const uint8_t* tiledata = (const uint8_t*)&picanmraw[tileend - tilestart + 1];

	unsigned oldsize = array.Size();
	if (array.Size() < tileend + 1)
		array.Resize(tileend + 1);

	// picanm info needs to be stashed aside for later use, except for the offsets which are written into the image object
	if (picanmarray.Size() < tileend + 1)
		picanmarray.Resize(tileend + 1);

	// If there's a gap, initialize it.
	for (unsigned i = oldsize; i < tilestart; i++)
	{
		array[i] = nullptr;
		picanmarray[i] = {};
	}

	for (unsigned i = tilestart; i <= tileend; ++i)
	{
		int pic = i - tilestart;
		int width = LittleShort(tilesizx[pic]);
		int height = LittleShort(tilesizy[pic]);
		uint32_t anm = LittleLong(picanmraw[pic]);
		int size = width * height;

		picanmarray[i] = picanmraw[pic]; // this must also be retained for invalid tiles.
		if (width <= 0 || height <= 0)
		{
			// If an existing tile is discarded, add it to the free list
			if (array[i] && i < oldsize)
			{
				array[i]->~FImageSource();	// really a no-op but let's stay on the safe side.
#if 0
				// recycling items causes corruption so disable for now and revisit later.
				freelist.Push(array[i]);
#endif
			}
			array[i] = nullptr;
			continue;
		}

		FString texname;
		auto tex = GetTileImage(RawData, uint32_t(tiledata - tiles), width, height, freelist);
		int leftoffset = (int8_t)((anm >> 8) & 255);
		int topoffset = (int8_t)((anm >> 16) & 255);
		tex->SetOffsets(leftoffset, topoffset);
		tiledata += size;
		array[i] = tex;
	}
}

//===========================================================================
//
// CountTiles
//
// Returns the number of tiles provided by an artfile
//
//===========================================================================

int CountTiles(const char* fn, const uint8_t* RawData)
{
	int version = LittleLong(*(uint32_t*)RawData);
	if (version != 1)
	{
		Printf("%s: Invalid art file version.  Must be 1, got %d\n", fn, version);
		return 0;
	}

	int tilestart = LittleLong(((uint32_t*)RawData)[2]);
	int tileend = LittleLong(((uint32_t*)RawData)[3]);

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
// LoadArtFile
//
// Returns the number of tiles found.
//
// let's load everything into memory on startup.
//
//===========================================================================

static void AddArtFile(const FString& filename)
{
	FileReader fr = fileSystem.OpenFileReader(filename);
	if (fr.isOpen())
	{
		auto artdata = fr.Read();
		if (artdata.Size() > 16)
		{
			if (memcmp(artdata.Data(), "BUILDART", 8) == 0)
			{
				artdata.Delete(0, 8);
			}
			// Only load the data if the header is present
			if (CountTiles(filename, artdata.Data()) > 0)
			{
				// The texture manager already has a store for Build ART files, so let's use it. :)
				auto& store = TexMan.GetNewBuildTileData();
				store = std::move(artdata);
			}
		}
	}
}

//===========================================================================
//
// Loads an art set but does not do anything with the tiles yet.
//
//===========================================================================

void InitArtFiles(TArray<FString>& addedArt)
{
	const int MAXARTFILES_BASE = 200;

	for (int index = 0; index < MAXARTFILES_BASE; index++)
	{
		FStringf fn("tiles%03d.art", index);
		AddArtFile(fn);
	}
	for (auto& addart : addedArt)
	{
		AddArtFile(addart);
	}
}

//===========================================================================
//
// Creates image sources for all valid tiles in the ART set.
//
//===========================================================================

void GetArtImages(TArray<FImageSource*>& array, TArray<unsigned>& picanm)
{
	TArray<void*> freelist;
	for (auto& f : TexMan.GetBuildTileDataStore())
	{
		GetImagesFromFile(array, picanm, freelist, f);
	}
}

//==========================================================================
//
//
//
//==========================================================================

FImageSource* createDummyTile(int width, int height)
{
	if (width > 0 && height > 0)
		return new FDummyTile(width, height);
	else
		return nullptr;
}

//==========================================================================
//
//
//
//==========================================================================

FImageSource* createWritableTile(int width, int height)
{
	if (width > 0 && height > 0)
	{
		auto tile = new FWritableTile;
		tile->ResizeImage(width, height);
		return tile;
	}
	else
		return nullptr;
}

//==========================================================================
//
//
//
//==========================================================================

FImageSource* makeTileWritable(FImageSource* img)
{
	return new FRestorableTile(img);
}

