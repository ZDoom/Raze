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

int BitsToSend;
bool g_MyAimMode;

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
    sPlayerInput[nPlayer].buttons &= 0x0FB;
    buttonMap.ClearButton(gamefunc_Open);
}

int GetLocalInput()
{
    int lLocalButtons;
    if (PlayerList[nLocalPlayer].nHealth)
    {
        lLocalButtons = (buttonMap.ButtonDown(gamefunc_Crouch) << 4) | (buttonMap.ButtonDown(gamefunc_Fire) << 3)
            | (buttonMap.ButtonDown(gamefunc_Jump) << 0);
    }
    else
    {
        lLocalButtons = 0;
    }

    lLocalButtons |= buttonMap.ButtonDown(gamefunc_Open) << 2;
    return lLocalButtons;
}

void BackupInput()
{

}

void SendInput()
{

}

void CheckKeys()
{
    // go to 3rd person view?
    if (buttonMap.ButtonDown(gamefunc_Third_Person_View))
    {
        if (!nFreeze)
        {
            if (bCamera) {
                bCamera = false;
            }
            else {
                bCamera = true;
            }

            if (bCamera)
                GrabPalette();
        }
        buttonMap.ClearButton(gamefunc_Third_Person_View);
        return;
    }

    if (paused)
    {
        return;
    }
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

    localInput = {};
    InputPacket input{};
    fix16_t input_angle = 0;

    if (PlayerList[nLocalPlayer].nHealth == 0)
    {
        lPlayerYVel = 0;
        lPlayerXVel = 0;
        nPlayerDAng = 0;
        return;
    }

    if (!after)
    {
        ApplyGlobalInput(localInput, &info);
    }


    // JBF: Run key behaviour is selectable
    int const playerRunning = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    int const turnAmount = playerRunning ? 12 : 8;
    int const keyMove = playerRunning ? 12 : 6;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        input.svel -= info.mousex * 4.f;
        input.svel -= info.dyaw * keyMove;
    }
    else
    {
        input_angle = fix16_sadd(input_angle, fix16_from_float(info.mousex));
        input_angle = fix16_sadd(input_angle, fix16_from_dbl(scaleAdjustmentToInterval(info.dyaw)));
    }

    g_MyAimMode = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);

    if (g_MyAimMode)
        input.q16horz = fix16_sadd(input.q16horz, fix16_from_float(info.mousey));
    else
        input.fvel -= info.mousey * 8.f;

    if (!in_mouseflip) input.q16horz = -input.q16horz;

    input.q16horz = fix16_ssub(input.q16horz, fix16_from_dbl(scaleAdjustmentToInterval(info.dpitch)));
    input.svel -= info.dx * keyMove;
    input.fvel -= info.dz * keyMove;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
            input.svel -= -keyMove;

        if (buttonMap.ButtonDown(gamefunc_Turn_Right))
            input.svel -= keyMove;
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
        input.svel += keyMove;

    if (buttonMap.ButtonDown(gamefunc_Strafe_Right))
        input.svel += -keyMove;

    if (buttonMap.ButtonDown(gamefunc_Move_Forward))
        input.fvel += keyMove;

    if (buttonMap.ButtonDown(gamefunc_Move_Backward))
        input.fvel += -keyMove;

    localInput.fvel = clamp(localInput.fvel + input.fvel, -12, 12);
    localInput.svel = clamp(localInput.svel + input.svel, -12, 12);

    localInput.q16avel = fix16_sadd(localInput.q16avel, input_angle);

    if (!nFreeze)
    {
        PlayerList[nLocalPlayer].q16angle = fix16_sadd(PlayerList[nLocalPlayer].q16angle, input_angle) & 0x7FFFFFF;

        // A horiz diff of 128 equal 45 degrees,
        // so we convert horiz to 1024 angle units

        float const horizAngle = clamp(atan2f(PlayerList[nLocalPlayer].q16horiz - fix16_from_int(92), fix16_from_int(128)) * (512.f / fPI) + fix16_to_float(input.q16horz), -255.f, 255.f);
        PlayerList[nLocalPlayer].q16horiz = fix16_from_int(92) + Blrintf(fix16_from_int(128) * tanf(horizAngle * (fPI / 512.f)));

        // Look/aim up/down functions.
        if (buttonMap.ButtonDown(gamefunc_Look_Up) || buttonMap.ButtonDown(gamefunc_Aim_Up))
        {
            bLockPan = false;
            if (PlayerList[nLocalPlayer].q16horiz < fix16_from_int(180)) {
                PlayerList[nLocalPlayer].q16horiz = fix16_sadd(PlayerList[nLocalPlayer].q16horiz, fix16_from_dbl(scaleAdjustmentToInterval(4)));
            }

            bPlayerPan = true;
            nDestVertPan[nLocalPlayer] = PlayerList[nLocalPlayer].q16horiz;
        }
        else if (buttonMap.ButtonDown(gamefunc_Look_Down) || buttonMap.ButtonDown(gamefunc_Aim_Down))
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

    if (g_MyAimMode)
        bLockPan = true;

    // loc_1C05E
    fix16_t ecx = nDestVertPan[nLocalPlayer] - PlayerList[nLocalPlayer].q16horiz;

    if (g_MyAimMode)
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



//---------------------------------------------------------------------------
//
// CCMD based input. The basics are from Randi's ZDuke but this uses dynamic
// registration to only have the commands active when this game module runs.
//
//---------------------------------------------------------------------------

int ccmd_centerview(CCmdFuncPtr parm);


void registerinputcommands()
{
    C_RegisterFunction("centerview", nullptr, ccmd_centerview);

    // These are only here to silence the engine when the keys bound to them are pressed. The functions do not exist.
    C_RegisterFunction("turnaround", nullptr, [](CCmdFuncPtr)->int { return CCMD_OK; });
    C_RegisterFunction("holsterweapon", nullptr, [](CCmdFuncPtr)->int { return CCMD_OK; });

}

// This is called from ImputState::ClearAllInput and resets all static state being used here.
void GameInterface::clearlocalinputstate()
{
    BitsToSend = 0;

}

  END_PS_NS
