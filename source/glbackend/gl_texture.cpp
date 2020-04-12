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
#include "hightile.h"
#include "polymost.h"
#include "textures.h"
#include "bitmap.h"
#include "v_font.h"
#include "palettecontainer.h"
#include "../../glbackend/glbackend.h"

// Test CVARs.
CVAR(Int, fixpalette, -1, 0)
CVAR(Int, fixpalswap, -1, 0)

template<class T>
void FlipNonSquareBlock(T* dst, const T* src, int x, int y, int srcpitch)
{
	for (int i = 0; i < x; ++i)
	{
		for (int j = 0; j < y; ++j)
		{
			dst[i * y + j] = src[i + j * srcpitch];
		}
	}
}


//===========================================================================
// 
//	Create an indexed version of the requested texture
//
//===========================================================================

FHardwareTexture* GLInstance::CreateIndexedTexture(FTexture* tex)
{
	auto siz = tex->GetSize();

	const uint8_t* p = tex->Get8BitPixels();
	TArray<uint8_t> store(siz.x * siz.y, true);
	if (!p)
	{
		tex->Create8BitPixels(store.Data());
		p = store.Data();
	}

	auto glpic = GLInterface.NewTexture();
	glpic->CreateTexture(siz.x, siz.y, FHardwareTexture::Indexed, false);

	TArray<uint8_t> flipped(siz.x * siz.y, true);
	FlipNonSquareBlock(flipped.Data(), p, siz.y, siz.x, siz.y);
	glpic->LoadTexture(flipped.Data());
	return glpic;
}

//===========================================================================
//
//	Create a true color version of the requested tile
//
//===========================================================================

FHardwareTexture* GLInstance::CreateTrueColorTexture(FTexture* tex, int palid, bool checkfulltransparency, bool rgb8bit)
{
	auto palette = palid < 0? nullptr : palmanager.GetPaletteData(palid);
	if (palid >= 0 && palette == nullptr) return nullptr;
	auto texbuffer = tex->CreateTexBuffer(palette, checkfulltransparency? 0: CTF_ProcessData);
	// Check if the texture is fully transparent. When creating a brightmap such textures can be discarded.
	if (checkfulltransparency)
	{
		int siz = texbuffer.mWidth * texbuffer.mHeight * 4;
		bool found = false;
		for (int i = 3; i < siz; i+=4)
		{
			if (texbuffer.mBuffer[i] > 0)
			{
				found = true;
				break;
			}
		}
		if (!found) return nullptr;
	}

	auto glpic = GLInterface.NewTexture();
	if (!rgb8bit)
		glpic->CreateTexture(texbuffer.mWidth, texbuffer.mHeight, FHardwareTexture::TrueColor, true);
	else
		glpic->CreateTexture(texbuffer.mWidth, texbuffer.mHeight, FHardwareTexture::Brightmap, false);	// Use a more memory friendly format for simple brightmaps.
	glpic->LoadTexture(texbuffer.mBuffer);
	return glpic;
}

//===========================================================================
// 
//	Retrieve the texture to be used.
//
//===========================================================================

FHardwareTexture* GLInstance::LoadTexture(FTexture* tex, int textype, int palid)
{
	if (textype == TT_INDEXED) palid = -1;
	auto phwtex = tex->GetHardwareTexture(palid);
	if (phwtex) return *phwtex;

	FHardwareTexture *hwtex = nullptr;
	if (textype == TT_INDEXED)
		hwtex = CreateIndexedTexture(tex);
	else if (tex->GetUseType() != FTexture::Canvas)
		hwtex = CreateTrueColorTexture(tex, textype == TT_HICREPLACE? -1 : palid, textype == TT_BRIGHTMAP, textype == TT_BRIGHTMAP);
	else
		hwtex = nullptr;
	
	if (hwtex) tex->SetHardwareTexture(palid, hwtex);
	return hwtex;
}

//===========================================================================
// 
//	Sets a texture for rendering. This should be the ONLY place to bind in-game textures
//
//===========================================================================

struct TexturePick
{
	FTexture* texture;		// which texture to use
	int translation;		// which translation table to use
	int tintFlags;			// which shader tinting options to use
	PalEntry tintColor;		// Tint color
	PalEntry basepalTint;	// can the base palette be done with a global tint effect?
};

TexturePick PickTexture(int tilenum, int basepal, int palette)
{
	TexturePick pick = { nullptr, 0, -1, 0xffffff, 0xffffff };
	int usepalette = fixpalette >= 0 ? fixpalette : basepal;
	int usepalswap = fixpalswap >= 0 ? fixpalswap : palette;
	auto& h = hictinting[palette];
	auto tex = TileFiles.tiles[tilenum];
	auto rep = (hw_hightile && !(h.f & HICTINT_ALWAYSUSEART)) ? tex->FindReplacement(usepalswap) : nullptr;
	// Canvas textures must be treated like hightile replacements in the following code.
	bool truecolor = rep || tex->GetUseType() == FTexture::Canvas;
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
		if (!rep || rep->palnum != palette || (h.f & HICTINT_APPLYOVERALTPAL)) applytint = true;
	}
	else
	{
		// Tinting is not used on indexed textures, unless explicitly requested
		if (h.f & (HICTINT_ALWAYSUSEART | HICTINT_USEONART))
		{
			applytint = true;
			if (!(h.f & HICTINT_APPLYOVERPALSWAP)) usepalswap = 0;
		}
		pick.translation = TRANSLATION(usepalette + 1, usepalswap);
	}
	pick.texture = tex;
	if (applytint && h.f)
	{
		pick.tintFlags = h.f;
		pick.tintColor = h.tint;
	}
	return pick;
}

bool GLInstance::SetTextureInternal(int picnum, FTexture* tex, int palette, int method, int sampleroverride, FTexture *det, float detscale, FTexture *glow)
{
	if (tex->GetWidth() <= 0 || tex->GetHeight() <= 0) return false;
	int usepalette = fixpalette >= 0 ? fixpalette : curbasepal;
	int usepalswap = fixpalswap >= 0 ? fixpalswap : palette;
	GLInterface.SetPalette(usepalette);
	GLInterface.SetPalswap(usepalswap);
	bool texbound[3] = {};
	int MatrixChange = 0;

	TextureType = hw_int_useindexedcolortextures? TT_INDEXED : TT_TRUECOLOR;

	int lookuppal = 0;
	VSMatrix texmat;

	GLInterface.SetBasepalTint(0xffffff);

	auto& h = hictinting[palette];
	bool applytint = false;
	// Canvas textures must be treated like hightile replacements in the following code.
	auto rep = (hw_hightile && !(h.f & HICTINT_ALWAYSUSEART)) ? tex->FindReplacement(palette) : nullptr;
	if (rep || tex->GetUseType() == FTexture::Canvas)
	{
		if (usepalette != 0)
		{
			// This is a global setting for the entire scene, so let's do it here, right at the start. (Fixme: Store this in a static table instead of reusing the same entry for all palettes.)
			auto& hh = hictinting[MAXPALOOKUPS - 1];
			// This sets a tinting color for global palettes, e.g. water or slime - only used for hires replacements (also an option for low-resource hardware where duplicating the textures may be problematic.)
			GLInterface.SetBasepalTint(hh.tint);
		}

		if (rep)
		{
			tex = rep->faces[0];
		}
		if (!rep || rep->palnum != palette || (h.f & HICTINT_APPLYOVERALTPAL)) applytint = true;
		TextureType = TT_HICREPLACE;
	}
	else
	{
		// Only look up the palette if we really want to use it (i.e. when creating a true color texture of an ART tile.)
		if (TextureType == TT_TRUECOLOR)
		{
			// Tinting is not used on indexed textures
			if (h.f & (HICTINT_ALWAYSUSEART | HICTINT_USEONART))
			{
				applytint = true;
				if (!(h.f & HICTINT_APPLYOVERPALSWAP)) usepalswap = 0;
			}
#pragma message("fix color 255")
			lookuppal = palmanager.LookupPalette(usepalette, usepalswap, false, 0);// fixpalette < 0 ? !!(curpaletteflags & Pal_Fullscreen) : 0);
		}
	}

	// This is intentionally the same value for both parameters. The shader does not use the same uniform for modulation and overlay colors.
	if (applytint && h.f) 
		GLInterface.SetTinting(h.f, h.tint, h.tint);
	else GLInterface.SetTinting(-1, 0xffffff, 0xffffff);


	// Load the main texture
	auto mtex = LoadTexture(tex, TextureType, lookuppal);
	if (mtex)
	{
		auto sampler = (method & DAMETH_CLAMPED) ? (sampleroverride != -1 ? sampleroverride : SamplerClampXY) : SamplerRepeat;
		if (TextureType == TT_INDEXED)
		{
			sampler = sampler + SamplerNoFilterRepeat - SamplerRepeat;
		}
		UseDetailMapping(false);
		UseGlowMapping(false);
		UseBrightmaps(false);

		BindTexture(0, mtex, sampler);
		// Needs a) testing and b) verification for correctness. This doesn't look like it makes sense.
		if (rep && (rep->scale.x != 1.0f || rep->scale.y != 1.0f))
		{
			//texmat.loadIdentity();
			//texmat.scale(rep->scale.x, rep->scale.y, 1.0f);
			//GLInterface.SetMatrix(Matrix_Texture, &texmat);
		}

		// Also load additional layers needed for this texture.
		if (hw_detailmapping && hw_hightile)
		{
			float detscalex = detscale, detscaley = detscale;
			if (!(method & DAMETH_MODEL))
			{
				auto drep = tex->FindReplacement(DETAILPAL);
				if (drep)
				{
					det = drep->faces[0];
					detscalex = drep->scale.x;
					detscaley = drep->scale.y;
				}
			}
			if (det)
			{
				auto htex = LoadTexture(det, TT_HICREPLACE, 0);
				UseDetailMapping(true);
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
		if (hw_glowmapping && hw_hightile)
		{
			if (!(method & DAMETH_MODEL))
			{
				auto drep = tex->FindReplacement(GLOWPAL);
				if (drep)
				{
					glow = drep->faces[0];
				}
			}
			if (glow)
			{
				auto htex = LoadTexture(glow, TT_HICREPLACE, 0);
				UseGlowMapping(true);
				BindTexture(4, htex, SamplerRepeat);
				texbound[1] = true;
			}
		}
#if 1
		if (!(tex->PicAnim.sf & PICANM_NOFULLBRIGHT_BIT) && !(globalflags & GLOBAL_NO_GL_FULLBRIGHT) && !tex->NoBrightmapFlag[usepalswap])
		{
			if (TextureType == TT_HICREPLACE)
			{
				auto brep = tex->FindReplacement(BRIGHTPAL);
				if (brep)
				{
					LoadTexture(brep->faces[0], TT_HICREPLACE, 0);
					UseBrightmaps(true);
					BindTexture(5, mtex, sampler);
					texbound[2] = true;
				}
				else
				{
					tex->PicAnim.sf |= PICANM_NOFULLBRIGHT_BIT;
				}
			}
			else if (TextureType == TT_TRUECOLOR)
			{
				lookuppal = palmanager.LookupPalette(usepalette, usepalswap, true);
				if (lookuppal >= 0)
				{
					auto htex = LoadTexture(tex, TT_BRIGHTMAP, lookuppal);
					if (htex == nullptr)
					{
						// Flag the texture as not being brightmapped for the given palette
						tex->NoBrightmapFlag.Set(usepalswap);
					}
					else
					{
						UseBrightmaps(true);
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
		al = ((unsigned)picnum < MAXTILES && alphahackarray[picnum] != 0) ? alphahackarray[picnum] * (1.f / 255.f) :
			(tex->alphaThreshold >= 0 ? tex->alphaThreshold * (1.f / 255.f) : 0.f);
	}
	GLInterface.SetAlphaThreshold(al);
	return true;
}


//===========================================================================
// 
// Sets a named texture for 2D rendering. In this case the palette is
// a direct index into the palette map.
//
//===========================================================================

bool GLInstance::SetNamedTexture(FTexture* tex, int palette, int sampler)
{
	auto mtex = LoadTexture(tex, palette>= 0? TT_TRUECOLOR : TT_HICREPLACE, palette);
	if (!mtex) return false;

	BindTexture(0, mtex, sampler);
	GLInterface.SetAlphaThreshold(tex->isTranslucent()? 0.f : 0.5f);
	return true;
}


