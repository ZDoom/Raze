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
#include "buildtiles.h"
#include "gamefuncs.h"
#include "tarray.h"


static TArray<SkyDefinition> skies;

static SkyDefinition *FindSky(int tilenum)
{
    for (auto& sky : skies)
        if (tilenum == sky.tilenum) return &sky;

    return nullptr;
}

static SkyDefinition *FindSkyCRC(int64_t crc)
{
    for (auto& sky : skies)
        if (crc == sky.crc32) return &sky;

    return nullptr;
}

void addSky(SkyDefinition& sky, int tilenum)
{
	SkyDefinition* old = FindSky(tilenum);

	sky.tilenum = tilenum;
	sky.crc32 = INT64_MAX;
	if (sky.scale == 0) sky.scale = 1.f;
	
	if (old) *old = sky;
	else skies.Push(sky);
}

void addSkyCRC(SkyDefinition& sky, int64_t crc32)
{
	SkyDefinition* old = FindSkyCRC(crc32);
	
	sky.tilenum = -1;
	sky.crc32 = crc32;
	if (sky.scale == 0) sky.scale = 1.f;
	
	if (old) *old = sky;
	else skies.Push(sky);
}

SkyDefinition getSky(int tilenum)
{
	SkyDefinition result;
	auto sky = FindSky(tilenum);
	if (sky) result = *sky;
	else
	{
		// todo: handle CRC.

		sky = FindSky(DEFAULTPSKY);
		if (sky)
			result = *sky;
		else
		{
			result = {};
			result.scale = 1.f;
		}
		int w = tileWidth(tilenum);
		if (result.lognumtiles == 0 || w >= 256)
		{
			if (w < 512) result.lognumtiles = 2;
			else if (w < 1024) result.lognumtiles = 1;
			else result.lognumtiles = 0;
		}

	}
	return result;
}

void defineSky(int tilenum, int lognumtiles, const int16_t *tileofs, int yoff, float yscale, int yoff2)
{
	SkyDefinition sky;
	sky.baselineofs = yoff2 == 0x7fffffff ? yoff : yoff2;
	sky.pmoffset = yoff;
	sky.lognumtiles = lognumtiles;
	sky.scale = yscale;
	memset(sky.offsets, 0, sizeof(sky.offsets));
    if (tileofs) memcpy(sky.offsets, tileofs, 2 << lognumtiles);
	addSky(sky, tilenum);
}
