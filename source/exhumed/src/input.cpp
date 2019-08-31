
#include "input.h"
#include "engine.h"
#include "exhumed.h"
#include "player.h"
#include "serial.h"
#include "network.h"
#include "keyboard.h"
#include "control.h"
#include "config.h"
#include <string.h>

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
    CONTROL_ClearButton(gamefunc_Open);
}

void GetLocalInput()
{
    int ebx = 6;
    int eax = 24;
    int edx = -8;
    uint8_t cl;

    // TODO: clean up

    uint32_t esi;

    while (ebx >= 0)
    {
        if (eax <= 31)
        {
            esi = CONTROL_ButtonState;
            cl = eax;
        }
        else
        {
            esi = CONTROL_ButtonState>>32;
            cl = edx;
        }

        if ((esi >> cl) & 1)
            break;

        eax--;
        edx--;
        ebx--;
    }

    ebx++;

    if (PlayerList[nLocalPlayer].nHealth)
    {
        eax = (BUTTON(gamefunc_Crouch) << 4) | (BUTTON(gamefunc_Fire) << 3);

        edx = BUTTON(gamefunc_Jump);

        ebx <<= 13;

        eax |= edx;
        eax |= ebx;

        lLocalButtons = eax;
    }
    else
    {
        lLocalButtons = 0;
    }

    lLocalButtons |= BUTTON(gamefunc_Open) << 2;

    if (BUTTON(gamefunc_Open)) {
        int breakme = 123;
    }

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
        if (bSerialPlay) {
            UpdateSerialInputs();
        }
        else {
            UpdateNetInputs();
        }

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

void ClearAllKeys()
{
    KB_ClearKeysDown();
    KB_FlushKeyboardQueue();
}

void WaitNoKey(int nSecs, void (*pFunc) (void))
{
    int nTotalTime = (kTimerTicks * nSecs) + totalclock;

    while (nTotalTime > totalclock)
    {
#ifdef _MSC_VER
        HandleAsync();
#endif
        if (pFunc) {
            pFunc();
        }
    }
}

int WaitAnyKey(int nSecs)
{
    int nTotalTime = totalclock + (kTimerTicks * nSecs);

    while (1)
    {
#ifdef _MSC_VER
        HandleAsync();
#endif
        if (nTotalTime <= totalclock || nSecs == -1) {
            return -1;
        }

        int i = 0;

        do
        {
            if (KB_KeyDown[i])
            {
                KB_KeyDown[i] = 0;
                return i;
            }

            i++;

        } while (i < 106);
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