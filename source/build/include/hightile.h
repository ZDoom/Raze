#ifndef HIGHTILE_PRIV_H
#define HIGHTILE_PRIV_H

#include "palentry.h"

typedef struct {
    polytintflags_t f;
    PalEntry tint;
    PalEntry shade;
} polytint_t;

extern polytint_t hictinting[MAXPALOOKUPS];


// replacement flags
enum
{
    HICR_FORCEFILTER = 2,
};

// hictinting[].f / gloadtile_hi() and mdloadskin() <effect> arg bits
enum
{
    HICTINT_GRAYSCALE = 1,
    HICTINT_INVERT = 2,
    HICTINT_COLORIZE = 4,
    HICTINT_USEONART = 8,
    HICTINT_APPLYOVERPALSWAP = 16,
    HICTINT_APPLYOVERALTPAL = 32,

    HICTINT_BLEND_MULTIPLY = 0<<6,
    HICTINT_BLEND_SCREEN = 1<<6,
    HICTINT_BLEND_OVERLAY = 2<<6,
    HICTINT_BLEND_HARDLIGHT = 3<<6,

    HICTINT_BLENDMASK = 64|128,
    HICTINT_ALWAYSUSEART = 256,
    HICTINT_PRECOMPUTED = HICTINT_COLORIZE | HICTINT_BLENDMASK,
    HICTINT_ENABLE = 32768
};

#endif
