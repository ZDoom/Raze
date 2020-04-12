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
#include "v_font.h"
#include "palette.h"

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

void PaletteManager::DeleteAllTextures()
{
	for (auto& pal : palettes)
	{
		if (pal.paltexture) delete pal.paltexture;
		pal.paltexture = nullptr;
	}
	for (auto& pal : palswaps)
	{
		if (pal.swaptexture) delete pal.swaptexture;
		pal.swaptexture = nullptr;
	}
	if (palswapTexture) delete palswapTexture;
	palswapTexture = nullptr;
}

void PaletteManager::DeleteAll()
{
	DeleteAllTextures();
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

unsigned PaletteManager::FindPalswap(const uint8_t* paldata, palette_t &fadecolor)
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
	memset(pd.brightcolors, 0, 256);
	pd.isbright = false;

	for (int i = 0; i < 255; i++)
	{
		int firstmap = paldata[i];
		int lastmap = paldata[i + 256 * (numshades - 2)];

		PalEntry color1 = palettes[palettemap[0]].colors[firstmap];
		PalEntry color2 = palettes[palettemap[0]].colors[lastmap];
		int lum1 = color1.Amplitude();
		int lum2 = color2.Amplitude();
		if (lum1 > 40 && lum2 * 10 >= lum1 * 9)
		{
			pd.brightcolors[i] = 255;
			pd.isbright = true;
		}
	}


	if (fadecolor.f == 0)
	{
		// Find what index maps to black (or the darkest available color)
		int found = -1;
		PalEntry foundColor = 0xffffffff;
		for (int i = 0; i < 255; i++)
		{
			int map = paldata[i];
			PalEntry color = palettes[palettemap[0]].colors[map];
			if (color.Luminance() < foundColor.Luminance())
			{
				foundColor = color;
				found = i;
			}
		}

		// Determine the fade color. We pick what black, or the darkest color, maps to in the lowest shade level.
		int map = paldata[(numshades - 2) * 256 + found]; // do not look in the latest shade level because it doesn't always contain useful data for this.
		pd.fadeColor = palettes[palettemap[0]].colors[map];
		if (pd.fadeColor.Luminance() < 10) pd.fadeColor = 0;	// Account for the inability to check the last fade level by using a higher threshold for determining black fog.
	}
	else
	{
		pd.fadeColor = PalEntry(fadecolor.r, fadecolor.g, fadecolor.b);
	}

	return palswaps.Push(pd);
}

//===========================================================================
//
// 
//
//===========================================================================

void PaletteManager::SetPalette(int index, const uint8_t* data)
{
	// New palettes may only be added if declared transient or on startup. 
	// Otherwise this would require a renderer reset to flush out the textures affected by the change.

	if (index < 0 || index > 255) return;	// invalid index - ignore.
	palettemap[index] = FindPalette(data);
}

//===========================================================================
//
// 
//
//===========================================================================

void PaletteManager::BindPalette(int index)
{
	if (palettemap[index] < palettes.Size())
	{
		auto uindex = palettemap[index];
		if (uindex != lastindex)
		{
			lastindex = uindex;
			if (palettes[uindex].paltexture == nullptr)
			{
				auto p = GLInterface.NewTexture();
				p->CreateTexture(256, 1, FHardwareTexture::TrueColor, false);
				p->LoadTexture((uint8_t*)palettes[uindex].colors);
				p->SetSampler(SamplerNoFilterClampXY);
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

void PaletteManager::SetPalswapData(int index, const uint8_t* data, int numshades_, palette_t &fadecolor)
{
	if (index < 0 || index > 255) return;	// invalid index - ignore.
	numshades = numshades_;
	palswapmap[index] = FindPalswap(data, fadecolor);
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
			auto& ps = palswaps[uindex];
			if (ps.swaptexture == nullptr)
			{
				auto p = GLInterface.NewTexture();
				p->CreateTexture(256, numshades, FHardwareTexture::Indexed, false);
				p->LoadTexture((uint8_t*)ps.lookup);
				p->SetSampler(SamplerNoFilterClampXY);
				ps.swaptexture = p;
			}
			inst->BindTexture(1, ps.swaptexture);
			inst->SetFadeColor(ps.fadeColor);
		}
	}

}


int PaletteManager::LookupPalette(int palette, int palswap, bool brightmap, bool nontransparent255)
{
	int realpal = palettemap[palette];
	int realswap = palswapmap[palswap];
	int combined = (nontransparent255? 0x2000000 : 0) + (brightmap? 0x1000000 : 0) + realpal * 0x10000 + realswap;
	int* combinedindex = swappedpalmap.CheckKey(combined);
	if (combinedindex) return *combinedindex;
	
	PaletteData* paldata = &palettes[realpal];
	PalswapData* swapdata = &palswaps[realswap];
	PalEntry swappedpalette[256];
	int end = paldata->colors[255].a == 255 ? 256 : 255;
	if (!brightmap)
	{
		for (int i = 0; i < end; i++)
		{
			int swapi = swapdata->lookup[i];
			swappedpalette[i] = paldata->colors[swapi];
			swappedpalette[i].a = 255;
		}
	}
	else
	{
		if (!swapdata->isbright)
		{
			swappedpalmap.Insert(combined, -1);
			return -1;
		}

		bool found = false;
		memset(swappedpalette, 0, sizeof(swappedpalette));
		for (int i = 0; i < 255; i++)
		{
			int swapi = swapdata->brightcolors[i];
			if (swapi)
			{
				found = true;
				swappedpalette[i] = 0xffffffff;
			}
		}
		if (!found)
		{
			swappedpalmap.Insert(combined, -1);
			return -1;
		}
	}
	if (end == 255) swappedpalette[255] = 0;
	int palid = FindPalette((uint8_t*)swappedpalette);
	swappedpalmap.Insert(combined, palid);
	return palid;
}
