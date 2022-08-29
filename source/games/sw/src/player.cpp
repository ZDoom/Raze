//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"

#include "build.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "player.h"
#include "lists.h"

#include "gamecontrol.h"

#include "menus.h"
#include "network.h"
#include "pal.h"
#include "mclip.h"

#include "sprite.h"
#include "weapon.h"
#include "break.h"
#include "jsector.h"
#include "sector.h"
#include "misc.h"
#include "interpolate.h"
#include "interpso.h"
#include "razemenu.h"
#include "gstrings.h"
#include "raze_music.h"
#include "v_draw.h"
#include "gamestate.h"
#include "vm.h"

BEGIN_SW_NS

void pSpriteControl(PLAYER* pp);
int WeaponOperate(PLAYER* pp);
SECTOR_OBJECT* PlayerOnObject(sectortype* sect_match);
void PlayerRemoteReset(PLAYER* pp, sectortype* sect);
void KillAllPanelInv(PLAYER* pp);
void DoPlayerDeathDrown(PLAYER* pp);
void pWeaponForceRest(PLAYER* pp);

#define SO_DRIVE_SOUND 2
#define SO_IDLE_SOUND 1

extern bool NoMeters;

#define PLAYER_CRAWL_WADE_DEPTH (30)

USERSAVE puser[MAX_SW_PLAYERS_REG];

//int16_t gNet.MultiGameType = MULTI_GAME_NONE;
bool NightVision = false;
extern int FinishAnim;


// the smaller the number the slower the going
#define PLAYER_RUN_FRICTION (50000L)
//#define PLAYER_RUN_FRICTION 0xcb00
#define PLAYER_JUMP_FRICTION PLAYER_RUN_FRICTION
#define PLAYER_FALL_FRICTION PLAYER_RUN_FRICTION

#define PLAYER_WADE_FRICTION PLAYER_RUN_FRICTION
#define PLAYER_FLY_FRICTION (55808L)

#define PLAYER_CRAWL_FRICTION (45056L)
#define PLAYER_SWIM_FRICTION (49152L)
#define PLAYER_DIVE_FRICTION (49152L)

// only for z direction climbing
#define PLAYER_CLIMB_FRICTION (45056L)

//#define BOAT_FRICTION 0xd000
#define BOAT_FRICTION 0xcb00
//#define TANK_FRICTION 0xcb00
#define TANK_FRICTION (53248L)
#define PLAYER_SLIDE_FRICTION (53248L)

#define JUMP_STUFF 4

// just like 2 except can jump higher - less gravity
// goes better with slightly slower run speed than I had it at
#if JUMP_STUFF == 4
#define PLAYER_JUMP_GRAV 24
#define PLAYER_JUMP_AMT (-650)
#define PLAYER_CLIMB_JUMP_AMT (-1100)
#define MAX_JUMP_DURATION 12
uint8_t PlayerGravity = PLAYER_JUMP_GRAV;
#endif

bool ToggleFlyMode = false;

extern bool DebugOperate;

//uint8_t synctics, lastsynctics;

int ChopTics;

PLAYER Player[MAX_SW_PLAYERS_REG + 1];

// These are a bunch of kens variables for the player

short NormalVisibility;

DSWActor* FindNearSprite(DSWActor*, short stat);
bool PlayerOnLadder(PLAYER* pp);
void DoPlayerSlide(PLAYER* pp);
void DoPlayerBeginSwim(PLAYER* pp);
void DoPlayerSwim(PLAYER* pp);
void DoPlayerWade(PLAYER* pp);
void DoPlayerBeginWade(PLAYER* pp);
void DoPlayerBeginCrawl(PLAYER* pp);
void DoPlayerCrawl(PLAYER* pp);
void DoPlayerRun(PLAYER* pp);
void DoPlayerBeginRun(PLAYER* pp);
void DoPlayerFall(PLAYER* pp);
void DoPlayerBeginFall(PLAYER* pp);
void DoPlayerJump(PLAYER* pp);
void DoPlayerBeginJump(PLAYER* pp);
void DoPlayerForceJump(PLAYER* pp);
void DoPlayerBeginFly(PLAYER* pp);
void DoPlayerFly(PLAYER* pp);
void DoPlayerBeginClimb(PLAYER* pp);
void DoPlayerClimb(PLAYER* pp);
void DoPlayerBeginDie(PLAYER* pp);
void DoPlayerDie(PLAYER* pp);
// void DoPlayerBeginOperateBoat(PLAYER* pp);
void DoPlayerBeginOperateVehicle(PLAYER* pp);
void DoPlayerBeginOperate(PLAYER* pp);
// void DoPlayerOperateBoat(PLAYER* pp);
void DoPlayerOperateVehicle(PLAYER* pp);
void DoPlayerOperateTurret(PLAYER* pp);
void DoPlayerBeginDive(PLAYER* pp);
void DoPlayerDive(PLAYER* pp);
void DoPlayerTeleportPause(PLAYER* pp);
bool PlayerFlyKey(void);
void OperateSectorObject(SECTOR_OBJECT* sop, short newang, const DVector2& newpos);
void CheckFootPrints(PLAYER* pp);
bool DoPlayerTestCrawl(PLAYER* pp);
void DoPlayerDeathFlip(PLAYER* pp);
void DoPlayerDeathCrumble(PLAYER* pp);
void DoPlayerDeathExplode(PLAYER* pp);
void DoPlayerDeathFall(PLAYER* pp);

void PlayerCheckValidMove(PLAYER* pp);
void PlayerWarpUpdatePos(PLAYER* pp);
void DoPlayerBeginDiveNoWarp(PLAYER* pp);
int PlayerCanDiveNoWarp(PLAYER* pp);
void DoPlayerCurrent(PLAYER* pp);
int GetOverlapSector2(int x, int y, sectortype** over, sectortype** under);
void PlayerToRemote(PLAYER* pp);
void PlayerRemoteInit(PLAYER* pp);
void PlayerSpawnPosition(PLAYER* pp);

extern short target_ang;

//////////////////////
//
// PLAYER SPECIFIC
//
//////////////////////

#if 1
#define PLAYER_NINJA_RATE 14

int DoFootPrints(DSWActor* actor);

STATE s_PlayerNinjaRun[5][6] =
{

    {
        {PLAYER_NINJA_RUN_R0 + 0, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[0][1]},
        {PLAYER_NINJA_RUN_R0 + 1, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[0][2]},
        {PLAYER_NINJA_RUN_R0 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[0][3]},
        {PLAYER_NINJA_RUN_R0 + 2, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[0][4]},
        {PLAYER_NINJA_RUN_R0 + 3, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[0][5]},
        {PLAYER_NINJA_RUN_R0 + 3, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[0][0]},
    },
    {
        {PLAYER_NINJA_RUN_R1 + 0, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[1][1]},
        {PLAYER_NINJA_RUN_R1 + 1, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[1][2]},
        {PLAYER_NINJA_RUN_R1 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[1][3]},
        {PLAYER_NINJA_RUN_R1 + 2, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[1][4]},
        {PLAYER_NINJA_RUN_R1 + 3, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[1][5]},
        {PLAYER_NINJA_RUN_R1 + 3, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[1][0]},
    },
    {
        {PLAYER_NINJA_RUN_R2 + 0, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[2][1]},
        {PLAYER_NINJA_RUN_R2 + 1, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[2][2]},
        {PLAYER_NINJA_RUN_R2 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[2][3]},
        {PLAYER_NINJA_RUN_R2 + 2, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[2][4]},
        {PLAYER_NINJA_RUN_R2 + 3, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[2][5]},
        {PLAYER_NINJA_RUN_R2 + 3, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[2][0]},
    },
    {
        {PLAYER_NINJA_RUN_R3 + 0, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[3][1]},
        {PLAYER_NINJA_RUN_R3 + 1, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[3][2]},
        {PLAYER_NINJA_RUN_R3 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[3][3]},
        {PLAYER_NINJA_RUN_R3 + 2, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[3][4]},
        {PLAYER_NINJA_RUN_R3 + 3, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[3][5]},
        {PLAYER_NINJA_RUN_R3 + 3, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[3][0]},
    },
    {
        {PLAYER_NINJA_RUN_R4 + 0, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[4][1]},
        {PLAYER_NINJA_RUN_R4 + 1, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[4][2]},
        {PLAYER_NINJA_RUN_R4 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[4][3]},
        {PLAYER_NINJA_RUN_R4 + 2, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[4][4]},
        {PLAYER_NINJA_RUN_R4 + 3, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[4][5]},
        {PLAYER_NINJA_RUN_R4 + 3, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[4][0]},
    },

};

STATE* sg_PlayerNinjaRun[] =
{
    s_PlayerNinjaRun[0],
    s_PlayerNinjaRun[1],
    s_PlayerNinjaRun[2],
    s_PlayerNinjaRun[3],
    s_PlayerNinjaRun[4]
};
#else
#define PLAYER_NINJA_RATE 10

STATE s_PlayerNinjaRun[5][8] =
{

    {
        {PLAYER_NINJA_RUN_R0 + 0, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[0][1]},
        {PLAYER_NINJA_RUN_R0 + 0, PLAYER_NINJA_RATE | SF_PLAYER_FUNC,DoFootPrints, &s_PlayerNinjaRun[0][2]},
        {PLAYER_NINJA_RUN_R0 + 1, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[0][3]},
        {PLAYER_NINJA_RUN_R0 + 2, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[0][4]},
        {PLAYER_NINJA_RUN_R0 + 3, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[0][5]},
        {PLAYER_NINJA_RUN_R0 + 4, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[0][6]},
        {PLAYER_NINJA_RUN_R0 + 5, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[0][7]},
        {PLAYER_NINJA_RUN_R0 + 5, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[0][0]},
    },
    {
        {PLAYER_NINJA_RUN_R1 + 0, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[1][1]},
        {PLAYER_NINJA_RUN_R1 + 0, PLAYER_NINJA_RATE | SF_PLAYER_FUNC,DoFootPrints, &s_PlayerNinjaRun[1][2]},
        {PLAYER_NINJA_RUN_R1 + 1, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[1][3]},
        {PLAYER_NINJA_RUN_R1 + 2, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[1][4]},
        {PLAYER_NINJA_RUN_R1 + 3, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[1][5]},
        {PLAYER_NINJA_RUN_R1 + 4, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[1][6]},
        {PLAYER_NINJA_RUN_R1 + 5, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[1][7]},
        {PLAYER_NINJA_RUN_R1 + 5, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[1][0]},
    },
    {
        {PLAYER_NINJA_RUN_R2 + 0, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[2][1]},
        {PLAYER_NINJA_RUN_R2 + 0, PLAYER_NINJA_RATE | SF_PLAYER_FUNC,DoFootPrints, &s_PlayerNinjaRun[2][2]},
        {PLAYER_NINJA_RUN_R2 + 1, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[2][3]},
        {PLAYER_NINJA_RUN_R2 + 2, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[2][4]},
        {PLAYER_NINJA_RUN_R2 + 3, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[2][5]},
        {PLAYER_NINJA_RUN_R2 + 4, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[2][6]},
        {PLAYER_NINJA_RUN_R2 + 5, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[2][7]},
        {PLAYER_NINJA_RUN_R2 + 5, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[2][0]},
    },
    {
        {PLAYER_NINJA_RUN_R3 + 0, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[3][1]},
        {PLAYER_NINJA_RUN_R3 + 0, PLAYER_NINJA_RATE | SF_PLAYER_FUNC,DoFootPrints, &s_PlayerNinjaRun[3][2]},
        {PLAYER_NINJA_RUN_R3 + 1, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[3][3]},
        {PLAYER_NINJA_RUN_R3 + 2, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[3][4]},
        {PLAYER_NINJA_RUN_R3 + 3, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[3][5]},
        {PLAYER_NINJA_RUN_R3 + 4, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[3][6]},
        {PLAYER_NINJA_RUN_R3 + 5, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[3][7]},
        {PLAYER_NINJA_RUN_R3 + 5, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[3][0]},
    },
    {
        {PLAYER_NINJA_RUN_R4 + 0, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[4][1]},
        {PLAYER_NINJA_RUN_R4 + 0, PLAYER_NINJA_RATE | SF_PLAYER_FUNC,DoFootPrints, &s_PlayerNinjaRun[4][2]},
        {PLAYER_NINJA_RUN_R4 + 1, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[4][3]},
        {PLAYER_NINJA_RUN_R4 + 2, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[4][4]},
        {PLAYER_NINJA_RUN_R4 + 3, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[4][5]},
        {PLAYER_NINJA_RUN_R4 + 4, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[4][6]},
        {PLAYER_NINJA_RUN_R4 + 5, PLAYER_NINJA_RATE | SF_TIC_ADJUST, NullAnimator, &s_PlayerNinjaRun[4][7]},
        {PLAYER_NINJA_RUN_R4 + 5, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaRun[4][0]},
    }
};

STATE* sg_PlayerNinjaRun[] =
{
    s_PlayerNinjaRun[0],
    s_PlayerNinjaRun[1],
    s_PlayerNinjaRun[2],
    s_PlayerNinjaRun[3],
    s_PlayerNinjaRun[4]
};
#endif

//////////////////////
//
// PLAYER_NINJA STAND
//
//////////////////////

#define PLAYER_NINJA_STAND_RATE 10

STATE s_PlayerNinjaStand[5][1] =
{
    {
        {PLAYER_NINJA_STAND_R0 + 0, PLAYER_NINJA_STAND_RATE, NullAnimator, &s_PlayerNinjaStand[0][0]},
    },
    {
        {PLAYER_NINJA_STAND_R1 + 0, PLAYER_NINJA_STAND_RATE, NullAnimator, &s_PlayerNinjaStand[1][0]},
    },
    {
        {PLAYER_NINJA_STAND_R2 + 0, PLAYER_NINJA_STAND_RATE, NullAnimator, &s_PlayerNinjaStand[2][0]},
    },
    {
        {PLAYER_NINJA_STAND_R3 + 0, PLAYER_NINJA_STAND_RATE, NullAnimator, &s_PlayerNinjaStand[3][0]},
    },
    {
        {PLAYER_NINJA_STAND_R4 + 0, PLAYER_NINJA_STAND_RATE, NullAnimator, &s_PlayerNinjaStand[4][0]},
    },
};
STATE* sg_PlayerNinjaStand[] =
{
    s_PlayerNinjaStand[0],
    s_PlayerNinjaStand[1],
    s_PlayerNinjaStand[2],
    s_PlayerNinjaStand[3],
    s_PlayerNinjaStand[4]
};


#define PLAYER_NINJA_STAR_RATE 12

extern STATE* sg_NinjaRun[];
int DoPlayerSpriteReset(DSWActor* actor);

#if 0
STATE s_PlayerNinjaThrow[5][4] =
{
    {
        {PLAYER_SHOOT_R0 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][1]},
        {PLAYER_SHOOT_R0 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][2]},
        {PLAYER_SHOOT_R0 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][3]},
        {PLAYER_SHOOT_R0 + 0, PLAYER_NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_SHOOT_R1 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][1]},
        {PLAYER_SHOOT_R1 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][2]},
        {PLAYER_SHOOT_R1 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][3]},
        {PLAYER_SHOOT_R1 + 0, PLAYER_NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_SHOOT_R2 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][1]},
        {PLAYER_SHOOT_R2 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][2]},
        {PLAYER_SHOOT_R2 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][3]},
        {PLAYER_SHOOT_R2 + 0, PLAYER_NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_SHOOT_R3 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][1]},
        {PLAYER_SHOOT_R3 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][2]},
        {PLAYER_SHOOT_R3 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][3]},
        {PLAYER_SHOOT_R3 + 0, PLAYER_NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_SHOOT_R4 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][1]},
        {PLAYER_SHOOT_R4 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][2]},
        {PLAYER_SHOOT_R4 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][3]},
        {PLAYER_SHOOT_R4 + 0, PLAYER_NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
};
#endif

#if 1
STATE s_PlayerNinjaThrow[5][4] =
{
    {
        {PLAYER_NINJA_SHOOT_R0 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][1]},
        {PLAYER_NINJA_SHOOT_R0 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][2]},
        {PLAYER_NINJA_SHOOT_R0 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][3]},
        {PLAYER_NINJA_SHOOT_R0 + 0, PLAYER_NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_NINJA_SHOOT_R1 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][1]},
        {PLAYER_NINJA_SHOOT_R1 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][2]},
        {PLAYER_NINJA_SHOOT_R1 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][3]},
        {PLAYER_NINJA_SHOOT_R1 + 0, PLAYER_NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_NINJA_SHOOT_R2 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][1]},
        {PLAYER_NINJA_SHOOT_R2 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][2]},
        {PLAYER_NINJA_SHOOT_R2 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][3]},
        {PLAYER_NINJA_SHOOT_R2 + 0, PLAYER_NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_NINJA_SHOOT_R3 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][1]},
        {PLAYER_NINJA_SHOOT_R3 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][2]},
        {PLAYER_NINJA_SHOOT_R3 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][3]},
        {PLAYER_NINJA_SHOOT_R3 + 0, PLAYER_NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_NINJA_SHOOT_R4 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][1]},
        {PLAYER_NINJA_SHOOT_R4 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][2]},
        {PLAYER_NINJA_SHOOT_R4 + 0, PLAYER_NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][3]},
        {PLAYER_NINJA_SHOOT_R4 + 0, PLAYER_NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
};
#endif

STATE* sg_PlayerNinjaThrow[] =
{
    s_PlayerNinjaThrow[0],
    s_PlayerNinjaThrow[1],
    s_PlayerNinjaThrow[2],
    s_PlayerNinjaThrow[3],
    s_PlayerNinjaThrow[4]
};

//////////////////////
//
// PLAYER_NINJA JUMP
//
//////////////////////

#define PLAYER_NINJA_JUMP_RATE 24

STATE s_PlayerNinjaJump[5][4] =
{
    {
        {PLAYER_NINJA_JUMP_R0 + 0, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[0][1]},
        {PLAYER_NINJA_JUMP_R0 + 1, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[0][2]},
        {PLAYER_NINJA_JUMP_R0 + 2, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[0][3]},
        {PLAYER_NINJA_JUMP_R0 + 3, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[0][3]},
    },
    {
        {PLAYER_NINJA_JUMP_R1 + 0, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[1][1]},
        {PLAYER_NINJA_JUMP_R1 + 1, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[1][2]},
        {PLAYER_NINJA_JUMP_R1 + 2, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[1][3]},
        {PLAYER_NINJA_JUMP_R1 + 3, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[1][3]},
    },
    {
        {PLAYER_NINJA_JUMP_R2 + 0, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[2][1]},
        {PLAYER_NINJA_JUMP_R2 + 1, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[2][2]},
        {PLAYER_NINJA_JUMP_R2 + 2, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[2][3]},
        {PLAYER_NINJA_JUMP_R2 + 3, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[2][3]},
    },
    {
        {PLAYER_NINJA_JUMP_R3 + 0, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[3][1]},
        {PLAYER_NINJA_JUMP_R3 + 1, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[3][2]},
        {PLAYER_NINJA_JUMP_R3 + 2, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[3][3]},
        {PLAYER_NINJA_JUMP_R3 + 3, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[3][3]},
    },
    {
        {PLAYER_NINJA_JUMP_R4 + 0, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[4][1]},
        {PLAYER_NINJA_JUMP_R4 + 1, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[4][2]},
        {PLAYER_NINJA_JUMP_R4 + 2, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[4][3]},
        {PLAYER_NINJA_JUMP_R4 + 3, PLAYER_NINJA_JUMP_RATE, NullAnimator, &s_PlayerNinjaJump[4][3]},
    },
};


STATE* sg_PlayerNinjaJump[] =
{
    s_PlayerNinjaJump[0],
    s_PlayerNinjaJump[1],
    s_PlayerNinjaJump[2],
    s_PlayerNinjaJump[3],
    s_PlayerNinjaJump[4]
};


//////////////////////
//
// PLAYER_NINJA FALL
//
//////////////////////

#define PLAYER_NINJA_FALL_RATE 16

STATE s_PlayerNinjaFall[5][2] =
{
    {
        {PLAYER_NINJA_JUMP_R0 + 1, PLAYER_NINJA_FALL_RATE, NullAnimator, &s_PlayerNinjaFall[0][1]},
        {PLAYER_NINJA_JUMP_R0 + 2, PLAYER_NINJA_FALL_RATE, NullAnimator, &s_PlayerNinjaFall[0][1]},
    },
    {
        {PLAYER_NINJA_JUMP_R1 + 1, PLAYER_NINJA_FALL_RATE, NullAnimator, &s_PlayerNinjaFall[1][1]},
        {PLAYER_NINJA_JUMP_R1 + 2, PLAYER_NINJA_FALL_RATE, NullAnimator, &s_PlayerNinjaFall[1][1]},
    },
    {
        {PLAYER_NINJA_JUMP_R2 + 1, PLAYER_NINJA_FALL_RATE, NullAnimator, &s_PlayerNinjaFall[2][1]},
        {PLAYER_NINJA_JUMP_R2 + 2, PLAYER_NINJA_FALL_RATE, NullAnimator, &s_PlayerNinjaFall[2][1]},
    },
    {
        {PLAYER_NINJA_JUMP_R3 + 1, PLAYER_NINJA_FALL_RATE, NullAnimator, &s_PlayerNinjaFall[3][1]},
        {PLAYER_NINJA_JUMP_R3 + 2, PLAYER_NINJA_FALL_RATE, NullAnimator, &s_PlayerNinjaFall[3][1]},
    },
    {
        {PLAYER_NINJA_JUMP_R4 + 1, PLAYER_NINJA_FALL_RATE, NullAnimator, &s_PlayerNinjaFall[4][1]},
        {PLAYER_NINJA_JUMP_R4 + 2, PLAYER_NINJA_FALL_RATE, NullAnimator, &s_PlayerNinjaFall[4][1]},
    },
};


STATE* sg_PlayerNinjaFall[] =
{
    s_PlayerNinjaFall[0],
    s_PlayerNinjaFall[1],
    s_PlayerNinjaFall[2],
    s_PlayerNinjaFall[3],
    s_PlayerNinjaFall[4]
};

//////////////////////
//
// PLAYER_NINJA CLIMB
//
//////////////////////


#define PLAYER_NINJA_CLIMB_RATE 20
STATE s_PlayerNinjaClimb[5][4] =
{
    {
        {PLAYER_NINJA_CLIMB_R0 + 0, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[0][1]},
        {PLAYER_NINJA_CLIMB_R0 + 1, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[0][2]},
        {PLAYER_NINJA_CLIMB_R0 + 2, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[0][3]},
        {PLAYER_NINJA_CLIMB_R0 + 3, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[0][0]},
    },
    {
        {PLAYER_NINJA_CLIMB_R1 + 0, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[1][1]},
        {PLAYER_NINJA_CLIMB_R1 + 1, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[1][2]},
        {PLAYER_NINJA_CLIMB_R1 + 2, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[1][3]},
        {PLAYER_NINJA_CLIMB_R1 + 3, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[1][0]},
    },
    {
        {PLAYER_NINJA_CLIMB_R2 + 0, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[2][1]},
        {PLAYER_NINJA_CLIMB_R2 + 1, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[2][2]},
        {PLAYER_NINJA_CLIMB_R2 + 2, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[2][3]},
        {PLAYER_NINJA_CLIMB_R2 + 3, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[2][0]},
    },
    {
        {PLAYER_NINJA_CLIMB_R3 + 0, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[3][1]},
        {PLAYER_NINJA_CLIMB_R3 + 1, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[3][2]},
        {PLAYER_NINJA_CLIMB_R3 + 2, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[3][3]},
        {PLAYER_NINJA_CLIMB_R3 + 3, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[3][0]},
    },
    {
        {PLAYER_NINJA_CLIMB_R4 + 0, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[4][1]},
        {PLAYER_NINJA_CLIMB_R4 + 1, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[4][2]},
        {PLAYER_NINJA_CLIMB_R4 + 2, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[4][3]},
        {PLAYER_NINJA_CLIMB_R4 + 3, PLAYER_NINJA_CLIMB_RATE, NullAnimator, &s_PlayerNinjaClimb[4][0]},
    },
};

STATE* sg_PlayerNinjaClimb[] =
{
    s_PlayerNinjaClimb[0],
    s_PlayerNinjaClimb[1],
    s_PlayerNinjaClimb[2],
    s_PlayerNinjaClimb[3],
    s_PlayerNinjaClimb[4]
};

//////////////////////
//
// PLAYER_NINJA CRAWL
//
//////////////////////


#define PLAYER_NINJA_CRAWL_RATE 14
STATE s_PlayerNinjaCrawl[5][6] =
{
    {
        {PLAYER_NINJA_CRAWL_R0 + 0, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[0][1]},
        {PLAYER_NINJA_CRAWL_R0 + 1, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[0][2]},
        {PLAYER_NINJA_CRAWL_R0 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaCrawl[0][3]},
        {PLAYER_NINJA_CRAWL_R0 + 2, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[0][4]},
        {PLAYER_NINJA_CRAWL_R0 + 1, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[0][5]},
        {PLAYER_NINJA_CRAWL_R0 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaCrawl[0][0]},
    },
    {
        {PLAYER_NINJA_CRAWL_R1 + 0, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[1][1]},
        {PLAYER_NINJA_CRAWL_R1 + 1, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[1][2]},
        {PLAYER_NINJA_CRAWL_R1 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaCrawl[1][3]},
        {PLAYER_NINJA_CRAWL_R1 + 2, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[1][4]},
        {PLAYER_NINJA_CRAWL_R1 + 1, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[1][5]},
        {PLAYER_NINJA_CRAWL_R1 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaCrawl[1][0]},
    },
    {
        {PLAYER_NINJA_CRAWL_R2 + 0, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[2][1]},
        {PLAYER_NINJA_CRAWL_R2 + 1, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[2][2]},
        {PLAYER_NINJA_CRAWL_R2 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaCrawl[2][3]},
        {PLAYER_NINJA_CRAWL_R2 + 2, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[2][4]},
        {PLAYER_NINJA_CRAWL_R2 + 1, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[2][5]},
        {PLAYER_NINJA_CRAWL_R2 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaCrawl[2][0]},
    },
    {
        {PLAYER_NINJA_CRAWL_R3 + 0, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[3][1]},
        {PLAYER_NINJA_CRAWL_R3 + 1, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[3][2]},
        {PLAYER_NINJA_CRAWL_R3 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaCrawl[3][3]},
        {PLAYER_NINJA_CRAWL_R3 + 2, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[3][4]},
        {PLAYER_NINJA_CRAWL_R3 + 1, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[3][5]},
        {PLAYER_NINJA_CRAWL_R3 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaCrawl[3][0]},
    },
    {
        {PLAYER_NINJA_CRAWL_R4 + 0, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[4][1]},
        {PLAYER_NINJA_CRAWL_R4 + 1, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[4][2]},
        {PLAYER_NINJA_CRAWL_R4 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaCrawl[4][3]},
        {PLAYER_NINJA_CRAWL_R4 + 2, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[4][4]},
        {PLAYER_NINJA_CRAWL_R4 + 1, PLAYER_NINJA_CRAWL_RATE, NullAnimator, &s_PlayerNinjaCrawl[4][5]},
        {PLAYER_NINJA_CRAWL_R4 + 1, 0 | SF_QUICK_CALL, DoFootPrints, &s_PlayerNinjaCrawl[4][0]},
    },
};


STATE* sg_PlayerNinjaCrawl[] =
{
    s_PlayerNinjaCrawl[0],
    s_PlayerNinjaCrawl[1],
    s_PlayerNinjaCrawl[2],
    s_PlayerNinjaCrawl[3],
    s_PlayerNinjaCrawl[4]
};

//////////////////////
//
// PLAYER NINJA SWIM
//
//////////////////////


#define PLAYER_NINJA_SWIM_RATE 22 // Was 18
STATE s_PlayerNinjaSwim[5][4] =
{
    {
        {PLAYER_NINJA_SWIM_R0 + 0, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[0][1]},
        {PLAYER_NINJA_SWIM_R0 + 1, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[0][2]},
        {PLAYER_NINJA_SWIM_R0 + 2, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[0][3]},
        {PLAYER_NINJA_SWIM_R0 + 3, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[0][0]},
    },
    {
        {PLAYER_NINJA_SWIM_R1 + 0, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[1][1]},
        {PLAYER_NINJA_SWIM_R1 + 1, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[1][2]},
        {PLAYER_NINJA_SWIM_R1 + 2, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[1][3]},
        {PLAYER_NINJA_SWIM_R1 + 3, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[1][0]},
    },
    {
        {PLAYER_NINJA_SWIM_R2 + 0, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[2][1]},
        {PLAYER_NINJA_SWIM_R2 + 1, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[2][2]},
        {PLAYER_NINJA_SWIM_R2 + 2, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[2][3]},
        {PLAYER_NINJA_SWIM_R2 + 3, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[2][0]},
    },
    {
        {PLAYER_NINJA_SWIM_R3 + 0, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[3][1]},
        {PLAYER_NINJA_SWIM_R3 + 1, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[3][2]},
        {PLAYER_NINJA_SWIM_R3 + 2, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[3][3]},
        {PLAYER_NINJA_SWIM_R3 + 3, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[3][0]},
    },
    {
        {PLAYER_NINJA_SWIM_R4 + 0, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[4][1]},
        {PLAYER_NINJA_SWIM_R4 + 1, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[4][2]},
        {PLAYER_NINJA_SWIM_R4 + 2, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[4][3]},
        {PLAYER_NINJA_SWIM_R4 + 3, PLAYER_NINJA_SWIM_RATE, NullAnimator, &s_PlayerNinjaSwim[4][0]},
    },
};


STATE* sg_PlayerNinjaSwim[] =
{
    s_PlayerNinjaSwim[0],
    s_PlayerNinjaSwim[1],
    s_PlayerNinjaSwim[2],
    s_PlayerNinjaSwim[3],
    s_PlayerNinjaSwim[4]
};


#define NINJA_HeadHurl_RATE 16
#define NINJA_Head_RATE 16
#define NINJA_HeadFly 1134
#define NINJA_HeadFly_RATE 16

STATE s_PlayerHeadFly[5][8] =
{
    {
        {NINJA_HeadFly + 0, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[0][1]},
        {NINJA_HeadFly + 1, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[0][2]},
        {NINJA_HeadFly + 2, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[0][3]},
        {NINJA_HeadFly + 3, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[0][4]},
        {NINJA_HeadFly + 4, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[0][5]},
        {NINJA_HeadFly + 5, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[0][6]},
        {NINJA_HeadFly + 6, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[0][7]},
        {NINJA_HeadFly + 7, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[0][0]}
    },
    {
        {NINJA_HeadFly + 0, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[1][1]},
        {NINJA_HeadFly + 1, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[1][2]},
        {NINJA_HeadFly + 2, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[1][3]},
        {NINJA_HeadFly + 3, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[1][4]},
        {NINJA_HeadFly + 4, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[1][5]},
        {NINJA_HeadFly + 5, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[1][6]},
        {NINJA_HeadFly + 6, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[1][7]},
        {NINJA_HeadFly + 7, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[1][0]}
    },
    {
        {NINJA_HeadFly + 0, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[2][1]},
        {NINJA_HeadFly + 1, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[2][2]},
        {NINJA_HeadFly + 2, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[2][3]},
        {NINJA_HeadFly + 3, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[2][4]},
        {NINJA_HeadFly + 4, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[2][5]},
        {NINJA_HeadFly + 5, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[2][6]},
        {NINJA_HeadFly + 6, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[2][7]},
        {NINJA_HeadFly + 7, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[2][0]}
    },
    {
        {NINJA_HeadFly + 0, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[3][1]},
        {NINJA_HeadFly + 1, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[3][2]},
        {NINJA_HeadFly + 2, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[3][3]},
        {NINJA_HeadFly + 3, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[3][4]},
        {NINJA_HeadFly + 4, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[3][5]},
        {NINJA_HeadFly + 5, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[3][6]},
        {NINJA_HeadFly + 6, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[3][7]},
        {NINJA_HeadFly + 7, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[3][0]}
    },
    {
        {NINJA_HeadFly + 0, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[4][1]},
        {NINJA_HeadFly + 1, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[4][2]},
        {NINJA_HeadFly + 2, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[4][3]},
        {NINJA_HeadFly + 3, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[4][4]},
        {NINJA_HeadFly + 4, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[4][5]},
        {NINJA_HeadFly + 5, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[4][6]},
        {NINJA_HeadFly + 6, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[4][7]},
        {NINJA_HeadFly + 7, NINJA_HeadFly_RATE, NullAnimator, &s_PlayerHeadFly[4][0]}
    },
};

STATE* sg_PlayerHeadFly[] =
{
    s_PlayerHeadFly[0],
    s_PlayerHeadFly[1],
    s_PlayerHeadFly[2],
    s_PlayerHeadFly[3],
    s_PlayerHeadFly[4]
};

//#define NINJA_Head_FRAMES 1
//#define NINJA_Head_R0 1142
//#define NINJA_Head_R1 NINJA_Head_R0 + (NINJA_Head_FRAMES * 1)
//#define NINJA_Head_R2 NINJA_Head_R0 + (NINJA_Head_FRAMES * 2)
//#define NINJA_Head_R3 NINJA_Head_R0 + (NINJA_Head_FRAMES * 3)
//#define NINJA_Head_R4 NINJA_Head_R0 + (NINJA_Head_FRAMES * 4)

STATE s_PlayerHead[5][1] =
{
    {
        {NINJA_Head_R0 + 0, NINJA_Head_RATE, NullAnimator, &s_PlayerHead[0][0]},
    },
    {
        {NINJA_Head_R1 + 0, NINJA_Head_RATE, NullAnimator, &s_PlayerHead[1][0]},
    },
    {
        {NINJA_Head_R2 + 0, NINJA_Head_RATE, NullAnimator, &s_PlayerHead[2][0]},
    },
    {
        {NINJA_Head_R3 + 0, NINJA_Head_RATE, NullAnimator, &s_PlayerHead[3][0]},
    },
    {
        {NINJA_Head_R4 + 0, NINJA_Head_RATE, NullAnimator, &s_PlayerHead[4][0]},
    },
};

STATE* sg_PlayerHead[] =
{
    s_PlayerHead[0],
    s_PlayerHead[1],
    s_PlayerHead[2],
    s_PlayerHead[3],
    s_PlayerHead[4]
};

#define NINJA_HeadHurl_FRAMES 1
#define NINJA_HeadHurl_R0 1147
#define NINJA_HeadHurl_R1 NINJA_HeadHurl_R0 + (NINJA_HeadHurl_FRAMES * 1)
#define NINJA_HeadHurl_R2 NINJA_HeadHurl_R0 + (NINJA_HeadHurl_FRAMES * 2)
#define NINJA_HeadHurl_R3 NINJA_HeadHurl_R0 + (NINJA_HeadHurl_FRAMES * 3)
#define NINJA_HeadHurl_R4 NINJA_HeadHurl_R0 + (NINJA_HeadHurl_FRAMES * 4)

STATE s_PlayerHeadHurl[5][1] =
{
    {
        {NINJA_HeadHurl_R0 + 0, NINJA_HeadHurl_RATE, NullAnimator, &s_PlayerHeadHurl[0][0]},
    },
    {
        {NINJA_HeadHurl_R1 + 0, NINJA_HeadHurl_RATE, NullAnimator, &s_PlayerHeadHurl[1][0]},
    },
    {
        {NINJA_HeadHurl_R2 + 0, NINJA_HeadHurl_RATE, NullAnimator, &s_PlayerHeadHurl[2][0]},
    },
    {
        {NINJA_HeadHurl_R3 + 0, NINJA_HeadHurl_RATE, NullAnimator, &s_PlayerHeadHurl[3][0]},
    },
    {
        {NINJA_HeadHurl_R4 + 0, NINJA_HeadHurl_RATE, NullAnimator, &s_PlayerHeadHurl[4][0]},
    },
};

STATE* sg_PlayerHeadHurl[] =
{
    s_PlayerHeadHurl[0],
    s_PlayerHeadHurl[1],
    s_PlayerHeadHurl[2],
    s_PlayerHeadHurl[3],
    s_PlayerHeadHurl[4]
};

#define PLAYER_NINJA_DIE_RATE 22

STATE s_PlayerDeath[5][10] =
{
    {
        {PLAYER_NINJA_DIE + 0, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][1]},
        {PLAYER_NINJA_DIE + 1, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][2]},
        {PLAYER_NINJA_DIE + 2, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][3]},
        {PLAYER_NINJA_DIE + 3, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][4]},
        {PLAYER_NINJA_DIE + 4, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][5]},
        {PLAYER_NINJA_DIE + 5, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][6]},
        {PLAYER_NINJA_DIE + 6, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][7]},
        {PLAYER_NINJA_DIE + 7, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][8]},
        {PLAYER_NINJA_DIE + 8, 0 | SF_QUICK_CALL, QueueFloorBlood, &s_PlayerDeath[0][9]},
        {PLAYER_NINJA_DIE + 8, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][9]},
    },
    {
        {PLAYER_NINJA_DIE + 0, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][1]},
        {PLAYER_NINJA_DIE + 1, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][2]},
        {PLAYER_NINJA_DIE + 2, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][3]},
        {PLAYER_NINJA_DIE + 3, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][4]},
        {PLAYER_NINJA_DIE + 4, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][5]},
        {PLAYER_NINJA_DIE + 5, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][6]},
        {PLAYER_NINJA_DIE + 6, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][7]},
        {PLAYER_NINJA_DIE + 7, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][8]},
        {PLAYER_NINJA_DIE + 8, 0 | SF_QUICK_CALL, QueueFloorBlood, &s_PlayerDeath[1][9]},
        {PLAYER_NINJA_DIE + 8, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][9]},
    },
    {
        {PLAYER_NINJA_DIE + 0, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][1]},
        {PLAYER_NINJA_DIE + 1, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][2]},
        {PLAYER_NINJA_DIE + 2, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][3]},
        {PLAYER_NINJA_DIE + 3, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][4]},
        {PLAYER_NINJA_DIE + 4, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][5]},
        {PLAYER_NINJA_DIE + 5, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][6]},
        {PLAYER_NINJA_DIE + 6, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][7]},
        {PLAYER_NINJA_DIE + 7, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][8]},
        {PLAYER_NINJA_DIE + 8, 0 | SF_QUICK_CALL, QueueFloorBlood, &s_PlayerDeath[2][9]},
        {PLAYER_NINJA_DIE + 8, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][9]},
    },
    {
        {PLAYER_NINJA_DIE + 0, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][1]},
        {PLAYER_NINJA_DIE + 1, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][2]},
        {PLAYER_NINJA_DIE + 2, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][3]},
        {PLAYER_NINJA_DIE + 3, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][4]},
        {PLAYER_NINJA_DIE + 4, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][5]},
        {PLAYER_NINJA_DIE + 5, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][6]},
        {PLAYER_NINJA_DIE + 6, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][7]},
        {PLAYER_NINJA_DIE + 7, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][8]},
        {PLAYER_NINJA_DIE + 8, 0 | SF_QUICK_CALL, QueueFloorBlood, &s_PlayerDeath[3][9]},
        {PLAYER_NINJA_DIE + 8, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][9]},
    },
    {
        {PLAYER_NINJA_DIE + 0, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][1]},
        {PLAYER_NINJA_DIE + 1, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][2]},
        {PLAYER_NINJA_DIE + 2, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][3]},
        {PLAYER_NINJA_DIE + 3, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][4]},
        {PLAYER_NINJA_DIE + 4, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][5]},
        {PLAYER_NINJA_DIE + 5, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][6]},
        {PLAYER_NINJA_DIE + 6, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][7]},
        {PLAYER_NINJA_DIE + 7, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][8]},
        {PLAYER_NINJA_DIE + 8, 0 | SF_QUICK_CALL, QueueFloorBlood, &s_PlayerDeath[4][9]},
        {PLAYER_NINJA_DIE + 8, PLAYER_NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][9]},
    },
};

STATE* sg_PlayerDeath[] =
{
    s_PlayerDeath[0],
    s_PlayerDeath[1],
    s_PlayerDeath[2],
    s_PlayerDeath[3],
    s_PlayerDeath[4]
};

//////////////////////
//
// PLAYER NINJA SWORD
//
//////////////////////


#define PLAYER_NINJA_SWORD_RATE 12
STATE s_PlayerNinjaSword[5][4] =
{
    {
        {PLAYER_NINJA_SWORD_R0 + 0, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[0][1]},
        {PLAYER_NINJA_SWORD_R0 + 1, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[0][2]},
        {PLAYER_NINJA_SWORD_R0 + 2, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[0][3]},
        {PLAYER_NINJA_SWORD_R0 + 2, PLAYER_NINJA_SWORD_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaSword[0][0]},
    },
    {
        {PLAYER_NINJA_SWORD_R1 + 0, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[1][1]},
        {PLAYER_NINJA_SWORD_R1 + 1, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[1][2]},
        {PLAYER_NINJA_SWORD_R1 + 2, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[1][3]},
        {PLAYER_NINJA_SWORD_R1 + 2, PLAYER_NINJA_SWORD_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaSword[1][0]},
    },
    {
        {PLAYER_NINJA_SWORD_R2 + 0, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[2][1]},
        {PLAYER_NINJA_SWORD_R2 + 1, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[2][2]},
        {PLAYER_NINJA_SWORD_R2 + 2, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[2][3]},
        {PLAYER_NINJA_SWORD_R2 + 2, PLAYER_NINJA_SWORD_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaSword[2][0]},
    },
    {
        {PLAYER_NINJA_SWORD_R3 + 0, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[3][1]},
        {PLAYER_NINJA_SWORD_R3 + 1, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[3][2]},
        {PLAYER_NINJA_SWORD_R3 + 2, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[3][3]},
        {PLAYER_NINJA_SWORD_R3 + 2, PLAYER_NINJA_SWORD_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaSword[3][0]},
    },
    {
        {PLAYER_NINJA_SWORD_R4 + 0, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[4][1]},
        {PLAYER_NINJA_SWORD_R4 + 1, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[4][2]},
        {PLAYER_NINJA_SWORD_R4 + 2, PLAYER_NINJA_SWORD_RATE, NullAnimator, &s_PlayerNinjaSword[4][3]},
        {PLAYER_NINJA_SWORD_R4 + 2, PLAYER_NINJA_SWORD_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaSword[4][0]},
    },
};


STATE* sg_PlayerNinjaSword[] =
{
    s_PlayerNinjaSword[0],
    s_PlayerNinjaSword[1],
    s_PlayerNinjaSword[2],
    s_PlayerNinjaSword[3],
    s_PlayerNinjaSword[4]
};

//////////////////////
//
// PLAYER NINJA PUNCH
//
//////////////////////


#define PLAYER_NINJA_PUNCH_RATE 15
STATE s_PlayerNinjaPunch[5][4] =
{
    {
        {PLAYER_NINJA_PUNCH_R0 + 0, PLAYER_NINJA_PUNCH_RATE, NullAnimator, &s_PlayerNinjaPunch[0][1]},
        {PLAYER_NINJA_PUNCH_R0 + 1, PLAYER_NINJA_PUNCH_RATE, NullAnimator, &s_PlayerNinjaPunch[0][2]},
        {PLAYER_NINJA_PUNCH_R0 + 1, PLAYER_NINJA_PUNCH_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaPunch[0][2]},
    },
    {
        {PLAYER_NINJA_PUNCH_R1 + 0, PLAYER_NINJA_PUNCH_RATE, NullAnimator, &s_PlayerNinjaPunch[1][1]},
        {PLAYER_NINJA_PUNCH_R1 + 1, PLAYER_NINJA_PUNCH_RATE, NullAnimator, &s_PlayerNinjaPunch[1][2]},
        {PLAYER_NINJA_PUNCH_R1 + 1, PLAYER_NINJA_PUNCH_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaPunch[1][2]},
    },
    {
        {PLAYER_NINJA_PUNCH_R2 + 0, PLAYER_NINJA_PUNCH_RATE, NullAnimator, &s_PlayerNinjaPunch[2][1]},
        {PLAYER_NINJA_PUNCH_R2 + 1, PLAYER_NINJA_PUNCH_RATE, NullAnimator, &s_PlayerNinjaPunch[2][2]},
        {PLAYER_NINJA_PUNCH_R2 + 1, PLAYER_NINJA_PUNCH_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaPunch[2][2]},
    },
    {
        {PLAYER_NINJA_PUNCH_R3 + 0, PLAYER_NINJA_PUNCH_RATE, NullAnimator, &s_PlayerNinjaPunch[3][1]},
        {PLAYER_NINJA_PUNCH_R3 + 1, PLAYER_NINJA_PUNCH_RATE, NullAnimator, &s_PlayerNinjaPunch[3][2]},
        {PLAYER_NINJA_PUNCH_R3 + 1, PLAYER_NINJA_PUNCH_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaPunch[3][2]},
    },
    {
        {PLAYER_NINJA_PUNCH_R4 + 0, PLAYER_NINJA_PUNCH_RATE, NullAnimator, &s_PlayerNinjaPunch[4][1]},
        {PLAYER_NINJA_PUNCH_R4 + 1, PLAYER_NINJA_PUNCH_RATE, NullAnimator, &s_PlayerNinjaPunch[4][2]},
        {PLAYER_NINJA_PUNCH_R4 + 1, PLAYER_NINJA_PUNCH_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaPunch[4][2]},
    },
};


STATE* sg_PlayerNinjaPunch[] =
{
    s_PlayerNinjaPunch[0],
    s_PlayerNinjaPunch[1],
    s_PlayerNinjaPunch[2],
    s_PlayerNinjaPunch[3],
    s_PlayerNinjaPunch[4]
};

//////////////////////
//
// PLAYER NINJA FLY
//
//////////////////////


#define PLAYER_NINJA_FLY_RATE 15
#define PLAYER_NINJA_FLY_R0 1200
#define PLAYER_NINJA_FLY_R1 1200
#define PLAYER_NINJA_FLY_R2 1200
#define PLAYER_NINJA_FLY_R3 1200
#define PLAYER_NINJA_FLY_R4 1200

STATE s_PlayerNinjaFly[5][4] =
{
    {
        {PLAYER_NINJA_FLY_R0 + 0, PLAYER_NINJA_FLY_RATE, NullAnimator, &s_PlayerNinjaFly[0][0]},
    },
    {
        {PLAYER_NINJA_FLY_R1 + 0, PLAYER_NINJA_FLY_RATE, NullAnimator, &s_PlayerNinjaFly[1][0]},
    },
    {
        {PLAYER_NINJA_FLY_R2 + 0, PLAYER_NINJA_FLY_RATE, NullAnimator, &s_PlayerNinjaFly[2][0]},
    },
    {
        {PLAYER_NINJA_FLY_R3 + 0, PLAYER_NINJA_FLY_RATE, NullAnimator, &s_PlayerNinjaFly[3][0]},
    },
    {
        {PLAYER_NINJA_FLY_R4 + 0, PLAYER_NINJA_FLY_RATE, NullAnimator, &s_PlayerNinjaFly[4][0]},
    },
};


STATE* sg_PlayerNinjaFly[] =
{
    s_PlayerNinjaFly[0],
    s_PlayerNinjaFly[1],
    s_PlayerNinjaFly[2],
    s_PlayerNinjaFly[3],
    s_PlayerNinjaFly[4]
};

/////////////////////////////////////////////////////////////////////////////

void DoPlayerSpriteThrow(PLAYER* pp)
{
    if (!(pp->Flags & (PF_DIVING|PF_FLYING|PF_CRAWLING)))
    {
        if (pp->CurWpn == pp->Wpn[WPN_SWORD] && pp->actor->user.Rot != sg_PlayerNinjaSword)
            NewStateGroup(pp->actor, sg_PlayerNinjaSword);
        else
            //if (pp->CurWpn == pp->Wpn[WPN_FIST] && pp->actor->user.Rot != sg_PlayerNinjaPunch)
            NewStateGroup(pp->actor, sg_PlayerNinjaPunch);
        //else
        //    NewStateGroup(pp->actor, sg_PlayerNinjaThrow);
    }
}

int DoPlayerSpriteReset(DSWActor* actor)
{
    PLAYER* pp;

    if (!actor->user.PlayerP)
        return 0;

    pp = actor->user.PlayerP;

    // need to figure out what frames to put sprite into
    if (pp->DoPlayerAction == DoPlayerCrawl)
        NewStateGroup(pp->actor, actor->user.ActorActionSet->Crawl);
    else
    {
        if (pp->Flags & (PF_PLAYER_MOVED))
            NewStateGroup(pp->actor, actor->user.ActorActionSet->Run);
        else
            NewStateGroup(pp->actor, actor->user.ActorActionSet->Stand);
    }

    return 0;
}

int SetVisHigh(void)
{
//    g_visibility = NormalVisibility>>1;
    return 0;
}

int SetVisNorm(void)
{
//    g_visibility = NormalVisibility;
    return 0;
}

void pSetVisNorm(PANEL_SPRITE* psp)
{
//    SetVisNorm();
}

TARGET_SORT TargetSort[MAX_TARGET_SORT];
unsigned TargetSortCount;

static int CompareTarget(void const * a, void const * b)
{
    auto tgt1 = (TARGET_SORT const *)a;
    auto tgt2 = (TARGET_SORT const *)b;

    // will return a number less than 0 if tgt1 < tgt2
    return tgt2->weight - tgt1->weight;
}

bool
FAFcansee(int32_t xs, int32_t ys, int32_t zs, int16_t sects,
          int32_t xe, int32_t ye, int32_t ze, int16_t secte);

DSWActor* DoPickTarget(DSWActor* actor, uint32_t max_delta_ang, int skip_targets)
{
    const int PICK_DIST = 40000;

    short angle2, delta_ang;
    int dist;
    int16_t* shp;
    unsigned ndx;
    TARGET_SORT* ts;
    int ang_weight, dist_weight;

    // !JIM! Watch out for max_delta_ang of zero!
    if (max_delta_ang == 0) max_delta_ang = 1;

    TargetSortCount = 0;
    TargetSort[0].actor = nullptr;

    for (shp = StatDamageList; shp < &StatDamageList[SIZ(StatDamageList)]; shp++)
    {
        SWStatIterator it(*shp);
        while (auto itActor = it.Next())
        {
            // don't pick yourself
            if (actor == itActor)
                continue;

            if (skip_targets != 2) // Used for spriteinfo mode
            {
                if (skip_targets && (itActor->user.Flags & SPR_TARGETED))
                    continue;

                // don't pick a dead player
                if (itActor->user.PlayerP && (itActor->user.PlayerP->Flags & PF_DEAD))
                    continue;
            }

            // Only look at closest ones
            //if ((dist = Distance(actor->spr.x, actor->spr.y, itActor->spr.x, itActor->spr.y)) > PICK_DIST)
            if ((dist = FindDistance3D(actor->int_pos() - itActor->int_pos())) > PICK_DIST)
                continue;

            if (skip_targets != 2) // Used for spriteinfo mode
            {
                // don't set off mine
                if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                    continue;
            }

            // Get the angle to the player
            angle2 = NORM_ANGLE(getangle(itActor->int_pos().X - actor->int_pos().X, itActor->int_pos().Y - actor->int_pos().Y));

            // Get the angle difference
            // delta_ang = labs(pp->angle.ang.Buildang() - angle2);

            delta_ang = short(abs(getincangle(angle2, actor->int_ang())));

            // If delta_ang not in the range skip this one
            if (delta_ang > (int)max_delta_ang)
                continue;

            DVector3 apos = actor->spr.pos;
            DVector2 ipos = itActor->spr.pos.XY();

            double ezh = ActorZOfTop(itActor) + (ActorSizeZ(itActor) * 0.25);
            double ezhm = ActorZOfTop(itActor) + (ActorSizeZ(itActor) * 0.5);
            double ezhl = ActorZOfBottom(itActor) - (ActorSizeZ(itActor) * 0.25);

            if (actor->hasU() && actor->user.PlayerP)
                apos.Z = actor->user.PlayerP->pos.Z;
            else
                apos.Z = ActorZOfTop(actor) + (ActorSizeZ(actor) * 0.25);

            // If you can't see 'em you can't shoot 'em
            if (!FAFcansee(apos, actor->sector(), DVector3(ipos, ezh), itActor->sector()) &&
                !FAFcansee(apos, actor->sector(), DVector3(ipos, ezhm), itActor->sector()) &&
                !FAFcansee(apos, actor->sector(), DVector3(ipos, ezhl), itActor->sector())
                )
                continue;

            // get ndx - there is only room for 15
            if (TargetSortCount > SIZ(TargetSort)-1)
            {
                for (ndx = 0; ndx < SIZ(TargetSort); ndx++)
                {
                    if (dist < TargetSort[ndx].dist)
                        break;
                }

                if (ndx == SIZ(TargetSort))
                    continue;
            }
            else
            {
                ndx = TargetSortCount;
            }

            ts = &TargetSort[ndx];
            ts->actor = itActor;
            ts->dang = delta_ang;
            ts->dist = dist;
            // gives a value between 0 and 65535
            ang_weight = IntToFixed(max_delta_ang - ts->dang)/max_delta_ang;
            // gives a value between 0 and 65535
            dist_weight = IntToFixed((PICK_DIST / 2) - ((ts->dist) >> 1)) / (PICK_DIST / 2);
            //weighted average
            ts->weight = (ang_weight + dist_weight*4)/5;

            TargetSortCount++;
            if (TargetSortCount >= SIZ(TargetSort))
                TargetSortCount = SIZ(TargetSort);
        }
    }

    if (TargetSortCount > 1)
        qsort(&TargetSort, TargetSortCount, sizeof(TARGET_SORT), CompareTarget);

    return TargetSort[0].actor;
}

void DoPlayerResetMovement(PLAYER* pp)
{
    pp->vect.X = pp->ovect.X = 0;
    pp->vect.Y = pp->ovect.Y = 0;
    pp->slide_vect.X = 0;
    pp->slide_vect.Y = 0;
    pp->drive_avel = 0;
    pp->Flags &= ~(PF_PLAYER_MOVED);
}

void DoPlayerTeleportPause(PLAYER* pp)
{
    DSWActor* actor = pp->actor;

    // set this so we don't get stuck in teleporting loop
    pp->lastcursector = pp->cursector;

    if ((actor->user.WaitTics-=synctics) <= 0)
    {
        //actor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);
        pp->Flags2 &= ~(PF2_TELEPORTED);
        DoPlayerResetMovement(pp);
        DoPlayerBeginRun(pp);
        return;
    }
}

void DoPlayerTeleportToSprite(PLAYER* pp, vec3_t* pos, int ang)
{
    pp->angle.ang = pp->angle.oang = DAngle::fromBuild(ang);
    pp->set_int_ppos_XY(pos->XY());

    pp->oldpos.XY() = pp->opos.XY() = pp->pos.XY();

    
    //getzsofslopeptr(actor->s
    // ector(), pp->posx, pp->posy, &cz, &fz);
    //pp->posz = pp->oposz = fz - PLAYER_HEIGHT;

    pp->set_int_ppos_Z(pos->Z - PLAYER_HEIGHT);
    pp->opos.Z = pp->pos.Z;

    updatesector(pp->int_ppos().X, pp->int_ppos().Y, &pp->cursector);
    pp->Flags2 |= (PF2_TELEPORTED);
}

void DoPlayerTeleportToOffset(PLAYER* pp)
{
    pp->oldpos.XY() = pp->opos.XY() = pp->pos.XY();

    updatesector(pp->int_ppos().X, pp->int_ppos().Y, &pp->cursector);
    pp->Flags2 |= (PF2_TELEPORTED);
}

void DoSpawnTeleporterEffect(DSWActor* actor)
{
    extern STATE s_TeleportEffect[];
    int nx, ny;

    nx = MOVEx(512, actor->int_ang());
    ny = MOVEy(512, actor->int_ang());

    nx += actor->int_pos().X;
    ny += actor->int_pos().Y;

    auto effectActor = SpawnActor(STAT_MISSILE, 0, s_TeleportEffect, actor->sector(),
                         nx, ny, int_ActorZOfTop(actor) + Z(16),
                         actor->int_ang(), 0);

    SetActorZ(effectActor, effectActor->spr.pos);

    effectActor->spr.shade = -40;
    effectActor->spr.xrepeat = effectActor->spr.yrepeat = 42;
    effectActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    effectActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    effectActor->spr.cstat |= (CSTAT_SPRITE_ALIGNMENT_WALL);
}

void DoSpawnTeleporterEffectPlace(DSWActor* actor)
{
    extern STATE s_TeleportEffect[];

    auto effectActor = SpawnActor(STAT_MISSILE, 0, s_TeleportEffect, actor->sector(), ActorVectOfTop(actor).plusZ(16), actor->spr.angle, 0);

    SetActorZ(effectActor, effectActor->spr.pos);

    effectActor->spr.shade = -40;
    effectActor->spr.xrepeat = effectActor->spr.yrepeat = 42;
    effectActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    effectActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    effectActor->spr.cstat |= (CSTAT_SPRITE_ALIGNMENT_WALL);
}

void DoPlayerWarpTeleporter(PLAYER* pp)
{
    auto ppActor = pp->actor;
    short pnum;
    DSWActor* act_warp;

#if 0
    TAG 2 = match
            TAG 3 = Type
                    Sprite - 0,32 always teleports you to the center at the angle the sprite is facing
    Offset - 1 always teleports you by the offset.Does not touch the angle
    TAG 4 = angle
            TAG 5 to 8 = random match locations
#endif

    if ((act_warp = Warp(pp->pos, &pp->cursector)) == nullptr)
    {
        return;
    }

    switch (SP_TAG3(act_warp))
    {
    case 1:
        DoPlayerTeleportToOffset(pp);
        UpdatePlayerSprite(pp);
        break;
    default:
    {
        auto pos = act_warp->int_pos();
        DoPlayerTeleportToSprite(pp, &pos, act_warp->int_ang());
        act_warp->set_int_pos(pos);

        PlaySound(DIGI_TELEPORT, pp, v3df_none);

        DoPlayerResetMovement(pp);

        ppActor->user.WaitTics = 30;
        DoPlayerBeginRun(pp);
        pp->DoPlayerAction = DoPlayerTeleportPause;

        NewStateGroup(ppActor, ppActor->user.ActorActionSet->Stand);

        UpdatePlayerSprite(pp);
        DoSpawnTeleporterEffect(ppActor);

        TRAVERSE_CONNECT(pnum)
        {
            if (pnum != pp - Player)
            {
                PLAYER* npp = &Player[pnum];

                // if someone already standing there
                if (npp->cursector == pp->cursector)
                {
                    PlayerUpdateHealth(npp, -npp->actor->user.Health);  // Make sure he dies!
                    // telefraged by teleporting player
                    PlayerCheckDeath(npp, pp->actor);
                }
            }
        }

        break;
    }
    }

    ppActor->backuppos();
}

void DoPlayerSetWadeDepth(PLAYER* pp)
{
    sectortype* sectp;

    pp->WadeDepth = 0;

    if (pp->lo_sectp)
        sectp = pp->lo_sectp;
    else
        return;

    if ((sectp->extra & SECTFX_SINK))
    {
        // make sure your even in the water
        if (pp->pos.Z + PLAYER_HEIGHTF > pp->lo_sectp->floorz - FixedToInt(pp->lo_sectp->depth_fixed))
            pp->WadeDepth = FixedToInt(pp->lo_sectp->depth_fixed);
    }
}


void DoPlayerHeight(PLAYER* pp)
{
    double diff = pp->pos.Z - (pp->loz - PLAYER_HEIGHTF);
    pp->pos.Z -= diff * 0.375;
}

void DoPlayerJumpHeight(PLAYER* pp)
{
    if (pp->lo_sectp && (pp->lo_sectp->extra & SECTFX_DYNAMIC_AREA))
    {
        if (pp->pos.Z + PLAYER_HEIGHTF > pp->loz)
        {
            pp->pos.Z = pp->loz - PLAYER_HEIGHTF;
            DoPlayerBeginRun(pp);
        }
    }
}

void DoPlayerCrawlHeight(PLAYER* pp)
{
    double diff = pp->pos.Z - (pp->loz - PLAYER_CRAWL_HEIGHTF);
    pp->pos.Z -= diff * 0.375;
}

void UpdatePlayerSpriteAngle(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    plActor->backupang();
    plActor->spr.angle = pp->angle.ang;

    plActor = pp->PlayerUnderActor;

    if (!Prediction && plActor)
    {
        plActor->backupang();
        plActor->set_int_ang(pp->angle.ang.Buildang());
    }
}

void DoPlayerTurn(PLAYER* pp, float const avel, double const scaleAdjust)
{
    pp->angle.applyinput(avel, &pp->input.actions, scaleAdjust);
    UpdatePlayerSpriteAngle(pp);
}

void DoPlayerTurnVehicle(PLAYER* pp, float avel, int z, int floor_dist)
{
    SECTOR_OBJECT* sop = pp->sop;

    if (sop->drive_angspeed)
    {
        float drive_oavel = pp->drive_avel;
        pp->drive_avel = float((MulScaleF(avel, sop->drive_angspeed, 16) + (drive_oavel * (sop->drive_angslide - 1))) / sop->drive_angslide);

        avel = pp->drive_avel;
    }
    else
    {
        avel *= synctics * 0.125f;
    }

    if (avel != 0)
    {
        auto sum = pp->angle.ang + DAngle::fromDeg(avel);
        if (MultiClipTurn(pp, NORM_ANGLE(sum.Buildang()), z, floor_dist))
        {
            pp->angle.ang = sum;
            pp->actor->set_int_ang(pp->angle.ang.Buildang());
        }
    }
}

void DoPlayerTurnVehicleRect(PLAYER* pp, int *x, int *y, int *ox, int *oy)
{
    float avel;
    SECTOR_OBJECT* sop = pp->sop;

    if (sop->drive_angspeed)
    {
        float drive_oavel = pp->drive_avel;
        pp->drive_avel = float((MulScaleF(pp->input.avel, sop->drive_angspeed, 16) + (drive_oavel * (sop->drive_angslide - 1))) / sop->drive_angslide);

        avel = pp->drive_avel;
    }
    else
    {
        avel = pp->input.avel * synctics * 0.125f;
    }

    if (avel != 0)
    {
        auto sum = pp->angle.ang + DAngle::fromDeg(avel);
        if (RectClipTurn(pp, NORM_ANGLE(sum.Buildang()), x, y, ox, oy))
        {
            pp->angle.ang = sum;
            pp->actor->set_int_ang(pp->angle.ang.Buildang());
        }
    }
}

void DoPlayerTurnTurret(PLAYER* pp, float avel)
{
    DAngle new_ang, diff;
    SECTOR_OBJECT* sop = pp->sop;

    if (sop->drive_angspeed)
    {
        float drive_oavel = pp->drive_avel;
        pp->drive_avel = float((MulScaleF(avel, sop->drive_angspeed, 16) + (drive_oavel * (sop->drive_angslide - 1))) / sop->drive_angslide);

        avel = pp->drive_avel;
    }
    else
    {
        avel = avel * synctics * 0.25f;
    }

    if (fabs(avel) >= FLT_EPSILON)
    {
        new_ang = pp->angle.ang + DAngle::fromDeg(avel);

        if (sop->limit_ang_center >= nullAngle)
        {
            diff = deltaangle(sop->limit_ang_center, new_ang);

            if (abs(diff) >= sop->limit_ang_delta)
            {
                if (diff < nullAngle)
                    new_ang = sop->limit_ang_center - sop->limit_ang_delta;
                else
                    new_ang = sop->limit_ang_center + sop->limit_ang_delta;
            }
        }

        pp->angle.ang = new_ang;
        pp->actor->set_int_ang(pp->angle.ang.Buildang());
    }

    OperateSectorObject(pp->sop, pp->angle.ang.Buildang(), pp->sop->pmid);
}

void SlipSlope(PLAYER* pp)
{
    short ang;

    if (!pp->insector() || !pp->cursector->hasU())
        return;

    auto sectu = pp->cursector;

    if (!(sectu->flags & SECTFU_SLIDE_SECTOR) || !(pp->cursector->floorstat & CSTAT_SECTOR_SLOPE))
        return;

    ang = getangle(pp->cursector->firstWall()->delta());

    ang = NORM_ANGLE(ang + 512);

    pp->vect.X += MulScale(bcos(ang), pp->cursector->floorheinum, sectu->speed);
    pp->vect.Y += MulScale(bsin(ang), pp->cursector->floorheinum, sectu->speed);
}

void DoPlayerHorizon(PLAYER* pp, float const horz, double const scaleAdjust)
{
    bool const canslopetilt = !(pp->Flags & (PF_FLYING|PF_SWIMMING|PF_DIVING|PF_CLIMBING|PF_JUMPING|PF_FALLING)) && pp->cursector && (pp->cursector->floorstat & CSTAT_SECTOR_SLOPE);
    pp->horizon.calcviewpitch(pp->int_ppos().vec2, pp->angle.ang, pp->input.actions & SB_AIMMODE, canslopetilt, pp->cursector, scaleAdjust, (pp->Flags & PF_CLIMBING));
    pp->horizon.applyinput(horz, &pp->input.actions, scaleAdjust);
}

void DoPlayerBob(PLAYER* pp)
{
    int dist;
    int amt;

    dist = 0;

    dist = DistanceI(pp->pos, pp->oldpos);

    if (dist > 512)
        dist = 0;

    // if running make a longer stride
    if (pp->input.actions & SB_RUN)
    {
        //amt = 10;
        amt = 12;
        amt = MulScale(amt, dist<<8, 16);
        dist = MulScale(dist, 26000, 16);
    }
    else
    {
        amt = 5;
        amt = MulScale(amt, dist<<9, 16);
        dist = MulScale(dist, 32000, 16);
    }

    // controls how fast you move through the sin table
    pp->bcnt += dist;

    // wrap bcnt
    pp->bcnt &= 2047;

    // move pp->q16horiz up and down from 100 using sintable
    //pp->bob_z = Z((8 * bsin(pp->bcnt)) >> 14);
    pp->bob_z = MulScale(Z(amt), bsin(pp->bcnt), 14);
}

void DoPlayerBeginRecoil(PLAYER* pp, short pix_amt)
{
    pp->Flags |= (PF_RECOIL);

    pp->recoil_amt = pix_amt;
    pp->recoil_speed = 80;
    pp->recoil_ndx = 0;
    pp->recoil_ohorizoff = pp->recoil_horizoff = 0;
}

void DoPlayerRecoil(PLAYER* pp)
{
    // controls how fast you move through the sin table
    pp->recoil_ndx += pp->recoil_speed;

    if (bsin(pp->recoil_ndx) < 0)
    {
        pp->Flags &= ~(PF_RECOIL);
        pp->recoil_ohorizoff = pp->recoil_horizoff = 0;
        return;
    }

    // move pp->q16horiz up and down
    pp->recoil_ohorizoff = pp->recoil_horizoff;
    pp->recoil_horizoff = pp->recoil_amt * bsin(pp->recoil_ndx, 2);
}



// for wading
void DoPlayerSpriteBob(PLAYER* pp, double player_height, double bob_amt, short bob_speed)
{
    pp->bob_ndx = (pp->bob_ndx + (synctics << bob_speed)) & 2047;
    pp->bob_amt = bob_amt * DAngle::fromBuild(pp->bob_ndx).Sin();
    pp->actor->spr.pos.Z = pp->pos.Z + player_height + pp->bob_amt;
}

void UpdatePlayerUnderSprite(PLAYER* pp)
{
    DSWActor* act_over = pp->actor;

    int water_level_z, zdiff;
    bool above_water, in_dive_area;

    if (Prediction)
        return;

    ASSERT(act_over->hasU());

    // dont bother spawning if you ain't really in the water
    water_level_z = act_over->sector()->int_floorz(); // - Z(pp->WadeDepth);

    // if not below water
    above_water = (int_ActorZOfBottom(act_over) <= water_level_z);
    in_dive_area = SpriteInDiveArea(act_over);

    // if not in dive area OR (in dive area AND above the water) - Kill it
    if (!in_dive_area || (in_dive_area && above_water))
    {

        // if under sprite exists and not in a dive area - Kill it
        if (pp->PlayerUnderActor != nullptr)
        {
            KillActor(pp->PlayerUnderActor);
            pp->PlayerUnderActor = nullptr;
        }
        return;
    }
    else
    {
        // if in a dive area and a under sprite does not exist - create it
        if (pp->PlayerUnderActor == nullptr)
        {
            SpawnPlayerUnderSprite(pp);
        }
    }

    DSWActor* act_under = pp->PlayerUnderActor;

    act_under->spr.pos = act_over->spr.pos;
    ChangeActorSect(act_under, act_over->sector());

    SpriteWarpToUnderwater(act_under);

    // find z water level of the top sector
    // diff between the bottom of the upper sprite and the water level
    zdiff = int_ActorZOfBottom(act_over) - water_level_z;

    // add diff to ceiling
    act_under->set_int_z(act_under->sector()->int_ceilingz() + zdiff);

    act_under->user.State = act_over->user.State;
    act_under->user.Rot = act_over->user.Rot;
    act_under->user.StateStart = act_over->user.StateStart;
    act_under->spr.picnum = act_over->spr.picnum;
}


void UpdatePlayerSprite(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    if (!actor) return;

    // Update sprite representation of player

    actor->set_int_xy(pp->int_ppos().X, pp->int_ppos().Y);

    // there are multiple death functions
    if (pp->Flags & (PF_DEAD))
    {
        ChangeActorSect(pp->actor, pp->cursector);
        actor->set_int_ang(pp->angle.ang.Buildang());
        UpdatePlayerUnderSprite(pp);
        return;
    }

    if (pp->sop_control)
    {
        actor->spr.pos.Z = pp->cursector->floorz;
        ChangeActorSect(pp->actor, pp->cursector);
    }
    else if (pp->DoPlayerAction == DoPlayerCrawl)
    {
        actor->spr.pos.Z = pp->pos.Z + PLAYER_CRAWL_HEIGHTF;
        ChangeActorSect(pp->actor, pp->cursector);
    }
#if 0
    else if (pp->DoPlayerAction == DoPlayerSwim)
    {
        actor->spr.z = pp->loz - Z(pp->WadeDepth) + Z(1);
        ChangeActorSect(pp->actor, pp->cursector);
    }
#endif
    else if (pp->DoPlayerAction == DoPlayerWade)
    {
        actor->spr.pos.Z = pp->pos.Z + PLAYER_HEIGHTF;
        ChangeActorSect(pp->actor, pp->cursector);

        if (pp->WadeDepth > Z(29))
        {
            DoPlayerSpriteBob(pp, PLAYER_HEIGHTF, 3, 3);
        }
    }
    else if (pp->DoPlayerAction == DoPlayerDive)
    {
        // bobbing and sprite position taken care of in DoPlayerDive
        actor->spr.pos.Z = pp->pos.Z + 10;
        ChangeActorSect(pp->actor, pp->cursector);
    }
    else if (pp->DoPlayerAction == DoPlayerClimb)
    {
        actor->spr.pos.Z = pp->pos.Z + 17;

        ChangeActorSect(pp->actor, pp->cursector);
    }
    else if (pp->DoPlayerAction == DoPlayerFly)
    {
        // bobbing and sprite position taken care of in DoPlayerFly
        DoPlayerSpriteBob(pp, PLAYER_HEIGHTF, 6, 3);
        ChangeActorSect(pp->actor, pp->cursector);
    }
    else if (pp->DoPlayerAction == DoPlayerJump || pp->DoPlayerAction == DoPlayerFall || pp->DoPlayerAction == DoPlayerForceJump)
    {
        actor->spr.pos.Z = pp->pos.Z + PLAYER_HEIGHTF;
        ChangeActorSect(pp->actor, pp->cursector);
    }
    else if (pp->DoPlayerAction == DoPlayerTeleportPause)
    {
        actor->spr.pos.Z = pp->pos.Z + PLAYER_HEIGHTF;
        ChangeActorSect(pp->actor, pp->cursector);
    }
    else
    {
        actor->spr.pos.Z = pp->loz;
        ChangeActorSect(pp->actor, pp->cursector);
    }

    UpdatePlayerUnderSprite(pp);

    actor->set_int_ang(pp->angle.ang.Buildang());
}

void DoPlayerZrange(PLAYER* pp)
{
    Collision ceilhit, florhit;

    DSWActor* actor = pp->actor;
    if (!actor) return;

    // Don't let you fall if you're just slightly over a cliff
    // This function returns the highest and lowest z's
    // for an entire box, NOT just a point.  -Useful for clipping
    auto bakcstat = actor->spr.cstat;
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
    vec3_t pos = pp->int_ppos();
    pos.Z += Z(8);
    FAFgetzrange(pos, pp->cursector, &pp->hiz, &ceilhit, &pp->loz, &florhit, ((int)actor->spr.clipdist<<2) - GETZRANGE_CLIP_ADJ, CLIPMASK_PLAYER);
    actor->spr.cstat = bakcstat;

    Collision ceilColl(ceilhit);
    Collision floorColl(florhit);

//  16384+sector (sector first touched) or
//  49152+spritenum (sprite first touched)

    pp->lo_sectp = pp->hi_sectp = nullptr;
    pp->lowActor = nullptr;
    pp->highActor = nullptr;

    if (ceilColl.type == kHitSprite)
    {
        pp->highActor = ceilColl.actor();
    }
    else
    {
        pp->hi_sectp = ceilColl.hitSector;
    }

    if (floorColl.type == kHitSprite)
    {
        pp->lowActor = floorColl.actor();

        // prevent player from standing on Zombies
        auto fsp = floorColl.actor();
        if (fsp->spr.statnum == STAT_ENEMY && floorColl.actor()->user.ID == ZOMBIE_RUN_R0)
        {
            pp->lo_sectp = fsp->sector();
            pp->loz = fsp->spr.pos.Z;
            pp->lowActor = nullptr;
        }
    }
    else
    {
        pp->lo_sectp = floorColl.hitSector;
    }
}

void DoPlayerSlide(PLAYER* pp)
{
    DSWActor* actor = pp->actor;

    int push_ret;

    if ((pp->slide_vect.X|pp->slide_vect.Y) == 0)
        return;

    if (pp->sop)
        return;

    pp->slide_vect.X  = MulScale(pp->slide_vect.X, PLAYER_SLIDE_FRICTION, 16);
    pp->slide_vect.Y  = MulScale(pp->slide_vect.Y, PLAYER_SLIDE_FRICTION, 16);

    if (labs(pp->slide_vect.X) < 12800 && labs(pp->slide_vect.Y) < 12800)
        pp->slide_vect.X = pp->slide_vect.Y = 0;

    push_ret = pushmove(pp->pos, &pp->cursector, ((int)actor->spr.clipdist<<2), pp->player_int_ceiling_dist(), pp->player_int_floor_dist(), CLIPMASK_PLAYER);
    if (push_ret < 0)
    {
        if (!(pp->Flags & PF_DEAD))
        {
            PlayerUpdateHealth(pp, -actor->user.Health);  // Make sure he dies!
            PlayerCheckDeath(pp, nullptr);

            if (pp->Flags & (PF_DEAD))
                return;
        }
        return;
    }
    Collision coll;
    clipmove(pp->pos, &pp->cursector, pp->slide_vect.X, pp->slide_vect.Y, ((int)actor->spr.clipdist<<2), pp->player_int_ceiling_dist(), pp->player_int_floor_dist(), CLIPMASK_PLAYER, coll);

    PlayerCheckValidMove(pp);
    push_ret = pushmove(pp->pos, &pp->cursector, ((int)actor->spr.clipdist<<2), pp->player_int_ceiling_dist(), pp->player_int_floor_dist(), CLIPMASK_PLAYER);
    if (push_ret < 0)
    {
        if (!(pp->Flags & PF_DEAD))
        {
            PlayerUpdateHealth(pp, -actor->user.Health);  // Make sure he dies!
            PlayerCheckDeath(pp, nullptr);

            if (pp->Flags & (PF_DEAD))
                return;
        }
        return;
    }
}

void PlayerCheckValidMove(PLAYER* pp)
{
    if (!pp->insector())
    {
        pp->pos = pp->oldpos;
        pp->cursector = pp->lastcursector;
    }
}

void PlayerSectorBound(PLAYER* pp, int amt)
{
    if (!pp->insector())
        return;

    int cz,fz;

    // player should never go into a sector

    // was getting some problems with this
    // when jumping onto hight sloped sectors

    // call this routine to make sure he doesn't
    // called from DoPlayerMove() but can be called
    // from anywhere it is needed

    getzsofslopeptr(pp->cursector, pp->int_ppos().X, pp->int_ppos().Y, &cz, &fz);

    if (pp->int_ppos().Z > fz - amt)
        pp->set_int_ppos_Z(fz - amt);

    if (pp->int_ppos().Z < cz + amt)
        pp->set_int_ppos_Z(cz + amt);

}

void DoPlayerMove(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    int friction;
    int push_ret = 0;

    // If SO interpolation is disabled, make sure the player's aiming,
    // turning and movement still get appropriately interpolated.
    // We do this from here instead of MovePlayer, covering the case
    // the player gets pushed by a wall (e.g., on the boat in level 5).
    bool interpolate_ride = pp->sop_riding && (!cl_sointerpolation || CommEnabled);

    void SlipSlope(PLAYER* pp);

    SlipSlope(pp);

    if (!SyncInput())
    {
        pp->Flags2 |= (PF2_INPUT_CAN_TURN_GENERAL);
    }
    else
    {
        DoPlayerTurn(pp, pp->input.avel, 1);
    }

    pp->oldpos = pp->pos;
    pp->lastcursector = pp->cursector;

    if (PLAYER_MOVING(pp) == 0)
        pp->Flags &= ~(PF_PLAYER_MOVED);
    else
        pp->Flags |= (PF_PLAYER_MOVED);

    DoPlayerSlide(pp);

    pp->ovect.X = pp->vect.X;
    pp->ovect.Y = pp->vect.Y;

    pp->vect.X += ((pp->input.fvel*synctics*2)<<6);
    pp->vect.Y += ((pp->input.svel*synctics*2)<<6);

    friction = pp->friction;
    if (!(pp->Flags & PF_SWIMMING) && pp->WadeDepth)
    {
        friction -= pp->WadeDepth * 100L;
    }

    pp->vect.X  = MulScale(pp->vect.X, friction, 16);
    pp->vect.Y  = MulScale(pp->vect.Y, friction, 16);

    if (pp->Flags & (PF_FLYING))
    {
        // do a bit of weighted averaging
        pp->vect.X = (pp->vect.X + (pp->ovect.X*1))/2;
        pp->vect.Y = (pp->vect.Y + (pp->ovect.Y*1))/2;
    }
    else if (pp->Flags & (PF_DIVING))
    {
        // do a bit of weighted averaging
        pp->vect.X = (pp->vect.X + (pp->ovect.X*2))/3;
        pp->vect.Y = (pp->vect.Y + (pp->ovect.Y*2))/3;
    }

    if (labs(pp->vect.X) < 12800 && labs(pp->vect.Y) < 12800)
        pp->vect.X = pp->vect.Y = 0;

    actor->spr.xvel = FindDistance2D(pp->vect.X,pp->vect.Y)>>14;

    if (pp->Flags & (PF_CLIP_CHEAT))
    {
        auto sect = pp->cursector;
        if (interpolate_ride)
        {
            pp->opos.XY() = pp->pos.XY();
        }
        pp->add_int_ppos_XY({ pp->vect.X >> 14, pp->vect.Y >> 14 });
        updatesector(pp->int_ppos().X, pp->int_ppos().Y, &sect);
        if (sect != nullptr)
            pp->cursector = sect;
    }
    else
    {
        push_ret = pushmove(pp->pos, &pp->cursector, ((int)actor->spr.clipdist<<2), pp->player_int_ceiling_dist(), pp->player_int_floor_dist() - Z(16), CLIPMASK_PLAYER);

        if (push_ret < 0)
        {
            if (!(pp->Flags & PF_DEAD))
            {
                PlayerUpdateHealth(pp, -actor->user.Health);  // Make sure he dies!
                PlayerCheckDeath(pp, nullptr);

                if (pp->Flags & (PF_DEAD))
                    return;
            }
        }

        if (interpolate_ride)
        {
            pp->opos.XY() = pp->pos.XY();
        }

        auto save_cstat = actor->spr.cstat;
        actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
        Collision coll;
        updatesector(pp->int_ppos().X, pp->int_ppos().Y, &pp->cursector);
        clipmove(pp->pos, &pp->cursector, pp->vect.X, pp->vect.Y, ((int)actor->spr.clipdist<<2), pp->player_int_ceiling_dist(), pp->player_int_floor_dist(), CLIPMASK_PLAYER, coll);

        actor->spr.cstat = save_cstat;
        PlayerCheckValidMove(pp);

        push_ret = pushmove(pp->pos, &pp->cursector, ((int)actor->spr.clipdist<<2), pp->player_int_ceiling_dist(), pp->player_int_floor_dist() - Z(16), CLIPMASK_PLAYER);
        if (push_ret < 0)
        {

            if (!(pp->Flags & PF_DEAD))
            {
                PlayerUpdateHealth(pp, -actor->user.Health);  // Make sure he dies!
                PlayerCheckDeath(pp, nullptr);

                if (pp->Flags & (PF_DEAD))
                    return;
            }
        }
    }

    if (interpolate_ride)
    {
        pp->opos.Z = pp->pos.Z;
        pp->angle.backup();
    }

    // check for warp - probably can remove from CeilingHit
    if (WarpPlane(pp->pos, &pp->cursector))
    {
        PlayerWarpUpdatePos(pp);
    }

    DoPlayerZrange(pp);

    //PlayerSectorBound(pp, Z(1));

    DoPlayerSetWadeDepth(pp);

    if (!SyncInput())
    {
        pp->Flags2 |= (PF2_INPUT_CAN_AIM);
    }
    else
    {
        DoPlayerHorizon(pp, pp->input.horz, 1);
    }

    if (pp->insector() && (pp->cursector->extra & SECTFX_DYNAMIC_AREA))
    {
        if (pp->Flags & (PF_FLYING|PF_JUMPING|PF_FALLING))
        {
            if (pp->pos.Z > pp->loz)
                pp->pos.Z = pp->loz - PLAYER_HEIGHTF;

            if (pp->pos.Z < pp->hiz)
                pp->pos.Z = pp->hiz + PLAYER_HEIGHTF;
        }
        else if (pp->Flags & (PF_SWIMMING|PF_DIVING))
        {
            if (pp->pos.Z > pp->loz)
                pp->pos.Z = pp->loz - PLAYER_SWIM_HEIGHTF;

            if (pp->pos.Z < pp->hiz)
                pp->pos.Z = pp->hiz + PLAYER_SWIM_HEIGHTF;
        }
    }
}

void DoPlayerSectorUpdatePreMove(PLAYER* pp)
{
    auto sect = pp->cursector;

    if (sect == nullptr)
        return;

    if ((pp->cursector->extra & SECTFX_DYNAMIC_AREA))
    {
        updatesectorz(pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, &sect);
        if (sect == nullptr)
        {
            sect = pp->cursector;
            updatesector(pp->int_ppos().X, pp->int_ppos().Y, &sect);
        }
        ASSERT(sect);
    }
    else if (FAF_ConnectArea(sect))
    {
        updatesectorz(pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, &sect);
        if (sect == nullptr)
        {
            sect = pp->cursector;
            updatesector(pp->int_ppos().X, pp->int_ppos().Y, &sect);
        }
        ASSERT(sect);
    }

    pp->setcursector(sect);
}

void DoPlayerSectorUpdatePostMove(PLAYER* pp)
{
    auto sect = pp->cursector;
    int fz,cz;

    // need to do updatesectorz if in connect area
    if (sect != nullptr && FAF_ConnectArea(sect))
    {
        updatesectorz(pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, &pp->cursector);

        // can mess up if below
        if (!pp->insector())
        {
            pp->setcursector(sect);

            // adjust the posz to be in a sector
            getzsofslopeptr(pp->cursector, pp->int_ppos().X, pp->int_ppos().Y, &cz, &fz);
            if (pp->int_ppos().Z > fz)
                pp->set_int_ppos_Z(fz);

            if (pp->int_ppos().Z < cz)
                pp->set_int_ppos_Z(cz);

            // try again
            updatesectorz(pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, &pp->cursector);
        }
    }
    else
    {
        PlayerSectorBound(pp, Z(1));
    }

}

void PlaySOsound(sectortype* sect, short sound_num)
{
    // play idle sound - sound 1
    SWSectIterator it(sect);
    while (auto actor = it.Next())
    {
        if (actor->spr.statnum == STAT_SOUND_SPOT)
        {
            DoSoundSpotStopSound(actor->spr.lotag);
            DoSoundSpotMatch(actor->spr.lotag, sound_num, 0);
        }
    }
}

void StopSOsound(sectortype* sect)
{
    // play idle sound - sound 1
    SWSectIterator it(sect);
    while (auto actor = it.Next())
    {
        if (actor->spr.statnum == STAT_SOUND_SPOT)
            DoSoundSpotStopSound(actor->spr.lotag);
    }
}


void DoTankTreads(PLAYER* pp)
{
    int i;
    int vel;
    sectortype* *sectp;
    int j;
    int dot;
    bool reverse = false;

    if (Prediction)
        return;

    vel = FindDistance2D(pp->vect.X>>8, pp->vect.Y>>8);
    dot = DOT_PRODUCT_2D(pp->vect.X, pp->vect.Y, pp->angle.ang.Cos() * (1 << 14), pp->angle.ang.Sin() * (1 << 14));
    if (dot < 0)
        reverse = true;

    for (sectp = pp->sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        SWSectIterator it(*sectp);
        while (auto actor = it.Next())
        {
            // BOOL1 is set only if pans with SO
            if (!TEST_BOOL1(actor))
                continue;

            if (actor->spr.statnum == STAT_WALL_PAN)
            {
                if (reverse)
                {
                    if (!TEST_BOOL2(actor))
                    {
                        SET_BOOL2(actor);
                        actor->spr.angle += DAngle180;
                    }
                }
                else
                {
                    if (TEST_BOOL2(actor))
                    {
                        RESET_BOOL2(actor);
                        actor->spr.angle += DAngle180;
                    }
                }

                SP_TAG5(actor) = vel;
            }
            else if (actor->spr.statnum == STAT_FLOOR_PAN)
            {
                if (reverse)
                {
                    if (!TEST_BOOL2(actor))
                    {
                        SET_BOOL2(actor);
                        actor->spr.angle += DAngle180;
                    }
                }
                else
                {
                    if (TEST_BOOL2(actor))
                    {
                        RESET_BOOL2(actor);
                        actor->spr.angle += DAngle180;
                    }
                }

                SP_TAG5(actor) = vel;
            }
            else if (actor->spr.statnum == STAT_CEILING_PAN)
            {
                if (reverse)
                {
                    if (!TEST_BOOL2(actor))
                    {
                        SET_BOOL2(actor);
                        actor->spr.angle += DAngle180;
                    }
                }
                else
                {
                    if (TEST_BOOL2(actor))
                    {
                        RESET_BOOL2(actor);
                        actor->spr.angle += DAngle180;
                    }
                }

                SP_TAG5(actor) = vel;
            }
        }
    }


}

void SetupDriveCrush(PLAYER* pp, int *x, int *y)
{
    int radius = pp->sop_control->clipdist;

    x[0] = pp->int_ppos().X - radius;
    y[0] = pp->int_ppos().Y - radius;

    x[1] = pp->int_ppos().X + radius;
    y[1] = pp->int_ppos().Y - radius;

    x[2] = pp->int_ppos().X + radius;
    y[2] = pp->int_ppos().Y + radius;

    x[3] = pp->int_ppos().X - radius;
    y[3] = pp->int_ppos().Y + radius;
}

void DriveCrush(PLAYER* pp, int *x, int *y)
{
    int testpointinquad(int x, int y, int *qx, int *qy);

    SECTOR_OBJECT* sop = pp->sop_control;
    short stat;
    sectortype* *sectp;

    if (MoveSkip4 == 0)
        return;

    // not moving - don't crush
    if ((pp->vect.X|pp->vect.Y) == 0 && pp->input.avel == 0)
        return;

    // main sector
    SWSectIterator it(sop->op_main_sector);
    while (auto actor = it.Next())
    {
        if (testpointinquad(actor->int_pos().X, actor->int_pos().Y, x, y))
        {
            if ((actor->spr.extra & SPRX_BREAKABLE) && HitBreakSprite(actor, 0))
                continue;

            if (actor->spr.statnum == STAT_MISSILE)
                continue;

            if (actor->spr.picnum == ST1)
                continue;

            if ((actor->spr.extra & SPRX_PLAYER_OR_ENEMY))
            {
                if (!(actor->user.Flags & SPR_DEAD) && !(actor->spr.extra & SPRX_BREAKABLE))
                    continue;
            }

            if (actor->spr.cstat & (CSTAT_SPRITE_INVISIBLE))
                continue;

            if (actor->spr.statnum > STAT_DONT_DRAW)
                continue;

            if (actor->int_pos().Z < sop->crush_z)
                continue;

            SpriteQueueDelete(actor);
            KillActor(actor);
        }
    }

    // all enemys
    SWStatIterator it2(STAT_ENEMY);
    while (auto actor = it.Next())
    {
        if (testpointinquad(actor->int_pos().X, actor->int_pos().Y, x, y))
        {
            //if (actor->spr.z < pp->posz)
            if (actor->int_pos().Z < sop->crush_z)
                continue;

            int32_t const vel = FindDistance2D(pp->vect.X>>8, pp->vect.Y>>8);
            if (vel < 9000)
            {
                DoActorBeginSlide(actor, getangle(pp->vect.X, pp->vect.Y), vel/8, 5);
                if (DoActorSlide(actor))
                    continue;
            }

            UpdateSinglePlayKills(actor);

            if (SpawnShrap(actor, nullptr, -99))
                SetSuicide(actor);
            else
                KillActor(actor);
        }
    }

    // all dead actors
    it2.Reset(STAT_DEAD_ACTOR);
    while (auto actor = it.Next())
    {
        if (testpointinquad(actor->int_pos().X, actor->int_pos().Y, x, y))
        {
            if (actor->int_pos().Z < sop->crush_z)
                continue;

            SpriteQueueDelete(actor);
            KillActor(actor);
        }
    }

    // all players
    for (stat = 0; stat < MAX_SW_PLAYERS; stat++)
    {
        it2.Reset(stat);
        auto actor = it.Next();

        if (actor == nullptr)
            continue;

        if (actor->user.PlayerP == pp)
            continue;

        if (testpointinquad(actor->int_pos().X, actor->int_pos().Y, x, y))
        {
            int damage;

            //if (actor->spr.z < pp->posz)
            if (actor->int_pos().Z < sop->crush_z)
                continue;

            damage = -(actor->user.Health + 100);
            PlayerDamageSlide(actor->user.PlayerP, damage, pp->angle.ang.Buildang());
            PlayerUpdateHealth(actor->user.PlayerP, damage);
            PlayerCheckDeath(actor->user.PlayerP, pp->actor);
        }
    }


    // if it ends up actually in the drivable sector kill it
    for (sectp = sop->sectp; *sectp; sectp++)
    {
        it.Reset(*sectp);
        while (auto actor = it.Next())
        {
            // give some extra buffer
            if (actor->int_pos().Z < sop->crush_z + Z(40))
                continue;

            if ((actor->spr.extra & SPRX_PLAYER_OR_ENEMY))
            {
                if (actor->spr.statnum == STAT_ENEMY)
                {
                    if (SpawnShrap(actor, nullptr, -99))
                        SetSuicide(actor);
                    else
                        KillActor(actor);
                }
            }
        }
    }
}

void DoPlayerMoveVehicle(PLAYER* pp)
{
    int z;
    int floor_dist;
    DSWActor* actor = pp->sop->sp_child;
    if (!actor) return;
    DSWActor* plActor = pp->actor;
    int x[4], y[4], ox[4], oy[4];
    int wallcount;
    int count=0;

    sectortype* *sectp;
    SECTOR_OBJECT* sop = pp->sop;
    walltype* wp;
    int j,k;
    short startwall,endwall;

    bool RectClip = !!(sop->flags & SOBJ_RECT_CLIP);

    if (Prediction)
        return;

    if (!Prediction)
    {
        if (labs(pp->input.fvel|pp->input.svel) && !labs(pp->lastinput.fvel| pp->lastinput.svel))
            PlaySOsound(pp->sop->mid_sector,SO_DRIVE_SOUND);
        else if (!labs(pp->input.fvel|pp->input.svel) && labs(pp->lastinput.fvel| pp->lastinput.svel))
            PlaySOsound(pp->sop->mid_sector,SO_IDLE_SOUND);
    }

    // force synchronised input here for now.
    setForcedSyncInput();

    if (PLAYER_MOVING(pp) == 0)
        pp->Flags &= ~(PF_PLAYER_MOVED);
    else
        pp->Flags |= (PF_PLAYER_MOVED);

    pp->ovect.X = pp->vect.X;
    pp->ovect.Y = pp->vect.Y;

    if (sop->drive_speed)
    {
        pp->vect.X = MulScale(pp->input.fvel, sop->drive_speed, 6);
        pp->vect.Y = MulScale(pp->input.svel, sop->drive_speed, 6);

        // does sliding/momentum
        pp->vect.X = (pp->vect.X + (pp->ovect.X*(sop->drive_slide-1)))/sop->drive_slide;
        pp->vect.Y = (pp->vect.Y + (pp->ovect.Y*(sop->drive_slide-1)))/sop->drive_slide;
    }
    else
    {
        pp->vect.X += ((pp->input.fvel*synctics*2)<<6);
        pp->vect.Y += ((pp->input.svel*synctics*2)<<6);

        pp->vect.X  = MulScale(pp->vect.X, TANK_FRICTION, 16);
        pp->vect.Y  = MulScale(pp->vect.Y, TANK_FRICTION, 16);

        pp->vect.X = (pp->vect.X + (pp->ovect.X*1))/2;
        pp->vect.Y = (pp->vect.Y + (pp->ovect.Y*1))/2;
    }

    if (labs(pp->vect.X) < 12800 && labs(pp->vect.Y) < 12800)
        pp->vect.X = pp->vect.Y = 0;

    pp->lastcursector = pp->cursector;
    z = pp->int_ppos().Z + Z(10);

    if (RectClip)
    {
        for (sectp = sop->sectp, wallcount = 0, j = 0; *sectp; sectp++, j++)
        {
            for(auto& wal : wallsofsector(*sectp))
            {
                if (wal.extra && (wal.extra & (WALLFX_LOOP_OUTER|WALLFX_LOOP_OUTER_SECONDARY)) == WALLFX_LOOP_OUTER)
                {
                    x[count] = wal.wall_int_pos().X;
                    y[count] = wal.wall_int_pos().Y;

                    ox[count] = sop->int_pmid().X - sop->xorig[wallcount];
                    oy[count] = sop->int_pmid().Y - sop->yorig[wallcount];

                    count++;
                }

                wallcount++;
            }
        }

        PRODUCTION_ASSERT(count == 4);
    }

    auto save_sect = pp->cursector;
    OperateSectorObject(pp->sop, pp->angle.ang.Buildang(), { MAXSO, MAXSO });
    pp->setcursector(pp->sop->op_main_sector); // for speed

    floor_dist = labs(z - pp->sop->floor_loz);


    if (RectClip)
    {
        HitInfo hit{};
        int vel;
        int ret;

        auto save_cstat = plActor->spr.cstat;
        plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
        DoPlayerTurnVehicleRect(pp, x, y, ox, oy);

        ret = RectClipMove(pp, x, y);
        DriveCrush(pp, x, y);
        plActor->spr.cstat = save_cstat;

        if (!ret)
        {
            vel = FindDistance2D(pp->vect.X>>8, pp->vect.Y>>8);

            if (vel > 13000)
            {
                vec3_t hit_pos = { (x[0] + x[1]) >> 1, (y[0] + y[1]) >> 1, pp->cursector->int_floorz() - Z(10) };

                hitscan(hit_pos, pp->cursector,
                    { MOVEx(256, pp->angle.ang.Buildang()), MOVEy(256, pp->angle.ang.Buildang()), 0 },
                    hit, CLIPMASK_PLAYER);

                if (FindDistance2D(hit.int_hitpos().vec2 - hit_pos.vec2) < 800)
                {
                    if (hit.hitWall)
                        actor->user.coll.setWall(wallnum(hit.hitWall));
                    else if (hit.actor())
                        actor->user.coll.setSprite(hit.actor());
                    else
                        actor->user.coll.setNone();

                    VehicleMoveHit(actor);
                }

                if (!(sop->flags & SOBJ_NO_QUAKE))
                {
                    SetPlayerQuake(pp);
                }
            }

            if (vel > 12000)
            {
                pp->vect.X = pp->vect.Y = pp->ovect.X = pp->ovect.Y = 0;
            }
        }
    }
    else
    {
        if (!SyncInput())
        {
            pp->Flags2 |= (PF2_INPUT_CAN_TURN_VEHICLE);
        }
        else
        {
            DoPlayerTurnVehicle(pp, pp->input.avel, z, floor_dist);
        }

        auto save_cstat = plActor->spr.cstat;
        plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
        if (pp->sop->clipdist)
        {
            vec3_t clippos = { pp->int_ppos().X, pp->int_ppos().Y, z };
            Collision coll;
            clipmove(clippos, &pp->cursector, pp->vect.X, pp->vect.Y, (int)pp->sop->clipdist, Z(4), floor_dist, CLIPMASK_PLAYER, actor->user.coll);

            pp->set_int_ppos_XY(clippos.XY());
        }
        else
        {
            actor->user.coll = MultiClipMove(pp, z, floor_dist);
        }
        plActor->spr.cstat = save_cstat;

        if (actor->user.coll.type != kHitNone)
        {
            int vel;

            vel = FindDistance2D(pp->vect.X>>8, pp->vect.Y>>8);

            if (vel > 13000)
            {
                VehicleMoveHit(actor);
                pp->slide_vect.X = -pp->vect.X<<1;
                pp->slide_vect.Y = -pp->vect.Y<<1;
                if (!(sop->flags & SOBJ_NO_QUAKE))
                    SetPlayerQuake(pp);
            }

            if (vel > 12000)
            {
                pp->vect.X = pp->vect.Y = pp->ovect.X = pp->ovect.Y = 0;
            }
        }
    }

    OperateSectorObject(pp->sop, pp->angle.ang.Buildang(), { pp->int_ppos().X * inttoworld, pp->int_ppos().Y * inttoworld });
    pp->cursector = save_sect; // for speed

    if (!SyncInput())
    {
        pp->Flags2 |= (PF2_INPUT_CAN_AIM);
    }
    else
    {
        DoPlayerHorizon(pp, pp->input.horz, 1);
    }

    DoTankTreads(pp);
}

void DoPlayerMoveTurret(PLAYER* pp)
{
    if (!Prediction)
    {
        if (pp->input.avel && !pp->lastinput.avel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else if (!pp->input.avel && pp->lastinput.avel)
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
    }

    if (!SyncInput())
    {
        pp->Flags2 |= (PF2_INPUT_CAN_TURN_TURRET);
    }
    else
    {
        DoPlayerTurnTurret(pp, pp->input.avel);
    }

    if (PLAYER_MOVING(pp) == 0)
        pp->Flags &= ~(PF_PLAYER_MOVED);
    else
        pp->Flags |= (PF_PLAYER_MOVED);

    if (!SyncInput())
    {
        pp->Flags2 |= (PF2_INPUT_CAN_AIM);
    }
    else
    {
        DoPlayerHorizon(pp, pp->input.horz, 1);
    }
}

void DoPlayerBeginJump(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    pp->Flags |= (PF_JUMPING);
    pp->Flags &= ~(PF_FALLING);
    pp->Flags &= ~(PF_CRAWLING);
    pp->Flags &= ~(PF_LOCK_CRAWL);

    pp->p_floor_dist = PLAYER_JUMP_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_JUMP_CEILING_DIST;
    pp->friction = PLAYER_JUMP_FRICTION;

    PlayerGravity = PLAYER_JUMP_GRAV;

    pp->jump_speed = PLAYER_JUMP_AMT + pp->WadeDepth * 4;

    if (DoPlayerWadeSuperJump(pp))
    {
        pp->jump_speed = PLAYER_JUMP_AMT - pp->WadeDepth * 5;
    }

    pp->JumpDuration = MAX_JUMP_DURATION;
    pp->DoPlayerAction = DoPlayerJump;

    ///DamageData[plActor->user.WeaponNum].Init(pp);

    NewStateGroup(pp->actor, plActor->user.ActorActionSet->Jump);
}

void DoPlayerBeginForceJump(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    pp->Flags |= (PF_JUMPING);
    pp->Flags &= ~(PF_FALLING|PF_CRAWLING|PF_CLIMBING|PF_LOCK_CRAWL);

    pp->JumpDuration = MAX_JUMP_DURATION;
    pp->DoPlayerAction = DoPlayerForceJump;

    pp->p_floor_dist = PLAYER_JUMP_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_JUMP_CEILING_DIST;
    pp->friction = PLAYER_JUMP_FRICTION;

    PlayerGravity = PLAYER_JUMP_GRAV;

    ///DamageData[plActor->user.WeaponNum].Init(pp);

    NewStateGroup(pp->actor, plActor->user.ActorActionSet->Jump);
}

bool PlayerCeilingHit(PLAYER* pp, double zlimit)
{
    return (pp->pos.Z < zlimit);
}

bool PlayerFloorHit(PLAYER* pp, double zlimit)
{
    return (pp->pos.Z > zlimit);
}

void DoPlayerJump(PLAYER* pp)
{
    short i;

    // reset flag key for double jumps
    if (!(pp->input.actions & SB_JUMP))
    {
        pp->KeyPressBits |= SB_JUMP;
    }

    // instead of multiplying by synctics, use a loop for greater accuracy
    for (i = 0; i < synctics; i++)
    {
        // PlayerGravity += synctics;  // See how increase gravity as we go?
        if (pp->input.actions & SB_JUMP)
        {
            if (pp->JumpDuration > 0)
            {
                pp->jump_speed -= PlayerGravity;
                pp->JumpDuration--;
            }
        }

        // adjust jump speed by gravity - if jump speed greater than 0 player
        // have started falling
        if ((pp->jump_speed += PlayerGravity) > 0)
        {
            DoPlayerBeginFall(pp);
            DoPlayerFall(pp);
            return;
        }

        // adjust height by jump speed
        pp->pos.Z += pp->jump_speed * JUMP_FACTOR;

        // if player gets to close the ceiling while jumping
        if (PlayerCeilingHit(pp, pp->hiz + 4))
        {
            // put player at the ceiling
            pp->pos.Z = pp->hiz + 4;

            // reverse your speed to falling
            pp->jump_speed = -pp->jump_speed;

            // start falling
            DoPlayerBeginFall(pp);
            DoPlayerFall(pp);
            return;
        }

        // added this because jumping up to slopes or jumping on steep slopes
        // sometimes caused the view to go into the slope
        // if player gets to close the floor while jumping
        if (PlayerFloorHit(pp, pp->loz - pp->p_floor_dist))
        {
            pp->pos.Z = pp->loz - pp->p_floor_dist;

            pp->jump_speed = 0;
            PlayerSectorBound(pp, Z(1));
            DoPlayerBeginRun(pp);
            DoPlayerHeight(pp);
            return;
        }
    }

    if (PlayerFlyKey())
    {
        DoPlayerBeginFly(pp);
        return;
    }

    // If moving forward and tag is a ladder start climbing
    if (PlayerOnLadder(pp))
    {
        DoPlayerBeginClimb(pp);
        return;
    }

    DoPlayerMove(pp);

    DoPlayerJumpHeight(pp);
}


void DoPlayerForceJump(PLAYER* pp)
{
    short i;

    // instead of multiplying by synctics, use a loop for greater accuracy
    for (i = 0; i < synctics; i++)
    {
        // adjust jump speed by gravity - if jump speed greater than 0 player
        // have started falling
        if ((pp->jump_speed += PlayerGravity) > 0)
        {
            DoPlayerBeginFall(pp);
            DoPlayerFall(pp);
            return;
        }

        // adjust height by jump speed
        pp->pos.Z += pp->jump_speed * JUMP_FACTOR;

        // if player gets to close the ceiling while jumping
        //if (pp->posz < pp->hiz + Z(4))
        if (PlayerCeilingHit(pp, pp->hiz + 4))
        {
            // put player at the ceiling
            pp->pos.Z = pp->hiz + 4;

            // reverse your speed to falling
            pp->jump_speed = -pp->jump_speed;

            // start falling
            DoPlayerBeginFall(pp);
            DoPlayerFall(pp);
            return;
        }
    }

    DoPlayerMove(pp);
}

void DoPlayerBeginFall(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    pp->Flags |= (PF_FALLING);
    pp->Flags &= ~(PF_JUMPING);
    pp->Flags &= ~(PF_CRAWLING);
    pp->Flags &= ~(PF_LOCK_CRAWL);

    pp->p_floor_dist = PLAYER_FALL_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_FALL_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerFall;
    pp->friction = PLAYER_FALL_FRICTION;

    // Only change to falling frame if you were in the jump frame
    // Otherwise an animation may be messed up such as Running Jump Kick
    if (plActor->user.Rot == plActor->user.ActorActionSet->Jump)
        NewStateGroup(pp->actor, plActor->user.ActorActionSet->Fall);
}

void StackedWaterSplash(PLAYER* pp)
{
    if (FAF_ConnectArea(pp->cursector))
    {
        auto sect = pp->cursector;

        updatesectorz(pp->int_ppos().X, pp->int_ppos().Y, int_ActorZOfBottom(pp->actor), &sect);

        if (SectorIsUnderwaterArea(sect))
        {
            PlaySound(DIGI_SPLASH1, pp, v3df_dontpan);
        }
    }
}

void DoPlayerFall(PLAYER* pp)
{
    short i;
    int depth;

    // reset flag key for double jumps
    if (!(pp->input.actions & SB_JUMP))
    {
        pp->KeyPressBits |= SB_JUMP;
    }

    if (SectorIsUnderwaterArea(pp->cursector))
    {
        StackedWaterSplash(pp);
        DoPlayerBeginDiveNoWarp(pp);
        return;
    }

    for (i = 0; i < synctics; i++)
    {
        // adjust jump speed by gravity
        pp->jump_speed += PlayerGravity;
        if (pp->jump_speed > 4100)
            pp->jump_speed = 4100;

        // adjust player height by jump speed
        pp->pos.Z += pp->jump_speed * JUMP_FACTOR;

        if (pp->jump_speed > 2000)
        {
            PlayerSound(DIGI_FALLSCREAM, v3df_dontpan|v3df_doppler|v3df_follow,pp);
        }
        else if (pp->jump_speed > 1300)
        {
            if (!(pp->input.actions & SB_CENTERVIEW))
            {
                pp->input.actions |= SB_CENTERVIEW;
            }
        }



        depth = GetZadjustment(pp->cursector, FLOOR_Z_ADJUST)>>8;
        if (depth == 0)
            depth = pp->WadeDepth;

        double recoil_amnt;

        if (depth > 20)
            recoil_amnt = 0;
        else
            recoil_amnt = min(pp->jump_speed * 6 * JUMP_FACTOR, 35.);

        // need a test for head hits a sloped ceiling while falling
        // if player gets to close the Ceiling while Falling
        if (PlayerCeilingHit(pp, pp->hiz + pp->p_ceiling_dist))
        {
            // put player at the ceiling
            pp->pos.Z = pp->hiz + pp->p_ceiling_dist;
            // don't return or anything - allow to fall until
            // hit floor
        }

        if (PlayerFloorHit(pp, pp->loz - PLAYER_HEIGHTF + recoil_amnt))
        {
            sectortype* sectp = pp->cursector;

            PlayerSectorBound(pp, Z(1));

            if (sectp->hasU() && ((sectp->extra & SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE))
            {
                PlaySound(DIGI_SPLASH1, pp, v3df_dontpan);
            }
            else
            {
                if (pp->jump_speed > 1020)
                    // Feet hitting ground sound
                    PlaySound(DIGI_HITGROUND, pp, v3df_follow|v3df_dontpan);
            }

            StopPlayerSound(pp, DIGI_FALLSCREAM);

            // i any kind of crawl key get rid of recoil
            if (DoPlayerTestCrawl(pp) || (pp->input.actions & SB_CROUCH))
            {
                pp->pos.Z = pp->loz - PLAYER_CRAWL_HEIGHTF;
            }
            else
            {
                // this was causing the z to snap immediately
                // changed it so it stays gradual

                pp->pos.Z += recoil_amnt;
                DoPlayerHeight(pp);
            }

            // do some damage
            if (pp->jump_speed > 1700 && depth == 0)
            {

                PlayerSound(DIGI_PLAYERPAIN2, v3df_follow|v3df_dontpan,pp);

                if (pp->jump_speed > 1700 && pp->jump_speed < 4000)
                {
                    if (pp->jump_speed > 0)
                        PlayerUpdateHealth(pp, -((pp->jump_speed-1700)/40));
                }
                else if (pp->jump_speed >= 4000)
                {
                    DSWActor* plActor = pp->actor;
                    PlayerUpdateHealth(pp, -plActor->user.Health);  // Make sure he dies!
                    plActor->user.Health = 0;
                }

                PlayerCheckDeath(pp, nullptr);

                if (pp->Flags & (PF_DEAD))
                    return;
            }

            if (pp->input.actions & SB_CROUCH)
            {
                StackedWaterSplash(pp);
                DoPlayerBeginCrawl(pp);
                return;
            }

            if (PlayerCanDiveNoWarp(pp))
            {
                DoPlayerBeginDiveNoWarp(pp);
                return;
            }

            StackedWaterSplash(pp);
            DoPlayerBeginRun(pp);
            return;
        }
    }

    if (PlayerFlyKey())
    {
        DoPlayerBeginFly(pp);
        return;
    }

    // If moving forward and tag is a ladder start climbing
    if (PlayerOnLadder(pp))
    {
        DoPlayerBeginClimb(pp);
        return;
    }

    DoPlayerMove(pp);
}

void DoPlayerBeginClimb(PLAYER* pp)
{
    DSWActor* actor = pp->actor;

    pp->Flags &= ~(PF_JUMPING|PF_FALLING);
    pp->Flags &= ~(PF_CRAWLING);
    pp->Flags &= ~(PF_LOCK_CRAWL);

    pp->DoPlayerAction = DoPlayerClimb;

    pp->Flags |= (PF_CLIMBING|PF_WEAPON_DOWN);
    actor->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    //DamageData[plActor->user.WeaponNum].Init(pp);

    //NewStateGroup(pp->actor, pp->actor->user.ActorActionSet->Climb);
    NewStateGroup(pp->actor, sg_PlayerNinjaClimb);
}


void DoPlayerClimb(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    int climb_amt;
    int i;
    int climbvel;
    int dot;
    bool LadderUpdate = false;

    if (Prediction)
        return;

    pp->vect.X += ((pp->input.fvel*synctics*2)<<6);
    pp->vect.Y += ((pp->input.svel*synctics*2)<<6);
    pp->vect.X  = MulScale(pp->vect.X, PLAYER_CLIMB_FRICTION, 16);
    pp->vect.Y  = MulScale(pp->vect.Y, PLAYER_CLIMB_FRICTION, 16);
    if (labs(pp->vect.X) < 12800 && labs(pp->vect.Y) < 12800)
        pp->vect.X = pp->vect.Y = 0;

    climbvel = FindDistance2D(pp->vect.X, pp->vect.Y)>>9;
    dot = DOT_PRODUCT_2D(pp->vect.X, pp->vect.Y, pp->angle.ang.Cos() * (1 << 14), pp->angle.ang.Sin() * (1 << 14));
    if (dot < 0)
        climbvel = -climbvel;

    // need to rewrite this for FAF stuff

    // Jump off of the ladder
    if (pp->input.actions & SB_JUMP)
    {
        pp->Flags &= ~(PF_CLIMBING|PF_WEAPON_DOWN);
        plActor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
        DoPlayerBeginJump(pp);
        return;
    }

    if (climbvel != 0)
    {
        // move player to center of ladder
        for (i = synctics; i; i--)
        {
            const double ADJ_AMT = 0.5;

            auto ppos = pp->pos.XY();
            // player
            if (ppos.X != pp->LadderPosition.X)
            {
                if (ppos.X < pp->LadderPosition.X)
                    ppos.X += ADJ_AMT;
                else if (ppos.X > pp->LadderPosition.X)
                    ppos.X -= ADJ_AMT;

                if (fabs(ppos.X - pp->LadderPosition.X) <= ADJ_AMT)
                    ppos.X = pp->LadderPosition.X;
            }

            if (ppos.Y != pp->LadderPosition.Y)
            {
                if (ppos.Y < pp->LadderPosition.Y)
                    ppos.Y += ADJ_AMT;
                else if (ppos.Y > pp->LadderPosition.Y)
                    ppos.Y -= ADJ_AMT;

                if (fabs(ppos.Y - pp->LadderPosition.Y) <= ADJ_AMT)
                    ppos.Y = pp->LadderPosition.Y;
            }
            pp->pos.XY() = ppos;

            // sprite
            ppos = plActor->spr.pos.XY();
            if (ppos.X != plActor->user.pos.X)
            {
                if (ppos.X < plActor->user.pos.X)
                    ppos.X += ADJ_AMT;
                else if (ppos.X > plActor->user.pos.X)
                    ppos.X -= ADJ_AMT;

                if (abs(ppos.X - plActor->user.pos.X) <= ADJ_AMT)
                    ppos.X = plActor->user.pos.X;
            }

            if (ppos.Y != plActor->user.pos.Y)
            {
                if (ppos.Y < plActor->user.pos.Y)
                    ppos.Y += ADJ_AMT;
                else if (ppos.Y > plActor->user.pos.Y)
                    ppos.Y -= ADJ_AMT;

                if (abs(ppos.Y - plActor->user.pos.Y) <= ADJ_AMT)
                    ppos.Y = plActor->user.pos.Y;
            }
            plActor->spr.pos.XY() = ppos;
        }
    }

    DoPlayerZrange(pp);

    if (!pp->LadderSector)
	{
		return;
	}

    // moving UP
    if (climbvel > 0)
    {
        // pp->climb_ndx += climb_rate * synctics;
        climb_amt = (climbvel>>4) * 8;

        pp->climb_ndx &= 1023;

        pp->add_int_ppos_Z(-climb_amt);

        // if player gets to close the ceiling while climbing
        if (PlayerCeilingHit(pp, pp->hiz))
        {
            // put player at the hiz
            pp->pos.Z = pp->hiz;
            NewStateGroup(pp->actor, sg_PlayerNinjaClimb);
        }

        // if player gets to close the ceiling while climbing
        if (PlayerCeilingHit(pp, pp->hiz + 4))
        {
            // put player at the ceiling
            pp->set_int_ppos_Z(pp->LadderSector->int_ceilingz() + Z(4));
            NewStateGroup(pp->actor, sg_PlayerNinjaClimb);
        }

        // if floor is ABOVE you && your head goes above it, do a jump up to
        // terrace

        if (pp->pos.Z < pp->LadderSector->floorz - 6)
        {
            pp->jump_speed = PLAYER_CLIMB_JUMP_AMT;
            pp->Flags &= ~(PF_CLIMBING|PF_WEAPON_DOWN);
            plActor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
            DoPlayerBeginForceJump(pp);
        }
    }
    else
    // move DOWN
    if (climbvel < 0)
    {
        // pp->climb_ndx += climb_rate * synctics;
        climb_amt = -(climbvel>>4) * 8;

        pp->climb_ndx &= 1023;

        // pp->posz += MulScale(climb_amt, bsin(pp->climb_ndx), 14);
        pp->add_int_ppos_Z(climb_amt);

        // if you are touching the floor
        if (PlayerFloorHit(pp, pp->loz - 4 - PLAYER_HEIGHTF))
        {
            // stand on floor
            pp->pos.Z = pp->loz - 4 - PLAYER_HEIGHTF;

            // if moving backwards start running
            if (climbvel < 0)
            {
                pp->Flags &= ~(PF_CLIMBING|PF_WEAPON_DOWN);
                plActor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
                DoPlayerBeginRun(pp);
                return;
            }
        }
    }
    else
    {
        NewStateGroup(pp->actor, sg_PlayerNinjaClimb);
    }

    // setsprite to players location
    plActor->spr.pos.Z = pp->pos.Z + PLAYER_HEIGHTF;
    ChangeActorSect(pp->actor, pp->cursector);

    if (!SyncInput())
    {
        pp->Flags2 |= (PF2_INPUT_CAN_AIM);
    }
    else
    {
        DoPlayerHorizon(pp, pp->input.horz, 1);
    }

    if (FAF_ConnectArea(pp->cursector))
    {
        updatesectorz(pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, &pp->cursector);
        LadderUpdate = true;
    }

    if (WarpPlane(pp->pos, &pp->cursector))
    {
        PlayerWarpUpdatePos(pp);
        LadderUpdate = true;
    }

    if (LadderUpdate)
    {
        int nx,ny;
        HitInfo near;

        // constantly look for new ladder sector because of warping at any time
        neartag(pp->int_ppos(), pp->cursector, pp->angle.ang.Buildang(), near, 800, NTAG_SEARCH_LO_HI);

        if (near.hitWall)
        {
            auto lActor = FindNearSprite(pp->actor, STAT_CLIMB_MARKER);
            if (!lActor) return;

            // determine where the player is supposed to be in relation to the ladder
            // move out in front of the ladder
            nx = MOVEx(100, lActor->int_ang());
            ny = MOVEy(100, lActor->int_ang());

            // set ladder sector
            pp->LadderSector = near.hitWall->twoSided()? near.hitWall->nextSector() : near.hitWall->sectorp();

            // set players "view" distance from the ladder - needs to be farther than
            // the sprite

            pp->LadderPosition.X = lActor->spr.pos.X + nx * 5 * inttoworld;
            pp->LadderPosition.Y = lActor->spr.pos.Y + ny * 5 * inttoworld;

            pp->angle.settarget(lActor->spr.angle + DAngle180);
        }
    }
}


int DoPlayerWadeSuperJump(PLAYER* pp)
{
    HitInfo hit{};
    unsigned i;
    //short angs[3];
    static short angs[3] = {0, 0, 0};
    int zh = pp->cursector->int_floorz() - Z(pp->WadeDepth) - Z(2);

    if (Prediction) return false;   // !JIM! 8/5/97 Teleporter FAFhitscan SuperJump bug.

    for (i = 0; i < SIZ(angs); i++)
    {
        FAFhitscan(pp->int_ppos().X, pp->int_ppos().Y, zh, pp->cursector,    // Start position
                   bcos(pp->angle.ang.Buildang() + angs[i]),   // X vector of 3D ang
                   bsin(pp->angle.ang.Buildang() + angs[i]),   // Y vector of 3D ang
                   0, hit, CLIPMASK_MISSILE);            // Z vector of 3D ang

        if (hit.hitWall != nullptr && hit.hitSector != nullptr)
        {
            hit.hitSector = hit.hitWall->nextSector();

            if (hit.hitSector != nullptr && labs(hit.hitSector->int_floorz() - pp->int_ppos().Z) < Z(50))
            {
                if (Distance(pp->int_ppos().X, pp->int_ppos().Y, hit.int_hitpos().X, hit.int_hitpos().Y) < ((((int)pp->actor->spr.clipdist)<<2) + 256))
                    return true;
            }
        }
    }

    return false;
}

bool PlayerFlyKey(void)
{
    if (!ToggleFlyMode)
        return false;

    ToggleFlyMode = false;

    if (!GodMode)
        return false;

    return true;
}

void DoPlayerBeginCrawl(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    pp->Flags &= ~(PF_FALLING | PF_JUMPING);
    pp->Flags |= (PF_CRAWLING);

    pp->friction = PLAYER_CRAWL_FRICTION;
    pp->p_floor_dist = PLAYER_CRAWL_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_CRAWL_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerCrawl;

    //pp->posz = pp->loz - PLAYER_CRAWL_HEIGHT;

    NewStateGroup(pp->actor, plActor->user.ActorActionSet->Crawl);
}

bool PlayerFallTest(PLAYER* pp, double player_height)
{
    // If the floor is far below you, fall hard instead of adjusting height
    if (abs(pp->pos.Z - pp->loz) > player_height + PLAYER_FALL_HEIGHTF)
    {
        // if on a STEEP slope sector and you have not moved off of the sector
        if (pp->lo_sectp &&
            labs(pp->lo_sectp->floorheinum) > 3000 &&
            (pp->lo_sectp->floorstat & CSTAT_SECTOR_SLOPE) &&
            pp->lo_sectp == pp->lastcursector)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    return false;
}

const int PLAYER_STANDING_ROOM = 68;

void DoPlayerCrawl(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    if (SectorIsUnderwaterArea(pp->cursector))
    {
        // if stacked water - which it should be
        if (FAF_ConnectArea(pp->cursector))
        {
            // adjust the z
            pp->pos.Z = pp->cursector->ceilingz + 12;
        }

        DoPlayerBeginDiveNoWarp(pp);
        return;
    }

    // Current Z position, adjust down to the floor, adjust to player height,
    // adjust for "bump head"

    // Let off of crawl to get up
    if (!(pp->input.actions & SB_CROUCH))
    {
        if (abs(pp->loz - pp->hiz) >= PLAYER_STANDING_ROOM)
        {
            pp->Flags &= ~(PF_CRAWLING);
            DoPlayerBeginRun(pp);
            return;
        }
    }

    if (pp->lo_sectp && (pp->lo_sectp->extra & SECTFX_CURRENT))
    {
        DoPlayerCurrent(pp);
    }

    // Move around
    DoPlayerMove(pp);

    if (pp->WadeDepth > PLAYER_CRAWL_WADE_DEPTH)
    {
        pp->Flags &= ~(PF_CRAWLING);
        DoPlayerBeginRun(pp);
        return;
    }

    if (!(pp->Flags & PF_PLAYER_MOVED))
    {
        NewStateGroup(pp->actor, plActor->user.ActorActionSet->Crawl);
    }

    // If the floor is far below you, fall hard instead of adjusting height
    if (PlayerFallTest(pp, PLAYER_CRAWL_HEIGHTF))
    {
        pp->jump_speed = Z(1);
        pp->Flags &= ~(PF_CRAWLING);
        DoPlayerBeginFall(pp);
        // call PlayerFall now seems to iron out a hitch before falling
        DoPlayerFall(pp);
        return;
    }

    if (pp->insector() && (pp->cursector->extra & SECTFX_DYNAMIC_AREA))
    {
        pp->pos.Z = pp->loz - PLAYER_CRAWL_HEIGHTF;
    }

    DoPlayerBob(pp);
    DoPlayerCrawlHeight(pp);
}

void DoPlayerBeginFly(PLAYER* pp)
{
    pp->Flags &= ~(PF_FALLING | PF_JUMPING | PF_CRAWLING);
    pp->Flags |= (PF_FLYING);

    pp->friction = PLAYER_FLY_FRICTION;
    pp->p_floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerFly;

    pp->z_speed = -Z(10);
    pp->jump_speed = 0;
    pp->bob_amt = 0;
    pp->bob_ndx = 1024;

    NewStateGroup(pp->actor, sg_PlayerNinjaFly);
}

int GetSinNdx(int range, int bob_amt)
{
    int amt;

    amt = Z(512) / range;

    return bob_amt * amt;
}

void PlayerWarpUpdatePos(PLAYER* pp)
{
    if (Prediction)
        return;

    pp->opos = pp->pos;
    DoPlayerZrange(pp);
    UpdatePlayerSprite(pp);
}

void DoPlayerFly(PLAYER* pp)
{
    if (SectorIsUnderwaterArea(pp->cursector))
    {
        DoPlayerBeginDiveNoWarp(pp);
        return;
    }

    if (pp->input.actions & SB_CROUCH)
    {
        pp->z_speed += PLAYER_FLY_INC;

        if (pp->z_speed > PLAYER_FLY_MAX_SPEED)
            pp->z_speed = PLAYER_FLY_MAX_SPEED;
    }

    if (pp->input.actions & SB_JUMP)
    {
        pp->z_speed -= PLAYER_FLY_INC;

        if (pp->z_speed < -PLAYER_FLY_MAX_SPEED)
            pp->z_speed = -PLAYER_FLY_MAX_SPEED;
    }

    pp->z_speed = MulScale(pp->z_speed, 58000, 16);

    pp->add_int_ppos_Z(pp->z_speed);

    // Make the min distance from the ceiling/floor match bobbing amount
    // so the player never goes into the ceiling/floor

    // Only get so close to the ceiling
    if (PlayerCeilingHit(pp, pp->hiz + PLAYER_FLY_BOB_AMTF + 8))
    {
        pp->pos.Z = pp->hiz + PLAYER_FLY_BOB_AMTF + 8;
        pp->z_speed = 0;
    }

    // Only get so close to the floor
    if (PlayerFloorHit(pp, pp->loz - PLAYER_HEIGHTF - PLAYER_FLY_BOB_AMTF))
    {
        pp->pos.Z = pp->loz - PLAYER_HEIGHTF - PLAYER_FLY_BOB_AMTF;
        pp->z_speed = 0;
    }

    if (PlayerFlyKey())
    {
        pp->Flags &= ~(PF_FLYING);
        pp->bob_amt = 0;
        pp->bob_ndx = 0;
        DoPlayerBeginFall(pp);
        DoPlayerFall(pp);
        return;
    }

    DoPlayerMove(pp);
}


DSWActor* FindNearSprite(DSWActor* actor, short stat)
{
    int fs;
    int dist, near_dist = 15000;
    DSWActor* near_fp = nullptr;


    SWStatIterator it(stat);
    while (auto itActor = it.Next())
    {
        dist = DistanceI(actor->spr.pos, itActor->spr.pos);

        if (dist < near_dist)
        {
            near_dist = dist;
            near_fp = itActor;
        }
    }

    return near_fp;
}

bool PlayerOnLadder(PLAYER* pp)
{
    int nx, ny;
    unsigned i;
    HitInfo hit, near;
    int dir, dist;


    static short angles[] =
    {
        30, -30
    };

    if (Prediction)
        return false;

    neartag(pp->int_ppos(), pp->cursector, pp->angle.ang.Buildang(), near, 1024 + 768, NTAG_SEARCH_LO_HI);

    dir = DOT_PRODUCT_2D(pp->vect.X, pp->vect.Y, pp->angle.ang.Cos() * (1 << 14), pp->angle.ang.Sin() * (1 << 14));

    if (dir < 0)
        return false;

    if (near.hitWall == nullptr || near.hitWall->lotag != TAG_WALL_CLIMB)
        return false;

    for (i = 0; i < SIZ(angles); i++)
    {
        neartag(pp->int_ppos(), pp->cursector, NORM_ANGLE(pp->angle.ang.Buildang() + angles[i]), near, 600, NTAG_SEARCH_LO_HI);

        if (near.hitWall == nullptr || near.int_hitpos().X < 100 || near.hitWall->lotag != TAG_WALL_CLIMB)
            return false;

        FAFhitscan(pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, pp->cursector,
                   bcos(pp->angle.ang.Buildang() + angles[i]),
                   bsin(pp->angle.ang.Buildang() + angles[i]),
                   0,
                   hit, CLIPMASK_MISSILE);

        dist = DIST(pp->int_ppos().X, pp->int_ppos().Y, hit.int_hitpos().X, hit.int_hitpos().Y);

        if (hit.actor() != nullptr)
        {
            int cstat = hit.actor()->spr.cstat;
            // if the sprite blocking you hit is not a wall sprite there is something between
            // you and the ladder
            if ((cstat & CSTAT_SPRITE_BLOCK) &&
                !(cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                return false;
            }
        }
        else
        {
            // if you hit a wall and it is not a climb wall - forget it
            if (hit.hitWall != nullptr && hit.hitWall->lotag != TAG_WALL_CLIMB)
                return false;
        }
    }


    auto lActor = FindNearSprite(pp->actor, STAT_CLIMB_MARKER);

    if (!lActor)
        return false;

    // determine where the player is supposed to be in relation to the ladder
    // move out in front of the ladder
    nx = MOVEx(100, lActor->int_ang());
    ny = MOVEy(100, lActor->int_ang());

    pp->LadderSector = near.hitWall->twoSided() ? near.hitWall->nextSector() : near.hitWall->sectorp();

    // set players "view" distance from the ladder - needs to be farther than
    // the sprite

    pp->LadderPosition.X = lActor->spr.pos.X + nx * 5 * inttoworld;
    pp->LadderPosition.Y = lActor->spr.pos.Y + ny * 5 * inttoworld;

    pp->angle.settarget(lActor->spr.angle + DAngle180);

    return true;
}

bool DoPlayerTestCrawl(PLAYER* pp)
{
    if (abs(pp->loz - pp->hiz) < PLAYER_STANDING_ROOM)
        return true;

    return false;
}

int PlayerInDiveArea(PLAYER* pp)
{
    sectortype* sectp;

    if (pp->lo_sectp)
    {
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //Attention: This changed on 07/29/97
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        sectp = pp->cursector;
        //sectp = pp->lo_sectp;
    }
    else
        return false;

    if ((sectp->extra & SECTFX_DIVE_AREA))
    {
        CheckFootPrints(pp);
        return true;
    }

    return false;
}

int PlayerCanDive(PLAYER* pp)
{
    if (Prediction)
        return false;

    // Crawl - check for diving
    if ((pp->input.actions & SB_CROUCH) || pp->jump_speed > 0)
    {
        if (PlayerInDiveArea(pp))
        {
            pp->pos.Z += 20;
            pp->z_speed = Z(20);
            pp->jump_speed = 0;

            if (pp->pos.Z > pp->loz - pp->WadeDepth - 2)
            {
                DoPlayerBeginDive(pp);
            }

            return true;
        }
    }

    return false;
}

int PlayerCanDiveNoWarp(PLAYER* pp)
{
    if (Prediction)
        return false;

    // check for diving
    if (pp->jump_speed > 1400)
    {
        if (FAF_ConnectArea(pp->cursector))
        {
            auto sect = pp->cursector;

            updatesectorz(pp->int_ppos().X, pp->int_ppos().Y, int_ActorZOfBottom(pp->actor), &sect);

            if (SectorIsUnderwaterArea(sect))
            {
                pp->setcursector(sect);
                pp->set_int_ppos_Z(sect->int_ceilingz() + Z(20));

                pp->z_speed = Z(20);
                pp->jump_speed = 0;

                PlaySound(DIGI_SPLASH1, pp, v3df_dontpan);
                DoPlayerBeginDiveNoWarp(pp);
                return true;
            }
        }
    }

    return false;
}


int GetOverlapSector(int x, int y, sectortype** over, sectortype** under)
{
    int i, found = 0;
    sectortype* sf[3]= {nullptr,nullptr};                       // sectors found
    auto secto = *over;
    auto sectu = *under;

    if ((sectu->hasU() && sectu->number >= 30000) || (secto->hasU() && secto->number >= 30000))
        return GetOverlapSector2(x,y,over,under);

    // instead of check ALL sectors, just check the two most likely first
    if (inside(x, y, *over))
    {
        sf[found] = *over;
        found++;
    }

    if (inside(x, y, *under))
    {
        sf[found] = *under;
        found++;
    }

    // if nothing was found, check them all
    if (found == 0)
    {
        for (auto& sect: sector)
        {
            if (inside(x, y, &sect))
            {
                sf[found] = &sect;
                found++;
                if (found > 2) return 0;
            }
        }
    }

    if (!found)
    {
        // Contrary to expectations, this *CAN* happen in valid scenarios and therefore should not abort.
        //I_Error("GetOverlapSector x = %d, y = %d, over %d, under %d", x, y, sectnum(*over), sectnum(*under));
        return 0;
    }

    PRODUCTION_ASSERT(found <= 2);

    // the are overlaping - check the z coord
    if (found == 2)
    {
        if (sf[0]->floorz > sf[1]->floorz)
        {
            *under = sf[0];
            *over = sf[1];
        }
        else
        {
            *under = sf[1];
            *over = sf[0];
        }
    }
    else
    // the are NOT overlaping
    {
        *over = sf[0];
        *under = nullptr;
    }

    return found;
}

int GetOverlapSector2(int x, int y, sectortype** over, sectortype** under)
{
    int found = 0;
    sectortype* sf[2]= {nullptr, nullptr};                       // sectors found

    unsigned stat;
    static short UnderStatList[] = {STAT_UNDERWATER, STAT_UNDERWATER2};

    // NOTE: For certain heavily overlapped areas in $seabase this is a better
    // method.

    // instead of check ALL sectors, just check the two most likely first
    if (inside(x, y, *over))
    {
        sf[found] = *over;
        found++;
    }

    if (inside(x, y, *under))
    {
        sf[found] = *under;
        found++;
    }

    // if nothing was found, check them all
    if (found == 0)
    {
        SWStatIterator it(STAT_DIVE_AREA);
        while (auto actor = it.Next())
        {
            if (inside(x, y, actor->sector()))
            {
                sf[found] = actor->sector();
                found++;
                PRODUCTION_ASSERT(found <= 2);
            }
        }

        for (stat = 0; stat < SIZ(UnderStatList); stat++)
        {
            it.Reset(UnderStatList[stat]);
            while (auto actor = it.Next())
            {
                // ignore underwater areas with lotag of 0
                if (actor->spr.lotag == 0)
                    continue;

                if (inside(x, y, actor->sector()))
                {
                    sf[found] = actor->sector();
                    found++;
                    PRODUCTION_ASSERT(found <= 2);
                }
            }
        }
    }

    if (!found)
    {
        // Contrary to expectations, this *CAN* happen in valid scenarios and therefore should not abort.
        //I_Error("GetOverlapSector2 x = %d, y = %d, over %d, under %d", x, y, sectnum(*over), sectnum(*under));
        return 0;
    }

    PRODUCTION_ASSERT(found <= 2);

    // the are overlaping - check the z coord
    if (found == 2)
    {
        if (sf[0]->floorz > sf[1]->floorz)
        {
            *under = sf[0];
            *over = sf[1];
        }
        else
        {
            *under = sf[1];
            *over = sf[0];
        }
    }
    else
    // the are NOT overlaping
    {
        *over = sf[0];
        *under = nullptr;
    }

    return found;
}


void DoPlayerWarpToUnderwater(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    auto sectu = pp->cursector;
    DSWActor* under_act = nullptr, * over_act = nullptr;
    bool Found = false;

    if (Prediction)
        return;


    // search for DIVE_AREA "over" sprite for reference point
    SWStatIterator it(STAT_DIVE_AREA);
    while ((over_act = it.Next()))
    {
        if ((over_act->sector()->extra & SECTFX_DIVE_AREA) &&
            over_act->sector()->hasU() &&
            over_act->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    PRODUCTION_ASSERT(Found == true);
    Found = false;

    // search for UNDERWATER "under" sprite for reference point
    it.Reset(STAT_UNDERWATER);
    while ((under_act = it.Next()))
    {
        if ((under_act->sector()->extra & SECTFX_UNDERWATER) &&
            under_act->sector()->hasU() &&
            under_act->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    PRODUCTION_ASSERT(Found == true);

    // get the offset from the sprite
    plActor->user.pos.XY() = over_act->spr.pos.XY() - pp->pos.XY();

    // update to the new x y position
    pp->pos.XY() = under_act->spr.pos.XY() - plActor->user.pos.XY();

    auto over  = over_act->sector();
    auto under = under_act->sector();

    if (GetOverlapSector(pp->int_ppos().X, pp->int_ppos().Y, &over, &under) == 2)
    {
        pp->setcursector(under);
    }
    else
        pp->setcursector(over);

    pp->set_int_ppos_Z(under_act->sector()->int_ceilingz() + Z(6));

    pp->opos = pp->pos;

    DoPlayerZrange(pp);
    return;
}

void DoPlayerWarpToSurface(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    auto sectu = pp->cursector;

    DSWActor* under_act = nullptr, * over_act = nullptr;
    bool Found = false;

    if (Prediction)
        return;

    // search for UNDERWATER "under" sprite for reference point
    SWStatIterator it(STAT_UNDERWATER);
    while ((under_act = it.Next()))
    {
        if ((under_act->sector()->extra & SECTFX_UNDERWATER) &&
            under_act->sector()->hasU() &&
            under_act->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    PRODUCTION_ASSERT(Found == true);
    Found = false;

    // search for DIVE_AREA "over" sprite for reference point
    it.Reset(STAT_DIVE_AREA);
    while ((over_act = it.Next()))
    {
        if ((over_act->sector()->extra & SECTFX_DIVE_AREA) &&
            over_act->sector()->hasU() &&
            over_act->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    PRODUCTION_ASSERT(Found == true);

    // get the offset from the under sprite
    plActor->user.pos.XY() = under_act->spr.pos.XY() - pp->pos.XY();

    // update to the new x y position
    pp->pos.XY() = over_act->spr.pos.XY() - plActor->user.pos.XY();

    auto over = over_act->sector();
    auto under = under_act->sector();

    if (GetOverlapSector(pp->int_ppos().X, pp->int_ppos().Y, &over, &under))
    {
        pp->setcursector(over);
    }

    pp->pos.Z = over_act->sector()->floorz - 2;

    // set z range and wade depth so we know how high to set view
    DoPlayerZrange(pp);
    DoPlayerSetWadeDepth(pp);

    pp->pos.Z -= pp->WadeDepth;

    pp->opos = pp->pos;

    return;
}


void DoPlayerDivePalette(PLAYER* pp)
{
    if (pp != Player + screenpeek) return;

    if ((pp->DeathType == PLAYER_DEATH_DROWN || ((Player+screenpeek)->Flags & PF_DIVING)) && !(pp->Flags & PF_DIVING_IN_LAVA))
    {
        SetFadeAmt(pp,-1005,210); // Dive color , org color 208
    }
    else
    {
        // Put it all back to normal
        if (pp->StartColor == 210)
        {
            videoFadePalette(0,0,0,0);
            pp->FadeAmt = 0;
        }
    }
}


void DoPlayerBeginDive(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    if (Prediction)
        return;

    if (pp->Bloody) pp->Bloody = false; // Water washes away the blood

    pp->Flags |= (PF_DIVING);
    DoPlayerDivePalette(pp);
    DoPlayerNightVisionPalette(pp);

    if (pp == Player + screenpeek)
    {
        COVER_SetReverb(140); // Underwater echo
        pp->Reverb = 140;
    }

    SpawnSplash(pp->actor);

    DoPlayerWarpToUnderwater(pp);
    OperateTripTrigger(pp);

    pp->Flags &= ~(PF_JUMPING | PF_FALLING);
    pp->Flags &= ~(PF_CRAWLING);
    pp->Flags &= ~(PF_LOCK_CRAWL);

    pp->friction = PLAYER_DIVE_FRICTION;
    pp->p_ceiling_dist = PLAYER_DIVE_CEILING_DIST;
    pp->p_floor_dist = PLAYER_DIVE_FLOOR_DIST;
    plActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    pp->DoPlayerAction = DoPlayerDive;

    //pp->z_speed = 0;

    pp->DiveTics = PLAYER_DIVE_TIME;
    pp->DiveDamageTics = 0;

    DoPlayerMove(pp); // needs to be called to reset the pp->loz/hiz variable
    ///DamageData[plActor->user.WeaponNum].Init(pp);

    NewStateGroup(pp->actor, plActor->user.ActorActionSet->Dive);

    DoPlayerDive(pp);
}

void DoPlayerBeginDiveNoWarp(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    if (Prediction)
        return;

    if (!SectorIsUnderwaterArea(pp->cursector))
        return;

    if (pp->Bloody) pp->Bloody = false; // Water washes away the blood

    if (pp == Player + screenpeek)
    {
        COVER_SetReverb(140); // Underwater echo
        pp->Reverb = 140;
    }

    CheckFootPrints(pp);

    if ((pp->lo_sectp->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
    {
        pp->Flags |= (PF_DIVING_IN_LAVA);
        plActor->user.DamageTics = 0;
    }

    pp->Flags |= (PF_DIVING);
    DoPlayerDivePalette(pp);
    DoPlayerNightVisionPalette(pp);

    pp->Flags &= ~(PF_JUMPING | PF_FALLING);

    pp->friction = PLAYER_DIVE_FRICTION;
    pp->p_ceiling_dist = PLAYER_DIVE_CEILING_DIST;
    pp->p_floor_dist = PLAYER_DIVE_FLOOR_DIST;
    plActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    pp->DoPlayerAction = DoPlayerDive;
    pp->z_speed = 0;
    pp->DiveTics = PLAYER_DIVE_TIME;
    pp->DiveDamageTics = 0;
    DoPlayerMove(pp); // needs to be called to reset the pp->loz/hiz variable
    ///DamageData[plActor->user.WeaponNum].Init(pp);
    NewStateGroup(pp->actor, plActor->user.ActorActionSet->Dive);
    DoPlayerDive(pp);
}

void DoPlayerStopDiveNoWarp(PLAYER* pp)
{
    if (Prediction)
        return;

    StopPlayerSound(pp);

    // stop diving no warp
    PlayerSound(DIGI_SURFACE, v3df_dontpan|v3df_follow|v3df_doppler,pp);

    pp->bob_amt = 0;

    pp->Flags &= ~(PF_DIVING|PF_DIVING_IN_LAVA);
    DoPlayerDivePalette(pp);
    DoPlayerNightVisionPalette(pp);
    pp->actor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
    if (pp == Player + screenpeek)
    {
        COVER_SetReverb(0);
        pp->Reverb = 0;
    }

    DoPlayerZrange(pp);
}

void DoPlayerStopDive(PLAYER* pp)
{
    DSWActor* actor = pp->actor;

    if (Prediction)
        return;

    StopPlayerSound(pp);

    // stop diving with warp
    PlayerSound(DIGI_SURFACE, v3df_dontpan|v3df_follow|v3df_doppler,pp);

    pp->bob_amt = 0;
    DoPlayerWarpToSurface(pp);
    DoPlayerBeginWade(pp);
    pp->Flags &= ~(PF_DIVING|PF_DIVING_IN_LAVA);

    DoPlayerDivePalette(pp);
    DoPlayerNightVisionPalette(pp);
    actor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
    if (pp == Player + screenpeek)
    {
        COVER_SetReverb(0);
        pp->Reverb = 0;
    }
}

void DoPlayerDiveMeter(PLAYER* pp)
{
    short color=0,metertics,meterunit;
    int y;


    if (NoMeters) return;

    // Don't draw bar from other players
    if (pp != Player+myconnectindex) return;

    if (!(pp->Flags & (PF_DIVING|PF_DIVING_IN_LAVA))) return;

    meterunit = PLAYER_DIVE_TIME / 30;
    if (meterunit > 0)
        metertics = pp->DiveTics / meterunit;
    else
        return;

    if (metertics <= 0 && !(pp->Flags & (PF_DIVING|PF_DIVING_IN_LAVA)))
    {
        return;
    }

    if (metertics <= 0) return;

    if (numplayers < 2) y = 10;
    else if (numplayers >=2 && numplayers <= 4) y = 20;
    else
        y = 30;

    if (metertics <= 12 && metertics > 6)
        color = 20;
    else if (metertics <= 6)
        color = 25;
    else
        color = 22;

    DrawTexture(twod, tileGetTexture(5408, true), 208, y, DTA_FullscreenScale, FSMode_Fit320x200,
        DTA_CenterOffsetRel, 2, DTA_TranslationIndex, TRANSLATION(Translation_Remap, 1), TAG_DONE);

    DrawTexture(twod, tileGetTexture(5406 - metertics, true), 265, y, DTA_FullscreenScale, FSMode_Fit320x200,
        DTA_CenterOffsetRel, 2, DTA_TranslationIndex, TRANSLATION(Translation_Remap, color), TAG_DONE);
}

void DoPlayerDive(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    auto sectu = pp->cursector;

    // whenever your view is not in a water area
    if (!SectorIsUnderwaterArea(pp->cursector))
    {
        DoPlayerStopDiveNoWarp(pp);
        DoPlayerBeginRun(pp);
        return;
    }

    if ((pp->DiveTics -= synctics) < 0)
    {
        if ((pp->DiveDamageTics -= synctics) < 0)
        {
            pp->DiveDamageTics = PLAYER_DIVE_DAMAGE_TIME;
            //PlayerUpdateHealth(pp, PLAYER_DIVE_DAMAGE_AMOUNT);
            PlayerSound(DIGI_WANGDROWNING, v3df_dontpan|v3df_follow, pp);
            PlayerUpdateHealth(pp, -3 -(RandomRange(7<<8)>>8));
            PlayerCheckDeath(pp, nullptr);
            if (pp->Flags & (PF_DEAD))
                return;
        }
    }

    // underwater current
    if (pp->lo_sectp && (pp->lo_sectp->extra & SECTFX_CURRENT))
    {
        DoPlayerCurrent(pp);
    }

    // while diving in lava
    // every DamageTics time take some damage
    if (pp->Flags & (PF_DIVING_IN_LAVA))
    {
        if ((plActor->user.DamageTics -= synctics) < 0)
        {
            plActor->user.DamageTics = 30;   // !JIM! Was DAMAGE_TIME

            PlayerUpdateHealth(pp, -40);
        }
    }

    if (pp->input.actions & SB_CROUCH)
    {
        pp->z_speed += PLAYER_DIVE_INC;

        if (pp->z_speed > PLAYER_DIVE_MAX_SPEED)
            pp->z_speed = PLAYER_DIVE_MAX_SPEED;
    }

    if (pp->input.actions & SB_JUMP)
    {
        pp->z_speed -= PLAYER_DIVE_INC;

        if (pp->z_speed < -PLAYER_DIVE_MAX_SPEED)
            pp->z_speed = -PLAYER_DIVE_MAX_SPEED;
    }

    pp->z_speed = MulScale(pp->z_speed, 58000, 16);

    if (labs(pp->z_speed) < 16)
        pp->z_speed = 0;

    pp->add_int_ppos_Z(pp->z_speed);

    if (pp->z_speed < 0 && FAF_ConnectArea(pp->cursector))
    {
        if (pp->int_ppos().Z < pp->cursector->int_ceilingz() + Z(10))
        {
            auto sect = pp->cursector;

            // check for sector above to see if it is an underwater sector also
            updatesectorz(pp->int_ppos().X, pp->int_ppos().Y, pp->cursector->int_ceilingz() - Z(8), &sect);

            if (!SectorIsUnderwaterArea(sect))
            {
                // if not underwater sector we must surface
                // force into above sector
                pp->set_int_ppos_Z(pp->cursector->int_ceilingz() - Z(8));
                pp->setcursector(sect);
                DoPlayerStopDiveNoWarp(pp);
                DoPlayerBeginRun(pp);
                return;
            }
        }
    }


    // Only get so close to the ceiling
    // if its a dive sector without a match or a UNDER2 sector with CANT_SURFACE set
    if (sectu && (sectu->number == 0 || (sectu->flags & SECTFU_CANT_SURFACE)))
    {
        // for room over room water the hiz will be the top rooms ceiling
        if (pp->pos.Z < pp->hiz + pp->p_ceiling_dist)
        {
            pp->pos.Z = pp->hiz + pp->p_ceiling_dist;
        }
    }
    else
    {
        // close to a warping sector - stop diveing with a warp to surface
        // !JIM! FRANK - I added !pp->hiActor so that you don't warp to surface when
        //     there is a sprite above you since getzrange returns a hiz < ceiling height
        //     if you are clipping into a sprite and not the ceiling.
        if (pp->pos.Z < pp->hiz + 4 && !pp->highActor)
        {
            DoPlayerStopDive(pp);
            return;
        }
    }

    // Only get so close to the floor
    if (pp->pos.Z >= pp->loz - PLAYER_DIVE_HEIGHTF)
    {
        pp->pos.Z = pp->loz - PLAYER_DIVE_HEIGHTF;
    }

    // make player bob if sitting still
    if (!PLAYER_MOVING(pp) && pp->z_speed == 0 && pp->up_speed == 0)
    {
        DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHTF, PLAYER_DIVE_BOB_AMTF, 3);
    }
    // player is moving
    else
    {
        // if bob_amt is approx 0
        if (abs(pp->bob_amt) < 1)
        {
            pp->bob_amt = 0;
            pp->bob_ndx = 0;
        }
        // else keep bobbing until its back close to 0
        else
        {
            DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHTF, PLAYER_DIVE_BOB_AMTF, 3);
        }
    }

    // Reverse bobbing when getting close to the floor
    if (pp->pos.Z + pp->bob_amt >= pp->loz - PLAYER_DIVE_HEIGHTF)
    {
        pp->bob_ndx = NORM_ANGLE(pp->bob_ndx + ((1024 + 512) - pp->bob_ndx) * 2);
        DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHTF, PLAYER_DIVE_BOB_AMTF, 3);
    }
    // Reverse bobbing when getting close to the ceiling
    if (pp->pos.Z + pp->bob_amt < pp->hiz + pp->p_ceiling_dist)
    {
        pp->bob_ndx = NORM_ANGLE(pp->bob_ndx + ((512) - pp->bob_ndx) * 2);
        DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHTF, PLAYER_DIVE_BOB_AMTF, 3);
    }

    DoPlayerMove(pp);

    // Random bubble sounds
    // if((RandomRange(1000<<5)>>5) < 100)
    //     PlaySound(DIGI_BUBBLES, pp, v3df_dontpan|v3df_follow);

    if ((!Prediction && pp->z_speed && ((RANDOM_P2(1024<<5)>>5) < 64)) ||
        (PLAYER_MOVING(pp) && (RANDOM_P2(1024<<5)>>5) < 64))
    {
        int nx,ny;

        PlaySound(DIGI_BUBBLES, pp, v3df_none);
        auto bubble = SpawnBubble(pp->actor);
        if (bubble != nullptr)
        {
            // back it up a bit to get it out of your face
            nx = MOVEx((128+64), NORM_ANGLE(bubble->int_ang() + 1024));
            ny = MOVEy((128+64), NORM_ANGLE(bubble->int_ang() + 1024));

            move_sprite(bubble, nx, ny, 0L, plActor->user.int_ceiling_dist(), plActor->user.int_floor_dist(), 0, synctics);
        }
    }
}

int DoPlayerTestPlaxDeath(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    // landed on a paralax floor
    if (pp->lo_sectp && (pp->lo_sectp->floorstat & CSTAT_SECTOR_SKY))
    {
        PlayerUpdateHealth(pp, -plActor->user.Health);
        PlayerCheckDeath(pp, nullptr);
        return true;
    }

    return false;
}

void DoPlayerCurrent(PLAYER* pp)
{
    int xvect, yvect;
    auto sectu = pp->cursector;
    int push_ret;

    if (!sectu)
        return;

    xvect = sectu->speed * synctics * bcos(sectu->ang) >> 4;
    yvect = sectu->speed * synctics * bsin(sectu->ang) >> 4;

    push_ret = pushmove(pp->pos, &pp->cursector, ((int)pp->actor->spr.clipdist<<2), pp->player_int_ceiling_dist(), pp->player_int_floor_dist(), CLIPMASK_PLAYER);
    if (push_ret < 0)
    {
        if (!(pp->Flags & PF_DEAD))
        {
            DSWActor* plActor = pp->actor;

            PlayerUpdateHealth(pp, -plActor->user.Health);  // Make sure he dies!
            PlayerCheckDeath(pp, nullptr);

            if (pp->Flags & (PF_DEAD))
                return;
        }
        return;
    }
    Collision coll;
    clipmove(pp->pos, &pp->cursector, xvect, yvect, ((int)pp->actor->spr.clipdist<<2), pp->player_int_ceiling_dist(), pp->player_int_floor_dist(), CLIPMASK_PLAYER, coll);

    PlayerCheckValidMove(pp);
    pushmove(pp->pos, &pp->cursector, ((int)pp->actor->spr.clipdist<<2), pp->player_int_ceiling_dist(), pp->player_int_floor_dist(), CLIPMASK_PLAYER);
    if (push_ret < 0)
    {
        if (!(pp->Flags & PF_DEAD))
        {
            DSWActor* plActor = pp->actor;

            PlayerUpdateHealth(pp, -plActor->user.Health);  // Make sure he dies!
            PlayerCheckDeath(pp, nullptr);

            if (pp->Flags & (PF_DEAD))
                return;
        }
        return;
    }
}

void DoPlayerFireOutWater(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    if (Prediction)
        return;

    if (pp->WadeDepth > 20)
    {
        if (plActor->user.flameActor != nullptr)
            SetSuicide(plActor->user.flameActor);
        plActor->user.flameActor = nullptr;
        plActor->user.Flags2 |= SPR2_FLAMEDIE;
    }
}

void DoPlayerFireOutDeath(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    if (Prediction)
        return;

    if (plActor->user.flameActor != nullptr)
        SetSuicide(plActor->user.flameActor);

    plActor->user.flameActor = nullptr;
    plActor->user.Flags2 |= SPR2_FLAMEDIE;
}

void DoPlayerBeginWade(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    // landed on a paralax floor?
    if (DoPlayerTestPlaxDeath(pp))
        return;

    pp->Flags &= ~(PF_JUMPING | PF_FALLING);
    pp->Flags &= ~(PF_CRAWLING);

    pp->friction = PLAYER_WADE_FRICTION;
    pp->p_floor_dist = PLAYER_WADE_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_WADE_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerWade;

    DoPlayerFireOutWater(pp);

    if (pp->jump_speed > 100)
        SpawnSplash(pp->actor);

    // fix it so that you won't go under water unless you hit the water at a
    // certain speed
    if (pp->jump_speed > 0 && pp->jump_speed < 1300)
        pp->jump_speed = 0;

    ASSERT(plActor->user.ActorActionSet->Run);

    NewStateGroup(pp->actor, plActor->user.ActorActionSet->Run);
}


void DoPlayerWade(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    DoPlayerFireOutWater(pp);

    if (DebugOperate)
    {
        if (pp->input.actions & SB_OPEN)
        {
            if (pp->KeyPressBits & SB_OPEN)
            {
                if ((pp->cursector->extra & SECTFX_OPERATIONAL))
                {
                    pp->KeyPressBits &= ~SB_OPEN;
                    DoPlayerBeginOperate(pp);
                    pp->bob_amt = 0;
                    pp->bob_ndx = 0;
                    return;
                }
            }
        }
        else
        {
            pp->KeyPressBits |= SB_OPEN;
        }
    }

    // Crawl if in small area automatically
    if (DoPlayerTestCrawl(pp) && pp->WadeDepth <= PLAYER_CRAWL_WADE_DEPTH)
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    // Crawl Commanded
    if ((pp->input.actions & SB_CROUCH) && pp->WadeDepth <= PLAYER_CRAWL_WADE_DEPTH)
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    if (pp->input.actions & SB_JUMP)
    {
        if (pp->KeyPressBits & SB_JUMP)
        {
            pp->KeyPressBits &= ~SB_JUMP;
            //DoPlayerHeight(pp);
            //DoPlayerHeight(pp);
            //DoPlayerHeight(pp);
            //DoPlayerHeight(pp);
            DoPlayerBeginJump(pp);
            pp->bob_amt = 0;
            pp->bob_ndx = 0;
            return;
        }
    }
    else
    {
        pp->KeyPressBits |= SB_JUMP;
    }

    if (PlayerFlyKey())
    {
        DoPlayerBeginFly(pp);
        pp->bob_amt = 0;
        pp->bob_ndx = 0;
        return;
    }

    // If moving forward and tag is a ladder start climbing
    if (PlayerOnLadder(pp))
    {
        DoPlayerBeginClimb(pp);
        return;
    }

    if (pp->lo_sectp && (pp->lo_sectp->extra & SECTFX_CURRENT))
    {
        DoPlayerCurrent(pp);
    }

    // Move about
    DoPlayerMove(pp);

    if (pp->Flags & (PF_PLAYER_MOVED))
    {
        if (plActor->user.Rot != plActor->user.ActorActionSet->Run)
            NewStateGroup(pp->actor, plActor->user.ActorActionSet->Run);
    }
    else
    {
        if (plActor->user.Rot != plActor->user.ActorActionSet->Stand)
            NewStateGroup(pp->actor, plActor->user.ActorActionSet->Stand);
    }

    // If the floor is far below you, fall hard instead of adjusting height
    if (abs(pp->pos.Z - pp->loz) > PLAYER_HEIGHTF + PLAYER_FALL_HEIGHTF)
    {
        pp->jump_speed = 256;
        DoPlayerBeginFall(pp);
        // call PlayerFall now seems to iron out a hitch before falling
        DoPlayerFall(pp);
        return;
    }

    if (PlayerCanDive(pp))
    {
        pp->bob_amt = 0;
        pp->bob_ndx = 0;
        return;
    }

    // If the floor is far below you, fall hard instead of adjusting height
    if (abs(pp->pos.Z - pp->loz) > PLAYER_HEIGHTF + PLAYER_FALL_HEIGHTF)
    {
        pp->jump_speed = Z(1);
        DoPlayerBeginFall(pp);
        // call PlayerFall now seems to iron out a hitch before falling
        DoPlayerFall(pp);
        pp->bob_amt = 0;
        pp->bob_ndx = 0;
        return;
    }


    DoPlayerBob(pp);

    // Adjust height moving up and down sectors
    DoPlayerHeight(pp);

    if (!pp->WadeDepth)
    {
        DoPlayerBeginRun(pp);
        return;
    }
}


void DoPlayerBeginOperateVehicle(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    pp->p_floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerOperateVehicle;

    // temporary set to get weapons down
    if ((pp->sop->flags & SOBJ_HAS_WEAPON))
        pp->Flags |= (PF_WEAPON_DOWN);

    ///DamageData[plActor->user.WeaponNum].Init(pp);

    ASSERT(plActor->user.ActorActionSet->Stand);

    NewStateGroup(pp->actor, plActor->user.ActorActionSet->Stand);
}

void DoPlayerBeginOperateTurret(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    pp->p_floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerOperateTurret;

    // temporary set to get weapons down
    if ((pp->sop->flags & SOBJ_HAS_WEAPON))
        pp->Flags |= (PF_WEAPON_DOWN);

    ///DamageData[plActor->user.WeaponNum].Init(pp);

    ASSERT(plActor->user.ActorActionSet->Stand);

    NewStateGroup(pp->actor, plActor->user.ActorActionSet->Stand);
}

void FindMainSector(SECTOR_OBJECT* sop)
{
    // find the main sector - only do this once for each sector object
    if (sop->op_main_sector == nullptr)
    {
        auto oldpos = sop->pmid;

        PlaceSectorObject(sop, { MAXSO, MAXSO });

        // set it to something valid
        sop->op_main_sector = &sector[0];

        updatesectorz(oldpos, &sop->op_main_sector);

        PlaceSectorObject(sop, oldpos.XY());
    }
}

void DoPlayerOperateMatch(PLAYER* pp, bool starting)
{
    if (!pp->sop)
        return;

    SWSectIterator it(pp->sop->mid_sector);
    while (auto actor = it.Next())
    {
        if (actor->spr.statnum == STAT_ST1 && actor->spr.hitag == SO_DRIVABLE_ATTRIB)
        {
            if (starting)
            {
                if (SP_TAG5(actor))
                    DoMatchEverything(pp, SP_TAG5(actor), -1);
            }
            else
            {
                if (TEST_BOOL2(actor) && SP_TAG5(actor))
                    DoMatchEverything(pp, SP_TAG5(actor)+1, -1);
            }
            break;
        }
    }
}

void DoPlayerBeginOperate(PLAYER* pp)
{
    SECTOR_OBJECT* sop;
    int cz, fz;
    int i;

    sop = PlayerOnObject(pp->cursector);

    // if someone already controlling it
    if (sop->controller)
        return;

    if ((sop->flags & SOBJ_REMOTE_ONLY))
        return;

    if (!sop)
    {
        DoPlayerBeginRun(pp);
        return;
    }

    // won't operate - broken
    if (sop->max_damage != -9999 && sop->max_damage <= 0)
    {
        if (pp->InventoryAmount[INVENTORY_REPAIR_KIT])
        {
            UseInventoryRepairKit(pp);
            sop->max_damage = sop->sp_child->user.MaxHealth;
            VehicleSetSmoke(sop, nullptr);
            sop->flags &= ~(SOBJ_BROKEN);
        }
        else
        {
            PlayerSound(DIGI_USEBROKENVEHICLE, v3df_follow|v3df_dontpan,pp);
            return;
        }
    }

    pp->sop = pp->sop_control = sop;
    sop->controller = pp->actor;

    pp->angle.oang = pp->angle.ang = DAngle::fromBuild(sop->ang);
    pp->pos.XY() = sop->pmid.XY();
    updatesector(pp->pos, &pp->cursector);
    getzsofslopeptr(pp->cursector, pp->int_ppos().X, pp->int_ppos().Y, &cz, &fz);
    pp->set_int_ppos_Z(fz - PLAYER_HEIGHT);

    pp->Flags &= ~(PF_CRAWLING|PF_JUMPING|PF_FALLING|PF_LOCK_CRAWL);

    DoPlayerOperateMatch(pp, true);

    // look for gun before trying to using it
    for (i = 0; sop->so_actors[i] != nullptr; i++)
    {
        if (sop->so_actors[i]->spr.statnum == STAT_SO_SHOOT_POINT)
        {
            sop->flags |= (SOBJ_HAS_WEAPON);
            break;
        }
    }

    DoPlayerResetMovement(pp);

    switch (sop->track)
    {
    case SO_VEHICLE:
        if (pp->input.fvel|pp->input.svel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
        pp->set_int_ppos_Z(fz - PLAYER_HEIGHT);
        DoPlayerBeginOperateVehicle(pp);
        break;
    case SO_TURRET_MGUN:
    case SO_TURRET:
        if (pp->input.avel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
        pp->set_int_ppos_Z(fz - PLAYER_HEIGHT);
        DoPlayerBeginOperateTurret(pp);
        break;
#if 0
    case SO_SPEED_BOAT:
        if (pp->input.fvel|pp->input.svel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
        pp->posz = fz - PLAYER_HEIGHT;
        DoPlayerBeginOperateBoat(pp);
        break;
#endif
    default:
        return;
    }

}

void DoPlayerBeginRemoteOperate(PLAYER* pp, SECTOR_OBJECT* sop)
{
    int cz, fz;
    int i;

    pp->sop_remote = pp->sop = pp->sop_control = sop;
    sop->controller = pp->actor;

    // won't operate - broken
    if (sop->max_damage != -9999 && sop->max_damage <= 0)
    {
        if (pp->InventoryAmount[INVENTORY_REPAIR_KIT])
        {
            UseInventoryRepairKit(pp);
            sop->max_damage = sop->sp_child->user.MaxHealth;
            VehicleSetSmoke(sop, nullptr);
            sop->flags &= ~(SOBJ_BROKEN);
        }
        else
        {
            PlayerSound(DIGI_USEBROKENVEHICLE, v3df_follow|v3df_dontpan,pp);
            return;
        }
    }

    auto save_sect = pp->cursector;

    pp->angle.oang = pp->angle.ang = DAngle::fromBuild(sop->ang);
    pp->pos.XY() = sop->pmid.XY();
    updatesector(pp->pos, &pp->cursector);
    getzsofslopeptr(pp->cursector, pp->int_ppos().X, pp->int_ppos().Y, &cz, &fz);
    pp->set_int_ppos_Z(fz - PLAYER_HEIGHT);

    pp->Flags &= ~(PF_CRAWLING|PF_JUMPING|PF_FALLING|PF_LOCK_CRAWL);

    DoPlayerOperateMatch(pp, true);

    // look for gun before trying to using it
    for (i = 0; sop->so_actors[i] != nullptr; i++)
    {
        if (sop->so_actors[i]->spr.statnum == STAT_SO_SHOOT_POINT)
        {
            sop->flags |= (SOBJ_HAS_WEAPON);
            break;
        }
    }

    DoPlayerResetMovement(pp);

    PlayerToRemote(pp);
    PlayerRemoteInit(pp);

    switch (sop->track)
    {
    case SO_VEHICLE:
        if (pp->input.fvel|pp->input.svel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
        pp->set_int_ppos_Z(fz - PLAYER_HEIGHT);
        DoPlayerBeginOperateVehicle(pp);
        break;
    case SO_TURRET_MGUN:
    case SO_TURRET:
        if (pp->input.avel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
        pp->set_int_ppos_Z(fz - PLAYER_HEIGHT);
        DoPlayerBeginOperateTurret(pp);
        break;
    default:
        return;
    }

    PlayerRemoteReset(pp, save_sect);
}

void PlayerToRemote(PLAYER* pp)
{
    pp->remote.cursectp = pp->cursector;
    pp->remote.lastcursectp = pp->lastcursector;

    pp->remote.pos = pp->pos;

    pp->remote.vect.X = pp->vect.X;
    pp->remote.vect.Y = pp->vect.Y;
    pp->remote.ovect.Y = pp->ovect.X;
    pp->remote.ovect.Y = pp->ovect.Y;
    pp->remote.slide_vect.X = pp->slide_vect.X;
    pp->remote.slide_vect.Y = pp->slide_vect.Y;
}

void RemoteToPlayer(PLAYER* pp)
{
    pp->setcursector(pp->remote.cursectp);
    pp->lastcursector = pp->remote.lastcursectp;

    pp->pos = pp->remote.pos;

    pp->vect.X = pp->remote.vect.X;
    pp->vect.Y = pp->remote.vect.Y;
    pp->ovect.X = pp->remote.ovect.Y;
    pp->ovect.Y = pp->remote.ovect.Y;
    pp->slide_vect.X = pp->remote.slide_vect.X;
    pp->slide_vect.Y = pp->remote.slide_vect.Y;
}

void PlayerRemoteReset(PLAYER* pp, sectortype* sect)
{
    pp->setcursector(sect);
    pp->lastcursector = pp->cursector;

    auto rsp = pp->remoteActor;
    pp->pos.XY() = rsp->spr.pos.XY();
    pp->pos.Z = sect->floorz - PLAYER_HEIGHTF;

    pp->vect.X = pp->vect.Y = pp->ovect.X = pp->ovect.Y = pp->slide_vect.X = pp->slide_vect.Y = 0;

    UpdatePlayerSprite(pp);
}

void PlayerRemoteInit(PLAYER* pp)
{
    pp->remote.vect.X        = 0;
    pp->remote.vect.Y        = 0;
    pp->remote.ovect.Y       = 0;
    pp->remote.ovect.Y       = 0;
    pp->remote.slide_vect.X  = 0;
    pp->remote.slide_vect.Y  = 0;
}

void DoPlayerStopOperate(PLAYER* pp)
{
    pp->Flags &= ~(PF_WEAPON_DOWN);
    DoPlayerResetMovement(pp);
    DoTankTreads(pp);
    DoPlayerOperateMatch(pp, false);
    StopSOsound(pp->sop->mid_sector);

    if (pp->sop_remote)
    {
        DSWActor* rsp = pp->remoteActor;
        if (TEST_BOOL1(rsp))
            pp->angle.ang = pp->angle.oang = rsp->spr.angle;
        else
            pp->angle.ang = pp->angle.oang = VecToAngle(pp->sop_remote->int_pmid().X - pp->int_ppos().X, pp->sop_remote->int_pmid().Y - pp->int_ppos().Y);
    }

    if (pp->sop_control)
    {
        pp->sop_control->controller = nullptr;
    }
    pp->sop_control = nullptr;
    pp->sop_riding = nullptr;
    pp->sop_remote = nullptr;
    pp->sop = nullptr;
    DoPlayerBeginRun(pp);
}

void DoPlayerOperateTurret(PLAYER* pp)
{
    if (pp->input.actions & SB_OPEN)
    {
        if (pp->KeyPressBits & SB_OPEN)
        {
            pp->KeyPressBits &= ~SB_OPEN;
            DoPlayerStopOperate(pp);
            return;
        }
    }
    else
    {
        pp->KeyPressBits |= SB_OPEN;
    }

    if (pp->sop->max_damage != -9999 && pp->sop->max_damage <= 0)
    {
        DoPlayerStopOperate(pp);
        return;
    }

    auto save_sect = pp->cursector;

    if (pp->sop_remote)
        RemoteToPlayer(pp);

    DoPlayerMoveTurret(pp);

    if (pp->sop_remote)
    {
        PlayerToRemote(pp);
        PlayerRemoteReset(pp, save_sect);
    }
}



void DoPlayerOperateVehicle(PLAYER* pp)
{
    if (pp->input.actions & SB_OPEN)
    {
        if (pp->KeyPressBits & SB_OPEN)
        {
            pp->KeyPressBits &= ~SB_OPEN;
            DoPlayerStopOperate(pp);
            return;
        }
    }
    else
    {
        pp->KeyPressBits |= SB_OPEN;
    }

    if (pp->sop->max_damage != -9999 && pp->sop->max_damage <= 0)
    {
        DoPlayerStopOperate(pp);
        return;
    }

    auto save_sect = pp->cursector;

    if (pp->sop_remote)
        RemoteToPlayer(pp);

    DoPlayerMoveVehicle(pp);

    if (pp->sop_remote)
    {
        PlayerToRemote(pp);
        PlayerRemoteReset(pp, save_sect);
    }
}

void DoPlayerDeathJump(PLAYER* pp)
{
    short i;

#define PLAYER_DEATH_GRAV 8

    // instead of multiplying by synctics, use a loop for greater accuracy
    for (i = 0; i < synctics; i++)
    {
        // adjust jump speed by gravity - if jump speed greater than 0 player
        // have started falling
        if ((pp->jump_speed += PLAYER_DEATH_GRAV) > 0)
        {
            pp->Flags &= ~(PF_JUMPING);
            pp->Flags |= (PF_FALLING);
            DoPlayerDeathFall(pp);
            return;
        }

        // adjust height by jump speed
        pp->pos.Z += pp->jump_speed * JUMP_FACTOR;

        // if player gets to close the ceiling while jumping
        //if (pp->posz < pp->hiz + Z(4))
        if (PlayerCeilingHit(pp, pp->hiz + 4))
        {
            // put player at the ceiling
            pp->pos.Z = pp->hiz + 4;

            // reverse your speed to falling
            pp->jump_speed = -pp->jump_speed;

            // start falling
            pp->Flags &= ~(PF_JUMPING);
            pp->Flags |= (PF_FALLING);
            DoPlayerDeathFall(pp);
            return;
        }
    }
}

void DoPlayerDeathFall(PLAYER* pp)
{
    double loz;

    for (int i = 0; i < synctics; i++)
    {
        // adjust jump speed by gravity
        pp->jump_speed += PLAYER_DEATH_GRAV;

        // adjust player height by jump speed
        pp->pos.Z += pp->jump_speed * JUMP_FACTOR;

        if (pp->lo_sectp && (pp->lo_sectp->extra & SECTFX_SINK))
        {
            loz = pp->lo_sectp->floorz;
        }
        else
            loz = pp->loz;

        if (PlayerFloorHit(pp, loz - PLAYER_DEATH_HEIGHTF))
        {
            if (loz != pp->loz)
                SpawnSplash(pp->actor);

            if (RandomRange(1000) > 500)
                PlaySound(DIGI_BODYFALL1, pp, v3df_dontpan);
            else
                PlaySound(DIGI_BODYFALL2, pp, v3df_dontpan);

            pp->pos.Z = loz - PLAYER_DEATH_HEIGHTF;
            pp->Flags &= ~(PF_FALLING);
        }
    }
}

#define MAX_SUICIDE 11
const char *SuicideNote[MAX_SUICIDE] =
{
    "decided to do the graveyard tour.",
    "had enough and checked out.",
    "didn't fear the Reaper.",
    "dialed the 1-800-CYANIDE line.",
    "wasted himself.",
    "kicked his own ass.",
    "went out in blaze of his own glory.",
    "killed himself before anyone else could.",
    "needs shooting lessons.",
    "blew his head off.",
    "did everyone a favor and offed himself."
};

char *KilledPlayerMessage(PLAYER* pp, PLAYER* killer)
{
    const int MAX_KILL_NOTES = 16;
    short rnd = StdRandomRange(MAX_KILL_NOTES);
    const char *p1 = pp->PlayerName;
    const char *p2 = killer->PlayerName;

    if (pp->HitBy == killer->actor)
    {
        sprintf(ds,"%s was killed by %s.",p1,p2);
        return ds;
    }
    else
        switch (rnd)
        {
        case 0:
            sprintf(ds,"%s was wasted by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 1:
            sprintf(ds,"%s got his ass kicked by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 2:
            sprintf(ds,"%s bows down before the mighty power of %s.",p1,p2);
            return ds;
        case 3:
            sprintf(ds,"%s was killed by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 4:
            sprintf(ds,"%s got slapped down hard by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 5:
            sprintf(ds,"%s got on his knees before %s.",p1,p2);
            return ds;
        case 6:
            sprintf(ds,"%s was totally out classed by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 7:
            sprintf(ds,"%s got chewed apart by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 8:
            sprintf(ds,"%s was retired by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 9:
            sprintf(ds,"%s was greased by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 10:
            sprintf(ds,"%s was humbled lower than dirt by %s.",p1,p2);
            return ds;
        case 11:
            sprintf(ds,"%s beats %s like a red headed step child.",p2,p1);
            return ds;
        case 12:
            sprintf(ds,"%s begs for mercy as %s terminates him with extreme prejudice.",p1,p2);
            return ds;
        case 13:
            sprintf(ds,"%s falls before the superior skills of %s.",p1,p2);
            return ds;
        case 14:
            sprintf(ds,"%s gives %s a beating he'll never forget.",p2,p1);
            return ds;
        case 15:
            sprintf(ds,"%s puts the Smack Dab on %s with his %s.",p2,p1,DeathString(pp->HitBy));
            return ds;
        }
    return nullptr;
};

void DoPlayerDeathMessage(PLAYER* pp, PLAYER* killer)
{
    int pnum;
    bool SEND_OK = false;

    killer->KilledPlayer[pp-Player]++;

    if (pp == killer && pp == Player + myconnectindex)
    {
        sprintf(ds,"%s %s",pp->PlayerName,SuicideNote[StdRandomRange(MAX_SUICIDE)]);
        SEND_OK = true;
    }
    else
    // I am being killed
    if (killer == Player + myconnectindex)
    {
        sprintf(ds,"%s",KilledPlayerMessage(pp,killer));
        SEND_OK = true;
    }

    if (SEND_OK)
    {
        TRAVERSE_CONNECT(pnum)
        {
            if (pnum == myconnectindex)
                Printf(PRINT_NOTIFY|PRINT_TEAMCHAT, "%s\n", ds);
            else
                SW_SendMessage(pnum, ds);
        }
    }

}

enum
{
    PLAYER_DEATH_HORIZ_UP_VALUE = 65,
    PLAYER_DEATH_HORIZ_JUMP_VALUE = 50,
    PLAYER_DEATH_HORIZ_FALL_VALUE = -50
};

void DoPlayerBeginDie(PLAYER* pp)
{
    extern bool ReloadPrompt;
    short bak;
    int choosesnd = 0;

    DSWActor* plActor = pp->actor;

    static void (*PlayerDeathFunc[MAX_PLAYER_DEATHS]) (PLAYER*) =
    {
        DoPlayerDeathFlip,
        DoPlayerDeathCrumble,
        DoPlayerDeathExplode,
        DoPlayerDeathFlip,
        DoPlayerDeathExplode,
        DoPlayerDeathDrown,
    };

    if (Prediction)
        return;

    if (GodMode)
        return;

    StopPlayerSound(pp);

    // Do the death scream
    choosesnd = RandomRange(MAX_PAIN);

    PlayerSound(PlayerLowHealthPainVocs[choosesnd],v3df_dontpan|v3df_doppler|v3df_follow,pp);

    PutStringInfo(pp, GStrings("TXTS_PRESSSPACE"));

    if (pp->sop_control)
        DoPlayerStopOperate(pp);

    // if diving force death to drown type
    if (pp->Flags & (PF_DIVING))
        pp->DeathType = PLAYER_DEATH_DROWN;

    pp->Flags &= ~(PF_JUMPING|PF_FALLING|PF_DIVING|PF_FLYING|PF_CLIMBING|PF_CRAWLING|PF_LOCK_CRAWL);

    pp->tilt_dest = 0;

    ActorCoughItem(pp->actor);

    if (numplayers > 1)
    {
        // Give kill credit to player if necessary
        DSWActor* killer = pp->KillerActor;
        if (killer != nullptr)
        {
            ASSERT(killer->hasU());

            if (killer->hasU() && killer->user.PlayerP)
            {
                if (pp == killer->user.PlayerP)
                {
                    // Killed yourself
                    PlayerUpdateKills(pp, -1);
                    DoPlayerDeathMessage(pp, pp);
                }
                else
                {
                    // someone else killed you
                    if (gNet.TeamPlay)
                    {
                        // playing team play
                        if (pp->actor->user.spal == killer->user.spal)
                        {
                            // Killed your team member
                            PlayerUpdateKills(pp, -1);
                            DoPlayerDeathMessage(pp, killer->user.PlayerP);
                        }
                        else
                        {
                            // killed another team member
                            PlayerUpdateKills(killer->user.PlayerP, 1);
                            DoPlayerDeathMessage(pp, killer->user.PlayerP);
                        }
                    }
                    else
                    {
                        // not playing team play
                        PlayerUpdateKills(killer->user.PlayerP, 1);
                        DoPlayerDeathMessage(pp, killer->user.PlayerP);
                    }
                }
            }
        }
        else
        {
            // Killed by some hazard - negative frag
            PlayerUpdateKills(pp, -1);
            DoPlayerDeathMessage(pp, pp);
        }
    }


    // Get rid of all panel spells that are currently working
    KillAllPanelInv(pp);

    pp->input.actions &= ~SB_CENTERVIEW;

    pp->friction = PLAYER_RUN_FRICTION;
    pp->slide_vect.X = pp->slide_vect.Y = 0;
    pp->p_floor_dist = PLAYER_WADE_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_WADE_CEILING_DIST;
    ASSERT(pp->DeathType < SIZ(PlayerDeathFunc));
    pp->DoPlayerAction = PlayerDeathFunc[pp->DeathType];
    pp->sop_control = nullptr;
    pp->sop_remote = nullptr;
    pp->sop_riding = nullptr;
    pp->sop = nullptr;
    pp->Flags &= ~(PF_TWO_UZI);

    NewStateGroup(pp->actor, plActor->user.ActorActionSet->Run);
    pWeaponForceRest(pp);

    switch (pp->DeathType)
    {
    case PLAYER_DEATH_DROWN:
    {
        pp->Flags |= (PF_JUMPING);
        plActor->user.ID = NINJA_DEAD;
        pp->jump_speed = -200;
        NewStateGroup(pp->actor, sg_PlayerDeath);
        DoFindGround(pp->actor);
        DoBeginJump(pp->actor);
        plActor->user.jump_speed = -300;
        break;
    }
    case PLAYER_DEATH_FLIP:
    case PLAYER_DEATH_RIPPER:

        //PlaySound(DIGI_SCREAM1, pp, v3df_dontpan|v3df_follow);

        pp->Flags |= (PF_JUMPING);
        plActor->user.ID = NINJA_DEAD;
        pp->jump_speed = -300;
        NewStateGroup(pp->actor, sg_PlayerDeath);
        //pp->ceiling_dist = Z(0);
        //pp->floor_dist = Z(0);

        plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        plActor->user.ceiling_dist = (10);
        plActor->user.floor_dist = (0);
        DoFindGround(pp->actor);
        DoBeginJump(pp->actor);
        plActor->user.jump_speed = -400;
        break;
    case PLAYER_DEATH_CRUMBLE:

        PlaySound(DIGI_BODYSQUISH1, pp, v3df_dontpan);

        pp->Flags |= (PF_DEAD_HEAD | PF_JUMPING);
        pp->jump_speed = -300;
        plActor->user.slide_vel = 0;
        SpawnShrap(pp->actor, nullptr);
        plActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        NewStateGroup(pp->actor, sg_PlayerHeadFly);
        plActor->user.ID = NINJA_Head_R0;
        plActor->spr.xrepeat = 48;
        plActor->spr.yrepeat = 48;
        // Blood fountains
        InitBloodSpray(pp->actor,true,105);
        break;
    case PLAYER_DEATH_EXPLODE:

        PlaySound(DIGI_BODYSQUISH1, pp, v3df_dontpan);

        pp->Flags |= (PF_DEAD_HEAD | PF_JUMPING);
        pp->jump_speed = -650;
        SpawnShrap(pp->actor, nullptr);
        plActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        NewStateGroup(pp->actor, sg_PlayerHeadFly);
        plActor->user.ID = NINJA_Head_R0;
        plActor->spr.xrepeat = 48;
        plActor->spr.yrepeat = 48;
        // Blood fountains
        InitBloodSpray(pp->actor,true,-1);
        InitBloodSpray(pp->actor,true,-1);
        InitBloodSpray(pp->actor,true,-1);
        break;
    case PLAYER_DEATH_SQUISH:

        PlaySound(DIGI_BODYCRUSHED1, pp, v3df_dontpan);

        pp->Flags |= (PF_DEAD_HEAD | PF_JUMPING);
        pp->jump_speed = 200;
        plActor->user.slide_vel = 800;
        SpawnShrap(pp->actor, nullptr);
        plActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        NewStateGroup(pp->actor, sg_PlayerHeadFly);
        plActor->user.ID = NINJA_Head_R0;
        plActor->spr.xrepeat = 48;
        plActor->spr.yrepeat = 48;
        // Blood fountains
        InitBloodSpray(pp->actor,true,105);
        break;

    }

    pp->Flags |= (PF_DEAD);
    plActor->user.Flags &= ~(SPR_BOUNCE);
    pp->Flags &= ~(PF_HEAD_CONTROL);
}

void DoPlayerDeathHoriz(PLAYER* pp, short target, short speed)
{
    if ((pp->horizon.horiz.asbuild() - target) > 1)
    {   
        pp->horizon.addadjustment(buildhoriz(-speed));
    }

    if ((target - pp->horizon.horiz.asbuild()) > 1)
    {
        pp->horizon.addadjustment(buildhoriz(speed));
    }
}

int DoPlayerDeathTilt(PLAYER* pp, short target, short speed)
{
    if (pp->tilt > target)
    {
        pp->tilt -= speed;
        if (pp->tilt <= target)
            pp->tilt = target;
    }

    if (pp->tilt < target)
    {
        pp->tilt += speed;
        if (pp->tilt >= target)
            pp->tilt = target;
    }

    return pp->tilt == target;
}


void DoPlayerDeathZrange(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    // make sure we don't land on a regular sprite
    DoFindGround(pp->actor);

    // update player values with results from DoFindGround
    pp->loz = plActor->user.loz;
    pp->lowActor = plActor->user.lowActor;
    pp->lo_sectp = plActor->user.lo_sectp;
}

void DoPlayerDeathHurl(PLAYER* pp)
{
    if (numplayers > 1)
    {
        if (pp->input.actions & SB_FIRE)
        {
            if (pp->KeyPressBits & SB_FIRE)
            {


                pp->Flags |= (PF_HEAD_CONTROL);
                NewStateGroup(pp->actor, sg_PlayerHeadHurl);
                if (MoveSkip4 == 0)
                {
                    SpawnShrap(pp->actor, nullptr);
                    if (RandomRange(1000) > 400)
                        PlayerSound(DIGI_DHVOMIT, v3df_dontpan|v3df_follow,pp);
                }
                return;
            }
        }
    }

    if (!(pp->Flags & (PF_JUMPING|PF_FALLING)))
        NewStateGroup(pp->actor, sg_PlayerHead);
}


void DoPlayerDeathFollowKiller(PLAYER* pp)
{
    // if it didn't make it to this angle because of a low ceiling or something
    // continue on to it
    DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUE, 4);
    //DoPlayerDeathTilt(pp, pp->tilt_dest, 4 * synctics);

    // allow turning
    if (pp->Flags & (PF_DEAD_HEAD|PF_HEAD_CONTROL))
    {  
        if (!SyncInput())
        {
            pp->Flags2 |= (PF2_INPUT_CAN_TURN_GENERAL);
        }
        else
        {
            DoPlayerTurn(pp, pp->input.avel, 1);
        }
    }

    // follow what killed you if its available
    DSWActor* killer = pp->KillerActor;
    if (killer)
    {
        if (FAFcansee(ActorVectOfTop(killer), killer->sector(), pp->pos, pp->cursector))
        {
            pp->angle.addadjustment(deltaangle(pp->angle.ang, VecToAngle(killer->int_pos().X - pp->int_ppos().X, killer->int_pos().Y - pp->int_ppos().Y)) * (1. / 16.));
        }
    }
}

void DoPlayerDeathCheckKeys(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    if (pp->input.actions & SB_OPEN)
    {
        // Spawn a dead LoWang body for non-head deaths
        // Hey Frank, if you think of a better check, go ahead and put it in.
        if (PlayerFloorHit(pp, pp->loz - PLAYER_HEIGHTF))
        {
            if (pp->DeathType == PLAYER_DEATH_FLIP || pp->DeathType == PLAYER_DEATH_RIPPER)
                QueueLoWangs(pp->actor);
        }
        else
        {
            // If he's not on the floor, then gib like a mo-fo!
            InitBloodSpray(plActor,true,-1);
            InitBloodSpray(plActor,true,-1);
            InitBloodSpray(plActor,true,-1);
        }

        PlayerSpawnPosition(pp);

        NewStateGroup(plActor, plActor->user.ActorActionSet->Stand);
        plActor->spr.picnum = plActor->user.State->Pic;
        plActor->spr.picnum = plActor->user.State->Pic;
        plActor->spr.xrepeat = plActor->spr.yrepeat = PLAYER_NINJA_XREPEAT;
        plActor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
        plActor->spr.pos = pp->pos.plusZ(PLAYER_HEIGHTF);
        plActor->set_int_ang(pp->angle.ang.Buildang());

        DoSpawnTeleporterEffect(plActor);
        PlaySound(DIGI_TELEPORT, pp, v3df_none);

        DoPlayerZrange(pp);

        pp->sop_control = nullptr;
        pp->sop_remote = nullptr;
        pp->sop_riding = nullptr;
        pp->sop = nullptr;

        pp->Flags &= ~(PF_WEAPON_DOWN|PF_WEAPON_RETRACT);
        pp->Flags &= ~(PF_DEAD);
        plActor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
        plActor->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        pp->input.actions |= SB_CENTERVIEW;
        plActor->spr.xrepeat = PLAYER_NINJA_XREPEAT;
        plActor->spr.yrepeat = PLAYER_NINJA_YREPEAT;

        //pp->tilt = 0;
        pp->horizon.horiz = q16horiz(0);
        DoPlayerResetMovement(pp);
        plActor->user.ID = NINJA_RUN_R0;
        PlayerDeathReset(pp);

        if (pp == Player + screenpeek)
        {
            videoFadePalette(0,0,0,0);
        }

        pp->NightVision = false;
        pp->FadeAmt = 0;
        DoPlayerDivePalette(pp);
        DoPlayerNightVisionPalette(pp);

        if (numplayers > 1)
        {
            // need to call this routine BEFORE resetting DEATH flag
            DoPlayerBeginRun(pp);
        }
        else
        {
            gameaction = ga_autoloadgame;
        }

        DoPlayerFireOutDeath(pp);
    }
}

void DoPlayerHeadDebris(PLAYER* pp)
{
    sectortype* sectp = pp->cursector;

    if ((sectp->extra & SECTFX_SINK))
    {
        DoPlayerSpriteBob(pp, 8, 4, 3);
    }
    else
    {
        pp->bob_amt = 0;
    }
}

void DoPlayerDeathCheckKick(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    unsigned stat;
    int dist;
    int a,b,c;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == pp->actor)
                break;

            // don't set off mine
            if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                continue;

            DISTANCE(itActor->spr.pos, plActor->spr.pos, dist, a, b, c);

            if (unsigned(dist) < itActor->user.Radius + 100)
            {
                pp->KillerActor = itActor;

                plActor->user.slide_ang = getangle(plActor->int_pos().X - itActor->int_pos().X, plActor->int_pos().Y - itActor->int_pos().Y);
                plActor->user.slide_ang = NORM_ANGLE(plActor->user.slide_ang + (RANDOM_P2(128<<5)>>5) - 64);

                plActor->user.slide_vel = itActor->spr.xvel<<1;
                plActor->user.Flags &= ~(SPR_BOUNCE);
                pp->jump_speed = -500;
                NewStateGroup(pp->actor, sg_PlayerHeadFly);
                pp->Flags |= (PF_JUMPING);
                SpawnShrap(pp->actor, nullptr);
            }
        }
    }

    DoPlayerZrange(pp);

    // sector stomper kick
    if (abs(pp->loz - pp->hiz) < ActorSizeZ(plActor) - 8)
    {
        plActor->user.slide_ang = RANDOM_P2(2048);
        plActor->user.slide_vel = 1000;
        plActor->user.Flags &= ~(SPR_BOUNCE);
        pp->jump_speed = -100;
        NewStateGroup(pp->actor, sg_PlayerHeadFly);
        pp->Flags |= (PF_JUMPING);
        SpawnShrap(pp->actor, nullptr);
    }
}


void DoPlayerDeathMoveHead(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    int dax,day;

    dax = MOVEx(plActor->user.slide_vel, plActor->user.slide_ang);
    day = MOVEy(plActor->user.slide_vel, plActor->user.slide_ang);

    plActor->user.coll = move_sprite(pp->actor, dax, day, 0, Z(16), Z(16), 1, synctics);
    {
        switch (plActor->user.coll.type)
        {
        case kHitSprite:
        {
            short wall_ang, dang;

            auto hitActor = plActor->user.coll.actor();

            if (!(hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
                break;


            wall_ang = NORM_ANGLE(hitActor->int_ang());
            dang = getincangle(wall_ang, plActor->user.slide_ang);
            plActor->user.slide_ang = NORM_ANGLE(wall_ang + 1024 - dang);

            SpawnShrap(pp->actor, nullptr);
            break;
        }
        case kHitWall:
        {
            int wall_ang = NORM_ANGLE(getangle(plActor->user.coll.hitWall->delta())-512);

            int dang = getincangle(wall_ang, plActor->user.slide_ang);
            plActor->user.slide_ang = NORM_ANGLE(wall_ang + 1024 - dang);

            SpawnShrap(pp->actor, nullptr);
            break;
        }
        }
    }

    pp->pos.XY() = plActor->spr.pos.XY();
    pp->setcursector(plActor->sector());

    // try to stay in valid area - death sometimes throws you out of the map
    auto sect = pp->cursector;
    updatesector(pp->int_ppos().X, pp->int_ppos().Y, &sect);
    if (sect == nullptr)
    {
        pp->cursector = pp->lv_sector;
        ChangeActorSect(pp->actor, pp->lv_sector);
        pp->set_int_ppos_XY(pp->lv.XY());
        plActor->set_int_xy(pp->int_ppos().X, pp->int_ppos().Y);
    }
    else
    {
        pp->lv_sector = sect;
        pp->lv.XY() = pp->int_ppos().XY();
    }
}

void DoPlayerDeathFlip(PLAYER* pp)
{
    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if ((pp->Flags & (PF_JUMPING|PF_FALLING)))
    {
        if ((pp->Flags & PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUE, 2);
            if (MoveSkip2 == 0)
                DoJump(pp->actor);
        }

        if ((pp->Flags & PF_FALLING))
        {
            DoPlayerDeathFall(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUE, 4);
            if (MoveSkip2 == 0)
                DoFall(pp->actor);
        }
    }
    else
    {
        DoPlayerDeathFollowKiller(pp);
    }

    DoPlayerDeathCheckKeys(pp);
}



void DoPlayerDeathDrown(PLAYER* pp)
{
    DSWActor* actor = pp->actor;

    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if ((pp->Flags & (PF_JUMPING|PF_FALLING)))
    {
        if ((pp->Flags & PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUE, 2);
            if (MoveSkip2 == 0)
                DoJump(pp->actor);
        }

        if ((pp->Flags & PF_FALLING))
        {
            pp->pos.Z += 2;
            if (MoveSkip2 == 0)
                actor->spr.pos.Z += 4;

            // Stick like glue when you hit the ground
            if (pp->pos.Z > pp->loz - PLAYER_DEATH_HEIGHTF)
            {
                pp->pos.Z = pp->loz - PLAYER_DEATH_HEIGHTF;
                pp->Flags &= ~(PF_FALLING);
            }
        }
    }

    DoPlayerDeathFollowKiller(pp);
    DoPlayerDeathCheckKeys(pp);
}


void DoPlayerDeathBounce(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    if (Prediction)
        return;

    if (pp->lo_sectp && (pp->lo_sectp->extra & SECTFX_SINK))
    {
        plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        NewStateGroup(pp->actor, sg_PlayerHead);
        plActor->user.slide_vel = 0;
        plActor->user.Flags |= (SPR_BOUNCE);


        return;
    }

    plActor->user.Flags |= (SPR_BOUNCE);
    pp->jump_speed = -300;
    plActor->user.slide_vel >>= 2;
    plActor->user.slide_ang = NORM_ANGLE((RANDOM_P2(64<<8)>>8) - 32);
    pp->Flags |= (PF_JUMPING);
    SpawnShrap(pp->actor, nullptr);
}




void DoPlayerDeathCrumble(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if ((pp->Flags & (PF_JUMPING|PF_FALLING)))
    {
        if ((pp->Flags & PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_JUMP_VALUE, 4);
        }

        if ((pp->Flags & PF_FALLING))
        {
            DoPlayerDeathFall(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_FALL_VALUE, 3);
        }

        if (!(pp->Flags & (PF_JUMPING|PF_FALLING)))
        {
            if (!(plActor->user.Flags & SPR_BOUNCE))
            {
                DoPlayerDeathBounce(pp);
                return;
            }

            plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
            NewStateGroup(pp->actor, sg_PlayerHead);
        }
        else
        {
            DoPlayerDeathMoveHead(pp);
        }
    }
    else
    {
        DoPlayerDeathCheckKick(pp);
        DoPlayerDeathHurl(pp);
        DoPlayerDeathFollowKiller(pp);
        //pp->posz = pp->loz - PLAYER_DEATH_HEIGHT;
    }

    DoPlayerDeathCheckKeys(pp);
    plActor->spr.pos.Z = pp->pos.Z + PLAYER_DEAD_HEAD_FLOORZ_OFFSET;
    DoPlayerHeadDebris(pp);
}

void DoPlayerDeathExplode(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if ((pp->Flags & (PF_JUMPING|PF_FALLING)))
    {
        if ((pp->Flags & PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_JUMP_VALUE, 4);
        }

        if ((pp->Flags & PF_FALLING))
        {
            DoPlayerDeathFall(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_JUMP_VALUE, 3);
        }

        if (!(pp->Flags & (PF_JUMPING|PF_FALLING)))
        {
            if (!(plActor->user.Flags & SPR_BOUNCE))
            {
                DoPlayerDeathBounce(pp);
                return;
            }

            plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
            NewStateGroup(pp->actor, sg_PlayerHead);
        }
        else
        {
            DoPlayerDeathMoveHead(pp);
        }
    }
    else
    {
        // special line for amoeba

        DoPlayerDeathCheckKick(pp);
        DoPlayerDeathHurl(pp);
        DoPlayerDeathFollowKiller(pp);
    }

    DoPlayerDeathCheckKeys(pp);
    plActor->spr.pos.Z = pp->pos.Z + PLAYER_DEAD_HEAD_FLOORZ_OFFSET;
    DoPlayerHeadDebris(pp);
}

void DoPlayerBeginRun(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    // Crawl if in small aread automatically
    if (DoPlayerTestCrawl(pp))
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    pp->Flags &= ~(PF_CRAWLING|PF_JUMPING|PF_FALLING|PF_LOCK_CRAWL|PF_CLIMBING);

    if (pp->WadeDepth)
    {
        DoPlayerBeginWade(pp);
        return;
    }

    pp->friction = PLAYER_RUN_FRICTION;
    pp->p_floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerRun;

    ///DamageData[plActor->user.WeaponNum].Init(pp);

    ASSERT(plActor->user.ActorActionSet->Run);

    if (pp->Flags & (PF_PLAYER_MOVED))
        NewStateGroup(pp->actor, plActor->user.ActorActionSet->Run);
    else
        NewStateGroup(pp->actor, plActor->user.ActorActionSet->Stand);
}

void DoPlayerRun(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    if (SectorIsUnderwaterArea(pp->cursector))
    {
        DoPlayerBeginDiveNoWarp(pp);
        return;
    }

    // Crawl if in small aread automatically
    if (DoPlayerTestCrawl(pp))
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    // Crawl Commanded
    if (pp->input.actions & SB_CROUCH)
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    // Jump
    if (pp->input.actions & SB_JUMP)
    {
        if (pp->KeyPressBits & SB_JUMP)
        {
            pp->KeyPressBits &= ~SB_JUMP;
            // make sure you stand at full heights for jumps/double jumps
            //DoPlayerHeight(pp);
            //DoPlayerHeight(pp);
            //DoPlayerHeight(pp);
            //DoPlayerHeight(pp);
            //DoPlayerHeight(pp);
            pp->pos.Z = pp->loz - PLAYER_HEIGHTF;
            DoPlayerBeginJump(pp);
            return;
        }
    }
    else
    {
        pp->KeyPressBits |= SB_JUMP;
    }

    if (PlayerFlyKey())
    {
        DoPlayerBeginFly(pp);
        return;
    }

    if (DebugOperate)
    {
        if (!(pp->Flags & PF_DEAD) && !Prediction)
        {
            if (pp->input.actions & SB_OPEN)
            {
                if ((pp->KeyPressBits & SB_OPEN) && pp->insector())
                {
                    if ((pp->cursector->extra & SECTFX_OPERATIONAL))
                    {
                        pp->KeyPressBits &= ~SB_OPEN;
                        DoPlayerBeginOperate(pp);
                        return;
                    }
                    else if ((pp->cursector->extra & SECTFX_TRIGGER))
                    {
                        auto sActor = FindNearSprite(pp->actor, STAT_TRIGGER);
                        if (sActor && SP_TAG5(sActor) == TRIGGER_TYPE_REMOTE_SO)
                        {
                            pp->remoteActor = sActor;
                            pp->KeyPressBits &= ~SB_OPEN;
                            DoPlayerBeginRemoteOperate(pp, &SectorObject[SP_TAG7(sActor)]);
                            return;
                        }
                    }
                }
            }
            else
            {
                pp->KeyPressBits |= SB_OPEN;
            }
        }
    }

    DoPlayerBob(pp);

    if (pp->WadeDepth)
    {
        DoPlayerBeginWade(pp);
        return;
    }


    // If moving forward and tag is a ladder start climbing
    if (PlayerOnLadder(pp))
    {
        DoPlayerBeginClimb(pp);
        return;
    }

    // Move about
    DoPlayerMove(pp);

    if (plActor->user.Rot != sg_PlayerNinjaSword && plActor->user.Rot != sg_PlayerNinjaPunch)
    {
        if (pp->Flags & (PF_PLAYER_MOVED))
        {
            if (plActor->user.Rot != plActor->user.ActorActionSet->Run)
                NewStateGroup(pp->actor, plActor->user.ActorActionSet->Run);
        }
        else
        {
            if (plActor->user.Rot != plActor->user.ActorActionSet->Stand)
                NewStateGroup(pp->actor, plActor->user.ActorActionSet->Stand);
        }
    }

    // If the floor is far below you, fall hard instead of adjusting height
    if (PlayerFallTest(pp, PLAYER_HEIGHTF))
    {
        pp->jump_speed = Z(1);
        DoPlayerBeginFall(pp);
        // call PlayerFall now seems to iron out a hitch before falling
        DoPlayerFall(pp);
        return;
    }

    if ((pp->cursector->extra & SECTFX_DYNAMIC_AREA))
    {
        pp->pos.Z = pp->loz - PLAYER_HEIGHTF;
    }

    // Adjust height moving up and down sectors
    DoPlayerHeight(pp);
}


void PlayerStateControl(DSWActor* actor)
{
    if (actor == nullptr || !actor->hasU()) return;

    actor->user.Tics += synctics;

    // Skip states if too much time has passed
    while (actor->user.Tics >= (actor->user.State->Tics & SF_TICS_MASK))
    {

        // Set Tics
        actor->user.Tics -= (actor->user.State->Tics & SF_TICS_MASK);

        // Transition to the next state
        actor->user.State = actor->user.State->NextState;

        // !JIM! Added this so I can do quick calls in player states!
        // Need this in order for floor blood and footprints to not get called more than once.
        while ((actor->user.State->Tics & SF_QUICK_CALL))
        {
            // Call it once and go to the next state
            (*actor->user.State->Animator)(actor);

            // if still on the same QUICK_CALL should you
            // go to the next state.
            if ((actor->user.State->Tics & SF_QUICK_CALL))
                actor->user.State = actor->user.State->NextState;
        }

        if (!actor->user.State->Pic)
        {
            NewStateGroup(actor, (STATE* *) actor->user.State->NextState);
        }
    }

    // Set picnum to the correct pic
    if (actor->user.RotNum > 1)
        actor->spr.picnum = actor->user.Rot[0]->Pic;
    else
        actor->spr.picnum = actor->user.State->Pic;

    // Call the correct animator
    if ((actor->user.State->Tics & SF_PLAYER_FUNC))
        if (actor->user.State->Animator)
            (*actor->user.State->Animator)(actor);

    return;
}

void MoveSkipSavePos(void)
{
    int i;
    short pnum;
    PLAYER* pp;

    MoveSkip8 = (MoveSkip8 + 1) & 7;
    MoveSkip4 = (MoveSkip4 + 1) & 3;
    MoveSkip2 ^= 1;

    // Save off player
    TRAVERSE_CONNECT(pnum)
    {
        pp = Player + pnum;

        pp->opos = pp->pos;
        pp->obob_z = pp->bob_z;
        pp->angle.backup();
        pp->horizon.backup();
    }

    // save off stats for skip4
    if (MoveSkip4 == 0)
    {
        short stat;

        for (stat = STAT_SKIP4_START; stat <= STAT_SKIP4_INTERP_END; stat++)
        {
            SWStatIterator it(stat);
            while (auto actor = it.Next())
            {
                if (!actor->hasU()) continue;
                actor->backuppos();
                actor->user.oz = actor->opos.Z;
            }
        }
    }

    // save off stats for skip2
    if (MoveSkip2 == 0)
    {
        short stat;

        for (stat = STAT_SKIP2_START; stat <= STAT_SKIP2_INTERP_END; stat++)
        {
            SWStatIterator it(stat);
            while (auto actor = it.Next())
            {
                if (!actor->hasU()) continue;
                actor->backuppos();
                actor->user.oz = actor->opos.Z;
            }
        }
    }

    SWSpriteIterator it;
    // back up all sprite angles.
    while (auto actor = it.Next())
    {
        actor->backupang();
    }
}


void PlayerTimers(PLAYER* pp)
{
    InventoryTimer(pp);
}

void ChopsCheck(PLAYER* pp)
{
    if (!M_Active() && !(pp->Flags & PF_DEAD) && !pp->sop_riding && numplayers <= 1)
    {
        if (pp->input.actions & ~SB_RUN || pp->input.fvel || pp->input.svel || pp->input.avel || pp->input.horz ||
            (pp->Flags & (PF_CLIMBING | PF_FALLING | PF_DIVING)))
        {
            // Hit a input key or other reason to stop chops
            //if (pp->Chops && pp->Chops->State != pp->Chops->State->RetractState)
            if (pp->Chops)
            {
                if (!pp->sop_control) // specail case
                    pp->Flags &= ~(PF_WEAPON_DOWN);
                ChopsSetRetract(pp);
            }
            ChopTics = 0;
        }
        else
        {
            ChopTics += synctics;
            if (!pp->Chops)
            {
                // Chops not up
                if (ChopTics > 30*120)
                {
                    ChopTics = 0;
                    // take weapon down
                    pp->Flags |= (PF_WEAPON_DOWN);
                    InitChops(pp);
                }
            }
            else
            {
                // Chops already up
                if (ChopTics > 30*120)
                {
                    ChopTics = 0;
                    // bring weapon back up
                    pp->Flags &= ~(PF_WEAPON_DOWN);
                    ChopsSetRetract(pp);
                }
            }
        }
    }
}

void PlayerGlobal(PLAYER* pp)
{
    // This is the place for things that effect the player no matter what hes
    // doing


    PlayerTimers(pp);

    if (pp->Flags & (PF_RECOIL))
        DoPlayerRecoil(pp);

    if (!(pp->Flags & PF_CLIP_CHEAT))
    {
        if (pp->hi_sectp && pp->lo_sectp)
        {
            const double PLAYER_MIN_HEIGHT = 20;

            if (abs(pp->loz - pp->hiz) < PLAYER_MIN_HEIGHT)
            {
                if (!(pp->Flags & PF_DEAD))
                {
                    PlayerUpdateHealth(pp, -pp->actor->user.Health);  // Make sure he dies!
                    PlayerCheckDeath(pp, nullptr);

                    if (pp->Flags & (PF_DEAD))
                        return;
                }
            }
        }
    }

    if (pp->FadeAmt > 0 && MoveSkip4 == 0)
    {
        DoPaletteFlash(pp);
    }

    // camera stuff that can't be done in drawscreen
    if (pp->circle_camera_dist > CIRCLE_CAMERA_DIST_MIN)
        pp->circle_camera_ang += DAngle::fromBuild(14);

    if (pp->camera_check_time_delay > 0)
    {
        pp->camera_check_time_delay -= synctics;
        if (pp->camera_check_time_delay <= 0)
            pp->camera_check_time_delay = 0;
    }
}


void MultiPlayLimits(void)
{
    short pnum;
    PLAYER* pp;
    bool Done = false;

    if (gNet.MultiGameType != MULTI_GAME_COMMBAT)
        return;

    if (gNet.KillLimit)
    {
        TRAVERSE_CONNECT(pnum)
        {
            pp = Player + pnum;
            if (pp->Kills >= gNet.KillLimit)
            {
                Done = true;
            }
        }
    }

    if (gNet.TimeLimit)
    {
        gNet.TimeLimitClock -= synctics;

        if (gNet.TimeLimitClock <= 0)
            Done = true;
    }

    if (Done)
    {
        gNet.TimeLimitClock = gNet.TimeLimit;

        MapRecord *next = nullptr;
        next = FindNextMap(currentLevel);
		ChangeLevel(next, g_nextskill);
    }
}

void PauseMultiPlay(void)
{
}

void domovethings(void)
{
    short i, pnum;

    PLAYER* pp;
    extern int FinishTimer;


    UpdateInterpolations();                  // Stick at beginning of domovethings
    so_updateinterpolations();               // Stick at beginning of domovethings
    MoveSkipSavePos();

    if (paused)
    {
        if (!ReloadPrompt)
            return;
    }

    PlayClock += synctics;
    if (PlayClock == 2*synctics) gameaction = ga_autosave;	// let the game run for 1 frame before saving.

    thinktime.Reset();
    thinktime.Clock();

    DoAnim(synctics);

    // should pass pnum and use syncbits
    DoSector();

    ProcessVisOn();
    if (MoveSkip4 == 0)
    {
        ProcessQuakeOn();
        ProcessQuakeSpot();
        JS_ProcessEchoSpot();
    }

    actortime.Reset();
    actortime.Clock();
    SpriteControl();
    actortime.Unclock();

    TRAVERSE_CONNECT(pnum)
    {
        extern short screenpeek;
        extern bool PlayerTrackingMode;
        extern PLAYER* GlobPlayerP;

        pp = Player + pnum;
        GlobPlayerP = pp;

        if (pp->cookieTime)
        {
            pp->cookieTime -= synctics;
            if (pp->cookieTime <= 0)
            {
                memset(pp->cookieQuote, 0, sizeof(pp->cookieQuote));
                pp->cookieTime = 0;
            }
        }

        // auto tracking mode for single player multi-game
        if (numplayers <= 1 && PlayerTrackingMode && pnum == screenpeek && screenpeek != myconnectindex)
        {
            int deltax = Player[myconnectindex].int_ppos().X - Player[screenpeek].int_ppos().X;
            int deltay = Player[myconnectindex].int_ppos().Y - Player[screenpeek].int_ppos().Y;
            Player[screenpeek].angle.settarget(VecToAngle(deltax, deltay));
        }

        if (!(pp->Flags & PF_DEAD))
        {
            WeaponOperate(pp);
            PlayerOperateEnv(pp);
        }

        // do for moving sectors
        DoPlayerSectorUpdatePreMove(pp);
        ChopsCheck(pp);

        // Reset flags used while tying input to framerate
        pp->Flags2 &= ~(PF2_INPUT_CAN_AIM|PF2_INPUT_CAN_TURN_GENERAL|PF2_INPUT_CAN_TURN_VEHICLE|PF2_INPUT_CAN_TURN_TURRET);
        pp->horizon.resetadjustment();
        pp->angle.resetadjustment();

        // disable synchronised input if set by game.
        resetForcedSyncInput();

        if (pp->DoPlayerAction) pp->DoPlayerAction(pp);

        UpdatePlayerSprite(pp);

        pSpriteControl(pp);

        PlayerStateControl(pp->actor);

        DoPlayerSectorUpdatePostMove(pp);
        PlayerGlobal(pp);
    }

    MultiPlayLimits();

    thinktime.Unclock();

#if 0
    CorrectPrediction(movefifoplc - 1);
#endif
    if (FinishTimer)
    {
        if ((FinishTimer -= synctics) <= 0)
        {
            FinishTimer = 0;
			MapRecord *map = nullptr;
			if (FinishAnim == ANIM_SUMO || FinishAnim == ANIM_SERP)
			{
				map = FindNextMap(currentLevel);
			}
			ChangeLevel(map, g_nextskill, true);
        }
    }
}


void InitAllPlayers(void)
{
    PLAYER* pp;
    PLAYER* pfirst = Player;
    int i;
    extern bool NewGame;
    //int fz,cz;

    pfirst->horizon.horiz = q16horiz(0);

    // Initialize all [MAX_SW_PLAYERS] arrays here!
    for (pp = Player; pp < &Player[MAX_SW_PLAYERS]; pp++)
    {
        pp->pos = pp->opos = pfirst->pos;
        pp->angle.ang = pp->angle.oang = pfirst->angle.ang;
        pp->horizon.horiz = pp->horizon.ohoriz = pfirst->horizon.horiz;
        pp->cursector = pfirst->cursector;
        // set like this so that player can trigger something on start of the level
        pp->lastcursector = pfirst->cursector+1;

        //pp->MaxHealth = 100;

        pp->oldpos.X = 0;
        pp->oldpos.Y = 0;
        pp->climb_ndx = 10;
        pp->KillerActor = nullptr;
        pp->Kills = 0;
        pp->bcnt = 0;
        pp->UziShellLeftAlt = 0;
        pp->UziShellRightAlt = 0;

        pp->p_ceiling_dist = PLAYER_RUN_CEILING_DIST;
        pp->p_floor_dist = PLAYER_RUN_FLOOR_DIST;

        pp->WpnGotOnceFlags = 0;
        pp->DoPlayerAction = DoPlayerBeginRun;
        pp->KeyPressBits = ESyncBits::FromInt(0xFFFFFFFF);
        memset(pp->KilledPlayer,0,sizeof(pp->KilledPlayer));

        if (NewGame)
        {
            for (i = 0; i < MAX_INVENTORY; i++)
            {
                pp->InventoryAmount[i] = 0;
                pp->InventoryPercent[i] = 0;
            }
        }

        // My palette flashing stuff
        pp->FadeAmt = 0;
        pp->FadeTics = 0;
        pp->StartColor = 0;
        pp->horizon.horizoff = q16horiz(0);

        INITLIST(&pp->PanelSpriteList);
    }
}

int SearchSpawnPosition(PLAYER* pp)
{
    PLAYER* opp; // other player
    short pos_num;
    short pnum;
    bool blocked;

    do
    {
        // get a spawn position
        pos_num = RandomRange(MAX_SW_PLAYERS);
        SWStatIterator it(STAT_MULTI_START + pos_num);
        auto spawn_sprite = it.Next();
        if (spawn_sprite == nullptr)
            return 0;

        blocked = false;

        // check to see if anyone else is blocking this spot
        TRAVERSE_CONNECT(pnum)
        {
            opp = &Player[pnum];

            if (opp != pp)  // don't test for yourself
            {
                if (FindDistance3D(spawn_sprite->int_pos() - opp->int_ppos()) < 1000)
                {
                    blocked = true;
                    break;
                }
            }
        }
    }
    while (blocked);

    return pos_num;
}

bool SpawnPositionUsed[MAX_SW_PLAYERS_REG+1];

void PlayerSpawnPosition(PLAYER* pp)
{
    short pnum = short(pp - Player);
    short pos_num = pnum;
    int fz,cz;
    int i;
    DSWActor* spawn_sprite = nullptr;

    // find the first unused spawn position
    // garauntees that the spawn pos 0 will be used
    // Note: This code is not used if the player is DEAD and respawning

    for (i = 0; i < MAX_SW_PLAYERS; i++)
    {
        if (!SpawnPositionUsed[i])
        {
            pos_num = i;
            break;
        }
    }

    // need to call this routine BEFORE resetting DEATH flag

    switch (gNet.MultiGameType)
    {
    case MULTI_GAME_NONE:
    {
        // start from the beginning
        SWStatIterator it(STAT_MULTI_START + 0);
        spawn_sprite = it.Next();
        break;
    }
    case MULTI_GAME_COMMBAT:
    case MULTI_GAME_AI_BOTS:
    {
        // start from random position after death
        if (pp->Flags & (PF_DEAD))
        {
            pos_num = SearchSpawnPosition(pp);
        }
        SWStatIterator it(STAT_MULTI_START + pos_num);
        spawn_sprite = it.Next();
        break;
    }
    case MULTI_GAME_COOPERATIVE:
    {
        // start your assigned spot
        SWStatIterator it(STAT_MULTI_START + pos_num);
        spawn_sprite = it.Next();
        break;
    }
    }

    SpawnPositionUsed[pos_num] = true;

    if (spawn_sprite == nullptr)
    {
        SWStatIterator it(STAT_MULTI_START + 0);
        spawn_sprite = it.Next();
    }

    ASSERT(spawn_sprite != nullptr);

    pp->pos = spawn_sprite->spr.pos;
    pp->opos = pp->pos;
    pp->angle.ang = pp->angle.oang = spawn_sprite->spr.angle;
    pp->setcursector(spawn_sprite->sector());

    getzsofslopeptr(pp->cursector, pp->int_ppos().X, pp->int_ppos().Y, &cz, &fz);
    // if too close to the floor - stand up
    if (pp->int_ppos().Z > fz - PLAYER_HEIGHT)
    {
        pp->set_int_ppos_Z(fz - PLAYER_HEIGHT);
        pp->opos.Z = pp->pos.Z;
    }
}


void InitMultiPlayerInfo(void)
{
    PLAYER* pp;
    short pnum;
    unsigned stat;
    int tag;
    static short MultiStatList[] =
    {
        STAT_MULTI_START,
        STAT_CO_OP_START
    };

    // this routine is called before SpriteSetup - process start positions NOW
    SWStatIterator it(STAT_DEFAULT);
    while (auto actor = it.Next())
    {
        tag = actor->spr.hitag;

        if (actor->spr.picnum == ST1)
        {
            switch (tag)
            {
            case MULTI_PLAYER_START:
                change_actor_stat(actor, STAT_MULTI_START + actor->spr.lotag);
                break;
            case MULTI_COOPERATIVE_START:
                change_actor_stat(actor, STAT_CO_OP_START + actor->spr.lotag);
                break;
            }
        }
    }

    // set up the zero starting positions - its not saved in the map as a ST1 sprite
    // like the others
    pp = Player;
    for (stat = 0; stat < SIZ(MultiStatList); stat++)
    {
        if (gNet.MultiGameType != MULTI_GAME_NONE)
        {
            // if start position is physically set then don't spawn a new one
            it.Reset(MultiStatList[stat]);
            if (it.Next())
                continue;
        }

        auto start0 = SpawnActor(MultiStatList[stat], ST1, nullptr, pp->cursector, pp->pos, pp->angle.ang, 0);
        start0->clearUser();
        start0->spr.picnum = ST1;
    }

    memset(SpawnPositionUsed,0,sizeof(SpawnPositionUsed));

    // Initialize multi player positions here
    //for (pp = Player; pp < Player + numplayers; pp++)
    TRAVERSE_CONNECT(pnum)
    {
        pp = Player + pnum;
        switch (gNet.MultiGameType)
        {
        case MULTI_GAME_NONE:
            PlayerSpawnPosition(pp);
            break;
        //return;
        case MULTI_GAME_COMMBAT:
        case MULTI_GAME_AI_BOTS:
            // there are no keys in deathmatch play
            memset(Player[0].HasKey,0xFFFF,sizeof(Player[0].HasKey));
            memset(pp->HasKey,0xFFFF,sizeof(pp->HasKey));
            PlayerSpawnPosition(pp);
            break;
        case MULTI_GAME_COOPERATIVE:
            PlayerSpawnPosition(pp);
            break;
        }
    }
}

// If player stepped in something gooey, track it all over the place.
int DoFootPrints(DSWActor* actor)
{
    if (actor->user.PlayerP)
    {
        if (!actor->user.PlayerP->insector())
            return 0;

        if (FAF_ConnectArea(actor->user.PlayerP->cursector))
            return 0;

        if (actor->user.PlayerP->NumFootPrints > 0)
        {
            QueueFootPrint(actor);
        }
    }

    return 0;
}

void CheckFootPrints(PLAYER* pp)
{
    if (pp->NumFootPrints <= 0 || FootMode != WATER_FOOT)
    {
        // Hey, you just got your feet wet!
        pp->NumFootPrints = RandomRange(10)+3;
        FootMode = WATER_FOOT;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------


#include "saveable.h"

static saveable_code saveable_player_code[] =
{
    SAVE_CODE(DoPlayerSlide),
    SAVE_CODE(DoPlayerWade),
    SAVE_CODE(DoPlayerBeginWade),
    SAVE_CODE(DoPlayerBeginCrawl),
    SAVE_CODE(DoPlayerCrawl),
    SAVE_CODE(DoPlayerRun),
    SAVE_CODE(DoPlayerBeginRun),
    SAVE_CODE(DoPlayerFall),
    SAVE_CODE(DoPlayerBeginFall),
    SAVE_CODE(DoPlayerJump),
    SAVE_CODE(DoPlayerBeginJump),
    SAVE_CODE(DoPlayerForceJump),
    SAVE_CODE(DoPlayerBeginFly),
    SAVE_CODE(DoPlayerFly),
    SAVE_CODE(DoPlayerBeginClimb),
    SAVE_CODE(DoPlayerClimb),
    SAVE_CODE(DoPlayerBeginDie),
    SAVE_CODE(DoPlayerBeginOperateVehicle),
    SAVE_CODE(DoPlayerBeginOperate),
    SAVE_CODE(DoPlayerOperateVehicle),
    SAVE_CODE(DoPlayerOperateTurret),
    SAVE_CODE(DoPlayerBeginDive),
    SAVE_CODE(DoPlayerDive),
    SAVE_CODE(DoPlayerTeleportPause),
    SAVE_CODE(DoPlayerTestCrawl),
    SAVE_CODE(DoPlayerDeathFlip),
    SAVE_CODE(DoPlayerDeathCrumble),
    SAVE_CODE(DoPlayerDeathExplode),
    SAVE_CODE(DoPlayerDeathFall),
    SAVE_CODE(DoPlayerBeginDiveNoWarp),
    SAVE_CODE(DoPlayerCurrent),
};

static saveable_data saveable_player_data[] =
{
    SAVE_DATA(Player),
    SAVE_DATA(s_PlayerNinjaRun),
    SAVE_DATA(sg_PlayerNinjaRun),
    SAVE_DATA(s_PlayerNinjaStand),
    SAVE_DATA(sg_PlayerNinjaStand),
    SAVE_DATA(s_PlayerNinjaThrow),
    SAVE_DATA(sg_PlayerNinjaThrow),
    SAVE_DATA(s_PlayerNinjaJump),
    SAVE_DATA(sg_PlayerNinjaJump),
    SAVE_DATA(s_PlayerNinjaFall),
    SAVE_DATA(sg_PlayerNinjaFall),
    SAVE_DATA(s_PlayerNinjaClimb),
    SAVE_DATA(sg_PlayerNinjaClimb),
    SAVE_DATA(s_PlayerNinjaCrawl),
    SAVE_DATA(sg_PlayerNinjaCrawl),
    SAVE_DATA(s_PlayerNinjaSwim),
    SAVE_DATA(sg_PlayerNinjaSwim),
    SAVE_DATA(s_PlayerHeadFly),
    SAVE_DATA(sg_PlayerHeadFly),
    SAVE_DATA(s_PlayerHead),
    SAVE_DATA(sg_PlayerHead),
    SAVE_DATA(s_PlayerHeadHurl),
    SAVE_DATA(sg_PlayerHeadHurl),
    SAVE_DATA(s_PlayerDeath),
    SAVE_DATA(sg_PlayerDeath),
    SAVE_DATA(s_PlayerNinjaSword),
    SAVE_DATA(sg_PlayerNinjaSword),
    SAVE_DATA(s_PlayerNinjaPunch),
    SAVE_DATA(sg_PlayerNinjaPunch),
    SAVE_DATA(s_PlayerNinjaFly),
    SAVE_DATA(sg_PlayerNinjaFly),
};

saveable_module saveable_player =
{
    // code
    saveable_player_code,
    SIZ(saveable_player_code),

    // data
    saveable_player_data,
    SIZ(saveable_player_data)
};

DEFINE_FIELD_X(SWPlayer, PLAYER, sop_remote)
DEFINE_FIELD_X(SWPlayer, PLAYER, jump_count)
DEFINE_FIELD_X(SWPlayer, PLAYER, jump_speed)
DEFINE_FIELD_X(SWPlayer, PLAYER, down_speed)
DEFINE_FIELD_X(SWPlayer, PLAYER, up_speed)
DEFINE_FIELD_X(SWPlayer, PLAYER, z_speed)
DEFINE_FIELD_X(SWPlayer, PLAYER, climb_ndx)
DEFINE_FIELD_X(SWPlayer, PLAYER, hiz)
DEFINE_FIELD_X(SWPlayer, PLAYER, loz)
DEFINE_FIELD_X(SWPlayer, PLAYER, p_ceiling_dist)
DEFINE_FIELD_X(SWPlayer, PLAYER, p_floor_dist)
DEFINE_FIELD_X(SWPlayer, PLAYER, circle_camera_dist)
//DEFINE_FIELD_X(SWPlayer, PLAYER, six)
//DEFINE_FIELD_X(SWPlayer, PLAYER, siy)
//DEFINE_FIELD_X(SWPlayer, PLAYER, siz)
DEFINE_FIELD_X(SWPlayer, PLAYER, siang)
//DEFINE_FIELD_X(SWPlayer, PLAYER, xvect)
//DEFINE_FIELD_X(SWPlayer, PLAYER, yvect)
//DEFINE_FIELD_X(SWPlayer, PLAYER, oxvect)
//DEFINE_FIELD_X(SWPlayer, PLAYER, oyvect)
DEFINE_FIELD_X(SWPlayer, PLAYER, friction)
//DEFINE_FIELD_X(SWPlayer, PLAYER, slide_xvect)
//DEFINE_FIELD_X(SWPlayer, PLAYER, slide_yvect)
DEFINE_FIELD_X(SWPlayer, PLAYER, slide_ang)
DEFINE_FIELD_X(SWPlayer, PLAYER, slide_dec)
DEFINE_FIELD_X(SWPlayer, PLAYER, drive_avel)
DEFINE_FIELD_X(SWPlayer, PLAYER, circle_camera_ang)
DEFINE_FIELD_X(SWPlayer, PLAYER, camera_check_time_delay)
DEFINE_FIELD_X(SWPlayer, PLAYER, cursector)
DEFINE_FIELD_X(SWPlayer, PLAYER, lastcursector)
DEFINE_FIELD_X(SWPlayer, PLAYER, hvel)
DEFINE_FIELD_X(SWPlayer, PLAYER, tilt)
DEFINE_FIELD_X(SWPlayer, PLAYER, tilt_dest)
DEFINE_FIELD_X(SWPlayer, PLAYER, recoil_amt)
DEFINE_FIELD_X(SWPlayer, PLAYER, recoil_speed)
DEFINE_FIELD_X(SWPlayer, PLAYER, recoil_ndx)
DEFINE_FIELD_X(SWPlayer, PLAYER, recoil_horizoff)
DEFINE_FIELD_X(SWPlayer, PLAYER, recoil_ohorizoff)
//DEFINE_FIELD_X(SWPlayer, PLAYER, oldposx)
//DEFINE_FIELD_X(SWPlayer, PLAYER, oldposy)
//DEFINE_FIELD_X(SWPlayer, PLAYER, oldposz)
//DEFINE_FIELD_X(SWPlayer, PLAYER, Revolve.X)
//DEFINE_FIELD_X(SWPlayer, PLAYER, Revolve.Y)
DEFINE_FIELD_X(SWPlayer, PLAYER, RevolveDeltaAng)
DEFINE_FIELD_X(SWPlayer, PLAYER, pnum)
DEFINE_FIELD_X(SWPlayer, PLAYER, LadderSector)
//DEFINE_FIELD_X(SWPlayer, PLAYER, lx)
//DEFINE_FIELD_X(SWPlayer, PLAYER, ly)
DEFINE_FIELD_X(SWPlayer, PLAYER, JumpDuration)
DEFINE_FIELD_X(SWPlayer, PLAYER, WadeDepth)
DEFINE_FIELD_X(SWPlayer, PLAYER, bob_amt)
DEFINE_FIELD_X(SWPlayer, PLAYER, bob_ndx)
DEFINE_FIELD_X(SWPlayer, PLAYER, bcnt)
DEFINE_FIELD_X(SWPlayer, PLAYER, bob_z)
DEFINE_FIELD_X(SWPlayer, PLAYER, obob_z)
DEFINE_FIELD_X(SWPlayer, PLAYER, playerreadyflag)
DEFINE_FIELD_X(SWPlayer, PLAYER, Flags)
DEFINE_FIELD_X(SWPlayer, PLAYER, Flags2)
DEFINE_FIELD_X(SWPlayer, PLAYER, HasKey)
DEFINE_FIELD_X(SWPlayer, PLAYER, SwordAng)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnGotOnceFlags)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnFlags)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnAmmo)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnNum)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnRocketType)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnRocketHeat)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnRocketNuke)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnFlameType)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnFirstType)
DEFINE_FIELD_X(SWPlayer, PLAYER, WeaponType)
DEFINE_FIELD_X(SWPlayer, PLAYER, FirePause)
DEFINE_FIELD_X(SWPlayer, PLAYER, InventoryNum)
DEFINE_FIELD_X(SWPlayer, PLAYER, InventoryBarTics)
DEFINE_FIELD_X(SWPlayer, PLAYER, InventoryTics)
DEFINE_FIELD_X(SWPlayer, PLAYER, InventoryPercent)
DEFINE_FIELD_X(SWPlayer, PLAYER, InventoryAmount)
DEFINE_FIELD_X(SWPlayer, PLAYER, InventoryActive)
DEFINE_FIELD_X(SWPlayer, PLAYER, DiveTics)
DEFINE_FIELD_X(SWPlayer, PLAYER, DiveDamageTics)
DEFINE_FIELD_X(SWPlayer, PLAYER, DeathType)
DEFINE_FIELD_X(SWPlayer, PLAYER, Kills)
DEFINE_FIELD_X(SWPlayer, PLAYER, SecretsFound)
DEFINE_FIELD_X(SWPlayer, PLAYER, Armor)
DEFINE_FIELD_X(SWPlayer, PLAYER, MaxHealth)
DEFINE_FIELD_X(SWPlayer, PLAYER, UziShellLeftAlt)
DEFINE_FIELD_X(SWPlayer, PLAYER, UziShellRightAlt)
DEFINE_FIELD_X(SWPlayer, PLAYER, TeamColor)
DEFINE_FIELD_X(SWPlayer, PLAYER, FadeTics)
DEFINE_FIELD_X(SWPlayer, PLAYER, FadeAmt)
DEFINE_FIELD_X(SWPlayer, PLAYER, NightVision)
DEFINE_FIELD_X(SWPlayer, PLAYER, StartColor)
DEFINE_FIELD_X(SWPlayer, PLAYER, IsAI)
DEFINE_FIELD_X(SWPlayer, PLAYER, fta)
DEFINE_FIELD_X(SWPlayer, PLAYER, ftq)
DEFINE_FIELD_X(SWPlayer, PLAYER, NumFootPrints)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnUziType)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnShotgunType)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnShotgunAuto)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnShotgunLastShell)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnRailType)
DEFINE_FIELD_X(SWPlayer, PLAYER, Bloody)
DEFINE_FIELD_X(SWPlayer, PLAYER, InitingNuke)
DEFINE_FIELD_X(SWPlayer, PLAYER, TestNukeInit)
DEFINE_FIELD_X(SWPlayer, PLAYER, NukeInitialized)
DEFINE_FIELD_X(SWPlayer, PLAYER, FistAng)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnKungFuMove)
DEFINE_FIELD_X(SWPlayer, PLAYER, HitBy)
DEFINE_FIELD_X(SWPlayer, PLAYER, Reverb)
DEFINE_FIELD_X(SWPlayer, PLAYER, Heads)
DEFINE_FIELD_X(SWPlayer, PLAYER, PlayerVersion)
DEFINE_FIELD_X(SWPlayer, PLAYER, WpnReloadState)

DEFINE_ACTION_FUNCTION(_SWPlayer, WeaponNum)
{
    PARAM_SELF_STRUCT_PROLOGUE(PLAYER);
    ACTION_RETURN_INT(self->actor->user.WeaponNum);
}

DEFINE_ACTION_FUNCTION(_SWPlayer, Health)
{
    PARAM_SELF_STRUCT_PROLOGUE(PLAYER);
    ACTION_RETURN_INT(self->actor->user.Health);
}

DEFINE_ACTION_FUNCTION(_SWPlayer, MaxUserHealth)
{
    PARAM_SELF_STRUCT_PROLOGUE(PLAYER);
    ACTION_RETURN_INT(self->actor->user.MaxHealth);
}

DEFINE_ACTION_FUNCTION(_SWPlayer, GetBuildAngle)
{
    PARAM_SELF_STRUCT_PROLOGUE(PLAYER);
    ACTION_RETURN_INT(self->angle.ang.Buildang());
}

DEFINE_ACTION_FUNCTION(_SW, WeaponMaxAmmo)
{
    PARAM_PROLOGUE;
    PARAM_INT(wp);
    ACTION_RETURN_INT(DamageData[wp].max_ammo);
}

DEFINE_ACTION_FUNCTION(_SW, InventoryFlags)
{
    PARAM_PROLOGUE;
    PARAM_INT(inv);
    INVENTORY_DATA* id = &InventoryData[inv];
    ACTION_RETURN_INT(id->Flags);
}

DEFINE_ACTION_FUNCTION(_SW, GetViewPlayer)
{
    PARAM_PROLOGUE;
    ACTION_RETURN_POINTER(&Player[screenpeek]);
}

DEFINE_ACTION_FUNCTION(_SW, RealWeapon)
{
    PARAM_PROLOGUE;
    PARAM_INT(inv);
    int w = DamageData[inv].with_weapon;
    ACTION_RETURN_INT(w == -1? inv : w);
}

END_SW_NS
