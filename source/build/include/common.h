//
// Definitions of common non-engine data structures/functions
// (and declarations of data appearing in both)
// for EDuke32 and Mapster32
//

#ifndef EDUKE32_COMMON_H_
#define EDUKE32_COMMON_H_

#include "cache1d.h"
#include "compat.h"
#include "pragmas.h"  // klabs
#include "scriptfile.h"


#ifdef __cplusplus
extern "C" {
#endif

//// TYPES
struct strllist
{
    struct strllist *next;
    char *str;
};

typedef struct
{
    const char *text;
    int32_t tokenid;
}
tokenlist;

typedef struct
{
    CACHE1D_FIND_REC *finddirs, *findfiles;
    int32_t numdirs, numfiles;
}
fnlist_t;

#define FNLIST_INITIALIZER { NULL, NULL, 0, 0 }

enum
{
    T_EOF = -2,
    T_ERROR = -1,
};


//// EXTERN DECLS
extern struct strllist *CommandPaths, *CommandGrps;

#ifdef __cplusplus
extern "C" {
#endif
extern const char *s_buildRev;
extern const char *s_buildTimestamp;
#ifdef __cplusplus
}
#endif

//// FUNCTIONS
extern void PrintBuildInfo(void);

extern void clearDefNamePtr(void);

void G_AddGroup(const char *buffer);
void G_AddPath(const char *buffer);
void G_AddDef(const char *buffer);
void G_AddDefModule(const char *buffer);
#ifdef HAVE_CLIPSHAPE_FEATURE
void G_AddClipMap(const char *buffer);
#endif

// returns a buffer of size BMAX_PATH
static inline char *dup_filename(const char *fn)
{
    char * const buf = (char *) Xmalloc(BMAX_PATH);
    return Bstrncpyz(buf, fn, BMAX_PATH);
}

static inline void realloc_copy(char **fn, const char *buf)
{
    uint8_t len = Bstrlen(buf) + 1;
    *fn = (char *)Xrealloc(*fn, len);
    Bstrncpy(*fn, buf, len);
}

int32_t getatoken(scriptfile *sf, const tokenlist *tl, int32_t ntokens);

int32_t G_CheckCmdSwitch(int32_t argc, char const * const * argv, const char *str);

int32_t testkopen(const char *filename, char searchfirst);  // full-blown kopen4load
int32_t check_file_exist(const char *fn);  // findfrompath with pathsearchmode=1 / search in zips

void fnlist_clearnames(fnlist_t *fnl);
int32_t fnlist_getnames(fnlist_t *fnl, const char *dirname, const char *pattern,
                        int32_t dirflags, int32_t fileflags);


int32_t maybe_append_ext(char *wbuf, int32_t wbufsiz, const char *fn, const char *ext);

// Approximations to 2D and 3D Euclidean distances. Initial EDuke32 SVN import says
// in mact/src/mathutil.c: "Ken's reverse-engineering job".
// Note that mathutil.c contains practically the same code, but where the
// individual x/y(/z) distances are passed instead.
static inline int32_t sepldist(const int32_t dx, const int32_t dy)
{
    vec2_t d = { klabs(dx), klabs(dy) };

    if (d.x < d.y)
        swaplong(&d.x, &d.y);

    d.y += (d.y>>1);

    return d.x - (d.x>>5) - (d.x>>7) + (d.y>>2) + (d.y>>6);
}

// dz: in Build coordinates
static inline int32_t sepdist(const int32_t dx, const int32_t dy, const int32_t dz)
{
    vec3_t d = { klabs(dx), klabs(dy), klabs(dz>>4) };

    if (d.x < d.y)
        swaplong(&d.x, &d.y);

    if (d.x < d.z)
        swaplong(&d.x, &d.z);

    d.y += d.z;

    return d.x - (d.x>>4) + (d.y>>2) + (d.y>>3);
}

int32_t FindDistance2D(int32_t dx, int32_t dy);
int32_t FindDistance3D(int32_t dx, int32_t dy, int32_t dz);
int32_t ldist(const void *s1, const void *s2);
int32_t dist(const void *s1, const void *s2);

void COMMON_clearbackground(int32_t numcols, int32_t numrows);

// timer defs for profiling function chunks the simple way
#define EDUKE32_TMRDEF int32_t t[20], ti=0; const char *tmrstr=__func__; fprintf(stderr,"%s\n",tmrstr); t[ti++]=timerGetTicks();
#define EDUKE32_TMRTIC t[ti++]=timerGetTicks()
#define EDUKE32_TMRPRN do { int ii=0; fprintf(stderr,"%s: ",tmrstr); for (ii=1; ii<ti; ii++) fprintf(stderr,"%d ", t[ii]-t[ii-1]); fprintf(stderr,"\n"); } while (0)

void Duke_CommonCleanup(void);

#ifdef __cplusplus
}
#endif

#endif
