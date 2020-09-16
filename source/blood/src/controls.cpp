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
#include "globals.h"
#include "levels.h"
#include "view.h"
#include "d_event.h"
#include "gamestate.h"
#include "sound.h"

BEGIN_BLD_NS

static InputPacket gInput;

void applylook(PLAYER *pPlayer, fixed_t const q16avel, double const scaleAdjust);
void sethorizon(PLAYER *pPlayer, fixed_t const q16horz, double const scaleAdjust);

static void GetInputInternal(ControlInfo* const hidInput)
{
    double const scaleAdjust = InputScale();
    int prevPauseState = paused;

    InputPacket input = {};

    ApplyGlobalInput(gInput, hidInput);

    bool mouseaim = !(gInput.actions & SB_AIMMODE);
    if (!mouseaim) gInput.actions |= SB_CENTERVIEW;

    if (gPlayer[myconnectindex].nextWeapon == 0)
    {
    }

    if (gInput.actions & (SB_LOOK_UP|SB_LOOK_DOWN))
        gInput.actions |= SB_CENTERVIEW;

    int const run = !!(gInput.actions & SB_RUN);
    int const keyMove = (1 + run) << 10;

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
        input.q16avel -= FloatToFixed(scaleAdjust * (ClipHigh(12 * turnHeldTime, gTurnSpeed) >> 2));
    if (turnRight)
        input.q16avel += FloatToFixed(scaleAdjust * (ClipHigh(12 * turnHeldTime, gTurnSpeed) >> 2));

    if (run && turnHeldTime > 24)
        input.q16avel <<= 1;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        input.svel -= xs_CRoundToInt((hidInput->mousex * 32.) + (scaleAdjust * (hidInput->dyaw * keyMove)));
    }
    else
    {
        input.q16avel += FloatToFixed(hidInput->mousex + (scaleAdjust * (hidInput->dyaw)));
    }

    input.svel -= xs_CRoundToInt(scaleAdjust * (hidInput->dx * keyMove));
    input.fvel -= xs_CRoundToInt(scaleAdjust * (hidInput->dz * keyMove));

    if (mouseaim)
        input.q16horz += FloatToFixed(hidInput->mousey / mlookScale);
    else
        input.fvel -= xs_CRoundToInt(hidInput->mousey * 64.);

    if (!in_mouseflip)
        input.q16horz = -input.q16horz;

    input.q16horz -= FloatToFixed(scaleAdjust * (hidInput->dpitch / mlookScale));

    gInput.fvel = clamp(gInput.fvel + input.fvel, -2048, 2048);
    gInput.svel = clamp(gInput.svel + input.svel, -2048, 2048);
    gInput.q16avel += input.q16avel;
    gInput.q16horz = clamp(gInput.q16horz + input.q16horz, IntToFixed(-127) >> 2, IntToFixed(127) >> 2);

    if (!cl_syncinput && gamestate == GS_LEVEL)
    {
        applylook(&gPlayer[myconnectindex], input.q16avel, scaleAdjust);
        sethorizon(&gPlayer[myconnectindex], input.q16horz, scaleAdjust);
    }
}

void GameInterface::GetInput(InputPacket* packet, ControlInfo* const hidInput)
{
    GetInputInternal(hidInput);
    if (packet)
    {
        *packet = gInput;
        gInput = {};
    }
}

END_BLD_NS
