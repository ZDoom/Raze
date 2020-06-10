// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#ifndef pragmas_h_
#define pragmas_h_


#define EDUKE32_GENERATE_PRAGMAS                                                                            \
    EDUKE32_SCALER_PRAGMA(1)  EDUKE32_SCALER_PRAGMA(2)  EDUKE32_SCALER_PRAGMA(3)  EDUKE32_SCALER_PRAGMA(4)  \
    EDUKE32_SCALER_PRAGMA(5)  EDUKE32_SCALER_PRAGMA(6)  EDUKE32_SCALER_PRAGMA(7)  EDUKE32_SCALER_PRAGMA(8)  \
    EDUKE32_SCALER_PRAGMA(9)  EDUKE32_SCALER_PRAGMA(10) EDUKE32_SCALER_PRAGMA(11) EDUKE32_SCALER_PRAGMA(12) \
    EDUKE32_SCALER_PRAGMA(13) EDUKE32_SCALER_PRAGMA(14) EDUKE32_SCALER_PRAGMA(15) EDUKE32_SCALER_PRAGMA(16) \
    EDUKE32_SCALER_PRAGMA(17) EDUKE32_SCALER_PRAGMA(18) EDUKE32_SCALER_PRAGMA(19) EDUKE32_SCALER_PRAGMA(20) \
    EDUKE32_SCALER_PRAGMA(21) EDUKE32_SCALER_PRAGMA(22) EDUKE32_SCALER_PRAGMA(23) EDUKE32_SCALER_PRAGMA(24) \
    EDUKE32_SCALER_PRAGMA(25) EDUKE32_SCALER_PRAGMA(26) EDUKE32_SCALER_PRAGMA(27) EDUKE32_SCALER_PRAGMA(28) \
    EDUKE32_SCALER_PRAGMA(29) EDUKE32_SCALER_PRAGMA(30) EDUKE32_SCALER_PRAGMA(31)

extern int32_t reciptable[2048];

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

static inline uint64_t divideu64(uint64_t const n, uint64_t const d)
{
    static libdivide::libdivide_u64_t udiv;
    static uint64_t lastd;

    if (d == lastd)
        goto skip;

    udiv = libdivide::libdivide_u64_gen((lastd = d));
skip:
    return libdivide::libdivide_u64_do(n, &udiv);
}

static inline int64_t tabledivide64(int64_t const n, int64_t const d)
{
    static libdivide::libdivide_s64_t sdiv;
    static int64_t lastd;
    auto const dptr = ((uint64_t)d < DIVTABLESIZE) ? &divtable64[d] : &sdiv;

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
    auto const dptr = ((uint32_t)d < DIVTABLESIZE) ? &divtable32[d] : &sdiv;

    if (d == lastd || dptr != &sdiv)
        goto skip;

    sdiv = libdivide::libdivide_s32_gen((lastd = d));
skip:
    return libdivide::libdivide_s32_do(n, dptr);
}

extern uint32_t divideu32_noinline(uint32_t n, uint32_t d);
extern uint64_t divideu64_noinline(uint64_t n, uint64_t d);
extern int32_t tabledivide32_noinline(int32_t n, int32_t d);


static inline int32_t divscale(int32_t eax, int32_t ebx, int32_t ecx) { return dw(tabledivide64(qw(eax) << by(ecx), ebx)); }

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


template <typename T>
static FORCE_INLINE void swapptr(T * const a, T * const b)
{
    T const t = *a;
    *a = *b;
    *b = t;
}

#ifndef pragmas_have_swaps
#define swapchar swapptr
#define swapshort swapptr
#define swaplong swapptr
#define swapfloat swapptr
#define swapdouble swapptr
#define swap64bit swapptr

static FORCE_INLINE void swapchar2(void *a, void *b, int32_t s)
{
    swapchar((char *)a, (char *)b);
    swapchar((char *)a + 1, (char *)b + s);
}
#endif

#ifndef pragmas_have_klabs
#define klabs(x) abs(x)
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

#endif  // pragmas_h_
