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

static int WeaponToSend = 0;
ESyncBits ActionsToSend = 0;
static int dpad_lock = 0;
bool sendPause;

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

void InputState::AddEvent(const event_t *ev)
{
	if (ev->type == EV_KeyDown || ev->type == EV_KeyUp)
	{
		int key = ev->data1;
		bool state = ev->type == EV_KeyDown;
		KeyStatus[key] = (uint8_t)state;
		if (state && !(key > KEY_LASTJOYBUTTON && key < KEY_PAD_LTHUMB_RIGHT))
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
	ActionsToSend = 0;
	WeaponToSend = 0;
	dpad_lock = 0;
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
		"Show_Opponents_Weapon",
		"See_Coop_View",
		"Mouse_Aiming",
		"Dpad_Select",
		"Dpad_Aiming",
		"Third_Person_View",
		"Toggle_Crouch",
		"Quick_Kick",
	};
	buttonMap.SetButtons(actions, NUM_ACTIONS);
}

//==========================================================================
//
//
//
//==========================================================================

CCMD(slot)
{
	// The max differs between games so we have to handle this here.
	int max = (g_gameType & GAMEFLAG_PSEXHUMED) || (g_gameType & (GAMEFLAG_DUKE | GAMEFLAG_SHAREWARE)) == (GAMEFLAG_DUKE | GAMEFLAG_SHAREWARE) ? 7 : (g_gameType & GAMEFLAG_BLOOD) ? 12 : 10;
	if (argv.argc() != 2)
	{
		Printf("slot <weaponslot>: select a weapon from the given slot (1-%d)", max);
	}

	auto slot = atoi(argv[1]);
	if (slot >= 1 && slot <= max)
	{
		WeaponToSend = slot;
	}
}

CCMD(weapprev)
{
	WeaponToSend = WeaponSel_Prev;
}

CCMD(weapnext)
{
	WeaponToSend = WeaponSel_Next;
}

CCMD(weapalt)
{
	WeaponToSend = WeaponSel_Alt;	// Only used by SW - should also be made usable by Blood ans Duke which put multiple weapons in the same slot.
}

CCMD(useitem)
{
	int max = (g_gameType & GAMEFLAG_PSEXHUMED)? 6 : (g_gameType & GAMEFLAG_SW)? 7 : (g_gameType & GAMEFLAG_BLOOD) ? 4 : 5;
	if (argv.argc() != 2)
	{
		Printf("useitem <itemnum>: activates an inventory item (1-%d)", max);
	}

	auto slot = atoi(argv[1]);
	if (slot >= 1 && slot <= max)
	{
		ActionsToSend |= ESyncBits::FromInt(SB_ITEM_BIT_1 << (slot - 1));
	}
}

CCMD(invprev)
{
	ActionsToSend |= SB_INVPREV;
}

CCMD(invnext)
{
	ActionsToSend |= SB_INVNEXT;
}

CCMD(invuse)
{
	ActionsToSend |= SB_INVUSE;
}

CCMD(centerview)
{
	ActionsToSend |= SB_CENTERVIEW;
}

CCMD(turnaround)
{
	ActionsToSend |= SB_TURNAROUND;
}

CCMD(holsterweapon)
{
	ActionsToSend |= SB_HOLSTER;
}

CCMD(pause)
{
	sendPause = true;
}



void ApplyGlobalInput(InputPacket& input, ControlInfo *info)
{
	if (WeaponToSend != 0) input.setNewWeapon(WeaponToSend);
	WeaponToSend = 0;
	if (info && buttonMap.ButtonDown(gamefunc_Dpad_Select))
	{
		// These buttons should not autorepeat. The game handlers are not really equipped for that.
		if (info->dz > 0 && !(dpad_lock & 1)) { dpad_lock |= 1;  input.setNewWeapon(WeaponSel_Prev); }
		else dpad_lock &= ~1;
		if (info->dz < 0 && !(dpad_lock & 2)) { dpad_lock |= 2;  input.setNewWeapon(WeaponSel_Next); }
		else dpad_lock &= ~2;
		if ((info->dx < 0 || info->dyaw < 0) && !(dpad_lock & 4)) { dpad_lock |= 4;  input.actions |= SB_INVPREV; }
		else dpad_lock &= ~4;
		if ((info->dx > 0 || info->dyaw > 0) && !(dpad_lock & 8)) { dpad_lock |= 8;  input.actions |= SB_INVNEXT; }
		else dpad_lock &= ~8;

		// This eats the controller input for regular use
		info->dx = 0;
		info->dz = 0;
		info->dyaw = 0;
	}
	else dpad_lock = 0;

	input.actions |= ActionsToSend;
	ActionsToSend = 0;

}

#if 0
void registerinputcommands()
{
	C_RegisterFunction("centerview", nullptr, [](CCmdFuncPtr)->int { BitsToSend.lookCenter = 1; return CCMD_OK; });
	C_RegisterFunction("holsterweapon", nullptr, [](CCmdFuncPtr)->int { BitsToSend.holsterWeapon = 1; return CCMD_OK; });
	C_RegisterFunction("turnaround", nullptr, [](CCmdFuncPtr)->int { BitsToSend.spin180 = 1; return CCMD_OK; });
}

//---------------------------------------------------------------------------
//
// CCMD based input. The basics are from Randi's ZDuke but this uses dynamic
// registration to only have the commands active when this game module runs.
//
//---------------------------------------------------------------------------

	C_RegisterFunction("backoff", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_ESCAPE; return CCMD_OK; });
}


#endif
