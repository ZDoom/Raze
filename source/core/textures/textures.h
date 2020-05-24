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

class FImageSource;
class FTexture;
class FHardwareTexture;

// picanm[].sf:
// |bit(1<<7)
// |animtype|animtype|texhitscan|nofullbright|speed|speed|speed|speed|
enum AnimFlags
{
	PICANM_ANIMTYPE_NONE = 0,
	PICANM_ANIMTYPE_OSC = (1 << 6),
	PICANM_ANIMTYPE_FWD = (2 << 6),
	PICANM_ANIMTYPE_BACK = (3 << 6),

	PICANM_ANIMTYPE_SHIFT = 6,
	PICANM_ANIMTYPE_MASK = (3 << 6),  // must be 192
	PICANM_MISC_MASK = (3 << 4),
	PICANM_TEXHITSCAN_BIT = (2 << 4),
	PICANM_NOFULLBRIGHT_BIT = (1 << 4),
	PICANM_ANIMSPEED_MASK = 15,  // must be 15
};


enum ECreateTexBufferFlags
{
	CTF_CheckHires = 1,		// use external hires replacement if found
	CTF_Expand = 2,			// create buffer with a one-pixel wide border
	CTF_ProcessData = 4,	// run postprocessing on the generated buffer. This is only needed when using the data for a hardware texture.
	CTF_CheckOnly = 8,		// Only runs the code to get a content ID but does not create a texture. Can be used to access a caching system for the hardware textures.
};

enum
{
	MAXTILES = 30720,
	MAXUSERTILES = (MAXTILES-16)  // reserve 16 tiles at the end

};

// NOTE: If the layout of this struct is changed, loadpics() must be modified
// accordingly.
struct picanm_t 
{
	uint8_t num;  // animate number
	int8_t xofs, yofs;
	uint8_t sf;  // anim. speed and flags
	uint8_t extra;

	void Clear()
	{
		extra = sf = yofs = xofs = num = 0;
	}
};

struct rottile_t
{
	int16_t newtile;
	int16_t owner;
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

picanm_t    tileConvertAnimFormat(int32_t const picanmdisk);


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
		other.mBuffer = nullptr;
	}

	FTextureBuffer& operator=(FTextureBuffer &&other)
	{
		mBuffer = other.mBuffer;
		mWidth = other.mWidth;
		mHeight = other.mHeight;
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
		Image,	// anything that is not a regular tile.
		Art,	// from an ART file, readonly
		Writable,	// A writable texture
		Restorable,	// a writable copy of something else
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
	int GetLeftOffset() const { return PicAnim.xofs; }
	int GetTopOffset() const { return PicAnim.yofs; }
	picanm_t& GetAnim() { return PicAnim;  }	// This must be modifiable. There's quite a bit of code messing around with the flags in here.
	rottile_t& GetRotTile() { return RotTile; }
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
	picanm_t PicAnim = {};
	FixedBitArray<256> NoBrightmapFlag{ 0 };

protected:
	
	void CopySize(FTexture *BaseTexture)
	{
		Size.x = BaseTexture->GetWidth();
		Size.y = BaseTexture->GetHeight();
		PicAnim.xofs = BaseTexture->PicAnim.xofs;
		PicAnim.yofs = BaseTexture->PicAnim.yofs;
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
	rottile_t RotTile = { -1,-1 };
	uint8_t bMasked = true;		// Texture (might) have holes
	int8_t bTranslucent = -1;	// Does this texture have an active alpha channel?
	bool skyColorDone = false;
	UseType useType = Image;
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


