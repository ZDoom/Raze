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


#define TEXCACHE_FREEBUFS() { Xfree(pic), Xfree(packbuf), Xfree(midbuf); }

globaltexcache texcache;

static pthtyp *texcache_tryart(int32_t const dapicnum, int32_t const dapalnum, int32_t const dashade, int32_t dameth)
{
    const int32_t j = dapicnum&(GLTEXCACHEADSIZ-1);
    pthtyp *pth;
    int32_t tintpalnum = -1;
    int32_t searchpalnum = dapalnum;
    polytintflags_t const tintflags = hictinting[dapalnum].f;

    if (tintflags & (HICTINT_USEONART|HICTINT_ALWAYSUSEART))
    {
        tintpalnum = dapalnum;
        dameth &= ~PTH_INDEXED;
        if (!(tintflags & HICTINT_APPLYOVERPALSWAP))
            searchpalnum = 0;
    }

    // load from art
    for (pth=texcache.list[j]; pth; pth=pth->next)
        if (pth->picnum == dapicnum &&
            (dameth & PTH_INDEXED ? (pth->flags & PTH_INDEXED) &&
                                    (pth->flags & PTH_CLAMPED) == TO_PTH_CLAMPED(dameth) :
                 (pth->palnum == dapalnum && pth->shade == dashade &&
                 !(pth->flags & PTH_INDEXED) &&
                 (pth->flags & (PTH_CLAMPED | PTH_HIGHTILE | PTH_NOTRANSFIX)) ==
                     (TO_PTH_CLAMPED(dameth) | TO_PTH_NOTRANSFIX(dameth)) &&
                 polymost_want_npotytex(dameth, tilesiz[dapicnum].y) == !!(pth->flags&PTH_NPOTWALL))))
        {
            if (pth->flags & PTH_INVALIDATED)
            {
                pth->flags &= ~PTH_INVALIDATED;
                gloadtile_art(dapicnum, searchpalnum, tintpalnum, dashade, dameth, pth, 0);
                pth->palnum = dapalnum;
            }

            return pth;
        }

    pth = (pthtyp *)Xcalloc(1,sizeof(pthtyp));

#ifdef TIMING
	cycle_t clock;
	clock.Reset();
	clock.Clock();
#endif
	gloadtile_art(dapicnum, searchpalnum, tintpalnum, dashade, dameth, pth, 1);
	//thl.AddToCache(dapicnum, dapalnum, dameth);
#ifdef TIMING
	clock.Unclock();
	OSD_Printf("Loaded texture %d, palnum %d, meth %d -> %2.3f\n", dapicnum, dapalnum, dameth, clock.TimeMS());
#endif

    pth->palnum = dapalnum;
    pth->next = texcache.list[j];
    texcache.list[j] = pth;

    return pth;
}

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
                pth->flags = TO_PTH_CLAMPED(dameth) | TO_PTH_NOTRANSFIX(dameth) |
                             PTH_HIGHTILE | (drawingskybox>0)*PTH_SKYBOX;
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

// <dashade>: ignored if not in Polymost+r_usetileshades
pthtyp *texcache_fetch(int32_t dapicnum, int32_t dapalnum, int32_t dashade, int32_t dameth)
{
    const int32_t j = dapicnum & (GLTEXCACHEADSIZ - 1);
    hicreplctyp *si = usehightile ? hicfindsubst(dapicnum, dapalnum, hictinting[dapalnum].f & HICTINT_ALWAYSUSEART) : NULL;

    if (drawingskybox && usehightile)
        if ((si = hicfindskybox(dapicnum, dapalnum)) == NULL)
            return NULL;

    if ((globalflags & GLOBAL_NO_GL_TILESHADES) || videoGetRenderMode() != REND_POLYMOST)
        dashade = 0;

    if (!si)
    {
        return (dapalnum >= (MAXPALOOKUPS - RESERVEDPALS) || hicprecaching) ?
                NULL : texcache_tryart(dapicnum, dapalnum, dashade, dameth);
    }

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
            && (pth->flags & (PTH_CLAMPED | PTH_HIGHTILE | PTH_SKYBOX | PTH_NOTRANSFIX))
               == (TO_PTH_CLAMPED(dameth) | TO_PTH_NOTRANSFIX(dameth) | PTH_HIGHTILE | (drawingskybox > 0) * PTH_SKYBOX)
            && (drawingskybox > 0 ? (pth->skyface == drawingskybox) : 1))
        {
            if (pth->flags & PTH_INVALIDATED)
            {
                pth->flags &= ~PTH_INVALIDATED;

				int32_t tilestat = gloadtile_hi(dapicnum, dapalnum, drawingskybox, si, dameth, pth, 0,
                                        (checktintpal > 0) ? 0 : tintflags);  // reload tile

                if (!tilestat)
                    continue;

                if (tilestat == -2)  // bad filename
                    hicclearsubst(dapicnum, dapalnum);

                return (drawingskybox || hicprecaching) ? NULL : texcache_tryart(dapicnum, dapalnum, dashade, dameth);
            }

            return pth;
        }
    }

    pthtyp *pth = (pthtyp *)Xcalloc(1, sizeof(pthtyp));

    // possibly fetch an already loaded multitexture :_)
    if (dapalnum == DETAILPAL && texcache_fetchmulti(pth, si, dapicnum, dameth))
        return pth;

#ifdef TIMING
	cycle_t clock;
	clock.Reset();
	clock.Clock();
#endif
    int32_t tilestat = gloadtile_hi(dapicnum, dapalnum, drawingskybox, si, dameth, pth, 1, (checktintpal > 0) ? 0 : tintflags);
#ifdef TIMING
	clock.Unclock();
	OSD_Printf("Loaded texture %d, palnum %d, meth %d -> %2.3f\n", dapicnum, dapalnum, dameth, clock.TimeMS());
#endif
    if (!tilestat)
    {
        pth->next = texcache.list[j];
        pth->palnum = checkcachepal;
        texcache.list[j] = pth;
        return pth;
    }

    if (tilestat == -2)  // bad filename
        hicclearsubst(dapicnum, dapalnum);

    Xfree(pth);

    return (drawingskybox || hicprecaching) ? NULL : texcache_tryart(dapicnum, dapalnum, dashade, dameth);
}




#endif
