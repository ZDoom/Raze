#pragma once

#include <stdint.h>
#include "gamefuncs.h"
#include "tiletexture.h"
#include "s_soundinternal.h"

// extended texture info for which there is no room in the texture manager.

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

	int speed() const
	{
		return sf & PICANM_ANIMSPEED_MASK;
	}

	int type() const
	{
		return sf & PICANM_ANIMTYPE_MASK;
	}

};


struct TileOffs
{
	int16_t xsize, ysize, xoffs, yoffs;
};

// probably only useful for Duke. We'll see
struct SwitchDef
{
	enum
	{
		shootable = 1,
		oneway = 2,
		resettable = 4,
		nofilter = 8,
	};
	enum
	{
		None = 0,	// no switch, so that all non-switches can use an empty entry to avoid validation checks
		Regular = 1,
		Combo = 2,
		Multi = 3,
		Access = 4
	};
	uint8_t type;
	uint8_t flags;
	FSoundID soundid;
	FTextureID states[4];
};

inline TArray<SwitchDef> switches;

struct TexExtInfo
{
	uint16_t animindex;	// not used yet - for ZDoom-style animations.
	uint16_t switchindex;
	uint8_t surftype;	// Contents depend on the game, e.g. this holds Blood's surfType. Other games have hard coded handling for similar effects.
	union
	{
		uint8_t switchphase;	// For Duke: index of texture in switch sequence.
		uint8_t tileshade;		// Blood's shade.dat
	};
	int16_t tiletovox;	// engine-side voxel index
	picanm_t picanm;	// tile-based animation data.
	uint32_t flags;		// contents are game dependent.
	TileOffs hiofs;
}; 

inline TArray<TexExtInfo> texExtInfo;
inline int firstarttile, maxarttile;	// we need this for conversion between tile numbers and texture IDs

//==========================================================================
//
// THe tile container
//
//==========================================================================

struct TexturePick
{
	FGameTexture* texture;		// which texture to use
	int translation;		// which translation table to use
	int tintFlags;			// which shader tinting options to use
	PalEntry tintColor;		// Tint color
	PalEntry basepalTint;	// can the base palette be done with a global tint effect?
};
bool PickTexture(FGameTexture* tex, int paletteid, TexturePick& pick, bool wantindexed = false);



void tileUpdatePicnum(FTextureID& tileptr, int randomize = -1);
void tileUpdateAnimations();
int tilehasmodelorvoxel(FTextureID tilenume, int pal);

inline const TexExtInfo& GetExtInfo(FTextureID tex) // this is only for reading, not for modifying!
{
	unsigned index = tex.GetIndex();
	if (index >= texExtInfo.Size()) index = 0;	// index 0 (the null texture) serves as backup if textures get added at runtime.
	return texExtInfo[index];
}

inline TexExtInfo& AccessExtInfo(FTextureID tex) // this is for modifying and should only be called by init code!
{
	unsigned index = tex.GetIndex();
	if (index >= texExtInfo.Size())
	{
		unsigned now = texExtInfo.Size();
		texExtInfo.Resize(index + 1);
		for (; now <= index; now++) texExtInfo[now] = texExtInfo[0];
	}
	return texExtInfo[index];
}

inline bool isaccessswitch(FTextureID texid)
{
	return switches[GetExtInfo(texid).switchindex].type == SwitchDef::Access;
}

inline bool isshootableswitch(FTextureID texid)
{
	return switches[GetExtInfo(texid).switchindex].flags & SwitchDef::shootable;
}

inline int tilehasvoxel(FTextureID texid)
{
	if (r_voxels)
	{
		auto x = GetExtInfo(texid);
		if (x.tiletovox != -1) return true;
	}
	return false;
}

inline FTextureID tileGetTextureID(int tilenum)
{
	if ((unsigned)tilenum >= MAXTILES) return FNullTextureID();
	return FSetTextureID(firstarttile + min(tilenum, maxarttile));
}

// Use this only for places where some legacy feature needs a tile index. The only such places are CON and nnext.
inline int legacyTileNum(FTextureID id)
{
	int index = id.GetIndex() - firstarttile;
	if (index < 0 || index > maxarttile) return maxarttile;
	return index;
}

inline const TileOffs* GetHiresOffset(FTextureID tex)
{
	// fixme: This must return nullptr if the tile has no replacement. 
	auto& x = GetExtInfo(tex);
	if (x.hiofs.xsize != 0) return &x.hiofs;
	else return nullptr;
}

inline unsigned tileflags(FTextureID texid)
{
	return GetExtInfo(texid).flags;
}

inline uint8_t tilesurface(FTextureID texid)
{
	return GetExtInfo(texid).surftype;
}

