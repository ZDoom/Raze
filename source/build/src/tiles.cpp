
#include "compat.h"
#include "build.h"
#include "baselayer.h"
#include "engine_priv.h"
#include "cache1d.h"
#include "lz4.h"

void *pic = NULL;

// The tile file number (tilesXXX <- this) of each tile:
// 0 <= . < MAXARTFILES_BASE: tile is in a "base" ART file
// MAXARTFILES_BASE <= . < MAXARTFILES_TOTAL: tile is in a map-specific ART file
static uint8_t tilefilenum[MAXTILES];
EDUKE32_STATIC_ASSERT(MAXARTFILES_TOTAL <= 256);

static int32_t tilefileoffs[MAXTILES];

// Backup tilefilenum[] and tilefileoffs[]. These get allocated only when
// necessary (have per-map ART files).
static uint8_t *g_bakTileFileNum;
static int32_t *g_bakTileFileOffs;
static vec2s_t *g_bakTileSiz;
static picanm_t *g_bakPicAnm;
// NOTE: picsiz[] is not backed up, but recalculated when necessary.

//static int32_t artsize = 0;
static int32_t cachesize = 0;

static char artfilename[20];
static char mapartfilename[BMAX_PATH];  // map-specific ART file name
static int32_t mapartfnXXofs;  // byte offset to 'XX' (the number part) in the above
static int32_t artfil = -1, artfilnum, artfilplc;

////////// Per-map ART file loading //////////

// Some forward declarations.
static void set_picsiz(int32_t picnum);
static const char *E_GetArtFileName(int32_t tilefilei);
static int32_t E_ReadArtFileOfID(int32_t tilefilei);

static inline void clearmapartfilename(void)
{
    Bmemset(mapartfilename, 0, sizeof(mapartfilename));
    mapartfnXXofs = 0;
}

static inline void E_RecalcPicSiz(void)
{
    for (bssize_t i=0; i<MAXTILES; i++)
        set_picsiz(i);
}

#define RESTORE_MAPART_ARRAY(origar, bakar)                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        EDUKE32_STATIC_ASSERT(sizeof(origar[0]) == sizeof(bakar[0]));                                                  \
        Bmemcpy(origar, bakar, MAXUSERTILES * sizeof(origar[0]));                                                      \
        DO_FREE_AND_NULL(bakar);                                                                                       \
    \
} while (0)

// Allocate per-map ART backup array and back up the original!
#ifdef __cplusplus
template <typename origar_t, typename bakar_t>
static inline void ALLOC_MAPART_ARRAY(origar_t &origar, bakar_t &bakar)
{
    bakar = (bakar_t) Xmalloc(MAXUSERTILES*sizeof(origar[0]));
    Bmemcpy(bakar, origar, MAXUSERTILES*sizeof(origar[0]));
}
#else
#define ALLOC_MAPART_ARRAY(origar, bakar)                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        bakar = Xmalloc(MAXUSERTILES * sizeof(origar[0]));                                                             \
        Bmemcpy(bakar, origar, MAXUSERTILES * sizeof(origar[0]));                                                      \
    \
} while (0)
#endif

void E_MapArt_Clear(void)
{
    if (g_bakTileFileNum == NULL)
        return;  // per-map ART N/A

    clearmapartfilename();

    if (artfilnum >= MAXARTFILES_BASE)
    {
        kclose(artfil);

        artfil = -1;
        artfilnum = -1;
        artfilplc = 0L;
    }

    for (bssize_t i=0; i<MAXTILES; i++)
    {
        if (tilefilenum[i] >= MAXARTFILES_BASE)
        {
            // XXX: OK way to free it? Better: cache1d API. CACHE1D_FREE
            walock[i] = 1;
            waloff[i] = 0;
        }
    }

    // Restore original per-tile arrays
    RESTORE_MAPART_ARRAY(tilefilenum, g_bakTileFileNum);
    RESTORE_MAPART_ARRAY(tilefileoffs, g_bakTileFileOffs);
    RESTORE_MAPART_ARRAY(tilesiz, g_bakTileSiz);
    RESTORE_MAPART_ARRAY(picanm, g_bakPicAnm);

    E_RecalcPicSiz();
#ifdef USE_OPENGL
    gltexinvalidatetype(INVALIDATE_ART);
# ifdef POLYMER
    if (getrendermode() == REND_POLYMER)
        polymer_texinvalidate();
# endif
#endif
}

void E_MapArt_Setup(const char *filename)
{
    E_MapArt_Clear();

    if (Bstrlen(filename) + 7 >= sizeof(mapartfilename))
        return;

    Bstrcpy(mapartfilename, filename);
    append_ext_UNSAFE(mapartfilename, "_XX.art");
    mapartfnXXofs = Bstrlen(mapartfilename) - 6;

    // Check for first per-map ART file: if that one doesn't exist, don't load any.
    int32_t fil = kopen4load(E_GetArtFileName(MAXARTFILES_BASE), 0);

    if (fil == -1)
    {
        clearmapartfilename();
        return;
    }

    kclose(fil);

    // Allocate backup arrays.
    ALLOC_MAPART_ARRAY(tilefilenum, g_bakTileFileNum);
    ALLOC_MAPART_ARRAY(tilefileoffs, g_bakTileFileOffs);
    ALLOC_MAPART_ARRAY(tilesiz, g_bakTileSiz);
    ALLOC_MAPART_ARRAY(picanm, g_bakPicAnm);

    for (bssize_t i=MAXARTFILES_BASE; i<MAXARTFILES_TOTAL; i++)
    {
        int ret = E_ReadArtFileOfID(i);

        if (ret != 0)
        {
            // NOTE: i == MAXARTFILES_BASE && ret == -1 can only have happened
            // if the file was deleted between the above file existence check
            // and now.  Very cornerly... but I like my code to be prepared to
            // any eventuality.
            if (i == MAXARTFILES_BASE || ret != -1)
                E_MapArt_Clear();
            break;
        }
    }

    E_RecalcPicSiz();
#ifdef USE_OPENGL
    gltexinvalidatetype(INVALIDATE_ART);
# ifdef POLYMER
    if (getrendermode() == REND_POLYMER)
        polymer_texinvalidate();
# endif
#endif
}

//
// ART loading
//

void E_CreateDummyTile(int32_t const tile)
{
    faketile[tile>>3] |= pow2char[tile&7];
    DO_FREE_AND_NULL(faketiledata[tile]);
}

void E_CreateFakeTile(int32_t const tile, int32_t tsiz, char const * const buffer)
{
    int const compressed_tsiz = LZ4_compressBound(tsiz);
    faketiledata[tile] = (char *) Xrealloc(faketiledata[tile], compressed_tsiz);

    if ((tsiz = LZ4_compress_default(buffer, faketiledata[tile], tsiz, compressed_tsiz)) != -1)
    {
        faketiledata[tile] = (char *) Xrealloc(faketiledata[tile], tsiz);
        faketile[tile>>3] |= pow2char[tile&7];
    }
    else
    {
        DO_FREE_AND_NULL(faketiledata[tile]);
        faketile[tile>>3] &= ~pow2char[tile&7];
    }
}

void E_UndefineTile(int32_t const tile)
{
    tilesiz[tile].x = 0;
    tilesiz[tile].y = 0;
    picsiz[tile] = 0;

    // CACHE1D_FREE
    walock[tile] = 1;
    waloff[tile] = 0;

    DO_FREE_AND_NULL(faketiledata[tile]);
    faketile[tile>>3] &= ~pow2char[tile&7];

    Bmemset(&picanm[tile], 0, sizeof(picanm_t));

    vox_undefine(tile);

#ifdef USE_OPENGL
    for (ssize_t i=MAXPALOOKUPS-1; i>=0; --i)
        hicclearsubst(tile, i);

    md_undefinetile(tile);
#endif
}

static void set_picsiz(int32_t picnum)
{
    int j = 15;

    while ((j > 1) && (pow2long[j] > tilesiz[picnum].x))
        j--;
    picsiz[picnum] = j;

    j = 15;
    while ((j > 1) && (pow2long[j] > tilesiz[picnum].y))
        j--;
    picsiz[picnum] |= j<<4;
}

void set_tilesiz(int32_t picnum, int16_t dasizx, int16_t dasizy)
{
    tilesiz[picnum].x = dasizx;
    tilesiz[picnum].y = dasizy;

    set_picsiz(picnum);
}

int32_t tile_exists(int32_t picnum)
{
    if (waloff[picnum] == 0)
        loadtile(picnum);

    return (waloff[picnum] != 0 && tilesiz[picnum].x > 0 && tilesiz[picnum].y > 0);
}

int32_t E_ReadArtFileHeader(int32_t const fil, char const * const fn, artheader_t * const local)
{
    int32_t artversion;
    kread(fil, &artversion, 4); artversion = B_LITTLE32(artversion);
    if (artversion != 1)
    {
        initprintf("loadpics: Invalid art file version in %s\n", fn);
        kclose(fil);
        return 1;
    }

    int32_t numtiles_dummy;
    kread(fil, &numtiles_dummy, 4);

    kread(fil, &local->tilestart, 4); local->tilestart = B_LITTLE32(local->tilestart);
    kread(fil, &local->tileend, 4);   local->tileend   = B_LITTLE32(local->tileend);

    if ((uint32_t) local->tilestart >= MAXUSERTILES || (uint32_t) local->tileend >= MAXUSERTILES)
    {
        initprintf("loadpics: Invalid localtilestart or localtileend in %s\n", fn);
        kclose(fil);
        return 1;
    }
    if (local->tileend < local->tilestart)
    {
        initprintf("loadpics: localtileend < localtilestart in %s\n", fn);
        kclose(fil);
        return 1;
    }

    local->numtiles = (local->tileend-local->tilestart+1);

    return 0;
}

int32_t E_ReadArtFileHeaderFromBuffer(uint8_t const * const buf, artheader_t * const local)
{
    int const artversion = B_LITTLE32(B_UNBUF32(&buf[0]));
    if (EDUKE32_PREDICT_FALSE(artversion != 1))
    {
        initprintf("loadpics: Invalid art file version\n");
        return 1;
    }

    local->tilestart = B_LITTLE32(B_UNBUF32(&buf[8]));
    local->tileend   = B_LITTLE32(B_UNBUF32(&buf[12]));

    if (EDUKE32_PREDICT_FALSE((unsigned) local->tilestart >= MAXUSERTILES || (unsigned) local->tileend >= MAXUSERTILES))
    {
        initprintf("loadpics: Invalid localtilestart or localtileend\n");
        return 1;
    }
    if (EDUKE32_PREDICT_FALSE(local->tileend < local->tilestart))
    {
        initprintf("loadpics: localtileend < localtilestart\n");
        return 1;
    }

    local->numtiles = (local->tileend-local->tilestart+1);

    return 0;
}

int32_t E_CheckUnitArtFileHeader(uint8_t const * const buf, int32_t length)
{
    if (EDUKE32_PREDICT_FALSE(length <= ARTv1_UNITOFFSET))
        return -1;

    artheader_t local;
    if (EDUKE32_PREDICT_FALSE(E_ReadArtFileHeaderFromBuffer(buf, &local) != 0))
        return -2;

    if (EDUKE32_PREDICT_FALSE(local.numtiles != 1))
        return -3;

    return 0;
}

void E_ConvertARTv1picanmToMemory(int32_t const picnum)
{
    EDUKE32_STATIC_ASSERT(sizeof(picanm_t) == 4);
    EDUKE32_STATIC_ASSERT(PICANM_ANIMTYPE_MASK == 192);

    picanm_t * const thispicanm = &picanm[picnum];

    // Old on-disk format: anim type is in the 2 highest bits of the lowest byte.
    thispicanm->sf &= ~192;
    thispicanm->sf |= thispicanm->num&192;
    thispicanm->num &= ~192;

    // don't allow setting texhitscan/nofullbright from ART
    thispicanm->sf &= ~PICANM_MISC_MASK;
}

void E_ReadArtFileTileInfo(int32_t const fil, artheader_t const * const local)
{
    int16_t *tilesizx = (int16_t *) Xmalloc(local->numtiles * sizeof(int16_t));
    int16_t *tilesizy = (int16_t *) Xmalloc(local->numtiles * sizeof(int16_t));
    kread(fil, tilesizx, local->numtiles*sizeof(int16_t));
    kread(fil, tilesizy, local->numtiles*sizeof(int16_t));
    kread(fil, &picanm[local->tilestart], local->numtiles*sizeof(picanm_t));

    for (bssize_t i=local->tilestart; i<=local->tileend; i++)
    {
        tilesiz[i].x = B_LITTLE16(tilesizx[i-local->tilestart]);
        tilesiz[i].y = B_LITTLE16(tilesizy[i-local->tilestart]);

        E_ConvertARTv1picanmToMemory(i);
    }

    DO_FREE_AND_NULL(tilesizx);
    DO_FREE_AND_NULL(tilesizy);
}

void E_ReadArtFileIntoFakeData(int32_t const fil, artheader_t const * const local)
{
    char *buffer = NULL;
    int32_t buffersize = 0;

    for (bssize_t i=local->tilestart; i<=local->tileend; i++)
    {
        int const dasiz = tilesiz[i].x * tilesiz[i].y;

        if (dasiz == 0)
        {
            E_UndefineTile(i);
            continue;
        }

        maybe_grow_buffer(&buffer, &buffersize, dasiz);
        kread(fil, buffer, dasiz);
        E_CreateFakeTile(i, dasiz, buffer);
    }

    DO_FREE_AND_NULL(buffer);
}

static const char *E_GetArtFileName(int32_t tilefilei)
{
    if (tilefilei >= MAXARTFILES_BASE)
    {
        int32_t o = mapartfnXXofs;
        tilefilei -= MAXARTFILES_BASE;

        mapartfilename[o+1] = '0' + tilefilei%10;
        mapartfilename[o+0] = '0' + (tilefilei/10)%10;

        return mapartfilename;
    }
    else
    {
        artfilename[7] = '0' + tilefilei%10;
        artfilename[6] = '0' + (tilefilei/10)%10;
        artfilename[5] = '0' + (tilefilei/100)%10;

        return artfilename;
    }
}

// Returns:
//  0: successfully read ART file
// >0: error with the ART file
// -1: ART file does not exist
//<-1: per-map ART issue
static int32_t E_ReadArtFileOfID(int32_t tilefilei)
{
    const char *fn = E_GetArtFileName(tilefilei);
    const int32_t permap = (tilefilei >= MAXARTFILES_BASE);  // is it a per-map ART file?
    int32_t fil;

    if ((fil = kopen4load(fn, 0)) != -1)
    {
#ifdef WITHKPLIB
        if (permap && cache1d_file_fromzip(fil))
        {
            initprintf("loadpics: per-map ART file \"%s\": can't be read from a ZIP file\n", fn);
            kclose(fil);
            return -2;
        }
#endif

        artheader_t local;
        int const headerval = E_ReadArtFileHeader(fil, fn, &local);
        if (headerval != 0)
        {
            kclose(fil);
            return headerval;
        }

        if (permap)
        {
            // Check whether we can evict existing tiles to make place for
            // per-map ART ones.
            for (int i=local.tilestart; i<=local.tileend; i++)
            {
                // Tiles having dummytile replacements or those that are
                // cache1d-locked can't be replaced.
                if (faketile[i>>3] & pow2char[i&7] || walock[i] >= 200)
                {
                    initprintf("loadpics: per-map ART file \"%s\": "
                        "tile %d has dummytile or is locked\n", fn, i);
                    kclose(fil);
                    return -3;
                }
            }

            // Free existing tiles from the cache1d. CACHE1D_FREE
            Bmemset(&waloff[local.tilestart], 0, local.numtiles*sizeof(intptr_t));
            Bmemset(&walock[local.tilestart], 1, local.numtiles*sizeof(walock[0]));
        }

        E_ReadArtFileTileInfo(fil, &local);

        if (cache1d_file_fromzip(fil))
        {
            E_ReadArtFileIntoFakeData(fil, &local);
        }
        else
        {
            int offscount = 4+4+4+4+(local.numtiles<<3);

            for (bssize_t i=local.tilestart; i<=local.tileend; ++i)
            {
                int const dasiz = tilesiz[i].x * tilesiz[i].y;

                tilefilenum[i] = tilefilei;
                tilefileoffs[i] = offscount;

                offscount += dasiz;
                // artsize += ((dasiz+15)&0xfffffff0);
            }
        }

#ifdef DEBUGGINGAIDS
        if (permap)
            initprintf("Read in per-map ART file \"%s\"\n", fn);
#endif
        kclose(fil);
        return 0;
    }

    return -1;
}

//
// loadpics
//
int32_t loadpics(const char *filename, int32_t askedsize)
{
    Bstrncpyz(artfilename, filename, sizeof(artfilename));

    Bmemset(&tilesiz[0], 0, sizeof(vec2s_t) * MAXTILES);
    Bmemset(picanm, 0, sizeof(picanm));

    //    artsize = 0;

    for (bssize_t tilefilei=0; tilefilei<MAXARTFILES_BASE; tilefilei++)
        E_ReadArtFileOfID(tilefilei);

    Bmemset(gotpic, 0, sizeof(gotpic));

    //cachesize = min((int32_t)((Bgetsysmemsize()/100)*60),max(artsize,askedsize));
    cachesize = (Bgetsysmemsize() <= (uint32_t)askedsize) ? (int32_t)((Bgetsysmemsize() / 100) * 60) : askedsize;

    // NOTE: this doesn't make a lot of sense on modern OSs...
    while ((pic = Bmalloc(cachesize)) == NULL)
    {
        cachesize -= 65536;
        if (cachesize < 65536)
            return -1;
    }
    initcache((intptr_t) pic, cachesize);

    E_RecalcPicSiz();

    artfil = -1;
    artfilnum = -1;
    artfilplc = 0L;

    return 0;
}


//
// loadtile
//
static void postloadtile(int16_t tilenume);

void loadtile(int16_t tilenume)
{
    if ((unsigned) tilenume >= (unsigned) MAXTILES) return;
    int const dasiz = tilesiz[tilenume].x*tilesiz[tilenume].y;
    if (dasiz <= 0) return;

    // Allocate storage if necessary.
    if (waloff[tilenume] == 0)
    {
        walock[tilenume] = 199;
        allocache(&waloff[tilenume], dasiz, &walock[tilenume]);
    }

    E_LoadTileIntoBuffer(tilenume, dasiz, (char *) waloff[tilenume]);

    postloadtile(tilenume);
}

void E_LoadTileIntoBuffer(int16_t tilenume, int32_t dasiz, char *buffer)
{
    // dummy tiles for highres replacements and tilefromtexture definitions

    if (faketile[tilenume>>3] & pow2char[tilenume&7])
    {
        if (faketiledata[tilenume] != NULL)
            LZ4_decompress_fast(faketiledata[tilenume], buffer, dasiz);

        faketimerhandler();
        return;
    }

    int const tfn = tilefilenum[tilenume];

    // Potentially switch open ART file.
    if (tfn != artfilnum)
    {
        if (artfil != -1)
            kclose(artfil);

        char const *fn = E_GetArtFileName(tfn);

        artfil = kopen4load(fn, 0);

        if (artfil == -1)
        {
            initprintf("Failed opening ART file \"%s\"!\n", fn);
            uninitengine();
            Bexit(11);
        }

        artfilnum = tfn;
        artfilplc = 0L;

        faketimerhandler();
    }

    // Seek to the right position.
    if (artfilplc != tilefileoffs[tilenume])
    {
        klseek(artfil, tilefileoffs[tilenume], BSEEK_SET);
        faketimerhandler();
    }

    kread(artfil, buffer, dasiz);
    faketimerhandler();
    artfilplc = tilefileoffs[tilenume]+dasiz;
}

static void postloadtile(int16_t tilenume)
{
#if !defined DEBUG_TILESIZY_512 && !defined DEBUG_TILEOFFSETS
    UNREFERENCED_PARAMETER(tilenume);
#endif
#ifdef DEBUG_TILESIZY_512
    if (tilesizy[tilenume] >= 512)
    {
        int32_t i;
        char *p = (char *) waloff[tilenume];
        for (i=0; i<tilesizx[tilenume]*tilesizy[tilenume]; i++)
            p[i] = i;
    }
#endif
#ifdef DEBUG_TILEOFFSETS
    // Add some dark blue marker lines to STEAM and CEILINGSTEAM.
    // See test_tileoffsets.map.
    if (tilenume==1250 || tilenume==1255)
    {
        char *p = (char *) waloff[tilenume];
        p[0] = p[1] = p[2] = p[3] = 254;
    }

    // Add some offset to the cocktail glass neon sign. It's more asymmetric
    // than the steam, and thus more suited to debugging the spatial
    // orientation of drawn sprites.
    if (tilenume==1008)
    {
        picanm[tilenume].xofs = 8;
        picanm[tilenume].yofs = 12;
    }
#endif
}

// Assumes pic has been initialized to zero.
void E_RenderArtDataIntoBuffer(palette_t * const pic, uint8_t const * const buf, int32_t const bufsizx, int32_t const sizx, int32_t const sizy)
{
    for (bssize_t y = 0; y < sizy; ++y)
    {
        palette_t * const picrow = &pic[bufsizx * y];

        for (bssize_t x = 0; x < sizx; ++x)
        {
            uint8_t index = buf[sizy * x + y];

            if (index == 255)
                continue;

            index *= 3;

            // pic is BGRA
            picrow[x].r = palette[index+2];
            picrow[x].g = palette[index+1];
            picrow[x].b = palette[index];
            picrow[x].f = 255;
        }
    }
}

//
// allocatepermanenttile
//
intptr_t allocatepermanenttile(int16_t tilenume, int32_t xsiz, int32_t ysiz)
{
    if (xsiz <= 0 || ysiz <= 0 || (unsigned) tilenume >= MAXTILES)
        return 0;

    int const dasiz = xsiz*ysiz;

    walock[tilenume] = 255;
    allocache(&waloff[tilenume], dasiz, &walock[tilenume]);

    set_tilesiz(tilenume, xsiz, ysiz);
    Bmemset(&picanm[tilenume], 0, sizeof(picanm_t));

    return waloff[tilenume];
}

//
// copytilepiece
//
void copytilepiece(int32_t tilenume1, int32_t sx1, int32_t sy1, int32_t xsiz, int32_t ysiz,
    int32_t tilenume2, int32_t sx2, int32_t sy2)
{
    char *ptr1, *ptr2, dat;
    int32_t xsiz1, ysiz1, xsiz2, ysiz2, i, j, x1, y1, x2, y2;

    xsiz1 = tilesiz[tilenume1].x; ysiz1 = tilesiz[tilenume1].y;
    xsiz2 = tilesiz[tilenume2].x; ysiz2 = tilesiz[tilenume2].y;
    if ((xsiz1 > 0) && (ysiz1 > 0) && (xsiz2 > 0) && (ysiz2 > 0))
    {
        if (waloff[tilenume1] == 0) loadtile(tilenume1);
        if (waloff[tilenume2] == 0) loadtile(tilenume2);

        x1 = sx1;
        for (i=0; i<xsiz; i++)
        {
            y1 = sy1;
            for (j=0; j<ysiz; j++)
            {
                x2 = sx2+i;
                y2 = sy2+j;
                if ((x2 >= 0) && (y2 >= 0) && (x2 < xsiz2) && (y2 < ysiz2))
                {
                    ptr1 = (char *) (waloff[tilenume1] + x1*ysiz1 + y1);
                    ptr2 = (char *) (waloff[tilenume2] + x2*ysiz2 + y2);
                    dat = *ptr1;
                    if (dat != 255)
                        *ptr2 = *ptr1;
                }

                y1++; if (y1 >= ysiz1) y1 = 0;
            }
            x1++; if (x1 >= xsiz1) x1 = 0;
        }
    }
}

void Buninitart(void)
{
    if (artfil != -1)
        kclose(artfil);

    DO_FREE_AND_NULL(pic);
}
