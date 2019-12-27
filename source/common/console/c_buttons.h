#pragma once

#include <stdint.h>
#include "tarray.h"
#include "zstring.h"
#include "name.h"

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
	gamefunc_Alt_Fire,	// Duke3D, Blood
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
	gamefunc_Holo_Duke,			// Duke3D, RR
	gamefunc_Jetpack,
	gamefunc_JumpBoots = gamefunc_Jetpack,
	gamefunc_NightVision,
	gamefunc_Night_Vision = gamefunc_NightVision,
	gamefunc_BeastVision = gamefunc_NightVision,
	gamefunc_MedKit,
	gamefunc_Med_Kit = gamefunc_MedKit,
	gamefunc_TurnAround,
	gamefunc_SendMessage,
	gamefunc_Map,
	gamefunc_Map_Toggle = gamefunc_Map,
	gamefunc_Shrink_Screen,
	gamefunc_Enlarge_Screen,
	gamefunc_Center_View,
	gamefunc_Look_Straight = gamefunc_Center_View,
	gamefunc_Holster_Weapon,
	gamefunc_Show_Opponents_Weapon,
	gamefunc_Map_Follow_Mode,
	gamefunc_See_Coop_View,
	gamefunc_See_Co_Op_View = gamefunc_See_Coop_View,
	gamefunc_Mouse_Aiming,
	gamefunc_Mouseview = gamefunc_Mouse_Aiming,
	gamefunc_Toggle_Crosshair,
	gamefunc_Steroids,
	gamefunc_Quick_Kick,
	gamefunc_Next_Weapon,
	gamefunc_Previous_Weapon,
	gamefunc_Show_DukeMatch_Scores,
	gamefunc_Dpad_Select,
	gamefunc_Dpad_Aiming,
	gamefunc_Last_Weapon,
	gamefunc_Alt_Weapon,
	gamefunc_Third_Person_View,
	gamefunc_See_Chase_View = gamefunc_Third_Person_View,
	gamefunc_Toggle_Crouch,	// This is the last one used by EDuke32.
	gamefunc_CrystalBall,
	gamefunc_ProximityBombs,
    gamefunc_RemoteBombs,
	gamefunc_Smoke_Bomb,			// and these by ShadowWarrior (todo: There's quite a bit of potential for consolidation here - is it worth it?)
	gamefunc_Gas_Bomb,
	gamefunc_Flash_Bomb,
	gamefunc_Caltrops,

	gamefunc_Zoom_In,	// Map controls should not pollute the global button namespace.
	gamefunc_Zoom_Out,
	
	NUMGAMEFUNCTIONS
};



// Actions
struct FButtonStatus
{
	enum { MAX_KEYS = 6 };	// Maximum number of keys that can press this button

	uint16_t Keys[MAX_KEYS];
	bool bDown;				// Button is down right now
	bool bWentDown;			// Button went down this tic
	bool bWentUp;			// Button went up this tic

	bool PressKey (int keynum);		// Returns true if this key caused the button to be pressed.
	bool ReleaseKey (int keynum);	// Returns true if this key is no longer pressed.
	void ResetTriggers () { bWentDown = bWentUp = false; }
	void Reset () { bDown = bWentDown = bWentUp = false; }
};


class ButtonMap
{
	
	FButtonStatus Buttons[NUMGAMEFUNCTIONS];
	FString NumToName[NUMGAMEFUNCTIONS];		// The internal name of the button
	TMap<FName, int> NameToNum;
	
public:
	ButtonMap();
	void SetGameAliases();
	
	constexpr int NumButtons() const
	{
		return NUMGAMEFUNCTIONS;
	}

	int FindButtonIndex(const char* func) const;
	
	FButtonStatus *FindButton(const char *func)
	{
		int index = FindButtonIndex(func);
		return index > -1? &Buttons[index] : nullptr;
	}
	
	// This is still in use but all cases are scheduled for termination.
	const char* GetButtonName(int32_t func) const
	{
		if ((unsigned)func >= (unsigned)NumButtons())
			return nullptr;
		return NumToName[func];
	}

	void ResetButtonTriggers ();	// Call ResetTriggers for all buttons
	void ResetButtonStates ();		// Same as above, but also clear bDown
	int ListActionCommands(const char* pattern);


	bool ButtonDown(int x) const
	{
		return Buttons[x].bDown;
	}

	bool ButtonPressed(int x) const
	{
		return Buttons[x].bWentDown;
	}

	bool ButtonReleased(int x) const
	{
		return Buttons[x].bWentUp;
	}

	void ClearButton(int x)
	{
		Buttons[x].Reset();
	}
};

extern ButtonMap buttonMap;
