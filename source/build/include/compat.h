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

////////// Language and compiler feature polyfills //////////

# define EXTERNC

#if defined __GNUC__ || defined __clang__
# define ATTRIBUTE(attrlist) __attribute__(attrlist)
#else
# define ATTRIBUTE(attrlist)
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

typedef intptr_t bssize_t;

#define BMAX_PATH 256

////////// Metaprogramming structs //////////

using  native_t = intptr_t;

typedef struct {
    float x, y;
} vec2f_t;

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

////////// Language tricks that depend on size_t //////////

#include "basics.h"

////////// Bitfield manipulation //////////

inline void bitmap_set(uint8_t *const ptr, int const n) { ptr[n>>3] |= 1 << (n&7); }
inline char bitmap_test(uint8_t const *const ptr, int const n) { return ptr[n>>3] & (1 << (n&7)); }

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

#endif // compat_h_
