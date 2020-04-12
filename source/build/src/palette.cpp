// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "compat.h"
#include "build.h"
#include "engine_priv.h"
#include "baselayer.h"
#include "imagehelpers.h"

#include "palette.h"
#include "superfasthash.h"
#include "common.h"
#include "memarena.h"
#include "palettecontainer.h"
#include "palutil.h"
#include "colormatcher.h"
#include "../../glbackend/glbackend.h"

FMemArena lookuparena;

uint8_t curbasepal;
int32_t globalblend;

palette_t palfadergb = { 0, 0, 0, 0 };
unsigned char palfadedelta = 0;
ESetPalFlags curpaletteflags;

#if defined(USE_OPENGL)
palette_t palookupfog[MAXPALOOKUPS];
#endif

// For every pal number, whether tsprite pal should not be taken over from
// floor pal.
// NOTE: g_noFloorPal[0] is irrelevant as it's never checked.
int8_t g_noFloorPal[MAXPALOOKUPS];

int32_t curbrightness = 0;

void setBlendFactor(int index, int alpha);


int DetermineTranslucency(const uint8_t *table)
{
	uint8_t index;
	PalEntry newcolor;
	PalEntry newcolor2;

	index = table[GPalette.BlackIndex * 256 + GPalette.WhiteIndex];
    newcolor = GPalette.BaseColors[index];

	index = table[GPalette.WhiteIndex * 256 + GPalette.BlackIndex];
    newcolor2 = GPalette.BaseColors[index];
    if (newcolor2.r == 255)	// if black on white results in white it's either
							// fully transparent or additive
	{
		return -newcolor.r;
	}

	return newcolor.r;
}

void paletteSetColorTable(int32_t id, uint8_t const* table, bool notransparency)
{
    if (id == 0)
    {
        GPalette.SetPalette(table, 255);
    }
    FRemapTable remap;
    remap.AddColors(0, 256, table);
    if (!notransparency)
    {
        remap.Palette[255] = 0;
        remap.Remap[255] = 255;
    }
    GPalette.UpdateTranslation(TRANSLATION(Translation_BasePalettes, id), &remap);

    // Todo: remove this once the texture code can use GPalette directly
#ifdef USE_OPENGL
    uploadbasepalette(id);
#endif
}



void fullscreen_tint_gl(PalEntry pe);

static void alloc_palookup(int32_t pal)
{
    // The asm functions vlineasm1, mvlineasm1 (maybe others?) access the next
    // lookuptables[...] shade entry for tilesizy==512 tiles.
    // See DEBUG_TILESIZY_512 and the comment in a.nasm: vlineasm1.
    lookuptables[pal] = (char *) Xaligned_alloc(16, (numshades + 1) * 256);
    memset(lookuptables[pal], 0, (numshades + 1) * 256);
}

static void maybe_alloc_palookup(int32_t palnum);

void (*paletteLoadFromDisk_replace)(void) = NULL;

inline bool read_and_test(FileReader& handle, void* buffer, int32_t leng)
{
	return handle.Read(buffer, leng) != leng;
};

//
// loadpalette (internal)
//
void paletteLoadFromDisk(void)
{
    GPalette.Init(MAXPALOOKUPS + 1);    // one slot for each translation, plus a separate one for the base palettes.

#ifdef USE_OPENGL
    for (auto & x : glblend)
        x = defaultglblend;
#endif

    if (paletteLoadFromDisk_replace)
    {
        paletteLoadFromDisk_replace();
        return;
    }

	auto fil = fileSystem.OpenFileReader("palette.dat");
	if (!fil.isOpen())
        return;


    // PALETTE_MAIN

    uint8_t palette[768];
    if (768 != fil.Read(palette, 768))
        return;

    for (unsigned char & k : palette)
        k <<= 2;

    paletteSetColorTable(0, palette);
    paletteloaded |= PALETTE_MAIN;


    // PALETTE_SHADES

    if (2 != fil.Read(&numshades, 2))
        return;

    numshades = B_LITTLE16(numshades);

    if (numshades <= 1)
    {
        Printf("Warning: Invalid number of shades in \"palette.dat\"!\n");
        numshades = 0;
        return;
    }

    // Auto-detect LameDuke. Its PALETTE.DAT doesn't have a 'numshades' 16-bit
    // int after the base palette, but starts directly with the shade tables.
    // Thus, the first two bytes will be 00 01, which is 256 if read as
    // little-endian int16_t.
    int32_t lamedukep = 0;
    if (numshades >= 256)
    {
        uint16_t temp;
        if (read_and_test(fil, &temp, 2))
            return;
        temp = B_LITTLE16(temp);
        if (temp == 770 || numshades > 256) // 02 03
        {
            if (fil.Seek(-4, FileReader::SeekCur) < 0)
            {
                Printf("Warning: seek failed in loadpalette()!\n");
                return;
            }

            numshades = 32;
            lamedukep = 1;
        }
        else
        {
            if (fil.Seek(-2, FileReader::SeekCur) < 0)
            {
                Printf("Warning: seek failed in loadpalette()!\n");
                return;
            }
        }
    }

    // Read base shade table (lookuptables 0).
    maybe_alloc_palookup(0);
    if (read_and_test(fil, lookuptables[0], numshades<<8))
        return;

    paletteloaded |= PALETTE_SHADE;
    paletteloaded |= PALETTE_TRANSLUC;


    // additional blending tables

    uint8_t magic[12];
    if (!read_and_test(fil, magic, sizeof(magic)) && !Bmemcmp(magic, "MoreBlendTab", sizeof(magic)))
    {
        uint8_t addblendtabs;
        if (read_and_test(fil, &addblendtabs, 1))
        {
            Printf("Warning: failed reading additional blending table count\n");
            return;
        }

        uint8_t blendnum;
        char *tab = (char *) Xmalloc(256*256);
        for (bssize_t i=0; i<addblendtabs; i++)
        {
            if (read_and_test(fil, &blendnum, 1))
            {
                Printf("Warning: failed reading additional blending table index\n");
                Xfree(tab);
                return;
            }

            if (read_and_test(fil, tab, 256*256))
            {
                Printf("Warning: failed reading additional blending table\n");
                Xfree(tab);
                return;
            }

			setBlendFactor(blendnum, DetermineTranslucency((const uint8_t*)tab));

        }
        Xfree(tab);

        // Read log2 of count of alpha blending tables.
        uint8_t lognumalphatabs;
        if (!read_and_test(fil, &lognumalphatabs, 1))
        {
            if (!(lognumalphatabs >= 1 && lognumalphatabs <= 7))
                Printf("invalid lognumalphatabs value, must be in [1 .. 7]\n");
            else
                numalphatabs = 1<<lognumalphatabs;
        }
    }
}

uint8_t PaletteIndexFullbrights[32];

void palettePostLoadTables(void)
{
    globalpal = 0;

    char const * const palookup0 = lookuptables[0];

    ImageHelpers::SetPalette(GPalette.BaseColors);
    
    // Bmemset(PaletteIndexFullbrights, 0, sizeof(PaletteIndexFullbrights));
    for (bssize_t c = 0; c < 255; ++c) // skipping transparent color
    {
        uint8_t const index = palookup0[c];
        PalEntry color = GPalette.BaseColors[index];

        // don't consider #000000 fullbright
        if (color.r == 0 && color.g == 0 && color.b == 0)
            continue;

        for (size_t s = c + 256, s_end = 256*numshades; s < s_end; s += 256)
            if (palookup0[s] != index)
                goto PostLoad_NotFullbright;

        SetPaletteIndexFullbright(c);

        PostLoad_NotFullbright: ;
    }
}

void paletteFixTranslucencyMask(void)
{
    for (auto thispalookup : lookuptables)
    {
        if (thispalookup == NULL)
            continue;

        for (bssize_t j=0; j<numshades; j++)
            thispalookup[(j<<8) + 255] = 255;
    }
}

// Load LOOKUP.DAT, which contains lookup tables and additional base palettes.
//
// <fp>: open file handle
//
// Returns:
//  - on success, 0
//  - on error, -1 (didn't read enough data)
//  - -2: error, we already wrote an error message ourselves
int32_t paletteLoadLookupTable(FileReader &fp)
{
    uint8_t numlookups;
    char remapbuf[256];

    if (1 != fp.Read(&numlookups, 1))
        return -1;

    for (bssize_t j=0; j<numlookups; j++)
    {
        uint8_t palnum;

        if (1 != fp.Read(&palnum, 1))
            return -1;

        if (palnum >= 256-RESERVEDPALS)
        {
            Printf("ERROR: attempt to load lookup at reserved pal %d\n", palnum);
            return -2;
        }

        if (256 != fp.Read(remapbuf, 256))
            return -1;

        paletteMakeLookupTable(palnum, remapbuf, 0, 0, 0, 0);
    }

    return 0;
}

void paletteSetupDefaultFog(void)
{
    // Find a gap of four consecutive unused pal numbers to generate fog shade
    // tables.
    for (bssize_t j=1; j<=255-3; j++)
        if (!lookuptables[j] && !lookuptables[j+1] && !lookuptables[j+2] && !lookuptables[j+3])
        {
            paletteMakeLookupTable(j, NULL, 60, 60, 60, 1);
            paletteMakeLookupTable(j+1, NULL, 60, 0, 0, 1);
            paletteMakeLookupTable(j+2, NULL, 0, 60, 0, 1);
            paletteMakeLookupTable(j+3, NULL, 0, 0, 60, 1);

            break;
        }
}

void palettePostLoadLookups(void)
{
    // Alias remaining unused pal numbers to the base shade table.
    for (bssize_t j=1; j<MAXPALOOKUPS; j++)
    {
        // If an existing lookup is identical to #0, free it.
        if (lookuptables[j] && lookuptables[j] != lookuptables[0] && !Bmemcmp(lookuptables[0], lookuptables[j], 256*numshades))
            paletteFreeLookupTable(j);

        if (!lookuptables[j])
            paletteMakeLookupTable(j, NULL, 0, 0, 0, 1);
    }
}

static int32_t palookup_isdefault(int32_t palnum)  // KEEPINSYNC engine.lua
{
    return (lookuptables[palnum] == NULL || (palnum!=0 && lookuptables[palnum] == lookuptables[0]));
}

static void maybe_alloc_palookup(int32_t palnum)
{
    if (palookup_isdefault(palnum))
    {
        alloc_palookup(palnum);
        if (lookuptables[palnum] == NULL)
            Bexit(1);
    }
}

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

void setBlendFactor(int index, int alpha)
{
	if (index >= 0 && index < MAXBLENDTABS)
	{
		auto& myblend = glblend[index];
		if (index >= 0)
		{
			myblend.def[0].alpha = index / 255.f;
			myblend.def[1].alpha = 1.f - (index / 255.f);
			myblend.def[0].src = myblend.def[1].src = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
			myblend.def[0].dst = myblend.def[1].dst = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
		}
		else
		{
			myblend.def[0].alpha = 1;
			myblend.def[1].alpha = 1;
			myblend.def[0].src = myblend.def[1].src = BLENDFACTOR_ONE;
			myblend.def[0].dst = myblend.def[1].dst = BLENDFACTOR_ONE;
		}
	}
}

FRenderStyle GetBlend(int blend, int def)
{
    static uint8_t const blendFuncTokens[NUMBLENDFACTORS] =
    {
        STYLEALPHA_Zero,
        STYLEALPHA_One,
        STYLEALPHA_SrcCol,
        STYLEALPHA_InvSrcCol,
        STYLEALPHA_Src,
        STYLEALPHA_InvSrc,
        STYLEALPHA_Dst,
        STYLEALPHA_InvDst,
        STYLEALPHA_DstCol,
        STYLEALPHA_InvDstCol,
    };
    FRenderStyle rs;
    rs.BlendOp = STYLEOP_Add;
    glblenddef_t const* const glbdef = glblend[blend].def + def;
    rs.SrcAlpha = blendFuncTokens[glbdef->src];
    rs.DestAlpha = blendFuncTokens[glbdef->dst];
    rs.Flags = 0;
    return rs;
}

void handle_blend(uint8_t enable, uint8_t blend, uint8_t def)
{
    if (!enable)
    {
        GLInterface.SetRenderStyle(LegacyRenderStyles[STYLE_Translucent]);
		return;
    }
    auto rs = GetBlend(blend, def);
    GLInterface.SetRenderStyle(rs);
}

float float_trans(uint32_t maskprops, uint8_t blend)
{
    switch (maskprops)
    {
    case DAMETH_TRANS1:
    case DAMETH_TRANS2:
        return glblend[blend].def[maskprops - 2].alpha;
    default:
        return 1.0f;
    }
}

#endif

int32_t paletteSetLookupTable(int32_t palnum, const uint8_t *shtab)
{
    if (shtab != NULL)
    {
        maybe_alloc_palookup(palnum);
        Bmemcpy(lookuptables[palnum], shtab, 256*numshades);
    }

    return 0;
}

void paletteFreeLookupTable(int32_t const palnum)
{
    if (palnum == 0 && lookuptables[palnum] != NULL)
    {
        for (bssize_t i = 1; i < MAXPALOOKUPS; i++)
            if (lookuptables[i] == lookuptables[palnum])
                lookuptables[i] = NULL;

        ALIGNED_FREE_AND_NULL(lookuptables[palnum]);
    }
    else if (lookuptables[palnum] == lookuptables[0])
        lookuptables[palnum] = NULL;
    else
        ALIGNED_FREE_AND_NULL(lookuptables[palnum]);
}

//
// makepalookup
//
void paletteMakeLookupTable(int32_t palnum, const char *remapbuf, uint8_t r, uint8_t g, uint8_t b, char noFloorPal)
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
            lookuptables[palnum] = lookuptables[0];  // Alias to base shade table!
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
                const char *src = lookuptables[0];
                lookuptables[palnum][256*j + i] = src[256*j + remapbuf[i]];
            }
    }
    else
    {
        // colored fog case

        char *ptr2 = lookuptables[palnum];

        for (i=0; i<numshades; i++)
        {
            int32_t palscale = divscale16(i, numshades-1);

            for (j=0; j<256; j++)
            {
                PalEntry pe = GPalette.BaseColors[remapbuf[j]];
                *ptr2++ = ColorMatcher.Pick(pe.r + mulscale16(r-pe.r, palscale),
                    pe.g + mulscale16(g-pe.g, palscale),
                    pe.b + mulscale16(b-pe.b, palscale));
            }
        }
    }

#if defined(USE_OPENGL)
    palookupfog[palnum].r = r;
    palookupfog[palnum].g = g;
    palookupfog[palnum].b = b;
	palookupfog[palnum].f = 1;
#endif
}

//
// setbrightness
//
// flags:
//  1: don't setpalette(),  not checked anymore.
//  2: don't gltexinvalidateall()
//  4: don't calc curbrightness from dabrightness,  DON'T USE THIS FLAG!
//  8: don't gltexinvalidate8()
// 16: don't reset palfade*
// 32: apply brightness to scene in OpenGL
void videoSetPalette(int dabrightness, int dapalid, ESetPalFlags flags)
{
	if (GPalette.GetTranslation(Translation_BasePalettes, dapalid) == nullptr)
		dapalid = 0;
	curbasepal = dapalid;

	// In-scene brightness mode for RR's thunderstorm. This shouldn't affect the global gamma ramp.
	if ((videoGetRenderMode() >= REND_POLYMOST) && (flags & Pal_SceneBrightness))
	{
    	r_scenebrightness = clamp(dabrightness, 0, 15);
	}
	else
	{
		r_scenebrightness = 0;
	}

	if ((flags & Pal_DontResetFade) == 0)
	{
		palfadergb.r = palfadergb.g = palfadergb.b = 0;
		palfadedelta = 0;
	}

    curpaletteflags = flags;
}

//
// setpalettefade
//
void videoFadePalette(uint8_t r, uint8_t g, uint8_t b, uint8_t offset)
{
    palfadergb.r = r;
    palfadergb.g = g;
    palfadergb.b = b;
    palfadedelta = offset;
}

void paletteFreeAll()
{
    paletteloaded = 0;

    for (bssize_t i = 0; i < MAXPALOOKUPS; i++)
        if (i == 0 || lookuptables[i] != lookuptables[0])
        {
            // Take care of handling aliased ^^^ cases!
            Xaligned_free(lookuptables[i]);
        }
    Bmemset(lookuptables, 0, sizeof(lookuptables));
}