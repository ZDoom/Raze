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


#include "basics.h"

////////// Bitfield manipulation //////////

#endif // compat_h_
