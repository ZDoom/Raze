/*
** fontchars.cpp
** Texture class for font characters
**
**---------------------------------------------------------------------------
** Copyright 2004-2006 Randy Heit
** Copyright 2006-2018 Christoph Oelckers
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

#include "filesystem.h"
#include "bitmap.h"
#include "image.h"
#include "imagehelpers.h"
#include "fontchars.h"
#include "printf.h"

//==========================================================================
//
// FFontChar1 :: FFontChar1
//
// Used by fonts made from textures.
//
//==========================================================================

FFontChar1::FFontChar1 (FTexture *sourcelump)
: BaseTexture(sourcelump), SourceRemap (nullptr)
{
	// now copy all the properties from the base texture
	assert(BaseTexture != nullptr);
	CopySize(BaseTexture);
}

//==========================================================================
//
// FFontChar1 :: GetPixels
//
// Render style is not relevant for fonts. This must not use it!
//
//==========================================================================

void  FFontChar1::Create8BitPixels (uint8_t *data)
{
	// Make the texture as normal, then remap it so that all the colors
	// are at the low end of the palette
	// Why? It only creates unnecessary work!
	BaseTexture->Create8BitPixels(data);

	if (SourceRemap)
	{
		for (int x = 0; x < GetWidth() * GetHeight(); ++x)
		{
			data[x] = SourceRemap[data[x]];
		}
	}
}

//==========================================================================
//
// FFontChar2 :: FFontChar2
//
// Used by FON1 and FON2 fonts.
//
//==========================================================================

FFontChar2::FFontChar2 (TArray<uint8_t>& sourcelump, int sourcepos, int width, int height, int leftofs, int topofs)
: sourceData (sourcelump), SourcePos (sourcepos), SourceRemap(nullptr)
{
	Size.x = width;
	Size.y = height;
	PicAnim.xofs = leftofs;
	PicAnim.yofs = topofs;
}

//==========================================================================
//
// FFontChar2 :: SetSourceRemap
//
//==========================================================================

void FFontChar2::SetSourceRemap(const uint8_t *sourceremap)
{
	SourceRemap = sourceremap;
}

//==========================================================================
//
// FFontChar2 :: Get8BitPixels
//
// Like for FontChar1, the render style has no relevance here as well.
//
//==========================================================================

void FFontChar2::Create8BitPixels(uint8_t *Pixels)
{
	FileReader lump;
	lump.OpenMemory(sourceData.Data(), sourceData.Size());
	int destSize = GetWidth() * GetHeight();
	uint8_t max = 255;
	bool rle = true;

	// This is to "fix" bad fonts
	{
		uint8_t buff[16];
		lump.Read (buff, 4);
		if (buff[3] == '2')
		{
			lump.Read (buff, 7);
			max = buff[6];
			lump.Seek (SourcePos - 11, FileReader::SeekCur);
		}
		else if (buff[3] == 0x1A)
		{
			lump.Read(buff, 13);
			max = buff[12] - 1;
			lump.Seek (SourcePos - 17, FileReader::SeekCur);
			rle = false;
		}
		else
		{
			lump.Seek (SourcePos - 4, FileReader::SeekCur);
		}
	}

	int runlen = 0, setlen = 0;
	uint8_t setval = 0;  // Shut up, GCC!
	uint8_t *dest_p = Pixels;
	int dest_adv = GetHeight();
	int dest_rew = destSize - 1;

	if (rle)
	{
		for (int y = GetHeight(); y != 0; --y)
		{
			for (int x = GetWidth(); x != 0; )
			{
				if (runlen != 0)
				{
					uint8_t color = lump.ReadUInt8();
					color = std::min(color, max);
					if (SourceRemap != nullptr)
					{
						color = SourceRemap[color];
					}
					*dest_p = color;
					dest_p += dest_adv;
					x--;
					runlen--;
				}
				else if (setlen != 0)
				{
					*dest_p = setval;
					dest_p += dest_adv;
					x--;
					setlen--;
				}
				else
				{
					int8_t code = lump.ReadInt8();
					if (code >= 0)
					{
						runlen = code + 1;
					}
					else if (code != -128)
					{
						uint8_t color = lump.ReadUInt8();
						setlen = (-code) + 1;
						setval = std::min(color, max);
						if (SourceRemap != nullptr)
						{
							setval = SourceRemap[setval];
						}
					}
				}
			}
			dest_p -= dest_rew;
		}
	}
	else
	{
		for (int y = GetHeight(); y != 0; --y)
		{
			for (int x = GetWidth(); x != 0; --x)
			{
				uint8_t color = lump.ReadUInt8();
				if (color > max)
				{
					color = max;
				}
				if (SourceRemap != nullptr)
				{
					color = SourceRemap[color];
				}
				*dest_p = color;
				dest_p += dest_adv;
			}
			dest_p -= dest_rew;
		}
	}

	if (destSize < 0)
	{
		I_Error ("The font %s is corrupt", GetName().GetChars());
	}
}

FBitmap FFontChar2::GetBgraBitmap(const PalEntry* remap, int* ptrans)
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
