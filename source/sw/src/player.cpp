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
#include "build.h"
#include "common.h"

#include "mytypes.h"
#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "player.h"
#include "lists.h"
#include "warp.h"
#include "quake.h"
#include "text.h"

#include "common_game.h"
#include "function.h"
#include "control.h"
#include "trigger.h"

#include "savedef.h"
#include "menus.h"
#include "network.h"
#include "pal.h"
#include "demo.h"
#include "mclip.h"
#include "fx_man.h"

#include "sprite.h"
#include "weapon.h"
#include "ninja.h"
#include "break.h"
#include "jsector.h"
#include "sector.h"
#include "actor.h"
#include "colormap.h"
#include "music.h"
#include "vis.h"
#include "track.h"
#include "interp.h"


#define SO_DRIVE_SOUND 2
#define SO_IDLE_SOUND 1

extern SWBOOL NoMeters;
extern int Follow_posx,Follow_posy;

#define TEST_UNDERWATER(pp) (TEST(sector[(pp)->cursectnum].extra, SECTFX_UNDERWATER))
extern unsigned char palette_data[256][3];      // Global palette array

//#define PLAYER_MIN_HEIGHT (Z(30))
//#define PLAYER_MIN_HEIGHT_JUMP (Z(20))
#define PLAYER_MIN_HEIGHT (Z(20))
#define PLAYER_CRAWL_WADE_DEPTH (30)

USER puser[MAX_SW_PLAYERS_REG];

//int16_t gNet.MultiGameType = MULTI_GAME_NONE;
SWBOOL NightVision = FALSE;
extern SWBOOL FinishedLevel;

//#define PLAYER_TURN_SCALE (8)
#define PLAYER_TURN_SCALE (12)

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

#define PLAYER_RUN_LOCK(pp)                                 \
    if (TEST_SYNC_KEY((pp), SK_RUN_LOCK))               \
    {                                               \
        if (FLAG_KEY_PRESSED((pp), SK_RUN_LOCK))        \
        {                                           \
            FLAG_KEY_RELEASE((pp), SK_RUN_LOCK);        \
            FLIP((pp)->Flags, PF_LOCK_RUN);             \
            gs.AutoRun = !!TEST((pp)->Flags, PF_LOCK_RUN); \
            sprintf(ds, "Run mode %s", TEST((pp)->Flags, PF_LOCK_RUN) ? "ON" : "OFF"); \
            PutStringInfo((pp), ds);                    \
        }                                           \
    }                                               \
    else                                                \
        FLAG_KEY_RESET((pp), SK_RUN_LOCK)               \

#define JUMP_STUFF 4

// just like 2 except can jump higher - less gravity
// goes better with slightly slower run speed than I had it at
#if JUMP_STUFF == 4
#define PLAYER_JUMP_GRAV 24
#define PLAYER_JUMP_AMT (-650)
#define PLAYER_CLIMB_JUMP_AMT (-1100)
#define MAX_JUMP_DURATION 12
char PlayerGravity = PLAYER_JUMP_GRAV;
#endif

int vel, svel, angvel;
extern SWBOOL DebugOperate;

//unsigned char synctics, lastsynctics;

int dimensionmode, zoom;

PLAYER Player[MAX_SW_PLAYERS_REG + 1];

//short snum = 0;

// These are a bunch of kens variables for the player

short NormalVisibility;

static short oldmousebstatus = 0;
static short mousx, mousy, mousz, bstatus;

int InitBloodSpray(int16_t SpriteNum, SWBOOL dogib, short velocity);

SPRITEp FindNearSprite(SPRITEp sp, short stat);
SWBOOL PlayerOnLadder(PLAYERp pp);
void DoPlayerSlide(PLAYERp pp);
void DoPlayerBeginSwim(PLAYERp pp);
void DoPlayerSwim(PLAYERp pp);
void DoPlayerWade(PLAYERp pp);
void DoPlayerBeginWade(PLAYERp pp);
void DoPlayerBeginCrawl(PLAYERp pp);
void DoPlayerCrawl(PLAYERp pp);
void DoPlayerRun(PLAYERp pp);
void DoPlayerBeginRun(PLAYERp pp);
void DoPlayerFall(PLAYERp pp);
void DoPlayerBeginFall(PLAYERp pp);
void DoPlayerJump(PLAYERp pp);
void DoPlayerBeginJump(PLAYERp pp);
void DoPlayerForceJump(PLAYERp pp);
void DoPlayerBeginFly(PLAYERp pp);
void DoPlayerFly(PLAYERp pp);
void DoPlayerBeginClimb(PLAYERp pp);
void DoPlayerClimb(PLAYERp pp);
void DoPlayerBeginDie(PLAYERp pp);
void DoPlayerDie(PLAYERp pp);
void DoPlayerBeginOperateBoat(PLAYERp pp);
void DoPlayerBeginOperateTank(PLAYERp pp);
void DoPlayerBeginOperate(PLAYERp pp);
void DoPlayerOperateBoat(PLAYERp pp);
void DoPlayerOperateTank(PLAYERp pp);
void DoPlayerOperateTurret(PLAYERp pp);
void DoPlayerBeginDive(PLAYERp pp);
void DoPlayerDive(PLAYERp pp);
void DoPlayerTeleportPause(PLAYERp pp);
SWBOOL PlayerFlyKey(PLAYERp pp);
void OperateSectorObject(SECTOR_OBJECTp sop, short newang, int newx, int newy);
void CheckFootPrints(PLAYERp pp);
SWBOOL DoPlayerTestCrawl(PLAYERp pp);
void DoPlayerDeathFlip(PLAYERp pp);
void DoPlayerDeathCrumble(PLAYERp pp);
void DoPlayerDeathExplode(PLAYERp pp);
void DoPlayerDeathFall(PLAYERp pp);

void PlayerCheckValidMove(PLAYERp pp);
void PlayerWarpUpdatePos(PLAYERp pp);
void DoPlayerBeginDiveNoWarp(PLAYERp pp);
int PlayerCanDiveNoWarp(PLAYERp pp);
void DoPlayerCurrent(PLAYERp pp);
int GetOverlapSector2(int x, int y, short *over, short *under);
void PlayerToRemote(PLAYERp pp);
void PlayerRemoteInit(PLAYERp pp);
void PlayerSpawnPosition(PLAYERp pp);

extern short target_ang;

//////////////////////
//
// PLAYER SPECIFIC
//
//////////////////////

#if 1
#define PLAYER_NINJA_RATE 14

int DoFootPrints(short SpriteNum);

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

STATEp sg_PlayerNinjaRun[] =
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

STATEp sg_PlayerNinjaRun[] =
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
STATEp sg_PlayerNinjaStand[] =
{
    s_PlayerNinjaStand[0],
    s_PlayerNinjaStand[1],
    s_PlayerNinjaStand[2],
    s_PlayerNinjaStand[3],
    s_PlayerNinjaStand[4]
};


#define NINJA_STAR_RATE 12

extern STATEp sg_NinjaRun[];
int DoPlayerSpriteReset(short SpriteNum);

#if 0
STATE s_PlayerNinjaThrow[5][4] =
{
    {
        {PLAYER_SHOOT_R0 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][1]},
        {PLAYER_SHOOT_R0 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][2]},
        {PLAYER_SHOOT_R0 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][3]},
        {PLAYER_SHOOT_R0 + 0, NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_SHOOT_R1 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][1]},
        {PLAYER_SHOOT_R1 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][2]},
        {PLAYER_SHOOT_R1 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][3]},
        {PLAYER_SHOOT_R1 + 0, NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_SHOOT_R2 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][1]},
        {PLAYER_SHOOT_R2 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][2]},
        {PLAYER_SHOOT_R2 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][3]},
        {PLAYER_SHOOT_R2 + 0, NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_SHOOT_R3 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][1]},
        {PLAYER_SHOOT_R3 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][2]},
        {PLAYER_SHOOT_R3 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][3]},
        {PLAYER_SHOOT_R3 + 0, NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_SHOOT_R4 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][1]},
        {PLAYER_SHOOT_R4 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][2]},
        {PLAYER_SHOOT_R4 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][3]},
        {PLAYER_SHOOT_R4 + 0, NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
};
#endif

#if 1
STATE s_PlayerNinjaThrow[5][4] =
{
    {
        {PLAYER_NINJA_SHOOT_R0 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][1]},
        {PLAYER_NINJA_SHOOT_R0 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][2]},
        {PLAYER_NINJA_SHOOT_R0 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[0][3]},
        {PLAYER_NINJA_SHOOT_R0 + 0, NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_NINJA_SHOOT_R1 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][1]},
        {PLAYER_NINJA_SHOOT_R1 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][2]},
        {PLAYER_NINJA_SHOOT_R1 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[1][3]},
        {PLAYER_NINJA_SHOOT_R1 + 0, NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_NINJA_SHOOT_R2 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][1]},
        {PLAYER_NINJA_SHOOT_R2 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][2]},
        {PLAYER_NINJA_SHOOT_R2 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[2][3]},
        {PLAYER_NINJA_SHOOT_R2 + 0, NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_NINJA_SHOOT_R3 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][1]},
        {PLAYER_NINJA_SHOOT_R3 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][2]},
        {PLAYER_NINJA_SHOOT_R3 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[3][3]},
        {PLAYER_NINJA_SHOOT_R3 + 0, NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
    {
        {PLAYER_NINJA_SHOOT_R4 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][1]},
        {PLAYER_NINJA_SHOOT_R4 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][2]},
        {PLAYER_NINJA_SHOOT_R4 + 0, NINJA_STAR_RATE, NullAnimator, &s_PlayerNinjaThrow[4][3]},
        {PLAYER_NINJA_SHOOT_R4 + 0, NINJA_STAR_RATE | SF_PLAYER_FUNC, DoPlayerSpriteReset, &s_PlayerNinjaThrow[0][3]},
    },
};
#endif

STATEp sg_PlayerNinjaThrow[] =
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


STATEp sg_PlayerNinjaJump[] =
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


STATEp sg_PlayerNinjaFall[] =
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

STATEp sg_PlayerNinjaClimb[] =
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


STATEp sg_PlayerNinjaCrawl[] =
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


STATEp sg_PlayerNinjaSwim[] =
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

STATEp sg_PlayerHeadFly[] =
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

STATEp sg_PlayerHead[] =
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

STATEp sg_PlayerHeadHurl[] =
{
    s_PlayerHeadHurl[0],
    s_PlayerHeadHurl[1],
    s_PlayerHeadHurl[2],
    s_PlayerHeadHurl[3],
    s_PlayerHeadHurl[4]
};

#define NINJA_DIE_RATE 22

STATE s_PlayerDeath[5][10] =
{
    {
        {PLAYER_NINJA_DIE + 0, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][1]},
        {PLAYER_NINJA_DIE + 1, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][2]},
        {PLAYER_NINJA_DIE + 2, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][3]},
        {PLAYER_NINJA_DIE + 3, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][4]},
        {PLAYER_NINJA_DIE + 4, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][5]},
        {PLAYER_NINJA_DIE + 5, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][6]},
        {PLAYER_NINJA_DIE + 6, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][7]},
        {PLAYER_NINJA_DIE + 7, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][8]},
        {PLAYER_NINJA_DIE + 8, 0 | SF_QUICK_CALL, QueueFloorBlood, &s_PlayerDeath[0][9]},
        {PLAYER_NINJA_DIE + 8, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[0][9]},
    },
    {
        {PLAYER_NINJA_DIE + 0, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][1]},
        {PLAYER_NINJA_DIE + 1, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][2]},
        {PLAYER_NINJA_DIE + 2, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][3]},
        {PLAYER_NINJA_DIE + 3, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][4]},
        {PLAYER_NINJA_DIE + 4, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][5]},
        {PLAYER_NINJA_DIE + 5, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][6]},
        {PLAYER_NINJA_DIE + 6, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][7]},
        {PLAYER_NINJA_DIE + 7, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][8]},
        {PLAYER_NINJA_DIE + 8, 0 | SF_QUICK_CALL, QueueFloorBlood, &s_PlayerDeath[1][9]},
        {PLAYER_NINJA_DIE + 8, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[1][9]},
    },
    {
        {PLAYER_NINJA_DIE + 0, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][1]},
        {PLAYER_NINJA_DIE + 1, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][2]},
        {PLAYER_NINJA_DIE + 2, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][3]},
        {PLAYER_NINJA_DIE + 3, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][4]},
        {PLAYER_NINJA_DIE + 4, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][5]},
        {PLAYER_NINJA_DIE + 5, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][6]},
        {PLAYER_NINJA_DIE + 6, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][7]},
        {PLAYER_NINJA_DIE + 7, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][8]},
        {PLAYER_NINJA_DIE + 8, 0 | SF_QUICK_CALL, QueueFloorBlood, &s_PlayerDeath[2][9]},
        {PLAYER_NINJA_DIE + 8, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[2][9]},
    },
    {
        {PLAYER_NINJA_DIE + 0, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][1]},
        {PLAYER_NINJA_DIE + 1, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][2]},
        {PLAYER_NINJA_DIE + 2, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][3]},
        {PLAYER_NINJA_DIE + 3, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][4]},
        {PLAYER_NINJA_DIE + 4, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][5]},
        {PLAYER_NINJA_DIE + 5, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][6]},
        {PLAYER_NINJA_DIE + 6, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][7]},
        {PLAYER_NINJA_DIE + 7, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][8]},
        {PLAYER_NINJA_DIE + 8, 0 | SF_QUICK_CALL, QueueFloorBlood, &s_PlayerDeath[3][9]},
        {PLAYER_NINJA_DIE + 8, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[3][9]},
    },
    {
        {PLAYER_NINJA_DIE + 0, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][1]},
        {PLAYER_NINJA_DIE + 1, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][2]},
        {PLAYER_NINJA_DIE + 2, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][3]},
        {PLAYER_NINJA_DIE + 3, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][4]},
        {PLAYER_NINJA_DIE + 4, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][5]},
        {PLAYER_NINJA_DIE + 5, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][6]},
        {PLAYER_NINJA_DIE + 6, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][7]},
        {PLAYER_NINJA_DIE + 7, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][8]},
        {PLAYER_NINJA_DIE + 8, 0 | SF_QUICK_CALL, QueueFloorBlood, &s_PlayerDeath[4][9]},
        {PLAYER_NINJA_DIE + 8, NINJA_DIE_RATE, NullAnimator, &s_PlayerDeath[4][9]},
    },
};

STATEp sg_PlayerDeath[] =
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


STATEp sg_PlayerNinjaSword[] =
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


STATEp sg_PlayerNinjaPunch[] =
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


STATEp sg_PlayerNinjaFly[] =
{
    s_PlayerNinjaFly[0],
    s_PlayerNinjaFly[1],
    s_PlayerNinjaFly[2],
    s_PlayerNinjaFly[3],
    s_PlayerNinjaFly[4]
};

/////////////////////////////////////////////////////////////////////////////

void
DoPlayerSpriteThrow(PLAYERp pp)
{
    if (!TEST(pp->Flags, PF_DIVING|PF_FLYING|PF_CRAWLING))
    {
        if (pp->CurWpn == pp->Wpn[WPN_SWORD] && User[pp->PlayerSprite]->Rot != sg_PlayerNinjaSword)
            NewStateGroup(pp->PlayerSprite, sg_PlayerNinjaSword);
        else
            //if (pp->CurWpn == pp->Wpn[WPN_FIST] && User[pp->PlayerSprite]->Rot != sg_PlayerNinjaPunch)
            NewStateGroup(pp->PlayerSprite, sg_PlayerNinjaPunch);
        //else
        //    NewStateGroup(pp->PlayerSprite, sg_PlayerNinjaThrow);
    }
}

int
DoPlayerSpriteReset(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    PLAYERp pp;

    if (!u->PlayerP)
        return 0;

    pp = u->PlayerP;

    // need to figure out what frames to put sprite into
    if (pp->DoPlayerAction == DoPlayerCrawl)
        NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Crawl);
    else
    {
        if (TEST(pp->Flags, PF_PLAYER_MOVED))
            NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Run);
        else
            NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Stand);
    }

    return 0;
}

int
SetVisHigh(void)
{
//    g_visibility = NormalVisibility>>1;
    return 0;
}

int
SetVisNorm(void)
{
//    g_visibility = NormalVisibility;
    return 0;
}

void pSetVisNorm(PANEL_SPRITEp psp)
{
//    SetVisNorm();
}

short
GetDeltaAngle(short ang1, short ang2)
{
    // Look at the smaller angle if > 1024 (180 degrees)
    if (labs(ang1 - ang2) > 1024)
    {
        if (ang1 <= 1024)
            ang1 += 2048;

        if (ang2 <= 1024)
            ang2 += 2048;
    }

    //if (ang1 - ang2 == -1024)
    //    return(1024);

    return ang1 - ang2;

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

SWBOOL
FAFcansee(int32_t xs, int32_t ys, int32_t zs, int16_t sects,
          int32_t xe, int32_t ye, int32_t ze, int16_t secte);

int
DoPickTarget(SPRITEp sp, uint32_t max_delta_ang, SWBOOL skip_targets)
{
#define PICK_DIST 40000L

    short i, nexti, angle2, delta_ang;
    int dist, zh;
    SPRITEp ep;
    USERp eu;
    int16_t* shp;
    USERp u = User[sp - sprite];
    int ezh, ezhl, ezhm;
    unsigned ndx;
    TARGET_SORTp ts;
    int ang_weight, dist_weight;

    // !JIM! Watch out for max_delta_ang of zero!
    if (max_delta_ang == 0) max_delta_ang = 1;

    TargetSortCount = 0;
    TargetSort[0].sprite_num = -1;

    for (shp = StatDamageList; shp < &StatDamageList[SIZ(StatDamageList)]; shp++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[*shp], i, nexti)
        {
            ep = &sprite[i];
            eu = User[i];

            // don't pick yourself
            if (i == (sp - sprite))
                continue;

            if (skip_targets != 2) // Used for spriteinfo mode
            {
                if (skip_targets && TEST(eu->Flags, SPR_TARGETED))
                    continue;

                // don't pick a dead player
                if (eu->PlayerP && TEST(eu->PlayerP->Flags, PF_DEAD))
                    continue;
            }

            // Only look at closest ones
            //if ((dist = Distance(sp->x, sp->y, ep->x, ep->y)) > PICK_DIST)
            if ((dist = FindDistance3D(sp->x - ep->x, sp->y - ep->y, (sp->z - ep->z)>>4)) > PICK_DIST)
                continue;

            if (skip_targets != 2) // Used for spriteinfo mode
            {
                // don't set off mine
                if (!TEST(ep->extra, SPRX_PLAYER_OR_ENEMY))
                    continue;
            }

            // Get the angle to the player
            angle2 = NORM_ANGLE(getangle(ep->x - sp->x, ep->y - sp->y));

            // Get the angle difference
            // delta_ang = labs(pp->pang - angle2);

            delta_ang = labs(GetDeltaAngle(sp->ang, angle2));

            // If delta_ang not in the range skip this one
            if (delta_ang > (int)max_delta_ang)
                continue;

            if (u && u->PlayerP)
                zh = u->PlayerP->posz;
            else
                zh = SPRITEp_TOS(sp) + DIV4(SPRITEp_SIZE_Z(sp));

            ezh = SPRITEp_TOS(ep) + DIV4(SPRITEp_SIZE_Z(ep));
            ezhm = SPRITEp_TOS(ep) + DIV2(SPRITEp_SIZE_Z(ep));
            ezhl = SPRITEp_BOS(ep) - DIV4(SPRITEp_SIZE_Z(ep));

            // If you can't see 'em you can't shoot 'em
            if (!FAFcansee(sp->x, sp->y, zh, sp->sectnum, ep->x, ep->y, ezh, ep->sectnum) &&
                !FAFcansee(sp->x, sp->y, zh, sp->sectnum, ep->x, ep->y, ezhm, ep->sectnum) &&
                !FAFcansee(sp->x, sp->y, zh, sp->sectnum, ep->x, ep->y, ezhl, ep->sectnum)
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
            ts->sprite_num = i;
            ts->dang = delta_ang;
            ts->dist = dist;
            // gives a value between 0 and 65535
            ang_weight = ((max_delta_ang - ts->dang)<<16)/max_delta_ang;
            // gives a value between 0 and 65535
            dist_weight = ((DIV2(PICK_DIST) - DIV2(ts->dist))<<16)/DIV2(PICK_DIST);
            //weighted average
            ts->weight = (ang_weight + dist_weight*4)/5;

            TargetSortCount++;
            if (TargetSortCount >= SIZ(TargetSort))
                TargetSortCount = SIZ(TargetSort);
        }
    }

    if (TargetSortCount > 1)
        qsort(&TargetSort, TargetSortCount, sizeof(TARGET_SORT), CompareTarget);

    return TargetSort[0].sprite_num;
}

void
DoPlayerResetMovement(PLAYERp pp)
{
    pp->xvect = pp->oxvect = 0;
    pp->yvect = pp->oxvect = 0;
    pp->slide_xvect = 0;
    pp->slide_yvect = 0;
    pp->drive_angvel = 0;
    pp->drive_oangvel = 0;
    RESET(pp->Flags, PF_PLAYER_MOVED);
}

void
DoPlayerTeleportPause(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp sp = pp->SpriteP;

    // set this so we don't get stuck in teleporting loop
    pp->lastcursectnum = pp->cursectnum;

    if ((u->WaitTics-=synctics) <= 0)
    {
        //RESET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
        RESET(pp->Flags2, PF2_TELEPORTED);
        DoPlayerResetMovement(pp);
        DoPlayerBeginRun(pp);
        return;
    }

    //sp->shade -= 2;
    //if (sp->shade <= 0)
    //    sp->shade = 0;

    //DoPlayerBob(pp);
}

void
DoPlayerTeleportToSprite(PLAYERp pp, SPRITEp sp)
{
    int cz, fz;

    pp->pang = pp->oang = sp->ang;
    pp->posx = pp->oposx = pp->oldposx = sp->x;
    pp->posy = pp->oposy = pp->oldposy = sp->y;

    //getzsofslope(sp->sectnum, pp->posx, pp->posy, &cz, &fz);
    //pp->posz = pp->oposz = fz - PLAYER_HEIGHT;

    pp->posz = pp->oposz = sp->z - PLAYER_HEIGHT;

    COVERupdatesector(pp->posx, pp->posy, &pp->cursectnum);
    //pp->lastcursectnum = pp->cursectnum;
    SET(pp->Flags2, PF2_TELEPORTED);
}

void
DoPlayerTeleportToOffset(PLAYERp pp)
{
    int fz,cz;

    pp->oposx = pp->oldposx = pp->posx;
    pp->oposy = pp->oldposy = pp->posy;

    COVERupdatesector(pp->posx, pp->posy, &pp->cursectnum);
    //pp->lastcursectnum = pp->cursectnum;
    SET(pp->Flags2, PF2_TELEPORTED);
}

void
DoSpawnTeleporterEffect(SPRITEp sp)
{
    extern STATE s_TeleportEffect[];
    short effect;
    USERp eu;
    int nx, ny;
    SPRITEp ep;

    nx = MOVEx(512L, sp->ang);
    ny = MOVEy(512L, sp->ang);

    nx += sp->x;
    ny += sp->y;

    effect = SpawnSprite(STAT_MISSILE, 0, s_TeleportEffect, sp->sectnum,
                         nx, ny, SPRITEp_TOS(sp) + Z(16),
                         sp->ang, 0);

    ep = &sprite[effect];
    eu = User[effect];

    setspritez(effect, (vec3_t *)ep);

    ep->shade = -40;
    ep->xrepeat = ep->yrepeat = 42;
    SET(ep->cstat, CSTAT_SPRITE_YCENTER);
    RESET(ep->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    SET(ep->cstat, CSTAT_SPRITE_ALIGNMENT_WALL);
    //ep->ang = NORM_ANGLE(ep->ang + 512);
}

void
DoSpawnTeleporterEffectPlace(SPRITEp sp)
{
    extern STATE s_TeleportEffect[];
    short effect;
    USERp eu;
    int nx, ny;
    SPRITEp ep;

    effect = SpawnSprite(STAT_MISSILE, 0, s_TeleportEffect, sp->sectnum,
                         sp->x, sp->y, SPRITEp_TOS(sp) + Z(16),
                         sp->ang, 0);

    ep = &sprite[effect];
    eu = User[effect];

    setspritez(effect, (vec3_t *)ep);

    ep->shade = -40;
    ep->xrepeat = ep->yrepeat = 42;
    SET(ep->cstat, CSTAT_SPRITE_YCENTER);
    RESET(ep->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    SET(ep->cstat, CSTAT_SPRITE_ALIGNMENT_WALL);
}

void
DoPlayerWarpTeleporter(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite], eu;
    SPRITEp sp = pp->SpriteP, ep;
    short pnum;
    SPRITEp sp_warp;

#if 0
    TAG 2 = match
            TAG 3 = Type
                    Sprite - 0,32 always teleports you to the center at the angle the sprite is facing
    Offset - 1 always teleports you by the offset.Does not touch the angle
    TAG 4 = angle
            TAG 5 to 8 = random match locations
#endif


    if ((sp_warp = Warp(&pp->posx, &pp->posy, &pp->posz, &pp->cursectnum)) == NULL)
        return;

    switch (SP_TAG3(sp_warp))
    {
    case 1:
        DoPlayerTeleportToOffset(pp);
        UpdatePlayerSprite(pp);
        break;
    default:
        DoPlayerTeleportToSprite(pp, sp_warp);

        PlaySound(DIGI_TELEPORT, &pp->posx, &pp->posy, &pp->posz, v3df_none);

        DoPlayerResetMovement(pp);

        u->WaitTics = 30;
        //sp->shade =
        //SET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
        DoPlayerBeginRun(pp);
        //DoPlayerStand(pp);
        pp->DoPlayerAction = DoPlayerTeleportPause;

        NewStateGroup(pp->PlayerSprite, User[pp->PlayerSprite]->ActorActionSet->Stand);

        UpdatePlayerSprite(pp);
        DoSpawnTeleporterEffect(sp);

        TRAVERSE_CONNECT(pnum)
        {
            if (pnum != pp - Player)
            {
                PLAYERp npp = &Player[pnum];

                // if someone already standing there
                if (npp->cursectnum == pp->cursectnum)
                {
                    PlayerUpdateHealth(npp, -User[npp->PlayerSprite]->Health);  // Make sure he dies!
                    // telefraged by teleporting player
                    //PlayerCheckDeath(npp, npp->PlayerSprite);
                    PlayerCheckDeath(npp, pp->PlayerSprite);
                }
            }
        }

        break;
    }

    u->ox = sp->x;
    u->oy = sp->y;
    u->oz = sp->z;
}

void
DoPlayerSetWadeDepth(PLAYERp pp)
{
    SECTORp sectp;

    pp->WadeDepth = 0;

    if (pp->lo_sectp)
        sectp = pp->lo_sectp;
    else
        return;

    if (TEST(sectp->extra, SECTFX_SINK))
    {
        // make sure your even in the water
        if (pp->posz + PLAYER_HEIGHT > pp->lo_sectp->floorz - Z(SectUser[pp->lo_sectp - sector]->depth))
            pp->WadeDepth = SectUser[pp->lo_sectp - sector]->depth;
    }
}


void
DoPlayerHeight(PLAYERp pp)
{
    int diff;

    diff = pp->posz - (pp->loz - PLAYER_HEIGHT);

    pp->posz = pp->posz - (DIV4(diff) + DIV8(diff));
}

void
DoPlayerJumpHeight(PLAYERp pp)
{
    if (pp->lo_sectp && TEST(pp->lo_sectp->extra, SECTFX_DYNAMIC_AREA))
    {
        if (pp->posz + PLAYER_HEIGHT > pp->loz)
        {
            pp->posz = pp->loz - PLAYER_HEIGHT;
            DoPlayerBeginRun(pp);
        }
    }
}

void
DoPlayerCrawlHeight(PLAYERp pp)
{
    int diff;

    diff = pp->posz - (pp->loz - PLAYER_CRAWL_HEIGHT);
    pp->posz = pp->posz - (DIV4(diff) + DIV8(diff));
}

void
DoPlayerTurn(PLAYERp pp)
{
    int doubvel;
    short angvel;

#define TURN_SHIFT 2

    if (!TEST(pp->Flags, PF_TURN_180))
    {
        if (TEST_SYNC_KEY(pp, SK_TURN_180))
        {
            if (FLAG_KEY_PRESSED(pp, SK_TURN_180))
            {
                short delta_ang;

                FLAG_KEY_RELEASE(pp, SK_TURN_180);

                pp->turn180_target = NORM_ANGLE(pp->pang + 1024);

                // make the first turn in the clockwise direction
                // the rest will follow
                delta_ang = GetDeltaAngle(pp->turn180_target, pp->pang);
                pp->pang = NORM_ANGLE(pp->pang + (labs(delta_ang) >> TURN_SHIFT));

                SET(pp->Flags, PF_TURN_180);
            }
        }
        else
        {
            FLAG_KEY_RESET(pp, SK_TURN_180);
        }
    }

    if (TEST(pp->Flags, PF_TURN_180))
    {
        short delta_ang;

        delta_ang = GetDeltaAngle(pp->turn180_target, pp->pang);
        pp->pang = NORM_ANGLE(pp->pang + (delta_ang >> TURN_SHIFT));

        sprite[pp->PlayerSprite].ang = pp->pang;
        if (!Prediction)
        {
            if (pp->PlayerUnderSprite >= 0)
                sprite[pp->PlayerUnderSprite].ang = pp->pang;
        }

        // get new delta to see how close we are
        delta_ang = GetDeltaAngle(pp->turn180_target, pp->pang);

        if (labs(delta_ang) < (3<<TURN_SHIFT))
        {
            pp->pang = pp->turn180_target;
            RESET(pp->Flags, PF_TURN_180);
        }
        else
            return;
    }

    angvel = pp->input.angvel * PLAYER_TURN_SCALE;

    if (angvel != 0)
    {
        // running is not handled here now
        angvel += DIV4(angvel);

        pp->pang += DIV32(angvel * synctics);
        pp->pang = NORM_ANGLE(pp->pang);

        // update players sprite angle
        // NOTE: It's also updated in UpdatePlayerSprite, but needs to be
        // here to cover
        // all cases.
        sprite[pp->PlayerSprite].ang = pp->pang;
        if (!Prediction)
        {
            if (pp->PlayerUnderSprite >= 0)
                sprite[pp->PlayerUnderSprite].ang = pp->pang;
        }

    }
}

void
DoPlayerTurnBoat(PLAYERp pp)
{
    int angvel;
    int angslide;
    SECTOR_OBJECTp sop = pp->sop;

    if (sop->drive_angspeed)
    {
        pp->drive_oangvel = pp->drive_angvel;
        pp->drive_angvel = mulscale16(pp->input.angvel, sop->drive_angspeed);

        angslide = sop->drive_angslide;
        pp->drive_angvel = (pp->drive_angvel + (pp->drive_oangvel*(angslide-1)))/angslide;

        angvel = pp->drive_angvel;
    }
    else
    {
        angvel = pp->input.angvel * PLAYER_TURN_SCALE;
        angvel += angvel - DIV4(angvel);
        angvel = DIV32(angvel * synctics);
    }

    if (angvel != 0)
    {
        pp->pang = NORM_ANGLE(pp->pang + angvel);
        sprite[pp->PlayerSprite].ang = pp->pang;
    }
}

void
DoPlayerTurnTank(PLAYERp pp, int z, int floor_dist)
{
    int angvel;
    SECTOR_OBJECTp sop = pp->sop;

    if (sop->drive_angspeed)
    {
        int angslide;

        pp->drive_oangvel = pp->drive_angvel;
        pp->drive_angvel = mulscale16(pp->input.angvel, sop->drive_angspeed);

        angslide = sop->drive_angslide;
        pp->drive_angvel = (pp->drive_angvel + (pp->drive_oangvel*(angslide-1)))/angslide;

        angvel = pp->drive_angvel;
    }
    else
    {
        angvel = DIV8(pp->input.angvel * synctics);
    }

    if (angvel != 0)
    {
        if (MultiClipTurn(pp, NORM_ANGLE(pp->pang + angvel), z, floor_dist))
        {
            pp->pang = NORM_ANGLE(pp->pang + angvel);
            sprite[pp->PlayerSprite].ang = pp->pang;
        }
    }
}

void
DoPlayerTurnTankRect(PLAYERp pp, int *x, int *y, int *ox, int *oy)
{
    int angvel;
    SECTOR_OBJECTp sop = pp->sop;

    if (sop->drive_angspeed)
    {
        int angslide;

        pp->drive_oangvel = pp->drive_angvel;
        pp->drive_angvel = mulscale16(pp->input.angvel, sop->drive_angspeed);

        angslide = sop->drive_angslide;
        pp->drive_angvel = (pp->drive_angvel + (pp->drive_oangvel*(angslide-1)))/angslide;

        angvel = pp->drive_angvel;
    }
    else
    {
        angvel = DIV8(pp->input.angvel * synctics);
    }

    if (angvel != 0)
    {
        if (RectClipTurn(pp, NORM_ANGLE(pp->pang + angvel), x, y, ox, oy))
        {
            pp->pang = NORM_ANGLE(pp->pang + angvel);
            sprite[pp->PlayerSprite].ang = pp->pang;
        }
    }
}

void
DoPlayerTurnTurret(PLAYERp pp)
{
    int angvel;
    short new_ang;
    short diff;
    SECTOR_OBJECTp sop = pp->sop;
    SW_PACKET last_input;
    int fifo_ndx;

    if (!Prediction)
    {
        // this code looks at the fifo to get the last value for comparison
        fifo_ndx = (movefifoplc-2) & (MOVEFIFOSIZ - 1);
        last_input = pp->inputfifo[fifo_ndx];

        if (pp->input.angvel && !last_input.angvel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else if (!pp->input.angvel && last_input.angvel)
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
    }

    if (sop->drive_angspeed)
    {
        int angslide;

        pp->drive_oangvel = pp->drive_angvel;
        pp->drive_angvel = mulscale16(pp->input.angvel, sop->drive_angspeed);

        angslide = sop->drive_angslide;
        pp->drive_angvel = (pp->drive_angvel + (pp->drive_oangvel*(angslide-1)))/angslide;

        angvel = pp->drive_angvel;
    }
    else
    {
        angvel = DIV4(pp->input.angvel * synctics);
    }

    if (angvel != 0)
    {
        new_ang = NORM_ANGLE(pp->pang + angvel);

        if (sop->limit_ang_center >= 0)
        {
            diff = GetDeltaAngle(new_ang, sop->limit_ang_center);

            if (labs(diff) >= sop->limit_ang_delta)
            {
                if (diff < 0)
                    new_ang = sop->limit_ang_center - sop->limit_ang_delta;
                else
                    new_ang = sop->limit_ang_center + sop->limit_ang_delta;

            }
        }

        pp->pang = new_ang;
        sprite[pp->PlayerSprite].ang = pp->pang;
    }
}

void SlipSlope(PLAYERp pp)
{
    short wallptr = sector[pp->cursectnum].wallptr;
    short ang;
    SECT_USERp sectu = SectUser[pp->cursectnum];

    if (!sectu || !TEST(sectu->flags, SECTFU_SLIDE_SECTOR) || !TEST(sector[pp->cursectnum].floorstat, FLOOR_STAT_SLOPE))
        return;

    ang = getangle(wall[wall[wallptr].point2].x - wall[wallptr].x, wall[wall[wallptr].point2].y - wall[wallptr].y);

    ang = NORM_ANGLE(ang + 512);

    pp->xvect += mulscale(sintable[NORM_ANGLE(ang + 512)], sector[pp->cursectnum].floorheinum, sectu->speed);
    pp->yvect += mulscale(sintable[ang], sector[pp->cursectnum].floorheinum, sectu->speed);
}

void
PlayerAutoLook(PLAYERp pp)
{
    int x,y,k,j;
    short tempsect;


    if (!TEST(pp->Flags, PF_FLYING|PF_SWIMMING|PF_DIVING|PF_CLIMBING|PF_JUMPING|PF_FALLING))
    {
        if (!TEST(pp->Flags, PF_MOUSE_AIMING_ON) && TEST(sector[pp->cursectnum].floorstat, FLOOR_STAT_SLOPE)) // If the floor is sloped
        {
            // Get a point, 512 units ahead of player's position
            x = pp->posx + (sintable[(pp->pang + 512) & 2047] >> 5);
            y = pp->posy + (sintable[pp->pang & 2047] >> 5);
            tempsect = pp->cursectnum;
            COVERupdatesector(x, y, &tempsect);

            if (tempsect >= 0)              // If the new point is inside a valid
            // sector...
            {
                // Get the floorz as if the new (x,y) point was still in
                // your sector
                j = getflorzofslope(pp->cursectnum, pp->posx, pp->posy);
                k = getflorzofslope(pp->cursectnum, x, y);

                // If extended point is in same sector as you or the slopes
                // of the sector of the extended point and your sector match
                // closely (to avoid accidently looking straight out when
                // you're at the edge of a sector line) then adjust horizon
                // accordingly
                if ((pp->cursectnum == tempsect) ||
                    (klabs(getflorzofslope(tempsect, x, y) - k) <= (4 << 8)))
                {
                    pp->horizoff += (((j - k) * 160) >> 16);
                }
            }
        }
    }

    if (TEST(pp->Flags, PF_CLIMBING))
    {
        // tilt when climbing but you can't even really tell it
        if (pp->horizoff < 100)
            pp->horizoff += (((100 - pp->horizoff) >> 3) + 1);
    }
    else
    {
        // Make horizoff grow towards 0 since horizoff is not modified when
        // you're not on a slope
        if (pp->horizoff > 0)
            pp->horizoff -= ((pp->horizoff >> 3) + 1);
        if (pp->horizoff < 0)
            pp->horizoff += (((-pp->horizoff) >> 3) + 1);
    }
}

extern int PlaxCeilGlobZadjust, PlaxFloorGlobZadjust;
void
DoPlayerHorizon(PLAYERp pp)
{
    int i;
#define HORIZ_SPEED (16)

//    //DSPRINTF(ds,"pp->horizoff, %d", pp->horizoff);
//    MONO_PRINT(ds);

    PlayerAutoLook(pp);

    if (pp->input.aimvel)
    {
        pp->horizbase += pp->input.aimvel;
        SET(pp->Flags, PF_LOCK_HORIZ | PF_LOOKING);
    }

    if (TEST_SYNC_KEY(pp, SK_CENTER_VIEW))
    {
//        if (TEST(pp->Flags, PF_MOUSE_AIMING_ON))
        {
            pp->horiz = pp->horizbase = 100;
            pp->horizoff = 0;
        }
//        else
        {
            //gs.MouseAimingOn = FALSE;
//            RESET(pp->Flags, PF_LOCK_HORIZ);
//            SET(pp->Flags, PF_LOOKING);
        }
    }

    // this is the locked type
    if (TEST_SYNC_KEY(pp, SK_SNAP_UP) || TEST_SYNC_KEY(pp, SK_SNAP_DOWN))
    {
        // set looking because player is manually looking
        SET(pp->Flags, PF_LOCK_HORIZ | PF_LOOKING);

        // adjust pp->horizon negative
        if (TEST_SYNC_KEY(pp, SK_SNAP_DOWN))
            pp->horizbase -= (HORIZ_SPEED/2);

        // adjust pp->horizon positive
        if (TEST_SYNC_KEY(pp, SK_SNAP_UP))
            pp->horizbase += (HORIZ_SPEED/2);
    }


    // this is the unlocked type
    if (TEST_SYNC_KEY(pp, SK_LOOK_UP) || TEST_SYNC_KEY(pp, SK_LOOK_DOWN))
    {
        RESET(pp->Flags, PF_LOCK_HORIZ);
        SET(pp->Flags, PF_LOOKING);

        // adjust pp->horizon negative
        if (TEST_SYNC_KEY(pp, SK_LOOK_DOWN))
            pp->horizbase -= HORIZ_SPEED;

        // adjust pp->horizon positive
        if (TEST_SYNC_KEY(pp, SK_LOOK_UP))
            pp->horizbase += HORIZ_SPEED;
    }


    if (!TEST(pp->Flags, PF_LOCK_HORIZ))
    {
        if (!(TEST_SYNC_KEY(pp, SK_LOOK_UP) || TEST_SYNC_KEY(pp, SK_LOOK_DOWN)))
        {
            // not pressing the pp->horiz keys
            if (pp->horizbase != 100)
            {

                // move pp->horiz back to 100
                for (i = 1; i; i--)
                {
                    // this formula does not work for pp->horiz = 101-103
                    pp->horizbase += 25 - (pp->horizbase >> 2);
                }
            }
            else
            {
                // not looking anymore because pp->horiz is back at 100
                RESET(pp->Flags, PF_LOOKING);
            }
        }
    }

#if 1
    // bound the base
    pp->horizbase = max(pp->horizbase, PLAYER_HORIZ_MIN);
    pp->horizbase = min(pp->horizbase, PLAYER_HORIZ_MAX);

    // bound adjust horizoff
    if (pp->horizbase + pp->horizoff < PLAYER_HORIZ_MIN)
        pp->horizoff = PLAYER_HORIZ_MIN - pp->horizbase;
    else if (pp->horizbase + pp->horizoff > PLAYER_HORIZ_MAX)
        pp->horizoff = PLAYER_HORIZ_MAX - pp->horizbase;

    ////DSPRINTF(ds,"base %d, off %d, base + off %d",pp->horizbase, pp->horizoff, pp->horizbase + pp->horizoff);
    //MONO_PRINT(ds);

    // add base and offsets
    pp->horiz = pp->horizbase + pp->horizoff;
#else
    if (pp->horizbase + pp->horizoff < PLAYER_HORIZ_MIN)
        pp->horizbase += HORIZ_SPEED;
    else if (pp->horizbase + pp->horizoff > PLAYER_HORIZ_MAX)
        pp->horizbase -= HORIZ_SPEED;

    pp->horiz = pp->horizbase + pp->horizoff;
#endif

}

void
DoPlayerBob(PLAYERp pp)
{
    extern uint32_t MoveThingsCount;
    int dist;
    int amt;

    dist = 0;

    dist = Distance(pp->posx, pp->posy, pp->oldposx, pp->oldposy);

    if (dist > 512)
        dist = 0;

    // if running make a longer stride
    if (TEST_SYNC_KEY(pp, SK_RUN) || TEST(pp->Flags, PF_LOCK_RUN))
    {
        //amt = 10;
        amt = 12;
        amt = mulscale16(amt, dist<<8);

        dist = mulscale16(dist, 26000);
        // controls how fast you move through the sin table
        pp->bcnt += dist;

        // wrap bcnt
        pp->bcnt &= 2047;

        // move pp->horiz up and down from 100 using sintable
        //pp->bob_z = Z((8 * sintable[pp->bcnt]) >> 14);
        pp->bob_z = mulscale14(Z(amt),sintable[pp->bcnt]);
    }
    else
    {
        amt = 5;
        amt = mulscale16(amt, dist<<9);

        dist = mulscale16(dist, 32000);
        // controls how fast you move through the sin table
        pp->bcnt += dist;

        // wrap bcnt
        pp->bcnt &= 2047;

        // move pp->horiz up and down from 100 using sintable
        //pp->bob_z = Z((4 * sintable[pp->bcnt]) >> 14);
        pp->bob_z = mulscale14(Z(amt),sintable[pp->bcnt]);
    }
}

void
DoPlayerBeginRecoil(PLAYERp pp, short pix_amt)
{
#if 0
    return;
#else
    SET(pp->Flags, PF_RECOIL);

    pp->recoil_amt = pix_amt;
    pp->recoil_speed = 80;
    pp->recoil_ndx = 0;
    pp->recoil_horizoff = 0;
#endif
}

void
DoPlayerRecoil(PLAYERp pp)
{
    int dist;

    // controls how fast you move through the sin table
    pp->recoil_ndx += pp->recoil_speed;

    if (sintable[pp->recoil_ndx] < 0)
    {
        RESET(pp->Flags, PF_RECOIL);
        pp->recoil_horizoff = 0;
        return;
    }

    // move pp->horiz up and down
    pp->recoil_horizoff = ((pp->recoil_amt * sintable[pp->recoil_ndx]) >> 14);
}



// for wading
void
DoPlayerSpriteBob(PLAYERp pp, short player_height, short bob_amt, short bob_speed)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp sp = pp->SpriteP;

    pp->bob_ndx = (pp->bob_ndx + (synctics << bob_speed)) & 2047;

    pp->bob_amt = ((bob_amt * (int) sintable[pp->bob_ndx]) >> 14);

    sp->z = (pp->posz + player_height) + pp->bob_amt;
}

void
UpdatePlayerUnderSprite(PLAYERp pp)
{
    SPRITEp over_sp = pp->SpriteP;
    USERp over_u = User[pp->PlayerSprite];

    SPRITEp sp;
    USERp u;
    short SpriteNum;

    int water_level_z, zdiff;
    SWBOOL above_water, in_dive_area;

    if (Prediction)
        return;

    ASSERT(over_sp);
    ASSERT(over_u);

    // dont bother spawning if you ain't really in the water
    //water_level_z = sector[over_sp->sectnum].floorz - Z(pp->WadeDepth);
    water_level_z = sector[over_sp->sectnum].floorz; // - Z(pp->WadeDepth);

    // if not below water
    above_water = (SPRITEp_BOS(over_sp) <= water_level_z);
    in_dive_area = SpriteInDiveArea(over_sp);

    // if not in dive area OR (in dive area AND above the water) - Kill it
    if (!in_dive_area || (in_dive_area && above_water))
    {

        // if under sprite exists and not in a dive area - Kill it
        if (pp->PlayerUnderSprite >= 0)
        {
            KillSprite(pp->PlayerUnderSprite);
            pp->PlayerUnderSprite = -1;
            pp->UnderSpriteP = NULL;
        }
        return;
    }
    else
    {
        // if in a dive area and a under sprite does not exist - create it
        if (pp->PlayerUnderSprite < 0)
        {
            SpawnPlayerUnderSprite(pp);
        }
    }

    sp = pp->UnderSpriteP;
    u = User[pp->PlayerUnderSprite];

    SpriteNum = pp->PlayerUnderSprite;

    sp->x = over_sp->x;
    sp->y = over_sp->y;
    sp->z = over_sp->z;
    changespritesect(SpriteNum, over_sp->sectnum);

    SpriteWarpToUnderwater(sp);

    // find z water level of the top sector
    // diff between the bottom of the upper sprite and the water level
    zdiff = SPRITEp_BOS(over_sp) - water_level_z;

    // add diff to ceiling
    sp->z = sector[sp->sectnum].ceilingz + zdiff;

    u->State = over_u->State;
    u->Rot = over_u->Rot;
    u->StateStart = over_u->StateStart;

    sp->picnum = over_sp->picnum;
}


void
UpdatePlayerSprite(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;

    // Update sprite representation of player

    sp->x = pp->posx;
    sp->y = pp->posy;

    // there are multiple death functions
    if (TEST(pp->Flags, PF_DEAD))
    {
        changespritesect(pp->PlayerSprite, pp->cursectnum);
        sprite[pp->PlayerSprite].ang = pp->pang;
        UpdatePlayerUnderSprite(pp);
        return;
    }

    if (pp->sop_control)
    {
        sp->z = sector[pp->cursectnum].floorz;
        changespritesect(pp->PlayerSprite, pp->cursectnum);
    }
    else if (pp->DoPlayerAction == DoPlayerCrawl)
    {
        sp->z = pp->posz + PLAYER_CRAWL_HEIGHT;
        changespritesect(pp->PlayerSprite, pp->cursectnum);
    }
#if 0
    else if (pp->DoPlayerAction == DoPlayerSwim)
    {
        sp->z = pp->loz - Z(pp->WadeDepth) + Z(1);
        changespritesect(pp->PlayerSprite, pp->cursectnum);
    }
#endif
    else if (pp->DoPlayerAction == DoPlayerWade)
    {
        sp->z = pp->posz + PLAYER_HEIGHT;
        changespritesect(pp->PlayerSprite, pp->cursectnum);

        if (pp->WadeDepth > Z(29))
        {
            DoPlayerSpriteBob(pp, PLAYER_HEIGHT, Z(3), 3);
        }
    }
    else if (pp->DoPlayerAction == DoPlayerDive)
    {
        // bobbing and sprite position taken care of in DoPlayerDive
        sp->z = pp->posz + Z(10);
        changespritesect(pp->PlayerSprite, pp->cursectnum);
    }
    else if (pp->DoPlayerAction == DoPlayerClimb)
    {
        sp->z = pp->posz + Z(17);

        // move it forward a bit to look like its on the ladder
        //sp->x += MOVEx(256+64, sp->ang);
        //sp->y += MOVEy(256+64, sp->ang);

        changespritesect(pp->PlayerSprite, pp->cursectnum);
    }
    else if (pp->DoPlayerAction == DoPlayerFly)
    {
        // sp->z = pp->posz + PLAYER_HEIGHT;
        // bobbing and sprite position taken care of in DoPlayerFly
        //sp->z = pp->posz + PLAYER_HEIGHT;
        //DoPlayerSpriteBob(pp, PLAYER_HEIGHT, PLAYER_FLY_BOB_AMT, 3);
        DoPlayerSpriteBob(pp, PLAYER_HEIGHT, Z(6), 3);
        changespritesect(pp->PlayerSprite, pp->cursectnum);
    }
    else if (pp->DoPlayerAction == DoPlayerJump || pp->DoPlayerAction == DoPlayerFall || pp->DoPlayerAction == DoPlayerForceJump)
    {
        sp->z = pp->posz + PLAYER_HEIGHT;
        changespritesect(pp->PlayerSprite, pp->cursectnum);
    }
    else if (pp->DoPlayerAction == DoPlayerTeleportPause)
    {
        sp->z = pp->posz + PLAYER_HEIGHT;
        changespritesect(pp->PlayerSprite, pp->cursectnum);
    }
    else
    {
        sp->z = pp->loz;
        changespritesect(pp->PlayerSprite, pp->cursectnum);
    }

    UpdatePlayerUnderSprite(pp);

    sprite[pp->PlayerSprite].ang = pp->pang;
}

void
DoPlayerZrange(PLAYERp pp)
{
    int ceilhit, florhit;
    short bakcstat;

    // Don't let you fall if you're just slightly over a cliff
    // This function returns the highest and lowest z's
    // for an entire box, NOT just a point.  -Useful for clipping
    bakcstat = pp->SpriteP->cstat;
    RESET(pp->SpriteP->cstat, CSTAT_SPRITE_BLOCK);
    FAFgetzrange(pp->posx, pp->posy, pp->posz + Z(8), pp->cursectnum, &pp->hiz, &ceilhit, &pp->loz, &florhit, ((int)pp->SpriteP->clipdist<<2) - GETZRANGE_CLIP_ADJ, CLIPMASK_PLAYER);
    pp->SpriteP->cstat = bakcstat;

//  16384+sector (sector first touched) or
//  49152+spritenum (sprite first touched)

    pp->lo_sectp = pp->hi_sectp = NULL;
    pp->lo_sp = pp->hi_sp = NULL;

    if (TEST(ceilhit, 0xc000) == 49152)
    {
        pp->hi_sp = &sprite[ceilhit & 4095];
    }
    else
    {
        pp->hi_sectp = &sector[ceilhit & 4095];
    }

    if (TEST(florhit, 0xc000) == 49152)
    {
        pp->lo_sp = &sprite[florhit & 4095];

        // prevent player from standing on Zombies
        if (pp->lo_sp->statnum == STAT_ENEMY && User[pp->lo_sp - sprite]->ID == ZOMBIE_RUN_R0)
        {
            pp->lo_sectp = &sector[pp->lo_sp->sectnum];
            pp->loz = pp->lo_sp->z;
            pp->lo_sp = NULL;
        }
    }
    else
    {
        pp->lo_sectp = &sector[florhit & 4095];
    }
}

void
DoPlayerSlide(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    int ret;
    int push_ret;

    if ((pp->slide_xvect|pp->slide_yvect) == 0)
        return;

    if (pp->sop)
        return;

    pp->slide_xvect  = mulscale16(pp->slide_xvect, PLAYER_SLIDE_FRICTION);
    pp->slide_yvect  = mulscale16(pp->slide_yvect, PLAYER_SLIDE_FRICTION);

    if (labs(pp->slide_xvect) < 12800 && labs(pp->slide_yvect) < 12800)
        pp->slide_xvect = pp->slide_yvect = 0;

    push_ret = pushmove((vec3_t *)pp, &pp->cursectnum, ((int)pp->SpriteP->clipdist<<2), pp->ceiling_dist, pp->floor_dist, CLIPMASK_PLAYER);
    if (push_ret < 0)
    {
        if (!TEST(pp->Flags, PF_DEAD))
        {
            PlayerUpdateHealth(pp, -u->Health);  // Make sure he dies!
            PlayerCheckDeath(pp, -1);

            if (TEST(pp->Flags, PF_DEAD))
                return;
        }
        return;
    }
    ret = clipmove((vec3_t *)pp, &pp->cursectnum, pp->slide_xvect, pp->slide_yvect, ((int)pp->SpriteP->clipdist<<2), pp->ceiling_dist, pp->floor_dist, CLIPMASK_PLAYER);
    PlayerCheckValidMove(pp);
    push_ret = pushmove((vec3_t *)pp, &pp->cursectnum, ((int)pp->SpriteP->clipdist<<2), pp->ceiling_dist, pp->floor_dist, CLIPMASK_PLAYER);
    if (push_ret < 0)
    {
        if (!TEST(pp->Flags, PF_DEAD))
        {
            PlayerUpdateHealth(pp, -u->Health);  // Make sure he dies!
            PlayerCheckDeath(pp, -1);

            if (TEST(pp->Flags, PF_DEAD))
                return;
        }
        return;
    }
}

void PlayerMoveHitDebug(short ret)
{
    SPRITEp sp;
    extern SWBOOL DebugActor;

    switch (TEST(ret, HIT_MASK))
    {
    case HIT_SPRITE:
        sp = &sprite[NORM_SPRITE(ret)];
        //DSPRINTF(ds, "Hit a Sprite %d, stat %d ", sp-sprite, (short)sp->statnum);
        if (sp->statnum == STAT_MISSILE)
        {
            //DSPRINTF(ds, "Monster hit bullet %d, stat %d ", sp-sprite, (short)sp->statnum);
        }
        else
        {
            //DSPRINTF(ds, "Hit a Sprite %d, stat %d ", sp-sprite, (short)sp->statnum);
        }
        break;
    case HIT_WALL:
        //DSPRINTF(ds, "Hit a Wall %d    ", NORM_WALL(ret));
        break;
    case HIT_SECTOR:
        //DSPRINTF(ds, "Hit a Sector %d  ", NORM_SECTOR(ret));
        break;
    }

    MONO_PRINT(ds);
}

void PlayerCheckValidMove(PLAYERp pp)
{
    if (pp->cursectnum == -1)
    {
        static int count = 0;

#if DEBUG
        //DSPRINTF(ds,"PROBLEM!!!!! Player %d is not in a sector", pp - Player);
        MONO_PRINT(ds);
#endif

        pp->posx = pp->oldposx;
        pp->posy = pp->oldposy;
        pp->posz = pp->oldposz;
        pp->cursectnum = pp->lastcursectnum;

        // if stuck here for more than 10 seconds
        if (count++ > 40 * 10)
        {
            ASSERT(TRUE == FALSE);
        }
    }
}

void
MoveScrollMode2D(PLAYERp pp)
{
#define TURBOTURNTIME (120/8)
#define NORMALTURN   (12+6)
#define RUNTURN      (28)
#define PREAMBLETURN 3
#define NORMALKEYMOVE 35
#define MAXVEL       ((NORMALKEYMOVE*2)+10)
#define MAXSVEL      ((NORMALKEYMOVE*2)+10)
#define MAXANGVEL    100

    ControlInfo scrl_input;
    int32_t running;
    int32_t keymove;
    int32_t momx, momy;
    static int mfvel=0, mfsvel=0;
    extern SWBOOL HelpInputMode, ScrollMode2D;


    CONTROL_GetInput(&scrl_input);

    mfsvel = mfvel = 0;

    if (MenuInputMode || UsingMenus)
        return;

    // Recenter view if told
    if (BUTTON(gamefunc_Center_View))
    {
        Follow_posx = pp->posx;
        Follow_posy = pp->posy;
    }

    // Toggle follow map mode on/off
    if (BUTTON(gamefunc_Map_Follow_Mode) && !BUTTONHELD(gamefunc_Map_Follow_Mode))
    {
        ScrollMode2D = !ScrollMode2D;
        // Reset coords
        Follow_posx = pp->posx;
        Follow_posy = pp->posy;
    }

    running = BUTTON(gamefunc_Run) || TEST(pp->Flags, PF_LOCK_RUN);

    if (BUTTON(gamefunc_Strafe))
        mfsvel -= scrl_input.dyaw>>2;
    mfsvel -= scrl_input.dx>>2;
    mfvel = -scrl_input.dz>>2;

    if (running)
    {
        //keymove = NORMALKEYMOVE << 1;
        keymove = NORMALKEYMOVE;
    }
    else
    {
        keymove = NORMALKEYMOVE;
    }

    if (!HelpInputMode && !ConPanel)
    {
        if (BUTTON(gamefunc_Turn_Left))
        {
            mfsvel -= -keymove;
        }
        if (BUTTON(gamefunc_Turn_Right))
        {
            mfsvel -= keymove;
        }
    }

    if (!InputMode && !ConPanel)
    {
        if (BUTTON(gamefunc_Strafe_Left))
        {
            mfsvel += keymove;
        }

        if (BUTTON(gamefunc_Strafe_Right))
        {
            mfsvel += -keymove;
        }
    }

    if (!UsingMenus && !HelpInputMode && !ConPanel)
    {
        if (BUTTON(gamefunc_Move_Forward))
        {
            mfvel += keymove;
        }

        if (BUTTON(gamefunc_Move_Backward))
        {
            mfvel += -keymove;
        }
    }

    if (mfvel < -MAXVEL)
        mfvel = -MAXVEL;
    if (mfvel > MAXVEL)
        mfvel = MAXVEL;
    if (mfsvel < -MAXSVEL)
        mfsvel = -MAXSVEL;
    if (mfsvel > MAXSVEL)
        mfsvel = MAXSVEL;

    momx = mulscale9(mfvel, sintable[NORM_ANGLE(pp->pang + 512)]);
    momy = mulscale9(mfvel, sintable[NORM_ANGLE(pp->pang)]);

    momx += mulscale9(mfsvel, sintable[NORM_ANGLE(pp->pang)]);
    momy += mulscale9(mfsvel, sintable[NORM_ANGLE(pp->pang + 1536)]);

    //mfvel = momx;
    //mfsvel = momy;

    Follow_posx += momx;
    Follow_posy += momy;

    Follow_posx = max(Follow_posx, x_min_bound);
    Follow_posy = max(Follow_posy, y_min_bound);
    Follow_posx = min(Follow_posx, x_max_bound);
    Follow_posy = min(Follow_posy, y_max_bound);

}

void
DoPlayerMenuKeys(PLAYERp pp)
{
    if (!CommEnabled)
    {
        if (TEST_SYNC_KEY((pp), SK_AUTO_AIM))
        {
            if (FLAG_KEY_PRESSED(pp, SK_AUTO_AIM))
            {
                FLAG_KEY_RELEASE(pp, SK_AUTO_AIM);
                FLIP(pp->Flags, PF_AUTO_AIM);
            }
        }
        else
            FLAG_KEY_RESET(pp, SK_AUTO_AIM);
    }
}

void PlayerSectorBound(PLAYERp pp, int amt)
{
    int cz,fz;

    // player should never go into a sector

    // was getting some problems with this
    // when jumping onto hight sloped sectors

    // call this routine to make sure he doesn't
    // called from DoPlayerMove() but can be called
    // from anywhere it is needed

    getzsofslope(pp->cursectnum, pp->posx, pp->posy, &cz, &fz);

    if (pp->posz > fz - amt)
        pp->posz = fz - amt;

    if (pp->posz < cz + amt)
        pp->posz = cz + amt;

}

void
DoPlayerMove(PLAYERp pp)
{
    int i, ceilhit, florhit;
    short nvel,svel;
    int ret = 0;
    SWBOOL slow = FALSE;
    USERp u = User[pp->PlayerSprite];
    int friction;
    int oposz;
    int save_cstat;
    int push_ret = 0;
    void SlipSlope(PLAYERp pp);

    SlipSlope(pp);

    PLAYER_RUN_LOCK(pp);

    DoPlayerTurn(pp);

    pp->oldposx = pp->posx;
    pp->oldposy = pp->posy;
    pp->oldposz = pp->posz;
    pp->lastcursectnum = pp->cursectnum;

    if (PLAYER_MOVING(pp) == 0)
        RESET(pp->Flags, PF_PLAYER_MOVED);
    else
        SET(pp->Flags, PF_PLAYER_MOVED);

    DoPlayerSlide(pp);

    pp->oxvect = pp->xvect;
    pp->oyvect = pp->yvect;

    pp->xvect += ((pp->input.vel*synctics*2)<<6);
    pp->yvect += ((pp->input.svel*synctics*2)<<6);

    friction = pp->friction;
    if (!TEST(pp->Flags, PF_SWIMMING) && pp->WadeDepth)
    {
        friction -= pp->WadeDepth * 100L;
    }

    pp->xvect  = mulscale16(pp->xvect, friction);
    pp->yvect  = mulscale16(pp->yvect, friction);

    if (TEST(pp->Flags, PF_FLYING))
    {
        // do a bit of weighted averaging
        pp->xvect = (pp->xvect + (pp->oxvect*1))/2;
        pp->yvect = (pp->yvect + (pp->oyvect*1))/2;
    }
    else if (TEST(pp->Flags, PF_DIVING))
    {
        // do a bit of weighted averaging
        pp->xvect = (pp->xvect + (pp->oxvect*2))/3;
        pp->yvect = (pp->yvect + (pp->oyvect*2))/3;
    }

    if (labs(pp->xvect) < 12800 && labs(pp->yvect) < 12800)
        pp->xvect = pp->yvect = 0;

    pp->SpriteP->xvel = FindDistance2D(pp->xvect,pp->yvect)>>14;

    if (TEST(pp->Flags, PF_CLIP_CHEAT))
    {
        short sectnum=pp->cursectnum;
        pp->posx += pp->xvect >> 14;
        pp->posy += pp->yvect >> 14;
        COVERupdatesector(pp->posx, pp->posy, &sectnum);
        if (sectnum != -1)
            pp->cursectnum = sectnum;
    }
    else
    {
        push_ret = pushmove((vec3_t *)pp, &pp->cursectnum, ((int)pp->SpriteP->clipdist<<2), pp->ceiling_dist, pp->floor_dist - Z(16), CLIPMASK_PLAYER);

        if (push_ret < 0)
        {
            if (!TEST(pp->Flags, PF_DEAD))
            {
                PlayerUpdateHealth(pp, -u->Health);  // Make sure he dies!
                PlayerCheckDeath(pp, -1);

                if (TEST(pp->Flags, PF_DEAD))
                    return;
            }
        }

        save_cstat = pp->SpriteP->cstat;
        RESET(pp->SpriteP->cstat, CSTAT_SPRITE_BLOCK);
        COVERupdatesector(pp->posx, pp->posy, &pp->cursectnum);
        ret = clipmove((vec3_t *)pp, &pp->cursectnum, pp->xvect, pp->yvect, ((int)pp->SpriteP->clipdist<<2), pp->ceiling_dist, pp->floor_dist, CLIPMASK_PLAYER);
        pp->SpriteP->cstat = save_cstat;
        PlayerCheckValidMove(pp);

        push_ret = pushmove((vec3_t *)pp, &pp->cursectnum, ((int)pp->SpriteP->clipdist<<2), pp->ceiling_dist, pp->floor_dist - Z(16), CLIPMASK_PLAYER);
        if (push_ret < 0)
        {

            if (!TEST(pp->Flags, PF_DEAD))
            {
                PlayerUpdateHealth(pp, -u->Health);  // Make sure he dies!
                PlayerCheckDeath(pp, -1);

                if (TEST(pp->Flags, PF_DEAD))
                    return;
            }
        }
    }

    // check for warp - probably can remove from CeilingHit
    if (WarpPlane(&pp->posx, &pp->posy, &pp->posz, &pp->cursectnum))
        PlayerWarpUpdatePos(pp);

    DoPlayerZrange(pp);

    //PlayerSectorBound(pp, Z(1));

    DoPlayerSetWadeDepth(pp);

    DoPlayerHorizon(pp);

    if (TEST(sector[pp->cursectnum].extra, SECTFX_DYNAMIC_AREA))
    {
        if (TEST(pp->Flags, PF_FLYING|PF_JUMPING|PF_FALLING))
        {
            if (pp->posz > pp->loz)
                pp->posz = pp->loz - PLAYER_HEIGHT;

            if (pp->posz < pp->hiz)
                pp->posz = pp->hiz + PLAYER_HEIGHT;
        }
        else if (TEST(pp->Flags, PF_SWIMMING|PF_DIVING))
        {
            if (pp->posz > pp->loz)
                pp->posz = pp->loz - PLAYER_SWIM_HEIGHT;

            if (pp->posz < pp->hiz)
                pp->posz = pp->hiz + PLAYER_SWIM_HEIGHT;
        }
        // moved to crawling and running respectively
#if 0
        else if (TEST(pp->Flags, PF_CRAWLING))
        {
            pp->posz = pp->loz - PLAYER_CRAWL_HEIGHT;
        }
        else
        {
            if (pp->posz > pp->loz)
                pp->posz = pp->loz - PLAYER_HEIGHT;

            if (pp->posz < pp->hiz)
                pp->posz = pp->hiz + PLAYER_HEIGHT;
            pp->posz = pp->loz - PLAYER_HEIGHT;
        }
#endif
    }
}

void
DoPlayerSectorUpdatePreMove(PLAYERp pp)
{
    short sectnum = pp->cursectnum;

    if (TEST(sector[pp->cursectnum].extra, SECTFX_DYNAMIC_AREA))
    {
        updatesectorz(pp->posx, pp->posy, pp->posz, &sectnum);
        if (sectnum < 0)
        {
            sectnum = pp->cursectnum;
            COVERupdatesector(pp->posx, pp->posy, &sectnum);
        }
        ASSERT(sectnum >= 0);
    }
    else if (FAF_ConnectArea(sectnum))
    {
        updatesectorz(pp->posx, pp->posy, pp->posz, &sectnum);
        if (sectnum < 0)
        {
            sectnum = pp->cursectnum;
            COVERupdatesector(pp->posx, pp->posy, &sectnum);
        }
        ASSERT(sectnum >= 0);
    }

    pp->cursectnum = sectnum;
}

void
DoPlayerSectorUpdatePostMove(PLAYERp pp)
{
    short sectnum;
    int fz,cz;

    // need to do updatesectorz if in connect area
    if (FAF_ConnectArea(pp->cursectnum))
    {
        sectnum = pp->cursectnum;
        updatesectorz(pp->posx, pp->posy, pp->posz, &pp->cursectnum);

        // can mess up if below
        if (pp->cursectnum < 0)
        {
            pp->cursectnum = sectnum;

            // adjust the posz to be in a sector
            getzsofslope(pp->cursectnum, pp->posx, pp->posy, &cz, &fz);
            if (pp->posz > fz)
                pp->posz = fz;

            if (pp->posz < cz)
                pp->posz = cz;

            // try again
            updatesectorz(pp->posx, pp->posy, pp->posz, &pp->cursectnum);
            ASSERT(pp->cursectnum >= 0);
        }
    }
    else
    {
        PlayerSectorBound(pp, Z(1));
    }

}

void PlaySOsound(short sectnum, short sound_num)
{
    short i,nexti;

    // play idle sound - sound 1
    TRAVERSE_SPRITE_SECT(headspritesect[sectnum], i, nexti)
    {
        if (sprite[i].statnum == STAT_SOUND_SPOT)
        {
            DoSoundSpotStopSound(sprite[i].lotag);
            DoSoundSpotMatch(sprite[i].lotag, sound_num, 0);
        }
    }
}

void StopSOsound(short sectnum)
{
    short i,nexti;

    // play idle sound - sound 1
    TRAVERSE_SPRITE_SECT(headspritesect[sectnum], i, nexti)
    {
        if (sprite[i].statnum == STAT_SOUND_SPOT)
            DoSoundSpotStopSound(sprite[i].lotag);
    }
}

void
DoPlayerMoveBoat(PLAYERp pp)
{
    int xvect, yvect, z;
    int floor_dist;
    int ret;
    short save_sectnum;
    USERp u = User[pp->PlayerSprite];
    SECTOR_OBJECTp sop = pp->sop;

    SW_PACKET last_input;
    int fifo_ndx;

    if (Prediction)
        return;

    if (!Prediction)
    {
        // this code looks at the fifo to get the last value for comparison
        fifo_ndx = (movefifoplc-2) & (MOVEFIFOSIZ - 1);
        last_input = pp->inputfifo[fifo_ndx];

        if (labs(pp->input.vel|pp->input.svel) && !labs(last_input.vel|last_input.svel))
            PlaySOsound(pp->sop->mid_sector,SO_DRIVE_SOUND);
        else if (!labs(pp->input.vel|pp->input.svel) && labs(last_input.vel|last_input.svel))
            PlaySOsound(pp->sop->mid_sector,SO_IDLE_SOUND);
    }

    PLAYER_RUN_LOCK(pp);

    DoPlayerTurnBoat(pp);

    if (PLAYER_MOVING(pp) == 0)
        RESET(pp->Flags, PF_PLAYER_MOVED);
    else
        SET(pp->Flags, PF_PLAYER_MOVED);

    pp->oxvect = pp->xvect;
    pp->oyvect = pp->yvect;

    if (sop->drive_speed)
    {
        pp->xvect = mulscale6(pp->input.vel, sop->drive_speed);
        pp->yvect = mulscale6(pp->input.svel, sop->drive_speed);

        // does sliding/momentum
        pp->xvect = (pp->xvect + (pp->oxvect*(sop->drive_slide-1)))/sop->drive_slide;
        pp->yvect = (pp->yvect + (pp->oyvect*(sop->drive_slide-1)))/sop->drive_slide;
    }
    else
    {
        pp->xvect += ((pp->input.vel*synctics*2)<<6);
        pp->yvect += ((pp->input.svel*synctics*2)<<6);

        pp->xvect  = mulscale16(pp->xvect, BOAT_FRICTION);
        pp->yvect  = mulscale16(pp->yvect, BOAT_FRICTION);

        // does sliding/momentum
        pp->xvect = (pp->xvect + (pp->oxvect*5))/6;
        pp->yvect = (pp->yvect + (pp->oyvect*5))/6;
    }

    if (labs(pp->xvect) < 12800 && labs(pp->yvect) < 12800)
        pp->xvect = pp->yvect = 0;

    pp->lastcursectnum = pp->cursectnum;
    z = pp->posz + Z(10);

    save_sectnum = pp->cursectnum;
    OperateSectorObject(pp->sop, pp->pang, MAXSO, MAXSO);
    pp->cursectnum = pp->sop->op_main_sector; // for speed

    floor_dist = labs(z - pp->sop->floor_loz);
    ret = clipmove_old(&pp->posx, &pp->posy, &z, &pp->cursectnum, pp->xvect, pp->yvect, (int)pp->sop->clipdist, Z(4), floor_dist, CLIPMASK_PLAYER);

    OperateSectorObject(pp->sop, pp->pang, pp->posx, pp->posy);
    pp->cursectnum = save_sectnum; // for speed

    DoPlayerHorizon(pp);
}

#if 0
STATE s_TankTreadMove[] =
{
    {755, 6|SF_WALL_ANIM, NULL, s_TankTreadMove[1]},
    {756, 6|SF_WALL_ANIM, NULL, s_TankTreadMove[0]},
};

STATE s_TankTreadStill[] =
{
    {755, 6|SF_WALL_ANIM, NULL, s_TankTreadMove[0]},
};
#endif

void DoTankTreads(PLAYERp pp)
{
    SPRITEp sp;
    short i,nexti;
    int vel;
    SECTORp *sectp;
    int j;
    int dot;
    SWBOOL reverse = FALSE;
    WALLp wp;
    short startwall,endwall;
    int k;

    if (Prediction)
        return;

    vel = FindDistance2D(pp->xvect>>8, pp->yvect>>8);
    dot = DOT_PRODUCT_2D(pp->xvect, pp->yvect, sintable[NORM_ANGLE(pp->pang+512)], sintable[pp->pang]);
    if (dot < 0)
        reverse = TRUE;

    for (sectp = pp->sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        TRAVERSE_SPRITE_SECT(headspritesect[*sectp - sector], i, nexti)
        {
            sp = &sprite[i];

            // BOOL1 is set only if pans with SO
            if (!TEST_BOOL1(sp))
                continue;

#if 0
            if (sp->statnum == STAT_WALL_ANIM)
            {
                if (SP_TAG3(sp) == TANK_TREAD_WALL_ANIM)
                {
                    if (vel)
                    {
                        if (u->StateStart != s_TankTreadMove)
                            ChangeState(i, s_TankTreadMove);
                    }
                    else
                    {
                        if (u->StateStart != s_TankTreadStill)
                            ChangeState(i, s_TankTreadStill);
                    }
                }
            }
            else
#endif
            if (sp->statnum == STAT_WALL_PAN)
            {
                if (reverse)
                {
                    if (!TEST_BOOL2(sp))
                    {
                        SET_BOOL2(sp);
                        sp->ang = NORM_ANGLE(sp->ang + 1024);
                    }
                }
                else
                {
                    if (TEST_BOOL2(sp))
                    {
                        RESET_BOOL2(sp);
                        sp->ang = NORM_ANGLE(sp->ang + 1024);
                    }
                }

                SP_TAG5(sp) = vel;
            }
            else if (sp->statnum == STAT_FLOOR_PAN)
            {
                sp = &sprite[i];

                if (reverse)
                {
                    if (!TEST_BOOL2(sp))
                    {
                        SET_BOOL2(sp);
                        sp->ang = NORM_ANGLE(sp->ang + 1024);
                    }
                }
                else
                {
                    if (TEST_BOOL2(sp))
                    {
                        RESET_BOOL2(sp);
                        sp->ang = NORM_ANGLE(sp->ang + 1024);
                    }
                }

                SP_TAG5(sp) = vel;
            }
            else if (sp->statnum == STAT_CEILING_PAN)
            {
                sp = &sprite[i];

                if (reverse)
                {
                    if (!TEST_BOOL2(sp))
                    {
                        SET_BOOL2(sp);
                        sp->ang = NORM_ANGLE(sp->ang + 1024);
                    }
                }
                else
                {
                    if (TEST_BOOL2(sp))
                    {
                        RESET_BOOL2(sp);
                        sp->ang = NORM_ANGLE(sp->ang + 1024);
                    }
                }

                SP_TAG5(sp) = vel;
            }
        }
    }


}

void
SetupDriveCrush(PLAYERp pp, int *x, int *y)
{
    int radius = pp->sop_control->clipdist;

    x[0] = pp->posx - radius;
    y[0] = pp->posy - radius;

    x[1] = pp->posx + radius;
    y[1] = pp->posy - radius;

    x[2] = pp->posx + radius;
    y[2] = pp->posy + radius;

    x[3] = pp->posx - radius;
    y[3] = pp->posy + radius;
}

void
DriveCrush(PLAYERp pp, int *x, int *y)
{
    int testpointinquad(int x, int y, int *qx, int *qy);

    SECTOR_OBJECTp sop = pp->sop_control;
    int radius;
    SPRITEp sp;
    USERp u;
    int i,nexti;
    short stat;
    int dot;
    SECTORp *sectp;

    if (MoveSkip4 == 0)
        return;

    // not moving - don't crush
    if ((pp->xvect|pp->yvect) == 0 && pp->input.angvel == 0)
        return;

    radius = sop->clipdist;

    // main sector
    TRAVERSE_SPRITE_SECT(headspritesect[sop->op_main_sector], i, nexti)
    {
        sp = &sprite[i];
        u = User[i];

        if (testpointinquad(sp->x, sp->y, x, y))
        {
            if (TEST(sp->extra, SPRX_BREAKABLE) && HitBreakSprite(i,0))
                continue;

            if (sp->statnum == STAT_MISSILE)
                continue;

            if (sp->picnum == ST1)
                continue;

            if (TEST(sp->extra, SPRX_PLAYER_OR_ENEMY))
            {
                if (!TEST(u->Flags, SPR_DEAD) && !TEST(sp->extra, SPRX_BREAKABLE))
                    continue;
            }

            if (TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                continue;

            if (sp->statnum > STAT_DONT_DRAW)
                continue;

            if (sp->z < sop->crush_z)
                continue;

            SpriteQueueDelete(i);
            KillSprite(i);
        }
    }

    // all enemys
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], i, nexti)
    {
        sp = &sprite[i];

        if (testpointinquad(sp->x, sp->y, x, y))
        {
            //if (sp->z < pp->posz)
            if (sp->z < sop->crush_z)
                continue;

            vel = FindDistance2D(pp->xvect>>8, pp->yvect>>8);
            if (vel < 9000)
            {
                DoActorBeginSlide(i, getangle(pp->xvect, pp->yvect), vel/8, 5);
                if (DoActorSlide(i))
                    continue;
            }

            UpdateSinglePlayKills(i);

            if (SpawnShrap(i, -99))
                SetSuicide(i);
            else
                KillSprite(i);
        }
    }

    // all dead actors
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_DEAD_ACTOR], i, nexti)
    {
        sp = &sprite[i];

        if (testpointinquad(sp->x, sp->y, x, y))
        {
            if (sp->z < sop->crush_z)
                continue;

            SpriteQueueDelete(i);
            KillSprite(i);
        }
    }

    // all players
    for (stat = 0; stat < MAX_SW_PLAYERS; stat++)
    {
        i = headspritestat[STAT_PLAYER0 + stat];

        if (i < 0)
            continue;

        sp = &sprite[i];
        u = User[i];

        if (u->PlayerP == pp)
            continue;

        if (testpointinquad(sp->x, sp->y, x, y))
        {
            int damage;

            //if (sp->z < pp->posz)
            if (sp->z < sop->crush_z)
                continue;

            damage = -(u->Health + 100);
            PlayerDamageSlide(u->PlayerP, damage, pp->pang);
            PlayerUpdateHealth(u->PlayerP, damage);
            //PlayerCheckDeath(u->PlayerP, -1);
            PlayerCheckDeath(u->PlayerP, pp->PlayerSprite);
        }
    }


    // if it ends up actually in the drivable sector kill it
    for (sectp = sop->sectp; *sectp; sectp++)
    {
        TRAVERSE_SPRITE_SECT(headspritesect[(*sectp) - sector], i, nexti)
        {
            sp = &sprite[i];
            u = User[i];

            // give some extra buffer
            if (sp->z < sop->crush_z + Z(40))
                continue;

            if (TEST(sp->extra, SPRX_PLAYER_OR_ENEMY))
            {
                if (sp->statnum == STAT_ENEMY)
                {
                    if (SpawnShrap(i, -99))
                        SetSuicide(i);
                    else
                        KillSprite(i);
                }
            }
        }
    }
}

void
DoPlayerMoveTank(PLAYERp pp)
{
    int xvect, yvect, z;
    int floor_dist;
    int ret;
    short save_sectnum;
    SPRITEp sp = pp->sop->sp_child;
    USERp u = User[sp - sprite];
    int save_cstat;
    int angvel;
    int x[4], y[4], ox[4], oy[4];
    int wallcount;
    int count=0;

    SECTORp *sectp;
    SECTOR_OBJECTp sop = pp->sop;
    WALLp wp;
    int j,k;
    short startwall,endwall;

    SW_PACKET last_input;
    int fifo_ndx;
    SWBOOL RectClip = !!TEST(sop->flags, SOBJ_RECT_CLIP);

    if (Prediction)
        return;

    if (!Prediction)
    {
        // this code looks at the fifo to get the last value for comparison
        fifo_ndx = (movefifoplc-2) & (MOVEFIFOSIZ - 1);
        last_input = pp->inputfifo[fifo_ndx];

        if (labs(pp->input.vel|pp->input.svel) && !labs(last_input.vel|last_input.svel))
            PlaySOsound(pp->sop->mid_sector,SO_DRIVE_SOUND);
        else if (!labs(pp->input.vel|pp->input.svel) && labs(last_input.vel|last_input.svel))
            PlaySOsound(pp->sop->mid_sector,SO_IDLE_SOUND);
    }

    PLAYER_RUN_LOCK(pp);

    if (PLAYER_MOVING(pp) == 0)
        RESET(pp->Flags, PF_PLAYER_MOVED);
    else
        SET(pp->Flags, PF_PLAYER_MOVED);

    pp->oxvect = pp->xvect;
    pp->oyvect = pp->yvect;

    if (sop->drive_speed)
    {
        pp->xvect = mulscale6(pp->input.vel, sop->drive_speed);
        pp->yvect = mulscale6(pp->input.svel, sop->drive_speed);

        // does sliding/momentum
        pp->xvect = (pp->xvect + (pp->oxvect*(sop->drive_slide-1)))/sop->drive_slide;
        pp->yvect = (pp->yvect + (pp->oyvect*(sop->drive_slide-1)))/sop->drive_slide;
    }
    else
    {
        pp->xvect += ((pp->input.vel*synctics*2)<<6);
        pp->yvect += ((pp->input.svel*synctics*2)<<6);

        pp->xvect  = mulscale16(pp->xvect, TANK_FRICTION);
        pp->yvect  = mulscale16(pp->yvect, TANK_FRICTION);

        pp->xvect = (pp->xvect + (pp->oxvect*1))/2;
        pp->yvect = (pp->yvect + (pp->oyvect*1))/2;
    }

    if (labs(pp->xvect) < 12800 && labs(pp->yvect) < 12800)
        pp->xvect = pp->yvect = 0;

    pp->lastcursectnum = pp->cursectnum;
    z = pp->posz + Z(10);

    if (RectClip)
    {
        for (sectp = sop->sectp, wallcount = 0, j = 0; *sectp; sectp++, j++)
        {
            startwall = (*sectp)->wallptr;
            endwall = startwall + (*sectp)->wallnum - 1;

            for (wp = &wall[startwall], k = startwall; k <= endwall; wp++, k++)
            {
                if (wp->extra && TEST(wp->extra, WALLFX_LOOP_OUTER|WALLFX_LOOP_OUTER_SECONDARY) == WALLFX_LOOP_OUTER)
                {
                    x[count] = wp->x;
                    y[count] = wp->y;

                    ox[count] = sop->xmid - sop->xorig[wallcount];
                    oy[count] = sop->ymid - sop->yorig[wallcount];

                    count++;
                }

                wallcount++;
            }
        }

        PRODUCTION_ASSERT(count == 4);
    }

    save_sectnum = pp->cursectnum;
    OperateSectorObject(pp->sop, pp->pang, MAXSO, MAXSO);
    pp->cursectnum = pp->sop->op_main_sector; // for speed

    floor_dist = labs(z - pp->sop->floor_loz);


    if (RectClip)
    {
        int nx,ny;
        hitdata_t hitinfo;
        int vel;
        int ret;

        save_cstat = pp->SpriteP->cstat;
        RESET(pp->SpriteP->cstat, CSTAT_SPRITE_BLOCK);
        DoPlayerTurnTankRect(pp, x, y, ox, oy);

        ret = RectClipMove(pp, x, y);
        DriveCrush(pp, x, y);
        pp->SpriteP->cstat = save_cstat;

        if (!ret)
        {
            vel = FindDistance2D(pp->xvect>>8, pp->yvect>>8);

            if (vel > 13000)
            {
                vec3_t hit_pos = { DIV2(x[0] + x[1]), DIV2(y[0] + y[1]), sector[pp->cursectnum].floorz - Z(10) };

                hitscan(&hit_pos, pp->cursectnum,
                        //pp->xvect, pp->yvect, 0,
                        MOVEx(256, pp->pang), MOVEy(256, pp->pang), 0,
                        &hitinfo, CLIPMASK_PLAYER);

                ////DSPRINTF(ds,"hitinfo.sect %d, hitinfo.wall %d, hitinfo.pos.x %d, hitinfo.pos.y %d, hitinfo.pos.z %d",hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z);
                //MONO_PRINT(ds);

                if (FindDistance2D(hitinfo.pos.x - hit_pos.x, hitinfo.pos.y - hit_pos.y) < 800)
                {
                    if (hitinfo.wall >= 0)
                        u->ret = hitinfo.wall|HIT_WALL;
                    else if (hitinfo.sprite >= 0)
                        u->ret = hitinfo.sprite|HIT_SPRITE;
                    else
                        u->ret = 0;

                    VehicleMoveHit(sp - sprite);
                }

                if (!TEST(sop->flags, SOBJ_NO_QUAKE))
                {
                    SetPlayerQuake(pp);
                }
            }

            if (vel > 12000)
            {
                pp->xvect = pp->yvect = pp->oxvect = pp->oyvect = 0;
            }
        }
    }
    else
    {
        DoPlayerTurnTank(pp, z, floor_dist);

        save_cstat = pp->SpriteP->cstat;
        RESET(pp->SpriteP->cstat, CSTAT_SPRITE_BLOCK);
        if (pp->sop->clipdist)
            u->ret = clipmove_old(&pp->posx, &pp->posy, &z, &pp->cursectnum, pp->xvect, pp->yvect, (int)pp->sop->clipdist, Z(4), floor_dist, CLIPMASK_PLAYER);
        else
            u->ret = MultiClipMove(pp, z, floor_dist);
        pp->SpriteP->cstat = save_cstat;

        //SetupDriveCrush(pp, x, y);
        //DriveCrush(pp, x, y);

        if (u->ret)
        {
            int vel;

            vel = FindDistance2D(pp->xvect>>8, pp->yvect>>8);

            if (vel > 13000)
            {
                VehicleMoveHit(sp - sprite);
                pp->slide_xvect = -pp->xvect<<1;
                pp->slide_yvect = -pp->yvect<<1;
                if (!TEST(sop->flags, SOBJ_NO_QUAKE))
                    SetPlayerQuake(pp);
            }

            if (vel > 12000)
            {
                pp->xvect = pp->yvect = pp->oxvect = pp->oyvect = 0;
            }
        }
    }

    OperateSectorObject(pp->sop, pp->pang, pp->posx, pp->posy);
    pp->cursectnum = save_sectnum; // for speed

    DoPlayerHorizon(pp);

    DoTankTreads(pp);
}

void
DoPlayerMoveTurret(PLAYERp pp)
{
    int xvect, yvect, z;
    int floor_dist;
    int ret;
    short save_sectnum;
    USERp u = User[pp->PlayerSprite];

    PLAYER_RUN_LOCK(pp);

    DoPlayerTurnTurret(pp);

    if (PLAYER_MOVING(pp) == 0)
        RESET(pp->Flags, PF_PLAYER_MOVED);
    else
        SET(pp->Flags, PF_PLAYER_MOVED);

    OperateSectorObject(pp->sop, pp->pang, pp->sop->xmid, pp->sop->ymid);

    DoPlayerHorizon(pp);
}

void
DoPlayerBeginJump(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    SET(pp->Flags, PF_JUMPING);
    RESET(pp->Flags, PF_FALLING);
    RESET(pp->Flags, PF_CRAWLING);
    RESET(pp->Flags, PF_LOCK_CRAWL);

    pp->floor_dist = PLAYER_JUMP_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_JUMP_CEILING_DIST;
    pp->friction = PLAYER_JUMP_FRICTION;

    PlayerGravity = PLAYER_JUMP_GRAV;

    pp->jump_speed = PLAYER_JUMP_AMT + pp->WadeDepth * 4;

    if (DoPlayerWadeSuperJump(pp))
    {
        pp->jump_speed = PLAYER_JUMP_AMT - pp->WadeDepth * 5;
    }

    pp->JumpDuration = MAX_JUMP_DURATION;
    pp->DoPlayerAction = DoPlayerJump;

    ///DamageData[u->WeaponNum].Init(pp);

    NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Jump);
}

void
DoPlayerBeginForceJump(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    SET(pp->Flags, PF_JUMPING);
    RESET(pp->Flags, PF_FALLING|PF_CRAWLING|PF_CLIMBING|PF_LOCK_CRAWL);

    pp->JumpDuration = MAX_JUMP_DURATION;
    pp->DoPlayerAction = DoPlayerForceJump;

    pp->floor_dist = PLAYER_JUMP_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_JUMP_CEILING_DIST;
    pp->friction = PLAYER_JUMP_FRICTION;

    PlayerGravity = PLAYER_JUMP_GRAV;

    ///DamageData[u->WeaponNum].Init(pp);

    NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Jump);
}

void
DoPlayerJump(PLAYERp pp)
{
    short i;

    // reset flag key for double jumps
    if (!TEST_SYNC_KEY(pp, SK_JUMP))
    {
        FLAG_KEY_RESET(pp, SK_JUMP);
    }

    // instead of multiplying by synctics, use a loop for greater accuracy
    for (i = 0; i < synctics; i++)
    {
        // PlayerGravity += synctics;  // See how increase gravity as we go?
        if (TEST_SYNC_KEY(pp, SK_JUMP))
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
        pp->posz += pp->jump_speed;

        // if player gets to close the ceiling while jumping
        //if (pp->posz < pp->hiz + Z(4))
        if (PlayerCeilingHit(pp, pp->hiz + Z(4)))
        {
            // put player at the ceiling
            pp->posz = pp->hiz + Z(4);

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
        if (PlayerFloorHit(pp, pp->loz - pp->floor_dist))
        {
            pp->posz = pp->loz - pp->floor_dist;

            pp->jump_speed = 0;
            PlayerSectorBound(pp, Z(1));
            DoPlayerBeginRun(pp);
            DoPlayerHeight(pp);
            return;
        }
    }

    if (PlayerFlyKey(pp))
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


void
DoPlayerForceJump(PLAYERp pp)
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
        pp->posz += pp->jump_speed;

        // if player gets to close the ceiling while jumping
        //if (pp->posz < pp->hiz + Z(4))
        if (PlayerCeilingHit(pp, pp->hiz + Z(4)))
        {
            // put player at the ceiling
            pp->posz = pp->hiz + Z(4);

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

void
DoPlayerBeginFall(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    SET(pp->Flags, PF_FALLING);
    RESET(pp->Flags, PF_JUMPING);
    RESET(pp->Flags, PF_CRAWLING);
    RESET(pp->Flags, PF_LOCK_CRAWL);

    pp->floor_dist = PLAYER_FALL_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_FALL_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerFall;
    pp->friction = PLAYER_FALL_FRICTION;

    // Only change to falling frame if you were in the jump frame
    // Otherwise an animation may be messed up such as Running Jump Kick
    if (u->Rot == u->ActorActionSet->Jump)
        NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Fall);
}

void StackedWaterSplash(PLAYERp pp)
{
    if (FAF_ConnectArea(pp->cursectnum))
    {
        short sectnum = pp->cursectnum;

        updatesectorz(pp->posx, pp->posy, SPRITEp_BOS(pp->SpriteP), &sectnum);

        if (SectorIsUnderwaterArea(sectnum))
        {
            PlaySound(DIGI_SPLASH1, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan);
        }
    }
}

void
DoPlayerFall(PLAYERp pp)
{
    short i;
    int recoil_amt;
    int depth;
    static int handle=0;

    // reset flag key for double jumps
    if (!TEST_SYNC_KEY(pp, SK_JUMP))
    {
        FLAG_KEY_RESET(pp, SK_JUMP);
    }

    if (SectorIsUnderwaterArea(pp->cursectnum))
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
        pp->posz += pp->jump_speed;

        ////DSPRINTF(ds,"Fall velocity = %d",pp->jump_speed);
        //MONO_PRINT(ds);

        if (pp->jump_speed > 2000)
        {
            PlayerSound(DIGI_FALLSCREAM, &pp->posx, &pp->posy, &pp->posz,
                        v3df_dontpan|v3df_doppler|v3df_follow,pp);
            handle = pp->TalkVocHandle; // Save id for later
        }
        else if (pp->jump_speed > 1300)
        {
            if (TEST(pp->Flags, PF_LOCK_HORIZ))
            {
                RESET(pp->Flags, PF_LOCK_HORIZ);
                SET(pp->Flags, PF_LOOKING);
            }
        }



        depth = GetZadjustment(pp->cursectnum, FLOOR_Z_ADJUST)>>8;
        if (depth == 0)
            depth = pp->WadeDepth;

        if (depth > 20)
            recoil_amt = 0;
        else
            recoil_amt = min(pp->jump_speed*6,Z(35));

        // need a test for head hits a sloped ceiling while falling
        // if player gets to close the Ceiling while Falling
        if (PlayerCeilingHit(pp, pp->hiz + pp->ceiling_dist))
        {
            // put player at the ceiling
            pp->posz = pp->hiz + pp->ceiling_dist;
            // don't return or anything - allow to fall until
            // hit floor
        }

        if (PlayerFloorHit(pp, pp->loz - PLAYER_HEIGHT + recoil_amt))
        {
            SECT_USERp sectu = SectUser[pp->cursectnum];
            SECTORp sectp = &sector[pp->cursectnum];

            PlayerSectorBound(pp, Z(1));

            if (sectu && (TEST(sectp->extra, SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE))
            {
                PlaySound(DIGI_SPLASH1, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan);
            }
            else
            {
                if (pp->jump_speed > 1020)
                    // Feet hitting ground sound
                    PlaySound(DIGI_HITGROUND, &pp->posx, &pp->posy, &pp->posz, v3df_follow|v3df_dontpan);
            }

            if (handle && FX_SoundActive(handle))
            {
                // My sound code will detect the sound has stopped and clean up
                // for you.
                FX_StopSound(handle);
                pp->PlayerTalking = FALSE;
                handle = 0;
            }

            // i any kind of crawl key get rid of recoil
            if (DoPlayerTestCrawl(pp) || TEST_SYNC_KEY(pp, SK_CRAWL))
            {
                pp->posz = pp->loz - PLAYER_CRAWL_HEIGHT;
            }
            else
            {
                // this was causing the z to snap immediately
                // changed it so it stays gradual

                //pp->posz = pp->loz - PLAYER_HEIGHT + recoil_amt;

                pp->posz += recoil_amt;
                DoPlayerHeight(pp);
            }

            // do some damage
            if (pp->jump_speed > 1700 && depth == 0)
            {

                PlayerSound(DIGI_PLAYERPAIN2, &pp->posx, &pp->posy, &pp->posz, v3df_follow|v3df_dontpan,pp);
                // PlayerUpdateHealth(pp, -RANDOM_RANGE(PLAYER_FALL_DAMAGE_AMOUNT) - 2);

                if (pp->jump_speed > 1700 && pp->jump_speed < 4000)
                {
                    if (pp->jump_speed > 0)
                        PlayerUpdateHealth(pp, -((pp->jump_speed-1700)/40));
                }
                else if (pp->jump_speed >= 4000)
                {
                    USERp u = User[pp->PlayerSprite];
                    PlayerUpdateHealth(pp, -u->Health);  // Make sure he dies!
                    u->Health = 0;
                }

                PlayerCheckDeath(pp, -1);

                if (TEST(pp->Flags, PF_DEAD))
                    return;
            }

            if (TEST_SYNC_KEY(pp, SK_CRAWL))
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

    if (PlayerFlyKey(pp))
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

void
DoPlayerBeginClimb(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp sp = pp->SpriteP;

    RESET(pp->Flags, PF_JUMPING|PF_FALLING);
    RESET(pp->Flags, PF_CRAWLING);
    RESET(pp->Flags, PF_LOCK_CRAWL);

    pp->DoPlayerAction = DoPlayerClimb;

    SET(pp->Flags, PF_CLIMBING|PF_WEAPON_DOWN);
    SET(sp->cstat, CSTAT_SPRITE_YCENTER);

    //DamageData[u->WeaponNum].Init(pp);

    //NewStateGroup(pp->PlayerSprite, User[pp->PlayerSprite]->ActorActionSet->Climb);
    NewStateGroup(pp->PlayerSprite, sg_PlayerNinjaClimb);
}


void
DoPlayerClimb(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    int climb_amt;
    char i;
    short oldang, delta_ang;
    SPRITEp sp = pp->SpriteP;
    int climbvel;
    int dot;
    short sec,wal,spr;
    int dist;
    short lastsectnum;
    SWBOOL LadderUpdate = FALSE;

    if (Prediction)
        return;

    pp->xvect += ((pp->input.vel*synctics*2)<<6);
    pp->yvect += ((pp->input.svel*synctics*2)<<6);
    pp->xvect  = mulscale16(pp->xvect, PLAYER_CLIMB_FRICTION);
    pp->yvect  = mulscale16(pp->yvect, PLAYER_CLIMB_FRICTION);
    if (labs(pp->xvect) < 12800 && labs(pp->yvect) < 12800)
        pp->xvect = pp->yvect = 0;

    climbvel = FindDistance2D(pp->xvect, pp->yvect)>>9;
    dot = DOT_PRODUCT_2D(pp->xvect, pp->yvect, sintable[NORM_ANGLE(pp->pang+512)], sintable[pp->pang]);
    if (dot < 0)
        climbvel = -climbvel;

    // Run lock - routine doesn't call DoPlayerMove
    PLAYER_RUN_LOCK(pp);

    // need to rewrite this for FAF stuff

    // Jump off of the ladder
    if (TEST_SYNC_KEY(pp, SK_JUMP))
    {
        RESET(pp->Flags, PF_CLIMBING|PF_WEAPON_DOWN);
        RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
        DoPlayerBeginJump(pp);
        return;
    }

    if (climbvel != 0)
    {
        // move player to center of ladder
        for (i = synctics; i; i--)
        {
#define ADJ_AMT 8

            // player
            if (pp->posx != pp->lx)
            {
                if (pp->posx < pp->lx)
                    pp->posx += ADJ_AMT;
                else if (pp->posx > pp->lx)
                    pp->posx -= ADJ_AMT;

                if (labs(pp->posx - pp->lx) <= ADJ_AMT)
                    pp->posx = pp->lx;
            }

            if (pp->posy != pp->ly)
            {
                if (pp->posy < pp->ly)
                    pp->posy += ADJ_AMT;
                else if (pp->posy > pp->ly)
                    pp->posy -= ADJ_AMT;

                if (labs(pp->posy - pp->ly) <= ADJ_AMT)
                    pp->posy = pp->ly;
            }

            // sprite
            if (sp->x != u->sx)
            {
                if (sp->x < u->sx)
                    sp->x += ADJ_AMT;
                else if (sp->x > u->sx)
                    sp->x -= ADJ_AMT;

                if (labs(sp->x - u->sx) <= ADJ_AMT)
                    sp->x = u->sx;
            }

            if (sp->y != u->sy)
            {
                if (sp->y < u->sy)
                    sp->y += ADJ_AMT;
                else if (sp->y > u->sy)
                    sp->y -= ADJ_AMT;

                if (labs(sp->y - u->sy) <= ADJ_AMT)
                    sp->y = u->sy;
            }
        }
    }

    DoPlayerZrange(pp);

    ASSERT(pp->LadderSector >= 0 && pp->LadderSector <= MAXSECTORS);

    // moving UP
    if (climbvel > 0)
    {
        // pp->climb_ndx += climb_rate * synctics;
        climb_amt = (climbvel>>4) * 8;

        pp->climb_ndx &= 1023;

        pp->posz -= climb_amt;

        // if player gets to close the ceiling while climbing
        if (PlayerCeilingHit(pp, pp->hiz))
        {
            // put player at the hiz
            pp->posz = pp->hiz;
            NewStateGroup(pp->PlayerSprite, sg_PlayerNinjaClimb);
        }

        // if player gets to close the ceiling while climbing
        if (PlayerCeilingHit(pp, pp->hiz + Z(4)))
        {
            // put player at the ceiling
            pp->posz = sector[pp->LadderSector].ceilingz + Z(4);
            NewStateGroup(pp->PlayerSprite, sg_PlayerNinjaClimb);
        }

        // if floor is ABOVE you && your head goes above it, do a jump up to
        // terrace

        if (pp->posz < sector[pp->LadderSector].floorz - Z(6))
        {
            pp->jump_speed = PLAYER_CLIMB_JUMP_AMT;
            RESET(pp->Flags, PF_CLIMBING|PF_WEAPON_DOWN);
            RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
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

        // pp->posz += (climb_amt * sintable[pp->climb_ndx]) >> 14;
        pp->posz += climb_amt;

        // if you are touching the floor
        //if (pp->posz >= pp->loz - Z(4) - PLAYER_HEIGHT)
        if (PlayerFloorHit(pp, pp->loz - Z(4) - PLAYER_HEIGHT))
        {
            // stand on floor
            pp->posz = pp->loz - Z(4) - PLAYER_HEIGHT;

            // if moving backwards start running
            if (climbvel < 0)
            {
                RESET(pp->Flags, PF_CLIMBING|PF_WEAPON_DOWN);
                RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
                DoPlayerBeginRun(pp);
                return;
            }
        }
    }
    else
    {
        NewStateGroup(pp->PlayerSprite, sg_PlayerNinjaClimb);
    }

    // setsprite to players location
    sp->z = pp->posz + PLAYER_HEIGHT;
    changespritesect(pp->PlayerSprite, pp->cursectnum);

    DoPlayerHorizon(pp);

    if (FAF_ConnectArea(pp->cursectnum))
    {
        updatesectorz(pp->posx, pp->posy, pp->posz, &pp->cursectnum);
        LadderUpdate = TRUE;
    }

    if (WarpPlane(&pp->posx, &pp->posy, &pp->posz, &pp->cursectnum))
    {
        PlayerWarpUpdatePos(pp);
        LadderUpdate = TRUE;
    }

    if (LadderUpdate)
    {
        SPRITEp lsp;
        int nx,ny;

        // constantly look for new ladder sector because of warping at any time
        neartag(pp->posx, pp->posy, pp->posz,
                pp->cursectnum, pp->pang,
                &sec, &wal, &spr,
                &dist, 800L, NTAG_SEARCH_LO_HI, NULL);

        if (wal >= 0)
        {
            pp->LadderSector = wall[wal].nextsector;

            lsp = FindNearSprite(pp->SpriteP, STAT_CLIMB_MARKER);

            // determine where the player is supposed to be in relation to the ladder
            // move out in front of the ladder
            nx = MOVEx(100, lsp->ang);
            ny = MOVEy(100, lsp->ang);

            // set angle player is supposed to face.
            pp->LadderAngle = NORM_ANGLE(lsp->ang + 1024);
            pp->LadderSector = wall[wal].nextsector;

            // set players "view" distance from the ladder - needs to be farther than
            // the sprite

            pp->lx = lsp->x + nx * 5;
            pp->ly = lsp->y + ny * 5;

            pp->pang = pp->LadderAngle;
        }
    }
}


int
DoPlayerWadeSuperJump(PLAYERp pp)
{
    hitdata_t hitinfo;
    USERp u = User[pp->PlayerSprite];
    unsigned i;
    //short angs[3];
    static short angs[3] = {0, 0, 0};
    int zh = sector[pp->cursectnum].floorz - Z(pp->WadeDepth) - Z(2);

    if (Prediction) return FALSE;   // !JIM! 8/5/97 Teleporter FAFhitscan SuperJump bug.

    for (i = 0; i < SIZ(angs); i++)
    {
        FAFhitscan(pp->posx, pp->posy, zh, pp->cursectnum,    // Start position
                   sintable[NORM_ANGLE(pp->pang + angs[i] + 512)],       // X vector of 3D ang
                   sintable[NORM_ANGLE(pp->pang + angs[i])],         // Y vector of 3D ang
                   0,                          // Z vector of 3D ang
                   &hitinfo, CLIPMASK_MISSILE);

        if (hitinfo.wall >= 0 && hitinfo.sect >= 0)
        {
            hitinfo.sect = wall[hitinfo.wall].nextsector;

            if (labs(sector[hitinfo.sect].floorz - pp->posz) < Z(50))
            {
                if (Distance(pp->posx, pp->posy, hitinfo.pos.x, hitinfo.pos.y) < ((((int)pp->SpriteP->clipdist)<<2) + 256))
                    return TRUE;
            }
        }
    }

    return FALSE;
}

SWBOOL PlayerFlyKey(PLAYERp pp)
{
    SWBOOL key;

    if (!GodMode)
        return FALSE;

    if (InputMode)
        return FALSE;

    key = KEY_PRESSED(KEYSC_J);

    if (key)
        KEY_PRESSED(KEYSC_J) = 0;

    return key;
}

#if 0
void
DoPlayerBeginSwim(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp sp = &sprite[pp->PlayerSprite];

    RESET(pp->Flags, PF_FALLING | PF_JUMPING);
    SET(pp->Flags, PF_SWIMMING);

    // reset because of bobbing when wading
    pp->SpriteP->z = pp->posz + PLAYER_SWIM_HEIGHT;

    pp->friction = PLAYER_SWIM_FRICTION;
    pp->floor_dist = PLAYER_SWIM_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_SWIM_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerSwim;

    //DamageData[u->WeaponNum].Init(pp);

    NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Swim);

    SET(sp->cstat, CSTAT_SPRITE_YCENTER);
}


void
DoPlayerSwim(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp sp = &sprite[pp->PlayerSprite];

    // Jump to get up
    if (TEST_SYNC_KEY(pp, SK_JUMP))
    {
        if (FLAG_KEY_PRESSED(pp, SK_JUMP))
        {
            FLAG_KEY_RELEASE(pp, SK_JUMP);
            RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
            RESET(pp->Flags, PF_SWIMMING);
            DoPlayerBeginJump(pp);
            // return;
        }
    }
    else
    {
        FLAG_KEY_RESET(pp, SK_JUMP);
    }

    // If too shallow to swim or stopped the "RUN" key
    if (pp->WadeDepth < MIN_SWIM_DEPTH || !(TEST_SYNC_KEY(pp, SK_RUN) || TEST(pp->Flags, PF_LOCK_RUN)))
    {
        RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
        RESET(pp->Flags, PF_SWIMMING);
        DoPlayerBeginRun(pp);
        // smooth the transition by updating the sprite now
        //UpdatePlayerSprite(pp);
        return;
    }

    // Move around
    DoPlayerMove(pp);

    if (PlayerCanDive(pp))
        return;

    if (!TEST(pp->Flags, PF_PLAYER_MOVED))
    {
        RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
        RESET(pp->Flags, PF_SWIMMING);
        DoPlayerBeginWade(pp);
        DoPlayerWade(pp);
        return;
    }
}
#endif

void
DoPlayerBeginCrawl(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    RESET(pp->Flags, PF_FALLING | PF_JUMPING);
    SET(pp->Flags, PF_CRAWLING);

    pp->friction = PLAYER_CRAWL_FRICTION;
    pp->floor_dist = PLAYER_CRAWL_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_CRAWL_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerCrawl;

    //pp->posz = pp->loz - PLAYER_CRAWL_HEIGHT;

    NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Crawl);
}

SWBOOL PlayerFallTest(PLAYERp pp, int player_height)
{
    // If the floor is far below you, fall hard instead of adjusting height
    if (labs(pp->posz - pp->loz) > player_height + PLAYER_FALL_HEIGHT)
    {
        // if on a STEEP slope sector and you have not moved off of the sector
        if (pp->lo_sectp &&
            labs(pp->lo_sectp->floorheinum) > 3000 &&
            TEST(pp->lo_sectp->floorstat, FLOOR_STAT_SLOPE) &&
            pp->lo_sectp == &sector[pp->lastcursectnum])
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }

    return FALSE;
}

void
DoPlayerCrawl(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    if (SectorIsUnderwaterArea(pp->cursectnum))
    {
        // if stacked water - which it should be
        if (FAF_ConnectArea(pp->cursectnum))
        {
            // adjust the z
            pp->posz = sector[pp->cursectnum].ceilingz + Z(12);
        }

        DoPlayerBeginDiveNoWarp(pp);
        return;
    }

    // Current Z position, adjust down to the floor, adjust to player height,
    // adjust for "bump head"
//#define PLAYER_STANDING_ROOM(pp) ((pp)->posz + PLAYER_CRAWL_HEIGHT - PLAYER_HEIGHT - PLAYER_RUN_CEILING_DIST)
#define PLAYER_STANDING_ROOM Z(68)

    if (TEST(pp->Flags, PF_LOCK_CRAWL))
    {
        if (TEST_SYNC_KEY(pp, SK_CRAWL_LOCK))
        {
            if (FLAG_KEY_PRESSED(pp, SK_CRAWL_LOCK))
            {
                //if (pp->hiz < PLAYER_STANDING_ROOM(pp))
                if (labs(pp->loz - pp->hiz) >= PLAYER_STANDING_ROOM)
                {
                    FLAG_KEY_RELEASE(pp, SK_CRAWL_LOCK);

                    RESET(pp->Flags, PF_CRAWLING);
                    DoPlayerBeginRun(pp);
                    return;
                }
            }
        }
        else
        {
            FLAG_KEY_RESET(pp, SK_CRAWL_LOCK);
        }

        // Jump to get up
        if (TEST_SYNC_KEY(pp, SK_JUMP))
        {
            if (labs(pp->loz - pp->hiz) >= PLAYER_STANDING_ROOM)
            {
                //pp->posz = pp->loz - PLAYER_HEIGHT;

                RESET(pp->Flags, PF_CRAWLING);
                DoPlayerBeginRun(pp);
                return;
            }
        }

    }
    else
    {
        // Let off of crawl to get up
        if (!TEST_SYNC_KEY(pp, SK_CRAWL))
        {
            if (labs(pp->loz - pp->hiz) >= PLAYER_STANDING_ROOM)
            {
                RESET(pp->Flags, PF_CRAWLING);
                DoPlayerBeginRun(pp);
                return;
            }
        }
    }

    if (pp->lo_sectp && TEST(pp->lo_sectp->extra, SECTFX_CURRENT))
    {
        DoPlayerCurrent(pp);
    }

    // Move around
    DoPlayerMove(pp);

    if (pp->WadeDepth > PLAYER_CRAWL_WADE_DEPTH)
    {
        RESET(pp->Flags, PF_CRAWLING);
        DoPlayerBeginRun(pp);
        return;
    }

    if (!TEST(pp->Flags, PF_PLAYER_MOVED))
    {
        extern STATEp sg_NinjaCrawl[];

        NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Crawl);
    }

    // If the floor is far below you, fall hard instead of adjusting height
    //if (labs(pp->posz - pp->loz) > PLAYER_CRAWL_HEIGHT + PLAYER_FALL_HEIGHT)
    if (PlayerFallTest(pp, PLAYER_CRAWL_HEIGHT))
    {
        pp->jump_speed = Z(1);
        //pp->posz -= PLAYER_HEIGHT - PLAYER_CRAWL_HEIGHT;
        RESET(pp->Flags, PF_CRAWLING);
        DoPlayerBeginFall(pp);
        // call PlayerFall now seems to iron out a hitch before falling
        DoPlayerFall(pp);
        return;
    }

    if (TEST(sector[pp->cursectnum].extra, SECTFX_DYNAMIC_AREA))
    {
        pp->posz = pp->loz - PLAYER_CRAWL_HEIGHT;
    }

    DoPlayerBob(pp);
    DoPlayerCrawlHeight(pp);
}

void
DoPlayerBeginFly(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    extern STATEp sg_NinjaFly[];

    RESET(pp->Flags, PF_FALLING | PF_JUMPING | PF_CRAWLING);
    SET(pp->Flags, PF_FLYING);

    pp->friction = PLAYER_FLY_FRICTION;
    pp->floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerFly;

    pp->z_speed = -Z(10);
    pp->jump_speed = 0;
    pp->bob_amt = 0;
    pp->bob_ndx = 1024;

    ///DamageData[u->WeaponNum].Init(pp);

    NewStateGroup(pp->PlayerSprite, sg_PlayerNinjaFly);
}

int GetSinNdx(int range, int bob_amt)
{
    int amt;

    amt = Z(512) / range;

    return bob_amt * amt;
}

void PlayerWarpUpdatePos(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];

    if (Prediction)
        return;

    pp->oposx = pp->posx;
    pp->oposy = pp->posy;
    pp->oposz = pp->posz;
    DoPlayerZrange(pp);
    UpdatePlayerSprite(pp);
}

SWBOOL PlayerCeilingHit(PLAYERp pp, int zlimit)
{
    if (pp->posz < zlimit)
    {
        return TRUE;
    }

    return FALSE;
}

SWBOOL PlayerFloorHit(PLAYERp pp, int zlimit)
{
    if (pp->posz > zlimit)
    {
        return TRUE;
    }

    return FALSE;
}

void
DoPlayerFly(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    if (SectorIsUnderwaterArea(pp->cursectnum))
    {
        DoPlayerBeginDiveNoWarp(pp);
        return;
    }

    if (TEST_SYNC_KEY(pp, SK_CRAWL))
    {
        pp->z_speed += PLAYER_FLY_INC;

        if (pp->z_speed > PLAYER_FLY_MAX_SPEED)
            pp->z_speed = PLAYER_FLY_MAX_SPEED;
    }

    if (TEST_SYNC_KEY(pp, SK_JUMP))
    {
        pp->z_speed -= PLAYER_FLY_INC;

        if (pp->z_speed < -PLAYER_FLY_MAX_SPEED)
            pp->z_speed = -PLAYER_FLY_MAX_SPEED;
    }

    pp->z_speed = mulscale16(pp->z_speed, 58000);

    pp->posz += pp->z_speed;

    // Make the min distance from the ceiling/floor match bobbing amount
    // so the player never goes into the ceiling/floor

    // Only get so close to the ceiling
    if (PlayerCeilingHit(pp, pp->hiz + PLAYER_FLY_BOB_AMT + Z(8)))
    {
        pp->posz = pp->hiz + PLAYER_FLY_BOB_AMT + Z(8);
        pp->z_speed = 0;
    }

    // Only get so close to the floor
    if (PlayerFloorHit(pp, pp->loz - PLAYER_HEIGHT - PLAYER_FLY_BOB_AMT))
    {
        pp->posz = pp->loz - PLAYER_HEIGHT - PLAYER_FLY_BOB_AMT;
        pp->z_speed = 0;
    }

    if (PlayerFlyKey(pp))
    {
        RESET(pp->Flags, PF_FLYING);
        pp->bob_amt = 0;
        pp->bob_ndx = 0;
        DoPlayerBeginFall(pp);
        DoPlayerFall(pp);
        return;
    }

    DoPlayerMove(pp);
}


SPRITEp
FindNearSprite(SPRITEp sp, short stat)
{
    short fs, next_fs;
    int dist, near_dist = 15000;
    SPRITEp fp, near_fp = NULL;


    TRAVERSE_SPRITE_STAT(headspritestat[stat], fs, next_fs)
    {
        fp = &sprite[fs];

        dist = Distance(sp->x, sp->y, fp->x, fp->y);

        if (dist < near_dist)
        {
            near_dist = dist;
            near_fp = fp;
        }
    }

    return near_fp;
}

SWBOOL
PlayerOnLadder(PLAYERp pp)
{
    short sec, wal, spr;
    int dist, nx, ny;
    unsigned i;
    USERp u = User[pp->PlayerSprite];
    SPRITEp lsp;
    hitdata_t hitinfo;
    int dir;

    int neartaghitdist;
    short neartagsector, neartagwall, neartagsprite;

    static short angles[] =
    {
        30, -30
    };

    if (Prediction)
        return 0;

    neartag(pp->posx, pp->posy, pp->posz, pp->cursectnum, pp->pang,
            &neartagsector, &neartagwall, &neartagsprite,
            &neartaghitdist, 1024L+768L, NTAG_SEARCH_LO_HI, NULL);

    dir = DOT_PRODUCT_2D(pp->xvect, pp->yvect, sintable[NORM_ANGLE(pp->pang+512)], sintable[pp->pang]);

    if (dir < 0)
        return FALSE;

    if (neartagwall < 0 || wall[neartagwall].lotag != TAG_WALL_CLIMB)
        return FALSE;

    for (i = 0; i < SIZ(angles); i++)
    {
        neartag(pp->posx, pp->posy, pp->posz, pp->cursectnum, NORM_ANGLE(pp->pang + angles[i]),
                &sec, &wal, &spr,
                &dist, 600L, NTAG_SEARCH_LO_HI, NULL);

        if (wal < 0 || dist < 100 || wall[wal].lotag != TAG_WALL_CLIMB)
            return FALSE;

        FAFhitscan(pp->posx, pp->posy, pp->posz, pp->cursectnum,
                   sintable[NORM_ANGLE(pp->pang  + angles[i] + 512)],
                   sintable[NORM_ANGLE(pp->pang + angles[i])],
                   0,
                   &hitinfo, CLIPMASK_MISSILE);

        dist = DIST(pp->posx, pp->posy, hitinfo.pos.x, hitinfo.pos.y);

        if (hitinfo.sprite >= 0)
        {
            // if the sprite blocking you hit is not a wall sprite there is something between
            // you and the ladder
            if (TEST(sprite[hitinfo.sprite].cstat, CSTAT_SPRITE_BLOCK) &&
                !TEST(sprite[hitinfo.sprite].cstat, CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                return FALSE;
            }
        }
        else
        {
            // if you hit a wall and it is not a climb wall - forget it
            if (hitinfo.wall >= 0 && wall[hitinfo.wall].lotag != TAG_WALL_CLIMB)
                return FALSE;
        }
    }


    lsp = FindNearSprite(pp->SpriteP, STAT_CLIMB_MARKER);

    if (!lsp)
        return FALSE;

    // determine where the player is supposed to be in relation to the ladder
    // move out in front of the ladder
    nx = MOVEx(100, lsp->ang);
    ny = MOVEy(100, lsp->ang);

    // set angle player is supposed to face.
    pp->LadderAngle = NORM_ANGLE(lsp->ang + 1024);

#if DEBUG
    if (wall[wal].nextsector < 0)
    {
        TerminateGame();
        printf("Take out white wall ladder x = %d, y = %d",wall[wal].x, wall[wal].y);
        exit(0);
    }
#endif

    pp->LadderSector = wall[wal].nextsector;
    //DSPRINTF(ds, "Ladder Sector %d", pp->LadderSector);
    MONO_PRINT(ds);

    // set players "view" distance from the ladder - needs to be farther than
    // the sprite

    pp->lx = lsp->x + nx * 5;
    pp->ly = lsp->y + ny * 5;

    pp->pang = pp->LadderAngle;

    return TRUE;
}

SWBOOL DoPlayerTestCrawl(PLAYERp pp)
{
    if (labs(pp->loz - pp->hiz) < PLAYER_STANDING_ROOM)
        return TRUE;

    return FALSE;
}

int
PlayerInDiveArea(PLAYERp pp)
{
    SECTORp sectp;
    SWBOOL InMasked=FALSE;

    if (pp->lo_sectp)
    {
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //Attention: This changed on 07/29/97
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        sectp = &sector[pp->cursectnum];
        //sectp = pp->lo_sectp;
    }
    else
        return FALSE;

    if (TEST(sectp->extra, SECTFX_DIVE_AREA))
    {
        CheckFootPrints(pp);
        return TRUE;
    }

    return FALSE;
}

int
PlayerCanDive(PLAYERp pp)
{
    if (Prediction)
        return FALSE;

    // Crawl - check for diving
    if (TEST_SYNC_KEY(pp, SK_CRAWL) || pp->jump_speed > 0)
    {
        if (PlayerInDiveArea(pp))
        {
            pp->posz += Z(20);
            pp->z_speed = Z(20);
            pp->jump_speed = 0;

            if (pp->posz > pp->loz - Z(pp->WadeDepth) - Z(2))
            {
                DoPlayerBeginDive(pp);
            }

            return TRUE;
        }
    }

    return FALSE;
}

int
PlayerCanDiveNoWarp(PLAYERp pp)
{
    if (Prediction)
        return FALSE;

    // check for diving
    if (pp->jump_speed > 1400)
    {
        if (FAF_ConnectArea(pp->cursectnum))
        {
            short sectnum = pp->cursectnum;

            updatesectorz(pp->posx, pp->posy, SPRITEp_BOS(pp->SpriteP), &sectnum);

            if (SectorIsUnderwaterArea(sectnum))
            {
                pp->cursectnum = sectnum;
                pp->posz = sector[sectnum].ceilingz;

                pp->posz += Z(20);
                pp->z_speed = Z(20);
                pp->jump_speed = 0;

                PlaySound(DIGI_SPLASH1, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan);
                DoPlayerBeginDiveNoWarp(pp);
                return TRUE;
            }
        }
    }

    return FALSE;
}


int
GetOverlapSector(int x, int y, short *over, short *under)
{
    int i, found = 0;
    short sf[2]= {0,0};                       // sectors found

    if ((SectUser[*under] && SectUser[*under]->number >= 30000) || (SectUser[*over] && SectUser[*over]->number >= 30000))
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
        for (found = 0, i = 0; i < numsectors; i++)
        {
            if (inside(x, y, i))
            {
                sf[found] = i;
                found++;
                PRODUCTION_ASSERT(found <= 2);
            }
        }
    }

    if (!found)
    {
        TerminateGame();
        printf("GetOverlapSector x = %d, y = %d, over %d, under %d", x, y, *over, *under);
        exit(0);
    }

    PRODUCTION_ASSERT(found != 0);
    PRODUCTION_ASSERT(found <= 2);

    // the are overlaping - check the z coord
    if (found == 2)
    {
        if (sector[sf[0]].floorz > sector[sf[1]].floorz)
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
        *under = -1;
    }

    return found;
}

int
GetOverlapSector2(int x, int y, short *over, short *under)
{
    int i, nexti, found = 0;
    short sf[2]= {0,0};                       // sectors found

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
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_DIVE_AREA],i,nexti)
        {
            if (inside(x, y, sprite[i].sectnum))
            {
                sf[found] = sprite[i].sectnum;
                found++;
                PRODUCTION_ASSERT(found <= 2);
            }
        }

        for (stat = 0; stat < SIZ(UnderStatList); stat++)
        {
            TRAVERSE_SPRITE_STAT(headspritestat[UnderStatList[stat]],i,nexti)
            {
                // ignore underwater areas with lotag of 0
                if (sprite[i].lotag == 0)
                    continue;

                if (inside(x, y, sprite[i].sectnum))
                {
                    sf[found] = sprite[i].sectnum;
                    found++;
                    PRODUCTION_ASSERT(found <= 2);
                }
            }
        }
    }

    if (!found)
    {
        TerminateGame();
        printf("GetOverlapSector x = %d, y = %d, over %d, under %d", x, y, *over, *under);
        exit(0);
    }

    PRODUCTION_ASSERT(found != 0);
    PRODUCTION_ASSERT(found <= 2);

    // the are overlaping - check the z coord
    if (found == 2)
    {
        if (sector[sf[0]].floorz > sector[sf[1]].floorz)
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
        *under = -1;
    }

    return found;
}


void
DoPlayerWarpToUnderwater(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp sp = &sprite[pp->PlayerSprite];
    short i, nexti;
    SECT_USERp sectu = SectUser[pp->cursectnum];
    SPRITEp under_sp = NULL, over_sp = NULL;
    char Found = FALSE;
    short over, under;

    if (Prediction)
        return;


    // search for DIVE_AREA "over" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_DIVE_AREA], i, nexti)
    {
        over_sp = &sprite[i];

        if (TEST(sector[over_sp->sectnum].extra, SECTFX_DIVE_AREA) &&
            SectUser[over_sp->sectnum] &&
            SectUser[over_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    PRODUCTION_ASSERT(Found == TRUE);
    Found = FALSE;

    // search for UNDERWATER "under" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_UNDERWATER], i, nexti)
    {
        under_sp = &sprite[i];

        if (TEST(sector[under_sp->sectnum].extra, SECTFX_UNDERWATER) &&
            SectUser[under_sp->sectnum] &&
            SectUser[under_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    PRODUCTION_ASSERT(Found == TRUE);

    // get the offset from the sprite
    u->sx = over_sp->x - pp->posx;
    u->sy = over_sp->y - pp->posy;

    // update to the new x y position
    pp->posx = under_sp->x - u->sx;
    pp->posy = under_sp->y - u->sy;

    over  = over_sp->sectnum;
    under = under_sp->sectnum;

    if (GetOverlapSector(pp->posx, pp->posy, &over, &under) == 2)
    {
        pp->cursectnum = under;
    }
    else
        pp->cursectnum = over;

    pp->posz = sector[under_sp->sectnum].ceilingz + Z(6);

    pp->oposx = pp->posx;
    pp->oposy = pp->posy;
    pp->oposz = pp->posz;

    DoPlayerZrange(pp);
    return;
}

void
DoPlayerWarpToSurface(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp sp = &sprite[pp->PlayerSprite];
    short i, nexti;
    SECT_USERp sectu = SectUser[pp->cursectnum];
    short over, under;

    SPRITEp under_sp = NULL, over_sp = NULL;
    char Found = FALSE;

    if (Prediction)
        return;

    // search for UNDERWATER "under" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_UNDERWATER], i, nexti)
    {
        under_sp = &sprite[i];

        if (TEST(sector[under_sp->sectnum].extra, SECTFX_UNDERWATER) &&
            SectUser[under_sp->sectnum] &&
            SectUser[under_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    PRODUCTION_ASSERT(Found == TRUE);
    Found = FALSE;

    // search for DIVE_AREA "over" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_DIVE_AREA], i, nexti)
    {
        over_sp = &sprite[i];

        if (TEST(sector[over_sp->sectnum].extra, SECTFX_DIVE_AREA) &&
            SectUser[over_sp->sectnum] &&
            SectUser[over_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    PRODUCTION_ASSERT(Found == TRUE);

    // get the offset from the under sprite
    u->sx = under_sp->x - pp->posx;
    u->sy = under_sp->y - pp->posy;

    // update to the new x y position
    pp->posx = over_sp->x - u->sx;
    pp->posy = over_sp->y - u->sy;

    over = over_sp->sectnum;
    under = under_sp->sectnum;

    if (GetOverlapSector(pp->posx, pp->posy, &over, &under))
    {
        pp->cursectnum = over;
    }

    pp->posz = sector[over_sp->sectnum].floorz - Z(2);

    // set z range and wade depth so we know how high to set view
    DoPlayerZrange(pp);
    DoPlayerSetWadeDepth(pp);

    pp->posz -= Z(pp->WadeDepth);

    pp->oposx = pp->posx;
    pp->oposy = pp->posy;
    pp->oposz = pp->posz;

    return;
}


#if 1
void
DoPlayerDivePalette(PLAYERp pp)
{
    if (pp != Player + screenpeek) return;

    if ((pp->DeathType == PLAYER_DEATH_DROWN || TEST((Player+screenpeek)->Flags, PF_DIVING)) && !TEST(pp->Flags, PF_DIVING_IN_LAVA))
    {
        SetFadeAmt(pp,-1005,210); // Dive color , org color 208
    }
    else
    {
        // Put it all back to normal
        if (pp->StartColor == 210)
        {
            memcpy(pp->temp_pal, palette_data, sizeof(palette_data));
            memcpy(palookup[PALETTE_DEFAULT], DefaultPalette, 256 * 32);
            if (videoGetRenderMode() < REND_POLYMOST)
                COVERsetbrightness(gs.Brightness, &palette_data[0][0]);
            else
                videoFadePalette(0,0,0,0);
            pp->FadeAmt = 0;
        }
    }
}
#endif


void
DoPlayerBeginDive(PLAYERp pp)
{
    SPRITEp sp = &sprite[pp->PlayerSprite];
    USERp u = User[pp->PlayerSprite];

    if (Prediction)
        return;

    if (pp->Bloody) pp->Bloody = FALSE; // Water washes away the blood

    SET(pp->Flags, PF_DIVING);
    DoPlayerDivePalette(pp);
    DoPlayerNightVisionPalette(pp);

    if (pp == Player + screenpeek)
    {
        COVER_SetReverb(140); // Underwater echo
        pp->Reverb = 140;
    }

    SpawnSplash(pp->PlayerSprite);

    DoPlayerWarpToUnderwater(pp);
    OperateTripTrigger(pp);

    RESET(pp->Flags, PF_JUMPING | PF_FALLING);
    RESET(pp->Flags, PF_CRAWLING);
    RESET(pp->Flags, PF_LOCK_CRAWL);

    pp->friction = PLAYER_DIVE_FRICTION;
    pp->ceiling_dist = PLAYER_DIVE_CEILING_DIST;
    pp->floor_dist = PLAYER_DIVE_FLOOR_DIST;
    SET(sp->cstat, CSTAT_SPRITE_YCENTER);
    pp->DoPlayerAction = DoPlayerDive;

    //pp->z_speed = 0;

    pp->DiveTics = PLAYER_DIVE_TIME;
    pp->DiveDamageTics = 0;

    DoPlayerMove(pp); // needs to be called to reset the pp->loz/hiz variable
    ///DamageData[u->WeaponNum].Init(pp);

    NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Dive);

    DoPlayerDive(pp);
}

void DoPlayerBeginDiveNoWarp(PLAYERp pp)
{
    SPRITEp sp = &sprite[pp->PlayerSprite];
    USERp u = User[pp->PlayerSprite];

    if (Prediction)
        return;

    if (!SectorIsUnderwaterArea(pp->cursectnum))
        return;

    if (pp->Bloody) pp->Bloody = FALSE; // Water washes away the blood

    if (pp == Player + screenpeek)
    {
        COVER_SetReverb(140); // Underwater echo
        pp->Reverb = 140;
    }

    CheckFootPrints(pp);

    if (TEST(pp->lo_sectp->extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
    {
        SET(pp->Flags, PF_DIVING_IN_LAVA);
        u->DamageTics = 0;
    }

    SET(pp->Flags, PF_DIVING);
    DoPlayerDivePalette(pp);
    DoPlayerNightVisionPalette(pp);

    RESET(pp->Flags, PF_JUMPING | PF_FALLING);

    pp->friction = PLAYER_DIVE_FRICTION;
    pp->ceiling_dist = PLAYER_DIVE_CEILING_DIST;
    pp->floor_dist = PLAYER_DIVE_FLOOR_DIST;
    SET(sp->cstat, CSTAT_SPRITE_YCENTER);
    pp->DoPlayerAction = DoPlayerDive;
    pp->z_speed = 0;
    pp->DiveTics = PLAYER_DIVE_TIME;
    pp->DiveDamageTics = 0;
    DoPlayerMove(pp); // needs to be called to reset the pp->loz/hiz variable
    ///DamageData[u->WeaponNum].Init(pp);
    NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Dive);
    DoPlayerDive(pp);
}

void
DoPlayerStopDiveNoWarp(PLAYERp pp)
{
    SPRITEp sp = &sprite[pp->PlayerSprite];
    USERp u = User[pp->PlayerSprite];

    if (Prediction)
        return;

    if (!NoMeters) SetRedrawScreen(pp);

    if (pp->TalkVocHandle && FX_SoundActive(pp->TalkVocHandle))
    {
        FX_StopSound(pp->TalkVocHandle);
        pp->PlayerTalking = FALSE;
    }

    // stop diving no warp
    PlayerSound(DIGI_SURFACE,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow|v3df_doppler,pp);

    pp->bob_amt = 0;

    RESET(pp->Flags, PF_DIVING|PF_DIVING_IN_LAVA);
    DoPlayerDivePalette(pp);
    DoPlayerNightVisionPalette(pp);
    RESET(pp->SpriteP->cstat, CSTAT_SPRITE_YCENTER);
    if (pp == Player + screenpeek)
    {
        COVER_SetReverb(0);
        pp->Reverb = 0;
    }

    DoPlayerZrange(pp);
}

void
DoPlayerStopDive(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp sp = &sprite[pp->PlayerSprite];

    if (Prediction)
        return;

    if (!NoMeters) SetRedrawScreen(pp);

    if (pp->TalkVocHandle && FX_SoundActive(pp->TalkVocHandle))
    {
        FX_StopSound(pp->TalkVocHandle);
        pp->PlayerTalking = FALSE;
    }

    // stop diving with warp
    PlayerSound(DIGI_SURFACE,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow|v3df_doppler,pp);

    pp->bob_amt = 0;
    DoPlayerWarpToSurface(pp);
    DoPlayerBeginWade(pp);
    RESET(pp->Flags, PF_DIVING|PF_DIVING_IN_LAVA);

    DoPlayerDivePalette(pp);
    DoPlayerNightVisionPalette(pp);
    RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
    if (pp == Player + screenpeek)
    {
        COVER_SetReverb(0);
        pp->Reverb = 0;
    }
}

void
DoPlayerDiveMeter(PLAYERp pp)
{
    short color=0,i=0,metertics,meterunit;
    int y;
    extern char buffer[];


    if (NoMeters) return;

    // Don't draw bar from other players
    if (pp != Player+myconnectindex) return;

    if (!TEST(pp->Flags, PF_DIVING|PF_DIVING_IN_LAVA)) return;

    meterunit = PLAYER_DIVE_TIME / 30;
    if (meterunit > 0)
        metertics = pp->DiveTics / meterunit;
    else
        return;

    if (metertics <= 0 && !TEST(pp->Flags, PF_DIVING|PF_DIVING_IN_LAVA))
    {
        SetRedrawScreen(pp);
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

    rotatesprite((200+8)<<16,y<<16,65536L,0,5408,1,1,
                 (ROTATE_SPRITE_SCREEN_CLIP),0,0,xdim-1,ydim-1);

    rotatesprite((218+47)<<16,y<<16,65536L,0,5406-metertics,1,color,
                 (ROTATE_SPRITE_SCREEN_CLIP),0,0,xdim-1,ydim-1);
}

void
DoPlayerDive(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SECT_USERp sectu = SectUser[pp->cursectnum];

    // whenever your view is not in a water area
    if (!SectorIsUnderwaterArea(pp->cursectnum))
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
            PlayerSound(DIGI_WANGDROWNING, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_follow, pp);
            PlayerUpdateHealth(pp, -3 -(RANDOM_RANGE(7<<8)>>8));
            PlayerCheckDeath(pp, -1);
            if (TEST(pp->Flags, PF_DEAD))
                return;
        }
    }

    // underwater current
    if (pp->lo_sectp && TEST(pp->lo_sectp->extra, SECTFX_CURRENT))
    {
        DoPlayerCurrent(pp);
    }

    // while diving in lava
    // every DamageTics time take some damage
    if (TEST(pp->Flags, PF_DIVING_IN_LAVA))
    {
        if ((u->DamageTics -= synctics) < 0)
        {
            u->DamageTics = 30;   // !JIM! Was DAMAGE_TIME

            PlayerUpdateHealth(pp, -40);
        }
    }

    if (TEST_SYNC_KEY(pp, SK_CRAWL))
    {
        pp->z_speed += PLAYER_DIVE_INC;

        if (pp->z_speed > PLAYER_DIVE_MAX_SPEED)
            pp->z_speed = PLAYER_DIVE_MAX_SPEED;
    }

    if (TEST_SYNC_KEY(pp, SK_JUMP))
    {
        pp->z_speed -= PLAYER_DIVE_INC;

        if (pp->z_speed < -PLAYER_DIVE_MAX_SPEED)
            pp->z_speed = -PLAYER_DIVE_MAX_SPEED;
    }

    pp->z_speed = mulscale16(pp->z_speed, 58000);

    if (labs(pp->z_speed) < 16)
        pp->z_speed = 0;

    pp->posz += pp->z_speed;

    if (pp->z_speed < 0 && FAF_ConnectArea(pp->cursectnum))
    {
        if (pp->posz < sector[pp->cursectnum].ceilingz + Z(10))
        {
            short sectnum = pp->cursectnum;

            // check for sector above to see if it is an underwater sector also
            updatesectorz(pp->posx, pp->posy, sector[pp->cursectnum].ceilingz - Z(8), &sectnum);

            if (sectnum >= 0 && !SectorIsUnderwaterArea(sectnum))
            {
                // if not underwater sector we must surface
                // force into above sector
                pp->posz = sector[pp->cursectnum].ceilingz - Z(8);
                pp->cursectnum = sectnum;
                DoPlayerStopDiveNoWarp(pp);
                DoPlayerBeginRun(pp);
                return;
            }
        }
    }


    // Only get so close to the ceiling
    // if its a dive sector without a match or a UNDER2 sector with CANT_SURFACE set
    if (sectu && (sectu->number == 0 || TEST(sectu->flags, SECTFU_CANT_SURFACE)))
    {
        // for room over room water the hiz will be the top rooms ceiling
        if (pp->posz < pp->hiz + pp->ceiling_dist)
        {
            pp->posz = pp->hiz + pp->ceiling_dist;
        }
    }
    else
    {
        // close to a warping sector - stop diveing with a warp to surface
        // !JIM! FRANK - I added !pp->hi_sp so that you don't warp to surface when
        //     there is a sprite above you since getzrange returns a hiz < ceiling height
        //     if you are clipping into a sprite and not the ceiling.
        if (pp->posz < pp->hiz + Z(4) && !pp->hi_sp)
        {
            DoPlayerStopDive(pp);
            return;
        }
    }

    // Only get so close to the floor
    if (pp->posz >= pp->loz - PLAYER_DIVE_HEIGHT)
    {
        pp->posz = pp->loz - PLAYER_DIVE_HEIGHT;
    }

    // make player bob if sitting still
    if (!PLAYER_MOVING(pp) && pp->z_speed == 0 && pp->up_speed == 0)
    {
        DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHT, PLAYER_DIVE_BOB_AMT, 3);
    }
    // player is moving
    else
    {
        // if bob_amt is approx 0
        if (labs(pp->bob_amt) < Z(1))
        {
            pp->bob_amt = 0;
            pp->bob_ndx = 0;
        }
        // else keep bobbing until its back close to 0
        else
        {
            DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHT, PLAYER_DIVE_BOB_AMT, 3);
        }
    }

    // Reverse bobbing when getting close to the floor
    if (pp->posz + pp->bob_amt >= pp->loz - PLAYER_DIVE_HEIGHT)
    {
        pp->bob_ndx = NORM_ANGLE(pp->bob_ndx + ((1024 + 512) - pp->bob_ndx) * 2);
        DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHT, PLAYER_DIVE_BOB_AMT, 3);
    }
    // Reverse bobbing when getting close to the ceiling
    if (pp->posz + pp->bob_amt < pp->hiz + pp->ceiling_dist)
    {
        pp->bob_ndx = NORM_ANGLE(pp->bob_ndx + ((512) - pp->bob_ndx) * 2);
        DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHT, PLAYER_DIVE_BOB_AMT, 3);
    }

    DoPlayerMove(pp);

    // Random bubble sounds
    // if((RANDOM_RANGE(1000<<5)>>5) < 100)
    //     PlaySound(DIGI_BUBBLES, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_follow);

    if ((!Prediction && pp->z_speed && ((RANDOM_P2(1024<<5)>>5) < 64)) ||
        (PLAYER_MOVING(pp) && (RANDOM_P2(1024<<5)>>5) < 64))
    {
        short bubble;
        USERp bu;
        SPRITEp bp;
        int nx,ny;

        PlaySound(DIGI_BUBBLES, &pp->posx, &pp->posy, &pp->posz, v3df_none);
        bubble = SpawnBubble(pp->SpriteP - sprite);
        if (bubble >= 0)
        {
            bu = User[bubble];
            bp = &sprite[bubble];

            // back it up a bit to get it out of your face
            nx = MOVEx((128+64), NORM_ANGLE(bp->ang + 1024));
            ny = MOVEy((128+64), NORM_ANGLE(bp->ang + 1024));

            move_sprite(bubble, nx, ny, 0L, u->ceiling_dist, u->floor_dist, 0, synctics);
        }
    }
}

int
DoPlayerTestPlaxDeath(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    // landed on a paralax floor
    if (pp->lo_sectp && TEST(pp->lo_sectp->floorstat, FLOOR_STAT_PLAX))
    {
        PlayerUpdateHealth(pp, -u->Health);
        PlayerCheckDeath(pp, -1);
        return TRUE;
    }

    return FALSE;
}

void
DoPlayerCurrent(PLAYERp pp)
{
    int xvect, yvect;
    SECT_USERp sectu = SectUser[pp->cursectnum];
    int push_ret;

    if (!sectu)
        return;

    xvect = sectu->speed * synctics * (int) sintable[NORM_ANGLE(sectu->ang + 512)] >> 4;
    yvect = sectu->speed * synctics * (int) sintable[sectu->ang] >> 4;

    push_ret = pushmove((vec3_t *)pp, &pp->cursectnum, ((int)pp->SpriteP->clipdist<<2), pp->ceiling_dist, pp->floor_dist, CLIPMASK_PLAYER);
    if (push_ret < 0)
    {
        if (!TEST(pp->Flags, PF_DEAD))
        {
            USERp u = User[pp->PlayerSprite];

            PlayerUpdateHealth(pp, -u->Health);  // Make sure he dies!
            PlayerCheckDeath(pp, -1);

            if (TEST(pp->Flags, PF_DEAD))
                return;
        }
        return;
    }
    clipmove((vec3_t *)pp, &pp->cursectnum, xvect, yvect, ((int)pp->SpriteP->clipdist<<2), pp->ceiling_dist, pp->floor_dist, CLIPMASK_PLAYER);
    PlayerCheckValidMove(pp);
    pushmove((vec3_t *)pp, &pp->cursectnum, ((int)pp->SpriteP->clipdist<<2), pp->ceiling_dist, pp->floor_dist, CLIPMASK_PLAYER);
    if (push_ret < 0)
    {
        if (!TEST(pp->Flags, PF_DEAD))
        {
            USERp u = User[pp->PlayerSprite];

            PlayerUpdateHealth(pp, -u->Health);  // Make sure he dies!
            PlayerCheckDeath(pp, -1);

            if (TEST(pp->Flags, PF_DEAD))
                return;
        }
        return;
    }
}

void
DoPlayerFireOutWater(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    if (Prediction)
        return;

    if (pp->WadeDepth > 20)
    {
        if (u->flame >= 0)
            SetSuicide(u->flame);
        u->flame = -2;
    }
}

void
DoPlayerFireOutDeath(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    if (Prediction)
        return;

    if (u->flame >= 0)
        SetSuicide(u->flame);

    u->flame = -2;
}

void
DoPlayerBeginWade(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    // landed on a paralax floor?
    if (DoPlayerTestPlaxDeath(pp))
        return;

    RESET(pp->Flags, PF_JUMPING | PF_FALLING);
    RESET(pp->Flags, PF_CRAWLING);

    pp->friction = PLAYER_WADE_FRICTION;
    pp->floor_dist = PLAYER_WADE_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_WADE_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerWade;

    DoPlayerFireOutWater(pp);

    if (pp->jump_speed > 100)
        SpawnSplash(pp->PlayerSprite);

    // fix it so that you won't go under water unless you hit the water at a
    // certain speed
    if (pp->jump_speed > 0 && pp->jump_speed < 1300)
        pp->jump_speed = 0;

    ASSERT(u->ActorActionSet->Run);

    NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Run);
}


void
DoPlayerWade(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    int dot;
    int wadedir = 0;

    DoPlayerFireOutWater(pp);

    dot = DOT_PRODUCT_2D(pp->xvect, pp->yvect, sintable[NORM_ANGLE(pp->pang+512)], sintable[pp->pang]);

    if (dot < 0)
        wadedir = -2;
    else if (dot > 0)
        wadedir = 2;

    if (DebugOperate)
    {
        if (TEST_SYNC_KEY(pp, SK_OPERATE))
        {
            if (FLAG_KEY_PRESSED(pp, SK_OPERATE))
            {
                if (TEST(sector[pp->cursectnum].extra, SECTFX_OPERATIONAL))
                {
                    FLAG_KEY_RELEASE(pp, SK_OPERATE);
                    DoPlayerBeginOperate(pp);
                    pp->bob_amt = 0;
                    pp->bob_ndx = 0;
                    return;
                }
            }
        }
        else
        {
            FLAG_KEY_RESET(pp, SK_OPERATE);
        }
    }

    // Crawl if in small area automatically
    if (DoPlayerTestCrawl(pp) && pp->WadeDepth <= PLAYER_CRAWL_WADE_DEPTH)
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    // Crawl Commanded
    if (TEST_SYNC_KEY(pp, SK_CRAWL) && pp->WadeDepth <= PLAYER_CRAWL_WADE_DEPTH)
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    if (TEST_SYNC_KEY(pp, SK_JUMP))
    {
        if (FLAG_KEY_PRESSED(pp, SK_JUMP))
        {
            FLAG_KEY_RELEASE(pp, SK_JUMP);
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
        FLAG_KEY_RESET(pp, SK_JUMP);
    }

    if (PlayerFlyKey(pp))
    {
        //pp->InventoryTics[INVENTORY_FLY] = -99;
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

    if (pp->lo_sectp && TEST(pp->lo_sectp->extra, SECTFX_CURRENT))
    {
        DoPlayerCurrent(pp);
    }

    // Move about
    DoPlayerMove(pp);

    if (TEST(pp->Flags, PF_PLAYER_MOVED))
    {
        if (u->Rot != u->ActorActionSet->Run)
            NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Run);
    }
    else
    {
        if (u->Rot != u->ActorActionSet->Stand)
            NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Stand);
    }

    // If the floor is far below you, fall hard instead of adjusting height
    if (labs(pp->posz - pp->loz) > PLAYER_HEIGHT + PLAYER_FALL_HEIGHT)
    {
        pp->jump_speed = Z(1);
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
    if (labs(pp->posz - pp->loz) > PLAYER_HEIGHT + PLAYER_FALL_HEIGHT)
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

#if 0
    if ((TEST_SYNC_KEY(pp, SK_RUN) || TEST(pp->Flags, PF_LOCK_RUN)) && PlayerInDiveArea(pp))
    {
        DoPlayerBeginSwim(pp);
        pp->bob_amt = 0;
        pp->bob_ndx = 0;
        return;
    }
#endif

    if (!pp->WadeDepth)
    {
        DoPlayerBeginRun(pp);
        return;
    }
}


void
DoPlayerBeginOperateBoat(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    pp->floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerOperateBoat;

    // temporary set to get weapons down
    if (TEST(pp->sop->flags, SOBJ_HAS_WEAPON))
        SET(pp->Flags, PF_WEAPON_DOWN);

    ///DamageData[u->WeaponNum].Init(pp);

    ASSERT(u->ActorActionSet->Run);

    NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Run);
}

void
DoPlayerBeginOperateTank(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    pp->floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerOperateTank;

    // temporary set to get weapons down
    if (TEST(pp->sop->flags, SOBJ_HAS_WEAPON))
        SET(pp->Flags, PF_WEAPON_DOWN);

    ///DamageData[u->WeaponNum].Init(pp);

    ASSERT(u->ActorActionSet->Stand);

    NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Stand);
}

void
DoPlayerBeginOperateTurret(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    pp->floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerOperateTurret;

    // temporary set to get weapons down
    if (TEST(pp->sop->flags, SOBJ_HAS_WEAPON))
        SET(pp->Flags, PF_WEAPON_DOWN);

    ///DamageData[u->WeaponNum].Init(pp);

    ASSERT(u->ActorActionSet->Stand);

    NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Stand);
}

void FindMainSector(SECTOR_OBJECTp sop)
{
    // find the main sector - only do this once for each sector object
    if (sop->op_main_sector < 0)
    {
        int sx = sop->xmid;
        int sy = sop->ymid;

        PlaceSectorObject(sop, sop->ang, MAXSO, MAXSO);

        // set it to something valid
        sop->op_main_sector = 0;

        //COVERupdatesector(sx, sy, &sop->op_main_sector);
        //updatesectorz(sx, sy, sop->zmid - Z(8), &sop->op_main_sector);

        updatesectorz(sx, sy, sop->zmid, &sop->op_main_sector);

        //COVERupdatesector(sx, sy, &sop->op_main_sector);

        ////DSPRINTF(ds,"main sector %d, zmid %d",sop->op_main_sector, sop->zmid);
        //MONO_PRINT(ds);

        PlaceSectorObject(sop, sop->ang, sx, sy);
    }
}

void DoPlayerOperateMatch(PLAYERp pp, SWBOOL starting)
{
    SPRITEp sp;
    short i,nexti;

    if (!pp->sop)
        return;

    TRAVERSE_SPRITE_SECT(headspritesect[pp->sop->mid_sector], i, nexti)
    {
        sp = &sprite[i];

        if (sp->statnum == STAT_ST1 && sp->hitag == SO_DRIVABLE_ATTRIB)
        {
            if (starting)
            {
                if (SP_TAG5(sp))
                    DoMatchEverything(pp, SP_TAG5(sp), -1);
            }
            else
            {
                if (TEST_BOOL2(sp) && SP_TAG5(sp))
                    DoMatchEverything(pp, SP_TAG5(sp)+1, -1);
            }
            break;
        }
    }
}

void
DoPlayerBeginOperate(PLAYERp pp)
{
    SECTOR_OBJECTp PlayerOnObject(short sectnum_match);
    SECTOR_OBJECTp sop;
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];
    int cz, fz;
    int i;

    sop = PlayerOnObject(pp->cursectnum);

    // if someone already controlling it
    if (sop->controller)
        return;

    if (TEST(sop->flags, SOBJ_REMOTE_ONLY))
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
            sop->max_damage = User[sop->sp_child - sprite]->MaxHealth;
            VehicleSetSmoke(sop, NULL);
            RESET(sop->flags, SOBJ_BROKEN);
        }
        else
        {
            PlayerSound(DIGI_USEBROKENVEHICLE, &pp->posx, &pp->posy, &pp->posz, v3df_follow|v3df_dontpan,pp);
            return;
        }
    }

    pp->sop = pp->sop_control = sop;
    sop->controller = pp->SpriteP;

    pp->pang = sop->ang;
    pp->posx = sop->xmid;
    pp->posy = sop->ymid;
    COVERupdatesector(pp->posx, pp->posy, &pp->cursectnum);
    getzsofslope(pp->cursectnum, pp->posx, pp->posy, &cz, &fz);
    pp->posz = fz - PLAYER_HEIGHT;

    RESET(pp->Flags, PF_CRAWLING|PF_JUMPING|PF_FALLING|PF_LOCK_CRAWL);

    DoPlayerOperateMatch(pp, TRUE);

    // look for gun before trying to using it
    for (i = 0; sop->sp_num[i] != -1; i++)
    {
        if (sprite[sop->sp_num[i]].statnum == STAT_SO_SHOOT_POINT)
        {
            SET(sop->flags, SOBJ_HAS_WEAPON);
            break;
        }
    }

    DoPlayerResetMovement(pp);

    switch (sop->track)
    {
    case SO_TANK:
        if (pp->input.vel|pp->input.svel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
        pp->posz = fz - PLAYER_HEIGHT;
        DoPlayerBeginOperateTank(pp);
        break;
    case SO_TURRET_MGUN:
    case SO_TURRET:
        if (pp->input.angvel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
        pp->posz = fz - PLAYER_HEIGHT;
        DoPlayerBeginOperateTurret(pp);
        break;
    case SO_SPEED_BOAT:
        if (pp->input.vel|pp->input.svel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
        pp->posz = fz - PLAYER_HEIGHT;
        DoPlayerBeginOperateBoat(pp);
        break;
    default:
        return;
    }

}

void
DoPlayerBeginRemoteOperate(PLAYERp pp, SECTOR_OBJECTp sop)
{
    SECTOR_OBJECTp PlayerOnObject(short sectnum_match);
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];
    int cz, fz;
    int i;
    short save_sectnum;
    void PlayerRemoteReset(PLAYERp pp, short sectnum);

    pp->sop_remote = pp->sop = pp->sop_control = sop;
    sop->controller = pp->SpriteP;

    // won't operate - broken
    if (sop->max_damage != -9999 && sop->max_damage <= 0)
    {
        if (pp->InventoryAmount[INVENTORY_REPAIR_KIT])
        {
            UseInventoryRepairKit(pp);
            sop->max_damage = User[sop->sp_child - sprite]->MaxHealth;
            VehicleSetSmoke(sop, NULL);
            RESET(sop->flags, SOBJ_BROKEN);
        }
        else
        {
            PlayerSound(DIGI_USEBROKENVEHICLE, &pp->posx, &pp->posy, &pp->posz, v3df_follow|v3df_dontpan,pp);
            return;
        }
    }

    save_sectnum = pp->cursectnum;

    pp->pang = sop->ang;
    pp->posx = sop->xmid;
    pp->posy = sop->ymid;
    COVERupdatesector(pp->posx, pp->posy, &pp->cursectnum);
    getzsofslope(pp->cursectnum, pp->posx, pp->posy, &cz, &fz);
    pp->posz = fz - PLAYER_HEIGHT;

    RESET(pp->Flags, PF_CRAWLING|PF_JUMPING|PF_FALLING|PF_LOCK_CRAWL);

    DoPlayerOperateMatch(pp, TRUE);

    // look for gun before trying to using it
    for (i = 0; sop->sp_num[i] != -1; i++)
    {
        if (sprite[sop->sp_num[i]].statnum == STAT_SO_SHOOT_POINT)
        {
            SET(sop->flags, SOBJ_HAS_WEAPON);
            break;
        }
    }

    DoPlayerResetMovement(pp);

    PlayerToRemote(pp);
    PlayerRemoteInit(pp);

    switch (sop->track)
    {
    case SO_TANK:
        if (pp->input.vel|pp->input.svel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
        pp->posz = fz - PLAYER_HEIGHT;
        DoPlayerBeginOperateTank(pp);
        break;
    case SO_TURRET_MGUN:
    case SO_TURRET:
        if (pp->input.angvel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
        pp->posz = fz - PLAYER_HEIGHT;
        DoPlayerBeginOperateTurret(pp);
        break;
    case SO_SPEED_BOAT:
        if (pp->input.vel|pp->input.svel)
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
        pp->posz = fz - PLAYER_HEIGHT;
        DoPlayerBeginOperateBoat(pp);
        break;
    default:
        return;
    }

    PlayerRemoteReset(pp, save_sectnum);
}

void PlayerToRemote(PLAYERp pp)
{
    pp->remote.cursectnum = pp->cursectnum;
    pp->remote.lastcursectnum = pp->lastcursectnum;

    pp->remote.posx = pp->posx;
    pp->remote.posy = pp->posy;
    pp->remote.posz = pp->posz;

    pp->remote.xvect = pp->xvect;
    pp->remote.yvect = pp->yvect;
    pp->remote.oxvect = pp->oxvect;
    pp->remote.oyvect = pp->oyvect;
    pp->remote.slide_xvect = pp->slide_xvect;
    pp->remote.slide_yvect = pp->slide_yvect;
}

void RemoteToPlayer(PLAYERp pp)
{
    pp->cursectnum = pp->remote.cursectnum;
    pp->lastcursectnum = pp->remote.lastcursectnum;

    pp->posx = pp->remote.posx;
    pp->posy = pp->remote.posy;
    pp->posz = pp->remote.posz;

    pp->xvect = pp->remote.xvect;
    pp->yvect = pp->remote.yvect;
    pp->oxvect = pp->remote.oxvect;
    pp->oyvect = pp->remote.oyvect;
    pp->slide_xvect = pp->remote.slide_xvect;
    pp->slide_yvect = pp->remote.slide_yvect;
}

void PlayerRemoteReset(PLAYERp pp, short sectnum)
{
    pp->cursectnum = pp->lastcursectnum = sectnum;

    pp->posx = pp->remote_sprite->x;
    pp->posy = pp->remote_sprite->y;
    pp->posz = sector[sectnum].floorz - PLAYER_HEIGHT;

    pp->xvect = pp->yvect = pp->oxvect = pp->oyvect = pp->slide_xvect = pp->slide_yvect = 0;

    UpdatePlayerSprite(pp);
}

void PlayerRemoteInit(PLAYERp pp)
{
    pp->remote.xvect        = 0;
    pp->remote.yvect        = 0;
    pp->remote.oxvect       = 0;
    pp->remote.oyvect       = 0;
    pp->remote.slide_xvect  = 0;
    pp->remote.slide_yvect  = 0;
}

void
DoPlayerStopOperate(PLAYERp pp)
{
    RESET(pp->Flags, PF_WEAPON_DOWN);
    DoPlayerResetMovement(pp);
    DoTankTreads(pp);
    DoPlayerOperateMatch(pp, FALSE);
    StopSOsound(pp->sop->mid_sector);

    if (pp->sop_remote)
    {
        if (TEST_BOOL1(pp->remote_sprite))
            pp->pang = pp->oang = pp->remote_sprite->ang;
        else
            pp->pang = pp->oang = getangle(pp->sop_remote->xmid - pp->posx, pp->sop_remote->ymid - pp->posy);
    }

    if (pp->sop_control)
    {
        pp->sop_control->controller = NULL;
    }
    pp->sop_control = NULL;
    pp->sop_riding = NULL;
    pp->sop_remote = NULL;
    pp->sop = NULL;
    DoPlayerBeginRun(pp);
}

void
DoPlayerOperateTurret(PLAYERp pp)
{
    short oldang;
    short save_sectnum;

    if (TEST_SYNC_KEY(pp, SK_OPERATE))
    {
        if (FLAG_KEY_PRESSED(pp, SK_OPERATE))
        {
            FLAG_KEY_RELEASE(pp, SK_OPERATE);
            DoPlayerStopOperate(pp);
            return;
        }
    }
    else
    {
        FLAG_KEY_RESET(pp, SK_OPERATE);
    }

    if (pp->sop->max_damage != -9999 && pp->sop->max_damage <= 0)
    {
        DoPlayerStopOperate(pp);
        return;
    }

    save_sectnum = pp->cursectnum;

    if (pp->sop_remote)
        RemoteToPlayer(pp);

    DoPlayerMoveTurret(pp);

    if (pp->sop_remote)
    {
        PlayerToRemote(pp);
        PlayerRemoteReset(pp, save_sectnum);
    }
}


void
DoPlayerOperateBoat(PLAYERp pp)
{
    short oldang;
    short save_sectnum;

    if (TEST_SYNC_KEY(pp, SK_OPERATE))
    {
        if (FLAG_KEY_PRESSED(pp, SK_OPERATE))
        {
            FLAG_KEY_RELEASE(pp, SK_OPERATE);
            DoPlayerStopOperate(pp);
            return;
        }
    }
    else
    {
        FLAG_KEY_RESET(pp, SK_OPERATE);
    }

    if (pp->sop->max_damage != -9999 && pp->sop->max_damage <= 0)
    {
        DoPlayerStopOperate(pp);
        return;
    }

    save_sectnum = pp->cursectnum;

    if (pp->sop_remote)
        RemoteToPlayer(pp);

    DoPlayerMoveBoat(pp);

    if (pp->sop_remote)
    {
        PlayerToRemote(pp);
        PlayerRemoteReset(pp, save_sectnum);
    }
}

void
DoPlayerOperateTank(PLAYERp pp)
{
    short oldang;
    short save_sectnum;

    //ASSERT(!TEST_SYNC_KEY(pp, SK_OPERATE));
    if (TEST_SYNC_KEY(pp, SK_OPERATE))
    {
        if (FLAG_KEY_PRESSED(pp, SK_OPERATE))
        {
            FLAG_KEY_RELEASE(pp, SK_OPERATE);
            DoPlayerStopOperate(pp);
            return;
        }
    }
    else
    {
        FLAG_KEY_RESET(pp, SK_OPERATE);
    }

    if (pp->sop->max_damage != -9999 && pp->sop->max_damage <= 0)
    {
        DoPlayerStopOperate(pp);
        return;
    }

    save_sectnum = pp->cursectnum;

    if (pp->sop_remote)
        RemoteToPlayer(pp);

    DoPlayerMoveTank(pp);

    if (pp->sop_remote)
    {
        PlayerToRemote(pp);
        PlayerRemoteReset(pp, save_sectnum);
    }
}

void
DoPlayerDeathJump(PLAYERp pp)
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
            RESET(pp->Flags, PF_JUMPING);
            SET(pp->Flags, PF_FALLING);
            DoPlayerDeathFall(pp);
            return;
        }

        // adjust height by jump speed
        pp->posz += pp->jump_speed;

        // if player gets to close the ceiling while jumping
        //if (pp->posz < pp->hiz + Z(4))
        if (PlayerCeilingHit(pp, pp->hiz + Z(4)))
        {
            // put player at the ceiling
            pp->posz = pp->hiz + Z(4);

            // reverse your speed to falling
            pp->jump_speed = -pp->jump_speed;

            // start falling
            RESET(pp->Flags, PF_JUMPING);
            SET(pp->Flags, PF_FALLING);
            DoPlayerDeathFall(pp);
            return;
        }
    }
}

void
DoPlayerDeathFall(PLAYERp pp)
{
    short i;
    int loz;

    for (i = 0; i < synctics; i++)
    {
        // adjust jump speed by gravity
        pp->jump_speed += PLAYER_DEATH_GRAV;

        // adjust player height by jump speed
        pp->posz += pp->jump_speed;

#if 0
        if (pp->lo_sectp && TEST(pp->lo_sectp->extra, SECTFX_SINK))
        {
            loz = pp->lo_sectp->floorz - Z(SectUser[pp->lo_sectp - sector]->depth);
        }
        else
            loz = pp->loz;
#else
        if (pp->lo_sectp && TEST(pp->lo_sectp->extra, SECTFX_SINK))
        {
            loz = pp->lo_sectp->floorz;
        }
        else
            loz = pp->loz;
#endif

        if (PlayerFloorHit(pp, loz - PLAYER_DEATH_HEIGHT))
        //if (pp->posz > loz - PLAYER_DEATH_HEIGHT)
        {
            if (loz != pp->loz)
                SpawnSplash(pp->PlayerSprite);

            if (RANDOM_RANGE(1000) > 500)
                PlaySound(DIGI_BODYFALL1, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan);
            else
                PlaySound(DIGI_BODYFALL2, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan);

            pp->posz = loz - PLAYER_DEATH_HEIGHT;
            RESET(pp->Flags, PF_FALLING);
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

char *KilledPlayerMessage(PLAYERp pp, PLAYERp killer)
{
#define MAX_KILL_NOTES 16
    short rnd = STD_RANDOM_RANGE(MAX_KILL_NOTES);
    char *p1 = pp->PlayerName;
    char *p2 = killer->PlayerName;

    if (pp->HitBy == killer->PlayerSprite)
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
    return NULL;
};

void
DoPlayerDeathMessage(PLAYERp pp, PLAYERp killer)
{
    int pnum;
    short i;
    SWBOOL SEND_OK = FALSE;

    killer->KilledPlayer[pp-Player]++;

    if (pp == killer && pp == Player + myconnectindex)
    {
        sprintf(ds,"%s %s",pp->PlayerName,SuicideNote[STD_RANDOM_RANGE(MAX_SUICIDE)]);
        SEND_OK = TRUE;
    }
    else
    // I am being killed
    if (killer == Player + myconnectindex)
    {
        sprintf(ds,"%s",KilledPlayerMessage(pp,killer));
        SEND_OK = TRUE;
    }

    if (SEND_OK)
    {
        TRAVERSE_CONNECT(pnum)
        {
            if (pnum == myconnectindex)
                adduserquote(ds);
            else
                SW_SendMessage(pnum, ds);
        }
    }

}


void
DoPlayerBeginDie(PLAYERp pp)
{
    extern SWBOOL ReloadPrompt;
    void KillAllPanelInv(PLAYERp pp);
    void DoPlayerDeathDrown(PLAYERp pp);
    void pWeaponForceRest(PLAYERp pp);
    short bak;
    int choosesnd = 0;
    extern short GlobInfoStringTime;
    extern short QuickLoadNum;

    USERp u = User[pp->PlayerSprite];

    static void (*PlayerDeathFunc[MAX_PLAYER_DEATHS]) (PLAYERp) =
    {
        DoPlayerDeathFlip,
        DoPlayerDeathCrumble,
        DoPlayerDeathExplode,
        DoPlayerDeathFlip,
        DoPlayerDeathExplode,
        DoPlayerDeathDrown,
    };

    short random;

#define PLAYER_DEATH_TILT_VALUE       (32)
#define PLAYER_DEATH_HORIZ_UP_VALUE   (165)
#define PLAYER_DEATH_HORIZ_JUMP_VALUE (150)
#define PLAYER_DEATH_HORIZ_FALL_VALUE (50)

    if (Prediction)
        return;

    if (GodMode)
        return;

    // Override any previous talking, death scream has precedance
    if (pp->PlayerTalking)
    {
        if (FX_SoundActive(pp->TalkVocHandle))
            FX_StopSound(pp->TalkVocHandle);
        pp->PlayerTalking = FALSE;
        pp->TalkVocnum = -1;
        pp->TalkVocHandle = -1;
    }

    // Do the death scream
    choosesnd = RANDOM_RANGE(MAX_PAIN);

    PlayerSound(PlayerLowHealthPainVocs[choosesnd],&pp->posx,
                &pp->posy,&pp->posy,v3df_dontpan|v3df_doppler|v3df_follow,pp);

    if (!CommEnabled && numplayers <= 1 && QuickLoadNum >= 0)
    {
        ReloadPrompt = TRUE;
    }
    else
    {
        bak = GlobInfoStringTime;
        GlobInfoStringTime = 999;
        PutStringInfo(pp, "Press SPACE to restart");
        GlobInfoStringTime = bak;
    }

    if (pp->sop_control)
        DoPlayerStopOperate(pp);

    // if diving force death to drown type
    if (TEST(pp->Flags, PF_DIVING))
        pp->DeathType = PLAYER_DEATH_DROWN;

    RESET(pp->Flags, PF_JUMPING|PF_FALLING|PF_DIVING|PF_FLYING|PF_CLIMBING|PF_CRAWLING|PF_LOCK_CRAWL);

#if 0
    // get tilt value
    random = RANDOM_P2(1024);
    if (random < 128)
        pp->tilt_dest = 0;
    else if (random < 512+64)
        pp->tilt_dest = PLAYER_DEATH_TILT_VALUE;
    else
        pp->tilt_dest = -PLAYER_DEATH_TILT_VALUE;
#else
    pp->tilt_dest = 0;
#endif

    ActorCoughItem(pp->PlayerSprite);

    if (numplayers > 1)
    {
        // Give kill credit to player if necessary
        if (pp->Killer >= 0)
        {
            USERp ku = User[pp->Killer];

            ASSERT(ku);

            if (ku && ku->PlayerP)
            {
                if (pp == ku->PlayerP)
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
                        if (User[pp->PlayerSprite]->spal == ku->spal)
                        {
                            // Killed your team member
                            PlayerUpdateKills(pp, -1);
                            DoPlayerDeathMessage(pp, ku->PlayerP);
                        }
                        else
                        {
                            // killed another team member
                            PlayerUpdateKills(ku->PlayerP, 1);
                            DoPlayerDeathMessage(pp, ku->PlayerP);
                        }
                    }
                    else
                    {
                        // not playing team play
                        PlayerUpdateKills(ku->PlayerP, 1);
                        DoPlayerDeathMessage(pp, ku->PlayerP);
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

    SET(pp->Flags, PF_LOCK_HORIZ);

    pp->friction = PLAYER_RUN_FRICTION;
    pp->slide_xvect = pp->slide_yvect = 0;
    pp->floor_dist = PLAYER_WADE_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_WADE_CEILING_DIST;
    ASSERT(pp->DeathType < SIZ(PlayerDeathFunc));
    pp->DoPlayerAction = PlayerDeathFunc[pp->DeathType];
    pp->sop_control = NULL;
    pp->sop_remote = NULL;
    pp->sop_riding = NULL;
    pp->sop = NULL;
    RESET(pp->Flags, PF_TWO_UZI);

    NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Run);
    pWeaponForceRest(pp);

    switch (pp->DeathType)
    {
    case PLAYER_DEATH_DROWN:
    {
        SET(pp->Flags, PF_JUMPING);
        u->ID = NINJA_DEAD;
        pp->jump_speed = -200;
        NewStateGroup(pp->PlayerSprite, sg_PlayerDeath);
        DoFindGround(pp->PlayerSprite);
        DoBeginJump(pp->PlayerSprite);
        u->jump_speed = -300;
        break;
    }
    case PLAYER_DEATH_FLIP:
    case PLAYER_DEATH_RIPPER:

        //PlaySound(DIGI_SCREAM1, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_follow);

        SET(pp->Flags, PF_JUMPING);
        u->ID = NINJA_DEAD;
        pp->jump_speed = -300;
        NewStateGroup(pp->PlayerSprite, sg_PlayerDeath);
        //pp->ceiling_dist = Z(0);
        //pp->floor_dist = Z(0);

        RESET(pp->SpriteP->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        u->ceiling_dist = Z(10);
        u->floor_dist = Z(0);
        DoFindGround(pp->PlayerSprite);
        DoBeginJump(pp->PlayerSprite);
        u->jump_speed = -400;
        break;
    case PLAYER_DEATH_CRUMBLE:

        PlaySound(DIGI_BODYSQUISH1, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan);

        SET(pp->Flags, PF_DEAD_HEAD | PF_JUMPING);
        pp->jump_speed = -300;
        u->slide_vel = 0;
        SpawnShrap(pp->PlayerSprite,-1);
        SET(pp->SpriteP->cstat, CSTAT_SPRITE_YCENTER);
        NewStateGroup(pp->PlayerSprite, sg_PlayerHeadFly);
        u->ID = NINJA_Head_R0;
        pp->SpriteP->xrepeat = 48;
        pp->SpriteP->yrepeat = 48;
        // Blood fountains
        InitBloodSpray(pp->PlayerSprite,TRUE,105);
        break;
    case PLAYER_DEATH_EXPLODE:

        PlaySound(DIGI_BODYSQUISH1, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan);

        SET(pp->Flags, PF_DEAD_HEAD | PF_JUMPING);
        pp->jump_speed = -650;
        SpawnShrap(pp->PlayerSprite,-1);
        SET(pp->SpriteP->cstat, CSTAT_SPRITE_YCENTER);
        NewStateGroup(pp->PlayerSprite, sg_PlayerHeadFly);
        u->ID = NINJA_Head_R0;
        pp->SpriteP->xrepeat = 48;
        pp->SpriteP->yrepeat = 48;
        // Blood fountains
        InitBloodSpray(pp->PlayerSprite,TRUE,-1);
        InitBloodSpray(pp->PlayerSprite,TRUE,-1);
        InitBloodSpray(pp->PlayerSprite,TRUE,-1);
        break;
    case PLAYER_DEATH_SQUISH:

        PlaySound(DIGI_BODYCRUSHED1, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan);

        SET(pp->Flags, PF_DEAD_HEAD | PF_JUMPING);
        pp->jump_speed = 200;
        u->slide_vel = 800;
        SpawnShrap(pp->PlayerSprite, -1);
        SET(pp->SpriteP->cstat, CSTAT_SPRITE_YCENTER);
        NewStateGroup(pp->PlayerSprite, sg_PlayerHeadFly);
        u->ID = NINJA_Head_R0;
        pp->SpriteP->xrepeat = 48;
        pp->SpriteP->yrepeat = 48;
        // Blood fountains
        InitBloodSpray(pp->PlayerSprite,TRUE,105);
        break;

    }

    SET(pp->Flags, PF_DEAD);
    RESET(u->Flags,SPR_BOUNCE);
    RESET(pp->Flags, PF_HEAD_CONTROL);
}

int
DoPlayerDeathHoriz(PLAYERp pp, short target, short speed)
{
    if (pp->horiz > target)
    {
        pp->horiz -= speed;
        if (pp->horiz <= target)
            pp->horiz = target;
    }

    if (pp->horiz < target)
    {
        pp->horiz += speed;
        if (pp->horiz >= target)
            pp->horiz = target;
    }

    return pp->horiz == target;
}

int
DoPlayerDeathTilt(PLAYERp pp, short target, short speed)
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


void
DoPlayerDeathZrange(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];

    // make sure we don't land on a regular sprite
    DoFindGround(pp->PlayerSprite);

    // update player values with results from DoFindGround
//    pp->hiz = u->hiz;
    pp->loz = u->loz;
    pp->lo_sp = u->lo_sp;
    //pp->hi_sp = u->hi_sp;
    pp->lo_sectp = u->lo_sectp;
    //pp->hi_sectp = u->hi_sectp;
}

void DoPlayerDeathHurl(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];

    if (numplayers > 1)
    {
        if (TEST_SYNC_KEY(pp, SK_SHOOT))
        {
            if (FLAG_KEY_PRESSED(pp, SK_SHOOT))
            {


                SET(pp->Flags, PF_HEAD_CONTROL);
                NewStateGroup(pp->PlayerSprite, sg_PlayerHeadHurl);
                if (MoveSkip4 == 0)
                {
                    SpawnShrap(pp->PlayerSprite, -1);
                    if (RANDOM_RANGE(1000) > 400)
                        PlayerSound(DIGI_DHVOMIT, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_follow,pp);
                }
                return;
            }
        }
    }

    if (!TEST(pp->Flags, PF_JUMPING|PF_FALLING))
        NewStateGroup(pp->PlayerSprite, sg_PlayerHead);
}


void DoPlayerDeathFollowKiller(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];

    // if it didn't make it to this angle because of a low ceiling or something
    // continue on to it
    DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUE, 4);
    //DoPlayerDeathTilt(pp, pp->tilt_dest, 4 * synctics);

    // allow turning
    if ((TEST(pp->Flags, PF_DEAD_HEAD) && pp->input.angvel != 0) || TEST(pp->Flags, PF_HEAD_CONTROL))
    {
        // Allow them to turn fast
        PLAYER_RUN_LOCK(pp);

        DoPlayerTurn(pp);
        return;
    }

    // follow what killed you if its available
    if (pp->Killer > -1)
    {
        SPRITEp kp = &sprite[pp->Killer];
        short ang2,delta_ang;

        if (FAFcansee(kp->x, kp->y, SPRITEp_TOS(kp), kp->sectnum,
                      pp->posx, pp->posy, pp->posz, pp->cursectnum))
        {
            ang2 = getangle(kp->x - pp->posx, kp->y - pp->posy);

            delta_ang = GetDeltaAngle(ang2, pp->pang);
            pp->pang = NORM_ANGLE(pp->pang + (delta_ang >> 4));
        }
    }
}

void DoPlayerDeathCheckKeys(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];
    extern SWBOOL DemoMode,DemoDone;
    extern SWBOOL InputMode;

    //if (TEST_SYNC_KEY(pp, SK_OPERATE))
    if (TEST_SYNC_KEY(pp, SK_SPACE_BAR))
    {
        // Spawn a dead LoWang body for non-head deaths
        // Hey Frank, if you think of a better check, go ahead and put it in.
        if (PlayerFloorHit(pp, pp->loz - PLAYER_HEIGHT))
        {
            if (pp->DeathType == PLAYER_DEATH_FLIP || pp->DeathType == PLAYER_DEATH_RIPPER)
                QueueLoWangs(pp->PlayerSprite);
        }
        else
        {
            // If he's not on the floor, then gib like a mo-fo!
            InitBloodSpray(pp->PlayerSprite,TRUE,-1);
            InitBloodSpray(pp->PlayerSprite,TRUE,-1);
            InitBloodSpray(pp->PlayerSprite,TRUE,-1);
        }

        pClearTextLine(pp, TEXT_INFO_LINE(0));

        PlayerSpawnPosition(pp);

        NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Stand);
        pp->SpriteP->picnum = u->State->Pic;
        pp->SpriteP->xrepeat = pp->SpriteP->yrepeat = PLAYER_NINJA_XREPEAT;
        RESET(pp->SpriteP->cstat, CSTAT_SPRITE_YCENTER);
        pp->SpriteP->x = pp->posx;
        pp->SpriteP->y = pp->posy;
        pp->SpriteP->z = pp->posz+PLAYER_HEIGHT;
        pp->SpriteP->ang = pp->pang;

        DoSpawnTeleporterEffect(pp->SpriteP);
        PlaySound(DIGI_TELEPORT, &pp->posx, &pp->posy, &pp->posz, v3df_none);

        DoPlayerZrange(pp);

        pp->sop_control = NULL;
        pp->sop_remote = NULL;
        pp->sop_riding = NULL;
        pp->sop = NULL;

        RESET(pp->Flags, PF_WEAPON_DOWN|PF_WEAPON_RETRACT);
        RESET(pp->Flags, PF_DEAD);
        RESET(pp->Flags, PF_LOCK_HORIZ);
        RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
        SET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        sp->xrepeat = PLAYER_NINJA_XREPEAT;
        sp->yrepeat = PLAYER_NINJA_YREPEAT;

        //pp->tilt = 0;
        pp->horiz = pp->horizbase = 100;
        DoPlayerResetMovement(pp);
        u->ID = NINJA_RUN_R0;
        PlayerDeathReset(pp);

        if (pp == Player + screenpeek)
        {
            if (videoGetRenderMode() < REND_POLYMOST)
                COVERsetbrightness(gs.Brightness,&palette_data[0][0]);
            else
                videoFadePalette(0,0,0,0);
            //memcpy(&palette_data[0][0],&palette_data[0][0],768);
            memcpy(&pp->temp_pal[0],&palette_data[0][0],768);
        }

        pp->NightVision = FALSE;
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
            // restart the level in single play
            if (DemoMode)
                DemoDone = TRUE;
            else
                ExitLevel = TRUE;
        }

        DoPlayerFireOutDeath(pp);
    }
}

void
DoPlayerHeadDebris(PLAYERp pp)
{
    SECTORp sectp = &sector[pp->cursectnum];

    if (TEST(sectp->extra, SECTFX_SINK))
    {
        DoPlayerSpriteBob(pp, Z(8), Z(4), 3);
    }
    else
    {
        pp->bob_amt = 0;
    }
}

SPRITEp DoPlayerDeathCheckKick(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP, hp;
    USERp u = User[pp->PlayerSprite], hu;
    short i,nexti;
    unsigned stat,dist;
    int a,b,c;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];
            hu = User[i];

            if (i == pp->PlayerSprite)
                break;

            // don't set off mine
            if (!TEST(hp->extra, SPRX_PLAYER_OR_ENEMY))
                continue;

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, a, b, c);

            if (dist < hu->Radius + 100)
            {
                pp->Killer = i;

                u->slide_ang = getangle(sp->x - hp->x, sp->y - hp->y);
                u->slide_ang = NORM_ANGLE(u->slide_ang + (RANDOM_P2(128<<5)>>5) - 64);

                u->slide_vel = hp->xvel<<1;
                RESET(u->Flags,SPR_BOUNCE);
                pp->jump_speed = -500;
                NewStateGroup(pp->PlayerSprite, sg_PlayerHeadFly);
                SET(pp->Flags, PF_JUMPING);
                SpawnShrap(pp->PlayerSprite, -1);
                return hp;
            }
        }
    }

    DoPlayerZrange(pp);

    // sector stomper kick
    if (labs(pp->loz - pp->hiz) < SPRITEp_SIZE_Z(pp->SpriteP) - Z(8))
    {
        u->slide_ang = RANDOM_P2(2048);
        u->slide_vel = 1000;
        RESET(u->Flags,SPR_BOUNCE);
        pp->jump_speed = -100;
        NewStateGroup(pp->PlayerSprite, sg_PlayerHeadFly);
        SET(pp->Flags, PF_JUMPING);
        SpawnShrap(pp->PlayerSprite, -1);
        return NULL;
    }

    return NULL;
}


void DoPlayerDeathMoveHead(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP, hp;
    USERp u = User[pp->PlayerSprite], hu;
    int dax,day;
    short sectnum;

    dax = MOVEx(u->slide_vel, u->slide_ang);
    day = MOVEy(u->slide_vel, u->slide_ang);

    if ((u->ret = move_sprite(pp->PlayerSprite, dax, day, 0, Z(16), Z(16), 1, synctics)))
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_SPRITE:
        {
            short wall_ang, dang;
            short hit_sprite = -2;
            SPRITEp hsp;

            //PlaySound(DIGI_DHCLUNK, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan);

            hit_sprite = NORM_SPRITE(u->ret);
            hsp = &sprite[hit_sprite];

            if (!TEST(hsp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL))
                break;


            wall_ang = NORM_ANGLE(hsp->ang);
            dang = GetDeltaAngle(u->slide_ang, wall_ang);
            u->slide_ang = NORM_ANGLE(wall_ang + 1024 - dang);

            SpawnShrap(pp->PlayerSprite, -1);
            break;
        }
        case HIT_WALL:
        {
            short w,nw,wall_ang,dang;

            //PlaySound(DIGI_DHCLUNK, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan);

            w = NORM_WALL(u->ret);

            nw = wall[w].point2;
            wall_ang = NORM_ANGLE(getangle(wall[nw].x - wall[w].x, wall[nw].y - wall[w].y)-512);

            dang = GetDeltaAngle(u->slide_ang, wall_ang);
            u->slide_ang = NORM_ANGLE(wall_ang + 1024 - dang);

            SpawnShrap(pp->PlayerSprite, -1);
            break;
        }
        }
    }

    pp->posx = sp->x;
    pp->posy = sp->y;
    pp->cursectnum = sp->sectnum;

    // try to stay in valid area - death sometimes throws you out of the map
    sectnum = pp->cursectnum;
    COVERupdatesector(pp->posx, pp->posy, &sectnum);
    if (sectnum < 0)
    {
        pp->cursectnum = pp->lv_sectnum;
        changespritesect(pp->PlayerSprite, pp->lv_sectnum);
        pp->posx = sp->x = pp->lv_x;
        pp->posy = sp->y = pp->lv_y;
    }
    else
    {
        pp->lv_sectnum = sectnum;
        pp->lv_x = pp->posx;
        pp->lv_y = pp->posy;
    }
}

void DoPlayerDeathFlip(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];

    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if (TEST(pp->Flags,PF_JUMPING|PF_FALLING))
    {
        if (TEST(pp->Flags,PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUE, 2);
            if (MoveSkip2 == 0)
                DoJump(pp->PlayerSprite);
        }

        if (TEST(pp->Flags,PF_FALLING))
        {
            DoPlayerDeathFall(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUE, 4);
            if (MoveSkip2 == 0)
                DoFall(pp->PlayerSprite);
        }
    }
    else
    {
        DoPlayerDeathFollowKiller(pp);
    }

    DoPlayerDeathCheckKeys(pp);
}



void DoPlayerDeathDrown(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];

    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if (TEST(pp->Flags,PF_JUMPING|PF_FALLING))
    {
        if (TEST(pp->Flags,PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUE, 2);
            if (MoveSkip2 == 0)
                DoJump(pp->PlayerSprite);
        }

        if (TEST(pp->Flags,PF_FALLING))
        {
            pp->posz += Z(2);
            if (MoveSkip2 == 0)
                sp->z += Z(4);

            // Stick like glue when you hit the ground
            if (pp->posz > pp->loz - PLAYER_DEATH_HEIGHT)
            {
                pp->posz = pp->loz - PLAYER_DEATH_HEIGHT;
                RESET(pp->Flags, PF_FALLING);
            }
        }
    }

    DoPlayerDeathFollowKiller(pp);
    DoPlayerDeathCheckKeys(pp);
}


void DoPlayerDeathBounce(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];

    if (Prediction)
        return;

    if (pp->lo_sectp && TEST(pp->lo_sectp->extra, SECTFX_SINK))
    {
        int loz;

        RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        NewStateGroup(pp->PlayerSprite, sg_PlayerHead);
        u->slide_vel = 0;
        SET(u->Flags, SPR_BOUNCE);

        if (pp->lo_sectp && TEST(pp->lo_sectp->extra, SECTFX_SINK))
        {
            loz = pp->lo_sectp->floorz - Z(SectUser[pp->lo_sectp - sector]->depth);
        }
        else
            loz = pp->loz;


        return;
    }

    SET(u->Flags, SPR_BOUNCE);
    pp->jump_speed = -300;
    u->slide_vel >>= 2;
    u->slide_ang = NORM_ANGLE((RANDOM_P2(64<<8)>>8) - 32);
    SET(pp->Flags, PF_JUMPING);
    SpawnShrap(pp->PlayerSprite, -1);
}




void DoPlayerDeathCrumble(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];

    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if (TEST(pp->Flags,PF_JUMPING|PF_FALLING))
    {
        if (TEST(pp->Flags,PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_JUMP_VALUE, 4);
        }

        if (TEST(pp->Flags,PF_FALLING))
        {
            DoPlayerDeathFall(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_FALL_VALUE, 3);
        }

        if (!TEST(pp->Flags,PF_JUMPING|PF_FALLING))
        {
            if (!TEST(u->Flags, SPR_BOUNCE))
            {
                DoPlayerDeathBounce(pp);
                return;
            }

            RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
            NewStateGroup(pp->PlayerSprite, sg_PlayerHead);
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
    sp->z = pp->posz+PLAYER_DEAD_HEAD_FLOORZ_OFFSET;
    DoPlayerHeadDebris(pp);
}

void DoPlayerDeathExplode(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];

    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if (TEST(pp->Flags,PF_JUMPING|PF_FALLING))
    {
        if (TEST(pp->Flags,PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_JUMP_VALUE, 4);
        }

        if (TEST(pp->Flags,PF_FALLING))
        {
            DoPlayerDeathFall(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_JUMP_VALUE, 3);
        }

        if (!TEST(pp->Flags,PF_JUMPING|PF_FALLING))
        {
            if (!TEST(u->Flags, SPR_BOUNCE))
            {
                DoPlayerDeathBounce(pp);
                return;
            }

            RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
            NewStateGroup(pp->PlayerSprite, sg_PlayerHead);
        }
        else
        {
            DoPlayerDeathMoveHead(pp);
        }
    }
    else
    {
        // special line for amoeba
        //COVERupdatesector(pp->posx, pp->posy, &pp->cursectnum);

        DoPlayerDeathCheckKick(pp);
        DoPlayerDeathHurl(pp);
        DoPlayerDeathFollowKiller(pp);
        //pp->posz = pp->loz - PLAYER_DEATH_HEIGHT;
    }

    DoPlayerDeathCheckKeys(pp);
    sp->z = pp->posz+PLAYER_DEAD_HEAD_FLOORZ_OFFSET;
    DoPlayerHeadDebris(pp);
}

void
DoPlayerBeginRun(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    // Crawl if in small aread automatically
    if (DoPlayerTestCrawl(pp))
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    RESET(pp->Flags, PF_CRAWLING|PF_JUMPING|PF_FALLING|PF_LOCK_CRAWL|PF_CLIMBING);

    if (pp->WadeDepth)
    {
        DoPlayerBeginWade(pp);
        return;
    }

    pp->friction = PLAYER_RUN_FRICTION;
    pp->floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerRun;

    ///DamageData[u->WeaponNum].Init(pp);

    ASSERT(u->ActorActionSet->Run);

    if (TEST(pp->Flags, PF_PLAYER_MOVED))
        NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Run);
    else
        NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Stand);
}

void
DoPlayerRun(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];

    if (SectorIsUnderwaterArea(pp->cursectnum))
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
    if (TEST_SYNC_KEY(pp, SK_CRAWL))
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    // Jump
    if (TEST_SYNC_KEY(pp, SK_JUMP))
    {
        if (FLAG_KEY_PRESSED(pp, SK_JUMP))
        {
            FLAG_KEY_RELEASE(pp, SK_JUMP);
            // make sure you stand at full heights for jumps/double jumps
            //DoPlayerHeight(pp);
            //DoPlayerHeight(pp);
            //DoPlayerHeight(pp);
            //DoPlayerHeight(pp);
            //DoPlayerHeight(pp);
            pp->posz = pp->loz - PLAYER_HEIGHT;
            DoPlayerBeginJump(pp);
            return;
        }
    }
    else
    {
        FLAG_KEY_RESET(pp, SK_JUMP);
    }

    // Crawl lock
    // if (KEY_PRESSED(KEYSC_NUM))
    if (TEST_SYNC_KEY(pp, SK_CRAWL_LOCK))
    {
        if (FLAG_KEY_PRESSED(pp, SK_CRAWL_LOCK))
        {
            FLAG_KEY_RELEASE(pp, SK_CRAWL_LOCK);
            SET(pp->Flags, PF_LOCK_CRAWL);
            DoPlayerBeginCrawl(pp);
            return;
        }
    }
    else
    {
        FLAG_KEY_RESET(pp, SK_CRAWL_LOCK);
    }

    if (PlayerFlyKey(pp))
    {
        //pp->InventoryTics[INVENTORY_FLY] = -99;
        DoPlayerBeginFly(pp);
        return;
    }

    if (DebugOperate)
    {
        if (!TEST(pp->Flags, PF_DEAD) && !Prediction)
        {
            if (TEST_SYNC_KEY(pp, SK_OPERATE))
            {
                if (FLAG_KEY_PRESSED(pp, SK_OPERATE))
                {
                    if (TEST(sector[pp->cursectnum].extra, SECTFX_OPERATIONAL))
                    {
                        FLAG_KEY_RELEASE(pp, SK_OPERATE);
                        DoPlayerBeginOperate(pp);
                        return;
                    }
                    else if (TEST(sector[pp->cursectnum].extra, SECTFX_TRIGGER))
                    {
                        SPRITEp sp;

                        sp = FindNearSprite(pp->SpriteP, STAT_TRIGGER);
                        if (sp && SP_TAG5(sp) == TRIGGER_TYPE_REMOTE_SO)
                        {
                            pp->remote_sprite = sp;
                            FLAG_KEY_RELEASE(pp, SK_OPERATE);
                            ASSERT(pp->remote_sprite);
                            DoPlayerBeginRemoteOperate(pp, &SectorObject[SP_TAG7(pp->remote_sprite)]);
                            return;
                        }
                    }
                }
            }
            else
            {
                FLAG_KEY_RESET(pp, SK_OPERATE);
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

    if (u->Rot != sg_PlayerNinjaSword && u->Rot != sg_PlayerNinjaPunch)
    {
        if (TEST(pp->Flags, PF_PLAYER_MOVED))
        {
            if (u->Rot != u->ActorActionSet->Run)
                NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Run);
        }
        else
        {
            if (u->Rot != u->ActorActionSet->Stand)
                NewStateGroup(pp->PlayerSprite, u->ActorActionSet->Stand);
        }
    }

    // If the floor is far below you, fall hard instead of adjusting height
    if (PlayerFallTest(pp, PLAYER_HEIGHT))
    {
        pp->jump_speed = Z(1);
        DoPlayerBeginFall(pp);
        // call PlayerFall now seems to iron out a hitch before falling
        DoPlayerFall(pp);
        return;
    }

    if (TEST(sector[pp->cursectnum].extra, SECTFX_DYNAMIC_AREA))
    {
        pp->posz = pp->loz - PLAYER_HEIGHT;
    }

    // Adjust height moving up and down sectors
    DoPlayerHeight(pp);
}


int
PlayerStateControl(int16_t SpriteNum)
{
    USERp u;

    // Convienience var
    u = User[SpriteNum];

    u->Tics += synctics;

    // Skip states if too much time has passed
    while (u->Tics >= TEST(u->State->Tics, SF_TICS_MASK))
    {

        // Set Tics
        u->Tics -= TEST(u->State->Tics, SF_TICS_MASK);

        // Transition to the next state
        u->State = u->State->NextState;

        // !JIM! Added this so I can do quick calls in player states!
        // Need this in order for floor blood and footprints to not get called more than once.
        while (TEST(u->State->Tics, SF_QUICK_CALL))
        {
            // Call it once and go to the next state
            (*u->State->Animator)(SpriteNum);

            // if still on the same QUICK_CALL should you
            // go to the next state.
            if (TEST(u->State->Tics, SF_QUICK_CALL))
                u->State = u->State->NextState;
        }

        if (!u->State->Pic)
        {
            NewStateGroup(SpriteNum, (STATEp *) u->State->NextState);
        }
    }

    // Set picnum to the correct pic
    //sprite[SpriteNum].picnum = u->State->Pic;
    if (u->RotNum > 1)
        sprite[SpriteNum].picnum = u->Rot[0]->Pic;
    else
        sprite[SpriteNum].picnum = u->State->Pic;

    // Call the correct animator
    if (TEST(u->State->Tics, SF_PLAYER_FUNC))
        if (u->State->Animator)
            (*u->State->Animator)(SpriteNum);

    return 0;
}

void
MoveSkipSavePos(void)
{
    SPRITEp sp;
    USERp u;
    short i,nexti;
    short pnum;
    PLAYERp pp;

    MoveSkip8 = (MoveSkip8 + 1) & 7;
    MoveSkip4 = (MoveSkip4 + 1) & 3;
    MoveSkip2 ^= 1;

    // Save off player
    TRAVERSE_CONNECT(pnum)
    {
        pp = Player + pnum;

        pp->oposx = pp->posx;
        pp->oposy = pp->posy;
        pp->oposz = pp->posz;
        pp->oang = pp->pang;
        pp->ohoriz = pp->horiz;
    }

    // save off stats for skip4
    if (MoveSkip4 == 0)
    {
        short stat;

        for (stat = STAT_SKIP4_START; stat <= STAT_SKIP4_INTERP_END; stat++)
        {
            TRAVERSE_SPRITE_STAT(headspritestat[stat], i, nexti)
            {
                sp = &sprite[i];
                u = User[i];

                u->ox = sp->x;
                u->oy = sp->y;
                u->oz = sp->z;
            }
        }
    }

    // save off stats for skip2
    if (MoveSkip2 == 0)
    {
        short stat;

        for (stat = STAT_SKIP2_START; stat <= STAT_SKIP2_INTERP_END; stat++)
        {
            TRAVERSE_SPRITE_STAT(headspritestat[stat], i, nexti)
            {
                sp = &sprite[i];
                u = User[i];

                u->ox = sp->x;
                u->oy = sp->y;
                u->oz = sp->z;
            }
        }
    }
}


void PlayerTimers(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;

    InventoryTimer(pp);
}

void ChopsCheck(PLAYERp pp)
{
    extern SWBOOL HelpInputMode;
    extern int ChopTics;

    if (!UsingMenus && !HelpInputMode && !TEST(pp->Flags, PF_DEAD) && !pp->sop_riding && numplayers <= 1)
    {
        if ((pp->input.bits|pp->input.vel|pp->input.svel|pp->input.angvel|pp->input.aimvel) ||
            TEST(pp->Flags, PF_CLIMBING|PF_FALLING|PF_DIVING))
        {
            // Hit a input key or other reason to stop chops
            //if (pp->Chops && pp->Chops->State != pp->Chops->State->RetractState)
            if (pp->Chops)
            {
                if (!pp->sop_control) // specail case
                    RESET(pp->Flags, PF_WEAPON_DOWN);
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
                    SET(pp->Flags, PF_WEAPON_DOWN);
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
                    RESET(pp->Flags, PF_WEAPON_DOWN);
                    ChopsSetRetract(pp);
                }
            }
        }
    }
}

void PlayerGlobal(PLAYERp pp)
{
    // This is the place for things that effect the player no matter what hes
    // doing


    PlayerTimers(pp);

    if (TEST(pp->Flags, PF_RECOIL))
        DoPlayerRecoil(pp);

    if (!TEST(pp->Flags, PF_CLIP_CHEAT))
    {
        if (pp->hi_sectp && pp->lo_sectp)
        {
            int min_height;

#if 0
            if (TEST(pp->Flags, PF_JUMPING))
                // this is a special case for jumping.  Jumps have a very small
                // z height for the box so players can jump into small areas.
                min_height = PLAYER_MIN_HEIGHT_JUMP;
            else
                min_height = PLAYER_MIN_HEIGHT;
#else
            // just adjusted min height to something small to take care of all cases
            min_height = PLAYER_MIN_HEIGHT;
#endif

            if (labs(pp->loz - pp->hiz) < min_height)
            {
                if (!TEST(pp->Flags, PF_DEAD))
                {
                    ////DSPRINTF(ds,"Squish diff %d, min %d, cz %d, fz %d, lo %d, hi %d",labs(pp->loz - pp->hiz)>>8,min_height>>8, pp->ceiling_dist>>8, pp->floor_dist>>8,pp->lo_sectp-sector,pp->hi_sectp-sector);
                    //MONO_PRINT(ds);
                    PlayerUpdateHealth(pp, -User[pp->PlayerSprite]->Health);  // Make sure he dies!
                    PlayerCheckDeath(pp, -1);

                    if (TEST(pp->Flags, PF_DEAD))
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
        pp->circle_camera_ang = NORM_ANGLE(pp->circle_camera_ang + 14);

    if (pp->camera_check_time_delay > 0)
    {
        pp->camera_check_time_delay -= synctics;
        if (pp->camera_check_time_delay <= 0)
            pp->camera_check_time_delay = 0;
    }
}


void UpdateScrollingMessages(void)
{
    short i;

    // Update the scrolling multiplayer messages
    for (i=0; i<MAXUSERQUOTES; i++)
    {
        if (user_quote_time[i])
        {
            user_quote_time[i]--;

            if (user_quote_time[i] <= 0)
            {
                SetRedrawScreen(Player + myconnectindex);
                user_quote_time[i] = 0;
            }
            //if (!user_quote_time[i]) pub = NUMPAGES;
        }
    }

    if (gs.BorderNum > BORDER_BAR+1)
    {
        quotebot = quotebotgoal;
    }
    else
    {
        //     if ((klabs(quotebotgoal-quotebot) <= 16) && gs.BorderNum < BORDER_NONE+2)
        if ((klabs(quotebotgoal-quotebot) <= 16))
            quotebot += ksgn(quotebotgoal-quotebot);
        else
            quotebot = quotebotgoal;
    }
}

void UpdateConMessages(void)
{
    short i;

    if (!ConInputMode) return;

    if ((klabs(conbotgoal-conbot) <= 12))
        conbot += ksgn(conbotgoal-conbot);
    else
        conbot = conbotgoal;
}

void MultiPlayLimits(void)
{
    short pnum;
    PLAYERp pp;
    SWBOOL Done = FALSE;

    if (ExitLevel)
        return;

    if (gNet.MultiGameType != MULTI_GAME_COMMBAT)
        return;

    if (gNet.KillLimit)
    {
        TRAVERSE_CONNECT(pnum)
        {
            pp = Player + pnum;
            if (pp->Kills >= gNet.KillLimit)
            {
                Done = TRUE;
            }
        }
    }

    if (gNet.TimeLimit)
    {
        gNet.TimeLimitClock -= synctics;

        if ((gNet.TimeLimitClock%120) <= 3)
        {
            PlayerUpdateTimeLimit(Player + screenpeek);
        }

        if (gNet.TimeLimitClock <= 0)
            Done = TRUE;
    }

    if (Done)
    {
        gNet.TimeLimitClock = gNet.TimeLimit;

        // do not increment if level is 23 thru 28
        if (Level <= 22)
            Level++;

        ExitLevel = TRUE;
        FinishedLevel = TRUE;
    }
}

void PauseMultiPlay(void)
{
    static SWBOOL SavePrediction;
    PLAYERp pp;
    short pnum,p;
    extern SWBOOL GamePaused;

    // check for pause of multi-play game
    TRAVERSE_CONNECT(pnum)
    {
        pp = Player + pnum;

        if (TEST_SYNC_KEY(pp, SK_PAUSE))
        {
            if (FLAG_KEY_PRESSED(pp, SK_PAUSE))
            {
                FLAG_KEY_RELEASE(pp, SK_PAUSE);

                GamePaused ^= 1;

                if (GamePaused)
                {
                    short w,h;
#define MSG_GAME_PAUSED "Game Paused"
                    MNU_MeasureString(MSG_GAME_PAUSED, &w, &h);

                    TRAVERSE_CONNECT(p)
                    PutStringTimer(Player + p, TEXT_TEST_COL(w), 100, MSG_GAME_PAUSED, 999);

                    SavePrediction = PredictionOn;
                    PredictionOn = FALSE;
                    MUSIC_Pause();
                }
                else
                {
                    PredictionOn = SavePrediction;
                    MUSIC_Continue();
                    TRAVERSE_CONNECT(p)
                    pClearTextLine(Player + p, 100);
                }
            }
        }
        else
        {
            FLAG_KEY_RESET(pp, SK_PAUSE);
        }
    }
}

void
domovethings(void)
{
    extern SWBOOL DebugAnim;
    extern SWBOOL DebugPanel;
    extern SWBOOL DebugActor;
    extern SWBOOL DebugSector;
    extern SWBOOL DebugActorFreeze;
    extern SWBOOL ResCheat;
    extern int PlayClock;
    short i, j, pnum, nexti;
    int WeaponOperate(PLAYERp pp);
    extern SWBOOL GamePaused;
    PLAYERp pp;
    USERp u;
    SPRITEp sp;
    SWBOOL MyCommPlayerQuit(void);
    extern unsigned int MoveThingsCount;
    extern SWBOOL ScrollMode2D;
    extern SWBOOL ReloadPrompt;
    extern int FinishTimer;


    // grab values stored in the fifo and put them in the players vars
    TRAVERSE_CONNECT(i)
    {
        pp = Player + i;
        pp->input = pp->inputfifo[movefifoplc & (MOVEFIFOSIZ - 1)];
    }
    movefifoplc++;

    if (MyCommPlayerQuit())
        return;

    UpdateScrollingMessages();  // Update the multiplayer type messages
    UpdateConMessages();    // Update the console messages

#if SYNC_TEST
    if (/* CTW REMOVED !gTenActivated ||*/ !(movefifoplc & 0x3f))
        getsyncstat();
#ifdef DEBUG                            // in DEBUG mode even TEN does sync all the time
    else
        getsyncstat();
#endif
#endif

    // count the number of times this loop is called and use
    // for things like sync testing
    MoveThingsCount++;

    //RTS_Keys();

    // recording is done here
    if (DemoRecording)
    {
        TRAVERSE_CONNECT(i)
        {
            pp = Player + i;

            DemoBuffer[DemoRecCnt] = pp->input;

            if (DemoDebugMode)
            {
                DemoRecCnt++;
                if (DemoRecCnt > DemoDebugBufferMax-1)
                {
                    DemoDebugWrite();
                    DemoRecCnt = 0;
                }
            }
            else
            {
                DemoRecCnt++;
                if (DemoRecCnt > DEMO_BUFFER_MAX-1)
                {
                    DemoWriteBuffer();
                    DemoRecCnt = 0;
                }
            }
        }
    }

    totalsynctics += synctics;

    updateinterpolations();                  // Stick at beginning of domovethings
    short_updateinterpolations();            // Stick at beginning of domovethings
    MoveSkipSavePos();

#if 0
    {
        extern SWBOOL PauseKeySet;
        if (KEY_PRESSED(KEYSC_F5) && !(KEY_PRESSED(KEYSC_ALT) || KEY_PRESSED(KEYSC_RALT)) && !PauseKeySet)
        {
            KEY_PRESSED(KEYSC_F5) = 0;
            ResChange();
        }
    }
#endif


#if 0 // has been moved to draw code
    if (ResCheat)
    {
        ResCheat = FALSE;
        ResChange();
    }
#endif

    // check for pause of multi-play game
    if (CommEnabled)
        PauseMultiPlay();

    TRAVERSE_CONNECT(pnum)
    {
        pp = Player + pnum;
        DoPlayerMenuKeys(pp);
    }

    if (GamePaused)
    {
        if (!ReloadPrompt)
            return;
    }

    PlayClock += synctics;

    if (!DebugAnim)
        if (!DebugActorFreeze)
            DoAnim(synctics);

    // should pass pnum and use syncbits
    if (!DebugSector)
        DoSector();

    ProcessVisOn();
    if (MoveSkip4 == 0)
    {
        ProcessQuakeOn();
        ProcessQuakeSpot();
        JS_ProcessEchoSpot();
    }

    FAKETIMERHANDLER();

    SpriteControl();

    FAKETIMERHANDLER();

    TRAVERSE_CONNECT(pnum)
    {
        extern short screenpeek;
        extern SWBOOL PlayerTrackingMode;
        void pSpriteControl(PLAYERp pp);
        extern PLAYERp GlobPlayerP;
        extern SWBOOL ScrollMode2D;

        pp = Player + pnum;
        GlobPlayerP = pp;

        // auto tracking mode for single player multi-game
        if (numplayers <= 1 && PlayerTrackingMode && pnum == screenpeek && screenpeek != myconnectindex)
        {
            Player[screenpeek].pang = getangle(Player[myconnectindex].posx - Player[screenpeek].posx, Player[myconnectindex].posy - Player[screenpeek].posy);
        }

        if (!TEST(pp->Flags, PF_DEAD))
        {
#if DEBUG
            if (!DebugPanel)
                WeaponOperate(pp);
            if (!DebugSector)
                PlayerOperateEnv(pp);
#else
            WeaponOperate(pp);
            PlayerOperateEnv(pp);
#endif
        }

        FAKETIMERHANDLER();

        // do for moving sectors
        DoPlayerSectorUpdatePreMove(pp);
        ChopsCheck(pp);

        //if (!ScrollMode2D)
        (*pp->DoPlayerAction)(pp);

        UpdatePlayerSprite(pp);

#if DEBUG
        if (!DebugPanel)
#endif
        pSpriteControl(pp);

        PlayerStateControl(pp->PlayerSprite);

        DoPlayerSectorUpdatePostMove(pp);
        PlayerGlobal(pp);
    }


    MultiPlayLimits();

    if (MoveSkip8 == 0)     // 8=5x 4=10x, 2=20x, 0=40x per second
        DoUpdateSounds3D();

    CorrectPrediction(movefifoplc - 1);

    if (FinishTimer)
    {
        if ((FinishTimer -= synctics) <= 0)
        {
            FinishTimer = 0;
            ExitLevel = TRUE;
            FinishedLevel = TRUE;
        }
    }


    //if (DemoSyncRecord && !DemoPlaying)
    //    demosync_record();
}


extern unsigned char palette_data[256][3];      // Global palette array

void
InitAllPlayers(void)
{
    PLAYERp pp;
    PLAYERp pfirst = Player;
    int i;
    extern SWBOOL NewGame;
    //int fz,cz;

    //getzsofslope(pfirst->cursectnum, pfirst->posx, pfirst->posy, &cz, &fz);
    //pfirst->posz = fz - PLAYER_HEIGHT;
    pfirst->horiz = pfirst->horizbase = 100;

    // Initialize all [MAX_SW_PLAYERS] arrays here!
    for (pp = Player; pp < &Player[MAX_SW_PLAYERS]; pp++)
    {
        pp->posx = pp->oposx = pfirst->posx;
        pp->posy = pp->oposy = pfirst->posy;
        pp->posz = pp->oposz = pfirst->posz;
        pp->pang = pp->oang = pfirst->pang;
        pp->horiz = pp->ohoriz = pfirst->horiz;
        pp->cursectnum = pfirst->cursectnum;
        // set like this so that player can trigger something on start of the level
        pp->lastcursectnum = pfirst->cursectnum+1;

        //pp->MaxHealth = 100;

        pp->horizbase = pfirst->horizbase;
        pp->oldposx = 0;
        pp->oldposy = 0;
        pp->climb_ndx = 10;
        pp->Killer = -1;
        pp->Kills = 0;
        pp->bcnt = 0;
        pp->UziShellLeftAlt = 0;
        pp->UziShellRightAlt = 0;

        pp->ceiling_dist = PLAYER_RUN_CEILING_DIST;
        pp->floor_dist = PLAYER_RUN_FLOOR_DIST;

        pp->WpnGotOnceFlags = 0;
        pp->DoPlayerAction = DoPlayerBeginRun;
        pp->KeyPressFlags = 0xFFFFFFFF;
        memset(pp->KilledPlayer,0,sizeof(pp->KilledPlayer));

        if (NewGame)
        {
            for (i = 0; i < MAX_INVENTORY; i++)
            {
                //pp->InventoryAmount[i] = InventoryData[i].MaxInv = 0;
                pp->InventoryAmount[i] = 0;
                pp->InventoryPercent[i] = 0;
            }
        }

        // My palette flashing stuff
        pp->FadeAmt = 0;
        pp->FadeTics = 0;
        pp->StartColor = 0;
        pp->horizoff = 0;
        memcpy(&pp->temp_pal[0],&palette_data[0][0],768);

        INITLIST(&pp->PanelSpriteList);
    }
}

int SearchSpawnPosition(PLAYERp pp)
{
    PLAYERp opp; // other player
    SPRITEp sp;
    short pos_num;
    short pnum,spawn_sprite;
    SWBOOL blocked;

    do
    {
        // get a spawn position
        pos_num = RANDOM_RANGE(MAX_SW_PLAYERS);
        spawn_sprite = headspritestat[STAT_MULTI_START + pos_num];
        if (spawn_sprite <= -1)
            return 0;

        sp = &sprite[spawn_sprite];

        blocked = FALSE;

        // check to see if anyone else is blocking this spot
        TRAVERSE_CONNECT(pnum)
        {
            opp = &Player[pnum];

            if (opp != pp)  // don't test for yourself
            {
                if (FindDistance3D(sp->x - opp->posx, sp->y - opp->posy, (sp->z - opp->posz)>>4) < 1000)
                {
                    blocked = TRUE;
                    break;
                }
            }
        }
    }
    while (blocked);

    return pos_num;
}

SWBOOL SpawnPositionUsed[MAX_SW_PLAYERS_REG+1];

void
PlayerSpawnPosition(PLAYERp pp)
{
    SPRITEp sp;
    short pnum = pp - Player;
    short spawn_sprite = 0, pos_num = pnum;
    int fz,cz;
    int i;

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
        // start from the beginning
        spawn_sprite = headspritestat[STAT_MULTI_START + 0];
        break;
    case MULTI_GAME_COMMBAT:
    case MULTI_GAME_AI_BOTS:
        // start from random position after death
        if (TEST(pp->Flags, PF_DEAD))
        {
            pos_num = SearchSpawnPosition(pp);
        }

        spawn_sprite = headspritestat[STAT_MULTI_START + pos_num];
        break;
    case MULTI_GAME_COOPERATIVE:
        // start your assigned spot
        spawn_sprite = headspritestat[STAT_CO_OP_START + pos_num];
        break;
    }

    SpawnPositionUsed[pos_num] = TRUE;

    if (spawn_sprite < 0)
    {
        spawn_sprite = headspritestat[STAT_MULTI_START + 0];
        //TerminateGame();
        //printf("Map does not contain a spawn position for Player %d.", pp - Player);
        //exit(0);
    }

    ASSERT(spawn_sprite >= 0);

    sp = &sprite[spawn_sprite];


    pp->posx = pp->oposx = sp->x;
    pp->posy = pp->oposy = sp->y;
    pp->posz = pp->oposz = sp->z;
    pp->pang = pp->oang = sp->ang;
    pp->cursectnum = sp->sectnum;

    getzsofslope(pp->cursectnum, pp->posx, pp->posy, &cz, &fz);
    // if too close to the floor - stand up
    if (pp->posz > fz - PLAYER_HEIGHT)
    {
        pp->posz = pp->oposz = fz - PLAYER_HEIGHT;
    }
}


void
InitMultiPlayerInfo(void)
{
    PLAYERp pp;
    SPRITEp sp;
    short pnum, start0;
    unsigned stat;
    short SpriteNum, NextSprite, tag;
    static short MultiStatList[] =
    {
        STAT_MULTI_START,
        STAT_CO_OP_START
    };

    // this routine is called before SpriteSetup - process start positions NOW
    TRAVERSE_SPRITE_STAT(headspritestat[0], SpriteNum, NextSprite)
    {
        sp = &sprite[SpriteNum];

        tag = sp->hitag;

        if (sp->picnum == ST1)
        {
            switch (tag)
            {
            case MULTI_PLAYER_START:
                change_sprite_stat(SpriteNum, STAT_MULTI_START + sp->lotag);
                break;
            case MULTI_COOPERATIVE_START:
                change_sprite_stat(SpriteNum, STAT_CO_OP_START + sp->lotag);
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
            if (headspritestat[MultiStatList[stat] + 0] >= 0)
                continue;
        }

        start0 = SpawnSprite(MultiStatList[stat], ST1, NULL, pp->cursectnum, pp->posx, pp->posy, pp->posz, pp->pang, 0);
        ASSERT(start0 >= 0);
        if (User[start0])
        {
            FreeMem(User[start0]);
            User[start0] = NULL;
        }
        sprite[start0].picnum = ST1;
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
int
DoFootPrints(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (u->PlayerP)
    {
        if (FAF_ConnectArea(u->PlayerP->cursectnum))
            return 0;

        if (u->PlayerP->NumFootPrints > 0)
        {
            QueueFootPrint(SpriteNum);
        }
    }

    return 0;
}

void CheckFootPrints(PLAYERp pp)
{
    if (pp->NumFootPrints <= 0 || FootMode != WATER_FOOT)
    {
        // Hey, you just got your feet wet!
        pp->NumFootPrints = RANDOM_RANGE(10)+3;
        FootMode = WATER_FOOT;
    }
}


#include "saveable.h"

static saveable_code saveable_player_code[] =
{
    SAVE_CODE(DoPlayerSlide),
    //SAVE_CODE(DoPlayerBeginSwim),
    //SAVE_CODE(DoPlayerSwim),
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
    //SAVE_CODE(DoPlayerDie),
    SAVE_CODE(DoPlayerBeginOperateBoat),
    SAVE_CODE(DoPlayerBeginOperateTank),
    SAVE_CODE(DoPlayerBeginOperate),
    SAVE_CODE(DoPlayerOperateBoat),
    SAVE_CODE(DoPlayerOperateTank),
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
