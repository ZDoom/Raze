/*
** searchpaths.cpp
**
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "m_crc32.h"
#include "i_specialpaths.h"
#include "i_system.h"
#include "gameconfigfile.h"
#include "cmdlib.h"
#include "utf8.h"
#include "sc_man.h"
#include "resourcefile.h"
#include "printf.h"
#include "version.h"
#include "gamecontrol.h"
#include "m_argv.h"
#include "filesystem.h"
#include "findfile.h"

static const char* res_exts[] = { ".grp", ".zip", ".pk3", ".pk4", ".7z", ".pk7" };

int g_gameType;



void AddSearchPath(TArray<FString>& searchpaths, const char* path)
{
	auto fpath = M_GetNormalizedPath(path);
	if (fpath.Len() > 1 && fpath.Back() == '/') fpath.Truncate(fpath.Len() - 1);
	if (DirExists(fpath))
	{
		if (searchpaths.Find(fpath) == searchpaths.Size())
			searchpaths.Push(fpath);
	}
}

#ifndef _WIN32

void G_AddExternalSearchPaths(TArray<FString> &searchpaths)
{
	searchpaths.Append(I_GetSteamPath());
	searchpaths.Append(I_GetGogPaths());
}


#else
//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

struct RegistryPathInfo
{
	const wchar_t *regPath;
	const wchar_t *regKey;
	const char **subpaths;
};

static const char * gameroot[] = { "/gameroot", nullptr};
static const char * swaddons[] = { "/gameroot/addons", "/gameroot", nullptr};
static const char * dukeaddons[] = { "/gameroot", "/gameroot/addons/dc", "/gameroot/addons/nw", "/gameroot/addons/vacation", nullptr};
static const char * dn3d[] = { "/Duke Nukem 3D", nullptr};
static const char * nam[] = { "/NAM", nullptr};
static const char * ww2gi[] = { "/WW2GI", nullptr};
static const char * bloodfs[] = { "", R"(/addons/Cryptic Passage)", nullptr};
static const char * sw[] = { "/Shadow Warrior", nullptr};
static const char * redneck[] = { "/Redneck", "/AGAIN", "/HUNTIN", nullptr };
static const char * dukezoom[] = { "/", "/AddOns", nullptr };

#ifndef _WIN64
#define WOW64 "\\"
#else
#define WOW64 "\\Wow6432Node\\"
#endif

static const RegistryPathInfo paths[] = {
	{ L"SOFTWARE" WOW64 "GOG.com\\GOGDUKE3D", L"PATH", nullptr },
	{ L"SOFTWARE" WOW64 "GOG.com\\GOGREDNECKRAMPAGE", L"PATH", nullptr },
	{ L"SOFTWARE" WOW64 "GOG.com\\GOGCREDNECKRIDESAGAIN", L"PATH", nullptr },
	{ L"SOFTWARE" WOW64 "GOG.com\\GOGONEUNITONEBLOOD", L"PATH", nullptr},
	{ L"SOFTWARE" WOW64 "GOG.com\\GOGSHADOWARRIOR", L"PATH", nullptr},
	{ L"SOFTWARE" WOW64 "GOG.com\\Games\\1374469660", L"path", bloodfs},
	{ L"SOFTWARE" WOW64 "GOG.com\\Games\\1740836875", L"path", nullptr},
	{ L"SOFTWARE" WOW64 "GOG.com\\Games\\2132611980", L"path", nullptr}, // Powerslave

	{ L"SOFTWARE" WOW64 "ZOOM PLATFORM\\Duke Nukem 3D - Atomic Edition", L"InstallPath", dukezoom },

	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 434050", L"InstallLocation", nullptr },
	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 225140", L"InstallLocation", dukeaddons },
	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 359850", L"InstallLocation", dn3d },
	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 329650", L"InstallLocation", nam },
	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 376750", L"InstallLocation", ww2gi },
	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 299030", L"InstallLocation", nullptr }, // Blood: One Unit Whole Blood (Steam)
	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1010750",L"InstallLocation", bloodfs},
	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 225160", L"InstallLocation", swaddons }, // Shadow Warrior Classic Redux - Steam
	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 238070", L"InstallLocation", gameroot}, // Shadow Warrior Classic (1997) - Steam
	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 358400", L"InstallLocation", sw},

	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 565550", L"InstallLocation", redneck},
	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 580940", L"InstallLocation", redneck},
	{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 580930", L"InstallLocation", redneck},
#if 0
	{ L"SOFTWARE\\3DRealms\\Duke Nukem 3D", nullptr, dn3d },
	{ L"SOFTWARE\\3DRealms\\Anthology", nullptr, dn3d },
	{ L"SOFTWARE\\3DRealms\\Shadow Warrior", nullptr, sw},
	{ L"SOFTWARE\\3DRealms\\Anthology", nullptr, sw},
#endif

	{ nullptr}
};

bool I_QueryPathKey(const wchar_t* keypath, const wchar_t* valname, FString& value);

void G_AddExternalSearchPaths(TArray<FString> &searchpaths)
{
	for (auto &entry : paths)
	{
		// 3D Realms Anthology
		FString buf;
		if (I_QueryPathKey(entry.regPath, entry.regKey, buf))
		{
			FixPathSeperator(buf);
			if (!entry.subpaths) AddSearchPath(searchpaths, buf);
			else
			{
				FString path;
				for (int i = 0; entry.subpaths[i]; i++)
				{
					path.Format("%s%s", buf, entry.subpaths[i]);
					AddSearchPath(searchpaths, path);
				}
			}
		}
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
	FString dirpath = MakeUTF8(dirmatch);	// convert into clean UTF-8
	dirpath.Truncate(dirpath.Len() - 2);	// remove the '/*'
	FString AbsPath = M_GetNormalizedPath(dirpath);
	if (DirExists(AbsPath))
	{
		findstate_t findstate;
		void* handle;
		if ((handle = I_FindFirst(AbsPath + "/*", &findstate)) != (void*)-1)
		{
			do
			{
				if (I_FindAttr(&findstate) & FA_DIREC)
				{
					auto p = I_FindName(&findstate);
					if (strcmp(p, ".") && strcmp(p, ".."))
					{
						FStringf fullpath("%s/%s", AbsPath.GetChars(), p);
						searchpath.Push(fullpath);
					}
				}
			} while (I_FindNext(handle, &findstate) == 0);
			I_FindClose(handle);
		}
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
				// Checking Steam via a list entry allows easy removal if not wanted.
				if (stricmp(value, "$STEAM") == 0)
				{
					G_AddExternalSearchPaths(searchpaths);
					continue;
				}

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
#ifdef _WIN32
		bool isUNCpath = false;
		auto chars = str.GetChars();
		if (chars[0] == '/' && chars[1] == '/')
			isUNCpath = true;
#endif
		str.Substitute("//", "/");	// Double slashes can happen when constructing paths so just get rid of them here.
		if (str.Back() == '/') str.Truncate(str.Len() - 1);
#ifdef _WIN32
		if (isUNCpath)
			str = (FString)"/" + str;
#endif
	}
	return searchpaths;
}

//==========================================================================
//
//
//
//==========================================================================

struct FileEntry
{
	FString FileName;
	size_t FileLength;
	time_t FileTime;
	uint32_t CRCValue;
	uint32_t Index;
};

TArray<FileEntry> CollectAllFilesInSearchPath()
{
	uint32_t index = 0;
	TArray<FileEntry> filelist;

	if (userConfig.gamegrp.IsNotEmpty())
	{
		// If the user specified a file on the command line, insert that first, if found.
		FileReader fr;
		if (fr.OpenFile(userConfig.gamegrp))
		{
			FileEntry fe = { userConfig.gamegrp, (size_t)fr.GetLength(), 0, 0, index++ };
			filelist.Push(fe);
		}
	}

	auto paths = CollectSearchPaths();
	for(auto &path : paths)
	{
		if (DirExists(path))
		{
			findstate_t findstate;
			void* handle;
			if ((handle = I_FindFirst(path + "/*.*", &findstate)) != (void*)-1)
			{
				do
				{
					if (!(I_FindAttr(&findstate) & FA_DIREC))
					{
						auto p = I_FindName(&findstate);
						filelist.Reserve(1);
						auto& flentry = filelist.Last();
						flentry.FileName.Format("%s/%s", path.GetChars(), p);
						GetFileInfo(flentry.FileName, &flentry.FileLength, &flentry.FileTime);
						flentry.Index = index++; // to preserve order when working on the list.
					}
				} while (I_FindNext(handle, &findstate) == 0);
				I_FindClose(handle);
			}
		}
	}
	return filelist;
}

//==========================================================================
//
//
//
//==========================================================================

static TArray<FileEntry> LoadCRCCache(void)
{
	auto cachepath = M_GetAppDataPath(false) + "/grpcrccache.txt";
	FScanner sc;
	TArray<FileEntry> crclist;

	try
	{
		if (sc.OpenFile(cachepath))
		{
			while (sc.GetString())
			{
				crclist.Reserve(1);
				auto &flentry = crclist.Last();
				flentry.FileName = sc.String;
				sc.MustGetNumber();
				flentry.FileLength = sc.BigNumber;
				sc.MustGetNumber();
				flentry.FileTime = sc.BigNumber;
				sc.MustGetNumber();
				flentry.CRCValue = (unsigned)sc.BigNumber;
			}
		}
	}
	catch (CRecoverableError &)
	{
		// If there's a parsing error, return what we got and discard the rest.
	}
	return crclist;
}

//==========================================================================
//
//
//
//==========================================================================

void SaveCRCs(TArray<FileEntry>& crclist)
{
	auto cachepath = M_GetAppDataPath(true) + "/grpcrccache.txt";

	FileWriter* fw = FileWriter::Open(cachepath);
	if (fw)
	{
		for (auto& crc : crclist)
		{
			FStringf line("\"%s\" %llu %llu %u\n", crc.FileName.GetChars(), (long long)crc.FileLength, (long long)crc.FileTime, crc.CRCValue);
			fw->Write(line.GetChars(), line.Len());
		}
		delete fw;
	}
}

//==========================================================================
//
//
//
//==========================================================================

static TArray<GrpInfo> ParseGrpInfo(const char *fn, FileReader &fr, TMap<FString, uint32_t> &CRCMap)
{
	TArray<GrpInfo> groups;
	TMap<FString, int> FlagMap;
	
	FlagMap.Insert("GAMEFLAG_DUKE", GAMEFLAG_DUKE);
	FlagMap.Insert("GAMEFLAG_NAM", GAMEFLAG_NAM);
	FlagMap.Insert("GAMEFLAG_NAPALM", GAMEFLAG_NAPALM);
	FlagMap.Insert("GAMEFLAG_WW2GI", GAMEFLAG_WW2GI);
	FlagMap.Insert("GAMEFLAG_ADDON", GAMEFLAG_ADDON);
	FlagMap.Insert("GAMEFLAG_SHAREWARE", GAMEFLAG_SHAREWARE);
	FlagMap.Insert("GAMEFLAG_DUKEBETA", GAMEFLAG_DUKEBETA); // includes 0x20 since it's a shareware beta
	FlagMap.Insert("GAMEFLAG_RR", GAMEFLAG_RR);
	FlagMap.Insert("GAMEFLAG_RRRA", GAMEFLAG_RRRA);
	FlagMap.Insert("GAMEFLAG_BLOOD", GAMEFLAG_BLOOD);
	FlagMap.Insert("GAMEFLAG_SW", GAMEFLAG_SW);
	FlagMap.Insert("GAMEFLAG_POWERSLAVE", GAMEFLAG_POWERSLAVE);
	FlagMap.Insert("GAMEFLAG_EXHUMED", GAMEFLAG_EXHUMED);
	FlagMap.Insert("GAMEFLAG_DUKEDC", GAMEFLAG_DUKEDC);
	FlagMap.Insert("GAMEFLAG_DUKENW", GAMEFLAG_DUKENW);
	FlagMap.Insert("GAMEFLAG_DUKEVACA", GAMEFLAG_DUKEVACA);
	FlagMap.Insert("GAMEFLAG_BLOODCP", GAMEFLAG_BLOODCP);
	FlagMap.Insert("GAMEFLAG_ROUTE66", GAMEFLAG_ROUTE66);
	FlagMap.Insert("GAMEFLAG_SWWANTON", GAMEFLAG_SWWANTON);
	FlagMap.Insert("GAMEFLAG_SWTWINDRAG", GAMEFLAG_SWTWINDRAG);

	FScanner sc;
	auto mem = fr.Read();
	sc.OpenMem(fn, (const char *)mem.Data(), mem.Size());
	
	while (sc.GetToken())
	{
		sc.TokenMustBe(TK_Identifier);
		if (sc.Compare("CRC"))
		{
			sc.MustGetToken('{');
			while (!sc.CheckToken('}'))
			{
				sc.MustGetToken(TK_Identifier);
				FString key = sc.String;
				sc.MustGetToken(TK_IntConst);
				if (sc.BigNumber < 0 || sc.BigNumber >= UINT_MAX)
				{
					sc.ScriptError("CRC hash %s out of range", sc.String);
				}
				CRCMap.Insert(key, (uint32_t)sc.BigNumber);
			}
		}
		else if (sc.Compare("grpinfo") || sc.Compare("addon"))
		{
			groups.Reserve(1);
			auto& grp = groups.Last();
			grp.isAddon = sc.Compare("addon");
			sc.MustGetToken('{');
			while (!sc.CheckToken('}'))
			{
				sc.MustGetToken(TK_Identifier);
				if (sc.Compare("name"))
				{
					sc.MustGetToken(TK_StringConst);
					grp.name = sc.String;
				}
				else if (sc.Compare("scriptname"))
				{
					sc.MustGetToken(TK_StringConst);
					grp.scriptname = sc.String;
				}
				else if (sc.Compare("loaddirectory"))
				{
					// Only load the directory, but not the file.
					grp.loaddirectory = true;
				}
				else if (sc.Compare("defname"))
				{
					sc.MustGetToken(TK_StringConst);
					grp.defname = sc.String;
				}
				else if (sc.Compare("rtsname"))
				{
					sc.MustGetToken(TK_StringConst);
					grp.rtsname = sc.String;
				}
				else if (sc.Compare("gamefilter"))
				{
					sc.MustGetToken(TK_StringConst);
					grp.gamefilter = sc.String;
				}
				else if (sc.Compare("gameid"))
				{
					sc.MustGetToken(TK_StringConst);
					grp.gameid = sc.String;
				}
				else if (sc.Compare("fgcolor"))
				{
					sc.MustGetToken(TK_IntConst);
					grp.FgColor = sc.Number;
				}
				else if (sc.Compare("bkcolor"))
				{
					sc.MustGetToken(TK_IntConst);
					grp.BgColor = sc.Number;
				}
				else if (sc.Compare("crc"))
				{
					sc.MustGetAnyToken();
					if (sc.TokenType == TK_IntConst)
					{
						grp.CRC = (uint32_t)sc.BigNumber;
					}
					else if (sc.TokenType == '-')
					{
						sc.MustGetToken(TK_IntConst);
						grp.CRC = (uint32_t)-sc.BigNumber;
					}
					else if (sc.TokenType == TK_Identifier)
					{
						auto ip = CRCMap.CheckKey(sc.String);
						if (ip) grp.CRC = *ip;
						else sc.ScriptError("Unknown CRC value %s", sc.String);
					}
					else sc.TokenMustBe(TK_IntConst);
				}
				else if (sc.Compare("dependency"))
				{
					sc.MustGetAnyToken();
					if (sc.TokenType == TK_IntConst)
					{
						grp.dependencyCRC = (uint32_t)sc.BigNumber;
					}
					else if (sc.TokenType == '-')
					{
						sc.MustGetToken(TK_IntConst);
						grp.dependencyCRC = (uint32_t)-sc.BigNumber;
					}
					else if (sc.TokenType == TK_Identifier)
					{
						auto ip = CRCMap.CheckKey(sc.String);
						if (ip) grp.dependencyCRC = *ip;
						else sc.ScriptError("Unknown CRC value %s", sc.String);
					}
					else sc.TokenMustBe(TK_IntConst);
				}
				else if (sc.Compare("size"))
				{
					sc.MustGetToken(TK_IntConst);
					grp.size = sc.BigNumber;
				}
				else if (sc.Compare("flags"))
				{
					do
					{
						sc.MustGetAnyToken();
						if (sc.TokenType == TK_IntConst)
						{
							grp.flags |= sc.Number;
						}
						else if (sc.TokenType == TK_Identifier)
						{
							auto ip = FlagMap.CheckKey(sc.String);
							if (ip) grp.flags |= *ip;
							else sc.ScriptError("Unknown flag value %s", sc.String);
						}
						else sc.TokenMustBe(TK_IntConst);
					}
					while (sc.CheckToken('|'));
				}
				else if (sc.Compare("mustcontain"))
				{
					do
					{
						sc.MustGetToken(TK_StringConst);
						grp.mustcontain.Push(sc.String);
					}
					while (sc.CheckToken(','));
				}
				else if (sc.Compare("deletecontent"))
				{
					do
					{
						sc.MustGetToken(TK_StringConst);
						grp.tobedeleted.Push(sc.String);
					} while (sc.CheckToken(','));
				}
				else if (sc.Compare("loadgrp"))
				{
				do
				{
					sc.MustGetToken(TK_StringConst);
					grp.loadfiles.Push(sc.String);
				} while (sc.CheckToken(','));
				}
				else if (sc.Compare("loadart"))
				{
					do
					{
						sc.MustGetToken(TK_StringConst);
						grp.loadart.Push(sc.String);
					}
					while (sc.CheckToken(','));
				}
				else if (sc.Compare("exclepisodes"))
				{
					do
					{
						sc.MustGetToken(TK_StringConst);
						grp.exclepisodes.Push(sc.String);
					}
					while (sc.CheckToken(','));
				}
				else sc.ScriptError(nullptr);
			}
			if (grp.dependencyCRC == 0 && (grp.flags & (GAMEFLAG_DUKE | GAMEFLAG_NAM | GAMEFLAG_NAPALM | GAMEFLAG_WW2GI | GAMEFLAG_RRALL | GAMEFLAG_BLOOD | GAMEFLAG_SW | GAMEFLAG_PSEXHUMED)) == 0)
			{
				sc.ScriptMessage("Warning: GRP without game defined. Ignoring");
				groups.Pop();
			}
		}
		else sc.ScriptError(nullptr);
	}
	return groups;
}

//==========================================================================
//
//
//
//==========================================================================

TArray<GrpInfo> ParseAllGrpInfos(TArray<FileEntry>& filelist)
{
	TArray<GrpInfo> groups;
	TMap<FString, uint32_t> CRCMap;
	// This opens the base resource only for reading the grpinfo from it which we need before setting up the game state.
	std::unique_ptr<FResourceFile> engine_res;
	const char* baseres = BaseFileSearch(ENGINERES_FILE, nullptr, true, GameConfig);
	engine_res.reset(FResourceFile::OpenResourceFile(baseres, true, true));
	if (engine_res)
	{
		auto basegrp = engine_res->FindLump("engine/grpinfo.txt");
		if (basegrp)
		{
			auto fr = basegrp->NewReader();
			if (fr.isOpen())
			{
				groups = ParseGrpInfo("engine/grpinfo.txt", fr, CRCMap);
			}
		}
	}
	for (auto& entry : filelist)
	{
		auto lowerstr = entry.FileName.MakeLower();
		if (lowerstr.Len() >= 8)
		{
			const char* exten = lowerstr.GetChars() + lowerstr.Len() - 8;
			if (!stricmp(exten, ".grpinfo"))
			{
				// parse it.
				FileReader fr;
				if (fr.OpenFile(entry.FileName))
				{
					auto g = ParseGrpInfo(entry.FileName, fr, CRCMap);
					groups.Append(g);
				}
			}
		}
	}
	return groups;
}

//==========================================================================
//
//
//
//==========================================================================
					
void GetCRC(FileEntry *entry, TArray<FileEntry> &CRCCache)
{
	for (auto &ce : CRCCache)
	{
		// File size, modification date snd name all must match exactly to pick an entry.
		if (entry->FileLength == ce.FileLength && entry->FileTime == ce.FileTime && entry->FileName.Compare(ce.FileName) == 0)
		{
			entry->CRCValue = ce.CRCValue;
			return;
		}
	}
	FileReader f;
	if (f.OpenFile(entry->FileName))
	{
		TArray<uint8_t> buffer(65536, 1);
		uint32_t crcval = 0;
		size_t b;
		do
		{
			b = f.Read(buffer.Data(), buffer.Size());
			if (b > 0) crcval = AddCRC32(crcval, buffer.Data(), unsigned(b));
		}
		while (b == buffer.Size());
		entry->CRCValue = crcval;
		CRCCache.Push(*entry);
	}
}

GrpInfo *IdentifyGroup(FileEntry *entry, TArray<GrpInfo *> &groups)
{
	for (auto g : groups)
	{
		if (entry->FileLength == g->size && entry->CRCValue == g->CRC)
			return g;
	}
	return nullptr;
}

//==========================================================================
//
//
//
//==========================================================================

static bool CheckAddon(GrpInfo* addon, GrpInfo* main, const char* filename)
{
	if (addon->dependencyCRC != main->CRC) return false;
	FString path = ExtractFilePath(filename);
	if (path.Len() > 0 && path.Back() != '/') path += '/';
	for (auto& fn : addon->mustcontain)
	{
		FString check = path + fn;
		if (FileExists(check)) continue;
#if !defined _WIN32 
		check = path + fn.MakeLower();
		if (FileExists(check)) continue;
		check = path + fn.MakeUpper();
		if (FileExists(check)) continue;
#endif
		return false;
	}
	return true;
}

//==========================================================================
//
//
//
//==========================================================================

TArray<GrpEntry> GrpScan()
{
	TArray<GrpEntry> foundGames;

	TArray<FileEntry*> sortedFileList;
	TArray<GrpInfo*> sortedGroupList;
	TArray<GrpInfo*> contentGroupList;
	TArray<GrpInfo*> addonList;

	auto allFiles = CollectAllFilesInSearchPath();
	auto allGroups = ParseAllGrpInfos(allFiles);

	auto cachedCRCs = LoadCRCCache();
	auto numCRCs = cachedCRCs.Size();

	for (unsigned i = 0; i < allGroups.Size(); i++)
		allGroups[i].index = i;

	// Remove all unnecessary content from the file list. Since this contains all data from the search path's directories it can be quite large.
	// Sort both lists by file size so that we only need to pass over each list once to weed out all unrelated content. Go backward to avoid too much item movement
	// (most will be deleted anyway.)


	for (auto& f : allFiles) sortedFileList.Push(&f);
	for (auto& g : allGroups)
	{
		if (g.isAddon)
			addonList.Push(&g);
		else if (g.CRC == 0 && g.mustcontain.Size() > 0)
			contentGroupList.Push(&g);
		else
			sortedGroupList.Push(&g);
	}

	// As a first pass we need to look for all known game resources which only are identified by a content list
	if (contentGroupList.Size())
	{
		for (auto fe : sortedFileList)
		{
			FString fn = fe->FileName.MakeLower();
			for (auto ext : res_exts)
			{
				if (strcmp(ext, fn.GetChars() + fn.Len() - 4) == 0)
				{
					auto resf = FResourceFile::OpenResourceFile(fe->FileName, true, true);
					if (resf)
					{
						for (auto grp : contentGroupList)
						{
							bool ok = true;
							for (auto &lump : grp->mustcontain)
							{
								if (!resf->FindLump(lump))
								{
									ok = false;
									break;
								}
							}
							if (ok)
							{
								// got a match
								foundGames.Reserve(1);
								auto& fg = foundGames.Last();
								fg.FileInfo = *grp;
								fg.FileName = fe->FileName;
								break;
							}
						}
						delete resf;
					}
				}
			}
		}
	}


	std::sort(sortedFileList.begin(), sortedFileList.end(), [](FileEntry* lhs, FileEntry* rhs) { return lhs->FileLength < rhs->FileLength; });
	std::sort(sortedGroupList.begin(), sortedGroupList.end(), [](GrpInfo* lhs, GrpInfo* rhs) { return lhs->size < rhs->size; });

	int findex = sortedFileList.Size() - 1;
	int gindex = sortedGroupList.Size() - 1;


	while (findex >= 0 && gindex >= 0)
	{
		if (sortedFileList[findex]->FileLength > sortedGroupList[gindex]->size)
		{
			// File is larger than the largest known group so it cannot be a candidate.
			sortedFileList.Delete(findex--);
		}
		else if (sortedFileList[findex]->FileLength < sortedGroupList[gindex]->size)
		{
			// The largest available file is smaller than this group so we cannot possibly have it.
			sortedGroupList.Delete(gindex--);
		}
		else
		{
			findex--;
			gindex--;
			// We found a matching file. Skip over all other entries of the same size so we can analyze those later as well
			while (findex >= 0 && sortedFileList[findex]->FileLength == sortedFileList[findex + 1]->FileLength) findex--;
			while (gindex >= 0 && sortedGroupList[gindex]->size == sortedGroupList[gindex + 1]->size) gindex--;
		}
	}
	sortedFileList.Delete(0, findex + 1);
	sortedGroupList.Delete(0, gindex + 1);

	for (auto entry : sortedFileList)
	{
		GetCRC(entry, cachedCRCs);
		auto grp = IdentifyGroup(entry, sortedGroupList);
		if (grp)
		{
			foundGames.Reserve(1);
			auto& fg = foundGames.Last();
			fg.FileInfo = *grp;
			fg.FileName = entry->FileName;
			for (auto addon : addonList)
			{
				if (CheckAddon(addon, grp, entry->FileName))
				{
					foundGames.Reserve(1);
					auto& fga = foundGames.Last();
					fga.FileInfo = *addon;
				}
			}
		}
	}

	// We must check for all addons if their dependency is present.
	for (int i = foundGames.Size() - 1; i >= 0; i--)
	{
		auto crc = foundGames[i].FileInfo.dependencyCRC;
		if (crc != 0)
		{
			bool found = false;
			for (auto& fg : foundGames)
			{
				if (fg.FileInfo.CRC == crc)
				{
					found = true;
					break;
				}
			}
			if (!found) foundGames.Delete(i); // Dependent add-on without dependency cannot be played.
		}
	}

	// return everything to its proper order (using qsort because sorting an array of complex structs with std::sort is a messy affair.)
	qsort(foundGames.Data(), foundGames.Size(), sizeof(foundGames[0]), [](const void* a, const void* b)->int
		{
			auto A = (const GrpEntry*)a;
			auto B = (const GrpEntry*)b;
			return (int)A->FileInfo.index - (int)B->FileInfo.index;
		});

	// Finally, scan the list for duplicstes. Only the first occurence should count.

	for (unsigned i = 0; i < foundGames.Size(); i++)
	{
		for (unsigned j = foundGames.Size() - 1; j > i; j--)
		{
			if (foundGames[i].FileInfo.CRC == foundGames[j].FileInfo.CRC && (foundGames[j].FileInfo.CRC != 0 || foundGames[i].FileInfo.name == foundGames[j].FileInfo.name))
				foundGames.Delete(j);
		}
	}

	// new CRCs got added so save the list.
	if (cachedCRCs.Size() > numCRCs)
	{
		SaveCRCs(cachedCRCs);
	}
	
	return foundGames;
}


//==========================================================================
//
// Fallback in case nothing got defined.
// Also used by 'includedefault' which forces this to be statically defined
// (and which shouldn't be used to begin with because there are no default DEFs!)
//
//==========================================================================

const char* G_DefaultDefFile(void)
{
	if (isBlood()) 
		return "blood.def";

	if (g_gameType & GAMEFLAG_DUKE)
		return "duke3d.def";

	if (g_gameType & GAMEFLAG_RRRA)
		return "rrra.def";

	if (g_gameType & GAMEFLAG_RR)
		return "rr.def";

	if (isWW2GI())
		return "ww2gi.def";

	if (isSWALL())
		return "sw.def";

	if (g_gameType & GAMEFLAG_PSEXHUMED)
		return "exhumed.def";

	if (g_gameType & GAMEFLAG_NAM)
		return fileSystem.FindFile("nam.def") ? "nam.def" : "napalm.def";

	if (g_gameType & GAMEFLAG_NAPALM)
		return fileSystem.FindFile("napalm.def") ? "napalm.def" : "nam.def";

	return "duke3d.def";
}

const char* G_DefFile(void)
{
	return userConfig.DefaultDef.IsNotEmpty() ? userConfig.DefaultDef.GetChars() : G_DefaultDefFile();
}


#if 0
// Should this be added to the game data collector?
bool AddINIFile(const char* pzFile, bool bForce = false)
{
	char* pzFN;
	static INICHAIN* pINIIter = NULL;
	if (!bForce)
	{
		if (findfrompath(pzFile, &pzFN)) return false; // failed to resolve the filename
		if (!FileExists(pzFN))
		{
			M_Free(pzFN);
			return false;
		} // failed to stat the file
		M_Free(pzFN);
		IniFile* pTempIni = new IniFile(pzFile);
		if (!pTempIni->FindSection("Episode1"))
		{
			delete pTempIni;
			return false;
		}
		delete pTempIni;
	}
	if (!pINIChain)
		pINIIter = pINIChain = new INICHAIN;
	else
		pINIIter = pINIIter->pNext = new INICHAIN;
	pINIIter->pNext = NULL;
	pINIIter->pDescription = NULL;
	strncpy(pINIIter->zName, pzFile, BMAX_PATH);
	for (int i = 0; i < countof(gINIDescription); i++)
	{
		if (!strnicmp(pINIIter->zName, gINIDescription[i].pzFilename, BMAX_PATH))
		{
			pINIIter->pDescription = &gINIDescription[i];
			break;
		}
	}
	return true;
}

void ScanINIFiles(void)
{
	nINICount = 0;
	BUILDVFS_FIND_REC* pINIList = klistpath("/", "*.ini", BUILDVFS_FIND_FILE);
	pINIChain = NULL;

	if (bINIOverride || !pINIList)
	{
		AddINIFile(BloodIniFile, true);
	}

	for (auto pIter = pINIList; pIter; pIter = pIter->next)
	{
		AddINIFile(pIter->name);
	}
	klistfree(pINIList);
	pINISelected = pINIChain;
	for (auto pIter = pINIChain; pIter; pIter = pIter->pNext)
	{
		if (!strnicmp(BloodIniFile, pIter->zName, BMAX_PATH))
		{
			pINISelected = pIter;
			break;
		}
	}
#endif
