// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#pragma once

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


extern uint16_t sqrtable[4096], shlookup[4096+256],sqrtable_old[2048];


    static inline int32_t nsqrtasm(uint32_t a)
    {
        // JBF 20030901: This was a damn lot simpler to reverse engineer than
        // msqrtasm was. Really, it was just like simplifying an algebra equation.
        uint16_t c;

        if (a & 0xff000000)  			// test eax, 0xff000000  /  jnz short over24
        {
            c = shlookup[(a >> 24) + 4096];	// mov ebx, eax
                                            // over24: shr ebx, 24
                                            // mov cx, word ptr shlookup[ebx*2+8192]
        }
        else
        {
            c = shlookup[a >> 12];		// mov ebx, eax
                                        // shr ebx, 12
                                        // mov cx, word ptr shlookup[ebx*2]
                                        // jmp short under24
        }
        a >>= c&0xff;				// under24: shr eax, cl
        a = (a&0xffff0000)|(sqrtable[a]);	// mov ax, word ptr sqrtable[eax*2]
        a >>= ((c&0xff00) >> 8);		// mov cl, ch
                                        // shr eax, cl
        return a;
    }

    static inline int32_t getclipmask(int32_t a, int32_t b, int32_t c, int32_t d)
    {
        // Ken did this
        d = ((a<0)<<3) + ((b<0)<<2) + ((c<0)<<1) + (d<0);
        return (((d<<4)^0xf0)|d);
    }



inline int32_t ksqrtasm_old(int32_t n)
{
    uint32_t shift = 0;
    n = klabs((int32_t)n);
    while (n >= 2048)
    {
        n >>= 2;
        ++shift;
    }
    uint32_t const s = sqrtable_old[n];
    return (s << shift) >> 10;
}

inline int32_t clip_nsqrtasm(int32_t n)
{
    if (enginecompatibility_mode == ENGINECOMPATIBILITY_19950829)
        return ksqrtasm_old(n);
    return nsqrtasm(n);
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
extern int32_t searchx, searchy;
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

////// yax'y stuff //////
#ifdef USE_OPENGL
extern void polymost_scansector(int32_t sectnum);
#endif
int32_t renderAddTsprite(int16_t z, int16_t sectnum);


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

template <typename T> static FORCE_INLINE void tileUpdatePicnum(T * const tileptr, int const obj)
{
    auto &tile = *tileptr;

    if (picanm[tile].sf & PICANM_ANIMTYPE_MASK)
        tile += animateoffs(tile, obj);

    if (((obj & 16384) == 16384) && (globalorientation & CSTAT_WALL_ROTATE_90) && RotTile(tile).newtile != -1)
        tile = RotTile(tile).newtile;
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

    dax = sintable[ang&2047]*xrepeat;
    day = sintable[(ang+1536)&2047]*xrepeat;

    l = tileWidth(tilenum);
    k = (l>>1)+xoff;

    *x1 -= mulscale16(dax,k);
    *x2 = *x1 + mulscale16(dax,l);

    *y1 -= mulscale16(day,k);
    *y2 = *y1 + mulscale16(day,l);
}

// x1, y1: in/out
// rest x/y: out
template <typename T>
static inline void get_floorspr_points(T const * const spr, int32_t px, int32_t py,
                                       int32_t *x1, int32_t *x2, int32_t *x3, int32_t *x4,
                                       int32_t *y1, int32_t *y2, int32_t *y3, int32_t *y4)
{
    const int32_t tilenum = spr->picnum;
    const int32_t cosang = sintable[(spr->ang+512)&2047];
    const int32_t sinang = sintable[spr->ang&2047];

    vec2_t const span = { tileWidth(tilenum), tileHeight(tilenum)};
    vec2_t const repeat = { spr->xrepeat, spr->yrepeat };

    vec2_t adjofs = { tileLeftOffset(tilenum) + spr->xoffset, tileTopOffset(tilenum) + spr->yoffset };

    if (spr->cstat & 4)
        adjofs.x = -adjofs.x;

    if (spr->cstat & 8)
        adjofs.y = -adjofs.y;

    vec2_t const center = { ((span.x >> 1) + adjofs.x) * repeat.x, ((span.y >> 1) + adjofs.y) * repeat.y };
    vec2_t const rspan  = { span.x * repeat.x, span.y * repeat.y };
    vec2_t const ofs    = { -mulscale16(cosang, rspan.y), -mulscale16(sinang, rspan.y) };

    *x1 += dmulscale16(sinang, center.x, cosang, center.y) - px;
    *y1 += dmulscale16(sinang, center.y, -cosang, center.x) - py;

    *x2 = *x1 - mulscale16(sinang, rspan.x);
    *y2 = *y1 + mulscale16(cosang, rspan.x);

    *x3 = *x2 + ofs.x, *x4 = *x1 + ofs.x;
    *y3 = *y2 + ofs.y, *y4 = *y1 + ofs.y;
}

inline int widthBits(int num)
{
    int w = tileWidth(num);
    int j = 15;

    while ((j > 1) && ((1 << j) > w))
        j--;
    return j;
}

inline int heightBits(int num)
{
    int w = tileHeight(num);
    int j = 15;

    while ((j > 1) && ((1 << j) > w))
        j--;
    return j;
}


#endif	/* ENGINE_PRIV_H */
