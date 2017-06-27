#ifdef USE_OPENGL

#include "baselayer.h"
#include "build.h"
#include "lz4.h"
#include "hightile.h"
#include "polymost.h"
#include "texcache.h"
#include "dxtfilter.h"
#include "scriptfile.h"
#include "xxhash.h"
#include "kplib.h"

#define CLEAR_GL_ERRORS() while(bglGetError() != GL_NO_ERROR) { }
#define TEXCACHE_FREEBUFS() { Bfree(pic), Bfree(packbuf), Bfree(midbuf); }

globaltexcache texcache;

char TEXCACHEFILE[BMAX_PATH] = "textures";

static const char *texcache_errors[TEXCACHEERRORS] = {
    "no error",
    "out of memory!",
    "read too few bytes from cache file",
    "dedxtfilter failed",
    "bglCompressedTexImage2DARB failed",
    "bglGetTexLevelParameteriv failed",
};

static pthtyp *texcache_tryart(int32_t const dapicnum, int32_t const dapalnum, int32_t const dashade, int32_t const dameth)
{
    const int32_t j = dapicnum&(GLTEXCACHEADSIZ-1);
    pthtyp *pth;
    int32_t tintpalnum = -1;
    int32_t searchpalnum = dapalnum;
    polytintflags_t const tintflags = hictinting[dapalnum].f;

    if (tintflags & (HICTINT_USEONART|HICTINT_ALWAYSUSEART))
    {
        tintpalnum = dapalnum;
        if (!(tintflags & HICTINT_APPLYOVERPALSWAP))
            searchpalnum = 0;
    }

    // load from art
    for (pth=texcache.list[j]; pth; pth=pth->next)
        if (pth->picnum == dapicnum && pth->palnum == dapalnum && pth->shade == dashade &&
                (pth->flags & (PTH_CLAMPED | PTH_HIGHTILE | PTH_NOTRANSFIX)) ==
                    (TO_PTH_CLAMPED(dameth) | TO_PTH_NOTRANSFIX(dameth)) &&
                polymost_want_npotytex(dameth, tilesiz[dapicnum].y) == !!(pth->flags&PTH_NPOTWALL)
           )
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

    gloadtile_art(dapicnum, searchpalnum, tintpalnum, dashade, dameth, pth, 1);

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

    if (!r_usetileshades || (globalflags & GLOBAL_NO_GL_TILESHADES) || getrendermode() != REND_POLYMOST)
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
    const int32_t checkcachepal = (tintflags & HICTINT_IN_MEMORY) || ((tintflags & HICTINT_APPLYOVERALTPAL) && si->palnum > 0) ? dapalnum : si->palnum;

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

    int32_t tilestat =
    gloadtile_hi(dapicnum, dapalnum, drawingskybox, si, dameth, pth, 1, (checktintpal > 0) ? 0 : tintflags);

    if (!tilestat)
    {
        pth->next = texcache.list[j];
        pth->palnum = checkcachepal;
        texcache.list[j] = pth;
        return pth;
    }

    if (tilestat == -2)  // bad filename
        hicclearsubst(dapicnum, dapalnum);

    Bfree(pth);

    return (drawingskybox || hicprecaching) ? NULL : texcache_tryart(dapicnum, dapalnum, dashade, dameth);
}

static void texcache_closefiles(void)
{
    if (texcache.handle != -1)
    {
        Bclose(texcache.handle);
        texcache.handle = -1;
    }
    MAYBE_FCLOSE_AND_NULL(texcache.index);
}

void texcache_freeptrs(void)
{
    texcache.entrybufsiz = 0;

    if (!texcache.entries)
        return;

    for (bssize_t i = 0; i < texcache.numentries; i++)
        if (texcache.entries[i])
        {
            for (bssize_t ii = texcache.numentries - 1; ii >= 0; ii--)
                if (i != ii && texcache.entries[ii] == texcache.entries[i])
                {
                    /*OSD_Printf("removing duplicate cacheptr %d\n",ii);*/
                    texcache.entries[ii] = NULL;
                }

            DO_FREE_AND_NULL(texcache.entries[i]->name);
            DO_FREE_AND_NULL(texcache.entries[i]);
        }

    DO_FREE_AND_NULL(texcache.entries);
}

static inline void texcache_clearmemcache(void)
{
    DO_FREE_AND_NULL(texcache.buf);
    texcache.memsize = -1;
}

void texcache_syncmemcache(void)
{
    int32_t len = Bfilelength(texcache.handle);

    if (!texcache.buf || texcache.handle == -1 || len <= (int32_t)texcache.memsize)
        return;

    texcache.buf = (uint8_t *)Brealloc(texcache.buf, len);

    if (!texcache.buf)
    {
        texcache_clearmemcache();
        initprintf("Failed syncing memcache!\n");
        glusememcache = 0;
    }
    else
    {
        initprintf("Syncing memcache\n");
        Blseek(texcache.handle, texcache.memsize, BSEEK_SET);
        if (Bread(texcache.handle, texcache.buf + texcache.memsize, len - texcache.memsize) != (bssize_t)(len-texcache.memsize))
        {
            initprintf("Failed reading texcache into memcache!\n");
            texcache_clearmemcache();
            glusememcache = 0;
        }
        else
            texcache.memsize = len;
    }
}

void texcache_init(void)
{
    if (!texcache.index)
        texcache.handle = -1;

    texcache_closefiles();
    texcache_clearmemcache();
    texcache_freeptrs();

    texcache.current = texcache.first = (texcacheindex *)Xcalloc(1, sizeof(texcacheindex));
    texcache.numentries = 0;

    //    Bmemset(&firstcacheindex, 0, sizeof(texcacheindex));
    //    Bmemset(&cacheptrs[0], 0, sizeof(cacheptrs));

    texcache.hashes.size = TEXCACHEHASHSIZE;
    hash_init(&texcache.hashes);
}

static void texcache_deletefiles(void)
{
    unlink(TEXCACHEFILE);
    Bstrcpy(ptempbuf, TEXCACHEFILE);
    Bstrcat(ptempbuf, ".cache");
    unlink(ptempbuf);
}

int32_t texcache_enabled(void)
{
#if defined EDUKE32_GLES || !defined USE_GLEXT
    return 0;
#else
    if (!glinfo.texcompr || !glusetexcompr || !glusetexcache)
        return 0;

    if (!texcache.index || texcache.handle < 0)
    {
        OSD_Printf("Warning: no active cache!\n");
        return 0;
    }

    return 1;
#endif
}

void texcache_openfiles(void)
{
    Bstrcpy(ptempbuf, TEXCACHEFILE);
    Bstrcat(ptempbuf, ".cache");

    texcache.index      = Bfopen(ptempbuf, "at+");
    texcache.handle = Bopen(TEXCACHEFILE, BO_BINARY | BO_CREAT | BO_APPEND | BO_RDWR, BS_IREAD | BS_IWRITE);

    if (!texcache.index || texcache.handle < 0)
    {
        initprintf("Unable to open cache file \"%s\" or \"%s\": %s\n", TEXCACHEFILE, ptempbuf, strerror(errno));
        texcache_closefiles();
        glusetexcache = 0;
        return;
    }

    Bfseek(texcache.index, 0, BSEEK_END);
    if (!Bftell(texcache.index))
    {
        Brewind(texcache.index);
        Bfprintf(texcache.index,"// automatically generated by the engine, DO NOT MODIFY!\n");
    }
    else Brewind(texcache.index);

    initprintf("Opened \"%s\" as cache file\n", TEXCACHEFILE);
}


void texcache_checkgarbage(void)
{
    if (!texcache_enabled())
        return;

    texcache.current = texcache.first;

    int32_t bytes = 0;

    while (texcache.current->next)
    {
        bytes += texcache.current->len;
        texcache.current = texcache.current->next;
    }

    bytes = Blseek(texcache.handle, 0, BSEEK_END)-bytes;

    if (bytes)
        initprintf("Cache contains %d bytes of garbage data\n", bytes);
}

void texcache_invalidate(void)
{
#ifdef DEBUGGINGAIDS
    OSD_Printf("texcache_invalidate()\n");
#endif
    r_downsizevar = r_downsize; // update the cvar representation when the menu changes r_downsize

    polymost_glreset();

    texcache_init();
    texcache_deletefiles();
    texcache_openfiles();
}

int texcache_loadoffsets(void)
{
    Bstrcpy(ptempbuf, TEXCACHEFILE);
    Bstrcat(ptempbuf, ".cache");
    scriptfile *script = scriptfile_fromfile(ptempbuf);

    if (!script) return -1;

    int32_t foffset, fsize;
    char *fname;

    while (!scriptfile_eof(script))
    {
        if (scriptfile_getstring(script, &fname)) break;	// hashed filename
        if (scriptfile_getnumber(script, &foffset)) break;	// offset in cache
        if (scriptfile_getnumber(script, &fsize)) break;	// size

        int const i = hash_find(&texcache.hashes,fname);

        if (i > -1)
        {
            // update an existing entry
            texcacheindex *t = texcache.entries[i];
            t->offset = foffset;
            t->len = fsize;
            /*initprintf("%s %d got a match for %s offset %d\n",__FILE__, __LINE__, fname,foffset);*/
        }
        else
        {
            texcacheindex * const index = texcache.current;

            index->name   = Xstrdup(fname);
            index->offset = foffset;
            index->len    = fsize;
            index->next   = (texcacheindex *) Xcalloc(1, sizeof(texcacheindex));
            hash_add(&texcache.hashes, fname, texcache.numentries, 1);
            if (++texcache.numentries > texcache.entrybufsiz)
            {
                texcache.entrybufsiz += 512;
                texcache.entries = (texcacheindex **) Xrealloc(texcache.entries, sizeof(intptr_t) * texcache.entrybufsiz);
            }
            texcache.entries[texcache.numentries-1] = texcache.current;
            texcache.current = index->next;
        }
    }

    scriptfile_close(script);
    return 0;
}

// Read from on-disk texcache or its in-memory cache.
int texcache_readdata(void *outBuf, int32_t len)
{
    const int32_t ofilepos = texcache.pos;

    texcache.pos += len;

    if (texcache.buf && texcache.memsize >= ofilepos + len)
    {
        //        initprintf("using memcache!\n");
        Bmemcpy(outBuf, texcache.buf + ofilepos, len);
        return 0;
    }

    if (Blseek(texcache.handle, ofilepos, BSEEK_SET) != ofilepos ||
        Bread(texcache.handle, outBuf, len) < len)
        return 1;

    return 0;
}

char const * texcache_calcid(char *outbuf, const char *filename, const int32_t len, const int32_t dameth, const char effect)
{
    // Assert that BMAX_PATH is a multiple of 4 so that struct texcacheid_t
    // gets no padding inserted by the compiler.
    EDUKE32_STATIC_ASSERT((BMAX_PATH & 3) == 0);

    struct texcacheid_t {
        int32_t len, method;
        char effect, name[BMAX_PATH+3];  // +3: pad to a multiple of 4
    } id = { len, dameth, effect, "" };

    Bstrcpy(id.name, filename);

    size_t const fnlen = Bstrlen(filename);
    while (Bstrlen(id.name) < BMAX_PATH - fnlen)
        Bstrcat(id.name, filename);

    Bsprintf(outbuf, "%08x%08x%08x",
             XXH32((uint8_t *)id.name, fnlen, TEXCACHEMAGIC[3]),
             XXH32((uint8_t *)id.name, Bstrlen(id.name), TEXCACHEMAGIC[3]),
             XXH32((uint8_t *)&id, sizeof(struct texcacheid_t), TEXCACHEMAGIC[3]));

    return outbuf;
}

#define FAIL(x) { err = x; goto failure; }

// returns 1 on success
int texcache_readtexheader(char const * cacheid, texcacheheader *head, int32_t modelp)
{
    if (!texcache_enabled())
        return 0;

    int32_t i = hash_find(&texcache.hashes, cacheid);

    if (i < 0 || !texcache.entries[i])
        return 0;  // didn't find it

    texcache.pos = texcache.entries[i]->offset;
//    initprintf("%s %d got a match for %s offset %d\n",__FILE__, __LINE__, cachefn,offset);

    int err = 0;

    if (texcache_readdata(head, sizeof(texcacheheader)))
        FAIL(0);

    if (Bmemcmp(head->magic, TEXCACHEMAGIC, 4))
        FAIL(1);

    // native (little-endian) -> internal
    head->xdim    = B_LITTLE32(head->xdim);
    head->ydim    = B_LITTLE32(head->ydim);
    head->flags   = B_LITTLE32(head->flags);
    head->quality = B_LITTLE32(head->quality);

    if (modelp && head->quality != r_downsize)
        FAIL(2);
    if ((head->flags & CACHEAD_COMPRESSED) && glusetexcache != 2)
        FAIL(3);
    if (!(head->flags & CACHEAD_COMPRESSED) && glusetexcache == 2)
        FAIL(4);

    // handle nodownsize
    if (!modelp && !(head->flags & CACHEAD_NODOWNSIZE) && head->quality != r_downsize)
        return 0;

    if (gltexmaxsize && (head->xdim > (1<<gltexmaxsize) || head->ydim > (1<<gltexmaxsize)))
        FAIL(5);
    if (!glinfo.texnpot && (head->flags & CACHEAD_NONPOW2))
        FAIL(6);

    return 1;

failure:
    {
        static const char *error_msgs[] = {
            "failed reading texture cache header",  // 0
            "header magic string doesn't match",  // 1
            "r_downsize doesn't match",  // 2  (skins only)
            "compression doesn't match: cache contains compressed tex",  // 3
            "compression doesn't match: cache contains uncompressed tex",  // 4
            "texture in cache exceeds maximum supported size",  // 5
            "texture in cache has non-power-of-two size, unsupported",  // 6
        };

        initprintf("%s cache miss: %s\n", modelp?"Skin":"Texture", error_msgs[err]);
    }

    return 0;
}

#undef READTEXHEADER_FAILURE

#if defined USE_GLEXT && !defined EDUKE32_GLES

void texcache_prewritetex(texcacheheader *head)
{
    Bmemcpy(head->magic, TEXCACHEMAGIC, 4);   // sizes are set by caller

    if (glusetexcache == 2)
        head->flags |= CACHEAD_COMPRESSED;

    // native -> external (little-endian)
    head->xdim    = B_LITTLE32(head->xdim);
    head->ydim    = B_LITTLE32(head->ydim);
    head->flags   = B_LITTLE32(head->flags);
    head->quality = B_LITTLE32(head->quality);
}

#define WRITEX_FAIL_ON_ERROR() if (bglGetError() != GL_NO_ERROR) goto failure

void texcache_writetex_fromdriver(char const * const cacheid, texcacheheader *head)
{
    if (!texcache_enabled()) return;

    GLint gi = GL_FALSE;
    bglGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &gi);
    if (gi != GL_TRUE)
    {
        static GLint glGetTexLevelParameterivOK = GL_TRUE;
        if (glGetTexLevelParameterivOK == GL_TRUE)
        {
            OSD_Printf("Error: glGetTexLevelParameteriv returned GL_FALSE!\n");
            glGetTexLevelParameterivOK = GL_FALSE;
        }
        return;
    }

    texcache_prewritetex(head);
    Blseek(texcache.handle, 0, BSEEK_END);
    size_t const offset = Blseek(texcache.handle, 0, BSEEK_CUR);

    texcachepicture pict;

    char *pic     = nullptr;
    char *packbuf = nullptr;
    void *midbuf  = nullptr;
    size_t alloclen = 0;

    //    OSD_Printf("Caching %s, offset 0x%x\n", cachefn, offset);
    if (Bwrite(texcache.handle, head, sizeof(texcacheheader)) != sizeof(texcacheheader)) goto failure;

    CLEAR_GL_ERRORS();

    for (uint32_t level = 0, padx = 0, pady = 0; level == 0 || (padx > 1 || pady > 1); ++level)
    {
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_ARB, &gi);
        WRITEX_FAIL_ON_ERROR();
        if (gi != GL_TRUE)
            goto failure;  // an uncompressed mipmap

        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_INTERNAL_FORMAT, &gi);
        WRITEX_FAIL_ON_ERROR();

#if defined __APPLE__ && defined POLYMER
        if (pr_ati_textureformat_one && gi == 1) gi = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
#endif
        // native -> external (little endian)
        pict.format = B_LITTLE32(gi);

        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &gi);
        WRITEX_FAIL_ON_ERROR();
        padx      = gi;
        pict.xdim = B_LITTLE32(gi);

        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &gi);
        WRITEX_FAIL_ON_ERROR();
        pady      = gi;
        pict.ydim = B_LITTLE32(gi);

        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_BORDER, &gi);
        WRITEX_FAIL_ON_ERROR();
        pict.border = B_LITTLE32(gi);

        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_DEPTH, &gi);
        WRITEX_FAIL_ON_ERROR();
        pict.depth = B_LITTLE32(gi);

        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &gi);
        WRITEX_FAIL_ON_ERROR();
        uint32_t miplen = gi;
        pict.size       = B_LITTLE32(gi);

        if (alloclen < miplen)
        {
            alloclen = miplen;
            pic      = (char *)Xrealloc(pic, miplen);
            packbuf  = (char *)Xrealloc(packbuf, miplen);
            midbuf   = (void *)Xrealloc(midbuf, miplen);
        }

        bglGetCompressedTexImageARB(GL_TEXTURE_2D, level, pic);
        WRITEX_FAIL_ON_ERROR();

        if (Bwrite(texcache.handle, &pict, sizeof(texcachepicture)) != sizeof(texcachepicture)) goto failure;
        if (dxtfilter(texcache.handle, &pict, pic, midbuf, packbuf, miplen)) goto failure;
    }

    texcache_postwritetex(cacheid, offset);
    TEXCACHE_FREEBUFS();
    return;

failure:
    initprintf("ERROR: cache failure!\n");
    texcache.current->offset = 0;
    Bfree(texcache.current->name);
    TEXCACHE_FREEBUFS();
}

#undef WRITEX_FAIL_ON_ERROR

void texcache_postwritetex(char const * const cacheid, int32_t const offset)
{
    int32_t const i = hash_find(&texcache.hashes, cacheid);

    texcacheindex *t;

    if (i > -1)
    {
        // update an existing entry
        t = texcache.entries[i];

        t->offset = offset;
        t->len    = Blseek(texcache.handle, 0, BSEEK_CUR) - t->offset;
        /*initprintf("%s %d got a match for %s offset %d\n",__FILE__, __LINE__, cachefn,offset);*/
    }
    else
    {
        t = texcache.current;

        Bfree(t->name);
        t->name   = Xstrdup(cacheid);
        t->offset = offset;
        t->len    = Blseek(texcache.handle, 0, BSEEK_CUR) - t->offset;
        t->next   = (texcacheindex *)Xcalloc(1, sizeof(texcacheindex));

        hash_add(&texcache.hashes, cacheid, texcache.numentries, 0);

        if (++texcache.numentries > texcache.entrybufsiz)
        {
            texcache.entrybufsiz += 512;
            texcache.entries = (texcacheindex **)Xrealloc(texcache.entries, sizeof(intptr_t) * texcache.entrybufsiz);
        }

        texcache.entries[texcache.numentries - 1] = t;
        texcache.current = t->next;
    }

    if (texcache.index)
    {
        fseek(texcache.index, 0, BSEEK_END);
        Bfprintf(texcache.index, "%s %d %d\n", t->name, t->offset, t->len);
    }
    else
        OSD_Printf("wtf?\n");
}

#endif

static void texcache_setuptexture(int32_t *doalloc, GLuint *glpic)
{
    if (*doalloc&1)
    {
        bglGenTextures(1, glpic);  //# of textures (make OpenGL allocate structure)
        *doalloc |= 2;	// prevents bglGenTextures being called again if we fail in here
    }

    bglBindTexture(GL_TEXTURE_2D, *glpic);
}

static int32_t texcache_loadmips(const texcacheheader *head, GLenum *glerr)
{
    texcachepicture pict;

    char *pic     = nullptr;
    char *packbuf = nullptr;
    void *midbuf  = nullptr;

    int32_t alloclen=0;

#if !defined USE_GLEXT && defined EDUKE32_GLES
    UNREFERENCED_PARAMETER(glerr);
    UNREFERENCED_PARAMETER(head);
#endif

    for (bssize_t level = 0; level==0 || (pict.xdim > 1 || pict.ydim > 1); level++)
    {
        if (texcache_readdata(&pict, sizeof(texcachepicture)))
        {
            TEXCACHE_FREEBUFS();
            return TEXCACHERR_BUFFERUNDERRUN;
        }

        // external (little endian) -> native
        pict.size   = B_LITTLE32(pict.size);
        pict.format = B_LITTLE32(pict.format);
        pict.xdim   = B_LITTLE32(pict.xdim);
        pict.ydim   = B_LITTLE32(pict.ydim);
        pict.border = B_LITTLE32(pict.border);
        pict.depth  = B_LITTLE32(pict.depth);

        if (alloclen < pict.size)
        {
            alloclen = pict.size;
            pic      = (char *)Xrealloc(pic, pict.size);
            packbuf  = (char *)Xrealloc(packbuf, pict.size + 16);
            midbuf   = (void *)Xrealloc(midbuf, pict.size);
        }

#if defined USE_GLEXT && !defined EDUKE32_GLES
        if (dedxtfilter(texcache.handle, &pict, pic, midbuf, packbuf, (head->flags & CACHEAD_COMPRESSED) != 0))
        {
            TEXCACHE_FREEBUFS();
            return TEXCACHERR_DEDXT;
        }

        bglCompressedTexImage2DARB(GL_TEXTURE_2D, level, pict.format, pict.xdim, pict.ydim, pict.border, pict.size, pic);
        if ((*glerr=bglGetError()) != GL_NO_ERROR)
        {
            TEXCACHE_FREEBUFS();
            return TEXCACHERR_COMPTEX;
        }

        GLint format;
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_INTERNAL_FORMAT, &format);
        if ((*glerr = bglGetError()) != GL_NO_ERROR)
        {
            TEXCACHE_FREEBUFS();
            return TEXCACHERR_GETTEXLEVEL;
        }

        if (pict.format != format)
        {
            OSD_Printf("gloadtile_cached: invalid texture cache file format %d %d\n", pict.format, format);
            TEXCACHE_FREEBUFS();
            return -1;
        }
#endif
    }

    TEXCACHE_FREEBUFS();
    return 0;
}

int32_t texcache_loadskin(const texcacheheader *head, int32_t *doalloc, GLuint *glpic, vec2_t *siz)
{
    int32_t err=0;
    GLenum glerr=GL_NO_ERROR;

    texcache_setuptexture(doalloc, glpic);

    siz->x = head->xdim;
    siz->y = head->ydim;

    CLEAR_GL_ERRORS();

    if ((err = texcache_loadmips(head, &glerr)))
    {
        if (err > 0)
            initprintf("texcache_loadskin: %s  (glerr=%x)\n", texcache_errors[err], glerr);

        return -1;
    }

    return 0;
}

int32_t texcache_loadtile(const texcacheheader *head, int32_t *doalloc, pthtyp *pth)
{
    int32_t err   = 0;
    GLenum  glerr = GL_NO_ERROR;

    texcache_setuptexture(doalloc, &pth->glpic);

    pth->siz.x = head->xdim;
    pth->siz.y = head->ydim;

    CLEAR_GL_ERRORS();

    if ((err = texcache_loadmips(head, &glerr)))
    {
        if (err > 0)
            initprintf("texcache_loadtile: %s  (glerr=%x)\n", texcache_errors[err], glerr);

        return -1;
    }

    return 0;
}

void texcache_setupmemcache(void)
{
    if (!glusememcache || !texcache_enabled())
        return;

    texcache.memsize = Bfilelength(texcache.handle);

    if (texcache.memsize <= 0)
        return;

    texcache.buf = (uint8_t *)Brealloc(texcache.buf, texcache.memsize);

    if (!texcache.buf)
    {
        initprintf("Failed allocating %d bytes for memcache!\n", (int)texcache.memsize);
        texcache_clearmemcache();
        glusememcache = 0;
        return;
    }

    if (Bread(texcache.handle, texcache.buf, texcache.memsize) != (bssize_t)texcache.memsize)
    {
        initprintf("Failed reading texcache into memcache!\n");
        texcache_clearmemcache();
        glusememcache = 0;
    }
}

#endif
