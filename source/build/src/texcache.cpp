#ifdef USE_OPENGL

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

static pthtyp *texcache_tryart(int32_t const dapicnum, int32_t const dapalnum, int32_t const dashade, int32_t dameth)
{
    const int32_t j = dapicnum&(GLTEXCACHEADSIZ-1);
    pthtyp *pth;
    int32_t tintpalnum = -1;
    int32_t searchpalnum = dapalnum;
    polytintflags_t const tintflags = hictinting[dapalnum].f;

    if (tintflags & (HICTINT_USEONART))
    {
        tintpalnum = dapalnum;
        if (!(tintflags & HICTINT_APPLYOVERPALSWAP))
            searchpalnum = 0;
    }

    // load from art
    for (pth=texcache.list[j]; pth; pth=pth->next)
		if (pth->picnum == dapicnum && ((pth->flags & (PTH_INDEXED|PTH_HIGHTILE)) == PTH_INDEXED))
		{
			if (pth->flags & PTH_INVALIDATED)
			{
				pth->flags &= ~PTH_INVALIDATED;
				gloadtile_art(dapicnum, dameth, pth, 0);
				pth->palnum = dapalnum;
			}

			return pth;
		}

    pth = (pthtyp *)Xcalloc(1,sizeof(pthtyp));

	gloadtile_art(dapicnum, dameth, pth, 1);

    pth->palnum = dapalnum;
    pth->next = texcache.list[j];
    texcache.list[j] = pth;

    return pth;
}

#if 0
pthtyp *texcache_fetchmulti(pthtyp *pth, hicreplctyp *si, int32_t dapicnum, int32_t dameth)
{
    const int32_t j = dapicnum&(GLTEXCACHEADSIZ-1);
    int32_t i;

    for (i = 0; i <= (GLTEXCACHEADSIZ - 1); i++)
    {
        const pthtyp *pth2;

        for (pth2=texcache.list[i]; pth2; pth2=pth2->next)
        {
            if (pth2->hicr && pth2->hicr->filename && si->filename && filnamcmp(pth2->hicr->filename, si->filename) == 0)
            {
                Bmemcpy(pth, pth2, sizeof(pthtyp));
                pth->picnum = dapicnum;
                pth->flags = PTH_HIGHTILE | (drawingskybox>0)*PTH_SKYBOX;
                if (pth2->flags & PTH_HASALPHA)
                    pth->flags |= PTH_HASALPHA;
                pth->hicr = si;

                pth->next = texcache.list[j];
                texcache.list[j] = pth;

                return pth;
            }
        }
    }
    return NULL;
}
#endif

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

extern int r_detailmapping, r_glowmapping, usehightile;

bool GLInstance::ApplyTextureProps()
{
	int pal = palmanager.ActivePalswap();
	if (currentTexture == nullptr) return false;
	auto rep = currentTexture->FindReplacement(pal);
	VSMatrix texmat;
	bool changed = false;

	// texture scale 
	if (rep && ((rep->scale.x != 1.0f) || (rep->scale.y != 1.0f)))
	{
		texmat.loadIdentity();
		texmat.scale(rep->scale.x, rep->scale.y, 1.0f);
		GLInterface.SetMatrix(Matrix_Texture, &texmat);
	}

	if (r_detailmapping)
	{
		auto detailrep = currentTexture->FindReplacement(DETAILPAL);
		if (detailrep)
		{
			UseDetailMapping(true);
			//BindTexture(3, detailrep->faces[0], SamplerRepeat);

			texmat.loadIdentity();
			bool scaled = false;

			if (rep && ((rep->scale.x != 1.0f) || (rep->scale.y != 1.0f)))
			{
				texmat.scale(rep->scale.x, rep->scale.y, 1.0f);
				scaled = true;
			}

			if ((detailrep->scale.x != 1.0f) || (detailrep->scale.y != 1.0f))
			{
				texmat.scale(detailrep->scale.x, detailrep->scale.y, 1.0f);
				scaled = true;
			}

			if (scaled) GLInterface.SetMatrix(Matrix_Detail, &texmat);
			changed |= scaled;
		}
	}

	// glow texture
	if (r_glowmapping)
	{
		auto glowrep = currentTexture->FindReplacement(GLOWPAL);
		if (glowrep)
		{
			UseGlowMapping(true);
			//BindTexture(4, glowrep->faces[0], SamplerRepeat);
		}
	}

	auto brightrep = currentTexture->FindReplacement(BRIGHTPAL);
	if (brightrep)
	{
		//UseGlowMapping(true);
		//BindTexture(5, glowrep->faces[0], SamplerRepeat);
	}

	return false; // true if the matrices were changed, false otherwise
}



void GLInstance::SetTexture(FTexture* tex, int palette, int method)
{

	GLInterface.SetPalswap(fixpalswap >= 1 ? fixpalswap - 1 : globalpal);
	GLInterface.SetPalette(fixpalette >= 1 ? fixpalette - 1 : curbasepal);


	pthtyp* pth = texcache_fetch(globalpicnum, globalpal, getpalookup(1, globalshade), method | PTH_INDEXED);
	GLInterface.BindTexture(0, pth->glpic, mSampler);

	// Fixme: Alpha test on shaders must be done differently.
// Also: Consider a texture's alpha threshold.
	float const al = alphahackarray[globalpicnum] != 0 ? alphahackarray[globalpicnum] * (1.f / 255.f) :
		(pth->hicr && pth->hicr->alphacut >= 0.f ? pth->hicr->alphacut : 0.f);

	GLInterface.SetAlphaThreshold(al);
}


#endif

#endif