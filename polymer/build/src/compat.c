/*
 * Playing-field leveller for Build
 * by Jonathon Fowler
 *
 * A note about this:
 * 1. There is some kind of problem somewhere in the functions below because
 *    compiling with __compat_h_macrodef__ disabled makes stupid things happen.
 * 2. The functions below, aside from the ones which aren't trivial, were never
 *    really intended to be used for anything except tracking anr removing ties
 *    to the standard C library from games. Using the Bxx versions of functions
 *    means we can redefine those names to link up with different runtime library
 *    names.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0400
#include <windows.h>
#include <shlobj.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(__WATCOMC__)
# include <direct.h>
#elif defined(_MSC_VER)
# include <io.h>
#else
# include <dirent.h>
#endif

#include "compat.h"


#ifndef __compat_h_macrodef__

int Brand(void)
{
	return rand();
}

void *Bmalloc(bsize_t size)
{
	return malloc(size);
}

void Bfree(void *ptr)
{
	free(ptr);
}

int Bopen(const char *pathname, int flags, unsigned mode)
{
	int n=0,o=0;
	
	if (flags&BO_BINARY) n|=O_BINARY; else n|=O_TEXT;
	if ((flags&BO_RDWR)==BO_RDWR) n|=O_RDWR;
	else if ((flags&BO_RDWR)==BO_WRONLY) n|=O_WRONLY;
	else if ((flags&BO_RDWR)==BO_RDONLY) n|=O_RDONLY;
	if (flags&BO_APPEND) n|=O_APPEND;
	if (flags&BO_CREAT) n|=O_CREAT;
	if (flags&BO_TRUNC) n|=O_TRUNC;
	if (mode&BS_IREAD) o|=S_IREAD;
	if (mode&BS_IWRITE) o|=S_IWRITE;
	if (mode&BS_IEXEC) o|=S_IEXEC;
	
	return open(pathname,n,o);
}

int Bclose(int fd)
{
	return close(fd);
}

bssize_t Bwrite(int fd, const void *buf, bsize_t count)
{
	return write(fd,buf,count);
}

bssize_t Bread(int fd, void *buf, bsize_t count)
{
	return read(fd,buf,count);
}

int Blseek(int fildes, int offset, int whence)
{
	switch (whence) {
		case BSEEK_SET: whence=SEEK_SET; break;
		case BSEEK_CUR: whence=SEEK_CUR; break;
		case BSEEK_END: whence=SEEK_END; break;
	}
	return lseek(fildes,offset,whence);
}

BFILE *Bfopen(const char *path, const char *mode)
{
	return (BFILE*)fopen(path,mode);
}

int Bfclose(BFILE *stream)
{
	return fclose((FILE*)stream);
}

void Brewind(BFILE *stream)
{
	rewind((FILE*)stream);
}

int Bfgetc(BFILE *stream)
{
	return fgetc((FILE*)stream);
}

char *Bfgets(char *s, int size, BFILE *stream)
{
	return fgets(s,size,(FILE*)stream);
}

int Bfputc(int c, BFILE *stream)
{
	return fputc(c,(FILE*)stream);
}

int Bfputs(const char *s, BFILE *stream)
{
	return fputs(s,(FILE*)stream);
}

bsize_t Bfread(void *ptr, bsize_t size, bsize_t nmemb, BFILE *stream)
{
	return fread(ptr,size,nmemb,(FILE*)stream);
}

bsize_t Bfwrite(const void *ptr, bsize_t size, bsize_t nmemb, BFILE *stream)
{
	return fwrite(ptr,size,nmemb,(FILE*)stream);
}


char *Bstrdup(const char *s)
{
	return strdup(s);
}

char *Bstrcpy(char *dest, const char *src)
{
	return strcpy(dest,src);
}

char *Bstrncpy(char *dest, const char *src, bsize_t n)
{
	return strncpy(dest,src,n);
}

int Bstrcmp(const char *s1, const char *s2)
{
	return strcmp(s1,s2);
}

int Bstrncmp(const char *s1, const char *s2, bsize_t n)
{
	return strncmp(s1,s2,n);
}

int Bstrcasecmp(const char *s1, const char *s2)
{
#ifdef _MSC_VER
	return stricmp(s1,s2);
#else
	return strcasecmp(s1,s2);
#endif
}

int Bstrncasecmp(const char *s1, const char *s2, bsize_t n)
{
#ifdef _MSC_VER
	return strnicmp(s1,s2,n);
#else
	return strncasecmp(s1,s2,n);
#endif
}

char *Bstrcat(char *dest, const char *src)
{
	return strcat(dest,src);
}

char *Bstrncat(char *dest, const char *src, bsize_t n)
{
	return strncat(dest,src,n);
}

bsize_t Bstrlen(const char *s)
{
	return strlen(s);
}

char *Bstrchr(const char *s, int c)
{
	return strchr(s,c);
}

char *Bstrrchr(const char *s, int c)
{
	return strrchr(s,c);
}

int Batoi(const char *nptr)
{
	return atoi(nptr);
}

long Batol(const char *nptr)
{
	return atol(nptr);
}

long int Bstrtol(const char *nptr, char **endptr, int base)
{
	return strtol(nptr,endptr,base);
}

unsigned long int Bstrtoul(const char *nptr, char **endptr, int base)
{
	return strtoul(nptr,endptr,base);
}

void *Bmemcpy(void *dest, const void *src, bsize_t n)
{
	return memcpy(dest,src,n);
}

void *Bmemmove(void *dest, const void *src, bsize_t n)
{
	return memmove(dest,src,n);
}

void *Bmemchr(const void *s, int c, bsize_t n)
{
	return memchr(s,c,n);
}

void *Bmemset(void *s, int c, bsize_t n)
{
	return memset(s,c,n);
}

int Bprintf(const char *format, ...)
{
	va_list ap;
	int r;

	va_start(ap,format);
#ifdef _MSC_VER
	r = _vprintf(format,ap);
#else
	r = vprintf(format,ap);
#endif
	va_end(ap);
	return r;
}

int Bsprintf(char *str, const char *format, ...)
{
	va_list ap;
	int r;

	va_start(ap,format);
#ifdef _MSC_VER
	r = _vsprintf(str,format,ap);
#else
	r = vsprintf(str,format,ap);
#endif
	va_end(ap);
	return r;
}

int Bsnprintf(char *str, bsize_t size, const char *format, ...)
{
	va_list ap;
	int r;

	va_start(ap,format);
#ifdef _MSC_VER
	r = _vsnprintf(str,size,format,ap);
#else
	r = vsnprintf(str,size,format,ap);
#endif
	va_end(ap);
	return r;
}

int Bvsnprintf(char *str, bsize_t size, const char *format, va_list ap)
{
#ifdef _MSC_VER
	return _vsnprintf(str,size,format,ap);
#else
	return vsnprintf(str,size,format,ap);
#endif
}

char *Bgetenv(const char *name)
{
	return getenv(name);
}

char *Bgetcwd(char *buf, bsize_t size)
{
	return getcwd(buf,size);
}

#endif	// __compat_h_macrodef__


//
// Stuff which must be a function
//

char *Bgethomedir(void)
{
#ifdef _WIN32
	TCHAR appdata[MAX_PATH];

//# if defined SHGetFolderPath
//	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata)))
//# if defined SHGetSpecialFolderPath
	if (SUCCEEDED(SHGetSpecialFolderPathA(NULL, appdata, CSIDL_APPDATA, FALSE)))
//# else
//#  error Cannot find SHGetFolderPath or SHGetSpecialFolderPath. Perhaps your shlobj.h is ancient?
//# endif
		return strdup(appdata);
	return NULL;
#else
	char *e = getenv("HOME");
	if (!e) return NULL;
	return strdup(e);
#endif
}

int Bcorrectfilename(char *filename, int removefn)
{
	char *fn;
	char *tokarr[64], *first, *next, *token;
	int i, ntok = 0, leadslash = 0, trailslash = 0;
	
	fn = strdup(filename);
	if (!fn) return -1;
	
	for (first=fn; *first; first++) {
#ifdef _WIN32
		if (*first == '\\') *first = '/';
#endif
	}
	leadslash = (*fn == '/');
	trailslash = (first>fn && first[-1] == '/');
	
	first = fn;
	do {
		token = Bstrtoken(first, "/", &next, 1);
		first = NULL;
		if (!token) break;
		else if (token[0] == 0) continue;
		else if (token[0] == '.' && token[1] == 0) continue;
		else if (token[0] == '.' && token[1] == '.' && token[2] == 0) ntok = max(0,ntok-1);
		else tokarr[ntok++] = token;
	} while (1);
	
	if (!trailslash && removefn) { ntok = max(0,ntok-1); trailslash = 1; }
	if (ntok == 0 && trailslash && leadslash) trailslash = 0;
	
	first = filename;
	if (leadslash) *(first++) = '/';
	for (i=0; i<ntok; i++) {
		if (i>0) *(first++) = '/';
		for (token=tokarr[i]; *token; token++)
			*(first++) = *token;
	}
	if (trailslash) *(first++) = '/';
	*(first++) = 0;

	return 0;
}

int Bcanonicalisefilename(char *filename, int removefn)
{
	char cwd[BMAX_PATH], fn[BMAX_PATH], *p;
	char *fnp = filename;
#ifdef _WIN32
	int drv = 0;
#endif
	
#ifdef _WIN32
	{
		if (filename[0] && filename[1] == ':') {
			// filename is prefixed with a drive
			drv = toupper(filename[0])-'A' + 1;
			fnp += 2;
		}
		if (!_getdcwd(drv, cwd, sizeof(cwd))) return -1;
		for (p=cwd; *p; p++) if (*p == '\\') *p = '/';
	}
#else
	if (!getcwd(cwd,sizeof(cwd))) return -1;
#endif
	p = strrchr(cwd,'/'); if (!p || p[1]) strcat(cwd, "/");
	
	strcpy(fn, fnp);
#ifdef _WIN32
	for (p=fn; *p; p++) if (*p == '\\') *p = '/';
#endif
	
	if (fn[0] != '/') {
		// we are dealing with a path relative to the current directory
		strcpy(filename, cwd);
		strcat(filename, fn);
	} else {
#ifdef _WIN32
		filename[0] = cwd[0];
		filename[1] = ':';
		filename[2] = 0;
#else
		filename[0] = 0;
#endif
		strcat(filename, fn);
	}
	fnp = filename;
#ifdef _WIN32
	fnp += 2;	// skip the drive
#endif
	
	return Bcorrectfilename(fnp,1);	
}

char *Bgetsystemdrives(void)
{
#ifdef _WIN32
	char *str, *p;
	DWORD drv, mask;
	int number=0;
	
	drv = GetLogicalDrives();
	if (drv == 0) return NULL;

	for (mask=1; mask<0x8000000l; mask<<=1) {
		if ((drv&mask) == 0) continue;
		number++;
	}

	str = p = (char *)malloc(1 + (3*number));
	if (!str) return NULL;

	number = 0;
	for (mask=1; mask<0x8000000l; mask<<=1, number++) {
		if ((drv&mask) == 0) continue;
		*(p++) = 'A' + number;
		*(p++) = ':';
		*(p++) = 0;
	}
	*(p++) = 0;

	return str;
#else
	// Perhaps have Unix OS's put /, /home/user, and /mnt/* in the "drives" list?
	return NULL;
#endif
}


long Bfilelength(int fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0) return -1;
	return(long)(st.st_size);
}


typedef struct {
#ifdef _MSC_VER
	long dir;
	struct _finddata_t fid;
#else
	DIR *dir;
#endif
	struct Bdirent info;
	int status;
	char name[1];
} BDIR_real;

BDIR* Bopendir(const char *name)
{
	BDIR_real *dirr;
#ifdef _MSC_VER
	char *t,*tt;
	t = (char*)malloc(strlen(name)+1+4);
	if (!t) return NULL;
#endif

	dirr = (BDIR_real*)malloc(sizeof(BDIR_real) + strlen(name));
	if (!dirr) {
#ifdef _MSC_VER
		free(t);
#endif
		return NULL;
	}

#ifdef _MSC_VER
	strcpy(t,name);
	tt = t+strlen(name)-1;
	while (*tt == ' ' && tt>t) tt--;
	if (*tt != '/' && *tt != '\\') *(++tt) = '/';
	*(++tt) = '*';
	*(++tt) = '.';
	*(++tt) = '*';
	*(++tt) = 0;
	
	dirr->dir = _findfirst(t,&dirr->fid);
	free(t);
	if (dirr->dir == -1) {
		free(dirr);
		return NULL;
	}
#else
	dirr->dir = opendir(name);
	if (dirr->dir == NULL) {
		free(dirr);
		return NULL;
	}
#endif

	dirr->status = 0;
	strcpy(dirr->name, name);
	
	return (BDIR*)dirr;
}

struct Bdirent*	Breaddir(BDIR *dir)
{
	BDIR_real *dirr = (BDIR_real*)dir;
	struct dirent *de;
	struct stat st;
	char *fn;

#ifdef _MSC_VER
	if (dirr->status > 0) {
		if (_findnext(dirr->dir,&dirr->fid) != 0) {
			dirr->status = -1;
			return NULL;
		}
	}
	dirr->info.namlen = strlen(dirr->fid.name);
	dirr->info.name = dirr->fid.name;
	dirr->status++;
#else
	de = readdir(dirr->dir);
	if (de == NULL) {
		dirr->status = -1;
		return NULL;
	} else {
		dirr->status++;
	}
//# if defined(__WATCOMC__) || defined(__linux) || defined(__BEOS__) || defined(__QNX__) || defined(SKYOS)
	dirr->info.namlen = strlen(de->d_name);
//# else
//	dirr->info.namlen = de->d_namlen;
//# endif
	dirr->info.name   = de->d_name;
#endif
	dirr->info.mode = 0;
	dirr->info.size = 0;
	dirr->info.mtime = 0;

	fn = (char *)malloc(strlen(dirr->name) + 1 + dirr->info.namlen + 1);
	if (fn) {
		sprintf(fn,"%s/%s",dirr->name,dirr->info.name);
		if (!stat(fn, &st)) {
			dirr->info.mode = st.st_mode;
			dirr->info.size = st.st_size;
			dirr->info.mtime = st.st_mtime;
		}
		free(fn);
	}

	return &dirr->info;
}

int Bclosedir(BDIR *dir)
{
	BDIR_real *dirr = (BDIR_real*)dir;
	
#ifdef _MSC_VER
	_findclose(dirr->dir);
#else
	closedir(dirr->dir);
#endif
	free(dirr);

	return 0;
}


char *Bstrtoken(char *s, char *delim, char **ptrptr, int chop)
{
	char *p, *start;

	if (!ptrptr) return NULL;
	
	if (s) p = s;
	else p = *ptrptr;

	if (!p) return NULL;

	while (*p != 0 && strchr(delim, *p)) p++;
	if (*p == 0) {
		*ptrptr = NULL;
		return NULL;
	}
	start = p;
	while (*p != 0 && !strchr(delim, *p)) p++;
	if (*p == 0) *ptrptr = NULL;
	else {
		if (chop) *(p++) = 0;
		*ptrptr = p;
	}

	return start;
}


	//Brute-force case-insensitive, slash-insensitive, * and ? wildcard matcher
	//Given: string i and string j. string j can have wildcards
	//Returns: 1:matches, 0:doesn't match
long Bwildmatch (const char *i, const char *j)
{
	const char *k;
	char c0, c1;

	if (!*j) return(1);
	do
	{
		if (*j == '*')
		{
			for(k=i,j++;*k;k++) if (Bwildmatch(k,j)) return(1);
			continue;
		}
		if (!*i) return(0);
		if (*j == '?') { i++; j++; continue; }
		c0 = *i; if ((c0 >= 'a') && (c0 <= 'z')) c0 -= 32;
		c1 = *j; if ((c1 >= 'a') && (c1 <= 'z')) c1 -= 32;
#ifdef _WIN32
		if (c0 == '/') c0 = '\\';
		if (c1 == '/') c1 = '\\';
#endif
		if (c0 != c1) return(0);
		i++; j++;
	} while (*j);
	return(!*i);
}

#if !defined(_WIN32)
char *Bstrlwr(char *s)
{
	char *t = s;
	if (!s) return s;
	while (*t) { *t = Btolower(*t); t++; }
	return s;
}

char *Bstrupr(char *s)
{
	char *t = s;
	if (!s) return s;
	while (*t) { *t = Btoupper(*t); t++; }
	return s;
}
#endif


//
// getsysmemsize() -- gets the amount of system memory in the machine
//
unsigned int Bgetsysmemsize(void)
{
#ifdef _WIN32
	MEMORYSTATUS memst;
	GlobalMemoryStatus(&memst);
	return (unsigned int)memst.dwTotalPhys;
#elif (defined(_SC_PAGE_SIZE) || defined(_SC_PAGESIZE)) && defined(_SC_PHYS_PAGES)
	unsigned int siz = 0x7fffffff;
	long scpagesiz, scphyspages;

#ifdef _SC_PAGE_SIZE
	scpagesiz = sysconf(_SC_PAGE_SIZE);
#else
	scpagesiz = sysconf(_SC_PAGESIZE);
#endif
	scphyspages = sysconf(_SC_PHYS_PAGES);
	if (scpagesiz >= 0 && scphyspages >= 0)
		siz = (unsigned int)min(longlong(0x7fffffff), (int64)scpagesiz * (int64)scphyspages);

	//initprintf("Bgetsysmemsize(): %d pages of %d bytes, %d bytes of system memory\n",
	//		scphyspages, scpagesiz, siz);

	return siz;
#else
	return 0x7fffffff;
#endif
}



