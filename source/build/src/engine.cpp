// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#define engine_c_

#include "a.h"
#include "baselayer.h"
#include "build.h"

#include "imagehelpers.h"
#include "common.h"
#include "compat.h"
#include "engine_priv.h"
#include "osd.h"
#include "palette.h"
#include "pragmas.h"
#include "scriptfile.h"
#include "gamecvars.h"
#include "c_console.h"
#include "v_2ddrawer.h"
#include "v_draw.h"
#include "imgui.h"
#include "stats.h"
#include "menu.h"
#include "version.h"

#ifdef USE_OPENGL
# include "glsurface.h"
# include "hightile.h"
# include "mdsprite.h"
# include "polymost.h"
#include "v_video.h"
#include "../../glbackend/glbackend.h"
#endif

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

// Handle nonpow2-ysize walls the old way?
static FORCE_INLINE int32_t oldnonpow2(void)
{
#if !defined CLASSIC_NONPOW2_YSIZE_WALLS
    return 1;
#else
    return (g_loadedMapVersion < 10);
#endif
}

bool playing_rr;
bool playing_blood;
int32_t rendmode=0;
int32_t glrendmode = REND_POLYMOST;
int32_t r_scenebrightness = 0;
int32_t r_rortexture = 0;
int32_t r_rortexturerange = 0;
int32_t r_rorphase = 0;
int32_t mdtims, omdtims;
uint8_t alphahackarray[MAXTILES];
int32_t polymostcenterhoriz = 100;

float fcosglobalang, fsinglobalang;
float fxdim, fydim, fydimen, fviewingrange;

uint8_t globalr = 255, globalg = 255, globalb = 255;

int16_t pskybits_override = -1;

// This was on the cache but is permanently allocated, so put it into something static. This needs some rethinking anyway
static TArray<TArray<uint8_t>> voxelmemory;

void (*loadvoxel_replace)(int32_t voxindex) = NULL;
int16_t tiletovox[MAXTILES];
#ifdef USE_OPENGL
char *voxfilenames[MAXVOXELS];
#endif
char g_haveVoxels;
//#define kloadvoxel loadvoxel

int32_t novoxmips = 1;

//These variables need to be copied into BUILD
#define MAXXSIZ 256
#define MAXYSIZ 256
#define MAXZSIZ 255
#ifdef EDUKE32_TOUCH_DEVICES
# define DISTRECIPSIZ (65536+256)
#else
# define DISTRECIPSIZ 131072
#endif

int32_t voxscale[MAXVOXELS];

static int32_t ggxinc[MAXXSIZ+1], ggyinc[MAXXSIZ+1];
static int32_t lowrecip[1024], nytooclose;
static const int32_t nytoofar = DISTRECIPSIZ*16384ull - 1048576;
static uint32_t *distrecip;
#define DISTRECIPCACHESIZE 3
static struct {
    uint32_t *distrecip;
    int32_t xdimen;
    int32_t age;
} distrecipcache[DISTRECIPCACHESIZE];
static int32_t distrecipagecnt = 0;

static TArray<int32_t> lookups;
static int32_t beforedrawrooms = 1;

static int32_t oxdimen = -1, oviewingrange = -1, oxyaspect = -1;

// r_usenewaspect is the cvar, newaspect_enable to trigger the new behaviour in the code
CVAR(Bool, r_usenewaspect, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);
int32_t newaspect_enable=0;

int32_t r_fpgrouscan = 1;
int32_t globalflags;

//Textured Map variables
static char globalpolytype;
static TArray<int16_t *>dotp1, dotp2;

static int8_t tempbuf[MAXWALLS];

// referenced from asm
int32_t ebpbak, espbak;
int32_t reciptable[2048], fpuasm;
intptr_t asm1, asm2, asm3, asm4, palookupoffse[4];
uint32_t vplce[4];
int32_t vince[4];
intptr_t bufplce[4];
int32_t globaltilesizy;
int32_t globalx1, globaly2, globalx3, globaly3;

int32_t sloptable[SLOPTABLESIZ];
#define SLOPALOOKUPSIZ 16384
static intptr_t slopalookup[SLOPALOOKUPSIZ];    // was 2048

static int32_t no_radarang2 = 0;
static int16_t radarang[1280];
static int32_t qradarang[10240];
static TArray<int32_t> radarang2;
const char ATTRIBUTE((used)) pow2char_[8] = {1,2,4,8,16,32,64,128};

uint16_t ATTRIBUTE((used)) sqrtable[4096], ATTRIBUTE((used)) shlookup[4096+256], ATTRIBUTE((used)) sqrtable_old[2048];

char britable[16][256]; // JBF 20040207: full 8bit precision

static char kensmessage[128];
const char *engineerrstr = "No error";

int32_t showfirstwall=0;
int32_t showheightindicators=1;
int32_t circlewall=-1;

static void classicScanSector(int16_t startsectnum);

int16_t editstatus = 0;
static fix16_t global100horiz;  // (-100..300)-scale horiz (the one passed to drawrooms)

int32_t(*getpalookup_replace)(int32_t davis, int32_t dashade) = NULL;

// adapted from build.c
static void getclosestpointonwall_internal(vec2_t const p, int32_t const dawall, vec2_t *const closest)
{
    vec2_t const w  = wall[dawall].pos;
    vec2_t const w2 = wall[wall[dawall].point2].pos;
    vec2_t const d  = { w2.x - w.x, w2.y - w.y };

    int64_t i = d.x * ((int64_t)p.x - w.x) + d.y * ((int64_t)p.y - w.y);

    if (i <= 0)
    {
        *closest = w;
        return;
    }

    int64_t const j = (int64_t)d.x * d.x + (int64_t)d.y * d.y;

    if (i >= j)
    {
        *closest = w2;
        return;
    }

    i = ((i << 15) / j) << 15;
    //i = tabledivide64((i << 15), j) << 15;

    *closest = { (int32_t)(w.x + ((d.x * i) >> 30)), (int32_t)(w.y + ((d.y * i) >> 30)) };
}

////////// YAX //////////

#ifdef YAX_DEBUG
// XXX: This could be replaced with the use of gethiticks().
double u64tickspersec;
#endif
#ifdef ENGINE_SCREENSHOT_DEBUG
int32_t engine_screenshot = 0;
#endif

void faketimerhandler()
{
}



#if !defined YAX_ENABLE
# warning Non-TROR builds are supported only for debugging. Expect savegame breakage etc...
#endif

#ifdef YAX_ENABLE
// all references to floor/ceiling bunchnums should be through the
// get/set functions!

int32_t g_nodraw = 0;
int32_t scansector_retfast = 0;
int32_t scansector_collectsprites = 1;
int32_t yax_globalcf = -1, yax_nomaskpass=0, yax_nomaskdidit;  // engine internal
int32_t r_tror_nomaskpass = 1;  // cvar
int32_t yax_globallev = YAX_MAXDRAWS;
int32_t yax_globalbunch = -1;
int32_t yax_polymostclearzbuffer = 1;

// duplicated tsprites
//  [i]:
//   i==MAXDRAWS: base level
//   i<MAXDRAWS: MAXDRAWS-i-1 is level towards ceiling
//   i>MAXDRAWS: i-MAXDRAWS-1 is level towards floor
static int16_t yax_spritesortcnt[1 + 2*YAX_MAXDRAWS];
static uint16_t yax_tsprite[1 + 2*YAX_MAXDRAWS][MAXSPRITESONSCREEN];
static uint8_t yax_tsprfrombunch[1 + 2*YAX_MAXDRAWS][MAXSPRITESONSCREEN];

// drawn sectors
uint8_t yax_gotsector[(MAXSECTORS+7)>>3];  // engine internal

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
        uint8_t *const bunchmap = havebunch + ((YAX_MAXBUNCHES+7)>>3);
        int32_t dasub = 0;

        Bmemset(havebunch, 0, (YAX_MAXBUNCHES+7)>>3);
        for (i=0; i<numsectors; i++)
        {
            yax_getbunches(i, &cb, &fb);
            if (cb>=0)
                havebunch[cb>>3] |= pow2char[cb&7];
            if (fb>=0)
                havebunch[fb>>3] |= pow2char[fb&7];
        }

        for (i=0; i<YAX_MAXBUNCHES; i++)
        {
            if ((havebunch[i>>3]&pow2char[i&7])==0)
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
uint8_t haveymost[(YAX_MAXBUNCHES+7)>>3];

static inline int32_t yax_walldist(int32_t w)
{
    vec2_t closest;
    getclosestpointonwall_internal({ globalposx, globalposy }, w, &closest);
    return klabs(closest.x-globalposx) + klabs(closest.y-globalposy);
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
                        if ((lastgotsector[ns>>3]&pow2char[ns&7])==0)
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
                if (videoGetRenderMode() == REND_CLASSIC)
                    classicScanSector(k);
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
                    SECTORFLD(i,picnum, cf) = playing_blood ? MAXTILES-2 : 13; //FOF;
            }
            else
            {
                SECTORFLD(i,picnum, cf) = opicnum[cf][i];
            }
#ifdef POLYMER
            // will be called only in editor
            if (videoGetRenderMode() == REND_POLYMER)
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
    uspriteptr_t spr;

    for (i=0; i<sortcnt; i++)
    {
        spritenum = yax_tsprite[yax_globallev][i];

        gotthrough = spritenum&(MAXSPRITES|(MAXSPRITES<<1));

        spritenum &= MAXSPRITES-1;
        spr = (uspriteptr_t)&sprite[spritenum];
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

        tspriteptr_t tsp = renderAddTSpriteFromSprite(spritenum);
        tsp->sectnum = sectnum;  // potentially tweak sectnum!
    }
}


void yax_preparedrawrooms(void)
{
    if (videoGetRenderMode() == REND_POLYMER || numyaxbunches==0)
        return;

    g_nodraw = 1;
    Bmemset(yax_spritesortcnt, 0, sizeof(yax_spritesortcnt));
    Bmemset(haveymost, 0, (numyaxbunches+7)>>3);

    if (videoGetRenderMode() == REND_CLASSIC && ymostallocsize < xdimen*numyaxbunches)
    {
        ymostallocsize = xdimen*numyaxbunches;
        yumost = (int16_t *)Xrealloc(yumost, ymostallocsize*sizeof(int16_t));
        ydmost = (int16_t *)Xrealloc(ydmost, ymostallocsize*sizeof(int16_t));
    }
}

void yax_drawrooms(void (*SpriteAnimFunc)(int32_t,int32_t,int32_t,int32_t,int32_t),
                   int16_t sectnum, int32_t didmirror, int32_t smoothr)
{
    static uint8_t havebunch[(YAX_MAXBUNCHES+7)>>3];

    const fix16_t horiz = global100horiz;

    int32_t i, j, k, lev, cf, nmp;
    int32_t bnchcnt, bnchnum[2] = {0,0}, maxlev[2];
    int16_t ourbunch[2] = {-1,-1}, osectnum=sectnum;
    int32_t bnchbeg[YAX_MAXDRAWS][2], bnchend[YAX_MAXDRAWS][2];
    int32_t bbeg, numhere;

    // original (1st-draw) and accumulated ('per-level') gotsector bitmaps
    static uint8_t ogotsector[(MAXSECTORS+7)>>3], lgotsector[(MAXSECTORS+7)>>3];
#ifdef YAX_DEBUG
    uint64_t t;
#endif

    if (videoGetRenderMode() == REND_POLYMER || numyaxbunches==0)
    {
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
                if (!(gotsector[i>>3]&pow2char[i&7]))
                    continue;

                j = yax_getbunch(i, cf);
                if (j >= 0 && !(havebunch[j>>3]&pow2char[j&7]))
                {
                    if (videoGetRenderMode() == REND_CLASSIC && (haveymost[j>>3]&pow2char[j&7])==0)
                    {
                        yaxdebug("%s, l %d: skipped bunch %d (no *most)", cf?"v":"^", lev, j);
                        continue;
                    }

                    if ((SECTORFLD(i,stat, cf)&2) ||
                            (cf==0 && globalposz >= sector[i].ceilingz) ||
                            (cf==1 && globalposz <= sector[i].floorz))
                    {
                        havebunch[j>>3] |= pow2char[j&7];
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
                    Bmemset(lgotsector, 0, sizeof(lgotsector));

                for (bnchcnt=bbeg; bnchcnt < bbeg+numhere; bnchcnt++)
                {
                    j = bunches[cf][bnchcnt];  // the actual bunchnum...
                    yax_globalbunch = j;
#ifdef YAX_DEBUG
                    t=timerGetTicksU64();
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
                        renderDrawRoomsQ16(globalposx,globalposy,globalposz,qglobalang,horiz,k+MAXSECTORS);
                        if (numhere > 1)
                            for (i=0; i<(numsectors+7)>>3; i++)
                                lgotsector[i] |= gotsector[i];

                        yaxdebug("l%d: faked (bn %2d) sec %4d,%3d dspr, ob=[%2d,%2d], sn=%4d, %.3f ms",
                                 yax_globallev-YAX_MAXDRAWS, j, k, yax_spritesortcnt[yax_globallev]-odsprcnt,
                                 ourbunch[0],ourbunch[1],sectnum,
                                 (double)(1000*(timerGetTicksU64()-t))/u64tickspersec);
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

#ifdef USE_OPENGL
    if (videoGetRenderMode() == REND_POLYMOST)
    {
		GLInterface.ClearScreen(0, true);
		yax_polymostclearzbuffer = 0;
    }
#endif

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
                t=timerGetTicksU64();
#endif
                yax_tweakpicnums(j, cf, 0);
                if (k < 0)
                    continue;

                yax_nomaskdidit = 0;
                for (nmp=r_tror_nomaskpass; nmp>=0; nmp--)
                {
                    yax_nomaskpass = nmp;
                    renderDrawRoomsQ16(globalposx,globalposy,globalposz,qglobalang,horiz,k+MAXSECTORS);  // +MAXSECTORS: force

                    if (nmp==1)
                    {
                        yaxdebug("nm1 l%d: DRAWN (bn %2d) sec %4d,          %.3f ms",
                                 yax_globallev-YAX_MAXDRAWS, j, k,
                                 (double)(1000*(timerGetTicksU64()-t))/u64tickspersec);

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
                         (double)(1000*(timerGetTicksU64()-t))/u64tickspersec);

                SpriteAnimFunc(globalposx, globalposy, globalposz, globalang, smoothr);
                renderDrawMasks();
            }

            if (lev < maxlev[cf])
                for (bnchcnt=bnchbeg[lev+1][cf]; bnchcnt<bnchend[lev+1][cf]; bnchcnt++)
                    yax_tweakpicnums(bunches[cf][bnchcnt], cf, 1);  // restore picnums
        }
    }

#ifdef YAX_DEBUG
    t=timerGetTicksU64();
#endif
    yax_globalcf = -1;
    yax_globalbunch = -1;
    yax_globallev = YAX_MAXDRAWS;
    scansector_collectsprites = 0;

    // draw base level
    renderDrawRoomsQ16(globalposx,globalposy,globalposz,qglobalang,horiz,
              osectnum + MAXSECTORS*didmirror);
//    if (scansector_collectsprites)
//        spritesortcnt = 0;
    yax_copytsprites();
    yaxdebug("DRAWN base level sec %d,%3d tspr, %.3f ms", osectnum,
             spritesortcnt, (double)(1000*(timerGetTicksU64()-t))/u64tickspersec);
    scansector_collectsprites = 1;

    for (cf=0; cf<2; cf++)
        if (maxlev[cf] >= 0)
            for (bnchcnt=bnchbeg[0][cf]; bnchcnt<bnchend[0][cf]; bnchcnt++)
                yax_tweakpicnums(bunches[cf][bnchcnt], cf, 1);  // restore picnums

#ifdef ENGINE_SCREENSHOT_DEBUG
    engine_screenshot = 0;
#endif

#ifdef USE_OPENGL
    if (videoGetRenderMode() == REND_POLYMOST)
        yax_polymostclearzbuffer = 1;
#endif
}

#endif  // defined YAX_ENABLE


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
static TArray<int16_t> smost, umost, dmost, bakumost, bakdmost;
static TArray<int16_t> uplc, dplc, uwall, dwall;
static TArray<int32_t> swplc, lplc, swall, lwall;
#ifdef HIGH_PRECISION_SPRITE
static TArray<float> swallf;
#endif

TArray<uint8_t> mirrorBuffer;

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
fix16_t qglobalhoriz;
float fglobalposx, fglobalposy, fglobalposz;
int16_t globalang, globalcursectnum;
fix16_t qglobalang;
int32_t globalpal, cosglobalang, singlobalang;
int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;
static int32_t globaluclip, globaldclip;
int32_t globvis, globalvisibility;
int32_t globalhisibility, globalpisibility, globalcisibility;
#ifdef USE_OPENGL
int32_t globvis2, globalvisibility2, globalhisibility2, globalpisibility2, globalcisibility2;
#endif
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
static TArray<int32_t> lastx;

int32_t halfxdim16, midydim16;

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

#define MAXSETVIEW 4

static int32_t setviewcnt = 0; // interface layers use this now
static intptr_t bakframeplace[MAXSETVIEW];
static int32_t bakxsiz[MAXSETVIEW], bakysiz[MAXSETVIEW];
static vec2_t bakwindowxy1[MAXSETVIEW], bakwindowxy2[MAXSETVIEW];
#ifdef USE_OPENGL
static int32_t bakrendmode;
#endif
static int32_t baktile;

#ifdef GAMENAME
char apptitle[256] = GAMENAME;
#else
char apptitle[256] = "Build Engine";
#endif

//
// Internal Engine Functions
//

// returns: 0=continue sprite collecting;
//          1=break out of sprite collecting;
int32_t renderAddTsprite(int16_t z, int16_t sectnum)
{
    auto const spr = (uspriteptr_t)&sprite[z];
#ifdef YAX_ENABLE
    if (g_nodraw==0)
    {
        if (numyaxbunches==0)
        {
#endif
            if (spritesortcnt >= maxspritesonscreen)
                return 1;

            renderAddTSpriteFromSprite(z);

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

static FORCE_INLINE vec2_t get_rel_coords(int32_t const x, int32_t const y)
{
    return { dmulscale6(y, cosglobalang, -x, singlobalang),
             dmulscale6(x, cosviewingrangeglobalang, y, sinviewingrangeglobalang) };
}

// Note: the returned y coordinates are not actually screen coordinates, but
// potentially clipped player-relative y coordinates.
static int get_screen_coords(const vec2_t &p1, const vec2_t &p2,
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

        int32_t const tempint = (p1.y - p1.x) + (p2.x - p2.y);

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
static void classicScanSector(int16_t startsectnum)
{
    if (startsectnum < 0)
        return;

    if (automapping)
        show2dsector[startsectnum>>3] |= pow2char[startsectnum&7];

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
            auto const spr = (uspriteptr_t)&sprite[i];

            if (((spr->cstat & 0x8000) && !showinvisibility) || spr->xrepeat == 0 || spr->yrepeat == 0)
                continue;

            vec2_t const s = { spr->x-globalposx, spr->y-globalposy };

            if ((spr->cstat&48) || ((coord_t)s.x*cosglobalang+(coord_t)s.y*singlobalang > 0))
                if ((spr->cstat&(64+48))!=(64+16) || dmulscale6(sintable[(spr->ang+512)&2047],-s.x, sintable[spr->ang&2047],-s.y) > 0)
                    if (renderAddTsprite(i, sectnum))
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
            auto const wal = (uwallptr_t)&wall[w];
            const int32_t nextsectnum = wal->nextsector;
            auto const wal2 = (uwallptr_t)&wall[wal->point2];

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

            if (numscans >= MAXWALLSB-1)
            {
                OSD_Printf("!!numscans\n");
                return;
            }

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
        if (bunchp2[s] >= 0 && (didscan[s>>3] & pow2char[s&7])==0)
        {
            printf("scan ");

            int z = s;
            do
            {
                const int cond = (wall[thewall[z]].point2 != thewall[bunchp2[z]] ||
                                  xb2[z] >= xb1[bunchp2[z]]);

                printf("%s%d(%d) ", cond ? "!" : "", z, thewall[z]);

                if (didscan[z>>3] & pow2char[z&7])
                {
                    printf("*");
                    break;
                }

                didscan[z>>3] |= pow2char[z&7];
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

static WSHELPER_DECL void tweak_tsizes(vec2_16_t *tsiz)
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

static WSHELPER_DECL void calc_bufplc(intptr_t *bufplc, int32_t lw, vec2_16_t tsiz)
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
	*bufplc = (intptr_t)tilePtr(globalpicnum) + i;
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
    *vplc = min<inthi_t>(max<inthi_t>(0, tmpvplc), UINT32_MAX);
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


//
// maskwallscan (internal)
//
static void maskwallscan(int32_t x1, int32_t x2, int32_t saturatevplc)
{
    if (globalshiftval < 0) return;
    if ((uwall[x1] > ydimen) && (uwall[x2] > ydimen)) return;
    if ((dwall[x1] < 0) && (dwall[x2] < 0)) return;

    auto tsiz = tilesiz[globalpicnum];
    if ((tsiz.x <= 0) || (tsiz.y <= 0)) return;

    setgotpic(globalpicnum);

    tileLoad(globalpicnum);

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
        y1ve[0] = max<int>(uwall[x],startumost[x+windowxy1.x]-windowxy1.y);
        y2ve[0] = min<int>(dwall[x],startdmost[x+windowxy1.x]-windowxy1.y);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup + getpalookupsh(mulscale16(swall[x],globvis));

        calc_bufplc(&bufplce[0], lwall[x], tsiz);
        calc_vplcinc(&vplce[0], &vince[0], swall.Data(), x, y1ve[0]);

        mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0],p+ylookup[y1ve[0]]);
    }
    for (; x<=x2-3; x+=4,p+=4)
    {
        char bad = 0;

        for (bssize_t z=3,dax=x+3; z>=0; z--,dax--)
        {
            y1ve[z] = max<int>(uwall[dax],startumost[dax+windowxy1.x]-windowxy1.y);
            y2ve[z] = min<int>(dwall[dax],startdmost[dax+windowxy1.x]-windowxy1.y)-1;
            if (y2ve[z] < y1ve[z]) { bad += pow2char[z]; continue; }

            calc_bufplc(&bufplce[z], lwall[dax], tsiz);
            calc_vplcinc(&vplce[z], &vince[z], swall.Data(), dax, y1ve[z]);
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
        y1ve[0] = max<int>(uwall[x],startumost[x+windowxy1.x]-windowxy1.y);
        y2ve[0] = min<int>(dwall[x],startdmost[x+windowxy1.x]-windowxy1.y);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup + getpalookupsh(mulscale16(swall[x],globvis));

        calc_bufplc(&bufplce[0], lwall[x], tsiz);
        calc_vplcinc(&vplce[0], &vince[0], swall.Data(), x, y1ve[0]);

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
    vec2_t const l1vect   = wall[thewall[l1]].pos;
    vec2_t const l1p2vect = wall[wall[thewall[l1]].point2].pos;
    vec2_t const l2vect   = wall[thewall[l2]].pos;
    vec2_t const l2p2vect = wall[wall[thewall[l2]].point2].pos;
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
static inline int32_t spritewallfront(tspritetype const * const s, int32_t w)
{
    auto const wal = (uwallptr_t)&wall[w];
    auto const wal2 = (uwallptr_t)&wall[wal->point2];
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
    asm1 = (inthi_t)mulscale6(globalx1, r);
    asm2 = (inthi_t)mulscale6(globaly2, r);
    int32_t const s = getpalookupsh(mulscale22(r,globvis));

    hlineasm4(xr-xl,0,s,(uint32_t)mulscale6(globalx2,r)+globalypanning,(uint32_t)mulscale6(globaly1,r)+globalxpanning,
              ylookup[yp]+xr+frameoffset);
}


//
// slowhline (internal)
//
static inline void slowhline(int32_t xr, int32_t yp)
{
    int32_t const xl = lastx[yp]; if (xl > xr) return;
    int32_t const r = horizlookup2[yp-globalhoriz+horizycent];
    asm1 = (inthi_t)mulscale6(globalx1, r);
    asm2 = (inthi_t)mulscale6(globaly2, r);

    asm3 = (intptr_t)globalpalwritten + getpalookupsh(mulscale22(r,globvis));
    if (!(globalorientation&256))
    {
        mhline(globalbufplc,(uint32_t)mulscale6(globaly1,r)+globalxpanning-asm1*(xr-xl),(xr-xl)<<16,0L,
               (uint32_t)mulscale6(globalx2,r)+globalypanning-asm2*(xr-xl),ylookup[yp]+xl+frameoffset);
        return;
    }
    thline(globalbufplc,(uint32_t)mulscale6(globaly1,r)+globalxpanning-asm1*(xr-xl),(xr-xl)<<16,0L,
           (uint32_t)mulscale6(globalx2,r)+globalypanning-asm2*(xr-xl),ylookup[yp]+xl+frameoffset);
}


//
// prepwall (internal)
//
static void prepwall(int32_t z, uwallptr_t wal)
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
int32_t (*animateoffs_replace)(int const tilenum, int fakevar) = NULL;
int32_t animateoffs(int const tilenum, int fakevar)
{
    if (animateoffs_replace)
    {
        return animateoffs_replace(tilenum, fakevar);
    }

    int const animnum = picanm[tilenum].num;

    if (animnum <= 0)
        return 0;

    int const i = (int) totalclocklock >> (picanm[tilenum].sf & PICANM_ANIMSPEED_MASK);
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
    ix1 = clamp(ix1, 0, xdim-1);
    ix2 = clamp(ix2, 0, xdim-1);
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

    auto const wal = (uwallptr_t)&wall[wi];
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

static int32_t setup_globals_cf1(usectorptr_t sec, int32_t pal, int32_t zd,
                                 int32_t picnum, int32_t shade, int32_t stat,
                                 int32_t xpanning, int32_t ypanning, int32_t x1)
{
    int32_t i;

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
    tileUpdatePicnum(&globalpicnum, 0);
    setgotpic(globalpicnum);
    if ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0)) return 1;
        tileLoad(globalpicnum);


    globalbufplc = (intptr_t)tilePtr(globalpicnum);

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
        vec2_t const xy = { wall[wall[sec->wallptr].point2].x - wall[sec->wallptr].x,
                            wall[wall[sec->wallptr].point2].y - wall[sec->wallptr].y };
        i = nsqrtasm(uhypsq(xy.x,xy.y)); if (i == 0) i = 1024; else i = tabledivide32(1048576, i);
        int const wcos = mulscale6(xy.x, i), wsin = mulscale6(xy.y, i);
        globalx1 = dmulscale14(wcos,singlobalang,-wsin,cosglobalang);
        globaly1 = dmulscale14(wcos,cosglobalang,wsin,singlobalang);
        globalx2 = -globalx1;
        globaly2 = -globaly1;

        globalxpanning = (coord_t)((globalposx - wall[sec->wallptr].x)<<6) * wcos + (coord_t)((globalposy - wall[sec->wallptr].y)<<6) * wsin;
        globalypanning = (coord_t)((globalposy - wall[sec->wallptr].y)<<6) * wcos - (coord_t)((globalposx - wall[sec->wallptr].x)<<6) * wsin;
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
    auto const sec = (usectorptr_t)&sector[sectnum];

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
        setup_blend(0, 1);
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
     auto const sec = (usectorptr_t)&sector[sectnum];

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
    vec2_16_t tsiz;
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

        tileLoad(globalpicnum);


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

    int32_t const y1v = max<int>(uwall[x],startumost[x+windowxy1.x]-windowxy1.y);
    int32_t const y2v = min<int>(dwall[x],startdmost[x+windowxy1.x]-windowxy1.y) - 1;

    if (y2v < y1v) return;

    intptr_t palookupoffs = FP_OFF(palookup[globalpal]) + getpalookupsh(mulscale16(swall[x],globvis));

    vec2_16_t const ntsiz = { (int16_t)-tilesiz[globalpicnum].x, (int16_t)-tilesiz[globalpicnum].y };
    intptr_t bufplc;
    calc_bufplc(&bufplc, lwall[x], ntsiz);
    uint32_t vplc;
    int32_t vinc;
    calc_vplcinc(&vplc, &vinc, swall.Data(), x, y1v);

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

    y1ve[0] = max<int>(uwall[x],startumost[x+windowxy1.x]-windowxy1.y);
    y2ve[0] = min<int>(dwall[x],startdmost[x+windowxy1.x]-windowxy1.y)-1;
    if (y2ve[0] < y1ve[0]) { transmaskvline(x2); return; }
    y1ve[1] = max<int>(uwall[x2],startumost[x2+windowxy1.x]-windowxy1.y);
    y2ve[1] = min<int>(dwall[x2],startdmost[x2+windowxy1.x]-windowxy1.y)-1;
    if (y2ve[1] < y1ve[1]) { transmaskvline(x); return; }

    palookupoffse[0] = FP_OFF(palookup[globalpal]) + getpalookupsh(mulscale16(swall[x],globvis));
    palookupoffse[1] = FP_OFF(palookup[globalpal]) + getpalookupsh(mulscale16(swall[x2],globvis));

    setuptvlineasm2(globalshiftval,palookupoffse[0],palookupoffse[1]);

    vec2_16_t const ntsiz = { (int16_t)-tilesiz[globalpicnum].x, (int16_t)-tilesiz[globalpicnum].y };

    calc_bufplc(&bufplce[0], lwall[x], ntsiz);
    calc_bufplc(&bufplce[1], lwall[x2], ntsiz);
    calc_vplcinc(&vplce[0], &vince[0], swall.Data(), x, y1ve[0]);
    calc_vplcinc(&vplce[1], &vince[1], swall.Data(), x2, y1ve[1]);

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

        tileLoad(globalpicnum);


    setuptvlineasm(globalshiftval, saturatevplc);

    int32_t x = x1;
    while ((x <= x2) && (startumost[x+windowxy1.x] > startdmost[x+windowxy1.x]))
        ++x;

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

    const uint32_t xmul = globalxspan;
    const uint32_t ymul = globalyspan;
    const uint32_t yspan = globalyspan;
    const int32_t xinc = asm1, yinc = asm2;

    for (cntup16>>=16; cntup16>0; cntup16--)
    {
        ch = buf[mulscale31(bx>>1, xmul)*yspan + mulscale31(by>>1, ymul)];

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
    const char *const A_C_RESTRICT trans = paletteGetBlendTable(globalblend);

    const uint32_t xmul = globalxspan;
    const uint32_t ymul = globalyspan;
    const uint32_t yspan = globalyspan;
    const int32_t xinc = asm1, yinc = asm2;

    if (globalorientation&512)
    {
        for (cntup16>>=16; cntup16>0; cntup16--)
        {
            ch = buf[mulscale31(bx>>1, xmul)*yspan + mulscale31(by>>1, ymul)];
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
            ch = buf[mulscale31(bx>>1, xmul)*yspan + mulscale31(by>>1, ymul)];
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
    sethlinesizes(logylogx&255, logylogx>>8, bufplc);
    gpinc = pinc;
    gglogx = (logylogx&255); gglogy = (logylogx>>8);
    ggbuf = (char *)bufplc; ggpinc = pinc;
    ggpal = palookup[globalpal] + getpalookupsh(0);
}

// cnt iterations
static void tslopevlin(uint8_t *p, const intptr_t *slopalptr, bssize_t cnt, int32_t bx, int32_t by)
{
    const char *const A_C_RESTRICT buf = ggbuf;
    const char *const A_C_RESTRICT trans = paletteGetBlendTable(0);
    const int32_t bzinc = (asm1>>3), pinc = ggpinc;

    const int32_t transmode = (globalorientation&128);
    const uint32_t xtou = globalx3, ytov = globaly3;
    const int32_t logx = gglogx, logy = gglogy;

    int32_t bz = asm3;

    do
    {
        int const i = (sloptable[(bz>>6)+HALFSLOPTABLESIZ]); bz += bzinc;
        uint32_t u = bx + xtou*i;
        uint32_t v = by + ytov*i;
        uint8_t ch = buf[((u>>(32-logx))<<logy)+(v>>(32-logy))];
        if (ch != 255)
        {
            ch = *(uint8_t *)(slopalptr[0] + ch);
            *p = trans[transmode ? *p|(ch<<8) : (*p<<8)|ch];
        }

        slopalptr--;
        p += pinc;
    }
    while (--cnt);
}

// cnt iterations
static void mslopevlin(uint8_t *p, const intptr_t *slopalptr, bssize_t cnt, int32_t bx, int32_t by)
{
    const char *const A_C_RESTRICT buf = ggbuf;
    const int32_t bzinc = (asm1>>3), pinc = ggpinc;

    const uint32_t xtou = globalx3, ytov = globaly3;
    const int32_t logx = gglogx, logy = gglogy;

    int32_t bz = asm3;

    do
    {
        int const i = (sloptable[(bz>>6)+HALFSLOPTABLESIZ]); bz += bzinc;
        uint32_t u = bx + xtou*i;
        uint32_t v = by + ytov*i;
        uint8_t ch = buf[((u>>(32-logx))<<logy)+(v>>(32-logy))];
        if (ch != 255)
            *p = *(uint8_t *)(slopalptr[0] + ch);

        slopalptr--;
        p += pinc;
    }
    while (--cnt);
}

//
// grouscan (internal)
//
#define BITSOFPRECISION 3  //Don't forget to change this in A.ASM also!
static void fgrouscan(int32_t dax1, int32_t dax2, int32_t sectnum, char dastat)
{
    int32_t i, j, l, globalx1, globaly1, y1, y2, daslope, daz, wxi, wyi;
    float fi, wx, wy, dasqr;
    float globalx, globaly, globalx2, globaly2, globalx3, globaly3, globalz, globalzd, globalzx;
    int32_t shoffs, m1, m2, shy1, shy2;
    intptr_t *mptr1, *mptr2;

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

    tileUpdatePicnum(&globalpicnum, sectnum);
    setgotpic(globalpicnum);
    if ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0)) return;
        tileLoad(globalpicnum);


    wal = (uwalltype *)&wall[sec->wallptr];
    wxi = wall[wal->point2].x - wal->x;
    wyi = wall[wal->point2].y - wal->y;
    dasqr = 1073741824.f/nsqrtasm(uhypsq(wxi,wyi));
    fi = daslope*dasqr*(1.f/2097152.f);
    wx = wxi*fi; wy = wyi*fi;

    globalx = -float(singlobalang)*float(xdimenrecip)*(1.f/524288.f);
    globaly = float(cosglobalang)*float(xdimenrecip)*(1.f/524288.f);
    globalx1 = globalposx<<8;
    globaly1 = -globalposy<<8;
    fi = (dax1-halfxdimen)*xdimenrecip;
    globalx2 = float(cosglobalang)*float(viewingrangerecip)*(1.f/4096.f) - float(singlobalang)*fi*(1.f/134217728.f);
    globaly2 = float(singlobalang)*float(viewingrangerecip)*(1.f/4096.f) + float(cosglobalang)*fi*(1.f/134217728.f);
    globalzd = xdimscale*512.f;
    globalzx = -(wx*globaly2-wy*globalx2)*(1.f/131072.f) + (1-globalhoriz)*globalzd*(1.f/1024.f);
    globalz = -(wx*globaly-wy*globalx)*(1.f/33554432.f);

    if (globalorientation&64)  //Relative alignment
    {
        float dx, dy, x, y;
        dx = (wall[wal->point2].x-wal->x)*dasqr*(1.f/16384.f);
        dy = (wall[wal->point2].y-wal->y)*dasqr*(1.f/16384.f);

        fi = float(nsqrtasm(daslope*daslope+16777216));

        x = globalx; y = globaly;
        globalx = (x*dx+y*dy)*(1.f/65536.f);
        globaly = (-y*dx+x*dy)*fi*(1.f/268435456.f);

        x = (wal->x-globalposx)*256.f; y = (wal->y-globalposy)*256.f;
        globalx1 = Blrintf((-x*dx-y*dy)*(1.f/65536.f));
        globaly1 = Blrintf((-y*dx+x*dy)*fi*(1.f/268435456.f));

        x = globalx2; y = globaly2;
        globalx2 = (x*dx+y*dy)*(1.f/65536.f);
        globaly2 = (-y*dx+x*dy)*fi*(1.f/268435456.f);
    }
    if (globalorientation&0x4)
    {
        fi = globalx; globalx = -globaly; globaly = -fi;
        i = globalx1; globalx1 = globaly1; globaly1 = i;
        fi = globalx2; globalx2 = -globaly2; globaly2 = -fi;
    }
    if (globalorientation&0x10) { globalx1 = -globalx1, globalx2 = -globalx2, globalx = -globalx; }
    if (globalorientation&0x20) { globaly1 = -globaly1, globaly2 = -globaly2, globaly = -globaly; }

    float fdaz = (wx*(globalposy-wal->y)-wy*(globalposx-wal->x))*(1.f/512.f) + (daz-globalposz)*256.f;
    globalx2 = (globalx2*fdaz)*(1.f/1048576.f); globalx = (globalx*fdaz)*(1.f/268435456.f);
    globaly2 = (globaly2*-fdaz)*(1.f/1048576.f); globaly = (globaly*-fdaz)*(1.f/268435456.f);

    i = 8-(picsiz[globalpicnum]&15); j = 8-(picsiz[globalpicnum]>>4);
    if (globalorientation&8) { i++; j++; }
    globalx1 <<= (i+12); globalx2 *= 1<<i; globalx *= 1<<i;
    globaly1 <<= (j+12); globaly2 *= 1<<j; globaly *= 1<<j;

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

    globalx1 >>= 16;
    globaly1 >>= 16;

    //asm1 = -(globalzd>>(16-BITSOFPRECISION));
    float bzinc = -globalzd*(1.f/65536.f);

    {
        int32_t vis = globalvisibility;
        int64_t lvis;

        if (sec->visibility != 0) vis = mulscale4(vis, (uint8_t)(sec->visibility+16));
        lvis = ((uint64_t)(vis*fdaz)) >> 13;  // NOTE: lvis can be negative now!
        lvis = (lvis * xdimscale) >> 16;
        globvis = lvis;
    }

    intptr_t fj = FP_OFF(palookup[globalpal]);

    setupslopevlin_alsotrans((picsiz[globalpicnum]&15) + ((picsiz[globalpicnum]>>4)<<8),
                             (intptr_t)tilePtr(globalpicnum),-ylookup[1]);

    l = Blrintf((globalzd)*(1.f/65536.f));

    int32_t const shinc = Blrintf(globalz*xdimenscale*(1.f/65536.f));

    shoffs = (shinc > 0) ? (4 << 15) : ((16380 - ydimen) << 15);  // JBF: was 2044
    y1     = (dastat == 0) ? umost[dax1] : max(umost[dax1], dplc[dax1]);

    m1 = Blrintf((y1*globalzd)*(1.f/65536.f) + globalzx*(1.f/64.f));
    //Avoid visibility overflow by crossing horizon
    m1 += klabs(l);
    m2 = m1+l;
    shy1 = y1+(shoffs>>15);
    if ((unsigned)shy1 >= SLOPALOOKUPSIZ-1)
    {
        OSD_Printf("%s:%d: slopalookup[%" PRId32 "] overflow drawing sector %d!\n", EDUKE32_FUNCTION, __LINE__, shy1, sectnum);
        return;
    }

    mptr1 = &slopalookup[shy1]; mptr2 = mptr1+1;

    for (int x=dax1; x<=dax2; x++)
    {
        if (dastat == 0) { y1 = umost[x]; y2 = min(dmost[x],uplc[x])-1; }
        else { y1 = max(umost[x],dplc[x]); y2 = dmost[x]-1; }
        if (y1 <= y2)
        {
            shy1 = y1+(shoffs>>15);
            shy2 = y2+(shoffs>>15);

            // Ridiculously steep gradient?
            if ((unsigned)shy1 >= SLOPALOOKUPSIZ)
            {
                OSD_Printf("%s:%d: slopalookup[%" PRId32 "] overflow drawing sector %d!\n", EDUKE32_FUNCTION, __LINE__, shy1, sectnum);
                goto next_most;
            }
            if ((unsigned)shy2 >= SLOPALOOKUPSIZ)
            {
                OSD_Printf("%s:%d: slopalookup[%" PRId32 "] overflow drawing sector %d!\n", EDUKE32_FUNCTION, __LINE__, shy2, sectnum);
                goto next_most;
            }

            intptr_t *nptr1 = &slopalookup[shy1];
            intptr_t *nptr2 = &slopalookup[shy2];

            while (nptr1 <= mptr1)
            {
                *mptr1-- = fj + getpalookupsh(mulscale24(krecipasm(m1),globvis));
                m1 -= l;
            }
            while (nptr2 >= mptr2)
            {
                *mptr2++ = fj + getpalookupsh(mulscale24(krecipasm(m2),globvis));
                m2 += l;
            }

            globalx3 = globalx2*(1.f/1024.f);
            globaly3 = globaly2*(1.f/1024.f);
            float bz = (y2*globalzd)*(1.f/65536.f) + globalzx*(1.f/64.f);
            uint8_t *p = (uint8_t*)(ylookup[y2]+x+frameoffset);
            intptr_t* A_C_RESTRICT slopalptr = (intptr_t*)nptr2;
            const char* const A_C_RESTRICT trans = paletteGetBlendTable(0);
            uint32_t u, v;
            int cnt = y2-y1+1;
#define LINTERPSIZ 4
            int u0 = Blrintf(1048576.f*globalx3/bz);
            int v0 = Blrintf(1048576.f*globaly3/bz);
            switch (globalorientation&0x180)
            {
            case 0:
                while (cnt > 0)
                {
                    bz += bzinc*(1<<LINTERPSIZ);
                    int u1 = Blrintf(1048576.f*globalx3/bz);
                    int v1 = Blrintf(1048576.f*globaly3/bz);
                    u1 = (u1-u0)>>LINTERPSIZ;
                    v1 = (v1-v0)>>LINTERPSIZ;
                    int cnt2 = min(cnt, 1<<LINTERPSIZ);
                    for (; cnt2>0; cnt2--)
                    {
                        u = (globalx1+u0)&0xffff;
                        v = (globaly1+v0)&0xffff;
                        *p = *(uint8_t *)(((intptr_t)slopalptr[0])+ggbuf[((u>>(16-gglogx))<<gglogy)+(v>>(16-gglogy))]);
                        slopalptr--;
                        p += ggpinc;
                        u0 += u1;
                        v0 += v1;
                    }
                    cnt -= 1<<LINTERPSIZ;
                }
                break;
            case 128:
                while (cnt > 0)
                {
                    bz += bzinc*(1<<LINTERPSIZ);
                    int u1 = Blrintf(1048576.f*globalx3/bz);
                    int v1 = Blrintf(1048576.f*globaly3/bz);
                    u1 = (u1-u0)>>LINTERPSIZ;
                    v1 = (v1-v0)>>LINTERPSIZ;
                    int cnt2 = min(cnt, 1<<LINTERPSIZ);
                    for (; cnt2>0; cnt2--)
                    {
                        u = (globalx1+u0)&0xffff;
                        v = (globaly1+v0)&0xffff;
                        uint8_t ch = ggbuf[((u>>(16-gglogx))<<gglogy)+(v>>(16-gglogy))];
                        if (ch != 255)
                            *p = *(uint8_t *)(((intptr_t)slopalptr[0])+ch);
                        slopalptr--;
                        p += ggpinc;
                        u0 += u1;
                        v0 += v1;
                    }
                    cnt -= 1<<LINTERPSIZ;
                }
                break;
            case 256:
                while (cnt > 0)
                {
                    bz += bzinc*(1<<LINTERPSIZ);
                    int u1 = Blrintf(1048576.f*globalx3/bz);
                    int v1 = Blrintf(1048576.f*globaly3/bz);
                    u1 = (u1-u0)>>LINTERPSIZ;
                    v1 = (v1-v0)>>LINTERPSIZ;
                    int cnt2 = min(cnt, 1<<LINTERPSIZ);
                    for (; cnt2>0; cnt2--)
                    {
                        u = (globalx1+u0)&0xffff;
                        v = (globaly1+v0)&0xffff;
                        uint8_t ch = ggbuf[((u>>(16-gglogx))<<gglogy)+(v>>(16-gglogy))];
                        if (ch != 255)
                        {
                            ch = *(uint8_t *)(((intptr_t)slopalptr[0])+ch);
                            *p = trans[(*p<<8)|ch];
                        }
                        slopalptr--;
                        p += ggpinc;
                        u0 += u1;
                        v0 += v1;
                    }
                    cnt -= 1<<LINTERPSIZ;
                }
                break;
            case 384:
                while (cnt > 0)
                {
                    bz += bzinc*(1<<LINTERPSIZ);
                    int u1 = Blrintf(1048576.f*globalx3/bz);
                    int v1 = Blrintf(1048576.f*globaly3/bz);
                    u1 = (u1-u0)>>LINTERPSIZ;
                    v1 = (v1-v0)>>LINTERPSIZ;
                    int cnt2 = min(cnt, 1<<LINTERPSIZ);
                    for (; cnt2>0; cnt2--)
                    {
                        u = (globalx1+u0)&0xffff;
                        v = (globaly1+v0)&0xffff;
                        uint8_t ch = ggbuf[((u>>(16-gglogx))<<gglogy)+(v>>(16-gglogy))];
                        if (ch != 255)
                        {
                            ch = *(uint8_t *)(((intptr_t)slopalptr[0])+ch);
                            *p = trans[ch<<8|*p];
                        }
                        slopalptr--;
                        p += ggpinc;
                        u0 += u1;
                        v0 += v1;
                    }
                    cnt -= 1<<LINTERPSIZ;
                }
                break;
            }
#undef LINTERPSIZ
            if ((x&15) == 0) faketimerhandler();
        }
next_most:
        globalx2 += globalx;
        globaly2 += globaly;
        globalzx += globalz;
        shoffs += shinc;
    }
}

static void grouscan(int32_t dax1, int32_t dax2, int32_t sectnum, char dastat)
{
    if (r_fpgrouscan)
    {
        fgrouscan(dax1, dax2, sectnum, dastat);
        return;
    }
    int32_t i, l, x, y, dx, dy, wx, wy, y1, y2, daz;
    int32_t daslope, dasqr;
    int32_t shoffs, m1, m2, shy1, shy2;
    intptr_t *mptr1, *mptr2, j;

    // Er, yes, they're not global anymore:
    int32_t globalx, globaly, globalz, globalzx;

    auto const sec = (usectorptr_t)&sector[sectnum];
    uwallptr_t wal;

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

    tileUpdatePicnum(&globalpicnum, sectnum);
    setgotpic(globalpicnum);
    if ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0)) return;
        tileLoad(globalpicnum);


    wal = (uwallptr_t)&wall[sec->wallptr];
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
    globalzd = decltype(globalzd)(xdimscale)<<9;
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
                             (intptr_t)tilePtr(globalpicnum),-ylookup[1]);

    l = (globalzd>>16);

    int32_t const shinc = mulscale16(globalz,xdimenscale);

    shoffs = (shinc > 0) ? (4 << 15) : ((16380 - ydimen) << 15);  // JBF: was 2044
    y1     = (dastat == 0) ? umost[dax1] : max(umost[dax1], dplc[dax1]);

    m1 = mulscale16(y1,globalzd) + (globalzx>>6);
    //Avoid visibility overflow by crossing horizon
    m1 += klabs((int32_t) (globalzd>>16));
    m2 = m1+l;
    shy1 = y1+(shoffs>>15);
    if ((unsigned)shy1 >= SLOPALOOKUPSIZ - 1)
    {
        OSD_Printf("%s:%d: slopalookup[%" PRId32 "] overflow drawing sector %d!\n", EDUKE32_FUNCTION, __LINE__, shy1, sectnum);
        return;
    }

    mptr1 = &slopalookup[shy1]; mptr2 = mptr1+1;

    for (x=dax1; x<=dax2; x++)
    {
        if (dastat == 0) { y1 = umost[x]; y2 = min(dmost[x],uplc[x])-1; }
        else { y1 = max(umost[x],dplc[x]); y2 = dmost[x]-1; }
        if (y1 <= y2)
        {
            shy1 = y1+(shoffs>>15);
            shy2 = y2+(shoffs>>15);

            // Ridiculously steep gradient?
            if ((unsigned)shy1 >= SLOPALOOKUPSIZ)
            {
                OSD_Printf("%s:%d: slopalookup[%" PRId32 "] overflow drawing sector %d!\n", EDUKE32_FUNCTION, __LINE__, shy1, sectnum);
                goto next_most;
            }
            if ((unsigned)shy2 >= SLOPALOOKUPSIZ)
            {
                OSD_Printf("%s:%d: slopalookup[%" PRId32 "] overflow drawing sector %d!\n", EDUKE32_FUNCTION, __LINE__, shy2, sectnum);
                goto next_most;
            }

            intptr_t *nptr1 = &slopalookup[shy1];
            intptr_t *nptr2 = &slopalookup[shy2];

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
            switch (globalorientation&0x180)
            {
            case 0:
                slopevlin(ylookup[y2]+x+frameoffset,krecipasm(asm3>>3),(intptr_t)nptr2,y2-y1+1,globalx1,globaly1);
                break;
            case 128:
                mslopevlin((uint8_t *)(ylookup[y2]+x+frameoffset),nptr2,y2-y1+1,globalx1,globaly1);
                break;
            case 256:
            case 384:
                tslopevlin((uint8_t *)(ylookup[y2]+x+frameoffset),nptr2,y2-y1+1,globalx1,globaly1);
                break;
            }

            if ((x&15) == 0) faketimerhandler();
        }
next_most:
        globalx2 += globalx;
        globaly2 += globaly;
        globalzx += globalz;
        shoffs += shinc;
    }
}


//
// parascan (internal)
//
static void parascan(char dastat, int32_t bunch)
{
    int32_t j, k, l, m, n, x, z, wallnum, nextsectnum, globalhorizbak;
    int16_t *topptr, *botptr;

    int32_t logtilesizy, tsizy;

    int32_t sectnum = thesector[bunchfirst[bunch]];
    auto const sec = (usectorptr_t)&sector[sectnum];

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
        topptr = umost.Data();
        botptr = uplc.Data();
    }
    else
    {
        globalpal = sec->floorpal;
        globalpicnum = sec->floorpicnum;
        globalshade = (int32_t)sec->floorshade;
        globalxpanning = (int32_t)sec->floorxpanning;
        globalypanning = (int32_t)sec->floorypanning;
        topptr = dplc.Data();
        botptr = dmost.Data();
    }

    if ((unsigned)globalpicnum >= MAXTILES) globalpicnum = 0;
    tileUpdatePicnum(&globalpicnum, sectnum);
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
        globalzd = (decltype(globalzd)((tsizy >> 1) + dapyoffs) << globalshiftval) + (decltype(globalzd)(globalypanning) << 24);
    }
    globalyscale = divscale16(globalyscale,daptileyscale);

    //if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;

    if (dapyscale != 65536)
        globalhoriz = mulscale16(globalhoriz-(ydimen>>1),dapyscale) + (ydimen>>1);

    k = 27 - (picsiz[globalpicnum]&15) - dapskybits;

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
                    lplc[j] = ((mulscale7(j-halfxdimen,n)+qglobalang)&0x7FFFFFF)>>k;
            }
            else
            {
                for (j=xb1[z]; j<=xb2[z]; j++)
                    lplc[j] = ((radarang2[j]+qglobalang)&0x7FFFFFF)>>k;
            }

            if (parallaxtype == 2 && !no_radarang2)
            {
                n = mulscale16(xdimscale,viewingrange);
                for (j=xb1[z]; j<=xb2[z]; j++)
                    swplc[j] = mulscale14(sintable[((radarang2[j]>>16)+512)&2047],n);
            }
            else
                clearbuf(&swplc[xb1[z]],xb2[z]-xb1[z]+1,mulscale16(xdimscale,viewingrange));
        }
        else if (x >= 0)
        {
            l = globalpicnum; m = (picsiz[globalpicnum]&15);
            globalpicnum = l + dapskyoff[lplc[x]>>m];

            if (((lplc[x]^lplc[xb1[z]-1])>>m) == 0)
                wallscan(x,xb1[z]-1,topptr,botptr,swplc.Data(),lplc.Data());
            else
            {
                j = x;
                while (x < xb1[z])
                {
                    n = l + dapskyoff[lplc[x]>>m];
                    if (n != globalpicnum)
                    {
                        wallscan(j,x-1,topptr,botptr,swplc.Data(),lplc.Data());
                        j = x;
                        globalpicnum = n;
                    }
                    x++;
                }
                if (j < x)
                    wallscan(j,x-1,topptr,botptr,swplc.Data(),lplc.Data());
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
            wallscan(x,xb2[bunchlast[bunch]],topptr,botptr,swplc.Data(),lplc.Data());
        else
        {
            j = x;
            while (x <= xb2[bunchlast[bunch]])
            {
                n = l + dapskyoff[lplc[x]>>m];
                if (n != globalpicnum)
                {
                    wallscan(j,x-1,topptr,botptr,swplc.Data(),lplc.Data());
                    j = x;
                    globalpicnum = n;
                }
                x++;
            }
            if (j <= x)
                wallscan(j,x-1,topptr,botptr,swplc.Data(),lplc.Data());
        }
        globalpicnum = l;
    }
    globalhoriz = globalhorizbak;
}


// set orientation, panning, shade, pal; picnum
static void setup_globals_wall1(uwallptr_t wal, int32_t dapicnum)
{
    globalorientation = wal->cstat;

    globalpicnum = dapicnum;
    if ((unsigned)globalpicnum >= MAXTILES) globalpicnum = 0;
    tileUpdatePicnum(&globalpicnum, 16384);

    globalxpanning = wal->xpanning;
    globalypanning = wal->ypanning;

    globalshade = wal->shade;
    globalpal = wal->pal;
    if (palookup[globalpal] == NULL) globalpal = 0;    // JBF: fixes crash
}

static void setup_globals_wall2(uwallptr_t wal, uint8_t secvisibility, int32_t topzref, int32_t botzref)
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

    globalzd += decltype(globalzd)(globalypanning) << 24;

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
static void classicDrawBunches(int32_t bunch)
{
    int32_t i, x;

    int32_t z = bunchfirst[bunch];

    const int32_t sectnum = thesector[z];
    auto const sec = (usectorptr_t)&sector[sectnum];

    uint8_t andwstat1 = 0xff, andwstat2 = 0xff;

    for (; z>=0; z=bunchp2[z]) //uplc/dplc calculation
    {
        andwstat1 &= wallmost(uplc.Data(),z,sectnum,(uint8_t)0);
        andwstat2 &= wallmost(dplc.Data(),z,sectnum,(uint8_t)1);
    }

#ifdef YAX_ENABLE
    if (g_nodraw)
    {
        int32_t baselevp, checkcf;
        int16_t bn[2];
# if 0
        int32_t obunchchk = (1 && yax_globalbunch>=0 &&
                             haveymost[yax_globalbunch>>3]&pow2char[yax_globalbunch&7]);

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
                if ((haveymost[bn[i]>>3]&pow2char[bn[i]&7])==0)
                {
                    // init yax *most arrays for that bunch
                    haveymost[bn[i]>>3] |= pow2char[bn[i]&7];
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
                yax_nomaskpass==1 || !(yax_gotsector[sectnum>>3]&pow2char[sectnum&7]))
#endif
        {
            if ((sec->ceilingstat&3) == 2)
                grouscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum,0);
            else if ((sec->ceilingstat&1) == 0)
                ceilscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum);
            else
                parascan(0,bunch);
        }

        if ((andwstat2&12) != 12)   //draw floors
#ifdef YAX_ENABLE
            // this is to prevent double-drawing of translucent masked floors
            if (r_tror_nomaskpass==0 || yax_globallev==YAX_MAXDRAWS || (sec->floorstat&256)==0 ||
                yax_nomaskpass==1 || !(yax_gotsector[sectnum>>3]&pow2char[sectnum&7]))
#endif
        {
            if ((sec->floorstat&3) == 2)
                grouscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum,1);
            else if ((sec->floorstat&1) == 0)
                florscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum);
            else
                parascan(1,bunch);
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
        auto const wal = (uwallptr_t)&wall[wallnum];

        const int32_t nextsectnum = wal->nextsector;
        auto const nextsec = nextsectnum>=0 ? (usectorptr_t)&sector[nextsectnum] : NULL;

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
                    wallmost(dwall.Data(),z,nextsectnum,(uint8_t)0);

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

                    gotswall = 1;
                    prepwall(z,wal);
                    wallscan(x1,x2,uplc.Data(),dwall.Data(),swall.Data(),lwall.Data());

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
                    wallmost(uwall.Data(),z,nextsectnum,(uint8_t)1);

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

                    auto const twal = (wal->cstat&2) ? (uwallptr_t)&wall[wal->nextwall] : wal;
                    setup_globals_wall1(twal, twal->picnum);

                    setup_globals_wall2(wal, sec->visibility, nextsec->floorz, sec->ceilingz);

                    if (gotswall == 0) { gotswall = 1; prepwall(z,wal); }
                    wallscan(x1,x2,uwall.Data(),dplc.Data(),swall.Data(),lwall.Data());

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
                    classicScanSector(nextsectnum);
                else
                {
                    for (x=x1; x<x2; x++)
                        if (umost[x] < dmost[x])
                            { classicScanSector(nextsectnum); break; }

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
            wallscan(x1,x2,uplc.Data(),dplc.Data(),swall.Data(),lwall.Data());

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
    }
}

// High-precision integer type for view-relative x and y in drawvox().
typedef zint_t voxint_t;

//
// drawvox
//
static void classicDrawVoxel(int32_t dasprx, int32_t daspry, int32_t dasprz, int32_t dasprang,
                             int32_t daxscale, int32_t dayscale, int32_t daindex,
                             int8_t dashade, char dapal, const int32_t *daumost, const int32_t *dadmost,
                             const int8_t cstat, const int32_t clipcf, int32_t floorz, int32_t ceilingz)
{
    int32_t i, j, k, x, y, mip;

    int32_t cosang = cosglobalang;
    int32_t sinang = singlobalang;
    int32_t sprcosang = sintable[(dasprang+512)&2047];
    int32_t sprsinang = sintable[dasprang&2047];

    i = klabs(dmulscale6(dasprx-globalposx, cosang, daspry-globalposy, sinang));
    j = getpalookup(mulscale21(globvis,i), dashade)<<8;
    setupdrawslab(ylookup[1], FP_OFF(palookup[dapal])+j);

    j = 1310720;
    //j *= min(daxscale,dayscale); j >>= 6;  //New hacks (for sized-down voxels)
    for (k=0; k<MAXVOXMIPS; k++)
    {
        if (i < j) { i = k; break; }
        j <<= 1;
    }
    if (k >= MAXVOXMIPS)
        i = MAXVOXMIPS-1;

    mip = 0;

    if (novoxmips)
    {
        mip = i;
        i = 0;
    }

    char *davoxptr = (char *)voxoff[daindex][i];
    if (!davoxptr && i > 0) { davoxptr = (char *)voxoff[daindex][0]; mip = i; i = 0;}
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

    int32_t *longptr = (int32_t *)davoxptr;
    const int32_t daxsiz = B_LITTLE32(longptr[0]), daysiz = B_LITTLE32(longptr[1]), dazsiz = B_LITTLE32(longptr[2]);
    int32_t daxpivot = B_LITTLE32(longptr[3]), daypivot = B_LITTLE32(longptr[4]), dazpivot = B_LITTLE32(longptr[5]);
    if (cstat & 4) daxpivot = (daxsiz<<8)-daxpivot;
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

    cosang <<= 2;
    sinang <<= 2;

    cosang >>= mip;
    sinang >>= mip;

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

    int32_t zoff = dazsiz<<14;
    if (!(cstat & 128))
        zoff += dazpivot<<7;
    else if ((cstat&48) != 48)
    {
        zoff += dazpivot<<7;
        zoff -= dazsiz<<14;
    }

    const int32_t syoff = divscale21(globalposz-dasprz,odayscale)+zoff;
    floorz = min(floorz, dasprz+mulscale21(-zoff+(dazsiz<<15),odayscale));
    ceilingz = max(ceilingz, dasprz+mulscale21(-zoff, odayscale));
    const int32_t flooroff = divscale21(floorz-globalposz,odayscale);
    const int32_t ceilingoff = divscale21(ceilingz-globalposz,odayscale);
    int32_t yoff = (klabs(gxinc)+klabs(gyinc))>>1;
    longptr = (int32_t *)davoxptr;
    int32_t xyvoxoffs = (daxsiz+1)<<2;

    videoBeginDrawing(); //{{{

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

        char oand = pow2char[(xs<backx)+0] + pow2char[(ys<backy)+2];

        if (cstat&4)
            oand ^= 3;

        char oand16 = oand+16;
        char oand32 = oand+32;

        if (cstat&8)
        {
            oand16 = oand+32;
            oand32 = oand+16;
        }

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
            const int32_t xf = (cstat & 4) ? daxsiz-1-x : x;
            const intptr_t slabxoffs = (intptr_t)&davoxptr[B_LITTLE32(longptr[xf])];
            int16_t *const shortptr = (int16_t *)&davoxptr[((xf*(daysiz+1))<<1) + xyvoxoffs];

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

                const int32_t l1 = mulscale(distrecip[clamp((ny-yoff)>>14, 1, DISTRECIPSIZ-1)], dayscale, 12+mip);
                // FIXME! AMCTC RC2/beta shotgun voxel
                // (e.g. training map right after M16 shooting):
                const int32_t l2 = mulscale(distrecip[clamp((ny+yoff)>>14, 1, DISTRECIPSIZ-1)], dayscale, 12+mip);
                int32_t cz1 = 0, cz2 = INT32_MAX;

                if (clipcf)
                {
                    cz1 = mulscale32((ceilingoff < 0) ? l1 : l2, ceilingoff) + globalhoriz;
                    cz2 = mulscale32((flooroff < 0) ? l2 : l1, flooroff) + globalhoriz;
                }

                for (; voxptr<voxend; voxptr+=voxptr[1]+3)
                {
                    if (cstat&8)
                        j = dazsiz-voxptr[0]-voxptr[1];
                    else
                        j = voxptr[0];
                    j = (j<<15)-syoff;

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

                    const int32_t um = max(daumost[lx], cz1);
                    const int32_t dm = min(dadmost[lx], cz2);
                    if (voxptr[1] == 1)
                    {
                        yplc = 0; yinc = 0;
                        if (z1 < um)
                            z1 = um;
                    }
                    else
                    {
                        if (z2-z1 >= 1024)
                            yinc = divscale16(voxptr[1], z2-z1);
                        else if (z2 > z1)
                            yinc = lowrecip[z2-z1]*voxptr[1]>>8;

                        if (z1 < um) { yplc = yinc*(um-z1); z1 = um; }
                        else yplc = 0;
                        
                        if (cstat & 8)
                            yinc = -yinc;
                        if (cstat & 8)
                            yplc = ((voxptr[1])<<16) - yplc + yinc;
                    }

                    if (z2 > dm)
                        z2 = dm;
                    z2 -= z1;
                    if (z2 <= 0)
                        continue;

                    drawslab(rx, yplc, z2, yinc, (intptr_t)&voxptr[3], ylookup[z1]+lx+frameoffset);
                }
            }
        }
    }


    videoEndDrawing();   //}}}
}


static void setup_globals_sprite1(tspriteptr_t tspr, usectorptr_t sec,
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

static void classicDrawSprite(int32_t snum)
{
    auto const tspr = tspriteptr[snum];
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

    auto const sec = (usectorptr_t)&sector[sectnum];

    int32_t cstat=tspr->cstat, tilenum;

    if ((cstat&48) != 48)
        tileUpdatePicnum(&tspr->picnum, spritenum+32768);

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

            // Blood's transparency table is inverted
            if (playing_blood)
                cstat ^= 512;
        }

        tspr->cstat = cstat;
    }

    tilenum = tspr->picnum;

    if ((cstat&48)==48)
        vtilenum = tilenum; // if the game wants voxels, it gets voxels
    else if ((cstat & 48) != 32 && r_voxels && tiletovox[tilenum] != -1 && spritenum != -1 && !(spriteext[spritenum].flags&SPREXT_NOTMD))
    {
        vtilenum = tiletovox[tilenum];
        cstat |= 48;
    }

    if ((cstat&48) != 48)
    {
        if (spritenum < 0 || tilesiz[tilenum].x <= 0 || tilesiz[tilenum].y <= 0)
            return;
    }

    if (!tspr->xrepeat || !tspr->yrepeat)
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
        vec2_16_t const span = tilesiz[tilenum];
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
            uwall[x] =   max<int>(startumost[windowxy1.x+x]-windowxy1.y,   startum);
            uwall[x+1] = max<int>(startumost[windowxy1.x+x+1]-windowxy1.y, startum);
            uwall[x+2] = max<int>(startumost[windowxy1.x+x+2]-windowxy1.y, startum);
            uwall[x+3] = max<int>(startumost[windowxy1.x+x+3]-windowxy1.y, startum);

            dwall[x] =   min<int>(startdmost[windowxy1.x+x]-windowxy1.y,   startdm);
            dwall[x+1] = min<int>(startdmost[windowxy1.x+x+1]-windowxy1.y, startdm);
            dwall[x+2] = min<int>(startdmost[windowxy1.x+x+2]-windowxy1.y, startdm);
            dwall[x+3] = min<int>(startdmost[windowxy1.x+x+3]-windowxy1.y, startdm);
        }
#endif
        for (; x<=rx; x++)
        {
            uwall[x] = max<int>(startumost[windowxy1.x+x]-windowxy1.y,startum);
            dwall[x] = min<int>(startdmost[windowxy1.x+x]-windowxy1.y,startdm);
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
        owallmost(uwall.Data(), MAXWALLSB-1, z1-globalposz);
        owallmost(dwall.Data(), MAXWALLSB-1, z2-globalposz);

        int32_t hplc = divscale19(xdimenscale,sy1);
        const int32_t hplc2 = divscale19(xdimenscale,sy2);
        const int32_t idiv = sx2-sx1;
        int32_t hinc[4] = { idiv ? tabledivide32(hplc2-hplc, idiv) : 0 };

#ifdef HIGH_PRECISION_SPRITE
        float const cc     = ((1<<19) * fxdimen * (float)yxaspect) * (1.f/320.f);
        float const loopcc = ((cstat&8) ? -1.f : 1.f) * (float(1<<30) * float(1<<24)) / float(yspan * tspr->yrepeat);

        float hplcf    = cc / sy1;
        float hincf[4] = { idiv ? (cc / sy2 - hplcf) / idiv : 0 };

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
        int32_t cosang = dmulscale14(sintable[(tspr->ang+512)&2047], cosglobalang, sintable[tspr->ang&2047], singlobalang);
        int32_t sinang = dmulscale14(sintable[(tspr->ang+512)&2047], -singlobalang, sintable[tspr->ang&2047], cosglobalang);
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
        globalzd = decltype(globalzd)(ryi[z])<<12;

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
            uwall[x] = max<int>(uwall[x],startumost[x+windowxy1.x]-windowxy1.y);
            dwall[x] = min<int>(dwall[x],startdmost[x+windowxy1.x]-windowxy1.y);
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

            tileLoad(globalpicnum);

        setgotpic(globalpicnum);
        globalbufplc = (intptr_t)tilePtr(globalpicnum);

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

        for (i=0; i<MAXVOXMIPS; i++)
            if (!voxoff[vtilenum][i])
            {
                if (loadvoxel_replace)
                    loadvoxel_replace(vtilenum);
                break;
            }

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

        off.x = tspr->xoffset;
        off.y = /*picanm[sprite[tspr->owner].picnum].yofs +*/ tspr->yoffset;
        if (cstat & 4) off.x = -off.x;
        if ((cstat & 8) && (tspr->cstat&48) != 0) off.y = -off.y;
        tspr->z -= off.y * tspr->yrepeat << 2;

        if ((sprite[spritenum].cstat&CSTAT_SPRITE_ALIGNMENT) == CSTAT_SPRITE_ALIGNMENT_WALL)
        {
            const int32_t xv = tspr->xrepeat*sintable[(tspr->ang+2560+1536)&2047];
            const int32_t yv = tspr->xrepeat*sintable[(tspr->ang+2048+1536)&2047];

            tspr->x -= mulscale16(xv, off.x);
            tspr->y -= mulscale16(yv, off.x);
        }

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

                if (!(cstat & 128))
                    y1 -= mulscale16(mulscale22(B_LITTLE32(longptr[5]), nyrepeat), xdsiz);
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

        x = tspr->x + spriteext[spritenum].position_offset.x;
        y = tspr->y + spriteext[spritenum].position_offset.y;
        z = tspr->z + spriteext[spritenum].position_offset.z;

        i = (int32_t)tspr->ang+1536;
        i += spriteext[spritenum].angoff;

        const int32_t ceilingz = (sec->ceilingstat&3) == 0 ? sec->ceilingz : INT32_MIN;
        const int32_t floorz = (sec->floorstat&3) == 0 ? sec->floorz : INT32_MAX;

        classicDrawVoxel(x,y,z,i,daxrepeat,(int32_t)tspr->yrepeat,vtilenum,
            tspr->shade,tspr->pal,lwall.Data(),swall.Data(),tspr->cstat,(tspr->cstat&48)!=48,floorz,ceilingz);
    }

    if (automapping == 1 && (unsigned)spritenum < MAXSPRITES)
        show2dsprite[spritenum>>3] |= pow2char[spritenum&7];
}

static void renderDrawSprite(int32_t snum)
{
    switch (videoGetRenderMode())
    {
    case REND_CLASSIC:
        classicDrawSprite(snum);
        return;
#ifdef USE_OPENGL
    case REND_POLYMOST:
        polymost_drawsprite(snum);
        return;
#endif
    }
}


//
// drawmaskwall (internal)
//
static void renderDrawMaskedWall(int16_t damaskwallcnt)
{
    //============================================================================= //POLYMOST BEGINS
#ifdef USE_OPENGL
    if (videoGetRenderMode() == REND_POLYMOST) { polymost_drawmaskwall(damaskwallcnt); return; }
#endif
    //============================================================================= //POLYMOST ENDS

    int32_t z = maskwall[damaskwallcnt];
    auto wal = (uwallptr_t)&wall[thewall[z]];
    int32_t sectnum = thesector[z];
    auto sec = (usectorptr_t)&sector[sectnum];
    auto nsec = (usectorptr_t)&sector[wal->nextsector];
    int32_t z1 = max(nsec->ceilingz,sec->ceilingz);
    int32_t z2 = min(nsec->floorz,sec->floorz);

    wallmost(uwall.Data(),z,sectnum,(uint8_t)0);
    wallmost(uplc.Data(),z,(int32_t)wal->nextsector,(uint8_t)0);
    for (bssize_t x=xb1[z]; x<=xb2[z]; x++)
        if (uplc[x] > uwall[x])
            uwall[x] = uplc[x];
    wallmost(dwall.Data(),z,sectnum,(uint8_t)1);
    wallmost(dplc.Data(),z,(int32_t)wal->nextsector,(uint8_t)1);
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
    uint32_t root, num;
    int32_t temp;

    j = 1; k = 0;
    for (i=0; i<4096; i++)
    {
        if (i >= j) { j <<= 2; k++; }
        sqrtable[i] = (uint16_t)(msqrtasm((i<<18)+131072)<<1);
        shlookup[i] = (k<<1)+((10-k)<<8);
        if (i < 256) shlookup[i+4096] = ((k+6)<<1)+((10-(k+6))<<8);
    }

    for(i=0;i<2048;i++)
    {
        root = 128;
        num = i<<20;
        do
        {
            temp = root;
            root = (root+num/root)>>1;
        } while((temp-root+1) > 2);
        temp = root*root-num;
        while (klabs(int32_t(temp-2*root+1)) < klabs(temp))
        {
            temp += -(2*root)+1;
            root--;
        }
        while (klabs(int32_t(temp+2*root+1)) < klabs(temp))
        {
            temp += 2*root+1;
            root++;
        }
        sqrtable_old[i] = root;
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
        horizycent = (ydim*4)>>1;
        horizlookup2[horizycent-1] = divscale32(131072,j);

        for (i=0; i < horizycent-1; i++)
        {
            horizlookup[i] = divscale28(1, i-(horizycent-1));
            horizlookup2[i] = divscale20(klabs(horizlookup[i]), j);
        }

        for (i=horizycent; i < ydim*4-1; i++)
        {
            horizlookup[i] = divscale28(1, i-(horizycent-1));
            horizlookup2[i] = divscale20(klabs(horizlookup[i]), j);
        }
    }

    if (xdimen != oxdimen || viewingrange != oviewingrange)
    {
        int32_t k, x, xinc;

        no_radarang2 = 0;
        oviewingrange = viewingrange;

        xinc = mulscale32(viewingrange*2560,xdimenrecip);
        x = (5120<<16)-mulscale1(xinc,xdimen);

        for (i=0; i<xdimen; i++)
        {
            j = (x&65535); k = (x>>16); x += xinc;

            if (k < 0 || k >= (int32_t)ARRAY_SIZE(qradarang)-1)
            {
                no_radarang2 = 1;
#ifdef DEBUGGINGAIDS
                if (editstatus)
                    initprintf("no rad2\n");
#endif
                break;
            }

            if (j != 0)
                j = mulscale16(qradarang[k+1]-qradarang[k], j);
            radarang2[i] = ((qradarang[k]+j)>>6);
        }

        if (xdimen != oxdimen && (voxoff[0][0] || playing_blood))
        {
            distrecip = NULL;
            for (i = 0; i < DISTRECIPCACHESIZE; i++)
            {
                if (distrecipcache[i].xdimen == xdimen)
                    distrecip = distrecipcache[i].distrecip;
            }
            if (distrecip == NULL)
            {
                int32_t minAge = 0;
                for (i = 1; i < DISTRECIPCACHESIZE; i++)
                {
                    if (distrecipcache[i].age < distrecipcache[minAge].age)
                        minAge = i;
                }
                if (distrecipcache[minAge].distrecip == NULL)
                    distrecipcache[minAge].distrecip = (uint32_t *)Xaligned_alloc(16, DISTRECIPSIZ * sizeof(uint32_t));

                distrecipcache[minAge].age = ++distrecipagecnt;
                distrecipcache[minAge].xdimen = xdimen;

                distrecip = distrecipcache[minAge].distrecip;

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

static int32_t engineLoadTables(void)
{
    static char tablesloaded = 0;

    if (tablesloaded == 0)
    {
        int32_t i;

        initksqrt();

        for (i=0; i<2048; i++)
            reciptable[i] = divscale30(2048, i+2048);

        for (i=0; i<SLOPTABLESIZ; i++)
            sloptable[i] = krecipasm(i-HALFSLOPTABLESIZ);

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

        for (i=0; i<5120; i++)
            qradarang[i] = fix16_from_float(atanf(((float)(5120-i)-0.5f) * (1.f/1280.f)) * (-64.f * (1.f/BANG2RAD)));
        for (i=0; i<5120; i++)
            qradarang[10239-i] = -qradarang[i];

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
    else if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE)
        tailspritefree = -1;

    do_insertsprite_at_headofstat(blanktouse, statnum);

    return blanktouse;
}

// remove sprite 'deleteme' from its status list
LISTFN_STATIC void do_deletespritestat(int16_t deleteme)
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
int32_t(*insertsprite_replace)(int16_t sectnum, int16_t statnum) = NULL;
int32_t insertsprite(int16_t sectnum, int16_t statnum)
{
    if (insertsprite_replace)
        return insertsprite_replace(sectnum, statnum);
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
int32_t (*deletesprite_replace)(int16_t spritenum) = NULL;
int32_t deletesprite(int16_t spritenum)
{
    if (deletesprite_replace)
        return deletesprite_replace(spritenum);
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
    if (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE)
        do_insertsprite_at_headofstat(spritenum, MAXSTATUS);
    else
    {
        prevspritestat[spritenum] = tailspritefree;
        nextspritestat[spritenum] = -1;
        if (tailspritefree >= 0)
            nextspritestat[tailspritefree] = spritenum;
        else
            headspritestat[MAXSTATUS] = spritenum;
        sprite[spritenum].statnum = MAXSTATUS;

        tailspritefree = spritenum;
    }
    Numsprites--;

    return 0;
}

//
// changespritesect
//
int32_t (*changespritesect_replace)(int16_t spritenum, int16_t newsectnum) = NULL;
int32_t changespritesect(int16_t spritenum, int16_t newsectnum)
{
    if (changespritesect_replace)
        return changespritesect_replace(spritenum, newsectnum);
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
int32_t (*changespritestat_replace)(int16_t spritenum, int16_t newstatnum) = NULL;
int32_t changespritestat(int16_t spritenum, int16_t newstatnum)
{
    if (changespritestat_replace)
        return changespritestat_replace(spritenum, newstatnum);
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
int32_t lintersect(const int32_t originX, const int32_t originY, const int32_t originZ,
                   const int32_t destX, const int32_t destY, const int32_t destZ,
                   const int32_t lineStartX, const int32_t lineStartY, const int32_t lineEndX, const int32_t lineEndY,
                   int32_t *intersectionX, int32_t *intersectionY, int32_t *intersectionZ)
{
    const vec2_t ray = { destX-originX,
                         destY-originY };
    const vec2_t lineVec = { lineEndX-lineStartX,
                             lineEndY-lineStartY };
    const vec2_t originDiff = { lineStartX-originX,
                                lineStartY-originY };

    const int32_t rayCrossLineVec = ray.x*lineVec.y - ray.y*lineVec.x;
    const int32_t originDiffCrossRay = originDiff.x*ray.y - originDiff.y*ray.x;

    if (rayCrossLineVec == 0)
    {
        if (originDiffCrossRay != 0 || enginecompatibility_mode != ENGINECOMPATIBILITY_NONE)
        {
            // line segments are parallel
            return 0;
        }

        // line segments are collinear
        const int32_t rayLengthSquared = ray.x*ray.x + ray.y*ray.y;
        const int32_t rayDotOriginDiff = ray.x*originDiff.x + ray.y*originDiff.y;
        const int32_t rayDotLineEndDiff = rayDotOriginDiff + ray.x*lineVec.x + ray.y*lineVec.y;
        int64_t t = min(rayDotOriginDiff, rayDotLineEndDiff);
        if (rayDotOriginDiff < 0)
        {
            if (rayDotLineEndDiff < 0)
                return 0;

            t = 0;
        }
        else if (rayDotOriginDiff > rayLengthSquared)
        {
            if (rayDotLineEndDiff > rayLengthSquared)
                return 0;

            t = rayDotLineEndDiff;
        }
        t = tabledivide64(t << 24L, rayLengthSquared);

        *intersectionX = originX + mulscale24(ray.x, t);
        *intersectionY = originY + mulscale24(ray.y, t);
        *intersectionZ = originZ + mulscale24(destZ-originZ, t);

        return 1;
    }

    const int32_t originDiffCrossLineVec = originDiff.x*lineVec.y - originDiff.y*lineVec.x;
    static const int32_t signBit = 1u<<31u;
    // Any point on either line can be expressed as p+t*r and q+u*s
    // The two line segments intersect when we can find a t & u such that p+t*r = q+u*s
    // If the point is outside of the bounds of the line segment, we know we don't have an intersection.
    // t is < 0 if (originDiffCrossLineVec^rayCrossLineVec) & signBit)
    // u is < 0 if (originDiffCrossRay^rayCrossLineVec) & signBit
    // t is > 1 if klabs(originDiffCrossLineVec) > klabs(rayCrossLineVec)
    // u is > 1 if klabs(originDiffCrossRay) > klabs(rayCrossLineVec)
    // where int32_t u = tabledivide64(((int64_t) originDiffCrossRay) << 24L, rayCrossLineVec);
    if (((originDiffCrossLineVec^rayCrossLineVec) & signBit) ||
        ((originDiffCrossRay^rayCrossLineVec) & signBit) ||
        klabs(originDiffCrossLineVec) > klabs(rayCrossLineVec) ||
        klabs(originDiffCrossRay) > klabs(rayCrossLineVec))
    {
        // line segments do not overlap
        return 0;
    }

    int64_t t = tabledivide64(((int64_t) originDiffCrossLineVec) << 24L, rayCrossLineVec);
    // For sake of completeness/readability, alternative to the above approach for an early out & avoidance of an extra division:
#if 0
    int64_t u = tabledivide64(((int64_t) originDiffCrossRay) << 24L, rayCrossLineVec);
    if (u < 0 || u > 1 << 24 || t < 0 || t > 1 << 24)
    {
        return 0;
    }
#endif

    *intersectionX = originX + mulscale24(ray.x, t);
    *intersectionY = originY + mulscale24(ray.y, t);
    *intersectionZ = originZ + mulscale24(destZ-originZ, t);

    return 1;
}

//
// rintersect (internal)
//
// returns: -1 if didn't intersect, coefficient (x3--x4 fraction)<<16 else
int32_t rintersect_old(int32_t x1, int32_t y1, int32_t z1,
                   int32_t vx, int32_t vy, int32_t vz,
                   int32_t x3, int32_t y3, int32_t x4, int32_t y4,
                   int32_t *intx, int32_t *inty, int32_t *intz)
{
    //p1 towards p2 is a ray

    int32_t const x34=x3-x4, y34=y3-y4;
    int32_t const x31=x3-x1, y31=y3-y1;

    int32_t const bot  = vx*y34 - vy*x34;
    int32_t const topt = x31*y34 - y31*x34;

    if (bot == 0)
        return -1;

    int32_t const topu = vx*y31 - vy*x31;

    if (bot > 0 && (topt < 0 || topu < 0 || topu >= bot))
        return -1;
    else if (bot < 0 && (topt > 0 || topu > 0 || topu <= bot))
        return -1;

    int32_t t = divscale16(topt, bot);
    *intx = x1 + mulscale16(vx, t);
    *inty = y1 + mulscale16(vy, t);
    *intz = z1 + mulscale16(vz, t);

    t = divscale16(topu, bot);

    return t;
}

int32_t rintersect(int32_t x1, int32_t y1, int32_t z1,
                   int32_t vx, int32_t vy, int32_t vz,
                   int32_t x3, int32_t y3, int32_t x4, int32_t y4,
                   int32_t *intx, int32_t *inty, int32_t *intz)
{
    //p1 towards p2 is a ray

    if (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE)
        return rintersect_old(x1,y1,z1,vx,vy,vz,x3,y3,x4,y4,intx,inty,intz);

    int64_t const x34=x3-x4, y34=y3-y4;
    int64_t const x31=x3-x1, y31=y3-y1;

    int64_t const bot  = vx*y34 - vy*x34;
    int64_t const topt = x31*y34 - y31*x34;

    if (bot == 0)
        return -1;

    int64_t const topu = vx*y31 - vy*x31;

    if (bot > 0 && (topt < 0 || topu < 0 || topu >= bot))
        return -1;
    else if (bot < 0 && (topt > 0 || topu > 0 || topu <= bot))
        return -1;

    int64_t t = tabledivide64_noinline(topt<<16, bot);
    *intx = x1 + ((vx*t)>>16);
    *inty = y1 + ((vy*t)>>16);
    *intz = z1 + ((vz*t)>>16);

    t = tabledivide64_noinline(topu<<16, bot);

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

psky_t * tileSetupSky(int32_t const tilenum)
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
// preinitengine
//
static int32_t preinitcalled = 0;

#if !defined DEBUG_MAIN_ARRAYS
static spriteext_t spriteext_s[MAXSPRITES+MAXUNIQHUDID];
static spritesmooth_t spritesmooth_s[MAXSPRITES+MAXUNIQHUDID];
static sectortype sector_s[MAXSECTORS + M32_FIXME_SECTORS];
static walltype wall_s[MAXWALLS + M32_FIXME_WALLS];
#ifndef NEW_MAP_FORMAT
static wallext_t wallext_s[MAXWALLS];
#endif
static spritetype sprite_s[MAXSPRITES];
static tspritetype tsprite_s[MAXSPRITESONSCREEN];
#endif

int32_t enginePreInit(void)
{
	polymost_initosdfuncs();
    initdivtables();

#if !defined DEBUG_MAIN_ARRAYS
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


#ifdef HAVE_CLIPSHAPE_FEATURE
    engineInitClipMaps();
#endif
    preinitcalled = 1;
    return 0;
}


//
// initengine
//
int32_t engineInit(void)
{
    int32_t i;

    if (!preinitcalled)
    {
        i = enginePreInit();
        if (i) return i;
    }

#ifdef YAX_DEBUG
    u64tickspersec = (double)timerGetFreqU64();
    if (u64tickspersec==0.0)
        u64tickspersec = 1.0;
#endif

    if (engineLoadTables())
        return 1;

    xyaspect = -1;

    rotatesprite_y_offset = 0;
    rotatesprite_yxaspect = 65536;

    showinvisibility = 0;

    for (i=1; i<1024; i++)
        lowrecip[i] = ((1<<24)-1)/i;

	voxelmemory.Reset();

    for (i=0; i<MAXTILES; i++)
        tiletovox[i] = -1;
    clearbuf(voxscale, sizeof(voxscale)>>2, 65536);
    clearbufbyte(voxrotate, sizeof(voxrotate), 0);

    paletteloaded = 0;

    searchit = 0; searchstat = -1;

    totalclock = 0;
    g_visibility = 512;
    parallaxvisibility = 512;

    maxspritesonscreen = MAXSPRITESONSCREEN;

    paletteLoadFromDisk();

#ifdef USE_OPENGL
    if (!mdinited) mdinit();
#endif

    return 0;
}

//
// E_PostInit
//
int32_t enginePostInit(void)
{
    if (!(paletteloaded & PALETTE_MAIN))
        I_FatalError("No palette found.");
    if (!(paletteloaded & PALETTE_SHADE))
        I_FatalError("No shade table found.");
    if (!(paletteloaded & PALETTE_TRANSLUC))
        I_FatalError("No translucency table found.");

    palettePostLoadTables();

    return 0;
}

//
// uninitengine
//
void engineUnInit(void)
{
#ifdef USE_OPENGL
    polymost_glreset();
    hicinit();
    freeallmodels();
# ifdef POLYMER
    polymer_uninit();
# endif
#endif

	TileFiles.CloseAll();

    for (bssize_t i=0; i<DISTRECIPCACHESIZE; i++)
        ALIGNED_FREE_AND_NULL(distrecipcache[i].distrecip);
    Bmemset(distrecipcache, 0, sizeof(distrecipcache));

    paletteloaded = 0;

    for (bssize_t i=0; i<MAXPALOOKUPS; i++)
        if (i==0 || palookup[i] != palookup[0])
        {
            // Take care of handling aliased ^^^ cases!
            Xaligned_free(palookup[i]);
        }
    Bmemset(palookup, 0, sizeof(palookup));

    for (bssize_t i=0; i<MAXBLENDTABS; i++)
        Xfree(blendtable[i]);
    Bmemset(blendtable, 0, sizeof(blendtable));

    for (bssize_t i=1; i<MAXBASEPALS; i++)
        Xfree(basepaltable[i]);
    Bmemset(basepaltable, 0, sizeof(basepaltable));
    basepaltable[0] = palette;

    for (bssize_t i = 0; i < num_usermaphacks; i++)
    {
        Xfree(usermaphacks[i].mhkfile);
        Xfree(usermaphacks[i].title);
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
void (*initspritelists_replace)(void) = NULL;
void initspritelists(void)
{
    if (initspritelists_replace)
    {
        initspritelists_replace();
        return;
    }
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


void set_globalang(fix16_t const ang)
{
    globalang = fix16_to_int(ang)&2047;
    qglobalang = ang & 0x7FFFFFF;

    float const f_ang = fix16_to_float(ang);
    float const f_ang_radians = f_ang * M_PI * (1.f/1024.f);

    float const fcosang = cosf(f_ang_radians) * 16384.f;
    float const fsinang = sinf(f_ang_radians) * 16384.f;

#ifdef USE_OPENGL
    fcosglobalang = fcosang;
    fsinglobalang = fsinang;
#endif
    
    cosglobalang = (int)fcosang;
    singlobalang = (int)fsinang;

    cosviewingrangeglobalang = mulscale16(cosglobalang,viewingrange);
    sinviewingrangeglobalang = mulscale16(singlobalang,viewingrange);
}

//
// drawrooms
//
int32_t renderDrawRoomsQ16(int32_t daposx, int32_t daposy, int32_t daposz,
                           fix16_t daang, fix16_t dahoriz, int16_t dacursectnum)
{
    int32_t i, j, /*cz, fz,*/ closest;
    int16_t *shortptr1, *shortptr2;

    beforedrawrooms = 0;

    set_globalpos(daposx, daposy, daposz);
    set_globalang(daang);

    global100horiz = dahoriz;

    // xdimenscale is scale(xdimen,yxaspect,320);
    // normalization by viewingrange so that center-of-aim doesn't depend on it
    qglobalhoriz = mulscale16(dahoriz-F16(100), divscale16(xdimenscale, viewingrange))+fix16_from_int(ydimen>>1);
    globalhoriz = fix16_to_int(qglobalhoriz);

    globaluclip = (0-globalhoriz)*xdimscale;
    globaldclip = (ydimen-globalhoriz)*xdimscale;

    i = mulscale16(xdimenscale,viewingrangerecip);
    globalpisibility = mulscale16(parallaxvisibility,i);
    switch (videoGetRenderMode())
    {
        // switch on renderers to make fog look almost the same everywhere

    case REND_CLASSIC:
        globalvisibility = mulscale16(g_visibility,i);
        break;
    case REND_POLYMOST:
        // NOTE: In Polymost, the fragment depth depends on the x screen size!
        globalvisibility = g_visibility * xdimen;
        globalvisibility2 = mulscale16(g_visibility, i);
        break;
    }

    globalhisibility = mulscale16(globalvisibility,xyaspect);
    globalcisibility = mulscale8(globalhisibility,320);

#ifdef USE_OPENGL
    globalhisibility2 = mulscale16(globalvisibility2,xyaspect);
    globalcisibility2 = mulscale8(globalhisibility2,320);
#endif

    globalcursectnum = dacursectnum;
    totalclocklock = totalclock;

    if ((xyaspect != oxyaspect) || (xdimen != oxdimen) || (viewingrange != oviewingrange))
        dosetaspect();

    Bmemset(gotsector, 0, sizeof(gotsector));

    if (videoGetRenderMode() != REND_CLASSIC
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

    for (int i = 0; i < numwalls; ++i)
    {
        if (wall[i].cstat & CSTAT_WALL_ROTATE_90)
        {
            auto &w    = wall[i];
			auto &tile = RotTile(w.picnum+animateoffs(w.picnum,16384));

            if (tile.newtile == -1 && tile.owner == -1)
            {
				auto owner = w.picnum + animateoffs(w.picnum, 16384);

				tile.newtile = TileFiles.tileCreateRotated(owner);
                Bassert(tile.newtile != -1);

                RotTile(tile.newtile).owner = w.picnum+animateoffs(w.picnum,16384);

            }
        }
    }

    // Update starting sector number (common to classic and Polymost).
    // ADJUST_GLOBALCURSECTNUM.
    if (globalcursectnum >= MAXSECTORS)
        globalcursectnum -= MAXSECTORS;
    else
    {
        i = globalcursectnum;
        updatesector(globalposx,globalposy,&globalcursectnum);
        if (globalcursectnum < 0) globalcursectnum = i;

        // PK 20110123: I'm not sure what the line above is supposed to do, but 'i'
        //              *can* be negative, so let's just quit here in that case...
        if (globalcursectnum<0)
            return 0;
    }

#ifdef USE_OPENGL
    //============================================================================= //POLYMOST BEGINS
    polymost_drawrooms();

    if (videoGetRenderMode() != REND_CLASSIC)
        return inpreparemirror;
    //============================================================================= //POLYMOST ENDS
#endif

    videoBeginDrawing(); //{{{

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
    //      { /* setvmode(0x3);*/ OSD_Printf("Nice try.\n"); Bexit(EXIT_SUCCESS); }

    numhits = xdimen; numscans = 0; numbunches = 0;
    maskwallcnt = 0; smostwallcnt = 0; smostcnt = 0; spritesortcnt = 0;

#ifdef YAX_ENABLE
    if (yax_globallev != YAX_MAXDRAWS)
    {
        j = yax_globalbunch*xdimen;

        Bmemcpy(umost.Data(), yumost+j, xdimen*sizeof(int16_t));
        Bmemcpy(dmost.Data(), ydmost+j, xdimen*sizeof(int16_t));

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
    classicScanSector(globalcursectnum);

    if (inpreparemirror)
    {
        // INPREPAREMIRROR_NO_BUNCHES
        // numbunches==0 can happen if the mirror is far away... the game code decides
        // to draw it, but scansector gets zero bunches.  Result: big screwup!
        // Set inpreparemirror to 0 to indicate that we were unable to render the mirror
        if (numbunches==0)
        {
            inpreparemirror = 0;
            videoEndDrawing();  //!!!
            return 0;
        }

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

        classicDrawBunches(0L);
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

        classicDrawBunches(closest);

        if (automapping)
        {
            for (int z=bunchfirst[closest]; z>=0; z=bunchp2[z])
                show2dwall[thewall[z]>>3] |= pow2char[thewall[z]&7];
        }

        numbunches--;
        bunchfirst[closest] = bunchfirst[numbunches];
        bunchlast[closest] = bunchlast[numbunches];
    }

    videoEndDrawing();   //}}}

    return inpreparemirror;
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
static inline _equation equation(float const x1, float const y1, float const x2, float const y2)
{
    const float f = x2-x1;

    // vertical
    if (f == 0.f)
       return { 1, 0, -x1 };
    else
    {
        float const ff = (y2 - y1) / f;
        return { ff, -1, (y1 - (ff * x1)) };
    }
}

int32_t wallvisible(int32_t const x, int32_t const y, int16_t const wallnum)
{
    // 1 if wall is in front of player 0 otherwise
    auto w1 = (uwallptr_t)&wall[wallnum];
    auto w2 = (uwallptr_t)&wall[w1->point2];

    int32_t const a1 = getangle(w1->x - x, w1->y - y);
    int32_t const a2 = getangle(w2->x - x, w2->y - y);

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



#ifdef DEBUG_MASK_DRAWING
int32_t g_maskDrawMode = 0;
#endif

static inline int comparetsprites(int const k, int const l)
{
#ifdef USE_OPENGL
    if (videoGetRenderMode() == REND_POLYMOST)
    {
        if ((tspriteptr[k]->cstat & 48) != (tspriteptr[l]->cstat & 48))
            return (tspriteptr[k]->cstat & 48) - (tspriteptr[l]->cstat & 48);

        if ((tspriteptr[k]->cstat & 48) == 16 && tspriteptr[k]->ang != tspriteptr[l]->ang)
            return tspriteptr[k]->ang - tspriteptr[l]->ang;
    }
#endif
    if (tspriteptr[k]->statnum != tspriteptr[l]->statnum)
        return tspriteptr[k]->statnum - tspriteptr[l]->statnum;

    if (tspriteptr[k]->x == tspriteptr[l]->x &&
        tspriteptr[k]->y == tspriteptr[l]->y &&
        tspriteptr[k]->z == tspriteptr[l]->z &&
        (tspriteptr[k]->cstat & 48) == (tspriteptr[l]->cstat & 48) &&
        tspriteptr[k]->owner != tspriteptr[l]->owner)
        return tspriteptr[k]->owner - tspriteptr[l]->owner;

    if (klabs(spritesxyz[k].z-globalposz) != klabs(spritesxyz[l].z-globalposz))
        return klabs(spritesxyz[k].z-globalposz)-klabs(spritesxyz[l].z-globalposz);

    return 0;
}

static void sortsprites(int const start, int const end)
{
    int32_t i, gap, y, ys;

    if (start >= end)
        return;

    gap = 1; while (gap < end - start) gap = (gap<<1)+1;
    for (gap>>=1; gap>0; gap>>=1)   //Sort sprite list
        for (i=start; i<end-gap; i++)
            for (bssize_t l=i; l>=start; l-=gap)
            {
                if (spritesxyz[l].y <= spritesxyz[l+gap].y) break;
                swapptr(&tspriteptr[l],&tspriteptr[l+gap]);
                swaplong(&spritesxyz[l].x,&spritesxyz[l+gap].x);
                swaplong(&spritesxyz[l].y,&spritesxyz[l+gap].y);
            }

    ys = spritesxyz[start].y; i = start;
    for (bssize_t j=start+1; j<=end; j++)
    {
        if (j < end)
        {
            y = spritesxyz[j].y;
            if (y == ys)
                continue;

            ys = y;
        }

        if (j > i+1)
        {
            for (bssize_t k=i; k<j; k++)
            {
                auto const s = tspriteptr[k];

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
                    if (comparetsprites(k, l) < 0)
                    {
                        swapptr(&tspriteptr[k],&tspriteptr[l]);
                        vec3_t tv3 = spritesxyz[k];
                        spritesxyz[k] = spritesxyz[l];
                        spritesxyz[l] = tv3;
                    }
        }
        i = j;
    }
}

//
// drawmasks
//
void renderDrawMasks(void)
{
#ifdef DEBUG_MASK_DRAWING
        static struct {
            int16_t di;  // &32768: &32767 is tspriteptr[], else thewall[] index
            int16_t i;   // sprite[] or wall[] index
        } debugmask[MAXWALLSB + MAXSPRITESONSCREEN + 1];

        int32_t dmasknum = 0;

# define debugmask_add(dispidx, idx) do { \
        if (g_maskDrawMode && videoGetRenderMode()==REND_CLASSIC) { \
            debugmask[dmasknum].di = dispidx; \
            debugmask[dmasknum++].i = idx; \
        } \
    } while (0)
#else
# define debugmask_add(dispidx, idx) do {} while (0)
#endif
    int32_t i = spritesortcnt-1;
    int32_t numSprites = spritesortcnt;

#ifdef USE_OPENGL
    if (videoGetRenderMode() == REND_POLYMOST)
    {
        spritesortcnt = 0;
        int32_t back = i;
        for (; i >= 0; --i)
        {
            if (polymost_spriteHasTranslucency(&tsprite[i]))
            {
                tspriteptr[spritesortcnt] = &tsprite[i];
                ++spritesortcnt;
            } else
            {
                tspriteptr[back] = &tsprite[i];
                --back;
            }
        }
    } else
#endif
    {
        for (; i >= 0; --i)
        {
            tspriteptr[i] = &tsprite[i];
        }
    }

    for (i=numSprites-1; i>=0; --i)
    {
        const int32_t xs = tspriteptr[i]->x-globalposx, ys = tspriteptr[i]->y-globalposy;
        const int32_t yp = dmulscale6(xs,cosviewingrangeglobalang,ys,sinviewingrangeglobalang);
#ifdef USE_OPENGL
        const int32_t modelp = polymost_spriteIsModelOrVoxel(tspriteptr[i]);
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
                //Delete face sprite if on wrong side!
                if (i >= spritesortcnt)
                {
                    --numSprites;
                    if (i != numSprites)
                    {
                        tspriteptr[i] = tspriteptr[numSprites];
                        spritesxyz[i].x = spritesxyz[numSprites].x;
                        spritesxyz[i].y = spritesxyz[numSprites].y;
                    }
                }
                else
                {
                    --numSprites;
                    --spritesortcnt;
                    if (i != numSprites)
                    {
                        tspriteptr[i] = tspriteptr[spritesortcnt];
                        spritesxyz[i].x = spritesxyz[spritesortcnt].x;
                        spritesxyz[i].y = spritesxyz[spritesortcnt].y;
                        tspriteptr[spritesortcnt] = tspriteptr[numSprites];
                        spritesxyz[spritesortcnt].x = spritesxyz[numSprites].x;
                        spritesxyz[spritesortcnt].y = spritesxyz[numSprites].y;
                    }
                }
                continue;
            }
        }
        spritesxyz[i].y = yp;
    }

    sortsprites(0, spritesortcnt);
    sortsprites(spritesortcnt, numSprites);

    videoBeginDrawing(); //{{{

#ifdef USE_OPENGL
    if (videoGetRenderMode() == REND_POLYMOST)
    {
        GLInterface.EnableBlend(false);
        GLInterface.EnableAlphaTest(true);
		GLInterface.SetClamp(1+2);
        GLInterface.SetDepthBias(-2, -256);

        if (spritesortcnt < numSprites)
        {
            int32_t py = spritesxyz[spritesortcnt].y;
            int32_t pcstat = tspriteptr[spritesortcnt]->cstat & 48;
            int32_t pangle = tspriteptr[spritesortcnt]->ang;
            i = spritesortcnt;
            for (bssize_t j = spritesortcnt + 1; j <= numSprites; j++)
            {
                if (j < numSprites)
                {
                    if (py == spritesxyz[j].y && pcstat == (tspriteptr[j]->cstat & 48) && (pcstat != 16 || pangle == tspriteptr[j]->ang)
                        && !polymost_spriteIsModelOrVoxel(tspriteptr[j]))
                        continue;
                    py = spritesxyz[j].y;
                    pcstat = (tspriteptr[j]->cstat & 48);
                    pangle = tspriteptr[j]->ang;
                }
                if (j - i == 1)
                {
                    debugmask_add(i | 32768, tspriteptr[i]->owner);
                    renderDrawSprite(i);
                    tspriteptr[i] = NULL;
                }
                else
                {
					GLInterface.SetDepthMask(false);

                    for (bssize_t k = j-1; k >= i; k--)
                    {
                        debugmask_add(k | 32768, tspriteptr[k]->owner);
                        renderDrawSprite(k);
                    }

					GLInterface.SetDepthMask(true);

					GLInterface.SetColorMask(false);
                    
                    for (bssize_t k = j-1; k >= i; k--)
                    {
                        renderDrawSprite(k);
                        tspriteptr[k] = NULL;
                    }

					GLInterface.SetColorMask(true);

                }
                i = j;
            }
        }

		GLInterface.SetClamp(0);
        int32_t numMaskWalls = maskwallcnt;
        maskwallcnt = 0;
        for (i = 0; i < numMaskWalls; i++)
        {
            if (polymost_maskWallHasTranslucency((uwalltype *) &wall[thewall[maskwall[i]]]))
            {
                maskwall[maskwallcnt] = maskwall[i];
                maskwallcnt++;
            }
            else
                renderDrawMaskedWall(i);
        }

        GLInterface.EnableBlend(true);
        GLInterface.EnableAlphaTest(true);
		GLInterface.SetDepthMask(false);
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
        const int32_t w = (videoGetRenderMode()==REND_POLYMER) ?
            maskwall[maskwallcnt-1] : thewall[maskwall[maskwallcnt-1]];

        maskwallcnt--;

        vec2f_t dot    = { (float)wall[w].x, (float)wall[w].y };
        vec2f_t dot2   = { (float)wall[wall[w].point2].x, (float)wall[wall[w].point2].y };
        vec2f_t middle = { (dot.x + dot2.x) * .5f, (dot.y + dot2.y) * .5f };

        _equation maskeq = equation(dot.x, dot.y, dot2.x, dot2.y);
        _equation p1eq   = equation(pos.x, pos.y, dot.x, dot.y);
        _equation p2eq   = equation(pos.x, pos.y, dot2.x, dot2.y);

#ifdef USE_OPENGL
        if (videoGetRenderMode() == REND_POLYMOST)
			GLInterface.SetClamp(1+2);
#endif

        i = spritesortcnt;
        while (i)
        {
            i--;
            if (tspriteptr[i] != NULL)
            {
                vec2f_t spr;
                auto const tspr = tspriteptr[i];

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

                            get_wallspr_points(tspr, &xx[0], &xx[1], &yy[0], &yy[1]);

                            if ((tspr->cstat & 48) != 16)
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
                        renderDrawSprite(i);

                        tspriteptr[i] = NULL;
                    }
                }
            }
        }

        debugmask_add(maskwall[maskwallcnt], thewall[maskwall[maskwallcnt]]);
#ifdef USE_OPENGL
        if (videoGetRenderMode() == REND_POLYMOST)
			GLInterface.SetClamp(0);
#endif
        renderDrawMaskedWall(maskwallcnt);
    }

#ifdef USE_OPENGL
    if (videoGetRenderMode() == REND_POLYMOST)
		GLInterface.SetClamp(1+2);
#endif
    while (spritesortcnt)
    {
        --spritesortcnt;
        if (tspriteptr[spritesortcnt] != NULL)
        {
            debugmask_add(i | 32768, tspriteptr[i]->owner);
            renderDrawSprite(spritesortcnt);
            tspriteptr[spritesortcnt] = NULL;
        }
    }
#ifdef USE_OPENGL
    if (videoGetRenderMode() == REND_POLYMOST)
    {
		GLInterface.SetDepthMask(true);
		GLInterface.SetClamp(0);
        GLInterface.SetDepthBias(0, 0);
    }
#endif


    videoEndDrawing();   //}}}
}


//
// fillpolygon (internal)
//
static void renderFillPolygon(int32_t npoints)
{
    // fix for bad next-point (xb1) values...
    for (int z = 0; z < npoints; z++)
        if ((unsigned)xb1[z] >= (unsigned)npoints)
            xb1[z] = 0;

    FVector2 xtex, ytex, otex;
    int x1 = mulscale16(globalx1, xyaspect);
    int y2 = mulscale16(globaly2, xyaspect);
    xtex.X = ((float)asm1) * (1.f / 4294967296.f);
    xtex.Y = ((float)asm2) * (1.f / 4294967296.f);
    ytex.X = ((float)x1) * (1.f / 4294967296.f);
    ytex.Y = ((float)y2) * (-1.f / 4294967296.f);
    otex.X = (fxdim * xtex.X + fydim * ytex.X) * -0.5f + fglobalposx * (1.f / 4294967296.f);
    otex.Y = (fxdim * xtex.Y + fydim * ytex.Y) * -0.5f - fglobalposy * (1.f / 4294967296.f);
    twod->FillPolygon(rx1, ry1, xb1, npoints, globalpicnum, globalpal, globalshade, globalorientation, xtex, ytex, otex, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
}

//
// drawmapview
//
void renderDrawMapView(int32_t dax, int32_t day, int32_t zoome, int16_t ang)
{
    int32_t i, j, k, l;
    int32_t x, y;
    int32_t s, ox, oy;

    int32_t const oyxaspect = yxaspect, oviewingrange = viewingrange;

    renderSetAspect(65536, divscale16((320*5)/8, 200));

    beforedrawrooms = 0;

    Bmemset(gotsector, 0, sizeof(gotsector));

    vec2_t const c1 = { (windowxy1.x<<12), (windowxy1.y<<12) };
    vec2_t const c2 = { ((windowxy2.x+1)<<12)-1, ((windowxy2.y+1)<<12)-1 };

    zoome <<= 8;

    vec2_t const bakgvect = { divscale28(sintable[(1536 - ang) & 2047], zoome),
                        divscale28(sintable[(2048 - ang) & 2047], zoome) };
    vec2_t const vect = { mulscale8(sintable[(2048 - ang) & 2047], zoome), mulscale8(sintable[(1536 - ang) & 2047], zoome) };
    vec2_t const vect2 = { mulscale16(vect.x, yxaspect), mulscale16(vect.y, yxaspect) };

    int32_t sortnum = 0;

    usectorptr_t sec;

    for (s=0,sec=(usectorptr_t)&sector[s]; s<numsectors; s++,sec++)
        if (show2dsector[s>>3]&pow2char[s&7])
        {
#ifdef YAX_ENABLE
            if (yax_getbunch(s, YAX_FLOOR) >= 0 && (sector[s].floorstat&(256+128))==0)
                continue;
#endif
            int32_t npoints = 0; i = 0;
            int32_t startwall = sec->wallptr;
            j = startwall; l = 0;
            uwallptr_t wal;
            int32_t w;
            for (w=sec->wallnum,wal=(uwallptr_t)&wall[startwall]; w>0; w--,wal++,j++)
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

            vec2_t bak = { rx1[0], mulscale16(ry1[0]-(ydim<<11),xyaspect)+(ydim<<11) };


            //Collect floor sprites to draw
            for (i=headspritesect[s]; i>=0; i=nextspritesect[i])
            {
                if (sprite[i].cstat & 32768)
                    continue;

                if ((sprite[i].cstat & 48) == 32)
                {
                    if ((sprite[i].cstat & (64 + 8)) == (64 + 8))
                        continue;
                    tsprite[sortnum++].owner = i;
                }
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
            tileUpdatePicnum(&globalpicnum, s);
            setgotpic(globalpicnum);
            if ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0)) continue;

			globalshade = max(min<int>(sec->floorshade, numshades - 1), 0);
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
            renderFillPolygon(npoints);
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
        auto const spr = (uspritetype * )&sprite[tsprite[s].owner];
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


            vec2_t bak = { rx1[0], mulscale16(ry1[0] - (ydim << 11), xyaspect) + (ydim << 11) };


            globalpicnum = spr->picnum;
            globalpal = spr->pal; // GL needs this, software doesn't
            if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
            tileUpdatePicnum(&globalpicnum, s);
            setgotpic(globalpicnum);
            if ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0)) continue;

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

            if ((spr->cstat&0x4) > 0) globalx1 = -globalx1, globaly1 = -globaly1, globalposx = -globalposx;
            asm1 = (globaly1<<2); globalx1 <<= 2; globalposx <<= (20+2);
            asm2 = (globalx2<<2); globaly2 <<= 2; globalposy <<= (20+2);

            set_globalpos(globalposx, globalposy, globalposz);

            // so polymost can get the translucency. ignored in software mode:
            globalorientation = ((spr->cstat&2)<<7) | ((spr->cstat&512)>>2);
            renderFillPolygon(npoints);
        }
    }

    if (r_usenewaspect)
        renderSetAspect(oviewingrange, oyxaspect);
    else
        renderSetAspect(65536, divscale16(ydim*320, xdim*200));
}

//////////////////// LOADING AND SAVING ROUTINES ////////////////////

static FORCE_INLINE int32_t have_maptext(void)
{
    return (mapversion >= 10);
}

static void enginePrepareLoadBoard(FileReader & fr, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum)
{
    initspritelists();

    Bmemset(show2dsector, 0, sizeof(show2dsector));
    Bmemset(show2dsprite, 0, sizeof(show2dsprite));
    Bmemset(show2dwall, 0, sizeof(show2dwall));
    Bmemset(editwall, 0, sizeof(editwall));
#ifdef USE_STRUCT_TRACKERS
    Bmemset(sectorchanged, 0, sizeof(sectorchanged));
    Bmemset(spritechanged, 0, sizeof(spritechanged));
    Bmemset(wallchanged, 0, sizeof(wallchanged));
#endif

#ifdef USE_OPENGL
    Polymost_prepare_loadboard();
#endif

    if (!have_maptext())
    {
        fr.Read(&dapos->x,4); dapos->x = B_LITTLE32(dapos->x);
        fr.Read(&dapos->y,4); dapos->y = B_LITTLE32(dapos->y);
        fr.Read(&dapos->z,4); dapos->z = B_LITTLE32(dapos->z);
        fr.Read(daang,2);  *daang  = B_LITTLE16(*daang) & 2047;
        fr.Read(dacursectnum,2); *dacursectnum = B_LITTLE16(*dacursectnum);
    }
}

static int32_t engineFinishLoadBoard(const vec3_t *dapos, int16_t *dacursectnum, int16_t numsprites, char myflags)
{
    int32_t i, realnumsprites=numsprites, numremoved;

#if !defined USE_OPENGL || !defined POLYMER
    UNREFERENCED_PARAMETER(myflags);
#endif

    for (i=0; i<numsprites; i++)
    {
        int32_t removeit = 0;

        if ((sprite[i].cstat & 48) == 48)
		{
			// If I understand this correctly, both of these essentially do the same thing...
            if (!playing_rr) sprite[i].cstat &= ~48;
            else sprite[i].cstat |= 32768;
		}

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
        if (videoGetRenderMode() == REND_POLYMER)
        {
            if ((myflags&4)==0)
                polymer_loadboard();
        }
# endif
#endif
    }

    guniqhudid = 0;

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
        initprintf("Map error: sprite #%d (%d,%d) with illegal statnum (%d) REMOVED.\n",
                   i, TrackerCast(sprite[i].x), TrackerCast(sprite[i].y), TrackerCast(sprite[i].statnum));
        remove_sprite(i);
    }
    else if ((unsigned)sprite[i].picnum >= MAXTILES)
    {
        initprintf("Map error: sprite #%d (%d,%d) with illegal picnum (%d) REMOVED.\n",
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

        initprintf("Map error: sprite #%d (%d,%d) with illegal sector (%d) ",
                   i, TrackerCast(sprite[i].x), TrackerCast(sprite[i].y), osectnum);

        if (sprite[i].statnum != MAXSTATUS)
            initprintf("changed to sector %d.\n", TrackerCast(sprite[i].sectnum));
        else
            initprintf("REMOVED.\n");
    }
}


#include "md4.h"

int32_t(*loadboard_replace)(const char *filename, char flags, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum) = NULL;

// flags: 1, 2: former parameter "fromwhere"
//           4: don't call polymer_loadboard
//           8: don't autoexec <mapname>.cfg
// returns: on success, number of removed sprites
//          -1: file not found
//          -2: invalid version
//          -3: invalid number of sectors, walls or sprites
//       <= -4: map-text error
int32_t engineLoadBoard(const char *filename, char flags, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum)
{
    if (loadboard_replace)
        return loadboard_replace(filename, flags, dapos, daang, dacursectnum);
    int32_t i;
    int16_t numsprites;
    const char myflags = flags&(~3);

    flags &= 3;

	FileReader fr = fileSystem.OpenFileReader(filename, 0);
	if (!fr.isOpen())
        { mapversion = 7; return -1; }

    if (fr.Read(&mapversion, 4) != 4)
    {
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
            return -2;
        }
    }

    enginePrepareLoadBoard(fr, dapos, daang, dacursectnum);


    ////////// Read sectors //////////

    fr.Read(&numsectors,2); numsectors = B_LITTLE16(numsectors);
    if ((unsigned)numsectors >= MYMAXSECTORS() + 1)
    {
    error:
        numsectors = 0;
        numwalls   = 0;
        numsprites = 0;
        return -3;
    }

    fr.Read(sector, sizeof(sectortypev7)*numsectors);

    for (i=numsectors-1; i>=0; i--)
    {
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
    }


    ////////// Read walls //////////

    fr.Read(&numwalls,2); numwalls = B_LITTLE16(numwalls);
    if ((unsigned)numwalls >= MYMAXWALLS()+1) goto error;

    fr.Read( wall, sizeof(walltypev7)*numwalls);

    for (i=numwalls-1; i>=0; i--)
    {
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
    }


    ////////// Read sprites //////////

    fr.Read(&numsprites,2); numsprites = B_LITTLE16(numsprites);
    if ((unsigned)numsprites >= MYMAXSPRITES()+1) goto error;

    fr.Read( sprite, sizeof(spritetype)*numsprites);

    fr.Seek(0, FileReader::SeekSet);
    int32_t boardsize = fr.GetLength();
    uint8_t *fullboard = (uint8_t*)Xmalloc(boardsize);
    fr.Read( fullboard, boardsize);
    md4once(fullboard, boardsize, g_loadedMapHack.md4);
    Xfree(fullboard);

    // Done reading file.

    if (!have_maptext())
    {
        for (i=numsprites-1; i>=0; i--)
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

            check_sprite(i);
        }
    }
    else
    {
        for (i=numsprites-1; i>=0; i--)
            check_sprite(i);
    }

    // Back up the map version of the *loaded* map. Must be before yax_update().
    g_loadedMapVersion = mapversion;
#ifdef YAX_ENABLE
    yax_update(mapversion<9);
#endif

    if ((myflags&8)==0)
    {
#if 0	// No, no! This is absolutely unacceptable. I won't support mods that require this kind of access.
        char fn[BMAX_PATH];

        Bstrcpy(fn, filename);
        append_ext_UNSAFE(fn, ".cfg");

        OSD_Exec(fn);
#endif
        // Per-map ART
        artSetupMapArt(filename);
    }

    // initprintf("Loaded map \"%s\" (md4sum: %08x%08x%08x%08x)\n", filename, B_BIG32(*((int32_t*)&md4out[0])), B_BIG32(*((int32_t*)&md4out[4])), B_BIG32(*((int32_t*)&md4out[8])), B_BIG32(*((int32_t*)&md4out[12])));

    return engineFinishLoadBoard(dapos, dacursectnum, numsprites, myflags);
}


//
// loadboardv5/6
//
#include "engine_oldmap.h"

// Powerslave uses v6
// Witchaven 1 and TekWar and LameDuke use v5
int32_t engineLoadBoardV5V6(const char *filename, char fromwhere, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum)
{
    int32_t i;
    int16_t numsprites;

    struct sectortypev5 v5sect;
    struct walltypev5   v5wall;
    struct spritetypev5 v5spr;
    struct sectortypev6 v6sect;
    struct walltypev6   v6wall;
    struct spritetypev6 v6spr;

	FileReader fr = fileSystem.OpenFileReader(filename, fromwhere);
    if (!fr.isOpen())
        { mapversion = 5L; return -1; }

    fr.Read(&mapversion,4); mapversion = B_LITTLE32(mapversion);
    if (mapversion != 5L && mapversion != 6L) { return -2; }

    enginePrepareLoadBoard(fr, dapos, daang, dacursectnum);

    fr.Read(&numsectors,2); numsectors = B_LITTLE16(numsectors);
    if (numsectors > MAXSECTORS) { return -1; }
    for (i=0; i<numsectors; i++)
    {
        switch (mapversion)
        {
        case 5:
            fr.Read(&v5sect,sizeof(struct sectortypev5));
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
            fr.Read(&v6sect,sizeof(struct sectortypev6));
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

    fr.Read(&numwalls,2); numwalls = B_LITTLE16(numwalls);
    if (numwalls > MAXWALLS) { return -1; }
    for (i=0; i<numwalls; i++)
    {
        switch (mapversion)
        {
        case 5:
            fr.Read(&v5wall,sizeof(struct walltypev5));
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
            fr.Read(&v6wall,sizeof(struct walltypev6));
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

    fr.Read(&numsprites,2); numsprites = B_LITTLE16(numsprites);
    if (numsprites > MAXSPRITES) { return -1; }
    for (i=0; i<numsprites; i++)
    {
        switch (mapversion)
        {
        case 5:
            fr.Read(&v5spr,sizeof(struct spritetypev5));
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
            fr.Read(&v6spr,sizeof(struct spritetypev6));
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

    // Done reading file.

    return engineFinishLoadBoard(dapos, dacursectnum, numsprites, 0);
}



#define YSAVES ((xdim*MAXSPRITES)>>7)

static void videoAllocateBuffers(void)
{
    // Needed for the game's TILT_SETVIEWTOTILE_320.
    const int32_t clamped_ydim = max(ydim, 320);

    smost.Resize(YSAVES);
    umost.Resize(xdim);
      dmost.Resize(xdim);
      startumost.Resize(xdim);
      startdmost.Resize(xdim);
      bakumost.Resize(xdim);
      bakdmost.Resize(xdim);
      uplc.Resize(xdim);
      dplc.Resize(xdim);
      uwall.Resize(xdim);
      dwall.Resize(xdim);
      swplc.Resize(xdim);
      lplc.Resize(xdim);
      swall.Resize(xdim);
      lwall.Resize((xdim + 4));
      radarang2.Resize(xdim);
      dotp1.Resize(clamped_ydim);
      dotp2.Resize(clamped_ydim);
      lastx.Resize(clamped_ydim);
      mirrorBuffer.Resize(xdim * ydim);

    ysavecnt = YSAVES;
    nodesperline = tabledivide32_noinline(YSAVES, ydim);

    if (videoGetRenderMode() == REND_CLASSIC)
    {
        glsurface_initialize({ xdim, ydim });
    }
}


//
// setgamemode
//
// JBF: davidoption now functions as a windowed-mode flag (0 == windowed, 1 == fullscreen)
int32_t videoSetGameMode(char davidoption, int32_t daupscaledxdim, int32_t daupscaledydim, int32_t dabpp, int32_t daupscalefactor)
{
    int32_t j;

    if (dabpp != 32) return -1; // block software mode.

    daupscaledxdim = max(320, daupscaledxdim);
    daupscaledydim = max(200, daupscaledydim);

    if (in3dmode() && 
        (xres == daupscaledxdim) && (yres == daupscaledydim) && (bpp == dabpp))
        return 0;

    Bstrcpy(kensmessage,"!!!! BUILD engine&tools programmed by Ken Silverman of E.G. RI."
           "  (c) Copyright 1995 Ken Silverman.  Summary:  BUILD = Ken. !!!!");
    //  if (getkensmessagecrc(FP_OFF(kensmessage)) != 0x56c764d4)
    //      { OSD_Printf("Nice try.\n"); Bexit(EXIT_SUCCESS); }

    //if (checkvideomode(&daxdim, &daydim, dabpp, davidoption)<0) return -1;

    //bytesperline is set in this function

    j = bpp;

    g_lastpalettesum = 0;

    rendmode = REND_POLYMOST;

    upscalefactor = 1;
    xdim = daupscaledxdim;
    ydim = daupscaledydim;
	V_UpdateModeSize(xdim, ydim);
    numpages = 1; // We have only one page, no exceptions.

#ifdef USE_OPENGL
    fxdim = (float) xdim;
    fydim = (float) ydim;
#endif

    videoAllocateBuffers();

#ifdef HIGH_PRECISION_SPRITE
    swallf.Resize(xdim);
#endif

    j = ydim*4;  //Leave room for horizlookup&horizlookup2
    lookups.Resize(2 * j);

    horizlookup = lookups.Data();
    horizlookup2 = lookups.Data() + j;
    horizycent = ((ydim*4)>>1);

    //Force drawrooms to call dosetaspect & recalculate stuff
    oxyaspect = oxdimen = oviewingrange = -1;

    calc_ylookup(bytesperline, ydim);

    videoSetViewableArea(0L,0L,xdim-1,ydim-1);
    videoClearScreen(0L);

    if (searchx < 0) { searchx = halfxdimen; searchy = (ydimen>>1); }

    qsetmode = 200;
    return 0;
}

//
// nextpage
//
void videoNextPage(void)
{
	static bool recursion;

	if (!recursion)
	{
		// This protection is needed because the menu can call scripts from inside its drawers and the scripts can call the busy-looping Screen_Play script event
		// which calls videoNextPage for page flipping again. In this loop the UI drawers may not get called again.
		// Ideally this stuff should be moved out of videoNextPage so that all those busy loops won't call UI overlays at all.
		recursion = true;
		M_Drawer();
		FStat::PrintStat();
		C_DrawConsole();
		recursion = false;
	}


    if (in3dmode())
    {
 		g_beforeSwapTime = timerGetHiTicks();

		videoShowFrame(0);
    }

    faketimerhandler();

#ifdef USE_OPENGL
    omdtims = mdtims;
    mdtims = timerGetTicks();

    for (native_t i = 0; i < Numsprites; ++i)
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
    if ((unsigned)voxindex >= MAXVOXELS)
        return -1;

    auto fil = fileSystem.OpenFileReader(filename, 0);
    if (!fil.isOpen())
        return -1;

    int32_t lengcnt = 0;
    const int32_t lengtot = fil.GetLength();

    for (bssize_t i=0; i<MAXVOXMIPS; i++)
    {
		int32_t dasiz = fil.ReadInt32();

		voxelmemory.Reserve(1);
		voxelmemory.Last() = fil.Read(dasiz);
		voxoff[voxindex][i] = (intptr_t)voxelmemory.Last().Data();

        lengcnt += dasiz+4;
        if (lengcnt >= lengtot-768)
            break;
    }


#ifdef USE_OPENGL
    if (voxmodels[voxindex])
    {
        voxfree(voxmodels[voxindex]);
        voxmodels[voxindex] = NULL;
    }

    Xfree(voxfilenames[voxindex]);
    voxfilenames[voxindex] = Xstrdup(filename);
#endif

    g_haveVoxels = 1;

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
        voxoff[voxindex][j] = 0;
    }
    voxscale[voxindex] = 65536;
    voxrotate[voxindex>>3] &= ~pow2char[voxindex&7];
    tiletovox[tile] = -1;

    // TODO: nextvoxid
}

void vox_deinit()
{
    for (auto &vox : voxmodels)
    {
        voxfree(vox);
        vox = nullptr;
    }
}

//
// inside
//
// See http://fabiensanglard.net/duke3d/build_engine_internals.php,
// "Inside details" for the idea behind the algorithm.
int32_t inside_ps(int32_t x, int32_t y, int16_t sectnum)
{
    if (sectnum >= 0 && sectnum < numsectors)
    {
        int32_t cnt = 0;
        auto wal       = (uwallptr_t)&wall[sector[sectnum].wallptr];
        int  wallsleft = sector[sectnum].wallnum;

        do
        {
            vec2_t v1 = { wal->x - x, wal->y - y };
            auto const &wal2 = *(uwallptr_t)&wall[wal->point2];
            vec2_t v2 = { wal2.x - x, wal2.y - y };

            if ((v1.y^v2.y) < 0)
                cnt ^= (((v1.x^v2.x) < 0) ? (v1.x*v2.y<v2.x*v1.y)^(v1.y<v2.y) : (v1.x >= 0));

            wal++;
        }
        while (--wallsleft);

        return cnt;
    }

    return -1;
}
int32_t inside_old(int32_t x, int32_t y, int16_t sectnum)
{
    if (sectnum >= 0 && sectnum < numsectors)
    {
        uint32_t cnt = 0;
        auto wal       = (uwallptr_t)&wall[sector[sectnum].wallptr];
        int  wallsleft = sector[sectnum].wallnum;

        do
        {
            // Get the x and y components of the [tested point]-->[wall
            // point{1,2}] vectors.
            vec2_t v1 = { wal->x - x, wal->y - y };
            auto const &wal2 = *(uwallptr_t)&wall[wal->point2];
            vec2_t v2 = { wal2.x - x, wal2.y - y };

            // If their signs differ[*], ...
            //
            // [*] where '-' corresponds to <0 and '+' corresponds to >=0.
            // Equivalently, the branch is taken iff
            //   y1 != y2 AND y_m <= y < y_M,
            // where y_m := min(y1, y2) and y_M := max(y1, y2).
            if ((v1.y^v2.y) < 0)
                cnt ^= (((v1.x^v2.x) >= 0) ? v1.x : (v1.x*v2.y-v2.x*v1.y)^v2.y);

            wal++;
        }
        while (--wallsleft);

        return cnt>>31;
    }

    return -1;
}

int32_t inside(int32_t x, int32_t y, int16_t sectnum)
{
    switch (enginecompatibility_mode)
    {
    case ENGINECOMPATIBILITY_NONE:
        break;
    case ENGINECOMPATIBILITY_19950829:
        return inside_ps(x, y, sectnum);
    default:
        return inside_old(x, y, sectnum);
    }
    if ((unsigned)sectnum < (unsigned)numsectors)
    {
        uint32_t cnt1 = 0, cnt2 = 0;

        auto wal       = (uwallptr_t)&wall[sector[sectnum].wallptr];
        int  wallsleft = sector[sectnum].wallnum;

        do
        {
            // Get the x and y components of the [tested point]-->[wall
            // point{1,2}] vectors.
            vec2_t v1 = { wal->x - x, wal->y - y };
            auto const &wal2 = *(uwallptr_t)&wall[wal->point2];
            vec2_t v2 = { wal2.x - x, wal2.y - y };

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
int32_t spriteheightofsptr(uspriteptr_t spr, int32_t *height, int32_t alsotileyofs)
{
    int32_t hei, zofs=0;
    const int32_t picnum=spr->picnum, yrepeat=spr->yrepeat;

    hei = (tilesiz[picnum].y*yrepeat)<<2;
    if (height != NULL)
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
        sprite[spritenum].pos = *newpos;

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
        sprite[spritenum].pos = *newpos;

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

    auto wal = (uwallptr_t)&wall[sector[sectnum].wallptr];
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
int32_t cansee_old(int32_t xs, int32_t ys, int32_t zs, int16_t sectnums, int32_t xe, int32_t ye, int32_t ze, int16_t sectnume)
{
    sectortype *sec, *nsec;
    walltype *wal, *wal2;
    int32_t intx, inty, intz, i, cnt, nextsector, dasectnum, dacnt, danum;

    if ((xs == xe) && (ys == ye) && (sectnums == sectnume)) return 1;
    
    clipsectorlist[0] = sectnums; danum = 1;
    for(dacnt=0;dacnt<danum;dacnt++)
    {
        dasectnum = clipsectorlist[dacnt]; sec = &sector[dasectnum];
        
        for(cnt=sec->wallnum,wal=&wall[sec->wallptr];cnt>0;cnt--,wal++)
        {
            wal2 = &wall[wal->point2];
            if (lintersect(xs,ys,zs,xe,ye,ze,wal->x,wal->y,wal2->x,wal2->y,&intx,&inty,&intz) != 0)
            {
                nextsector = wal->nextsector; if (nextsector < 0) return 0;

                if (intz <= sec->ceilingz) return 0;
                if (intz >= sec->floorz) return 0;
                nsec = &sector[nextsector];
                if (intz <= nsec->ceilingz) return 0;
                if (intz >= nsec->floorz) return 0;

                for(i=danum-1;i>=0;i--)
                    if (clipsectorlist[i] == nextsector) break;
                if (i < 0) clipsectorlist[danum++] = nextsector;
            }
        }

        if (clipsectorlist[dacnt] == sectnume)
            return 1;
    }
    return 0;
}

int32_t cansee(int32_t x1, int32_t y1, int32_t z1, int16_t sect1, int32_t x2, int32_t y2, int32_t z2, int16_t sect2)
{
    if (enginecompatibility_mode == ENGINECOMPATIBILITY_19950829)
        return cansee_old(x1, y1, z1, sect1, x2, y2, z2, sect2);
    int32_t dacnt, danum;
    const int32_t x21 = x2-x1, y21 = y2-y1, z21 = z2-z1;

    static uint8_t sectbitmap[(MAXSECTORS+7)>>3];
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
    Bmemset(sectbitmap, 0, sizeof(sectbitmap));
#ifdef YAX_ENABLE
restart_grand:
#endif
    if (x1 == x2 && y1 == y2)
        return (sect1 == sect2);

#ifdef YAX_ENABLE
    pendingsectnum = -1;
#endif
    sectbitmap[sect1>>3] |= pow2char[sect1&7];
    clipsectorlist[0] = sect1; danum = 1;

    for (dacnt=0; dacnt<danum; dacnt++)
    {
        const int32_t dasectnum = clipsectorlist[dacnt];
        auto const sec = (usectorptr_t)&sector[dasectnum];
        uwallptr_t wal;
        bssize_t cnt;
#ifdef YAX_ENABLE
        int32_t cfz1[2], cfz2[2];  // both wrt dasectnum
        int16_t bn[2];

        yax_getbunches(dasectnum, &bn[0], &bn[1]);
        getzsofslope(dasectnum, x1,y1, &cfz1[0], &cfz1[1]);
        getzsofslope(dasectnum, x2,y2, &cfz2[0], &cfz2[1]);
#endif
        for (cnt=sec->wallnum,wal=(uwallptr_t)&wall[sec->wallptr]; cnt>0; cnt--,wal++)
        {
            auto const wal2 = (uwallptr_t)&wall[wal->point2];
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

                            if (!(sectbitmap[ns>>3] & pow2char[ns&7]) && pendingsectnum==-1)
                            {
                                sectbitmap[ns>>3] |= pow2char[ns&7];
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
            if (!(sectbitmap[nexts>>3] & pow2char[nexts&7]))
            {
                sectbitmap[nexts>>3] |= pow2char[nexts&7];
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

    if (sectbitmap[sect2>>3] & pow2char[sect2&7])
        return 1;

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
        uwallptr_t wal;
        int32_t z;

        for (z=startwall,wal=(uwallptr_t)&wall[startwall]; z<=endwall; z++,wal++)
        {
            auto const wal2 = (uwallptr_t)&wall[wal->point2];
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
            auto const spr = (uspriteptr_t)&sprite[z];

            if (blacklist_sprite_func && blacklist_sprite_func(z))
                continue;

            if (((tagsearch&1) && spr->lotag) || ((tagsearch&2) && spr->hitag))
            {
                if (try_facespr_intersect(spr, sv, vx, vy, 0, &hitv, 1))
                {
                    *neartagsprite = z;
                    *neartaghitdist = dmulscale14(hitv.x-xs, sintable[(ange+2560)&2047],
                                                  hitv.y-ys, sintable[(ange+2048)&2047]);
                }
            }
        }
    }
    while (tempshortcnt < tempshortnum);
}


//
// dragpoint
//
// flags:
//  1: don't reset walbitmap[] (the bitmap of already dragged vertices)
//  2: In the editor, do wall[].cstat |= (1<<14) also for the lastwall().
void dragpoint(int16_t pointhighlight, int32_t dax, int32_t day, uint8_t flags)
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
            walbitmap[w>>3] |= pow2char[w&7];

            for (YAX_ITER_WALLS(w, j, tmpcf))
            {
                if ((walbitmap[j>>3]&pow2char[j&7])==0)
                {
                    walbitmap[j>>3] |= pow2char[j&7];
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

            if ((walbitmap[w>>3] & pow2char[w&7]))
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
            if (walbitmap[w>>3] & pow2char[w&7])
            {
                editwall[w>>3] |= 1<<(w&7);
                if (flags&2)
                {
                    int wn = lastwall(w);
                    editwall[wn>>3] |= 1<<(wn&7);
                }
            }
    }
}

//
// lastwall
//
int32_t lastwall(int16_t point)
{
    if (point > 0 && wall[point-1].point2 == point)
        return point-1;

    int i = point, cnt = numwalls;
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

////////// UPDATESECTOR* FAMILY OF FUNCTIONS //////////

/* Different "is inside" predicates.
 * NOTE: The redundant bound checks are expected to be optimized away in the
 * inlined code. */

static FORCE_INLINE CONSTEXPR int inside_exclude_p(int32_t const x, int32_t const y, int const sectnum, const uint8_t *excludesectbitmap)
{
    return (sectnum>=0 && !bitmap_test(excludesectbitmap, sectnum) && inside_p(x, y, sectnum));
}

/* NOTE: no bound check */
static inline int inside_z_p(int32_t const x, int32_t const y, int32_t const z, int const sectnum)
{
    int32_t cz, fz;
    getzsofslope(sectnum, x, y, &cz, &fz);
    return (z >= cz && z <= fz && inside_p(x, y, sectnum));
}

int32_t getwalldist(vec2_t const in, int const wallnum)
{
    vec2_t closest;
    getclosestpointonwall_internal(in, wallnum, &closest);
    return klabs(closest.x - in.x) + klabs(closest.y - in.y);
}

int32_t getwalldist(vec2_t const in, int const wallnum, vec2_t * const out)
{
    getclosestpointonwall_internal(in, wallnum, out);
    return klabs(out->x - in.x) + klabs(out->y - in.y);
}


int32_t getsectordist(vec2_t const in, int const sectnum, vec2_t * const out /*= nullptr*/)
{
    if (inside_p(in.x, in.y, sectnum))
    {
        if (out)
            *out = in;
        return 0;
    }

    int32_t distance = INT32_MAX;

    auto const sec       = (usectorptr_t)&sector[sectnum];
    int const  startwall = sec->wallptr;
    int const  endwall   = sec->wallptr + sec->wallnum;
    auto       uwal      = (uwallptr_t)&wall[startwall];
    vec2_t     closest = {};

    for (int j = startwall; j < endwall; j++, uwal++)
    {
        vec2_t p;
        int32_t const walldist = getwalldist(in, j, &p);

        if (walldist < distance)
        {
            distance = walldist;
            closest = p;
        }
    }

    if (out)
        *out = closest;

    return distance;
}

int findwallbetweensectors(int sect1, int sect2)
{
    if (sector[sect1].wallnum > sector[sect2].wallnum)
        swaplong(&sect1, &sect2);

    auto const sec  = (usectorptr_t)&sector[sect1];
    int const  last = sec->wallptr + sec->wallnum;

    for (int i = sec->wallptr; i < last; i++)
        if (wall[i].nextsector == sect2)
            return i;

    return -1;
}

//
// updatesector[z]
//
void updatesector(int32_t const x, int32_t const y, int16_t * const sectnum)
{
    if (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE)
    {
        if (inside_p(x, y, *sectnum))
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
    }
    else
    {
        int16_t sect = *sectnum;
        updatesectorneighbor(x, y, &sect, INITIALUPDATESECTORDIST, MAXUPDATESECTORDIST);
        if (sect != -1)
            SET_AND_RETURN(*sectnum, sect);
    }

    // we need to support passing in a sectnum of -1, unfortunately

    for (int i = numsectors - 1; i >= 0; --i)
        if (inside_p(x, y, i))
            SET_AND_RETURN(*sectnum, i);

    *sectnum = -1;
}

void updatesectorexclude(int32_t const x, int32_t const y, int16_t * const sectnum, const uint8_t * const excludesectbitmap)
{
    if (inside_exclude_p(x, y, *sectnum, excludesectbitmap))
        return;

    if (*sectnum >= 0 && *sectnum < numsectors)
    {
        auto wal = (uwallptr_t)&wall[sector[*sectnum].wallptr];
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

void updatesectorz(int32_t const x, int32_t const y, int32_t const z, int16_t * const sectnum)
{
    if (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE)
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
    }
    else
    {
        int16_t sect = *sectnum;
        updatesectorneighborz(x, y, z, &sect, INITIALUPDATESECTORDIST, MAXUPDATESECTORDIST);
        if (sect != -1)
            SET_AND_RETURN(*sectnum, sect);
    }

    // we need to support passing in a sectnum of -1, unfortunately
    for (int i = numsectors - 1; i >= 0; --i)
        if (inside_z_p(x, y, z, i))
            SET_AND_RETURN(*sectnum, i);

    *sectnum = -1;
}

void updatesectorneighbor(int32_t const x, int32_t const y, int16_t * const sectnum, int32_t initialMaxDistance /*= INITIALUPDATESECTORDIST*/, int32_t maxDistance /*= MAXUPDATESECTORDIST*/)
{
    int const initialsectnum = *sectnum;

    if ((unsigned)initialsectnum < (unsigned)numsectors && getsectordist({x, y}, initialsectnum) <= initialMaxDistance)
    {
        if (inside_p(x, y, initialsectnum))
            return;

        static int16_t sectlist[MAXSECTORS];
        static uint8_t sectbitmap[(MAXSECTORS+7)>>3];
        int16_t nsecs;

        bfirst_search_init(sectlist, sectbitmap, &nsecs, MAXSECTORS, initialsectnum);

        for (int sectcnt=0; sectcnt<nsecs; sectcnt++)
        {
            int const listsectnum = sectlist[sectcnt];

            if (inside_p(x, y, listsectnum))
                SET_AND_RETURN(*sectnum, listsectnum);

            auto const sec       = &sector[listsectnum];
            int const  startwall = sec->wallptr;
            int const  endwall   = sec->wallptr + sec->wallnum;
            auto       uwal      = (uwallptr_t)&wall[startwall];

            for (int j=startwall; j<endwall; j++, uwal++)
                if (uwal->nextsector >= 0 && getsectordist({x, y}, uwal->nextsector) <= maxDistance)
                    bfirst_search_try(sectlist, sectbitmap, &nsecs, uwal->nextsector);
        }
    }

    *sectnum = -1;
}

void updatesectorneighborz(int32_t const x, int32_t const y, int32_t const z, int16_t * const sectnum, int32_t initialMaxDistance /*= 0*/, int32_t maxDistance /*= 0*/)
{
    bool nofirstzcheck = false;

    if (*sectnum >= MAXSECTORS && *sectnum - MAXSECTORS < numsectors)
    {
        *sectnum -= MAXSECTORS;
        nofirstzcheck = true;
    }

    uint32_t const correctedsectnum = (unsigned)*sectnum;

    if (correctedsectnum < (unsigned)numsectors && getsectordist({x, y}, correctedsectnum) <= initialMaxDistance)
    {
        int32_t cz, fz;
        getzsofslope(correctedsectnum, x, y, &cz, &fz);

#ifdef YAX_ENABLE
        if (z < cz)
        {
            int const next = yax_getneighborsect(x, y, correctedsectnum, YAX_CEILING);
            if (next >= 0 && z >= getceilzofslope(next, x, y))
                SET_AND_RETURN(*sectnum, next);
        }

        if (z > fz)
        {
            int const next = yax_getneighborsect(x, y, correctedsectnum, YAX_FLOOR);
            if (next >= 0 && z <= getflorzofslope(next, x, y))
                SET_AND_RETURN(*sectnum, next);
        }
#endif
        if ((nofirstzcheck || (z >= cz && z <= fz)) && inside_p(x, y, *sectnum))
            return;

        static int16_t sectlist[MAXSECTORS];
        static uint8_t sectbitmap[(MAXSECTORS+7)>>3];
        int16_t nsecs;

        bfirst_search_init(sectlist, sectbitmap, &nsecs, MAXSECTORS, correctedsectnum);

        for (int sectcnt=0; sectcnt<nsecs; sectcnt++)
        {
            int const listsectnum = sectlist[sectcnt];

            if (inside_z_p(x, y, z, listsectnum))
                SET_AND_RETURN(*sectnum, listsectnum);

            auto const sec       = &sector[listsectnum];
            int const  startwall = sec->wallptr;
            int const  endwall   = sec->wallptr + sec->wallnum;
            auto       uwal      = (uwallptr_t)&wall[startwall];

            for (int j=startwall; j<endwall; j++, uwal++)
                if (uwal->nextsector >= 0 && getsectordist({x, y}, uwal->nextsector) <= maxDistance)
                    bfirst_search_try(sectlist, sectbitmap, &nsecs, uwal->nextsector);
        }
    }

    *sectnum = -1;
}

//
// rotatepoint
//
void rotatepoint(vec2_t const pivot, vec2_t p, int16_t const daang, vec2_t * const p2)
{
    int const dacos = sintable[(daang+2560)&2047];
    int const dasin = sintable[(daang+2048)&2047];
    p.x -= pivot.x;
    p.y -= pivot.y;
    p2->x = dmulscale14(p.x, dacos, -p.y, dasin) + pivot.x;
    p2->y = dmulscale14(p.y, dacos, p.x, dasin) + pivot.y;
}

int32_t setaspect_new_use_dimen = 0;

void videoSetCorrectedAspect()
{
    if (r_usenewaspect && newaspect_enable && videoGetRenderMode() != REND_POLYMER)
    {
        // In DOS the game world is displayed with an aspect of 1.28 instead 1.333,
        // meaning we have to stretch it by a factor of 1.25 instead of 1.2
        // to get perfect squares
        int32_t yx = (65536 * 5) / 4;
        int32_t vr, y, x;

        const int32_t xd = setaspect_new_use_dimen ? xdimen : xdim;
        const int32_t yd = setaspect_new_use_dimen ? ydimen : ydim;

        x = xd;
        y = yd;

        vr = divscale16(x*3, y*4);

        renderSetAspect(vr, yx);
    }
    else
        renderSetAspect(65536, divscale16(ydim*320, xdim*200));
}

//
// setview
//
void videoSetViewableArea(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    windowxy1.x = x1;
    windowxy1.y = y1;
    windowxy2.x = x2;
    windowxy2.y = y2;

    xdimen = (x2-x1)+1; halfxdimen = (xdimen>>1);
    xdimenrecip = divscale32(1L,xdimen);
    ydimen = (y2-y1)+1;

    fxdimen = (float) xdimen;
#ifdef USE_OPENGL
    fydimen = (float) ydimen;
#endif
    videoSetCorrectedAspect();

    for (bssize_t i=0; i<windowxy1.x; i++) { startumost[i] = 1, startdmost[i] = 0; }
    Bassert(windowxy2.x < xdim);  // xdim is the number of alloc'd elements in start*most[].
    for (bssize_t i=windowxy1.x; i<=windowxy2.x; i++)
        { startumost[i] = windowxy1.y, startdmost[i] = windowxy2.y+1; }
    for (bssize_t i=windowxy2.x+1; i<xdim; i++) { startumost[i] = 1, startdmost[i] = 0; }
}


//
// setaspect
//
void renderSetAspect(int32_t daxrange, int32_t daaspect)
{
    if (daxrange == 65536) daxrange--;  // This doesn't work correctly with 65536. All other values are fine. No idea where this is evaluated wrong.
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



#include "v_2ddrawer.h"
//
// rotatesprite
//
void rotatesprite_(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                   int8_t dashade, uint8_t dapalnum, int32_t dastat, uint8_t daalpha, uint8_t dablend,
                   int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2)
{
    if ((unsigned)picnum >= MAXTILES)
        return;

    if ((cx1 > cx2) || (cy1 > cy2)) return;
    if (z <= 16) return;
    tileUpdatePicnum(&picnum, (int16_t)0xc000);
    if ((tilesiz[picnum].x <= 0) || (tilesiz[picnum].y <= 0)) return;

    if (r_rotatespritenowidescreen)
    {
        dastat |= RS_STRETCH;
        dastat &= ~RS_ALIGN_MASK;
    }

    if (hw_models && tile2model[picnum].hudmem[(dastat & 4) >> 2])
    {
        polymost_dorotatespritemodel(sx, sy, z, a, picnum, dashade, dapalnum, dastat, daalpha, dablend, guniqhudid);
        return;
    }
    // We must store all calls in the 2D drawer so that the backend can operate on a clean 3D view.
    twod->rotatesprite(sx, sy, z, a, picnum, dashade, dapalnum, dastat, daalpha, dablend, cx1, cy1, cx2, cy2);

    // RS_PERM code was removed because the current backend supports only one page that needs to be redrawn each frame in which case the perm list was skipped anyway.
}




//
// clearview
//
void videoClearViewableArea(int32_t dacol)
{
    if (!in3dmode() && dacol != -1) return;

    if (dacol == -1) dacol = 0;

#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        palette_t const p = paletteGetColor(dacol);

        GLInterface.ClearScreen(PalEntry(p.r, p.g, p.b), false);
        return;
    }
#endif

    videoBeginDrawing(); //{{{
    //dacol += (dacol<<8); dacol += (dacol<<16);
    int const dx = windowxy2.x-windowxy1.x+1;
    intptr_t p = frameplace+ylookup[windowxy1.y]+windowxy1.x;
    for (bssize_t y=windowxy1.y; y<=windowxy2.y; ++y)
    {
        //clearbufbyte((void*)p,dx,dacol);
        Bmemset((void *)p,dacol,dx);
        p += ylookup[1];
    }
    videoEndDrawing();   //}}}

    faketimerhandler();
}


//
// clearallviews
//
void videoClearScreen(int32_t dacol)
{
    if (!in3dmode()) return;
    //dacol += (dacol<<8); dacol += (dacol<<16);

#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        palette_t const p = paletteGetColor(dacol);
        GLInterface.ClearScreen(PalEntry(255, p.r, p.g, p.b));
        return;
    }
#endif

    videoBeginDrawing(); //{{{
    Bmemset((void *)frameplace,dacol,bytesperline*ydim);
    videoEndDrawing();   //}}}
    //nextpage();

    faketimerhandler();
}


//MUST USE RESTOREFORDRAWROOMS AFTER DRAWING

//
// setviewtotile
//
void renderSetTarget(int16_t tilenume, int32_t xsiz, int32_t ysiz)
{
    if (setviewcnt >= MAXSETVIEW-1)
        return;
    if (xsiz <= 0 ||
        ysiz <= 0)
        return;

    //DRAWROOMS TO TILE BACKUP&SET CODE
	TileFiles.tileCreate(tilenume, xsiz, ysiz);
    bakxsiz[setviewcnt] = xdim; bakysiz[setviewcnt] = ydim;
    bakframeplace[setviewcnt] = frameplace; frameplace = (intptr_t)tilePtr(tilenume);
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
    xdim = ysiz;
    ydim = xsiz;
    videoSetViewableArea(0,0,ysiz-1,xsiz-1);
    renderSetAspect(65536,65536);

    calc_ylookup(ysiz, xsiz);
}


//
// setviewback
//
void renderRestoreTarget(void)
{
    if (setviewcnt <= 0) return;
    setviewcnt--;

    offscreenrendering = (setviewcnt>0);
#ifdef USE_OPENGL
    if (setviewcnt == 0)
    {
        rendmode = bakrendmode;
        tileInvalidate(baktile,-1,-1);
    }
#endif

    xdim = bakxsiz[setviewcnt];
    ydim = bakysiz[setviewcnt];
    videoSetViewableArea(bakwindowxy1[setviewcnt].x,bakwindowxy1[setviewcnt].y,
            bakwindowxy2[setviewcnt].x,bakwindowxy2[setviewcnt].y);
    copybufbyte(&bakumost[windowxy1.x],&startumost[windowxy1.x],(windowxy2.x-windowxy1.x+1)*sizeof(startumost[0]));
    copybufbyte(&bakdmost[windowxy1.x],&startdmost[windowxy1.x],(windowxy2.x-windowxy1.x+1)*sizeof(startdmost[0]));
    frameplace = bakframeplace[setviewcnt];

    calc_ylookup((setviewcnt == 0) ? bytesperline : bakxsiz[setviewcnt],
                 bakysiz[setviewcnt]);

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

    uint8_t *ptr1, *ptr2;
	auto p = tileData(tilenume);
	if (!p) return;	// safety precaution, this may only be called on writable tiles for camera textures.

    for (bssize_t i=siz-1, j; i>=3; i-=4)
    {
        ptr2 = ptr1 = (p+i*(siz+1));
        swapchar(--ptr1, (ptr2 -= siz));
        for (j=(i>>1)-1; j>=0; --j)
            swapchar2((ptr1 -= 2), (ptr2 -= (siz<<1)), siz);

        ptr2 = ptr1 = (p+(i-1)*(siz+1));
        for (j=((i-1)>>1)-1; j>=0; --j)
            swapchar2((ptr1 -= 2), (ptr2 -= (siz<<1)), siz);

        ptr2 = ptr1 = (p+(i-2)*(siz+1));
        swapchar(--ptr1, (ptr2 -= siz));

        for (j=((i-2)>>1)-1; j>=0; --j)
            swapchar2((ptr1 -= 2), (ptr2 -= (siz<<1)), siz);

        ptr2 = ptr1 = (p+(i-3)*(siz+1));

        for (j=((i-3)>>1)-1; j>=0; --j)
            swapchar2((ptr1 -= 2), (ptr2 -= (siz<<1)), siz);
    }
}

//
// preparemirror
//
void renderPrepareMirror(int32_t dax, int32_t day, int32_t daz, fix16_t daang, fix16_t dahoriz, int16_t dawall,
                         int32_t *tposx, int32_t *tposy, fix16_t *tang)
{
    const int32_t x = wall[dawall].x, dx = wall[wall[dawall].point2].x-x;
    const int32_t y = wall[dawall].y, dy = wall[wall[dawall].point2].y-y;

    const int32_t j = dx*dx + dy*dy;
    if (j == 0)
        return;

    int i = ((dax-x)*dx + (day-y)*dy)<<1;

    *tposx = (x<<1) + scale(dx,i,j) - dax;
    *tposy = (y<<1) + scale(dy,i,j) - day;
    *tang  = (fix16_from_int(getangle(dx, dy) << 1) - daang) & 0x7FFFFFF;

    inpreparemirror = 1;

#ifdef USE_OPENGL
    if (videoGetRenderMode() == REND_POLYMOST)
        polymost_prepareMirror(dax, day, daz, daang, dahoriz, dawall);
#endif
}


//
// completemirror
//
void renderCompleteMirror(void)
{
#ifdef USE_OPENGL
    if (videoGetRenderMode() == REND_POLYMOST)
        polymost_completeMirror();
#endif

    // Don't try to complete a mirror if we haven't drawn the reflection for one
    if (!inpreparemirror) { return; }
    inpreparemirror = 0;

#ifdef USE_OPENGL
    if (videoGetRenderMode() != REND_CLASSIC)
        return;
#endif

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

    videoBeginDrawing();

    // Width in pixels (screen x's are inclusive on both sides):
    int const width = mirrorsx2-mirrorsx1+1;
    // Height in pixels (screen y's are half-open because they come from umost/dmost):
    int const height = mirrorsy2-mirrorsy1;

    // Address of the mirror wall's top left corner in the source scene:
    intptr_t s = (intptr_t) mirrorBuffer.Data() + ylookup[windowxy1.y+mirrorsy1] + windowxy1.x+mirrorsx1;

    // Pointer to the mirror line's left corner in the destination:
    intptr_t d = (intptr_t) frameplace + ylookup[windowxy1.y+mirrorsy1] + windowxy2.x-mirrorsx2;

    for (bssize_t y=0; y<height; y++)
    {
#if 0
        if ((p-mirrorBuffer) + width-1 >= bytesperline*ydim)
            printf("oob read: mirrorsx1=%d, mirrorsx2=%d\n", mirrorsx1, mirrorsx2);
#endif
        copybufreverse((void *)(s+width-1), (void *)(d+1), width);

        s += ylookup[1];
        d += ylookup[1];
        faketimerhandler();
    }

    videoEndDrawing();
}


//
// sectorofwall
//
static int32_t sectorofwall_internal(int16_t wallNum)
{
    native_t gap = numsectors>>1, sectNum = gap;

    while (gap > 1)
    {
        gap >>= 1;
        native_t const n = !!(sector[sectNum].wallptr < wallNum);
        sectNum += (n ^ (n - 1)) * gap;
    }
    while (sector[sectNum].wallptr > wallNum) sectNum--;
    while (sector[sectNum].wallptr + sector[sectNum].wallnum <= wallNum) sectNum++;

    return sectNum;
}

int32_t sectorofwall(int16_t wallNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)wallNum >= (unsigned)numwalls))
        return -1;

    native_t const w = wall[wallNum].nextwall;

    return ((unsigned)w < MAXWALLS) ? wall[w].nextsector : sectorofwall_internal(wallNum);
}

int32_t sectorofwall_noquick(int16_t wallNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned) wallNum >= (unsigned) numwalls))
        return -1;

    return sectorofwall_internal(wallNum);
}


int32_t getceilzofslopeptr(usectorptr_t sec, int32_t dax, int32_t day)
{
    if (!(sec->ceilingstat&2))
        return sec->ceilingz;

    auto const wal  = (uwallptr_t)&wall[sec->wallptr];
    auto const wal2 = (uwallptr_t)&wall[wal->point2];

    vec2_t const w = *(vec2_t const *)wal;
    vec2_t const d = { wal2->x - w.x, wal2->y - w.y };

    int const i = nsqrtasm(uhypsq(d.x,d.y))<<5;
    if (i == 0) return sec->ceilingz;

    int const j = dmulscale3(d.x, day-w.y, -d.y, dax-w.x);
    int const shift = enginecompatibility_mode != ENGINECOMPATIBILITY_NONE ? 0 : 1;
    return sec->ceilingz + (scale(sec->ceilingheinum,j>>shift,i)<<shift);
}

int32_t getflorzofslopeptr(usectorptr_t sec, int32_t dax, int32_t day)
{
    if (!(sec->floorstat&2))
        return sec->floorz;

    auto const wal  = (uwallptr_t)&wall[sec->wallptr];
    auto const wal2 = (uwallptr_t)&wall[wal->point2];

    vec2_t const w = *(vec2_t const *)wal;
    vec2_t const d = { wal2->x - w.x, wal2->y - w.y };

    int const i = nsqrtasm(uhypsq(d.x,d.y))<<5;
    if (i == 0) return sec->floorz;

    int const j = dmulscale3(d.x, day-w.y, -d.y, dax-w.x);
    int const shift = enginecompatibility_mode != ENGINECOMPATIBILITY_NONE ? 0 : 1;
    return sec->floorz + (scale(sec->floorheinum,j>>shift,i)<<shift);
}

void getzsofslopeptr(usectorptr_t sec, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    *ceilz = sec->ceilingz; *florz = sec->floorz;

    if (((sec->ceilingstat|sec->floorstat)&2) != 2)
        return;

    auto const wal  = (uwallptr_t)&wall[sec->wallptr];
    auto const wal2 = (uwallptr_t)&wall[wal->point2];

    vec2_t const d = { wal2->x - wal->x, wal2->y - wal->y };

    int const i = nsqrtasm(uhypsq(d.x,d.y))<<5;
    if (i == 0) return;

    int const j = dmulscale3(d.x,day-wal->y, -d.y,dax-wal->x);
    int const shift = enginecompatibility_mode != ENGINECOMPATIBILITY_NONE ? 0 : 1;
    if (sec->ceilingstat&2)
        *ceilz += scale(sec->ceilingheinum,j>>shift,i)<<shift;
    if (sec->floorstat&2)
        *florz += scale(sec->floorheinum,j>>shift,i)<<shift;
}

#ifdef YAX_ENABLE
void yax_getzsofslope(int sectNum, int playerX, int playerY, int32_t *pCeilZ, int32_t *pFloorZ)
{
    int didCeiling = 0;
    int didFloor   = 0;
    int testSector = 0;

    if ((sector[sectNum].ceilingstat & 512) == 0)
    {
        testSector = yax_getneighborsect(playerX, playerY, sectNum, YAX_CEILING);

        if (testSector >= 0)
        {
ceiling:
            *pCeilZ    = getcorrectceilzofslope(testSector, playerX, playerY);
            didCeiling = 1;
        }
    }

    if ((sector[sectNum].floorstat & 512) == 0)
    {
        testSector = yax_getneighborsect(playerX, playerY, sectNum, YAX_FLOOR);

        if (testSector >= 0)
        {
floor:
            *pFloorZ = getcorrectflorzofslope(testSector, playerX, playerY);
            didFloor = 1;
        }
    }

    testSector = sectNum;

    if (!didCeiling)
        goto ceiling;
    else if (!didFloor)
        goto floor;
}
#endif

//
// alignceilslope
//
void alignceilslope(int16_t dasect, int32_t x, int32_t y, int32_t z)
{
    auto const wal = (uwallptr_t)&wall[sector[dasect].wallptr];
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
    auto const wal = (uwallptr_t)&wall[sector[dasect].wallptr];
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

    Xfree(tmpwall);
}



//
// setrendermode
//
int32_t videoSetRenderMode(int32_t renderer)
{
    UNREFERENCED_PARAMETER(renderer);

#ifdef USE_OPENGL
    if (bpp == 8)
    {
        GLInterface.EnableBlend(false);
        GLInterface.EnableAlphaTest(false);
        renderer = REND_CLASSIC;
    }

    else renderer = REND_POLYMOST;

    basepalreset = 1;

    rendmode = renderer;
    if (videoGetRenderMode() >= REND_POLYMOST)
        glrendmode = rendmode;

#endif

    return 0;
}

//
// setrollangle
//
#ifdef USE_OPENGL
void renderSetRollAngle(int32_t rolla)
{
    gtang = (float)rolla * (fPI * (1.f/1024.f));
}
#endif
