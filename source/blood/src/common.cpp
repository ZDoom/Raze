//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

//
// Common non-engine code/data for EDuke32 and Mapster32
//

#include "ns.h"	// Must come before everything else!

#include "compat.h"
#include "build.h"
#include "baselayer.h"
#include "cmdlib.h"
#include "palette.h"
#include "gamecvars.h"

#ifdef _WIN32
# define NEED_SHLWAPI_H
# include "windows_inc.h"
# include "win32/winbits.h"
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

BEGIN_BLD_NS

// g_grpNamePtr can ONLY point to a malloc'd block (length BMAX_PATH)
char *g_grpNamePtr = NULL;

void clearGrpNamePtr(void)
{
    Bfree(g_grpNamePtr);
    // g_grpNamePtr assumed to be assigned to right after
}

const char *G_DefaultGrpFile(void)
{
    return "blood.rff";
}

const char *G_DefaultDefFile(void)
{
    return "blood.def";
}

const char *G_GrpFile(void)
{
    return (g_grpNamePtr == NULL) ? G_DefaultGrpFile() : g_grpNamePtr;
}

const char *G_DefFile(void)
{
    return (g_defNamePtr == NULL) ? G_DefaultDefFile() : g_defNamePtr;
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
    const char *grpfile = G_GrpFile();
    G_TryLoadingGrp(grpfile);

    if (G_AllowAutoload())
    {
        G_LoadGroupsInDir("autoload");

        //if (i != -1)
        //    G_DoAutoload(grpfile);
    }

    if (g_modDir[0] != '/')
        G_LoadGroupsInDir(g_modDir);

    if (g_defNamePtr == NULL)
    {
        const char *tmpptr = getenv("BLOODDEF");
        if (tmpptr)
        {
            clearDefNamePtr();
            g_defNamePtr = dup_filename(tmpptr);
            initprintf("Using \"%s\" as definitions file\n", g_defNamePtr);
        }
    }

    loaddefinitions_game(BLOODWIDESCREENDEF, TRUE);
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

//////////

// loads all group (grp, zip, pk3/4) files in the given directory
void G_LoadGroupsInDir(const char *dirname)
{
    static const char *extensions[] = { "*.grp", "*.zip", "*.ssi", "*.pk3", "*.pk4" };
    char buf[BMAX_PATH];
    fnlist_t fnlist = FNLIST_INITIALIZER;

    for (auto & extension : extensions)
    {
        BUILDVFS_FIND_REC *rec;

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
	char const* const origparent = origfp.isOpen() ? kfileparent(origfp) : NULL;
	uint32_t const    parentlength = origparent != NULL ? Bstrlen(origparent) : 0;

    auto testfn = (char *)Xmalloc(Bstrlen(fn) + 12 + parentlength); // "music/" + overestimation of parent minus extension + ".flac" + '\0'

    // look in ./
    // ex: ./grabbag.mid
    Bstrcpy(testfn, fn);
	auto fp = S_TryExtensionReplacements(testfn, searchfirst, ismusic);
	if (fp.isOpen())
	{
		Bfree(testfn);
		return fp;
	}

    // look in ./music/<file's parent GRP name>/
    // ex: ./music/duke3d/grabbag.mid
    // ex: ./music/nwinter/grabbag.mid
    if (origparent != NULL)
    {
        char const * const parentextension = Bstrrchr(origparent, '.');
        uint32_t const namelength = parentextension != NULL ? (unsigned)(parentextension - origparent) : parentlength;

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

END_BLD_NS
