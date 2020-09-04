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
#include "v_video.h"

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
    static int nonsharedtimer;
    int ms = screen->FrameTime;
    int interval;
    if (nonsharedtimer > 0 || ms < nonsharedtimer)
    {
        interval = ms - nonsharedtimer;
    }
    else
    {
        interval = 0;
    }
    nonsharedtimer = screen->FrameTime;

    if (System_WantGuiCapture())
        return;

    if (automapMode != am_off)
    {
        double j = interval * (120. / 1000);

        if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
            lMapZoom += (int)fmulscale6(j, max(lMapZoom, 256));
        if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
            lMapZoom -= (int)fmulscale6(j, max(lMapZoom, 256));

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
    fixed_t input_angle = 0;

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
        input_angle += FloatToFixed(info.mousex + scaleAdjustmentToInterval(info.dyaw));
    }

    bool mouseaim = !(localInput.actions & SB_AIMMODE);

    if (mouseaim)
        tempinput.q16horz += FloatToFixed(info.mousey);
    else
        tempinput.fvel -= info.mousey * 8.f;

    if (!in_mouseflip) tempinput.q16horz = -tempinput.q16horz;

    tempinput.q16horz -= FloatToFixed(scaleAdjustmentToInterval(info.dpitch));
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
        input_angle += FloatToFixed(scaleAdjustmentToInterval(turn * 2));

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

    localInput.q16avel += input_angle;

    if (!nFreeze)
    {
        PlayerList[nLocalPlayer].q16angle = (PlayerList[nLocalPlayer].q16angle + input_angle) & 0x7FFFFFF;

        // A horiz diff of 128 equal 45 degrees,
        // so we convert horiz to 1024 angle units

        float const horizAngle = clamp(atan2f(PlayerList[nLocalPlayer].q16horiz - IntToFixed(92), IntToFixed(128)) * (512.f / fPI) + FixedToFloat(tempinput.q16horz), -255.f, 255.f);
        auto newq16horiz = IntToFixed(92) + xs_CRoundToInt(IntToFixed(128) * tanf(horizAngle * (fPI / 512.f)));
        if (PlayerList[nLocalPlayer].q16horiz != newq16horiz)
        {
            bLockPan = true;
            PlayerList[nLocalPlayer].q16horiz = newq16horiz;
            nDestVertPan[nLocalPlayer] = PlayerList[nLocalPlayer].q16horiz;
        }

        // Look/aim up/down functions.
        if (localInput.actions & (SB_LOOK_UP|SB_AIM_UP))
        {
            bLockPan |= (localInput.actions & SB_LOOK_UP);
            if (PlayerList[nLocalPlayer].q16horiz < IntToFixed(180)) {
                PlayerList[nLocalPlayer].q16horiz += FloatToFixed(scaleAdjustmentToInterval(4));
            }

            bPlayerPan = true;
            nDestVertPan[nLocalPlayer] = PlayerList[nLocalPlayer].q16horiz;
        }
        else if (localInput.actions & (SB_LOOK_DOWN|SB_AIM_DOWN))
        {
            bLockPan |= (localInput.actions & SB_LOOK_DOWN);
            if (PlayerList[nLocalPlayer].q16horiz > IntToFixed(4)) {
                PlayerList[nLocalPlayer].q16horiz -= FloatToFixed(scaleAdjustmentToInterval(4));
            }

            bPlayerPan = true;
            nDestVertPan[nLocalPlayer] = PlayerList[nLocalPlayer].q16horiz;
        }
    }

    // loc_1C048:
    if (totalvel[nLocalPlayer] > 20) {
        bPlayerPan = false;
    }
    if (nFreeze) return;

    // loc_1C05E
    fixed_t dVertPan = nDestVertPan[nLocalPlayer] - PlayerList[nLocalPlayer].q16horiz;
    if (dVertPan != 0 && !bLockPan)
    {
        int val = dVertPan / 4;
        if (abs(val) >= 4)
        {
            if (val >= 4)
                PlayerList[nLocalPlayer].q16horiz += IntToFixed(4);
            else if (val <= -4)
                PlayerList[nLocalPlayer].q16horiz -= IntToFixed(4);
        }
        else if (abs(dVertPan) >= FRACUNIT)
            PlayerList[nLocalPlayer].q16horiz += dVertPan / 2.0f;
        else
        {
            if (mouseaim) bLockPan = true;
            PlayerList[nLocalPlayer].q16horiz = nDestVertPan[nLocalPlayer];
        }
    }
    else bLockPan = mouseaim;
    PlayerList[nLocalPlayer].q16horiz = clamp(PlayerList[nLocalPlayer].q16horiz, 0, IntToFixed(184));
}


void GameInterface::GetInput(InputPacket* packet)
{
    PlayerInterruptKeys(packet == nullptr);
    if (packet) *packet = localInput;
}

END_PS_NS
