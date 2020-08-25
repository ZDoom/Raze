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


struct ControlInfo
{
	float       dx;
	float       dy;
	float       dz;
	float       dyaw;
	float       dpitch;
	float       droll;
	float       mousex;
	float       mousey;
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

	void GetMouseDelta(ControlInfo* info);

	void ClearAllInput();
	bool CheckAllInput()
	{
		bool res = AnyKeyStatus;
		AnyKeyStatus = false;
		return res;
	}
};

extern InputState inputState;

void CONTROL_GetInput(ControlInfo* info);
int32_t handleevents(void);
