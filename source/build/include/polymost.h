#ifndef polymost_h_
# define polymost_h_


#include "mdsprite.h"

void Polymost_CacheHitList(uint8_t* hash);

typedef struct { uint8_t r, g, b, a; } coltype;
typedef struct { float r, g, b, a; } coltypef;

extern int32_t rendmode;
extern float gtang;
extern double gxyaspect;
extern float grhalfxdown10x;
extern float gcosang, gsinang, gcosang2, gsinang2;

extern void Polymost_prepare_loadboard(void);

void polymost_outputGLDebugMessage(uint8_t severity, const char* format, ...);

//void phex(char v, char *s);
void polymost_drawsprite(int32_t snum);
void polymost_drawmaskwall(int32_t damaskwallcnt);
void polymost_dorotatespritemodel(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, uint8_t dapalnum, int32_t dastat, uint8_t daalpha, uint8_t dablend, int32_t uniqid);
void polymost_fillpolygon(int32_t npoints);
void polymost_initosdfuncs(void);
void polymost_drawrooms(void);
void polymost_prepareMirror(int32_t dax, int32_t day, int32_t daz, fixed_t daang, fixed_t dahoriz, int16_t mirrorWall);
void polymost_completeMirror();

int32_t polymost_maskWallHasTranslucency(uwalltype const * const wall);
int32_t polymost_spriteHasTranslucency(tspritetype const * const tspr);
int32_t polymost_spriteIsModelOrVoxel(tspritetype const * const tspr);

void polymost_glreset(void);

enum {
    INVALIDATE_ALL,
    INVALIDATE_ART,
    INVALIDATE_ALL_NON_INDEXED,
    INVALIDATE_ART_NON_INDEXED
};


extern float curpolygonoffset;

extern int32_t polymostcenterhoriz;

extern int16_t globalpicnum;

#define POLYMOST_CHOOSE_FOG_PAL(fogpal, pal) \
    ((fogpal) ? (fogpal) : (pal))
static FORCE_INLINE int32_t get_floor_fogpal(usectorptr_t const sec)
{
    return POLYMOST_CHOOSE_FOG_PAL(sec->fogpal, sec->floorpal);
}
static FORCE_INLINE int32_t get_ceiling_fogpal(usectorptr_t const sec)
{
    return POLYMOST_CHOOSE_FOG_PAL(sec->fogpal, sec->ceilingpal);
}
static FORCE_INLINE int32_t fogshade(int32_t const shade, int32_t const pal)
{
    return (globalflags & GLOBAL_NO_GL_FOGSHADE) ? 0 : shade;
}

static FORCE_INLINE int check_nonpow2(int32_t const x)
{
    return (x > 1 && (x&(x-1)));
}

static inline float polymost_invsqrt_approximation(float x)
{
#if !B_BIG_ENDIAN
    float const haf = x * .5f;
    union { float f; uint32_t i; } n = { x };
    n.i = 0x5f375a86 - (n.i >> 1);
    return n.f * (1.5f - haf * (n.f * n.f));
#else
    // this is the comment
    return 1.f / Bsqrtf(x);
#endif
}

// Flags of the <dameth> argument of various functions
enum {
    DAMETH_NOMASK = 0,
    DAMETH_MASK = 1,
    DAMETH_TRANS1 = 2,
    DAMETH_TRANS2 = 3,

    DAMETH_MASKPROPS = 3,

    DAMETH_CLAMPED = 4,
	DAMETH_MODEL = 8,
	DAMETH_SKY = 16,

    DAMETH_WALL = 32,  // signals a texture for a wall (for r_npotwallmode)

    // used internally by polymost_domost
    DAMETH_BACKFACECULL = -1,
};

#define DAMETH_NARROW_MASKPROPS(dameth) (((dameth)&(~DAMETH_TRANS1))|(((dameth)&DAMETH_TRANS1)>>1))
EDUKE32_STATIC_ASSERT(DAMETH_NARROW_MASKPROPS(DAMETH_MASKPROPS) == DAMETH_MASK);

extern float fcosglobalang, fsinglobalang;
extern float fxdim, fydim, fydimen, fviewingrange;
extern int32_t viewingrangerecip;

#endif
