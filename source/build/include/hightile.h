#ifndef HIGHTILE_PRIV_H
#define HIGHTILE_PRIV_H

typedef struct {
    polytintflags_t f;
    uint8_t r, g, b;
    uint8_t sr, sg, sb;
} polytint_t;

extern polytint_t hictinting[MAXPALOOKUPS];

static inline int have_basepal_tint(void)
{
    polytint_t const & tint = hictinting[MAXPALOOKUPS-1];
    return (tint.r != 255 ||
            tint.g != 255 ||
            tint.b != 255);
}

static inline void hictinting_apply(float *color, int32_t palnum)
{
    polytint_t const & tint = hictinting[palnum];
    color[0] *= (float)tint.r * (1.f/255.f);
    color[1] *= (float)tint.g * (1.f/255.f);
    color[2] *= (float)tint.b * (1.f/255.f);
}

static inline void hictinting_apply_ub(uint8_t *color, int32_t palnum)
{
    polytint_t const & tint = hictinting[palnum];
    color[0] = (uint8_t)(color[0] * (float)tint.r * (1.f/255.f));
    color[1] = (uint8_t)(color[1] * (float)tint.g * (1.f/255.f));
    color[2] = (uint8_t)(color[2] * (float)tint.b * (1.f/255.f));
}

static inline void globaltinting_apply(float *color)
{
    color[0] *= (float)globalr * (1.f/255.f);
    color[1] *= (float)globalg * (1.f/255.f);
    color[2] *= (float)globalb * (1.f/255.f);
}

static inline void globaltinting_apply_ub(uint8_t *color)
{
    color[0] = (uint8_t)(color[0] * (float)globalr * (1.f/255.f));
    color[1] = (uint8_t)(color[1] * (float)globalg * (1.f/255.f));
    color[2] = (uint8_t)(color[2] * (float)globalb * (1.f/255.f));
}


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
    HICTINT_NOFOGSHADE = 512,

    HICTINT_PRECOMPUTED = HICTINT_COLORIZE | HICTINT_BLENDMASK,
    HICTINT_IN_MEMORY = HICTINT_PRECOMPUTED | HICTINT_GRAYSCALE | HICTINT_INVERT,

    HICTINT_MEMORY_COMBINATIONS = 1<<5,
};

#endif
