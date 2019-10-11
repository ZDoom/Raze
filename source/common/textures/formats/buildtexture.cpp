/*
** buildtexture.cpp
** Handling Build textures (now as a usable editing feature!)
**
**---------------------------------------------------------------------------
** Copyright 2004-2006 Randy Heit
** Copyright 2018 Christoph Oelckers
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

#include <stdint.h>
#include "zstring.h"
#include "files.h"
#include "templates.h"
#include "bitmap.h"
#include "textures/textures.h"
#include "image.h"


//==========================================================================
//
// A texture defined in a Build TILESxxx.ART file
//
//==========================================================================

class FBuildTexture : public FImageSource
{
public:
	FBuildTexture (const FString &pathprefix, int tilenum, const uint8_t *pixels, int width, int height, int left, int top);
	//void CreatePalettedPixels(uint8_t *destbuffer) override;

protected:
	const uint8_t *RawPixels;
};


//==========================================================================
//
//
//
//==========================================================================

FBuildTexture::FBuildTexture(const FString &pathprefix, int tilenum, const uint8_t *pixels, int width, int height, int left, int top)
: RawPixels (pixels)
{
	Width = width;
	Height = height;
	LeftOffset = left;
	TopOffset = top;
}

#if 0
TArray<uint8_t> FBuildTexture::CreatePalettedPixels(int conversion)
{
	TArray<uint8_t> Pixels(Width * Height, true);
	memcpy(Pixels.Data(), RawPixels, Width * Height);
	return Pixels;
}
#endif

#if 0
//===========================================================================
//
// AddTiles
//
// Adds all the tiles in an artfile to the texture manager.
//
//===========================================================================

void FTextureManager::AddTiles (const FString &pathprefix, const void *tiles)
{

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
		//FTextureID texnum;
		FTexture *tex;

		if (width <= 0 || height <= 0) continue;

		tex = new FImageTexture(new FBuildTexture (pathprefix, i, tiledata, width, height, xoffs, yoffs));
		//texnum = AddTexture (tex);
		tiledata += size;
		tex->Name.Format("%sTILE%04d", pathprefix.GetChars(), i);

		// I have no idea if this makes sense or if we better leave animation control to the game code.
		// To be decided later.
#if 0
		if ((picanm[pic] & 63) && (picanm[pic] & 192))
		{
			int type, speed;

			switch (picanm[pic] & 192)
			{
			case 64:	type = 2;	break;
			case 128:	type = 0;	break;
			case 192:	type = 1;	break;
			default:    type = 0;   break;  // Won't happen, but GCC bugs me if I don't put this here.
			}

			speed = (anm >> 24) & 15;
			speed = MAX (1, (1 << speed) * 1000 / 120);	// Convert from 120 Hz to 1000 Hz.

			AddSimpleAnim (texnum, picanm[pic] & 63, type, speed);
		}
#endif

		// Blood's rotation types:
		// 0 - Single
		// 1 - 5 Full
		// 2 - 8 Full
		// 3 - Bounce (looks no different from Single; seems to signal bouncy sprites)
		// 4 - 5 Half (not used in game)
		// 5 - 3 Flat (not used in game)
		// 6 - Voxel
		// 7 - Spin Voxel

#if 0
		int rotType = (anm >> 28) & 7;
		if (rotType == 1)
		{
			spriteframe_t rot;
			rot.Texture[0] =
			rot.Texture[1] = texnum;
			for (int j = 1; j < 4; ++j)
			{
				rot.Texture[j*2] =
				rot.Texture[j*2+1] =
				rot.Texture[16-j*2] =
				rot.Texture[17-j*2] = texnum.GetIndex() + j;
			}
			rot.Texture[8] =
			rot.Texture[9] = texnum.GetIndex() + 4;
			rot.Flip = 0x00FC;
			rot.Voxel = NULL;
			tex->Rotations = SpriteFrames.Push (rot);
		}
		else if (rotType == 2)
		{
			spriteframe_t rot;
			rot.Texture[0] =
			rot.Texture[1] = texnum;
			for (int j = 1; j < 8; ++j)
			{
				rot.Texture[16-j*2] =
				rot.Texture[17-j*2] = texnum.GetIndex() + j;
			}
			rot.Flip = 0;
			rot.Voxel = NULL;
			tex->Rotations = SpriteFrames.Push (rot);
		}
#endif
	}
}

//===========================================================================
//
// CountTiles
//
// Returns the number of tiles provided by an artfile
//
//===========================================================================

static int CountTiles (const void *tiles)
{
	int version = LittleLong(*(uint32_t *)tiles);
	if (version != 1)
	{
		return 0;
	}

	int tilestart = LittleLong(((uint32_t *)tiles)[2]);
	int tileend = LittleLong(((uint32_t *)tiles)[3]);

	return tileend >= tilestart ? tileend - tilestart + 1 : 0;
}

//===========================================================================
//
// R_CountBuildTiles
//
// Returns the number of tiles found. Also loads all the data for
// R_InitBuildTiles() to process later.
//
//===========================================================================

void FTextureManager::InitBuildTiles(TArray<FString> &tileFiles)
{
	int numtiles;
	int totaltiles = 0;

	for (auto &artpath : tileFiles)
	{
		int lumpnum = fileSystem.FindFile(artpath);	
		if (lumpnum >= 0)
		{
			BuildTileData.Reserve(1);
			auto &artdata = BuildTileData.Last();
			artdata = fileSystem.GetFileData(lumpnum);

			if ((numtiles = CountTiles(artdata.Data())) > 0)
			{
				AddTiles("", &artdata[0]);
				totaltiles += numtiles;
			}
		}
	}
}

#endif