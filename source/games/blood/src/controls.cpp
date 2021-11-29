#pragma once
//-------------------------------------------------------------------------
/*
Copyright (C) 2020 Christoph Oelckers & Mitchell Richters

This file is part of Raze.

Raze is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

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

#include "blood.h"
#include "gamestate.h"
#include "inputstate.h"
#include "gamestruct.h"
#include "razemenu.h"

BEGIN_BLD_NS

static InputPacket gInput;

void UpdatePlayerSpriteAngle(PLAYER* pPlayer);
void doslopetilting(PLAYER* pPlayer, double const scaleAdjust);

void GameInterface::GetInput(ControlInfo* const hidInput, double const scaleAdjust, InputPacket* packet)
{
    if (paused || M_Active())
    {
        gInput = {};
        return;
    }

    PLAYER* pPlayer = &gPlayer[myconnectindex];
    InputPacket input {};

    ApplyGlobalInput(gInput, hidInput);
    processMovement(&input, &gInput, hidInput, scaleAdjust);

    if (!SyncInput() && gamestate == GS_LEVEL)
    {
        // Perform unsynchronised angle/horizon if not dead.
        if (gView->pXSprite->health != 0)
        {
            pPlayer->angle.applyinput(input.avel, &pPlayer->input.actions, scaleAdjust);
            pPlayer->horizon.applyinput(input.horz, &pPlayer->input.actions, scaleAdjust);
            doslopetilting(pPlayer, scaleAdjust);
        }

        pPlayer->angle.processhelpers(scaleAdjust);
        pPlayer->horizon.processhelpers(scaleAdjust);
        UpdatePlayerSpriteAngle(pPlayer);
    }

    if (packet)
    {
        *packet = gInput;
        gInput = {};
    }
}

//---------------------------------------------------------------------------
//
// This is called from InputState::ClearAllInput and resets all static state being used here.
//
//---------------------------------------------------------------------------

void GameInterface::clearlocalinputstate()
{
    gInput = {};
}

END_BLD_NS
