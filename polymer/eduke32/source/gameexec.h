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

#ifdef LUNATIC
# include "lunatic_game.h"
#endif

#ifdef __cplusplus
extern "C" {
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
void A_Fall(int nSprite);
int32_t A_FurthestVisiblePoint(int32_t iActor,uspritetype * const ts,int32_t *dax,int32_t *day);
int32_t A_GetFurthestAngle(int32_t iActor,int32_t angs);
void A_GetZLimits(int32_t iActor);
int G_GetAngleDelta(int a,int na);
void G_RestoreMapState();
void G_SaveMapState();

#if !defined LUNATIC
void VM_DrawTile(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation);
void VM_DrawTilePal(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p);
void VM_DrawTilePalSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p);
void VM_DrawTileSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation);
#else
void VM_DrawTileGeneric(int32_t x, int32_t y, int32_t zoom, int32_t tilenum,
    int32_t shade, int32_t orientation, int32_t p);
#endif

int32_t VM_OnEventWithBoth_(int nEventID, int nSprite, int nPlayer, int nDist, int32_t nReturn);
int32_t VM_OnEventWithReturn_(int nEventID, int nSprite, int nPlayer, int32_t nReturn);
int32_t VM_OnEventWithDist_(int nEventID, int nSprite, int nPlayer, int nDist);
int32_t VM_OnEvent_(int nEventID, int nSprite, int nPlayer);

FORCE_INLINE int VM_HaveEvent(int nEventID)
{
#ifdef LUNATIC
    return L_IsInitialized(&g_ElState) && El_HaveEvent(nEventID);
#else
    return !!apScriptGameEvent[nEventID];
#endif
}

FORCE_INLINE int32_t VM_OnEventWithBoth(int nEventID, int nSprite, int nPlayer, int nDist, int32_t nReturn)
{
    return VM_HaveEvent(nEventID) ? VM_OnEventWithBoth_(nEventID, nSprite, nPlayer, nDist, nReturn) : nReturn;
}

FORCE_INLINE int32_t VM_OnEventWithReturn(int nEventID, int nSprite, int nPlayer, int nReturn)
{
    return VM_HaveEvent(nEventID) ? VM_OnEventWithReturn_(nEventID, nSprite, nPlayer, nReturn) : nReturn;
}

FORCE_INLINE int32_t VM_OnEventWithDist(int nEventID, int nSprite, int nPlayer, int nDist)
{
    return VM_HaveEvent(nEventID) ? VM_OnEventWithDist_(nEventID, nSprite, nPlayer, nDist) : 0;
}

FORCE_INLINE int32_t VM_OnEvent(int nEventID, int nSprite, int nPlayer)
{
    return VM_HaveEvent(nEventID) ? VM_OnEvent_(nEventID, nSprite, nPlayer) : 0;
}

#define CON_ERRPRINTF(Text, ...) do { \
    OSD_Printf("Line %d, %s: " Text, g_errorLineNum, keyw[g_tw], ## __VA_ARGS__); \
} while (0)

void G_GetTimeDate(int32_t *vals);
int G_StartTrack(int nLevel);
int32_t A_Dodge(spritetype *s);
#ifdef LUNATIC
void G_ShowView(vec3_t vec, int32_t a, int32_t horiz, int32_t sect,
                int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t unbiasedp);
void P_AddWeaponMaybeSwitchI(int32_t snum, int32_t weap);
void VM_FallSprite(int32_t i);
int32_t VM_ResetPlayer2(int32_t snum, int32_t flags);
int32_t VM_CheckSquished2(int32_t i, int32_t snum);
#endif

#ifdef __cplusplus
}
#endif

#endif
