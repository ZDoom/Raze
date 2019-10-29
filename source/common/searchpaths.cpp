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

//
// Search path management
//


#if defined _WIN32
//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

static int G_ReadRegistryValue(char const * const SubKey, char const * const Value, char * const Output, DWORD * OutputSize)
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
#elif defined (__APPLE__) || defined (__FreeBSD__) || defined(__OpenBSD__) || defined (__linux__)
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
	searchpaths.Push(path);
    path.Format("%s/steamapps/common/Duke Nukem 3D/gameroot/addons/dc", basepath);
	searchpaths.Push(path);
    path.Format("%s/steamapps/common/Duke Nukem 3D/gameroot/addons/nw", basepath);
	searchpaths.Push(path);
    path.Format("%s/steamapps/common/Duke Nukem 3D/gameroot/addons/vacation", basepath);
	searchpaths.Push(path);

    // Duke Nukem 3D (3D Realms Anthology (Steam) / Kill-A-Ton Collection 2015)
#ifdef __APPLE__
    path.Format("%s/steamapps/common/Duke Nukem 3D/Duke Nukem 3D.app/drive_c/Program Files/Duke Nukem 3D", basepath);
	searchpaths.Push(path);
#endif

    // NAM (Steam)
#ifdef __APPLE__
    path.Format("%s/steamapps/common/Nam/Nam.app/Contents/Resources/Nam.boxer/C.harddisk/NAM", basepath);
#else
    path.Format("%s/steamapps/common/Nam/NAM", basepath);
#endif
	searchpaths.Push(path);

    // WWII GI (Steam)
    path.Format("%s/steamapps/common/World War II GI/WW2GI", basepath);
	searchpaths.Push(path);
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
		searchpaths.Push(path);
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
    if (G_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 434050)", "InstallLocation", buf, &bufsize))
    {
		searchpaths.Push(buf);
    }

    // Duke Nukem 3D: Megaton Edition (Steam)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 225140)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        strncpy(suffix, "/gameroot", remaining);
		searchpaths.Push(buf);
        strncpy(suffix, "/gameroot/addons/dc", remaining);
		searchpaths.Push(buf);
        strncpy(suffix, "/gameroot/addons/nw", remaining);
		searchpaths.Push(buf);
        strncpy(suffix, "/gameroot/addons/vacation", remaining);
		searchpaths.Push(buf);
    }

    // Duke Nukem 3D (3D Realms Anthology (Steam) / Kill-A-Ton Collection 2015)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 359850)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        strncpy(suffix, "/Duke Nukem 3D", remaining);
		searchpaths.Push(buf);
    }

    // Duke Nukem 3D: Atomic Edition (GOG.com)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\GOG.com\\GOGDUKE3D", "PATH", buf, &bufsize))
    {
		searchpaths.Push(buf);
    }

    // Duke Nukem 3D (3D Realms Anthology)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\3DRealms\\Duke Nukem 3D", NULL, buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/Duke Nukem 3D", remaining);
		searchpaths.Push(buf);
    }

    // 3D Realms Anthology
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\3DRealms\\Anthology", NULL, buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/Duke Nukem 3D", remaining);
		searchpaths.Push(buf);
    }

    // NAM (Steam)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 329650)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/NAM", remaining);
		searchpaths.Push(buf);
    }

    // WWII GI (Steam)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 376750)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        Bstrncpy(suffix, "/WW2GI", remaining);
		searchpaths.Push(buf);
    }

    // Redneck Rampage (GOG.com)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\GOG.com\\GOGREDNECKRAMPAGE", "PATH", buf, &bufsize))
    {
		searchpaths.Push(buf);
    }

    // Redneck Rampage Rides Again (GOG.com)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\GOG.com\\GOGCREDNECKRIDESAGAIN", "PATH", buf, &bufsize))
    {
		searchpaths.Push(buf);
    }
	
    // Blood: One Unit Whole Blood (Steam)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 299030)", "InstallLocation", buf, &bufsize))
    {
		searchpaths.Push(buf);
    }

    // Blood: One Unit Whole Blood (GOG.com)
    bufsize = sizeof(buf);
    if (G_ReadRegistryValue("SOFTWARE\\GOG.com\\GOGONEUNITONEBLOOD", "PATH", buf, &bufsize))
    {
		searchpaths.Push(buf);
    }

    // Blood: Fresh Supply (Steam)
    bufsize = sizeof(buf);
    if (!found && G_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 1010750)", "InstallLocation", buf, &bufsize))
    {
		searchpaths.Push(buf);
        strncat(buf, R"(\addons\Cryptic Passage)", 23);
		searchpaths.Push(buf);
    }

    // Blood: Fresh Supply (GOG.com)
    bufsize = sizeof(buf);
    if (!found && G_ReadRegistryValue(R"(SOFTWARE\Wow6432Node\GOG.com\Games\1374469660)", "path", buf, &bufsize))
    {
		searchpaths.Push(buf);
        strncat(buf, R"(\addons\Cryptic Passage)", 23);
		searchpaths.Push(buf);
    }
}
#endif

//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

void AddExpandedPath(TArray<FString> &searchpaths, const char *basepath)
{
	
}

#ifdef _WIN32
//==========================================================================
//
// Windows version
//
//==========================================================================

void CollectSubdirectories(TArray<FString> &searchpath, const char *dirmatch)
{
	struct _wfinddata_t fileinfo;
	intptr_t handle;
	FString dirpath;
	int count = 0;
	auto wdirmatch = WideString(dirmatch);

	dirpath.Truncate(dirpath.Len()-1); // remove the '*'
	
	if ((handle = _wfindfirst(wdirmatch, &fileinfo)) == -1)
	{
		// silently ignore non-existent paths
		return;
	}
	else
	{
		do
		{
			if (fileinfo.attrib & _A_HIDDEN)
			{
				// Skip hidden files and directories. (Prevents SVN bookkeeping
				// info from being included.)
				continue;
			}
			FString fi = FString(fileinfo.name);
			if (fileinfo.attrib & _A_SUBDIR)
			{

				if (fi[0] == '.' &&
					(fi[1] == '\0' ||
					 (fi[1] == '.' && fi[2] == '\0')))
				{
					// Do not record . and .. directories.
					continue;
				}
				FString newdir = dirpath + fi;
				count += AddDirectory(newdir);
			}
		} while (_wfindnext(handle, &fileinfo) == 0);
		_findclose(handle);
	}
	return count;
}

#else

//==========================================================================
//
// add_dirs
// 4.4BSD version
//
//==========================================================================

void FDirectory::AddDirectory(const char *dirpath)
{
	char *argv [2] = { NULL, NULL };
	argv[0] = new char[strlen(dirpath)+1];
	strcpy(argv[0], dirpath);
	FTS *fts;
	FTSENT *ent;

	fts = fts_open(argv, FTS_LOGICAL, NULL);
	if (fts == NULL)
	{
		return 0;
	}

	const size_t namepos = strlen(dirpath);
	FString pathfix;

	while ((ent = fts_read(fts)) != NULL)
	{
		if (ent->fts_info != FTS_D)
		{
			// We're only interested in getting directories.
			continue;
		}
		fts_set(fts, ent, FTS_SKIP);
		if (ent->fts_name[0] == '.')
		{
			// Skip hidden directories. (Prevents SVN bookkeeping
			// info from being included.)
		}

		// Some implementations add an extra separator between
		// root of the hierarchy and entity's path.
		// It needs to be removed in order to resolve
		// lumps' relative paths properly.
		const char* path = ent->fts_path;

		if ('/' == path[namepos])
		{
			pathfix = FString(path, namepos);
			pathfix.AppendCStrPart(&path[namepos + 1], ent->fts_pathlen - namepos - 1);

			path = pathfix.GetChars();
		}

		searchpaths.Push(path);
	}
	fts_close(fts);
	delete[] argv[0];
}
#endif

//==========================================================================
//
// CollectSearchPaths
//
// collect all paths in a local array for easier management
//
//==========================================================================

TArray<FString> CollectSearchPaths()
{
	TArray<FString> searchpths;
	
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
					if (isalpha(nice[0] && nice[1] == ':' && nice[2] != '/') continue;	// ignore drive relative paths because they are meaningless.
#endif
					// A path ending with "/*" means to add all subdirectories.
					if (nice[nice.Len()-2] == '/' && nice[nice.Len()-1] == '*')
					{
						AddExpandedPath(searchpaths, nice);
					}
					// Checking Steam via a list entry allows easy removal if not wanted.
					else if (nice.CompareNoCase("$STEAM"))
					{
						G_AddExternalSearchPaths(searchpaths);
					}
					else
					{
						mSearchPaths.Push(nice);
					}
				}
			}
		}
	}
	// Unify and remove trailing slashes
	for (auto &str : mSearchPaths)
	{
		str.Substitute("\\", "/");
		if (str.Back() == '/') str.Truncate(str.Len() - 1);
	}
}

