/*
** textures.h
**
**---------------------------------------------------------------------------
** Copyright 2005-2016 Randy Heit
** Copyright 2005-2016 Christoph Oelckers
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
*/

#ifndef __TEXTURES_H
#define __TEXTURES_H

#include "textureid.h"
#include "zstring.h"
#include "palentry.h"
/*
#include "v_palette.h"
#include "r_data/v_colortables.h"
#include "colormatcher.h"
#include "r_data/renderstyle.h"
#include "r_data/r_translate.h"
#include "hwrenderer/textures/hw_texcontainer.h"
*/
#include <vector>

class FImageSource;

enum ECreateTexBufferFlags
{
	CTF_CheckHires = 1,		// use external hires replacement if found
	CTF_Expand = 2,			// create buffer with a one-pixel wide border
	CTF_ProcessData = 4,	// run postprocessing on the generated buffer. This is only needed when using the data for a hardware texture.
	CTF_CheckOnly = 8,		// Only runs the code to get a content ID but does not create a texture. Can be used to access a caching system for the hardware textures.
};



class FBitmap;
struct FRemapTable;
struct FCopyInfo;
class FScanner;

// Texture IDs
class FTextureManager;
class FTerrainTypeArray;
class IHardwareTexture;
class FMaterial;
class FMultipatchTextureBuilder;

extern int r_spriteadjustSW, r_spriteadjustHW;

class FNullTextureID : public FTextureID
{
public:
	FNullTextureID() : FTextureID(0) {}
};



class FGLRenderState;

class FSerializer;
namespace OpenGLRenderer
{
	class FGLRenderState;
	class FHardwareTexture;
}

union FContentIdBuilder
{
	uint64_t id;
	struct
	{
		unsigned imageID : 24;
		unsigned translation : 16;
		unsigned expand : 1;
		unsigned scaler : 4;
		unsigned scalefactor : 4;
	};
};

struct FTextureBuffer
{
	uint8_t *mBuffer = nullptr;
	int mWidth = 0;
	int mHeight = 0;
	uint64_t mContentId = 0;	// unique content identifier. (Two images created from the same image source with the same settings will return the same value.)

	FTextureBuffer() = default;

	~FTextureBuffer()
	{
		if (mBuffer) delete[] mBuffer;
	}

	FTextureBuffer(const FTextureBuffer &other) = delete;
	FTextureBuffer(FTextureBuffer &&other)
	{
		mBuffer = other.mBuffer;
		mWidth = other.mWidth;
		mHeight = other.mHeight;
		mContentId = other.mContentId;
		other.mBuffer = nullptr;
	}

	FTextureBuffer& operator=(FTextureBuffer &&other)
	{
		mBuffer = other.mBuffer;
		mWidth = other.mWidth;
		mHeight = other.mHeight;
		mContentId = other.mContentId;
		other.mBuffer = nullptr;
		return *this;
	}

};

// Base texture class
class FTexture
{

public:
	static FTexture *CreateTexture(const char *name);
	static FTexture* GetTexture(const char* path);

	virtual ~FTexture ();
	virtual FImageSource *GetImage() const { return nullptr; }

	const FString &GetName() const { return Name; }
	bool isMasked() const { return bMasked; }
	bool isTranslucent() const { return bMasked; }
	PalEntry GetSkyCapColor(bool bottom);

	// Returns the whole texture, stored in column-major order
	virtual void Create8BitPixels(uint8_t* buffer);
	virtual const uint8_t* Get8BitPixels();
	virtual FBitmap GetBgraBitmap(PalEntry *remap, int *trans = nullptr);

	static int SmoothEdges(unsigned char * buffer,int w, int h);
	static PalEntry averageColor(const uint32_t *data, int size, int maxout);

	int GetWidth() { return Width; }
	int GetHeight() { return Height; }
	int GetLeftOffset() { return LeftOffset; }
	int GetTopOffset() { return TopOffset; }
	FTextureBuffer CreateTexBuffer(int translation, int flags = 0);
	bool GetTranslucency();
	void CheckTrans(unsigned char * buffer, int size, int trans);
	bool ProcessData(unsigned char * buffer, int w, int h, bool ispatch);

protected:
	
	void CopySize(FTexture *BaseTexture)
	{
		Width = BaseTexture->GetWidth();
		Height = BaseTexture->GetHeight();
		TopOffset = BaseTexture->TopOffset;
		TopOffset = BaseTexture->TopOffset;
	}

	int Width = 0, Height = 0;
	int LeftOffset = 0, TopOffset = 0;
	FString Name;
	uint8_t bMasked = true;		// Texture (might) have holes
	int8_t bTranslucent = -1;	// Does this texture have an active alpha channel?
	bool skyColorDone = false;
	PalEntry FloorSkyColor;
	PalEntry CeilingSkyColor;

	FTexture (const char *name = NULL);

};

#endif


