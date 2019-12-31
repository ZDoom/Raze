#include "inputstate.h"
#include "v_draw.h"
#include "build.h"
#include "gamecvars.h"

void InputState::GetMouseDelta(ControlInfo * info)
{
    vec2_t input;
    if (!appactive)
    {
		input = {0,0};
        return;
    }

	input = g_mousePos;
	g_mousePos = {};

    vec2f_t finput = { float(input.x), float(input.y) };

    if (in_mousesmoothing)
    {
        static vec2_t last;
        finput = { float(input.x + last.x) * 0.5f, float(input.y + last.y) * 0.5f };
        last = input;
    }

    info->mousex = int(finput.x * (16.f) * in_mousesensitivity * in_mousescalex);
    info->mousey = int(finput.y * (16.f) * in_mousesensitivity * in_mousescaley);

	// todo: Use these when the mouse is used for moving instead of turning.
	//info->mousex = int(finput.x * (4.f) * in_mousesensitivity * in_mouseside);
	//info->mousey = int(finput.y * (4.f) * in_mousesensitivity * in_mouseforward);

	if (in_mousedeadzone)
	{
		if (info->mousey > 0)
			info->mousey = max(info->mousey - in_mousedeadzone, 0);
		else if (info->mousey < 0)
			info->mousey = min(info->mousey + in_mousedeadzone, 0);

		if (info->mousex > 0)
			info->mousex = max(info->mousex - in_mousedeadzone, 0);
		else if (info->mousex < 0)
			info->mousex = min(info->mousex + in_mousedeadzone, 0);
	}

	if (in_mousebias)
	{
		if (abs(info->mousex) > abs(info->mousey))
			info->mousey = tabledivide32_noinline(info->mousey, in_mousebias);
		else
			info->mousex = tabledivide32_noinline(info->mousex, in_mousebias);
	}

}

void InputState::AddEvent(const event_t *ev)
{
	if (ev->type == EV_KeyDown || ev->type == EV_KeyUp)
	{
		keySetState(ev->data1, ev->type == EV_KeyDown);
		if (ev->data2) keySetChar(ev->data2);
	}
}

void I_StartTic();

int32_t handleevents(void)
{
	timerUpdateClock();

	// The mouse wheel is not a real key so in order to be "pressed" it may only be cleared at the end of the tic (or the start of the next.)
	if (inputState.GetKeyStatus(KEY_MWHEELUP))
	{
		event_t ev = { EV_KeyUp, 0, (int16_t)KEY_MWHEELUP };
		D_PostEvent(&ev);
	}
	if (inputState.GetKeyStatus(KEY_MWHEELDOWN))
	{
		event_t ev = { EV_KeyUp, 0, (int16_t)KEY_MWHEELDOWN };
		D_PostEvent(&ev);
	}
	if (inputState.GetKeyStatus(KEY_MWHEELLEFT))
	{
		event_t ev = { EV_KeyUp, 0, (int16_t)KEY_MWHEELLEFT };
		D_PostEvent(&ev);
	}
	if (inputState.GetKeyStatus(KEY_MWHEELRIGHT))
	{
		event_t ev = { EV_KeyUp, 0, (int16_t)KEY_MWHEELRIGHT };
		D_PostEvent(&ev);
	}

	I_StartTic();
	return 0;
}

void CONTROL_GetInput(ControlInfo* info)
{
	memset(info, 0, sizeof(ControlInfo));

	if (in_mouse >= 0)
		inputState.GetMouseDelta(info);

	if (in_joystick >= 0)
	{
		// Handle joysticks/game controllers.
		float joyaxes[NUM_JOYAXIS];

		I_GetAxes(joyaxes);

		info->dyaw += joyaxes[JOYAXIS_Yaw];
		info->dx += joyaxes[JOYAXIS_Side];
		info->dz += joyaxes[JOYAXIS_Forward];
		info->dpitch += joyaxes[JOYAXIS_Pitch];
	}
}
