//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "ps_input.h"
#include "engine.h"
#include "exhumed.h"
#include "player.h"
#include "aistuff.h"
#include "status.h"
#include "view.h"
#include "gamecontrol.h"
#include <string.h>

BEGIN_PS_NS

extern short bPlayerPan;
extern short bLockPan;

short nInputStack = 0;

short bStackNode[kMaxPlayers];

short nTypeStack[kMaxPlayers];
PlayerInput sPlayerInput[kMaxPlayers];

int *pStackPtr;

// (nInputStack * 32) - 11;

void PushInput(PlayerInput *pInput, int edx)
{
    if (!bStackNode[edx])
    {
//		memcpy(sInputStack[nInputStack], pInput,
    }
}

int PopInput()
{
    if (!nInputStack)
        return -1;

    nInputStack--;

    // TEMP
    return 0;
}

void InitInput()
{
    memset(nTypeStack, 0, sizeof(nTypeStack));
    nInputStack = 0;
    memset(bStackNode, 0, sizeof(bStackNode));

//	pStackPtr = &sInputStack;
}

void ClearSpaceBar(short nPlayer)
{
    sPlayerInput[nPlayer].actions &= SB_OPEN;
    buttonMap.ClearButton(gamefunc_Open);
}


void BackupInput()
{

}

void SendInput()
{

}

static int32_t nonsharedtimer;

void CheckKeys2()
{
    if (automapMode != am_off)
    {
        int const timerOffset = (gameclock - nonsharedtimer);
        nonsharedtimer += timerOffset;

        if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
            lMapZoom += mulscale6(timerOffset, max<int>(lMapZoom, 256));

        if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
            lMapZoom -= mulscale6(timerOffset, max<int>(lMapZoom, 256));

        lMapZoom = clamp(lMapZoom, 48, 2048);
    }

    if (PlayerList[nLocalPlayer].nHealth <= 0)
    {
        SetAirFrame();
    }
}


void PlayerInterruptKeys(bool after)
{
    ControlInfo info;
    memset(&info, 0, sizeof(ControlInfo)); // this is done within CONTROL_GetInput() anyway
    CONTROL_GetInput(&info);

    static double lastInputTicks;
    auto const    currentHiTicks = I_msTimeF();
    double const  elapsedInputTicks = currentHiTicks - lastInputTicks;

    lastInputTicks = currentHiTicks;

    auto scaleAdjustmentToInterval = [=](double x) { return x * (120 / 4) / (1000.0 / elapsedInputTicks); };

    if (paused)
        return;

    InputPacket tempinput{};
    fix16_t input_angle = 0;

    if (PlayerList[nLocalPlayer].nHealth == 0)
    {
        localInput = {};
        lPlayerYVel = 0;
        lPlayerXVel = 0;
        nPlayerDAng = 0;
        return;
    }

    if (!after)
    {
        localInput = {};
        ApplyGlobalInput(localInput, &info);
        if (PlayerList[nLocalPlayer].nHealth == 0) localInput.actions &= ~(SB_FIRE | SB_JUMP | SB_CROUCH);
    }


    // JBF: Run key behaviour is selectable
    int const playerRunning = !!(localInput.actions & SB_RUN);
    int const turnAmount = playerRunning ? 12 : 8;
    int const keyMove = playerRunning ? 12 : 6;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        tempinput.svel -= info.mousex * 4.f;
        tempinput.svel -= info.dyaw * keyMove;
    }
    else
    {
        input_angle = fix16_sadd(input_angle, fix16_from_float(info.mousex));
        input_angle = fix16_sadd(input_angle, fix16_from_dbl(scaleAdjustmentToInterval(info.dyaw)));
    }

    bool mouseaim = !(localInput.actions & SB_AIMMODE);

    if (mouseaim)
        tempinput.q16horz = fix16_sadd(tempinput.q16horz, fix16_from_float(info.mousey));
    else
        tempinput.fvel -= info.mousey * 8.f;

    if (!in_mouseflip) tempinput.q16horz = -tempinput.q16horz;

    tempinput.q16horz = fix16_ssub(tempinput.q16horz, fix16_from_dbl(scaleAdjustmentToInterval(info.dpitch)));
    tempinput.svel -= info.dx * keyMove;
    tempinput.fvel -= info.dz * keyMove;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
            tempinput.svel -= -keyMove;

        if (buttonMap.ButtonDown(gamefunc_Turn_Right))
            tempinput.svel -= keyMove;
    }
    else
    {
        static int turn = 0;
        static int counter = 0;
        // normal, non strafing movement
        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
        {
            turn -= 2;

            if (turn < -turnAmount)
                turn = -turnAmount;
        }
        else if (buttonMap.ButtonDown(gamefunc_Turn_Right))
        {
            turn += 2;

            if (turn > turnAmount)
                turn = turnAmount;
        }

        if (turn < 0)
        {
            turn++;
            if (turn > 0)
                turn = 0;
        }

        if (turn > 0)
        {
            turn--;
            if (turn < 0)
                turn = 0;
        }

        //if ((counter++) % 4 == 0) // what was this for???
        input_angle = fix16_sadd(input_angle, fix16_from_dbl(scaleAdjustmentToInterval(turn * 2)));

    }

    if (buttonMap.ButtonDown(gamefunc_Strafe_Left))
        tempinput.svel += keyMove;

    if (buttonMap.ButtonDown(gamefunc_Strafe_Right))
        tempinput.svel += -keyMove;

    if (buttonMap.ButtonDown(gamefunc_Move_Forward))
        tempinput.fvel += keyMove;

    if (buttonMap.ButtonDown(gamefunc_Move_Backward))
        tempinput.fvel += -keyMove;

    localInput.fvel = clamp(localInput.fvel + tempinput.fvel, -12, 12);
    localInput.svel = clamp(localInput.svel + tempinput.svel, -12, 12);

    localInput.q16avel = fix16_sadd(localInput.q16avel, input_angle);

    if (!nFreeze)
    {
        PlayerList[nLocalPlayer].q16angle = fix16_sadd(PlayerList[nLocalPlayer].q16angle, input_angle) & 0x7FFFFFF;

        // A horiz diff of 128 equal 45 degrees,
        // so we convert horiz to 1024 angle units

        float const horizAngle = clamp(atan2f(PlayerList[nLocalPlayer].q16horiz - fix16_from_int(92), fix16_from_int(128)) * (512.f / fPI) + fix16_to_float(tempinput.q16horz), -255.f, 255.f);
        PlayerList[nLocalPlayer].q16horiz = fix16_from_int(92) + Blrintf(fix16_from_int(128) * tanf(horizAngle * (fPI / 512.f)));

        // Look/aim up/down functions.
        if (localInput.actions & (SB_LOOK_UP|SB_AIM_UP))
        {
            bLockPan = false;
            if (PlayerList[nLocalPlayer].q16horiz < fix16_from_int(180)) {
                PlayerList[nLocalPlayer].q16horiz = fix16_sadd(PlayerList[nLocalPlayer].q16horiz, fix16_from_dbl(scaleAdjustmentToInterval(4)));
            }

            bPlayerPan = true;
            nDestVertPan[nLocalPlayer] = PlayerList[nLocalPlayer].q16horiz;
        }
        else if (localInput.actions & (SB_LOOK_DOWN|SB_AIM_DOWN))
        {
            bLockPan = false;
            if (PlayerList[nLocalPlayer].q16horiz > fix16_from_int(4)) {
                PlayerList[nLocalPlayer].q16horiz = fix16_ssub(PlayerList[nLocalPlayer].q16horiz, fix16_from_dbl(scaleAdjustmentToInterval(4)));
            }

            bPlayerPan = true;
            nDestVertPan[nLocalPlayer] = PlayerList[nLocalPlayer].q16horiz;
        }
    }

    // loc_1C048:
    if (totalvel[nLocalPlayer] > 20) {
        bPlayerPan = false;
    }

    if (mouseaim)
        bLockPan = true;

    // loc_1C05E
    fix16_t ecx = nDestVertPan[nLocalPlayer] - PlayerList[nLocalPlayer].q16horiz;

    if (mouseaim)
    {
        ecx = 0;
    }

    if (!nFreeze)
    {
        if (ecx)
        {
            if (ecx / 4 == 0)
            {
                if (ecx >= 0) {
                    ecx = 1;
                }
                else
                {
                    ecx = -1;
                }
            }
            else
            {
                ecx /= 4;

                if (ecx > fix16_from_int(4))
                {
                    ecx = fix16_from_int(4);
                }
                else if (ecx < -fix16_from_int(4))
                {
                    ecx = -fix16_from_int(4);
                }
            }

            PlayerList[nLocalPlayer].q16horiz = fix16_sadd(PlayerList[nLocalPlayer].q16horiz, ecx);
        }

        PlayerList[nLocalPlayer].q16horiz = fix16_clamp(PlayerList[nLocalPlayer].q16horiz, fix16_from_int(0), fix16_from_int(184));
    }
}


END_PS_NS
