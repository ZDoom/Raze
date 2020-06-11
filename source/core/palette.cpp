/*
** palette.cpp
**
**---------------------------------------------------------------------------
** Copyright 2019-2020 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/ 

#include "build.h"
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

LookupTableInfo lookups;

uint8_t curbasepal;
int32_t globalblend;
PalEntry palfadergb;

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

    // Base palette
    uint8_t palette[768];
    if (768 != fil.Read(palette, 768))
        return;

    for (auto & pe : palette)
        pe <<= 2;

    paletteSetColorTable(0, palette, false, false);
    paletteloaded |= PALETTE_MAIN;

    // LameDuke and Witchaven use an older variant.
    if (fil.GetLength() == 41600)
    {
        numshades = 32;
    }
    else
    {
        // Shade tables
        numshades = fil.ReadInt16();

        if (numshades <= 1)
        {
            Printf("Warning: Invalid number of shades in \"palette.dat\"!\n");
            numshades = 0;
            return;
        }
    }

    // Read base shade table (lookuptables 0).
    int length = numshades * 256;
    auto buffer = fil.Read(length);
    if (buffer.Size() != length) return;
    lookups.setTable(0, buffer.Data());

    paletteloaded |= PALETTE_SHADE | PALETTE_TRANSLUC;
}

//==========================================================================
//
// postprocess the palette data after everything has been loaded
//
//==========================================================================

void LookupTableInfo::postLoadTables(void)
{
    globalpal = 0;
    GPalette.GenerateGlobalBrightmapFromColormap(getTable(0), numshades);

    // Try to detect fullbright translations. Unfortunately this cannot be used to detect fade strength because of loss of color precision in the palette map.
    for (int j = 0; j < MAXPALOOKUPS; j++)
    {
        auto lookup = tables[j].Shades;
        if (lookup.Len() > 0)
        {
            auto basetable = (uint8_t*)lookup.GetChars();
            auto midtable = basetable + ((numshades / 2) - 1) * 256;
            int lumibase = 0, lumimid = 0;
            for (int i = 1; i < 255; i++)   // intentionally leave out 0 and 255, because the table here is not translucency adjusted to the palette.
            {
                lumibase += GPalette.BaseColors[basetable[i]].Amplitude();
                lumimid += GPalette.BaseColors[midtable[i]].Amplitude();
            }
            float divider = float(lumimid) / float(lumibase);
            bool isbright = false;
            if (divider > 0.9)
            {
                tables[j].ShadeFactor = 1 / 10000.f;   // this translation is fullbright.
            }
            else
            {
                if (tables[j].ShadeFactor == 0) tables[j].ShadeFactor = 1.f;
                // Fullbright lookups do not need brightmaps.
                auto fog = tables[j].FadeColor;
                if (GPalette.HasGlobalBrightmap && fog.r == 0 && fog.g == 0 && fog.b == 0)
                {
                    isbright = true;
                    // A translation is fullbright only if all fullbright colors in the base table are mapped to another fullbright color.
                    auto brightmap = GPalette.GlobalBrightmap.Remap;
                    for (int i = 1; i < 255; i++)   // This also ignores entries 0 and 255 for the same reason as above.
                    {
                        int map = basetable[i];
                        if (brightmap[i] == GPalette.WhiteIndex && brightmap[map] != GPalette.WhiteIndex)
                        {
                            isbright = false;
                            break;
                        }
                    }
                }
            }
            tables[j].hasBrightmap = isbright;
            DPrintf(DMSG_NOTIFY, "Lookup %d is %sbright\n", j, isbright ? "" : "not ");
        }
    }
}

//==========================================================================
//
// load the lookup tables from lookup.dat
//
//==========================================================================

int32_t LookupTableInfo::loadTable(FileReader &fp)
{
    uint8_t buffer[256];
    int numlookups = fp.ReadUInt8();
    if (numlookups < 1)
        return -1;

    for (int j=0; j<numlookups; j++)
    {
        int palnum = fp.ReadUInt8();

        if (256 != fp.Read(buffer, 256))
            return -1;

        if (palnum < 0 || palnum >= 256 - RESERVEDPALS)
        {
            Printf("ERROR: attempt to load lookup at invalid index %d\n", palnum);
        }
        else
            makeTable(palnum, buffer, 0, 0, 0, 0);
    }

    return 0;
}

//==========================================================================
//
// Find a gap of four consecutive unused pal numbers to generate fog shade tables.
//
//==========================================================================

void LookupTableInfo::setupDefaultFog(void)
{
    for (int j = 1; j <= 255 - 3; j++)
    {
        if (tables[j].Shades.IsEmpty() && tables[j+1].Shades.IsEmpty() && tables[j + 2].Shades.IsEmpty() && tables[j + 3].Shades.IsEmpty())
        {
            makeTable(j, NULL, 60, 60, 60, 1);
            makeTable(j + 1, NULL, 60, 0, 0, 1);
            makeTable(j + 2, NULL, 0, 60, 0, 1);
            makeTable(j + 3, NULL, 0, 0, 60, 1);
            break;
        }
    }
}

//==========================================================================
//
// post process the lookup tables once everything has been loaded
//
//==========================================================================

void LookupTableInfo::postLoadLookups()
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
                if (!tables[l].Shades.IsEmpty())
                {
                    const uint8_t* lookup = (uint8_t*)tables[l].Shades.GetChars();
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
    // Assuming that color 255 is always transparent, do the following:
    // Copy color 0 to color 255
    // Set color 0 to transparent black
    // Swap all remap entries from 0 to 255 and vice versa
    // Always map 0 to 0.

    auto colorswap = [](FRemapTable* remap)
    {
        remap->Palette[255] = remap->Palette[0];
        remap->Palette[0] = 0;
        remap->Remap[255] = remap->Remap[0];
        for (auto& c : remap->Remap)
        {
            if (c == 0) c = 255;
            else if (c == 255) c = 0;
        }
        remap->Remap[0] = 0;
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

int32_t LookupTableInfo::setTable(int palnum, const uint8_t *shtab)
{
    if (shtab != NULL)
    {
        int length = numshades * 256;
        tables[palnum].Shades = FString((const char*)shtab, length);
    }

    return 0;
}

//==========================================================================
//
// creates a lookup table from scratch
//
//==========================================================================

void LookupTableInfo::makeTable(int palnum, const uint8_t *remapbuf, int r, int g, int b, bool noFloorPal)
{
    uint8_t idmap[256];

    // NOTE: palnum==0 is allowed
    if (paletteloaded == 0 || (unsigned)palnum >= MAXPALOOKUPS)
        return;

    tables[palnum].noFloorPal = noFloorPal;

    if (remapbuf == nullptr)
    {
        if (r == 0 && g == 0 && b == 0)
        {
            clearTable(palnum);
            return;
        }

        for (int i = 0; i < 256; i++) idmap[i] = i;
        remapbuf = idmap;
    }

    int length = numshades * 256;
    auto p = tables[palnum].Shades.LockNewBuffer(length);
    if (r == 0 && g == 0 && b == 0)
    {
        // "black fog"/visibility case -- only remap color indices

        auto src = getTable(0);

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
            int colfac = (numshades - i);
            for (int j = 0; j < 256; j++)
            {
                PalEntry pe = GPalette.BaseColors[remapbuf[j]];
                p[256 * i + j] = ColorMatcher.Pick(
                    (pe.r * colfac + r * i) / numshades,
                    (pe.g * colfac + g * i) / numshades,
                    (pe.b * colfac + b * i) / numshades);
            }
        }
    }

    tables[palnum].FadeColor.r = r;
    tables[palnum].FadeColor.g = g;
    tables[palnum].FadeColor.b = b;
    tables[palnum].FadeColor.a = 1;
}


//==========================================================================
//
// hicsetpalettetint(pal,r,g,b,sr,sg,sb,effect)
//   The tinting values represent a mechanism for emulating the effect of global sector
//   palette shifts on true-colour textures and only true-colour textures.
//   effect bitset: 1 = greyscale, 2 = invert
//
//==========================================================================

void LookupTableInfo::setPaletteTint(int palnum, int r, int g, int b, int sr, int sg, int sb, int flags)
{
    if ((unsigned)palnum >= MAXPALOOKUPS) return;

    auto &lookup = tables[palnum];
    lookup.tintColor = PalEntry(r, g, b);
    lookup.tintShade = PalEntry(sr, sg, sb);
    lookup.tintFlags = flags | TINTF_ENABLE;
}

//==========================================================================
//
//
//
//==========================================================================

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
