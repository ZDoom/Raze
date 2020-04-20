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
#include "m_swap.h"
#include "v_colortables.h"
#include "v_font.h"
#include "../../glbackend/glbackend.h"

// FString is a nice and convenient way to have automatically managed shared storage.
FString LookupTables[MAXPALOOKUPS];

uint8_t curbasepal;
int32_t globalblend;

PalEntry palfadergb;

#if defined(USE_OPENGL)
palette_t palookupfog[MAXPALOOKUPS];
#endif

// For every pal number, whether tsprite pal should not be taken over from
// floor pal.
// NOTE: g_noFloorPal[0] is irrelevant as it's never checked.
int8_t g_noFloorPal[MAXPALOOKUPS];


//==========================================================================
//
// Adds a palette to the global list of base palettes
//
//==========================================================================

void paletteSetColorTable(int32_t id, uint8_t const* table, bool notransparency, bool twodonly)
{
    if (id == 0)
    {
        GPalette.SetPalette(table, 255);
        GPalette.BaseColors[255] = 0;
        BuildTransTable(GPalette.BaseColors);
    }
    FRemapTable remap;
    remap.AddColors(0, 256, table, 255);
    if (!notransparency)
    {
        remap.Palette[255] = 0;
        remap.Remap[255] = 255;
    }
    remap.Inactive = twodonly;  // use Inactive as a marker for the postprocessing so that for pure 2D palettes the creation of shade tables can be skipped.
    GPalette.UpdateTranslation(TRANSLATION(Translation_BasePalettes, id), &remap);
}

//==========================================================================
//
// loads the main palette file.
//
//==========================================================================

void paletteLoadFromDisk(void)
{
    for (auto & x : glblend)
        x = defaultglblend;

	auto fil = fileSystem.OpenFileReader("palette.dat");
	if (!fil.isOpen())
        return;


    // PALETTE_MAIN

    uint8_t palette[768];
    if (768 != fil.Read(palette, 768))
        return;

    for (unsigned char & k : palette)
        k <<= 2;

    paletteSetColorTable(0, palette, false, false);
    paletteloaded |= PALETTE_MAIN;

    // PALETTE_SHADES
    numshades = fil.ReadInt16();

    if (numshades <= 1)
    {
        Printf("Warning: Invalid number of shades in \"palette.dat\"!\n");
        numshades = 0;
        return;
    }

#if 0
    // Reminder: Witchaven's shade table has no index and no easy means to autodetect.
    if (numshades == 0 && (g_gameType & GAMEFLAG_WITCHAVEN))
    {
        numshades = 32;
        fil.Seek(-2, FileReader::SeekCur);
    }
    else
#endif
    {
        // LameDuke's is yet another variant.
        if (numshades >= 256)
        {
            uint16_t temp = fil.ReadUInt16();
            if (temp == 770 || numshades > 256) // 02 03
            {
                fil.Seek(-4, FileReader::SeekCur);
                numshades = 32;
            }
            else
            {
                fil.Seek(-2, FileReader::SeekCur);
            }
        }
    }


    // Read base shade table (lookuptables 0).
    int length = numshades * 256;
    auto buf = fil.Read();
    LookupTables[0] = FString((char*)buf.Data(), buf.Size());
    if (buf.Size() != length)
        return;

    paletteloaded |= PALETTE_SHADE;
    paletteloaded |= PALETTE_TRANSLUC;
}

//==========================================================================
//
// postprocess the palette data after everything has been loaded
//
//==========================================================================

void palettePostLoadTables(void)
{
    globalpal = 0;
    GPalette.GenerateGlobalBrightmapFromColormap((const uint8_t*)LookupTables[0].GetChars(), numshades);
}

//==========================================================================
//
// Ensure that all lookups map 255 to itself to preserve transparency.
//
//==========================================================================

void paletteFixTranslucencyMask(void)
{
    for (auto &thispalookup : LookupTables)
    {
        if (thispalookup.IsEmpty())
            continue;

        for (int j = 0; j < numshades; j++)
        {
            auto p = thispalookup.LockBuffer();
            p[(j << 8) + 255] = 255;
            thispalookup.UnlockBuffer();
        }
    }
}

//==========================================================================
//
// load the lookup tables from lookup.dat
//
//==========================================================================

int32_t paletteLoadLookupTable(FileReader &fp)
{
    char remapbuf[256];
    int numlookups = fp.ReadUInt8();
    if (numlookups < 1)
        return -1;

    for (int j=0; j<numlookups; j++)
    {
        int palnum = fp.ReadUInt8();

        if (256 != fp.Read(remapbuf, 256))
            return -1;

        if (palnum >= 256 - RESERVEDPALS)
        {
            Printf("ERROR: attempt to load lookup at reserved pal %d\n", palnum);
        }
        else
            paletteMakeLookupTable(palnum, remapbuf, 0, 0, 0, 0);
    }

    return 0;
}

//==========================================================================
//
// Find a gap of four consecutive unused pal numbers to generate fog shade tables.
//
//==========================================================================

void paletteSetupDefaultFog(void)
{
    for (int j = 1; j <= 255 - 3; j++)
    {
        if (!LookupTables[j].IsEmpty() && !LookupTables[j + 1].IsEmpty() && !LookupTables[j + 2].IsEmpty() && !LookupTables[j + 3].IsEmpty())
        {
            paletteMakeLookupTable(j, NULL, 60, 60, 60, 1);
            paletteMakeLookupTable(j + 1, NULL, 60, 0, 0, 1);
            paletteMakeLookupTable(j + 2, NULL, 0, 60, 0, 1);
            paletteMakeLookupTable(j + 3, NULL, 0, 0, 60, 1);

            break;
        }
    }
}

//==========================================================================
//
// post process the lookup tables once everything has been loaded
//
//==========================================================================

void palettePostLoadLookups(void)
{
    int numpalettes = GPalette.NumTranslations(Translation_BasePalettes);
    if (numpalettes == 0) return;
    auto basepalette = GPalette.GetTranslation(Translation_BasePalettes, 0);

    for (int i = 0; i < numpalettes; i++)
    {
        auto palette = GPalette.GetTranslation(Translation_BasePalettes, i);
        if (!palette) continue;

        if (palette->Inactive)
        {
            palette->Inactive = false;
            GPalette.CopyTranslation(TRANSLATION(Translation_Remap + i, 0), TRANSLATION(Translation_BasePalettes, i));
        }
        else
        {
            for (int l = 0; l < MAXPALOOKUPS; l++)
            {
                if (!LookupTables[l].IsEmpty())
                {
                    const uint8_t* lookup = (uint8_t*)LookupTables[l].GetChars();
                    FRemapTable remap;
                    if (i == 0 || (palette != basepalette && !palette->Inactive))
                    {
                        memcpy(remap.Remap, lookup, 256);
                        for (int j = 0; j < 256; j++)
                        {
                            remap.Palette[j] = palette->Palette[remap.Remap[j]];
                        }
                        remap.NumEntries = 256;
                        GPalette.UpdateTranslation(TRANSLATION(i + Translation_Remap, l), &remap);
                    }
                    if (palette != basepalette) palette->Inactive = false;  // clear the marker flag
                }
            }
        }
    }

    // Swap colors 0 and 255 in all tables so that all paletted images have their transparent color at index 0.
    // This means: 
    // - Swap palette and remap entries in all stored remap tables
    // - change all remap entries of 255 to 0 and vice versa

    auto colorswap = [](FRemapTable* remap)
    {
        std::swap(remap->Palette[0], remap->Palette[255]);
        std::swap(remap->Remap[0], remap->Remap[255]);
        for (auto& c : remap->Remap)
        {
            if (c == 0) c = 255;
            else if (c == 255) c = 0;
        }
    };

    for (auto remap : GPalette.uniqueRemaps)
    {
        if (!remap->ForFont) colorswap(remap);
    }
    colorswap(&GPalette.GlobalBrightmap);
    std::swap(GPalette.BaseColors[0], GPalette.BaseColors[255]);
}

//==========================================================================
//
// set a lookup table from external data
//
//==========================================================================

int32_t paletteSetLookupTable(int32_t palnum, const uint8_t *shtab)
{
    if (shtab != NULL)
    {
        int length = numshades * 256;
        LookupTables[palnum] = FString((const char*)shtab, length);
    }

    return 0;
}

//==========================================================================
//
// creates a lookup table from scratch
//
//==========================================================================

void paletteMakeLookupTable(int32_t palnum, const char *remapbuf, uint8_t r, uint8_t g, uint8_t b, char noFloorPal)
{
    char idmap[256];

    // NOTE: palnum==0 is allowed
    if (paletteloaded == 0 || (unsigned)palnum >= MAXPALOOKUPS)
        return;

    g_noFloorPal[palnum] = noFloorPal;

    if (remapbuf == nullptr)
    {
        if (r == 0 || g == 0 || b == 0)
        {
            LookupTables[palnum] = ""; // clear this entry so that later it can be filled with the base remap.
            return;
        }

        for (int i = 0; i < 256; i++) idmap[i] = i;
        remapbuf = idmap;
    }

    int length = numshades * 256;
    TArray<uint8_t> p(length, true);
    if (r == 0 || g == 0 || b == 0)
    {
        // "black fog"/visibility case -- only remap color indices

        auto src = (const uint8_t*)LookupTables[0].GetChars();

        for (int j = 0; j < numshades; j++)
            for (int i = 0; i < 256; i++)
            {
                p[256 * j + i] = src[256 * j + remapbuf[i]];
            }
    }
    else
    {
        // colored fog case

        for (int i = 0; i < numshades; i++)
        {
            for (int j = 0; j < 256; j++)
            {
                PalEntry pe = GPalette.BaseColors[remapbuf[j]];
                p[j] = ColorMatcher.Pick(
                    pe.r + Scale(r - pe.r, i, numshades - 1),
                    pe.g + Scale(g - pe.g, i, numshades - 1),
                    pe.b + Scale(b - pe.b, i, numshades - 1));
            }
        }
    }
    LookupTables[palnum] = FString((char*)p.Data(), length);

#if defined(USE_OPENGL)
    palookupfog[palnum].r = r;
    palookupfog[palnum].g = g;
    palookupfog[palnum].b = b;
	palookupfog[palnum].f = 1;
#endif
}


void videoSetPalette(int dabrightness, int palid, ESetPalFlags flags)
{
	curbasepal = (GPalette.GetTranslation(Translation_BasePalettes, palid) == nullptr)? 0 : palid;
    if ((flags & Pal_DontResetFade) == 0) palfadergb = 0;
}

//==========================================================================
//
// map Build blend definitions to actual render style / alpha combos.
//
//==========================================================================

glblend_t const nullglblend =
{
    {
        { 1.f, STYLEALPHA_One, STYLEALPHA_Zero, 0 },
        { 1.f, STYLEALPHA_One, STYLEALPHA_Zero, 0 },
    },
};
glblend_t const defaultglblend =
{
    {
        { 2.f / 3.f, STYLEALPHA_Src, STYLEALPHA_InvSrc, 0 },
        { 1.f / 3.f, STYLEALPHA_Src, STYLEALPHA_InvSrc, 0 },
    },
};

glblend_t glblend[MAXBLENDTABS];

FRenderStyle GetRenderStyle(int blend, int def)
{
    FRenderStyle rs;
    rs.BlendOp = STYLEOP_Add;
    auto glbdef = &glblend[blend].def[def];
    rs.SrcAlpha = glbdef->src;
    rs.DestAlpha = glbdef->dst;
    rs.Flags = 0;
    return rs;
}

void SetRenderStyleFromBlend(uint8_t enable, uint8_t blend, uint8_t def)
{
    if (!enable)
    {
        GLInterface.SetRenderStyle(LegacyRenderStyles[STYLE_Translucent]);
        return;
    }
    auto rs = GetRenderStyle(blend, def);
    GLInterface.SetRenderStyle(rs);
}

float GetAlphaFromBlend(uint32_t method, uint32_t blend)
{
    return method == DAMETH_TRANS1 || method == DAMETH_TRANS2 ? glblend[blend].def[method - DAMETH_TRANS1].alpha : 1.f;
}
