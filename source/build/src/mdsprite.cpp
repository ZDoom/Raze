//------------------------------------- MD2/MD3 LIBRARY BEGINS -------------------------------------

#ifdef USE_OPENGL

#include "compat.h"
#include "build.h"
#include "glbuild.h"
#include "pragmas.h"
#include "baselayer.h"
#include "engine_priv.h"
#include "hightile.h"
#include "polymost.h"
#include "texcache.h"
#include "mdsprite.h"
#include "cache1d.h"
#include "kplib.h"
#include "common.h"
#include "palette.h"

static int32_t curextra=MAXTILES;

#define MIN_CACHETIME_PRINT 10


static int32_t addtileP(int32_t model,int32_t tile,int32_t pallet)
{
    // tile >= 0 && tile < MAXTILES

    UNREFERENCED_PARAMETER(model);
    if (curextra==MAXTILES+EXTRATILES-1)
    {
        initprintf("warning: max EXTRATILES reached\n");
        return curextra;
    }

    if (tile2model[tile].modelid==-1)
    {
        tile2model[tile].pal=pallet;
        return tile;
    }

    if (tile2model[tile].pal==pallet)
        return tile;

    while (tile2model[tile].nexttile!=-1)
    {
        tile=tile2model[tile].nexttile;
        if (tile2model[tile].pal==pallet)
            return tile;
    }

    tile2model[tile].nexttile=curextra;
    tile2model[curextra].pal=pallet;

    return curextra++;
}

int32_t Ptile2tile(int32_t tile,int32_t palette)
{
    int t = tile;
    while ((tile = tile2model[tile].nexttile) != -1)
        if (tile2model[tile].pal == palette)
        {
            t = tile;
            break;
        }
    return t;
}

#define MODELALLOCGROUP 256
static int32_t nummodelsalloced = 0;

static int32_t maxmodelverts = 0, allocmodelverts = 0;
static int32_t maxmodeltris = 0, allocmodeltris = 0;
static vec3f_t *vertlist = NULL; //temp array to store interpolated vertices for drawing

#ifdef USE_GLEXT
static int32_t allocvbos = 0, curvbo = 0;
static GLuint *vertvbos = NULL;
static GLuint *indexvbos = NULL;
#endif

#ifdef POLYMER
static int32_t *tribuf = NULL;
static int32_t tribufverts = 0;
#endif

static mdmodel_t *mdload(const char *);
static void mdfree(mdmodel_t *);
int32_t globalnoeffect=0;

extern int32_t timerticspersec;

#ifdef USE_GLEXT
void md_freevbos()
{
    int32_t i;

    for (i=0; i<nextmodelid; i++)
        if (models[i]->mdnum == 3)
        {
            md3model_t *m = (md3model_t *)models[i];
            if (m->vbos)
            {
                //            OSD_Printf("freeing model %d vbo\n",i);
                bglDeleteBuffersARB(m->head.numsurfs, m->vbos);
                DO_FREE_AND_NULL(m->vbos);
            }
        }

    if (allocvbos)
    {
        bglDeleteBuffersARB(allocvbos, indexvbos);
        bglDeleteBuffersARB(allocvbos, vertvbos);
        allocvbos = 0;
    }
}
#endif

void freeallmodels()
{
    int32_t i;

    if (models)
    {
        for (i=0; i<nextmodelid; i++) mdfree(models[i]);
        DO_FREE_AND_NULL(models);
        nummodelsalloced = 0;
        nextmodelid = 0;
    }

    Bmemset(tile2model,-1,sizeof(tile2model));
    for (i=0; i<MAXTILES; i++)
        Bmemset(tile2model[i].hudmem, 0, sizeof(tile2model[i].hudmem));

    curextra=MAXTILES;

    if (vertlist)
    {
        DO_FREE_AND_NULL(vertlist);
        allocmodelverts = maxmodelverts = 0;
        allocmodeltris = maxmodeltris = 0;
    }

#ifdef USE_GLEXT
    md_freevbos();
#endif
#ifdef POLYMER
    DO_FREE_AND_NULL(tribuf);
#endif
}


// Skin texture names can be aliased! This is ugly, but at least correct.
static void nullskintexids(GLuint texid)
{
    int32_t i, j;

    for (i=0; i<nextmodelid; i++)
    {
        mdmodel_t *m = models[i];

        if (m->mdnum == 2 || m->mdnum == 3)
        {
            mdskinmap_t *sk;
            md2model_t *m2 = (md2model_t *)m;

            for (j=0; j < m2->numskins * HICTINT_MEMORY_COMBINATIONS; j++)
                if (m2->texid[j] == texid)
                    m2->texid[j] = 0;

            for (sk=m2->skinmap; sk; sk=sk->next)
                for (j=0; j < HICTINT_MEMORY_COMBINATIONS; j++)
                    if (sk->texid[j] == texid)
                        sk->texid[j] = 0;
        }
    }
}

void clearskins(int32_t type)
{
    int32_t i, j;

    for (i=0; i<nextmodelid; i++)
    {
        mdmodel_t *m = models[i];

        if (m->mdnum == 1)
        {
            voxmodel_t *v = (voxmodel_t *)m;

            for (j=0; j<MAXPALOOKUPS; j++)
                if (v->texid[j])
                {
                    bglDeleteTextures(1, &v->texid[j]);
                    v->texid[j] = 0;
                }
        }
        else if ((m->mdnum == 2 || m->mdnum == 3) && type == INVALIDATE_ALL)
        {
            mdskinmap_t *sk;
            md2model_t *m2 = (md2model_t *)m;

            for (j=0; j < m2->numskins * HICTINT_MEMORY_COMBINATIONS; j++)
                if (m2->texid[j])
                {
                    GLuint otexid = m2->texid[j];

                    bglDeleteTextures(1, &m2->texid[j]);
                    m2->texid[j] = 0;

                    nullskintexids(otexid);
                }

            for (sk=m2->skinmap; sk; sk=sk->next)
                for (j=0; j < HICTINT_MEMORY_COMBINATIONS; j++)
                    if (sk->texid[j])
                    {
                        GLuint otexid = sk->texid[j];

                        bglDeleteTextures(1, &sk->texid[j]);
                        sk->texid[j] = 0;

                        nullskintexids(otexid);
                    }
        }
    }

    for (i=0; i<MAXVOXELS; i++)
    {
        voxmodel_t *v = voxmodels[i];
        if (!v) continue;

        for (j=0; j<MAXPALOOKUPS; j++)
            if (v->texid[j])
            {
                bglDeleteTextures(1, &v->texid[j]);
                v->texid[j] = 0;
            }
    }
}

void mdinit()
{
    freeallmodels();
    mdinited = 1;
}

int32_t md_loadmodel(const char *fn)
{
    mdmodel_t *vm, **ml;

    if (!mdinited) mdinit();

    if (nextmodelid >= nummodelsalloced)
    {
        ml = (mdmodel_t **)Xrealloc(models,(nummodelsalloced+MODELALLOCGROUP)*sizeof(void *));
        models = ml; nummodelsalloced += MODELALLOCGROUP;
    }

    vm = mdload(fn); if (!vm) return -1;
    models[nextmodelid++] = vm;
    return nextmodelid-1;
}

int32_t md_setmisc(int32_t modelid, float scale, int32_t shadeoff, float zadd, float yoffset, int32_t flags)
{
    mdmodel_t *m;

    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;
    m = models[modelid];
    m->bscale = scale;
    m->shadeoff = shadeoff;
    m->zadd = zadd;
    m->yoffset = yoffset;
    m->flags = flags;

    return 0;
}

static int32_t framename2index(mdmodel_t *vm, const char *nam)
{
    int32_t i = 0;

    switch (vm->mdnum)
    {
    case 2:
    {
        md2model_t *m = (md2model_t *)vm;
        md2frame_t *fr;
        for (i=0; i<m->numframes; i++)
        {
            fr = (md2frame_t *)&m->frames[i*m->framebytes];
            if (!Bstrcmp(fr->name, nam)) break;
        }
    }
    break;
    case 3:
    {
        md3model_t *m = (md3model_t *)vm;
        for (i=0; i<m->numframes; i++)
            if (!Bstrcmp(m->head.frames[i].nam,nam)) break;
    }
    break;
    }
    return i;
}

int32_t md_defineframe(int32_t modelid, const char *framename, int32_t tilenume, int32_t skinnum, float smoothduration, int32_t pal)
{
    md2model_t *m;
    int32_t i;

    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;
    if ((uint32_t)tilenume >= (uint32_t)MAXTILES) return -2;
    if (!framename) return -3;

    tilenume=addtileP(modelid,tilenume,pal);
    m = (md2model_t *)models[modelid];
    if (m->mdnum == 1)
    {
        tile2model[tilenume].modelid = modelid;
        tile2model[tilenume].framenum = tile2model[tilenume].skinnum = 0;
        return 0;
    }

    i = framename2index((mdmodel_t *)m,framename);
    if (i == m->numframes) return -3;   // frame name invalid

    tile2model[tilenume].modelid = modelid;
    tile2model[tilenume].framenum = i;
    tile2model[tilenume].skinnum = skinnum;
    tile2model[tilenume].smoothduration = Blrintf((float)UINT16_MAX * smoothduration);

    return i;
}

int32_t md_defineanimation(int32_t modelid, const char *framestart, const char *frameend, int32_t fpssc, int32_t flags)
{
    md2model_t *m;
    mdanim_t ma, *map;
    int32_t i;

    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;

    Bmemset(&ma, 0, sizeof(ma));
    m = (md2model_t *)models[modelid];
    if (m->mdnum < 2) return 0;

    //find index of start frame
    i = framename2index((mdmodel_t *)m,framestart);
    if (i == m->numframes) return -2;
    ma.startframe = i;

    //find index of finish frame which must trail start frame
    i = framename2index((mdmodel_t *)m,frameend);
    if (i == m->numframes) return -3;
    ma.endframe = i;

    ma.fpssc = fpssc;
    ma.flags = flags;

    map = (mdanim_t *)Xmalloc(sizeof(mdanim_t));

    Bmemcpy(map, &ma, sizeof(ma));

    map->next = m->animations;
    m->animations = map;

    return 0;
}

#if 0
// FIXME: CURRENTLY DISABLED: interpolation may access frames we consider 'unused'?
int32_t md_thinoutmodel(int32_t modelid, uint8_t *usedframebitmap)
{
    md3model_t *m;
    md3surf_t *s;
    mdanim_t *anm;
    int32_t i, surfi, sub, usedframes;
    static int16_t otonframe[1024];

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;
    m = (md3model_t *)models[modelid];
    if (m->mdnum != 3) return -2;

    for (anm=m->animations; anm; anm=anm->next)
    {
        if (anm->endframe <= anm->startframe)
        {
//            initprintf("backward anim %d-%d\n", anm->startframe, anm->endframe);
            return -3;
        }

        for (i=anm->startframe; i<anm->endframe; i++)
            usedframebitmap[i>>3] |= (1<<(i&7));
    }

    sub = 0;
    for (i=0; i<m->numframes; i++)
    {
        if (!(usedframebitmap[i>>3]&(1<<(i&7))))
        {
            sub++;
            otonframe[i] = -1;
            continue;
        }

        otonframe[i] = i-sub;
    }

    usedframes = m->numframes - sub;
    if (usedframes==0 || usedframes==m->numframes)
        return usedframes;

    //// THIN OUT! ////

    for (i=0; i<m->numframes; i++)
    {
        if (otonframe[i]>=0 && otonframe[i] != i)
        {
            if (m->muladdframes)
                Bmemcpy(&m->muladdframes[2*otonframe[i]], &m->muladdframes[2*i], 2*sizeof(vec3f_t));
            Bmemcpy(&m->head.frames[otonframe[i]], &m->head.frames[i], sizeof(md3frame_t));
        }
    }

    for (surfi=0; surfi < m->head.numsurfs; surfi++)
    {
        s = &m->head.surfs[surfi];

        for (i=0; i<m->numframes; i++)
            if (otonframe[i]>=0 && otonframe[i] != i)
                Bmemcpy(&s->xyzn[otonframe[i]*s->numverts], &s->xyzn[i*s->numverts], s->numverts*sizeof(md3xyzn_t));
    }

    ////// tweak frame indices in various places

    for (anm=m->animations; anm; anm=anm->next)
    {
        if (otonframe[anm->startframe]==-1 || otonframe[anm->endframe-1]==-1)
            initprintf("md %d WTF: anm %d %d\n", modelid, anm->startframe, anm->endframe);

        anm->startframe = otonframe[anm->startframe];
        anm->endframe = otonframe[anm->endframe-1];
    }

    for (i=0; i<MAXTILES+EXTRATILES; i++)
        if (tile2model[i].modelid == modelid)
        {
            if (otonframe[tile2model[i].framenum]==-1)
                initprintf("md %d WTF: tile %d, fr %d\n", modelid, i, tile2model[i].framenum);
            tile2model[i].framenum = otonframe[tile2model[i].framenum];
        }

    ////// realloc & change "numframes" everywhere

    if (m->muladdframes)
        m->muladdframes = Xrealloc(m->muladdframes, 2*sizeof(vec3f_t)*usedframes);
    m->head.frames = Xrealloc(m->head.frames, sizeof(md3frame_t)*usedframes);

    for (surfi=0; surfi < m->head.numsurfs; surfi++)
    {
        m->head.surfs[surfi].numframes = usedframes;
        // CAN'T do that because xyzn is offset from a larger block when loaded from md3:
//        m->head.surfs[surfi].xyzn = Xrealloc(m->head.surfs[surfi].xyzn, s->numverts*usedframes*sizeof(md3xyzn_t));
    }

    m->head.numframes = usedframes;
    m->numframes = usedframes;

    ////////////
    return usedframes;
}
#endif

int32_t md_defineskin(int32_t modelid, const char *skinfn, int32_t palnum, int32_t skinnum, int32_t surfnum, float param, float specpower, float specfactor, int32_t flags)
{
    mdskinmap_t *sk, *skl;
    md2model_t *m;

    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;
    if (!skinfn) return -2;
    if ((unsigned)palnum >= (unsigned)MAXPALOOKUPS) return -3;

    m = (md2model_t *)models[modelid];
    if (m->mdnum < 2) return 0;
    if (m->mdnum == 2) surfnum = 0;

    skl = NULL;
    for (sk = m->skinmap; sk; skl = sk, sk = sk->next)
        if (sk->palette == (uint8_t)palnum && skinnum == sk->skinnum && surfnum == sk->surfnum)
            break;
    if (!sk)
    {
        sk = (mdskinmap_t *)Xcalloc(1,sizeof(mdskinmap_t));

        if (!skl) m->skinmap = sk;
        else skl->next = sk;
    }
    else Bfree(sk->fn);

    sk->palette = (uint8_t)palnum;
    sk->flags = (uint8_t)flags;
    sk->skinnum = skinnum;
    sk->surfnum = surfnum;
    sk->param = param;
    sk->specpower = specpower;
    sk->specfactor = specfactor;
    sk->fn = Xstrdup(skinfn);

    return 0;
}

int32_t md_definehud(int32_t modelid, int32_t tilex, vec3f_t add, int32_t angadd, int32_t flags, int32_t fov)
{
    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;
    if ((uint32_t)tilex >= (uint32_t)MAXTILES) return -2;

    tile2model[tilex].hudmem[(flags>>2)&1] = (hudtyp *)Xmalloc(sizeof(hudtyp));

    hudtyp * const hud = tile2model[tilex].hudmem[(flags>>2)&1];

    hud->add = add;
    hud->angadd = ((int16_t)angadd)|2048;
    hud->flags = (int16_t)flags;
    hud->fov = (int16_t)fov;

    return 0;
}

int32_t md_undefinetile(int32_t tile)
{
    if (!mdinited) return 0;
    if ((unsigned)tile >= (unsigned)MAXTILES) return -1;

    tile2model[tile].modelid = -1;
    tile2model[tile].nexttile = -1;
    DO_FREE_AND_NULL(tile2model[tile].hudmem[0]);
    DO_FREE_AND_NULL(tile2model[tile].hudmem[1]);
    return 0;
}

/* this function is problematic, it leaves NULL holes in model[]
 * (which runs from 0 to nextmodelid-1) */
int32_t md_undefinemodel(int32_t modelid)
{
    int32_t i;
    if (!mdinited) return 0;
    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;

    for (i=MAXTILES+EXTRATILES-1; i>=0; i--)
        if (tile2model[i].modelid == modelid)
        {
            tile2model[i].modelid = -1;
            DO_FREE_AND_NULL(tile2model[i].hudmem[0]);
            DO_FREE_AND_NULL(tile2model[i].hudmem[1]);
        }

    if (models)
    {
        mdfree(models[modelid]);
        models[modelid] = NULL;
    }

    return 0;
}

static inline int32_t hicfxmask(size_t pal)
{
    return globalnoeffect ? 0 : (hictinting[pal].f & HICTINT_IN_MEMORY);
}
static inline int32_t hicfxid(size_t pal)
{
    return globalnoeffect ? 0 : ((hictinting[pal].f & (HICTINT_GRAYSCALE|HICTINT_INVERT|HICTINT_COLORIZE)) | ((hictinting[pal].f & HICTINT_BLENDMASK)<<3));
}

static int32_t mdloadskin_notfound(char * const skinfile, char const * const fn)
{
    OSD_Printf("Skin \"%s\" not found.\n", fn);

    skinfile[0] = 0;
    return 0;
}

static int32_t mdloadskin_failed(char * const skinfile, char const * const fn)
{
    OSD_Printf("Failed loading skin file \"%s\".\n", fn);

    skinfile[0] = 0;
    return 0;
}

//Note: even though it says md2model, it works for both md2model&md3model
int32_t mdloadskin(md2model_t *m, int32_t number, int32_t pal, int32_t surf)
{
    int32_t i;
    char *skinfile, fn[BMAX_PATH];
    GLuint *texidx = NULL;
    mdskinmap_t *sk, *skzero = NULL;
    int32_t doalloc = 1;

    if (m->mdnum == 2)
        surf = 0;

    if ((unsigned)pal >= (unsigned)MAXPALOOKUPS)
        return 0;

    i = -1;
    for (sk = m->skinmap; sk; sk = sk->next)
    {
        if (sk->palette == pal && sk->skinnum == number && sk->surfnum == surf)
        {
            skinfile = sk->fn;
            texidx = &sk->texid[hicfxid(pal)];
            Bstrncpyz(fn, skinfile, BMAX_PATH);
            //OSD_Printf("Using exact match skin (pal=%d,skinnum=%d,surfnum=%d) %s\n",pal,number,surf,skinfile);
            break;
        }
        //If no match, give highest priority to number, then pal.. (Parkar's request, 02/27/2005)
        else if ((sk->palette ==   0) && (sk->skinnum == number) && (sk->surfnum == surf) && (i < 5)) { i = 5; skzero = sk; }
        else if ((sk->palette == pal) && (sk->skinnum ==      0) && (sk->surfnum == surf) && (i < 4)) { i = 4; skzero = sk; }
        else if ((sk->palette ==   0) && (sk->skinnum ==      0) && (sk->surfnum == surf) && (i < 3)) { i = 3; skzero = sk; }
        else if ((sk->palette ==   0) && (sk->skinnum == number) && (i < 2)) { i = 2; skzero = sk; }
        else if ((sk->palette == pal) && (sk->skinnum ==      0) && (i < 1)) { i = 1; skzero = sk; }
        else if ((sk->palette ==   0) && (sk->skinnum ==      0) && (i < 0)) { i = 0; skzero = sk; }
    }

    if (!sk)
    {
        if (pal >= (MAXPALOOKUPS - RESERVEDPALS))
            return 0;

        if (skzero)
        {
            skinfile = skzero->fn;
            texidx = &skzero->texid[hicfxid(pal)];
            Bstrncpyz(fn, skinfile, BMAX_PATH);
            //OSD_Printf("Using def skin 0,0 as fallback, pal=%d\n", pal);
        }
        else
            return 0;
#if 0
        {
            // fall back to the model-defined texture
            if ((unsigned)number >= (unsigned)m->numskins)
                number = 0;

            // m->skinfn is undefined when md3model_t is cast to md2model_t --> crash
            skinfile = m->skinfn + number*64;
            texidx = &m->texid[number * HICTINT_MEMORY_COMBINATIONS + hicfxid(pal)];
            Bstrncpyz(fn, m->basepath, BMAX_PATH);
            if ((Bstrlen(fn) + Bstrlen(skinfile)) < BMAX_PATH)
                Bstrcat(fn,skinfile);
            //OSD_Printf("Using MD2/MD3 skin (%d) %s, pal=%d\n",number,skinfile,pal);
        }
#endif
    }

    if (!skinfile[0])
        return 0;

    if (*texidx)
        return *texidx;

    // possibly fetch an already loaded multitexture :_)
    if (pal >= (MAXPALOOKUPS - RESERVEDPALS))
        for (i=0; i<nextmodelid; i++)
            for (skzero = ((md2model_t *)models[i])->skinmap; skzero; skzero = skzero->next)
                if (!Bstrcasecmp(skzero->fn, sk->fn) && skzero->texid[hicfxid(pal)])
                {
                    size_t f = hicfxid(pal);

                    sk->texid[f] = skzero->texid[f];
                    return sk->texid[f];
                }

    // for sk->flags below
    if (!sk)
        sk = skzero;

    *texidx = 0;

    int32_t filh;
    if ((filh = kopen4load(fn, 0)) < 0)
        return mdloadskin_notfound(skinfile, fn);


    int32_t picfillen = kfilelength(filh);
    kclose(filh);	// FIXME: shouldn't have to do this. bug in cache1d.c

    int32_t startticks = getticks(), willprint = 0;

    char hasalpha;
    texcacheheader cachead;
    char texcacheid[BMAX_PATH];
    texcache_calcid(texcacheid, fn, picfillen, pal<<8, hicfxmask(pal));
    int32_t gotcache = texcache_readtexheader(texcacheid, &cachead, 1);
    vec2_t siz = { 0, 0 }, tsiz = { 0, 0 };

    if (gotcache && !texcache_loadskin(&cachead, &doalloc, texidx, &siz))
    {
        tsiz.x = cachead.xdim;
        tsiz.y = cachead.ydim;
        hasalpha = !!(cachead.flags & CACHEAD_HASALPHA);

        if (pal < (MAXPALOOKUPS - RESERVEDPALS))
            m->usesalpha = hasalpha;
    }
    else
    {
        polytintflags_t const effect = hicfxmask(pal);

        // CODEDUP: gloadtile_hi

        int32_t isart = 0;

        gotcache = 0;	// the compressed version will be saved to disk

        int32_t const length = kpzbufload(fn);
        if (length == 0)
            return mdloadskin_notfound(skinfile, fn);

        // tsizx/y = replacement texture's natural size
        // xsiz/y = 2^x size of replacement

#ifdef WITHKPLIB
        kpgetdim(kpzbuf,picfillen,&tsiz.x,&tsiz.y);
#endif

        if (tsiz.x == 0 || tsiz.y == 0)
        {
            if (E_CheckUnitArtFileHeader((uint8_t *)kpzbuf, picfillen))
                return mdloadskin_failed(skinfile, fn);

            tsiz.x = B_LITTLE16(B_UNBUF16(&kpzbuf[16]));
            tsiz.y = B_LITTLE16(B_UNBUF16(&kpzbuf[18]));

            if (tsiz.x == 0 || tsiz.y == 0)
                return mdloadskin_failed(skinfile, fn);

            isart = 1;
        }

        if (!glinfo.texnpot)
        {
            for (siz.x=1; siz.x<tsiz.x; siz.x+=siz.x) { }
            for (siz.y=1; siz.y<tsiz.y; siz.y+=siz.y) { }
        }
        else
            siz = tsiz;

        if (isart)
        {
            if (tsiz.x * tsiz.y + ARTv1_UNITOFFSET > picfillen)
                return mdloadskin_failed(skinfile, fn);
        }

        int32_t const bytesperline = siz.x * sizeof(coltype);
        coltype *pic = (coltype *)Xcalloc(siz.y, bytesperline);

        static coltype *lastpic = NULL;
        static char *lastfn = NULL;
        static int32_t lastsize = 0;

        if (lastpic && lastfn && !Bstrcmp(lastfn,fn))
        {
            willprint=1;
            Bmemcpy(pic, lastpic, siz.x*siz.y*sizeof(coltype));
        }
        else
        {
            if (isart)
            {
                E_RenderArtDataIntoBuffer((palette_t *)pic, (uint8_t *)&kpzbuf[ARTv1_UNITOFFSET], siz.x, tsiz.x, tsiz.y);
            }
#ifdef WITHKPLIB
            else
            {
                if (kprender(kpzbuf,picfillen,(intptr_t)pic,bytesperline,siz.x,siz.y))
                {
                    Bfree(pic);
                    return mdloadskin_failed(skinfile, fn);
                }
            }
#endif

            willprint=2;

            if (hicprecaching)
            {
                lastfn = fn;  // careful...
                if (!lastpic)
                {
                    lastpic = (coltype *)Bmalloc(siz.x*siz.y*sizeof(coltype));
                    lastsize = siz.x*siz.y;
                }
                else if (lastsize < siz.x*siz.y)
                {
                    Bfree(lastpic);
                    lastpic = (coltype *)Bmalloc(siz.x*siz.y*sizeof(coltype));
                }
                if (lastpic)
                    Bmemcpy(lastpic, pic, siz.x*siz.y*sizeof(coltype));
            }
            else if (lastpic)
            {
                DO_FREE_AND_NULL(lastpic);
                lastfn = NULL;
                lastsize = 0;
            }
        }

        char *cptr = britable[gammabrightness ? 0 : curbrightness];

        polytint_t const & tint = hictinting[pal];
        int32_t r = (glinfo.bgra) ? tint.r : tint.b;
        int32_t g = tint.g;
        int32_t b = (glinfo.bgra) ? tint.b : tint.r;

        char al = 255;
        char onebitalpha = 1;

        for (bssize_t y = 0, j = 0; y < tsiz.y; ++y, j += siz.x)
        {
            coltype tcol, *rpptr = &pic[j];

            for (bssize_t x = 0; x < tsiz.x; ++x)
            {
                tcol.b = cptr[rpptr[x].b];
                tcol.g = cptr[rpptr[x].g];
                tcol.r = cptr[rpptr[x].r];
                al &= tcol.a = rpptr[x].a;
                onebitalpha &= tcol.a == 0 || tcol.a == 255;

                if (effect & HICTINT_GRAYSCALE)
                {
                    tcol.g = tcol.r = tcol.b = (uint8_t) ((tcol.b * GRAYSCALE_COEFF_RED) +
                                                          (tcol.g * GRAYSCALE_COEFF_GREEN) +
                                                          (tcol.r * GRAYSCALE_COEFF_BLUE));
                }

                if (effect & HICTINT_INVERT)
                {
                    tcol.b = 255 - tcol.b;
                    tcol.g = 255 - tcol.g;
                    tcol.r = 255 - tcol.r;
                }

                if (effect & HICTINT_COLORIZE)
                {
                    tcol.b = min((int32_t)((tcol.b) * r) >> 6, 255);
                    tcol.g = min((int32_t)((tcol.g) * g) >> 6, 255);
                    tcol.r = min((int32_t)((tcol.r) * b) >> 6, 255);
                }

                switch (effect & HICTINT_BLENDMASK)
                {
                    case HICTINT_BLEND_SCREEN:
                        tcol.b = 255 - (((255 - tcol.b) * (255 - r)) >> 8);
                        tcol.g = 255 - (((255 - tcol.g) * (255 - g)) >> 8);
                        tcol.r = 255 - (((255 - tcol.r) * (255 - b)) >> 8);
                        break;
                    case HICTINT_BLEND_OVERLAY:
                        tcol.b = tcol.b < 128 ? (tcol.b * r) >> 7 : 255 - (((255 - tcol.b) * (255 - r)) >> 7);
                        tcol.g = tcol.g < 128 ? (tcol.g * g) >> 7 : 255 - (((255 - tcol.g) * (255 - g)) >> 7);
                        tcol.r = tcol.r < 128 ? (tcol.r * b) >> 7 : 255 - (((255 - tcol.r) * (255 - b)) >> 7);
                        break;
                    case HICTINT_BLEND_HARDLIGHT:
                        tcol.b = r < 128 ? (tcol.b * r) >> 7 : 255 - (((255 - tcol.b) * (255 - r)) >> 7);
                        tcol.g = g < 128 ? (tcol.g * g) >> 7 : 255 - (((255 - tcol.g) * (255 - g)) >> 7);
                        tcol.r = b < 128 ? (tcol.r * b) >> 7 : 255 - (((255 - tcol.r) * (255 - b)) >> 7);
                        break;
                }

                rpptr[x] = tcol;
            }
        }

        hasalpha = (al != 255);

        // mdloadskin doesn't duplicate npow2 texture pixels

        if (!glinfo.bgra)
        {
            for (bssize_t j = siz.x*siz.y - 1; j >= 0; j--)
                swapchar(&pic[j].r, &pic[j].b);
        }

        if (pal < (MAXPALOOKUPS - RESERVEDPALS))
            m->usesalpha = hasalpha;
        if ((doalloc&3)==1)
            bglGenTextures(1, texidx);

        bglBindTexture(GL_TEXTURE_2D, *texidx);

        //gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA,xsiz,ysiz,GL_BGRA_EXT,GL_UNSIGNED_BYTE,(char *)fptr);

        int32_t const texfmt = glinfo.bgra ? GL_BGRA : GL_RGBA;

        uploadtexture((doalloc&1), siz, texfmt, pic, tsiz,
                      DAMETH_HI | DAMETH_MASK |
                      TO_DAMETH_NODOWNSIZE(sk->flags) |
                      TO_DAMETH_NOTEXCOMPRESS(sk->flags) |
                      TO_DAMETH_ARTIMMUNITY(sk->flags) |
                      (onebitalpha ? DAMETH_ONEBITALPHA : 0) |
                      (hasalpha ? DAMETH_HASALPHA : 0));

        Bfree(pic);
    }

    if (!m->skinloaded)
    {
        if (siz.x != tsiz.x || siz.y != tsiz.y)
        {
            float fx, fy;
            fx = ((float)tsiz.x)/((float)siz.x);
            fy = ((float)tsiz.y)/((float)siz.y);
            if (m->mdnum == 2)
            {
                int32_t *lptr;
                for (lptr=m->glcmds; (i=*lptr++);)
                    for (i=labs(i); i>0; i--,lptr+=3)
                    {
                        ((float *)lptr)[0] *= fx;
                        ((float *)lptr)[1] *= fy;
                    }
            }
            else if (m->mdnum == 3)
            {
                md3model_t *m3 = (md3model_t *)m;
                md3surf_t *s;
                int32_t surfi;
                for (surfi=0; surfi<m3->head.numsurfs; surfi++)
                {
                    s = &m3->head.surfs[surfi];
                    for (i=s->numverts-1; i>=0; i--)
                    {
                        s->uv[i].u *= fx;
                        s->uv[i].v *= fy;
                    }
                }
            }
        }
        m->skinloaded = 1+number;
    }

    int32_t const filter = sk->flags & HICR_FORCEFILTER ? TEXFILTER_ON : gltexfiltermode;

    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[filter].mag);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[filter].min);
#ifdef USE_GLEXT
    if (glinfo.maxanisotropy > 1.0)
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
#endif
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

#if defined USE_GLEXT && !defined EDUKE32_GLES
    if (!gotcache && glinfo.texcompr && glusetexcache && !(sk->flags & HICR_NOTEXCOMPRESS) &&
        (glusetexcompr == 2 || (glusetexcompr && !(sk->flags & HICR_ARTIMMUNITY))))
        {
            const int32_t nonpow2 = check_nonpow2(siz.x) || check_nonpow2(siz.y);

            // save off the compressed version
            cachead.quality = (sk->flags & (HICR_NODOWNSIZE|HICR_ARTIMMUNITY)) ? 0 : r_downsize;
            cachead.xdim = tsiz.x>>cachead.quality;
            cachead.ydim = tsiz.y>>cachead.quality;

            cachead.flags = nonpow2*CACHEAD_NONPOW2 | (hasalpha ? CACHEAD_HASALPHA : 0) |
                            (sk->flags & (HICR_NODOWNSIZE|HICR_ARTIMMUNITY) ? CACHEAD_NODOWNSIZE : 0);

///            OSD_Printf("Caching \"%s\"\n",fn);
            texcache_writetex_fromdriver(texcacheid, &cachead);

            if (willprint)
            {
                int32_t etime = getticks()-startticks;
                if (etime>=MIN_CACHETIME_PRINT)
                    OSD_Printf("Load skin: p%d-e%d \"%s\"... cached... %d ms\n", pal, hicfxmask(pal), fn, etime);
                willprint = 0;
            }
            else
                OSD_Printf("Cached skin \"%s\"\n", fn);
        }
#endif

    if (willprint)
    {
        int32_t etime = getticks()-startticks;
        if (etime>=MIN_CACHETIME_PRINT)
            OSD_Printf("Load skin: p%d-e%d \"%s\"... %d ms\n", pal, hicfxmask(pal), fn, etime);
    }

    return (*texidx);
}

//Note: even though it says md2model, it works for both md2model&md3model
void updateanimation(md2model_t *m, const uspritetype *tspr, uint8_t lpal)
{
    const mdanim_t *anim;
    int32_t i, j, k;
    int32_t fps;

    int32_t tile, smoothdurationp;
    spritesmooth_t *smooth;
    spriteext_t *sprext;

    if (m->numframes < 2)
    {
        m->interpol = 0;
        return;
    }

    tile = Ptile2tile(tspr->picnum,lpal);
    m->cframe = m->nframe = tile2model[tile].framenum;
#ifdef DEBUGGINGAIDS
    if (m->cframe >= m->numframes)
        OSD_Printf("1: c > n\n");
#endif

    smoothdurationp = (r_animsmoothing && (tile2model[tile].smoothduration != 0));

    smooth = ((unsigned)tspr->owner < MAXSPRITES+MAXUNIQHUDID) ? &spritesmooth[tspr->owner] : NULL;
    sprext = ((unsigned)tspr->owner < MAXSPRITES+MAXUNIQHUDID) ? &spriteext[tspr->owner] : NULL;

    for (anim = m->animations; anim && anim->startframe != m->cframe; anim = anim->next)
    {
        /* do nothing */;
    }

    if (!anim)
    {
        if (!smoothdurationp || ((smooth->mdoldframe == m->cframe) && (smooth->mdcurframe == m->cframe)))
        {
            m->interpol = 0;
            return;
        }

        // assert(smoothdurationp && ((smooth->mdoldframe != m->cframe) || (smooth->mdcurframe != m->cframe)))

        if (smooth->mdoldframe != m->cframe)
        {
            if (smooth->mdsmooth == 0)
            {
                sprext->mdanimtims = mdtims;
                m->interpol = 0;
                smooth->mdsmooth = 1;
                smooth->mdcurframe = m->cframe;
            }

            if (smooth->mdcurframe != m->cframe)
            {
                sprext->mdanimtims = mdtims;
                m->interpol = 0;
                smooth->mdsmooth = 1;
                smooth->mdoldframe = smooth->mdcurframe;
                smooth->mdcurframe = m->cframe;
            }
        }
        else  // if (smooth->mdcurframe != m->cframe)
        {
            sprext->mdanimtims = mdtims;
            m->interpol = 0;
            smooth->mdsmooth = 1;
            smooth->mdoldframe = smooth->mdcurframe;
            smooth->mdcurframe = m->cframe;
        }
    }
    else if (/* anim && */ sprext->mdanimcur != anim->startframe)
    {
        //if (sprext->flags & SPREXT_NOMDANIM) OSD_Printf("SPREXT_NOMDANIM\n");
        //OSD_Printf("smooth launched ! oldanim %i new anim %i\n", sprext->mdanimcur, anim->startframe);
        sprext->mdanimcur = (int16_t)anim->startframe;
        sprext->mdanimtims = mdtims;
        m->interpol = 0;

        if (!smoothdurationp)
        {
            m->cframe = m->nframe = anim->startframe;
#ifdef DEBUGGINGAIDS
            if (m->cframe >= m->numframes)
                OSD_Printf("2: c > n\n");
#endif
            goto prep_return;
        }

        m->nframe = anim->startframe;
        m->cframe = smooth->mdoldframe;
#ifdef DEBUGGINGAIDS
        if (m->cframe >= m->numframes)
            OSD_Printf("3: c > n\n");
#endif
        smooth->mdsmooth = 1;
        goto prep_return;
    }

    fps = smooth->mdsmooth ?
          Blrintf((1.0f / ((float)tile2model[tile].smoothduration * (1.f / (float)UINT16_MAX))) * 66.f) :
          anim->fpssc;

    i = (mdtims - sprext->mdanimtims)*((fps*timerticspersec)/120);

    if (smooth->mdsmooth)
        j = 65536;
    else
        j = ((anim->endframe+1-anim->startframe)<<16);
    // XXX: Just in case you play the game for a VERY long time...
    if (i < 0) { i = 0; sprext->mdanimtims = mdtims; }
    //compare with j*2 instead of j to ensure i stays > j-65536 for MDANIM_ONESHOT
    if (anim && (i >= j+j) && (fps) && !mdpause) //Keep mdanimtims close to mdtims to avoid the use of MOD
        sprext->mdanimtims += j/((fps*timerticspersec)/120);

    k = i;

    if (anim && (anim->flags&MDANIM_ONESHOT))
        { if (i > j-65536) i = j-65536; }
    else { if (i >= j) { i -= j; if (i >= j) i %= j; } }

    if (r_animsmoothing && smooth->mdsmooth)
    {
        m->nframe = anim ? anim->startframe : smooth->mdcurframe;
        m->cframe = smooth->mdoldframe;
#ifdef DEBUGGINGAIDS
        if (m->cframe >= m->numframes)
            OSD_Printf("4: c > n\n");
#endif
        //OSD_Printf("smoothing... cframe %i nframe %i\n", m->cframe, m->nframe);
        if (k > 65535)
        {
            sprext->mdanimtims = mdtims;
            m->interpol = 0;
            smooth->mdsmooth = 0;
            m->cframe = m->nframe; // = anim ? anim->startframe : smooth->mdcurframe;
#ifdef DEBUGGINGAIDS
            if (m->cframe >= m->numframes)
                OSD_Printf("5: c > n\n");
#endif
            smooth->mdoldframe = m->cframe;
            //OSD_Printf("smooth stopped !\n");
            goto prep_return;
        }
    }
    else
    {
        m->cframe = (i>>16)+anim->startframe;
#ifdef DEBUGGINGAIDS
        if (m->cframe >= m->numframes)
            OSD_Printf("6: c > n\n");
#endif
        m->nframe = m->cframe+1;
        if (m->nframe > anim->endframe)  // VERIFY: (!(r_animsmoothing && smooth->mdsmooth)) implies (anim!=NULL) ?
            m->nframe = anim->startframe;

        smooth->mdoldframe = m->cframe;
        //OSD_Printf("not smoothing... cframe %i nframe %i\n", m->cframe, m->nframe);
    }

    m->interpol = ((float)(i&65535))/65536.f;
    //OSD_Printf("interpol %f\n", m->interpol);

prep_return:
    if (m->cframe >= m->numframes)
        m->cframe = 0;
    if (m->nframe >= m->numframes)
        m->nframe = 0;
}

#ifdef USE_GLEXT
// VBO generation and allocation
static void mdloadvbos(md3model_t *m)
{
    int32_t     i;

    m->vbos = (GLuint *)Xmalloc(m->head.numsurfs * sizeof(GLuint));
    bglGenBuffersARB(m->head.numsurfs, m->vbos);

    i = 0;
    while (i < m->head.numsurfs)
    {
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, m->vbos[i]);
        bglBufferDataARB(GL_ARRAY_BUFFER_ARB, m->head.surfs[i].numverts * sizeof(md3uv_t), m->head.surfs[i].uv, GL_STATIC_DRAW_ARB);
        i++;
    }
    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}
#endif

//--------------------------------------- MD2 LIBRARY BEGINS ---------------------------------------
static md2model_t *md2load(int32_t fil, const char *filnam)
{
    md2model_t *m;
    md3model_t *m3;
    md3surf_t *s;
    md2frame_t *f;
    md2head_t head;
    char st[BMAX_PATH];
    int32_t i, j, k;

    int32_t ournumskins, ournumglcmds;

    m = (md2model_t *)Xcalloc(1,sizeof(md2model_t));
    m->mdnum = 2; m->scale = .01f;

    kread(fil,(char *)&head,sizeof(md2head_t));
#if B_BIG_ENDIAN != 0
    head.id = B_LITTLE32(head.id);                 head.vers = B_LITTLE32(head.vers);
    head.skinxsiz = B_LITTLE32(head.skinxsiz);     head.skinysiz = B_LITTLE32(head.skinysiz);
    head.framebytes = B_LITTLE32(head.framebytes); head.numskins = B_LITTLE32(head.numskins);
    head.numverts = B_LITTLE32(head.numverts);     head.numuv = B_LITTLE32(head.numuv);
    head.numtris = B_LITTLE32(head.numtris);       head.numglcmds = B_LITTLE32(head.numglcmds);
    head.numframes = B_LITTLE32(head.numframes);   head.ofsskins = B_LITTLE32(head.ofsskins);
    head.ofsuv = B_LITTLE32(head.ofsuv);           head.ofstris = B_LITTLE32(head.ofstris);
    head.ofsframes = B_LITTLE32(head.ofsframes);   head.ofsglcmds = B_LITTLE32(head.ofsglcmds);
    head.ofseof = B_LITTLE32(head.ofseof);
#endif

    if ((head.id != IDP2_MAGIC) || (head.vers != 8)) { Bfree(m); return 0; } //"IDP2"

    ournumskins = head.numskins ? head.numskins : 1;
    ournumglcmds = head.numglcmds ? head.numglcmds : 1;

    m->numskins = head.numskins;
    m->numframes = head.numframes;
    m->numverts = head.numverts;
    m->numglcmds = head.numglcmds;
    m->framebytes = head.framebytes;

    m->frames = (char *)Xmalloc(m->numframes*m->framebytes);
    m->glcmds = (int32_t *)Xmalloc(ournumglcmds*sizeof(int32_t));
    m->tris = (md2tri_t *)Xmalloc(head.numtris*sizeof(md2tri_t));
    m->uv = (md2uv_t *)Xmalloc(head.numuv*sizeof(md2uv_t));

    klseek(fil,head.ofsframes,SEEK_SET);
    if (kread(fil,(char *)m->frames,m->numframes*m->framebytes) != m->numframes*m->framebytes)
        { Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return 0; }

    if (m->numglcmds > 0)
    {
        klseek(fil,head.ofsglcmds,SEEK_SET);
        if (kread(fil,(char *)m->glcmds,m->numglcmds*sizeof(int32_t)) != (int32_t)(m->numglcmds*sizeof(int32_t)))
            { Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return 0; }
    }

    klseek(fil,head.ofstris,SEEK_SET);
    if (kread(fil,(char *)m->tris,head.numtris*sizeof(md2tri_t)) != (int32_t)(head.numtris*sizeof(md2tri_t)))
        { Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return 0; }

    klseek(fil,head.ofsuv,SEEK_SET);
    if (kread(fil,(char *)m->uv,head.numuv*sizeof(md2uv_t)) != (int32_t)(head.numuv*sizeof(md2uv_t)))
        { Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return 0; }

#if B_BIG_ENDIAN != 0
    {
        char *f = (char *)m->frames;
        int32_t *l,j;
        md2frame_t *fr;

        for (i = m->numframes-1; i>=0; i--)
        {
            fr = (md2frame_t *)f;
            l = (int32_t *)&fr->mul;
            for (j=5; j>=0; j--) l[j] = B_LITTLE32(l[j]);
            f += m->framebytes;
        }

        for (i = m->numglcmds-1; i>=0; i--)
        {
            m->glcmds[i] = B_LITTLE32(m->glcmds[i]);
        }
        for (i = head.numtris-1; i>=0; i--)
        {
            m->tris[i].v[0] = B_LITTLE16(m->tris[i].v[0]);
            m->tris[i].v[1] = B_LITTLE16(m->tris[i].v[1]);
            m->tris[i].v[2] = B_LITTLE16(m->tris[i].v[2]);
            m->tris[i].u[0] = B_LITTLE16(m->tris[i].u[0]);
            m->tris[i].u[1] = B_LITTLE16(m->tris[i].u[1]);
            m->tris[i].u[2] = B_LITTLE16(m->tris[i].u[2]);
        }
        for (i = head.numuv-1; i>=0; i--)
        {
            m->uv[i].u = B_LITTLE16(m->uv[i].u);
            m->uv[i].v = B_LITTLE16(m->uv[i].v);
        }
    }
#endif

    Bstrcpy(st,filnam);
    for (i=strlen(st)-1; i>0; i--)
        if ((st[i] == '/') || (st[i] == '\\')) { i++; break; }
    if (i<0) i=0;
    st[i] = 0;
    m->basepath = (char *)Xmalloc(i+1);
    Bstrcpy(m->basepath, st);

    m->skinfn = (char *)Xmalloc(ournumskins*64);
    if (m->numskins > 0)
    {
        klseek(fil,head.ofsskins,SEEK_SET);
        if (kread(fil,m->skinfn,64*m->numskins) != 64*m->numskins)
            { Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return 0; }
    }

    m->texid = (GLuint *)Xcalloc(ournumskins, sizeof(GLuint) * HICTINT_MEMORY_COMBINATIONS);

    maxmodelverts = max(maxmodelverts, m->numverts);
    maxmodeltris = max(maxmodeltris, head.numtris);

    //return m;

    // the MD2 is now loaded internally - let's begin the MD3 conversion process
    //OSD_Printf("Beginning md3 conversion.\n");
    m3 = (md3model_t *)Xcalloc(1, sizeof(md3model_t));
    m3->mdnum = 3; m3->texid = 0; m3->scale = m->scale;
    m3->head.id = IDP3_MAGIC; m3->head.vers = 15;

    m3->head.flags = 0;

    m3->head.numframes = m->numframes;
    m3->head.numtags = 0; m3->head.numsurfs = 1;
    m3->head.numskins = 0;

    m3->numskins = m3->head.numskins;
    m3->numframes = m3->head.numframes;

    m3->head.frames = (md3frame_t *)Xcalloc(m3->head.numframes, sizeof(md3frame_t));
    m3->muladdframes = (vec3f_t *)Xcalloc(m->numframes * 2, sizeof(vec3f_t));

    f = (md2frame_t *)(m->frames);

    // frames converting
    i = 0;
    while (i < m->numframes)
    {
        f = (md2frame_t *)&m->frames[i*m->framebytes];
        Bstrcpy(m3->head.frames[i].nam, f->name);
        //OSD_Printf("Copied frame %s.\n", m3->head.frames[i].nam);
        m3->muladdframes[i*2] = f->mul;
        m3->muladdframes[i*2+1] = f->add;
        i++;
    }

    m3->head.tags = NULL;

    m3->head.surfs = (md3surf_t *)Xcalloc(1, sizeof(md3surf_t));
    s = m3->head.surfs;

    // model converting
    s->id = IDP3_MAGIC; s->flags = 0;
    s->numframes = m->numframes; s->numshaders = 0;
    s->numtris = head.numtris;
    s->numverts = head.numtris * 3; // oh man talk about memory effectiveness :((((
    // MD2 is actually more accurate than MD3 in term of uv-mapping, because each triangle has a triangle counterpart on the UV-map.
    // In MD3, each vertex unique UV coordinates, meaning that you have to duplicate vertices if you need non-seamless UV-mapping.

    maxmodelverts = max(maxmodelverts, s->numverts);

    Bstrcpy(s->nam, "Dummy surface from MD2");

    s->shaders = NULL;

    s->tris = (md3tri_t *)Xcalloc(head.numtris, sizeof(md3tri_t));
    s->uv = (md3uv_t *)Xcalloc(s->numverts, sizeof(md3uv_t));
    s->xyzn = (md3xyzn_t *)Xcalloc(s->numverts * m->numframes, sizeof(md3xyzn_t));

    //memoryusage += (s->numverts * m->numframes * sizeof(md3xyzn_t));
    //OSD_Printf("Current model geometry memory usage : %i.\n", memoryusage);

    //OSD_Printf("Number of frames : %i\n", m->numframes);
    //OSD_Printf("Number of triangles : %i\n", head.numtris);
    //OSD_Printf("Number of vertices : %i\n", s->numverts);

    // triangle converting
    i = 0;
    while (i < head.numtris)
    {
        j = 0;
        //OSD_Printf("Triangle : %i\n", i);
        while (j < 3)
        {
            // triangle vertex indexes
            s->tris[i].i[j] = i*3 + j;

            // uv coords
            s->uv[i*3+j].u = (float)(m->uv[m->tris[i].u[j]].u) / (float)(head.skinxsiz);
            s->uv[i*3+j].v = (float)(m->uv[m->tris[i].u[j]].v) / (float)(head.skinysiz);

            // vertices for each frame
            k = 0;
            while (k < m->numframes)
            {
                f = (md2frame_t *)&m->frames[k*m->framebytes];
                s->xyzn[(k*s->numverts) + (i*3) + j].x = (int16_t) (((f->verts[m->tris[i].v[j]].v[0] * f->mul.x) + f->add.x) * 64.f);
                s->xyzn[(k*s->numverts) + (i*3) + j].y = (int16_t) (((f->verts[m->tris[i].v[j]].v[1] * f->mul.y) + f->add.y) * 64.f);
                s->xyzn[(k*s->numverts) + (i*3) + j].z = (int16_t) (((f->verts[m->tris[i].v[j]].v[2] * f->mul.z) + f->add.z) * 64.f);

                k++;
            }
            j++;
        }
        //OSD_Printf("End triangle.\n");
        i++;
    }
    //OSD_Printf("Finished md3 conversion.\n");

    {
        mdskinmap_t *sk;

        sk = (mdskinmap_t *)Xcalloc(1,sizeof(mdskinmap_t));
        sk->palette = 0;
        sk->skinnum = 0;
        sk->surfnum = 0;

        if (m->numskins > 0)
        {
            sk->fn = (char *)Xmalloc(strlen(m->basepath)+strlen(m->skinfn)+1);
            Bstrcpy(sk->fn, m->basepath);
            Bstrcat(sk->fn, m->skinfn);
        }
        m3->skinmap = sk;
    }

    m3->indexes = (uint16_t *)Xmalloc(sizeof(uint16_t) * s->numtris);
    m3->vindexes = (uint16_t *)Xmalloc(sizeof(uint16_t) * s->numtris * 3);
    m3->maxdepths = (float *)Xmalloc(sizeof(float) * s->numtris);

    m3->vbos = NULL;

    // die MD2 ! DIE !
    Bfree(m->texid); Bfree(m->skinfn); Bfree(m->basepath); Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m);

    return ((md2model_t *)m3);
}
//---------------------------------------- MD2 LIBRARY ENDS ----------------------------------------

// DICHOTOMIC RECURSIVE SORTING - USED BY MD3DRAW
int32_t partition(uint16_t *indexes, float *depths, int32_t f, int32_t l)
{
    int32_t up = f, down = l;
    float piv = depths[f];
    uint16_t piv2 = indexes[f];
    do
    {
        while ((depths[up] <= piv) && (up < l))
            up++;
        while ((depths[down] > piv)  && (down > f))
            down--;
        if (up < down)
        {
            swapfloat(&depths[up], &depths[down]);
            swapshort(&indexes[up], &indexes[down]);
        }
    }
    while (down > up);
    depths[f] = depths[down];
    depths[down] = piv;
    indexes[f] = indexes[down];
    indexes[down] = piv2;
    return down;
}

static inline void quicksort(uint16_t *indexes, float *depths, int32_t first, int32_t last)
{
    int32_t pivIndex;
    if (first >= last) return;
    pivIndex = partition(indexes, depths, first, last);
    if (first < (pivIndex-1)) quicksort(indexes, depths, first, (pivIndex-1));
    if ((pivIndex+1) >= last) return;
    quicksort(indexes, depths, (pivIndex+1), last);
}
// END OF QUICKSORT LIB

//--------------------------------------- MD3 LIBRARY BEGINS ---------------------------------------

static md3model_t *md3load(int32_t fil)
{
    int32_t i, surfi, ofsurf, offs[4], leng[4];
    int32_t maxtrispersurf;
    md3model_t *m;
    md3surf_t *s;

    m = (md3model_t *)Xcalloc(1,sizeof(md3model_t));
    m->mdnum = 3; m->texid = 0; m->scale = .01f;

    m->muladdframes = NULL;

    kread(fil,&m->head,SIZEOF_MD3HEAD_T);

#if B_BIG_ENDIAN != 0
    m->head.id = B_LITTLE32(m->head.id);             m->head.vers = B_LITTLE32(m->head.vers);
    m->head.flags = B_LITTLE32(m->head.flags);       m->head.numframes = B_LITTLE32(m->head.numframes);
    m->head.numtags = B_LITTLE32(m->head.numtags);   m->head.numsurfs = B_LITTLE32(m->head.numsurfs);
    m->head.numskins = B_LITTLE32(m->head.numskins); m->head.ofsframes = B_LITTLE32(m->head.ofsframes);
    m->head.ofstags = B_LITTLE32(m->head.ofstags); m->head.ofssurfs = B_LITTLE32(m->head.ofssurfs);
    m->head.eof = B_LITTLE32(m->head.eof);
#endif

    if ((m->head.id != IDP3_MAGIC) && (m->head.vers != 15)) { Bfree(m); return 0; } //"IDP3"

    m->numskins = m->head.numskins; //<- dead code?
    m->numframes = m->head.numframes;

    ofsurf = m->head.ofssurfs;

    klseek(fil,m->head.ofsframes,SEEK_SET); i = m->head.numframes*sizeof(md3frame_t);
    m->head.frames = (md3frame_t *)Xmalloc(i);
    kread(fil,m->head.frames,i);

    if (m->head.numtags == 0) m->head.tags = NULL;
    else
    {
        klseek(fil,m->head.ofstags,SEEK_SET); i = m->head.numtags*sizeof(md3tag_t);
        m->head.tags = (md3tag_t *)Xmalloc(i);
        kread(fil,m->head.tags,i);
    }

    klseek(fil,m->head.ofssurfs,SEEK_SET);
    m->head.surfs = (md3surf_t *)Xcalloc(m->head.numsurfs, sizeof(md3surf_t));
    // NOTE: We assume that NULL is represented by all-zeros.
    // surfs[0].geometry is for POLYMER_MD_PROCESS_CHECK (else: crashes).
    // surfs[i].geometry is for FREE_SURFS_GEOMETRY.
    Bassert(m->head.surfs[0].geometry == NULL);

#if B_BIG_ENDIAN != 0
    {
        int32_t j, *l;

        for (i = m->head.numframes-1; i>=0; i--)
        {
            l = (int32_t *)&m->head.frames[i].min;
            for (j=3+3+3+1-1; j>=0; j--) l[j] = B_LITTLE32(l[j]);
        }

        for (i = m->head.numtags-1; i>=0; i--)
        {
            l = (int32_t *)&m->head.tags[i].p;
            for (j=3+3+3+3-1; j>=0; j--) l[j] = B_LITTLE32(l[j]);
        }
    }
#endif

    maxtrispersurf = 0;

    for (surfi=0; surfi<m->head.numsurfs; surfi++)
    {
        s = &m->head.surfs[surfi];
        klseek(fil,ofsurf,SEEK_SET); kread(fil,s,SIZEOF_MD3SURF_T);

#if B_BIG_ENDIAN != 0
        {
            int32_t j, *l;
            s->id = B_LITTLE32(s->id);
            l =	(int32_t *)&s->flags;
            for	(j=1+1+1+1+1+1+1+1+1+1-1; j>=0; j--) l[j] = B_LITTLE32(l[j]);
        }
#endif

        offs[0] = ofsurf+s->ofstris;
        offs[1] = ofsurf+s->ofsshaders;
        offs[2] = ofsurf+s->ofsuv;
        offs[3] = ofsurf+s->ofsxyzn;

        leng[0] = s->numtris*sizeof(md3tri_t);
        leng[1] = s->numshaders*sizeof(md3shader_t);
        leng[2] = s->numverts*sizeof(md3uv_t);
        leng[3] = s->numframes*s->numverts*sizeof(md3xyzn_t);

        //memoryusage += (s->numverts * s->numframes * sizeof(md3xyzn_t));
        //OSD_Printf("Current model geometry memory usage : %i.\n", memoryusage);

        s->tris = (md3tri_t *)Xmalloc((leng[0] + leng[1]) + (leng[2] + leng[3]));

        s->shaders = (md3shader_t *)(((intptr_t)s->tris)+leng[0]);
        s->uv      = (md3uv_t *)(((intptr_t)s->shaders)+leng[1]);
        s->xyzn    = (md3xyzn_t *)(((intptr_t)s->uv)+leng[2]);

        klseek(fil,offs[0],SEEK_SET); kread(fil,s->tris   ,leng[0]);
        klseek(fil,offs[1],SEEK_SET); kread(fil,s->shaders,leng[1]);
        klseek(fil,offs[2],SEEK_SET); kread(fil,s->uv     ,leng[2]);
        klseek(fil,offs[3],SEEK_SET); kread(fil,s->xyzn   ,leng[3]);

#if B_BIG_ENDIAN != 0
        {
            int32_t j, *l;

            for (i=s->numtris-1; i>=0; i--)
            {
                for (j=2; j>=0; j--) s->tris[i].i[j] = B_LITTLE32(s->tris[i].i[j]);
            }
            for (i=s->numshaders-1; i>=0; i--)
            {
                s->shaders[i].i = B_LITTLE32(s->shaders[i].i);
            }
            for (i=s->numverts-1; i>=0; i--)
            {
                l = (int32_t *)&s->uv[i].u;
                l[0] = B_LITTLE32(l[0]);
                l[1] = B_LITTLE32(l[1]);
            }
            for (i=s->numframes*s->numverts-1; i>=0; i--)
            {
                s->xyzn[i].x = (int16_t)B_LITTLE16((uint16_t)s->xyzn[i].x);
                s->xyzn[i].y = (int16_t)B_LITTLE16((uint16_t)s->xyzn[i].y);
                s->xyzn[i].z = (int16_t)B_LITTLE16((uint16_t)s->xyzn[i].z);
            }
        }
#endif
        maxmodelverts = max(maxmodelverts, s->numverts);
        maxmodeltris = max(maxmodeltris, s->numtris);
        maxtrispersurf = max(maxtrispersurf, s->numtris);
        ofsurf += s->ofsend;
    }

    m->indexes = (uint16_t *)Xmalloc(sizeof(uint16_t) * maxtrispersurf);
    m->vindexes = (uint16_t *)Xmalloc(sizeof(uint16_t) * maxtrispersurf * 3);
    m->maxdepths = (float *)Xmalloc(sizeof(float) * maxtrispersurf);

    m->vbos = NULL;

    return m;
}

#ifdef POLYMER
static inline void  invertmatrix(float *m, float *out)
{
    float det;

    det  = (m[0] * (m[4]*m[8] - m[5] * m[7]))
         - (m[1] * (m[3]*m[8] - m[5] * m[6]))
         + (m[2] * (m[3]*m[7] - m[4] * m[6]));

    if (det == 0.0f)
    {
        Bmemset(out, 0, sizeof(float) * 9);
        out[8] = out[4] = out[0] = 1.f;
        return;
    }

    det = 1.0f / det;

    out[0] = det * (m[4] * m[8] - m[5] * m[7]);
    out[1] = det * (m[2] * m[7] - m[1] * m[8]);
    out[2] = det * (m[1] * m[5] - m[2] * m[4]);
    out[3] = det * (m[5] * m[6] - m[3] * m[8]);
    out[4] = det * (m[0] * m[8] - m[2] * m[6]);
    out[5] = det * (m[2] * m[3] - m[0] * m[5]);
    out[6] = det * (m[3] * m[7] - m[1] * m[6]);
    out[7] = det * (m[1] * m[6] - m[0] * m[7]);
    out[8] = det * (m[0] * m[4] - m[1] * m[3]);
}

static inline void  normalize(float *vec)
{
    float norm;

    if ((norm = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]) == 0.f)
        return;

    norm = polymost_invsqrt_approximation(norm);
    vec[0] *= norm;
    vec[1] *= norm;
    vec[2] *= norm;
}
#endif

static void      md3postload_common(md3model_t *m)
{
    int         framei, surfi, verti;
    md3frame_t  *frame;
    md3xyzn_t   *frameverts;
    float       dist, vec1[3];

    // apparently we can't trust loaded models bounding box/sphere information,
    // so let's compute it ourselves

    framei = 0;

    while (framei < m->head.numframes)
    {
        frame = &m->head.frames[framei];

        Bmemset(&frame->min, 0, sizeof(vec3f_t));
        Bmemset(&frame->max, 0, sizeof(vec3f_t));

        frame->r        = 0.0f;

        surfi = 0;
        while (surfi < m->head.numsurfs)
        {
            frameverts = &m->head.surfs[surfi].xyzn[framei * m->head.surfs[surfi].numverts];

            verti = 0;
            while (verti < m->head.surfs[surfi].numverts)
            {
                if (!verti && !surfi)
                {
                    md3xyzn_t const & framevert = frameverts[0];

                    frame->min.x    = framevert.x;
                    frame->min.y    = framevert.y;
                    frame->min.z    = framevert.z;

                    frame->max      = frame->min;
                }
                else
                {
                    md3xyzn_t const & framevert = frameverts[verti];

                    if (frame->min.x > framevert.x)
                        frame->min.x = framevert.x;
                    if (frame->max.x < framevert.x)
                        frame->max.x = framevert.x;

                    if (frame->min.y > framevert.y)
                        frame->min.y = framevert.y;
                    if (frame->max.y < framevert.y)
                        frame->max.y = framevert.y;

                    if (frame->min.z > framevert.z)
                        frame->min.z = framevert.z;
                    if (frame->max.z < framevert.z)
                        frame->max.z = framevert.z;
                }

                ++verti;
            }

            ++surfi;
        }

        frame->cen.x = (frame->min.x + frame->max.x) * .5f;
        frame->cen.y = (frame->min.y + frame->max.y) * .5f;
        frame->cen.z = (frame->min.z + frame->max.z) * .5f;

        surfi = 0;
        while (surfi < m->head.numsurfs)
        {
            md3surf_t const & surf = m->head.surfs[surfi];

            frameverts = &surf.xyzn[framei * surf.numverts];

            verti = 0;
            while (verti < surf.numverts)
            {
                md3xyzn_t const & framevert = frameverts[verti];

                vec1[0] = framevert.x - frame->cen.x;
                vec1[1] = framevert.y - frame->cen.y;
                vec1[2] = framevert.z - frame->cen.z;

                dist = vec1[0] * vec1[0] + vec1[1] * vec1[1] + vec1[2] * vec1[2];

                if (dist > frame->r)
                    frame->r = dist;

                ++verti;
            }

            ++surfi;
        }

        frame->r = Bsqrtf(frame->r);

        ++framei;
    }
}

#ifdef POLYMER
// pre-check success of conversion since it must not fail later.
// keep in sync with md3postload_polymer!
static int md3postload_polymer_check(md3model_t *m)
{
    ssize_t surfi, trii;
    md3surf_t   *s;

    surfi = 0;
    while (surfi < m->head.numsurfs)
    {
        s = &m->head.surfs[surfi];

        uint32_t const numverts = s->numverts;

        trii = 0;
        while (trii < s->numtris)
        {
            uint32_t const * const u = (uint32_t const *)s->tris[trii].i;

            // let the vertices know they're being referenced by a triangle
            if (u[0] >= numverts || u[1] >= numverts || u[2] >= numverts)
            {
                // corrupt model
                OSD_Printf("%s: Triangle index out of bounds!\n", m->head.nam);
                return 1;
            }

            ++trii;
        }

        ++surfi;
    }

    return 0;
}

// Precalculated cos/sin arrays.
static float g_mdcos[256], g_mdsin[256];
static int32_t mdtrig_init = 0;

static void init_mdtrig_arrays(void)
{
    int32_t i;

    for (i=0; i<256; i++)
    {
        float ang = i * (2.f * fPI) * (1.f/255.f);
        g_mdcos[i] = cosf(ang);
        g_mdsin[i] = sinf(ang);
    }

    mdtrig_init = 1;
}
#endif

int      md3postload_polymer(md3model_t *m)
{
#ifdef POLYMER
    int         framei, surfi, verti, trii;
    float       vec1[5], vec2[5], mat[9], r;

    // POLYMER_MD_PROCESS_CHECK
    if (m->head.surfs[0].geometry)
        return -1;  // already postprocessed

    if (!mdtrig_init)
        init_mdtrig_arrays();

    // let's also repack the geometry to more usable formats

    surfi = 0;
    while (surfi < m->head.numsurfs)
    {
        handleevents();

        md3surf_t *const s = &m->head.surfs[surfi];
#ifdef DEBUG_MODEL_MEM
        i = (m->head.numframes * s->numverts * sizeof(float) * 15);
        if (i > 1<<20)
            initprintf("size %d (%d fr, %d v): md %s surf %d/%d\n", i, m->head.numframes, s->numverts,
                       m->head.nam, surfi, m->head.numsurfs);
#endif
        s->geometry = (float *)Xcalloc(m->head.numframes * s->numverts * 15, sizeof(float));

        if (s->numverts > tribufverts)
        {
            tribuf = (int32_t *) Xrealloc(tribuf, s->numverts * sizeof(int32_t));
            tribufverts = s->numverts;
        }

        Bmemset(tribuf, 0, s->numverts * sizeof(int32_t));

        verti = 0;
        while (verti < (m->head.numframes * s->numverts))
        {
            md3xyzn_t const & xyzn = s->xyzn[verti];

            // normal extraction from packed spherical coordinates
            // FIXME: swapping lat and lng because of npherno's compiler
            uint8_t lat = xyzn.nlng;
            uint8_t lng = xyzn.nlat;
            size_t verti15 = (size_t)verti * 15;

            s->geometry[verti15 + 0] = xyzn.x;
            s->geometry[verti15 + 1] = xyzn.y;
            s->geometry[verti15 + 2] = xyzn.z;

            s->geometry[verti15 + 3] = g_mdcos[lat] * g_mdsin[lng];
            s->geometry[verti15 + 4] = g_mdsin[lat] * g_mdsin[lng];
            s->geometry[verti15 + 5] = g_mdcos[lng];

            ++verti;
        }

        uint32_t numverts = s->numverts;

        trii = 0;
        while (trii < s->numtris)
        {
            int32_t const * const i = s->tris[trii].i;
            uint32_t const * const u = (uint32_t const *)i;

            // let the vertices know they're being referenced by a triangle
            if (u[0] >= numverts ||u[1] >= numverts || u[2] >= numverts)
            {
                // corrupt model
                return 0;
            }
            tribuf[u[0]]++;
            tribuf[u[1]]++;
            tribuf[u[2]]++;

            uint32_t const tris15[] = { u[0] * 15, u[1] * 15, u[2] * 15 };


            framei = 0;
            while (framei < m->head.numframes)
            {
                const uint32_t verti15 = framei * s->numverts * 15;

                vec1[0] = s->geometry[verti15 + tris15[1]]     - s->geometry[verti15 + tris15[0]];
                vec1[1] = s->geometry[verti15 + tris15[1] + 1] - s->geometry[verti15 + tris15[0] + 1];
                vec1[2] = s->geometry[verti15 + tris15[1] + 2] - s->geometry[verti15 + tris15[0] + 2];
                vec1[3] = s->uv[u[1]].u - s->uv[u[0]].u;
                vec1[4] = s->uv[u[1]].v - s->uv[u[0]].v;

                vec2[0] = s->geometry[verti15 + tris15[2]]     - s->geometry[verti15 + tris15[1]];
                vec2[1] = s->geometry[verti15 + tris15[2] + 1] - s->geometry[verti15 + tris15[1] + 1];
                vec2[2] = s->geometry[verti15 + tris15[2] + 2] - s->geometry[verti15 + tris15[1] + 2];
                vec2[3] = s->uv[u[2]].u - s->uv[u[1]].u;
                vec2[4] = s->uv[u[2]].v - s->uv[u[1]].v;

                r = (vec1[3] * vec2[4] - vec2[3] * vec1[4]);
                if (r != 0.0f)
                {
                    r = 1.f/r;

                    // tangent
                    mat[0] = (vec2[4] * vec1[0] - vec1[4] * vec2[0]) * r;
                    mat[1] = (vec2[4] * vec1[1] - vec1[4] * vec2[1]) * r;
                    mat[2] = (vec2[4] * vec1[2] - vec1[4] * vec2[2]) * r;

                    normalize(&mat[0]);

                    // bitangent
                    mat[3] = (vec1[3] * vec2[0] - vec2[3] * vec1[0]) * r;
                    mat[4] = (vec1[3] * vec2[1] - vec2[3] * vec1[1]) * r;
                    mat[5] = (vec1[3] * vec2[2] - vec2[3] * vec1[2]) * r;

                    normalize(&mat[3]);
                }
                else
                    Bmemset(mat, 0, sizeof(float) * 6);

                // T and B are shared for the three vertices in that triangle
                size_t const offs = (framei * numverts * 15) + 6;
                size_t j = 0;
                do
                {
                    size_t const offsi = offs + j;
                    s->geometry[offsi + tris15[0]] += mat[j];
                    s->geometry[offsi + tris15[1]] += mat[j];
                    s->geometry[offsi + tris15[2]] += mat[j];
                }
                while (++j < 6);

                ++framei;
            }

            ++trii;
        }

        // now that we accumulated the TBNs, average and invert them for each vertex
        int verti_end = m->head.numframes * s->numverts;

        verti = 0;
        while (verti < verti_end)
        {
            const int32_t curnumtris = tribuf[verti % s->numverts];
            uint32_t const verti15 = verti * 15;

            if (curnumtris > 0)
            {
                const float rfcurnumtris = 1.f/(float)curnumtris;
                size_t i = 6;
                do
                    s->geometry[i + verti15] *= rfcurnumtris;
                while (++i < 12);
            }
#ifdef DEBUG_MODEL_MEM
            else if (verti == verti%s->numverts)
            {
                OSD_Printf("%s: vert %d is unused\n", m->head.nam, verti);
            }
#endif
            // copy N over
            Bmemcpy(&s->geometry[verti15 + 12], &s->geometry[verti15 + 3], sizeof(float) * 3);
            invertmatrix(&s->geometry[verti15 + 6], mat);
            Bmemcpy(&s->geometry[verti15 + 6], mat, sizeof(float) * 9);

            ++verti;
        }

        ++surfi;
    }

#else
    UNREFERENCED_PARAMETER(m);
#endif

    return 1;
}


void md3_vox_calcmat_common(const uspritetype *tspr, const vec3f_t *a0, float f, float mat[16])
{
    float g;
    float k0, k1, k2, k3, k4, k5, k6, k7;

    k0 = ((float)(tspr->x-globalposx))*f*(1.f/1024.f);
    k1 = ((float)(tspr->y-globalposy))*f*(1.f/1024.f);
    f = gcosang2*gshang;
    g = gsinang2*gshang;
    k4 = (float)sintable[(tspr->ang+spriteext[tspr->owner].angoff+1024)&2047] * (1.f/16384.f);
    k5 = (float)sintable[(tspr->ang+spriteext[tspr->owner].angoff+ 512)&2047] * (1.f/16384.f);
    k2 = k0*(1-k4)+k1*k5;
    k3 = k1*(1-k4)-k0*k5;
    k6 = f*gstang - gsinang*gctang; k7 = g*gstang + gcosang*gctang;
    mat[0] = k4*k6 + k5*k7; mat[4] = gchang*gstang; mat[ 8] = k4*k7 - k5*k6; mat[12] = k2*k6 + k3*k7;
    k6 = f*gctang + gsinang*gstang; k7 = g*gctang - gcosang*gstang;
    mat[1] = k4*k6 + k5*k7; mat[5] = gchang*gctang; mat[ 9] = k4*k7 - k5*k6; mat[13] = k2*k6 + k3*k7;
    k6 =           gcosang2*gchang; k7 =           gsinang2*gchang;
    mat[2] = k4*k6 + k5*k7; mat[6] =-gshang;        mat[10] = k4*k7 - k5*k6; mat[14] = k2*k6 + k3*k7;

    mat[12] = (mat[12] + a0->y*mat[0]) + (a0->z*mat[4] + a0->x*mat[ 8]);
    mat[13] = (mat[13] + a0->y*mat[1]) + (a0->z*mat[5] + a0->x*mat[ 9]);
    mat[14] = (mat[14] + a0->y*mat[2]) + (a0->z*mat[6] + a0->x*mat[10]);
}

static void md3draw_handle_triangles(const md3surf_t *s, uint16_t *indexhandle,
                                            int32_t texunits, const md3model_t *M)
{
    int32_t i;

    if (r_vertexarrays)
    {
        int32_t k = 0;

        if (M == NULL)
        {
            for (i=s->numtris-1; i>=0; i--, k+=3)
            {
                indexhandle[k]   = s->tris[i].i[0];
                indexhandle[k+1] = s->tris[i].i[1];
                indexhandle[k+2] = s->tris[i].i[2];
            }
            return;
        }


        for (i=s->numtris-1; i>=0; i--, k+=3)
        {
            uint16_t tri = M->indexes[i];

            indexhandle[k]   = s->tris[tri].i[0];
            indexhandle[k+1] = s->tris[tri].i[1];
            indexhandle[k+2] = s->tris[tri].i[2];
        }
        return;
    }

    bglBegin(GL_TRIANGLES);
    for (i=s->numtris-1; i>=0; i--)
    {
        uint16_t tri = M ? M->indexes[i] : i;
        int32_t j;

        for (j=0; j<3; j++)
        {
            int32_t k = s->tris[tri].i[j];

#ifdef USE_GLEXT
            if (texunits > GL_TEXTURE0_ARB)
            {
                int32_t l = GL_TEXTURE0_ARB;
                while (l <= texunits)
                    bglMultiTexCoord2fARB(l++, s->uv[k].u, s->uv[k].v);
            }
            else
#endif
                bglTexCoord2f(s->uv[k].u, s->uv[k].v);

            bglVertex3fv((float *) &vertlist[k]);
        }
    }
    bglEnd();

#ifndef USE_GLEXT
    UNREFERENCED_PARAMETER(texunits);
#endif
}

static int32_t polymost_md3draw(md3model_t *m, const uspritetype *tspr)
{
    vec3f_t m0, m1, a0;
    md3xyzn_t *v0, *v1;
    int32_t i, surfi;
    float f, g, k0, k1, k2=0, k3=0, mat[16];  // inits: compiler-happy
    GLfloat pc[4];
    int32_t texunits = GL_TEXTURE0_ARB;

    const int32_t owner = tspr->owner;
    // PK: XXX: These owner bound checks are redundant because sext is
    // dereferenced unconditionally below anyway.
    const spriteext_t *const sext = ((unsigned)owner < MAXSPRITES+MAXUNIQHUDID) ? &spriteext[owner] : NULL;
    const uint8_t lpal = ((unsigned)owner < MAXSPRITES) ? sprite[tspr->owner].pal : tspr->pal;
    const int32_t sizyrep = tilesiz[tspr->picnum].y*tspr->yrepeat;

#ifdef USE_GLEXT
    if (r_vbos && (m->vbos == NULL))
        mdloadvbos(m);
#endif

    //    if ((tspr->cstat&48) == 32) return 0;

    updateanimation((md2model_t *)m, tspr, lpal);

    //create current&next frame's vertex list from whole list

    f = m->interpol; g = 1.f - f;

    if (m->interpol < 0.f || m->interpol > 1.f ||
        (unsigned)m->cframe >= (unsigned)m->numframes ||
            (unsigned)m->nframe >= (unsigned)m->numframes)
    {
#ifdef DEBUGGINGAIDS
        OSD_Printf("%s: mdframe oob: c:%d n:%d total:%d interpol:%.02f\n",
                   m->head.nam, m->cframe, m->nframe, m->numframes, m->interpol);
#endif

        m->interpol = fclamp(m->interpol, 0.f, 1.f);
        m->cframe = clamp(m->cframe, 0, m->numframes-1);
        m->nframe = clamp(m->nframe, 0, m->numframes-1);
    }

    m0.z = m0.y = m0.x = g *= m->scale * (1.f/64.f);
    m1.z = m1.y = m1.x = f *= m->scale * (1.f/64.f);

    a0.x = a0.y = 0;
    a0.z = m->zadd * m->scale;

    // Parkar: Moved up to be able to use k0 for the y-flipping code
    k0 = (float)tspr->z;
    if ((globalorientation&128) && !((globalorientation&48)==32))
        k0 += (float)(sizyrep<<1);

    // Parkar: Changed to use the same method as centeroriented sprites
    if (globalorientation&8) //y-flipping
    {
        m0.z = -m0.z; m1.z = -m1.z; a0.z = -a0.z;
        k0 -= (float)(sizyrep<<2);
    }
    if (globalorientation&4) { m0.y = -m0.y; m1.y = -m1.y; a0.y = -a0.y; } //x-flipping

    // yoffset differs from zadd in that it does not follow cstat&8 y-flipping
    a0.z += m->yoffset*m->scale;

    f = ((float)tspr->xrepeat) * (1.f/64.f) * m->bscale;
    m0.x *= f; m0.y *= -f;
    m1.x *= f; m1.y *= -f;
    a0.x *= f; a0.y *= -f;
    f = ((float)tspr->yrepeat) * (1.f/64.f) * m->bscale;
    m0.z *= f; m1.z *= f; a0.z *= f;

    // floor aligned
    k1 = (float)tspr->y;
    if ((globalorientation&48)==32)
    {
        m0.z = -m0.z; m1.z = -m1.z; a0.z = -a0.z;
        m0.y = -m0.y; m1.y = -m1.y; a0.y = -a0.y;
        f = a0.x; a0.x = a0.z; a0.z = f;
        k1 += (float)(sizyrep>>3);
    }

    // Note: These SCREEN_FACTORS will be neutralized in axes offset
    // calculations below again, but are needed for the base offsets.
    f = (65536.f*512.f)/(fxdimen*fviewingrange);
    g = 32.f/(fxdimen*gxyaspect);
    m0.y *= f; m1.y *= f; a0.y = (((float)(tspr->x-globalposx))*  (1.f/1024.f) + a0.y)*f;
    m0.x *=-f; m1.x *=-f; a0.x = ((k1     -fglobalposy) * -(1.f/1024.f) + a0.x)*-f;
    m0.z *= g; m1.z *= g; a0.z = ((k0     -fglobalposz) * -(1.f/16384.f) + a0.z)*g;

    md3_vox_calcmat_common(tspr, &a0, f, mat);

    // floor aligned
    if ((globalorientation&48)==32)
    {
        f = mat[4]; mat[4] = mat[8]*16.f; mat[8] = -f*(1.f/16.f);
        f = mat[5]; mat[5] = mat[9]*16.f; mat[9] = -f*(1.f/16.f);
        f = mat[6]; mat[6] = mat[10]*16.f; mat[10] = -f*(1.f/16.f);
    }

    //Mirrors
    if (grhalfxdown10x < 0) { mat[0] = -mat[0]; mat[4] = -mat[4]; mat[8] = -mat[8]; mat[12] = -mat[12]; }

    //------------
    // TSPR_EXTRA_MDHACK is an ugly hack in game.c:G_DoSpriteAnimations() telling md2sprite
    // to use Z-buffer hacks to hide overdraw problems with the flat-tsprite-on-floor shadows,
    // also disabling detail, glow, normal, and specular maps.

    if (tspr->extra&TSPR_EXTRA_MDHACK)
    {
#ifdef __arm__ // GL ES has a glDepthRangef and the loss of precision is OK there
        float f = (float) (tspr->owner + 1) * (FLT_EPSILON * 8.0);
        if (f != 0.0) f *= 1.f/(float) (sepldist(globalposx - tspr->x, globalposy - tspr->y)>>5);
#else
        double f = (double) (tspr->owner + 1) * (FLT_EPSILON * 8.0);
        if (f != 0.0) f *= 1.0/(double) (sepldist(globalposx - tspr->x, globalposy - tspr->y)>>5);
//        bglBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
#endif
        bglDepthFunc(GL_LEQUAL);
//        bglDepthRange(0.0 - f, 1.0 - f);
    }

//    bglPushAttrib(GL_POLYGON_BIT);
    if ((grhalfxdown10x >= 0) ^((globalorientation&8) != 0) ^((globalorientation&4) != 0)) bglFrontFace(GL_CW); else bglFrontFace(GL_CCW);
    bglEnable(GL_CULL_FACE);
    bglCullFace(GL_BACK);

    bglEnable(GL_TEXTURE_2D);

    // tinting
    pc[0] = pc[1] = pc[2] = ((float)(numshades-min(max((globalshade * shadescale)+m->shadeoff,0),numshades)))/((float)numshades);
    polytintflags_t const tintflags = hictinting[globalpal].f;
    if (!(tintflags & HICTINT_PRECOMPUTED))
    {
        if (!(m->flags&1))
            hictinting_apply(pc, globalpal);
        else globalnoeffect=1;
    }

    // global tinting
    if (have_basepal_tint())
        hictinting_apply(pc, MAXPALOOKUPS-1);

    pc[3] = (tspr->cstat&2) ? glblend[tspr->blend].def[!!(tspr->cstat&512)].alpha : 1.0f;
    pc[3] *= 1.0f - sext->alpha;

    handle_blend(!!(tspr->cstat & 2), tspr->blend, !!(tspr->cstat & 512));

    if (m->usesalpha) //Sprites with alpha in texture
    {
        //      bglEnable(GL_BLEND);// bglBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        //      bglEnable(GL_ALPHA_TEST); bglAlphaFunc(GL_GREATER,0.32);
        //      float al = 0.32;
        // PLAG : default cutoff removed
        float al = 0.0;
        if (alphahackarray[globalpicnum] != 0)
            al=alphahackarray[globalpicnum] * (1.f/255.f);
        bglEnable(GL_BLEND);
        bglEnable(GL_ALPHA_TEST);
        bglAlphaFunc(GL_GREATER,al);
    }
    else
    {
        if ((tspr->cstat&2) || sext->alpha > 0.f || pc[3] < 1.0f)
            bglEnable(GL_BLEND); //else bglDisable(GL_BLEND);
    }
    bglColor4f(pc[0],pc[1],pc[2],pc[3]);
    //if (MFLAGS_NOCONV(m))
    //    bglColor4f(0.0f, 0.0f, 1.0f, 1.0f);
    //------------

    // PLAG: Cleaner model rotation code
    if (sext->pitch || sext->roll)
    {
        float f = 1.f/(fxdimen * fviewingrange) * (256.f/(65536.f*128.f)) * (m0.x+m1.x);
        Bmemset(&a0, 0, sizeof(a0));

        if (sext->offset.x)
            a0.x = (float) sext->offset.x * f;

        if (sext->offset.y)  // Compare with SCREEN_FACTORS above
            a0.y = (float) sext->offset.y * f;

        if ((sext->offset.z) && !(tspr->extra&TSPR_EXTRA_MDHACK))  // Compare with SCREEN_FACTORS above
            a0.z = (float)sext->offset.z / (gxyaspect * fxdimen * (65536.f/128.f) * (m0.z+m1.z));

        k0 = (float)sintable[(sext->pitch+512)&2047] * (1.f/16384.f);
        k1 = (float)sintable[sext->pitch&2047] * (1.f/16384.f);
        k2 = (float)sintable[(sext->roll+512)&2047] * (1.f/16384.f);
        k3 = (float)sintable[sext->roll&2047] * (1.f/16384.f);
    }

    float const xpanning = (float)sext->xpanning * (1.f/256.f);
    float const ypanning = (float)sext->ypanning * (1.f/256.f);

    for (surfi=0; surfi<m->head.numsurfs; surfi++)
    {
        //PLAG : sorting stuff
#ifdef USE_GLEXT
        void               *vbotemp;
        vec3f_t            *vertexhandle = NULL;
#endif
        uint16_t           *indexhandle;
        vec3f_t fp;

        const md3surf_t *const s = &m->head.surfs[surfi];

        v0 = &s->xyzn[m->cframe*s->numverts];
        v1 = &s->xyzn[m->nframe*s->numverts];

#ifdef USE_GLEXT
        if (r_vertexarrays && r_vbos)
        {
            if (++curvbo >= r_vbocount)
                curvbo = 0;

            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, vertvbos[curvbo]);
            vbotemp = bglMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
            vertexhandle = (vec3f_t *)vbotemp;
        }
#endif

        if (sext->pitch || sext->roll)
        {
            vec3f_t fp1, fp2;

            for (i=s->numverts-1; i>=0; i--)
            {
                fp.z = v0[i].x + a0.x;
                fp.x = v0[i].y + a0.y;
                fp.y = v0[i].z + a0.z;

                fp1.x = fp.x*k2 +       fp.y*k3;
                fp1.y = fp.x*k0*(-k3) + fp.y*k0*k2 + fp.z*(-k1);
                fp1.z = fp.x*k1*(-k3) + fp.y*k1*k2 + fp.z*k0;

                fp.z = v1[i].x + a0.x;
                fp.x = v1[i].y + a0.y;
                fp.y = v1[i].z + a0.z;

                fp2.x = fp.x*k2 +       fp.y*k3;
                fp2.y = fp.x*k0*(-k3) + fp.y*k0*k2 + fp.z*(-k1);
                fp2.z = fp.x*k1*(-k3) + fp.y*k1*k2 + fp.z*k0;
                fp.z = (fp1.z - a0.x)*m0.x + (fp2.z - a0.x)*m1.x;
                fp.x = (fp1.x - a0.y)*m0.y + (fp2.x - a0.y)*m1.y;
                fp.y = (fp1.y - a0.z)*m0.z + (fp2.y - a0.z)*m1.z;

#ifdef USE_GLEXT
                if (r_vertexarrays && r_vbos)
                    vertexhandle[i] = fp;
#endif

                vertlist[i] = fp;
            }
        }
        else
        {
            for (i=s->numverts-1; i>=0; i--)
            {
                fp.z = v0[i].x*m0.x + v1[i].x*m1.x;
                fp.y = v0[i].z*m0.z + v1[i].z*m1.z;
                fp.x = v0[i].y*m0.y + v1[i].y*m1.y;

#ifdef USE_GLEXT
                if (r_vertexarrays && r_vbos)
                    vertexhandle[i] = fp;
#endif

                vertlist[i] = fp;
            }
        }

#ifdef USE_GLEXT
        if (r_vertexarrays && r_vbos)
        {
            bglUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        }
#endif

        bglMatrixMode(GL_MODELVIEW); //Let OpenGL (and perhaps hardware :) handle the matrix rotation
        mat[3] = mat[7] = mat[11] = 0.f; mat[15] = 1.f; bglLoadMatrixf(mat);
        // PLAG: End

        i = mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,globalpal,surfi);
        if (!i)
            continue;
        //i = mdloadskin((md2model *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,surfi); //hack for testing multiple surfaces per MD3
        bglBindTexture(GL_TEXTURE_2D, i);

        bglMatrixMode(GL_TEXTURE);
        bglLoadIdentity();
        bglTranslatef(xpanning, ypanning, 1.0f);
        bglMatrixMode(GL_MODELVIEW);

        if (!(tspr->extra&TSPR_EXTRA_MDHACK))
        {
#ifdef USE_GLEXT
            i = r_detailmapping ? mdloadskin((md2model_t *) m, tile2model[Ptile2tile(tspr->picnum, lpal)].skinnum, DETAILPAL, surfi) : 0;

            if (i)
            {
                mdskinmap_t *sk;

                polymost_setupdetailtexture(++texunits, i);

                for (sk = m->skinmap; sk; sk = sk->next)
                    if ((int32_t) sk->palette == DETAILPAL && sk->skinnum == tile2model[Ptile2tile(tspr->picnum, lpal)].skinnum && sk->surfnum == surfi)
                        f = sk->param;

                bglMatrixMode(GL_TEXTURE);
                bglLoadIdentity();
                bglTranslatef(xpanning, ypanning, 1.0f);
                bglScalef(f, f, 1.0f);
                bglMatrixMode(GL_MODELVIEW);
            }

            i = r_glowmapping ? mdloadskin((md2model_t *) m, tile2model[Ptile2tile(tspr->picnum, lpal)].skinnum, GLOWPAL, surfi) : 0;

            if (i)
            {
                polymost_setupglowtexture(++texunits, i);

                bglMatrixMode(GL_TEXTURE);
                bglLoadIdentity();
                bglTranslatef(xpanning, ypanning, 1.0f);
                bglMatrixMode(GL_MODELVIEW);
            }

            if (r_vertexarrays && r_vbos)
            {
                bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexvbos[curvbo]);
                vbotemp = bglMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
                indexhandle = (uint16_t *) vbotemp;
            }
            else
#endif
                indexhandle = m->vindexes;

            //PLAG: delayed polygon-level sorted rendering

            if (m->usesalpha)
            {
                for (i=0; i<=s->numtris-1; ++i)
                {
                    vec3f_t const vlt[3] = { vertlist[s->tris[i].i[0]], vertlist[s->tris[i].i[1]], vertlist[s->tris[i].i[2]] };

                    // Matrix multiplication - ugly but clear
                    vec3f_t const fp[3] = { { (vlt[0].x * mat[0]) + (vlt[0].y * mat[4]) + (vlt[0].z * mat[8]) + mat[12],
                                              (vlt[0].x * mat[1]) + (vlt[0].y * mat[5]) + (vlt[0].z * mat[9]) + mat[13],
                                              (vlt[0].x * mat[2]) + (vlt[0].y * mat[6]) + (vlt[0].z * mat[10]) + mat[14] },

                                            { (vlt[1].x * mat[0]) + (vlt[1].y * mat[4]) + (vlt[1].z * mat[8]) + mat[12],
                                              (vlt[1].x * mat[1]) + (vlt[1].y * mat[5]) + (vlt[1].z * mat[9]) + mat[13],
                                              (vlt[1].x * mat[2]) + (vlt[1].y * mat[6]) + (vlt[1].z * mat[10]) + mat[14] },

                                            { (vlt[2].x * mat[0]) + (vlt[2].y * mat[4]) + (vlt[2].z * mat[8]) + mat[12],
                                              (vlt[2].x * mat[1]) + (vlt[2].y * mat[5]) + (vlt[2].z * mat[9]) + mat[13],
                                              (vlt[2].x * mat[2]) + (vlt[2].y * mat[6]) + (vlt[2].z * mat[10]) + mat[14] } };

                    f = (fp[0].x * fp[0].x) + (fp[0].y * fp[0].y) + (fp[0].z * fp[0].z);
                    g = (fp[1].x * fp[1].x) + (fp[1].y * fp[1].y) + (fp[1].z * fp[1].z);

                    if (f > g)
                        f = g;

                    g = (fp[2].x * fp[2].x) + (fp[2].y * fp[2].y) + (fp[2].z * fp[2].z);

                    if (f > g)
                        f = g;

                    m->maxdepths[i] = f;
                    m->indexes[i]   = i;
                }

                // dichotomic recursive sorting - about 100x less iterations than bubblesort
                quicksort(m->indexes, m->maxdepths, 0, s->numtris - 1);
            }

            md3draw_handle_triangles(s, indexhandle, texunits, m->usesalpha ? m : NULL);
        }
        else
        {
#ifdef USE_GLEXT
            if (r_vertexarrays && r_vbos)
            {
                bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexvbos[curvbo]);
                vbotemp = bglMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
                indexhandle = (uint16_t *) vbotemp;
            }
            else
#endif
                indexhandle = m->vindexes;

            md3draw_handle_triangles(s, indexhandle, texunits, NULL);
        }

        if (r_vertexarrays)
        {
#ifdef USE_GLEXT
            int32_t l;

            if (r_vbos)
            {
                bglUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
                bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
                bglBindBufferARB(GL_ARRAY_BUFFER_ARB, m->vbos[surfi]);

                l = GL_TEXTURE0_ARB;
                do
                {
                    bglClientActiveTextureARB(l++);
                    bglEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    bglTexCoordPointer(2, GL_FLOAT, 0, 0);
                } while (l <= texunits);

                bglBindBufferARB(GL_ARRAY_BUFFER_ARB, vertvbos[curvbo]);
                bglVertexPointer(3, GL_FLOAT, 0, 0);

                bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexvbos[curvbo]);
                bglDrawElements(GL_TRIANGLES, s->numtris * 3, GL_UNSIGNED_SHORT, 0);

                bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
                bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
            }
            else // r_vbos
            {
                l = GL_TEXTURE0_ARB;
                do
                {
                    bglClientActiveTextureARB(l++);
                    bglEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    bglTexCoordPointer(2, GL_FLOAT, 0, &(s->uv[0].u));
                } while (l <= texunits);

                bglVertexPointer(3, GL_FLOAT, 0, &(vertlist[0].x));

                bglDrawElements(GL_TRIANGLES, s->numtris * 3, GL_UNSIGNED_SHORT, m->vindexes);
            } // r_vbos

            while (texunits > GL_TEXTURE0_ARB)
            {
                bglMatrixMode(GL_TEXTURE);
                bglLoadIdentity();
                bglMatrixMode(GL_MODELVIEW);
                bglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1.0f);
                bglDisable(GL_TEXTURE_2D);
                bglDisableClientState(GL_TEXTURE_COORD_ARRAY);
                bglClientActiveTextureARB(texunits - 1);
                bglActiveTextureARB(--texunits);
            }
#else
            bglEnableClientState(GL_TEXTURE_COORD_ARRAY);
            bglTexCoordPointer(2, GL_FLOAT, 0, &(s->uv[0].u));

            bglVertexPointer(3, GL_FLOAT, 0, &(vertlist[0].x));

            bglDrawElements(GL_TRIANGLES, s->numtris * 3, GL_UNSIGNED_SHORT, m->vindexes);
#endif
        }
#ifdef USE_GLEXT
        else // r_vertexarrays
        {
            while (texunits > GL_TEXTURE0_ARB)
            {
                bglMatrixMode(GL_TEXTURE);
                bglLoadIdentity();
                bglMatrixMode(GL_MODELVIEW);
                bglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1.0f);
                bglDisable(GL_TEXTURE_2D);
                bglActiveTextureARB(--texunits);
            }
        } // r_vertexarrays
#endif
    }
    //------------

    if (m->usesalpha) bglDisable(GL_ALPHA_TEST);

    bglDisable(GL_CULL_FACE);
//    bglPopAttrib();

    bglMatrixMode(GL_TEXTURE);
    bglLoadIdentity();
    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    globalnoeffect=0;
    return 1;
}

static void md3free(md3model_t *m)
{
    mdanim_t *anim, *nanim = NULL;
    mdskinmap_t *sk, *nsk = NULL;

    if (!m) return;

    for (anim=m->animations; anim; anim=nanim)
    {
        nanim = anim->next;
        Bfree(anim);
    }
    for (sk=m->skinmap; sk; sk=nsk)
    {
        nsk = sk->next;
        Bfree(sk->fn);
        Bfree(sk);
    }

    if (m->head.surfs)
    {
        for (bssize_t surfi=m->head.numsurfs-1; surfi>=0; surfi--)
        {
            md3surf_t *s = &m->head.surfs[surfi];
            Bfree(s->tris);
            Bfree(s->geometry);  // FREE_SURFS_GEOMETRY
        }
        Bfree(m->head.surfs);
    }
    Bfree(m->head.tags);
    Bfree(m->head.frames);

    Bfree(m->texid);

    Bfree(m->muladdframes);

    Bfree(m->indexes);
    Bfree(m->vindexes);
    Bfree(m->maxdepths);

#ifdef USE_GLEXT
    if (m->vbos)
    {
        bglDeleteBuffersARB(m->head.numsurfs, m->vbos);
        DO_FREE_AND_NULL(m->vbos);
    }
#endif

    Bfree(m);
}

//---------------------------------------- MD3 LIBRARY ENDS ----------------------------------------
//--------------------------------------- MD LIBRARY BEGINS  ---------------------------------------

mdmodel_t *mdload(const char *filnam)
{
    mdmodel_t *vm;
    int32_t fil;
    int32_t i;

    vm = (mdmodel_t *)voxload(filnam);
    if (vm) return vm;

    fil = kopen4load(filnam,0);

    if (fil < 0)
        return NULL;

    kread(fil,&i,4);
    klseek(fil,0,SEEK_SET);

    switch (B_LITTLE32(i))
    {
    case IDP2_MAGIC:
//        initprintf("Warning: model \"%s\" is version IDP2; wanted version IDP3\n",filnam);
        vm = (mdmodel_t *)md2load(fil,filnam);
        break; //IDP2
    case IDP3_MAGIC:
        vm = (mdmodel_t *)md3load(fil);
        break; //IDP3
    default:
        vm = NULL;
        break;
    }

    kclose(fil);

    if (vm)
    {
        md3model_t *vm3 = (md3model_t *)vm;

        // smuggle the file name into the model struct.
        // head.nam is unused as far as I can tell
        Bstrncpyz(vm3->head.nam, filnam, sizeof(vm3->head.nam));

        md3postload_common(vm3);

#ifdef POLYMER
        if (glrendmode != REND_POLYMER)
            if (md3postload_polymer_check(vm3))
            {
                mdfree(vm);
                vm = NULL;
            }
#endif
    }

    return vm;
}

#ifdef USE_GLEXT
void md_allocvbos(void)
{
    int32_t i;

    indexvbos = (GLuint *) Xrealloc(indexvbos, sizeof(GLuint) * r_vbocount);
    vertvbos = (GLuint *) Xrealloc(vertvbos, sizeof(GLuint) * r_vbocount);

    if (r_vbocount != allocvbos)
    {
        bglGenBuffersARB(r_vbocount - allocvbos, &(indexvbos[allocvbos]));
        bglGenBuffersARB(r_vbocount - allocvbos, &(vertvbos[allocvbos]));

        i = allocvbos;
        while (i < r_vbocount)
        {
            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexvbos[i]);
            bglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, maxmodeltris * 3 * sizeof(uint16_t), NULL, GL_STREAM_DRAW_ARB);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, vertvbos[i]);
            bglBufferDataARB(GL_ARRAY_BUFFER_ARB, maxmodelverts * sizeof(vec3f_t), NULL, GL_STREAM_DRAW_ARB);
            i++;
        }

        bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

        allocvbos = r_vbocount;
    }
}
#endif

int32_t polymost_mddraw(const uspritetype *tspr)
{
#ifdef USE_GLEXT
    if (r_vbos && (r_vbocount > allocvbos))
        md_allocvbos();
#endif

    if (maxmodelverts > allocmodelverts)
    {
        vertlist = (vec3f_t *) Xrealloc(vertlist, sizeof(vec3f_t)*maxmodelverts);
        allocmodelverts = maxmodelverts;
    }

    mdmodel_t *const vm = models[tile2model[Ptile2tile(tspr->picnum,
    (tspr->owner >= MAXSPRITES) ? tspr->pal : sprite[tspr->owner].pal)].modelid];
    if (vm->mdnum == 1) { return polymost_voxdraw((voxmodel_t *)vm,tspr); }
    if (vm->mdnum == 3) { return polymost_md3draw((md3model_t *)vm,tspr); }
    return 0;
}

void mdfree(mdmodel_t *vm)
{
    if (vm->mdnum == 1) { voxfree((voxmodel_t *)vm); return; }
    if (vm->mdnum == 2 || vm->mdnum == 3) { md3free((md3model_t *)vm); return; }
}

#endif

//---------------------------------------- MD LIBRARY ENDS  ----------------------------------------
