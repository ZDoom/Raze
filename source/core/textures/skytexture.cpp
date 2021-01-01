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

FGameTexture* GetSkyTexture(int basetile, int lognumtiles, const int16_t *tilemap)
{
	char synthname[60];
	
	
	if (lognumtiles == 0 || lognumtiles > 4) 
	{
		// no special handling - let the old code do its job as-is
		return nullptr;
	}
	
	int numtiles = 1 << lognumtiles;
	mysnprintf(synthname, 60, "%04x", basetile);
	for(int i = 0; i < numtiles; i++)
	{
		synthname[4+i] = 'A' + tilemap[i];
	};
	synthname[4+numtiles] = 0;
	auto tex = TexMan.FindGameTexture(synthname);
	if (tex) return tex;
	
	TArray<TexPartBuild> build(numtiles, true);
	int tilewidth = tileWidth(basetile);
	for(int i = 0; i < numtiles; i++)
	{
		auto tex = tileGetTexture(basetile + tilemap[i]);
		if (!tex || !tex->isValid() || tex->GetTexture() == 0) return nullptr;
		build[i].TexImage = static_cast<FImageTexture*>(tex->GetTexture());
		build[i].OriginX = tilewidth * i;
	}
	auto tt = MakeGameTexture(new FImageTexture(new FMultiPatchTexture(tilewidth*numtiles, tileHeight(basetile), build, false, false)), synthname, ETextureType::Override);
	TexMan.AddGameTexture(tt, true);
	return tt;
}
