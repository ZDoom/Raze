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

int WeaponToSend, BitsToSend;

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
    int i = WeaponToSend;
    if (WeaponToSend == PlayerList[nLocalPlayer].nCurrentWeapon)
        WeaponToSend = 0;

    if (PlayerList[nLocalPlayer].nHealth)
    {
        lLocalButtons = (buttonMap.ButtonDown(gamefunc_Crouch) << 4) | (buttonMap.ButtonDown(gamefunc_Fire) << 3)
            | (buttonMap.ButtonDown(gamefunc_Jump) << 0);
        lLocalCodes |= (i << 13);
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

short nNetMoveFrames = 0;

void UpdateInputs()
{
    nNetMoveFrames = moveframes;

    if (nNetPlayerCount)
    {
        //UpdateNetInputs();

        nNetMoves++;

        if (!nNetMoves) {
            nNetMoves++;
        }
    }
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
        int const timerOffset = ((int)totalclock - nonsharedtimer);
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


//---------------------------------------------------------------------------
//
// CCMD based input. The basics are from Randi's ZDuke but this uses dynamic
// registration to only have the commands active when this game module runs.
//
//---------------------------------------------------------------------------

static int ccmd_slot(CCmdFuncPtr parm)
{
    if (parm->numparms != 1) return CCMD_SHOWHELP;

    auto slot = atoi(parm->parms[0]);
    if (slot >= 1 && slot <= 7)
    {
        WeaponToSend = slot;
        return CCMD_OK;
    }
    return CCMD_SHOWHELP;
}

int ccmd_centerview(CCmdFuncPtr parm);


void registerinputcommands()
{
    C_RegisterFunction("slot", "slot <weaponslot>: select a weapon from the given slot (1-10)", ccmd_slot);
    C_RegisterFunction("pause", nullptr, [](CCmdFuncPtr)->int { /*BitsToSend |= SKB_PAUSE;*/ sendPause = true; return CCMD_OK; });
    C_RegisterFunction("centerview", nullptr, ccmd_centerview);
    C_RegisterFunction("invprev", nullptr, [](CCmdFuncPtr)->int { if (PlayerList[nLocalPlayer].nHealth > 0) SetPrevItem(nLocalPlayer); return CCMD_OK; });
    C_RegisterFunction("invnext", nullptr, [](CCmdFuncPtr)->int { if (PlayerList[nLocalPlayer].nHealth > 0) SetNextItem(nLocalPlayer); return CCMD_OK; });
    C_RegisterFunction("invuse", nullptr, [](CCmdFuncPtr)->int { if (PlayerList[nLocalPlayer].nHealth > 0) UseCurItem(nLocalPlayer); return CCMD_OK; });
    // todo: 
    //C_RegisterFunction("weapprev", nullptr, [](CCmdFuncPtr)->int { WeaponToSend = 11; return CCMD_OK; });
    //C_RegisterFunction("weapnext", nullptr, [](CCmdFuncPtr)->int { WeaponToSend = 12; return CCMD_OK; });
    //C_RegisterFunction("turnaround", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= SKB_TURNAROUND; return CCMD_OK; });

}

// This is called from ImputState::ClearAllInput and resets all static state being used here.
void GameInterface::clearlocalinputstate()
{
    WeaponToSend = 0;
    BitsToSend = 0;

}

  END_PS_NS
