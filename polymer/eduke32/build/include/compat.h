// Compatibility declarations for things which might not be present in
// certain build environments. It also levels the playing field caused
// by different platforms.

#ifndef __compat_h__
#define __compat_h__

# ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
# endif

#ifndef UNREFERENCED_PARAMETER
    #define UNREFERENCED_PARAMETER(x) x=x
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

#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif

// This gives us access to 'intptr_t' and 'uintptr_t', which are
// abstractions to the size of a pointer on a given platform
// (ie, they're guaranteed to be the same size as a pointer)

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS
#ifdef HAVE_INTTYPES
#  include <stdint.h>
#  include <inttypes.h>

// Ghetto. Blame devkitPPC's faulty headers.
#ifdef GEKKO
#  undef PRIdPTR
#  define PRIdPTR "d"
#  undef PRIxPTR
#  define PRIxPTR "x"
#  undef SCNx32
#  define SCNx32 "x"
#endif

#elif defined(_MSC_VER)
#  include "msvc/inttypes.h" // from http://code.google.com/p/msinttypes/
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

// Define this to rewrite all 'B' versions to library functions. This
// is for platforms which give us a standard sort of C library so we
// link directly. Platforms like PalmOS which don't have a standard C
// library will need to wrap these functions with suitable emulations.
#define __compat_h_macrodef__

#ifdef EXTERNC
#  include <cstdarg>
#  ifdef __compat_h_macrodef__
#   include <cstdio>
#   include <cstring>
#   include <cstdlib>
#   include <ctime>
#  endif
#else
# include <stdarg.h>
# include <stddef.h>
#endif

#ifdef __compat_h_macrodef__
# ifndef EXTERNC
#  include <stdio.h>
#  include <string.h>
#  include <stdlib.h>
#  include <time.h>
# endif
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
#include <direct.h>

# define longlong(x) x##i64

#if _MSC_VER < 1800
# define inline __inline

static inline float nearbyintf(float x) 
{ 
    uint32_t w1, w2;
    __asm fnstcw w1
    w2 = w1 | 0x00000020;
    __asm
    {
        fldcw w2
        fld x
        frndint
        fclex
        fldcw w1
    }
}
#endif
#else
# define longlong(x) x##ll
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
# define EXTERN_INLINE
# define EXTERN_INLINE_HEADER extern
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
# define B_ENDIAN_C_INLINE 1

#elif defined(GEKKO) || defined(__ANDROID__)
# define B_LITTLE_ENDIAN 0
# define B_BIG_ENDIAN 1
# define B_ENDIAN_C_INLINE 1

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

#elif defined(__FreeBSD__) || defined(__NetBSD__)
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
#if !defined __x86_64__ && defined __GNUC__
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
# define B_ENDIAN_C_INLINE 1

#elif defined(__QNX__)
# if defined __LITTLEENDIAN__
#  define B_LITTLE_ENDIAN 1
#  define B_BIG_ENDIAN    0
# elif defined __BIGENDIAN__
#  define B_LITTLE_ENDIAN 0
#  define B_BIG_ENDIAN    1
# endif
# define B_ENDIAN_C_INLINE 1

#elif defined(__sun)
# if defined _LITTLE_ENDIAN
#  define B_LITTLE_ENDIAN 1
#  define B_BIG_ENDIAN    0
# elif defined _BIG_ENDIAN
#  define B_LITTLE_ENDIAN 0
#  define B_BIG_ENDIAN    1
# endif
# define B_ENDIAN_C_INLINE 1

#elif defined(_WIN32) || defined(SKYOS) || defined(__SYLLABLE__)
# define B_LITTLE_ENDIAN 1
# define B_BIG_ENDIAN    0
# define B_ENDIAN_C_INLINE 1
#endif

#if !defined(B_LITTLE_ENDIAN) || !defined(B_BIG_ENDIAN)
# error Unknown endianness
#endif

#ifdef EXTERNC

# ifndef SCREWED_UP_CPP
//   using namespace std;
# endif

extern "C" {
#endif

#if defined B_ENDIAN_X86_INLINE
# if defined(_MSC_VER)
	// inline asm using bswap/xchg
# elif defined(__GNUC__)
	// inline asm using bswap/xchg
# endif
#elif defined B_ENDIAN_C_INLINE
static inline uint16_t B_SWAP16(uint16_t s) { return (s>>8)|(s<<8); }
static inline uint32_t  B_SWAP32(uint32_t  l) { return ((l>>8)&0xff00)|((l&0xff00)<<8)|(l<<24)|(l>>24); }
static inline uint64_t B_SWAP64(uint64_t l) { return (l>>56)|((l>>40)&0xff00)|((l>>24)&0xff0000)|((l>>8)&0xff000000)|((l&255)<<56)|((l&0xff00)<<40)|((l&0xff0000)<<24)|((l&0xff000000)<<8); }
#endif

#if defined(USE_MSC_PRAGMAS)
static inline void ftol(float f, int32_t *a)
{
    _asm
    {
        mov eax, a
            fld f
            fistp dword ptr [eax]
    }
}

static inline void dtol(double d, int32_t *a)
{
    _asm
    {
        mov eax, a
            fld d
            fistp dword ptr [eax]
    }
}
#elif defined(USE_GCC_PRAGMAS)

static inline void ftol(float f, int32_t *a)
{
    __asm__ __volatile__(
#if 0 //(__GNUC__ >= 3)
        "flds %1; fistpl %0;"
#else
        "flds %1; fistpl (%0);"
#endif
        : "=r"(a) : "m"(f) : "memory","cc");
}

static inline void dtol(double d, int32_t *a)
{
    __asm__ __volatile__(
#if 0 //(__GNUC__ >= 3)
        "fldl %1; fistpl %0;"
#else
        "fldl %1; fistpl (%0);"
#endif
        : "=r"(a) : "m"(d) : "memory","cc");
}

#else
static inline void ftol(float f, int32_t *a)
{
    *a = (int32_t)f;
}

static inline void dtol(double d, int32_t *a)
{
    *a = (int32_t)d;
}
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

#ifdef __compat_h_macrodef__

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
# define min(a,b) ( ((a) < (b)) ? (a) : (b) )
#endif

#ifndef max
# define max(a,b) ( ((a) > (b)) ? (a) : (b) )
#endif

#if __GNUC__ >= 4
# define CLAMP_DECL static inline __attribute__((warn_unused_result))
#else
# define CLAMP_DECL static inline
#endif

// Clamp <in> to [<min>..<max>]. The case in <= min is handled first.
CLAMP_DECL int32_t clamp(int32_t in, int32_t min, int32_t max)
{
    return in <= min ? min : (in >= max ? max : in);
}

// Clamp <in> to [<min>..<max>]. The case in >= max is handled first.
CLAMP_DECL int32_t clamp2(int32_t in, int32_t min, int32_t max)
{
    return in >= max ? max : (in <= min ? min : in);
}

#define BMAX_PATH 256


struct Bdirent {
	uint16_t namlen;
	char *name;
	uint32_t mode;
	uint32_t size;
	uint32_t mtime;
};
typedef void BDIR;

BDIR*		Bopendir(const char *name);
struct Bdirent*	Breaddir(BDIR *dir);
int32_t		Bclosedir(BDIR *dir);

#ifdef _MSC_VER
typedef intptr_t ssize_t;
#endif

#ifdef __compat_h_macrodef__
  typedef FILE BFILE;
# define bsize_t size_t
# define bssize_t ssize_t
#else
  typedef void          BFILE;
  typedef uint32_t bsize_t;
  typedef int32_t bssize_t;
#endif

#if RAND_MAX == 32767
static inline uint16_t system_15bit_rand(void) { return (uint16_t)rand(); }
#else  // RAND_MAX > 32767, assumed to be of the form 2^k - 1
static inline uint16_t system_15bit_rand(void) { return ((uint16_t)rand())&0x7fff; }
#endif

#if defined(_MSC_VER)
// XXX: non-__compat_h_macrodef__ version?
#define strtoll _strtoi64
#endif

#ifdef __compat_h_macrodef__
# define Bassert assert
# define Brand rand
# define Balloca alloca
#  define Bmalloc malloc
#  define Bcalloc calloc
#  define Brealloc realloc
#  define Bfree free
#  define Bstrdup strdup
#  define Bmemalign memalign
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
# define Bstat stat
# define Bfstat fstat
# else
# define Bstat stat
# define Bfstat fstat
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
# else
# if defined(__QNX__)
#  define Bstrcasecmp stricmp
#  define Bstrncasecmp strnicmp
# else
#  define Bstrcasecmp strcasecmp
#  define Bstrncasecmp strncasecmp
# endif
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
# define Bgetcwd getcwd
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
int32_t Bprintf(const char *format, ...) ATTRIBUTE((format(printf,1,2)));
int32_t Bsprintf(char *str, const char *format, ...) ATTRIBUTE((format(printf,2,3)));
int32_t Bsnprintf(char *str, bsize_t size, const char *format, ...) ATTRIBUTE((format(printf,3,4)));
int32_t Bvsnprintf(char *str, bsize_t size, const char *format, va_list ap);
char *Bgetcwd(char *buf, bsize_t size);
char *Bgetenv(const char *name);
#endif

char *Bgethomedir(void);
char *Bgetsupportdir(int32_t global);
uint32_t Bgetsysmemsize(void);
int32_t Bcorrectfilename(char *filename, int32_t removefn);
int32_t Bcanonicalisefilename(char *filename, int32_t removefn);
char *Bgetsystemdrives(void);
int32_t Bfilelength(int32_t fd);
char *Bstrtoken(char *s, const char *delim, char **ptrptr, int32_t chop);
char *Bstrtolower(char *str);
int32_t Bwildmatch (const char *i, const char *j);

#if !defined(_WIN32)
char *Bstrlwr(char *);
char *Bstrupr(char *);
#endif

// Copy min(strlen(src)+1, n) characters into dst, always terminate with a NUL.
static inline char *Bstrncpyz(char *dst, const char *src, bsize_t n)
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

#ifdef EXTERNC
}
#endif

#define MAYBE_FCLOSE_AND_NULL(fileptr) do { \
    if (fileptr) { Bfclose(fileptr); fileptr=NULL; } \
} while (0)

#define NOWARN(print_func, fmt, ...) do { \
    print_func(fmt, ## __VA_ARGS__); \
} while (0)

#define NOWARN_RETURN(print_func, var, fmt, ...) do { \
    var = print_func(fmt, ## __VA_ARGS__); \
} while (0)

#ifdef __cplusplus
#ifndef FORCE_WARNINGS
#ifdef _MSC_VER
// TODO: add MSVC pragmas to disable equivalent warning, if necessary later
#else
#ifdef _WIN32
// MinGW's _Pragma is completely broken so our GCC NOWARN macro is useless there
 #pragma GCC diagnostic ignored "-Wformat"
#else
 #undef NOWARN
 #undef NOWARN_RETURN

 #define NOWARN(print_func, fmt, ...) do { _Pragma("GCC diagnostic ignored \"-Wformat\"") \
    print_func(fmt, ## __VA_ARGS__); \
    _Pragma("GCC diagnostic warning \"-Wformat\"") } while (0)

 #define NOWARN_RETURN(print_func, var, fmt, ...) do { _Pragma("GCC diagnostic ignored \"-Wformat\"") \
    var = print_func(fmt, ## __VA_ARGS__); \
    _Pragma("GCC diagnostic warning \"-Wformat\"") } while (0)
#endif // _WIN32
#endif // _MSC_VER
#endif // FORCE_WARNINGS
#endif // __cplusplus

#define OSD_Printf_nowarn(fmt, ...) NOWARN(OSD_Printf, fmt, ## __VA_ARGS__)
#define Bsprintf_nowarn(fmt, ...) NOWARN(Bsprintf, fmt, ## __VA_ARGS__)
#define Bsprintf_nowarn_return(x, fmt, ...) NOWARN_RETURN(Bsprintf, x, fmt, ## __VA_ARGS__)
#define initprintf_nowarn(fmt, ...) NOWARN(initprintf, fmt, ## __VA_ARGS__)
#define message_nowarn(fmt, ...) NOWARN(message, fmt, ## __VA_ARGS__)

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

#endif // __compat_h__

