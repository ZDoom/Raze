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

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

//==========================================================================
//
// BaseFileSearch
//
// If a file does not exist at <file>, looks for it in the directories
// specified in the config file. Returns the path to the file, if found,
// or nullptr if it could not be found.
//
//==========================================================================

FString BaseFileSearch (const char *file, const char *ext, bool lookfirstinprogdir)
{
	FString wad;

	if (file == nullptr || *file == '\0')
	{
		return nullptr;
	}
	if (lookfirstinprogdir)
	{
		wad.Format("%s%s%s", progdir.GetChars(), progdir.Back() == '/' ? "" : "/", file);
		if (DirEntryExists (wad))
		{
			return wad;
		}
	}

	if (DirEntryExists (file))
	{
		wad.Format("%s", file);
		return wad;
	}

	if (GameConfig != nullptr && GameConfig->SetSection ("FileSearch.Directories"))
	{
		const char *key;
		const char *value;

		while (GameConfig->NextInSection (key, value))
		{
			if (stricmp (key, "Path") == 0)
			{
				FString dir;

				dir = NicePath(value);
				if (dir.IsNotEmpty())
				{
					wad.Format("%s%s%s", dir.GetChars(), dir.Back() == '/' ? "" : "/", file);
					if (DirEntryExists (wad))
					{
						return wad;
					}
				}
			}
		}
	}

	// Retry, this time with a default extension
	if (ext != nullptr)
	{
		FString tmp = file;
		DefaultExtension (tmp, ext);
		return BaseFileSearch (tmp, nullptr, lookfirstinprogdir);
	}
	return nullptr;
}
 
//==========================================================================
//
// D_AddFile
//
//==========================================================================

bool D_AddFile (TArray<FString> &wadfiles, const char *file, bool check = true, int position = -1)
{
	if (file == NULL || *file == '\0')
	{
		return false;
	}

	if (check && !DirEntryExists (file))
	{
		const char *f = BaseFileSearch (file, ".grp", false);
		if (f == NULL)
		{
			Printf ("Can't find '%s'\n", file);
			return false;
		}
		file = f;
	}

	FString f = file;
	f.Substitute("\\", "/");
	if (position == -1) wadfiles.Push(f);
	else wadfiles.Insert(position, f);
	return true;
}

 //==========================================================================
//
// D_AddWildFile
//
//==========================================================================

void D_AddWildFile (TArray<FString> &wadfiles, const char *value)
{
	if (value == NULL || *value == '\0')
	{
		return;
	}
	FString wadfile = BaseFileSearch (value, ".wad", false);

	if (wadfile.Len() != 0)
	{
		D_AddFile (wadfiles, wadfile);
	}
	else
	{ // Try pattern matching
		findstate_t findstate;
		char path[260];
		char *sep;
		void *handle = I_FindFirst (value, &findstate);

		strcpy (path, value);
		sep = strrchr (path, '/');
		if (sep == NULL)
		{
			sep = strrchr (path, '\\');
#ifdef _WIN32
			if (sep == NULL && path[1] == ':')
			{
				sep = path + 1;
			}
#endif
		}

		if (handle != ((void *)-1))
		{
			do
			{
				if (!(I_FindAttr(&findstate) & FA_DIREC))
				{
					if (sep == NULL)
					{
						D_AddFile (wadfiles, I_FindName (&findstate));
					}
					else
					{
						strcpy (sep+1, I_FindName (&findstate));
						D_AddFile (wadfiles, path, false );
					}
				}
			} while (I_FindNext (handle, &findstate) == 0);
		}
		I_FindClose (handle);
	}
} 

//==========================================================================
//
// D_AddConfigWads
//
// Adds all files in the specified config file section.
//
//==========================================================================

void D_AddConfigWads (TArray<FString> &wadfiles, const char *section)
{
	if (GameConfig->SetSection (section))
	{
		const char *key;
		const char *value;
		FConfigFile::Position pos;

		while (GameConfig->NextInSection (key, value))
		{
			if (stricmp (key, "Path") == 0)
			{
				// D_AddWildFile resets GameConfig's position, so remember it
				GameConfig->GetPosition (pos);
				D_AddWildFile (wadfiles, ExpandEnvVars(value));
				// Reset GameConfig's position to get next wad
				GameConfig->SetPosition (pos);
			}
		}
	}
}
 
 //==========================================================================
//
// D_AddDirectory
//
// Add all .wad files in a directory. Does not descend into subdirectories.
//
//==========================================================================

static void D_AddDirectory (TArray<FString> &wadfiles, const char *dir)
{
	char curdir[PATH_MAX];

	if (getcwd (curdir, PATH_MAX))
	{
		char skindir[PATH_MAX];
		findstate_t findstate;
		void *handle;
		size_t stuffstart;

		stuffstart = strlen (dir);
		memcpy (skindir, dir, stuffstart*sizeof(*dir));
		skindir[stuffstart] = 0;

		if (skindir[stuffstart-1] == '/')
		{
			skindir[--stuffstart] = 0;
		}

		if (!chdir (skindir))
		{
			skindir[stuffstart++] = '/';
			int savedstart = stuffstart;
			static const char* validexts[] = { "*.grp", "*.zip", "*.pk3", "*.pk4", "*.7z", "*.pk7", "*.dat", "*.rff" };
			for (auto ext : validexts)
			{
				stuffstart = savedstart;
				if ((handle = I_FindFirst(ext, &findstate)) != (void*)-1)
				{
					do
					{
						if (!(I_FindAttr(&findstate) & FA_DIREC))
						{
							strcpy(skindir + stuffstart, I_FindName(&findstate));
							D_AddFile(wadfiles, skindir);
						}
					} while (I_FindNext(handle, &findstate) == 0);
					I_FindClose(handle);
				}
			}
		}
		chdir (curdir);
	}
}

//==========================================================================
//
//
//
//==========================================================================

static FString ParseGameInfo(TArray<FString>& pwads, const char* fn, const char* data, int size)
{
	FScanner sc;
	FString iwad;
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
			iwad = sc.String;
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
					pos += D_AddFile(pwads, sc.String, true, pos);
				}
				else
				{
					pos += D_AddFile(pwads, checkpath, true, pos);
				}
			} while (sc.CheckToken(','));
		}
		else if (!nextKey.CompareNoCase("STARTUPTITLE"))
		{
			sc.MustGetString();
			RazeStartupInfo.Name = sc.String;
		}
		else if (!nextKey.CompareNoCase("STARTUPCOLORS"))
		{
			sc.MustGetString();
			RazeStartupInfo.FgColor = V_GetColor(NULL, sc);
			sc.MustGetStringName(",");
			sc.MustGetString();
			RazeStartupInfo.BkColor = V_GetColor(NULL, sc);
		}
		else if (!nextKey.CompareNoCase("MODERN"))
		{
			sc.MustGetNumber();
			RazeStartupInfo.modern = sc.Number ? 1 : -1;
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
	return iwad;
}
//==========================================================================
//
//
//
//==========================================================================

static FString CheckGameInfo(TArray<FString>& pwads)
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
					FString iwad = ParseGameInfo(pwads, resfile->FileName, (const char*)lmp->Lock(), lmp->LumpSize);
					delete resfile;
					return iwad;
				}
			}
			delete resfile;
		}
	}
	return "";
}

//==========================================================================
//
//
//
//==========================================================================

FString GetGameFronUserFiles()
{
	TArray<FString> Files;

	if (userConfig.AddFilesPre) for (auto& file : *userConfig.AddFilesPre)
	{
		D_AddFile(Files, file);
	}
	if (userConfig.AddFiles)
	{
		for (auto& file : *userConfig.AddFiles)
		{
			D_AddFile(Files, file);
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
				D_AddDirectory(Files, fn);
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

		for (uint32_t i = 0; i < fileSystem.GetNumEntries(); i++)
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

void InitFileSystem(TArray<GrpEntry>& groups)
{
	TArray<int> dependencies;
	TArray<FString> Files;

	// First comes the engine's own stuff.
	FString baseres = progdir + ENGINERES_FILE;
	D_AddFile(Files, baseres);

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
			D_AddFile(Files, grp.FileName);
			fn = ExtractFilePath(grp.FileName);
			if (fn.Len() > 0 && fn.Back() != '/') fn += '/';
		}

		for (auto& fname : grp.FileInfo.loadfiles)
		{
			FString newname = fn + fname;
			D_AddFile(Files, newname);
		}
		bool insert = (!insertdirectoriesafter && &grp == &groups[0]) || (insertdirectoriesafter && &grp == &groups.Last());

		// Add the game's main directory in the proper spot.
		if (insert)
		{
			// Build's original 'file system' loads all GRPs before the first external directory.
			// Do this only if explicitly requested because this severely limits the usability of GRP files.
			if (insertdirectoriesafter && userConfig.AddFilesPre) for (auto& file : *userConfig.AddFilesPre)
			{
				D_AddFile(Files, '*' + file);	// The * tells the file system not to pull in all subdirectories.
			}

			D_AddFile(Files, fn);
		}
		i--;
	}

	const char* key;
	const char* value;
	if (GameConfig->SetSection("global.Autoload"))
	{
		while (GameConfig->NextInSection(key, value))
		{
			if (stricmp(key, "Path") == 0)
			{
				FString nice = NicePath(value);
				D_AddFile(Files, nice);
			}
		}
	}
	
	if (!insertdirectoriesafter && userConfig.AddFilesPre) for (auto& file : *userConfig.AddFilesPre)
	{
		D_AddFile(Files, file);
	}
	if (userConfig.AddFiles)
	{
		for (auto& file : *userConfig.AddFiles)
		{
			D_AddFile(Files, file);
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
				D_AddDirectory(Files, fn);
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

	lfi.dotFilter = LumpFilter;
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
