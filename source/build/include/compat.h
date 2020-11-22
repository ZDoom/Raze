// Compatibility declarations for things which might not be present in
// certain build environments. It also levels the playing field caused
// by different platforms.

#ifndef compat_h_
#define compat_h_

#pragma once

#include "xs_Float.h"
#include "m_alloc.h"
#include "intvec.h"
#include "m_swap.h"
#include "serializer.h"

////////// Compiler detection //////////

#ifdef __GNUC__
# define EDUKE32_GCC_PREREQ(major, minor) (major < __GNUC__ || (major == __GNUC__ && minor <= __GNUC_MINOR__))
#else
# define EDUKE32_GCC_PREREQ(major, minor) 0
#endif


////////// Language detection //////////

////////// Language and compiler feature polyfills //////////

# define EXTERNC

#ifndef UNREFERENCED_PARAMETER
# define UNREFERENCED_PARAMETER(x) (x) = (x)
#endif

#if defined __GNUC__ || defined __clang__
# define ATTRIBUTE(attrlist) __attribute__(attrlist)
#else
# define ATTRIBUTE(attrlist)
#endif


#ifndef MAY_ALIAS
# ifdef _MSC_VER
#  define MAY_ALIAS
# else
#  define MAY_ALIAS __attribute__((may_alias))
# endif
#endif

#ifndef FORCE_INLINE
# ifdef _MSC_VER
#  define FORCE_INLINE __forceinline
# else
#  ifdef __GNUC__
#    define FORCE_INLINE inline __attribute__((always_inline))
#  else
#    define FORCE_INLINE inline
#  endif
# endif
#endif

#  define fallthrough__ [[fallthrough]]

////////// Architecture detection //////////

#ifdef WORDS_BIGENDIAN
#  define B_BIG_ENDIAN    1
#else
#  define B_BIG_ENDIAN    0
#endif

#include <inttypes.h>
#include <stdint.h>

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !(defined _WIN32 && defined __clang__)
#include <float.h>
#endif
#include <math.h>

#include <ctype.h>
#include <errno.h>
#include <time.h>

#include <assert.h>

# include <limits>
#  include <algorithm>
#  include <functional>
#  include <type_traits>

////////// Platform headers //////////

#if !defined __APPLE__ && (!defined EDUKE32_BSD || !__STDC__)
# include <malloc.h>
#endif

#include "engineerrors.h"

////////// DEPRECATED: Standard library prefixing //////////

typedef intptr_t ssize_t;

typedef ssize_t bssize_t;

#define BMAX_PATH 256

////////// Metaprogramming structs //////////

using std::enable_if_t;
using  native_t = intptr_t;

typedef struct MAY_ALIAS {
    int32_t x, y;
} vec2_t;

typedef struct {
    float x, y;
} vec2f_t;

typedef struct {
    double x, y;
} vec2d_t;

typedef struct MAY_ALIAS {
    union {
        struct { int32_t x, y, z; };
        vec2_t  vec2;
    };
} vec3_t;

typedef struct MAY_ALIAS {
    union {
        struct { int16_t x, y, z; };
        vec2_16_t vec2;
    };
} vec3_16_t;

typedef struct {
    union {
        struct {
            union { float x, d; };
            union { float y, u; };
            union { float z, v; };
        };
        vec2f_t vec2;
    };
} vec3f_t;

static_assert(sizeof(vec3f_t) == sizeof(float) * 3);

typedef struct {
    union { double x; double d; };
    union { double y; double u; };
    union { double z; double v; };
} vec3d_t;

static_assert(sizeof(vec3d_t) == sizeof(double) * 3);


////////// Language tricks that depend on size_t //////////

#include "basics.h"

////////// Pointer management //////////

#define DO_FREE_AND_NULL(var) do { \
    Xfree(var); (var) = NULL; \
} while (0)


////////// Data serialization //////////

inline int32_t B_LITTLE32(int32_t val) { return LittleLong(val); }
inline uint32_t B_LITTLE32(uint32_t val) { return LittleLong(val); }
inline int32_t B_LITTLE16(int16_t val) { return LittleShort(val); }
inline uint32_t B_LITTLE16(uint16_t val) { return LittleShort(val); }

static FORCE_INLINE void B_BUF32(void * const buf, uint32_t const x) { *(uint32_t *) buf = x; }
static FORCE_INLINE uint32_t B_UNBUF32(void const * const buf) { return *(uint32_t const *) buf; }
static FORCE_INLINE uint16_t B_UNBUF16(void const * const buf) { return *(uint16_t const *) buf; }



////////// Abstract data operations //////////

using std::min;
using std::max;

////////// Bitfield manipulation //////////

// This once was a static array, requiring a memory acces where a shift would suffice.
// Revert the above to a real bit shift through some C++ operator magic. That saves me from reverting all the code that uses this construct.
// Note: Only occurs 25 times in the code, should be removed for good.
static struct 
{
	constexpr uint8_t operator[](int index) const { return 1 << index; };
} pow2char;


static FORCE_INLINE void bitmap_set(uint8_t *const ptr, int const n) { ptr[n>>3] |= 1 << (n&7); }
static FORCE_INLINE char bitmap_test(uint8_t const *const ptr, int const n) { return ptr[n>>3] & (1 << (n&7)); }

////////// Utility functions //////////

// breadth-first search helpers
template <typename T>
void bfirst_search_init(T *const list, uint8_t *const bitmap, T *const eltnumptr, int const maxelts, int const firstelt)
{
    memset(bitmap, 0, (maxelts+7)>>3);

    list[0] = firstelt;
    bitmap_set(bitmap, firstelt);
    *eltnumptr = 1;
}

template <typename T>
void bfirst_search_try(T *const list, uint8_t *const bitmap, T *const eltnumptr, int const elt)
{
    if (!bitmap_test(bitmap, elt))
    {
        bitmap_set(bitmap, elt);
        list[(*eltnumptr)++] = elt;
    }
}

////////// PANICKING ALLOCATION WRAPPERS //////////


#define Xstrdup(s)    (strdup(s))
#define Xmalloc(size) (M_Malloc(size))
#define Xcalloc(nmemb, size) (M_Calloc(nmemb, size))
#define Xrealloc(ptr, size)  (M_Realloc(ptr, size))
#define Xfree(ptr) (M_Free(ptr))

////////// Inlined external libraries //////////

/* End dependence on compat.o object. */

inline FSerializer& Serialize(FSerializer& arc, const char* key, vec2_t& c, vec2_t* def)
{
    if (def && !memcmp(&c, def, sizeof(c))) return arc;
    if (arc.BeginObject(key))
    {
        arc("x", c.x, def? &def->x : nullptr)
            ("y", c.y, def ? &def->y : nullptr)
            .EndObject();
    }
    return arc;
}

inline FSerializer& Serialize(FSerializer& arc, const char* key, vec3_t& c, vec3_t* def)
{
    if (def && !memcmp(&c, def, sizeof(c))) return arc;
    if (arc.BeginObject(key))
    {
        arc("x", c.x, def ? &def->x : nullptr)
            ("y", c.y, def ? &def->y : nullptr)
            ("z", c.z, def ? &def->z : nullptr)
            .EndObject();
    }
    return arc;
}

#endif // compat_h_
