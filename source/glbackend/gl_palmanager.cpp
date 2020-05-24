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
#include "build.h"

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
	for (auto& pal : palswaptextures)
	{
		if (pal) delete pal;
		pal = nullptr;
	}
	lastindex = ~0u;
	lastsindex = ~0u;
}

//===========================================================================
//
// 
//
//===========================================================================

void PaletteManager::BindPalette(int index)
{
	auto palettedata = GPalette.GetTranslation(Translation_BasePalettes, index);
	if (palettedata == nullptr)
	{
		index = 0;
		palettedata = GPalette.GetTranslation(Translation_BasePalettes, index);
	};

	if (palettedata)
	{
		if (index != lastindex)
		{
			lastindex = index;

			if (palettetextures[index] == nullptr)
			{
				auto p = GLInterface.NewTexture();
				p->CreateTexture(256, 1, FHardwareTexture::TrueColor, false);
				p->LoadTexture((uint8_t*)palettedata->Palette);
				p->SetSampler(SamplerNoFilterClampXY);
				palettetextures[index] = p;
			}
			inst->BindTexture(2, palettetextures[index]);
		}
	}

}

//===========================================================================
//
// 
//
//===========================================================================

void PaletteManager::BindPalswap(int index)
{
	if (LookupTables[index].Len() == 0) index = 0;
	if (LookupTables[index].Len() > 0)
	{
		if (index != lastsindex)
		{
			lastsindex = index;
			if (palswaptextures[index] == nullptr)
			{
				auto p = GLInterface.NewTexture();
				p->CreateTexture(256, numshades, FHardwareTexture::Indexed, false);
				p->LoadTexture((uint8_t*)LookupTables[index].GetChars());
				p->SetSampler(SamplerNoFilterClampXY);
				palswaptextures[index] = p;
			}
			inst->BindTexture(1, palswaptextures[index]);
			inst->SetFadeColor(PalEntry(palookupfog[index].r, palookupfog[index].g, palookupfog[index].b));
		}
	}

}


