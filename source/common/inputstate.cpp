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
