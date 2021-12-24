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
#include "hw_palmanager.h"

#include "resourcefile.h"
#include "imagehelpers.h"
#include "v_font.h"
#include "palette.h"
#include "build.h"
#include "v_video.h"

static PaletteManager* palmanager;

//===========================================================================
//
// This class manages the hardware data for the indexed render mode.
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
	for (auto& pal : palettetextures)
	{
		if (pal) delete pal;
		pal = nullptr;
	}
	for (auto& pal : lookuptextures)
	{
		if (pal) delete pal;
		pal = nullptr;
	}
}

//===========================================================================
//
// 
//
//===========================================================================

IHardwareTexture *PaletteManager::GetPalette(int index)
{
	auto palettedata = GPalette.GetTranslation(Translation_BasePalettes, index);
	if (palettedata == nullptr)
	{
		index = 0;
		palettedata = GPalette.GetTranslation(Translation_BasePalettes, index);
	};

	if (palettedata)
	{
		if (palettetextures[index] == nullptr)
		{
			auto p = screen->CreateHardwareTexture(4);
			p->CreateTexture((uint8_t*)palettedata->Palette, 256, 1, 15, false, "Palette");
			palettetextures[index] = p;
		}
		return palettetextures[index];
	}
	I_FatalError("palette not initialized for palette emulated render mode");	// we cannot continue if this happens.
	return nullptr;
}

//===========================================================================
//
// 
//
//===========================================================================

IHardwareTexture* PaletteManager::GetLookup(int index)
{
	if (!lookups.checkTable(index)) index = 0;
	if (lookups.checkTable(index))
	{
		if (lookuptextures[index] == nullptr)
		{
			auto p = screen->CreateHardwareTexture(1);

			// Perform a 0<->255 index swap. The lookup tables are still the original data.
			TArray<uint8_t> lookup(numshades * 256, true);
			memcpy(lookup.Data(), lookups.getTable(index), lookup.Size());
			for (int i = 0; i < numshades; i++)
			{
				auto palette = &lookup[i * 256];
				palette[255] = palette[0];
				palette[0] = 0;
				for (int v = 1; v <= 255; v++) if (palette[v] == 0) palette[v] = 255;
			}
			p->CreateTexture(lookup.Data(), 256, numshades, 15, false, "PaletteLookup");
			lookuptextures[index] = p;
		}
		return lookuptextures[index];
	}
	I_FatalError("palette lookups not initialized for palette emulated render mode");	// we cannot continue if this happens.
	return nullptr;
}

//===========================================================================
//
// 
//
//===========================================================================

IHardwareTexture *setpalettelayer(int layer, int translation)
{
	if (!palmanager) palmanager = new PaletteManager;
	if (layer == 1)
		return palmanager->GetPalette(GetTranslationType(translation) - Translation_Remap);
	else if (layer == 2)
		return palmanager->GetLookup(GetTranslationIndex(translation));
	else return nullptr;
}

void ClearPalManager()
{
	if (palmanager) delete palmanager;
	palmanager = nullptr;
}