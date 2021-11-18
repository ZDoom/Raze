#pragma once

#include <stdint.h>
#include "compat.h"
#include "printf.h"
#include "c_dispatch.h" 
#include "tarray.h"
#include "scancodes.h"
#include "c_bind.h"
#include "c_buttons.h"
#include "d_event.h"
#include "m_joy.h"
#include "gamecvars.h"
#include "packet.h"


struct ControlInfo
{
	float       dx;
	float       dy;
	float       dz;
	float       dyaw;
	float       dpitch;
	float       droll;
	float       mouseturnx;
	float       mouseturny;
	float       mousemovex;
	float       mousemovey;
};


class InputState
{
	uint8_t KeyStatus[NUM_KEYS];
	bool AnyKeyStatus;
	vec2f_t  g_mousePos;

public:

	bool ShiftPressed()
	{
		return KeyStatus[sc_LeftShift] || KeyStatus[sc_RightShift];
	}

	void AddEvent(const event_t* ev);

	void MouseAddToPos(float x, float y)
	{
		g_mousePos.x += x;
		g_mousePos.y += y;
	}

	void GetMouseDelta(ControlInfo* hidInput);

	void ClearAllInput();
	bool CheckAllInput()
	{
		bool res = AnyKeyStatus;
		AnyKeyStatus = false;
		return res;
	}
};

extern InputState inputState;

ControlInfo CONTROL_GetInput();
int32_t handleevents(void);

enum GameFunction_t
{
	gamefunc_Move_Forward,		//
	gamefunc_Move_Backward,		//
	gamefunc_Turn_Left,			//
	gamefunc_Turn_Right,		//
	gamefunc_Strafe,			//
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
	gamefunc_Strafe_Left,		//
	gamefunc_Strafe_Right,		//
	gamefunc_Aim_Up,
	gamefunc_Aim_Down,
	gamefunc_Shrink_Screen, // Automap only
	gamefunc_Enlarge_Screen, // Automap only
	gamefunc_Mouse_Aiming,
	gamefunc_Dpad_Select,
	gamefunc_Dpad_Aiming,
	gamefunc_Toggle_Crouch,
	gamefunc_Quick_Kick,
	gamefunc_AM_PanLeft,
	gamefunc_AM_PanRight,
	gamefunc_AM_PanUp,
	gamefunc_AM_PanDown,
	NUM_ACTIONS
};

void SetupGameButtons();
void ApplyGlobalInput(InputPacket& input, ControlInfo* const hidInput, bool const crouchable = true, bool const disableToggle = false);
extern ESyncBits ActionsToSend;
double InputScale();
extern bool gamesetinput;
extern double InputScalePercentage;

inline bool SyncInput()
{
	return gamesetinput || cl_syncinput;
}

//---------------------------------------------------------------------------
//
// Inline functions to help with edge cases where synchronised input is needed.
//
//---------------------------------------------------------------------------

inline void setForcedSyncInput()
{
	gamesetinput = true;
}
inline void resetForcedSyncInput()
{
	gamesetinput = false;
}
