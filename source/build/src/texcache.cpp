
#include "baselayer.h"
#include "build.h"
#include "lz4.h"
#include "hightile.h"
#include "polymost.h"
#include "texcache.h"
#include "scriptfile.h"
#include "xxhash.h"
#include "kplib.h"

#include "vfs.h"
#include "textures.h"
#include "bitmap.h"
#include "../../glbackend/glbackend.h"

extern int r_detailmapping, r_glowmapping, usehightile, r_useindexedcolortextures;
extern int fixpalette, fixpalswap;

#if 0

void gltexinvalidate(int32_t dapicnum, int32_t dapalnum, int32_t dameth)
{
	const int32_t pic = (dapicnum & (GLTEXCACHEADSIZ - 1));

	for (pthtyp* pth = texcache.list[pic]; pth; pth = pth->next)
		if (pth->picnum == dapicnum && pth->palnum == dapalnum)
		{
			pth->flags |= PTH_INVALIDATED;
			if (pth->flags & PTH_HASFULLBRIGHT)
				pth->ofb->flags |= PTH_INVALIDATED;
		}
}

//Make all textures "dirty" so they reload, but not re-allocate
//This should be much faster than polymost_glreset()
//Use this for palette effects ... but not ones that change every frame!
void gltexinvalidatetype(int32_t type)
{
#if 0
	for (bssize_t j = 0; j <= GLTEXCACHEADSIZ - 1; j++)
	{
		for (pthtyp* pth = texcache.list[j]; pth; pth = pth->next)
		{
			if (type == INVALIDATE_ALL ||
				(type == INVALIDATE_ALL_NON_INDEXED && !(pth->flags & PTH_INDEXED)) ||
				(type == INVALIDATE_ART && pth->hicr == NULL) ||
				(type == INVALIDATE_ART_NON_INDEXED && pth->hicr == NULL && !(pth->flags & PTH_INDEXED)))
			{
				pth->flags |= PTH_INVALIDATED;
				if (pth->flags & PTH_HASFULLBRIGHT)
					pth->ofb->flags |= PTH_INVALIDATED;
			}
		}
	}
#endif

	clearskins(type);

#ifdef DEBUGGINGAIDS
	OSD_Printf("gltexinvalidateall()\n");
#endif
}


void cleartexturecache()
{
	for (bssize_t i = 0; i <= GLTEXCACHEADSIZ - 1; i++)
	{
		for (pthtyp* pth = texcache.list[i]; pth;)
		{
			pthtyp* const next = pth->next;

			if (pth->flags & PTH_HASFULLBRIGHT)
			{
				delete pth->ofb->glpic;
				Xfree(pth->ofb);
			}

			delete pth->glpic;
			Xfree(pth);
			pth = next;
		}

		texcache.list[i] = NULL;
	}
}

static void polymost_setupsampler(FHardwareTexture* tex, const int32_t dameth, int filter)
{

	if (!(dameth & DAMETH_CLAMPED))
	{
		tex->SetSampler(SamplerRepeat);
	}
	else
	{
		// For sprite textures, clamping looks better than wrapping
		tex->SetSampler(SamplerClampXY);
	}
}


void gloadtile_art(int32_t dapic, int32_t dameth, pthtyp* pth, int32_t doalloc)
{
	vec2_16_t const& tsizart = tilesiz[dapic];
	vec2_t siz = { tsizart.x, tsizart.y };
	vec2_t ssiz = siz;
	//POGOTODO: npoty
	char npoty = 0;

	tileLoad(globalpicnum);

	const uint8_t* p = tilePtr(dapic);
	if (!p)
	{
		static uint8_t pix = 255;
		siz.x = siz.y = 1;

		p = &pix;
	}
	{
		if (doalloc)
		{
			assert(pth->glpic == nullptr);
			pth->glpic = GLInterface.NewTexture();
			pth->glpic->CreateTexture(siz.x, siz.y, true, false);
			pth->glpic->SetSampler((dameth & DAMETH_CLAMPED) ? SamplerClampXY : SamplerRepeat);

			polymost_setupsampler(pth->glpic, dameth, 0);
		}
		TArray<uint8_t> flipped(siz.x * siz.y, true);
		FlipNonSquareBlock(flipped.Data(), p, siz.y, siz.x, siz.y);

		pth->glpic->LoadTexture(flipped.Data());
	}


	pth->picnum = dapic;
	pth->palnum = 0;
	pth->shade = 0;
	pth->effects = 0;
	pth->flags = PTH_HASALPHA | PTH_ONEBITALPHA | PTH_INDEXED;
	//pth->hicr = NULL;
	pth->siz = ssiz;
}


#if 0
int32_t gloadtile_hi(int32_t dapic, int32_t dapalnum, int32_t facen, hicreplctyp* hicr,
	int32_t dameth, pthtyp* pth, int32_t doalloc, polytintflags_t effect)
{
	if (!hicr) return -1;

	char* fn;

	if (facen > 0)
	{
		if (!hicr->skybox || facen > 6 || !hicr->skybox->face[facen - 1])
			return -1;

		fn = hicr->skybox->face[facen - 1];
	}
	else
	{
		if (!hicr->filename)
			return -1;

		fn = hicr->filename;
	}

	auto texture = TileFiles.GetTexture(fn);

	if (texture == nullptr)
	{
		OSD_Printf("hightile: %s (pic %d) not found\n", fn, dapic);
		return -2;
	}

	if ((doalloc & 3) == 1)
	{
		pth->glpic = GLInterface.NewTexture();
		pth->glpic->CreateTexture(texture->GetWidth(), texture->GetHeight(), false, true);
	}
	auto image = texture->GetBgraBitmap(nullptr, nullptr);
	bool hasalpha = texture->GetTranslucency();
	bool onebitalpha = texture->isMasked();

	pth->glpic->LoadTexture(image);

#if 0	// I don't really think this is a good idea. The hightile should look indistinguishable to the game compared to the regular one.
	vec2_t tsiz = { texture->GetWidth(), texture->GetHeight() };
	// precalculate scaling parameters for replacement
	if (facen > 0)
		pth->scale = { (float)tsiz.x * (1.0f / 64.f), (float)tsiz.y * (1.0f / 64.f) };
	else
		pth->scale = { (float)tsiz.x / (float)tilesiz[dapic].x, (float)tsiz.y / (float)tilesiz[dapic].y };
#else
	pth->scale = { 1.f,1.f };
#endif

	polymost_setupsampler(pth->glpic, dameth, (hicr->flags & HICR_FORCEFILTER) ? TEXFILTER_ON : -1);

	pth->picnum = dapic;
	pth->effects = effect;
	pth->flags = PTH_HIGHTILE | ((facen > 0) * PTH_SKYBOX) |
		(onebitalpha ? PTH_ONEBITALPHA : 0) |
		(hasalpha ? PTH_HASALPHA : 0) |
		((hicr->flags & HICR_FORCEFILTER) ? PTH_FORCEFILTER : 0);
	pth->skyface = facen;
	//pth->hicr = hicr;

	if (facen > 0) pth->siz = { 64, 64 }; else pth->siz = { tilesiz[dapic].x, tilesiz[dapic].y };
	return 0;
}
#endif

#define TEXCACHE_FREEBUFS() { Xfree(pic), Xfree(packbuf), Xfree(midbuf); }

globaltexcache texcache;




// <dashade>: ignored if not in Polymost+r_usetileshades
pthtyp *texcache_fetch(int32_t dapicnum, int32_t dapalnum, int32_t dashade, int32_t dameth)
{
#if 0
    const int32_t j = dapicnum & (GLTEXCACHEADSIZ - 1);
    hicreplctyp *si = usehightile ? hicfindsubst(dapicnum, dapalnum) : NULL;

	if (drawingskybox && usehightile)
	{
		auto si = hicfindskybox(dapicnum, dapalnum);
		if (si == nullptr)
			return nullptr;
	}

    if (!si)
#endif
	{
        return (dapalnum >= (MAXPALOOKUPS - RESERVEDPALS) || hicprecaching) ?
                NULL : texcache_tryart(dapicnum, dapalnum, dashade, dameth);
    }
#if 0
    /* if palette > 0 && replacement found
     *    no effects are applied to the texture
     * else if palette > 0 && no replacement found
     *    effects are applied to the palette 0 texture if it exists
     */

    polytintflags_t const tintflags = hictinting[dapalnum].f;

    const int32_t checktintpal = (tintflags & HICTINT_APPLYOVERALTPAL) ? 0 : si->palnum;
    const int32_t checkcachepal = ((tintflags & HICTINT_IN_MEMORY) || ((tintflags & HICTINT_APPLYOVERALTPAL) && si->palnum > 0)) ? dapalnum : si->palnum;

    // load a replacement
    for (pthtyp *pth = texcache.list[j]; pth; pth = pth->next)
    {
        if (pth->picnum == dapicnum && pth->palnum == checkcachepal && (checktintpal > 0 ? 1 : (pth->effects == tintflags))
            && (pth->flags & (PTH_HIGHTILE | PTH_SKYBOX)) == (PTH_HIGHTILE | (drawingskybox > 0) * PTH_SKYBOX)
            && (drawingskybox > 0 ? (pth->skyface == drawingskybox) : 1))
        {
            if (pth->flags & PTH_INVALIDATED)
            {
                pth->flags &= ~PTH_INVALIDATED;

				int32_t tilestat = gloadtile_hi(dapicnum, dapalnum, drawingskybox, si, dameth, pth, 0,
                                        (checktintpal > 0) ? 0 : tintflags);  // reload tile

                if (!tilestat)
                    continue;

                return (drawingskybox || hicprecaching) ? NULL : texcache_tryart(dapicnum, dapalnum, dashade, dameth);
            }

            return pth;
        }
    }

    pthtyp *pth = (pthtyp *)Xcalloc(1, sizeof(pthtyp));

    // possibly fetch an already loaded multitexture :_)
    if (dapalnum == DETAILPAL && texcache_fetchmulti(pth, si, dapicnum, dameth))
        return pth;

    int32_t tilestat = gloadtile_hi(dapicnum, dapalnum, drawingskybox, si, dameth, pth, 1, (checktintpal > 0) ? 0 : tintflags);

	if (!tilestat)
    {
        pth->next = texcache.list[j];
        pth->palnum = checkcachepal;
        texcache.list[j] = pth;
        return pth;
    }
    Xfree(pth);

    return (drawingskybox || hicprecaching) ? NULL : texcache_tryart(dapicnum, dapalnum, dashade, dameth);
#endif

}

#endif


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

FHardwareTexture* CreateIndexedTexture(FTexture* tex)
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
//	Retrieve the texture to be used.
//
//===========================================================================

FHardwareTexture* GLInstance::LoadTexture(FTexture* tex, int textype, int palid)
{
	if (textype == TT_TRUECOLOR && palid == 0) textype = TT_HICREPLACE;	// true color tiles with the base palette won't get processed.
	if (textype == TT_INDEXED)
	{
		auto hwtex = tex->GetHardwareTexture(-1);
		if (hwtex) return hwtex;
		else
		{
			hwtex = CreateIndexedTexture(tex);
			tex->SetHardwareTexture(-1, hwtex);
			return hwtex;
		}
	}
}

//===========================================================================
// 
//	Sets a texture for rendering
//
//===========================================================================

bool GLInstance::SetTexture(FTexture* tex, int palette, int method, int sampleroverride)
{
	int usepalette = fixpalette >= 1 ? fixpalette - 1 : curbasepal;
	int usepalswap = fixpalswap >= 1 ? fixpalswap - 1 : palette;
	GLInterface.SetPalette(usepalette);
	GLInterface.SetPalswap(usepalswap);

	TextureType = r_useindexedcolortextures? TT_INDEXED : TT_TRUECOLOR;

	int lookuppal = 0;
	VSMatrix texmat;

	auto rep = usehightile? currentTexture->FindReplacement(palette) : nullptr;
	if (rep)
	{
		// Hightile replacements have only one texture representation and it is always the base.
		tex = rep->faces[0];
		TextureType = TT_HICREPLACE;
	}
	else
	{
		// Only look up the palette if we really want to use it (i.e. when creating a true color texture of an ART tile.)
		if (!r_useindexedcolortextures) lookuppal = palmanager.LookupPalette(usepalette, usepalswap);
	}

	// Load the main texture
	auto mtex = LoadTexture(tex, TextureType, lookuppal);
	if (mtex)
	{
		auto sampler = (method & DAMETH_CLAMPED) ? (sampleroverride != -1 ? sampleroverride : SamplerClampXY) : SamplerRepeat;

		BindTexture(0, mtex, sampler);
		if (rep && ((rep->scale.x != 1.0f) || (rep->scale.y != 1.0f)))
		{
			texmat.loadIdentity();
			texmat.scale(rep->scale.x, rep->scale.y, 1.0f);
			GLInterface.SetMatrix(Matrix_Texture, &texmat);
			MatrixChange |= 1;
		}

		// Also load additional layers needed for this texture.
		if (r_detailmapping && usehightile)
		{
			auto drep = currentTexture->FindReplacement(DETAILPAL);
			if (drep)
			{
				auto htex = LoadTexture(drep->faces[0], TT_HICREPLACE, 0);
				UseDetailMapping(true);
				BindTexture(3, htex, SamplerRepeat);

				texmat.loadIdentity();

				if (rep && ((rep->scale.x != 1.0f) || (rep->scale.y != 1.0f)))
				{
					texmat.scale(rep->scale.x, rep->scale.y, 1.0f);
					MatrixChange |= 2;
				}

				if ((drep->scale.x != 1.0f) || (drep->scale.y != 1.0f))
				{
					texmat.scale(drep->scale.x, drep->scale.y, 1.0f);
					MatrixChange |= 2;
				}
				if (MatrixChange & 2) GLInterface.SetMatrix(Matrix_Detail, &texmat);
			}
		}
		if (r_glowmapping && usehightile)
		{
			auto drep = currentTexture->FindReplacement(GLOWPAL);
			if (drep)
			{
				auto htex = LoadTexture(drep->faces[0], TT_HICREPLACE, 0);
				UseGlowMapping(true);
				BindTexture(4, htex, SamplerRepeat);

			}
		}
		auto brep = currentTexture->FindReplacement(BRIGHTPAL);
		if (brep)
		{
			auto htex = LoadTexture(brep->faces[0], TT_HICREPLACE, 0);
			BindTexture(5, mtex, sampler);
		}
	}

	float al = 0;
	if (TextureType == TT_HICREPLACE)
	{
		al = /*alphahackarray[globalpicnum] != 0 ? alphahackarray[globalpicnum] * (1.f / 255.f) :*/
			(tex->alphaThreshold >= 0.f ? tex->alphaThreshold : 0.f);
	}
	GLInterface.SetAlphaThreshold(al);
}



