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
#include "ns.h"
#include "build.h"
#include "palette.h"
#include "v_video.h"
#include "hw_material.h"
#include "gamestruct.h"
#include "gamecontrol.h"
#include "texturemanager.h"
#include "hw_models.h"
#include "hw_voxels.h"
#include "mapinfo.h"

BEGIN_BLD_NS
extern short voxelIndex[MAXTILES];
END_BLD_NS

static void PrecacheTex(FGameTexture* tex, int palid)
{
	if (!tex || !tex->isValid()) return;
	int scaleflags = 0;
	if (shouldUpscale(tex, UF_Texture)) scaleflags |= CTF_Upscale;

	auto mat = FMaterial::ValidateTexture(tex, scaleflags);
	screen->PrecacheMaterial(mat, palid);
}

static void doprecache(int picnum, int palette)
{
   if ((palette < (MAXPALOOKUPS - RESERVEDPALS)) && (!lookups.checkTable(palette))) return;

    int palid = TRANSLATION(Translation_Remap + curbasepal, palette);
    auto tex = tileGetTexture(picnum);
    PrecacheTex(tex, palid);

    if (!hw_models) return;

    int const mid = md_tilehasmodel(picnum, palette);

	if (mid < 0 || models[mid]->mdnum < 2)
	{
		if (r_voxels)
		{
			int vox = tiletovox[picnum];
			if (vox == -1) vox = gi->Voxelize(picnum);
			if (vox == -1 && isBlood()) vox = Blood::voxelIndex[picnum];
			if (vox >= 0 && vox < MAXVOXELS && voxmodels[vox] && voxmodels[vox]->model)
			{
				FHWModelRenderer mr(*screen->RenderState(), 0);
				voxmodels[vox]->model->BuildVertexBuffer(&mr);
			}
		}
		return;
	}

    int const surfaces = (models[mid]->mdnum == 3) ? ((md3model_t *)models[mid])->head.numsurfs : 0;

    for (int i = 0; i <= surfaces; i++)
	{
        auto skintex = mdloadskin((md2model_t *)models[mid], 0, palette, i, nullptr);
        int paletteid = TRANSLATION(Translation_Remap + curbasepal, palette);
        if (skintex) PrecacheTex(skintex, paletteid);
	}
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
		doprecache(dapicnum, dapalnum);
	}

	// Cache everything the map explicitly declares.
	TMap<FString, bool> cachetexmap;
	for (auto& tex : currentLevel->PrecacheTextures) cachetexmap.Insert(tex, true);

	decltype(cachetexmap)::Iterator it2(cachetexmap);
	decltype(cachetexmap)::Pair* pair2;
	while (it2.NextPair(pair2))
	{
		auto tex = TexMan.FindGameTexture(pair2->Key, ETextureType::Any);
		if (tex) PrecacheTex(tex, 0);
	}

	cachemap.Clear();
}

