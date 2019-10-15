// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#ifndef cache1d_h_
#define cache1d_h_

#include "compat.h"
#include "files.h"

#include "vfs.h"

extern char *kpzbuf;
extern int32_t kpzbufsiz;
extern int32_t kpzbufload(const char *);

void	cacheInitBuffer(int32_t dacachesize);
void	cacheAllocateBlock(intptr_t *newhandle, int32_t newbytes, uint8_t *newlockptr);
void	cacheAgeEntries(void);

using buildvfs_kfd = int32_t;
#define buildvfs_kfd_invalid (-1)

extern int32_t pathsearchmode;	// 0 = gamefs mode (default), 1 = localfs mode (editor's mode)
char *listsearchpath(int32_t initp);
int32_t     addsearchpath_user(const char *p, int32_t user);
#define addsearchpath(a) addsearchpath_user(a, 0)
int32_t     removesearchpath(const char *p);
void     removesearchpaths_withuser(int32_t usermask);
int32_t		findfrompath(const char *fn, char **where);
buildvfs_kfd     openfrompath(const char *fn, int32_t flags, int32_t mode);
buildvfs_FILE fopenfrompath(const char *fn, const char *mode);

extern char g_modDir[BMAX_PATH];
extern int32_t numgroupfiles;
int initgroupfile(const char *filename);
void	uninitgroupfile(void);
buildvfs_kfd	kopen4load(const char *filename, char searchfirst);	// searchfirst: 0 = anywhere, 1 = first group, 2 = any group
buildvfs_kfd	kopen4loadfrommod(const char *filename, char searchfirst);
int32_t	kread(buildvfs_kfd handle, void *buffer, int32_t leng);
#define kread_and_test(handle, buffer, leng) EDUKE32_PREDICT_FALSE(kread((handle), (buffer), (leng)) != (leng))
int32_t	klseek(buildvfs_kfd handle, int32_t offset, int32_t whence);
#define klseek_and_test(handle, offset, whence) EDUKE32_PREDICT_FALSE(klseek((handle), (offset), (whence)) < 0)
int32_t	kfilelength(buildvfs_kfd handle);
int32_t	ktell(buildvfs_kfd handle);
void	kclose(buildvfs_kfd handle);

void krename(int32_t crcval, int32_t filenum, const char *newname);
char const * kfileparent(int32_t handle);

extern int32_t kpzbufloadfil(buildvfs_kfd);

#ifdef WITHKPLIB
int32_t cache1d_file_fromzip(buildvfs_kfd fil);
#endif

typedef struct
{
    intptr_t *hand;
    int32_t   leng;
    uint8_t *    lock;
} cactype;

enum {
    CACHE1D_FIND_FILE = 1,
    CACHE1D_FIND_DIR = 2,
    CACHE1D_FIND_DRIVE = 4,
    CACHE1D_FIND_NOCURDIR = 8,

    CACHE1D_OPT_NOSTACK = 0x100,

    // the lower the number, the higher the priority
    CACHE1D_SOURCE_DRIVE = 0,
    CACHE1D_SOURCE_CURDIR = 1,
    CACHE1D_SOURCE_PATH = 2,	// + path stack depth
    CACHE1D_SOURCE_ZIP = 0x7ffffffe,
    CACHE1D_SOURCE_GRP = 0x7fffffff,
};
typedef struct _CACHE1D_FIND_REC {
    char *name;
    int32_t type, source;
    struct _CACHE1D_FIND_REC *next, *prev, *usera, *userb;
} CACHE1D_FIND_REC;
int32_t klistaddentry(CACHE1D_FIND_REC **rec, const char *name, int32_t type, int32_t source);
void klistfree(CACHE1D_FIND_REC *rec);
CACHE1D_FIND_REC *klistpath(const char *path, const char *mask, int type);

extern int32_t lz4CompressionLevel;
int32_t kdfread_LZ4(void *buffer, int dasizeof, int count, buildvfs_kfd fil);
void dfwrite_LZ4(const void *buffer, int dasizeof, int count, buildvfs_FILE fil);

class KFileReaderInterface : public FileReaderInterface
{
	buildvfs_kfd khandle = buildvfs_kfd_invalid;
public:

	KFileReaderInterface(int handle)
	{
		khandle = handle;
		Length = 0;
		if (khandle != buildvfs_kfd_invalid)
		{
			klseek(khandle, 0, SEEK_END);
			Length = ktell(khandle);
			klseek(khandle, 0, SEEK_SET);
		}
	}
	~KFileReaderInterface() 
	{
		if (khandle != buildvfs_kfd_invalid)
		{
			kclose(khandle);
		}
	}
	virtual long Tell() const
	{
		return ktell(khandle);
	}
	virtual long Seek(long offset, int origin)
	{
		return klseek(khandle, offset, origin);
	}
	virtual long Read(void* buffer, long len)
	{
		return kread(khandle, buffer, (int32_t)len);
	}
	virtual char* Gets(char* strbuf, int len)
	{
		// Not supported by the underlying system, so we do not need it anyway.
		return nullptr;
	}

};

inline FileReader kopenFileReader(const char* name, int where)
{
	int handle = where == 0 ? kopen4loadfrommod(name, 0) : kopen4load(name, where);
	KFileReaderInterface *fri = handle == buildvfs_kfd_invalid? nullptr : new KFileReaderInterface(handle);
	return FileReader(fri);
}

#endif // cache1d_h_

