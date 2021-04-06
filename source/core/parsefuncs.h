
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

void parseAnimTileRange(FScanner& sc, FScriptPosition& pos)
{
	SetAnim set;
	if (!sc.GetNumber(set.tile1, true)) return;
	if (!sc.GetNumber(set.tile2, true)) return;
	if (!sc.GetNumber(set.speed, true)) return;
	if (!sc.GetNumber(set.type, true)) return;
	processSetAnim("animtilerange", pos, set);
}

