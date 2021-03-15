// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#pragma once

#include "cmdlib.h"

#ifndef ENGINE_PRIV_H
#define ENGINE_PRIV_H

#define MAXARTFILES_BASE 200
#define MAXARTFILES_TOTAL 220
#define MAXCLIPDIST 1024

// Uncomment to clear the screen before each top-level draw (classic only).
// FIXME: doesn't work with mirrors.
//#define ENGINE_CLEAR_SCREEN

    extern intptr_t asm1, asm2;
    extern int32_t globalx1, globaly2;



static inline int32_t getclipmask(int32_t a, int32_t b, int32_t c, int32_t d)
{
    // Ken did this
    d = ((a<0)<<3) + ((b<0)<<2) + ((c<0)<<1) + (d<0);
    return (((d<<4)^0xf0)|d);
}



extern int16_t thesector[MAXWALLSB], thewall[MAXWALLSB];
extern int16_t bunchfirst[MAXWALLSB], bunchlast[MAXWALLSB];
extern int16_t maskwall[MAXWALLSB], maskwallcnt;
extern tspriteptr_t tspriteptr[MAXSPRITESONSCREEN + 1];
extern int32_t xdimen, xdimenrecip, halfxdimen, xdimenscale, xdimscale, ydimen;
extern float fxdimen;
extern int32_t globalposx, globalposy, globalposz;
extern fixed_t qglobalhoriz, qglobalang;
extern float fglobalposx, fglobalposy, fglobalposz;
extern int16_t globalang, globalcursectnum;
extern int32_t globalpal, cosglobalang, singlobalang;
extern int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;
extern int32_t xyaspect;
extern int32_t globalshade;
extern int16_t globalpicnum;

extern int32_t globalorientation;

extern int16_t editstatus;

extern int16_t searchit;
extern int16_t searchsector, searchwall, searchstat;
extern int16_t searchbottomwall, searchisbottom;

extern char inpreparemirror;

extern int16_t sectorborder[256];
extern int32_t hitallsprites;

extern int32_t xb1[MAXWALLSB];
extern int32_t rx1[MAXWALLSB], ry1[MAXWALLSB];
extern int16_t bunchp2[MAXWALLSB];
extern int16_t numscans, numbunches;
extern int32_t rxi[8], ryi[8];
extern int32_t reciptable[2048];


// int32_t wallmost(int16_t *mostbuf, int32_t w, int32_t sectnum, char dastat);
int32_t wallfront(int32_t l1, int32_t l2);

void set_globalang(fixed_t const ang);

int32_t animateoffs(int tilenum, int fakevar);

static FORCE_INLINE int32_t bad_tspr(tspriteptr_t tspr)
{
    // NOTE: tspr->owner >= MAXSPRITES (could be model) has to be handled by
    // caller.
    return (tspr->owner < 0 || (unsigned)tspr->picnum >= MAXTILES);
}

//
// getpalookup (internal)
//
static FORCE_INLINE int32_t getpalookup(int32_t davis, int32_t dashade)
{
    if (getpalookup_replace)
        return getpalookup_replace(davis, dashade);
    return min(max(dashade + (davis >> 8), 0), numshades - 1);
}

static FORCE_INLINE int32_t getpalookupsh(int32_t davis) { return getpalookup(davis, globalshade) << 8; }

static FORCE_INLINE void setgotpic(int32_t tilenume)
{
    gotpic[tilenume>>3] |= pow2char[tilenume&7];
}


// Get properties of parallaxed sky to draw.
// Returns: pointer to tile offset array. Sets-by-pointer the other three.
const int16_t* getpsky(int32_t picnum, int32_t* dapyscale, int32_t* dapskybits, int32_t* dapyoffs, int32_t* daptileyscale);

static FORCE_INLINE void set_globalpos(int32_t const x, int32_t const y, int32_t const z)
{
    globalposx = x, fglobalposx = (float)x;
    globalposy = y, fglobalposy = (float)y;
    globalposz = z, fglobalposz = (float)z;
}


// x1, y1: in/out
// rest x/y: out
template <typename T>
static inline void get_wallspr_points(T const * const spr, int32_t *x1, int32_t *x2,
                                      int32_t *y1, int32_t *y2)
{
    //These lines get the 2 points of the rotated sprite
    //Given: (x1, y1) starts out as the center point

    const int32_t tilenum=spr->picnum, ang=spr->ang;
    const int32_t xrepeat = spr->xrepeat;
    int32_t xoff = tileLeftOffset(tilenum) + spr->xoffset;
    int32_t k, l, dax, day;

    if (spr->cstat&4)
        xoff = -xoff;

    dax = bsin(ang) * xrepeat;
    day = -bcos(ang) * xrepeat;

    l = tileWidth(tilenum);
    k = (l>>1)+xoff;

    *x1 -= MulScale(dax,k, 16);
    *x2 = *x1 + MulScale(dax,l, 16);

    *y1 -= MulScale(day,k, 16);
    *y2 = *y1 + MulScale(day,l, 16);
}

// x1, y1: in/out
// rest x/y: out
template <typename T>
static inline void get_floorspr_points(T const * const spr, int32_t px, int32_t py,
                                       int32_t *x1, int32_t *x2, int32_t *x3, int32_t *x4,
                                       int32_t *y1, int32_t *y2, int32_t *y3, int32_t *y4)
{
    const int32_t tilenum = spr->picnum;
    const int32_t cosang = bcos(spr->ang);
    const int32_t sinang = bsin(spr->ang);

    vec2_t const span = { tileWidth(tilenum), tileHeight(tilenum)};
    vec2_t const repeat = { spr->xrepeat, spr->yrepeat };

    vec2_t adjofs = { tileLeftOffset(tilenum) + spr->xoffset, tileTopOffset(tilenum) + spr->yoffset };

    if (spr->cstat & 4)
        adjofs.x = -adjofs.x;

    if (spr->cstat & 8)
        adjofs.y = -adjofs.y;

    vec2_t const center = { ((span.x >> 1) + adjofs.x) * repeat.x, ((span.y >> 1) + adjofs.y) * repeat.y };
    vec2_t const rspan  = { span.x * repeat.x, span.y * repeat.y };
    vec2_t const ofs    = { -MulScale(cosang, rspan.y, 16), -MulScale(sinang, rspan.y, 16) };

    *x1 += DMulScale(sinang, center.x, cosang, center.y, 16) - px;
    *y1 += DMulScale(sinang, center.y, -cosang, center.x, 16) - py;

    *x2 = *x1 - MulScale(sinang, rspan.x, 16);
    *y2 = *y1 + MulScale(cosang, rspan.x, 16);

    *x3 = *x2 + ofs.x, *x4 = *x1 + ofs.x;
    *y3 = *y2 + ofs.y, *y4 = *y1 + ofs.y;
}

inline int widthBits(int num)
{
    return sizeToBits(tileWidth(num));
}

inline int heightBits(int num)
{
    return sizeToBits(tileHeight(num));
}


#endif	/* ENGINE_PRIV_H */
