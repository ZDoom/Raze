#include "inputstate.h"
#include "v_draw.h"
#include "build.h"
#include "gamecvars.h"

int32_t InputState::mouseReadAbs(vec2_t * const pResult)
{
	auto pInput = &g_mouseAbs;
    if (!g_mouseEnabled || !appactive || !g_mouseInsideWindow || GUICapture)
        return 0;

    int32_t const xwidth = max(scale(240<<16, screen->GetWidth(), screen->GetHeight()), 320<<16);

    pResult->x = scale(pInput->x, xwidth, xres) - ((xwidth>>1) - (320<<15));
    pResult->y = scale(pInput->y, 200<<16, yres);

    pResult->y = divscale16(pResult->y - (200<<15), rotatesprite_yxaspect) + (200<<15) - rotatesprite_y_offset;

    return 1;
}

void InputState::GetMouseDelta(ControlInfo * info)
{
    vec2_t input;
    if (!g_mouseEnabled || !g_mouseGrabbed || !appactive)
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

}

void InputState::AddEvent(const event_t *ev)
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
		keySetState(ev->data1, ev->type == EV_KeyDown);
		if (ev->data2) keySetChar(ev->data2);
	}
}
