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

#ifndef gameexec_h_
#define gameexec_h_

#include "build.h"
#include "global.h"
#include "gamedef.h"  // vmstate_t

BEGIN_DUKE_NS

enum
{
	EVENT_INIT = 0,
	EVENT_ENTERLEVEL,
	EVENT_RESETWEAPONS,	// for each player
	EVENT_RESETINVENTORY, // for each player
	EVENT_HOLSTER,		// for each player
	EVENT_LOOKLEFT,		// for each player
	EVENT_LOOKRIGHT,	// for each player
	EVENT_SOARUP,		// for each player
	EVENT_SOARDOWN,		// for each player
	EVENT_CROUCH,		// for each player
	EVENT_JUMP,			// for each player
	EVENT_RETURNTOCENTER,	// for each player
	EVENT_LOOKUP,		// for each player
	EVENT_LOOKDOWN,		// for each player
	EVENT_AIMUP,		// for each player
	EVENT_AIMDOWN,		// for each player
	EVENT_FIRE,			// for each player
	EVENT_CHANGEWEAPON,	// for each player
	EVENT_GETSHOTRANGE,	// for each player
	EVENT_GETAUTOAIMANGLE,	// for each player
	EVENT_GETLOADTILE,

	EVENT_CHEATGETSTEROIDS,
	EVENT_CHEATGETHEAT,
	EVENT_CHEATGETBOOT,
	EVENT_CHEATGETSHIELD,
	EVENT_CHEATGETSCUBA,
	EVENT_CHEATGETHOLODUKE,
	EVENT_CHEATGETJETPACK,
	EVENT_CHEATGETFIRSTAID,
	EVENT_QUICKKICK,
	EVENT_INVENTORY,
	EVENT_USENIGHTVISION,
	EVENT_USESTEROIDS,
	EVENT_INVENTORYLEFT,
	EVENT_INVENTORYRIGHT,
	EVENT_HOLODUKEON,
	EVENT_HOLODUKEOFF,
	EVENT_USEMEDKIT,
	EVENT_USEJETPACK,
	EVENT_TURNAROUND,

	EVENT_NUMEVENTS,
	EVENT_MAXEVENT = EVENT_NUMEVENTS - 1
};

extern TArray<int> ScriptCode;


void OnEvent(int id, int pnum = -1, int snum = -1, int dist = -1);

static FORCE_INLINE int32_t VM_OnEvent(int nEventID, int spriteNum=-1, int playerNum=-1, int nDist=-1, int32_t nReturn=0)
{
    // set return
    if (IsGameEvent(nEventID))
    {
        SetGameVarID(g_iReturnVarID, nReturn, spriteNum, playerNum);
        OnEvent(nEventID, spriteNum, playerNum, -1);
        return GetGameVarID(g_iReturnVarID, spriteNum, playerNum);
    }
    return nReturn;
}

static FORCE_INLINE int32_t VM_OnEventWithReturn(int nEventID, int spriteNum, int playerNum, int32_t nReturn)
{
	// set return
    if (IsGameEvent(nEventID))
    {
        SetGameVarID(g_iReturnVarID, nReturn, spriteNum, playerNum);
        OnEvent(nEventID, spriteNum, playerNum, -1);
        return GetGameVarID(g_iReturnVarID, spriteNum, playerNum);
    }
    return nReturn;
}


void execute(int s, int p, int d);

void makeitfall(int s);
int furthestangle(int spriteNum, int angDiv);
void getglobalz(int s);
int getincangle(int c, int n);

void G_GetTimeDate(int32_t * pValues);
int G_StartTrack(int levelNum);

END_DUKE_NS

#endif
