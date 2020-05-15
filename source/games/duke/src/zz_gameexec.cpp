//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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

#include "concmd.h"
#include "compat.h"

#include "duke3d.h"

#include "anim.h"

#include "menus.h"
#include "osdcmds.h"
#include "savegame.h"
#include "gamecvars.h"
#include "version.h"

#include "debugbreak.h"

BEGIN_DUKE_NS


// verification that the event actually exists happens elsewhere
static FORCE_INLINE int32_t VM_EventInlineInternal__(int const eventNum, int const spriteNum, int const playerNum,
                                                       int const playerDist = -1, int32_t returnValue = 0)
{
#if 0
    vmstate_t const newVMstate = { spriteNum, playerNum, playerDist, 0,
                                   &sprite[spriteNum&(MAXSPRITES-1)],
                                   &actor[spriteNum&(MAXSPRITES-1)].t_data[0],
                                   g_player[playerNum&(MAXPLAYERS-1)].ps,
                                   &actor[spriteNum&(MAXSPRITES-1)] };

    auto &globalReturn = aGameVars[g_returnVarID].lValue;

    struct
    {
        vmstate_t vm;
        intptr_t globalReturn;
        int eventNum;
        intptr_t const *insptr;
    } const saved = { vm, globalReturn, g_currentEvent, insptr };

    vm = newVMstate;
    g_currentEvent = eventNum;
    insptr = apScript + apScriptGameEvent[eventNum];
    globalReturn = returnValue;

    double const t = timerGetHiTicks();

    if ((unsigned)spriteNum >= MAXSPRITES)
        VM_DummySprite();

    if ((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers)
        vm.pPlayer = g_player[0].ps;

    while (1) if (parse()) break;

    if (killit_flag == 1)
    {
        // if player was set to squish, first stop that...
        if (ps[g_p].actorsqu == g_i)
            ps[g_p].actorsqu = -1;
        deletesprite(g_i);
    }

    // restoring these needs to happen after VM_DeleteSprite() due to event recursion
    returnValue = globalReturn;

    vm             = saved.vm;
    globalReturn   = saved.globalReturn;
    g_currentEvent = saved.eventNum;
    insptr         = saved.insptr;

    return returnValue;
#endif
    return 0;
}

// the idea here is that the compiler inlines the call to VM_EventInlineInternal__() and gives us a set of
// functions which are optimized further based on distance/return having values known at compile time

int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum, int const nDist, int32_t const nReturn)
{
    return VM_EventInlineInternal__(nEventID, spriteNum, playerNum, nDist, nReturn);
}

int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum, int const nDist)
{
    return VM_EventInlineInternal__(nEventID, spriteNum, playerNum, nDist);
}

int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum)
{
    return VM_EventInlineInternal__(nEventID, spriteNum, playerNum);
}

int32_t VM_ExecuteEventWithValue(int const nEventID, int const spriteNum, int const playerNum, int32_t const nReturn)
{
    return VM_EventInlineInternal__(nEventID, spriteNum, playerNum, -1, nReturn);
}

END_DUKE_NS
