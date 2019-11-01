// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "compat.h"

#ifdef _WIN32
// for FILENAME_CASE_CHECK
# define NEED_SHELLAPI_H
# include "windows_inc.h"
#endif
#include "cache1d.h"
#include "pragmas.h"
#include "baselayer.h"
#include "lz4.h"

#include "vfs.h"

#ifdef WITHKPLIB
#include "kplib.h"
#include "zstring.h"

//Insert '|' in front of filename
//Doing this tells kzopen to load the file only if inside a .ZIP file
static intptr_t kzipopen(const char *filnam)
{
    uint32_t i;
    char newst[BMAX_PATH+8];

    newst[0] = '|';
    for (i=0; i < BMAX_PATH+4 && filnam[i]; i++) newst[i+1] = filnam[i];
    newst[i+1] = 0;
    return kzopen(newst);
}

#endif

char *kpzbuf = NULL;


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



uint8_t toupperlookup[256] =
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



// Only the sound code still uses this - but it never frees the data.
// So we may just toss out the cache and do regular allocations.
// The TArray is merely for taking down the data before shutdown.
static TArray<TArray<uint8_t>> pseudocache;

void cacheAllocateBlock(intptr_t *newhandle, int32_t newbytes, uint8_t *)
{
	pseudocache.Reserve(1);
	auto& buffer = pseudocache.Last();
	buffer.Resize(newbytes);
	*newhandle = reinterpret_cast<intptr_t>(buffer.Data());
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


char g_modDir[BMAX_PATH] = "/";



int32_t klistaddentry(CACHE1D_FIND_REC **rec, const char *name, int32_t type, int32_t source)
{
    CACHE1D_FIND_REC *r = NULL, *attach = NULL;

    if (*rec)
    {
        int32_t insensitive, v;
        CACHE1D_FIND_REC *last = NULL;

        for (attach = *rec; attach; last = attach, attach = attach->next)
        {
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
        Xfree(rec);
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
        if (klistaddentry(&rec, "..", CACHE1D_FIND_DIR, CACHE1D_SOURCE_CURDIR) < 0)
        {
            Xfree(path);
            klistfree(rec);
            return NULL;
        }
    }

    if (!(type & CACHE1D_OPT_NOSTACK))  	// current directory and paths in the search stack
    {

        int32_t stackdepth = CACHE1D_SOURCE_CURDIR;


        static const char *const CUR_DIR = "./";
        // Adjusted for the following "autoload" dir fix - NY00123
        searchpath_t *search = NULL;
        const char *d = pathsearchmode ? _path : CUR_DIR;
        char buf[BMAX_PATH];
        BDIR *dir;
        struct Bdirent *dirent;
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

    Xfree(path);
    // XXX: may be NULL if no file was listed, and thus indistinguishable from
    // an error condition.
    return rec;
failure:
    Xfree(path);
    klistfree(rec);
    return NULL;
}

