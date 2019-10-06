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
#include "glad/glad.h"
#include "glbackend.h"
#include "gl_samplers.h"
#include "gl_shader.h"

#include "baselayer.h"
#include "resourcefile.h"

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
	if (transientpalette.paltexture) delete transientpalette.paltexture;
	if (palswapTexture) delete palswapTexture;
	palswapTexture = nullptr;
	transientpalette.paltexture = nullptr;
	transientpalette.crc32 = -1;
	palettes.Reset();
	palswaps.Reset();
	lastindex = -1;
	memset(palettemap, 0, sizeof(palettemap));
	memset(palswapmap, 0, sizeof(palswapmap));

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
	auto crc32 = CalcCRC32(paldata, 256);
	for (unsigned int i = 0; i < palswaps.Size(); i++)
	{
		if (crc32 == palswaps[i].crc32)
		{
			if (!memcmp(paldata, palswaps[i].swaps, 256))
			{
				return i;
			}
		}
	}
	PalswapData pd;
	memcpy(pd.swaps, paldata, 256);
	pd.crc32 = crc32;
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

void PaletteManager::SetPalswapData(int index, const uint8_t* data)
{
	// New palettes may only be added if declared transient or on startup. 
	// Otherwise this would require a renderer reset to flush out the textures affected by the change.

	if (index < 0 || index > 255) return;	// invalid index - ignore.
	palswapmap[index] = FindPalswap(data);

}

void PaletteManager::UpdatePalswaps(int width, int height)
{
	if (palswapTexture) delete palswapTexture;
	for (auto& pal : palettes)
	{
		pal.shadesdone = false;
	}
	// recreate it

	vec2f_t polymost1PalswapSize = { width * (1.f / PALSWAP_TEXTURE_SIZE),
							 height * (1.f / PALSWAP_TEXTURE_SIZE) };

	vec2f_t polymost1PalswapInnerSize = { (width - 1) * (1.f / PALSWAP_TEXTURE_SIZE),
								  (height - 1) * (1.f / PALSWAP_TEXTURE_SIZE) };

	inst->SetPalswapSize(&polymost1PalswapInnerSize.x);

}


void GLInstance::SetPalswap(int index)
{
	float v1 = index * renderState.PalswapSize[0];
	float v2 = floorf(v1);
	renderState.PalswapPos[0] = v1 - v2 + (0.5f / PALSWAP_TEXTURE_SIZE);
	renderState.PalswapPos[1] = v2 * renderState.PalswapSize[1] + (0.5f / PALSWAP_TEXTURE_SIZE);
}


#if 0

static void polymost_setPalswapSize(uint32_t width, uint32_t height)
{
	if (currentShaderProgramID != polymost1CurrentShaderProgramID)
		return;

}


#endif

// No point porting this, it's too much work for a short lived solution.
#if 0
char allocateTexture = !palswapTextureID;
if (allocateTexture)
{
	G etTextureHandle(&palswapTextureID);
}
g lBindTexture(GL _TEXTURE_2D, palswapTextureID);
if (allocateTexture)
{
	g lTexParameteri(GL _TEXTURE_2D, GL _TEXTURE_BASE_LEVEL, 0);
	g lTexParameteri(GL _TEXTURE_2D, GL _TEXTURE_MAX_LEVEL, 0);
	g lTexParameteri(GL _TEXTURE_2D, GL _TEXTURE_MAG_FILTER, GL _NEAREST);
	g lTexParameteri(GL _TEXTURE_2D, GL _TEXTURE_MIN_FILTER, GL _NEAREST);
	g lTexParameteri(GL _TEXTURE_2D, GL _TEXTURE_MAX_ANISOTROPY_EXT, 1);
	g lTexParameteri(GL _TEXTURE_2D, GL _TEXTURE_WRAP_S, GL _CLAMP_TO_EDGE);
	g lTexParameteri(GL _TEXTURE_2D, GL _TEXTURE_WRAP_T, GL _CLAMP_TO_EDGE);
	g lTexImage2D(GL _TEXTURE_2D, 0, GL _RED, PALSWAP_TEXTURE_SIZE, PALSWAP_TEXTURE_SIZE, 0, GL _RED, GL _UNSIGNED_BYTE, NULL);
}

int32_t column = palookupnum % (PALSWAP_TEXTURE_SIZE / 256);
int32_t row = palookupnum / (PALSWAP_TEXTURE_SIZE / 256);
int32_t rowOffset = (numshades + 1) * row;
if (rowOffset > PALSWAP_TEXTURE_SIZE)
{
	OSD_Printf("Polymost: palswaps are too large for palswap tilesheet!\n");
	return;
}
g lTexSubImage2D(GL _TEXTURE_2D, 0, 256 * column, rowOffset, 256, numshades + 1, GL _RED, GL _UNSIGNED_BYTE, palookup[palookupnum]);

polymost_setPalswapSize(256, numshades + 1);

static void polymost_updatePalette()
{
	if (videoGetRenderMode() != REND_POLYMOST)
	{
		return;
	}

	polymost_setPalswap(globalpal);
	polymost_setShade(globalshade);

}
#endif
