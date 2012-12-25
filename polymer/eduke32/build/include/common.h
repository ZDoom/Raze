//
// Definitions of common non-engine data structures/functions
// (and declarations of data appearing in both)
// for EDuke32 and Mapster32
//

#ifndef EDUKE32_COMMON_H_
#define EDUKE32_COMMON_H_

#include "scriptfile.h"
#include "cache1d.h"

#ifdef EXTERNC
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


//// FUNCTIONS
void G_AddGroup(const char *buffer);
void G_AddPath(const char *buffer);

int32_t getatoken(scriptfile *sf, const tokenlist *tl, int32_t ntokens);

int32_t testkopen(const char *filename, char searchfirst);  // full-blown kopen4load
int32_t check_file_exist(const char *fn);  // findfrompath with pathsearchmode=1 / search in zips

void fnlist_clearnames(fnlist_t *fnl);
int32_t fnlist_getnames(fnlist_t *fnl, const char *dirname, const char *pattern,
                        int32_t dirflags, int32_t fileflags);

void G_LoadGroupsInDir(const char *dirname);
void G_DoAutoload(const char *dirname);

char *dup_filename(const char *fn);
int32_t maybe_append_ext(char *wbuf, int32_t wbufsiz, const char *fn, const char *ext);

int32_t ldist(const spritetype *s1, const spritetype *s2);
int32_t dist(const spritetype *s1, const spritetype *s2);

void COMMON_clearbackground(int32_t numcols, int32_t numrows);

// timer defs for profiling function chunks the simple way
#define EDUKE32_TMRDEF int32_t t[20], ti=0; const char *tmrstr=__func__; fprintf(stderr,"%s\n",tmrstr); t[ti++]=getticks();
#define EDUKE32_TMRTIC t[ti++]=getticks()
#define EDUKE32_TMRPRN do { int ii=0; fprintf(stderr,"%s: ",tmrstr); for (ii=1; ii<ti; ii++) fprintf(stderr,"%d ", t[ii]-t[ii-1]); fprintf(stderr,"\n"); } while (0)

#ifdef EXTERNC
}
#endif

#endif
