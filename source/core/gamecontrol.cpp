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
#include "s_soundinternal.h"
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

CUSTOM_CVAR(String, language, "auto", CVAR_ARCHIVE | CVAR_NOINITCALL | CVAR_GLOBALCONFIG)
{
	GStrings.UpdateLanguage(self);
}

// The last remains of sdlayer.cpp
double g_beforeSwapTime;
GameInterface* gi;
int myconnectindex, numplayers;
int connecthead, connectpoint2[MAXMULTIPLAYERS];
int32_t xres = -1, yres = -1, bpp = 0, refreshfreq = -1;
auto vsnprintfptr = vsnprintf;	// This is an inline in Visual Studio but we need an address for it to satisfy the MinGW compiled libraries.


MapRecord mapList[512];		// Due to how this gets used it needs to be static. EDuke defines 7 episode plus one spare episode with 64 potential levels each and relies on the static array which is freely accessible by scripts.
MapRecord *currentLevel;	// level that is currently played. (The real level, not what script hacks modfifying the current level index can pretend.)
MapRecord* lastLevel;		// Same here, for the last level.
MapRecord userMapRecord;	// stand-in for the user map.

gamestate_t gamestate = GS_STARTUP;

FILE* hashfile;

FStartupInfo RazeStartupInfo;
FMemArena dump;	// this is for memory blocks than cannot be deallocated without some huge effort. Put them in here so that they do not register on shutdown.

void C_CON_SetAliases();
InputState inputState;
void SetClipshapes();
int ShowStartupWindow(TArray<GrpEntry> &);
FString GetGameFronUserFiles();
void InitFileSystem(TArray<GrpEntry>&);
void I_SetWindowTitle(const char* caption);
void InitENet();
void ShutdownENet();
void S_ParseSndInfo();

bool AppActive;
int chatmodeon;	// needed by the common console code.

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



//==========================================================================
//
//
//
//==========================================================================

bool grab_mouse;

void mouseGrabInput(bool grab)
{
	grab_mouse = grab;
	if (grab) GUICapture &= ~1;
	else GUICapture |= 1;
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

#if 0 // MP disabled pending evaluation
	auto v = Args->CheckValue("-port");
	if (v) netPort = strtol(v, nullptr, 0);

	netServerMode = Args->CheckParm("-server");
	netServerAddress = Args->CheckValue("-connect");
	netPassword = Args->CheckValue("-password");
#endif

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

	static const char* nomos[] = { "-nomonsters", "-nodudes", nullptr };
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

namespace Duke
{
	::GameInterface* CreateInterface();
}
namespace Redneck
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

CVAR(Bool, duke_compatibility_15, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

void CheckFrontend(int flags)
{
	bool duke_compat = duke_compatibility_15;
	// This point is too early to have cmdline CVAR checkers working so it must be with a switch.
	auto c = Args->CheckValue("-duke_compatibility_15");
	if (c)
	{
		if (strtol(c, nullptr, 0)) duke_compatibility_15 = true;
		else duke_compatibility_15 = false;
	}
	if (flags & GAMEFLAG_BLOOD)
	{
		gi = Blood::CreateInterface();
	}
	else if (flags & GAMEFLAG_RRALL)
	{
		gi = Redneck::CreateInterface();
	}
	else if (flags & GAMEFLAG_SW)
	{
		gi = ShadowWarrior::CreateInterface();
	}
	else if (flags & GAMEFLAG_PSEXHUMED)
	{
		gi = Powerslave::CreateInterface();
	}
	else if ((flags & GAMEFLAG_FURY) || RazeStartupInfo.modern > 0)
	{
		gi = Duke::CreateInterface();
	}
	else if (RazeStartupInfo.modern < 0)
	{
		gi = Redneck::CreateInterface();
	}
	else
	{
		gi = *duke_compatibility_15 ? Redneck::CreateInterface() : Duke::CreateInterface();
	}

}

void I_StartupJoysticks();
void I_ShutdownInput();
int RunGame();

int GameMain()
{
	int r;
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
	TileFiles.ClearTextureCache();
	TileFiles.CloseAll();	// do this before shutting down graphics.
	GLInterface.Deinit();
	I_ShutdownGraphics();
	M_DeinitMenus();
	engineUnInit();
	if (gi)
	{
		delete gi;
		gi = nullptr;
	}
	InitENet();
	DeleteStartupScreen();
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

	if (RazeStartupInfo.Name.IsNotEmpty()) I_SetWindowTitle(RazeStartupInfo.Name);
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
	SetClipshapes();
	userConfig.ProcessOptions();
	G_LoadConfig();
	ShutdownENet();
	auto usedgroups = SetupGame();


	InitFileSystem(usedgroups);
	if (usedgroups.Size() == 0) return 0;

	// Handle CVARs with game specific defaults here.
	if (g_gameType & GAMEFLAG_BLOOD)
	{
		mus_redbook.SetGenericRepDefault(false, CVAR_Bool);	// Blood should default to CD Audio off - all other games must default to on.
		hud_size.SetGenericRepDefault(6, CVAR_Int);
		hud_size_max = 7;
	}
	if (g_gameType & GAMEFLAG_SW)
	{
		hud_size.SetGenericRepDefault(8, CVAR_Int);
		hud_size_max = 9;
		cl_weaponswitch.SetGenericRepDefault(1, CVAR_Int);
		if (cl_weaponswitch > 1) cl_weaponswitch = 1;
	}
	if (g_gameType & GAMEFLAG_PSEXHUMED)
	{
		hud_size.SetGenericRepDefault(7, CVAR_Int);
		hud_size_max = 8;
	}

	G_ReadConfig(currentGame);

	V_InitFontColors();
	GStrings.LoadStrings(language);

	I_Init();
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
	V_InitFonts();
	C_CON_SetAliases();
	sfx_empty = fileSystem.FindFile("engine/dsempty.lmp"); // this must be done outside the sound code because it's initialized late.
	I_InitSound();
	Mus_InitMusic();
	timerSetCallback(Mus_UpdateMusic);
	S_ParseSndInfo();
	InitStatistics();
	M_Init();
	SetDefaultStrings();
	if (g_gameType & (GAMEFLAG_RR)) InitRREndMap();	// this needs to be done better later
	if (Args->CheckParm("-sounddebug"))
		C_DoCommand("stat sounddebug");

	if (enginePreInit())
	{
		I_FatalError("app_main: There was a problem initializing the Build engine: %s\n", engineerrstr);
	}

	mouseGrabInput(true);	// the intros require the mouse to be grabbed.

	auto exec = C_ParseCmdLineParams(nullptr);
	if (exec) exec->ExecCommands();

	gamestate = GS_LEVEL;
	return gi->app_main();
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
			if (strlen(*s) == 0)
				*s = sc.String;
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
// S_SetSoundPaused
//
// Called with state non-zero when the app is active, zero when it isn't.
//
//==========================================================================

void S_SetSoundPaused(int state)
{
#if 0
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
				GSnd->SetInactive(gamestate == GS_LEVEL || gamestate == GS_TITLELEVEL ?
					SoundRenderer::INACTIVE_Complete :
					SoundRenderer::INACTIVE_Mute);
			}
		}
	}
	if (!netgame
#ifdef _DEBUG
		&& !demoplayback
#endif
		)
	{
		pauseext = !state;
	}
#endif
}

int CalcSmoothRatio(const ClockTicks &totalclk, const ClockTicks &ototalclk, int realgameticspersec)
{
	const double TICRATE = 120.;

	double rfreq = (refreshfreq != -1 ? refreshfreq : 60);
	rfreq = rfreq * TICRATE / timerGetClockRate();

	double elapsedTime = (totalclk - ototalclk).toScale16F();
	double elapsedFrames = elapsedTime * rfreq * (1. / TICRATE);
	double ratio = (elapsedFrames * realgameticspersec) / rfreq;
	return clamp(xs_RoundToInt(ratio * 65536), 0, 65536);
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
				gi->PrintMessage(PRINT_MEDIUM, "\"%s\" = \"%s\"\n", var->GetName(), val.Bool ? "true" : "false");
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
					if (text) gi->PrintMessage(PRINT_MEDIUM, "%s\n", text);
				}
			}
		}
	}
}

// Just a placeholder for now.
bool CheckCheatmode(bool printmsg)
{
	return false;
}

