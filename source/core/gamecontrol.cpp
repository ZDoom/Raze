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
#include "razemenu.h"
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
#include "automap.h"
#include "v_draw.h"
#include "gi.h"

CVAR(Bool, autoloadlights, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR(Bool, autoloadbrightmaps, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVARD(Bool, invertmousex, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "invert horizontal mouse movement")
CVARD(Bool, invertmouse, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "invert vertical mouse movement")

EXTERN_CVAR(Bool, ui_generic)

CUSTOM_CVAR(String, language, "auto", CVAR_ARCHIVE | CVAR_NOINITCALL | CVAR_GLOBALCONFIG)
{
	GStrings.UpdateLanguage(self);
	UpdateGenericUI(ui_generic);
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
int lastTic;

extern bool pauseext;

cycle_t thinktime, actortime, gameupdatetime, drawtime;

gamestate_t gamestate = GS_STARTUP;
gameaction_t gameaction = ga_nothing;
// gameaction state
MapRecord* g_nextmap;
int g_nextskill;


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
void MainLoop();


bool AppActive = true;

FString currentGame;
FString LumpFilter;

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

bool System_DispatchEvent(event_t* ev)
{
	if (ev->type == EV_Mouse && !System_WantGuiCapture())
	{
		inputState.MouseAddToPos(ev->x, -ev->y);
		return true;
	}

	inputState.AddEvent(ev);
	return false;
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
namespace Exhumed
{
	::GameInterface* CreateInterface();
}

void CheckFrontend(int flags)
{
	if (flags & GAMEFLAG_BLOOD)
	{
		gi = Blood::CreateInterface();
	}
	else if (flags & GAMEFLAG_SW)
	{
		gi = ShadowWarrior::CreateInterface();
	}
	else if (flags & GAMEFLAG_PSEXHUMED)
	{
		gi = Exhumed::CreateInterface();
	}
	else
	{
		gi = Duke3d::CreateInterface();
	}
}

void I_StartupJoysticks();
void I_ShutdownInput();
int RunGame();
void System_MenuClosed();

int GameMain()
{
	int r;

	sysCallbacks =
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
		nullptr,
		nullptr,
		nullptr,
		System_DispatchEvent,
		validFilter,
		StrTable_GetGender,
		System_MenuClosed,
		nullptr
	};

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
	M_ClearMenus();
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
	gameinfo.gametype = g_gameType;
	return usedgroups;
}

//==========================================================================
//
//
//
//==========================================================================

void InitLanguages()
{
	GStrings.LoadStrings(language);
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
	InitLanguages();


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
	I_InitSound();
	Mus_InitMusic();
	S_ParseSndInfo();
	S_ParseReverbDef();
	InitStatistics();
	LoadScripts();
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
	gameinfo.mBackButton = "engine/graphics/m_back.png";
	gi->app_init();
	SetDefaultMenuColors();
	M_Init();
	BuildGameMenus();
	if (!(paletteloaded & PALETTE_MAIN))
		I_FatalError("No palette found.");

	V_LoadTranslations();   // loading the translations must be delayed until the palettes have been fully set up.
	lookups.postLoadTables();
	TileFiles.PostLoadSetup();
	videoInit();

	D_CheckNetGame();
	MainLoop();
	return 0; // this is never reached. MainLoop only exits via exception.
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
	screen->BeginFrame();
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

void GameInterface::FreeLevelData()
{
	// Make sure that there is no more level to toy around with.
	initspritelists();
	numsectors = numwalls = 0;
	currentLevel = nullptr;
}

//---------------------------------------------------------------------------
//
// Load crosshair definitions
//
//---------------------------------------------------------------------------

FGameTexture* CrosshairImage;
int CrosshairNum;

CVAR(Int, crosshair, 0, CVAR_ARCHIVE)
CVAR(Color, crosshaircolor, 0xff0000, CVAR_ARCHIVE);
CVAR(Int, crosshairhealth, 2, CVAR_ARCHIVE);

void ST_LoadCrosshair(int num, bool alwaysload)
{
	char name[16];

	if (!alwaysload && CrosshairNum == num && CrosshairImage != NULL)
	{ // No change.
		return;
	}

	if (num == 0)
	{
		CrosshairNum = 0;
		CrosshairImage = NULL;
		return;
	}
	if (num < 0)
	{
		num = -num;
	}

	mysnprintf(name, countof(name), "XHAIRB%d", num);
	FTextureID texid = TexMan.CheckForTexture(name, ETextureType::MiscPatch, FTextureManager::TEXMAN_TryAny | FTextureManager::TEXMAN_ShortNameOnly);
	if (!texid.isValid())
	{
		mysnprintf(name, countof(name), "XHAIRB1");
		texid = TexMan.CheckForTexture(name, ETextureType::MiscPatch, FTextureManager::TEXMAN_TryAny | FTextureManager::TEXMAN_ShortNameOnly);
		if (!texid.isValid())
		{
			texid = TexMan.CheckForTexture("XHAIRS1", ETextureType::MiscPatch, FTextureManager::TEXMAN_TryAny | FTextureManager::TEXMAN_ShortNameOnly);
		}
	}
	CrosshairNum = num;
	CrosshairImage = TexMan.GetGameTexture(texid);
}

//---------------------------------------------------------------------------
//
// DrawCrosshair
//
//---------------------------------------------------------------------------

void DrawGenericCrosshair(int num, int phealth, double xdelta)
{
	uint32_t color;
	double size;
	int w, h;

	ST_LoadCrosshair(num, false);

	// Don't draw the crosshair if there is none
	if (CrosshairImage == NULL)
	{
		return;
	}

	float crosshairscale = cl_crosshairscale * 0.005;
	if (crosshairscale > 0.0f)
	{
		size = twod->GetHeight() * crosshairscale / 200.;
	}
	else
	{
		size = 1.;
	}

	w = int(CrosshairImage->GetDisplayWidth() * size);
	h = int(CrosshairImage->GetDisplayHeight() * size);

	if (crosshairhealth == 1) 
	{
		// "Standard" crosshair health (green-red)
		int health = phealth;

		if (health >= 85)
		{
			color = 0x00ff00;
		}
		else
		{
			int red, green;
			health -= 25;
			if (health < 0)
			{
				health = 0;
			}
			if (health < 30)
			{
				red = 255;
				green = health * 255 / 30;
			}
			else
			{
				red = (60 - health) * 255 / 30;
				green = 255;
			}
			color = (red << 16) | (green << 8);
		}
	}
	else if (crosshairhealth == 2)
	{
		// "Enhanced" crosshair health (blue-green-yellow-red)
		int health = clamp(phealth, 0, 200);
		float rr, gg, bb;

		float saturation = health < 150 ? 1.f : 1.f - (health - 150) / 100.f;

		HSVtoRGB(&rr, &gg, &bb, health * 1.2f, saturation, 1);
		int red = int(rr * 255);
		int green = int(gg * 255);
		int blue = int(bb * 255);

		color = (red << 16) | (green << 8) | blue;
	}
	else
	{
		color = crosshaircolor;
	}

	DrawTexture(twod, CrosshairImage,
		(windowxy1.x + windowxy2.x) / 2 + xdelta * (windowxy2.y - windowxy1.y) / 240.,
		(windowxy1.y + windowxy2.y) / 2,
		DTA_DestWidth, w,
		DTA_DestHeight, h,
		DTA_AlphaChannel, true,
		DTA_FillColor, color & 0xFFFFFF,
		TAG_DONE);
}


void DrawCrosshair(int deftile, int health, double xdelta, double ydelta, double scale, PalEntry color)
{
	int type = -1;
	if (automapMode == am_off && cl_crosshair)
	{
		if (deftile < MAXTILES && crosshair == 0)
		{
			auto tile = tileGetTexture(deftile);
			if (tile)
			{
				double crosshair_scale = cl_crosshairscale * .01 * scale;
				DrawTexture(twod, tile, 160 + xdelta, 100 + ydelta, DTA_Color, color,
					DTA_FullscreenScale, FSMode_Fit320x200, DTA_ScaleX, crosshair_scale, DTA_ScaleY, crosshair_scale, DTA_CenterOffsetRel, true,
					DTA_ViewportX, windowxy1.x, DTA_ViewportY, windowxy1.y, DTA_ViewportWidth, windowxy2.x - windowxy1.x + 1, DTA_ViewportHeight, windowxy2.y - windowxy1.y + 1, TAG_DONE);

				return;
			}
		}
		DrawGenericCrosshair(crosshair == 0 ? 2 : *crosshair, health, xdelta);
	}
}
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void LoadDefinitions()
{
	loaddefinitionsfile("engine/engine.def");	// Internal stuff that is required.

	const char* defsfile = G_DefFile();

	cycle_t deftimer;
	deftimer.Reset();
	deftimer.Clock();
	if (!loaddefinitionsfile(defsfile, true))
	{
		deftimer.Unclock();
		Printf(PRINT_NONOTIFY, "Definitions file \"%s\" loaded in %.3f ms.\n", defsfile, deftimer.TimeMS());
	}
	userConfig.AddDefs.reset();

	// load the widescreen replacements last so that they do not clobber the CRC for the original items so that mod-side replacement are picked up.
	if (fileSystem.FindFile("engine/widescreen.def") >= 0 && !Args->CheckParm("-nowidescreen"))
	{
		loaddefinitionsfile("engine/widescreen.def");
	}


}

//---------------------------------------------------------------------------
//
// code fron gameexec/conrun
//
//---------------------------------------------------------------------------

int getincangle(int a, int na)
{
	a &= 2047;
	na &= 2047;

	if(abs(a-na) < 1024)
		return (na-a);
	else
	{
		if(na > 1024) na -= 2048;
		if(a > 1024) a -= 2048;

		na -= 2048;
		a -= 2048;
		return (na-a);
	}
}

fixed_t getincangleq16(fixed_t a, fixed_t na)
{
	a &= 0x7FFFFFF;
	na &= 0x7FFFFFF;

	if(abs(a-na) < IntToFixed(1024))
		return (na-a);
	else
	{
		if(na > IntToFixed(1024)) na -= IntToFixed(2048);
		if(a > IntToFixed(1024)) a -= IntToFixed(2048);

		na -= IntToFixed(2048);
		a -= IntToFixed(2048);
		return (na-a);
	}
}

//---------------------------------------------------------------------------
//
// Player's movement function, called from game's ticker or from gi->GetInput() as required.
//
//---------------------------------------------------------------------------

void processMovement(InputPacket* currInput, InputPacket* inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt, bool const allowstrafe, double const turnscale)
{
	// set up variables
	int const running = !!(inputBuffer->actions & SB_RUN);
	int const keymove = gi->playerKeyMove() << running;
	int const cntrlvelscale = g_gameType & GAMEFLAG_PSEXHUMED ? 8 : 1;
	float const mousevelscale = keymove / 160.f;

	// process mouse and initial controller input.
	if (buttonMap.ButtonDown(gamefunc_Strafe) && allowstrafe)
		currInput->svel -= xs_CRoundToInt(hidInput->mousemovex * mousevelscale + (scaleAdjust * (hidInput->dyaw / 60) * keymove * cntrlvelscale));
	else
		currInput->q16avel += FloatToFixed(hidInput->mouseturnx + (scaleAdjust * hidInput->dyaw));

	if (!(inputBuffer->actions & SB_AIMMODE))
		currInput->q16horz -= FloatToFixed(hidInput->mouseturny);
	else
		currInput->fvel -= xs_CRoundToInt(hidInput->mousemovey * mousevelscale);

	if (invertmouse)
		currInput->q16horz = -currInput->q16horz;

	if (invertmousex)
		currInput->q16avel = -currInput->q16avel;

	// process remaining controller input.
	currInput->q16horz -= FloatToFixed(scaleAdjust * hidInput->dpitch);
	currInput->svel -= xs_CRoundToInt(scaleAdjust * hidInput->dx * keymove * cntrlvelscale);
	currInput->fvel -= xs_CRoundToInt(scaleAdjust * hidInput->dz * keymove * cntrlvelscale);

	// process keyboard turning keys.
	if (buttonMap.ButtonDown(gamefunc_Strafe) && allowstrafe)
	{
		if (abs(inputBuffer->svel) < keymove)
		{
			if (buttonMap.ButtonDown(gamefunc_Turn_Left))
				currInput->svel += keymove;

			if (buttonMap.ButtonDown(gamefunc_Turn_Right))
				currInput->svel -= keymove;
		}
	}
	else
	{
		static double turnheldtime;
		int const turnheldamt = 120 / GameTicRate;
		double const turboturntime = 590. / GameTicRate;
		double turnamount = ((running ? 43375. / 27. : 867.5) / GameTicRate) * turnscale;
		double preambleturn = turnamount / (347. / 92.);

		// allow Exhumed to use its legacy values given the drastic difference from the other games.
		if ((g_gameType & GAMEFLAG_PSEXHUMED) && cl_exhumedoldturn)
		{
			preambleturn = turnamount = running ? 12 : 8;
		}

		if (buttonMap.ButtonDown(gamefunc_Turn_Left) || (buttonMap.ButtonDown(gamefunc_Strafe_Left) && !allowstrafe))
		{
			turnheldtime += scaleAdjust * turnheldamt;
			currInput->q16avel -= FloatToFixed(scaleAdjust * (turnheldtime >= turboturntime ? turnamount : preambleturn));
		}
		else if (buttonMap.ButtonDown(gamefunc_Turn_Right) || (buttonMap.ButtonDown(gamefunc_Strafe_Right) && !allowstrafe))
		{
			turnheldtime += scaleAdjust * turnheldamt;
			currInput->q16avel += FloatToFixed(scaleAdjust * (turnheldtime >= turboturntime ? turnamount : preambleturn));
		}
		else
		{
			turnheldtime = 0;
		}
	}

	// process keyboard forward/side velocity keys.
	if (abs(inputBuffer->svel) < keymove)
	{
		if (buttonMap.ButtonDown(gamefunc_Strafe_Left) && allowstrafe)
			currInput->svel += keymove;

		if (buttonMap.ButtonDown(gamefunc_Strafe_Right) && allowstrafe)
			currInput->svel -= keymove;
	}
	if (abs(inputBuffer->fvel) < keymove)
	{
		if (isRR() && drink_amt >= 66 && drink_amt <= 87)
		{
			if (buttonMap.ButtonDown(gamefunc_Move_Forward))
			{
				currInput->fvel += keymove;
				if (drink_amt & 1)
					currInput->svel += keymove;
				else
					currInput->svel -= keymove;
			}

			if (buttonMap.ButtonDown(gamefunc_Move_Backward))
			{
				currInput->fvel -= keymove;
				if (drink_amt & 1)
					currInput->svel -= keymove;
				else
					currInput->svel += keymove;
			}
		}
		else
		{
			if (buttonMap.ButtonDown(gamefunc_Move_Forward))
				currInput->fvel += keymove;

			if (buttonMap.ButtonDown(gamefunc_Move_Backward))
				currInput->fvel -= keymove;
		}
	}

	// add collected input to game's local input accumulation packet.
	inputBuffer->fvel = clamp(inputBuffer->fvel + currInput->fvel, -keymove, keymove);
	inputBuffer->svel = clamp(inputBuffer->svel + currInput->svel, -keymove, keymove);
	inputBuffer->q16avel += currInput->q16avel;
	inputBuffer->q16horz += currInput->q16horz;
}

//---------------------------------------------------------------------------
//
// Player's horizon function, called from game's ticker or from gi->GetInput() as required.
//
//---------------------------------------------------------------------------

void sethorizon(fixed_t* q16horiz, fixed_t const q16horz, ESyncBits* actions, double const scaleAdjust)
{
	// Calculate adjustment as true pitch (Fixed point math really sucks...)
	double horizAngle = atan2(*q16horiz - IntToFixed(100), IntToFixed(128)) * (512. / pi::pi());

	if (q16horz)
	{
		*actions &= ~SB_CENTERVIEW;
		horizAngle = clamp(horizAngle + FixedToFloat(q16horz), -180, 180);
	}

	// this is the locked type
	if (*actions & (SB_AIM_UP|SB_AIM_DOWN))
	{
		*actions &= ~SB_CENTERVIEW;
		double const amount = 250. / GameTicRate;

		if (*actions & SB_AIM_DOWN)
			horizAngle -= scaleAdjust * amount;

		if (*actions & SB_AIM_UP)
			horizAngle += scaleAdjust * amount;
	}

	// this is the unlocked type
	if (*actions & (SB_LOOK_UP|SB_LOOK_DOWN))
	{
		*actions |= SB_CENTERVIEW;
		double const amount = 500. / GameTicRate;

		if (*actions & SB_LOOK_DOWN)
			horizAngle -= scaleAdjust * amount;

		if (*actions & SB_LOOK_UP)
			horizAngle += scaleAdjust * amount;
	}

	// convert back to Build's horizon
	*q16horiz = IntToFixed(100) + xs_CRoundToInt(IntToFixed(128) * tan(horizAngle * (pi::pi() / 512.)));

	// return to center if conditions met.
	if ((*actions & SB_CENTERVIEW) && !(*actions & (SB_LOOK_UP|SB_LOOK_DOWN)))
	{
		if (*q16horiz < FloatToFixed(99.75) || *q16horiz > FloatToFixed(100.25))
		{
			// move *q16horiz back to 100
			*q16horiz += xs_CRoundToInt(scaleAdjust * (((1000. / GameTicRate) * FRACUNIT) - (*q16horiz * (10. / GameTicRate))));
		}
		else
		{
			// not looking anymore because *q16horiz is back at 100
			*q16horiz = IntToFixed(100);
			*actions &= ~SB_CENTERVIEW;
		}
	}

	// clamp before returning
	*q16horiz = clamp(*q16horiz, gi->playerHorizMin(), gi->playerHorizMax());
}

//---------------------------------------------------------------------------
//
// Player's angle function, called from game's ticker or from gi->GetInput() as required.
//
//---------------------------------------------------------------------------

void applylook(fixed_t* q16ang, fixed_t* q16look_ang, fixed_t* q16rotscrnang, fixed_t* spin, fixed_t const q16avel, ESyncBits* actions, double const scaleAdjust, bool const crouching)
{
	// return q16rotscrnang to 0 and set to 0 if less than a quarter of a FRACUNIT (16384)
	*q16rotscrnang -= xs_CRoundToInt(scaleAdjust * (*q16rotscrnang * (15. / GameTicRate)));
	if (abs(*q16rotscrnang) < (FRACUNIT >> 2)) *q16rotscrnang = 0;

	// return q16look_ang to 0 and set to 0 if less than a quarter of a FRACUNIT (16384)
	*q16look_ang -= xs_CRoundToInt(scaleAdjust * (*q16look_ang * (7.5 / GameTicRate)));
	if (abs(*q16look_ang) < (FRACUNIT >> 2)) *q16look_ang = 0;

	if (*actions & SB_LOOK_LEFT)
	{
		// start looking left
		*q16look_ang -= FloatToFixed(scaleAdjust * (4560. / GameTicRate));
		*q16rotscrnang += FloatToFixed(scaleAdjust * (720. / GameTicRate));
	}

	if (*actions & SB_LOOK_RIGHT)
	{
		// start looking right
		*q16look_ang += FloatToFixed(scaleAdjust * (4560. / GameTicRate));
		*q16rotscrnang -= FloatToFixed(scaleAdjust * (720. / GameTicRate));
	}

	if (*actions & SB_TURNAROUND)
	{
		if (*spin == 0)
		{
			// currently not spinning, so start a spin
			*spin = IntToFixed(-1024);
		}
		*actions &= ~SB_TURNAROUND;
	}

	if (*spin < 0)
	{
		// return spin to 0
		fixed_t add = FloatToFixed(scaleAdjust * ((!crouching ? 3840. : 1920.) / GameTicRate));
		*spin += add;
		if (*spin > 0)
		{
			// Don't overshoot our target. With variable factor this is possible.
			add -= *spin;
			*spin = 0;
		}
		*q16ang += add;
	}

	if (q16avel)
	{
		// add player's input
		*q16ang = (*q16ang + q16avel) & 0x7FFFFFF;
	}
}

//---------------------------------------------------------------------------
//
// Player's ticrate helper functions.
//
//---------------------------------------------------------------------------

void playerAddAngle(fixed_t* q16ang, double* helper, double adjustment)
{
	if (!cl_syncinput)
	{
		*helper += adjustment;
	}
	else
	{
		*q16ang = (*q16ang + FloatToFixed(adjustment)) & 0x7FFFFFF;
	}
}

void playerSetAngle(fixed_t* q16ang, fixed_t* helper, double adjustment)
{
	if (!cl_syncinput)
	{
		// Add slight offset if adjustment is coming in as absolute 0.
		if (adjustment == 0) adjustment += (1. / (FRACUNIT >> 1));

		*helper = *q16ang + getincangleq16(*q16ang, FloatToFixed(adjustment));
	}
	else
	{
		*q16ang = FloatToFixed(adjustment);
	}
}

void playerAddHoriz(fixed_t* q16horiz, double* helper, double adjustment)
{
	if (!cl_syncinput)
	{
		*helper += adjustment;
	}
	else
	{
		*q16horiz += FloatToFixed(adjustment);
	}
}

void playerSetHoriz(fixed_t* q16horiz, fixed_t* helper, double adjustment)
{
	if (!cl_syncinput)
	{
		// Add slight offset if adjustment is coming in as absolute 0.
		if (adjustment == 0) adjustment += (1. / (FRACUNIT >> 1));

		*helper = FloatToFixed(adjustment);
	}
	else
	{
		*q16horiz = FloatToFixed(adjustment);
	}
}

//---------------------------------------------------------------------------
//
// Player's ticrate helper processor.
//
//---------------------------------------------------------------------------

void playerProcessHelpers(fixed_t* q16ang, double* angAdjust, fixed_t* angTarget, fixed_t* q16horiz, double* horizAdjust, fixed_t* horizTarget, double const scaleAdjust)
{
	// Process angle amendments from the game's ticker.
	if (*angTarget)
	{
		fixed_t angDelta = getincangleq16(*q16ang, *angTarget);
		*q16ang = (*q16ang + xs_CRoundToInt(scaleAdjust * angDelta));

		if (abs(*q16ang - *angTarget) < FRACUNIT)
		{
			*q16ang = *angTarget;
			*angTarget = 0;
		}
	}
	else if (*angAdjust)
	{
		*q16ang = (*q16ang + FloatToFixed(scaleAdjust * *angAdjust)) & 0x7FFFFFF;
	}

	// Process horizon amendments from the game's ticker.
	if (*horizTarget)
	{
		fixed_t horizDelta = *horizTarget - *q16horiz;
		*q16horiz += xs_CRoundToInt(scaleAdjust * horizDelta);

		if (abs(*q16horiz - *horizTarget) < FRACUNIT)
		{
			*q16horiz = *horizTarget;
			*horizTarget = 0;
		}
	}
	else if (*horizAdjust)
	{
		*q16horiz += FloatToFixed(scaleAdjust * *horizAdjust);
	}
}

bool M_Active()
{
	return CurrentMenu != nullptr || ConsoleState == c_down || ConsoleState == c_falling;
}

struct gamefilter
{
	const char* gamename;
	int gameflag;
};

static const gamefilter games[] = {
	{ "Duke", GAMEFLAG_DUKE},
	{ "Nam", GAMEFLAG_NAM | GAMEFLAG_NAPALM},
	{ "NamOnly", GAMEFLAG_NAM},	// for cases where the difference matters.
	{ "Napalm", GAMEFLAG_NAPALM},
	{ "WW2GI", GAMEFLAG_WW2GI},
	{ "Redneck", GAMEFLAG_RR},
	{ "RedneckRides", GAMEFLAG_RRRA},
	{ "Deer", GAMEFLAG_DEER},
	{ "Blood", GAMEFLAG_BLOOD},
	{ "ShadowWarrior", GAMEFLAG_SW},
	{ "Exhumed", GAMEFLAG_POWERSLAVE | GAMEFLAG_EXHUMED},
	{ "Plutopak", GAMEFLAG_PLUTOPAK},
	{ "Worldtour", GAMEFLAG_WORLDTOUR},
	{ "Shareware", GAMEFLAG_SHAREWARE},
};

bool validFilter(const char* str)
{
	for (auto& gf : games)
	{
		if (g_gameType & gf.gameflag)
		{
			if (!stricmp(str, gf.gamename)) return true;
		}
	}
	return false;
}

#include "vm.h"

DEFINE_ACTION_FUNCTION(_Screen, GetViewWindow)
{
	PARAM_PROLOGUE;
	if (numret > 0) ret[0].SetInt(windowxy1.x);
	if (numret > 1) ret[1].SetInt(windowxy1.y);
	if (numret > 2) ret[2].SetInt(windowxy2.x - windowxy1.x + 1);
	if (numret > 3) ret[3].SetInt(windowxy2.y - windowxy1.y + 1);
	return MIN(numret, 4);
}

DEFINE_ACTION_FUNCTION_NATIVE(_Build, ShadeToLight, shadeToLight)
{
	PARAM_PROLOGUE;
	PARAM_INT(shade);
	ACTION_RETURN_INT(shadeToLight(shade));
}

extern bool demoplayback;
DEFINE_GLOBAL(multiplayer)
DEFINE_GLOBAL(netgame)
DEFINE_GLOBAL(gameaction)
DEFINE_GLOBAL(gamestate)
DEFINE_GLOBAL(demoplayback)
DEFINE_GLOBAL(consoleplayer)
