/*
** Main input handler
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

#include "inputstate.h"
#include "i_system.h"
#include "v_draw.h"
#include "build.h"
#include "gamecvars.h"
#include "v_video.h"
#include "statusbar.h"

//==========================================================================
//
//
//
//==========================================================================

void InputState::GetMouseDelta(ControlInfo * info)
{
    vec2f_t input, finput;

	input = g_mousePos;
	g_mousePos = {};

    if (in_mousesmoothing)
    {
        static vec2f_t last;
        finput = { (input.x + last.x) * 0.5f, (input.y + last.y) * 0.5f };
        last = input;
    }
    else
    {
    	finput = { input.x, input.y };
    }

    info->mousex = finput.x * (16.f / 32.f) * in_mousesensitivity * in_mousescalex / 3.f;
    info->mousey = finput.y * (16.f / 64.f) * in_mousesensitivity * in_mousescaley;

	// todo: Use these when the mouse is used for moving instead of turning.
	//info->mousex = int(finput.x * (4.f) * in_mousesensitivity * in_mouseside);
	//info->mousey = int(finput.y * (4.f) * in_mousesensitivity * in_mouseforward);

	if (in_mousebias)
	{
		if (fabs(info->mousex) > fabs(info->mousey))
			info->mousey /= in_mousebias;
		else
			info->mousex /= in_mousebias;
	}

}

//==========================================================================
//
//
//
//==========================================================================

void InputState::keySetState(int32_t key, int32_t state)
{
	KeyStatus[key] = (uint8_t)state;

	if (state)
	{
		g_keyFIFO[g_keyFIFOend] = key;
		g_keyFIFO[(g_keyFIFOend + 1) & (KEYFIFOSIZ - 1)] = state;
		g_keyFIFOend = ((g_keyFIFOend + 2) & (KEYFIFOSIZ - 1));
	}
}


//==========================================================================
//
//
//
//==========================================================================

void InputState::AddEvent(const event_t *ev)
{
	if (ev->type == EV_KeyDown || ev->type == EV_KeyUp)
	{
		keySetState(ev->data1, ev->type == EV_KeyDown);
		if (ev->data2 && ev->type == EV_KeyDown)
		{
			g_keyAsciiFIFO[g_keyAsciiEnd] = (char16_t)ev->data2;
			g_keyAsciiEnd = ((g_keyAsciiEnd + 1) & (KEYFIFOSIZ - 1));
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void InputState::ClearAllInput()
{
	memset(KeyStatus, 0, sizeof(KeyStatus));
	keyFlushChars();
	keyFlushScans();
	buttonMap.ResetButtonStates();	// this is important. If all input is cleared, the buttons must be cleared as well.
	gi->clearlocalinputstate();		// also clear game local input state.
}


//==========================================================================
//
//
//
//==========================================================================

void I_StartTic();
bool ToggleFullscreen;

int32_t handleevents(void)
{
	if (ToggleFullscreen)
	{
		vid_fullscreen = !vid_fullscreen;
		ToggleFullscreen = false;
	}
	// fullscreen toggle has been requested
	if (setmodeneeded)
	{
		setmodeneeded = false;
		screen->ToggleFullscreen(vid_fullscreen);
		V_OutputResized(screen->GetWidth(), screen->GetHeight());
	}

	// change the view size if needed
	if (setsizeneeded)
	{
		videoSetGameMode(vid_fullscreen, SCREENWIDTH, SCREENHEIGHT, 32, 1);
		setViewport(hud_size);
		setsizeneeded = false;
	}

	timerUpdateClock();

	I_StartFrame();
	I_StartTic();
	return 0;
}

//==========================================================================
//
//
//
//==========================================================================

void CONTROL_GetInput(ControlInfo* info)
{
	memset(info, 0, sizeof(ControlInfo));

	inputState.GetMouseDelta(info);

	if (use_joystick)
	{
		// Handle joysticks/game controllers.
		float joyaxes[NUM_JOYAXIS];

		I_GetAxes(joyaxes);

		info->dyaw += -joyaxes[JOYAXIS_Yaw] * 45.f;
		info->dx += -joyaxes[JOYAXIS_Side] * 0.75f;
		info->dz += -joyaxes[JOYAXIS_Forward] * 0.75f;
		info->dpitch += -joyaxes[JOYAXIS_Pitch] * 22.5f;
	}
}
