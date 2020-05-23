/*
** arttexture.cpp
** Texture class for ART-based hightiles
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
#include "templates.h"
#include "bitmap.h"
#include "image.h"
#include "palettecontainer.h"

#include "build.h"

#if 0
//==========================================================================
//
// an AET texture
//
//==========================================================================

class FArtTexture : public FImageSource
{

public:
	FArtTexture (int width, int height, int p);
	void CreatePalettedPixels(uint8_t* buffer) override;
	int CopyPixels(FBitmap *bmp, int conversion) override;
	int32_t picanmdisk;	// Todo: Get this out again on the other side.
};

//==========================================================================
//
//
//
//==========================================================================

FImageSource *ArtImage_TryCreate(FileReader & file)
{
	auto buffer = file.Read();

	// Cannot load if smaller than the header.
	if (buffer.Size() < ARTv1_UNITOFFSET) return nullptr;
	uint32_t* header = (uint32_t *) buffer.Data();
	int ver = LittleLong(header[0]);
	if (ver != 1) return nullptr;

	// Only allow files with one tile.
	int firsttile = LittleLong(header[1]);
	int lasttile = LittleLong(header[2]);
	if (firsttile != lasttile) return nullptr;

	int32_t picanmdisk = LittleLong(header[5]);
	int Width = LittleShort(B_UNBUF16(&buffer[16]));
	int Height = LittleShort(B_UNBUF16(&buffer[18]));

	if (Width <= 0 || Height <= 0)
	{
		return nullptr;
	}

	int32_t NumPixels = Width * Height;

	if (ARTv1_UNITOFFSET + NumPixels > (int)buffer.Size())
	{
		return nullptr;
	}
	
	return new FArtTexture(Width, Height, picanmdisk);
}

//==========================================================================
//
//
//
//==========================================================================

FArtTexture::FArtTexture(int width, int height, int p)
{
	Width = width;
	Height = height;
	picanmdisk = p;
}

//==========================================================================
//
// This will never be called by the software renderer but let's be safe.
//
//==========================================================================

void FArtTexture::CreatePalettedPixels(uint8_t* buffer)
{
	FileReader fr = fileSystem.OpenFileReader(Name);
	if (!fr.isOpen()) return;
	int numpixels = Width * Height;
	fr.Read(buffer, numpixels);
	auto remap = GPalette.Remap;
	for (int i = 0; i < numpixels; i++)
	{
		buffer[i] = remap[buffer[i]];
	}
}

//===========================================================================
//
// FArtTexture::CopyPixels
//
// This format is special because it ignores the game palettes and
// only outputs a true color image with the primary palette.
//
//===========================================================================

int FArtTexture::CopyPixels(FBitmap *bmp, int conversion)
{
	// Treat both buffer as linear contiguous blocks.
	// Both Src and Dst are ordered the same with no padding.
	int numpixels = Width * Height;
	bool hasalpha = false;
	FileReader fr = fileSystem.OpenFileReader(Name);
	if (!fr.isOpen()) return 0;
	TArray<uint8_t> source(numpixels, true);
	fr.Read(source.Data(), numpixels);
	auto dest = bmp->GetPixels();
	
	auto remap = GPalette.Remap;
	auto pal = GPalette.BaseColors;
    for (int y = 0; y < numpixels; ++y)
    {
		int index = remap[source[y]];
        if (index == TRANSPARENT_INDEX)
		{
			hasalpha = true;
            continue;
		}
		
		dest[0] = pal[index].b;
		dest[1] = pal[index].g;
		dest[2] = pal[index].r;
		dest[3] = 255;
		dest += 4;
    }
	bMasked = hasalpha;
	return 0;
}	 

#endif