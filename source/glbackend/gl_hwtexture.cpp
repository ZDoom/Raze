// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2004-2016 Christoph Oelckers
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//

#include <algorithm>
#include "glad/glad.h"
#include "glbackend.h"
#include "bitmap.h"
//#include "compat.h"

// Workaround to avoid including the dirty 'compat.h' header. This will hopefully not be needed anymore once the texture format uses something better.
# define B_LITTLE_ENDIAN 1
# define B_BIG_ENDIAN    0



//===========================================================================
// 
//	Allocates an empty texture
//
//===========================================================================

unsigned int FHardwareTexture::CreateTexture(int w, int h, bool eightbit, bool mipmapped)
{
	glTexID = GLInterface.GetTextureID();
	glActiveTexture(GL_TEXTURE15);
	glBindTexture(GL_TEXTURE_2D, glTexID);
	int size = std::max(w, h);
	int bits = 0;
	while (size) bits++, size >>= 1;
	glTextureBytes = eightbit? 1 : 4;
	if (eightbit) mipmapped = false;
	mWidth = w;
	mHeight = h;

	glTexStorage2D(GL_TEXTURE_2D, mipmapped? bits : 1, eightbit? GL_R8 : GL_RGBA8, w, h);
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

	int dstformat = glTextureBytes == 1 ? GL_R8 : GL_RGBA8;// TexFormat[gl_texture_format];
	int srcformat = glTextureBytes == 1 ? GL_RED : GL_BGRA;// TexFormat[gl_texture_format];

	glActiveTexture(GL_TEXTURE15);
	glBindTexture(GL_TEXTURE_2D, glTexID);

	if (glTextureBytes < 4) glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, srcformat, GL_UNSIGNED_BYTE, buffer);
	if (mipmapped) glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	if (glTextureBytes < 4) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	return glTexID;
}

//===========================================================================
// 
//	Destroys the texture
//
//===========================================================================
FHardwareTexture::~FHardwareTexture() 
{ 
	if (glTexID != 0) glDeleteTextures(1, &glTexID);
}


unsigned int FHardwareTexture::GetTextureHandle()
{
	return glTexID;
}


