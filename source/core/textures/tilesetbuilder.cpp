/*
** tilesetbuilder.cpp
** Constructs the full tile set and adds it to the texture manager.
**
**---------------------------------------------------------------------------
** Copyright 2019-2022 Christoph Oelckers
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

#include "image.h"
#include "texturemanager.h"
#include "m_crc32.h"
#include "c_dispatch.h"
#include "tiletexture.h"
#include "tilesetbuilder.h"
#include "gamecontrol.h"
#include "startupinfo.h"
#include "printf.h"
#include "m_argv.h"
#include "gamestruct.h"

const char* G_DefFile(void);
void loaddefinitionsfile(TilesetBuildInfo& info, const char* fn, bool cumulative = false, bool maingrp = false);


//==========================================================================
//
// Returns checksum for a given tile or texture
//
//==========================================================================

int32_t tileGetCRC32(FImageSource* image)
{
	if (image == nullptr) return 0;
	auto pixels = image->GetPalettedPixels(0);
	if (pixels.Size() == 0) return 0;

	// To get proper CRCs as the calling code expects we need to put the translucent index back to 255.
	for (auto& p : pixels)
	{
		if (p == 0) p = 255;
		else if (p == 255) p = 0;
	}

	return crc32(0, (const Bytef*)pixels.Data(), pixels.Size());
}

CCMD(tilecrc)
{
	if (argv.argc() > 1)
	{
		char* p;
		int tile = strtol(argv[1], &p, 10);
		FGameTexture* tex;
		if (tile >= 0 && tile < MAXTILES && !*p)
		{
			tex = TexMan.GetGameTexture(tileGetTextureID(tile));
		}
		else
		{
			tex = TexMan.FindGameTexture(argv[1], ETextureType::Any);
		}
		auto img = tex? tex->GetTexture()->GetImage() : nullptr;
		if (!img) Printf("%s: not a valid texture", argv[1]);
		else Printf("%d\n", tileGetCRC32(img));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void LoadDefinitions(TilesetBuildInfo& info)
{
	for (unsigned i = 0; i < info.tile.Size(); i++)
	{
		if (info.tile[i].extinfo.tiletovox > info.nextvoxid) info.nextvoxid = info.tile[i].extinfo.tiletovox;
	}
	info.nextvoxid++;
	const char* defsfile = G_DefFile();
	FString razedefsfile = defsfile;
	razedefsfile.Substitute(".def", "-raze.def");

	loaddefinitionsfile(info, "engine/engine.def", true, true);	// Internal stuff that is required.

	// check what we have.
	// user .defs override the default ones and are not cumulative.
	// if we fine even one Raze-specific file, all of those will be loaded cumulatively.
	// otherwise the default rules inherited from older ports apply.
	if (userConfig.UserDef.IsNotEmpty())
	{
		loaddefinitionsfile(info, userConfig.UserDef, false);
	}
	else
	{
		if (fileSystem.FileExists(razedefsfile))
		{
			loaddefinitionsfile(info, razedefsfile, true);
		}
		else if (fileSystem.FileExists(defsfile))
		{
			loaddefinitionsfile(info, defsfile, false);
		}
	}

	if (userConfig.AddDefs)
	{
		for (auto& m : *userConfig.AddDefs)
		{
			loaddefinitionsfile(info, m, false);
		}
		userConfig.AddDefs.reset();
	}

	if (GameStartupInfo.def.IsNotEmpty())
	{
		loaddefinitionsfile(info, GameStartupInfo.def);	// Stuff from gameinfo.
	}

	// load the widescreen replacements last. This ensures that mods still get the correct CRCs for their own tile replacements.
	if (fileSystem.FindFile("engine/widescreen.def") >= 0 && !Args->CheckParm("-nowidescreen"))
	{
		loaddefinitionsfile(info, "engine/widescreen.def");
	}
	fileSystem.InitHashChains(); // make sure that any resources that got added can be found again.
}

//==========================================================================
//
// 
//
//==========================================================================

picanm_t tileConvertAnimFormat(int32_t const picanimraw)
{
	// Unpack a 4 byte packed anim descriptor into something more accessible
	picanm_t anm;
	anm.num = picanimraw & 63;
	anm.sf = ((picanimraw >> 24) & 15) | (picanimraw & 192);
	anm.extra = (picanimraw >> 28) & 15;
	return anm;
}

//==========================================================================
//
// 
//
//==========================================================================
FImageSource* createWritableTile(int width, int height);
FImageSource* makeTileWritable(FImageSource* img);

void TilesetBuildInfo::MakeWritable(int tileno)
{
	if (tile[tileno].tileimage != nullptr)
	{
		auto newtex = makeTileWritable(tile[tileno].tileimage);
		tile[tileno].tileimage = newtex;
		tile[tileno].imported = nullptr;
	}
}

void TilesetBuildInfo::CreateWritable(int tileno, int w, int h)
{
	auto newtex = createWritableTile(w, h);
	tile[tileno].tileimage = newtex;
	tile[tileno].imported = nullptr;
}

//===========================================================================
//
// MakeCanvas
//
// Turns texture into a canvas (i.e. camera texture)
//
//===========================================================================

void TilesetBuildInfo::MakeCanvas(int tilenum, int width, int height)
{
	auto ftex = new FCanvasTexture(width * 4, height * 4);
	ftex->aspectRatio = (float)width / height;
	auto canvas = MakeGameTexture(ftex, FStringf("#%05d", tilenum), ETextureType::Any);
	canvas->SetSize(width * 4, height * 4);
	canvas->SetDisplaySize((float)width, (float)height);
	canvas->GetTexture()->SetSize(width * 4, height * 4);
	tile[tilenum].imported = canvas;
	tile[tilenum].tileimage = nullptr;
}

static void GenerateRotations(int firsttileid, const char* basename, int tile, int numframes, int numrotations, int order)
{
	if (order == 0)
	{
		for (int frame = 0; frame < numframes; frame++)
		{
			for (int rotation = 0; rotation < numrotations; rotation++)
			{
				FStringf str("%s@%c%x", basename, frame + 'A', rotation + 1);
				TexMan.AddAlias(str, FSetTextureID(firsttileid + tile));
				tile++;
			}
		}
	}
	else if (order >= 1)
	{
		for (int rotation = 0; rotation < numrotations; rotation++)
		{
			for (int frame = 0; frame < numframes; frame++)
			{
				FStringf str("%s@%c%x", basename, frame + 'A', rotation + 1);
				TexMan.AddAlias(str, FSetTextureID(firsttileid + tile));
				tile += order;
			}
		}
	}
}

static void CompleteRotations(int firsttileid, const char* basename, const char* getname, int numframes, int numrotations)
{
	for (int rotation = numrotations; ; rotation++)
	{
		for (int frame = 0; frame < numframes; frame++)
		{
			FStringf str("%s@%c%x", getname, frame + 'A', rotation + 1);
			auto texid = TexMan.CheckForTexture(str, ETextureType::Any);
			if (frame == 0 && !texid.isValid())
			{
				// rotation does not exist for the first frame -> we reached the end.
				return;
			}
			str.Format("%s@%c%x", basename, frame + 'A', rotation + 1);
			TexMan.AddAlias(str, texid);
		}
	}
}

static void SubstituteRotations(int firsttileid, const char* basename, int numframes, int destrot, int srcrot)
{
	for (int frame = 0; frame < numframes; frame++)
	{
		FStringf str("%s@%c%x", basename, frame + 'A', srcrot);
		auto texid = TexMan.CheckForTexture(str, ETextureType::Any);
		if (!texid.isValid())
		{
			continue;
		}
		str.Format("%s@%c%x", basename, frame + 'A', destrot);
		TexMan.AddAlias(str, texid);
	}
}

void LoadAliases(int firsttileid, int maxarttile)
{
	int lump, lastlump = 0;
	while ((lump = fileSystem.FindLump("TEXNAMES", &lastlump, false)) != -1)
	{
		FScanner sc;
		sc.OpenLumpNum(lump);
		sc.SetCMode(true);
		while (sc.GetNumber())
		{
			int tile = sc.Number;
			if (tile < 0 || tile > maxarttile) tile = maxarttile;
			sc.MustGetStringName("=");
			sc.MustGetString();
			TexMan.AddAlias(sc.String, FSetTextureID(firsttileid + tile));
			FString basename = sc.String;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				int numframes = sc.Number;
				int numrotations = 1, order = 0;
				if (sc.CheckString(","))
				{
					sc.MustGetNumber();
					numrotations = sc.Number;
					if (sc.CheckString(","))
					{
						sc.MustGetNumber();
						order = sc.Number;
					}
				}
				if (numframes <= 0 || numframes > 26)
				{
					sc.ScriptMessage("%d: Bad number of frames\n", numframes);
					continue;
				}
				if (numrotations >= 16 || numrotations < 1)
				{
					sc.ScriptMessage("%d: Bad number of rotations\n", numrotations);
					continue;
				}
				if (order < 0)
				{
					sc.ScriptMessage("%d: Bad order\n", order);
					continue;
				}
				GenerateRotations(firsttileid, basename, tile, numframes, numrotations, order);
				if (sc.CheckString(","))
				{
					sc.MustGetString();
					if (sc.String[0] != '@')
					{
						CompleteRotations(firsttileid, basename, sc.String, numframes, numrotations);
					}
					else
					{
						sc.UnGet();
						do
						{
							sc.MustGetString();
							int destrot = (int)strtoll(sc.String + 1, nullptr, 10);
							sc.MustGetStringName("=");
							sc.MustGetString();
							int srcrot = (int)strtoll(sc.String + 1, nullptr, 10);
							SubstituteRotations(firsttileid, basename, numframes, destrot, srcrot);
						} while (sc.CheckString(","));
					}
				}

			}
		}
	}

}
//==========================================================================
//
// 
//
//==========================================================================

void ConstructTileset()
{
	TilesetBuildInfo info {};
	TArray<FImageSource*> images;
	TArray<unsigned> rawpicanm;
	GetArtImages(images, rawpicanm);
	info.tile.Resize(MAXTILES);
	memset(info.tile.Data(), 0, info.tile.Size() * sizeof(info.tile[0]));
	// fill up the arrays to the maximum allowed but remember the highest original number.
	for (unsigned i = 0; i < images.Size(); i++)
	{
		info.tile[i].orgimage = info.tile[i].tileimage = images[i];
		if (images[i])
		{
			auto s = images[i]->GetOffsets();
			info.tile[i].leftOffset = s.first;
			info.tile[i].topOffset = s.second;
		}
		info.tile[i].extinfo.picanm = tileConvertAnimFormat(rawpicanm[i]);
	}
	images.Reset();
	rawpicanm.Reset();
	for (auto& a : info.tile)
	{
		a.alphathreshold = 0.5f;
		a.extinfo.tiletovox = -1;
	}
	gi->LoadTextureInfo(info); // initialize game data that must be done before loading .DEF
	LoadDefinitions(info);
	gi->SetupSpecialTextures(info);	// initialize game data that needs .DEF being processed.

	// now that everything has been set up, we can add the textures to the texture manager.
	// To keep things simple everything from .ART files and its replacements will remain in order and 
	// converting between a Build tilenum and a texture ID can done with a single addition.
	// Even though this requires adding quite a few empty textures to the texture manager, it makes things a lot easier,
	// because it ensures an unambiguous mapping and allows communicating with features that only work with tile numbers.
	// as long as no named textures are used.

	auto nulltex = TexMan.GameByIndex(0);	// Use the null texture's backing data for all empty placeholders. 
											// Only the outward facing FGameTexture needs to be different

	firstarttile = TexMan.NumTextures();
	maxarttile = MAXTILES - 1;
	while (maxarttile >= 0 && info.tile[maxarttile].tileimage == nullptr) maxarttile--;
	if (maxarttile < 0) return;	// should never happen, but who knows - maybe someone will make a game without ART files later... :D
	maxarttile++;	// create a placeholder in the first unused spot. This will later get used for all out of range tile numbers.

	int lastid = firstarttile - 1;
	for (int i = 0; i <= maxarttile; i++)
	{
		FTexture* ftex = nullptr;
		FGameTexture* gtex;
		FStringf tname("#%05d", i);
		if (info.tile[i].tileimage == nullptr)
		{
			if (info.tile[i].imported == nullptr || i == 0)
			{
				ftex = nulltex->GetTexture();
				gtex = MakeGameTexture(ftex, tname, ETextureType::Null);
			}
			else
			{
				// Canvas textures can be used directly without wrapping them again.
				gtex = info.tile[i].imported;
			}
		}
		else
		{
			if (info.tile[i].imported) ftex = info.tile[i].imported->GetTexture();
			else ftex = new FImageTexture(info.tile[i].tileimage);
			gtex = MakeGameTexture(ftex, tname, i == 0? ETextureType::FirstDefined : ETextureType::Any);
			gtex->SetOffsets(info.tile[i].leftOffset, info.tile[i].topOffset);
		}
		if (info.tile[i].extinfo.picanm.sf & PICANM_NOFULLBRIGHT_BIT)
		{
			gtex->SetDisableFullbright(true);
		}

		auto id = TexMan.AddGameTexture(gtex, true);
		if (id.GetIndex() != lastid + 1)
		{
			// this should never happen unless the texture manager gets redone in an incompatible fashion.
			I_FatalError("Unable to assign consecutive texture IDs to tile set.");
		}
		lastid = id.GetIndex();
	}
	// Now create the extended info. This will leave room for all regular textures as well so that later code can assign info to these, too.
	// Textures being added afterward will always see the default extinfo, even if they are not covered by this array.
	texExtInfo.Resize(TexMan.NumTextures());
	memset(texExtInfo.Data(), 0, sizeof(texExtInfo[0]) * texExtInfo.Size());
	for (auto& x : texExtInfo) x.tiletovox = -1;
	// now copy all extinfo stuff that got parsed by .DEF or some game specific setup.
	for (int i = 0; i <= maxarttile; i++)
	{
		texExtInfo[i + firstarttile] = info.tile[i].extinfo;
	}

	LoadAliases(firstarttile, maxarttile);

	for (auto& a : info.aliases)
	{
		TexMan.AddAlias(a.first.GetChars(), min(maxarttile, a.second) + firstarttile);
	}
}
