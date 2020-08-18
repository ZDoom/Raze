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
#include "network.h"
#include <string.h>

BEGIN_PS_NS

int nNetMoves = 0;

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

void GetLocalInput()
{
    int i;
    for (i = 6; i >= 0; i--)
    {
        if (buttonMap.ButtonDown(gamefunc_Weapon_1+i))
            break;
    }
    i++;

    if (PlayerList[nLocalPlayer].nHealth)
    {
        lLocalButtons = (buttonMap.ButtonDown(gamefunc_Crouch) << 4) | (buttonMap.ButtonDown(gamefunc_Fire) << 3)
                      | (buttonMap.ButtonDown(gamefunc_Jump)<<0) | (i<<13);
    }
    else
    {
        lLocalButtons = 0;
    }

    lLocalButtons |= buttonMap.ButtonDown(gamefunc_Open) << 2;

// TODO	ExecRecord(&sPlayerInput[nLocalPlayer], sizeof(PlayerInput));
}

void BackupInput()
{

}

void SendInput()
{

}

void LogoffPlayer(int nPlayer)
{
    if (nPlayer == nLocalPlayer)
        return;

    if (PlayerList[nPlayer].someNetVal == -1)
        return;

    memset(&sPlayerInput[nPlayer], 0, sizeof(sPlayerInput));

    sprite[nDoppleSprite[nPlayer]].cstat = 0x8000u;
    sprite[nPlayerFloorSprite[nPlayer]].cstat = 0x8000u;
    sprite[PlayerList[nPlayer].nSprite].cstat = 0x8000u;

    PlayerList[nPlayer].someNetVal = -1;

    StatusMessage(150, "Player %d has left the game", nPlayer);

// TODO	ClearPlayerInput(&sPlayerInput[nPlayer]);
    nNetPlayerCount--;
}

void UpdateInputs()
{
    nNetMoveFrames = moveframes;

    if (nNetPlayerCount)
    {
        UpdateNetInputs();

        nNetMoves++;

        if (!nNetMoves) {
            nNetMoves++;
        }
    }
}

/*
ClearSpaceBar_
    GetLocalInput_
    GetModemInput_
    BackupInput_
    SendInput_
    SendToUnAckd_
    LogoffPlayer_
    UpdateInputs_
    faketimerhandler_
*/

void WaitNoKey(int nSecs, void (*pFunc) (void))
{
    int nTotalTime = (kTimerTicks * nSecs) + (int)totalclock;

    while (nTotalTime > (int)totalclock)
    {
        HandleAsync();
        if (pFunc) {
            pFunc();
        }
    }
}

int WaitAnyKey(int nSecs)
{
    int nTotalTime = (int)totalclock + (kTimerTicks * nSecs);

    while (1)
    {
        HandleAsync();
        if (nTotalTime <= (int)totalclock || nSecs == -1) {
            return -1;
        }
		if (inputState.CheckAllInput()) return 1;
    }
}


/*
    Name:  _nLocalPlayer
  Name:  _nNetPlayerCount
  Name:  _nModemPlayer
  Name:  _nNetMoves
  Name:  _lLocalButtons
  Name:  _lLocalCodes
  Name:  _nInputStack
  Name:  _bSyncNet
  Name:  _lStartupTime
  Name:  _bStackNode
  Name:  _sSync
  Name:  _nTypeStack
  Name:  _sPlayerInput
  Name:  _pStackPtr
  Name:  _sInputStack

  */
  END_PS_NS
