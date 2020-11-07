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
#include "razemenu.h"

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

    PLAYER* pPlayer = &gPlayer[myconnectindex];
    double const scaleAdjust = InputScale();
    InputPacket input {};

    ApplyGlobalInput(gInput, hidInput);
    processMovement(&input, &gInput, hidInput, scaleAdjust);

    // Handle crouch toggling.
    if (buttonMap.ButtonDown(gamefunc_Toggle_Crouch) || pPlayer->crouch_toggle)
    {
        gInput.actions |= SB_CROUCH;
    }
    if (buttonMap.ButtonDown(gamefunc_Toggle_Crouch))
    {
        pPlayer->crouch_toggle = !pPlayer->crouch_toggle;
        buttonMap.ClearButton(gamefunc_Toggle_Crouch);
    }
    if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Jump))
    {
        pPlayer->crouch_toggle = false;
    }

    if (!cl_syncinput && gamestate == GS_LEVEL)
    {
        // Perform unsynchronised angle/horizon if not dead.
        if (gView->pXSprite->health != 0)
        {
            applylook(&pPlayer->angle, input.avel, &pPlayer->input.actions, scaleAdjust, pPlayer->posture != 0);
            sethorizon(&pPlayer->horizon.horiz, input.horz, &pPlayer->input.actions, scaleAdjust);
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
