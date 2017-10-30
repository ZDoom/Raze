//
// Common non-engine code/data for EDuke32 and Mapster32
//

#include "compat.h"
#include "build.h"
#include "baselayer.h"
#include "palette.h"

#include "grpscan.h"

#ifdef _WIN32
# define NEED_SHLWAPI_H
# include "windows_inc.h"
# include "winbits.h"
# ifndef KEY_WOW64_64KEY
#  define KEY_WOW64_64KEY 0x0100
# endif
# ifndef KEY_WOW64_32KEY
#  define KEY_WOW64_32KEY 0x0200
# endif
#elif defined __APPLE__
# include "osxbits.h"
#endif

#include "common.h"
#include "common_game.h"

struct grpfile_t const *g_selectedGrp;

int32_t g_gameType = GAMEFLAG_DUKE;
int     g_addonNum = 0;

// g_gameNamePtr can point to one of: grpfiles[].name (string literal), string
// literal, malloc'd block (XXX: possible leak)
const char *g_gameNamePtr = NULL;

// grp/con handling

static const char *defaultconfilename                = "GAME.CON";
#ifndef EDUKE32_STANDALONE
static const char *defaultgamegrp[GAMECOUNT]         = { "DUKE3D.GRP", "NAM.GRP", "NAPALM.GRP", "WW2GI.GRP" };
static const char *defaultdeffilename[GAMECOUNT]     = { "duke3d.def", "nam.def", "napalm.def", "ww2gi.def" };
static const char *defaultgameconfilename[GAMECOUNT] = { "EDUKE.CON", "NAM.CON", "NAPALM.CON", "WW2GI.CON" };
#endif

// g_grpNamePtr can ONLY point to a malloc'd block (length BMAX_PATH)
char *g_grpNamePtr = NULL;
// g_scriptNamePtr can ONLY point to a malloc'd block (length BMAX_PATH)
char *g_scriptNamePtr = NULL;

void clearGrpNamePtr(void)
{
    Bfree(g_grpNamePtr);
    // g_grpNamePtr assumed to be assigned to right after
}

void clearScriptNamePtr(void)
{
    Bfree(g_scriptNamePtr);
    // g_scriptNamePtr assumed to be assigned to right after
}

const char *G_DefaultGrpFile(void)
{
#ifndef EDUKE32_STANDALONE
    if (DUKE)
        return defaultgamegrp[GAME_DUKE];
    else if (NAPALM)
        return defaultgamegrp[GAME_NAPALM];
    else if (WW2GI)
        return defaultgamegrp[GAME_WW2GI];
    else if (NAM)
        return defaultgamegrp[GAME_NAM];

    return defaultgamegrp[0];
#else
    return "(none)";
#endif
}
const char *G_DefaultDefFile(void)
{
#ifndef EDUKE32_STANDALONE
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
#else
    return "(none)";
#endif
}
const char *G_DefaultConFile(void)
{
#ifndef EDUKE32_STANDALONE
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
#endif
    return defaultconfilename;
}

const char *G_GrpFile(void)
{
    return (g_grpNamePtr == NULL) ? G_DefaultGrpFile() : g_grpNamePtr;
}

const char *G_DefFile(void)
{
    return (g_defNamePtr == NULL) ? G_DefaultDefFile() : g_defNamePtr;
}

const char *G_ConFile(void)
{
    return (g_scriptNamePtr == NULL) ? G_DefaultConFile() : g_scriptNamePtr;
}

//////////

// Set up new-style multi-psky handling.
void G_InitMultiPsky(int CLOUDYOCEAN__DYN, int MOONSKY1__DYN, int BIGORBIT1__DYN, int LA__DYN)
{
    // When adding other multi-skies, take care that the tileofs[] values are
    // <= PSKYOFF_MAX. (It can be increased up to MAXPSKYTILES, but should be
    // set as tight as possible.)

    // The default sky properties (all others are implicitly zero):
    psky_t *sky      = E_DefinePsky(DEFAULTPSKY);
    sky->lognumtiles = 3;
    sky->horizfrac   = 32768;

    // CLOUDYOCEAN
    // Aligns with the drawn scene horizon because it has one itself.
    sky              = E_DefinePsky(CLOUDYOCEAN__DYN);
    sky->lognumtiles = 3;
    sky->horizfrac   = 65536;

    // MOONSKY1
    //        earth          mountain   mountain         sun
    sky              = E_DefinePsky(MOONSKY1__DYN);
    sky->lognumtiles = 3;
    sky->horizfrac   = 32768;
    sky->tileofs[6]  = 1;
    sky->tileofs[1]  = 2;
    sky->tileofs[4]  = 2;
    sky->tileofs[2]  = 3;

    // BIGORBIT1   // orbit
    //       earth1         2           3           moon/sun
    sky              = E_DefinePsky(BIGORBIT1__DYN);
    sky->lognumtiles = 3;
    sky->horizfrac   = 32768;
    sky->tileofs[5]  = 1;
    sky->tileofs[6]  = 2;
    sky->tileofs[7]  = 3;
    sky->tileofs[2]  = 4;

    // LA // la city
    //       earth1         2           3           moon/sun
    sky              = E_DefinePsky(LA__DYN);
    sky->lognumtiles = 3;
    sky->horizfrac   = 16384 + 1024;
    sky->tileofs[0]  = 1;
    sky->tileofs[1]  = 2;
    sky->tileofs[2]  = 1;
    sky->tileofs[3]  = 3;
    sky->tileofs[4]  = 4;
    sky->tileofs[5]  = 0;
    sky->tileofs[6]  = 2;
    sky->tileofs[7]  = 3;

#if 0
    // This assertion should hold. See note above.
    for (bssize_t i=0; i<pskynummultis; ++i)
        for (bssize_t j=0; j<(1<<multipsky[i].lognumtiles); ++j)
            Bassert(multipsky[i].tileofs[j] <= PSKYOFF_MAX);
#endif
}

void G_SetupGlobalPsky(void)
{
    int skyIdx = 0;

    // NOTE: Loop must be running backwards for the same behavior as the game
    // (greatest sector index with matching parallaxed sky takes precedence).
    for (bssize_t i = numsectors - 1; i >= 0; i--)
    {
        if (sector[i].ceilingstat & 1)
        {
            skyIdx = getpskyidx(sector[i].ceilingpicnum);
            if (skyIdx > 0)
                break;
        }
    }

    g_pskyidx = skyIdx;
}

//////////

static char g_rootDir[BMAX_PATH];
char g_modDir[BMAX_PATH] = "/";

int kopen4loadfrommod(const char *fileName, char searchfirst)
{
    int kFile = -1;

    if (g_modDir[0] != '/' || g_modDir[1] != 0)
    {
        static char staticFileName[BMAX_PATH];
        Bsnprintf(staticFileName, sizeof(staticFileName), "%s/%s", g_modDir, fileName);
        kFile = kopen4load(staticFileName, searchfirst);
    }

    return (kFile < 0) ? kopen4load(fileName, searchfirst) : kFile;
}

int g_useCwd;
static void G_LoadAddon(void);
int32_t g_groupFileHandle;

void G_ExtPreInit(int32_t argc,char const * const * argv)
{
    g_useCwd = G_CheckCmdSwitch(argc, argv, "-usecwd");

#ifdef _WIN32
    GetModuleFileName(NULL,g_rootDir,BMAX_PATH);
    Bcorrectfilename(g_rootDir,1);
    //chdir(g_rootDir);
#else
    getcwd(g_rootDir,BMAX_PATH);
    strcat(g_rootDir,"/");
#endif
}

void G_ExtInit(void)
{
    char cwd[BMAX_PATH];

#ifdef EDUKE32_OSX
    char *appdir = Bgetappdir();
    addsearchpath(appdir);
    Bfree(appdir);
#endif

    if (getcwd(cwd,BMAX_PATH) && Bstrcmp(cwd,"/") != 0)
        addsearchpath(cwd);

    if (CommandPaths)
    {
        int32_t i;
        struct strllist *s;
        while (CommandPaths)
        {
            s = CommandPaths->next;
            i = addsearchpath(CommandPaths->str);
            if (i < 0)
            {
                initprintf("Failed adding %s for game data: %s\n", CommandPaths->str,
                           i==-1 ? "not a directory" : "no such directory");
            }

            Bfree(CommandPaths->str);
            Bfree(CommandPaths);
            CommandPaths = s;
        }
    }

#if defined(_WIN32)
    if (!access("user_profiles_enabled", F_OK))
#else
    if (g_useCwd == 0 && access("user_profiles_disabled", F_OK))
#endif
    {
        char *homedir;
        int32_t asperr;

        if ((homedir = Bgethomedir()))
        {
            Bsnprintf(cwd,sizeof(cwd),"%s/"
#if defined(_WIN32)
                      "EDuke32 Settings"
#elif defined(GEKKO)
                      "apps/eduke32"
#else
                      ".eduke32"
#endif
                      ,homedir);
            asperr = addsearchpath(cwd);
            if (asperr == -2)
            {
                if (Bmkdir(cwd,S_IRWXU) == 0) asperr = addsearchpath(cwd);
                else asperr = -1;
            }
            if (asperr == 0)
                Bchdir(cwd);
            Bfree(homedir);
        }
    }

    // JBF 20031220: Because it's annoying renaming GRP files whenever I want to test different game data
#ifndef EDUKE32_STANDALONE
    if (g_grpNamePtr == NULL)
    {
        const char *cp = getenv("DUKE3DGRP");
        if (cp)
        {
            clearGrpNamePtr();
            g_grpNamePtr = dup_filename(cp);
            initprintf("Using \"%s\" as main GRP file\n", g_grpNamePtr);
        }
    }
#endif
}

void G_ScanGroups(void)
{
    ScanGroups();

    g_selectedGrp = NULL;

    char const * const currentGrp = G_GrpFile();

    for (grpfile_t const *fg = foundgrps; fg; fg=fg->next)
    {
        if (!Bstrcasecmp(fg->filename, currentGrp))
        {
            g_selectedGrp = fg;
            break;
        }
    }

    if (g_selectedGrp == NULL)
        g_selectedGrp = foundgrps;
}

static int32_t G_TryLoadingGrp(char const * const grpfile)
{
    int32_t i;

    if ((i = initgroupfile(grpfile)) == -1)
        initprintf("Warning: could not find main data file \"%s\"!\n", grpfile);
    else
        initprintf("Using \"%s\" as main game data file.\n", grpfile);

    return i;
}

static int32_t G_LoadGrpDependencyChain(grpfile_t const * const grp)
{
    if (!grp)
        return -1;

    if (grp->type->dependency && grp->type->dependency != grp->type->crcval)
        G_LoadGrpDependencyChain(FindGroup(grp->type->dependency));

    int32_t const i = G_TryLoadingGrp(grp->filename);

    if (grp->type->postprocessing)
        grp->type->postprocessing(i);

    return i;
}

void G_LoadGroups(int32_t autoload)
{
    if (g_modDir[0] != '/')
    {
        char cwd[BMAX_PATH];

        Bstrcat(g_rootDir, g_modDir);
        addsearchpath(g_rootDir);
        //        addsearchpath(mod_dir);

        if (getcwd(cwd, BMAX_PATH))
        {
            Bsprintf(cwd, "%s/%s", cwd, g_modDir);
            if (!Bstrcmp(g_rootDir, cwd))
            {
                if (addsearchpath(cwd) == -2)
                    if (Bmkdir(cwd, S_IRWXU) == 0)
                        addsearchpath(cwd);
            }
        }

#ifdef USE_OPENGL
        Bsprintf(cwd, "%s/%s", g_modDir, TEXCACHEFILE);
        Bstrcpy(TEXCACHEFILE, cwd);
#endif
    }

    if (g_addonNum)
        G_LoadAddon();

    const char *grpfile;
    int32_t i;

    if ((i = G_LoadGrpDependencyChain(g_selectedGrp)) != -1)
    {
        grpfile = g_selectedGrp->filename;

        clearGrpNamePtr();
        g_grpNamePtr = dup_filename(grpfile);

        grpinfo_t const * const type = g_selectedGrp->type;

        g_gameType = type->game;
        g_gameNamePtr = type->name;

        if (type->scriptname && g_scriptNamePtr == NULL)
            g_scriptNamePtr = dup_filename(type->scriptname);

        if (type->defname && g_defNamePtr == NULL)
            g_defNamePtr = dup_filename(type->defname);
    }
    else
    {
        grpfile = G_GrpFile();
        i = G_TryLoadingGrp(grpfile);
    }

    if (autoload)
    {
        G_LoadGroupsInDir("autoload");

        if (i != -1)
            G_DoAutoload(grpfile);
    }

    if (g_modDir[0] != '/')
        G_LoadGroupsInDir(g_modDir);

#ifndef EDUKE32_STANDALONE
    if (g_defNamePtr == NULL)
    {
        const char *tmpptr = getenv("DUKE3DDEF");
        if (tmpptr)
        {
            clearDefNamePtr();
            g_defNamePtr = dup_filename(tmpptr);
            initprintf("Using \"%s\" as definitions file\n", g_defNamePtr);
        }
    }
#endif

    loaddefinitions_game(G_DefFile(), TRUE);

    struct strllist *s;

    int const bakpathsearchmode = pathsearchmode;
    pathsearchmode = 1;

    while (CommandGrps)
    {
        int32_t j;

        s = CommandGrps->next;

        if ((j = initgroupfile(CommandGrps->str)) == -1)
            initprintf("Could not find file \"%s\".\n", CommandGrps->str);
        else
        {
            g_groupFileHandle = j;
            initprintf("Using file \"%s\" as game data.\n", CommandGrps->str);
            if (autoload)
                G_DoAutoload(CommandGrps->str);
        }

        Bfree(CommandGrps->str);
        Bfree(CommandGrps);
        CommandGrps = s;
    }
    pathsearchmode = bakpathsearchmode;
}

#if defined _WIN32 && !defined EDUKE32_STANDALONE
static int G_ReadRegistryValue(char const * const SubKey, char const * const Value, char * const Output, DWORD * OutputSize)
{
    // KEY_WOW64_32KEY gets us around Wow6432Node on 64-bit builds
    REGSAM const wow64keys[] = { KEY_WOW64_32KEY, KEY_WOW64_64KEY };

    for (size_t k = 0; k < ARRAY_SIZE(wow64keys); ++k)
    {
        HKEY hkey;
        LONG keygood = RegOpenKeyEx(HKEY_LOCAL_MACHINE, NULL, 0, KEY_READ | wow64keys[k], &hkey);

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

static void G_LoadAddon(void)
{
#ifndef EDUKE32_STANDALONE
    int32_t crc = 0;  // compiler-happy

    switch (g_addonNum)
    {
    case ADDON_DUKEDC:
        crc = DUKEDC_CRC;
        break;
    case ADDON_NWINTER:
        crc = DUKENW_CRC;
        break;
    case ADDON_CARIBBEAN:
        crc = DUKECB_CRC;
        break;
    }

    if (!crc) return;

    grpfile_t const * const grp = FindGroup(crc);

    if (grp)
        g_selectedGrp = grp;
#endif
}

#ifndef EDUKE32_STANDALONE
#ifndef EDUKE32_TOUCH_DEVICES
#if defined EDUKE32_OSX || defined __linux__ || defined EDUKE32_BSD
static void G_AddSteamPaths(const char *basepath)
{
    char buf[BMAX_PATH];

    // Duke Nukem 3D: Megaton Edition (Steam)
    Bsnprintf(buf, sizeof(buf), "%s/steamapps/common/Duke Nukem 3D/gameroot", basepath);
    addsearchpath(buf);
    Bsnprintf(buf, sizeof(buf), "%s/steamapps/common/Duke Nukem 3D/gameroot/addons/dc", basepath);
    addsearchpath_user(buf, SEARCHPATH_REMOVE);
    Bsnprintf(buf, sizeof(buf), "%s/steamapps/common/Duke Nukem 3D/gameroot/addons/nw", basepath);
    addsearchpath_user(buf, SEARCHPATH_REMOVE);
    Bsnprintf(buf, sizeof(buf), "%s/steamapps/common/Duke Nukem 3D/gameroot/addons/vacation", basepath);
    addsearchpath_user(buf, SEARCHPATH_REMOVE);

    // Duke Nukem 3D (3D Realms Anthology (Steam) / Kill-A-Ton Collection 2015)
#if defined EDUKE32_OSX
    Bsnprintf(buf, sizeof(buf), "%s/steamapps/common/Duke Nukem 3D/Duke Nukem 3D.app/drive_c/Program Files/Duke Nukem 3D", basepath);
    addsearchpath_user(buf, SEARCHPATH_REMOVE);
#endif

    // NAM (Steam)
#if defined EDUKE32_OSX
    Bsnprintf(buf, sizeof(buf), "%s/steamapps/common/Nam/Nam.app/Contents/Resources/Nam.boxer/C.harddisk/NAM", basepath);
#else
    Bsnprintf(buf, sizeof(buf), "%s/steamapps/common/Nam/NAM", basepath);
#endif
    addsearchpath_user(buf, SEARCHPATH_NAM);

    // WWII GI (Steam)
    Bsnprintf(buf, sizeof(buf), "%s/steamapps/common/World War II GI/WW2GI", basepath);
    addsearchpath_user(buf, SEARCHPATH_WW2GI);
}

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

static void G_ParseSteamKeyValuesForPaths(const char *vdf)
{
    int32_t fd = Bopen(vdf, BO_RDONLY);
    int32_t size = Bfilelength(fd);
    char *vdfbufstart, *vdfbuf, *vdfbufend;

    if (size <= 0)
        return;

    vdfbufstart = vdfbuf = (char*)Bmalloc(size);
    size = (int32_t)Bread(fd, vdfbuf, size);
    Bclose(fd);
    vdfbufend = vdfbuf + size;

    if (KeyValues_FindParentKey(&vdfbuf, vdfbufend, "LibraryFolders"))
    {
        char *result;
        vdfbuf++;
        while ((result = KeyValues_FindKeyValue(&vdfbuf, vdfbufend, NULL)) != NULL)
            G_AddSteamPaths(result);
    }

    Bfree(vdfbufstart);
}
#endif
#endif
#endif

void G_AddSearchPaths(void)
{
#ifndef EDUKE32_STANDALONE
#ifndef EDUKE32_TOUCH_DEVICES
#if defined __linux__ || defined EDUKE32_BSD
    char buf[BMAX_PATH];
    char *homepath = Bgethomedir();

    Bsnprintf(buf, sizeof(buf), "%s/.steam/steam", homepath);
    G_AddSteamPaths(buf);

    Bsnprintf(buf, sizeof(buf), "%s/.steam/steam/steamapps/libraryfolders.vdf", homepath);
    G_ParseSteamKeyValuesForPaths(buf);

    Bfree(homepath);

    addsearchpath("/usr/share/games/jfduke3d");
    addsearchpath("/usr/local/share/games/jfduke3d");
    addsearchpath("/usr/share/games/eduke32");
    addsearchpath("/usr/local/share/games/eduke32");
#elif defined EDUKE32_OSX
    char buf[BMAX_PATH];
    int32_t i;
    char *applications[] = { osx_getapplicationsdir(0), osx_getapplicationsdir(1) };
    char *support[] = { osx_getsupportdir(0), osx_getsupportdir(1) };

    for (i = 0; i < 2; i++)
    {
        Bsnprintf(buf, sizeof(buf), "%s/Steam", support[i]);
        G_AddSteamPaths(buf);

        Bsnprintf(buf, sizeof(buf), "%s/Steam/steamapps/libraryfolders.vdf", support[i]);
        G_ParseSteamKeyValuesForPaths(buf);

        // Duke Nukem 3D: Atomic Edition (GOG.com)
        Bsnprintf(buf, sizeof(buf), "%s/Duke Nukem 3D.app/Contents/Resources/Duke Nukem 3D.boxer/C.harddisk", applications[i]);
        addsearchpath_user(buf, SEARCHPATH_REMOVE);
    }

    for (i = 0; i < 2; i++)
    {
        Bsnprintf(buf, sizeof(buf), "%s/JFDuke3D", support[i]);
        addsearchpath(buf);
        Bsnprintf(buf, sizeof(buf), "%s/EDuke32", support[i]);
        addsearchpath(buf);
    }

    for (i = 0; i < 2; i++)
    {
        Bfree(applications[i]);
        Bfree(support[i]);
    }
#elif defined (_WIN32)
    char buf[BMAX_PATH] = {0};
    DWORD bufsize;

    // Duke Nukem 3D: 20th Anniversary World Tour (Steam)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 434050", "InstallLocation", buf, &bufsize))
    {
        addsearchpath_user(buf, SEARCHPATH_REMOVE);
    }

    // Duke Nukem 3D: Megaton Edition (Steam)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 225140", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        DWORD const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/gameroot", remaining);
        addsearchpath(buf);
        Bstrncpy(suffix, "/gameroot/addons/dc", remaining);
        addsearchpath_user(buf, SEARCHPATH_REMOVE);
        Bstrncpy(suffix, "/gameroot/addons/nw", remaining);
        addsearchpath_user(buf, SEARCHPATH_REMOVE);
        Bstrncpy(suffix, "/gameroot/addons/vacation", remaining);
        addsearchpath_user(buf, SEARCHPATH_REMOVE);
    }

    // Duke Nukem 3D (3D Realms Anthology (Steam) / Kill-A-Ton Collection 2015)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 359850", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        DWORD const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/Duke Nukem 3D", remaining);
        addsearchpath_user(buf, SEARCHPATH_REMOVE);
    }

    // Duke Nukem 3D: Atomic Edition (GOG.com)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\GOG.com\\GOGDUKE3D", "PATH", buf, &bufsize))
    {
        addsearchpath_user(buf, SEARCHPATH_REMOVE);
    }

    // Duke Nukem 3D (3D Realms Anthology)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\3DRealms\\Duke Nukem 3D", NULL, buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        DWORD const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/Duke Nukem 3D", remaining);
        addsearchpath_user(buf, SEARCHPATH_REMOVE);
    }

    // 3D Realms Anthology
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\3DRealms\\Anthology", NULL, buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        DWORD const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/Duke Nukem 3D", remaining);
        addsearchpath_user(buf, SEARCHPATH_REMOVE);
    }

    // NAM (Steam)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 329650", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        DWORD const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/NAM", remaining);
        addsearchpath_user(buf, SEARCHPATH_NAM);
    }

    // WWII GI (Steam)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 376750", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        DWORD const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/WW2GI", remaining);
        addsearchpath_user(buf, SEARCHPATH_WW2GI);
    }
#endif
#endif
#endif
}

void G_CleanupSearchPaths(void)
{
    removesearchpaths_withuser(SEARCHPATH_REMOVE);

    if (!NAM)
        removesearchpaths_withuser(SEARCHPATH_NAM);

    if (!WW2GI)
        removesearchpaths_withuser(SEARCHPATH_WW2GI);
}

//////////

struct strllist *CommandPaths, *CommandGrps;

char **g_scriptModules = NULL;
int32_t g_scriptModulesNum = 0;

void G_AddGroup(const char *buffer)
{
    char buf[BMAX_PATH];

    struct strllist *s = (struct strllist *)Xcalloc(1,sizeof(struct strllist));

    Bstrcpy(buf, buffer);

    if (Bstrchr(buf,'.') == 0)
        Bstrcat(buf,".grp");

    s->str = Xstrdup(buf);

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
    struct strllist *s = (struct strllist *)Xcalloc(1,sizeof(struct strllist));
    s->str = Xstrdup(buffer);

    if (CommandPaths)
    {
        struct strllist *t;
        for (t = CommandPaths; t->next; t=t->next) ;
        t->next = s;
        return;
    }
    CommandPaths = s;
}

void G_AddCon(const char *buffer)
{
    clearScriptNamePtr();
    g_scriptNamePtr = dup_filename(buffer);
    initprintf("Using CON file \"%s\".\n",g_scriptNamePtr);
}

void G_AddConModule(const char *buffer)
{
    g_scriptModules = (char **) Xrealloc (g_scriptModules, (g_scriptModulesNum+1) * sizeof(char *));
    g_scriptModules[g_scriptModulesNum] = Xstrdup(buffer);
    ++g_scriptModulesNum;
}

//////////

// loads all group (grp, zip, pk3/4) files in the given directory
void G_LoadGroupsInDir(const char *dirname)
{
    static const char *extensions[] = { "*.grp", "*.zip", "*.ssi", "*.pk3", "*.pk4" };
    char buf[BMAX_PATH];
    fnlist_t fnlist = FNLIST_INITIALIZER;

    for (unsigned i=0; i<(sizeof(extensions)/sizeof(extensions[0])); i++)
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

//////////

void G_LoadLookups(void)
{
    int32_t fp, j;

    if ((fp=kopen4loadfrommod("lookup.dat",0)) == -1)
        if ((fp=kopen4loadfrommod("lookup.dat",1)) == -1)
            return;

    j = loadlookups(fp);

    if (j < 0)
    {
        if (j == -1)
            initprintf("ERROR loading \"lookup.dat\": failed reading enough data.\n");

        return kclose(fp);
    }

    uint8_t paldata[768];

    for (j=1; j<=5; j++)
    {
        // Account for TITLE and REALMS swap between basepal number and on-disk order.
        int32_t basepalnum = (j == 3 || j == 4) ? 4+3-j : j;

        if (kread_and_test(fp, paldata, 768))
            return kclose(fp);

        for (bssize_t k = 0; k < 768; k++)
            paldata[k] <<= 2;

        setbasepal(basepalnum, paldata);
    }

    kclose(fp);
}

//////////

#ifdef FORMAT_UPGRADE_ELIGIBLE

static int32_t S_TryFormats(char * const testfn, char * const fn_suffix, char const searchfirst)
{
#ifdef HAVE_FLAC
    {
        Bstrcpy(fn_suffix, ".flac");
        int32_t const fp = kopen4loadfrommod(testfn, searchfirst);
        if (fp >= 0)
            return fp;
    }
#endif

#ifdef HAVE_VORBIS
    {
        Bstrcpy(fn_suffix, ".ogg");
        int32_t const fp = kopen4loadfrommod(testfn, searchfirst);
        if (fp >= 0)
            return fp;
    }
#endif

    return -1;
}

static int32_t S_TryExtensionReplacements(char * const testfn, char const searchfirst, uint8_t const ismusic)
{
    char * extension = Bstrrchr(testfn, '.');
    char * const fn_end = Bstrchr(testfn, '\0');

    // ex: grabbag.voc --> grabbag_voc.*
    if (extension != NULL)
    {
        *extension = '_';

        int32_t const fp = S_TryFormats(testfn, fn_end, searchfirst);
        if (fp >= 0)
            return fp;
    }
    else
    {
        extension = fn_end;
    }

    // ex: grabbag.mid --> grabbag.*
    if (ismusic)
    {
        int32_t const fp = S_TryFormats(testfn, extension, searchfirst);
        if (fp >= 0)
            return fp;
    }

    return -1;
}

int32_t S_OpenAudio(const char *fn, char searchfirst, uint8_t const ismusic)
{
    int32_t const origfp = kopen4loadfrommod(fn, searchfirst);
    char const * const origparent = origfp != -1 ? kfileparent(origfp) : NULL;
    uint32_t const origparentlength = origparent != NULL ? Bstrlen(origparent) : 0;

    char * const testfn = (char *)Xmalloc(Bstrlen(fn) + 12 + origparentlength); // "music/" + overestimation of parent minus extension + ".flac" + '\0'

    // look in ./
    // ex: ./grabbag.mid
    {
        Bstrcpy(testfn, fn);
        int32_t const fp = S_TryExtensionReplacements(testfn, searchfirst, ismusic);
        if (fp >= 0)
        {
            Bfree(testfn);
            kclose(origfp);
            return fp;
        }
    }

    // look in ./music/<file's parent GRP name>/
    // ex: ./music/duke3d/grabbag.mid
    // ex: ./music/nwinter/grabbag.mid
    if (origparent != NULL)
    {
        char const * const origparentextension = Bstrrchr(origparent, '.');
        uint32_t namelength = origparentextension != NULL ? (unsigned)(origparentextension - origparent) : origparentlength;

        Bsprintf(testfn, "music/%.*s/%s", namelength, origparent, fn);
        int32_t const fp = S_TryExtensionReplacements(testfn, searchfirst, ismusic);
        if (fp >= 0)
        {
            Bfree(testfn);
            kclose(origfp);
            return fp;
        }
    }

    // look in ./music/
    // ex: ./music/grabbag.mid
    {
        Bsprintf(testfn, "music/%s", fn);
        int32_t const fp = S_TryExtensionReplacements(testfn, searchfirst, ismusic);
        if (fp >= 0)
        {
            Bfree(testfn);
            kclose(origfp);
            return fp;
        }
    }

    Bfree(testfn);
    return origfp;
}

#endif
