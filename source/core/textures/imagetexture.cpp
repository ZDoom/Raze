/*
** imagetexture.cpp
** Texture class based on FImageSource
**
**---------------------------------------------------------------------------
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

#include "files.h"
#include "templates.h"
#include "bitmap.h"
#include "image.h"
#include "imagehelpers.h"


//==========================================================================
//
//
//
//==========================================================================

FImageTexture::FImageTexture(FImageSource *img, const char *name)
: FTexture(name)
{
	mImage = img;
	if (img != nullptr)
	{
		SetSize(img->GetWidth(), img->GetHeight());

		auto offsets = img->GetOffsets();
		leftoffset = offsets.first;
		topoffset = offsets.second;

		bMasked = img->bMasked;
		bTranslucent = img->bTranslucent;
	}
}

//===========================================================================
//
// 
//
//===========================================================================

FBitmap FImageTexture::GetBgraBitmap(const PalEntry *p, int *trans)
{
	return mImage->GetCachedBitmap(p, FImageSource::normal, trans);
}

//===========================================================================
//
// 
//
//===========================================================================

void FImageTexture::Create8BitPixels(uint8_t* buffer)
{
	//ImageHelpers::alphaThreshold = alphaThreshold;
	auto buf = mImage->GetPalettedPixels(FImageSource::normal);
	memcpy(buffer, buf.Data(), buf.Size());
}

FTexture* CreateImageTexture(FImageSource* img, const char *name) noexcept
{
	return new FImageTexture(img, name);
}
 