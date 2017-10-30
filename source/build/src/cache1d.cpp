// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)

#include "compat.h"

#ifdef CACHE1D_COMPRESS_ONLY
// Standalone libcache1d.so containing only the compression/decompression
// functions.
# define C1D_STATIC
#else
// cache1d.o for EDuke32
# define C1D_STATIC static

#ifdef _WIN32
// for FILENAME_CASE_CHECK
# define NEED_SHELLAPI_H
# include "windows_inc.h"
#endif
#include "cache1d.h"
#include "pragmas.h"
#include "baselayer.h"

#ifdef WITHKPLIB
#include "kplib.h"

//Insert '|' in front of filename
//Doing this tells kzopen to load the file only if inside a .ZIP file
static intptr_t kzipopen(const char *filnam)
{
    uint32_t i;
    char newst[BMAX_PATH+8];

    newst[0] = '|';
    for (i=0; filnam[i] && (i < sizeof(newst)-2); i++) newst[i+1] = filnam[i];
    newst[i+1] = 0;
    return kzopen(newst);
}

#endif

char *kpzbuf = NULL;
int32_t kpzbufsiz;

int32_t kpzbufloadfil(int32_t const handle)
{
    int32_t const leng = kfilelength(handle);
    if (leng > kpzbufsiz)
    {
        kpzbuf = (char *) Xrealloc(kpzbuf, leng+1);
        kpzbufsiz = leng;
        if (!kpzbuf)
            return 0;
    }

    kpzbuf[leng] = 0;  // FIXME: buf[leng] read in kpegrend(), see BUF_LENG_READ
    kread(handle, kpzbuf, leng);

    return leng;
}

int32_t kpzbufload(char const * const filnam)
{
    int32_t const handle = kopen4load(filnam, 0);
    if (handle < 0)
        return 0;

    int32_t const leng = kpzbufloadfil(handle);

    kclose(handle);

    return leng;
}

//   This module keeps track of a standard linear cacheing system.
//   To use this module, here's all you need to do:
//
//   Step 1: Allocate a nice BIG buffer, like from 1MB-4MB and
//           Call initcache(int32_t cachestart, int32_t cachesize) where
//
//              cachestart = (intptr_t)(pointer to start of BIG buffer)
//              cachesize = length of BIG buffer
//
//   Step 2: Call allocache(intptr_t *bufptr, int32_t bufsiz, char *lockptr)
//              whenever you need to allocate a buffer, where:
//
//              *bufptr = pointer to multi-byte pointer to buffer
//                 Confused?  Using this method, cache2d can remove
//                 previously allocated things from the cache safely by
//                 setting the multi-byte pointer to 0.
//              bufsiz = number of bytes to allocate
//              *lockptr = pointer to locking char which tells whether
//                 the region can be removed or not.  If *lockptr = 0 then
//                 the region is not locked else its locked.
//
//   Step 3: If you need to remove everything from the cache, or every
//           unlocked item from the cache, you can call uninitcache();
//              Call uninitcache(0) to remove all unlocked items, or
//              Call uninitcache(1) to remove everything.
//           After calling uninitcache, it is still ok to call allocache
//           without first calling initcache.

#define MAXCACHEOBJECTS 9216

#if !defined DEBUG_ALLOCACHE_AS_MALLOC
static int32_t cachesize = 0;
static char zerochar = 0;
static intptr_t cachestart = 0;
static int32_t agecount = 0;
static int32_t lockrecip[200];

int32_t cacnum = 0;
cactype cac[MAXCACHEOBJECTS];
#endif

char toupperlookup[256] =
{
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
    0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x7f,
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
    0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
    0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
    0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
    0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
    0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
    0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};

static void reportandexit(const char *errormessage);


void initcache(intptr_t dacachestart, int32_t dacachesize)
{
#ifndef DEBUG_ALLOCACHE_AS_MALLOC
    int32_t i;

    for (i=1; i<200; i++)
        lockrecip[i] = tabledivide32_noinline(1<<28, 200-i);

    // The following code was relocated here from engine.c, since this
    // function is only ever called once (from there), and it seems to
    // really belong here:
    //
    //   initcache((FP_OFF(pic)+15)&0xfffffff0,(cachesize-((-FP_OFF(pic))&15))&0xfffffff0);
    //
    // I'm not sure why it's necessary, but the code is making sure the
    // cache starts on a multiple of 16 bytes?  -- SA

//printf("BEFORE: cachestart = %x, cachesize = %d\n", dacachestart, dacachesize);
    cachestart = ((uintptr_t)dacachestart+15)&~(uintptr_t)0xf;
    cachesize = (dacachesize-(((uintptr_t)(dacachestart))&0xf))&~(uintptr_t)0xf;
//printf("AFTER : cachestart = %x, cachesize = %d\n", cachestart, cachesize);

    cac[0].leng = cachesize;
    cac[0].lock = &zerochar;
    cacnum = 1;

    initprintf("Initialized %.1fM cache\n", (float)(dacachesize/1024.f/1024.f));
#else
    UNREFERENCED_PARAMETER(dacachestart);
    UNREFERENCED_PARAMETER(dacachesize);
#endif
}

#ifdef DEBUG_ALLOCACHE_AS_MALLOC
void allocache(intptr_t *newhandle, int32_t newbytes, char *newlockptr)
{
    UNREFERENCED_PARAMETER(newlockptr);

    *newhandle = (intptr_t)Xmalloc(newbytes);
}
#else
static inline void inc_and_check_cacnum(void)
{
    if (++cacnum > MAXCACHEOBJECTS)
        reportandexit("Too many objects in cache! (cacnum > MAXCACHEOBJECTS)");
}

void allocache(intptr_t *newhandle, int32_t newbytes, char *newlockptr)
{
    int32_t i, z, bestz=0, bestval, besto=0, o1, sucklen, suckz;

//printf("  ==> asking for %d bytes, ", newbytes);
    // Make all requests a multiple of 16 bytes
    newbytes = (newbytes+15)&0xfffffff0;
//printf("allocated %d bytes\n", newbytes);

    if ((unsigned)newbytes > (unsigned)cachesize)
    {
        Bprintf("Cachesize: %d\n",cachesize);
        Bprintf("*Newhandle: 0x%" PRIxPTR ", Newbytes: %d, *Newlock: %d\n",(intptr_t)newhandle,newbytes,*newlockptr);
        reportandexit("BUFFER TOO BIG TO FIT IN CACHE!");
    }

    if (*newlockptr == 0)
    {
        reportandexit("ALLOCACHE CALLED WITH LOCK OF 0!");
    }

    //Find best place
    bestval = 0x7fffffff; o1 = cachesize;
    for (z=cacnum-1; z>=0; z--)
    {
        int32_t zz, o2, daval;

        o1 -= cac[z].leng;
        o2 = o1+newbytes;

        if (o2 > cachesize)
            continue;

        daval = 0;
        for (i=o1,zz=z; i<o2; i+=cac[zz++].leng)
        {
            if (*cac[zz].lock == 0)
                continue;

            if (*cac[zz].lock >= 200)
            {
                daval = 0x7fffffff;
                break;
            }

            // Potential for eviction increases with
            //  - smaller item size
            //  - smaller lock byte value (but in [1 .. 199])
            daval += mulscale32(cac[zz].leng+65536, lockrecip[*cac[zz].lock]);
            if (daval >= bestval)
                break;
        }

        if (daval < bestval)
        {
            bestval = daval; besto = o1; bestz = z;
            if (bestval == 0) break;
        }
    }

    //printf("%d %d %d\n",besto,newbytes,*newlockptr);

    if (bestval == 0x7fffffff)
        reportandexit("CACHE SPACE ALL LOCKED UP!");

    //Suck things out
    for (sucklen=-newbytes,suckz=bestz; sucklen<0; sucklen+=cac[suckz++].leng)
        if (*cac[suckz].lock)
            *cac[suckz].hand = 0;

    //Remove all blocks except 1
    suckz -= bestz+1;
    cacnum -= suckz;

    Bmemmove(&cac[bestz], &cac[bestz+suckz], (cacnum-bestz)*sizeof(cactype));
    cac[bestz].hand = newhandle;
    *newhandle = cachestart + besto;
    cac[bestz].leng = newbytes;
    cac[bestz].lock = newlockptr;

    //Add new empty block if necessary
    if (sucklen <= 0)
        return;

    bestz++;
    if (bestz == cacnum)
    {
        inc_and_check_cacnum();

        cac[bestz].leng = sucklen;
        cac[bestz].lock = &zerochar;
        return;
    }

    if (*cac[bestz].lock == 0)
    {
        cac[bestz].leng += sucklen;
        return;
    }

    inc_and_check_cacnum();

    for (z=cacnum-1; z>bestz; z--)
        cac[z] = cac[z-1];
    cac[bestz].leng = sucklen;
    cac[bestz].lock = &zerochar;
}
#endif

void agecache(void)
{
#ifndef DEBUG_ALLOCACHE_AS_MALLOC
    bssize_t cnt = (cacnum>>4);

    if (agecount >= cacnum)
        agecount = cacnum-1;

    if (agecount < 0 || !cnt)
        return;

    for (; cnt>=0; cnt--)
    {
        // If we have pointer to lock char and it's in [2 .. 199], decrease.
        if (cac[agecount].lock && (((*cac[agecount].lock)-2)&255) < 198)
            (*cac[agecount].lock)--;

        agecount--;
        if (agecount < 0)
            agecount = cacnum-1;
    }
#endif
}

static void reportandexit(const char *errormessage)
{
#ifndef DEBUG_ALLOCACHE_AS_MALLOC
    int32_t i, j;

    //setvmode(0x3);
    j = 0;
    for (i=0; i<cacnum; i++)
    {
        Bprintf("%d- ",i);
        if (cac[i].hand) Bprintf("ptr: 0x%" PRIxPTR ", ",*cac[i].hand);
        else Bprintf("ptr: NULL, ");
        Bprintf("leng: %d, ",cac[i].leng);
        if (cac[i].lock) Bprintf("lock: %d\n",*cac[i].lock);
        else Bprintf("lock: NULL\n");
        j += cac[i].leng;
    }
    Bprintf("Cachesize = %d\n",cachesize);
    Bprintf("Cacnum = %d\n",cacnum);
    Bprintf("Cache length sum = %d\n",j);
#endif
    initprintf("ERROR: %s\n",errormessage);
    Bexit(1);
}

#include <errno.h>

typedef struct _searchpath
{
    struct _searchpath *next;
    char *path;
    size_t pathlen;		// to save repeated calls to strlen()
    int32_t user;
} searchpath_t;
static searchpath_t *searchpathhead = NULL;
static size_t maxsearchpathlen = 0;
int32_t pathsearchmode = 0;

char *listsearchpath(int32_t initp)
{
    static searchpath_t *sp;

    if (initp)
        sp = searchpathhead;
    else if (sp != NULL)
        sp = sp->next;

    return sp ? sp->path : NULL;
}

int32_t addsearchpath_user(const char *p, int32_t user)
{
    struct Bstat st;
    char *s;
    searchpath_t *srch;
    char *path = Xstrdup(p);

    if (path[Bstrlen(path)-1] == '\\')
        path[Bstrlen(path)-1] = 0; // hack for stat() returning ENOENT on paths ending in a backslash

    if (Bstat(path, &st) < 0)
    {
        Bfree(path);
        if (errno == ENOENT) return -2;
        return -1;
    }
    if (!(st.st_mode & BS_IFDIR))
    {
        Bfree(path);
        return -1;
    }

    srch = (searchpath_t *)Xmalloc(sizeof(searchpath_t));

    srch->next    = searchpathhead;
    srch->pathlen = Bstrlen(path)+1;
    srch->path    = (char *)Xmalloc(srch->pathlen + 1);

    Bstrcpy(srch->path, path);
    for (s=srch->path; *s; s++) { }
    s--;

    if (s<srch->path || toupperlookup[*s] != '/')
        Bstrcat(srch->path, "/");

    searchpathhead = srch;
    if (srch->pathlen > maxsearchpathlen)
        maxsearchpathlen = srch->pathlen;

    Bcorrectfilename(srch->path,0);

    srch->user = user;

    initprintf("Using %s for game data\n", srch->path);

    Bfree(path);
    return 0;
}

int32_t removesearchpath(const char *p)
{
    searchpath_t *srch;
    char *s;
    char *path = (char *)Xmalloc(Bstrlen(p) + 2);

    Bstrcpy(path, p);

    if (path[Bstrlen(path)-1] == '\\')
        path[Bstrlen(path)-1] = 0;

    for (s=path; *s; s++) { }
    s--;

    if (s<path || toupperlookup[*s] != '/')
        Bstrcat(path, "/");

    Bcorrectfilename(path,0);

    for (srch = searchpathhead; srch; srch = srch->next)
    {
        if (!Bstrncmp(path, srch->path, srch->pathlen))
        {
//            initprintf("Removing %s from path stack\n", path);

            if (srch == searchpathhead)
                searchpathhead = srch->next;
            else
            {
                searchpath_t *sp;

                for (sp = searchpathhead; sp; sp = sp->next)
                {
                    if (sp->next == srch)
                    {
//                        initprintf("matched %s\n", srch->path);
                        sp->next = srch->next;
                        break;
                    }
                }
            }

            Bfree(srch->path);
            Bfree(srch);
            break;
        }
    }

    Bfree(path);
    return 0;
}

void removesearchpaths_withuser(int32_t usermask)
{
    searchpath_t *next;

    for (searchpath_t *srch = searchpathhead; srch; srch = next)
    {
        next = srch->next;

        if (srch->user & usermask)
        {

            if (srch == searchpathhead)
                searchpathhead = srch->next;
            else
            {
                searchpath_t *sp;

                for (sp = searchpathhead; sp; sp = sp->next)
                {
                    if (sp->next == srch)
                    {
                        sp->next = srch->next;
                        break;
                    }
                }
            }

            Bfree(srch->path);
            Bfree(srch);
        }
    }
}

int32_t findfrompath(const char *fn, char **where)
{
    // pathsearchmode == 0: tests current dir and then the dirs of the path stack
    // pathsearchmode == 1: tests fn without modification, then like for pathsearchmode == 0

    if (pathsearchmode)
    {
        // test unmolested filename first
        if (access(fn, F_OK) >= 0)
        {
            *where = Xstrdup(fn);
            return 0;
        }
#ifndef _WIN32
        else
        {
            char *tfn = Bstrtolower(Xstrdup(fn));

            if (access(tfn, F_OK) >= 0)
            {
                *where = tfn;
                return 0;
            }

            Bstrupr(tfn);

            if (access(tfn, F_OK) >= 0)
            {
                *where = tfn;
                return 0;
            }

            Bfree(tfn);
        }
#endif
    }

    char const *cpfn;

    for (cpfn = fn; toupperlookup[*cpfn] == '/'; cpfn++) { }
    char *ffn = Xstrdup(cpfn);

    Bcorrectfilename(ffn,0);	// compress relative paths

    int32_t allocsiz = max(maxsearchpathlen, 2);	// "./" (aka. curdir)
    allocsiz += strlen(ffn);
    allocsiz += 1;	// a nul

    char *pfn = (char *)Xmalloc(allocsiz);

    strcpy(pfn, "./");
    strcat(pfn, ffn);
    if (access(pfn, F_OK) >= 0)
    {
        *where = pfn;
        Bfree(ffn);
        return 0;
    }

    for (searchpath_t *sp = searchpathhead; sp; sp = sp->next)
    {
        char *tfn = Xstrdup(ffn);

        strcpy(pfn, sp->path);
        strcat(pfn, ffn);
        //initprintf("Trying %s\n", pfn);
        if (access(pfn, F_OK) >= 0)
        {
            *where = pfn;
            Bfree(ffn);
            Bfree(tfn);
            return 0;
        }

#ifndef _WIN32
        //Check with all lowercase
        strcpy(pfn, sp->path);
        Bstrtolower(tfn);
        strcat(pfn, tfn);
        if (access(pfn, F_OK) >= 0)
        {
            *where = pfn;
            Bfree(ffn);
            Bfree(tfn);
            return 0;
        }

        //Check again with uppercase
        strcpy(pfn, sp->path);
        Bstrupr(tfn);
        strcat(pfn, tfn);
        if (access(pfn, F_OK) >= 0)
        {
            *where = pfn;
            Bfree(ffn);
            Bfree(tfn);
            return 0;
        }
#endif
        Bfree(tfn);
    }

    Bfree(pfn); Bfree(ffn);
    return -1;
}

#if defined(_WIN32) && defined(DEBUGGINGAIDS)
# define FILENAME_CASE_CHECK
#endif

static int32_t openfrompath_internal(const char *fn, char **where, int32_t flags, int32_t mode)
{
    if (findfrompath(fn, where) < 0)
        return -1;

    return Bopen(*where, flags, mode);
}

int32_t openfrompath(const char *fn, int32_t flags, int32_t mode)
{
    char *pfn = NULL;

    int32_t h = openfrompath_internal(fn, &pfn, flags, mode);

    Bfree(pfn);

    return h;
}

BFILE *fopenfrompath(const char *fn, const char *mode)
{
    int32_t fh;
    BFILE *h;
    int32_t bmode = 0, smode = 0;
    const char *c;

    for (c=mode; c[0];)
    {
        if (c[0] == 'r' && c[1] == '+') { bmode = BO_RDWR; smode = BS_IREAD|BS_IWRITE; c+=2; }
        else if (c[0] == 'r')                { bmode = BO_RDONLY; smode = BS_IREAD; c+=1; }
        else if (c[0] == 'w' && c[1] == '+') { bmode = BO_RDWR|BO_CREAT|BO_TRUNC; smode = BS_IREAD|BS_IWRITE; c+=2; }
        else if (c[0] == 'w')                { bmode = BO_WRONLY|BO_CREAT|BO_TRUNC; smode = BS_IREAD|BS_IWRITE; c+=2; }
        else if (c[0] == 'a' && c[1] == '+') { bmode = BO_RDWR|BO_CREAT; smode=BS_IREAD|BS_IWRITE; c+=2; }
        else if (c[0] == 'a')                { bmode = BO_WRONLY|BO_CREAT; smode=BS_IREAD|BS_IWRITE; c+=1; }
        else if (c[0] == 'b')                { bmode |= BO_BINARY; c+=1; }
        else if (c[1] == 't')                { bmode |= BO_TEXT; c+=1; }
        else c++;
    }
    fh = openfrompath(fn,bmode,smode);
    if (fh < 0) return NULL;

    h = fdopen(fh,mode);
    if (!h) close(fh);

    return h;
}

#define MAXGROUPFILES 8     // Warning: Fix groupfil if this is changed
#define MAXOPENFILES 64     // Warning: Fix filehan if this is changed

enum {
    GRP_RESERVED_ID_START = 254,

    GRP_ZIP = GRP_RESERVED_ID_START,
    GRP_FILESYSTEM = GRP_RESERVED_ID_START + 1,
};

EDUKE32_STATIC_ASSERT(MAXGROUPFILES <= GRP_RESERVED_ID_START);

int32_t numgroupfiles = 0;
static int32_t gnumfiles[MAXGROUPFILES];
static intptr_t groupfil[MAXGROUPFILES] = {-1,-1,-1,-1,-1,-1,-1,-1};
static int32_t groupfilpos[MAXGROUPFILES];
static uint8_t groupfilgrp[MAXGROUPFILES];
static char *gfilelist[MAXGROUPFILES];
static char *groupname[MAXGROUPFILES];
static int32_t *gfileoffs[MAXGROUPFILES];

static uint8_t filegrp[MAXOPENFILES];
static int32_t filepos[MAXOPENFILES];
static intptr_t filehan[MAXOPENFILES] =
{
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

#ifdef WITHKPLIB
static char filenamsav[MAXOPENFILES][260];
static int32_t kzcurhand = -1;

int32_t cache1d_file_fromzip(int32_t fil)
{
    return (filegrp[fil] == GRP_ZIP);
}
#endif

static int32_t kopen_internal(const char *filename, char **lastpfn, char searchfirst, char checkcase, char tryzip, int32_t newhandle, uint8_t *arraygrp, intptr_t *arrayhan, int32_t *arraypos);
static int32_t kread_grp(int32_t handle, void *buffer, int32_t leng);
static int32_t klseek_grp(int32_t handle, int32_t offset, int32_t whence);
static void kclose_grp(int32_t handle);

int initgroupfile(const char *filename)
{
    char buf[70];

    // translate all backslashes (0x5c) to forward slashes (0x2f)
    toupperlookup[0x5c] = 0x2f;

    if (filename == NULL)
        return -1;

    // Technically you should be able to load more zips even if your GRPs are maxed out,
    // but this system is already enough of a disaster.
    if (numgroupfiles >= MAXGROUPFILES)
        return -1;

    char *zfn = NULL;

    if (kopen_internal(filename, &zfn, 0, 0, 0, numgroupfiles, groupfilgrp, groupfil, groupfilpos) < 0)
        return -1;

#ifdef WITHKPLIB
    // check if ZIP
    if (zfn)
    {
        kread_grp(numgroupfiles, buf, 4);
        if (buf[0] == 0x50 && buf[1] == 0x4B && buf[2] == 0x03 && buf[3] == 0x04)
        {
            kclose_grp(numgroupfiles);

            kzaddstack(zfn);
            Bfree(zfn);
            return MAXGROUPFILES;
        }
        klseek_grp(numgroupfiles,0,BSEEK_SET);

        Bfree(zfn);
    }
#else
    Bfree(zfn);
#endif

    // check if GRP
    kread_grp(numgroupfiles,buf,16);
    if (!Bmemcmp(buf, "KenSilverman", 12))
    {
        gnumfiles[numgroupfiles] = B_LITTLE32(*((int32_t *)&buf[12]));

        gfilelist[numgroupfiles] = (char *)Xmalloc(gnumfiles[numgroupfiles]<<4);
        gfileoffs[numgroupfiles] = (int32_t *)Xmalloc((gnumfiles[numgroupfiles]+1)<<2);

        kread_grp(numgroupfiles,gfilelist[numgroupfiles],gnumfiles[numgroupfiles]<<4);

        int32_t j = (gnumfiles[numgroupfiles]+1)<<4, k;
        for (bssize_t i=0; i<gnumfiles[numgroupfiles]; i++)
        {
            k = B_LITTLE32(*((int32_t *)&gfilelist[numgroupfiles][(i<<4)+12]));
            gfilelist[numgroupfiles][(i<<4)+12] = 0;
            gfileoffs[numgroupfiles][i] = j;
            j += k;
        }
        gfileoffs[numgroupfiles][gnumfiles[numgroupfiles]] = j;
        groupname[numgroupfiles] = Xstrdup(filename);
        return numgroupfiles++;
    }
    klseek_grp(numgroupfiles, 0, BSEEK_SET);

    // check if SSI
    // this performs several checks because there is no "SSI" magic
    int32_t version;
    kread_grp(numgroupfiles, &version, 4);
    version = B_LITTLE32(version);
    while (version == 1 || version == 2) // if
    {
        char zerobuf[70];
        Bmemset(zerobuf, 0, 70);

        int32_t numfiles;
        kread_grp(numgroupfiles, &numfiles, 4);
        numfiles = B_LITTLE32(numfiles);

        uint8_t temp, temp2;

        // get the string length
        kread_grp(numgroupfiles, &temp, 1);
        if (temp > 31) // 32 bytes allocated for the string
            break;
        // seek to the end of the string
        klseek_grp(numgroupfiles, temp, BSEEK_CUR);
        // verify everything remaining is a null terminator
        temp = 32 - temp;
        kread_grp(numgroupfiles, buf, temp);
        if (Bmemcmp(buf, zerobuf, temp))
            break;

        if (version == 2)
        {
            // get the string length
            kread_grp(numgroupfiles, &temp, 1);
            if (temp > 11) // 12 bytes allocated for the string
                break;
            // seek to the end of the string
            klseek_grp(numgroupfiles, temp, BSEEK_CUR);
            // verify everything remaining is a null terminator
            temp = 12 - temp;
            kread_grp(numgroupfiles, buf, temp);
            if (Bmemcmp(buf, zerobuf, temp))
                break;
        }

        temp2 = 0;
        for (uint8_t i=0;i<3;i++)
        {
            // get the string length
            kread_grp(numgroupfiles, &temp, 1);
            if (temp > 70) // 70 bytes allocated for the string
            {
                temp2 = 1;
                break;
            }
            // seek to the end of the string
            klseek_grp(numgroupfiles, temp, BSEEK_CUR);
            // verify everything remaining is a null terminator
            temp = 70 - temp;
            if (temp == 0)
                continue;
            kread_grp(numgroupfiles, buf, temp);
            temp2 |= Bmemcmp(buf, zerobuf, temp);
        }
        if (temp2)
            break;

        // Passed all the tests: read data.

        gnumfiles[numgroupfiles] = numfiles;

        gfilelist[numgroupfiles] = (char *)Xmalloc(gnumfiles[numgroupfiles]<<4);
        gfileoffs[numgroupfiles] = (int32_t *)Xmalloc((gnumfiles[numgroupfiles]+1)<<2);

        int32_t j = (version == 2 ? 267 : 254) + (numfiles * 121), k;
        for (bssize_t i = 0; i < numfiles; i++)
        {
            // get the string length
            kread_grp(numgroupfiles, &temp, 1);
            if (temp > 12)
                temp = 12;
            // read the file name
            kread_grp(numgroupfiles, &gfilelist[numgroupfiles][i<<4], temp);
            gfilelist[numgroupfiles][(i<<4)+temp] = 0;

            // skip to the end of the 12 bytes
            klseek_grp(numgroupfiles, 12-temp, BSEEK_CUR);

            // get the file size
            kread_grp(numgroupfiles, &k, 4);
            k = B_LITTLE32(k);

            // record the offset of the file in the SSI
            gfileoffs[numgroupfiles][i] = j;
            j += k;

            // skip unknown data
            klseek_grp(numgroupfiles, 104, BSEEK_CUR);
        }
        gfileoffs[numgroupfiles][gnumfiles[numgroupfiles]] = j;
        groupname[numgroupfiles] = Xstrdup(filename);
        return numgroupfiles++;
    }

    kclose_grp(numgroupfiles);
    return -1;
}

void uninitgroupfile(void)
{
    int32_t i;

    for (i=numgroupfiles-1; i>=0; i--)
        if (groupfil[i] != -1)
        {
            DO_FREE_AND_NULL(gfilelist[i]);
            DO_FREE_AND_NULL(gfileoffs[i]);
            DO_FREE_AND_NULL(groupname[i]);

            Bclose(groupfil[i]);
            groupfil[i] = -1;
        }
    numgroupfiles = 0;

    // JBF 20040111: "close" any files open in groups
    for (i=0; i<MAXOPENFILES; i++)
    {
        if (filegrp[i] < GRP_RESERVED_ID_START)   // JBF 20040130: not external or ZIPped
            filehan[i] = -1;
    }
}

#ifdef FILENAME_CASE_CHECK
// See
// http://stackoverflow.com/questions/74451/getting-actual-file-name-with-proper-casing-on-windows
// for relevant discussion.

static char fnbuf[BMAX_PATH];
int fnofs;

int32_t (*check_filename_casing_fn)(void) = NULL;

// -1: failure, 0: match, 1: mismatch
static int32_t check_filename_mismatch(const char * const filename, int ofs)
{
    if (!GetShortPathNameA(filename, fnbuf, BMAX_PATH)) return -1;
    if (!GetLongPathNameA(fnbuf, fnbuf, BMAX_PATH)) return -1;

    fnofs = ofs;

    int len = Bstrlen(fnbuf+ofs);

    char const * const fn = filename+ofs;

    if (!Bstrncmp(fnbuf+ofs, fn, len))
        return 0;

    char * const tfn = Bstrtolower(Xstrdup(fn));

    if (!Bstrncmp(fnbuf+ofs, tfn, len))
    {
        Bfree(tfn);
        return 0;
    }

    Bstrupr(tfn);

    if (!Bstrncmp(fnbuf+ofs, tfn, len))
    {
        Bfree(tfn);
        return 0;
    }

    Bfree(tfn);

    return 1;
}
#endif

static int32_t kopen_internal(const char *filename, char **lastpfn, char searchfirst, char checkcase, char tryzip, int32_t newhandle, uint8_t *arraygrp, intptr_t *arrayhan, int32_t *arraypos)
{
    int32_t fil;
    if (searchfirst == 0 && (fil = openfrompath_internal(filename, lastpfn, BO_BINARY|BO_RDONLY, BS_IREAD)) >= 0)
    {
#ifdef FILENAME_CASE_CHECK
        if (checkcase && check_filename_casing_fn && check_filename_casing_fn())
        {
            int32_t status;
            char *cp, *lastslash;

            // convert all slashes to backslashes because SHGetFileInfo()
            // complains else!
            lastslash = *lastpfn;
            for (cp=*lastpfn; *cp; cp++)
                if (*cp=='/')
                {
                    *cp = '\\';
                    lastslash = cp;
                }
            if (lastslash != *lastpfn)
                lastslash++;

            status = check_filename_mismatch(*lastpfn, lastslash-*lastpfn);

            if (status == -1)
            {
//                initprintf("SHGetFileInfo failed with error code %lu\n", GetLastError());
            }
            else if (status == 1)
            {
                initprintf("warning: case mismatch: passed \"%s\", real \"%s\"\n",
                           lastslash, fnbuf+fnofs);
            }
        }
#else
        UNREFERENCED_PARAMETER(checkcase);
#endif
        arraygrp[newhandle] = GRP_FILESYSTEM;
        arrayhan[newhandle] = fil;
        arraypos[newhandle] = 0;
        return newhandle;
    }

    for (; toupperlookup[*filename] == '/'; filename++) { }

#ifdef WITHKPLIB
    if (tryzip)
    {
        intptr_t i;
        if ((kzcurhand != newhandle) && (kztell() >= 0))
        {
            if (kzcurhand >= 0) arraypos[kzcurhand] = kztell();
            kzclose();
            kzcurhand = -1;
        }
        if (searchfirst != 1 && (i = kzipopen(filename)) != 0)
        {
            kzcurhand = newhandle;
            arraygrp[newhandle] = GRP_ZIP;
            arrayhan[newhandle] = i;
            arraypos[newhandle] = 0;
            strcpy(filenamsav[newhandle],filename);
            return newhandle;
        }
    }
#else
    UNREFERENCED_PARAMETER(tryzip);
#endif

    for (bssize_t k = searchfirst != 1 ? numgroupfiles-1 : 0; k >= 0; --k)
    {
        if (groupfil[k] < 0)
            continue;

        for (bssize_t i = gnumfiles[k]-1; i >= 0; --i)
        {
            char const * const gfileptr = (char *)&gfilelist[k][i<<4];

            unsigned int j;
            for (j = 0; j < 13; ++j)
            {
                if (!filename[j]) break;
                if (toupperlookup[filename[j]] != toupperlookup[gfileptr[j]])
                    goto gnumfiles_continue;
            }
            if (j<13 && gfileptr[j]) continue;   // JBF: because e1l1.map might exist before e1l1
            if (j==13 && filename[j]) continue;   // JBF: long file name

            arraygrp[newhandle] = k;
            arrayhan[newhandle] = i;
            arraypos[newhandle] = 0;
            return newhandle;

gnumfiles_continue: ;
        }
    }

    return -1;
}

void krename(int32_t crcval, int32_t filenum, const char *newname)
{
    Bstrncpy((char *)&gfilelist[crcval][filenum<<4], newname, 12);
}

char const * kfileparent(int32_t const handle)
{
    int32_t const groupnum = filegrp[handle];

    if ((unsigned)groupnum >= MAXGROUPFILES || groupfil[groupnum] == -1)
        return NULL;

    return groupname[groupnum];
}

int32_t kopen4load(const char *filename, char searchfirst)
{
    int32_t newhandle = MAXOPENFILES-1;

    if (filename==NULL)
        return -1;

    while (filehan[newhandle] != -1)
    {
        newhandle--;
        if (newhandle < 0)
        {
            Bprintf("TOO MANY FILES OPEN IN FILE GROUPING SYSTEM!");
            Bexit(0);
        }
    }

    char *lastpfn = NULL;

    int32_t h = kopen_internal(filename, &lastpfn, searchfirst, 1, 1, newhandle, filegrp, filehan, filepos);

    Bfree(lastpfn);

    return h;
}

int32_t kread_internal(int32_t handle, void *buffer, int32_t leng, uint8_t *arraygrp, intptr_t *arrayhan, int32_t *arraypos)
{
    int32_t filenum = arrayhan[handle];
    int32_t groupnum = arraygrp[handle];

    if (groupnum == GRP_FILESYSTEM) return Bread(filenum,buffer,leng);
#ifdef WITHKPLIB
    else if (groupnum == GRP_ZIP)
    {
        if (kzcurhand != handle)
        {
            if (kztell() >= 0) { arraypos[kzcurhand] = kztell(); kzclose(); }
            kzcurhand = handle;
            kzipopen(filenamsav[handle]);
            kzseek(arraypos[handle],SEEK_SET);
        }
        return kzread(buffer,leng);
    }
#endif

    if (EDUKE32_PREDICT_FALSE(groupfil[groupnum] == -1))
        return 0;

    int32_t rootgroupnum = groupnum;
    int32_t i = 0;
    while (groupfilgrp[rootgroupnum] != GRP_FILESYSTEM)
    {
        i += gfileoffs[groupfilgrp[rootgroupnum]][groupfil[rootgroupnum]];
        rootgroupnum = groupfilgrp[rootgroupnum];
    }
    if (EDUKE32_PREDICT_TRUE(groupfil[rootgroupnum] != -1))
    {
        i += gfileoffs[groupnum][filenum]+arraypos[handle];
        if (i != groupfilpos[rootgroupnum])
        {
            Blseek(groupfil[rootgroupnum],i,BSEEK_SET);
            groupfilpos[rootgroupnum] = i;
        }
        leng = min(leng,(gfileoffs[groupnum][filenum+1]-gfileoffs[groupnum][filenum])-arraypos[handle]);
        leng = Bread(groupfil[rootgroupnum],buffer,leng);
        arraypos[handle] += leng;
        groupfilpos[rootgroupnum] += leng;
        return leng;
    }

    return 0;
}

int32_t klseek_internal(int32_t handle, int32_t offset, int32_t whence, uint8_t *arraygrp, intptr_t *arrayhan, int32_t *arraypos)
{
    int32_t i, groupnum;

    groupnum = arraygrp[handle];

    if (groupnum == GRP_FILESYSTEM) return Blseek(arrayhan[handle],offset,whence);
#ifdef WITHKPLIB
    else if (groupnum == GRP_ZIP)
    {
        if (kzcurhand != handle)
        {
            if (kztell() >= 0) { arraypos[kzcurhand] = kztell(); kzclose(); }
            kzcurhand = handle;
            kzipopen(filenamsav[handle]);
            kzseek(arraypos[handle],SEEK_SET);
        }
        return kzseek(offset,whence);
    }
#endif

    if (groupfil[groupnum] != -1)
    {
        switch (whence)
        {
        case BSEEK_SET:
            arraypos[handle] = offset; break;
        case BSEEK_END:
            i = arrayhan[handle];
            arraypos[handle] = (gfileoffs[groupnum][i+1]-gfileoffs[groupnum][i])+offset;
            break;
        case BSEEK_CUR:
            arraypos[handle] += offset; break;
        }
        return arraypos[handle];
    }
    return -1;
}

int32_t kfilelength_internal(int32_t handle, uint8_t *arraygrp, intptr_t *arrayhan, int32_t *arraypos)
{
    int32_t i, groupnum;

    groupnum = arraygrp[handle];
    if (groupnum == GRP_FILESYSTEM)
    {
        // return (filelength(arrayhan[handle]))
        return Bfilelength(arrayhan[handle]);
    }
#ifdef WITHKPLIB
    else if (groupnum == GRP_ZIP)
    {
        if (kzcurhand != handle)
        {
            if (kztell() >= 0) { arraypos[kzcurhand] = kztell(); kzclose(); }
            kzcurhand = handle;
            kzipopen(filenamsav[handle]);
            kzseek(arraypos[handle],SEEK_SET);
        }
        return kzfilelength();
    }
#endif
    i = arrayhan[handle];
    return gfileoffs[groupnum][i+1]-gfileoffs[groupnum][i];
}

int32_t ktell_internal(int32_t handle, uint8_t *arraygrp, intptr_t *arrayhan, int32_t *arraypos)
{
    int32_t groupnum = arraygrp[handle];

    if (groupnum == GRP_FILESYSTEM) return Blseek(arrayhan[handle],0,BSEEK_CUR);
#ifdef WITHKPLIB
    else if (groupnum == GRP_ZIP)
    {
        if (kzcurhand != handle)
        {
            if (kztell() >= 0) { arraypos[kzcurhand] = kztell(); kzclose(); }
            kzcurhand = handle;
            kzipopen(filenamsav[handle]);
            kzseek(arraypos[handle],SEEK_SET);
        }
        return kztell();
    }
#endif
    if (groupfil[groupnum] != -1)
        return arraypos[handle];
    return -1;
}

void kclose_internal(int32_t handle, uint8_t *arraygrp, intptr_t *arrayhan)
{
    if (handle < 0) return;
    if (arraygrp[handle] == GRP_FILESYSTEM) Bclose(arrayhan[handle]);
#ifdef WITHKPLIB
    else if (arraygrp[handle] == GRP_ZIP)
    {
        kzclose();
        kzcurhand = -1;
    }
#endif
    arrayhan[handle] = -1;
}

int32_t kread(int32_t handle, void *buffer, int32_t leng)
{
    return kread_internal(handle, buffer, leng, filegrp, filehan, filepos);
}
int32_t klseek(int32_t handle, int32_t offset, int32_t whence)
{
    return klseek_internal(handle, offset, whence, filegrp, filehan, filepos);
}
int32_t kfilelength(int32_t handle)
{
    return kfilelength_internal(handle, filegrp, filehan, filepos);
}
int32_t ktell(int32_t handle)
{
    return ktell_internal(handle, filegrp, filehan, filepos);
}
void kclose(int32_t handle)
{
    return kclose_internal(handle, filegrp, filehan);
}

static int32_t kread_grp(int32_t handle, void *buffer, int32_t leng)
{
    return kread_internal(handle, buffer, leng, groupfilgrp, groupfil, groupfilpos);
}
static int32_t klseek_grp(int32_t handle, int32_t offset, int32_t whence)
{
    return klseek_internal(handle, offset, whence, groupfilgrp, groupfil, groupfilpos);
}
static void kclose_grp(int32_t handle)
{
    return kclose_internal(handle, groupfilgrp, groupfil);
}

static int32_t klistaddentry(CACHE1D_FIND_REC **rec, const char *name, int32_t type, int32_t source)
{
    CACHE1D_FIND_REC *r = NULL, *attach = NULL;

    if (*rec)
    {
        int32_t insensitive, v;
        CACHE1D_FIND_REC *last = NULL;

        for (attach = *rec; attach; last = attach, attach = attach->next)
        {
            if (type == CACHE1D_FIND_DRIVE) continue;	// we just want to get to the end for drives
#ifdef _WIN32
            insensitive = 1;
#else
            if (source == CACHE1D_SOURCE_GRP || attach->source == CACHE1D_SOURCE_GRP)
                insensitive = 1;
            else if (source == CACHE1D_SOURCE_ZIP || attach->source == CACHE1D_SOURCE_ZIP)
                insensitive = 1;
            else
            {
                extern int16_t editstatus;  // XXX
                insensitive = !editstatus;
            }
            // ^ in the game, don't show file list case-sensitive
#endif
            if (insensitive) v = Bstrcasecmp(name, attach->name);
            else v = Bstrcmp(name, attach->name);

            // sorted list
            if (v > 0) continue;	// item to add is bigger than the current one
            // so look for something bigger than us
            if (v < 0)  			// item to add is smaller than the current one
            {
                attach = NULL;		// so wedge it between the current item and the one before
                break;
            }

            // matched
            if (source >= attach->source) return 1;	// item to add is of lower priority
            r = attach;
            break;
        }

        // wasn't found in the list, so attach to the end
        if (!attach) attach = last;
    }

    if (r)
    {
        r->type = type;
        r->source = source;
        return 0;
    }

    r = (CACHE1D_FIND_REC *)Xmalloc(sizeof(CACHE1D_FIND_REC)+strlen(name)+1);

    r->name = (char *)r + sizeof(CACHE1D_FIND_REC); strcpy(r->name, name);
    r->type = type;
    r->source = source;
    r->usera = r->userb = NULL;

    if (!attach)  	// we are the first item
    {
        r->prev = NULL;
        r->next = *rec;
        if (*rec)(*rec)->prev = r;
        *rec = r;
    }
    else
    {
        r->prev = attach;
        r->next = attach->next;
        if (attach->next) attach->next->prev = r;
        attach->next = r;
    }

    return 0;
}

void klistfree(CACHE1D_FIND_REC *rec)
{
    CACHE1D_FIND_REC *n;

    while (rec)
    {
        n = rec->next;
        Bfree(rec);
        rec = n;
    }
}

CACHE1D_FIND_REC *klistpath(const char *_path, const char *mask, int32_t type)
{
    CACHE1D_FIND_REC *rec = NULL;
    char *path;

    // pathsearchmode == 0: enumerates a path in the virtual filesystem
    // pathsearchmode == 1: enumerates the system filesystem path passed in

    path = Xstrdup(_path);

    // we don't need any leading dots and slashes or trailing slashes either
    {
        int32_t i,j;
        for (i=0; path[i] == '.' || toupperlookup[path[i]] == '/';) i++;
        for (j=0; (path[j] = path[i]); j++,i++) ;
        while (j>0 && toupperlookup[path[j-1]] == '/') j--;
        path[j] = 0;
        //initprintf("Cleaned up path = \"%s\"\n",path);
    }

    if (*path && (type & CACHE1D_FIND_DIR))
    {
        if (klistaddentry(&rec, "..", CACHE1D_FIND_DIR, CACHE1D_SOURCE_CURDIR) < 0) goto failure;
    }

    if (!(type & CACHE1D_OPT_NOSTACK))  	// current directory and paths in the search stack
    {
        searchpath_t *search = NULL;
        BDIR *dir;
        struct Bdirent *dirent;

        static const char *const CUR_DIR = "./";
        // Adjusted for the following "autoload" dir fix - NY00123
        const char *d = pathsearchmode ? _path : CUR_DIR;
        int32_t stackdepth = CACHE1D_SOURCE_CURDIR;
        char buf[BMAX_PATH];

        do
        {
            if (d==CUR_DIR && (type & CACHE1D_FIND_NOCURDIR))
                goto next;

            strcpy(buf, d);
            if (!pathsearchmode)
            {
                // Fix for "autoload" dir in multi-user environments - NY00123
                strcat(buf, path);
                if (*path) strcat(buf, "/");
            }

            dir = Bopendir(buf);
            if (dir)
            {
                while ((dirent = Breaddir(dir)))
                {
                    if ((dirent->name[0] == '.' && dirent->name[1] == 0) ||
                            (dirent->name[0] == '.' && dirent->name[1] == '.' && dirent->name[2] == 0))
                        continue;
                    if ((type & CACHE1D_FIND_DIR) && !(dirent->mode & BS_IFDIR)) continue;
                    if ((type & CACHE1D_FIND_FILE) && (dirent->mode & BS_IFDIR)) continue;
                    if (!Bwildmatch(dirent->name, mask)) continue;
                    switch (klistaddentry(&rec, dirent->name,
                                          (dirent->mode & BS_IFDIR) ? CACHE1D_FIND_DIR : CACHE1D_FIND_FILE,
                                          stackdepth))
                    {
                    case -1: goto failure;
                        //case 1: initprintf("%s:%s dropped for lower priority\n", d,dirent->name); break;
                        //case 0: initprintf("%s:%s accepted\n", d,dirent->name); break;
                    default:
                        break;
                    }
                }
                Bclosedir(dir);
            }
next:
            if (pathsearchmode)
                break;

            if (!search)
            {
                search = searchpathhead;
                stackdepth = CACHE1D_SOURCE_PATH;
            }
            else
            {
                search = search->next;
                stackdepth++;
            }

            if (search)
                d = search->path;
        }
        while (search);
    }

#ifdef WITHKPLIB
    if (!(type & CACHE1D_FIND_NOCURDIR))  // TEMP, until we have sorted out fs.listpath() API
    if (!pathsearchmode)  	// next, zip files
    {
        char buf[BMAX_PATH+4];
        int32_t i, j, ftype;
        strcpy(buf,path);
        if (*path) strcat(buf,"/");
        strcat(buf,mask);
        for (kzfindfilestart(buf); kzfindfile(buf);)
        {
            if (buf[0] != '|') continue;	// local files we don't need

            // scan for the end of the string and shift
            // everything left a char in the process
            for (i=1; (buf[i-1]=buf[i]); i++)
            {
                /* do nothing */
            }
            i-=2;
            if (i < 0)
                i = 0;

            // if there's a slash at the end, this is a directory entry
            if (toupperlookup[buf[i]] == '/') { ftype = CACHE1D_FIND_DIR; buf[i] = 0; }
            else ftype = CACHE1D_FIND_FILE;

            // skip over the common characters at the beginning of the base path and the zip entry
            for (j=0; buf[j] && path[j]; j++)
            {
                if (toupperlookup[ path[j] ] == toupperlookup[ buf[j] ]) continue;
                break;
            }
            // we've now hopefully skipped the common path component at the beginning.
            // if that's true, we should be staring at a null byte in path and either any character in buf
            // if j==0, or a slash if j>0
            if ((!path[0] && buf[j]) || (!path[j] && toupperlookup[ buf[j] ] == '/'))
            {
                if (j>0) j++;

                // yep, so now we shift what follows back to the start of buf and while we do that,
                // keep an eye out for any more slashes which would mean this entry has sub-entities
                // and is useless to us.
                for (i = 0; (buf[i] = buf[j]) && toupperlookup[buf[j]] != '/'; i++,j++) ;
                if (toupperlookup[buf[j]] == '/') continue;	// damn, try next entry
            }
            else
            {
                // if we're here it means we have a situation where:
                //   path = foo
                //   buf = foobar...
                // or
                //   path = foobar
                //   buf = foo...
                // which would mean the entry is higher up in the directory tree and is also useless
                continue;
            }

            if ((type & CACHE1D_FIND_DIR) && ftype != CACHE1D_FIND_DIR) continue;
            if ((type & CACHE1D_FIND_FILE) && ftype != CACHE1D_FIND_FILE) continue;

            // the entry is in the clear
            switch (klistaddentry(&rec, buf, ftype, CACHE1D_SOURCE_ZIP))
            {
            case -1:
                goto failure;
                //case 1: initprintf("<ZIP>:%s dropped for lower priority\n", buf); break;
                //case 0: initprintf("<ZIP>:%s accepted\n", buf); break;
            default:
                break;
            }
        }
    }
#endif
    // then, grp files
    if (!(type & CACHE1D_FIND_NOCURDIR))  // TEMP, until we have sorted out fs.listpath() API
    if (!pathsearchmode && !*path && (type & CACHE1D_FIND_FILE))
    {
        char buf[13];
        int32_t i,j;
        buf[12] = 0;
        for (i=0; i<MAXGROUPFILES; i++)
        {
            if (groupfil[i] == -1) continue;
            for (j=gnumfiles[i]-1; j>=0; j--)
            {
                Bmemcpy(buf,&gfilelist[i][j<<4],12);
                if (!Bwildmatch(buf,mask)) continue;
                switch (klistaddentry(&rec, buf, CACHE1D_FIND_FILE, CACHE1D_SOURCE_GRP))
                {
                case -1:
                    goto failure;
                    //case 1: initprintf("<GRP>:%s dropped for lower priority\n", workspace); break;
                    //case 0: initprintf("<GRP>:%s accepted\n", workspace); break;
                default:
                    break;
                }
            }
        }
    }

    if (pathsearchmode && (type & CACHE1D_FIND_DRIVE))
    {
        char *drives, *drp;
        drives = Bgetsystemdrives();
        if (drives)
        {
            for (drp=drives; *drp; drp+=strlen(drp)+1)
            {
                if (klistaddentry(&rec, drp, CACHE1D_FIND_DRIVE, CACHE1D_SOURCE_DRIVE) < 0)
                {
                    Bfree(drives);
                    goto failure;
                }
            }
            Bfree(drives);
        }
    }

    Bfree(path);
    // XXX: may be NULL if no file was listed, and thus indistinguishable from
    // an error condition.
    return rec;
failure:
    Bfree(path);
    klistfree(rec);
    return NULL;
}


#endif // #ifdef CACHE1D_COMPRESS_ONLY / else


//Internal LZW variables
#define LZWSIZE 16384           //Watch out for shorts!
#define LZWSIZEPAD (LZWSIZE+(LZWSIZE>>4))

// lzwrawbuf LZWSIZE+1 (formerly): see (*) below
// XXX: lzwrawbuf size increased again :-/
static char lzwtmpbuf[LZWSIZEPAD], lzwrawbuf[LZWSIZEPAD], lzwcompbuf[LZWSIZEPAD];
static int16_t lzwbuf2[LZWSIZEPAD], lzwbuf3[LZWSIZEPAD];

static int32_t lzwcompress(const char *lzwinbuf, int32_t uncompleng, char *lzwoutbuf);
static int32_t lzwuncompress(const char *lzwinbuf, int32_t compleng, char *lzwoutbuf);

#ifndef CACHE1D_COMPRESS_ONLY
static int32_t kdfread_func(intptr_t fil, void *outbuf, int32_t length)
{
    return kread((int32_t)fil, outbuf, length);
}

static void dfwrite_func(intptr_t fp, const void *inbuf, int32_t length)
{
    Bfwrite(inbuf, length, 1, (BFILE *)fp);
}
#else
# define kdfread_func NULL
# define dfwrite_func NULL
#endif

// These two follow the argument order of the C functions "read" and "write":
// handle, buffer, length.
C1D_STATIC int32_t (*c1d_readfunc)(intptr_t, void *, int32_t) = kdfread_func;
C1D_STATIC void (*c1d_writefunc)(intptr_t, const void *, int32_t) = dfwrite_func;


////////// COMPRESSED READ //////////

static uint32_t decompress_part(intptr_t f, uint32_t *kgoalptr)
{
    int16_t leng;

    // Read compressed length first.
    if (c1d_readfunc(f, &leng, 2) != 2)
        return 1;
    leng = B_LITTLE16(leng);

    if (c1d_readfunc(f,lzwcompbuf, leng) != leng)
        return 1;

    *kgoalptr = lzwuncompress(lzwcompbuf, leng, lzwrawbuf);
    return 0;
}

// Read from 'f' into 'buffer'.
C1D_STATIC int32_t c1d_read_compressed(void *buffer, bsize_t dasizeof, bsize_t count, intptr_t f)
{
    char *ptr = (char *)buffer;

    if (dasizeof > LZWSIZE)
    {
        count *= dasizeof;
        dasizeof = 1;
    }

    uint32_t kgoal;

    if (decompress_part(f, &kgoal))
        return -1;

    Bmemcpy(ptr, lzwrawbuf, (int32_t)dasizeof);

    uint32_t k = (int32_t)dasizeof;

    for (uint32_t i=1; i<count; i++)
    {
        if (k >= kgoal)
        {
            k = decompress_part(f, &kgoal);
            if (k) return -1;
        }

        uint32_t j = 0;

        if (dasizeof >= 4)
        {
            for (; j<dasizeof-4; j+=4)
            {
                ptr[j+dasizeof] = ((ptr[j]+lzwrawbuf[j+k])&255);
                ptr[j+1+dasizeof] = ((ptr[j+1]+lzwrawbuf[j+1+k])&255);
                ptr[j+2+dasizeof] = ((ptr[j+2]+lzwrawbuf[j+2+k])&255);
                ptr[j+3+dasizeof] = ((ptr[j+3]+lzwrawbuf[j+3+k])&255);
            }
        }

        for (; j<dasizeof; j++)
            ptr[j+dasizeof] = ((ptr[j]+lzwrawbuf[j+k])&255);

        k += dasizeof;
        ptr += dasizeof;
    }

    return count;
}

int32_t kdfread(void *buffer, bsize_t dasizeof, bsize_t count, int32_t fil)
{
    return c1d_read_compressed(buffer, dasizeof, count, (intptr_t)fil);
}


////////// COMPRESSED WRITE //////////

static uint32_t compress_part(uint32_t k, intptr_t f)
{
    const int16_t leng = (int16_t)lzwcompress(lzwrawbuf, k, lzwcompbuf);
    const int16_t swleng = B_LITTLE16(leng);

    c1d_writefunc(f, &swleng, 2);
    c1d_writefunc(f, lzwcompbuf, leng);

    return 0;
}

// Write from 'buffer' to 'f'.
C1D_STATIC void c1d_write_compressed(const void *buffer, bsize_t dasizeof, bsize_t count, intptr_t f)
{
    char const *ptr = (char const *)buffer;

    if (dasizeof > LZWSIZE)
    {
        count *= dasizeof;
        dasizeof = 1;
    }

    Bmemcpy(lzwrawbuf, ptr, (int32_t)dasizeof);

    uint32_t k = dasizeof;
    if (k > LZWSIZE-dasizeof)
        k = compress_part(k, f);

    for (uint32_t i=1; i<count; i++)
    {
        uint32_t j = 0;

        if (dasizeof >= 4)
        {
            for (; j<dasizeof-4; j+=4)
            {
                lzwrawbuf[j+k] = ((ptr[j+dasizeof]-ptr[j])&255);
                lzwrawbuf[j+1+k] = ((ptr[j+1+dasizeof]-ptr[j+1])&255);
                lzwrawbuf[j+2+k] = ((ptr[j+2+dasizeof]-ptr[j+2])&255);
                lzwrawbuf[j+3+k] = ((ptr[j+3+dasizeof]-ptr[j+3])&255);
            }
        }

        for (; j<dasizeof; j++)
            lzwrawbuf[j+k] = ((ptr[j+dasizeof]-ptr[j])&255);

        k += dasizeof;
        if (k > LZWSIZE-dasizeof)
            k = compress_part(k, f);

        ptr += dasizeof;
    }

    if (k > 0)
        compress_part(k, f);
}

void dfwrite(const void *buffer, bsize_t dasizeof, bsize_t count, BFILE *fil)
{
    c1d_write_compressed(buffer, dasizeof, count, (intptr_t)fil);
}

////////// CORE COMPRESSION FUNCTIONS //////////

static int32_t lzwcompress(const char *lzwinbuf, int32_t uncompleng, char *lzwoutbuf)
{
    int32_t i, addr, addrcnt, *intptr;
    int32_t bytecnt1, bitcnt, numbits, oneupnumbits;
    int16_t *shortptr;

    int16_t *const lzwcodehead = lzwbuf2;
    int16_t *const lzwcodenext = lzwbuf3;

    for (i=255; i>=4; i-=4)
    {
        lzwtmpbuf[i]   = i,   lzwcodenext[i]   = (i+1)&255;
        lzwtmpbuf[i-1] = i-1, lzwcodenext[i-1] = (i)  &255;
        lzwtmpbuf[i-2] = i-2, lzwcodenext[i-2] = (i-1)&255;
        lzwtmpbuf[i-3] = i-3, lzwcodenext[i-3] = (i-2)&255;
        lzwcodehead[i] = lzwcodehead[i-1] = lzwcodehead[i-2] = lzwcodehead[i-3] = -1;
    }

    for (; i>=0; i--)
    {
        lzwtmpbuf[i] = i;
        lzwcodenext[i] = (i+1)&255;
        lzwcodehead[i] = -1;
    }

    Bmemset(lzwoutbuf, 0, 4+uncompleng+1);
//    clearbuf(lzwoutbuf,((uncompleng+15)+3)>>2,0L);

    addrcnt = 256; bytecnt1 = 0; bitcnt = (4<<3);
    numbits = 8; oneupnumbits = (1<<8);
    do
    {
        addr = lzwinbuf[bytecnt1];
        do
        {
            int32_t newaddr;

            if (++bytecnt1 == uncompleng)
                break;  // (*) see XXX below

            if (lzwcodehead[addr] < 0)
            {
                lzwcodehead[addr] = addrcnt;
                break;
            }

            newaddr = lzwcodehead[addr];
            while (lzwtmpbuf[newaddr] != lzwinbuf[bytecnt1])
            {
                if (lzwcodenext[newaddr] < 0)
                {
                    lzwcodenext[newaddr] = addrcnt;
                    break;
                }
                newaddr = lzwcodenext[newaddr];
            }

            if (lzwcodenext[newaddr] == addrcnt)
                break;
            addr = newaddr;
        }
        while (addr >= 0);

        lzwtmpbuf[addrcnt] = lzwinbuf[bytecnt1];  // XXX: potential oob access of lzwinbuf via (*) above
        lzwcodehead[addrcnt] = -1;
        lzwcodenext[addrcnt] = -1;

        intptr = (int32_t *)&lzwoutbuf[bitcnt>>3];
        intptr[0] |= B_LITTLE32(addr<<(bitcnt&7));
        bitcnt += numbits;
        if ((addr&((oneupnumbits>>1)-1)) > ((addrcnt-1)&((oneupnumbits>>1)-1)))
            bitcnt--;

        addrcnt++;
        if (addrcnt > oneupnumbits)
            { numbits++; oneupnumbits <<= 1; }
    }
    while ((bytecnt1 < uncompleng) && (bitcnt < (uncompleng<<3)));

    intptr = (int32_t *)&lzwoutbuf[bitcnt>>3];
    intptr[0] |= B_LITTLE32(addr<<(bitcnt&7));
    bitcnt += numbits;
    if ((addr&((oneupnumbits>>1)-1)) > ((addrcnt-1)&((oneupnumbits>>1)-1)))
        bitcnt--;

    shortptr = (int16_t *)lzwoutbuf;
    shortptr[0] = B_LITTLE16((int16_t)uncompleng);

    if (((bitcnt+7)>>3) < uncompleng)
    {
        shortptr[1] = B_LITTLE16((int16_t)addrcnt);
        return (bitcnt+7)>>3;
    }

    // Failed compressing, mark this in the stream.
    shortptr[1] = 0;

    for (i=0; i<uncompleng-4; i+=4)
    {
        lzwoutbuf[i+4] = lzwinbuf[i];
        lzwoutbuf[i+5] = lzwinbuf[i+1];
        lzwoutbuf[i+6] = lzwinbuf[i+2];
        lzwoutbuf[i+7] = lzwinbuf[i+3];
    }

    for (; i<uncompleng; i++)
        lzwoutbuf[i+4] = lzwinbuf[i];

    return uncompleng+4;
}

static int32_t lzwuncompress(const char *lzwinbuf, int32_t compleng, char *lzwoutbuf)
{
    int32_t currstr, numbits, oneupnumbits;
    int32_t i, bitcnt, outbytecnt;

    const int16_t *const shortptr = (const int16_t *)lzwinbuf;
    const int32_t strtot = B_LITTLE16(shortptr[1]);
    const int32_t uncompleng = B_LITTLE16(shortptr[0]);

    if (strtot == 0)
    {
        if (lzwoutbuf==lzwrawbuf && lzwinbuf==lzwcompbuf)
        {
            Bassert((compleng-4)+3+0u < sizeof(lzwrawbuf));
            Bassert((compleng-4)+3+0u < sizeof(lzwcompbuf)-4);
        }

        Bmemcpy(lzwoutbuf, lzwinbuf+4, (compleng-4)+3);
        return uncompleng;
    }

    for (i=255; i>=4; i-=4)
    {
        lzwbuf2[i]   = lzwbuf3[i]   = i;
        lzwbuf2[i-1] = lzwbuf3[i-1] = i-1;
        lzwbuf2[i-2] = lzwbuf3[i-2] = i-2;
        lzwbuf2[i-3] = lzwbuf3[i-3] = i-3;
    }

    lzwbuf2[i]   = lzwbuf3[i]   = i;
    lzwbuf2[i-1] = lzwbuf3[i-1] = i-1;
    lzwbuf2[i-2] = lzwbuf3[i-2] = i-2;

    currstr = 256; bitcnt = (4<<3); outbytecnt = 0;
    numbits = 8; oneupnumbits = (1<<8);
    do
    {
        const int32_t *const intptr = (const int32_t *)&lzwinbuf[bitcnt>>3];

        int32_t dat = ((B_LITTLE32(intptr[0])>>(bitcnt&7)) & (oneupnumbits-1));
        int32_t leng;

        bitcnt += numbits;
        if ((dat&((oneupnumbits>>1)-1)) > ((currstr-1)&((oneupnumbits>>1)-1)))
            { dat &= ((oneupnumbits>>1)-1); bitcnt--; }

        lzwbuf3[currstr] = dat;

        for (leng=0; dat>=256; leng++,dat=lzwbuf3[dat])
            lzwtmpbuf[leng] = lzwbuf2[dat];

        lzwoutbuf[outbytecnt++] = dat;

        for (i=leng-1; i>=4; i-=4, outbytecnt+=4)
        {
            lzwoutbuf[outbytecnt]   = lzwtmpbuf[i];
            lzwoutbuf[outbytecnt+1] = lzwtmpbuf[i-1];
            lzwoutbuf[outbytecnt+2] = lzwtmpbuf[i-2];
            lzwoutbuf[outbytecnt+3] = lzwtmpbuf[i-3];
        }

        for (; i>=0; i--)
            lzwoutbuf[outbytecnt++] = lzwtmpbuf[i];

        lzwbuf2[currstr-1] = dat; lzwbuf2[currstr] = dat;
        currstr++;
        if (currstr > oneupnumbits)
            { numbits++; oneupnumbits <<= 1; }
    }
    while (currstr < strtot);

    return uncompleng;
}

/*
 * vim:ts=4:sw=4:
 */
