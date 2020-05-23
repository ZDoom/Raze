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

struct HightileReplacement
{
	FTexture *faces[6]; // only one gets used by a texture, the other 5 are for skyboxes only
    vec2f_t scale;
    float alphacut, specpower, specfactor;
    uint16_t palnum, flags;
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
	const vec2_16_t &GetSize() const { return Size; }
	const uint8_t& GetPicSize() const { return PicSize; }
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
	void AddReplacement(const HightileReplacement &);
	void DeleteReplacement(int palnum);
	void DeleteReplacements()
	{
		Hightiles.Clear();
	}

	void SetHardwareTexture(int palid, FHardwareTexture* htex)
	{
		HardwareTextures.Insert(palid, htex);
	}
	FHardwareTexture** GetHardwareTexture(int palid)
	{
		return HardwareTextures.CheckKey(palid);
	}

	HightileReplacement * FindReplacement(int palnum, bool skybox = false);
	
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

		while ((j > 1) && ((1 << j) > Size.x))
			j--;
		PicSize = j;

		j = 15;
		while ((j > 1) && ((1 << j) > Size.y))
			j--;
		PicSize |= j << 4;
	}


	FString Name;
	vec2_16_t Size = { 0,0 };	// Keep this in the native format so that we can use it without copying it around.
	rottile_t RotTile = { -1,-1 };
	uint8_t bMasked = true;		// Texture (might) have holes
	int8_t bTranslucent = -1;	// Does this texture have an active alpha channel?
	bool skyColorDone = false;
	uint8_t PicSize = 0;			// A special piece of Build weirdness.
	UseType useType = Image;
	PalEntry FloorSkyColor;
	PalEntry CeilingSkyColor;
	TArray<uint8_t> CachedPixels;
	TArray<HightileReplacement> Hightiles;
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
class FTileTexture : public FTexture
{
public:
	FTileTexture()
	{
		useType = Art;
	}
	void SetName(const char* name) { Name = name; }
	FBitmap GetBgraBitmap(const PalEntry* remap, int* ptrans) override;
	void Create8BitPixels(uint8_t* buffer) override;
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
// A tile with its own pixel buffer
//
//==========================================================================

class FLooseTile : public FTileTexture
{
	TArray<uint8_t> RawPixels;
public:
	FLooseTile(TArray<uint8_t> &store, int width, int height)
	{
		useType = Art;	// Whatever this was before - now it's a tile!
		RawPixels = std::move(store);
		SetSize(width, height);
	}

	const uint8_t* Get8BitPixels() override
	{
		return RawPixels.Data();
	}
};

//==========================================================================
//
// A non-existent tile
//
//==========================================================================

class FDummyTile : public FTileTexture
{
public:
	FDummyTile(int width, int height)
	{
		useType = Art;
		SetSize(width, height);
	}

	const uint8_t* Get8BitPixels() override
	{
		return NULL;
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

public:
	FWritableTile()
	{
		useType = Writable;
	}

	const uint8_t* Get8BitPixels() override
	{
		return buffer.Data();
	}

	uint8_t* GetWritableBuffer() override
	{
		// Todo: Invalidate all hardware textures depending on this.
		return buffer.Data();
	}

	bool Resize(int w, int h)
	{
		if (w <= 0 || h <=  0)
		{
			buffer.Reset();
			return false;
		}
		else
		{
			SetSize(w, h);
			buffer.Resize(w * h);
			return true;
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
		Resize(GetWidth(), GetHeight());
		Reload();
	}

	void Reload() override
	{
		Base->Create8BitPixels(buffer.Data());
	}
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

	BuildArtFile() = default;
	BuildArtFile(const BuildArtFile&) = delete;
	BuildArtFile& operator=(const BuildArtFile&) = delete;
	BuildArtFile(const BuildArtFile&& other)
	{
		filename = std::move(other.filename);
		RawData = std::move(other.RawData);
	}

	BuildArtFile& operator=(const BuildArtFile&& other)
	{
		filename = std::move(other.filename);
		RawData = std::move(other.RawData);
		return *this;
	}
};

//==========================================================================
//
// THe tile container
//
//==========================================================================

struct BuildTiles
{
	FTexture* Placeholder;
	TDeletingArray<BuildArtFile*> ArtFiles;
	TDeletingArray<BuildArtFile*> PerMapArtFiles;
	TDeletingArray<FTexture*> AllTiles;	// This is for deleting tiles when shutting down.
	TDeletingArray<FTexture*> AllMapTiles;	// Same for map tiles;
	FTexture* tiles[MAXTILES];
	FTexture* tilesbak[MAXTILES];
	TMap<FString, FTexture*> textures;
	TArray<FString> addedArt;

	BuildTiles()
	{
		Placeholder = new FDummyTile(0, 0);
		for (auto& tile : tiles) tile = Placeholder;
		for (auto& tile : tilesbak) tile = Placeholder;
	}
	~BuildTiles()
	{
		CloseAll();
	}

	void CloseAll();
	FTexture* GetTexture(const char* path);

	void AddTile(int tilenum, FTexture* tex, bool permap = false);

	void AddTiles(int firsttile, TArray<uint8_t>& store, bool permap);

	void AddFile(BuildArtFile* bfd, bool permap)
	{
		if (!permap) ArtFiles.Push(bfd);
		else PerMapArtFiles.Push(bfd);
	}
	int FindFile(const FString& filename)
	{
		return ArtFiles.FindEx([filename](const BuildArtFile* element) { return filename.CompareNoCase(element->filename) == 0; });
	}
	int LoadArtFile(const char* file, bool mapart = false, int firsttile = -1);
	void CloseAllMapArt();
	void LoadArtSet(const char* filename);
	void AddArt(TArray<FString>& art)
	{
		addedArt = std::move(art);
	}
	FTexture* ValidateCustomTile(int tilenum, int type);
	int32_t artLoadFiles(const char* filename);
	uint8_t* tileMakeWritable(int num);
	uint8_t* tileCreate(int tilenum, int width, int height);
	void tileSetExternal(int tilenum, int width, int height, uint8_t* data);
	int findUnusedTile(void);
	int tileCreateRotated(int owner);
	void ClearTextureCache(bool artonly = false);
	void InvalidateTile(int num);
	void MakeCanvas(int tilenum, int width, int height);
};

int tileGetCRC32(int tileNum);
int tileImportFromTexture(const char* fn, int tilenum, int alphacut, int istexture);
void tileCopy(int tile, int tempsource, int temppal, int xoffset, int yoffset, int flags);
void tileSetDummy(int tile, int width, int height);
void tileDelete(int tile);
void tileRemoveReplacement(int tile);
bool tileLoad(int tileNum);
void    artClearMapArt(void);
void    artSetupMapArt(const char* filename);
void tileSetAnim(int tile, const picanm_t& anm);
int tileSetHightileReplacement(int picnum, int palnum, const char *filen, float alphacut, float xscale, float yscale, float specpower, float specfactor, uint8_t flags);
int tileSetSkybox(int picnum, int palnum, const char **facenames, int flags );
int tileDeleteReplacement(int picnum, int palnum);
void tileCopySection(int tilenum1, int sx1, int sy1, int xsiz, int ysiz, int tilenum2, int sx2, int sy2);

extern BuildTiles TileFiles;
inline bool tileCheck(int num)
{
	auto tex = TileFiles.tiles[num];
	return tex->GetWidth() > 0 && tex->GetHeight() > 0;
}

inline const uint8_t* tilePtr(int num)
{
	auto tex = TileFiles.tiles[num];
	auto p = tex->Get8BitPixels();
	if (p) return p;
	return tex->CachedPixels.Data();
}

inline uint8_t* tileData(int num)
{
	auto tex = TileFiles.tiles[num];
	return tex->GetWritableBuffer();
}

// Some hacks to allow accessing the no longer existing arrays as if they still were arrays to avoid changing hundreds of lines of code.
struct TileSiz
{
	const vec2_16_t &operator[](size_t index)
	{
		assert(index < MAXTILES);
		return TileFiles.tiles[index]->GetSize();
	}
};
extern TileSiz tilesiz;

struct PicAnm
{
	picanm_t& operator[](size_t index)
	{
		assert(index < MAXTILES);
		return TileFiles.tiles[index]->GetAnim();
	}
};
extern PicAnm picanm;

// Helpers to read the refactored tilesiz array.
inline int tileWidth(int num)
{
	return tilesiz[num].x;
}

inline int tileHeight(int num)
{
	return tilesiz[num].y;
}

inline int widthBits(int num)
{
	int w = tileWidth(num);
	int j = 15;

	while ((j > 1) && ((1 << j) > w))
		j--;
	return j;
}

inline int heightBits(int num)
{
	int w = tileHeight(num);
	int j = 15;

	while ((j > 1) && ((1 << j) > w))
		j--;
	return j;
}

inline rottile_t& RotTile(int tile)
{
	assert(tile < MAXTILES);
	return TileFiles.tiles[tile]->GetRotTile();
}


inline void tileInvalidate(int16_t tilenume, int32_t, int32_t)
{
	TileFiles.InvalidateTile(tilenume);
}

#endif


