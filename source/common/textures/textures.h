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

#include "textureid.h"
#include "zstring.h"
#include "tarray.h"
#include "palentry.h"

class FImageSource;

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

struct size_16_t	// must be the same format as vec2_16_t, we cannot include a dirty header like compat.h here.
{
	int16_t x, y;
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
	enum UseType : uint8_t
	{
		Image,	// anything that is not a regular tile.
		Art,	// from an ART file, readonly
		Writable,	// A writable texture
		Restorable,	// a writable copy of something else
		User		// A texture with user-provided image content
	};

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
	virtual uint8_t* GetWritableBuffer() { return nullptr; }	// For dynamic tiles. Requesting this must also invalidate the texture.
	virtual FBitmap GetBgraBitmap(PalEntry *remap, int *trans = nullptr);

	static int SmoothEdges(unsigned char * buffer,int w, int h);
	static PalEntry averageColor(const uint32_t *data, int size, int maxout);

	int GetWidth() const { return Size.x; }
	int GetHeight() const { return Size.y; }
	const size_16_t &GetSize() const { return Size; }
	const uint8_t& GetPicSize() const { return PicSize; }
	int GetLeftOffset() const { return PicAnim.xofs; }
	int GetTopOffset() const { return PicAnim.yofs; }
	picanm_t& GetAnim() { return PicAnim;  }	// This must be modifiable. There's quite a bit of code messing around with the flags in here.
	FTextureBuffer CreateTexBuffer(int translation, int flags = 0);
	bool GetTranslucency();
	void CheckTrans(unsigned char * buffer, int size, int trans);
	bool ProcessData(unsigned char * buffer, int w, int h, bool ispatch);
	virtual void Reload() {}

protected:
	
	void CopySize(FTexture *BaseTexture)
	{
		Size.x = BaseTexture->GetWidth();
		Size.y = BaseTexture->GetHeight();
		PicAnim.xofs = BaseTexture->PicAnim.xofs;
		PicAnim.yofs = BaseTexture->PicAnim.yofs;
		UpdatePicSize();
	}

	void SetSize(int w, int h)
	{
		Size = { int16_t(w), int16_t(h) };
		UpdatePicSize();
	}

	void UpdatePicSize()
	{
		int j = 15;
		int size;

		while ((j > 1) && ((1 << j) > Size.x))
			j--;
		size = j;

		j = 15;
		while ((j > 1) && ((1 << j) > Size.y))
			j--;
		PicSize |= j << 4;
	}


	FString Name;
	size_16_t Size = { 0,0 };	// Keep this in the native format so that we can use it without copying it around.
	picanm_t PicAnim = {};
	rottile_t RotTile = { -1,-1 };
	uint8_t bMasked = true;		// Texture (might) have holes
	int8_t bTranslucent = -1;	// Does this texture have an active alpha channel?
	bool skyColorDone = false;
	uint8_t PicSize;			// A special piece of Build weirdness.
	UseType useType = Image;
	PalEntry FloorSkyColor;
	PalEntry CeilingSkyColor;

	FTexture (const char *name = NULL);

};


class FTileTexture : public FTexture
{
public:
	FTileTexture()
	{
		useType = Art;
	}
	void SetName(const char* name) { Name = name; }
	FBitmap GetBgraBitmap(PalEntry* remap, int* ptrans) override;
	void Create8BitPixels(uint8_t* buffer) override;
	virtual bool Resize(int w, int h) { return false; }	// Regular tiles cannot be resized.
};

//==========================================================================
//
// A tile coming from an ART file.
//
//==========================================================================

class FArtTile : public FTileTexture
{
	const TArray<uint8_t>& RawPixels;
	const uint32_t Offset;
public:
	FArtTile(const TArray<uint8_t>& backingstore, uint32_t offset, int width, int height, int picanm)
		: RawPixels(backingstore), Offset(offset)
	{
		SetSize(width, height);
		PicAnim = tileConvertAnimFormat(picanm);
	}

	const uint8_t* Get8BitPixels() override
	{
		return RawPixels.Data() ? RawPixels.Data() + Offset : nullptr;
	}
};

//==========================================================================
//
// A tile with a writable surface
//
//==========================================================================

class FWritableTile : public FTileTexture
{
protected:
	TArray<uint8_t> buffer;

	FWritableTile() {}
public:
	FWritableTile(int width, int height)
	{
		useType = Writable;
		SetSize(width, height);
		buffer.Resize(width * height);
	}

	const uint8_t* Get8BitPixels() override
	{
		return buffer.Data();
	}

	bool Resize(int w, int h) override
	{
		if (w * h == 0)
		{
			buffer.Reset();
		}
		else
		{
			SetSize(w, h);
			buffer.Resize(w * h);
		}
	}

};

//==========================================================================
//
// A tile with a writable surface
//
//==========================================================================

class FRestorableTile : public FWritableTile
{
	FTexture* Base;

public:
	FRestorableTile(FTexture* base)
	{
		useType = Restorable;
		Base = base;
		CopySize(base);
		Reload();
	}

	void Reload() override
	{
		Base->Create8BitPixels(buffer.Data());
	}

	bool Resize(int w, int h) override
	{
		return false;
	}
};

//==========================================================================
//
// A tile with a user provided buffer
//
//==========================================================================

class FUserTile : public FTileTexture
{
	const uint8_t* RawPixels;
public:
	FUserTile(const uint8_t* data, int width, int height)
		: RawPixels(data)
	{
		useType = User;
		SetSize(width, height);
	}

	const uint8_t* Get8BitPixels() override
	{
		return RawPixels;
	}
};

//==========================================================================
//
// A tile with a user provided buffer
//
//==========================================================================

struct BuildTileDescriptor
{
	int tilenum;
	FTexture* Texture;
};

//==========================================================================
//
// One ART file.
//
//==========================================================================

struct BuildArtFile
{
	FString filename;
	TArray<uint8_t> RawData;
	TArray<BuildTileDescriptor> Textures;

	void AddTiles();
	~BuildArtFile()
	{
		for (auto& desc : Textures)
		{
			delete desc.Texture;
		}
	}

	BuildArtFile() = default;
	BuildArtFile(const BuildArtFile&) = delete;
	BuildArtFile& operator=(const BuildArtFile&) = delete;
	BuildArtFile(const BuildArtFile&& other)
	{
		filename = std::move(other.filename);
		RawData = std::move(other.RawData);
		Textures = std::move(other.Textures);
	}

	BuildArtFile& operator=(const BuildArtFile&& other)
	{
		filename = std::move(other.filename);
		RawData = std::move(other.RawData);
		Textures = std::move(other.Textures);
	}
};

//==========================================================================
//
// THe tile container
//
//==========================================================================

struct BuildFiles
{
	TArray<BuildArtFile> ArtFiles;
	TArray<BuildArtFile> PerMapArtFiles;
	void AddFile(BuildArtFile& bfd, bool permap)
	{
		if (!permap) ArtFiles.Push(std::move(bfd));
		else PerMapArtFiles.Push(std::move(bfd));
	}
	int FindFile(const FString& filename)
	{
		return ArtFiles.FindEx([filename](const BuildArtFile& element) { return filename.CompareNoCase(element.filename) == 0; });
	}
	void LoadArtFile(const char* file, bool mapart);
	void CloseAllMapArt();
	void LoadArtSet(const char* filename);

};

extern BuildFiles TileFiles;

#endif


