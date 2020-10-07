// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#define engine_c_

#include "gl_load.h"
#include "build.h"
#include "automap.h"

#include "imagehelpers.h"
#include "compat.h"
#include "engine_priv.h"
#include "palette.h"
#include "pragmas.h"
#include "scriptfile.h"
#include "gamecvars.h"
#include "c_console.h"
#include "v_2ddrawer.h"
#include "v_draw.h"
#include "stats.h"
#include "menu.h"
#include "version.h"
#include "earcut.hpp"
#include "gamestate.h"
#include "inputstate.h"
#include "printf.h"

#ifdef USE_OPENGL
# include "mdsprite.h"
# include "polymost.h"
#include "v_video.h"
#include "../../glbackend/glbackend.h"
#include "gl_renderer.h"
#endif


int32_t rendmode=0;
int32_t glrendmode = REND_POLYMOST;
int32_t r_rortexture = 0;
int32_t r_rortexturerange = 0;
int32_t r_rorphase = 0;
int32_t mdtims, omdtims;

float fcosglobalang, fsinglobalang;
float fxdim, fydim, fydimen, fviewingrange;

uint8_t globalr = 255, globalg = 255, globalb = 255;

int16_t pskybits_override = -1;

// This was on the cache but is permanently allocated, so put it into something static. This needs some rethinking anyway
static TArray<TArray<uint8_t>> voxelmemory;

int16_t tiletovox[MAXTILES];
#ifdef USE_OPENGL
char *voxfilenames[MAXVOXELS];
#endif
char g_haveVoxels;
//#define kloadvoxel loadvoxel

int32_t novoxmips = 1;

int32_t voxscale[MAXVOXELS];

static int32_t beforedrawrooms = 1;

static int32_t oxdimen = -1, oviewingrange = -1, oxyaspect = -1;

int32_t globalflags;

static int8_t tempbuf[MAXWALLS];

// referenced from asm
int32_t reciptable[2048];
intptr_t asm1, asm2;
int32_t globalx1, globaly2, globalx3, globaly3;

static int32_t no_radarang2 = 0;
static int16_t radarang[1280];
static int32_t qradarang[10240];

uint16_t ATTRIBUTE((used)) sqrtable[4096], ATTRIBUTE((used)) shlookup[4096+256], ATTRIBUTE((used)) sqrtable_old[2048];

static char kensmessage[128];
const char *engineerrstr = "No error";

int32_t showfirstwall=0;
int32_t showheightindicators=1;
int32_t circlewall=-1;

int16_t editstatus = 0;
static fixed_t global100horiz;  // (-100..300)-scale horiz (the one passed to drawrooms)

static FString printcoords(void)
{
    FString str;

    str.Format(
        "pos.x: %d\n"
        "pos.y: %d\n"
        "pos.z: %d\n"
        "ang  : %d\n"
        "horiz: %d\n",
        globalposx, globalposy,
        globalposz, globalang, 
        FixedToInt(global100horiz)
    );

    return str;
}

CCMD(printcoords)
{
    Printf("%s", printcoords().GetChars());
}

ADD_STAT(printcoords)
{
    return printcoords();
}

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

    *closest = { (int32_t)(w.x + ((d.x * i) >> 30)), (int32_t)(w.y + ((d.y * i) >> 30)) };
}

int32_t xb1[MAXWALLSB];  // Polymost uses this as a temp array
static int32_t xb2[MAXWALLSB];
int32_t rx1[MAXWALLSB], ry1[MAXWALLSB];
int16_t bunchp2[MAXWALLSB], thesector[MAXWALLSB];

int16_t bunchfirst[MAXWALLSB], bunchlast[MAXWALLSB];


static vec3_t spritesxyz[MAXSPRITESONSCREEN+1];

int32_t xdimen = -1, xdimenrecip, halfxdimen, xdimenscale, xdimscale;
float fxdimen = -1.f;
int32_t ydimen;

int32_t rxi[8], ryi[8];

int32_t globalposx, globalposy, globalposz;
fixed_t qglobalhoriz;
float fglobalposx, fglobalposy, fglobalposz;
int16_t globalang, globalcursectnum;
fixed_t qglobalang;
int32_t globalpal, globalfloorpal, cosglobalang, singlobalang;
int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;

int32_t xyaspect;
int32_t viewingrangerecip;

static char globalxshift, globalyshift;
static int32_t globalxpanning, globalypanning;
int32_t globalshade, globalorientation;
int16_t globalpicnum;


static int32_t globaly1, globalx2;

int16_t sectorborder[256];
int16_t pointhighlight=-1, linehighlight=-1, highlightcnt=0;

int32_t halfxdim16, midydim16;

static_assert(MAXWALLSB < INT16_MAX);
int16_t numscans, numbunches;
static int16_t numhits;

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


//
// Internal Engine Functions
//

// returns: 0=continue sprite collecting;
//          1=break out of sprite collecting;
int32_t renderAddTsprite(int16_t z, int16_t sectnum)
{
    auto const spr = (uspriteptr_t)&sprite[z];
    if (spritesortcnt >= maxspritesonscreen)
        return 1;

    renderAddTSpriteFromSprite(z);


    return 0;
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

    int const i = (int) I_GetBuildTime() >> (picanm[tilenum].sf & PICANM_ANIMSPEED_MASK);
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


// globalpicnum --> globalxshift, globalyshift
static void calc_globalshifts(void)
{
    globalxshift = (8-widthBits(globalpicnum));
    globalyshift = (8-heightBits(globalpicnum));
    if (globalorientation&8) { globalxshift++; globalyshift++; }
    // PK: the following can happen for large (>= 512) tile sizes.
    // NOTE that global[xy]shift are unsigned chars.
    if (globalxshift > 31) globalxshift=0;
    if (globalyshift > 31) globalyshift=0;
}

static void renderDrawSprite(int32_t snum)
{
    polymost_drawsprite(snum);
}


//
// drawmaskwall (internal)
//
static void renderDrawMaskedWall(int16_t damaskwallcnt)
{
    if (videoGetRenderMode() == REND_POLYMOST) 
    { 
        polymost_drawmaskwall(damaskwallcnt); return; 
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
            temp += 1-int(2*root);
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
    }

    if (xdimen != oxdimen || viewingrange != oviewingrange)
    {
        int32_t k, x, xinc;

        no_radarang2 = 0;
        oviewingrange = viewingrange;

        xinc = mulscale32(viewingrange*2560,xdimenrecip);
        x = IntToFixed(5120)-mulscale1(xinc,xdimen);

        for (i=0; i<xdimen; i++)
        {
            j = (x&65535); k = FixedToInt(x); x += xinc;

            if (k < 0 || k >= (int32_t)countof(qradarang)-1)
            {
                no_radarang2 = 1;
                break;
            }

            if (j != 0)
                j = mulscale16(qradarang[k+1]-qradarang[k], j);
        }

        oxdimen = xdimen;
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

        for (i=0; i<=512; i++)
            sintable[i] = (int16_t)(16384.f * sinf((float)i * BANG2RAD) + 0.0001f);
        for (i=513; i<1024; i++)
            sintable[i] = sintable[1024-i];
        for (i=1024; i<2048; i++)
            sintable[i] = -sintable[i-1024];

        for (i=0; i<640; i++)
            radarang[i] = (int16_t)(atanf(((float)(640-i)-0.5f) * (1.f/160.f)) * (-64.f * (1.f/BANG2RAD)) + 0.0001f);
        for (i=0; i<640; i++)
            radarang[1279-i] = -radarang[i];

        for (i=0; i<5120; i++)
            qradarang[i] = FloatToFixed(atanf(((float)(5120-i)-0.5f) * (1.f/1280.f)) * (-64.f * (1.f/BANG2RAD)));
        for (i=0; i<5120; i++)
            qradarang[10239-i] = -qradarang[i];

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
        assert((unsigned)sectnum < MAXSECTORS);

        do_insertsprite_at_headofsect(newspritenum, sectnum);
        Numsprites++;
    }

    return newspritenum;

}

//
// deletesprite
//
int32_t (*deletesprite_replace)(int16_t spritenum) = NULL;
void polymost_deletesprite(int num);
int32_t deletesprite(int16_t spritenum)
{
    polymost_deletesprite(spritenum);
    if (deletesprite_replace)
        return deletesprite_replace(spritenum);
    assert((sprite[spritenum].statnum == MAXSTATUS)
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
        t = (t << 24) / rayLengthSquared;

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

    int64_t t = (int64_t(originDiffCrossLineVec) << 24) / rayCrossLineVec;
    // For sake of completeness/readability, alternative to the above approach for an early out & avoidance of an extra division:

    *intersectionX = originX + mulscale24(ray.x, t);
    *intersectionY = originY + mulscale24(ray.y, t);
    *intersectionZ = originZ + mulscale24(destZ-originZ, t);

    return 1;
}

//
// rintersect (internal)
//
// returns: -1 if didn't intersect, coefficient IntToFixed(x3--x4 fraction) else
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

    int64_t t = (topt << 16) / bot;
    *intx = x1 + ((vx*t) >> 16);
    *inty = y1 + ((vy*t) >> 16);
    *intz = z1 + ((vz*t) >> 16);

    t = (topu << 16) / bot;

    assert((unsigned)t < 65536);

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
    for (auto& sky : multipskies)
        if (tilenum == sky.tilenum)
        {
            sky.combinedtile = -1;  // invalidate the old content
            return &sky;
        }

    multipskies.Reserve(1);
    multipskies.Last() = {};
    multipskies.Last().tilenum = tilenum;
    multipskies.Last().combinedtile = -1;
    multipskies.Last().yscale = 65536;
    return &multipskies.Last();
}

psky_t * defineSky(int32_t const tilenum, int horiz, int lognumtiles, const uint16_t *tileofs, int yoff)
{
    auto sky = tileSetupSky(tilenum);
    sky->horizfrac = horiz;
    sky->lognumtiles = lognumtiles;
    sky->yoffs = yoff;
    memcpy(sky->tileofs, tileofs, 2 << lognumtiles);
    return sky;
}

// Get properties of parallaxed sky to draw.
// Returns: pointer to tile offset array. Sets-by-pointer the other three.
const int16_t* getpsky(int32_t picnum, int32_t* dapyscale, int32_t* dapskybits, int32_t* dapyoffs, int32_t* daptileyscale)
{
    psky_t const* const psky = getpskyidx(picnum);

    if (dapskybits)
        *dapskybits = (pskybits_override == -1 ? psky->lognumtiles : pskybits_override);
    if (dapyscale)
        *dapyscale = (parallaxyscale_override == 0 ? psky->horizfrac : parallaxyscale_override);
    if (dapyoffs)
        *dapyoffs = psky->yoffs + parallaxyoffs_override;
    if (daptileyscale)
        *daptileyscale = psky->yscale;

    return psky->tileofs;
}



//
// preinitengine
//
static int32_t preinitcalled = 0;

static spriteext_t spriteext_s[MAXSPRITES+MAXUNIQHUDID];
static spritesmooth_t spritesmooth_s[MAXSPRITES+MAXUNIQHUDID];
static sectortype sector_s[MAXSECTORS];
static walltype wall_s[MAXWALLS];
static wallext_t wallext_s[MAXWALLS];
static spritetype sprite_s[MAXSPRITES];
static tspritetype tsprite_s[MAXSPRITESONSCREEN];

int32_t enginePreInit(void)
{
	polymost_initosdfuncs();

    sector = sector_s;
    wall = wall_s;
    wallext = wallext_s;
    sprite = sprite_s;
    tsprite = tsprite_s;
    spriteext = spriteext_s;
    spritesmooth = spritesmooth_s;


    preinitcalled = 1;
    return 0;
}


void (*paletteLoadFromDisk_replace)(void) = NULL;   // replacement hook for Blood.

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

    if (engineLoadTables())
        return 1;

    xyaspect = -1;

    showinvisibility = 0;

	voxelmemory.Reset();

    for (i=0; i<MAXTILES; i++)
        tiletovox[i] = -1;
    for (auto& v : voxscale) v = 65536;
    memset(voxrotate, 0, sizeof(voxrotate));

    paletteloaded = 0;

    searchit = 0; searchstat = -1;

    g_visibility = 512;
    parallaxvisibility = 512;

    maxspritesonscreen = MAXSPRITESONSCREEN;

    GPalette.Init(MAXPALOOKUPS + 1);    // one slot for each translation, plus a separate one for the base palettes.
    if (paletteLoadFromDisk_replace)
    {
        paletteLoadFromDisk_replace();
    }
    else
    {
        paletteLoadFromDisk();
    }

#ifdef USE_OPENGL
    if (!mdinited) mdinit();
#endif

    return 0;
}

//
// uninitengine
//

void engineUnInit(void)
{
    polymost_glreset();
    freeallmodels();
# ifdef POLYMER
    polymer_uninit();
# endif

	TileFiles.CloseAll();
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


void set_globalang(fixed_t const ang)
{
    globalang = FixedToInt(ang)&2047;
    qglobalang = ang & 0x7FFFFFF;

    float const f_ang = FixedToFloat(ang);
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
EXTERN_CVAR(Int, gl_fogmode)
int32_t renderDrawRoomsQ16(int32_t daposx, int32_t daposy, int32_t daposz,
                           fixed_t daang, fixed_t dahoriz, int16_t dacursectnum)
{
    int32_t i;

    if (gl_fogmode == 1) gl_fogmode = 2;	// only radial fog works with Build's screwed up coordinate system.

    set_globalpos(daposx, daposy, daposz);
    set_globalang(daang);

    global100horiz = dahoriz;

    // xdimenscale is scale(xdimen,yxaspect,320);
    // normalization by viewingrange so that center-of-aim doesn't depend on it
    qglobalhoriz = mulscale16(dahoriz, divscale16(xdimenscale, viewingrange))+IntToFixed(ydimen>>1);

    globalcursectnum = dacursectnum;

    if ((xyaspect != oxyaspect) || (xdimen != oxdimen) || (viewingrange != oviewingrange))
        dosetaspect();

    memset(gotsector, 0, sizeof(gotsector));

    i = xdimen-1;

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
                assert(tile.newtile != -1);

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

    polymost_drawrooms();

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


static inline int32_t         sameside(const _equation *eq, const vec2f_t *p1, const vec2f_t *p2)
{
    const float sign1 = (eq->a * p1->x) + (eq->b * p1->y) + eq->c;
    const float sign2 = (eq->a * p2->x) + (eq->b * p2->y) + eq->c;
    return (sign1 * sign2) > 0.f;
}


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
                std::swap(tspriteptr[l],tspriteptr[l+gap]);
                std::swap(spritesxyz[l].x,spritesxyz[l+gap].x);
                std::swap(spritesxyz[l].y,spritesxyz[l+gap].y);
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
                    int32_t yoff = tileTopOffset(s->picnum) + s->yoffset;
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
                        std::swap(tspriteptr[k], tspriteptr[l]);
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
# define debugmask_add(dispidx, idx) do {} while (0)
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
    renderBeginScene();

#ifdef USE_OPENGL
    if (videoGetRenderMode() == REND_POLYMOST)
    {
        GLInterface.EnableBlend(false);
        GLInterface.EnableAlphaTest(true);
        GLInterface.SetDepthBias(-2, -256);

        if (spritesortcnt < numSprites)
        {
            i = spritesortcnt;
            for (bssize_t i = spritesortcnt; i < numSprites;)
            {
                int32_t py = spritesxyz[i].y;
                int32_t pcstat = tspriteptr[i]->cstat & 48;
                int32_t pangle = tspriteptr[i]->ang;
                int j = i + 1;
                if (!polymost_spriteIsModelOrVoxel(tspriteptr[i]))
                {
                    while (j < numSprites && py == spritesxyz[j].y && pcstat == (tspriteptr[j]->cstat & 48) && (pcstat != 16 || pangle == tspriteptr[j]->ang)
                        && !polymost_spriteIsModelOrVoxel(tspriteptr[j]))
                    {
                        j++;
                    }
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
        renderDrawMaskedWall(maskwallcnt);
    }

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
    renderFinishScene();
	GLInterface.SetDepthMask(true);
	GLInterface.SetDepthBias(0, 0);
}


//==========================================================================
//
//
//
//==========================================================================

void FillPolygon(int* rx1, int* ry1, int* xb1, int32_t npoints, int picnum, int palette, int shade, int props, const FVector2& xtex, const FVector2& ytex, const FVector2& otex,
    int clipx1, int clipy1, int clipx2, int clipy2)
{
    //Convert int32_t to float (in-place)
    TArray<FVector4> points(npoints, true);
    using Point = std::pair<float, float>;
    std::vector<std::vector<Point>> polygon;
    std::vector<Point>* curPoly;

    polygon.resize(1);
    curPoly = &polygon.back();

    for (bssize_t i = 0; i < npoints; ++i)
    {
        auto X = ((float)rx1[i]) * (1.0f / 4096.f);
        auto Y = ((float)ry1[i]) * (1.0f / 4096.f);
        curPoly->push_back(std::make_pair(X, Y));
        if (xb1[i] < i && i < npoints - 1)
        {
            polygon.resize(polygon.size() + 1);
            curPoly = &polygon.back();
        }
    }
    // Now make sure that the outer boundary is the first polygon by picking a point that's as much to the outside as possible.
    int outer = 0;
    float minx = FLT_MAX;
    float miny = FLT_MAX;
    for (size_t a = 0; a < polygon.size(); a++)
    {
        for (auto& pt : polygon[a])
        {
            if (pt.first < minx || (pt.first == minx && pt.second < miny))
            {
                minx = pt.first;
                miny = pt.second;
                outer = a;
            }
        }
    }
    if (outer != 0) std::swap(polygon[0], polygon[outer]);
    auto indices = mapbox::earcut(polygon);

    int p = 0;
    for (size_t a = 0; a < polygon.size(); a++)
    {
        for (auto& pt : polygon[a])
        {
            FVector4 point = { pt.first, pt.second, float(pt.first * xtex.X + pt.second * ytex.X + otex.X), float(pt.first * xtex.Y + pt.second * ytex.Y + otex.Y) };
            points[p++] = point;
        }
    }

    int maskprops = (props >> 7) & DAMETH_MASKPROPS;
    FRenderStyle rs = LegacyRenderStyles[STYLE_Translucent];
    double alpha = 1.;
    if (maskprops > DAMETH_MASK)
    {
        rs = GetRenderStyle(0, maskprops == DAMETH_TRANS2);
        alpha = GetAlphaFromBlend(maskprops, 0);
    }
    int translation = TRANSLATION(Translation_Remap + curbasepal, palette);
    int light = clamp(scale((numshades - shade), 255, numshades), 0, 255);
    PalEntry pe = PalEntry(uint8_t(alpha*255), light, light, light);

    twod->AddPoly(tileGetTexture(picnum), points.Data(), points.Size(), indices.data(), indices.size(), translation, pe, rs, clipx1, clipy1, clipx2, clipy2);
}

//==========================================================================
//
//
//
//==========================================================================


#include "build.h"
#include "../src/engine_priv.h"


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
    FillPolygon(rx1, ry1, xb1, npoints, globalpicnum, globalpal, globalshade, globalorientation, xtex, ytex, otex, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
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

    memset(gotsector, 0, sizeof(gotsector));

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
        if (gFullMap || show2dsector[s])
        {
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

            globalfloorpal = globalpal = sec->floorpal;

            globalpicnum = sec->floorpicnum;
            if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
            tileUpdatePicnum(&globalpicnum, s);
            setgotpic(globalpicnum);
            if ((tilesiz[globalpicnum].x <= 0) || (tilesiz[globalpicnum].y <= 0)) continue;

			globalshade = max(min<int>(sec->floorshade, numshades - 1), 0);
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
                std::swap(tsprite[j].owner, tsprite[j+gap].owner);
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

            //relative alignment stuff
            ox = v2.x-v1.x; oy = v2.y-v1.y;
            i = ox*ox+oy*oy; if (i == 0) continue; i = 65536*16384 / i;
            globalx1 = mulscale10(dmulscale10(ox,bakgvect.x,oy,bakgvect.y),i);
            globaly1 = mulscale10(dmulscale10(ox,bakgvect.y,-oy,bakgvect.x),i);
            ox = v1.y-v4.y; oy = v4.x-v1.x;
            i = ox*ox+oy*oy; if (i == 0) continue; i = 65536 * 16384 / i;
            globalx2 = mulscale10(dmulscale10(ox,bakgvect.x,oy,bakgvect.y),i);
            globaly2 = mulscale10(dmulscale10(ox,bakgvect.y,-oy,bakgvect.x),i);

            ox = widthBits(globalpicnum); 
            oy = heightBits(globalpicnum);
            if ((1 << ox) != xspan)
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


        renderSetAspect(oviewingrange, oyxaspect);
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

    strcpy(kensmessage,"!!!! BUILD engine&tools programmed by Ken Silverman of E.G. RI."
           "  (c) Copyright 1995 Ken Silverman.  Summary:  BUILD = Ken. !!!!");

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

    j = ydim*4;  //Leave room for horizlookup&horizlookup2

    //Force drawrooms to call dosetaspect & recalculate stuff
    oxyaspect = oxdimen = oviewingrange = -1;

    videoSetViewableArea(0L,0L,xdim-1,ydim-1);
    videoClearScreen(0L);

    if (searchx < 0) { searchx = halfxdimen; searchy = (ydimen>>1); }

    return 0;
}

//
// qloadkvx
//



int32_t qloadkvx(int32_t voxindex, const char *filename)
{
    if ((unsigned)voxindex >= MAXVOXELS)
        return -1;

    auto fil = fileSystem.OpenFileReader(filename);
    if (!fil.isOpen())
        return -1;

    int32_t lengcnt = 0;
    const int32_t lengtot = fil.GetLength();

    for (bssize_t i=0; i<MAXVOXMIPS; i++)
    {
		int32_t dasiz = fil.ReadInt32();

		voxelmemory.Reserve(1);
		voxelmemory.Last() = fil.Read(dasiz);

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

int32_t getangle(int32_t xvect, int32_t yvect)
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

fixed_t gethiq16angle(int32_t xvect, int32_t yvect)
{
    fixed_t rv;

    if ((xvect | yvect) == 0)
        rv = 0;
    else if (xvect == 0)
        rv = IntToFixed(512 + ((yvect < 0) << 10));
    else if (yvect == 0)
        rv = IntToFixed(((xvect < 0) << 10));
    else if (xvect == yvect)
        rv = IntToFixed(256 + ((xvect < 0) << 10));
    else if (xvect == -yvect)
        rv = IntToFixed(768 + ((xvect > 0) << 10));
    else if (klabs(xvect) > klabs(yvect))
        rv = ((qradarang[5120 + scale(1280, yvect, xvect)] >> 6) + IntToFixed(((xvect < 0) << 10))) & 0x7FFFFFF;
    else rv = ((qradarang[5120 - scale(1280, xvect, yvect)] >> 6) + IntToFixed(512 + ((yvect < 0) << 10))) & 0x7FFFFFF;

    return rv;
}

//
// ksqrt
//
int32_t ksqrt(uint32_t num)
{
    if (enginecompatibility_mode == ENGINECOMPATIBILITY_19950829)
        return ksqrtasm_old(num);
    return nsqrtasm(num);
}

// Gets the BUILD unit height and z offset of a sprite.
// Returns the z offset, 'height' may be NULL.
int32_t spriteheightofsptr(uspriteptr_t spr, int32_t *height, int32_t alsotileyofs)
{
    int32_t hei, zofs=0;
    const int32_t picnum=spr->picnum, yrepeat=spr->yrepeat;

    hei = (tileHeight(picnum)*yrepeat)<<2;
    if (height != NULL)
        *height = hei;

    if (spr->cstat&128)
        zofs = hei>>1;

    // NOTE: a positive per-tile yoffset translates the sprite into the
    // negative world z direction (i.e. upward).
    if (alsotileyofs)
        zofs -= tileTopOffset(picnum) *yrepeat<<2;

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
    memset(sectbitmap, 0, sizeof(sectbitmap));
    if (x1 == x2 && y1 == y2)
        return (sect1 == sect2);

    sectbitmap[sect1>>3] |= pow2char[sect1&7];
    clipsectorlist[0] = sect1; danum = 1;

    for (dacnt=0; dacnt<danum; dacnt++)
    {
        const int32_t dasectnum = clipsectorlist[dacnt];
        auto const sec = (usectorptr_t)&sector[dasectnum];
        uwallptr_t wal;
        bssize_t cnt;
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
                continue;
            }

            nexts = wal->nextsector;

                if (nexts < 0 || wal->cstat&32)
                    return 0;

            t = divscale24(t,bot);
            x = x1 + mulscale24(x21,t);
            y = y1 + mulscale24(y21,t);
            z = z1 + mulscale24(z21,t);

            getzsofslope(dasectnum, x,y, &cfz[0],&cfz[1]);

            if (z <= cfz[0] || z >= cfz[1])
            {
                return 0;
            }

            getzsofslope(nexts, x,y, &cfz[0],&cfz[1]);
            if (z <= cfz[0] || z >= cfz[1])
                return 0;

            if (!(sectbitmap[nexts>>3] & pow2char[nexts&7]))
            {
                sectbitmap[nexts>>3] |= pow2char[nexts&7];
                clipsectorlist[danum++] = nexts;
            }
        }

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
        memset(walbitmap, 0, (numwalls+7)>>3);
    yaxwalls[numyaxwalls++] = pointhighlight;

    for (i=0; i<numyaxwalls; i++)
    {
        int32_t clockwise = 0;
        int32_t w = yaxwalls[i];
        const int32_t tmpstartwall = w;

        bssize_t cnt = MAXWALLS;

        while (1)
        {
            wall[w].x = dax;
            wall[w].y = day;
            walbitmap[w>>3] |= pow2char[w&7];

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
                Printf("dragpoint %d: infloop!\n", pointhighlight);
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

static inline int inside_exclude_p(int32_t const x, int32_t const y, int const sectnum, const uint8_t *excludesectbitmap)
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
        std::swap(sect1, sect2);

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
#if 0
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
#endif
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

// NOTE: This comes from Duke, therefore it's GPL!
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

void videoSetCorrectedAspect()
{
        // In DOS the game world is displayed with an aspect of 1.28 instead 1.333,
        // meaning we have to stretch it by a factor of 1.25 instead of 1.2
        // to get perfect squares
        int32_t yx = (65536 * 5) / 4;
        int32_t vr, y, x;

        x = xdim;
        y = ydim;

        vr = divscale16(x*3, y*4);

        renderSetAspect(vr, yx);
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
// clearview
//
void videoClearViewableArea(int32_t dacol)
{
    GLInterface.ClearScreen(dacol, false);
}


//
// clearallviews
//
void videoClearScreen(int32_t dacol)
{
    GLInterface.ClearScreen(dacol | PalEntry(255,0,0,0));
}


//MUST USE RESTOREFORDRAWROOMS AFTER DRAWING

static int32_t setviewcnt = 0; // interface layers use this now
static int32_t bakxsiz, bakysiz;
static vec2_t bakwindowxy1, bakwindowxy2;

//
// setviewtotile
//
FCanvasTexture* renderSetTarget(int16_t tilenume)
{
    auto tex = tileGetTexture(tilenume);
    if (!tex || !tex->isHardwareCanvas()) return nullptr;
    auto canvas = static_cast<FCanvasTexture*>(tex->GetTexture());
    if (!canvas) return nullptr;
    int xsiz = tex->GetTexelWidth(), ysiz = tex->GetTexelHeight();
    if (setviewcnt > 0 || xsiz <= 0 || ysiz <= 0)
        return nullptr;

    //DRAWROOMS TO TILE BACKUP&SET CODE
    bakxsiz = xdim; bakysiz = ydim;
    bakwindowxy1 = windowxy1;
    bakwindowxy2 = windowxy2;

    setviewcnt++;

    xdim = ysiz;
    ydim = xsiz;
    videoSetViewableArea(0,0,ysiz-1,xsiz-1);
    renderSetAspect(65536,65536);
    return canvas;
}


//
// setviewback
//
void renderRestoreTarget()
{
    if (setviewcnt <= 0) return;
    setviewcnt--;

    xdim = bakxsiz;
    ydim = bakysiz;
    videoSetViewableArea(bakwindowxy1.x,bakwindowxy1.y,
            bakwindowxy2.x,bakwindowxy2.y);

}


//
// preparemirror
//
void renderPrepareMirror(int32_t dax, int32_t day, int32_t daz, fixed_t daang, fixed_t dahoriz, int16_t dawall,
                         int32_t *tposx, int32_t *tposy, fixed_t *tang)
{
    const int32_t x = wall[dawall].x, dx = wall[wall[dawall].point2].x-x;
    const int32_t y = wall[dawall].y, dy = wall[wall[dawall].point2].y-y;

    const int32_t j = dx*dx + dy*dy;
    if (j == 0)
        return;

    int i = ((dax-x)*dx + (day-y)*dy)<<1;

    *tposx = (x<<1) + scale(dx,i,j) - dax;
    *tposy = (y<<1) + scale(dy,i,j) - day;
    *tang  = ((gethiq16angle(dx, dy) << 1) - daang) & 0x7FFFFFF;

    inpreparemirror = 1;

    polymost_prepareMirror(dax, day, daz, daang, dahoriz, dawall);
}


//
// completemirror
//
void renderCompleteMirror(void)
{
    polymost_completeMirror();
    inpreparemirror = 0;
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
	if ((unsigned)wallNum < (unsigned)numwalls)
	{
		native_t const w = wall[wallNum].nextwall;
		return ((unsigned)w < MAXWALLS) ? wall[w].nextsector : sectorofwall_internal(wallNum);
	}
	return -1;
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
// setrollangle
//
#ifdef USE_OPENGL
void renderSetRollAngle(float rolla)
{
    gtang = rolla * (fPI * (1.f/1024.f));
}
#endif

