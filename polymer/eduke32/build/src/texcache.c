#ifdef USE_OPENGL

#include "build.h"
#include "quicklz.h"
#include "hightile.h"
#include "polymost.h"
#include "texcache.h"
#include "dxtfilter.h"
#include "scriptfile.h"
#include "crc32.h"

#define CLEAR_GL_ERRORS() while(bglGetError() != GL_NO_ERROR) { }
#define REALLOC_OR_FAIL(ptr, size, type) { ptr = (type *)Brealloc(ptr, size); if (!ptr) goto failure; }
#define TEXCACHE_FREEBUFS() { Bfree(pic), Bfree(packbuf), Bfree(midbuf); }

globaltexcache texcache;

char TEXCACHEFILE[BMAX_PATH] = "textures";

static const char *texcache_errorstr[TEXCACHEERRORS] = {
    "no error",
    "out of memory!",
    "read too few bytes from cache file",
    "dedxtfilter failed",
    "bglCompressedTexImage2DARB failed",
    "bglGetTexLevelParameteriv failed",
};

pthtyp *texcache_fetch(int32_t dapicnum, int32_t dapalnum, int32_t dashade, int32_t dameth)
{
    int32_t i, j;
    hicreplctyp *si;
    pthtyp *pth, *pth2;

    j = (dapicnum&(GLTEXCACHEADSIZ-1));
    
    if (getrendermode() != REND_POLYMOST || !r_usetileshades) dashade = 0;

    si = usehightile ? hicfindsubst(dapicnum,dapalnum,drawingskybox) : NULL;

    if (!si)
    {
        if (drawingskybox || dapalnum >= (MAXPALOOKUPS - RESERVEDPALS)) return NULL;
        goto tryart;
    }

    /* if palette > 0 && replacement found
     *    no effects are applied to the texture
     * else if palette > 0 && no replacement found
     *    effects are applied to the palette 0 texture if it exists
     */

    // load a replacement
    for (pth=texcache.list[j]; pth; pth=pth->next)
    {
        if (pth->picnum == dapicnum && pth->palnum == si->palnum &&
                (si->palnum>0 ? 1 : (pth->effects == hictinting[dapalnum].f)) &&
                (pth->flags & (1+2+4)) == (((dameth&4)>>2)+2+((drawingskybox>0)<<2)) &&
                (drawingskybox>0 ? (pth->skyface == drawingskybox) : 1)
           )
        {
            if (pth->flags & 128)
            {
                pth->flags &= ~128;
                if (gloadtile_hi(dapicnum,dapalnum,drawingskybox,si,dameth,pth,0,
                                 (si->palnum>0) ? 0 : hictinting[dapalnum].f))    // reload tile
                {
                    if (drawingskybox) return NULL;
                    goto tryart;   // failed, so try for ART
                }
            }

            return(pth);
        }
    }


    pth = (pthtyp *)Bcalloc(1,sizeof(pthtyp));
    if (!pth) return NULL;

    // possibly fetch an already loaded multitexture :_)
    if (dapalnum >= (MAXPALOOKUPS - RESERVEDPALS))
        for (i = (GLTEXCACHEADSIZ - 1); i >= 0; i--)
            for (pth2=texcache.list[i]; pth2; pth2=pth2->next)
            {
                if ((pth2->hicr) && (pth2->hicr->filename) && (Bstrcasecmp(pth2->hicr->filename, si->filename) == 0))
                {
                    Bmemcpy(pth, pth2, sizeof(pthtyp));
                    pth->picnum = dapicnum;
                    pth->flags = ((dameth&4)>>2) + 2 + ((drawingskybox>0)<<2);
                    if (pth2->flags & 8) pth->flags |= 8; //hasalpha
                    pth->hicr = si;
                    pth->next = texcache.list[j];

                    texcache.list[j] = pth;
                    return(pth);
                }
            }

    if (gloadtile_hi(dapicnum,dapalnum,drawingskybox,si,dameth,pth,1, (si->palnum>0) ? 0 : hictinting[dapalnum].f))
    {
        Bfree(pth);
        if (drawingskybox) return NULL;
        goto tryart;   // failed, so try for ART
    }

    pth->palnum = si->palnum;
    pth->next = texcache.list[j];
    texcache.list[j] = pth;

    return(pth);

tryart:
    if (hicprecaching) return NULL;

    // load from art
    for (pth=texcache.list[j]; pth; pth=pth->next)
        if (pth->picnum == dapicnum && pth->palnum == dapalnum && pth->shade == dashade && 
                (pth->flags & (1+2)) == ((dameth&4)>>2)
           )
        {
            if (pth->flags & 128)
            {
                pth->flags &= ~128;
                if (gloadtile_art(dapicnum,dapalnum,dashade,dameth,pth,0))
                    return NULL; //reload tile (for animations)
            }

            return(pth);
        }

    pth = (pthtyp *)Bcalloc(1,sizeof(pthtyp));
    if (!pth) return NULL;

    if (gloadtile_art(dapicnum,dapalnum,dashade,dameth,pth,1))
    {
        Bfree(pth);
        return NULL;
    }

    pth->next = texcache.list[j];
    texcache.list[j] = pth;

    return(pth);
}

static void texcache_closefiles(void)
{
    if (texcache.filehandle != -1)
    {
        Bclose(texcache.filehandle);
        texcache.filehandle = -1;
    }

    if (texcache.index)
    {
        Bfclose(texcache.index);
        texcache.index = NULL;
    }
}

void texcache_freeptrs(void)
{
    int32_t i;

    for (i = texcache.numentries-1; i >= 0; i--)
        if (texcache.ptrs[i])
        {
            int32_t ii;
            for (ii = texcache.numentries-1; ii >= 0; ii--)
                if (i != ii && texcache.ptrs[ii] == texcache.ptrs[i])
                {
                    /*OSD_Printf("removing duplicate cacheptr %d\n",ii);*/
                    texcache.ptrs[ii] = NULL;
                }

                Bfree(texcache.ptrs[i]);
                texcache.ptrs[i] = NULL;
        }
}

void texcache_clearmemcache(void)
{
    Bfree(texcache.memcache.ptr);
    texcache.memcache.ptr = NULL;
    texcache.memcache.size = -1;
}

void texcache_syncmemcache(void)
{
    size_t len;

    if (!texcache.memcache.ptr || texcache.filehandle == -1 ||
            (bsize_t)Bfilelength(texcache.filehandle) <= texcache.memcache.size)
        return;

    len = Bfilelength(texcache.filehandle);

    texcache.memcache.ptr = (uint8_t *)Brealloc(texcache.memcache.ptr, len);

    if (!texcache.memcache.ptr)
    {
        texcache_clearmemcache();
        initprintf("Failed syncing memcache to texcache, disabling memcache.\n");
        texcache.memcache.noalloc = 1;
    }
    else
    {
        initprintf("Syncing memcache to texcache\n");
        Blseek(texcache.filehandle, texcache.memcache.size, BSEEK_SET);
        if (Bread(texcache.filehandle, texcache.memcache.ptr + texcache.memcache.size, len - texcache.memcache.size) != (bssize_t)(len-texcache.memcache.size))
        {
            initprintf("polymost_cachesync: Failed reading texcache into memcache!\n");
            texcache_clearmemcache();
            texcache.memcache.noalloc = 1;
        }
        else
        {
            texcache.memcache.size = len;
        }
    }
}

void texcache_init(void)
{
    if (!texcache.index)
        texcache.filehandle = -1;

    texcache_closefiles();
    texcache_clearmemcache();
    texcache_freeptrs();

    texcache.currentindex = texcache.firstindex = (texcacheindex *)Bcalloc(1, sizeof(texcacheindex));
    texcache.numentries = 0;

    //    Bmemset(&firstcacheindex, 0, sizeof(texcacheindex));
    //    Bmemset(&cacheptrs[0], 0, sizeof(cacheptrs));

    texcache.hashes.size = TEXCACHEHASHSIZE;
    hash_init(&texcache.hashes);
}

static void texcache_deletefiles(void)
{
    Bstrcpy(ptempbuf, TEXCACHEFILE);
    unlink(ptempbuf);
    Bstrcat(ptempbuf, ".cache");
    unlink(ptempbuf);
}

static int32_t texcache_enabled(void)
{
    if (!glusetexcompr || !glusetexcache) return 0;

    if (!glinfo.texcompr || !bglCompressedTexImage2DARB || !bglGetCompressedTexImageARB)
    {
        // lacking the necessary extensions to do this
        OSD_Printf("Warning: the GL driver lacks necessary functions to use caching\n");
        glusetexcache = 0;
        return 0;
    }

    if (!texcache.index || texcache.filehandle < 0)
    {
        OSD_Printf("Warning: no active cache!\n");
        return 0;
    }

    return 1;
}

void texcache_openfiles(void)
{
    Bstrcpy(ptempbuf,TEXCACHEFILE);
    Bstrcat(ptempbuf,".cache");
    texcache.index = Bfopen(ptempbuf, "at+");
    texcache.filehandle = Bopen(TEXCACHEFILE, BO_BINARY|BO_CREAT|BO_APPEND|BO_RDWR, BS_IREAD|BS_IWRITE);

    if (!texcache.index || texcache.filehandle < 0)
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
        Bfprintf(texcache.index,"// automatically generated by EDuke32, DO NOT MODIFY!\n");
    }
    else Brewind(texcache.index);

    initprintf("Opened \"%s\" as cache file\n", TEXCACHEFILE);
}


void texcache_checkgarbage(void)
{
    int32_t i = 0;

    if (!texcache_enabled())
        return;

    texcache.currentindex = texcache.firstindex;
    while (texcache.currentindex->next)
    {
        i += texcache.currentindex->len;
        texcache.currentindex = texcache.currentindex->next;
    }

    i = Blseek(texcache.filehandle, 0, BSEEK_END)-i;

    if (i)
        initprintf("Cache contains %d bytes of garbage data\n",i);
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

int32_t texcache_loadoffsets(void)
{
    int32_t foffset, fsize, i;
    char *fname;

    scriptfile *script;

    Bstrcpy(ptempbuf,TEXCACHEFILE);
    Bstrcat(ptempbuf,".cache");
    script = scriptfile_fromfile(ptempbuf);

    if (!script) return -1;

    while (!scriptfile_eof(script))
    {
        if (scriptfile_getstring(script, &fname)) break;	// hashed filename
        if (scriptfile_getnumber(script, &foffset)) break;	// offset in cache
        if (scriptfile_getnumber(script, &fsize)) break;	// size

        i = hash_find(&texcache.hashes,fname);
        if (i > -1)
        {
            // update an existing entry
            texcacheindex *t = texcache.ptrs[i];
            t->offset = foffset;
            t->len = fsize;
            /*initprintf("%s %d got a match for %s offset %d\n",__FILE__, __LINE__, fname,foffset);*/
        }
        else
        {
            Bstrncpyz(texcache.currentindex->name, fname, BMAX_PATH);
            texcache.currentindex->offset = foffset;
            texcache.currentindex->len = fsize;
            texcache.currentindex->next = (texcacheindex *)Bcalloc(1, sizeof(texcacheindex));
            hash_add(&texcache.hashes, fname, texcache.numentries, 1);
            texcache.ptrs[texcache.numentries++] = texcache.currentindex;
            texcache.currentindex = texcache.currentindex->next;
        }
    }

    scriptfile_close(script);
    return 0;
}

// Read from on-disk texcache or its in-memory cache.
int32_t texcache_readdata(void *dest, int32_t len)
{
    const int32_t ocachepos = texcache.filepos;

    texcache.filepos += len;

    if (texcache.memcache.ptr && texcache.memcache.size >= (bsize_t)ocachepos+len)
    {
        //        initprintf("using memcache!\n");
        Bmemcpy(dest, texcache.memcache.ptr+ocachepos, len);
        return 0;
    }

    if (Blseek(texcache.filehandle, ocachepos, BSEEK_SET) != ocachepos)
        return 1;

    if (Bread(texcache.filehandle, dest, len) < len)
        return 1;

    return 0;
}

static const char * texcache_calcid(char *cachefn, const char *fn, const int32_t len, const int32_t dameth, const char effect)
{
    struct texcacheid_t {
        int32_t len, method;
        char effect, name[BMAX_PATH];
    } id = { len, dameth, effect, "" };

    Bstrcpy(id.name, fn);

    while (Bstrlen(id.name) < BMAX_PATH - Bstrlen(fn))
        Bstrcat(id.name, fn);

    Bsprintf(cachefn, "%x%x%x",
        crc32once((uint8_t *)fn, Bstrlen(fn)),
        crc32once((uint8_t *)id.name, Bstrlen(id.name)),
        crc32once((uint8_t *)&id, sizeof(struct texcacheid_t)));
    
    return cachefn;
}

#define READTEXHEADER_FAILURE(x) { err = x; goto failure; }

// returns 1 on success
int32_t texcache_readtexheader(const char *fn, int32_t len, int32_t dameth, char effect,
                         texcacheheader *head, int32_t modelp)
{
    int32_t i, err = 0;
    char cachefn[BMAX_PATH];

    if (!texcache_enabled()) return 0;

    i = hash_find(&texcache.hashes, texcache_calcid(cachefn, fn, len, dameth, effect));

    if (i < 0 || !texcache.ptrs[i])
        return 0;  // didn't find it

    texcache.filepos = texcache.ptrs[i]->offset;
    //        initprintf("%s %d got a match for %s offset %d\n",__FILE__, __LINE__, cachefn,offset);


    if (texcache_readdata(head, sizeof(texcacheheader))) READTEXHEADER_FAILURE(0);

    if (Bmemcmp(head->magic, TEXCACHEMAGIC, 4)) READTEXHEADER_FAILURE(1);

    // native (little-endian) -> internal
    head->xdim = B_LITTLE32(head->xdim);
    head->ydim = B_LITTLE32(head->ydim);
    head->flags = B_LITTLE32(head->flags);
    head->quality = B_LITTLE32(head->quality);

    if (modelp && head->quality != r_downsize) READTEXHEADER_FAILURE(2);
    if ((head->flags & 4) && glusetexcache != 2) READTEXHEADER_FAILURE(3);
    if (!(head->flags & 4) && glusetexcache == 2) READTEXHEADER_FAILURE(4);

    // handle nocompress
    if (!modelp && !(head->flags & 8) && head->quality != r_downsize)
        return 0;

    if (gltexmaxsize && (head->xdim > (1<<gltexmaxsize) || head->ydim > (1<<gltexmaxsize))) READTEXHEADER_FAILURE(5);
    if (!glinfo.texnpot && (head->flags & 1)) READTEXHEADER_FAILURE(6);

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
#define WRITEX_FAIL_ON_ERROR() if (bglGetError() != GL_NO_ERROR) goto failure

void texcache_writetex(const char *fn, int32_t len, int32_t dameth, char effect, texcacheheader *head)
{
    char cachefn[BMAX_PATH];
    char *pic = NULL, *packbuf = NULL;
    void *midbuf = NULL;
    uint32_t alloclen=0, level;
    uint32_t padx=0, pady=0;
    GLint gi;
    int32_t offset = 0;

    if (!texcache_enabled()) return;

    gi = GL_FALSE;
    bglGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &gi);
    if (gi != GL_TRUE)
    {
        OSD_Printf("Error: glGetTexLevelParameteriv returned GL_FALSE!\n");
        return;
    }

    Blseek(texcache.filehandle, 0, BSEEK_END);

    offset = Blseek(texcache.filehandle, 0, BSEEK_CUR);
    //    OSD_Printf("Caching %s, offset 0x%x\n", cachefn, offset);

    Bmemcpy(head->magic, TEXCACHEMAGIC, 4);   // sizes are set by caller

    if (glusetexcache == 2) head->flags |= 4;

    // native -> external (little-endian)
    head->xdim = B_LITTLE32(head->xdim);
    head->ydim = B_LITTLE32(head->ydim);
    head->flags = B_LITTLE32(head->flags);
    head->quality = B_LITTLE32(head->quality);

    if (Bwrite(texcache.filehandle, head, sizeof(texcacheheader)) != sizeof(texcacheheader)) goto failure;

    CLEAR_GL_ERRORS();

    for (level = 0; level==0 || (padx > 1 || pady > 1); level++)
    {
        uint32_t miplen;
        texcachepicture pict;

        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_ARB, &gi); WRITEX_FAIL_ON_ERROR();
        if (gi != GL_TRUE) goto failure;   // an uncompressed mipmap
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_INTERNAL_FORMAT, &gi); WRITEX_FAIL_ON_ERROR();

#ifdef __APPLE__
        if (pr_ati_textureformat_one && gi == 1) gi = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
#endif
        // native -> external (little endian)
        pict.format = B_LITTLE32(gi);
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &gi); WRITEX_FAIL_ON_ERROR();
        padx = gi; pict.xdim = B_LITTLE32(gi);
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &gi); WRITEX_FAIL_ON_ERROR();
        pady = gi; pict.ydim = B_LITTLE32(gi);
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_BORDER, &gi); WRITEX_FAIL_ON_ERROR();
        pict.border = B_LITTLE32(gi);
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_DEPTH, &gi); WRITEX_FAIL_ON_ERROR();
        pict.depth = B_LITTLE32(gi);
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &gi); WRITEX_FAIL_ON_ERROR();
        miplen = gi; pict.size = B_LITTLE32(gi);

        if (alloclen < miplen)
        {
            REALLOC_OR_FAIL(pic, miplen, char);
            alloclen = miplen;
            REALLOC_OR_FAIL(packbuf, alloclen+400, char);
            REALLOC_OR_FAIL(midbuf, miplen, void);
        }

        bglGetCompressedTexImageARB(GL_TEXTURE_2D, level, pic); WRITEX_FAIL_ON_ERROR();

        if (Bwrite(texcache.filehandle, &pict, sizeof(texcachepicture)) != sizeof(texcachepicture)) goto failure;
        if (dxtfilter(texcache.filehandle, &pict, pic, midbuf, packbuf, miplen)) goto failure;
    }

    {
        texcacheindex *t;
        int32_t i = hash_find(&texcache.hashes, texcache_calcid(cachefn, fn, len, dameth, effect));
        if (i > -1)
        {
            // update an existing entry
            t = texcache.ptrs[i];
            t->offset = offset;
            t->len = Blseek(texcache.filehandle, 0, BSEEK_CUR) - t->offset;
            /*initprintf("%s %d got a match for %s offset %d\n",__FILE__, __LINE__, cachefn,offset);*/
        }
        else
        {
            t = texcache.currentindex;
            Bstrcpy(t->name, cachefn);
            t->offset = offset;
            t->len = Blseek(texcache.filehandle, 0, BSEEK_CUR) - t->offset;
            t->next = (texcacheindex *)Bcalloc(1, sizeof(texcacheindex));

            hash_add(&texcache.hashes, cachefn, texcache.numentries, 0);
            texcache.ptrs[texcache.numentries++] = t;
            texcache.currentindex = t->next;
        }

        if (texcache.index)
        {
            fseek(texcache.index, 0, BSEEK_END);
            Bfprintf(texcache.index, "%s %d %d\n", t->name, t->offset, t->len);
        }
        else OSD_Printf("wtf?\n");
    }

    goto success;

failure:
    initprintf("ERROR: cache failure!\n");
    texcache.currentindex->offset = 0;
    Bmemset(texcache.currentindex->name,0,sizeof(texcache.currentindex->name));

success:
    TEXCACHE_FREEBUFS();
}

#undef WRITEX_FAIL_ON_ERROR

static void texcache_setuptexture(int32_t *doalloc, GLuint *glpic)
{
    if (*doalloc&1)
    {
        bglGenTextures(1,glpic);  //# of textures (make OpenGL allocate structure)
        *doalloc |= 2;	// prevents bglGenTextures being called again if we fail in here
    }
    bglBindTexture(GL_TEXTURE_2D,*glpic);
}

static int32_t texcache_loadmips(const texcacheheader *head, GLenum *glerr, int32_t *xsiz, int32_t *ysiz)
{
    int32_t level;
    texcachepicture pict;
    char *pic = NULL, *packbuf = NULL;
    void *midbuf = NULL;
    int32_t alloclen=0;

    for (level = 0; level==0 || (pict.xdim > 1 || pict.ydim > 1); level++)
    {
        GLint format;

        if (texcache_readdata(&pict, sizeof(texcachepicture)))
        {
            TEXCACHE_FREEBUFS();
            return TEXCACHERR_BUFFERUNDERRUN;
        }

        // external (little endian) -> native
        pict.size = B_LITTLE32(pict.size);
        pict.format = B_LITTLE32(pict.format);
        pict.xdim = B_LITTLE32(pict.xdim);
        pict.ydim = B_LITTLE32(pict.ydim);
        pict.border = B_LITTLE32(pict.border);
        pict.depth = B_LITTLE32(pict.depth);

        if (level == 0)
        { 
            if (xsiz) *xsiz = pict.xdim;
            if (ysiz) *ysiz = pict.ydim;
        }

        if (alloclen < pict.size)
        {
            REALLOC_OR_FAIL(pic, pict.size, char);
            alloclen = pict.size;
            REALLOC_OR_FAIL(packbuf, alloclen+16, char);
            REALLOC_OR_FAIL(midbuf, pict.size, void);
        }

        if (dedxtfilter(texcache.filehandle, &pict, pic, midbuf, packbuf, (head->flags&4)==4))
        {
            TEXCACHE_FREEBUFS();
            return TEXCACHERR_DEDXT;
        }

        bglCompressedTexImage2DARB(GL_TEXTURE_2D,level,pict.format,pict.xdim,pict.ydim,pict.border,pict.size,pic);
        if ((*glerr=bglGetError()) != GL_NO_ERROR)
        {
            TEXCACHE_FREEBUFS();
            return TEXCACHERR_COMPTEX;
        }

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
    }

    TEXCACHE_FREEBUFS();
    return 0;

failure:
    TEXCACHE_FREEBUFS();
    return TEXCACHERR_OUTOFMEMORY;
}

int32_t texcache_loadskin(const texcacheheader *head, int32_t *doalloc, GLuint *glpic, int32_t *xsiz, int32_t *ysiz)
{
    int32_t err=0;
    GLenum glerr=GL_NO_ERROR;

    texcache_setuptexture(doalloc, glpic);

    CLEAR_GL_ERRORS();

    if ((err = texcache_loadmips(head, &glerr, xsiz, ysiz)))
    {
        if (err > 0)
            initprintf("texcache_loadskin: %s  (glerr=%x)\n", texcache_errorstr[err], glerr);

        return -1;
    }

    return 0;
}

int32_t texcache_loadtile(const texcacheheader *head, int32_t *doalloc, pthtyp *pth)
{
    int32_t err=0;
    GLenum glerr=GL_NO_ERROR;

    texcache_setuptexture(doalloc, &pth->glpic);

    pth->sizx = head->xdim;
    pth->sizy = head->ydim;

    CLEAR_GL_ERRORS();

    if ((err = texcache_loadmips(head, &glerr, NULL, NULL)))
    {
        if (err > 0)
            initprintf("texcache_loadtile: %s  (glerr=%x)\n", texcache_errorstr[err], glerr);

        return -1;
    }

    return 0;
}

void texcache_setupmemcache(void)
{
    if (!glusememcache || texcache.memcache.noalloc || !texcache_enabled())
        return;

    texcache.memcache.size = Bfilelength(texcache.filehandle);

    if (texcache.memcache.size <= 0)
        return;

    texcache.memcache.ptr = (uint8_t *)Brealloc(texcache.memcache.ptr, texcache.memcache.size);

    if (!texcache.memcache.ptr)
    {
        initprintf("Failed allocating %d bytes for memcache, disabling memcache.\n", (int)texcache.memcache.size);
        texcache_clearmemcache();
        texcache.memcache.noalloc = 1;
        return;
    }

    if (Bread(texcache.filehandle, texcache.memcache.ptr, texcache.memcache.size) != (bssize_t)texcache.memcache.size)
    {
        initprintf("Failed reading texcache into memcache!\n");
        texcache_clearmemcache();
        texcache.memcache.noalloc = 1;
    }
}

#endif
