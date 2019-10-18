/*
** gl_palmanager.cpp
** Palette management
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

#include <memory>
#include "m_crc32.h"
#include "glbackend.h"

#include "baselayer.h"
#include "resourcefile.h"
#include "imagehelpers.h"

//===========================================================================
//
// The palette manager will contain all palettes being used for texture
// creation. It is also responsible for creating palette textures for indexed
// rendering.
//
//===========================================================================

PaletteManager::~PaletteManager()
{
	DeleteAll();
}

//===========================================================================
//
// 
//
//===========================================================================

void PaletteManager::DeleteAll()
{
	for (auto& pal : palettes)
	{
		if (pal.paltexture) delete pal.paltexture;
	}
	for (auto& pal : palswaps)
	{
		if (pal.swaptexture) delete pal.swaptexture;
	}
	if (transientpalette.paltexture) delete transientpalette.paltexture;
	if (palswapTexture) delete palswapTexture;
	palswapTexture = nullptr;
	transientpalette.paltexture = nullptr;
	transientpalette.crc32 = -1;
	palettes.Reset();
	palswaps.Reset();
	lastindex = ~0u;
	lastsindex = ~0u;
	memset(palettemap, 0, sizeof(palettemap));
	memset(palswapmap, 0, sizeof(palswapmap));
	memset(addshade, 0, sizeof(addshade));
	memset(mulshade, 0, sizeof(mulshade));
	numshades = 1;


}

//===========================================================================
//
// Adds a new palette while looking for duplicates.
//
//===========================================================================

unsigned PaletteManager::FindPalette(const uint8_t *paldata)
{
	auto crc32 = CalcCRC32(paldata, 1024);
	for (unsigned int i = 0; i< palettes.Size(); i++)
	{
		if (crc32 == palettes[i].crc32)
		{
			if (!memcmp(paldata, palettes[i].colors, 1024))
			{
				return i;
			}
		}
	}
	PaletteData pd;
	memcpy(pd.colors, paldata, 1024);
	pd.crc32 = crc32;
	pd.paltexture = nullptr;
	pd.shadesdone = false;
	return palettes.Push(pd);
}

//===========================================================================
//
// Adds a new palette while looking for duplicates.
//
//===========================================================================

unsigned PaletteManager::FindPalswap(const uint8_t* paldata)
{
	if (paldata == nullptr) return 0;
	auto crc32 = CalcCRC32(paldata, 256 * numshades);
	for (unsigned int i = 0; i < palswaps.Size(); i++)
	{
		if (crc32 == palswaps[i].crc32)
		{
			if (!memcmp(paldata, palswaps[i].lookup, 256 * numshades))
			{
				return i;
			}
		}
	}
	PalswapData pd;
	pd.lookup = paldata;
	pd.crc32 = crc32;
	pd.swaptexture = nullptr;
	return palswaps.Push(pd);
}

//===========================================================================
//
// 
//
//===========================================================================

void PaletteManager::SetPalette(int index, const uint8_t* data, bool transient)
{
	// New palettes may only be added if declared transient or on startup. 
	// Otherwise this would require a renderer reset to flush out the textures affected by the change.

	if (index < 0 || index > 255) return;	// invalid index - ignore.
	if (transient)
	{
		// Transient palettes do not get stored in the list because it is assumed that they won't be needed for long.
		// Only clear the texture if the palette is different.
		if (memcmp(data, transientpalette.colors, 1024))
		{
			memcpy(transientpalette.colors, data, 1024);
			if (transientpalette.paltexture) delete transientpalette.paltexture;
			transientpalette.paltexture = nullptr;
		}
		transientpalette.crc32 = index;
		palettemap[index] = 0;
		return;
	}
	palettemap[index] = FindPalette(data);
	if (index == 0)
	{
		ImageHelpers::SetPalette((PalEntry*)data);	// Palette 0 is always the reference for downconverting images
	}
}

//===========================================================================
//
// 
//
//===========================================================================

void PaletteManager::BindPalette(int index)
{
	if (index == transientpalette.crc32)
	{
		if (transientpalette.paltexture == nullptr)
		{
			auto p = GLInterface.NewTexture();
			p->CreateTexture(256, 1, false, false);
			p->LoadTexture((uint8_t*)transientpalette.colors);
			p->SetSampler(Sampler2DNoFilter);
			transientpalette.paltexture = p;
		}
		inst->BindTexture(2, transientpalette.paltexture);
	}
	else if (palettemap[index] < palettes.Size())
	{
		auto uindex = palettemap[index];
		if (uindex != lastindex)
		{
			lastindex = uindex;
			if (palettes[uindex].paltexture == nullptr)
			{
				auto p = GLInterface.NewTexture();
				p->CreateTexture(256, 1, false, false);
				p->LoadTexture((uint8_t*)palettes[uindex].colors);
				p->SetSampler(Sampler2DNoFilter);
				palettes[uindex].paltexture = p;
			}
			inst->BindTexture(2, palettes[uindex].paltexture);
		}
	}

}

//===========================================================================
//
// 
//
//===========================================================================

void PaletteManager::SetPalswapData(int index, const uint8_t* data, int numshades_)
{
	if (index < 0 || index > 255) return;	// invalid index - ignore.
	numshades = numshades_;
	palswapmap[index] = FindPalswap(data);
}

//===========================================================================
//
// 
//
//===========================================================================

void PaletteManager::BindPalswap(int index)
{
	if (palswapmap[index] < palswaps.Size())
	{
		auto uindex = palswapmap[index];
		if (uindex != lastsindex)
		{
			lastsindex = uindex;
			if (palswaps[uindex].swaptexture == nullptr)
			{
				auto p = GLInterface.NewTexture();
				p->CreateTexture(256, numshades, true, false);
				p->LoadTexture((uint8_t*)palswaps[uindex].lookup);
				p->SetSampler(Sampler2DNoFilter);
				palswaps[uindex].swaptexture = p;
			}
			inst->BindTexture(1, palswaps[uindex].swaptexture);
		}
	}

}


int PaletteManager::LookupPalette(int palette, int palswap, bool brightmap)
{
	int realpal = palettemap[palette];
	int realswap = palswapmap[palswap];
	int combined = realpal * 0x10000 + realswap;
	int* combinedindex = swappedpalmap.CheckKey(combined);
	if (combinedindex) return *combinedindex;
	
	PaletteData* paldata = &palettes[realpal];
	PalswapData* swapdata = &palswaps[realswap];
	PalEntry swappedpalette[256];
	for (int i = 0; i < 256; i++)
	{
		int swapi = swapdata->lookup[i];
		swappedpalette[i] = paldata->colors[swapi];
	}
	int palid = FindPalette((uint8_t*)swappedpalette);
	swappedpalmap.Insert(combined, palid);
	return palid;
}
