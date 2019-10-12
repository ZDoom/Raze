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

struct BuildTileDescriptor
{
	int tilenum;
	FTexture *Texture;
};

struct BuildFileDescriptor
{
	FString filename;
	TArray<uint8_t> RawData;
	TArray<BuildTileDescriptor> Textures;
	bool MapArt;
	bool Active;

	void AddTiles();
};

struct BuildFiles
{
	TArray<BuildFileDescriptor> FileDescriptors;
	void AddFile(BuildFileDescriptor &bfd)
	{
		FileDescriptors.Push(std::move(bfd));
	}
	BuildFileDescriptor *FindFile(const FString &filename)
	{
		auto ndx = FileDescriptors.FindEx([filename](const BuildFileDescriptor & element) { return filename.CompareNoCase(element.filename) == 0; });
		if (ndx < FileDescriptors.Size()) return &FileDescriptors[ndx];
			return nullptr;
	}
	void LoadArtFile(const char *file, bool mapart);
	void CloseAllMapArt();
	int LoadIndexedFile(const char* base, int index);
	void LoadArtSet(const char* filename);

};
			
static BuildFiles alltilefiles;

//===========================================================================
//
// AddTiles
//
// Adds all the tiles in an artfile to the texture manager.
//
//===========================================================================

void BuildFileDescriptor::AddTiles ()
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
		int xoffs = (int8_t)((anm >> 8) & 255) + width/2;
		int yoffs = (int8_t)((anm >> 16) & 255) + height/2;
		int size = width*height;

		if (width <= 0 || height <= 0) continue;

		// This name is mainly for debugging so that there is something more to go by than the mere index.
		FStringf name("TILE_%s_%05d_%08x_%04x_%04x", filename, uint32_t(tiledata - tiles), width, height);
		auto tex = FTexture::GetTileTexture(name, RawData, uint32_t(tiledata - tiles), width, height, xoffs, yoffs);
		BuildTileDescriptor desc;
		//Textures.Push();
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
	for (auto& fd : FileDescriptors)
	{
		if (fd.MapArt)
		{
			fd.Active = false;
			fd.RawData.Reset();
		}
	}
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
	if (old)
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
					FileDescriptors.Reserve(1);
					auto &fd = FileDescriptors.Last();
					fd.filename = fn;
					fd.MapArt = mapart;
					fd.Active = true;
					fd.RawData = std::move(artdata);
					fd.AddTiles();
				}
			}
		}
	}
}

#if 0

//===========================================================================
//
// Returns:
//  0: successfully read ART file
// >0: error with the ART file
// -1: ART file does not exist
//<-1: per-map ART issue
//
//===========================================================================

int BuildFiles::LoadIndexedFile(const char *base, int index)
{
	FStringf name(base, index);

	const char* fn = artGetIndexedFileName(tilefilei);
	const int32_t permap = (tilefilei >= MAXARTFILES_BASE);  // is it a per-map ART file?
	buildvfs_kfd fil;

	if ((fil = kopen4loadfrommod(fn, 0)) != buildvfs_kfd_invalid)
	{
		artheader_t local;
		int const headerval = artReadHeader(fil, fn, &local);
		if (headerval != 0)
		{
			kclose(fil);
			return headerval;
		}

		if (permap)
		{
			// Check whether we can evict existing tiles to make place for
			// per-map ART ones.
			for (int i = local.tilestart; i <= local.tileend; i++)
			{
				// Tiles having dummytile replacements or those that are
				// cache1d-locked can't be replaced.
				if (faketile[i >> 3] & pow2char[i & 7] || walock[i] >= 200)
				{
					initprintf("loadpics: per-map ART file \"%s\": "
						"tile %d has dummytile or is locked\n", fn, i);
					kclose(fil);
					return -3;
				}
			}

			// Free existing tiles from the cache1d. CACHE1D_FREE
			Bmemset(&tileptr[local.tilestart], 0, local.numtiles * sizeof(uint8_t*));
			Bmemset(&tiledata[local.tilestart], 0, local.numtiles * sizeof(uint8_t*));
			Bmemset(&walock[local.tilestart], 1, local.numtiles * sizeof(walock[0]));
		}

		artReadManifest(fil, &local);

		if (cache1d_file_fromzip(fil))
		{
			if (permap)
				artPreloadFileSafe(fil, &local);
			else
				artPreloadFile(fil, &local);
		}
		else
		{
			int offscount = ktell(fil);

			for (bssize_t i = local.tilestart; i <= local.tileend; ++i)
			{
				int const dasiz = tilesiz[i].x * tilesiz[i].y;

				tilefilenum[i] = tilefilei;
				tilefileoffs[i] = offscount;

				offscount += dasiz;
				// artsize += ((dasiz+15)&0xfffffff0);
			}
		}

		kclose(fil);
		return 0;
	}

	return -1;
}

//
// loadpics
//
int32_t BuildFiles::LoadArtSet(const char* filename)
{
	Bstrncpyz(artfilenameformat, filename, sizeof(artfilenameformat));

	Bmemset(&tilesizearray[0], 0, sizeof(vec2_16_t) * MAXTILES);
	Bmemset(picanm, 0, sizeof(picanm));

	for (auto& rot : rottile)
		rot = { -1, -1 };

	//    artsize = 0;

	for (int tilefilei = 0; tilefilei < MAXARTFILES_BASE; tilefilei++)
		artReadIndexedFile(tilefilei);

	Bmemset(gotpic, 0, sizeof(gotpic));

	//cachesize = min((int32_t)((Bgetsysmemsize()/100)*60),max(artsize,askedsize));
	cachesize = (Bgetsysmemsize() <= (uint32_t)askedsize) ? (int32_t)((Bgetsysmemsize() / 100) * 60) : askedsize;
	pic = Xaligned_alloc(Bgetpagesize(), cachesize);
	cacheInitBuffer((intptr_t)pic, cachesize);

	artUpdateManifest();

	artfil = buildvfs_kfd_invalid;
	artfilnum = -1;
	artfilplc = 0L;

	return 0;
}

#endif