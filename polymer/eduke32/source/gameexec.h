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

#ifndef __gameexec_h__
#define __gameexec_h__

#include "build.h"
#include "sector.h"  // mapstate_t
#include "gamedef.h"  // vmstate_t
#include "events_defs.h"

#ifdef LUNATIC
# include "lunatic_game.h"
#endif

extern int32_t ticrandomseed;

extern vmstate_t vm;
#if !defined LUNATIC
extern int32_t g_tw;
extern int32_t g_errorLineNum;
extern int32_t g_currentEventExec;

void A_LoadActor(int32_t iActor);
#endif

void A_Execute(int32_t iActor, int32_t iPlayer, int32_t lDist);
void A_Fall(int32_t iActor);
int32_t A_FurthestVisiblePoint(int32_t iActor,spritetype *ts,int32_t *dax,int32_t *day);
int32_t A_GetFurthestAngle(int32_t iActor,int32_t angs);
void A_GetZLimits(int32_t iActor);
int32_t G_GetAngleDelta(int32_t a,int32_t na);
void G_RestoreMapState();
void G_SaveMapState();

int32_t VM_OnEventWithBoth_(int32_t iEventID, int32_t iActor, int32_t iPlayer, int32_t lDist, int32_t iReturn);
int32_t VM_OnEventWithReturn_(int32_t iEventID, int32_t iActor, int32_t iPlayer, int32_t iReturn);
int32_t VM_OnEventWithDist_(int32_t iEventID, int32_t iActor, int32_t iPlayer, int32_t lDist);
int32_t VM_OnEvent_(int32_t iEventID, int32_t iActor, int32_t iPlayer);

static inline int32_t VM_HaveEvent(int32_t iEventID)
{
#ifdef LUNATIC
    return L_IsInitialized(&g_ElState) && El_HaveEvent(iEventID);
#else
    return apScriptGameEvent[iEventID]!=NULL;
#endif
}

static inline int32_t VM_OnEventWithBoth(int32_t iEventID, int32_t iActor, int32_t iPlayer, int32_t lDist, int32_t iReturn)
{
    return VM_HaveEvent(iEventID) ? VM_OnEventWithBoth_(iEventID, iActor, iPlayer, lDist, iReturn) : iReturn;
}

static inline int32_t VM_OnEventWithReturn(int32_t iEventID, int32_t iActor, int32_t iPlayer, int32_t iReturn)
{
    return VM_HaveEvent(iEventID) ? VM_OnEventWithReturn_(iEventID, iActor, iPlayer, iReturn) : iReturn;
}

static inline int32_t VM_OnEventWithDist(int32_t iEventID, int32_t iActor, int32_t iPlayer, int32_t lDist)
{
    return VM_HaveEvent(iEventID) ? VM_OnEventWithDist_(iEventID, iActor, iPlayer, lDist) : 0;
}

static inline int32_t VM_OnEvent(int32_t iEventID, int32_t iActor, int32_t iPlayer)
{
    return VM_HaveEvent(iEventID) ? VM_OnEvent_(iEventID, iActor, iPlayer) : 0;
}

void VM_ScriptInfo(void);

#define CON_ERRPRINTF(Text, ...) do { \
    OSD_Printf_nowarn("Line %d, %s: " Text, g_errorLineNum, keyw[g_tw], ## __VA_ARGS__); \
} while (0)

#endif
