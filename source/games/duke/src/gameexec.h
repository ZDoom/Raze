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

BEGIN_DUKE_NS
int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum, int const nDist, int32_t const nReturn);
int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum, int const nDist);
int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum);
int32_t VM_ExecuteEventWithValue(int const nEventID, int const spriteNum, int const playerNum, int32_t const nReturn);

static FORCE_INLINE int VM_HaveEvent(int const nEventID)
{
    return !!apScriptGameEvent[nEventID];
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

inline int OnEvent(int id, int pnum, int snum, int what)
{
    return VM_OnEvent(id, snum, pnum, what);
}

static FORCE_INLINE int32_t VM_OnEventWithReturn(int nEventID, int spriteNum, int playerNum, int32_t nReturn)
{
    return VM_HaveEvent(nEventID) ? VM_ExecuteEventWithValue(nEventID, spriteNum, playerNum, nReturn) : nReturn;
}


extern int32_t ticrandomseed;

extern int32_t g_tw;
extern int32_t g_currentEvent;
extern int32_t g_errorLineNum;

void execute(int s, int p, int d);
void makeitfall(int s);
int furthestangle(int spriteNum, int angDiv);
void getglobalz(int s);
int getincangle(int c, int n);
//void G_RestoreMapState();
//void G_SaveMapState();

#define CON_ERRPRINTF(Text, ...) do { \
    Printf("Line %d, %s: " Text, g_errorLineNum, VM_GetKeywordForID(g_tw), ## __VA_ARGS__); \
} while (0)

#define CON_CRITICALERRPRINTF(Text, ...) do { \
    I_Error("Line %d, %s: " Text, VM_DECODE_LINE_NUMBER(g_tw), VM_GetKeywordForID(VM_DECODE_INST(g_tw)), ## __VA_ARGS__); \
} while (0)

void G_GetTimeDate(int32_t * pValues);
int G_StartTrack(int levelNum);
void VM_UpdateAnim(int spriteNum, int32_t *pData);

END_DUKE_NS

#endif
