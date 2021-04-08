/*
** hightile.cpp
** Handling hires replacement definitions
**
**---------------------------------------------------------------------------
** Copyright 2020 Christoph Oelckers
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

#include "files.h"
#include "zstring.h"
#include "buildtiles.h"
#include "image.h"

#include "palette.h"
#include "m_crc32.h"
#include "build.h"
#include "gamecontrol.h"
#include "palettecontainer.h"
#include "texturemanager.h"
#include "c_dispatch.h"
#include "sc_man.h"
#include "gamestruct.h"
#include "hw_renderstate.h"
#include "skyboxtexture.h"

CVARD(Bool, hw_shadeinterpolate, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "enable/disable shade interpolation")

struct HightileReplacement
{
	FGameTexture* image;
	FVector2 scale;
	float alphacut, specpower, specfactor;
	uint16_t palnum;
	bool issky;
};

static TMap<int, TArray<HightileReplacement>> tileReplacements;
static TMap<int, TArray<HightileReplacement>> textureReplacements;

struct FontCharInf
{
	FGameTexture* base;
	FGameTexture* untranslated;
	FGameTexture* translated;
};

static TArray<FontCharInf> deferredChars;

void FontCharCreated(FGameTexture* base, FGameTexture* untranslated, FGameTexture* translated)
{
	// Store these in a list for now - they can only be processed in the finalization step.
	if (translated == untranslated) translated = nullptr;
	FontCharInf fci = { base, untranslated, translated };
	deferredChars.Push(fci);
}


//===========================================================================
//
// Replacement textures
//
//===========================================================================

static void AddReplacement(int picnum, const HightileReplacement& replace)
{
	auto& Hightiles = tileReplacements[picnum];
	for (auto& ht : Hightiles)
	{
		if (replace.palnum == ht.palnum && replace.issky == ht.issky)
		{
			ht = replace;
			return;
		}
	}
	Hightiles.Push(replace);
}

//==========================================================================
//
//  Remove a replacement
//
//==========================================================================

void tileRemoveReplacement(int picnum)
{
	tileReplacements.Remove(picnum);
}

//===========================================================================
//
//
//
//===========================================================================

static HightileReplacement* FindReplacement(FTextureID picnum, int palnum, bool skybox)
{
	auto Hightiles = textureReplacements.CheckKey(picnum.GetIndex());
	if (!Hightiles) return nullptr;
	for (;;)
	{
		for (auto& rep : *Hightiles)
		{
			if (rep.palnum == palnum && rep.issky == skybox) return &rep;
		}
		if (!palnum || palnum >= MAXPALOOKUPS - RESERVEDPALS) break;
		palnum = 0;
	}
	return nullptr;	// no replacement found
}

int checkTranslucentReplacement(FTextureID picnum, int pal)
{
	FGameTexture* tex = nullptr;
	auto si = FindReplacement(picnum, pal, 0);
	if (si && hw_hightile) tex = si->image;
	if (!tex || tex->GetTexelWidth() == 0 || tex->GetTexelHeight() == 0) return false;
	return tex && tex->GetTranslucency();
}

FGameTexture* SkyboxReplacement(FTextureID picnum, int palnum)
{
	auto hr = FindReplacement(picnum, palnum, true);
	if (!hr) return nullptr;
	return hr->image;
}


//==========================================================================
//
// Processes data from .def files into the textures
//
//==========================================================================

void PostLoadSetup()
{
	for(int i=0;i<MAXTILES;i++)
	{
		auto tex = tileGetTexture(i);
		if (!tex->isValid()) continue;
		auto Hightile = tileReplacements.CheckKey(i);
		if (!Hightile) continue;

		FGameTexture* detailTex = nullptr, * glowTex = nullptr, * normalTex = nullptr, *specTex = nullptr;
		float scalex = 1.f, scaley = 1.f;
		for (auto& rep : *Hightile)
		{
			if (rep.palnum == GLOWPAL)
			{
				glowTex = rep.image;
			}
			if (rep.palnum == NORMALPAL)
			{
				normalTex = rep.image;
			}
			if (rep.palnum == SPECULARPAL)
			{
				specTex = rep.image;
			}
			if (rep.palnum == DETAILPAL)
			{
				detailTex = rep.image;
				scalex = rep.scale.X;
				scaley = rep.scale.Y;
			}
		}

		if (detailTex || glowTex || normalTex || specTex)
		{
			for (auto& rep : *Hightile)
			{
				if (rep.issky) continue;	// do not muck around with skyboxes (yet)
				if (rep.palnum < NORMALPAL)
				{
					auto tex = rep.image;
					// Make a copy so that multiple appearances of the same texture with different layers can be handled. They will all refer to the same internal texture anyway.
					tex = MakeGameTexture(tex->GetTexture(), "", ETextureType::Any);
					if (glowTex) tex->SetGlowmap(glowTex->GetTexture());
					if (detailTex) tex->SetDetailmap(detailTex->GetTexture());
					if (normalTex) tex->SetNormalmap(normalTex->GetTexture());
					if (specTex) tex->SetSpecularmap(specTex->GetTexture());
					tex->SetDetailScale(scalex, scaley);
					rep.image = tex;
				}
			}
		}
		textureReplacements.Insert(tex->GetID().GetIndex(), std::move(*Hightile));
	}
	tileReplacements.Clear();

	int i = 0;
	for (auto& ci : deferredChars)
	{
		i++;
		auto rep = textureReplacements.CheckKey(ci.base->GetID().GetIndex());
		if (rep)
		{
			if (ci.untranslated)
			{
				auto rrep = *rep;
				textureReplacements.Insert(ci.untranslated->GetID().GetIndex(), std::move(rrep));
			}

			if (ci.translated)
			{
				//auto reptex = FindReplacement(ci.base->GetID(), 0, false);
				//if (reptex)
				{
					// Todo: apply the translation.
					//auto rrep = *rep;
					//textureReplacements.Insert(ci.translated->GetID().GetIndex(), std::move(rrep));
				}
			}
		}
	}
}

//==========================================================================
//
//   Specifies a replacement texture for an ART tile.
//
//==========================================================================

int tileSetHightileReplacement(int picnum, int palnum, const char* filename, float alphacut, float xscale, float yscale, float specpower, float specfactor)
{
	if ((uint32_t)picnum >= (uint32_t)MAXTILES) return -1;
	if ((uint32_t)palnum >= (uint32_t)MAXPALOOKUPS) return -1;

	auto tex = tileGetTexture(picnum);
	if (tex->GetTexelWidth() <= 0 || tex->GetTexelHeight() <= 0)
	{
		Printf("Warning: defined hightile replacement for empty tile %d.", picnum);
		return -1;	// cannot add replacements to empty tiles, must create one beforehand
	}
	HightileReplacement replace = {};

	FTextureID texid = TexMan.CheckForTexture(filename, ETextureType::Any);
	if (!texid.isValid()) 
	{
		Printf("%s: Replacement for tile %d does not exist or is invalid\n", filename, picnum);
		return -1;
	}

	replace.image = TexMan.GetGameTexture(texid);
    replace.alphacut = min(alphacut,1.f);
	replace.scale = { xscale, yscale };
	replace.specpower = specpower; // currently unused
	replace.specfactor = specfactor; // currently unused
	replace.issky = 0;
	replace.palnum = (uint16_t)palnum;
	AddReplacement(picnum, replace);
	return 0;
}


//==========================================================================
//
//  Define the faces of a skybox
//
//==========================================================================

int tileSetSkybox(int picnum, int palnum, FString* facenames)
{
	if ((uint32_t)picnum >= (uint32_t)MAXTILES) return -1;
	if ((uint32_t)palnum >= (uint32_t)MAXPALOOKUPS) return -1;

	auto tex = tileGetTexture(picnum);
	if (tex->GetTexelWidth() <= 0 || tex->GetTexelHeight() <= 0)
	{
		Printf("Warning: defined skybox replacement for empty tile %d.", picnum);
		return -1;	// cannot add replacements to empty tiles, must create one beforehand
	}
	HightileReplacement replace = {};

	FGameTexture *faces[6];
	for (int i = 0; i < 6; i++)
	{
		FTextureID texid = TexMan.CheckForTexture(facenames[i], ETextureType::Any);
		if (!texid.isValid())
		{
			Printf("%s: Skybox image for tile %d does not exist or is invalid\n", facenames[i].GetChars(), picnum);
			return -1;
		}
		faces[i] = TexMan.GetGameTexture(texid);
	}
	FSkyBox* sbtex = new FSkyBox("");
	memcpy(sbtex->faces, faces, sizeof(faces));
	sbtex->previous = faces[0];	// won't ever be used, just to be safe.
	sbtex->fliptop = true;
	replace.image = MakeGameTexture(sbtex, "", ETextureType::Override);
	TexMan.AddGameTexture(replace.image, false);
    replace.issky = 1;
	replace.palnum = (uint16_t)palnum;
	AddReplacement(picnum, replace);
	return 0;
}

//===========================================================================
// 
//	Picks a texture for rendering for a given tilenum/palette combination
//
//===========================================================================

bool PickTexture(FRenderState *state, FGameTexture* tex, int paletteid, TexturePick& pick)
{
	if (!tex->isValid() || tex->GetTexelWidth() <= 0 || tex->GetTexelHeight() <= 0) return false;

	int usepalette = paletteid == 0? 0 : GetTranslationType(paletteid) - Translation_Remap;
	int usepalswap = GetTranslationIndex(paletteid);
	bool foggy = state && (state->GetFogColor() & 0xffffff);
	int TextureType = hw_int_useindexedcolortextures && !foggy? TT_INDEXED : TT_TRUECOLOR;

	pick.translation = paletteid;
	pick.basepalTint = 0xffffff;

	auto& h = lookups.tables[usepalswap];
	bool applytint = false;
	// Canvas textures must be treated like hightile replacements in the following code.

	int hipalswap = usepalette >= 0 ? usepalswap : 0;
	auto rep = (hw_hightile && !(h.tintFlags & TINTF_ALWAYSUSEART)) ? FindReplacement(tex->GetID(), hipalswap, false) : nullptr;
	if (rep || tex->GetTexture()->isHardwareCanvas())
	{
		if (usepalette > 0)
		{
			// This is a global setting for the entire scene, so let's do it here, right at the start. (Fixme: Store this in a static table instead of reusing the same entry for all palettes.)
			auto& hh = lookups.tables[MAXPALOOKUPS - 1];
			// This sets a tinting color for global palettes, e.g. water or slime - only used for hires replacements (also an option for low-resource hardware where duplicating the textures may be problematic.)
			pick.basepalTint = hh.tintColor;
		}

		if (rep)
		{
			tex = rep->image;
		}
		if (!rep || rep->palnum != hipalswap || (h.tintFlags & TINTF_APPLYOVERALTPAL)) 
			applytint = true;
		pick.translation = 0;
	}
	else
	{
		// Only look up the palette if we really want to use it (i.e. when creating a true color texture of an ART tile.)
		if (TextureType == TT_TRUECOLOR)
		{
			if (h.tintFlags & (TINTF_ALWAYSUSEART | TINTF_USEONART))
			{
				applytint = true;
				if (!(h.tintFlags & TINTF_APPLYOVERPALSWAP)) usepalswap = 0;
			}
			pick.translation = paletteid == 0? 0 : TRANSLATION(usepalette + Translation_Remap, usepalswap);
		}
		else pick.translation |= 0x80000000;
	}

	if (applytint && h.tintFlags)
	{
		pick.tintFlags = h.tintFlags;
		pick.tintColor = h.tintColor;
	}
	else
	{
		pick.tintFlags = -1;
		pick.tintColor = 0xffffff;
	}
	pick.texture = tex;

	return true;
}

bool PreBindTexture(FRenderState* state, FGameTexture*& tex, EUpscaleFlags& flags, int& scaleflags, int& clampmode, int& translation, int& overrideshader)
{
	TexturePick pick;
	auto t = tex;

	if (tex->GetUseType() == ETextureType::Special) return true;

	if (PickTexture(state, tex, translation, pick))
	{
		int TextureType = (pick.translation & 0x80000000) ? TT_INDEXED : TT_TRUECOLOR;
		int lookuppal = pick.translation & 0x7fffffff;

		if (pick.translation & 0x80000000)
		{
			scaleflags |= CTF_Indexed;
			if (state) state->EnableFog(0);
		}
		tex = pick.texture;
		translation = lookuppal;

		FVector4 addcol(0, 0, 0, 0);
		FVector4 modcol(pick.basepalTint.r * (1.f / 255.f), pick.basepalTint.g * (1.f / 255.f), pick.basepalTint.b * (1.f / 255.f), 0);
		FVector4 blendcol(0, 0, 0, 0);
		int flags = 0;

		if (pick.basepalTint != 0xffffff) flags |= TextureManipulation::ActiveBit;
		if (pick.tintFlags != -1)
		{
			flags |= TextureManipulation::ActiveBit;
			if (pick.tintFlags & TINTF_COLORIZE)
			{
				modcol.X *= pick.tintColor.r  * (1.f / 64.f);
				modcol.Y *= pick.tintColor.g * (1.f / 64.f);
				modcol.Z *= pick.tintColor.b * (1.f / 64.f);
			}
			if (pick.tintFlags & TINTF_GRAYSCALE)
				modcol.W = 1.f;

			if (pick.tintFlags & TINTF_INVERT)
				flags |= TextureManipulation::InvertBit;

			if (pick.tintFlags & TINTF_BLENDMASK)
			{
				blendcol = modcol;	// WTF???, but the tinting code really uses the same color for both!
				flags |= (((pick.tintFlags & TINTF_BLENDMASK) >> 6) + 1) & TextureManipulation::BlendMask;
			}
		}
		addcol.W = flags;
		if ((pick.translation & 0x80000000) && hw_shadeinterpolate) addcol.W += 16384;	// hijack a free bit in here.
		state->SetTextureColors(&modcol.X, &addcol.X, &blendcol.X);
	}
	return tex->GetTexelWidth() > t->GetTexelWidth() && tex->GetTexelHeight() > t->GetTexelHeight();	// returning 'true' means to disable programmatic upscaling.
}

