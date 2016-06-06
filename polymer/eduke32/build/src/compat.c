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

#include "compat.h"

#ifdef _WIN32
# include <shlobj.h>
# include <direct.h>
#elif __APPLE__
# include "osxbits.h"
#endif

#if defined(_MSC_VER)
# include <io.h>
#else
# include <dirent.h>
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

void handle_memerr(void)
{
    if (g_MemErrHandler)
    {
#ifdef DEBUGGINGAIDS
        g_MemErrHandler(g_MemErrLine, g_MemErrFile, g_MemErrFunc);
#else
        g_MemErrHandler(0, "???", "???");
#endif
    }

    Bexit(EXIT_FAILURE);
}

void set_memerr_handler(void(*handlerfunc)(int32_t, const char *, const char *))
{
    g_MemErrHandler = handlerfunc;
}

//////////

#ifndef compat_h_macrodef__

void Bassert(int expr) { assert(expr); }
int32_t Brand(void) { return rand(); }
void *Bmalloc(bsize_t size) { return malloc(size); }
void Bfree(void *ptr) { free(ptr); }

int32_t Bopen(const char *pathname, int32_t flags, uint32_t mode)
{
    int32_t n = 0, o = 0;

    if (flags & BO_BINARY)
        n |= O_BINARY;
    else
        n |= O_TEXT;
    if ((flags & BO_RDWR) == BO_RDWR)
        n |= O_RDWR;
    else if ((flags & BO_RDWR) == BO_WRONLY)
        n |= O_WRONLY;
    else if ((flags & BO_RDWR) == BO_RDONLY)
        n |= O_RDONLY;
    if (flags & BO_APPEND)
        n |= O_APPEND;
    if (flags & BO_CREAT)
        n |= O_CREAT;
    if (flags & BO_TRUNC)
        n |= O_TRUNC;
    if (mode & BS_IREAD)
        o |= S_IREAD;
    if (mode & BS_IWRITE)
        o |= S_IWRITE;
    if (mode & BS_IEXEC)
        o |= S_IEXEC;

    return open(pathname, n, o);
}

int32_t Bclose(int32_t fd) { return close(fd); }
bssize_t Bwrite(int32_t fd, const void *buf, bsize_t count) { return write(fd, buf, count); }
bssize_t Bread(int32_t fd, void *buf, bsize_t count) { return read(fd, buf, count); }

int32_t Blseek(int32_t fildes, int32_t offset, int32_t whence)
{
    switch (whence)
    {
        case BSEEK_SET: whence = SEEK_SET; break;
        case BSEEK_CUR: whence = SEEK_CUR; break;
        case BSEEK_END: whence = SEEK_END; break;
    }
    return lseek(fildes, offset, whence);
}

BFILE *Bfopen(const char *path, const char *mode) { return (BFILE *)fopen(path, mode); }
int32_t Bfclose(BFILE *stream) { return fclose((FILE *)stream); }
void Brewind(BFILE *stream) { rewind((FILE *)stream); }
int32_t Bfgetc(BFILE *stream) { return fgetc((FILE *)stream); }
char *Bfgets(char *s, int32_t size, BFILE *stream) { return fgets(s, size, (FILE *)stream); }
int32_t Bfputc(int32_t c, BFILE *stream) { return fputc(c, (FILE *)stream); }
int32_t Bfputs(const char *s, BFILE *stream) { return fputs(s, (FILE *)stream); }
bsize_t Bfread(void *ptr, bsize_t size, bsize_t nmemb, BFILE *stream) { return fread(ptr, size, nmemb, (FILE *)stream); }
bsize_t Bfwrite(const void *ptr, bsize_t size, bsize_t nmemb, BFILE *stream) { return fwrite(ptr, size, nmemb, (FILE *)stream); }
char *Bstrdup(const char *s) { return strdup(s); }
char *Bstrcpy(char *dest, const char *src) { return strcpy(dest, src); }
char *Bstrncpy(char *dest, const char *src, bsize_t n) { return Bstrncpy(dest, src, n); }
int32_t Bstrcmp(const char *s1, const char *s2) { return strcmp(s1, s2); }
int32_t Bstrncmp(const char *s1, const char *s2, bsize_t n) { return strncmp(s1, s2, n); }

int32_t Bstrcasecmp(const char *s1, const char *s2)
{
#ifdef _MSC_VER
    return _stricmp(s1, s2);
#else
    return strcasecmp(s1, s2);
#endif
}

int32_t Bstrncasecmp(const char *s1, const char *s2, bsize_t n)
{
#ifdef _MSC_VER
    return _strnicmp(s1, s2, n);
#else
    return strncasecmp(s1, s2, n);
#endif
}

char *Bstrcat(char *dest, const char *src) { return strcat(dest, src); }
char *Bstrncat(char *dest, const char *src, bsize_t n) { return strncat(dest, src, n); }
bsize_t Bstrlen(const char *s) { return strlen(s); }
char *Bstrchr(const char *s, int32_t c) { return strchr(s, c); }
char *Bstrrchr(const char *s, int32_t c) { return strrchr(s, c); }
int32_t Batoi(const char *nptr) { return strtol(nptr, NULL, 10); }
int32_t Batol(const char *nptr) { return strtol(nptr, NULL, 10); }
int32_t Bstrtol(const char *nptr, char **endptr, int32_t base) { return strtol(nptr, endptr, base); }
uint32_t Bstrtoul(const char *nptr, char **endptr, int32_t base) { return strtoul(nptr, endptr, base); }
void *Bmemcpy(void *dest, const void *src, bsize_t n) { return memcpy(dest, src, n); }
void *Bmemmove(void *dest, const void *src, bsize_t n) { return memmove(dest, src, n); }
void *Bmemchr(const void *s, int32_t c, bsize_t n) { return memchr(s, c, n); }
void *Bmemset(void *s, int32_t c, bsize_t n) { return memset(s, c, n); }

int32_t Bprintf(const char *format, ...)
{
    va_list ap;
    int32_t r;

    va_start(ap, format);
#ifdef _MSC_VER
    r = _vprintf(format, ap);
#else
    r = vprintf(format, ap);
#endif
    va_end(ap);
    return r;
}

int32_t Bsprintf(char *str, const char *format, ...)
{
    va_list ap;
    int32_t r;

    va_start(ap, format);
#ifdef _MSC_VER
    r = _vsprintf(str, format, ap);
#else
    r = vsprintf(str, format, ap);
#endif
    va_end(ap);
    return r;
}

int32_t Bsnprintf(char *str, bsize_t size, const char *format, ...)
{
    va_list ap;
    int32_t r;

    va_start(ap, format);
#ifdef _MSC_VER
    r = _vsnprintf(str, size, format, ap);
#else
    r = vsnprintf(str, size, format, ap);
#endif
    va_end(ap);
    return r;
}

int32_t Bvsnprintf(char *str, bsize_t size, const char *format, va_list ap)
{
#ifdef _MSC_VER
    return _vsnprintf(str, size, format, ap);
#else
    return vsnprintf(str, size, format, ap);
#endif
}

char *Bgetenv(const char *name) { return getenv(name); }
char *Bgetcwd(char *buf, bsize_t size) { return getcwd(buf, size); }

#endif	// __compat_h_macrodef__


//
// Stuff which must be a function
//
#ifdef _WIN32
typedef BOOL (WINAPI * aSHGetSpecialFolderPathAtype)(HWND, LPTSTR, int, BOOL);
#endif

char *Bgethomedir(void)
{
#ifdef _WIN32
    aSHGetSpecialFolderPathAtype aSHGetSpecialFolderPathA;
    TCHAR appdata[MAX_PATH];
    int32_t loaded = 0;
    HMODULE hShell32 = GetModuleHandle("shell32.dll");

    if (hShell32 == NULL)
    {
        hShell32 = LoadLibrary("shell32.dll");
        loaded = 1;
    }

    if (hShell32 == NULL)
        return NULL;

    aSHGetSpecialFolderPathA = (aSHGetSpecialFolderPathAtype)GetProcAddress(hShell32, "SHGetSpecialFolderPathA");
    if (aSHGetSpecialFolderPathA != NULL)
        if (SUCCEEDED(aSHGetSpecialFolderPathA(NULL, appdata, CSIDL_APPDATA, FALSE)))
        {
            if (loaded)
                FreeLibrary(hShell32);
            return Bstrdup(appdata);
        }

    if (loaded)
        FreeLibrary(hShell32);
    return NULL;
#elif defined EDUKE32_OSX
    return osx_gethomedir();
#elif defined(GEKKO)
    // return current drive's name
    char *drv, cwd[BMAX_PATH] = {0};
    getcwd(cwd, BMAX_PATH);
    drv = strchr(cwd, ':');
    if (drv)
        drv[1] = '\0';
    return Bstrdup(cwd);
#else
    char *e = getenv("HOME");
    if (!e) return NULL;
    return Bstrdup(e);
#endif
}

char *Bgetappdir(void)
{
    char *dir = NULL;

#ifdef _WIN32
	TCHAR appdir[MAX_PATH];

	if (GetModuleFileName(NULL, appdir, MAX_PATH) > 0) {
		// trim off the filename
		char *slash = Bstrrchr(appdir, '\\');
		if (slash) slash[0] = 0;
		dir = Bstrdup(appdir);
    }

#elif defined EDUKE32_OSX
    dir = osx_getappdir();
#elif defined __FreeBSD__
    // the sysctl should also work when /proc/ is not mounted (which seems to
    // be common on FreeBSD), so use it..
    char buf[PATH_MAX] = {0};
    int name[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    size_t len = sizeof(buf)-1;
    int ret = sysctl(name, sizeof(name)/sizeof(name[0]), buf, &len, NULL, 0);
    if(ret == 0 && buf[0] != '\0') {
        // again, remove executable name with dirname()
        // on FreeBSD dirname() seems to use some internal buffer
        dir = strdup(dirname(buf));
    }
#elif defined __linux || defined EDUKE32_BSD
    char buf[PATH_MAX] = {0};
    char buf2[PATH_MAX] = {0};
#  ifdef __linux
    Bsnprintf(buf, sizeof(buf), "/proc/%d/exe", getpid());
#  else // the BSDs.. except for FreeBSD which has a sysctl
    Bsnprintf(buf, sizeof(buf), "/proc/%d/file", getpid());
#  endif
    int len = readlink(buf, buf2, sizeof(buf2));
    if (len != -1) {
        // remove executable name with dirname(3)
        // on Linux, dirname() will modify buf2 (cutting off executable name) and return it
        // on FreeBSD it seems to use some internal buffer instead.. anyway, just strdup()
        dir = Bstrdup(dirname(buf2));
    }
#endif

    return dir;
}

int32_t Bcorrectfilename(char *filename, int32_t removefn)
{
    char *fn;
    char *tokarr[64], *first, *next = NULL, *token;
    int32_t i, ntok = 0, leadslash = 0, trailslash = 0;

    fn = Bstrdup(filename);
    if (!fn) return -1;

    for (first=fn; *first; first++)
    {
#ifdef _WIN32
        if (*first == '\\') *first = '/';
#endif
    }
    leadslash = (*fn == '/');
    trailslash = (first>fn && first[-1] == '/');

    first = fn;
    do
    {
        token = Bstrtoken(first, "/", &next, 1);
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
    for (i=0; i<ntok; i++)
    {
        if (i>0) *(first++) = '/';
        for (token=tokarr[i]; *token; token++)
            *(first++) = *token;
    }
    if (trailslash) *(first++) = '/';
    *(first++) = 0;

    Bfree(fn);
    return 0;
}

int32_t Bcanonicalisefilename(char *filename, int32_t removefn)
{
    char cwd[BMAX_PATH];
    char *fnp = filename;

#ifdef _WIN32
    int drv = 0;

    if (filename[0] && filename[1] == ':')
    {
        // filename is prefixed with a drive
        drv = toupper(filename[0]) - 'A' + 1;
        fnp += 2;
    }

    if (!_getdcwd(drv, cwd, sizeof(cwd)))
        return -1;

    for (char *p = cwd; *p; p++)
        if (*p == '\\')
            *p = '/';
#else
    if (!getcwd(cwd, sizeof(cwd)))
        return -1;
#endif

    char *p = Bstrrchr(cwd, '/');
    if (!p || p[1])
        Bstrcat(cwd, "/");

    char fn[BMAX_PATH];
    Bstrcpy(fn, fnp);

#ifdef _WIN32
    for (p = fn; *p; p++)
        if (*p == '\\')
            *p = '/';
#endif

    if (fn[0] != '/')
    {
        // we are dealing with a path relative to the current directory
        Bstrcpy(filename, cwd);
        Bstrcat(filename, fn);
    }
    else
    {
#ifdef _WIN32
        filename[0] = cwd[0];
        filename[1] = ':';
        filename[2] = 0;
        Bstrcat(filename, fn);
#else
        Bstrcpy(filename, fn);
#endif
    }
    fnp = filename;
#ifdef _WIN32
    fnp += 2;  // skip the drive
#endif
    UNREFERENCED_PARAMETER(removefn);  // change the call below to use removefn instead of 1?
    return Bcorrectfilename(fnp, 1);
}

char *Bgetsystemdrives(void)
{
#ifdef _WIN32
    char *str, *p;
    DWORD drv, mask;
    int32_t number = 0;

    drv = GetLogicalDrives();
    if (drv == 0)
        return NULL;

    for (mask = 1; mask < 0x8000000l; mask <<= 1)
    {
        if ((drv & mask) == 0)
            continue;
        number++;
    }

    str = p = (char *)Bmalloc(1 + (3 * number));
    if (!str)
        return NULL;

    number = 0;
    for (mask = 1; mask < 0x8000000l; mask <<= 1, number++)
    {
        if ((drv & mask) == 0)
            continue;
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


int32_t Bfilelength(int32_t fd)
{
    struct Bstat st;
    return (Bfstat(fd, &st) < 0) ? -1 : (int32_t)(st.st_size);
}


typedef struct
{
#ifdef _MSC_VER
    intptr_t dir;
    struct _finddata_t fid;
#else
    DIR *dir;
#endif
    struct Bdirent info;
    int32_t status;
    char name[1];
} BDIR_real;

BDIR *Bopendir(const char *name)
{
    BDIR_real *dirr;
#ifdef _MSC_VER
    char *t, *tt;
    t = (char *)Bmalloc(Bstrlen(name) + 1 + 4);
    if (!t)
        return NULL;
#endif

    dirr = (BDIR_real *)Bmalloc(sizeof(BDIR_real) + Bstrlen(name));
    if (!dirr)
    {
#ifdef _MSC_VER
        Bfree(t);
#endif
        return NULL;
    }

#ifdef _MSC_VER
    Bstrcpy(t, name);
    tt = t + Bstrlen(name) - 1;
    while (*tt == ' ' && tt > t) tt--;
    if (*tt != '/' && *tt != '\\')
        *(++tt) = '/';
    *(++tt) = '*';
    *(++tt) = '.';
    *(++tt) = '*';
    *(++tt) = 0;

    dirr->dir = _findfirst(t, &dirr->fid);
    Bfree(t);
    if (dirr->dir == -1)
    {
        Bfree(dirr);
        return NULL;
    }
#else
    dirr->dir = opendir(name);
    if (dirr->dir == NULL)
    {
        Bfree(dirr);
        return NULL;
    }
#endif

    dirr->status = 0;
    Bstrcpy(dirr->name, name);

    return (BDIR *)dirr;
}

struct Bdirent *Breaddir(BDIR *dir)
{
    BDIR_real *dirr = (BDIR_real *)dir;

#ifdef _MSC_VER
    if (dirr->status > 0)
    {
        if (_findnext(dirr->dir, &dirr->fid) != 0)
        {
            dirr->status = -1;
            return NULL;
        }
    }
    dirr->info.namlen = Bstrlen(dirr->fid.name);
    dirr->info.name = dirr->fid.name;
    dirr->status++;
#else
    struct dirent *de = readdir(dirr->dir);
    if (de == NULL)
    {
        dirr->status = -1;
        return NULL;
    }
    else
    {
        dirr->status++;
    }
    dirr->info.namlen = Bstrlen(de->d_name);
    dirr->info.name = de->d_name;
#endif
    dirr->info.mode = 0;
    dirr->info.size = 0;
    dirr->info.mtime = 0;

    char *fn = (char *)Bmalloc(Bstrlen(dirr->name) + 1 + dirr->info.namlen + 1);
    if (fn)
    {
        Bsprintf(fn, "%s/%s", dirr->name, dirr->info.name);
        struct Bstat st;
        if (!Bstat(fn, &st))
        {
            dirr->info.mode = st.st_mode;
            dirr->info.size = st.st_size;
            dirr->info.mtime = st.st_mtime;
        }
        Bfree(fn);
    }

    return &dirr->info;
}

int32_t Bclosedir(BDIR *dir)
{
    BDIR_real *dirr = (BDIR_real *)dir;

#ifdef _MSC_VER
    _findclose(dirr->dir);
#else
    closedir(dirr->dir);
#endif
    Bfree(dirr);

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


//Brute-force case-insensitive, slash-insensitive, * and ? wildcard matcher
//Returns: 1:matches, 0:doesn't match
#ifndef WITHKPLIB
extern char toupperlookup[256];

static int32_t wildmatch(const char *match, const char *wild)
{
    do
    {
        if (*match && (toupperlookup[*wild] == toupperlookup[*match] || *wild == '?'))
        {
            wild++, match++;
            continue;
        }
        else if ((*match|*wild) == '\0')
            return 1;
        else if (*wild == '*')
        {
            do { wild++; } while (*wild == '*');
            do
            {
                if (*wild == '\0')
                    return 1;
                while (*match && toupperlookup[*match] != toupperlookup[*wild]) match++;
                if (*match && *(match+1) && toupperlookup[*(match+1)] != toupperlookup[*(wild+1)])
                {
                    match++;
                    continue;
                }
                break;
            }
            while (1);
            if (toupperlookup[*match] == toupperlookup[*wild])
                continue;
        }
        return 0;
    }
    while (1);
}
#endif

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


//
// Bgetsysmemsize() -- gets the amount of system memory in the machine
//
#ifdef _WIN32
typedef BOOL (WINAPI *aGlobalMemoryStatusExType)(LPMEMORYSTATUSEX);
#endif

uint32_t Bgetsysmemsize(void)
{
#ifdef _WIN32
    uint32_t siz = UINT32_MAX;
    HMODULE lib = LoadLibrary("KERNEL32.DLL");

    if (lib)
    {
        aGlobalMemoryStatusExType aGlobalMemoryStatusEx =
            (aGlobalMemoryStatusExType)GetProcAddress(lib, "GlobalMemoryStatusEx");

        if (aGlobalMemoryStatusEx)
        {
            //WinNT
            MEMORYSTATUSEX memst;
            memst.dwLength = sizeof(MEMORYSTATUSEX);
            if (aGlobalMemoryStatusEx(&memst))
                siz = (uint32_t)min(UINT32_MAX, memst.ullTotalPhys);
        }
        else
        {
            // Yeah, there's enough Win9x hatred here that a perfectly good workaround
            // has been replaced by an error message.  Oh well, we don't support 9x anyway.
            initprintf("Bgetsysmemsize(): error determining system memory size!\n");
        }

        FreeLibrary(lib);
    }

    return siz;
#elif (defined(_SC_PAGE_SIZE) || defined(_SC_PAGESIZE)) && defined(_SC_PHYS_PAGES) && !defined(GEKKO)
    uint32_t siz = UINT32_MAX;
    int64_t scpagesiz, scphyspages;

#ifdef _SC_PAGE_SIZE
    scpagesiz = sysconf(_SC_PAGE_SIZE);
#else
    scpagesiz = sysconf(_SC_PAGESIZE);
#endif
    scphyspages = sysconf(_SC_PHYS_PAGES);
    if (scpagesiz >= 0 && scphyspages >= 0)
        siz = (uint32_t)min(UINT32_MAX, (int64_t)scpagesiz * (int64_t)scphyspages);

    //initprintf("Bgetsysmemsize(): %d pages of %d bytes, %d bytes of system memory\n",
    //		scphyspages, scpagesiz, siz);

    return siz;
#else
    return UINT32_MAX;
#endif
}

#ifdef GEKKO
int access(const char *pathname, int mode)
{
    struct stat st;
    if (stat(pathname, &st)==-1)
        return -1;

    // TODO: Check mode against st_mode
    UNREFERENCED_PARAMETER(mode);

    return 0;
}
#endif

#define LIBDIVIDE_BODY
#include "libdivide.h"

