#pragma once

#include "textures.h"

enum class ReplacementType : int
{
	Art,
	Writable,
	Restorable,
	Canvas
};

struct HightileReplacement
{
	FTexture* faces[6]; // only one gets used by a texture, the other 5 are for skyboxes only
	vec2f_t scale;
	float alphacut, specpower, specfactor;
	uint16_t palnum, flags;
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
		PicAnim = tileConvertAnimFormat(picanm, &leftoffset, &topoffset);
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
	picanm_t picanmbackup;	// animation descriptor backup when using map tiles
	rottile_t RotTile;// = { -1,-1 };
	TArray<HightileReplacement> Hightiles;
	ReplacementType replacement;
};

struct BuildTiles
{
	FTexture* Placeholder;
	TDeletingArray<BuildArtFile*> ArtFiles;
	TileDesc tiledata[MAXTILES];
	TDeletingArray<BuildArtFile*> PerMapArtFiles;
	TDeletingArray<FTexture*> AllTiles;	// This is for deleting tiles when shutting down.
	TDeletingArray<FTexture*> AllMapTiles;	// Same for map tiles;
	FTexture* tiles[MAXTILES];
	FTexture* tilesbak[MAXTILES];
	TMap<FString, FTexture*> textures;
	TArray<FString> addedArt;
	TMap<FTexture*, int> TextureToTile;

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
	int GetTileIndex(FTexture* tex)
	{
		auto p = TextureToTile.CheckKey(tex);
		return p ? *p : -1;
	}

	void SetupReverseTileMap()
	{
		TextureToTile.Clear();
		for (int i = 0; i < MAXTILES; i++)
		{
			if (tiles[i] != nullptr) TextureToTile.Insert(tiles[i], i);
		}

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
	assert(num < MAXTILES);
	return TileFiles.tiles[num]->GetDisplayWidth();
}

inline int tileHeight(int num)
{
	assert(num < MAXTILES);
	return TileFiles.tiles[num]->GetDisplayHeight();
}

inline int tileLeftOffset(int num)
{
	assert(num < MAXTILES);
	return TileFiles.tiles[num]->GetDisplayLeftOffset();
}

inline int tileTopOffset(int num)
{
	assert(num < MAXTILES);
	return TileFiles.tiles[num]->GetDisplayTopOffset();
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


inline void tileInvalidate(int tilenume, int32_t, int32_t)
{
	TileFiles.InvalidateTile(tilenume);
}

inline FTexture* tileGetTexture(int tile)
{
	assert(tile < MAXTILES);
	return TileFiles.tiles[tile];
}