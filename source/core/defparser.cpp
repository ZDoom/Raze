
/*
** def.cpp
**
**---------------------------------------------------------------------------
** Copyright 2021 Christoph Oelckers
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
**
*/

#include "build.h"

#include "buildtiles.h"
#include "bitmap.h"
#include "m_argv.h"
#include "gamestruct.h"
#include "gamecontrol.h"
#include "palettecontainer.h"
#include "mapinfo.h"
#include "hw_voxels.h"
#include "psky.h"
#include "gamefuncs.h"
#include "tilesetbuilder.h"
#include "models/modeldata.h"
#include "texturemanager.h"


int tileSetHightileReplacement(int tilenum, int palnum, FTextureID texid, float alphacut, float xscale, float yscale, float specpower, float specfactor, bool indexed);

int tileSetSkybox(int tilenum, int palnum, FString* facenames, bool indexed = false);
void tileRemoveReplacement(int num);
void AddUserMapHack(const FString& title, const FString& mhkfile, uint8_t* md4);

static TilesetBuildInfo* tbuild;
static void defsparser(FScanner& sc);
const char* G_DefaultDefFile(void);

static void performInclude(FScanner* sc, const char* fn, FScriptPosition* pos)
{
	int lump = fileSystem.FindFile(fn);
	if (lump == -1)
	{
		if (!pos) Printf("Warning: Unable to open %s\n", fn);
		else pos->Message(MSG_ERROR, "Unable to open %s", fn);
	}
	else
	{
		FScanner included;
		included.OpenLumpNum(lump);
		if (sc) included.symbols = std::move(sc->symbols);
		defsparser(included);
		if (sc) sc->symbols = std::move(included.symbols);
	}
}

void parseInclude(FScanner& sc, FScriptPosition& pos)
{
	sc.MustGetString();
	performInclude(&sc, sc.String, &pos);
}

void parseIncludeDefault(FScanner& sc, FScriptPosition& pos)
{
	performInclude(&sc, G_DefaultDefFile(), &pos);
}

//===========================================================================
// 
//	Helpers for tile parsing
//
//===========================================================================

bool ValidateTileRange(const char* cmd, int& begin, int& end, FScriptPosition pos, bool allowswap = true)
{
	if (end < begin)
	{
		pos.Message(MSG_WARNING, "%s: tile range [%d..%d] is backwards. Indices were swapped.", cmd, begin, end);
		std::swap(begin, end);
	}

	if ((unsigned)begin >= MAXUSERTILES || (unsigned)end >= MAXUSERTILES)
	{
		pos.Message(MSG_ERROR, "%s: Invalid tile range [%d..%d]", cmd, begin, end);
		return false;
	}

	return true;
}

bool ValidateTilenum(const char* cmd, int tile, FScriptPosition pos)
{
	if ((unsigned)tile >= MAXUSERTILES)
	{
		pos.Message(MSG_ERROR, "%s: Invalid tile number %d", cmd, tile);
		return false;
	}

	return true;
}

//===========================================================================
// 
//
//
//===========================================================================

static int tileSetHightileReplacement(FScanner& sc, int tilenum, int palnum, const char* filename, float alphacut, float xscale, float yscale, float specpower, float specfactor, bool indexed = false)
{
	if ((uint32_t)tilenum >= (uint32_t)MAXTILES) return -1;
	if ((uint32_t)palnum >= (uint32_t)MAXPALOOKUPS) return -1;

	auto tex = tbuild->tile[tilenum].tileimage;

	if (tex == nullptr || tex->GetWidth() <= 0 || tex->GetHeight() <= 0)
	{
		sc.ScriptMessage("Warning: defined hightile replacement for empty tile %d.", tilenum);
		return -1;	// cannot add replacements to empty tiles, must create one beforehand
	}

	FTextureID texid = TexMan.CheckForTexture(filename, ETextureType::Any, FTextureManager::TEXMAN_ForceLookup);
	if (!texid.isValid())
	{
		sc.ScriptMessage("%s: Replacement for tile %d does not exist or is invalid\n", filename, tilenum);
		return -1;
	}
	tileSetHightileReplacement(tilenum, palnum, texid, alphacut, xscale, yscale, specpower, specfactor, indexed);
	return 0;
}

//==========================================================================
//
// Import a tile from an external image.
// This has been signifcantly altered so it may not cover everything yet.
//
//==========================================================================

static int tileImportFromTexture(FScanner& sc, const char* fn, int tilenum, int alphacut, int istexture)
{
	FTextureID texid = TexMan.CheckForTexture(fn, ETextureType::Any, FTextureManager::TEXMAN_ForceLookup);
	if (!texid.isValid()) return -1;
	auto tex = TexMan.GetGameTexture(texid);

	int32_t xsiz = tex->GetTexelWidth(), ysiz = tex->GetTexelHeight();

	if (xsiz <= 0 || ysiz <= 0)
		return -2;

	tbuild->tile[tilenum].imported = TexMan.GetGameTexture(texid);
	tbuild->tile[tilenum].tileimage = tbuild->tile[tilenum].imported->GetTexture()->GetImage();

	if (istexture)
		tileSetHightileReplacement(sc, tilenum, 0, fn, (float)(255 - alphacut) * (1.f / 255.f), 1.0f, 1.0f, 1.0, 1.0);
	return 0;

}

//===========================================================================
// 
//	Internal worker for tileImportTexture
//
//===========================================================================

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


static void processTileImport(FScanner& sc, const char* cmd, FScriptPosition& pos, TileImport& imp)
{
	if (!ValidateTilenum(cmd, imp.tile, pos))
		return;

	auto tex = tbuild->tile[imp.tile].tileimage;
	if (imp.crc32 != INT64_MAX && int(imp.crc32) != tileGetCRC32(tex))
		return;

	if (imp.sizex != INT_MAX)
	{
		// the size must match the previous tile
		if (!tex)
		{
			if (imp.sizex != 0 || imp.sizey != 0) return;
		}
		else
		{
			if (tex->GetWidth() != imp.sizex || tex->GetHeight() != imp.sizey) return;
		}
	}

	imp.alphacut = clamp(imp.alphacut, 0, 255);

	if (imp.fn.IsNotEmpty() && tileImportFromTexture(sc, imp.fn.GetChars(), imp.tile, imp.alphacut, imp.istexture) < 0) return;

	tbuild->tile[imp.tile].extinfo.picanm.sf |= imp.flags;
	if (imp.surface != INT_MAX) tbuild->tile[imp.tile].extinfo.surftype = imp.surface;
	if (imp.shade != INT_MAX) tbuild->tile[imp.tile].extinfo.tileshade = imp.shade;

	// This is not quite the same as originally, for two reasons:
	// 1: Since these are texture properties now, there's no need to clear them.
	// 2: The original code assumed that an imported texture cannot have an offset. But this can import Doom patches and PNGs with grAb, so the situation is very different.
	auto itex = tbuild->tile[imp.tile].imported;

	if (imp.xoffset == INT_MAX) imp.xoffset = itex->GetTexelLeftOffset();
	else imp.xoffset = clamp(imp.xoffset, -128, 127);
	if (imp.yoffset == INT_MAX) imp.yoffset = itex->GetTexelTopOffset();
	else imp.yoffset = clamp(imp.yoffset, -128, 127);
	tbuild->tile[imp.tile].leftOffset = imp.xoffset;
	tbuild->tile[imp.tile].topOffset = imp.yoffset;
	if (imp.extra != INT_MAX) tbuild->tile[imp.tile].extinfo.picanm.extra = imp.extra;
}

//===========================================================================
// 
//	Internal worker for tileSetAnim
//
//===========================================================================

struct SetAnim
{
	int tile1, tile2, speed, type;
};

static void setAnim(int tile, int type, int speed, int frames)
{
	auto& anm = tbuild->tile[tile].extinfo.picanm;
	anm.sf &= ~(PICANM_ANIMTYPE_MASK | PICANM_ANIMSPEED_MASK);
	anm.sf |= clamp(speed, 0, 15) | (type << PICANM_ANIMTYPE_SHIFT);
	anm.num = frames;
}

static void processSetAnim(const char* cmd, FScriptPosition& pos, SetAnim& imp)
{
	if (!ValidateTilenum(cmd, imp.tile1, pos) ||
		!ValidateTilenum(cmd, imp.tile2, pos))
		return;

	if (imp.type < 0 || imp.type > 3)
	{
		pos.Message(MSG_ERROR, "%s: animation type must be 0-3, got %d", cmd, imp.type);
		return;
	}

	int count = imp.tile2 - imp.tile1;
	if (imp.type == (PICANM_ANIMTYPE_BACK >> PICANM_ANIMTYPE_SHIFT) && imp.tile1 > imp.tile2)
		count = -count;

	setAnim(imp.tile1, imp.type, imp.speed, count);
}

//==========================================================================
//
// Copies a tile into another and optionally translates its palette.
//
//==========================================================================

static void tileCopy(int tile, int source, int pal, int xoffset, int yoffset, int flags)
{
	picanm_t* picanm = nullptr;
	picanm_t* sourceanm = nullptr;

	if (source == -1) source = tile;
	auto& tiledesc = tbuild->tile[tile];
	auto& srcdesc = tbuild->tile[source];
	tiledesc.extinfo.picanm = {};
	tiledesc.extinfo.picanm.sf = (srcdesc.extinfo.picanm.sf & PICANM_MISC_MASK) | flags;
	tiledesc.tileimage = srcdesc.tileimage;
	tiledesc.imported = srcdesc.imported;
	tiledesc.leftOffset = srcdesc.leftOffset;
	tiledesc.topOffset = srcdesc.topOffset;
	/* todo: create translating image source class.
	if (pal != -1)
	{
		tiledesc.tileimage = createTranslatedImage(tiledesc.tileimage, pal);
	}
	*/
}

//===========================================================================
//
//
//
//===========================================================================

template<int cnt>
static void parseSkip(FScanner& sc, FScriptPosition& pos)
{
	for (int i = 0; i < cnt; i++) if (!sc.GetString()) return;
}

static void parseDefine(FScanner& sc, FScriptPosition& pos)
{
	FString name;
	if (!sc.GetString(name))  return;
	if (!sc.GetNumber()) return;
	sc.AddSymbol(name.GetChars(), sc.Number);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseDefineTexture(FScanner& sc, FScriptPosition& pos)
{
	int tile, palette;

	if (!sc.GetNumber(tile, true)) return;
	if (!sc.GetNumber(palette, true))  return;
	if (!sc.GetNumber(true)) return; //formerly x-center, unused
	if (!sc.GetNumber(true)) return; //formerly y-center, unused
	if (!sc.GetNumber(true)) return; //formerly x-size, unused
	if (!sc.GetNumber(true)) return; //formerly y-size, unused
	if (!sc.GetString())  return;

	tileSetHightileReplacement(sc, tile, palette, sc.String, -1.0, 1.0, 1.0, 1.0, 1.0);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseTexturePaletteBlock(FScanner& sc, int tile)
{
	FScanner::SavedPos blockend;
	FScriptPosition pos = sc;

	int pal = -1, xsiz = 0, ysiz = 0;
	FString fn;
	float alphacut = -1.0, xscale = 1.0, yscale = 1.0, specpower = 1.0, specfactor = 1.0;
	bool indexed = false;

	if (!sc.GetNumber(pal, true)) return;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.GetString();
		if (sc.Compare("file")) sc.GetString(fn);
		else if (sc.Compare("alphacut")) sc.GetFloat(alphacut, true);
		else if (sc.Compare({ "xscale", "scale", "intensity", "detailscale" })) sc.GetFloat(xscale, true); // what's the point of all of these names?
		else if (sc.Compare("yscale")) sc.GetFloat(yscale, true);
		else if (sc.Compare({ "specpower", "specularpower", "parallaxscale" })) sc.GetFloat(specpower, true);
		else if (sc.Compare({ "specfactor", "specularfactor", "parallaxbias" })) sc.GetFloat(specfactor, true);
		else if (sc.Compare("orig_sizex")) sc.GetNumber(xsiz, true);
		else if (sc.Compare("orig_sizey")) sc.GetNumber(ysiz, true);
		else if (sc.Compare("indexed")) indexed = true;
	};

	if ((unsigned)tile < MAXUSERTILES)
	{
		if ((unsigned)pal >= MAXREALPAL) pos.Message(MSG_ERROR, "texture (%d): invalid palette number %d ", tile, pal);
		else if (fn.IsEmpty()) pos.Message(MSG_ERROR, "texture (%d): missing file name in palette definition", tile);
		else if (!fileSystem.FileExists(fn.GetChars())) pos.Message(MSG_ERROR, "texture (%d): file '%s' not found in palette definition", tile, fn.GetChars());
		else
		{
			if (xsiz > 0 && ysiz > 0)
			{
				tbuild->tile[tile].tileimage = createDummyTile(xsiz, ysiz);
			}
			xscale = 1.0f / xscale;
			yscale = 1.0f / yscale;

			tileSetHightileReplacement(sc, tile, pal, fn.GetChars(), alphacut, xscale, yscale, specpower, specfactor, indexed);
		}
	}
}

static void parseTextureSpecialBlock(FScanner& sc, int tile, int pal)
{
	FScanner::SavedPos blockend;
	FScriptPosition pos = sc;

	FString fn;
	float xscale = 1.0, yscale = 1.0, specpower = 1.0, specfactor = 1.0;

	if (sc.StartBraces(&blockend)) return; 
	while (!sc.FoundEndBrace(blockend))
	{
		sc.GetString();
		if (sc.Compare("file")) sc.GetString(fn);
		else if (sc.Compare({ "xscale", "scale", "intensity", "detailscale" })) sc.GetFloat(xscale, true); // what's the point of all of these names?
		else if (sc.Compare("yscale")) sc.GetFloat(yscale, true);
		else if (sc.Compare({ "specpower", "specularpower", "parallaxscale" })) sc.GetFloat(specpower, true);
		else if (sc.Compare({ "specfactor", "specularfactor", "parallaxbias" })) sc.GetFloat(specfactor, true);
	};

	if ((unsigned)tile < MAXUSERTILES)
	{
		if (fn.IsEmpty()) pos.Message(MSG_ERROR, "texture (%d): missing file name for layer definition", tile);
		else if (!fileSystem.FileExists(fn.GetChars())) pos.Message(MSG_ERROR, "texture (%d): file '%s' not found in layer definition", tile, fn.GetChars());
		else
		{
			if (pal == DETAILPAL)
			{
				xscale = 1.0f / xscale;
				yscale = 1.0f / yscale;
			}

			tileSetHightileReplacement(sc, tile, pal, fn.GetChars(), -1.f, xscale, yscale, specpower, specfactor);
		}
	}
}

static void parseTexture(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	int tile = -1;

	if (!sc.GetNumber(tile, true)) return;
	ValidateTilenum("texture", tile, pos); // do not abort, we still need to parse over the data.

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("pal")) parseTexturePaletteBlock(sc, tile);
		else if (sc.Compare("detail")) parseTextureSpecialBlock(sc, tile, DETAILPAL);
		else if (sc.Compare("glow")) parseTextureSpecialBlock(sc, tile, GLOWPAL);
		else if (sc.Compare("specular")) parseTextureSpecialBlock(sc, tile, SPECULARPAL);
		else if (sc.Compare("normal")) parseTextureSpecialBlock(sc, tile, NORMALPAL);
	}
}

//===========================================================================
//
//
//
//===========================================================================

static void parseUndefTexture(FScanner& sc, FScriptPosition& pos)
{
	if (!sc.GetNumber(true)) return;
	if (ValidateTilenum("undeftexture", sc.Number, pos)) tileRemoveReplacement(sc.Number);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseUndefTextureRange(FScanner& sc, FScriptPosition& pos)
{
	int start, end;
	if (!sc.GetNumber(start, true)) return;
	if (!sc.GetNumber(end, true)) return;
	if (ValidateTileRange("undeftexturerange", start, end, pos))
		for (int i = start; i <= end; i++) tileRemoveReplacement(i);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseTileFromTexture(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	TileImport imp;

	if (!sc.GetNumber(imp.tile, true)) return;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare({ "file", "name" })) sc.GetString(imp.fn);
		else if (sc.Compare("alphacut")) sc.GetNumber(imp.alphacut, true);
		else if (sc.Compare({ "xoff", "xoffset" })) sc.GetNumber(imp.xoffset, true);
		else if (sc.Compare({ "yoff", "yoffset" })) sc.GetNumber(imp.yoffset, true);
		else if (sc.Compare("texhitscan")) imp.flags |= PICANM_TEXHITSCAN_BIT;
		else if (sc.Compare("nofullbright")) imp.flags |= PICANM_NOFULLBRIGHT_BIT;
		else if (sc.Compare("texture")) imp.istexture = 1;
		else if (sc.Compare("ifcrc")) sc.GetNumber(imp.crc32, true);
		else if (sc.Compare("extra")) sc.GetNumber(imp.extra, true);
		else if (sc.Compare("surface")) sc.GetNumber(imp.surface, true);
		else if (sc.Compare("voxel")) sc.GetNumber(imp.vox, true);
		else if (sc.Compare("shade")) sc.GetNumber(imp.shade, true);
		else if (sc.Compare("view")) { sc.GetNumber(imp.extra, true); imp.extra &= 7; }
		else if (sc.Compare("ifmatch"))
		{
			FScanner::SavedPos blockend2;
			if (sc.StartBraces(&blockend2)) return;
			while (!sc.FoundEndBrace(blockend2))
			{
				sc.MustGetString();
				if (sc.Compare("size"))
				{
					sc.GetNumber(imp.sizex, true);
					sc.GetNumber(imp.sizey, true);
				}
				else if (sc.Compare("crc32")) sc.GetNumber(imp.crc32, true);
			}
		}
	}
	processTileImport(sc, "tilefromtexture", pos, imp);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseCopyTile(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	int tile = -1, source = -1;
	int havetile = 0, xoffset = -1024, yoffset = -1024;
	int flags = 0, temppal = -1, tempsource = -1;

	if (!sc.GetNumber(tile, true)) return;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("tile"))
		{
			if (sc.GetNumber(tempsource, true))
			{
				if (ValidateTilenum("copytile", tempsource, pos))
				{
					source = tempsource;
					havetile = true;
				}
			}
		}
		else if (sc.Compare("pal"))
		{
			// This is a bit messy because it doesn't wait until everything is parsed. Sadly that quirk needs to be replicated...
			if (sc.GetNumber(temppal, true))
			{
				// palettize self case
				if (!havetile)
				{
					if (ValidateTilenum("copytile", tile, pos)) havetile = true;
				}

				if ((unsigned)temppal >= MAXREALPAL)
				{
					pos.Message(MSG_ERROR, "copytile: palette number %d out of range (max=%d)\n", temppal, MAXREALPAL - 1);
					break;
				}
			}
		}
		else if (sc.Compare({ "xoff", "xoffset" })) sc.GetNumber(xoffset, true);
		else if (sc.Compare({ "yoff", "yoffset" })) sc.GetNumber(yoffset, true);
		else if (sc.Compare("texhitscan")) flags |= PICANM_TEXHITSCAN_BIT;
		else if (sc.Compare("nofullbright")) flags |= PICANM_NOFULLBRIGHT_BIT;
	}

	if (!ValidateTilenum("copytile", tile, pos)) return;
	// if !havetile, we have never confirmed a valid source
	if (!havetile && !ValidateTilenum("copytile", source, pos)) return;

	tileCopy(tile, source, temppal, xoffset, yoffset, flags);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseImportTile(FScanner& sc, FScriptPosition& pos)
{
	int tile;

	if (!sc.GetNumber(tile, true)) return;
	if (!sc.GetString())  return;
	if (!ValidateTilenum("importtile", tile, pos)) return;

	int texstatus = tileImportFromTexture(sc, sc.String, tile, 255, 0);
	if (texstatus >= 0) tbuild->tile[tile].extinfo.picanm = {};
}

//===========================================================================
//
//
//
//===========================================================================

static void parseDummyTile(FScanner& sc, FScriptPosition& pos)
{
	int tile, xsiz, ysiz;

	if (!sc.GetNumber(tile, true)) return;
	if (!sc.GetNumber(xsiz, true)) return;
	if (!sc.GetNumber(ysiz, true)) return;
	if (!ValidateTilenum("dummytile", tile, pos)) return;
	auto& tiled = tbuild->tile[tile];
	tiled.tileimage = createDummyTile(xsiz, ysiz);
	tiled.imported = nullptr;
}

//===========================================================================
//
//
//
//===========================================================================

static void parseDummyTileRange(FScanner& sc, FScriptPosition& pos)
{
	int tile1, tile2, xsiz, ysiz;

	if (!sc.GetNumber(tile1, true)) return;
	if (!sc.GetNumber(tile2, true)) return;
	if (!sc.GetNumber(xsiz, true)) return;
	if (!sc.GetNumber(ysiz, true)) return;
	if (!ValidateTileRange("dummytilerange", tile1, tile2, pos)) return;
	if (xsiz < 0 || ysiz < 0) return;

	for (int i = tile1; i <= tile2; i++)
	{
		auto& tiled = tbuild->tile[i];
		tiled.tileimage = createDummyTile(xsiz, ysiz);
		tiled.imported = nullptr;
	}
}

//===========================================================================
//
//
//
//===========================================================================

static void parseUndefineTile(FScanner& sc, FScriptPosition& pos)
{
	int tile;

	if (!sc.GetNumber(tile, true)) return;
	if (ValidateTilenum("undefinetile", tile, pos))
	{
		auto& tiled = tbuild->tile[tile];
		tiled.tileimage = nullptr;
		tiled.imported = nullptr;
	}
}

//===========================================================================
//
//
//
//===========================================================================

static void parseUndefineTileRange(FScanner& sc, FScriptPosition& pos)
{
	int tile1, tile2;

	if (!sc.GetNumber(tile1, true)) return;
	if (!sc.GetNumber(tile2, true)) return;
	if (!ValidateTileRange("undefinetilerange", tile1, tile2, pos)) return;

	for (int i = tile1; i <= tile2; i++)
	{
		auto& tiled = tbuild->tile[i];
		tiled.tileimage = nullptr;
		tiled.imported = nullptr;
	}
}

//===========================================================================
//
//
//
//===========================================================================

static void parseDefineSkybox(FScanner& sc, FScriptPosition& pos)
{
	int tile, palette;
	FString fn[6];

	if (!sc.GetNumber(tile, true)) return;
	if (!sc.GetNumber(palette, true))  return;
	if (!sc.GetNumber(true)) return; //'future extension' (for what?)
	for (int i = 0; i < 6; i++)
	{
		if (!sc.GetString()) return;
		fn[i] = sc.String;
	}
	tileSetSkybox(tile, palette, fn);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseSkybox(FScanner& sc, FScriptPosition& pos)
{
	FString faces[6];
	FScanner::SavedPos blockend;
	int tile = -1, pal = 0;
	bool indexed = false;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("tile")) sc.GetNumber(tile, true);
		else if (sc.Compare("pal")) sc.GetNumber(pal, true);
		else if (sc.Compare({ "ft", "front", "forward" })) sc.GetString(faces[0]);
		else if (sc.Compare({ "rt", "right" })) sc.GetString(faces[1]);
		else if (sc.Compare({ "bk", "back" })) sc.GetString(faces[2]);
		else if (sc.Compare({ "lt", "lf", "left" })) sc.GetString(faces[3]);
		else if (sc.Compare({ "up", "ceiling", "top", "ceil" })) sc.GetString(faces[4]);
		else if (sc.Compare({ "dn", "floor", "bottom", "down" })) sc.GetString(faces[5]);
		else if (sc.Compare("indexed")) indexed = true;
		// skip over everything else.
	}
	if (tile < 0) pos.Message(MSG_ERROR, "skybox: missing tile number");
	else tileSetSkybox(tile, pal, faces, indexed);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseSetupTile(FScanner& sc, FScriptPosition& pos)
{
	int tile;
	if (!sc.GetNumber(tile, true)) return;
	if (!ValidateTilenum("setuptile", tile, pos)) return;
	auto& tiled = tbuild->tile[tile];
	if (!sc.GetNumber(tiled.extinfo.hiofs.xsize, true)) return;
	if (!sc.GetNumber(tiled.extinfo.hiofs.ysize, true)) return;
	if (!sc.GetNumber(tiled.extinfo.hiofs.xoffs, true)) return;
	if (!sc.GetNumber(tiled.extinfo.hiofs.yoffs, true)) return;
}

//===========================================================================
//
//
//
//===========================================================================

static void parseSetupTileRange(FScanner& sc, FScriptPosition& pos)
{
	int tilestart, tileend;
	if (!sc.GetNumber(tilestart, true)) return;
	if (!sc.GetNumber(tileend, true)) return;
	if (!ValidateTileRange("setuptilerange", tilestart, tileend, pos)) return;

	TileOffs hiofs;
	if (!sc.GetNumber(hiofs.xsize, true)) return;
	if (!sc.GetNumber(hiofs.ysize, true)) return;
	if (!sc.GetNumber(hiofs.xoffs, true)) return;
	if (!sc.GetNumber(hiofs.yoffs, true)) return;

	for (int i = tilestart; i <= tileend; i++) 
		tbuild->tile[i].extinfo.hiofs = hiofs;
}

//===========================================================================
//
//
//
//===========================================================================

static void parseAnimTileRange(FScanner& sc, FScriptPosition& pos)
{
	SetAnim set;
	if (!sc.GetNumber(set.tile1, true)) return;
	if (!sc.GetNumber(set.tile2, true)) return;
	if (!sc.GetNumber(set.speed, true)) return;
	if (!sc.GetNumber(set.type, true)) return;
	processSetAnim("animtilerange", pos, set);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseAlphahack(FScanner& sc, FScriptPosition& pos)
{
	int tile;

	if (!sc.GetNumber(tile, true)) return;
	if (!sc.GetFloat(true)) return;
	if ((unsigned)tile < MAXTILES) tbuild->tile[tile].alphathreshold = (float)sc.Float;
}

//===========================================================================
//
//
//
//===========================================================================

static void parseAlphahackRange(FScanner& sc, FScriptPosition& pos)
{
	int tilestart, tileend;

	if (!sc.GetNumber(tilestart, true)) return;
	if (!sc.GetNumber(tileend, true)) return;
	if (!sc.GetFloat(true)) return;
	if (!ValidateTileRange("alphahackrange", tilestart, tileend, pos)) return;

	for (int i = tilestart; i <= tileend; i++)
		tbuild->tile[i].alphathreshold = (float)sc.Float;
}

//===========================================================================
//
//
//
//===========================================================================
static int lastvoxid = -1;

static void parseDefineVoxel(FScanner& sc, FScriptPosition& pos)
{
	sc.MustGetString();

	if (tbuild->nextvoxid == MAXVOXELS)
	{
		pos.Message(MSG_ERROR, "Maximum number of voxels (%d) already defined.", MAXVOXELS);
		return;
	}

	if (voxDefine(tbuild->nextvoxid, sc.String))
	{
		pos.Message(MSG_ERROR, "Unable to load voxel file \"%s\"", sc.String);
		return;
	}

	lastvoxid = tbuild->nextvoxid++;
}

//===========================================================================
//
//
//
//===========================================================================

static void parseDefineVoxelTiles(FScanner& sc, FScriptPosition& pos)
{
	int tilestart, tileend;
	if (!sc.GetNumber(tilestart, true)) return;
	if (!sc.GetNumber(tileend, true)) return;
	if (!ValidateTileRange("definevoxeltiles", tilestart, tileend, pos)) return;

	if (lastvoxid < 0)
	{
		pos.Message(MSG_WARNING, "Warning: Ignoring voxel tiles definition without valid voxel.\n");
		return;
	}
	for (int i = tilestart; i <= tileend; i++) tbuild->tile[i].extinfo.tiletovox = lastvoxid;
}

//===========================================================================
//
//
//
//===========================================================================

static void parseVoxel(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	int tile0 = MAXTILES, tile1 = -1;
	FString fn;
	bool error = false;

	if (!sc.GetString(fn)) return;

	if (tbuild->nextvoxid == MAXVOXELS)
	{
		pos.Message(MSG_ERROR, "Maximum number of voxels (%d) already defined.", MAXVOXELS);
		error = true;
	}
	else  if (voxDefine(tbuild->nextvoxid, fn.GetChars()))
	{
		pos.Message(MSG_ERROR, "Unable to load voxel file \"%s\"", fn.GetChars());
		error = true;
	}

	int lastvoxid = tbuild->nextvoxid++;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("tile"))
		{
			sc.GetNumber(true);
			if (ValidateTilenum("voxel", sc.Number, pos))
			{
				if (!error) tbuild->tile[sc.Number].extinfo.tiletovox = lastvoxid;
			}
		}
		if (sc.Compare("tile0")) sc.GetNumber(tile0, true);
		if (sc.Compare("tile1"))
		{
			sc.GetNumber(tile1, true);
			if (ValidateTileRange("voxel", tile0, tile1, pos) && !error)
			{
				for (int i = tile0; i <= tile1; i++) tbuild->tile[i].extinfo.tiletovox = lastvoxid;
			}
		}
		if (sc.Compare("scale"))
		{
			sc.GetFloat(true);
			if (!error) voxscale[lastvoxid] = (float)sc.Float;
		}
		if (sc.Compare("rotate") && !error) voxrotate.Set(lastvoxid);
	}
}

//===========================================================================
//
//
//
//===========================================================================

static void parseDefineTint(FScanner& sc, FScriptPosition& pos)
{
	int pal, r, g, b, f;

	if (!sc.GetNumber(pal, true)) return;
	if (!sc.GetNumber(r)) return;
	if (!sc.GetNumber(g)) return;
	if (!sc.GetNumber(b)) return;
	if (!sc.GetNumber(f)) return;
	lookups.setPaletteTint(pal, r, g, b, 0, 0, 0, f);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseFogpal(FScanner& sc, FScriptPosition& pos)
{
	int pal, r, g, b;

	if (!sc.GetNumber(pal, true)) return;
	if (!sc.GetNumber(r)) return;
	if (!sc.GetNumber(g)) return;
	if (!sc.GetNumber(b)) return;

	r = clamp(r, 0, 63);
	g = clamp(g, 0, 63);
	b = clamp(b, 0, 63);

	lookups.makeTable(pal, nullptr, r << 2, g << 2, b << 2, 1);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseNoFloorpalRange(FScanner& sc, FScriptPosition& pos)
{
	int start, end;
	if (!sc.GetNumber(start, true)) return;
	if (!sc.GetNumber(end, true)) return;
	if (start > 1) start = 1;
	if (end > MAXPALOOKUPS - 1) end = MAXPALOOKUPS - 1;
	for (int i = start; i <= end; i++)
		lookups.tables[i].noFloorPal = true;
}

//===========================================================================
//
//
//
//===========================================================================

static void parseTint(FScanner& sc, FScriptPosition& pos)
{
	int red = 255, green = 255, blue = 255, shadered = 0, shadegreen = 0, shadeblue = 0, pal = -1, flags = 0;
	FScanner::SavedPos blockend;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("pal")) sc.GetNumber(pal, true);
		else if (sc.Compare({"red", "r"})) sc.GetNumber(red);
		else if (sc.Compare({ "green", "g" })) sc.GetNumber(green);
		else if (sc.Compare({ "blue", "b" })) sc.GetNumber(blue);
		else if (sc.Compare({ "shadered", "sr" })) sc.GetNumber(shadered);
		else if (sc.Compare({ "shadegreen", "sg" })) sc.GetNumber(shadegreen);
		else if (sc.Compare({ "shadeblue", "sb" })) sc.GetNumber(shadeblue);
		else if (sc.Compare("flags")) sc.GetNumber(flags, true);
	}

	if (pal < 0)
		pos.Message(MSG_ERROR, "tint: palette number missing");
	else
		lookups.setPaletteTint(pal, clamp(red, 0, 255), clamp(green, 0, 255), clamp(blue, 0, 255), 
			clamp(shadered, 0, 255), clamp(shadegreen, 0, 255), clamp(shadeblue, 0, 255), flags);
}


//===========================================================================
//
//
//
//===========================================================================

static void parseMusic(FScanner& sc, FScriptPosition& pos)
{
	FString id, file;
	FScanner::SavedPos blockend;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("id")) sc.GetString(id);
		else if (sc.Compare("file")) sc.GetString(file);
	}
	SetMusicReplacement(id.GetChars(), file.GetChars());
}

//===========================================================================
//
//
//
//===========================================================================

static void parseMapinfo(FScanner& sc, FScriptPosition& pos)
{
	FString title;
	FString mhkfile;
	uint8_t md4b[16]{};

	FScanner::SavedPos blockend;
	TArray<FString> md4s;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("mapfile")) sc.GetString();
		else if (sc.Compare("maptitle")) sc.GetString(title);
		else if (sc.Compare("mhkfile")) sc.GetString(mhkfile);
		else if (sc.Compare("mapmd4"))
		{
			sc.GetString();
			md4s.Push(sc.String);
		}
	}
	for (auto& md4 : md4s)
	{
		for (int i = 0; i < 16; i++)
		{
			char smallbuf[3] = { md4[2 * i], md4[2 * i + 1], 0 };
			md4b[i] = (uint8_t)strtol(smallbuf, nullptr, 16);
		}
		AddUserMapHack(title, mhkfile, md4b);
	}
}

//===========================================================================
//
//
//
//===========================================================================

static void parseEcho(FScanner& sc, FScriptPosition& pos)
{
	sc.MustGetString();
	Printf("%s\n", sc.String);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseRffDefineId(FScanner& sc, FScriptPosition& pos)
{
	FString resName;
	FString resType;
	int resID;

	if (!sc.GetString(resName)) return;
	if (!sc.GetString(resType)) return;
	if (!sc.GetNumber(resID)) return;
	if (!sc.GetString()) return;
	resName.AppendFormat(".%s", resType.GetChars());
	fileSystem.CreatePathlessCopy(resName.GetChars(), resID, 0);
}

//===========================================================================
//
// empty stub
//
//===========================================================================

static void parseEmptyBlock(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;

	if (sc.StartBraces(&blockend)) return;
	sc.RestorePos(blockend);
	sc.CheckString("}");
}

static void parseEmptyBlockWithParm(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;

	sc.MustGetString();
	if (sc.StartBraces(&blockend)) return;
	sc.RestorePos(blockend);
	sc.CheckString("}");
}


//===========================================================================
//
//
//
//===========================================================================

static void parseTexHitscanRange(FScanner& sc, FScriptPosition& pos)
{
	int start, end;

	if (!sc.GetNumber(start, true)) return;
	if (!sc.GetNumber(end, true)) return;

	if (start < 0) start = 0;
	if (end >= MAXUSERTILES) end = MAXUSERTILES - 1;
	for (int i = start; i <= end; i++)
		tbuild->tile[i].extinfo.picanm.sf |= PICANM_TEXHITSCAN_BIT;
}

//===========================================================================
//
//
//
//===========================================================================

static void parseNoFullbrightRange(FScanner& sc, FScriptPosition& pos)
{
	int start, end;

	if (!sc.GetNumber(start, true)) return;
	if (!sc.GetNumber(end, true)) return;

	if (start < 0) start = 0;
	if (end >= MAXUSERTILES) end = MAXUSERTILES - 1;
	for (int i = start; i <= end; i++)
	{
		tbuild->tile[i].extinfo.picanm.sf |= PICANM_NOFULLBRIGHT_BIT;
	}
}

//===========================================================================
//
// 
//
//===========================================================================

static void parseHighpalookup(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	int basepal = -1, pal = -1;
	FString fn;

	if (sc.StartBraces(&blockend)) return;

	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("basepal")) sc.GetNumber(basepal);
		else if (sc.Compare("pal")) sc.GetNumber(pal);
		else if (sc.Compare("file")) sc.GetString(fn);
	}
	if ((unsigned)basepal >= MAXBASEPALS)
	{
		pos.Message(MSG_ERROR, "highpalookup: invalid base palette number %d", basepal);
	}
	else if ((unsigned)pal >= MAXREALPAL)
	{
		pos.Message(MSG_ERROR, "highpalookup: invalid palette number %d", pal);
	}
	else if (fn.IsEmpty())
	{
		pos.Message(MSG_ERROR, "highpalookup: missing file name");
	}
	else if (!fileSystem.FileExists(fn.GetChars()))
	{
		pos.Message(MSG_ERROR, "highpalookup: file %s not found", fn.GetChars());
	}
	// todo
}


struct ModelStatics
{
	int lastmodelid;
	int modelskin, lastmodelskin;
	int seenframe;
} mdglobal;

//===========================================================================
//
// 
//
//===========================================================================

static void parseDefineModel(FScanner& sc, FScriptPosition& pos)
{
	FString modelfn;
	double scale;
	int shadeoffs;

	if (!sc.GetString(modelfn)) return;
	if (!sc.GetFloat(scale, true)) return;
	if (!sc.GetNumber(shadeoffs, true)) return;

	mdglobal.lastmodelid = modelManager.LoadModel(modelfn.GetChars());
	if (mdglobal.lastmodelid < 0)
	{
		pos.Message(MSG_WARNING, "definemodel: unable to load model file '%s'", modelfn.GetChars());
	}
	else
	{
		modelManager.SetMisc(mdglobal.lastmodelid, (float)scale, shadeoffs, 0.0, 0.0, 0);
		mdglobal.modelskin = mdglobal.lastmodelskin = 0;
		mdglobal.seenframe = 0;
	}
}

//===========================================================================
//
// 
//
//===========================================================================

static void parseDefineModelFrame(FScanner& sc, FScriptPosition& pos)
{
	FString framename;
	bool ok = true;
	int firsttile, lasttile;

	if (!sc.GetString(framename)) return;
	if (!sc.GetNumber(firsttile, true)) return;
	if (!sc.GetNumber(lasttile, true)) return;

	if (!ValidateTileRange("definemodelframe", firsttile, lasttile, pos)) return;

	if (mdglobal.lastmodelid < 0)
	{
		pos.Message(MSG_WARNING, "definemodelframe: Ignoring frame definition outside model.");
		return;
	}
	for (int i = firsttile; i <= lasttile && ok; i++)
	{
		int err = (modelManager.DefineFrame(mdglobal.lastmodelid, framename.GetChars(), i, max(0, mdglobal.modelskin), 0.0f, 0));
		if (err < 0) ok = false; 
		if (err == -2) pos.Message(MSG_ERROR, "Invalid tile number %d", i);
		else if (err == -3) pos.Message(MSG_ERROR, "Invalid frame name '%s'", framename.GetChars());
	}
	mdglobal.seenframe = 1;
}

//===========================================================================
//
// 
//
//===========================================================================

static void parseDefineModelAnim(FScanner& sc, FScriptPosition& pos)
{
	FString startframe, endframe;
	int32_t flags;
	double dfps;

	if (!sc.GetString(startframe)) return;
	if (!sc.GetString(endframe)) return;
	if (!sc.GetFloat(dfps, true)) return;
	if (!sc.GetNumber(flags, true)) return;

	if (mdglobal.lastmodelid < 0)
	{
		pos.Message(MSG_WARNING, "definemodelframe: Ignoring animation definition outside model.");
		return;
	}
	int err = (modelManager.DefineAnimation(mdglobal.lastmodelid, startframe.GetChars(), endframe.GetChars(), (int32_t)(dfps * (65536.0 * .001)), flags));
	if (err == -2) pos.Message(MSG_ERROR, "Invalid start frame name %s", startframe.GetChars());
	else if (err == -3) pos.Message(MSG_ERROR, "Invalid end frame name %s", endframe.GetChars());
}

//===========================================================================
//
// 
//
//===========================================================================

static void parseDefineModelSkin(FScanner& sc, FScriptPosition& pos)
{
	int palnum;
	FString skinfn;

	if (!sc.GetNumber(palnum, true)) return;
	if (!sc.GetString(skinfn)) return;

	if (mdglobal.seenframe) { mdglobal.modelskin = ++mdglobal.lastmodelskin; }
	mdglobal.seenframe = 0;

	if (!fileSystem.FileExists(skinfn.GetChars())) return;

	int err = (modelManager.DefineSkin(mdglobal.lastmodelid, skinfn.GetChars(), palnum, max(0, mdglobal.modelskin), 0, 0.0f, 1.0f, 1.0f, 0));
	if (err == -2) pos.Message(MSG_ERROR, "Invalid skin file name %s", skinfn.GetChars());
	else if (err == -3) pos.Message(MSG_ERROR, "Invalid palette %d", palnum);
}

//===========================================================================
//
// 
//
//===========================================================================

static void parseSelectModelSkin(FScanner& sc, FScriptPosition& pos)
{
	sc.GetNumber(mdglobal.modelskin, true);
}


//===========================================================================
//
// 
//
//===========================================================================

static void parseUndefModel(FScanner& sc, FScriptPosition& pos)
{
	int tile;
	if (!sc.GetNumber(tile, true)) return;
	if (!ValidateTilenum("undefmodel", tile, pos)) return;
	modelManager.UndefineTile(tile);
}

static void parseUndefModelRange(FScanner& sc, FScriptPosition& pos)
{
	int start, end;

	if (!sc.GetNumber(start, true)) return;
	if (!sc.GetNumber(end, true)) return;
	if (!ValidateTileRange("undefmodel", start, end, pos)) return;
	for (int i = start; i <= end; i++) modelManager.UndefineTile(i);
}

static void parseUndefModelOf(FScanner& sc, FScriptPosition& pos)
{
	int tile;
	if (!sc.GetNumber(tile, true)) return;
	if (!ValidateTilenum("undefmodelof", tile, pos)) return;
	pos.Message(MSG_WARNING, "undefmodelof: currently non-functional.");
}

//===========================================================================
//
// 
//
//===========================================================================

static bool parseModelFrameBlock(FScanner& sc, FixedBitArray<1024>& usedframes)
{
	FScanner::SavedPos blockend;
	FScriptPosition pos = sc;

	FString framename;
	bool ok = true;
	int pal = -1;
	int starttile = -1, endtile = -1;
	float smoothduration = 0.1f;

	if (sc.StartBraces(&blockend)) return false;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("pal")) sc.GetNumber(pal, true);
		else if (sc.Compare({ "name", "frame" })) sc.GetString(framename);
		else if (sc.Compare("tile")) { sc.GetNumber(starttile, true); endtile = starttile; }
		else if (sc.Compare("tile0")) sc.GetNumber(starttile, true);
		else if (sc.Compare("tile1")) sc.GetNumber(endtile, true);
		else if (sc.Compare("smoothduration")) sc.GetFloat(smoothduration, true);
	}

	if (!ValidateTileRange("model/frame", starttile, endtile, pos)) return false;

	if (smoothduration > 1.0)
	{
		pos.Message(MSG_WARNING, "smoothduration out of range");
		smoothduration = 1.0;
	}
	for (int i = starttile; i <= endtile && ok; i++)
	{
		int res = modelManager.DefineFrame(mdglobal.lastmodelid, framename.GetChars(), i, max(0, mdglobal.modelskin), smoothduration, pal);
		if (res < 0)
		{
			ok = false;
			if (res == -2) pos.Message(MSG_WARNING, "Invalid tile number %d", i);
			else if (res == -3) pos.Message(MSG_WARNING, "%s: Invalid frame name", framename.GetChars());
		}
		else if (res < 1024) usedframes.Set(res);
	}
	mdglobal.seenframe = 1;
	return ok;
}

static bool parseModelAnimBlock(FScanner& sc)
{
	FScanner::SavedPos blockend;
	FScriptPosition pos = sc;

	FString startframe, endframe;
	int flags = 0;
	double fps = 1.0;

	if (sc.StartBraces(&blockend)) return false;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("frame0")) sc.GetString(startframe);
		else if (sc.Compare("frame1")) sc.GetString(endframe);
		else if (sc.Compare("fps")) sc.GetFloat(fps, true);
		else if (sc.Compare("flags")) sc.GetNumber(flags, true);
	}

	if (startframe.IsEmpty())
	{
		pos.Message(MSG_ERROR, "missing start frame for anim definition");
		return false;
	}
	if (endframe.IsEmpty())
	{
		pos.Message(MSG_ERROR, "missing end frame for anim definition");
		return false;
	}

	int res = modelManager.DefineAnimation(mdglobal.lastmodelid, startframe.GetChars(), endframe.GetChars(), (int)(fps * (65536.0 * .001)), flags);
	if (res < 0)
	{
		if (res == -2) pos.Message(MSG_ERROR, "Invalid start frame name %s", startframe.GetChars());
		else if (res == -3) pos.Message(MSG_ERROR, "Invalid end frame name %s", endframe.GetChars());
		return false;
	}
	return true;
}

static bool parseModelSkinBlock(FScanner& sc, int pal)
{
	FScanner::SavedPos blockend;
	FScriptPosition pos = sc;

	FString filename;
	int surface = 0;
	float param = 1.0, specpower = 1.0, specfactor = 1.0;
	int flags = 0;

	if (sc.StartBraces(&blockend)) return false;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("pal")) sc.GetNumber(pal, true);
		else if (sc.Compare("file")) sc.GetString(filename);
		else if (sc.Compare({ "surface", "surf" })) sc.GetNumber(surface, true);
		else if (sc.Compare({ "intensity", "scale", "detailscale" })) sc.GetFloat(param, true);
		else if (sc.Compare({ "specpower", "specularpower", "parallaxscale" })) sc.GetFloat(specpower, true);
		else if (sc.Compare({ "specfactor", "specularfactor", "parallaxbias" })) sc.GetFloat(specfactor, true);
		else if (sc.Compare("forcefilter")) { /* not suppoted yet*/ }
	}


	if (filename.IsEmpty())
	{
		pos.Message(MSG_ERROR, "missing 'skin filename' for skin definition");
		return false;
	}

	if (mdglobal.seenframe) mdglobal.modelskin = ++mdglobal.lastmodelskin;
	mdglobal.seenframe = 0;

	if (!fileSystem.FileExists(filename.GetChars()))
	{
		pos.Message(MSG_ERROR, "%s: file not found", filename.GetChars());
		return false;
	}

	if (pal == DETAILPAL) param = 1.f / param;
	int res = modelManager.DefineSkin(mdglobal.lastmodelid, filename.GetChars(), pal, max(0, mdglobal.modelskin), surface, param, specpower, specfactor, flags);
	if (res < 0)
	{
		if (res == -2) pos.Message(MSG_ERROR, "Invalid skin filename %s", filename.GetChars());
		else if (res == -3) pos.Message(MSG_ERROR, "Invalid palette number %d", pal);
		return false;
	}
	return true;
}

static bool parseModelHudBlock(FScanner& sc)
{
	FScanner::SavedPos blockend;
	FScriptPosition pos = sc;

	int starttile = -1, endtile = -1, flags = 0, fov = -1, angadd = 0;
	DVector3 add{};

	if (sc.StartBraces(&blockend)) return false;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("tile")) { sc.GetNumber(starttile, true); endtile = starttile; }
		else if (sc.Compare("tile0")) sc.GetNumber(starttile, true);
		else if (sc.Compare("tile1")) sc.GetNumber(endtile, true);
		else if (sc.Compare("xadd")) sc.GetFloat(add.X, true);
		else if (sc.Compare("yadd")) sc.GetFloat(add.Y, true);
		else if (sc.Compare("zadd")) sc.GetFloat(add.Z, true);
		else if (sc.Compare("angadd")) sc.GetNumber(angadd, true);
		else if (sc.Compare("fov")) sc.GetNumber(fov, true);
		else if (sc.Compare("hide")) flags |= HUDFLAG_HIDE;
		else if (sc.Compare("nobob")) flags |= HUDFLAG_NOBOB;
		else if (sc.Compare("flipped")) flags |= HUDFLAG_FLIPPED;
		else if (sc.Compare("nodepth")) flags |= HUDFLAG_NODEPTH;
	}

	if (!ValidateTileRange("hud", starttile, endtile, pos)) return false;

	for (int i = starttile; i <= endtile; i++)
	{
		FVector3 addf = { (float)add.X, (float)add.Y, (float)add.Z };
		int res = modelManager.DefineHud(mdglobal.lastmodelid, i, addf, angadd, flags, fov);
		if (res < 0)
		{
			if (res == -2) pos.Message(MSG_ERROR, "Invalid tile number %d", i);
			return false;
		}
	}
	return true;
}

static void parseModel(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;

	FString modelfn;
	double scale = 1.0, mzadd = 0.0, myoffset = 0.0;
	int32_t shadeoffs = 0, flags = 0;
	FixedBitArray<1024> usedframes;

	usedframes.Zero();
	mdglobal.modelskin = mdglobal.lastmodelskin = 0;
	mdglobal.seenframe = 0;

	if (!sc.GetString(modelfn)) return;

	if (sc.StartBraces(&blockend)) return;

	mdglobal.lastmodelid = modelManager.LoadModel(modelfn.GetChars());
	if (mdglobal.lastmodelid < 0)
	{
		pos.Message(MSG_WARNING, "Unable to load model file '%s'", modelfn.GetChars());
		sc.RestorePos(blockend);
		sc.CheckString("}");
		return;
	}

	bool ok = true;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("scale")) sc.GetFloat(scale, true);
		else if (sc.Compare("shade")) sc.GetNumber(shadeoffs, true);
		else if (sc.Compare("zadd")) sc.GetFloat(mzadd, true);
		else if (sc.Compare("yoffset")) sc.GetFloat(myoffset, true);
		else if (sc.Compare("frame")) ok &= parseModelFrameBlock(sc, usedframes);
		else if (sc.Compare("anim")) ok &= parseModelAnimBlock(sc);
		else if (sc.Compare("skin")) ok &= parseModelSkinBlock(sc, 0);
		else if (sc.Compare("detail")) ok &= parseModelSkinBlock(sc, DETAILPAL);
		else if (sc.Compare("glow")) ok &= parseModelSkinBlock(sc, GLOWPAL);
		else if (sc.Compare("specular")) ok &= parseModelSkinBlock(sc, SPECULARPAL);
		else if (sc.Compare("normal")) ok &= parseModelSkinBlock(sc, NORMALPAL);
		else if (sc.Compare("hud")) ok &= parseModelHudBlock(sc);
		else if (sc.Compare("flags")) sc.GetNumber(flags, true);
	}

	if (!ok)
	{
		if (mdglobal.lastmodelid >= 0)
		{
			pos.Message(MSG_ERROR, "Removing model %d due to errors.", mdglobal.lastmodelid);
			modelManager.UndefineModel(mdglobal.lastmodelid);
		}
	}
	else
	{
		modelManager.SetMisc(mdglobal.lastmodelid, (float)scale, shadeoffs, (float)mzadd, (float)myoffset, flags);
		mdglobal.modelskin = mdglobal.lastmodelskin = 0;
		mdglobal.seenframe = 0;
	}
}


//===========================================================================
//
// 
//
//===========================================================================

static bool parseDefineQAVInterpolateIgnoreBlock(FScanner& sc, const int res_id, TMap<int, TArray<int>>& ignoredata, const int numframes)
{
	FScanner::SavedPos blockend;
	FScriptPosition pos = sc;

	FString scframes, sctiles;
	TArray<int> framearray, tilearray;

	if (sc.StartBraces(&blockend))
	{
		pos.Message(MSG_ERROR, "defineqav (%d): interpolate: ignore: malformed syntax, unable to continue", res_id);
		return false;
	}
	while (!sc.FoundEndBrace(blockend))
	{
		sc.GetString();
		if (sc.Compare("frames")) sc.GetString(scframes);
		else if (sc.Compare("tiles")) sc.GetString(sctiles);
	}

	// Confirm we received something for 'frames' and 'tiles'.
	if (scframes.IsEmpty() || sctiles.IsEmpty())
	{
		pos.Message(MSG_ERROR, "defineqav (%d): interpolate: ignore: unable to get any values for 'frames' or 'tiles', unable to continue", res_id);
		return false;
	}

	auto arraybuilder = [&](const FString& input, TArray<int>& output, const int maxvalue) -> bool
	{
		if (input.CompareNoCase("all") == 0)
		{
			// All indices from 0 through to maxvalue are to be added to output array.
			output.Push(0);
			output.Push(maxvalue);
		}
		else if (input.IndexOf("-") != -1)
		{
			// Input is a range of values, split on the hypthen and add each value to the output array.
			auto temparray = input.Split("-");
			if (temparray.Size() == 2)
			{
				// Test if keywords 'first' and 'last' have been used.'
				output.Push(temparray[0].CompareNoCase("first") == 0 ? 0 : atoi(temparray[0].GetChars()));
				output.Push(temparray[1].CompareNoCase("last") == 0 ? maxvalue : atoi(temparray[1].GetChars()));
			}
		}
		else
		{
			// We just have a number. Convert the string into an int and push it twice to the output array.
			auto tempvalue = atoi(input.GetChars());
			for (auto i = 0; i < 2; i++) output.Push(tempvalue);
		}
		if (output.Size() != 2 || output[0] > output[1] || output[0] < 0 || output[1] > maxvalue)
		{
			pos.Message(MSG_ERROR, "defineqav (%d): interpolate: ignore: value of '%s' is malformed, unable to continue", res_id, input.GetChars());
			return false;
		}
		return true;
	};

	if (!arraybuilder(scframes, framearray, numframes - 1)) return false;
	if (!arraybuilder(sctiles, tilearray, 7)) return false;

	// Process arrays and add ignored frames as required.
	for (auto i = framearray[0]; i <= framearray[1]; i++)
	{
		auto& frametiles = ignoredata[i];
		for (auto j = tilearray[0]; j <= tilearray[1]; j++)
		{
			if (!frametiles.Contains(j)) frametiles.Push(j);
		}
	}
	return true;
}

static bool parseDefineQAVInterpolateBlock(FScanner& sc, const int res_id, const int numframes)
{
	FScanner::SavedPos blockend;
	FScriptPosition pos = sc;

	FString interptype;
	bool loopable = false;
	TMap<int, TArray<int>> ignoredata;

	if (sc.StartBraces(&blockend))
	{
		pos.Message(MSG_ERROR, "defineqav (%d): interpolate: malformed syntax, unable to continue", res_id);
		return false;
	}
	while (!sc.FoundEndBrace(blockend))
	{
		sc.GetString();
		if (sc.Compare("type"))
		{
			if (interptype.IsNotEmpty())
			{
				pos.Message(MSG_ERROR, "defineqav (%d): interpolate: more than one interpolation type defined, unable to continue", res_id);
				return false;
			}
			sc.GetString(interptype);
			if (!gi->IsQAVInterpTypeValid(interptype))
			{
				pos.Message(MSG_ERROR, "defineqav (%d): interpolate: interpolation type not found", res_id);
				return false;
			}
		}
		else if (sc.Compare("loopable")) loopable = true;
		else if (sc.Compare("ignore")) if (!parseDefineQAVInterpolateIgnoreBlock(sc, res_id, ignoredata, numframes)) return false;
	}

	// Add interpolation properties to game for processing while drawing.
	gi->AddQAVInterpProps(res_id, interptype, loopable, std::move(ignoredata));
	return true;
}

static void parseDefineQAV(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	FString fn;
	int res_id = -1;
	int numframes = -1;
	bool interpolate = false;

	if (!sc.GetNumber(res_id, true))
	{
		pos.Message(MSG_ERROR, "defineqav: invalid or non-defined resource ID");
		return;
	}

	if (sc.StartBraces(&blockend))
	{
		pos.Message(MSG_ERROR, "defineqav (%d): malformed syntax, unable to continue", res_id);
		return;
	}
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("file"))
		{
			if (fn.IsNotEmpty())
			{
				pos.Message(MSG_ERROR, "defineqav (%d): more than one file defined, unable to continue", res_id);
				return;
			}
			sc.GetString(fn);

			// Test file's validity.
			FixPathSeperator(fn);
			auto lump = fileSystem.FindFile(fn.GetChars());
			if (lump < 0)
			{
				pos.Message(MSG_ERROR, "defineqav (%d): file '%s' could not be found", res_id, fn.GetChars());
				return;
			}

			// Read file to get number of frames from QAV, skipping first 8 bytes.
			auto fr = fileSystem.OpenFileReader(lump);
			fr.ReadUInt64();
			numframes = fr.ReadInt32();
		}
		else if (sc.Compare("interpolate"))
		{
			if (interpolate)
			{
				pos.Message(MSG_ERROR, "defineqav (%d): more than one interpolate block defined, unable to continue", res_id);
				return;
			}
			interpolate = true;
			if (!parseDefineQAVInterpolateBlock(sc, res_id, numframes)) return;
		}
	}

	// If we're not interpolating, remove any reference to interpolation data for this res_id.
	if (!interpolate) gi->RemoveQAVInterpProps(res_id);

	// Add new file to filesystem.
	fileSystem.CreatePathlessCopy(fn.GetChars(), res_id, 0);
}

//===========================================================================
//
// 
//
//===========================================================================

struct dispatch
{
	const char* text;
	void (*handler)(FScanner& sc, FScriptPosition& pos);
};


static const dispatch basetokens[] =
{
	{ "include",         parseInclude          },
	{ "#include",        parseInclude          },
	{ "includedefault",  parseIncludeDefault   },
	{ "#includedefault", parseIncludeDefault   },
	{ "define",          parseDefine           },
	{ "#define",         parseDefine           },

	// deprecated style
	{ "definetexture",   parseDefineTexture    },
	{ "defineskybox",    parseDefineSkybox     },
	{ "definetint",      parseDefineTint       },
	{ "definemodel",     parseDefineModel      },
	{ "definemodelframe",parseDefineModelFrame },
	{ "definemodelanim", parseDefineModelAnim  },
	{ "definemodelskin", parseDefineModelSkin  },
	{ "selectmodelskin", parseSelectModelSkin  },
	{ "definevoxel",     parseDefineVoxel      },
	{ "definevoxeltiles",parseDefineVoxelTiles },

	// new style
	{ "model",           parseModel            },
	{ "voxel",           parseVoxel            },
	{ "skybox",          parseSkybox           },
	{ "highpalookup",    parseHighpalookup     },
	{ "tint",            parseTint             },
	{ "texture",         parseTexture          },
	{ "tile",            parseTexture          },
	{ "music",           parseMusic            },
	{ "sound",           parseEmptyBlock       },
	{ "animsounds",      parseEmptyBlockWithParm       },
	{ "cutscene",        parseEmptyBlockWithParm         },
	{ "nofloorpalrange", parseNoFloorpalRange  },
	{ "texhitscanrange", parseTexHitscanRange  },
	{ "nofullbrightrange", parseNoFullbrightRange },
	// other stuff
	{ "undefmodel",      parseUndefModel       },
	{ "undefmodelrange", parseUndefModelRange  },
	{ "undefmodelof",    parseUndefModelOf     },
	{ "undeftexture",    parseUndefTexture     },
	{ "undeftexturerange", parseUndefTextureRange },
	{ "alphahack",	     parseAlphahack 		},
	{ "alphahackrange",  parseAlphahackRange 	},
	{ "spritecol",	     parseSkip<3> 			},
	{ "2dcol",	     	 parseSkip<4> 			},
	{ "2dcolidxrange",   parseSkip<3>			},
	{ "fogpal",	     	 parseFogpal	 		},
	{ "loadgrp",     	 parseSkip<1>	 		},
	{ "dummytile",     	 parseDummyTile			},
	{ "dummytilerange",  parseDummyTileRange   },
	{ "setuptile",       parseSetupTile        },
	{ "setuptilerange",  parseSetupTileRange   },
	{ "undefinetile",    parseUndefineTile		},
	{ "undefinetilerange", parseUndefineTileRange },
	{ "animtilerange",   parseAnimTileRange    },
	{ "cachesize",       parseSkip<1>			},
	{ "dummytilefrompic",parseImportTile       },
	{ "tilefromtexture", parseTileFromTexture  },
	{ "mapinfo",         parseMapinfo          },
	{ "echo",            parseEcho             },
	{ "globalflags",     parseSkip<1>      },
	{ "copytile",        parseCopyTile         },
	{ "globalgameflags", parseSkip<1>  },
	{ "multipsky",       parseEmptyBlockWithParm  },
	{ "undefblendtablerange", parseSkip<2> },
	{ "shadefactor",     parseSkip<1>      },
	{ "newgamechoices",  parseEmptyBlock   },
	{ "rffdefineid",     parseRffDefineId      },
	{ "defineqav",       parseDefineQAV        },

	{ nullptr,           nullptr               },
};

static void defsparser(FScanner& sc)
{
	int iter = 0;

	sc.SetNoFatalErrors(true);
	sc.SetNoOctals(true);
	while (1)
	{
		if (++iter >= 50)
		{
			Printf(".");
			iter = 0;
		}
		FScriptPosition pos = sc;
		if (!sc.GetString()) return;
		int index = sc.MustMatchString(&basetokens[0].text, sizeof(basetokens[0]));
		if (index != -1) basetokens[index].handler(sc, pos);
	}
}

void loaddefinitionsfile(TilesetBuildInfo& info, const char* fn, bool cumulative, bool maingame)
{
	tbuild = &info;
	bool done = false;
	auto parseit = [&](int lump)
	{
		FScanner sc;
		sc.OpenLumpNum(lump);
		defsparser(sc);
		done = true;
		Printf(PRINT_NONOTIFY, "\n");
	};

	cycle_t deftimer;
	deftimer.Reset();

	auto printtimer = [&](const char* fn)
	{
		deftimer.Unclock();
		DPrintf(DMSG_SPAMMY, "Definitions file \"%s\" loaded, %f ms.\n", fn, deftimer.TimeMS());
		deftimer.Reset();
	};

	if (!cumulative)
	{
		int lump = fileSystem.FindFile(fn);
		if (lump >= 0)
		{
			Printf(PRINT_NONOTIFY, "Loading \"%s\"\n", fn);
			deftimer.Clock();
			parseit(lump);
			printtimer(fn);
		}
	}
	else
	{
		int lump, lastlump = 0;
		while ((lump = fileSystem.FindLumpFullName(fn, &lastlump)) >= 0)
		{
			if (maingame && fileSystem.GetFileContainer(lump) > fileSystem.GetMaxIwadNum()) break;
			Printf(PRINT_NONOTIFY, "Loading \"%s\"\n", fileSystem.GetFileFullPath(lump).c_str());
			deftimer.Clock();
			parseit(lump);
			printtimer(fn);
		}
	}
}
