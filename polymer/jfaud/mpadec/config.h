#ifdef __POWERPC__
# define ARCH_PPC
#else
# if defined __x86_64__ || defined __amd64__
#  define ARCH_AMD64
# else
#  define ARCH_X86
# endif
#endif

/* Hardware architecture */
//#define ARCH_ALPHA
//#define ARCH_PPC
//#define ARCH_SPARC
//#define ARCH_X86
//#define ARCH_AMD64
//#define ARCH_IA64

#if defined _WIN32 && !defined __MINGW32__
#define HAVE_IO_H
#define HAVE_CONIO_H
#define _INTPTR_T_DEFINED
#else
#define HAVE_INTTYPES_H
#define HAVE_UNISTD_H
#endif
#undef OSS

#define FLOAT double

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#else
typedef signed char      int8_t;
typedef unsigned char    uint8_t;
typedef signed short     int16_t;
typedef unsigned short   uint16_t;
typedef signed int       int32_t;
typedef unsigned int     uint32_t;
#if defined(__BORLANDC__) || defined(_MSC_VER) || defined(__WATCOMC__)
typedef signed __int64   int64_t;
typedef unsigned __int64 uint64_t;
#elif defined(__GNUC__)
typedef signed long long   int64_t;
typedef unsigned long long uint64_t;
#endif
#if defined(ARCH_AMD64) || defined(ARCH_IA64) || defined(ARCH_ALPHA)
typedef int64_t  intptr_t;
typedef uint64_t uintptr_t;
#else
//typedef signed long      int32_t;
//typedef unsigned long    uint32_t;
typedef int32_t  intptr_t;
typedef uint32_t uintptr_t;
#endif
#endif

#undef PACKED
#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

#ifdef _WIN32
# if defined __GNUC__
#  define MPADECAPI __attribute__((stdcall))
# elif defined _MSC_VER
#  define MPADECAPI _stdcall
# elif defined __WATCOMC__
#  define MPADECAPI __stdcall
# endif
#else
# define MPADECAPI
#endif

#include <string.h>
#include <memory.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#undef FALSE
#undef TRUE
#define FALSE 0
#define TRUE  1

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef _WIN32
#define strcasecmp stricmp
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

