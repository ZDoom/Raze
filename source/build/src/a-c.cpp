// A.ASM replacement using C
// Mainly by Ken Silverman, with things melded with my port by
// Jonathon Fowler (jf@jonof.id.au)
//
// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "a.h"
#include "pragmas.h"

#ifdef ENGINE_USING_A_C

#define BITSOFPRECISION 3
#define BITSOFPRECISIONPOW 8

// Compile code to saturate vplc for sprites to prevent stray lines at the
// bottom of non-y-flipped ones?
#define USE_SATURATE_VPLC
// Also for translucent masks?
//#define USE_SATURATE_VPLC_TRANS

extern intptr_t asm1, asm2, asm3, asm4;
extern int32_t globalx3, globaly3;

#ifdef USE_ASM64
# define A64_ASSIGN(var, val) var=val
#else
# define A64_ASSIGN(var, val)
#endif

#ifdef USE_ASM64
// variables for a64.yasm
int32_t a64_bpl, a64_transmode, a64_glogy;
intptr_t a64_paloffs;
char *a64_gtrans;
#endif

static int32_t bpl, transmode = 0;
static char *gbuf;
static int32_t glogx, glogy;
int32_t gpinc;
static int32_t gbxinc, gbyinc;
static char *gpal, *ghlinepal, *gtrans;
static char *gpal2;

//Global variable functions
void setvlinebpl(int32_t dabpl) { A64_ASSIGN(a64_bpl, dabpl); bpl = dabpl;}
void fixtransluscence(intptr_t datransoff)
{
    A64_ASSIGN(a64_gtrans, (char *)datransoff);
    gtrans = (char *)datransoff;
}
void settransnormal(void) { A64_ASSIGN(a64_transmode, 0); transmode = 0; }
void settransreverse(void) { A64_ASSIGN(a64_transmode, 1); transmode = 1; }


///// Ceiling/floor horizontal line functions /////

void sethlinesizes(int32_t logx, int32_t logy, intptr_t bufplc)
{ glogx = logx; glogy = logy; gbuf = (char *)bufplc; }
void setpalookupaddress(char *paladdr) { ghlinepal = paladdr; }
void setuphlineasm4(int32_t bxinc, int32_t byinc) { gbxinc = bxinc; gbyinc = byinc; }
void hlineasm4(bssize_t cnt, int32_t skiploadincs, int32_t paloffs, uint32_t by, uint32_t bx, intptr_t p)
{
    Bassert(gbuf);

    if (!skiploadincs) { gbxinc = asm1; gbyinc = asm2; }

    const char *const A_C_RESTRICT palptr = &ghlinepal[paloffs];
    const char *const A_C_RESTRICT buf = gbuf;
    const vec2_t inc = { gbxinc, gbyinc };
    const vec2_t log = { glogx, glogy };
    const vec2_t log32 = { 32-log.x, 32-log.y };
    char *pp = (char *)p;

#ifdef CLASSIC_SLICE_BY_4
    for (; cnt>=4; cnt-=4, pp-=4)
    {
#if 1
        *pp = palptr[buf[((bx>>log32.x)<<log.y)+(by>>log32.y)]];
        *(pp-1) = palptr[buf[(((bx-inc.x)>>log32.x)<<log.y)+((by-inc.y)>>log32.y)]];
        *(pp-2) = palptr[buf[(((bx-(inc.x<<1))>>log32.x)<<log.y)+((by-(inc.y<<1))>>log32.y)]];
        *(pp-3) = palptr[buf[(((bx-(inc.x*3))>>log32.x)<<log.y)+((by-(inc.y*3))>>log32.y)]];
#else
        *(int32_t *)(pp-3) = palptr[buf[(((bx-(inc.x*3))>>log32.x)<<log.y)+((by-(inc.y*3))>>log32.y)]] +
            (palptr[buf[(((bx-(inc.x<<1))>>log32.x)<<log.y)+((by-(inc.y<<1))>>log32.y)]]<<8) +
            (palptr[buf[(((bx-inc.x)>>log32.x)<<log.y)+((by-inc.y)>>log32.y)]]<<16) +
            (palptr[buf[((bx>>log32.x)<<log.y)+(by>>log32.y)]]<<24);
#endif
        bx -= inc.x<<2;
        by -= inc.y<<2;
    }
#endif

    for (; cnt>=0; cnt--, pp--)
    {
        *pp = palptr[buf[((bx>>log32.x)<<log.y)+(by>>log32.y)]];
        bx -= inc.x;
        by -= inc.y;
    }
}


///// Sloped ceiling/floor vertical line functions /////
void slopevlin(intptr_t p, int32_t i, intptr_t slopaloffs, bssize_t cnt, int32_t bx, int32_t by)
{
    intptr_t * A_C_RESTRICT slopalptr;
    int32_t bz, bzinc;
    uint32_t u, v;

    bz = asm3; bzinc = (asm1>>3);
    slopalptr = (intptr_t *)slopaloffs;
    for (; cnt>0; cnt--)
    {
        i = (sloptable[(bz>>6)+HALFSLOPTABLESIZ]); bz += bzinc;
        u = bx+(inthi_t)globalx3*i;
        v = by+(inthi_t)globaly3*i;
        (*(char *)p) = *(char *)(((intptr_t)slopalptr[0])+gbuf[((u>>(32-glogx))<<glogy)+(v>>(32-glogy))]);
        slopalptr--;
        p += gpinc;
    }
}


///// Wall,face sprite/wall sprite vertical line functions /////


extern int32_t globaltilesizy;

static inline uint32_t ourmulscale32(uint32_t a, uint32_t b)
{
    return ((uint64_t)a*b)>>32;
}

static inline int32_t getpix(int32_t logy, const char *buf, uint32_t vplc)
{
    return logy ? buf[vplc>>logy] : buf[ourmulscale32(vplc,globaltilesizy)];
}

void setupvlineasm(int32_t neglogy) { glogy = neglogy; }
// cnt+1 loop iterations!
int32_t vlineasm1(int32_t vinc, intptr_t paloffs, bssize_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
{
    const char *const A_C_RESTRICT buf = (char *)bufplc;
    const char *const A_C_RESTRICT pal = (char *)paloffs;
    const int32_t logy = glogy, ourbpl = bpl;
    char *pp = (char *)p;

    cnt++;

    if (logy)
    {
#ifdef CLASSIC_SLICE_BY_4
        for (; cnt>=4; cnt-=4)
        {
            *pp = pal[buf[vplc>>logy]];
            *(pp+ourbpl) = pal[buf[(vplc+vinc)>>logy]];
            *(pp+(ourbpl<<1)) = pal[buf[(vplc+(vinc<<1))>>logy]];
            *(pp+(ourbpl*3)) = pal[buf[(vplc+(vinc*3))>>logy ]];
            pp += ourbpl<<2;
            vplc += vinc<<2;
        }
#endif
        while (cnt--)
        {
            *pp = pal[buf[vplc>>logy]];
            pp += ourbpl;
            vplc += vinc;
        }
    }
    else
    {
#ifdef CLASSIC_SLICE_BY_4
        for (; cnt>=4; cnt-=4)
        {
            *pp = pal[buf[ourmulscale32(vplc, globaltilesizy)]];
            *(pp+ourbpl) = pal[buf[ourmulscale32((vplc+vinc),globaltilesizy)]];
            *(pp+(ourbpl<<1)) = pal[buf[ourmulscale32((vplc+(vinc<<1)), globaltilesizy)]];
            *(pp+(ourbpl*3)) = pal[buf[ourmulscale32((vplc+(vinc*3)), globaltilesizy)]];
            pp += ourbpl<<2;
            vplc += vinc<<2;
        }
#endif
        while (cnt--)
        {
            *pp = pal[buf[ourmulscale32(vplc,globaltilesizy)]], pp += ourbpl;
            vplc += vinc;
        }
    }
    return vplc;
}


extern intptr_t palookupoffse[4];
extern uint32_t vplce[4];
extern int32_t vince[4];
extern intptr_t bufplce[4];

#if (EDUKE32_GCC_PREREQ(4,7) || __has_extension(attribute_ext_vector_type)) && defined BITNESS64
// XXX: The "Ubuntu clang version 3.5-1ubuntu1 (trunk) (based on LLVM 3.5)"
// does not compile us with USE_VECTOR_EXT. Maybe a newer one does?
# if !defined __clang__
#  define USE_VECTOR_EXT
# endif
#endif

#ifdef USE_VECTOR_EXT
typedef uint32_t uint32_vec4 __attribute__ ((vector_size (16)));
#endif

#ifdef USE_SATURATE_VPLC
# define saturate_vplc(vplc, vinc) vplc |= g_saturate & -(vplc < (uint32_t)vinc)
// NOTE: the vector types yield -1 for logical "true":
# define saturate_vplc_vec(vplc, vinc) vplc |= g_saturate & (vplc < vinc)
# ifdef USE_SATURATE_VPLC_TRANS
#  define saturate_vplc_trans(vplc, vinc) saturate_vplc(vplc, vinc)
# else
#  define saturate_vplc_trans(vplc, vinc)
# endif
#else
# define saturate_vplc(vplc, vinc)
# define saturate_vplc_vec(vplc, vinc)
# define saturate_vplc_trans(vplc, vinc)
#endif

#ifdef CLASSIC_NONPOW2_YSIZE_WALLS
// cnt >= 1
static void vlineasm4nlogy(bssize_t cnt, char *p, char *const A_C_RESTRICT * pal, char *const A_C_RESTRICT * buf,
# ifdef USE_VECTOR_EXT
    uint32_vec4 vplc, const uint32_vec4 vinc)
# else
    uint32_t * vplc, const int32_t *vinc)
# endif
{
    const int32_t ourbpl = bpl;

    do
    {
        p[0] = pal[0][buf[0][ourmulscale32(vplc[0], globaltilesizy)]];
        p[1] = pal[1][buf[1][ourmulscale32(vplc[1], globaltilesizy)]];
        p[2] = pal[2][buf[2][ourmulscale32(vplc[2], globaltilesizy)]];
        p[3] = pal[3][buf[3][ourmulscale32(vplc[3], globaltilesizy)]];

# if defined USE_VECTOR_EXT
        vplc += vinc;
# else
        vplc[0] += vinc[0];
        vplc[1] += vinc[1];
        vplc[2] += vinc[2];
        vplc[3] += vinc[3];
# endif
        p += ourbpl;
    } while (--cnt);

    Bmemcpy(&vplce[0], &vplc[0], sizeof(uint32_t) * 4);
}
#endif

// cnt >= 1
void vlineasm4(bssize_t cnt, char *p)
{
    char * const A_C_RESTRICT pal[4] = {(char *)palookupoffse[0], (char *)palookupoffse[1], (char *)palookupoffse[2], (char *)palookupoffse[3]};
    char * const A_C_RESTRICT buf[4] = {(char *)bufplce[0], (char *)bufplce[1], (char *)bufplce[2], (char *)bufplce[3]};
#ifdef USE_VECTOR_EXT
    uint32_vec4 vinc = {(uint32_t)vince[0], (uint32_t)vince[1], (uint32_t)vince[2], (uint32_t)vince[3]};
    uint32_vec4 vplc = {vplce[0], vplce[1], vplce[2], vplce[3]};
#else
    const int32_t vinc[4] = {vince[0], vince[1], vince[2], vince[3]};
    uint32_t vplc[4] = {vplce[0], vplce[1], vplce[2], vplce[3]};
#endif
    const int32_t logy = glogy, ourbpl = bpl;

#ifdef CLASSIC_NONPOW2_YSIZE_WALLS
    if (EDUKE32_PREDICT_FALSE(!logy))
    {
        // This should only happen when 'globalshiftval = 0' has been set in engine.c.
        vlineasm4nlogy(cnt, p, pal, buf, vplc, vinc);
        return;
    }
#else
    assert(logy);
#endif

    // just fucking shoot me
#ifdef CLASSIC_SLICE_BY_4
    for (; cnt>=4;cnt-=4)
    {
        p[0]                = pal[0][buf[0][ vplc[0]>>logy ]];
        p[1]                = pal[1][buf[1][ vplc[1]>>logy ]];
        p[2]                = pal[2][buf[2][ vplc[2]>>logy ]];
        p[3]                = pal[3][buf[3][ vplc[3]>>logy ]];
        (p+ourbpl)[0]       = pal[0][buf[0][ (vplc[0]+vinc[0])>>logy ]];
        (p+ourbpl)[1]       = pal[1][buf[1][ (vplc[1]+vinc[1])>>logy ]];
        (p+ourbpl)[2]       = pal[2][buf[2][ (vplc[2]+vinc[2])>>logy ]];
        (p+ourbpl)[3]       = pal[3][buf[3][ (vplc[3]+vinc[3])>>logy ]];
        (p+(ourbpl<<1))[0]  = pal[0][buf[0][ (vplc[0]+(vinc[0]<<1))>>logy ]];
        (p+(ourbpl<<1))[1]  = pal[1][buf[1][ (vplc[1]+(vinc[1]<<1))>>logy ]];
        (p+(ourbpl<<1))[2]  = pal[2][buf[2][ (vplc[2]+(vinc[2]<<1))>>logy ]];
        (p+(ourbpl<<1))[3]  = pal[3][buf[3][ (vplc[3]+(vinc[3]<<1))>>logy ]];
        (p+(ourbpl*3))[0]   = pal[0][buf[0][ (vplc[0]+(vinc[0]*3))>>logy ]];
        (p+(ourbpl*3))[1]   = pal[1][buf[1][ (vplc[1]+(vinc[1]*3))>>logy ]];
        (p+(ourbpl*3))[2]   = pal[2][buf[2][ (vplc[2]+(vinc[2]*3))>>logy ]];
        (p+(ourbpl*3))[3]   = pal[3][buf[3][ (vplc[3]+(vinc[3]*3))>>logy ]];

#if defined USE_VECTOR_EXT
        vplc += vinc<<2;
#else
        vplc[0] += vinc[0]<<2;
        vplc[1] += vinc[1]<<2;
        vplc[2] += vinc[2]<<2;
        vplc[3] += vinc[3]<<2;
#endif
        p += ourbpl<<2;
    }
#endif

    while (cnt--)
    {
        p[0] = pal[0][buf[0][vplc[0]>>logy]];
        p[1] = pal[1][buf[1][vplc[1]>>logy]];
        p[2] = pal[2][buf[2][vplc[2]>>logy]];
        p[3] = pal[3][buf[3][vplc[3]>>logy]];

#if defined USE_VECTOR_EXT
        vplc += vinc;
#else
        vplc[0] += vinc[0];
        vplc[1] += vinc[1];
        vplc[2] += vinc[2];
        vplc[3] += vinc[3];
#endif
        p += ourbpl;
    }

    Bmemcpy(&vplce[0], &vplc[0], sizeof(uint32_t) * 4);
}

#ifdef USE_SATURATE_VPLC
static int32_t g_saturate;  // -1 if saturating vplc is requested, 0 else
# define set_saturate(dosaturate) g_saturate = -(int)!!dosaturate
#else
# define set_saturate(dosaturate) UNREFERENCED_PARAMETER(dosaturate)
#endif

void setupmvlineasm(int32_t neglogy, int32_t dosaturate)
{
    glogy = neglogy;
    set_saturate(dosaturate);
}

// cnt+1 loop iterations!
int32_t mvlineasm1(int32_t vinc, intptr_t paloffs, bssize_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
{
    char ch;

    const char *const A_C_RESTRICT buf = (char *)bufplc;
    const char *const A_C_RESTRICT pal = (char *)paloffs;
    const int32_t logy = glogy, ourbpl = bpl;
    char *pp = (char *)p;

    cnt++;

    if (!logy)
    {
        do
        {
            ch = buf[ourmulscale32(vplc,globaltilesizy)];
            if (ch != 255) *pp = pal[ch];
            pp += ourbpl;
            vplc += vinc;
            saturate_vplc(vplc, vinc);
        }
        while (--cnt);

        return vplc;
    }

    do
    {

        if (buf[vplc>>logy] != 255)
            *pp = pal[buf[vplc>>logy]];
        pp += ourbpl;
        vplc += vinc;
        saturate_vplc(vplc, vinc);
    }
    while (--cnt);

    return vplc;
}

// cnt >= 1
void mvlineasm4(bssize_t cnt, char *p)
{
    char *const A_C_RESTRICT pal[4] = {(char *)palookupoffse[0], (char *)palookupoffse[1], (char *)palookupoffse[2], (char *)palookupoffse[3]};
    char *const A_C_RESTRICT buf[4] = {(char *)bufplce[0], (char *)bufplce[1], (char *)bufplce[2], (char *)bufplce[3]};
#ifdef USE_VECTOR_EXT
    uint32_vec4 vinc = {(uint32_t)vince[0], (uint32_t)vince[1], (uint32_t)vince[2], (uint32_t)vince[3]};
    uint32_vec4 vplc = {vplce[0], vplce[1], vplce[2], vplce[3]};
#else
    const int32_t vinc[4] = {vince[0], vince[1], vince[2], vince[3]};
    uint32_t vplc[4] = {vplce[0], vplce[1], vplce[2], vplce[3]};
#endif
    const int32_t logy = glogy, ourbpl = bpl;
    char ch;

    if (logy)
    {
        do
        {
            ch = buf[0][vplc[0]>>logy];
            if (ch != 255) p[0] = pal[0][ch];
            ch = buf[1][vplc[1]>>logy];
            if (ch != 255) p[1] = pal[1][ch];
            ch = buf[2][vplc[2]>>logy];
            if (ch != 255) p[2] = pal[2][ch];
            ch = buf[3][vplc[3]>>logy];
            if (ch != 255) p[3] = pal[3][ch];

#if !defined USE_VECTOR_EXT
            vplc[0] += vinc[0];
            vplc[1] += vinc[1];
            vplc[2] += vinc[2];
            vplc[3] += vinc[3];
            saturate_vplc(vplc[0], vinc[0]);
            saturate_vplc(vplc[1], vinc[1]);
            saturate_vplc(vplc[2], vinc[2]);
            saturate_vplc(vplc[3], vinc[3]);
#else
            vplc += vinc;
            saturate_vplc_vec(vplc, vinc);
#endif
            p += ourbpl;
        }
        while (--cnt);
    }
    else
    {
        do
        {
            ch = buf[0][ourmulscale32(vplc[0],globaltilesizy)];
            if (ch != 255) p[0] = pal[0][ch];
            ch = buf[1][ourmulscale32(vplc[1],globaltilesizy)];
            if (ch != 255) p[1] = pal[1][ch];
            ch = buf[2][ourmulscale32(vplc[2],globaltilesizy)];
            if (ch != 255) p[2] = pal[2][ch];
            ch = buf[3][ourmulscale32(vplc[3],globaltilesizy)];
            if (ch != 255) p[3] = pal[3][ch];

#if !defined USE_VECTOR_EXT
            vplc[0] += vinc[0];
            vplc[1] += vinc[1];
            vplc[2] += vinc[2];
            vplc[3] += vinc[3];
            saturate_vplc(vplc[0], vinc[0]);
            saturate_vplc(vplc[1], vinc[1]);
            saturate_vplc(vplc[2], vinc[2]);
            saturate_vplc(vplc[3], vinc[3]);
#else
            vplc += vinc;
            saturate_vplc_vec(vplc, vinc);
#endif
            p += ourbpl;
        }
        while (--cnt);
    }

    Bmemcpy(&vplce[0], &vplc[0], sizeof(uint32_t) * 4);
}

#ifdef USE_ASM64
# define GLOGY a64_glogy
#else
# define GLOGY glogy
#endif

void setuptvlineasm(int32_t neglogy, int32_t dosaturate)
{
    GLOGY = neglogy;
    set_saturate(dosaturate);
}

#if !defined USE_ASM64
// cnt+1 loop iterations!
int32_t tvlineasm1(int32_t vinc, intptr_t paloffs, bssize_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
{
    char ch;

    const char *const A_C_RESTRICT buf = (char *)bufplc;
    const char *const A_C_RESTRICT pal = (char *)paloffs;
    const char *const A_C_RESTRICT trans = (char *)gtrans;
    const int32_t logy = glogy, ourbpl = bpl, transm = transmode;
    char *pp = (char *)p;

    cnt++;

    uint8_t const shift = transm<<3;

    do
    {
        ch = getpix(logy, buf, vplc);
        if (ch != 255) *pp = trans[((*pp)<<(8-shift))|(pal[ch]<<shift)];
        pp += ourbpl;
        vplc += vinc;
        saturate_vplc_trans(vplc, vinc);
    }
    while (--cnt);

    return vplc;
}
#endif

void setuptvlineasm2(int32_t neglogy, intptr_t paloffs1, intptr_t paloffs2)
{
    GLOGY = neglogy;
    A64_ASSIGN(a64_paloffs, paloffs1);
    gpal = (char *)paloffs1;
    gpal2 = (char *)paloffs2;
}

#if !defined USE_ASM64
// Pass: asm1=vinc2, asm2=pend
// Return: asm1=vplc1, asm2=vplc2
void tvlineasm2(uint32_t vplc2, int32_t vinc1, intptr_t bufplc1, intptr_t bufplc2, uint32_t vplc1, intptr_t p)
{
    char ch;

    bssize_t cnt = tabledivide32(asm2-p-1, bpl);  // >= 1
    const int32_t vinc2 = asm1;

    const char *const A_C_RESTRICT buf1 = (char *)bufplc1;
    const char *const A_C_RESTRICT buf2 = (char *)bufplc2;
    const int32_t logy = glogy, ourbpl = bpl, transm = transmode;

    char *pp = (char *)p;

    cnt++;

    uint8_t const shift = transm<<3;

    do
    {
        ch = getpix(logy, buf1, vplc1);
        if (ch != 255) pp[0] = gtrans[(pp[0]<<(8-shift))|(gpal[ch]<<shift)];
        vplc1 += vinc1;
        saturate_vplc_trans(vplc1, vinc1);

        ch = getpix(logy, buf2, vplc2);
        if (ch != 255) pp[1] = gtrans[(pp[1]<<(8-shift))|(gpal2[ch]<<shift)];
        vplc2 += vinc2;
        saturate_vplc_trans(vplc2, vinc2);

        pp += ourbpl;
    }
    while (--cnt > 0);

    asm1 = vplc1;
    asm2 = vplc2;
}
#endif

//Floor sprite horizontal line functions
void msethlineshift(int32_t logx, int32_t logy) { glogx = logx; glogy = logy; }
// cntup16>>16 + 1 iterations
void mhline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p)
{
    char ch;

    const int32_t xinc = asm1, yinc = asm2;

    UNREFERENCED_PARAMETER(junk);

    gbuf = (char *)bufplc;
    gpal = (char *)asm3;

    cntup16>>=16;
    cntup16++;
    do
    {
        ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
        if (ch != 255) *((char *)p) = gpal[ch];
        bx += xinc;
        by += yinc;
        p++;
    }
    while (--cntup16);
}

void tsethlineshift(int32_t logx, int32_t logy) { glogx = logx; glogy = logy; }
// cntup16>>16 + 1 iterations
void thline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p)
{
    char ch;

    const int32_t xinc = asm1, yinc = asm2;

    UNREFERENCED_PARAMETER(junk);

    gbuf = (char *)bufplc;
    gpal = (char *)asm3;

    cntup16>>=16;
    cntup16++;

    uint8_t const shift = transmode<<3;

    do
    {
        ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
        if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<(8-shift))|(gpal[ch]<<shift)];
        bx += xinc;
        by += yinc;
        p++;
    }
    while (--cntup16);
}


//Rotatesprite vertical line functions
void setupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz)
{
    gpal = (char *)paloffs;
    gbxinc = bxinc;
    gbyinc = byinc;
    glogy = ysiz;
}
void spritevline(int32_t bx, int32_t by, bssize_t cnt, intptr_t bufplc, intptr_t p)
{
    gbuf = (char *)bufplc;
    for (; cnt>1; cnt--)
    {
        (*(char *)p) = gpal[gbuf[(bx>>16)*glogy+(by>>16)]];
        bx += gbxinc;
        by += gbyinc;
        p += bpl;
    }
}

//Rotatesprite vertical line functions
void msetupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz)
{
    gpal = (char *)paloffs;
    gbxinc = bxinc;
    gbyinc = byinc;
    glogy = ysiz;
}
void mspritevline(int32_t bx, int32_t by, bssize_t cnt, intptr_t bufplc, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    for (; cnt>1; cnt--)
    {
        ch = gbuf[(bx>>16)*glogy+(by>>16)];
        if (ch != 255) (*(char *)p) = gpal[ch];
        bx += gbxinc;
        by += gbyinc;
        p += bpl;
    }
}

void tsetupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz)
{
    gpal = (char *)paloffs;
    gbxinc = bxinc;
    gbyinc = byinc;
    glogy = ysiz;
}
void tspritevline(int32_t bx, int32_t by, bssize_t cnt, intptr_t bufplc, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;

    uint8_t const shift = transmode<<3;

    for (; cnt>1; cnt--)
    {
        ch = gbuf[(bx>>16)*glogy+(by>>16)];
        if (ch != 255) *((char *)p) =  gtrans[((*((char *)p))<<(8-shift))+(gpal[ch]<<shift)];
        bx += gbxinc;
        by += gbyinc;
        p += bpl;
    }
}

void setupdrawslab(int32_t dabpl, intptr_t pal)
{
    bpl  = dabpl;
    gpal = (char *)pal;
}

void drawslab(int32_t dx, int32_t v, int32_t dy, int32_t vi, intptr_t vptr, intptr_t p)
{
    do
    {
        char const c = gpal[(int32_t)(*(char *)((v>>16)+vptr))];
        for (int x=0; x < dx; x++)
            ((char*)p)[x] = c;
        p += bpl;
        v += vi;
    }
    while (--dy);
}

#if 0
void stretchhline(intptr_t p0, int32_t u, bssize_t cnt, int32_t uinc, intptr_t rptr, intptr_t p)
{
    p0 = p-(cnt<<2);
    do
    {
        p--;
        *(char *)p = *(char *)((u>>16)+rptr); u -= uinc;
    }
    while (p > p0);
}
#endif

#endif
/*
 * vim:ts=4:
 */

