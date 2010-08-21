//------------------------------------- MD2/MD3 LIBRARY BEGINS -------------------------------------

#ifdef POLYMOST

#include "compat.h"
#include "build.h"
#include "glbuild.h"
#include "pragmas.h"
#include "baselayer.h"
#include "engine_priv.h"
#include "hightile.h"
#include "polymost.h"
#include "mdsprite.h"
#include "cache1d.h"
#include "kplib.h"
#include "md4.h"

#include <math.h>

voxmodel_t *voxmodels[MAXVOXELS];
int32_t curextra=MAXTILES;
// nedpool *model_data_pool;
// #define MODEL_POOL_SIZE 20971520
#define model_data_pool (nedpool *) 0 // take it out of the system pool

int32_t addtileP(int32_t model,int32_t tile,int32_t pallet)
{
    UNREFERENCED_PARAMETER(model);
    if (curextra==MAXTILES+EXTRATILES-2)return curextra;
    if (tile2model[tile].modelid==-1) {tile2model[tile].pal=pallet; return tile;}
    if (tile2model[tile].pal==pallet)return tile;
    while (tile2model[tile].next!=-1)
    {
        tile=tile2model[tile].next;
        if (tile2model[tile].pal==pallet)return tile;
    }
    tile2model[tile].next=curextra;
    tile2model[curextra].pal=pallet;
    return curextra++;
}
int32_t Ptile2tile(int32_t tile,int32_t pallet)
{
    int32_t t=tile;
//  if(tile>=1550&&tile<=1589){initprintf("(%d, %d)\n",tile,pallet);pallet=0;}
    while ((tile=tile2model[tile].next)!=-1)
        if (tile2model[tile].pal==pallet)
            return tile;
    return t;
}

int32_t mdinited=0;
int32_t mdpause=0;

#define MODELALLOCGROUP 256
int32_t nummodelsalloced = 0, nextmodelid = 0;

static int32_t maxmodelverts = 0, allocmodelverts = 0;
static int32_t maxmodeltris = 0, allocmodeltris = 0;
static point3d *vertlist = NULL; //temp array to store interpolated vertices for drawing

static int32_t allocvbos = 0, curvbo = 0;
static GLuint* vertvbos = NULL;
static GLuint* indexvbos = NULL;

mdmodel_t *mdload(const char *);
int32_t mddraw(spritetype *);
void mdfree(mdmodel_t *);
int32_t globalnoeffect=0;

extern int32_t timerticspersec;

void freevbos()
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
                Bfree(m->vbos);
                m->vbos = NULL;
            }
        }

    if (allocvbos)
    {
        bglDeleteBuffersARB(allocvbos, indexvbos);
        bglDeleteBuffersARB(allocvbos, vertvbos);
        allocvbos = 0;
    }
}

void freeallmodels()
{
    int32_t i;

    if (models)
    {
        for (i=0; i<nextmodelid; i++) mdfree(models[i]);
        Bfree(models); models = NULL;
        nummodelsalloced = 0;
        nextmodelid = 0;
    }

    memset(tile2model,-1,sizeof(tile2model));
    curextra=MAXTILES;

    if (vertlist)
    {
        Bfree(vertlist);
        vertlist = NULL;
        allocmodelverts = maxmodelverts = 0;
        allocmodeltris = maxmodeltris = 0;
    }
    freevbos();

/*
    if (model_data_pool)
    {
        neddestroypool(model_data_pool);
        model_data_pool = NULL;
    }
*/
}

void clearskins()
{
    mdmodel_t *m;
    int32_t i, j;

    for (i=0; i<nextmodelid; i++)
    {
        m = models[i];
        if (m->mdnum == 1)
        {
            voxmodel_t *v = (voxmodel_t*)m;
            for (j=0; j<MAXPALOOKUPS; j++)
            {
                if (v->texid[j]) bglDeleteTextures(1,(GLuint*)&v->texid[j]);
                v->texid[j] = 0;
            }
        }
        else if (m->mdnum == 2 || m->mdnum == 3)
        {
            md2model_t *m2 = (md2model_t*)m;
            mdskinmap_t *sk;
            for (j=0; j<m2->numskins*(HICEFFECTMASK+1); j++)
            {
                if (m2->texid[j]) bglDeleteTextures(1,(GLuint*)&m2->texid[j]);
                m2->texid[j] = 0;
            }

            for (sk=m2->skinmap; sk; sk=sk->next)
                for (j=0; j<(HICEFFECTMASK+1); j++)
                {
                    if (sk->texid[j]) bglDeleteTextures(1,(GLuint*)&sk->texid[j]);
                    sk->texid[j] = 0;
                }
        }
    }

    for (i=0; i<MAXVOXELS; i++)
    {
        voxmodel_t *v = (voxmodel_t*)voxmodels[i]; if (!v) continue;
        for (j=0; j<MAXPALOOKUPS; j++)
        {
            if (v->texid[j]) bglDeleteTextures(1,(GLuint*)&v->texid[j]);
            v->texid[j] = 0;
        }
    }
}

void mdinit()
{
    memset(hudmem,0,sizeof(hudmem));
    freeallmodels();
//    if (!model_data_pool)
//        model_data_pool = nedcreatepool(MODEL_POOL_SIZE, 0);
    mdinited = 1;
}

int32_t md_loadmodel(const char *fn)
{
    mdmodel_t *vm, **ml;

    if (!mdinited) mdinit();

    if (nextmodelid >= nummodelsalloced)
    {
        ml = (mdmodel_t **)Brealloc(models,(nummodelsalloced+MODELALLOCGROUP)*sizeof(void*)); if (!ml) return(-1);
        models = ml; nummodelsalloced += MODELALLOCGROUP;
    }

    vm = mdload(fn); if (!vm) return(-1);
    models[nextmodelid++] = vm;
    return(nextmodelid-1);
}

int32_t md_setmisc(int32_t modelid, float scale, int32_t shadeoff, float zadd, int32_t flags)
{
    mdmodel_t *m;

    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;
    m = models[modelid];
    m->bscale = scale;
    m->shadeoff = shadeoff;
    m->zadd = zadd;
    m->flags = flags;

    return 0;
}

/*
int32_t md_tilehasmodel(int32_t tilenume,int32_t pal)
{
    if (!mdinited) return -1;
    return tile2model[Ptile2tile(tilenume,pal)].modelid;
}
*/

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
    return(i);
}

int32_t md_defineframe(int32_t modelid, const char *framename, int32_t tilenume, int32_t skinnum, float smoothduration, int32_t pal)
{
    md2model_t *m;
    int32_t i;

    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return(-1);
    if ((uint32_t)tilenume >= (uint32_t)MAXTILES) return(-2);
    if (!framename) return(-3);

    tilenume=addtileP(modelid,tilenume,pal);
    m = (md2model_t *)models[modelid];
    if (m->mdnum == 1)
    {
        tile2model[tilenume].modelid = modelid;
        tile2model[tilenume].framenum = tile2model[tilenume].skinnum = 0;
        return 0;
    }

    i = framename2index((mdmodel_t*)m,framename);
    if (i == m->numframes) return(-3);   // frame name invalid

    tile2model[tilenume].modelid = modelid;
    tile2model[tilenume].framenum = i;
    tile2model[tilenume].skinnum = skinnum;
    tile2model[tilenume].smoothduration = smoothduration;

    return 0;
}

int32_t md_defineanimation(int32_t modelid, const char *framestart, const char *frameend, int32_t fpssc, int32_t flags)
{
    md2model_t *m;
    mdanim_t ma, *map;
    int32_t i;

    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return(-1);

    memset(&ma, 0, sizeof(ma));
    m = (md2model_t *)models[modelid];
    if (m->mdnum < 2) return 0;

    //find index of start frame
    i = framename2index((mdmodel_t*)m,framestart);
    if (i == m->numframes) return -2;
    ma.startframe = i;

    //find index of finish frame which must trail start frame
    i = framename2index((mdmodel_t*)m,frameend);
    if (i == m->numframes) return -3;
    ma.endframe = i;

    ma.fpssc = fpssc;
    ma.flags = flags;

    map = (mdanim_t*)Bcalloc(1,sizeof(mdanim_t));
    if (!map) return(-4);
    Bmemcpy(map, &ma, sizeof(ma));

    map->next = m->animations;
    m->animations = map;

    return(0);
}

int32_t md_defineskin(int32_t modelid, const char *skinfn, int32_t palnum, int32_t skinnum, int32_t surfnum, float param, float specpower, float specfactor)
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
        if (sk->palette == (uint8_t)palnum && skinnum == sk->skinnum && surfnum == sk->surfnum) break;
    if (!sk)
    {
        sk = (mdskinmap_t *)Bcalloc(1,sizeof(mdskinmap_t));
        if (!sk) return -4;

        if (!skl) m->skinmap = sk;
        else skl->next = sk;
    }
    else if (sk->fn) Bfree(sk->fn);

    sk->palette = (uint8_t)palnum;
    sk->skinnum = skinnum;
    sk->surfnum = surfnum;
    sk->param = param;
    sk->specpower = specpower;
    sk->specfactor = specfactor;
    sk->fn = (char *)Bmalloc(strlen(skinfn)+1);
    if (!sk->fn) return(-4);
    strcpy(sk->fn, skinfn);

    return 0;
}

int32_t md_definehud(int32_t modelid, int32_t tilex, double xadd, double yadd, double zadd, double angadd, int32_t flags)
{
    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;
    if ((uint32_t)tilex >= (uint32_t)MAXTILES) return -2;

    hudmem[(flags>>2)&1][tilex].xadd = xadd;
    hudmem[(flags>>2)&1][tilex].yadd = yadd;
    hudmem[(flags>>2)&1][tilex].zadd = zadd;
    hudmem[(flags>>2)&1][tilex].angadd = ((int16_t)angadd)|2048;
    hudmem[(flags>>2)&1][tilex].flags = (int16_t)flags;

    return 0;
}

int32_t md_undefinetile(int32_t tile)
{
    if (!mdinited) return 0;
    if ((unsigned)tile >= (unsigned)MAXTILES) return -1;

    tile2model[tile].modelid = -1;
    tile2model[tile].next=-1;
    return 0;
}

int32_t md_undefinemodel(int32_t modelid)
{
    int32_t i;
    if (!mdinited) return 0;
    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;

    for (i=MAXTILES+EXTRATILES-1; i>=0; i--)
        if (tile2model[i].modelid == modelid)
            tile2model[i].modelid = -1;

    if (models)
    {
        mdfree(models[modelid]);
        models[modelid] = NULL;
    }

    return 0;
}

md2model_t *modelhead;
mdskinmap_t *skhead;

static int32_t daskinloader(int32_t filh, intptr_t *fptr, int32_t *bpl, int32_t *sizx, int32_t *sizy, int32_t *osizx, int32_t *osizy, char *hasalpha, int32_t pal, char effect)
{
    int32_t picfillen, j,y,x;
    char *picfil,*cptr,al=255;
    coltype *pic;
    int32_t xsiz, ysiz, tsizx, tsizy;
    int32_t r, g, b;

    picfillen = kfilelength(filh);
    picfil = (char *)Bmalloc(picfillen+1); if (!picfil) { return -1; }
    kread(filh, picfil, picfillen);

    // tsizx/y = replacement texture's natural size
    // xsiz/y = 2^x size of replacement

    kpgetdim(picfil,picfillen,&tsizx,&tsizy);
    if (tsizx == 0 || tsizy == 0) { Bfree(picfil); return -1; }

    if (!glinfo.texnpot)
    {
        for (xsiz=1; xsiz<tsizx; xsiz+=xsiz);
        for (ysiz=1; ysiz<tsizy; ysiz+=ysiz);
    }
    else
    {
        xsiz = tsizx;
        ysiz = tsizy;
    }
    *osizx = tsizx; *osizy = tsizy;
    pic = (coltype *)Bmalloc(xsiz*ysiz*sizeof(coltype));
    if (!pic) { Bfree(picfil); return -1; }
    memset(pic,0,xsiz*ysiz*sizeof(coltype));

    if (kprender(picfil,picfillen,(intptr_t)pic,xsiz*sizeof(coltype),xsiz,ysiz,0,0))
        { Bfree(picfil); Bfree(pic); return -1; }
    Bfree(picfil);

    cptr = &britable[gammabrightness ? 0 : curbrightness][0];
    r=(glinfo.bgra)?hictinting[pal].b:hictinting[pal].r;
    g=hictinting[pal].g;
    b=(glinfo.bgra)?hictinting[pal].r:hictinting[pal].b;
    for (y=0,j=0; y<tsizy; y++,j+=xsiz)
    {
        coltype *rpptr = &pic[j], tcol;

        for (x=0; x<tsizx; x++)
        {
            tcol.b = cptr[rpptr[x].b];
            tcol.g = cptr[rpptr[x].g];
            tcol.r = cptr[rpptr[x].r];

            if (effect & 1)
            {
                // greyscale
                tcol.b = max(tcol.b, max(tcol.g, tcol.r));
                tcol.g = tcol.r = tcol.b;
            }
            if (effect & 2)
            {
                // invert
                tcol.b = 255-tcol.b;
                tcol.g = 255-tcol.g;
                tcol.r = 255-tcol.r;
            }
            if (effect & 4)
            {
                // colorize
                tcol.b = min((int32_t)(tcol.b)*b/64,255);
                tcol.g = min((int32_t)(tcol.g)*g/64,255);
                tcol.r = min((int32_t)(tcol.r)*r/64,255);
            }

            rpptr[x].b = tcol.b;
            rpptr[x].g = tcol.g;
            rpptr[x].r = tcol.r;
            al &= rpptr[x].a;
        }
    }
    if (!glinfo.bgra)
    {
        for (j=xsiz*ysiz-1; j>=0; j--)
        {
            swapchar(&pic[j].r, &pic[j].b);
        }
    }

    *sizx = xsiz;
    *sizy = ysiz;
    *bpl = xsiz;
    *fptr = (intptr_t)pic;
    *hasalpha = (al != 255);
    return 0;
}

// JONOF'S COMPRESSED TEXTURE CACHE STUFF ---------------------------------------------------
int32_t mdloadskin_trytexcache(char *fn, int32_t len, int32_t pal, char effect, texcacheheader *head)
{
    int32_t fp;
    char cachefn[BMAX_PATH], *cp;
    uint8_t mdsum[16];

    if (!glinfo.texcompr || !glusetexcompr || !glusetexcache || !cacheindexptr || cachefilehandle < 0) return -1;
    if (!bglCompressedTexImage2DARB || !bglGetCompressedTexImageARB)
    {
        // lacking the necessary extensions to do this
        OSD_Printf("Warning: the GL driver lacks necessary functions to use caching\n");
        glusetexcache = 0;
        return -1;
    }

    md4once((uint8_t *)fn, strlen(fn), mdsum);
//    for (cp = cachefn, fp = 0; (*cp = TEXCACHEFILE[fp]); cp++,fp++);
//    *(cp++) = '/';
    cp = cachefn;
    for (fp = 0; fp < 16; phex(mdsum[fp++], cp), cp+=2);
    Bsprintf(cp, "-%x-%x%x", len, pal, effect);

//    fil = kopen4load(cachefn, 0);
//    if (fil < 0) return -1;

    {
        int32_t offset = 0;
        int32_t len = 0;
        int32_t i;
        /*
                texcacheindex *cacheindexptr = &firstcacheindex;

                do
                {
        //            initprintf("checking %s against %s\n",cachefn,cacheindexptr->name);
                    if (!Bstrcmp(cachefn,cacheindexptr->name))
                    {
                        offset = cacheindexptr->offset;
                        len = cacheindexptr->len;
        //                initprintf("got a match for %s offset %d\n",cachefn,offset);
                        break;
                    }
                    cacheindexptr = cacheindexptr->next;
                }
                while (cacheindexptr->next);
                */
        i = hash_find(&h_texcache,cachefn);
        if (i != -1)
        {
            texcacheindex *cacheindexptr = cacheptrs[i];
            len = cacheindexptr->len;
            offset = cacheindexptr->offset;
            // initprintf("got a match for %s offset %d\n",cachefn,offset);
        }
        if (len == 0) return -1; // didn't find it
        cachepos = offset;
    }

//    initprintf("Loading cached skin: %s\n", cachefn);

    if (memcachedata && memcachesize >= (signed)(cachepos + sizeof(texcacheheader)))
    {
        //        initprintf("using memcache!\n");
        Bmemcpy(head, memcachedata + cachepos, sizeof(texcacheheader));
        cachepos += sizeof(texcacheheader);
    }
    else
    {
        Blseek(cachefilehandle, cachepos, BSEEK_SET);
        if (Bread(cachefilehandle, head, sizeof(texcacheheader)) < (int32_t)sizeof(texcacheheader))
        {
            cachepos += sizeof(texcacheheader); 
            goto failure;
        }
        cachepos += sizeof(texcacheheader);
    }

    if (memcmp(head->magic, TEXCACHEMAGIC, 4)) goto failure;

    head->xdim = B_LITTLE32(head->xdim);
    head->ydim = B_LITTLE32(head->ydim);
    head->flags = B_LITTLE32(head->flags);
    head->quality = B_LITTLE32(head->quality);

    if (head->quality != r_downsize) goto failure;
    if ((head->flags & 4) && glusetexcache != 2) goto failure;
    if (!(head->flags & 4) && glusetexcache == 2) goto failure;
    if (gltexmaxsize && (head->xdim > (1<<gltexmaxsize) || head->ydim > (1<<gltexmaxsize))) goto failure;
    if (!glinfo.texnpot && (head->flags & 1)) goto failure;

    return cachefilehandle;
failure:
//    kclose(fil);
    initprintf("cache miss\n");
    return -1;
}

static int32_t mdloadskin_cached(int32_t fil, texcacheheader *head, int32_t *doalloc, GLuint *glpic, int32_t *xsiz, int32_t *ysiz, int32_t pal)
{
    int32_t level, r;
    texcachepicture pict;
    void *pic = NULL, *packbuf = NULL;
    void *midbuf = NULL;
    int32_t alloclen=0;

    UNREFERENCED_PARAMETER(pal);

    if (*doalloc&1)
    {
        bglGenTextures(1,glpic);  //# of textures (make OpenGL allocate structure)
        *doalloc |= 2;	// prevents bglGenTextures being called again if we fail in here
    }
    bglBindTexture(GL_TEXTURE_2D,*glpic);

    bglGetError();

    // load the mipmaps
    for (level = 0; level==0 || (pict.xdim > 1 || pict.ydim > 1); level++)
    {
        Blseek(fil, cachepos, BSEEK_SET);
        r = Bread(fil, &pict, sizeof(texcachepicture));
        cachepos += sizeof(texcachepicture);
        if (r < (int32_t)sizeof(texcachepicture)) goto failure;

        pict.size = B_LITTLE32(pict.size);
        pict.format = B_LITTLE32(pict.format);
        pict.xdim = B_LITTLE32(pict.xdim);
        pict.ydim = B_LITTLE32(pict.ydim);
        pict.border = B_LITTLE32(pict.border);
        pict.depth = B_LITTLE32(pict.depth);

        if (level == 0) { *xsiz = pict.xdim; *ysiz = pict.ydim; }

        if (alloclen < pict.size)
        {
            void *picc = Brealloc(pic, pict.size);
            if (!picc) goto failure; else pic = picc;
            alloclen = pict.size;

            picc = Brealloc(packbuf, alloclen+16);
            if (!picc) goto failure; else packbuf = picc;

            picc = Brealloc(midbuf, pict.size);
            if (!picc) goto failure; else midbuf = picc;
        }

        if (dedxtfilter(fil, &pict, pic, midbuf, packbuf, (head->flags&4)==4)) goto failure;

        bglCompressedTexImage2DARB(GL_TEXTURE_2D,level,pict.format,pict.xdim,pict.ydim,pict.border,
                                   pict.size,pic);
        if (bglGetError() != GL_NO_ERROR) goto failure;
    }

    if (midbuf) Bfree(midbuf);
    if (pic) Bfree(pic);
    if (packbuf) Bfree(packbuf);
    return 0;
failure:
    if (midbuf) Bfree(midbuf);
    if (pic) Bfree(pic);
    if (packbuf) Bfree(packbuf);
    return -1;
}
// --------------------------------------------------- JONOF'S COMPRESSED TEXTURE CACHE STUFF

//Note: even though it says md2model, it works for both md2model&md3model
int32_t mdloadskin(md2model_t *m, int32_t number, int32_t pal, int32_t surf)
{
    int32_t i,j, bpl, xsiz=0, ysiz=0, osizx, osizy, texfmt = GL_RGBA, intexfmt = GL_RGBA;
    intptr_t fptr=0;
    char *skinfile, hasalpha, fn[BMAX_PATH];
    GLuint *texidx = NULL;
    mdskinmap_t *sk, *skzero = NULL;
    int32_t doalloc = 1, filh;
    int32_t cachefil = -1, picfillen;
    texcacheheader cachead;

    modelhead=m; // for palmaps

    if (m->mdnum == 2) surf = 0;

    if ((unsigned)pal >= (unsigned)MAXPALOOKUPS) return 0;
    i = -1;
    for (sk = m->skinmap; sk; sk = sk->next)
    {
        if ((int32_t)sk->palette == pal && sk->skinnum == number && sk->surfnum == surf)
        {
            skinfile = sk->fn;
            texidx = &sk->texid[(globalnoeffect)?0:(hictinting[pal].f&HICEFFECTMASK)];
            strncpy(fn,skinfile,BMAX_PATH);
            //OSD_Printf("Using exact match skin (pal=%d,skinnum=%d,surfnum=%d) %s\n",pal,number,surf,skinfile);
            break;
        }
        //If no match, give highest priority to number, then pal.. (Parkar's request, 02/27/2005)
        else if (((int32_t)sk->palette ==   0) && (sk->skinnum == number) && (sk->surfnum == surf) && (i < 5)) { i = 5; skzero = sk; }
        else if (((int32_t)sk->palette == pal) && (sk->skinnum ==      0) && (sk->surfnum == surf) && (i < 4)) { i = 4; skzero = sk; }
        else if (((int32_t)sk->palette ==   0) && (sk->skinnum ==      0) && (sk->surfnum == surf) && (i < 3)) { i = 3; skzero = sk; }
        else if (((int32_t)sk->palette ==   0) && (sk->skinnum == number) && (i < 2)) { i = 2; skzero = sk; }
        else if (((int32_t)sk->palette == pal) && (sk->skinnum ==      0) && (i < 1)) { i = 1; skzero = sk; }
        else if (((int32_t)sk->palette ==   0) && (sk->skinnum ==      0) && (i < 0)) { i = 0; skzero = sk; }
    }
    if (!sk)
    {
        if (pal >= (MAXPALOOKUPS - RESERVEDPALS))
            return (0);
        if (skzero)
        {
            skinfile = skzero->fn;
            texidx = &skzero->texid[(globalnoeffect)?0:(hictinting[pal].f&HICEFFECTMASK)];
            strncpy(fn,skinfile,BMAX_PATH);
            //OSD_Printf("Using def skin 0,0 as fallback, pal=%d\n", pal);
        }
        else
        {
            if ((unsigned)number >= (unsigned)m->numskins) number = 0;
            skinfile = m->skinfn + number*64;
            texidx = &m->texid[ number * (HICEFFECTMASK+1) + (globalnoeffect)?0:(hictinting[pal].f&HICEFFECTMASK)];
            strncpy(fn,m->basepath,BMAX_PATH);
            if ((Bstrlen(fn) + Bstrlen(skinfile)) < BMAX_PATH)
                strcat(fn,skinfile);
            //OSD_Printf("Using MD2/MD3 skin (%d) %s, pal=%d\n",number,skinfile,pal);
        }
    }
    skhead=sk; // for palmaps
    if (!skinfile[0]) return 0;

    if (*texidx) return *texidx;

    // possibly fetch an already loaded multitexture :_)
    if (pal >= (MAXPALOOKUPS - RESERVEDPALS))
        for (i=0; i<nextmodelid; i++)
            for (skzero = ((md2model_t *)models[i])->skinmap; skzero; skzero = skzero->next)
                if (!Bstrcasecmp(skzero->fn, sk->fn) && skzero->texid[(globalnoeffect)?0:(hictinting[pal].f&HICEFFECTMASK)])
                {
                    sk->texid[(globalnoeffect)?0:(hictinting[pal].f&HICEFFECTMASK)] = skzero->texid[(globalnoeffect)?0:(hictinting[pal].f&HICEFFECTMASK)];
                    return sk->texid[(globalnoeffect)?0:(hictinting[pal].f&HICEFFECTMASK)];
                }

    *texidx = 0;

    if ((filh = kopen4load(fn, 0)) < 0)
    {
        OSD_Printf("Skin %s not found.\n",fn);
        skinfile[0] = 0;
        return 0;
    }

    picfillen = kfilelength(filh);
    kclose(filh);	// FIXME: shouldn't have to do this. bug in cache1d.c

    cachefil = mdloadskin_trytexcache(fn, picfillen, pal<<8, (globalnoeffect)?0:(hictinting[pal].f&HICEFFECTMASK), &cachead);
    if (cachefil >= 0 && !mdloadskin_cached(cachefil, &cachead, &doalloc, texidx, &xsiz, &ysiz, pal))
    {
        osizx = cachead.xdim;
        osizy = cachead.ydim;
        hasalpha = (cachead.flags & 2) ? 1 : 0;
        if (pal < (MAXPALOOKUPS - RESERVEDPALS))m->usesalpha = hasalpha;
//        kclose(cachefil);
        //kclose(filh);	// FIXME: uncomment when cache1d.c is fixed
        // cachefil >= 0, so it won't be rewritten
    }
    else
    {
//        if (cachefil >= 0) kclose(cachefil);
        cachefil = -1;	// the compressed version will be saved to disk

        if ((filh = kopen4load(fn, 0)) < 0) return -1;
        if (daskinloader(filh,&fptr,&bpl,&xsiz,&ysiz,&osizx,&osizy,&hasalpha,pal,(globalnoeffect)?0:(hictinting[pal].f&HICEFFECTMASK)))
        {
            kclose(filh);
            OSD_Printf("Failed loading skin file \"%s\"\n", fn);
            skinfile[0] = 0;
            return(0);
        }
        else kclose(filh);
        if (pal < (MAXPALOOKUPS - RESERVEDPALS)) m->usesalpha = hasalpha;
        if ((doalloc&3)==1) bglGenTextures(1,(GLuint*)texidx);
        bglBindTexture(GL_TEXTURE_2D,*texidx);

        //gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA,xsiz,ysiz,GL_BGRA_EXT,GL_UNSIGNED_BYTE,(char *)fptr);
        if (glinfo.texcompr && glusetexcompr) intexfmt = hasalpha ? GL_COMPRESSED_RGBA_ARB : GL_COMPRESSED_RGB_ARB;
        else if (!hasalpha) intexfmt = GL_RGB;
        if (glinfo.bgra) texfmt = GL_BGRA;
        uploadtexture((doalloc&1), xsiz, ysiz, intexfmt, texfmt, (coltype*)fptr, xsiz, ysiz, 0|8192);
        Bfree((void*)fptr);
    }

    if (!m->skinloaded)
    {
        if (xsiz != osizx || ysiz != osizy)
        {
            float fx, fy;
            fx = ((float)osizx)/((float)xsiz);
            fy = ((float)osizy)/((float)ysiz);
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

    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[gltexfiltermode].mag);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[gltexfiltermode].min);
    if (glinfo.maxanisotropy > 1.0)
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    if (glinfo.texcompr && glusetexcompr && glusetexcache)
        if (cachefil < 0)
        {
            // save off the compressed version
            cachead.quality = r_downsize;
            cachead.xdim = osizx>>cachead.quality;
            cachead.ydim = osizy>>cachead.quality;

            i = 0;
            for (j=0; j<31; j++)
            {
                if (xsiz == pow2long[j]) { i |= 1; }
                if (ysiz == pow2long[j]) { i |= 2; }
            }
            cachead.flags = (i!=3) | (hasalpha ? 2 : 0);
            OSD_Printf("Caching \"%s\"\n",fn);
            writexcache(fn, picfillen, pal<<8, (globalnoeffect)?0:(hictinting[pal].f&HICEFFECTMASK), &cachead);
        }

    return(*texidx);
}

//Note: even though it says md2model, it works for both md2model&md3model
void updateanimation(md2model_t *m, spritetype *tspr)
{
    mdanim_t *anim;
    int32_t i, j, k;
    int32_t fps;
    char lpal = (tspr->owner >= MAXSPRITES) ? tspr->pal : sprite[tspr->owner].pal;

    if (m->numframes < 2)
    {
        m->interpol = 0;
        return;
    }

    m->cframe = m->nframe = tile2model[Ptile2tile(tspr->picnum,lpal)].framenum;

    for (anim = m->animations;
            anim && anim->startframe != m->cframe;
            anim = anim->next) ;
    if (!anim)
    {
        if (r_animsmoothing && (tile2model[Ptile2tile(tspr->picnum,lpal)].smoothduration != 0) && (spritesmooth[tspr->owner].mdoldframe != m->cframe))
        {
            if (spritesmooth[tspr->owner].mdsmooth == 0)
            {
                spriteext[tspr->owner].mdanimtims = mdtims;
                m->interpol = 0;
                spritesmooth[tspr->owner].mdsmooth = 1;
                spritesmooth[tspr->owner].mdcurframe = m->cframe;
            }
            if (r_animsmoothing && (tile2model[Ptile2tile(tspr->picnum,lpal)].smoothduration != 0) && (spritesmooth[tspr->owner].mdcurframe != m->cframe))
            {
                spriteext[tspr->owner].mdanimtims = mdtims;
                m->interpol = 0;
                spritesmooth[tspr->owner].mdsmooth = 1;
                spritesmooth[tspr->owner].mdoldframe = spritesmooth[tspr->owner].mdcurframe;
                spritesmooth[tspr->owner].mdcurframe = m->cframe;
            }
        }
        else if (r_animsmoothing && (tile2model[Ptile2tile(tspr->picnum,lpal)].smoothduration != 0) && (spritesmooth[tspr->owner].mdcurframe != m->cframe))
        {
            spriteext[tspr->owner].mdanimtims = mdtims;
            m->interpol = 0;
            spritesmooth[tspr->owner].mdsmooth = 1;
            spritesmooth[tspr->owner].mdoldframe = spritesmooth[tspr->owner].mdcurframe;
            spritesmooth[tspr->owner].mdcurframe = m->cframe;
        }
        else
        {
            m->interpol = 0;
            return;
        }
    }

    if (anim && (((int32_t)spriteext[tspr->owner].mdanimcur) != anim->startframe))
    {
        //if (spriteext[tspr->owner].flags & SPREXT_NOMDANIM) OSD_Printf("SPREXT_NOMDANIM\n");
        //OSD_Printf("smooth launched ! oldanim %i new anim %i\n", spriteext[tspr->owner].mdanimcur, anim->startframe);
        spriteext[tspr->owner].mdanimcur = (int16_t)anim->startframe;
        spriteext[tspr->owner].mdanimtims = mdtims;
        m->interpol = 0;
        if (!r_animsmoothing || (tile2model[Ptile2tile(tspr->picnum,lpal)].smoothduration == 0))
        {
            m->cframe = m->nframe = anim->startframe;
            return;
        }
        m->nframe = anim->startframe;
        m->cframe = spritesmooth[tspr->owner].mdoldframe;
        spritesmooth[tspr->owner].mdsmooth = 1;
        return;
    }

    if (spritesmooth[tspr->owner].mdsmooth)
        ftol((1.0f / (float)(tile2model[Ptile2tile(tspr->picnum,lpal)].smoothduration)) * 66.f, &fps);
    else
        fps = anim->fpssc;

    i = (mdtims-spriteext[tspr->owner].mdanimtims)*((fps*timerticspersec)/120);

    if (spritesmooth[tspr->owner].mdsmooth)
        j = 65536;
    else
        j = ((anim->endframe+1-anim->startframe)<<16);
    //Just in case you play the game for a VERY int32_t time...
    if (i < 0) { i = 0; spriteext[tspr->owner].mdanimtims = mdtims; }
    //compare with j*2 instead of j to ensure i stays > j-65536 for MDANIM_ONESHOT
    if ((anim) && (i >= j+j) && (fps) && !mdpause) //Keep mdanimtims close to mdtims to avoid the use of MOD
        spriteext[tspr->owner].mdanimtims += j/((fps*timerticspersec)/120);

    k = i;

    if (anim && (anim->flags&MDANIM_ONESHOT))
        { if (i > j-65536) i = j-65536; }
    else { if (i >= j) { i -= j; if (i >= j) i %= j; } }

    if (r_animsmoothing && spritesmooth[tspr->owner].mdsmooth)
    {
        m->nframe = (anim) ? anim->startframe : spritesmooth[tspr->owner].mdcurframe;
        m->cframe = spritesmooth[tspr->owner].mdoldframe;
        //OSD_Printf("smoothing... cframe %i nframe %i\n", m->cframe, m->nframe);
        if (k > 65535)
        {
            spriteext[tspr->owner].mdanimtims = mdtims;
            m->interpol = 0;
            spritesmooth[tspr->owner].mdsmooth = 0;
            m->cframe = m->nframe = (anim) ? anim->startframe : spritesmooth[tspr->owner].mdcurframe;
            spritesmooth[tspr->owner].mdoldframe = m->cframe;
            //OSD_Printf("smooth stopped !\n");
            return;
        }
    }
    else
    {
        m->cframe = (i>>16)+anim->startframe;
        m->nframe = m->cframe+1; if (m->nframe > anim->endframe) m->nframe = anim->startframe;
        spritesmooth[tspr->owner].mdoldframe = m->cframe;
        //OSD_Printf("not smoothing... cframe %i nframe %i\n", m->cframe, m->nframe);
    }
    m->interpol = ((float)(i&65535))/65536.f;
    //OSD_Printf("interpol %f\n", m->interpol);
}

// VBO generation and allocation
static void mdloadvbos(md3model_t *m)
{
    int32_t     i;

    m->vbos = Bmalloc(m->head.numsurfs * sizeof(GLuint));
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

    m = (md2model_t *)Bcalloc(1,sizeof(md2model_t)); if (!m) return(0);
    m->mdnum = 2; m->scale = .01f;

    kread(fil,(char *)&head,sizeof(md2head_t));
    head.id = B_LITTLE32(head.id);                 head.vers = B_LITTLE32(head.vers);
    head.skinxsiz = B_LITTLE32(head.skinxsiz);     head.skinysiz = B_LITTLE32(head.skinysiz);
    head.framebytes = B_LITTLE32(head.framebytes); head.numskins = B_LITTLE32(head.numskins);
    head.numverts = B_LITTLE32(head.numverts);     head.numuv = B_LITTLE32(head.numuv);
    head.numtris = B_LITTLE32(head.numtris);       head.numglcmds = B_LITTLE32(head.numglcmds);
    head.numframes = B_LITTLE32(head.numframes);   head.ofsskins = B_LITTLE32(head.ofsskins);
    head.ofsuv = B_LITTLE32(head.ofsuv);           head.ofstris = B_LITTLE32(head.ofstris);
    head.ofsframes = B_LITTLE32(head.ofsframes);   head.ofsglcmds = B_LITTLE32(head.ofsglcmds);
    head.ofseof = B_LITTLE32(head.ofseof);

    if ((head.id != 0x32504449) || (head.vers != 8)) { Bfree(m); return(0); } //"IDP2"

    m->numskins = head.numskins;
    m->numframes = head.numframes;
    m->numverts = head.numverts;
    m->numglcmds = head.numglcmds;
    m->framebytes = head.framebytes;

    m->frames = (char *)Bcalloc(m->numframes,m->framebytes); if (!m->frames) { Bfree(m); return(0); }
    m->glcmds = (int32_t *)Bcalloc(m->numglcmds,sizeof(int32_t)); if (!m->glcmds) { Bfree(m->frames); Bfree(m); return(0); }
    m->tris = (md2tri_t *)Bcalloc(head.numtris, sizeof(md2tri_t)); if (!m->tris) { Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }
    m->uv = (md2uv_t *)Bcalloc(head.numuv, sizeof(md2uv_t)); if (!m->uv) { Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }

    klseek(fil,head.ofsframes,SEEK_SET);
    if (kread(fil,(char *)m->frames,m->numframes*m->framebytes) != m->numframes*m->framebytes)
        { Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }

    klseek(fil,head.ofsglcmds,SEEK_SET);
    if (kread(fil,(char *)m->glcmds,m->numglcmds*sizeof(int32_t)) != (int32_t)(m->numglcmds*sizeof(int32_t)))
        { Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }

    klseek(fil,head.ofstris,SEEK_SET);
    if (kread(fil,(char *)m->tris,head.numtris*sizeof(md2tri_t)) != (int32_t)(head.numtris*sizeof(md2tri_t)))
        { Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }

    klseek(fil,head.ofsuv,SEEK_SET);
    if (kread(fil,(char *)m->uv,head.numuv*sizeof(md2uv_t)) != (int32_t)(head.numuv*sizeof(md2uv_t)))
        { Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }

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
	for(i = head.numuv-1; i>=0; i--)
	{
		m->uv[i].u = B_LITTLE16(m->uv[i].u);
		m->uv[i].v = B_LITTLE16(m->uv[i].v);
	}
    }
#endif

    strcpy(st,filnam);
    for (i=strlen(st)-1; i>0; i--)
        if ((st[i] == '/') || (st[i] == '\\')) { i++; break; }
    if (i<0) i=0;
    st[i] = 0;
    m->basepath = (char *)Bmalloc(i+1); if (!m->basepath) { Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }
    strcpy(m->basepath, st);

    m->skinfn = (char *)Bcalloc(m->numskins,64); if (!m->skinfn) { Bfree(m->basepath); Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }
    klseek(fil,head.ofsskins,SEEK_SET);
    if (kread(fil,m->skinfn,64*m->numskins) != 64*m->numskins)
        { Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }

    m->texid = (GLuint *)Bcalloc(m->numskins, sizeof(GLuint) * (HICEFFECTMASK+1));
    if (!m->texid) { Bfree(m->skinfn); Bfree(m->basepath); Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }

    maxmodelverts = max(maxmodelverts, m->numverts);
    maxmodeltris = max(maxmodeltris, head.numtris);

    //return(m);

    // the MD2 is now loaded internally - let's begin the MD3 conversion process
    //OSD_Printf("Beginning md3 conversion.\n");
    m3 = (md3model_t *)Bcalloc(1, sizeof(md3model_t)); if (!m3) { Bfree(m->skinfn); Bfree(m->basepath); Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }
    m3->mdnum = 3; m3->texid = 0; m3->scale = m->scale;
    m3->head.id = 0x33504449; m3->head.vers = 15;
    // this changes the conversion code to do real MD2->MD3 conversion
    // it breaks HRP MD2 oozfilter, change the flags to 1337 to revert
    // to the old, working code
    m3->head.flags = 0;
    m3->head.numframes = m->numframes;
    m3->head.numtags = 0; m3->head.numsurfs = 1;
    m3->head.numskins = 0;

    m3->numskins = m3->head.numskins;
    m3->numframes = m3->head.numframes;

    m3->head.frames = (md3frame_t *)Bcalloc(m3->head.numframes, sizeof(md3frame_t)); if (!m3->head.frames) { Bfree(m3); Bfree(m->skinfn); Bfree(m->basepath); Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }
    m3->muladdframes = (point3d *)Bcalloc(m->numframes * 2, sizeof(point3d));

    f = (md2frame_t *)(m->frames);

    // frames converting
    i = 0;
    while (i < m->numframes)
    {
        f = (md2frame_t *)&m->frames[i*m->framebytes];
        strcpy(m3->head.frames[i].nam, f->name);
        //OSD_Printf("Copied frame %s.\n", m3->head.frames[i].nam);
        m3->muladdframes[i*2] = f->mul;
        m3->muladdframes[i*2+1] = f->add;
        i++;
    }

    m3->head.tags = NULL;

    m3->head.surfs = (md3surf_t *)Bcalloc(1, sizeof(md3surf_t)); if (!m3->head.surfs) { Bfree(m3->head.frames); Bfree(m3); Bfree(m->skinfn); Bfree(m->basepath); Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }
    s = m3->head.surfs;

    // model converting
    s->id = 0x33504449; s->flags = 0;
    s->numframes = m->numframes; s->numshaders = 0;
    s->numtris = head.numtris;
    s->numverts = head.numtris * 3; // oh man talk about memory effectiveness :((((
    // MD2 is actually more accurate than MD3 in term of uv-mapping, because each triangle has a triangle counterpart on the UV-map.
    // In MD3, each vertex unique UV coordinates, meaning that you have to duplicate vertices if you need non-seamless UV-mapping.

    maxmodelverts = max(maxmodelverts, s->numverts);

    strcpy(s->nam, "Dummy surface from MD2");

    s->shaders = NULL;

    s->tris = (md3tri_t *)Bcalloc(head.numtris, sizeof(md3tri_t)); if (!s->tris) { Bfree(s); Bfree(m3->head.frames); Bfree(m3); Bfree(m->skinfn); Bfree(m->basepath); Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }
    s->uv = (md3uv_t *)Bcalloc(s->numverts, sizeof(md3uv_t)); if (!s->uv) { Bfree(s->tris); Bfree(s); Bfree(m3->head.frames); Bfree(m3); Bfree(m->skinfn); Bfree(m->basepath); Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }
    s->xyzn = (md3xyzn_t *)Bcalloc(s->numverts * m->numframes, sizeof(md3xyzn_t)); if (!s->xyzn) { Bfree(s->uv); Bfree(s->tris); Bfree(s); Bfree(m3->head.frames); Bfree(m3); Bfree(m->skinfn); Bfree(m->basepath); Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m); return(0); }

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
                if (m3->head.flags == 1337)
                {
                    s->xyzn[(k*s->numverts) + (i*3) + j].x = f->verts[m->tris[i].v[j]].v[0];
                    s->xyzn[(k*s->numverts) + (i*3) + j].y = f->verts[m->tris[i].v[j]].v[1];
                    s->xyzn[(k*s->numverts) + (i*3) + j].z = f->verts[m->tris[i].v[j]].v[2];
                }
                else
                {
                    s->xyzn[(k*s->numverts) + (i*3) + j].x = (int16_t)(((f->verts[m->tris[i].v[j]].v[0] * f->mul.x) + f->add.x) * 64.f);
                    s->xyzn[(k*s->numverts) + (i*3) + j].y = (int16_t)(((f->verts[m->tris[i].v[j]].v[1] * f->mul.y) + f->add.y) * 64.f);
                    s->xyzn[(k*s->numverts) + (i*3) + j].z = (int16_t)(((f->verts[m->tris[i].v[j]].v[2] * f->mul.z) + f->add.z) * 64.f);
                }

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

        sk = (mdskinmap_t *)Bcalloc(1,sizeof(mdskinmap_t));
        sk->palette = 0;
        sk->skinnum = 0;
        sk->surfnum = 0;

        if (m->numskins > 0)
        {
            sk->fn = (char *)Bmalloc(strlen(m->basepath)+strlen(m->skinfn)+1);
            strcpy(sk->fn, m->basepath);
            strcat(sk->fn, m->skinfn);
        }
        m3->skinmap = sk;
    }

    m3->indexes = Bmalloc(sizeof(uint16_t) * s->numtris);
    m3->vindexes = Bmalloc(sizeof(uint16_t) * s->numtris * 3);
    m3->maxdepths = Bmalloc(sizeof(float) * s->numtris);

    m3->vbos = NULL;

    // die MD2 ! DIE !
    Bfree(m->texid); Bfree(m->skinfn); Bfree(m->basepath); Bfree(m->uv); Bfree(m->tris); Bfree(m->glcmds); Bfree(m->frames); Bfree(m);

    return((md2model_t *)m3);
}
//---------------------------------------- MD2 LIBRARY ENDS ----------------------------------------

// DICHOTOMIC RECURSIVE SORTING - USED BY MD3DRAW - MAY PUT IT IN ITS OWN SOURCE FILE LATER
int32_t partition(uint16_t *indexes, float *depths, int32_t f, int32_t l)
{
    int32_t up,down;
    float tempf;
    uint16_t tempus;
    float piv = depths[f];
    uint16_t piv2 = indexes[f];
    up = f;
    down = l;
    do
    {
        while ((depths[up] <= piv) && (up < l))
            up++;
        while ((depths[down] > piv)  && (down > f))
            down--;
        if (up < down)
        {
            tempf = depths[up];
            depths[up] = depths[down];
            depths[down] = tempf;
            tempus = indexes[up];
            indexes[up] = indexes[down];
            indexes[down] = tempus;
        }
    }
    while (down > up);
    depths[f] = depths[down];
    depths[down] = piv;
    indexes[f] = indexes[down];
    indexes[down] = piv2;
    return down;
}

void quicksort(uint16_t *indexes, float *depths, int32_t first, int32_t last)
{
    int32_t pivIndex = 0;
    if (first < last)
    {
        pivIndex = partition(indexes,depths,first, last);
        quicksort(indexes,depths,first,(pivIndex-1));
        quicksort(indexes,depths,(pivIndex+1),last);
    }
}
// END OF QUICKSORT LIB

//--------------------------------------- MD3 LIBRARY BEGINS ---------------------------------------

static md3model_t *md3load(int32_t fil)
{
    int32_t i, surfi, ofsurf, offs[4], leng[4];
    int32_t maxtrispersurf;
    md3model_t *m;
    md3surf_t *s;

    m = (md3model_t *)Bcalloc(1,sizeof(md3model_t)); if (!m) return(0);
    m->mdnum = 3; m->texid = 0; m->scale = .01f;

    m->muladdframes = NULL;

    kread(fil,&m->head,SIZEOF_MD3HEAD_T);
    m->head.id = B_LITTLE32(m->head.id);             m->head.vers = B_LITTLE32(m->head.vers);
    m->head.flags = B_LITTLE32(m->head.flags);       m->head.numframes = B_LITTLE32(m->head.numframes);
    m->head.numtags = B_LITTLE32(m->head.numtags);   m->head.numsurfs = B_LITTLE32(m->head.numsurfs);
    m->head.numskins = B_LITTLE32(m->head.numskins); m->head.ofsframes = B_LITTLE32(m->head.ofsframes);
    m->head.ofstags = B_LITTLE32(m->head.ofstags); m->head.ofssurfs = B_LITTLE32(m->head.ofssurfs);
    m->head.eof = B_LITTLE32(m->head.eof);

    if ((m->head.id != 0x33504449) && (m->head.vers != 15)) { Bfree(m); return(0); } //"IDP3"

    m->numskins = m->head.numskins; //<- dead code?
    m->numframes = m->head.numframes;

    ofsurf = m->head.ofssurfs;

    klseek(fil,m->head.ofsframes,SEEK_SET); i = m->head.numframes*sizeof(md3frame_t);
    m->head.frames = (md3frame_t *)Bmalloc(i); if (!m->head.frames) { Bfree(m); return(0); }
    kread(fil,m->head.frames,i);

    if (m->head.numtags == 0) m->head.tags = NULL;
    else
    {
        klseek(fil,m->head.ofstags,SEEK_SET); i = m->head.numtags*sizeof(md3tag_t);
        m->head.tags = (md3tag_t *)Bmalloc(i); if (!m->head.tags) { Bfree(m->head.frames); Bfree(m); return(0); }
        kread(fil,m->head.tags,i);
    }

    klseek(fil,m->head.ofssurfs,SEEK_SET); i = m->head.numsurfs*sizeof(md3surf_t);
    m->head.surfs = (md3surf_t *)Bmalloc(i); if (!m->head.surfs) { if (m->head.tags) Bfree(m->head.tags); Bfree(m->head.frames); Bfree(m); return(0); }

#if B_BIG_ENDIAN != 0
    {
        int32_t *l;

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
            int32_t *l;
            s->id = B_LITTLE32(s->id);
            l =	(int32_t *)&s->flags;
            for	(j=1+1+1+1+1+1+1+1+1+1-1; j>=0; j--) l[j] = B_LITTLE32(l[j]);
        }
#endif

        offs[0] = ofsurf+s->ofstris; leng[0] = s->numtris*sizeof(md3tri_t);
        offs[1] = ofsurf+s->ofsshaders; leng[1] = s->numshaders*sizeof(md3shader_t);
        offs[2] = ofsurf+s->ofsuv; leng[2] = s->numverts*sizeof(md3uv_t);
        offs[3] = ofsurf+s->ofsxyzn; leng[3] = s->numframes*s->numverts*sizeof(md3xyzn_t);
        //memoryusage += (s->numverts * s->numframes * sizeof(md3xyzn_t));
        //OSD_Printf("Current model geometry memory usage : %i.\n", memoryusage);


        s->tris = (md3tri_t *)Bmalloc(leng[0]+leng[1]+leng[2]+leng[3]);
        if (!s->tris)
        {
            for (surfi--; surfi>=0; surfi--) Bfree(m->head.surfs[surfi].tris);
            if (m->head.tags) Bfree(m->head.tags); Bfree(m->head.frames); Bfree(m); return(0);
        }
        s->shaders = (md3shader_t *)(((intptr_t)s->tris)+leng[0]);
        s->uv      = (md3uv_t     *)(((intptr_t)s->shaders)+leng[1]);
        s->xyzn    = (md3xyzn_t   *)(((intptr_t)s->uv)+leng[2]);

        klseek(fil,offs[0],SEEK_SET); kread(fil,s->tris   ,leng[0]);
        klseek(fil,offs[1],SEEK_SET); kread(fil,s->shaders,leng[1]);
        klseek(fil,offs[2],SEEK_SET); kread(fil,s->uv     ,leng[2]);
        klseek(fil,offs[3],SEEK_SET); kread(fil,s->xyzn   ,leng[3]);

#if B_BIG_ENDIAN != 0
        {
            int32_t *l;

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
                l = (int32_t*)&s->uv[i].u;
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

#if 0
    {
        char *buf, st[BMAX_PATH+2], bst[BMAX_PATH+2];
        int32_t j, bsc;

        strcpy(st,filnam);
        for (i=0,j=0; st[i]; i++) if ((st[i] == '/') || (st[i] == '\\')) j = i+1;
        st[j] = '*'; st[j+1] = 0;
        kzfindfilestart(st); bsc = -1;
        while (kzfindfile(st))
        {
            if (st[0] == '\\') continue;

            for (i=0,j=0; st[i]; i++) if (st[i] == '.') j = i+1;
            if ((!stricmp(&st[j],"JPG")) || (!stricmp(&st[j],"PNG")) || (!stricmp(&st[j],"GIF")) ||
                    (!stricmp(&st[j],"PCX")) || (!stricmp(&st[j],"TGA")) || (!stricmp(&st[j],"BMP")) ||
                    (!stricmp(&st[j],"CEL")))
            {
                for (i=0; st[i]; i++) if (st[i] != filnam[i]) break;
                if (i > bsc) { bsc = i; strcpy(bst,st); }
            }
        }
        if (!mdloadskin(&m->texid,&m->usesalpha,bst)) ;//bad!
    }
#endif

    m->indexes = Bmalloc(sizeof(uint16_t) * maxtrispersurf);
    m->vindexes = Bmalloc(sizeof(uint16_t) * maxtrispersurf * 3);
    m->maxdepths = Bmalloc(sizeof(float) * maxtrispersurf);

    m->vbos = NULL;

    return(m);
}

static inline void  invertmatrix(float* m, float* out)
{
    float det;

    det  = m[0] * (m[4]*m[8] - m[5] * m[7]);
    det -= m[1] * (m[3]*m[8] - m[5] * m[6]);
    det += m[2] * (m[3]*m[7] - m[4] * m[6]);

    det = 1.0f / det;

    out[0] = det * (m[4] * m[8] - m[5] * m[7]);
    out[3] = det * (m[5] * m[6] - m[3] * m[8]);
    out[6] = det * (m[3] * m[7] - m[1] * m[6]);

    out[1] = det * (m[2] * m[7] - m[1] * m[8]);
    out[4] = det * (m[0] * m[8] - m[2] * m[6]);
    out[7] = det * (m[1] * m[6] - m[0] * m[7]);

    out[2] = det * (m[1] * m[5] - m[2] * m[4]);
    out[5] = det * (m[2] * m[3] - m[0] * m[5]);
    out[8] = det * (m[0] * m[4] - m[1] * m[3]);
}

static inline void  normalize(float* vec)
{
    double norm;

    norm = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];

    norm = sqrt(norm);
    norm = 1.0 / norm;
    vec[0] *= norm;
    vec[1] *= norm;
    vec[2] *= norm;
}

static int      md3postload(md3model_t* m)
{
    int         framei, surfi, verti, trii, i;
    md3surf_t   *s;
    md3frame_t  *frame;
    md3xyzn_t   *frameverts;
    int         *numtris;
    float       dist, lat, lng, vec1[5], vec2[5], mat[9], r;

    // apparently we can't trust loaded models bounding box/sphere information,
    // so let's compute it ourselves

    framei = 0;

    while (framei < m->head.numframes)
    {
        frame = &m->head.frames[framei];

        frame->min.x    = 0.0f;
        frame->min.y    = 0.0f;
        frame->min.z    = 0.0f;

        frame->max.x    = 0.0f;
        frame->max.y    = 0.0f;
        frame->max.z    = 0.0f;

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
                    frame->min.x    = frameverts[verti].x;
                    frame->min.y    = frameverts[verti].y;
                    frame->min.z    = frameverts[verti].z;

                    frame->max.x    = frameverts[verti].x;
                    frame->max.y    = frameverts[verti].y;
                    frame->max.z    = frameverts[verti].z;
                }
                else
                {
                    if (frame->min.x > frameverts[verti].x)
                        frame->min.x = frameverts[verti].x;
                    if (frame->max.x < frameverts[verti].x)
                        frame->max.x = frameverts[verti].x;

                    if (frame->min.y > frameverts[verti].y)
                        frame->min.y = frameverts[verti].y;
                    if (frame->max.y < frameverts[verti].y)
                        frame->max.y = frameverts[verti].y;

                    if (frame->min.z > frameverts[verti].z)
                        frame->min.z = frameverts[verti].z;
                    if (frame->max.z < frameverts[verti].z)
                        frame->max.z = frameverts[verti].z;
                }

                verti++;
            }
            surfi++;
        }

        frame->cen.x = (frame->min.x + frame->max.x) / 2.0f;
        frame->cen.y = (frame->min.y + frame->max.y) / 2.0f;
        frame->cen.z = (frame->min.z + frame->max.z) / 2.0f;

        surfi = 0;
        while (surfi < m->head.numsurfs)
        {
            frameverts = &m->head.surfs[surfi].xyzn[framei * m->head.surfs[surfi].numverts];

            verti = 0;
            while (verti < m->head.surfs[surfi].numverts)
            {
                vec1[0] = frameverts[verti].x - frame->cen.x;
                vec1[1] = frameverts[verti].y - frame->cen.y;
                vec1[2] = frameverts[verti].z - frame->cen.z;

                dist = vec1[0] * vec1[0] + vec1[1] * vec1[1] + vec1[2] * vec1[2];

                if (dist > frame->r)
                    frame->r = dist;

                verti++;
            }
            surfi++;
        }

        frame->r = sqrt(frame->r);

        framei++;
    }

    // let's also repack the geometry to more usable formats

    surfi = 0;
    while (surfi < m->head.numsurfs)
    {
        s = &m->head.surfs[surfi];

        s->geometry = Bcalloc(m->head.numframes * s->numverts * sizeof(float), 15);

        numtris = Bcalloc(s->numverts, sizeof(int));

        verti = 0;
        while (verti < (m->head.numframes * s->numverts))
        {
            s->geometry[(verti * 15) + 0] = s->xyzn[verti].x;
            s->geometry[(verti * 15) + 1] = s->xyzn[verti].y;
            s->geometry[(verti * 15) + 2] = s->xyzn[verti].z;

            // normal extraction from packed spherical coordinates
            // FIXME: swapping lat and lng because of npherno's compiler
            lat = s->xyzn[verti].nlng * (2 * PI) / 255.0f;
            lng = s->xyzn[verti].nlat * (2 * PI) / 255.0f;

            s->geometry[(verti * 15) + 3] = cos(lat) * sin(lng);
            s->geometry[(verti * 15) + 4] = sin(lat) * sin(lng);
            s->geometry[(verti * 15) + 5] = cos(lng);

            verti++;
        }

        trii = 0;
        while (trii < s->numtris)
        {
            // let the vertices know they're being referenced by a triangle
            if (s->tris[trii].i[0] >= s->numverts || s->tris[trii].i[0] < 0 ||
                s->tris[trii].i[1] >= s->numverts || s->tris[trii].i[1] < 0 ||
                s->tris[trii].i[2] >= s->numverts || s->tris[trii].i[2] < 0) {
                // corrupt model
                Bfree(numtris);
                OSD_Printf("Triangle index out of bounds!\n");
                return 0;
            }
            numtris[s->tris[trii].i[0]]++;
            numtris[s->tris[trii].i[1]]++;
            numtris[s->tris[trii].i[2]]++;

            framei = 0;
            while (framei < m->head.numframes)
            {
                vec1[0] = s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[1] * 15) + 0] - 
                          s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[0] * 15) + 0];
                vec1[1] = s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[1] * 15) + 1] - 
                          s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[0] * 15) + 1];
                vec1[2] = s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[1] * 15) + 2] - 
                          s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[0] * 15) + 2];
                vec1[3] = s->uv[s->tris[trii].i[1]].u - s->uv[s->tris[trii].i[0]].u;
                vec1[4] = s->uv[s->tris[trii].i[1]].v - s->uv[s->tris[trii].i[0]].v;

                vec2[0] = s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[2] * 15) + 0] - 
                          s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[1] * 15) + 0];
                vec2[1] = s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[2] * 15) + 1] - 
                          s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[1] * 15) + 1];
                vec2[2] = s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[2] * 15) + 2] - 
                          s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[1] * 15) + 2];
                vec2[3] = s->uv[s->tris[trii].i[2]].u - s->uv[s->tris[trii].i[1]].u;
                vec2[4] = s->uv[s->tris[trii].i[2]].v - s->uv[s->tris[trii].i[1]].v;

                r = 1.0 / (vec1[3] * vec2[4] - vec2[3] * vec1[4]);

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

                // T and B are shared for the three vertices in that triangle
                i = 0;
                while (i < 6)
                {
                    s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[0] * 15) + 6 + i] += mat[i];
                    s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[1] * 15) + 6 + i] += mat[i];
                    s->geometry[(framei * s->numverts * 15) + (s->tris[trii].i[2] * 15) + 6 + i] += mat[i];
                    i++;
                }

                framei++;
            }

            trii++;
        }

        // now that we accumulated the TBNs, average and invert them for each vertex
        verti = 0;
        while (verti < (m->head.numframes * s->numverts))
        {
            i = 6;
            while (i < 12)
            {
                s->geometry[(verti * 15) + i] /= numtris[verti % s->numverts];
                i++;
            }
            // copy N over
            s->geometry[(verti * 15) + 12] = s->geometry[(verti * 15) + 3];
            s->geometry[(verti * 15) + 13] = s->geometry[(verti * 15) + 4];
            s->geometry[(verti * 15) + 14] = s->geometry[(verti * 15) + 5];

            invertmatrix(&s->geometry[(verti * 15) + 6], mat);
            memcpy(&s->geometry[(verti * 15) + 6], mat, sizeof(float) * 9);

            verti++;
        }

        Bfree(numtris);

        surfi++;
    }

    return 1;
}

static int32_t md3draw(md3model_t *m, spritetype *tspr)
{
    point3d fp, fp1, fp2, m0, m1, a0;
    md3xyzn_t *v0, *v1;
    int32_t i, j, k, l, surfi;
    float f, g, k0, k1, k2, k3, k4, k5, k6, k7, mat[16];
    md3surf_t *s;
    GLfloat pc[4];
    int32_t                 texunits = GL_TEXTURE0_ARB;
    mdskinmap_t *sk;
    //PLAG : sorting stuff
    void*               vbotemp;
    point3d*            vertexhandle = NULL;
    uint16_t*     indexhandle;
    char lpal = (tspr->owner >= MAXSPRITES) ? tspr->pal : sprite[tspr->owner].pal;

    if (r_vbos && (m->vbos == NULL))
        mdloadvbos(m);

    //    if ((tspr->cstat&48) == 32) return 0;

    updateanimation((md2model_t *)m,tspr);

    //create current&next frame's vertex list from whole list

    f = m->interpol; g = 1-f;

    if (m->interpol < 0 || m->interpol > 1 ||
            m->cframe < 0 || m->cframe >= m->numframes ||
            m->nframe < 0 || m->nframe >= m->numframes)
    {
        OSD_Printf("Model frame out of bounds!\n");
        if (m->interpol < 0)
            m->interpol = 0;
        if (m->interpol > 1)
            m->interpol = 1;
        if (m->cframe < 0)
            m->cframe = 0;
        if (m->cframe >= m->numframes)
            m->cframe = m->numframes - 1;
        if (m->nframe < 0)
            m->nframe = 0;
        if (m->nframe >= m->numframes)
            m->nframe = m->numframes - 1;
    }

    if (m->head.flags == 1337)
    {
        // md2
        m0.x = m->scale * g; m1.x = m->scale *f;
        m0.y = m->scale * g; m1.y = m->scale *f;
        m0.z = m->scale * g; m1.z = m->scale *f;
        a0.x = a0.y = 0; a0.z = ((globalorientation&8)?-m->zadd:m->zadd)*m->scale;
    }
    else
    {
        m0.x = (1.0/64.0) * m->scale * g; m1.x = (1.0/64.0) * m->scale *f;
        m0.y = (1.0/64.0) * m->scale * g; m1.y = (1.0/64.0) * m->scale *f;
        m0.z = (1.0/64.0) * m->scale * g; m1.z = (1.0/64.0) * m->scale *f;
        a0.x = a0.y = 0; a0.z = ((globalorientation&8)?-m->zadd:m->zadd)*m->scale;
    }


    // Parkar: Moved up to be able to use k0 for the y-flipping code
    k0 = (float)tspr->z;
    if ((globalorientation&128) && !((globalorientation&48)==32)) k0 += (float)((tilesizy[tspr->picnum]*tspr->yrepeat)<<1);

    // Parkar: Changed to use the same method as centeroriented sprites
    if (globalorientation&8) //y-flipping
    {
        m0.z = -m0.z; m1.z = -m1.z; a0.z = -a0.z;
        k0 -= (float)((tilesizy[tspr->picnum]*tspr->yrepeat)<<2);
    }
    if (globalorientation&4) { m0.y = -m0.y; m1.y = -m1.y; a0.y = -a0.y; } //x-flipping

    f = ((float)tspr->xrepeat)/64*m->bscale;
    m0.x *= f; m1.x *= f; a0.x *= f; f = -f;   // 20040610: backwards models aren't cool
    m0.y *= f; m1.y *= f; a0.y *= f;
    f = ((float)tspr->yrepeat)/64*m->bscale;
    m0.z *= f; m1.z *= f; a0.z *= f;

    // floor aligned
    k1 = (float)tspr->y;
    if ((globalorientation&48)==32)
    {
        m0.z = -m0.z; m1.z = -m1.z; a0.z = -a0.z;
        m0.y = -m0.y; m1.y = -m1.y; a0.y = -a0.y;
        f = a0.x; a0.x = a0.z; a0.z = f;
        k1 += (float)((tilesizy[tspr->picnum]*tspr->yrepeat)>>3);
    }

    f = (65536.0*512.0)/((float)xdimen*viewingrange);
    g = 32.0/((float)xdimen*gxyaspect);
    m0.y *= f; m1.y *= f; a0.y = (((float)(tspr->x-globalposx))/  1024.0 + a0.y)*f;
    m0.x *=-f; m1.x *=-f; a0.x = (((float)(k1     -globalposy))/ -1024.0 + a0.x)*-f;
    m0.z *= g; m1.z *= g; a0.z = (((float)(k0     -globalposz))/-16384.0 + a0.z)*g;

    k0 = ((float)(tspr->x-globalposx))*f/1024.0;
    k1 = ((float)(tspr->y-globalposy))*f/1024.0;
    f = gcosang2*gshang;
    g = gsinang2*gshang;
    k4 = (float)sintable[(tspr->ang+spriteext[tspr->owner].angoff+1024)&2047] / 16384.0;
    k5 = (float)sintable[(tspr->ang+spriteext[tspr->owner].angoff+ 512)&2047] / 16384.0;
    k2 = k0*(1-k4)+k1*k5;
    k3 = k1*(1-k4)-k0*k5;
    k6 = f*gstang - gsinang*gctang; k7 = g*gstang + gcosang*gctang;
    mat[0] = k4*k6 + k5*k7; mat[4] = gchang*gstang; mat[ 8] = k4*k7 - k5*k6; mat[12] = k2*k6 + k3*k7;
    k6 = f*gctang + gsinang*gstang; k7 = g*gctang - gcosang*gstang;
    mat[1] = k4*k6 + k5*k7; mat[5] = gchang*gctang; mat[ 9] = k4*k7 - k5*k6; mat[13] = k2*k6 + k3*k7;
    k6 =           gcosang2*gchang; k7 =           gsinang2*gchang;
    mat[2] = k4*k6 + k5*k7; mat[6] =-gshang;        mat[10] = k4*k7 - k5*k6; mat[14] = k2*k6 + k3*k7;

    mat[12] += a0.y*mat[0] + a0.z*mat[4] + a0.x*mat[ 8];
    mat[13] += a0.y*mat[1] + a0.z*mat[5] + a0.x*mat[ 9];
    mat[14] += a0.y*mat[2] + a0.z*mat[6] + a0.x*mat[10];

    // floor aligned
    if ((globalorientation&48)==32)
    {
        f = mat[4]; mat[4] = mat[8]*16.0; mat[8] = -f*(1.0/16.0);
        f = mat[5]; mat[5] = mat[9]*16.0; mat[9] = -f*(1.0/16.0);
        f = mat[6]; mat[6] = mat[10]*16.0; mat[10] = -f*(1.0/16.0);
    }

    //Mirrors
    if (grhalfxdown10x < 0) { mat[0] = -mat[0]; mat[4] = -mat[4]; mat[8] = -mat[8]; mat[12] = -mat[12]; }

    //------------
    //bit 10 is an ugly hack in game.c\animatesprites telling MD2SPRITE
    //to use Z-buffer hacks to hide overdraw problems with the shadows
    if (tspr->cstat&1024)
    {
        bglDepthFunc(GL_LESS); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS
        bglDepthRange(0.0,0.9999);
    }
    bglPushAttrib(GL_POLYGON_BIT);
    if ((grhalfxdown10x >= 0) ^((globalorientation&8) != 0) ^((globalorientation&4) != 0)) bglFrontFace(GL_CW); else bglFrontFace(GL_CCW);
    bglEnable(GL_CULL_FACE);
    bglCullFace(GL_BACK);

    bglEnable(GL_TEXTURE_2D);

    pc[0] = pc[1] = pc[2] = ((float)(numpalookups-min(max((globalshade * shadescale)+m->shadeoff,0),numpalookups)))/((float)numpalookups);
    if (!(hictinting[globalpal].f&4))
    {
        if (!(m->flags&1) || (!(tspr->owner >= MAXSPRITES) && sector[sprite[tspr->owner].sectnum].floorpal!=0))
        {
            pc[0] *= (float)hictinting[globalpal].r / 255.0;
            pc[1] *= (float)hictinting[globalpal].g / 255.0;
            pc[2] *= (float)hictinting[globalpal].b / 255.0;
            if (hictinting[MAXPALOOKUPS-1].r != 255 || hictinting[MAXPALOOKUPS-1].g != 255 || hictinting[MAXPALOOKUPS-1].b != 255)
            {
                pc[0] *= (float)hictinting[MAXPALOOKUPS-1].r / 255.0f;
                pc[1] *= (float)hictinting[MAXPALOOKUPS-1].g / 255.0f;
                pc[2] *= (float)hictinting[MAXPALOOKUPS-1].b / 255.0f;
            }
        }
        else globalnoeffect=1;
    }

    if (tspr->cstat&2) { if (!(tspr->cstat&512)) pc[3] = 0.66f; else pc[3] = 0.33f; }
    else pc[3] = 1.0;
    if (m->usesalpha) //Sprites with alpha in texture
    {
        //      bglEnable(GL_BLEND);// bglBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        //      bglEnable(GL_ALPHA_TEST); bglAlphaFunc(GL_GREATER,0.32);
        //      float al = 0.32;
        // PLAG : default cutoff removed
        float al = 0.0;
        if (alphahackarray[globalpicnum] != 0)
            al=alphahackarray[globalpicnum];
        bglEnable(GL_BLEND);
        bglEnable(GL_ALPHA_TEST);
        bglAlphaFunc(GL_GREATER,al);
    }
    else
    {
        if (tspr->cstat&2) bglEnable(GL_BLEND); //else bglDisable(GL_BLEND);
    }
    bglColor4f(pc[0],pc[1],pc[2],pc[3]);
    //if (m->head.flags == 1337)
    //    bglColor4f(0.0f, 0.0f, 1.0f, 1.0f);
    //------------

    // PLAG: Cleaner model rotation code
    if (spriteext[tspr->owner].pitch || spriteext[tspr->owner].roll || m->head.flags == 1337)
    {
        if (spriteext[tspr->owner].xoff)
            a0.x = (float)(spriteext[tspr->owner].xoff / (2560 * (m0.x+m1.x)));
        else
            a0.x = 0;
        if (spriteext[tspr->owner].yoff)
            a0.y = (float)(spriteext[tspr->owner].yoff / (2560 * (m0.x+m1.x)));
        else
            a0.y = 0;
        if ((spriteext[tspr->owner].zoff) && !(tspr->cstat&1024))
            a0.z = (float)(spriteext[tspr->owner].zoff / (655360 * (m0.z+m1.z)));
        else
            a0.z = 0;
        k0 = (float)sintable[(spriteext[tspr->owner].pitch+512)&2047] / 16384.0;
        k1 = (float)sintable[spriteext[tspr->owner].pitch&2047] / 16384.0;
        k2 = (float)sintable[(spriteext[tspr->owner].roll+512)&2047] / 16384.0;
        k3 = (float)sintable[spriteext[tspr->owner].roll&2047] / 16384.0;
    }
    for (surfi=0; surfi<m->head.numsurfs; surfi++)
    {
        s = &m->head.surfs[surfi];
        v0 = &s->xyzn[m->cframe*s->numverts];
        v1 = &s->xyzn[m->nframe*s->numverts];

        if (r_vertexarrays && r_vbos)
        {
            if (++curvbo >= r_vbocount)
                curvbo = 0;

            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, vertvbos[curvbo]);
            vbotemp = bglMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
            vertexhandle = (point3d *)vbotemp;
        }

        for (i=s->numverts-1; i>=0; i--)
        {
            if (spriteext[tspr->owner].pitch || spriteext[tspr->owner].roll || m->head.flags == 1337)
            {
                fp.z = ((m->head.flags == 1337) ? (v0[i].x * m->muladdframes[m->cframe*2].x) + m->muladdframes[m->cframe*2+1].x : v0[i].x) + a0.x;
                fp.x = ((m->head.flags == 1337) ? (v0[i].y * m->muladdframes[m->cframe*2].y) + m->muladdframes[m->cframe*2+1].y : v0[i].y) + a0.y;
                fp.y = ((m->head.flags == 1337) ? (v0[i].z * m->muladdframes[m->cframe*2].z) + m->muladdframes[m->cframe*2+1].z : v0[i].z) + a0.z;
                fp1.x = fp.x*k2 +       fp.y*k3;
                fp1.y = fp.x*k0*(-k3) + fp.y*k0*k2 + fp.z*(-k1);
                fp1.z = fp.x*k1*(-k3) + fp.y*k1*k2 + fp.z*k0;
                fp.z = ((m->head.flags == 1337) ? (v1[i].x * m->muladdframes[m->nframe*2].x) + m->muladdframes[m->nframe*2+1].x : v1[i].x) + a0.x;
                fp.x = ((m->head.flags == 1337) ? (v1[i].y * m->muladdframes[m->nframe*2].y) + m->muladdframes[m->nframe*2+1].y : v1[i].y) + a0.y;
                fp.y = ((m->head.flags == 1337) ? (v1[i].z * m->muladdframes[m->nframe*2].z) + m->muladdframes[m->nframe*2+1].z : v1[i].z) + a0.z;
                fp2.x = fp.x*k2 +       fp.y*k3;
                fp2.y = fp.x*k0*(-k3) + fp.y*k0*k2 + fp.z*(-k1);
                fp2.z = fp.x*k1*(-k3) + fp.y*k1*k2 + fp.z*k0;
                fp.z = (fp1.z - a0.x)*m0.x + (fp2.z - a0.x)*m1.x;
                fp.x = (fp1.x - a0.y)*m0.y + (fp2.x - a0.y)*m1.y;
                fp.y = (fp1.y - a0.z)*m0.z + (fp2.y - a0.z)*m1.z;
            }
            else
            {
                fp.z = v0[i].x*m0.x + v1[i].x*m1.x;
                fp.y = v0[i].z*m0.z + v1[i].z*m1.z;
                fp.x = v0[i].y*m0.y + v1[i].y*m1.y;
            }
            if (r_vertexarrays && r_vbos)
            {
                vertexhandle[i].x = fp.x;
                vertexhandle[i].y = fp.y;
                vertexhandle[i].z = fp.z;
            }
            vertlist[i].x = fp.x;
            vertlist[i].y = fp.y;
            vertlist[i].z = fp.z;
        }

        if (r_vertexarrays && r_vbos)
        {
            bglUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        }

        bglMatrixMode(GL_MODELVIEW); //Let OpenGL (and perhaps hardware :) handle the matrix rotation
        mat[3] = mat[7] = mat[11] = 0.f; mat[15] = 1.f; bglLoadMatrixf(mat);
        // PLAG: End

        i = mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,globalpal,surfi); if (!i) continue;
        //i = mdloadskin((md2model *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,surfi); //hack for testing multiple surfaces per MD3
        bglBindTexture(GL_TEXTURE_2D, i);

        if (r_detailmapping && !(tspr->cstat&1024))
            i = mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,DETAILPAL,surfi);
        else
            i = 0;

        if (i)
        {
            bglActiveTextureARB(++texunits);

            bglEnable(GL_TEXTURE_2D);
            bglBindTexture(GL_TEXTURE_2D, i);

            bglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);

            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

            bglTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

            bglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2.0f);

            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

            for (sk = m->skinmap; sk; sk = sk->next)
                if ((int32_t)sk->palette == DETAILPAL && sk->skinnum == tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum && sk->surfnum == surfi)
                    f = sk->param;

            bglMatrixMode(GL_TEXTURE);
            bglLoadIdentity();
            bglScalef(f, f, 1.0f);
            bglMatrixMode(GL_MODELVIEW);
        }

        if (r_glowmapping && !(tspr->cstat&1024))
            i = mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,GLOWPAL,surfi);
        else
            i = 0;

        if (i)
        {
            bglActiveTextureARB(++texunits);

            bglEnable(GL_TEXTURE_2D);
            bglBindTexture(GL_TEXTURE_2D, i);

            bglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB);

            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_ONE_MINUS_SRC_ALPHA);

            bglTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        }

        if (r_vertexarrays && r_vbos)
        {
            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexvbos[curvbo]);
            vbotemp = bglMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
            indexhandle = (uint16_t *)vbotemp;
        }
        else
            indexhandle = m->vindexes;

        //PLAG: delayed polygon-level sorted rendering
        if (m->usesalpha && !(tspr->cstat & 1024))
        {
            for (i=s->numtris-1; i>=0; i--)
            {
                // Matrix multiplication - ugly but clear
                fp.x = (vertlist[s->tris[i].i[0]].x * mat[0]) + (vertlist[s->tris[i].i[0]].y * mat[4]) + (vertlist[s->tris[i].i[0]].z * mat[8]) + mat[12];
                fp.y = (vertlist[s->tris[i].i[0]].x * mat[1]) + (vertlist[s->tris[i].i[0]].y * mat[5]) + (vertlist[s->tris[i].i[0]].z * mat[9]) + mat[13];
                fp.z = (vertlist[s->tris[i].i[0]].x * mat[2]) + (vertlist[s->tris[i].i[0]].y * mat[6]) + (vertlist[s->tris[i].i[0]].z * mat[10]) + mat[14];

                fp1.x = (vertlist[s->tris[i].i[1]].x * mat[0]) + (vertlist[s->tris[i].i[1]].y * mat[4]) + (vertlist[s->tris[i].i[1]].z * mat[8]) + mat[12];
                fp1.y = (vertlist[s->tris[i].i[1]].x * mat[1]) + (vertlist[s->tris[i].i[1]].y * mat[5]) + (vertlist[s->tris[i].i[1]].z * mat[9]) + mat[13];
                fp1.z = (vertlist[s->tris[i].i[1]].x * mat[2]) + (vertlist[s->tris[i].i[1]].y * mat[6]) + (vertlist[s->tris[i].i[1]].z * mat[10]) + mat[14];

                fp2.x = (vertlist[s->tris[i].i[2]].x * mat[0]) + (vertlist[s->tris[i].i[2]].y * mat[4]) + (vertlist[s->tris[i].i[2]].z * mat[8]) + mat[12];
                fp2.y = (vertlist[s->tris[i].i[2]].x * mat[1]) + (vertlist[s->tris[i].i[2]].y * mat[5]) + (vertlist[s->tris[i].i[2]].z * mat[9]) + mat[13];
                fp2.z = (vertlist[s->tris[i].i[2]].x * mat[2]) + (vertlist[s->tris[i].i[2]].y * mat[6]) + (vertlist[s->tris[i].i[2]].z * mat[10]) + mat[14];

                f = (fp.x * fp.x) + (fp.y * fp.y) + (fp.z * fp.z);

                g = (fp1.x * fp1.x) + (fp1.y * fp1.y) + (fp1.z * fp1.z);
                if (f > g)
                    f = g;
                g = (fp2.x * fp2.x) + (fp2.y * fp2.y) + (fp2.z * fp2.z);
                if (f > g)
                    f = g;

                m->maxdepths[i] = f;
                m->indexes[i] = i;
            }

            // dichotomic recursive sorting - about 100x less iterations than bubblesort
            quicksort(m->indexes, m->maxdepths, 0, s->numtris - 1);

            if (r_vertexarrays)
            {
                k = 0;
                for (i=s->numtris-1; i>=0; i--)
                    for (j=0; j<3; j++)
                        indexhandle[k++] = s->tris[m->indexes[i]].i[j];
            }
            else
            {
                bglBegin(GL_TRIANGLES);
                for (i=s->numtris-1; i>=0; i--)
                    for (j=0; j<3; j++)
                    {
                        k = s->tris[m->indexes[i]].i[j];
                        if (texunits > GL_TEXTURE0_ARB)
                        {
                            l = GL_TEXTURE0_ARB;
                            while (l <= texunits)
                                bglMultiTexCoord2fARB(l++, s->uv[k].u,s->uv[k].v);
                        }
                        else
                            bglTexCoord2f(s->uv[k].u,s->uv[k].v);
                        bglVertex3fv((float *)&vertlist[k]);
                    }
                bglEnd();
            }
        }
        else
        {
            if (r_vertexarrays)
            {
                k = 0;
                for (i=s->numtris-1; i>=0; i--)
                    for (j=0; j<3; j++)
                        indexhandle[k++] = s->tris[i].i[j];
            }
            else
            {
                bglBegin(GL_TRIANGLES);
                for (i=s->numtris-1; i>=0; i--)
                    for (j=0; j<3; j++)
                    {
                        k = s->tris[i].i[j];
                        if (texunits > GL_TEXTURE0_ARB)
                        {
                            l = GL_TEXTURE0_ARB;
                            while (l <= texunits)
                                bglMultiTexCoord2fARB(l++, s->uv[k].u,s->uv[k].v);
                        }
                        else
                            bglTexCoord2f(s->uv[k].u,s->uv[k].v);
                        bglVertex3fv((float *)&vertlist[k]);
                    }
                bglEnd();
            }
        }

        if (r_vertexarrays && r_vbos)
        {
            bglUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
        }

        if (r_vertexarrays)
        {
            if (r_vbos)
                bglBindBufferARB(GL_ARRAY_BUFFER_ARB, m->vbos[surfi]);
            l = GL_TEXTURE0_ARB;
            while (l <= texunits)
            {
                bglClientActiveTextureARB(l++);
                bglEnableClientState(GL_TEXTURE_COORD_ARRAY);
                if (r_vbos)
                    bglTexCoordPointer(2, GL_FLOAT, 0, 0);
                else
                    bglTexCoordPointer(2, GL_FLOAT, 0, &(s->uv[0].u));
            }

            if (r_vbos)
            {
                bglBindBufferARB(GL_ARRAY_BUFFER_ARB, vertvbos[curvbo]);
                bglVertexPointer(3, GL_FLOAT, 0, 0);
            }
            else
                bglVertexPointer(3, GL_FLOAT, 0, &(vertlist[0].x));

            if (r_vbos)
            {
                bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexvbos[curvbo]);
                bglDrawElements(GL_TRIANGLES, s->numtris * 3, GL_UNSIGNED_SHORT, 0);
            }
            else
                bglDrawElements(GL_TRIANGLES, s->numtris * 3, GL_UNSIGNED_SHORT, m->vindexes);

            if (r_vbos)
            {
                bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
                bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
            }
        }

        while (texunits > GL_TEXTURE0_ARB)
        {
            bglMatrixMode(GL_TEXTURE);
            bglLoadIdentity();
            bglMatrixMode(GL_MODELVIEW);
            bglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1.0f);
            bglDisable(GL_TEXTURE_2D);
            if (r_vertexarrays)
            {
                bglDisableClientState(GL_TEXTURE_COORD_ARRAY);
                bglClientActiveTextureARB(texunits - 1);
            }
            bglActiveTextureARB(--texunits);
        }
    }
    //------------

    if (m->usesalpha) bglDisable(GL_ALPHA_TEST);
    bglDisable(GL_CULL_FACE);
    bglPopAttrib();
    if (tspr->cstat&1024)
    {
        bglDepthFunc(GL_LESS); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS
        bglDepthRange(0.0,0.99999);
    }
    bglLoadIdentity();

    globalnoeffect=0;
    return 1;
}

static void md3free(md3model_t *m)
{
    mdanim_t *anim, *nanim = NULL;
    mdskinmap_t *sk, *nsk = NULL;
    md3surf_t *s;
    int32_t surfi;

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
        for (surfi=m->head.numsurfs-1; surfi>=0; surfi--)
        {
            s = &m->head.surfs[surfi];
            if (s->tris) Bfree(s->tris);
            if (m->head.flags == 1337)
            {
                if (s->shaders) Bfree(s->shaders);
                if (s->uv) Bfree(s->uv);
                if (s->xyzn) Bfree(s->xyzn);
                if (s->geometry) Bfree(s->geometry);
            }
        }
        Bfree(m->head.surfs);
    }
    if (m->head.tags) Bfree(m->head.tags);
    if (m->head.frames) Bfree(m->head.frames);

    if (m->texid) Bfree(m->texid);

    if (m->muladdframes) Bfree(m->muladdframes);

    if (m->indexes) Bfree(m->indexes);
    if (m->vindexes) Bfree(m->vindexes);
    if (m->maxdepths) Bfree(m->maxdepths);

    if (m->vbos)
    {
        bglDeleteBuffersARB(m->head.numsurfs, m->vbos);
        Bfree(m->vbos);
        m->vbos = NULL;
    }

    Bfree(m);
}

//---------------------------------------- MD3 LIBRARY ENDS ----------------------------------------
//--------------------------------------- VOX LIBRARY BEGINS ---------------------------------------

//For loading/conversion only
static int32_t xsiz, ysiz, zsiz, yzsiz, *vbit = 0; //vbit: 1 bit per voxel: 0=air,1=solid
static float xpiv, ypiv, zpiv; //Might want to use more complex/unique names!
static int32_t *vcolhashead = 0, vcolhashsizm1;
typedef struct { int32_t p, c, n; } voxcol_t;
static voxcol_t *vcol = 0; int32_t vnum = 0, vmax = 0;
typedef struct { int16_t x, y; } spoint2d;
static spoint2d *shp;
static int32_t *shcntmal, *shcnt = 0, shcntp;
static int32_t mytexo5, *zbit, gmaxx, gmaxy, garea, pow2m1[33];
static voxmodel_t *gvox;

//pitch must equal xsiz*4
uint32_t gloadtex(int32_t *picbuf, int32_t xsiz, int32_t ysiz, int32_t is8bit, int32_t dapal)
{
    uint32_t rtexid;
    coltype *pic, *pic2;
    char *cptr;
    int32_t i;

    pic = (coltype *)picbuf; //Correct for GL's RGB order; also apply gamma here..
    pic2 = (coltype *)Bmalloc(xsiz*ysiz*sizeof(int32_t)); if (!pic2) return((unsigned)-1);
    cptr = (char*)&britable[gammabrightness ? 0 : curbrightness][0];
    if (!is8bit)
    {
        for (i=xsiz*ysiz-1; i>=0; i--)
        {
            pic2[i].b = cptr[pic[i].r];
            pic2[i].g = cptr[pic[i].g];
            pic2[i].r = cptr[pic[i].b];
            pic2[i].a = 255;
        }
    }
    else
    {
        if (palookup[dapal] == NULL) dapal = 0;
        for (i=xsiz*ysiz-1; i>=0; i--)
        {
            pic2[i].b = cptr[palette[(int32_t)palookup[dapal][pic[i].a]*3+2]*4];
            pic2[i].g = cptr[palette[(int32_t)palookup[dapal][pic[i].a]*3+1]*4];
            pic2[i].r = cptr[palette[(int32_t)palookup[dapal][pic[i].a]*3+0]*4];
            pic2[i].a = 255;
        }
    }

    bglGenTextures(1,(GLuint*)&rtexid);
    bglBindTexture(GL_TEXTURE_2D,rtexid);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    bglTexImage2D(GL_TEXTURE_2D,0,4,xsiz,ysiz,0,GL_RGBA,GL_UNSIGNED_BYTE,(char *)pic2);
    Bfree(pic2);
    return(rtexid);
}

static int32_t getvox(int32_t x, int32_t y, int32_t z)
{
    z += x*yzsiz + y*zsiz;
    for (x=vcolhashead[(z*214013)&vcolhashsizm1]; x>=0; x=vcol[x].n)
        if (vcol[x].p == z) return(vcol[x].c);
    return(0x808080);
}

static void putvox(int32_t x, int32_t y, int32_t z, int32_t col)
{
    if (vnum >= vmax) { vmax = max(vmax<<1,4096); vcol = (voxcol_t *)Brealloc(vcol,vmax*sizeof(voxcol_t)); }

    z += x*yzsiz + y*zsiz;
    vcol[vnum].p = z; z = ((z*214013)&vcolhashsizm1);
    vcol[vnum].c = col;
    vcol[vnum].n = vcolhashead[z]; vcolhashead[z] = vnum++;
}

//Set all bits in vbit from (x,y,z0) to (x,y,z1-1) to 0's
#if 0
static void setzrange0(int32_t *lptr, int32_t z0, int32_t z1)
{
    int32_t z, ze;
    if (!((z0^z1)&~31)) { lptr[z0>>5] &= ((~(-1<<SHIFTMOD32(z0)))|(-1<<SHIFTMOD32(z1))); return; }
    z = (z0>>5); ze = (z1>>5);
    lptr[z] &=~(-1<<SHIFTMOD32(z0)); for (z++; z<ze; z++) lptr[z] = 0;
    lptr[z] &= (-1<<SHIFTMOD32(z1));
}
#endif
//Set all bits in vbit from (x,y,z0) to (x,y,z1-1) to 1's
static void setzrange1(int32_t *lptr, int32_t z0, int32_t z1)
{
    int32_t z, ze;
    if (!((z0^z1)&~31)) { lptr[z0>>5] |= ((~(-1<<SHIFTMOD32(z1)))&(-1<<SHIFTMOD32(z0))); return; }
    z = (z0>>5); ze = (z1>>5);
    lptr[z] |= (-1<<SHIFTMOD32(z0)); for (z++; z<ze; z++) lptr[z] = -1;
    lptr[z] |=~(-1<<SHIFTMOD32(z1));
}

static int32_t isrectfree(int32_t x0, int32_t y0, int32_t dx, int32_t dy)
{
#if 0
    int32_t i, j, x;
    i = y0*gvox->mytexx + x0;
    for (dy=0; dy; dy--,i+=gvox->mytexx)
        for (x=0; x<dx; x++) { j = i+x; if (zbit[j>>5]&(1<<SHIFTMOD32(j))) return(0); }
#else
    int32_t i, c, m, m1, x;

    i = y0*mytexo5 + (x0>>5); dx += x0-1; c = (dx>>5) - (x0>>5);
    m = ~pow2m1[x0&31]; m1 = pow2m1[(dx&31)+1];
    if (!c) { for (m&=m1; dy; dy--,i+=mytexo5) if (zbit[i]&m) return(0); }
    else
    {
        for (; dy; dy--,i+=mytexo5)
        {
            if (zbit[i]&m) return(0);
            for (x=1; x<c; x++) if (zbit[i+x]) return(0);
            if (zbit[i+x]&m1) return(0);
        }
    }
#endif
    return(1);
}

static void setrect(int32_t x0, int32_t y0, int32_t dx, int32_t dy)
{
#if 0
    int32_t i, j, y;
    i = y0*gvox->mytexx + x0;
    for (y=0; y<dy; y++,i+=gvox->mytexx)
        for (x=0; x<dx; x++) { j = i+x; zbit[j>>5] |= (1<<SHIFTMOD32(j)); }
#else
    int32_t i, c, m, m1, x;

    i = y0*mytexo5 + (x0>>5); dx += x0-1; c = (dx>>5) - (x0>>5);
    m = ~pow2m1[x0&31]; m1 = pow2m1[(dx&31)+1];
    if (!c) { for (m&=m1; dy; dy--,i+=mytexo5) zbit[i] |= m; }
    else
    {
        for (; dy; dy--,i+=mytexo5)
        {
            zbit[i] |= m;
            for (x=1; x<c; x++) zbit[i+x] = -1;
            zbit[i+x] |= m1;
        }
    }
#endif
}

static void cntquad(int32_t x0, int32_t y0, int32_t z0, int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2, int32_t face)
{
    int32_t x, y, z;

    UNREFERENCED_PARAMETER(x1);
    UNREFERENCED_PARAMETER(y1);
    UNREFERENCED_PARAMETER(z1);
    UNREFERENCED_PARAMETER(face);

    x = labs(x2-x0); y = labs(y2-y0); z = labs(z2-z0);
    if (!x) x = z; else if (!y) y = z;
    if (x < y) { z = x; x = y; y = z; }
    shcnt[y*shcntp+x]++;
    if (x > gmaxx) gmaxx = x;
    if (y > gmaxy) gmaxy = y;
    garea += (x+(VOXBORDWIDTH<<1))*(y+(VOXBORDWIDTH<<1));
    gvox->qcnt++;
}

static void addquad(int32_t x0, int32_t y0, int32_t z0, int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2, int32_t face)
{
    int32_t i, j, x, y, z, xx, yy, nx = 0, ny = 0, nz = 0, *lptr;
    voxrect_t *qptr;

    x = labs(x2-x0); y = labs(y2-y0); z = labs(z2-z0);
    if (!x) { x = y; y = z; i = 0; }
    else if (!y) { y = z; i = 1; }
    else i = 2;
    if (x < y) { z = x; x = y; y = z; i += 3; }
    z = shcnt[y*shcntp+x]++;
    lptr = &gvox->mytex[(shp[z].y+VOXBORDWIDTH)*gvox->mytexx+(shp[z].x+VOXBORDWIDTH)];
    switch (face)
    {
    case 0:
        ny = y1; x2 = x0; x0 = x1; x1 = x2; break;
    case 1:
        ny = y0; y0++; y1++; y2++; break;
    case 2:
        nz = z1; y0 = y2; y2 = y1; y1 = y0; z0++; z1++; z2++; break;
    case 3:
        nz = z0; break;
    case 4:
        nx = x1; y2 = y0; y0 = y1; y1 = y2; x0++; x1++; x2++; break;
    case 5:
        nx = x0; break;
    }
    for (yy=0; yy<y; yy++,lptr+=gvox->mytexx)
        for (xx=0; xx<x; xx++)
        {
            switch (face)
            {
            case 0:
                if (i < 3) { nx = x1+x-1-xx; nz = z1+yy;   } //back
                else { nx = x1+y-1-yy; nz = z1+xx;   }
                break;
            case 1:
                if (i < 3) { nx = x0+xx;     nz = z0+yy;   } //front
                else { nx = x0+yy;     nz = z0+xx;   }
                break;
            case 2:
                if (i < 3) { nx = x1-x+xx;   ny = y1-1-yy; } //bot
                else { nx = x1-1-yy;   ny = y1-1-xx; }
                break;
            case 3:
                if (i < 3) { nx = x0+xx;     ny = y0+yy;   } //top
                else { nx = x0+yy;     ny = y0+xx;   }
                break;
            case 4:
                if (i < 3) { ny = y1+x-1-xx; nz = z1+yy;   } //right
                else { ny = y1+y-1-yy; nz = z1+xx;   }
                break;
            case 5:
                if (i < 3) { ny = y0+xx;     nz = z0+yy;   } //left
                else { ny = y0+yy;     nz = z0+xx;   }
                break;
            }
            lptr[xx] = getvox(nx,ny,nz);
        }

    //Extend borders horizontally
    for (yy=VOXBORDWIDTH; yy<y+VOXBORDWIDTH; yy++)
        for (xx=0; xx<VOXBORDWIDTH; xx++)
        {
            lptr = &gvox->mytex[(shp[z].y+yy)*gvox->mytexx+shp[z].x];
            lptr[xx] = lptr[VOXBORDWIDTH]; lptr[xx+x+VOXBORDWIDTH] = lptr[x-1+VOXBORDWIDTH];
        }
    //Extend borders vertically
    for (yy=0; yy<VOXBORDWIDTH; yy++)
    {
        Bmemcpy(&gvox->mytex[(shp[z].y+yy)*gvox->mytexx+shp[z].x],
                &gvox->mytex[(shp[z].y+VOXBORDWIDTH)*gvox->mytexx+shp[z].x],
                (x+(VOXBORDWIDTH<<1))<<2);
        Bmemcpy(&gvox->mytex[(shp[z].y+y+yy+VOXBORDWIDTH)*gvox->mytexx+shp[z].x],
                &gvox->mytex[(shp[z].y+y-1+VOXBORDWIDTH)*gvox->mytexx+shp[z].x],
                (x+(VOXBORDWIDTH<<1))<<2);
    }

    qptr = &gvox->quad[gvox->qcnt];
    qptr->v[0].x = x0; qptr->v[0].y = y0; qptr->v[0].z = z0;
    qptr->v[1].x = x1; qptr->v[1].y = y1; qptr->v[1].z = z1;
    qptr->v[2].x = x2; qptr->v[2].y = y2; qptr->v[2].z = z2;
    for (j=0; j<3; j++) { qptr->v[j].u = shp[z].x+VOXBORDWIDTH; qptr->v[j].v = shp[z].y+VOXBORDWIDTH; }
    if (i < 3) qptr->v[1].u += x; else qptr->v[1].v += y;
    qptr->v[2].u += x; qptr->v[2].v += y;

    qptr->v[3].u = qptr->v[0].u - qptr->v[1].u + qptr->v[2].u;
    qptr->v[3].v = qptr->v[0].v - qptr->v[1].v + qptr->v[2].v;
    qptr->v[3].x = qptr->v[0].x - qptr->v[1].x + qptr->v[2].x;
    qptr->v[3].y = qptr->v[0].y - qptr->v[1].y + qptr->v[2].y;
    qptr->v[3].z = qptr->v[0].z - qptr->v[1].z + qptr->v[2].z;
    if (gvox->qfacind[face] < 0) gvox->qfacind[face] = gvox->qcnt;
    gvox->qcnt++;

}

static int32_t isolid(int32_t x, int32_t y, int32_t z)
{
    if ((uint32_t)x >= (uint32_t)xsiz) return(0);
    if ((uint32_t)y >= (uint32_t)ysiz) return(0);
    if ((uint32_t)z >= (uint32_t)zsiz) return(0);
    z += x*yzsiz + y*zsiz; return(vbit[z>>5]&(1<<SHIFTMOD32(z)));
}

static voxmodel_t *vox2poly()
{
    int32_t i, j, x, y, z, v, ov, oz = 0, cnt, sc, x0, y0, dx, dy,*bx0, *by0;
    void (*daquad)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);

    gvox = (voxmodel_t *)Bmalloc(sizeof(voxmodel_t)); if (!gvox) return(0);
    memset(gvox,0,sizeof(voxmodel_t));

    //x is largest dimension, y is 2nd largest dimension
    x = xsiz; y = ysiz; z = zsiz;
    if ((x < y) && (x < z)) x = z; else if (y < z) y = z;
    if (x < y) { z = x; x = y; y = z; }
    shcntp = x; i = x*y*sizeof(int32_t);
    shcntmal = (int32_t *)Bmalloc(i); if (!shcntmal) { Bfree(gvox); return(0); }
    memset(shcntmal,0,i); shcnt = &shcntmal[-shcntp-1];
    gmaxx = gmaxy = garea = 0;

    if (pow2m1[32] != -1) { for (i=0; i<32; i++) pow2m1[i] = (1<<i)-1; pow2m1[32] = -1; }
    for (i=0; i<7; i++) gvox->qfacind[i] = -1;

    i = ((max(ysiz,zsiz)+1)<<2);
    bx0 = (int32_t *)Bmalloc(i<<1); if (!bx0) { Bfree(gvox); return(0); }
    by0 = (int32_t *)(((intptr_t)bx0)+i);

    for (cnt=0; cnt<2; cnt++)
    {
        if (!cnt) daquad = cntquad;
        else daquad = addquad;
        gvox->qcnt = 0;

        memset(by0,-1,(max(ysiz,zsiz)+1)<<2); v = 0;

        for (i=-1; i<=1; i+=2)
            for (y=0; y<ysiz; y++)
                for (x=0; x<=xsiz; x++)
                    for (z=0; z<=zsiz; z++)
                    {
                        ov = v; v = (isolid(x,y,z) && (!isolid(x,y+i,z)));
                        if ((by0[z] >= 0) && ((by0[z] != oz) || (v >= ov)))
                            { daquad(bx0[z],y,by0[z],x,y,by0[z],x,y,z,i>=0); by0[z] = -1; }
                        if (v > ov) oz = z; else if ((v < ov) && (by0[z] != oz)) { bx0[z] = x; by0[z] = oz; }
                    }

        for (i=-1; i<=1; i+=2)
            for (z=0; z<zsiz; z++)
                for (x=0; x<=xsiz; x++)
                    for (y=0; y<=ysiz; y++)
                    {
                        ov = v; v = (isolid(x,y,z) && (!isolid(x,y,z-i)));
                        if ((by0[y] >= 0) && ((by0[y] != oz) || (v >= ov)))
                            { daquad(bx0[y],by0[y],z,x,by0[y],z,x,y,z,(i>=0)+2); by0[y] = -1; }
                        if (v > ov) oz = y; else if ((v < ov) && (by0[y] != oz)) { bx0[y] = x; by0[y] = oz; }
                    }

        for (i=-1; i<=1; i+=2)
            for (x=0; x<xsiz; x++)
                for (y=0; y<=ysiz; y++)
                    for (z=0; z<=zsiz; z++)
                    {
                        ov = v; v = (isolid(x,y,z) && (!isolid(x-i,y,z)));
                        if ((by0[z] >= 0) && ((by0[z] != oz) || (v >= ov)))
                            { daquad(x,bx0[z],by0[z],x,y,by0[z],x,y,z,(i>=0)+4); by0[z] = -1; }
                        if (v > ov) oz = z; else if ((v < ov) && (by0[z] != oz)) { bx0[z] = y; by0[z] = oz; }
                    }

        if (!cnt)
        {
            shp = (spoint2d *)Bmalloc(gvox->qcnt*sizeof(spoint2d));
            if (!shp) { Bfree(bx0); Bfree(gvox); return(0); }

            sc = 0;
            for (y=gmaxy; y; y--)
                for (x=gmaxx; x>=y; x--)
                {
                    i = shcnt[y*shcntp+x]; shcnt[y*shcntp+x] = sc; //shcnt changes from counter to head index
                    for (; i>0; i--) { shp[sc].x = x; shp[sc].y = y; sc++; }
                }

            for (gvox->mytexx=32; gvox->mytexx<(gmaxx+(VOXBORDWIDTH<<1)); gvox->mytexx<<=1);
            for (gvox->mytexy=32; gvox->mytexy<(gmaxy+(VOXBORDWIDTH<<1)); gvox->mytexy<<=1);
            while (gvox->mytexx*gvox->mytexy*8 < garea*9) //This should be sufficient to fit most skins...
            {
skindidntfit:
                ;
                if (gvox->mytexx <= gvox->mytexy) gvox->mytexx <<= 1; else gvox->mytexy <<= 1;
            }
            mytexo5 = (gvox->mytexx>>5);

            i = (((gvox->mytexx*gvox->mytexy+31)>>5)<<2);
            zbit = (int32_t *)Bmalloc(i); if (!zbit) { Bfree(bx0); Bfree(gvox); Bfree(shp); return(0); }
            memset(zbit,0,i);

            v = gvox->mytexx*gvox->mytexy;
            for (z=0; z<sc; z++)
            {
                dx = shp[z].x+(VOXBORDWIDTH<<1); dy = shp[z].y+(VOXBORDWIDTH<<1); i = v;
                do
                {
#if (VOXUSECHAR != 0)
                    x0 = (((rand()&32767)*(min(gvox->mytexx,255)-dx))>>15);
                    y0 = (((rand()&32767)*(min(gvox->mytexy,255)-dy))>>15);
#else
                    x0 = (((rand()&32767)*(gvox->mytexx+1-dx))>>15);
                    y0 = (((rand()&32767)*(gvox->mytexy+1-dy))>>15);
#endif
                    i--;
                    if (i < 0) //Time-out! Very slow if this happens... but at least it still works :P
                    {
                        Bfree(zbit);

                        //Re-generate shp[].x/y (box sizes) from shcnt (now head indices) for next pass :/
                        j = 0;
                        for (y=gmaxy; y; y--)
                            for (x=gmaxx; x>=y; x--)
                            {
                                i = shcnt[y*shcntp+x];
                                for (; j<i; j++) { shp[j].x = x0; shp[j].y = y0; }
                                x0 = x; y0 = y;
                            }
                        for (; j<sc; j++) { shp[j].x = x0; shp[j].y = y0; }

                        goto skindidntfit;
                    }
                }
                while (!isrectfree(x0,y0,dx,dy));
                while ((y0) && (isrectfree(x0,y0-1,dx,1))) y0--;
                while ((x0) && (isrectfree(x0-1,y0,1,dy))) x0--;
                setrect(x0,y0,dx,dy);
                shp[z].x = x0; shp[z].y = y0; //Overwrite size with top-left location
            }

            gvox->quad = (voxrect_t *)Bmalloc(gvox->qcnt*sizeof(voxrect_t));
            if (!gvox->quad) { Bfree(zbit); Bfree(shp); Bfree(bx0); Bfree(gvox); return(0); }

            gvox->mytex = (int32_t *)Bmalloc(gvox->mytexx*gvox->mytexy*sizeof(int32_t));
            if (!gvox->mytex) { Bfree(gvox->quad); Bfree(zbit); Bfree(shp); Bfree(bx0); Bfree(gvox); return(0); }
        }
    }
    Bfree(shp); Bfree(zbit); Bfree(bx0);
    return(gvox);
}

static int32_t loadvox(const char *filnam)
{
    int32_t i, j, k, x, y, z, pal[256], fil;
    char c[3], *tbuf;

    fil = kopen4load((char *)filnam,0); if (fil < 0) return(-1);
    kread(fil,&xsiz,4); xsiz = B_LITTLE32(xsiz);
    kread(fil,&ysiz,4); ysiz = B_LITTLE32(ysiz);
    kread(fil,&zsiz,4); zsiz = B_LITTLE32(zsiz);
    xpiv = ((float)xsiz)*.5;
    ypiv = ((float)ysiz)*.5;
    zpiv = ((float)zsiz)*.5;

    klseek(fil,-768,SEEK_END);
    for (i=0; i<256; i++)
        { kread(fil,c,3); pal[i] = (((int32_t)c[0])<<18)+(((int32_t)c[1])<<10)+(((int32_t)c[2])<<2)+(i<<24); }
    pal[255] = -1;

    vcolhashsizm1 = 8192-1;
    vcolhashead = (int32_t *)Bmalloc((vcolhashsizm1+1)*sizeof(int32_t)); if (!vcolhashead) { kclose(fil); return(-1); }
    memset(vcolhashead,-1,(vcolhashsizm1+1)*sizeof(int32_t));

    yzsiz = ysiz*zsiz; i = ((xsiz*yzsiz+31)>>3);
    vbit = (int32_t *)Bmalloc(i); if (!vbit) { kclose(fil); return(-1); }
    memset(vbit,0,i);

    tbuf = (char *)Bmalloc(zsiz*sizeof(uint8_t)); if (!tbuf) { kclose(fil); return(-1); }

    klseek(fil,12,SEEK_SET);
    for (x=0; x<xsiz; x++)
        for (y=0,j=x*yzsiz; y<ysiz; y++,j+=zsiz)
        {
            kread(fil,tbuf,zsiz);
            for (z=zsiz-1; z>=0; z--)
                { if (tbuf[z] != 255) { i = j+z; vbit[i>>5] |= (1<<SHIFTMOD32(i)); } }
        }

    klseek(fil,12,SEEK_SET);
    for (x=0; x<xsiz; x++)
        for (y=0,j=x*yzsiz; y<ysiz; y++,j+=zsiz)
        {
            kread(fil,tbuf,zsiz);
            for (z=0; z<zsiz; z++)
            {
                if (tbuf[z] == 255) continue;
                if ((!x) || (!y) || (!z) || (x == xsiz-1) || (y == ysiz-1) || (z == zsiz-1))
                    { putvox(x,y,z,pal[tbuf[z]]); continue; }
                k = j+z;
                if ((!(vbit[(k-yzsiz)>>5]&(1<<SHIFTMOD32(k-yzsiz)))) ||
                        (!(vbit[(k+yzsiz)>>5]&(1<<SHIFTMOD32(k+yzsiz)))) ||
                        (!(vbit[(k- zsiz)>>5]&(1<<SHIFTMOD32(k- zsiz)))) ||
                        (!(vbit[(k+ zsiz)>>5]&(1<<SHIFTMOD32(k+ zsiz)))) ||
                        (!(vbit[(k-    1)>>5]&(1<<SHIFTMOD32(k-    1)))) ||
                        (!(vbit[(k+    1)>>5]&(1<<SHIFTMOD32(k+    1)))))
                    { putvox(x,y,z,pal[tbuf[z]]); continue; }
            }
        }

    Bfree(tbuf); kclose(fil); return(0);
}

static int32_t loadkvx(const char *filnam)
{
    int32_t i, j, k, x, y, z, pal[256], z0, z1, mip1leng, ysizp1, fil;
    uint16_t *xyoffs;
    char c[3], *tbuf, *cptr;

    fil = kopen4load((char *)filnam,0); if (fil < 0) return(-1);
    kread(fil,&mip1leng,4); mip1leng = B_LITTLE32(mip1leng);
    kread(fil,&xsiz,4);     xsiz = B_LITTLE32(xsiz);
    kread(fil,&ysiz,4);     ysiz = B_LITTLE32(ysiz);
    kread(fil,&zsiz,4);     zsiz = B_LITTLE32(zsiz);
    kread(fil,&i,4); xpiv = ((float)B_LITTLE32(i))/256.0;
    kread(fil,&i,4); ypiv = ((float)B_LITTLE32(i))/256.0;
    kread(fil,&i,4); zpiv = ((float)B_LITTLE32(i))/256.0;
    klseek(fil,(xsiz+1)<<2,SEEK_CUR);
    ysizp1 = ysiz+1;
    i = xsiz*ysizp1*sizeof(int16_t);
    xyoffs = (uint16_t *)Bmalloc(i); if (!xyoffs) { kclose(fil); return(-1); }
    kread(fil,xyoffs,i); for (i=i/sizeof(int16_t)-1; i>=0; i--) xyoffs[i] = B_LITTLE16(xyoffs[i]);

    klseek(fil,-768,SEEK_END);
    for (i=0; i<256; i++)
        { kread(fil,c,3); pal[i] = B_LITTLE32((((int32_t)c[0])<<18)+(((int32_t)c[1])<<10)+(((int32_t)c[2])<<2)+(i<<24)); }

    yzsiz = ysiz*zsiz; i = ((xsiz*yzsiz+31)>>3);
    vbit = (int32_t *)Bmalloc(i); if (!vbit) { Bfree(xyoffs); kclose(fil); return(-1); }
    memset(vbit,0,i);

    for (vcolhashsizm1=4096; vcolhashsizm1<(mip1leng>>1); vcolhashsizm1<<=1); vcolhashsizm1--; //approx to numvoxs!
    vcolhashead = (int32_t *)Bmalloc((vcolhashsizm1+1)*sizeof(int32_t)); if (!vcolhashead) { Bfree(xyoffs); kclose(fil); return(-1); }
    memset(vcolhashead,-1,(vcolhashsizm1+1)*sizeof(int32_t));

    klseek(fil,28+((xsiz+1)<<2)+((ysizp1*xsiz)<<1),SEEK_SET);

    i = kfilelength(fil)-ktell(fil);
    tbuf = (char *)Bmalloc(i); if (!tbuf) { Bfree(xyoffs); kclose(fil); return(-1); }
    kread(fil,tbuf,i); kclose(fil);

    cptr = tbuf;
    for (x=0; x<xsiz; x++) //Set surface voxels to 1 else 0
        for (y=0,j=x*yzsiz; y<ysiz; y++,j+=zsiz)
        {
            i = xyoffs[x*ysizp1+y+1] - xyoffs[x*ysizp1+y]; if (!i) continue;
            z1 = 0;
            while (i)
            {
                z0 = (int32_t)cptr[0]; k = (int32_t)cptr[1]; cptr += 3;
                if (!(cptr[-1]&16)) setzrange1(vbit,j+z1,j+z0);
                i -= k+3; z1 = z0+k;
                setzrange1(vbit,j+z0,j+z1);
                for (z=z0; z<z1; z++) putvox(x,y,z,pal[*cptr++]);
            }
        }

    Bfree(tbuf); Bfree(xyoffs); return(0);
}

static int32_t loadkv6(const char *filnam)
{
    int32_t i, j, x, y, numvoxs, z0, z1, fil;
    uint16_t *ylen;
    char c[8];

    fil = kopen4load((char *)filnam,0); if (fil < 0) return(-1);
    kread(fil,&i,4); if (B_LITTLE32(i) != 0x6c78764b) { kclose(fil); return(-1); } //Kvxl
    kread(fil,&xsiz,4);    xsiz = B_LITTLE32(xsiz);
    kread(fil,&ysiz,4);    ysiz = B_LITTLE32(ysiz);
    kread(fil,&zsiz,4);    zsiz = B_LITTLE32(zsiz);
    kread(fil,&i,4);       xpiv = (float)(B_LITTLE32(i));
    kread(fil,&i,4);       ypiv = (float)(B_LITTLE32(i));
    kread(fil,&i,4);       zpiv = (float)(B_LITTLE32(i));
    kread(fil,&numvoxs,4); numvoxs = B_LITTLE32(numvoxs);

    ylen = (uint16_t *)Bmalloc(xsiz*ysiz*sizeof(int16_t));
    if (!ylen) { kclose(fil); return(-1); }

    klseek(fil,32+(numvoxs<<3)+(xsiz<<2),SEEK_SET);
    kread(fil,ylen,xsiz*ysiz*sizeof(int16_t)); for (i=xsiz*ysiz-1; i>=0; i--) ylen[i] = B_LITTLE16(ylen[i]);
    klseek(fil,32,SEEK_SET);

    yzsiz = ysiz*zsiz; i = ((xsiz*yzsiz+31)>>3);
    vbit = (int32_t *)Bmalloc(i); if (!vbit) { Bfree(ylen); kclose(fil); return(-1); }
    memset(vbit,0,i);

    for (vcolhashsizm1=4096; vcolhashsizm1<numvoxs; vcolhashsizm1<<=1); vcolhashsizm1--;
    vcolhashead = (int32_t *)Bmalloc((vcolhashsizm1+1)*sizeof(int32_t)); if (!vcolhashead) { Bfree(ylen); kclose(fil); return(-1); }
    memset(vcolhashead,-1,(vcolhashsizm1+1)*sizeof(int32_t));

    for (x=0; x<xsiz; x++)
        for (y=0,j=x*yzsiz; y<ysiz; y++,j+=zsiz)
        {
            z1 = zsiz;
            for (i=ylen[x*ysiz+y]; i>0; i--)
            {
                kread(fil,c,8); //b,g,r,a,z_lo,z_hi,vis,dir
                z0 = B_LITTLE16(*(uint16_t *)&c[4]);
                if (!(c[6]&16)) setzrange1(vbit,j+z1,j+z0);
                vbit[(j+z0)>>5] |= (1<<SHIFTMOD32(j+z0));
                putvox(x,y,z0,B_LITTLE32(*(int32_t *)&c[0])&0xffffff);
                z1 = z0+1;
            }
        }
    Bfree(ylen); kclose(fil); return(0);
}

#if 0
//While this code works, it's way too slow and can only cause trouble.
static int32_t loadvxl(const char *filnam)
{
    int32_t i, j, x, y, z, fil;
    char *v, *vbuf;

    fil = kopen4load((char *)filnam,0); if (fil < 0) return(-1);
    kread(fil,&i,4);
    kread(fil,&xsiz,4);
    kread(fil,&ysiz,4);
    if ((i != 0x09072000) || (xsiz != 1024) || (ysiz != 1024)) { kclose(fil); return(-1); }
    zsiz = 256;
    klseek(fil,96,SEEK_CUR); //skip pos&orient
    xpiv = ((float)xsiz)*.5;
    ypiv = ((float)ysiz)*.5;
    zpiv = ((float)zsiz)*.5;

    yzsiz = ysiz*zsiz; i = ((xsiz*yzsiz+31)>>3);
    vbit = (int32_t *)Bmalloc(i); if (!vbit) { kclose(fil); return(-1); }
    memset(vbit,-1,i);

    vcolhashsizm1 = 1048576-1;
    vcolhashead = (int32_t *)Bmalloc((vcolhashsizm1+1)*sizeof(int32_t)); if (!vcolhashead) { kclose(fil); return(-1); }
    memset(vcolhashead,-1,(vcolhashsizm1+1)*sizeof(int32_t));

    //Allocate huge buffer and load rest of file into it...
    i = kfilelength(fil)-ktell(fil);
    vbuf = (char *)Bmalloc(i); if (!vbuf) { kclose(fil); return(-1); }
    kread(fil,vbuf,i);
    kclose(fil);

    v = vbuf;
    for (y=0; y<ysiz; y++)
        for (x=0,j=y*zsiz; x<xsiz; x++,j+=yzsiz)
        {
            z = 0;
            while (1)
            {
                setzrange0(vbit,j+z,j+v[1]);
                for (z=v[1]; z<=v[2]; z++) putvox(x,y,z,(*(int32_t *)&v[(z-v[1]+1)<<2])&0xffffff);
                if (!v[0]) break; z = v[2]-v[1]-v[0]+2; v += v[0]*4;
                for (z+=v[3]; z<v[3]; z++) putvox(x,y,z,(*(int32_t *)&v[(z-v[3])<<2])&0xffffff);
            }
            v += ((((int32_t)v[2])-((int32_t)v[1])+2)<<2);
        }
    Bfree(vbuf); return(0);
}
#endif

void voxfree(voxmodel_t *m)
{
    if (!m) return;
    if (m->mytex) Bfree(m->mytex);
    if (m->quad) Bfree(m->quad);
    if (m->texid) Bfree(m->texid);
    Bfree(m);
}

voxmodel_t *voxload(const char *filnam)
{
    int32_t i, is8bit, ret;
    voxmodel_t *vm;

    i = strlen(filnam)-4; if (i < 0) return(0);
    if (!Bstrcasecmp(&filnam[i],".vox")) { ret = loadvox(filnam); is8bit = 1; }
    else if (!Bstrcasecmp(&filnam[i],".kvx")) { ret = loadkvx(filnam); is8bit = 1; }
    else if (!Bstrcasecmp(&filnam[i],".kv6")) { ret = loadkv6(filnam); is8bit = 0; }
    //else if (!Bstrcasecmp(&filnam[i],".vxl")) { ret = loadvxl(filnam); is8bit = 0; }
    else return(0);
    if (ret >= 0) vm = vox2poly(); else vm = 0;
    if (vm)
    {
        vm->mdnum = 1; //VOXel model id
        vm->scale = vm->bscale = 1.0;
        vm->xsiz = xsiz; vm->ysiz = ysiz; vm->zsiz = zsiz;
        vm->xpiv = xpiv; vm->ypiv = ypiv; vm->zpiv = zpiv;
        vm->is8bit = is8bit;

        vm->texid = (uint32_t *)Bcalloc(MAXPALOOKUPS,sizeof(uint32_t));
        if (!vm->texid) { voxfree(vm); vm = 0; }
    }
    if (shcntmal) { Bfree(shcntmal); shcntmal = 0; }
    if (vbit) { Bfree(vbit); vbit = 0; }
    if (vcol) { Bfree(vcol); vcol = 0; vnum = 0; vmax = 0; }
    if (vcolhashead) { Bfree(vcolhashead); vcolhashead = 0; }
    return(vm);
}

//Draw voxel model as perfect cubes
int32_t voxdraw(voxmodel_t *m, spritetype *tspr)
{
    point3d fp, m0, a0;
    int32_t i, j, fi, xx, yy, zz;
    float ru, rv, phack[2], clut[6] = {1,1,1,1,1,1}; //1.02,1.02,0.94,1.06,0.98,0.98};
    float f, g, k0, k1, k2, k3, k4, k5, k6, k7, mat[16], omat[16], pc[4];
    vert_t *vptr;

    if ((intptr_t)m == (intptr_t)(-1)) // hackhackhack
        return 0;
    if ((tspr->cstat&48)==32) return 0;

    //updateanimation((md2model *)m,tspr);

    m0.x = m->scale;
    m0.y = m->scale;
    m0.z = m->scale;
    a0.x = a0.y = 0; a0.z = ((globalorientation&8)?-m->zadd:m->zadd)*m->scale;

    //if (globalorientation&8) //y-flipping
    //{
    //   m0.z = -m0.z; a0.z = -a0.z;
    //      //Add height of 1st frame (use same frame to prevent animation bounce)
    //   a0.z += m->zsiz*m->scale;
    //}
    //if (globalorientation&4) { m0.y = -m0.y; a0.y = -a0.y; } //x-flipping

    f = ((float)tspr->xrepeat)*(256.0/320.0)/64.0*m->bscale;
    m0.x *= f; a0.x *= f; f = -f;
    m0.y *= f; a0.y *= f;
    f = ((float)tspr->yrepeat)/64.0*m->bscale;
    m0.z *= f; a0.z *= f;

    k0 = (float)tspr->z;
    if (globalorientation&128) k0 += (float)((tilesizy[tspr->picnum]*tspr->yrepeat)<<1);

    f = (65536.0*512.0)/((float)xdimen*viewingrange);
    g = 32.0/((float)xdimen*gxyaspect);
    m0.y *= f; a0.y = (((float)(tspr->x-globalposx))/  1024.0 + a0.y)*f;
    m0.x *=-f; a0.x = (((float)(tspr->y-globalposy))/ -1024.0 + a0.x)*-f;
    m0.z *= g; a0.z = (((float)(k0     -globalposz))/-16384.0 + a0.z)*g;

    k0 = ((float)(tspr->x-globalposx))*f/1024.0;
    k1 = ((float)(tspr->y-globalposy))*f/1024.0;
    f = gcosang2*gshang;
    g = gsinang2*gshang;
    k4 = (float)sintable[(tspr->ang+spriteext[tspr->owner].angoff+1024)&2047] / 16384.0;
    k5 = (float)sintable[(tspr->ang+spriteext[tspr->owner].angoff+ 512)&2047] / 16384.0;
    k2 = k0*(1-k4)+k1*k5;
    k3 = k1*(1-k4)-k0*k5;
    k6 = f*gstang - gsinang*gctang; k7 = g*gstang + gcosang*gctang;
    mat[0] = k4*k6 + k5*k7; mat[4] = gchang*gstang; mat[ 8] = k4*k7 - k5*k6; mat[12] = k2*k6 + k3*k7;
    k6 = f*gctang + gsinang*gstang; k7 = g*gctang - gcosang*gstang;
    mat[1] = k4*k6 + k5*k7; mat[5] = gchang*gctang; mat[ 9] = k4*k7 - k5*k6; mat[13] = k2*k6 + k3*k7;
    k6 =           gcosang2*gchang; k7 =           gsinang2*gchang;
    mat[2] = k4*k6 + k5*k7; mat[6] =-gshang;        mat[10] = k4*k7 - k5*k6; mat[14] = k2*k6 + k3*k7;

    mat[12] += a0.y*mat[0] + a0.z*mat[4] + a0.x*mat[ 8];
    mat[13] += a0.y*mat[1] + a0.z*mat[5] + a0.x*mat[ 9];
    mat[14] += a0.y*mat[2] + a0.z*mat[6] + a0.x*mat[10];

    //Mirrors
    if (grhalfxdown10x < 0) { mat[0] = -mat[0]; mat[4] = -mat[4]; mat[8] = -mat[8]; mat[12] = -mat[12]; }

    //------------
    //bit 10 is an ugly hack in game.c\animatesprites telling MD2SPRITE
    //to use Z-buffer hacks to hide overdraw problems with the shadows
    if (tspr->cstat&1024)
    {
        bglDepthFunc(GL_LESS); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS
        bglDepthRange(0.0,0.9999);
    }
    bglPushAttrib(GL_POLYGON_BIT);
    if ((grhalfxdown10x >= 0) /*^ ((globalorientation&8) != 0) ^ ((globalorientation&4) != 0)*/) bglFrontFace(GL_CW); else bglFrontFace(GL_CCW);
    bglEnable(GL_CULL_FACE);
    bglCullFace(GL_BACK);

    bglEnable(GL_TEXTURE_2D);

    pc[0] = pc[1] = pc[2] = ((float)(numpalookups-min(max((globalshade * shadescale)+m->shadeoff,0),numpalookups)))/((float)numpalookups);
    pc[0] *= (float)hictinting[globalpal].r / 255.0;
    pc[1] *= (float)hictinting[globalpal].g / 255.0;
    pc[2] *= (float)hictinting[globalpal].b / 255.0;
    if (tspr->cstat&2) { if (!(tspr->cstat&512)) pc[3] = 0.66f; else pc[3] = 0.33f; }
    else pc[3] = 1.0;
    if (tspr->cstat&2) bglEnable(GL_BLEND); //else bglDisable(GL_BLEND);
    //------------

    //transform to Build coords
    Bmemcpy(omat,mat,sizeof(omat));
    f = 1.f/64.f;
    g = m0.x*f; mat[0] *= g; mat[1] *= g; mat[2] *= g;
    g = m0.y*f; mat[4] = omat[8]*g; mat[5] = omat[9]*g; mat[6] = omat[10]*g;
    g =-m0.z*f; mat[8] = omat[4]*g; mat[9] = omat[5]*g; mat[10] = omat[6]*g;
    mat[12] -= (m->xpiv*mat[0] + m->ypiv*mat[4] + (m->zpiv+m->zsiz*.5)*mat[ 8]);
    mat[13] -= (m->xpiv*mat[1] + m->ypiv*mat[5] + (m->zpiv+m->zsiz*.5)*mat[ 9]);
    mat[14] -= (m->xpiv*mat[2] + m->ypiv*mat[6] + (m->zpiv+m->zsiz*.5)*mat[10]);
    bglMatrixMode(GL_MODELVIEW); //Let OpenGL (and perhaps hardware :) handle the matrix rotation
    mat[3] = mat[7] = mat[11] = 0.f; mat[15] = 1.f;

    bglLoadMatrixf(mat);

    ru = 1.f/((float)m->mytexx);
    rv = 1.f/((float)m->mytexy);
#if (VOXBORDWIDTH == 0)
    uhack[0] = ru*.125; uhack[1] = -uhack[0];
    vhack[0] = rv*.125; vhack[1] = -vhack[0];
#endif
    phack[0] = 0; phack[1] = 1.f/256.f;

    if (!m->texid[globalpal]) m->texid[globalpal] = gloadtex(m->mytex,m->mytexx,m->mytexy,m->is8bit,globalpal);
    else bglBindTexture(GL_TEXTURE_2D,m->texid[globalpal]);
    bglBegin(GL_QUADS);
    for (i=0,fi=0; i<m->qcnt; i++)
    {
        if (i == m->qfacind[fi]) { f = clut[fi++]; bglColor4f(pc[0]*f,pc[1]*f,pc[2]*f,pc[3]*f); }
        vptr = &m->quad[i].v[0];

        xx = vptr[0].x+vptr[2].x;
        yy = vptr[0].y+vptr[2].y;
        zz = vptr[0].z+vptr[2].z;

        for (j=0; j<4; j++)
        {
#if (VOXBORDWIDTH == 0)
            bglTexCoord2f(((float)vptr[j].u)*ru+uhack[vptr[j].u!=vptr[0].u],
                          ((float)vptr[j].v)*rv+vhack[vptr[j].v!=vptr[0].v]);
#else
            bglTexCoord2f(((float)vptr[j].u)*ru,((float)vptr[j].v)*rv);
#endif
            fp.x = ((float)vptr[j].x) - phack[xx>vptr[j].x*2] + phack[xx<vptr[j].x*2];
            fp.y = ((float)vptr[j].y) - phack[yy>vptr[j].y*2] + phack[yy<vptr[j].y*2];
            fp.z = ((float)vptr[j].z) - phack[zz>vptr[j].z*2] + phack[zz<vptr[j].z*2];
            bglVertex3fv((float *)&fp);
        }
    }
    bglEnd();

    //------------
    bglDisable(GL_CULL_FACE);
    bglPopAttrib();
    if (tspr->cstat&1024)
    {
        bglDepthFunc(GL_LESS); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS
        bglDepthRange(0.0,0.99999);
    }
    bglLoadIdentity();
    return 1;
}

//---------------------------------------- VOX LIBRARY ENDS ----------------------------------------
//--------------------------------------- MD LIBRARY BEGINS  ---------------------------------------

mdmodel_t *mdload(const char *filnam)
{
    mdmodel_t *vm;
    int32_t fil;
    int32_t i;

    vm = (mdmodel_t*)voxload(filnam); if (vm) return(vm);

    fil = kopen4load((char *)filnam,0); if (fil < 0) return(0);
    kread(fil,&i,4); klseek(fil,0,SEEK_SET);
    switch (B_LITTLE32(i))
    {
    case 0x32504449:
//        initprintf("Warning: model '%s' is version IDP2; wanted version IDP3\n",filnam);
        vm = (mdmodel_t*)md2load(fil,filnam);
        break; //IDP2
    case 0x33504449:
        vm = (mdmodel_t*)md3load(fil);
        break; //IDP3
    default:
        vm = (mdmodel_t*)0; break;
    }
    kclose(fil);
    if (vm && !md3postload((md3model_t*)vm)) {
        mdfree(vm);
        vm = (mdmodel_t*)0;
    }
    return(vm);
}

int32_t mddraw(spritetype *tspr)
{
    mdmodel_t *vm;
    int32_t i;

    if (r_vbos && (r_vbocount > allocvbos))
    {
        indexvbos = Brealloc(indexvbos, sizeof(GLuint) * r_vbocount);
        vertvbos = Brealloc(vertvbos, sizeof(GLuint) * r_vbocount);

        bglGenBuffersARB(r_vbocount - allocvbos, &(indexvbos[allocvbos]));
        bglGenBuffersARB(r_vbocount - allocvbos, &(vertvbos[allocvbos]));

        i = allocvbos;
        while (i < r_vbocount)
        {
            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexvbos[i]);
            bglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, maxmodeltris * 3 * sizeof(uint16_t), NULL, GL_STREAM_DRAW_ARB);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, vertvbos[i]);
            bglBufferDataARB(GL_ARRAY_BUFFER_ARB, maxmodelverts * sizeof(point3d), NULL, GL_STREAM_DRAW_ARB);
            i++;
        }

        bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

        allocvbos = r_vbocount;
    }

    if (maxmodelverts > allocmodelverts)
    {
        point3d *vl = (point3d *)Brealloc(vertlist,sizeof(point3d)*maxmodelverts);
        if (!vl) { OSD_Printf("ERROR: Not enough memory to allocate %d vertices!\n",maxmodelverts); return 0; }
        vertlist = vl;
        allocmodelverts = maxmodelverts;
    }

    vm = models[tile2model[Ptile2tile(tspr->picnum,(tspr->owner >= MAXSPRITES) ? tspr->pal : sprite[tspr->owner].pal)].modelid];
    if (vm->mdnum == 1) { return voxdraw((voxmodel_t *)vm,tspr); }
    if (vm->mdnum == 3) { return md3draw((md3model_t *)vm,tspr); }
    return 0;
}

void mdfree(mdmodel_t *vm)
{
    if (vm->mdnum == 1) { voxfree((voxmodel_t *)vm); return; }
    if (vm->mdnum == 2 || vm->mdnum == 3) { md3free((md3model_t *)vm); return; }
}

#endif

//---------------------------------------- MD LIBRARY ENDS  ----------------------------------------
