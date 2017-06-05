
#include "compat.h"
#include "build.h"
#include "scriptfile.h"
#include "cache1d.h"
#include "kplib.h"
#include "baselayer.h"

#include "common.h"

void PrintBuildInfo(void)
{
    buildprint(
        "Built ", s_buildTimestamp, ", "

#if defined __INTEL_COMPILER
        "ICC ", __INTEL_COMPILER / 100, ".", __INTEL_COMPILER % 100, " " __INTEL_COMPILER_BUILD_DATE " (" __VERSION__ ")"
#elif defined __clang__
        "clang "
# ifdef DEBUGGINGAIDS
        __clang_version__
# else
        , __clang_major__, ".", __clang_minor__, ".", __clang_patchlevel__,
# endif
#elif defined _MSC_VER
        "MSVC ",
# if defined _MSC_FULL_VER
            _MSC_FULL_VER / 10000000, ".", _MSC_FULL_VER % 10000000 / 100000, ".", _MSC_FULL_VER % 100000, ".", _MSC_BUILD,
# else
            _MSC_VER / 100, ".", _MSC_VER % 100,
# endif
#elif defined __GNUC__
        "GCC "
# ifdef DEBUGGINGAIDS
            __VERSION__
# else
            , __GNUC__, ".", __GNUC_MINOR__,
#  if defined __GNUC_PATCHLEVEL__
            ".", __GNUC_PATCHLEVEL__,
#  endif
# endif
#else
        "Unknown"
#endif
        ", "
#ifdef BITNESS64
        "64"
#else
        "32"
#endif
        "-bit "
#if B_BIG_ENDIAN == 1
        "big-endian"
#endif
        "\n");

    // TODO: architecture, OS, maybe build and feature settings
}

// def/clipmap handling

// g_defNamePtr can ONLY point to a malloc'd block (length BMAX_PATH)
char *g_defNamePtr = NULL;

void clearDefNamePtr(void)
{
    Bfree(g_defNamePtr);
    // g_defNamePtr assumed to be assigned to right after
}

char **g_defModules = NULL;
int32_t g_defModulesNum = 0;

#ifdef HAVE_CLIPSHAPE_FEATURE
char **g_clipMapFiles = NULL;
int32_t g_clipMapFilesNum = 0;
#endif

void G_AddDef(const char *buffer)
{
    clearDefNamePtr();
    g_defNamePtr = dup_filename(buffer);
    initprintf("Using DEF file \"%s\".\n",g_defNamePtr);
}

void G_AddDefModule(const char *buffer)
{
    g_defModules = (char **) Xrealloc (g_defModules, (g_defModulesNum+1) * sizeof(char *));
    g_defModules[g_defModulesNum] = Xstrdup(buffer);
    ++g_defModulesNum;
}

#ifdef HAVE_CLIPSHAPE_FEATURE
void G_AddClipMap(const char *buffer)
{
    g_clipMapFiles = (char **) Xrealloc (g_clipMapFiles, (g_clipMapFilesNum+1) * sizeof(char *));
    g_clipMapFiles[g_clipMapFilesNum] = Xstrdup(buffer);
    ++g_clipMapFilesNum;
}
#endif

//////////

int32_t getatoken(scriptfile *sf, const tokenlist *tl, int32_t ntokens)
{
    char *tok;
    int32_t i;

    if (!sf) return T_ERROR;
    tok = scriptfile_gettoken(sf);
    if (!tok) return T_EOF;

    for (i=ntokens-1; i>=0; i--)
    {
        if (!Bstrcasecmp(tok, tl[i].text))
            return tl[i].tokenid;
    }
    return T_ERROR;
}

//////////

int32_t G_CheckCmdSwitch(int32_t argc, char const * const * argv, const char *str)
{
    int32_t i;
    for (i=0; i<argc; i++)
    {
        if (str && !Bstrcasecmp(argv[i], str))
            return 1;
    }

    return 0;
}

// returns: 1 if file could be opened, 0 else
int32_t testkopen(const char *filename, char searchfirst)
{
    int32_t fd = kopen4load(filename, searchfirst);
    if (fd >= 0)
        kclose(fd);
    return (fd >= 0);
}

// checks from path and in ZIPs, returns 1 if NOT found
int32_t check_file_exist(const char *fn)
{
    int32_t opsm = pathsearchmode;
    char *tfn;

    pathsearchmode = 1;
    if (findfrompath(fn,&tfn) < 0)
    {
        char buf[BMAX_PATH];

        Bstrcpy(buf,fn);
        kzfindfilestart(buf);
        if (!kzfindfile(buf))
        {
            initprintf("Error: file \"%s\" does not exist\n",fn);
            pathsearchmode = opsm;
            return 1;
        }
    }
    else Bfree(tfn);
    pathsearchmode = opsm;

    return 0;
}


//// FILE NAME / DIRECTORY LISTS ////
void fnlist_clearnames(fnlist_t *fnl)
{
    klistfree(fnl->finddirs);
    klistfree(fnl->findfiles);

    fnl->finddirs = fnl->findfiles = NULL;
    fnl->numfiles = fnl->numdirs = 0;
}

// dirflags, fileflags:
//  -1 means "don't get dirs/files",
//  otherwise ORed to flags for respective klistpath
int32_t fnlist_getnames(fnlist_t *fnl, const char *dirname, const char *pattern,
                        int32_t dirflags, int32_t fileflags)
{
    CACHE1D_FIND_REC *r;

    fnlist_clearnames(fnl);

    if (dirflags != -1)
        fnl->finddirs = klistpath(dirname, "*", CACHE1D_FIND_DIR|dirflags);
    if (fileflags != -1)
        fnl->findfiles = klistpath(dirname, pattern, CACHE1D_FIND_FILE|fileflags);

    for (r=fnl->finddirs; r; r=r->next)
        fnl->numdirs++;
    for (r=fnl->findfiles; r; r=r->next)
        fnl->numfiles++;

    return 0;
}


////

// Copy FN to WBUF and append an extension if it's not there, which is checked
// case-insensitively.
// Returns: 1 if not all characters could be written to WBUF, 0 else.
int32_t maybe_append_ext(char *wbuf, int32_t wbufsiz, const char *fn, const char *ext)
{
    const int32_t slen=Bstrlen(fn), extslen=Bstrlen(ext);
    const int32_t haveext = (slen>=extslen && Bstrcasecmp(&fn[slen-extslen], ext)==0);

    Bassert((intptr_t)wbuf != (intptr_t)fn);  // no aliasing

    // If 'fn' has no extension suffixed, append one.
    return (Bsnprintf(wbuf, wbufsiz, "%s%s", fn, haveext ? "" : ext) >= wbufsiz);
}


int32_t ldist(const void *s1, const void *s2)
{
    vec2_t const *const sp1 = (vec2_t const *)s1;
    vec2_t const *const sp2 = (vec2_t const *)s2;
    return sepldist(sp1->x - sp2->x, sp1->y - sp2->y);
}

int32_t dist(const void *s1, const void *s2)
{
    vec3_t const *const sp1 = (vec3_t const *)s1;
    vec3_t const *const sp2 = (vec3_t const *)s2;
    return sepdist(sp1->x - sp2->x, sp1->y - sp2->y, sp1->z - sp2->z);
}

int32_t FindDistance2D(int32_t x, int32_t y)
{
    return sepldist(x, y);
}

int32_t FindDistance3D(int32_t x, int32_t y, int32_t z)
{
    return sepdist(x, y, z);
}


// Clear OSD background
void COMMON_clearbackground(int32_t numcols, int32_t numrows)
{
    UNREFERENCED_PARAMETER(numcols);

# ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST && qsetmode==200)
    {
//        bglPushAttrib(GL_FOG_BIT);
        bglDisable(GL_FOG);

        setpolymost2dview();
        bglColor4f(0.f, 0.f, 0.f, 0.67f);
        bglEnable(GL_BLEND);
        bglRecti(0, 0, xdim, 8*numrows+8);
        bglColor4f(0.f, 0.f, 0.f, 1.f);
        bglRecti(0, 8*numrows+4, xdim, 8*numrows+8);

//        bglPopAttrib();
        bglEnable(GL_FOG);

        return;
    }
# endif

    CLEARLINES2D(0, min(ydim, numrows*8+8), editorcolors[16]);
}
