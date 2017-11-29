#include "compat.h"
#include "build.h"
#include "engine_priv.h"
#include "baselayer.h"
#include "colmatch.h"
#include "cache1d.h"
#include "palette.h"
#include "a.h"
#include "xxhash.h"

uint8_t *basepaltable[MAXBASEPALS] = { palette };
uint8_t basepalreset=1;
uint8_t curbasepal;
int32_t globalblend;

uint32_t g_lastpalettesum = 0;
palette_t curpalette[256];			// the current palette, unadjusted for brightness or tint
palette_t curpalettefaded[256];		// the current palette, adjusted for brightness and tint (ie. what gets sent to the card)
palette_t palfadergb = { 0, 0, 0, 0 };
char palfadedelta = 0;
uint8_t blackcol;

int32_t realmaxshade;
float frealmaxshade;

#if defined(USE_OPENGL)
palette_t palookupfog[MAXPALOOKUPS];
#endif

// For every pal number, whether tsprite pal should not be taken over from
// floor pal.
// NOTE: g_noFloorPal[0] is irrelevant as it's never checked.
int8_t g_noFloorPal[MAXPALOOKUPS];

int32_t curbrightness = 0, gammabrightness = 0;

static void setpalettefade_calc(uint8_t offset);

void fade_screen_black(int32_t moreopaquep)
{
#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
        fullscreen_tint_gl(0, 0, 0, moreopaquep ? 168 : 84);
    else
#endif
    {
        Bassert(!offscreenrendering);

        begindrawing();
        {
            char *const p = (char *) frameplace;
            const char *const trans = getblendtab(0);
            const int32_t shiftamnt = ((!!moreopaquep)*8);
            const int32_t dimprod = xdim*ydim;
            int32_t i = 0;

#ifdef CLASSIC_SLICE_BY_4
            for (; i<dimprod-4; i+=4)
            {
                p[i] = trans[p[i]<<shiftamnt];
                p[i+1] = trans[p[i+1]<<shiftamnt];
                p[i+2] = trans[p[i+2]<<shiftamnt];
                p[i+3] = trans[p[i+3]<<shiftamnt];
            }
#endif

            for (; i<dimprod; i++)
                p[i] = trans[p[i]<<shiftamnt];
        }
        enddrawing();
    }
}

void setup_blend(int32_t blend, int32_t doreverse)
{
    if (blendtable[blend] == NULL)
        blend = 0;

    if (globalblend != blend)
    {
        globalblend = blend;
        fixtransluscence(FP_OFF(getblendtab(blend)));
    }

    if (doreverse)
        settransreverse();
    else
        settransnormal();
}

static void alloc_palookup(int32_t pal)
{
    // The asm functions vlineasm1, mvlineasm1 (maybe others?) access the next
    // palookup[...] shade entry for tilesizy==512 tiles.
    // See DEBUG_TILESIZY_512 and the comment in a.nasm: vlineasm1.
    palookup[pal] = (char *) Xaligned_alloc(16, (numshades + 1) * 256);
    memset(palookup[pal], 0, (numshades + 1) * 256);
}

static void maybe_alloc_palookup(int32_t palnum);

//
// loadpalette (internal)
//
void loadpalette(void)
{
    initfastcolorlookup_scale(30, 59, 11);
    initfastcolorlookup_gridvectors();

#ifdef USE_OPENGL
    for (size_t x = 0; x < MAXBLENDTABS; ++x)
        glblend[x] = defaultglblend;
#endif

    int32_t fil;
    if ((fil = kopen4load("palette.dat", 0)) == -1)
        return;


    // PALETTE_MAIN

    if (kread_and_test(fil, palette, 768))
        return kclose(fil);

    for (bssize_t k = 0; k < 768; k++)
        palette[k] <<= 2;

    initfastcolorlookup_palette(palette);

    paletteloaded |= PALETTE_MAIN;


    // PALETTE_SHADES

    if (kread_and_test(fil, &numshades, 2))
        return kclose(fil);
    numshades = B_LITTLE16(numshades);

    if (numshades <= 1)
    {
        initprintf("Warning: Invalid number of shades in \"palette.dat\"!\n");
        numshades = 0;
        return kclose(fil);
    }

    // Auto-detect LameDuke. Its PALETTE.DAT doesn't have a 'numshades' 16-bit
    // int after the base palette, but starts directly with the shade tables.
    // Thus, the first two bytes will be 00 01, which is 256 if read as
    // little-endian int16_t.
    int32_t lamedukep = 0;
    if (numshades >= 256)
    {
        static char const * const seekfail = "Warning: klseek() failed in loadpalette()!\n";

        uint16_t temp;
        if (kread_and_test(fil, &temp, 2))
            return kclose(fil);
        temp = B_LITTLE16(temp);
        if (temp == 770 || numshades > 256) // 02 03
        {
            if (klseek(fil, -4, BSEEK_CUR) < 0)
            {
                initputs(seekfail);
                return kclose(fil);
            }

            numshades = 32;
            lamedukep = 1;
        }
        else
        {
            if (klseek(fil, -2, BSEEK_CUR) < 0)
            {
                initputs(seekfail);
                return kclose(fil);
            }
        }
    }

    // Read base shade table (palookup 0).
    maybe_alloc_palookup(0);
    if (kread_and_test(fil, palookup[0], numshades<<8))
        return kclose(fil);

    paletteloaded |= PALETTE_SHADE;


    // PALETTE_TRANSLUC

    char * const transluc = blendtable[0] = (char *) Xcalloc(256, 256);

    // Read translucency (blending) table.
    if (lamedukep)
    {
        for (bssize_t i=0; i<255; i++)
        {
            // NOTE: LameDuke's table doesn't have the last row or column (i==255).

            // Read the entries above and on the diagonal, if the table is
            // thought as being row-major.
            if (kread_and_test(fil, &transluc[256*i + i], 256-i-1))
                return kclose(fil);

            // Duplicate the entries below the diagonal.
            for (bssize_t j=0; j<i; j++)
                transluc[256*i + j] = transluc[256*j + i];
        }
    }
    else
    {
        if (kread_and_test(fil, transluc, 65536))
            return kclose(fil);
    }

    paletteloaded |= PALETTE_TRANSLUC;


    // additional blending tables

    uint8_t magic[12];
    if (!kread_and_test(fil, magic, sizeof(magic)) && !Bmemcmp(magic, "MoreBlendTab", sizeof(magic)))
    {
        uint8_t addblendtabs;
        if (kread_and_test(fil, &addblendtabs, 1))
        {
            initprintf("Warning: failed reading additional blending table count\n");
            return kclose(fil);
        }

        uint8_t blendnum;
        char *tab = (char *) Xmalloc(256*256);
        for (bssize_t i=0; i<addblendtabs; i++)
        {
            if (kread_and_test(fil, &blendnum, 1))
            {
                initprintf("Warning: failed reading additional blending table index\n");
                Bfree(tab);
                return kclose(fil);
            }

            if (getblendtab(blendnum) != NULL)
                initprintf("Warning: duplicate blending table index %3d encountered\n", blendnum);

            if (kread_and_test(fil, tab, 256*256))
            {
                initprintf("Warning: failed reading additional blending table\n");
                Bfree(tab);
                return kclose(fil);
            }

            setblendtab(blendnum, tab);
        }
        Bfree(tab);

        // Read log2 of count of alpha blending tables.
        uint8_t lognumalphatabs;
        if (!kread_and_test(fil, &lognumalphatabs, 1))
        {
            if (!(lognumalphatabs >= 1 && lognumalphatabs <= 7))
                initprintf("invalid lognumalphatabs value, must be in [1 .. 7]\n");
            else
                numalphatabs = 1<<lognumalphatabs;
        }
    }

    kclose(fil);
}

uint32_t PaletteIndexFullbrights[8];

void E_PostLoadPalette(void)
{
    globalpal = 0;

    globalpalwritten = palookup[0];
    setpalookupaddress(globalpalwritten);

    fixtransluscence(FP_OFF(blendtable[0]));

    char const * const palookup0 = palookup[0];

#ifdef DEBUG_TILESIZY_512
    // Bump shade 1 by 16.
    for (bssize_t i=256; i<512; i++)
        palookup0[i] = palookup0[i+(16<<8)];
#endif

    blackcol = getclosestcol(0, 0, 0);
    whitecol = getclosestcol(255, 255, 255);
    redcol = getclosestcol(255, 0, 0);

    for (size_t i = 0; i<16; i++)
    {
        palette_t *edcol = (palette_t *) &vgapal16[4*i];
        editorcolors[i] = getclosestcol_lim(edcol->b, edcol->g, edcol->r, 239);
    }

    // Bmemset(PaletteIndexFullbrights, 0, sizeof(PaletteIndexFullbrights));
    for (bssize_t c = 0; c < 255; ++c) // skipping transparent color
    {
        uint8_t const index = palookup0[c];
        rgb24_t const & color = *(rgb24_t *)&palette[index*3];

        // don't consider #000000 fullbright
        if (EDUKE32_PREDICT_FALSE(color.r == 0 && color.g == 0 && color.b == 0))
            continue;

        for (size_t s = c + 256, s_end = 256*numshades; s < s_end; s += 256)
            if (EDUKE32_PREDICT_FALSE(palookup0[s] != index))
                goto PostLoad_NotFullbright;

        SetPaletteIndexFullbright(c);

        PostLoad_NotFullbright: ;
    }

    uint8_t const * const blackcolor = &palette[blackcol*3];
    size_t s;
    for (s = numshades < 2 ? 0 : numshades-2; s > 0; --s)
    {
        for (size_t c = s*256, c_end = c+255; c < c_end; ++c) // skipping transparent color
        {
            uint8_t const index = palookup0[c];
            uint8_t const * const color = &palette[index*3];
            if (!IsPaletteIndexFullbright(index) && memcmp(blackcolor, color, sizeof(rgb24_t)))
                goto PostLoad_FoundShade;
        }
    }
    PostLoad_FoundShade: ;
    frealmaxshade = (float)(realmaxshade = s+1);
}

void E_ReplaceTransparentColorWithBlack(void)
{
    for (bssize_t i=0; i<MAXPALOOKUPS; i++)
    {
        char * const thispalookup = palookup[i];
        if (thispalookup == NULL)
            continue;

        for (bssize_t j=0; j<numshades; j++)
        {
            thispalookup[(j<<8) + 255] = 255;
        }
    }

    // fix up translucency table so that transluc(255,x)
    // and transluc(x,255) is black instead of purple.
    for (bssize_t i=0; i<MAXBLENDTABS; i++)
    {
        char * const transluc = blendtable[i];
        if (transluc == NULL)
            continue;

        for (bssize_t j=0; j<255; j++)
        {
            transluc[(255<<8) + j] = transluc[(blackcol<<8) + j];
            transluc[255 + (j<<8)] = transluc[blackcol + (j<<8)];
        }
        transluc[(255<<8) + 255] = transluc[(blackcol<<8) + blackcol];
    }
}

// Load LOOKUP.DAT, which contains lookup tables and additional base palettes.
//
// <fp>: kopen4load file handle
//
// Returns:
//  - on success, 0
//  - on error, -1 (didn't read enough data)
//  - -2: error, we already wrote an error message ourselves
int32_t loadlookups(int32_t fp)
{
    uint8_t numlookups;
    char remapbuf[256];

    if (kread_and_test(fp, &numlookups, 1))
        return -1;

    for (bssize_t j=0; j<numlookups; j++)
    {
        uint8_t palnum;

        if (kread_and_test(fp, &palnum, 1))
            return -1;

        if (palnum >= 256-RESERVEDPALS)
        {
            initprintf("ERROR: attempt to load lookup at reserved pal %d\n", palnum);
            return -2;
        }

        if (kread_and_test(fp, remapbuf, 256))
            return -1;

        makepalookup(palnum, remapbuf, 0, 0, 0, 0);
    }

    return 0;
}

void generatefogpals(void)
{
    // Find a gap of four consecutive unused pal numbers to generate fog shade
    // tables.
    for (bssize_t j=1; j<=255-3; j++)
        if (!palookup[j] && !palookup[j+1] && !palookup[j+2] && !palookup[j+3])
        {
            makepalookup(j, NULL, 60, 60, 60, 1);
            makepalookup(j+1, NULL, 60, 0, 0, 1);
            makepalookup(j+2, NULL, 0, 60, 0, 1);
            makepalookup(j+3, NULL, 0, 0, 60, 1);

            break;
        }
}

void fillemptylookups(void)
{
    // Alias remaining unused pal numbers to the base shade table.
    for (bssize_t j=1; j<MAXPALOOKUPS; j++)
    {
        // If an existing lookup is identical to #0, free it.
        if (palookup[j] && palookup[j] != palookup[0] && !Bmemcmp(palookup[0], palookup[j], 256*numshades))
            removepalookup(j);

        if (!palookup[j])
            makepalookup(j, NULL, 0, 0, 0, 1);
    }
}

static int32_t palookup_isdefault(int32_t palnum)  // KEEPINSYNC engine.lua
{
    return (palookup[palnum] == NULL || (palnum!=0 && palookup[palnum] == palookup[0]));
}

static void maybe_alloc_palookup(int32_t palnum)
{
    if (palookup_isdefault(palnum))
    {
        alloc_palookup(palnum);
        if (palookup[palnum] == NULL)
            Bexit(1);
    }
}

void setblendtab(int32_t blend, const char *tab)
{
    if (blendtable[blend] == NULL)
        blendtable[blend] = (char *) Xmalloc(256*256);

    Bmemcpy(blendtable[blend], tab, 256*256);
}
void removeblendtab(int32_t const blend)
{
    DO_FREE_AND_NULL(blendtable[blend]);
}

#ifdef LUNATIC
const char *(getblendtab) (int32_t blend)
{
    return blendtable[blend];
}
#endif

#ifdef USE_OPENGL
glblend_t const nullglblend =
{
    {
        { 1.f, BLENDFACTOR_ONE, BLENDFACTOR_ZERO, 0 },
        { 1.f, BLENDFACTOR_ONE, BLENDFACTOR_ZERO, 0 },
    },
};
glblend_t const defaultglblend =
{
    {
        { 2.f/3.f, BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, 0 },
        { 1.f/3.f, BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, 0 },
    },
};

glblend_t glblend[MAXBLENDTABS];

void handle_blend(uint8_t enable, uint8_t blend, uint8_t def)
{
    static GLenum const blendFuncTokens[NUMBLENDFACTORS] =
    {
        GL_ZERO,
        GL_ONE,
        GL_SRC_COLOR,
        GL_ONE_MINUS_SRC_COLOR,
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA,
        GL_DST_ALPHA,
        GL_ONE_MINUS_DST_ALPHA,
        GL_DST_COLOR,
        GL_ONE_MINUS_DST_COLOR,
    };

    if (!enable)
    {
        bglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        return;
    }

    glblenddef_t const * const glbdef = glblend[blend].def + def;
    bglBlendFunc(blendFuncTokens[glbdef->src], blendFuncTokens[glbdef->dst]);
}
#endif

int32_t setpalookup(int32_t palnum, const uint8_t *shtab)
{
    if (numshades != 32)
        return -1;

    if (shtab != NULL)
    {
        maybe_alloc_palookup(palnum);
        Bmemcpy(palookup[palnum], shtab, 256*numshades);
    }

    return 0;
}
void removepalookup(int32_t const palnum)
{
    if (palnum == 0 && palookup[palnum] != NULL)
    {
        for (bssize_t i = 1; i < MAXPALOOKUPS; i++)
            if (palookup[i] == palookup[palnum])
                palookup[i] = NULL;

        ALIGNED_FREE_AND_NULL(palookup[palnum]);
    }
    else if (palookup[palnum] == palookup[0])
        palookup[palnum] = NULL;
    else
        ALIGNED_FREE_AND_NULL(palookup[palnum]);
}

//
// makepalookup
//
void makepalookup(int32_t palnum, const char *remapbuf, uint8_t r, uint8_t g, uint8_t b, char noFloorPal)
{
    int32_t i, j;

    static char idmap[256] = { 1 };

    if (paletteloaded == 0)
        return;

    // NOTE: palnum==0 is allowed
    if ((unsigned) palnum >= MAXPALOOKUPS)
        return;

    g_noFloorPal[palnum] = noFloorPal;

    if (remapbuf==NULL)
    {
        if ((r|g|b) == 0)
        {
            palookup[palnum] = palookup[0];  // Alias to base shade table!
            return;
        }

        if (idmap[0]==1)  // init identity map
            for (i=0; i<256; i++)
                idmap[i] = i;

        remapbuf = idmap;
    }

    maybe_alloc_palookup(palnum);

    if ((r|g|b) == 0)
    {
        // "black fog"/visibility case -- only remap color indices

        for (j=0; j<numshades; j++)
            for (i=0; i<256; i++)
            {
                const char *src = palookup[0];
                palookup[palnum][256*j + i] = src[256*j + remapbuf[i]];
            }
    }
    else
    {
        // colored fog case

        char *ptr2 = palookup[palnum];

        for (i=0; i<numshades; i++)
        {
            int32_t palscale = divscale16(i, numshades-1);

            for (j=0; j<256; j++)
            {
                const char *ptr = (const char *) &palette[remapbuf[j]*3];
                *ptr2++ = getclosestcol(ptr[0] + mulscale16(r-ptr[0], palscale),
                    ptr[1] + mulscale16(g-ptr[1], palscale),
                    ptr[2] + mulscale16(b-ptr[2], palscale));
            }
        }
    }

#if defined(USE_OPENGL)
    palookupfog[palnum].r = r;
    palookupfog[palnum].g = g;
    palookupfog[palnum].b = b;
#endif
}

//
// setbasepal
//
void setbasepal(int32_t id, uint8_t const * const table)
{
    if (basepaltable[id] == NULL)
        basepaltable[id] = (uint8_t *) Xmalloc(768);

    Bmemcpy(basepaltable[id], table, 768);
}
void removebasepal(int32_t const id)
{
    if (id == 0)
        Bmemset(basepaltable[id], 0, 768);
    else
        DO_FREE_AND_NULL(basepaltable[id]);
}

//
// setbrightness
//
// flags:
//  1: don't setpalette(),  DON'T USE THIS FLAG!
//  2: don't gltexinvalidateall()
//  4: don't calc curbrightness from dabrightness,  DON'T USE THIS FLAG!
//  8: don't gltexinvalidate8()
// 16: don't reset palfade*
void setbrightness(char dabrightness, uint8_t dapalid, uint8_t flags)
{
    int32_t i, j;
    const uint8_t *dapal;

#ifdef USE_OPENGL
    int32_t paldidchange;
#endif
    int32_t palsumdidchange;
    //    uint32_t lastbright = curbrightness;

    Bassert((flags&4)==0);

    if (/*(unsigned)dapalid >= MAXBASEPALS ||*/ basepaltable[dapalid] == NULL)
        dapalid = 0;
#ifdef USE_OPENGL
    paldidchange = (curbasepal != dapalid || basepalreset);
#endif
    curbasepal = dapalid;
    basepalreset = 0;

    dapal = basepaltable[curbasepal];

    if (!(flags&4))
    {
        curbrightness = clamp(dabrightness, 0, 15);
        //        if (lastbright != (unsigned)curbrightness)
        //            vid_gamma = 1.0 + ((float)curbrightness / 10.0);
    }

    setgamma();
    j = !gammabrightness ? curbrightness : 0;

    for (i=0; i<256; i++)
    {
        // save palette without any brightness adjustment
        curpalette[i].r = dapal[i*3+0];
        curpalette[i].g = dapal[i*3+1];
        curpalette[i].b = dapal[i*3+2];
        curpalette[i].f = 0;

        // brightness adjust the palette
        curpalettefaded[i].b = britable[j][curpalette[i].b];
        curpalettefaded[i].g = britable[j][curpalette[i].g];
        curpalettefaded[i].r = britable[j][curpalette[i].r];
        curpalettefaded[i].f = 0;
    }

    if ((flags&16) && palfadedelta)  // keep the fade
        setpalettefade_calc(palfadedelta>>2);

    {
        static uint32_t lastpalettesum=0;
        uint32_t newpalettesum = XXH32((uint8_t *) curpalettefaded, sizeof(curpalettefaded), sizeof(curpalettefaded));

        palsumdidchange = (newpalettesum != lastpalettesum);

        if (palsumdidchange || newpalettesum != g_lastpalettesum)
        {
            //            if ((flags&1) == 0)
            setpalette(0, 256);
        }

        g_lastpalettesum = lastpalettesum = newpalettesum;
    }

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
    {
        // Only reset the textures if the corresponding preserve flags are clear and
        // either (a) the new palette is different to the last, or (b) the brightness
        // changed and we couldn't set it using hardware gamma.

        // XXX: no-HW-gamma OpenGL platforms will exhibit bad performance with
        // simultaneous basepal and tint changes?
        const int32_t doinvalidate = (paldidchange || (palsumdidchange && !gammabrightness));

        if (!(flags&2) && doinvalidate)
            gltexinvalidatetype(INVALIDATE_ALL);
        if (!(flags&8) && doinvalidate)
            gltexinvalidatetype(INVALIDATE_ART);
#ifdef POLYMER
        if ((getrendermode() == REND_POLYMER) && doinvalidate)
            polymer_texinvalidate();
#endif
    }
#endif

    if ((flags&16)==0)
    {
        palfadergb.r = palfadergb.g = palfadergb.b = 0;
        palfadedelta = 0;
    }
}

palette_t getpal(int32_t col)
{
    if (!gammabrightness)
    {
        palette_t const p = { britable[curbrightness][curpalette[col].r], britable[curbrightness][curpalette[col].g],
                              britable[curbrightness][curpalette[col].b], 0 };
        return p;
    }

    return curpalette[col];
}

static void setpalettefade_calc(uint8_t offset)
{
    int32_t i;
    palette_t p;

    for (i=0; i<256; i++)
    {
        p = getpal(i);

        curpalettefaded[i].b =
            p.b + (((palfadergb.b - p.b) * offset) >> 8);
        curpalettefaded[i].g =
            p.g + (((palfadergb.g - p.g) * offset) >> 8);
        curpalettefaded[i].r =
            p.r + (((palfadergb.r - p.r) * offset) >> 8);
        curpalettefaded[i].f = 0;
    }
}

//#define DEBUG_PALETTEFADE

//
// setpalettefade
//
void setpalettefade(uint8_t r, uint8_t g, uint8_t b, uint8_t offset)
{
    palfadergb.r = r;
    palfadergb.g = g;
    palfadergb.b = b;
#ifdef DEBUG_PALETTEFADE
    if (offset)
        offset = max(offset, 128);
#endif
    palfadedelta = offset;

    setpalettefade_calc(offset);

    {
        static uint32_t lastpalettesum=0;
        uint32_t newpalettesum = XXH32((uint8_t *) curpalettefaded, sizeof(curpalettefaded), sizeof(curpalettefaded));

        if (newpalettesum != lastpalettesum || newpalettesum != g_lastpalettesum)
            setpalette(0, 256);

        g_lastpalettesum = lastpalettesum = newpalettesum;
    }
}
