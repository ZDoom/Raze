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
#include "input.h"
#include "exhumed.h"
#include "player.h"
#include "status.h"
#include "view.h"
#include "razemenu.h"

BEGIN_PS_NS

PlayerInput sPlayerInput[kMaxPlayers];

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ClearSpaceBar(int nPlayer)
{
    sPlayerInput[nPlayer].actions &= SB_OPEN;
    buttonMap.ClearButton(gamefunc_Open);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::GetInput(ControlInfo* const hidInput, double const scaleAdjust, InputPacket* packet)
{
    if (paused || M_Active())
    {
        localInput = {};
        return;
    }

    if (packet != nullptr)
    {
        localInput = {};
        ApplyGlobalInput(localInput, hidInput);
        if (PlayerList[nLocalPlayer].nHealth == 0) localInput.actions &= SB_OPEN;
    }

    Player* pPlayer = &PlayerList[nLocalPlayer];
    InputPacket input {};

    if (PlayerList[nLocalPlayer].nHealth != 0)
    {
        processMovement(&input, &localInput, hidInput, scaleAdjust);
    }
    else
    {
        sPlayerInput[nLocalPlayer].vel.Zero();
    }

    if (!SyncInput() && gamestate == GS_LEVEL && !nFreeze)
    {
        pPlayer->Angles.RenderAngles.Yaw += DAngle::fromDeg(input.avel);
        pPlayer->Angles.RenderAngles.Pitch += DAngle::fromDeg(input.horz);

        if (input.horz)
        {
            pPlayer->bPlayerPan = pPlayer->bLockPan = true;
        }

        UpdatePlayerSpriteAngle(pPlayer);
    }

    if (packet)
    {
        *packet = localInput;
    }
}

//---------------------------------------------------------------------------
//
// This is called from InputState::ClearAllInput and resets all static state being used here.
//
//---------------------------------------------------------------------------

void GameInterface::clearlocalinputstate()
{
    localInput = {};
}

END_PS_NS
