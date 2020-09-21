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
#include "exhumed.h"
#include "player.h"
#include "status.h"
#include "view.h"
#include "menu.h"

BEGIN_PS_NS

extern short bPlayerPan;
extern short bLockPan;

static int turn;
static int counter;

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


void CheckKeys2()
{
    if (PlayerList[nLocalPlayer].nHealth <= 0)
    {
        SetAirFrame();
    }
}


static void processMovement(ControlInfo* const hidInput)
{
    // JBF: Run key behaviour is selectable
    int const playerRunning = !!(localInput.actions & SB_RUN);
    int const turnAmount = playerRunning ? 12 : 8;
    int const keyMove = playerRunning ? 12 : 6;
    bool const mouseaim = !(localInput.actions & SB_AIMMODE);
    double const scaleAdjust = InputScale();
    InputPacket tempinput {};

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        tempinput.svel -= xs_CRoundToInt((hidInput->mousex * 32.) + (scaleAdjust * (hidInput->dyaw * keyMove)));
    }
    else
    {
        tempinput.q16avel += FloatToFixed(hidInput->mousex + (scaleAdjust * hidInput->dyaw));
    }

    if (mouseaim)
    {
        tempinput.q16horz += FloatToFixed(hidInput->mousey);
    }
    else
    {
        tempinput.fvel -= xs_CRoundToInt(hidInput->mousey * 8.);
    }

    if (!in_mouseflip) 
        tempinput.q16horz = -tempinput.q16horz;

    tempinput.q16horz -= FloatToFixed(scaleAdjust * hidInput->dpitch);
    tempinput.svel -= xs_CRoundToInt(scaleAdjust * (hidInput->dx * keyMove));
    tempinput.fvel -= xs_CRoundToInt(scaleAdjust * (hidInput->dz * keyMove));

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
            tempinput.svel -= -keyMove;

        if (buttonMap.ButtonDown(gamefunc_Turn_Right))
            tempinput.svel -= keyMove;
    }
    else
    {
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
        tempinput.q16avel += FloatToFixed(scaleAdjust * (turn * 2));

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
    localInput.q16avel += tempinput.q16avel;
    localInput.q16horz += tempinput.q16horz;

    if (!cl_syncinput)
    {
        Player* pPlayer = &PlayerList[nLocalPlayer];

        if (!nFreeze)
        {
            applylook(&pPlayer->q16angle, &pPlayer->q16look_ang, &pPlayer->q16rotscrnang, &pPlayer->spin, tempinput.q16avel, &sPlayerInput[nLocalPlayer].actions, scaleAdjust, false);
            sethorizon(&pPlayer->q16horiz, tempinput.q16horz, &sPlayerInput[nLocalPlayer].actions, scaleAdjust);
            UpdatePlayerSpriteAngle(pPlayer);
        }

        playerProcessHelpers(&pPlayer->q16angle, &pPlayer->angAdjust, &pPlayer->angTarget, &pPlayer->q16horiz, &pPlayer->horizAdjust, &pPlayer->horizTarget, scaleAdjust);
    }
}


void GameInterface::GetInput(InputPacket* packet, ControlInfo* const hidInput)
{
    if (paused || M_Active())
    {
        localInput = {};
        return;
    }

    if (PlayerList[nLocalPlayer].nHealth == 0)
    {
        lPlayerYVel = 0;
        lPlayerXVel = 0;
        return;
    }

    if (packet != nullptr)
    {
        localInput = {};
        ApplyGlobalInput(localInput, hidInput);
        if (PlayerList[nLocalPlayer].nHealth == 0) localInput.actions &= SB_OPEN;
    }

    processMovement(hidInput);
    if (packet) *packet = localInput;
}

END_PS_NS
