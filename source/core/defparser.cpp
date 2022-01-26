
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

#include "mdsprite.h"  // md3model_t
#include "buildtiles.h"
#include "bitmap.h"
#include "m_argv.h"
#include "gamestruct.h"
#include "gamecontrol.h"
#include "palettecontainer.h"
#include "mapinfo.h"
#include "hw_voxels.h"
#include "psky.h"

int tileSetHightileReplacement(int picnum, int palnum, const char* filename, float alphacut, float xscale, float yscale, float specpower, float specfactor, bool indexed = false);
int tileSetSkybox(int picnum, int palnum, FString* facenames, bool indexed = false);
void tileRemoveReplacement(int num);
void AddUserMapHack(usermaphack_t&);

static void defsparser(FScanner& sc);

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
//
//
//===========================================================================

template<int cnt>
void parseSkip(FScanner& sc, FScriptPosition& pos)
{
	for (int i = 0; i < cnt; i++) if (!sc.GetString()) return;
}

void parseDefine(FScanner& sc, FScriptPosition& pos)
{
	FString name;
	if (!sc.GetString(name))  return;
	if (!sc.GetNumber()) return;
	sc.AddSymbol(name, sc.Number);
}

//===========================================================================
//
//
//
//===========================================================================

void parseDefineTexture(FScanner& sc, FScriptPosition& pos)
{
	int tile, palette;

	if (!sc.GetNumber(tile, true)) return;
	if (!sc.GetNumber(palette, true))  return;
	if (!sc.GetNumber(true)) return; //formerly x-center, unused
	if (!sc.GetNumber(true)) return; //formerly y-center, unused
	if (!sc.GetNumber(true)) return; //formerly x-size, unused
	if (!sc.GetNumber(true)) return; //formerly y-size, unused
	if (!sc.GetString())  return;

	tileSetHightileReplacement(tile, palette, sc.String, -1.0, 1.0, 1.0, 1.0, 1.0);
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
		else if (!fileSystem.FileExists(fn)) pos.Message(MSG_ERROR, "texture (%d): file '%s' not found in palette definition", tile, fn.GetChars());
		else
		{
			if (xsiz > 0 && ysiz > 0)
			{
				tileSetDummy(tile, xsiz, ysiz);
			}
			xscale = 1.0f / xscale;
			yscale = 1.0f / yscale;

			tileSetHightileReplacement(tile, pal, fn, alphacut, xscale, yscale, specpower, specfactor, indexed);
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
		else if (!fileSystem.FileExists(fn)) pos.Message(MSG_ERROR, "texture (%d): file '%s' not found in layer definition", tile, fn.GetChars());
		else
		{
			if (pal == DETAILPAL)
			{
				xscale = 1.0f / xscale;
				yscale = 1.0f / yscale;
			}

			tileSetHightileReplacement(tile, pal, fn, -1.f, xscale, yscale, specpower, specfactor);
		}
	}
}

void parseTexture(FScanner& sc, FScriptPosition& pos)
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

void parseUndefTexture(FScanner& sc, FScriptPosition& pos)
{
	if (!sc.GetNumber(true)) return;
	if (ValidateTilenum("undeftexture", sc.Number, pos)) tileRemoveReplacement(sc.Number);
}

//===========================================================================
//
//
//
//===========================================================================

void parseUndefTextureRange(FScanner& sc, FScriptPosition& pos)
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

void parseTileFromTexture(FScanner& sc, FScriptPosition& pos)
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
	processTileImport("tilefromtexture", pos, imp);
}

//===========================================================================
//
//
//
//===========================================================================

void parseCopyTile(FScanner& sc, FScriptPosition& pos)
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

void parseImportTile(FScanner& sc, FScriptPosition& pos)
{
	int tile;

	if (!sc.GetNumber(tile, true)) return;
	if (!sc.GetString())  return;
	if (!ValidateTilenum("importtile", tile, pos)) return;

	int texstatus = tileImportFromTexture(sc.String, tile, 255, 0);
	if (texstatus >= 0) TileFiles.tiledata[tile].picanm = {};
}

//===========================================================================
//
//
//
//===========================================================================

void parseDummyTile(FScanner& sc, FScriptPosition& pos)
{
	int tile, xsiz, ysiz;

	if (!sc.GetNumber(tile, true)) return;
	if (!sc.GetNumber(xsiz, true)) return;
	if (!sc.GetNumber(ysiz, true)) return;
	if (!ValidateTilenum("dummytile", tile, pos)) return;
	tileSetDummy(tile, xsiz, ysiz);
}

//===========================================================================
//
//
//
//===========================================================================

void parseDummyTileRange(FScanner& sc, FScriptPosition& pos)
{
	int tile1, tile2, xsiz, ysiz;

	if (!sc.GetNumber(tile1, true)) return;
	if (!sc.GetNumber(tile2, true)) return;
	if (!sc.GetNumber(xsiz, true)) return;
	if (!sc.GetNumber(ysiz, true)) return;
	if (!ValidateTileRange("dummytilerange", tile1, tile2, pos)) return;
	if (xsiz < 0 || ysiz < 0) return;

	for (int i = tile1; i <= tile2; i++) tileSetDummy(i, xsiz, ysiz);
}

//===========================================================================
//
//
//
//===========================================================================

void parseUndefineTile(FScanner& sc, FScriptPosition& pos)
{
	int tile;

	if (!sc.GetNumber(tile, true)) return;
	if (ValidateTilenum("undefinetile", tile, pos)) 
		tileDelete(tile);
}

//===========================================================================
//
//
//
//===========================================================================

void parseUndefineTileRange(FScanner& sc, FScriptPosition& pos)
{
	int tile1, tile2;

	if (!sc.GetNumber(tile1, true)) return;
	if (!sc.GetNumber(tile2, true)) return;
	if (!ValidateTileRange("undefinetilerange", tile1, tile2, pos)) return;

	for (int i = tile1; i <= tile2; i++) tileDelete(i);
}

//===========================================================================
//
//
//
//===========================================================================

void parseDefineSkybox(FScanner& sc, FScriptPosition& pos)
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

void parseSkybox(FScanner& sc, FScriptPosition& pos)
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

void parseSetupTile(FScanner& sc, FScriptPosition& pos)
{
	int tile;
	if (!sc.GetNumber(tile, true)) return;
	if (!ValidateTilenum("setuptile", tile, pos)) return;
	auto& tiled = TileFiles.tiledata[tile];
	if (!sc.GetNumber(tiled.hiofs.xsize, true)) return;
	if (!sc.GetNumber(tiled.hiofs.ysize, true)) return;
	if (!sc.GetNumber(tiled.hiofs.xoffs, true)) return;
	if (!sc.GetNumber(tiled.hiofs.yoffs, true)) return;
}

//===========================================================================
//
//
//
//===========================================================================

void parseSetupTileRange(FScanner& sc, FScriptPosition& pos)
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

	for (int i = tilestart; i <= tileend; i++) TileFiles.tiledata[i].hiofs = hiofs;
}

//===========================================================================
//
//
//
//===========================================================================

void parseAnimTileRange(FScanner& sc, FScriptPosition& pos)
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

void parseAlphahack(FScanner& sc, FScriptPosition& pos)
{
	int tile;

	if (!sc.GetNumber(tile, true)) return;
	if (!sc.GetFloat(true)) return;
	if ((unsigned)tile < MAXTILES) TileFiles.tiledata[tile].texture->alphaThreshold = (float)sc.Float;
}

//===========================================================================
//
//
//
//===========================================================================

void parseAlphahackRange(FScanner& sc, FScriptPosition& pos)
{
	int tilestart, tileend;

	if (!sc.GetNumber(tilestart, true)) return;
	if (!sc.GetNumber(tileend, true)) return;
	if (!sc.GetFloat(true)) return;
	if (!ValidateTileRange("alphahackrange", tilestart, tileend, pos)) return;

	for (int i = tilestart; i <= tileend; i++)
		TileFiles.tiledata[i].texture->alphaThreshold = (float)sc.Number;
}

//===========================================================================
//
//
//
//===========================================================================
static int lastvoxid = -1;

void parseDefineVoxel(FScanner& sc, FScriptPosition& pos)
{
	sc.MustGetString();
	while (nextvoxid < MAXVOXELS && voxreserve[nextvoxid]) nextvoxid++;

	if (nextvoxid == MAXVOXELS)
	{
		pos.Message(MSG_ERROR, "Maximum number of voxels (%d) already defined.", MAXVOXELS);
		return;
	}

	if (voxDefine(nextvoxid, sc.String))
	{
		pos.Message(MSG_ERROR, "Unable to load voxel file \"%s\"", sc.String);
		return;
	}

	lastvoxid = nextvoxid++;
}

//===========================================================================
//
//
//
//===========================================================================

void parseDefineVoxelTiles(FScanner& sc, FScriptPosition& pos)
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
	for (int i = tilestart; i <= tileend; i++) tiletovox[i] = lastvoxid;
}

//===========================================================================
//
//
//
//===========================================================================

void parseVoxel(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	int tile0 = MAXTILES, tile1 = -1;
	FString fn;
	bool error = false;

	if (!sc.GetString(fn)) return;

	while (nextvoxid < MAXVOXELS && voxreserve[nextvoxid]) nextvoxid++;

	if (nextvoxid == MAXVOXELS)
	{
		pos.Message(MSG_ERROR, "Maximum number of voxels (%d) already defined.", MAXVOXELS);
		error = true;
	}
	else  if (voxDefine(nextvoxid, fn))
	{
		pos.Message(MSG_ERROR, "Unable to load voxel file \"%s\"", fn.GetChars());
		error = true;
	}

	int lastvoxid = nextvoxid++;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("tile"))
		{
			sc.GetNumber(true);
			if (ValidateTilenum("voxel", sc.Number, pos))
			{
				if (!error) tiletovox[sc.Number] = lastvoxid;
			}
		}
		if (sc.Compare("tile0")) sc.GetNumber(tile0, true);
		if (sc.Compare("tile1"))
		{
			sc.GetNumber(tile1, true);
			if (ValidateTileRange("voxel", tile0, tile1, pos) && !error)
			{
				for (int i = tile0; i <= tile1; i++) tiletovox[i] = lastvoxid;
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

void parseDefineTint(FScanner& sc, FScriptPosition& pos)
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

void parseFogpal(FScanner& sc, FScriptPosition& pos)
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

void parseNoFloorpalRange(FScanner& sc, FScriptPosition& pos)
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

void parseTint(FScanner& sc, FScriptPosition& pos)
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

void parseMusic(FScanner& sc, FScriptPosition& pos)
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
	SetMusicReplacement(id, file);
}

//===========================================================================
//
//
//
//===========================================================================

void parseMapinfo(FScanner& sc, FScriptPosition& pos)
{
	usermaphack_t mhk;
	FScanner::SavedPos blockend;
	TArray<FString> md4s;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("mapfile")) sc.GetString();
		else if (sc.Compare("maptitle")) sc.GetString(mhk.title);
		else if (sc.Compare("mhkfile")) sc.GetString(mhk.mhkfile);
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
			mhk.md4[i] = (uint8_t)strtol(smallbuf, nullptr, 16);
		}
		AddUserMapHack(mhk);
	}
}

//===========================================================================
//
//
//
//===========================================================================

void parseEcho(FScanner& sc, FScriptPosition& pos)
{
	sc.MustGetString();
	Printf("%s\n", sc.String);
}

//===========================================================================
//
//
//
//===========================================================================

void parseMultiPsky(FScanner& sc, FScriptPosition& pos)
{
	// The maximum tile offset ever used in any tiled parallaxed multi-sky.
	enum { PSKYOFF_MAX = 16 };

	FScanner::SavedPos blockend;
	SkyDefinition sky{};

	bool crc;
	sky.scale = 1.f;
	sky.baselineofs = INT_MIN;
	if (sc.CheckString("crc"))
	{
		if (!sc.GetNumber(sky.crc32, true)) return;
		crc = true;
	}
	else
	{
		if (!sc.GetNumber(sky.tilenum, true)) return;
		crc = false;
	}

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("horizfrac")) sc.GetNumber(true); // not used anymore
		else if (sc.Compare("yoffset")) sc.GetNumber(sky.pmoffset, true);
		else if (sc.Compare("baseline")) sc.GetNumber(sky.baselineofs, true);
		else if (sc.Compare("lognumtiles")) sc.GetNumber(sky.lognumtiles, true);
		else if (sc.Compare("yscale")) { int intscale; sc.GetNumber(intscale, true); sky.scale = intscale * (1. / 65536.); }
		else if (sc.Compare({ "tile", "panel" }))
		{
			if (!sc.CheckString("}"))
			{
				int panel = 0, offset = 0;
				sc.GetNumber(panel, true);
				sc.GetNumber(offset, true);
				if ((unsigned)panel < MAXPSKYTILES && (unsigned)offset <= PSKYOFF_MAX) sky.offsets[panel] = offset;
			}
			else
			{
				int panel = 0, offset;
				while (!sc.CheckString("}"))
				{
					sc.GetNumber(offset, true);
					if ((unsigned)panel < MAXPSKYTILES && (unsigned)offset <= PSKYOFF_MAX) sky.offsets[panel] = offset;
					panel++;
				}
			}
		}
	}
	if (sky.baselineofs == INT_MIN) sky.baselineofs = sky.pmoffset;
	if (!crc && sky.tilenum != DEFAULTPSKY && (unsigned)sky.tilenum >= MAXUSERTILES) return;
	if ((1 << sky.lognumtiles) > MAXPSKYTILES) return;
	if (crc) addSkyCRC(sky, sky.crc32);
	else addSky(sky, sky.tilenum);
}

//===========================================================================
//
//
//
//===========================================================================

void parseRffDefineId(FScanner& sc, FScriptPosition& pos)
{
	FString resName;
	FString resType;
	int resID;

	if (!sc.GetString(resName)) return;
	if (!sc.GetString(resType)) return;
	if (!sc.GetNumber(resID)) return;
	if (!sc.GetString()) return;
	resName.AppendFormat(".%s", resType.GetChars());
	fileSystem.CreatePathlessCopy(resName, resID, 0);
}

//===========================================================================
//
// empty stub
//
//===========================================================================

void parseEmptyBlock(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;

	if (sc.StartBraces(&blockend)) return;
	sc.RestorePos(blockend);
	sc.CheckString("}");
}

void parseEmptyBlockWithParm(FScanner& sc, FScriptPosition& pos)
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

void parseTexHitscanRange(FScanner& sc, FScriptPosition& pos)
{
	int start, end;

	if (!sc.GetNumber(start, true)) return;
	if (!sc.GetNumber(end, true)) return;

	if (start < 0) start = 0;
	if (end >= MAXUSERTILES) end = MAXUSERTILES - 1;
	for (int i = start; i <= end; i++)
		TileFiles.tiledata[i].picanm.sf |= PICANM_TEXHITSCAN_BIT;
}

//===========================================================================
//
//
//
//===========================================================================

void parseNoFullbrightRange(FScanner& sc, FScriptPosition& pos)
{
	int start, end;

	if (!sc.GetNumber(start, true)) return;
	if (!sc.GetNumber(end, true)) return;

	if (start < 0) start = 0;
	if (end >= MAXUSERTILES) end = MAXUSERTILES - 1;
	for (int i = start; i <= end; i++)
	{
		auto tex = tileGetTexture(i);
		if (tex->isValid())	tex->SetDisableBrightmap();
	}
}

//===========================================================================
//
//
//
//===========================================================================

void parseArtFile(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	FString file;
	int tile = -1;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend)) 
	{
		sc.MustGetString();
		if (sc.Compare("file")) sc.GetString(file);
		else if (sc.Compare("tile")) sc.GetNumber(tile, true);
	}

	if (file.IsEmpty())
	{
		pos.Message(MSG_ERROR, "artfile: missing file name");
	}
	else if (tile >= 0 && ValidateTilenum("artfile", tile, pos))
		TileFiles.LoadArtFile(file, nullptr, tile);
}

//===========================================================================
//
// this is only left in for compatibility purposes.
// There's way better methods to handle translucency.
//
//===========================================================================

static void parseBlendTableGlBlend(FScanner& sc, int id)
{
	FScanner::SavedPos blockend;
	FScriptPosition pos = sc;
	FString file;

	if (sc.StartBraces(&blockend)) return;

	glblend_t* const glb = glblend + id;
	*glb = nullglblend;

	while (!sc.FoundEndBrace(blockend))
	{
		int which = 0;
		sc.MustGetString();
		if (sc.Compare("forward")) which = 1;
		else if (sc.Compare("reverse")) which = 2;
		else if (sc.Compare("both")) which = 3;
		else continue;

		FScanner::SavedPos blockend2;

		if (sc.StartBraces(&blockend2)) return;
		glblenddef_t bdef{};
		while (!sc.FoundEndBrace(blockend2))
		{
			int whichb = 0;
			sc.MustGetString();
			if (sc.Compare("}")) break;
			if (sc.Compare({ "src", "sfactor", "top" })) whichb = 0;
			else if (sc.Compare({ "dst", "dfactor", "bottom" })) whichb = 1;
			else if (sc.Compare("alpha"))
			{
				sc.GetFloat(true);
				bdef.alpha = (float)sc.Float;
				continue;
			}
			uint8_t* const factor = whichb == 0 ? &bdef.src : &bdef.dst;
			sc.MustGetString();
			if (sc.Compare("ZERO")) *factor = STYLEALPHA_Zero;
			else if (sc.Compare("ONE")) *factor = STYLEALPHA_One;
			else if (sc.Compare("SRC_COLOR")) *factor = STYLEALPHA_SrcCol;
			else if (sc.Compare("ONE_MINUS_SRC_COLOR")) *factor = STYLEALPHA_InvSrcCol;
			else if (sc.Compare("SRC_ALPHA")) *factor = STYLEALPHA_Src;
			else if (sc.Compare("ONE_MINUS_SRC_ALPHA")) *factor = STYLEALPHA_InvSrc;
			else if (sc.Compare("DST_ALPHA")) *factor = STYLEALPHA_Dst;
			else if (sc.Compare("ONE_MINUS_DST_ALPHA")) *factor = STYLEALPHA_InvDst;
			else if (sc.Compare("DST_COLOR")) *factor = STYLEALPHA_DstCol;
			else if (sc.Compare("ONE_MINUS_DST_COLOR")) *factor = STYLEALPHA_InvDstCol;
			else sc.ScriptMessage("Unknown blend operation %s", sc.String);
		}
		if (which & 1) glb->def[0] = bdef;
		if (which & 2) glb->def[1] = bdef;
	}
}

void parseBlendTable(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	int id;

	if (!sc.GetNumber(id, true)) return;

	if (sc.StartBraces(&blockend)) return;

	if ((unsigned)id >= MAXBLENDTABS)
	{
		pos.Message(MSG_ERROR, "blendtable: Invalid blendtable number %d", id);
		sc.RestorePos(blockend);
		return;
	}

	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("raw")) parseEmptyBlock(sc, pos); // Raw translucency map for the software renderer. We have no use for this.
		else if (sc.Compare("glblend")) parseBlendTableGlBlend(sc, id);
		else if (sc.Compare("undef")) glblend[id] = defaultglblend;
		else if (sc.Compare("copy"))
		{
			sc.GetNumber(true);

			if ((unsigned)sc.Number >= MAXBLENDTABS || sc.Number == id)
			{
				pos.Message(MSG_ERROR, "blendtable: Invalid source blendtable number %d in copy", sc.Number);
			}
			else glblend[id] = glblend[sc.Number];
		}
	}
}

//===========================================================================
//
// thw same note as for blendtable applies here.
//
//===========================================================================

void parseNumAlphaTabs(FScanner& sc, FScriptPosition& pos)
{
	int value;
	if (!sc.GetNumber(value)) return;

	for (int a = 1, value2 = value * 2 + (value & 1); a <= value; ++a)
	{
		float finv2value = 1.f / (float)value2;

		glblend_t* const glb = glblend + a;
		*glb = defaultglblend;
		glb->def[0].alpha = (float)(value2 - a) * finv2value;
		glb->def[1].alpha = (float)a * finv2value;
	}
}

//===========================================================================
//
//
//
//===========================================================================

static bool parseBasePaletteRaw(FScanner& sc, FScriptPosition& pos, int id)
{
	FScanner::SavedPos blockend;
	FString fn;
	int32_t offset = 0;
	int32_t shiftleft = 0;

	if (sc.StartBraces(&blockend)) return false;

	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("file")) sc.GetString(fn);
		else if (sc.Compare("offset")) sc.GetNumber(offset, true);
		else if (sc.Compare("shiftleft")) sc.GetNumber(shiftleft, true);
	}

	if (fn.IsEmpty())
	{
		pos.Message(MSG_ERROR, "basepalette: filename missing");
	}
	else if (offset < 0)
	{
		pos.Message(MSG_ERROR, "basepalette: Invalid file offset");
	}
	else if ((unsigned)shiftleft >= 8)
	{
		pos.Message(MSG_ERROR, "basepalette: Invalid left shift %d provided", shiftleft);
	}
	else
	{
		FileReader fil = fileSystem.OpenFileReader(fn);
		if (!fil.isOpen())
		{
			pos.Message(MSG_ERROR, "basepalette: Failed opening \"%s\"", fn.GetChars());
		}
		else
		{
			fil.Seek(offset, FileReader::SeekSet);
			auto palbuf = fil.Read();
			if (palbuf.Size() < 768)
			{
				pos.Message(MSG_ERROR, "basepalette: Read failed");
			}
			else
			{
				if (shiftleft != 0)
				{
					for (auto& pe : palbuf)
						pe <<= shiftleft;
				}

				paletteSetColorTable(id, palbuf.Data(), false, false);
				return true;
			}
		}
	}
	return false;
}

void parseBasePalette(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	int id;
	bool didLoadPal = false;

	if (!sc.GetNumber(id)) return;

	if (sc.StartBraces(&blockend)) return;

	if ((unsigned)id >= MAXBASEPALS)
	{
		pos.Message(MSG_ERROR, "basepalette: Invalid basepal number %d", id);
		sc.RestorePos(blockend);
		sc.CheckString("{");
		return;
	}

	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("raw")) didLoadPal |= parseBasePaletteRaw(sc, pos, id);
		else if (sc.Compare("copy"))
		{
			int source = -1;
			sc.GetNumber(source);

			if ((unsigned)source >= MAXBASEPALS || source == id)
			{
				pos.Message(MSG_ERROR, "basepalette: Invalid source basepal number %d", source);
			}
			else
			{
				auto sourcepal = GPalette.GetTranslation(Translation_BasePalettes, source);
				if (sourcepal == nullptr)
				{
					pos.Message(MSG_ERROR, "basepalette: Source basepal %d does not exist", source);
				}
				else
				{
					GPalette.CopyTranslation(TRANSLATION(Translation_BasePalettes, id), TRANSLATION(Translation_BasePalettes, source));
					didLoadPal = true;
				}
			}
		}
		else if (sc.Compare("undef"))
		{
			GPalette.ClearTranslationSlot(TRANSLATION(Translation_BasePalettes, id));
			didLoadPal = 0;
			if (id == 0) paletteloaded &= ~PALETTE_MAIN;
		}
	}

	if (didLoadPal && id == 0)
	{
		paletteloaded |= PALETTE_MAIN;
	}
}

//===========================================================================
//
//
//
//===========================================================================

void parseUndefBasePaletteRange(FScanner& sc, FScriptPosition& pos)
{
	int start, end;

	if (!sc.GetNumber(start)) return;
	if (!sc.GetNumber(end)) return;

	if (start > end || (unsigned)start >= MAXBASEPALS || (unsigned)end >= MAXBASEPALS)
	{
		pos.Message(MSG_ERROR, "undefbasepaletterange: Invalid range [%d, %d]", start, end);
		return;
	}
	for (int i = start; i <= end; i++) GPalette.ClearTranslationSlot(TRANSLATION(Translation_BasePalettes, i));
	if (start == 0) paletteloaded &= ~PALETTE_MAIN;
}

//===========================================================================
//
// sadly this looks broken by design with its hard coded 32 shades...
//
//===========================================================================

static void parsePalookupRaw(FScanner& sc, int id, int& didLoadShade)
{
	FScanner::SavedPos blockend;
	FScriptPosition pos = sc;

	if (sc.StartBraces(&blockend)) return;

	FString fn;
	int32_t offset = 0;
	int32_t length = 256 * 32; // hardcoding 32 instead of numshades

	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("file")) sc.GetString(fn);
		else if (sc.Compare("offset")) sc.GetNumber(offset);
		else if (sc.Compare("noshades")) length = 256;
	}

	if (fn.IsEmpty())
	{
		pos.Message(MSG_ERROR, "palookup: filename missing");
	}
	else if (offset < 0)
	{
		pos.Message(MSG_ERROR, "palookup: Invalid file offset %d", offset);
	}
	else
	{
		FileReader fil = fileSystem.OpenFileReader(fn);
		if (!fil.isOpen())
		{
			pos.Message(MSG_ERROR, "palookup: Failed opening \"%s\"", fn.GetChars());
		}
		else
		{
			fil.Seek(offset, FileReader::SeekSet);
			auto palookupbuf = fil.Read();
			if (palookupbuf.Size() < 256)
			{
				pos.Message(MSG_ERROR, "palookup: Read failed");
			}
			else if (palookupbuf.Size() >= 256 * 32)
			{
				didLoadShade = 1;
				numshades = 32;
				lookups.setTable(id, palookupbuf.Data());
			}
			else
			{
				if (!(paletteloaded & PALETTE_SHADE))
				{
					pos.Message(MSG_ERROR, "palookup: Shade tables not loaded");
				}
				else
					lookups.makeTable(id, palookupbuf.Data(), 0, 0, 0, lookups.tables[id].noFloorPal);
			}
		}
	}
}

static void parsePalookupFogpal(FScanner& sc, int id)
{
	FScanner::SavedPos blockend;
	FScriptPosition pos = sc;

	if (sc.StartBraces(&blockend)) return;

	int red = 0, green = 0, blue = 0;

	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare({ "r", "red" })) sc.GetNumber(red);
		else if (sc.Compare({ "g", "green" })) sc.GetNumber(green);
		else if (sc.Compare({ "b", "blue" })) sc.GetNumber(blue);
	}
	red = clamp(red, 0, 255);
	green = clamp(green, 0, 255);
	blue = clamp(blue, 0, 255);

	if (!(paletteloaded & PALETTE_SHADE))
	{
		pos.Message(MSG_ERROR, "palookup: Shade tables not loaded");
	}
	else
		lookups.makeTable(id, nullptr, red, green, blue, 1);
}

static void parsePalookupMakePalookup(FScanner& sc, FScriptPosition& pos, int id, int& didLoadShade)
{
	FScanner::SavedPos blockend;

	if (sc.StartBraces(&blockend)) return;

	int red = 0, green = 0, blue = 0;
	int remappal = -1;

	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare({ "r", "red" })) sc.GetNumber(red);
		else if (sc.Compare({ "g", "green" })) sc.GetNumber(green);
		else if (sc.Compare({ "b", "blue" })) sc.GetNumber(blue);
		else if (sc.Compare("remappal")) sc.GetNumber(remappal, true);
		else if (sc.Compare("remapself")) remappal = id;
	}
	red = clamp(red, 0, 255);
	green = clamp(green, 0, 255);
	blue = clamp(blue, 0, 255);

	if ((unsigned)remappal >= MAXPALOOKUPS)
	{
		pos.Message(MSG_ERROR, "palookup: Invalid remappal %d", remappal);
	}
	else if (!(paletteloaded & PALETTE_SHADE))
	{
		pos.Message(MSG_ERROR, "palookup: Shade tables not loaded");
	}
	else
		lookups.makeTable(id, nullptr, red, green, blue, lookups.tables[id].noFloorPal);
}

void parsePalookup(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	int id;
	int didLoadShade = 0;

	if (!sc.GetNumber(id, true)) return;
	if (sc.StartBraces(&blockend)) return;

	if ((unsigned)id >= MAXPALOOKUPS)
	{
		pos.Message(MSG_ERROR, "palookup: Invalid palette number %d", id);
		sc.RestorePos(blockend);
		return;
	}

	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("raw")) parsePalookupRaw(sc, id, didLoadShade);
		else if (sc.Compare("fogpal")) parsePalookupFogpal(sc, id);
		else if (sc.Compare("makepalookup")) parsePalookupMakePalookup(sc, pos, id, didLoadShade);
		else if (sc.Compare("floorpal")) lookups.tables[id].noFloorPal = 0;
		else if (sc.Compare("nofloorpal")) lookups.tables[id].noFloorPal = 1;
		else if (sc.Compare("copy"))
		{
			int source = 0;
			sc.GetNumber(source, true);

			if ((unsigned)source >= MAXPALOOKUPS || source == id)
			{
				pos.Message(MSG_ERROR, "palookup: Invalid source pal number %d", source);
			}
			else if (source == 0 && !(paletteloaded & PALETTE_SHADE))
			{
				pos.Message(MSG_ERROR, "palookup: Shade tables not loaded");
			}
			else
			{
				// do not overwrite the base with an empty table.
				if (lookups.checkTable(source) || id > 0) lookups.copyTable(id, source);
				didLoadShade = 1;
			}
		}
		else if (sc.Compare("undef")) 
		{
			lookups.clearTable(id);
			didLoadShade = 0;
			if (id == 0) paletteloaded &= ~PALETTE_SHADE;
		}

	}
	if (didLoadShade && id == 0)
	{
		paletteloaded |= PALETTE_SHADE;
	}
}

//===========================================================================
//
// 
//
//===========================================================================

void parseMakePalookup(FScanner& sc, FScriptPosition& pos)
{
	FScanner::SavedPos blockend;
	int red = 0, green = 0, blue = 0, pal = -1;
	int remappal = 0;
	int nofloorpal = -1;
	bool havepal = false, haveremappal = false, haveremapself = false;

	if (sc.StartBraces(&blockend)) return;

	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare({ "r", "red" })) sc.GetNumber(red);
		else if (sc.Compare({ "g", "green" })) sc.GetNumber(green);
		else if (sc.Compare({ "b", "blue" })) sc.GetNumber(blue);
		else if (sc.Compare("remappal"))
		{
			sc.GetNumber(remappal, true);
			haveremappal = true;
		}
		else if (sc.Compare("remapself"))
		{
			haveremapself = true;
		}
		else if (sc.Compare("nofloorpal")) sc.GetNumber(nofloorpal, true);
		else if (sc.Compare("pal"))
		{
			havepal = true;
			sc.GetNumber(pal, true);
		}
	}
	red = clamp(red, 0, 63);
	green = clamp(green, 0, 63);
	blue = clamp(blue, 0, 63);

	if (!havepal)
	{
		pos.Message(MSG_ERROR, "makepalookup: missing palette number");
	} 
	else if (pal == 0 || (unsigned)pal >= MAXREALPAL)
	{
		pos.Message(MSG_ERROR, "makepalookup: palette number %d out of range (1 .. %d)", pal, MAXREALPAL - 1);
	}
	else if (haveremappal && haveremapself)
	{
		// will also disallow multiple remappals or remapselfs
		pos.Message(MSG_ERROR, "makepalookup: must have either 'remappal' or 'remapself' but not both");
	}
	else if ((haveremappal && (unsigned)remappal >= MAXREALPAL))
	{
		pos.Message(MSG_ERROR, "makepalookup: remap palette number %d out of range (0 .. %d)", pal, MAXREALPAL - 1);
	}
	else
	{
		if (haveremapself) remappal = pal;
		lookups.makeTable(pal, lookups.getTable(remappal), red << 2, green << 2, blue << 2,
			remappal == 0 ? 1 : (nofloorpal == -1 ? lookups.tables[remappal].noFloorPal : nofloorpal));
	}
}

//===========================================================================
//
// 
//
//===========================================================================

void parseUndefPalookupRange(FScanner& sc, FScriptPosition& pos)
{
	int id0, id1;

	if (!sc.GetNumber(id0, true)) return;
	if (!sc.GetNumber(id1, true)) return;

	if (id0 > id1 || (unsigned)id0 >= MAXPALOOKUPS || (unsigned)id1 >= MAXPALOOKUPS)
	{
		pos.Message(MSG_ERROR, "undefpalookuprange: Invalid range");
	}
	else
	{
		for (int i = id0; i <= id1; i++) lookups.clearTable(i);
		if (id0 == 0) paletteloaded &= ~PALETTE_SHADE;
	}
}

//===========================================================================
//
// 
//
//===========================================================================

void parseHighpalookup(FScanner& sc, FScriptPosition& pos)
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
	else if (!fileSystem.FileExists(fn))
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

void parseDefineModel(FScanner& sc, FScriptPosition& pos)
{
	FString modelfn;
	double scale;
	int shadeoffs;

	if (!sc.GetString(modelfn)) return;
	if (!sc.GetFloat(scale, true)) return;
	if (!sc.GetNumber(shadeoffs, true)) return;

	mdglobal.lastmodelid = md_loadmodel(modelfn);
	if (mdglobal.lastmodelid < 0)
	{
		pos.Message(MSG_WARNING, "definemodel: unable to load model file '%s'", modelfn.GetChars());
	}
	else
	{
		md_setmisc(mdglobal.lastmodelid, (float)scale, shadeoffs, 0.0, 0.0, 0);
		mdglobal.modelskin = mdglobal.lastmodelskin = 0;
		mdglobal.seenframe = 0;
	}
}

//===========================================================================
//
// 
//
//===========================================================================

void parseDefineModelFrame(FScanner& sc, FScriptPosition& pos)
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
		int err = (md_defineframe(mdglobal.lastmodelid, framename, i, max(0, mdglobal.modelskin), 0.0f, 0));
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

void parseDefineModelAnim(FScanner& sc, FScriptPosition& pos)
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
	int err = (md_defineanimation(mdglobal.lastmodelid, startframe, endframe, (int32_t)(dfps * (65536.0 * .001)), flags));
	if (err == -2) pos.Message(MSG_ERROR, "Invalid start frame name %s", startframe.GetChars());
	else if (err == -3) pos.Message(MSG_ERROR, "Invalid end frame name %s", endframe.GetChars());
}

//===========================================================================
//
// 
//
//===========================================================================

void parseDefineModelSkin(FScanner& sc, FScriptPosition& pos)
{
	int palnum;
	FString skinfn;

	if (!sc.GetNumber(palnum, true)) return;
	if (!sc.GetString(skinfn)) return;

	if (mdglobal.seenframe) { mdglobal.modelskin = ++mdglobal.lastmodelskin; }
	mdglobal.seenframe = 0;

	if (!fileSystem.FileExists(skinfn)) return;

	int err = (md_defineskin(mdglobal.lastmodelid, skinfn, palnum, max(0, mdglobal.modelskin), 0, 0.0f, 1.0f, 1.0f, 0));
	if (err == -2) pos.Message(MSG_ERROR, "Invalid skin file name %s", skinfn.GetChars());
	else if (err == -3) pos.Message(MSG_ERROR, "Invalid palette %d", palnum);
}

//===========================================================================
//
// 
//
//===========================================================================

void parseSelectModelSkin(FScanner& sc, FScriptPosition& pos)
{
	sc.GetNumber(mdglobal.modelskin, true);
}


//===========================================================================
//
// 
//
//===========================================================================

void parseUndefModel(FScanner& sc, FScriptPosition& pos)
{
	int tile;
	if (!sc.GetNumber(tile, true)) return;
	if (!ValidateTilenum("undefmodel", tile, pos)) return;
	md_undefinetile(tile);
}

void parseUndefModelRange(FScanner& sc, FScriptPosition& pos)
{
	int start, end;

	if (!sc.GetNumber(start, true)) return;
	if (!sc.GetNumber(end, true)) return;
	if (!ValidateTileRange("undefmodel", start, end, pos)) return;
	for (int i = start; i <= end; i++) md_undefinetile(i);
}

void parseUndefModelOf(FScanner& sc, FScriptPosition& pos)
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
		int res = md_defineframe(mdglobal.lastmodelid, framename, i, max(0, mdglobal.modelskin), smoothduration, pal);
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

	int res = md_defineanimation(mdglobal.lastmodelid, startframe, endframe, (int)(fps * (65536.0 * .001)), flags);
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

	if (!fileSystem.FileExists(filename))
	{
		pos.Message(MSG_ERROR, "%s: file not found", filename.GetChars());
		return false;
	}

	if (pal == DETAILPAL) param = 1.f / param;
	int res = md_defineskin(mdglobal.lastmodelid, filename, pal, max(0, mdglobal.modelskin), surface, param, specpower, specfactor, flags);
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
		int res = md_definehud(mdglobal.lastmodelid, i, addf, angadd, flags, fov);
		if (res < 0)
		{
			if (res == -2) pos.Message(MSG_ERROR, "Invalid tile number %d", i);
			return false;
		}
	}
	return true;
}

void parseModel(FScanner& sc, FScriptPosition& pos)
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

	mdglobal.lastmodelid = md_loadmodel(modelfn);
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
			md_undefinemodel(mdglobal.lastmodelid);
			nextmodelid--;
		}
	}
	else
	{
		md_setmisc(mdglobal.lastmodelid, (float)scale, shadeoffs, (float)mzadd, (float)myoffset, flags);
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
				output.Push(temparray[0].CompareNoCase("first") == 0 ? 0 : atoi(temparray[0]));
				output.Push(temparray[1].CompareNoCase("last") == 0 ? maxvalue : atoi(temparray[1]));
			}
		}
		else
		{
			// We just have a number. Convert the string into an int and push it twice to the output array.
			auto tempvalue = atoi(input);
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
			auto lump = fileSystem.FindFile(fn);
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
	fileSystem.CreatePathlessCopy(fn, res_id, 0);
}

static void parseSpawnClasses(FScanner& sc, FScriptPosition& pos)
{
	FString fn;
	int res_id = -1;
	int numframes = -1;
	bool interpolate = false;

	sc.SetCMode(true);
	if (!sc.CheckString("{"))
	{
		pos.Message(MSG_ERROR, "spawnclasses:'{' expected, unable to continue");
		sc.SetCMode(false);
		return;
	}
	while (!sc.CheckString("}"))
	{
		int num = -1;
		int base = -1;
		FName cname;
		sc.GetNumber(num, true);
		sc.MustGetStringName("=");
		sc.MustGetString();
		cname = sc.String;
		if (sc.CheckString(","))
		{
			sc.GetNumber(base, true);
		}

		// todo: check for proper base class
		spawnMap.Insert(num, { cname, nullptr, base });
	}
	sc.SetCMode(false);
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
	{ "makepalookup",    parseMakePalookup     },
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
	{ "artfile",         parseArtFile          },
	{ "mapinfo",         parseMapinfo          },
	{ "echo",            parseEcho             },
	{ "globalflags",     parseSkip<1>      },
	{ "copytile",        parseCopyTile         },
	{ "globalgameflags", parseSkip<1>  },
	{ "multipsky",       parseMultiPsky        },
	{ "basepalette",     parseBasePalette      },
	{ "palookup",        parsePalookup         },
	{ "blendtable",      parseBlendTable       },
	{ "numalphatables",  parseNumAlphaTabs     },
	{ "undefbasepaletterange", parseUndefBasePaletteRange },
	{ "undefpalookuprange", parseUndefPalookupRange },
	{ "undefblendtablerange", parseSkip<2> },
	{ "shadefactor",     parseSkip<1>      },
	{ "newgamechoices",  parseEmptyBlock   },
	{ "rffdefineid",     parseRffDefineId      },
	{ "defineqav",       parseDefineQAV        },

	{ "spawnclasses",		parseSpawnClasses },
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

void loaddefinitionsfile(const char* fn, bool cumulative, bool maingame)
{
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
			Printf(PRINT_NONOTIFY, "Loading \"%s\"\n", fileSystem.GetFileFullPath(lump).GetChars());
			deftimer.Clock();
			parseit(lump);
			printtimer(fn);
		}
	}
}
