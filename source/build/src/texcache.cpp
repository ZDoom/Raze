
#include "baselayer.h"
#include "build.h"
#include "lz4.h"
#include "hightile.h"
#include "polymost.h"
#include "scriptfile.h"
#include "xxhash.h"
#include "kplib.h"

#include "vfs.h"
#include "textures.h"
#include "bitmap.h"
#include "../../glbackend/glbackend.h"

// External CVARs.
extern int r_detailmapping, r_glowmapping, usehightile, r_useindexedcolortextures;
extern int fixpalette, fixpalswap;


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
	bool npoty = false;

	const uint8_t* p = tex->Get8BitPixels();
	TArray<uint8_t> store(siz.x * siz.y, true);
	if (!p)
	{
		tex->Create8BitPixels(store.Data());
		p = store.Data();
	}

	auto glpic = GLInterface.NewTexture();
	glpic->CreateTexture(siz.x, siz.y, true, false);

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

FHardwareTexture* GLInstance::CreateTrueColorTexture(FTexture* tex, int palid, bool checkfulltransparency)
{
	auto siz = tex->GetSize();
	bool npoty = false;

	auto palette = palid < 0? nullptr : palmanager.GetPaletteData(palid);
	auto texbuffer = tex->CreateTexBuffer(palette, CTF_ProcessData);
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
	glpic->CreateTexture(texbuffer.mWidth, texbuffer.mHeight, false, true);
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

	FHardwareTexture *hwtex;
	if (textype == TT_INDEXED)
		hwtex = CreateIndexedTexture(tex);
	else
		hwtex = CreateTrueColorTexture(tex, textype == TT_HICREPLACE? -1 : palid, textype == TT_BRIGHTMAP);
	
	tex->SetHardwareTexture(palid, hwtex);
	return hwtex;
}

//===========================================================================
// 
//	Sets a texture for rendering. This should be the ONLY place to bind in-game textures
//
//===========================================================================

bool GLInstance::SetTextureInternal(int picnum, FTexture* tex, int palette, int method, int sampleroverride, float xpanning, float ypanning, FTexture *det, float detscale, FTexture *glow)
{
	if (picnum == 3692)
	{
		int a = 0;
	}
	if (tex->GetWidth() <= 0 || tex->GetHeight() <= 0) return false;
	int usepalette = fixpalette >= 1 ? fixpalette - 1 : curbasepal;
	int usepalswap = fixpalswap >= 1 ? fixpalswap - 1 : palette;
	GLInterface.SetPalette(usepalette);
	GLInterface.SetPalswap(usepalswap);

	TextureType = r_useindexedcolortextures? TT_INDEXED : TT_TRUECOLOR;

	int lookuppal = 0;
	VSMatrix texmat;

	auto rep = usehightile? tex->FindReplacement(palette) : nullptr;
	if (rep)
	{
		// Hightile replacements have only one texture representation and it is always the base.
		tex = rep->faces[0];
		TextureType = TT_HICREPLACE;
	}
	else
	{
		// Only look up the palette if we really want to use it (i.e. when creating a true color texture of an ART tile.)
		if (!r_useindexedcolortextures) lookuppal = palmanager.LookupPalette(usepalette, usepalswap, false);
	}

	// Load the main texture
	auto mtex = LoadTexture(tex, TextureType, lookuppal);
	if (mtex)
	{
		auto sampler = method & DAMETH_SKY? SamplerClampXY : (method & DAMETH_CLAMPED) ? (sampleroverride != -1 ? sampleroverride : SamplerClampXY) : SamplerRepeat;
		if (TextureType == TT_INDEXED) sampler = sampler + SamplerNoFilterRepeat - SamplerRepeat;

		BindTexture(0, mtex, sampler);
		if (rep && (rep->scale.x != 1.0f || rep->scale.y != 1.0f || xpanning != 0 || ypanning != 0))
		{
			texmat.loadIdentity();
			texmat.translate(xpanning, ypanning, 0);
			texmat.scale(rep->scale.x, rep->scale.y, 1.0f);
			GLInterface.SetMatrix(Matrix_Texture, &texmat);
			MatrixChange |= 1;
		}

		// Also load additional layers needed for this texture.
		if (r_detailmapping && usehightile)
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

				// Q: Pass the scale factor as a separate uniform to get rid of the additional matrix?
				if (MatrixChange & 1) MatrixChange |= 2;
				else texmat.loadIdentity();

				if ((detscalex != 1.0f) || (detscaley != 1.0f))
				{
					texmat.scale(detscalex, detscaley, 1.0f);
					MatrixChange |= 2;
				}
				if (MatrixChange & 2) GLInterface.SetMatrix(Matrix_Detail, &texmat);
			}
		}
		if (r_glowmapping && usehightile)
		{
			if (!(method & DAMETH_MODEL))
			{
				auto drep = tex->FindReplacement(DETAILPAL);
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
			}
		}
		if (!(tex->PicAnim.sf & PICANM_NOFULLBRIGHT_BIT) && !(globalflags & GLOBAL_NO_GL_FULLBRIGHT))
		{
			if (TextureType == TT_HICREPLACE)
			{
				auto brep = tex->FindReplacement(BRIGHTPAL);
				if (brep)
				{
					auto htex = LoadTexture(brep->faces[0], TT_HICREPLACE, 0);
					// UseBrightmapping(true);
					BindTexture(5, mtex, sampler);
				}
			}
			else if (TextureType == TT_TRUECOLOR)
			{
				lookuppal = palmanager.LookupPalette(usepalette, usepalswap, true);
				if (lookuppal >= 0)
				{
					auto htex = LoadTexture(tex, TT_BRIGHTMAP, lookuppal);
					// UseBrightmapping(true);
					BindTexture(5, mtex, sampler);
				}
			}
		}
	}
	else return false;

	float al = 0;
	if (TextureType == TT_HICREPLACE)
	{
		al = ((unsigned)picnum < MAXTILES && alphahackarray[picnum] != 0) ? alphahackarray[picnum] * (1.f / 255.f) :
			(tex->alphaThreshold >= 0.f ? tex->alphaThreshold : 0.f);
	}
	GLInterface.SetAlphaThreshold(al);
	return true;
}


