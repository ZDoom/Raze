// cache1d.h

#ifndef __cache1d_h__
#define __cache1d_h__

#ifdef __cplusplus
extern "C" {
#endif

void	initcache(long dacachestart, long dacachesize);
void	allocache(long *newhandle, long newbytes, char *newlockptr);
void	suckcache(long *suckptr);
void	agecache(void);

extern int pathsearchmode;	// 0 = gamefs mode (default), 1 = localfs mode (editor's mode)
int     addsearchpath(const char *p);
int		findfrompath(const char *fn, char **where);
int     openfrompath(const char *fn, int flags, int mode);
BFILE  *fopenfrompath(const char *fn, const char *mode);

long	initgroupfile(char *filename);
void	uninitsinglegroupfile(long grphandle);
void	uninitgroupfile(void);
long	kopen4load(char *filename, char searchfirst);	// searchfirst: 0 = anywhere, 1 = first group, 2 = any group
long	kread(long handle, void *buffer, long leng);
long	klseek(long handle, long offset, long whence);
long	kfilelength(long handle);
long	ktell(long handle);
void	kclose(long handle);

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
	int type, source;
	struct _CACHE1D_FIND_REC *next, *prev, *usera, *userb;
} CACHE1D_FIND_REC;
void klistfree(CACHE1D_FIND_REC *rec);
CACHE1D_FIND_REC *klistpath(const char *path, const char *mask, int type);

int	kdfread(void *buffer, bsize_t dasizeof, bsize_t count, long fil);
int	dfread(void *buffer, bsize_t dasizeof, bsize_t count, BFILE *fil);
void	kdfwrite(void *buffer, bsize_t dasizeof, bsize_t count, long fil);
void	dfwrite(void *buffer, bsize_t dasizeof, bsize_t count, BFILE *fil);

#ifdef __cplusplus
}
#endif

#endif // __cache1d_h__

