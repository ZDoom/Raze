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

#include "memarena.h"
#include "bitmap.h"
#include "image.h"
#include "files.h"
#include "filesystem/filesystem.h"
#include "imagehelpers.h"

int FImageSource::NextID;

//==========================================================================
//
//
//
//==========================================================================

typedef FImageSource * (*CreateFunc)(FileReader & file);

struct TexCreateInfo
{
	CreateFunc TryCreate;
};

FImageSource *PNGImage_TryCreate(FileReader &);
FImageSource *JPEGImage_TryCreate(FileReader &);
FImageSource *DDSImage_TryCreate(FileReader &);
FImageSource *PCXImage_TryCreate(FileReader &);
FImageSource *TGAImage_TryCreate(FileReader &);
FImageSource *ArtImage_TryCreate(FileReader &);
FImageSource *StbImage_TryCreate(FileReader &);


// Examines the lump contents to decide what type of texture to create,
// and creates the texture.
FImageSource * FImageSource::GetImage(const char *name)
{
	static TexCreateInfo CreateInfo[] = {
		{ PNGImage_TryCreate },
		{ JPEGImage_TryCreate },
		{ DDSImage_TryCreate },
		{ PCXImage_TryCreate },
		{ StbImage_TryCreate },
		{ ArtImage_TryCreate },
		{ nullptr }
	};

	auto data = fileSystem.OpenFileReader(name, 0);
	if (!data.isOpen())  return nullptr;

	for (size_t i = 0; CreateInfo[i].TryCreate; i++)
	{
		auto image = CreateInfo[i].TryCreate(data);
		if (image != nullptr)
		{
			image->Name = name;
			return image;
		}
	}
	return nullptr;
}
