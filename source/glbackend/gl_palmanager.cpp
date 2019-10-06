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
	return palettes.Push(pd);
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



#if 0

static void polymost_setPalswap(uint32_t index)
{
	static uint32_t lastPalswapIndex;

	if (currentShaderProgramID != polymost1CurrentShaderProgramID)
		return;

	lastPalswapIndex = index;
	polymost1PalswapPos.x = index * polymost1PalswapSize.x;
	polymost1PalswapPos.y = floorf(polymost1PalswapPos.x);
	polymost1PalswapPos = { polymost1PalswapPos.x - polymost1PalswapPos.y + (0.5f / PALSWAP_TEXTURE_SIZE),
							polymost1PalswapPos.y * polymost1PalswapSize.y + (0.5f / PALSWAP_TEXTURE_SIZE) };
	glUniform2f(polymost1PalswapPosLoc, polymost1PalswapPos.x, polymost1PalswapPos.y);
}

static void polymost_setPalswapSize(uint32_t width, uint32_t height)
{
	if (currentShaderProgramID != polymost1CurrentShaderProgramID)
		return;

	polymost1PalswapSize = { width * (1.f / PALSWAP_TEXTURE_SIZE),
							 height * (1.f / PALSWAP_TEXTURE_SIZE) };

	polymost1PalswapInnerSize = { (width - 1) * (1.f / PALSWAP_TEXTURE_SIZE),
								  (height - 1) * (1.f / PALSWAP_TEXTURE_SIZE) };

	glUniform2f(polymost1PalswapSizeLoc, polymost1PalswapInnerSize.x, polymost1PalswapInnerSize.y);
}


#endif