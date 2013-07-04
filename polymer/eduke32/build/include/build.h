// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)


#ifndef __build_h__
#define __build_h__

#include "compat.h"
#include "pragmas.h"
#include "glbuild.h"

#ifdef EXTERNC
extern "C" {
#endif

enum rendmode_t {
    REND_CLASSIC,
    REND_POLYMOST = 3,
    REND_POLYMER
};

#define PI 3.14159265358979323846

#define MAXSECTORSV8 4096
#define MAXWALLSV8 16384
#define MAXSPRITESV8 16384

#define MAXSECTORSV7 1024
#define MAXWALLSV7 8192
#define MAXSPRITESV7 4096

#ifndef GEKKO
# define MAXSECTORS MAXSECTORSV8
# define MAXWALLS MAXWALLSV8
# define MAXSPRITES MAXSPRITESV8

# define MAXXDIM 7680
# define MAXYDIM 3200

#ifdef LUNATIC
# define NEW_MAP_FORMAT
// A marker for LuaJIT C function callbacks, but not merely:
# define LUNATIC_CB ATTRIBUTE((used))
// Used for variables and functions referenced from Lua:
# define LUNATIC_EXTERN ATTRIBUTE((used))
#else
# ifdef NEW_MAP_FORMAT
#  error "New map format can only be used with Lunatic"
# endif
# define LUNATIC_EXTERN static
#endif

// additional space beyond wall, in walltypes:
# define M32_FIXME_WALLS 512
# define M32_FIXME_SECTORS 2
#else
# define MAXSECTORS MAXSECTORSV7
# define MAXWALLS MAXWALLSV7
# define MAXSPRITES MAXSPRITESV7

# define MAXXDIM 860
# define MAXYDIM 490

# define M32_FIXME_WALLS 0
# define M32_FIXME_SECTORS 0
#endif

#define MAXWALLSB ((MAXWALLS>>2)+(MAXWALLS>>3))

#define MAXTILES 30720
#define MAXVOXELS 4096
#define MAXSTATUS 1024
#define MAXPLAYERS 16
#define MAXBASEPALS 8
#define MAXPALOOKUPS 256
#define MAXPSKYMULTIS 8
#define MAXPSKYTILES 256
#define MAXSPRITESONSCREEN 4096
#define MAXUNIQHUDID 256 //Extra slots so HUD models can store animation state without messing game sprites

#define RESERVEDPALS 4 // don't forget to increment this when adding reserved pals
#define DETAILPAL   (MAXPALOOKUPS - 1)
#define GLOWPAL     (MAXPALOOKUPS - 2)
#define SPECULARPAL (MAXPALOOKUPS - 3)
#define NORMALPAL   (MAXPALOOKUPS - 4)

#define TSPR_TEMP 99
#define TSPR_MIRROR 100

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

#define CLEARLINES2D(Startline, Numlines, Color) \
    clearbuf((char *)(frameplace + ((Startline)*bytesperline)), (bytesperline*(Numlines))>>2, (Color))


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

# ifdef NEW_MAP_FORMAT
#  define YAX_MAXBUNCHES 512
#  define YAX_BIT__COMPAT 1024
#  define YAX_NEXTWALLBIT__COMPAT(Cf) (1<<(10+Cf))
#  define YAX_NEXTWALLBITS__COMPAT (YAX_NEXTWALLBIT__COMPAT(0)|YAX_NEXTWALLBIT__COMPAT(1))
# else
#  define YAX_MAXBUNCHES 256
#  define YAX_BIT 1024
   // "has next wall when constrained"-bit (1<<10: ceiling, 1<<11: floor)
#  define YAX_NEXTWALLBIT(Cf) (1<<(10+Cf))
#  define YAX_NEXTWALLBITS (YAX_NEXTWALLBIT(0)|YAX_NEXTWALLBIT(1))
# endif

int32_t get_alwaysshowgray(void);  // editor only
void yax_updategrays(int32_t posze);

#ifdef YAX_ENABLE
# ifdef NEW_MAP_FORMAT
   // New map format -- no hijacking of otherwise used members.
#  define YAX_PTRNEXTWALL(Ptr, Wall, Cf) (*(&Ptr[Wall].upwall + Cf))
#  define YAX_NEXTWALLDEFAULT(Cf) (-1)
# else
   // More user tag hijacking: lotag/extra. :/
#  define YAX_PTRNEXTWALL(Ptr, Wall, Cf) (*(int16_t *)(&Ptr[Wall].lotag + 2*Cf))
#  define YAX_NEXTWALLDEFAULT(Cf) (((Cf)==YAX_CEILING) ? 0 : -1)
   extern int16_t yax_bunchnum[MAXSECTORS][2];
   extern int16_t yax_nextwall[MAXWALLS][2];
# endif

# define YAX_NEXTWALL(Wall, Cf) YAX_PTRNEXTWALL(wall, Wall, Cf)

# define YAX_ITER_WALLS(Wal, Itervar, Cfvar) Cfvar=0, Itervar=(Wal); Itervar!=-1; \
    Itervar=yax_getnextwall(Itervar, Cfvar), \
        (void)(Itervar==-1 && Cfvar==0 && (Cfvar=1) && (Itervar=yax_getnextwall((Wal), Cfvar)))

# define SECTORS_OF_BUNCH(Bunchnum, Cf, Itervar) Itervar = headsectbunch[Cf][Bunchnum]; \
    Itervar != -1; Itervar = nextsectbunch[Cf][Itervar]

extern int32_t r_tror_nomaskpass;

# ifdef NEW_MAP_FORMAT
// Moved below declarations of sector, wall, sprite.
# else
int16_t yax_getbunch(int16_t i, int16_t cf);
void yax_getbunches(int16_t i, int16_t *cb, int16_t *fb);
int16_t yax_getnextwall(int16_t wal, int16_t cf);
void yax_setnextwall(int16_t wal, int16_t cf, int16_t thenextwall);
# endif

void yax_setbunch(int16_t i, int16_t cf, int16_t bunchnum);
void yax_setbunches(int16_t i, int16_t cb, int16_t fb);
int16_t yax_vnextsec(int16_t line, int16_t cf);
void yax_update(int32_t resetstat);
int32_t yax_getneighborsect(int32_t x, int32_t y, int32_t sectnum, int32_t cf);

static inline int32_t yax_waltosecmask(int32_t walclipmask)
{
    // blocking: walstat&1 --> secstat&512
    // hitscan: walstat&64 --> secstat&2048
    return ((walclipmask&1)<<9) | ((walclipmask&64)<<5);
}
void yax_preparedrawrooms(void);
void yax_drawrooms(void (*SpriteAnimFunc)(int32_t,int32_t,int32_t,int32_t),
                   int16_t sectnum, int32_t didmirror, int32_t smoothr);
# define YAX_SKIPSECTOR(i) if (graysectbitmap[(i)>>3]&(1<<((i)&7))) continue
# define YAX_SKIPWALL(i) if (graywallbitmap[(i)>>3]&(1<<((i)&7))) continue
#else
# define yax_preparedrawrooms()
# define yax_drawrooms(SpriteAnimFunc, sectnum, didmirror, smoothr)
# define YAX_SKIPSECTOR(i) (i)=(i)
# define YAX_SKIPWALL(i) (i)=(i)
#endif

#define CLIPMASK0 (((1L)<<16)+1L)
#define CLIPMASK1 (((256L)<<16)+64L)

// max x/y val (= max editorgridextent in Mapster32)
#define BXY_MAX 524288

enum {
    ROTATESPRITE_FULL16 = 2048,
    // ROTATESPRITE_MAX-1 is the mask of all externally available orientation bits
    ROTATESPRITE_MAX = 4096,

    RS_CENTERORIGIN = (1<<30),
};

    //Make all variables in BUILD.H defined in the ENGINE,
    //and externed in GAME
#ifdef ENGINE
#  define EXTERN
#else
#  define EXTERN extern
#endif

#ifdef __cplusplus

static inline void sector_tracker_hook(uintptr_t address);
static inline void wall_tracker_hook(uintptr_t address);
static inline void sprite_tracker_hook(uintptr_t address);

#define __TRACKER_NAME SectorTracker
#define __TRACKER_GLOBAL_HOOK sector_tracker_hook
#include "tracker.hpp"
#undef __TRACKER_NAME
#undef __TRACKER_GLOBAL_HOOK

#define __TRACKER_NAME WallTracker
#define __TRACKER_GLOBAL_HOOK wall_tracker_hook
#include "tracker.hpp"
#undef __TRACKER_NAME
#undef __TRACKER_GLOBAL_HOOK

#define __TRACKER_NAME SpriteTracker
#define __TRACKER_GLOBAL_HOOK sprite_tracker_hook
#include "tracker.hpp"
#undef __TRACKER_NAME
#undef __TRACKER_GLOBAL_HOOK

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


//ceilingstat/floorstat:
//   bit 0: 1 = parallaxing, 0 = not                                 "P"
//   bit 1: 1 = groudraw, 0 = not
//   bit 2: 1 = swap x&y, 0 = not                                    "F"
//   bit 3: 1 = double smooshiness                                   "E"
//   bit 4: 1 = x-flip                                               "F"
//   bit 5: 1 = y-flip                                               "F"
//   bit 6: 1 = Align texture to first wall of sector                "R"
//   bits 8-7:                                                       "T"
//          00 = normal floors
//          01 = masked floors
//          10 = transluscent masked floors
//          11 = reverse transluscent masked floors
//   bit 9: 1 = blocking ceiling/floor
//   bit 10: 1 = YAX'ed ceiling/floor
//   bit 11: 1 = hitscan-sensitive ceiling/floor
//   bits 12-15: reserved

//////////////////// Version 7 map format ////////////////////

    //40 bytes
typedef struct
{
    Tracker(Sector, int16_t) wallptr, wallnum;
    Tracker(Sector, int32_t) ceilingz, floorz;
    Tracker(Sector, uint16_t) ceilingstat, floorstat;
    Tracker(Sector, int16_t) ceilingpicnum, ceilingheinum;
    Tracker(Sector, int8_t) ceilingshade;
    Tracker(Sector, uint8_t) ceilingpal, /*CM_FLOORZ:*/ ceilingxpanning, ceilingypanning;
    Tracker(Sector, int16_t) floorpicnum, floorheinum;
    Tracker(Sector, int8_t) floorshade;
    Tracker(Sector, uint8_t) floorpal, floorxpanning, floorypanning;
    Tracker(Sector, uint8_t) /*CM_CEILINGZ:*/ visibility, filler;
    Tracker(Sector, uint16_t) lotag, hitag;
    Tracker(Sector, int16_t) extra;
} sectortypev7;

//cstat:
//   bit 0: 1 = Blocking wall (use with clipmove, getzrange)         "B"
//   bit 1: 1 = bottoms of invisible walls swapped, 0 = not          "2"
//   bit 2: 1 = align picture on bottom (for doors), 0 = top         "O"
//   bit 3: 1 = x-flipped, 0 = normal                                "F"
//   bit 4: 1 = masking wall, 0 = not                                "M"
//   bit 5: 1 = 1-way wall, 0 = not                                  "1"
//   bit 6: 1 = Blocking wall (use with hitscan / cliptype 1)        "H"
//   bit 7: 1 = Transluscence, 0 = not                               "T"
//   bit 8: 1 = y-flipped, 0 = normal                                "F"
//   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
//   bits 10 and 11: reserved (in use by YAX)
//   bits 12-15: reserved  (14: temp use by editor)

    //32 bytes
typedef struct
{
    Tracker(Wall, int32_t) x, y;
    Tracker(Wall, int16_t) point2, nextwall, nextsector;
    Tracker(Wall, uint16_t) cstat;
    Tracker(Wall, int16_t) picnum, overpicnum;
    Tracker(Wall, int8_t) shade;
    Tracker(Wall, uint8_t) pal, xrepeat, yrepeat, xpanning, ypanning;
    Tracker(Wall, uint16_t) lotag, hitag;
    Tracker(Wall, int16_t) extra;
} walltypev7;


enum {
    SPR_XFLIP = 4,
    SPR_YFLIP = 8,

    SPR_WALL = 16,
    SPR_FLOOR = 32,
    SPR_ALIGN_MASK = 32+16,
};

//cstat:
//   bit 0: 1 = Blocking sprite (use with clipmove, getzrange)       "B"
//   bit 1: 1 = transluscence, 0 = normal                            "T"
//   bit 2: 1 = x-flipped, 0 = normal                                "F"
//   bit 3: 1 = y-flipped, 0 = normal                                "F"
//   bits 5-4: 00 = FACE sprite (default)                            "R"
//             01 = WALL sprite (like masked walls)
//             10 = FLOOR sprite (parallel to ceilings&floors)
//   bit 6: 1 = 1-sided sprite, 0 = normal                           "1"
//   bit 7: 1 = Real centered centering, 0 = foot center             "C"
//   bit 8: 1 = Blocking sprite (use with hitscan / cliptype 1)      "H"
//   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
//   bit 10: reserved (in use by a renderer hack, see CSTAT_SPRITE_MDHACK)
//   bit 11: 1 = determine shade based only on its own shade member (see CON's spritenoshade command)
//   bit 12: reserved
//   bit 13: 1 = does not cast shadow
//   bit 14: 1 = invisible but casts shadow
//   bit 15: 1 = Invisible sprite, 0 = not invisible

    //44 bytes
typedef struct
{
    Tracker(Sprite, int32_t) x, y, z;
    Tracker(Sprite, uint16_t) cstat;
    Tracker(Sprite, int16_t) picnum;
    Tracker(Sprite, int8_t) shade;
    Tracker(Sprite, uint8_t) pal, clipdist, filler;
    Tracker(Sprite, uint8_t) xrepeat, yrepeat;
    Tracker(Sprite, int8_t) xoffset, yoffset;
    Tracker(Sprite, int16_t) sectnum, statnum;
    Tracker(Sprite, int16_t) ang, owner, xvel, yvel, zvel;
    Tracker(Sprite, uint16_t) lotag, hitag;
    Tracker(Sprite, int16_t) extra;
} spritetype;

//////////////////// END Version 7 map format ////////////////

#ifdef NEW_MAP_FORMAT
//////////////////// Lunatic new-generation map format ////////////////////

// 44 bytes
typedef struct
{
    Tracker(Sector, int16_t) wallptr, wallnum;

    Tracker(Sector, int16_t) ceilingpicnum, ceilingheinum, ceilingbunch;
    Tracker(Sector, uint16_t) ceilingstat;
    Tracker(Sector, int32_t) ceilingz;
    Tracker(Sector, int8_t) ceilingshade;
    Tracker(Sector, uint8_t) ceilingpal, /*CM_FLOORZ:*/ ceilingxpanning, ceilingypanning;

    Tracker(Sector, int16_t) floorpicnum, floorheinum, floorbunch;
    Tracker(Sector, uint16_t) floorstat;
    Tracker(Sector, int32_t) floorz;
    Tracker(Sector, int8_t) floorshade;
    Tracker(Sector, uint8_t) floorpal, floorxpanning, floorypanning;

    Tracker(Sector, uint8_t) /*CM_CEILINGZ:*/ visibility, filler;
    Tracker(Sector, uint16_t) lotag, hitag;
    Tracker(Sector, int16_t) extra;
} sectortypevx;

# define SECTORVX_SZ1 offsetof(sectortypevx, ceilingpicnum)
# define SECTORVX_SZ4 sizeof(sectortypevx)-offsetof(sectortypevx, visibility)

static inline void copy_v7_from_vx_sector(sectortypev7 *v7sec, const sectortypevx *vxsec)
{
    /* [wallptr..wallnum] */
    Bmemcpy(v7sec, vxsec, SECTORVX_SZ1);

    /* ceiling* */
    v7sec->ceilingpicnum = vxsec->ceilingpicnum;
    v7sec->ceilingheinum = vxsec->ceilingheinum;
    v7sec->ceilingstat = vxsec->ceilingstat;
    v7sec->ceilingz = vxsec->ceilingz;
    v7sec->ceilingshade = vxsec->ceilingshade;
    v7sec->ceilingpal = vxsec->ceilingpal;
    v7sec->ceilingxpanning = vxsec->ceilingxpanning;
    v7sec->ceilingypanning = vxsec->ceilingypanning;

    /* floor* */
    v7sec->floorpicnum = vxsec->floorpicnum;
    v7sec->floorheinum = vxsec->floorheinum;
    v7sec->floorstat = vxsec->floorstat;
    v7sec->floorz = vxsec->floorz;
    v7sec->floorshade = vxsec->floorshade;
    v7sec->floorpal = vxsec->floorpal;
    v7sec->floorxpanning = vxsec->floorxpanning;
    v7sec->floorypanning = vxsec->floorypanning;

    /* [visibility..extra] */
    Bmemcpy(&v7sec->visibility, &vxsec->visibility, SECTORVX_SZ4);

    /* Clear YAX_BIT of ceiling and floor. (New-map format build saves TROR
     * maps as map-text.) */
    v7sec->ceilingstat &= ~YAX_BIT__COMPAT;
    v7sec->floorstat &= ~YAX_BIT__COMPAT;
}

static inline void inplace_vx_from_v7_sector(sectortypevx *vxsec)
{
    const sectortypev7 *v7sec = (sectortypev7 *)vxsec;
    sectortypev7 bakv7sec;

    // Can't do this in-place since the members were rearranged.
    Bmemcpy(&bakv7sec, v7sec, sizeof(sectortypev7));

    /* [wallptr..wallnum] is already at the right place */

    /* ceiling* */
    vxsec->ceilingpicnum = bakv7sec.ceilingpicnum;
    vxsec->ceilingheinum = bakv7sec.ceilingheinum;
    vxsec->ceilingstat = bakv7sec.ceilingstat;
    vxsec->ceilingz = bakv7sec.ceilingz;
    vxsec->ceilingshade = bakv7sec.ceilingshade;
    vxsec->ceilingpal = bakv7sec.ceilingpal;
    vxsec->ceilingxpanning = bakv7sec.ceilingxpanning;
    vxsec->ceilingypanning = bakv7sec.ceilingypanning;

    /* floor* */
    vxsec->floorpicnum = bakv7sec.floorpicnum;
    vxsec->floorheinum = bakv7sec.floorheinum;
    vxsec->floorstat = bakv7sec.floorstat;
    vxsec->floorz = bakv7sec.floorz;
    vxsec->floorshade = bakv7sec.floorshade;
    vxsec->floorpal = bakv7sec.floorpal;
    vxsec->floorxpanning = bakv7sec.floorxpanning;
    vxsec->floorypanning = bakv7sec.floorypanning;

    /* [visibility..extra] */
    Bmemmove(&vxsec->visibility, &bakv7sec.visibility, SECTORVX_SZ4);
}

static inline void inplace_vx_tweak_sector(sectortypevx *vxsec, int32_t yaxp)
{
    if (yaxp)
    {
        int32_t cisext = (vxsec->ceilingstat&YAX_BIT__COMPAT);
        int32_t fisext = (vxsec->floorstat&YAX_BIT__COMPAT);

        vxsec->ceilingbunch = cisext ? vxsec->ceilingxpanning : -1;
        vxsec->floorbunch = fisext ? vxsec->floorxpanning : -1;

        if (cisext)
            vxsec->ceilingxpanning = 0;
        if (fisext)
            vxsec->floorxpanning = 0;
    }
    else
    {
        vxsec->ceilingbunch = vxsec->floorbunch = -1;
    }

    /* Clear YAX_BIT of ceiling and floor (map-int VX doesn't use it). */
    vxsec->ceilingstat &= ~YAX_BIT__COMPAT;
    vxsec->floorstat &= ~YAX_BIT__COMPAT;
}

# undef SECTORVX_SZ1
# undef SECTORVX_SZ4

// 36 bytes
typedef struct
{
    Tracker(Wall, int32_t) x, y;
    Tracker(Wall, int16_t) point2, nextwall, nextsector;
    Tracker(Wall, int16_t) upwall, dnwall;
    Tracker(Wall, uint16_t) cstat;
    Tracker(Wall, int16_t) picnum, overpicnum;
    Tracker(Wall, int8_t) shade;
    Tracker(Wall, uint8_t) pal, xrepeat, yrepeat, xpanning, ypanning;
    Tracker(Wall, uint16_t) lotag, hitag;
    Tracker(Wall, int16_t) extra;
} walltypevx;

# define WALLVX_SZ2 sizeof(walltypevx)-offsetof(walltypevx, cstat)

static inline void copy_v7_from_vx_wall(walltypev7 *v7wal, const walltypevx *vxwal)
{
    /* [x..nextsector] */
    Bmemcpy(v7wal, vxwal, offsetof(walltypevx, upwall));
    /* [cstat..extra] */
    Bmemcpy(&v7wal->cstat, &vxwal->cstat, WALLVX_SZ2);
    /* Clear YAX_NEXTWALLBITS. */
    v7wal->cstat &= ~YAX_NEXTWALLBITS__COMPAT;
}

static inline void inplace_vx_from_v7_wall(walltypevx *vxwal)
{
    const walltypev7 *v7wal = (walltypev7 *)vxwal;

    /* [cstat..extra] */
    Bmemmove(&vxwal->cstat, &v7wal->cstat, WALLVX_SZ2);
}

static inline void inplace_vx_tweak_wall(walltypevx *vxwal, int32_t yaxp)
{
    if (yaxp)
    {
        int32_t haveupwall = (vxwal->cstat & YAX_NEXTWALLBIT__COMPAT(YAX_CEILING));
        int32_t havednwall = (vxwal->cstat & YAX_NEXTWALLBIT__COMPAT(YAX_FLOOR));

        vxwal->upwall = haveupwall ? vxwal->lotag : -1;
        vxwal->dnwall = havednwall ? vxwal->extra : -1;

        if (haveupwall)
            vxwal->lotag = 0;
        if (havednwall)
            vxwal->extra = -1;
    }
    else
    {
        vxwal->upwall = vxwal->dnwall = -1;
    }

    /* Clear YAX_NEXTWALLBITS (map-int VX doesn't use it). */
    vxwal->cstat &= ~YAX_NEXTWALLBITS__COMPAT;
}

# undef WALLVX_SZ2

// NOTE: spritetype is currently the same for V7/8/9 and VX in-memory map formats.

typedef sectortypevx sectortype;
typedef walltypevx walltype;
//////////////////// END Lunatic new-generation map format ////////////////
#else
typedef sectortypev7 sectortype;
typedef walltypev7 walltype;
#endif

typedef struct {
    uint32_t mdanimtims;
    int16_t mdanimcur;
    int16_t angoff, pitch, roll;
    int32_t xoff, yoff, zoff;
    uint8_t flags;
    uint8_t xpanning, ypanning;
    uint8_t filler;
    float alpha;
    // NOTE: keep 'tspr' on an 8-byte boundary:
    spritetype *tspr;
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

#define SPREXT_NOTMD 1
#define SPREXT_NOMDANIM 2
#define SPREXT_AWAY1 4
#define SPREXT_AWAY2 8
#define SPREXT_TSPRACCESS 16
#define SPREXT_TEMPINVISIBLE 32

#define CSTAT_SPRITE_MDHACK 1024

EXTERN int32_t guniqhudid;
EXTERN int32_t spritesortcnt;
extern int32_t g_loadedMapVersion;

#if !defined DEBUG_MAIN_ARRAYS
EXTERN spriteext_t *spriteext;
EXTERN spritesmooth_t *spritesmooth;

EXTERN sectortype *sector;
EXTERN walltype *wall;
EXTERN spritetype *sprite;
EXTERN spritetype *tsprite;
#else
EXTERN spriteext_t spriteext[MAXSPRITES+MAXUNIQHUDID];
EXTERN spritesmooth_t spritesmooth[MAXSPRITES+MAXUNIQHUDID];

EXTERN sectortype sector[MAXSECTORS + M32_FIXME_SECTORS];
EXTERN walltype wall[MAXWALLS + M32_FIXME_WALLS];
EXTERN spritetype sprite[MAXSPRITES];
EXTERN spritetype tsprite[MAXSPRITESONSCREEN];
#endif

EXTERN uint32_t sectorchanged[MAXSECTORS + M32_FIXME_SECTORS];
EXTERN uint32_t wallchanged[MAXWALLS + M32_FIXME_WALLS];
EXTERN uint32_t spritechanged[MAXSPRITES];

#ifdef NEW_MAP_FORMAT
static inline int16_t yax_getbunch(int16_t i, int16_t cf)
{
    return cf ? sector[i].floorbunch : sector[i].ceilingbunch;
}

static inline void yax_getbunches(int16_t i, int16_t *cb, int16_t *fb)
{
    *cb = yax_getbunch(i, YAX_CEILING);
    *fb = yax_getbunch(i, YAX_FLOOR);
}

static inline int16_t yax_getnextwall(int16_t wal, int16_t cf)
{
    return cf ? wall[wal].dnwall : wall[wal].upwall;
}

static inline void yax_setnextwall(int16_t wal, int16_t cf, int16_t thenextwall)
{
    YAX_NEXTWALL(wal, cf) = thenextwall;
}
#endif

static inline void sector_tracker_hook(uintptr_t address)
{
    address -= (uintptr_t)(sector);
    address /= sizeof(sectortype);

    if (address > MAXSECTORS + M32_FIXME_SECTORS) return;

    sectorchanged[address]++;
}

static inline void wall_tracker_hook(uintptr_t address)
{
    address -= (uintptr_t)(wall);
    address /= sizeof(walltype);

    if (address > MAXWALLS + M32_FIXME_WALLS) return;

    wallchanged[address]++;
}

static inline void sprite_tracker_hook(uintptr_t address)
{
    if (address >= (uintptr_t)(sprite) &&
        address < (uintptr_t)(sprite) + MAXSPRITES * sizeof(spritetype))
    {
        address -= (uintptr_t)(sprite);
        address /= sizeof(spritetype);

        spritechanged[address]++;
    }
}

EXTERN int16_t maskwall[MAXWALLSB], maskwallcnt;
EXTERN int16_t thewall[MAXWALLSB];
EXTERN spritetype *tspriteptr[MAXSPRITESONSCREEN + 1];

EXTERN int32_t xdim, ydim, numpages;
EXTERN int32_t yxaspect, viewingrange;
#ifdef __cplusplus
extern "C" {
#endif
EXTERN intptr_t ylookup[MAXYDIM+1];
#ifdef __cplusplus
};
#endif

#define MAXVALIDMODES 256
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
EXTERN char display_mirror;
EXTERN int32_t totalclock;
EXTERN int32_t numframes, randomseed;
EXTERN int16_t sintable[2048];
EXTERN uint8_t palette[768];
EXTERN int16_t numshades;
EXTERN char *palookup[MAXPALOOKUPS];
EXTERN uint8_t **basepaltableptr;
EXTERN char parallaxtype, showinvisibility;
EXTERN int32_t parallaxyoffs, parallaxyscale;
EXTERN int32_t g_visibility, parallaxvisibility;
EXTERN int32_t g_rotatespriteNoWidescreen;

EXTERN int32_t windowx1, windowy1, windowx2, windowy2;
EXTERN int16_t startumost[MAXXDIM], startdmost[MAXXDIM];

// original multi-psky handling (only one per map)
EXTERN int16_t pskyoff[MAXPSKYTILES], pskybits;
// new multi-psky -- up to MAXPSKYMULTIS
EXTERN int16_t pskynummultis;
EXTERN int32_t pskymultiyscale[MAXPSKYMULTIS];
EXTERN int16_t pskymultilist[MAXPSKYMULTIS], pskymultibits[MAXPSKYMULTIS];
EXTERN int16_t pskymultioff[MAXPSKYMULTIS][MAXPSKYTILES];

// last sprite in the freelist, that is the spritenum for which
//   .statnum==MAXSTATUS && nextspritestat[spritenum]==-1
// (or -1 if freelist is empty):
EXTERN int16_t tailspritefree;

EXTERN int16_t headspritesect[MAXSECTORS+1], headspritestat[MAXSTATUS+1];
EXTERN int16_t prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
EXTERN int16_t nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];

EXTERN int16_t tilesizx[MAXTILES], tilesizy[MAXTILES];
EXTERN char picsiz[MAXTILES];
EXTERN char walock[MAXTILES];
#ifdef __cplusplus
extern "C" {
#endif
extern const char pow2char[8];
extern const int32_t pow2long[32];
#ifdef __cplusplus
};
#endif

// picanm[].sf:
// |bit(1<<7)
// |animtype|animtype|texhitscan|nofullbright|speed|speed|speed|speed|
enum {
    PICANM_ANIMTYPE_NONE = 0,
    PICANM_ANIMTYPE_OSC = (1<<6),
    PICANM_ANIMTYPE_FWD = (2<<6),
    PICANM_ANIMTYPE_BACK = (3<<6),

    PICANM_ANIMTYPE_SHIFT = 6,
    PICANM_ANIMTYPE_MASK = (3<<6),  // must be 192
    PICANM_MISC_MASK = (3<<4),
    PICANM_TEXHITSCAN_BIT = (2<<4),
    PICANM_NOFULLBRIGHT_BIT = (1<<4),
    PICANM_ANIMSPEED_MASK = 15,  // must be 15
};

// NOTE: If the layout of this struct is changed, loadpics() must be modified
// accordingly.
typedef struct {
    uint8_t num;  // animate number
    int8_t xofs, yofs;
    uint8_t sf;  // anim. speed and flags
} picanm_t;
EXTERN picanm_t picanm[MAXTILES];
EXTERN intptr_t waloff[MAXTILES];  // stores pointers to cache  -- SA

EXTERN int32_t windowpos, windowx, windowy;

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

EXTERN char show2dsector[(MAXSECTORS+7)>>3];
EXTERN char show2dwall[(MAXWALLS+7)>>3];
EXTERN char show2dsprite[(MAXSPRITES+7)>>3];

// In the editor, gotpic is only referenced from inline assembly;
// the compiler needs that hint or building with LTO will discard it.
#ifndef __clang__
# define GOTPIC_USED ATTRIBUTE((used))
#else
# define GOTPIC_USED
#endif

EXTERN char GOTPIC_USED gotpic[(MAXTILES+7)>>3];
EXTERN char gotsector[(MAXSECTORS+7)>>3];

EXTERN char editorcolors[256];

EXTERN int32_t faketilesiz[MAXTILES];
EXTERN char *faketiledata[MAXTILES];

EXTERN char spritecol2d[MAXTILES][2];
extern char vgapal16[4*256];

extern uint32_t drawlinepat;

extern void faketimerhandler(void);

extern char apptitle[256];
typedef struct {
    char r,g,b,f;
} palette_t;
extern palette_t curpalette[256], curpalettefaded[256], palfadergb;
extern char palfadedelta;

extern int32_t novoxmips;

#ifdef DEBUGGINGAIDS
extern float debug1, debug2;
#endif

extern int16_t tiletovox[MAXTILES];
extern int32_t usevoxels, voxscale[MAXVOXELS];

#ifdef USE_OPENGL
extern int32_t usemodels, usehightile;
extern int32_t rendmode;
#endif
EXTERN int32_t h_xsize[MAXTILES], h_ysize[MAXTILES];
EXTERN int8_t h_xoffs[MAXTILES], h_yoffs[MAXTILES];

extern const char *engineerrstr;

EXTERN int32_t editorzrange[2];

static inline int32_t getrendermode(void)
{
#ifndef USE_OPENGL
    return REND_CLASSIC;
#else
    return rendmode;
#endif
}

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
        TILESIZX[MAXTILES] - simply the x-dimension of the tile number.
        TILESIZY[MAXTILES] - simply the y-dimension of the tile number.
        WALOFF[MAXTILES] - the actual address pointing to the top-left
                                 corner of the tile.
        PICANM[MAXTILES] - flags for animating the tile.

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
    int32_t x, y;
} vec2_t;

typedef struct {
    int32_t x, y, z;
} vec3_t;

typedef struct {
    vec3_t pos;
    int16_t sprite, wall, sect;
} hitdata_t;


int32_t    preinitengine(void);	// a partial setup of the engine used for launch windows
int32_t    initengine(void);
void   uninitengine(void);
void   initspritelists(void);
int32_t   loadboard(const char *filename, char flags, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum);
int32_t   loadmaphack(const char *filename);
void delete_maphack_lights();
#ifdef HAVE_CLIPSHAPE_FEATURE
int32_t clipmapinfo_load(void);
#endif
int32_t   saveboard(const char *filename, const vec3_t *dapos, int16_t daang, int16_t dacursectnum);
void set_tilesiz(int32_t picnum, int16_t dasizx, int16_t dasizy);
int32_t tile_exists(int32_t picnum);
int32_t   loadpics(const char *filename, int32_t askedsize);
void   loadtile(int16_t tilenume);
int32_t   qloadkvx(int32_t voxindex, const char *filename);
intptr_t   allocatepermanenttile(int16_t tilenume, int32_t xsiz, int32_t ysiz);
//void   copytilepiece(int32_t tilenume1, int32_t sx1, int32_t sy1, int32_t xsiz, int32_t ysiz, int32_t tilenume2, int32_t sx2, int32_t sy2);
void   makepalookup(int32_t palnum, const char *remapbuf, int8_t r, int8_t g, int8_t b, char dastat);
//void   setvgapalette(void);
void   setbasepaltable(uint8_t **basepaltable, uint8_t basepalcount);
void   setbrightness(char dabrightness, uint8_t dapalid, uint8_t flags);
void   setpalettefade(char r, char g, char b, char offset);
void   squarerotatetile(int16_t tilenume);
void fade_screen_black(int32_t moreopaquep);

int32_t   setgamemode(char davidoption, int32_t daxdim, int32_t daydim, int32_t dabpp);
void   nextpage(void);
void   setaspect_new();
void   setview(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void   setaspect(int32_t daxrange, int32_t daaspect);
void   flushperms(void);

void plotlines2d(const int32_t *xx, const int32_t *yy, int32_t numpoints, char col) ATTRIBUTE((nonnull(1,2)));

void   plotpixel(int32_t x, int32_t y, char col);
char   getpixel(int32_t x, int32_t y);
void   setviewtotile(int16_t tilenume, int32_t xsiz, int32_t ysiz);
void   setviewback(void);
void   preparemirror(int32_t dax, int32_t day, int16_t daang, int16_t dawall,
                     int32_t *tposx, int32_t *tposy, int16_t *tang);
void   completemirror(void);

int32_t   drawrooms(int32_t daposx, int32_t daposy, int32_t daposz,
                    int16_t daang, int32_t dahoriz, int16_t dacursectnum);
void   drawmasks(void);
void   clearview(int32_t dacol);
void   clearallviews(int32_t dacol);
void   drawmapview(int32_t dax, int32_t day, int32_t zoome, int16_t ang);
void   rotatesprite_(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                    int8_t dashade, char dapalnum, int32_t dastat, uint8_t daalpha,
                    int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2);
void   drawline256(int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col);
int32_t    printext16(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol,
                      const char *name, char fontsize) ATTRIBUTE((nonnull(5)));
void   printext256(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol,
                   const char *name, char fontsize) ATTRIBUTE((nonnull(5)));

////////// specialized rotatesprite wrappers for (very) often used cases //////////
static inline void rotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                                int8_t dashade, char dapalnum, int32_t dastat,
                                int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2)
{
    rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, cx1, cy1, cx2, cy2);
}
// Don't clip at all, i.e. the whole screen real estate is available:
static inline void rotatesprite_fs(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                                   int8_t dashade, char dapalnum, int32_t dastat)
{
    rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, 0,0,xdim-1,ydim-1);
}

static inline void rotatesprite_win(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                                    int8_t dashade, char dapalnum, int32_t dastat)
{
    rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, windowx1,windowy1,windowx2,windowy2);
}

void bfirst_search_init(int16_t *list, uint8_t *bitmap, int32_t *eltnumptr, int32_t maxnum, int16_t firstelt);
void bfirst_search_try(int16_t *list, uint8_t *bitmap, int32_t *eltnumptr, int16_t elt);

int32_t   clipmove(vec3_t *vect, int16_t *sectnum, int32_t xvect, int32_t yvect, int32_t walldist,
                   int32_t ceildist, int32_t flordist, uint32_t cliptype) ATTRIBUTE((nonnull(1,2)));
int32_t clipmovex(vec3_t *pos, int16_t *sectnum, int32_t xvect, int32_t yvect,
                  int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype,
                  uint8_t noslidep) ATTRIBUTE((nonnull(1,2)));
int32_t   clipinsidebox(int32_t x, int32_t y, int16_t wallnum, int32_t walldist);
int32_t   clipinsideboxline(int32_t x, int32_t y, int32_t x1, int32_t y1,
                            int32_t x2, int32_t y2, int32_t walldist);
int32_t   pushmove(vec3_t *vect, int16_t *sectnum, int32_t walldist,
                   int32_t ceildist, int32_t flordist, uint32_t cliptype) ATTRIBUTE((nonnull(1,2)));
void   getzrange(const vec3_t *vect, int16_t sectnum, int32_t *ceilz, int32_t *ceilhit, int32_t *florz,
                 int32_t *florhit, int32_t walldist, uint32_t cliptype) ATTRIBUTE((nonnull(1,3,4,5,6)));
int32_t   hitscan(const vec3_t *sv, int16_t sectnum, int32_t vx, int32_t vy, int32_t vz,
                  hitdata_t *hitinfo, uint32_t cliptype) ATTRIBUTE((nonnull(1,6)));
void   neartag(int32_t xs, int32_t ys, int32_t zs, int16_t sectnum, int16_t ange,
               int16_t *neartagsector, int16_t *neartagwall, int16_t *neartagsprite,
               int32_t *neartaghitdist, int32_t neartagrange, uint8_t tagsearch,
               int32_t (*blacklist_sprite_func)(int32_t)) ATTRIBUTE((nonnull(6,7,8)));
int32_t   cansee(int32_t x1, int32_t y1, int32_t z1, int16_t sect1,
                 int32_t x2, int32_t y2, int32_t z2, int16_t sect2);
void   updatesector(int32_t x, int32_t y, int16_t *sectnum) ATTRIBUTE((nonnull(3)));
void updatesectorbreadth(int32_t x, int32_t y, int16_t *sectnum) ATTRIBUTE((nonnull(3)));
void updatesectorexclude(int32_t x, int32_t y, int16_t *sectnum,
                         const uint8_t *excludesectbitmap) ATTRIBUTE((nonnull(3,4)));
void   updatesectorz(int32_t x, int32_t y, int32_t z, int16_t *sectnum) ATTRIBUTE((nonnull(4)));
int32_t   inside(int32_t x, int32_t y, int16_t sectnum);
void   dragpoint(int16_t pointhighlight, int32_t dax, int32_t day, uint8_t flags);
void   setfirstwall(int16_t sectnum, int16_t newfirstwall);

extern const int16_t *chsecptr_onextwall;
int32_t checksectorpointer(int16_t i, int16_t sectnum);

void   getmousevalues(int32_t *mousx, int32_t *mousy, int32_t *bstatus) ATTRIBUTE((nonnull(1,2,3)));
int32_t    krand(void);
int32_t   ksqrt(uint32_t num);
int32_t   __fastcall getangle(int32_t xvect, int32_t yvect);

static inline uint32_t uhypsq(int32_t dx, int32_t dy)
{
    return (uint32_t)dx*dx + (uint32_t)dy*dy;
}

void   rotatepoint(int32_t xpivot, int32_t ypivot, int32_t x, int32_t y,
                   int16_t daang, int32_t *x2, int32_t *y2) ATTRIBUTE((nonnull(6,7)));
int32_t   lastwall(int16_t point);
int32_t   nextsectorneighborz(int16_t sectnum, int32_t thez, int16_t topbottom, int16_t direction);

int32_t   getceilzofslopeptr(const sectortype *sec, int32_t dax, int32_t day) ATTRIBUTE((nonnull(1)));
int32_t   getflorzofslopeptr(const sectortype *sec, int32_t dax, int32_t day) ATTRIBUTE((nonnull(1)));
void   getzsofslopeptr(const sectortype *sec, int32_t dax, int32_t day,
                       int32_t *ceilz, int32_t *florz) ATTRIBUTE((nonnull(1,4,5)));

static inline int32_t getceilzofslope(int16_t sectnum, int32_t dax, int32_t day)
{
    return getceilzofslopeptr(&sector[sectnum], dax, day);
}

static inline int32_t getflorzofslope(int16_t sectnum, int32_t dax, int32_t day)
{
    return getflorzofslopeptr(&sector[sectnum], dax, day);
}

static inline void getzsofslope(int16_t sectnum, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    getzsofslopeptr(&sector[sectnum], dax, day, ceilz, florz);
}

// Is <wal> a red wall in a safe fashion, i.e. only if consistency invariant
// ".nextsector >= 0 iff .nextwall >= 0" holds.
static inline int32_t redwallp(const walltype *wal)
{
    return (wal->nextwall >= 0 && wal->nextsector >= 0);
}

void   alignceilslope(int16_t dasect, int32_t x, int32_t y, int32_t z);
void   alignflorslope(int16_t dasect, int32_t x, int32_t y, int32_t z);
int32_t   sectorofwall(int16_t theline);
int32_t   sectorofwall_noquick(int16_t theline);
int32_t   loopnumofsector(int16_t sectnum, int16_t wallnum);
void setslope(int32_t sectnum, int32_t cf, int16_t slope);

int32_t lineintersect(int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2, int32_t x3,
                      int32_t y3, int32_t x4, int32_t y4, int32_t *intx, int32_t *inty, int32_t *intz);

int32_t rayintersect(int32_t x1, int32_t y1, int32_t z1, int32_t vx, int32_t vy, int32_t vz, int32_t x3,
                     int32_t y3, int32_t x4, int32_t y4, int32_t *intx, int32_t *inty, int32_t *intz);

void do_insertsprite_at_headofstat(int16_t spritenum, int16_t statnum);
int32_t insertspritestat(int16_t statnum);
void do_insertsprite_at_headofsect(int16_t spritenum, int16_t sectnum);
void do_deletespritesect(int16_t deleteme);

int32_t insertsprite(int16_t sectnum, int16_t statnum);
int32_t deletesprite(int16_t spritenum);

int32_t   changespritesect(int16_t spritenum, int16_t newsectnum);
int32_t   changespritestat(int16_t spritenum, int16_t newstatnum);
int32_t   setsprite(int16_t spritenum, const vec3_t *) ATTRIBUTE((nonnull(2)));
int32_t   setspritez(int16_t spritenum, const vec3_t *) ATTRIBUTE((nonnull(2)));

int32_t spriteheightofsptr(const spritetype *spr, int32_t *height, int32_t alsotileyofs);
static inline int32_t spriteheightofs(int16_t i, int32_t *height, int32_t alsotileyofs)
{
    return spriteheightofsptr(&sprite[i], height, alsotileyofs);
}

int32_t   screencapture(const char *filename, char inverseit, const char *versionstr) ATTRIBUTE((nonnull(1)));

int32_t   getclosestcol(int32_t r, int32_t g, int32_t b);

// PLAG: line utility functions
typedef struct  s_equation {
    float       a, b, c;
}               _equation;
typedef struct  s_point2d {
    float       x, y;
}               _point2d;
int32_t             wallvisible(int32_t x, int32_t y, int16_t wallnum);

#define STATUS2DSIZ 144
#define STATUS2DSIZ2 26

//void   qsetmode640350(void);
//void   qsetmode640480(void);
void   qsetmodeany(int32_t,int32_t);
void   clear2dscreen(void);
void   draw2dgrid(int32_t posxe, int32_t posye, int32_t posze, int16_t cursectnum,
                  int16_t ange, int32_t zoome, int16_t gride);
void   draw2dscreen(const vec3_t *pos, int16_t cursectnum,
                    int16_t ange, int32_t zoome, int16_t gride) ATTRIBUTE((nonnull(1)));
int32_t   drawline16(int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col);
void   drawcircle16(int32_t x1, int32_t y1, int32_t r, int32_t eccen, char col);

int32_t   setrendermode(int32_t renderer);

#ifdef USE_OPENGL
void    setrollangle(int32_t rolla);
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
void invalidatetile(int16_t tilenume, int32_t pal, int32_t how);

void setpolymost2dview(void);   // sets up GL for 2D drawing

int32_t polymost_drawtilescreen(int32_t tilex, int32_t tiley, int32_t wallnum, int32_t dimen, int32_t tilezoom,
                                int32_t usehitile, uint8_t *loadedhitile);
void polymost_glreset(void);
void polymost_precache(int32_t dapicnum, int32_t dapalnum, int32_t datype);

#ifdef USE_OPENGL
extern int32_t glanisotropy;
extern int32_t glusetexcompr;
extern int32_t gltexfiltermode;
extern int32_t glredbluemode;
extern int32_t glusetexcache, glusememcache;
extern int32_t glmultisample, glnvmultisamplehint;
extern int32_t glwidescreen, glprojectionhacks;
extern int32_t gltexmaxsize;
void gltexapplyprops (void);
void texcache_invalidate(void);

extern int32_t r_detailmapping;
extern int32_t r_glowmapping;
extern int32_t r_vertexarrays;
extern int32_t r_vbos;
extern int32_t r_vbocount;
extern int32_t r_animsmoothing;
extern int32_t r_parallaxskyclamping;
extern int32_t r_parallaxskypanning;
extern int32_t r_fullbrights;
extern int32_t r_downsize;
extern int32_t r_downsizevar;
extern int32_t mdtims, omdtims;
extern int32_t glrendmode;
#endif

void hicinit(void);
// effect bitset: 1 = greyscale, 2 = invert
void hicsetpalettetint(int32_t palnum, char r, char g, char b, char effect);
// flags bitset: 1 = don't compress
int32_t hicsetsubsttex(int32_t picnum, int32_t palnum, const char *filen, float alphacut,
                       float xscale, float yscale, float specpower, float specfactor, char flags);
int32_t hicsetskybox(int32_t picnum, int32_t palnum, char *faces[6]);
int32_t hicclearsubst(int32_t picnum, int32_t palnum);

int32_t Ptile2tile(int32_t tile, int32_t pallet) ATTRIBUTE((pure));
int32_t md_loadmodel(const char *fn);
int32_t md_setmisc(int32_t modelid, float scale, int32_t shadeoff, float zadd, float yoffset, int32_t flags);
// int32_t md_tilehasmodel(int32_t tilenume, int32_t pal);

extern const char *G_DefaultDefFile(void);
extern const char *G_DefFile(void);
extern char *g_defNamePtr;

extern char **g_defModules;
extern int32_t g_defModulesNum;

#ifdef HAVE_CLIPSHAPE_FEATURE
extern char **g_clipMapFiles;
extern int32_t g_clipMapFilesNum;
#endif

#ifdef USE_OPENGL
typedef struct
{
    // maps build tiles to particular animation frames of a model
    int32_t     modelid;
    int32_t     skinnum;
    int32_t     framenum;   // calculate the number from the name when declaring
    float   smoothduration;
    int32_t     next;
    char    pal;
} tile2model_t;

# define EXTRATILES (MAXTILES/8)

EXTERN int32_t mdinited;
EXTERN tile2model_t tile2model[MAXTILES+EXTRATILES];

static inline int32_t md_tilehasmodel(int32_t tilenume,int32_t pal)
{
    if (!mdinited) return -1;
    return tile2model[Ptile2tile(tilenume,pal)].modelid;
}
#endif  // defined USE_OPENGL

int32_t md_defineframe(int32_t modelid, const char *framename, int32_t tilenume,
                       int32_t skinnum, float smoothduration, int32_t pal);
int32_t md_defineanimation(int32_t modelid, const char *framestart, const char *frameend,
                           int32_t fps, int32_t flags);
int32_t md_defineskin(int32_t modelid, const char *skinfn, int32_t palnum, int32_t skinnum,
                      int32_t surfnum, float param, float specpower, float specfactor);
int32_t md_definehud (int32_t modelid, int32_t tilex, double xadd, double yadd, double zadd,
                      double angadd, int32_t flags, int32_t fov);
int32_t md_undefinetile(int32_t tile);
int32_t md_undefinemodel(int32_t modelid);

int32_t loaddefinitionsfile(const char *fn);

// if loadboard() fails with -2 return, try loadoldboard(). if it fails with
// -2, board is dodgy
int32_t loadoldboard(const char *filename, char fromwhere, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum);


// Hash functions

typedef struct _hashitem // size is 12/24 bits.
{
    char *string;
    int32_t key;
    struct _hashitem *next;
} hashitem_t;

typedef struct
{
    int32_t size;
    hashitem_t **items;
} hashtable_t;

void hash_init(hashtable_t *t);
void hash_free(hashtable_t *t);
int32_t  hash_findcase(const hashtable_t *t, const char *s);
int32_t  hash_find(const hashtable_t *t, const char *s);
void hash_add(hashtable_t *t, const char *s, int32_t key, int32_t replace);
void hash_delete(hashtable_t *t, const char *s);

#ifdef POLYMER
# include "polymer.h"
#else
# ifdef USE_OPENGL
#  include "polymost.h"
# endif
#endif

extern void initialize_engine_globals(void);

static inline void push_nofog(void)
{
#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
    {
        bglPushAttrib(GL_ENABLE_BIT);
        bglDisable(GL_FOG);
    }
#endif
}

static inline void pop_nofog(void)
{
#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
        bglPopAttrib();
#endif
}


#ifdef EXTERNC
}
#endif

#endif // __build_h__
