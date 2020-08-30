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

////////// Compiler detection //////////

#ifdef __GNUC__
# define EDUKE32_GCC_PREREQ(major, minor) (major < __GNUC__ || (major == __GNUC__ && minor <= __GNUC_MINOR__))
#else
# define EDUKE32_GCC_PREREQ(major, minor) 0
#endif

#ifdef __clang__
# define EDUKE32_CLANG_PREREQ(major, minor) (major < __clang_major__ || (major == __clang_major__ && minor <= __clang_minor__))
#else
# define EDUKE32_CLANG_PREREQ(major, minor) 0
#endif
#ifndef __has_builtin
# define __has_builtin(x) 0  // Compatibility with non-clang compilers.
#endif
#ifndef __has_feature
# define __has_feature(x) 0  // Compatibility with non-clang compilers.
#endif
#ifndef __has_extension
# define __has_extension __has_feature // Compatibility with pre-3.0 compilers.
#endif
#ifndef __has_cpp_attribute
# define __has_cpp_attribute(x) 0
#endif

#ifdef _MSC_VER
# define EDUKE32_MSVC_PREREQ(major) ((major) <= (_MSC_VER))
# ifdef __cplusplus
#  define EDUKE32_MSVC_CXX_PREREQ(major) ((major) <= (_MSC_VER))
# else
#  define EDUKE32_MSVC_CXX_PREREQ(major) 0
# endif
#else
# define EDUKE32_MSVC_PREREQ(major) 0
# define EDUKE32_MSVC_CXX_PREREQ(major) 0
#endif


////////// Language detection //////////

#if defined __STDC__
# if defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L
#  define CSTD 2011
# elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#  define CSTD 1999
# elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 199409L
#  define CSTD 1994
# else
#  define CSTD 1989
# endif
#else
# define CSTD 0
#endif

#if defined __cplusplus && __cplusplus >= 201703L
# define CXXSTD 2017
#elif defined __cplusplus && __cplusplus >= 201402L
# define CXXSTD 2014
#elif defined __cplusplus && __cplusplus >= 201103L
# define CXXSTD 2011
#elif defined __cplusplus && __cplusplus >= 199711L
# define CXXSTD 2014    // Thanks, Microsoft... :?
#else
# define CXXSTD 0
#endif


////////// Language and compiler feature polyfills //////////

# define EXTERNC

#ifndef UNREFERENCED_PARAMETER
# define UNREFERENCED_PARAMETER(x) (x) = (x)
#endif

#ifndef UNREFERENCED_CONST_PARAMETER
# ifdef _MSC_VER
#  define UNREFERENCED_CONST_PARAMETER(x) ((void)(x))
# else
#  define UNREFERENCED_CONST_PARAMETER(x)
# endif
#endif

#if defined __GNUC__ || defined __clang__
# define ATTRIBUTE(attrlist) __attribute__(attrlist)
#else
# define ATTRIBUTE(attrlist)
#endif

#if EDUKE32_GCC_PREREQ(4,0)
# define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
# define WARN_UNUSED_RESULT
#endif

#if defined _MSC_VER && _MSC_VER < 1800
# define inline __inline
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


#if 0 && defined(__OPTIMIZE__) && (defined __GNUC__ || __has_builtin(__builtin_expect))
#define EDUKE32_PREDICT_FALSE(x)     __builtin_expect(!!(x),0)
#else
#define EDUKE32_PREDICT_FALSE(x) (x)
#endif


#if EDUKE32_GCC_PREREQ(2,0) || defined _MSC_VER
# define EDUKE32_FUNCTION __FUNCTION__
#elif CSTD >= 1999 || CXXSTD >= 2011
# define EDUKE32_FUNCTION __func__
#else
# define EDUKE32_FUNCTION "???"
#endif

#ifdef _MSC_VER
# define EDUKE32_PRETTY_FUNCTION __FUNCSIG__
#elif EDUKE32_GCC_PREREQ(2,0)
# define EDUKE32_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
# define EDUKE32_PRETTY_FUNCTION EDUKE32_FUNCTION
#endif

#ifdef __COUNTER__
# define EDUKE32_UNIQUE_SRC_ID __COUNTER__
#else
# define EDUKE32_UNIQUE_SRC_ID __LINE__
#endif

#if CXXSTD >= 2017
# define EDUKE32_STATIC_ASSERT(cond) static_assert(cond)
#elif CXXSTD >= 2011 || CSTD >= 2011 || EDUKE32_MSVC_CXX_PREREQ(1600)
# define EDUKE32_STATIC_ASSERT(cond) static_assert(cond, "")
#else
/* C99 / C++03 static assertions based on source found in LuaJIT's src/lj_def.h. */
# define EDUKE32_ASSERT_NAME2(name, line) name ## line
# define EDUKE32_ASSERT_NAME(line) EDUKE32_ASSERT_NAME2(eduke32_assert_, line)
# define EDUKE32_STATIC_ASSERT(cond) \
    extern void EDUKE32_ASSERT_NAME(EDUKE32_UNIQUE_SRC_ID)(int STATIC_ASSERTION_FAILED[(cond)?1:-1])
#endif

#ifndef FP_OFF
# define FP_OFF(__p) ((uintptr_t)(__p))
#endif


#if defined __cplusplus && (__cplusplus >= 201103L || __has_feature(cxx_constexpr) || EDUKE32_MSVC_PREREQ(1900))
# define HAVE_CONSTEXPR
# define CONSTEXPR constexpr
#else
# define CONSTEXPR
#endif

#if CXXSTD >= 2011 || EDUKE32_MSVC_PREREQ(1700)
# define FINAL final
#else
# define FINAL
#endif

#if CXXSTD >= 2014
# define CONSTEXPR_CXX14 CONSTEXPR
#else
# define CONSTEXPR_CXX14
#endif

#if CXXSTD >= 2011
# if __has_cpp_attribute(fallthrough)
#  define fallthrough__ [[fallthrough]]
# elif __has_cpp_attribute(clang::fallthrough)
#  define fallthrough__ [[clang::fallthrough]]
# elif __has_cpp_attribute(gnu::fallthrough)
#  define fallthrough__ [[gnu::fallthrough]]
# endif
#endif
#ifndef fallthrough__
# if !defined __clang__ && EDUKE32_GCC_PREREQ(7,0)
#  define fallthrough__ __attribute__((fallthrough))
# elif defined _MSC_VER
#  define fallthrough__ __fallthrough
# else
#  define fallthrough__
# endif
#endif



////////// Architecture detection //////////

#ifdef WORDS_BIGENDIAN
#  define B_LITTLE_ENDIAN 0
#  define B_BIG_ENDIAN    1
#else
#  define B_LITTLE_ENDIAN 1
#  define B_BIG_ENDIAN    0
#endif


////////// Standard library headers //////////

#undef __USE_MINGW_ANSI_STDIO // Workaround for MinGW-w64.

#ifndef __STDC_FORMAT_MACROS
# define __STDC_FORMAT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
# define __STDC_LIMIT_MACROS
#endif

#ifndef _USE_MATH_DEFINES
# define _USE_MATH_DEFINES
#endif

#include <inttypes.h>
#include <stdint.h>

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#ifndef USE_PHYSFS
#include <stdio.h>
#endif
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

#ifdef __cplusplus
# include <limits>
# if CXXSTD >= 2011 || EDUKE32_MSVC_PREREQ(1800)
#  include <algorithm>
#  include <functional>
#  include <type_traits>
// we need this because MSVC does not properly identify C++11 support
#  define HAVE_CXX11_HEADERS
# endif
#endif

////////// Platform headers //////////

#if !defined __APPLE__ && (!defined EDUKE32_BSD || !__STDC__)
# include <malloc.h>
#endif

#ifndef USE_PHYSFS
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32)
# include <direct.h>
# include <io.h>
#else
# include <unistd.h>
#endif
#endif

#include "engineerrors.h"

////////// DEPRECATED: Standard library prefixing //////////

typedef intptr_t ssize_t;

typedef size_t bsize_t;
typedef ssize_t bssize_t;

#define BMAX_PATH 256

#define Bstrcpy strcpy
#define Bstrncpy strncpy
#define Bstrcmp strcmp
#define Bstrncmp strncmp
#define Bstrcat strcat
#define Bstrncat strncat
#define Bstrlen strlen
#define Bstrchr strchr
#define Bstrrchr strrchr
#define Bstrtol strtol
#define Bstrtoul strtoul
#define Bstrtod strtod
#define Bstrstr strstr
#define Bmemcpy memcpy
#define Bmemset memset


////////// Standard library wrappers //////////

#if defined(_MSC_VER)
# define Bstrcasecmp _stricmp
# define Bstrncasecmp _strnicmp
#elif defined(__QNX__)
# define Bstrcasecmp stricmp
# define Bstrncasecmp strnicmp
#else
# define Bstrcasecmp strcasecmp
# define Bstrncasecmp strncasecmp
#endif

static inline int32_t atoi_safe(const char *str) { return (int32_t)strtoll(str, NULL, 10); }

#define Batoi(x) atoi_safe(x)
#define Batol(str) (strtol(str, NULL, 10))

static inline int Blrintf(const double x)
{
    return xs_CRoundToInt(x);
}

#if defined(__arm__)
# define Bsqrtf __builtin_sqrtf
#else
# define Bsqrtf sqrtf
#endif


////////// Metaprogramming structs //////////

# if CXXSTD >= 2014
using std::enable_if_t;
# elif defined HAVE_CXX11_HEADERS
template <bool B, class T = void>
using enable_if_t = typename std::enable_if<B, T>::type;
# endif


using  native_t = intptr_t;

typedef struct MAY_ALIAS {
    int32_t x, y;
} vec2_t;

typedef struct {
    uint32_t x, y;
} vec2u_t;

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

EDUKE32_STATIC_ASSERT(sizeof(vec3f_t) == sizeof(float) * 3);

typedef struct {
    union { double x; double d; };
    union { double y; double u; };
    union { double z; double v; };
} vec3d_t;

EDUKE32_STATIC_ASSERT(sizeof(vec3d_t) == sizeof(double) * 3);


////////// Language tricks that depend on size_t //////////

#include "basics.h"

# define ARRAY_SIZE(arr) countof(arr)



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
static FORCE_INLINE CONSTEXPR uint32_t B_UNBUF32(void const * const buf) { return *(uint32_t const *) buf; }
static FORCE_INLINE CONSTEXPR uint16_t B_UNBUF16(void const * const buf) { return *(uint16_t const *) buf; }



////////// Abstract data operations //////////

#define ABSTRACT_DECL static FORCE_INLINE WARN_UNUSED_RESULT CONSTEXPR

template <typename T, typename X, typename Y> ABSTRACT_DECL T clamp(T in, X min, Y max) { return in <= (T) min ? (T) min : (in >= (T) max ? (T) max : in); }
template <typename T, typename X, typename Y> ABSTRACT_DECL T clamp2(T in, X min, Y max) { return in >= (T) max ? (T) max : (in <= (T) min ? (T) min : in); }
using std::min;
using std::max;

////////// Bitfield manipulation //////////

#if 0
// Behold the probably most useless (de)optimization I ever discovered. 
// Replacing a simple bit shift with a memory access through a global pointer is surely going to improve matters... >(
// Constexpr means shit here if the index is not a constant!
static CONSTEXPR const char pow2char[8] = {1,2,4,8,16,32,64,128u};
#else
// Revert the above to a real bit shift through some C++ operator magic. That saves me from reverting all the code that uses this construct.
static struct 
{
	constexpr uint8_t operator[](int index) const { return 1 << index; };
} pow2char;
#endif

static FORCE_INLINE void bitmap_set(uint8_t *const ptr, int const n) { ptr[n>>3] |= pow2char[n&7]; }
static FORCE_INLINE void bitmap_clear(uint8_t *const ptr, int const n) { ptr[n>>3] &= ~pow2char[n&7]; }
static FORCE_INLINE CONSTEXPR char bitmap_test(uint8_t const *const ptr, int const n) { return ptr[n>>3] & pow2char[n&7]; }

////////// Utility functions //////////

// breadth-first search helpers
template <typename T>
void bfirst_search_init(T *const list, uint8_t *const bitmap, T *const eltnumptr, int const maxelts, int const firstelt)
{
    Bmemset(bitmap, 0, (maxelts+7)>>3);

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

// Copy min(strlen(src)+1, n) characters into dst, always terminate with a NUL.
static FORCE_INLINE char *Bstrncpyz(char *dst, const char *src, bsize_t n)
{
    Bstrncpy(dst, src, n);
    dst[n-1] = 0;
    return dst;
}

////////// PANICKING ALLOCATION WRAPPERS //////////


#define Xstrdup(s)    (strdup(s))
#define Xmalloc(size) (M_Malloc(size))
#define Xcalloc(nmemb, size) (M_Calloc(nmemb, size))
#define Xrealloc(ptr, size)  (M_Realloc(ptr, size))
#define Xfree(ptr) (M_Free(ptr))

////////// Inlined external libraries //////////

#include "fix16.h"
#include "vectors.h"

inline FVector3 GetSoundPos(const vec3_t *pos)
{
    // converts a Build coordinate to a sound system coordinate
    const float xmul = 1 / 16.f;
    const float ymul = -1 / 16.f;
    const float zmul = -1 / 256.f;
    return { pos->x* xmul, pos->z* zmul, pos->y* ymul };
}

/* End dependence on compat.o object. */


////////// EDuke32-specific features //////////

#ifndef TRUE
# define TRUE 1
#endif

#ifndef FALSE
# define FALSE 0
#endif

#define WITHKPLIB

#endif // compat_h_
