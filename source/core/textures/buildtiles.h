#pragma once

#include "textures.h"
#include "i_time.h"

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

class FTileTexture : public FTexture
{
public:
	FTileTexture()
	{}
	virtual uint8_t* GetRawData() = 0;
	void SetName(const char* name) { Name = name; }
	TArray<uint8_t> Get8BitPixels(bool alphatex) override;
	FBitmap GetBgraBitmap(const PalEntry* remap, int* trans = nullptr) override;
	bool GetTranslucency() override { return false; }
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
		SetSize(width, height);
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
	FLooseTile(TArray<uint8_t> &store, int width, int height)
	{
		RawPixels = std::move(store);
		SetSize(width, height);
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
		SetSize(width, height);
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
		Base = base;
		CopySize(base);
		Resize(GetTexelWidth(), GetTexelHeight());
		Reload();
	}

	void Reload()
	{
		buffer = std::move(Base->Get8BitPixels(false));
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

struct TileDesc
{
	FTexture* texture;	// the currently active tile
	FTexture* backup;	// original backup for map tiles
	RawCacheNode rawCache;	// this is needed for hitscan testing to avoid reloading the texture each time.
	picanm_t picanm;		// animation descriptor
	rottile_t RotTile;// = { -1,-1 };
	TArray<HightileReplacement> Hightiles;
	ReplacementType replacement;
};

struct BuildTiles
{
	TDeletingArray<BuildArtFile*> ArtFiles;
	TileDesc tiledata[MAXTILES];
	TArray<FString> maptilesadded;	// keep track of maps for which custom tiles have been added.

	TArray<FString> addedArt;

	//picanm_t tileConvertAnimFormat(int32_t const picanimraw);
	//PicAnim = tileConvertAnimFormat(picanm);

	void Init(); // cannot be a constructor.
	~BuildTiles()
	{
		CloseAll();
	}

	void CloseAll();

	void AddTile(int tilenum, FTexture* tex, bool permap = false);

	void AddTiles(int firsttile, TArray<uint8_t>& store, const char * mapname);

	void AddFile(BuildArtFile* bfd, bool permap)
	{
		ArtFiles.Push(bfd);
	}
	int FindFile(const FString& filename)
	{
		return ArtFiles.FindEx([filename](const BuildArtFile* element) { return filename.CompareNoCase(element->filename) == 0; });
	}
	int LoadArtFile(const char* file, const char *mapname, int firsttile = -1);
	void LoadArtSet(const char* filename);
	void AddArt(TArray<FString>& art)
	{
		addedArt = std::move(art);
	}
	FTexture* GetTile(int num)
	{
		assert(num < MAXTILES);
		return tiledata[num].texture;
	}
	FTexture* ValidateCustomTile(int tilenum, ReplacementType type);
	int32_t artLoadFiles(const char* filename);
	uint8_t* tileMakeWritable(int num);
	uint8_t* tileCreate(int tilenum, int width, int height);
	int findUnusedTile(void);
	int tileCreateRotated(int owner);
	void InvalidateTile(int num);
	void MakeCanvas(int tilenum, int width, int height);
	HightileReplacement* FindReplacement(int picnum, int palnum, bool skybox = false);
	void AddReplacement(int picnum, const HightileReplacement&);
	void DeleteReplacement(int picnum, int palnum);
	void DeleteReplacements(int picnum)
	{
		assert(picnum < MAXTILES);
		tiledata[picnum].Hightiles.Clear();
	}


};

int tileGetCRC32(int tileNum);
int tileImportFromTexture(const char* fn, int tilenum, int alphacut, int istexture);
void tileCopy(int tile, int tempsource, int temppal, int xoffset, int yoffset, int flags);
void tileSetDummy(int tile, int width, int height);
void tileDelete(int tile);
void tileRemoveReplacement(int tile);
void    artClearMapArt(void);
void    artSetupMapArt(const char* filename);
int tileSetHightileReplacement(int picnum, int palnum, const char *filen, float alphacut, float xscale, float yscale, float specpower, float specfactor, uint8_t flags);
int tileSetSkybox(int picnum, int palnum, const char **facenames, int flags );
int tileDeleteReplacement(int picnum, int palnum);
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
		TileFiles.tiledata[num].rawCache.data = std::move(tex->Get8BitPixels(false));
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
	auto tex = TileFiles.tiledata[num].texture;
	auto p = dynamic_cast<FWritableTile*>(tex);
	return p ? p->GetRawData() : nullptr;
}

// Some hacks to allow accessing the no longer existing arrays as if they still were arrays to avoid changing hundreds of lines of code.
struct TileSiz
{
	const vec2_16_t operator[](size_t index)
	{
		assert(index < MAXTILES);
		vec2_16_t v = { (int16_t)TileFiles.tiledata[index].texture->GetTexelWidth(), (int16_t)TileFiles.tiledata[index].texture->GetTexelHeight() };
		return v;
	}
};
extern TileSiz tilesiz;

inline int tileWidth(int index)
{
	assert(index < MAXTILES);
	return TileFiles.tiledata[index].texture->GetTexelWidth();
}

inline int tileHeight(int index)
{
	assert(index < MAXTILES);
	return TileFiles.tiledata[index].texture->GetTexelHeight();
}

struct PicAnm
{
	picanm_t& operator[](size_t index)
	{
		assert(index < MAXTILES);
		return TileFiles.tiledata[index].picanm;
	}
};
extern PicAnm picanm;

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
	return TileFiles.tiledata[tile].RotTile;
}


inline void tileInvalidate(int16_t tilenume, int32_t, int32_t)
{
	TileFiles.InvalidateTile(tilenume);
}
