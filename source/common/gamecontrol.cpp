#include "gamecontrol.h"
#include "tarray.h"
#include "zstring.h"
#include "name.h"
#include "control.h"
#include "keyboard.h"
#include "sc_man.h"
#include "c_cvars.h"
#include "gameconfigfile.h"
#include "build.h"

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

extern FString currentGame;

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

//==========================================================================
//
//
//
//==========================================================================

void CONFIG_Init()
{
	// This must be done before initializing any data, so doing it late in the startup process won't work.
	if (CONTROL_Startup(controltype_keyboardandmouse, BGetTime, gi->TicRate))
	{
		exit(1);
	}

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
	SetupButtonFunctions();
	CONTROL_ClearAssignments();
	CONFIG_SetDefaultKeys(cl_defaultconfiguration == 1 ? "demolition/origbinds.txt" : cl_defaultconfiguration == 2 ? "demolition/leftbinds.txt" : "demolition/defbinds.txt");
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
			CONTROL_DefineFlag(num, false);
		}

	}
}

