//-------------------------------------------------------------------------
/*
Copyright (C) 2019 Christoph Oelckers

This is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
//-------------------------------------------------------------------------

#include <stdexcept>
#include "gamecontrol.h"
#include "tarray.h"
#include "zstring.h"
#include "name.h"
#include "sc_man.h"
#include "c_cvars.h"
#include "gameconfigfile.h"
#include "gamecvars.h"
#include "build.h"
#include "inputstate.h"
#include "m_argv.h"
#include "rts.h"
#include "printf.h"
#include "c_bind.h"
#include "v_font.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "i_specialpaths.h"
#include "raze_music.h"
#include "statistics.h"
#include "menu.h"
#include "gstrings.h"
#include "quotemgr.h"
#include "mapinfo.h"
#include "raze_sound.h"
#include "i_system.h"
#include "inputstate.h"
#include "v_video.h"
#include "st_start.h"
#include "s_music.h"
#include "i_video.h"
#include "v_text.h"
#include "resourcefile.h"
#include "c_dispatch.h"
#include "glbackend/glbackend.h"
#include "engineerrors.h"
#include "mmulti.h"
#include "gamestate.h"
#include "gstrings.h"
#include "texturemanager.h"
#include "i_interface.h"
#include "x86.h"
#include "startupinfo.h"
#include "mapinfo.h"
#include "menustate.h"
#include "screenjob.h"
#include "statusbar.h"
#include "uiinput.h"
#include "d_net.h"

CVAR(Bool, autoloadlights, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR(Bool, autoloadbrightmaps, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

CUSTOM_CVAR(String, language, "auto", CVAR_ARCHIVE | CVAR_NOINITCALL | CVAR_GLOBALCONFIG)
{
	GStrings.UpdateLanguage(self);
}

CUSTOM_CVAR(Int, mouse_capturemode, 1, CVAR_GLOBALCONFIG | CVAR_ARCHIVE)
{
	if (self < 0)
	{
		self = 0;
	}
	else if (self > 2)
	{
		self = 2;
	}
}

// The last remains of sdlayer.cpp
GameInterface* gi;
int myconnectindex, numplayers;
int connecthead, connectpoint2[MAXMULTIPLAYERS];
auto vsnprintfptr = vsnprintf;	// This is an inline in Visual Studio but we need an address for it to satisfy the MinGW compiled libraries.
int gameclock, gameclockstart;
int lastTic;

int automapMode;
bool automapFollow;
extern bool pauseext;

CCMD(togglemap)
{
	if (gamestate == GS_LEVEL)
	{
		automapMode++;
		if (automapMode == am_count) automapMode = am_off;
		if ((g_gameType & GAMEFLAG_BLOOD) && automapMode == am_overlay) automapMode = am_full; // todo: investigate if this can be re-enabled
		gi->ResetFollowPos(false);
	}
}

CCMD(togglefollow)
{
	automapFollow = !automapFollow;
	gi->ResetFollowPos(true);
}

cycle_t thinktime, actortime, gameupdatetime, drawtime;

gamestate_t gamestate = GS_STARTUP;

FILE* hashfile;

FStartupInfo GameStartupInfo;
FMemArena dump;	// this is for memory blocks than cannot be deallocated without some huge effort. Put them in here so that they do not register on shutdown.

InputState inputState;
int ShowStartupWindow(TArray<GrpEntry> &);
FString GetGameFronUserFiles();
void InitFileSystem(TArray<GrpEntry>&);
void I_SetWindowTitle(const char* caption);
void S_ParseSndInfo();
void I_DetectOS(void);
void LoadScripts();
void app_loop();
void MainLoop();


bool AppActive = true;

FString currentGame;
FString LumpFilter;
TMap<FName, int32_t> NameToTileIndex; // for assigning names to tiles. The menu accesses this list. By default it gets everything from the dynamic tile map in Duke Nukem and Redneck Rampage.
										// Todo: Add additional definition file for the other games or textures not in that list so that the menu does not have to rely on indices.

CVAR(Bool, queryiwad, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);
CVAR(String, defaultiwad, "", CVAR_ARCHIVE | CVAR_GLOBALCONFIG);
CVAR(Bool, disableautoload, false, CVAR_ARCHIVE | CVAR_NOINITCALL | CVAR_GLOBALCONFIG)
//CVAR(Bool, autoloadbrightmaps, false, CVAR_ARCHIVE | CVAR_NOINITCALL | CVAR_GLOBALCONFIG)	// hopefully this is an option for later
//CVAR(Bool, autoloadlights, false, CVAR_ARCHIVE | CVAR_NOINITCALL | CVAR_GLOBALCONFIG)

extern int hud_size_max;

int paused;
bool pausedWithKey;

CUSTOM_CVAR(Int, cl_gender, 0, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	if (self < 0 || self > 3) self = 0;
}

int StrTable_GetGender()
{
	return cl_gender;
}

bool validFilter(const char* str);

static StringtableCallbacks stblcb =
{
	validFilter,
	StrTable_GetGender
};

extern int chatmodeon;

bool System_WantGuiCapture()
{
	bool wantCapt;

	if (menuactive == MENU_Off)
	{
		wantCapt = ConsoleState == c_down || ConsoleState == c_falling || chatmodeon;
	}
	else
	{
		wantCapt = (menuactive == MENU_On || menuactive == MENU_OnNoPause);
	}
	return wantCapt;
}

bool System_WantLeftButton()
{
	return false;// (gamestate == GS_MENUSCREEN || gamestate == GS_TITLELEVEL);
}

bool System_NetGame()
{
	return false;	// fixme later. For now there is no netgame support.
}

bool System_WantNativeMouse()
{
	return false;
}

static bool System_CaptureModeInGame()
{
	return true;
}

static bool System_DisableTextureFilter()
{
	return  hw_useindexedcolortextures;
}

static IntRect System_GetSceneRect()
{
	// Special handling so the view with a visible status bar displays properly
	int height = windowxy2.y - windowxy1.y + 1, width = windowxy2.x - windowxy1.x + 1;

	IntRect mSceneViewport;
	mSceneViewport.left = windowxy1.x;
	mSceneViewport.top = windowxy1.y;
	mSceneViewport.width = width;
	mSceneViewport.height = height;
	return mSceneViewport;
}

//==========================================================================
//
// DoomSpecificInfo
//
// Called by the crash logger to get application-specific information.
//
//==========================================================================

void System_CrashInfo(char* buffer, size_t bufflen, const char *lfstr)
{
	const char* arg;
	char* const buffend = buffer + bufflen - 2;	// -2 for CRLF at end
	int i;

	buffer += mysnprintf(buffer, buffend - buffer, GAMENAME " version %s (%s)", GetVersionString(), GetGitHash());

	buffer += snprintf(buffer, buffend - buffer, "%sCommand line:", lfstr);
	for (i = 0; i < Args->NumArgs(); ++i)
	{
		buffer += snprintf(buffer, buffend - buffer, " %s", Args->GetArg(i));
	}

	for (i = 0; (arg = fileSystem.GetResourceFileName(i)) != NULL; ++i)
	{
		buffer += mysnprintf(buffer, buffend - buffer, "%sFile %d: %s", lfstr, i, arg);
	}
	buffer += mysnprintf(buffer, buffend - buffer, "%s", lfstr);
	*buffer = 0;
}



//==========================================================================
//
//
//
//==========================================================================

UserConfig userConfig;

void UserConfig::ProcessOptions()
{
	// -help etc are omitted

	// -cfg / -setupfile refer to Build style config which are not supported.
	if (Args->CheckParm("-cfg") || Args->CheckParm("-setupfile"))
	{
		Printf("Build-format config files not supported and will be ignored\n");
	}

	auto v = Args->CheckValue("-addon");
	if (v)
	{
		auto val = strtol(v, nullptr, 0);
		static const char* const addons[] = { "DUKE3D.GRP", "DUKEDC.GRP", "NWINTER.GRP", "VACATION.GRP" };
		if (val >= 0 && val < 4) gamegrp = addons[val];
		else Printf("%s: Unknown Addon\n", v);
	}
	else if (Args->CheckParm("-nam"))
	{
		gamegrp = "NAM.GRP";
	}
	else if (Args->CheckParm("-napalm"))
	{
		gamegrp = "NAPALM.GRP";
	}
	else if (Args->CheckParm("-ww2gi"))
	{
		gamegrp = "WW2GI.GRP";
	}
	// Set up all needed content for these two mod which feature a very messy distribution.
	// As an alternative they can be zipped up - the launcher will be able to detect and set up such versions automatically.
	else if (Args->CheckParm("-route66"))
	{
		gamegrp = "REDNECK.GRP";
		DefaultCon = "GAME66.CON";
		const char* argv[] = { "tilesa66.art" , "tilesb66.art" };
		AddArt.reset(new FArgs(2, argv));
		toBeDeleted.Push("turd66.anm*turdmov.anm");
		toBeDeleted.Push("turd66.voc*turdmov.voc");
		toBeDeleted.Push("end66.anm*rr_outro.anm");
		toBeDeleted.Push("end66.voc*rr_outro.voc");
	}
	else if (Args->CheckParm("-cryptic"))
	{
		gamegrp = "BLOOD.RFF";
		DefaultCon = "CRYPTIC.INI";
		const char* argv[] = { "cpart07.ar_" , "cpart15.ar_" };
		AddArt.reset(new FArgs(2, argv));
	}

	v = Args->CheckValue("-gamegrp");
	if (v)
	{
		gamegrp = v;
	}
	else
	{
		// This is to enable the use of Doom launchers. that are limited to -iwad for specifying the game's main resource.
		v = Args->CheckValue("-iwad");
		if (v)
		{
			gamegrp = v;
		}
	}

	Args->CollectFiles("-rts", ".rts");
	auto rts = Args->CheckValue("-rts");
	if (rts) RTS_Init(rts);

	Args->CollectFiles("-map", ".map");
	CommandMap = Args->CheckValue("-map");

	static const char* defs[] = { "-def", "-h", nullptr };
	Args->CollectFiles("-def", defs, ".def");
	DefaultDef = Args->CheckValue("-def");

	if (DefaultCon.IsEmpty())
	{
		static const char* cons[] = { "-con", "-x", nullptr };
		Args->CollectFiles("-con", cons, ".con");
		DefaultCon = Args->CheckValue("-con");
		if (DefaultCon.IsEmpty()) DefaultCon = Args->CheckValue("-ini");
	}

	static const char* demos[] = { "-playback", "-d", "-demo", nullptr };
	Args->CollectFiles("-demo", demos, ".dmo");
	CommandDemo = Args->CheckValue("-demo");

	static const char* names[] = { "-pname", "-name", nullptr };
	Args->CollectFiles("-name", names, ".---");	// this shouldn't collect any file names at all so use a nonsense extension
	CommandName = Args->CheckValue("-name");

	static const char* nomos[] = { "-nomonsters", "-nodudes", "-nocreatures", nullptr };
	Args->CollectFiles("-nomonsters", nomos, ".---");	// this shouldn't collect any file names at all so use a nonsense extension
	nomonsters = Args->CheckParm("-nomonsters");

	static const char* acons[] = { "-addcon", "-mx", nullptr };
	Args->CollectFiles("-addcon", acons, ".con");
	AddCons.reset(Args->GatherFiles("-addcon"));

	static const char* adefs[] = { "-adddef", "-mh", nullptr };
	Args->CollectFiles("-adddef", adefs, ".def");
	AddDefs.reset(Args->GatherFiles("-adddef"));

	Args->CollectFiles("-art", ".art");
	AddArt.reset(Args->GatherFiles("-art"));

	nologo = Args->CheckParm("-nologo") || Args->CheckParm("-quick");
	nosound = Args->CheckParm("-nosfx") || Args->CheckParm("-nosound");
	if (Args->CheckParm("-setup")) queryiwad = 1;
	else if (Args->CheckParm("-nosetup")) queryiwad = 0;


	if (Args->CheckParm("-file"))
	{
		// For file loading there's two modes:
		// If -file is given, all content will be processed in order and the legacy options be ignored entirely.
		//This allows mixing directories and GRP files in arbitrary order.
		Args->CollectFiles("-file", NULL);
		AddFiles.reset(Args->GatherFiles("-file"));
	}
	else
	{
		// Trying to emulate Build. This means to treat RFF files as lowest priority, then all GRPs and then all directories. 
		// This is only for people depending on lauchers. Since the semantics are so crappy it is strongly recommended to
		// use -file instead which gives the user full control over the order in which things are added.
		// For single mods this is no problem but don't even think about loading more stuff consistently...

		static const char* grps[] = { "-g", "-grp", nullptr };
		static const char* dirs[] = { "-game_dir", "-j",  nullptr };
		static const char* rffs[] = { "-rff", "-snd",  nullptr };
		static const char* twostep[] = { "-rff", "-grp",  nullptr };

		// Abuse the inner workings to get the files into proper order. This is not 100% accurate but should work fine for everything that doesn't intentionally fuck things up.
		Args->CollectFiles("-rff", rffs, ".rff");
		Args->CollectFiles("-grp", grps, nullptr);
		Args->CollectFiles("-grp", twostep, nullptr);	// The two previous calls have already brought the content in order so collecting it again gives us one list with everything.
		AddFilesPre.reset(Args->GatherFiles("-grp"));
		Args->CollectFiles("-game_dir", dirs, nullptr);
		AddFiles.reset(Args->GatherFiles("-game_dir"));
	}
	if (Args->CheckParm("-showcoords") || Args->CheckParm("-w"))
	{
		C_DoCommand("stat coord");
	}

}

//==========================================================================
//
//
//
//==========================================================================

void CheckUserMap()
{
	if (userConfig.CommandMap.IsEmpty()) return;
	FString startupMap = userConfig.CommandMap;
	if (startupMap.IndexOfAny("/\\") < 0) startupMap.Insert(0, "/");
	DefaultExtension(startupMap, ".map");
	startupMap.Substitute("\\", "/");
	NormalizeFileName(startupMap);

	if (fileSystem.FileExists(startupMap))
	{
		Printf("Using level: \"%s\".\n", startupMap.GetChars());
	}
	else
	{
		Printf("Level \"%s\" not found.\n", startupMap.GetChars());
		startupMap = "";
	}
	userConfig.CommandMap = startupMap;
}

//==========================================================================
//
//
//
//==========================================================================

namespace Duke3d
{
	::GameInterface* CreateInterface();
}
namespace Blood
{
	::GameInterface* CreateInterface();
}
namespace ShadowWarrior
{
	::GameInterface* CreateInterface();
}
namespace Powerslave
{
	::GameInterface* CreateInterface();
}

extern int MinFPSRate; // this is a bit messy.

void CheckFrontend(int flags)
{
	if (flags & GAMEFLAG_BLOOD)
	{
		gi = Blood::CreateInterface();
	}
	else if (flags & GAMEFLAG_SW)
	{
		MinFPSRate = 40;
		gi = ShadowWarrior::CreateInterface();
	}
	else if (flags & GAMEFLAG_PSEXHUMED)
	{
		gi = Powerslave::CreateInterface();
	}
	else
	{
		gi = Duke3d::CreateInterface();
	}
}

void I_StartupJoysticks();
void I_ShutdownInput();
int RunGame();

int GameMain()
{
	int r;

	static SystemCallbacks syscb =
	{
		System_WantGuiCapture,
		System_WantLeftButton,
		System_NetGame,
		System_WantNativeMouse,
		System_CaptureModeInGame,
		nullptr,
		nullptr,
		nullptr,
		System_DisableTextureFilter,
		nullptr,
		System_GetSceneRect,
	};
	sysCallbacks = &syscb;

	try
	{
		r = RunGame();
	}
	catch (const CExitEvent & exit)
	{
		// Just let the rest of the function execute.
		r = exit.Reason();
	}
	catch (const std::exception & err)
	{
		// shut down critical systems before showing a message box.
		I_ShowFatalError(err.what());
		r = -1;
	}
	DeleteScreenJob();
	M_ClearMenus(true);
	if (gi)
	{
		gi->FreeGameData();	// Must be done before taking down any subsystems.
	}
	S_StopMusic(true);
	if (soundEngine) delete soundEngine;
	soundEngine = nullptr;
	I_CloseSound();
	I_ShutdownInput();
	G_SaveConfig();
	C_DeinitConsole();
	V_ClearFonts();
	vox_deinit();
	TexMan.DeleteAll();
	TileFiles.CloseAll();	// delete the texture data before shutting down graphics.
	GLInterface.Deinit();
	I_ShutdownGraphics();
	M_DeinitMenus();
	engineUnInit();
	if (gi)
	{
		delete gi;
		gi = nullptr;
	}
	DeleteStartupScreen();
	PClass::StaticShutdown();
	if (Args) delete Args;
	return r;
}

//==========================================================================
//
//
//
//==========================================================================

void SetDefaultStrings()
{
	if ((g_gameType & GAMEFLAG_DUKE) && fileSystem.FindFile("E4L1.MAP") < 0)
	{
		// Pre-Atomic releases do not define this.
		gVolumeNames[0] = "$L.A. Meltdown";
		gVolumeNames[1] = "$Lunar Apocalypse";
		gVolumeNames[2] = "$Shrapnel City";
		if (g_gameType & GAMEFLAG_SHAREWARE) gVolumeNames[3] = "$The Birth";
		gSkillNames[0] = "$Piece of Cake";
		gSkillNames[1] = "$Let's Rock";
		gSkillNames[2] = "$Come get Some";
		gSkillNames[3] = "$Damn I'm Good";
	}
	// Blood hard codes its skill names, so we have to define them manually.
	if (g_gameType & GAMEFLAG_BLOOD)
	{
		gSkillNames[0] = "$STILL KICKING";
		gSkillNames[1] = "$PINK ON THE INSIDE";
		gSkillNames[2] = "$LIGHTLY BROILED";
		gSkillNames[3] = "$WELL DONE";
		gSkillNames[4] = "$EXTRA CRISPY";
	}
	
	//Set a few quotes which are used for common handling of a few status messages
	quoteMgr.InitializeQuote(23, "$MESSAGES: ON");
	quoteMgr.InitializeQuote(24, "$MESSAGES: OFF");
	quoteMgr.InitializeQuote(83, "$FOLLOW MODE OFF");
	quoteMgr.InitializeQuote(84, "$FOLLOW MODE ON");
	quoteMgr.InitializeQuote(85, "$AUTORUNOFF");
	quoteMgr.InitializeQuote(86, "$AUTORUNON");
}

//==========================================================================
//
//
//
//==========================================================================

static TArray<GrpEntry> SetupGame()
{
	// Startup dialog must be presented here so that everything can be set up before reading the keybinds.

	auto groups = GrpScan();
	if (groups.Size() == 0)
	{
		// Abort if no game data found.
		G_SaveConfig();
		I_Error("Unable to find any game data. Please verify your settings.");
	}

	decltype(groups) usedgroups;

	int groupno = -1;

	// If the user has specified a file name, let's see if we know it.
	//
	FString game = GetGameFronUserFiles();
	if (userConfig.gamegrp.IsEmpty())
	{
		userConfig.gamegrp = game;
	}

	if (userConfig.gamegrp.Len())
	{
		FString gamegrplower = "/" + userConfig.gamegrp.MakeLower();

		int g = 0;
		for (auto& grp : groups)
		{
			auto grplower = grp.FileName.MakeLower();
			grplower.Substitute("\\", "/");
			if (grplower.LastIndexOf(gamegrplower) == grplower.Len() - gamegrplower.Len())
			{
				groupno = g;
				break;
			}
			g++;
		}
	}

	if (groupno == -1)
	{
		int pick = 0;

		// We got more than one so present the IWAD selection box.
		if (groups.Size() > 1)
		{
			// Locate the user's prefered IWAD, if it was found.
			if (defaultiwad[0] != '\0')
			{
				for (unsigned i = 0; i < groups.Size(); ++i)
				{
					FString& basename = groups[i].FileName;
					if (stricmp(basename, defaultiwad) == 0)
					{
						pick = i;
						break;
					}
				}
			}
			if (groups.Size() > 1)
			{
				TArray<WadStuff> wads;
				for (auto& found : groups)
				{
					WadStuff stuff;
					stuff.Name = found.FileInfo.name;
					stuff.Path = ExtractFileBase(found.FileName);
					wads.Push(stuff);
				}
				pick = I_PickIWad(&wads[0], (int)wads.Size(), queryiwad, pick);
				if (pick >= 0)
				{
					// The newly selected IWAD becomes the new default
					defaultiwad = groups[pick].FileName;
				}
				groupno = pick;
			}
		}
        else if (groups.Size() == 1)
        {
            groupno = 0;
        }
	}

	if (groupno == -1) return TArray<GrpEntry>();
	auto& group = groups[groupno];

	if (GameStartupInfo.Name.IsNotEmpty()) I_SetWindowTitle(GameStartupInfo.Name);
	else I_SetWindowTitle(group.FileInfo.name);

	// Now filter out the data we actually need and delete the rest.

	usedgroups.Push(group);

	auto crc = group.FileInfo.dependencyCRC;
	if (crc != 0) for (auto& dep : groups)
	{
		if (dep.FileInfo.CRC == crc)
		{
			usedgroups.Insert(0, dep);	// Order from least dependent to most dependent, which is the loading order of data.
		}
	}
	groups.Reset();

	FString selectedScript;
	FString selectedDef;
	for (auto& ugroup : usedgroups)
	{
		// For CONs the command line has priority, aside from that, the last one wins. For Blood this handles INIs - the rules are the same.
		if (ugroup.FileInfo.scriptname.IsNotEmpty()) selectedScript = ugroup.FileInfo.scriptname;
		if (ugroup.FileInfo.defname.IsNotEmpty()) selectedDef = ugroup.FileInfo.defname;

		// CVAR has priority. This also overwrites the global variable each time. Init here is lazy so this is ok.
		if (ugroup.FileInfo.rtsname.IsNotEmpty() && **rtsname == 0) RTS_Init(ugroup.FileInfo.rtsname);

		// For the game filter the last non-empty one wins.
		if (ugroup.FileInfo.gamefilter.IsNotEmpty()) LumpFilter = ugroup.FileInfo.gamefilter;
		g_gameType |= ugroup.FileInfo.flags;
	}
	if (userConfig.DefaultCon.IsEmpty()) userConfig.DefaultCon = selectedScript;
	if (userConfig.DefaultDef.IsEmpty()) userConfig.DefaultDef = selectedDef;

	// This can only happen with a custom game that does not define any filter.
	// In this case take the display name and strip all whitespace and invaliid path characters from it.
	if (LumpFilter.IsEmpty())
	{
		LumpFilter = usedgroups.Last().FileInfo.name;
		LumpFilter.StripChars(".:/\\<>?\"*| \t\r\n");
	}

	currentGame = LumpFilter;
	currentGame.Truncate(currentGame.IndexOf("."));
	CheckFrontend(g_gameType);
	return usedgroups;
}

//==========================================================================
//
//
//
//==========================================================================

int RunGame()
{
	// Set up the console before anything else so that it can receive text.
	C_InitConsole(1024, 768, true);

	// +logfile gets checked too late to catch the full startup log in the logfile so do some extra check for it here.
	FString logfile = Args->TakeValue("+logfile");

	// As long as this engine is still in prerelease mode let's always write a log file.
	if (logfile.IsEmpty()) logfile.Format("%s" GAMENAMELOWERCASE ".log", M_GetDocumentsPath().GetChars());

	if (logfile.IsNotEmpty())
	{
		execLogfile(logfile);
	}
	I_DetectOS();
	userConfig.ProcessOptions();
	G_LoadConfig();
	auto usedgroups = SetupGame();


	InitFileSystem(usedgroups);
	if (usedgroups.Size() == 0) return 0;

	// Handle CVARs with game specific defaults here.
	if (g_gameType & GAMEFLAG_BLOOD)
	{
		mus_redbook.SetGenericRepDefault(false, CVAR_Bool);	// Blood should default to CD Audio off - all other games must default to on.
		am_showlabel.SetGenericRepDefault(true, CVAR_Bool);
	}
	if (g_gameType & GAMEFLAG_SW)
	{
		cl_weaponswitch.SetGenericRepDefault(1, CVAR_Int);
		if (cl_weaponswitch > 1) cl_weaponswitch = 1;
	}
	if (g_gameType & (GAMEFLAG_BLOOD|GAMEFLAG_RR))
	{
		am_nameontop.SetGenericRepDefault(true, CVAR_Bool);	// Blood and RR show the map name on the top of the screen by default.
	}

	G_ReadConfig(currentGame);

	V_InitFontColors();
	GStrings.LoadStrings(language);

	CheckCPUID(&CPU);
	CalculateCPUSpeed();
	auto ci = DumpCPUInfo(&CPU);
	Printf("%s", ci.GetChars());

	V_InitScreenSize();
	V_InitScreen();
	StartScreen = FStartupScreen::CreateInstance(100);

	TArray<FString> addArt;
	for (auto& grp : usedgroups)
	{
		for (auto& art : grp.FileInfo.loadart)
		{
			addArt.Push(art);
		}
	}
	if (userConfig.AddArt) for (auto& art : *userConfig.AddArt)
	{
		addArt.Push(art);
	}
	TileFiles.AddArt(addArt);

	inputState.ClearAllInput();
	
	if (!GameConfig->IsInitialized())
	{
		CONFIG_ReadCombatMacros();
	}

	if (userConfig.CommandName.IsNotEmpty())
	{
		playername = userConfig.CommandName;
	}
	GameTicRate = 30;
	CheckUserMap();
	GPalette.Init(MAXPALOOKUPS + 2);    // one slot for each translation, plus a separate one for the base palettes and the internal one
	TexMan.Init([]() {}, [](BuildInfo &) {});
	V_InitFonts();
	TileFiles.Init();
	sfx_empty = fileSystem.FindFile("engine/dsempty.lmp"); // this must be done outside the sound code because it's initialized late.
	I_InitSound();
	Mus_InitMusic();
	S_ParseSndInfo();
	InitStatistics();
	LoadScripts();
	M_Init();
	SetDefaultStrings();
	if (Args->CheckParm("-sounddebug"))
		C_DoCommand("stat sounddebug");

	if (enginePreInit())
	{
		I_FatalError("There was a problem initializing the Build engine: %s\n", engineerrstr);
	}

	auto exec = C_ParseCmdLineParams(nullptr);
	if (exec) exec->ExecCommands();

	SetupGameButtons();
	gi->app_init();
	enginePostInit(); // This must not be done earlier!
	videoInit();

	// Duke has transitioned to the new main loop, the other games haven't yet.
	if (g_gameType & (GAMEFLAG_DUKE | GAMEFLAG_RRALL | GAMEFLAG_NAM | GAMEFLAG_NAPALM | GAMEFLAG_WW2GI))
	{
		D_CheckNetGame();
		MainLoop();
	}
	else app_loop();
	return 0; // this is never reached. app_loop only exits via exception.
}

//---------------------------------------------------------------------------
//
// The one and only main loop in the entire engine. Yay!
//
//---------------------------------------------------------------------------


void TickSubsystems()
{
	// run these on an independent timer until we got something working for the games.
	static const uint64_t tickInterval = 1'000'000'000 / 30;
	static uint64_t nexttick = 0;

	auto nowtick = I_nsTime();
	if (nexttick == 0) nexttick = nowtick;
	int cnt = 0;
	while (nexttick <= nowtick && cnt < 5)
	{
		nexttick += tickInterval;
		C_Ticker();
		M_Ticker();
		C_RunDelayedCommands();
		cnt++;
	}
	// If this took too long the engine was most likely suspended so recalibrate the timer.
	// Perfect precision is not needed here.
	if (cnt == 5) nexttick = nowtick + tickInterval;
}

void updatePauseStatus()
{
	// This must go through the network in multiplayer games.
	if (M_Active() || System_WantGuiCapture())
	{
		paused = 1;
	}
	else if (!M_Active() || !System_WantGuiCapture())
	{
		if (!pausedWithKey)
		{
			paused = 0;
		}

		if (sendPause)
		{
			sendPause = false;
			paused = pausedWithKey ? 0 : 2;
			pausedWithKey = !!paused;
		}
	}

	paused ? S_PauseSound(!pausedWithKey, !paused) : S_ResumeSound(paused);
}


void app_loop()
{
	gamestate = GS_STARTUP;

	while (true)
	{
		try
		{
			I_SetFrameTime();
			TickSubsystems();

			handleevents();
			updatePauseStatus();
			D_ProcessEvents();

			gi->RunGameFrame();

			// Draw overlay elements to the 2D drawer
			FStat::PrintStat(twod);
			CT_Drawer();
			C_DrawConsole();
			M_Drawer();

			// Handle the final 2D overlays.
			if (gamestate == GS_LEVEL) DrawFullscreenBlends();
			DrawRateStuff();

			soundEngine->UpdateSounds(I_GetTime());
			Mus_UpdateMusic();		// must be at the end.

			videoShowFrame(0);
			videoSetBrightness(0);	// immediately reset this so that the value doesn't stick around in the backend.
		}
		catch (CRecoverableError& err)
		{
			gi->ErrorCleanup();
			C_FullConsole();
			Printf(TEXTCOLOR_RED "%s\n", err.what());
		}
	}
}


//==========================================================================
//
// 
//
//==========================================================================

void PolymostProcessVoxels(void);

void videoInit()
{
	lookups.postLoadLookups();
	V_Init2();
	videoSetGameMode(vid_fullscreen, screen->GetWidth(), screen->GetHeight(), 32, 1);

	PolymostProcessVoxels();
	GLInterface.Init(screen->GetWidth());
	GLInterface.InitGLState(4, 4/*glmultisample*/);
	screen->SetTextureFilterMode();
	setViewport(hud_size);
}

void G_FatalEngineError(void)
{
	I_FatalError("There was a problem initializing the engine: %s\n\nThe application will now close.", engineerrstr);
}

//==========================================================================
//
// 
//
//==========================================================================

CVAR(String, combatmacro0, "", CVAR_ARCHIVE | CVAR_USERINFO)
CVAR(String, combatmacro1, "", CVAR_ARCHIVE | CVAR_USERINFO)
CVAR(String, combatmacro2, "", CVAR_ARCHIVE | CVAR_USERINFO)
CVAR(String, combatmacro3, "", CVAR_ARCHIVE | CVAR_USERINFO)
CVAR(String, combatmacro4, "", CVAR_ARCHIVE | CVAR_USERINFO)
CVAR(String, combatmacro5, "", CVAR_ARCHIVE | CVAR_USERINFO)
CVAR(String, combatmacro6, "", CVAR_ARCHIVE | CVAR_USERINFO)
CVAR(String, combatmacro7, "", CVAR_ARCHIVE | CVAR_USERINFO)
CVAR(String, combatmacro8, "", CVAR_ARCHIVE | CVAR_USERINFO)
CVAR(String, combatmacro9, "", CVAR_ARCHIVE | CVAR_USERINFO)
FStringCVar* const CombatMacros[] = { &combatmacro0, &combatmacro1, &combatmacro2, &combatmacro3, &combatmacro4, &combatmacro5, &combatmacro6, &combatmacro7, &combatmacro8, &combatmacro9};

void CONFIG_ReadCombatMacros()
{
	FScanner sc;
	try
	{
		sc.Open("engine/combatmacros.txt");
		for (auto s : CombatMacros)
		{
			sc.MustGetToken(TK_StringConst);
			UCVarValue val;
			val.String = sc.String;
			s->SetGenericRepDefault(val, CVAR_String);
		}
	}
	catch (const CRecoverableError &)
	{
		// We do not want this to error out. Just ignore if it fails.
	}
}

//==========================================================================
//
// 
//
//==========================================================================

static FString CONFIG_GetMD4EntryName(uint8_t const* const md4)
{
	return FStringf("MD4_%08x%08x%08x%08x",
		B_BIG32(B_UNBUF32(&md4[0])), B_BIG32(B_UNBUF32(&md4[4])),
		B_BIG32(B_UNBUF32(&md4[8])), B_BIG32(B_UNBUF32(&md4[12])));
}

int32_t CONFIG_GetMapBestTime(char const* const mapname, uint8_t const* const mapmd4)
{

	auto m = CONFIG_GetMD4EntryName(mapmd4);
	if (GameConfig->SetSection("MapTimes"))
	{
		auto s = GameConfig->GetValueForKey(m);
		if (s) return (int)strtoull(s, nullptr, 0);
	}
	return -1;
}

int CONFIG_SetMapBestTime(uint8_t const* const mapmd4, int32_t tm)
{
	FStringf t("%d", tm);
	auto m = CONFIG_GetMD4EntryName(mapmd4);
	if (GameConfig->SetSection("MapTimes"))
	{
		GameConfig->SetValueForKey(m, t);
	}
	return 0;
}


CCMD(snd_reset)
{
	Mus_Stop();
	if (soundEngine) soundEngine->Reset();
	Mus_ResumeSaved();
}

//==========================================================================
//
// S_PauseSound
//
// Stop music and sound effects, during game PAUSE.
//
//==========================================================================

void S_PauseSound (bool notmusic, bool notsfx)
{
	if (!notmusic)
	{
		S_PauseMusic();
	}
	if (!notsfx)
	{
		soundEngine->SetPaused(true);
		GSnd->SetSfxPaused (true, 0);
	}
}

//==========================================================================
//
// S_ResumeSound
//
// Resume music and sound effects, after game PAUSE.
//
//==========================================================================

void S_ResumeSound (bool notsfx)
{
	S_ResumeMusic();
	if (!notsfx)
	{
		soundEngine->SetPaused(false);
		GSnd->SetSfxPaused (false, 0);
	}
}

//==========================================================================
//
// S_SetSoundPaused
//
// Called with state non-zero when the app is active, zero when it isn't.
//
//==========================================================================

void S_SetSoundPaused(int state)
{
	if (state)
	{
		if (paused == 0)
		{
			S_ResumeSound(true);
			if (GSnd != nullptr)
			{
				GSnd->SetInactive(SoundRenderer::INACTIVE_Active);
			}
		}
	}
	else
	{
		if (paused == 0)
		{
			S_PauseSound(false, true);
			if (GSnd != nullptr)
			{
				GSnd->SetInactive(SoundRenderer::INACTIVE_Complete);
			}
		}
	}
#if 0
	if (!netgame
#if 0 //def _DEBUG
		&& !demoplayback
#endif
		)
	{
		pauseext = !state;
	}
#endif
}

FString G_GetDemoPath()
{
	FString path = M_GetDemoPath();

	path << LumpFilter << '/';
	CreatePath(path);

	return path;
}

CCMD(printinterface)
{
	Printf("Current interface is %s\n", gi->Name());
}

CCMD (togglemsg)
{
	FBaseCVar *var, *prev;
	UCVarValue val;

	if (argv.argc() > 1)
	{
		if ( (var = FindCVar (argv[1], &prev)) )
		{
			var->MarkUnsafe();

			val = var->GetGenericRep (CVAR_Bool);
			val.Bool = !val.Bool;
			var->SetGenericRep (val, CVAR_Bool);
			const char *statestr = argv.argc() <= 2? "*" : argv[2];
			if (*statestr == '*')
			{
				Printf(PRINT_MEDIUM|PRINT_NOTIFY, "\"%s\" = \"%s\"\n", var->GetName(), val.Bool ? "true" : "false");
			}
			else
			{
				int state = (int)strtoll(argv[2], nullptr,  0);
				if (state != 0)
				{
					// Order of Duke's quote string varies, some have on first, some off, so use the sign of the parameter to decide.
					// Positive means Off/On, negative means On/Off
					int quote = state > 0? state + val.Bool : -(state + val.Bool);
					auto text = quoteMgr.GetQuote(quote);
					if (text) Printf(PRINT_MEDIUM|PRINT_NOTIFY, "%s\n", text);
				}
			}
		}
	}
}

// Just a placeholder for now.
bool CheckCheatmode(bool printmsg)
{
	return gi->CheatAllowed(printmsg);
}

bool OkForLocalization(FTextureID texnum, const char* substitute)
{
	return false;
}


// Mainly a dummy.
CCMD(taunt)
{
	if (argv.argc() > 2)
	{
		int taunt = atoi(argv[1]);
		int mode = atoi(argv[2]);
		
		// In a ZDoom-style protocol this should be sent:
		// Net_WriteByte(DEM_TAUNT);
		// Net_WriteByte(taunt);
		// Net_WriteByte(mode);
		if (mode == 1)
		{
			// todo:
			//gi->PlayTaunt(taunt);
			// Duke:
			// startrts(taunt, 1)
			// Blood:
			// sndStartSample(4400 + taunt, 128, 1, 0);
			// SW:
			// PlaySoundRTS(taunt);
			// Exhumed does not implement RTS, should be like Duke
			//
		}
		Printf(PRINT_NOTIFY, "%s", **CombatMacros[taunt - 1]);

	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void startmainmenu()
{
	gamestate = GS_MENUSCREEN;
	M_StartControlPanel(false);
	M_SetMenu(NAME_Mainmenu);
	FX_StopAllSounds();
}

