#pragma once

#include <limits.h>
#include "textures.h"
#include "image.h"
#include "i_time.h"
#include "intvec.h"
#include "name.h"

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
};
picanm_t    tileConvertAnimFormat(int32_t const picanmdisk, int* lo, int* to);

struct rottile_t
{
	int16_t newtile;
	int16_t owner;
};

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

struct RawCacheNode
{
	TArray<uint8_t> data;	
	uint64_t lastUseTime;
};

struct TileOffs
{
	int xsize, ysize, xoffs, yoffs;
};

struct TileDesc
{
	FGameTexture* texture;	// the currently active tile
	FGameTexture* backup;	// original backup for map tiles
	RawCacheNode rawCache;	// this is needed for hitscan testing to avoid reloading the texture each time.
	picanm_t picanm;		// animation descriptor
	picanm_t picanmbackup;	// animation descriptor backup when using map tiles
	rottile_t RotTile;// = { -1,-1 };
	ReplacementType replacement;
	float alphaThreshold;

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
	TileDesc tiledata[MAXTILES];
	TArray<FString> addedArt;
	TArray<FString> maptilesadded;
	TMap<int, FGameTexture*> cameratextures;
	TMap<FName, int> nametoindex;

	void addName(const char* name, int index)
	{
		nametoindex.Insert(name, index);
	}

	int tileForName(const char* name)
	{
		FName nm(name, true);
		if (nm == NAME_None) return -1;
		auto nmm = nametoindex.CheckKey(nm);
		if (!nmm) return -1;
		return *nmm;
	}

	void Init(); // This cannot be a constructor because it needs the texture manager running.
	~BuildTiles()
	{
		CloseAll();
	}

	void SetBackup()
	{
		for (auto& td : tiledata)
		{
			td.backup = td.texture;
			td.picanmbackup = td.picanm;
		}
	}

	void CloseAll();

	void AddTile(int tilenum, FGameTexture* tex, bool permap = false);

	void AddTiles(int firsttile, TArray<uint8_t>& store, const char* mapname);

	void AddFile(BuildArtFile* bfd, bool permap)
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
	int32_t artLoadFiles(const char* filename);
	uint8_t* tileMakeWritable(int num);
	uint8_t* tileCreate(int tilenum, int width, int height);
	int findUnusedTile(void);
	int tileCreateRotated(int owner);
	void InvalidateTile(int num);
	void MakeCanvas(int tilenum, int width, int height);
};

int tileGetCRC32(int tileNum);
int tileImportFromTexture(const char* fn, int tilenum, int alphacut, int istexture);
void tileCopy(int tile, int tempsource, int temppal, int xoffset, int yoffset, int flags);
void tileSetDummy(int tile, int width, int height);
void tileDelete(int tile);
bool tileLoad(int tileNum);
void    artClearMapArt(void);
void    artSetupMapArt(const char* filename);
void tileCopySection(int tilenum1, int sx1, int sy1, int xsiz, int ysiz, int tilenum2, int sx2, int sy2);

extern BuildTiles TileFiles;
inline bool tileCheck(int num)
{
	auto tex = TileFiles.tiledata[num].texture;
	return tex && tex->GetTexelWidth() > 0 && tex->GetTexelHeight() > 0;
}

inline const uint8_t* tilePtr(int num)
{
	if (TileFiles.tiledata[num].rawCache.data.Size() == 0)
	{
		auto tex = TileFiles.tiledata[num].texture;
		if (!tex || tex->GetTexelWidth() <= 0 || tex->GetTexelHeight() <= 0) return nullptr;
		TileFiles.tiledata[num].rawCache.data = tex->GetTexture()->Get8BitPixels(false);
	}
	TileFiles.tiledata[num].rawCache.lastUseTime = I_nsTime();
	return TileFiles.tiledata[num].rawCache.data.Data();
}

inline bool tileLoad(int tileNum)
{
	return !!tilePtr(tileNum);
}

inline uint8_t* tileData(int num)
{
	auto tex = dynamic_cast<FImageTexture*>(TileFiles.tiledata[num].texture->GetTexture());
	if (!tex) return nullptr;
	auto p = dynamic_cast<FWritableTile*>(tex->GetImage());
	return p ? p->GetRawData() : nullptr;
}

inline const uint8_t* tileRawData(int num)
{
	auto tex = dynamic_cast<FImageTexture*>(TileFiles.tiledata[num].texture->GetTexture());
	if (!tex) return nullptr;
	auto p = dynamic_cast<FTileTexture*>(tex->GetImage());
	return p ? p->GetRawData() : nullptr;
}

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

inline int tileLeftOffset(int num)
{
	assert((unsigned)num < MAXTILES);
	if ((unsigned)num >= MAXTILES) return 0;
	return (int)TileFiles.tiledata[num].texture->GetDisplayLeftOffset();
}

inline int tileTopOffset(int num)
{
	assert((unsigned)num < MAXTILES);
	if ((unsigned)num >= MAXTILES) return 0;
	return (int)TileFiles.tiledata[num].texture->GetDisplayTopOffset();
}

inline rottile_t& RotTile(int tile)
{
	assert(tile < MAXTILES);
	return TileFiles.tiledata[tile].RotTile;
}


int32_t animateoffs(int const tilenum, int fakevar);

inline FGameTexture* tileGetTexture(int tile, bool animate = false)
{
	assert((unsigned)tile < MAXTILES);
	if (tile < 0 || tile >= MAXTILES) return nullptr;
	if (animate)
	{
		if (TileFiles.tiledata[tile].picanm.sf & PICANM_ANIMTYPE_MASK)
			tile += animateoffs(tile, 0);

	}
	return TileFiles.tiledata[tile].texture;
}

bool tileEqualTo(int me, int other);
void tileUpdateAnimations();

bool ValidateTileRange(const char* cmd, int& begin, int& end, FScriptPosition pos, bool allowswap = true);
bool ValidateTilenum(const char* cmd, int tile, FScriptPosition pos);


struct TileImport
{
	FString fn;
	int tile = -1;
	int alphacut = 128, flags = 0;
	int haveextra = 0;
	int xoffset = INT_MAX, yoffset = INT_MAX;
	int istexture = 0, extra = INT_MAX;
	int64_t crc32 = INT64_MAX;
	int sizex = INT_MAX, sizey;
	// Blood extensions
	int surface = INT_MAX, vox = INT_MAX, shade = INT_MAX;

};

void processTileImport(const char* cmd, FScriptPosition& pos, TileImport& imp);

struct SetAnim
{
	int tile1, tile2, speed, type;
};

void processSetAnim(const char* cmd, FScriptPosition& pos, SetAnim& imp);
class FGameTexture;
bool PickTexture(FGameTexture* tex, int paletteid, TexturePick& pick, bool wantindexed = false);

inline FixedBitArray<MAXTILES> gotpic;

inline bool testgotpic(int32_t tilenume, bool reset = false)
{
	bool res = gotpic[tilenume];
	if (reset) gotpic.Clear(tilenume);
	return res;
}

