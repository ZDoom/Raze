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
#include "d_net.h"
#include "gamestate.h"
#include "gameinput.h"

static int WeaponToSend = 0;
ESyncBits ActionsToSend = 0;
static int dpad_lock = 0;
bool sendPause;
bool crouch_toggle;

CVAR(Float, m_pitch, 1.f, CVAR_GLOBALCONFIG | CVAR_ARCHIVE)		// Mouse speeds
CVAR(Float, m_yaw, 1.f, CVAR_GLOBALCONFIG | CVAR_ARCHIVE)
CVAR(Float, m_forward, 1.f, CVAR_GLOBALCONFIG | CVAR_ARCHIVE)
CVAR(Float, m_side, 1.f, CVAR_GLOBALCONFIG | CVAR_ARCHIVE)

//==========================================================================
//
//
//
//==========================================================================

void InputState::GetMouseDelta(ControlInfo * hidInput)
{
	hidInput->mouseturnx = g_mousePos.x * m_yaw * (1.f / 18.f);
	hidInput->mouseturny = g_mousePos.y * m_pitch * (1.f / 14.f);
	hidInput->mousemovex = g_mousePos.x * m_side;
	hidInput->mousemovey = g_mousePos.y * m_forward;

	g_mousePos = {};
}

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
	WeaponToSend = 0;
	dpad_lock = 0;

	if (gamestate != GS_LEVEL)
	{
		ActionsToSend = 0;
		crouch_toggle = false;
		gi->clearlocalinputstate();		// also clear game local input state.
	}
	else if (gamestate == GS_LEVEL && crouch_toggle)
	{
		ActionsToSend |= SB_CROUCH;
	}
	else
	{
		ActionsToSend = 0;
	}

	buttonMap.ResetButtonStates();	// this is important. If all input is cleared, the buttons must be cleared as well.
	resetTurnHeldAmt();
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
		setVideoMode();
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

ControlInfo CONTROL_GetInput()
{
	ControlInfo hidInput {};

	inputState.GetMouseDelta(&hidInput);

	if (use_joystick)
	{
		// Handle joysticks/game controllers.
		float joyaxes[NUM_JOYAXIS];

		I_GetAxes(joyaxes);

		hidInput.dyaw += -joyaxes[JOYAXIS_Yaw];
		hidInput.dx += joyaxes[JOYAXIS_Side] * .5f;
		hidInput.dz += joyaxes[JOYAXIS_Forward] * .5f;
		hidInput.dpitch += -joyaxes[JOYAXIS_Pitch];
	}

	return hidInput;
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

//==========================================================================
//
//
//
//==========================================================================

CCMD(slot)
{
	// The max differs between games so we have to handle this here.
	int max = (g_gameType & GAMEFLAG_PSEXHUMED) || (g_gameType & (GAMEFLAG_DUKE | GAMEFLAG_SHAREWARE)) == (GAMEFLAG_DUKE | GAMEFLAG_SHAREWARE) ? 7 : isBlood() ? 12 : 10;
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
	int max = (g_gameType & GAMEFLAG_PSEXHUMED)? 6 : isSWALL()? 7 : isBlood() ? 4 : 5;
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

CCMD(backoff)
{
	ActionsToSend |= SB_ESCAPE;
}

CCMD(pause)
{
	sendPause = true;
}

CCMD(warptocoords)
{
	if (netgame)
	{
		Printf("warptocoords cannot be used in multiplayer.\n");
		return;
	}
	if (argv.argc() < 4)
	{
		Printf("warptocoords [x] [y] [z] [ang] (optional) [horiz] (optional): warps the player to the specified coordinates\n");
		return;
	}
	if (gamestate != GS_LEVEL)
	{
		Printf("warptocoords: must be in a level\n");
		return;
	}
	int x = atoi(argv[1]);
	int y = atoi(argv[2]);
	int z = atoi(argv[3]);
	int ang = INT_MIN, horiz = INT_MIN;
	if (argv.argc() > 4)
	{
		ang = atoi(argv[4]);
	}
	if (argv.argc() > 5)
	{
		horiz = atoi(argv[5]);
	}

	gi->WarpToCoords(x, y, z, ang, horiz);
}

CCMD(third_person_view)
{
	gi->ToggleThirdPerson();
}

CCMD(coop_view)
{
	gi->SwitchCoopView();
}

CCMD(show_weapon)
{
	gi->ToggleShowWeapon();
}

void ApplyGlobalInput(InputPacket& input, ControlInfo* hidInput, bool const crouchable, bool const disableToggle)
{
	if (WeaponToSend != 0) input.setNewWeapon(WeaponToSend);
	WeaponToSend = 0;
	if (hidInput && buttonMap.ButtonDown(gamefunc_Dpad_Select))
	{
		// These buttons should not autorepeat. The game handlers are not really equipped for that.
		if (hidInput->dz > 0 && !(dpad_lock & 1)) { dpad_lock |= 1;  input.setNewWeapon(WeaponSel_Prev); }
		else dpad_lock &= ~1;
		if (hidInput->dz < 0 && !(dpad_lock & 2)) { dpad_lock |= 2;  input.setNewWeapon(WeaponSel_Next); }
		else dpad_lock &= ~2;
		if ((hidInput->dx < 0 || hidInput->dyaw < 0) && !(dpad_lock & 4)) { dpad_lock |= 4;  input.actions |= SB_INVPREV; }
		else dpad_lock &= ~4;
		if ((hidInput->dx > 0 || hidInput->dyaw > 0) && !(dpad_lock & 8)) { dpad_lock |= 8;  input.actions |= SB_INVNEXT; }
		else dpad_lock &= ~8;

		// This eats the controller input for regular use
		hidInput->dx = 0;
		hidInput->dz = 0;
		hidInput->dyaw = 0;
	}
	else dpad_lock = 0;

	input.actions |= ActionsToSend;
	ActionsToSend = 0;

	if (buttonMap.ButtonDown(gamefunc_Aim_Up) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && hidInput->dz > 0)) 
		input.actions |= SB_AIM_UP;

	if ((buttonMap.ButtonDown(gamefunc_Aim_Down) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && hidInput->dz < 0))) 
		input.actions |= SB_AIM_DOWN;

	if (buttonMap.ButtonDown(gamefunc_Dpad_Aiming))
		hidInput->dz = 0;

	if (buttonMap.ButtonDown(gamefunc_Jump))
		input.actions |= SB_JUMP;

	if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Toggle_Crouch) || crouch_toggle)
		input.actions |= SB_CROUCH;

	if (buttonMap.ButtonDown(gamefunc_Toggle_Crouch))
	{
		crouch_toggle = !crouch_toggle && crouchable;
		if (crouchable)	buttonMap.ClearButton(gamefunc_Toggle_Crouch);
	}

	if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Jump) || disableToggle)
		crouch_toggle = false;

	if (buttonMap.ButtonDown(gamefunc_Fire))
		input.actions |= SB_FIRE;

	if (buttonMap.ButtonDown(gamefunc_Alt_Fire))
		input.actions |= SB_ALTFIRE;

	if (buttonMap.ButtonDown(gamefunc_Open))
	{
		if (isBlood()) buttonMap.ClearButton(gamefunc_Open);
		input.actions |= SB_OPEN;
	}
	if (G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run)))
		input.actions |= SB_RUN;

	if (!in_mousemode && !buttonMap.ButtonDown(gamefunc_Mouse_Aiming)) 
		input.actions |= SB_AIMMODE;

	if (buttonMap.ButtonDown(gamefunc_Look_Up)) 
		input.actions |= SB_LOOK_UP;

	if (buttonMap.ButtonDown(gamefunc_Look_Down)) 
		input.actions |= SB_LOOK_DOWN;

	if (buttonMap.ButtonDown(gamefunc_Look_Left)) 
		input.actions |= SB_LOOK_LEFT;

	if (buttonMap.ButtonDown(gamefunc_Look_Right)) 
		input.actions |= SB_LOOK_RIGHT;
}

