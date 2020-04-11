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

#define MAXBASEPALS 256
#define MAXPALOOKUPS 256
#define MAXBLENDTABS 256

#define RESERVEDPALS 4 // don't forget to increment this when adding reserved pals
#define DETAILPAL   (MAXPALOOKUPS - 1)
#define GLOWPAL     (MAXPALOOKUPS - 2)
#define SPECULARPAL (MAXPALOOKUPS - 3)
#define NORMALPAL   (MAXPALOOKUPS - 4)
#define BRIGHTPAL	(MAXPALOOKUPS)

extern uint8_t curbasepal;

extern uint8_t PaletteIndexFullbrights[32];


inline bool IsPaletteIndexFullbright(uint8_t col)
{
	return (PaletteIndexFullbrights[col >> 3] & (1u << (col & 7)));
}

inline void SetPaletteIndexFullbright(int col)
{
	PaletteIndexFullbrights[col >> 3] |= (1u << (col & 7));
}

struct palette_t 
{
    uint8_t r, g, b, f;
};
typedef struct {
    uint8_t r, g, b;
} rgb24_t;
extern palette_t curpalette[256], palfadergb;

extern unsigned char palfadedelta;
void paletteMakeLookupTable(int32_t palnum, const char *remapbuf, uint8_t r, uint8_t g, uint8_t b, char noFloorPal);
void paletteSetColorTable(int32_t id, uint8_t const *table, bool transient = false);
void paletteFreeColorTable(int32_t id);
void paletteFreeColorTables();
int32_t paletteSetLookupTable(int32_t palnum, const uint8_t *shtab);
void paletteFreeLookupTable(int32_t palnum);

#include "tflags.h"
enum ESetPalFlag
{
    Pal_DontResetFade = 1,
    Pal_SceneBrightness = 2,
    Pal_Fullscreen = 4,
    Pal_2D = 8,
};

typedef TFlags<ESetPalFlag> ESetPalFlags;
    DEFINE_TFLAGS_OPERATORS(ESetPalFlags)

extern ESetPalFlags curpaletteflags;

void videoSetPalette(int dabrightness, int dapalid, ESetPalFlags flags);
void videoFadePalette(uint8_t r, uint8_t g, uint8_t b, uint8_t offset);
#ifdef USE_OPENGL
void videoTintBlood(int32_t r, int32_t g, int32_t b);
#endif

extern int32_t realmaxshade;
extern float frealmaxshade;

extern int32_t globalpal;
extern int32_t globalblend;
extern uint32_t g_lastpalettesum;
extern palette_t paletteGetColor(int32_t col);
extern void paletteLoadFromDisk(void);
extern void palettePostLoadTables(void);
extern uint8_t basepalreset;
extern int32_t curbrightness;

extern int32_t paletteLoadLookupTable(FileReader &fp);
extern void paletteSetupDefaultFog(void);
void paletteFreeLookups();
extern void palettePostLoadLookups(void);
extern void paletteFixTranslucencyMask(void);

extern int8_t g_noFloorPal[MAXPALOOKUPS];

extern char britable[16][256];

#ifdef USE_OPENGL
extern palette_t palookupfog[MAXPALOOKUPS];

static inline void bricolor(palette_t *wpptr, int32_t dacol)
{
    wpptr->r = curpalette[dacol].r;
    wpptr->g = curpalette[dacol].g;
    wpptr->b = curpalette[dacol].b;
}

enum
{
    BLENDFACTOR_ZERO = 0,
    BLENDFACTOR_ONE,
    BLENDFACTOR_SRC_COLOR,
    BLENDFACTOR_ONE_MINUS_SRC_COLOR,
    BLENDFACTOR_SRC_ALPHA,
    BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
    BLENDFACTOR_DST_ALPHA,
    BLENDFACTOR_ONE_MINUS_DST_ALPHA,
    BLENDFACTOR_DST_COLOR,
    BLENDFACTOR_ONE_MINUS_DST_COLOR,
    NUMBLENDFACTORS,
};

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

FRenderStyle GetBlend(int blend, int def);
extern void handle_blend(uint8_t enable, uint8_t blend, uint8_t def);
float float_trans(uint32_t maskprops, uint8_t blend);

#endif

#endif
