
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

//===========================================================================
//
//
//
//===========================================================================

template<int cnt>
void parseSkip(FScanner& sc, FScriptPosition& pos)
{
	for (int i = 0; i < cnt; i++) if (!sc.GetNumber(true)) return;
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
    int32_t tile = -1, pal = 0;

    if (sc.StartBraces(&blockend)) return;
    while (!sc.FoundEndBrace(blockend))
    {
        sc.GetString();
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
