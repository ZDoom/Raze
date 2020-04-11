/*
** c_con.cpp
** Interface for CON scripts.
**
**
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
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
*/

#include <stdlib.h>
#include "basics.h"
#include "zstring.h"
#include "c_bind.h"
#include "gamecontrol.h"
#include "cmdlib.h"

//=============================================================================
//
// Interface for CON scripts to get descriptions for actions
// This code can query the 64 original game functions by index.
//
// This is mostly an ill-advised hackery to change menu texts. It no longer
// changes the menu but gets used when scripts try to write
// key related messages. Ion Fury uses this and is one of the reasons
// why that game is by all accounts not localizable.
//
// Important note: The list of actions may not be altered. If some of these
// commands get refactored, the string below needs to be changed to match
// the new command.
//
//=============================================================================

static GameFuncDesc con_gamefuncs[] = {
{"+Move_Forward",			"Move_Forward"},
{"+Move_Backward",			"Move_Backward"},
{"+Turn_Left",				"Turn_Left"},
{"+Turn_Right",				"Turn_Right"},
{"+Strafe",					"Strafe"},
{"+Fire",					"Fire"},
{"+Open",					"Open"},
{"+Run",					"Run"},
{"+Alt_Fire",				"Alt_Fire"},
{"+Jump",					"Jump"},
{"+Crouch",					"Crouch"},
{"+Look_Up",				"Look_Up"},
{"+Look_Down",				"Look_Down"},
{"+Look_Left",				"Look_Left"},
{"+Look_Right",				"Look_Right"},
{"+Strafe_Left",			"Strafe_Left"},
{"+Strafe_Right",			"Strafe_Right"},
{"+Aim_Up",					"Aim_Up"},
{"+Aim_Down",				"Aim_Down"},
{"+Weapon_1",				"Weapon_1"},
{"+Weapon_2",				"Weapon_2"},
{"+Weapon_3",				"Weapon_3"},
{"+Weapon_4",				"Weapon_4"},
{"+Weapon_5",				"Weapon_5"},
{"+Weapon_6",				"Weapon_6"},
{"+Weapon_7",				"Weapon_7"},
{"+Weapon_8",				"Weapon_8"},
{"+Weapon_9",				"Weapon_9"},
{"+Weapon_10",				"Weapon_10"},
{"+Inventory",				"Inventory"},
{"+Inventory_Left",			"Inventory_Left"},
{"+Inventory_Right",		"Inventory_Right"},
{"+Holo_Duke",				"Holo_Duke"},
{"+Jetpack",				"Jetpack"},
{"+NightVision",			"NightVision"},
{"+MedKit",					"MedKit"},
{"+TurnAround",				"TurnAround"},
{"+SendMessage",			"SendMessage"},
{"+Map",					"Map"},
{"+Shrink_Screen",			"Shrink_Screen"},
{"+Enlarge_Screen",			"Enlarge_Screen"},
{"+Center_View",			"Center_View"},
{"+Holster_Weapon",			"Holster_Weapon"},
{"+Show_Opponents_Weapon",	"Show_Opponents_Weapon"},
{"+Map_Follow_Mode",		"Map_Follow_Mode"},
{"+See_Coop_View",			"See_Coop_View"},
{"+Mouse_Aiming",			"Mouse_Aiming"},
{"+Toggle_Crosshair",		"Toggle_Crosshair"},
{"+Steroids",				"Steroids"},
{"+Quick_Kick",				"Quick_Kick"},
{"+Next_Weapon",			"Next_Weapon"},
{"+Previous_Weapon",		"Previous_Weapon"},
{"ToggleConsole",			"Show_Console"},
{"+Show_DukeMatch_Scores",	"Show_Scoreboard"},
{"+Dpad_Select",			"Dpad_Select"},
{"+Dpad_Aiming",			"Dpad_Aiming"},
{"toggle cl_autorun",				"AutoRun"},
{"+Last_Used_Weapon",		"Last_Used_Weapon"},
{"QuickSave",				"Quick_Save"},
{"QuickLoad",				"Quick_Load"},
{"+Alt_Weapon",				"Alt_Weapon"},
{"+Third_Person_View",		"Third_Person_View"},
{"+Toggle_Crouch",			"Toggle_Crouch"}
};

//=============================================================================
//
//
//
//=============================================================================

void C_CON_SetAliases()
{
	if (g_gameType & GAMEFLAG_NAM)
	{
		con_gamefuncs[32].description = "Holo_Soldier";
		con_gamefuncs[33].description = "Huey";
		con_gamefuncs[48].description = "Tank_Mode";
	}
	if (g_gameType & GAMEFLAG_WW2GI)
	{
		con_gamefuncs[32].description = "Fire_Mission";
		con_gamefuncs[33].description = "";
		con_gamefuncs[48].description = "Smokes";
	}

}

static FString C_CON_GetGameFuncOnKeyboard(int gameFunc)
{
	if (gameFunc >= 0 && gameFunc < countof(con_gamefuncs))
	{
		auto keys = Bindings.GetKeysForCommand(con_gamefuncs[gameFunc].action);
		for (auto key : keys)
		{
			if (key < KEY_FIRSTMOUSEBUTTON)
			{
				auto scan = KB_ScanCodeToString(key);
				if (scan) return scan;
			}
		}
	}
	return "";
}

static FString C_CON_GetGameFuncOnMouse(int gameFunc)
{
	if (gameFunc >= 0 && gameFunc < countof(con_gamefuncs))
	{
		auto keys = Bindings.GetKeysForCommand(con_gamefuncs[gameFunc].action);
		for (auto key : keys)
		{
			if ((key >= KEY_FIRSTMOUSEBUTTON && key < KEY_FIRSTJOYBUTTON) || (key >= KEY_MWHEELUP && key <= KEY_MWHEELLEFT))
			{
				auto scan = KB_ScanCodeToString(key);
				if (scan) return scan;
			}
		}
	}
	return "";
}

char const* C_CON_GetGameFuncOnJoystick(int gameFunc)
{
	if (gameFunc >= 0 && gameFunc < countof(con_gamefuncs))
	{
		auto keys = Bindings.GetKeysForCommand(con_gamefuncs[gameFunc].action);
		for (auto key : keys)
		{
			if (key >= KEY_FIRSTJOYBUTTON && !(key >= KEY_MWHEELUP && key <= KEY_MWHEELLEFT))
			{
				auto scan = KB_ScanCodeToString(key);
				if (scan) return scan;
			}
		}
	}
	return "";
}

FString C_CON_GetBoundKeyForLastInput(int gameFunc)
{
	if (inputState.gamePadActive())
	{
		FString name = C_CON_GetGameFuncOnJoystick(gameFunc);
		if (name.IsNotEmpty())
		{
			return name;
		}
	}

	FString name = C_CON_GetGameFuncOnKeyboard(gameFunc);
	if (name.IsNotEmpty())
	{
		return name;
	}

	name = C_CON_GetGameFuncOnMouse(gameFunc);
	if (name.IsNotEmpty())
	{
		return name;
	}

	name = C_CON_GetGameFuncOnJoystick(gameFunc);
	if (name.IsNotEmpty())
	{
		return name;
	}
	return "UNBOUND";
}


//=============================================================================
//
//
//
//=============================================================================

void C_CON_SetButtonAlias(int num, const char *text)
{
	if (num >= 0 && num < countof(con_gamefuncs))
	{
		if (con_gamefuncs[num].replaced) free((void*)con_gamefuncs[num].description);
		con_gamefuncs[num].description = strdup(text);
		con_gamefuncs[num].replaced = true;
	}
}

//=============================================================================
//
//
//
//=============================================================================

void C_CON_ClearButtonAlias(int num)
{
	if (num >= 0 && num < countof(con_gamefuncs))
	{
		if (con_gamefuncs[num].replaced) free((void*)con_gamefuncs[num].description);
		con_gamefuncs[num].description = "";
		con_gamefuncs[num].replaced = false;
	}
}

//=============================================================================
//
//
//
//=============================================================================

const char *C_CON_GetButtonFunc(int num)
{
	if (num >= 0 && num < countof(con_gamefuncs))
	{
		return con_gamefuncs[num].action;
	}
	return "";
}
