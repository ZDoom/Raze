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

enum
{
	MAXARTFILES_BASE = 200,
	MAXARTFILES_TOTAL = 220
};

picanm_t tileConvertAnimFormat(int32_t const picanmdisk)
{
	// Unpack a 4 byte packed anim descriptor into the internal 5 byte format.
	picanm_t anm;
	anm.num = picanmdisk & 63;
	anm.xofs = (picanmdisk >> 8) & 255;
	anm.yofs = (picanmdisk >> 16) & 255;
	anm.sf = ((picanmdisk >> 24) & 15) | (picanmdisk & 192);
	anm.extra = (picanmdisk >> 28) & 15;
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
	bmp.Create(Size.x, Size.y);
	const uint8_t* ppix = Get8BitPixels();	// any properly implemented tile MUST return something valid here.
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

FTexture* GetTileTexture(const char* name, const TArray<uint8_t>& backingstore, uint32_t offset, int width, int height, int picanm)
{
	auto tex = new FArtTile(backingstore, offset, width, height, picanm);
	if (tex)
	{
		tex->SetName(name);
	}
	return tex;
}

//===========================================================================
//
// AddTiles
//
// Adds all the tiles in an artfile to the texture manager.
//
//===========================================================================

void BuildArtFile::AddTiles ()
{

	const uint8_t *tiles = RawData.Data();
//	int numtiles = LittleLong(((uint32_t *)tiles)[1]);	// This value is not reliable
	int tilestart = LittleLong(((uint32_t *)tiles)[2]);
	int tileend = LittleLong(((uint32_t *)tiles)[3]);
	const uint16_t *tilesizx = &((const uint16_t *)tiles)[8];
	const uint16_t *tilesizy = &tilesizx[tileend - tilestart + 1];
	const uint32_t *picanm = (const uint32_t *)&tilesizy[tileend - tilestart + 1];
	const uint8_t *tiledata = (const uint8_t *)&picanm[tileend - tilestart + 1];

	for (int i = tilestart; i <= tileend; ++i)
	{
		int pic = i - tilestart;
		int width = LittleShort(tilesizx[pic]);
		int height = LittleShort(tilesizy[pic]);
		uint32_t anm = LittleLong(picanm[pic]);
		int size = width*height;

		if (width <= 0 || height <= 0) continue;

		// This name is mainly for debugging so that there is something more to go by than the mere index.
		FStringf name("TILE_%s_%05d_%08x_%04x_%04x", filename, uint32_t(tiledata - tiles), width, height);
		auto tex = GetTileTexture(name, RawData, uint32_t(tiledata - tiles), width, height, anm);
		BuildTileDescriptor desc = { i, tex };
		Textures.Push(desc);
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

int CountTiles (const void *RawData)
{
	int version = LittleLong(*(uint32_t *)RawData);
	if (version != 1)
	{
		return 0;
	}

	int tilestart = LittleLong(((uint32_t *)RawData)[2]);
	int tileend = LittleLong(((uint32_t *)RawData)[3]);

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
	PerMapArtFiles.Clear();
}

//===========================================================================
//
// LoadArtFile
//
// Returns the number of tiles found. Also loads all the data for
// R_InitBuildTiles() to process later.
//
// let's load everything into memory on startup.
// Even for Ion Fury this will merely add 60 MB, because the engine already needs to cache the data, albeit in a compressed-per-lump form,
// so its 100MB art file will only have a partial impact on memory.
//
//===========================================================================

void BuildFiles::LoadArtFile(const char *fn, bool mapart)
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
				if (CountTiles(artptr) > 0)
				{
					auto& descs = mapart ? PerMapArtFiles : ArtFiles;
					descs.Reserve(1);
					auto& fd = descs.Last();
					fd.filename = fn;
					fd.RawData = std::move(artdata);
					fd.AddTiles();
				}
			}
		}
	}
	else
	{
		// Reuse the old one but move it to the top.
		auto fd = std::move(ArtFiles[old]);
		ArtFiles.Delete(old);
		ArtFiles.Push(std::move(fd));
	}
}

static FString artGetIndexedFileName(const char *base, int32_t index)
{
	FString result;
	if (index >= MAXARTFILES_BASE)
	{
		char XX[3] = { char('0' + (index / 10) % 10), char('0' + index % 10), 0 };
		result = base;
		result.Substitute("XX", XX);
	}
	else
	{
		result.Format(base, index);
	}
	return result;
}


void BuildFiles::LoadArtSet(const char* filename)
{
	for (int index = 0; index < MAXARTFILES_BASE; index++)
	{
		auto fn = artGetIndexedFileName(filename, index);
		LoadArtFile(fn, false);
	}
}

BuildFiles TileFiles;
