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

// XXX: Only for testing on x86. Don't use from outside; it doesn't account for
// whether we're compiling for e.g. x86_64 which will never use asm anyway.
//#define USE_ASM_DIVSCALE

#if !defined USE_ASM_DIVSCALE
#ifdef GEKKO
#include <math.h>
static inline int32_t divscale(int32_t eax, int32_t ebx, int32_t ecx)
{
    return ldexp(eax, ecx) / ebx;
}

# define _scaler(a) \
    static inline int32_t divscale##a(int32_t eax, int32_t ebx) \
{ \
    return divscale(eax, ebx, a); \
} \

#else
static inline int32_t divscale(int32_t eax, int32_t ebx, int32_t ecx) { return dw((qw(eax) << by(ecx)) / ebx); }

# define _scaler(a) \
    static inline int32_t divscale##a(int32_t eax, int32_t ebx) \
{ \
    return dw((qw(eax) << by(a)) / ebx); \
} \

#endif

PRAGMA_FUNCS _scaler(32)

#undef _scaler
#endif  // !defined USE_ASM_DIVSCALE

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
static inline void drawpixels(void* s, int16_t a)  { *((int16_t*)(s)) = a; }
static inline void drawpixelses(void* s, int32_t a) { *((int32_t*)(s)) = a; }

static inline int32_t divmod(int32_t a, int32_t b) { uint32_t _a=(uint32_t)a, _b=(uint32_t)b; dmval = _a%_b; return _a/_b; }
static inline int32_t moddiv(int32_t a, int32_t b) { uint32_t _a=(uint32_t)a, _b=(uint32_t)b; dmval = _a/_b; return _a%_b; }

static inline int32_t klabs(int32_t a) { if (a < 0) return -a; return a; }
static inline int32_t ksgn(int32_t a)  { if (a > 0) return 1; if (a < 0) return -1; return 0; }

static inline int32_t umin(int32_t a, int32_t b) { if ((uint32_t)a < (uint32_t)b) return a; return b; }
static inline int32_t umax(int32_t a, int32_t b) { if ((uint32_t)a < (uint32_t)b) return b; return a; }
static inline int32_t kmin(int32_t a, int32_t b) { if ((int32_t)a < (int32_t)b) return a; return b; }
static inline int32_t kmax(int32_t a, int32_t b) { if ((int32_t)a < (int32_t)b) return b; return a; }

static inline int32_t sqr(int32_t eax) { return (eax) * (eax); }
static inline int32_t scale(int32_t eax, int32_t edx, int32_t ecx) { return dw((qw(eax) * edx) / ecx); }
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
    while (c--) {
        swapchar(dst++, src--);
    }
}

#ifdef EXTERNC
}
#endif

#endif // __pragmas_h__

