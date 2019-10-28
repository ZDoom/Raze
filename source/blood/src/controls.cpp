//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "ns.h"	// Must come before everything else!

#include "compat.h"
#include "baselayer.h"
#include "keyboard.h"
#include "mouse.h"
#include "joystick.h"
#include "control.h"
#include "gamecontrol.h"
#include "common_game.h"
#include "blood.h"
#include "config.h"
#include "controls.h"
#include "globals.h"
#include "levels.h"
#include "map2d.h"
#include "view.h"

BEGIN_BLD_NS

int32_t ctrlCheckAllInput(void)
{
    return (
        KB_KeyWaiting() ||
        MOUSE_GetButtons() ||
        JOYSTICK_GetButtons()
        );
}

void ctrlClearAllInput(void)
{
    KB_FlushKeyboardQueue();
    KB_ClearKeysDown();
    MOUSE_ClearAllButtons();
    JOYSTICK_ClearAllButtons();
}

GINPUT gInput;
bool bSilentAim = false;

int iTurnCount = 0;

int32_t GetTime(void)
{
    return (int32_t)totalclock;
}

void ctrlInit(void)
{
    KB_ClearKeysDown();
    KB_FlushKeyboardQueue();
    KB_FlushKeyboardQueueScans();
    CONFIG_SetupMouse();
    CONFIG_SetupJoystick();

    CONTROL_JoystickEnabled = (in_joystick && CONTROL_JoyPresent);
    CONTROL_MouseEnabled = (in_mouse && CONTROL_MousePresent);
    CONTROL_SmoothMouse = in_mousesmoothing;

    // JBF 20040215: evil and nasty place to do this, but joysticks are evil and nasty too
    for (int i = 0; i < joystick.numAxes; i++)
        joySetDeadZone(i, JoystickAnalogueDead[i], JoystickAnalogueSaturate[i]);
}

void ctrlTerm(void)
{
    CONTROL_Shutdown();
}

int32_t mouseyaxismode = -1;



// This is mainly here to account for the different implementation between Blood and the other games
// and to allow unified handling and the same value range in the CVAR code.
// Unlike EDuke's version, NBlood's was actually fine, it just had a too small value range to be chosen as the unified version.
bool validate_hud(int layout)
{
	return layout > 3 && layout != 8;	// 8 is the status bar overlay which NBlood did not implement.
}

void set_hud_layout(int layout)
{
	static const uint8_t screen_size_vals[] = { 7, 7, 7, 7, 6, 5, 4, 3, 3, 2, 1, 0 };

	if (validate_hud(layout))
	{
		viewResizeView(screen_size_vals[layout]);
	}
}

void set_hud_scale(int scale)
{
	// Not implemented, only needed as a placeholder. Maybe implement it after all? The hud is a bit large at its default size.
}


void ctrlGetInput(void)
{
    ControlInfo info;
    int forward = 0, strafe = 0;
    fix16_t turn = 0;
    memset(&gInput, 0, sizeof(gInput));

    if (!gGameStarted || gInputMode != kInputGame)
    {
        CONTROL_GetInput(&info);
        return;
    }

    CONTROL_ProcessBinds();

    if (in_aimmode)
        g_MyAimMode = 0;

    if (BUTTON(gamefunc_Mouse_Aiming))
    {
        if (in_aimmode)
            g_MyAimMode = 1;
        else
        {
            inputState.ClearButton(gamefunc_Mouse_Aiming);
            g_MyAimMode = !g_MyAimMode;
            if (g_MyAimMode)
            {
                if (!bSilentAim)
                    viewSetMessage("Mouse aiming ON");
            }
            else
            {
                if (!bSilentAim)
                    viewSetMessage("Mouse aiming OFF");
                gInput.keyFlags.lookCenter = 1;
            }
        }
    }
    else if (in_aimmode)
        gInput.keyFlags.lookCenter = 1;

    CONTROL_GetInput(&info);

    if (in_mousedeadzone)
    {
        if (info.mousey > 0)
            info.mousey = max(info.mousey - in_mousedeadzone, 0);
        else if (info.mousey < 0)
            info.mousey = min(info.mousey + in_mousedeadzone, 0);

        if (info.mousex > 0)
            info.mousex = max(info.mousex - in_mousedeadzone, 0);
        else if (info.mousex < 0)
            info.mousex = min(info.mousex + in_mousedeadzone, 0);
    }

    if (in_mousebias)
    {
        if (klabs(info.mousex) > klabs(info.mousey))
            info.mousey = tabledivide32_noinline(info.mousey, in_mousebias);
        else
            info.mousex = tabledivide32_noinline(info.mousex, in_mousebias);
    }

    if (gQuitRequest)
        gInput.keyFlags.quit = 1;

    if (gGameStarted && gInputMode != kInputMessage && gInputMode != kInputMenu
        && BUTTON(gamefunc_Send_Message))
    {
        inputState.ClearButton(gamefunc_Send_Message);
        keyFlushScans();
        gInputMode = kInputMessage;
    }

    if (BUTTON(gamefunc_AutoRun))
    {
        inputState.ClearButton(gamefunc_AutoRun);
        gAutoRun = !gAutoRun;
        if (gAutoRun)
            viewSetMessage("Auto run ON");
        else
            viewSetMessage("Auto run OFF");
    }

    if (BUTTON(gamefunc_Map_Toggle))
    {
        inputState.ClearButton(gamefunc_Map_Toggle);
        viewToggle(gViewMode);
    }

    if (BUTTON(gamefunc_Map_Follow_Mode))
    {
        inputState.ClearButton(gamefunc_Map_Follow_Mode);
        gFollowMap = !gFollowMap;
        gViewMap.FollowMode(gFollowMap);
    }

    if (BUTTON(gamefunc_Shrink_Screen))
    {
        if (gViewMode == 3)
        {
            inputState.ClearButton(gamefunc_Shrink_Screen);
			G_ChangeHudLayout(-1);
		}
        if (gViewMode == 2 || gViewMode == 4)
        {
            gZoom = ClipLow(gZoom - (gZoom >> 4), 64);
            gViewMap.nZoom = gZoom;
        }
    }

    if (BUTTON(gamefunc_Enlarge_Screen))
    {
        if (gViewMode == 3)
        {
            inputState.ClearButton(gamefunc_Enlarge_Screen);
			G_ChangeHudLayout(1);
        }
        if (gViewMode == 2 || gViewMode == 4)
        {
            gZoom = ClipHigh(gZoom + (gZoom >> 4), 4096);
            gViewMap.nZoom = gZoom;
        }
    }

    if (BUTTON(gamefunc_Toggle_Crosshair))
    {
        inputState.ClearButton(gamefunc_Toggle_Crosshair);
        cl_crosshair = !cl_crosshair;
    }

    if (BUTTON(gamefunc_Next_Weapon))
    {
        inputState.ClearButton(gamefunc_Next_Weapon);
        gInput.keyFlags.nextWeapon = 1;
    }

    if (BUTTON(gamefunc_Previous_Weapon))
    {
        inputState.ClearButton(gamefunc_Previous_Weapon);
        gInput.keyFlags.prevWeapon = 1;
    }

    if (BUTTON(gamefunc_Show_Opponents_Weapon))
    {
        inputState.ClearButton(gamefunc_Show_Opponents_Weapon);
        cl_showweapon = (cl_showweapon + 1) & 3;
    }

    if (BUTTON(gamefunc_Jump))
        gInput.buttonFlags.jump = 1;

    if (BUTTON(gamefunc_Crouch))
        gInput.buttonFlags.crouch = 1;

    if (BUTTON(gamefunc_Weapon_Fire))
        gInput.buttonFlags.shoot = 1;

    if (BUTTON(gamefunc_Weapon_Special_Fire))
        gInput.buttonFlags.shoot2 = 1;

    if (BUTTON(gamefunc_Open))
    {
        inputState.ClearButton(gamefunc_Open);
        gInput.keyFlags.action = 1;
    }

    gInput.buttonFlags.lookUp = BUTTON(gamefunc_Look_Up);
    gInput.buttonFlags.lookDown = BUTTON(gamefunc_Look_Down);

    if (gInput.buttonFlags.lookUp || gInput.buttonFlags.lookDown)
        gInput.keyFlags.lookCenter = 1;
    else
    {
        gInput.buttonFlags.lookUp = BUTTON(gamefunc_Aim_Up);
        gInput.buttonFlags.lookDown = BUTTON(gamefunc_Aim_Down);
    }

    if (BUTTON(gamefunc_Aim_Center))
    {
        inputState.ClearButton(gamefunc_Aim_Center);
        gInput.keyFlags.lookCenter = 1;
    }

    gInput.keyFlags.spin180 = BUTTON(gamefunc_Turn_Around);

    if (BUTTON(gamefunc_Inventory_Left))
    {
        inputState.ClearButton(gamefunc_Inventory_Left);
        gInput.keyFlags.prevItem = 1;
    }

    if (BUTTON(gamefunc_Inventory_Right))
    {
        inputState.ClearButton(gamefunc_Inventory_Right);
        gInput.keyFlags.nextItem = 1;
    }

    if (BUTTON(gamefunc_Inventory_Use))
    {
        inputState.ClearButton(gamefunc_Inventory_Use);
        gInput.keyFlags.useItem = 1;
    }

    if (BUTTON(gamefunc_BeastVision))
    {
        inputState.ClearButton(gamefunc_BeastVision);
        gInput.useFlags.useBeastVision = 1;
    }

    if (BUTTON(gamefunc_CrystalBall))
    {
        inputState.ClearButton(gamefunc_CrystalBall);
        gInput.useFlags.useCrystalBall = 1;
    }

    if (BUTTON(gamefunc_JumpBoots))
    {
        inputState.ClearButton(gamefunc_JumpBoots);
        gInput.useFlags.useJumpBoots = 1;
    }

    if (BUTTON(gamefunc_MedKit))
    {
        inputState.ClearButton(gamefunc_MedKit);
        gInput.useFlags.useMedKit = 1;
    }

    for (int i = 0; i < 10; i++)
    {
        if (BUTTON(gamefunc_Weapon_1 + i))
        {
            inputState.ClearButton(gamefunc_Weapon_1 + i);
            gInput.newWeapon = 1 + i;
        }
    }

    if (BUTTON(gamefunc_ProximityBombs))
    {
        inputState.ClearButton(gamefunc_ProximityBombs);
        gInput.newWeapon = 11;
    }

    if (BUTTON(gamefunc_RemoteBombs))
    {
        inputState.ClearButton(gamefunc_RemoteBombs);
        gInput.newWeapon = 12;
    }

    if (BUTTON(gamefunc_Holster_Weapon))
    {
        inputState.ClearButton(gamefunc_Holster_Weapon);
        gInput.keyFlags.holsterWeapon = 1;
    }

    char run = G_CheckAutorun(BUTTON(gamefunc_Run));
	char run2 = false; // What??? BUTTON(gamefunc_Run);

    gInput.syncFlags.run = run;

    if (BUTTON(gamefunc_Move_Forward))
        forward += (1+run)<<10;

    if (BUTTON(gamefunc_Move_Backward))
        forward -= (1+run)<<10;

    char turnLeft = 0, turnRight = 0;

    if (BUTTON(gamefunc_Strafe))
    {
        if (BUTTON(gamefunc_Turn_Left))
            strafe += (1 + run) << 10;
        if (BUTTON(gamefunc_Turn_Right))
            strafe -= (1 + run) << 10;
    }
    else
    {
        if (BUTTON(gamefunc_Strafe_Left))
            strafe += (1 + run) << 10;
        if (BUTTON(gamefunc_Strafe_Right))
            strafe -= (1 + run) << 10;
        if (BUTTON(gamefunc_Turn_Left))
            turnLeft = 1;
        if (BUTTON(gamefunc_Turn_Right))
            turnRight = 1;
    }

    if (turnLeft || turnRight)
        iTurnCount += 4;
    else
        iTurnCount = 0;

    if (turnLeft)
        turn -= fix16_from_int(ClipHigh(12 * iTurnCount, gTurnSpeed))>>2;
    if (turnRight)
        turn += fix16_from_int(ClipHigh(12 * iTurnCount, gTurnSpeed))>>2;

    if ((run2 || run) && iTurnCount > 24)
        turn <<= 1;

    if (BUTTON(gamefunc_Strafe))
        strafe = ClipRange(strafe - info.mousex, -2048, 2048);
    else
        turn = fix16_clamp(turn + fix16_div(fix16_from_int(info.mousex), F16(32)), F16(-1024)>>2, F16(1024)>>2);

    strafe = ClipRange(strafe-(info.dx<<5), -2048, 2048);

#if 0
    if (info.dz < 0)
        gInput.mlook = ClipRange((info.dz+127)>>7, -127, 127);
    else
        gInput.mlook = ClipRange(info.dz>>7, -127, 127);
#endif
    if (g_MyAimMode)
        gInput.q16mlook = fix16_clamp(fix16_div(fix16_from_int(info.mousey), F16(128)), F16(-127)>>2, F16(127)>>2);
    else
        forward = ClipRange(forward - info.mousey, -2048, 2048);
    if (!in_mouseflip)
        gInput.q16mlook = -gInput.q16mlook;

    if (KB_KeyPressed(sc_Pause)) // 0xc5 in disassembly
    {
        gInput.keyFlags.pause = 1;
        KB_ClearKeyDown(sc_Pause);
    }

    if (!gViewMap.bFollowMode && gViewMode == 4)
    {
        gViewMap.turn = fix16_to_int(turn<<2);
        gViewMap.forward = forward>>8;
        gViewMap.strafe = strafe>>8;
        turn = 0;
        forward = 0;
        strafe = 0;
    }
    gInput.forward = forward;
    gInput.q16turn = turn;
    gInput.strafe = strafe;
}

END_BLD_NS
