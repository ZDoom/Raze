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

#include "compat.h"
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
    MAXSECTORS = 4096,
    MAXWALLS = 16384,
    MAXSPRITES = 16384,

    MAXVOXMIPS = 5,

    MAXWALLSB = ((MAXWALLS >> 2) + (MAXWALLS >> 3)),

    MAXVOXELS = 1024,
    MAXSTATUS = 1024,
    // Maximum number of component tiles in a multi-psky:
    MAXPSKYTILES = 16,
    MAXSPRITESONSCREEN = MAXSPRITES >> 2,
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


#include "buildtypes.h"

using uspriteptr_t = spritetype const *;
using uwallptr_t   = walltype const *;
using usectorptr_t = sectortype const *;
using tspriteptr_t = tspritetype *;



#include "clip.h"

int32_t getwalldist(vec2_t const in, int const wallnum);
int32_t getwalldist(vec2_t const in, int const wallnum, vec2_t * const out);

typedef struct {
    uint32_t mdanimtims;
    int16_t mdanimcur;
    int16_t angoff, pitch, roll;
    vec3_t pivot_offset, position_offset;
    uint8_t flags;
    float alpha;
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

// using the clipdist field
enum
{
    TSPR_FLAGS_MDHACK = 1u<<0u,
    TSPR_FLAGS_DRAW_LAST = 1u<<1u,
};

EXTERN int32_t guniqhudid;

struct usermaphack_t 
{
    FString mhkfile;
    FString title;
    uint8_t md4[16]{};
};

extern spriteext_t spriteext[MAXSPRITES];
extern spritesmooth_t spritesmooth[MAXSPRITES + MAXUNIQHUDID];

extern sectortype sector[MAXSECTORS];
extern walltype wall[MAXWALLS];
extern spritetype sprite[MAXSPRITES];
EXTERN int leveltimer;

inline sectortype* spritetype::sector() const
{
    return &::sector[sectnum];
}

inline sectortype* walltype::nextSector() const
{
    return &::sector[nextsector];
}

inline walltype* walltype::nextWall() const
{
    return &::wall[nextwall];
}

inline walltype* walltype::point2Wall() const
{
    return &::wall[point2];
}

inline walltype* sectortype::firstWall() const
{
    return &wall[wallptr];
}


extern sectortype sectorbackup[MAXSECTORS];
extern walltype wallbackup[MAXWALLS];


inline tspriteptr_t renderAddTSpriteFromSprite(spritetype* tsprite, int& spritesortcnt, uint16_t const spritenum)
{
    auto tspr = &tsprite[spritesortcnt++];
    auto const spr = &sprite[spritenum];
    *tspr = *spr;
    tspr->clipdist = 0;
    tspr->owner = spritenum;
    return tspr;
}

// returns: 0=continue sprite collecting;
//          1=break out of sprite collecting;
inline int32_t renderAddTsprite(spritetype* tsprite, int& spritesortcnt, int16_t z, int16_t sectnum)
{
    if (spritesortcnt >= MAXSPRITESONSCREEN) return 1;
    renderAddTSpriteFromSprite(tsprite, spritesortcnt, z);
    return 0;
}



EXTERN int32_t xdim, ydim;
EXTERN int32_t yxaspect, viewingrange;

EXTERN int32_t Numsprites;
EXTERN int16_t numsectors, numwalls;
EXTERN int32_t display_mirror;

inline bool validSectorIndex(int sectnum)
{
	return sectnum >= 0 && sectnum < numsectors;
}

inline bool validWallIndex(int wallnum)
{
	return wallnum >= 0 && wallnum < numwalls;
}

EXTERN int32_t randomseed;

EXTERN uint8_t paletteloaded;

enum {
    PALETTE_MAIN = 1<<0,
    PALETTE_SHADE = 1<<1,
    PALETTE_TRANSLUC = 1<<2,
};

EXTERN int32_t g_visibility;

EXTERN vec2_t windowxy1, windowxy2;

// The maximum tile offset ever used in any tiled parallaxed multi-sky.
#define PSKYOFF_MAX 16
#define DEFAULTPSKY -1

typedef struct {
    int tilenum;
    // The proportion at which looking up/down affects the apparent 'horiz' of
    // a parallaxed sky, scaled by 65536 (so, a value of 65536 makes it align
    // with the drawn surrounding scene):
    int horizfrac;

    // The texel index offset in the y direction of a parallaxed sky:
    // XXX: currently always 0.
    int yoffs;
    int yoffs2;

    int lognumtiles;  // 1<<lognumtiles: number of tiles in multi-sky
    int16_t tileofs[MAXPSKYTILES];  // for 0 <= j < (1<<lognumtiles): tile offset relative to basetile

    int32_t yscale;
} psky_t;

// Index of map-global (legacy) multi-sky:
// New multi-psky
EXTERN TArray<psky_t> multipskies;

static inline psky_t *getpskyidx(int32_t picnum)
{
    for (auto& sky : multipskies)
        if (picnum == sky.tilenum) return &sky;

    return &multipskies[0];
}


EXTERN psky_t * tileSetupSky(int32_t tilenum);
psky_t* defineSky(int32_t const tilenum, int horiz, int lognumtiles, const uint16_t* tileofs, int yoff = 0, int yoff2 = 0x7fffffff);

// Get properties of parallaxed sky to draw.
// Returns: pointer to tile offset array. Sets-by-pointer the other three.
const int16_t* getpsky(int32_t picnum, int32_t* dapyscale, int32_t* dapskybits, int32_t* dapyoffs, int32_t* daptileyscale, bool alt = false);


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

EXTERN uint8_t gotpic[(MAXTILES+7)>>3];
extern FixedBitArray<MAXSECTORS> gotsector;


extern uint32_t drawlinepat;

extern uint8_t globalr, globalg, globalb;

enum {
    GLOBAL_NO_GL_TILESHADES = 1<<0,
    GLOBAL_NO_GL_FULLBRIGHT = 1<<1,
    GLOBAL_NO_GL_FOGSHADE = 1<<2,
};

extern const char *engineerrstr;

EXTERN int32_t editorzrange[2];

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


OTHER VARIABLES:

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

int32_t    engineInit(void);
void   engineUnInit(void);
void   initspritelists(void);

void ValidateSprite(spritetype& spr);
void engineLoadBoard(const char *filename, int flags, vec3_t *dapos, int16_t *daang, int *dacursectnum);
void loadMapBackup(const char* filename);
void G_LoadMapHack(const char* filename, const unsigned char*);

void   videoSetCorrectedAspect();
void   videoSetViewableArea(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void   renderSetAspect(int32_t daxrange, int32_t daaspect);

FCanvasTexture *renderSetTarget(int16_t tilenume);
void   renderRestoreTarget();

void setVideoMode();

class F2DDrawer;


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
int32_t   inside(int32_t x, int32_t y, int sectnum);
void   dragpoint(int pointhighlight, int32_t dax, int32_t day, uint8_t flags = 0);
int32_t try_facespr_intersect(uspriteptr_t const spr, vec3_t const in,
                                     int32_t vx, int32_t vy, int32_t vz,
                                     vec3_t * const intp, int32_t strictly_smaller_than_p);

#define MAXUPDATESECTORDIST 1536
#define INITIALUPDATESECTORDIST 256
void updatesector(int const x, int const y, int * const sectnum) ATTRIBUTE((nonnull(3)));
void updatesectorz(int32_t const x, int32_t const y, int32_t const z, int * const sectnum) ATTRIBUTE((nonnull(4)));

void updatesectorneighbor(int32_t const x, int32_t const y, int * const sectnum, int32_t initialMaxDistance = INITIALUPDATESECTORDIST, int32_t maxDistance = MAXUPDATESECTORDIST) ATTRIBUTE((nonnull(3)));
void updatesectorneighborz(int32_t const x, int32_t const y, int32_t const z, int * const sectnum, int32_t initialMaxDistance = INITIALUPDATESECTORDIST, int32_t maxDistance = MAXUPDATESECTORDIST) ATTRIBUTE((nonnull(4)));

int findwallbetweensectors(int sect1, int sect2);
inline int sectoradjacent(int sect1, int sect2) { return findwallbetweensectors(sect1, sect2) != -1; }
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

inline constexpr uint32_t uhypsq(int32_t const dx, int32_t const dy)
{
    return (uint32_t)dx*dx + (uint32_t)dy*dy;
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
inline sectortype* nextsectorneighborzptr(int16_t sectnum, int32_t refz, int16_t topbottom, int16_t direction)
{
	auto sect = nextsectorneighborz(sectnum, refz, topbottom, direction);
	return sect == -1? nullptr : &sector[sect];
}

inline sectortype* nextsectorneighborzptr(sectortype* sectp, int32_t refz, int16_t topbottom, int16_t direction)
{
	auto sect = nextsectorneighborz(int(sectp - sector), refz, topbottom, direction);
	return sect == -1? nullptr : &sector[sect];
}

int32_t   getceilzofslopeptr(usectorptr_t sec, int32_t dax, int32_t day) ATTRIBUTE((nonnull(1)));
int32_t   getflorzofslopeptr(usectorptr_t sec, int32_t dax, int32_t day) ATTRIBUTE((nonnull(1)));
void   getzsofslopeptr(usectorptr_t sec, int32_t dax, int32_t day,
                       int32_t *ceilz, int32_t *florz) ATTRIBUTE((nonnull(1,4,5)));
void yax_getzsofslope(int sectNum, int playerX, int playerY, int32_t* pCeilZ, int32_t* pFloorZ);

int32_t yax_getceilzofslope(int const sectnum, vec2_t const vect);
int32_t yax_getflorzofslope(int const sectnum, vec2_t const vect);

inline int32_t getceilzofslope(int sectnum, int32_t dax, int32_t day)
{
    return getceilzofslopeptr((usectorptr_t)&sector[sectnum], dax, day);
}

inline int32_t getflorzofslope(int sectnum, int32_t dax, int32_t day)
{
    return getflorzofslopeptr((usectorptr_t)&sector[sectnum], dax, day);
}

inline void getzsofslope(int sectnum, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    getzsofslopeptr((usectorptr_t)&sector[sectnum], dax, day, ceilz, florz);
}

inline void getcorrectzsofslope(int sectnum, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    vec2_t closest = { dax, day };
    getsectordist(closest, sectnum, &closest);
    getzsofslopeptr((usectorptr_t)&sector[sectnum], closest.x, closest.y, ceilz, florz);
}

inline int32_t getcorrectceilzofslope(int sectnum, int32_t dax, int32_t day)
{
    vec2_t closest = { dax, day };
    getsectordist(closest, sectnum, &closest);
    return getceilzofslopeptr((usectorptr_t)&sector[sectnum], closest.x, closest.y);
}

inline int32_t getcorrectflorzofslope(int sectnum, int32_t dax, int32_t day)
{
    vec2_t closest = { dax, day };
    getsectordist(closest, sectnum, &closest);
    return getflorzofslopeptr((usectorptr_t)&sector[sectnum], closest.x, closest.y);
}

// Is <wal> a red wall in a safe fashion, i.e. only if consistency invariant
// ".nextsector >= 0 iff .nextwall >= 0" holds.
inline int32_t redwallp(uwallptr_t wal)
{
    return (wal->nextwall >= 0 && wal->nextsector >= 0);
}

inline int32_t E_SpriteIsValid(const int32_t i)
{
    return ((unsigned)i < MAXSPRITES && sprite[i].statnum != MAXSTATUS);
}


void   alignceilslope(int16_t dasect, int32_t x, int32_t y, int32_t z);
void   alignflorslope(int16_t dasect, int32_t x, int32_t y, int32_t z);
void setslope(int32_t sectnum, int32_t cf, int16_t slope);

int32_t lintersect(int32_t originX, int32_t originY, int32_t originZ,
                   int32_t destX, int32_t destY, int32_t destZ,
                   int32_t lineStartX, int32_t lineStartY, int32_t lineEndX, int32_t lineEndY,
                   int32_t *intersectionX, int32_t *intersectionY, int32_t *intersectionZ);

int32_t rayintersect(int32_t x1, int32_t y1, int32_t z1, int32_t vx, int32_t vy, int32_t vz, int32_t x3,
                     int32_t y3, int32_t x4, int32_t y4, int32_t *intx, int32_t *inty, int32_t *intz);
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

int32_t   setspritez(int16_t spritenum, const vec3_t *) ATTRIBUTE((nonnull(2)));

int32_t spriteheightofsptr(uspriteptr_t spr, int32_t *height, int32_t alsotileyofs);
inline int32_t spriteheightofs(int16_t i, int32_t *height, int32_t alsotileyofs)
{
    return spriteheightofsptr((uspriteptr_t)&sprite[i], height, alsotileyofs);
}

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

typedef struct { vec3f_t add; int16_t angadd, flags, fov; } hudtyp;

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
int32_t md_definehud (int32_t modelid, int32_t tilex, vec3f_t add,
                      int32_t angadd, int32_t flags, int32_t fov);
int32_t md_undefinetile(int32_t tile);
int32_t md_undefinemodel(int32_t modelid);

#ifdef USE_OPENGL
# include "polymost.h"
#endif

extern int skiptile;

static vec2_t const zerovec = { 0, 0 };

inline int inside_p(int32_t const x, int32_t const y, int const sectnum) { return (sectnum >= 0 && inside(x, y, sectnum) == 1); }

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

static inline int32_t setspritez_old(int16_t spritenum, int32_t x, int32_t y, int32_t z)
{
    const vec3_t vector = { x, y, z };
    return setspritez(spritenum, &vector);
}

extern int32_t rintersect(int32_t x1, int32_t y1, int32_t z1,
    int32_t vx_, int32_t vy_, int32_t vz,
    int32_t x3, int32_t y3, int32_t x4, int32_t y4,
    int32_t *intx, int32_t *inty, int32_t *intz);

extern int32_t(*animateoffs_replace)(int const tilenum, int fakevar);
extern void(*initspritelists_replace)(void);
extern int32_t(*insertsprite_replace)(int16_t sectnum, int16_t statnum);
extern int32_t(*deletesprite_replace)(int16_t spritenum);
extern int32_t(*changespritesect_replace)(int16_t spritenum, int16_t newsectnum);
extern int32_t(*changespritestat_replace)(int16_t spritenum, int16_t newstatnum);

// Masking these into the object index to keep it in 16 bit was probably the single most dumbest and pointless thing Build ever did.
// Gonna be fun to globally replace these to finally lift the limit this imposes on map size.
// Names taken from DukeGDX
enum EHitBits
{
    kHitNone = 0,
    kHitTypeMask = 0xC000,
    kHitIndexMask = 0x3FFF,
    kHitSector = 0x4000,
    kHitWall = 0x8000,
    kHitSprite = 0xC000,
};

void updateModelInterpolation();

inline void tileUpdatePicnum(int* const tileptr, int const obj, int stat)
{
    auto& tile = *tileptr;

    if (picanm[tile].sf & PICANM_ANIMTYPE_MASK)
        tile += animateoffs(tile, obj);

    if (((obj & 16384) == 16384) && (stat & CSTAT_WALL_ROTATE_90) && RotTile(tile).newtile != -1)
        tile = RotTile(tile).newtile;
}

inline void setgotpic(int32_t tilenume)
{
    gotpic[tilenume >> 3] |= 1 << (tilenume & 7);
}

inline void cleargotpic(int32_t tilenume)
{
    gotpic[tilenume >> 3] &= ~(1 << (tilenume & 7));
}

inline bool testgotpic(int32_t tilenume, bool reset = false)
{
    bool res = gotpic[tilenume >> 3] & (1 << (tilenume & 7));
    if (reset) gotpic[tilenume >> 3] &= ~(1 << (tilenume & 7));
    return res;
}


#include "iterators.h"

#endif // build_h_
