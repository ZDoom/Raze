/*
** precache.cpp
**
** 
**
**---------------------------------------------------------------------------
** Copyright 2019-2021 Christoph Oelckers
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
*/

#include "build.h"
#include "palette.h"
#include "v_video.h"
#include "hw_material.h"
#include "glbackend/gl_models.h"

static void PrecacheTex(FGameTexture* tex, int palid)
{
	if (!tex || !tex->isValid()) return;
	int scaleflags = 0;
	if (shouldUpscale(tex, UF_Texture)) scaleflags |= CTF_Upscale;

	auto mat = FMaterial::ValidateTexture(tex, scaleflags);
	screen->PrecacheMaterial(mat, palid);
}

static void doprecache(int32_t dapicnum, int32_t dapalnum, int32_t datype)
{
    // dapicnum and dapalnum are like you'd expect
    // datype is 0 for a wall/floor/ceiling and 1 for a sprite
    //    basically this just means walls are repeating
    //    while sprites are clamped

   if ((dapalnum < (MAXPALOOKUPS - RESERVEDPALS)) && (!lookups.checkTable(dapalnum))) return;//dapalnum = 0;

    //Printf("precached %d %d type %d\n", dapicnum, dapalnum, datype);
    int palid = TRANSLATION(Translation_Remap + curbasepal, dapalnum);
    auto tex = tileGetTexture(dapicnum);
    PrecacheTex(tex, palid);

    if (datype == 0 || !hw_models) return;

    int const mid = md_tilehasmodel(dapicnum, dapalnum);

	if (mid < 0 || models[mid]->mdnum < 2)
	{
		int vox = tiletovox[dapicnum];
		if (vox != -1 && voxmodels[vox] && voxmodels[vox]->model)
		{
			FHWModelRenderer mr(*screen->RenderState(), 0);
			voxmodels[vox]->model->BuildVertexBuffer(&mr);
		}
		return;
	}

    int const surfaces = (models[mid]->mdnum == 3) ? ((md3model_t *)models[mid])->head.numsurfs : 0;

    for (int i = 0; i <= surfaces; i++)
	{
        auto tex = mdloadskin((md2model_t *)models[mid], 0, dapalnum, i, nullptr);
        int palid = TRANSLATION(Translation_Remap + curbasepal, dapalnum);
        if (tex) PrecacheTex(tex, palid);
	}
}

void PrecacheHardwareTextures(int nTile)
{
	// PRECACHE
	// This really *really* needs improvement on the game side - the entire precaching logic has no clue about the different needs of a hardware renderer.
	doprecache(nTile, 0, 1);
}


TMap<int64_t, bool> cachemap;

void markTileForPrecache(int tilenum, int palnum)
{
	int i, j;
	if ((picanm[tilenum].sf & PICANM_ANIMTYPE_MASK) == PICANM_ANIMTYPE_BACK)
	{
		i = tilenum - picanm[tilenum].num;
		j = tilenum;
	}
	else
	{
		i = tilenum;
		j = tilenum + picanm[tilenum].num;
	}

	for (; i <= j; i++)
	{
		int64_t val = i + (int64_t(palnum) << 32);
		cachemap.Insert(val, true);
	}
}

void precacheMarkedTiles()
{
	screen->StartPrecaching();
	decltype(cachemap)::Iterator it(cachemap);
	decltype(cachemap)::Pair* pair;
	while (it.NextPair(pair))
	{
		int dapicnum = pair->Key & 0x7fffffff;
		int dapalnum = pair->Key >> 32;
		doprecache(dapicnum, dapalnum, 0);
	}
}

