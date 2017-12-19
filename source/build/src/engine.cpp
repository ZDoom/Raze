// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)

#define ENGINE

#include "compat.h"
#include "build.h"
#include "editor.h"
#include "pragmas.h"
#include "cache1d.h"
#include "a.h"
#include "osd.h"
#include "crc32.h"
#include "lz4.h"
#include "colmatch.h"

#include "baselayer.h"
#include "scriptfile.h"

#ifdef USE_OPENGL
# include "glbuild.h"
# include "mdsprite.h"
# ifdef POLYMER
#  include "polymer.h"
# endif
# include "hightile.h"
# include "polymost.h"
#endif

#include "engine_priv.h"
#include "palette.h"

#ifdef LUNATIC
# include "lunatic.h"
L_State g_engState;
#endif

#define CACHEAGETIME 16

//////////
// Compilation switches for optional/extended engine features

#if !defined(__arm__) && !defined(GEKKO)
# define HIGH_PRECISION_SPRITE
#endif

#if !defined EDUKE32_TOUCH_DEVICES && !defined GEKKO && !defined __OPENDINGUX__
// Handle absolute z difference of floor/ceiling to camera >= 1<<24.
// Also: higher precision view-relative x and y for drawvox().
# define CLASSIC_Z_DIFF_64
#endif

#define MULTI_COLUMN_VLINE
//#define DEBUG_TILESIZY_512
//#define DEBUG_TILEOFFSETS
//////////

#ifdef LUNATIC
# if !defined DEBUG_MAIN_ARRAYS
LUNATIC_EXTERN const int32_t engine_main_arrays_are_static = 0;  // for Lunatic
# else
LUNATIC_EXTERN const int32_t engine_main_arrays_are_static = 1;
# endif

#if MAXSECTORS==MAXSECTORSV8
LUNATIC_EXTERN const int32_t engine_v8 = 1;
#else
LUNATIC_EXTERN const int32_t engine_v8 = 0;
#endif
#endif

#ifdef DEBUGGINGAIDS
float debug1, debug2;
#endif

int32_t mapversion=7; // JBF 20040211: default mapversion to 7
int32_t g_loadedMapVersion = -1;  // -1: none (e.g. started new)

static int32_t get_mapversion(void);

// Handle nonpow2-ysize walls the old way?
static inline int32_t oldnonpow2(void)
{
#if !defined CLASSIC_NONPOW2_YSIZE_WALLS
    return 1;
#else
    return (g_loadedMapVersion < 10);
#endif
}

int16_t pskybits_override = -1;

//void loadvoxel(int32_t voxindex) { UNREFERENCED_PARAMATER(voxindex); }
int16_t tiletovox[MAXTILES];
int32_t usevoxels = 1;
#ifdef USE_OPENGL
static char *voxfilenames[MAXVOXELS], g_haveVoxels=0;  // for deferred voxel->model conversion
#endif
//#define kloadvoxel loadvoxel

int32_t novoxmips = 1;

//These variables need to be copied into BUILD
#define MAXXSIZ 256
#define MAXYSIZ 256
#define MAXZSIZ 255
#define MAXVOXMIPS 5
#ifdef EDUKE32_TOUCH_DEVICES
# define DISTRECIPSIZ (65536+256)
#else
# define DISTRECIPSIZ 131072
#endif
intptr_t voxoff[MAXVOXELS][MAXVOXMIPS]; // used in KenBuild
static char voxlock[MAXVOXELS][MAXVOXMIPS];
int32_t voxscale[MAXVOXELS];

static int32_t ggxinc[MAXXSIZ+1], ggyinc[MAXXSIZ+1];
static int32_t lowrecip[1024], nytooclose;
static const int32_t nytoofar = DISTRECIPSIZ*16384ull - 1048576;
static uint32_t *distrecip;

static int32_t *lookups = NULL;
static int32_t beforedrawrooms = 1;

static int32_t oxdimen = -1, oviewingrange = -1, oxyaspect = -1;

// r_usenewaspect is the cvar, newaspect_enable to trigger the new behaviour in the code
int32_t r_usenewaspect = 1, newaspect_enable=0;
uint32_t r_screenxy = 0;

int32_t globalflags;

float vid_gamma = DEFAULT_GAMMA;
float vid_contrast = DEFAULT_CONTRAST;
float vid_brightness = DEFAULT_BRIGHTNESS;

//Textured Map variables
static char globalpolytype;
static int16_t **dotp1, **dotp2;

static int8_t tempbuf[MAXWALLS];

// referenced from asm
#if !defined(NOASM) && defined __cplusplus
extern "C" {
#endif
int32_t ebpbak, espbak;
int32_t reciptable[2048], fpuasm;
intptr_t asm1, asm2, asm3, asm4, palookupoffse[4];
uint32_t vplce[4];
int32_t vince[4];
intptr_t bufplce[4];
int32_t globaltilesizy;
int32_t globalx1, globaly2, globalx3, globaly3;
#if !defined(NOASM) && defined __cplusplus
}
#endif

int32_t sloptable[16384];
static intptr_t slopalookup[16384];    // was 2048

static int32_t lastageclock;
static int32_t no_radarang2 = 0;
static int16_t radarang[1280], *radarang2;

uint16_t ATTRIBUTE((used)) sqrtable[4096], ATTRIBUTE((used)) shlookup[4096+256];
const char pow2char[8] = {1,2,4,8,16,32,64,128};
const int32_t pow2long[32] =
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

char britable[16][256]; // JBF 20040207: full 8bit precision

extern char textfont[2048], smalltextfont[2048];

static char kensmessage[128];
const char *engineerrstr = "No error";

int32_t showfirstwall=0;
int32_t showheightindicators=1;
int32_t circlewall=-1;

// forward refs
#ifdef __cplusplus
extern "C" {
#endif
void setup_sideview_sincos(void);
int32_t getscreenvdisp(int32_t bz, int32_t zoome);
void screencoords(int32_t *xres, int32_t *yres, int32_t x, int32_t y, int32_t zoome);
int32_t scalescreeny(int32_t sy);
#ifdef YAX_ENABLE
void yax_tweakpicnums(int32_t bunchnum, int32_t cf, int32_t restore);
#endif
int32_t getinvdisplacement(int32_t *dx, int32_t *dy, int32_t dz);
#ifdef __cplusplus
}
#endif

static void scansector(int16_t startsectnum);
static void draw_rainbow_background(void);

int16_t editstatus = 0;
static int32_t global100horiz;  // (-100..300)-scale horiz (the one passed to drawrooms)


////////// YAX //////////

int32_t numgraysects = 0;
uint8_t graysectbitmap[MAXSECTORS>>3];
uint8_t graywallbitmap[MAXWALLS>>3];
int32_t autogray = 0, showinnergray = 1;

//#define YAX_DEBUG_YMOSTS

#ifdef YAX_DEBUG
// XXX: This could be replaced with the use of gethiticks().
double u64tickspersec;
#endif
#ifdef ENGINE_SCREENSHOT_DEBUG
int32_t engine_screenshot = 0;
#endif

int32_t get_alwaysshowgray(void)
{
    return showinnergray || !(editorzrange[0]==INT32_MIN && editorzrange[1]==INT32_MAX);
}

void yax_updategrays(int32_t posze)
{
    int32_t i, j;
#ifdef YAX_ENABLE
    int32_t mingoodz=INT32_MAX, maxgoodz=INT32_MIN;
#else
    UNREFERENCED_PARAMETER(posze);
#endif

    Bmemset(graysectbitmap, 0, sizeof(graysectbitmap));
    Bmemset(graywallbitmap, 0, sizeof(graywallbitmap));

    for (i=0; i<numsectors; i++)
    {
#ifdef YAX_ENABLE
        int16_t cb, fb;
        yax_getbunches(i, &cb, &fb);

        // Update grayouts due to TROR, has to be --v--       half-open      --v--
        // because only one level should ever be    v                          v
        // active.                                  v                          v
        int32_t keep = ((cb<0 || sector[i].ceilingz < posze) && (fb<0 || posze <= sector[i].floorz));
        if (autogray && (cb>=0 || fb>=0) && (sector[i].ceilingz <= posze && posze <= sector[i].floorz))
        {
            mingoodz = min(mingoodz, sector[i].ceilingz);
            maxgoodz = max(maxgoodz, sector[i].floorz);
        }
#endif
        // update grayouts due to editorzrange
        keep &= (sector[i].ceilingz >= editorzrange[0] && sector[i].floorz <= editorzrange[1]);

        if (!keep)  // outside bounds, gray out!
            graysectbitmap[i>>3] |= (1<<(i&7));
    }

#ifdef YAX_ENABLE
    if (autogray && mingoodz<=maxgoodz)
    {
        for (i=0; i<numsectors; i++)
            if (!(mingoodz <= sector[i].ceilingz && sector[i].floorz <= maxgoodz))
                graysectbitmap[i>>3] |= (1<<(i&7));
    }
#endif

    numgraysects = 0;
    for (i=0; i<numsectors; i++)
    {
        if (graysectbitmap[i>>3]&(1<<(i&7)))
        {
            numgraysects++;
            for (j=sector[i].wallptr; j<sector[i].wallptr+sector[i].wallnum; j++)
                graywallbitmap[j>>3] |= (1<<(j&7));
        }
    }
}


#if !defined YAX_ENABLE
# warning Non-TROR builds are supported only for debugging. Expect savegame breakage etc...
#endif

#ifdef YAX_ENABLE
// all references to floor/ceiling bunchnums should be through the
// get/set functions!

int32_t g_nodraw = 0;
int32_t scansector_retfast = 0;
static int32_t scansector_collectsprites = 1;
int32_t yax_globalcf = -1, yax_nomaskpass=0, yax_nomaskdidit;  // engine internal
int32_t r_tror_nomaskpass = 1;  // cvar
int32_t yax_globallev = YAX_MAXDRAWS;
int32_t yax_globalbunch = -1;

// duplicated tsprites
//  [i]:
//   i==MAXDRAWS: base level
//   i<MAXDRAWS: MAXDRAWS-i-1 is level towards ceiling
//   i>MAXDRAWS: i-MAXDRAWS-1 is level towards floor
static int16_t yax_spritesortcnt[1 + 2*YAX_MAXDRAWS];
static uint16_t yax_tsprite[1 + 2*YAX_MAXDRAWS][MAXSPRITESONSCREEN];
static uint8_t yax_tsprfrombunch[1 + 2*YAX_MAXDRAWS][MAXSPRITESONSCREEN];

// drawn sectors
uint8_t yax_gotsector[MAXSECTORS>>3];  // engine internal

# if !defined NEW_MAP_FORMAT
// Game-time YAX data structures, V7-V9 map formats.
int16_t yax_bunchnum[MAXSECTORS][2];
int16_t yax_nextwall[MAXWALLS][2];

static FORCE_INLINE int32_t yax_islockededge(int32_t line, int32_t cf)
{
    return !!(wall[line].cstat&(YAX_NEXTWALLBIT(cf)));
}

#define YAX_PTRBUNCHNUM(Ptr, Sect, Cf) (*(&Ptr[Sect].ceilingxpanning + 8*Cf))
#define YAX_BUNCHNUM(Sect, Cf) YAX_PTRBUNCHNUM(sector, Sect, Cf)

//// bunch getters/setters
int16_t yax_getbunch(int16_t i, int16_t cf)
{
    if (editstatus==0)
        return yax_bunchnum[i][cf];

    return (*(&sector[i].ceilingstat + cf) & YAX_BIT) ? YAX_BUNCHNUM(i, cf) : -1;
}
# else
#  define YAX_PTRBUNCHNUM(Ptr, Sect, Cf) (*((Cf) ? &(Ptr)[Sect].floorbunch : &(Ptr)[Sect].ceilingbunch))
#  define YAX_BUNCHNUM(Sect, Cf) YAX_PTRBUNCHNUM(sector, Sect, Cf)

#  if !defined NEW_MAP_FORMAT
static FORCE_INLINE int32_t yax_islockededge(int32_t line, int32_t cf)
{
    return (yax_getnextwall(line, cf) >= 0);
}
#  endif
# endif

// bunchnum: -1: also clear yax-nextwalls (forward and reverse)
//           -2: don't clear reverse yax-nextwalls
//           -3: don't clear either forward or reverse yax-nextwalls
void yax_setbunch(int16_t i, int16_t cf, int16_t bunchnum)
{
    if (editstatus==0)
    {
#ifdef NEW_MAP_FORMAT
        YAX_BUNCHNUM(i, cf) = bunchnum;
#else
        yax_bunchnum[i][cf] = bunchnum;
#endif
        return;
    }

    if (bunchnum < 0)
    {
        if (bunchnum > -3)
        {
            // TODO: for in-game too?
            for (bssize_t ynw, j=sector[i].wallptr; j<sector[i].wallptr+sector[i].wallnum; j++)
            {
                ynw = yax_getnextwall(j, cf);
                if (ynw >= 0)
                {
                    if (bunchnum > -2)
                        yax_setnextwall(ynw, !cf, -1);
                    yax_setnextwall(j, cf, -1);
                }
            }
        }

#if !defined NEW_MAP_FORMAT
        *(&sector[i].ceilingstat + cf) &= ~YAX_BIT;
        // NOTE: Don't reset xpanning-as-index, since we can be called from
        // e.g. Mapster32's "Inner loop made into new sector" functionality.
//        YAX_BUNCHNUM(i, cf) = 0;
#else
        YAX_BUNCHNUM(i, cf) = -1;
#endif
        return;
    }

#if !defined NEW_MAP_FORMAT
    *(&sector[i].ceilingstat + cf) |= YAX_BIT;
#endif
    YAX_BUNCHNUM(i, cf) = bunchnum;
}

void yax_setbunches(int16_t i, int16_t cb, int16_t fb)
{
    yax_setbunch(i, YAX_CEILING, cb);
    yax_setbunch(i, YAX_FLOOR, fb);
}

# if !defined NEW_MAP_FORMAT
//// nextwall getters/setters
int16_t yax_getnextwall(int16_t wal, int16_t cf)
{
    if (editstatus==0)
        return yax_nextwall[wal][cf];

    return yax_islockededge(wal, cf) ? YAX_NEXTWALL(wal, cf) : -1;
}

// unchecked!
void yax_setnextwall(int16_t wal, int16_t cf, int16_t thenextwall)
{
    if (editstatus==0)
    {
        yax_nextwall[wal][cf] = thenextwall;
        return;
    }

    if (thenextwall >= 0)
    {
        wall[wal].cstat |= YAX_NEXTWALLBIT(cf);
        YAX_NEXTWALL(wal, cf) = thenextwall;
    }
    else
    {
        wall[wal].cstat &= ~YAX_NEXTWALLBIT(cf);
        YAX_NEXTWALL(wal, cf) = YAX_NEXTWALLDEFAULT(cf);
    }
}
# endif

// make one step in the vertical direction, and if the wall we arrive at
// is red, return its nextsector.
int16_t yax_vnextsec(int16_t line, int16_t cf)
{
    int16_t const ynw = yax_getnextwall(line, cf);
    return (ynw < 0) ? -1 : wall[ynw].nextsector;
}


//// in-struct --> array transfer (only resetstat==0); list construction
// resetstat:  0: reset and read data from structs and construct linked lists etc.
//             1: only reset
//             2: read data from game-time arrays and construct linked lists etc.
void yax_update(int32_t resetstat)
{
    int32_t i;
#if !defined NEW_MAP_FORMAT
    int32_t j;
    const int32_t oeditstatus=editstatus;
#endif
    int16_t cb, fb;

    if (resetstat != 2)
        numyaxbunches = 0;

    for (i=0; i<MAXSECTORS; i++)
    {
#if !defined NEW_MAP_FORMAT
        if (resetstat != 2 || i>=numsectors)
            yax_bunchnum[i][0] = yax_bunchnum[i][1] = -1;
#endif
        nextsectbunch[0][i] = nextsectbunch[1][i] = -1;
    }
    for (i=0; i<YAX_MAXBUNCHES; i++)
        headsectbunch[0][i] = headsectbunch[1][i] = -1;
#if !defined NEW_MAP_FORMAT
    for (i=0; i<MAXWALLS; i++)
        if (resetstat != 2 || i>=numwalls)
            yax_nextwall[i][0] = yax_nextwall[i][1] = -1;
#endif

    if (resetstat==1)
        return;

    // Constuct singly linked list of sectors-of-bunch.

#if !defined NEW_MAP_FORMAT
    // Read bunchnums directly from the sector struct in yax_[gs]etbunch{es}!
    editstatus = (resetstat==0);
    // NOTE: Use oeditstatus to check for in-gamedness from here on!
#endif

    if (resetstat==0)
    {
        // make bunchnums consecutive
        uint8_t *const havebunch = (uint8_t *)tempbuf;
        uint8_t *const bunchmap = havebunch + (YAX_MAXBUNCHES>>3);
        int32_t dasub = 0;

        Bmemset(havebunch, 0, YAX_MAXBUNCHES>>3);
        for (i=0; i<numsectors; i++)
        {
            yax_getbunches(i, &cb, &fb);
            if (cb>=0)
                havebunch[cb>>3] |= (1<<(cb&7));
            if (fb>=0)
                havebunch[fb>>3] |= (1<<(fb&7));
        }

        for (i=0; i<YAX_MAXBUNCHES; i++)
        {
            if ((havebunch[i>>3]&(1<<(i&7)))==0)
            {
                bunchmap[i] = 255;
                dasub++;
                continue;
            }

            bunchmap[i] = i-dasub;
        }

        for (i=0; i<numsectors; i++)
        {
            yax_getbunches(i, &cb, &fb);
            if (cb>=0)
                yax_setbunch(i, YAX_CEILING, bunchmap[cb]);
            if (fb>=0)
                yax_setbunch(i, YAX_FLOOR, bunchmap[fb]);
        }
    }

    // In-struct --> array transfer (resetstat==0 and !defined NEW_MAP_FORMAT)
    // and list construction.
    for (i=numsectors-1; i>=0; i--)
    {
        yax_getbunches(i, &cb, &fb);
#if !defined NEW_MAP_FORMAT
        if (resetstat==0)
        {
            yax_bunchnum[i][0] = cb;
            yax_bunchnum[i][1] = fb;
        }
#endif

        if (cb >= 0)
        {
#if !defined NEW_MAP_FORMAT
            if (resetstat==0)
                for (j=sector[i].wallptr; j<sector[i].wallptr+sector[i].wallnum; j++)
                {
                    if (yax_islockededge(j,YAX_CEILING))
                    {
                        yax_nextwall[j][0] = YAX_NEXTWALL(j,0);
                        if (oeditstatus==0)
                            YAX_NEXTWALL(j,0) = 0;  // reset lotag!
                    }
                }
#endif
            if (headsectbunch[0][cb] == -1)
            {
                headsectbunch[0][cb] = i;
                // not duplicated in floors, since every extended ceiling
                // must have a corresponding floor:
                if (resetstat==0)
                    numyaxbunches++;
            }
            else
            {
                int32_t tmpsect = headsectbunch[0][cb];
                headsectbunch[0][cb] = i;
                nextsectbunch[0][i] = tmpsect;
            }
        }

        if (fb >= 0)
        {
#if !defined NEW_MAP_FORMAT
            if (resetstat==0)
                for (j=sector[i].wallptr; j<sector[i].wallptr+sector[i].wallnum; j++)
                {
                    if (yax_islockededge(j,YAX_FLOOR))
                    {
                        yax_nextwall[j][1] = YAX_NEXTWALL(j,1);
                        if (oeditstatus==0)
                            YAX_NEXTWALL(j,1) = -1;  // reset extra!
                    }
                }
#endif
            if (headsectbunch[1][fb] == -1)
                headsectbunch[1][fb] = i;
            else
            {
                int32_t tmpsect = headsectbunch[1][fb];
                headsectbunch[1][fb] = i;
                nextsectbunch[1][i] = tmpsect;
            }
        }
    }

#if !defined NEW_MAP_FORMAT
    editstatus = oeditstatus;
#else
    mapversion = get_mapversion();
#endif
}

int32_t yax_getneighborsect(int32_t x, int32_t y, int32_t sectnum, int32_t cf)
{
    int16_t bunchnum = yax_getbunch(sectnum, cf);

    if (bunchnum < 0)
        return -1;

    for (bssize_t SECTORS_OF_BUNCH(bunchnum, !cf, i))
        if (inside(x, y, i)==1)
            return i;

    return -1;
}

// indexed as a list:
static int16_t bunches[2][YAX_MAXBUNCHES];
// indexed with bunchnums directly:
static int16_t bunchsec[YAX_MAXBUNCHES], bunchdist[YAX_MAXBUNCHES];

static int32_t ymostallocsize = 0;  // numyaxbunches*xdimen (no sizeof(int16_t) here!)
static int16_t *yumost=NULL, *ydmost=NULL;  // used as if [numyaxbunches][xdimen]
uint8_t haveymost[YAX_MAXBUNCHES>>3];

// adapted from build.c
static void yax_getclosestpointonwall(int32_t dawall, int32_t *closestx, int32_t *closesty)
{
    int64_t i, j, wx,wy, wx2,wy2, dx, dy;

    wx = wall[dawall].x;
    wy = wall[dawall].y;
    wx2 = wall[wall[dawall].point2].x;
    wy2 = wall[wall[dawall].point2].y;

    dx = wx2 - wx;
    dy = wy2 - wy;
    i = dx*(globalposx-wx) + dy*(globalposy-wy);
    if (i <= 0) { *closestx = wx; *closesty = wy; return; }
    j = dx*dx + dy*dy;
    if (i >= j) { *closestx = wx2; *closesty = wy2; return; }
    i=((i<<15)/j)<<15;
    *closestx = wx + ((dx*i)>>30);
    *closesty = wy + ((dy*i)>>30);
}

static inline int32_t yax_walldist(int32_t w)
{
    int32_t closestx, closesty;

    yax_getclosestpointonwall(w, &closestx, &closesty);
    return klabs(closestx-globalposx) + klabs(closesty-globalposy);

//    return klabs(wall[w].x-globalposx) + klabs(wall[w].y-globalposy);
}

// calculate distances to bunches and best start-drawing sectors
static void yax_scanbunches(int32_t bbeg, int32_t numhere, const uint8_t *lastgotsector)
{
    int32_t bnchcnt, bunchnum, j, k;
    int32_t startwall, endwall;

    UNREFERENCED_PARAMETER(lastgotsector);

    scansector_retfast = 1;
    scansector_collectsprites = 0;

    for (bnchcnt=bbeg; bnchcnt<bbeg+numhere; bnchcnt++)
    {
        int32_t walldist, bestsec=-1;
        int32_t bestwalldist=INT32_MAX, bestbestdist=INT32_MAX;

        bunchnum = bunches[yax_globalcf][bnchcnt];

        for (SECTORS_OF_BUNCH(bunchnum,!yax_globalcf, k))
        {
            int32_t checkthisec = 0;

            if (inside(globalposx, globalposy, k)==1)
            {
                bestsec = k;
                bestbestdist = 0;
                break;
            }

            startwall = sector[k].wallptr;
            endwall = startwall+sector[k].wallnum;

            for (j=startwall; j<endwall; j++)
            {
/*
                if ((w=yax_getnextwall(j,!yax_globalcf))>=0)
                    if ((ns=wall[w].nextsector)>=0)
                        if ((lastgotsector[ns>>3]&(1<<(ns&7)))==0)
                            continue;
*/
                walldist = yax_walldist(j);
                if (walldist < bestwalldist)
                {
                    checkthisec = 1;
                    bestwalldist = walldist;
                }
            }

            if (checkthisec)
            {
                numscans = numbunches = 0;
                if (getrendermode() == REND_CLASSIC)
                    scansector(k);
#ifdef USE_OPENGL
                else
                    polymost_scansector(k);
#endif
                if (numbunches > 0)
                {
                    bestsec = k;
                    bestbestdist = bestwalldist;
                }
            }
        }

        bunchsec[bunchnum] = bestsec;
        bunchdist[bunchnum] = bestbestdist;
    }

    scansector_collectsprites = 1;
    scansector_retfast = 0;
}

static int yax_cmpbunches(const void *b1, const void *b2)
{
    return (bunchdist[B_UNBUF16(b2)] - bunchdist[B_UNBUF16(b1)]);
}


void yax_tweakpicnums(int32_t bunchnum, int32_t cf, int32_t restore)
{
    // for polymer, this is called before polymer_drawrooms() with restore==0
    // and after polymer_drawmasks() with restore==1

    int32_t i, dastat;
    static int16_t opicnum[2][MAXSECTORS];
#ifdef DEBUGGINGAIDS
    static uint8_t expect_restore[2][YAX_MAXBUNCHES];

    // must call this with restore == 0, 1,  0, 1,  0, 1,  ...
    Bassert(expect_restore[cf][bunchnum] == restore);
    expect_restore[cf][bunchnum] = !expect_restore[cf][bunchnum];
#endif

    for (SECTORS_OF_BUNCH(bunchnum, cf, i))
    {
        dastat = (SECTORFLD(i,stat, cf)&(128+256));

        // only consider non-masked ceilings/floors
        if (dastat==0 || (restore==1 && opicnum[cf][i]&0x8000))
        {
            if (!restore)
            {
                opicnum[cf][i] = SECTORFLD(i,picnum, cf);
                if (editstatus && showinvisibility)
                    SECTORFLD(i,picnum, cf) = MAXTILES-1;
                else //if ((dastat&(128+256))==0)
                    SECTORFLD(i,picnum, cf) = 13; //FOF;
            }
            else
            {
                SECTORFLD(i,picnum, cf) = opicnum[cf][i];
            }
#ifdef POLYMER
            // will be called only in editor
            if (getrendermode() == REND_POLYMER)
            {
                if (!restore)
                {
                    SECTORFLD(i,stat, cf) |= 128;
                    opicnum[cf][i] |= 0x8000;
                }
                else
                {
                    SECTORFLD(i,stat, cf) &= ~128;
                    SECTORFLD(i,picnum, cf) &= 0x7fff;
                    opicnum[cf][i] = 0;
                }
            }
#endif
        }
    }
}

static void yax_copytsprites()
{
    int32_t i, spritenum, gotthrough, sectnum;
    int32_t sortcnt = yax_spritesortcnt[yax_globallev];
    const uspritetype *spr;

    for (i=0; i<sortcnt; i++)
    {
        spritenum = yax_tsprite[yax_globallev][i];

        gotthrough = spritenum&(MAXSPRITES|(MAXSPRITES<<1));

        spritenum &= MAXSPRITES-1;
        spr = (uspritetype *)&sprite[spritenum];
        sectnum = spr->sectnum;

        if (gotthrough == (MAXSPRITES|(MAXSPRITES<<1)))
        {
            if (yax_globalbunch != yax_tsprfrombunch[yax_globallev][i])
                continue;
        }
        else
        {
            int32_t cf = -1;

            if (gotthrough == MAXSPRITES)
                cf = YAX_CEILING;  // sprite got here through the ceiling of lower sector
            else if (gotthrough == (MAXSPRITES<<1))
                cf = YAX_FLOOR;  // sprite got here through the floor of upper sector

            if (cf != -1)
            {
                if ((yax_globallev-YAX_MAXDRAWS)*(-1 + 2*cf) > 0)
                    if (yax_getbunch(sectnum, cf) != yax_globalbunch)
                        continue;

                sectnum = yax_getneighborsect(spr->x, spr->y, sectnum, cf);
                if (sectnum < 0)
                    continue;
            }
        }

        if (spritesortcnt >= maxspritesonscreen)
            break;

        Bmemcpy(&tsprite[spritesortcnt], spr, sizeof(spritetype));
        tsprite[spritesortcnt].owner = spritenum;
        tsprite[spritesortcnt].extra = 0;
        tsprite[spritesortcnt].sectnum = sectnum;  // potentially tweak sectnum!
        spritesortcnt++;
    }
}


void yax_preparedrawrooms(void)
{
    if (getrendermode() == REND_POLYMER || numyaxbunches==0)
        return;

    g_nodraw = 1;
    Bmemset(yax_spritesortcnt, 0, sizeof(yax_spritesortcnt));
    Bmemset(haveymost, 0, (numyaxbunches+7)>>3);

    if (getrendermode() == REND_CLASSIC && ymostallocsize < xdimen*numyaxbunches)
    {
        ymostallocsize = xdimen*numyaxbunches;
        yumost = (int16_t *)Xrealloc(yumost, ymostallocsize*sizeof(int16_t));
        ydmost = (int16_t *)Xrealloc(ydmost, ymostallocsize*sizeof(int16_t));
    }
}

void yax_drawrooms(void (*SpriteAnimFunc)(int32_t,int32_t,int32_t,int32_t),
                   int16_t sectnum, int32_t didmirror, int32_t smoothr)
{
    static uint8_t havebunch[YAX_MAXBUNCHES>>3];

    const int32_t horiz = global100horiz;

    int32_t i, j, k, lev, cf, nmp;
    int32_t bnchcnt, bnchnum[2] = {0,0}, maxlev[2];
    int16_t ourbunch[2] = {-1,-1}, osectnum=sectnum;
    int32_t bnchbeg[YAX_MAXDRAWS][2], bnchend[YAX_MAXDRAWS][2];
    int32_t bbeg, numhere;

    // original (1st-draw) and accumulated ('per-level') gotsector bitmaps
    static uint8_t ogotsector[MAXSECTORS>>3], lgotsector[MAXSECTORS>>3];
#ifdef YAX_DEBUG
    uint64_t t;
#endif

    if (getrendermode() == REND_POLYMER || numyaxbunches==0)
    {
#ifdef ENGINE_SCREENSHOT_DEBUG
        engine_screenshot = 0;
#endif
        return;
    }

    // if we're here, there was just a drawrooms() call with g_nodraw=1

    Bmemcpy(ogotsector, gotsector, (numsectors+7)>>3);

    if (sectnum >= 0)
        yax_getbunches(sectnum, &ourbunch[0], &ourbunch[1]);
    Bmemset(&havebunch, 0, (numyaxbunches+7)>>3);

    // first scan all bunches above, then all below...
    for (cf=0; cf<2; cf++)
    {
        yax_globalcf = cf;

        if (cf==1)
        {
            sectnum = osectnum;
            Bmemcpy(gotsector, ogotsector, (numsectors+7)>>3);
        }

        for (lev=0; /*lev<YAX_MAXDRAWS*/; lev++)
        {
            yax_globallev = YAX_MAXDRAWS + (-1 + 2*cf)*(lev+1);

            bbeg = bnchbeg[lev][cf] = bnchend[lev][cf] = bnchnum[cf];
            numhere = 0;

            for (i=0; i<numsectors; i++)
            {
                if (!(gotsector[i>>3]&(1<<(i&7))))
                    continue;

                j = yax_getbunch(i, cf);
                if (j >= 0 && !(havebunch[j>>3]&(1<<(j&7))))
                {
                    if (getrendermode() == REND_CLASSIC && (haveymost[j>>3]&(1<<(j&7)))==0)
                    {
                        yaxdebug("%s, l %d: skipped bunch %d (no *most)", cf?"v":"^", lev, j);
                        continue;
                    }

                    if ((SECTORFLD(i,stat, cf)&2) ||
                            (cf==0 && globalposz > sector[i].ceilingz) ||
                            (cf==1 && globalposz < sector[i].floorz))
                    {
                        havebunch[j>>3] |= (1<<(j&7));
                        bunches[cf][bnchnum[cf]++] = j;
                        bnchend[lev][cf]++;
                        numhere++;
                    }
                }
            }

            if (numhere > 0)
            {
                // found bunches -- need to fake-draw

                yax_scanbunches(bbeg, numhere, (uint8_t *)gotsector);

                qsort(&bunches[cf][bbeg], numhere, sizeof(int16_t), &yax_cmpbunches);

                if (numhere > 1 && lev != YAX_MAXDRAWS-1)
                    Bmemset(lgotsector, 0, (numsectors+7)>>3);

                for (bnchcnt=bbeg; bnchcnt < bbeg+numhere; bnchcnt++)
                {
                    j = bunches[cf][bnchcnt];  // the actual bunchnum...
                    yax_globalbunch = j;
#ifdef YAX_DEBUG
                    t=getu64ticks();
#endif
                    k = bunchsec[j];

                    if (k < 0)
                    {
                        yaxprintf("%s, l %d: skipped bunch %d\n", cf?"v":"^", lev, j);
                        continue;
                    }

                    if (lev != YAX_MAXDRAWS-1)
                    {
#ifdef YAX_DEBUG
                        int32_t odsprcnt = yax_spritesortcnt[yax_globallev];
#endif
                        // +MAXSECTORS: force
                        drawrooms(globalposx,globalposy,globalposz,globalang,horiz,k+MAXSECTORS);
                        if (numhere > 1)
                            for (i=0; i<(numsectors+7)>>3; i++)
                                lgotsector[i] |= gotsector[i];

                        yaxdebug("l%d: faked (bn %2d) sec %4d,%3d dspr, ob=[%2d,%2d], sn=%4d, %.3f ms",
                                 yax_globallev-YAX_MAXDRAWS, j, k, yax_spritesortcnt[yax_globallev]-odsprcnt,
                                 ourbunch[0],ourbunch[1],sectnum,
                                 (double)(1000*(getu64ticks()-t))/u64tickspersec);
                    }

                    if (ourbunch[cf]==j)
                    {
                        ourbunch[cf] = yax_getbunch(k, cf);
                        sectnum = k;
                    }
                }

                if (numhere > 1 && lev != YAX_MAXDRAWS-1)
                    Bmemcpy(gotsector, lgotsector, (numsectors+7)>>3);
            }

            if (numhere==0 || lev==YAX_MAXDRAWS-1)
            {
                // no new bunches or max level reached
                maxlev[cf] = lev - (numhere==0);
                break;
            }
        }
    }

//    yax_globalcf = -1;

    // now comes the real drawing!
    g_nodraw = 0;
    scansector_collectsprites = 0;

    if (editstatus==1 && in3dmode())
    {
        if (getrendermode() == REND_CLASSIC)
        {
            begindrawing();
            draw_rainbow_background();
            enddrawing();
        }
#ifdef USE_OPENGL
        else
        {
            bglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        }
#endif
    }

    for (cf=0; cf<2; cf++)
    {
        yax_globalcf = cf;

        for (lev=maxlev[cf]; lev>=0; lev--)
        {
            yax_globallev = YAX_MAXDRAWS + (-1 + 2*cf)*(lev+1);
            scansector_collectsprites = (lev == YAX_MAXDRAWS-1);

            for (bnchcnt=bnchbeg[lev][cf]; bnchcnt<bnchend[lev][cf]; bnchcnt++)
            {
                j = bunches[cf][bnchcnt];  // the actual bunchnum...
                k = bunchsec[j];  // best start-drawing sector
                yax_globalbunch = j;
#ifdef YAX_DEBUG
                t=getu64ticks();
#endif
                yax_tweakpicnums(j, cf, 0);
                if (k < 0)
                    continue;

                yax_nomaskdidit = 0;
                for (nmp=r_tror_nomaskpass; nmp>=0; nmp--)
                {
                    yax_nomaskpass = nmp;
                    drawrooms(globalposx,globalposy,globalposz,globalang,horiz,k+MAXSECTORS);  // +MAXSECTORS: force

                    if (nmp==1)
                    {
                        yaxdebug("nm1 l%d: DRAWN (bn %2d) sec %4d,          %.3f ms",
                                 yax_globallev-YAX_MAXDRAWS, j, k,
                                 (double)(1000*(getu64ticks()-t))/u64tickspersec);

                        if (!yax_nomaskdidit)
                        {
                            yax_nomaskpass = 0;
                            break;  // no need to draw the same stuff twice
                        }
                        Bmemcpy(yax_gotsector, gotsector, (numsectors+7)>>3);
                    }
                }

                if (!scansector_collectsprites)
                    spritesortcnt = 0;
                yax_copytsprites();
                yaxdebug("nm0 l%d: DRAWN (bn %2d) sec %4d,%3d tspr, %.3f ms",
                         yax_globallev-YAX_MAXDRAWS, j, k, spritesortcnt,
                         (double)(1000*(getu64ticks()-t))/u64tickspersec);

                SpriteAnimFunc(globalposx, globalposy, globalang, smoothr);
                drawmasks();
            }

            if (lev < maxlev[cf])
                for (bnchcnt=bnchbeg[lev+1][cf]; bnchcnt<bnchend[lev+1][cf]; bnchcnt++)
                    yax_tweakpicnums(bunches[cf][bnchcnt], cf, 1);  // restore picnums
        }
    }

#ifdef YAX_DEBUG
    t=getu64ticks();
#endif
    yax_globalcf = -1;
    yax_globalbunch = -1;
    yax_globallev = YAX_MAXDRAWS;
    scansector_collectsprites = 0;

    // draw base level
    drawrooms(globalposx,globalposy,globalposz,globalang,horiz,
              osectnum + MAXSECTORS*didmirror);
//    if (scansector_collectsprites)
//        spritesortcnt = 0;
    yax_copytsprites();
    yaxdebug("DRAWN base level sec %d,%3d tspr, %.3f ms", osectnum,
             spritesortcnt, (double)(1000*(getu64ticks()-t))/u64tickspersec);
    scansector_collectsprites = 1;

    for (cf=0; cf<2; cf++)
        if (maxlev[cf] >= 0)
            for (bnchcnt=bnchbeg[0][cf]; bnchcnt<bnchend[0][cf]; bnchcnt++)
                yax_tweakpicnums(bunches[cf][bnchcnt], cf, 1);  // restore picnums

#ifdef ENGINE_SCREENSHOT_DEBUG
    engine_screenshot = 0;
#endif

#ifdef YAX_DEBUG_YMOSTS
    if (getrendermode() == REND_CLASSIC && numyaxbunches>0)
    {
        char purple = getclosestcol(255, 0, 255);
        char yellow = getclosestcol(255, 255, 0);

        begindrawing();
        for (i=0; i<numyaxbunches; i++)
        {
            int32_t x, x1;

            if ((haveymost[i>>3]&(1<<i&7))==0)
                continue;

            x1 = i*xdimen;

            for (x=x1; x<x1+xdimen; x++)
            {
                if (yumost[x] >= 0 && yumost[x] < ydim && (x&1))
                    *((char *)frameplace + yumost[x]*bytesperline + x-x1) = purple;

                if (ydmost[x]-1 >= 0 && ydmost[x]-1 < ydim && !(x&1))
                    *((char *)frameplace + (ydmost[x]-1)*bytesperline + x-x1) = yellow;
            }
        }
        enddrawing();
    }
#endif
}

#endif  // defined YAX_ENABLE

// must have writable frame buffer, i.e. done begindrawing()
static void draw_rainbow_background(void)
{
    int32_t y, i;
    const int32_t N = 240;  // don't use fullbright colors
    const int32_t numfull=bytesperline/N, numrest=bytesperline%N;

    const char *const src = palookup[0] + 256*18;
    char *dst = (char *)frameplace;

    for (y=0; y<ydim; y++)
    {
        for (i=0; i<numfull; i++)
            Bmemcpy(&dst[N*i], src, N);
        if (numrest > 0)
            Bmemcpy(&dst[N*i], src, numrest);

        dst += bytesperline;
    }
}

//
// setslope
//
void setslope(int32_t sectnum, int32_t cf, int16_t slope)
{
    if (slope==0)
    {
        SECTORFLD(sectnum,stat, cf) &= ~2;
        SECTORFLD(sectnum,heinum, cf) = 0;
    }
    else
    {
        SECTORFLD(sectnum,stat, cf) |= 2;
        SECTORFLD(sectnum,heinum, cf) = slope;
    }
}

#define WALLS_ARE_CONSISTENT(k) ((wall[k].x == x2 && wall[k].y == y2)   \
                                 && ((wall[wall[k].point2]).x == x1 && (wall[wall[k].point2]).y == y1))

static int32_t getscore(int32_t w1c, int32_t w1f, int32_t w2c, int32_t w2f)
{
    if (w1c > w1f)
        swaplong(&w1c, &w1f);
    if (w2c > w2f)
        swaplong(&w2c, &w2f);

    // now: c <= f for each "wall-vline"

    int32_t maxceil = max(w1c, w2c);
    int32_t minflor = min(w1f, w2f);

    return minflor-maxceil;
}

const int16_t *chsecptr_onextwall = NULL;

int32_t checksectorpointer(int16_t i, int16_t sectnum)
{
    int32_t startsec, endsec;
    int32_t j, k, startwall, endwall, x1, y1, x2, y2, numnewwalls=0;
    int32_t bestnextwall=-1, bestnextsec=-1, bestwallscore=INT32_MIN;
    int32_t cz[4], fz[4], tmp[2], tmpscore=0;
#ifdef YAX_ENABLE
    int16_t cb[2], fb[2];
#endif

#if 0
    if (checksectorpointer_warn && (i<0 || i>=max(numwalls,newnumwalls)))
    {
        char buf[128];
        Bsprintf(buf, "WARN: checksectorpointer called with i=%d but (new)numwalls=%d", i, max(numwalls,newnumwalls));
        OSD_Printf("%s\n", buf);
        printmessage16("%s", buf);
        return 0;
    }
#endif

    x1 = wall[i].x;
    y1 = wall[i].y;
    x2 = (wall[wall[i].point2]).x;
    y2 = (wall[wall[i].point2]).y;

    k = wall[i].nextwall;
    if (k >= 0)          //Check for early exit
    {
        if (WALLS_ARE_CONSISTENT(k))
            return 0;

        wall[k].nextwall = wall[k].nextsector = -1;
    }

    if ((unsigned)wall[i].nextsector < (unsigned)numsectors && wall[i].nextwall < 0)
    {
        // if we have a nextsector but no nextwall, take this as a hint
        // to search only the walls of that sector
        startsec = wall[i].nextsector;
        endsec = startsec+1;
    }
    else
    {
        startsec = 0;
        endsec = numsectors;
    }

    wall[i].nextsector = wall[i].nextwall = -1;

    if (chsecptr_onextwall && (k=chsecptr_onextwall[i])>=0 && wall[k].nextwall<0)
    {
        // old next wall found
        if (WALLS_ARE_CONSISTENT(k))
        {
            j = sectorofwall(k);

            wall[i].nextsector = j;
            wall[i].nextwall = k;
            wall[k].nextsector = sectnum;
            wall[k].nextwall = i;

            return 1;
        }
    }

    for (j=startsec; j<endsec; j++)
    {
        if (j == sectnum)
            continue;

        YAX_SKIPSECTOR(j);

        startwall = sector[j].wallptr;
        endwall = startwall + sector[j].wallnum;
        for (k=startwall; k<endwall; k++)
        {
            if (!WALLS_ARE_CONSISTENT(k))
                continue;

            // Don't create link if the other side is connected to another wall.
            // The nextwall relation should be definitely one-to-one at all times!
            if (wall[k].nextwall>=0 && wall[k].nextwall != i)
                continue;
#ifdef YAX_ENABLE
            yax_getbunches(sectnum, &cb[0], &fb[0]);
            yax_getbunches(j, &cb[1], &fb[1]);

            if ((cb[0]>=0 && cb[0]==cb[1]) || (fb[0]>=0 && fb[0]==fb[1]))
            {
                tmpscore = INT32_MAX;
            }
            else
#endif
            {
                getzsofslope(sectnum, x1,y1, &cz[0],&fz[0]);
                getzsofslope(sectnum, x2,y2, &cz[1],&fz[1]);
                getzsofslope(j, x1,y1, &cz[2],&fz[2]);
                getzsofslope(j, x2,y2, &cz[3],&fz[3]);

                tmp[0] = getscore(cz[0],fz[0], cz[2],fz[2]);
                tmp[1] = getscore(cz[1],fz[1], cz[3],fz[3]);

                if ((tmp[0]^tmp[1]) >= 0)
                    tmpscore = tmp[0]+tmp[1];
                else
                    tmpscore = max(tmp[0], tmp[1]);
            }

            if (bestnextwall == -1 || tmpscore > bestwallscore)
            {
                bestwallscore = tmpscore;
                bestnextwall = k;
                bestnextsec = j;
            }

            numnewwalls++;
        }
    }

    // sectnum -2 means dry run
    if (bestnextwall >= 0 && sectnum!=-2)
#ifdef YAX_ENABLE
        // for walls with TROR neighbors, be conservative in case if score <=0
        // (meaning that no wall area is mutually visible) -- it could be that
        // another sector is a better candidate later on
        if ((yax_getnextwall(i, 0)<0 && yax_getnextwall(i, 1)<0) || bestwallscore>0)
#endif
        {
//    initprintf("w%d new nw=%d (score %d)\n", i, bestnextwall, bestwallscore)
            wall[i].nextsector = bestnextsec;
            wall[i].nextwall = bestnextwall;
            wall[bestnextwall].nextsector = sectnum;
            wall[bestnextwall].nextwall = i;
        }

    return numnewwalls;
}

#undef WALLS_ARE_CONSISTENT

int32_t xb1[MAXWALLSB];  // Polymost uses this as a temp array
static int32_t yb1[MAXWALLSB], xb2[MAXWALLSB], yb2[MAXWALLSB];
int32_t rx1[MAXWALLSB], ry1[MAXWALLSB];
static int32_t rx2[MAXWALLSB], ry2[MAXWALLSB];
int16_t bunchp2[MAXWALLSB], thesector[MAXWALLSB];

int16_t bunchfirst[MAXWALLSB], bunchlast[MAXWALLSB];

static int32_t nodesperline, ysavecnt;
static int16_t *smost, *umost, *dmost, *bakumost, *bakdmost;
static int16_t *uplc, *dplc, *uwall, *dwall;
static int32_t *swplc, *lplc, *swall, *lwall;
#ifdef HIGH_PRECISION_SPRITE
static float *swallf;
#endif

static int32_t smostcnt;
static int32_t smoststart[MAXWALLSB];
static char smostwalltype[MAXWALLSB];
static int32_t smostwall[MAXWALLSB], smostwallcnt = -1;

static vec3_t spritesxyz[MAXSPRITESONSCREEN+1];

int32_t xdimen = -1, xdimenrecip, halfxdimen, xdimenscale, xdimscale;
float fxdimen = -1.f;
int32_t ydimen;
intptr_t frameoffset;

static int32_t nrx1[8], nry1[8], nrx2[8], nry2[8]; // JBF 20031206: Thanks Ken

int32_t rxi[8], ryi[8];
static int32_t rzi[8], rxi2[8], ryi2[8], rzi2[8];
static int32_t xsi[8], ysi[8], horizycent;
static int32_t *horizlookup=0, *horizlookup2=0;

int32_t globalposx, globalposy, globalposz, globalhoriz;
float fglobalposx, fglobalposy, fglobalposz;
int16_t globalang, globalcursectnum;
int32_t globalpal, cosglobalang, singlobalang;
int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;
static int32_t globaluclip, globaldclip;
int32_t globvis, globalvisibility;
int32_t globalhisibility, globalpisibility, globalcisibility;
//char globparaceilclip, globparaflorclip;

int32_t xyaspect;
static int32_t viewingrangerecip;

static char globalxshift, globalyshift;
static int32_t globalxpanning, globalypanning;
int32_t globalshade, globalorientation;
int16_t globalpicnum;
static int16_t globalshiftval;
#ifdef HIGH_PRECISION_SPRITE
static int64_t globalzd;
#else
static int32_t globalzd;
#endif
static int32_t globalyscale;
static int32_t globalxspan, globalyspan, globalispow2=1;  // true if texture has power-of-two x and y size
static intptr_t globalbufplc;

static int32_t globaly1, globalx2;

int16_t sectorborder[256];
int32_t ydim16, qsetmode = 0;
int16_t pointhighlight=-1, linehighlight=-1, highlightcnt=0;
static int32_t *lastx;

int32_t halfxdim16, midydim16;

static vec2_t const hitscangoal = { (1<<29)-1, (1<<29)-1 };
#ifdef USE_OPENGL
int32_t hitallsprites = 0;
#endif

typedef struct
{
    int32_t sx, sy, z;
    int16_t a, picnum;
    int8_t dashade;
    char dapalnum, dastat;
    uint8_t daalpha, dablend;
    char pagesleft;
    int32_t cx1, cy1, cx2, cy2;
    int32_t uniqid;    //JF extension
} permfifotype;
static permfifotype permfifo[MAXPERMS];
static int32_t permhead = 0, permtail = 0;

EDUKE32_STATIC_ASSERT(MAXWALLSB < INT16_MAX);
int16_t numscans, numbunches;
static int16_t numhits;

uint8_t vgapal16[4*256] =
{
    0,0,0,0, 170,0,0,0, 0,170,0,0, 170,170,0,0, 0,0,170,0,
    170,0,170,0, 0,85,170,0, 170,170,170,0, 85,85,85,0, 255,85,85,0,
    85,255,85,0, 255,255,85,0, 85,85,255,0, 255,85,255,0, 85,255,255,0,
    255,255,255,0
};

int16_t searchit;
int32_t searchx = -1, searchy;                          //search input
int16_t searchsector, searchwall, searchstat;     //search output

// SEARCHBOTTOMWALL:
//   When aiming at a the bottom part of a 2-sided wall whose bottom part
//   is swapped (.cstat&2), searchbottomwall equals that wall's .nextwall. In all
//   other cases (when aiming at a wall), searchbottomwall equals searchwall.
//
// SEARCHISBOTTOM:
//  When aiming at a 2-sided wall, 1 if aiming at the bottom part, 0 else
int16_t searchbottomwall, searchisbottom;

char inpreparemirror = 0;
static int32_t mirrorsx1, mirrorsy1, mirrorsx2, mirrorsy2;

static int32_t setviewcnt = 0; // interface layers use this now
static intptr_t bakframeplace[4];
static int32_t bakxsiz[4], bakysiz[4];
static vec2_t bakwindowxy1[4], bakwindowxy2[4];
#ifdef USE_OPENGL
static int32_t bakrendmode;
#endif
static int32_t baktile;

char apptitle[256] = "Build Engine";

//
// Internal Engine Functions
//

// returns: 0=continue sprite collecting;
//          1=break out of sprite collecting;
int32_t engine_addtsprite(int16_t z, int16_t sectnum)
{
    uspritetype *spr = (uspritetype *)&sprite[z];
#ifdef YAX_ENABLE
    if (g_nodraw==0)
    {
        if (numyaxbunches==0)
        {
#endif
            if (spritesortcnt >= maxspritesonscreen)
                return 1;

            Bmemcpy(&tsprite[spritesortcnt], spr, sizeof(spritetype));
            tsprite[spritesortcnt].extra = 0;
            tsprite[spritesortcnt++].owner = z;

#ifdef YAX_ENABLE
        }
    }
    else
        if (yax_nomaskpass==0)
    {
        int16_t *sortcnt = &yax_spritesortcnt[yax_globallev];

        if (*sortcnt >= maxspritesonscreen)
            return 1;

        yax_tsprite[yax_globallev][*sortcnt] = z;
        if (yax_globalbunch >= 0)
        {
            yax_tsprite[yax_globallev][*sortcnt] |= (MAXSPRITES|(MAXSPRITES<<1));
            yax_tsprfrombunch[yax_globallev][*sortcnt] = yax_globalbunch;
        }
        (*sortcnt)++;

        // now check whether the tsprite needs duplication into another level
        if ((spr->cstat&48)==32)
            return 0;

        int16_t cb, fb;

        yax_getbunches(sectnum, &cb, &fb);
        if (cb < 0 && fb < 0)
            return 0;

        int32_t spheight;
        int16_t spzofs = spriteheightofs(z, &spheight, 1);

        // TODO: get*zofslope?
        if (cb>=0 && spr->z+spzofs-spheight < sector[sectnum].ceilingz)
        {
            sortcnt = &yax_spritesortcnt[yax_globallev-1];
            if (*sortcnt < maxspritesonscreen)
            {
                yax_tsprite[yax_globallev-1][*sortcnt] = z|MAXSPRITES;
                (*sortcnt)++;
            }
        }
        if (fb>=0 && spr->z+spzofs > sector[sectnum].floorz)
        {
            sortcnt = &yax_spritesortcnt[yax_globallev+1];
            if (*sortcnt < maxspritesonscreen)
            {
                yax_tsprite[yax_globallev+1][*sortcnt] = z|(MAXSPRITES<<1);
                (*sortcnt)++;
            }
        }
    }
#endif

    return 0;
}

static inline vec2_t get_rel_coords(int32_t const x, int32_t const y)
{
    vec2_t const p = {
        dmulscale6(y,cosglobalang, -x,singlobalang),
        dmulscale6(x,cosviewingrangeglobalang, y,sinviewingrangeglobalang)
    };

    return p;
}

// Note: the returned y coordinates are not actually screen coordinates, but
// potentially clipped player-relative y coordinates.
static int get_screen_coords(const vec2_t p1, const vec2_t p2,
                             int32_t *sx1ptr, int32_t *sy1ptr,
                             int32_t *sx2ptr, int32_t *sy2ptr)
{
    int32_t sx1, sy1, sx2, sy2;

    // First point.

    if (p1.x >= -p1.y)
    {
        if (p1.x > p1.y || p1.y == 0)
            return 0;

        sx1 = halfxdimen + scale(p1.x, halfxdimen, p1.y)
            + (p1.x >= 0);  // Fix for SIGNED divide
        if (sx1 >= xdimen)
            sx1 = xdimen-1;

        sy1 = p1.y;
    }
    else
    {
        if (p2.x < -p2.y)
            return 0;

        sx1 = 0;

        int32_t tempint = (p1.x + p1.y) - (p2.x + p2.y);
        if (tempint == 0)
            return 0;
        sy1 = p1.y + scale(p2.y-p1.y, p1.x+p1.y, tempint);
    }

    if (sy1 < 256)
        return 0;

    // Second point.

    if (p2.x <= p2.y)
    {
        if (p2.x < -p2.y || p2.y == 0)
            return 0;

        sx2 = halfxdimen + scale(p2.x,halfxdimen,p2.y) - 1
            + (p2.x >= 0);  // Fix for SIGNED divide
        if (sx2 >= xdimen)
            sx2 = xdimen-1;

        sy2 = p2.y;
    }
    else
    {
        if (p1.x > p1.y)
            return 0;

        sx2 = xdimen-1;

        int32_t tempint = (p1.y - p1.x) + (p2.x - p2.y);
        if (tempint == 0)
            return 0;
        sy2 = p1.y + scale(p2.y-p1.y, p1.y-p1.x, tempint);
    }

    if (sy2 < 256 || sx1 > sx2)
        return 0;

    *sx1ptr = sx1; *sy1ptr = sy1;
    *sx2ptr = sx2; *sy2ptr = sy2;

    return 1;
}

//
// scansector (internal)
//
static void scansector(int16_t startsectnum)
{
    if (startsectnum < 0)
        return;

    sectorborder[0] = startsectnum;
    int32_t sectorbordercnt = 1;

    do
    {
        const int32_t sectnum = sectorborder[--sectorbordercnt];

#ifdef YAX_ENABLE
        if (scansector_collectsprites)
#endif
        for (bssize_t i=headspritesect[sectnum]; i>=0; i=nextspritesect[i])
        {
            const uspritetype *const spr = (uspritetype *)&sprite[i];

            if (((spr->cstat & 0x8000) && !showinvisibility) || spr->xrepeat == 0 || spr->yrepeat == 0)
                continue;

            vec2_t const s = { spr->x-globalposx, spr->y-globalposy };

            if ((spr->cstat&48) || ((coord_t)s.x*cosglobalang+(coord_t)s.y*singlobalang > 0))
                if ((spr->cstat&(64+48))!=(64+16) || dmulscale6(sintable[(spr->ang+512)&2047],-s.x, sintable[spr->ang&2047],-s.y) > 0)
                    if (engine_addtsprite(i, sectnum))
                        break;
        }

        gotsector[sectnum>>3] |= pow2char[sectnum&7];

        const int32_t onumbunches = numbunches;
        const int32_t onumscans = numscans;

        const int32_t startwall = sector[sectnum].wallptr;
        const int32_t endwall = startwall + sector[sectnum].wallnum;
        int32_t scanfirst = numscans;

        vec2_t p1, p2 = { 0, 0 };

        for (bssize_t w=startwall; w<endwall; w++)
        {
            const uwalltype *const wal = (uwalltype *)&wall[w];
            const int32_t nextsectnum = wal->nextsector;
            const uwalltype *const wal2 = (uwalltype *)&wall[wal->point2];

            const int32_t x1 = wal->x-globalposx, y1 = wal->y-globalposy;
            const int32_t x2 = wal2->x-globalposx, y2 = wal2->y-globalposy;

            // The following block checks for a potential collection of a
            // sector that is "thin" in screen space. This is necessary because
            // not all sectors that are needed to be drawn may be collected via
            // drawalls() -> scansector() (although those are the majority).
            // Example: standing at exactly the intersection of a large sector
            // into four quadrant sub-sectors.
#if 1
            if (nextsectnum >= 0 && (wal->cstat&32) == 0 && sectorbordercnt < ARRAY_SSIZE(sectorborder))
#ifdef YAX_ENABLE
                if (yax_nomaskpass==0 || !yax_isislandwall(w, !yax_globalcf) || (yax_nomaskdidit=1, 0))
#endif
                if ((gotsector[nextsectnum>>3]&pow2char[nextsectnum&7]) == 0)
                {
                    // OV: E2L10
                    coord_t temp = (coord_t)x1*y2-(coord_t)x2*y1;
                    int32_t tempint = temp;
                    if (((uint64_t)tempint+262144) < 524288)  // BXY_MAX?
                        if (mulscale5(tempint,tempint) <= (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1))
                        {
                            sectorborder[sectorbordercnt++] = nextsectnum;
                            gotsector[nextsectnum>>3] |= pow2char[nextsectnum&7];
                        }
                }
#endif
            p1 = (w == startwall || wall[w - 1].point2 != w) ? get_rel_coords(x1, y1) : p2;
            p2 = get_rel_coords(x2, y2);

            if (p1.y < 256 && p2.y < 256)
                goto skipitaddwall;

            // If wall's NOT facing you
            if (dmulscale32(p1.x, p2.y, -p2.x, p1.y) >= 0)
                goto skipitaddwall;

            if (get_screen_coords(p1, p2, &xb1[numscans], &yb1[numscans], &xb2[numscans], &yb2[numscans]))
            {
                // Made it all the way!
                thesector[numscans] = sectnum; thewall[numscans] = w;
                rx1[numscans] = p1.x; ry1[numscans] = p1.y;
                rx2[numscans] = p2.x; ry2[numscans] = p2.y;
                bunchp2[numscans] = numscans+1;
                numscans++;
            }

skipitaddwall:
            if (wall[w].point2 < w && scanfirst < numscans)
                bunchp2[numscans-1] = scanfirst, scanfirst = numscans;
        }

        for (bssize_t s=onumscans; s<numscans; s++)
            if (wall[thewall[s]].point2 != thewall[bunchp2[s]] || xb2[s] >= xb1[bunchp2[s]])
            {
                bunchfirst[numbunches++] = bunchp2[s], bunchp2[s] = -1;
#ifdef YAX_ENABLE
                if (scansector_retfast)
                    return;
#endif
            }

        for (bssize_t bn=onumbunches; bn<numbunches; bn++)
        {
            int32_t s;
            for (s=bunchfirst[bn]; bunchp2[s]>=0; s=bunchp2[s])
                /* do nothing */;
            bunchlast[bn] = s;
        }
    }
    while (sectorbordercnt > 0);
}

#if DEBUGGINGAIDS >= 2
// Printing functions for collected scans (called "wall proxies" by
// http://fabiensanglard.net/duke3d/build_engine_internals.php) and
// bunches. For use from within the debugger.

void printscans(void)
{
    static uint8_t didscan[(MAXWALLSB+7)>>3];

    Bmemset(didscan, 0, sizeof(didscan));

    for (bssize_t s=0; s<numscans; s++)
    {
        if (bunchp2[s] >= 0 && (didscan[s>>3] & (1<<(s&7)))==0)
        {
            printf("scan ");

            int z = s;
            do
            {
                const int cond = (wall[thewall[z]].point2 != thewall[bunchp2[z]] ||
                                  xb2[z] >= xb1[bunchp2[z]]);

                printf("%s%d(%d) ", cond ? "!" : "", z, thewall[z]);

                if (didscan[z>>3] & (1<<(z&7)))
                {
                    printf("*");
                    break;
                }

                didscan[z>>3] |= (1<<(z&7));
                z = bunchp2[z];
            } while (z >= 0);

            printf("\n");
        }
    }
}

void printbunches(void)
{
    for (bssize_t bn=0; bn<numbunches; bn++)
    {
        printf("bunch %d: ", bn);
        for (bssize_t s=bunchfirst[bn]; s>=0; s=bunchp2[s])
            printf("%d(%d) ", s, thewall[s]);
        printf("\n");
    }
}
#endif

////////// *WALLSCAN HELPERS //////////

#define WSHELPER_DECL inline //ATTRIBUTE((always_inline))

static WSHELPER_DECL void tweak_tsizes(vec2s_t *tsiz)
{
    if (pow2long[picsiz[globalpicnum]&15] == tsiz->x)
        tsiz->x--;
    else
        tsiz->x = -tsiz->x;

    if (pow2long[picsiz[globalpicnum]>>4] == tsiz->y)
        tsiz->y = (picsiz[globalpicnum]>>4);
    else
        tsiz->y = -tsiz->y;
}

static WSHELPER_DECL void calc_bufplc(intptr_t *bufplc, int32_t lw, vec2s_t tsiz)
{
    // CAUTION: lw can be negative!
    int32_t i = lw + globalxpanning;

//    if (i >= tsizx)
    {
        if (tsiz.x < 0)
            i = (uint32_t)i % -tsiz.x;
        else
            i &= tsiz.x;
    }

    if (tsiz.y < 0)
        i *= -tsiz.y;
    else
        i <<= tsiz.y;

//    Bassert(i >= 0 && i < tilesiz[globalpicnum].x*tilesiz[globalpicnum].y);

    // Address is at the first row of tile storage (which is column-major).
    *bufplc = waloff[globalpicnum] + i;
}

static WSHELPER_DECL void calc_vplcinc_wall(uint32_t *vplc, int32_t *vinc, inthi_t sw, int32_t y1v)
{
    *vinc = sw*globalyscale;
    *vplc = globalzd + (uint32_t)(*vinc)*(y1v-globalhoriz+1);
}

#ifdef HIGH_PRECISION_SPRITE
static WSHELPER_DECL void calc_vplcinc_sprite(uint32_t *vplc, int32_t *vinc, int32_t x, int32_t y1v)
{
    inthi_t const tmpvinc = inthi_rintf(swallf[x]);
    inthi_t const tmpvplc = globalzd + tmpvinc*(y1v-globalhoriz+1);

    *vinc = tmpvinc;
    // Clamp the vertical texture coordinate!
    *vplc = min(max(0, tmpvplc), UINT32_MAX);
}
#endif

static int32_t drawing_sprite = 0;

static WSHELPER_DECL void calc_vplcinc(uint32_t *vplc, int32_t *vinc, const int32_t *swal, int32_t x, int32_t y1v)
{
#if !defined HIGH_PRECISION_SPRITE
    (void)drawing_sprite;
#else
    if (drawing_sprite)
        calc_vplcinc_sprite(vplc, vinc, x, y1v);
    else
#endif
        calc_vplcinc_wall(vplc, vinc, swal[x], y1v);
}

#undef NONPOW2_YSIZE_ASM
#if !defined ENGINE_USING_A_C
# if defined CLASSIC_NONPOW2_YSIZE_WALLS || defined CLASSIC_NONPOW2_YSIZE_SPRITES
#  define NONPOW2_YSIZE_ASM
# endif
#endif


//
// maskwallscan (internal)
//
static void maskwallscan(int32_t x1, int32_t x2, int32_t saturatevplc)
{
    if (globalshiftval < 0) return;
    if ((uwall[x1] > ydimen) && (uwall[x2] > ydimen)) return;
    if ((dwall[x1] < 0) && (dwall[x2] < 0)) return;

    vec2s_t tsiz = tilesiz[globalpicnum];
    if ((tsiz.x <= 0) || (tsiz.y <= 0)) return;

    setgotpic(globalpicnum);

    if (waloff[globalpicnum] == 0) loadtile(globalpicnum);

    tweak_tsizes(&tsiz);

    if (EDUKE32_PREDICT_FALSE(palookup[globalpal] == NULL))
        globalpal = 0;

    intptr_t const fpalookup = FP_OFF(palookup[globalpal]);

    setupmvlineasm(globalshiftval, saturatevplc);

    int32_t x = x1;
    while ((x <= x2) && (startumost[x+windowxy1.x] > startdmost[x+windowxy1.x]))
        x++;

    intptr_t p = x+frameoffset;

    int32_t y1ve[4], y2ve[4];

#ifdef NONPOW2_YSIZE_ASM
    if (globalshiftval==0)
        goto do_mvlineasm1;
#endif

#ifdef MULTI_COLUMN_VLINE
    for (; (x<=x2)&&(p&3); x++,p++)
    {
        y1ve[0] = max(uwall[x],startumost[x+windowxy1.x]-windowxy1.y);
        y2ve[0] = min(dwall[x],startdmost[x+windowxy1.x]-windowxy1.y);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup + getpalookupsh(mulscale16(swall[x],globvis));

        calc_bufplc(&bufplce[0], lwall[x], tsiz);
        calc_vplcinc(&vplce[0], &vince[0], swall, x, y1ve[0]);

        mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0],p+ylookup[y1ve[0]]);
    }
    for (; x<=x2-3; x+=4,p+=4)
    {
        char bad = 0;

        for (bssize_t z=3,dax=x+3; z>=0; z--,dax--)
        {
            y1ve[z] = max(uwall[dax],startumost[dax+windowxy1.x]-windowxy1.y);
            y2ve[z] = min(dwall[dax],startdmost[dax+windowxy1.x]-windowxy1.y)-1;
            if (y2ve[z] < y1ve[z]) { bad += pow2char[z]; continue; }

            calc_bufplc(&bufplce[z], lwall[dax], tsiz);
            calc_vplcinc(&vplce[z], &vince[z], swall, dax, y1ve[z]);
        }
        if (bad == 15) continue;

        palookupoffse[0] = fpalookup + getpalookupsh(mulscale16(swall[x],globvis));
        palookupoffse[3] = fpalookup + getpalookupsh(mulscale16(swall[x+3],globvis));

        if ((palookupoffse[0] == palookupoffse[3]) && ((bad&0x9) == 0))
        {
            palookupoffse[1] = palookupoffse[0];
            palookupoffse[2] = palookupoffse[0];
        }
        else
        {
            palookupoffse[1] = fpalookup + getpalookupsh(mulscale16(swall[x+1],globvis));
            palookupoffse[2] = fpalookup + getpalookupsh(mulscale16(swall[x+2],globvis));
        }

        int32_t const u4 = max(max(y1ve[0],y1ve[1]),max(y1ve[2],y1ve[3]));
        int32_t const d4 = min(min(y2ve[0],y2ve[1]),min(y2ve[2],y2ve[3]));

        if ((bad > 0) || (u4 >= d4))
        {
            if (!(bad&1)) mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0],vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
            if (!(bad&2)) mvlineasm1(vince[1],palookupoffse[1],y2ve[1]-y1ve[1],vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
            if (!(bad&4)) mvlineasm1(vince[2],palookupoffse[2],y2ve[2]-y1ve[2],vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
            if (!(bad&8)) mvlineasm1(vince[3],palookupoffse[3],y2ve[3]-y1ve[3],vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);
            continue;
        }

        if (u4 > y1ve[0]) vplce[0] = mvlineasm1(vince[0],palookupoffse[0],u4-y1ve[0]-1,vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
        if (u4 > y1ve[1]) vplce[1] = mvlineasm1(vince[1],palookupoffse[1],u4-y1ve[1]-1,vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
        if (u4 > y1ve[2]) vplce[2] = mvlineasm1(vince[2],palookupoffse[2],u4-y1ve[2]-1,vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
        if (u4 > y1ve[3]) vplce[3] = mvlineasm1(vince[3],palookupoffse[3],u4-y1ve[3]-1,vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);

        if (d4 >= u4) mvlineasm4(d4-u4+1, (char *)(ylookup[u4]+p));

        intptr_t const pp = p+ylookup[d4+1];

        if (y2ve[0] > d4) mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-d4-1,vplce[0],bufplce[0],pp+0);
        if (y2ve[1] > d4) mvlineasm1(vince[1],palookupoffse[1],y2ve[1]-d4-1,vplce[1],bufplce[1],pp+1);
        if (y2ve[2] > d4) mvlineasm1(vince[2],palookupoffse[2],y2ve[2]-d4-1,vplce[2],bufplce[2],pp+2);
        if (y2ve[3] > d4) mvlineasm1(vince[3],palookupoffse[3],y2ve[3]-d4-1,vplce[3],bufplce[3],pp+3);
    }
#endif

#ifdef NONPOW2_YSIZE_ASM
do_mvlineasm1:
#endif
    for (; x<=x2; x++,p++)
    {
        y1ve[0] = max(uwall[x],startumost[x+windowxy1.x]-windowxy1.y);
        y2ve[0] = min(dwall[x],startdmost[x+windowxy1.x]-windowxy1.y);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup + getpalookupsh(mulscale16(swall[x],globvis));

        calc_bufplc(&bufplce[0], lwall[x], tsiz);
        calc_vplcinc(&vplce[0], &vince[0], swall, x, y1ve[0]);

#ifdef NONPOW2_YSIZE_ASM
        if (globalshiftval==0)
            mvlineasm1nonpow2(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0],p+ylookup[y1ve[0]]);
        else
#endif
        mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0],p+ylookup[y1ve[0]]);
    }

    faketimerhandler();
}


//
// wallfront (internal)
//
int32_t wallfront(int32_t l1, int32_t l2)
{
    vec2_t const l1vect   = *(vec2_t *)&wall[thewall[l1]];
    vec2_t const l1p2vect = *(vec2_t *)&wall[wall[thewall[l1]].point2];
    vec2_t const l2vect   = *(vec2_t *)&wall[thewall[l2]];
    vec2_t const l2p2vect = *(vec2_t *)&wall[wall[thewall[l2]].point2];
    vec2_t d = { l1p2vect.x - l1vect.x, l1p2vect.y - l1vect.y };
    int32_t t1 = dmulscale2(l2vect.x-l1vect.x, d.y, -d.x, l2vect.y-l1vect.y); //p1(l2) vs. l1
    int32_t t2 = dmulscale2(l2p2vect.x-l1vect.x, d.y, -d.x, l2p2vect.y-l1vect.y); //p2(l2) vs. l1

    if (t1 == 0) { if (t2 == 0) return -1; t1 = t2; }
    if (t2 == 0) t2 = t1;

    if ((t1^t2) >= 0) //pos vs. l1
        return (dmulscale2(globalposx-l1vect.x, d.y, -d.x, globalposy-l1vect.y) ^ t1) >= 0;

    d.x = l2p2vect.x-l2vect.x;
    d.y = l2p2vect.y-l2vect.y;

    t1 = dmulscale2(l1vect.x-l2vect.x, d.y, -d.x, l1vect.y-l2vect.y); //p1(l1) vs. l2
    t2 = dmulscale2(l1p2vect.x-l2vect.x, d.y, -d.x, l1p2vect.y-l2vect.y); //p2(l1) vs. l2

    if (t1 == 0) { if (t2 == 0) return -1; t1 = t2; }
    if (t2 == 0) t2 = t1;

    if ((t1^t2) >= 0) //pos vs. l2
        return (dmulscale2(globalposx-l2vect.x,d.y,-d.x,globalposy-l2vect.y) ^ t1) < 0;

    return -2;
}

//
// spritewallfront (internal)
//
static inline int32_t spritewallfront(const uspritetype *s, int32_t w)
{
    const uwalltype *const wal = (uwalltype *)&wall[w];
    const uwalltype *wal2 = (uwalltype *)&wall[wal->point2];
    const vec2_t v = { wal->x, wal->y };

    return dmulscale32(wal2->x - v.x, s->y - v.y, -(s->x - v.x), wal2->y - v.y) >= 0;
}

//
//  spritebehindwall(internal)
//
#if 0
static int32_t spriteobstructswall(spritetype *s, int32_t w)
{
    walltype *wal;
    int32_t x, y;
    int32_t x1, y1;
    int32_t x2, y2;
    double a1, b1, c1;
    double a2, b2, c2;
    double d1, d2;

    // wall line equation
    wal = &wall[w]; x1 = wal->x - globalposx; y1 = wal->y - globalposy;
    wal = &wall[wal->point2]; x2 = wal->x - globalposx; y2 = wal->y - globalposy;
    if ((x2 - x1) != 0)
        a1 = (float)(y2 - y1)/(x2 - x1);
    else
        a1 = 1e+37; // not infinite, but almost ;)
    b1 = -1;
    c1 = (y1 - (a1 * x1));

    // player to sprite line equation
    if ((s->x - globalposx) != 0)
        a2 = (float)(s->y - globalposy)/(s->x - globalposx);
    else
        a2 = 1e+37;
    b2 = -1;
    c2 = 0;

    // intersection point
    d1 = (float)(1) / (a1*b2 - a2*b1);
    x = ((b1*c2 - b2*c1) * d1);
    y = ((a2*c1 - a1*c2) * d1);

    // distance between the sprite and the player
    a1 = s->x - globalposx;
    b1 = s->y - globalposy;
    d1 = (a1 * a1 + b1 * b1);

    // distance between the intersection point and the player
    d2 = (x * x + y * y);

    // check if the sprite obstructs the wall
    if ((d1 < d2) && (min(x1, x2) <= x) && (x <= max(x1, x2)) && (min(y1, y2) <= y) && (y <= max(y1, y2)))
        return 1;
    else
        return 0;
}
#endif
//
// bunchfront (internal)
//
static inline int32_t bunchfront(int32_t b1, int32_t b2)
{
    int b1f = bunchfirst[b1];
    int const x1b1 = xb1[b1f];
    int const x2b2 = xb2[bunchlast[b2]] + 1;

    if (x1b1 >= x2b2)
        return -1;

    int b2f = bunchfirst[b2];
    int const x1b2 = xb1[b2f];
    int const x2b1 = xb2[bunchlast[b1]] + 1;

    if (x1b2 >= x2b1)
        return -1;

    if (x1b1 >= x1b2)
    {
        for (; xb2[b2f] < x1b1; b2f = bunchp2[b2f]) { }
        return wallfront(b1f, b2f);
    }

    for (; xb2[b1f] < x1b2; b1f = bunchp2[b1f]) { }
    return wallfront(b1f, b2f);
}


//
// hline (internal)
//
static inline void hline(int32_t xr, int32_t yp)
{
    int32_t const xl = lastx[yp];
    if (xl > xr) return;
    int32_t const r = horizlookup2[yp-globalhoriz+horizycent];
    asm1 = (inthi_t)globalx1*r;
    asm2 = (inthi_t)globaly2*r;
    int32_t const s = getpalookupsh(mulscale16(r,globvis));

    hlineasm4(xr-xl,0,s,(uint32_t)globalx2*r+globalypanning,(uint32_t)globaly1*r+globalxpanning,
              ylookup[yp]+xr+frameoffset);
}


//
// slowhline (internal)
//
static inline void slowhline(int32_t xr, int32_t yp)
{
    int32_t const xl = lastx[yp]; if (xl > xr) return;
    int32_t const r = horizlookup2[yp-globalhoriz+horizycent];
    asm1 = (inthi_t)globalx1*r;
    asm2 = (inthi_t)globaly2*r;

    asm3 = (intptr_t)globalpalwritten + getpalookupsh(mulscale16(r,globvis));
    if (!(globalorientation&256))
    {
        mhline(globalbufplc,(uint32_t)globaly1*r+globalxpanning-asm1*(xr-xl),(xr-xl)<<16,0L,
               (uint32_t)globalx2*r+globalypanning-asm2*(xr-xl),ylookup[yp]+xl+frameoffset);
        return;
    }
    thline(globalbufplc,(uint32_t)globaly1*r+globalxpanning-asm1*(xr-xl),(xr-xl)<<16,0L,
           (uint32_t)globalx2*r+globalypanning-asm2*(xr-xl),ylookup[yp]+xl+frameoffset);
}


//
// prepwall (internal)
//
static void prepwall(int32_t z, const uwalltype *wal)
{
    int32_t l=0, ol=0, x;

    int32_t walxrepeat = (wal->xrepeat<<3);

    //lwall calculation
    int32_t tmpx = xb1[z]-halfxdimen;

    const int32_t topinc = -(ry1[z]>>2);
    const int32_t botinc = (ry2[z]-ry1[z])>>8;
    int32_t top = mulscale5(rx1[z],xdimen) + mulscale2(topinc,tmpx);
    int32_t bot = mulscale11(rx1[z]-rx2[z],xdimen) + mulscale2(botinc,tmpx);

    const int32_t splc = mulscale19(ry1[z],xdimscale);
    const int32_t sinc = mulscale16(ry2[z]-ry1[z],xdimscale);

    x = xb1[z];
    if (bot != 0)
    {
        l = divscale12(top,bot);
        swall[x] = mulscale21(l,sinc)+splc;
        l *= walxrepeat;
        lwall[x] = (l>>18);
    }
    while (x+4 <= xb2[z])
    {
        int32_t i;

        top += topinc; bot += botinc;
        if (bot != 0)
        {
            ol = l; l = divscale12(top,bot);
            swall[x+4] = mulscale21(l,sinc)+splc;
            l *= walxrepeat;
            lwall[x+4] = (l>>18);
        }

        i = (ol+l)>>1;

        lwall[x+2] = i>>18;
        lwall[x+1] = (ol+i)>>19;
        lwall[x+3] = (l+i)>>19;

        swall[x+2] = (swall[x]+swall[x+4])>>1;
        swall[x+1] = (swall[x]+swall[x+2])>>1;
        swall[x+3] = (swall[x+4]+swall[x+2])>>1;

        x += 4;
    }
    if (x+2 <= xb2[z])
    {
        top += (topinc>>1); bot += (botinc>>1);
        if (bot != 0)
        {
            ol = l; l = divscale12(top,bot);
            swall[x+2] = mulscale21(l,sinc)+splc;
            l *= walxrepeat;
            lwall[x+2] = (l>>18);
        }
        lwall[x+1] = (l+ol)>>19;
        swall[x+1] = (swall[x]+swall[x+2])>>1;
        x += 2;
    }
    if (x+1 <= xb2[z])
    {
        bot += (botinc>>2);
        if (bot != 0)
        {
            l = divscale12(top+(topinc>>2),bot);
            swall[x+1] = mulscale21(l,sinc)+splc;
            lwall[x+1] = mulscale18(l,walxrepeat);
        }
    }

    if (lwall[xb1[z]] < 0)
        lwall[xb1[z]] = 0;
    if (lwall[xb2[z]] >= walxrepeat && walxrepeat)
        lwall[xb2[z]] = walxrepeat-1;

    if (wal->cstat&8)
    {
        walxrepeat--;
        for (x=xb1[z]; x<=xb2[z]; x++)
            lwall[x] = walxrepeat-lwall[x];
    }
}


//
// animateoffs (internal)
//
#ifdef DEBUGGINGAIDS
int32_t animateoffs(int const tilenum, int fakevar)
#else
int32_t animateoffs(int const tilenum)
#endif
{
#ifdef DEBUGGINGAIDS
    UNREFERENCED_PARAMETER(fakevar);
#endif

    int const animnum = picanm[tilenum].num;

    if (animnum <= 0)
        return 0;

    int const i = totalclocklock >> (picanm[tilenum].sf & PICANM_ANIMSPEED_MASK);
    int offs = 0;

    switch (picanm[tilenum].sf & PICANM_ANIMTYPE_MASK)
    {
        case PICANM_ANIMTYPE_OSC:
        {
            int k = (i % (animnum << 1));
            offs = (k < animnum) ? k : (animnum << 1) - k;
        }
        break;
        case PICANM_ANIMTYPE_FWD: offs = i % (animnum + 1); break;
        case PICANM_ANIMTYPE_BACK: offs = -(i % (animnum + 1)); break;
    }

    return offs;
}


static inline void wallmosts_finish(int16_t *mostbuf, int32_t z1, int32_t z2,
                                    int32_t ix1, int32_t iy1, int32_t ix2, int32_t iy2)
{
    const int32_t y = scale(z1, xdimenscale, iy1)<<4;

#if 0
    // enable for paranoia:
    ix1 = clamp(ix1, 0, xres-1);
    ix2 = clamp(ix2, 0, xres-1);
    if (ix2-ix1 < 0)
        swaplong(&ix1, &ix2);
#endif
    // PK 20110423: a bit consistency checking is a good thing:
    int32_t const tmp = (ix2 - ix1 >= 0) ? (ix2 - ix1 + 1) : 1;
    int32_t const yinc = tabledivide32((scale(z2, xdimenscale, iy2) << 4) - y, tmp);

    qinterpolatedown16short((intptr_t)&mostbuf[ix1], tmp, y + (globalhoriz << 16), yinc);

    mostbuf[ix1] = clamp(mostbuf[ix1], 0, ydimen);
    mostbuf[ix2] = clamp(mostbuf[ix2], 0, ydimen);
}

#ifdef CLASSIC_Z_DIFF_64
typedef int64_t zint_t;

// For drawvox()
static FORCE_INLINE zint_t mulscale16z(int32_t a, int32_t d)
{
    return ((zint_t)a * d)>>16;
}

static FORCE_INLINE zint_t mulscale20z(int32_t a, int32_t d)
{
    return ((zint_t)a * d)>>20;
}

static FORCE_INLINE zint_t dmulscale24z(int32_t a, int32_t d, int32_t S, int32_t D)
{
    return (((zint_t)a * d) + ((zint_t)S * D)) >> 24;
}
#else
typedef int32_t zint_t;
# define mulscale16z mulscale16
# define mulscale20z mulscale20
# define dmulscale24z dmulscale24
#endif

//
// owallmost (internal)
//
static int32_t owallmost(int16_t *mostbuf, int32_t w, zint_t z)
{
    z <<= 7;
    const zint_t s1 = mulscale20z(globaluclip,yb1[w]), s2 = mulscale20z(globaluclip,yb2[w]);
    const zint_t s3 = mulscale20z(globaldclip,yb1[w]), s4 = mulscale20z(globaldclip,yb2[w]);
    const int32_t bad = (z<s1)+((z<s2)<<1)+((z>s3)<<2)+((z>s4)<<3);

    int32_t ix1 = xb1[w], iy1 = yb1[w];
    int32_t ix2 = xb2[w], iy2 = yb2[w];

    if ((bad&3) == 3)
    {
        for (bssize_t i=ix1; i<=ix2; i++)
            mostbuf[i] = 0;
        return bad;
    }

    if ((bad&12) == 12)
    {
        for (bssize_t i=ix1; i<=ix2; i++)
            mostbuf[i] = ydimen;
        return bad;
    }

    if (bad&3)
    {
        int32_t t = divscale30(z-s1,s2-s1);
        int32_t inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        int32_t xcross = xb1[w] + scale(mulscale30(yb2[w],t),xb2[w]-xb1[w],inty);

        if ((bad&3) == 2)
        {
            if (xb1[w] <= xcross) { iy2 = inty; ix2 = xcross; }
            for (bssize_t i=xcross+1; i<=xb2[w]; i++)
                mostbuf[i] = 0;
        }
        else
        {
            if (xcross <= xb2[w]) { iy1 = inty; ix1 = xcross; }
            for (bssize_t i=xb1[w]; i<=xcross; i++)
                mostbuf[i] = 0;
        }
    }

    if (bad&12)
    {
        int32_t t = divscale30(z-s3,s4-s3);
        int32_t inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        int32_t xcross = xb1[w] + scale(mulscale30(yb2[w],t),xb2[w]-xb1[w],inty);

        if ((bad&12) == 8)
        {
            if (xb1[w] <= xcross) { iy2 = inty; ix2 = xcross; }
            for (bssize_t i=xcross+1; i<=xb2[w]; i++)
                mostbuf[i] = ydimen;
        }
        else
        {
            if (xcross <= xb2[w]) { iy1 = inty; ix1 = xcross; }
            for (bssize_t i=xb1[w]; i<=xcross; i++)
                mostbuf[i] = ydimen;
        }
    }

    wallmosts_finish(mostbuf, z, z, ix1, iy1, ix2, iy2);

    return bad;
}

static inline zint_t wallmost_getz(int32_t fw, int32_t t, zint_t z,
                                   int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                                   int32_t xv, int32_t yv, int32_t dx, int32_t dy)
{
    // XXX: OVERFLOW with huge sectors and sloped ceilngs/floors!
    int32_t i = xv*(y1-globalposy) - yv*(x1-globalposx);
    const int32_t j = yv*x2 - xv*y2;

    if (klabs(j) > klabs(i>>3))
        i = divscale28(i,j);

    return dmulscale24z(dx*t, mulscale20z(y2,i)+((y1-wall[fw].y)<<8),
                        -dy*t, mulscale20z(x2,i)+((x1-wall[fw].x)<<8)) + ((z-globalposz)<<7);
}

//
// wallmost (internal)
//
static int32_t wallmost(int16_t *mostbuf, int32_t w, int32_t sectnum, char dastat)
{
    int32_t t, z;
    int32_t xv, yv;

    if (dastat == 0)
    {
        z = sector[sectnum].ceilingz-globalposz;
        if ((sector[sectnum].ceilingstat&2) == 0)
            return owallmost(mostbuf,w,z);
    }
    else
    {
        z = sector[sectnum].floorz-globalposz;
        if ((sector[sectnum].floorstat&2) == 0)
            return owallmost(mostbuf,w,z);
    }

    const int wi = thewall[w];
    if (wi == sector[sectnum].wallptr)
        return owallmost(mostbuf,w,z);

    const uwalltype *const wal = (uwalltype *)&wall[wi];
    const int32_t x1 = wal->x, x2 = wall[wal->point2].x-x1;
    const int32_t y1 = wal->y, y2 = wall[wal->point2].y-y1;

    const int w1 = sector[sectnum].wallptr, w2 = wall[w1].point2;
    const int32_t dx = wall[w2].x-wall[w1].x, dy = wall[w2].y-wall[w1].y;
    const int32_t dasqr = krecipasm(nsqrtasm(uhypsq(dx,dy)));

    if (dastat == 0)
    {
        t = mulscale15(sector[sectnum].ceilingheinum, dasqr);
        z = sector[sectnum].ceilingz;
    }
    else
    {
        t = mulscale15(sector[sectnum].floorheinum,dasqr);
        z = sector[sectnum].floorz;
    }


    if (xb1[w] == 0)
        { xv = cosglobalang+sinviewingrangeglobalang; yv = singlobalang-cosviewingrangeglobalang; }
    else
        { xv = x1-globalposx; yv = y1-globalposy; }
    zint_t z1 = wallmost_getz(w1, t, z, x1, y1, x2, y2, xv, yv, dx, dy);


    if (xb2[w] == xdimen-1)
        { xv = cosglobalang-sinviewingrangeglobalang; yv = singlobalang+cosviewingrangeglobalang; }
    else
        { xv = (x2+x1)-globalposx; yv = (y2+y1)-globalposy; }
    zint_t z2 = wallmost_getz(w1, t, z, x1, y1, x2, y2, xv, yv, dx, dy);


    const zint_t s1 = mulscale20(globaluclip,yb1[w]), s2 = mulscale20(globaluclip,yb2[w]);
    const zint_t s3 = mulscale20(globaldclip,yb1[w]), s4 = mulscale20(globaldclip,yb2[w]);
    const int32_t bad = (z1<s1)+((z2<s2)<<1)+((z1>s3)<<2)+((z2>s4)<<3);

    int32_t ix1 = xb1[w], ix2 = xb2[w];
    int32_t iy1 = yb1[w], iy2 = yb2[w];

    if ((bad&3) == 3)
    {
        for (bssize_t i=ix1; i<=ix2; i++)
            mostbuf[i] = 0;
        return bad;
    }

    if ((bad&12) == 12)
    {
        for (bssize_t i=ix1; i<=ix2; i++)
            mostbuf[i] = ydimen;
        return bad;
    }

    const int32_t oz1 = z1, oz2 = z2;

    if (bad&3)
    {
        //inty = intz / (globaluclip>>16)
        t = divscale30(oz1-s1,s2-s1+oz1-oz2);
        int32_t inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        int32_t intz = oz1 + mulscale30(oz2-oz1,t);
        int32_t xcross = xb1[w] + scale(mulscale30(yb2[w],t),xb2[w]-xb1[w],inty);

        //t = divscale30((x1<<4)-xcross*yb1[w],xcross*(yb2[w]-yb1[w])-((x2-x1)<<4));
        //inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        //intz = z1 + mulscale30(z2-z1,t);

        if ((bad&3) == 2)
        {
            if (xb1[w] <= xcross) { z2 = intz; iy2 = inty; ix2 = xcross; }
            for (bssize_t i=xcross+1; i<=xb2[w]; i++)
                mostbuf[i] = 0;
        }
        else
        {
            if (xcross <= xb2[w]) { z1 = intz; iy1 = inty; ix1 = xcross; }
            for (bssize_t i=xb1[w]; i<=xcross; i++)
                mostbuf[i] = 0;
        }
    }

    if (bad&12)
    {
        //inty = intz / (globaldclip>>16)
        t = divscale30(oz1-s3,s4-s3+oz1-oz2);
        int32_t inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        int32_t intz = oz1 + mulscale30(oz2-oz1,t);
        int32_t xcross = xb1[w] + scale(mulscale30(yb2[w],t),xb2[w]-xb1[w],inty);

        //t = divscale30((x1<<4)-xcross*yb1[w],xcross*(yb2[w]-yb1[w])-((x2-x1)<<4));
        //inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        //intz = z1 + mulscale30(z2-z1,t);

        if ((bad&12) == 8)
        {
            if (xb1[w] <= xcross) { z2 = intz; iy2 = inty; ix2 = xcross; }
            for (bssize_t i=xcross+1; i<=xb2[w]; i++)
                mostbuf[i] = ydimen;
        }
        else
        {
            if (xcross <= xb2[w]) { z1 = intz; iy1 = inty; ix1 = xcross; }
            for (bssize_t i=xb1[w]; i<=xcross; i++)
                mostbuf[i] = ydimen;
        }
    }

    wallmosts_finish(mostbuf, z1, z2, ix1, iy1, ix2, iy2);

    return bad;
}


// globalpicnum --> globalxshift, globalyshift
static void calc_globalshifts(void)
{
    globalxshift = (8-(picsiz[globalpicnum]&15));
    globalyshift = (8-(picsiz[globalpicnum]>>4));
    if (globalorientation&8) { globalxshift++; globalyshift++; }
    // PK: the following can happen for large (>= 512) tile sizes.
    // NOTE that global[xy]shift are unsigned chars.
    if (globalxshift > 31) globalxshift=0;
    if (globalyshift > 31) globalyshift=0;
}

static int32_t setup_globals_cf1(const usectortype *sec, int32_t pal, int32_t zd,
                                 int32_t picnum, int32_t shade, int32_t stat,
                                 int32_t xpanning, int32_t ypanning, int32_t x1)
{
    int32_t i, j, ox, oy;

    if (palookup[pal] != globalpalwritten)
    {
        globalpalwritten = palookup[pal];
        if (!globalpalwritten) globalpalwritten = palookup[globalpal];  // JBF: fixes null-pointer crash
        setpalookupaddress(globalpalwritten);
    }

    globalzd = zd;
    if (globalzd > 0) return 1;

    globalpicnum = picnum;
    if ((unsigned)globalpicnum >= MAXTILES) globalpicnum = 0;
    DO_TILE_ANIM(globalpicnum, 0);
    setgotpic(globalpicnum);
    if ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0)) return 1;
    if (waloff[globalpicnum] == 0) loadtile(globalpicnum);

    globalbufplc = waloff[globalpicnum];

    globalshade = shade;
    globvis = globalcisibility;
    if (sec->visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sec->visibility+16));
    globalorientation = stat;

    if ((globalorientation&64) == 0)
    {
        globalx1 = singlobalang; globalx2 = singlobalang;
        globaly1 = cosglobalang; globaly2 = cosglobalang;
        globalxpanning = ((inthi_t)globalposx<<20);
        globalypanning = -((inthi_t)globalposy<<20);
    }
    else
    {
        j = sec->wallptr;
        ox = wall[wall[j].point2].x - wall[j].x;
        oy = wall[wall[j].point2].y - wall[j].y;
        i = nsqrtasm(uhypsq(ox,oy)); if (i == 0) i = 1024; else i = tabledivide32(1048576, i);
        globalx1 = mulscale10(dmulscale10(ox,singlobalang,-oy,cosglobalang),i);
        globaly1 = mulscale10(dmulscale10(ox,cosglobalang,oy,singlobalang),i);
        globalx2 = -globalx1;
        globaly2 = -globaly1;

        ox = ((wall[j].x-globalposx)<<6); oy = ((wall[j].y-globalposy)<<6);
        i = dmulscale14(oy,cosglobalang,-ox,singlobalang);
        j = dmulscale14(ox,cosglobalang,oy,singlobalang);
        ox = i; oy = j;
        globalxpanning = (coord_t)globalx1*ox - (coord_t)globaly1*oy;
        globalypanning = (coord_t)globaly2*ox + (coord_t)globalx2*oy;
    }
    globalx2 = mulscale16(globalx2,viewingrangerecip);
    globaly1 = mulscale16(globaly1,viewingrangerecip);

    calc_globalshifts();

    if ((globalorientation&0x4) > 0)
    {
        i = globalxpanning; globalxpanning = globalypanning; globalypanning = i;
        i = globalx2; globalx2 = -globaly1; globaly1 = -i;
        i = globalx1; globalx1 = globaly2; globaly2 = i;
    }
    if ((globalorientation&0x10) > 0) globalx1 = -globalx1, globaly1 = -globaly1, globalxpanning = -(inthi_t)globalxpanning;
    if ((globalorientation&0x20) > 0) globalx2 = -globalx2, globaly2 = -globaly2, globalypanning = -(inthi_t)globalypanning;
    globalx1 <<= globalxshift; globaly1 <<= globalxshift;
    globalx2 <<= globalyshift;  globaly2 <<= globalyshift;
    globalxpanning <<= globalxshift; globalypanning <<= globalyshift;
    globalxpanning = (uint32_t)globalxpanning + (xpanning<<24);
    globalypanning = (uint32_t)globalypanning + (ypanning<<24);
    globaly1 = (-globalx1-globaly1)*halfxdimen;
    globalx2 = (globalx2-globaly2)*halfxdimen;

    sethlinesizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,globalbufplc);

    globalx2 += globaly2*(x1-1);
    globaly1 += globalx1*(x1-1);
    globalx1 = mulscale16(globalx1,globalzd);
    globalx2 = mulscale16(globalx2,globalzd);
    globaly1 = mulscale16(globaly1,globalzd);
    globaly2 = mulscale16(globaly2,globalzd);
    globvis = klabs(mulscale10(globvis,globalzd));

    return 0;
}

//
// ceilscan (internal)
//
static void ceilscan(int32_t x1, int32_t x2, int32_t sectnum)
{
    int32_t x, y1, y2;
    const usectortype *const sec = (usectortype *)&sector[sectnum];

    if (setup_globals_cf1(sec, sec->ceilingpal, sec->ceilingz-globalposz,
                          sec->ceilingpicnum, sec->ceilingshade, sec->ceilingstat,
                          sec->ceilingxpanning, sec->ceilingypanning, x1))
        return;

    if (!(globalorientation&0x180))
    {
        y1 = umost[x1]; y2 = y1;
        for (x=x1; x<=x2; x++)
        {
            const int32_t twall = umost[x]-1;
            const int32_t bwall = min(uplc[x],dmost[x]);

            if (twall < bwall-1)
            {
                if (twall >= y2)
                {
                    while (y1 < y2-1) hline(x-1,++y1);
                    y1 = twall;
                }
                else
                {
                    while (y1 < twall) hline(x-1,++y1);
                    while (y1 > twall) lastx[y1--] = x;
                }
                while (y2 > bwall) hline(x-1,--y2);
                while (y2 < bwall) lastx[y2++] = x;
            }
            else
            {
                while (y1 < y2-1) hline(x-1,++y1);
                if (x == x2) { globalx2 += globaly2; globaly1 += globalx1; break; }
                y1 = umost[x+1]; y2 = y1;
            }
            globalx2 += globaly2; globaly1 += globalx1;
        }
        while (y1 < y2-1) hline(x2,++y1);
        faketimerhandler();
        return;
    }

    switch (globalorientation&0x180)
    {
    case 128:
        msethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 256:
        setup_blend(0, 0);
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 384:
        setup_blend(0, 0);
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    }

    y1 = umost[x1]; y2 = y1;
    for (x=x1; x<=x2; x++)
    {
        const int32_t twall = umost[x]-1;
        const int32_t bwall = min(uplc[x],dmost[x]);

        if (twall < bwall-1)
        {
            if (twall >= y2)
            {
                while (y1 < y2-1) slowhline(x-1,++y1);
                y1 = twall;
            }
            else
            {
                while (y1 < twall) slowhline(x-1,++y1);
                while (y1 > twall) lastx[y1--] = x;
            }
            while (y2 > bwall) slowhline(x-1,--y2);
            while (y2 < bwall) lastx[y2++] = x;
        }
        else
        {
            while (y1 < y2-1) slowhline(x-1,++y1);
            if (x == x2) { globalx2 += globaly2; globaly1 += globalx1; break; }
            y1 = umost[x+1]; y2 = y1;
        }
        globalx2 += globaly2; globaly1 += globalx1;
    }
    while (y1 < y2-1) slowhline(x2,++y1);
    faketimerhandler();
}


//
// florscan (internal)
//
static void florscan(int32_t x1, int32_t x2, int32_t sectnum)
{
     int32_t x, y1, y2;
     const usectortype *const sec = (usectortype *)&sector[sectnum];

     if (setup_globals_cf1(sec, sec->floorpal, globalposz-sec->floorz,
                           sec->floorpicnum, sec->floorshade, sec->floorstat,
                           sec->floorxpanning, sec->floorypanning, x1))
         return;

    if (!(globalorientation&0x180))
    {
        y1 = max(dplc[x1],umost[x1]); y2 = y1;
        for (x=x1; x<=x2; x++)
        {
            const int32_t twall = max(dplc[x],umost[x])-1;
            const int32_t bwall = dmost[x];

            if (twall < bwall-1)
            {
                if (twall >= y2)
                {
                    while (y1 < y2-1) hline(x-1,++y1);
                    y1 = twall;
                }
                else
                {
                    while (y1 < twall) hline(x-1,++y1);
                    while (y1 > twall) lastx[y1--] = x;
                }
                while (y2 > bwall) hline(x-1,--y2);
                while (y2 < bwall) lastx[y2++] = x;
            }
            else
            {
                while (y1 < y2-1) hline(x-1,++y1);
                if (x == x2) { globalx2 += globaly2; globaly1 += globalx1; break; }
                y1 = max(dplc[x+1],umost[x+1]); y2 = y1;
            }
            globalx2 += globaly2; globaly1 += globalx1;
        }
        while (y1 < y2-1) hline(x2,++y1);
        faketimerhandler();
        return;
    }

    switch (globalorientation&0x180)
    {
    case 128:
        msethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 256:
        setup_blend(0, 0);
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 384:
        setup_blend(0, 1);
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    }

    y1 = max(dplc[x1],umost[x1]); y2 = y1;
    for (x=x1; x<=x2; x++)
    {
        const int32_t twall = max(dplc[x],umost[x])-1;
        const int32_t bwall = dmost[x];

        if (twall < bwall-1)
        {
            if (twall >= y2)
            {
                while (y1 < y2-1) slowhline(x-1,++y1);
                y1 = twall;
            }
            else
            {
                while (y1 < twall) slowhline(x-1,++y1);
                while (y1 > twall) lastx[y1--] = x;
            }
            while (y2 > bwall) slowhline(x-1,--y2);
            while (y2 < bwall) lastx[y2++] = x;
        }
        else
        {
            while (y1 < y2-1) slowhline(x-1,++y1);
            if (x == x2) { globalx2 += globaly2; globaly1 += globalx1; break; }
            y1 = max(dplc[x+1],umost[x+1]); y2 = y1;
        }
        globalx2 += globaly2; globaly1 += globalx1;
    }
    while (y1 < y2-1) slowhline(x2,++y1);
    faketimerhandler();
}


//
// wallscan (internal)
//
static void wallscan(int32_t x1, int32_t x2,
                     const int16_t *uwal, const int16_t *dwal,
                     const int32_t *swal, const int32_t *lwal)
{
    int32_t x;
    intptr_t fpalookup;
    int32_t y1ve[4], y2ve[4];
    vec2s_t tsiz;
#ifdef MULTI_COLUMN_VLINE
    char bad;
    int32_t u4, d4, z;
    uintptr_t p;
#endif

#ifdef YAX_ENABLE
    if (g_nodraw)
        return;
#endif
    setgotpic(globalpicnum);
    if (globalshiftval < 0)
        return;

    if (x2 >= xdim)
        x2 = xdim-1;
    assert((unsigned)x1 < (unsigned)xdim);

    tsiz = tilesiz[globalpicnum];

    if ((tsiz.x <= 0) || (tsiz.y <= 0)) return;
    if ((uwal[x1] > ydimen) && (uwal[x2] > ydimen)) return;
    if ((dwal[x1] < 0) && (dwal[x2] < 0)) return;

    if (waloff[globalpicnum] == 0) loadtile(globalpicnum);

    tweak_tsizes(&tsiz);

    fpalookup = FP_OFF(palookup[globalpal]);

    setupvlineasm(globalshiftval);


    x = x1;
    while ((x <= x2) && (umost[x] > dmost[x]))
        x++;

#ifdef NONPOW2_YSIZE_ASM
    if (globalshiftval==0)
        goto do_vlineasm1;
#endif

#ifdef MULTI_COLUMN_VLINE
    for (; (x<=x2)&&((x+frameoffset)&3); x++)
    {
        y1ve[0] = max(uwal[x],umost[x]);
        y2ve[0] = min(dwal[x],dmost[x]);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup + getpalookupsh(mulscale16(swal[x],globvis));

        calc_bufplc(&bufplce[0], lwal[x], tsiz);
        calc_vplcinc(&vplce[0], &vince[0], swal, x, y1ve[0]);

        vlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0],x+frameoffset+ylookup[y1ve[0]]);
    }
    for (; x<=x2-3; x+=4)
    {
        bad = 0;
        for (z=3; z>=0; z--)
        {
            y1ve[z] = max(uwal[x+z],umost[x+z]);
            y2ve[z] = min(dwal[x+z],dmost[x+z])-1;
            if (y2ve[z] < y1ve[z]) { bad += pow2char[z]; continue; }

            calc_bufplc(&bufplce[z], lwal[x+z], tsiz);
            calc_vplcinc(&vplce[z], &vince[z], swal, x+z, y1ve[z]);
        }
        if (bad == 15) continue;

        palookupoffse[0] = fpalookup + getpalookupsh(mulscale16(swal[x],globvis));
        palookupoffse[3] = fpalookup + getpalookupsh(mulscale16(swal[x+3],globvis));

        if ((palookupoffse[0] == palookupoffse[3]) && ((bad&0x9) == 0))
        {
            palookupoffse[1] = palookupoffse[0];
            palookupoffse[2] = palookupoffse[0];
        }
        else
        {
            palookupoffse[1] = fpalookup + getpalookupsh(mulscale16(swal[x+1],globvis));
            palookupoffse[2] = fpalookup + getpalookupsh(mulscale16(swal[x+2],globvis));
        }

        u4 = max(max(y1ve[0],y1ve[1]),max(y1ve[2],y1ve[3]));
        d4 = min(min(y2ve[0],y2ve[1]),min(y2ve[2],y2ve[3]));

        if ((bad != 0) || (u4 >= d4))
        {
            if (!(bad&1)) prevlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0],vplce[0],bufplce[0],ylookup[y1ve[0]]+x+frameoffset+0);
            if (!(bad&2)) prevlineasm1(vince[1],palookupoffse[1],y2ve[1]-y1ve[1],vplce[1],bufplce[1],ylookup[y1ve[1]]+x+frameoffset+1);
            if (!(bad&4)) prevlineasm1(vince[2],palookupoffse[2],y2ve[2]-y1ve[2],vplce[2],bufplce[2],ylookup[y1ve[2]]+x+frameoffset+2);
            if (!(bad&8)) prevlineasm1(vince[3],palookupoffse[3],y2ve[3]-y1ve[3],vplce[3],bufplce[3],ylookup[y1ve[3]]+x+frameoffset+3);
            continue;
        }

        if (u4 > y1ve[0]) vplce[0] = prevlineasm1(vince[0],palookupoffse[0],u4-y1ve[0]-1,vplce[0],bufplce[0],ylookup[y1ve[0]]+x+frameoffset+0);
        if (u4 > y1ve[1]) vplce[1] = prevlineasm1(vince[1],palookupoffse[1],u4-y1ve[1]-1,vplce[1],bufplce[1],ylookup[y1ve[1]]+x+frameoffset+1);
        if (u4 > y1ve[2]) vplce[2] = prevlineasm1(vince[2],palookupoffse[2],u4-y1ve[2]-1,vplce[2],bufplce[2],ylookup[y1ve[2]]+x+frameoffset+2);
        if (u4 > y1ve[3]) vplce[3] = prevlineasm1(vince[3],palookupoffse[3],u4-y1ve[3]-1,vplce[3],bufplce[3],ylookup[y1ve[3]]+x+frameoffset+3);

        if (d4 >= u4) vlineasm4(d4-u4+1, (char *)(ylookup[u4]+x+frameoffset));

        p = x+frameoffset+ylookup[d4+1];
        if (y2ve[0] > d4) prevlineasm1(vince[0],palookupoffse[0],y2ve[0]-d4-1,vplce[0],bufplce[0],p+0);
        if (y2ve[1] > d4) prevlineasm1(vince[1],palookupoffse[1],y2ve[1]-d4-1,vplce[1],bufplce[1],p+1);
        if (y2ve[2] > d4) prevlineasm1(vince[2],palookupoffse[2],y2ve[2]-d4-1,vplce[2],bufplce[2],p+2);
        if (y2ve[3] > d4) prevlineasm1(vince[3],palookupoffse[3],y2ve[3]-d4-1,vplce[3],bufplce[3],p+3);
    }
#endif

#ifdef NONPOW2_YSIZE_ASM
do_vlineasm1:
#endif
    for (; x<=x2; x++)
    {
        y1ve[0] = max(uwal[x],umost[x]);
        y2ve[0] = min(dwal[x],dmost[x]);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup + getpalookupsh(mulscale16(swal[x],globvis));

        calc_bufplc(&bufplce[0], lwal[x], tsiz);
        calc_vplcinc(&vplce[0], &vince[0], swal, x, y1ve[0]);

#ifdef NONPOW2_YSIZE_ASM
        if (globalshiftval==0)
            vlineasm1nonpow2(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0],x+frameoffset+ylookup[y1ve[0]]);
        else
#endif
        vlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0],x+frameoffset+ylookup[y1ve[0]]);
    }

    faketimerhandler();
}

//
// transmaskvline (internal)
//
static void transmaskvline(int32_t x)
{
    if ((unsigned)x >= (unsigned)xdimen) return;

    int32_t const y1v = max(uwall[x],startumost[x+windowxy1.x]-windowxy1.y);
    int32_t const y2v = min(dwall[x],startdmost[x+windowxy1.x]-windowxy1.y) - 1;

    if (y2v < y1v) return;

    intptr_t palookupoffs = FP_OFF(palookup[globalpal]) + getpalookupsh(mulscale16(swall[x],globvis));

    vec2s_t const ntsiz = { (int16_t)-tilesiz[globalpicnum].x, (int16_t)-tilesiz[globalpicnum].y };
    intptr_t bufplc;
    calc_bufplc(&bufplc, lwall[x], ntsiz);
    uint32_t vplc;
    int32_t vinc;
    calc_vplcinc(&vplc, &vinc, swall, x, y1v);

    intptr_t p = ylookup[y1v]+x+frameoffset;

#ifdef NONPOW2_YSIZE_ASM
    if (globalshiftval==0)
        tvlineasm1nonpow2(vinc,palookupoffs,y2v-y1v,vplc,bufplc,p);
    else
#endif
    tvlineasm1(vinc,palookupoffs,y2v-y1v,vplc,bufplc,p);
}


//
// transmaskvline2 (internal)
//
#ifdef MULTI_COLUMN_VLINE
static void transmaskvline2(int32_t x)
{
    if ((unsigned)x >= (unsigned)xdimen) return;
    if (x == xdimen-1) { transmaskvline(x); return; }

    int32_t y1ve[2], y2ve[2];
    int32_t x2 = x+1;

    y1ve[0] = max(uwall[x],startumost[x+windowxy1.x]-windowxy1.y);
    y2ve[0] = min(dwall[x],startdmost[x+windowxy1.x]-windowxy1.y)-1;
    if (y2ve[0] < y1ve[0]) { transmaskvline(x2); return; }
    y1ve[1] = max(uwall[x2],startumost[x2+windowxy1.x]-windowxy1.y);
    y2ve[1] = min(dwall[x2],startdmost[x2+windowxy1.x]-windowxy1.y)-1;
    if (y2ve[1] < y1ve[1]) { transmaskvline(x); return; }

    palookupoffse[0] = FP_OFF(palookup[globalpal]) + getpalookupsh(mulscale16(swall[x],globvis));
    palookupoffse[1] = FP_OFF(palookup[globalpal]) + getpalookupsh(mulscale16(swall[x2],globvis));

    setuptvlineasm2(globalshiftval,palookupoffse[0],palookupoffse[1]);

    vec2s_t const ntsiz = { (int16_t)-tilesiz[globalpicnum].x, (int16_t)-tilesiz[globalpicnum].y };

    calc_bufplc(&bufplce[0], lwall[x], ntsiz);
    calc_bufplc(&bufplce[1], lwall[x2], ntsiz);
    calc_vplcinc(&vplce[0], &vince[0], swall, x, y1ve[0]);
    calc_vplcinc(&vplce[1], &vince[1], swall, x2, y1ve[1]);

    int32_t const y1 = max(y1ve[0],y1ve[1]);
    int32_t const y2 = min(y2ve[0],y2ve[1]);

    uintptr_t p = x+frameoffset;

    if (y1ve[0] != y1ve[1])
    {
        if (y1ve[0] < y1)
            vplce[0] = tvlineasm1(vince[0],palookupoffse[0],y1-y1ve[0]-1,vplce[0],bufplce[0],ylookup[y1ve[0]]+p);
        else
            vplce[1] = tvlineasm1(vince[1],palookupoffse[1],y1-y1ve[1]-1,vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
    }

    if (y2 > y1)
    {
        asm1 = vince[1];
        asm2 = ylookup[y2]+p+1;
        tvlineasm2(vplce[1],vince[0],bufplce[0],bufplce[1],vplce[0],ylookup[y1]+p);
    }
    else
    {
        asm1 = vplce[0];
        asm2 = vplce[1];
    }

    if (y2ve[0] > y2ve[1])
        tvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y2-1,asm1,bufplce[0],ylookup[y2+1]+p);
    else if (y2ve[0] < y2ve[1])
        tvlineasm1(vince[1],palookupoffse[1],y2ve[1]-y2-1,asm2,bufplce[1],ylookup[y2+1]+p+1);

    faketimerhandler();
}
#endif

//
// transmaskwallscan (internal)
//
static void transmaskwallscan(int32_t x1, int32_t x2, int32_t saturatevplc)
{
    setgotpic(globalpicnum);

    Bassert(globalshiftval>=0 || ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0)));
    // globalshiftval<0 implies following condition
    if ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0))
        return;

    if (waloff[globalpicnum] == 0) loadtile(globalpicnum);

    setuptvlineasm(globalshiftval, saturatevplc);

    int32_t x = x1;
    while ((x <= x2) && (startumost[x+windowxy1.x] > startdmost[x+windowxy1.x]))
        ++x;

#ifndef ENGINE_USING_A_C
    if (globalshiftval==0)
    {
        while (x <= x2) transmaskvline(x++);
    }
    else
#endif
    {
#ifdef MULTI_COLUMN_VLINE
        if ((x <= x2) && (x&1)) transmaskvline(x++);
        while (x < x2) transmaskvline2(x), x += 2;
#endif
        while (x <= x2) transmaskvline(x++);
    }

    faketimerhandler();
}


////////// NON-power-of-two replacements for mhline/thline, adapted from a.c //////////
#if defined(__GNUC__) && defined(__i386__) && !defined(NOASM)
// from pragmas.h
# define ourdivscale32(d,b) \
    ({ int32_t __d=(d), __b=(b), __r; \
       __asm__ __volatile__ ("xorl %%eax, %%eax; divl %%ebx" \
        : "=a" (__r), "=d" (__d) : "d" (__d), "b" (__b) : "cc"); \
     __r; })
#else
# define ourdivscale32(d,b) divscale32(d,b)
#endif

// cntup16>>16 iterations

static void nonpow2_mhline(intptr_t bufplc, uint32_t bx, int32_t cntup16, uint32_t by, char *p)
{
    char ch;

    const char *const A_C_RESTRICT buf = (char *)bufplc;
    const char *const A_C_RESTRICT pal = (char *)asm3;

    const uint32_t xdiv = globalxspan > 1 ? (uint32_t)ourdivscale32(1, globalxspan) : UINT32_MAX;
    const uint32_t ydiv = globalyspan > 1 ? (uint32_t)ourdivscale32(1, globalyspan) : UINT32_MAX;
    const uint32_t yspan = globalyspan;
    const int32_t xinc = asm1, yinc = asm2;

    for (cntup16>>=16; cntup16>0; cntup16--)
    {
        ch = buf[(divideu32(bx, xdiv))*yspan + divideu32(by, ydiv)];

        if (ch != 255) *p = pal[ch];
        bx += xinc;
        by += yinc;
        p++;
    }
}

// cntup16>>16 iterations
static void nonpow2_thline(intptr_t bufplc, uint32_t bx, int32_t cntup16, uint32_t by, char *p)
{
    char ch;

    const char *const A_C_RESTRICT buf = (char *)bufplc;
    const char *const A_C_RESTRICT pal = (char *)asm3;
    const char *const A_C_RESTRICT trans = getblendtab(globalblend);

    const uint32_t xdiv = globalxspan > 1 ? (uint32_t)ourdivscale32(1, globalxspan) : UINT32_MAX;
    const uint32_t ydiv = globalyspan > 1 ? (uint32_t)ourdivscale32(1, globalyspan) : UINT32_MAX;
    const uint32_t yspan = globalyspan;
    const int32_t xinc = asm1, yinc = asm2;

    if (globalorientation&512)
    {
        for (cntup16>>=16; cntup16>0; cntup16--)
        {
            ch = buf[divideu32(bx, xdiv)*yspan + divideu32(by, ydiv)];
            if (ch != 255) *p = trans[(*p)|(pal[ch]<<8)];
            bx += xinc;
            by += yinc;
            p++;
        }
    }
    else
    {
        for (cntup16>>=16; cntup16>0; cntup16--)
        {
            ch = buf[divideu32(bx, xdiv)*yspan + divideu32(by, ydiv)];
            if (ch != 255) *p = trans[((*p)<<8)|pal[ch]];
            bx += xinc;
            by += yinc;
            p++;
        }
    }
}
////////// END non-power-of-two replacements //////////

//
// ceilspritehline (internal)
//
static void ceilspritehline(int32_t x2, int32_t y)
{
    int32_t x1, v, bx, by;

    //x = x1 + (x2-x1)t + (y1-y2)u  ~  x = 160v
    //y = y1 + (y2-y1)t + (x2-x1)u  ~  y = (scrx-160)v
    //z = z1 = z2                   ~  z = posz + (scry-horiz)v

    x1 = lastx[y]; if (x2 < x1) return;

    v = mulscale20(globalzd,horizlookup[y-globalhoriz+horizycent]);
    bx = (uint32_t)mulscale14(globalx2*x1+globalx1,v) + globalxpanning;
    by = (uint32_t)mulscale14(globaly2*x1+globaly1,v) + globalypanning;
    asm1 = mulscale14(globalx2,v);
    asm2 = mulscale14(globaly2,v);

    asm3 = FP_OFF(palookup[globalpal]) + getpalookupsh(mulscale28(klabs(v),globvis));

    if (globalispow2)
    {
        if ((globalorientation&2) == 0)
            mhline(globalbufplc,bx,(x2-x1)<<16,0L,by,ylookup[y]+x1+frameoffset);
        else
            thline(globalbufplc,bx,(x2-x1)<<16,0L,by,ylookup[y]+x1+frameoffset);
    }
    else
    {
        if ((globalorientation&2) == 0)
            nonpow2_mhline(globalbufplc,bx,(x2-x1)<<16,by,(char *)(ylookup[y]+x1+frameoffset));
        else
            nonpow2_thline(globalbufplc,bx,(x2-x1)<<16,by,(char *)(ylookup[y]+x1+frameoffset));
    }
}


//
// ceilspritescan (internal)
//
static void ceilspritescan(int32_t x1, int32_t x2)
{
    int32_t y1 = uwall[x1];
    int32_t y2 = y1;

    for (bssize_t x=x1; x<=x2; ++x)
    {
        const int32_t twall = uwall[x]-1;
        const int32_t bwall = dwall[x];

        if (twall < bwall-1)
        {
            if (twall >= y2)
            {
                while (y1 < y2-1) ceilspritehline(x-1,++y1);
                y1 = twall;
            }
            else
            {
                while (y1 < twall) ceilspritehline(x-1,++y1);
                while (y1 > twall) lastx[y1--] = x;
            }
            while (y2 > bwall) ceilspritehline(x-1,--y2);
            while (y2 < bwall) lastx[y2++] = x;
        }
        else
        {
            while (y1 < y2-1) ceilspritehline(x-1,++y1);
            if (x == x2) break;
            y1 = uwall[x+1]; y2 = y1;
        }
    }
    while (y1 < y2-1) ceilspritehline(x2,++y1);
    faketimerhandler();
}

////////// translucent slope vline, based on a-c.c's slopevlin //////////
static int32_t gglogx, gglogy, ggpinc;
static char *ggbuf, *ggpal;

#ifdef ENGINE_USING_A_C
extern int32_t gpinc;
#endif

static inline void setupslopevlin_alsotrans(int32_t logylogx, intptr_t bufplc, int32_t pinc)
{
#ifdef ENGINE_USING_A_C
    sethlinesizes(logylogx&255, logylogx>>8, bufplc);
    gpinc = pinc;
#else
    setupslopevlin(logylogx, bufplc, pinc);
#endif
    gglogx = (logylogx&255); gglogy = (logylogx>>8);
    ggbuf = (char *)bufplc; ggpinc = pinc;
    ggpal = palookup[globalpal] + getpalookupsh(0);
}

// cnt iterations
static void tslopevlin(uint8_t *p, const intptr_t *slopalptr, bssize_t cnt, int32_t bx, int32_t by)
{
    const char *const A_C_RESTRICT buf = ggbuf;
    const char *const A_C_RESTRICT pal = ggpal;
    const char *const A_C_RESTRICT trans = getblendtab(0);
    const int32_t bzinc = (asm1>>3), pinc = ggpinc;

    const int32_t transmode = (globalorientation&128);
    const uint32_t xtou = globalx3, ytov = globaly3;
    const int32_t logx = gglogx, logy = gglogy;

    int32_t bz = asm3;

    do
    {
        int const i = (sloptable[(bz>>6)+8192]); bz += bzinc;
        uint32_t u = bx + xtou*i;
        uint32_t v = by + ytov*i;
        uint8_t ch = *(uint8_t *)(slopalptr[0] + buf[((u>>(32-logx))<<logy)+(v>>(32-logy))]);

        if (ch != 255)
            *p = trans[transmode ? *p|(pal[ch]<<8) : (*p<<8)|pal[ch]];

        slopalptr--;
        p += pinc;
    }
    while (--cnt);
}

//
// grouscan (internal)
//
#define BITSOFPRECISION 3  //Don't forget to change this in A.ASM also!
static void grouscan(int32_t dax1, int32_t dax2, int32_t sectnum, char dastat)
{
    int32_t i, l, x, y, dx, dy, wx, wy, y1, y2, daz;
    int32_t daslope, dasqr;
    int32_t shoffs, m1, m2;
    intptr_t *mptr1, *mptr2, j;

    // Er, yes, they're not global anymore:
    int32_t globalx, globaly, globalz, globalzx;

    const usectortype *const sec = (usectortype *)&sector[sectnum];
    const uwalltype *wal;

    if (dastat == 0)
    {
        if (globalposz <= getceilzofslope(sectnum,globalposx,globalposy))
            return;  //Back-face culling
        globalorientation = sec->ceilingstat;
        globalpicnum = sec->ceilingpicnum;
        globalshade = sec->ceilingshade;
        globalpal = sec->ceilingpal;
        daslope = sec->ceilingheinum;
        daz = sec->ceilingz;
    }
    else
    {
        if (globalposz >= getflorzofslope(sectnum,globalposx,globalposy))
            return;  //Back-face culling
        globalorientation = sec->floorstat;
        globalpicnum = sec->floorpicnum;
        globalshade = sec->floorshade;
        globalpal = sec->floorpal;
        daslope = sec->floorheinum;
        daz = sec->floorz;
    }

    DO_TILE_ANIM(globalpicnum, sectnum);
    setgotpic(globalpicnum);
    if ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0)) return;
    if (waloff[globalpicnum] == 0) loadtile(globalpicnum);

    wal = (uwalltype *)&wall[sec->wallptr];
    wx = wall[wal->point2].x - wal->x;
    wy = wall[wal->point2].y - wal->y;
    dasqr = krecipasm(nsqrtasm(uhypsq(wx,wy)));
    i = mulscale21(daslope,dasqr);
    wx *= i; wy *= i;

    globalx = -mulscale19(singlobalang,xdimenrecip);
    globaly = mulscale19(cosglobalang,xdimenrecip);
    globalx1 = (globalposx<<8);
    globaly1 = -(globalposy<<8);
    i = (dax1-halfxdimen)*xdimenrecip;
    globalx2 = mulscale16(cosglobalang<<4,viewingrangerecip) - mulscale27(singlobalang,i);
    globaly2 = mulscale16(singlobalang<<4,viewingrangerecip) + mulscale27(cosglobalang,i);
    globalzd = (xdimscale<<9);
    globalzx = -dmulscale17(wx,globaly2,-wy,globalx2) + mulscale10(1-globalhoriz,globalzd);
    globalz = -dmulscale25(wx,globaly,-wy,globalx);

    if (globalorientation&64)  //Relative alignment
    {
        dx = mulscale14(wall[wal->point2].x-wal->x,dasqr);
        dy = mulscale14(wall[wal->point2].y-wal->y,dasqr);

        i = nsqrtasm(daslope*daslope+16777216);

        x = globalx; y = globaly;
        globalx = dmulscale16(x,dx,y,dy);
        globaly = mulscale12(dmulscale16(-y,dx,x,dy),i);

        x = ((wal->x-globalposx)<<8); y = ((wal->y-globalposy)<<8);
        globalx1 = dmulscale16(-x,dx,-y,dy);
        globaly1 = mulscale12(dmulscale16(-y,dx,x,dy),i);

        x = globalx2; y = globaly2;
        globalx2 = dmulscale16(x,dx,y,dy);
        globaly2 = mulscale12(dmulscale16(-y,dx,x,dy),i);
    }
    if (globalorientation&0x4)
    {
        i = globalx; globalx = -globaly; globaly = -i;
        i = globalx1; globalx1 = globaly1; globaly1 = i;
        i = globalx2; globalx2 = -globaly2; globaly2 = -i;
    }
    if (globalorientation&0x10) { globalx1 = -globalx1, globalx2 = -globalx2, globalx = -globalx; }
    if (globalorientation&0x20) { globaly1 = -globaly1, globaly2 = -globaly2, globaly = -globaly; }

    daz = dmulscale9(wx,globalposy-wal->y,-wy,globalposx-wal->x) + ((daz-globalposz)<<8);
    globalx2 = mulscale20(globalx2,daz); globalx = mulscale28(globalx,daz);
    globaly2 = mulscale20(globaly2,-daz); globaly = mulscale28(globaly,-daz);

    i = 8-(picsiz[globalpicnum]&15); j = 8-(picsiz[globalpicnum]>>4);
    if (globalorientation&8) { i++; j++; }
    globalx1 <<= (i+12); globalx2 <<= i; globalx <<= i;
    globaly1 <<= (j+12); globaly2 <<= j; globaly <<= j;

    if (dastat == 0)
    {
        globalx1 += (uint32_t)sec->ceilingxpanning<<24;
        globaly1 += (uint32_t)sec->ceilingypanning<<24;
    }
    else
    {
        globalx1 += (uint32_t)sec->floorxpanning<<24;
        globaly1 += (uint32_t)sec->floorypanning<<24;
    }

    asm1 = -(globalzd>>(16-BITSOFPRECISION));

    {
        int32_t vis = globalvisibility;
        int64_t lvis;

        if (sec->visibility != 0) vis = mulscale4(vis, (uint8_t)(sec->visibility+16));
        lvis = ((uint64_t)vis*daz) >> 13;  // NOTE: lvis can be negative now!
        lvis = (lvis * xdimscale) >> 16;
        globvis = lvis;
    }

    j = FP_OFF(palookup[globalpal]);

    setupslopevlin_alsotrans((picsiz[globalpicnum]&15) + ((picsiz[globalpicnum]>>4)<<8),
                             waloff[globalpicnum],-ylookup[1]);

    l = (globalzd>>16);

    int32_t const shinc = mulscale16(globalz,xdimenscale);

    shoffs = (shinc > 0) ? (4 << 15) : ((16380 - ydimen) << 15);  // JBF: was 2044
    y1     = (dastat == 0) ? umost[dax1] : max(umost[dax1], dplc[dax1]);

    m1 = mulscale16(y1,globalzd) + (globalzx>>6);
    //Avoid visibility overflow by crossing horizon
    m1 += klabs((int32_t) (globalzd>>16));
    m2 = m1+l;
    mptr1 = (intptr_t *)&slopalookup[y1+(shoffs>>15)]; mptr2 = mptr1+1;

    for (x=dax1; x<=dax2; x++)
    {
        if (dastat == 0) { y1 = umost[x]; y2 = min(dmost[x],uplc[x])-1; }
        else { y1 = max(umost[x],dplc[x]); y2 = dmost[x]-1; }
        if (y1 <= y2)
        {
            intptr_t *nptr1 = &slopalookup[y1+(shoffs>>15)];
            intptr_t *nptr2 = &slopalookup[y2+(shoffs>>15)];

            while (nptr1 <= mptr1)
            {
                *mptr1-- = j + getpalookupsh(mulscale24(krecipasm(m1),globvis));
                m1 -= l;
            }
            while (nptr2 >= mptr2)
            {
                *mptr2++ = j + getpalookupsh(mulscale24(krecipasm(m2),globvis));
                m2 += l;
            }

            globalx3 = (globalx2>>10);
            globaly3 = (globaly2>>10);
            asm3 = mulscale16(y2,globalzd) + (globalzx>>6);
            if ((globalorientation&256)==0)
                slopevlin(ylookup[y2]+x+frameoffset,krecipasm(asm3>>3),(intptr_t)nptr2,y2-y1+1,globalx1,globaly1);
            else
                tslopevlin((uint8_t *)(ylookup[y2]+x+frameoffset),nptr2,y2-y1+1,globalx1,globaly1);

            if ((x&15) == 0) faketimerhandler();
        }
        globalx2 += globalx;
        globaly2 += globaly;
        globalzx += globalz;
        shoffs += shinc;
    }
}


//
// parascan (internal)
//
static void parascan(int32_t dax1, int32_t dax2, int32_t sectnum, char dastat, int32_t bunch)
{
    usectortype *sec;
    int32_t j, k, l, m, n, x, z, wallnum, nextsectnum, globalhorizbak;
    int16_t *topptr, *botptr;

    int32_t logtilesizy, tsizy;

    UNREFERENCED_PARAMETER(dax1);
    UNREFERENCED_PARAMETER(dax2);

    sectnum = thesector[bunchfirst[bunch]]; sec = (usectortype *)&sector[sectnum];

    globalhorizbak = globalhoriz;
    globvis = globalpisibility;
    //globalorientation = 0L;
    if (sec->visibility != 0)
        globvis = mulscale4(globvis, (uint8_t)(sec->visibility+16));

    if (dastat == 0)
    {
        globalpal = sec->ceilingpal;
        globalpicnum = sec->ceilingpicnum;
        globalshade = (int32_t)sec->ceilingshade;
        globalxpanning = (int32_t)sec->ceilingxpanning;
        globalypanning = (int32_t)sec->ceilingypanning;
        topptr = umost;
        botptr = uplc;
    }
    else
    {
        globalpal = sec->floorpal;
        globalpicnum = sec->floorpicnum;
        globalshade = (int32_t)sec->floorshade;
        globalxpanning = (int32_t)sec->floorxpanning;
        globalypanning = (int32_t)sec->floorypanning;
        topptr = dplc;
        botptr = dmost;
    }

    if ((unsigned)globalpicnum >= MAXTILES) globalpicnum = 0;
    DO_TILE_ANIM(globalpicnum, sectnum);
    setgotpic(globalpicnum);

    logtilesizy = (picsiz[globalpicnum]>>4);
    tsizy = tilesiz[globalpicnum].y;

    if (tsizy==0)
        return;


    int32_t dapyscale, dapskybits, dapyoffs, daptileyscale;
    int8_t const * const dapskyoff = getpsky(globalpicnum, &dapyscale, &dapskybits, &dapyoffs, &daptileyscale);

    globalshiftval = logtilesizy;

    // before proper non-power-of-two tilesizy drawing
    if (oldnonpow2() && pow2long[logtilesizy] != tsizy)
        globalshiftval++;
#ifdef CLASSIC_NONPOW2_YSIZE_WALLS
    // non power-of-two y size textures!
    if ((!oldnonpow2() && pow2long[logtilesizy] != tsizy) || tsizy >= 512)
    {
        globaltilesizy = tsizy;
        globalyscale = 65536 / tsizy;
        globalshiftval = 0;
        globalzd = divscale32(((tsizy>>1)+dapyoffs), tsizy) + ((uint32_t)globalypanning<<24);
    }
    else
#endif
    {
        globalshiftval = 32-globalshiftval;
        globalyscale = (8<<(globalshiftval-19));
        globalzd = (((tsizy>>1)+dapyoffs)<<globalshiftval) + ((uint32_t)globalypanning<<24);
    }
    globalyscale = divscale16(globalyscale,daptileyscale);

    //if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;

    if (dapyscale != 65536)
        globalhoriz = mulscale16(globalhoriz-(ydimen>>1),dapyscale) + (ydimen>>1);

    k = 11 - (picsiz[globalpicnum]&15) - dapskybits;

    // WGR2 SVN: select new episode after playing wgmicky1 with Polymer
    //  (maybe switched to classic earlier).
    //  --> rendmode==0, glrendermode == REND_POLYMER, we end up with globalpicnum==266,
    //      picsiz...==9 and dapskybits==3
    // FIXME ?
    if (k < 0)
        k = 0;

    x = -1;

    for (z=bunchfirst[bunch]; z>=0; z=bunchp2[z])
    {
        wallnum = thewall[z]; nextsectnum = wall[wallnum].nextsector;

        if (nextsectnum >= 0)  //else negative array access
        {
            if (dastat == 0) j = sector[nextsectnum].ceilingstat;
            else j = sector[nextsectnum].floorstat;
        }

        if ((nextsectnum < 0) || (wall[wallnum].cstat&32) || ((j&1) == 0))
        {
            if (x == -1) x = xb1[z];

            if (parallaxtype == 0 || no_radarang2)
            {
                n = mulscale16(xdimenrecip,viewingrange);
                for (j=xb1[z]; j<=xb2[z]; j++)
                    lplc[j] = ((mulscale23(j-halfxdimen,n)+globalang)&2047)>>k;
            }
            else
            {
                for (j=xb1[z]; j<=xb2[z]; j++)
                    lplc[j] = ((radarang2[j]+globalang)&2047)>>k;
            }

            if (parallaxtype == 2 && !no_radarang2)
            {
                n = mulscale16(xdimscale,viewingrange);
                for (j=xb1[z]; j<=xb2[z]; j++)
                    swplc[j] = mulscale14(sintable[(radarang2[j]+512)&2047],n);
            }
            else
                clearbuf(&swplc[xb1[z]],xb2[z]-xb1[z]+1,mulscale16(xdimscale,viewingrange));
        }
        else if (x >= 0)
        {
            l = globalpicnum; m = (picsiz[globalpicnum]&15);
            globalpicnum = l + dapskyoff[lplc[x]>>m];

            if (((lplc[x]^lplc[xb1[z]-1])>>m) == 0)
                wallscan(x,xb1[z]-1,topptr,botptr,swplc,lplc);
            else
            {
                j = x;
                while (x < xb1[z])
                {
                    n = l + dapskyoff[lplc[x]>>m];
                    if (n != globalpicnum)
                    {
                        wallscan(j,x-1,topptr,botptr,swplc,lplc);
                        j = x;
                        globalpicnum = n;
                    }
                    x++;
                }
                if (j < x)
                    wallscan(j,x-1,topptr,botptr,swplc,lplc);
            }

            globalpicnum = l;
            x = -1;
        }
    }

    if (x >= 0)
    {
        l = globalpicnum; m = (picsiz[globalpicnum]&15);
        globalpicnum = l + dapskyoff[lplc[x]>>m];

        if (((lplc[x]^lplc[xb2[bunchlast[bunch]]])>>m) == 0)
            wallscan(x,xb2[bunchlast[bunch]],topptr,botptr,swplc,lplc);
        else
        {
            j = x;
            while (x <= xb2[bunchlast[bunch]])
            {
                n = l + dapskyoff[lplc[x]>>m];
                if (n != globalpicnum)
                {
                    wallscan(j,x-1,topptr,botptr,swplc,lplc);
                    j = x;
                    globalpicnum = n;
                }
                x++;
            }
            if (j <= x)
                wallscan(j,x-1,topptr,botptr,swplc,lplc);
        }
        globalpicnum = l;
    }
    globalhoriz = globalhorizbak;
}


// set orientation, panning, shade, pal; picnum
static void setup_globals_wall1(const uwalltype *wal, int32_t dapicnum)
{
    globalorientation = wal->cstat;

    globalpicnum = dapicnum;
    if ((unsigned)globalpicnum >= MAXTILES) globalpicnum = 0;
    DO_TILE_ANIM(globalpicnum, 0);

    globalxpanning = wal->xpanning;
    globalypanning = wal->ypanning;

    globalshade = wal->shade;
    globalpal = wal->pal;
    if (palookup[globalpal] == NULL) globalpal = 0;    // JBF: fixes crash
}

static void setup_globals_wall2(const uwalltype *wal, uint8_t secvisibility, int32_t topzref, int32_t botzref)
{
    const int32_t logtilesizy = (picsiz[globalpicnum]>>4);
    const int32_t tsizy = tilesiz[globalpicnum].y;

    if (tsizy==0)
    {
        globalshiftval = -1;
        return;
    }

    globvis = globalvisibility;
    if (secvisibility != 0)
        globvis = mulscale4(globvis, (uint8_t)(secvisibility+16));

    globalshiftval = logtilesizy;

    // before proper non-power-of-two tilesizy drawing
    if (oldnonpow2() && pow2long[logtilesizy] != tsizy)
        globalshiftval++;
#ifdef CLASSIC_NONPOW2_YSIZE_WALLS
    // non power-of-two y size textures!
    if ((!oldnonpow2() && pow2long[logtilesizy] != tsizy) || tsizy >= 512)
    {
        globaltilesizy = tsizy;
        globalyscale = divscale13(wal->yrepeat, tsizy);
        globalshiftval = 0;
    }
    else
#endif
    {
        // globalshiftval==13 --> globalshiftval==19
        //  ==> upper texture y size limit *here* = 8192
        globalshiftval = 32-globalshiftval;
        globalyscale = wal->yrepeat<<(globalshiftval-19);
    }

    if ((globalorientation&4) == 0)
        globalzd = (((int64_t)(globalposz-topzref)*globalyscale)<<8);
    else  // bottom-aligned
        globalzd = (((int64_t)(globalposz-botzref)*globalyscale)<<8);
    globalzd = (uint32_t)globalzd + (globalypanning<<24);

    if (globalorientation&256)  // y-flipped
        globalyscale = -globalyscale, globalzd = -(inthi_t)globalzd;
}


/* _______________
 * X umost #######
 * ###### ________
 * ______/
 * X dwall
 *
 * ________
 * X uwall \______
 * ///////////////
 * _______________
 * X dmost
 */

#ifdef YAX_ENABLE
// returns: should dmost be raised when drawing a "ceiling wall"?
static int32_t should_clip_cwall(int32_t x1, int32_t x2)
{
    int32_t x;

    if (yax_globallev <= YAX_MAXDRAWS)
        return 1;

    for (x=x1; x<=x2; x++)
        if (dwall[x] < dmost[x] || uplc[x] < dmost[x])
            return 1;

    return 0;
}

// returns: should umost be lowered when drawing a "floor wall"?
static int32_t should_clip_fwall(int32_t x1, int32_t x2)
{
    int32_t x;

    if (yax_globallev >= YAX_MAXDRAWS)
        return 1;

    for (x=x1; x<=x2; x++)
        if (uwall[x] > umost[x] || dplc[x] > umost[x])
            return 1;

    return 0;
}
#endif

//
// drawalls (internal)
//
static void drawalls(int32_t bunch)
{
    int32_t i, x;

    int32_t z = bunchfirst[bunch];

    const int32_t sectnum = thesector[z];
    const usectortype *const sec = (usectortype *)&sector[sectnum];

    uint8_t andwstat1 = 0xff, andwstat2 = 0xff;

    for (; z>=0; z=bunchp2[z]) //uplc/dplc calculation
    {
        andwstat1 &= wallmost(uplc,z,sectnum,(uint8_t)0);
        andwstat2 &= wallmost(dplc,z,sectnum,(uint8_t)1);
    }

#ifdef YAX_ENABLE
    if (g_nodraw)
    {
        int32_t baselevp, checkcf;
        int16_t bn[2];
# if 0
        int32_t obunchchk = (1 && yax_globalbunch>=0 &&
                             haveymost[yax_globalbunch>>3]&(1<<(yax_globalbunch&7)));

        // if (obunchchk)
        const int32_t x2 = yax_globalbunch*xdimen;
# endif
        baselevp = (yax_globallev == YAX_MAXDRAWS);

        yax_getbunches(sectnum, &bn[0], &bn[1]);
        checkcf = (bn[0]>=0) + ((bn[1]>=0)<<1);
        if (!baselevp)
            checkcf &= (1<<yax_globalcf);

        if ((andwstat1&3) == 3)  // ceilings clipped
            checkcf &= ~1;
        if ((andwstat2&12) == 12)  // floors clipped
            checkcf &= ~2;

        for (i=0; i<2; i++)
            if (checkcf&(1<<i))
            {
                if ((haveymost[bn[i]>>3]&(1<<(bn[i]&7)))==0)
                {
                    // init yax *most arrays for that bunch
                    haveymost[bn[i]>>3] |= (1<<(bn[i]&7));
                    for (x=xdimen*bn[i]; x<xdimen*(bn[i]+1); x++)
                    {
                        yumost[x] = ydimen;
                        ydmost[x] = 0;
                    }
                }

                const int32_t x1 = bn[i]*xdimen;

                for (x=x1+xb1[bunchfirst[bunch]]; x<=x1+xb2[bunchlast[bunch]]; x++)
                {
                    if (i==YAX_CEILING)
                    {
                        yumost[x] = min(yumost[x], umost[x-x1]);
                        ydmost[x] = max(ydmost[x], min(dmost[x-x1], uplc[x-x1]));
                    }
                    else
                    {
                        yumost[x] = min(yumost[x], max(umost[x-x1], dplc[x-x1]));
                        ydmost[x] = max(ydmost[x], dmost[x-x1]);
                    }
# if 0
                    if (obunchchk)
                    {
                        yumost[x] = max(yumost[x], yumost[x-x1+x2]);
                        ydmost[x] = min(ydmost[x], ydmost[x-x1+x2]);
                    }
# endif
                }
            }
    }
    else
#endif
    {
        if ((andwstat1&3) != 3)     //draw ceilings
#ifdef YAX_ENABLE
            // this is to prevent double-drawing of translucent masked ceilings
            if (r_tror_nomaskpass==0 || yax_globallev==YAX_MAXDRAWS || (sec->ceilingstat&256)==0 ||
                yax_nomaskpass==1 || !(yax_gotsector[sectnum>>3]&(1<<(sectnum&7))))
#endif
        {
            if ((sec->ceilingstat&3) == 2)
                grouscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum,0);
            else if ((sec->ceilingstat&1) == 0)
                ceilscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum);
            else
                parascan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum,0,bunch);
        }

        if ((andwstat2&12) != 12)   //draw floors
#ifdef YAX_ENABLE
            // this is to prevent double-drawing of translucent masked floors
            if (r_tror_nomaskpass==0 || yax_globallev==YAX_MAXDRAWS || (sec->floorstat&256)==0 ||
                yax_nomaskpass==1 || !(yax_gotsector[sectnum>>3]&(1<<(sectnum&7))))
#endif
        {
            if ((sec->floorstat&3) == 2)
                grouscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum,1);
            else if ((sec->floorstat&1) == 0)
                florscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum);
            else
                parascan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum,1,bunch);
        }
    }


    //DRAW WALLS SECTION!
    for (z=bunchfirst[bunch]; z>=0; z=bunchp2[z])
    {
        const int32_t x1 = xb1[z], x2 = xb2[z];

        if (umost[x2] >= dmost[x2])
        {
            for (x=x1; x<x2; x++)
                if (umost[x] < dmost[x])
                    break;
            if (x >= x2)
            {
                smostwall[smostwallcnt] = z;
                smostwalltype[smostwallcnt] = 0;
                smostwallcnt++;
                continue;
            }
        }

        const int32_t wallnum = thewall[z];
        const uwalltype *const wal = (uwalltype *)&wall[wallnum];

        const int32_t nextsectnum = wal->nextsector;
        const usectortype *const nextsec = nextsectnum>=0 ? (usectortype *)&sector[nextsectnum] : NULL;

        int32_t gotswall = 0;

        const int32_t startsmostwallcnt = smostwallcnt;
        const int32_t startsmostcnt = smostcnt;

        if (searchit == 2 && (searchx >= x1 && searchx <= x2))
        {
            if (searchy <= uplc[searchx]
#ifdef YAX_ENABLE
                && umost[searchx] <= searchy && getceilzofslope(sectnum, globalposx, globalposy) <= globalposz
                && (yax_getbunch(sectnum, YAX_CEILING) < 0 || showinvisibility || (sec->ceilingstat&(256+128)) || klabs(yax_globallev-YAX_MAXDRAWS)==YAX_MAXDRAWS)
#endif
                ) //ceiling
            {
                searchsector = sectnum; searchwall = wallnum;
                searchstat = 1; searchit = 1;
            }
            else if (dplc[searchx] <= searchy
#ifdef YAX_ENABLE
                     && searchy < dmost[searchx] && getflorzofslope(sectnum, globalposx, globalposy) >= globalposz
                     && (yax_getbunch(sectnum, YAX_FLOOR) < 0 || showinvisibility || (sec->floorstat&(256+128)) || klabs(yax_globallev-YAX_MAXDRAWS)==YAX_MAXDRAWS)
#endif
                ) //floor
            {
                searchsector = sectnum; searchwall = wallnum;
                searchstat = 2; searchit = 1;
            }
        }

#ifdef YAX_ENABLE
        if (yax_nomaskpass==0 || !yax_isislandwall(wallnum, !yax_globalcf) || (yax_nomaskdidit=1, 0))
#endif
        if (nextsectnum >= 0)
        {
            // 2       <---       3
            // x------------------x
            // 0       --->       1
            //
            //     4 (our pos, z wrt the nextsector!)

            int32_t cz[5], fz[5];

            getzsofslope((int16_t)sectnum,wal->x,wal->y,&cz[0],&fz[0]);
            getzsofslope((int16_t)sectnum,wall[wal->point2].x,wall[wal->point2].y,&cz[1],&fz[1]);
            getzsofslope((int16_t)nextsectnum,wal->x,wal->y,&cz[2],&fz[2]);
            getzsofslope((int16_t)nextsectnum,wall[wal->point2].x,wall[wal->point2].y,&cz[3],&fz[3]);
            getzsofslope((int16_t)nextsectnum,globalposx,globalposy,&cz[4],&fz[4]);

            if ((wal->cstat&48) == 16)
                maskwall[maskwallcnt++] = z;

            if (((sec->ceilingstat&1) == 0) || ((nextsec->ceilingstat&1) == 0))
            {
                if ((cz[2] <= cz[0]) && (cz[3] <= cz[1]))
                {
//                    if (globparaceilclip)
                    if (getceilzofslope(sectnum, globalposx, globalposy) <= globalposz)
                        for (x=x1; x<=x2; x++)
                            if (uplc[x] > umost[x])
                                if (umost[x] <= dmost[x])
                                {
                                    umost[x] = uplc[x];
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                }
                else
                {
                    wallmost(dwall,z,nextsectnum,(uint8_t)0);

                    if ((cz[2] > fz[0]) || (cz[3] > fz[1]))
                        for (i=x1; i<=x2; i++) if (dwall[i] > dplc[i]) dwall[i] = dplc[i];

                    if (searchit == 2 && (searchx >= x1 && searchx <= x2))
#ifdef YAX_ENABLE
                        if (uplc[searchx] <= searchy)
#endif
                        if (searchy <= dwall[searchx]) //wall
                        {
                            searchsector = sectnum; searchbottomwall = searchwall = wallnum;
                            searchisbottom = 0;
                            searchstat = 0; searchit = 1;
                        }

                    setup_globals_wall1(wal, wal->picnum);
                    setup_globals_wall2(wal, sec->visibility, nextsec->ceilingz, sec->ceilingz);

                    if (gotswall == 0) { gotswall = 1; prepwall(z,wal); }
                    wallscan(x1,x2,uplc,dwall,swall,lwall);

                    if ((cz[2] >= cz[0]) && (cz[3] >= cz[1]))
                    {
#ifdef YAX_ENABLE
                        if (should_clip_cwall(x1, x2))
#endif
                        for (x=x1; x<=x2; x++)
                            if (dwall[x] > umost[x])
                                if (umost[x] <= dmost[x])
                                {
                                    umost[x] = dwall[x];
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                    }
                    else
                    {
#ifdef YAX_ENABLE
                        if (should_clip_cwall(x1, x2))
#endif
                        for (x=x1; x<=x2; x++)
                            if (umost[x] <= dmost[x])
                            {
                                i = max(uplc[x],dwall[x]);
                                if (i > umost[x])
                                {
                                    umost[x] = i;
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                            }
                    }
                }

                if ((cz[2] < cz[0]) || (cz[3] < cz[1]) || (globalposz < cz[4]))
                {
                    i = x2-x1+1;
                    if (smostcnt+i < ysavecnt)
                    {
                        smoststart[smostwallcnt] = smostcnt;
                        smostwall[smostwallcnt] = z;
                        smostwalltype[smostwallcnt] = 1;   //1 for umost
                        smostwallcnt++;
                        copybufbyte(&umost[x1],&smost[smostcnt],i*sizeof(smost[0]));
                        smostcnt += i;
                    }
                }
            }

            if (((sec->floorstat&1) == 0) || ((nextsec->floorstat&1) == 0))
            {
                if ((fz[2] >= fz[0]) && (fz[3] >= fz[1]))
                {
//                    if (globparaflorclip)
                    if (getflorzofslope(sectnum, globalposx, globalposy) >= globalposz)
                        for (x=x1; x<=x2; x++)
                            if (dplc[x] < dmost[x])
                                if (umost[x] <= dmost[x])
                                {
                                    dmost[x] = dplc[x];
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                }
                else
                {
                    wallmost(uwall,z,nextsectnum,(uint8_t)1);

                    if ((fz[2] < cz[0]) || (fz[3] < cz[1]))
                        for (i=x1; i<=x2; i++) if (uwall[i] < uplc[i]) uwall[i] = uplc[i];

                    if (searchit == 2 && (searchx >= x1 && searchx <= x2))
#ifdef YAX_ENABLE
                        if (dplc[searchx] >= searchy)
#endif
                        if (searchy >= uwall[searchx]) //wall
                        {
                            searchsector = sectnum; searchbottomwall = searchwall = wallnum;
                            if ((wal->cstat&2) > 0) searchbottomwall = wal->nextwall;
                            searchisbottom = 1;
                            searchstat = 0; searchit = 1;
                        }

                    const uwalltype *twal = (wal->cstat&2) ? (uwalltype *)&wall[wal->nextwall] : wal;
                    setup_globals_wall1(twal, twal->picnum);

                    setup_globals_wall2(wal, sec->visibility, nextsec->floorz, sec->ceilingz);

                    if (gotswall == 0) { gotswall = 1; prepwall(z,wal); }
                    wallscan(x1,x2,uwall,dplc,swall,lwall);

                    if ((fz[2] <= fz[0]) && (fz[3] <= fz[1]))
                    {
#ifdef YAX_ENABLE
                        if (should_clip_fwall(x1, x2))
#endif
                        for (x=x1; x<=x2; x++)
                            if (uwall[x] < dmost[x] && umost[x] <= dmost[x])
                            {
                                dmost[x] = uwall[x];
                                if (umost[x] > dmost[x]) numhits--;
                            }
                    }
                    else
                    {
#ifdef YAX_ENABLE
                        if (should_clip_fwall(x1, x2))
#endif
                        for (x=x1; x<=x2; x++)
                            if (umost[x] <= dmost[x])
                            {
                                i = min(dplc[x],uwall[x]);
                                if (i < dmost[x])
                                {
                                    dmost[x] = i;
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                            }
                    }
                }

                if ((fz[2] > fz[0]) || (fz[3] > fz[1]) || (globalposz > fz[4]))
                {
                    i = x2-x1+1;
                    if (smostcnt+i < ysavecnt)
                    {
                        smoststart[smostwallcnt] = smostcnt;
                        smostwall[smostwallcnt] = z;
                        smostwalltype[smostwallcnt] = 2;   //2 for dmost
                        smostwallcnt++;
                        copybufbyte(&dmost[x1],&smost[smostcnt],i*sizeof(smost[0]));
                        smostcnt += i;
                    }
                }
            }

            if (numhits < 0)
                return;

            if (!(wal->cstat&32) && (gotsector[nextsectnum>>3]&pow2char[nextsectnum&7]) == 0)
            {
                if (umost[x2] < dmost[x2])
                    scansector(nextsectnum);
                else
                {
                    for (x=x1; x<x2; x++)
                        if (umost[x] < dmost[x])
                            { scansector(nextsectnum); break; }

                    //If can't see sector beyond, then cancel smost array and just
                    //store wall!
                    if (x == x2)
                    {
                        smostwallcnt = startsmostwallcnt;
                        smostcnt = startsmostcnt;
                        smostwall[smostwallcnt] = z;
                        smostwalltype[smostwallcnt] = 0;
                        smostwallcnt++;
                    }
                }
            }
        }

        if (nextsectnum < 0 || (wal->cstat&32))   //White/1-way wall
        {
            setup_globals_wall1(wal, (nextsectnum < 0) ? wal->picnum : wal->overpicnum);
            setup_globals_wall2(wal, sec->visibility,
                                (nextsectnum >= 0) ? nextsec->ceilingz : sec->ceilingz,
                                (nextsectnum >= 0) ? sec->ceilingz : sec->floorz);

            if (gotswall == 0) { gotswall = 1; prepwall(z,wal); }
            wallscan(x1,x2,uplc,dplc,swall,lwall);

#ifdef YAX_ENABLE
            // TODO: slopes?

            if (globalposz > sec->floorz && yax_isislandwall(wallnum, YAX_FLOOR))
            {
                for (x=x1; x<=x2; x++)
                    if (dplc[x] > umost[x] && umost[x] <= dmost[x])
                    {
                        umost[x] = dplc[x];
                        if (umost[x] > dmost[x]) numhits--;
                    }
            }
            else if (globalposz < sec->ceilingz && yax_isislandwall(wallnum, YAX_CEILING))
            {
                for (x=x1; x<=x2; x++)
                    if (uplc[x] < dmost[x] && umost[x] <= dmost[x])
                    {
                        dmost[x] = uplc[x];
                        if (umost[x] > dmost[x]) numhits--;
                    }
            }
            else
#endif
            for (x=x1; x<=x2; x++)
                if (umost[x] <= dmost[x])
                    { umost[x] = 1; dmost[x] = 0; numhits--; }
            smostwall[smostwallcnt] = z;
            smostwalltype[smostwallcnt] = 0;
            smostwallcnt++;

            if (searchit == 2 && (x1 <= searchx && searchx <= x2))
#ifdef YAX_ENABLE
                if (uplc[searchx] <= searchy && searchy < dplc[searchx])
#endif
            {
                searchit = 1; searchsector = sectnum;
                searchbottomwall = searchwall = wallnum;
                searchstat = (nextsectnum < 0) ? 0 : 4;
            }
        }

#ifdef ENGINE_SCREENSHOT_DEBUG
        if (engine_screenshot)
# ifdef YAX_ENABLE
        if (!g_nodraw)
# endif
        {
            static char fn[32], tmpbuf[80];
            char purple = getclosestcol(255, 0, 255);
            char yellow = getclosestcol(255, 255, 0);
            char *bakframe = (char *)Xaligned_alloc(16, xdim*ydim);

            begindrawing();  //{{{
            Bmemcpy(bakframe, (char *)frameplace, xdim*ydim);
            for (x=0; x<xdim; x++)
            {
                if (umost[x] > dmost[x])
                {
                    *((char *)frameplace + (ydim/2)*bytesperline + x) = yellow;
                    *((char *)frameplace + (ydim/2+1)*bytesperline + x) = purple;
                    continue;
                }

                if (umost[x] >= 0 && umost[x] < ydim)
                    *((char *)frameplace + umost[x]*bytesperline + x) = purple;

                if (dmost[x]-1 >= 0 && dmost[x]-1 < ydim)
                    *((char *)frameplace + (dmost[x]-1)*bytesperline + x) = yellow;
            }

            Bsprintf(tmpbuf, "nmp%d l%d b%d s%d w%d", yax_nomaskpass, yax_globallev-YAX_MAXDRAWS,
                     yax_globalbunch, sectnum, wallnum);
            printext256(8,8, whitecol,0, tmpbuf, 0);

            Bsprintf(fn, "engshot%04d.png", engine_screenshot);
            screencapture(fn, 0);
            engine_screenshot++;

            Bmemcpy((char *)frameplace, bakframe, xdim*ydim);
            enddrawing();  //}}}

            Baligned_free(bakframe);
        }
#endif
    }
}

// High-precision integer type for view-relative x and y in drawvox().
typedef zint_t voxint_t;

//
// drawvox
//
static void drawvox(int32_t dasprx, int32_t daspry, int32_t dasprz, int32_t dasprang,
                    int32_t daxscale, int32_t dayscale, int32_t daindex,
                    int8_t dashade, char dapal, const int32_t *daumost, const int32_t *dadmost)
{
    int32_t i, j, k, x, y;

    int32_t cosang = sintable[(globalang+512)&2047];
    int32_t sinang = sintable[globalang&2047];
    int32_t sprcosang = sintable[(dasprang+512)&2047];
    int32_t sprsinang = sintable[dasprang&2047];

    i = klabs(dmulscale6(dasprx-globalposx, cosang, daspry-globalposy, sinang));
    j = getpalookup(mulscale21(globvis,i), dashade)<<8;
    setupdrawslab(ylookup[1], FP_OFF(palookup[dapal])+j);

    j = 1310720;
    j *= min(daxscale,dayscale); j >>= 6;  //New hacks (for sized-down voxels)
    for (k=0; k<MAXVOXMIPS; k++)
    {
        if (i < j) { i = k; break; }
        j <<= 1;
    }
    if (k >= MAXVOXMIPS)
        i = MAXVOXMIPS-1;

    if (novoxmips)
        i = 0;

    char *davoxptr = (char *)voxoff[daindex][i];
    if (!davoxptr && i > 0) { davoxptr = (char *)voxoff[daindex][0]; i = 0; }
    if (!davoxptr)
        return;

    if (voxscale[daindex] == 65536)
        { daxscale <<= (i+8); dayscale <<= (i+8); }
    else
    {
        daxscale = mulscale8(daxscale<<i,voxscale[daindex]);
        dayscale = mulscale8(dayscale<<i,voxscale[daindex]);
    }

    const int32_t odayscale = dayscale;
    daxscale = mulscale16(daxscale,xyaspect);
    daxscale = scale(daxscale, xdimenscale, xdimen<<8);
    dayscale = scale(dayscale, mulscale16(xdimenscale,viewingrangerecip), xdimen<<8);

    const int32_t daxscalerecip = divideu32_noinline(1<<30, daxscale);
    const int32_t dayscalerecip = divideu32_noinline(1<<30, dayscale);

    int32_t *longptr = (int32_t *)davoxptr;
    const int32_t daxsiz = B_LITTLE32(longptr[0]), daysiz = B_LITTLE32(longptr[1]); //dazsiz = B_LITTLE32(longptr[2]);
    const int32_t daxpivot = B_LITTLE32(longptr[3]), daypivot = B_LITTLE32(longptr[4]), dazpivot = B_LITTLE32(longptr[5]);
    davoxptr += (6<<2);

    x = mulscale16(globalposx-dasprx, daxscalerecip);
    y = mulscale16(globalposy-daspry, daxscalerecip);
    const int32_t backx = (dmulscale10(x,sprcosang, y,sprsinang)+daxpivot)>>8;
    const int32_t backy = (dmulscale10(y,sprcosang, x,-sprsinang)+daypivot)>>8;
    const int32_t cbackx = min(max(backx,0),daxsiz-1);
    const int32_t cbacky = min(max(backy,0),daysiz-1);

    sprcosang = mulscale14(daxscale, sprcosang);
    sprsinang = mulscale14(daxscale, sprsinang);

    x = (dasprx-globalposx) - dmulscale18(daxpivot,sprcosang, daypivot,-sprsinang);
    y = (daspry-globalposy) - dmulscale18(daypivot,sprcosang, daxpivot,sprsinang);

    cosang = mulscale16(cosang, dayscalerecip);
    sinang = mulscale16(sinang, dayscalerecip);

    const voxint_t gxstart = (voxint_t)y*cosang - (voxint_t)x*sinang;
    const voxint_t gystart = (voxint_t)x*cosang + (voxint_t)y*sinang;
    const int32_t gxinc = dmulscale10(sprsinang,cosang, sprcosang,-sinang);
    const int32_t gyinc = dmulscale10(sprcosang,cosang, sprsinang,sinang);

    x = 0; y = 0; j = max(daxsiz,daysiz);
    for (i=0; i<=j; i++)
    {
        ggxinc[i] = x; x += gxinc;
        ggyinc[i] = y; y += gyinc;
    }

    if ((klabs(globalposz-dasprz)>>10) >= klabs(odayscale))
        return;

    const int32_t syoff = divscale21(globalposz-dasprz,odayscale) + (dazpivot<<7);
    int32_t yoff = (klabs(gxinc)+klabs(gyinc))>>1;
    longptr = (int32_t *)davoxptr;
    int32_t xyvoxoffs = (daxsiz+1)<<2;

    begindrawing(); //{{{

    for (bssize_t cnt=0; cnt<8; cnt++)
    {
        int32_t xs=0, ys=0, xi=0, yi=0;

        switch (cnt)
        {
        case 0:
            xs = 0;        ys = 0;        xi = 1;  yi = 1;  break;
        case 1:
            xs = daxsiz-1; ys = 0;        xi = -1; yi = 1;  break;
        case 2:
            xs = 0;        ys = daysiz-1; xi = 1;  yi = -1; break;
        case 3:
            xs = daxsiz-1; ys = daysiz-1; xi = -1; yi = -1; break;
        case 4:
            xs = 0;        ys = cbacky;   xi = 1;  yi = 2;  break;
        case 5:
            xs = daxsiz-1; ys = cbacky;   xi = -1; yi = 2;  break;
        case 6:
            xs = cbackx;   ys = 0;        xi = 2;  yi = 1;  break;
        case 7:
            xs = cbackx;   ys = daysiz-1; xi = 2;  yi = -1; break;
        }

        int32_t xe = cbackx, ye = cbacky;

        if (cnt < 4)
        {
            if ((xi < 0) && (xe >= xs)) continue;
            if ((xi > 0) && (xe <= xs)) continue;
            if ((yi < 0) && (ye >= ys)) continue;
            if ((yi > 0) && (ye <= ys)) continue;
        }
        else
        {
            if ((xi < 0) && (xe > xs)) continue;
            if ((xi > 0) && (xe < xs)) continue;
            if ((yi < 0) && (ye > ys)) continue;
            if ((yi > 0) && (ye < ys)) continue;
            xe += xi; ye += yi;
        }

        int32_t x1=0, y1=0, z1, x2=0, y2=0, z2;

        i = ksgn(ys-backy) + ksgn(xs-backx)*3 + 4;
        switch (i)
        {
        case 6:
        case 7:
            x1 = 0; y1 = 0; break;
        case 8:
        case 5:
            x1 = gxinc; y1 = gyinc; break;
        case 0:
        case 3:
            x1 = gyinc; y1 = -gxinc; break;
        case 2:
        case 1:
            x1 = gxinc+gyinc; y1 = gyinc-gxinc; break;
        }
        switch (i)
        {
        case 2:
        case 5:
            x2 = 0; y2 = 0; break;
        case 0:
        case 1:
            x2 = gxinc; y2 = gyinc; break;
        case 8:
        case 7:
            x2 = gyinc; y2 = -gxinc; break;
        case 6:
        case 3:
            x2 = gxinc+gyinc; y2 = gyinc-gxinc; break;
        }

        const char oand = pow2char[(xs<backx)+0] + pow2char[(ys<backy)+2];
        const char oand16 = oand+16;
        const char oand32 = oand+32;

        int32_t dagxinc, dagyinc;

        if (yi > 0) { dagxinc = gxinc; dagyinc = mulscale16(gyinc,viewingrangerecip); }
        else { dagxinc = -gxinc; dagyinc = -mulscale16(gyinc,viewingrangerecip); }

        //Fix for non 90 degree viewing ranges
        const int32_t nxoff = mulscale16(x2-x1,viewingrangerecip);
        x1 = mulscale16(x1, viewingrangerecip);

        const voxint_t ggxstart = gxstart + ggyinc[ys];
        const voxint_t ggystart = gystart - ggxinc[ys];

        for (x=xs; x!=xe; x+=xi)
        {
            const intptr_t slabxoffs = (intptr_t)&davoxptr[B_LITTLE32(longptr[x])];
            int16_t *const shortptr = (int16_t *)&davoxptr[((x*(daysiz+1))<<1) + xyvoxoffs];

            voxint_t nx = mulscale16z(ggxstart+ggxinc[x], viewingrangerecip) + x1;
            voxint_t ny = ggystart + ggyinc[x];

            for (y=ys; y!=ye; y+=yi,nx+=dagyinc,ny-=dagxinc)
            {
                if (ny <= nytooclose || ny >= nytoofar)
                    continue;

                char *voxptr = (char *)(B_LITTLE16(shortptr[y])+slabxoffs);
                char *const voxend = (char *)(B_LITTLE16(shortptr[y+1])+slabxoffs);
                if (voxptr == voxend)
                    continue;

                // AMCTC V1 MEGABASE: (ny+y1)>>14 == 65547
                // (after long corridor with the blinds)
                //
                // Also, OOB (<0?) in my amcvoxels_crash.map.
                const int32_t il = clamp((ny+y1)>>14, 1, DISTRECIPSIZ-1);
                int32_t lx = mulscale32(nx>>3, distrecip[il]) + halfxdimen;
                if (lx < 0)
                    lx = 0;

                const int32_t ir = clamp((ny+y2)>>14, 1, DISTRECIPSIZ-1);
                int32_t rx = mulscale32((nx+nxoff)>>3, distrecip[ir]) + halfxdimen;
                if (rx > xdimen)
                    rx = xdimen;

                if (rx <= lx)
                    continue;

                rx -= lx;

                const int32_t l1 = distrecip[clamp((ny-yoff)>>14, 1, DISTRECIPSIZ-1)];
                // FIXME! AMCTC RC2/beta shotgun voxel
                // (e.g. training map right after M16 shooting):
                const int32_t l2 = distrecip[clamp((ny+yoff)>>14, 1, DISTRECIPSIZ-1)];

                for (; voxptr<voxend; voxptr+=voxptr[1]+3)
                {
                    j = (voxptr[0]<<15)-syoff;

                    if (j < 0)
                    {
                        k = j+(voxptr[1]<<15);
                        if (k < 0)
                        {
                            if ((voxptr[2]&oand32) == 0) continue;
                            z2 = mulscale32(l2,k) + globalhoriz;     //Below slab
                        }
                        else
                        {
                            if ((voxptr[2]&oand) == 0) continue;    //Middle of slab
                            z2 = mulscale32(l1,k) + globalhoriz;
                        }
                        z1 = mulscale32(l1,j) + globalhoriz;
                    }
                    else
                    {
                        if ((voxptr[2]&oand16) == 0) continue;
                        z1 = mulscale32(l2,j) + globalhoriz;        //Above slab
                        z2 = mulscale32(l1,j+(voxptr[1]<<15)) + globalhoriz;
                    }

                    int32_t yplc, yinc=0;

                    if (voxptr[1] == 1)
                    {
                        yplc = 0; yinc = 0;
                        if (z1 < daumost[lx])
                            z1 = daumost[lx];
                    }
                    else
                    {
                        if (z2-z1 >= 1024)
                            yinc = divscale16(voxptr[1], z2-z1);
                        else if (z2 > z1)
                            yinc = lowrecip[z2-z1]*voxptr[1]>>8;

                        if (z1 < daumost[lx]) { yplc = yinc*(daumost[lx]-z1); z1 = daumost[lx]; }
                        else yplc = 0;
                    }

                    if (z2 > dadmost[lx])
                        z2 = dadmost[lx];
                    z2 -= z1;
                    if (z2 <= 0)
                        continue;

                    drawslab(rx, yplc, z2, yinc, (intptr_t)&voxptr[3], ylookup[z1]+lx+frameoffset);
                }
            }
        }
    }

#if 0
    for (x=0; x<xdimen; x++)
    {
        if (daumost[x]>=0 && daumost[x]<ydimen)
            *(char *)(frameplace + x + bytesperline*daumost[x]) = editorcolors[13];
        if (dadmost[x]>=0 && dadmost[x]<ydimen)
            *(char *)(frameplace + x + bytesperline*dadmost[x]) = editorcolors[14];
    }
#endif

    enddrawing();   //}}}
}


static void setup_globals_sprite1(const uspritetype *tspr, const usectortype *sec,
                                     int32_t yspan, int32_t yoff, int32_t tilenum,
                                     int32_t cstat, int32_t *z1ptr, int32_t *z2ptr)
{
    int32_t logtilesizy, tsizy;
    int32_t z1, z2 = tspr->z - ((yoff*tspr->yrepeat)<<2);

    if (cstat&128)
    {
        z2 += ((yspan*tspr->yrepeat)<<1);
        if (yspan&1) z2 += (tspr->yrepeat<<1);        //Odd yspans
    }
    z1 = z2 - ((yspan*tspr->yrepeat)<<2);

    globalorientation = 0;
    globalpicnum = tilenum;
    if ((unsigned)globalpicnum >= MAXTILES) globalpicnum = 0;
    // sprite panning
    globalxpanning = (((256-spriteext[tspr->owner].xpanning)&255) * tilesiz[globalpicnum].x)>>8;
    globalypanning = 0;

    globvis = globalvisibility;
    if (sec->visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sec->visibility+16));

    logtilesizy = (picsiz[globalpicnum]>>4);
    tsizy = tilesiz[globalpicnum].y;

    globalshiftval = logtilesizy;
#if !defined CLASSIC_NONPOW2_YSIZE_SPRITES
    // before proper non-power-of-two tilesizy drawing
    if (pow2long[logtilesizy] != tsizy)
        globalshiftval++;
#else
    // non power-of-two y size textures!
    if (pow2long[logtilesizy] != tsizy || tsizy >= 512)
    {
        globaltilesizy = tsizy;
        globalyscale = (1<<22)/(tsizy*tspr->yrepeat);
        globalshiftval = 0;
    }
    else
#endif
    {
        globalshiftval = 32-globalshiftval;
        globalyscale = divscale(512,tspr->yrepeat,globalshiftval-19);
    }

    globalzd = ((int64_t)(globalposz-z1)*globalyscale)<<8;
    if ((cstat&8) > 0)
    {
        globalyscale = -globalyscale;
        globalzd = ((int64_t)(globalposz-z2)*globalyscale)<<8;
    }

    *z1ptr = z1;
    *z2ptr = z2;
}

//
// drawsprite (internal)
//

static size_t falpha_to_blend(float alpha, int32_t *cstatptr, uint8_t *blendptr, int32_t transbit1, int32_t transbit2)
{
    int32_t cstat = *cstatptr | transbit1;

    int32_t const twonumalphatabs = 2*numalphatabs + (numalphatabs&1);
    int32_t blendidx = Blrintf(alpha * twonumalphatabs);
    if (blendidx > numalphatabs)
    {
        blendidx = twonumalphatabs - blendidx;
        cstat |= transbit2;
    }
    else
    {
        cstat &= ~transbit2;
    }

    if (blendidx < 1)
        return cstat&transbit2;

    // blendidx now in [1 .. numalphatabs]

    *cstatptr = cstat;
    *blendptr = blendidx;
    return 0;
}

static FORCE_INLINE int32_t mulscale_triple30(int32_t a, int32_t b, int32_t c)
{
    return ((int64_t)a * b * c)>>30;
}

static void drawsprite_classic(int32_t snum)
{
    uspritetype *const tspr = tspriteptr[snum];
    const int32_t sectnum = tspr->sectnum;

    if (sectnum < 0 || bad_tspr(tspr))
        return;

    int32_t x1, y1, x2, y2, i, j, k, x;
    int32_t z, zz, z1, z2, xp1, yp1, xp2, yp2;
    int32_t dax, day, dax1, dax2, y;
    int32_t vtilenum = 0;


    uint8_t blendidx = tspr->blend;
    const int32_t xb = spritesxyz[snum].x;
    const int32_t yp = spritesxyz[snum].y;

    const int32_t spritenum = tspr->owner;
    const float alpha = spriteext[spritenum].alpha;

    const usectortype *const sec = (usectortype *)&sector[sectnum];

    int32_t cstat=tspr->cstat, tilenum;

    DO_TILE_ANIM(tspr->picnum, spritenum+32768);

    if (!(cstat&2) && alpha > 0.0f)
    {
        if (alpha >= 1.0f)
            return;

        if (numalphatabs != 0)
        {
            if (falpha_to_blend(alpha, &cstat, &blendidx, 2, 512))
                return;
        }
        else if (alpha >= 1.f/3.f)
        {
            cstat |= 2;

            if (alpha >= 2.f/3.f)
                cstat |= 512;
            else
                cstat &= ~512;
        }

        tspr->cstat = cstat;
    }

    tilenum = tspr->picnum;

    if ((cstat&48)==48)
        vtilenum = tilenum; // if the game wants voxels, it gets voxels
    else if (usevoxels && tiletovox[tilenum] != -1 && !(spriteext[spritenum].flags&SPREXT_NOTMD))
    {
        vtilenum = tiletovox[tilenum];
        cstat |= 48;
    }

    if ((cstat&48) != 48)
    {
        if (spritenum < 0 || tilesiz[tilenum].x <= 0 || tilesiz[tilenum].y <= 0)
            return;
    }

    if (tspr->xrepeat <= 0 || tspr->yrepeat <= 0)
        return;

    globalpal = tspr->pal;
    if (palookup[globalpal] == NULL) globalpal = 0;    // JBF: fixes null-pointer crash
    globalshade = tspr->shade;

    if (cstat&2)
        setup_blend(blendidx, cstat&512);

    vec2_t off = { picanm[tilenum].xofs + tspr->xoffset, picanm[tilenum].yofs + tspr->yoffset };

    if ((cstat&48) == 0)
    {
        int32_t startum, startdm;
        int32_t linum, linuminc;

draw_as_face_sprite:
        if (yp <= (4<<8)) return;

        int const isiz = divscale19(xdimenscale,yp);
        int const xv = mulscale16(((int32_t)tspr->xrepeat)<<16,xyaspect);
        vec2s_t const span = tilesiz[tilenum];
        vec2_t const siz = { mulscale30(isiz, xv * span.x), mulscale14(isiz, tspr->yrepeat * span.y) };

        if (EDUKE32_PREDICT_FALSE((span.x>>11) >= siz.x || span.y >= (siz.y>>1)))
            return;  //Watch out for divscale overflow

        x1 = xb-(siz.x>>1);
        if (span.x&1) x1 += mulscale31(isiz,xv);  //Odd xspans
        i = mulscale30(isiz,xv*off.x);
        if ((cstat&4) == 0) x1 -= i; else x1 += i;

        y1 = mulscale16(tspr->z-globalposz,isiz);
        y1 -= mulscale14(isiz,tspr->yrepeat*off.y);
        y1 += (globalhoriz<<8)-siz.y;
        if (cstat&128)
        {
            y1 += (siz.y>>1);
            if (span.y&1) y1 += mulscale15(isiz,tspr->yrepeat);  //Odd yspans
        }

        x2 = x1+siz.x-1;
        y2 = y1+siz.y-1;
        if ((y1|255) >= (y2|255)) return;

        int32_t lx = (x1>>8)+1; if (lx < 0) lx = 0;
        int32_t rx = (x2>>8); if (rx >= xdimen) rx = xdimen-1;
        if (lx > rx) return;

        startum = ((sec->ceilingstat&3) == 0) ? globalhoriz+mulscale24(isiz,sec->ceilingz-globalposz)-1 : 0;
        startdm = ((sec->floorstat&3) == 0) ? globalhoriz+mulscale24(isiz,sec->floorz-globalposz)+1 : INT32_MAX;

        if ((y1>>8) > startum) startum = (y1>>8);
        if ((y2>>8) < startdm) startdm = (y2>>8);

        if (startum < -32768) startum = -32768;
        if (startdm > 32767) startdm = 32767;
        if (startum >= startdm) return;

        if ((cstat&4) == 0)
        {
            linuminc = divscale24(span.x,siz.x);
            linum = mulscale8((lx<<8)-x1,linuminc);
        }
        else
        {
            linuminc = -divscale24(span.x,siz.x);
            linum = mulscale8((lx<<8)-x2,linuminc);
        }

        if ((cstat&8) > 0)
            swaplong(&y1, &y2);

        x = lx;
#ifdef CLASSIC_SLICE_BY_4
        for (; x<=rx-4; x+=4)
        {
            uwall[x] =   max(startumost[windowxy1.x+x]-windowxy1.y,   (int16_t) startum);
            uwall[x+1] = max(startumost[windowxy1.x+x+1]-windowxy1.y, (int16_t) startum);
            uwall[x+2] = max(startumost[windowxy1.x+x+2]-windowxy1.y, (int16_t) startum);
            uwall[x+3] = max(startumost[windowxy1.x+x+3]-windowxy1.y, (int16_t) startum);

            dwall[x] =   min(startdmost[windowxy1.x+x]-windowxy1.y,   (int16_t) startdm);
            dwall[x+1] = min(startdmost[windowxy1.x+x+1]-windowxy1.y, (int16_t) startdm);
            dwall[x+2] = min(startdmost[windowxy1.x+x+2]-windowxy1.y, (int16_t) startdm);
            dwall[x+3] = min(startdmost[windowxy1.x+x+3]-windowxy1.y, (int16_t) startdm);
        }
#endif
        for (; x<=rx; x++)
        {
            uwall[x] = max(startumost[windowxy1.x+x]-windowxy1.y,(int16_t)startum);
            dwall[x] = min(startdmost[windowxy1.x+x]-windowxy1.y,(int16_t)startdm);
        }

        int32_t daclip = 0;
        for (i=smostwallcnt-1; i>=0; i--)
        {
            if (smostwalltype[i]&daclip) continue;
            j = smostwall[i];
            if ((xb1[j] > rx) || (xb2[j] < lx)) continue;
            if ((yp <= yb1[j]) && (yp <= yb2[j])) continue;
            if (spritewallfront(tspr,(int32_t)thewall[j]) && ((yp <= yb1[j]) || (yp <= yb2[j]))) continue;

            const int32_t dalx2 = max(xb1[j],lx);
            const int32_t darx2 = min(xb2[j],rx);

            switch (smostwalltype[i])
            {
            case 0:
                if (dalx2 <= darx2)
                {
                    if ((dalx2 == lx) && (darx2 == rx)) return;
                    //clearbufbyte(&dwall[dalx2],(darx2-dalx2+1)*sizeof(dwall[0]),0L);
                    for (k=dalx2; k<=darx2; k++) dwall[k] = 0;
                }
                break;
            case 1:
                k = smoststart[i] - xb1[j];
                x = dalx2;
#ifdef CLASSIC_SLICE_BY_4 // ok, this one is really by 2 ;)
                for (; x<=darx2-2; x+=2)
                {
                    if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
                    if (smost[k+x+1] > uwall[x+1]) uwall[x+1] = smost[k+x+1];
                }
#endif
                for (; x<=darx2; x++)
                    if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
                if ((dalx2 == lx) && (darx2 == rx)) daclip |= 1;
                break;
            case 2:
                k = smoststart[i] - xb1[j];
                x = dalx2;
#ifdef CLASSIC_SLICE_BY_4
                for (; x<=darx2-4; x+=4)
                {
                    if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
                    if (smost[k+x+1] < dwall[x+1]) dwall[x+1] = smost[k+x+1];
                    if (smost[k+x+2] < dwall[x+2]) dwall[x+2] = smost[k+x+2];
                    if (smost[k+x+3] < dwall[x+3]) dwall[x+3] = smost[k+x+3];
                }
#endif
                for (; x<=darx2; x++)
                    if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
                if ((dalx2 == lx) && (darx2 == rx)) daclip |= 2;
                break;
            }
        }

        if (uwall[rx] >= dwall[rx])
        {
            for (x=lx; x<rx; x++)
                if (uwall[x] < dwall[x]) break;
            if (x == rx) return;
        }

        //sprite
#ifdef YAX_ENABLE
        if (yax_globallev==YAX_MAXDRAWS || searchit==2)
#endif
        if (searchit >= 1 && (searchx >= lx && searchx <= rx))
            if (searchy >= uwall[searchx] && searchy < dwall[searchx])
            {
                searchsector = sectnum; searchwall = spritenum;
                searchstat = 3; searchit = 1;
            }

        setup_globals_sprite1(tspr, sec, span.y, off.y, tilenum, cstat, &z1, &z2);

        qinterpolatedown16((intptr_t)&lwall[lx],rx-lx+1,linum,linuminc);
        clearbuf(&swall[lx],rx-lx+1,mulscale19(yp,xdimscale));

        {
#ifdef HIGH_PRECISION_SPRITE
            union { float f; int32_t i; } sw = {
                // initialize the float of the union
                ((cstat&8) ? -1 : 1)
                * (float)yp * xdimscale
                * (1<<(22-19)) / (span.y*tspr->yrepeat)
            };

            clearbuf(&swallf[lx], rx-lx+1, sw.i);
#endif
        }

        drawing_sprite = 1;

        if ((cstat&2) == 0)
            maskwallscan(lx,rx, (cstat&8)==0);
        else
            transmaskwallscan(lx,rx, (cstat&8)==0);

        drawing_sprite = 0;
    }
    else if ((cstat&48) == 16)
    {
        const int32_t xspan = tilesiz[tilenum].x;
        const int32_t yspan = tilesiz[tilenum].y;
        const int32_t xv = tspr->xrepeat*sintable[(tspr->ang+2560+1536)&2047];
        const int32_t yv = tspr->xrepeat*sintable[(tspr->ang+2048+1536)&2047];

        if ((cstat&4) > 0) off.x = -off.x;
        if ((cstat&8) > 0) off.y = -off.y;

        i = (xspan>>1) + off.x;
        x1 = tspr->x-globalposx-mulscale16(xv,i); x2 = x1+mulscale16(xv,xspan);
        y1 = tspr->y-globalposy-mulscale16(yv,i); y2 = y1+mulscale16(yv,xspan);

        vec2_t p1 = get_rel_coords(x1, y1);
        vec2_t p2 = get_rel_coords(x2, y2);

        if (p1.y <= 0 && p2.y <= 0)
            return;

        x1 += globalposx; y1 += globalposy;
        x2 += globalposx; y2 += globalposy;

        int32_t swapped = 0;
        if (dmulscale32(p1.x, p2.y, -p2.x, p1.y) >= 0)  // If wall's NOT facing you
        {
            if ((cstat&64) != 0)
                return;

            const vec2_t pt = p2;
            p2 = p1;
            p1 = pt;
            i = x1, x1 = x2, x2 = i;
            i = y1, y1 = y2, y2 = i;
            swapped = 1;
        }

        int32_t sx1, sx2, sy1, sy2;
        if (!get_screen_coords(p1, p2, &sx1, &sy1, &sx2, &sy2))
            return;

        const int32_t topinc = -mulscale10(p1.y,xspan);
        int32_t top = ((mulscale10(p1.x,xdimen) - mulscale9(sx1-halfxdimen,p1.y))*xspan)>>3;
        const int32_t botinc = (p2.y-p1.y)>>8;
        int32_t bot = mulscale11(p1.x-p2.x,xdimen) + mulscale2(sx1-halfxdimen,botinc);

        j = sx2+3;
        z = mulscale20(top,krecipasm(bot));
        lwall[sx1] = (z>>8);
        for (x=sx1+4; x<=j; x+=4)
        {
            top += topinc; bot += botinc;
            zz = z; z = mulscale20(top,krecipasm(bot));
            i = ((z+zz)>>1);
            lwall[x-3] = ((i+zz)>>9);
            lwall[x-2] = (i>>8);
            lwall[x-1] = ((i+z)>>9);
            lwall[x] = (z>>8);
        }

        if (lwall[sx1] < 0) lwall[sx1] = 0;
        if (lwall[sx2] >= xspan) lwall[sx2] = xspan-1;

        if ((swapped^((cstat&4)>0)) > 0)
        {
            j = xspan-1;
            for (x=sx1; x<=sx2; x++)
                lwall[x] = j-lwall[x];
        }

        // XXX: UNUSED?
        rx1[MAXWALLSB-1] = p1.x; ry1[MAXWALLSB-1] = p1.y;
        rx2[MAXWALLSB-1] = p2.x; ry2[MAXWALLSB-1] = p2.y;

        setup_globals_sprite1(tspr, sec, yspan, off.y, tilenum, cstat, &z1, &z2);

        if ((sec->ceilingstat&1) == 0 && z1 < sec->ceilingz)
            z1 = sec->ceilingz;
        if ((sec->floorstat&1) == 0 && z2 > sec->floorz)
            z2 = sec->floorz;

        xb1[MAXWALLSB-1] = sx1;
        xb2[MAXWALLSB-1] = sx2;
        yb1[MAXWALLSB-1] = sy1;
        yb2[MAXWALLSB-1] = sy2;
        owallmost(uwall, MAXWALLSB-1, z1-globalposz);
        owallmost(dwall, MAXWALLSB-1, z2-globalposz);

        int32_t hplc = divscale19(xdimenscale,sy1);
        const int32_t hplc2 = divscale19(xdimenscale,sy2);
        const int32_t idiv = sx2-sx1;
        int32_t hinc[4] = { idiv ? tabledivide32(hplc2-hplc, idiv) : 0 };

#ifdef HIGH_PRECISION_SPRITE
        const float cc = ((1<<19)*fxdimen*(float)yxaspect) * (1.f/320.f);
        const float loopcc = ((cstat&8) ? -1 : 1)*((float)(1<<30)*(1<<24))
            / (yspan*tspr->yrepeat);
        float hplcf = cc/sy1;
        float hincf[4] = {idiv ? (cc/sy2 - hplcf)/idiv : 0};

#ifdef CLASSIC_SLICE_BY_4
        hincf[1] = hincf[0] * 2.f;
        hincf[2] = hincf[0] * 3.f;
        hincf[3] = hincf[0] * 4.f;
#endif // CLASSIC_SLICE_BY_4
#endif // HIGH_PRECISION_SPRITE
#ifdef CLASSIC_SLICE_BY_4
        hinc[1] = hinc[0]<<1;
        hinc[2] = hinc[0]*3;
        hinc[3] = hinc[0]<<2;
#endif
        i = sx1;

#ifdef CLASSIC_SLICE_BY_4
        for (; i<=sx2-4; i+=4)
        {
            swall[i] = (krecipasm(hplc)<<2);
            swall[i+1] = (krecipasm(hplc+hinc[0])<<2);
            swall[i+2] = (krecipasm(hplc+hinc[1])<<2);
            swall[i+3] = (krecipasm(hplc+hinc[2])<<2);
            hplc += hinc[3];
#ifdef HIGH_PRECISION_SPRITE
            swallf[i] =   loopcc/hplcf;
            swallf[i+1] = loopcc/(hplcf+hincf[0]);
            swallf[i+2] = loopcc/(hplcf+hincf[1]);
            swallf[i+3] = loopcc/(hplcf+hincf[2]);
            hplcf += hincf[3];
#endif // HIGH_PRECISION_SPRITE
        }
#endif // CLASSIC_SLICE_BY_4

        for (; i<=sx2; i++)
        {
            swall[i] = (krecipasm(hplc)<<2);
            hplc += hinc[0];
#ifdef HIGH_PRECISION_SPRITE
            swallf[i] = loopcc/hplcf;
            hplcf += hincf[0];
#endif
        }

        for (i=smostwallcnt-1; i>=0; i--)
        {
            j = smostwall[i];

            if (xb1[j] > sx2 || xb2[j] < sx1)
                continue;

            int32_t dalx2 = xb1[j];
            int32_t darx2 = xb2[j];

            if (max(sy1,sy2) > min(yb1[j],yb2[j]))
            {
                if (min(sy1,sy2) > max(yb1[j],yb2[j]))
                {
                    x = INT32_MIN;
                }
                else
                {
                    x = thewall[j]; xp1 = wall[x].x; yp1 = wall[x].y;
                    x = wall[x].point2; xp2 = wall[x].x; yp2 = wall[x].y;

                    z1 = (xp2-xp1)*(y1-yp1) - (yp2-yp1)*(x1-xp1);
                    z2 = (xp2-xp1)*(y2-yp1) - (yp2-yp1)*(x2-xp1);
                    if ((z1^z2) >= 0)
                        x = (z1+z2);
                    else
                    {
                        z1 = (x2-x1)*(yp1-y1) - (y2-y1)*(xp1-x1);
                        z2 = (x2-x1)*(yp2-y1) - (y2-y1)*(xp2-x1);

                        if ((z1^z2) >= 0)
                            x = -(z1+z2);
                        else
                        {
                            if ((xp2-xp1)*(tspr->y-yp1) == (tspr->x-xp1)*(yp2-yp1))
                            {
                                if (wall[thewall[j]].nextsector == tspr->sectnum)
                                    x = INT32_MIN;
                                else
                                    x = INT32_MAX;
                            }
                            else
                            {
                                //INTERSECTION!
                                x = (xp1-globalposx) + scale(xp2-xp1,z1,z1-z2);
                                y = (yp1-globalposy) + scale(yp2-yp1,z1,z1-z2);

                                yp1 = dmulscale14(x,cosviewingrangeglobalang,y,sinviewingrangeglobalang);

                                if (yp1 > 0)
                                {
                                    xp1 = dmulscale14(y,cosglobalang,-x,singlobalang);

                                    x = halfxdimen + scale(xp1,halfxdimen,yp1);
                                    if (xp1 >= 0) x++;   //Fix for SIGNED divide

                                    if (z1 < 0)
                                        { if (dalx2 < x) dalx2 = x; }
                                    else
                                        { if (darx2 > x) darx2 = x; }
                                    x = INT32_MIN+1;
                                }
                                else
                                    x = INT32_MAX;
                            }
                        }
                    }
                }

                if (x < 0)
                {
                    if (dalx2 < sx1) dalx2 = sx1;
                    if (darx2 > sx2) darx2 = sx2;

                    switch (smostwalltype[i])
                    {
                    case 0:
                        if (dalx2 <= darx2)
                        {
                            if ((dalx2 == sx1) && (darx2 == sx2)) return;
                            //clearbufbyte(&dwall[dalx2],(darx2-dalx2+1)*sizeof(dwall[0]),0L);
                            for (k=dalx2; k<=darx2; k++) dwall[k] = 0;
                        }
                        break;
                    case 1:
                        k = smoststart[i] - xb1[j];
                        x = dalx2;
#ifdef CLASSIC_SLICE_BY_4
                        for (; x<=darx2-2; x+=2)
                        {
                            if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
                            if (smost[k+x+1] > uwall[x+1]) uwall[x+1] = smost[k+x+1];
                        }
#endif
                        for (; x<=darx2; x++)
                            if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
                        break;
                    case 2:
                        k = smoststart[i] - xb1[j];
                        x = dalx2;
#ifdef CLASSIC_SLICE_BY_4
                        for (; x<=darx2-4; x+=4)
                        {
                            if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
                            if (smost[k+x+1] < dwall[x+1]) dwall[x+1] = smost[k+x+1];
                            if (smost[k+x+2] < dwall[x+2]) dwall[x+2] = smost[k+x+2];
                            if (smost[k+x+3] < dwall[x+3]) dwall[x+3] = smost[k+x+3];
                        }
#endif
                        for (; x<=darx2; x++)
                            if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
                        break;
                    }
                }
            }
        }

        //sprite
#ifdef YAX_ENABLE
        if (yax_globallev==YAX_MAXDRAWS || searchit==2)
#endif
        if (searchit >= 1 && (searchx >= sx1 && searchx <= sx2))
            if (searchy >= uwall[searchx] && searchy <= dwall[searchx])
            {
                searchsector = sectnum; searchwall = spritenum;
                searchstat = 3; searchit = 1;
            }

        drawing_sprite = 1;

        if ((cstat&2) == 0)
            maskwallscan(sx1,sx2, (cstat&8)==0);
        else
            transmaskwallscan(sx1,sx2, (cstat&8)==0);

        drawing_sprite = 0;
    }
    else if ((cstat&48) == 32)
    {
        if ((cstat&64) != 0)
            if ((globalposz > tspr->z) == ((cstat&8)==0))
                return;

        if ((cstat&4) > 0) off.x = -off.x;
        if ((cstat&8) > 0) off.y = -off.y;

        const int32_t xspan = tilesiz[tilenum].x;
        const int32_t yspan = tilesiz[tilenum].y;

        //Rotate center point
        dax = tspr->x-globalposx;
        day = tspr->y-globalposy;
        rzi[0] = dmulscale10(cosglobalang,dax,singlobalang,day);
        rxi[0] = dmulscale10(cosglobalang,day,-singlobalang,dax);

        //Get top-left corner
        i = ((tspr->ang+2048-globalang)&2047);
        int32_t cosang = sintable[(i+512)&2047];
        int32_t sinang = sintable[i];
        dax = ((xspan>>1)+off.x)*tspr->xrepeat;
        day = ((yspan>>1)+off.y)*tspr->yrepeat;
        rzi[0] += dmulscale12(sinang,dax,cosang,day);
        rxi[0] += dmulscale12(sinang,day,-cosang,dax);

        //Get other 3 corners
        dax = xspan*tspr->xrepeat;
        day = yspan*tspr->yrepeat;
        rzi[1] = rzi[0]-mulscale12(sinang,dax);
        rxi[1] = rxi[0]+mulscale12(cosang,dax);
        dax = -mulscale12(cosang,day);
        day = -mulscale12(sinang,day);
        rzi[2] = rzi[1]+dax; rxi[2] = rxi[1]+day;
        rzi[3] = rzi[0]+dax; rxi[3] = rxi[0]+day;

        //Put all points on same z
        ryi[0] = scale((tspr->z-globalposz),yxaspect,320<<8);
        if (ryi[0] == 0) return;
        ryi[1] = ryi[2] = ryi[3] = ryi[0];

        if ((cstat&4) == 0)
            { z = 0; z1 = 1; z2 = 3; }
        else
            { z = 1; z1 = 0; z2 = 2; }

        dax = rzi[z1]-rzi[z]; day = rxi[z1]-rxi[z];
        int32_t bot = dmulscale8(dax,dax,day,day);
        if ((klabs(dax)>>13) >= bot || (klabs(day)>>13) >= bot)
            return;
        globalx1 = divscale18(dax,bot);
        globalx2 = divscale18(day,bot);

        dax = rzi[z2]-rzi[z]; day = rxi[z2]-rxi[z];
        bot = dmulscale8(dax,dax,day,day);
        if ((klabs(dax)>>13) >= bot || (klabs(day)>>13) >= bot)
            return;
        globaly1 = divscale18(dax,bot);
        globaly2 = divscale18(day,bot);

        //Calculate globals for hline texture mapping function
        globalxpanning = (rxi[z]<<12);
        globalypanning = (rzi[z]<<12);
        globalzd = (ryi[z]<<12);

        rzi[0] = mulscale16(rzi[0],viewingrange);
        rzi[1] = mulscale16(rzi[1],viewingrange);
        rzi[2] = mulscale16(rzi[2],viewingrange);
        rzi[3] = mulscale16(rzi[3],viewingrange);

        if (ryi[0] < 0)   //If ceilsprite is above you, reverse order of points
        {
            i = rxi[1]; rxi[1] = rxi[3]; rxi[3] = i;
            i = rzi[1]; rzi[1] = rzi[3]; rzi[3] = i;
        }

        //Clip polygon in 3-space
        int32_t npoints = 4;

        //Clip edge 1
        int32_t npoints2 = 0;
        int32_t zzsgn = rxi[0]+rzi[0], zsgn;
        for (z=0; z<npoints; z++)
        {
            zz = z+1; if (zz == npoints) zz = 0;
            zsgn = zzsgn; zzsgn = rxi[zz]+rzi[zz];
            if (zsgn >= 0)
            {
                rxi2[npoints2] = rxi[z]; ryi2[npoints2] = ryi[z]; rzi2[npoints2] = rzi[z];
                npoints2++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                int32_t t = divscale30(zsgn,zsgn-zzsgn);
                rxi2[npoints2] = rxi[z] + mulscale30(t,rxi[zz]-rxi[z]);
                ryi2[npoints2] = ryi[z] + mulscale30(t,ryi[zz]-ryi[z]);
                rzi2[npoints2] = rzi[z] + mulscale30(t,rzi[zz]-rzi[z]);
                npoints2++;
            }
        }
        if (npoints2 <= 2) return;

        //Clip edge 2
        npoints = 0;
        zzsgn = rxi2[0]-rzi2[0];
        for (z=0; z<npoints2; z++)
        {
            zz = z+1; if (zz == npoints2) zz = 0;
            zsgn = zzsgn; zzsgn = rxi2[zz]-rzi2[zz];
            if (zsgn <= 0)
            {
                rxi[npoints] = rxi2[z]; ryi[npoints] = ryi2[z]; rzi[npoints] = rzi2[z];
                npoints++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                int32_t t = divscale30(zsgn,zsgn-zzsgn);
                rxi[npoints] = rxi2[z] + mulscale30(t,rxi2[zz]-rxi2[z]);
                ryi[npoints] = ryi2[z] + mulscale30(t,ryi2[zz]-ryi2[z]);
                rzi[npoints] = rzi2[z] + mulscale30(t,rzi2[zz]-rzi2[z]);
                npoints++;
            }
        }
        if (npoints <= 2) return;

        //Clip edge 3
        npoints2 = 0;
        zzsgn = ryi[0]*halfxdimen + (rzi[0]*(globalhoriz-0));
        for (z=0; z<npoints; z++)
        {
            zz = z+1; if (zz == npoints) zz = 0;
            zsgn = zzsgn; zzsgn = ryi[zz]*halfxdimen + (rzi[zz]*(globalhoriz-0));
            if (zsgn >= 0)
            {
                rxi2[npoints2] = rxi[z];
                ryi2[npoints2] = ryi[z];
                rzi2[npoints2] = rzi[z];
                npoints2++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                int32_t t = divscale30(zsgn,zsgn-zzsgn);
                rxi2[npoints2] = rxi[z] + mulscale30(t,rxi[zz]-rxi[z]);
                ryi2[npoints2] = ryi[z] + mulscale30(t,ryi[zz]-ryi[z]);
                rzi2[npoints2] = rzi[z] + mulscale30(t,rzi[zz]-rzi[z]);
                npoints2++;
            }
        }
        if (npoints2 <= 2) return;

        //Clip edge 4
        npoints = 0;
        zzsgn = ryi2[0]*halfxdimen + (rzi2[0]*(globalhoriz-ydimen));
        for (z=0; z<npoints2; z++)
        {
            zz = z+1; if (zz == npoints2) zz = 0;
            zsgn = zzsgn; zzsgn = ryi2[zz]*halfxdimen + (rzi2[zz]*(globalhoriz-ydimen));
            if (zsgn <= 0)
            {
                rxi[npoints] = rxi2[z];
                ryi[npoints] = ryi2[z];
                rzi[npoints] = rzi2[z];
                npoints++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                int32_t t = divscale30(zsgn,zsgn-zzsgn);
                rxi[npoints] = rxi2[z] + mulscale30(t,rxi2[zz]-rxi2[z]);
                ryi[npoints] = ryi2[z] + mulscale30(t,ryi2[zz]-ryi2[z]);
                rzi[npoints] = rzi2[z] + mulscale30(t,rzi2[zz]-rzi2[z]);
                npoints++;
            }
        }
        if (npoints <= 2) return;

        //Project onto screen
        int32_t lpoint = -1, lmax = INT32_MAX;
        int32_t rpoint = -1, rmax = INT32_MIN;
        for (z=0; z<npoints; z++)
        {
            xsi[z] = scale(rxi[z],xdimen<<15,rzi[z]) + (xdimen<<15);
            ysi[z] = scale(ryi[z],xdimen<<15,rzi[z]) + (globalhoriz<<16);
            if (xsi[z] < 0) xsi[z] = 0;
            if (xsi[z] > (xdimen<<16)) xsi[z] = (xdimen<<16);
            if (ysi[z] < ((int32_t)0<<16)) ysi[z] = ((int32_t)0<<16);
            if (ysi[z] > ((int32_t)ydimen<<16)) ysi[z] = ((int32_t)ydimen<<16);
            if (xsi[z] < lmax) lmax = xsi[z], lpoint = z;
            if (xsi[z] > rmax) rmax = xsi[z], rpoint = z;
        }

        //Get uwall arrays
        for (z=lpoint; z!=rpoint; z=zz)
        {
            zz = z+1; if (zz == npoints) zz = 0;

            dax1 = ((xsi[z]+65535)>>16);
            dax2 = ((xsi[zz]+65535)>>16);
            if (dax2 > dax1)
            {
                int32_t yinc = divscale16(ysi[zz]-ysi[z],xsi[zz]-xsi[z]);
                y = ysi[z] + mulscale16((dax1<<16)-xsi[z],yinc);
                qinterpolatedown16short((intptr_t)(&uwall[dax1]),dax2-dax1,y,yinc);
            }
        }

        //Get dwall arrays
        for (; z!=lpoint; z=zz)
        {
            zz = z+1; if (zz == npoints) zz = 0;

            dax1 = ((xsi[zz]+65535)>>16);
            dax2 = ((xsi[z]+65535)>>16);
            if (dax2 > dax1)
            {
                int32_t yinc = divscale16(ysi[zz]-ysi[z],xsi[zz]-xsi[z]);
                y = ysi[zz] + mulscale16((dax1<<16)-xsi[zz],yinc);
                qinterpolatedown16short((intptr_t)(&dwall[dax1]),dax2-dax1,y,yinc);
            }
        }


        const int32_t lx = ((lmax+65535)>>16);
        const int32_t rx = min(((rmax+65535)>>16), xdim-1);
        // min(): OOB prevention. Simple test case: have a floor-aligned sprite
        // to the right of the player. Slowly rotate right toward it. When it
        // just becomes visible, the condition rx == xdim can occur.

        // Don't pointlessly keep going. If the following condition holds, the
        // ceilspritescan() at the end of our block would not draw any lines,
        // and moreover may access uwall[] OOB (with x1==xdim).
        if (rx-1 < lx)
            return;

        for (x=lx; x<=rx; x++)
        {
            uwall[x] = max(uwall[x],startumost[x+windowxy1.x]-windowxy1.y);
            dwall[x] = min(dwall[x],startdmost[x+windowxy1.x]-windowxy1.y);
        }

        //Additional uwall/dwall clipping goes here
        for (i=smostwallcnt-1; i>=0; i--)
        {
            j = smostwall[i];
            if ((xb1[j] > rx) || (xb2[j] < lx)) continue;
            if ((yp <= yb1[j]) && (yp <= yb2[j])) continue;

            //if (spritewallfront(tspr,thewall[j]) == 0)
            x = thewall[j]; xp1 = wall[x].x; yp1 = wall[x].y;
            x = wall[x].point2; xp2 = wall[x].x; yp2 = wall[x].y;
            x = (xp2-xp1)*(tspr->y-yp1)-(tspr->x-xp1)*(yp2-yp1);
            if ((yp > yb1[j]) && (yp > yb2[j])) x = -1;
            if ((x >= 0) && ((x != 0) || (wall[thewall[j]].nextsector != tspr->sectnum))) continue;

            const int32_t dalx2 = max(xb1[j],lx);
            const int32_t darx2 = min(xb2[j],rx);

            switch (smostwalltype[i])
            {
            case 0:
                if (dalx2 <= darx2)
                {
                    if ((dalx2 == lx) && (darx2 == rx)) return;
                    //clearbufbyte(&dwall[dalx2],(darx2-dalx2+1)*sizeof(dwall[0]),0L);
                    for (x=dalx2; x<=darx2; x++) dwall[x] = 0;
                }
                break;
            case 1:
                k = smoststart[i] - xb1[j];
                for (x=dalx2; x<=darx2; x++)
                    if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
                break;
            case 2:
                k = smoststart[i] - xb1[j];
                for (x=dalx2; x<=darx2; x++)
                    if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
                break;
            }
        }

        //sprite
#ifdef YAX_ENABLE
        if (yax_globallev==YAX_MAXDRAWS || searchit==2)
#endif
        if (searchit >= 1 && (searchx >= lx && searchx <= rx))
            if (searchy >= uwall[searchx] && searchy <= dwall[searchx])
            {
                searchsector = sectnum; searchwall = spritenum;
                searchstat = 3; searchit = 1;
            }

        globalorientation = cstat;
        globalpicnum = tilenum;
        if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;

        if (waloff[globalpicnum] == 0) loadtile(globalpicnum);
        setgotpic(globalpicnum);
        globalbufplc = waloff[globalpicnum];

        globvis = mulscale16(globalhisibility,viewingrange);
        if (sec->visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sec->visibility+16));

        x = picsiz[globalpicnum]; y = ((x>>4)&15); x &= 15;
#if 0
        if (pow2long[x] != xspan)
        {
            x++;
            globalx1 = mulscale(globalx1,xspan,x);
            globalx2 = mulscale(globalx2,xspan,x);
        }
#endif
        dax = globalxpanning; day = globalypanning;
        globalxpanning = -dmulscale6(globalx1,day,globalx2,dax);
        globalypanning = -dmulscale6(globaly1,day,globaly2,dax);

        globalx2 = mulscale16(globalx2,viewingrange);
        globaly2 = mulscale16(globaly2,viewingrange);
        globalzd = mulscale16(globalzd,viewingrangerecip);

        globalx1 = (globalx1-globalx2)*halfxdimen;
        globaly1 = (globaly1-globaly2)*halfxdimen;

        if ((cstat&2) == 0)
            msethlineshift(x,y);
        else
            tsethlineshift(x,y);

        globalispow2 = (pow2long[x]==xspan && pow2long[y]==yspan);
        globalxspan = xspan;
        globalyspan = yspan;

        //Draw it!
        ceilspritescan(lx,rx-1);
        globalispow2 = 1;
    }
    else if ((cstat&48) == 48)
    {
        const int32_t daxrepeat = ((sprite[spritenum].cstat&48)==16) ?
            (tspr->xrepeat * 5) / 4 :
            tspr->xrepeat;

        const int32_t lx = 0, rx = xdim-1;

        for (x=lx; x<=rx; x++)
        {
            lwall[x] = startumost[x+windowxy1.x]-windowxy1.y;
            swall[x] = startdmost[x+windowxy1.x]-windowxy1.y;
        }
        for (i=smostwallcnt-1; i>=0; i--)
        {
            j = smostwall[i];
            if ((xb1[j] > rx) || (xb2[j] < lx)) continue;
            if ((yp <= yb1[j]) && (yp <= yb2[j])) continue;
            if (spritewallfront(tspr,(int32_t)thewall[j]) && ((yp <= yb1[j]) || (yp <= yb2[j]))) continue;

            const int32_t dalx2 = max(xb1[j],lx);
            const int32_t darx2 = min(xb2[j],rx);

            switch (smostwalltype[i])
            {
            case 0:
                if (dalx2 <= darx2)
                {
                    if ((dalx2 == lx) && (darx2 == rx)) return;
                    //clearbufbyte(&swall[dalx2],(darx2-dalx2+1)*sizeof(swall[0]),0L);
                    for (x=dalx2; x<=darx2; x++) swall[x] = 0;
                }
                break;
            case 1:
                k = smoststart[i] - xb1[j];
                x = dalx2;
#ifdef CLASSIC_SLICE_BY_4
                for (; x<=darx2-2; x+=2)
                {
                    if (smost[k+x] > lwall[x]) lwall[x] = smost[k+x];
                    if (smost[k+x+1] > lwall[x+1]) lwall[x+1] = smost[k+x+1];
                }
#endif
                for (; x<=darx2; x++)
                    if (smost[k+x] > lwall[x]) lwall[x] = smost[k+x];
                break;
            case 2:
                k = smoststart[i] - xb1[j];
                x = dalx2;
#ifdef CLASSIC_SLICE_BY_4
                for (; x<=darx2-4; x+=4)
                {
                    if (smost[k+x] < swall[x]) swall[x] = smost[k+x];
                    if (smost[k+x+1] < swall[x+1]) swall[x+1] = smost[k+x+1];
                    if (smost[k+x+2] < swall[x+2]) swall[x+2] = smost[k+x+2];
                    if (smost[k+x+3] < swall[x+3]) swall[x+3] = smost[k+x+3];
                }
#endif
                for (; x<=darx2; x++)
                    if (smost[k+x] < swall[x]) swall[x] = smost[k+x];
                break;
            }
        }

        if (lwall[rx] >= swall[rx])
        {
            for (x=lx; x<rx; x++)
                if (lwall[x] < swall[x]) break;
            if (x == rx) return;
        }
/*
        for (i=0; i<MAXVOXMIPS; i++)
            if (!voxoff[vtilenum][i])
            {
                kloadvoxel(vtilenum);
                break;
            }
*/
        const int32_t *const longptr = (int32_t *)voxoff[vtilenum][0];
        if (longptr == NULL)
        {
            globalshade = 32;
            tspr->xrepeat = tspr->yrepeat = 255;
            goto draw_as_face_sprite;
        }

        int32_t nxrepeat, nyrepeat;

        if (voxscale[vtilenum] == 65536)
        {
            nxrepeat = (daxrepeat<<16);
            nyrepeat = (((int32_t)tspr->yrepeat)<<16);
        }
        else
        {
            nxrepeat = daxrepeat*voxscale[vtilenum];
            nyrepeat = ((int32_t)tspr->yrepeat)*voxscale[vtilenum];
        }

        if (!(cstat&128)) tspr->z -= mulscale22(B_LITTLE32(longptr[5]),nyrepeat);
        off.y = /*picanm[sprite[tspr->owner].picnum].yofs +*/ tspr->yoffset;
        tspr->z -= mulscale14(off.y,nyrepeat);

        globvis = globalvisibility;
        if (sec->visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sec->visibility+16));

#ifdef YAX_ENABLE
        if (yax_globallev==YAX_MAXDRAWS || searchit==2)
#endif
        if (searchit >= 1 && yp > (4<<8) && (searchy >= lwall[searchx] && searchy < swall[searchx]))
        {
            int32_t const xdsiz = divscale19(xdimenscale,yp);
            int32_t const xv = mulscale16(nxrepeat,xyaspect);

            int32_t const xspan = ((B_LITTLE32(longptr[0]) + B_LITTLE32(longptr[1])) >> 1);
            int32_t const yspan = B_LITTLE32(longptr[2]);

            vec2_t const siz = { mulscale_triple30(xdsiz, xv, xspan), mulscale_triple30(xdsiz, nyrepeat, yspan) };

            //Watch out for divscale overflow
            if (((xspan>>11) < siz.x) && (yspan < (siz.y>>1)))
            {
                x1 = xb-(siz.x>>1);
                if (xspan&1) x1 += mulscale31(xdsiz,xv);  //Odd xspans
                i = mulscale30(xdsiz,xv*off.x);
                if ((cstat&4) == 0) x1 -= i; else x1 += i;

                y1 = mulscale16(tspr->z-globalposz,xdsiz);
                //y1 -= mulscale30(xdsiz,nyrepeat*yoff);
                y1 += (globalhoriz<<8)-siz.y;
                //if (cstat&128)  //Already fixed up above
                y1 += (siz.y>>1);

                x2 = x1+siz.x-1;
                y2 = y1+siz.y-1;
                if ((y1|255) < (y2|255) && searchx >= (x1>>8)+1 && searchx <= (x2>>8))
                {
                    int32_t startum, startdm;

                    if ((sec->ceilingstat&3) == 0)
                        startum = globalhoriz+mulscale24(xdsiz,sec->ceilingz-globalposz)-1;
                    else
                        startum = 0;

                    if ((sec->floorstat&3) == 0)
                        startdm = globalhoriz+mulscale24(xdsiz,sec->floorz-globalposz)+1;
                    else
                        startdm = INT32_MAX;

                    //sprite
                    if (searchy >= max(startum,(y1>>8)) && searchy < min(startdm,(y2>>8)))
                    {
                        searchsector = sectnum; searchwall = spritenum;
                        searchstat = 3; searchit = 1;
                    }
                }
            }
        }

        i = (int32_t)tspr->ang+1536;
        i += spriteext[spritenum].angoff;
        drawvox(tspr->x,tspr->y,tspr->z,i,daxrepeat,(int32_t)tspr->yrepeat,vtilenum,tspr->shade,tspr->pal,lwall,swall);
    }
}

static void drawsprite(int32_t snum)
{
    switch (getrendermode())
    {
    case REND_CLASSIC:
        drawsprite_classic(snum);
        return;
#ifdef USE_OPENGL
    case REND_POLYMOST:
        polymost_drawsprite(snum);
        bglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        bglDepthFunc(GL_LESS); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS
//        bglDepthRange(0.0, 1.0); //<- this is more widely supported than glPolygonOffset
        return;
# ifdef POLYMER
    case REND_POLYMER:
        bglEnable(GL_ALPHA_TEST);
        bglEnable(GL_BLEND);
        polymer_drawsprite(snum);
        bglDisable(GL_BLEND);
        bglDisable(GL_ALPHA_TEST);
        return;
# endif
#endif
    }
}


//
// drawmaskwall (internal)
//
static void drawmaskwall(int16_t damaskwallcnt)
{
    //============================================================================= //POLYMOST BEGINS
#ifdef USE_OPENGL
    if (getrendermode() == REND_POLYMOST) { polymost_drawmaskwall(damaskwallcnt); return; }
# ifdef POLYMER
    if (getrendermode() == REND_POLYMER)
    {
        bglEnable(GL_ALPHA_TEST);
        bglEnable(GL_BLEND);

        polymer_drawmaskwall(damaskwallcnt);

        bglDisable(GL_BLEND);
        bglDisable(GL_ALPHA_TEST);

        return;
    }
#endif
#endif
    //============================================================================= //POLYMOST ENDS

    int32_t z = maskwall[damaskwallcnt];
    uwalltype *wal = (uwalltype *)&wall[thewall[z]];
    int32_t sectnum = thesector[z];
    usectortype *sec = (usectortype *)&sector[sectnum];
    usectortype *nsec = (usectortype *)&sector[wal->nextsector];
    int32_t z1 = max(nsec->ceilingz,sec->ceilingz);
    int32_t z2 = min(nsec->floorz,sec->floorz);

    wallmost(uwall,z,sectnum,(uint8_t)0);
    wallmost(uplc,z,(int32_t)wal->nextsector,(uint8_t)0);
    for (bssize_t x=xb1[z]; x<=xb2[z]; x++)
        if (uplc[x] > uwall[x])
            uwall[x] = uplc[x];
    wallmost(dwall,z,sectnum,(uint8_t)1);
    wallmost(dplc,z,(int32_t)wal->nextsector,(uint8_t)1);
    for (bssize_t x=xb1[z]; x<=xb2[z]; x++)
        if (dplc[x] < dwall[x])
            dwall[x] = dplc[x];
    prepwall(z,wal);

    setup_globals_wall1(wal, wal->overpicnum);
    setup_globals_wall2(wal, sec->visibility, z1, z2);

    for (bssize_t i=smostwallcnt-1; i>=0; i--)
    {
        int j=smostwall[i];
        if ((xb1[j] > xb2[z]) || (xb2[j] < xb1[z])) continue;
        if (wallfront(j,z)) continue;

        int lx = max(xb1[j],xb1[z]);
        int rx = min(xb2[j],xb2[z]);

        switch (smostwalltype[i])
        {
        case 0:
            if (lx <= rx)
            {
                if ((lx == xb1[z]) && (rx == xb2[z])) return;
                //clearbufbyte(&dwall[lx],(rx-lx+1)*sizeof(dwall[0]),0L);
                for (bssize_t x=lx; x<=rx; x++) dwall[x] = 0;
            }
            break;
        case 1:
            for (bssize_t x=lx, k = smoststart[i] - xb1[j]; x<=rx; x++)
                if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
            break;
        case 2:
            for (bssize_t x=lx, k = smoststart[i] - xb1[j]; x<=rx; x++)
                if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
            break;
        }
    }

    //maskwall
    if (searchit >= 1 && (searchx >= xb1[z] && searchx <= xb2[z]))
        if (searchy >= uwall[searchx] && searchy <= dwall[searchx])
        {
            searchsector = sectnum; searchbottomwall = searchwall = thewall[z];
            searchstat = 4; searchit = 1;
        }

    if ((globalorientation&128) == 0)
    {
        maskwallscan(xb1[z],xb2[z], 0);
    }
    else
    {
        if (globalorientation&128)
#ifdef NEW_MAP_FORMAT
            setup_blend(wal->blend, globalorientation&512);
#else
            setup_blend(wallext[thewall[z]].blend, globalorientation&512);
#endif
        transmaskwallscan(xb1[z],xb2[z], 0);
    }
}


//
// fillpolygon (internal)
//
static void fillpolygon(int32_t npoints)
{
    int32_t i, z, y, miny, maxy;

    // fix for bad next-point (xb1) values...
    for (z=0; z<npoints; z++)
        if ((unsigned)xb1[z] >= (unsigned)npoints)
            xb1[z] = 0;

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST && in3dmode())
    {
        polymost_fillpolygon(npoints);
        return;
    }
#endif

    // 1. Calculate y bounds.
    miny = INT32_MAX; maxy = INT32_MIN;
    for (z=npoints-1; z>=0; z--)
    {
        y = ry1[z];
        miny = min(miny,y);
        maxy = max(maxy,y);
    }

    miny >>= 12;
    maxy >>= 12;

    if (miny < 0)
        miny = 0;
    if (maxy >= ydim)
        maxy = ydim-1;

    for (i=0, y=miny; y<=maxy; y++, i++)
    {
        //They're pointers! - watch how you optimize this thing
        dotp1[y] = &smost[i*nodesperline];
        dotp2[y] = &smost[i*nodesperline + (nodesperline>>1)];
    }

    for (z=npoints-1; z>=0; z--)
    {
        const int32_t zz=xb1[z];

        // NOTE: clamp for crash prevention... :-/
        // r1874 says: "Fix more overheadmap crashes, this time with 'Last
        // Pissed Time'"
        const int32_t y1 = clamp(ry1[z], 0, (ydim<<12)-1);
        const int32_t y2 = clamp(ry1[zz], 0, (ydim<<12)-1);

        const int32_t day1 = y1>>12;
        const int32_t day2 = y2>>12;

        if (day1 != day2)
        {
            int32_t x1=rx1[z], x2=rx1[zz];
            const int32_t xinc = divscale12(x2-x1, y2-y1);

            if (day2 > day1)
            {
                x1 += mulscale12((day1<<12)+4095-y1, xinc);
                for (y=day1; y<day2; y++)
                {
                    Bassert(dotp2[y]);
                    *(dotp2[y]++) = x1>>12;
                    x1 += xinc;
                }
            }
            else
            {
                x2 += mulscale12((day2<<12)+4095-y2, xinc);
                for (y=day2; y<day1; y++)
                {
                    Bassert(dotp1[y]);
                    *(dotp1[y]++) = x2>>12;
                    x2 += xinc;
                }
            }
        }
    }

    globalx1 = mulscale16(globalx1,xyaspect);
    globaly2 = mulscale16(globaly2,xyaspect);

    {
        const int32_t oy = miny+1-(ydim>>1);
        globalposx += oy*(int64_t)globalx1;
        globalposy += oy*(int64_t)globaly2;
    }

    setuphlineasm4(asm1,asm2);

    for (i=0, y=miny; y<=maxy; y++, i++)
    {
        int16_t *const xptr = &smost[i*nodesperline];
        int16_t *const xptr2 = &smost[i*nodesperline + (nodesperline>>1)];

        const bssize_t cnt = dotp1[y]-xptr;

        for (z=cnt-1; z>=0; z--)
        {
            int32_t x1, x2;
            int32_t zz, i1=0, i2=0;  // point indices (like loop z)

            for (zz=z; zz>0; zz--)
            {
                if (xptr[zz] < xptr[i1])
                    i1 = zz;
                if (xptr2[zz] < xptr2[i2])
                    i2 = zz;
            }

            x1 = xptr[i1];
            xptr[i1] = xptr[z];

            x2 = xptr2[i2]-1;
            xptr2[i2] = xptr2[z];

            if (x1 > x2)
                continue;

            if ((unsigned)x1 >= xdim+0u || (unsigned)x2 >= xdim+0u)
                continue;

            if (globalpolytype < 1)
            {
                //maphline
                const int32_t ox = x2+1-(xdim>>1);

                hlineasm4(x2 - x1, -1L, globalshade << 8,
                          ox * asm2 - globalposy, ox * asm1 + globalposx,
                          ylookup[y] + x2 + frameplace);
            }
            else
            {
                //maphline
                const int32_t ox = x1+1-(xdim>>1);
                const int32_t bx = ox*asm1 + globalposx;
                const int32_t by = ox*asm2 - globalposy;

                const intptr_t p = ylookup[y]+x1+frameplace;

                if (globalpolytype == 1)
                    mhline(globalbufplc,bx,(x2-x1)<<16,0L,by,p);
                else
                    thline(globalbufplc,bx,(x2-x1)<<16,0L,by,p);
            }
        }

        globalposx += (int64_t)globalx1;
        globalposy += (int64_t)globaly2;
    }

    faketimerhandler();
}

static inline int32_t addscaleclamp(int32_t a, int32_t b, int32_t s1, int32_t s2)
{
    // a + scale(b, s1, s1-s2), but without arithmetic exception when the
    // scale() expression overflows

    int64_t tmp = (int64_t)a + tabledivide64((int64_t)b*s1, s1-s2);

    if (EDUKE32_PREDICT_FALSE(tmp <= INT32_MIN+1))
        return INT32_MIN+1;
    if (EDUKE32_PREDICT_FALSE(tmp >= INT32_MAX))
        return INT32_MAX;
    return tmp;
}

//
// clippoly (internal)
//
static int32_t clippoly(int32_t npoints, int32_t clipstat)
{
    int32_t z, zz, s1, s2, t, npoints2, start2, z1, z2, z3, z4, splitcnt;
    int32_t cx1, cy1, cx2, cy2;

    cx1 = windowxy1.x;
    cy1 = windowxy1.y;
    cx2 = windowxy2.x+1;
    cy2 = windowxy2.y+1;
    cx1 <<= 12; cy1 <<= 12; cx2 <<= 12; cy2 <<= 12;

    if (clipstat&0xa)   //Need to clip top or left
    {
        npoints2 = 0; start2 = 0; z = 0; splitcnt = 0;
        do
        {
            s2 = cx1-rx1[z];
            do
            {
                zz = xb1[z]; xb1[z] = -1;
                s1 = s2; s2 = cx1-rx1[zz];
                if (s1 < 0)
                {
                    rx2[npoints2] = rx1[z]; ry2[npoints2] = ry1[z];
                    xb2[npoints2] = npoints2+1; npoints2++;
                }
                if ((s1^s2) < 0)
                {
                    rx2[npoints2] = addscaleclamp(rx1[z], rx1[zz]-rx1[z], s1, s2);
                    ry2[npoints2] = addscaleclamp(ry1[z], ry1[zz]-ry1[z], s1, s2);
                    if (s1 < 0) bunchp2[splitcnt++] = npoints2;
                    xb2[npoints2] = npoints2+1;
                    npoints2++;
                }
                z = zz;
            }
            while (xb1[z] >= 0);

            if (npoints2 >= start2+3)
                xb2[npoints2-1] = start2, start2 = npoints2;
            else
                npoints2 = start2;

            z = 1;
            while ((z < npoints) && (xb1[z] < 0)) z++;
        }
        while (z < npoints);
        if (npoints2 <= 2) return 0;

        for (z=1; z<splitcnt; z++)
            for (zz=0; zz<z; zz++)
            {
                z1 = bunchp2[z]; z2 = xb2[z1]; z3 = bunchp2[zz]; z4 = xb2[z3];
                s1  = klabs(rx2[z1]-rx2[z2])+klabs(ry2[z1]-ry2[z2]);
                s1 += klabs(rx2[z3]-rx2[z4])+klabs(ry2[z3]-ry2[z4]);
                s2  = klabs(rx2[z1]-rx2[z4])+klabs(ry2[z1]-ry2[z4]);
                s2 += klabs(rx2[z3]-rx2[z2])+klabs(ry2[z3]-ry2[z2]);
                if (s2 < s1)
                    { t = xb2[bunchp2[z]]; xb2[bunchp2[z]] = xb2[bunchp2[zz]]; xb2[bunchp2[zz]] = t; }
            }


        npoints = 0; start2 = 0; z = 0; splitcnt = 0;
        do
        {
            s2 = cy1-ry2[z];
            do
            {
                zz = xb2[z]; xb2[z] = -1;
                s1 = s2; s2 = cy1-ry2[zz];
                if (s1 < 0)
                {
                    rx1[npoints] = rx2[z]; ry1[npoints] = ry2[z];
                    xb1[npoints] = npoints+1; npoints++;
                }
                if ((s1^s2) < 0)
                {
                    rx1[npoints] = addscaleclamp(rx2[z], rx2[zz]-rx2[z], s1, s2);
                    ry1[npoints] = addscaleclamp(ry2[z], ry2[zz]-ry2[z], s1, s2);
                    if (s1 < 0) bunchp2[splitcnt++] = npoints;
                    xb1[npoints] = npoints+1;
                    npoints++;
                }
                z = zz;
            }
            while (xb2[z] >= 0);

            if (npoints >= start2+3)
                xb1[npoints-1] = start2, start2 = npoints;
            else
                npoints = start2;

            z = 1;
            while ((z < npoints2) && (xb2[z] < 0)) z++;
        }
        while (z < npoints2);
        if (npoints <= 2) return 0;

        for (z=1; z<splitcnt; z++)
            for (zz=0; zz<z; zz++)
            {
                z1 = bunchp2[z]; z2 = xb1[z1]; z3 = bunchp2[zz]; z4 = xb1[z3];
                s1  = klabs(rx1[z1]-rx1[z2])+klabs(ry1[z1]-ry1[z2]);
                s1 += klabs(rx1[z3]-rx1[z4])+klabs(ry1[z3]-ry1[z4]);
                s2  = klabs(rx1[z1]-rx1[z4])+klabs(ry1[z1]-ry1[z4]);
                s2 += klabs(rx1[z3]-rx1[z2])+klabs(ry1[z3]-ry1[z2]);
                if (s2 < s1)
                    { t = xb1[bunchp2[z]]; xb1[bunchp2[z]] = xb1[bunchp2[zz]]; xb1[bunchp2[zz]] = t; }
            }
    }
    if (clipstat&0x5)   //Need to clip bottom or right
    {
        npoints2 = 0; start2 = 0; z = 0; splitcnt = 0;
        do
        {
            s2 = rx1[z]-cx2;
            do
            {
                zz = xb1[z]; xb1[z] = -1;
                s1 = s2; s2 = rx1[zz]-cx2;
                if (s1 < 0)
                {
                    rx2[npoints2] = rx1[z]; ry2[npoints2] = ry1[z];
                    xb2[npoints2] = npoints2+1; npoints2++;
                }
                if ((s1^s2) < 0)
                {
                    rx2[npoints2] = addscaleclamp(rx1[z], rx1[zz]-rx1[z], s1, s2);
                    ry2[npoints2] = addscaleclamp(ry1[z], ry1[zz]-ry1[z], s1, s2);
                    if (s1 < 0) bunchp2[splitcnt++] = npoints2;
                    xb2[npoints2] = npoints2+1;
                    npoints2++;
                }
                z = zz;
            }
            while (xb1[z] >= 0);

            if (npoints2 >= start2+3)
                xb2[npoints2-1] = start2, start2 = npoints2;
            else
                npoints2 = start2;

            z = 1;
            while ((z < npoints) && (xb1[z] < 0)) z++;
        }
        while (z < npoints);
        if (npoints2 <= 2) return 0;

        for (z=1; z<splitcnt; z++)
            for (zz=0; zz<z; zz++)
            {
                z1 = bunchp2[z]; z2 = xb2[z1]; z3 = bunchp2[zz]; z4 = xb2[z3];
                s1  = klabs(rx2[z1]-rx2[z2])+klabs(ry2[z1]-ry2[z2]);
                s1 += klabs(rx2[z3]-rx2[z4])+klabs(ry2[z3]-ry2[z4]);
                s2  = klabs(rx2[z1]-rx2[z4])+klabs(ry2[z1]-ry2[z4]);
                s2 += klabs(rx2[z3]-rx2[z2])+klabs(ry2[z3]-ry2[z2]);
                if (s2 < s1)
                    { t = xb2[bunchp2[z]]; xb2[bunchp2[z]] = xb2[bunchp2[zz]]; xb2[bunchp2[zz]] = t; }
            }


        npoints = 0; start2 = 0; z = 0; splitcnt = 0;
        do
        {
            s2 = ry2[z]-cy2;
            do
            {
                zz = xb2[z]; xb2[z] = -1;
                s1 = s2; s2 = ry2[zz]-cy2;
                if (s1 < 0)
                {
                    rx1[npoints] = rx2[z]; ry1[npoints] = ry2[z];
                    xb1[npoints] = npoints+1; npoints++;
                }
                if ((s1^s2) < 0)
                {
                    rx1[npoints] = addscaleclamp(rx2[z], rx2[zz]-rx2[z], s1, s2);
                    ry1[npoints] = addscaleclamp(ry2[z], ry2[zz]-ry2[z], s1, s2);
                    if (s1 < 0) bunchp2[splitcnt++] = npoints;
                    xb1[npoints] = npoints+1;
                    npoints++;
                }
                z = zz;
            }
            while (xb2[z] >= 0);

            if (npoints >= start2+3)
                xb1[npoints-1] = start2, start2 = npoints;
            else
                npoints = start2;

            z = 1;
            while ((z < npoints2) && (xb2[z] < 0)) z++;
        }
        while (z < npoints2);
        if (npoints <= 2) return 0;

        for (z=1; z<splitcnt; z++)
            for (zz=0; zz<z; zz++)
            {
                z1 = bunchp2[z]; z2 = xb1[z1]; z3 = bunchp2[zz]; z4 = xb1[z3];
                s1  = klabs(rx1[z1]-rx1[z2])+klabs(ry1[z1]-ry1[z2]);
                s1 += klabs(rx1[z3]-rx1[z4])+klabs(ry1[z3]-ry1[z4]);
                s2  = klabs(rx1[z1]-rx1[z4])+klabs(ry1[z1]-ry1[z4]);
                s2 += klabs(rx1[z3]-rx1[z2])+klabs(ry1[z3]-ry1[z2]);
                if (s2 < s1)
                    { t = xb1[bunchp2[z]]; xb1[bunchp2[z]] = xb1[bunchp2[zz]]; xb1[bunchp2[zz]] = t; }
            }
    }
    return npoints;
}


//
// clippoly4 (internal)
//
//Assume npoints=4 with polygon on &nrx1,&nry1
//JBF 20031206: Thanks to Ken's hunting, s/(rx1|ry1|rx2|ry2)/n\1/ in this function
static int32_t clippoly4(int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2)
{
    int32_t n, nn, z, zz, x, x1, x2, y, y1, y2, t;

    nn = 0; z = 0;
    do
    {
        zz = ((z+1)&3);
        x1 = nrx1[z]; x2 = nrx1[zz]-x1;

        if ((cx1 <= x1) && (x1 <= cx2))
            nrx2[nn] = x1, nry2[nn] = nry1[z], nn++;

        if (x2 <= 0) x = cx2; else x = cx1;
        t = x-x1;
        if (((t-x2)^t) < 0)
            nrx2[nn] = x, nry2[nn] = nry1[z]+scale(t,nry1[zz]-nry1[z],x2), nn++;

        if (x2 <= 0) x = cx1; else x = cx2;
        t = x-x1;
        if (((t-x2)^t) < 0)
            nrx2[nn] = x, nry2[nn] = nry1[z]+scale(t,nry1[zz]-nry1[z],x2), nn++;

        z = zz;
    }
    while (z != 0);
    if (nn < 3) return 0;

    n = 0; z = 0;
    do
    {
        zz = z+1; if (zz == nn) zz = 0;
        y1 = nry2[z]; y2 = nry2[zz]-y1;

        if ((cy1 <= y1) && (y1 <= cy2))
            nry1[n] = y1, nrx1[n] = nrx2[z], n++;

        if (y2 <= 0) y = cy2; else y = cy1;
        t = y-y1;
        if (((t-y2)^t) < 0)
            nry1[n] = y, nrx1[n] = nrx2[z]+scale(t,nrx2[zz]-nrx2[z],y2), n++;

        if (y2 <= 0) y = cy1; else y = cy2;
        t = y-y1;
        if (((t-y2)^t) < 0)
            nry1[n] = y, nrx1[n] = nrx2[z]+scale(t,nrx2[zz]-nrx2[z],y2), n++;

        z = zz;
    }
    while (z != 0);
    return n;
}


// INTERNAL helper function for classic/polymost dorotatesprite
//  sxptr, sxptr, z: in/out
//  ret_yxaspect, ret_xyaspect: out
void dorotspr_handle_bit2(int32_t *sxptr, int32_t *syptr, int32_t *z, int32_t dastat,
                          int32_t cx1_plus_cx2, int32_t cy1_plus_cy2,
                          int32_t *ret_yxaspect, int32_t *ret_xyaspect)
{
    if ((dastat & RS_AUTO) == 0)
    {
        if (!(dastat & RS_STRETCH) && 4*ydim <= 3*xdim)
        {
            *ret_yxaspect = (12<<16)/10;
            *ret_xyaspect = (10<<16)/12;
        }
        else
        {
            *ret_yxaspect = yxaspect;
            *ret_xyaspect = xyaspect;
        }

        // *sxptr and *syptr and *z are left unchanged

        return;
    }
    else
    {
        // dastat&2: Auto window size scaling
        const int32_t oxdim = xdim;
        const int32_t oydim = ydim;
        int32_t xdim = oxdim;  // SHADOWS global
        int32_t ydim = oydim;

        int32_t zoomsc, sx=*sxptr, sy=*syptr;
        int32_t ouryxaspect = yxaspect, ourxyaspect = xyaspect;

        // screen center to s[xy], 320<<16 coords.
        const int32_t normxofs = sx-(320<<15), normyofs = sy-(200<<15);

        if (!(dastat & RS_STRETCH) && 4*ydim <= 3*xdim)
        {
            if ((dastat & RS_ALIGN_MASK) == RS_ALIGN_MASK)
                ydim = scale(xdim, 3, 4);
            else
                xdim = scale(ydim, 4, 3);

            ouryxaspect = (12<<16)/10;
            ourxyaspect = (10<<16)/12;
        }

        // nasty hacks go here
        if (!(dastat & RS_NOCLIP))
        {
            const int32_t twice_midcx = cx1_plus_cx2+2;

            // screen x center to sx1, scaled to viewport
            const int32_t scaledxofs = scale(normxofs, scale(xdimen, xdim, oxdim), 320);

            int32_t xbord = 0;

            if ((dastat & RS_ALIGN_MASK) && (dastat & RS_ALIGN_MASK) != RS_ALIGN_MASK)
            {
                xbord = scale(oxdim-xdim, twice_midcx, oxdim);

                if ((dastat & RS_ALIGN_R)==0)
                    xbord = -xbord;
            }

            sx = ((twice_midcx+xbord)<<15) + scaledxofs;

            zoomsc = xdimenscale;   //= scale(xdimen,yxaspect,320);

            if ((dastat & RS_ALIGN_MASK) == RS_ALIGN_MASK)
                zoomsc = scale(zoomsc, ydim, oydim);

            sy = ((cy1_plus_cy2+2)<<15) + mulscale16(normyofs, zoomsc);
        }
        else
        {
            //If not clipping to startmosts, & auto-scaling on, as a
            //hard-coded bonus, scale to full screen instead

            sx = (xdim<<15)+32768 + scale(normxofs,xdim,320);

            zoomsc = scale(xdim, ouryxaspect, 320);
            sy = (ydim<<15)+32768 + mulscale16(normyofs, zoomsc);

            if ((dastat & RS_ALIGN_MASK) == RS_ALIGN_MASK)
                sy += (oydim-ydim)<<15;
            else if ((dastat & RS_ALIGN_MASK) == RS_ALIGN_R)
                sx += (oxdim-xdim)<<16;
            else if ((dastat & RS_ALIGN_MASK) == 0)
                sx += (oxdim-xdim)<<15;

            if (dastat & RS_CENTERORIGIN)
                sx += oxdim<<15;
        }

        *sxptr = sx;
        *syptr = sy;
        *z = mulscale16(*z, zoomsc);

        *ret_yxaspect = ouryxaspect;
        *ret_xyaspect = ourxyaspect;
    }
}


//
// dorotatesprite (internal)
//
//JBF 20031206: Thanks to Ken's hunting, s/(rx1|ry1|rx2|ry2)/n\1/ in this function
static void dorotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                           int8_t dashade, char dapalnum, int32_t dastat, uint8_t daalpha, uint8_t dablend,
                           int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2,
                           int32_t uniqid)
{
    // NOTE: if these are made unsigned (for safety), angled tiles may draw
    // incorrectly, showing vertical seams at intervals.
    int32_t bx, by;

    int32_t cosang, sinang, v, nextv, dax1, dax2, oy;
    int32_t i, x, y, x1, y1, x2, y2, gx1, gy1;
    intptr_t p, bufplc, palookupoffs;
    int32_t xsiz, ysiz, xoff, yoff, npoints, yplc, yinc, lx, rx;
    int32_t xv, yv, xv2, yv2;

    int32_t ouryxaspect, ourxyaspect;

    if (g_rotatespriteNoWidescreen)
    {
        dastat |= RS_STRETCH;
        dastat &= ~RS_ALIGN_MASK;
    }

    //============================================================================= //POLYMOST BEGINS
#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST && in3dmode())
    {
        polymost_dorotatesprite(sx,sy,z,a,picnum,dashade,dapalnum,dastat,daalpha,dablend,cx1,cy1,cx2,cy2,uniqid);
        return;
    }
#else
    UNREFERENCED_PARAMETER(uniqid);
#endif
    //============================================================================= //POLYMOST ENDS

    // bound clipping rectangle to screen
    if (cx1 < 0) cx1 = 0;
    else if (cx1 > xres-1) cx1 = xres-1;
    if (cy1 < 0) cy1 = 0;
    else if (cy1 > yres-1) cy1 = yres-1;
    if (cx2 < 0) cx2 = 0;
    else if (cx2 > xres-1) cx2 = xres-1;
    if (cy2 < 0) cy2 = 0;
    else if (cy2 > yres-1) cy2 = yres-1;

    xsiz = tilesiz[picnum].x;
    ysiz = tilesiz[picnum].y;

    if (dastat & RS_TOPLEFT)
    {
        // Bit 1<<4 set: origin is top left corner?
        xoff = 0;
        yoff = 0;
    }
    else
    {
        // Bit 1<<4 clear: origin is center of tile, and per-tile offset is applied.
        // TODO: split the two?
        xoff = picanm[picnum].xofs + (xsiz>>1);
        yoff = picanm[picnum].yofs + (ysiz>>1);
    }

    // Bit 1<<2: invert y
    if (dastat & RS_YFLIP)
        yoff = ysiz-yoff;

    cosang = sintable[(a+512)&2047];
    sinang = sintable[a&2047];

    dorotspr_handle_bit2(&sx, &sy, &z, dastat, cx1+cx2, cy1+cy2, &ouryxaspect, &ourxyaspect);

    xv = mulscale14(cosang,z);
    yv = mulscale14(sinang,z);
    if ((dastat&RS_AUTO) || (dastat&RS_NOCLIP) == 0) //Don't aspect unscaled perms
    {
        xv2 = mulscale16(xv,ourxyaspect);
        yv2 = mulscale16(yv,ourxyaspect);
    }
    else
    {
        xv2 = xv;
        yv2 = yv;
    }

    nry1[0] = sy - (yv*xoff + xv*yoff);
    nry1[1] = nry1[0] + yv*xsiz;
    nry1[3] = nry1[0] + xv*ysiz;
    nry1[2] = nry1[1]+nry1[3]-nry1[0];
    i = (cy1<<16); if ((nry1[0]<i) && (nry1[1]<i) && (nry1[2]<i) && (nry1[3]<i)) return;
    i = (cy2<<16); if ((nry1[0]>i) && (nry1[1]>i) && (nry1[2]>i) && (nry1[3]>i)) return;

    nrx1[0] = sx - (xv2*xoff - yv2*yoff);
    nrx1[1] = nrx1[0] + xv2*xsiz;
    nrx1[3] = nrx1[0] - yv2*ysiz;
    nrx1[2] = nrx1[1]+nrx1[3]-nrx1[0];
    i = (cx1<<16); if ((nrx1[0]<i) && (nrx1[1]<i) && (nrx1[2]<i) && (nrx1[3]<i)) return;
    i = (cx2<<16); if ((nrx1[0]>i) && (nrx1[1]>i) && (nrx1[2]>i) && (nrx1[3]>i)) return;

    gx1 = nrx1[0]; gy1 = nry1[0];   //back up these before clipping

    npoints = clippoly4(cx1<<16,cy1<<16,(cx2+1)<<16,(cy2+1)<<16);
    if (npoints < 3) return;

    lx = nrx1[0]; rx = nrx1[0];

    nextv = 0;
    for (v=npoints-1; v>=0; v--)
    {
        x1 = nrx1[v]; x2 = nrx1[nextv];
        dax1 = (x1>>16); if (x1 < lx) lx = x1;
        dax2 = (x2>>16); if (x1 > rx) rx = x1;
        if (dax1 != dax2)
        {
            y1 = nry1[v]; y2 = nry1[nextv];
            yinc = divscale16(y2-y1,x2-x1);
            if (dax2 > dax1)
            {
                yplc = y1 + mulscale16((dax1<<16)+65535-x1,yinc);
                // Assertion fails with DNF mod: in mapster32,
                // set dt_t 3864  (bike HUD, 700x220)
                // set dt_a 100
                // set dt_z 1280000  <- CRASH!
                Bassert((unsigned)dax1 < MAXXDIM && (unsigned)dax2 < MAXXDIM+1);
                qinterpolatedown16short((intptr_t)&uplc[dax1], dax2-dax1, yplc, yinc);
            }
            else
            {
                yplc = y2 + mulscale16((dax2<<16)+65535-x2,yinc);
                Bassert((unsigned)dax2 < MAXXDIM && (unsigned)dax1 < MAXXDIM+1);
                qinterpolatedown16short((intptr_t)&dplc[dax2], dax1-dax2, yplc, yinc);
            }
        }
        nextv = v;
    }

    if (waloff[picnum] == 0) loadtile(picnum);
    setgotpic(picnum);
    bufplc = waloff[picnum];

    if (palookup[dapalnum] == NULL) dapalnum = 0;
    palookupoffs = FP_OFF(palookup[dapalnum]) + (getpalookup(0, dashade)<<8);

    // Alpha handling
    if (!(dastat&RS_TRANS1) && daalpha > 0)
    {
        if (daalpha == 255)
            return;

        if (numalphatabs != 0)
        {
            if (falpha_to_blend((float)daalpha / 255.0f, &dastat, &dablend, RS_TRANS1, RS_TRANS2))
                return;
        }
        else if (daalpha > 84)
        {
            dastat |= RS_TRANS1;

            if (daalpha > 168)
                dastat |= RS_TRANS2;
            else
                dastat &= ~RS_TRANS2;
        }
    }

    i = divscale32(1L,z);
    xv = mulscale14(sinang,i);
    yv = mulscale14(cosang,i);
    if ((dastat&RS_AUTO) || (dastat&RS_NOCLIP)==0) //Don't aspect unscaled perms
    {
        yv2 = mulscale16(-xv,ouryxaspect);
        xv2 = mulscale16(yv,ouryxaspect);
    }
    else
    {
        yv2 = -xv;
        xv2 = yv;
    }

    x1 = (lx>>16);
    x2 = (rx>>16);

    oy = 0;
    x = (x1<<16)-1-gx1;
    y = (oy<<16)+65535-gy1;
    bx = dmulscale16(x,xv2,y,xv);
    by = dmulscale16(x,yv2,y,yv);

    if (dastat & RS_YFLIP)
    {
        yv = -yv;
        yv2 = -yv2;
        by = (ysiz<<16)-1-by;
    }

#if defined ENGINE_USING_A_C
    if ((dastat&RS_TRANS1)==0 && ((a&1023) == 0) && (ysiz <= 256))  //vlineasm4 has 256 high limit!
#else
    if ((dastat&RS_TRANS1) == 0)
#endif
    {
        int32_t y1ve[4], y2ve[4], u4, d4;

        if (((a&1023) == 0) && (ysiz <= 256))  //vlineasm4 has 256 high limit!
        {
            if (dastat & RS_NOMASK)
                setupvlineasm(24L);
            else
                setupmvlineasm(24L, 0);

            by <<= 8; yv <<= 8; yv2 <<= 8;

            palookupoffse[0] = palookupoffse[1] = palookupoffse[2] = palookupoffse[3] = palookupoffs;
            vince[0] = vince[1] = vince[2] = vince[3] = yv;

            for (x=x1; x<x2; x+=4)
            {
                char bad;
                int32_t xx, xend;

                bad = 15; xend = min(x2-x,4);
                for (xx=0; xx<xend; xx++)
                {
                    bx += xv2;

                    y1 = uplc[x+xx]; y2 = dplc[x+xx];
                    if ((dastat & RS_NOCLIP) == 0)
                    {
                        if (startumost[x+xx] > y1) y1 = startumost[x+xx];
                        if (startdmost[x+xx] < y2) y2 = startdmost[x+xx];
                    }
                    if (y2 <= y1) continue;

                    by += (uint32_t)yv*(y1-oy); oy = y1;

                    // Assertion would fail with DNF mod without (uint32_t) below: in mapster32,
                    // set dt_t 3864  (bike HUD, 700x220)
                    // set dt_z 16777216
                    // <Increase yxaspect by pressing [9]>  <-- CRASH!
                    // (It also fails when wrecking the bike in-game by driving into a wall.)
//                    Bassert(bx >= 0);

                    bufplce[xx] = ((uint32_t)bx>>16)*ysiz+bufplc;
                    vplce[xx] = by;
                    y1ve[xx] = y1;
                    y2ve[xx] = y2-1;
                    bad &= ~pow2char[xx];
                }

                p = x+frameplace;

                u4 = INT32_MIN;
                d4 = INT32_MAX;
                for (xx=0; xx<4; xx++)
                    if (!(bad&pow2char[xx]))
                    {
                        u4 = max(u4, y1ve[xx]);
                        d4 = min(d4, y2ve[xx]);
                    }
                // This version may access uninitialized y?ve[] values with
                // thin tiles, e.g. 3085 (MINIFONT period, 1x5):
//                u4 = max(max(y1ve[0],y1ve[1]),max(y1ve[2],y1ve[3]));
//                d4 = min(min(y2ve[0],y2ve[1]),min(y2ve[2],y2ve[3]));

                if (dastat & RS_NOMASK)
                {
                    if ((bad != 0) || (u4 >= d4))
                    {
                        if (!(bad&1)) prevlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0],vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
                        if (!(bad&2)) prevlineasm1(vince[1],palookupoffse[1],y2ve[1]-y1ve[1],vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
                        if (!(bad&4)) prevlineasm1(vince[2],palookupoffse[2],y2ve[2]-y1ve[2],vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
                        if (!(bad&8)) prevlineasm1(vince[3],palookupoffse[3],y2ve[3]-y1ve[3],vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);
                        continue;
                    }

                    if (u4 > y1ve[0]) vplce[0] = prevlineasm1(vince[0],palookupoffse[0],u4-y1ve[0]-1,vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
                    if (u4 > y1ve[1]) vplce[1] = prevlineasm1(vince[1],palookupoffse[1],u4-y1ve[1]-1,vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
                    if (u4 > y1ve[2]) vplce[2] = prevlineasm1(vince[2],palookupoffse[2],u4-y1ve[2]-1,vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
                    if (u4 > y1ve[3]) vplce[3] = prevlineasm1(vince[3],palookupoffse[3],u4-y1ve[3]-1,vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);

                    if (d4 >= u4) vlineasm4(d4-u4+1, (char *)(ylookup[u4]+p));

                    i = p+ylookup[d4+1];
                    if (y2ve[0] > d4) prevlineasm1(vince[0],palookupoffse[0],y2ve[0]-d4-1,vplce[0],bufplce[0],i+0);
                    if (y2ve[1] > d4) prevlineasm1(vince[1],palookupoffse[1],y2ve[1]-d4-1,vplce[1],bufplce[1],i+1);
                    if (y2ve[2] > d4) prevlineasm1(vince[2],palookupoffse[2],y2ve[2]-d4-1,vplce[2],bufplce[2],i+2);
                    if (y2ve[3] > d4) prevlineasm1(vince[3],palookupoffse[3],y2ve[3]-d4-1,vplce[3],bufplce[3],i+3);
                }
                else
                {
                    if ((bad != 0) || (u4 >= d4))
                    {
                        if (!(bad&1)) mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0],vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
                        if (!(bad&2)) mvlineasm1(vince[1],palookupoffse[1],y2ve[1]-y1ve[1],vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
                        if (!(bad&4)) mvlineasm1(vince[2],palookupoffse[2],y2ve[2]-y1ve[2],vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
                        if (!(bad&8)) mvlineasm1(vince[3],palookupoffse[3],y2ve[3]-y1ve[3],vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);
                        continue;
                    }

                    if (u4 > y1ve[0]) vplce[0] = mvlineasm1(vince[0],palookupoffse[0],u4-y1ve[0]-1,vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
                    if (u4 > y1ve[1]) vplce[1] = mvlineasm1(vince[1],palookupoffse[1],u4-y1ve[1]-1,vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
                    if (u4 > y1ve[2]) vplce[2] = mvlineasm1(vince[2],palookupoffse[2],u4-y1ve[2]-1,vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
                    if (u4 > y1ve[3]) vplce[3] = mvlineasm1(vince[3],palookupoffse[3],u4-y1ve[3]-1,vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);

                    if (d4 >= u4) mvlineasm4(d4-u4+1, (char *)(ylookup[u4]+p));

                    i = p+ylookup[d4+1];
                    if (y2ve[0] > d4) mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-d4-1,vplce[0],bufplce[0],i+0);
                    if (y2ve[1] > d4) mvlineasm1(vince[1],palookupoffse[1],y2ve[1]-d4-1,vplce[1],bufplce[1],i+1);
                    if (y2ve[2] > d4) mvlineasm1(vince[2],palookupoffse[2],y2ve[2]-d4-1,vplce[2],bufplce[2],i+2);
                    if (y2ve[3] > d4) mvlineasm1(vince[3],palookupoffse[3],y2ve[3]-d4-1,vplce[3],bufplce[3],i+3);
                }

                faketimerhandler();
            }
        }
#ifndef ENGINE_USING_A_C
        else
        {
            int32_t ny1, ny2;
            int32_t qlinemode = 0;

            if (dastat & RS_NOMASK)
            {
                if ((xv2&0x0000ffff) == 0)
                {
                    qlinemode = 1;
                    setupqrhlineasm4(0L,yv2<<16,(xv2>>16)*ysiz+(yv2>>16),palookupoffs,0L,0L);
                }
                else
                {
                    qlinemode = 0;
                    setuprhlineasm4(xv2<<16,yv2<<16,(xv2>>16)*ysiz+(yv2>>16),palookupoffs,ysiz,0L);
                }
            }
            else
                setuprmhlineasm4(xv2<<16,yv2<<16,(xv2>>16)*ysiz+(yv2>>16),palookupoffs,ysiz,0L);

            y1 = uplc[x1];
            if (((dastat & RS_NOCLIP) == 0) && startumost[x1] > y1)
                y1 = startumost[x1];
            y2 = y1;
            for (x=x1; x<x2; x++)
            {
                ny1 = uplc[x]-1; ny2 = dplc[x];
                if ((dastat & RS_NOCLIP) == 0)
                {
                    if (startumost[x]-1 > ny1) ny1 = startumost[x]-1;
                    if (startdmost[x] < ny2) ny2 = startdmost[x];
                }

                if (ny1 < ny2-1)
                {
                    if (ny1 >= y2)
                    {
                        while (y1 < y2-1)
                        {
                            y1++; if ((y1&31) == 0) faketimerhandler();

                            //x,y1
                            bx += xv*(y1-oy); by += yv*(y1-oy); oy = y1;
                            if (dastat & RS_NOMASK)
                            {
                                if (qlinemode) qrhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,0L    ,by<<16,ylookup[y1]+x+frameplace);
                                else rhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                            }
                            else rmhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                        }
                        y1 = ny1;
                    }
                    else
                    {
                        while (y1 < ny1)
                        {
                            y1++; if ((y1&31) == 0) faketimerhandler();

                            //x,y1
                            bx += xv*(y1-oy); by += yv*(y1-oy); oy = y1;
                            if (dastat & RS_NOMASK)
                            {
                                if (qlinemode) qrhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,0L    ,by<<16,ylookup[y1]+x+frameplace);
                                else rhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                            }
                            else rmhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                        }

                        while (y1 > ny1) lastx[y1--] = x;
                    }

                    while (y2 > ny2)
                    {
                        y2--; if ((y2&31) == 0) faketimerhandler();

                        //x,y2
                        bx += xv*(y2-oy); by += yv*(y2-oy); oy = y2;
                        if (dastat & RS_NOMASK)
                        {
                            if (qlinemode) qrhlineasm4(x-lastx[y2],(bx>>16)*ysiz+(by>>16)+bufplc,0L,0L    ,by<<16,ylookup[y2]+x+frameplace);
                            else rhlineasm4(x-lastx[y2],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y2]+x+frameplace);
                        }
                        else rmhlineasm4(x-lastx[y2],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y2]+x+frameplace);
                    }

                    while (y2 < ny2) lastx[y2++] = x;
                }
                else
                {
                    while (y1 < y2-1)
                    {
                        y1++; if ((y1&31) == 0) faketimerhandler();

                        //x,y1
                        bx += xv*(y1-oy); by += yv*(y1-oy); oy = y1;
                        if (dastat & RS_NOMASK)
                        {
                            if (qlinemode) qrhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,0L    ,by<<16,ylookup[y1]+x+frameplace);
                            else rhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                        }
                        else rmhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                    }

                    if (x == x2-1) { bx += xv2; by += yv2; break; }
                    y1 = uplc[x+1];
                    if (((dastat & RS_NOCLIP) == 0) && startumost[x+1] > y1)
                        y1 = startumost[x+1];
                    y2 = y1;
                }
                bx += xv2; by += yv2;
            }

            while (y1 < y2-1)
            {
                y1++; if ((y1&31) == 0) faketimerhandler();

                //x2,y1
                bx += xv*(y1-oy); by += yv*(y1-oy); oy = y1;
                if (dastat & RS_NOMASK)
                {
                    if (qlinemode) qrhlineasm4(x2-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,0L,by<<16,ylookup[y1]+x2+frameplace);
                    else rhlineasm4(x2-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x2+frameplace);
                }
                else rmhlineasm4(x2-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x2+frameplace);
            }
        }
#endif  // !defined ENGINE_USING_A_C
    }
    else
    {
        if ((dastat & RS_TRANS1) == 0)
        {
#if !defined ENGINE_USING_A_C
            if (dastat & RS_NOMASK)
                setupspritevline(palookupoffs,(xv>>16)*ysiz,xv<<16,ysiz,yv,0L);
            else
                msetupspritevline(palookupoffs,(xv>>16)*ysiz,xv<<16,ysiz,yv,0L);
#else
            if (dastat & RS_NOMASK)
                setupspritevline(palookupoffs,xv,yv,ysiz);
            else
                msetupspritevline(palookupoffs,xv,yv,ysiz);
#endif
        }
        else
        {
#if !defined ENGINE_USING_A_C
            tsetupspritevline(palookupoffs,(xv>>16)*ysiz,xv<<16,ysiz,yv,0L);
#else
            tsetupspritevline(palookupoffs,xv,yv,ysiz);
#endif
            setup_blend(dablend, dastat & RS_TRANS2);
        }

        for (x=x1; x<x2; x++)
        {
            bx += xv2; by += yv2;

            y1 = uplc[x]; y2 = dplc[x];
            if ((dastat & RS_NOCLIP) == 0)
            {
                if (startumost[x] > y1) y1 = startumost[x];
                if (startdmost[x] < y2) y2 = startdmost[x];
            }
            if (y2 <= y1) continue;

            switch (y1-oy)
            {
            case -1:
                bx -= xv; by -= yv; oy = y1; break;
            case 0:
                break;
            case 1:
                bx += xv; by += yv; oy = y1; break;
            default:
                bx += xv*(y1-oy); by += yv*(y1-oy); oy = y1; break;
            }

            p = ylookup[y1]+x+frameplace;

            if ((dastat & RS_TRANS1) == 0)
            {
#if !defined ENGINE_USING_A_C
                if (dastat & RS_NOMASK)
                    spritevline(0L,by<<16,y2-y1+1,bx<<16,(bx>>16)*ysiz+(by>>16)+bufplc,p);
                else
                    mspritevline(0L,by<<16,y2-y1+1,bx<<16,(bx>>16)*ysiz+(by>>16)+bufplc,p);
#else
                if (dastat & RS_NOMASK)
                    spritevline(bx&65535,by&65535,y2-y1+1,(bx>>16)*ysiz+(by>>16)+bufplc,p);
                else
                    mspritevline(bx&65535,by&65535,y2-y1+1,(bx>>16)*ysiz+(by>>16)+bufplc,p);
#endif
            }
            else
            {
#if !defined ENGINE_USING_A_C
                tspritevline(0L,by<<16,y2-y1+1,bx<<16,(bx>>16)*ysiz+(by>>16)+bufplc,p);
#else
                tspritevline(bx&65535,by&65535,y2-y1+1,(bx>>16)*ysiz+(by>>16)+bufplc,p);
                //transarea += (y2-y1);
#endif
            }

            faketimerhandler();
        }
    }

    /*  if ((dastat & RS_PERM) && (origbuffermode == 0))
        {
            buffermode = obuffermode;
            setactivepage(activepage);
        }*/
}

static uint32_t msqrtasm(uint32_t c)
{
    uint32_t a = 0x40000000l, b = 0x20000000l;

    do
    {
        if (c >= a)
        {
            c -= a;
            a += b*4;
        }
        a -= b;
        a >>= 1;
        b >>= 2;
    } while (b);

    if (c >= a)
        a++;

    return a >> 1;
}

//
// initksqrt (internal)
//
static inline void initksqrt(void)
{
    int32_t i, j, k;

    j = 1; k = 0;
    for (i=0; i<4096; i++)
    {
        if (i >= j) { j <<= 2; k++; }
        sqrtable[i] = (uint16_t)(msqrtasm((i<<18)+131072)<<1);
        shlookup[i] = (k<<1)+((10-k)<<8);
        if (i < 256) shlookup[i+4096] = ((k+6)<<1)+((10-(k+6))<<8);
    }
}


//
// dosetaspect
//
static void dosetaspect(void)
{
    int32_t i, j;

    if (xyaspect != oxyaspect)
    {
        oxyaspect = xyaspect;
        j = xyaspect*320;
        horizlookup2[horizycent-1] = divscale26(131072,j);

        for (i=0; i < horizycent-1; i++)
        {
            horizlookup[i] = divscale28(1, i-(horizycent-1));
            horizlookup2[i] = divscale14(klabs(horizlookup[i]), j);
        }

        for (i=horizycent; i < ydim*4-1; i++)
        {
            horizlookup[i] = divscale28(1, i-(horizycent-1));
            horizlookup2[i] = divscale14(klabs(horizlookup[i]), j);
        }
    }

    if (xdimen != oxdimen || viewingrange != oviewingrange)
    {
        int32_t k, x, xinc;

        no_radarang2 = 0;
        oviewingrange = viewingrange;

        xinc = mulscale32(viewingrange*320,xdimenrecip);
        x = (640<<16)-mulscale1(xinc,xdimen);

        for (i=0; i<xdimen; i++)
        {
            j = (x&65535); k = (x>>16); x += xinc;

            if (k < 0 || k >= (int32_t)ARRAY_SIZE(radarang)-1)
            {
                no_radarang2 = 1;
#ifdef DEBUGGINGAIDS
                if (editstatus)
                    initprintf("no rad2\n");
#endif
                break;
            }

            if (j != 0)
                j = mulscale16(radarang[k+1]-radarang[k], j);
            radarang2[i] = (int16_t)((radarang[k]+j)>>6);
        }

        if (xdimen != oxdimen && voxoff[0][0])
        {
            if (distrecip == NULL)
                distrecip = (uint32_t *)Xaligned_alloc(16, DISTRECIPSIZ * sizeof(uint32_t));

            if (xdimen < 1 << 11)
            {
                for (i = 1; i < DISTRECIPSIZ; i++)
                    distrecip[i] = tabledivide32(xdimen << 20, i);
            }
            else
            {
                for (i = 1; i < DISTRECIPSIZ; i++)
                    distrecip[i] = tabledivide64((uint64_t)xdimen << 20, i);
            }

            nytooclose = xdimen*2100;
        }

        oxdimen = xdimen;
    }
}


//
// loadtables (internal)
//
static inline void calcbritable(void)
{
    int32_t i, j;
    float a, b;

    for (i=0; i<16; i++)
    {
        a = 8.f / ((float)i+8.f);
        b = 255.f / powf(255.f, a);

        for (j=0; j<256; j++) // JBF 20040207: full 8bit precision
            britable[i][j] = (uint8_t) (powf((float)j, a) * b);
    }
}

#define BANG2RAD (fPI * (1.f/1024.f))

static int32_t loadtables(void)
{
    static char tablesloaded = 0;

    if (tablesloaded == 0)
    {
        int32_t i;

        initksqrt();

        for (i=0; i<2048; i++)
            reciptable[i] = divscale30(2048, i+2048);

        for (i=0; i<16384; i++)
            sloptable[i] = krecipasm(i-8192);

        for (i=0; i<=512; i++)
            sintable[i] = (int16_t)(16384.f * sinf((float)i * BANG2RAD));
        for (i=513; i<1024; i++)
            sintable[i] = sintable[1024-i];
        for (i=1024; i<2048; i++)
            sintable[i] = -sintable[i-1024];

        for (i=0; i<640; i++)
            radarang[i] = (int16_t)(atanf(((float)(640-i)-0.5f) * (1.f/160.f)) * (-64.f * (1.f/BANG2RAD)));
        for (i=0; i<640; i++)
            radarang[1279-i] = -radarang[i];

#ifdef B_LITTLE_ENDIAN
        i = 0;
        if (Bcrc32((uint8_t *)sintable, sizeof(sintable), 0) != 0xee1e7aba)
            i |= 1;
        if (Bcrc32((uint8_t *)radarang, 640*sizeof(radarang[0]), 0) != 0xee893d92)
            i |= 2;

        if (i != 0)
        {
            static const char *str[3] = { "sine table", "arctangent table",
                                          "sine and arctangent tables" };
            initprintf("WARNING: Calculated %s differ%s from original!\n",
                       str[i-1], i==3 ? "" : "s");
        }
#endif
        // TABLES.DAT format:
        //kread(fil,sintable,2048*2);
        //kread(fil,radarang,640*2);
        //kread(fil,textfont,1024);
        //kread(fil,smalltextfont,1024);
        //kread(fil,britable,1024);

        calcbritable();

        tablesloaded = 1;
    }

    return 0;
}


////////// SPRITE LIST MANIPULATION FUNCTIONS //////////

#ifdef NETCODE_DISABLE
# define LISTFN_STATIC static
#else
# define LISTFN_STATIC
#endif

///// sector lists of sprites /////

// insert sprite at the head of sector list, change .sectnum
LISTFN_STATIC void do_insertsprite_at_headofsect(int16_t spritenum, int16_t sectnum)
{
    int16_t const ohead = headspritesect[sectnum];

    prevspritesect[spritenum] = -1;
    nextspritesect[spritenum] = ohead;
    if (ohead >= 0)
        prevspritesect[ohead] = spritenum;
    headspritesect[sectnum] = spritenum;

    sprite[spritenum].sectnum = sectnum;
}

// remove sprite 'deleteme' from its sector list
LISTFN_STATIC void do_deletespritesect(int16_t deleteme)
{
    int32_t const sectnum = sprite[deleteme].sectnum;
    int32_t const prev = prevspritesect[deleteme];
    int32_t const next = nextspritesect[deleteme];

    if (headspritesect[sectnum] == deleteme)
        headspritesect[sectnum] = next;
    if (prev >= 0)
        nextspritesect[prev] = next;
    if (next >= 0)
        prevspritesect[next] = prev;
}

///// now, status lists /////

// insert sprite at head of status list, change .statnum
LISTFN_STATIC void do_insertsprite_at_headofstat(int16_t spritenum, int16_t statnum)
{
    int16_t const ohead = headspritestat[statnum];

    prevspritestat[spritenum] = -1;
    nextspritestat[spritenum] = ohead;
    if (ohead >= 0)
        prevspritestat[ohead] = spritenum;
    headspritestat[statnum] = spritenum;

    sprite[spritenum].statnum = statnum;
}

// insertspritestat (internal)
LISTFN_STATIC int32_t insertspritestat(int16_t statnum)
{
    if ((statnum >= MAXSTATUS) || (headspritestat[MAXSTATUS] == -1))
        return -1;  //list full

    // remove one sprite from the statnum-freelist
    int16_t const blanktouse = headspritestat[MAXSTATUS];
    headspritestat[MAXSTATUS] = nextspritestat[blanktouse];

    // make back-link of the new freelist head point to nil
    if (headspritestat[MAXSTATUS] >= 0)
        prevspritestat[headspritestat[MAXSTATUS]] = -1;
    else
        tailspritefree = -1;

    do_insertsprite_at_headofstat(blanktouse, statnum);

    return blanktouse;
}

// remove sprite 'deleteme' from its status list
static void do_deletespritestat(int16_t deleteme)
{
    int32_t const sectnum = sprite[deleteme].statnum;
    int32_t const prev = prevspritestat[deleteme];
    int32_t const next = nextspritestat[deleteme];

    if (headspritestat[sectnum] == deleteme)
        headspritestat[sectnum] = next;
    if (prev >= 0)
        nextspritestat[prev] = next;
    if (next >= 0)
        prevspritestat[next] = prev;
}


//
// insertsprite
//
int32_t insertsprite(int16_t sectnum, int16_t statnum)
{
    // TODO: guard against bad sectnum?
    int32_t const newspritenum = insertspritestat(statnum);

    if (newspritenum >= 0)
    {
        Bassert((unsigned)sectnum < MAXSECTORS);

        do_insertsprite_at_headofsect(newspritenum, sectnum);
        Numsprites++;
    }

    return newspritenum;

}

//
// deletesprite
//
int32_t deletesprite(int16_t spritenum)
{
    Bassert((sprite[spritenum].statnum == MAXSTATUS)
            == (sprite[spritenum].sectnum == MAXSECTORS));

    if (sprite[spritenum].statnum == MAXSTATUS)
        return -1;  // already not in the world

    do_deletespritestat(spritenum);
    do_deletespritesect(spritenum);

    // (dummy) insert at tail of sector freelist, compat
    // for code that checks .sectnum==MAXSECTOR
    sprite[spritenum].sectnum = MAXSECTORS;

    // insert at tail of status freelist
    prevspritestat[spritenum] = tailspritefree;
    nextspritestat[spritenum] = -1;
    if (tailspritefree >= 0)
        nextspritestat[tailspritefree] = spritenum;
    else
        headspritestat[MAXSTATUS] = spritenum;
    sprite[spritenum].statnum = MAXSTATUS;

    tailspritefree = spritenum;
    Numsprites--;

    return 0;
}

//
// changespritesect
//
int32_t changespritesect(int16_t spritenum, int16_t newsectnum)
{
    // XXX: NOTE: MAXSECTORS is allowed
    if ((newsectnum < 0 || newsectnum > MAXSECTORS) || (sprite[spritenum].sectnum == MAXSECTORS))
        return -1;

    if (sprite[spritenum].sectnum == newsectnum)
        return 0;

    do_deletespritesect(spritenum);
    do_insertsprite_at_headofsect(spritenum, newsectnum);

    return 0;
}

//
// changespritestat
//
int32_t changespritestat(int16_t spritenum, int16_t newstatnum)
{
    // XXX: NOTE: MAXSTATUS is allowed
    if ((newstatnum < 0 || newstatnum > MAXSTATUS) || (sprite[spritenum].statnum == MAXSTATUS))
        return -1;  // can't set the statnum of a sprite not in the world

    if (sprite[spritenum].statnum == newstatnum)
        return 0;  // sprite already has desired statnum

    do_deletespritestat(spritenum);
    do_insertsprite_at_headofstat(spritenum, newstatnum);

    return 0;
}

//
// lintersect (internal)
//
int32_t lintersect(int32_t x1, int32_t y1, int32_t z1,
                   int32_t x2, int32_t y2, int32_t z2,
                   int32_t x3, int32_t y3, int32_t x4, int32_t y4,
                   int32_t *intx, int32_t *inty, int32_t *intz)
{
    // p1 to p2 is a line segment
    int32_t const x21 = x2 - x1;
    int32_t const x34 = x3 - x4;
    int32_t const y21 = y2 - y1;
    int32_t const y34 = y3 - y4;
    int32_t const bot = x21 * y34 - y21 * x34;
    int32_t topt;

    if (bot == 0)
        return 0;
    else if (bot > 0)
    {
        int32_t x31 = x3 - x1;
        int32_t y31 = y3 - y1;

        topt = x31 * y34 - y31 * x34;

        if ((unsigned)topt >= (unsigned)bot)
            return 0;

        int32_t topu = x21 * y31 - y21 * x31;

        if ((unsigned)topu >= (unsigned)bot)
            return 0;
    }
    else
    {
        int32_t x31 = x3 - x1;
        int32_t y31 = y3 - y1;

        topt = x31 * y34 - y31 * x34;

        if ((unsigned)topt <= (unsigned)bot)
            return 0;

        int32_t topu = x21 * y31 - y21 * x31;

        if ((unsigned)topu <= (unsigned)bot)
            return 0;
    }

    int32_t t = divscale24(topt, bot);
    *intx = x1 + mulscale24(x21, t);
    *inty = y1 + mulscale24(y21, t);
    *intz = z1 + mulscale24(z2 - z1, t);

    return 1;
}

//
// rintersect (internal)
//
// returns: -1 if didn't intersect, coefficient (x3--x4 fraction)<<16 else
int32_t rintersect(int32_t x1, int32_t y1, int32_t z1,
                   int32_t vx_, int32_t vy_, int32_t vz,
                   int32_t x3, int32_t y3, int32_t x4, int32_t y4,
                   int32_t *intx, int32_t *inty, int32_t *intz)
{
    //p1 towards p2 is a ray
    int64_t topt, topu, t;

    const int64_t vx=vx_, vy=vy_;
    const int64_t x34=x3-x4, y34=y3-y4;
    const int64_t bot = vx*y34 - vy*x34;

    if (bot == 0)
        return -1;

    if (bot >= 0)
    {
        int64_t x31=x3-x1, y31 = y3-y1;
        topt = x31*y34 - y31*x34; if (topt < 0) return -1;
        topu = vx*y31 - vy*x31; if (topu < 0 || topu >= bot) return -1;
    }
    else
    {
        int32_t x31=x3-x1, y31=y3-y1;
        topt = x31*y34 - y31*x34; if (topt > 0) return -1;
        topu = vx*y31 - vy*x31; if (topu > 0 || topu <= bot) return -1;
    }

    t = (topt<<16)/bot;
    *intx = x1 + ((vx*t)>>16);
    *inty = y1 + ((vy*t)>>16);
    *intz = z1 + ((vz*t)>>16);

    t = (topu<<16)/bot;
    Bassert((unsigned)t < 65536);

    return t;
}

int32_t rayintersect(int32_t x1, int32_t y1, int32_t z1, int32_t vx, int32_t vy, int32_t vz, int32_t x3,
                     int32_t y3, int32_t x4, int32_t y4, int32_t *intx, int32_t *inty, int32_t *intz)
{
    return (rintersect(x1, y1, z1, vx, vy, vz, x3, y3, x4, y4, intx, inty, intz) != -1);
}

//
// multi-pskies
//

psky_t * E_DefinePsky(int32_t const tilenum)
{
    for (bssize_t i = 0; i < pskynummultis; i++)
        if (multipskytile[i] == tilenum)
            return &multipsky[i];

    int32_t const newPskyID = pskynummultis++;
    multipsky = (psky_t *)Xrealloc(multipsky, pskynummultis * sizeof(psky_t));
    multipskytile = (int32_t *)Xrealloc(multipskytile, pskynummultis * sizeof(int32_t));

    psky_t * const newPsky = &multipsky[newPskyID];
    Bmemset(newPsky, 0, sizeof(psky_t));
    multipskytile[newPskyID] = tilenum;
    newPsky->yscale = 65536;

    return newPsky;
}

//
// Exported Engine Functions
//

#if !defined _WIN32 && defined DEBUGGINGAIDS && !defined GEKKO
#ifdef GEKKO
#define __rtems__
#define _POSIX_REALTIME_SIGNALS
#endif
#include <signal.h>
static void sighandler(int sig, siginfo_t *info, void *ctx)
{
    const char *s;
    UNREFERENCED_PARAMETER(ctx);
    switch (sig)
    {
    case SIGFPE:
        switch (info->si_code)
        {
        case FPE_INTDIV:
            s = "FPE_INTDIV (integer divide by zero)"; break;
        case FPE_INTOVF:
            s = "FPE_INTOVF (integer overflow)"; break;
        case FPE_FLTDIV:
            s = "FPE_FLTDIV (floating-point divide by zero)"; break;
        case FPE_FLTOVF:
            s = "FPE_FLTOVF (floating-point overflow)"; break;
        case FPE_FLTUND:
            s = "FPE_FLTUND (floating-point underflow)"; break;
        case FPE_FLTRES:
            s = "FPE_FLTRES (floating-point inexact result)"; break;
        case FPE_FLTINV:
            s = "FPE_FLTINV (floating-point invalid operation)"; break;
        case FPE_FLTSUB:
            s = "FPE_FLTSUB (floating-point subscript out of range)"; break;
        default:
            s = "?! (unknown)"; break;
        }
        ERRprintf("Caught SIGFPE at address %p, code %s. Aborting.\n", info->si_addr, s);
        break;
    default:
        break;
    }
    abort();
}
#endif

//
// E_FatalError
//
int32_t E_FatalError(char const * const msg)
{
    engineerrstr = msg;
    initprintf("ERROR: %s\n", engineerrstr);
    return -1;
}

//
// preinitengine
//
static int32_t preinitcalled = 0;

// #define DYNALLOC_ARRAYS

#ifdef DYNALLOC_ARRAYS
void *blockptr = NULL;
#elif !defined DEBUG_MAIN_ARRAYS
static spriteext_t spriteext_s[MAXSPRITES+MAXUNIQHUDID];
static spritesmooth_t spritesmooth_s[MAXSPRITES+MAXUNIQHUDID];
static sectortype sector_s[MAXSECTORS + M32_FIXME_SECTORS];
static walltype wall_s[MAXWALLS + M32_FIXME_WALLS];
#ifndef NEW_MAP_FORMAT
static wallext_t wallext_s[MAXWALLS];
#endif
static spritetype sprite_s[MAXSPRITES];
static uspritetype tsprite_s[MAXSPRITESONSCREEN];
#endif

int32_t preinitengine(void)
{
    initdivtables();
    if (initsystem()) Bexit(9);
    makeasmwriteable();

#ifdef DYNALLOC_ARRAYS
    {
        size_t i, size = 0;

        // allocate everything at once...  why not?  entries can just be added to this table
        // to allocate future arrays without further intervention
        struct
        {
            void **ptr;
            size_t size;
        }
        dynarray[] =
        {
            { (void **) &sector,           sizeof(sectortype)      *MAXSECTORS                },
            { (void **) &wall,             sizeof(walltype)        *MAXWALLS }, // +512: editor quirks. FIXME!
# ifndef NEW_MAP_FORMAT
            { (void **) &wallext,          sizeof(wallext_t)       *MAXWALLS                  },
# endif
            { (void **) &sprite,           sizeof(spritetype)      *MAXSPRITES                },
            { (void **) &tsprite,          sizeof(spritetype)      *MAXSPRITESONSCREEN        },
            { (void **) &spriteext,        sizeof(spriteext_t)     *(MAXSPRITES+MAXUNIQHUDID) },
            { (void **) &spritesmooth,     sizeof(spritesmooth_t) *(MAXSPRITES+MAXUNIQHUDID) },
        };

        if (editstatus)
        {
            dynarray[0].size += M32_FIXME_SECTORS*sizeof(sectortype);  // join sectors needs a temp. sector
            dynarray[1].size += M32_FIXME_WALLS*sizeof(walltype);
//            Bprintf("FIXME: Allocating additional space beyond wall[] for editor bugs.\n");
        }

#if 0
        for (i=0; i<(signed)ARRAY_SIZE(dynarray); i++)
            size += dynarray[i].size;

        if ((blockptr = Bcalloc(1, size)) == NULL)
            return 1;

        size = 0;

        for (i=0; i<(signed)ARRAY_SIZE(dynarray); i++)
        {
            *dynarray[i].ptr = (int8_t *)blockptr + size;
            size += dynarray[i].size;
        }
#else
        for (i = 0; i < (signed) ARRAY_SIZE(dynarray); i++)
        {
            Baligned_free(*dynarray[i].ptr);
            *dynarray[i].ptr = Xaligned_alloc(16, dynarray[i].size);
        }
#endif
    }

#elif !defined DEBUG_MAIN_ARRAYS
    sector = sector_s;
    wall = wall_s;
# ifndef NEW_MAP_FORMAT
    wallext = wallext_s;
# endif
    sprite = sprite_s;
    tsprite = tsprite_s;
    spriteext = spriteext_s;
    spritesmooth = spritesmooth_s;
#endif

#if !defined ENGINE_USING_A_C
    mmxoverlay();
#endif

    validmodecnt = 0;
    getvalidmodes();

    initcrc32table();

#ifdef HAVE_CLIPSHAPE_FEATURE
    clipmapinfo_init();
#endif
    preinitcalled = 1;
    return 0;
}


//
// initengine
//
int32_t initengine(void)
{
    int32_t i, j;

#if !defined _WIN32 && defined DEBUGGINGAIDS && !defined GEKKO
    struct sigaction sigact, oldact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_sigaction = &sighandler;
    sigact.sa_flags = SA_SIGINFO;
    sigaction(SIGFPE, &sigact, &oldact);
#endif

    if (!preinitcalled)
    {
        i = preinitengine();
        if (i) return i;
    }

#ifdef YAX_DEBUG
    u64tickspersec = (double)getu64tickspersec();
    if (u64tickspersec==0.0)
        u64tickspersec = 1.0;
#endif

    if (loadtables())
        return 1;

    xyaspect = -1;

    showinvisibility = 0;

    for (i=1; i<1024; i++)
        lowrecip[i] = ((1<<24)-1)/i;

    for (i=0; i<MAXVOXELS; i++)
        for (j=0; j<MAXVOXMIPS; j++)
        {
            voxoff[i][j] = 0L;
            voxlock[i][j] = 200;
        }
    for (i=0; i<MAXTILES; i++)
        tiletovox[i] = -1;
    clearbuf(voxscale, sizeof(voxscale)>>2, 65536);

    paletteloaded = 0;

    searchit = 0; searchstat = -1;

    totalclock = 0;
    g_visibility = 512;
    parallaxvisibility = 512;

    maxspritesonscreen = MAXSPRITESONSCREEN;

    loadpalette();

#ifdef USE_OPENGL
    if (!hicinitcounter) hicinit();
    if (!mdinited) mdinit();
#endif

#ifdef LUNATIC
    if (L_CreateState(&g_engState, "eng", NULL))
        return E_FatalError("Failed creating engine Lua state!");

    {
        static char const * const luastr = "_LUNATIC_AUX=true; decl=require('ffi').cdef; require'defs_common'";

        if (L_RunString(&g_engState, luastr, -1, "eng"))
            return E_FatalError("Failed setting up engine Lua state");
    }
#endif

    return 0;
}

//
// E_PostInit
//
int32_t E_PostInit(void)
{
    if (!(paletteloaded & PALETTE_MAIN))
        return E_FatalError("No palette found.");
    if (!(paletteloaded & PALETTE_SHADE))
        return E_FatalError("No shade table found.");
    if (!(paletteloaded & PALETTE_TRANSLUC))
        return E_FatalError("No translucency table found.");

    E_PostLoadPalette();

    return 0;
}

//
// uninitengine
//
void uninitengine(void)
{
#ifdef USE_OPENGL
    polymost_glreset();
    hicinit();
    freeallmodels();
# ifdef POLYMER
    polymer_uninit();
# endif
#endif

    Buninitart();

    DO_FREE_AND_NULL(lookups);
    ALIGNED_FREE_AND_NULL(distrecip);

    paletteloaded = 0;

    for (bssize_t i=0; i<MAXPALOOKUPS; i++)
        if (i==0 || palookup[i] != palookup[0])
        {
            // Take care of handling aliased ^^^ cases!
            Baligned_free(palookup[i]);
        }
    Bmemset(palookup, 0, sizeof(palookup));

    for (bssize_t i=0; i<MAXBLENDTABS; i++)
        Bfree(blendtable[i]);
    Bmemset(blendtable, 0, sizeof(blendtable));

    for (bssize_t i=1; i<MAXBASEPALS; i++)
        Bfree(basepaltable[i]);
    Bmemset(basepaltable, 0, sizeof(basepaltable));
    basepaltable[0] = palette;

#ifdef DYNALLOC_ARRAYS
    DO_FREE_AND_NULL(blockptr);
#endif
    DO_FREE_AND_NULL(kpzbuf);
    kpzbufsiz = 0;

    uninitsystem();

    for (bssize_t i = 0; i < num_usermaphacks; i++)
    {
        Bfree(usermaphacks[i].mhkfile);
        Bfree(usermaphacks[i].title);
    }
    DO_FREE_AND_NULL(usermaphacks);
    num_usermaphacks = 0;

    DO_FREE_AND_NULL(multipsky);
    DO_FREE_AND_NULL(multipskytile);
    pskynummultis = 0;
}


//
// initspritelists
//
void initspritelists(void)
{
    int32_t i;

    // initial list state for statnum lists:
    //
    //  statnum 0: nil
    //  statnum 1: nil
    //     . . .
    //  statnum MAXSTATUS-1: nil
    //  "statnum MAXSTATUS": nil <- 0 <-> 1 <-> 2 <-> ... <-> MAXSPRITES-1 -> nil
    //
    // That is, the dummy MAXSTATUS statnum has all sprites.

    for (i=0; i<MAXSECTORS; i++)   //Init doubly-linked sprite sector lists
        headspritesect[i] = -1;
    headspritesect[MAXSECTORS] = 0;

    for (i=0; i<MAXSPRITES; i++)
    {
        prevspritesect[i] = i-1;
        nextspritesect[i] = i+1;
        sprite[i].sectnum = MAXSECTORS;
    }
    prevspritesect[0] = -1;
    nextspritesect[MAXSPRITES-1] = -1;


    for (i=0; i<MAXSTATUS; i++)   //Init doubly-linked sprite status lists
        headspritestat[i] = -1;
    headspritestat[MAXSTATUS] = 0;

    for (i=0; i<MAXSPRITES; i++)
    {
        prevspritestat[i] = i-1;
        nextspritestat[i] = i+1;
        sprite[i].statnum = MAXSTATUS;
    }
    prevspritestat[0] = -1;
    nextspritestat[MAXSPRITES-1] = -1;

    tailspritefree = MAXSPRITES-1;
    Numsprites = 0;
}


void set_globalang(int16_t ang)
{
    globalang = ang&2047;
    cosglobalang = sintable[(globalang+512)&2047];
    singlobalang = sintable[globalang&2047];
#ifdef USE_OPENGL
    fcosglobalang = (float) cosglobalang;
    fsinglobalang = (float) singlobalang;
#endif
    cosviewingrangeglobalang = mulscale16(cosglobalang,viewingrange);
    sinviewingrangeglobalang = mulscale16(singlobalang,viewingrange);
}

//
// drawrooms
//
int32_t drawrooms(int32_t daposx, int32_t daposy, int32_t daposz,
               int16_t daang, int32_t dahoriz, int16_t dacursectnum)
{
    int32_t i, j, /*cz, fz,*/ closest;
    int16_t *shortptr1, *shortptr2;

    int32_t didmirror = 0;

    beforedrawrooms = 0;

    set_globalpos(daposx, daposy, daposz);
    set_globalang(daang);

    global100horiz = dahoriz;

    // xdimenscale is scale(xdimen,yxaspect,320);
    // normalization by viewingrange so that center-of-aim doesn't depend on it
    globalhoriz = mulscale16(dahoriz-100,divscale16(xdimenscale,viewingrange))+(ydimen>>1);

    globaluclip = (0-globalhoriz)*xdimscale;
    globaldclip = (ydimen-globalhoriz)*xdimscale;

    i = mulscale16(xdimenscale,viewingrangerecip);
    globalpisibility = mulscale16(parallaxvisibility,i);
    switch (getrendermode())
    {
        // switch on renderers to make fog look almost the same everywhere

    case REND_CLASSIC:
        globalvisibility = mulscale16(g_visibility,i);
        break;
#ifdef USE_OPENGL
    case REND_POLYMOST:
        // NOTE: In Polymost, the fragment depth depends on the x screen size!
        if (r_usenewshading == 4)
            globalvisibility = g_visibility * xdimen;
        else if (r_usenewshading >= 2)
            globalvisibility = scale(g_visibility<<2, xdimen, 1680);
        else
            globalvisibility = scale(g_visibility<<2, xdimen, 1100);
        break;
# ifdef POLYMER
    case REND_POLYMER:
        if (r_usenewshading == 4)
            globalvisibility = g_visibility;
        else
            globalvisibility = g_visibility<<2;
        break;
# endif
#endif
    }

    globalhisibility = mulscale16(globalvisibility,xyaspect);
    globalcisibility = mulscale8(globalhisibility,320);

    globalcursectnum = dacursectnum;
    totalclocklock = totalclock;

    if ((xyaspect != oxyaspect) || (xdimen != oxdimen) || (viewingrange != oviewingrange))
        dosetaspect();

    Bmemset(gotsector, 0, ((numsectors+7)>>3));

    if (getrendermode() != REND_CLASSIC
#ifdef YAX_ENABLE
        || yax_globallev==YAX_MAXDRAWS
#endif
        )
    {
        shortptr1 = (int16_t *)&startumost[windowxy1.x];
        shortptr2 = (int16_t *)&startdmost[windowxy1.x];
        i = xdimen-1;
        do
        {
            umost[i] = shortptr1[i]-windowxy1.y;
            dmost[i] = shortptr2[i]-windowxy1.y;
        }
        while (i--);  // xdimen == 1 is OK!
        umost[0] = shortptr1[0]-windowxy1.y;
        dmost[0] = shortptr2[0]-windowxy1.y;
    }

#ifdef USE_OPENGL
# ifdef POLYMER
    if (getrendermode() == REND_POLYMER)
    {
#  ifdef YAX_ENABLE
        // BEGIN_TWEAK ceiling/floor fake 'TROR' pics, see END_TWEAK in build.c
        if (editstatus && showinvisibility)
        {
            for (i=0; i<numyaxbunches; i++)
            {
                yax_tweakpicnums(i, YAX_CEILING, 0);
                yax_tweakpicnums(i, YAX_FLOOR, 0);
            }
        }
#  endif
        polymer_glinit();
        polymer_drawrooms(daposx, daposy, daposz, daang, dahoriz, dacursectnum);
        bglDisable(GL_CULL_FACE);
        gloy1 = 0;
        return 0;
    }
# endif
#endif

    // Update starting sector number (common to classic and Polymost).
    // ADJUST_GLOBALCURSECTNUM.
    if (globalcursectnum >= MAXSECTORS)
        globalcursectnum -= MAXSECTORS;
    else
    {
        i = globalcursectnum;
        updatesectorbreadth(globalposx,globalposy,&globalcursectnum);
        if (globalcursectnum < 0) globalcursectnum = i;

        // PK 20110123: I'm not sure what the line above is supposed to do, but 'i'
        //              *can* be negative, so let's just quit here in that case...
        if (globalcursectnum<0)
            return 0;
    }

#ifdef USE_OPENGL
    //============================================================================= //POLYMOST BEGINS
    polymost_drawrooms();

    if (getrendermode() != REND_CLASSIC)
        return 0;
    //============================================================================= //POLYMOST ENDS
#endif

    begindrawing(); //{{{

#ifdef ENGINE_CLEAR_SCREEN
#ifdef YAX_ENABLE
    if (!g_nodraw)
#endif
    if (numyaxbunches==0)
        draw_rainbow_background();
#endif

    frameoffset = frameplace + windowxy1.y*bytesperline + windowxy1.x;

    //if (smostwallcnt < 0)
    //  if (getkensmessagecrc(FP_OFF(kensmessage)) != 0x56c764d4)
    //      { /* setvmode(0x3);*/ OSD_Printf("Nice try.\n"); Bexit(0); }

    numhits = xdimen; numscans = 0; numbunches = 0;
    maskwallcnt = 0; smostwallcnt = 0; smostcnt = 0; spritesortcnt = 0;

#ifdef YAX_ENABLE
    if (yax_globallev != YAX_MAXDRAWS)
    {
        j = yax_globalbunch*xdimen;

        Bmemcpy(umost, yumost+j, xdimen*sizeof(int16_t));
        Bmemcpy(dmost, ydmost+j, xdimen*sizeof(int16_t));

        for (i=0; i<xdimen; i++)
            if (umost[i] > dmost[i])
                numhits--;

//        yaxdebug("cf %d, tlev %d, bunch %d: numhits=%d", yax_globalcf, yax_globallev, yax_globalbunch, numhits);
    }
#endif

/*
    globparaceilclip = 1;
    globparaflorclip = 1;
    getzsofslope(globalcursectnum,globalposx,globalposy,&cz,&fz);
    if (globalposz < cz) globparaceilclip = 0;
    if (globalposz > fz) globparaflorclip = 0;
*/
    scansector(globalcursectnum);

    if (inpreparemirror)
    {
        // INPREPAREMIRROR_NO_BUNCHES
        // numbunches==0 can happen if the mirror is far away... the game code decides
        // to draw it, but scansector gets zero bunches.  Result: big screwup!
        // Leave inpreparemirror as is, it's restored by completemirror.
        if (numbunches==0)
        {
            enddrawing();  //!!!
            return 0;
        }

        inpreparemirror = 0;
        didmirror = 1;

        mirrorsx1 = xdimen-1; mirrorsx2 = 0;
        for (i=numscans-1; i>=0; i--)
        {
            if (wall[thewall[i]].nextsector >= 0)
            {
                if (xb1[i] < mirrorsx1) mirrorsx1 = xb1[i];
                if (xb2[i] > mirrorsx2) mirrorsx2 = xb2[i];
            }
        }

        for (i=0; i<mirrorsx1; i++)
            if (umost[i] <= dmost[i])
                { umost[i] = 1; dmost[i] = 0; numhits--; }
        for (i=mirrorsx2+1; i<xdimen; i++)
            if (umost[i] <= dmost[i])
                { umost[i] = 1; dmost[i] = 0; numhits--; }

        drawalls(0L);
        numbunches--;
        bunchfirst[0] = bunchfirst[numbunches];
        bunchlast[0] = bunchlast[numbunches];

        mirrorsy1 = min(umost[mirrorsx1],umost[mirrorsx2]);
        mirrorsy2 = max(dmost[mirrorsx1],dmost[mirrorsx2]);
    }

    while ((numbunches > 0) && (numhits > 0))
    {
        Bmemset(tempbuf, 0, numbunches);
        tempbuf[0] = 1;

        closest = 0;              //Almost works, but not quite :(
        for (i=1; i<numbunches; i++)
        {
            if ((j = bunchfront(i,closest)) < 0) continue;
            tempbuf[i] = 1;
            if (j == 0) tempbuf[closest] = 1, closest = i;
        }
        for (i=0; i<numbunches; i++) //Double-check
        {
            if (tempbuf[i]) continue;
            if ((j = bunchfront(i,closest)) < 0) continue;
            tempbuf[i] = 1;
            if (j == 0) tempbuf[closest] = 1, closest = i, i = 0;
        }

        drawalls(closest);

        numbunches--;
        bunchfirst[closest] = bunchfirst[numbunches];
        bunchlast[closest] = bunchlast[numbunches];
    }

    enddrawing();   //}}}

    return didmirror;
}

// UTILITY TYPES AND FUNCTIONS FOR DRAWMASKS OCCLUSION TREE
// typedef struct          s_maskleaf
// {
//     int32_t                index;
//     _point2d            p1, p2;
//     _equation           maskeq, p1eq, p2eq;
//     struct s_maskleaf*  branch[MAXWALLSB];
//     int32_t                 drawing;
// }                       _maskleaf;
//
// _maskleaf               maskleaves[MAXWALLSB];

// returns equation of a line given two points
static inline _equation       equation(float x1, float y1, float x2, float y2)
{
    _equation   ret;
    const float f = x2-x1;

    // vertical
    if (f == 0.f)
    {
        ret.a = 1;
        ret.b = 0;
        ret.c = -x1;

        return ret;
    }

    ret.a = (float) (y2 - y1)/f;
    ret.b = -1;
    ret.c = (y1 - (ret.a * x1));

    return ret;
}

int32_t                 wallvisible(int32_t x, int32_t y, int16_t wallnum)
{
    // 1 if wall is in front of player 0 otherwise
    uwalltype *w1 = (uwalltype *)&wall[wallnum];
    uwalltype *w2 = (uwalltype *)&wall[w1->point2];

    int32_t a1 = getangle(w1->x - x, w1->y - y);
    int32_t a2 = getangle(w2->x - x, w2->y - y);

    //if ((wallnum == 23) || (wallnum == 9))
    //    OSD_Printf("Wall %d : %d - sector %d - x %d - y %d.\n", wallnum, (a2 + (2048 - a1)) & 2047, globalcursectnum, globalposx, globalposy);

    return (((a2 + (2048 - a1)) & 2047) <= 1024);
}

#if 0
// returns the intersection point between two lines
_point2d        intersection(_equation eq1, _equation eq2)
{
    _point2d    ret;
    float       det;

    det = (float)(1) / (eq1.a*eq2.b - eq2.a*eq1.b);
    ret.x = ((eq1.b*eq2.c - eq2.b*eq1.c) * det);
    ret.y = ((eq2.a*eq1.c - eq1.a*eq2.c) * det);

    return ret;
}

// check if a point that's on the line is within the segment boundaries
int32_t             pointonmask(_point2d point, _maskleaf* wall)
{
    if ((min(wall->p1.x, wall->p2.x) <= point.x) && (point.x <= max(wall->p1.x, wall->p2.x)) && (min(wall->p1.y, wall->p2.y) <= point.y) && (point.y <= max(wall->p1.y, wall->p2.y)))
        return 1;
    return 0;
}

// returns 1 if wall2 is hidden by wall1
int32_t             wallobstructswall(_maskleaf* wall1, _maskleaf* wall2)
{
    _point2d    cross;

    cross = intersection(wall2->p1eq, wall1->maskeq);
    if (pointonmask(cross, wall1))
        return 1;

    cross = intersection(wall2->p2eq, wall1->maskeq);
    if (pointonmask(cross, wall1))
        return 1;

    cross = intersection(wall1->p1eq, wall2->maskeq);
    if (pointonmask(cross, wall2))
        return 1;

    cross = intersection(wall1->p2eq, wall2->maskeq);
    if (pointonmask(cross, wall2))
        return 1;

    return 0;
}

// recursive mask drawing function
static inline void    drawmaskleaf(_maskleaf* wall)
{
    int32_t i;

    wall->drawing = 1;
    i = 0;
    while (wall->branch[i] != NULL)
    {
        if (wall->branch[i]->drawing == 0)
        {
            //OSD_Printf("Drawing parent of %i : mask %i\n", wall->index, wall->branch[i]->index);
            drawmaskleaf(wall->branch[i]);
        }
        i++;
    }

    //OSD_Printf("Drawing mask %i\n", wall->index);
    drawmaskwall(wall->index);
}
#endif

static inline int32_t         sameside(const _equation *eq, const vec2f_t *p1, const vec2f_t *p2)
{
    const float sign1 = (eq->a * p1->x) + (eq->b * p1->y) + eq->c;
    const float sign2 = (eq->a * p2->x) + (eq->b * p2->y) + eq->c;
    return (sign1 * sign2) > 0.f;
}

// x1, y1: in/out
// rest x/y: out
void get_wallspr_points(const uspritetype *spr, int32_t *x1, int32_t *x2,
                               int32_t *y1, int32_t *y2);
void get_floorspr_points(const uspritetype *spr, int32_t px, int32_t py,
                                int32_t *x1, int32_t *x2, int32_t *x3, int32_t *x4,
                                int32_t *y1, int32_t *y2, int32_t *y3, int32_t *y4);

#ifdef DEBUG_MASK_DRAWING
int32_t g_maskDrawMode = 0;
#endif

//
// drawmasks
//
void drawmasks(void)
{
#ifdef DEBUG_MASK_DRAWING
        static struct {
            int16_t di;  // &32768: &32767 is tspriteptr[], else thewall[] index
            int16_t i;   // sprite[] or wall[] index
        } debugmask[MAXWALLSB + MAXSPRITESONSCREEN + 1];

        int32_t dmasknum = 0;

# define debugmask_add(dispidx, idx) do { \
        if (g_maskDrawMode && getrendermode()==REND_CLASSIC) { \
            debugmask[dmasknum].di = dispidx; \
            debugmask[dmasknum++].i = idx; \
        } \
    } while (0)
#else
# define debugmask_add(dispidx, idx) do {} while (0)
#endif
    int32_t i;

    for (i=spritesortcnt-1; i>=0; i--)
        tspriteptr[i] = &tsprite[i];

    for (i=spritesortcnt-1; i>=0; i--)
    {
        const int32_t xs = tspriteptr[i]->x-globalposx, ys = tspriteptr[i]->y-globalposy;
        const int32_t yp = dmulscale6(xs,cosviewingrangeglobalang,ys,sinviewingrangeglobalang);
#ifdef USE_OPENGL
        const int32_t modelp = (usemodels && tile2model[tspriteptr[i]->picnum].modelid >= 0);
#endif

        if (yp > (4<<8))
        {
            const int32_t xp = dmulscale6(ys,cosglobalang,-xs,singlobalang);

            if (mulscale24(labs(xp+yp),xdimen) >= yp)
                goto killsprite;

            spritesxyz[i].x = scale(xp+yp,xdimen<<7,yp);
        }
        else if ((tspriteptr[i]->cstat&48) == 0)
        {
killsprite:
#ifdef USE_OPENGL
            if (!modelp)
#endif
            {
                spritesortcnt--;  //Delete face sprite if on wrong side!
                if (i != spritesortcnt)
                {
                    tspriteptr[i] = tspriteptr[spritesortcnt];
                    spritesxyz[i].x = spritesxyz[spritesortcnt].x;
                    spritesxyz[i].y = spritesxyz[spritesortcnt].y;
                }
                continue;
            }
        }
        spritesxyz[i].y = yp;
    }

    int32_t gap, ys;

    gap = 1; while (gap < spritesortcnt) gap = (gap<<1)+1;
    for (gap>>=1; gap>0; gap>>=1)   //Sort sprite list
        for (i=0; i<spritesortcnt-gap; i++)
            for (bssize_t l=i; l>=0; l-=gap)
            {
                if (spritesxyz[l].y <= spritesxyz[l+gap].y) break;
                swapptr(&tspriteptr[l],&tspriteptr[l+gap]);
                swaplong(&spritesxyz[l].x,&spritesxyz[l+gap].x);
                swaplong(&spritesxyz[l].y,&spritesxyz[l+gap].y);
            }

    if (spritesortcnt > 0)
        spritesxyz[spritesortcnt].y = (spritesxyz[spritesortcnt-1].y^1);

    ys = spritesxyz[0].y; i = 0;
    for (bssize_t j=1; j<=spritesortcnt; j++)
    {
        if (spritesxyz[j].y == ys)
            continue;

        ys = spritesxyz[j].y;

        if (j > i+1)
        {
            for (bssize_t k=i; k<j; k++)
            {
                const uspritetype *const s = tspriteptr[k];

                spritesxyz[k].z = s->z;
                if ((s->cstat&48) != 32)
                {
                    int32_t yoff = picanm[s->picnum].yofs + s->yoffset;
                    int32_t yspan = (tilesiz[s->picnum].y*s->yrepeat<<2);

                    spritesxyz[k].z -= (yoff*s->yrepeat)<<2;

                    if (!(s->cstat&128))
                        spritesxyz[k].z -= (yspan>>1);
                    if (klabs(spritesxyz[k].z-globalposz) < (yspan>>1))
                        spritesxyz[k].z = globalposz;
                }
            }

            for (bssize_t k=i+1; k<j; k++)
                for (bssize_t l=i; l<k; l++)
                    if (klabs(spritesxyz[k].z-globalposz) < klabs(spritesxyz[l].z-globalposz))
                    {
                        swapptr(&tspriteptr[k],&tspriteptr[l]);
                        vec3_t tv3 = spritesxyz[k];
                        spritesxyz[k] = spritesxyz[l];
                        spritesxyz[l] = tv3;
                    }

            for (bssize_t k=i+1; k<j; k++)
                for (bssize_t l=i; l<k; l++)
                    if (tspriteptr[k]->x == tspriteptr[l]->x &&
                        tspriteptr[k]->y == tspriteptr[l]->y &&
                        (tspriteptr[k]->cstat & 48) == (tspriteptr[l]->cstat & 48) &&
                        tspriteptr[k]->owner < tspriteptr[l]->owner)
                    {
                        swapptr(&tspriteptr[k], &tspriteptr[l]);
                        vec3_t tv3 = spritesxyz[k];
                        spritesxyz[k] = spritesxyz[l];
                        spritesxyz[l] = tv3;
                    }
        }
        i = j;
    }

    begindrawing(); //{{{
#if 0
    for (i=spritesortcnt-1; i>=0; i--)
    {
        double xs = tspriteptr[i]->x-globalposx;
        double ys = tspriteptr[i]->y-globalposy;
        int32_t zs = tspriteptr[i]->z-globalposz;

        int32_t xp = ys*cosglobalang-xs*singlobalang;
        int32_t yp = (zs<<1);
        int32_t zp = xs*cosglobalang+ys*singlobalang;

        xs = ((double)xp*(halfxdimen<<12)/zp)+((halfxdimen+windowxy1.x)<<12);
        ys = ((double)yp*(xdimenscale<<12)/zp)+((globalhoriz+windowxy1.y)<<12);

        if (xs >= INT32_MIN && xs <= INT32_MAX && ys >= INT32_MIN && ys <= INT32_MAX)
        {
            drawline256(xs-65536,ys-65536,xs+65536,ys+65536,31);
            drawline256(xs+65536,ys-65536,xs-65536,ys+65536,31);
        }
    }
#endif

    vec2f_t pos;

    pos.x = fglobalposx;
    pos.y = fglobalposy;

    // CAUTION: maskwallcnt and spritesortcnt may be zero!
    // Writing e.g. "while (maskwallcnt--)" is wrong!
    while (maskwallcnt)
    {
        // PLAG: sorting stuff
        const int32_t w = (getrendermode()==REND_POLYMER) ?
            maskwall[maskwallcnt-1] : thewall[maskwall[maskwallcnt-1]];

        maskwallcnt--;

        vec2f_t dot    = { (float)wall[w].x, (float)wall[w].y };
        vec2f_t dot2   = { (float)wall[wall[w].point2].x, (float)wall[wall[w].point2].y };
        vec2f_t middle = { (dot.x + dot2.x) * .5f, (dot.y + dot2.y) * .5f };

        _equation maskeq = equation(dot.x, dot.y, dot2.x, dot2.y);
        _equation p1eq   = equation(pos.x, pos.y, dot.x, dot.y);
        _equation p2eq   = equation(pos.x, pos.y, dot2.x, dot2.y);

        i = spritesortcnt;
        while (i)
        {
            i--;
            if (tspriteptr[i] != NULL
#ifdef USE_OPENGL
                && (!(tspriteptr[i]->cstat & 1024) || getrendermode() != REND_POLYMOST)
#endif
               )
            {
                vec2f_t spr;
                const uspritetype *tspr = tspriteptr[i];

                spr.x = (float)tspr->x;
                spr.y = (float)tspr->y;

                if (!sameside(&maskeq, &spr, &pos))
                {
                    // Sprite and camera are on different sides of the
                    // masked wall.

                    // Check if the sprite is inside the 'cone' given by
                    // the rays from the camera to the two wall-points.
                    const int32_t inleft = sameside(&p1eq, &middle, &spr);
                    const int32_t inright = sameside(&p2eq, &middle, &spr);

                    int32_t ok = (inleft && inright);

                    if (!ok)
                    {
                        // If not, check if any of the border points are...
                        int32_t xx[4] = { tspr->x };
                        int32_t yy[4] = { tspr->y };
                        int32_t numpts, jj;

                        const _equation pineq = inleft ? p1eq : p2eq;

                        if ((tspr->cstat & 48) == 32)
                        {
                            numpts = 4;
                            get_floorspr_points(tspr, 0, 0,
                                                &xx[0], &xx[1], &xx[2], &xx[3],
                                                &yy[0], &yy[1], &yy[2], &yy[3]);
                        }
                        else
                        {
                            const int32_t oang = tspr->ang;
                            numpts = 2;

                            // Consider face sprites as wall sprites with camera ang.
                            // XXX: factor 4/5 needed?
                            if ((tspr->cstat & 48) != 16)
                                tspriteptr[i]->ang = globalang;

                            get_wallspr_points((const uspritetype *)tspr, &xx[0], &xx[1], &yy[0], &yy[1]);

                            if ((tspr->cstat & 48) == 0)
                                tspriteptr[i]->ang = oang;
                        }

                        for (jj=0; jj<numpts; jj++)
                        {
                            spr.x = (float)xx[jj];
                            spr.y = (float)yy[jj];

                            if (!sameside(&maskeq, &spr, &pos))  // behind the maskwall,
                                if ((sameside(&p1eq, &middle, &spr) &&  // inside the 'cone',
                                        sameside(&p2eq, &middle, &spr))
                                        || !sameside(&pineq, &middle, &spr))  // or on the other outside.
                                {
                                    ok = 1;
                                    break;
                                }
                        }
                    }

                    if (ok)
                    {
                        debugmask_add(i | 32768, tspr->owner);
                        drawsprite(i);

                        tspriteptr[i] = NULL;
                    }
                }
            }
        }

        debugmask_add(maskwall[maskwallcnt], thewall[maskwall[maskwallcnt]]);
        drawmaskwall(maskwallcnt);
    }

    i = spritesortcnt;

    while (i)
    {
        i--;
        if (tspriteptr[i] != NULL
#ifdef USE_OPENGL
            && (!(tspriteptr[i]->cstat & 1024) || getrendermode() != REND_POLYMOST)
#endif
           )
        {
            debugmask_add(i | 32768, tspriteptr[i]->owner);
            drawsprite(i);

            tspriteptr[i] = NULL;
        }
    }

#ifdef USE_OPENGL
    if (getrendermode() == REND_POLYMOST)
    {
        bglDepthMask(GL_FALSE);

        while (spritesortcnt)
        {
            spritesortcnt--;
            if (tspriteptr[spritesortcnt] != NULL)
            {
                Bassert(tspriteptr[spritesortcnt]->cstat & 1024);
                drawsprite(spritesortcnt);
                tspriteptr[spritesortcnt] = NULL;
            }
        }

        bglDepthMask(GL_TRUE);
    }
#endif
    spritesortcnt = 0;

#ifdef POLYMER
    if (getrendermode() == REND_POLYMER)
        polymer_drawmasks();
#endif
#ifdef DEBUG_MASK_DRAWING
    if (g_maskDrawMode && getrendermode() == REND_CLASSIC)
    {
        for (i=0; i<dmasknum; i++)
        {
            EDUKE32_STATIC_ASSERT(MAXWALLS <= 32768 && MAXSPRITES <= 32768);
            int32_t spritep = !!(debugmask[i].di & 32768);
            int32_t di = debugmask[i].di & 32767;
//            int32_t ii = debugmask[i].i;

            char numstr[12];
            Bsprintf(numstr, "%d", i+1);

            if (spritep)
            {
                int32_t sx = spritesxyz[di].x>>8, sy = ydim/2 + 8;
                // XXX: printext256 really ought to do bound checking on the
                // x/y coords!
                sx = clamp(sx, 0, xdim-8*Bstrlen(numstr)-1);
                printext256(sx, sy, 241, 0, numstr, 0);
            }
            else
            {
                int32_t sx = xb1[di] + (xb2[di]-xb1[di])/2, sy = ydim/2;
                sx = clamp(sx, 0, xdim-8*Bstrlen(numstr)-1);
                printext256(sx, sy, 31, 0, numstr, 0);
            }
        }
    }
#endif

    enddrawing();   //}}}
}

//
// drawmapview
//
void drawmapview(int32_t dax, int32_t day, int32_t zoome, int16_t ang)
{
    int32_t i, j, k, l;
    int32_t x, y;
    int32_t s, ox, oy;

    int32_t const oyxaspect = yxaspect, oviewingrange = viewingrange;

    setaspect(65536, divscale16((320*5)/8, 200));

    beforedrawrooms = 0;

    Bmemset(gotsector, 0, (numsectors+7)>>3);

    vec2_t const c1 = { (windowxy1.x<<12), (windowxy1.y<<12) };
    vec2_t const c2 = { ((windowxy2.x+1)<<12)-1, ((windowxy2.y+1)<<12)-1 };

    zoome <<= 8;

    vec2_t const bakgvect = { divscale28(sintable[(1536 - ang) & 2047], zoome),
                        divscale28(sintable[(2048 - ang) & 2047], zoome) };
    vec2_t const vect = { mulscale8(sintable[(2048 - ang) & 2047], zoome), mulscale8(sintable[(1536 - ang) & 2047], zoome) };
    vec2_t const vect2 = { mulscale16(vect.x, yxaspect), mulscale16(vect.y, yxaspect) };

    int32_t sortnum = 0;

    begindrawing(); //{{{

    usectortype *sec;

    for (s=0,sec=(usectortype *)&sector[s]; s<numsectors; s++,sec++)
        if (show2dsector[s>>3]&pow2char[s&7])
        {
#ifdef YAX_ENABLE
            if (yax_getbunch(s, YAX_FLOOR) >= 0 && (sector[s].floorstat&(256+128))==0)
                continue;
#endif
            int32_t npoints = 0; i = 0;
            int32_t startwall = sec->wallptr;
#if 0
            for (w=sec->wallnum,wal=&wall[startwall]; w>0; w--,wal++)
            {
                ox = wal->x - dax; oy = wal->y - day;
                x = dmulscale16(ox,xvect,-oy,yvect) + (xdim<<11);
                y = dmulscale16(oy,xvect2,ox,yvect2) + (ydim<<11);
                i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
                rx1[npoints] = x;
                ry1[npoints] = y;
                xb1[npoints] = wal->point2 - startwall;
                npoints++;
            }
#else
            j = startwall; l = 0;
            uwalltype *wal;
            int32_t w;
            for (w=sec->wallnum,wal=(uwalltype *)&wall[startwall]; w>0; w--,wal++,j++)
            {
                k = lastwall(j);
                if ((k > j) && (npoints > 0)) { xb1[npoints-1] = l; l = npoints; } //overwrite point2
                //wall[k].x wal->x wall[wal->point2].x
                //wall[k].y wal->y wall[wal->point2].y
                if (!dmulscale1(wal->x-wall[k].x,wall[wal->point2].y-wal->y,-(wal->y-wall[k].y),wall[wal->point2].x-wal->x)) continue;
                ox = wal->x - dax; oy = wal->y - day;
                x = dmulscale16(ox,vect.x,-oy,vect.y) + (xdim<<11);
                y = dmulscale16(oy,vect2.x,ox,vect2.y) + (ydim<<11);
                i |= getclipmask(x-c1.x,c2.x-x,y-c1.y,c2.y-y);
                rx1[npoints] = x;
                ry1[npoints] = y;
                xb1[npoints] = npoints+1;
                npoints++;
            }
            if (npoints > 0) xb1[npoints-1] = l; //overwrite point2
#endif
            if ((i&0xf0) != 0xf0) continue;

            vec2_t bak = { rx1[0], mulscale16(ry1[0]-(ydim<<11),xyaspect)+(ydim<<11) };

            if (i&0x0f)
            {
                npoints = clippoly(npoints,i);
                if (npoints < 3) continue;
            }

            //Collect floor sprites to draw
            for (i=headspritesect[s]; i>=0; i=nextspritesect[i])
                if ((sprite[i].cstat&48) == 32)
                {
                    if ((sprite[i].cstat&(64+8)) == (64+8)) continue;
                    tsprite[sortnum++].owner = i;
                }

            gotsector[s>>3] |= pow2char[s&7];

            globalorientation = (int32_t)sec->floorstat;
            if ((globalorientation&1) != 0) continue;

            globalpal = sec->floorpal;

            if (palookup[sec->floorpal] != globalpalwritten)
            {
                globalpalwritten = palookup[sec->floorpal];
                if (!globalpalwritten) globalpalwritten = palookup[0];	// JBF: fixes null-pointer crash
                setpalookupaddress(globalpalwritten);
            }
            globalpicnum = sec->floorpicnum;
            if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
            DO_TILE_ANIM(globalpicnum, s);
            setgotpic(globalpicnum);
            if ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0)) continue;
            if (waloff[globalpicnum] == 0) loadtile(globalpicnum);
            globalbufplc = waloff[globalpicnum];
            globalshade = max(min(sec->floorshade,numshades-1),0);
            globvis = globalhisibility;
            if (sec->visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sec->visibility+16));
            globalpolytype = 0;
            if ((globalorientation&64) == 0)
            {
                set_globalpos(dax, day, globalposz);
                globalx1 = bakgvect.x; globaly1 = bakgvect.y;
                globalx2 = bakgvect.x; globaly2 = bakgvect.y;
            }
            else
            {
                ox = wall[wall[startwall].point2].x - wall[startwall].x;
                oy = wall[wall[startwall].point2].y - wall[startwall].y;
                i = nsqrtasm(uhypsq(ox,oy)); if (i == 0) continue;
                i = 1048576/i;
                globalx1 = mulscale10(dmulscale10(ox,bakgvect.x,oy,bakgvect.y),i);
                globaly1 = mulscale10(dmulscale10(ox,bakgvect.y,-oy,bakgvect.x),i);
                ox = (bak.x>>4)-(xdim<<7); oy = (bak.y>>4)-(ydim<<7);
                globalposx = dmulscale28(-oy, globalx1, -ox, globaly1);
                globalposy = dmulscale28(-ox, globalx1, oy, globaly1);
                globalx2 = -globalx1;
                globaly2 = -globaly1;

                int32_t const daslope = sector[s].floorheinum;
                i = nsqrtasm(daslope*daslope+16777216);
                set_globalpos(globalposx, mulscale12(globalposy,i), globalposz);
                globalx2 = mulscale12(globalx2,i);
                globaly2 = mulscale12(globaly2,i);
            }

            calc_globalshifts();

            sethlinesizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,globalbufplc);

            if ((globalorientation&0x4) > 0)
            {
                i = globalposx; globalposx = -globalposy; globalposy = -i;
                i = globalx2; globalx2 = globaly1; globaly1 = i;
                i = globalx1; globalx1 = -globaly2; globaly2 = -i;
            }
            if ((globalorientation&0x10) > 0) globalx1 = -globalx1, globaly1 = -globaly1, globalposx = -globalposx;
            if ((globalorientation&0x20) > 0) globalx2 = -globalx2, globaly2 = -globaly2, globalposy = -globalposy;
            asm1 = (globaly1<<globalxshift);
            asm2 = (globalx2<<globalyshift);
            globalx1 <<= globalxshift;
            globaly2 <<= globalyshift;
            set_globalpos(((int64_t) globalposx<<(20+globalxshift))+(((uint32_t) sec->floorxpanning)<<24),
                ((int64_t) globalposy<<(20+globalyshift))-(((uint32_t) sec->floorypanning)<<24),
                globalposz);
            fillpolygon(npoints);
        }

    //Sort sprite list
    int32_t gap = 1;

    while (gap < sortnum) gap = (gap << 1) + 1;

    for (gap>>=1; gap>0; gap>>=1)
        for (i=0; i<sortnum-gap; i++)
            for (j=i; j>=0; j-=gap)
            {
                if (sprite[tsprite[j].owner].z <= sprite[tsprite[j+gap].owner].z) break;
                swapshort(&tsprite[j].owner,&tsprite[j+gap].owner);
            }

    for (s=sortnum-1; s>=0; s--)
    {
        uspritetype * const spr = (uspritetype * )&sprite[tsprite[s].owner];
        if ((spr->cstat&48) == 32)
        {
            const int32_t xspan = tilesiz[spr->picnum].x;

            int32_t npoints = 0;
            vec2_t v1 = { spr->x, spr->y }, v2, v3, v4;

            get_floorspr_points(spr, 0, 0, &v1.x, &v2.x, &v3.x, &v4.x,
                                &v1.y, &v2.y, &v3.y, &v4.y);

            xb1[0] = 1; xb1[1] = 2; xb1[2] = 3; xb1[3] = 0;
            npoints = 4;

            i = 0;

            ox = v1.x - dax; oy = v1.y - day;
            x = dmulscale16(ox,vect.x,-oy,vect.y) + (xdim<<11);
            y = dmulscale16(oy,vect2.x,ox,vect2.y) + (ydim<<11);
            i |= getclipmask(x-c1.x,c2.x-x,y-c1.y,c2.y-y);
            rx1[0] = x; ry1[0] = y;

            ox = v2.x - dax; oy = v2.y - day;
            x = dmulscale16(ox,vect.x,-oy,vect.y) + (xdim<<11);
            y = dmulscale16(oy,vect2.x,ox,vect2.y) + (ydim<<11);
            i |= getclipmask(x-c1.x,c2.x-x,y-c1.y,c2.y-y);
            rx1[1] = x; ry1[1] = y;

            ox = v3.x - dax; oy = v3.y - day;
            x = dmulscale16(ox,vect.x,-oy,vect.y) + (xdim<<11);
            y = dmulscale16(oy,vect2.x,ox,vect2.y) + (ydim<<11);
            i |= getclipmask(x-c1.x,c2.x-x,y-c1.y,c2.y-y);
            rx1[2] = x; ry1[2] = y;

            x = rx1[0]+rx1[2]-rx1[1];
            y = ry1[0]+ry1[2]-ry1[1];
            i |= getclipmask(x-c1.x,c2.x-x,y-c1.y,c2.y-y);
            rx1[3] = x; ry1[3] = y;

            if ((i&0xf0) != 0xf0) continue;

            vec2_t bak = { rx1[0], mulscale16(ry1[0] - (ydim << 11), xyaspect) + (ydim << 11) };

            if (i&0x0f)
            {
                npoints = clippoly(npoints,i);
                if (npoints < 3) continue;
            }

            globalpicnum = spr->picnum;
            globalpal = spr->pal; // GL needs this, software doesn't
            if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
            DO_TILE_ANIM(globalpicnum, s);
            setgotpic(globalpicnum);
            if ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0)) continue;
            if (waloff[globalpicnum] == 0) loadtile(globalpicnum);
            globalbufplc = waloff[globalpicnum];

            // 'loading' the tile doesn't actually guarantee that it's there afterwards.
            // This can really happen when drawing the second frame of a floor-aligned
            // 'storm icon' sprite (4894+1)
            if (!globalbufplc)
                continue;

            if ((sector[spr->sectnum].ceilingstat&1) > 0)
                globalshade = ((int32_t)sector[spr->sectnum].ceilingshade);
            else
                globalshade = ((int32_t)sector[spr->sectnum].floorshade);
            globalshade = max(min(globalshade+spr->shade+6,numshades-1),0);
            asm3 = FP_OFF(palookup[spr->pal]+(globalshade<<8));
            globvis = globalhisibility;
            if (sec->visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sec->visibility+16));
            globalpolytype = ((spr->cstat&2)>>1)+1;

            //relative alignment stuff
            ox = v2.x-v1.x; oy = v2.y-v1.y;
            i = ox*ox+oy*oy; if (i == 0) continue; i = tabledivide32_noinline(65536*16384, i);
            globalx1 = mulscale10(dmulscale10(ox,bakgvect.x,oy,bakgvect.y),i);
            globaly1 = mulscale10(dmulscale10(ox,bakgvect.y,-oy,bakgvect.x),i);
            ox = v1.y-v4.y; oy = v4.x-v1.x;
            i = ox*ox+oy*oy; if (i == 0) continue; i = tabledivide32_noinline(65536*16384, i);
            globalx2 = mulscale10(dmulscale10(ox,bakgvect.x,oy,bakgvect.y),i);
            globaly2 = mulscale10(dmulscale10(ox,bakgvect.y,-oy,bakgvect.x),i);

            ox = picsiz[globalpicnum]; oy = ((ox>>4)&15); ox &= 15;
            if (pow2long[ox] != xspan)
            {
                ox++;
                globalx1 = mulscale(globalx1,xspan,ox);
                globaly1 = mulscale(globaly1,xspan,ox);
            }

            bak.x = (bak.x>>4)-(xdim<<7); bak.y = (bak.y>>4)-(ydim<<7);
            globalposx = dmulscale28(-bak.y,globalx1,-bak.x,globaly1);
            globalposy = dmulscale28(bak.x,globalx2,-bak.y,globaly2);

            if ((spr->cstat&2) == 0)
                msethlineshift(ox,oy);
            else
            {
                setup_blend(spr->blend, spr->cstat&512);
                tsethlineshift(ox,oy);
            }

            if ((spr->cstat&0x4) > 0) globalx1 = -globalx1, globaly1 = -globaly1, globalposx = -globalposx;
            asm1 = (globaly1<<2); globalx1 <<= 2; globalposx <<= (20+2);
            asm2 = (globalx2<<2); globaly2 <<= 2; globalposy <<= (20+2);

            set_globalpos(globalposx, globalposy, globalposz);

            // so polymost can get the translucency. ignored in software mode:
            globalorientation = ((spr->cstat&2)<<7) | ((spr->cstat&512)>>2);
            fillpolygon(npoints);
        }
    }

    enddrawing();   //}}}

    if (r_usenewaspect)
        setaspect(oviewingrange, oyxaspect);
    else
        setaspect(65536, divscale16(ydim*320, xdim*200));
}

//////////////////// LOADING AND SAVING ROUTINES ////////////////////

static FORCE_INLINE int32_t have_maptext(void)
{
    return (mapversion >= 10);
}

static void prepare_loadboard(int32_t fil, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum)
{
    initspritelists();

    Bmemset(show2dsector, 0, sizeof(show2dsector));
    Bmemset(show2dsprite, 0, sizeof(show2dsprite));
    Bmemset(show2dwall, 0, sizeof(show2dwall));
    Bmemset(sectorchanged, 0, sizeof(sectorchanged));
    Bmemset(spritechanged, 0, sizeof(spritechanged));
    Bmemset(wallchanged, 0, sizeof(wallchanged));

#ifdef USE_OPENGL
    Polymost_prepare_loadboard();
#endif

    if (!have_maptext())
    {
        kread(fil,&dapos->x,4); dapos->x = B_LITTLE32(dapos->x);
        kread(fil,&dapos->y,4); dapos->y = B_LITTLE32(dapos->y);
        kread(fil,&dapos->z,4); dapos->z = B_LITTLE32(dapos->z);
        kread(fil,daang,2);  *daang  = B_LITTLE16(*daang) & 2047;
        kread(fil,dacursectnum,2); *dacursectnum = B_LITTLE16(*dacursectnum);
    }
}

static int32_t finish_loadboard(const vec3_t *dapos, int16_t *dacursectnum, int16_t numsprites, char myflags)
{
    int32_t i, realnumsprites=numsprites, numremoved;

#if !defined USE_OPENGL || !defined POLYMER
    UNREFERENCED_PARAMETER(myflags);
#endif

    for (i=0; i<numsprites; i++)
    {
        int32_t removeit = 0;

        if ((sprite[i].cstat & 48) == 48)
            sprite[i].cstat &= ~48;

        if (sprite[i].statnum == MAXSTATUS)
        {
            // Sprite was removed in loadboard() -> check_sprite(). Insert it
            // for now, because we must maintain the sprite numbering.
            sprite[i].statnum = sprite[i].sectnum = 0;
            removeit = 1;
        }

        insertsprite(sprite[i].sectnum, sprite[i].statnum);

        if (removeit)
        {
            // Flag .statnum==MAXSTATUS, temporarily creating an inconsistency
            // with sprite list.
            sprite[i].statnum = MAXSTATUS;
            realnumsprites--;
        }
    }

    if (numsprites != realnumsprites)
    {
        for (i=0; i<numsprites; i++)
            if (sprite[i].statnum == MAXSTATUS)
            {
                // Now remove it for real!
                sprite[i].statnum = 0;
                deletesprite(i);
            }
    }

    numremoved = (numsprites-realnumsprites);
    numsprites = realnumsprites;
    Bassert(numsprites == Numsprites);

    //Must be after loading sectors, etc!
    updatesector(dapos->x, dapos->y, dacursectnum);

#ifdef HAVE_CLIPSHAPE_FEATURE
    if (!quickloadboard)
#endif
    {
        Bmemset(spriteext, 0, sizeof(spriteext_t)*MAXSPRITES);
#ifndef NEW_MAP_FORMAT
        Bmemset(wallext, 0, sizeof(wallext_t)*MAXWALLS);
#endif

#ifdef USE_OPENGL
        Bmemset(spritesmooth, 0, sizeof(spritesmooth_t)*(MAXSPRITES+MAXUNIQHUDID));

# ifdef POLYMER
        if (getrendermode() == REND_POLYMER)
        {
            if ((myflags&4)==0)
                polymer_loadboard();
        }
# endif
#endif
    }

    guniqhudid = 0;

    Bmemset(tilecols, 0, sizeof(tilecols));
    return numremoved;
}


#define MYMAXSECTORS() (MAXSECTORS==MAXSECTORSV7 || mapversion <= 7 ? MAXSECTORSV7 : MAXSECTORSV8)
#define MYMAXWALLS()   (MAXSECTORS==MAXSECTORSV7 || mapversion <= 7 ? MAXWALLSV7 : MAXWALLSV8)
#define MYMAXSPRITES() (MAXSECTORS==MAXSECTORSV7 || mapversion <= 7 ? MAXSPRITESV7 : MAXSPRITESV8)

// Sprite checking

static void remove_sprite(int32_t i)
{
    Bmemset(&sprite[i], 0, sizeof(spritetype));
    sprite[i].statnum = MAXSTATUS;
    sprite[i].sectnum = MAXSECTORS;
}

// This is only to be run after reading the sprite array!
static void check_sprite(int32_t i)
{
    if ((unsigned)sprite[i].statnum >= MAXSTATUS)
    {
        initprintf("%sMap error: sprite #%d (%d,%d) with illegal statnum (%d) REMOVED.\n", osd->draw.errorfmt,
                   i, TrackerCast(sprite[i].x), TrackerCast(sprite[i].y), TrackerCast(sprite[i].statnum));
        remove_sprite(i);
    }
    else if ((unsigned)sprite[i].picnum >= MAXTILES)
    {
        initprintf("%sMap error: sprite #%d (%d,%d) with illegal picnum (%d) REMOVED.\n", osd->draw.errorfmt,
                   i, TrackerCast(sprite[i].x), TrackerCast(sprite[i].y), TrackerCast(sprite[i].sectnum));
        remove_sprite(i);
    }
    else if ((unsigned)sprite[i].sectnum >= (unsigned)numsectors)
    {
        const int32_t osectnum = sprite[i].sectnum;

        sprite[i].sectnum = -1;
        updatesector(sprite[i].x, sprite[i].y, &sprite[i].sectnum);

        if (sprite[i].sectnum < 0)
            remove_sprite(i);

        initprintf("%sMap error: sprite #%d (%d,%d) with illegal sector (%d) ", osd->draw.errorfmt,
                   i, TrackerCast(sprite[i].x), TrackerCast(sprite[i].y), osectnum);

        if (sprite[i].statnum != MAXSTATUS)
            initprintf("changed to sector %d.\n", TrackerCast(sprite[i].sectnum));
        else
            initprintf("REMOVED.\n");
    }
}

#ifdef NEW_MAP_FORMAT
// Returns the number of sprites, or <0 on error.
LUNATIC_CB int32_t (*loadboard_maptext)(int32_t fil, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum);
#endif

#include "md4.h"

// flags: 1, 2: former parameter "fromwhere"
//           4: don't call polymer_loadboard
//           8: don't autoexec <mapname>.cfg
// returns: on success, number of removed sprites
//          -1: file not found
//          -2: invalid version
//          -3: invalid number of sectors, walls or sprites
//       <= -4: map-text error
int32_t loadboard(const char *filename, char flags, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum)
{
    int32_t fil, i;
    int16_t numsprites;
    const char myflags = flags&(~3);

    flags &= 3;

    if ((fil = kopen4load(filename,flags)) == -1)
        { mapversion = 7; return -1; }

    if (kread(fil, &mapversion, 4) != 4)
    {
        kclose(fil);
        return -2;
    }

    {
        int32_t ok = 0;

#ifdef NEW_MAP_FORMAT
        // Check for map-text first.
        if (!Bmemcmp(&mapversion, "--ED", 4))
        {
            mapversion = 10;
            ok = 1;
        }
        else
#endif
        {
            // Not map-text. We expect a little-endian version int now.
            mapversion = B_LITTLE32(mapversion);
#ifdef YAX_ENABLE
            ok |= (mapversion==9);
#endif
#if MAXSECTORS==MAXSECTORSV8
            // v8 engine
            ok |= (mapversion==8);
#endif
            ok |= (mapversion==7);
        }

        if (!ok)
        {
            kclose(fil);
            return -2;
        }
    }

    prepare_loadboard(fil, dapos, daang, dacursectnum);

#ifdef NEW_MAP_FORMAT
    if (have_maptext())
    {
        int32_t ret = klseek(fil, 0, SEEK_SET);

        if (ret == 0)
            ret = loadboard_maptext(fil, dapos, daang, dacursectnum);

        if (ret < 0)
        {
            kclose(fil);
            return ret;
        }

        numsprites = ret;
        goto skip_reading_mapbin;
    }
#endif

    ////////// Read sectors //////////

    kread(fil,&numsectors,2); numsectors = B_LITTLE16(numsectors);
    if ((unsigned)numsectors >= MYMAXSECTORS()+1) { kclose(fil); return -3; }

    kread(fil, sector, sizeof(sectortypev7)*numsectors);

    for (i=numsectors-1; i>=0; i--)
    {
#ifdef NEW_MAP_FORMAT
        Bmemmove(&sector[i], &(((sectortypev7 *)sector)[i]), sizeof(sectortypevx));
        inplace_vx_from_v7_sector(&sector[i]);
#endif
        sector[i].wallptr       = B_LITTLE16(sector[i].wallptr);
        sector[i].wallnum       = B_LITTLE16(sector[i].wallnum);
        sector[i].ceilingz      = B_LITTLE32(sector[i].ceilingz);
        sector[i].floorz        = B_LITTLE32(sector[i].floorz);
        sector[i].ceilingstat   = B_LITTLE16(sector[i].ceilingstat);
        sector[i].floorstat     = B_LITTLE16(sector[i].floorstat);
        sector[i].ceilingpicnum = B_LITTLE16(sector[i].ceilingpicnum);
        sector[i].ceilingheinum = B_LITTLE16(sector[i].ceilingheinum);
        sector[i].floorpicnum   = B_LITTLE16(sector[i].floorpicnum);
        sector[i].floorheinum   = B_LITTLE16(sector[i].floorheinum);
        sector[i].lotag         = B_LITTLE16(sector[i].lotag);
        sector[i].hitag         = B_LITTLE16(sector[i].hitag);
        sector[i].extra         = B_LITTLE16(sector[i].extra);
#ifdef NEW_MAP_FORMAT
        inplace_vx_tweak_sector(&sector[i], mapversion==9);
#endif
    }


    ////////// Read walls //////////

    kread(fil,&numwalls,2); numwalls = B_LITTLE16(numwalls);
    if ((unsigned)numwalls >= MYMAXWALLS()+1) { kclose(fil); return -3; }

    kread(fil, wall, sizeof(walltypev7)*numwalls);

    for (i=numwalls-1; i>=0; i--)
    {
#ifdef NEW_MAP_FORMAT
        Bmemmove(&wall[i], &(((walltypev7 *)wall)[i]), sizeof(walltypevx));
        inplace_vx_from_v7_wall(&wall[i]);
#endif
        wall[i].x          = B_LITTLE32(wall[i].x);
        wall[i].y          = B_LITTLE32(wall[i].y);
        wall[i].point2     = B_LITTLE16(wall[i].point2);
        wall[i].nextwall   = B_LITTLE16(wall[i].nextwall);
        wall[i].nextsector = B_LITTLE16(wall[i].nextsector);
        wall[i].cstat      = B_LITTLE16(wall[i].cstat);
        wall[i].picnum     = B_LITTLE16(wall[i].picnum);
        wall[i].overpicnum = B_LITTLE16(wall[i].overpicnum);
        wall[i].lotag      = B_LITTLE16(wall[i].lotag);
        wall[i].hitag      = B_LITTLE16(wall[i].hitag);
        wall[i].extra      = B_LITTLE16(wall[i].extra);
#ifdef NEW_MAP_FORMAT
        inplace_vx_tweak_wall(&wall[i], mapversion==9);
#endif
    }


    ////////// Read sprites //////////

    kread(fil,&numsprites,2); numsprites = B_LITTLE16(numsprites);
    if ((unsigned)numsprites >= MYMAXSPRITES()+1) { kclose(fil); return -3; }

    kread(fil, sprite, sizeof(spritetype)*numsprites);

#ifdef NEW_MAP_FORMAT
skip_reading_mapbin:
#endif

    klseek(fil, 0, SEEK_SET);
    int32_t boardsize = kfilelength(fil);
    uint8_t *fullboard = (uint8_t*)Xmalloc(boardsize);
    kread(fil, fullboard, boardsize);
    md4once(fullboard, boardsize, g_loadedMapHack.md4);
    Bfree(fullboard);

    kclose(fil);
    // Done reading file.

    for (i=numsprites-1; i>=0; i--)
    {
        if (!have_maptext())
        {
            sprite[i].x       = B_LITTLE32(sprite[i].x);
            sprite[i].y       = B_LITTLE32(sprite[i].y);
            sprite[i].z       = B_LITTLE32(sprite[i].z);
            sprite[i].cstat   = B_LITTLE16(sprite[i].cstat);
            sprite[i].picnum  = B_LITTLE16(sprite[i].picnum);
            sprite[i].sectnum = B_LITTLE16(sprite[i].sectnum);
            sprite[i].statnum = B_LITTLE16(sprite[i].statnum);
            sprite[i].ang     = B_LITTLE16(sprite[i].ang);
            sprite[i].owner   = B_LITTLE16(sprite[i].owner);
            sprite[i].xvel    = B_LITTLE16(sprite[i].xvel);
            sprite[i].yvel    = B_LITTLE16(sprite[i].yvel);
            sprite[i].zvel    = B_LITTLE16(sprite[i].zvel);
            sprite[i].lotag   = B_LITTLE16(sprite[i].lotag);
            sprite[i].hitag   = B_LITTLE16(sprite[i].hitag);
            sprite[i].extra   = B_LITTLE16(sprite[i].extra);
        }

        check_sprite(i);
    }

    // Back up the map version of the *loaded* map. Must be before yax_update().
    g_loadedMapVersion = mapversion;
#ifdef YAX_ENABLE
    yax_update(mapversion<9);
    if (editstatus)
        yax_updategrays(dapos->z);
#endif

    if ((myflags&8)==0)
    {
        char fn[BMAX_PATH];

        Bstrcpy(fn, filename);
        append_ext_UNSAFE(fn, ".cfg");

        OSD_Exec(fn);

        system_getcvars();

        // Per-map ART
        E_MapArt_Setup(filename);
    }

    // initprintf("Loaded map \"%s\" (md4sum: %08x%08x%08x%08x)\n", filename, B_BIG32(*((int32_t*)&md4out[0])), B_BIG32(*((int32_t*)&md4out[4])), B_BIG32(*((int32_t*)&md4out[8])), B_BIG32(*((int32_t*)&md4out[12])));

    return finish_loadboard(dapos, dacursectnum, numsprites, myflags);
}


//
// loadboardv5/6
//
#include "engine_oldmap.h"

// Powerslave uses v6
// Witchaven 1 and TekWar and LameDuke use v5
int32_t loadoldboard(const char *filename, char fromwhere, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum)
{
    int32_t fil, i;
    int16_t numsprites;

    struct sectortypev5 v5sect;
    struct walltypev5   v5wall;
    struct spritetypev5 v5spr;
    struct sectortypev6 v6sect;
    struct walltypev6   v6wall;
    struct spritetypev6 v6spr;

    if ((fil = kopen4load(filename,fromwhere)) == -1)
        { mapversion = 5L; return -1; }

    kread(fil,&mapversion,4); mapversion = B_LITTLE32(mapversion);
    if (mapversion != 5L && mapversion != 6L) { kclose(fil); return -2; }

    prepare_loadboard(fil, dapos, daang, dacursectnum);

    kread(fil,&numsectors,2); numsectors = B_LITTLE16(numsectors);
    if (numsectors > MAXSECTORS) { kclose(fil); return -1; }
    for (i=0; i<numsectors; i++)
    {
        switch (mapversion)
        {
        case 5:
            kread(fil,&v5sect,sizeof(struct sectortypev5));
            v5sect.wallptr = B_LITTLE16(v5sect.wallptr);
            v5sect.wallnum = B_LITTLE16(v5sect.wallnum);
            v5sect.ceilingpicnum = B_LITTLE16(v5sect.ceilingpicnum);
            v5sect.floorpicnum = B_LITTLE16(v5sect.floorpicnum);
            v5sect.ceilingheinum = B_LITTLE16(v5sect.ceilingheinum);
            v5sect.floorheinum = B_LITTLE16(v5sect.floorheinum);
            v5sect.ceilingz = B_LITTLE32(v5sect.ceilingz);
            v5sect.floorz = B_LITTLE32(v5sect.floorz);
            v5sect.lotag = B_LITTLE16(v5sect.lotag);
            v5sect.hitag = B_LITTLE16(v5sect.hitag);
            v5sect.extra = B_LITTLE16(v5sect.extra);
            break;
        case 6:
            kread(fil,&v6sect,sizeof(struct sectortypev6));
            v6sect.wallptr = B_LITTLE16(v6sect.wallptr);
            v6sect.wallnum = B_LITTLE16(v6sect.wallnum);
            v6sect.ceilingpicnum = B_LITTLE16(v6sect.ceilingpicnum);
            v6sect.floorpicnum = B_LITTLE16(v6sect.floorpicnum);
            v6sect.ceilingheinum = B_LITTLE16(v6sect.ceilingheinum);
            v6sect.floorheinum = B_LITTLE16(v6sect.floorheinum);
            v6sect.ceilingz = B_LITTLE32(v6sect.ceilingz);
            v6sect.floorz = B_LITTLE32(v6sect.floorz);
            v6sect.lotag = B_LITTLE16(v6sect.lotag);
            v6sect.hitag = B_LITTLE16(v6sect.hitag);
            v6sect.extra = B_LITTLE16(v6sect.extra);
            break;
        }

        switch (mapversion)
        {
        case 5:
            convertv5sectv6(&v5sect,&v6sect);
            fallthrough__;
        case 6:
            convertv6sectv7(&v6sect,&sector[i]);
            break;
        }
    }

    kread(fil,&numwalls,2); numwalls = B_LITTLE16(numwalls);
    if (numwalls > MAXWALLS) { kclose(fil); return -1; }
    for (i=0; i<numwalls; i++)
    {
        switch (mapversion)
        {
        case 5:
            kread(fil,&v5wall,sizeof(struct walltypev5));
            v5wall.x = B_LITTLE32(v5wall.x);
            v5wall.y = B_LITTLE32(v5wall.y);
            v5wall.point2 = B_LITTLE16(v5wall.point2);
            v5wall.picnum = B_LITTLE16(v5wall.picnum);
            v5wall.overpicnum = B_LITTLE16(v5wall.overpicnum);
            v5wall.cstat = B_LITTLE16(v5wall.cstat);
            v5wall.nextsector1 = B_LITTLE16(v5wall.nextsector1);
            v5wall.nextwall1 = B_LITTLE16(v5wall.nextwall1);
            v5wall.nextsector2 = B_LITTLE16(v5wall.nextsector2);
            v5wall.nextwall2 = B_LITTLE16(v5wall.nextwall2);
            v5wall.lotag = B_LITTLE16(v5wall.lotag);
            v5wall.hitag = B_LITTLE16(v5wall.hitag);
            v5wall.extra = B_LITTLE16(v5wall.extra);
            break;
        case 6:
            kread(fil,&v6wall,sizeof(struct walltypev6));
            v6wall.x = B_LITTLE32(v6wall.x);
            v6wall.y = B_LITTLE32(v6wall.y);
            v6wall.point2 = B_LITTLE16(v6wall.point2);
            v6wall.nextsector = B_LITTLE16(v6wall.nextsector);
            v6wall.nextwall = B_LITTLE16(v6wall.nextwall);
            v6wall.picnum = B_LITTLE16(v6wall.picnum);
            v6wall.overpicnum = B_LITTLE16(v6wall.overpicnum);
            v6wall.cstat = B_LITTLE16(v6wall.cstat);
            v6wall.lotag = B_LITTLE16(v6wall.lotag);
            v6wall.hitag = B_LITTLE16(v6wall.hitag);
            v6wall.extra = B_LITTLE16(v6wall.extra);
            break;
        }

        switch (mapversion)
        {
        case 5:
            convertv5wallv6(&v5wall,&v6wall,i);
            fallthrough__;
        case 6:
            convertv6wallv7(&v6wall,&wall[i]);
            break;
        }
    }

    kread(fil,&numsprites,2); numsprites = B_LITTLE16(numsprites);
    if (numsprites > MAXSPRITES) { kclose(fil); return -1; }
    for (i=0; i<numsprites; i++)
    {
        switch (mapversion)
        {
        case 5:
            kread(fil,&v5spr,sizeof(struct spritetypev5));
            v5spr.x = B_LITTLE32(v5spr.x);
            v5spr.y = B_LITTLE32(v5spr.y);
            v5spr.z = B_LITTLE32(v5spr.z);
            v5spr.picnum = B_LITTLE16(v5spr.picnum);
            v5spr.ang = B_LITTLE16(v5spr.ang);
            v5spr.xvel = B_LITTLE16(v5spr.xvel);
            v5spr.yvel = B_LITTLE16(v5spr.yvel);
            v5spr.zvel = B_LITTLE16(v5spr.zvel);
            v5spr.owner = B_LITTLE16(v5spr.owner);
            v5spr.sectnum = B_LITTLE16(v5spr.sectnum);
            v5spr.statnum = B_LITTLE16(v5spr.statnum);
            v5spr.lotag = B_LITTLE16(v5spr.lotag);
            v5spr.hitag = B_LITTLE16(v5spr.hitag);
            v5spr.extra = B_LITTLE16(v5spr.extra);
            break;
        case 6:
            kread(fil,&v6spr,sizeof(struct spritetypev6));
            v6spr.x = B_LITTLE32(v6spr.x);
            v6spr.y = B_LITTLE32(v6spr.y);
            v6spr.z = B_LITTLE32(v6spr.z);
            v6spr.cstat = B_LITTLE16(v6spr.cstat);
            v6spr.picnum = B_LITTLE16(v6spr.picnum);
            v6spr.ang = B_LITTLE16(v6spr.ang);
            v6spr.xvel = B_LITTLE16(v6spr.xvel);
            v6spr.yvel = B_LITTLE16(v6spr.yvel);
            v6spr.zvel = B_LITTLE16(v6spr.zvel);
            v6spr.owner = B_LITTLE16(v6spr.owner);
            v6spr.sectnum = B_LITTLE16(v6spr.sectnum);
            v6spr.statnum = B_LITTLE16(v6spr.statnum);
            v6spr.lotag = B_LITTLE16(v6spr.lotag);
            v6spr.hitag = B_LITTLE16(v6spr.hitag);
            v6spr.extra = B_LITTLE16(v6spr.extra);
            break;
        }

        switch (mapversion)
        {
        case 5:
            convertv5sprv6(&v5spr,&v6spr);
            fallthrough__;
        case 6:
            convertv6sprv7(&v6spr,&sprite[i]);
            break;
        }

        check_sprite(i);
    }

    kclose(fil);
    // Done reading file.

    return finish_loadboard(dapos, dacursectnum, numsprites, 0);
}


#ifdef NEW_MAP_FORMAT
LUNATIC_CB int32_t (*saveboard_maptext)(const char *filename, const vec3_t *dapos, int16_t daang, int16_t dacursectnum);
#endif

// Get map version of external map format (<10: old binary format, ==10: new
// 'VX' map-text format).
static int32_t get_mapversion(void)
{
#ifdef YAX_ENABLE
    if (numyaxbunches > 0)
# ifdef NEW_MAP_FORMAT
        return 10;
# else
        return 9;
# endif
#endif

#ifdef NEW_MAP_FORMAT
    {
        int32_t i;
        for (i=0; i<numwalls; i++)
            if (wall[i].blend != 0)
                return 10;
    }
#endif
    if (numsectors > MAXSECTORSV7 || numwalls > MAXWALLSV7 || Numsprites > MAXSPRITESV7)
        return 8;

    return 7;
}

//
// saveboard
//
int32_t saveboard(const char *filename, const vec3_t *dapos, int16_t daang, int16_t dacursectnum)
{
    int16_t numsprites, ts;
    int32_t i, j, fil, tl;

    // First, some checking.
    for (j=0; j<MAXSPRITES; j++)
    {
        if ((unsigned)sprite[j].statnum > MAXSTATUS)
        {
            initprintf("Map error: sprite #%d(%d,%d) with an illegal statnum(%d)\n",
                       j,TrackerCast(sprite[j].x),TrackerCast(sprite[j].y),TrackerCast(sprite[j].statnum));
            changespritestat(j,0);
        }

        if ((unsigned)sprite[j].sectnum > MAXSECTORS)
        {
            initprintf("Map error: sprite #%d(%d,%d) with an illegal sectnum(%d)\n",
                       j,TrackerCast(sprite[j].x),TrackerCast(sprite[j].y),TrackerCast(sprite[j].sectnum));
            changespritesect(j,0);
        }
    }

    // Count the number of sprites.
    numsprites = 0;
    for (j=0; j<MAXSPRITES; j++)
    {
        if (sprite[j].statnum != MAXSTATUS)
            numsprites++;
    }

    // Check consistency of sprite-in-the-world predicate (.statnum != MAXSTATUS)
    // and the engine-reported number of sprites 'Numsprites'.
    Bassert(numsprites == Numsprites);

    // Determine the map version.
    mapversion = get_mapversion();

#ifdef NEW_MAP_FORMAT
    if (mapversion == 10)
    {
        initprintf("Saving of TROR maps not yet accessible in the Lunatic preview build\n");
        return -1;
//        return saveboard_maptext(filename, dapos, daang, dacursectnum);
    }
#endif

    fil = Bopen(filename, BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY, BS_IREAD|BS_IWRITE);

    if (fil == -1)
    {
        initprintf("Couldn't open \"%s\" for writing: %s\n", filename, strerror(errno));
        return -1;
    }

    tl = B_LITTLE32(mapversion);    Bwrite(fil,&tl,4);

    tl = B_LITTLE32(dapos->x);      Bwrite(fil,&tl,4);
    tl = B_LITTLE32(dapos->y);      Bwrite(fil,&tl,4);
    tl = B_LITTLE32(dapos->z);      Bwrite(fil,&tl,4);
    ts = B_LITTLE16(daang);        Bwrite(fil,&ts,2);
    ts = B_LITTLE16(dacursectnum); Bwrite(fil,&ts,2);

    ts = B_LITTLE16(numsectors);    Bwrite(fil,&ts,2);

    while (1)  // if, really
    {
        usectortypev7 *const tsect = (usectortypev7 *)Xmalloc(sizeof(usectortypev7) * numsectors);
        uwalltypev7 *twall;

#ifdef NEW_MAP_FORMAT
        for (i=0; i<numsectors; i++)
            copy_v7_from_vx_sector(&tsect[i], &sector[i]);
#else
        Bmemcpy(tsect, sector, sizeof(sectortypev7)*numsectors);
#endif

        for (i=0; i<numsectors; i++)
        {
            usectortypev7 *const sec = &tsect[i];

            sec->wallptr       = B_LITTLE16(sec->wallptr);
            sec->wallnum       = B_LITTLE16(sec->wallnum);
            sec->ceilingz      = B_LITTLE32(sec->ceilingz);
            sec->floorz        = B_LITTLE32(sec->floorz);
            sec->ceilingstat   = B_LITTLE16(sec->ceilingstat);
            sec->floorstat     = B_LITTLE16(sec->floorstat);
            sec->ceilingpicnum = B_LITTLE16(sec->ceilingpicnum);
            sec->ceilingheinum = B_LITTLE16(sec->ceilingheinum);
            sec->floorpicnum   = B_LITTLE16(sec->floorpicnum);
            sec->floorheinum   = B_LITTLE16(sec->floorheinum);
            sec->lotag         = B_LITTLE16(sec->lotag);
            sec->hitag         = B_LITTLE16(sec->hitag);
            sec->extra         = B_LITTLE16(sec->extra);
#ifdef YAX_ENABLE__COMPAT
            if (editstatus == 0)
            {
                // if in-game, pack game-time bunchnum data back into structs
                int32_t cf, bn;

                for (cf=0; cf<2; cf++)
                    if ((bn=yax_getbunch(i, cf)) >= 0)
                        YAX_PTRBUNCHNUM(tsect, i, cf) = bn;
            }
#endif
        }

        Bwrite(fil, tsect, sizeof(sectortypev7)*numsectors);
        Bfree(tsect);

        ts = B_LITTLE16(numwalls);
        Bwrite(fil,&ts,2);

        twall = (uwalltypev7 *)Xmalloc(sizeof(uwalltypev7) * numwalls);

#ifdef NEW_MAP_FORMAT
        for (i=0; i<numwalls; i++)
            copy_v7_from_vx_wall(&twall[i], &wall[i]);
#else
        Bmemcpy(twall, wall, sizeof(walltypev7)*numwalls);
#endif

        for (i=0; i<numwalls; i++)
        {
            uwalltypev7 *const wal = &twall[i];

            wal->x          = B_LITTLE32(wal->x);
            wal->y          = B_LITTLE32(wal->y);
            wal->point2     = B_LITTLE16(wal->point2);
            wal->nextwall   = B_LITTLE16(wal->nextwall);
            wal->nextsector = B_LITTLE16(wal->nextsector);
            wal->cstat      = B_LITTLE16(wal->cstat);
            wal->picnum     = B_LITTLE16(wal->picnum);
            wal->overpicnum = B_LITTLE16(wal->overpicnum);
#ifdef YAX_ENABLE__COMPAT
            if (editstatus == 0)
            {
                // if in-game, pack game-time yax-nextwall data back into structs
                int16_t ynw;
                if ((ynw=yax_getnextwall(i, YAX_CEILING))>=0)
                    YAX_PTRNEXTWALL(twall,i,YAX_CEILING) = ynw;
                if ((ynw=yax_getnextwall(i, YAX_FLOOR))>=0)
                    YAX_PTRNEXTWALL(twall,i,YAX_FLOOR) = ynw;
            }
#endif
            wal->lotag      = B_LITTLE16(wal->lotag);
            wal->hitag      = B_LITTLE16(wal->hitag);
            wal->extra      = B_LITTLE16(wal->extra);
        }

        Bwrite(fil, twall, sizeof(walltypev7)*numwalls);
        Bfree(twall);

        ts = B_LITTLE16(numsprites);    Bwrite(fil,&ts,2);

        if (numsprites > 0)
        {
            uspritetype *const tspri = (uspritetype *)Xmalloc(sizeof(spritetype) * numsprites);
            uspritetype *spri = tspri;

            for (j=0; j<MAXSPRITES; j++)
            {
                if (sprite[j].statnum != MAXSTATUS)
                {
                    Bmemcpy(spri, &sprite[j], sizeof(spritetype));
                    spri->x       = B_LITTLE32(spri->x);
                    spri->y       = B_LITTLE32(spri->y);
                    spri->z       = B_LITTLE32(spri->z);
                    spri->cstat   = B_LITTLE16(spri->cstat);
                    spri->picnum  = B_LITTLE16(spri->picnum);
                    spri->sectnum = B_LITTLE16(spri->sectnum);
                    spri->statnum = B_LITTLE16(spri->statnum);
                    spri->ang     = B_LITTLE16(spri->ang);
                    spri->owner   = B_LITTLE16(spri->owner);
                    spri->xvel    = B_LITTLE16(spri->xvel);
                    spri->yvel    = B_LITTLE16(spri->yvel);
                    spri->zvel    = B_LITTLE16(spri->zvel);
                    spri->lotag   = B_LITTLE16(spri->lotag);
                    spri->hitag   = B_LITTLE16(spri->hitag);
                    spri->extra   = B_LITTLE16(spri->extra);
                    spri++;
                }
            }

            Bwrite(fil, tspri, sizeof(spritetype)*numsprites);
            Bfree(tspri);
        }

        Bclose(fil);
        return 0;
    }

    Bclose(fil);
    return -1;
}

#define YSAVES ((xdim*MAXSPRITES)>>7)

static void initsmost(void)
{
    int32_t i;
    // Needed for the game's TILT_SETVIEWTOTILE_320.
    const int32_t clamped_ydim = max(ydim, 320);

    struct
    {
        void **ptr;
        size_t size;
    } dynarray[] = {
          { (void **)&smost, YSAVES * sizeof(int16_t) },
          { (void **)&umost, xdim * sizeof(int16_t) },
          { (void **)&dmost, xdim * sizeof(int16_t) },
          { (void **)&startumost, xdim * sizeof(int16_t) },
          { (void **)&startdmost, xdim * sizeof(int16_t) },
          { (void **)&bakumost, xdim * sizeof(int16_t) },
          { (void **)&bakdmost, xdim * sizeof(int16_t) },
          { (void **)&uplc, xdim * sizeof(int16_t) },
          { (void **)&dplc, xdim * sizeof(int16_t) },
          { (void **)&uwall, xdim * sizeof(int16_t) },
          { (void **)&dwall, xdim * sizeof(int16_t) },
          { (void **)&swplc, xdim * sizeof(int32_t) },
          { (void **)&lplc, xdim * sizeof(int32_t) },
          { (void **)&swall, xdim * sizeof(int32_t) },
          { (void **)&lwall, (xdim + 4) * sizeof(int32_t) },
          { (void **)&radarang2, xdim * sizeof(int16_t) },
          { (void **)&dotp1, clamped_ydim * sizeof(intptr_t) },
          { (void **)&dotp2, clamped_ydim * sizeof(intptr_t) },
          { (void **)&lastx, clamped_ydim * sizeof(int32_t) },
      };

    for (i = 0; i < (signed)ARRAY_SIZE(dynarray); i++)
    {
        Baligned_free(*dynarray[i].ptr);

        *dynarray[i].ptr = Xaligned_alloc(16, dynarray[i].size);
    }

    ysavecnt = YSAVES;
    nodesperline = tabledivide32_noinline(YSAVES, ydim);
}

#ifdef USE_OPENGL
static void PolymostProcessVoxels(void)
{
    if (!g_haveVoxels)
        return;

    g_haveVoxels = 0;

    OSD_Printf("Generating voxel models for Polymost. This may take a while...\n");
    nextpage();

    for (bssize_t i=0; i<MAXVOXELS; i++)
    {
        if (voxfilenames[i])
        {
            voxmodels[i] = voxload(voxfilenames[i]);
            DO_FREE_AND_NULL(voxfilenames[i]);
        }
    }
}
#endif

//
// setgamemode
//
// JBF: davidoption now functions as a windowed-mode flag (0 == windowed, 1 == fullscreen)
extern char videomodereset;
int32_t setgamemode(char davidoption, int32_t daxdim, int32_t daydim, int32_t dabpp)
{
    int32_t j;

#ifdef USE_OPENGL
    extern char nogl;

    if (nogl) dabpp = 8;
#endif
    daxdim = max(320, daxdim);
    daydim = max(200, daydim);

    if (in3dmode() && videomodereset == 0 &&
            (davidoption == fullscreen) && (xdim == daxdim) && (ydim == daydim) && (bpp == dabpp))
        return 0;

    Bstrcpy(kensmessage,"!!!! BUILD engine&tools programmed by Ken Silverman of E.G. RI."
           "  (c) Copyright 1995 Ken Silverman.  Summary:  BUILD = Ken. !!!!");
    //  if (getkensmessagecrc(FP_OFF(kensmessage)) != 0x56c764d4)
    //      { OSD_Printf("Nice try.\n"); Bexit(0); }

    //if (checkvideomode(&daxdim, &daydim, dabpp, davidoption)<0) return -1;

    //bytesperline is set in this function

    j = bpp;

    g_lastpalettesum = 0;
    if (setvideomode(daxdim,daydim,dabpp,davidoption) < 0) return -1;

    // Workaround possible bugs in the GL driver
    makeasmwriteable();

#ifdef USE_OPENGL
    if (dabpp > 8) rendmode = glrendmode;    // GL renderer
    else if (dabpp == 8 && j > 8) rendmode = REND_CLASSIC;
#endif

    xdim = daxdim;
    ydim = daydim;

#ifdef USE_OPENGL
    fxdim = (float) daxdim;
    fydim = (float) daydim;
#endif

    initsmost();

#ifdef HIGH_PRECISION_SPRITE
    swallf = (float *) Xrealloc(swallf, xdim * sizeof(float));
#endif

    Bfree(lookups);

    j = ydim*4;  //Leave room for horizlookup&horizlookup2
    lookups = (int32_t *)Xmalloc(2*j*sizeof(lookups[0]));

    horizlookup = lookups;
    horizlookup2 = lookups + j;
    horizycent = ((ydim*4)>>1);

    //Force drawrooms to call dosetaspect & recalculate stuff
    oxyaspect = oxdimen = oviewingrange = -1;

    calc_ylookup(bytesperline, ydim);

    setview(0L,0L,xdim-1,ydim-1);
    clearallviews(0L);
    setbrightness(curbrightness,0,0);

    if (searchx < 0) { searchx = halfxdimen; searchy = (ydimen>>1); }

#ifdef USE_OPENGL
    if (getrendermode() == REND_POLYMOST)
        PolymostProcessVoxels();

    if (getrendermode() >= REND_POLYMOST)
    {
        polymost_glreset();
        polymost_glinit();
    }
# ifdef POLYMER
    if (getrendermode() == REND_POLYMER)
    {
        if (!polymer_init())
            rendmode = REND_POLYMOST;
    }
#endif
#endif
    qsetmode = 200;
    return 0;
}


//
// nextpage
//
void nextpage(void)
{
    permfifotype *per;

    //char snotbuf[32];
    //j = 0; k = 0;
    //for(i=0;i<4096;i++)
    //   if (waloff[i] != 0)
    //   {
    //      sprintf(snotbuf,"%d-%d",i,tilesizx[i]*tilesizy[i]);
    //      printext256((j>>5)*40+32,(j&31)*6,walock[i]>>3,-1,snotbuf,1);
    //      k += tilesizx[i]*tilesizy[i];
    //      j++;
    //   }
    //sprintf(snotbuf,"Total: %d",k);
    //printext256((j>>5)*40+32,(j&31)*6,31,-1,snotbuf,1);

    switch (qsetmode)
    {
    case 200:
        begindrawing(); //{{{
        for (bssize_t i=permtail; i!=permhead; i=((i+1)&(MAXPERMS-1)))
        {
            per = &permfifo[i];
            if ((per->pagesleft > 0) && (per->pagesleft <= numpages))
                dorotatesprite(per->sx,per->sy,per->z,per->a,per->picnum,
                               per->dashade,per->dapalnum,per->dastat,per->daalpha,per->dablend,
                               per->cx1,per->cy1,per->cx2,per->cy2,per->uniqid);
        }
        enddrawing();   //}}}

        OSD_Draw();
        showframe(0);

        begindrawing(); //{{{
        for (bssize_t i=permtail; i!=permhead; i=((i+1)&(MAXPERMS-1)))
        {
            per = &permfifo[i];
            if (per->pagesleft >= 130)
                dorotatesprite(per->sx,per->sy,per->z,per->a,per->picnum,
                               per->dashade,per->dapalnum,per->dastat,per->daalpha,per->dablend,
                               per->cx1,per->cy1,per->cx2,per->cy2,per->uniqid);

            if (per->pagesleft&127) per->pagesleft--;
            if (((per->pagesleft&127) == 0) && (i == permtail))
                permtail = ((permtail+1)&(MAXPERMS-1));
        }
        enddrawing();   //}}}
        break;

    case 350:
    case 480:
        break;
    }
    faketimerhandler();

    if ((totalclock >= lastageclock+CACHEAGETIME) || (totalclock < lastageclock))
        { lastageclock = totalclock; agecache(); }

#ifdef USE_OPENGL
    omdtims = mdtims; mdtims = getticks();

    for (bssize_t i = 0; i < Numsprites; ++i)
        if ((mdpause && spriteext[i].mdanimtims) || (spriteext[i].flags & SPREXT_NOMDANIM))
            spriteext[i].mdanimtims += mdtims - omdtims;
#endif

    beforedrawrooms = 1;
    numframes++;
}

//
// qloadkvx
//
int32_t qloadkvx(int32_t voxindex, const char *filename)
{
    const int32_t fil = kopen4load(filename, 0);
    if (fil == -1)
        return -1;

    int32_t lengcnt = 0;
    const int32_t lengtot = kfilelength(fil);

    for (bssize_t i=0; i<MAXVOXMIPS; i++)
    {
        int32_t dasiz;
        kread(fil, &dasiz, 4); dasiz = B_LITTLE32(dasiz);

        //Must store filenames to use cacheing system :(
        voxlock[voxindex][i] = 200;
        allocache(&voxoff[voxindex][i], dasiz, &voxlock[voxindex][i]);

        char *ptr = (char *) voxoff[voxindex][i];
        kread(fil, ptr, dasiz);

        lengcnt += dasiz+4;
        if (lengcnt >= lengtot-768)
            break;
    }

    kclose(fil);

#ifdef USE_OPENGL
    if (voxmodels[voxindex])
    {
        voxfree(voxmodels[voxindex]);
        voxmodels[voxindex] = NULL;
    }

    Bfree(voxfilenames[voxindex]);
    voxfilenames[voxindex] = Bstrdup(filename);
    g_haveVoxels = 1;
#endif

    return 0;
}

void vox_undefine(int32_t const tile)
{
    ssize_t voxindex = tiletovox[tile];
    if (voxindex < 0)
        return;

#ifdef USE_OPENGL
    if (voxmodels[voxindex])
    {
        voxfree(voxmodels[voxindex]);
        voxmodels[voxindex] = NULL;
    }
    DO_FREE_AND_NULL(voxfilenames[voxindex]);
#endif

    for (ssize_t j = 0; j < MAXVOXMIPS; ++j)
    {
        // CACHE1D_FREE
        voxlock[voxindex][j] = 1;
        voxoff[voxindex][j] = 0;
    }
    voxscale[voxindex] = 65536;
    tiletovox[tile] = -1;

    // TODO: nextvoxid
}

//
// inside
//
// See http://fabiensanglard.net/duke3d/build_engine_internals.php,
// "Inside details" for the idea behind the algorithm.
int32_t inside(int32_t x, int32_t y, int16_t sectnum)
{
    if (sectnum >= 0 && sectnum < numsectors)
    {
        uint32_t cnt1 = 0, cnt2 = 0;
        uwalltype const * wal = (uwalltype *) &wall[sector[sectnum].wallptr];
        int wallsleft = sector[sectnum].wallnum;

        do
        {
            // Get the x and y components of the [tested point]-->[wall
            // point{1,2}] vectors.
            vec2_t v1 = { wal->x - x, wal->y - y };
            vec2_t v2 = { wall[wal->point2].x - x, wall[wal->point2].y - y };

            // First, test if the point is EXACTLY_ON_WALL_POINT.
            if ((v1.x|v1.y) == 0 || (v2.x|v2.y)==0)
                return 1;

            // If their signs differ[*], ...
            //
            // [*] where '-' corresponds to <0 and '+' corresponds to >=0.
            // Equivalently, the branch is taken iff
            //   y1 != y2 AND y_m <= y < y_M,
            // where y_m := min(y1, y2) and y_M := max(y1, y2).
            if ((v1.y^v2.y) < 0)
                cnt1 ^= (((v1.x^v2.x) >= 0) ? v1.x : (v1.x*v2.y-v2.x*v1.y)^v2.y);

            v1.y--;
            v2.y--;

            // Now, do the same comparisons, but with the interval half-open on
            // the other side! That is, take the branch iff
            //   y1 != y2 AND y_m < y <= y_M,
            // For a rectangular sector, without EXACTLY_ON_WALL_POINT, this
            // would still leave the lower left and upper right points
            // "outside" the sector.
            if ((v1.y^v2.y) < 0)
            {
                v1.x--;
                v2.x--;

                cnt2 ^= (((v1.x^v2.x) >= 0) ? v1.x : (v1.x*v2.y-v2.x*v1.y)^v2.y);
            }

            wal++;
        }
        while (--wallsleft);

        return (cnt1|cnt2)>>31;
    }

    return -1;
}

int32_t LUNATIC_FASTCALL getangle(int32_t xvect, int32_t yvect)
{
    int32_t rv;

    if ((xvect | yvect) == 0)
        rv = 0;
    else if (xvect == 0)
        rv = 512 + ((yvect < 0) << 10);
    else if (yvect == 0)
        rv = ((xvect < 0) << 10);
    else if (xvect == yvect)
        rv = 256 + ((xvect < 0) << 10);
    else if (xvect == -yvect)
        rv = 768 + ((xvect > 0) << 10);
    else if (klabs(xvect) > klabs(yvect))
        rv = ((radarang[640 + scale(160, yvect, xvect)] >> 6) + ((xvect < 0) << 10)) & 2047;
    else rv = ((radarang[640 - scale(160, xvect, yvect)] >> 6) + 512 + ((yvect < 0) << 10)) & 2047;

    return rv;
}

//
// ksqrt
//
int32_t ksqrt(uint32_t num)
{
    return nsqrtasm(num);
}

#ifdef LUNATIC
int32_t Mulscale(int32_t a, int32_t b, int32_t sh)
{
    return mulscale(a, b, sh);
}
#endif

// Gets the BUILD unit height and z offset of a sprite.
// Returns the z offset, 'height' may be NULL.
int32_t spriteheightofsptr(const uspritetype *spr, int32_t *height, int32_t alsotileyofs)
{
    int32_t hei, zofs=0;
    const int32_t picnum=spr->picnum, yrepeat=spr->yrepeat;

    hei = (tilesiz[picnum].y*yrepeat)<<2;
    *height = hei;

    if (spr->cstat&128)
        zofs = hei>>1;

    // NOTE: a positive per-tile yoffset translates the sprite into the
    // negative world z direction (i.e. upward).
    if (alsotileyofs)
        zofs -= picanm[picnum].yofs*yrepeat<<2;

    return zofs;
}

//
// setsprite
//
int32_t setsprite(int16_t spritenum, const vec3_t *newpos)
{
    int16_t tempsectnum = sprite[spritenum].sectnum;

    if ((void const *) newpos != (void *) &sprite[spritenum])
        *(vec3_t *) &sprite[spritenum] = *newpos;

    updatesector(newpos->x,newpos->y,&tempsectnum);

    if (tempsectnum < 0)
        return -1;
    if (tempsectnum != sprite[spritenum].sectnum)
        changespritesect(spritenum,tempsectnum);

    return 0;
}

int32_t setspritez(int16_t spritenum, const vec3_t *newpos)
{
    int16_t tempsectnum = sprite[spritenum].sectnum;

    if ((void const *)newpos != (void *)&sprite[spritenum])
        *(vec3_t *) &sprite[spritenum] = *newpos;

    updatesectorz(newpos->x,newpos->y,newpos->z,&tempsectnum);

    if (tempsectnum < 0)
        return -1;
    if (tempsectnum != sprite[spritenum].sectnum)
        changespritesect(spritenum,tempsectnum);

    return 0;
}


//
// nextsectorneighborz
//
// -1: ceiling or up
//  1: floor or down
int32_t nextsectorneighborz(int16_t sectnum, int32_t refz, int16_t topbottom, int16_t direction)
{
    int32_t nextz = (direction==1) ? INT32_MAX : INT32_MIN;
    int32_t sectortouse = -1;

    const uwalltype *wal = (uwalltype *)&wall[sector[sectnum].wallptr];
    int32_t i = sector[sectnum].wallnum;

    do
    {
        const int32_t ns = wal->nextsector;

        if (ns >= 0)
        {
            const int32_t testz = (topbottom == 1) ?
                sector[ns].floorz : sector[ns].ceilingz;

            const int32_t update = (direction == 1) ?
                (nextz > testz && testz > refz) :
                (nextz < testz && testz < refz);

            if (update)
            {
                nextz = testz;
                sectortouse = ns;
            }
        }

        wal++;
        i--;
    }
    while (i != 0);

    return sectortouse;
}


//
// cansee
//
int32_t cansee(int32_t x1, int32_t y1, int32_t z1, int16_t sect1, int32_t x2, int32_t y2, int32_t z2, int16_t sect2)
{
    int32_t dacnt, danum;
    const int32_t x21 = x2-x1, y21 = y2-y1, z21 = z2-z1;

    static uint8_t sectbitmap[MAXSECTORS>>3];
#ifdef YAX_ENABLE
    int16_t pendingsectnum;
    vec3_t pendingvec;

    // Negative sectnums can happen, for example if the player is using noclip.
    // MAXSECTORS can happen from C-CON, e.g. canseespr with a sprite not in
    // the game world.
    if ((unsigned)sect1 >= MAXSECTORS || (unsigned)sect2 >= MAXSECTORS)
        return 0;

    Bmemset(&pendingvec, 0, sizeof(vec3_t));  // compiler-happy
#endif
    Bmemset(sectbitmap, 0, (numsectors+7)>>3);
#ifdef YAX_ENABLE
restart_grand:
#endif
    if (x1 == x2 && y1 == y2)
        return (sect1 == sect2);

#ifdef YAX_ENABLE
    pendingsectnum = -1;
#endif
    sectbitmap[sect1>>3] |= (1<<(sect1&7));
    clipsectorlist[0] = sect1; danum = 1;

    for (dacnt=0; dacnt<danum; dacnt++)
    {
        const int32_t dasectnum = clipsectorlist[dacnt];
        const usectortype *const sec = (usectortype *)&sector[dasectnum];
        const uwalltype *wal;
        bssize_t cnt;
#ifdef YAX_ENABLE
        int32_t cfz1[2], cfz2[2];  // both wrt dasectnum
        int16_t bn[2];

        yax_getbunches(dasectnum, &bn[0], &bn[1]);
        getzsofslope(dasectnum, x1,y1, &cfz1[0], &cfz1[1]);
        getzsofslope(dasectnum, x2,y2, &cfz2[0], &cfz2[1]);
#endif
        for (cnt=sec->wallnum,wal=(uwalltype *)&wall[sec->wallptr]; cnt>0; cnt--,wal++)
        {
            const uwalltype *const wal2 = (uwalltype *)&wall[wal->point2];
            const int32_t x31 = wal->x-x1, x34 = wal->x-wal2->x;
            const int32_t y31 = wal->y-y1, y34 = wal->y-wal2->y;

            int32_t x, y, z, nexts, t, bot;
            int32_t cfz[2];

            bot = y21*x34-x21*y34; if (bot <= 0) continue;
            // XXX: OVERFLOW
            t = y21*x31-x21*y31; if ((unsigned)t >= (unsigned)bot) continue;
            t = y31*x34-x31*y34;
            if ((unsigned)t >= (unsigned)bot)
            {
#ifdef YAX_ENABLE
                if (t >= bot)
                {
                    int32_t cf, frac, ns;
                    for (cf=0; cf<2; cf++)
                    {
                        if ((cf==0 && bn[0]>=0 && z1 > cfz1[0] && cfz2[0] > z2) ||
                            (cf==1 && bn[1]>=0 && z1 < cfz1[1] && cfz2[1] < z2))
                        {
                            if ((cfz1[cf]-cfz2[cf])-(z1-z2)==0)
                                continue;
                            frac = divscale24(z1-cfz1[cf], (z1-z2)-(cfz1[cf]-cfz2[cf]));

                            if ((unsigned)frac >= (1<<24))
                                continue;

                            x = x1 + mulscale24(x21,frac);
                            y = y1 + mulscale24(y21,frac);

                            ns = yax_getneighborsect(x, y, dasectnum, cf);
                            if (ns < 0)
                                continue;

                            if (!(sectbitmap[ns>>3] & (1<<(ns&7))) && pendingsectnum==-1)
                            {
                                sectbitmap[ns>>3] |= (1<<(ns&7));
                                pendingsectnum = ns;
                                pendingvec.x = x;
                                pendingvec.y = y;
                                pendingvec.z = z1 + mulscale24(z21,frac);
                            }
                        }
                    }
                }
#endif
                continue;
            }

            nexts = wal->nextsector;

#ifdef YAX_ENABLE
            if (bn[0]<0 && bn[1]<0)
#endif
                if (nexts < 0 || wal->cstat&32)
                    return 0;

            t = divscale24(t,bot);
            x = x1 + mulscale24(x21,t);
            y = y1 + mulscale24(y21,t);
            z = z1 + mulscale24(z21,t);

            getzsofslope(dasectnum, x,y, &cfz[0],&cfz[1]);

            if (z <= cfz[0] || z >= cfz[1])
            {
#ifdef YAX_ENABLE
                int32_t cf, frac;

                // XXX: Is this any good?
                for (cf=0; cf<2; cf++)
                    if ((cf==0 && bn[0]>=0 && z <= cfz[0] && z1 >= cfz1[0]) ||
                        (cf==1 && bn[1]>=0 && z >= cfz[1] && z1 <= cfz1[1]))
                    {
                        if ((cfz1[cf]-cfz[cf])-(z1-z)==0)
                            continue;

                        frac = divscale24(z1-cfz1[cf], (z1-z)-(cfz1[cf]-cfz[cf]));
                        t = mulscale24(t, frac);

                        if ((unsigned)t < (1<<24))
                        {
                            x = x1 + mulscale24(x21,t);
                            y = y1 + mulscale24(y21,t);

                            nexts = yax_getneighborsect(x, y, dasectnum, cf);
                            if (nexts >= 0)
                                goto add_nextsector;
                        }
                    }

#endif
                return 0;
            }

#ifdef YAX_ENABLE
            if (nexts < 0 || (wal->cstat&32))
                return 0;
#endif
            getzsofslope(nexts, x,y, &cfz[0],&cfz[1]);
            if (z <= cfz[0] || z >= cfz[1])
                return 0;

add_nextsector:
            if (!(sectbitmap[nexts>>3] & (1<<(nexts&7))))
            {
                sectbitmap[nexts>>3] |= (1<<(nexts&7));
                clipsectorlist[danum++] = nexts;
            }
        }

#ifdef YAX_ENABLE
        if (pendingsectnum>=0)
        {
            sect1 = pendingsectnum;
            x1 = pendingvec.x;
            y1 = pendingvec.y;
            z1 = pendingvec.z;
            goto restart_grand;
        }
#endif
    }

    if (sectbitmap[sect2>>3] & (1<<(sect2&7)))
        return 1;

    return 0;
}

static inline void hit_set(hitdata_t *hit, int32_t sectnum, int32_t wallnum, int32_t spritenum,
                           int32_t x, int32_t y, int32_t z)
{
    hit->sect = sectnum;
    hit->wall = wallnum;
    hit->sprite = spritenum;
    hit->pos.x = x;
    hit->pos.y = y;
    hit->pos.z = z;
}

static int32_t hitscan_hitsectcf=-1;

// stat, heinum, z: either ceiling- or floor-
// how: -1: behave like ceiling, 1: behave like floor
static int32_t hitscan_trysector(const vec3_t *sv, const usectortype *sec, hitdata_t *hit,
                                 int32_t vx, int32_t vy, int32_t vz,
                                 uint16_t stat, int16_t heinum, int32_t z, int32_t how, const intptr_t *tmp)
{
    int32_t x1 = INT32_MAX, y1, z1;
    int32_t i;

    if (stat&2)
    {
        const uwalltype *const wal = (uwalltype *)&wall[sec->wallptr];
        const uwalltype *const wal2 = (uwalltype *)&wall[wal->point2];
        int32_t j, dax=wal2->x-wal->x, day=wal2->y-wal->y;

        i = nsqrtasm(uhypsq(dax,day)); if (i == 0) return 1; //continue;
        i = divscale15(heinum,i);
        dax *= i; day *= i;

        j = (vz<<8)-dmulscale15(dax,vy,-day,vx);
        if (j != 0)
        {
            i = ((z - sv->z)<<8)+dmulscale15(dax,sv->y-wal->y,-day,sv->x-wal->x);
            if (((i^j) >= 0) && ((klabs(i)>>1) < klabs(j)))
            {
                i = divscale30(i,j);
                x1 = sv->x + mulscale30(vx,i);
                y1 = sv->y + mulscale30(vy,i);
                z1 = sv->z + mulscale30(vz,i);
            }
        }
    }
    else if ((how*vz > 0) && (how*sv->z <= how*z))
    {
        z1 = z; i = z1-sv->z;
        if ((klabs(i)>>1) < vz*how)
        {
            i = divscale30(i,vz);
            x1 = sv->x + mulscale30(vx,i);
            y1 = sv->y + mulscale30(vy,i);
        }
    }

    if ((x1 != INT32_MAX) && (klabs(x1-sv->x)+klabs(y1-sv->y) < klabs((hit->pos.x)-sv->x)+klabs((hit->pos.y)-sv->y)))
    {
        if (tmp==NULL)
        {
            if (inside(x1,y1,sec-(usectortype *)sector) == 1)
            {
                hit_set(hit, sec-(usectortype *)sector, -1, -1, x1, y1, z1);
                hitscan_hitsectcf = (how+1)>>1;
            }
        }
        else
        {
            const int32_t curidx=(int32_t)tmp[0];
            const uspritetype *const curspr=(uspritetype *)tmp[1];
            const int32_t thislastsec = tmp[2];

            if (!thislastsec)
            {
                if (inside(x1,y1,sec-(usectortype *)sector) == 1)
                    hit_set(hit, curspr->sectnum, -1, curspr-(uspritetype *)sprite, x1, y1, z1);
            }
#ifdef HAVE_CLIPSHAPE_FEATURE
            else
            {
                for (i=clipinfo[curidx].qbeg; i<clipinfo[curidx].qend; i++)
                {
                    if (inside(x1,y1,sectq[i]) == 1)
                    {
                        hit_set(hit, curspr->sectnum, -1, curspr-(uspritetype *)sprite, x1, y1, z1);
                        break;
                    }
                }
            }
#endif
        }
    }

    return 0;
}

// x1, y1: in/out
// rest x/y: out
void get_wallspr_points(uspritetype const * const spr, int32_t *x1, int32_t *x2,
                               int32_t *y1, int32_t *y2)
{
    //These lines get the 2 points of the rotated sprite
    //Given: (x1, y1) starts out as the center point

    const int32_t tilenum=spr->picnum, ang=spr->ang;
    const int32_t xrepeat = spr->xrepeat;
    int32_t xoff = picanm[tilenum].xofs + spr->xoffset;
    int32_t k, l, dax, day;

    if (spr->cstat&4)
        xoff = -xoff;

    dax = sintable[ang&2047]*xrepeat;
    day = sintable[(ang+1536)&2047]*xrepeat;

    l = tilesiz[tilenum].x;
    k = (l>>1)+xoff;

    *x1 -= mulscale16(dax,k);
    *x2 = *x1 + mulscale16(dax,l);

    *y1 -= mulscale16(day,k);
    *y2 = *y1 + mulscale16(day,l);
}

// x1, y1: in/out
// rest x/y: out
void get_floorspr_points(uspritetype const * const spr, int32_t px, int32_t py,
                                int32_t *x1, int32_t *x2, int32_t *x3, int32_t *x4,
                                int32_t *y1, int32_t *y2, int32_t *y3, int32_t *y4)
{
    const int32_t tilenum = spr->picnum;
    // &2047 in sinang:
    // DNE 1.3D lights camera action (1st level), spr->ang==2306
    // (probably from CON)
    const int32_t cosang = sintable[(spr->ang+512)&2047];
    const int32_t sinang = sintable[spr->ang&2047];

    const int32_t xspan=tilesiz[tilenum].x, xrepeat=spr->xrepeat;
    const int32_t yspan=tilesiz[tilenum].y, yrepeat=spr->yrepeat;

    int32_t xoff = picanm[tilenum].xofs + spr->xoffset;
    int32_t yoff = picanm[tilenum].yofs + spr->yoffset;
    int32_t k, l, dax, day;

    if (spr->cstat&4)
        xoff = -xoff;
    if (spr->cstat&8)
        yoff = -yoff;

    dax = ((xspan>>1)+xoff)*xrepeat;
    day = ((yspan>>1)+yoff)*yrepeat;

    *x1 += dmulscale16(sinang,dax, cosang,day) - px;
    *y1 += dmulscale16(sinang,day, -cosang,dax) - py;

    l = xspan*xrepeat;
    *x2 = *x1 - mulscale16(sinang,l);
    *y2 = *y1 + mulscale16(cosang,l);

    l = yspan*yrepeat;
    k = -mulscale16(cosang,l); *x3 = *x2+k; *x4 = *x1+k;
    k = -mulscale16(sinang,l); *y3 = *y2+k; *y4 = *y1+k;
}

static int32_t get_floorspr_clipyou(int32_t x1, int32_t x2, int32_t x3, int32_t x4,
                                   int32_t y1, int32_t y2, int32_t y3, int32_t y4)
{
    int32_t clipyou = 0;

    if ((y1^y2) < 0)
    {
        if ((x1^x2) < 0) clipyou ^= (x1*y2 < x2*y1)^(y1<y2);
        else if (x1 >= 0) clipyou ^= 1;
    }
    if ((y2^y3) < 0)
    {
        if ((x2^x3) < 0) clipyou ^= (x2*y3 < x3*y2)^(y2<y3);
        else if (x2 >= 0) clipyou ^= 1;
    }
    if ((y3^y4) < 0)
    {
        if ((x3^x4) < 0) clipyou ^= (x3*y4 < x4*y3)^(y3<y4);
        else if (x3 >= 0) clipyou ^= 1;
    }
    if ((y4^y1) < 0)
    {
        if ((x4^x1) < 0) clipyou ^= (x4*y1 < x1*y4)^(y4<y1);
        else if (x4 >= 0) clipyou ^= 1;
    }

    return clipyou;
}

// intp: point of currently best (closest) intersection
static int32_t try_facespr_intersect(uspritetype const * const spr, const vec3_t *refpos,
                                     int32_t vx, int32_t vy, int32_t vz,
                                     vec3_t *intp, int32_t strictly_smaller_than_p)
{
    const int32_t x1=spr->x, y1=spr->y;
    const int32_t xs=refpos->x, ys=refpos->y;

    const int32_t topt = vx*(x1-xs) + vy*(y1-ys);
    if (topt > 0)
    {
        const int32_t bot = vx*vx + vy*vy;
        if (bot != 0)
        {
            int32_t i;
            const int32_t intz = refpos->z + scale(vz,topt,bot);
            const int32_t z1 = spr->z + spriteheightofsptr(spr, &i, 1);

            if (intz >= z1-i && intz <= z1)
            {
                const int32_t topu = vx*(y1-ys) - vy*(x1-xs);

                const int32_t offx = scale(vx,topu,bot);
                const int32_t offy = scale(vy,topu,bot);
                const int32_t dist = offx*offx + offy*offy;

                i = tilesiz[spr->picnum].x*spr->xrepeat;
                if (dist <= mulscale7(i,i))
                {
                    const int32_t intx = xs + scale(vx,topt,bot);
                    const int32_t inty = ys + scale(vy,topt,bot);

                    if (klabs(intx-xs)+klabs(inty-ys) + strictly_smaller_than_p
                            <= klabs(intp->x-xs)+klabs(intp->y-ys))
                    {
                        intp->x = intx;
                        intp->y = inty;
                        intp->z = intz;
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

//
// hitscan
//
int32_t hitscan(const vec3_t *sv, int16_t sectnum, int32_t vx, int32_t vy, int32_t vz,
                hitdata_t *hit, uint32_t cliptype)
{
    int32_t x1, y1=0, z1=0, x2, y2, intx, inty, intz;
    int32_t i, k, daz;
    int16_t tempshortcnt, tempshortnum;

    uspritetype *curspr = NULL;
    int32_t clipspritecnt, curidx=-1;
    // tmp: { (int32_t)curidx, (spritetype *)curspr, (!=0 if outer sector) }
    intptr_t tmp[3], *tmpptr=NULL;
#ifdef YAX_ENABLE
    vec3_t newsv;
    int32_t oldhitsect = -1, oldhitsect2 = -2;
#endif
    const int32_t dawalclipmask = (cliptype&65535);
    const int32_t dasprclipmask = (cliptype>>16);

    hit->sect = -1; hit->wall = -1; hit->sprite = -1;
    if (sectnum < 0)
        return -1;

#ifdef YAX_ENABLE
restart_grand:
#endif
    *(vec2_t *)&hit->pos = hitscangoal;

    clipsectorlist[0] = sectnum;
    tempshortcnt = 0; tempshortnum = 1;
    clipspritecnt = clipspritenum = 0;
    do
    {
        const usectortype *sec;
        const uwalltype *wal;
        int32_t dasector, z, startwall, endwall;

#ifdef HAVE_CLIPSHAPE_FEATURE
        if (tempshortcnt >= tempshortnum)
        {
            // one bunch of sectors completed, prepare the next
            if (!curspr)
                mapinfo_set(&origmapinfo, &clipmapinfo);  // replace sector and wall with clip map

            curspr = (uspritetype *)&sprite[clipspritelist[clipspritecnt]];
            curidx = clipshape_idx_for_sprite(curspr, curidx);

            if (curidx < 0)
            {
                clipspritecnt++;
                continue;
            }

            tmp[0] = (intptr_t)curidx;
            tmp[1] = (intptr_t)curspr;
            tmpptr = tmp;

            clipsprite_initindex(curidx, curspr, &i, sv);  // &i is dummy
            tempshortnum = (int16_t)clipsectnum;
            tempshortcnt = 0;
        }
#endif
        dasector = clipsectorlist[tempshortcnt]; sec = (usectortype *)&sector[dasector];

        i = 1;
#ifdef HAVE_CLIPSHAPE_FEATURE
        if (curspr)
        {
            if (dasector == sectq[clipinfo[curidx].qend])
            {
                i = -1;
                tmp[2] = 1;
            }
            else tmp[2] = 0;
        }
#endif
        if (hitscan_trysector(sv, sec, hit, vx,vy,vz, sec->ceilingstat, sec->ceilingheinum, sec->ceilingz, -i, tmpptr))
            continue;
        if (hitscan_trysector(sv, sec, hit, vx,vy,vz, sec->floorstat, sec->floorheinum, sec->floorz, i, tmpptr))
            continue;

        ////////// Walls //////////

        startwall = sec->wallptr; endwall = startwall + sec->wallnum;
        for (z=startwall,wal=(uwalltype *)&wall[startwall]; z<endwall; z++,wal++)
        {
            const int32_t nextsector = wal->nextsector;
            const uwalltype *const wal2 = (uwalltype *)&wall[wal->point2];
            int32_t daz2, zz;

            if (curspr && nextsector<0) continue;

            x1 = wal->x; y1 = wal->y; x2 = wal2->x; y2 = wal2->y;

            if ((coord_t)(x1-sv->x)*(y2-sv->y) < (coord_t)(x2-sv->x)*(y1-sv->y)) continue;
            if (rintersect(sv->x,sv->y,sv->z, vx,vy,vz, x1,y1, x2,y2, &intx,&inty,&intz) == -1) continue;

            if (klabs(intx-sv->x)+klabs(inty-sv->y) >= klabs((hit->pos.x)-sv->x)+klabs((hit->pos.y)-sv->y))
                continue;

            if (!curspr)
            {
                if ((nextsector < 0) || (wal->cstat&dawalclipmask))
                {
                    hit_set(hit, dasector, z, -1, intx, inty, intz);
                    continue;
                }

                getzsofslope(nextsector,intx,inty,&daz,&daz2);
                if (intz <= daz || intz >= daz2)
                {
                    hit_set(hit, dasector, z, -1, intx, inty, intz);
                    continue;
                }
            }
#ifdef HAVE_CLIPSHAPE_FEATURE
            else
            {
                int32_t cz,fz;

                if (wal->cstat&dawalclipmask)
                {
                    hit_set(hit, curspr->sectnum, -1, curspr-(uspritetype *)sprite, intx, inty, intz);
                    continue;
                }

                getzsofslope(nextsector,intx,inty,&daz,&daz2);
                getzsofslope(sectq[clipinfo[curidx].qend],intx,inty,&cz,&fz);
                // ceil   cz daz daz2 fz   floor
                if ((cz <= intz && intz <= daz) || (daz2 <= intz && intz <= fz))
                {
                    hit_set(hit, curspr->sectnum, -1, curspr-(uspritetype *)sprite, intx, inty, intz);
                    continue;
                }
            }
#endif
            for (zz=tempshortnum-1; zz>=0; zz--)
                if (clipsectorlist[zz] == nextsector) break;
            if (zz < 0) clipsectorlist[tempshortnum++] = nextsector;
        }

        ////////// Sprites //////////

        if (dasprclipmask==0)
            continue;

#ifdef HAVE_CLIPSHAPE_FEATURE
        if (curspr)
            continue;
#endif
        for (z=headspritesect[dasector]; z>=0; z=nextspritesect[z])
        {
            const uspritetype *const spr = (uspritetype *)&sprite[z];
            const int32_t cstat = spr->cstat;
#ifdef USE_OPENGL
            if (!hitallsprites)
#endif
                if ((cstat&dasprclipmask) == 0)
                    continue;

#ifdef HAVE_CLIPSHAPE_FEATURE
            // try and see whether this sprite's picnum has sector-like clipping data
            i = pictoidx[spr->picnum];
            // handle sector-like floor sprites separately
            while (i>=0 && (spr->cstat&32) != (clipmapinfo.sector[sectq[clipinfo[i].qbeg]].CM_CSTAT&32))
                i = clipinfo[i].next;
            if (i>=0 && clipspritenum<MAXCLIPNUM)
            {
                clipspritelist[clipspritenum++] = z;
                continue;
            }
#endif
            x1 = spr->x; y1 = spr->y; z1 = spr->z;
            switch (cstat&48)
            {
            case 0:
            {
                if (try_facespr_intersect(spr, sv, vx, vy, vz, &hit->pos, 0))
                {
                    hit->sect = dasector;
                    hit->wall = -1;
                    hit->sprite = z;
                }

                break;
            }

            case 16:
            {
                int32_t ucoefup16;
                int32_t tilenum = spr->picnum;

                get_wallspr_points(spr, &x1, &x2, &y1, &y2);

                if ((cstat&64) != 0)   //back side of 1-way sprite
                    if ((coord_t)(x1-sv->x)*(y2-sv->y) < (coord_t)(x2-sv->x)*(y1-sv->y)) continue;

                ucoefup16 = rintersect(sv->x,sv->y,sv->z,vx,vy,vz,x1,y1,x2,y2,&intx,&inty,&intz);
                if (ucoefup16 == -1) continue;

                if (klabs(intx-sv->x)+klabs(inty-sv->y) > klabs((hit->pos.x)-sv->x)+klabs((hit->pos.y)-sv->y))
                    continue;

                daz = spr->z + spriteheightofs(z, &k, 1);
                if (intz > daz-k && intz < daz)
                {
                    if (picanm[tilenum].sf&PICANM_TEXHITSCAN_BIT)
                    {
                        DO_TILE_ANIM(tilenum, 0);

                        if (!waloff[tilenum])
                            loadtile(tilenum);

                        if (waloff[tilenum])
                        {
                            // daz-intz > 0 && daz-intz < k
                            int32_t xtex = mulscale16(ucoefup16, tilesiz[tilenum].x);
                            int32_t vcoefup16 = 65536-divscale16(daz-intz, k);
                            int32_t ytex = mulscale16(vcoefup16, tilesiz[tilenum].y);

                            const char *texel = (char *)(waloff[tilenum] + tilesiz[tilenum].y*xtex + ytex);
                            if (*texel == 255)
                                continue;
                        }
                    }

                    hit_set(hit, dasector, -1, z, intx, inty, intz);
                }
                break;
            }

            case 32:
            {
                int32_t x3, y3, x4, y4, zz;

                if (vz == 0) continue;
                intz = z1;
                if (((intz-sv->z)^vz) < 0) continue;
                if ((cstat&64) != 0)
                    if ((sv->z > intz) == ((cstat&8)==0)) continue;
#if 1
                // Abyss crash prevention code ((intz-sv->z)*zx overflowing a 8-bit word)
                // PK: the reason for the crash is not the overflowing (even if it IS a problem;
                // signed overflow is undefined behavior in C), but rather the idiv trap when
                // the resulting quotient doesn't fit into a *signed* 32-bit integer.
                zz = (uint32_t)(intz-sv->z) * vx;
                intx = sv->x+scale(zz,1,vz);
                zz = (uint32_t)(intz-sv->z) * vy;
                inty = sv->y+scale(zz,1,vz);
#else
                intx = sv->x+scale(intz-sv->z,vx,vz);
                inty = sv->y+scale(intz-sv->z,vy,vz);
#endif
                if (klabs(intx-sv->x)+klabs(inty-sv->y) > klabs((hit->pos.x)-sv->x)+klabs((hit->pos.y)-sv->y))
                    continue;

                get_floorspr_points((uspritetype const *)spr, intx, inty, &x1, &x2, &x3, &x4,
                                    &y1, &y2, &y3, &y4);

                if (get_floorspr_clipyou(x1, x2, x3, x4, y1, y2, y3, y4))
                {
                    hit_set(hit, dasector, -1, z, intx, inty, intz);
                }

                break;
            }
            }
        }
    }
    while (++tempshortcnt < tempshortnum || clipspritecnt < clipspritenum);

#ifdef HAVE_CLIPSHAPE_FEATURE
    if (curspr)
        mapinfo_set(NULL, &origmapinfo);
#endif

#ifdef YAX_ENABLE
    if (numyaxbunches == 0 || editstatus)
        return 0;

    if (hit->sprite==-1 && hit->wall==-1 && hit->sect!=oldhitsect
        && hit->sect != oldhitsect2)  // 'ping-pong' infloop protection
    {
        if (hit->sect == -1 && oldhitsect >= 0)
        {
            // this is bad: we didn't hit anything after going through a ceiling/floor
            Bmemcpy(&hit->pos, &newsv, sizeof(vec3_t));
            hit->sect = oldhitsect;

            return 0;
        }

        // 1st, 2nd, ... ceil/floor hit
        // hit->sect is >=0 because if oldhitsect's init and check above
        if (SECTORFLD(hit->sect,stat, hitscan_hitsectcf)&yax_waltosecmask(dawalclipmask))
            return 0;

        i = yax_getneighborsect(hit->pos.x, hit->pos.y, hit->sect, hitscan_hitsectcf);
        if (i >= 0)
        {
            Bmemcpy(&newsv, &hit->pos, sizeof(vec3_t));
            sectnum = i;
            sv = &newsv;

            oldhitsect2 = oldhitsect;
            oldhitsect = hit->sect;
            hit->sect = -1;

            // sector-like sprite re-init:
            curspr = 0;
            curidx = -1;
            tmpptr = NULL;

            goto restart_grand;
        }
    }
#endif

    return 0;
}


//
// neartag
//
void neartag(int32_t xs, int32_t ys, int32_t zs, int16_t sectnum, int16_t ange,
             int16_t *neartagsector, int16_t *neartagwall, int16_t *neartagsprite, int32_t *neartaghitdist,  /* out */
             int32_t neartagrange, uint8_t tagsearch,
             int32_t (*blacklist_sprite_func)(int32_t))
{
    int16_t tempshortcnt, tempshortnum;

    const int32_t vx = mulscale14(sintable[(ange+2560)&2047],neartagrange);
    const int32_t vy = mulscale14(sintable[(ange+2048)&2047],neartagrange);
    vec3_t hitv = { xs+vx, ys+vy, 0 };
    const vec3_t sv = { xs, ys, zs };

    *neartagsector = -1; *neartagwall = -1; *neartagsprite = -1;
    *neartaghitdist = 0;

    if (sectnum < 0 || (tagsearch & 3) == 0)
        return;

    clipsectorlist[0] = sectnum;
    tempshortcnt = 0; tempshortnum = 1;

    do
    {
        const int32_t dasector = clipsectorlist[tempshortcnt];

        const int32_t startwall = sector[dasector].wallptr;
        const int32_t endwall = startwall + sector[dasector].wallnum - 1;
        const uwalltype *wal;
        int32_t z;

        for (z=startwall,wal=(uwalltype *)&wall[startwall]; z<=endwall; z++,wal++)
        {
            const uwalltype *const wal2 = (uwalltype *)&wall[wal->point2];
            const int32_t nextsector = wal->nextsector;

            const int32_t x1=wal->x, y1=wal->y, x2=wal2->x, y2=wal2->y;
            int32_t intx, inty, intz, good = 0;

            if (nextsector >= 0)
            {
                if ((tagsearch&1) && sector[nextsector].lotag) good |= 1;
                if ((tagsearch&2) && sector[nextsector].hitag) good |= 1;
            }

            if ((tagsearch&1) && wal->lotag) good |= 2;
            if ((tagsearch&2) && wal->hitag) good |= 2;

            if ((good == 0) && (nextsector < 0)) continue;
            if ((coord_t)(x1-xs)*(y2-ys) < (coord_t)(x2-xs)*(y1-ys)) continue;

            if (lintersect(xs,ys,zs,hitv.x,hitv.y,hitv.z,x1,y1,x2,y2,&intx,&inty,&intz) == 1)
            {
                if (good != 0)
                {
                    if (good&1) *neartagsector = nextsector;
                    if (good&2) *neartagwall = z;
                    *neartaghitdist = dmulscale14(intx-xs,sintable[(ange+2560)&2047],inty-ys,sintable[(ange+2048)&2047]);
                    hitv.x = intx; hitv.y = inty; hitv.z = intz;
                }

                if (nextsector >= 0)
                {
                    int32_t zz;
                    for (zz=tempshortnum-1; zz>=0; zz--)
                        if (clipsectorlist[zz] == nextsector) break;
                    if (zz < 0) clipsectorlist[tempshortnum++] = nextsector;
                }
            }
        }

        tempshortcnt++;

        if (tagsearch & 4)
            continue; // skip sprite search

        for (z=headspritesect[dasector]; z>=0; z=nextspritesect[z])
        {
            const uspritetype *const spr = (uspritetype *)&sprite[z];

            if (blacklist_sprite_func && blacklist_sprite_func(z))
                continue;

            if (((tagsearch&1) && spr->lotag) || ((tagsearch&2) && spr->hitag))
            {
                if (try_facespr_intersect(spr, &sv, vx, vy, 0, &hitv, 1))
                {
                    *neartagsprite = z;
                    *neartaghitdist = dmulscale14(hitv.x-xs, sintable[(ange+2560)&2047],
                                                  hitv.y-ys, sintable[(ange+2048)&2047]);
                }
            }
        }
    }
    while (tempshortcnt < tempshortnum);

    return;
}


//
// dragpoint
//
// flags:
//  1: don't reset walbitmap[] (the bitmap of already dragged vertices)
//  2: In the editor, do wall[].cstat |= (1<<14) also for the lastwall().
void dragpoint(int16_t pointhighlight, int32_t dax, int32_t day, uint8_t flags)
#ifdef YAX_ENABLE
{
    int32_t i, numyaxwalls=0;
    static int16_t yaxwalls[MAXWALLS];

    uint8_t *const walbitmap = (uint8_t *)tempbuf;

    if ((flags&1)==0)
        Bmemset(walbitmap, 0, (numwalls+7)>>3);
    yaxwalls[numyaxwalls++] = pointhighlight;

    for (i=0; i<numyaxwalls; i++)
    {
        int32_t clockwise = 0;
        int32_t w = yaxwalls[i];
        const int32_t tmpstartwall = w;

        bssize_t cnt = MAXWALLS;

        while (1)
        {
            int32_t j, tmpcf;

            wall[w].x = dax;
            wall[w].y = day;
            walbitmap[w>>3] |= (1<<(w&7));

            for (YAX_ITER_WALLS(w, j, tmpcf))
            {
                if ((walbitmap[j>>3]&(1<<(j&7)))==0)
                {
                    walbitmap[j>>3] |= (1<<(j&7));
                    yaxwalls[numyaxwalls++] = j;
                }
            }

            if (!clockwise)  //search points CCW
            {
                if (wall[w].nextwall >= 0)
                    w = wall[wall[w].nextwall].point2;
                else
                {
                    w = tmpstartwall;
                    clockwise = 1;
                }
            }

            cnt--;
            if (cnt==0)
            {
                initprintf("dragpoint %d: infloop!\n", pointhighlight);
                i = numyaxwalls;
                break;
            }

            if (clockwise)
            {
                int32_t thelastwall = lastwall(w);
                if (wall[thelastwall].nextwall >= 0)
                    w = wall[thelastwall].nextwall;
                else
                    break;
            }

            if ((walbitmap[w>>3] & (1<<(w&7))))
            {
                if (clockwise)
                    break;

                w = tmpstartwall;
                clockwise = 1;
                continue;
            }
        }
    }

    if (editstatus)
    {
        int32_t w;
        // TODO: extern a separate bitmap instead?
        for (w=0; w<numwalls; w++)
            if (walbitmap[w>>3] & (1<<(w&7)))
            {
                wall[w].cstat |= (1<<14);
                if (flags&2)
                    wall[lastwall(w)].cstat |= (1<<14);
            }
    }
}
#else
{
    int16_t cnt, tempshort;
    int32_t thelastwall;

    tempshort = pointhighlight;    //search points CCW
    cnt = MAXWALLS;

    wall[tempshort].x = dax;
    wall[tempshort].y = day;

    if (editstatus)
    {
        wall[pointhighlight].cstat |= (1<<14);
        if (linehighlight >= 0 && linehighlight < MAXWALLS)
            wall[linehighlight].cstat |= (1<<14);
        wall[lastwall(pointhighlight)].cstat |= (1<<14);
    }

    do
    {
        if (wall[tempshort].nextwall >= 0)
        {
            tempshort = wall[wall[tempshort].nextwall].point2;

            wall[tempshort].x = dax;
            wall[tempshort].y = day;
            wall[tempshort].cstat |= (1<<14);
        }
        else
        {
            tempshort = pointhighlight;    //search points CW if not searched all the way around
            do
            {
                thelastwall = lastwall(tempshort);
                if (wall[thelastwall].nextwall >= 0)
                {
                    tempshort = wall[thelastwall].nextwall;
                    wall[tempshort].x = dax;
                    wall[tempshort].y = day;
                    wall[tempshort].cstat |= (1<<14);
                }
                else
                {
                    break;
                }
                cnt--;
            }
            while ((tempshort != pointhighlight) && (cnt > 0));
            break;
        }
        cnt--;
    }
    while ((tempshort != pointhighlight) && (cnt > 0));
}
#endif

//
// lastwall
//
int32_t lastwall(int16_t point)
{
    if (point > 0 && wall[point-1].point2 == point)
        return point-1;

    int i = point, cnt = MAXWALLS;
    do
    {
        int const j = wall[i].point2;

        if (j == point)
        {
            point = i;
            break;
        }

        i = j;
    }
    while (--cnt);

    return point;
}

// breadth-first search helpers
void bfirst_search_init(int16_t *list, uint8_t *bitmap, int32_t *eltnumptr, int32_t maxnum, int16_t firstelt)
{
    Bmemset(bitmap, 0, (maxnum+7)>>3);

    list[0] = firstelt;
    bitmap[firstelt>>3] |= (1<<(firstelt&7));
    *eltnumptr = 1;
}

void bfirst_search_try(int16_t *list, uint8_t *bitmap, int32_t *eltnumptr, int16_t elt)
{
    if (elt < 0)
        return;

    if ((bitmap[elt>>3]&(1<<(elt&7)))==0)
    {
        bitmap[elt>>3] |= (1<<(elt&7));
        list[*eltnumptr] = elt;
        (*eltnumptr)++;
    }
}

////////// UPDATESECTOR* FAMILY OF FUNCTIONS //////////

/* Different "is inside" predicates.
 * NOTE: The redundant bound checks are expected to be optimized away in the
 * inlined code. */
static inline int32_t inside_p(int32_t x, int32_t y, int16_t sectnum)
{
    return (sectnum>=0 && inside(x, y, sectnum) == 1);
}

static inline int32_t inside_exclude_p(int32_t x, int32_t y, int16_t i, const uint8_t *excludesectbitmap)
{
    return (i>=0 && !(excludesectbitmap[i>>3]&(1<<(i&7))) && inside_p(x, y, i));
}

/* NOTE: no bound check */
static inline int32_t inside_z_p(int32_t x, int32_t y, int32_t z, int16_t i)
{
    int32_t cz, fz;
    getzsofslope(i, x, y, &cz, &fz);
    return (z >= cz && z <= fz && inside_p(x, y, i));
}

#define SET_AND_RETURN(Lval, Rval) do \
{ \
    (Lval) = (Rval); \
    return; \
} while (0)


//
// updatesector[z]
//
void updatesector(int32_t x, int32_t y, int16_t *sectnum)
{
    if (inside_p(x,y,*sectnum))
        return;

    if ((unsigned)*sectnum < (unsigned)numsectors)
    {
        const uwalltype *wal = (uwalltype *)&wall[sector[*sectnum].wallptr];
        int wallsleft = sector[*sectnum].wallnum;

        do
        {
            int const next = wal->nextsector;
            if (inside_p(x, y, next))
                SET_AND_RETURN(*sectnum, next);
            wal++;
        }
        while (--wallsleft);
    }

    for (bssize_t i=numsectors-1; i>=0; --i)
        if (inside_p(x, y, i))
            SET_AND_RETURN(*sectnum, i);

    *sectnum = -1;
}

void updatesectorbreadth(int32_t x, int32_t y, int16_t *sectnum)
{
    static int16_t sectlist[MAXSECTORS];
    static uint8_t sectbitmap[MAXSECTORS>>3];
    int32_t nsecs, sectcnt, j;

    if ((unsigned)(*sectnum) >= (unsigned)numsectors)
        return;

    bfirst_search_init(sectlist, sectbitmap, &nsecs, numsectors, *sectnum);

    for (sectcnt=0; sectcnt<nsecs; sectcnt++)
    {
        if (inside_p(x,y, sectlist[sectcnt]))
            SET_AND_RETURN(*sectnum, sectlist[sectcnt]);

        {
            const sectortype *sec = &sector[sectlist[sectcnt]];
            int32_t startwall = sec->wallptr;
            int32_t endwall = sec->wallptr + sec->wallnum;

            for (j=startwall; j<endwall; j++)
                if (wall[j].nextsector >= 0)
                    bfirst_search_try(sectlist, sectbitmap, &nsecs, wall[j].nextsector);
        }
    }

    *sectnum = -1;
}

void updatesectorexclude(int32_t x, int32_t y, int16_t *sectnum, const uint8_t *excludesectbitmap)
{
    if (inside_exclude_p(x, y, *sectnum, excludesectbitmap))
        return;

    if (*sectnum >= 0 && *sectnum < numsectors)
    {
        const uwalltype *wal = (uwalltype *)&wall[sector[*sectnum].wallptr];
        int wallsleft = sector[*sectnum].wallnum;

        do
        {
            int const next = wal->nextsector;
            if (inside_exclude_p(x, y, next, excludesectbitmap))
                SET_AND_RETURN(*sectnum, next);
            wal++;
        }
        while (--wallsleft);
    }

    for (bssize_t i=numsectors-1; i>=0; --i)
        if (inside_exclude_p(x, y, i, excludesectbitmap))
            SET_AND_RETURN(*sectnum, i);

    *sectnum = -1;
}

// new: if *sectnum >= MAXSECTORS, *sectnum-=MAXSECTORS is considered instead
//      as starting sector and the 'initial' z check is skipped
//      (not initial anymore because it follows the sector updating due to TROR)
void updatesectorz(int32_t x, int32_t y, int32_t z, int16_t *sectnum)
{
    if ((uint32_t)(*sectnum) < 2*MAXSECTORS)
    {
        int32_t nofirstzcheck = 0;

        if (*sectnum >= MAXSECTORS)
        {
            *sectnum -= MAXSECTORS;
            nofirstzcheck = 1;
        }

        // this block used to be outside the "if" and caused crashes in Polymost Mapster32
        int32_t cz, fz;
        getzsofslope(*sectnum, x, y, &cz, &fz);

#ifdef YAX_ENABLE
        if (z < cz)
        {
            int const next = yax_getneighborsect(x, y, *sectnum, YAX_CEILING);
            if (next >= 0 && z >= getceilzofslope(next, x, y))
                SET_AND_RETURN(*sectnum, next);
        }

        if (z > fz)
        {
            int const next = yax_getneighborsect(x, y, *sectnum, YAX_FLOOR);
            if (next >= 0 && z <= getflorzofslope(next, x, y))
                SET_AND_RETURN(*sectnum, next);
        }
#endif
        if (nofirstzcheck || (z >= cz && z <= fz))
            if (inside_p(x, y, *sectnum))
                return;

        uwalltype const * wal = (uwalltype *)&wall[sector[*sectnum].wallptr];
        int wallsleft = sector[*sectnum].wallnum;
        do
        {
            // YAX: TODO: check neighboring sectors here too?
            int const next = wal->nextsector;
            if (next>=0 && inside_z_p(x,y,z, next))
                SET_AND_RETURN(*sectnum, next);

            wal++;
        }
        while (--wallsleft);
    }

    for (bssize_t i=numsectors-1; i>=0; --i)
        if (inside_z_p(x,y,z, i))
            SET_AND_RETURN(*sectnum, i);

    *sectnum = -1;
}


//
// rotatepoint
//
void rotatepoint(vec2_t const pivot, vec2_t p, int16_t daang, vec2_t *p2)
{
    int const dacos = sintable[(daang+2560)&2047];
    int const dasin = sintable[(daang+2048)&2047];
    p.x -= pivot.x;
    p.y -= pivot.y;
    p2->x = dmulscale14(p.x, dacos, -p.y, dasin) + pivot.x;
    p2->y = dmulscale14(p.y, dacos, p.x, dasin) + pivot.y;
}


//
// getmousevalues
//

void getmousevalues(int32_t *mousx, int32_t *mousy, int32_t *bstatus)
{
    readmousexy(mousx,mousy);
    readmousebstatus(bstatus);
}


#if KRANDDEBUG
# include <execinfo.h>
# define KRD_MAXCALLS 262144
# define KRD_DEPTH 8
static int32_t krd_numcalls=0;
static void *krd_fromwhere[KRD_MAXCALLS][KRD_DEPTH];
static int32_t krd_enabled=0;

void krd_enable(int which)  // 0: disable, 1: rec, 2: play
{
    krd_enabled = which;

    if (which)
        Bmemset(krd_fromwhere, 0, sizeof(krd_fromwhere));
}

int32_t krd_print(const char *filename)
{
    FILE *fp;
    int32_t i, j;

    if (!krd_enabled) return 1;
    krd_enabled = 0;

    fp = fopen(filename, "wb");
    if (!fp) { OSD_Printf("krd_print (2): fopen"); return 1; }

    for (i=0; i<krd_numcalls; i++)
    {
        for (j=1;; j++)  // skip self entry
        {
            if (j>=KRD_DEPTH || krd_fromwhere[i][j]==NULL)
            {
                fprintf(fp, "\n");
                break;
            }
            fprintf(fp, " [%p]", krd_fromwhere[i][j]);
        }
    }

    krd_numcalls = 0;

    fclose(fp);
    return 0;
}
#endif  // KRANDDEBUG

#if KRANDDEBUG || defined LUNATIC
//
// krand
//
int32_t krand(void)
{
//    randomseed = (randomseed*27584621)+1;
    randomseed = (randomseed * 1664525ul) + 221297ul;
#ifdef KRANDDEBUG
    if (krd_enabled)
        if (krd_numcalls < KRD_MAXCALLS)
        {
            backtrace(krd_fromwhere[krd_numcalls], KRD_DEPTH);
            krd_numcalls++;
        }
#endif
    return ((uint32_t)randomseed)>>16;
}
#endif

//
// getzrange
//
void getzrange(const vec3_t *pos, int16_t sectnum,
               int32_t *ceilz, int32_t *ceilhit, int32_t *florz, int32_t *florhit,
               int32_t walldist, uint32_t cliptype)
{
    if (sectnum < 0)
    {
        *ceilz = INT32_MIN; *ceilhit = -1;
        *florz = INT32_MAX; *florhit = -1;
        return;
    }

    int32_t clipsectcnt = 0;

#ifdef YAX_ENABLE
    // YAX round, -1:center, 0:ceiling, 1:floor
    int32_t mcf=-1;
#endif

    uspritetype *curspr=NULL;  // non-NULL when handling sprite with sector-like clipping
    int32_t curidx=-1, clipspritecnt = 0;

    //Extra walldist for sprites on sector lines
    const int32_t extradist = walldist+MAXCLIPDIST+1;
    const int32_t xmin = pos->x-extradist, ymin = pos->y-extradist;
    const int32_t xmax = pos->x+extradist, ymax = pos->y+extradist;

    const int32_t dawalclipmask = (cliptype&65535);
    const int32_t dasprclipmask = (cliptype>>16);

    getzsofslope(sectnum,pos->x,pos->y,ceilz,florz);
    *ceilhit = sectnum+16384; *florhit = sectnum+16384;

#ifdef YAX_ENABLE
    origclipsectorlist[0] = sectnum;
    origclipsectnum = 1;
#endif
    clipsectorlist[0] = sectnum;
    clipsectnum = 1;
    clipspritenum = 0;

#ifdef HAVE_CLIPSHAPE_FEATURE
    if (0)
    {
beginagain:
        // replace sector and wall with clip map
        mapinfo_set(&origmapinfo, &clipmapinfo);
        clipsectcnt = clipsectnum;  // should be a nop, "safety"...
    }
#endif

#ifdef YAX_ENABLE
restart_grand:
#endif
    do  //Collect sectors inside your square first
    {
#ifdef HAVE_CLIPSHAPE_FEATURE
        if (clipsectcnt>=clipsectnum)
        {
            // one set of clip-sprite sectors completed, prepare the next

            curspr = (uspritetype *)&sprite[clipspritelist[clipspritecnt]];
            curidx = clipshape_idx_for_sprite(curspr, curidx);

            if (curidx < 0)
            {
                // didn't find matching clipping sectors for sprite
                clipspritecnt++;
                continue;
            }

            clipsprite_initindex(curidx, curspr, &clipsectcnt, pos);

            for (bssize_t i=0; i<clipsectnum; i++)
            {
                int const k = clipsectorlist[i];

                if (k==sectq[clipinfo[curidx].qend])
                    continue;

                int32_t daz, daz2;
                getzsofslope(k,pos->x,pos->y,&daz,&daz2);
                int32_t fz, cz;
                getzsofslope(sectq[clipinfo[curidx].qend],pos->x,pos->y,&cz,&fz);
                const int hitwhat = (curspr-(uspritetype *)sprite)+49152;

                if ((sector[k].ceilingstat&1)==0)
                {
                    if (pos->z < cz && cz < *florz) { *florz = cz; *florhit = hitwhat; }
                    if (pos->z > daz && daz > *ceilz) { *ceilz = daz; *ceilhit = hitwhat; }
                }
                if ((sector[k].floorstat&1)==0)
                {
                    if (pos->z < daz2 && daz2 < *florz) { *florz = daz2; *florhit = hitwhat; }
                    if (pos->z > fz && fz > *ceilz) { *ceilz = fz; *ceilhit = hitwhat; }
                }
            }
        }
#endif
        ////////// Walls //////////

        const sectortype *const startsec = &sector[clipsectorlist[clipsectcnt]];
        const int startwall = startsec->wallptr;
        const int endwall = startwall + startsec->wallnum;

        for (bssize_t j=startwall; j<endwall; j++)
        {
            const int k = wall[j].nextsector;

            if (k >= 0)
            {
                vec2_t const v1 = *(vec2_t *)&wall[j];
                vec2_t const v2 = *(vec2_t *)&wall[wall[j].point2];

                if ((v1.x < xmin && (v2.x < xmin)) || (v1.x > xmax && v2.x > xmax) ||
                    (v1.y < ymin && (v2.y < ymin)) || (v1.y > ymax && v2.y > ymax))
                    continue;

                vec2_t const d = { v2.x-v1.x, v2.y-v1.y };
                if (d.x*(pos->y-v1.y) < (pos->x-v1.x)*d.y) continue; //back

                vec2_t da = { (d.x > 0) ? d.x*(ymin-v1.y) : d.x*(ymax-v1.y),
                              (d.y > 0) ? d.y*(xmax-v1.x) : d.y*(xmin-v1.x) };

                if (da.x >= da.y)
                    continue;

                if (wall[j].cstat&dawalclipmask) continue;  // XXX?
                const sectortype *const sec = &sector[k];

#ifdef HAVE_CLIPSHAPE_FEATURE
                if (curspr)
                {
                    if (k==sectq[clipinfo[curidx].qend])
                        continue;
                    if ((sec->ceilingstat&1) && (sec->floorstat&1))
                        continue;
                }
                else
#endif
                if (editstatus == 0)
                {
                    if (((sec->ceilingstat&1) == 0) && (pos->z <= sec->ceilingz+(3<<8))) continue;
                    if (((sec->floorstat&1) == 0) && (pos->z >= sec->floorz-(3<<8))) continue;
                }

                int i;
                for (i=clipsectnum-1; i>=0; --i)
                    if (clipsectorlist[i] == k) break;

                if (i < 0) clipsectorlist[clipsectnum++] = k;

                if (((v1.x < xmin + MAXCLIPDIST) && (v2.x < xmin + MAXCLIPDIST)) ||
                    ((v1.x > xmax - MAXCLIPDIST) && (v2.x > xmax - MAXCLIPDIST)) ||
                    ((v1.y < ymin + MAXCLIPDIST) && (v2.y < ymin + MAXCLIPDIST)) ||
                    ((v1.y > ymax - MAXCLIPDIST) && (v2.y > ymax - MAXCLIPDIST)))
                    continue;

                if (d.x > 0) da.x += d.x*MAXCLIPDIST; else da.x -= d.x*MAXCLIPDIST;
                if (d.y > 0) da.y -= d.y*MAXCLIPDIST; else da.y += d.y*MAXCLIPDIST;
                if (da.x >= da.y)
                    continue;
#ifdef YAX_ENABLE
                if (mcf==-1 && curspr==NULL)
                    origclipsectorlist[origclipsectnum++] = k;
#endif
                //It actually got here, through all the continue's!!!
                int32_t daz, daz2;
                getzsofslope(k, pos->x,pos->y, &daz,&daz2);

#ifdef HAVE_CLIPSHAPE_FEATURE
                if (curspr)
                {
                    int32_t fz,cz, hitwhat=(curspr-(uspritetype *)sprite)+49152;
                    getzsofslope(sectq[clipinfo[curidx].qend],pos->x,pos->y,&cz,&fz);

                    if ((sec->ceilingstat&1)==0)
                    {
                        if (pos->z < cz && cz < *florz) { *florz = cz; *florhit = hitwhat; }
                        if (pos->z > daz && daz > *ceilz) { *ceilz = daz; *ceilhit = hitwhat; }
                    }
                    if ((sec->floorstat&1)==0)
                    {
                        if (pos->z < daz2 && daz2 < *florz) { *florz = daz2; *florhit = hitwhat; }
                        if (pos->z > fz && fz > *ceilz) { *ceilz = fz; *ceilhit = hitwhat; }
                    }
                }
                else
#endif
                {
#ifdef YAX_ENABLE
                    int16_t cb, fb;
                    yax_getbunches(k, &cb, &fb);
#endif
                    if (daz > *ceilz)
#ifdef YAX_ENABLE
                        if (mcf!=YAX_FLOOR && cb < 0)
#endif
                        *ceilz = daz, *ceilhit = k+16384;

                    if (daz2 < *florz)
#ifdef YAX_ENABLE
                        if (mcf!=YAX_CEILING && fb < 0)
#endif
                        *florz = daz2, *florhit = k+16384;
                }
            }
        }
        clipsectcnt++;
    }
    while (clipsectcnt < clipsectnum || clipspritecnt < clipspritenum);

#ifdef HAVE_CLIPSHAPE_FEATURE
    if (curspr)
    {
        mapinfo_set(NULL, &origmapinfo);  // restore original map
        clipsectnum = clipspritenum = 0;  // skip the next for loop and check afterwards
    }
#endif

    ////////// Sprites //////////

    for (bssize_t i=0; i<clipsectnum; i++)
    {
        if (dasprclipmask==0)
            break;

        for (bssize_t j=headspritesect[clipsectorlist[i]]; j>=0; j=nextspritesect[j])
        {
            const int32_t cstat = sprite[j].cstat;
            int32_t daz, daz2;

            if (cstat&dasprclipmask)
            {
                int32_t clipyou = 0;

#ifdef HAVE_CLIPSHAPE_FEATURE
                if (clipsprite_try((uspritetype *)&sprite[j], xmin,ymin, xmax,ymax))
                    continue;
#endif
                vec2_t v1 = *(vec2_t *)&sprite[j];

                switch (cstat&48)
                {
                    case 0:
                    {
                        int32_t k = walldist+(sprite[j].clipdist<<2)+1;
                        if ((klabs(v1.x-pos->x) <= k) && (klabs(v1.y-pos->y) <= k))
                        {
                            daz = sprite[j].z + spriteheightofs(j, &k, 1);
                            daz2 = daz - k;
                            clipyou = 1;
                        }
                        break;
                    }

                    case 16:
                    {
                        vec2_t v2;
                        get_wallspr_points((uspritetype *)&sprite[j], &v1.x, &v2.x, &v1.y, &v2.y);

                        if (clipinsideboxline(pos->x,pos->y,v1.x,v1.y,v2.x,v2.y,walldist+1) != 0)
                        {
                            int32_t k;
                            daz = sprite[j].z + spriteheightofs(j, &k, 1);
                            daz2 = daz-k;
                            clipyou = 1;
                        }
                        break;
                    }

                    case 32:
                    {
                        daz = sprite[j].z; daz2 = daz;

                        if ((cstat&64) != 0 && (pos->z > daz) == ((cstat&8)==0))
                            continue;

                        vec2_t v2, v3, v4;
                        get_floorspr_points((uspritetype const *) &sprite[j], pos->x, pos->y, &v1.x, &v2.x, &v3.x, &v4.x,
                                            &v1.y, &v2.y, &v3.y, &v4.y);

                        vec2_t const da = { mulscale14(sintable[(sprite[j].ang - 256 + 512) & 2047], walldist + 4),
                                            mulscale14(sintable[(sprite[j].ang - 256) & 2047], walldist + 4) };

                        v1.x += da.x; v2.x -= da.y; v3.x -= da.x; v4.x += da.y;
                        v1.y += da.y; v2.y += da.x; v3.y -= da.y; v4.y -= da.x;

                        clipyou = get_floorspr_clipyou(v1.x, v2.x, v3.x, v4.x, v1.y, v2.y, v3.y, v4.y);
                        break;
                    }
                }

                if (clipyou != 0)
                {
                    if ((pos->z > daz) && (daz > *ceilz
#ifdef YAX_ENABLE
                                           || (daz == *ceilz && yax_getbunch(clipsectorlist[i], YAX_CEILING)>=0)
#endif
                            ))
                    {
                        *ceilz = daz;
                        *ceilhit = j+49152;
                    }

                    if ((pos->z < daz2) && (daz2 < *florz
#ifdef YAX_ENABLE
                                            // can have a floor-sprite lying directly on the floor!
                                            || (daz2 == *florz && yax_getbunch(clipsectorlist[i], YAX_FLOOR)>=0)
#endif
                            ))
                    {
                        *florz = daz2;
                        *florhit = j+49152;
                    }
                }
            }
        }
    }

#ifdef HAVE_CLIPSHAPE_FEATURE
    if (clipspritenum>0)
        goto beginagain;
#endif

#ifdef YAX_ENABLE
    if (numyaxbunches > 0)
    {
        int const dasecclipmask = yax_waltosecmask(dawalclipmask);
        int16_t cb, fb;

        yax_getbunches(sectnum, &cb, &fb);

        mcf++;
        clipsectcnt = 0; clipsectnum = 0;

        int didchange = 0;
        if (cb>=0 && mcf==0 && *ceilhit==sectnum+16384)
        {
            int i;
            for (i=0; i<origclipsectnum; i++)
            {
                int const j = origclipsectorlist[i];
                if (yax_getbunch(j, YAX_CEILING) >= 0)
                    if (sector[j].ceilingstat&dasecclipmask)
                        break;
            }

            if (i==origclipsectnum)
                for (i=0; i<origclipsectnum; i++)
                {
                    cb = yax_getbunch(origclipsectorlist[i], YAX_CEILING);
                    if (cb < 0)
                        continue;

                    for (bssize_t SECTORS_OF_BUNCH(cb,YAX_FLOOR, j))
                        if (inside(pos->x,pos->y, j)==1)
                        {
                            clipsectorlist[clipsectnum++] = j;
                            int const daz = getceilzofslope(j, pos->x,pos->y);
                            if (!didchange || daz > *ceilz)
                                didchange=1, *ceilhit = j+16384, *ceilz = daz;
                        }
                }

            if (clipsectnum==0)
                mcf++;
        }
        else if (mcf==0)
            mcf++;

        didchange = 0;
        if (fb>=0 && mcf==1 && *florhit==sectnum+16384)
        {
            int i=0;
            for (; i<origclipsectnum; i++)
            {
                int const j = origclipsectorlist[i];
                if (yax_getbunch(j, YAX_FLOOR) >= 0)
                    if (sector[j].floorstat&dasecclipmask)
                        break;
            }

            // (almost) same as above, but with floors...
            if (i==origclipsectnum)
                for (i=0; i<origclipsectnum; i++)
                {
                    fb = yax_getbunch(origclipsectorlist[i], YAX_FLOOR);
                    if (fb < 0)
                        continue;

                    for (bssize_t SECTORS_OF_BUNCH(fb, YAX_CEILING, j))
                        if (inside(pos->x,pos->y, j)==1)
                        {
                            clipsectorlist[clipsectnum++] = j;
                            int const daz = getflorzofslope(j, pos->x,pos->y);
                            if (!didchange || daz < *florz)
                                didchange=1, *florhit = j+16384, *florz = daz;
                        }
                }
        }

        if (clipsectnum > 0)
        {
            // sector-like sprite re-init:
            curidx = -1;
            curspr = NULL;
            clipspritecnt = 0; clipspritenum = 0;

            goto restart_grand;
        }
    }
#endif
}

int32_t setaspect_new_use_dimen = 0;

void setaspect_new()
{
    if (r_usenewaspect && newaspect_enable && getrendermode() != REND_POLYMER)
    {
        // The correction factor 100/107 has been found
        // out experimentally. Squares FTW!
        int32_t vr, yx=(65536*4*100)/(3*107);
        int32_t y, x;

        const int32_t xd = setaspect_new_use_dimen ? xdimen : xdim;
        const int32_t yd = setaspect_new_use_dimen ? ydimen : ydim;

        if (fullscreen && !setaspect_new_use_dimen)
        {
            const int32_t screenw = r_screenxy/100;
            const int32_t screenh = r_screenxy%100;

            if (screenw==0 || screenh==0)
            {
                // Assume square pixel aspect.
                x = xd;
                y = yd;
            }
            else
            {
                int32_t pixratio;

                x = screenw;
                y = screenh;

                pixratio = divscale16(xdim*screenh, ydim*screenw);
                yx = divscale16(yx, pixratio);
            }
        }
        else
        {
            x = xd;
            y = yd;
        }

        vr = divscale16(x*3, y*4);

        setaspect(vr, yx);
    }
    else
        setaspect(65536, divscale16(ydim*320, xdim*200));
}

//
// setview
//
void setview(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    windowxy1.x = x1; wx1 = (x1<<12);
    windowxy1.y = y1; wy1 = (y1<<12);
    windowxy2.x = x2; wx2 = ((x2+1)<<12);
    windowxy2.y = y2; wy2 = ((y2+1)<<12);

    xdimen = (x2-x1)+1; halfxdimen = (xdimen>>1);
    xdimenrecip = divscale32(1L,xdimen);
    ydimen = (y2-y1)+1;

    fxdimen = (float) xdimen;
#ifdef USE_OPENGL
    fydimen = (float) ydimen;
#endif
    setaspect_new();

    for (bssize_t i=0; i<windowxy1.x; i++) { startumost[i] = 1, startdmost[i] = 0; }
    Bassert(windowxy2.x < xdim);  // xdim is the number of alloc'd elements in start*most[].
    for (bssize_t i=windowxy1.x; i<=windowxy2.x; i++)
        { startumost[i] = windowxy1.y, startdmost[i] = windowxy2.y+1; }
    for (bssize_t i=windowxy2.x+1; i<xdim; i++) { startumost[i] = 1, startdmost[i] = 0; }
}


//
// setaspect
//
void setaspect(int32_t daxrange, int32_t daaspect)
{
    viewingrange = daxrange;
    viewingrangerecip = divscale32(1,daxrange);
#ifdef USE_OPENGL
    fviewingrange = (float) daxrange;
#endif

    yxaspect = daaspect;
    xyaspect = divscale32(1,yxaspect);
    xdimenscale = scale(xdimen,yxaspect,320);
    xdimscale = scale(320,xyaspect,xdimen);
}


//
// flushperms
//
void flushperms(void)
{
    permhead = permtail = 0;
}


//
// rotatesprite
//
void rotatesprite_(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                   int8_t dashade, char dapalnum, int32_t dastat, uint8_t daalpha, uint8_t dablend,
                   int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2)
{
    int32_t i;

    if ((unsigned)picnum >= MAXTILES)
        return;

    if ((cx1 > cx2) || (cy1 > cy2)) return;
    if (z <= 16) return;
    DO_TILE_ANIM(picnum, (int16_t)0xc000);
    if ((tilesiz[picnum].x <= 0) || (tilesiz[picnum].y <= 0)) return;

    // Experimental / development bits. ONLY FOR INTERNAL USE!
    //  bit RS_CENTERORIGIN: see dorotspr_handle_bit2
    ////////////////////

    if (((dastat & RS_PERM) == 0) || (numpages < 2) || (beforedrawrooms != 0))
    {
        begindrawing(); //{{{
        dorotatesprite(sx,sy,z,a,picnum,dashade,dapalnum,dastat,daalpha,dablend,cx1,cy1,cx2,cy2,guniqhudid);
        enddrawing();   //}}}
    }

    if ((dastat & RS_NOMASK) && (cx1 <= 0) && (cy1 <= 0) && (cx2 >= xdim-1) && (cy2 >= ydim-1) &&
            (sx == (160<<16)) && (sy == (100<<16)) && (z == 65536L) && (a == 0) && ((dastat&RS_TRANS1) == 0))
        permhead = permtail = 0;

    if ((dastat & RS_PERM) == 0)
        return;

    if (numpages >= 2)
    {
        permfifotype *per = &permfifo[permhead];

        per->sx = sx; per->sy = sy; per->z = z; per->a = a;
        per->picnum = picnum;
        per->dashade = dashade; per->dapalnum = dapalnum;
        per->dastat = dastat;
        per->daalpha = daalpha;
        per->dablend = dablend;
        per->pagesleft = numpages+((beforedrawrooms&1)<<7);
        per->cx1 = cx1; per->cy1 = cy1; per->cx2 = cx2; per->cy2 = cy2;
        per->uniqid = guniqhudid;   //JF extension

        //Would be better to optimize out true bounding boxes
        if (dastat & RS_NOMASK)  //If non-masking write, checking for overlapping cases
        {
            for (i=permtail; i!=permhead; i=((i+1)&(MAXPERMS-1)))
            {
                permfifotype *per2 = &permfifo[i];

                if ((per2->pagesleft&127) == 0) continue;
                if (per2->sx != per->sx) continue;
                if (per2->sy != per->sy) continue;
                if (per2->z != per->z) continue;
                if (per2->a != per->a) continue;
                if (tilesiz[per2->picnum].x > tilesiz[per->picnum].x) continue;
                if (tilesiz[per2->picnum].y > tilesiz[per->picnum].y) continue;
                if (per2->cx1 < per->cx1) continue;
                if (per2->cy1 < per->cy1) continue;
                if (per2->cx2 > per->cx2) continue;
                if (per2->cy2 > per->cy2) continue;

                per2->pagesleft = 0;
            }

            if ((per->z == 65536) && (per->a == 0))
                for (i=permtail; i!=permhead; i=((i+1)&(MAXPERMS-1)))
                {
                    permfifotype *per2 = &permfifo[i];

                    if ((per2->pagesleft&127) == 0) continue;
                    if (per2->z != 65536) continue;
                    if (per2->a != 0) continue;
                    if (per2->cx1 < per->cx1) continue;
                    if (per2->cy1 < per->cy1) continue;
                    if (per2->cx2 > per->cx2) continue;
                    if (per2->cy2 > per->cy2) continue;
                    if ((per2->sx>>16) < (per->sx>>16)) continue;
                    if ((per2->sy>>16) < (per->sy>>16)) continue;
                    if ((per2->sx>>16)+tilesiz[per2->picnum].x > (per->sx>>16)+tilesiz[per->picnum].x) continue;
                    if ((per2->sy>>16)+tilesiz[per2->picnum].y > (per->sy>>16)+tilesiz[per->picnum].y) continue;

                    per2->pagesleft = 0;
                }
        }

        permhead = ((permhead+1)&(MAXPERMS-1));
    }
}


//
// clearview
//
void clearview(int32_t dacol)
{
    if (!in3dmode() && dacol != -1) return;

    if (dacol == -1) dacol = 0;

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
    {
        palette_t const p = getpal(dacol);

        bglClearColor((float)p.r * (1.f/255.f),
                      (float)p.g * (1.f/255.f),
                      (float)p.b * (1.f/255.f),
                      0);
        bglClear(GL_COLOR_BUFFER_BIT);
        return;
    }
#endif

    begindrawing(); //{{{
    //dacol += (dacol<<8); dacol += (dacol<<16);
    int const dx = windowxy2.x-windowxy1.x+1;
    intptr_t p = frameplace+ylookup[windowxy1.y]+windowxy1.x;
    for (bssize_t y=windowxy1.y; y<=windowxy2.y; ++y)
    {
        //clearbufbyte((void*)p,dx,dacol);
        Bmemset((void *)p,dacol,dx);
        p += ylookup[1];
    }
    enddrawing();   //}}}

    faketimerhandler();
}


//
// clearallviews
//
void clearallviews(int32_t dacol)
{
    if (!in3dmode()) return;
    //dacol += (dacol<<8); dacol += (dacol<<16);

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
    {
        palette_t const p = getpal(dacol);

        bglViewport(0,0,xdim,ydim); glox1 = -1;
        bglClearColor((float)p.r * (1.f/255.f),
                      (float)p.g * (1.f/255.f),
                      (float)p.b * (1.f/255.f),
                      0);
        bglClear(GL_COLOR_BUFFER_BIT);
        return;
    }
#endif

    begindrawing(); //{{{
    Bmemset((void *)frameplace,dacol,bytesperline*yres);
    enddrawing();   //}}}
    //nextpage();

    faketimerhandler();
}


//MUST USE RESTOREFORDRAWROOMS AFTER DRAWING

//
// setviewtotile
//
void setviewtotile(int16_t tilenume, int32_t xsiz, int32_t ysiz)
{
    //DRAWROOMS TO TILE BACKUP&SET CODE
    tilesiz[tilenume].x = xsiz; tilesiz[tilenume].y = ysiz;
    bakxsiz[setviewcnt] = xsiz; bakysiz[setviewcnt] = ysiz;
    bakframeplace[setviewcnt] = frameplace; frameplace = waloff[tilenume];
    bakwindowxy1[setviewcnt] = windowxy1;
    bakwindowxy2[setviewcnt] = windowxy2;

    if (setviewcnt == 0)
    {
#ifdef USE_OPENGL
        bakrendmode = rendmode;
#endif
        baktile = tilenume;
    }

#ifdef USE_OPENGL
    rendmode = REND_CLASSIC;
#endif

    copybufbyte(&startumost[windowxy1.x],&bakumost[windowxy1.x],(windowxy2.x-windowxy1.x+1)*sizeof(bakumost[0]));
    copybufbyte(&startdmost[windowxy1.x],&bakdmost[windowxy1.x],(windowxy2.x-windowxy1.x+1)*sizeof(bakdmost[0]));
    setviewcnt++;

    offscreenrendering = 1;
    setview(0,0,ysiz-1,xsiz-1);
    setaspect(65536,65536);

    calc_ylookup(ysiz, xsiz);
}


//
// setviewback
//
void setviewback(void)
{
    if (setviewcnt <= 0) return;
    setviewcnt--;

    offscreenrendering = (setviewcnt>0);
#ifdef USE_OPENGL
    if (setviewcnt == 0)
    {
        rendmode = bakrendmode;
        invalidatetile(baktile,-1,-1);
    }
#endif

    setview(bakwindowxy1[setviewcnt].x,bakwindowxy1[setviewcnt].y,
            bakwindowxy2[setviewcnt].x,bakwindowxy2[setviewcnt].y);
    copybufbyte(&bakumost[windowxy1.x],&startumost[windowxy1.x],(windowxy2.x-windowxy1.x+1)*sizeof(startumost[0]));
    copybufbyte(&bakdmost[windowxy1.x],&startdmost[windowxy1.x],(windowxy2.x-windowxy1.x+1)*sizeof(startdmost[0]));
    frameplace = bakframeplace[setviewcnt];

    calc_ylookup(bytesperline,
                 (setviewcnt == 0) ? bakxsiz[0] : max(bakxsiz[setviewcnt - 1], bakxsiz[setviewcnt]));

    modechange=1;
}


//
// squarerotatetile
//
void squarerotatetile(int16_t tilenume)
{
    int const siz = tilesiz[tilenume].x;

    if (siz != tilesiz[tilenume].y)
        return;

    char *ptr1, *ptr2;

    for (bssize_t i=siz-1, j; i>=3; i-=4)
    {
        ptr2 = ptr1 = (char *) (waloff[tilenume]+i*(siz+1));
        swapchar(--ptr1, (ptr2 -= siz));
        for (j=(i>>1)-1; j>=0; --j)
            swapchar2((ptr1 -= 2), (ptr2 -= (siz<<1)), siz);

        ptr2 = ptr1 = (char *) (waloff[tilenume]+(i-1)*(siz+1));
        for (j=((i-1)>>1)-1; j>=0; --j)
            swapchar2((ptr1 -= 2), (ptr2 -= (siz<<1)), siz);

        ptr2 = ptr1 = (char *) (waloff[tilenume]+(i-2)*(siz+1));
        swapchar(--ptr1, (ptr2 -= siz));

        for (j=((i-2)>>1)-1; j>=0; --j)
            swapchar2((ptr1 -= 2), (ptr2 -= (siz<<1)), siz);

        ptr2 = ptr1 = (char *) (waloff[tilenume]+(i-3)*(siz+1));

        for (j=((i-3)>>1)-1; j>=0; --j)
            swapchar2((ptr1 -= 2), (ptr2 -= (siz<<1)), siz);
    }
}

//
// preparemirror
//
void preparemirror(int32_t dax, int32_t day, int16_t daang, int16_t dawall,
                   int32_t *tposx, int32_t *tposy, int16_t *tang)
{
    const int32_t x = wall[dawall].x, dx = wall[wall[dawall].point2].x-x;
    const int32_t y = wall[dawall].y, dy = wall[wall[dawall].point2].y-y;

    const int32_t j = dx*dx + dy*dy;
    if (j == 0)
        return;

    int i = ((dax-x)*dx + (day-y)*dy)<<1;

    *tposx = (x<<1) + scale(dx,i,j) - dax;
    *tposy = (y<<1) + scale(dy,i,j) - day;
    *tang = ((getangle(dx,dy)<<1)-daang)&2047;

    inpreparemirror = 1;
}


//
// completemirror
//
void completemirror(void)
{
#ifdef USE_OPENGL
    if (getrendermode() != REND_CLASSIC)
        return;
#endif

    // Can't reverse when the world has not yet been drawn from the other side.
    if (inpreparemirror) { inpreparemirror = 0; return; }

    // The mirroring code maps the rightmost pixel to the right neighbor of the
    // leftmost one (see copybufreverse() call below). Thus, the leftmost would
    // be mapped to the right neighbor of the rightmost one, which would be out
    // of bounds.
    if (mirrorsx1 == 0)
        mirrorsx1 = 1;

    // Require that the mirror is at least one pixel wide before padding.
    if (mirrorsx1 > mirrorsx2)
        return;

    // Variables mirrorsx{1,2} refer to the source scene here, the one drawn
    // from the inside of the mirror.

    begindrawing();

    // Width in pixels (screen x's are inclusive on both sides):
    int const width = mirrorsx2-mirrorsx1+1;
    // Height in pixels (screen y's are half-open because they come from umost/dmost):
    int const height = mirrorsy2-mirrorsy1;

    // Address of the mirror wall's top left corner in the source scene:
    intptr_t p = frameplace + ylookup[windowxy1.y+mirrorsy1] + windowxy1.x+mirrorsx1;

    // Offset (wrt p) of a mirror line's left corner in the destination:
    // p+destof == frameplace + ylookup[...] + windowxy2.x-mirrorsx2
    int const destofs = windowxy2.x-mirrorsx2-windowxy1.x-mirrorsx1;

    for (bssize_t y=0; y<height; y++)
    {
#if 0
        if ((p-frameplace) + width-1 >= bytesperline*ydim)
            printf("oob read: mirrorsx1=%d, mirrorsx2=%d\n", mirrorsx1, mirrorsx2);
#endif
        copybufbyte((void *)p, tempbuf, width);
        copybufreverse(&tempbuf[width-1], (void *)(p+destofs+1), width);

        p += ylookup[1];
        faketimerhandler();
    }

    enddrawing();
}


//
// sectorofwall
//
static int32_t sectorofwall_internal(int16_t theline)
{
    int32_t gap = numsectors>>1, i = gap;

    while (gap > 1)
    {
        gap >>= 1;
        if (sector[i].wallptr < theline) i += gap; else i -= gap;
    }
    while (sector[i].wallptr > theline) i--;
    while (sector[i].wallptr+sector[i].wallnum <= theline) i++;

    return i;
}

int32_t sectorofwall(int16_t theline)
{
    if ((unsigned)theline >= (unsigned)numwalls)
        return -1;

    int i = wall[theline].nextwall;
    if ((unsigned)i < MAXWALLS)
        return wall[i].nextsector;

    return sectorofwall_internal(theline);
}

int32_t sectorofwall_noquick(int16_t theline)
{
    if ((unsigned)theline >= (unsigned)numwalls)
        return -1;

    return sectorofwall_internal(theline);
}


int32_t getceilzofslopeptr(const usectortype *sec, int32_t dax, int32_t day)
{
    if (!(sec->ceilingstat&2))
        return sec->ceilingz;

    uwalltype const *wal = (uwalltype *)&wall[sec->wallptr];

    // floor(sqrt(2**31-1)) == 46340
    vec2_t const w = *(vec2_t const *)wal;
    vec2_t const d = { wall[wal->point2].x-w.x , wall[wal->point2].y-w.y };

    int const i = nsqrtasm(uhypsq(d.x,d.y))<<5;
    if (i == 0) return sec->ceilingz;

    int const j = dmulscale3(d.x, day-w.y, -d.y, dax-w.x);
    return sec->ceilingz + (scale(sec->ceilingheinum,j>>1,i)<<1);
}

int32_t getflorzofslopeptr(const usectortype *sec, int32_t dax, int32_t day)
{
    if (!(sec->floorstat&2))
        return sec->floorz;

    uwalltype const *wal = (uwalltype *) &wall[sec->wallptr];

    vec2_t const w = *(vec2_t const *)wal;
    vec2_t const d = { wall[wal->point2].x-w.x , wall[wal->point2].y-w.y };

    int const i = nsqrtasm(uhypsq(d.x,d.y))<<5;
    if (i == 0) return sec->floorz;

    int const j = dmulscale3(d.x, day-w.y, -d.y, dax-w.x);
    return sec->floorz + (scale(sec->floorheinum,j>>1,i)<<1);
}

void getzsofslopeptr(const usectortype *sec, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    *ceilz = sec->ceilingz; *florz = sec->floorz;

    if (((sec->ceilingstat|sec->floorstat)&2) != 2)
        return;

    uwalltype const *wal = (uwalltype *) &wall[sec->wallptr];
    uwalltype const *wal2 = (uwalltype *) &wall[wal->point2];
    vec2_t const d = { wal2->x-wal->x, wal2->y-wal->y };

    int const i = nsqrtasm(uhypsq(d.x,d.y))<<5;
    if (i == 0) return;

    int const j = dmulscale3(d.x,day-wal->y, -d.y,dax-wal->x);
    if (sec->ceilingstat&2)
        *ceilz += scale(sec->ceilingheinum,j>>1,i)<<1;
    if (sec->floorstat&2)
        *florz += scale(sec->floorheinum,j>>1,i)<<1;
}


//
// alignceilslope
//
void alignceilslope(int16_t dasect, int32_t x, int32_t y, int32_t z)
{
    const uwalltype *const wal = (uwalltype *)&wall[sector[dasect].wallptr];
    const int32_t dax = wall[wal->point2].x-wal->x;
    const int32_t day = wall[wal->point2].y-wal->y;

    const int32_t i = (y-wal->y)*dax - (x-wal->x)*day;
    if (i == 0)
        return;

    sector[dasect].ceilingheinum = scale((z-sector[dasect].ceilingz)<<8,
                                         nsqrtasm(uhypsq(dax,day)), i);
    if (sector[dasect].ceilingheinum == 0)
        sector[dasect].ceilingstat &= ~2;
    else sector[dasect].ceilingstat |= 2;
}


//
// alignflorslope
//
void alignflorslope(int16_t dasect, int32_t x, int32_t y, int32_t z)
{
    const uwalltype *const wal = (uwalltype *)&wall[sector[dasect].wallptr];
    const int32_t dax = wall[wal->point2].x-wal->x;
    const int32_t day = wall[wal->point2].y-wal->y;

    const int32_t i = (y-wal->y)*dax - (x-wal->x)*day;
    if (i == 0)
        return;

    sector[dasect].floorheinum = scale((z-sector[dasect].floorz)<<8,
                                       nsqrtasm(uhypsq(dax,day)), i);
    if (sector[dasect].floorheinum == 0)
        sector[dasect].floorstat &= ~2;
    else sector[dasect].floorstat |= 2;
}


//
// loopnumofsector
//
int32_t loopnumofsector(int16_t sectnum, int16_t wallnum)
{
    int32_t numloops = 0;
    const int32_t startwall = sector[sectnum].wallptr;
    const int32_t endwall = startwall + sector[sectnum].wallnum;

    for (bssize_t i=startwall; i<endwall; i++)
    {
        if (i == wallnum)
            return numloops;

        if (wall[i].point2 < i)
            numloops++;
    }

    return -1;
}


//
// setfirstwall
//
void setfirstwall(int16_t sectnum, int16_t newfirstwall)
{
    int32_t i, j, numwallsofloop;
    int32_t dagoalloop;
    uwalltype *tmpwall;

    const int32_t startwall = sector[sectnum].wallptr;
    const int32_t danumwalls = sector[sectnum].wallnum;
    const int32_t endwall = startwall+danumwalls;

    if (newfirstwall < startwall || newfirstwall >= startwall+danumwalls)
        return;

    tmpwall = (uwalltype *)Xmalloc(danumwalls * sizeof(walltype));

    Bmemcpy(tmpwall, &wall[startwall], danumwalls*sizeof(walltype));

    numwallsofloop = 0;
    i = newfirstwall;
    do
    {
        numwallsofloop++;
        i = wall[i].point2;
    }
    while (i != newfirstwall);

    //Put correct loop at beginning
    dagoalloop = loopnumofsector(sectnum,newfirstwall);
    if (dagoalloop > 0)
    {
        j = 0;
        while (loopnumofsector(sectnum,j+startwall) != dagoalloop)
            j++;

        for (i=0; i<danumwalls; i++)
        {
            int32_t k = i+j;
            if (k >= danumwalls) k -= danumwalls;
            Bmemcpy(&wall[startwall+i], &tmpwall[k], sizeof(walltype));

            wall[startwall+i].point2 += danumwalls-startwall-j;
            if (wall[startwall+i].point2 >= danumwalls)
                wall[startwall+i].point2 -= danumwalls;
            wall[startwall+i].point2 += startwall;
        }

        newfirstwall += danumwalls-j;
        if (newfirstwall >= startwall+danumwalls)
            newfirstwall -= danumwalls;
    }

    for (i=0; i<numwallsofloop; i++)
        Bmemcpy(&tmpwall[i], &wall[i+startwall], sizeof(walltype));
    for (i=0; i<numwallsofloop; i++)
    {
        int32_t k = i+newfirstwall-startwall;
        if (k >= numwallsofloop) k -= numwallsofloop;
        Bmemcpy(&wall[startwall+i], &tmpwall[k], sizeof(walltype));

        wall[startwall+i].point2 += numwallsofloop-newfirstwall;
        if (wall[startwall+i].point2 >= numwallsofloop)
            wall[startwall+i].point2 -= numwallsofloop;
        wall[startwall+i].point2 += startwall;
    }

    for (i=startwall; i<endwall; i++)
        if (wall[i].nextwall >= 0)
            wall[wall[i].nextwall].nextwall = i;

#ifdef YAX_ENABLE
    int16_t cb, fb;
    yax_getbunches(sectnum, &cb, &fb);

    if (cb>=0 || fb>=0)
    {
        for (i=startwall; i<endwall; i++)
        {
            j = yax_getnextwall(i, YAX_CEILING);
            if (j >= 0)
                yax_setnextwall(j, YAX_FLOOR, i);

            j = yax_getnextwall(i, YAX_FLOOR);
            if (j >= 0)
                yax_setnextwall(j, YAX_CEILING, i);
        }
    }
#endif

    Bfree(tmpwall);
}

//
// qsetmodeany
//
void qsetmodeany(int32_t daxdim, int32_t daydim)
{
    if (daxdim < 640) daxdim = 640;
    if (daydim < 480) daydim = 480;

    if (qsetmode != ((daxdim<<16)|(daydim&0xffff)))
    {
        g_lastpalettesum = 0;
        if (setvideomode(daxdim, daydim, 8, fullscreen) < 0)
            return;

        xdim = xres;
        ydim = yres;

#ifdef USE_OPENGL
        fxdim = (float) xres;
        fydim = (float) yres;
#endif

        initsmost();

        ydim16 = yres - STATUS2DSIZ2;
        halfxdim16 = xres >> 1;
        midydim16 = ydim16 >> 1; // scale(200,yres,480);

        begindrawing(); //{{{
        Bmemset((char *)frameplace, 0, yres*bytesperline);
        enddrawing();   //}}}
    }

    qsetmode = ((daxdim<<16)|(daydim&0xffff));
}

static int32_t printext_checkypos(int32_t ypos, int32_t *yminptr, int32_t *ymaxptr)
{
    int32_t ymin=0, ymax=7;

    if (ypos < 0)
    {
/*
        ymin = 0-ypos;
        if (ymin > 7)
            return 1;
*/
    }
    else if (ypos+7 >= ydim)
    {
        ymax = ydim-ypos-1;
        if (ymax < 0)
            return 1;
    }

    *yminptr = ymin;
    *ymaxptr = ymax;

    return 0;
}

//
// printext16
//
int32_t printext16(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, const char *name, char fontsize)
{
    int32_t ymin, ymax;

    if (printext_checkypos(ypos, &ymin, &ymax))
        return 0;

    if (fontsize & 2) printext16(xpos+1, ypos+1, 0, -1, name, (fontsize & ~2) | 4);

    int32_t const ocol = col, obackcol = backcol;
    char smallbuf[4];

    int32_t stx = xpos;
    const int32_t xpos0 = xpos;

    char const * const fontptr = (fontsize & 1) ? smalltextfont : textfont;
    int const charxsiz = 8 - ((fontsize & 1)<<2);

    for (bssize_t i=0; name[i]; i++)
    {
        if (name[i] == '^')
        {
            i++;
            if (name[i] == 'O') // ^O resets formatting
            {
                if (fontsize & 4) continue;

                col = ocol;
                backcol = obackcol;
                continue;
            }
            if (isdigit(name[i]))
            {
                if (isdigit(name[i+1]))
                {
                    if (isdigit(name[i+2]))
                    {
                        Bmemcpy(&smallbuf[0],&name[i],3);
                        i += 2;
                        smallbuf[3] = '\0';
                    }
                    else
                    {
                        Bmemcpy(&smallbuf[0],&name[i],2);
                        i++;
                        smallbuf[2] = '\0';
                    }
                }
                else
                {
                    smallbuf[0] = name[i];
                    smallbuf[1] = '\0';
                }
                if (!(fontsize & 4))
                    col = editorcolors[Batol(smallbuf)];

                if (name[i+1] == ',' && isdigit(name[i+2]))
                {
                    i+=2;
                    if (isdigit(name[i+1]))
                    {
                        if (isdigit(name[i+2]))
                        {
                            Bmemcpy(&smallbuf[0],&name[i],3);
                            i += 2;
                            smallbuf[3] = '\0';
                        }
                        else
                        {
                            Bmemcpy(&smallbuf[0],&name[i],2);
                            i++;
                            smallbuf[2] = '\0';
                        }
                    }
                    else
                    {
                        smallbuf[0] = name[i];
                        smallbuf[1] = '\0';
                    }

                    if (!(fontsize & 4))
                        backcol = editorcolors[Batol(smallbuf)];
                }
                continue;
            }
        }

        if (name[i] == '\n')
        {
            xpos = stx = xpos0;
            ypos += 8;
            if (printext_checkypos(ypos, &ymin, &ymax))
                return 0;
            continue;
        }

        if (stx<0)
        {
            stx += charxsiz;
            continue;
        }

        char const * const letptr = &fontptr[name[i]<<3];

        begindrawing(); //{{{
        char *ptr = (char *)(bytesperline*ypos + (stx-(fontsize&1)) + frameplace);
        int const trans = (obackcol < -1);

        if (trans && backcol < 0)
            backcol = -backcol;

        if (backcol >= 0)
        {
            for (bssize_t y=ymin; y<=ymax; y++)
            {
                for (bssize_t x=0; x<charxsiz; x++)
                {
                    if ((unsigned) (stx+x) >= (unsigned)xdim || ptr < (char *) frameplace) break;
                    ptr[x] = (letptr[y] & pow2char[7 - (fontsize & 1) - x]) ?
                             (uint8_t)col :
                             trans ? (uint8_t)blendtable[0][(ptr[x] * 256) + backcol] : backcol;
                }
                ptr += bytesperline;
            }
        }
        else
        {
            for (bssize_t y=ymin; y<=ymax; y++)
            {
                for (bssize_t x=0; x<charxsiz; x++)
                {
                    if ((unsigned) (stx+x) >= (unsigned)xdim || ptr < (char *) frameplace) break;
                    if (letptr[y]&pow2char[7-(fontsize&1)-x]) ptr[x] = (uint8_t) col;
                }
                ptr += bytesperline;
            }
        }
        enddrawing();   //}}}

        stx += charxsiz;

        if (stx >= xdim)
            break;
    }

    return stx;
}


//
// printext256
//
void printext256(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, const char *name, char fontsize)
{
    int32_t stx, i, x, y, charxsiz;
    char *fontptr, *letptr, *ptr;

    stx = xpos;

    if (fontsize) { fontptr = smalltextfont; charxsiz = 4; }
    else { fontptr = textfont; charxsiz = 8; }

#ifdef USE_OPENGL
    if (!polymost_printext256(xpos,ypos,col,backcol,name,fontsize)) return;
# if 0
    if (getrendermode() >= REND_POLYMOST && in3dmode())
    {
        int32_t xx, yy;
        int32_t lc=-1;
        palette_t p=getpal(col), b=getpal(backcol);

        setpolymost2dview();
        bglDisable(GL_ALPHA_TEST);
        bglDepthMask(GL_FALSE);	// disable writing to the z-buffer

        bglBegin(GL_POINTS);

        for (i=0; name[i]; i++)
        {
            // TODO: factor out!
            if (name[i] == '^' && isdigit(name[i+1]))
            {
                char smallbuf[8];
                int32_t bi=0;

                while (isdigit(name[i+1]) && bi<3)
                {
                    smallbuf[bi++]=name[i+1];
                    i++;
                }
                smallbuf[bi++]=0;
                if (col)
                    col = Batol(smallbuf);

                p = getpal(col);

                continue;
            }
            letptr = &fontptr[name[i]<<3];
            xx = stx-fontsize;
            yy = ypos+7 + 2; //+1 is hack!
            for (y=7; y>=0; y--)
            {
                for (x=charxsiz-1; x>=0; x--)
                {
                    if (letptr[y]&pow2char[7-fontsize-x])
                    {
                        if (lc!=col)
                            bglColor4ub(p.r,p.g,p.b,255);
                        lc = col;
                        bglVertex2i(xx+x,yy);
                    }
                    else if (backcol >= 0)
                    {
                        if (lc!=backcol)
                            bglColor4ub(b.r,b.g,b.b,255);
                        lc = backcol;
                        bglVertex2i(xx+x,yy);
                    }
                }
                yy--;
            }
            stx += charxsiz;
        }

        bglEnd();
        bglDepthMask(GL_TRUE);  // re-enable writing to the z-buffer

        return;
    }
# endif
#endif

    begindrawing(); //{{{
    for (i=0; name[i]; i++)
    {
        if (name[i] == '^' && isdigit(name[i+1]))
        {
            char smallbuf[8];
            int32_t bi=0;
            while (isdigit(name[i+1]) && bi<3)
            {
                smallbuf[bi++]=name[i+1];
                i++;
            }
            smallbuf[bi++]=0;
            if (col)col = Batol(smallbuf);
            continue;
        }

        if (stx-fontsize+charxsiz > xdim)
            break;

        letptr = &fontptr[name[i]<<3];
        ptr = (char *)(ylookup[ypos+7]+(stx-fontsize)+frameplace);
        for (y=7; y>=0; y--)
        {
            for (x=charxsiz-1; x>=0; x--)
            {
                if (letptr[y]&pow2char[7-fontsize-x])
                    ptr[x] = (uint8_t)col;
                else if (backcol >= 0)
                    ptr[x] = (uint8_t)backcol;
            }
            ptr -= ylookup[1];
        }
        stx += charxsiz;
    }
    enddrawing();   //}}}
}


#ifdef POLYMER
static void PolymerProcessModels(void)
{
    // potentially deferred MD3 postprocessing
    for (bssize_t i=0; i<nextmodelid; i++)
    {
        if (models[i]->mdnum==3 && ((md3model_t *)models[i])->head.surfs[0].geometry == NULL)
        {
            static int32_t warned=0;

            if (!warned)
            {
                OSD_Printf("Post-processing MD3 models for Polymer. This may take a while...\n");
                nextpage();
                warned = 1;
            }

            if (!md3postload_polymer((md3model_t *)models[i]))
                OSD_Printf("INTERNAL ERROR: mdmodel %s failed postprocessing!\n",
                           ((md3model_t *)models[i])->head.nam);

            if (((md3model_t *)models[i])->head.surfs[0].geometry == NULL)
                OSD_Printf("INTERNAL ERROR: wtf?\n");
        }
//        else
//            OSD_Printf("mdmodel %d already postprocessed.\n", i);
    }
}
#endif

//
// setrendermode
//
int32_t setrendermode(int32_t renderer)
{
    UNREFERENCED_PARAMETER(renderer);

#ifdef USE_OPENGL
    if (bpp == 8)
        renderer = REND_CLASSIC;
# ifdef POLYMER
    else
        renderer = clamp(renderer, REND_POLYMOST, REND_POLYMER);

    if (renderer == REND_POLYMER)
    {
        PolymerProcessModels();

        if (!polymer_init())
            renderer = REND_POLYMOST;
    }
    else if (getrendermode() == REND_POLYMER)  // going from Polymer to another renderer
    {
        delete_maphack_lights();
        G_Polymer_UnInit();
        polymer_uninit();
    }
# else
    else renderer = REND_POLYMOST;
# endif

    basepalreset = 1;

    rendmode = renderer;
    if (getrendermode() >= REND_POLYMOST)
        glrendmode = rendmode;
#endif

    return 0;
}

//
// setrollangle
//
#ifdef USE_OPENGL
void setrollangle(int32_t rolla)
{
    gtang = (float)rolla * (fPI * (1.f/1024.f));
}
#endif


//
// invalidatetile
//  pal: pass -1 to invalidate all palettes for the tile, or >=0 for a particular palette
//  how: pass -1 to invalidate all instances of the tile in texture memory, or a bitfield
//         bit 0: opaque or masked (non-translucent) texture, using repeating
//         bit 1: ignored
//         bit 2: ignored (33% translucence, using repeating)
//         bit 3: ignored (67% translucence, using repeating)
//         bit 4: opaque or masked (non-translucent) texture, using clamping
//         bit 5: ignored
//         bit 6: ignored (33% translucence, using clamping)
//         bit 7: ignored (67% translucence, using clamping)
//       clamping is for sprites, repeating is for walls
//
void invalidatetile(int16_t tilenume, int32_t pal, int32_t how)
{
#if !defined USE_OPENGL
    UNREFERENCED_PARAMETER(tilenume);
    UNREFERENCED_PARAMETER(pal);
    UNREFERENCED_PARAMETER(how);
#else
    if (getrendermode() >= REND_POLYMOST)
    {
        const int32_t firstpal = (pal < 0) ? 0 : pal;
        const int32_t numpals = (pal < 0) ? MAXPALOOKUPS : 1;

        for (bssize_t hp = 0; hp <= 4; hp+=4)
        {
            if (how & pow2long[hp])
                for (bssize_t np = firstpal; np < firstpal+numpals; np++)
                    gltexinvalidate(tilenume, np, hp);
        }

#ifdef POLYMER
        if (getrendermode() == REND_POLYMER)
            polymer_invalidateartmap(tilenume);
#endif
    }
#endif
}

/*
 * vim:ts=8:
 */

