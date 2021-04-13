
/*
** parsefuncs.h
** handlers for .def parser
** only to be included by the actual parser
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

int tileSetHightileReplacement(int picnum, int palnum, const char* filename, float alphacut, float xscale, float yscale, float specpower, float specfactor);
int tileSetSkybox(int picnum, int palnum, FString* facenames);
void tileRemoveReplacement(int num);
void AddUserMapHack(usermaphack_t&);

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

static void parseTexturePaletteBlock(FScanner& sc, FScriptPosition& pos, int tile)
{
	FScanner::SavedPos blockend;

	int pal = -1, xsiz = 0, ysiz = 0;
	FString fn;
	double alphacut = -1.0, xscale = 1.0, yscale = 1.0, specpower = 1.0, specfactor = 1.0;

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
	};

	if ((unsigned)tile < MAXUSERTILES)
	{
		if ((unsigned)pal >= MAXPALOOKUPS - RESERVEDPALS)
		{
			pos.Message(MSG_ERROR, "missing or invalid 'palette number' for texture definition");
		}
		else if (fn.IsEmpty())
		{
			pos.Message(MSG_ERROR, "missing 'file name' for texture definition");
		}
		else if (!fileSystem.FileExists(fn))
		{
			pos.Message(MSG_ERROR, "%s not found in replacement for tile %d", fn.GetChars(), tile);
		}
		else
		{
			if (xsiz > 0 && ysiz > 0)
			{
				tileSetDummy(tile, xsiz, ysiz);
			}
			xscale = 1.0f / xscale;
			yscale = 1.0f / yscale;

			tileSetHightileReplacement(tile, pal, fn, alphacut, xscale, yscale, specpower, specfactor);
		}
	}
}

static void parseTextureSpecialBlock(FScanner& sc, FScriptPosition& pos, int tile, int specialpal)
{
	FScanner::SavedPos blockend;

	int pal = -1;
	FString fn;
	double xscale = 1.0, yscale = 1.0, specpower = 1.0, specfactor = 1.0;

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
		if (fn.IsEmpty())
		{
			pos.Message(MSG_ERROR, "missing 'file name' for texture definition");
		}
		else if (!fileSystem.FileExists(fn))
		{
			pos.Message(MSG_ERROR, "%s not found in replacement for tile %d", fn.GetChars(), tile);
		}
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
		if (sc.Compare("pal")) parseTexturePaletteBlock(sc, pos, tile);
		else if (sc.Compare("detail")) parseTextureSpecialBlock(sc, pos, tile, DETAILPAL);
		else if (sc.Compare("glow")) parseTextureSpecialBlock(sc, pos, tile, GLOWPAL);
		else if (sc.Compare("specular")) parseTextureSpecialBlock(sc, pos, tile, SPECULARPAL);
		else if (sc.Compare("normal")) parseTextureSpecialBlock(sc, pos, tile, NORMALPAL);
	}
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
	int flags = 0, tsiz = 0, temppal = -1, tempsource = -1;

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

				if ((unsigned)temppal >= MAXPALOOKUPS - RESERVEDPALS)
				{
					pos.Message(MSG_ERROR, "copytile 'palette number' out of range (max=%d)\n", MAXPALOOKUPS - RESERVEDPALS - 1);
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
		// skip over everything else.
	}
	if (tile < 0)
	{
		pos.Message(MSG_ERROR, "skybox: missing tile number");
		return;
	}
	tileSetSkybox(tile, pal, faces);
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
		pos.Message(MSG_ERROR, "Maximum number of voxels (%d) already defined.\n", MAXVOXELS);
		return;
	}

	if (voxDefine(nextvoxid, sc.String))
	{
		pos.Message(MSG_ERROR, "Unable to load voxel file \"%s\"\n", sc.String);
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

	if (!sc.GetString(fn)) return;

	while (nextvoxid < MAXVOXELS && voxreserve[nextvoxid]) nextvoxid++;

	if (nextvoxid == MAXVOXELS)
	{
		pos.Message(MSG_ERROR, "Maximum number of voxels (%d) already defined.\n", MAXVOXELS);
		return;
	}

	if (voxDefine(nextvoxid, fn))
	{
		pos.Message(MSG_ERROR, "Unable to load voxel file \"%s\"\n", fn.GetChars());
		return;
	}

	int lastvoxid = nextvoxid++;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("tile"))
		{
			sc.GetNumber(true);
			if (ValidateTilenum("voxel", sc.Number, pos)) tiletovox[sc.Number] = lastvoxid;
		}
		if (sc.Compare("tile0")) sc.GetNumber(tile0, true);
		if (sc.Compare("tile1"))
		{
			sc.GetNumber(tile1, true);
			if (ValidateTileRange("voxel", tile0, tile1, pos))
			{
				for (int i = tile0; i <= tile1; i++) tiletovox[i] = lastvoxid;
			}
		}
		if (sc.Compare("scale"))
		{
			sc.GetFloat(true);
			voxscale[lastvoxid] = (float)sc.Float;
		}
		if (sc.Compare("rotate")) voxrotate.Set(lastvoxid);
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
		pos.Message(MSG_ERROR, "tint: missing palette number");
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
	SetMusicForMap(id, file, true);
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
			for (int i = 0; i < 16; i++)
			{
				char smallbuf[3] = { sc.String[2 * i], sc.String[2 * i + 1], 0 };
				mhk.md4[i] = strtol(smallbuf, nullptr, 16);
			}
		}
	}
	AddUserMapHack(mhk);
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
	usermaphack_t mhk;
	FScanner::SavedPos blockend;
	psky_t sky{};

	sky.yscale = 65536;
	if (!sc.GetNumber(sky.tilenum, true)) return;

	if (sc.StartBraces(&blockend)) return;
	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("horizfrac")) sc.GetNumber(sky.horizfrac, true);
		else if (sc.Compare("yoffset")) sc.GetNumber(sky.yoffs, true);
		else if (sc.Compare("lognumtiles")) sc.GetNumber(sky.lognumtiles, true);
		else if (sc.Compare("yscale")) sc.GetNumber(sky.yscale, true);
		else if (sc.Compare({ "tile", "panel" }))
		{
			int panel, offset;
			sc.GetNumber(panel, true);
			sc.GetNumber(offset, true);
			if ((unsigned)panel < MAXPSKYTILES && (unsigned)offset <= PSKYOFF_MAX) sky.tileofs[panel] = offset;
		}
	}

	if (sky.tilenum != DEFAULTPSKY && (unsigned)sky.tilenum >= MAXUSERTILES) return;
	if ((1 << sky.lognumtiles) > MAXPSKYTILES) return;
	auto psky = tileSetupSky(sky.tilenum);
	*psky = sky;
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
		pos.Message(MSG_ERROR, "missing 'file name' for artfile definition");
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

static void parseBlendTableGlBlend(FScanner& sc, FScriptPosition& pos, int id)
{
	FScanner::SavedPos blockend;
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
		else if (sc.Compare("glblend")) parseBlendTableGlBlend(sc, pos, id);
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
// sadly this looks broken by design with its hard coded 32 shades...
//
//===========================================================================

static void parsePalookupRaw(FScanner& sc, FScriptPosition& pos, int id, int& didLoadShade)
{
	FScanner::SavedPos blockend;

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
		pos.Message(MSG_ERROR, "palookup: No filename provided");
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
		else if (fil.Seek(offset, FileReader::SeekSet) < 0)
		{
			pos.Message(MSG_ERROR, "palookup: Seek failed");
		}
		else
		{
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

static void parsePalookupFogpal(FScanner& sc, FScriptPosition& pos, int id)
{
	FScanner::SavedPos blockend;

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
		pos.Message(MSG_ERROR, "palookup: Invalid pal number %d", id);
		sc.RestorePos(blockend);
		return;
	}

	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare("raw")) parsePalookupRaw(sc, pos, id, didLoadShade);
		else if (sc.Compare("fogpal")) parsePalookupFogpal(sc, pos, id);
		else if (sc.Compare("makepalookup")) parsePalookupMakePalookup(sc, pos, id, didLoadShade);
		else if (sc.Compare("floorpal")) lookups.tables[id].noFloorPal = 0;
		else if (sc.Compare("nofloorpal")) lookups.tables[id].noFloorPal = 1;
		else if (sc.Compare("copy"))
		{
			int source;
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
	int havepal = 0, remappal = 0;
	int nofloorpal = -1;

	if (sc.StartBraces(&blockend)) return;

	enum {
		HAVE_PAL = 1,
		HAVE_REMAPPAL = 2,
		HAVE_REMAPSELF = 4,

		HAVEPAL_SPECIAL = HAVE_REMAPPAL | HAVE_REMAPSELF,
		HAVEPAL_ERROR = 8,
	};

	while (!sc.FoundEndBrace(blockend))
	{
		sc.MustGetString();
		if (sc.Compare({ "r", "red" })) sc.GetNumber(red);
		else if (sc.Compare({ "g", "green" })) sc.GetNumber(green);
		else if (sc.Compare({ "b", "blue" })) sc.GetNumber(blue);
		else if (sc.Compare("remappal"))
		{
			sc.GetNumber(remappal, true);
			if (havepal & HAVEPAL_SPECIAL) havepal |= HAVEPAL_ERROR;
			havepal |= HAVE_REMAPPAL;
		}
		else if (sc.Compare("remapself"))
		{
			if (havepal & HAVEPAL_SPECIAL) havepal |= HAVEPAL_ERROR;
			havepal |= HAVE_REMAPSELF;
		}
		else if (sc.Compare("nofloorpal")) sc.GetNumber(nofloorpal, true);
		else if (sc.Compare("pal")) sc.GetNumber(pal, true);
	}
	red = clamp(red, 0, 63);
	green = clamp(green, 0, 63);
	blue = clamp(blue, 0, 63);

	if ((havepal & HAVE_PAL) == 0)
	{
		pos.Message(MSG_ERROR, "makepalookup: missing 'palette number'");
	}
	else if (pal == 0 || (unsigned)pal >= MAXPALOOKUPS - RESERVEDPALS)
	{
		pos.Message(MSG_ERROR, "makepalookup: 'palette number' %d out of range (1 .. %d)\n", pal, MAXPALOOKUPS - RESERVEDPALS - 1);
	}
	else if (havepal & HAVEPAL_ERROR)
	{
		// will also disallow multiple remappals or remapselfs
		pos.Message(MSG_ERROR, "makepalookup: must have exactly one of either 'remappal' or 'remapself'\n");
	}
	else if ((havepal & HAVE_REMAPPAL && (unsigned)remappal >= MAXPALOOKUPS - RESERVEDPALS))
	{
		pos.Message(MSG_ERROR, "makepalookup: 'remap palette number' %d out of range (max=%d)\n", pal, MAXPALOOKUPS - RESERVEDPALS - 1);
	}
	else
	{
		if (havepal & HAVE_REMAPSELF) remappal = pal;
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
