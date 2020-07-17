//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#include "duke3d.h"
#include "d_event.h"
#include "gamevar.h"

BEGIN_DUKE_NS


int32_t PHEIGHT = PHEIGHT_DUKE;

int32_t lastvisinc;

#define TURBOTURNTIME (TICRATE/8) // 7
#define NORMALTURN    15
#define PREAMBLETURN  5
#define NORMALKEYMOVE 40
#define MAXVEL        ((NORMALKEYMOVE*2)+10)
#define MAXSVEL       ((NORMALKEYMOVE*2)+10)
#define MAXANGVEL     1024
#define MAXHORIZVEL   256
#define ONEEIGHTYSCALE 4

#define MOTOTURN      20
#define MAXVELMOTO    120

int32_t g_myAimStat = 0, g_oldAimStat = 0;
int32_t mouseyaxismode = -1;

enum inputlock_t
{
    IL_NOANGLE = 0x1,
    IL_NOHORIZ = 0x2,
    IL_NOMOVE  = 0x4,

    IL_NOTHING = IL_NOANGLE|IL_NOHORIZ|IL_NOMOVE,
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static int P_CheckLockedMovement(int snum)
{
    auto p = &ps[snum];

    if (sprite[p->i].extra <= 0 || (p->dead_flag && !ud.god) || movementBlocked(snum))
        return IL_NOTHING;

    if (p->on_crane >= 0)
        return IL_NOMOVE | IL_NOANGLE;

    if (p->newowner != -1)
        return IL_NOANGLE | IL_NOHORIZ;

    if (p->return_to_center > 0)
        return IL_NOHORIZ;

    return 0;
}

//---------------------------------------------------------------------------
//
// split off so that it can later be integrated into the other games more easily.
//
//---------------------------------------------------------------------------

void checkCrouchToggle(player_struct* p)
{
    int const sectorLotag = p->cursectnum != -1 ? sector[p->cursectnum].lotag : 0;
    int const crouchable = sectorLotag != ST_2_UNDERWATER && (sectorLotag != ST_1_ABOVE_WATER || p->spritebridge);

    if (buttonMap.ButtonDown(gamefunc_Toggle_Crouch))
    {
        p->crouch_toggle = !p->crouch_toggle && crouchable;

        if (crouchable)
            buttonMap.ClearButton(gamefunc_Toggle_Crouch);
    }

    if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Jump) || p->jetpack_on || (!crouchable && p->on_ground))
        p->crouch_toggle = 0;
}

//---------------------------------------------------------------------------
//
// common code for all input modes (with one minor special check)
//
//---------------------------------------------------------------------------

void FinalizeInput(int playerNum, input_t &input, bool vehicle)
{
    auto p = &ps[playerNum];
    int const movementLocked = P_CheckLockedMovement(playerNum);

    if ((ud.scrollmode && ud.overhead_on) || (movementLocked & IL_NOTHING) == IL_NOTHING)
    {
        if (ud.scrollmode && ud.overhead_on)
        {
            ud.folfvel = input.fvel;
            ud.folavel = fix16_to_int(input.q16avel);
        }

        loc.fvel = loc.svel = 0;
        loc.q16avel = loc.q16horz = 0;
    }
    else
    {
        if (!(movementLocked & IL_NOMOVE))
        {
            if (!vehicle)
            {
                loc.fvel = clamp(loc.fvel + input.fvel, -MAXVEL, MAXVEL);
                loc.svel = clamp(loc.svel + input.svel, -MAXSVEL, MAXSVEL);
            }
            else
                loc.fvel = clamp(input.fvel, -(MAXVELMOTO / 8), MAXVELMOTO);
        }

        if (!(movementLocked & IL_NOANGLE))
        {
            loc.q16avel = fix16_sadd(loc.q16avel, input.q16avel);
            if (!synchronized_input)
            {
                p->q16ang = fix16_sadd(p->q16ang, input.q16avel) & 0x7FFFFFF;

                if (input.q16avel)
                {
                    p->one_eighty_count = 0;
                }
            }
        }

        if (!(movementLocked & IL_NOHORIZ))
        {
            loc.q16horz = fix16_clamp(fix16_sadd(loc.q16horz, input.q16horz), F16(-MAXHORIZVEL), F16(MAXHORIZVEL));
            if (!synchronized_input)
                p->q16horiz += input.q16horz; // will be clamped below in sethorizon.
        }
    }
}

END_DUKE_NS
