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

// Test CVARs.
#ifdef _DEBUG
CVAR(Int, fixpalette, -1, 0)
CVAR(Int, fixpalswap, -1, 0)
#endif

bool GLInstance::SetTexture(int picnum, FGameTexture* tex, int paletteid, int sampler, bool notindexed)
{
#ifdef _DEBUG
	int basepal = GetTranslationType(paletteid) - Translation_Remap;
	int translation = GetTranslationIndex(paletteid);
	int usepalette = fixpalette >= 0 ? fixpalette : basepal;
	int usepalswap = fixpalswap >= 0 ? fixpalswap : translation;
	paletteid = TRANSLATION(Translation_Remap + usepalette, usepalswap);
#endif

	TexturePick texpick;
	if (!PickTexture(picnum, tex, paletteid, texpick)) return false;

	int TextureType = (texpick.translation & 0x80000000) ? TT_INDEXED : TT_TRUECOLOR;
	if (TextureType == TT_INDEXED)
	{
		sampler = sampler + CLAMP_NOFILTER - CLAMP_NONE;
	}


	// This is intentionally the same value for both parameters. The shader does not use the same uniform for modulation and overlay colors.
	SetTinting(texpick.tintFlags, texpick.tintColor, texpick.tintColor);
	int lookuppal = texpick.translation & 0x7fffffff;
	SetPalswap(GetTranslationIndex(lookuppal));

	SetBasepalTint(texpick.basepalTint);
	auto &mat = renderState.mMaterial;
	int flags = (!notindexed && hw_useindexedcolortextures) ? CTF_Indexed : 0;
	mat.mMaterial = FMaterial::ValidateTexture(texpick.texture, flags); // todo allow scaling
	mat.mClampMode = sampler;
	mat.mTranslation = texpick.translation;
	mat.mOverrideShader = -1;
	mat.mChanged = true;
	GLInterface.SetAlphaThreshold(tex->alphaThreshold);
	return true;
}

//===========================================================================
// 
// stand-ins for the texture system. Nothing of this is used right now, but needs to be present to satisfy the linker
//
//===========================================================================

void InitBuildTiles()
{

}

TArray<UserShaderDesc> usershaders;

void UpdateVRModes(bool considerQuadBuffered = true)
{
	// should update the menu.
}
