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

CVAR(Bool, sw_nocenterview, false, CVAR_ARCHIVE)

BEGIN_SW_NS

void pSpriteControl(DSWPlayer* pp);
int WeaponOperate(DSWPlayer* pp);
SECTOR_OBJECT* PlayerOnObject(sectortype* sect_match);
void PlayerRemoteReset(DSWPlayer* pp, sectortype* sect);
void KillAllPanelInv(DSWPlayer* pp);
void DoPlayerDeathDrown(DSWPlayer* pp);
void pWeaponForceRest(DSWPlayer* pp);

#define SO_DRIVE_SOUND 2
#define SO_IDLE_SOUND 1

extern bool NoMeters;

#define PLAYER_CRAWL_WADE_DEPTH (30)

USERSAVE puser[MAX_SW_PLAYERS_REG];

//int16_t gNet.MultiGameType = MULTI_GAME_NONE;
bool NightVision = false;
extern int FinishAnim;

constexpr double INPUT_SCALE = (105. / 64.); // Old code used << 6 to get a Q14.18 value

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
constexpr double PLAYER_CLIMB_FRICTION = FixedToFloat(45056);
constexpr double TANK_FRICTION = FixedToFloat(53248);
constexpr double PLAYER_SLIDE_FRICTION = FixedToFloat(53248);

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

// These are a bunch of kens variables for the player

short NormalVisibility;

DSWActor* FindNearSprite(DSWActor*, short stat);
bool PlayerOnLadder(DSWPlayer* pp);
void DoPlayerSlide(DSWPlayer* pp);
void DoPlayerBeginSwim(DSWPlayer* pp);
void DoPlayerSwim(DSWPlayer* pp);
void DoPlayerWade(DSWPlayer* pp);
void DoPlayerBeginWade(DSWPlayer* pp);
void DoPlayerBeginCrawl(DSWPlayer* pp);
void DoPlayerCrawl(DSWPlayer* pp);
void DoPlayerRun(DSWPlayer* pp);
void DoPlayerBeginRun(DSWPlayer* pp);
void DoPlayerFall(DSWPlayer* pp);
void DoPlayerBeginFall(DSWPlayer* pp);
void DoPlayerJump(DSWPlayer* pp);
void DoPlayerBeginJump(DSWPlayer* pp);
void DoPlayerForceJump(DSWPlayer* pp);
void DoPlayerBeginFly(DSWPlayer* pp);
void DoPlayerFly(DSWPlayer* pp);
void DoPlayerBeginClimb(DSWPlayer* pp);
void DoPlayerClimb(DSWPlayer* pp);
void DoPlayerBeginDie(DSWPlayer* pp);
// void DoPlayerBeginOperateBoat(PLAYER* pp);
void DoPlayerBeginOperateVehicle(DSWPlayer* pp);
void DoPlayerBeginOperate(DSWPlayer* pp);
// void DoPlayerOperateBoat(PLAYER* pp);
void DoPlayerOperateVehicle(DSWPlayer* pp);
void DoPlayerOperateTurret(DSWPlayer* pp);
void DoPlayerBeginDive(DSWPlayer* pp);
void DoPlayerDive(DSWPlayer* pp);
void DoPlayerTeleportPause(DSWPlayer* pp);
bool PlayerFlyKey(void);
void OperateSectorObject(SECTOR_OBJECT* sop, DAngle newang, const DVector2& newpos);
void CheckFootPrints(DSWPlayer* pp);
bool DoPlayerTestCrawl(DSWPlayer* pp);
void DoPlayerDeathFlip(DSWPlayer* pp);
void DoPlayerDeathCrumble(DSWPlayer* pp);
void DoPlayerDeathExplode(DSWPlayer* pp);
void DoPlayerDeathFall(DSWPlayer* pp);

void PlayerCheckValidMove(DSWPlayer* pp);
void PlayerWarpUpdatePos(DSWPlayer* pp, const DVector3& oldpos);
void DoPlayerBeginDiveNoWarp(DSWPlayer* pp);
int PlayerCanDiveNoWarp(DSWPlayer* pp);
void DoPlayerCurrent(DSWPlayer* pp);
int GetOverlapSector2(const DVector2& pos, sectortype** over, sectortype** under);
void PlayerToRemote(DSWPlayer* pp);
void PlayerRemoteInit(DSWPlayer* pp);
void PlayerSpawnPosition(DSWPlayer* pp);
void processWeapon(DSWPlayer* const pp);

extern short target_ang;

void DSWPlayer::Clear()
{
    Super::Clear();
    // Quick'n dirty clear. PanelSpriteList must be preserved, though, but cleared itself.
    auto p = PanelSpriteList;
    memset(&lowActor, 0, sizeof(DSWPlayer) - myoffsetof(DSWPlayer, lowActor));
    PanelSpriteList = p;
    INITLIST(p);
}

DSWPlayer::DSWPlayer(uint8_t p) : DCorePlayer(p)
{
    PanelSpriteList = Create<DPanelSprite>();
    GC::WriteBarrier(this, PanelSpriteList);
}
void DSWPlayer::OnDestroy()
{
    pClearSpriteList(this);
    PanelSpriteList->Destroy();
    PanelSpriteList = nullptr;
}


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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerSpriteThrow(DSWPlayer* pp)
{
    if (!(pp->Flags & (PF_DIVING|PF_FLYING|PF_CRAWLING)))
    {
        if (pp->CurWpn == pp->Wpn[WPN_SWORD] && pp->GetActor()->user.Rot != sg_PlayerNinjaSword)
            NewStateGroup(pp->GetActor(), sg_PlayerNinjaSword);
        else
            NewStateGroup(pp->GetActor(), sg_PlayerNinjaPunch);
        //else
        //    NewStateGroup(pp->GetActor(), sg_PlayerNinjaThrow);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoPlayerSpriteReset(DSWActor* actor)
{
    DSWPlayer* pp;

    if (!actor->user.PlayerP)
        return 0;

    pp = actor->user.PlayerP;

    // need to figure out what frames to put sprite into
    if (pp->DoPlayerAction == DoPlayerCrawl)
        NewStateGroup(pp->GetActor(), actor->user.ActorActionSet->Crawl);
    else
    {
        if (pp->Flags & (PF_PLAYER_MOVED))
            NewStateGroup(pp->GetActor(), actor->user.ActorActionSet->Run);
        else
            NewStateGroup(pp->GetActor(), actor->user.ActorActionSet->Stand);
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

void pSetVisNorm(DPanelSprite* psp)
{
//    SetVisNorm();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

TARGET_SORT TargetSort[MAX_TARGET_SORT];
unsigned TargetSortCount;

static int CompareTarget(void const * a, void const * b)
{
    auto tgt1 = (TARGET_SORT const *)a;
    auto tgt2 = (TARGET_SORT const *)b;

    // will return a number less than 0 if tgt1 < tgt2
    return Sgn(tgt2->weight - tgt1->weight);
}

bool
FAFcansee(int32_t xs, int32_t ys, int32_t zs, int16_t sects,
          int32_t xe, int32_t ye, int32_t ze, int16_t secte);

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DSWActor* DoPickTarget(DSWActor* actor, DAngle max_delta_ang, int skip_targets)
{
    const int PICK_DIST = 2500;
    

    int16_t* shp;
    unsigned ndx;
    TARGET_SORT* ts;
    double ang_weight, dist_weight;

    // !JIM! Watch out for max_delta_ang of zero!
    if (max_delta_ang == nullAngle) max_delta_ang = minAngle;

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
            double dist = (actor->spr.pos - itActor->spr.pos).Length();
            if (dist > PICK_DIST)
                continue;

            if (skip_targets != 2) // Used for spriteinfo mode
            {
                // don't set off mine
                if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                    continue;
            }

            // Get the angle to the player
            DAngle angle2 = (itActor->spr.pos - actor->spr.pos).Angle();

            // Get the angle difference

            DAngle delta_ang = absangle(angle2, actor->spr.Angles.Yaw);

            // If delta_ang not in the range skip this one
            if (delta_ang > max_delta_ang)
                continue;

            DVector3 apos = actor->spr.pos;
            DVector2 ipos = itActor->spr.pos.XY();

            double ezh = ActorZOfTop(itActor) + (ActorSizeZ(itActor) * 0.25);
            double ezhm = ActorZOfTop(itActor) + (ActorSizeZ(itActor) * 0.5);
            double ezhl = ActorZOfBottom(itActor) - (ActorSizeZ(itActor) * 0.25);

            if (actor->hasU() && actor->user.PlayerP)
                apos.Z = actor->user.PlayerP->GetActor()->getOffsetZ();
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
                    if (dist < TargetSort[ndx].dst)
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
            ts->dangle = delta_ang;
            ts->dst = dist;
            // gives a value between 0 and 65535
            ang_weight = (max_delta_ang.Degrees() - ts->dangle.Degrees()) / max_delta_ang.Degrees();
            // gives a value between 0 and 65535
            dist_weight = ((PICK_DIST - ts->dst) / PICK_DIST);
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerResetMovement(DSWPlayer* pp)
{
    pp->vect.Zero();
    pp->ovect.Zero();
    pp->slide_vect.Zero();
    pp->Flags &= ~(PF_PLAYER_MOVED);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerTeleportPause(DSWPlayer* pp)
{
    DSWActor* actor = pp->GetActor();

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerTeleportToSprite(DSWPlayer* pp, DVector3& pos, DAngle ang)
{
    pp->GetActor()->spr.Angles.Yaw = ang;
    pp->GetActor()->spr.pos = pos;
    pp->GetActor()->backuploc();

    updatesector(pp->GetActor()->getPosWithOffsetZ(), &pp->cursector);
    pp->Flags2 |= (PF2_TELEPORTED);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerTeleportToOffset(DSWPlayer* pp)
{
    pp->GetActor()->backupvec2();

    updatesector(pp->GetActor()->getPosWithOffsetZ(), &pp->cursector);
    pp->Flags2 |= (PF2_TELEPORTED);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoSpawnTeleporterEffect(DSWActor* actor)
{
    extern STATE s_TeleportEffect[];
    DVector2 vect = actor->spr.Angles.Yaw.ToVector() * 32;

    auto effectActor = SpawnActor(STAT_MISSILE, 0, s_TeleportEffect, actor->sector(),
                         ActorVectOfTop(actor).plusZ(16) + vect, actor->spr.Angles.Yaw);

    SetActorZ(effectActor, effectActor->spr.pos);

    effectActor->spr.shade = -40;
    effectActor->spr.scale = DVector2(0.65625, 0.65625);
    effectActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    effectActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    effectActor->spr.cstat |= (CSTAT_SPRITE_ALIGNMENT_WALL);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoSpawnTeleporterEffectPlace(DSWActor* actor)
{
    extern STATE s_TeleportEffect[];

    auto effectActor = SpawnActor(STAT_MISSILE, 0, s_TeleportEffect, actor->sector(), ActorVectOfTop(actor).plusZ(16), actor->spr.Angles.Yaw);

    SetActorZ(effectActor, effectActor->spr.pos);

    effectActor->spr.shade = -40;
    effectActor->spr.scale = DVector2(0.65625, 0.65625);
    effectActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    effectActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    effectActor->spr.cstat |= (CSTAT_SPRITE_ALIGNMENT_WALL);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerWarpTeleporter(DSWPlayer* pp)
{
    auto ppActor = pp->GetActor();
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

    if ((act_warp = Warp(ppActor->spr.pos, &pp->cursector)) == nullptr)
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
        DoPlayerTeleportToSprite(pp, act_warp->spr.pos, act_warp->spr.Angles.Yaw);

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
            if (pnum != pp->pnum)
            {
                DSWPlayer* npp = getPlayer(pnum);

                // if someone already standing there
                if (npp->cursector == pp->cursector)
                {
                    PlayerUpdateHealth(npp, -npp->GetActor()->user.Health);  // Make sure he dies!
                    // telefraged by teleporting player
                    PlayerCheckDeath(npp, pp->GetActor());
                }
            }
        }

        break;
    }
    }

    ppActor->backuppos();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerSetWadeDepth(DSWPlayer* pp)
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
        if (pp->GetActor()->getOffsetZ() + PLAYER_HEIGHTF > pp->lo_sectp->floorz - FixedToInt(pp->lo_sectp->depth_fixed))
            pp->WadeDepth = FixedToInt(pp->lo_sectp->depth_fixed);
    }
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerViewOffset(DSWPlayer* pp)
{
    pp->GetActor()->viewzoffset -= (pp->GetActor()->viewzoffset + pp->height) * 0.375;
}

void DoPlayerHeight(DSWPlayer* pp)
{
    DoPlayerViewOffset(pp);
    pp->GetActor()->spr.pos.Z -= (pp->GetActor()->spr.pos.Z - pp->loz) * 0.375;
}

void DoPlayerJumpHeight(DSWPlayer* pp)
{
    if (pp->lo_sectp && (pp->lo_sectp->extra & SECTFX_DYNAMIC_AREA))
    {
        if (pp->GetActor()->getOffsetZ() + PLAYER_HEIGHTF > pp->loz)
        {
            pp->posZset(pp->loz - PLAYER_HEIGHTF);
            DoPlayerBeginRun(pp);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdatePlayerSpriteAngle(DSWPlayer* pp)
{
    DSWActor* plActor = pp->PlayerUnderActor;

    if (!Prediction && plActor)
    {
        plActor->backupang();
        plActor->spr.Angles.Yaw = pp->GetActor()->spr.Angles.Yaw;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void DoPlayerVehicleInputScaling(DSWPlayer* const pp, DAngle DRotator::* const angle, const double scale)
{
    DAngle& currYaw = pp->cmd.ucmd.ang.*angle;

    if (const auto angspeed = pp->sop->drive_angspeed)
    {
        const auto angslide = pp->sop->drive_angslide;
        currYaw = ((currYaw * angspeed) + (pp->lastcmd.ucmd.ang.*angle * (angslide - 1))) / angslide;
    }
    else
    {
        currYaw *= synctics * scale;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SlipSlope(DSWPlayer* pp)
{
    if (!pp->insector() || !pp->cursector->hasU())
        return;

    auto sectu = pp->cursector;

    if (!(sectu->flags & SECTFU_SLIDE_SECTOR) || !(pp->cursector->floorstat & CSTAT_SECTOR_SLOPE))
        return;

    DAngle ang = pp->cursector->walls[0].delta().Angle() + DAngle90;

	pp->vect += ang.ToVector() * pp->cursector->floorheinum / (1 << (sectu->speed + 4)); // todo confirm scale
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBob(DSWPlayer* pp)
{
    double amt;

    double dist = (pp->GetActor()->spr.pos.XY() - pp->GetActor()->opos.XY()).Length();

    if (dist > 32)
        dist = 0;

    // if running make a longer stride
    if (pp->cmd.ucmd.actions & SB_RUN)
    {
        amt = dist * (12. / 16.);
        dist *= FixedToFloat(26000);
    }
    else
    {
        amt = dist * (5. / 8.);
        dist *= FixedToFloat(32000);
    }

    // controls how fast you move through the sin table
    pp->bcnt += int(dist * 16);

    // wrap bcnt
    pp->bcnt &= 2047;

    // move pp->q16horiz up and down from 100 using sintable
    pp->bob_z = amt * BobVal(pp->bcnt);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginRecoil(DSWPlayer* pp, double pix_amt)
{
    pp->Flags |= (PF_RECOIL);

    pp->recoil_amt = pix_amt;
    pp->recoil_speed = 80;
    pp->recoil_ndx = 0;
    pp->recoil_ohorizoff = pp->recoil_horizoff = nullAngle;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerRecoil(DSWPlayer* pp)
{
    // controls how fast you move through the sin table
    pp->recoil_ndx += pp->recoil_speed;

    if (BobVal(pp->recoil_ndx) < 0)
    {
        pp->Flags &= ~(PF_RECOIL);
        pp->recoil_ohorizoff = pp->recoil_horizoff = nullAngle;
        return;
    }

    // move pp->q16horiz up and down
    pp->recoil_ohorizoff = pp->recoil_horizoff;
    pp->recoil_horizoff = DAngle::fromDeg(pp->recoil_amt * BobVal(pp->recoil_ndx));
}

//---------------------------------------------------------------------------
//
// for wading
//
//---------------------------------------------------------------------------

void DoPlayerSpriteBob(DSWPlayer* pp, double player_height, double bobamt, short bob_speed)
{
    pp->bob_ndx = (pp->bob_ndx + (synctics << bob_speed)) & 2047;
    pp->pbob_amt = bobamt * BobVal(pp->bob_ndx);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdatePlayerUnderSprite(DSWPlayer* pp)
{
    DSWActor* act_over = pp->GetActor();

    double water_level_z, zdiff;
    bool above_water, in_dive_area;

    if (Prediction)
        return;

    ASSERT(act_over->hasU());

    // dont bother spawning if you ain't really in the water
    water_level_z = act_over->sector()->floorz;

    // if not below water
    above_water = (ActorZOfBottom(act_over) <= water_level_z);
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
    zdiff = ActorZOfBottom(act_over) - water_level_z;

    // add diff to ceiling
    act_under->spr.pos.Z = act_under->sector()->ceilingz + zdiff;

    act_under->user.State = act_over->user.State;
    act_under->user.Rot = act_over->user.Rot;
    act_under->user.StateStart = act_over->user.StateStart;
    act_under->spr.setspritetexture(act_over->spr.spritetexture());
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdatePlayerSprite(DSWPlayer* pp)
{
    // Adjust player height according to the next action.
    if (!(pp->Flags & PF_DEAD))
    {
        pp->height = (pp->DoPlayerAction == DoPlayerCrawl) ? PLAYER_CRAWL_HEIGHTF : (pp->DoPlayerAction == DoPlayerDive) ? PLAYER_DIVE_HEIGHTF : PLAYER_HEIGHTF;
    }

    ChangeActorSect(pp->GetActor(), pp->cursector);

    UpdatePlayerUnderSprite(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerZrange(DSWPlayer* pp)
{
    Collision ceilhit, florhit;

    DSWActor* actor = pp->GetActor();
    if (!actor) return;

    // Don't let you fall if you're just slightly over a cliff
    // This function returns the highest and lowest z's
    // for an entire box, NOT just a point.  -Useful for clipping
    auto bakcstat = actor->spr.cstat;
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);

    FAFgetzrange(pp->GetActor()->getPosWithOffsetZ().plusZ(8), pp->cursector, &pp->hiz, &ceilhit, &pp->loz, &florhit, actor->clipdist - GETZRANGE_CLIP_ADJ, CLIPMASK_PLAYER);
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerSlide(DSWPlayer* pp)
{
    DSWActor* actor = pp->GetActor();

    int push_ret;

    if (pp->slide_vect.isZero())
        return;

    if (pp->sop)
        return;

    pp->slide_vect *= PLAYER_SLIDE_FRICTION;

    if (abs(pp->slide_vect.X) < 0.05 && abs(pp->slide_vect.Y) < 0.05)
        pp->slide_vect.Zero();

    push_ret = pushmove(pp->GetActor()->spr.pos, pp->GetActor()->getOffsetZ(), &pp->cursector, actor->clipdist, pp->p_ceiling_dist, pp->p_floor_dist, CLIPMASK_PLAYER);
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
    clipmove(pp->GetActor()->spr.pos, pp->GetActor()->getOffsetZ(), &pp->cursector, pp->slide_vect, actor->clipdist, pp->p_ceiling_dist, pp->p_floor_dist, CLIPMASK_PLAYER, coll);

    PlayerCheckValidMove(pp);
    push_ret = pushmove(pp->GetActor()->spr.pos, pp->GetActor()->getOffsetZ(), &pp->cursector, actor->clipdist, pp->p_ceiling_dist, pp->p_floor_dist, CLIPMASK_PLAYER);
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerCheckValidMove(DSWPlayer* pp)
{
    if (!pp->insector())
    {
        pp->GetActor()->restorepos();
        pp->cursector = pp->lastcursector;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerSectorBound(DSWPlayer* pp, double amt)
{
    if (!pp->insector())
        return;

    double cz,fz;

    // player should never go into a sector

    // was getting some problems with this
    // when jumping onto hight sloped sectors

    // call this routine to make sure he doesn't
    // called from DoPlayerMove() but can be called
    // from anywhere it is needed

    calcSlope(pp->cursector, pp->GetActor()->getPosWithOffsetZ(), &cz, &fz);

    if (pp->GetActor()->getOffsetZ() > fz - amt)
        pp->posZset(fz - amt);

    if (pp->GetActor()->getOffsetZ() < cz + amt)
        pp->posZset(cz + amt);

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerMove(DSWPlayer* pp)
{
    DSWActor* actor = pp->GetActor();
    int friction;
    int push_ret = 0;

    // If SO interpolation is disabled, make sure the player's aiming,
    // turning and movement still get appropriately interpolated.
    // We do this from here instead of MovePlayer, covering the case
    // the player gets pushed by a wall (e.g., on the boat in level 5).
    bool interpolate_ride = pp->sop_riding && (!cl_sointerpolation || CommEnabled);

    void SlipSlope(DSWPlayer* pp);

    SlipSlope(pp);

    pp->doViewYaw();
    pp->doYawInput();
    UpdatePlayerSpriteAngle(pp);

    pp->lastcursector = pp->cursector;

    if (PLAYER_MOVING(pp) == 0)
        pp->Flags &= ~(PF_PLAYER_MOVED);
    else
        pp->Flags |= (PF_PLAYER_MOVED);

    DoPlayerSlide(pp);

    pp->ovect = pp->vect;
    pp->PrevRollVel = pp->RollVel;

    pp->vect += pp->cmd.ucmd.vel.XY() * INPUT_SCALE;
    pp->RollVel += pp->svel * INPUT_SCALE;

    friction = pp->friction;
    if (!(pp->Flags & PF_SWIMMING) && pp->WadeDepth)
    {
        friction -= pp->WadeDepth * 100;
    }

	pp->vect *= FixedToFloat(friction);
    pp->RollVel *= FixedToFloat(friction);

    if (pp->Flags & (PF_FLYING))
    {
        // do a bit of weighted averaging
        pp->vect = (pp->vect + (pp->ovect*1))/2;
        pp->RollVel = (pp->RollVel + (pp->PrevRollVel*1))/2;
    }
    else if (pp->Flags & (PF_DIVING))
    {
        // do a bit of weighted averaging
        pp->vect = (pp->vect + (pp->ovect*2))/3;
        pp->RollVel = (pp->RollVel + (pp->PrevRollVel*2))/3;
    }

    if (abs(pp->vect.X) < 0.05 && abs(pp->vect.Y) < 0.05)
    {
        pp->vect.Zero();
        pp->RollVel = 0;
    }

	actor->vel.X = pp->vect.Length();

    pp->doRollInput(pp->Flags & (PF_SWIMMING|PF_DIVING));

    if (pp->Flags & (PF_CLIP_CHEAT))
    {
        auto sect = pp->cursector;
        if (interpolate_ride)
        {
            actor->backupvec2();
        }
		actor->spr.pos.XY() += pp->vect;
        updatesector(pp->GetActor()->getPosWithOffsetZ(), &sect);
        if (sect != nullptr)
            pp->cursector = sect;
    }
    else
    {
        push_ret = pushmove(pp->GetActor()->spr.pos, pp->GetActor()->getOffsetZ(), &pp->cursector, actor->clipdist, pp->p_ceiling_dist, pp->p_floor_dist - 16., CLIPMASK_PLAYER);

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
            pp->GetActor()->backupvec2();
        }

        auto save_cstat = actor->spr.cstat;
        actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
        Collision coll;
        updatesector(pp->GetActor()->getPosWithOffsetZ(), &pp->cursector);
        clipmove(pp->GetActor()->spr.pos, pp->GetActor()->getOffsetZ(), &pp->cursector, pp->vect, actor->clipdist, pp->p_ceiling_dist, pp->p_floor_dist, CLIPMASK_PLAYER, coll);

        actor->spr.cstat = save_cstat;
        PlayerCheckValidMove(pp);

        push_ret = pushmove(pp->GetActor()->spr.pos, pp->GetActor()->getOffsetZ(), &pp->cursector, actor->clipdist, pp->p_ceiling_dist, pp->p_floor_dist - 16., CLIPMASK_PLAYER);
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
        pp->GetActor()->backupz();
        pp->GetActor()->backupang();
    }

    // check for warp - probably can remove from CeilingHit
    const auto oldpos = actor->spr.pos;
    if (WarpPlane(actor->spr.pos, &pp->cursector, actor->getOffsetZ()))
    {
        PlayerWarpUpdatePos(pp, oldpos);
    }
    else
    {
        DoPlayerZrange(pp);
    }

    //PlayerSectorBound(pp, 1);

    DoPlayerSetWadeDepth(pp);

    pp->doViewPitch(pp->Flags & PF_CLIMBING);
    pp->doPitchInput();

    if (pp->insector() && (pp->cursector->extra & SECTFX_DYNAMIC_AREA))
    {
        if (pp->Flags & (PF_FLYING|PF_JUMPING|PF_FALLING))
        {
            if (pp->GetActor()->getOffsetZ() > pp->loz)
                pp->posZset(pp->loz - PLAYER_HEIGHTF);

            if (pp->GetActor()->getOffsetZ() < pp->hiz)
                pp->posZset(pp->hiz + PLAYER_HEIGHTF);
        }
        else if (pp->Flags & (PF_SWIMMING|PF_DIVING))
        {
            if (pp->GetActor()->getOffsetZ() > pp->loz)
                pp->posZset(pp->loz - PLAYER_DIVE_HEIGHTF);

            if (pp->GetActor()->getOffsetZ() < pp->hiz)
                pp->posZset(pp->hiz + PLAYER_DIVE_HEIGHTF);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerSectorUpdatePreMove(DSWPlayer* pp)
{
    auto sect = pp->cursector;

    if (sect == nullptr)
        return;

    if ((pp->cursector->extra & SECTFX_DYNAMIC_AREA))
    {
        updatesectorz(pp->GetActor()->getPosWithOffsetZ(), &sect);
        if (sect == nullptr)
        {
            sect = pp->cursector;
            updatesector(pp->GetActor()->getPosWithOffsetZ(), &sect);
        }
        ASSERT(sect);
    }
    else if (FAF_ConnectArea(sect))
    {
        updatesectorz(pp->GetActor()->getPosWithOffsetZ(), &sect);
        if (sect == nullptr)
        {
            sect = pp->cursector;
            updatesector(pp->GetActor()->getPosWithOffsetZ(), &sect);
        }
        ASSERT(sect);
    }

    pp->setcursector(sect);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerSectorUpdatePostMove(DSWPlayer* pp)
{
    auto sect = pp->cursector;
    double fz,cz;

    // need to do updatesectorz if in connect area
    if (sect != nullptr && FAF_ConnectArea(sect))
    {
        updatesectorz(pp->GetActor()->getPosWithOffsetZ(), &pp->cursector);

        // can mess up if below
        if (!pp->insector())
        {
            pp->setcursector(sect);

            // adjust the posz to be in a sector
            calcSlope(pp->cursector, pp->GetActor()->getPosWithOffsetZ(), &cz, &fz);
            if (pp->GetActor()->getOffsetZ() > fz)
                pp->posZset(fz);

            if (pp->GetActor()->getOffsetZ() < cz)
                pp->posZset(cz);

            // try again
            updatesectorz(pp->GetActor()->getPosWithOffsetZ(), &pp->cursector);
        }
    }
    else
    {
        PlayerSectorBound(pp, 1);
    }

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoTankTreads(DSWPlayer* pp)
{
    int i;
    sectortype* *sectp;
    int j;
    bool reverse = false;

    if (Prediction)
        return;

    double vel = pp->vect.Length() * 64;
	double dot =  pp->vect.dot(pp->GetActor()->spr.Angles.Yaw.ToVector());
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
                        actor->spr.Angles.Yaw += DAngle180;
                    }
                }
                else
                {
                    if (TEST_BOOL2(actor))
                    {
                        RESET_BOOL2(actor);
                        actor->spr.Angles.Yaw += DAngle180;
                    }
                }

                actor->vel.X = vel;
            }
            else if (actor->spr.statnum == STAT_FLOOR_PAN)
            {
                if (reverse)
                {
                    if (!TEST_BOOL2(actor))
                    {
                        SET_BOOL2(actor);
                        actor->spr.Angles.Yaw += DAngle180;
                    }
                }
                else
                {
                    if (TEST_BOOL2(actor))
                    {
                        RESET_BOOL2(actor);
                        actor->spr.Angles.Yaw += DAngle180;
                    }
                }

                actor->vel.X = vel;
            }
            else if (actor->spr.statnum == STAT_CEILING_PAN)
            {
                if (reverse)
                {
                    if (!TEST_BOOL2(actor))
                    {
                        SET_BOOL2(actor);
                        actor->spr.Angles.Yaw += DAngle180;
                    }
                }
                else
                {
                    if (TEST_BOOL2(actor))
                    {
                        RESET_BOOL2(actor);
                        actor->spr.Angles.Yaw += DAngle180;
                    }
                }

                actor->vel.X = vel;
            }
        }
    }


}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DriveCrush(DSWPlayer* pp, DVector2* quad)
{
    SECTOR_OBJECT* sop = pp->sop_control;
    short stat;
    sectortype* *sectp;

    if (MoveSkip4 == 0)
        return;

    // not moving - don't crush
    if ((pp->vect.isZero()) == 0 && pp->cmd.ucmd.ang.Yaw == nullAngle)
        return;

    // main sector
    SWSectIterator it(sop->op_main_sector);
    while (auto actor = it.Next())
    {
        if (testpointinquad(actor->spr.pos.XY(), quad))
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

            if (actor->spr.pos.Z < sop->crush_z)
                continue;

            SpriteQueueDelete(actor);
            KillActor(actor);
        }
    }

    // all enemys
    SWStatIterator it2(STAT_ENEMY);
    while (auto actor = it.Next())
    {
        if (testpointinquad(actor->spr.pos.XY(), quad))
        {
            //if (actor->spr.z < pp->posz)
            if (actor->spr.pos.Z < sop->crush_z)
                continue;

			double const vel = pp->vect.Length() * 8;
            if (vel < 70.3125)
            {
                DoActorBeginSlide(actor, pp->vect.Angle(), vel);
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
        if (testpointinquad(actor->spr.pos.XY(), quad))
        {
            if (actor->spr.pos.Z < sop->crush_z)
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

        if (testpointinquad(actor->spr.pos.XY(), quad))
        {
            int damage;

            //if (actor->spr.z < pp->posz)
            if (actor->spr.pos.Z < sop->crush_z)
                continue;

            damage = -(actor->user.Health + 100);
            PlayerDamageSlide(actor->user.PlayerP, damage, pp->GetActor()->spr.Angles.Yaw);
            PlayerUpdateHealth(actor->user.PlayerP, damage);
            PlayerCheckDeath(actor->user.PlayerP, pp->GetActor());
        }
    }


    // if it ends up actually in the drivable sector kill it
    for (sectp = sop->sectp; *sectp; sectp++)
    {
        it.Reset(*sectp);
        while (auto actor = it.Next())
        {
            // give some extra buffer
            if (actor->spr.pos.Z < sop->crush_z + 40)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerMoveVehicle(DSWPlayer* pp)
{
    DSWActor* actor = pp->sop->sp_child;
    if (!actor) return;
    DSWActor* plActor = pp->GetActor();
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
        const auto lastvel = pp->lastcmd.ucmd.vel.XY().Sum();
        const auto thisvel = pp->cmd.ucmd.vel.XY().Sum();

        if (thisvel && !lastvel)
            PlaySOsound(pp->sop->mid_sector,SO_DRIVE_SOUND);
        else if (!thisvel && lastvel)
            PlaySOsound(pp->sop->mid_sector,SO_IDLE_SOUND);
    }

    if (PLAYER_MOVING(pp) == 0)
        pp->Flags &= ~(PF_PLAYER_MOVED);
    else
        pp->Flags |= (PF_PLAYER_MOVED);

    pp->ovect = pp->vect;

    if (sop->drive_speed)
    {
        pp->vect = pp->cmd.ucmd.vel.XY() * sop->drive_speed * (70. / 1048576.);

        // does sliding/momentum
        pp->vect = (pp->vect + (pp->ovect * (sop->drive_slide-1)))/sop->drive_slide;
    }
    else
    {
        pp->vect += pp->cmd.ucmd.vel.XY() * INPUT_SCALE;
        pp->vect *= TANK_FRICTION;

        pp->vect = (pp->vect + (pp->ovect*1))/2;
    }

    if (abs(pp->vect.X) < 0.04883 && abs(pp->vect.Y) < 0.04883)
        pp->vect.X = pp->vect.Y = 0;

    pp->lastcursector = pp->cursector;
    double zz = plActor->getOffsetZ() + 10;

    DVector2 pos[4], opos[4];

    if (RectClip)
    {
        for (sectp = sop->sectp, wallcount = 0, j = 0; *sectp; sectp++, j++)
        {
            for(auto& wal : (*sectp)->walls)
            {
                if (wal.extra && (wal.extra & (WALLFX_LOOP_OUTER|WALLFX_LOOP_OUTER_SECONDARY)) == WALLFX_LOOP_OUTER)
                {
                    pos[count] = wal.pos;
                    opos[count] = sop->pmid.XY() - sop->orig[wallcount];
                    count++;
                }

                wallcount++;
            }
        }

        PRODUCTION_ASSERT(count == 4);
    }

    auto save_sect = pp->cursector;
    OperateSectorObject(pp->sop, plActor->spr.Angles.Yaw, { MAXSO, MAXSO });
    pp->setcursector(pp->sop->op_main_sector); // for speed

    double floordist = abs(zz - pp->sop->floor_loz);
    gameInput.ForceInputSync(pp->pnum);

    DoPlayerVehicleInputScaling(pp, &DRotator::Yaw, 0.125);
    DoPlayerVehicleInputScaling(pp, &DRotator::Pitch, 0.125);

    if (RectClip)
    {
        HitInfo hit{};
        int ret;

        auto save_cstat = plActor->spr.cstat;
        plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);

        if (pp->cmd.ucmd.ang.Yaw != nullAngle)
        {
            auto sum = plActor->spr.Angles.Yaw + pp->cmd.ucmd.ang.Yaw;
            if (RectClipTurn(pp, sum, pos, opos))
            {
                plActor->spr.Angles.Yaw = sum;
            }
        }

        ret = RectClipMove(pp, pos);
        DriveCrush(pp, pos);
        plActor->spr.cstat = save_cstat;

        if (!ret)
        {
			double vel = pp->vect.Length() * 1024;

            if (vel > 13000)
            {
                DVector3 hitpos((pos[0] + pos[1]) * 0.5, pp->cursector->floorz - 10);

                hitscan(hitpos, pp->cursector,
                    DVector3(plActor->spr.Angles.Yaw.ToVector() * 16, 0),
                    hit, CLIPMASK_PLAYER);

                if ((hit.hitpos.XY() - hitpos.XY()).LengthSquared() < 50 * 50)
                {
                    if (hit.hitWall)
                        actor->user.coll.setWall(wallindex(hit.hitWall));
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
                pp->vect.Zero();
                pp->ovect.Zero();
            }
        }
    }
    else
    {
        if (pp->cmd.ucmd.ang.Yaw != nullAngle)
        {
            auto sum = plActor->spr.Angles.Yaw + pp->cmd.ucmd.ang.Yaw;
            if (MultiClipTurn(pp, sum, zz, floordist))
            {
                plActor->spr.Angles.Yaw = sum;
            }
        }

        auto save_cstat = plActor->spr.cstat;
        plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
        if (pp->sop->clipdist)
        {
            Collision coll;
            clipmove(plActor->spr.pos, zz, &pp->cursector, pp->vect, pp->sop->clipdist, 4., floordist, CLIPMASK_PLAYER, actor->user.coll);
        }
        else
        {
            actor->user.coll = MultiClipMove(pp, zz, floordist);
        }
        plActor->spr.cstat = save_cstat;

        if (actor->user.coll.type != kHitNone)
        {
			double vel = pp->vect.Length() * 1024;

            if (vel > 13000)
            {
                VehicleMoveHit(actor);
                pp->slide_vect.X = -2 * pp->vect.X;
                pp->slide_vect.Y = -2 * pp->vect.Y;
                if (!(sop->flags & SOBJ_NO_QUAKE))
                    SetPlayerQuake(pp);
            }

            if (vel > 12000)
            {
                pp->vect.Zero();
                pp->ovect.Zero();
            }
        }
    }

    OperateSectorObject(pp->sop, plActor->spr.Angles.Yaw, plActor->spr.pos.XY());
    pp->cursector = save_sect; // for speed

    pp->doViewPitch(pp->Flags & PF_CLIMBING);
    pp->doPitchInput();

    DoTankTreads(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerMoveTurret(DSWPlayer* pp)
{
    const auto pact = pp->GetActor();

    if (!Prediction)
    {
        if (pp->cmd.ucmd.ang.Yaw.Degrees() && !pp->lastcmd.ucmd.ang.Yaw.Degrees())
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else if (!pp->cmd.ucmd.ang.Yaw.Degrees() && pp->lastcmd.ucmd.ang.Yaw.Degrees())
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
    }

    gameInput.ForceInputSync(pp->pnum);

    DoPlayerVehicleInputScaling(pp, &DRotator::Yaw, 0.125);
    DoPlayerVehicleInputScaling(pp, &DRotator::Pitch, 0.125);

    if (pp->cmd.ucmd.ang.Yaw != nullAngle)
    {
        DAngle new_ang = pact->spr.Angles.Yaw + pp->cmd.ucmd.ang.Yaw;

        if (pp->sop->limit_ang_center >= nullAngle)
        {
            DAngle diff = deltaangle(pp->sop->limit_ang_center, new_ang);

            if (abs(diff) >= pp->sop->limit_ang_delta)
            {
                if (diff < nullAngle)
                    new_ang = pp->sop->limit_ang_center - pp->sop->limit_ang_delta;
                else
                    new_ang = pp->sop->limit_ang_center + pp->sop->limit_ang_delta;
            }
        }

        pact->spr.Angles.Yaw = new_ang;
    }

    OperateSectorObject(pp->sop, pact->spr.Angles.Yaw, pp->sop->pmid.XY());

    if (PLAYER_MOVING(pp) == 0)
        pp->Flags &= ~(PF_PLAYER_MOVED);
    else
        pp->Flags |= (PF_PLAYER_MOVED);

    pp->doViewPitch(pp->Flags & PF_CLIMBING);
    pp->doPitchInput();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginJump(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    pp->Flags |= (PF_JUMPING);
    pp->Flags &= ~(PF_FALLING);
    pp->Flags &= ~(PF_CRAWLING);

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

    NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Jump);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginForceJump(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    pp->Flags |= (PF_JUMPING);
    pp->Flags &= ~(PF_FALLING|PF_CRAWLING|PF_CLIMBING);

    pp->JumpDuration = MAX_JUMP_DURATION;
    pp->DoPlayerAction = DoPlayerForceJump;

    pp->p_floor_dist = PLAYER_JUMP_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_JUMP_CEILING_DIST;
    pp->friction = PLAYER_JUMP_FRICTION;

    PlayerGravity = PLAYER_JUMP_GRAV;

    ///DamageData[plActor->user.WeaponNum].Init(pp);

    NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Jump);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool PlayerCeilingHit(DSWPlayer* pp, double zlimit)
{
    return (pp->GetActor()->getOffsetZ() < zlimit);
}

bool PlayerFloorHit(DSWPlayer* pp, double zlimit)
{
    return (pp->GetActor()->getOffsetZ() > zlimit);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerJump(DSWPlayer* pp)
{
    short i;

    // reset flag key for double jumps
    if (!(pp->cmd.ucmd.actions & SB_JUMP))
    {
        pp->KeyPressBits |= SB_JUMP;
    }

    // instead of multiplying by synctics, use a loop for greater accuracy
    for (i = 0; i < synctics; i++)
    {
        // PlayerGravity += synctics;  // See how increase gravity as we go?
        if (pp->cmd.ucmd.actions & SB_JUMP)
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
        pp->GetActor()->spr.pos.Z += pp->jump_speed * JUMP_FACTOR;

        // if player gets to close the ceiling while jumping
        if (PlayerCeilingHit(pp, pp->hiz + 4))
        {
            // put player at the ceiling
            pp->posZset(pp->hiz + 4);

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
            pp->posZset(pp->loz - pp->p_floor_dist);

            pp->jump_speed = 0;
            PlayerSectorBound(pp, 1);
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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerForceJump(DSWPlayer* pp)
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
        pp->GetActor()->spr.pos.Z += pp->jump_speed * JUMP_FACTOR;

        // if player gets to close the ceiling while jumping
        if (PlayerCeilingHit(pp, pp->hiz + 4))
        {
            // put player at the ceiling
            pp->posZset(pp->hiz + 4);

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginFall(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    pp->Flags |= (PF_FALLING);
    pp->Flags &= ~(PF_JUMPING);
    pp->Flags &= ~(PF_CRAWLING);

    pp->p_floor_dist = PLAYER_FALL_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_FALL_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerFall;
    pp->friction = PLAYER_FALL_FRICTION;

    // Only change to falling frame if you were in the jump frame
    // Otherwise an animation may be messed up such as Running Jump Kick
    if (plActor->user.Rot == plActor->user.ActorActionSet->Jump)
        NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Fall);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void StackedWaterSplash(DSWPlayer* pp)
{
    if (FAF_ConnectArea(pp->cursector))
    {
        auto sect = pp->cursector;

        updatesectorz(ActorVectOfBottom(pp->GetActor()), &sect);

        if (SectorIsUnderwaterArea(sect))
        {
            PlaySound(DIGI_SPLASH1, pp, v3df_dontpan);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerFall(DSWPlayer* pp)
{
    short i;
    double depth;

    // reset flag key for double jumps
    if (!(pp->cmd.ucmd.actions & SB_JUMP))
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
        pp->GetActor()->spr.pos.Z += pp->jump_speed * JUMP_FACTOR;

        if (pp->jump_speed > 2000)
        {
            PlayerSound(DIGI_FALLSCREAM, v3df_dontpan|v3df_doppler|v3df_follow,pp);
        }
        else if (pp->jump_speed > 1300 && !sw_nocenterview)
        {
            if (!(pp->cmd.ucmd.actions & SB_CENTERVIEW))
            {
                pp->cmd.ucmd.actions |= SB_CENTERVIEW;
            }
        }



        depth = GetZadjustment(pp->cursector, FLOOR_Z_ADJUST);
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
            pp->posZset(pp->hiz + pp->p_ceiling_dist);
            // don't return or anything - allow to fall until
            // hit floor
        }

        if (PlayerFloorHit(pp, pp->loz - PLAYER_HEIGHTF + recoil_amnt))
        {
            sectortype* sectp = pp->cursector;

            PlayerSectorBound(pp, 1);

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
            if (DoPlayerTestCrawl(pp) || (pp->cmd.ucmd.actions & SB_CROUCH))
            {
                pp->posZset(pp->loz - PLAYER_CRAWL_HEIGHTF);
            }
            else
            {
                // this was causing the z to snap immediately
                // changed it so it stays gradual

                pp->GetActor()->spr.pos.Z += recoil_amnt;
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
                    DSWActor* plActor = pp->GetActor();
                    PlayerUpdateHealth(pp, -plActor->user.Health);  // Make sure he dies!
                    plActor->user.Health = 0;
                }

                PlayerCheckDeath(pp, nullptr);

                if (pp->Flags & (PF_DEAD))
                    return;
            }

            if ((pp->cmd.ucmd.actions & SB_CROUCH) || pp->cmd.ucmd.vel.Z < 0)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginClimb(DSWPlayer* pp)
{
    DSWActor* actor = pp->GetActor();

    pp->Flags &= ~(PF_JUMPING|PF_FALLING);
    pp->Flags &= ~(PF_CRAWLING);

    pp->DoPlayerAction = DoPlayerClimb;

    pp->Flags |= (PF_CLIMBING|PF_WEAPON_DOWN);
    actor->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    NewStateGroup(pp->GetActor(), sg_PlayerNinjaClimb);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerClimb(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();
    int i;
    bool LadderUpdate = false;

    if (Prediction)
        return;

    pp->vect += pp->cmd.ucmd.vel.XY() * INPUT_SCALE;
    pp->vect *= PLAYER_CLIMB_FRICTION;
    if (abs(pp->vect.X) < 0.05 && abs(pp->vect.Y) < 0.05)
        pp->vect.X = pp->vect.Y = 0;

    double climbVel = pp->vect.Length();
    double dot = pp->vect.dot(pp->GetActor()->spr.Angles.Yaw.ToVector());
    if (dot < 0)
        climbVel = -climbVel;

    // need to rewrite this for FAF stuff

    // Jump off of the ladder
    if (pp->cmd.ucmd.actions & SB_JUMP)
    {
        pp->Flags &= ~(PF_CLIMBING|PF_WEAPON_DOWN);
        plActor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
        DoPlayerBeginJump(pp);
        return;
    }

    if (climbVel != 0)
    {
        // move player to center of ladder
        for (i = synctics; i; i--)
        {
            const double ADJ_AMT = 0.5;

            auto ppos = plActor->spr.pos.XY();
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
            plActor->spr.pos.SetXY(ppos);
        }
    }

    DoPlayerZrange(pp);

    if (!pp->LadderSector)
	{
		return;
	}

    // moving UP
    if (climbVel > 0)
    {
        plActor->spr.pos.Z -= climbVel;
        pp->climb_ndx &= 1023;

        // if player gets to close the ceiling while climbing
        if (PlayerCeilingHit(pp, pp->hiz))
        {
            // put player at the hiz
            pp->posZset(pp->hiz);
            NewStateGroup(pp->GetActor(), sg_PlayerNinjaClimb);
        }

        // if player gets to close the ceiling while climbing
        if (PlayerCeilingHit(pp, pp->hiz + 4))
        {
            // put player at the ceiling
            pp->posZset(pp->LadderSector->ceilingz + 4);
            NewStateGroup(pp->GetActor(), sg_PlayerNinjaClimb);
        }

        // if floor is ABOVE you && your head goes above it, do a jump up to
        // terrace

        if (pp->GetActor()->getOffsetZ() < pp->LadderSector->floorz - 6)
        {
            pp->jump_speed = PLAYER_CLIMB_JUMP_AMT;
            pp->Flags &= ~(PF_CLIMBING|PF_WEAPON_DOWN);
            plActor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
            DoPlayerBeginForceJump(pp);
        }
    }
    else
    // move DOWN
    if (climbVel < 0)
    {
        plActor->spr.pos.Z -= climbVel;
        pp->climb_ndx &= 1023;

        // if you are touching the floor
        if (PlayerFloorHit(pp, pp->loz - 4 - PLAYER_HEIGHTF))
        {
            // stand on floor
            pp->posZset(pp->loz - 4 - PLAYER_HEIGHTF);

            // if moving backwards start running
            if (climbVel < 0)
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
        NewStateGroup(pp->GetActor(), sg_PlayerNinjaClimb);
    }

    // setsprite to players location
    ChangeActorSect(pp->GetActor(), pp->cursector);

    pp->doViewPitch(pp->Flags & PF_CLIMBING);
    pp->doPitchInput();

    if (FAF_ConnectArea(pp->cursector))
    {
        updatesectorz(pp->GetActor()->getPosWithOffsetZ(), &pp->cursector);
        LadderUpdate = true;
    }

    const auto oldpos = pp->GetActor()->spr.pos;
    if (WarpPlane(pp->GetActor()->spr.pos, &pp->cursector, pp->GetActor()->getOffsetZ()))
    {
        PlayerWarpUpdatePos(pp, oldpos);
        LadderUpdate = true;
    }

    if (LadderUpdate)
    {
        int nx,ny;
        HitInfo near;

        // constantly look for new ladder sector because of warping at any time
        neartag(pp->GetActor()->getPosWithOffsetZ(), pp->cursector, pp->GetActor()->spr.Angles.Yaw, near, 50., NT_Lotag | NT_Hitag | NT_NoSpriteCheck);

        if (near.hitWall)
        {
            auto lActor = FindNearSprite(pp->GetActor(), STAT_CLIMB_MARKER);
            if (!lActor) return;

            // determine where the player is supposed to be in relation to the ladder
            // move out in front of the ladder
			auto nvec = lActor->spr.Angles.Yaw.ToVector() * 31.25;

            // set ladder sector
            pp->LadderSector = near.hitWall->twoSided()? near.hitWall->nextSector() : near.hitWall->sectorp();

            // set players "view" distance from the ladder - needs to be farther than
            // the sprite

            pp->LadderPosition = lActor->spr.pos.XY() + nvec;

            pp->GetActor()->spr.Angles.Yaw = lActor->spr.Angles.Yaw + DAngle180;
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoPlayerWadeSuperJump(DSWPlayer* pp)
{
    HitInfo hit{};
    unsigned i;
    double zh = pp->cursector->floorz - pp->WadeDepth - 2;

    if (Prediction) return false;   // !JIM! 8/5/97 Teleporter FAFhitscan SuperJump bug.

    //for (i = 0; i < SIZ(angs); i++)
    {
        FAFhitscan(DVector3(pp->GetActor()->spr.pos.XY(), zh), pp->cursector, DVector3(pp->GetActor()->spr.Angles.Yaw.ToVector() * 1024, 0), hit, CLIPMASK_MISSILE);

        if (hit.hitWall != nullptr && hit.hitSector != nullptr)
        {
            hit.hitSector = hit.hitWall->nextSector();

            if (hit.hitSector != nullptr && abs(hit.hitSector->floorz - pp->GetActor()->getOffsetZ()) < 50)
            {
				double dist = (pp->GetActor()->spr.pos.XY() - hit.hitpos.XY()).Length();
				double comp = (pp->GetActor()->clipdist + 16);
                if (dist < comp)
                    return true;
            }
        }
    }

    return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool PlayerFlyKey(void)
{
    if (!ToggleFlyMode)
        return false;

    ToggleFlyMode = false;

    if (!GodMode)
        return false;

    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginCrawl(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    pp->Flags &= ~(PF_FALLING | PF_JUMPING);
    pp->Flags |= (PF_CRAWLING);

    pp->friction = PLAYER_CRAWL_FRICTION;
    pp->p_floor_dist = PLAYER_CRAWL_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_CRAWL_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerCrawl;

    //pp->posz = pp->loz - PLAYER_CRAWL_HEIGHT;

    NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Crawl);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool PlayerFallTest(DSWPlayer* pp, double player_height)
{
    // If the floor is far below you, fall hard instead of adjusting height
    if (abs(pp->GetActor()->getOffsetZ() - pp->loz) > player_height + PLAYER_FALL_HEIGHTF)
    {
        // if on a STEEP slope sector and you have not moved off of the sector
        if (pp->lo_sectp &&
            abs(pp->lo_sectp->floorheinum) > 3000 &&
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

const int PLAYER_STANDING_ROOM = 68;

void DoPlayerCrawl(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    if (SectorIsUnderwaterArea(pp->cursector))
    {
        // if stacked water - which it should be
        if (FAF_ConnectArea(pp->cursector))
        {
            // adjust the z
            pp->posZset(pp->cursector->ceilingz + 12);
        }

        DoPlayerBeginDiveNoWarp(pp);
        return;
    }

    if ((!(pp->cmd.ucmd.actions & SB_CROUCH) || pp->cmd.ucmd.vel.Z > 0) && abs(pp->loz - pp->hiz) >= PLAYER_STANDING_ROOM)
    {
        // Let off of crawl to get up
        pp->Flags &= ~PF_CRAWLING;
        DoPlayerBeginRun(pp);
        return;
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
        NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Crawl);
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
        pp->posZset(pp->loz - PLAYER_CRAWL_HEIGHTF);
    }

    DoPlayerBob(pp);
    DoPlayerHeight(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginFly(DSWPlayer* pp)
{
    pp->Flags &= ~(PF_FALLING | PF_JUMPING | PF_CRAWLING);
    pp->Flags |= (PF_FLYING);

    pp->friction = PLAYER_FLY_FRICTION;
    pp->p_floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerFly;

    pp->z_speed = -10;
    pp->jump_speed = 0;
    pp->pbob_amt = 0;
    pp->bob_ndx = 1024;

    NewStateGroup(pp->GetActor(), sg_PlayerNinjaFly);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerWarpUpdatePos(DSWPlayer* pp, const DVector3& oldpos)
{
    if (Prediction)
        return;

    pp->GetActor()->opos += pp->GetActor()->spr.pos - oldpos;
    DoPlayerZrange(pp);
    UpdatePlayerSprite(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerFly(DSWPlayer* pp)
{
    if (SectorIsUnderwaterArea(pp->cursector))
    {
        DoPlayerBeginDiveNoWarp(pp);
        return;
    }

    const auto kbdDir = !!(pp->cmd.ucmd.actions & SB_CROUCH) - !!(pp->cmd.ucmd.actions & SB_JUMP);
    const double velZ = clamp(PLAYER_FLY_INC * kbdDir - PLAYER_FLY_INC * pp->cmd.ucmd.vel.Z, -PLAYER_FLY_INC, PLAYER_FLY_INC);
    pp->z_speed = clamp(pp->z_speed + velZ, -PLAYER_FLY_MAX_SPEED, PLAYER_FLY_MAX_SPEED) * FixedToFloat(58000);
    pp->GetActor()->spr.pos.Z += pp->z_speed;

    // Make the min distance from the ceiling/floor match bobbing amount
    // so the player never goes into the ceiling/floor

    // Only get so close to the ceiling
    if (PlayerCeilingHit(pp, pp->hiz + PLAYER_FLY_BOB_AMT + 8))
    {
        pp->posZset(pp->hiz + PLAYER_FLY_BOB_AMT + 8);
        pp->z_speed = 0;
    }

    // Only get so close to the floor
    if (PlayerFloorHit(pp, pp->loz - PLAYER_HEIGHTF - PLAYER_FLY_BOB_AMT))
    {
        pp->posZset(pp->loz - PLAYER_HEIGHTF - PLAYER_FLY_BOB_AMT);
        pp->z_speed = 0;
    }

    if (PlayerFlyKey())
    {
        pp->Flags &= ~(PF_FLYING);
        pp->pbob_amt = 0;
        pp->bob_ndx = 0;
        DoPlayerBeginFall(pp);
        DoPlayerFall(pp);
        return;
    }

    DoPlayerMove(pp);

    // Adjust view height moving up and down sectors
    DoPlayerViewOffset(pp);

    DoPlayerSpriteBob(pp, PLAYER_HEIGHTF, 6, 3);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DSWActor* FindNearSprite(DSWActor* actor, short stat)
{
    int fs;
    double dist, near_dist = 937.5;
    DSWActor* near_fp = nullptr;


    SWStatIterator it(stat);
    while (auto itActor = it.Next())
    {
		dist = (actor->spr.pos.XY() - itActor->spr.pos.XY()).Length();

        if (dist < near_dist)
        {
            near_dist = dist;
            near_fp = itActor;
        }
    }

    return near_fp;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool PlayerOnLadder(DSWPlayer* pp)
{
    int nx, ny;
    unsigned i;
    HitInfo hit, near;

    static DAngle angles[] =
    {
        mapangle(30), -mapangle(30)
    };

    if (Prediction)
        return false;

    neartag(pp->GetActor()->getPosWithOffsetZ(), pp->cursector, pp->GetActor()->spr.Angles.Yaw, near, 64. + 48., NT_Lotag | NT_Hitag);

    double dir = pp->vect.dot(pp->GetActor()->spr.Angles.Yaw.ToVector());

    if (dir < 0)
        return false;

    if (near.hitWall == nullptr || near.hitWall->lotag != TAG_WALL_CLIMB)
        return false;

    for (i = 0; i < SIZ(angles); i++)
    {
        neartag(pp->GetActor()->getPosWithOffsetZ(), pp->cursector, pp->GetActor()->spr.Angles.Yaw + angles[i], near, 37.5, NT_Lotag | NT_Hitag | NT_NoSpriteCheck);

        if (near.hitWall == nullptr || near.hitpos.X < 6.25 || near.hitWall->lotag != TAG_WALL_CLIMB)
            return false;

        FAFhitscan(pp->GetActor()->getPosWithOffsetZ(), pp->cursector, DVector3((pp->GetActor()->spr.Angles.Yaw + angles[i]).ToVector() * 1024, 0), hit, CLIPMASK_MISSILE);

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


    auto lActor = FindNearSprite(pp->GetActor(), STAT_CLIMB_MARKER);

    if (!lActor)
        return false;

    // determine where the player is supposed to be in relation to the ladder
    // move out in front of the ladder
    auto npos = lActor->spr.Angles.Yaw.ToVector() * 31.25;

    pp->LadderSector = near.hitWall->twoSided() ? near.hitWall->nextSector() : near.hitWall->sectorp();

    // set players "view" distance from the ladder - needs to be farther than
    // the sprite

	pp->LadderPosition = lActor->spr.pos.XY() + npos;
    pp->GetActor()->spr.Angles.Yaw = lActor->spr.Angles.Yaw + DAngle180;

    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool DoPlayerTestCrawl(DSWPlayer* pp)
{
    return abs(pp->loz - pp->hiz) < PLAYER_STANDING_ROOM;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int PlayerInDiveArea(DSWPlayer* pp)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int PlayerCanDive(DSWPlayer* pp)
{
    if (Prediction)
        return false;

    const double velZ = clamp(20. * !!(pp->cmd.ucmd.actions & SB_CROUCH) - 20. * pp->cmd.ucmd.vel.Z, -20., 20.);

    // Crawl - check for diving
    if (velZ > 0 || pp->jump_speed > 0)
    {
        if (PlayerInDiveArea(pp))
        {
            pp->GetActor()->spr.pos.Z += velZ;
            pp->z_speed = velZ;
            pp->jump_speed = 0;

            if (pp->GetActor()->getOffsetZ() > pp->loz - pp->WadeDepth - 2)
            {
                DoPlayerBeginDive(pp);
            }

            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int PlayerCanDiveNoWarp(DSWPlayer* pp)
{
    if (Prediction)
        return false;

    // check for diving
    if (pp->jump_speed > 1400)
    {
        if (FAF_ConnectArea(pp->cursector))
        {
            auto sect = pp->cursector;

            updatesectorz(ActorVectOfBottom(pp->GetActor()), &sect);

            if (SectorIsUnderwaterArea(sect))
            {
                pp->setcursector(sect);
                pp->posZset(sect->ceilingz + 20);

                pp->z_speed = 20;
                pp->jump_speed = 0;

                PlaySound(DIGI_SPLASH1, pp, v3df_dontpan);
                DoPlayerBeginDiveNoWarp(pp);
                return true;
            }
        }
    }

    return false;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int GetOverlapSector(const DVector2& pos, sectortype** over, sectortype** under)
{
    int i, found = 0;
    sectortype* sf[3]= {nullptr,nullptr};                       // sectors found
    auto secto = *over;
    auto sectu = *under;

    if ((sectu->hasU() && sectu->number >= 30000) || (secto->hasU() && secto->number >= 30000))
        return GetOverlapSector2(pos, over, under);

    // instead of check ALL sectors, just check the two most likely first
    if (inside(pos.X, pos.Y, *over))
    {
        sf[found] = *over;
        found++;
    }

    if (inside(pos.X, pos.Y, *under))
    {
        sf[found] = *under;
        found++;
    }

    // if nothing was found, check them all
    if (found == 0)
    {
        for (auto& sect: sector)
        {
            if (inside(pos.X, pos.Y, &sect))
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int GetOverlapSector2(const DVector2& pos, sectortype** over, sectortype** under)
{
    int found = 0;
    sectortype* sf[2]= {nullptr, nullptr};                       // sectors found

    unsigned stat;
    static short UnderStatList[] = {STAT_UNDERWATER, STAT_UNDERWATER2};

    // NOTE: For certain heavily overlapped areas in $seabase this is a better
    // method.

    // instead of check ALL sectors, just check the two most likely first
    if (inside(pos.X, pos.Y, *over))
    {
        sf[found] = *over;
        found++;
    }

    if (inside(pos.X, pos.Y, *under))
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
            if (inside(pos.X, pos.Y, actor->sector()))
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

                if (inside(pos.X, pos.Y, actor->sector()))
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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerWarpToUnderwater(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

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
    plActor->user.pos.SetXY(over_act->spr.pos.XY() - pp->GetActor()->spr.pos.XY());

    // update to the new x y position
    pp->GetActor()->spr.pos.SetXY(under_act->spr.pos.XY() - plActor->user.pos.XY());

    auto over  = over_act->sector();
    auto under = under_act->sector();

    if (GetOverlapSector(pp->GetActor()->spr.pos.XY(), &over, &under) == 2)
    {
        pp->setcursector(under);
    }
    else
        pp->setcursector(over);

    pp->posZset(under_act->sector()->ceilingz + 6);

    plActor->backuppos();

    DoPlayerZrange(pp);
    return;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerWarpToSurface(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();
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
    plActor->user.pos.SetXY(under_act->spr.pos.XY() - pp->GetActor()->spr.pos.XY());

    // update to the new x y position
    pp->GetActor()->spr.pos.SetXY(over_act->spr.pos.XY() - plActor->user.pos.XY());

    auto over = over_act->sector();
    auto under = under_act->sector();

    if (GetOverlapSector(pp->GetActor()->spr.pos.XY(), &over, &under))
    {
        pp->setcursector(over);
    }

    pp->posZset(over_act->sector()->floorz - 2);

    // set z range and wade depth so we know how high to set view
    DoPlayerZrange(pp);
    DoPlayerSetWadeDepth(pp);

    plActor->spr.pos.Z -= pp->WadeDepth;

    plActor->backuppos();

    return;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDivePalette(DSWPlayer* pp)
{
    if (pp != getPlayer(screenpeek)) return;

    if ((pp->DeathType == PLAYER_DEATH_DROWN || (getPlayer(screenpeek)->Flags & PF_DIVING)) && !(pp->Flags & PF_DIVING_IN_LAVA))
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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginDive(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    if (Prediction)
        return;

    if (pp->Bloody) pp->Bloody = false; // Water washes away the blood

    pp->Flags |= (PF_DIVING);
    DoPlayerDivePalette(pp);
    DoPlayerNightVisionPalette(pp);

    if (pp == getPlayer(screenpeek))
    {
        COVER_SetReverb(140); // Underwater echo
        pp->Reverb = 140;
    }

    SpawnSplash(pp->GetActor());

    DoPlayerWarpToUnderwater(pp);
    OperateTripTrigger(pp);

    pp->Flags &= ~(PF_JUMPING | PF_FALLING);
    pp->Flags &= ~(PF_CRAWLING);

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

    NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Dive);

    DoPlayerDive(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginDiveNoWarp(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    if (Prediction)
        return;

    if (!SectorIsUnderwaterArea(pp->cursector))
        return;

    if (pp->Bloody) pp->Bloody = false; // Water washes away the blood

    if (pp == getPlayer(screenpeek))
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
    NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Dive);
    DoPlayerDive(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerStopDiveNoWarp(DSWPlayer* pp)
{
    if (Prediction)
        return;

    StopPlayerSound(pp);

    // stop diving no warp
    PlayerSound(DIGI_SURFACE, v3df_dontpan|v3df_follow|v3df_doppler,pp);

    pp->pbob_amt = 0;

    pp->Flags &= ~(PF_DIVING|PF_DIVING_IN_LAVA);
    DoPlayerDivePalette(pp);
    DoPlayerNightVisionPalette(pp);
    pp->GetActor()->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
    if (pp == getPlayer(screenpeek))
    {
        COVER_SetReverb(0);
        pp->Reverb = 0;
    }

    DoPlayerZrange(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerStopDive(DSWPlayer* pp)
{
    DSWActor* actor = pp->GetActor();

    if (Prediction)
        return;

    StopPlayerSound(pp);

    // stop diving with warp
    PlayerSound(DIGI_SURFACE, v3df_dontpan|v3df_follow|v3df_doppler,pp);

    pp->pbob_amt = 0;
    DoPlayerWarpToSurface(pp);
    DoPlayerBeginWade(pp);
    pp->Flags &= ~(PF_DIVING|PF_DIVING_IN_LAVA);

    DoPlayerDivePalette(pp);
    DoPlayerNightVisionPalette(pp);
    actor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
    if (pp == getPlayer(screenpeek))
    {
        COVER_SetReverb(0);
        pp->Reverb = 0;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDiveMeter(DSWPlayer* pp)
{
    short color=0,metertics,meterunit;
    int y;


    if (NoMeters) return;

    // Don't draw bar from other players
    if (pp != getPlayer(myconnectindex)) return;

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDive(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();
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

    const auto kbdDir = !!(pp->cmd.ucmd.actions & SB_CROUCH) - !!(pp->cmd.ucmd.actions & SB_JUMP);
    const double velZ = clamp(PLAYER_DIVE_INC * kbdDir - PLAYER_DIVE_INC * pp->cmd.ucmd.vel.Z, -PLAYER_DIVE_INC, PLAYER_DIVE_INC);
    pp->z_speed = clamp(pp->z_speed + velZ, -PLAYER_DIVE_MAX_SPEED, PLAYER_DIVE_MAX_SPEED) * FixedToFloat(58000);

    if (abs(pp->z_speed) < 1./16)
        pp->z_speed = 0;

    plActor->spr.pos.Z += pp->z_speed;

    if (pp->z_speed < 0 && FAF_ConnectArea(pp->cursector))
    {
        if (pp->GetActor()->getOffsetZ() < pp->cursector->ceilingz + 10)
        {
            auto sect = pp->cursector;

            // check for sector above to see if it is an underwater sector also
            updatesectorz(DVector3(plActor->spr.pos.XY(), pp->cursector->ceilingz - 8), &sect);

            if (!SectorIsUnderwaterArea(sect))
            {
                // if not underwater sector we must surface
                // force into above sector
                pp->posZset(pp->cursector->ceilingz- 8);
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
        if (pp->GetActor()->getOffsetZ() < pp->hiz + pp->p_ceiling_dist)
        {
            pp->posZset(pp->hiz + pp->p_ceiling_dist);
        }
    }
    else
    {
        // close to a warping sector - stop diveing with a warp to surface
        // !JIM! FRANK - I added !pp->hiActor so that you don't warp to surface when
        //     there is a sprite above you since getzrange returns a hiz < ceiling height
        //     if you are clipping into a sprite and not the ceiling.
        if (pp->GetActor()->getOffsetZ() < pp->hiz + 4 && !pp->highActor)
        {
            DoPlayerStopDive(pp);
            return;
        }
    }

    // Only get so close to the floor
    if (pp->GetActor()->getOffsetZ() >= pp->loz - PLAYER_DIVE_HEIGHTF)
    {
        pp->posZset(pp->loz - PLAYER_DIVE_HEIGHTF);
    }

    // make player bob if sitting still
    if (!PLAYER_MOVING(pp) && pp->z_speed == 0)
    {
        DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHTF, PLAYER_DIVE_BOB_AMT, 3);
    }
    // player is moving
    else
    {
        // if bob amt is approx 0
        if (abs(pp->pbob_amt) < 1)
        {
            pp->pbob_amt = 0;
            pp->bob_ndx = 0;
        }
        // else keep bobbing until its back close to 0
        else
        {
            DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHTF, PLAYER_DIVE_BOB_AMT, 3);
        }
    }

    // Reverse bobbing when getting close to the floor
    if (pp->GetActor()->getOffsetZ() + pp->pbob_amt >= pp->loz - PLAYER_DIVE_HEIGHTF)
    {
        pp->bob_ndx = NORM_ANGLE(pp->bob_ndx + ((1024 + 512) - pp->bob_ndx) * 2);
        DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHTF, PLAYER_DIVE_BOB_AMT, 3);
    }
    // Reverse bobbing when getting close to the ceiling
    if (pp->GetActor()->getOffsetZ() + pp->pbob_amt < pp->hiz + pp->p_ceiling_dist)
    {
        pp->bob_ndx = NORM_ANGLE(pp->bob_ndx + ((512) - pp->bob_ndx) * 2);
        DoPlayerSpriteBob(pp, PLAYER_DIVE_HEIGHTF, PLAYER_DIVE_BOB_AMT, 3);
    }

    DoPlayerMove(pp);

    // Random bubble sounds
    // if((RandomRange(1000<<5)>>5) < 100)
    //     PlaySound(DIGI_BUBBLES, pp, v3df_dontpan|v3df_follow);

    if ((!Prediction && pp->z_speed && ((RANDOM_P2(1024<<5)>>5) < 64)) ||
        (PLAYER_MOVING(pp) && (RANDOM_P2(1024<<5)>>5) < 64))
    {
        PlaySound(DIGI_BUBBLES, pp, v3df_none);
        auto bubble = SpawnBubble(pp->GetActor());
        if (bubble != nullptr)
        {
            // back it up a bit to get it out of your face
            auto vec = (bubble->spr.Angles.Yaw + DAngle180).ToVector() * 12;

            move_sprite(bubble, DVector3(vec, 0), plActor->user.ceiling_dist, plActor->user.floor_dist, 0, synctics);
        }
    }

    // Adjust view height moving up and down sectors
    DoPlayerViewOffset(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoPlayerTestPlaxDeath(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    // landed on a paralax floor
    if (pp->lo_sectp && (pp->lo_sectp->floorstat & CSTAT_SECTOR_SKY))
    {
        PlayerUpdateHealth(pp, -plActor->user.Health);
        PlayerCheckDeath(pp, nullptr);
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerCurrent(DSWPlayer* pp)
{
    int xvect, yvect;
    auto sectu = pp->cursector;
    int push_ret;

    if (!sectu)
        return;

    auto vect = sectu->angle.ToVector() / 256. * sectu->speed * synctics; // 16384 >> 4 - Beware of clipmove's odd format for vect!

    push_ret = pushmove(pp->GetActor()->spr.pos, pp->GetActor()->getOffsetZ(), &pp->cursector, pp->GetActor()->clipdist, pp->p_ceiling_dist, pp->p_floor_dist, CLIPMASK_PLAYER);
    if (push_ret < 0)
    {
        if (!(pp->Flags & PF_DEAD))
        {
            DSWActor* plActor = pp->GetActor();

            PlayerUpdateHealth(pp, -plActor->user.Health);  // Make sure he dies!
            PlayerCheckDeath(pp, nullptr);

            if (pp->Flags & (PF_DEAD))
                return;
        }
        return;
    }
    Collision coll;
    clipmove(pp->GetActor()->spr.pos, pp->GetActor()->getOffsetZ(), &pp->cursector, vect, pp->GetActor()->clipdist, pp->p_ceiling_dist, pp->p_floor_dist, CLIPMASK_PLAYER, coll);

    PlayerCheckValidMove(pp);
    pushmove(pp->GetActor()->spr.pos, pp->GetActor()->getOffsetZ(), &pp->cursector, pp->GetActor()->clipdist, pp->p_ceiling_dist, pp->p_floor_dist, CLIPMASK_PLAYER);
    if (push_ret < 0)
    {
        if (!(pp->Flags & PF_DEAD))
        {
            DSWActor* plActor = pp->GetActor();

            PlayerUpdateHealth(pp, -plActor->user.Health);  // Make sure he dies!
            PlayerCheckDeath(pp, nullptr);

            if (pp->Flags & (PF_DEAD))
                return;
        }
        return;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerFireOutWater(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerFireOutDeath(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    if (Prediction)
        return;

    if (plActor->user.flameActor != nullptr)
        SetSuicide(plActor->user.flameActor);

    plActor->user.flameActor = nullptr;
    plActor->user.Flags2 |= SPR2_FLAMEDIE;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginWade(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

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
        SpawnSplash(pp->GetActor());

    // fix it so that you won't go under water unless you hit the water at a
    // certain speed
    if (pp->jump_speed > 0 && pp->jump_speed < 1300)
        pp->jump_speed = 0;

    ASSERT(plActor->user.ActorActionSet->Run);

    NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Run);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerWade(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    DoPlayerFireOutWater(pp);

    if (DebugOperate)
    {
        if (pp->cmd.ucmd.actions & SB_OPEN)
        {
            if (pp->KeyPressBits & SB_OPEN)
            {
                if ((pp->cursector->extra & SECTFX_OPERATIONAL))
                {
                    pp->KeyPressBits &= ~SB_OPEN;
                    DoPlayerBeginOperate(pp);
                    pp->pbob_amt = 0;
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
    if (((pp->cmd.ucmd.actions & SB_CROUCH) || pp->cmd.ucmd.vel.Z < 0) && pp->WadeDepth <= PLAYER_CRAWL_WADE_DEPTH)
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    if (pp->cmd.ucmd.actions & SB_JUMP)
    {
        if (pp->KeyPressBits & SB_JUMP)
        {
            pp->KeyPressBits &= ~SB_JUMP;
            DoPlayerBeginJump(pp);
            pp->pbob_amt = 0;
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
        pp->pbob_amt = 0;
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
            NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Run);
    }
    else
    {
        if (plActor->user.Rot != plActor->user.ActorActionSet->Stand)
            NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Stand);
    }

    // If the floor is far below you, fall hard instead of adjusting height
    if (abs(pp->GetActor()->getOffsetZ() - pp->loz) > PLAYER_HEIGHTF + PLAYER_FALL_HEIGHTF)
    {
        pp->jump_speed = 256;
        DoPlayerBeginFall(pp);
        // call PlayerFall now seems to iron out a hitch before falling
        DoPlayerFall(pp);
        return;
    }

    if (PlayerCanDive(pp))
    {
        pp->pbob_amt = 0;
        pp->bob_ndx = 0;
        return;
    }

    // If the floor is far below you, fall hard instead of adjusting height
    if (abs(pp->GetActor()->getOffsetZ() - pp->loz) > PLAYER_HEIGHTF + PLAYER_FALL_HEIGHTF)
    {
        pp->jump_speed = Z(1);
        DoPlayerBeginFall(pp);
        // call PlayerFall now seems to iron out a hitch before falling
        DoPlayerFall(pp);
        pp->pbob_amt = 0;
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
    else if (pp->WadeDepth > Z(29))
    {
        DoPlayerSpriteBob(pp, PLAYER_HEIGHTF, 3, 3);
    }
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginOperateVehicle(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    pp->p_floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerOperateVehicle;

    // temporary set to get weapons down
    if ((pp->sop->flags & SOBJ_HAS_WEAPON))
        pp->Flags |= (PF_WEAPON_DOWN);

    ///DamageData[plActor->user.WeaponNum].Init(pp);

    ASSERT(plActor->user.ActorActionSet->Stand);

    NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Stand);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginOperateTurret(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    pp->p_floor_dist = PLAYER_RUN_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_RUN_CEILING_DIST;
    pp->DoPlayerAction = DoPlayerOperateTurret;

    // temporary set to get weapons down
    if ((pp->sop->flags & SOBJ_HAS_WEAPON))
        pp->Flags |= (PF_WEAPON_DOWN);

    ///DamageData[plActor->user.WeaponNum].Init(pp);

    ASSERT(plActor->user.ActorActionSet->Stand);

    NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Stand);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FindMainSector(SECTOR_OBJECT* sop)
{
    // find the main sector - only do this once for each sector object
    if (sop->op_main_sector == nullptr)
    {
        auto oldpos = sop->pmid;
        auto oldflags = sop->flags;
        sop->flags &= ~SOBJ_DONT_ROTATE;    // This flag must be disabled here because it messes with the movement that's intended below.

        PlaceSectorObject(sop, { MAXSO, MAXSO });

        // set it to something valid
        sop->op_main_sector = &sector[0];

        updatesectorz(oldpos, &sop->op_main_sector);

        PlaceSectorObject(sop, oldpos.XY());
        sop->flags = oldflags;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerOperateMatch(DSWPlayer* pp, bool starting)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginOperate(DSWPlayer* pp)
{
    SECTOR_OBJECT* sop;
    double cz, fz;
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
    sop->controller = pp->GetActor();

    pp->GetActor()->PrevAngles.Yaw = pp->GetActor()->spr.Angles.Yaw = sop->ang;
    pp->GetActor()->spr.pos.SetXY(sop->pmid.XY());
    updatesector(pp->GetActor()->getPosWithOffsetZ(), &pp->cursector);
    calcSlope(pp->cursector, pp->GetActor()->getPosWithOffsetZ(), &cz, &fz);
    pp->posZset(fz - PLAYER_HEIGHTF);

    pp->Flags &= ~(PF_CRAWLING|PF_JUMPING|PF_FALLING);

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
        if (!pp->cmd.ucmd.vel.XY().isZero())
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
		pp->posZset(fz - PLAYER_HEIGHTF);
        DoPlayerBeginOperateVehicle(pp);
        break;
    case SO_TURRET_MGUN:
    case SO_TURRET:
        if (pp->cmd.ucmd.ang.Yaw.Degrees())
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
		pp->posZset(fz - PLAYER_HEIGHTF);
        DoPlayerBeginOperateTurret(pp);
        break;
#if 0
    case SO_SPEED_BOAT:
        if (!pp->cmd.ucmd.vel.XY().isZero())
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
		pp->posZset(fz - PLAYER_HEIGHTF);
        DoPlayerBeginOperateBoat(pp);
        break;
#endif
    default:
        return;
    }

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginRemoteOperate(DSWPlayer* pp, SECTOR_OBJECT* sop)
{
    double cz, fz;
    int i;

    pp->sop_remote = pp->sop = pp->sop_control = sop;
    sop->controller = pp->GetActor();

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

    pp->GetActor()->PrevAngles.Yaw = pp->GetActor()->spr.Angles.Yaw = sop->ang;
    pp->GetActor()->spr.pos.SetXY(sop->pmid.XY());
    updatesector(pp->GetActor()->getPosWithOffsetZ(), &pp->cursector);
    calcSlope(pp->cursector, pp->GetActor()->getPosWithOffsetZ(), &cz, &fz);
    pp->posZset(fz - PLAYER_HEIGHTF);

    pp->Flags &= ~(PF_CRAWLING|PF_JUMPING|PF_FALLING);

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
        if (!pp->cmd.ucmd.vel.XY().isZero())
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
		pp->posZset(fz - PLAYER_HEIGHTF);
        DoPlayerBeginOperateVehicle(pp);
        break;
    case SO_TURRET_MGUN:
    case SO_TURRET:
        if (pp->cmd.ucmd.ang.Yaw.Degrees())
            PlaySOsound(pp->sop->mid_sector, SO_DRIVE_SOUND);
        else
            PlaySOsound(pp->sop->mid_sector, SO_IDLE_SOUND);
		pp->posZset(fz - PLAYER_HEIGHTF);
        DoPlayerBeginOperateTurret(pp);
        break;
    default:
        return;
    }

    PlayerRemoteReset(pp, save_sect);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerToRemote(DSWPlayer* pp)
{
    pp->remote.cursectp = pp->cursector;
    pp->remote.lastcursectp = pp->lastcursector;

    pp->remote.pos = pp->GetActor()->getPosWithOffsetZ();

    pp->remote.vect = pp->vect;
    pp->remote.ovect = pp->ovect;
    pp->remote.slide_vect = pp->slide_vect;
}

void RemoteToPlayer(DSWPlayer* pp)
{
    pp->setcursector(pp->remote.cursectp);
    pp->lastcursector = pp->remote.lastcursectp;

    pp->GetActor()->spr.pos = pp->remote.pos.plusZ(-pp->GetActor()->viewzoffset);

    pp->vect = pp->remote.vect;
    pp->ovect = pp->remote.ovect;
    pp->slide_vect = pp->remote.slide_vect;
}

void PlayerRemoteReset(DSWPlayer* pp, sectortype* sect)
{
    pp->setcursector(sect);
    pp->lastcursector = pp->cursector;

    auto rsp = pp->remoteActor;
    pp->GetActor()->spr.pos.SetXY(rsp->spr.pos.XY());
    pp->posZset(sect->floorz - PLAYER_HEIGHTF);

    pp->vect.Zero();
    pp->ovect.Zero();
    pp->slide_vect.Zero();

    UpdatePlayerSprite(pp);
}

void PlayerRemoteInit(DSWPlayer* pp)
{
    pp->remote.vect.Zero();
    pp->remote.ovect.Zero();
    pp->remote.slide_vect.Zero();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerStopOperate(DSWPlayer* pp)
{
    pp->Flags &= ~(PF_WEAPON_DOWN);
    DoPlayerResetMovement(pp);
    DoTankTreads(pp);
    DoPlayerOperateMatch(pp, false);
    StopSOsound(pp->sop->mid_sector);

    if (pp->sop_remote)
    {
        DSWActor* rsp = pp->remoteActor;
        pp->GetActor()->PrevAngles.Yaw = pp->GetActor()->spr.Angles.Yaw = rsp && TEST_BOOL1(rsp) ? rsp->spr.Angles.Yaw : (pp->sop_remote->pmid.XY() - pp->GetActor()->spr.pos.XY()).Angle();
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerOperateTurret(DSWPlayer* pp)
{
    if (pp->cmd.ucmd.actions & SB_OPEN)
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



//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerOperateVehicle(DSWPlayer* pp)
{
    if (pp->cmd.ucmd.actions & SB_OPEN)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathJump(DSWPlayer* pp)
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
        pp->GetActor()->spr.pos.Z += pp->jump_speed * JUMP_FACTOR;

        // if player gets to close the ceiling while jumping
        //if (pp->posz < pp->hiz + Z(4))
        if (PlayerCeilingHit(pp, pp->hiz + 4))
        {
            // put player at the ceiling
            pp->posZset(pp->hiz + 4);

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathFall(DSWPlayer* pp)
{
    double loz;

    for (int i = 0; i < synctics; i++)
    {
        // adjust jump speed by gravity
        pp->jump_speed += PLAYER_DEATH_GRAV;

        // adjust player height by jump speed
        pp->GetActor()->spr.pos.Z += pp->jump_speed * JUMP_FACTOR;

        if (pp->lo_sectp && (pp->lo_sectp->extra & SECTFX_SINK))
        {
            loz = pp->lo_sectp->floorz;
        }
        else
            loz = pp->loz;

        if (PlayerFloorHit(pp, loz - PLAYER_DEATH_HEIGHTF))
        {
            if (loz != pp->loz)
                SpawnSplash(pp->GetActor());

            if (RandomRange(1000) > 500)
                PlaySound(DIGI_BODYFALL1, pp, v3df_dontpan);
            else
                PlaySound(DIGI_BODYFALL2, pp, v3df_dontpan);

            pp->posZset(loz - PLAYER_DEATH_HEIGHTF);
            pp->Flags &= ~(PF_FALLING);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

// currently unused, if MP ever becomes workable all this needs to be made localization friendly.
// The texts are already translated for most languages.
char *KilledPlayerMessage(DSWPlayer* pp, DSWPlayer* killer)
{
    const int MAX_KILL_NOTES = 16;
    short rnd = StdRandomRange(MAX_KILL_NOTES);
    const char *p1 = pp->PlayerName;
    const char *p2 = killer->PlayerName;

    if (pp->HitBy == killer->GetActor())
    {
        snprintf(ds, sizeof(ds),"%s was killed by %s.",p1,p2);
        return ds;
    }
    else
        switch (rnd)
        {
        case 0:
            snprintf(ds, sizeof(ds),"%s was wasted by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 1:
            snprintf(ds, sizeof(ds),"%s got his ass kicked by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 2:
            snprintf(ds, sizeof(ds),"%s bows down before the mighty power of %s.",p1,p2);
            return ds;
        case 3:
            snprintf(ds, sizeof(ds),"%s was killed by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 4:
            snprintf(ds, sizeof(ds),"%s got slapped down hard by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 5:
            snprintf(ds, sizeof(ds),"%s got on his knees before %s.",p1,p2);
            return ds;
        case 6:
            snprintf(ds, sizeof(ds),"%s was totally out classed by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 7:
            snprintf(ds, sizeof(ds),"%s got chewed apart by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 8:
            snprintf(ds, sizeof(ds),"%s was retired by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 9:
            snprintf(ds, sizeof(ds),"%s was greased by %s's %s.",p1,p2,DeathString(pp->HitBy));
            return ds;
        case 10:
            snprintf(ds, sizeof(ds),"%s was humbled lower than dirt by %s.",p1,p2);
            return ds;
        case 11:
            snprintf(ds, sizeof(ds),"%s beats %s like a red headed step child.",p2,p1);
            return ds;
        case 12:
            snprintf(ds, sizeof(ds),"%s begs for mercy as %s terminates him with extreme prejudice.",p1,p2);
            return ds;
        case 13:
            snprintf(ds, sizeof(ds),"%s falls before the superior skills of %s.",p1,p2);
            return ds;
        case 14:
            snprintf(ds, sizeof(ds),"%s gives %s a beating he'll never forget.",p2,p1);
            return ds;
        case 15:
            snprintf(ds, sizeof(ds),"%s puts the Smack Dab on %s with his %s.",p2,p1,DeathString(pp->HitBy));
            return ds;
        }
    return nullptr;
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathMessage(DSWPlayer* pp, DSWPlayer* killer)
{
    int pnum;
    bool SEND_OK = false;

    killer->KilledPlayer[pp->pnum]++;

    if (pp == killer && pp == getPlayer(myconnectindex))
    {
        snprintf(ds, sizeof(ds),"%s %s",pp->PlayerName,SuicideNote[StdRandomRange(MAX_SUICIDE)]);
        SEND_OK = true;
    }
    else
    // I am being killed
    if (killer == getPlayer(myconnectindex))
    {
        snprintf(ds, sizeof(ds),"%s",KilledPlayerMessage(pp,killer));
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

enum
{
    PLAYER_DEATH_HORIZ_UP_VALUE = 65,
    PLAYER_DEATH_HORIZ_JUMP_VALUE = 50,
    PLAYER_DEATH_HORIZ_FALL_VALUE = -50
};

constexpr DAngle PLAYER_DEATH_HORIZ_UP_VALUEF = DAngle::fromDeg(-22.922);
constexpr DAngle PLAYER_DEATH_HORIZ_JUMP_VALUEF = DAngle::fromDeg(-21.33686);
constexpr DAngle PLAYER_DEATH_HORIZ_FALL_VALUEF = -PLAYER_DEATH_HORIZ_JUMP_VALUEF;

void DoPlayerBeginDie(DSWPlayer* pp)
{
    extern bool ReloadPrompt;
    short bak;
    int choosesnd = 0;

    DSWActor* plActor = pp->GetActor();

    static void (*PlayerDeathFunc[MAX_PLAYER_DEATHS]) (DSWPlayer*) =
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

    PutStringInfo(pp, GStrings.GetString("TXTS_PRESSSPACE"));

    if (pp->sop_control)
        DoPlayerStopOperate(pp);

    // if diving force death to drown type
    if (pp->Flags & (PF_DIVING))
        pp->DeathType = PLAYER_DEATH_DROWN;

    pp->Flags &= ~(PF_JUMPING|PF_FALLING|PF_DIVING|PF_FLYING|PF_CLIMBING|PF_CRAWLING);

    ActorCoughItem(pp->GetActor());

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
                        if (pp->GetActor()->user.spal == killer->user.spal)
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

    pp->cmd.ucmd.actions &= ~SB_CENTERVIEW;

    pp->friction = PLAYER_RUN_FRICTION;
    pp->slide_vect.Zero();
    pp->p_floor_dist = PLAYER_WADE_FLOOR_DIST;
    pp->p_ceiling_dist = PLAYER_WADE_CEILING_DIST;
    ASSERT(pp->DeathType < SIZ(PlayerDeathFunc));
    pp->DoPlayerAction = PlayerDeathFunc[pp->DeathType];
    pp->sop_control = nullptr;
    pp->sop_remote = nullptr;
    pp->sop_riding = nullptr;
    pp->sop = nullptr;
    pp->Flags &= ~(PF_TWO_UZI);

    NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Run);
    pWeaponForceRest(pp);

    switch (pp->DeathType)
    {
    case PLAYER_DEATH_DROWN:
    {
        pp->Flags |= (PF_JUMPING);
        plActor->user.ID = NINJA_DEAD;
        pp->jump_speed = -200;
        NewStateGroup(pp->GetActor(), sg_PlayerDeath);
        DoFindGround(pp->GetActor());
        DoBeginJump(pp->GetActor());
        plActor->user.jump_speed = -300;
        break;
    }
    case PLAYER_DEATH_FLIP:
    case PLAYER_DEATH_RIPPER:

        //PlaySound(DIGI_SCREAM1, pp, v3df_dontpan|v3df_follow);

        pp->Flags |= (PF_JUMPING);
        plActor->user.ID = NINJA_DEAD;
        pp->jump_speed = -300;
        NewStateGroup(pp->GetActor(), sg_PlayerDeath);
        //pp->ceiling_dist = Z(0);
        //pp->floor_dist = Z(0);

        plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        plActor->user.ceiling_dist = (10);
        plActor->user.floor_dist = (0);
        DoFindGround(pp->GetActor());
        DoBeginJump(pp->GetActor());
        plActor->user.jump_speed = -400;
        break;
    case PLAYER_DEATH_CRUMBLE:

        PlaySound(DIGI_BODYSQUISH1, pp, v3df_dontpan);

        pp->Flags |= (PF_DEAD_HEAD | PF_JUMPING);
        pp->jump_speed = -300;
        plActor->user.slide_vel = 0;
        SpawnShrap(pp->GetActor(), nullptr);
        plActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        NewStateGroup(pp->GetActor(), sg_PlayerHeadFly);
        plActor->user.ID = NINJA_Head_R0;
        plActor->spr.scale = DVector2(0.75, 0.75);
        // Blood fountains
        InitBloodSpray(pp->GetActor(),true,105);
        break;
    case PLAYER_DEATH_EXPLODE:

        PlaySound(DIGI_BODYSQUISH1, pp, v3df_dontpan);

        pp->Flags |= (PF_DEAD_HEAD | PF_JUMPING);
        pp->jump_speed = -650;
        SpawnShrap(pp->GetActor(), nullptr);
        plActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        NewStateGroup(pp->GetActor(), sg_PlayerHeadFly);
        plActor->user.ID = NINJA_Head_R0;
        plActor->spr.scale = DVector2(0.75, 0.75);
        // Blood fountains
        InitBloodSpray(pp->GetActor(),true,-1);
        InitBloodSpray(pp->GetActor(),true,-1);
        InitBloodSpray(pp->GetActor(),true,-1);
        break;
    case PLAYER_DEATH_SQUISH:

        PlaySound(DIGI_BODYCRUSHED1, pp, v3df_dontpan);

        pp->Flags |= (PF_DEAD_HEAD | PF_JUMPING);
        pp->jump_speed = 200;
        plActor->user.slide_vel = 50;
        SpawnShrap(pp->GetActor(), nullptr);
        plActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        NewStateGroup(pp->GetActor(), sg_PlayerHeadFly);
        plActor->user.ID = NINJA_Head_R0;
        plActor->spr.scale = DVector2(0.75, 0.75);
        // Blood fountains
        InitBloodSpray(pp->GetActor(),true,105);
        break;

    }

    pp->Flags |= (PF_DEAD);
    plActor->user.Flags &= ~(SPR_BOUNCE);
    pp->Flags &= ~(PF_HEAD_CONTROL);
    gameInput.ForceInputSync(pp->pnum);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static inline void DoPlayerDeathHoriz(DSWPlayer* pp, const DAngle target, const double speed)
{
    auto& pitch = pp->GetActor()->spr.Angles.Pitch;
    auto pspeed = DAngle::fromDeg(speed);

    if (pitch < target)
    {
        pitch = min(pitch + pspeed, target);
    }

    if (pitch > target)
    {
        pitch = max(pitch - pspeed, target);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathZrange(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    // make sure we don't land on a regular sprite
    DoFindGround(pp->GetActor());

    // update player values with results from DoFindGround
    pp->loz = plActor->user.loz;
    pp->lowActor = plActor->user.lowActor;
    pp->lo_sectp = plActor->user.lo_sectp;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathHurl(DSWPlayer* pp)
{
    if (numplayers > 1)
    {
        if (pp->cmd.ucmd.actions & SB_FIRE)
        {
            if (pp->KeyPressBits & SB_FIRE)
            {


                pp->Flags |= (PF_HEAD_CONTROL);
                NewStateGroup(pp->GetActor(), sg_PlayerHeadHurl);
                if (MoveSkip4 == 0)
                {
                    SpawnShrap(pp->GetActor(), nullptr);
                    if (RandomRange(1000) > 400)
                        PlayerSound(DIGI_DHVOMIT, v3df_dontpan|v3df_follow,pp);
                }
                return;
            }
        }
    }

    if (!(pp->Flags & (PF_JUMPING|PF_FALLING)))
        NewStateGroup(pp->GetActor(), sg_PlayerHead);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathFollowKiller(DSWPlayer* pp)
{
    // if it didn't make it to this angle because of a low ceiling or something
    // continue on to it
    DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUEF, 1.79);

    // allow turning
    if (pp->Flags & (PF_DEAD_HEAD|PF_HEAD_CONTROL))
    {  
        pp->GetActor()->spr.Angles.Yaw += pp->cmd.ucmd.ang.Yaw;
        UpdatePlayerSpriteAngle(pp);
    }

    // follow what killed you if its available
    DSWActor* killer = pp->KillerActor;
    if (killer)
    {
        if (FAFcansee(ActorVectOfTop(killer), killer->sector(), pp->GetActor()->getPosWithOffsetZ(), pp->cursector))
        {
            pp->GetActor()->spr.Angles.Yaw += deltaangle(pp->GetActor()->spr.Angles.Yaw, (killer->spr.pos.XY() - pp->GetActor()->spr.pos.XY()).Angle()) * (1. / 16.);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathCheckKeys(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    if (pp->cmd.ucmd.actions & SB_OPEN)
    {
        // Spawn a dead LoWang body for non-head deaths
        // Hey Frank, if you think of a better check, go ahead and put it in.
        if (PlayerFloorHit(pp, pp->loz - PLAYER_HEIGHTF))
        {
            if (pp->DeathType == PLAYER_DEATH_FLIP || pp->DeathType == PLAYER_DEATH_RIPPER)
                QueueLoWangs(pp->GetActor());
        }
        else
        {
            // If he's not on the floor, then gib like a mo-fo!
            InitBloodSpray(plActor,true,-1);
            InitBloodSpray(plActor,true,-1);
            InitBloodSpray(plActor,true,-1);
        }

        NewStateGroup(plActor, plActor->user.ActorActionSet->Stand);
        plActor->spr.picnum = plActor->user.State->Pic;
        plActor->spr.picnum = plActor->user.State->Pic;
        plActor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);

        //DoSpawnTeleporterEffect(plActor);
        PlaySound(DIGI_TELEPORT, pp, v3df_none);

        DoPlayerZrange(pp);

        plActor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
        plActor->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
		plActor->spr.scale = DVector2(PLAYER_NINJA_XREPEAT, PLAYER_NINJA_YREPEAT);

        plActor->user.ID = NINJA_RUN_R0;

        if (pp == getPlayer(screenpeek))
        {
            videoFadePalette(0,0,0,0);
        }

        pp->NightVision = false;
        pp->FadeAmt = 0;
        DoPlayerDivePalette(pp);
        DoPlayerNightVisionPalette(pp);

        if (numplayers > 1)
        {
            PlayerDeathReset(pp);

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerHeadDebris(DSWPlayer* pp)
{
    sectortype* sectp = pp->cursector;

    if ((sectp->extra & SECTFX_SINK))
    {
        DoPlayerSpriteBob(pp, 8, 4, 3);
    }
    else
    {
        pp->pbob_amt = 0;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathCheckKick(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();
    unsigned stat;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == pp->GetActor())
                break;

            // don't set off mine
            if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                continue;

			double dist = (itActor->spr.pos.XY() - plActor->spr.pos.XY()).Length();

            if (dist < itActor->user.fRadius() + 6.25)
            {
                pp->KillerActor = itActor;

                plActor->user.slide_ang = (plActor->spr.pos - itActor->spr.pos).Angle();
				plActor->user.slide_ang += RandomAngle(22.5) - DAngle22_5 / 2;

                plActor->user.slide_vel = itActor->vel.X * 2;
                plActor->user.Flags &= ~(SPR_BOUNCE);
                pp->jump_speed = -500;
                NewStateGroup(pp->GetActor(), sg_PlayerHeadFly);
                pp->Flags |= (PF_JUMPING);
                SpawnShrap(pp->GetActor(), nullptr);
            }
        }
    }

    DoPlayerZrange(pp);

    // sector stomper kick
    if (abs(pp->loz - pp->hiz) < ActorSizeZ(plActor) - 8)
    {
        plActor->user.slide_ang = RandomAngle();
        plActor->user.slide_vel = 62.5;
        plActor->user.Flags &= ~(SPR_BOUNCE);
        pp->jump_speed = -100;
        NewStateGroup(pp->GetActor(), sg_PlayerHeadFly);
        pp->Flags |= (PF_JUMPING);
        SpawnShrap(pp->GetActor(), nullptr);
    }
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathMoveHead(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    auto davec = plActor->user.slide_ang.ToVector() * plActor->user.slide_vel;

    plActor->user.coll = move_sprite(pp->GetActor(), DVector3(davec, 0), 16, 16, 1, synctics);
    {
        switch (plActor->user.coll.type)
        {
        case kHitSprite:
        {
            auto hitActor = plActor->user.coll.actor();

            if (!(hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
                break;


            DAngle wall_ang = hitActor->spr.Angles.Yaw;
            DAngle dang = deltaangle(wall_ang, plActor->user.slide_ang);
            plActor->user.slide_ang = wall_ang + DAngle180 - dang;

            SpawnShrap(pp->GetActor(), nullptr);
            break;
        }
        case kHitWall:
        {
            DAngle wall_ang = plActor->user.coll.hitWall->delta().Angle() - DAngle90;
            DAngle dang = deltaangle(wall_ang, plActor->user.slide_ang);
            plActor->user.slide_ang = wall_ang + DAngle180 - dang;

            SpawnShrap(pp->GetActor(), nullptr);
            break;
        }
        }
    }

    pp->GetActor()->spr.pos.SetXY(plActor->spr.pos.XY());
    pp->setcursector(plActor->sector());

    // try to stay in valid area - death sometimes throws you out of the map
    auto sect = pp->cursector;
    updatesector(pp->GetActor()->getPosWithOffsetZ(), &sect);
    if (sect == nullptr)
    {
        pp->cursector = pp->lv_sector;
        ChangeActorSect(pp->GetActor(), pp->lv_sector);
        pp->GetActor()->spr.pos.SetXY(pp->lv.XY());
        plActor->spr.pos.SetXY(pp->GetActor()->spr.pos.XY());
    }
    else
    {
        pp->lv_sector = sect;
        pp->lv.SetXY(pp->GetActor()->spr.pos.XY());
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathFlip(DSWPlayer* pp)
{
    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if ((pp->Flags & (PF_JUMPING|PF_FALLING)))
    {
        if ((pp->Flags & PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUEF, 0.8952);
        }

        if ((pp->Flags & PF_FALLING))
        {
            DoPlayerDeathFall(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUEF, 1.79);
        }
    }
    else
    {
        DoPlayerDeathFollowKiller(pp);
    }

    DoPlayerDeathCheckKeys(pp);
}



//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathDrown(DSWPlayer* pp)
{
    DSWActor* actor = pp->GetActor();

    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if ((pp->Flags & (PF_JUMPING|PF_FALLING)))
    {
        if ((pp->Flags & PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_UP_VALUEF, 0.8952);
        }

        if ((pp->Flags & PF_FALLING))
        {
            actor->spr.pos.Z += 2;

            // Stick like glue when you hit the ground
            if (pp->GetActor()->getOffsetZ() > pp->loz - PLAYER_DEATH_HEIGHTF)
            {
                pp->posZset(pp->loz - PLAYER_DEATH_HEIGHTF);
                pp->Flags &= ~(PF_FALLING);
            }
        }
    }

    DoPlayerDeathFollowKiller(pp);
    DoPlayerDeathCheckKeys(pp);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathBounce(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    if (Prediction)
        return;

    if (pp->lo_sectp && (pp->lo_sectp->extra & SECTFX_SINK))
    {
        plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        NewStateGroup(pp->GetActor(), sg_PlayerHead);
        plActor->user.slide_vel = 0;
        plActor->user.Flags |= (SPR_BOUNCE);


        return;
    }

    plActor->user.Flags |= (SPR_BOUNCE);
    pp->jump_speed = -300;
    plActor->user.slide_vel *= 0.25;
	plActor->user.slide_ang = RandomAngle(11.25) - DAngle22_5 / 4;
    pp->Flags |= (PF_JUMPING);
    SpawnShrap(pp->GetActor(), nullptr);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathCrumble(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if ((pp->Flags & (PF_JUMPING|PF_FALLING)))
    {
        if ((pp->Flags & PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_JUMP_VALUEF, 1.79);
        }

        if ((pp->Flags & PF_FALLING))
        {
            DoPlayerDeathFall(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_FALL_VALUEF, 1.343);
        }

        if (!(pp->Flags & (PF_JUMPING|PF_FALLING)))
        {
            if (!(plActor->user.Flags & SPR_BOUNCE))
            {
                DoPlayerDeathBounce(pp);
                return;
            }

            plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
            NewStateGroup(pp->GetActor(), sg_PlayerHead);
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
    DoPlayerHeadDebris(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerDeathExplode(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    if (Prediction)
        return;

    DoPlayerDeathZrange(pp);

    if ((pp->Flags & (PF_JUMPING|PF_FALLING)))
    {
        if ((pp->Flags & PF_JUMPING))
        {
            DoPlayerDeathJump(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_JUMP_VALUEF, 1.79);
        }

        if ((pp->Flags & PF_FALLING))
        {
            DoPlayerDeathFall(pp);
            DoPlayerDeathHoriz(pp, PLAYER_DEATH_HORIZ_JUMP_VALUEF, 1.343);
        }

        if (!(pp->Flags & (PF_JUMPING|PF_FALLING)))
        {
            if (!(plActor->user.Flags & SPR_BOUNCE))
            {
                DoPlayerDeathBounce(pp);
                return;
            }

            plActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
            NewStateGroup(pp->GetActor(), sg_PlayerHead);
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
    DoPlayerHeadDebris(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerBeginRun(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    // Crawl if in small aread automatically
    if (DoPlayerTestCrawl(pp))
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    pp->Flags &= ~(PF_CRAWLING|PF_JUMPING|PF_FALLING|PF_CLIMBING);

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
        NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Run);
    else
        NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Stand);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoPlayerRun(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

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
    if ((pp->cmd.ucmd.actions & SB_CROUCH) || pp->cmd.ucmd.vel.Z < 0)
    {
        DoPlayerBeginCrawl(pp);
        return;
    }

    // Jump
    if (pp->cmd.ucmd.actions & SB_JUMP)
    {
        if (pp->KeyPressBits & SB_JUMP)
        {
            pp->KeyPressBits &= ~SB_JUMP;
            // make sure you stand at full heights for jumps/double jumps
            pp->posZset(pp->loz - PLAYER_HEIGHTF);
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
            if (pp->cmd.ucmd.actions & SB_OPEN)
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
                        auto sActor = FindNearSprite(pp->GetActor(), STAT_TRIGGER);
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
                NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Run);
        }
        else
        {
            if (plActor->user.Rot != plActor->user.ActorActionSet->Stand)
                NewStateGroup(pp->GetActor(), plActor->user.ActorActionSet->Stand);
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

    if ((pp->cursector && pp->cursector->extra & SECTFX_DYNAMIC_AREA))
    {
        pp->posZset(pp->loz - PLAYER_HEIGHTF);
    }

    DoPlayerBob(pp);

    // Adjust height moving up and down sectors
    DoPlayerHeight(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

    // Set the correct pic
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void MoveSkipSavePos(void)
{
    int i;
    short pnum;
    DSWPlayer* pp;

    MoveSkip8 = (MoveSkip8 + 1) & 7;
    MoveSkip4 = (MoveSkip4 + 1) & 3;
    MoveSkip2 ^= 1;

    // Save off player
    TRAVERSE_CONNECT(pnum)
    {
        pp = getPlayer(pnum);

        pp->GetActor()->backuploc();
        pp->obob_z = pp->bob_z;
        pp->opbob_amt = pp->pbob_amt;
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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerTimers(DSWPlayer* pp)
{
    InventoryTimer(pp);
}

void ChopsCheck(DSWPlayer* pp)
{
    if (!M_Active() && !(pp->Flags & PF_DEAD) && !pp->sop_riding && numplayers <= 1)
    {
        if (pp->cmd.ucmd.actions & ~SB_RUN || !pp->cmd.ucmd.vel.XY().isZero() || !pp->cmd.ucmd.ang.isZero() ||
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
                    if (!pp->sop || !(pp->sop->flags & SOBJ_HAS_WEAPON))
                        pp->Flags &= ~(PF_WEAPON_DOWN);
                    ChopsSetRetract(pp);
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerGlobal(DSWPlayer* pp)
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
                    PlayerUpdateHealth(pp, -pp->GetActor()->user.Health);  // Make sure he dies!
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
    if (pp->circle_camera_dist > CIRCLE_CAMERA_DIST_MINF)
        pp->circle_camera_ang += mapangle(14);

    if (pp->camera_check_time_delay > 0)
    {
        pp->camera_check_time_delay -= synctics;
        if (pp->camera_check_time_delay <= 0)
            pp->camera_check_time_delay = 0;
    }
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void MultiPlayLimits(void)
{
    short pnum;
    DSWPlayer* pp;
    bool Done = false;

    if (gNet.MultiGameType != MULTI_GAME_COMMBAT)
        return;

    if (gNet.KillLimit)
    {
        TRAVERSE_CONNECT(pnum)
        {
            if (Level.kills.player[pnum] >= gNet.KillLimit)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void domovethings(void)
{
    short pnum;

    DSWPlayer* pp;
    extern int FinishTimer;


    UpdateInterpolations();                  // Stick at beginning of domovethings
    so_updateinterpolations();               // Stick at beginning of domovethings
    MoveSkipSavePos();

    if (paused)
    {
        if (!ReloadPrompt)
            return;
    }

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
        extern DSWPlayer* GlobPlayerP;

        pp = getPlayer(pnum);
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

        // process weapon bits
        processWeapon(pp);

        // auto tracking mode for single player multi-game
        if (numplayers <= 1 && PlayerTrackingMode && pnum == screenpeek && screenpeek != myconnectindex)
        {
            getPlayer(screenpeek)->GetActor()->spr.Angles.Yaw = (getPlayer(myconnectindex)->GetActor()->spr.pos.XY() - getPlayer(screenpeek)->GetActor()->spr.pos.XY()).Angle();
        }

        if (!(pp->Flags & PF_DEAD))
        {
            WeaponOperate(pp);
            PlayerOperateEnv(pp);
        }

        // do for moving sectors
        DoPlayerSectorUpdatePreMove(pp);
        ChopsCheck(pp);

        // Get strafe value before it's rotated by the angle.
        pp->svel = pp->cmd.ucmd.vel.Y;

        // convert fvel/svel into a vector before performing actions.
        pp->cmd.ucmd.vel.X += pp->cmd.ucmd.vel.Z * (pp->DoPlayerAction == DoPlayerClimb);
        pp->cmd.ucmd.vel.SetXY(pp->cmd.ucmd.vel.XY().Rotated(pp->GetActor()->spr.Angles.Yaw));

        if (pp->DoPlayerAction) pp->DoPlayerAction(pp);

        UpdatePlayerSprite(pp);

        pSpriteControl(pp);

        PlayerStateControl(pp->GetActor());

        DoPlayerSectorUpdatePostMove(pp);
        PlayerGlobal(pp);
    }

    MultiPlayLimits();

    PlayClock += synctics;

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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitAllPlayers(void)
{
    DSWPlayer* pp;
    DSWPlayer* pfirst = getPlayer(0);
    extern bool NewGame;
    //int fz,cz;

    // Initialize all [MAX_SW_PLAYERS] arrays here!
    for (int i = 0; i < MAX_SW_PLAYERS; i++)
    {
        auto pp = getPlayer(i);

        pp->cursector = pfirst->cursector;
        // set like this so that player can trigger something on start of the level
        pp->lastcursector = pfirst->cursector+1;

        //pp->MaxHealth = 100;

        pp->climb_ndx = 10;
        pp->KillerActor = nullptr;
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
            for (unsigned j = 0; j < MAX_INVENTORY; j++)
            {
                pp->InventoryAmount[j] = 0;
                pp->InventoryPercent[j] = 0;
            }
        }

        // My palette flashing stuff
        pp->FadeAmt = 0;
        pp->FadeTics = 0;
        pp->StartColor = 0;

        INITLIST(pp->PanelSpriteList);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SearchSpawnPosition(DSWPlayer* pp)
{
    DSWPlayer* opp; // other player
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
            opp = getPlayer(pnum);

            if (opp != pp)  // don't test for yourself
            {
                if ((spawn_sprite->spr.pos.XY() - opp->GetActor()->spr.pos.XY()).Length() < 62.25)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerSpawnPosition(DSWPlayer* pp)
{
    short pnum = short(pp->pnum);
    short pos_num = pnum;
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

    pp->setcursector(spawn_sprite->sector());

    if (pp->GetActor())
    {
        pp->GetActor()->spr.Angles.Yaw = spawn_sprite->spr.Angles.Yaw;
        pp->GetActor()->spr.pos = spawn_sprite->spr.pos;
        pp->GetActor()->viewzoffset = spawn_sprite->viewzoffset;
        pp->GetActor()->backuploc();
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitMultiPlayerInfo(const DVector3& spawnpos, const DAngle startang)
{
    DSWPlayer* pp;
    short pnum;
    unsigned stat;
    int tag;
    double fz,cz;
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
    pp = getPlayer(0);
    for (stat = 0; stat < SIZ(MultiStatList); stat++)
    {
        if (gNet.MultiGameType != MULTI_GAME_NONE)
        {
            // if start position is physically set then don't spawn a new one
            it.Reset(MultiStatList[stat]);
            if (it.Next())
                continue;
        }

        auto start0 = SpawnActor(MultiStatList[stat], ST1, nullptr, pp->cursector, spawnpos.plusZ(PLAYER_HEIGHTF), startang);
        start0->viewzoffset = -PLAYER_HEIGHTF;

        // if too close to the floor - stand up
        calcSlope(pp->cursector, start0->getPosWithOffsetZ(), &cz, &fz);
        if (start0->spr.pos.Z > fz)
        {
            start0->spr.pos.Z = fz;
        }
        start0->backuploc();
        start0->clearUser();
        start0->spr.picnum = ST1;
    }

    memset(SpawnPositionUsed,0,sizeof(SpawnPositionUsed));

    // Initialize multi player positions here
    //for (pp = Player; pp < Player + numplayers; pp++)
    TRAVERSE_CONNECT(pnum)
    {
        pp = getPlayer(pnum);
        switch (gNet.MultiGameType)
        {
        case MULTI_GAME_NONE:
            PlayerSpawnPosition(pp);
            break;
        //return;
        case MULTI_GAME_COMMBAT:
        case MULTI_GAME_AI_BOTS:
            // there are no keys in deathmatch play
            memset(getPlayer(0)->HasKey,0xFFFF,sizeof(getPlayer(0)->HasKey));
            memset(pp->HasKey,0xFFFF,sizeof(pp->HasKey));
            PlayerSpawnPosition(pp);
            break;
        case MULTI_GAME_COOPERATIVE:
            PlayerSpawnPosition(pp);
            break;
        }
    }
}

//---------------------------------------------------------------------------
//
// If player stepped in something gooey, track it all over the place.
//
//---------------------------------------------------------------------------

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

void CheckFootPrints(DSWPlayer* pp)
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

DEFINE_FIELD(DSWPlayer, actor)
DEFINE_FIELD(DSWPlayer, sop_remote)
DEFINE_FIELD(DSWPlayer, jump_count)
DEFINE_FIELD(DSWPlayer, jump_speed)
DEFINE_FIELD(DSWPlayer, z_speed)
DEFINE_FIELD(DSWPlayer, climb_ndx)
DEFINE_FIELD(DSWPlayer, hiz)
DEFINE_FIELD(DSWPlayer, loz)
DEFINE_FIELD(DSWPlayer, p_ceiling_dist)
DEFINE_FIELD(DSWPlayer, p_floor_dist)
DEFINE_FIELD(DSWPlayer, circle_camera_dist)
//DEFINE_FIELD(DSWPlayer, six)
//DEFINE_FIELD(DSWPlayer, siy)
//DEFINE_FIELD(DSWPlayer, siz)
//DEFINE_FIELD(DSWPlayer, xvect)
//DEFINE_FIELD(DSWPlayer, yvect)
//DEFINE_FIELD(DSWPlayer, oxvect)
//DEFINE_FIELD(DSWPlayer, oyvect)
DEFINE_FIELD(DSWPlayer, friction)
//DEFINE_FIELD(DSWPlayer, slide_xvect)
//DEFINE_FIELD(DSWPlayer, slide_yvect)
DEFINE_FIELD(DSWPlayer, slide_ang)
DEFINE_FIELD(DSWPlayer, slide_dec)
DEFINE_FIELD(DSWPlayer, circle_camera_ang)
DEFINE_FIELD(DSWPlayer, camera_check_time_delay)
DEFINE_FIELD(DSWPlayer, cursector)
DEFINE_FIELD(DSWPlayer, lastcursector)
DEFINE_FIELD(DSWPlayer, recoil_amt)
DEFINE_FIELD(DSWPlayer, recoil_speed)
DEFINE_FIELD(DSWPlayer, recoil_ndx)
DEFINE_FIELD(DSWPlayer, recoil_horizoff)
DEFINE_FIELD(DSWPlayer, recoil_ohorizoff)
//DEFINE_FIELD(DSWPlayer, oldposx)
//DEFINE_FIELD(DSWPlayer, oldposy)
//DEFINE_FIELD(DSWPlayer, oldposz)
//DEFINE_FIELD(DSWPlayer, Revolve)
DEFINE_FIELD(DSWPlayer, RevolveDeltaAng)
DEFINE_FIELD(DSWPlayer, pnum)
DEFINE_FIELD(DSWPlayer, LadderSector)
//DEFINE_FIELD_X(DSWPlayer, lx)
//DEFINE_FIELD_X(DSWPlayer, ly)
DEFINE_FIELD(DSWPlayer, JumpDuration)
DEFINE_FIELD(DSWPlayer, WadeDepth)
DEFINE_FIELD(DSWPlayer, pbob_amt)
DEFINE_FIELD(DSWPlayer, bob_ndx)
DEFINE_FIELD(DSWPlayer, bcnt)
DEFINE_FIELD(DSWPlayer, bob_z)
DEFINE_FIELD(DSWPlayer, obob_z)
DEFINE_FIELD(DSWPlayer, playerreadyflag)
DEFINE_FIELD(DSWPlayer, Flags)
DEFINE_FIELD(DSWPlayer, Flags2)
DEFINE_FIELD(DSWPlayer, HasKey)
DEFINE_FIELD(DSWPlayer, SwordAng)
DEFINE_FIELD(DSWPlayer, WpnGotOnceFlags)
DEFINE_FIELD(DSWPlayer, WpnFlags)
DEFINE_FIELD(DSWPlayer, WpnAmmo)
DEFINE_FIELD(DSWPlayer, WpnNum)
DEFINE_FIELD(DSWPlayer, WpnRocketType)
DEFINE_FIELD(DSWPlayer, WpnRocketHeat)
DEFINE_FIELD(DSWPlayer, WpnRocketNuke)
DEFINE_FIELD(DSWPlayer, WpnFlameType)
DEFINE_FIELD(DSWPlayer, WpnFirstType)
DEFINE_FIELD(DSWPlayer, WeaponType)
DEFINE_FIELD(DSWPlayer, FirePause)
DEFINE_FIELD(DSWPlayer, InventoryNum)
DEFINE_FIELD(DSWPlayer, InventoryBarTics)
DEFINE_FIELD(DSWPlayer, InventoryTics)
DEFINE_FIELD(DSWPlayer, InventoryPercent)
DEFINE_FIELD(DSWPlayer, InventoryAmount)
DEFINE_FIELD(DSWPlayer, InventoryActive)
DEFINE_FIELD(DSWPlayer, DiveTics)
DEFINE_FIELD(DSWPlayer, DiveDamageTics)
DEFINE_FIELD(DSWPlayer, DeathType)
DEFINE_FIELD(DSWPlayer, Armor)
DEFINE_FIELD(DSWPlayer, MaxHealth)
DEFINE_FIELD(DSWPlayer, UziShellLeftAlt)
DEFINE_FIELD(DSWPlayer, UziShellRightAlt)
DEFINE_FIELD(DSWPlayer, TeamColor)
DEFINE_FIELD(DSWPlayer, FadeTics)
DEFINE_FIELD(DSWPlayer, FadeAmt)
DEFINE_FIELD(DSWPlayer, NightVision)
DEFINE_FIELD(DSWPlayer, StartColor)
DEFINE_FIELD(DSWPlayer, IsAI)
DEFINE_FIELD(DSWPlayer, fta)
DEFINE_FIELD(DSWPlayer, ftq)
DEFINE_FIELD(DSWPlayer, NumFootPrints)
DEFINE_FIELD(DSWPlayer, WpnUziType)
DEFINE_FIELD(DSWPlayer, WpnShotgunType)
DEFINE_FIELD(DSWPlayer, WpnShotgunAuto)
DEFINE_FIELD(DSWPlayer, WpnShotgunLastShell)
DEFINE_FIELD(DSWPlayer, WpnRailType)
DEFINE_FIELD(DSWPlayer, Bloody)
DEFINE_FIELD(DSWPlayer, InitingNuke)
DEFINE_FIELD(DSWPlayer, TestNukeInit)
DEFINE_FIELD(DSWPlayer, NukeInitialized)
DEFINE_FIELD(DSWPlayer, FistAng)
DEFINE_FIELD(DSWPlayer, WpnKungFuMove)
DEFINE_FIELD(DSWPlayer, HitBy)
DEFINE_FIELD(DSWPlayer, Reverb)
DEFINE_FIELD(DSWPlayer, Heads)
DEFINE_FIELD(DSWPlayer, PlayerVersion)
DEFINE_FIELD(DSWPlayer, WpnReloadState)

DEFINE_ACTION_FUNCTION(_SWPlayer, WeaponNum)
{
    PARAM_SELF_STRUCT_PROLOGUE(DSWPlayer);
    ACTION_RETURN_INT(self->GetActor()->user.WeaponNum);
}

DEFINE_ACTION_FUNCTION(_SWPlayer, Health)
{
    PARAM_SELF_STRUCT_PROLOGUE(DSWPlayer);
    ACTION_RETURN_INT(self->GetActor()->user.Health);
}

DEFINE_ACTION_FUNCTION(_SWPlayer, MaxUserHealth)
{
    PARAM_SELF_STRUCT_PROLOGUE(DSWPlayer);
    ACTION_RETURN_INT(self->GetActor()->user.MaxHealth);
}

DEFINE_ACTION_FUNCTION(_SWPlayer, GetBuildAngle)
{
    PARAM_SELF_STRUCT_PROLOGUE(DSWPlayer);
    ACTION_RETURN_INT(self->GetActor()->spr.Angles.Yaw.Buildang());
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
    ACTION_RETURN_POINTER(getPlayer(screenpeek));
}

DEFINE_ACTION_FUNCTION(_SW, RealWeapon)
{
    PARAM_PROLOGUE;
    PARAM_INT(inv);
    int w = DamageData[inv].with_weapon;
    ACTION_RETURN_INT(w == -1? inv : w);
}

END_SW_NS
