
#include "compat.h"
#include "build.h"
#include "scriptfile.h"
#include "cache1d.h"
#include "kplib.h"
#include "baselayer.h"

#include "common.h"

const char* s_buildInfo =
#ifdef BITNESS64
        "(64-bit)"
#else
        "(32-bit)"
#endif
#if defined (_MSC_VER) || defined(__cplusplus)
#ifdef _MSC_VER
        " MSVC"
#endif
#ifdef __cplusplus
        " C++"
#endif
        " build"
#endif
;

// def/clipmap handling

// g_defNamePtr can ONLY point to a malloc'd block (length BMAX_PATH)
char *g_defNamePtr = NULL;

void clearDefNamePtr(void)
{
    if (g_defNamePtr != NULL)
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

int32_t G_CheckCmdSwitch(int32_t argc, const char **argv, const char *str)
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

    return(0);
}


////

// returns a buffer of size BMAX_PATH
char *dup_filename(const char *fn)
{
    char *buf = (char *)Xmalloc(BMAX_PATH);

    return Bstrncpyz(buf, fn, BMAX_PATH);
}


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


// Approximations to 2D and 3D Euclidean distances. Initial EDuke32 SVN import says
// in jmact/mathutil.c: "Ken's reverse-engineering job".
// Note that jmact/mathutil.c contains practically the same code, but where the
// individual x/y(/z) distances are passed instead.
int32_t ldist(const spritetype *s1, const spritetype *s2)
{
    int32_t x = klabs(s1->x-s2->x);
    int32_t y = klabs(s1->y-s2->y);

    if (x<y) swaplong(&x,&y);

    {
        int32_t t = y + (y>>1);
        return (x - (x>>5) - (x>>7)  + (t>>2) + (t>>6));
    }
}

int32_t dist(const spritetype *s1, const spritetype *s2)
{
    int32_t x = klabs(s1->x-s2->x);
    int32_t y = klabs(s1->y-s2->y);
    int32_t z = klabs((s1->z-s2->z)>>4);

    if (x<y) swaplong(&x,&y);
    if (x<z) swaplong(&x,&z);

    {
        int32_t t = y + z;
        return (x - (x>>4) + (t>>2) + (t>>3));
    }
}


// Clear OSD background
void COMMON_clearbackground(int32_t numcols, int32_t numrows)
{
    UNREFERENCED_PARAMETER(numcols);

# ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST && qsetmode==200)
    {
        bglPushAttrib(GL_FOG_BIT);
        bglDisable(GL_FOG);

        setpolymost2dview();
        bglColor4f(0,0,0,0.67f);
        bglEnable(GL_BLEND);
        bglRectd(0,0, xdim,8*numrows+8);
        bglColor4f(0,0,0,1);
        bglRectd(0,8*numrows+4, xdim,8*numrows+8);

        bglPopAttrib();

        return;
    }
# endif

    CLEARLINES2D(0, min(ydim, numrows*8+8), editorcolors[16]);
}
