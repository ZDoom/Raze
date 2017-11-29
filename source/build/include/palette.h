#pragma once

#ifndef palette_h_
#define palette_h_

#ifdef __cplusplus
extern "C" {
#endif

#define MAXBASEPALS 256
#define MAXPALOOKUPS 256
#define MAXBLENDTABS 256

#define RESERVEDPALS 4 // don't forget to increment this when adding reserved pals
#define DETAILPAL   (MAXPALOOKUPS - 1)
#define GLOWPAL     (MAXPALOOKUPS - 2)
#define SPECULARPAL (MAXPALOOKUPS - 3)
#define NORMALPAL   (MAXPALOOKUPS - 4)

extern uint8_t curbasepal;

#ifdef LUNATIC
extern const char *(getblendtab) (int32_t blend);
#else
#define getblendtab(blend) (blendtable[blend])
#endif

extern uint32_t PaletteIndexFullbrights[8];
#define IsPaletteIndexFullbright(col) (PaletteIndexFullbrights[(col)>>5] & (1u<<((col)&31)))
#define SetPaletteIndexFullbright(col) (PaletteIndexFullbrights[(col)>>5] |= (1u<<((col)&31)))

typedef struct {
    char r, g, b, f;
} palette_t;
typedef struct {
    uint8_t r, g, b;
} rgb24_t;
extern palette_t curpalette[256], curpalettefaded[256], palfadergb;
extern char palfadedelta;

extern int32_t realmaxshade;
extern float frealmaxshade;

extern int32_t globalblend;
extern uint32_t g_lastpalettesum;
extern palette_t getpal(int32_t col);
extern void loadpalette(void);
extern void E_PostLoadPalette(void);
extern void setup_blend(int32_t blend, int32_t doreverse);
extern uint8_t basepalreset;
extern int32_t curbrightness, gammabrightness;

extern int32_t loadlookups(int32_t fp);
extern void generatefogpals(void);
extern void fillemptylookups(void);
extern void E_ReplaceTransparentColorWithBlack(void);

extern int8_t g_noFloorPal[MAXPALOOKUPS];

extern char britable[16][256];

#ifdef USE_OPENGL
extern palette_t palookupfog[MAXPALOOKUPS];

static inline void bricolor(palette_t *wpptr, int32_t dacol)
{
    if (gammabrightness)
    {
        wpptr->r = curpalette[dacol].r;
        wpptr->g = curpalette[dacol].g;
        wpptr->b = curpalette[dacol].b;
    }
    else
    {
        wpptr->r = britable[curbrightness][curpalette[dacol].r];
        wpptr->g = britable[curbrightness][curpalette[dacol].g];
        wpptr->b = britable[curbrightness][curpalette[dacol].b];
    }
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

extern void handle_blend(uint8_t enable, uint8_t blend, uint8_t def);
#endif

#ifdef __cplusplus
}
#endif

#endif
