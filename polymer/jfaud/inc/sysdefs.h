#ifndef __sysdefs_h__
#define __sysdefs_h__

#if defined(__WATCOMC__) && ((__WATCOMC__ -0) < 1230)
# define SCREWED_UP_CPP
#endif

#if defined(__WATCOMC__) || defined(_MSC_VER) || (defined(_WIN32) && defined(__GNUC__))
typedef signed char      int8_t;
typedef unsigned char    uint8_t;
typedef signed short     int16_t;
typedef unsigned short   uint16_t;
typedef signed int       int32_t;
typedef unsigned int     uint32_t;
#ifndef __GNUC__
typedef signed __int64   int64_t;
typedef unsigned __int64 uint64_t;
#else	// __GNUC__
typedef signed long long int   int64_t;
typedef unsigned long long int uint64_t;
#endif	// __GNUC__
typedef signed int       intptr_t;
typedef unsigned int     uintptr_t;
#define INT8_C(c)    c
#define UINT8_C(c)   c ## u
#define INT16_C(c)   c
#define UINT16_C(c)  c ## u
#define INT32_C(c)   c
#define UINT32_C(c)  c ## u
#ifndef __GNUC__
#define INT64_C(c)  c ## i64
#define UINT64_C(c) c ## ui64
#else	// __GNUC__
#define INT64_C(c)  c ## ll
#define UINT64_C(c) c ## ull
#endif	// __GNUC__
#define inline __inline

#else	// not watcomc, msc, mingw32
# define __STDC_CONSTANT_MACROS
# include <stdint.h>
#endif

#if defined(linux)
# define PLATFORMLINUX   1
#elif defined(__FreeBSD__)
# define PLATFORMBSD     1
#elif defined(__APPLE__) && defined(__MACH__)
# define PLATFORMDARWIN  1
#elif defined(_WIN32)
# define PLATFORMWINDOWS 1
#endif

#if defined(PLATFORMLINUX)
# include <endian.h>
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define B_LITTLE_ENDIAN 1
#  define B_BIG_ENDIAN    0
# elif __BYTE_ORDER == __BIG_ENDIAN
#  define B_LITTLE_ENDIAN 0
#  define B_BIG_ENDIAN    1
# endif
# define B_ENDIAN_C_INLINE 1
#elif defined(PLATFORMBSD)
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
#elif defined(PLATFORMDARWIN)
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
#elif defined(PLATFORMWINDOWS)
# define B_LITTLE_ENDIAN 1
# define B_BIG_ENDIAN    0
# define B_ENDIAN_C_INLINE 1
#endif
#if !defined(B_LITTLE_ENDIAN) || !defined(B_BIG_ENDIAN)
# error Unknown endianness
#endif

#if defined B_ENDIAN_X86_INLINE
# if defined(_MSC_VER)
	// inline asm using bswap/xchg
# elif defined(__GNUC__)
	// inline asm using bswap/xchg
# elif defined(__WATCOMC__)
	// inline asm using bswap/xchg
# endif
#elif defined B_ENDIAN_C_INLINE
static inline uint16_t B_SWAP16(uint16_t s) { return (s>>8)|(s<<8); }
static inline uint32_t B_SWAP32(uint32_t l) { return ((l>>8)&0xff00)|((l&0xff00)<<8)|(l<<24)|(l>>24); }
static inline uint64_t B_SWAP64(uint64_t l) { return (l>>56)|((l>>40)&0xff00)|((l>>24)&0xff0000)|((l>>8)&0xff000000)|((l&255)<<56)|((l&0xff00)<<40)|((l&0xff0000)<<24)|((l&0xff000000)<<8); }
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

#if defined(__WATCOMC__) || defined(_MSC_VER)
# define strncasecmp strnicmp
# define strcasecmp  stricmp
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

#ifndef min
# define min(x,y) ((x)<(y)?(x):(y))
#endif

#ifndef max
# define max(x,y) ((x)>(y)?(x):(y))
#endif

#define arsiz(x) (sizeof(x)/sizeof(x[0]))

#endif
