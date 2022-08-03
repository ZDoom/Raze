// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#pragma once

#ifndef palette_h_
#define palette_h_

#include "renderstyle.h"
#include "filesystem.h"
#include "zstring.h"
#include "palentry.h"


enum
{
    MAXBASEPALS = 256,
    MAXPALOOKUPS = 256,
    MAXBLENDTABS = 256,

    RESERVEDPALS = 4, // don't forget to increment this when adding reserved pals
    DETAILPAL   = (MAXPALOOKUPS - 1),
    GLOWPAL     = (MAXPALOOKUPS - 2),
    SPECULARPAL = (MAXPALOOKUPS - 3),
    NORMALPAL   = (MAXPALOOKUPS - 4),
    BRIGHTPAL	= (MAXPALOOKUPS),

    MAXREALPAL = MAXPALOOKUPS - RESERVEDPALS
};

// fixme: should use the flags from the PRSFlags enum directly
enum
{
    TINTF_GRAYSCALE = 1,
    TINTF_INVERT = 2,
    TINTF_COLORIZE = 4,
    TINTF_USEONART = 8,
    TINTF_APPLYOVERPALSWAP = 16,
    TINTF_APPLYOVERALTPAL = 32,

    TINTF_BLEND_MULTIPLY = 0 << 6,
    TINTF_BLEND_SCREEN = 1 << 6,
    TINTF_BLEND_OVERLAY = 2 << 6,
    TINTF_BLEND_HARDLIGHT = 3 << 6,

    TINTF_BLENDMASK = 64 | 128,
    TINTF_ALWAYSUSEART = 256,
    TINTF_PRECOMPUTED = TINTF_COLORIZE | TINTF_BLENDMASK,
    TINTF_ENABLE = 32768
};

struct LookupTable
{
    FString Shades;
    PalEntry FadeColor = 0;
    float ShadeFactor = 1.f;
    float Visibility = 0;
    bool hasBrightmap = false;
    bool noFloorPal = false;

    int tintFlags = 0;
    PalEntry tintColor = 0xffffff;
    PalEntry tintShade = 0;
};

struct LookupTableInfo
{
    LookupTable tables[MAXPALOOKUPS];

    const uint8_t* getTable(int num)
    {
        if (tables[num].Shades.Len() == 0) num = 0;
        return (const uint8_t*)tables[num].Shades.GetChars();
    }

    bool checkTable(int num)
    {
        return tables[num].Shades.IsNotEmpty();
    }

    void copyTable(int dest, int src)
    {
        tables[dest].Shades = tables[src].Shades;
    }

    void clearTable(int dest)
    {
        tables[dest].Shades = "";
    }

    void makeTable(int palnum, const uint8_t* remapbuf, int r, int g, int b, bool noFloorPal);
    int setTable(int palnum, const uint8_t* remap);
    void postLoadTables();
    int loadTable(FileReader& fp);
    void postLoadLookups();
    void setupDefaultFog();

    void setFadeColor(int num, int r, int g, int b)
    {
        tables[num].FadeColor = PalEntry(1, r, g, b);
    }

    PalEntry getFade(int num)
    {
        return tables[num].FadeColor;
    }

    bool noFloorPal(int num) const
    {
        return tables[num].noFloorPal;
    }

    void setPaletteTint(int palnum, int r, int g, int b, int sr, int sg, int sb, int flags);


};

extern LookupTableInfo lookups;

enum
{
    Translation_BasePalettes = 1,
    Translation_Remap,
};

extern uint8_t curbasepal;
extern int32_t r_scenebrightness;

struct palette_t 
{
    uint8_t r, g, b, f;
};

extern PalEntry palfadergb;

void paletteSetColorTable(int32_t id, uint8_t const* table, bool notransparency, bool twodonly);

#include "tflags.h"
enum ESetPalFlag
{
    Pal_DontResetFade = 1,
};

inline void videoSetBrightness(int brightness)
{
    r_scenebrightness = clamp(brightness, 0, 15);
}

typedef TFlags<ESetPalFlag> ESetPalFlags;
    DEFINE_TFLAGS_OPERATORS(ESetPalFlags)

void videoSetPalette(int dapalid);
inline void videoFadePalette(uint8_t r, uint8_t g, uint8_t b, uint8_t offset)
{
    palfadergb.r = r;
    palfadergb.g = g;
    palfadergb.b = b;
    palfadergb.a = offset;
}
inline void videoclearFade()
{
    palfadergb.d = 0;
}


void videoTintBlood(int32_t r, int32_t g, int32_t b);

extern int numshades;
extern void paletteLoadFromDisk(void);


struct glblenddef_t
{
    float alpha;
    uint8_t src, dst, flags;
};

struct glblend_t
{
    glblenddef_t def[2];
};

extern glblend_t const nullglblend, defaultglblend;
extern glblend_t glblend[MAXBLENDTABS];

enum {
    DAMETH_TRANS1 = 2,
    DAMETH_TRANS2 = 3,
};

FRenderStyle GetRenderStyle(int blend, int def);
float GetAlphaFromBlend(uint32_t maskprops, uint32_t blend);
void DrawFullscreenBlends();

#endif
