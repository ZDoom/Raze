#ifndef polymost_h_
# define polymost_h_


#include "baselayer.h"  // glinfo
#include "hightile.h"
#include "mdsprite.h"

void Polymost_CacheHitList(uint8_t* hash);

	class FHardwareTexture;
typedef struct { uint8_t r, g, b, a; } coltype;
typedef struct { float r, g, b, a; } coltypef;

extern bool playing_rr;
extern int32_t rendmode;
extern float gtang;
extern float glox1, gloy1;
extern double gxyaspect;
extern float grhalfxdown10x;
extern float gcosang, gsinang, gcosang2, gsinang2;
extern float gchang, gshang, gctang, gstang, gvisibility;
extern float gvrcorrection;

struct glfiltermodes {
    const char *name;
};
#define NUMGLFILTERMODES 6
extern struct glfiltermodes glfiltermodes[NUMGLFILTERMODES];

extern void Polymost_prepare_loadboard(void);

void polymost_outputGLDebugMessage(uint8_t severity, const char* format, ...);

//void phex(char v, char *s);
void uploadtexture(FHardwareTexture *tex, int32_t doalloc, vec2_t siz, int32_t texfmt, coltype *pic, vec2_t tsiz, int32_t dameth);
void uploadbasepalette(int32_t basepalnum, bool transient = false);
void uploadpalswaps(int count, int32_t *palookupnum);
void polymost_drawsprite(int32_t snum);
void polymost_drawmaskwall(int32_t damaskwallcnt);
void polymost_dorotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                             int8_t dashade, uint8_t dapalnum, int32_t dastat, uint8_t daalpha, uint8_t dablend, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, int32_t uniqid);
void polymost_fillpolygon(int32_t npoints);
void polymost_initosdfuncs(void);
void polymost_drawrooms(void);
void polymost_prepareMirror(int32_t dax, int32_t day, int32_t daz, fix16_t daang, fix16_t dahoriz, int16_t mirrorWall);
void polymost_completeMirror();

int32_t polymost_maskWallHasTranslucency(uwalltype const * const wall);
int32_t polymost_spriteHasTranslucency(uspritetype const * const tspr);

float* multiplyMatrix4f(float m0[4*4], const float m1[4*4]);

void polymost_glinit(void);
void polymost_glreset(void);

enum {
    INVALIDATE_ALL,
    INVALIDATE_ART,
    INVALIDATE_ALL_NON_INDEXED,
    INVALIDATE_ART_NON_INDEXED
};

void gltexinvalidate(int32_t dapicnum, int32_t dapalnum, int32_t dameth);
void gltexinvalidatetype(int32_t type);
int32_t polymost_printext256(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, const char *name, char fontsize);

extern float curpolygonoffset;

extern float shadescale;
extern int32_t shadescale_unbounded;
extern uint8_t alphahackarray[MAXTILES];

extern int32_t r_usenewshading;
extern int32_t r_brightnesshack;
extern int32_t polymostcenterhoriz;

extern int16_t globalpicnum;

// Compare with polymer_eligible_for_artmap()
static FORCE_INLINE int32_t eligible_for_tileshades(int32_t const picnum, int32_t const pal)
{
    return !usehightile || !hicfindsubst(picnum, pal, hictinting[pal].f & HICTINT_ALWAYSUSEART);
}

static inline float getshadefactor(int32_t const shade)
{
    // 8-bit tiles, i.e. non-hightiles and non-models, don't get additional
    // shading with r_usetileshades!
    if (videoGetRenderMode() == REND_POLYMOST && !(globalflags & GLOBAL_NO_GL_TILESHADES) && eligible_for_tileshades(globalpicnum, globalpal))
        return 1.f;

    if (r_usenewshading == 4)
        return max(min(1.f - (shade * shadescale / frealmaxshade), 1.f), 0.f);

    float const shadebound = (float)((shadescale_unbounded || shade>=numshades) ? numshades : numshades-1);
    float const scaled_shade = (float)shade*shadescale;
    float const clamped_shade = min(max(scaled_shade, 0.f), shadebound);

    return ((float)(numshades-clamped_shade))/(float)numshades;
}

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
    polytintflags_t const tintflags = hictinting[pal].f;
    return (globalflags & GLOBAL_NO_GL_FOGSHADE || tintflags & HICTINT_NOFOGSHADE) ? 0 : shade;
}

static FORCE_INLINE int check_nonpow2(int32_t const x)
{
    return (x > 1 && (x&(x-1)));
}

static inline float polymost_invsqrt_approximation(float x)
{
#ifdef B_LITTLE_ENDIAN
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

    DAMETH_WALL = 32,  // signals a texture for a wall (for r_npotwallmode)

    // used internally by polymost_domost
    DAMETH_BACKFACECULL = -1,
};

#define DAMETH_NARROW_MASKPROPS(dameth) (((dameth)&(~DAMETH_TRANS1))|(((dameth)&DAMETH_TRANS1)>>1))
EDUKE32_STATIC_ASSERT(DAMETH_NARROW_MASKPROPS(DAMETH_MASKPROPS) == DAMETH_MASK);

// pthtyp pth->flags bits
enum pthtyp_flags {
    PTH_HIGHTILE = 2,
    PTH_SKYBOX = 4,
    PTH_HASALPHA = 8,
    PTH_HASFULLBRIGHT = 16,
    PTH_NPOTWALL = DAMETH_WALL,  // r_npotwallmode=1 generated texture
    PTH_FORCEFILTER = 64,

    PTH_INVALIDATED = 128,

    PTH_INDEXED = 512,
    PTH_ONEBITALPHA = 1024,
};

typedef struct pthtyp_t
{
    struct pthtyp_t *next;
    struct pthtyp_t *ofb; // fullbright pixels
    hicreplctyp     *hicr;

    FHardwareTexture * glpic;
    vec2f_t         scale;
    vec2_t          siz;
    int16_t         picnum;

    uint16_t        flags; // see pthtyp_flags
    polytintflags_t effects;
    char            palnum;
    char            shade;
    char            skyface;
} pthtyp;

extern void gloadtile_art(int32_t,int32_t,int32_t,int32_t,int32_t,pthtyp *,int32_t);
extern int32_t gloadtile_hi(int32_t,int32_t,int32_t,hicreplctyp *,int32_t,pthtyp *,int32_t, polytintflags_t);

extern int32_t globalnoeffect;
extern int32_t drawingskybox;
extern int32_t hicprecaching;
extern float fcosglobalang, fsinglobalang;
extern float fxdim, fydim, fydimen, fviewingrange;

extern char ptempbuf[MAXWALLSB<<1];

extern hitdata_t polymost_hitdata;

#include "texcache.h"

extern void polymost_setupglowtexture(int32_t texunits, FHardwareTexture *tex);
extern void polymost_setupdetailtexture(int32_t texunits, FHardwareTexture* tex);

#endif
