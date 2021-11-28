/*
** initfs.cpp
**
**---------------------------------------------------------------------------
** Copyright 1999-2016 Randy Heit 
** Copyright 2002-2019 Christoph Oelckers
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

#include "filesystem.h"
#include "cmdlib.h"
#include "zstring.h"
#include "gamecontrol.h"
#include "gameconfigfile.h"
#include "printf.h"
#include "m_argv.h"
#include "version.h"
#include "sc_man.h"
#include "v_video.h"
#include "v_text.h"
#include "findfile.h"
#include "palutil.h"
#include "startupinfo.h"

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

static const char* validexts[] = { "*.grp", "*.zip", "*.pk3", "*.pk4", "*.7z", "*.pk7", "*.dat", "*.rff", "*.ssi" };

//==========================================================================
//
//
//
//==========================================================================

static TArray<FString> ParseGameInfo(TArray<FString>& pwads, const char* fn, const char* data, int size)
{
	FScanner sc;
	TArray<FString> bases;
	int pos = 0;

	const char* lastSlash = strrchr(fn, '/');

	sc.OpenMem("GAMEINFO", data, size);
	sc.SetCMode(true);
	while (sc.GetToken())
	{
		sc.TokenMustBe(TK_Identifier);
		FString nextKey = sc.String;
		sc.MustGetToken('=');
		if (!nextKey.CompareNoCase("GAME"))
		{
			sc.MustGetString();
			bases.Push(sc.String);
		}
		else if (!nextKey.CompareNoCase("LOAD"))
		{
			do
			{
				sc.MustGetString();

				// Try looking for the wad in the same directory as the .wad
				// before looking for it in the current directory.

				FString checkpath;
				if (lastSlash != NULL)
				{
					checkpath = FString(fn, (lastSlash - fn) + 1);
					checkpath += sc.String;
				}
				else
				{
					checkpath = sc.String;
				}
				if (!FileExists(checkpath))
				{
					pos += D_AddFile(pwads, sc.String, true, pos, GameConfig);
				}
				else
				{
					pos += D_AddFile(pwads, checkpath, true, pos, GameConfig);
				}
			} while (sc.CheckToken(','));
		}
		else if (!nextKey.CompareNoCase("STARTUPTITLE"))
		{
			sc.MustGetString();
			GameStartupInfo.Name = sc.String;
		}
		else if (!nextKey.CompareNoCase("STARTUPCOLORS"))
		{
			sc.MustGetString();
			GameStartupInfo.FgColor = V_GetColor(sc);
			sc.MustGetStringName(",");
			sc.MustGetString();
			GameStartupInfo.BkColor = V_GetColor(sc);
		}
		else if (!nextKey.CompareNoCase("CON"))
		{
			sc.MustGetString();
			GameStartupInfo.con = sc.String;;
		}
		else if (!nextKey.CompareNoCase("DEF"))
		{
			sc.MustGetString();
			GameStartupInfo.def = sc.String;;
		}
		else
		{
			// Silently ignore unknown properties
			do
			{
				sc.MustGetAnyToken();
			} while (sc.CheckToken(','));
		}
	}
	return bases;
}
//==========================================================================
//
//
//
//==========================================================================

static TArray<FString> CheckGameInfo(TArray<FString>& pwads)
{
	// scan the list of WADs backwards to find the last one that contains a GAMEINFO lump
	for (int i = pwads.Size() - 1; i >= 0; i--)
	{
		bool isdir = false;
		FResourceFile* resfile;
		const char* filename = pwads[i];

		// Does this exist? If so, is it a directory?
		if (!DirEntryExists(pwads[i], &isdir))
		{
			Printf(TEXTCOLOR_RED "Could not find %s\n", filename);
			continue;
		}

		if (!isdir)
		{
			FileReader fr;
			if (!fr.OpenFile(filename))
			{
				// Didn't find file
				continue;
			}
			resfile = FResourceFile::OpenResourceFile(filename, fr, true);
		}
		else
			resfile = FResourceFile::OpenDirectory(filename, true);

		FName gameinfo = "GAMEINFO.TXT";
		if (resfile != NULL)
		{
			uint32_t cnt = resfile->LumpCount();
			for (int i = cnt - 1; i >= 0; i--)
			{
				FResourceLump* lmp = resfile->GetLump(i);

				if (FName(lmp->getName(), true) == gameinfo)
				{
					// Found one!
					auto bases = ParseGameInfo(pwads, resfile->FileName, (const char*)lmp->Lock(), lmp->LumpSize);
					delete resfile;
					return bases;
				}
			}
			delete resfile;
		}
	}
	return TArray<FString>();
}

//==========================================================================
//
//
//
//==========================================================================

TArray<FString> GetGameFronUserFiles()
{
	TArray<FString> Files;

	if (userConfig.AddFilesPre) for (auto& file : *userConfig.AddFilesPre)
	{
		D_AddFile(Files, file, true, -1, GameConfig);
	}
	if (userConfig.AddFiles)
	{
		for (auto& file : *userConfig.AddFiles)
		{
			D_AddFile(Files, file, true, -1, GameConfig);
		}

		// Finally, if the last entry in the chain is a directory, it's being considered the mod directory, and all GRPs inside need to be loaded, too.
		if (userConfig.AddFiles->NumArgs() > 0)
		{
			auto fn = (*userConfig.AddFiles)[userConfig.AddFiles->NumArgs() - 1];
			bool isdir = false;
			if (DirEntryExists(fn, &isdir) && isdir)
			{
				// Insert the GRPs before this entry itself.
				FString lastfn;
				Files.Pop(lastfn);
				for (auto ext : validexts)
				{
					D_AddDirectory(Files, fn, ext, GameConfig);
				}
				Files.Push(lastfn);
			}
		}
	}
	return CheckGameInfo(Files);
}

//==========================================================================
//
// Deletes unwanted content from the main game files
//
//==========================================================================

static void DeleteStuff(FileSystem &fileSystem, const TArray<FString>& deletelumps, int numgamefiles)
{
	// This must account for the game directory being inserted at index 2.
	// Deletion may only occur in the main game file, the directory and the add-on, there are no secondary dependencies, i.e. more than two game files.
	numgamefiles++;
	for (auto str : deletelumps)
	{
		FString renameTo;
		auto ndx = str.IndexOf("*");
		if (ndx >= 0)
		{
			renameTo = FName(str.Mid(ndx + 1)).GetChars();
			str.Truncate(ndx);
		}

		for (int i = 0; i < fileSystem.GetNumEntries(); i++)
		{
			int cf = fileSystem.GetFileContainer(i);
			auto fname = fileSystem.GetFileFullName(i, false);
			if (cf >= 1 && cf <= numgamefiles && !str.CompareNoCase(fname))
			{
				fileSystem.RenameFile(i, renameTo);
			}
		}
	}
}
//==========================================================================
//
//
//
//==========================================================================
const char* iwad_folders[13] = { "textures/", "hires/", "sounds/", "music/", "maps/" };
const char* iwad_reserved_duke[12] = { ".map", ".con", "menudef", "gldefs", "zscript", "maps/", nullptr };
const char* iwad_reserved_blood[12] = { ".map", ".ini", "menudef", "gldefs", "zscript", "maps/", nullptr };
const char* iwad_reserved_sw[12] = { ".map", "swcustom.txt", "menudef", "gldefs", "zscript", "maps/", nullptr };
const char* iwad_reserved_ex[12] = { ".map", "menudef", "gldefs", "zscript", "maps/", nullptr };

const char** iwad_reserved()
{
	return (g_gameType & GAMEFLAG_PSEXHUMED) ? iwad_reserved_ex :
		isSWALL() ? iwad_reserved_sw :
		(g_gameType & GAMEFLAG_BLOOD) ? iwad_reserved_blood : iwad_reserved_duke;
}

void InitFileSystem(TArray<GrpEntry>& groups)
{
	TArray<int> dependencies;
	TArray<FString> Files;

	// First comes the engine's own stuff.
	const char* baseres = BaseFileSearch(ENGINERES_FILE, nullptr, true, GameConfig);
	D_AddFile(Files, baseres, true, -1, GameConfig);

	bool insertdirectoriesafter = Args->CheckParm("-insertdirafter");

	int i = groups.Size()-1;
	FString fn;
	for (auto &grp : groups)
	{
		// Add all dependencies, plus the directory of the base dependency.
		// Directories of addon content are not added if they differ from the main directory. 
		// Also, the directory is inserted after the base dependency, allowing the addons to override directory content.
		// This can be overridden via command line switch if needed.
		if (!grp.FileInfo.loaddirectory && grp.FileName.IsNotEmpty())
		{
			D_AddFile(Files, grp.FileName, true, -1, GameConfig);
			fn = ExtractFilePath(grp.FileName);
			if (fn.Len() > 0 && fn.Back() != '/') fn += '/';
		}

		for (auto& fname : grp.FileInfo.loadfiles)
		{
			FString newname = fn + fname;
			D_AddFile(Files, newname, true, -1, GameConfig);
		}
		bool insert = (!insertdirectoriesafter && &grp == &groups[0]) || (insertdirectoriesafter && &grp == &groups.Last());

		// Add the game's main directory in the proper spot.
		if (insert)
		{
			// Build's original 'file system' loads all GRPs before the first external directory.
			// Do this only if explicitly requested because this severely limits the usability of GRP files.
			if (insertdirectoriesafter && userConfig.AddFilesPre) for (auto& file : *userConfig.AddFilesPre)
			{
				D_AddFile(Files, file, true, -1, GameConfig);
			}

			D_AddFile(Files, fn, true, -1, GameConfig);
		}
		i--;
	}
	fileSystem.SetIwadNum(1);
	fileSystem.SetMaxIwadNum(Files.Size() - 1);

	D_AddConfigFiles(Files, "Global.Autoload", "*.grp", GameConfig);

	size_t len;
	size_t lastpos = 0;

	while (lastpos < LumpFilter.Len() && (len = strcspn(LumpFilter.GetChars() + lastpos, ".")) > 0)
	{
		auto file = LumpFilter.Left(len + lastpos) + ".Autoload";
		D_AddConfigFiles(Files, file, "*.grp", GameConfig);
		lastpos += len + 1;
	}
	
	if (!insertdirectoriesafter && userConfig.AddFilesPre) for (auto& file : *userConfig.AddFilesPre)
	{
		D_AddFile(Files, file, true, -1, GameConfig);
	}
	if (userConfig.AddFiles)
	{
		for (auto& file : *userConfig.AddFiles)
		{
			D_AddFile(Files, file, true, -1, GameConfig);
		}

		// Finally, if the last entry in the chain is a directory, it's being considered the mod directory, and all GRPs inside need to be loaded, too.
		if (userConfig.AddFiles->NumArgs() > 0)
		{
			auto fn = (*userConfig.AddFiles)[userConfig.AddFiles->NumArgs() - 1];
			bool isdir = false;
			if (DirEntryExists(fn, &isdir) && isdir)
			{
				// Insert the GRPs before this entry itself.
				FString lastfn;
				Files.Pop(lastfn);
				for (auto ext : validexts)
				{
					D_AddDirectory(Files, fn, ext, GameConfig);
				}
				Files.Push(lastfn);
			}
		}
	}


	TArray<FString> todelete;
	for (auto& g : groups)
	{
		todelete.Append(g.FileInfo.tobedeleted);
	}
	todelete.Append(userConfig.toBeDeleted);
	LumpFilterInfo lfi;
	for (auto p : iwad_folders) lfi.reservedFolders.Push(p);
	for (auto p = iwad_reserved(); *p; p++) lfi.requiredPrefixes.Push(*p);
	if (isBlood())
	{
		lfi.embeddings.Push("blood.rff");
		lfi.embeddings.Push("sounds.rff");
	}

	lfi.dotFilter = LumpFilter;

	if (g_gameType & (GAMEFLAG_DUKE | GAMEFLAG_NAM | GAMEFLAG_NAPALM | GAMEFLAG_WW2GI | GAMEFLAG_RRALL)) lfi.gameTypeFilter.Push("DukeEngine");

	lfi.postprocessFunc = [&]()
	{
		DeleteStuff(fileSystem, todelete, groups.Size());
	};
	fileSystem.InitMultipleFiles(Files, false, &lfi);
	if (Args->CheckParm("-dumpfs"))
	{
		FILE* f = fopen("filesystem.dir", "wb");
		for (int i = 0; i < fileSystem.GetNumEntries(); i++)
		{
			auto fd = fileSystem.GetFileAt(i);
			fprintf(f, "%.50s   %60s  %d\n", fd->getName(), fileSystem.GetResourceFileFullName(fileSystem.GetFileContainer(i)), fd->Size());
		}
		fclose(f);
	}
}
