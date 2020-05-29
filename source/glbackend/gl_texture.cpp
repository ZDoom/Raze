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

bool GLInstance::SetTextureInternal(int picnum, FGameTexture* tex, int paletteid, int method, int sampleroverride, FGameTexture *det, float detscale, FGameTexture *glow)
{
	TexturePick pick;
	if (!PickTexture(picnum, tex, paletteid, pick)) return false;

	GLInterface.SetPalette(GetTranslationType(pick.translation & 0x7fffffff) - Translation_Remap);
	GLInterface.SetPalswap(GetTranslationIndex(pick.translation));
	GLInterface.SetBasepalTint(pick.basepalTint);

	bool texbound[3] = {};
	//int MatrixChange = 0;
	//VSMatrix texmat;

	// This is intentionally the same value for both parameters. The shader does not use the same uniform for modulation and overlay colors.
	GLInterface.SetTinting(pick.tintFlags, pick.tintColor, pick.tintColor);

	// Load the main texture
	
	int TextureType = (pick.translation & 0x80000000) ? TT_INDEXED : TT_TRUECOLOR;
	int lookuppal = pick.translation & 0x7fffffff;
	int bindflags = 0;

	auto mtex = LoadTexture(tex->GetTexture(), TextureType, lookuppal);
	if (mtex)
	{
		auto sampler = (method & DAMETH_CLAMPED) ? (sampleroverride != -1 ? sampleroverride : SamplerClampXY) : SamplerRepeat;
		if (TextureType == TT_INDEXED)
		{
			sampler = sampler + SamplerNoFilterRepeat - SamplerRepeat;
			bindflags = CTF_Indexed;
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
#if 0
		if (rep && (rep->scale.x != 1.0f || rep->scale.y != 1.0f))
		{
			//texmat.loadIdentity();
			//texmat.scale(rep->scale.x, rep->scale.y, 1.0f);
			//GLInterface.SetMatrix(Matrix_Texture, &texmat);
		}
#endif

		// Also load additional layers needed for this texture.
		if (hw_detailmapping)
		{
			auto det = tex->GetDetailmap();
			auto detscale = tex->GetDetailScale();
			if (det)
			{
				auto htex = LoadTexture(det, TT_TRUECOLOR, 0);
				UseDetailMapping(true);
				htex->BindOrCreate(det, 3, CLAMP_NONE, 0, 0);
				BindTexture(3, htex, SamplerRepeat);
				texbound[0] = true;

				/* todo: 
				GLInterface.SetDetailScale(detscale);
				*/
			}
		}
		if (hw_glowmapping)
		{
			auto glow = tex->GetGlowmap();
			if (glow)
			{
				auto htex = LoadTexture(glow, TT_TRUECOLOR, 0);
				UseGlowMapping(true);
				htex->BindOrCreate(glow, 4, sampler, 0, CTF_Upscale);
				BindTexture(4, htex, SamplerRepeat);
				texbound[1] = true;
			}
		}

		if (!(globalflags & GLOBAL_NO_GL_FULLBRIGHT))
		{
			auto btex = tex->GetBrightmap();
			if (btex)
			{
				auto htex = LoadTexture(btex, TT_TRUECOLOR, lookuppal);
				if (htex != nullptr)
				{
					UseBrightmaps(true);
					htex->BindOrCreate(btex, 5, sampler, 0, CTF_Upscale);
					BindTexture(5, htex, sampler);
					texbound[2] = true;
				}
			}
		}
		if (!texbound[0]) UnbindTexture(3);
		if (!texbound[1]) UnbindTexture(4);
		if (!texbound[2]) UnbindTexture(5);

	}
	else return false;

	GLInterface.SetAlphaThreshold(tex->alphaThreshold);
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
