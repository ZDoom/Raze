/*
** gameconfigfile.cpp
** An .ini parser specifically for zdoom.ini
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
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
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OFf
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <stdio.h>

#include "gameconfigfile.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "c_bind.h"
#include "m_argv.h"
#include "cmdlib.h"
#include "i_specialpaths.h"
#include "printf.h"
#include "gamecontrol.h"
#include "version.h"

#define LASTRUNVERSION "2"

#if !defined _MSC_VER && !defined __APPLE__
#include "i_system.h"  // for SHARE_DIR
#endif // !_MSC_VER && !__APPLE__

FGameConfigFile::FGameConfigFile ()
{
#ifdef __APPLE__
	FString user_docs, user_app_support, local_app_support;
	M_GetMacSearchDirectories(user_docs, user_app_support, local_app_support);
#endif

	FString pathname;

	OkayToWrite = false;	// Do not allow saving of the config before DoKeySetup()
	bModSetup = false;
	pathname = GetConfigPath (true);
	ChangePathName (pathname);
	LoadConfigFile ();

	// If zdoom.ini was read from the program directory, switch
	// to the user directory now. If it was read from the user
	// directory, this effectively does nothing.
	pathname = GetConfigPath (false);
	ChangePathName (pathname);

	// Set default IWAD search paths if none present
	if (!SetSection ("GameSearch.Directories"))
	{
		SetSection ("GameSearch.Directories", true);
		SetValueForKey ("Path", ".", true);
		SetValueForKey ("Path", "./*", true);
#ifdef __APPLE__
		SetValueForKey ("Path", user_docs + "/*", true);
		SetValueForKey ("Path", user_app_support + "/EDuke32", true);
		SetValueForKey ("Path", user_app_support + "/JFDuke32", true);
		SetValueForKey ("Path", user_app_support + "/NBlood", true);
		SetValueForKey ("Path", "$PROGDIR", true);
		SetValueForKey ("Path", "$PROGDIR/*", true);
		SetValueForKey ("Path", local_app_support + "/EDuke32", true);
		SetValueForKey ("Path", local_app_support + "/JFDuke32", true);
		SetValueForKey ("Path", local_app_support + "/NBlood", true);
		SetValueForKey("Path", local_app_support + "/JFSW", true);
		SetValueForKey("Path", local_app_support + "/VoidSW", true);

#elif !defined(__unix__)
		SetValueForKey ("Path", "$PROGDIR", true);
		SetValueForKey ("Path", "$PROGDIR/*", true);
#else
		SetValueForKey ("Path", "$HOME/" GAME_DIR, true);
		SetValueForKey ("Path", "$HOME/" GAME_DIR "/*", true);
		// Arch Linux likes them in /usr/share/raze
		// Debian likes them in /usr/share/games/raze
		// I assume other distributions don't do anything radically different
		SetValueForKey ("Path", "/opt/raze", true);
		SetValueForKey ("Path", "/usr/share/games/raze", true);
		SetValueForKey ("Path", "/usr/local/share/games/raze", true);
		SetValueForKey ("Path", "/usr/share/games/jfduke3d", true);
		SetValueForKey ("Path", "/usr/local/share/games/jfduke3d", true);
		SetValueForKey ("Path", "/usr/share/games/eduke32", true);
		SetValueForKey ("Path", "/usr/local/share/games/eduke32", true);
		SetValueForKey ("Path", "/usr/share/games/nblood", true);
		SetValueForKey ("Path", "/usr/local/share/games/nblood", true);
		SetValueForKey("Path", "/usr/share/games/jfsw", true);
		SetValueForKey("Path", "/usr/local/share/games/jfsw", true);
		SetValueForKey("Path", "/usr/share/games/voidsw", true);
		SetValueForKey("Path", "/usr/local/share/games/voidsw", true);

#endif
		SetValueForKey ("Path", "$STEAM", true); // also covers GOG.
	}

	// Set default search paths if none present
	if (!SetSection ("FileSearch.Directories"))
	{
		SetSection ("FileSearch.Directories", true);
#ifdef __APPLE__
		SetValueForKey ("Path", user_docs, true);
		SetValueForKey ("Path", user_app_support, true);
		SetValueForKey ("Path", "$PROGDIR", true);
		SetValueForKey ("Path", local_app_support, true);
#elif !defined(__unix__)
		SetValueForKey ("Path", "$PROGDIR", true);
		SetValueForKey ("Path", "$GAMEDIR", true);
#else
		SetValueForKey ("Path", "$HOME/" GAME_DIR, true);
		SetValueForKey ("Path", SHARE_DIR, true);
		SetValueForKey ("Path", "/usr/share/games/jfduke3d", true);
		SetValueForKey ("Path", "/usr/local/share/games/jfduke3d", true);
		SetValueForKey ("Path", "/usr/share/games/eduke32", true);
		SetValueForKey ("Path", "/usr/local/share/games/eduke32", true);
		SetValueForKey ("Path", "/usr/share/games/nblood", true);
		SetValueForKey ("Path", "/usr/local/share/games/nblood", true);
#endif
	}

	// Set default search paths if none present
	if (!SetSection("SoundfontSearch.Directories"))
	{
		SetSection("SoundfontSearch.Directories", true);
#ifdef __APPLE__
		SetValueForKey("Path", user_docs + "/soundfonts", true);
		SetValueForKey("Path", user_app_support + "/soundfonts", true);
		SetValueForKey("Path", "$PROGDIR/soundfonts", true);
		SetValueForKey("Path", local_app_support + "/soundfonts", true);
#elif !defined(__unix__)
		SetValueForKey("Path", "$PROGDIR/soundfonts", true);
#else
		SetValueForKey("Path", "$HOME/" GAME_DIR "/soundfonts", true);
		SetValueForKey("Path", "/usr/local/share/" GAME_DIR "/soundfonts", true);
		SetValueForKey("Path", "/usr/local/share/games/" GAME_DIR "/soundfonts", true);
		SetValueForKey("Path", "/usr/share/" GAME_DIR "/soundfonts", true);
		SetValueForKey("Path", "/usr/share/games/" GAME_DIR "/soundfonts", true);
#endif
	}

	// Add some self-documentation.
	SetSectionNote("GameSearch.Directories",
		"# These are the directories to automatically search for game data.\n"
		"# Each directory should be on a separate line, preceded by Path=\n");
	SetSectionNote("FileSearch.Directories",
		"# These are the directories to search for add-ons added with the -file\n"
		"# command line parameter, if they cannot be found with the path\n"
		"# as-is. Layout is the same as for GameSearch.Directories\n");
	SetSectionNote("SoundfontSearch.Directories",
		"# These are the directories to search for soundfonts that let listed in the menu.\n"
		"# Layout is the same as for GameSearch.Directories\n");
}

FGameConfigFile::~FGameConfigFile ()
{
}

void FGameConfigFile::WriteCommentHeader (FileWriter *file) const
{
	file->Printf ("# This file was generated by " GAMENAME " %s\n", GetVersionString());
}

void FGameConfigFile::DoAutoloadSetup (/*FIWadManager *iwad_man*/)
{
	// Create auto-load sections, so users know what's available.
	// Note that this totem pole is the reverse of the order that
	// they will appear in the file.

	double last = 0;
	if (SetSection ("LastRun"))
	{
		const char *lastver = GetValueForKey ("Version");
		if (lastver != NULL) last = atof(lastver);
		isInitialized = true;
	}

	CreateSectionAtStart("Exhumed.Autoload");
	CreateSectionAtStart("WW2GI.Autoload");
	CreateSectionAtStart("Nam.Autoload");
	CreateSectionAtStart("Exhumed.Autoload");
	CreateSectionAtStart("Redneck.Route66.Autoload");
	CreateSectionAtStart("Redneck.RidesAgain.Autoload");
	CreateSectionAtStart("Redneck.Redneck.Autoload");
	CreateSectionAtStart("Redneck.Autoload");
	CreateSectionAtStart("ShadowWarrior.TwinDragon.Autoload");
	CreateSectionAtStart("ShadowWarrior.Wanton.Autoload");
	CreateSectionAtStart("ShadowWarrior.ShadowWarrior.Autoload");
	CreateSectionAtStart("ShadowWarrior.Autoload");
	CreateSectionAtStart("Blood.Cryptic.Autoload");
	CreateSectionAtStart("Blood.Blood.Autoload");
	CreateSectionAtStart("Blood.Autoload");
	CreateSectionAtStart("Duke.WorldTour.Autoload");
	CreateSectionAtStart("Duke.NWinter.Autoload");
	CreateSectionAtStart("Duke.Vacation.15.Autoload");
	CreateSectionAtStart("Duke.Vacation.13.Autoload");
	CreateSectionAtStart("Duke.Vacation.Autoload");
	CreateSectionAtStart("Duke.DukeDC.15.Autoload");
	CreateSectionAtStart("Duke.DukeDC.13.Autoload");
	CreateSectionAtStart("Duke.DukeDC.Autoload");
	CreateSectionAtStart("Duke.Duke.15.Autoload");
	CreateSectionAtStart("Duke.Duke.13.Autoload");
	CreateSectionAtStart("Duke.Duke.Autoload");
	CreateSectionAtStart("Duke.PParadise.Autoload");
	CreateSectionAtStart("Duke.Autoload");

	CreateSectionAtStart("Global.Autoload");

	// The same goes for auto-exec files.
	CreateStandardAutoExec("ShadowWarrior.AutoLoad", true);
	CreateStandardAutoExec("Redneck.RidesAgain.AutoLoad", true);
	CreateStandardAutoExec("Redneck.Redneck.AutoLoad", true);
	CreateStandardAutoExec("WW2GI.AutoLoad", true);
	CreateStandardAutoExec("Nam.AutoLoad", true);
	CreateStandardAutoExec("DukeNukem3D.AutoLoad", true);
	CreateStandardAutoExec("DukeNukem3D.DN3D.AutoLoad", true);
	CreateStandardAutoExec("DukeNukem3D.DukeDC.AutoLoad", true);
	CreateStandardAutoExec("DukeNukem3D.NWinter.AutoLoad", true);
	CreateStandardAutoExec("DukeNukem3D.Vacation.AutoLoad", true);
	CreateStandardAutoExec("DukeNukem3D.PParadise.AutoLoad", true);

	// Move search paths back to the top.
	MoveSectionToStart("SoundfontSearch.Directories");
	MoveSectionToStart("FileSearch.Directories");
	MoveSectionToStart("IWADSearch.Directories");

	SetSectionNote("DukeNukem3D.AutoLoad",
		"# Files to automatically execute when running the corresponding game.\n"
		"# Each file should be on its own line, preceded by Path=\n\n");
	SetSectionNote("Global.Autoload",
		"# Files to always load. These are loaded after the game data but before\n"
		"# any files added with -file. Place each file on its own line, preceded\n"
		"# by Path=\n");
	SetSectionNote("DukeNukem3D.Autoload",
		"# Files to automatically load depending on the game you are playing\n\n");
}

void FGameConfigFile::DoGlobalSetup ()
{
	if (SetSection ("GlobalSettings.Unknown"))
	{
		ReadCVars (CVAR_GLOBALCONFIG);
	}
	if (SetSection ("GlobalSettings"))
	{
		ReadCVars (CVAR_GLOBALCONFIG);
	}
	if (SetSection ("LastRun"))
	{
		const char *lastver = GetValueForKey ("Version");
		if (lastver != NULL)
		{
			double last = atof (lastver);
			if (last < 2)
			{
				auto var = FindCVar("mod_dumb_mastervolume", NULL);
				if (var != NULL)
				{
					UCVarValue v = var->GetGenericRep(CVAR_Float);
					v.Float /= 4.f;
					if (v.Float < 1.f) v.Float = 1.f;
				}
			}
		}
	}
}


void FGameConfigFile::DoGameSetup (const char *gamename)
{
	const char *key;
	const char *value;

	sublen = countof(section) - 1 - mysnprintf (section, countof(section), "%s.", gamename);
	subsection = section + countof(section) - sublen - 1;
	section[countof(section) - 1] = '\0';
	
	strncpy (subsection, "UnknownConsoleVariables", sublen);
	if (SetSection (section))
	{
		ReadCVars (0);
	}

	strncpy (subsection, "ConsoleVariables", sublen);
	if (SetSection (section))
	{
		ReadCVars (0);
	}

	// The NetServerInfo section will be read and override anything loaded
	// here when it's determined that a netgame is being played.
	strncpy (subsection, "LocalServerInfo", sublen);
	if (SetSection (section))
	{
		ReadCVars (0);
	}

	strncpy (subsection, "Player", sublen);
	if (SetSection (section))
	{
		ReadCVars (0);
	}
	strncpy (subsection, "ConsoleAliases", sublen);
	if (SetSection (section))
	{
		const char *name = NULL;
		while (NextInSection (key, value))
		{
			FStringf cmd("alias %s \"%s\"", key, value);
			C_DoCommand(cmd);
		}
	}
}

// Moved from DoGameSetup so that it can happen after wads are loaded
void FGameConfigFile::DoKeySetup(const char *gamename)
{
	static const struct { const char *label; FKeyBindings *bindings; } binders[] =
	{
		{ "Bindings", &Bindings },
		{ "DoubleBindings", &DoubleBindings },
		{ "AutomapBindings", &AutomapBindings },
		{ NULL, NULL }
	};
	const char *key, *value;

	sublen = countof(section) - 1 - mysnprintf(section, countof(section), "%s.", gamename);
	subsection = section + countof(section) - sublen - 1;
	section[countof(section) - 1] = '\0';

	C_SetDefaultBindings ();

	for (int i = 0; binders[i].label != NULL; ++i)
	{
		strncpy(subsection, binders[i].label, sublen);
		if (SetSection(section))
		{
			FKeyBindings *bindings = binders[i].bindings;
			bindings->UnbindAll();
			while (NextInSection(key, value))
			{
				bindings->DoBind(key, value);
			}
		}
	}
	OkayToWrite = true;
}

void FGameConfigFile::ReadNetVars ()
{
	strncpy (subsection, "NetServerInfo", sublen);
	if (SetSection (section))
	{
		ReadCVars (0);
	}
}

// Read cvars from a cvar section of the ini. Flags are the flags to give
// to newly-created cvars that were not already defined.
void FGameConfigFile::ReadCVars (uint32_t flags)
{
	const char *key, *value;
	FBaseCVar *cvar;
	UCVarValue val;

	flags |= CVAR_ARCHIVE|CVAR_UNSETTABLE|CVAR_AUTO;
	while (NextInSection (key, value))
	{
		cvar = FindCVar (key, NULL);
		if (cvar == NULL)
		{
			cvar = new FStringCVar (key, NULL, flags);
		}
		val.String = const_cast<char *>(value);
		cvar->SetGenericRep (val, CVAR_String);
	}
}

void FGameConfigFile::ArchiveGameData (const char *gamename)
{
	char section[32*3], *subsection;

	sublen = countof(section) - 1 - mysnprintf (section, countof(section), "%s.", gamename);
	subsection = section + countof(section) - 1 - sublen;

	strncpy (subsection, "Player", sublen);
	SetSection (section, true);
	ClearCurrentSection ();
	C_ArchiveCVars (this, CVAR_ARCHIVE|CVAR_USERINFO);

	strncpy (subsection, "ConsoleVariables", sublen);
	SetSection (section, true);
	ClearCurrentSection ();
	C_ArchiveCVars (this, CVAR_ARCHIVE);

	strncpy(subsection, "VideoSettings", sublen);
	SetSection(section, true);
	ClearCurrentSection();
	C_ArchiveCVars(this, CVAR_ARCHIVE|CVAR_VIDEOCONFIG);

#if 0
	// Do not overwrite the serverinfo section if playing a netgame, and
	// this machine was not the initial host.
	if (!netgame || consoleplayer == 0)
	{
		strncpy (subsection, netgame ? "NetServerInfo" : "LocalServerInfo", sublen);
		SetSection (section, true);
		ClearCurrentSection ();
		C_ArchiveCVars (this, CVAR_ARCHIVE|CVAR_SERVERINFO);
	}
#endif

	strncpy (subsection, "UnknownConsoleVariables", sublen);
	SetSection (section, true);
	ClearCurrentSection ();
	C_ArchiveCVars (this, CVAR_ARCHIVE|CVAR_AUTO);

	strncpy (subsection, "ConsoleAliases", sublen);
	SetSection (section, true);
	ClearCurrentSection ();
	C_ArchiveAliases (this);

	//M_SaveCustomKeys (this, section, subsection, sublen);

	strcpy (subsection, "Bindings");
	SetSection (section, true);
	Bindings.ArchiveBindings (this);

	strncpy (subsection, "DoubleBindings", sublen);
	SetSection (section, true);
	DoubleBindings.ArchiveBindings (this);

	strncpy (subsection, "AutomapBindings", sublen);
	SetSection (section, true);
	AutomapBindings.ArchiveBindings (this);
}

void FGameConfigFile::ArchiveGlobalData ()
{
	SetSection ("LastRun", true);
	ClearCurrentSection ();
	SetValueForKey ("Version", LASTRUNVERSION);

	SetSection ("GlobalSettings", true);
	ClearCurrentSection ();
	C_ArchiveCVars (this, CVAR_ARCHIVE|CVAR_GLOBALCONFIG);

	SetSection ("GlobalSettings.Unknown", true);
	ClearCurrentSection ();
	C_ArchiveCVars (this, CVAR_ARCHIVE|CVAR_GLOBALCONFIG|CVAR_AUTO);
}

FString FGameConfigFile::GetConfigPath (bool tryProg)
{
	const char *pathval;

	pathval = Args->CheckValue ("-config");
	if (pathval != NULL)
	{
		return FString(pathval);
	}
	return M_GetConfigPath(tryProg);
}

void FGameConfigFile::CreateStandardAutoExec(const char *section, bool start)
{
	if (!SetSection(section))
	{
		FString path = M_GetAutoexecPath();
		SetSection (section, true);
		SetValueForKey ("Path", path.GetChars());
	}
	if (start)
	{
		MoveSectionToStart(section);
	}
}

void FGameConfigFile::AddAutoexec (FArgs *list, const char *game)
{
	char section[64];
	const char *key;
	const char *value;

	snprintf (section, countof(section), "%s.AutoExec", game);

	// If <game>.AutoLoad section does not exist, create it
	// with a default autoexec.cfg file present.
	CreateStandardAutoExec(section, false);
	// Run any files listed in the <game>.AutoLoad section
	if (!SectionIsEmpty())
	{
		while (NextInSection (key, value))
		{
			if (stricmp (key, "Path") == 0 && *value != '\0')
			{
				FString expanded_path = ExpandEnvVars(value);
				if (FileExists(expanded_path))
				{
					list->AppendArg (ExpandEnvVars(value));
				}
			}
		}
	}
}

CCMD (whereisini)
{
	FString path = M_GetConfigPath(false);
	Printf ("%s\n", path.GetChars());
}

FGameConfigFile* GameConfig;
static FString GameName;

//==========================================================================
//
// D_MultiExec
//
//==========================================================================

FExecList *D_MultiExec (FArgs *list, FExecList *exec)
{
	for (int i = 0; i < list->NumArgs(); ++i)
	{
		exec = C_ParseExecFile(list->GetArg(i), exec);
	}
	return exec;
}

void G_LoadConfig()
{
	GameConfig = new FGameConfigFile();
	GameConfig->DoGlobalSetup();
}

void G_ReadConfig(const char* game)
{
	GameConfig->DoGameSetup(game);
	GameConfig->DoKeySetup(game);

	// Process automatically executed files
	FExecList *exec;
	FArgs *execFiles = new FArgs;
	if (!(Args->CheckParm("-noautoexec")))
		GameConfig->AddAutoexec(execFiles, game);
	exec = D_MultiExec(execFiles, NULL);
	delete execFiles;

	// Process .cfg files at the start of the command line.
	execFiles = Args->GatherFiles ("-exec");
	exec = D_MultiExec(execFiles, exec);
	delete execFiles;

	// [RH] process all + commands on the command line
	exec = C_ParseCmdLineParams(exec);

	// Actually exec command line commands and exec files.
	if (exec != NULL)
	{
		exec->ExecCommands();
		delete exec;
		exec = NULL;
	}

	FBaseCVar::EnableCallbacks();
	GameName = game;
}

void G_SaveConfig()
{
	if (!GameConfig) return;
	GameConfig->ArchiveGlobalData();
	GameConfig->ArchiveGameData(GameName);
	GameConfig->WriteConfigFile();
	delete GameConfig;
	GameConfig = nullptr;
}

