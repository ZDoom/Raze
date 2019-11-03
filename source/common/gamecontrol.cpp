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

InputState inputState;
void SetClipshapes();
int ShowStartupWindow(TArray<GrpEntry> &);
void InitFileSystem(TArray<GrpEntry>&);
int globalShadeDiv;

struct GameFuncNameDesc
{
	int index;
	const char *name;
};

static const GameFuncNameDesc gamefuncs[] = {
	{ gamefunc_Move_Forward, "Move_Forward"},
	{ gamefunc_Move_Backward, "Move_Backward"},
	{ gamefunc_Turn_Left, "Turn_Left"},
	{ gamefunc_Turn_Right, "Turn_Right"},
	{ gamefunc_Strafe, "Strafe"},
	{ gamefunc_Fire, "Fire"},
	{ gamefunc_Open, "Open"},
	{ gamefunc_Run, "Run"},
	{ gamefunc_Alt_Fire, "Alt_Fire"},
	{ gamefunc_Jump, "Jump"},
	{ gamefunc_Crouch, "Crouch"},
	{ gamefunc_Look_Up, "Look_Up"},
	{ gamefunc_Look_Down, "Look_Down"},
	{ gamefunc_Look_Left, "Look_Left"},
	{ gamefunc_Look_Right, "Look_Right"},
	{ gamefunc_Strafe_Left, "Strafe_Left"},
	{ gamefunc_Strafe_Right, "Strafe_Right"},
	{ gamefunc_Aim_Up, "Aim_Up"},
	{ gamefunc_Aim_Down, "Aim_Down"},
	{ gamefunc_Weapon_1, "Weapon_1"},
	{ gamefunc_Weapon_2, "Weapon_2"},
	{ gamefunc_Weapon_3, "Weapon_3"},
	{ gamefunc_Weapon_4, "Weapon_4"},
	{ gamefunc_Weapon_5, "Weapon_5"},
	{ gamefunc_Weapon_6, "Weapon_6"},
	{ gamefunc_Weapon_7, "Weapon_7"},
	{ gamefunc_Weapon_8, "Weapon_8"},
	{ gamefunc_Weapon_9, "Weapon_9"},
	{ gamefunc_Weapon_10, "Weapon_10"},
	{ gamefunc_Inventory, "Inventory"},
	{ gamefunc_Inventory_Left, "Inventory_Left"},
	{ gamefunc_Inventory_Right, "Inventory_Right"},
	{ gamefunc_Holo_Duke, "Holo_Duke"},
	{ gamefunc_Jetpack, "Jetpack"},
	{ gamefunc_NightVision, "NightVision"},
	{ gamefunc_MedKit, "MedKit"},
	{ gamefunc_TurnAround, "TurnAround"},
	{ gamefunc_SendMessage, "SendMessage"},
	{ gamefunc_Map, "Map"},
	{ gamefunc_Shrink_Screen, "Shrink_Screen"},
	{ gamefunc_Enlarge_Screen, "Enlarge_Screen"},
	{ gamefunc_Center_View, "Center_View"},
	{ gamefunc_Holster_Weapon, "Holster_Weapon"},
	{ gamefunc_Show_Opponents_Weapon, "Show_Opponents_Weapon"},
	{ gamefunc_Map_Follow_Mode, "Map_Follow_Mode"},
	{ gamefunc_See_Coop_View, "See_Coop_View"},
	{ gamefunc_Mouse_Aiming, "Mouse_Aiming"},
	{ gamefunc_Toggle_Crosshair, "Toggle_Crosshair"},
	{ gamefunc_Steroids, "Steroids"},
	{ gamefunc_Quick_Kick, "Quick_Kick"},
	{ gamefunc_Next_Weapon, "Next_Weapon"},
	{ gamefunc_Previous_Weapon, "Previous_Weapon"},
	{ gamefunc_Show_Console, "Show_Console"},
	{ gamefunc_Show_DukeMatch_Scores, "Show_DukeMatch_Scores"},
	{ gamefunc_Dpad_Select, "Dpad_Select"},
	{ gamefunc_Dpad_Aiming, "Dpad_Aiming"},
	{ gamefunc_AutoRun, "AutoRun"},
	{ gamefunc_Last_Weapon, "Last_Used_Weapon"},
	{ gamefunc_Quick_Save, "Quick_Save"},
	{ gamefunc_Quick_Load, "Quick_Load"},
	{ gamefunc_Alt_Weapon, "Alt_Weapon"},
	{ gamefunc_Third_Person_View, "Third_Person_View"},
	{ gamefunc_Toggle_Crouch, "Toggle_Crouch"},
	{ gamefunc_See_Chase_View, "See_Chase_View"},	// the following were added by Blood
	{ gamefunc_Turn_Around, "Turn_Around"},
	{ gamefunc_Weapon_Fire,	"Weapon_Fire"},
	{ gamefunc_Weapon_Special_Fire,	"Weapon_Special_Fire"},
	{ gamefunc_Aim_Center, "Aim_Center"},
	{ gamefunc_Tilt_Left, "Tilt_Left"},
	{ gamefunc_Tilt_Right, "Tilt_Right"},
	{ gamefunc_Send_Message, "Send_Message"},
	{ gamefunc_BeastVision, "BeastVision"},
	{ gamefunc_CrystalBall, "CrystalBall"},
	{ gamefunc_JumpBoots, "JumpBoots"},
	{ gamefunc_ProximityBombs, "ProximityBombs"},
	{ gamefunc_RemoteBombs, "RemoteBombs"},
	{ gamefunc_Smoke_Bomb, "Smoke_Bomb" },
	{ gamefunc_Gas_Bomb, "Gas_Bomb" },
	{ gamefunc_Flash_Bomb, "Flash_Bomb" },
	{ gamefunc_Caltrops, "Calitrops" },

};

FString currentGame;
FString LumpFilter;

static TMap<FName, int> GF_NameToNum;
static FString GF_NumToName[NUMGAMEFUNCTIONS];	// This one will preserve the original name for writing to the config (which must be loaded before CON scripts can hack around with the alias array.)
static FString GF_NumToAlias[NUMGAMEFUNCTIONS];	// This is for CON scripts to hack apart.

uint8_t KeyboardKeys[NUMGAMEFUNCTIONS][2];
static FString stringStore[2 * NUMGAMEFUNCTIONS];	// toss all persistent strings from the OSDCMDs in here so that they stick around until shutdown.

CVAR(Int, cl_defaultconfiguration, 2, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)


static int osdcmd_button(osdcmdptr_t parm)
{
	static char const s_gamefunc_[] = "gamefunc_";
	int constexpr strlen_gamefunc_ = ARRAY_SIZE(s_gamefunc_) - 1;

	char const* p = parm->name + strlen_gamefunc_;

	//if (gInputMode == kInputGame) // only trigger these if in game (fixme: Ensure it works for all games!)
	CONTROL_ButtonFlags[CONFIG_FunctionNameToNum(p)] = 1; // FIXME

	return OSDCMD_OK;
}

//==========================================================================
//
//
//
//==========================================================================

void SetupButtonFunctions()
{
	unsigned index = 0;
	for (auto& func : GF_NumToAlias)
	{
		if (func[0] == '\0')
			continue;

	}

}

UserConfig userConfig;

void UserConfig::ProcessOptions()
{
	// -help etc are omitted

	// -cfg / -setupfile refer to Build style config which are not supported.
	if (Args->CheckParm("-cfg") || Args->CheckParm("-setupfile"))
	{
		initprintf("Build-format config files not supported and will be ignored\n");
	}

	auto v = Args->CheckValue("-port");
	if (v) netPort = strtol(v, nullptr, 0);

	netServerMode = Args->CheckParm("-server");
	netServerAddress = Args->CheckValue("-connect");
	netPassword = Args->CheckValue("-password");

	v = Args->CheckValue("-addon");
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
	extern GameInterface Interface;
}
namespace Redneck
{
	extern GameInterface Interface;
}
namespace Blood
{
	extern GameInterface Interface;
}
namespace ShadowWarrior
{
	extern GameInterface Interface;
}

void CheckFrontend(int flags)
{
	if (flags & GAMEFLAG_BLOOD)
	{
		gi = &Blood::Interface;
		globalShadeDiv = 62;
	}
	else if (flags & GAMEFLAG_RR)
	{
		gi = &Redneck::Interface;
		globalShadeDiv = 30;
	}
	else if (flags & GAMEFLAG_FURY)
	{
		gi = &Duke::Interface;
		globalShadeDiv = 26;	// This is different from all other games which need a value two less than the amount of shades.
	}
	else if (flags & GAMEFLAG_SW)
	{
		gi = &ShadowWarrior::Interface;
		globalShadeDiv = 30;
	}
	else
	{
		gi = &Duke::Interface;
		globalShadeDiv = 30;
	}
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
	if (userConfig.gamegrp)
	{
		std::filesystem::path gpath = std::filesystem::u8path(userConfig.gamegrp.GetChars());

		int g = 0;
		for (auto& grp : groups)
		{
			std::filesystem::path fpath = std::filesystem::u8path(grp.FileName.GetChars());
			std::error_code err;
			if (std::filesystem::equivalent(gpath, fpath, err))
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

	int index = 0;
	for(auto &gf : gamefuncs)
	{
		GF_NameToNum.Insert(gf.name, gf.index);
		GF_NumToAlias[gf.index] = GF_NumToName[gf.index] = gf.name;

		stringStore[index].Format("gamefunc_%s", gf.name);
		stringStore[index].ToLower();
		stringStore[index + 1] = stringStore[index];
		stringStore[index + 1] += ": game button";
		OSD_RegisterFunction(stringStore[index], stringStore[index + 1], osdcmd_button);
		index += 2;

	}
	InitFileSystem(usedgroups);

	CONTROL_ClearAssignments();
	CONFIG_InitMouseAndController();
	CONFIG_SetGameControllerDefaultsStandard();
	CONFIG_SetDefaultKeys(cl_defaultconfiguration == 1 ? "demolition/origbinds.txt" : cl_defaultconfiguration == 2 ? "demolition/leftbinds.txt" : "demolition/defbinds.txt");
	SetupButtonFunctions();

	G_ReadConfig(currentGame);
	if (!GameConfig->IsInitialized())
	{
		CONFIG_ReadCombatMacros();
	}

	if (userConfig.CommandName.IsNotEmpty())
	{
		playername = userConfig.CommandName;
	}



	return gi->app_main();
}

//==========================================================================
//
//
//
//==========================================================================

int32_t CONFIG_FunctionNameToNum(const char *func)
{
    if (!func) return -1;
	
	FName name(func, true);
	if (name == NAME_None) return -1;
	
	auto res = GF_NameToNum.CheckKey(name);
	if (!res) return -1;
	
	return *res;
}

const char *CONFIG_FunctionNumToName(int32_t func)
{
    if ((unsigned)func >= (unsigned)NUMGAMEFUNCTIONS)
        return NULL;
    return GF_NumToAlias[func];
}

const char *CONFIG_FunctionNumToRealName(int32_t func)
{
    if ((unsigned)func >= (unsigned)NUMGAMEFUNCTIONS)
        return NULL;
    return GF_NumToName[func];
}

void CONFIG_ReplaceButtonName(int num, const char *text)
{
    if ((unsigned)num >= (unsigned)NUMGAMEFUNCTIONS)
        return;
	GF_NumToAlias[num] = text;
	GF_NameToNum.Insert(text, num);
}

void CONFIG_DeleteButtonName(int num)
{
    if ((unsigned)num >= (unsigned)NUMGAMEFUNCTIONS)
        return;
	GF_NumToAlias[num] = "";
}

//==========================================================================
//
// wrapper for CONTROL_MapKey(), generates key bindings to reflect changes to keyboard setup
//
//==========================================================================

void CONFIG_MapKey(int which, kb_scancode key1, kb_scancode oldkey1, kb_scancode key2, kb_scancode oldkey2)
{
	int const keys[] = { key1, key2, oldkey1, oldkey2 };

	if (which == gamefunc_Show_Console)
		OSD_CaptureKey(key1);

	for (int k = 0; (unsigned)k < ARRAY_SIZE(keys); k++)
	{
		if (keys[k] == 0xff || !keys[k])
			continue;

		int match = 0;

		for (; sctokeylut[match].key; match++)
		{
			if (keys[k] == sctokeylut[match].sc)
				break;
		}

		FString tempbuf;

		for (int i = NUMGAMEFUNCTIONS - 1; i >= 0; i--)
		{
			if (KeyboardKeys[i][0] == keys[k] || KeyboardKeys[i][1] == keys[k])
			{
				tempbuf.AppendFormat("gamefunc_%s; ", CONFIG_FunctionNumToName(i));
			}
		}

		auto len = tempbuf.Len();

		if (len >= 2)
		{
			tempbuf.Truncate(len - 2); // cut off the trailing "; "
			CONTROL_BindKey(keys[k], tempbuf, 1, sctokeylut[match].key ? sctokeylut[match].key : "<?>");
		}
		else
		{
			CONTROL_FreeKeyBind(keys[k]);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void CONFIG_SetDefaultKeys(const char *defbinds, bool lazy/*=false*/)
{
	FScanner sc;

	sc.Open(defbinds);

	if (!lazy)
	{
		memset(KeyboardKeys, 0xff, sizeof(KeyboardKeys));
		CONTROL_ClearAllBinds();
	}

	while (sc.GetToken())
	{
		sc.TokenMustBe(TK_StringConst);
		int num = CONFIG_FunctionNameToNum(sc.String);
		int default0 = -1;
		int default1 = -1;

		if (sc.CheckToken(','))
		{
			sc.MustGetToken(TK_StringConst);
			default0 = KB_StringToScanCode(sc.String);
			if (sc.CheckToken(','))
			{
				sc.MustGetToken(TK_StringConst);
				default1 = KB_StringToScanCode(sc.String);
			}
			if (num >= 0 && num < NUMGAMEFUNCTIONS)
			{
				auto& key = KeyboardKeys[num];
#if 0
				// skip the function if the default key is already used
				// or the function is assigned to another key
				if (lazy && (key[0] != 0xff || (CONTROL_KeyIsBound(default0) && Bstrlen(CONTROL_KeyBinds[default0].cmdstr) > strlen_gamefunc_
					&& CONFIG_FunctionNameToNum(CONTROL_KeyBinds[default0].cmdstr + strlen_gamefunc_) >= 0)))
				{
					continue;
				}
#endif
				key[0] = default0;
				key[1] = default1;

				if (key[0]) CONTROL_FreeKeyBind(key[0]);
				if (key[1])	CONTROL_FreeKeyBind(key[1]);

				if (num == gamefunc_Show_Console)
					OSD_CaptureKey(key[0]);
				else
					CONFIG_MapKey(num, key[0], 0, key[1], 0);

			}
		}

	}
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
	sc.Open("demolition/combatmacros.txt");
	for (auto s : CombatMacros)
	{
		sc.MustGetToken(TK_StringConst);
		if (strlen(*s) == 0)
			*s = sc.String;
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
		if (s) (int)strtoull(s, nullptr, 0);
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

int32_t MouseFunctions[MAXMOUSEBUTTONS][2];
int32_t MouseDigitalFunctions[MAXMOUSEAXES][2];
int32_t MouseAnalogueAxes[MAXMOUSEAXES];
int32_t MouseAnalogueScale[MAXMOUSEAXES];
int32_t JoystickFunctions[MAXJOYBUTTONSANDHATS][2];
int32_t JoystickDigitalFunctions[MAXJOYAXES][2];
int32_t JoystickAnalogueAxes[MAXJOYAXES];
int32_t JoystickAnalogueScale[MAXJOYAXES];
int32_t JoystickAnalogueDead[MAXJOYAXES];
int32_t JoystickAnalogueSaturate[MAXJOYAXES];
int32_t JoystickAnalogueInvert[MAXJOYAXES];

static const char* mousedefaults[MAXMOUSEBUTTONS] =
{
"Weapon_Fire",
"Weapon_Special_Fire",
"",
"",
"Previous_Weapon",
"Next_Weapon",
};


static const char* mouseclickeddefaults[MAXMOUSEBUTTONS] =
{
};


static const char* mouseanalogdefaults[MAXMOUSEAXES] =
{
"analog_turning",
"analog_moving",
};


static const char* mousedigitaldefaults[MAXMOUSEDIGITAL] =
{
};

static const char* joystickdefaults[MAXJOYBUTTONSANDHATS] =
{
"Fire",
"Strafe",
"Run",
"Open",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"Aim_Down",
"Look_Right",
"Aim_Up",
"Look_Left",
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


static const char* joystickdigitaldefaults[MAXJOYDIGITAL] =
{
"",
"",
"",
"",
"",
"",
"Run",
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
	const char* val;
	FString section = currentGame + ".MouseSettings";
	if (!GameConfig->SetSection(section)) return;

	for (int i = 0; i < MAXMOUSEBUTTONS; i++)
	{
		section.Format("MouseButton%d", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			MouseFunctions[i][0] = CONFIG_FunctionNameToNum(val);

		section.Format("MouseButtonClicked%d", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			MouseFunctions[i][1] = CONFIG_FunctionNameToNum(val);
	}

	// map over the axes
	for (int i = 0; i < MAXMOUSEAXES; i++)
	{
		section.Format("MouseAnalogAxes%d", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(val);

		section.Format("MouseDigitalAxes%d_0", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(val);

		section.Format("MouseDigitalAxes%d_1", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			MouseDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(val);

		section.Format("MouseAnalogScale%d", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			MouseAnalogueScale[i] = (int32_t)strtoull(val, nullptr, 0);
	}

	for (int i = 0; i < MAXMOUSEBUTTONS; i++)
	{
		CONTROL_MapButton(MouseFunctions[i][0], i, 0, controldevice_mouse);
		CONTROL_MapButton(MouseFunctions[i][1], i, 1, controldevice_mouse);
	}
	for (int i = 0; i < MAXMOUSEAXES; i++)
	{
		CONTROL_MapAnalogAxis(i, MouseAnalogueAxes[i], controldevice_mouse);
		CONTROL_MapDigitalAxis(i, MouseDigitalFunctions[i][0], 0, controldevice_mouse);
		CONTROL_MapDigitalAxis(i, MouseDigitalFunctions[i][1], 1, controldevice_mouse);
		CONTROL_SetAnalogAxisScale(i, MouseAnalogueScale[i], controldevice_mouse);
	}
	CONTROL_MouseEnabled    = (in_mouse && CONTROL_MousePresent);
}


void CONFIG_SetupJoystick(void)
{
	const char* val;
	FString section = currentGame + ".ControllerSettings";
	if (!GameConfig->SetSection(section)) return;

	for (int i = 0; i < MAXJOYBUTTONSANDHATS; i++)
	{
		section.Format("ControllerButton%d", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(val);

		section.Format("ControllerButtonClicked%d", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(val);
	}

	// map over the axes
	for (int i = 0; i < MAXJOYAXES; i++)
	{
		section.Format("ControllerAnalogAxes%d", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			JoystickAnalogueAxes[i] = CONFIG_AnalogNameToNum(val);

		section.Format("ControllerDigitalAxes%d_0", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			JoystickDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(val);

		section.Format("ControllerDigitalAxes%d_1", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(val);

		section.Format("ControllerAnalogScale%d", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			JoystickAnalogueScale[i] = (int32_t)strtoull(val, nullptr, 0);

		section.Format("ControllerAnalogInvert%d", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			JoystickAnalogueInvert[i] = (int32_t)strtoull(val, nullptr, 0);

		section.Format("ControllerAnalogDead%d", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			JoystickAnalogueDead[i] = (int32_t)strtoull(val, nullptr, 0);

		section.Format("ControllerAnalogSaturate%d", i);
		val = GameConfig->GetValueForKey(section);
		if (val)
			JoystickAnalogueSaturate[i] = (int32_t)strtoull(val, nullptr, 0);
	}

	for (int i = 0; i < MAXJOYBUTTONSANDHATS; i++)
	{
		CONTROL_MapButton(JoystickFunctions[i][0], i, 0, controldevice_joystick);
		CONTROL_MapButton(JoystickFunctions[i][1], i, 1, controldevice_joystick);
	}
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
	CONTROL_MapButton(function, i, j, controldevice_joystick);
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

void CONFIG_SetGameControllerDefaultsStandard()
{
	CONFIG_SetGameControllerDefaultsClear();
	CONFIG_SetGameControllerAxesModern();

	static GameControllerButtonSetting const buttons[] =
	{
		{ GAMECONTROLLER_BUTTON_A, gamefunc_Jump },
		{ GAMECONTROLLER_BUTTON_B, gamefunc_Toggle_Crouch },
		{ GAMECONTROLLER_BUTTON_BACK, gamefunc_Map },
		{ GAMECONTROLLER_BUTTON_LEFTSTICK, gamefunc_Run },
		{ GAMECONTROLLER_BUTTON_RIGHTSTICK, gamefunc_Quick_Kick },
		{ GAMECONTROLLER_BUTTON_LEFTSHOULDER, gamefunc_Crouch },
		{ GAMECONTROLLER_BUTTON_RIGHTSHOULDER, gamefunc_Jump },
		{ GAMECONTROLLER_BUTTON_DPAD_UP, gamefunc_Previous_Weapon },
		{ GAMECONTROLLER_BUTTON_DPAD_DOWN, gamefunc_Next_Weapon },
	};

	static GameControllerButtonSetting const buttonsDuke[] =
	{
		{ GAMECONTROLLER_BUTTON_X, gamefunc_Open },
		{ GAMECONTROLLER_BUTTON_Y, gamefunc_Inventory },
		{ GAMECONTROLLER_BUTTON_DPAD_LEFT, gamefunc_Inventory_Left },
		{ GAMECONTROLLER_BUTTON_DPAD_RIGHT, gamefunc_Inventory_Right },
	};

	static GameControllerButtonSetting const buttonsFury[] =
	{
		{ GAMECONTROLLER_BUTTON_X, gamefunc_Steroids }, // Reload
		{ GAMECONTROLLER_BUTTON_Y, gamefunc_Open },
		{ GAMECONTROLLER_BUTTON_DPAD_LEFT, gamefunc_MedKit },
		{ GAMECONTROLLER_BUTTON_DPAD_RIGHT, gamefunc_NightVision }, // Radar
	};

	static GameControllerDigitalAxisSetting const digitalAxes[] =
	{
		{ GAMECONTROLLER_AXIS_TRIGGERLEFT, 1, gamefunc_Alt_Fire },
		{ GAMECONTROLLER_AXIS_TRIGGERRIGHT, 1, gamefunc_Fire },
	};

	for (auto const& button : buttons)
		button.apply();

	/*
	if (FURY)
	{
		for (auto const& button : buttonsFury)
			button.apply();
	}
	else
	*/
	{
		for (auto const& button : buttonsDuke)
			button.apply();
	}

	for (auto const& digitalAxis : digitalAxes)
		digitalAxis.apply();
}

void CONFIG_SetGameControllerDefaultsPro()
{
	CONFIG_SetGameControllerDefaultsClear();
	CONFIG_SetGameControllerAxesModern();

	static GameControllerButtonSetting const buttons[] =
	{
		{ GAMECONTROLLER_BUTTON_A, gamefunc_Open },
		{ GAMECONTROLLER_BUTTON_B, gamefunc_Third_Person_View },
		{ GAMECONTROLLER_BUTTON_Y, gamefunc_Quick_Kick },
		{ GAMECONTROLLER_BUTTON_BACK, gamefunc_Map },
		{ GAMECONTROLLER_BUTTON_LEFTSTICK, gamefunc_Run },
		{ GAMECONTROLLER_BUTTON_RIGHTSTICK, gamefunc_Crouch },
		{ GAMECONTROLLER_BUTTON_DPAD_UP, gamefunc_Previous_Weapon },
		{ GAMECONTROLLER_BUTTON_DPAD_DOWN, gamefunc_Next_Weapon },
	};

	static GameControllerButtonSetting const buttonsDuke[] =
	{
		{ GAMECONTROLLER_BUTTON_X, gamefunc_Inventory },
		{ GAMECONTROLLER_BUTTON_LEFTSHOULDER, gamefunc_Previous_Weapon },
		{ GAMECONTROLLER_BUTTON_RIGHTSHOULDER, gamefunc_Next_Weapon },
		{ GAMECONTROLLER_BUTTON_DPAD_LEFT, gamefunc_Inventory_Left },
		{ GAMECONTROLLER_BUTTON_DPAD_RIGHT, gamefunc_Inventory_Right },
	};

	static GameControllerButtonSetting const buttonsFury[] =
	{
		{ GAMECONTROLLER_BUTTON_X, gamefunc_Steroids }, // Reload
		{ GAMECONTROLLER_BUTTON_LEFTSHOULDER, gamefunc_Crouch },
		{ GAMECONTROLLER_BUTTON_RIGHTSHOULDER, gamefunc_Alt_Fire },
		{ GAMECONTROLLER_BUTTON_DPAD_LEFT, gamefunc_MedKit },
		{ GAMECONTROLLER_BUTTON_DPAD_RIGHT, gamefunc_NightVision }, // Radar
	};

	static GameControllerDigitalAxisSetting const digitalAxes[] =
	{
		{ GAMECONTROLLER_AXIS_TRIGGERLEFT, 1, gamefunc_Jump },
		{ GAMECONTROLLER_AXIS_TRIGGERRIGHT, 1, gamefunc_Fire },
	};

	for (auto const& button : buttons)
		button.apply();

#if 0 // ouch...
	if (FURY)
	{
		for (auto const& button : buttonsFury)
			button.apply();
	}
	else
#endif
	{
		for (auto const& button : buttonsDuke)
			button.apply();
	}

	for (auto const& digitalAxis : digitalAxes)
		digitalAxis.apply();
}

char const* CONFIG_GetGameFuncOnKeyboard(int gameFunc)
{
	const char* string0 = KB_ScanCodeToString(KeyboardKeys[gameFunc][0]);
	return string0[0] == '\0' ? KB_ScanCodeToString(KeyboardKeys[gameFunc][1]) : string0;
}

char const* CONFIG_GetGameFuncOnMouse(int gameFunc)
{
	for (int j = 0; j < 2; ++j)
		for (int i = 0; i < joystick.numButtons; ++i)
			if (JoystickFunctions[i][j] == gameFunc)
				return joyGetName(1, i);

	for (int i = 0; i < joystick.numAxes; ++i)
		for (int j = 0; j < 2; ++j)
			if (JoystickDigitalFunctions[i][j] == gameFunc)
				return joyGetName(0, i);

	return "";
}

char const* CONFIG_GetGameFuncOnJoystick(int gameFunc)
{
	for (int j = 0; j < 2; ++j)
		for (int i = 0; i < joystick.numButtons; ++i)
			if (JoystickFunctions[i][j] == gameFunc)
				return joyGetName(1, i);

	for (int i = 0; i < joystick.numAxes; ++i)
		for (int j = 0; j < 2; ++j)
			if (JoystickDigitalFunctions[i][j] == gameFunc)
				return joyGetName(0, i);

	return "";
}

// FIXME: Consider the mouse as well!
FString CONFIG_GetBoundKeyForLastInput(int gameFunc)
{
	if (CONTROL_LastSeenInput == LastSeenInput::Joystick)
	{
		char const* joyname = CONFIG_GetGameFuncOnJoystick(gameFunc);
		if (joyname != nullptr && joyname[0] != '\0')
		{
			return joyname;
		}

		char const* keyname = CONFIG_GetGameFuncOnKeyboard(gameFunc);
		if (keyname != nullptr && keyname[0] != '\0')
		{
			return keyname;
		}
	}
	else
	{
		char const* keyname = CONFIG_GetGameFuncOnKeyboard(gameFunc);
		if (keyname != nullptr && keyname[0] != '\0')
		{
			return keyname;
		}

		char const* joyname = CONFIG_GetGameFuncOnJoystick(gameFunc);
		if (joyname != nullptr && joyname[0] != '\0')
		{
			return joyname;
		}
	}
	return "UNBOUND";
}


void CONFIG_InitMouseAndController()
{
	memset(MouseFunctions, -1, sizeof(MouseFunctions));
	memset(MouseDigitalFunctions, -1, sizeof(MouseDigitalFunctions));
	memset(JoystickFunctions, -1, sizeof(JoystickFunctions));
	memset(JoystickDigitalFunctions, -1, sizeof(JoystickDigitalFunctions));

	for (int i = 0; i < MAXMOUSEBUTTONS; i++)
	{
		MouseFunctions[i][0] = CONFIG_FunctionNameToNum(mousedefaults[i]);
		CONTROL_MapButton(MouseFunctions[i][0], i, 0, controldevice_mouse);
		if (i >= 4) continue;
		MouseFunctions[i][1] = CONFIG_FunctionNameToNum(mouseclickeddefaults[i]);
		CONTROL_MapButton(MouseFunctions[i][1], i, 1, controldevice_mouse);
	}

	for (int i = 0; i < MAXMOUSEAXES; i++)
	{
		MouseAnalogueScale[i] = DEFAULTMOUSEANALOGUESCALE;
		CONTROL_SetAnalogAxisScale(i, MouseAnalogueScale[i], controldevice_mouse);

		MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i * 2]);
		MouseDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i * 2 + 1]);
		CONTROL_MapDigitalAxis(i, MouseDigitalFunctions[i][0], 0, controldevice_mouse);
		CONTROL_MapDigitalAxis(i, MouseDigitalFunctions[i][1], 1, controldevice_mouse);

		MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(mouseanalogdefaults[i]);
		CONTROL_MapAnalogAxis(i, MouseAnalogueAxes[i], controldevice_mouse);
	}
	CONFIG_SetupMouse();
	CONFIG_SetupJoystick();
	KB_ClearKeysDown();
	KB_FlushKeyboardQueue();
	KB_FlushKeyboardQueueScans();
}


void CONFIG_PutNumber(const char* key, int number)
{
	FStringf str("%d", number);
	GameConfig->SetValueForKey(key, str);
}

void CONFIG_WriteControllerSettings()
{
	FString buf;
	if (in_mouse)
	{
		FString section = currentGame + ".MouseSettings";
		GameConfig->SetSection(section);
		for (int i = 0; i < MAXMOUSEBUTTONS; i++)
		{
			if (CONFIG_FunctionNumToName(MouseFunctions[i][0]))
			{
				buf.Format("MouseButton%d", i);
				GameConfig->SetValueForKey(buf, CONFIG_FunctionNumToName(MouseFunctions[i][0]));
			}

			if (i >= (MAXMOUSEBUTTONS - 2)) continue;

			if (CONFIG_FunctionNumToName(MouseFunctions[i][1]))
			{
				buf.Format("MouseButtonClicked%d", i);
				GameConfig->SetValueForKey(buf, CONFIG_FunctionNumToName(MouseFunctions[i][1]));
			}
		}

		for (int i = 0; i < MAXMOUSEAXES; i++)
		{
			if (CONFIG_AnalogNumToName(MouseAnalogueAxes[i]))
			{
				buf.Format("MouseAnalogAxes%d", i);
				GameConfig->SetValueForKey(buf, CONFIG_AnalogNumToName(MouseAnalogueAxes[i]));
			}

			if (CONFIG_FunctionNumToName(MouseDigitalFunctions[i][0]))
			{
				buf.Format("MouseDigitalAxes%d_0", i);
				GameConfig->SetValueForKey(buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[i][0]));
			}

			if (CONFIG_FunctionNumToName(MouseDigitalFunctions[i][1]))
			{
				buf.Format("MouseDigitalAxes%d_1", i);
				GameConfig->SetValueForKey(buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[i][1]));
			}

			buf.Format("MouseAnalogScale%d", i);
			CONFIG_PutNumber(buf, MouseAnalogueScale[i]);
		}
	}

	if (in_joystick)
	{
		FString section = currentGame + ".ControllerSettings";
		GameConfig->SetSection(section);
		for (int dummy = 0; dummy < MAXJOYBUTTONSANDHATS; dummy++)
		{
			if (CONFIG_FunctionNumToName(JoystickFunctions[dummy][0]))
			{
				buf.Format("ControllerButton%d", dummy);
				GameConfig->SetValueForKey(buf, CONFIG_FunctionNumToName(JoystickFunctions[dummy][0]));
			}

			if (CONFIG_FunctionNumToName(JoystickFunctions[dummy][1]))
			{
				buf.Format("ControllerButtonClicked%d", dummy);
				GameConfig->SetValueForKey(buf, CONFIG_FunctionNumToName(JoystickFunctions[dummy][1]));
			}
		}
		for (int dummy = 0; dummy < MAXJOYAXES; dummy++)
		{
			if (CONFIG_AnalogNumToName(JoystickAnalogueAxes[dummy]))
			{
				buf.Format("ControllerAnalogAxes%d", dummy);
				GameConfig->SetValueForKey(buf, CONFIG_AnalogNumToName(JoystickAnalogueAxes[dummy]));
			}

			if (CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][0]))
			{
				buf.Format("ControllerDigitalAxes%d_0", dummy);
				GameConfig->SetValueForKey(buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][0]));
			}

			if (CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][1]))
			{
				buf.Format("ControllerDigitalAxes%d_1", dummy);
				GameConfig->SetValueForKey(buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][1]));
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
