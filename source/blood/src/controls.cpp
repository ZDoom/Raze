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
    MAXHORIZVEL = 32
};

void applylook(PLAYER *pPlayer, fixed_t const q16avel, double const scaleAdjust);
void sethorizon(PLAYER *pPlayer, fixed_t const q16horz, double const scaleAdjust);

//---------------------------------------------------------------------------
//
// handles the input bits
//
//---------------------------------------------------------------------------

static void processInputBits(ControlInfo* const hidInput, bool* mouseaim)
{
    ApplyGlobalInput(gInput, hidInput);
    *mouseaim = !(gInput.actions & SB_AIMMODE);

    if (!mouseaim || (gInput.actions & (SB_LOOK_UP|SB_LOOK_DOWN)))
    {
        gInput.actions |= SB_CENTERVIEW;
    }
}

//---------------------------------------------------------------------------
//
// handles movement
//
//---------------------------------------------------------------------------

static void processMovement(ControlInfo* const hidInput, bool const mouseaim)
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

    if (mouseaim)
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
        applylook(pPlayer, input.q16avel, scaleAdjust);
        sethorizon(pPlayer, input.q16horz, scaleAdjust);
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

    bool mouseaim;

    processInputBits(hidInput, &mouseaim);
    processMovement(hidInput, mouseaim);

    if (packet)
    {
        *packet = gInput;
        gInput = {};
    }
}

END_BLD_NS
