#pragma once

#include <limits.h>
#include "textures.h"
#include "image.h"
#include "i_time.h"
#include "intvec.h"
#include "name.h"

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

enum
{
	MAXTILES = 30720,
	MAXUSERTILES = (MAXTILES-16)  // reserve 16 tiles at the end

};

enum ETexType
{
	TT_INDEXED,
	TT_TRUECOLOR,
};


enum class ReplacementType : int
{
	Art,
	Writable,
	Restorable,
	Canvas
};


// NOTE: If the layout of this struct is changed, loadpics() must be modified
// accordingly.
struct picanm_t 
{
	uint16_t num;  // animate number
	uint8_t sf;  // anim. speed and flags
	uint8_t extra;

	void Clear()
	{
		extra = sf = 0;
		num = 0;
	}

	int speed()
	{
		return sf & PICANM_ANIMSPEED_MASK;
	}

	int type()
	{
		return sf & PICANM_ANIMTYPE_MASK;
	}

};
picanm_t    tileConvertAnimFormat(int32_t const picanmdisk, int* lo, int* to);

class FTileTexture : public FImageSource
{
public:
	FTileTexture()
	{
		bUseGamePalette = true;
		bTranslucent = false;
	}
	virtual uint8_t* GetRawData() = 0;
	virtual TArray<uint8_t> CreatePalettedPixels(int conversion);
	virtual int CopyPixels(FBitmap* bmp, int conversion);			// This will always ignore 'luminance'.
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
	FArtTile(const TArray<uint8_t>& backingstore, uint32_t offset, int width, int height)
		: RawPixels(backingstore), Offset(offset)
	{
		Width = width;
		Height = height;
	}
	uint8_t* GetRawData() override final
	{
		return &RawPixels[Offset];
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
	FLooseTile(TArray<uint8_t>& store, int width, int height)
	{
		RawPixels = std::move(store);
		Width = width;
		Height = height;
	}

	uint8_t* GetRawData() override
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
		Width = width;
		Height = height;
	}

	uint8_t* GetRawData() override
	{
		return nullptr;
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
		//useType = Writable;
	}

	uint8_t* GetRawData() override
	{
		return buffer.Data();
	}

	bool ResizeImage(int w, int h)
	{
		if (w <= 0 || h <= 0)
		{
			buffer.Reset();
			return false;
		}
		else
		{
			Width = w;
			Height = h;
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
	FImageSource* Base;

public:
	FRestorableTile(FImageSource* base)
	{
		Base = base;

		CopySize(*base);
		ResizeImage(Width, Height);
		Reload();
	}

	void Reload()
	{
		buffer = Base->GetPalettedPixels(0);
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

struct TileOffs
{
	int xsize, ysize, xoffs, yoffs;
};

struct TileDesc
{
	FGameTexture* texture;	// the currently active tile
	picanm_t picanm;		// animation descriptor
	ReplacementType replacement;
	float alphaThreshold;
	int tileflags;

	// Sprite offset hackery for hires replacements. This only gets used for sprites in the 3D view, nothing else.
	TileOffs hiofs;

};

struct TexturePick
{
	FGameTexture* texture;		// which texture to use
	int translation;		// which translation table to use
	int tintFlags;			// which shader tinting options to use
	PalEntry tintColor;		// Tint color
	PalEntry basepalTint;	// can the base palette be done with a global tint effect?
};

struct BuildTiles
{
	FGameTexture* Placeholder;
	TDeletingArray<BuildArtFile*> ArtFiles;
	TArray<TileDesc> tiledata;
	TArray<FString> addedArt;
	TMap<FName, int> nametoindex;
	TMap<int, int> textotile;
	bool locked;	// if this is true, no more tile modifications are allowed.

	void addName(const char* name, int index)
	{
		nametoindex.Insert(name, index);
	}

	void lock()
	{
		locked = true;
		// Now we can set up the reverse map.
		for (unsigned i = 0; i < MAXTILES; i++)
		{
			if (tiledata[i].texture != Placeholder)
			{
				textotile.Insert(tiledata[i].texture->GetID().GetIndex(), i);
			}
		}
	}

	int tileForName(const char* name)
	{
		FName nm(name, true);
		if (nm == NAME_None) return -1;
		auto nmm = nametoindex.CheckKey(nm);
		if (!nmm) return -1;
		return *nmm;
	}

	void SetAliases();


	void Init(); // This cannot be a constructor because it needs the texture manager running.
	~BuildTiles()
	{
		CloseAll();
	}

	void CloseAll();

	void AddTile(int tilenum, FGameTexture* tex);

	void AddTiles(int firsttile, TArray<uint8_t>& store, const char* mapname);

	void AddFile(BuildArtFile* bfd)
	{
		ArtFiles.Push(bfd);
	}
	int FindFile(const FString& filename)
	{
		return ArtFiles.FindEx([filename](const BuildArtFile* element) { return filename.CompareNoCase(element->filename) == 0; });
	}
	int LoadArtFile(const char* file, const char* mapname = nullptr, int firsttile = -1);
	void LoadArtSet(const char* filename);
	void AddArt(TArray<FString>& art)
	{
		addedArt = std::move(art);
	}

	void setAnim(int tile, int type, int speed, int frames)
	{
		auto& anm = tiledata[tile].picanm;
		anm.sf &= ~(PICANM_ANIMTYPE_MASK | PICANM_ANIMSPEED_MASK);
		anm.sf |= clamp(speed, 0, 15) | (type << PICANM_ANIMTYPE_SHIFT);
		anm.num = frames;
	}

	FGameTexture* ValidateCustomTile(int tilenum, ReplacementType type);
	uint8_t* tileMakeWritable(int num);
	uint8_t* tileCreate(int tilenum, int width, int height);
	void MakeCanvas(int tilenum, int width, int height);
};

int tileGetCRC32(int tileNum);
int tileImportFromTexture(const char* fn, int tilenum, int alphacut, int istexture);
void tileCopy(int tile, int tempsource, int temppal, int xoffset, int yoffset, int flags);
void tileSetDummy(int tile, int width, int height);
void tileDelete(int tile);

extern BuildTiles TileFiles;

// Some hacks to allow accessing the no longer existing arrays as if they still were arrays to avoid changing hundreds of lines of code.
struct PicAnm
{
	picanm_t& operator[](size_t index)
	{
		assert(index < MAXTILES);
		return TileFiles.tiledata[index].picanm;
	}
};
extern PicAnm picanm;

inline int tileWidth(int num)
{
	assert((unsigned)num < MAXTILES);
	if ((unsigned)num >= MAXTILES) return 1;
	return (int)TileFiles.tiledata[num].texture->GetDisplayWidth();
}

inline int tileHeight(int num)
{
	assert((unsigned)num < MAXTILES);
	if ((unsigned)num >= MAXTILES) return 1;
	return (int)TileFiles.tiledata[num].texture->GetDisplayHeight();
}

int tileAnimateOfs(int tilenum, int randomize = -1);

inline void tileUpdatePicnum(int* const tileptr, bool mayrotate = false, int randomize = -1)
{
	auto& tile = *tileptr;

	if (picanm[tile].type())
		tile += tileAnimateOfs(tile, randomize);
}


inline FGameTexture* tileGetTexture(int tile, bool animate = false)
{
	assert((unsigned)tile < MAXTILES && tile != -1);	// -1 is valid for overpicnum as 'no texture'.
	if (tile < 0 || tile >= MAXTILES) return nullptr;
	if (animate) tileUpdatePicnum(&tile);
	return TileFiles.tiledata[tile].texture;
}

inline FTextureID tileGetTextureID(int tile)
{
	if (tile < 0 || tile >= MAXTILES) return FNullTextureID();
	return TileFiles.tiledata[tile].texture->GetID();
}

inline int legacyTileNum(FTextureID tex)
{
	auto p = TileFiles.textotile.CheckKey(tex.GetIndex());
	return p ? *p : -1;
}

void tileUpdateAnimations();

const uint8_t* GetRawPixels(FTextureID texid);
uint8_t* GetWritablePixels(FTextureID texid);
void InvalidateTexture(FTextureID num);
TileOffs* GetHiresOffset(FTextureID num);

class FGameTexture;
bool PickTexture(FGameTexture* tex, int paletteid, TexturePick& pick, bool wantindexed = false);
FCanvasTexture* tileGetCanvas(int tilenum);
