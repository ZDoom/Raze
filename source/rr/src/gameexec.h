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
#include "sector.h"  // mapstate_t
#include "gamedef.h"  // vmstate_t
#include "events_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

extern int32_t ticrandomseed;

extern vmstate_t vm;
extern int32_t g_tw;
extern int32_t g_errorLineNum;
extern int32_t g_currentEventExec;

void A_LoadActor(int32_t spriteNum);

extern uint32_t g_actorCalls[MAXTILES];
extern double g_actorTotalMs[MAXTILES], g_actorMinMs[MAXTILES], g_actorMaxMs[MAXTILES];

void A_Execute(int spriteNum, int playerNum, int playerDist);
void A_Fall(int spriteNum);
int32_t A_GetFurthestAngle(int spriteNum, int angDiv);
void A_GetZLimits(int spriteNum);
int32_t __fastcall G_GetAngleDelta(int32_t currAngle, int32_t newAngle);
//void G_RestoreMapState();
//void G_SaveMapState();

#define CON_ERRPRINTF(Text, ...) do { \
    OSD_Printf("Line %d, %s: " Text, g_errorLineNum, VM_GetKeywordForID(g_tw), ## __VA_ARGS__); \
} while (0)

#define CON_CRITICALERRPRINTF(Text, ...) do { \
    OSD_Printf("Line %d, %s: " Text, g_errorLineNum, VM_GetKeywordForID(g_tw), ## __VA_ARGS__); \
    wm_msgbox(APPNAME, "Line %d, %s: " Text, g_errorLineNum, VM_GetKeywordForID(g_tw), ## __VA_ARGS__); \
} while (0)

void G_GetTimeDate(int32_t * pValues);
int G_StartTrack(int levelNum);
void VM_UpdateAnim(int spriteNum, int32_t *pData);

#ifdef __cplusplus
}
#endif

#endif
