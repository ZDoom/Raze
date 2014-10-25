// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)


#ifndef __pragmas_h__
#define __pragmas_h__

#ifdef EXTERNC
extern "C" {
#endif

#include <limits.h>

#define PRAGMA_FUNCS _scaler(1) _scaler(2)	_scaler(3)	_scaler(4)\
_scaler(5)	_scaler(6)	_scaler(7)	_scaler(8)\
_scaler(9)	_scaler(10)	_scaler(11)	_scaler(12)\
_scaler(13)	_scaler(14)	_scaler(15)	_scaler(16)\
_scaler(17)	_scaler(18)	_scaler(19)	_scaler(20)\
_scaler(21)	_scaler(22)	_scaler(23)	_scaler(24)\
_scaler(25)	_scaler(26)	_scaler(27)	_scaler(28)\
_scaler(29)	_scaler(30)	_scaler(31)

extern int32_t dmval;

// break the C version of divscale out from the others
// because asm version overflows in drawmapview()

#define qw(x)	((int64_t)(x))		// quadword cast
#define dw(x)	((int32_t)(x))		// doubleword cast
#define wo(x)	((int16_t)(x))		// word cast
#define by(x)	((uint8_t)(x))		// byte cast

#define LIBDIVIDE_ALWAYS
#define DIVTABLESIZE 16384

extern libdivide_s64pad_t divtable64[DIVTABLESIZE];
extern libdivide_s32pad_t divtable32[DIVTABLESIZE];

#if defined(__arm__) || defined(LIBDIVIDE_ALWAYS)
static inline uint32_t divideu32(uint32_t n, uint32_t d)
{
    static libdivide_u32_t udiv;
    static uint32_t lastd;

    if (d == lastd)
        goto skip;

    lastd = d;
    udiv = libdivide_u32_gen(d);
skip:
    return libdivide_u32_do(n, &udiv);
}

static inline int32_t tabledivide64(int64_t n, int32_t d)
{
    static libdivide_s64_t sdiv;
    static int32_t lastd;
    libdivide_s64_t *dptr = ((unsigned) d < DIVTABLESIZE) ? (libdivide_s64_t *)&divtable64[d] : &sdiv;

    if (d == lastd || dptr != &sdiv)
        goto skip;

    lastd = d;
    sdiv = libdivide_s64_gen(d);
skip:
    return libdivide_s64_do(n, dptr);
}

static inline int32_t tabledivide32(int32_t n, int32_t d)
{
    static libdivide_s32_t sdiv;
    static int32_t lastd;
    libdivide_s32_t *dptr = ((unsigned) d < DIVTABLESIZE) ? (libdivide_s32_t *)&divtable32[d] : &sdiv;

    if (d == lastd || dptr != &sdiv)
        goto skip;

    lastd = d;
    sdiv = libdivide_s32_gen(d);
skip:
    return libdivide_s32_do(n, dptr);
}
#else
static inline uint32_t divideu32(uint32_t n, uint32_t d) { return n / d; }

static inline int32_t tabledivide64(int64_t n, int32_t d) { return ((unsigned) d < DIVTABLESIZE) ?
    libdivide_s64_do(n, (libdivide_s64_t *) &divtable64[d]) : n / d; }

static inline int32_t tabledivide32(int32_t n, int32_t d) { return ((unsigned) d < DIVTABLESIZE) ?
    libdivide_s32_do(n, (libdivide_s32_t *) &divtable32[d]) : n / d; }
#endif

extern uint32_t divideu32_noinline(uint32_t n, uint32_t d);
extern int32_t tabledivide32_noinline(int32_t n, int32_t d);
extern int32_t tabledivide64_noinline(int64_t n, int32_t d);

#ifdef GEKKO
#include <math.h>
static inline int32_t divscale(int32_t eax, int32_t ebx, int32_t ecx)
{
    return tabledivide64(ldexp(eax, ecx), ebx);
}
#else
static inline int32_t divscale(int32_t eax, int32_t ebx, int32_t ecx)
{
    const int64_t numer = qw(eax) << by(ecx);
    return dw(tabledivide64(numer, ebx));
}
#endif

# define _scaler(a) static inline int32_t divscale##a(int32_t eax, int32_t ebx) { return divscale(eax, ebx, a); }
PRAGMA_FUNCS _scaler(32)
#undef _scaler

static inline int32_t scale(int32_t eax, int32_t edx, int32_t ecx)
{
    const int64_t numer = qw(eax) * edx;
    return dw(tabledivide64(numer, ecx));
}

#if defined(__GNUC__) && defined(GEKKO)

// GCC Inline Assembler version (PowerPC)
#include "pragmas_ppc.h"

#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)

// GCC Inline Assembler version (x86)
#include "pragmas_x86_gcc.h"

#elif defined(_MSC_VER) && !defined(NOASM)	// __GNUC__

// Microsoft C inline assembler
#include "pragmas_x86_msvc.h"

#elif defined(__arm__)				// _MSC_VER

// GCC Inline Assembler version (ARM)
#include "pragmas_arm.h"

#else

//
// Generic C
//

#define _scaler(a) \
static inline int32_t mulscale##a(int32_t eax, int32_t edx) \
{ \
	return dw((qw(eax) * qw(edx)) >> by(a)); \
} \
\
static inline int32_t dmulscale##a(int32_t eax, int32_t edx, int32_t esi, int32_t edi) \
{ \
	return dw(((qw(eax) * qw(edx)) + (qw(esi) * qw(edi))) >> by(a)); \
} \
\

PRAGMA_FUNCS _scaler(32)

#undef _scaler

static inline void swapchar(void* a, void* b)  { char t = *((char*)b); *((char*)b) = *((char*)a); *((char*)a) = t; }
static inline void swapchar2(void* a, void* b, int32_t s) { swapchar(a,b); swapchar((char*)a+1,(char*)b+s); }
static inline void swapshort(void* a, void* b) { int16_t t = *((int16_t*)b); *((int16_t*)b) = *((int16_t*)a); *((int16_t*)a) = t; }
static inline void swaplong(void* a, void* b)  { int32_t t = *((int32_t*)b); *((int32_t*)b) = *((int32_t*)a); *((int32_t*)a) = t; }
static inline void swapfloat(void* a, void* b)  { float t = *((float*)b); *((float*)b) = *((float*)a); *((float*)a) = t; }
static inline void swap64bit(void* a, void* b) { int64_t t = *((int64_t*)b); *((int64_t*)b) = *((int64_t*)a); *((int64_t*)a) = t; }

static inline char readpixel(void* s)    { return (*((char*)(s))); }
static inline void drawpixel(void* s, char a)    { *((char*)(s)) = a; }

static inline int32_t klabs(int32_t a) { const uint32_t m = a >> (sizeof(int) * CHAR_BIT - 1); return (a ^ m) - m; }
static inline int32_t ksgn(int32_t a)  { return (a>0)-(a<0); }

static inline int32_t umin(int32_t a, int32_t b) { if ((uint32_t)a < (uint32_t)b) return a; return b; }
static inline int32_t umax(int32_t a, int32_t b) { if ((uint32_t)a < (uint32_t)b) return b; return a; }
static inline int32_t kmin(int32_t a, int32_t b) { if ((int32_t)a < (int32_t)b) return a; return b; }
static inline int32_t kmax(int32_t a, int32_t b) { if ((int32_t)a < (int32_t)b) return b; return a; }

static inline int32_t sqr(int32_t eax) { return (eax) * (eax); }
static inline int32_t mulscale(int32_t eax, int32_t edx, int32_t ecx) { return dw((qw(eax) * edx) >> by(ecx)); }
static inline int32_t dmulscale(int32_t eax, int32_t edx, int32_t esi, int32_t edi, int32_t ecx) { return dw(((qw(eax) * edx) + (qw(esi) * edi)) >> by(ecx)); }

void qinterpolatedown16 (intptr_t bufptr, int32_t num, int32_t val, int32_t add);
void qinterpolatedown16short (intptr_t bufptr, int32_t num, int32_t val, int32_t add);

void clearbuf(void* d, int32_t c, int32_t a);
void copybuf(const void* s, void* d, int32_t c);
void swapbuf4(void* a, void* b, int32_t c);

void clearbufbyte(void *D, int32_t c, int32_t a);
void copybufbyte(const void *S, void *D, int32_t c);
void copybufreverse(const void *S, void *D, int32_t c);

#endif

#undef qw
#undef dw
#undef wo
#undef by

static inline void swapbufreverse(void *s, void *d, int32_t c)
{
    uint8_t *src = (uint8_t*)s, *dst = (uint8_t*)d;
    do
    {
        swapchar(dst, src);
        swapchar(dst+1, src-1);
        swapchar(dst+2, src-2);
        swapchar(dst+3, src-3);
        dst += 4, src -= 4;
    } while (--c > 4);
    while (c--) swapchar(dst++, src--);
}

#ifdef EXTERNC
}
#endif

#endif // __pragmas_h__

