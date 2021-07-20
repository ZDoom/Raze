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
#include "gamestate.h"
#include "gstrings.h"
#include "texturemanager.h"
#include "i_interface.h"
#include "x86.h"
#include "startupinfo.h"
#include "mapinfo.h"
#include "menustate.h"
#include "screenjob_.h"
#include "statusbar.h"
#include "uiinput.h"
#include "d_net.h"
#include "automap.h"
#include "v_draw.h"
#include "gi.h"
#include "vm.h"
#include "g_mapinfo.h"
#include "gamefuncs.h"
#include "hw_voxels.h"
#include "hw_palmanager.h"
#include "razefont.h"

CVAR(Bool, autoloadlights, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR(Bool, autoloadbrightmaps, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Bool, autoloadwidescreen, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

// Note: For the automap label there is a separate option "am_textfont".
CVARD(Bool, hud_textfont, false, CVAR_ARCHIVE, "Use the regular text font as replacement for the tiny 3x5 font for HUD messages whenever possible")

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
int connecthead, connectpoint2[MAXPLAYERS];
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
TArray<FString> GetGameFronUserFiles();
void InitFileSystem(TArray<GrpEntry>&);
void I_SetWindowTitle(const char* caption);
void S_ParseSndInfo();
void I_DetectOS(void);
void LoadScripts();
void MainLoop();
void SetConsoleNotifyBuffer();
bool PreBindTexture(FRenderState* state, FGameTexture*& tex, EUpscaleFlags& flags, int& scaleflags, int& clampmode, int& translation, int& overrideshader);
void highTileSetup();
void FontCharCreated(FGameTexture* base, FGameTexture* untranslated);
void LoadVoxelModels();

DStatusBarCore* StatusBar;


bool AppActive = true;

FString currentGame;
FString LumpFilter;

CVAR(Bool, queryiwad, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);
CVAR(String, defaultiwad, "", CVAR_ARCHIVE | CVAR_GLOBALCONFIG);
CVAR(Bool, disableautoload, false, CVAR_ARCHIVE | CVAR_NOINITCALL | CVAR_GLOBALCONFIG)

extern int hud_size_max;

int paused;
bool pausedWithKey;

bool gamesetinput = false;

int PlayClock;

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
	int viewbottom = windowxy2.y + 1;
	int viewheight = viewbottom - windowxy1.y;
	int viewright = windowxy2.x + 1;
	int viewwidth = viewright - windowxy1.x;

	int renderheight;
	
	if (viewheight == screen->GetHeight()) renderheight = viewheight;
	else renderheight = (viewwidth * screen->GetHeight() / screen->GetWidth()) & ~7;

	IntRect mSceneViewport;
	mSceneViewport.left = windowxy1.x;
	mSceneViewport.top = screen->GetHeight() - (renderheight + windowxy1.y - ((renderheight - viewheight) / 2));
	mSceneViewport.width = viewwidth;
	mSceneViewport.height = renderheight;
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

DEFINE_GLOBAL(userConfig)
DEFINE_FIELD_X(UserConfigStruct, UserConfig, nomonsters)
DEFINE_FIELD_X(UserConfigStruct, UserConfig, nosound)
DEFINE_FIELD_X(UserConfigStruct, UserConfig, nologo)

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
		const char* argv[] = { "CPART07.AR_", "CPART15.AR_" };
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
	UserDef = Args->CheckValue("-def");

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
	if (FindMapByName(userConfig.CommandMap))
	{
		return;	// we already got a record for this map so no need for further checks.
	}
	FString startupMap = userConfig.CommandMap;
	DefaultExtension(startupMap, ".map");
	startupMap.Substitute("\\", "/");
	NormalizeFileName(startupMap);

	if (!fileSystem.FileExists(startupMap))
	{
		Printf(PRINT_HIGH, "Level \"%s\" not found.\n", startupMap.GetChars());
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

static void System_ToggleFullConsole()
{
	gameaction = ga_fullconsole;
}

static void System_StartCutscene(bool blockui)
{
	gameaction = blockui ? ga_intro : ga_intermission;
}

void I_StartupJoysticks();
void I_ShutdownInput();
int RunGame();
void System_MenuClosed();
void System_MenuDim();

int GameMain()
{
	int r;

	SetConsoleNotifyBuffer();
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
		System_MenuDim,
		nullptr,
		System_DispatchEvent,
		validFilter,
		StrTable_GetGender,
		System_MenuClosed,
		nullptr,
		nullptr,
		PreBindTexture,
		FontCharCreated,
		System_ToggleFullConsole,
		System_StartCutscene,
	};

	try
	{
		r = RunGame();
	}
	catch (const CExitEvent& exit)
	{
		// Just let the rest of the function execute.
		r = exit.Reason();
	}
	catch (const std::exception& err)
	{
		// shut down critical systems before showing a message box.
		I_ShowFatalError(err.what());
		r = -1;
	}
	//DeleteScreenJob();
	DeinitMenus();
	if (StatusBar) StatusBar->Destroy();
	StatusBar = nullptr;
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
	voxClear();
	ClearPalManager();
	TexMan.DeleteAll();
	TileFiles.CloseAll();	// delete the texture data before shutting down graphics.
	I_ShutdownGraphics();
	freeallmodels();
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
	// Blood hard codes its skill names, so we have to define them manually.
	if (isBlood())
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
		I_Error("Unable to find any game data. Please verify your settings."
#ifdef WIN32
		);
#else
		"\nInstall game data files in subfolders of '%s'\n\n", M_GetAppDataPath(false).GetChars());
#endif
	}

	decltype(groups) usedgroups;

	int groupno = -1;

	auto game = GetGameFronUserFiles();
	if (userConfig.gamegrp.IsEmpty())
	{
		for (auto& str : game)
		{
			int g = 0;
			for (auto& grp : groups)
			{
				if (grp.FileInfo.gameid.CompareNoCase(str) == 0)
				{
					userConfig.gamegrp = grp.FileName;
					groupno = g;
					goto foundit;
				}
				g++;
			}
		}
	}
	foundit:

	// If the user has specified a file name, let's see if we know it.
	//
	if (groupno == -1 && userConfig.gamegrp.Len())
	{
		FString gamegrplower = userConfig.gamegrp.MakeLower();
		if (gamegrplower[1] != ':' || gamegrplower[2] != '/') gamegrplower.Insert(0, "/");

		int g = 0;
		for (auto& grp : groups)
		{
			auto grplower = grp.FileName.MakeLower();
			FixPathSeperator(grplower);
			int pos = grplower.LastIndexOf(gamegrplower);
			if (pos >= 0 && pos == grplower.Len() - gamegrplower.Len())
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
	if (userConfig.DefaultCon.IsEmpty()) userConfig.DefaultCon = GameStartupInfo.con.IsNotEmpty()? GameStartupInfo.con : selectedScript;
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
	PClass::StaticInit();
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


void CreateStatusBar()
{
	auto stbarclass = PClass::FindClass(globalCutscenes.StatusBarClass);
	if (!stbarclass)
	{
		I_FatalError("No status bar defined");
	}
	StatusBar = static_cast<DStatusBarCore*>(stbarclass->CreateNew());
	StatusBar->SetSize(0, 320, 200);
	InitStatusBar();
	GC::AddMarkerFunc([]() { GC::Mark(StatusBar); });
}


void GetGames()
{
	auto getgames = Args->CheckValue("-getgames");
	if (getgames)
	{
		try
		{
			auto groups = GrpScan();
			FSerializer arc;
			if (arc.OpenWriter())
			{
				if (arc.BeginArray("games"))
				{
					for (auto& entry : groups)
					{
						if (arc.BeginObject(nullptr))
						{
							arc("filename", entry.FileName)
								("description", entry.FileInfo.name)
								("defname", entry.FileInfo.defname)
								("scriptname", entry.FileInfo.scriptname)
								("gamefilter", entry.FileInfo.gamefilter)
								("gameid", entry.FileInfo.gameid)
								("fgcolor", entry.FileInfo.FgColor)
								("bkcolor", entry.FileInfo.BgColor)
								("addon", entry.FileInfo.isAddon)
								.EndObject();
						}
					}
					arc.EndArray();
				}
				unsigned int len;
				auto p = arc.GetOutput(&len);
				FILE* f = fopen(getgames, "wb");
				if (f)
				{
					fwrite(p, 1, len, f);
					fclose(f);
				}
			}
		}
		catch (...)
		{
			// Ignore all errors
		}
		throw CExitEvent(0);
	}
}

//==========================================================================
//
//
//
//==========================================================================

static void InitTextures()
{
	TexMan.usefullnames = true;
	TexMan.Init([]() {}, [](BuildInfo&) {});
	StartScreen->Progress();
	mdinit();

	TileFiles.Init();
	TileFiles.LoadArtSet("tiles%03d.art"); // it's the same for all games.
	voxInit();
	gi->LoadGameTextures(); // loads game-side data that must be present before processing the .def files.
	LoadDefinitions();
	InitFont();				// InitFonts may only be called once all texture data has been initialized.

	lookups.postLoadTables();
	highTileSetup();
	lookups.postLoadLookups();
	SetupFontSubstitution();
	V_LoadTranslations();   // loading the translations must be delayed until the palettes have been fully set up.
	TileFiles.SetBackup();
}

//==========================================================================
//
//
//
//==========================================================================

int RunGame()
{
	GameStartupInfo.FgColor = 0xffffff;

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
	GetGames();
	auto usedgroups = SetupGame();

	bool colorset = false;
	for (int i = usedgroups.Size()-1; i >= 0; i--)
	{
		auto& grp = usedgroups[i];
		if (grp.FileInfo.name.IsNotEmpty())
		{
			if (GameStartupInfo.Name.IsEmpty()) GameStartupInfo.Name = grp.FileInfo.name;
			if (!colorset && grp.FileInfo.FgColor != grp.FileInfo.BgColor && (GameStartupInfo.FgColor != 0 || GameStartupInfo.BkColor != 0))
			{
				GameStartupInfo.FgColor = grp.FileInfo.FgColor;
				GameStartupInfo.BkColor = grp.FileInfo.BgColor;
				colorset = true;
			}
		}
		if (grp.FileInfo.exclepisodes.Size())
		{
			for (auto& episode : grp.FileInfo.exclepisodes)
			{
				gi->AddExcludedEpisode(episode);
			}
		}
	}
	I_SetIWADInfo();

	InitFileSystem(usedgroups);
	if (usedgroups.Size() == 0) return 0;

	// Handle CVARs with game specific defaults here.
	if (isBlood())
	{
		mus_redbook.SetGenericRepDefault(false, CVAR_Bool);	// Blood should default to CD Audio off - all other games must default to on.
		am_showlabel.SetGenericRepDefault(true, CVAR_Bool);
	}
	if (isSWALL())
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
	StartScreen = FStartupScreen::CreateInstance(8);
	StartScreen->Progress();

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
	gi->loadPalette();
	StartScreen->Progress();
	InitTextures();

	StartScreen->Progress();
	I_InitSound();
	StartScreen->Progress();
	Mus_InitMusic();
	S_ParseSndInfo();
	S_ParseReverbDef();
	InitStatistics();
	LoadScripts();
	StartScreen->Progress();
	SetDefaultStrings();
	Job_Init();
	Local_Job_Init();
	if (Args->CheckParm("-sounddebug"))
		C_DoCommand("stat sounddebug");

	SetupGameButtons();
	gameinfo.mBackButton = "engine/graphics/m_back.png";
	StartScreen->Progress();

	engineInit();
	gi->app_init();
	StartScreen->Progress();
	G_ParseMapInfo();
	CreateStatusBar();
	SetDefaultMenuColors();
	M_Init();
	BuildGameMenus();
	StartScreen->Progress();
	if (!(paletteloaded & PALETTE_MAIN))
		I_FatalError("No palette found.");

	FMaterial::SetLayerCallback(setpalettelayer);
	if (GameStartupInfo.Name.IsNotEmpty()) I_SetWindowTitle(GameStartupInfo.Name);
	DeleteStartupScreen();

	V_Init2();
	twod->Begin(screen->GetWidth(), screen->GetHeight());
	twod->End();
	UpdateJoystickMenu(NULL);
	UpdateVRModes();

	setVideoMode();

	LoadVoxelModels();
	GLInterface.Init(screen->GetWidth());
	screen->BeginFrame();
	screen->SetTextureFilterMode();
	setViewport(hud_size);

	D_CheckNetGame();
	UpdateGenericUI(ui_generic);
	MainLoop();
	return 0; // this is never reached. MainLoop only exits via exception.
}

//---------------------------------------------------------------------------
//
// The one and only main loop in the entire engine. Yay!
//
//---------------------------------------------------------------------------

void updatePauseStatus()
{
	// This must go through the network in multiplayer games.
	if (M_Active() || System_WantGuiCapture() || !AppActive)
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

	if (paused)
		S_PauseSound(!pausedWithKey, !paused);
	else 
		S_ResumeSound(paused);
}

//==========================================================================
//
// 
//
//==========================================================================

void LoadVoxelModels(void);

void setVideoMode()
{
	xdim = screen->GetWidth();
	ydim = screen->GetHeight();
	V_UpdateModeSize(xdim, ydim);
	videoSetViewableArea(0, 0, xdim - 1, ydim - 1);
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
		S_PauseAllCustomStreams(true);
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
		S_PauseAllCustomStreams(false);
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
		}
		if (GSnd != nullptr)
		{
			GSnd->SetInactive(SoundRenderer::INACTIVE_Active);
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


void GameInterface::loadPalette()
{
	paletteLoadFromDisk();
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
// DrawCrosshair
//
//---------------------------------------------------------------------------

void ST_DrawCrosshair(int phealth, double xpos, double ypos, double scale);
//void DrawGenericCrosshair(int num, int phealth, double xdelta);
void ST_LoadCrosshair(int num, bool alwaysload);
CVAR(Int, crosshair, 0, CVAR_ARCHIVE)


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
				double crosshair_scale = crosshairscale * scale;
				DrawTexture(twod, tile, 160 + xdelta, 100 + ydelta, DTA_Color, color,
					DTA_FullscreenScale, FSMode_Fit320x200, DTA_ScaleX, crosshair_scale, DTA_ScaleY, crosshair_scale, DTA_CenterOffsetRel, true,
					DTA_ViewportX, windowxy1.x, DTA_ViewportY, windowxy1.y, DTA_ViewportWidth, windowxy2.x - windowxy1.x + 1, DTA_ViewportHeight, windowxy2.y - windowxy1.y + 1, TAG_DONE);

				return;
			}
		}
		// 0 means 'game provided crosshair' - use type 2 as fallback.
		ST_LoadCrosshair(crosshair == 0 ? 2 : *crosshair, false);

		double xpos = (windowxy1.x + windowxy2.x) / 2 + xdelta * (windowxy2.y - windowxy1.y) / 240.;
		double ypos = (windowxy1.y + windowxy2.y) / 2;
		ST_DrawCrosshair(health, xpos, ypos, 1);
	}
}
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void LoadDefinitions()
{
	const char* defsfile = G_DefFile();
	FString razedefsfile = defsfile;
	razedefsfile.Substitute(".def", "-raze.def");

	loaddefinitionsfile("engine/engine.def", true, true);	// Internal stuff that is required.

	// check what we have.
	// user .defs override the default ones and are not cumulative.
	// if we fine even one Raze-specific file, all of those will be loaded cumulatively.
	// otherwise the default rules inherited from older ports apply.
	if (userConfig.UserDef.IsNotEmpty())
	{
		loaddefinitionsfile(userConfig.UserDef, false);
	}
	else
	{
		if (fileSystem.FileExists(razedefsfile))
		{
			loaddefinitionsfile(razedefsfile, true);
		}
		else if (fileSystem.FileExists(defsfile))
		{
			loaddefinitionsfile(defsfile, false);
		}
	}

	if (userConfig.AddDefs)
	{
		for (auto& m : *userConfig.AddDefs)
		{
			loaddefinitionsfile(m, false);
		}
		userConfig.AddDefs.reset();
	}

	if (GameStartupInfo.def.IsNotEmpty())
	{
		loaddefinitionsfile(GameStartupInfo.def);	// Stuff from gameinfo.
	}

	// load the widescreen replacements last. This ensures that mods still get the correct CRCs for their own tile replacements.
	if (fileSystem.FindFile("engine/widescreen.def") >= 0 && !Args->CheckParm("-nowidescreen"))
	{
		loaddefinitionsfile("engine/widescreen.def");
	}
	fileSystem.InitHashChains(); // make sure that any resources that got added can be found again.
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

DEFINE_ACTION_FUNCTION_NATIVE(_Raze, ShadeToLight, shadeToLight)
{
	PARAM_PROLOGUE;
	PARAM_INT(shade);
	ACTION_RETURN_INT(shadeToLight(shade));
}

DEFINE_ACTION_FUNCTION(_Raze, PlayerName)
{
	PARAM_PROLOGUE;
	PARAM_INT(index);
	ACTION_RETURN_STRING(unsigned(index) >= MAXPLAYERS ? "" : PlayerName(index));
}

DEFINE_ACTION_FUNCTION_NATIVE(_Raze, bsin, bsin)
{
	PARAM_PROLOGUE;
	PARAM_INT(v);
	PARAM_INT(shift);
	ACTION_RETURN_INT(bsin(v, shift));
}

DEFINE_ACTION_FUNCTION_NATIVE(_Raze, bcos, bcos)
{
	PARAM_PROLOGUE;
	PARAM_INT(v);
	PARAM_INT(shift);
	ACTION_RETURN_INT(bcos(v, shift));
}

DEFINE_ACTION_FUNCTION_NATIVE(_Raze, GetBuildTime, I_GetBuildTime)
{
	ACTION_RETURN_INT(I_GetBuildTime());
}

DEFINE_ACTION_FUNCTION(_Raze, PickTexture)
{
	PARAM_PROLOGUE;
	PARAM_INT(texid);
	TexturePick pick;
	if (PickTexture(TexMan.GetGameTexture(FSetTextureID(texid)), TRANSLATION(Translation_Remap, 0), pick))
	{
		ACTION_RETURN_INT(pick.texture->GetID().GetIndex());
	}
	ACTION_RETURN_INT(texid);
}

DEFINE_ACTION_FUNCTION(_MapRecord, GetCluster)
{
	PARAM_SELF_STRUCT_PROLOGUE(MapRecord);
	ACTION_RETURN_POINTER(FindCluster(self->cluster));
}

DEFINE_ACTION_FUNCTION(_Screen, GetTextScreenSize)
{
	ACTION_RETURN_VEC2(DVector2(640, 480));
}

extern bool demoplayback;
DEFINE_GLOBAL(multiplayer)
DEFINE_GLOBAL(netgame)
DEFINE_GLOBAL(gameaction)
DEFINE_GLOBAL(gamestate)
DEFINE_GLOBAL(demoplayback)
DEFINE_GLOBAL(consoleplayer)
DEFINE_GLOBAL(currentLevel)
DEFINE_GLOBAL(paused)
DEFINE_GLOBAL(automapMode)
DEFINE_GLOBAL(PlayClock)

DEFINE_FIELD_X(ClusterDef, ClusterDef, name)
DEFINE_FIELD_X(ClusterDef, ClusterDef, InterBackground)

DEFINE_FIELD_X(MapRecord, MapRecord, parTime)
DEFINE_FIELD_X(MapRecord, MapRecord, designerTime)
DEFINE_FIELD_X(MapRecord, MapRecord, fileName)
DEFINE_FIELD_X(MapRecord, MapRecord, labelName)
DEFINE_FIELD_X(MapRecord, MapRecord, name)
DEFINE_FIELD_X(MapRecord, MapRecord, music)
DEFINE_FIELD_X(MapRecord, MapRecord, cdSongId)
DEFINE_FIELD_X(MapRecord, MapRecord, flags)
DEFINE_FIELD_X(MapRecord, MapRecord, levelNumber)
DEFINE_FIELD_X(MapRecord, MapRecord, cluster)
DEFINE_FIELD_X(MapRecord, MapRecord, NextMap)
DEFINE_FIELD_X(MapRecord, MapRecord, NextSecret)
//native readonly String messages[MAX_MESSAGES];
DEFINE_FIELD_X(MapRecord, MapRecord, Author)
DEFINE_FIELD_X(MapRecord, MapRecord, InterBackground)

DEFINE_FIELD_X(SummaryInfo, SummaryInfo, kills)
DEFINE_FIELD_X(SummaryInfo, SummaryInfo, maxkills)
DEFINE_FIELD_X(SummaryInfo, SummaryInfo, secrets)
DEFINE_FIELD_X(SummaryInfo, SummaryInfo, maxsecrets)
DEFINE_FIELD_X(SummaryInfo, SummaryInfo, supersecrets)
DEFINE_FIELD_X(SummaryInfo, SummaryInfo, playercount)
DEFINE_FIELD_X(SummaryInfo, SummaryInfo, time)
DEFINE_FIELD_X(SummaryInfo, SummaryInfo, cheated)
DEFINE_FIELD_X(SummaryInfo, SummaryInfo, endofgame)


void InitBuildTiles()
{
	// need to find a better way to handle this thing.
}

