//
// Common non-engine code/data for EDuke32 and Mapster32
//

#include "compat.h"
#include "build.h"
#include "scriptfile.h"
#include "cache1d.h"
#include "kplib.h"
#include "baselayer.h"
#include "names.h"

#ifdef _WIN32
# include "winbits.h"
# include <shlwapi.h>
# include <winnt.h>
# ifndef KEY_WOW64_32KEY
#  define KEY_WOW64_32KEY 0x0200
# endif
#endif

#include "common.h"
#include "common_game.h"

int32_t g_gameType = GAMEFLAG_DUKE;

// grp/con/def handling

const char *defaultgamegrp[GAMECOUNT] = { "DUKE3D.GRP", "NAM.GRP", "NAPALM.GRP", "WW2GI.GRP" };
const char *defaultdeffilename[GAMECOUNT] = { "duke3d.def", "nam.def", "napalm.def", "ww2gi.def" };
const char *defaultconfilename = "GAME.CON";
const char *defaultgameconfilename[GAMECOUNT] = { "EDUKE.CON", "NAM.CON", "NAPALM.CON", "WW2GI.CON" };

// g_grpNamePtr can ONLY point to a malloc'd block (length BMAX_PATH)
char *g_grpNamePtr = NULL;
// g_defNamePtr can ONLY point to a malloc'd block (length BMAX_PATH)
char *g_defNamePtr = NULL;
// g_scriptNamePtr can ONLY point to a malloc'd block (length BMAX_PATH)
char *g_scriptNamePtr = NULL;

void clearGrpNamePtr(void)
{
    if (g_grpNamePtr != NULL)
        Bfree(g_grpNamePtr);
    // g_grpNamePtr assumed to be assigned to right after
}

void clearDefNamePtr(void)
{
    if (g_defNamePtr != NULL)
        Bfree(g_defNamePtr);
    // g_defNamePtr assumed to be assigned to right after
}

void clearScriptNamePtr(void)
{
    if (g_scriptNamePtr != NULL)
        Bfree(g_scriptNamePtr);
    // g_scriptNamePtr assumed to be assigned to right after
}

const char *G_DefaultGrpFile(void)
{
    if (DUKE)
        return defaultgamegrp[GAME_DUKE];
    // order is important for the following three because GAMEFLAG_NAM overlaps all
    else if (NAPALM)
        return defaultgamegrp[GAME_NAPALM];
    else if (WW2GI)
        return defaultgamegrp[GAME_WW2GI];
    else if (NAM)
        return defaultgamegrp[GAME_NAM];

    return defaultgamegrp[0];
}
const char *G_DefaultDefFile(void)
{
    if (DUKE)
        return defaultdeffilename[GAME_DUKE];
    else if (WW2GI)
        return defaultdeffilename[GAME_WW2GI];
    else if (NAPALM)
    {
        if (!testkopen(defaultdeffilename[GAME_NAPALM],0) && testkopen(defaultdeffilename[GAME_NAM],0))
            return defaultdeffilename[GAME_NAM]; // NAM/NAPALM Sharing
        else
            return defaultdeffilename[GAME_NAPALM];
    }
    else if (NAM)
    {
        if (!testkopen(defaultdeffilename[GAME_NAM],0) && testkopen(defaultdeffilename[GAME_NAPALM],0))
            return defaultdeffilename[GAME_NAPALM]; // NAM/NAPALM Sharing
        else
            return defaultdeffilename[GAME_NAM];
    }

    return defaultdeffilename[0];
}
const char *G_DefaultConFile(void)
{
    if (DUKE && testkopen(defaultgameconfilename[GAME_DUKE],0))
        return defaultgameconfilename[GAME_DUKE];
    else if (WW2GI && testkopen(defaultgameconfilename[GAME_WW2GI],0))
        return defaultgameconfilename[GAME_WW2GI];
    else if (NAPALM)
    {
        if (!testkopen(defaultgameconfilename[GAME_NAPALM],0))
        {
            if (testkopen(defaultgameconfilename[GAME_NAM],0))
                return defaultgameconfilename[GAME_NAM]; // NAM/NAPALM Sharing
        }
        else
            return defaultgameconfilename[GAME_NAPALM];
    }
    else if (NAM)
    {
        if (!testkopen(defaultgameconfilename[GAME_NAM],0))
        {
            if (testkopen(defaultgameconfilename[GAME_NAPALM],0))
                return defaultgameconfilename[GAME_NAPALM]; // NAM/NAPALM Sharing
        }
        else
            return defaultgameconfilename[GAME_NAM];
    }

    return defaultconfilename;
}

const char *G_GrpFile(void)
{
    if (g_grpNamePtr == NULL)
        return G_DefaultGrpFile();
    else
        return g_grpNamePtr;
}

const char *G_DefFile(void)
{
    if (g_defNamePtr == NULL)
        return G_DefaultDefFile();
    else
        return g_defNamePtr;
}

const char *G_ConFile(void)
{
    if (g_scriptNamePtr == NULL)
        return G_DefaultConFile();
    else
        return g_scriptNamePtr;
}

//////////

#define NUMPSKYMULTIS 5
EDUKE32_STATIC_ASSERT(NUMPSKYMULTIS <= MAXPSKYMULTIS);
EDUKE32_STATIC_ASSERT(PSKYOFF_MAX <= MAXPSKYTILES);

// Set up new-style multi-psky handling.
// KEEPINSYNC MultiPsky_TileToIdx().
void G_InitMultiPsky(int32_t CLOUDYOCEAN__DYN, int32_t MOONSKY1__DYN, int32_t BIGORBIT1__DYN, int32_t LA__DYN)
{
    int32_t i;

    psky_t *defaultsky = &multipsky[0];
    psky_t *oceansky = &multipsky[1];
    psky_t *moonsky = &multipsky[2];
    psky_t *spacesky = &multipsky[3];
    psky_t *citysky = &multipsky[4];

    static int32_t inited;
    if (inited)
        return;
    inited = 1;

    multipskytile[0] = -1;
    multipskytile[1] = CLOUDYOCEAN__DYN;
    multipskytile[2] = MOONSKY1__DYN;
    multipskytile[3] = BIGORBIT1__DYN;
    multipskytile[4] = LA__DYN;

    pskynummultis = NUMPSKYMULTIS;

    // When adding other multi-skies, take care that the tileofs[] values are
    // <= PSKYOFF_MAX. (It can be increased up to MAXPSKYTILES, but should be
    // set as tight as possible.)

    // The default sky properties (all others are implicitly zero):
    defaultsky->lognumtiles = 3;
    defaultsky->horizfrac = 32768;

    // CLOUDYOCEAN
    // Aligns with the drawn scene horizon because it has one itself.
    oceansky->lognumtiles = 3;
    oceansky->horizfrac = 65536;

    // MOONSKY1
    //        earth          mountain   mountain         sun
    moonsky->lognumtiles = 3;
    moonsky->horizfrac = 32768;
    moonsky->tileofs[6] = 1;
    moonsky->tileofs[1] = 2;
    moonsky->tileofs[4] = 2;
    moonsky->tileofs[2] = 3;

    // BIGORBIT1   // orbit
    //       earth1         2           3           moon/sun
    spacesky->lognumtiles = 3;
    spacesky->horizfrac = 32768;
    spacesky->tileofs[5] = 1;
    spacesky->tileofs[6] = 2;
    spacesky->tileofs[7] = 3;
    spacesky->tileofs[2] = 4;

    // LA // la city
    //       earth1         2           3           moon/sun
    citysky->lognumtiles = 3;
    citysky->horizfrac = 16384+1024;
    citysky->tileofs[0] = 1;
    citysky->tileofs[1] = 2;
    citysky->tileofs[2] = 1;
    citysky->tileofs[3] = 3;
    citysky->tileofs[4] = 4;
    citysky->tileofs[5] = 0;
    citysky->tileofs[6] = 2;
    citysky->tileofs[7] = 3;

    for (i=0; i<pskynummultis; ++i)
    {
        int32_t j;
        for (j=0; j<(1<<multipsky[i].lognumtiles); ++j)
            Bassert(multipsky[i].tileofs[j] <= PSKYOFF_MAX);
    }
}

void G_SetupGlobalPsky(void)
{
    int32_t i, mskyidx=0;

    // NOTE: Loop must be running backwards for the same behavior as the game
    // (greatest sector index with matching parallaxed sky takes precedence).
    for (i=numsectors-1; i>=0; i--)
    {
        if (sector[i].ceilingstat & 1)
        {
            mskyidx = MultiPsky_TileToIdx(sector[i].ceilingpicnum);
            if (mskyidx > 0)
                break;
        }
    }

    g_pskyidx = mskyidx;
}

//////////

#ifdef GEKKO
#include "gctypes.h" // for bool
void L2Enhance();
void CON_EnableGecko(int channel,int safe);
bool fatInit (uint32_t cacheSize, bool setAsDefaultDevice);
#endif

void G_ExtPreInit(void)
{
#ifdef GEKKO
    L2Enhance();
    CON_EnableGecko(1, 1);
    Bprintf("Console started\n");
    fatInit(28, true);
#endif
}

#ifdef _WIN32
const char * G_GetInstallPath(int32_t insttype)
{
    static char spath[NUMINSTPATHS][BMAX_PATH];
    static int32_t success[NUMINSTPATHS] = { -1, -1 };
    int32_t siz = BMAX_PATH;

    if (success[insttype] == -1)
    {
        HKEY HKLM32;
        LONG keygood = RegOpenKeyEx(HKEY_LOCAL_MACHINE, NULL, 0, KEY_READ | KEY_WOW64_32KEY, &HKLM32);
        // KEY_WOW64_32KEY gets us around Wow6432Node on 64-bit builds

        if (keygood == ERROR_SUCCESS)
            switch (insttype)
            {
            case INSTPATH_STEAM:
                success[insttype] = SHGetValueA(HKLM32, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 225140", "InstallLocation", NULL, spath[insttype], (LPDWORD)&siz);
                break;
            case INSTPATH_GOG:
                success[insttype] = SHGetValueA(HKLM32, "SOFTWARE\\GOG.com\\GOGDUKE3D", "PATH", NULL, spath[insttype], (LPDWORD)&siz);
                break;
            }
    }

    if (success[insttype] == ERROR_SUCCESS)
        return spath[insttype];

    return NULL;
}
#endif

void G_AddSearchPaths(void)
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    addsearchpath("/usr/share/games/jfduke3d");
    addsearchpath("/usr/local/share/games/jfduke3d");
    addsearchpath("/usr/share/games/eduke32");
    addsearchpath("/usr/local/share/games/eduke32");
#elif defined(__APPLE__)
    addsearchpath("/Library/Application Support/JFDuke3D");
    addsearchpath("/Library/Application Support/EDuke32");
#elif defined (_WIN32)
    // detect Steam and GOG versions of Duke3D
    char buf[BMAX_PATH];
    const char* instpath;

    if ((instpath = G_GetInstallPath(INSTPATH_STEAM)))
    {
        Bsprintf(buf, "%s/gameroot", instpath);
        addsearchpath(buf);

        Bsprintf(buf, "%s/gameroot/addons", instpath);
        addsearchpath(buf);
    }

    if ((instpath = G_GetInstallPath(INSTPATH_GOG)))
        addsearchpath(instpath);
#endif
}

void G_CleanupSearchPaths(void)
{
#ifdef _WIN32
    char buf[BMAX_PATH];
    const char* instpath;

    if ((instpath = G_GetInstallPath(INSTPATH_STEAM)))
    {
        Bsprintf(buf, "%s/gameroot", instpath);
        removesearchpath(buf);

        Bsprintf(buf, "%s/gameroot/addons", instpath);
        removesearchpath(buf);
    }

    if ((instpath = G_GetInstallPath(INSTPATH_GOG)))
        removesearchpath(instpath);
#endif
}

//////////

struct strllist *CommandPaths, *CommandGrps;

void G_AddGroup(const char *buffer)
{
    char buf[BMAX_PATH];

    struct strllist *s = (struct strllist *)Bcalloc(1,sizeof(struct strllist));

    Bstrcpy(buf, buffer);

    if (Bstrchr(buf,'.') == 0)
        Bstrcat(buf,".grp");

    s->str = Bstrdup(buf);

    if (CommandGrps)
    {
        struct strllist *t;
        for (t = CommandGrps; t->next; t=t->next) ;
        t->next = s;
        return;
    }
    CommandGrps = s;
}

void G_AddPath(const char *buffer)
{
    struct strllist *s = (struct strllist *)Bcalloc(1,sizeof(struct strllist));
    s->str = Bstrdup(buffer);

    if (CommandPaths)
    {
        struct strllist *t;
        for (t = CommandPaths; t->next; t=t->next) ;
        t->next = s;
        return;
    }
    CommandPaths = s;
}

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


// loads all group (grp, zip, pk3/4) files in the given directory
void G_LoadGroupsInDir(const char *dirname)
{
    static const char *extensions[4] = { "*.grp", "*.zip", "*.pk3", "*.pk4" };

    char buf[BMAX_PATH];
    int32_t i;

    fnlist_t fnlist = FNLIST_INITIALIZER;

    for (i=0; i<4; i++)
    {
        CACHE1D_FIND_REC *rec;

        fnlist_getnames(&fnlist, dirname, extensions[i], -1, 0);

        for (rec=fnlist.findfiles; rec; rec=rec->next)
        {
            Bsnprintf(buf, sizeof(buf), "%s/%s", dirname, rec->name);
            initprintf("Using group file \"%s\".\n", buf);
            initgroupfile(buf);
        }

        fnlist_clearnames(&fnlist);
    }
}

void G_DoAutoload(const char *dirname)
{
    char buf[BMAX_PATH];

    Bsnprintf(buf, sizeof(buf), "autoload/%s", dirname);
    G_LoadGroupsInDir(buf);
}

////

// returns a buffer of size BMAX_PATH
char *dup_filename(const char *fn)
{
    char *buf = (char *)Bmalloc(BMAX_PATH);

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
        setpolymost2dview();
        bglColor4f(0,0,0,0.67f);
        bglEnable(GL_BLEND);
        bglRectd(0,0, xdim,8*numrows+8);
        bglColor4f(0,0,0,1);
        bglRectd(0,8*numrows+4, xdim,8*numrows+8);
        return;
    }
# endif

    CLEARLINES2D(0, min(ydim, numrows*8+8), editorcolors[16]);
}
