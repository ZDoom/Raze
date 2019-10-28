#pragma once

#include <stdint.h>
#include "tarray.h"
#include "scancodes.h"

typedef uint8_t kb_scancode;

extern kb_scancode KB_LastScan;

typedef struct
{
	const char* key;
	char* cmdstr;
	char repeat;
	char laststate;
}
consolekeybind_t;

// This encapsulates the entire game-readable input state which previously was spread out across several files.

enum
{
	NUMKEYS = 256,
	MAXMOUSEBUTTONS = 10,

};

extern consolekeybind_t CONTROL_KeyBinds[NUMKEYS + MAXMOUSEBUTTONS];


// Order is that of EDuke32 by necessity because it exposes the key binds to scripting  by index instead of by name.
enum GameFunction_t
{
	gamefunc_Move_Forward,
	gamefunc_Move_Backward,
	gamefunc_Turn_Left,
	gamefunc_Turn_Right,
	gamefunc_Strafe,
	gamefunc_Fire,
	gamefunc_Open,
	gamefunc_Run,
	gamefunc_Alt_Fire,
	gamefunc_Jump,
	gamefunc_Crouch,
	gamefunc_Look_Up,
	gamefunc_Look_Down,
	gamefunc_Look_Left,
	gamefunc_Look_Right,
	gamefunc_Strafe_Left,
	gamefunc_Strafe_Right,
	gamefunc_Aim_Up,
	gamefunc_Aim_Down,
	gamefunc_Weapon_1,
	gamefunc_Weapon_2,
	gamefunc_Weapon_3,
	gamefunc_Weapon_4,
	gamefunc_Weapon_5,
	gamefunc_Weapon_6,
	gamefunc_Weapon_7,
	gamefunc_Weapon_8,
	gamefunc_Weapon_9,
	gamefunc_Weapon_10,
	gamefunc_Inventory,
	gamefunc_Inventory_Use = gamefunc_Inventory,
	gamefunc_Inventory_Left,
	gamefunc_Inventory_Right,
	gamefunc_Holo_Duke,
	gamefunc_Jetpack,
	gamefunc_NightVision,
	gamefunc_Night_Vision = gamefunc_NightVision,
	gamefunc_MedKit,
	gamefunc_Med_Kit = gamefunc_MedKit,
	gamefunc_TurnAround,
	gamefunc_SendMessage,
	gamefunc_Map,
	gamefunc_Map_Toggle = gamefunc_Map,
	gamefunc_Shrink_Screen,
	gamefunc_Enlarge_Screen,
	gamefunc_Center_View,
	gamefunc_Holster_Weapon,
	gamefunc_Show_Opponents_Weapon,
	gamefunc_Map_Follow_Mode,
	gamefunc_See_Coop_View,
	gamefunc_See_Co_Op_View = gamefunc_See_Coop_View,
	gamefunc_Mouse_Aiming,
	gamefunc_Toggle_Crosshair,
	gamefunc_Steroids,
	gamefunc_Quick_Kick,
	gamefunc_Next_Weapon,
	gamefunc_Previous_Weapon,
	gamefunc_Show_Console,
	gamefunc_Show_DukeMatch_Scores,
	gamefunc_Dpad_Select,
	gamefunc_Dpad_Aiming,
	gamefunc_AutoRun,
	gamefunc_Last_Weapon,
	gamefunc_Quick_Save,
	gamefunc_Quick_Load,
	gamefunc_Alt_Weapon,
	gamefunc_Third_Person_View,
	gamefunc_Toggle_Crouch,
	gamefunc_See_Chase_View,	// this was added by Blood
	gamefunc_Turn_Around,
	gamefunc_Weapon_Fire,
	gamefunc_Weapon_Special_Fire,
	gamefunc_Aim_Center,
	gamefunc_Tilt_Left,
	gamefunc_Tilt_Right,
	gamefunc_Send_Message,
	gamefunc_BeastVision,
	gamefunc_CrystalBall,
	gamefunc_JumpBoots,
	gamefunc_ProximityBombs,
    gamefunc_RemoteBombs,
	gamefunc_Smoke_Bomb,			// and these by ShadowWarrior (todo: There's quite a bit of potential for consolidation here - is it worth it?)
	gamefunc_Gas_Bomb,
	gamefunc_Flash_Bomb,
	gamefunc_Caltrops,

	NUMGAMEFUNCTIONS
};


class InputState
{

	FixedBitArray<NUMGAMEFUNCTIONS> ButtonState, ButtonHeldState;
	uint8_t KeyStatus[NUMKEYS];

public:

	bool BUTTON(int x)
	{
		return ButtonState[x];
	}

	bool BUTTONHELD(int x)
	{
		return ButtonHeldState[x];
	}

	void SetButton(int x, bool set = true)
	{
		if (set) ButtonState.Set(x);
		else ButtonState.Clear(x);
	}
	
	void SetButtonHeld(int x, bool set = true)
	{
		if (set) ButtonHeldState.Set(x);
		else ButtonHeldState.Clear(x);
	}
	
	void ClearButton(int x)
	{
		ButtonState.Clear(x);
	}
	
	void ClearButtonHeld(int x)
	{
		ButtonHeldState.Clear(x);
	}
	
	void ClearButtonState()
	{
		ButtonState.Zero();
		ButtonHeldState.Zero();
	}
	
	void PrepareState()
	{
		ButtonHeldState = ButtonState;
		ButtonState.Zero();
	}
	
	uint8_t GetKeyStatus(int key)
	{
		return KeyStatus[key];
	}
	
	void SetKeyStatus(int key, int state = 1)
	{
		KeyStatus[key] = (uint8_t)state;
	}
	
	void ClearKeyStatus(int key)
	{
		KeyStatus[key] = 0;
	}
	
	void ClearAllKeyStatus()
	{
		memset(KeyStatus, 0, sizeof(KeyStatus));
	}

	bool AltPressed()
	{
		return KeyStatus[sc_LeftAlt] || KeyStatus[sc_RightAlt];
	}
	
	bool CtrlPressed()
	{
		return KeyStatus[sc_LeftControl] || KeyStatus[sc_RightControl];
	}
	
	bool ShiftPressed()
	{
		return KeyStatus[sc_LeftShift] || KeyStatus[sc_RightShift];
	}

	bool EscapePressed()
	{
		return !!KeyStatus[sc_Escape];
	}
	
};


extern InputState inputState;

inline bool BUTTON(int x)
{
	return inputState.BUTTON(x);
}

inline bool BUTTONHELD(int x)
{
	return inputState.BUTTONHELD(x);
}

inline bool BUTTONJUSTPRESSED(int x)
{
	return (BUTTON(x) && !BUTTONHELD(x));
}

inline bool BUTTONRELEASED(int x)
{
	return (!BUTTON(x) && BUTTONHELD(x));
}

inline bool BUTTONSTATECHANGED(int x)
{
	return (BUTTON(x) != BUTTONHELD(x));
}

inline uint8_t KB_KeyPressed(int scan) 
{
	return inputState.GetKeyStatus(scan);
}

inline void KB_ClearKeyDown(int scan) 
{
	inputState.ClearKeyStatus(scan);
}

inline bool KB_UnBoundKeyPressed(int scan) 
{
	return (inputState.GetKeyStatus(scan) != 0 && !CONTROL_KeyBinds[scan].cmdstr);
}

inline uint8_t KEY_PRESSED(int scan) // Shadow Warrior uses different names for many things.
{
	return inputState.GetKeyStatus(scan);
}

inline void KB_ClearKeysDown(void)
{
	KB_LastScan = 0;
	inputState.ClearAllKeyStatus();
}
