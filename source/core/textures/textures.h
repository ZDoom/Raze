/*
** textures.h
**
**---------------------------------------------------------------------------
** Copyright 2005-2016 Randy Heit
** Copyright 2005-2019 Christoph Oelckers
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

#include "compat.h"
#include "textureid.h"
#include "zstring.h"
#include "tarray.h"
#include "palentry.h"

class FHardwareTexture;
class FImageSource;

enum MaterialShaderIndex
{
	SHADER_Default,
	SHADER_Warp1,
	SHADER_Warp2,
	SHADER_Brightmap,
	SHADER_Specular,
	SHADER_SpecularBrightmap,
	SHADER_PBR,
	SHADER_PBRBrightmap,
	SHADER_Paletted,
	SHADER_NoTexture,
	SHADER_BasicFuzz,
	SHADER_SmoothFuzz,
	SHADER_SwirlyFuzz,
	SHADER_TranslucentFuzz,
	SHADER_JaggedFuzz,
	SHADER_NoiseFuzz,
	SHADER_SmoothNoiseFuzz,
	SHADER_SoftwareFuzz,
	FIRST_USER_SHADER
};

struct UserShaderDesc
{
	FString shader;
	MaterialShaderIndex shaderType;
	FString defines;
	bool disablealphatest = false;
};

extern TArray<UserShaderDesc> usershaders;


struct FloatRect
{
	float left,top;
	float width,height;


	void Offset(float xofs,float yofs)
	{
		left+=xofs;
		top+=yofs;
	}
	void Scale(float xfac,float yfac)
	{
		left*=xfac;
		width*=xfac;
		top*=yfac;
		height*=yfac;
	}
};

enum ECreateTexBufferFlags
{
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

enum FTextureFormat : uint32_t
{
	TEX_Pal,
	TEX_Gray,
	TEX_RGB,		// Actually ARGB

	TEX_Count
};

class FSoftwareTexture;
class FGLRenderState;

struct spriteframewithrotate;
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
	friend struct BuildTiles;
	friend bool tileLoad(int tileNum);
	friend const uint8_t* tilePtr(int num);

public:
	enum UseType : uint8_t
	{
		Canvas,		// camera texture
		User		// A texture with user-provided image content
	};

	static FTexture *CreateTexture(const char *name);

	virtual ~FTexture ();
	virtual FImageSource *GetImage() const { return nullptr; }

	const FString &GetName() const { return Name; }
	bool isMasked() const { return bMasked; }
	bool isTranslucent() const { return bMasked; }
	PalEntry GetSkyCapColor(bool bottom);

	// Returns the whole texture, stored in column-major order
	virtual void Create8BitPixels(uint8_t* buffer);
	virtual const uint8_t* Get8BitPixels();
	virtual uint8_t* GetWritableBuffer() { return nullptr; }	// For dynamic tiles. Requesting this must also invalidate the texture.
	virtual FBitmap GetBgraBitmap(const PalEntry *remap, int *trans = nullptr);

	static int SmoothEdges(unsigned char * buffer,int w, int h);
	static PalEntry averageColor(const uint32_t *data, int size, int maxout);

	int GetWidth() const { return Size.x; }
	int GetHeight() const { return Size.y; }
	int GetTexelWidth() const { return Size.x; }
	int GetTexelHeight() const { return Size.y; }
	int GetDisplayWidth() const { return Size.x; }
	int GetDisplayHeight() const { return Size.y; }
	const vec2_16_t &GetSize() const { return Size; }
	int GetLeftOffset() const { return leftoffset; }
	int GetTopOffset() const { return topoffset; }
	int GetDisplayLeftOffset() const { return leftoffset; }
	int GetDisplayTopOffset() const { return topoffset; }
	void SetOffsets(int x, int y) { leftoffset = x; topoffset = y; }
	FTextureBuffer CreateTexBuffer(const PalEntry *palette, int flags = 0);
	bool GetTranslucency();
	void CheckTrans(unsigned char * buffer, int size, int trans);
	bool ProcessData(unsigned char * buffer, int w, int h, bool ispatch);
	virtual void Reload() {}
	UseType GetUseType() const { return useType; }
	void DeleteHardwareTextures();

	void SetHardwareTexture(int palid, FHardwareTexture* htex)
	{
		HardwareTextures.Insert(palid, htex);
	}
	FHardwareTexture** GetHardwareTexture(int palid)
	{
		return HardwareTextures.CheckKey(palid);
	}

	int alphaThreshold = 128;
	FixedBitArray<256> NoBrightmapFlag{ 0 };

protected:
	
	void CopySize(FTexture *BaseTexture)
	{
		Size.x = BaseTexture->GetWidth();
		Size.y = BaseTexture->GetHeight();
		leftoffset = BaseTexture->leftoffset;
		topoffset = BaseTexture->topoffset;
	}

	void SetSize(int w, int h)
	{
		Size = { int16_t(w), int16_t(h) };
	}

	FString Name;
	union
	{
		vec2_16_t Size = { 0,0 };	// Keep this in the native format so that we can use it without copying it around.
		struct { uint16_t Width, Height; };
	};
	int leftoffset = 0, topoffset = 0;
	uint8_t bMasked = true;		// Texture (might) have holes
	int8_t bTranslucent = -1;	// Does this texture have an active alpha channel?
	bool skyColorDone = false;
	UseType useType = User;
	PalEntry FloorSkyColor;
	PalEntry CeilingSkyColor;
	TArray<uint8_t> CachedPixels;
	// Don't waste too much effort on efficient storage here. Polymost performs so many calculations on a single draw call that the minor map lookup hardly matters.
	TMap<int, FHardwareTexture*> HardwareTextures;	// Note: These must be deleted by the backend. When the texture manager is taken down it may already be too late to delete them.

	FTexture (const char *name = NULL);
	friend struct BuildTiles;
};

class FCanvasTexture : public FTexture
{
public:
	FCanvasTexture(const char* name, int width, int height)
	{
		Name = name;
		Size.x = width;
		Size.y = height;

		bMasked = false;
		bTranslucent = false;
		//bNoExpand = true;
		useType = FTexture::Canvas;
	}

	void NeedUpdate() { bNeedsUpdate = true; }
	void SetUpdated(bool rendertype) { bNeedsUpdate = false; bFirstUpdate = false; bLastUpdateType = rendertype; }

protected:

	bool bLastUpdateType = false;
	bool bNeedsUpdate = true;
public:
	bool bFirstUpdate = true;

	friend struct FCanvasTextureInfo;
};

#endif


