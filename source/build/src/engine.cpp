// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
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
#include "gamecvars.h"
#include "c_console.h"
#include "v_2ddrawer.h"
#include "v_draw.h"
#include "stats.h"
#include "razemenu.h"
#include "version.h"
#include "earcut.hpp"
#include "gamestate.h"
#include "inputstate.h"
#include "printf.h"
#include "gamecontrol.h"
#include "render.h"
#include "gamefuncs.h"
#include "hw_voxels.h"

#ifdef USE_OPENGL
# include "mdsprite.h"
# include "polymost.h"
#include "v_video.h"
#include "../../glbackend/glbackend.h"
#include "gl_renderer.h"
#endif

spriteext_t spriteext[MAXSPRITES];
spritesmooth_t spritesmooth[MAXSPRITES + MAXUNIQHUDID];

sectortype sector[MAXSECTORS];
walltype wall[MAXWALLS];
spritetype sprite[MAXSPRITES];

int32_t r_rortexture = 0;
int32_t r_rortexturerange = 0;
int32_t r_rorphase = 0;
int32_t mdtims, omdtims;

float fcosglobalang, fsinglobalang;
float fydimen, fviewingrange;

uint8_t globalr = 255, globalg = 255, globalb = 255;

int16_t pskybits_override = -1;

static int32_t beforedrawrooms = 1;

static int8_t tempbuf[MAXWALLS];

static int32_t no_radarang2 = 0;
static int16_t radarang[1280];

const char *engineerrstr = "No error";

int32_t showfirstwall=0;
int32_t showheightindicators=1;
int32_t circlewall=-1;

fixed_t global100horiz;  // (-100..300)-scale horiz (the one passed to drawrooms)

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

int32_t xdimen = -1, xdimenscale, xdimscale;
float fxdimen = -1.f;
int32_t ydimen;

int32_t globalposx, globalposy, globalposz;
fixed_t qglobalhoriz;
float fglobalposx, fglobalposy, fglobalposz;
int16_t globalang, globalcursectnum;
fixed_t qglobalang;
int32_t globalpal, globalfloorpal, cosglobalang, singlobalang;
int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;

int32_t viewingrangerecip;

static int32_t globalxpanning, globalypanning;
int32_t globalshade, globalorientation;
int16_t globalpicnum;


static int32_t globaly1, globalx2;

int16_t pointhighlight=-1, linehighlight=-1, highlightcnt=0;

static int16_t numhits;

char inpreparemirror = 0;


//
// Internal Engine Functions
//


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

static int32_t engineLoadTables(void)
{
    static char tablesloaded = 0;

    if (tablesloaded == 0)
    {
        int32_t i;

        for (i=0; i<=512; i++)
            sintable[i] = int(sin(i * BAngRadian) * +SINTABLEUNIT);
        for (i=513; i<1024; i++)
            sintable[i] = sintable[1024-i];
        for (i=1024; i<2048; i++)
            sintable[i] = -sintable[i-1024];

        for (i=0; i<640; i++)
            radarang[i] = atan((639.5 - i) / 160.) * (-64. / BAngRadian);
        for (i=0; i<640; i++)
            radarang[1279-i] = -radarang[i];

        tablesloaded = 1;
    }

    return 0;
}


////////// SPRITE LIST MANIPULATION FUNCTIONS //////////

///// sector lists of sprites /////

// insert sprite at the head of sector list, change .sectnum
static void do_insertsprite_at_headofsect(int16_t spritenum, int16_t sectnum)
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
static void do_deletespritesect(int16_t deleteme)
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
static void do_insertsprite_at_headofstat(int16_t spritenum, int16_t statnum)
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
static int32_t insertspritestat(int16_t statnum)
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

    sprite[newspritenum].time = leveltimer++;
    return newspritenum;

}

//
// deletesprite
//
int32_t (*deletesprite_replace)(int16_t spritenum) = NULL;
int32_t deletesprite(int16_t spritenum)
{
    Polymost::polymost_deletesprite(spritenum);
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

        *intersectionX = originX + MulScale(ray.x, t, 24);
        *intersectionY = originY + MulScale(ray.y, t, 24);
        *intersectionZ = originZ + MulScale(destZ-originZ, t, 24);

        return 1;
    }

    const int32_t originDiffCrossLineVec = originDiff.x*lineVec.y - originDiff.y*lineVec.x;
    static const int32_t signBit = 1u<<31u;
    // Any point on either line can be expressed as p+t*r and q+u*s
    // The two line segments intersect when we can find a t & u such that p+t*r = q+u*s
    // If the point is outside of the bounds of the line segment, we know we don't have an intersection.
    // t is < 0 if (originDiffCrossLineVec^rayCrossLineVec) & signBit)
    // u is < 0 if (originDiffCrossRay^rayCrossLineVec) & signBit
    // t is > 1 if abs(originDiffCrossLineVec) > abs(rayCrossLineVec)
    // u is > 1 if abs(originDiffCrossRay) > abs(rayCrossLineVec)
    // where int32_t u = tabledivide64(((int64_t) originDiffCrossRay) << 24L, rayCrossLineVec);
    if (((originDiffCrossLineVec^rayCrossLineVec) & signBit) ||
        ((originDiffCrossRay^rayCrossLineVec) & signBit) ||
        abs(originDiffCrossLineVec) > abs(rayCrossLineVec) ||
        abs(originDiffCrossRay) > abs(rayCrossLineVec))
    {
        // line segments do not overlap
        return 0;
    }

    int64_t t = (int64_t(originDiffCrossLineVec) << 24) / rayCrossLineVec;
    // For sake of completeness/readability, alternative to the above approach for an early out & avoidance of an extra division:

    *intersectionX = originX + MulScale(ray.x, t, 24);
    *intersectionY = originY + MulScale(ray.y, t, 24);
    *intersectionZ = originZ + MulScale(destZ-originZ, t, 24);

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

    int32_t t = DivScale(topt, bot, 16);
    *intx = x1 + MulScale(vx, t, 16);
    *inty = y1 + MulScale(vy, t, 16);
    *intz = z1 + MulScale(vz, t, 16);

    t = DivScale(topu, bot, 16);

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
            return &sky;
        }

    multipskies.Reserve(1);
    multipskies.Last() = {};
    multipskies.Last().tilenum = tilenum;
    multipskies.Last().yscale = 65536;
    return &multipskies.Last();
}

psky_t * defineSky(int32_t const tilenum, int horiz, int lognumtiles, const uint16_t *tileofs, int yoff, int yoff2)
{
    auto sky = tileSetupSky(tilenum);
    sky->horizfrac = horiz;
    sky->lognumtiles = lognumtiles;
    sky->yoffs = yoff;
    sky->yoffs2 = yoff2 == 0x7fffffff ? yoff : yoff2;
    memcpy(sky->tileofs, tileofs, 2 << lognumtiles);
    return sky;
}

// Get properties of parallaxed sky to draw.
// Returns: pointer to tile offset array. Sets-by-pointer the other three.
const int16_t* getpsky(int32_t picnum, int32_t* dapyscale, int32_t* dapskybits, int32_t* dapyoffs, int32_t* daptileyscale, bool alt)
{
    psky_t const* const psky = getpskyidx(picnum);

    if (dapskybits)
        *dapskybits = (pskybits_override == -1 ? psky->lognumtiles : pskybits_override);
    if (dapyscale)
        *dapyscale = (parallaxyscale_override == 0 ? psky->horizfrac : parallaxyscale_override);
    if (dapyoffs)
        *dapyoffs = (alt? psky->yoffs2 : psky->yoffs) + parallaxyoffs_override;
    if (daptileyscale)
        *daptileyscale = psky->yscale;

    return psky->tileofs;
}


//
// initengine
//
int32_t engineInit(void)
{
    engineLoadTables();
    g_visibility = 512;
    return 0;
}

//
// initspritelists
//
void (*initspritelists_replace)(void) = NULL;
void initspritelists(void)
{
    leveltimer = 0;
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

int32_t inside(int32_t x, int32_t y, int sectnum)
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
    else if (abs(xvect) > abs(yvect))
        rv = ((radarang[640 + Scale(160, yvect, xvect)] >> 6) + ((xvect < 0) << 10)) & 2047;
    else rv = ((radarang[640 - Scale(160, xvect, yvect)] >> 6) + 512 + ((yvect < 0) << 10)) & 2047;

    return rv;
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

    if (newpos != &sprite[spritenum].pos)
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

    sectbitmap[sect1>>3] |= (1 << (sect1&7));
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

            t = DivScale(t,bot, 24);
            x = x1 + MulScale(x21,t, 24);
            y = y1 + MulScale(y21,t, 24);
            z = z1 + MulScale(z21,t, 24);

            getzsofslope(dasectnum, x,y, &cfz[0],&cfz[1]);

            if (z <= cfz[0] || z >= cfz[1])
            {
                return 0;
            }

            getzsofslope(nexts, x,y, &cfz[0],&cfz[1]);
            if (z <= cfz[0] || z >= cfz[1])
                return 0;

            if (!(sectbitmap[nexts>>3] & (1 << (nexts&7))))
            {
                sectbitmap[nexts>>3] |= (1 << (nexts&7));
                clipsectorlist[danum++] = nexts;
            }
        }

    }

    if (sectbitmap[sect2>>3] & (1<<(sect2&7)))
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

    const int32_t vx = MulScale(bcos(ange), neartagrange, 14);
    const int32_t vy = MulScale(bsin(ange), neartagrange, 14);
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
                    *neartaghitdist = DMulScale(intx-xs, bcos(ange), inty-ys, bsin(ange), 14);
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

        SectIterator it(dasector);
        while ((z = it.NextIndex()) >= 0)
        {
            auto const spr = (uspriteptr_t)&sprite[z];

            if (spr->cstat2 & CSTAT2_SPRITE_NOFIND)
                continue;
            if (blacklist_sprite_func && blacklist_sprite_func(z))
                continue;

            if (((tagsearch&1) && spr->lotag) || ((tagsearch&2) && spr->hitag))
            {
                if (try_facespr_intersect(spr, sv, vx, vy, 0, &hitv, 1))
                {
                    *neartagsprite = z;
                    *neartaghitdist = DMulScale(hitv.x-xs, bcos(ange), hitv.y-ys, bsin(ange), 14);
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
            sector[wall[w].sector].dirty = 255;
            wall[w].x = dax;
            wall[w].y = day;
            walbitmap[w>>3] |= (1<<(w&7));

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
    return abs(closest.x - in.x) + abs(closest.y - in.y);
}

int32_t getwalldist(vec2_t const in, int const wallnum, vec2_t * const out)
{
    getclosestpointonwall_internal(in, wallnum, out);
    return abs(out->x - in.x) + abs(out->y - in.y);
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
    int16_t sect = *sectnum;
    updatesectorneighbor(x, y, &sect, INITIALUPDATESECTORDIST, MAXUPDATESECTORDIST);
    if (sect != -1)
        SET_AND_RETURN(*sectnum, sect);

    // we need to support passing in a sectnum of -1, unfortunately

    for (int i = numsectors - 1; i >= 0; --i)
        if (inside_p(x, y, i))
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

            if (nofirstzcheck || (z >= cz && z <= fz))
                if (inside_p(x, y, *sectnum))
                    return;

            walltype const * wal = &wall[sector[*sectnum].wallptr];
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
    int const dacos = bcos(daang);
    int const dasin = bsin(daang);
    p.x -= pivot.x;
    p.y -= pivot.y;
    p2->x = DMulScale(p.x, dacos, -p.y, dasin, 14) + pivot.x;
    p2->y = DMulScale(p.y, dacos, p.x, dasin, 14) + pivot.y;
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

    xdimen = (x2-x1)+1;
    ydimen = (y2-y1)+1;

    fxdimen = (float) xdimen;
    fydimen = (float) ydimen;
    videoSetCorrectedAspect();
}



#include "v_2ddrawer.h"



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


int32_t getceilzofslopeptr(usectorptr_t sec, int32_t dax, int32_t day)
{
    if (!(sec->ceilingstat&2))
        return sec->ceilingz;

    auto const wal  = (uwallptr_t)&wall[sec->wallptr];
    auto const wal2 = (uwallptr_t)&wall[wal->point2];

    vec2_t const w = *(vec2_t const *)wal;
    vec2_t const d = { wal2->x - w.x, wal2->y - w.y };

    int const i = ksqrt(uhypsq(d.x,d.y))<<5;
    if (i == 0) return sec->ceilingz;

    int const j = DMulScale(d.x, day-w.y, -d.y, dax-w.x, 3);
    int const shift = enginecompatibility_mode != ENGINECOMPATIBILITY_NONE ? 0 : 1;
    return sec->ceilingz + (Scale(sec->ceilingheinum,j>>shift,i)<<shift);
}

int32_t getflorzofslopeptr(usectorptr_t sec, int32_t dax, int32_t day)
{
    if (!(sec->floorstat&2))
        return sec->floorz;

    auto const wal  = (uwallptr_t)&wall[sec->wallptr];
    auto const wal2 = (uwallptr_t)&wall[wal->point2];

    vec2_t const w = *(vec2_t const *)wal;
    vec2_t const d = { wal2->x - w.x, wal2->y - w.y };

    int const i = ksqrt(uhypsq(d.x,d.y))<<5;
    if (i == 0) return sec->floorz;

    int const j = DMulScale(d.x, day-w.y, -d.y, dax-w.x, 3);
    int const shift = enginecompatibility_mode != ENGINECOMPATIBILITY_NONE ? 0 : 1;
    return sec->floorz + (Scale(sec->floorheinum,j>>shift,i)<<shift);
}

void getzsofslopeptr(usectorptr_t sec, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    *ceilz = sec->ceilingz; *florz = sec->floorz;

    if (((sec->ceilingstat|sec->floorstat)&2) != 2)
        return;

    auto const wal  = (uwallptr_t)&wall[sec->wallptr];
    auto const wal2 = (uwallptr_t)&wall[wal->point2];

    vec2_t const d = { wal2->x - wal->x, wal2->y - wal->y };

    int const i = ksqrt(uhypsq(d.x,d.y))<<5;
    if (i == 0) return;

    int const j = DMulScale(d.x,day-wal->y, -d.y,dax-wal->x, 3);
    int const shift = enginecompatibility_mode != ENGINECOMPATIBILITY_NONE ? 0 : 1;
    if (sec->ceilingstat&2)
        *ceilz += Scale(sec->ceilingheinum,j>>shift,i)<<shift;
    if (sec->floorstat&2)
        *florz += Scale(sec->floorheinum,j>>shift,i)<<shift;
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

    sector[dasect].ceilingheinum = Scale((z-sector[dasect].ceilingz)<<8,
                                         ksqrt(uhypsq(dax,day)), i);
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

    sector[dasect].floorheinum = Scale((z-sector[dasect].floorz)<<8,
                                       ksqrt(uhypsq(dax,day)), i);
    if (sector[dasect].floorheinum == 0)
        sector[dasect].floorstat &= ~2;
    else sector[dasect].floorstat |= 2;
}


int tilehasmodelorvoxel(int const tilenume, int pal)
{
    return
        (mdinited && hw_models && tile2model[Ptile2tile(tilenume, pal)].modelid != -1) ||
        (r_voxels && tiletovox[tilenume] != -1);
}
