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
#include "mmulti.h"
#include "gamecontrol.h"
#include "common_game.h"
#include "blood.h"
#include "config.h"
#include "controls.h"
#include "globals.h"
#include "levels.h"
#include "map2d.h"
#include "view.h"
#include "d_event.h"

BEGIN_BLD_NS

GINPUT gInput, gNetInput;
bool bSilentAim = false;

int iTurnCount = 0;

void ctrlInit(void)
{
}

void ctrlTerm(void)
{
}

int32_t mouseyaxismode = -1;

int32_t GetTime(void)
{
    return (int32_t)totalclock;
}

void GameInterface::set_hud_layout(int layout)
{
    layout = clamp(7 - layout, 0, 7);   // need to reverse the order because menu sliders always have low values to the left.
	viewResizeView(layout);
}


fix16_t gViewLook, gViewAngle;
float gViewAngleAdjust;
float gViewLookAdjust;
int gViewLookRecenter;

void ctrlGetInput(void)
{
    int prevPauseState = paused;
    ControlInfo info;

    static double lastInputTicks;
    auto const    currentHiTicks    = timerGetHiTicks();
    double const  elapsedInputTicks = currentHiTicks - lastInputTicks;

    lastInputTicks = currentHiTicks;

    auto scaleAdjustmentToInterval = [=](double x) { return x * kTicsPerSec / (1000.0 / elapsedInputTicks); };

    if (!gGameStarted || gInputMode != kInputGame)
    {
        gInput = {};
        CONTROL_GetInput(&info);
        return;
    }

    updatePauseStatus();
    if (paused != prevPauseState)
    {
        gInput.keyFlags.pause = 1;
    }

    if (paused)
        return;

    GINPUT input = {};

	bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);
	if (!mouseaim) gInput.keyFlags.lookCenter = 1;

    if (numplayers == 1)
    {
        gProfile[myconnectindex].nAutoAim = cl_autoaim;
        gProfile[myconnectindex].nWeaponSwitch = cl_weaponswitch;
    }

    CONTROL_GetInput(&info);

    if (gQuitRequest)
        gInput.keyFlags.quit = 1;

    if (buttonMap.ButtonDown(gamefunc_Map))
    {
        buttonMap.ClearButton(gamefunc_Map);
        viewToggle(gViewMode);
    }

    if (buttonMap.ButtonDown(gamefunc_Map_Follow_Mode))
    {
        buttonMap.ClearButton(gamefunc_Map_Follow_Mode);
        gFollowMap = !gFollowMap;
        gViewMap.FollowMode(gFollowMap);
    }

    if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
    {
        if (gViewMode == 3)
        {
            buttonMap.ClearButton(gamefunc_Shrink_Screen);
			G_ChangeHudLayout(-1);
		}
        if (gViewMode == 2 || gViewMode == 4)
        {
            gZoom = ClipLow(gZoom - (gZoom >> 4), 64);
            gViewMap.nZoom = gZoom;
        }
    }

    if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
    {
        if (gViewMode == 3)
        {
            buttonMap.ClearButton(gamefunc_Enlarge_Screen);
			G_ChangeHudLayout(1);
        }
        if (gViewMode == 2 || gViewMode == 4)
        {
            gZoom = ClipHigh(gZoom + (gZoom >> 4), 4096);
            gViewMap.nZoom = gZoom;
        }
    }

    if (buttonMap.ButtonDown(gamefunc_Toggle_Crosshair))
    {
        buttonMap.ClearButton(gamefunc_Toggle_Crosshair);
        cl_crosshair = !cl_crosshair;
    }

    if (gPlayer[myconnectindex].nextWeapon == 0)
    {
        if (buttonMap.ButtonPressed(gamefunc_Next_Weapon))
        {
            buttonMap.ClearButton(gamefunc_Next_Weapon);
            gInput.keyFlags.nextWeapon = 1;
        }

        if (buttonMap.ButtonPressed(gamefunc_Previous_Weapon))
        {
            buttonMap.ClearButton(gamefunc_Previous_Weapon);
            gInput.keyFlags.prevWeapon = 1;
        }
    }

    if (buttonMap.ButtonDown(gamefunc_Show_Opponents_Weapon))
    {
        buttonMap.ClearButton(gamefunc_Show_Opponents_Weapon);
        cl_showweapon = (cl_showweapon + 1) & 3;
    }

    if (buttonMap.ButtonDown(gamefunc_Jump))
        gInput.buttonFlags.jump = 1;

    if (buttonMap.ButtonDown(gamefunc_Crouch))
        gInput.buttonFlags.crouch = 1;

    if (buttonMap.ButtonDown(gamefunc_Fire))
        gInput.buttonFlags.shoot = 1;

    if (buttonMap.ButtonDown(gamefunc_Alt_Fire))
        gInput.buttonFlags.shoot2 = 1;

    if (buttonMap.ButtonDown(gamefunc_Open))
    {
        buttonMap.ClearButton(gamefunc_Open);
        gInput.keyFlags.action = 1;
    }

    gInput.buttonFlags.lookUp |= buttonMap.ButtonDown(gamefunc_Look_Up);
    gInput.buttonFlags.lookDown |= buttonMap.ButtonDown(gamefunc_Look_Down);

    if (buttonMap.ButtonDown(gamefunc_Look_Up) || buttonMap.ButtonDown(gamefunc_Look_Down))
        gInput.keyFlags.lookCenter = 1;
    else
    {
        gInput.buttonFlags.lookUp |= buttonMap.ButtonDown(gamefunc_Aim_Up);
        gInput.buttonFlags.lookDown |= buttonMap.ButtonDown(gamefunc_Aim_Down);
    }

    if (buttonMap.ButtonDown(gamefunc_Center_View))
    {
        buttonMap.ClearButton(gamefunc_Center_View);
        gInput.keyFlags.lookCenter = 1;
    }

    gInput.keyFlags.spin180 |= buttonMap.ButtonDown(gamefunc_TurnAround);

    if (buttonMap.ButtonDown(gamefunc_Inventory_Left))
    {
        buttonMap.ClearButton(gamefunc_Inventory_Left);
        gInput.keyFlags.prevItem = 1;
    }

    if (buttonMap.ButtonDown(gamefunc_Inventory_Right))
    {
        buttonMap.ClearButton(gamefunc_Inventory_Right);
        gInput.keyFlags.nextItem = 1;
    }

    if (buttonMap.ButtonDown(gamefunc_Inventory))
    {
        buttonMap.ClearButton(gamefunc_Inventory);
        gInput.keyFlags.useItem = 1;
    }

    if (buttonMap.ButtonDown(gamefunc_BeastVision))
    {
        buttonMap.ClearButton(gamefunc_BeastVision);
        gInput.useFlags.useBeastVision = 1;
    }

    if (buttonMap.ButtonDown(gamefunc_CrystalBall))
    {
        buttonMap.ClearButton(gamefunc_CrystalBall);
        gInput.useFlags.useCrystalBall = 1;
    }

    if (buttonMap.ButtonDown(gamefunc_JumpBoots))
    {
        buttonMap.ClearButton(gamefunc_JumpBoots);
        gInput.useFlags.useJumpBoots = 1;
    }

    if (buttonMap.ButtonDown(gamefunc_MedKit))
    {
        buttonMap.ClearButton(gamefunc_MedKit);
        gInput.useFlags.useMedKit = 1;
    }

    for (int i = 0; i < 10; i++)
    {
        if (buttonMap.ButtonDown(gamefunc_Weapon_1 + i))
        {
            buttonMap.ClearButton(gamefunc_Weapon_1 + i);
            gInput.newWeapon = 1 + i;
        }
    }

    if (buttonMap.ButtonDown(gamefunc_ProximityBombs))
    {
        buttonMap.ClearButton(gamefunc_ProximityBombs);
        gInput.newWeapon = 11;
    }

    if (buttonMap.ButtonDown(gamefunc_RemoteBombs))
    {
        buttonMap.ClearButton(gamefunc_RemoteBombs);
        gInput.newWeapon = 12;
    }

    if (buttonMap.ButtonDown(gamefunc_Holster_Weapon))
    {
        buttonMap.ClearButton(gamefunc_Holster_Weapon);
        gInput.keyFlags.holsterWeapon = 1;
    }

    int const run = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    int const run2 = false; // What??? buttonMap.ButtonDown(gamefunc_Run);
    int const keyMove = (1 + run) << 10;

    gInput.syncFlags.run |= run;

    if (gInput.forward < keyMove && gInput.forward > -keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Move_Forward))
            input.forward += keyMove;

        if (buttonMap.ButtonDown(gamefunc_Move_Backward))
            input.forward -= keyMove;
    }

    if (gInput.strafe < keyMove && gInput.strafe > -keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Strafe_Left))
            input.strafe += keyMove;
        if (buttonMap.ButtonDown(gamefunc_Strafe_Right))
            input.strafe -= keyMove;
    }


    char turnLeft = 0, turnRight = 0;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        if (gInput.strafe < keyMove && gInput.strafe > -keyMove)
        {
            if (buttonMap.ButtonDown(gamefunc_Turn_Left))
                input.strafe += keyMove;
            if (buttonMap.ButtonDown(gamefunc_Turn_Right))
                input.strafe -= keyMove;
        }
    }
    else
    {
        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
            turnLeft = 1;
        if (buttonMap.ButtonDown(gamefunc_Turn_Right))
            turnRight = 1;
    }

    static int32_t turnHeldTime;
    static int32_t lastInputClock;  // MED
    int32_t const  elapsedTics = (int32_t)totalclock - lastInputClock;

    // Blood's q16mlook scaling is different from the other games, therefore use the below constant to attenuate
    // the speed to match the other games.
    float const mlookScale = 3.25f;

    lastInputClock = (int32_t) totalclock;

    if (turnLeft || turnRight)
        turnHeldTime += elapsedTics;
    else
        turnHeldTime = 0;

    if (turnLeft)
        input.q16turn = fix16_ssub(input.q16turn, fix16_from_dbl(scaleAdjustmentToInterval(ClipHigh(12 * turnHeldTime, gTurnSpeed)>>2)));
    if (turnRight)
        input.q16turn = fix16_sadd(input.q16turn, fix16_from_dbl(scaleAdjustmentToInterval(ClipHigh(12 * turnHeldTime, gTurnSpeed)>>2)));

    if ((run2 || run) && turnHeldTime > 24)
        input.q16turn <<= 1;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        input.strafe -= info.mousex * 32.f;
        input.strafe -= scaleAdjustmentToInterval(info.dyaw * keyMove);
    }
    else
    {
        input.q16turn = fix16_sadd(input.q16turn, fix16_from_float(info.mousex));
        input.q16turn = fix16_sadd(input.q16turn, fix16_from_dbl(scaleAdjustmentToInterval(info.dyaw)));
    }

    input.strafe  -= scaleAdjustmentToInterval(info.dx * keyMove);
    input.forward -= scaleAdjustmentToInterval(info.dz * keyMove);

    if (mouseaim)
        input.q16mlook = fix16_sadd(input.q16mlook, fix16_from_float(info.mousey / mlookScale));
    else
        input.forward -= info.mousey * 64.f;
    if (!in_mouseflip)
        input.q16mlook = -input.q16mlook;

    input.q16mlook = fix16_ssub(input.q16mlook, fix16_from_dbl(scaleAdjustmentToInterval(info.dpitch / mlookScale)));

    if (!gViewMap.bFollowMode && gViewMode == 4)
    {
        gViewMap.turn += input.q16turn<<2;
        gViewMap.forward += input.forward;
        gViewMap.strafe += input.strafe;
        input.q16turn = 0;
        input.forward = 0;
        input.strafe = 0;
    }
    gInput.forward = clamp(gInput.forward + input.forward, -2048, 2048);
    gInput.strafe = clamp(gInput.strafe + input.strafe, -2048, 2048);
    gInput.q16turn = fix16_sadd(gInput.q16turn, input.q16turn);
    gInput.q16mlook = fix16_clamp(fix16_sadd(gInput.q16mlook, input.q16mlook), fix16_from_int(-127)>>2, fix16_from_int(127)>>2);
    if (gMe && gMe->pXSprite->health != 0 && !paused)
    {
        int upAngle = 289;
        int downAngle = -347;
        double lookStepUp = 4.0*upAngle/60.0;
        double lookStepDown = -4.0*downAngle/60.0;
        gViewAngle = (gViewAngle + input.q16turn + fix16_from_dbl(scaleAdjustmentToInterval(gViewAngleAdjust))) & 0x7ffffff;
        if (gViewLookRecenter)
        {
            if (gViewLook < 0)
                gViewLook = fix16_min(gViewLook+fix16_from_dbl(scaleAdjustmentToInterval(lookStepDown)), fix16_from_int(0));
            if (gViewLook > 0)
                gViewLook = fix16_max(gViewLook-fix16_from_dbl(scaleAdjustmentToInterval(lookStepUp)), fix16_from_int(0));
        }
        else
        {
            gViewLook = fix16_clamp(gViewLook+fix16_from_dbl(scaleAdjustmentToInterval(gViewLookAdjust)), fix16_from_int(downAngle), fix16_from_int(upAngle));
        }
        gViewLook = fix16_clamp(gViewLook+(input.q16mlook << 3), fix16_from_int(downAngle), fix16_from_int(upAngle));
    }
}

#if 0
if (gGameStarted && gInputMode != kInputMessage
    && buttonMap.ButtonDown(gamefunc_SendMessage))
{
    buttonMap.ClearButton(gamefunc_SendMessage);
    inputState.keyFlushScans();
    gInputMode = kInputMessage;
}

#endif

END_BLD_NS
