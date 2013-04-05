// cache1d.h

#ifndef __cache1d_h__
#define __cache1d_h__

#include "compat.h"
#include "mutex.h"

#ifdef EXTERNC
extern "C" {
#endif

#define MAXGROUPFILES 8     //Warning: Fix groupfil if this is changed
#define MAXOPENFILES 64     //Warning: Fix filehan if this is changed

#ifdef WITHKPLIB
int32_t cache1d_file_fromzip(int32_t fil);
#endif

void	initcache(intptr_t dacachestart, int32_t dacachesize);
void	allocache(intptr_t *newhandle, int32_t newbytes, char *newlockptr);
void	agecache(void);

extern int32_t pathsearchmode;	// 0 = gamefs mode (default), 1 = localfs mode (editor's mode)
int32_t     addsearchpath(const char *p);
int32_t     removesearchpath(const char *p);
int32_t		findfrompath(const char *fn, char **where);
int32_t     openfrompath(const char *fn, int32_t flags, int32_t mode);
BFILE  *fopenfrompath(const char *fn, const char *mode);

int32_t	initgroupfile(const char *filename);
void	uninitsinglegroupfile(int32_t grphandle);
void	uninitgroupfile(void);
int32_t	kopen4load(const char *filename, char searchfirst);	// searchfirst: 0 = anywhere, 1 = first group, 2 = any group
int32_t	kread(int32_t handle, void *buffer, int32_t leng);
int32_t	klseek(int32_t handle, int32_t offset, int32_t whence);
int32_t	kfilelength(int32_t handle);
int32_t	ktell(int32_t handle);
void	kclose(int32_t handle);

typedef struct { intptr_t *hand; int32_t leng; char *lock ; } cactype;

enum {
	CACHE1D_FIND_FILE = 1,
	CACHE1D_FIND_DIR = 2,
	CACHE1D_FIND_DRIVE = 4,

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
void klistfree(CACHE1D_FIND_REC *rec);
CACHE1D_FIND_REC *klistpath(const char *path, const char *mask, int32_t type);

int32_t	kdfread(void *buffer, bsize_t dasizeof, bsize_t count, int32_t fil);
#if 0
int32_t	dfread(void *buffer, bsize_t dasizeof, bsize_t count, BFILE *fil);
void	kdfwrite(const void *buffer, bsize_t dasizeof, bsize_t count, int32_t fil);
#endif
void	dfwrite(const void *buffer, bsize_t dasizeof, bsize_t count, BFILE *fil);

#ifdef EXTERNC
}
#endif

#endif // __cache1d_h__

