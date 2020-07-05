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

#if !defined __cplusplus || (__cplusplus < 201103L && !defined _MSC_VER)
# error C++11 or greater is required.
#endif

#if defined _MSC_VER && _MSC_VER < 1800
# error Visual Studio 2013 is the minimum supported version.
#endif

#include "compat.h"
#include "palette.h"
#include "pragmas.h"


#include "buildtiles.h"
#include "c_cvars.h"
#include "cmdlib.h"

typedef int64_t coord_t;

enum rendmode_t {
    REND_CLASSIC,
    REND_POLYMOST = 3,
    REND_POLYMER
};

#define PI 3.14159265358979323846
#define fPI 3.14159265358979323846f

#define BANG2RAD (fPI * (1.f/1024.f))

#define MAXSECTORSV8 4096
#define MAXWALLSV8 16384
#define MAXSPRITESV8 16384

#define MAXSECTORSV7 1024
#define MAXWALLSV7 8192
#define MAXSPRITESV7 4096

#define MAXVOXMIPS 5


# define MAXSECTORS MAXSECTORSV8
# define MAXWALLS MAXWALLSV8
# define MAXSPRITES MAXSPRITESV8

# define MAXXDIM 7680
# define MAXYDIM 3200
# define MINXDIM 640
# define MINYDIM 480

// additional space beyond wall, in walltypes:
# define M32_FIXME_WALLS 512
# define M32_FIXME_SECTORS 2


#define MAXWALLSB ((MAXWALLS>>2)+(MAXWALLS>>3))

#define MAXVOXELS 1024
#define MAXSTATUS 1024
#define MAXPLAYERS 16
// Maximum number of component tiles in a multi-psky:
#define MAXPSKYTILES 16
#define MAXSPRITESONSCREEN 2560
#define MAXUNIQHUDID 256 //Extra slots so HUD models can store animation state without messing game sprites

#define TSPR_TEMP 99

#define PR_LIGHT_PRIO_MAX       0
#define PR_LIGHT_PRIO_MAX_GAME  1
#define PR_LIGHT_PRIO_HIGH      2
#define PR_LIGHT_PRIO_HIGH_GAME 3
#define PR_LIGHT_PRIO_LOW       4
#define PR_LIGHT_PRIO_LOW_GAME  5

// Convenient sprite iterators, must not be used if any sprites inside the loop
// are potentially deleted or their sector changed...
#define SPRITES_OF(Statnum, Iter)  Iter=headspritestat[Statnum]; Iter>=0; Iter=nextspritestat[Iter]
#define SPRITES_OF_SECT(Sectnum, Iter)  Iter=headspritesect[Sectnum]; Iter>=0; Iter=nextspritesect[Iter]
// ... in which case this iterator may be used:
#define SPRITES_OF_SECT_SAFE(Sectnum, Iter, Next)  Iter=headspritesect[Sectnum]; \
    Iter>=0 && (Next=nextspritesect[Iter], 1); Iter=Next
#define SPRITES_OF_STAT_SAFE(Statnum, Iter, Next)  Iter=headspritestat[Statnum]; \
    Iter>=0 && (Next=nextspritestat[Iter], 1); Iter=Next


////////// True Room over Room (YAX == rot -17 of "PRO") //////////
#define YAX_ENABLE
//#define YAX_DEBUG
//#define ENGINE_SCREENSHOT_DEBUG

#ifdef YAX_ENABLE
# if !defined NEW_MAP_FORMAT
#  define YAX_ENABLE__COMPAT
# endif
#endif

////////// yax defs //////////
#define SECTORFLD(Sect,Fld, Cf) (*((Cf) ? (&sector[Sect].floor##Fld) : (&sector[Sect].ceiling##Fld)))

#define YAX_CEILING 0  // don't change!
#define YAX_FLOOR 1  // don't change!


#  define YAX_MAXBUNCHES 256
#  define YAX_BIT 1024
   // "has next wall when constrained"-bit (1<<10: ceiling, 1<<11: floor)
#  define YAX_NEXTWALLBIT(Cf) (1<<(10+Cf))
#  define YAX_NEXTWALLBITS (YAX_NEXTWALLBIT(0)|YAX_NEXTWALLBIT(1))

#ifdef YAX_ENABLE

   // More user tag hijacking: lotag/extra. :/
#  define YAX_PTRNEXTWALL(Ptr, Wall, Cf) (*(int16_t *)(&Ptr[Wall].lotag + (playing_blood ? 1 : 2)*Cf))
#  define YAX_NEXTWALLDEFAULT(Cf) (playing_blood ? 0 : ((Cf)==YAX_CEILING) ? 0 : -1)
   extern int16_t yax_bunchnum[MAXSECTORS][2];
   extern int16_t yax_nextwall[MAXWALLS][2];


# define YAX_NEXTWALL(Wall, Cf) YAX_PTRNEXTWALL(wall, Wall, Cf)

# define YAX_ITER_WALLS(Wal, Itervar, Cfvar) Cfvar=0, Itervar=(Wal); Itervar!=-1; \
    Itervar=yax_getnextwall(Itervar, Cfvar), \
        (void)(Itervar==-1 && Cfvar==0 && (Cfvar=1) && (Itervar=yax_getnextwall((Wal), Cfvar)))

# define SECTORS_OF_BUNCH(Bunchnum, Cf, Itervar) Itervar = headsectbunch[Cf][Bunchnum]; \
    Itervar != -1; Itervar = nextsectbunch[Cf][Itervar]

extern int32_t r_tror_nomaskpass;


int16_t yax_getbunch(int16_t i, int16_t cf);
static FORCE_INLINE void yax_getbunches(int16_t i, int16_t *cb, int16_t *fb)
{
    *cb = yax_getbunch(i, YAX_CEILING);
    *fb = yax_getbunch(i, YAX_FLOOR);
}
int16_t yax_getnextwall(int16_t wal, int16_t cf);
void yax_setnextwall(int16_t wal, int16_t cf, int16_t thenextwall);


void yax_setbunch(int16_t i, int16_t cf, int16_t bunchnum);
void yax_setbunches(int16_t i, int16_t cb, int16_t fb);
int16_t yax_vnextsec(int16_t line, int16_t cf);
void yax_update(int32_t resetstat);
int32_t yax_getneighborsect(int32_t x, int32_t y, int32_t sectnum, int32_t cf);

static FORCE_INLINE CONSTEXPR int32_t yax_waltosecmask(int32_t const walclipmask)
{
    // blocking: walstat&1 --> secstat&512
    // hitscan: walstat&64 --> secstat&2048
    return ((walclipmask&1)<<9) | ((walclipmask&64)<<5);
}
void yax_preparedrawrooms(void);
void yax_drawrooms(void (*SpriteAnimFunc)(int32_t,int32_t,int32_t,int32_t,int32_t),
                   int16_t sectnum, int32_t didmirror, int32_t smoothr);
#else
# define yax_preparedrawrooms()
# define yax_drawrooms(SpriteAnimFunc, sectnum, didmirror, smoothr)
#endif

#define CLIPMASK0 (((1)<<16)+1)
#define CLIPMASK1 (((256)<<16)+64)

#define NEXTWALL(i) (wall[wall[i].nextwall])
#define POINT2(i) (wall[wall[i].point2])

// max x/y val (= max editorgridextent in Mapster32)
#define BXY_MAX 524288

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

    ROTATESPRITE_FULL16 = 2048,
    RS_MODELSUBST= 4096,
    // ROTATESPRITE_MAX-1 is the mask of all externally available orientation bits
    ROTATESPRITE_MAX = 8192,

    RS_CENTER = (1<<29),    // proper center align.
    RS_CENTERORIGIN = (1<<30),
};

    //Make all variables in BUILD.H defined in the ENGINE,
    //and externed in GAME
#ifdef engine_c_
#  define EXTERN
#else
#  define EXTERN extern
#endif

#if defined __cplusplus && (defined USE_OPENGL || defined POLYMER)
# define USE_STRUCT_TRACKERS
#endif

#ifdef USE_STRUCT_TRACKERS


static FORCE_INLINE void sector_tracker_hook__(intptr_t address);
static FORCE_INLINE void wall_tracker_hook__(intptr_t address);
static FORCE_INLINE void sprite_tracker_hook__(intptr_t address);


#define TRACKER_NAME__ SectorTracker
#define TRACKER_HOOK_ sector_tracker_hook__
#include "tracker.hpp"
#undef TRACKER_NAME__
#undef TRACKER_HOOK_

#define TRACKER_NAME__ WallTracker
#define TRACKER_HOOK_ wall_tracker_hook__
#include "tracker.hpp"
#undef TRACKER_NAME__
#undef TRACKER_HOOK_

#define TRACKER_NAME__ SpriteTracker
#define TRACKER_HOOK_ sprite_tracker_hook__
#include "tracker.hpp"
#undef TRACKER_NAME__
#undef TRACKER_HOOK_

#define Tracker(Container, Type) Container##Tracker<Type>
#define TrackerCast(x) x.cast()

#else

#define Tracker(Container, Type) Type
#define TrackerCast(x) x

#endif // __cplusplus

// Links to various ABIs specifying (or documenting non-normatively) the
// alignment requirements of aggregates:
//
//  System V AMD64: http://www.x86-64.org/documentation/abi-0.99.pdf
//   (x86-64.org down as of 2013-02-02?)
//  "An array uses the same alignment as its elements, except that a local or global
//   array variable of length at least 16 bytes or a C99 variable-length array variable
//   always has alignment of at least 16 bytes."
//   (Not reproducible with GCC or LuaJIT on Ubuntu)
//
//  Win64: http://msdn.microsoft.com/en-us/library/9dbwhz68.aspx
//
//  x86: http://en.wikipedia.org/wiki/Data_structure_alignment#Typical_alignment_of_C_structs_on_x86

enum {
    SPR_XFLIP = 4,
    SPR_YFLIP = 8,

    SPR_WALL = 16,
    SPR_FLOOR = 32,
    SPR_ALIGN_MASK = 32+16,
};

#define UNTRACKED_STRUCTS__
#include "buildtypes.h"
#undef UNTRACKED_STRUCTS__
#undef buildtypes_h__
#include "buildtypes.h"

#if !defined NEW_MAP_FORMAT
using sectortype  = sectortypev7;
using usectortype = usectortypev7;

using walltype  = walltypev7;
using uwalltype = uwalltypev7;
#else
using sectortype  = sectortypevx;
using usectortype = usectortypevx;

using walltype  = walltypevx;
using uwalltype = uwalltypevx;
#endif

using spritetype  = spritetypev7;
using uspritetype = uspritetypev7;

using uspriteptr_t = uspritetype const *;
using uwallptr_t   = uwalltype const *;
using usectorptr_t = usectortype const *;
using tspriteptr_t = tspritetype *;

// this is probably never going to be necessary
EDUKE32_STATIC_ASSERT(sizeof(sectortype) == sizeof(usectortype));
EDUKE32_STATIC_ASSERT(sizeof(walltype) == sizeof(uwalltype));
EDUKE32_STATIC_ASSERT(sizeof(spritetype) == sizeof(uspritetype));


#include "clip.h"

int32_t getwalldist(vec2_t const in, int const wallnum);
int32_t getwalldist(vec2_t const in, int const wallnum, vec2_t * const out);

typedef struct {
    uint32_t mdanimtims;
    int16_t mdanimcur;
    int16_t angoff, pitch, roll;
    vec3_t pivot_offset, position_offset;
    uint8_t flags;
    uint8_t xpanning, ypanning; // EDuke script hacks.
    uint8_t filler;
    uint32_t filler2;
    float alpha;
    // NOTE: keep 'tspr' on an 8-byte boundary:
    tspriteptr_t tspr;
#if !defined UINTPTR_MAX
# error Need UINTPTR_MAX define to select between 32- and 64-bit structs
#endif
#if UINTPTR_MAX == 0xffffffff
    /* On a 32-bit build, pad the struct so it has the same size everywhere. */
    intptr_t dummy_;
#endif
} spriteext_t;

typedef struct {
    float smoothduration;
    int16_t mdcurframe, mdoldframe;
    int16_t mdsmooth;
    uint8_t filler[2];
} spritesmooth_t;

#ifndef NEW_MAP_FORMAT
typedef struct {
    uint8_t blend;
} wallext_t;
#endif

#define SPREXT_NOTMD 1
#define SPREXT_NOMDANIM 2
#define SPREXT_AWAY1 4
#define SPREXT_AWAY2 8
#define SPREXT_TSPRACCESS 16
#define SPREXT_TEMPINVISIBLE 32

#define NEG_ALPHA_TO_BLEND(alpha, blend, orientation) do { \
    if ((alpha) < 0) { (blend) = -(alpha); (alpha) = 0; (orientation) |= RS_TRANS1; } \
} while (0)

// using the clipdist field
enum
{
    TSPR_FLAGS_MDHACK = 1u<<0u,
    TSPR_FLAGS_DRAW_LAST = 1u<<1u,
    TSPR_FLAGS_NO_SHADOW = 1u<<2u,
    TSPR_FLAGS_INVISIBLE_WITH_SHADOW = 1u<<3u,
};

EXTERN int32_t guniqhudid;
EXTERN int32_t spritesortcnt;
extern int32_t g_loadedMapVersion;

typedef struct {
    char *mhkfile;
    char *title;
    uint8_t md4[16];
} usermaphack_t;

extern usermaphack_t g_loadedMapHack;
extern int compare_usermaphacks(const void *, const void *);
extern usermaphack_t *usermaphacks;
extern int32_t num_usermaphacks;

#if !defined DEBUG_MAIN_ARRAYS
EXTERN spriteext_t *spriteext;
EXTERN spritesmooth_t *spritesmooth;
# ifndef NEW_MAP_FORMAT
EXTERN wallext_t *wallext;
# endif

EXTERN sectortype *sector;
EXTERN walltype *wall;
EXTERN spritetype *sprite;
EXTERN tspriteptr_t tsprite;
#else
EXTERN spriteext_t spriteext[MAXSPRITES+MAXUNIQHUDID];
EXTERN spritesmooth_t spritesmooth[MAXSPRITES+MAXUNIQHUDID];
# ifndef NEW_MAP_FORMAT
EXTERN wallext_t wallext[MAXWALLS];
# endif

EXTERN sectortype sector[MAXSECTORS + M32_FIXME_SECTORS];
EXTERN walltype wall[MAXWALLS + M32_FIXME_WALLS];
EXTERN spritetype sprite[MAXSPRITES];
EXTERN uspritetype tsprite[MAXSPRITESONSCREEN];
#endif

#ifdef USE_STRUCT_TRACKERS
EXTERN uint32_t sectorchanged[MAXSECTORS + M32_FIXME_SECTORS];
EXTERN uint32_t wallchanged[MAXWALLS + M32_FIXME_WALLS];
EXTERN uint32_t spritechanged[MAXSPRITES];
#endif



#ifdef USE_STRUCT_TRACKERS
static FORCE_INLINE void sector_tracker_hook__(intptr_t const address)
{
    intptr_t const sectnum = (address - (intptr_t)sector) / sizeof(sectortype);

#if DEBUGGINGAIDS>=2
    Bassert((unsigned)sectnum < ((MAXSECTORS + M32_FIXME_SECTORS)));
#endif

    ++sectorchanged[sectnum];
}

static FORCE_INLINE void wall_tracker_hook__(intptr_t const address)
{
    intptr_t const wallnum = (address - (intptr_t)wall) / sizeof(walltype);

#if DEBUGGINGAIDS>=2
    Bassert((unsigned)wallnum < ((MAXWALLS + M32_FIXME_WALLS)));
#endif

    ++wallchanged[wallnum];
}

static FORCE_INLINE void sprite_tracker_hook__(intptr_t const address)
{
    intptr_t const spritenum = (address - (intptr_t)sprite) / sizeof(spritetype);

#if DEBUGGINGAIDS>=2
    Bassert((unsigned)spritenum < MAXSPRITES);
#endif

    ++spritechanged[spritenum];
}
#endif

static inline tspriteptr_t renderMakeTSpriteFromSprite(tspriteptr_t const tspr, uint16_t const spritenum)
{
    auto const spr = (uspriteptr_t)&sprite[spritenum];

    tspr->pos = spr->pos;
    tspr->cstat = spr->cstat;
    tspr->picnum = spr->picnum;
    tspr->shade = spr->shade;
    tspr->pal = spr->pal;
    tspr->blend = spr->blend;
    tspr->xrepeat = spr->xrepeat;
    tspr->yrepeat = spr->yrepeat;
    tspr->xoffset = spr->xoffset;
    tspr->yoffset = spr->yoffset;
    tspr->sectnum = spr->sectnum;
    tspr->statnum = spr->statnum;
    tspr->ang = spr->ang;
    tspr->vel = spr->vel;
    tspr->lotag = spr->lotag;
    tspr->hitag = spr->hitag;
    tspr->extra = spr->extra;

    tspr->clipdist = 0;
    tspr->owner = spritenum;

    return tspr;
}

static inline tspriteptr_t renderAddTSpriteFromSprite(uint16_t const spritenum)
{
    return renderMakeTSpriteFromSprite(&tsprite[spritesortcnt++], spritenum);
}


EXTERN int16_t maskwall[MAXWALLSB], maskwallcnt;
EXTERN int16_t thewall[MAXWALLSB];
EXTERN tspriteptr_t tspriteptr[MAXSPRITESONSCREEN + 1];

EXTERN int32_t wx1, wy1, wx2, wy2;
EXTERN int32_t xdim, ydim, numpages, upscalefactor;
EXTERN int32_t yxaspect, viewingrange;

EXTERN int32_t rotatesprite_y_offset;
EXTERN int32_t rotatesprite_yxaspect;

#ifndef GEKKO
#define MAXVALIDMODES 256
#else
#define MAXVALIDMODES 16
#endif
EXTERN int32_t validmodecnt;
struct validmode_t {
    int32_t xdim,ydim;
    char bpp;
    char fs;    // bit 0 = fullscreen flag
    char filler[2];
    int32_t extra; // internal use
};
EXTERN struct validmode_t validmode[MAXVALIDMODES];

EXTERN int32_t numyaxbunches;
#ifdef YAX_ENABLE
// Singly-linked list of sectnums grouped by bunches and ceiling (0)/floor (1)
// Usage e.g.:
//   int16_t bunchnum = yax_getbunch(somesector, YAX_CEILING);
// Iteration over all sectors whose floor bunchnum equals 'bunchnum' (i.e. "all
// floors of the other side"):
//   for (i=headsectbunch[1][bunchnum]; i!=-1; i=nextsectbunch[1][i])
//       <do stuff with sector i...>

EXTERN int16_t headsectbunch[2][YAX_MAXBUNCHES], nextsectbunch[2][MAXSECTORS];
#endif

EXTERN int32_t Numsprites;
EXTERN int16_t numsectors, numwalls;
EXTERN int32_t display_mirror;
// totalclocklock: the totalclock value that is backed up once on each
// drawrooms() and is used for animateoffs().
EXTERN ClockTicks totalclock, totalclocklock;
static inline int32_t BGetTime(void) { return (int32_t) totalclock; }

EXTERN int32_t numframes, randomseed;
EXTERN int16_t sintable[2048];

EXTERN int16_t numshades;
EXTERN uint8_t paletteloaded;


EXTERN int32_t maxspritesonscreen;

enum {
    PALETTE_MAIN = 1<<0,
    PALETTE_SHADE = 1<<1,
    PALETTE_TRANSLUC = 1<<2,
};

EXTERN char showinvisibility;
EXTERN int32_t g_visibility, parallaxvisibility;

// blendtable[1] to blendtable[numalphatabs] are considered to be
// alpha-blending tables:
EXTERN uint8_t numalphatabs;

EXTERN vec2_t windowxy1, windowxy2;

// The maximum tile offset ever used in any tiled parallaxed multi-sky.
#define PSKYOFF_MAX 16
#define DEFAULTPSKY -1

typedef struct {
    // The proportion at which looking up/down affects the apparent 'horiz' of
    // a parallaxed sky, scaled by 65536 (so, a value of 65536 makes it align
    // with the drawn surrounding scene):
    int32_t horizfrac;

    // The texel index offset in the y direction of a parallaxed sky:
    // XXX: currently always 0.
    int32_t yoffs;

    int8_t lognumtiles;  // 1<<lognumtiles: number of tiles in multi-sky
    int8_t tileofs[MAXPSKYTILES];  // for 0 <= j < (1<<lognumtiles): tile offset relative to basetile

    int32_t yscale;
    int combinedtile;
} psky_t;

// Index of map-global (legacy) multi-sky:
EXTERN int32_t g_pskyidx;
// New multi-psky
EXTERN int32_t pskynummultis;
EXTERN psky_t * multipsky;
// Mapping of multi-sky index to base sky tile number:
EXTERN int32_t * multipskytile;

static FORCE_INLINE int32_t getpskyidx(int32_t picnum)
{
    int32_t j;

    for (j=pskynummultis-1; j>0; j--)  // NOTE: j==0 on non-early loop end
        if (picnum == multipskytile[j])
            break;  // Have a match.

    return j;
}

EXTERN psky_t * tileSetupSky(int32_t tilenum);

EXTERN char parallaxtype;
EXTERN int32_t parallaxyoffs_override, parallaxyscale_override;
extern int16_t pskybits_override;

// last sprite in the freelist, that is the spritenum for which
//   .statnum==MAXSTATUS && nextspritestat[spritenum]==-1
// (or -1 if freelist is empty):
EXTERN int16_t tailspritefree;

EXTERN int16_t headspritesect[MAXSECTORS+1], headspritestat[MAXSTATUS+1];
EXTERN int16_t prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
EXTERN int16_t nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];

static CONSTEXPR const int32_t pow2long[32] =
{
    1, 2, 4, 8,
    16, 32, 64, 128,
    256, 512, 1024, 2048,
    4096, 8192, 16384, 32768,
    65536, 131072, 262144, 524288,
    1048576, 2097152, 4194304, 8388608,
    16777216, 33554432, 67108864, 134217728,
    268435456, 536870912, 1073741824, 2147483647
};

    //These variables are for auto-mapping with the draw2dscreen function.
    //When you load a new board, these bits are all set to 0 - since
    //you haven't mapped out anything yet.  Note that these arrays are
    //bit-mapped.
    //If you want draw2dscreen() to show sprite #54 then you say:
    //   spritenum = 54;
    //   show2dsprite[spritenum>>3] |= (1<<(spritenum&7));
    //And if you want draw2dscreen() to not show sprite #54 then you say:
    //   spritenum = 54;
    //   show2dsprite[spritenum>>3] &= ~(1<<(spritenum&7));

EXTERN int automapping;
EXTERN FixedBitArray<MAXSECTORS> show2dsector;
EXTERN bool gFullMap;

EXTERN char show2dwall[(MAXWALLS+7)>>3];
EXTERN char show2dsprite[(MAXSPRITES+7)>>3];


EXTERN uint8_t gotpic[(MAXTILES+7)>>3];
EXTERN char gotsector[(MAXSECTORS+7)>>3];


extern uint32_t drawlinepat;

extern void faketimerhandler(void);

extern char apptitle[256];

extern int32_t novoxmips;

#ifdef DEBUGGINGAIDS
extern float debug1, debug2;
#endif

extern int16_t tiletovox[MAXTILES];
extern int32_t voxscale[MAXVOXELS];
extern char g_haveVoxels;

#ifdef USE_OPENGL
extern int32_t rendmode;
#endif
extern uint8_t globalr, globalg, globalb;
EXTERN uint16_t h_xsize[MAXTILES], h_ysize[MAXTILES];
EXTERN int8_t h_xoffs[MAXTILES], h_yoffs[MAXTILES];

enum {
    GLOBAL_NO_GL_TILESHADES = 1<<0,
    GLOBAL_NO_GL_FULLBRIGHT = 1<<1,
    GLOBAL_NO_GL_FOGSHADE = 1<<2,
};

extern int32_t globalflags;

extern const char *engineerrstr;

EXTERN int32_t editorzrange[2];

static FORCE_INLINE int32_t videoGetRenderMode(void)
{
#ifndef USE_OPENGL
    return REND_CLASSIC;
#else
    return rendmode;
#endif
}

enum {
    ENGINECOMPATIBILITY_NONE = 0,
    ENGINECOMPATIBILITY_19950829, // Powerslave/Exhumed
    ENGINECOMPATIBILITY_19960925, // Blood v1.21
    ENGINECOMPATIBILITY_19961112, // Duke 3d v1.5, Redneck Rampage
};

EXTERN int32_t enginecompatibility_mode;

/*************************************************************************
POSITION VARIABLES:

        POSX is your x - position ranging from 0 to 65535
        POSY is your y - position ranging from 0 to 65535
            (the length of a side of the grid in EDITBORD would be 1024)
        POSZ is your z - position (height) ranging from 0 to 65535, 0 highest.
        ANG is your angle ranging from 0 to 2047.  Instead of 360 degrees, or
             2 * PI radians, I use 2048 different angles, so 90 degrees would
             be 512 in my system.

SPRITE VARIABLES:

    EXTERN short headspritesect[MAXSECTORS+1], headspritestat[MAXSTATUS+1];
    EXTERN short prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
    EXTERN short nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];

    Example: if the linked lists look like the following:
         ????????????????
               Sector lists:               Status lists:
         ????????????????J
           Sector0:  4, 5, 8             Status0:  2, 0, 8
           Sector1:  16, 2, 0, 7         Status1:  4, 5, 16, 7, 3, 9
           Sector2:  3, 9
         ????????????????
    Notice that each number listed above is shown exactly once on both the
        left and right side.  This is because any sprite that exists must
        be in some sector, and must have some kind of status that you define.


Coding example #1:
    To go through all the sprites in sector 1, the code can look like this:

        sectnum = 1;
        i = headspritesect[sectnum];
        while (i != -1)
        {
            nexti = nextspritesect[i];

            //your code goes here
            //ex: printf("Sprite %d is in sector %d\n",i,sectnum);

            i = nexti;
        }

Coding example #2:
    To go through all sprites with status = 1, the code can look like this:

        statnum = 1;        //status 1
        i = headspritestat[statnum];
        while (i != -1)
        {
            nexti = nextspritestat[i];

            //your code goes here
            //ex: printf("Sprite %d has a status of 1 (active)\n",i,statnum);

            i = nexti;
        }

             insertsprite(short sectnum, short statnum);
             deletesprite(short spritenum);
             changespritesect(short spritenum, short newsectnum);
             changespritestat(short spritenum, short newstatnum);

TILE VARIABLES:
        NUMTILES - the number of tiles found TILES.DAT.

TIMING VARIABLES:
        TOTALCLOCK - When the engine is initialized, TOTALCLOCK is set to zero.
            From then on, it is incremented 120 times a second by 1.  That
            means that the number of seconds elapsed is totalclock / 120.
        NUMFRAMES - The number of times the draw3dscreen function was called
            since the engine was initialized.  This helps to determine frame
            rate.  (Frame rate = numframes * 120 / totalclock.)

OTHER VARIABLES:

        STARTUMOST[320] is an array of the highest y-coordinates on each column
                that my engine is allowed to write to.  You need to set it only
                once.
        STARTDMOST[320] is an array of the lowest y-coordinates on each column
                that my engine is allowed to write to.  You need to set it only
                once.
        SINTABLE[2048] is a sin table with 2048 angles rather than the
            normal 360 angles for higher precision.  Also since SINTABLE is in
            all integers, the range is multiplied by 16383, so instead of the
            normal -1<sin(x)<1, the range of sintable is -16383<sintable[]<16383
            If you use this sintable, you can possibly speed up your code as
            well as save space in memory.  If you plan to use sintable, 2
            identities you may want to keep in mind are:
                sintable[ang&2047]       = sin(ang * (3.141592/1024)) * 16383
                sintable[(ang+512)&2047] = cos(ang * (3.141592/1024)) * 16383
        NUMSECTORS - the total number of existing sectors.  Modified every time
            you call the loadboard function.
***************************************************************************/

typedef struct {
    vec3_t pos;
    int16_t sprite, wall, sect;
} hitdata_t;

typedef struct artheader_t {
    int32_t tilestart, tileend, numtiles;
} artheader_t;
#define ARTv1_UNITOFFSET 24 // using sizeof does not work because picanm_t is not the in-file format.

int32_t    enginePreInit(void);	// a partial setup of the engine used for launch windows
int32_t    engineInit(void);
int32_t enginePostInit(void);
void   engineUnInit(void);
void   initspritelists(void);

int32_t   engineLoadBoard(const char *filename, char flags, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum);
int32_t   engineLoadMHK(const char *filename);
void engineClearLightsFromMHK();
#ifdef HAVE_CLIPSHAPE_FEATURE
int32_t engineLoadClipMaps(void);
#endif
int32_t   saveboard(const char *filename, const vec3_t *dapos, int16_t daang, int16_t dacursectnum);

int32_t   qloadkvx(int32_t voxindex, const char *filename);
void vox_undefine(int32_t const);
void vox_deinit();

int32_t   videoSetGameMode(char davidoption, int32_t daupscaledxdim, int32_t daupscaledydim, int32_t dabpp, int32_t daupscalefactor);
void   videoNextPage(void);
void   videoSetCorrectedAspect();
void   videoSetViewableArea(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void   renderSetAspect(int32_t daxrange, int32_t daaspect);
inline void   renderFlushPerms(void) {}

void   plotpixel(int32_t x, int32_t y, char col);
FCanvasTexture *renderSetTarget(int16_t tilenume);
void   renderRestoreTarget();
void   renderPrepareMirror(int32_t dax, int32_t day, int32_t daz, fix16_t daang, fix16_t dahoriz, int16_t dawall,
                           int32_t *tposx, int32_t *tposy, fix16_t *tang);
void   renderCompleteMirror(void);

int32_t renderDrawRoomsQ16(int32_t daposx, int32_t daposy, int32_t daposz, fix16_t daang, fix16_t dahoriz, int16_t dacursectnum);

static FORCE_INLINE int32_t drawrooms(int32_t daposx, int32_t daposy, int32_t daposz, int16_t daang, int16_t dahoriz, int16_t dacursectnum)
{
    return renderDrawRoomsQ16(daposx, daposy, daposz, fix16_from_int(daang), fix16_from_int(dahoriz), dacursectnum);
}

void   renderDrawMasks(void);
void videoInit();
void   videoClearViewableArea(int32_t dacol);
void   videoClearScreen(int32_t dacol);
void   renderDrawMapView(int32_t dax, int32_t day, int32_t zoome, int16_t ang);
void   rotatesprite_(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                     int8_t dashade, uint8_t dapalnum, int32_t dastat, uint8_t daalpha, uint8_t dablend,
                     int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, FGameTexture *pic = nullptr, int basepal = 0);
void   renderDrawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t col);
void   drawlinergb(int32_t x1, int32_t y1, int32_t x2, int32_t y2, palette_t p);
void drawlinergb(int32_t x1, int32_t y1, int32_t x2, int32_t y2, PalEntry p);

class F2DDrawer;
void twod_rotatesprite(F2DDrawer* twod, int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
    int8_t dashade, uint8_t dapalnum, int32_t dastat, uint8_t daalpha, uint8_t dablend,
    int32_t clipx1, int32_t clipy1, int32_t clipx2, int32_t clipy2, FGameTexture* pic = nullptr, int basepal = 0);

////////// specialized rotatesprite wrappers for (very) often used cases //////////
static FORCE_INLINE void rotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                                int8_t dashade, uint8_t dapalnum, int32_t dastat,
                                int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, FGameTexture* pic = nullptr, int basepal = 0)
{
    rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, 0, cx1, cy1, cx2, cy2, pic, basepal);
}
// Don't clip at all, i.e. the whole screen real estate is available:
static FORCE_INLINE void rotatesprite_fs(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                                   int8_t dashade, uint8_t dapalnum, int32_t dastat, FGameTexture* pic = nullptr, int basepal = 0)
{
    rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, 0, 0,0,xdim-1,ydim-1, pic, basepal);
}

static FORCE_INLINE void rotatesprite_fs_alpha(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                                  int8_t dashade, uint8_t dapalnum, int32_t dastat, uint8_t alpha)
{
    rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, alpha, 0, 0, 0, xdim-1, ydim-1);
}

static FORCE_INLINE void rotatesprite_win(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                                    int8_t dashade, uint8_t dapalnum, int32_t dastat)
{
    rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, 0, windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
}

void   getzrange(const vec3_t *pos, int16_t sectnum, int32_t *ceilz, int32_t *ceilhit, int32_t *florz,
                 int32_t *florhit, int32_t walldist, uint32_t cliptype) ATTRIBUTE((nonnull(1,3,4,5,6)));
inline void getzrange(int x, int y, int z, int16_t sectnum, int32_t* ceilz, int32_t* ceilhit, int32_t* florz,
    int32_t* florhit, int32_t walldist, uint32_t cliptype)
{
    vec3_t v = { x, y, z };
    getzrange(&v, sectnum, ceilz, ceilhit, florz, florhit, walldist, cliptype);
}
extern vec2_t hitscangoal;
int32_t   hitscan(const vec3_t *sv, int16_t sectnum, int32_t vx, int32_t vy, int32_t vz,
                  hitdata_t *hitinfo, uint32_t cliptype) ATTRIBUTE((nonnull(1,6)));
inline int hitscan(int x, int y, int z, int16_t sectnum, int32_t vx, int32_t vy, int32_t vz,
    short* hitsect, short* hitwall, short* hitspr, int* hitx, int* hity, int* hitz, uint32_t cliptype)
{
    vec3_t v{ x,y,z };
    hitdata_t hd{};
    int res = hitscan(&v, sectnum, vx, vy, vz, &hd, cliptype);
    *hitsect = hd.sect;
    *hitwall = hd.wall;
    *hitspr = hd.sprite;
    *hitx = hd.pos.x;
    *hity = hd.pos.y;
    *hitz = hd.pos.z   ;
    return res;
}

void   neartag(int32_t xs, int32_t ys, int32_t zs, int16_t sectnum, int16_t ange,
               int16_t *neartagsector, int16_t *neartagwall, int16_t *neartagsprite,
               int32_t *neartaghitdist, int32_t neartagrange, uint8_t tagsearch,
               int32_t (*blacklist_sprite_func)(int32_t) = nullptr) ATTRIBUTE((nonnull(6,7,8)));
int32_t   cansee(int32_t x1, int32_t y1, int32_t z1, int16_t sect1,
                 int32_t x2, int32_t y2, int32_t z2, int16_t sect2);
int32_t   inside(int32_t x, int32_t y, int16_t sectnum);
void   dragpoint(int16_t pointhighlight, int32_t dax, int32_t day, uint8_t flags = 0);
void   setfirstwall(int16_t sectnum, int16_t newfirstwall);
int32_t try_facespr_intersect(uspriteptr_t const spr, vec3_t const in,
                                     int32_t vx, int32_t vy, int32_t vz,
                                     vec3_t * const intp, int32_t strictly_smaller_than_p);

#define MAXUPDATESECTORDIST 1536
#define INITIALUPDATESECTORDIST 256
void updatesector(int32_t const x, int32_t const y, int16_t * const sectnum) ATTRIBUTE((nonnull(3)));
void updatesectorexclude(int32_t const x, int32_t const y, int16_t * const sectnum,
                         const uint8_t * const excludesectbitmap) ATTRIBUTE((nonnull(3,4)));
void updatesectorz(int32_t const x, int32_t const y, int32_t const z, int16_t * const sectnum) ATTRIBUTE((nonnull(4)));
void updatesectorneighbor(int32_t const x, int32_t const y, int16_t * const sectnum, int32_t initialMaxDistance = INITIALUPDATESECTORDIST, int32_t maxDistance = MAXUPDATESECTORDIST) ATTRIBUTE((nonnull(3)));
void updatesectorneighborz(int32_t const x, int32_t const y, int32_t const z, int16_t * const sectnum, int32_t initialMaxDistance = INITIALUPDATESECTORDIST, int32_t maxDistance = MAXUPDATESECTORDIST) ATTRIBUTE((nonnull(4)));

int findwallbetweensectors(int sect1, int sect2);
static FORCE_INLINE int sectoradjacent(int sect1, int sect2) { return findwallbetweensectors(sect1, sect2) != -1; }
int32_t getsectordist(vec2_t const in, int const sectnum, vec2_t * const out = nullptr);
extern const int16_t *chsecptr_onextwall;
int32_t checksectorpointer(int16_t i, int16_t sectnum);

#if !KRANDDEBUG
static FORCE_INLINE int32_t krand(void)
{
    randomseed = (randomseed * 1664525ul) + 221297ul;
    return ((uint32_t) randomseed)>>16;
}
#else
int32_t    krand(void);
#endif

int32_t   ksqrt(uint32_t num);
int32_t   __fastcall getangle(int32_t xvect, int32_t yvect);
fix16_t   __fastcall gethiq16angle(int32_t xvect, int32_t yvect);

static FORCE_INLINE fix16_t __fastcall getq16angle(int32_t xvect, int32_t yvect)
{
    return fix16_from_int(getangle(xvect, yvect));
}

static FORCE_INLINE CONSTEXPR uint32_t uhypsq(int32_t const dx, int32_t const dy)
{
    return (uint32_t)dx*dx + (uint32_t)dy*dy;
}

static FORCE_INLINE int32_t logapproach(int32_t const val, int32_t const targetval)
{
    int32_t const dif = targetval - val;
    return (dif>>1) ? val + (dif>>1) : targetval;
}

void rotatepoint(vec2_t const pivot, vec2_t p, int16_t const daang, vec2_t * const p2) ATTRIBUTE((nonnull(4)));
inline void rotatepoint(int px, int py, int ptx, int pty, int daang, int* resx, int* resy)
{
    vec2_t pivot = { px, py };
    vec2_t point = { ptx, pty };
    vec2_t result;
    rotatepoint(pivot, point, daang, &result);
    *resx = result.x;
    *resy = result.y;
}

int32_t   lastwall(int16_t point);
int32_t   nextsectorneighborz(int16_t sectnum, int32_t refz, int16_t topbottom, int16_t direction);

int32_t   getceilzofslopeptr(usectorptr_t sec, int32_t dax, int32_t day) ATTRIBUTE((nonnull(1)));
int32_t   getflorzofslopeptr(usectorptr_t sec, int32_t dax, int32_t day) ATTRIBUTE((nonnull(1)));
void   getzsofslopeptr(usectorptr_t sec, int32_t dax, int32_t day,
                       int32_t *ceilz, int32_t *florz) ATTRIBUTE((nonnull(1,4,5)));
void yax_getzsofslope(int sectNum, int playerX, int playerY, int32_t* pCeilZ, int32_t* pFloorZ);

int32_t yax_getceilzofslope(int const sectnum, vec2_t const vect);
int32_t yax_getflorzofslope(int const sectnum, vec2_t const vect);

static FORCE_INLINE int32_t getceilzofslope(int16_t sectnum, int32_t dax, int32_t day)
{
    return getceilzofslopeptr((usectorptr_t)&sector[sectnum], dax, day);
}

static FORCE_INLINE int32_t getflorzofslope(int16_t sectnum, int32_t dax, int32_t day)
{
    return getflorzofslopeptr((usectorptr_t)&sector[sectnum], dax, day);
}

static FORCE_INLINE void getzsofslope(int16_t sectnum, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    getzsofslopeptr((usectorptr_t)&sector[sectnum], dax, day, ceilz, florz);
}

static FORCE_INLINE void getcorrectzsofslope(int16_t sectnum, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    vec2_t closest = { dax, day };
    getsectordist(closest, sectnum, &closest);
    getzsofslopeptr((usectorptr_t)&sector[sectnum], closest.x, closest.y, ceilz, florz);
}

static FORCE_INLINE int32_t getcorrectceilzofslope(int16_t sectnum, int32_t dax, int32_t day)
{
    vec2_t closest = { dax, day };
    getsectordist(closest, sectnum, &closest);
    return getceilzofslopeptr((usectorptr_t)&sector[sectnum], closest.x, closest.y);
}

static FORCE_INLINE int32_t getcorrectflorzofslope(int16_t sectnum, int32_t dax, int32_t day)
{
    vec2_t closest = { dax, day };
    getsectordist(closest, sectnum, &closest);
    return getflorzofslopeptr((usectorptr_t)&sector[sectnum], closest.x, closest.y);
}

// Is <wal> a red wall in a safe fashion, i.e. only if consistency invariant
// ".nextsector >= 0 iff .nextwall >= 0" holds.
static FORCE_INLINE CONSTEXPR int32_t redwallp(uwallptr_t wal)
{
    return (wal->nextwall >= 0 && wal->nextsector >= 0);
}

static FORCE_INLINE CONSTEXPR int32_t E_SpriteIsValid(const int32_t i)
{
    return ((unsigned)i < MAXSPRITES && sprite[i].statnum != MAXSTATUS);
}

int clipshape_idx_for_sprite(uspriteptr_t curspr, int curidx);

void   alignceilslope(int16_t dasect, int32_t x, int32_t y, int32_t z);
void   alignflorslope(int16_t dasect, int32_t x, int32_t y, int32_t z);
int32_t sectorofwall(int16_t wallNum);
int32_t sectorofwall_noquick(int16_t wallNum);
int32_t   loopnumofsector(int16_t sectnum, int16_t wallnum);
void setslope(int32_t sectnum, int32_t cf, int16_t slope);

int32_t lintersect(int32_t originX, int32_t originY, int32_t originZ,
                   int32_t destX, int32_t destY, int32_t destZ,
                   int32_t lineStartX, int32_t lineStartY, int32_t lineEndX, int32_t lineEndY,
                   int32_t *intersectionX, int32_t *intersectionY, int32_t *intersectionZ);

int32_t rayintersect(int32_t x1, int32_t y1, int32_t z1, int32_t vx, int32_t vy, int32_t vz, int32_t x3,
                     int32_t y3, int32_t x4, int32_t y4, int32_t *intx, int32_t *inty, int32_t *intz);
#if !defined NETCODE_DISABLE
void do_insertsprite_at_headofstat(int16_t spritenum, int16_t statnum);
int32_t insertspritestat(int16_t statnum);
void do_deletespritestat(int16_t deleteme);
void do_insertsprite_at_headofsect(int16_t spritenum, int16_t sectnum);
void do_deletespritesect(int16_t deleteme);
#endif
int32_t insertsprite(int16_t sectnum, int16_t statnum);
int32_t deletesprite(int16_t spritenum);

int32_t   changespritesect(int16_t spritenum, int16_t newsectnum);
int32_t   changespritestat(int16_t spritenum, int16_t newstatnum);
int32_t   setsprite(int16_t spritenum, const vec3_t *) ATTRIBUTE((nonnull(2)));
inline int32_t   setsprite(int16_t spritenum, int x, int y, int z)
{
    vec3_t v = { x,y,z };
    return setsprite(spritenum, &v);
}

inline void setspritepos(int spnum, int x, int y, int z)
{
    sprite[spnum].pos = { x,y,z };
}
int32_t   setspritez(int16_t spritenum, const vec3_t *) ATTRIBUTE((nonnull(2)));

int32_t spriteheightofsptr(uspriteptr_t spr, int32_t *height, int32_t alsotileyofs);
static FORCE_INLINE int32_t spriteheightofs(int16_t i, int32_t *height, int32_t alsotileyofs)
{
    return spriteheightofsptr((uspriteptr_t)&sprite[i], height, alsotileyofs);
}

int videoCaptureScreen();

struct OutputFileCounter {
    uint16_t count = 0;
    FileWriter *opennextfile(char *, char *);
    FileWriter *opennextfile_withext(char *, const char *);
};

// PLAG: line utility functions
typedef struct s_equation
{
    float a, b, c;
} _equation;

int32_t wallvisible(int32_t const x, int32_t const y, int16_t const wallnum);

#define STATUS2DSIZ 144
#define STATUS2DSIZ2 26

int32_t   videoSetRenderMode(int32_t renderer);

#ifdef USE_OPENGL
void    renderSetRollAngle(float rolla);
#endif

//  pal: pass -1 to invalidate all palettes for the tile, or >=0 for a particular palette
//  how: pass -1 to invalidate all instances of the tile in texture memory, or a bitfield
//         bit 0: opaque or masked (non-translucent) texture, using repeating
//         bit 1: ignored
//         bit 2: 33% translucence, using repeating
//         bit 3: 67% translucence, using repeating
//         bit 4: opaque or masked (non-translucent) texture, using clamping
//         bit 5: ignored
//         bit 6: 33% translucence, using clamping
//         bit 7: 67% translucence, using clamping
//       clamping is for sprites, repeating is for walls
void tileInvalidate(int tilenume, int32_t pal, int32_t how);

void PrecacheHardwareTextures(int nTile);
void Polymost_Startup();

typedef uint16_t polytintflags_t;

enum cutsceneflags {
    CUTSCENE_FORCEFILTER = 1,
    CUTSCENE_FORCENOFILTER = 2,
    CUTSCENE_TEXTUREFILTER = 4,
};

enum {
    TEXFILTER_OFF = 0, // GL_NEAREST
    TEXFILTER_ON = 5, // GL_LINEAR_MIPMAP_LINEAR
};

extern int32_t gltexmaxsize;

EXTERN_CVAR(Bool, hw_detailmapping)
EXTERN_CVAR(Bool, hw_glowmapping)
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

extern int32_t r_downsize;
extern int32_t mdtims, omdtims;
extern int32_t glrendmode;

extern int32_t r_rortexture;
extern int32_t r_rortexturerange;
extern int32_t r_rorphase;

// flags bitset: 1 = don't compress
int32_t Ptile2tile(int32_t tile, int32_t palette) ATTRIBUTE((pure));
int32_t md_loadmodel(const char *fn);
int32_t md_setmisc(int32_t modelid, float scale, int32_t shadeoff, float zadd, float yoffset, int32_t flags);
// int32_t md_tilehasmodel(int32_t tilenume, int32_t pal);

extern TArray<FString> g_clipMapFiles;

EXTERN int32_t nextvoxid;
EXTERN intptr_t voxoff[MAXVOXELS][MAXVOXMIPS]; // used in KenBuild
EXTERN int8_t voxreserve[(MAXVOXELS+7)>>3];
EXTERN int8_t voxrotate[(MAXVOXELS+7)>>3];

#ifdef USE_OPENGL
// TODO: dynamically allocate this

typedef struct { vec3f_t add; int16_t angadd, flags, fov; } hudtyp;

typedef struct
{
    // maps build tiles to particular animation frames of a model
    int16_t     modelid;
    int16_t     framenum;   // calculate the number from the name when declaring
    int16_t     nexttile;
    uint16_t    smoothduration;
    hudtyp      *hudmem[2];
    int8_t      skinnum;
    char        pal;
} tile2model_t;

# define EXTRATILES (MAXTILES/8)

EXTERN int32_t mdinited;
EXTERN tile2model_t tile2model[MAXTILES+EXTRATILES];

static FORCE_INLINE int32_t md_tilehasmodel(int32_t const tilenume, int32_t const pal)
{
    return mdinited ? tile2model[Ptile2tile(tilenume,pal)].modelid : -1;
}
#endif  // defined USE_OPENGL

static FORCE_INLINE int tilehasmodelorvoxel(int const tilenume, int pal)
{
    UNREFERENCED_PARAMETER(pal);
    return
#ifdef USE_OPENGL
    (videoGetRenderMode() >= REND_POLYMOST && mdinited && hw_models && tile2model[Ptile2tile(tilenume, pal)].modelid != -1) ||
#endif
    (videoGetRenderMode() <= REND_POLYMOST && r_voxels && tiletovox[tilenume] != -1);
}

int32_t md_defineframe(int32_t modelid, const char *framename, int32_t tilenume,
                       int32_t skinnum, float smoothduration, int32_t pal);
int32_t md_defineanimation(int32_t modelid, const char *framestart, const char *frameend,
                           int32_t fps, int32_t flags);
int32_t md_defineskin(int32_t modelid, const char *skinfn, int32_t palnum, int32_t skinnum,
                      int32_t surfnum, float param, float specpower, float specfactor, int32_t flags);
int32_t md_definehud (int32_t modelid, int32_t tilex, vec3f_t add,
                      int32_t angadd, int32_t flags, int32_t fov);
int32_t md_undefinetile(int32_t tile);
int32_t md_undefinemodel(int32_t modelid);

int32_t loaddefinitionsfile(const char *fn);

// if loadboard() fails with -2 return, try loadoldboard(). if it fails with
// -2, board is dodgy
int32_t engineLoadBoardV5V6(const char *filename, char fromwhere, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum);

#include "hash.h"

#ifdef USE_OPENGL
# include "polymost.h"
#endif

extern int skiptile;

static vec2_t const zerovec = { 0, 0 };

static FORCE_INLINE CONSTEXPR int inside_p(int32_t const x, int32_t const y, int const sectnum) { return (sectnum >= 0 && inside(x, y, sectnum) == 1); }

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

static inline int32_t clipmove_old(int32_t *x, int32_t *y, int32_t *z, int16_t *sectnum, int32_t xvect, int32_t yvect, int32_t walldist,
                   int32_t ceildist, int32_t flordist, uint32_t cliptype) ATTRIBUTE((nonnull(1,2,3,4)));

static inline int32_t clipmove_old(int32_t *x, int32_t *y, int32_t *z, int16_t *sectnum, int32_t xvect, int32_t yvect, int32_t walldist,
                   int32_t ceildist, int32_t flordist, uint32_t cliptype)
{
    vec3_t vector = { *x, *y, *z };

    int32_t result = clipmove(&vector, sectnum, xvect, yvect, walldist, ceildist, flordist, cliptype);

    *x = vector.x;
    *y = vector.y;
    *z = vector.z;

    return result;
}

static inline int32_t pushmove_old(int32_t *x, int32_t *y, int32_t *z, int16_t *sectnum, int32_t walldist,
                   int32_t ceildist, int32_t flordist, uint32_t cliptype) ATTRIBUTE((nonnull(1,2,3,4)));

static inline int32_t pushmove_old(int32_t *x, int32_t *y, int32_t *z, int16_t *sectnum, int32_t walldist,
                   int32_t ceildist, int32_t flordist, uint32_t cliptype)
{
    vec3_t vector = { *x, *y, *z };

    int32_t result = pushmove(&vector, sectnum, walldist, ceildist, flordist, cliptype);

    *x = vector.x;
    *y = vector.y;
    *z = vector.z;

    return result;
}

static inline void getzrange_old(int32_t x, int32_t y, int32_t z, int16_t sectnum, int32_t *ceilz, int32_t *ceilhit, int32_t *florz,
                 int32_t *florhit, int32_t walldist, uint32_t cliptype) ATTRIBUTE((nonnull(5,6,7,8)));

static inline void getzrange_old(int32_t x, int32_t y, int32_t z, int16_t sectnum, int32_t *ceilz, int32_t *ceilhit, int32_t *florz,
                 int32_t *florhit, int32_t walldist, uint32_t cliptype)
{
    const vec3_t vector = { x, y, z };
    getzrange(&vector, sectnum, ceilz, ceilhit, florz, florhit, walldist, cliptype);
}

static inline int32_t setspritez_old(int16_t spritenum, int32_t x, int32_t y, int32_t z)
{
    const vec3_t vector = { x, y, z };
    return setspritez(spritenum, &vector);
}

extern int32_t rintersect(int32_t x1, int32_t y1, int32_t z1,
    int32_t vx_, int32_t vy_, int32_t vz,
    int32_t x3, int32_t y3, int32_t x4, int32_t y4,
    int32_t *intx, int32_t *inty, int32_t *intz);

void markTileForPrecache(int tilenum, int palnum);
void precacheMarkedTiles();

extern int32_t(*animateoffs_replace)(int const tilenum, int fakevar);
extern void(*paletteLoadFromDisk_replace)(void);
extern int32_t(*getpalookup_replace)(int32_t davis, int32_t dashade);
extern void(*initspritelists_replace)(void);
extern int32_t(*insertsprite_replace)(int16_t sectnum, int16_t statnum);
extern int32_t(*deletesprite_replace)(int16_t spritenum);
extern int32_t(*changespritesect_replace)(int16_t spritenum, int16_t newsectnum);
extern int32_t(*changespritestat_replace)(int16_t spritenum, int16_t newstatnum);
extern void(*loadvoxel_replace)(int32_t voxel);
extern int32_t(*loadboard_replace)(const char *filename, char flags, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum);
#ifdef USE_OPENGL
extern void(*PolymostProcessVoxels_Callback)(void);
#endif

class F2DDrawer;

extern F2DDrawer twodpsp;
extern F2DDrawer* twod;

// This is for safely substituting the 2D drawer for a block of code.
class PspTwoDSetter
{
	F2DDrawer* old;
public:
	PspTwoDSetter()
	{
		old = twod;
		twod = &twodpsp;
	}
	~PspTwoDSetter()
	{
		twod = old;
	}
	// Shadow Warrior fucked this up and draws the weapons in the same pass as the hud, meaning we have to switch this on and off depending on context.
	void set()
	{
		twod = &twodpsp;
	}
	void clear()
	{
		twod = old;
	}
};

#endif // build_h_
