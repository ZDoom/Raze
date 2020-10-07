//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "ns.h"	// Must come before everything else!

#include "mmulti.h"
#include "view.h"
#include "gamestate.h"
#include "menu.h"

BEGIN_BLD_NS

static InputPacket gInput;

void UpdatePlayerSpriteAngle(PLAYER* pPlayer);

void GameInterface::GetInput(InputPacket* packet, ControlInfo* const hidInput)
{
    if (paused || M_Active())
    {
        gInput = {};
        return;
    }

    double const scaleAdjust = InputScale();
    InputPacket input {};

    ApplyGlobalInput(gInput, hidInput);
    processMovement(&input, &gInput, hidInput, scaleAdjust);

    if (!cl_syncinput && gamestate == GS_LEVEL)
    {
        PLAYER* pPlayer = &gPlayer[myconnectindex];

        // Perform unsynchronised angle/horizon if not dead.
        if (gView->pXSprite->health != 0)
        {
            applylook(&pPlayer->q16ang, &pPlayer->q16look_ang, &pPlayer->q16rotscrnang, &pPlayer->spin, input.q16avel, &pPlayer->input.actions, scaleAdjust, pPlayer->posture != 0);
            sethorizon(&pPlayer->horizon.horiz, input.q16horz, &pPlayer->input.actions, scaleAdjust);
        }

        // temporary vals to pass through to playerProcessHelpers().
        fixed_t horiz = 0;
        fixed_t target = 0;
        double adjust = 0;
        playerProcessHelpers(&pPlayer->q16ang, &pPlayer->angAdjust, &pPlayer->angTarget, &horiz, &adjust, &target, scaleAdjust);
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
