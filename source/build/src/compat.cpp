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

#include "vfs.h"

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
    int32_t loaded = 0;
    auto hShell32 = GetModuleHandle("shell32.dll");

    if (hShell32 == NULL)
    {
        hShell32 = LoadLibrary("shell32.dll");
        loaded = 1;
    }

    if (hShell32 == NULL)
        return NULL;

    using SHGSFPA_t = BOOL (WINAPI *)(HWND, LPTSTR, int, BOOL);
    auto aSHGetSpecialFolderPathA = (SHGSFPA_t)(void (*)(void))GetProcAddress(hShell32, "SHGetSpecialFolderPathA");

    if (aSHGetSpecialFolderPathA != NULL)
    {
        TCHAR appdata[MAX_PATH];

        if (SUCCEEDED(aSHGetSpecialFolderPathA(NULL, appdata, CSIDL_APPDATA, FALSE)))
        {
            if (loaded)
                FreeLibrary(hShell32);
            return Xstrdup(appdata);
        }
    }

    if (loaded)
        FreeLibrary(hShell32);
    return NULL;
#elif defined EDUKE32_OSX
    return osx_gethomedir();
#elif defined(GEKKO)
    // return current drive's name
    char *drv, cwd[BMAX_PATH] = {0};
    buildvfs_getcwd(cwd, BMAX_PATH);
    drv = strchr(cwd, ':');
    if (drv)
        drv[1] = '\0';
    return Xstrdup(cwd);
#else
    char *e = getenv("HOME");
    if (!e) return NULL;
    return Xstrdup(e);
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
        dir = Xstrdup(appdir);
    }

#elif defined EDUKE32_OSX
    dir = osx_getappdir();
#elif defined __FreeBSD__
    // the sysctl should also work when /proc/ is not mounted (which seems to
    // be common on FreeBSD), so use it..
    char   buf[PATH_MAX] = {0};
    int    name[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    size_t len     = sizeof(buf) - 1;
    int    ret     = sysctl(name, ARRAY_SIZE(name), buf, &len, NULL, 0);

    if (ret == 0 && buf[0] != '\0')
    {
        // again, remove executable name with dirname()
        // on FreeBSD dirname() seems to use some internal buffer
        dir = Xstrdup(dirname(buf));
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
        dir = Xstrdup(dirname(buf2));
    }
#endif

    return dir;
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

#ifndef USE_PHYSFS
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
    if (!buildvfs_getcwd(cwd, sizeof(cwd)))
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
#endif

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

    str = p = (char *)Xmalloc(1 + (3 * number));
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


#ifndef USE_PHYSFS
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
    t = (char *)Xmalloc(Bstrlen(name) + 1 + 4);
#endif

    dirr = (BDIR_real *)Xmalloc(sizeof(BDIR_real) + Bstrlen(name));

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
    Xfree(t);
    if (dirr->dir == -1)
    {
        Xfree(dirr);
        return NULL;
    }
#else
    dirr->dir = opendir(name);
    if (dirr->dir == NULL)
    {
        Xfree(dirr);
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

    char *fn = (char *)Xmalloc(Bstrlen(dirr->name) + 1 + dirr->info.namlen + 1);
    Bsprintf(fn, "%s/%s", dirr->name, dirr->info.name);

#ifdef USE_PHYSFS
    PHYSFS_Stat st;
    if (PHYSFS_stat(fn, &st))
    {
        // dirr->info.mode = TODO;
        dirr->info.size = st.filesize;
        dirr->info.mtime = st.modtime;
    }
#else
    struct Bstat st;
    if (!Bstat(fn, &st))
    {
        dirr->info.mode = st.st_mode;
        dirr->info.size = st.st_size;
        dirr->info.mtime = st.st_mtime;
    }
#endif

    Xfree(fn);

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
    Xfree(dirr);

    return 0;
}
#endif


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

#define BMAXPAGESIZE 16384

int Bgetpagesize(void)
{
    static int pageSize = -1;

    if (pageSize == -1)
    {
#ifdef _WIN32
        SYSTEM_INFO system_info;
        GetSystemInfo(&system_info);
        pageSize = system_info.dwPageSize;
#else
        pageSize = sysconf(_SC_PAGESIZE);
#endif
    }

    return (unsigned)pageSize < BMAXPAGESIZE ? pageSize : BMAXPAGESIZE;
}

//
// Bgetsysmemsize() -- gets the amount of system memory in the machine
//
#ifdef _WIN32
typedef BOOL (WINAPI *aGlobalMemoryStatusExType)(LPMEMORYSTATUSEX);
#endif

uint32_t Bgetsysmemsize(void)
{
    uint32_t siz = UINT32_MAX;

#ifdef _WIN32
    HMODULE lib = LoadLibrary("KERNEL32.DLL");

    if (lib)
    {
        aGlobalMemoryStatusExType aGlobalMemoryStatusEx =
            (aGlobalMemoryStatusExType)(void (*)(void))GetProcAddress(lib, "GlobalMemoryStatusEx");

        if (aGlobalMemoryStatusEx)
        {
            //WinNT
            MEMORYSTATUSEX memst;
            memst.dwLength = sizeof(MEMORYSTATUSEX);
            if (aGlobalMemoryStatusEx(&memst))
                siz = min<decltype(memst.ullTotalPhys)>(UINT32_MAX, memst.ullTotalPhys);
        }

        if (!aGlobalMemoryStatusEx || siz == 0)
        {
            initprintf("Bgetsysmemsize(): error determining system memory size!\n");
            siz = UINT32_MAX;
        }

        FreeLibrary(lib);
    }
    else initprintf("Bgetsysmemsize(): unable to load KERNEL32.DLL!\n");
#elif (defined(_SC_PAGE_SIZE) || defined(_SC_PAGESIZE)) && defined(_SC_PHYS_PAGES) && !defined(GEKKO)
#ifdef _SC_PAGE_SIZE
    int64_t const scpagesiz = sysconf(_SC_PAGE_SIZE);
#else
    int64_t const scpagesiz = sysconf(_SC_PAGESIZE);
#endif
    int64_t const scphyspages = sysconf(_SC_PHYS_PAGES);

    if (scpagesiz >= 0 && scphyspages >= 0)
        siz = (uint32_t)min<uint64_t>(UINT32_MAX, scpagesiz * scphyspages);

    //initprintf("Bgetsysmemsize(): %d pages of %d bytes, %d bytes of system memory\n",
    //		scphyspages, scpagesiz, siz);

#endif

    return siz;
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
