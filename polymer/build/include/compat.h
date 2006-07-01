// Compatibility declarations for things which might not be present in
// certain build environments. It also levels the playing field caused
// by different platforms.

#ifndef __compat_h__
#define __compat_h__

// Define this to rewrite all 'B' versions to library functions. This
// is for platforms which give us a standard sort of C library so we
// link directly. Platforms like PalmOS which don't have a standard C
// library will need to wrap these functions with suitable emulations.
#define __compat_h_macrodef__

#ifdef __cplusplus
# include <cstdarg>
#else
# include <stdarg.h>
#endif

#ifdef __compat_h_macrodef__
# ifdef __cplusplus
#  include <cstdio>
#  include <cstring>
#  include <cstdlib>
#  include <ctime>
# else
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

#ifdef EFENCE
# include <efence.h>
#endif

#if defined(__WATCOMC__)
# define inline __inline
# define int64 __int64
# define uint64 unsigned __int64
# define longlong(x) x##i64
#elif defined(_MSC_VER)
# define inline __inline
# define int64 __int64
# define uint64 unsigned __int64
# define longlong(x) x##i64
#else
# define longlong(x) x##ll
typedef long long int64;
typedef unsigned long long uint64;
#endif

#ifndef NULL
# define NULL ((void *)0)
#endif

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

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
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

#ifdef __cplusplus
extern "C" {
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
static inline unsigned short B_SWAP16(unsigned short s) { return (s>>8)|(s<<8); }
static inline unsigned long  B_SWAP32(unsigned long  l) { return ((l>>8)&0xff00)|((l&0xff00)<<8)|(l<<24)|(l>>24); }
static inline uint64 B_SWAP64(uint64 l) { return (l>>56)|((l>>40)&0xff00)|((l>>24)&0xff0000)|((l>>8)&0xff000000)|((l&255)<<56)|((l&0xff00)<<40)|((l&0xff0000)<<24)|((l&0xff000000)<<8); }
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
# define FP_OFF(__p) ((unsigned)(__p))
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
# define BS_IWRITE S_IWRITE
# define BS_IREAD  S_IREAD
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


#define BMAX_PATH 260


struct Bdirent {
	unsigned short namlen;
	char *name;
	unsigned mode;
	unsigned size;
	unsigned mtime;
};
typedef void BDIR;

BDIR*		Bopendir(const char *name);
struct Bdirent*	Breaddir(BDIR *dir);
int		Bclosedir(BDIR *dir);


#ifdef __compat_h_macrodef__
# define BFILE FILE
# define bsize_t size_t
# define bssize_t ssize_t
#else
typedef void          BFILE;
typedef unsigned long bsize_t;
typedef signed   long bssize_t;
#endif


#ifdef __compat_h_macrodef__
# define Brand rand
# define Balloca alloca
# define Bmalloc malloc
# define Bcalloc calloc
# define Brealloc realloc
# define Bfree free
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
# define Bstat stat
# define Bfopen fopen
# define Bfclose fclose
# define Bfeof feof
# define Bfgetc fgetc
# define Brewind rewind
# define Bfgets fgets
# define Bfputc fputc
# define Bfputs fputs
# define Bfread fread
# define Bfwrite fwrite
# define Bfprintf fprintf
# define Bstrdup strdup
# define Bstrcpy strcpy
# define Bstrncpy strncpy
# define Bstrcmp strcmp
# define Bstrncmp strncmp
# if defined(__WATCOMC__) || defined(_MSC_VER) || defined(__QNX__)
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
# define Batoi atoi
# define Batol atol
# define Bstrtol strtol
# define Bstrtoul strtoul
# define Bstrtod strtod
# define Btoupper toupper
# define Btolower tolower
# define Bmemcpy memcpy
# define Bmemmove memmove
# define Bmemchr memchr
# define Bmemset memset
# define Bmemcmp memcmp
# define Bprintf printf
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

#else

int Brand(void);
void *Bmalloc(bsize_t size);
void Bfree(void *ptr);
int Bopen(const char *pathname, int flags, unsigned mode);
int Bclose(int fd);
bssize_t Bwrite(int fd, const void *buf, bsize_t count);
bssize_t Bread(int fd, void *buf, bsize_t count);
int Blseek(int fildes, int offset, int whence);
BFILE *Bfopen(const char *path, const char *mode);
int Bfclose(BFILE *stream);
int Bfeof(BFILE *stream);
int Bfgetc(BFILE *stream);
void Brewind(BFILE *stream);
char *Bfgets(char *s, int size, BFILE *stream);
int Bfputc(int c, BFILE *stream);
int Bfputs(const char *s, BFILE *stream);
bsize_t Bfread(void *ptr, bsize_t size, bsize_t nmemb, BFILE *stream);
bsize_t Bfwrite(const void *ptr, bsize_t size, bsize_t nmemb, BFILE *stream);
char *Bstrdup(const char *s);
char *Bstrcpy(char *dest, const char *src);
char *Bstrncpy(char *dest, const char *src, bsize_t n);
int Bstrcmp(const char *s1, const char *s2);
int Bstrncmp(const char *s1, const char *s2, bsize_t n);
int Bstrcasecmp(const char *s1, const char *s2);
int Bstrncasecmp(const char *s1, const char *s2, bsize_t n);
char *Bstrcat(char *dest, const char *src);
char *Bstrncat(char *dest, const char *src, bsize_t n);
bsize_t Bstrlen(const char *s);
char *Bstrchr(const char *s, int c);
char *Bstrrchr(const char *s, int c);
int Batoi(const char *nptr);
long Batol(const char *nptr);
long int Bstrtol(const char *nptr, char **endptr, int base);
unsigned long int Bstrtoul(const char *nptr, char **endptr, int base);
void *Bmemcpy(void *dest, const void *src, bsize_t n);
void *Bmemmove(void *dest, const void *src, bsize_t n);
void *Bmemchr(const void *s, int c, bsize_t n);
void *Bmemset(void *s, int c, bsize_t n);
int Bmemcmp(const void *s1, const void *s2, bsize_t n);
int Bprintf(const char *format, ...);
int Bsprintf(char *str, const char *format, ...);
int Bsnprintf(char *str, bsize_t size, const char *format, ...);
int Bvsnprintf(char *str, bsize_t size, const char *format, va_list ap);
char *Bgetcwd(char *buf, bsize_t size);
char *Bgetenv(const char *name);
#endif

char *Bgethomedir(void);
char *Bgetsupportdir(int global);
unsigned int Bgetsysmemsize(void);
int Bcorrectfilename(char *filename, int removefn);
int Bcanonicalisefilename(char *filename, int removefn);
char *Bgetsystemdrives(void);
long Bfilelength(int fd);
char *Bstrtoken(char *s, char *delim, char **ptrptr, int chop);
long Bwildmatch (const char *i, const char *j);

#if !defined(_WIN32)
char *Bstrlwr(char *);
char *Bstrupr(char *);
#endif

#ifdef __cplusplus
}
#endif

#endif // __compat_h__

