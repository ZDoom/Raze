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

#define MAXPERMS 512
#define MAXARTFILES_BASE 200
#define MAXARTFILES_TOTAL 220
#define MAXCLIPDIST 1024

// Uncomment to clear the screen before each top-level draw (classic only).
// FIXME: doesn't work with mirrors.
//#define ENGINE_CLEAR_SCREEN

#ifdef YAX_ENABLE
# define YAX_MAXDRAWS 8
#endif

    extern intptr_t asm1, asm2, asm3, asm4;
    extern int32_t globalx1, globaly2;


extern uint16_t ATTRIBUTE((used)) sqrtable[4096], ATTRIBUTE((used)) shlookup[4096+256], ATTRIBUTE((used)) sqrtable_old[2048];


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

    inline int32_t getkensmessagecrc(int32_t b)
    {
        UNREFERENCED_PARAMETER(b);
        return 0x56c764d4l;
    }


inline int32_t ksqrtasm_old(int32_t n)
{
    n = klabs(n);
    int shift;
    for (shift = 0; n >= 2048; n >>=2, shift++)
    {
    }
    return (sqrtable_old[n]<<shift)>>10;
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
extern uint8_t* mirrorBuffer;
extern int32_t xdimen, xdimenrecip, halfxdimen, xdimenscale, xdimscale, ydimen;
extern float fxdimen;
extern intptr_t frameoffset;
extern int32_t globalposx, globalposy, globalposz, globalhoriz;
extern fix16_t qglobalhoriz, qglobalang;
extern float fglobalposx, fglobalposy, fglobalposz;
extern int16_t globalang, globalcursectnum;
extern int32_t globalpal, cosglobalang, singlobalang;
extern int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;
extern int32_t globalhisibility, globalpisibility, globalcisibility;
#ifdef USE_OPENGL
extern int32_t globvis2, globalvisibility2, globalhisibility2, globalpisibility2, globalcisibility2;
#endif
extern int32_t globvis, globalvisibility;
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

extern uint8_t picsiz[MAXTILES];
extern int16_t sectorborder[256];
extern int32_t qsetmode;
extern int32_t hitallsprites;

extern int32_t xb1[MAXWALLSB];
extern int32_t rx1[MAXWALLSB], ry1[MAXWALLSB];
extern int16_t bunchp2[MAXWALLSB];
extern int16_t numscans, numbunches;
extern int32_t rxi[8], ryi[8];

#ifdef USE_OPENGL

// For GL_EXP2 fog:
#define FOGSCALE 0.0000768f

void calc_and_apply_fog(int32_t shade, int32_t vis, int32_t pal);
void calc_and_apply_fog_factor(int32_t shade, int32_t vis, int32_t pal, float factor);
#endif

extern void get_wallspr_points(uspriteptr_t spr, int32_t *x1, int32_t *x2,
    int32_t *y1, int32_t *y2);
extern void get_floorspr_points(uspriteptr_t spr, int32_t px, int32_t py,
    int32_t *x1, int32_t *x2, int32_t *x3, int32_t *x4,
    int32_t *y1, int32_t *y2, int32_t *y3, int32_t *y4);


// int32_t wallmost(int16_t *mostbuf, int32_t w, int32_t sectnum, char dastat);
int32_t wallfront(int32_t l1, int32_t l2);

void set_globalang(fix16_t const ang);

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

void dorotspr_handle_bit2(int32_t *sx, int32_t *sy, int32_t *z, int32_t dastat,
                          int32_t cx1_plus_cx2, int32_t cy1_plus_cy2,
                          int32_t *ret_yxaspect, int32_t *ret_xyaspect);

////// yax'y stuff //////
#ifdef USE_OPENGL
extern void polymost_scansector(int32_t sectnum);
#endif
int32_t renderAddTsprite(int16_t z, int16_t sectnum);
#ifdef YAX_ENABLE
extern int32_t g_nodraw, scansector_retfast, scansector_collectsprites;
extern int32_t yax_globallev, yax_globalbunch;
extern int32_t yax_globalcf, yax_nomaskpass, yax_nomaskdidit;
extern uint8_t haveymost[(YAX_MAXBUNCHES+7)>>3];
extern uint8_t yax_gotsector[(MAXSECTORS+7)>>3];
extern int32_t yax_polymostclearzbuffer;

static FORCE_INLINE int32_t yax_isislandwall(int32_t line, int32_t cf) { return (yax_vnextsec(line, cf) >= 0); }
#endif

#ifdef YAX_DEBUG
extern char m32_debugstr[64][128];
extern int32_t m32_numdebuglines;
# define yaxdebug(fmt, ...)  do { if (m32_numdebuglines<64) Bsnprintf(m32_debugstr[m32_numdebuglines++], 128, fmt, ##__VA_ARGS__); } while (0)
# define yaxprintf(fmt, ...) do { initprintf(fmt, ##__VA_ARGS__); } while (0)
#else
# define yaxdebug(fmt, ...)
# define yaxprintf(fmt, ...)
#endif



#if defined(_MSC_VER) && !defined(NOASM)

static inline void setgotpic(int32_t a)
{
    _asm {
        push ebx
        mov eax, a
        mov ebx, eax
        cmp byte ptr walock[eax], 200
        jae skipit
        mov byte ptr walock[eax], 199
skipit:
        shr eax, 3
        and ebx, 7
        mov dl, byte ptr gotpic[eax]
        mov bl, byte ptr pow2char[ebx]
        or dl, bl
        mov byte ptr gotpic[eax], dl
        pop ebx
    }
}

#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)	// _MSC_VER

#define setgotpic(a) \
({ int32_t __a=(a); \
    __asm__ __volatile__ ( \
                   "movl %%eax, %%ebx\n\t" \
                   "cmpb $200, " ASMSYM("walock") "(%%eax)\n\t" \
                   "jae 0f\n\t" \
                   "movb $199, " ASMSYM("walock") "(%%eax)\n\t" \
                   "0:\n\t" \
                   "shrl $3, %%eax\n\t" \
                   "andl $7, %%ebx\n\t" \
                   "movb " ASMSYM("gotpic") "(%%eax), %%dl\n\t" \
                   "movb " ASMSYM("pow2char_") "(%%ebx), %%bl\n\t" \
                   "orb %%bl, %%dl\n\t" \
                   "movb %%dl, " ASMSYM("gotpic") "(%%eax)" \
                   : "=a" (__a) : "a" (__a) \
                   : "ebx", "edx", "memory", "cc"); \
                       __a; })

#else	// __GNUC__ && __i386__

static FORCE_INLINE void setgotpic(int32_t tilenume)
{
    if (walock[tilenume] < 200) walock[tilenume] = 199;
    gotpic[tilenume>>3] |= pow2char[tilenume&7];
}

#endif

// Get properties of parallaxed sky to draw.
// Returns: pointer to tile offset array. Sets-by-pointer the other three.
static FORCE_INLINE const int8_t *getpsky(int32_t picnum, int32_t *dapyscale, int32_t *dapskybits, int32_t *dapyoffs, int32_t *daptileyscale)
{
    psky_t const * const psky = &multipsky[getpskyidx(picnum)];

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

    if (((obj & 16384) == 16384) && (globalorientation & CSTAT_WALL_ROTATE_90) && rottile[tile].newtile != -1)
        tile = rottile[tile].newtile;
}

#endif	/* ENGINE_PRIV_H */
