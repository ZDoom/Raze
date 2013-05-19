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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#ifndef __gameexec_h__
#define __gameexec_h__

#include "build.h"
#include "sector.h"  // mapstate_t
#include "gamedef.h"  // vmstate_t

// the order of these can't be changed or else compatibility with EDuke 2.0 mods will break
// KEEPINSYNC with EventNames[] and lunatic/con_lang.lua
enum GameEvent_t {
    EVENT_INIT,  // 0
    EVENT_ENTERLEVEL,
    EVENT_RESETWEAPONS,
    EVENT_RESETINVENTORY,
    EVENT_HOLSTER,
    EVENT_LOOKLEFT,  // 5
    EVENT_LOOKRIGHT,
    EVENT_SOARUP,
    EVENT_SOARDOWN,
    EVENT_CROUCH,
    EVENT_JUMP,  // 10
    EVENT_RETURNTOCENTER,
    EVENT_LOOKUP,
    EVENT_LOOKDOWN,
    EVENT_AIMUP,
    EVENT_FIRE,  // 15
    EVENT_CHANGEWEAPON,
    EVENT_GETSHOTRANGE,
    EVENT_GETAUTOAIMANGLE,
    EVENT_GETLOADTILE,
    EVENT_CHEATGETSTEROIDS,  // 20
    EVENT_CHEATGETHEAT,
    EVENT_CHEATGETBOOT,
    EVENT_CHEATGETSHIELD,
    EVENT_CHEATGETSCUBA,
    EVENT_CHEATGETHOLODUKE,  // 25
    EVENT_CHEATGETJETPACK,
    EVENT_CHEATGETFIRSTAID,
    EVENT_QUICKKICK,
    EVENT_INVENTORY,
    EVENT_USENIGHTVISION,  // 30
    EVENT_USESTEROIDS,
    EVENT_INVENTORYLEFT,
    EVENT_INVENTORYRIGHT,
    EVENT_HOLODUKEON,
    EVENT_HOLODUKEOFF,  // 35
    EVENT_USEMEDKIT,
    EVENT_USEJETPACK,
    EVENT_TURNAROUND,
    EVENT_DISPLAYWEAPON,
    EVENT_FIREWEAPON,  // 40
    EVENT_SELECTWEAPON,
    EVENT_MOVEFORWARD,
    EVENT_MOVEBACKWARD,
    EVENT_TURNLEFT,
    EVENT_TURNRIGHT,  // 45
    EVENT_STRAFELEFT,
    EVENT_STRAFERIGHT,
    EVENT_WEAPKEY1,
    EVENT_WEAPKEY2,
    EVENT_WEAPKEY3,  // 50
    EVENT_WEAPKEY4,
    EVENT_WEAPKEY5,
    EVENT_WEAPKEY6,
    EVENT_WEAPKEY7,
    EVENT_WEAPKEY8,  // 55
    EVENT_WEAPKEY9,
    EVENT_WEAPKEY10,
    EVENT_DRAWWEAPON,
    EVENT_DISPLAYCROSSHAIR,
    EVENT_DISPLAYREST,  // 60
    EVENT_DISPLAYSBAR,
    EVENT_RESETPLAYER,
    EVENT_INCURDAMAGE,
    EVENT_AIMDOWN,
    EVENT_GAME,  // 65
    EVENT_PREVIOUSWEAPON,
    EVENT_NEXTWEAPON,
    EVENT_SWIMUP,
    EVENT_SWIMDOWN,
    EVENT_GETMENUTILE,  // 70
    EVENT_SPAWN,
    EVENT_LOGO,
    EVENT_EGS,
    EVENT_DOFIRE,
    EVENT_PRESSEDFIRE,  // 75
    EVENT_USE,
    EVENT_PROCESSINPUT,
    EVENT_FAKEDOMOVETHINGS,
    EVENT_DISPLAYROOMS,
    EVENT_KILLIT,  // 80
    EVENT_LOADACTOR,
    EVENT_DISPLAYBONUSSCREEN,
    EVENT_DISPLAYMENU,
    EVENT_DISPLAYMENUREST,
    EVENT_DISPLAYLOADINGSCREEN,  // 85
    EVENT_ANIMATESPRITES,
    EVENT_NEWGAME,
    EVENT_SOUND,
    EVENT_CHECKTOUCHDAMAGE,
    EVENT_CHECKFLOORDAMAGE,  // 90
    EVENT_LOADGAME,
    EVENT_SAVEGAME,
    EVENT_PREGAME,
    EVENT_CHANGEMENU,
    MAXEVENTS
};

extern int32_t ticrandomseed;

extern vmstate_t vm;
#if !defined LUNATIC
extern int32_t g_tw;
extern int32_t g_errorLineNum;
extern int32_t g_currentEventExec;

void A_LoadActor(int32_t iActor);
#endif

void A_Execute(int32_t iActor,int32_t iPlayer,int32_t lDist);
void A_Fall(int32_t iActor);
int32_t A_FurthestVisiblePoint(int32_t iActor,spritetype *ts,int32_t *dax,int32_t *day);
int32_t A_GetFurthestAngle(int32_t iActor,int32_t angs);
void A_GetZLimits(int32_t iActor);
int32_t G_GetAngleDelta(int32_t a,int32_t na);
void G_RestoreMapState();
void G_SaveMapState();
int32_t VM_OnEvent(int32_t iEventID,int32_t iActor,int32_t iPlayer,int32_t lDist, int32_t iReturn);
void VM_ScriptInfo(void);

#define CON_ERRPRINTF(Text, ...) do { \
    OSD_Printf_nowarn("Line %d, %s: " Text, g_errorLineNum, keyw[g_tw], ## __VA_ARGS__); \
} while (0)

#endif
