/*
** gl_hwtexture.cpp
** GL texture abstraction
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

#include <algorithm>
#include "gl_load.h"
#include "glbackend.h"
#include "bitmap.h"
#include "c_dispatch.h"
#include "printf.h"
//#include "compat.h"

// Workaround to avoid including the dirty 'compat.h' header. This will hopefully not be needed anymore once the texture format uses something better.
# define B_LITTLE_ENDIAN 1
# define B_BIG_ENDIAN    0

uint64_t alltexturesize;

CCMD(alltexturesize)
{
	Printf("All textures are %llu bytes\n", alltexturesize);
}


//===========================================================================
// 
//	Allocates an empty texture
//
//===========================================================================

unsigned int FHardwareTexture::CreateTexture(int w, int h, int type, bool mipmapped)
{
	static int gltypes[] = { GL_R8, GL_RGBA8, GL_RGB5_A1, GL_RGBA2 };
	static uint8_t bytes[] = { 1, 4, 2, 1 };
	glGenTextures(1, &glTexID);
	glActiveTexture(GL_TEXTURE15);
	glBindTexture(GL_TEXTURE_2D, glTexID);
	int size = std::max(w, h);
	int bits = 0;
	while (size) bits++, size >>= 1;
	internalType = type;
	if (type == Indexed) mipmapped = false;
	mWidth = w;
	mHeight = h;

	allocated = w * h;
	if (mipmapped)
	{
		
		for (auto mip = allocated>>2; mip > 0; mip >>= 2)
		{
			allocated += mip;
		}
	}
	allocated *= bytes[type];
	alltexturesize += allocated;

	glTexStorage2D(GL_TEXTURE_2D, mipmapped? bits : 1, gltypes[type], w, h);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	this->mipmapped = mipmapped;
	return glTexID;
}


//===========================================================================
// 
//	Loads the texture image into the hardware
//
//===========================================================================

unsigned int FHardwareTexture::LoadTexture(const unsigned char * buffer)
{
	return LoadTexturePart(buffer, 0, 0, mWidth, mHeight);
}

unsigned int FHardwareTexture::LoadTexture(FBitmap& bmp)
{
	return LoadTexture(bmp.GetPixels());
}

//===========================================================================
// 
//	Loads the texture image into the hardware
//
//===========================================================================

unsigned int FHardwareTexture::LoadTexturePart(const unsigned char* buffer, int x, int y, int w, int h)
{
	if (glTexID == 0) return 0;

	int srcformat = internalType == Indexed ? GL_RED : GL_BGRA;// TexFormat[gl_texture_format];

	glActiveTexture(GL_TEXTURE15);
	glBindTexture(GL_TEXTURE_2D, glTexID);

	if (internalType == Indexed) glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, srcformat, GL_UNSIGNED_BYTE, buffer);
	if (mipmapped) glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	if (internalType == Indexed) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	return glTexID;
}

//===========================================================================
// 
//	Destroys the texture
//
//===========================================================================
FHardwareTexture::~FHardwareTexture() 
{ 
	alltexturesize -= allocated;
	if (glTexID != 0) glDeleteTextures(1, &glTexID);
}


unsigned int FHardwareTexture::GetTextureHandle()
{
	return glTexID;
}


