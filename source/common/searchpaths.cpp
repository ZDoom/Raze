//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) 2019 Christoph Oelckers


This is free software; you can redistribute it and/or
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

#include <filesystem>
#include "i_specialpaths.h"
#include "compat.h"
#include "gameconfigfile.h"
#include "cmdlib.h"
#include "utf8.h"
//
// Search path management
//

namespace fs = std::filesystem;


fs::path AbsolutePath(const char* path)
{
	FString dirpath = MakeUTF8(path);	// convert into clean UTF-8 - the input here may easily be 8 bit encoded.
	fs::path fpath = fs::u8path(dirpath.GetChars());
	return fs::absolute(fpath);
}


void AddSearchPath(TArray<FString>& searchpaths, const char* path)
{
	try
	{
		auto fpath = AbsolutePath(path);
		if (fs::is_directory(fpath))
		{
			FString apath = fpath.u8string().c_str();
			if (searchpaths.Find(apath) == searchpaths.Size())
				searchpaths.Push(apath);
		}
	}
	catch (fs::filesystem_error & err)
	{
	}
}

#ifndef _WIN32
//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

static void G_AddSteamPaths(TArray<FString> &searchpaths, const char *basepath)
{
	FString path;

    // Duke Nukem 3D: Megaton Edition (Steam)
    path.Format("%s/steamapps/common/Duke Nukem 3D/gameroot", basepath);
	AddSearchPath(searchpaths, path);
    path.Format("%s/steamapps/common/Duke Nukem 3D/gameroot/addons/dc", basepath);
	AddSearchPath(searchpaths, path);
    path.Format("%s/steamapps/common/Duke Nukem 3D/gameroot/addons/nw", basepath);
	AddSearchPath(searchpaths, path);
    path.Format("%s/steamapps/common/Duke Nukem 3D/gameroot/addons/vacation", basepath);
	AddSearchPath(searchpaths, path);

    // Duke Nukem 3D (3D Realms Anthology (Steam) / Kill-A-Ton Collection 2015)
#ifdef __APPLE__
    path.Format("%s/steamapps/common/Duke Nukem 3D/Duke Nukem 3D.app/drive_c/Program Files/Duke Nukem 3D", basepath);
	AddSearchPath(searchpaths, path);
#endif

    // NAM (Steam)
#ifdef __APPLE__
    path.Format("%s/steamapps/common/Nam/Nam.app/Contents/Resources/Nam.boxer/C.harddisk/NAM", basepath);
#else
    path.Format("%s/steamapps/common/Nam/NAM", basepath);
#endif
	AddSearchPath(searchpaths, path);

    // WWII GI (Steam)
    path.Format("%s/steamapps/common/World War II GI/WW2GI", basepath);
	AddSearchPath(searchpaths, path);
}

//-------------------------------------------------------------------------
//
// A bare-bones "parser" for Valve's KeyValues VDF format.
// There is no guarantee this will function properly with ill-formed files.
//
//-------------------------------------------------------------------------

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

//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

static void G_ParseSteamKeyValuesForPaths(TArray<FString> &searchpaths, const char *vdf)
{
	FileReader fr;
	if (fr.Open(vdf))
	{
		auto data = fr.Read();
		if (data.Size() == 0) return;
	}
	
	auto vdfvuf = (char*)data.Data();
	auto vdfbufend = vdfbuf + data.Size();

    if (KeyValues_FindParentKey(&vdfbuf, vdfbufend, "LibraryFolders"))
    {
        char *result;
        vdfbuf++;
        while ((result = KeyValues_FindKeyValue(&vdfbuf, vdfbufend, NULL)) != NULL)
            G_AddSteamPaths(searchpaths, result);
    }
}
#endif


#if defined (__FreeBSD__) || defined(__OpenBSD__) || defined (__linux__)

//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

void G_AddExternalSearchPaths(TArray<FString> &searchpaths)
{
	FString path;
    char *homepath = Bgethomedir();

    path.Format("%s/.steam/steam", homepath);
    G_AddSteamPaths(searchpaths, buf);

    path.Format("%s/.steam/steam/steamapps/libraryfolders.vdf", homepath);
    G_ParseSteamKeyValuesForPaths(searchpaths, buf);
}

#elif defined __APPLE__

//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

void G_AddExternalSearchPaths(TArray<FString> &searchpaths)
{
    char *applications[] = { osx_getapplicationsdir(0), osx_getapplicationsdir(1) };
    char *support[] = { osx_getsupportdir(0), osx_getsupportdir(1) };

	FString path;

    char buf[BMAX_PATH];
    int32_t i;

    for (i = 0; i < 2; i++)
    {
        path.Format("%s/Steam", support[i]);
        G_AddSteamPaths(searchpaths, buf);

        path.Format("%s/Steam/steamapps/libraryfolders.vdf", support[i]);
        G_ParseSteamKeyValuesForPaths(searchpaths, buf);

        // Duke Nukem 3D: Atomic Edition (GOG.com)
        path.Format("%s/Duke Nukem 3D.app/Contents/Resources/Duke Nukem 3D.boxer/C.harddisk", applications[i]);
		AddSearchPath(searchpaths, path);
    }

    for (i = 0; i < 2; i++)
    {
        Xfree(applications[i]);
        Xfree(support[i]);
    }
}

#elif defined (_WIN32)

//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

void G_AddExternalSearchPaths(TArray<FString> &searchpaths)
{

    char buf[BMAX_PATH] = {0};
    DWORD bufsize;

    // Duke Nukem 3D: 20th Anniversary World Tour (Steam)
    bufsize = sizeof(buf);
    if (ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 434050)", "InstallLocation", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }

    // Duke Nukem 3D: Megaton Edition (Steam)
    bufsize = sizeof(buf);
    if (ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 225140)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        strncpy(suffix, "/gameroot", remaining);
		AddSearchPath(searchpaths, buf);
        strncpy(suffix, "/gameroot/addons/dc", remaining);
		AddSearchPath(searchpaths, buf);
        strncpy(suffix, "/gameroot/addons/nw", remaining);
		AddSearchPath(searchpaths, buf);
        strncpy(suffix, "/gameroot/addons/vacation", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // Duke Nukem 3D (3D Realms Anthology (Steam) / Kill-A-Ton Collection 2015)
    bufsize = sizeof(buf);
    if (ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 359850)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        strncpy(suffix, "/Duke Nukem 3D", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // Duke Nukem 3D: Atomic Edition (GOG.com)
    bufsize = sizeof(buf);
    if (ReadRegistryValue("SOFTWARE\\GOG.com\\GOGDUKE3D", "PATH", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }

    // Duke Nukem 3D (3D Realms Anthology)
    bufsize = sizeof(buf);
    if (ReadRegistryValue("SOFTWARE\\3DRealms\\Duke Nukem 3D", NULL, buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        strncpy(suffix, "/Duke Nukem 3D", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // 3D Realms Anthology
    bufsize = sizeof(buf);
    if (ReadRegistryValue("SOFTWARE\\3DRealms\\Anthology", NULL, buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/Duke Nukem 3D", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // NAM (Steam)
    bufsize = sizeof(buf);
    if (ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 329650)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/NAM", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // WWII GI (Steam)
    bufsize = sizeof(buf);
    if (ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 376750)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/WW2GI", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // Redneck Rampage (GOG.com)
    bufsize = sizeof(buf);
    if (ReadRegistryValue("SOFTWARE\\GOG.com\\GOGREDNECKRAMPAGE", "PATH", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }

    // Redneck Rampage Rides Again (GOG.com)
    bufsize = sizeof(buf);
    if (ReadRegistryValue("SOFTWARE\\GOG.com\\GOGCREDNECKRIDESAGAIN", "PATH", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }
	
    // Blood: One Unit Whole Blood (Steam)
    bufsize = sizeof(buf);
    if (ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 299030)", "InstallLocation", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }

    // Blood: One Unit Whole Blood (GOG.com)
    bufsize = sizeof(buf);
    if (ReadRegistryValue("SOFTWARE\\GOG.com\\GOGONEUNITONEBLOOD", "PATH", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }

    // Blood: Fresh Supply (Steam)
    bufsize = sizeof(buf);
    if (ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 1010750)", "InstallLocation", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
        strncat(buf, R"(\addons\Cryptic Passage)", 23);
		AddSearchPath(searchpaths, buf);
    }

    // Blood: Fresh Supply (GOG.com)
    bufsize = sizeof(buf);
    if (ReadRegistryValue(R"(SOFTWARE\Wow6432Node\GOG.com\Games\1374469660)", "path", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
        strncat(buf, R"(\addons\Cryptic Passage)", 23);
		AddSearchPath(searchpaths, buf);
    }
}
#endif


//==========================================================================
//
//
//
//==========================================================================

void CollectSubdirectories(TArray<FString> &searchpath, const char *dirmatch)
{
	try
	{
		FString dirpath = MakeUTF8(dirmatch);	// convert into clean UTF-8
		dirpath.Truncate(dirpath.Len() - 2);	// remove the '/*'
		fs::path path = AbsolutePath(dirpath.GetChars());
		if (fs::exists(path) && fs::is_directory(path))
		{
			for (const auto& entry : fs::directory_iterator(path))
			{
				if (fs::is_directory(entry.status()))
				{
					FString newdir = absolute(entry.path()).u8string().c_str();
					if (searchpath.Find(newdir) == searchpath.Size())
						searchpath.Push(newdir);
				}
			}
		}
	}
	catch (fs::filesystem_error &)
	{
		// Just ignore this path if it caused an error.
	}
}

//==========================================================================
//
// CollectSearchPaths
//
// collect all paths in a local array for easier management
//
//==========================================================================

TArray<FString> CollectSearchPaths()
{
	TArray<FString> searchpaths;
	
	if (GameConfig->SetSection("GameSearch.Directories"))
	{
		const char *key;
		const char *value;

		while (GameConfig->NextInSection(key, value))
		{
			if (stricmp(key, "Path") == 0)
			{
				FString nice = NicePath(value);
				if (nice.Len() > 0)
				{
#ifdef _WIN32
					if (isalpha(nice[0] && nice[1] == ':' && nice[2] != '/')) continue;	// ignore drive relative paths because they are meaningless.
#endif
					// A path ending with "/*" means to add all subdirectories.
					if (nice[nice.Len()-2] == '/' && nice[nice.Len()-1] == '*')
					{
						CollectSubdirectories(searchpaths, nice);
					}
					// Checking Steam via a list entry allows easy removal if not wanted.
					else if (nice.CompareNoCase("$STEAM") == 0)
					{
						G_AddExternalSearchPaths(searchpaths);
					}
					else
					{
						AddSearchPath(searchpaths, nice);
					}
				}
			}
		}
	}
	// Unify and remove trailing slashes
	for (auto &str : searchpaths)
	{
		str.Substitute("\\", "/");
		str.Substitute("//", "/");	// Double slashes can happen when constructing paths so just get rid of them here.
		if (str.Back() == '/') str.Truncate(str.Len() - 1);
	}
	return searchpaths;
}

