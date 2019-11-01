
#include "compat.h"
#include "build.h"
#include "scriptfile.h"
#include "cache1d.h"
#include "kplib.h"
#include "baselayer.h"

#include "common.h"

#include "vfs.h"
#include "../../glbackend/glbackend.h"

// def/clipmap handling

#ifdef HAVE_CLIPSHAPE_FEATURE
GrowArray<char *> g_clipMapFiles;
#endif

void SetClipshapes()
{
#ifdef HAVE_CLIPSHAPE_FEATURE
	// pre-form the default 10 clipmaps
	for (int j = '0'; j <= '9'; ++j)
	{
		char clipshape[16] = "_clipshape0.map";

		clipshape[10] = j;
		g_clipMapFiles.append(Xstrdup(clipshape));
	}
#endif
}

#ifdef HAVE_CLIPSHAPE_FEATURE
void G_AddClipMap(const char *buffer)
{
    g_clipMapFiles.append(Xstrdup(buffer));
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

// checks from path and in ZIPs, returns 1 if NOT found
int32_t check_file_exist(const char *fn)
{
#ifdef USE_PHYSFS
    return !PHYSFS_exists(fn);
#else
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
    else Xfree(tfn);
    pathsearchmode = opsm;

    return 0;
#endif
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
    auto sp1 = (vec2_t const *)s1;
    auto sp2 = (vec2_t const *)s2;
    return sepldist(sp1->x - sp2->x, sp1->y - sp2->y)
        + (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE ? 1 : 0);
}

int32_t dist(const void *s1, const void *s2)
{
    auto sp1 = (vec3_t const *)s1;
    auto sp2 = (vec3_t const *)s2;
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
void COMMON_doclearbackground(int numcols, int height)
{
	GLInterface.UseColorOnly(true);

	polymostSet2dView();

	VSMatrix identity(0);
	GLInterface.SetMatrix(Matrix_ModelView, &identity);
	auto vert = GLInterface.AllocVertices(8);
	auto vt = vert.second;

	auto h4 = height - 4;

	GLInterface.EnableBlend(true);

	vt[0].Set(0, 0); //top-left
	vt[1].Set(0, h4); //bottom-left
	vt[2].Set(xdim, 0); //top-right
	vt[3].Set(xdim, h4);  //bottom-right
	GLInterface.SetColor(0.f, 0.f, 0.f, 0.67f);
	GLInterface.Draw(DT_TRIANGLE_STRIP, vert.first, 4);

	vt[0].Set(0, h4); //top-left
	vt[1].Set(0, height); //bottom-left
	vt[2].Set(xdim, h4); //top-right
	vt[3].Set(xdim, height);  //bottom-right
	GLInterface.SetColor(0.f, 0.f, 0.f, 1.f);
	GLInterface.Draw(DT_TRIANGLE_STRIP, vert.first+4, 4);

	GLInterface.UseColorOnly(false);
}

void COMMON_clearbackground(int numcols, int numrows)
{
	COMMON_doclearbackground(numcols, 8 * numrows + 8);
}

#if defined _WIN32 && !defined EDUKE32_STANDALONE
# define NEED_SHLWAPI_H
# include "windows_inc.h"
# ifndef KEY_WOW64_64KEY
#  define KEY_WOW64_64KEY 0x0100
# endif
# ifndef KEY_WOW64_32KEY
#  define KEY_WOW64_32KEY 0x0200
# endif

int Paths_ReadRegistryValue(char const * const SubKey, char const * const Value, char * const Output, DWORD * OutputSize)
{
    // KEY_WOW64_32KEY gets us around Wow6432Node on 64-bit builds
    REGSAM const wow64keys[] = { KEY_WOW64_32KEY, KEY_WOW64_64KEY };

    for (auto &wow64key : wow64keys)
    {
        HKEY hkey;
        LONG keygood = RegOpenKeyEx(HKEY_LOCAL_MACHINE, NULL, 0, KEY_READ | wow64key, &hkey);

        if (keygood != ERROR_SUCCESS)
            continue;

        LONG retval = SHGetValueA(hkey, SubKey, Value, NULL, Output, OutputSize);

        RegCloseKey(hkey);

        if (retval == ERROR_SUCCESS)
            return 1;
    }

    return 0;
}
#endif

// A bare-bones "parser" for Valve's KeyValues VDF format.
// There is no guarantee this will function properly with ill-formed files.
static void KeyValues_SkipWhitespace(char **vdfbuf, char * const vdfbufend)
{
    while (((*vdfbuf)[0] == ' ' || (*vdfbuf)[0] == '\n' || (*vdfbuf)[0] == '\r' || (*vdfbuf)[0] == '\t' || (*vdfbuf)[0] == '\0') && *vdfbuf < vdfbufend)
        (*vdfbuf)++;

    // comments
    if ((*vdfbuf) + 2 < vdfbufend && (*vdfbuf)[0] == '/' && (*vdfbuf)[1] == '/')
    {
        while ((*vdfbuf)[0] != '\n' && (*vdfbuf)[0] != '\r' && *vdfbuf < vdfbufend)
            (*vdfbuf)++;

        KeyValues_SkipWhitespace(vdfbuf, vdfbufend);
    }
}
static void KeyValues_SkipToEndOfQuotedToken(char **vdfbuf, char * const vdfbufend)
{
    (*vdfbuf)++;
    while ((*vdfbuf)[0] != '\"' && (*vdfbuf)[-1] != '\\' && *vdfbuf < vdfbufend)
        (*vdfbuf)++;
}
static void KeyValues_SkipToEndOfUnquotedToken(char **vdfbuf, char * const vdfbufend)
{
    while ((*vdfbuf)[0] != ' ' && (*vdfbuf)[0] != '\n' && (*vdfbuf)[0] != '\r' && (*vdfbuf)[0] != '\t' && (*vdfbuf)[0] != '\0' && *vdfbuf < vdfbufend)
        (*vdfbuf)++;
}
static void KeyValues_SkipNextWhatever(char **vdfbuf, char * const vdfbufend)
{
    KeyValues_SkipWhitespace(vdfbuf, vdfbufend);

    if (*vdfbuf == vdfbufend)
        return;

    if ((*vdfbuf)[0] == '{')
    {
        (*vdfbuf)++;
        do
        {
            KeyValues_SkipNextWhatever(vdfbuf, vdfbufend);
        }
        while ((*vdfbuf)[0] != '}');
        (*vdfbuf)++;
    }
    else if ((*vdfbuf)[0] == '\"')
        KeyValues_SkipToEndOfQuotedToken(vdfbuf, vdfbufend);
    else if ((*vdfbuf)[0] != '}')
        KeyValues_SkipToEndOfUnquotedToken(vdfbuf, vdfbufend);

    KeyValues_SkipWhitespace(vdfbuf, vdfbufend);
}
static char* KeyValues_NormalizeToken(char **vdfbuf, char * const vdfbufend)
{
    char *token = *vdfbuf;

    if ((*vdfbuf)[0] == '\"' && *vdfbuf < vdfbufend)
    {
        token++;

        KeyValues_SkipToEndOfQuotedToken(vdfbuf, vdfbufend);
        (*vdfbuf)[0] = '\0';

        // account for escape sequences
        char *writeseeker = token, *readseeker = token;
        while (readseeker <= *vdfbuf)
        {
            if (readseeker[0] == '\\')
                readseeker++;

            writeseeker[0] = readseeker[0];

            writeseeker++;
            readseeker++;
        }

        return token;
    }

    KeyValues_SkipToEndOfUnquotedToken(vdfbuf, vdfbufend);
    (*vdfbuf)[0] = '\0';

    return token;
}
static void KeyValues_FindKey(char **vdfbuf, char * const vdfbufend, const char *token)
{
    char *ParentKey = KeyValues_NormalizeToken(vdfbuf, vdfbufend);
    if (token != NULL) // pass in NULL to find the next key instead of a specific one
        while (Bstrcmp(ParentKey, token) != 0 && *vdfbuf < vdfbufend)
        {
            KeyValues_SkipNextWhatever(vdfbuf, vdfbufend);
            ParentKey = KeyValues_NormalizeToken(vdfbuf, vdfbufend);
        }

    KeyValues_SkipWhitespace(vdfbuf, vdfbufend);
}
static int32_t KeyValues_FindParentKey(char **vdfbuf, char * const vdfbufend, const char *token)
{
    KeyValues_SkipWhitespace(vdfbuf, vdfbufend);

    // end of scope
    if ((*vdfbuf)[0] == '}')
        return 0;

    KeyValues_FindKey(vdfbuf, vdfbufend, token);

    // ignore the wrong type
    while ((*vdfbuf)[0] != '{' && *vdfbuf < vdfbufend)
    {
        KeyValues_SkipNextWhatever(vdfbuf, vdfbufend);
        KeyValues_FindKey(vdfbuf, vdfbufend, token);
    }

    if (*vdfbuf == vdfbufend)
        return 0;

    return 1;
}
static char* KeyValues_FindKeyValue(char **vdfbuf, char * const vdfbufend, const char *token)
{
    KeyValues_SkipWhitespace(vdfbuf, vdfbufend);

    // end of scope
    if ((*vdfbuf)[0] == '}')
        return NULL;

    KeyValues_FindKey(vdfbuf, vdfbufend, token);

    // ignore the wrong type
    while ((*vdfbuf)[0] == '{' && *vdfbuf < vdfbufend)
    {
        KeyValues_SkipNextWhatever(vdfbuf, vdfbufend);
        KeyValues_FindKey(vdfbuf, vdfbufend, token);
    }

    KeyValues_SkipWhitespace(vdfbuf, vdfbufend);

    if (*vdfbuf == vdfbufend)
        return NULL;

    return KeyValues_NormalizeToken(vdfbuf, vdfbufend);
}

void Paths_ParseSteamKeyValuesForPaths(const char *vdf, SteamPathParseFunc func)
{
    buildvfs_fd fd = buildvfs_open_read(vdf);
    int32_t size = buildvfs_length(fd);
    char *vdfbufstart, *vdfbuf, *vdfbufend;

    if (size <= 0)
        return;

    vdfbufstart = vdfbuf = (char*)Xmalloc(size);
    size = (int32_t)buildvfs_read(fd, vdfbuf, size);
    buildvfs_close(fd);
    vdfbufend = vdfbuf + size;

    if (KeyValues_FindParentKey(&vdfbuf, vdfbufend, "LibraryFolders"))
    {
        char *result;
        vdfbuf++;
        while ((result = KeyValues_FindKeyValue(&vdfbuf, vdfbufend, NULL)) != NULL)
            func(result);
    }

    Xfree(vdfbufstart);
}
