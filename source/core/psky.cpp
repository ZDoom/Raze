/*
** pskies.cpp
** Handling Build skies
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

#include "psky.h"
#include "gamefuncs.h"
#include "tarray.h"
#include "texinfo.h"
#include "texturemanager.h"


static TArray<SkyDefinition> skies;

static SkyDefinition *FindSky(FTextureID texid)
{
    for (auto& sky : skies)
        if (texid == sky.texid) return &sky;

    return nullptr;
}

void addSky(SkyDefinition& sky, FTextureID texid)
{
	SkyDefinition* old = FindSky(texid);

	sky.texid = texid;
	if (sky.scale == 0) sky.scale = 1.f;
	
	if (old) *old = sky;
	else skies.Push(sky);
}

SkyDefinition getSky(FTextureID texid)
{
	SkyDefinition result;
	auto sky = FindSky(texid);
	if (sky) result = *sky;
	else
	{
		sky = FindSky(FNullTextureID());
		if (sky)
			result = *sky;
		else
		{
			result = {};
			result.scale = 1.f;
		}
		auto tex = TexMan.GetGameTexture(texid);
		if (tex->isValid())
		{
			int w = (int)tex->GetDisplayWidth();
			if (result.lognumtiles == 0 || w >= 256)
			{
				if (w < 512) result.lognumtiles = 2;
				else if (w < 1024) result.lognumtiles = 1;
				else result.lognumtiles = 0;
			}
		}
	}
	return result;
}

void defineSky(const char* tilename, int lognumtiles, const int16_t* tileofs, int yoff, float yscale, int yoff2)
{
	FTextureID texid = FNullTextureID();
	if (tilename)
	{
		texid = TexMan.CheckForTexture(tilename, ETextureType::Any);
		if (!texid.isValid()) return;
	}
	SkyDefinition sky;
	sky.baselineofs = yoff2 == 0x7fffffff ? yoff : yoff2;
	sky.lognumtiles = lognumtiles;
	sky.scale = yscale;
	memset(sky.offsets, 0, sizeof(sky.offsets));
    if (tileofs) memcpy(sky.offsets, tileofs, 2 << lognumtiles);
	addSky(sky, texid);
}
