/*
** skytexture.cpp
** Composite sky textures for Build.
**
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
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

#include "files.h"
#include "filesystem.h"
#include "image.h"
#include "multipatchtexture.h"
#include "printf.h"
#include "texturemanager.h"
#include "buildtiles.h"

FGameTexture* GetSkyTexture(int basetile, int lognumtiles, const int16_t *tilemap, int remap)
{
	FString synthname;


	if ((lognumtiles == 0 && remap == 0) || lognumtiles > 4 || lognumtiles < 0) 
	{
		// no special handling - let the old code do its job as-is
		return nullptr;
	}

	int numtiles = 1 << lognumtiles;
	synthname.Format("Sky%04x%02x", basetile, remap);
	for(int i = 0; i < numtiles; i++)
	{
		synthname += 'A' + tilemap[i];
	};
	auto tex = TexMan.FindGameTexture(synthname);
	if (tex) return tex;

	TArray<TexPartBuild> build(numtiles, true);
	int tilewidth = tileWidth(basetile);
	for(int i = 0; i < numtiles; i++)
	{
		auto texture = tileGetTexture(basetile + tilemap[i]);
		if (!texture || !texture->isValid() || texture->GetTexture() == 0) return nullptr;
		build[i].TexImage = static_cast<FImageTexture*>(texture->GetTexture());
		build[i].OriginX = tilewidth * i;
		build[i].Translation = GPalette.GetTranslation(GetTranslationType(remap), GetTranslationIndex(remap));
	}
	auto tt = MakeGameTexture(new FImageTexture(new FMultiPatchTexture(tilewidth*numtiles, tileHeight(basetile), build, false, false)), synthname, ETextureType::Override);
	tt->SetUpscaleFlag(tileGetTexture(basetile)->GetUpscaleFlag(), true);
	TexMan.AddGameTexture(tt, true);
	return tt;
}
