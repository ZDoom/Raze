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
#include"packet.h"
#include "gamecontrol.h"
#include "gamestruct.h"
#include "gamestate.h"
#include "gameinput.h"

//==========================================================================
//
//
//
//==========================================================================

static int exclKeys[] = { KEY_VOLUMEDOWN, KEY_VOLUMEUP };

void InputState::AddEvent(const event_t *ev)
{
	if (ev->type == EV_KeyDown || ev->type == EV_KeyUp)
	{
		int key = ev->data1;
		bool state = ev->type == EV_KeyDown;
		bool ignore = false;
		KeyStatus[key] = (uint8_t)state;

		// Check if key is to be excluded from setting AnyKeyStatus.
		for (int i = 0; i < 2; i++)
		{
			if (exclKeys[i] == key)
			{
				ignore = true;
				break;
			}
		}
		if (key > KEY_LASTJOYBUTTON && key < KEY_PAD_LTHUMB_RIGHT)
		{
			ignore = true;
		}

		if (state && !ignore)
			AnyKeyStatus = true;
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
	AnyKeyStatus = false;
	buttonMap.ResetButtonStates();	// this is important. If all input is cleared, the buttons must be cleared as well.
	clearLocalInputBuffer();		// also clear game local input state.
}


//==========================================================================
//
//
//
//==========================================================================

void I_StartTic();
extern bool ToggleFullscreen;

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
		setVideoMode();
		setViewport(hud_size);
		setsizeneeded = false;
	}

	I_StartFrame();
	I_StartTic();
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SetupGameButtons()
{
	static const char* actions[] = {
		"Move_Forward",
		"Move_Backward",
		"Turn_Left",
		"Turn_Right",
		"Strafe",
		"Fire",
		"Open",
		"Run",
		"Alt_Fire",
		"Jump",
		"Crouch",
		"Look_Up",
		"Look_Down",
		"Look_Left",
		"Look_Right",
		"Strafe_Left",
		"Strafe_Right",
		"Aim_Up",
		"Aim_Down",
		"Shrink_Screen",
		"Enlarge_Screen",
		"Mouse_Aiming",
		"Dpad_Select",
		"Dpad_Aiming",
		"Toggle_Crouch",
		"Quick_Kick",
		"AM_PanLeft",
		"AM_PanRight",
		"AM_PanUp",
		"AM_PanDown",

	};
	buttonMap.SetButtons(actions, NUM_ACTIONS);
}
