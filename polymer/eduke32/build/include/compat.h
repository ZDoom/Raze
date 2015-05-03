// Compatibility declarations for things which might not be present in
// certain build environments. It also levels the playing field caused
// by different platforms.

#ifndef compat_h_
#define compat_h_

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

#if !defined(_MSC_VER) || _MSC_FULL_VER < 180031101
#ifdef UNREFERENCED_PARAMETER
#undef UNREFERENCED_PARAMETER
#endif

#define UNREFERENCED_PARAMETER(x) x = x
#endif

#if defined __GNUC__ || defined __clang__
# define ATTRIBUTE(attrlist) __attribute__(attrlist)
#else
# define ATTRIBUTE(attrlist)
#endif

#if !defined __clang__ && !defined USING_LTO
# define ATTRIBUTE_OPTIMIZE(str) ATTRIBUTE((optimize(str)))
#else
# define ATTRIBUTE_OPTIMIZE(str)
#endif

#if defined __GNUC__ || __has_builtin(__builtin_expect)
#define EDUKE32_PREDICT_TRUE(x)       __builtin_expect(!!(x),1)
#define EDUKE32_PREDICT_FALSE(x)     __builtin_expect(!!(x),0)
#else
#define EDUKE32_PREDICT_TRUE(x) (x)
#define EDUKE32_PREDICT_FALSE(x) (x)
#endif

#if EDUKE32_GCC_PREREQ(4,5)  || __has_builtin(__builtin_unreachable)
#define EDUKE32_UNREACHABLE_SECTION(...)   __builtin_unreachable()
#elif _MSC_VER
#define EDUKE32_UNREACHABLE_SECTION(...)   __assume(0)
#else
#define EDUKE32_UNREACHABLE_SECTION(...) __VA_ARGS__
#endif

#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif

#if defined __FreeBSD__ || defined __NetBSD__ || defined __OpenBSD__ || defined __bsdi__ || defined __DragonFly__
# define EDUKE32_BSD
#endif

#if !defined __APPLE__ && (!defined EDUKE32_BSD || !__STDC__)
# include <malloc.h>
#endif

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#ifdef __APPLE__
# include <TargetConditionals.h>
# if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#  define EDUKE32_IOS
# else
#  define EDUKE32_OSX
#  if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_3
#   include <CoreFoundation/CoreFoundation.h>
#  endif
#  include <CoreServices/CoreServices.h>
# endif
#endif

#if defined __ANDROID__ || defined EDUKE32_IOS
# define EDUKE32_TOUCH_DEVICES
# define EDUKE32_GLES
#endif

// This gives us access to 'intptr_t' and 'uintptr_t', which are
// abstractions to the size of a pointer on a given platform
// (ie, they're guaranteed to be the same size as a pointer)

#undef __USE_MINGW_ANSI_STDIO // Workaround for MinGW-w64.

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
# define __STDC_LIMIT_MACROS
#endif
#if defined(HAVE_INTTYPES) || defined(__cplusplus)
# include <stdint.h>
# include <inttypes.h>

// Ghetto. Blame devkitPPC's faulty headers.
# ifdef GEKKO
#  undef PRIdPTR
#  define PRIdPTR "d"
#  undef PRIxPTR
#  define PRIxPTR "x"
#  undef SCNx32
#  define SCNx32 "x"
# endif

#elif defined(_MSC_VER)
# include "msvc/inttypes.h" // from http://code.google.com/p/msinttypes/
#endif

#ifndef _MSC_VER
#  ifndef __fastcall
#    if defined(__GNUC__) && defined(__i386__)
#      define __fastcall __attribute__((fastcall))
#    else
#      define __fastcall
#    endif
#  endif
#endif

#ifndef TRUE
#  define TRUE 1
#endif

#ifndef FALSE
#  define FALSE 0
#endif

#define WITHKPLIB

#include "libdivide.h"

// Define this to rewrite all 'B' versions to library functions. This
// is for platforms which give us a standard sort of C library so we
// link directly. Platforms like PalmOS which don't have a standard C
// library will need to wrap these functions with suitable emulations.
#define compat_h_macrodef__

#include <stdarg.h>
#include <stddef.h>

#ifdef compat_h_macrodef__
# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <time.h>
# include <fcntl.h>
# include <ctype.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
# if defined(_WIN32)
#  include <io.h>
# else
#  include <unistd.h>
# endif
#endif

#include <assert.h>

#ifdef EFENCE
# include <efence.h>
#elif defined DMALLOC
# include <dmalloc.h>
#endif

#if defined(_MSC_VER)
# include <direct.h>
# define longlong(x) x##i64
# if _MSC_VER < 1800
#  define inline __inline
# endif
#else
# define longlong(x) x##ll
#endif

#if defined(__arm__)
# define Bsqrt __builtin_sqrt
# define Bsqrtf __builtin_sqrtf
#else
# define Bsqrt sqrt
# define Bsqrtf sqrtf
#endif

#ifndef NULL
# define NULL ((void *)0)
#endif

#if DEBUGGINGAIDS>=2
# define DEBUG_MAIN_ARRAYS
#endif

#ifndef DISABLE_INLINING
# define EXTERN_INLINE static inline
# define EXTERN_INLINE_HEADER static inline
#else
# define EXTERN_INLINE __fastcall
# define EXTERN_INLINE_HEADER extern __fastcall
#endif

#ifndef FORCE_INLINE
# ifdef _MSC_VER    // Visual Studio
#  define FORCE_INLINE static __forceinline
# else
#  ifdef __GNUC__
#    define FORCE_INLINE static inline __attribute__((always_inline))
#  else
#    define FORCE_INLINE static inline
#  endif
# endif
#endif

#if !defined DEBUG_MAIN_ARRAYS
# define HAVE_CLIPSHAPE_FEATURE
#endif

// redefined for apple/ppc, which chokes on stderr when linking...
#define ERRprintf(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)

#if defined(__linux)
# include <endian.h>
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define B_LITTLE_ENDIAN 1
#  define B_BIG_ENDIAN    0
# elif __BYTE_ORDER == __BIG_ENDIAN
#  define B_LITTLE_ENDIAN 0
#  define B_BIG_ENDIAN    1
# endif
# define B_USE_COMPAT_SWAP 1

#elif defined(GEKKO) || defined(__ANDROID__)
# define B_LITTLE_ENDIAN 0
# define B_BIG_ENDIAN 1
# define B_USE_COMPAT_SWAP 1

#elif defined(__OpenBSD__)
# include <machine/endian.h>
# if _BYTE_ORDER == _LITTLE_ENDIAN
#  define B_LITTLE_ENDIAN 1
#  define B_BIG_ENDIAN    0
# elif _BYTE_ORDER == _BIG_ENDIAN
#  define B_LITTLE_ENDIAN 0
#  define B_BIG_ENDIAN    1
# endif
# define B_SWAP64(x) __swap64(x)
# define B_SWAP32(x) __swap32(x)
# define B_SWAP16(x) __swap16(x)

#elif defined EDUKE32_BSD
# include <sys/endian.h>
# if _BYTE_ORDER == _LITTLE_ENDIAN
#  define B_LITTLE_ENDIAN 1
#  define B_BIG_ENDIAN    0
# elif _BYTE_ORDER == _BIG_ENDIAN
#  define B_LITTLE_ENDIAN 0
#  define B_BIG_ENDIAN    1
# endif
# define B_SWAP64(x) __bswap64(x)
# define B_SWAP32(x) __bswap32(x)
# define B_SWAP16(x) __bswap16(x)

#elif defined(__APPLE__)
# if !defined __x86_64__ && defined __GNUC__
// PK 20110617: is*() crashes for me in x86 code compiled from 64-bit, and gives link errors on ppc
//              This hack patches all occurences.
#  define isdigit(ch) ({ int32_t c__dontuse_=ch; c__dontuse_>='0' && c__dontuse_<='9'; })
#  define isalpha(ch) ({ int32_t c__dontuse2_=ch; (c__dontuse2_>='A' && c__dontuse2_<='Z') || (c__dontuse2_>='a' && c__dontuse2_<='z'); })
#  define isalnum(ch2)  ({ int32_t c2__dontuse_=ch2; isalpha(c2__dontuse_) || isdigit(c2__dontuse_); })
#  if defined __BIG_ENDIAN__
#    define isspace(ch)  ({ int32_t c__dontuse_=ch; (c__dontuse_==' ' || c__dontuse_=='\t' || c__dontuse_=='\n' || c__dontuse_=='\v' || c__dontuse_=='\f' || c__dontuse_=='\r'); })
#    define isprint(ch)  ({ int32_t c__dontuse_=ch; (c__dontuse_>=0x20 && c__dontuse_<0x7f); })
#    undef ERRprintf
#    define ERRprintf(fmt, ...) printf(fmt, ## __VA_ARGS__)
#  endif
# endif
# if defined(__LITTLE_ENDIAN__)
#  define B_LITTLE_ENDIAN 1
#  define B_BIG_ENDIAN    0
# elif defined(__BIG_ENDIAN__)
#  define B_LITTLE_ENDIAN 0
#  define B_BIG_ENDIAN    1
# endif
# include <libkern/OSByteOrder.h>
# define B_SWAP64(x) OSSwapConstInt64(x)
# define B_SWAP32(x) OSSwapConstInt32(x)
# define B_SWAP16(x) OSSwapConstInt16(x)

#elif defined(__BEOS__)
# include <posix/endian.h>
# if LITTLE_ENDIAN != 0
#  define B_LITTLE_ENDIAN 1
#  define B_BIG_ENDIAN    0
# elif BIG_ENDIAN != 0
#  define B_LITTLE_ENDIAN 0
#  define B_BIG_ENDIAN    1
# endif
# define B_USE_COMPAT_SWAP 1

#elif defined(__QNX__)
# if defined __LITTLEENDIAN__
#  define B_LITTLE_ENDIAN 1
#  define B_BIG_ENDIAN    0
# elif defined __BIGENDIAN__
#  define B_LITTLE_ENDIAN 0
#  define B_BIG_ENDIAN    1
# endif
# define B_USE_COMPAT_SWAP 1

#elif defined(__sun)
# if defined _LITTLE_ENDIAN
#  define B_LITTLE_ENDIAN 1
#  define B_BIG_ENDIAN    0
# elif defined _BIG_ENDIAN
#  define B_LITTLE_ENDIAN 0
#  define B_BIG_ENDIAN    1
# endif
# define B_USE_COMPAT_SWAP 1

#elif defined(_WIN32) || defined(SKYOS) || defined(__SYLLABLE__)
# define B_LITTLE_ENDIAN 1
# define B_BIG_ENDIAN    0
# define B_USE_COMPAT_SWAP 1
#endif

#if !defined(B_LITTLE_ENDIAN) || !defined(B_BIG_ENDIAN)
# error Unknown endianness
#endif

#if defined _LP64 || defined __LP64__ || defined __64BIT__ || _ADDR64 || defined _WIN64 || defined __arch64__ ||       \
__WORDSIZE == 64 || (defined __sparc && defined __sparcv9) || defined __x86_64 || defined __amd64 ||                   \
defined __x86_64__ || defined __amd64__ || defined _M_X64 || defined _M_IA64 || defined __ia64 || defined __IA64__

# define BITNESS64

#endif

#ifdef __cplusplus

# ifndef SCREWED_UP_CPP
//   using namespace std;
# endif

extern "C" {
#endif

#if defined B_USE_COMPAT_SWAP
FORCE_INLINE uint16_t B_SWAP16(uint16_t s) { return (s >> 8) | (s << 8); }

# if !defined NOASM && defined __i386__ && defined _MSC_VER
FORCE_INLINE uint32_t B_SWAP32(uint32_t a)
{
    _asm
    {
        mov eax, a
        bswap eax
    }
}
# elif !defined NOASM && defined __i386__ && defined __GNUC__
FORCE_INLINE uint32_t B_SWAP32(uint32_t a)
{
    __asm__ __volatile__("bswap %0" : "+r"(a) : : "cc");
    return a;
}
# else
FORCE_INLINE uint32_t B_SWAP32(uint32_t l)
{
    return ((l >> 8) & 0xff00) | ((l & 0xff00) << 8) | (l << 24) | (l >> 24);
}
# endif
FORCE_INLINE uint64_t B_SWAP64(uint64_t l)
{
    return (l >> 56) | ((l >> 40) & 0xff00) | ((l >> 24) & 0xff0000) | ((l >> 8) & 0xff000000) |
        ((l & 255) << 56) | ((l & 0xff00) << 40) | ((l & 0xff0000) << 24) | ((l & 0xff000000) << 8);
}
#endif

FORCE_INLINE void B_BUF16(uint8_t *buf, uint16_t x)
{
    buf[0] = (x & 0x00FF);
    buf[1] = (x & 0xFF00) >> 8;
}
FORCE_INLINE void B_BUF32(uint8_t *buf, uint32_t x)
{
    buf[0] = (x & 0x000000FF);
    buf[1] = (x & 0x0000FF00) >> 8;
    buf[2] = (x & 0x00FF0000) >> 16;
    buf[3] = (x & 0xFF000000) >> 24;
}
#if 0
// i686-apple-darwin11-llvm-gcc-4.2 complains "integer constant is too large for 'long' type"
FORCE_INLINE void B_BUF64(uint8_t *buf, uint64_t x)
{
    buf[0] = (x & 0x00000000000000FF);
    buf[1] = (x & 0x000000000000FF00) >> 8;
    buf[2] = (x & 0x0000000000FF0000) >> 16;
    buf[3] = (x & 0x00000000FF000000) >> 24;
    buf[4] = (x & 0x000000FF00000000) >> 32;
    buf[5] = (x & 0x0000FF0000000000) >> 40;
    buf[6] = (x & 0x00FF000000000000) >> 48;
    buf[7] = (x & 0xFF00000000000000) >> 56;
}
#endif

FORCE_INLINE uint16_t B_UNBUF16(const uint8_t *buf) { return (buf[1] << 8) | (buf[0]); }
FORCE_INLINE uint32_t B_UNBUF32(const uint8_t *buf)
{
    return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | (buf[0]);
}
FORCE_INLINE uint64_t B_UNBUF64(const uint8_t *buf)
{
    return ((uint64_t)buf[7] << 56) | ((uint64_t)buf[6] << 48) | ((uint64_t)buf[5] << 40) |
        ((uint64_t)buf[4] << 32) | (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | (buf[0]);
}

#if defined BITNESS64 && (defined __SSE2__ || defined _MSC_VER)
#include <emmintrin.h>
FORCE_INLINE int32_t Blrintf(const float x)
{
    __m128 xx = _mm_load_ss(&x);
    return _mm_cvtss_si32(xx);
}
#elif defined (_MSC_VER)
FORCE_INLINE int32_t Blrintf(const float x)
{
    int n;
    __asm fld x;
    __asm fistp n;
    return n;
}
#else
#include <math.h>
#define Blrintf lrintf
#endif

#if B_LITTLE_ENDIAN == 1
# define B_LITTLE64(x) (x)
# define B_BIG64(x)    B_SWAP64(x)
# define B_LITTLE32(x) (x)
# define B_BIG32(x)    B_SWAP32(x)
# define B_LITTLE16(x) (x)
# define B_BIG16(x)    B_SWAP16(x)
#elif B_BIG_ENDIAN == 1
# define B_LITTLE64(x) B_SWAP64(x)
# define B_BIG64(x)    (x)
# define B_LITTLE32(x) B_SWAP32(x)
# define B_BIG32(x)    (x)
# define B_LITTLE16(x) B_SWAP16(x)
# define B_BIG16(x)    (x)
#endif

#ifndef FP_OFF
# define FP_OFF(__p) ((uintptr_t)(__p))
#endif

#ifdef compat_h_macrodef__

# ifndef O_BINARY
#  define O_BINARY 0
# endif
# ifndef O_TEXT
#  define O_TEXT 0
# endif

# ifndef F_OK
#  define F_OK 0
# endif

# define BO_BINARY O_BINARY
# define BO_TEXT   O_TEXT
# define BO_RDONLY O_RDONLY
# define BO_WRONLY O_WRONLY
# define BO_RDWR   O_RDWR
# define BO_APPEND O_APPEND
# define BO_CREAT  O_CREAT
# define BO_TRUNC  O_TRUNC
# define BS_IRGRP  S_IRGRP
# define BS_IWGRP  S_IWGRP
# define BS_IEXEC  S_IEXEC
# ifdef __ANDROID__
#  define BS_IWRITE S_IWUSR
#  define BS_IREAD  S_IRUSR
# else
#  define BS_IWRITE S_IWRITE
#  define BS_IREAD  S_IREAD
# endif
# define BS_IFIFO  S_IFIFO
# define BS_IFCHR  S_IFCHR
# define BS_IFBLK  S_IFBLK
# define BS_IFDIR  S_IFDIR
# define BS_IFREG  S_IFREG
# define BSEEK_SET SEEK_SET
# define BSEEK_CUR SEEK_CUR
# define BSEEK_END SEEK_END
#else
# define BO_BINARY 0
# define BO_TEXT   1
# define BO_RDONLY 2
# define BO_WRONLY 4
# define BO_RDWR   6
# define BO_APPEND 8
# define BO_CREAT  16
# define BO_TRUNC  32
# define BS_IRGRP  0
# define BS_IWGRP  0
# define BS_IEXEC  1
# define BS_IWRITE 2
# define BS_IREAD  4
# define BS_IFIFO  0x1000
# define BS_IFCHR  0x2000
# define BS_IFBLK  0x3000
# define BS_IFDIR  0x4000
# define BS_IFREG  0x8000
# define BSEEK_SET 0
# define BSEEK_CUR 1
# define BSEEK_END 2
#endif

#ifdef UNDERSCORES
# define ASMSYM(x) "_" x
#else
# define ASMSYM(x) x
#endif

#ifndef min
# define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
# define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#if __GNUC__ >= 4
# define CLAMP_DECL FORCE_INLINE __attribute__((warn_unused_result))
#else
# define CLAMP_DECL FORCE_INLINE
#endif

// Clamp <in> to [<min>..<max>]. The case in <= min is handled first.
CLAMP_DECL int32_t clamp(int32_t in, int32_t min, int32_t max) { return in <= min ? min : (in >= max ? max : in); }

// Clamp <in> to [<min>..<max>]. The case in >= max is handled first.
CLAMP_DECL int32_t clamp2(int32_t in, int32_t min, int32_t max) { return in >= max ? max : (in <= min ? min : in); }

// Clamp <in> to [<min>..<max>]. The case in <= min is handled first.
CLAMP_DECL float fclamp(float in, float min, float max) { return in <= min ? min : (in >= max ? max : in); }

// Clamp <in> to [<min>..<max>]. The case in >= max is handled first.
CLAMP_DECL float fclamp2(float in, float min, float max) { return in >= max ? max : (in <= min ? min : in); }

#define BMAX_PATH 256

/* Static assertions, based on source found in LuaJIT's src/lj_def.h. */
#define EDUKE32_ASSERT_NAME2(name, line) name ## line
#define EDUKE32_ASSERT_NAME(line) EDUKE32_ASSERT_NAME2(eduke32_assert_, line)
#ifdef __COUNTER__
# define EDUKE32_STATIC_ASSERT(cond) \
    extern void EDUKE32_ASSERT_NAME(__COUNTER__)(int STATIC_ASSERTION_FAILED[(cond)?1:-1])
#else
# define EDUKE32_STATIC_ASSERT(cond) \
    extern void EDUKE32_ASSERT_NAME(__LINE__)(int STATIC_ASSERTION_FAILED[(cond)?1:-1])
#endif

struct Bdirent
{
    uint16_t namlen;
    char *name;
    uint32_t mode;
    uint32_t size;
    uint32_t mtime;
};
typedef void BDIR;

BDIR *Bopendir(const char *name);
struct Bdirent *Breaddir(BDIR *dir);
int32_t Bclosedir(BDIR *dir);

#ifdef _MSC_VER
typedef intptr_t ssize_t;
#endif

#ifdef compat_h_macrodef__
  typedef FILE BFILE;
# define bsize_t size_t
# define bssize_t ssize_t
#else
  typedef void          BFILE;
  typedef uint32_t bsize_t;
  typedef int32_t bssize_t;
#endif


typedef struct {
    int32_t x, y;
} vec2_t;

typedef struct {
    int32_t x, y, z;
} vec3_t;

typedef struct {
    float x, y;
} vec2f_t;

typedef struct {
    union { float x; float d; };
    union { float y; float u; };
    union { float z; float v; };
} vec3f_t;

EDUKE32_STATIC_ASSERT(sizeof(vec3f_t) == sizeof(float) * 3);

typedef struct {
    union { double x; double d; };
    union { double y; double u; };
    union { double z; double v; };
} vec3d_t;

EDUKE32_STATIC_ASSERT(sizeof(vec3d_t) == sizeof(double) * 3);

#if RAND_MAX == 32767
FORCE_INLINE uint16_t system_15bit_rand(void) { return (uint16_t)rand(); }
#else  // RAND_MAX > 32767, assumed to be of the form 2^k - 1
FORCE_INLINE uint16_t system_15bit_rand(void) { return ((uint16_t)rand())&0x7fff; }
#endif

#if defined(_MSC_VER)
// XXX: non-__compat_h_macrodef__ version?
#define strtoll _strtoi64
#endif

#ifdef compat_h_macrodef__
# define Bassert assert
# define Brand rand
# define Balloca alloca
# define Bmalloc malloc
# define Bcalloc calloc
# define Brealloc realloc
# define Bfree free
# if defined(__cplusplus) && defined(_MSC_VER)
#  define Bstrdup _strdup
#  define Bchdir _chdir
#  define Bgetcwd _getcwd
# else
#  define Bstrdup strdup
#  define Bchdir chdir
#  define Bgetcwd getcwd
# endif
# define Bopen open
# define Bclose close
# define Bwrite write
# define Bread read
# define Blseek lseek
# if defined(__GNUC__)
#  define Btell(h) lseek(h,0,SEEK_CUR)
# else
#  define Btell tell
# endif
# ifdef _MSC_VER
#  define Bstat stat
#  define Bfstat fstat
# else
#  define Bstat stat
#  define Bfstat fstat
# endif
# define Bfileno fileno
# define Bferror ferror
# define Bfopen fopen
# define Bfclose fclose
# define Bfflush fflush
# define Bfeof feof
# define Bfgetc fgetc
# define Brewind rewind
# define Bfgets fgets
# define Bfputc fputc
# define Bfputs fputs
# define Bfread fread
# define Bfwrite fwrite
# define Bfprintf fprintf
# define Bfscanf fscanf
# define Bfseek fseek
# define Bftell ftell
# define Bputs puts
# define Bstrcpy strcpy
# define Bstrncpy strncpy
# define Bstrcmp strcmp
# define Bstrncmp strncmp
# if defined(_MSC_VER)
#  define Bstrcasecmp _stricmp
#  define Bstrncasecmp _strnicmp
# elif defined(__QNX__)
#  define Bstrcasecmp stricmp
#  define Bstrncasecmp strnicmp
# else
#  define Bstrcasecmp strcasecmp
#  define Bstrncasecmp strncasecmp
# endif
# if defined(_WIN32)
#  define Bstrlwr strlwr
#  define Bstrupr strupr
#  define Bmkdir(s,x) mkdir(s)
# else
#  define Bmkdir mkdir
# endif
# define Bstrcat strcat
# define Bstrncat strncat
# define Bstrlen strlen
# define Bstrchr strchr
# define Bstrrchr strrchr
// XXX: different across 32- and 64-bit archs (e.g.
// parsing the decimal representation of 0xffffffff,
// 4294967295 -- long is signed, so strtol would
// return LONG_MAX (== 0x7fffffff on 32-bit archs))
# define Batoi(str) ((int32_t)strtol(str, NULL, 10))
# define Batol(str) (strtol(str, NULL, 10))
# define Batof(str) (strtod(str, NULL))
# define Bstrtol strtol
# define Bstrtoul strtoul
# define Bstrtod strtod
# define Bstrstr strstr
# define Bislower islower
# define Bisupper isupper
# define Bisdigit isdigit
# define Btoupper toupper
# define Btolower tolower
# define Bmemcpy memcpy
# define Bmemmove memmove
# define Bmemchr memchr
# define Bmemset memset
# define Bmemcmp memcmp
# define Bscanf scanf
# define Bprintf printf
# define Bsscanf sscanf
# define Bsprintf sprintf
# ifdef _MSC_VER
#  define Bsnprintf _snprintf
#  define Bvsnprintf _vsnprintf
# else
#  define Bsnprintf snprintf
#  define Bvsnprintf vsnprintf
# endif
# define Bvfprintf vfprintf
# define Bgetenv getenv
# define Btime() time(NULL)
# define Butime utime

#else

void Bassert(int);
int32_t Brand(void);
void *Bmalloc(bsize_t size);
void Bfree(void *ptr);
int32_t Bopen(const char *pathname, int32_t flags, unsigned mode);
int32_t Bclose(int32_t fd);
bssize_t Bwrite(int32_t fd, const void *buf, bsize_t count);
bssize_t Bread(int32_t fd, void *buf, bsize_t count);
int32_t Blseek(int32_t fildes, int32_t offset, int32_t whence);
BFILE *Bfopen(const char *path, const char *mode);
int32_t Bfclose(BFILE *stream);
int32_t Bfeof(BFILE *stream);
int32_t Bfgetc(BFILE *stream);
void Brewind(BFILE *stream);
char *Bfgets(char *s, int32_t size, BFILE *stream);
int32_t Bfputc(int32_t c, BFILE *stream);
int32_t Bfputs(const char *s, BFILE *stream);
bsize_t Bfread(void *ptr, bsize_t size, bsize_t nmemb, BFILE *stream);
bsize_t Bfwrite(const void *ptr, bsize_t size, bsize_t nmemb, BFILE *stream);
char *Bstrdup(const char *s);
char *Bstrcpy(char *dest, const char *src);
char *Bstrncpy(char *dest, const char *src, bsize_t n);
int32_t Bstrcmp(const char *s1, const char *s2);
int32_t Bstrncmp(const char *s1, const char *s2, bsize_t n);
int32_t Bstrcasecmp(const char *s1, const char *s2);
int32_t Bstrncasecmp(const char *s1, const char *s2, bsize_t n);
char *Bstrcat(char *dest, const char *src);
char *Bstrncat(char *dest, const char *src, bsize_t n);
bsize_t Bstrlen(const char *s);
char *Bstrchr(const char *s, int32_t c);
char *Bstrrchr(const char *s, int32_t c);
int32_t Batoi(const char *nptr);
int32_t Batol(const char *nptr);
int32_t Bstrtol(const char *nptr, char **endptr, int32_t base);
uint32_t Bstrtoul(const char *nptr, char **endptr, int32_t base);
void *Bmemcpy(void *dest, const void *src, bsize_t n);
void *Bmemmove(void *dest, const void *src, bsize_t n);
void *Bmemchr(const void *s, int32_t c, bsize_t n);
void *Bmemset(void *s, int32_t c, bsize_t n);
int32_t Bmemcmp(const void *s1, const void *s2, bsize_t n);
int32_t Bprintf(const char *format, ...) ATTRIBUTE((format(printf, 1, 2)));
int32_t Bsprintf(char *str, const char *format, ...) ATTRIBUTE((format(printf, 2, 3)));
int32_t Bsnprintf(char *str, bsize_t size, const char *format, ...) ATTRIBUTE((format(printf, 3, 4)));
int32_t Bvsnprintf(char *str, bsize_t size, const char *format, va_list ap);
char *Bgetcwd(char *buf, bsize_t size);
char *Bgetenv(const char *name);
#endif

char *Bgethomedir(void);
char *Bgetappdir(void);
uint32_t Bgetsysmemsize(void);
int32_t Bcorrectfilename(char *filename, int32_t removefn);
int32_t Bcanonicalisefilename(char *filename, int32_t removefn);
char *Bgetsystemdrives(void);
int32_t Bfilelength(int32_t fd);
char *Bstrtoken(char *s, const char *delim, char **ptrptr, int chop);
char *Bstrtolower(char *str);
#define Bwildmatch wildmatch

#if !defined(_WIN32)
char *Bstrlwr(char *);
char *Bstrupr(char *);
#endif

// Copy min(strlen(src)+1, n) characters into dst, always terminate with a NUL.
FORCE_INLINE char *Bstrncpyz(char *dst, const char *src, bsize_t n)
{
    Bstrncpy(dst, src, n);
    dst[n-1] = 0;
    return dst;
}

// Append extension when <outbuf> contains no dot.
// <ext> can be like ".mhk" or like "_crash.map", no need to start with a dot.
// The ugly name is deliberate: we should be checking the sizes of all buffers!
static inline void append_ext_UNSAFE(char *outbuf, const char *ext)
{
    char *p = Bstrrchr(outbuf,'.');

    if (!p)
        Bstrcat(outbuf, ext);
    else
        Bstrcpy(p, ext);
}

#ifdef DEBUGGINGAIDS
extern void xalloc_set_location(int32_t line, const char *file, const char *func);
#endif
void set_memerr_handler(void (*handlerfunc)(int32_t, const char *, const char *));
void handle_memerr(void);

FORCE_INLINE char *xstrdup(const char *s)
{
    char *ptr = Bstrdup(s);
    if (ptr == NULL) handle_memerr();
    return ptr;
}

FORCE_INLINE void *xmalloc(const bsize_t size)
{
    void *ptr = Bmalloc(size);
    if (ptr == NULL) handle_memerr();
    return ptr;
}

FORCE_INLINE void *xcalloc(const bsize_t nmemb, const bsize_t size)
{
    void *ptr = Bcalloc(nmemb, size);
    if (ptr == NULL) handle_memerr();
    return ptr;
}

FORCE_INLINE void *xrealloc(void * const ptr, const bsize_t size)
{
    void *newptr = Brealloc(ptr, size);

    // According to the C Standard,
    //  - ptr == NULL makes realloc() behave like malloc()
    //  - size == 0 make it behave like free() if ptr != NULL
    // Since we want to catch an out-of-mem in the first case, this leaves:
    if (newptr == NULL && size != 0)
        handle_memerr();

    return newptr;
}

FORCE_INLINE void *xaligned_malloc(const bsize_t alignment, const bsize_t size)
{
#ifdef _WIN32
    void *ptr = _aligned_malloc(size, alignment);
#elif defined __APPLE__ || defined EDUKE32_BSD
    void *ptr = NULL;
    posix_memalign(&ptr, alignment, size);
#else
    void *ptr = memalign(alignment, size);
#endif

    if (ptr == NULL) handle_memerr();
    return ptr;
}


#ifdef __cplusplus
}
#endif

#define DO_FREE_AND_NULL(var) do { \
    if (var != NULL) { Bfree(var); var = NULL; } \
} while (0)

#define ALIGNED_FREE_AND_NULL(var) do { \
    if (var != NULL) { Baligned_free(var); var = NULL; } \
} while (0)

#define MAYBE_FCLOSE_AND_NULL(fileptr) do { \
    if (fileptr) { Bfclose(fileptr); fileptr=NULL; } \
} while (0)

#define ARRAY_SIZE(Ar) (sizeof(Ar)/sizeof((Ar)[0]))
#define ARRAY_SSIZE(Ar) (bssize_t)ARRAY_SIZE(Ar)

////////// PANICKING ALLOCATION MACROS (wrapping the functions) //////////
#ifdef DEBUGGINGAIDS
// Detection of __func__ or equivalent functionality, found in SDL_assert.h
# if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 supports __func__ as a standard. */
#  define EDUKE32_FUNCTION __func__
# elif ((__GNUC__ >= 2) || defined(_MSC_VER))
#  define EDUKE32_FUNCTION __FUNCTION__
# else
#  define EDUKE32_FUNCTION "???"
# endif

# define EDUKE32_PRE_XALLLOC xalloc_set_location(__LINE__, __FILE__, EDUKE32_FUNCTION)
# define Xstrdup(s) (EDUKE32_PRE_XALLLOC, xstrdup(s))
# define Xmalloc(size) (EDUKE32_PRE_XALLLOC, xmalloc(size))
# define Xcalloc(nmemb, size) (EDUKE32_PRE_XALLLOC, xcalloc(nmemb, size))
# define Xrealloc(ptr, size) (EDUKE32_PRE_XALLLOC, xrealloc(ptr, size))
# define Xaligned_alloc(size, alignment) (EDUKE32_PRE_XALLLOC, xaligned_malloc(size, alignment))
# define Bexit(status) do { initprintf("exit(%d) at %s:%d in %s()\n", status, __FILE__, __LINE__, EDUKE32_FUNCTION); exit(status); } while (0)
#else
# define Xstrdup xstrdup
# define Xmalloc xmalloc
# define Xcalloc xcalloc
# define Xrealloc xrealloc
# define Xaligned_alloc xaligned_malloc
# define Bexit exit
#endif

#ifdef _WIN32
# define Baligned_free(ptr) _aligned_free(ptr)
#else
# define Baligned_free(ptr) Bfree(ptr)
#endif

static inline void maybe_grow_buffer(char ** const buffer, int32_t * const buffersize, int32_t const newsize)
{
    if (newsize > *buffersize)
    {
        *buffer = (char *)Xrealloc(*buffer, newsize);
        *buffersize = newsize;
    }
}

//////////

#endif // compat_h_
