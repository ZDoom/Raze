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
#include "templates.h"

#define MAXBASEPALS 256
#define MAXPALOOKUPS 256
#define MAXBLENDTABS 256

#define RESERVEDPALS 4 // don't forget to increment this when adding reserved pals
#define DETAILPAL   (MAXPALOOKUPS - 1)
#define GLOWPAL     (MAXPALOOKUPS - 2)
#define SPECULARPAL (MAXPALOOKUPS - 3)
#define NORMALPAL   (MAXPALOOKUPS - 4)
#define BRIGHTPAL	(MAXPALOOKUPS)

extern FString LookupTables[MAXPALOOKUPS];
inline const uint8_t *paletteGetLookupTable(int num)
{
    return (const uint8_t*)LookupTables[num].GetChars();
}

inline void paletteCopyLookupTable(int dest, int src)
{
    LookupTables[dest] = LookupTables[src];
}
inline bool paletteCheckLookupTable(int num)
{
    return LookupTables[num].Len() > 0;
}
inline void paletteClearLookupTable(int num)
{
    LookupTables[num] = "";
}

enum
{
    Translation_BasePalettes,
    Translation_Remap,
};

extern uint8_t curbasepal;
extern FixedBitArray<256> FullbrightIndices;
extern int32_t r_scenebrightness;

struct palette_t 
{
    uint8_t r, g, b, f;
};

extern PalEntry palfadergb;

void paletteMakeLookupTable(int32_t palnum, const uint8_t *remapbuf, uint8_t r, uint8_t g, uint8_t b, char noFloorPal);
void paletteSetColorTable(int32_t id, uint8_t const* table, bool notransparency, bool twodonly);
int32_t paletteSetLookupTable(int32_t palnum, const uint8_t *shtab);

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

void videoSetPalette(int dabrightness, int dapalid, ESetPalFlags flags);
inline void videoFadePalette(uint8_t r, uint8_t g, uint8_t b, uint8_t offset)
{
    palfadergb.r = r;
    palfadergb.g = g;
    palfadergb.b = b;
    palfadergb.a = offset;
}


#ifdef USE_OPENGL
void videoTintBlood(int32_t r, int32_t g, int32_t b);
#endif

extern int32_t globalpal;
extern int32_t globalblend;
extern void paletteLoadFromDisk(void);
extern void palettePostLoadTables(void);

extern int32_t paletteLoadLookupTable(FileReader &fp);
extern void paletteSetupDefaultFog(void);
void paletteFreeLookups();
extern void palettePostLoadLookups(void);
extern void paletteFixTranslucencyMask(void);

extern int8_t g_noFloorPal[MAXPALOOKUPS];

extern char britable[16][256];

#ifdef USE_OPENGL
extern palette_t palookupfog[MAXPALOOKUPS];

typedef struct glblenddef_
{
    float alpha;
    uint8_t src, dst, flags;
} glblenddef_t;

typedef struct glblend_
{
    glblenddef_t def[2];
} glblend_t;

extern glblend_t const nullglblend, defaultglblend;
extern glblend_t glblend[MAXBLENDTABS];

FRenderStyle GetRenderStyle(int blend, int def);
extern void SetRenderStyleFromBlend(uint8_t enable, uint8_t blend, uint8_t def);
float GetAlphaFromBlend(uint32_t maskprops, uint32_t blend);

#endif

#endif
