/*
** texture.cpp
** The base texture class
**
**---------------------------------------------------------------------------
** Copyright 2004-2007 Randy Heit
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

#include "files.h"
#include "templates.h"

#include "bitmap.h"
#include "image.h"
#include "palette.h"
#include "../glbackend/gl_hwtexture.h"

FTexture *CreateBrightmapTexture(FImageSource*);

//==========================================================================
//
//
//
//==========================================================================


// Examines the lump contents to decide what type of texture to create,
// and creates the texture.
FTexture * FTexture::CreateTexture(const char *name)
{
	auto image = FImageSource::GetImage(name);
	if (image != nullptr)
	{
		FTexture *tex = new FImageTexture(image);
		if (tex != nullptr) 
		{
			tex->Name = name;
			return tex;
		}
	}
	return nullptr;
}

//==========================================================================
//
// 
//
//==========================================================================

FTexture::FTexture (const char *name)
{
	Name = name;
}

FTexture::~FTexture ()
{
}

//===========================================================================
//
// FTexture::GetBgraBitmap
//
// Default returns just an empty bitmap. This needs to be overridden by
// any subclass that actually does return a software pixel buffer.
//
//===========================================================================

FBitmap FTexture::GetBgraBitmap(const PalEntry *remap, int *ptrans)
{
	FBitmap bmp;
	bmp.Create(Size.x, Size.y);
	return bmp;
}

//===========================================================================
// 
//	Gets the average color of a texture for use as a sky cap color
//
//===========================================================================

PalEntry FTexture::averageColor(const uint32_t *data, int size, int maxout)
{
	int				i;
	unsigned int	r, g, b;

	// First clear them.
	r = g = b = 0;
	if (size == 0)
	{
		return PalEntry(255, 255, 255);
	}
	for (i = 0; i < size; i++)
	{
		b += BPART(data[i]);
		g += GPART(data[i]);
		r += RPART(data[i]);
	}

	r = r / size;
	g = g / size;
	b = b / size;

	int maxv = std::max(std::max(r, g), b);

	if (maxv && maxout)
	{
		r = uint64_t(r) * maxout / maxv;
		g = uint64_t(g) * maxout / maxv;
		b = uint64_t(b) * maxout / maxv;
	}
	return PalEntry(255, r, g, b);
}

PalEntry FTexture::GetSkyCapColor(bool bottom)
{
	if (!skyColorDone)
	{
		skyColorDone = true;

		FBitmap bitmap = GetBgraBitmap(nullptr);
		int w = bitmap.GetWidth();
		int h = bitmap.GetHeight();

		const uint32_t *buffer = (const uint32_t *)bitmap.GetPixels();
		if (buffer)
		{
			CeilingSkyColor = averageColor((uint32_t *)buffer, w * std::min(30, h), 0);
			if (h>30)
			{
				FloorSkyColor = averageColor(((uint32_t *)buffer) + (h - 30)*w, w * 30, 0);
			}
			else FloorSkyColor = CeilingSkyColor;
		}
	}
	return bottom ? FloorSkyColor : CeilingSkyColor;
}


//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

void FTexture::CheckTrans(unsigned char * buffer, int size, int trans)
{
	if (bTranslucent == -1)
	{
		bTranslucent = trans;
		if (trans == -1)
		{
			uint32_t * dwbuf = (uint32_t*)buffer;
			for (int i = 0; i<size; i++)
			{
				uint32_t alpha = dwbuf[i] >> 24;

				if (alpha != 0xff && alpha != 0)
				{
					bTranslucent = 1;
					return;
				}
			}
			bTranslucent = 0;
		}
	}
}


//===========================================================================
// 
// smooth the edges of transparent fields in the texture
//
//===========================================================================

#ifdef WORDS_BIGENDIAN
#define MSB 0
#define SOME_MASK 0xffffff00
#else
#define MSB 3
#define SOME_MASK 0x00ffffff
#endif

#define CHKPIX(ofs) (l1[(ofs)*4+MSB]==255 ? (( ((uint32_t*)l1)[0] = ((uint32_t*)l1)[ofs]&SOME_MASK), trans=true ) : false)

int FTexture::SmoothEdges(unsigned char * buffer, int w, int h)
{
	int x, y;
	int trans = buffer[MSB] == 0; // If I set this to false here the code won't detect textures 
								   // that only contain transparent pixels.
	int semitrans = false;
	unsigned char * l1;

	if (h <= 1 || w <= 1) return false;  // makes (a) no sense and (b) doesn't work with this code!

	l1 = buffer;


	if (l1[MSB] == 0 && !CHKPIX(1)) CHKPIX(w);
	else if (l1[MSB]<255) semitrans = true;
	l1 += 4;
	for (x = 1; x<w - 1; x++, l1 += 4)
	{
		if (l1[MSB] == 0 && !CHKPIX(-1) && !CHKPIX(1)) CHKPIX(w);
		else if (l1[MSB]<255) semitrans = true;
	}
	if (l1[MSB] == 0 && !CHKPIX(-1)) CHKPIX(w);
	else if (l1[MSB]<255) semitrans = true;
	l1 += 4;

	for (y = 1; y<h - 1; y++)
	{
		if (l1[MSB] == 0 && !CHKPIX(-w) && !CHKPIX(1)) CHKPIX(w);
		else if (l1[MSB]<255) semitrans = true;
		l1 += 4;
		for (x = 1; x<w - 1; x++, l1 += 4)
		{
			if (l1[MSB] == 0 && !CHKPIX(-w) && !CHKPIX(-1) && !CHKPIX(1) && !CHKPIX(-w - 1) && !CHKPIX(-w + 1) && !CHKPIX(w - 1) && !CHKPIX(w + 1)) CHKPIX(w);
			else if (l1[MSB]<255) semitrans = true;
		}
		if (l1[MSB] == 0 && !CHKPIX(-w) && !CHKPIX(-1)) CHKPIX(w);
		else if (l1[MSB]<255) semitrans = true;
		l1 += 4;
	}

	if (l1[MSB] == 0 && !CHKPIX(-w)) CHKPIX(1);
	else if (l1[MSB]<255) semitrans = true;
	l1 += 4;
	for (x = 1; x<w - 1; x++, l1 += 4)
	{
		if (l1[MSB] == 0 && !CHKPIX(-w) && !CHKPIX(-1)) CHKPIX(1);
		else if (l1[MSB]<255) semitrans = true;
	}
	if (l1[MSB] == 0 && !CHKPIX(-w)) CHKPIX(-1);
	else if (l1[MSB]<255) semitrans = true;

	return trans || (semitrans << 1);
}

//===========================================================================
// 
// Post-process the texture data after the buffer has been created
//
//===========================================================================

bool FTexture::ProcessData(unsigned char * buffer, int w, int h, bool ispatch)
{
	if (bMasked)
	{
		int res = SmoothEdges(buffer, w, h);
		bMasked = !!(res & 1);
	}
	return true;
}

//===========================================================================
// 
//	Initializes the buffer for the texture data
//
//===========================================================================

FTextureBuffer FTexture::CreateTexBuffer(const PalEntry * remap, int flags)
{
	FTextureBuffer result;

	unsigned char * buffer = nullptr;
	int W, H;
	int isTransparent = -1;
	bool checkonly = !!(flags & CTF_CheckOnly);

	W = GetWidth();
	H = GetHeight();

	if (!checkonly)
	{
		buffer = new unsigned char[W*(H + 1) * 4];
		memset(buffer, 0, W * (H + 1) * 4);

		FBitmap bmp(buffer, W * 4, W, H);

		int trans;
		auto Pixels = GetBgraBitmap(remap, &trans);
		bmp.Blit(0, 0, Pixels);

		if (remap == nullptr)
		{
			CheckTrans(buffer, W*H, trans);
			isTransparent = bTranslucent;
		}
		else
		{
			isTransparent = 0;
			// A translated image is not conclusive for setting the texture's transparency info.
		}
	}

	result.mBuffer = buffer;
	result.mWidth = W;
	result.mHeight = H;

	// Only do postprocessing for image-backed textures. (i.e. not for the burn texture which can also pass through here.)
	if (flags & CTF_ProcessData)
	{
		if (!checkonly) ProcessData(result.mBuffer, result.mWidth, result.mHeight, false);
	}

	return result;
}

//===========================================================================
// 
// Dummy texture for the 0-entry.
//
//===========================================================================

bool FTexture::GetTranslucency()
{
	if (bTranslucent == -1)
	{
		if (true)//!bHasCanvas)
		{
			// This will calculate all we need, so just discard the result.
			CreateTexBuffer(0);
		}
		/*
		else
		{
			bTranslucent = 0;
		}*/
	}
	return !!bTranslucent;
}

//===========================================================================
// 
// the default just returns an empty texture.
//
//===========================================================================

const uint8_t* FTexture::Get8BitPixels()
{
	return nullptr;	// most textures do not provide a static buffer.
}

void FTexture::Create8BitPixels(uint8_t *buffer)
{
	// The base class does not fill the texture.
}

//===========================================================================
//
// Replacement textures
//
//===========================================================================

void FTexture::AddReplacement(const HightileReplacement & replace)
{
	for (auto &ht : Hightiles)
	{
		if (replace.palnum == ht.palnum && (replace.faces[1] == nullptr) == (ht.faces[1] == nullptr))
		{
			ht = replace;
			return;
		}
	}
	Hightiles.Push(replace);
}

void FTexture::DeleteReplacement(int palnum)
{
	for (int i = Hightiles.Size() -1; i >= 0; i--)
	{
		if (Hightiles[i].palnum == palnum) Hightiles.Delete(i);
	}
}

//===========================================================================
//
//
//
//===========================================================================

HightileReplacement *FTexture::FindReplacement(int palnum, bool skybox)
{
	for(;;)
    {
		for (auto &rep : Hightiles)
		{
			if (rep.palnum == palnum && (rep.faces[1] != nullptr) == skybox) return &rep;
		}
        if (!palnum || palnum >= MAXPALOOKUPS - RESERVEDPALS) break;
        palnum = 0;
    }
    return nullptr;	// no replacement found
}

//===========================================================================
//
//
//
//===========================================================================

void FTexture::DeleteHardwareTextures()
{
	decltype(HardwareTextures)::Iterator it(HardwareTextures);
	decltype(HardwareTextures)::Pair *pair;
	while (it.NextPair(pair))
	{
		delete pair->Value;
	}
	HardwareTextures.Clear();
}
