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

InputPacket gInput, gNetInput;
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
    return gameclock;
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

    if (paused)
        return;

    InputPacket input = {};

    if (numplayers == 1)
    {
        gProfile[myconnectindex].nAutoAim = cl_autoaim;
        gProfile[myconnectindex].nWeaponSwitch = cl_weaponswitch;
    }

    CONTROL_GetInput(&info);

    ApplyGlobalInput(gInput, &info);

    bool mouseaim = !(gInput.actions & SB_AIMMODE);
    if (!mouseaim) gInput.actions |= SB_CENTERVIEW;

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

    if (run && turnHeldTime > 24)
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

END_BLD_NS
