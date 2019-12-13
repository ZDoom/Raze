#include <filesystem>
#include "gamecontrol.h"
#include "tarray.h"
#include "zstring.h"
#include "name.h"
#include "control.h"
#include "keyboard.h"
#include "sc_man.h"
#include "c_cvars.h"
#include "gameconfigfile.h"
#include "gamecvars.h"
#include "build.h"
#include "inputstate.h"
#include "_control.h"
#include "control.h"
#include "m_argv.h"
#include "rts.h"
#include "printf.h"
#include "c_bind.h"
#include "v_font.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "i_specialpaths.h"
#include "z_music.h"
#include "statistics.h"
#include "menu.h"
#include "gstrings.h"
#include "quotemgr.h"
#include "mapinfo.h"
#ifndef NETCODE_DISABLE
#include "enet.h"
#endif

MapRecord mapList[512];		// Due to how this gets used it needs to be static. EDuke defines 7 episode plus one spare episode with 64 potential levels each and relies on the static array which is freely accessible by scripts.
MapRecord *currentLevel;	// level that is currently played. (The real level, not what script hacks modfifying the current level index can pretend.)
MapRecord* lastLevel;		// Same here, for the last level.
MapRecord userMapRecord;	// stand-in for the user map.

void C_CON_SetAliases();
InputState inputState;
void SetClipshapes();
int ShowStartupWindow(TArray<GrpEntry> &);
void InitFileSystem(TArray<GrpEntry>&);
bool gHaveNetworking;


FString currentGame;
FString LumpFilter;
TMap<FName, int32_t> NameToTileIndex; // for assigning names to tiles. The menu accesses this list. By default it gets everything from the dynamic tile map in Duke Nukem and Redneck Rampage.
										// Todo: Add additional definition file for the other games or textures not in that list so that the menu does not have to rely on indices.

CVAR(Int, cl_defaultconfiguration, 2, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)



UserConfig userConfig;

void UserConfig::ProcessOptions()
{
	// -help etc are omitted

	// -cfg / -setupfile refer to Build style config which are not supported.
	if (Args->CheckParm("-cfg") || Args->CheckParm("-setupfile"))
	{
		initprintf("Build-format config files not supported and will be ignored\n");
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
		if (val > 0 && val < 4) gamegrp = addons[val];
		else initprintf("%s: Unknown Addon\n", v);
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

	static const char* cons[] = { "-con", "-x", nullptr };
	Args->CollectFiles("-con", cons, ".con");
	DefaultCon = Args->CheckValue("-con");

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

	CommandIni = Args->CheckValue("-ini");

	nologo = Args->CheckParm("-nologo") || Args->CheckParm("-quick");
	nomusic = Args->CheckParm("-nomusic");
	nosound = Args->CheckParm("-nosfx");
	if (Args->CheckParm("-nosound")) nomusic = nosound = true;
	if (Args->CheckParm("-setup")) setupstate = 1;
	else if (Args->CheckParm("-nosetup")) setupstate = 0;


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

void CheckFrontend(int flags)
{
	if (flags & GAMEFLAG_BLOOD)
	{
		gi = Blood::CreateInterface();
	}
	else if (flags & GAMEFLAG_RR)
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
	else
	{
		gi = Duke::CreateInterface();
	}
}


int GameMain()
{
	// Set up the console before anything else so that it can receive text.
	C_InitConsole(1024, 768, true);
	FStringf logpath("logfile %sdemolition.log", M_GetDocumentsPath().GetChars());
	C_DoCommand(logpath);

#ifndef NETCODE_DISABLE
	gHaveNetworking = !enet_initialize();
	if (!gHaveNetworking)
		initprintf("An error occurred while initializing ENet.\n");
#endif

	int r;
	try
	{
		r = CONFIG_Init();
	}
	catch (const std::runtime_error & err)
	{
		wm_msgbox("Error", "%s", err.what());
		return 3;
	}
	catch (const ExitEvent & exit)
	{
		// Just let the rest of the function execute.
		r = exit.Reason();
	}
	G_SaveConfig();
#ifndef NETCODE_DISABLE
	if (gHaveNetworking) enet_deinitialize();
#endif
	return r;
}

//==========================================================================
//
//
//
//==========================================================================

#define LOCALIZED_STRING(s) "$" s

void SetDefaultStrings()
{
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
	#if 0 // todo: print a message
			if (gAutoRun)
				viewSetMessage("Auto run ON");
			else
				viewSetMessage("Auto run OFF");

	#endif
}

//==========================================================================
//
//
//
//==========================================================================

int CONFIG_Init()
{
	SetClipshapes();

	// This must be done before initializing any data, so doing it late in the startup process won't work.
	if (CONTROL_Startup(controltype_keyboardandmouse, BGetTime, 120))
	{
		return 1;
	}

	userConfig.ProcessOptions();

	G_LoadConfig();

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
	if (groupno == -1 || userConfig.setupstate == 1)
		groupno = ShowStartupWindow(groups);

	if (groupno == -1) return 0;
	auto &group = groups[groupno];

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

	InitFileSystem(usedgroups);
	TArray<FString> addArt;
	for (auto& grp : usedgroups)
	{
		for (auto& art : grp.FileInfo.loadart)
		{
			addArt.Push(art);
		}
	}
	TileFiles.AddArt(addArt);

	CONTROL_ClearAssignments();
	CONFIG_InitMouseAndController();
	CONFIG_SetDefaultKeys(cl_defaultconfiguration == 1 ? "demolition/origbinds.txt" : cl_defaultconfiguration == 2 ? "demolition/leftbinds.txt" : "demolition/defbinds.txt");
	
	G_ReadConfig(currentGame);
	if (!GameConfig->IsInitialized())
	{
		CONFIG_ReadCombatMacros();
	}

	if (userConfig.CommandName.IsNotEmpty())
	{
		playername = userConfig.CommandName;
	}
	GStrings.LoadStrings();
	V_InitFonts();
	C_CON_SetAliases();
	Mus_Init();
	InitStatistics();
	M_Init();
	SetDefaultStrings();
	if (g_gameType & GAMEFLAG_RR) InitRREndMap();	// this needs to be done better later
	return gi->app_main();
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
		sc.Open("demolition/combatmacros.txt");
		for (auto s : CombatMacros)
		{
			sc.MustGetToken(TK_StringConst);
			if (strlen(*s) == 0)
				*s = sc.String;
		}
	}
	catch (const std::runtime_error &)
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
//==========================================================================
//
// 
//
//==========================================================================

int32_t MouseAnalogueAxes[MAXMOUSEAXES];
int32_t JoystickFunctions[MAXJOYBUTTONSANDHATS][2];
int32_t JoystickDigitalFunctions[MAXJOYAXES][2];
int32_t JoystickAnalogueAxes[MAXJOYAXES];
int32_t JoystickAnalogueScale[MAXJOYAXES];
int32_t JoystickAnalogueDead[MAXJOYAXES];
int32_t JoystickAnalogueSaturate[MAXJOYAXES];
int32_t JoystickAnalogueInvert[MAXJOYAXES];

static const char* mouseanalogdefaults[MAXMOUSEAXES] =
{
"analog_turning",
"analog_moving",
};


static const char* joystickclickeddefaults[MAXJOYBUTTONSANDHATS] =
{
"",
"Inventory",
"Jump",
"Crouch",
};


static const char* joystickanalogdefaults[MAXJOYAXES] =
{
"analog_turning",
"analog_moving",
"analog_strafing",
};


//==========================================================================
//
//
//
//==========================================================================

int32_t CONFIG_AnalogNameToNum(const char* func)
{
	if (!func)
		return -1;

	if (!Bstrcasecmp(func, "analog_turning"))
	{
		return analog_turning;
	}
	if (!Bstrcasecmp(func, "analog_strafing"))
	{
		return analog_strafing;
	}
	if (!Bstrcasecmp(func, "analog_moving"))
	{
		return analog_moving;
	}
	if (!Bstrcasecmp(func, "analog_lookingupanddown"))
	{
		return analog_lookingupanddown;
	}

	return -1;
}


//==========================================================================
//
//
//
//==========================================================================

const char* CONFIG_AnalogNumToName(int32_t func)
{
	switch (func)
	{
	case analog_turning:
		return "analog_turning";
	case analog_strafing:
		return "analog_strafing";
	case analog_moving:
		return "analog_moving";
	case analog_lookingupanddown:
		return "analog_lookingupanddown";
	}

	return NULL;
}

void CONFIG_SetupMouse(void)
{
	CONTROL_MouseEnabled    = (in_mouse && CONTROL_MousePresent);
}


void CONFIG_SetupJoystick(void)
{
	for (int i = 0; i < MAXJOYAXES; i++)
	{
		CONTROL_MapAnalogAxis(i, JoystickAnalogueAxes[i], controldevice_joystick);
		CONTROL_MapDigitalAxis(i, JoystickDigitalFunctions[i][0], 0, controldevice_joystick);
		CONTROL_MapDigitalAxis(i, JoystickDigitalFunctions[i][1], 1, controldevice_joystick);
		CONTROL_SetAnalogAxisScale(i, JoystickAnalogueScale[i], controldevice_joystick);
		CONTROL_SetAnalogAxisInvert(i, JoystickAnalogueInvert[i], controldevice_joystick);
	}
	
	CONTROL_JoystickEnabled = (in_joystick && CONTROL_JoyPresent);

	// JBF 20040215: evil and nasty place to do this, but joysticks are evil and nasty too
	for (int i=0; i<joystick.numAxes; i++)
		joySetDeadZone(i,JoystickAnalogueDead[i],JoystickAnalogueSaturate[i]);

}

static void CONFIG_SetJoystickButtonFunction(int i, int j, int function)
{
	JoystickFunctions[i][j] = function;
	//CONTROL_MapButton(function, i, j, controldevice_joystick);
}
static void CONFIG_SetJoystickAnalogAxisScale(int i, int scale)
{
	JoystickAnalogueScale[i] = scale;
	CONTROL_SetAnalogAxisScale(i, scale, controldevice_joystick);
}
static void CONFIG_SetJoystickAnalogAxisInvert(int i, int invert)
{
	JoystickAnalogueInvert[i] = invert;
	CONTROL_SetAnalogAxisInvert(i, invert, controldevice_joystick);
}
static void CONFIG_SetJoystickAnalogAxisDeadSaturate(int i, int dead, int saturate)
{
	JoystickAnalogueDead[i] = dead;
	JoystickAnalogueSaturate[i] = saturate;
	joySetDeadZone(i, dead, saturate);
}
static void CONFIG_SetJoystickDigitalAxisFunction(int i, int j, int function)
{
	JoystickDigitalFunctions[i][j] = function;
	CONTROL_MapDigitalAxis(i, function, j, controldevice_joystick);
}
static void CONFIG_SetJoystickAnalogAxisFunction(int i, int function)
{
	JoystickAnalogueAxes[i] = function;
	CONTROL_MapAnalogAxis(i, function, controldevice_joystick);
}

struct GameControllerButtonSetting
{
	GameControllerButton button;
	int function;

	void apply() const
	{
		CONFIG_SetJoystickButtonFunction(button, 0, function);
	}
};
struct GameControllerAnalogAxisSetting
{
	GameControllerAxis axis;
	int function;

	void apply() const
	{
		CONFIG_SetJoystickAnalogAxisFunction(axis, function);
	}
};
struct GameControllerDigitalAxisSetting
{
	GameControllerAxis axis;
	int polarity;
	int function;

	void apply() const
	{
		CONFIG_SetJoystickDigitalAxisFunction(axis, polarity, function);
	}
};


void CONFIG_SetGameControllerDefaultsClear()
{
	for (int i = 0; i < MAXJOYBUTTONSANDHATS; i++)
	{
		CONFIG_SetJoystickButtonFunction(i, 0, -1);
		CONFIG_SetJoystickButtonFunction(i, 1, -1);
	}

	for (int i = 0; i < MAXJOYAXES; i++)
	{
		CONFIG_SetJoystickAnalogAxisScale(i, DEFAULTJOYSTICKANALOGUESCALE);
		CONFIG_SetJoystickAnalogAxisInvert(i, 0);
		CONFIG_SetJoystickAnalogAxisDeadSaturate(i, DEFAULTJOYSTICKANALOGUEDEAD, DEFAULTJOYSTICKANALOGUESATURATE);

		CONFIG_SetJoystickDigitalAxisFunction(i, 0, -1);
		CONFIG_SetJoystickDigitalAxisFunction(i, 1, -1);

		CONFIG_SetJoystickAnalogAxisFunction(i, -1);
	}
}

static void CONFIG_SetGameControllerAxesModern()
{
	static GameControllerAnalogAxisSetting const analogAxes[] =
	{
		{ GAMECONTROLLER_AXIS_LEFTX, analog_strafing },
		{ GAMECONTROLLER_AXIS_LEFTY, analog_moving },
		{ GAMECONTROLLER_AXIS_RIGHTX, analog_turning },
		{ GAMECONTROLLER_AXIS_RIGHTY, analog_lookingupanddown },
	};

	CONFIG_SetJoystickAnalogAxisScale(GAMECONTROLLER_AXIS_RIGHTX, 32768 + 16384);
	CONFIG_SetJoystickAnalogAxisScale(GAMECONTROLLER_AXIS_RIGHTY, 32768 + 16384);

	for (auto const& analogAxis : analogAxes)
		analogAxis.apply();
}



void CONFIG_InitMouseAndController()
{
	memset(JoystickFunctions, -1, sizeof(JoystickFunctions));
	memset(JoystickDigitalFunctions, -1, sizeof(JoystickDigitalFunctions));

	for (int i = 0; i < MAXMOUSEAXES; i++)
	{
		MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(mouseanalogdefaults[i]);
		CONTROL_MapAnalogAxis(i, MouseAnalogueAxes[i], controldevice_mouse);
	}
	CONFIG_SetupMouse();
	CONFIG_SetupJoystick();
	inputState.ClearKeysDown();
	inputState.keyFlushChars();
	inputState.keyFlushScans();
}


void CONFIG_PutNumber(const char* key, int number)
{
	FStringf str("%d", number);
	GameConfig->SetValueForKey(key, str);
}

void CONFIG_WriteControllerSettings()
{
	FString buf;


	if (in_joystick)
	{
		FString section = currentGame + ".ControllerSettings";
		GameConfig->SetSection(section);
		for (int dummy = 0; dummy < MAXJOYAXES; dummy++)
		{
			if (CONFIG_AnalogNumToName(JoystickAnalogueAxes[dummy]))
			{
				buf.Format("ControllerAnalogAxes%d", dummy);
				GameConfig->SetValueForKey(buf, CONFIG_AnalogNumToName(JoystickAnalogueAxes[dummy]));
			}

			if (buttonMap.GetButtonName(JoystickDigitalFunctions[dummy][1]))
			{
				buf.Format("ControllerDigitalAxes%d_1", dummy);
				GameConfig->SetValueForKey(buf, buttonMap.GetButtonName(JoystickDigitalFunctions[dummy][1]));
			}

			buf.Format("ControllerAnalogScale%d", dummy);
			CONFIG_PutNumber(buf, JoystickAnalogueScale[dummy]);

			buf.Format("ControllerAnalogInvert%d", dummy);
			CONFIG_PutNumber(buf, JoystickAnalogueInvert[dummy]);

			buf.Format("ControllerAnalogDead%d", dummy);
			CONFIG_PutNumber(buf, JoystickAnalogueDead[dummy]);

			buf.Format("ControllerAnalogSaturate%d", dummy);
			CONFIG_PutNumber(buf, JoystickAnalogueSaturate[dummy]);
		}
	}
}
