//
// Common non-engine code/data for EDuke32 and Mapster32
//
#include "ns.h"	// Must come before everything else!

#include "compat.h"
#include "build.h"
#include "baselayer.h"
#include "palette.h"
#include "cmdlib.h"
#include "grpscan.h"
#include "gamecvars.h"
#include "rts.h"
#include "gamecontrol.h"


#include "common.h"
#include "common_game.h"

BEGIN_RR_NS

struct grpfile_t const *g_selectedGrp;

int32_t g_gameType = GAMEFLAG_DUKE;
int     g_addonNum = 0;

// g_gameNamePtr can point to one of: grpfiles[].name (string literal), string
// literal, malloc'd block (XXX: possible leak)
const char *g_gameNamePtr = NULL;

// grp/con handling

static const char *defaultconfilename                = "GAME.CON";
static const char *defaultgamegrp[GAMECOUNT]         = { "DUKE3D.GRP", "REDNECK.GRP", "REDNECK.GRP", "NAM.GRP", "NAPALM.GRP" };
static const char *defaultdeffilename[GAMECOUNT]     = { "duke3d.def", "rr.def", "rrra.def", "nam.def", "napalm.grp" };
static const char *defaultgameconfilename[GAMECOUNT] = { "GAME.CON", "GAME.CON", "GAME.CON", "NAM.CON", "NAPALM.CON" };

const char* G_DefaultGrpFile(void)
{
    if (DUKE)
        return defaultgamegrp[GAME_DUKE];
    else if (RR)
        return defaultgamegrp[GAME_RR];

    return defaultgamegrp[0];
}

const char* G_DefaultDefFile(void)
{
    if (DUKE)
        return defaultdeffilename[GAME_DUKE];
    else if (RRRA)
        return defaultdeffilename[GAME_RRRA];
    else if (RR)
        return defaultdeffilename[GAME_RR];

    return defaultdeffilename[0];
}
const char *G_DefaultConFile(void)
{
    /*if (DUKE && testkopen(defaultgameconfilename[GAME_DUKE],0))
        return defaultgameconfilename[GAME_DUKE];
    else if (WW2GI && testkopen(defaultgameconfilename[GAME_WW2GI],0))
        return defaultgameconfilename[GAME_WW2GI];
    else */if (NAPALM)
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
    return userConfig.gamegrp.IsNotEmpty()? userConfig.gamegrp.GetChars() : G_DefaultGrpFile();
}

const char *G_DefFile(void)
{
	return userConfig.DefaultDef.IsNotEmpty() ? userConfig.DefaultDef.GetChars() : G_DefaultDefFile();
}

const char *G_ConFile(void)
{
	return userConfig.DefaultCon.IsNotEmpty() ? userConfig.DefaultCon.GetChars() : G_DefaultConFile();
}

//////////

// Set up new-style multi-psky handling.
void G_InitMultiPsky(int CLOUDYOCEAN__DYN, int MOONSKY1__DYN, int BIGORBIT1__DYN, int LA__DYN)
{
    // When adding other multi-skies, take care that the tileofs[] values are
    // <= PSKYOFF_MAX. (It can be increased up to MAXPSKYTILES, but should be
    // set as tight as possible.)

    // The default sky properties (all others are implicitly zero):
    psky_t *sky      = tileSetupSky(DEFAULTPSKY);
    sky->lognumtiles = 3;
    sky->horizfrac   = 32768;

    // CLOUDYOCEAN
    // Aligns with the drawn scene horizon because it has one itself.
    sky              = tileSetupSky(CLOUDYOCEAN__DYN);
    sky->lognumtiles = 3;
    sky->horizfrac   = 65536;

    // MOONSKY1
    //        earth          mountain   mountain         sun
    sky              = tileSetupSky(MOONSKY1__DYN);
    sky->lognumtiles = 3;
    sky->horizfrac   = 32768;
    sky->tileofs[6]  = 1;
    sky->tileofs[1]  = 2;
    sky->tileofs[4]  = 2;
    sky->tileofs[2]  = 3;

    // BIGORBIT1   // orbit
    //       earth1         2           3           moon/sun
    sky              = tileSetupSky(BIGORBIT1__DYN);
    sky->lognumtiles = 3;
    sky->horizfrac   = 32768;
    sky->tileofs[5]  = 1;
    sky->tileofs[6]  = 2;
    sky->tileofs[7]  = 3;
    sky->tileofs[2]  = 4;

    // LA // la city
    //       earth1         2           3           moon/sun
    sky              = tileSetupSky(LA__DYN);
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

static void G_LoadAddon(void);
int32_t g_groupFileHandle;
struct strllist* CommandGrps;

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

void G_LoadGroups()
{
    if (g_modDir[0] != '/')
    {
        char cwd[BMAX_PATH];

		FString g_rootDir = progdir + g_modDir;
		addsearchpath(g_rootDir);
        //        addsearchpath(mod_dir);

        char path[BMAX_PATH];

        if (getcwd(cwd, BMAX_PATH))
        {
            Bsnprintf(path, sizeof(path), "%s/%s", cwd, g_modDir);
            if (!Bstrcmp(g_rootDir, path))
            {
                if (addsearchpath(path) == -2)
                    if (Bmkdir(path, S_IRWXU) == 0)
                        addsearchpath(path);
            }
        }

    }

    if (g_addonNum)
        G_LoadAddon();

    const char *grpfile;
    int32_t i;

    if ((i = G_LoadGrpDependencyChain(g_selectedGrp)) != -1)
    {
        grpfile = g_selectedGrp->filename;

        //clearGrpNamePtr();
        //g_grpNamePtr = dup_filename(grpfile);

        grpinfo_t const * const type = g_selectedGrp->type;

        g_gameType = type->game;
        g_gameNamePtr = type->name;

        //if (type->scriptname && g_scriptNamePtr == NULL)
          //  g_scriptNamePtr = dup_filename(type->scriptname);

        if (type->defname && g_defNamePtr == NULL)
            g_defNamePtr = dup_filename(type->defname);

		if (type->rtsname)
			RTS_Init(type->rtsname);
    }
    else
    {
        grpfile = G_GrpFile();
        i = G_TryLoadingGrp(grpfile);
    }

    if (G_AllowAutoload())
    {
        G_LoadGroupsInDir("autoload");

        if (i != -1)
            G_DoAutoload(grpfile);
    }

    if (g_modDir[0] != '/')
        G_LoadGroupsInDir(g_modDir);

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
            if (G_AllowAutoload())
                G_DoAutoload(CommandGrps->str);
        }

        Bfree(CommandGrps->str);
        Bfree(CommandGrps);
        CommandGrps = s;
    }
    pathsearchmode = bakpathsearchmode;
}


static void G_LoadAddon(void)
{
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
}




//////////


// loads all group (grp, zip, pk3/4) files in the given directory
void G_LoadGroupsInDir(const char *dirname)
{
    static const char *extensions[] = { "*.grp", "*.zip", "*.ssi", "*.pk3", "*.pk4" };
    char buf[BMAX_PATH];
    fnlist_t fnlist = FNLIST_INITIALIZER;

    for (auto & extension : extensions)
    {
        CACHE1D_FIND_REC *rec;

        fnlist_getnames(&fnlist, dirname, extension, -1, 0);

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
    int32_t j;

	auto fr = kopenFileReader("lookup.dat", 0);
	if (!fr.isOpen())
           return;

    j = paletteLoadLookupTable(fr);

    if (j < 0)
    {
        if (j == -1)
            initprintf("ERROR loading \"lookup.dat\": failed reading enough data.\n");

        return;
    }

    uint8_t paldata[768];

    for (j=1; j<=5; j++)
    {
        // Account for TITLE and REALMS swap between basepal number and on-disk order.
        int32_t basepalnum = (j == 3 || j == 4) ? 4+3-j : j;

		if (fr.Read(paldata, 768) != 768)
			return;

        for (bssize_t k = 0; k < 768; k++)
            paldata[k] <<= 2;

        paletteSetColorTable(basepalnum, paldata);
    }

    Bmemcpy(paldata, palette+1, 767);
    paldata[767] = palette[767];
    paletteSetColorTable(DRUGPAL, paldata);

    if (RR)
    {
        char table[256];
        for (bssize_t i = 0; i < 256; i++)
            table[i] = i;
        for (bssize_t i = 0; i < 32; i++)
            table[i] = i+32;

        paletteMakeLookupTable(7, table, 0, 0, 0, 0);

        for (bssize_t i = 0; i < 256; i++)
            table[i] = i;
        paletteMakeLookupTable(30, table, 0, 0, 0, 0);
        paletteMakeLookupTable(31, table, 0, 0, 0, 0);
        paletteMakeLookupTable(32, table, 0, 0, 0, 0);
        paletteMakeLookupTable(33, table, 0, 0, 0, 0);
        if (RRRA)
            paletteMakeLookupTable(105, table, 0, 0, 0, 0);

        j = 63;
        for (bssize_t i = 64; i < 80; i++)
        {
            j--;
            table[i] = j;
            table[i+16] = i-24;
        }
        table[80] = 80;
        table[81] = 81;
        for (bssize_t i = 0; i < 32; i++)
            table[i] = i+32;
        paletteMakeLookupTable(34, table, 0, 0, 0, 0);
        for (bssize_t i = 0; i < 256; i++)
            table[i] = i;
        for (bssize_t i = 0; i < 16; i++)
            table[i] = i+129;
        for (bssize_t i = 16; i < 32; i++)
            table[i] = i+192;
        paletteMakeLookupTable(35, table, 0, 0, 0, 0);
        if (RRRA)
            paletteMakeLookupTable(54, palookup[8], 32*4, 32*4, 32*4, 0);
    }
}

//////////

#ifdef FORMAT_UPGRADE_ELIGIBLE

static FileReader S_TryFormats(char * const testfn, char * const fn_suffix, char const searchfirst)
{
#ifdef HAVE_FLAC
    {
        Bstrcpy(fn_suffix, ".flac");
        auto fp = kopenFileReader(testfn, searchfirst);
        if (fp.isOpen())
            return fp;
    }
#endif

#ifdef HAVE_VORBIS
    {
        Bstrcpy(fn_suffix, ".ogg");
		auto fp = kopenFileReader(testfn, searchfirst);
		if (fp.isOpen())
			return fp;
	}
#endif

    return FileReader();
}

static FileReader S_TryExtensionReplacements(char * const testfn, char const searchfirst, uint8_t const ismusic)
{
    char * extension = Bstrrchr(testfn, '.');
    char * const fn_end = Bstrchr(testfn, '\0');

    // ex: grabbag.voc --> grabbag_voc.*
    if (extension != NULL)
    {
        *extension = '_';

        auto fp = S_TryFormats(testfn, fn_end, searchfirst);
        if (fp.isOpen())
            return fp;
    }
    else
    {
        extension = fn_end;
    }

    // ex: grabbag.mid --> grabbag.*
    if (ismusic)
    {
        auto fp = S_TryFormats(testfn, extension, searchfirst);
		if (fp.isOpen())
			return fp;
    }

    return FileReader();
}

FileReader S_OpenAudio(const char *fn, char searchfirst, uint8_t const ismusic)
{
    auto origfp = kopenFileReader(fn, searchfirst);
    char const * const origparent = origfp.isOpen() ? kfileparent(origfp) : NULL;
    uint32_t const origparentlength = origparent != NULL ? Bstrlen(origparent) : 0;

    char * const testfn = (char *)Xmalloc(Bstrlen(fn) + 12 + origparentlength); // "music/" + overestimation of parent minus extension + ".flac" + '\0'

    // look in ./
    // ex: ./grabbag.mid
    {
        Bstrcpy(testfn, fn);
        auto fp = S_TryExtensionReplacements(testfn, searchfirst, ismusic);
		if (fp.isOpen())
		{
            Bfree(testfn);
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
        auto fp = S_TryExtensionReplacements(testfn, searchfirst, ismusic);
		if (fp.isOpen())
		{
            Bfree(testfn);
            return fp;
        }
    }

    // look in ./music/
    // ex: ./music/grabbag.mid
    {
        Bsprintf(testfn, "music/%s", fn);
        auto fp = S_TryExtensionReplacements(testfn, searchfirst, ismusic);
		if (fp.isOpen())
		{
            Bfree(testfn);
            return fp;
        }
    }

    Bfree(testfn);
    return origfp;
}

#endif

END_RR_NS
