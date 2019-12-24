#pragma once

#include <stdint.h>
#include "tarray.h"
#include "scancodes.h"
#include "c_bind.h"
#include "c_buttons.h"
#include "d_event.h"
#include "osd.h"
#include "m_joy.h"
#include "gamecvars.h"

extern char appactive;

typedef uint16_t kb_scancode;
extern int GUICapture;

// This encapsulates the entire game-readable input state which previously was spread out across several files.

struct ControlInfo
{
	int32_t     dx;
	int32_t     dy;
	int32_t     dz;
	int32_t     dyaw;
	int32_t     dpitch;
	int32_t     droll;
	int32_t     mousex;
	int32_t     mousey;
};


class InputState
{
	enum
	{
		KEYFIFOSIZ = 64,
	};

	uint8_t KeyStatus[NUM_KEYS];

	kb_scancode g_keyFIFO[KEYFIFOSIZ];
	char16_t   g_keyAsciiFIFO[KEYFIFOSIZ];
	uint8_t g_keyFIFOpos;
	uint8_t g_keyFIFOend;
	uint8_t g_keyAsciiPos;
	uint8_t g_keyAsciiEnd;

	kb_scancode KB_LastScan;
	
	vec2_t  g_mousePos;

public:

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
	}
	
	bool keyBufferWaiting()
	{
		return (g_keyAsciiPos != g_keyAsciiEnd);
	}

	int keyBufferFull(void)
	{
		return ((g_keyAsciiEnd + 1) & (KEYFIFOSIZ - 1)) == g_keyAsciiPos;
	}

	void keySetState(int32_t key, int32_t state)
	{
		if (state && !GetKeyStatus(key))
		{
			KB_LastScan = key;
		}

		SetKeyStatus(key, state);
		if (state)
		{
			g_keyFIFO[g_keyFIFOend] = key;
			g_keyFIFO[(g_keyFIFOend + 1) & (KEYFIFOSIZ - 1)] = state;
			g_keyFIFOend = ((g_keyFIFOend + 2) & (KEYFIFOSIZ - 1));
		}
	}

	kb_scancode keyGetScan()
	{
		if (g_keyFIFOpos == g_keyFIFOend)
			return 0;

		auto const c = g_keyFIFO[g_keyFIFOpos];
		g_keyFIFOpos = ((g_keyFIFOpos + 2) & (KEYFIFOSIZ - 1));
		return c;
	}

	void keyFlushScans(void)
	{
		memset(&g_keyFIFO, 0, sizeof(g_keyFIFO));
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
		g_keyAsciiFIFO[g_keyAsciiEnd] = (char16_t)key;
		g_keyAsciiEnd = ((g_keyAsciiEnd + 1) & (KEYFIFOSIZ - 1));
	}

	void keyFlushChars(void)
	{
		memset(&g_keyAsciiFIFO, 0, sizeof(g_keyAsciiFIFO));
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
	
	void AddEvent(const event_t* ev);

	void MouseSetPos(int x, int y)
	{
		g_mousePos = { x, y };
	}
	void MouseAddToPos(int x, int y)
	{
		g_mousePos.x += x;
		g_mousePos.y += y;
	}

	bool gamePadActive()
	{
		// fixme: This needs to be tracked.
		return false;
	}
	void GetMouseDelta(ControlInfo* info);

	void ClearAllInput()
	{
		ClearKeysDown();
		keyFlushChars();
		keyFlushScans();
		buttonMap.ResetButtonStates();	// this is important. If all input is cleared, the buttons must be cleared as well.
	}

	bool CheckAllInput()
	{
		auto res = keyGetScan();
		ClearAllInput();
		return res;
	}

};

extern InputState inputState;

void CONTROL_GetInput(ControlInfo* info);
int32_t handleevents(void);


#define WIN_IS_PRESSED ( inputState.WinPressed() )
#define ALT_IS_PRESSED ( inputState.AltPressed() )
#define SHIFTS_IS_PRESSED ( inputState.ShiftPressed() )
