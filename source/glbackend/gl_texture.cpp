/*
** gl_texture.cpp
** high level GL texture interface
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

#include "palette.h"
#include "build.h"
#include "polymost.h"
#include "textures.h"
#include "bitmap.h"
#include "v_font.h"
#include "palettecontainer.h"
#include "../../glbackend/glbackend.h"
#include "texturemanager.h"
#include "v_video.h"

// Test CVARs.
CVAR(Int, fixpalette, -1, 0)
CVAR(Int, fixpalswap, -1, 0)

//===========================================================================
// 
//	Retrieve the texture to be used.
//
//===========================================================================

OpenGLRenderer::FHardwareTexture* GLInstance::LoadTexture(FTexture *tex, int textype, int palid)
{
	if (textype == TT_INDEXED) palid = -1;
	auto phwtex = tex->SystemTextures.GetHardwareTexture(palid, false);
	if (phwtex) return (OpenGLRenderer::FHardwareTexture*)phwtex;

	auto hwtex = static_cast<OpenGLRenderer::FHardwareTexture*>(screen->CreateHardwareTexture(textype == TT_INDEXED? 1:4));
	if (hwtex) tex->SystemTextures.AddHardwareTexture(palid, false, hwtex);
	return hwtex;
}

//===========================================================================
// 
//	Sets a texture for rendering. This should be the ONLY place to bind in-game textures
//
//===========================================================================

struct TexturePick
{
	FGameTexture* texture;		// which texture to use
	int translation;		// which translation table to use
	int tintFlags;			// which shader tinting options to use
	PalEntry tintColor;		// Tint color
	PalEntry basepalTint;	// can the base palette be done with a global tint effect?
};

#if 0
TexturePick PickTexture(int tilenum, int basepal, int palette)
{
	TexturePick pick = { nullptr, 0, -1, 0xffffff, 0xffffff };
	int usepalette = fixpalette >= 0 ? fixpalette : basepal;
	int usepalswap = fixpalswap >= 0 ? fixpalswap : palette;
	auto& h = hictinting[palette];
	auto tex = TileFiles.tiles[tilenum];
	auto rep = (hw_hightile && !(h.tintFlags & TINTF_ALWAYSUSEART)) ? TileFiles.FindReplacement(tilenum, usepalswap) : nullptr;
	// Canvas textures must be treated like hightile replacements in the following code.
	bool truecolor = rep || tex->GetUseType() == FGameTexture::Canvas;
	bool applytint = false;
	if (truecolor)
	{
		if (usepalette != 0)
		{
			// This is a global setting for the entire scene, so let's do it here, right at the start. (Fixme: Store this in a static table instead of reusing the same entry for all palettes.)
			auto& hh = hictinting[MAXPALOOKUPS - 1];
			// This sets a tinting color for global palettes, e.g. water or slime - only used for hires replacements (also an option for low-resource hardware where duplicating the textures may be problematic.)
			pick.basepalTint = hh.tint;
		}

		if (rep)
		{
			tex = rep->faces[0];
		}
		if (!rep || rep->palnum != palette || (h.tintFlags & TINTF_APPLYOVERALTPAL)) applytint = true;
	}
	else
	{
		// Tinting is not used on indexed textures, unless explicitly requested
		if (h.tintFlags & (TINTF_ALWAYSUSEART | TINTF_USEONART))
		{
			applytint = true;
			if (!(h.tintFlags & TINTF_APPLYOVERPALSWAP)) usepalswap = 0;
		}
		pick.translation = TRANSLATION(usepalette + 1, usepalswap);
	}
	pick.texture = tex;
	if (applytint && h.tintFlags)
	{
		pick.tintFlags = h.tintFlags;
		pick.tintColor = h.tint;
	}
	return pick;
}
#endif

bool GLInstance::SetTextureInternal(int picnum, FGameTexture* tex, int paletteid, int method, int sampleroverride, FGameTexture *det, float detscale, FGameTexture *glow)
{
	if (!tex->isValid() || tex->GetTexelWidth() <= 0 || tex->GetTexelHeight() <= 0) return false;
	int curbasepal = GetTranslationType(paletteid) - Translation_Remap;
	int palette = GetTranslationIndex(paletteid);
	int usepalette = fixpalette >= 0 ? fixpalette : curbasepal;
	int usepalswap = fixpalswap >= 0 ? fixpalswap : palette;
	GLInterface.SetPalette(usepalette);
	GLInterface.SetPalswap(usepalswap);
	bool texbound[3] = {};
	int MatrixChange = 0;

	TextureType = hw_int_useindexedcolortextures? TT_INDEXED : TT_TRUECOLOR;

	int lookuppal = 0;
	int bindflags = 0;
	VSMatrix texmat;

	GLInterface.SetBasepalTint(0xffffff);

	auto& h = lookups.tables[palette];
	bool applytint = false;
	// Canvas textures must be treated like hightile replacements in the following code.
	if (picnum < 0) picnum = TileFiles.GetTileIndex(tex);	// Allow getting replacements also when the texture is not passed by its tile number.
	auto rep = (picnum >= 0 && hw_hightile && !(h.tintFlags & TINTF_ALWAYSUSEART)) ? TileFiles.FindReplacement(picnum, palette) : nullptr;
	if (rep || tex->GetTexture()->isHardwareCanvas())
	{
		if (usepalette != 0)
		{
			// This is a global setting for the entire scene, so let's do it here, right at the start. (Fixme: Store this in a static table instead of reusing the same entry for all palettes.)
			auto& hh = lookups.tables[MAXPALOOKUPS - 1];
			// This sets a tinting color for global palettes, e.g. water or slime - only used for hires replacements (also an option for low-resource hardware where duplicating the textures may be problematic.)
			GLInterface.SetBasepalTint(hh.tintColor);
		}

		if (rep)
		{
			tex = rep->faces[0];
		}
		if (!rep || rep->palnum != palette || (h.tintFlags & TINTF_APPLYOVERALTPAL)) applytint = true;
		TextureType = TT_HICREPLACE;
		bindflags = CTF_Upscale;
	}
	else
	{
		// Only look up the palette if we really want to use it (i.e. when creating a true color texture of an ART tile.)
		if (TextureType == TT_TRUECOLOR)
		{
			// Tinting is not used on indexed textures
			if (h.tintFlags & (TINTF_ALWAYSUSEART | TINTF_USEONART))
			{
				applytint = true;
				if (!(h.tintFlags & TINTF_APPLYOVERPALSWAP)) usepalswap = 0;
			}
			lookuppal = TRANSLATION(usepalette + Translation_Remap, usepalswap);
			bindflags = CTF_Upscale;
		}
		else bindflags = CTF_Indexed;
	}

	// This is intentionally the same value for both parameters. The shader does not use the same uniform for modulation and overlay colors.
	if (applytint && h.tintFlags) 
		GLInterface.SetTinting(h.tintFlags, h.tintColor, h.tintColor);
	else GLInterface.SetTinting(-1, 0xffffff, 0xffffff);


	// Load the main texture
	
	auto mtex = LoadTexture(tex->GetTexture(), TextureType, lookuppal);
	if (mtex)
	{
		auto sampler = (method & DAMETH_CLAMPED) ? (sampleroverride != -1 ? sampleroverride : SamplerClampXY) : SamplerRepeat;
		if (TextureType == TT_INDEXED)
		{
			sampler = sampler + SamplerNoFilterRepeat - SamplerRepeat;
		}
		else if (tex->isHardwareCanvas())
		{
			sampler = CLAMP_CAMTEX;
		}
		UseDetailMapping(false);
		UseGlowMapping(false);
		UseBrightmaps(false);

		mtex->BindOrCreate(tex->GetTexture(), 0, sampler, lookuppal, bindflags);
		BindTexture(0, mtex, sampler);
		// Needs a) testing and b) verification for correctness. This doesn't look like it makes sense.
		if (rep && (rep->scale.x != 1.0f || rep->scale.y != 1.0f))
		{
			//texmat.loadIdentity();
			//texmat.scale(rep->scale.x, rep->scale.y, 1.0f);
			//GLInterface.SetMatrix(Matrix_Texture, &texmat);
		}

		// Also load additional layers needed for this texture.
		if (hw_detailmapping && hw_hightile && picnum > -1)
		{
			float detscalex = detscale, detscaley = detscale;
			if (!(method & DAMETH_MODEL))
			{
				auto drep = TileFiles.FindReplacement(picnum, DETAILPAL);
				if (drep)
				{
					det = drep->faces[0];
					detscalex = drep->scale.x;
					detscaley = drep->scale.y;
				}
			}
			if (det)
			{
				auto htex = LoadTexture(det->GetTexture(), TT_HICREPLACE, 0);
				UseDetailMapping(true);
				htex->BindOrCreate(det->GetTexture(), 3, CLAMP_NONE, 0, 0);
				BindTexture(3, htex, SamplerRepeat);
				texbound[0] = true;


				/* todo: instead of a matrix, just pass a two-component uniform. Using a full matrix here is problematic.
				if (MatrixChange & 1) MatrixChange |= 2;
				else texmat.loadIdentity();
				if ((detscalex != 1.0f) || (detscaley != 1.0f))
				{
					texmat.scale(detscalex, detscaley, 1.0f);
					MatrixChange |= 2;
				}
				if (MatrixChange & 2) GLInterface.SetMatrix(Matrix_Detail, &texmat);
				*/
			}
		}
		if (hw_glowmapping && hw_hightile && picnum > -1)
		{
			if (!(method & DAMETH_MODEL))
			{
				auto drep = TileFiles.FindReplacement(picnum, GLOWPAL);
				if (drep)
				{
					glow = drep->faces[0];
				}
			}
			if (glow)
			{
				auto htex = LoadTexture(glow->GetTexture(), TT_HICREPLACE, 0);
				UseGlowMapping(true);
				htex->BindOrCreate(glow->GetTexture(), 4, sampler, 0, CTF_Upscale);
				BindTexture(4, htex, SamplerRepeat);
				texbound[1] = true;
			}
		}
#if 1
		if (picnum > -1 && !(TileFiles.tiledata[picnum].picanm.sf & PICANM_NOFULLBRIGHT_BIT) && !(globalflags & GLOBAL_NO_GL_FULLBRIGHT))
		{
			if (TextureType == TT_HICREPLACE)
			{
				auto brep = TileFiles.FindReplacement(picnum, BRIGHTPAL);
				if (brep)
				{
					auto mtex = LoadTexture(brep->faces[0]->GetTexture(), TT_HICREPLACE, 0);
					UseBrightmaps(true);
					mtex->BindOrCreate(brep->faces[0]->GetTexture(), 5, sampler, 0, CTF_Upscale);
					BindTexture(5, mtex, sampler);
					texbound[2] = true;
				}
			}
			else if (TextureType == TT_TRUECOLOR)
			{
				auto btex = tex->GetBrightmap();
				if (btex)
				{
					auto htex = LoadTexture(btex, TT_BRIGHTMAP, lookuppal);
					if (htex != nullptr)
					{
						UseBrightmaps(true);
						htex->BindOrCreate(btex, 5, sampler, 0, CTF_Upscale);
						BindTexture(5, htex, sampler);
						texbound[2] = true;
					}
				}
			}
		}
		if (!texbound[0]) UnbindTexture(3);
		if (!texbound[1]) UnbindTexture(4);
		if (!texbound[2]) UnbindTexture(5);
#endif
	}
	else return false;

	float al = 0.5f;
	if (TextureType == TT_HICREPLACE)
	{
		al = ((unsigned)picnum < MAXTILES&& alphahackarray[picnum] != 0) ? alphahackarray[picnum] * (1.f / 255.f) :
			(tex->alphaThreshold >= 0 ? tex->alphaThreshold * (1.f / 255.f) : 0.f);
	}
	GLInterface.SetAlphaThreshold(al);
	return true;
}


//===========================================================================
// 
// stand-ins for the texture system. Nothing of this is used right now, but needs to be present to satisfy the linker
//
//===========================================================================

int PalCheck(int tex)
{
	return tex;
}

void InitBuildTiles()
{

}

TArray<UserShaderDesc> usershaders;
