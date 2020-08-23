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
    if (!nMapMode)
    {
        if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
        {
            buttonMap.ClearButton(gamefunc_Enlarge_Screen);
            if (!SHIFTS_IS_PRESSED)
            {
                G_ChangeHudLayout(1);
            }
            else
            {
                hud_scale = hud_scale + 4;
            }
        }

        if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
        {
            buttonMap.ClearButton(gamefunc_Shrink_Screen);
            if (!SHIFTS_IS_PRESSED)
            {
                G_ChangeHudLayout(-1);
            }
            else
            {
                hud_scale = hud_scale - 4;
            }
        }
    }

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
    if (buttonMap.ButtonDown(gamefunc_Map)) // e.g. TAB (to show 2D map)
    {
        buttonMap.ClearButton(gamefunc_Map);

        if (!nFreeze) {
            nMapMode = (nMapMode + 1) % 3;
        }
    }

    if (nMapMode != 0)
    {
        int const timerOffset = ((int)totalclock - nonsharedtimer);
        nonsharedtimer += timerOffset;

        if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
            lMapZoom += mulscale6(timerOffset, max<int>(lMapZoom, 256));

        if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
            lMapZoom -= mulscale6(timerOffset, max<int>(lMapZoom, 256));

        lMapZoom = clamp(lMapZoom, 48, 2048);
    }

    if (PlayerList[nLocalPlayer].nHealth > 0)
    {
        if (buttonMap.ButtonDown(gamefunc_Inventory_Left))
        {
            SetPrevItem(nLocalPlayer);
            buttonMap.ClearButton(gamefunc_Inventory_Left);
        }
        if (buttonMap.ButtonDown(gamefunc_Inventory_Right))
        {
            SetNextItem(nLocalPlayer);
            buttonMap.ClearButton(gamefunc_Inventory_Right);
        }
        if (buttonMap.ButtonDown(gamefunc_Inventory))
        {
            UseCurItem(nLocalPlayer);
            buttonMap.ClearButton(gamefunc_Inventory);
        }
    }
    else {
        SetAirFrame();
    }
}

  END_PS_NS
