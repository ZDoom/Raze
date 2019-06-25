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
#include "events_defs.h"
#include "gamedef.h"  // vmstate_t
#include "sector.h"  // mapstate_t

#ifdef LUNATIC
# include "lunatic_game.h"
#endif

int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum, int const nDist, int32_t const nReturn);
int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum, int const nDist);
int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum);
int32_t VM_ExecuteEventWithValue(int const nEventID, int const spriteNum, int const playerNum, int32_t const nReturn);

static FORCE_INLINE bool VM_HaveEvent(int const nEventID)
{
#ifdef LUNATIC
    return L_IsInitialized(&g_ElState) && El_HaveEvent(nEventID);
#else
    return !!apScriptEvents[nEventID];
#endif
}

static FORCE_INLINE int32_t VM_OnEvent(int nEventID, int spriteNum, int playerNum, int nDist, int32_t nReturn)
{
    return VM_HaveEvent(nEventID) ? VM_ExecuteEvent(nEventID, spriteNum, playerNum, nDist, nReturn) : nReturn;
}

static FORCE_INLINE int32_t VM_OnEvent(int nEventID, int spriteNum, int playerNum, int nDist)
{
    return VM_HaveEvent(nEventID) ? VM_ExecuteEvent(nEventID, spriteNum, playerNum, nDist) : 0;
}

static FORCE_INLINE int32_t VM_OnEvent(int nEventID, int spriteNum = -1, int playerNum = -1)
{
    return VM_HaveEvent(nEventID) ? VM_ExecuteEvent(nEventID, spriteNum, playerNum) : 0;
}

static FORCE_INLINE int32_t VM_OnEventWithReturn(int nEventID, int spriteNum, int playerNum, int32_t nReturn)
{
    return VM_HaveEvent(nEventID) ? VM_ExecuteEventWithValue(nEventID, spriteNum, playerNum, nReturn) : nReturn;
}

#ifdef __cplusplus
extern "C" {
#endif

enum vmflags_t
{
    VM_RETURN    = 0x00000001,
    VM_KILL      = 0x00000002,
    VM_NOEXECUTE = 0x00000004,
};

extern int32_t ticrandomseed;

extern vmstate_t vm;
#if !defined LUNATIC
extern int32_t g_tw;
extern int32_t g_currentEvent;

void A_LoadActor(int const spriteNum);
#endif

extern uint32_t g_eventCalls[MAXEVENTS], g_actorCalls[MAXTILES];
extern double g_eventTotalMs[MAXEVENTS], g_actorTotalMs[MAXTILES], g_actorMinMs[MAXTILES], g_actorMaxMs[MAXTILES];

void A_Execute(int spriteNum, int playerNum, int playerDist);
void A_Fall(int spriteNum);
int A_GetFurthestAngle(int const spriteNum, int const angDiv);
void A_GetZLimits(int spriteNum);
int __fastcall G_GetAngleDelta(int currAngle, int newAngle);
void G_RestoreMapState();
void G_SaveMapState();

void VM_DrawTileGeneric(int32_t x, int32_t y, int32_t zoom, int32_t tilenum,
    int32_t shade, int32_t orientation, int32_t p);

#if !defined LUNATIC
void VM_DrawTile(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation);
static inline void VM_DrawTilePal(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    VM_DrawTileGeneric(x, y, 65536, tilenum, shade, orientation, p);
}
static inline void VM_DrawTilePalSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    VM_DrawTileGeneric(x, y, 32768, tilenum, shade, orientation, p);
}
void VM_DrawTileSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation);
#endif

#define CON_ERRPRINTF(Text, ...) do { \
    vm.flags |= VM_RETURN; \
    OSD_Printf("Line %d, %s: " Text, VM_DECODE_LINE_NUMBER(g_tw), VM_GetKeywordForID(VM_DECODE_INST(g_tw)), ## __VA_ARGS__); \
} while (0)

#define CON_CRITICALERRPRINTF(Text, ...) do { \
    vm.flags |= VM_RETURN; \
    OSD_Printf("Line %d, %s: " Text, VM_DECODE_LINE_NUMBER(g_tw), VM_GetKeywordForID(VM_DECODE_INST(g_tw)), ## __VA_ARGS__); \
    wm_msgbox(APPNAME, "Line %d, %s: " Text, VM_DECODE_LINE_NUMBER(g_tw), VM_GetKeywordForID(VM_DECODE_INST(g_tw)), ## __VA_ARGS__); \
} while (0)

void G_GetTimeDate(int32_t * pValues);
int G_StartTrack(int levelNum);
#ifdef LUNATIC
void G_ShowView(vec3_t vec, fix16_t a, fix16_t horiz, int sect,
                int ix1, int iy1, int ix2, int iy2, bool unbiasedp);
void P_AddWeaponMaybeSwitchI(int32_t snum, int32_t weap);
void VM_FallSprite(int32_t i);
int32_t VM_ResetPlayer2(int32_t snum, int32_t flags);
int32_t VM_CheckSquished2(int32_t i, int32_t snum);
#endif

void VM_UpdateAnim(int const spriteNum, int32_t * const pData);
void VM_GetZRange(int const spriteNum, int32_t * const ceilhit, int32_t * const florhit, int const wallDist);

#ifdef __cplusplus
}
#endif

#endif
