/*
 * Playing-field leveler for Build
 */

#define LIBDIVIDE_BODY
#include "compat.h"
#include "debugbreak.h"

#ifdef _WIN32
# define NEED_SHLOBJ_H
# include "windows_inc.h"
#elif __APPLE__
# include "osxbits.h"
#endif

#ifndef USE_PHYSFS
#if defined(_MSC_VER)
# include <io.h>
#else
# include <dirent.h>
#endif
#endif

#if defined __linux || defined EDUKE32_BSD
# include <libgen.h> // for dirname()
#endif
#if defined EDUKE32_BSD
# include <limits.h> // for PATH_MAX
# include <sys/sysctl.h> // for sysctl() to get path to executable
#endif

#include "baselayer.h"

////////// PANICKING ALLOCATION FUNCTIONS //////////

static void (*g_MemErrHandler)(int32_t line, const char *file, const char *func);

#ifdef DEBUGGINGAIDS
static const char *g_MemErrFunc = "???";
static const char *g_MemErrFile = "???";
static int32_t g_MemErrLine;

void xalloc_set_location(int32_t line, const char *file, const char *func)
{
    g_MemErrLine = line;
    g_MemErrFile = file;

    if (func)
        g_MemErrFunc = func;
}
#endif

void *handle_memerr(void *p)
{
    UNREFERENCED_PARAMETER(p);
    debug_break();

    if (g_MemErrHandler)
    {
#ifdef DEBUGGINGAIDS
        g_MemErrHandler(g_MemErrLine, g_MemErrFile, g_MemErrFunc);
#else
        g_MemErrHandler(0, "???", "???");
#endif
    }

    Bexit(EXIT_FAILURE);
    EDUKE32_UNREACHABLE_SECTION(return &handle_memerr);
}

void set_memerr_handler(void(*handlerfunc)(int32_t, const char *, const char *))
{
    g_MemErrHandler = handlerfunc;
}

//
// Stuff which must be a function
//
char *Bgethomedir(void)
{
#ifdef _WIN32

    char appdata[MAX_PATH];

    if (SUCCEEDED(SHGetSpecialFolderPathA(NULL, appdata, CSIDL_APPDATA, FALSE)))
    {
        return Xstrdup(appdata);
    }
    return NULL;
#elif defined EDUKE32_OSX
    return osx_gethomedir();
#else
    char *e = getenv("HOME");
    if (!e) return NULL;
    return Xstrdup(e);
#endif
}


int32_t Bcorrectfilename(char *filename, int32_t removefn)
{
    char *fn = Xstrdup(filename);
    char *tokarr[64], *first, *next = NULL;

    for (first=fn; *first; first++)
    {
#ifdef _WIN32
        if (*first == '\\') *first = '/';
#endif
    }

    int leadslash = (*fn == '/');
    int trailslash = (first>fn && first[-1] == '/');
    int ntok = 0;

    first = fn;
    do
    {
        char *token = Bstrtoken(first, "/", &next, 1);
        first = NULL;
        if (!token) break;
        else if (token[0] == 0) continue;
        else if (token[0] == '.' && token[1] == 0) continue;
        else if (token[0] == '.' && token[1] == '.' && token[2] == 0) ntok = max(0,ntok-1);
        else tokarr[ntok++] = token;
    }
    while (1);

    if (!trailslash && removefn) { ntok = max(0,ntok-1); trailslash = 1; }
    if (ntok == 0 && trailslash && leadslash) trailslash = 0;

    first = filename;
    if (leadslash) *(first++) = '/';
    for (int i=0; i<ntok; i++)
    {
        if (i>0) *(first++) = '/';
        for (char *token=tokarr[i]; *token; token++)
            *(first++) = *token;
    }
    if (trailslash) *(first++) = '/';
    *(first++) = 0;

    Xfree(fn);
    return 0;
}


char *Bstrtoken(char *s, const char *delim, char **ptrptr, int chop)
{
    if (!ptrptr)
        return NULL;

    char *p = s ? s : *ptrptr;

    if (!p)
        return NULL;

    while (*p != 0 && Bstrchr(delim, *p)) p++;

    if (*p == 0)
    {
        *ptrptr = NULL;
        return NULL;
    }

    char * const start = p;

    while (*p != 0 && !Bstrchr(delim, *p)) p++;

    if (*p == 0)
        *ptrptr = NULL;
    else
    {
        if (chop)
            *(p++) = 0;
        *ptrptr = p;
    }

    return start;
}

char *Bstrtolower(char *str)
{
    if (!str)
        return NULL;

    int len = Bstrlen(str);

    if (len <= 0)
        return str;

    int i = 0;

    do
    {
        *(str + i) = Btolower(*(str + i));
        i++;
    } while (--len);

    return str;
}

#if !defined(_WIN32)
char *Bstrlwr(char *s)
{
    if (!s) return s;
    char *t = s;
    while (*t) { *t = Btolower(*t); t++; }
    return s;
}

char *Bstrupr(char *s)
{
    if (!s) return s;
    char *t = s;
    while (*t) { *t = Btoupper(*t); t++; }
    return s;
}
#endif

