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

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "pal.h"
#include "player.h"
#include "net.h"
#include "sprite.h"
#include "weapon.h"
#include "actor.h"
#include "track.h"

//extern short Zombies;
#define ZOMBIE_TIME_LIMIT ((120*20)/ACTORMOVETICS)

DECISION ZombieBattle[] =
{
    {399, InitActorMoveCloser},
    {1024, InitActorAttack}
};

DECISION ZombieOffense[] =
{
    {399, InitActorMoveCloser},
    {1024, InitActorAttack}
};

DECISION ZombieBroadcast[] =
{
    {6, InitActorAmbientNoise},
    {1024, InitActorDecide}
};

DECISION ZombieSurprised[] =
{
    {701, InitActorMoveCloser},
    {1024, InitActorDecide}
};

DECISION ZombieEvasive[] =
{
    {400,   InitActorDuck},
    {1024,  NULL}
};

DECISION ZombieLostTarget[] =
{
    {900, InitActorFindPlayer},
    {1024, InitActorWanderAround}
};

DECISION ZombieCloseRange[] =
{
    {800,   InitActorAttack             },
    {1024,  InitActorReposition         }
};

PERSONALITY ZombiePersonality =
{
    ZombieBattle,
    ZombieOffense,
    ZombieBroadcast,
    ZombieSurprised,
    ZombieEvasive,
    ZombieLostTarget,
    ZombieCloseRange,
    ZombieCloseRange
};

ATTRIBUTE ZombieAttrib =
{
    //{50, 60, 70, 80},                   // Speeds
    {120, 140, 170, 200},                 // Speeds
    {4, 0, 0, -2},                      // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        DIGI_NINJAAMBIENT, DIGI_NINJAALERT, DIGI_STAR,
        DIGI_NINJAPAIN, DIGI_NINJASCREAM,0,0,0,0,0
    }
};


//////////////////////
//
// ZOMBIE RUN
//
//////////////////////

#define ZOMBIE_RATE 32
ANIMATOR DoZombieMove, NullZombie;

STATE s_ZombieRun[5][4] =
{
    {
        {PLAYER_NINJA_RUN_R0 + 0, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[0][1]},
        {PLAYER_NINJA_RUN_R0 + 1, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[0][2]},
        {PLAYER_NINJA_RUN_R0 + 2, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[0][3]},
        {PLAYER_NINJA_RUN_R0 + 3, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[0][0]},
    },
    {
        {PLAYER_NINJA_RUN_R1 + 0, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[1][1]},
        {PLAYER_NINJA_RUN_R1 + 1, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[1][2]},
        {PLAYER_NINJA_RUN_R1 + 2, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[1][3]},
        {PLAYER_NINJA_RUN_R1 + 3, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[1][0]},
    },
    {
        {PLAYER_NINJA_RUN_R2 + 0, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[2][1]},
        {PLAYER_NINJA_RUN_R2 + 1, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[2][2]},
        {PLAYER_NINJA_RUN_R2 + 2, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[2][3]},
        {PLAYER_NINJA_RUN_R2 + 3, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[2][0]},
    },
    {
        {PLAYER_NINJA_RUN_R3 + 0, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[3][1]},
        {PLAYER_NINJA_RUN_R3 + 1, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[3][2]},
        {PLAYER_NINJA_RUN_R3 + 2, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[3][3]},
        {PLAYER_NINJA_RUN_R3 + 3, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[3][0]},
    },
    {
        {PLAYER_NINJA_RUN_R4 + 0, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[4][1]},
        {PLAYER_NINJA_RUN_R4 + 1, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[4][2]},
        {PLAYER_NINJA_RUN_R4 + 2, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[4][3]},
        {PLAYER_NINJA_RUN_R4 + 3, ZOMBIE_RATE | SF_TIC_ADJUST, DoZombieMove, &s_ZombieRun[4][0]},
    },
};

STATEp sg_ZombieRun[] =
{
    s_ZombieRun[0],
    s_ZombieRun[1],
    s_ZombieRun[2],
    s_ZombieRun[3],
    s_ZombieRun[4]
};

//////////////////////
//
// PLAYER_ZOMBIE STAND
//
//////////////////////

#define ZOMBIE_STAND_RATE 10

STATE s_ZombieStand[5][1] =
{
    {
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_STAND_RATE, NullZombie, &s_ZombieStand[0][0]},
    },
    {
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_STAND_RATE, NullZombie, &s_ZombieStand[1][0]},
    },
    {
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_STAND_RATE, NullZombie, &s_ZombieStand[2][0]},
    },
    {
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_STAND_RATE, NullZombie, &s_ZombieStand[3][0]},
    },
    {
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_STAND_RATE, NullZombie, &s_ZombieStand[4][0]},
    },
};

STATEp sg_ZombieStand[] =
{
    s_ZombieStand[0],
    s_ZombieStand[1],
    s_ZombieStand[2],
    s_ZombieStand[3],
    s_ZombieStand[4]
};

//////////////////////
//
// ZOMBIE PAIN
//
//////////////////////

#define ZOMBIE_PAIN_RATE 15
ANIMATOR DoZombiePain;

STATE s_ZombiePain[5][2] =
{
    {
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_PAIN_RATE, DoZombiePain, &s_ZombiePain[0][1]},
        {PLAYER_NINJA_STAND_R0 + 1, ZOMBIE_PAIN_RATE, DoZombiePain, &s_ZombiePain[0][1]},
    },
    {
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_PAIN_RATE, DoZombiePain, &s_ZombiePain[1][1]},
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_PAIN_RATE, DoZombiePain, &s_ZombiePain[1][1]},
    },
    {
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_PAIN_RATE, DoZombiePain, &s_ZombiePain[2][1]},
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_PAIN_RATE, DoZombiePain, &s_ZombiePain[2][1]},
    },
    {
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_PAIN_RATE, DoZombiePain, &s_ZombiePain[3][1]},
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_PAIN_RATE, DoZombiePain, &s_ZombiePain[3][1]},
    },
    {
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_PAIN_RATE, DoZombiePain, &s_ZombiePain[4][1]},
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_PAIN_RATE, DoZombiePain, &s_ZombiePain[4][1]},
    },
};

STATEp sg_ZombiePain[] =
{
    s_ZombiePain[0],
    s_ZombiePain[1],
    s_ZombiePain[2],
    s_ZombiePain[3],
    s_ZombiePain[4]
};

//////////////////////
//
// ZOMBIE NUKE
//
//////////////////////

#define ZOMBIE_NUKE_RATE 18
ANIMATOR InitEnemyNuke;

STATE s_ZombieNuke[5][6] =
{
    {
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_NUKE_RATE * 2, NullZombie, &s_ZombieNuke[0][1]},
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_NUKE_RATE, NullZombie, &s_ZombieNuke[0][2]},
        {PLAYER_NINJA_SHOOT_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyNuke, &s_ZombieNuke[0][3]},
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_NUKE_RATE * 2, NullZombie, &s_ZombieNuke[0][4]},
        {PLAYER_NINJA_SHOOT_R0 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieNuke[0][5]},
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_NUKE_RATE, DoZombieMove, &s_ZombieNuke[0][5]},
    },
    {
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_NUKE_RATE * 2, NullZombie, &s_ZombieNuke[1][1]},
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_NUKE_RATE, NullZombie, &s_ZombieNuke[1][2]},
        {PLAYER_NINJA_SHOOT_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyNuke, &s_ZombieNuke[1][3]},
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_NUKE_RATE * 2, NullZombie, &s_ZombieNuke[1][4]},
        {PLAYER_NINJA_SHOOT_R1 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieNuke[1][5]},
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_NUKE_RATE, DoZombieMove, &s_ZombieNuke[1][5]},
    },
    {
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE * 2, NullZombie, &s_ZombieNuke[2][1]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE, NullZombie, &s_ZombieNuke[2][2]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyNuke, &s_ZombieNuke[2][3]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE * 2, NullZombie, &s_ZombieNuke[2][4]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieNuke[2][5]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE, DoZombieMove, &s_ZombieNuke[2][5]},
    },
    {
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE * 2, NullZombie, &s_ZombieNuke[3][1]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE, NullZombie, &s_ZombieNuke[3][2]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyNuke, &s_ZombieNuke[3][3]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE * 2, NullZombie, &s_ZombieNuke[3][4]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieNuke[3][5]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE, DoZombieMove, &s_ZombieNuke[3][5]},
    },
    {
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE * 2, NullZombie, &s_ZombieNuke[4][1]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE, NullZombie, &s_ZombieNuke[4][2]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyNuke, &s_ZombieNuke[4][3]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE * 2, NullZombie, &s_ZombieNuke[4][4]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieNuke[4][5]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_NUKE_RATE, DoZombieMove, &s_ZombieNuke[4][5]},
    },
};

STATEp sg_ZombieNuke[] =
{
    s_ZombieNuke[0],
    s_ZombieNuke[1],
    s_ZombieNuke[2],
    s_ZombieNuke[3],
    s_ZombieNuke[4]
};


//////////////////////
//
// ZOMBIE ROCKET
//
//////////////////////

#define ZOMBIE_ROCKET_RATE 14
ANIMATOR InitEnemyRocket;

STATE s_ZombieRocket[5][5] =
{
    {
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_ROCKET_RATE * 2, NullZombie, &s_ZombieRocket[0][1]},
        {PLAYER_NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyRocket, &s_ZombieRocket[0][2]},
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_ROCKET_RATE, NullZombie, &s_ZombieRocket[0][3]},
        {PLAYER_NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieRocket[0][4]},
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_ROCKET_RATE, DoZombieMove, &s_ZombieRocket[0][4]},
    },
    {
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_ROCKET_RATE * 2, NullZombie, &s_ZombieRocket[1][1]},
        {PLAYER_NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyRocket, &s_ZombieRocket[1][2]},
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_ROCKET_RATE, NullZombie, &s_ZombieRocket[1][3]},
        {PLAYER_NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieRocket[1][4]},
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_ROCKET_RATE, DoZombieMove, &s_ZombieRocket[1][4]},
    },
    {
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_ROCKET_RATE * 2, NullZombie, &s_ZombieRocket[2][1]},
        {PLAYER_NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyRocket, &s_ZombieRocket[2][2]},
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_ROCKET_RATE, NullZombie, &s_ZombieRocket[2][3]},
        {PLAYER_NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieRocket[2][4]},
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_ROCKET_RATE, DoZombieMove, &s_ZombieRocket[2][4]},
    },
    {
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_ROCKET_RATE * 2, NullZombie, &s_ZombieRocket[3][1]},
        {PLAYER_NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyRocket, &s_ZombieRocket[3][2]},
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_ROCKET_RATE, NullZombie, &s_ZombieRocket[3][3]},
        {PLAYER_NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieRocket[3][4]},
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_ROCKET_RATE, DoZombieMove, &s_ZombieRocket[3][4]},
    },
    {
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_ROCKET_RATE * 2, NullZombie, &s_ZombieRocket[4][1]},
        {PLAYER_NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyRocket, &s_ZombieRocket[4][2]},
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_ROCKET_RATE, NullZombie, &s_ZombieRocket[4][3]},
        {PLAYER_NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieRocket[4][4]},
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_ROCKET_RATE, DoZombieMove, &s_ZombieRocket[4][4]},
    },
};


STATEp sg_ZombieRocket[] =
{
    s_ZombieRocket[0],
    s_ZombieRocket[1],
    s_ZombieRocket[2],
    s_ZombieRocket[3],
    s_ZombieRocket[4]
};

//////////////////////
//
// ZOMBIE ROCKET
//
//////////////////////

#define ZOMBIE_RAIL_RATE 14
ANIMATOR InitEnemyRail;

STATE s_ZombieRail[5][5] =
{
    {
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_RAIL_RATE * 2, NullZombie, &s_ZombieRail[0][1]},
        {PLAYER_NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyRail, &s_ZombieRail[0][2]},
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_RAIL_RATE, NullZombie, &s_ZombieRail[0][3]},
        {PLAYER_NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieRail[0][4]},
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_RAIL_RATE, DoZombieMove, &s_ZombieRail[0][4]},
    },
    {
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_RAIL_RATE * 2, NullZombie, &s_ZombieRail[1][1]},
        {PLAYER_NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyRail, &s_ZombieRail[1][2]},
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_RAIL_RATE, NullZombie, &s_ZombieRail[1][3]},
        {PLAYER_NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieRail[1][4]},
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_RAIL_RATE, DoZombieMove, &s_ZombieRail[1][4]},
    },
    {
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_RAIL_RATE * 2, NullZombie, &s_ZombieRail[2][1]},
        {PLAYER_NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyRail, &s_ZombieRail[2][2]},
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_RAIL_RATE, NullZombie, &s_ZombieRail[2][3]},
        {PLAYER_NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieRail[2][4]},
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_RAIL_RATE, DoZombieMove, &s_ZombieRail[2][4]},
    },
    {
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_RAIL_RATE * 2, NullZombie, &s_ZombieRail[3][1]},
        {PLAYER_NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyRail, &s_ZombieRail[3][2]},
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_RAIL_RATE, NullZombie, &s_ZombieRail[3][3]},
        {PLAYER_NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieRail[3][4]},
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_RAIL_RATE, DoZombieMove, &s_ZombieRail[3][4]},
    },
    {
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_RAIL_RATE * 2, NullZombie, &s_ZombieRail[4][1]},
        {PLAYER_NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyRail, &s_ZombieRail[4][2]},
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_RAIL_RATE, NullZombie, &s_ZombieRail[4][3]},
        {PLAYER_NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieRail[4][4]},
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_RAIL_RATE, DoZombieMove, &s_ZombieRail[4][4]},
    },
};


STATEp sg_ZombieRail[] =
{
    s_ZombieRail[0],
    s_ZombieRail[1],
    s_ZombieRail[2],
    s_ZombieRail[3],
    s_ZombieRail[4]
};

//////////////////////
//
// ZOMBIE GRENADE
//
//////////////////////

#define ZOMBIE_ROCKET_RATE 14
ANIMATOR InitSpriteGrenade;

STATE s_ZombieGrenade[5][5] =
{
    {
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_ROCKET_RATE * 2, NullZombie, &s_ZombieGrenade[0][1]},
        {PLAYER_NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitSpriteGrenade, &s_ZombieGrenade[0][2]},
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_ROCKET_RATE, NullZombie, &s_ZombieGrenade[0][3]},
        {PLAYER_NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieGrenade[0][4]},
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_ROCKET_RATE, DoZombieMove, &s_ZombieGrenade[0][4]},
    },
    {
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_ROCKET_RATE * 2, NullZombie, &s_ZombieGrenade[1][1]},
        {PLAYER_NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitSpriteGrenade, &s_ZombieGrenade[1][2]},
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_ROCKET_RATE, NullZombie, &s_ZombieGrenade[1][3]},
        {PLAYER_NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieGrenade[1][4]},
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_ROCKET_RATE, DoZombieMove, &s_ZombieGrenade[1][4]},
    },
    {
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_ROCKET_RATE * 2, NullZombie, &s_ZombieGrenade[2][1]},
        {PLAYER_NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitSpriteGrenade, &s_ZombieGrenade[2][2]},
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_ROCKET_RATE, NullZombie, &s_ZombieGrenade[2][3]},
        {PLAYER_NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieGrenade[2][4]},
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_ROCKET_RATE, DoZombieMove, &s_ZombieGrenade[2][4]},
    },
    {
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_ROCKET_RATE * 2, NullZombie, &s_ZombieGrenade[3][1]},
        {PLAYER_NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitSpriteGrenade, &s_ZombieGrenade[3][2]},
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_ROCKET_RATE, NullZombie, &s_ZombieGrenade[3][3]},
        {PLAYER_NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieGrenade[3][4]},
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_ROCKET_RATE, DoZombieMove, &s_ZombieGrenade[3][4]},
    },
    {
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_ROCKET_RATE * 2, NullZombie, &s_ZombieGrenade[4][1]},
        {PLAYER_NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitSpriteGrenade, &s_ZombieGrenade[4][2]},
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_ROCKET_RATE, NullZombie, &s_ZombieGrenade[4][3]},
        {PLAYER_NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieGrenade[4][4]},
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_ROCKET_RATE, DoZombieMove, &s_ZombieGrenade[4][4]},
    },
};


STATEp sg_ZombieGrenade[] =
{
    s_ZombieGrenade[0],
    s_ZombieGrenade[1],
    s_ZombieGrenade[2],
    s_ZombieGrenade[3],
    s_ZombieGrenade[4]
};


//////////////////////
//
// ZOMBIE FLASHBOMB
//
//////////////////////

#define ZOMBIE_FLASHBOMB_RATE 14
ANIMATOR InitFlashBomb;

STATE s_ZombieFlashBomb[5][5] =
{
    {
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_FLASHBOMB_RATE * 2, NullZombie, &s_ZombieFlashBomb[0][1]},
        {PLAYER_NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitFlashBomb, &s_ZombieFlashBomb[0][2]},
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_FLASHBOMB_RATE, NullZombie, &s_ZombieFlashBomb[0][3]},
        {PLAYER_NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieFlashBomb[0][4]},
        {PLAYER_NINJA_STAND_R0 + 0, ZOMBIE_FLASHBOMB_RATE, DoZombieMove, &s_ZombieFlashBomb[0][4]},
    },
    {
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_FLASHBOMB_RATE * 2, NullZombie, &s_ZombieFlashBomb[1][1]},
        {PLAYER_NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitFlashBomb, &s_ZombieFlashBomb[1][2]},
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_FLASHBOMB_RATE, NullZombie, &s_ZombieFlashBomb[1][3]},
        {PLAYER_NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieFlashBomb[1][4]},
        {PLAYER_NINJA_STAND_R1 + 0, ZOMBIE_FLASHBOMB_RATE, DoZombieMove, &s_ZombieFlashBomb[1][4]},
    },
    {
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_FLASHBOMB_RATE * 2, NullZombie, &s_ZombieFlashBomb[2][1]},
        {PLAYER_NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitFlashBomb, &s_ZombieFlashBomb[2][2]},
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_FLASHBOMB_RATE, NullZombie, &s_ZombieFlashBomb[2][3]},
        {PLAYER_NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieFlashBomb[2][4]},
        {PLAYER_NINJA_STAND_R2 + 0, ZOMBIE_FLASHBOMB_RATE, DoZombieMove, &s_ZombieFlashBomb[2][4]},
    },
    {
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_FLASHBOMB_RATE * 2, NullZombie, &s_ZombieFlashBomb[3][1]},
        {PLAYER_NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitFlashBomb, &s_ZombieFlashBomb[3][2]},
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_FLASHBOMB_RATE, NullZombie, &s_ZombieFlashBomb[3][3]},
        {PLAYER_NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieFlashBomb[3][4]},
        {PLAYER_NINJA_STAND_R3 + 0, ZOMBIE_FLASHBOMB_RATE, DoZombieMove, &s_ZombieFlashBomb[3][4]},
    },
    {
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_FLASHBOMB_RATE * 2, NullZombie, &s_ZombieFlashBomb[4][1]},
        {PLAYER_NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitFlashBomb, &s_ZombieFlashBomb[4][2]},
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_FLASHBOMB_RATE, NullZombie, &s_ZombieFlashBomb[4][3]},
        {PLAYER_NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieFlashBomb[4][4]},
        {PLAYER_NINJA_STAND_R4 + 0, ZOMBIE_FLASHBOMB_RATE, DoZombieMove, &s_ZombieFlashBomb[4][4]},
    },
};


STATEp sg_ZombieFlashBomb[] =
{
    s_ZombieFlashBomb[0],
    s_ZombieFlashBomb[1],
    s_ZombieFlashBomb[2],
    s_ZombieFlashBomb[3],
    s_ZombieFlashBomb[4]
};

//////////////////////
//
// ZOMBIE UZI
//
//////////////////////

#define ZOMBIE_UZI_RATE 8
ANIMATOR InitEnemyUzi,CheckFire;

STATE s_ZombieUzi[5][17] =
{
    {
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[0][1]},
        {PLAYER_NINJA_SHOOT_R0 + 0, 0 | SF_QUICK_CALL, CheckFire, &s_ZombieUzi[0][2]},
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[0][3]},
        {PLAYER_NINJA_SHOOT_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[0][4]},
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[0][5]},
        {PLAYER_NINJA_SHOOT_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[0][6]},
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[0][7]},
        {PLAYER_NINJA_SHOOT_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[0][8]},
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[0][9]},
        {PLAYER_NINJA_SHOOT_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[0][10]},
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[0][11]},
        {PLAYER_NINJA_SHOOT_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[0][12]},
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[0][13]},
        {PLAYER_NINJA_SHOOT_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[0][14]},
        {PLAYER_NINJA_SHOOT_R0 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[0][15]},
        {PLAYER_NINJA_SHOOT_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[0][16]},
        {PLAYER_NINJA_SHOOT_R0 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieUzi[0][16]},
    },
    {
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[1][1]},
        {PLAYER_NINJA_SHOOT_R1 + 0, 0 | SF_QUICK_CALL, CheckFire, &s_ZombieUzi[1][2]},
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[1][3]},
        {PLAYER_NINJA_SHOOT_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[1][4]},
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[1][5]},
        {PLAYER_NINJA_SHOOT_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[1][6]},
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[1][7]},
        {PLAYER_NINJA_SHOOT_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[1][8]},
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[1][9]},
        {PLAYER_NINJA_SHOOT_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[1][10]},
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[1][11]},
        {PLAYER_NINJA_SHOOT_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[1][12]},
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[1][13]},
        {PLAYER_NINJA_SHOOT_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[1][14]},
        {PLAYER_NINJA_SHOOT_R1 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[1][15]},
        {PLAYER_NINJA_SHOOT_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[1][16]},
        {PLAYER_NINJA_SHOOT_R1 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieUzi[1][16]},
    },
    {
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[2][1]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, CheckFire, &s_ZombieUzi[2][2]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[2][3]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[2][4]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[2][5]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[2][6]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[2][7]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[2][8]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[2][9]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[2][10]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[2][11]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[2][12]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[2][13]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[2][14]},
        {PLAYER_NINJA_SHOOT_R2 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[2][15]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[2][16]},
        {PLAYER_NINJA_SHOOT_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieUzi[2][16]},
    },
    {
        {PLAYER_NINJA_SHOOT_R3 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[3][1]},
        {PLAYER_NINJA_SHOOT_R3 + 0, 0 | SF_QUICK_CALL, CheckFire, &s_ZombieUzi[3][2]},
        {PLAYER_NINJA_SHOOT_R3 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[3][3]},
        {PLAYER_NINJA_SHOOT_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[3][4]},
        {PLAYER_NINJA_SHOOT_R3 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[3][5]},
        {PLAYER_NINJA_SHOOT_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[3][6]},
        {PLAYER_NINJA_SHOOT_R3 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[3][7]},
        {PLAYER_NINJA_SHOOT_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[3][8]},
        {PLAYER_NINJA_SHOOT_R3 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[3][9]},
        {PLAYER_NINJA_SHOOT_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[3][10]},
        {PLAYER_NINJA_SHOOT_R3 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[3][11]},
        {PLAYER_NINJA_SHOOT_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[3][12]},
        {PLAYER_NINJA_SHOOT_R3 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[3][13]},
        {PLAYER_NINJA_SHOOT_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[3][14]},
        {PLAYER_NINJA_SHOOT_R3 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[3][15]},
        {PLAYER_NINJA_SHOOT_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[3][16]},
        {PLAYER_NINJA_SHOOT_R3 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieUzi[3][16]},
    },
    {
        {PLAYER_NINJA_SHOOT_R4 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[4][1]},
        {PLAYER_NINJA_SHOOT_R4 + 0, 0 | SF_QUICK_CALL, CheckFire, &s_ZombieUzi[4][2]},
        {PLAYER_NINJA_SHOOT_R4 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[4][3]},
        {PLAYER_NINJA_SHOOT_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[4][4]},
        {PLAYER_NINJA_SHOOT_R4 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[4][5]},
        {PLAYER_NINJA_SHOOT_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[4][6]},
        {PLAYER_NINJA_SHOOT_R4 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[4][7]},
        {PLAYER_NINJA_SHOOT_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[4][8]},
        {PLAYER_NINJA_SHOOT_R4 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[4][9]},
        {PLAYER_NINJA_SHOOT_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[4][10]},
        {PLAYER_NINJA_SHOOT_R4 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[4][11]},
        {PLAYER_NINJA_SHOOT_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[4][12]},
        {PLAYER_NINJA_SHOOT_R4 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[4][13]},
        {PLAYER_NINJA_SHOOT_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[4][14]},
        {PLAYER_NINJA_SHOOT_R4 + 0, ZOMBIE_UZI_RATE, NullZombie, &s_ZombieUzi[4][15]},
        {PLAYER_NINJA_SHOOT_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ZombieUzi[4][16]},
        {PLAYER_NINJA_SHOOT_R4 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_ZombieUzi[4][16]},
    },
};


STATEp sg_ZombieUzi[] =
{
    s_ZombieUzi[0],
    s_ZombieUzi[1],
    s_ZombieUzi[2],
    s_ZombieUzi[3],
    s_ZombieUzi[4]
};

//////////////////////
//
// ZOMBIE FALL
//
//////////////////////

#define ZOMBIE_FALL_RATE 25

STATE s_ZombieFall[5][1] =
{
    {
        {PLAYER_NINJA_JUMP_R0 + 3, ZOMBIE_FALL_RATE, DoZombieMove, &s_ZombieFall[0][0]},
    },
    {
        {PLAYER_NINJA_JUMP_R1 + 3, ZOMBIE_FALL_RATE, DoZombieMove, &s_ZombieFall[1][0]},
    },
    {
        {PLAYER_NINJA_JUMP_R2 + 3, ZOMBIE_FALL_RATE, DoZombieMove, &s_ZombieFall[2][0]},
    },
    {
        {PLAYER_NINJA_JUMP_R3 + 3, ZOMBIE_FALL_RATE, DoZombieMove, &s_ZombieFall[3][0]},
    },
    {
        {PLAYER_NINJA_JUMP_R4 + 3, ZOMBIE_FALL_RATE, DoZombieMove, &s_ZombieFall[4][0]},
    }
};


STATEp sg_ZombieFall[] =
{
    &s_ZombieFall[0][0],
    &s_ZombieFall[1][0],
    &s_ZombieFall[2][0],
    &s_ZombieFall[3][0],
    &s_ZombieFall[4][0]
};

/*
STATEp *Stand[MAX_WEAPONS];
STATEp *Run;
STATEp *Jump;
STATEp *Fall;
STATEp *Crawl;
STATEp *Swim;
STATEp *Fly;
STATEp *Rise;
STATEp *Sit;
STATEp *Look;
STATEp *Climb;
STATEp *Pain;
STATEp *Death1;
STATEp *Death2;
STATEp *Dead;
STATEp *DeathJump;
STATEp *DeathFall;
STATEp *CloseAttack[2];
STATEp *Attack[6];
STATEp *Special[2];
*/

ACTOR_ACTION_SET ZombieActionSet =
{
    sg_ZombieStand,
    sg_ZombieRun,
    NULL,
    sg_ZombieFall,
    NULL,
    NULL,
    NULL,
    sg_ZombieRun,
    sg_ZombieRun,
    NULL,
    NULL,
    sg_ZombiePain,
    sg_ZombieRun,
    NULL,
    NULL,
    NULL,
    NULL,
#if 0
    {sg_ZombieUzi},
    {1024},
    {sg_ZombieUzi, sg_ZombieRocket, sg_ZombieGrenade, sg_ZombieNuke},
    {400, 750, 1000, 1024},
#endif
    {sg_ZombieRail},
    {1024},
    {sg_ZombieRail},
    {1024},
    {NULL},
    NULL,
    NULL
};

int
SetupZombie(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    ANIMATOR DoActorDecide;
    short pic = sp->picnum;

    u->Health = 100;
    u->StateEnd = &s_ZombiePain[0][0];
    u->Rot = sg_ZombieRun;
    sp->xrepeat = PLAYER_NINJA_XREPEAT;
    sp->yrepeat = PLAYER_NINJA_YREPEAT;

    u->Attrib = &ZombieAttrib;
    EnemyDefaults(SpriteNum, &ZombieActionSet, &ZombiePersonality);

    ChangeState(SpriteNum, s_ZombieRun[0]);
    DoActorSetSpeed(SpriteNum, NORM_SPEED);

    u->Radius = 280;
    SET(u->Flags, SPR_XFLIP_TOGGLE);

    return 0;
}

int
SpawnZombie(PLAYERp pp, short Weapon)
{
    short New,i,nexti;
    SPRITEp np,sp;
    USERp nu,u;
    short owner;

    owner = sprite[Weapon].owner;

    if (owner < 0)
        return -1;

    //Zombies++;

    New = SpawnSprite(STAT_ENEMY, ZOMBIE_RUN_R0, s_ZombieRun[0], pp->cursectnum, pp->posx, pp->posy, pp->posz, pp->pang, 0);
    np = &sprite[New];
    nu = User[New];
    np->sectnum = pp->cursectnum;
    np->owner = owner;
    np->pal = nu->spal = User[owner]->spal;
    np->ang = RANDOM_P2(2048);
    SetupZombie(New);
    //np->shade = sprite[pp->PlayerSprite].shade;
    np->shade = -10;
    SET(nu->Flags2, SPR2_DONT_TARGET_OWNER);
    SET(np->cstat, CSTAT_SPRITE_TRANSLUCENT);

    DoActorPickClosePlayer(New);
    //nu->tgt_sp = pp->SpriteP; // Make it target last killed player initially

    // make immediately active
    SET(nu->Flags, SPR_ACTIVE);

    RESET(nu->Flags, SPR_JUMPING);
    RESET(nu->Flags, SPR_FALLING);

    // if I didn't do this here they get stuck in the air sometimes
    DoActorZrange(New);

    return New;
}

int
SpawnZombie2(short Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    short New,i,nexti;
    SPRITEp np;
    USERp nu;
    short owner;
    SECT_USERp sectu = SectUser[sp->sectnum];
    SECTORp sectp = &sector[sp->sectnum];

    owner = sprite[Weapon].owner;

    if (owner < 0)
        return -1;

    if (sectu && (TEST(sectp->extra, SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE))
        return -1;

    if (SectorIsUnderwaterArea(sp->sectnum))
        return -1;

    //if (FAF_ConnectArea(sp->sectnum))
    //    return(-1);

    if (FAF_ConnectArea(sp->sectnum))
    {
        short sectnum = sp->sectnum;
        updatesectorz(sp->x, sp->y, sp->z + Z(10), &sectnum);
        if (sectnum >= 0 && SectorIsUnderwaterArea(sectnum))
            return -1;
    }


    //Zombies++;
    New = SpawnSprite(STAT_ENEMY, ZOMBIE_RUN_R0, s_ZombieRun[0], sp->sectnum, sp->x, sp->y, sp->z, sp->ang, 0);
    np = &sprite[New];
    nu = User[New];
    nu->Counter3 = 0;
    np->owner = owner;
    np->pal = nu->spal = User[owner]->spal;
    np->ang = RANDOM_P2(2048);
    SetupZombie(New);
    //np->shade = sprite[pp->PlayerSprite].shade;
    np->shade = -10;
    SET(nu->Flags2, SPR2_DONT_TARGET_OWNER);
    SET(np->cstat, CSTAT_SPRITE_TRANSLUCENT);

    DoActorPickClosePlayer(New);
    //nu->tgt_sp = pp->SpriteP; // Make it target last killed player initially

    // make immediately active
    SET(nu->Flags, SPR_ACTIVE);

    RESET(nu->Flags, SPR_JUMPING);
    RESET(nu->Flags, SPR_FALLING);

    // if I didn't do this here they get stuck in the air sometimes
    DoActorZrange(New);

    return New;
}

int
DoZombieMove(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (u->Counter3++ >= ZOMBIE_TIME_LIMIT)
    {
        InitBloodSpray(SpriteNum,TRUE,105);
        InitBloodSpray(SpriteNum,TRUE,105);
        InitBloodSpray(SpriteNum,TRUE,105);
        SetSuicide(SpriteNum);
        return 0;
    }

    if (u->tgt_sp && User[u->tgt_sp-sprite] && TEST(User[u->tgt_sp-sprite]->Flags, PF_DEAD))    // JBF: added User[] null check
        DoActorPickClosePlayer(SpriteNum);

    // jumping and falling
    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (TEST(u->Flags, SPR_JUMPING))
            DoActorJump(SpriteNum);
        else if (TEST(u->Flags, SPR_FALLING))
            DoActorFall(SpriteNum);
    }

    // sliding
    if (TEST(u->Flags, SPR_SLIDING))
        DoActorSlide(SpriteNum);

    // Do track or call current action function - such as DoActorMoveCloser()
    if (u->track >= 0)
        ActorFollowTrack(SpriteNum, ACTORMOVETICS);
    else
    {
        (*u->ActorActionFunc)(SpriteNum);
    }

    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        KeepActorOnFloor(SpriteNum);
    }

    // take damage from environment
    DoActorSectorDamage(SpriteNum);

    return 0;
}

int
NullZombie(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (u->Counter3++ >= ZOMBIE_TIME_LIMIT)
    {
        InitBloodSpray(SpriteNum,TRUE,105);
        InitBloodSpray(SpriteNum,TRUE,105);
        InitBloodSpray(SpriteNum,TRUE,105);
        SetSuicide(SpriteNum);
        return 0;
    }

    if (u->tgt_sp && User[u->tgt_sp-sprite] && TEST(User[u->tgt_sp-sprite]->Flags, PF_DEAD))
        DoActorPickClosePlayer(SpriteNum);

    if (u->WaitTics > 0)
        u->WaitTics -= ACTORMOVETICS;

    if (TEST(u->Flags, SPR_SLIDING) && !TEST(u->Flags, SPR_JUMPING|SPR_FALLING))
        DoActorSlide(SpriteNum);

    if (!TEST(u->Flags, SPR_JUMPING|SPR_FALLING))
        KeepActorOnFloor(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    return 0;
}


int DoZombiePain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullZombie(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(SpriteNum);

    return 0;
}


#include "saveable.h"

static saveable_code saveable_zombie_code[] =
{
    SAVE_CODE(SetupZombie),
    SAVE_CODE(SpawnZombie),
    SAVE_CODE(SpawnZombie2),
    SAVE_CODE(DoZombieMove),
    SAVE_CODE(NullZombie),
    SAVE_CODE(DoZombiePain),
};

static saveable_data saveable_zombie_data[] =
{
    SAVE_DATA(ZombieBattle),
    SAVE_DATA(ZombieOffense),
    SAVE_DATA(ZombieBroadcast),
    SAVE_DATA(ZombieSurprised),
    SAVE_DATA(ZombieEvasive),
    SAVE_DATA(ZombieLostTarget),
    SAVE_DATA(ZombieCloseRange),

    SAVE_DATA(ZombiePersonality),

    SAVE_DATA(ZombieAttrib),

    SAVE_DATA(s_ZombieRun),
    SAVE_DATA(sg_ZombieRun),
    SAVE_DATA(s_ZombieStand),
    SAVE_DATA(sg_ZombieStand),
    SAVE_DATA(s_ZombiePain),
    SAVE_DATA(sg_ZombiePain),
    SAVE_DATA(s_ZombieNuke),
    SAVE_DATA(sg_ZombieNuke),
    SAVE_DATA(s_ZombieRocket),
    SAVE_DATA(sg_ZombieRocket),
    SAVE_DATA(s_ZombieRail),
    SAVE_DATA(sg_ZombieRail),
    SAVE_DATA(s_ZombieGrenade),
    SAVE_DATA(sg_ZombieGrenade),
    SAVE_DATA(s_ZombieFlashBomb),
    SAVE_DATA(sg_ZombieFlashBomb),
    SAVE_DATA(s_ZombieUzi),
    SAVE_DATA(sg_ZombieUzi),
    SAVE_DATA(s_ZombieFall),
    SAVE_DATA(sg_ZombieFall),

    SAVE_DATA(ZombieActionSet),
};

saveable_module saveable_zombie =
{
    // code
    saveable_zombie_code,
    SIZ(saveable_zombie_code),

    // data
    saveable_zombie_data,
    SIZ(saveable_zombie_data)
};
