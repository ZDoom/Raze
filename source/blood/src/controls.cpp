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

static const double gTurnSpeed = 92.;
static InputPacket gInput;
static double turnHeldTime;

enum
{
    MAXFVEL     = 2048,
    MAXSVEL     = 2048,
    MAXHORIZVEL = 128
};

void UpdatePlayerSpriteAngle(PLAYER* pPlayer);

//---------------------------------------------------------------------------
//
// handles movement
//
//---------------------------------------------------------------------------

static void processMovement(ControlInfo* const hidInput)
{
    double const scaleAdjust = InputScale();
    int const run = !!(gInput.actions & SB_RUN);
    int const keyMove = (1 + run) << 10;
    InputPacket input = {};

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        input.svel -= xs_CRoundToInt((hidInput->mousex * 32.) + (scaleAdjust * (hidInput->dyaw * keyMove)));
    }
    else
    {
        input.q16avel += FloatToFixed(hidInput->mousex + (scaleAdjust * hidInput->dyaw));
    }

    if (!(gInput.actions & SB_AIMMODE))
    {
        input.q16horz += FloatToFixed(hidInput->mousey);
    }
    else
    {
        input.fvel -= xs_CRoundToInt(hidInput->mousey * 64.);
    }

    if (!in_mouseflip)
        input.q16horz = -input.q16horz;

    input.q16horz -= FloatToFixed(scaleAdjust * hidInput->dpitch);
    input.svel -= xs_CRoundToInt(scaleAdjust * (hidInput->dx * keyMove));
    input.fvel -= xs_CRoundToInt(scaleAdjust * (hidInput->dz * keyMove));

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        if (gInput.svel < keyMove && gInput.svel > -keyMove)
        {
            if (buttonMap.ButtonDown(gamefunc_Turn_Left))
                input.svel += keyMove;

            if (buttonMap.ButtonDown(gamefunc_Turn_Right))
                input.svel -= keyMove;
        }
    }
    else
    {
        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
        {
            turnHeldTime += scaleAdjust * kTicsPerFrame;
            input.q16avel -= FloatToFixed(scaleAdjust * (min(12. * turnHeldTime, gTurnSpeed) / 4.));
        }
        else if (buttonMap.ButtonDown(gamefunc_Turn_Right))
        {
            turnHeldTime += scaleAdjust * kTicsPerFrame;
            input.q16avel += FloatToFixed(scaleAdjust * (min(12. * turnHeldTime, gTurnSpeed) / 4.));
        }
        else
        {
            turnHeldTime = 0;
        }
    }

    if (run && turnHeldTime > 24.)
        input.q16avel <<= 1;

    if (abs(gInput.fvel) < keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Move_Forward))
            input.fvel += keyMove;

        if (buttonMap.ButtonDown(gamefunc_Move_Backward))
            input.fvel -= keyMove;
    }

    if (abs(gInput.svel) < keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Strafe_Left))
            input.svel += keyMove;

        if (buttonMap.ButtonDown(gamefunc_Strafe_Right))
            input.svel -= keyMove;
    }

    if (!cl_syncinput && gamestate == GS_LEVEL)
    {
        PLAYER* pPlayer = &gPlayer[myconnectindex];

        // Perform unsynchronised angle/horizon if not dead.
        if (gView->pXSprite->health != 0)
        {
            applylook(&pPlayer->q16ang, &pPlayer->q16look_ang, &pPlayer->q16rotscrnang, &pPlayer->spin, input.q16avel, &pPlayer->input.actions, scaleAdjust, pPlayer->posture != 0);
            UpdatePlayerSpriteAngle(pPlayer);
            sethorizon(&pPlayer->q16horiz, input.q16horz, &pPlayer->input.actions, scaleAdjust);
        }

        playerProcessHelpers(&pPlayer->q16ang, &pPlayer->angAdjust, &pPlayer->angTarget, &pPlayer->q16horiz, &pPlayer->horizAdjust, &pPlayer->horizTarget, scaleAdjust);
    }

    gInput.fvel = clamp(gInput.fvel + input.fvel, -MAXFVEL, MAXFVEL);
    gInput.svel = clamp(gInput.svel + input.svel, -MAXSVEL, MAXSVEL);
    gInput.q16avel += input.q16avel;
    gInput.q16horz = clamp(gInput.q16horz + input.q16horz, -IntToFixed(MAXHORIZVEL), IntToFixed(MAXHORIZVEL));
}

void GameInterface::GetInput(InputPacket* packet, ControlInfo* const hidInput)
{
    if (paused || M_Active())
    {
        gInput = {};
        return;
    }

    ApplyGlobalInput(gInput, hidInput);
    processMovement(hidInput);

    if (packet)
    {
        *packet = gInput;
        gInput = {};
    }
}

//---------------------------------------------------------------------------
//
// This is called from ImputState::ClearAllInput and resets all static state being used here.
//
//---------------------------------------------------------------------------

void GameInterface::clearlocalinputstate()
{
    gInput = {};
    turnHeldTime = 0;
}

END_BLD_NS
