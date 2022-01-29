// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#pragma once

#ifndef build_h_
#define build_h_

#define TRANSPARENT_INDEX 0

static_assert('\xff' == 255, "Char must be unsigned!");

#include "printf.h"
#include "palette.h"
#include "binaryangle.h"

    //Make all variables in BUILD.H defined in the ENGINE,
    //and externed in GAME
#ifdef engine_c_
#  define EXTERN
#else
#  define EXTERN extern
#endif

EXTERN int sintable[2048];

#include "buildtiles.h"
#include "c_cvars.h"
#include "cmdlib.h"
#include "mathutil.h"

typedef int64_t coord_t;

enum
{
    MAXVOXMIPS = 5,

    MAXWALLSB = 6144,

    MAXVOXELS = 1024,
    // Maximum number of component tiles in a multi-psky:
    MAXSPRITESONSCREEN = 4096,
    MAXUNIQHUDID = 256, //Extra slots so HUD models can store animation state without messing game sprites

    TSPR_TEMP = 99,

    CLIPMASK0 = (1 << 16) + 1,
    CLIPMASK1 = (256 << 16) + 64
};



#define POINT2(i) (wall[wall[i].point2])


// rotatesprite 'orientation' (actually much more) bits
enum {
    RS_TRANS1 = 1,
    RS_AUTO = 2,
    RS_YFLIP = 4,
    RS_NOCLIP = 8,
    RS_TOPLEFT = 16,
    RS_TRANS2 = 32,
    RS_NOMASK = 64,
    RS_PERM = 128,

    RS_ALIGN_L = 256,
    RS_ALIGN_R = 512,
    RS_ALIGN_MASK = 768,
    RS_STRETCH = 1024,

    RS_MODELSUBST= 4096,
    // ROTATESPRITE_MAX-1 is the mask of all externally available orientation bits
    ROTATESPRITE_MAX = 8192,
	RS_XFLIPHUD = RS_YFLIP,
	RS_YFLIPHUD = 16384, // this is for hud_drawsprite which uses RS_YFLIP for x-flipping but needs both flags

    RS_CENTER = (1<<29),    // proper center align.
    RS_CENTERORIGIN = (1<<30),
};

#include "maptypes.h"
#include "clip.h"

int32_t getwalldist(vec2_t const in, int const wallnum);
int32_t getwalldist(vec2_t const in, int const wallnum, vec2_t * const out);

EXTERN int32_t guniqhudid;

struct usermaphack_t 
{
    FString mhkfile;
    FString title;
    uint8_t md4[16]{};
};

EXTERN int leveltimer;

EXTERN int32_t xdim, ydim;
EXTERN int32_t yxaspect, viewingrange;

EXTERN int32_t Numsprites;
EXTERN int32_t display_mirror;

EXTERN int32_t randomseed;

EXTERN uint8_t paletteloaded;

enum {
    PALETTE_MAIN = 1<<0,
    PALETTE_SHADE = 1<<1,
    PALETTE_TRANSLUC = 1<<2,
};

inline int32_t g_visibility = 512, g_relvisibility = 0;

EXTERN vec2_t windowxy1, windowxy2;

// last sprite in the freelist, that is the spritenum for which 
//   .statnum==MAXSTATUS && nextspritestat[spritenum]==-1
// (or -1 if freelist is empty):
EXTERN int16_t tailspritefree;

extern uint32_t drawlinepat;

extern uint8_t globalr, globalg, globalb;

enum {
    ENGINECOMPATIBILITY_NONE = 0,
    ENGINECOMPATIBILITY_19950829, // Powerslave/Exhumed
    ENGINECOMPATIBILITY_19960925, // Blood v1.21
    ENGINECOMPATIBILITY_19961112, // Duke 3d v1.5, Redneck Rampage
};

EXTERN int32_t enginecompatibility_mode;


void engineInit(void);

void   videoSetCorrectedAspect();
void   videoSetViewableArea(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void   renderSetAspect(int32_t daxrange, int32_t daaspect);

FCanvasTexture *renderSetTarget(int16_t tilenume);
void   renderRestoreTarget();

void setVideoMode();

class F2DDrawer;


void getzrange(const vec3_t& pos, sectortype* sect, int32_t* ceilz, CollisionBase& ceilhit, int32_t* florz,
    CollisionBase& florhit, int32_t walldist, uint32_t cliptype);

extern vec2_t hitscangoal;

struct HitInfoBase;
int hitscan(const vec3_t& start, const sectortype* startsect, const vec3_t& direction, HitInfoBase& hitinfo, unsigned cliptype);
void neartag(const vec3_t& pos, sectortype* sect, int angle, HitInfoBase& result, int neartagrange, int tagsearch);

int cansee(int x1, int y1, int z1, sectortype* sect1, int x2, int y2, int z2, sectortype* sect2);

int32_t try_facespr_intersect(DCoreActor* spr, vec3_t const in,
                                     int32_t vx, int32_t vy, int32_t vz,
                                     vec3_t * const intp, int32_t strictly_smaller_than_p);

#define MAXUPDATESECTORDIST 1536
#define INITIALUPDATESECTORDIST 512 // was 256 which is too low - Exhumed LEV1 has problems with it 
void updatesector(int const x, int const y, int * const sectnum) ATTRIBUTE((nonnull(3)));
inline void updatesector(int const x, int const y, sectortype** const sectp)
{
	int sectno = *sectp? sector.IndexOf(*sectp) : -1;
	updatesector(x, y, &sectno);
	*sectp = sectno == -1? nullptr : &sector[sectno];
}
void updatesectorz(int32_t const x, int32_t const y, int32_t const z, int * const sectnum) ATTRIBUTE((nonnull(4)));

inline void updatesectorz(int32_t const x, int32_t const y, int32_t const z, sectortype** const sectp)
{
    int sectno = *sectp ? sector.IndexOf(*sectp) : -1;
    updatesectorz(x, y, z, &sectno);
    *sectp = sectno == -1 ? nullptr : &sector[sectno];
}



void updatesectorneighbor(int32_t const x, int32_t const y, int * const sectnum, int32_t maxDistance = MAXUPDATESECTORDIST) ATTRIBUTE((nonnull(3)));


int32_t getsectordist(vec2_t const in, int const sectnum, vec2_t * const out = nullptr);
extern const int16_t *chsecptr_onextwall;

inline int32_t krand(void)
{
    randomseed = (randomseed * 27584621) + 1;
    return ((uint32_t) randomseed)>>16;
}

inline int32_t ksqrt(uint64_t num)
{
    return int(sqrt(double(num)));
}

int32_t   getangle(int32_t xvect, int32_t yvect);
inline int32_t   getangle(const vec2_t& vec)
{
    return getangle(vec.X, vec.Y);
}

inline constexpr uint32_t uhypsq(int32_t const dx, int32_t const dy)
{
    return (uint32_t)dx*dx + (uint32_t)dy*dy;
}

void rotatepoint(vec2_t const pivot, vec2_t p, int16_t const daang, vec2_t * const p2) ATTRIBUTE((nonnull(4)));

sectortype* nextsectorneighborzptr(sectortype* sectp, int refz, int topbottom, int direction);
inline sectortype* safenextsectorneighborzptr(sectortype* sectp, int refz, int topbottom, int direction)
{
    auto sect = nextsectorneighborzptr(sectp, refz, topbottom, direction);
    return sect == nullptr ? sectp : sect;
}

int getceilzofslopeptr(const sectortype* sec, int dax, int day) ATTRIBUTE((nonnull(1)));
int getflorzofslopeptr(const sectortype* sec, int dax, int day) ATTRIBUTE((nonnull(1)));
void getzsofslopeptr(const sectortype* sec, int dax, int day, int *ceilz, int *florz) ATTRIBUTE((nonnull(1,4,5)));

inline void getcorrectzsofslope(int sectnum, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    vec2_t closest = { dax, day };
    getsectordist(closest, sectnum, &closest);
    getzsofslopeptr(&sector[sectnum], closest.X, closest.Y, ceilz, florz);
}

void   alignceilslope(sectortype* dasect, int32_t x, int32_t y, int32_t z);
void   alignflorslope(sectortype* dasect, int32_t x, int32_t y, int32_t z);

int32_t lintersect(int32_t originX, int32_t originY, int32_t originZ,
                   int32_t destX, int32_t destY, int32_t destZ,
                   int32_t lineStartX, int32_t lineStartY, int32_t lineEndX, int32_t lineEndY,
                   int32_t *intersectionX, int32_t *intersectionY, int32_t *intersectionZ);

int32_t spriteheightofsptr(DCoreActor* spr, int32_t *height, int32_t alsotileyofs);

int videoCaptureScreen();

void Polymost_Startup();

EXTERN_CVAR(Bool, hw_animsmoothing)
EXTERN_CVAR(Bool, hw_hightile)
EXTERN_CVAR(Bool, hw_models)
EXTERN_CVAR(Float, hw_shadescale)
EXTERN_CVAR(Float, gl_texture_filter_anisotropic)
EXTERN_CVAR(Int, gl_texture_filter)
extern bool hw_int_useindexedcolortextures;
EXTERN_CVAR(Bool, hw_useindexedcolortextures)
EXTERN_CVAR(Bool, hw_parallaxskypanning)
EXTERN_CVAR(Bool, r_voxels)

extern int32_t mdtims, omdtims;

extern int32_t r_rortexture;
extern int32_t r_rortexturerange;
extern int32_t r_rorphase;

// flags bitset: 1 = don't compress
int32_t Ptile2tile(int32_t tile, int32_t palette) ATTRIBUTE((pure));
int32_t md_loadmodel(const char *fn);
int32_t md_setmisc(int32_t modelid, float scale, int32_t shadeoff, float zadd, float yoffset, int32_t flags);
// int32_t md_tilehasmodel(int32_t tilenume, int32_t pal);

EXTERN int32_t nextvoxid;
EXTERN FixedBitArray<MAXVOXELS>voxreserve;

#ifdef USE_OPENGL
// TODO: dynamically allocate this

typedef struct { FVector3 add; int16_t angadd, flags, fov; } hudtyp;

typedef struct
{
    // maps build tiles to particular animation frames of a model
    int16_t     modelid;
    int16_t     framenum;   // calculate the number from the name when declaring
    int16_t     nexttile;
    uint16_t    smoothduration;
    hudtyp      hudmem[2];
    int8_t      skinnum;
    char        pal;
} tile2model_t;

# define EXTRATILES (MAXTILES/8)

EXTERN int32_t mdinited;
EXTERN tile2model_t tile2model[MAXTILES+EXTRATILES];

inline int32_t md_tilehasmodel(int32_t const tilenume, int32_t const pal)
{
    return mdinited ? tile2model[Ptile2tile(tilenume,pal)].modelid : -1;
}
#endif  // defined USE_OPENGL

int tilehasmodelorvoxel(int const tilenume, int pal);

int32_t md_defineframe(int32_t modelid, const char *framename, int32_t tilenume,
                       int32_t skinnum, float smoothduration, int32_t pal);
int32_t md_defineanimation(int32_t modelid, const char *framestart, const char *frameend,
                           int32_t fps, int32_t flags);
int32_t md_defineskin(int32_t modelid, const char *skinfn, int32_t palnum, int32_t skinnum,
                      int32_t surfnum, float param, float specpower, float specfactor, int32_t flags);
int32_t md_definehud (int32_t modelid, int32_t tilex, FVector3 add,
                      int32_t angadd, int32_t flags, int32_t fov);
int32_t md_undefinetile(int32_t tile);
int32_t md_undefinemodel(int32_t modelid);

#ifdef USE_OPENGL
# include "polymost.h"
#endif

extern int skiptile;

static vec2_t const zerovec = { 0, 0 };

#define SET_AND_RETURN(Lval, Rval) \
    do                             \
    {                              \
        (Lval) = (Rval);           \
        return;                    \
    } while (0)

static inline int64_t compat_maybe_truncate_to_int32(int64_t val)
{
    return enginecompatibility_mode != ENGINECOMPATIBILITY_NONE ? (int32_t)val : val;
}

extern int32_t rintersect(int32_t x1, int32_t y1, int32_t z1,
    int32_t vx_, int32_t vy_, int32_t vz,
    int32_t x3, int32_t y3, int32_t x4, int32_t y4,
    int32_t *intx, int32_t *inty, int32_t *intz);



void updateModelInterpolation();

inline void tileUpdatePicnum(int* const tileptr, int const obj, int stat)
{
    auto& tile = *tileptr;

    if (picanm[tile].sf & PICANM_ANIMTYPE_MASK)
        tile += animateoffs(tile, obj);

    if (((obj & 16384) == 16384) && (stat & CSTAT_WALL_ROTATE_90) && RotTile(tile).newtile != -1)
        tile = RotTile(tile).newtile;
}

#endif // build_h_
