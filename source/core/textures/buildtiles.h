#pragma once

#include <limits.h>
#include "textures.h"
#include "image.h"
#include "i_time.h"
#include "intvec.h"
#include "name.h"
#include "tiletexture.h"
#include "maptypes.h"
#include "texinfo.h"
#include "texturemanager.h"

// all that's left here is the wrappers that need to go away.


// wrappers that allow partial migration to a textureID-based setup.
inline const FTextureID walltype::walltexture() const
{
	return tileGetTextureID(wallpicnum);
}
inline const FTextureID walltype::overtexture() const
{
	return tileGetTextureID(overpicnum);
}

inline const FTextureID sectortype::ceilingtexture() const
{
	return tileGetTextureID(ceilingpicnum);
}

inline const FTextureID sectortype::floortexture() const
{
	return tileGetTextureID(floorpicnum);
}

inline const FTextureID spritetypebase::spritetexture() const
{
	return tileGetTextureID(picnum);
}

inline void sectortype::setfloortexture(FTextureID tex)
{
	floorpicnum = legacyTileNum(tex);
}

inline void sectortype::setceilingtexture(FTextureID tex)
{
	ceilingpicnum = legacyTileNum(tex);
}

inline void walltype::setwalltexture(FTextureID tex)
{
	wallpicnum = legacyTileNum(tex);
}

inline void walltype::setovertexture(FTextureID tex)
{
	if (tex.isValid()) overpicnum = legacyTileNum(tex);
	else overpicnum = -1;	// unlike the others this one can be invalid.
}


//[[deprecated]]
inline int tileForName(const char* name)
{
	auto texid = TexMan.CheckForTexture(name, ETextureType::Any, FTextureManager::TEXMAN_TryAny | FTextureManager::TEXMAN_ReturnAll);
	if (!texid.isValid()) return -1;
	return legacyTileNum(texid);
}

// Some hacks to allow accessing the no longer existing arrays as if they still were arrays to avoid changing hundreds of lines of code.
struct PicAnm
{
	//[[deprecated]]
	const picanm_t& operator[](int index) const
	{
		assert((unsigned)index < MAXTILES);
		return GetExtInfo(tileGetTextureID(index)).picanm;
	}
};
inline PicAnm picanm;

//[[deprecated]]
inline int tileWidth(int num)
{
	auto texid = tileGetTextureID(num);
	if (!texid.isValid()) return 1;
	else return (int)TexMan.GetGameTexture(texid)->GetDisplayWidth();
}

//[[deprecated]]
inline int tileHeight(int num)
{
	auto texid = tileGetTextureID(num);
	if (!texid.isValid()) return 1;
	else return (int)TexMan.GetGameTexture(texid)->GetDisplayHeight();
}


//[[deprecated]]
inline FGameTexture* tileGetTexture(int tile, bool animate = false)
{
	auto texid = tileGetTextureID(tile);
	if (animate) tileUpdatePicnum(texid);
	return TexMan.GetGameTexture(texid);
}
