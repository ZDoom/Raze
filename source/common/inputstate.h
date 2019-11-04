#pragma once

#include <stdint.h>
#include "tarray.h"
#include "scancodes.h"
#include "c_bind.h"
#include "d_event.h"

typedef uint8_t kb_scancode;

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

extern int32_t CONTROL_ButtonFlags[NUMKEYS];
extern bool CONTROL_BindsEnabled;

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


enum EMouseBits
{
	LEFT_MOUSE     = 1,
	RIGHT_MOUSE    = 2,
	MIDDLE_MOUSE   = 4,
	THUMB_MOUSE    = 8,
	WHEELUP_MOUSE  = 16,
	WHEELDOWN_MOUSE= 32,
	THUMB2_MOUSE    = 64,
};

enum
{
    MOUSE_IDLE = 0,
    MOUSE_PRESSED,
    MOUSE_HELD,
    MOUSE_RELEASED,
};


class InputState
{
	enum
	{
		KEYFIFOSIZ = 64,
	};
	// NOTE: This entire thing is mostly a band-aid to wrap something around MACT so that replacing it with a true event-driven system later
	// won't result in a total disaster. None of this is meant to live for long because the input method at use here is fundamentally flawed
	// because it does not track what triggered the button.
	struct ButtonStateFlags
	{
		bool ButtonActive;	// Button currently reports being active to the client
		bool ButtonCleared;	// Button has been cleared by the client, i.e. do not set to active until no input for this button is active anymore.
	};

	ButtonStateFlags ButtonState[NUMGAMEFUNCTIONS];
	uint8_t KeyStatus[NUMKEYS];

	char    g_keyFIFO[KEYFIFOSIZ];
	char    g_keyAsciiFIFO[KEYFIFOSIZ];
	uint8_t g_keyFIFOpos;
	uint8_t g_keyFIFOend;
	uint8_t g_keyAsciiPos;
	uint8_t g_keyAsciiEnd;

	kb_scancode KB_LastScan;
	
	int g_mouseBits;
	uint8_t g_mouseClickState;
public:

	bool BUTTON(int x)
	{
		return ButtonState[x].ButtonActive;
	}

	// Receive a status update
	void UpdateButton(int x, bool set)
	{
		auto &b = ButtonState[x];
		if (!b.ButtonCleared) b.ButtonActive = set;
		else if (!set) b.ButtonCleared = false;

	}
	
	void ClearButton(int x)
	{
		ButtonState[x].ButtonActive = false;
		ButtonState[x].ButtonCleared = true;
	}
	
	void ClearAllButtons()
	{
		for (auto & b : ButtonState)
		{
			b.ButtonActive = false;
			b.ButtonCleared = true;
		}
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

	bool WinPressed()
	{
		return KeyStatus[sc_LeftWin] || KeyStatus[sc_RightWin];
	}
	
	bool ShiftPressed()
	{
		return KeyStatus[sc_LeftShift] || KeyStatus[sc_RightShift];
	}
	
	bool EscapePressed()
	{
		return !!KeyStatus[sc_Escape];
	}
	
	void SetBindsEnabled(bool on)
	{
		// This just forwards the setting
		CONTROL_BindsEnabled = on;
	}
	
	bool keyBufferWaiting()
	{
		return (g_keyAsciiPos != g_keyAsciiEnd);
	}

	int keyBufferFull(void)
	{
		return ((g_keyAsciiEnd + 1) & (KEYFIFOSIZ - 1)) == g_keyAsciiPos;
	}

	void keyBufferInsert(char code)
	{
		g_keyAsciiFIFO[g_keyAsciiEnd] = code;
		g_keyAsciiEnd = ((g_keyAsciiEnd + 1) & (KEYFIFOSIZ - 1));
	}

	void keySetState(int32_t key, int32_t state)
	{
		if (state && !GetKeyStatus(key))
		{
			KB_LastScan = key;
		}

		SetKeyStatus(key, state);
		event_t ev = { (uint8_t)(state ? EV_KeyDown : EV_KeyUp), 0, (int16_t)key };

		if (state)
		{
			g_keyFIFO[g_keyFIFOend] = key;
			g_keyFIFO[(g_keyFIFOend + 1) & (KEYFIFOSIZ - 1)] = state;
			g_keyFIFOend = ((g_keyFIFOend + 2) & (KEYFIFOSIZ - 1));
		}
	}

	char keyGetScan(void)
	{
		if (g_keyFIFOpos == g_keyFIFOend)
			return 0;

		char const c = g_keyFIFO[g_keyFIFOpos];
		g_keyFIFOpos = ((g_keyFIFOpos + 2) & (KEYFIFOSIZ - 1));

		return c;
	}

	void keyFlushScans(void)
	{
		Bmemset(&g_keyFIFO, 0, sizeof(g_keyFIFO));
		g_keyFIFOpos = g_keyFIFOend = 0;
	}

	//
	// character-based input functions
	//
	char keyGetChar(void)
	{
		if (g_keyAsciiPos == g_keyAsciiEnd)
			return 0;

		char const c = g_keyAsciiFIFO[g_keyAsciiPos];
		g_keyAsciiPos = ((g_keyAsciiPos + 1) & (KEYFIFOSIZ - 1));

		return c;
	}
	
	void keySetChar(int key)
	{
		g_keyAsciiFIFO[g_keyAsciiEnd] = key;
		g_keyAsciiEnd = ((g_keyAsciiEnd + 1) & (KEYFIFOSIZ - 1));
	}

	void keyFlushChars(void)
	{
		Bmemset(&g_keyAsciiFIFO, 0, sizeof(g_keyAsciiFIFO));
		g_keyAsciiPos = g_keyAsciiEnd = 0;
	}

	inline bool UnboundKeyPressed(int scan)
	{
		return (GetKeyStatus(scan) != 0 && Bindings.GetBind(scan) == nullptr);
	}


	kb_scancode GetLastScanCode()
	{
		return (KB_LastScan);
	}

	void SetLastScanCode(kb_scancode scancode)
    {
        KB_LastScan = (scancode);  
    }

	void ClearLastScanCode()
    {
        KB_LastScan = sc_None;
    }

	void ClearKeysDown(void)
	{
		KB_LastScan = 0;
		ClearAllKeyStatus();
	}
	
	void mouseSetBit(int val, int state)
	{
		if (state) g_mouseBits |= val;
		else g_mouseBits &=~val;
	}
	
	void handleevents_updatemousestate(uint8_t state)
	{
		g_mouseClickState = state == EV_KeyUp ? MOUSE_RELEASED : MOUSE_PRESSED;
	}

	void AddEvent(const event_t *ev)
	{
		// Set the old mouseBits. Yet another piece of cruft that needs to go away.
		if (ev->type == EV_KeyDown || ev->type == EV_KeyUp)
		{
			switch (ev->data1)
			{
				case KEY_MOUSE1	: mouseSetBit(LEFT_MOUSE, ev->type == EV_KeyDown); handleevents_updatemousestate(ev->type); break;
				case KEY_MOUSE2	: mouseSetBit(RIGHT_MOUSE, ev->type == EV_KeyDown); break;
				case KEY_MOUSE3	: mouseSetBit(MIDDLE_MOUSE, ev->type == EV_KeyDown); break;
				case KEY_MOUSE4	: mouseSetBit(THUMB_MOUSE, ev->type == EV_KeyDown); break;
				case KEY_MWHEELUP: mouseSetBit(WHEELUP_MOUSE, ev->type == EV_KeyDown); break;
				case KEY_MWHEELDOWN: mouseSetBit(WHEELDOWN_MOUSE, ev->type == EV_KeyDown); break;
				case KEY_MOUSE5: mouseSetBit(THUMB2_MOUSE, ev->type == EV_KeyDown); break;
				default: break;
			}
		}
		keySetState(ev->data1, ev->type == EV_KeyDown);
		if (ev->data2) keySetChar(ev->data2);
	}

	int32_t mouseReadButtons(void)
	{
		return (!g_mouseEnabled || !appactive || !g_mouseInsideWindow || (osd && osd->flags & OSD_CAPTURE)) ? 0 : g_mouseBits;
	}
	
	int mouseClickState()
	{
		return g_mouseClickState;
	}
	
	void clearMouseClickState()
	{
		g_mouseClickState = MOUSE_IDLE;
	}
	

	int32_t mouseAdvanceClickState(void)
	{
		switch (g_mouseClickState)
		{
			case MOUSE_PRESSED: g_mouseClickState  = MOUSE_HELD; return 1;
			case MOUSE_RELEASED: g_mouseClickState = MOUSE_IDLE; return 1;
			case MOUSE_HELD: return 1;
		}
		return 0;
	}
	static inline int32_t MouseGetButtons(void) { return mouseReadButtons(); }
	static inline void MouseClearButton(int32_t b) { g_mouseBits &= ~b; }
	static inline void MouseClearAllButtonss(void) { g_mouseBits = 0; }
};


extern InputState inputState;

