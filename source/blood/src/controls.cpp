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
#include "controls.h"
#include "globals.h"
#include "levels.h"
#include "map2d.h"
#include "view.h"
#include "d_event.h"
#include "gamestate.h"
#include "sound.h"

BEGIN_BLD_NS

InputPacket gInput, gNetInput;
bool bSilentAim = false;

int iTurnCount = 0;
static int WeaponToSend;
static SYNCFLAGS BitsToSend;

void ctrlInit(void)
{
}

void ctrlTerm(void)
{
}

int32_t mouseyaxismode = -1;

int32_t GetTime(void)
{
    return gameclock;
}

fix16_t gViewLook, gViewAngle;
float gViewAngleAdjust;
float gViewLookAdjust;
int gViewLookRecenter;

void LocalKeys(void)
{
    if (buttonMap.ButtonDown(gamefunc_Third_Person_View))
    {
        buttonMap.ClearButton(gamefunc_Third_Person_View);
        if (gViewPos > VIEWPOS_0)
            gViewPos = VIEWPOS_0;
        else
            gViewPos = VIEWPOS_1;
    }
    if (buttonMap.ButtonDown(gamefunc_See_Coop_View))
    {
        buttonMap.ClearButton(gamefunc_See_Coop_View);
        if (gGameOptions.nGameType == 1)
        {
            gViewIndex = connectpoint2[gViewIndex];
            if (gViewIndex == -1)
                gViewIndex = connecthead;
            gView = &gPlayer[gViewIndex];
        }
        else if (gGameOptions.nGameType == 3)
        {
            int oldViewIndex = gViewIndex;
            do
            {
                gViewIndex = connectpoint2[gViewIndex];
                if (gViewIndex == -1)
                    gViewIndex = connecthead;
                if (oldViewIndex == gViewIndex || gMe->teamId == gPlayer[gViewIndex].teamId)
                    break;
            } while (oldViewIndex != gViewIndex);
            gView = &gPlayer[gViewIndex];
        }
    }
}



void ctrlGetInput(void)
{
    int prevPauseState = paused;
    ControlInfo info;

    static double lastInputTicks;
    auto const    currentHiTicks    = I_msTimeF();
    double const  elapsedInputTicks = currentHiTicks - lastInputTicks;

    lastInputTicks = currentHiTicks;

    auto scaleAdjustmentToInterval = [=](double x) { return x * kTicsPerSec / (1000.0 / elapsedInputTicks); };

    if (gamestate != GS_LEVEL || System_WantGuiCapture())
    {
        gInput = {};
        CONTROL_GetInput(&info);
        return;
    }

    if (paused != prevPauseState)
    {
        gInput.syncFlags.pause = 1;
    }

    if (paused)
        return;

    InputPacket input = {};

	bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);
	if (!mouseaim) gInput.syncFlags.lookCenter = 1;

    if (numplayers == 1)
    {
        gProfile[myconnectindex].nAutoAim = cl_autoaim;
        gProfile[myconnectindex].nWeaponSwitch = cl_weaponswitch;
    }

    CONTROL_GetInput(&info);

    if (gQuitRequest)
        gInput.syncFlags.quit = 1;

    gInput.syncFlags.value |= BitsToSend.value;
    ApplyGlobalInput(gInput, &info);

    BitsToSend.value = 0;
    WeaponToSend = 0;

    if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
    {
        if (automapMode != am_off)
        {
            gZoom = ClipLow(gZoom - (gZoom >> 4), 64);
            gViewMap.nZoom = gZoom;
        }
    }

    if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
    {
        if (automapMode != am_off)
        {
            gZoom = ClipHigh(gZoom + (gZoom >> 4), 4096);
            gViewMap.nZoom = gZoom;
        }
    }

    if (gPlayer[myconnectindex].nextWeapon == 0)
    {
    }

    if (buttonMap.ButtonDown(gamefunc_Show_Opponents_Weapon))
    {
        buttonMap.ClearButton(gamefunc_Show_Opponents_Weapon);
        cl_showweapon = (cl_showweapon + 1) & 3;
    }

    if (buttonMap.ButtonDown(gamefunc_Jump))
        gInput.syncFlags.jump = 1;

    if (buttonMap.ButtonDown(gamefunc_Crouch))
        gInput.syncFlags.crouch = 1;

    if (buttonMap.ButtonDown(gamefunc_Fire))
        gInput.syncFlags.shoot = 1;

    if (buttonMap.ButtonDown(gamefunc_Alt_Fire))
        gInput.syncFlags.shoot2 = 1;

    if (buttonMap.ButtonDown(gamefunc_Open))
    {
        buttonMap.ClearButton(gamefunc_Open);
        gInput.syncFlags.action = 1;
    }

    gInput.syncFlags.lookUp |= buttonMap.ButtonDown(gamefunc_Look_Up);
    gInput.syncFlags.lookDown |= buttonMap.ButtonDown(gamefunc_Look_Down);

    if (buttonMap.ButtonDown(gamefunc_Look_Up) || buttonMap.ButtonDown(gamefunc_Look_Down))
        gInput.syncFlags.lookCenter = 1;
    else
    {
        gInput.syncFlags.lookUp |= buttonMap.ButtonDown(gamefunc_Aim_Up);
        gInput.syncFlags.lookDown |= buttonMap.ButtonDown(gamefunc_Aim_Down);
    }

    int const run = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    int const run2 = false; // What??? buttonMap.ButtonDown(gamefunc_Run);
    int const keyMove = (1 + run) << 10;

    gInput.syncFlags.run |= run;

    if (gInput.fvel < keyMove && gInput.fvel > -keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Move_Forward))
            input.fvel += keyMove;

        if (buttonMap.ButtonDown(gamefunc_Move_Backward))
            input.fvel -= keyMove;
    }

    if (gInput.svel < keyMove && gInput.svel > -keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Strafe_Left))
            input.svel += keyMove;
        if (buttonMap.ButtonDown(gamefunc_Strafe_Right))
            input.svel -= keyMove;
    }


    char turnLeft = 0, turnRight = 0;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        if (gInput.svel < keyMove && gInput.svel > -keyMove)
        {
            if (buttonMap.ButtonDown(gamefunc_Turn_Left))
                input.svel += keyMove;
            if (buttonMap.ButtonDown(gamefunc_Turn_Right))
                input.svel -= keyMove;
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
    int32_t const  elapsedTics = gameclock - lastInputClock;

    // Blood's q16mlook scaling is different from the other games, therefore use the below constant to attenuate
    // the speed to match the other games.
    float const mlookScale = 3.25f;

    lastInputClock = gameclock;

    if (turnLeft || turnRight)
        turnHeldTime += elapsedTics;
    else
        turnHeldTime = 0;

    if (turnLeft)
        input.q16avel = fix16_ssub(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(ClipHigh(12 * turnHeldTime, gTurnSpeed)>>2)));
    if (turnRight)
        input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(ClipHigh(12 * turnHeldTime, gTurnSpeed)>>2)));

    if ((run2 || run) && turnHeldTime > 24)
        input.q16avel <<= 1;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        input.svel -= info.mousex * 32.f;
        input.svel -= scaleAdjustmentToInterval(info.dyaw * keyMove);
    }
    else
    {
        input.q16avel = fix16_sadd(input.q16avel, fix16_from_float(info.mousex));
        input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(info.dyaw)));
    }

    input.svel  -= scaleAdjustmentToInterval(info.dx * keyMove);
    input.fvel -= scaleAdjustmentToInterval(info.dz * keyMove);

    if (mouseaim)
        input.q16horz = fix16_sadd(input.q16horz, fix16_from_float(info.mousey / mlookScale));
    else
        input.fvel -= info.mousey * 64.f;
    if (!in_mouseflip)
        input.q16horz = -input.q16horz;

    input.q16horz = fix16_ssub(input.q16horz, fix16_from_dbl(scaleAdjustmentToInterval(info.dpitch / mlookScale)));

    if (!automapFollow && automapMode != am_off)
    {
        gViewMap.turn += input.q16avel<<2;
        gViewMap.forward += input.fvel;
        gViewMap.strafe += input.svel;
        input.q16avel = 0;
        input.fvel = 0;
        input.svel = 0;
    }
    gInput.fvel = clamp(gInput.fvel + input.fvel, -2048, 2048);
    gInput.svel = clamp(gInput.svel + input.svel, -2048, 2048);
    gInput.q16avel = fix16_sadd(gInput.q16avel, input.q16avel);
    gInput.q16horz = fix16_clamp(fix16_sadd(gInput.q16horz, input.q16horz), fix16_from_int(-127)>>2, fix16_from_int(127)>>2);
    if (gMe && gMe->pXSprite->health != 0 && !paused)
    {
        int upAngle = 289;
        int downAngle = -347;
        double lookStepUp = 4.0*upAngle/60.0;
        double lookStepDown = -4.0*downAngle/60.0;
        gViewAngle = (gViewAngle + input.q16avel + fix16_from_dbl(scaleAdjustmentToInterval(gViewAngleAdjust))) & 0x7ffffff;
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
        gViewLook = fix16_clamp(gViewLook+(input.q16horz << 3), fix16_from_int(downAngle), fix16_from_int(upAngle));
    }
}

//---------------------------------------------------------------------------
//
// CCMD based input. The basics are from Randi's ZDuke but this uses dynamic
// registration to only have the commands active when this game module runs.
//
//---------------------------------------------------------------------------

void registerinputcommands()
{
    C_RegisterFunction("pause", nullptr, [](CCmdFuncPtr)->int { BitsToSend.pause = 1; sendPause = true; return CCMD_OK; });
    C_RegisterFunction("centerview", nullptr, [](CCmdFuncPtr)->int { BitsToSend.lookCenter = 1; return CCMD_OK; });
    C_RegisterFunction("holsterweapon", nullptr, [](CCmdFuncPtr)->int { BitsToSend.holsterWeapon = 1; return CCMD_OK; });
    C_RegisterFunction("invprev", nullptr, [](CCmdFuncPtr)->int { BitsToSend.prevItem = 1; return CCMD_OK; });
    C_RegisterFunction("invnext", nullptr, [](CCmdFuncPtr)->int { BitsToSend.nextItem = 1; return CCMD_OK; });
    C_RegisterFunction("turnaround", nullptr, [](CCmdFuncPtr)->int { BitsToSend.spin180 = 1; return CCMD_OK; });
    C_RegisterFunction("invuse", nullptr, [](CCmdFuncPtr)->int { BitsToSend.useItem = 1; return CCMD_OK; });
}

// This is called from ImputState::ClearAllInput and resets all static state being used here.
void GameInterface::clearlocalinputstate()
{
    WeaponToSend = 0;
    BitsToSend.value = 0;
}


END_BLD_NS
