// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#ifndef pragmas_h_
#define pragmas_h_

#ifdef __cplusplus
extern "C" {
#endif

#define EDUKE32_GENERATE_PRAGMAS                                                                            \
    EDUKE32_SCALER_PRAGMA(1)  EDUKE32_SCALER_PRAGMA(2)  EDUKE32_SCALER_PRAGMA(3)  EDUKE32_SCALER_PRAGMA(4)  \
    EDUKE32_SCALER_PRAGMA(5)  EDUKE32_SCALER_PRAGMA(6)  EDUKE32_SCALER_PRAGMA(7)  EDUKE32_SCALER_PRAGMA(8)  \
    EDUKE32_SCALER_PRAGMA(9)  EDUKE32_SCALER_PRAGMA(10) EDUKE32_SCALER_PRAGMA(11) EDUKE32_SCALER_PRAGMA(12) \
    EDUKE32_SCALER_PRAGMA(13) EDUKE32_SCALER_PRAGMA(14) EDUKE32_SCALER_PRAGMA(15) EDUKE32_SCALER_PRAGMA(16) \
    EDUKE32_SCALER_PRAGMA(17) EDUKE32_SCALER_PRAGMA(18) EDUKE32_SCALER_PRAGMA(19) EDUKE32_SCALER_PRAGMA(20) \
    EDUKE32_SCALER_PRAGMA(21) EDUKE32_SCALER_PRAGMA(22) EDUKE32_SCALER_PRAGMA(23) EDUKE32_SCALER_PRAGMA(24) \
    EDUKE32_SCALER_PRAGMA(25) EDUKE32_SCALER_PRAGMA(26) EDUKE32_SCALER_PRAGMA(27) EDUKE32_SCALER_PRAGMA(28) \
    EDUKE32_SCALER_PRAGMA(29) EDUKE32_SCALER_PRAGMA(30) EDUKE32_SCALER_PRAGMA(31)

#if !defined(NOASM) && defined __cplusplus
extern "C" {
#endif
extern int32_t reciptable[2048], fpuasm;
#if !defined(NOASM) && defined __cplusplus
}
#endif

// break the C version of divscale out from the others
// because asm version overflows in drawmapview()

#define qw(x) ((int64_t)(x))  // quadword cast
#define dw(x) ((int32_t)(x))  // doubleword cast
#define wo(x) ((int16_t)(x))  // word cast
#define by(x) ((uint8_t)(x))  // byte cast

#define DIVTABLESIZE 16384

extern libdivide::libdivide_s64_t divtable64[DIVTABLESIZE];
extern libdivide::libdivide_s32_t divtable32[DIVTABLESIZE];
extern void initdivtables(void);

static inline uint32_t divideu32(uint32_t const n, uint32_t const d)
{
    static libdivide::libdivide_u32_t udiv;
    static uint32_t lastd;

    if (d == lastd)
        goto skip;

    udiv = libdivide::libdivide_u32_gen((lastd = d));
skip:
    return libdivide::libdivide_u32_do(n, &udiv);
}

static inline int64_t tabledivide64(int64_t const n, int64_t const d)
{
    static libdivide::libdivide_s64_t sdiv;
    static int32_t lastd;
    auto const dptr = ((unsigned)d < DIVTABLESIZE) ? &divtable64[d] : &sdiv;

    if (d == lastd || dptr != &sdiv)
        goto skip;

    sdiv = libdivide::libdivide_s64_gen((lastd = d));
skip:
    return libdivide::libdivide_s64_do(n, dptr);
}

static inline int32_t tabledivide32(int32_t const n, int32_t const d)
{
    static libdivide::libdivide_s32_t sdiv;
    static int32_t lastd;
    auto const dptr = ((unsigned)d < DIVTABLESIZE) ? &divtable32[d] : &sdiv;

    if (d == lastd || dptr != &sdiv)
        goto skip;

    sdiv = libdivide::libdivide_s32_gen((lastd = d));
skip:
    return libdivide::libdivide_s32_do(n, dptr);
}

extern uint32_t divideu32_noinline(uint32_t n, uint32_t d);
extern int32_t tabledivide32_noinline(int32_t n, int32_t d);
extern int64_t tabledivide64_noinline(int64_t n, int64_t d);

#ifdef GEKKO
static inline int32_t divscale(int32_t eax, int32_t ebx, int32_t ecx) { return dw(tabledivide64(ldexp(eax, ecx), ebx)); }
#else
static inline int32_t divscale(int32_t eax, int32_t ebx, int32_t ecx) { return dw(tabledivide64(qw(eax) << by(ecx), ebx)); }
#endif

static inline int64_t divscale64(int64_t eax, int64_t ebx, int64_t ecx) { return tabledivide64(eax << ecx, ebx); }

#define EDUKE32_SCALER_PRAGMA(a) \
    static FORCE_INLINE int32_t divscale##a(int32_t eax, int32_t ebx) { return divscale(eax, ebx, a); }
EDUKE32_GENERATE_PRAGMAS EDUKE32_SCALER_PRAGMA(32)
#undef EDUKE32_SCALER_PRAGMA

static inline int32_t scale(int32_t eax, int32_t edx, int32_t ecx)
{
    return dw(tabledivide64(qw(eax) * edx, ecx));
}

static FORCE_INLINE int32_t scaleadd(int32_t eax, int32_t edx, int32_t addend, int32_t ecx)
{
    return dw(tabledivide64(qw(eax) * edx + addend, ecx));
}

static inline int32_t roundscale(int32_t eax, int32_t edx, int32_t ecx)
{
    return scaleadd(eax, edx, ecx / 2, ecx);
}

#if defined(__GNUC__) && defined(GEKKO)

// GCC Inline Assembler version (PowerPC)
#include "pragmas_ppc.h"

#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)

// GCC Inline Assembler version (x86)
#include "pragmas_x86_gcc.h"

#elif defined(_MSC_VER) && !defined(NOASM)  // __GNUC__

// Microsoft C inline assembler
#include "pragmas_x86_msvc.h"

#elif defined(__arm__)  // _MSC_VER

// GCC Inline Assembler version (ARM)
#include "pragmas_arm.h"

#endif

//
// Generic C
//

#ifndef pragmas_have_mulscale

#define EDUKE32_SCALER_PRAGMA(a)                                                                                                     \
    static FORCE_INLINE CONSTEXPR int32_t mulscale##a(int32_t eax, int32_t edx) { return dw((qw(eax) * edx) >> by(a)); }             \
    static FORCE_INLINE CONSTEXPR int32_t dmulscale##a(int32_t eax, int32_t edx, int32_t esi, int32_t edi)                           \
    {                                                                                                                                \
        return dw(((qw(eax) * edx) + (qw(esi) * edi)) >> by(a));                                                                     \
    }                                                                                                                                \
    static FORCE_INLINE CONSTEXPR int32_t tmulscale##a(int32_t eax, int32_t edx, int32_t ebx, int32_t ecx, int32_t esi, int32_t edi) \
    {                                                                                                                                \
        return dw(((qw(eax) * edx) + (qw(ebx) * ecx) + (qw(esi) * edi)) >> by(a));                                                   \
    }

EDUKE32_GENERATE_PRAGMAS EDUKE32_SCALER_PRAGMA(32)

#undef EDUKE32_SCALER_PRAGMA

#endif


#ifdef __cplusplus
}
template <typename T>
static FORCE_INLINE void swap(T * const a, T * const b)
{
    T const t = *a;
    *a = *b;
    *b = t;
}
#define swapptr swap
extern "C" {
#else
static FORCE_INLINE void swapptr(void *a, void *b)
{
    intptr_t const t = *(intptr_t*) a;
    *(intptr_t*) a = *(intptr_t*) b;
    *(intptr_t*) b = t;
}
#endif

#ifndef pragmas_have_swaps
#ifdef __cplusplus
#define swapchar swap
#define swapshort swap
#define swaplong swap
#define swapfloat swap
#define swapdouble swap
#define swap64bit swap
#else
static FORCE_INLINE void swapchar(void *a, void *b)
{
    char const t = *(char *)b;
    *(char *)b = *(char *)a;
    *(char *)a = t;
}
static FORCE_INLINE void swapshort(void *a, void *b)
{
    int16_t const t = *(int16_t *)b;
    *(int16_t *)b = *(int16_t *)a;
    *(int16_t *)a = t;
}
static FORCE_INLINE void swaplong(void *a, void *b)
{
    int32_t const t = *(int32_t *)b;
    *(int32_t *)b = *(int32_t *)a;
    *(int32_t *)a = t;
}
static FORCE_INLINE void swapfloat(void *a, void *b)
{
    float const t = *(float *)b;
    *(float *)b = *(float *)a;
    *(float *)a = t;
}
static FORCE_INLINE void swapdouble(void *a, void *b)
{
    double const t = *(double *)b;
    *(double *)b = *(double *)a;
    *(double *)a = t;
}
static FORCE_INLINE void swap64bit(void *a, void *b)
{
    uint64_t const t = *(uint64_t *)b;
    *(uint64_t *)b = *(uint64_t *)a;
    *(uint64_t *)a = t;
}
#endif
static FORCE_INLINE void swapchar2(void *a, void *b, int32_t s)
{
    swapchar((char *)a, (char *)b);
    swapchar((char *)a + 1, (char *)b + s);
}
#endif

static FORCE_INLINE CONSTEXPR char readpixel(void *s) { return *(char *)s; }
static FORCE_INLINE void drawpixel(void *s, char a) { *(char *)s = a; }

#ifndef pragmas_have_klabs
#if 0
static FORCE_INLINE int32_t klabs(int32_t const a)
{
    uint32_t const m = a >> (sizeof(uint32_t) * CHAR_BIT - 1);
    return (a ^ m) - m;
}
#else
#define klabs(x) abs(x)
#endif
#endif
#ifndef pragmas_have_ksgn
static FORCE_INLINE CONSTEXPR int ksgn(int32_t a) { return (a > 0) - (a < 0); }
#endif

#ifndef pragmas_have_mulscale
static FORCE_INLINE CONSTEXPR int32_t mulscale(int32_t eax, int32_t edx, int32_t ecx) { return dw((qw(eax) * edx) >> by(ecx)); }
static FORCE_INLINE CONSTEXPR int32_t dmulscale(int32_t eax, int32_t edx, int32_t esi, int32_t edi, int32_t ecx)
{
    return dw(((qw(eax) * edx) + (qw(esi) * edi)) >> by(ecx));
}
#endif

#ifndef pragmas_have_qinterpolatedown16
void qinterpolatedown16(intptr_t bufptr, int32_t num, int32_t val, int32_t add);
void qinterpolatedown16short(intptr_t bufptr, int32_t num, int32_t val, int32_t add);
#endif

#ifndef pragmas_have_clearbuf
void clearbuf(void *d, int32_t c, int32_t a);
#endif
#ifndef pragmas_have_copybuf
void copybuf(const void *s, void *d, int32_t c);
#endif
#ifndef pragmas_have_swaps
void swapbuf4(void *a, void *b, int32_t c);
#endif

#ifndef pragmas_have_clearbufbyte
void clearbufbyte(void *D, int32_t c, int32_t a);
#endif
#ifndef pragmas_have_copybufbyte
void copybufbyte(const void *S, void *D, int32_t c);
#endif
#ifndef pragmas_have_copybufreverse
void copybufreverse(const void *S, void *D, int32_t c);
#endif

#ifndef pragmas_have_krecipasm
static inline int32_t krecipasm(int32_t i)
{
    // Ken did this
    union { int32_t i; float f; } x;
    x.f = (float)i;
    i = x.i;
    return ((reciptable[(i >> 12) & 2047] >> (((i - 0x3f800000) >> 23) & 31)) ^ (i >> 31));
}
#endif

#undef qw
#undef dw
#undef wo
#undef by

static inline void swapbufreverse(void *s, void *d, int32_t c)
{
    uint8_t *src = (uint8_t *)s, *dst = (uint8_t *)d;
    Bassert(c >= 4);

    do
    {
        swapchar(dst, src);
        swapchar(dst + 1, src - 1);
        swapchar(dst + 2, src - 2);
        swapchar(dst + 3, src - 3);
        dst += 4, src -= 4;
    } while ((c -= 4) > 4);

    while (c--)
        swapchar(dst++, src--);
}

#ifdef __cplusplus
}
#endif

#endif  // pragmas_h_
