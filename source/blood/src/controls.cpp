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

InputPacket gInput;
bool bSilentAim = false;

int iTurnCount = 0;

int32_t mouseyaxismode = -1;

fixed_t gViewLook, gViewAngle;
float gViewAngleAdjust;
float gViewLookAdjust;
int gViewLookRecenter;

void GetInputInternal(InputPacket &inputParm, ControlInfo* const hidInput)
{
    int prevPauseState = paused;

    static double lastInputTicks;
    auto const    currentHiTicks    = I_msTimeF();
    double const  elapsedInputTicks = currentHiTicks - lastInputTicks;

    lastInputTicks = currentHiTicks;

    auto scaleAdjustmentToInterval = [=](double x) { return x * kTicsPerSec / (1000.0 / elapsedInputTicks); };

    InputPacket input = {};

    ApplyGlobalInput(inputParm, hidInput);

    bool mouseaim = !(inputParm.actions & SB_AIMMODE);
    if (!mouseaim) inputParm.actions |= SB_CENTERVIEW;

    if (gPlayer[myconnectindex].nextWeapon == 0)
    {
    }

    if (inputParm.actions & (SB_LOOK_UP|SB_LOOK_DOWN))
        inputParm.actions |= SB_CENTERVIEW;

    int const run = !!(inputParm.actions & SB_RUN);
    int const keyMove = (1 + run) << 10;

    if (inputParm.fvel < keyMove && inputParm.fvel > -keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Move_Forward))
            input.fvel += keyMove;

        if (buttonMap.ButtonDown(gamefunc_Move_Backward))
            input.fvel -= keyMove;
    }

    if (inputParm.svel < keyMove && inputParm.svel > -keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Strafe_Left))
            input.svel += keyMove;
        if (buttonMap.ButtonDown(gamefunc_Strafe_Right))
            input.svel -= keyMove;
    }


    char turnLeft = 0, turnRight = 0;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        if (inputParm.svel < keyMove && inputParm.svel > -keyMove)
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
    int32_t const  elapsedTics = gFrameClock - lastInputClock;

    // Blood's q16mlook scaling is different from the other games, therefore use the below constant to attenuate
    // the speed to match the other games.
    float const mlookScale = 3.25f;

    lastInputClock = gFrameClock;

    if (turnLeft || turnRight)
        turnHeldTime += elapsedTics;
    else
        turnHeldTime = 0;

    if (turnLeft)
        input.q16avel -= FloatToFixed(scaleAdjustmentToInterval(ClipHigh(12 * turnHeldTime, gTurnSpeed)>>2));
    if (turnRight)
        input.q16avel += FloatToFixed(scaleAdjustmentToInterval(ClipHigh(12 * turnHeldTime, gTurnSpeed)>>2));

    if (run && turnHeldTime > 24)
        input.q16avel <<= 1;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        input.svel -= hidInput->mousex * 32.f;
        input.svel -= scaleAdjustmentToInterval(hidInput->dyaw * keyMove);
    }
    else
    {
        input.q16avel += FloatToFixed(hidInput->mousex);
        input.q16avel += FloatToFixed(scaleAdjustmentToInterval(hidInput->dyaw));
    }

    input.svel  -= scaleAdjustmentToInterval(hidInput->dx * keyMove);
    input.fvel -= scaleAdjustmentToInterval(hidInput->dz * keyMove);

    if (mouseaim)
        input.q16horz += FloatToFixed(hidInput->mousey / mlookScale);
    else
        input.fvel -= hidInput->mousey * 64.f;
    if (!in_mouseflip)
        input.q16horz = -input.q16horz;

    input.q16horz -= FloatToFixed(scaleAdjustmentToInterval(hidInput->dpitch / mlookScale));

    if (!automapFollow && automapMode != am_off)
    {
        gViewMap.turn += input.q16avel<<2;
        gViewMap.forward += input.fvel;
        gViewMap.strafe += input.svel;
        input.q16avel = 0;
        input.fvel = 0;
        input.svel = 0;
    }
    inputParm.fvel = clamp(inputParm.fvel + input.fvel, -2048, 2048);
    inputParm.svel = clamp(inputParm.svel + input.svel, -2048, 2048);
    inputParm.q16avel += input.q16avel;
    inputParm.q16horz = clamp(inputParm.q16horz + input.q16horz, IntToFixed(-127)>>2, IntToFixed(127)>>2);
    if (gMe && gMe->pXSprite && gMe->pXSprite->health != 0 && !paused)
    {
        int upAngle = 289;
        int downAngle = -347;
        double lookStepUp = 4.0*upAngle/60.0;
        double lookStepDown = -4.0*downAngle/60.0;
        gViewAngle = (gViewAngle + input.q16avel + FloatToFixed(scaleAdjustmentToInterval(gViewAngleAdjust))) & 0x7ffffff;
        if (gViewLookRecenter)
        {
            if (gViewLook < 0)
                gViewLook = min(gViewLook+FloatToFixed(scaleAdjustmentToInterval(lookStepDown)), 0);
            if (gViewLook > 0)
                gViewLook = max(gViewLook-FloatToFixed(scaleAdjustmentToInterval(lookStepUp)), 0);
        }
        else
        {
            gViewLook = clamp(gViewLook+FloatToFixed(scaleAdjustmentToInterval(gViewLookAdjust)), IntToFixed(downAngle), IntToFixed(upAngle));
        }
        gViewLook = clamp(gViewLook+(input.q16horz << 3), IntToFixed(downAngle), IntToFixed(upAngle));
    }
}

void GameInterface::GetInput(InputPacket* packet, ControlInfo* const hidInput)
{
    GetInputInternal(gInput, hidInput);
    if (packet)
    {
        *packet = gInput;
        gInput = {};
    }
}

END_BLD_NS
