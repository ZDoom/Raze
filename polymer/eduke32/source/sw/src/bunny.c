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
#include "sprite.h"
#include "actor.h"
#include "weapon.h"
#include "track.h"

short Bunny_Count = 0;
ANIMATOR DoActorMoveJump;
ANIMATOR DoBunnyMoveJump;
ANIMATOR DoBunnyQuickJump;

DECISION BunnyBattle[] =
{
    {748, InitActorMoveCloser},
    {750, InitActorAlertNoise},
    {760, InitActorAttackNoise},
    {1024, InitActorMoveCloser}
};

DECISION BunnyOffense[] =
{
    {600, InitActorMoveCloser},
    {700, InitActorAlertNoise},
    {1024, InitActorMoveCloser}
};

DECISION BunnyBroadcast[] =
{
    {21, InitActorAlertNoise},
    {51, InitActorAmbientNoise},
    {1024, InitActorDecide}
};

DECISION BunnySurprised[] =
{
    {500, InitActorRunAway},
    {701, InitActorMoveCloser},
    {1024, InitActorDecide}
};

DECISION BunnyEvasive[] =
{
    {500,  InitActorWanderAround},
    {1020, InitActorRunAway},
    {1024, InitActorAmbientNoise}
};

DECISION BunnyLostTarget[] =
{
    {900, InitActorFindPlayer},
    {1024, InitActorWanderAround}
};

DECISION BunnyCloseRange[] =
{
    {1024,  InitActorAttack             },
//    {1024,  InitActorReposition         }
};

DECISION BunnyWander[] =
{
    {1024, InitActorReposition}
};

PERSONALITY WhiteBunnyPersonality =
{
    BunnyBattle,
    BunnyOffense,
    BunnyBroadcast,
    BunnySurprised,
    BunnyEvasive,
    BunnyLostTarget,
    BunnyCloseRange,
    BunnyCloseRange
};

PERSONALITY BunnyPersonality =
{
    BunnyEvasive,
    BunnyEvasive,
    BunnyEvasive,
    BunnyWander,
    BunnyWander,
    BunnyWander,
    BunnyEvasive,
    BunnyEvasive
};

ATTRIBUTE BunnyAttrib =
{
    {100, 120, 140, 180},               // Speeds
    {5, 0, -2, -4},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_BUNNYAMBIENT, 0, DIGI_BUNNYATTACK,
        DIGI_BUNNYATTACK, DIGI_BUNNYDIE2, 0,
        0,0,0,0
    }
};

ATTRIBUTE WhiteBunnyAttrib =
{
    {200, 220, 340, 380},               // Speeds
    {5, 0, -2, -4},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_BUNNYAMBIENT, 0, DIGI_BUNNYATTACK,
        DIGI_BUNNYATTACK, DIGI_BUNNYDIE2, 0,
        0,0,0,0
    }
};

//////////////////////
//
// BUNNY RUN
//
//////////////////////

#define BUNNY_RUN_RATE 10

ANIMATOR DoBunnyMove, NullBunny, DoActorDebris, DoBunnyGrowUp;

STATE s_BunnyRun[5][6] =
{
    {
        {BUNNY_RUN_R0 + 0, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[0][1]},
        {BUNNY_RUN_R0 + 1, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[0][2]},
        {BUNNY_RUN_R0 + 2, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[0][3]},
        {BUNNY_RUN_R0 + 3, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[0][4]},
        {BUNNY_RUN_R0 + 4, SF_QUICK_CALL,                DoBunnyGrowUp, &s_BunnyRun[0][5]},
        {BUNNY_RUN_R0 + 4, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[0][0]},
    },
    {
        {BUNNY_RUN_R1 + 0, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[1][1]},
        {BUNNY_RUN_R1 + 1, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[1][2]},
        {BUNNY_RUN_R1 + 2, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[1][3]},
        {BUNNY_RUN_R1 + 3, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[1][4]},
        {BUNNY_RUN_R1 + 4, SF_QUICK_CALL,                DoBunnyGrowUp, &s_BunnyRun[1][5]},
        {BUNNY_RUN_R1 + 4, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[1][0]},
    },
    {
        {BUNNY_RUN_R2 + 0, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[2][1]},
        {BUNNY_RUN_R2 + 1, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[2][2]},
        {BUNNY_RUN_R2 + 2, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[2][3]},
        {BUNNY_RUN_R2 + 3, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[2][4]},
        {BUNNY_RUN_R2 + 4, SF_QUICK_CALL,                DoBunnyGrowUp, &s_BunnyRun[2][5]},
        {BUNNY_RUN_R2 + 4, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[2][0]},
    },
    {
        {BUNNY_RUN_R3 + 0, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[3][1]},
        {BUNNY_RUN_R3 + 1, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[3][2]},
        {BUNNY_RUN_R3 + 2, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[3][3]},
        {BUNNY_RUN_R3 + 3, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[3][4]},
        {BUNNY_RUN_R3 + 4, SF_QUICK_CALL,                DoBunnyGrowUp, &s_BunnyRun[3][5]},
        {BUNNY_RUN_R3 + 4, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[3][0]},
    },
    {
        {BUNNY_RUN_R4 + 0, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[4][1]},
        {BUNNY_RUN_R4 + 1, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[4][2]},
        {BUNNY_RUN_R4 + 2, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[4][3]},
        {BUNNY_RUN_R4 + 3, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[4][4]},
        {BUNNY_RUN_R4 + 4, SF_QUICK_CALL,                DoBunnyGrowUp, &s_BunnyRun[4][5]},
        {BUNNY_RUN_R4 + 4, BUNNY_RUN_RATE | SF_TIC_ADJUST, DoBunnyMove, &s_BunnyRun[4][0]},
    }
};


STATEp sg_BunnyRun[] =
{
    &s_BunnyRun[0][0],
    &s_BunnyRun[1][0],
    &s_BunnyRun[2][0],
    &s_BunnyRun[3][0],
    &s_BunnyRun[4][0]
};

//////////////////////
//
// BUNNY STAND
//
//////////////////////

#define BUNNY_STAND_RATE 12
ANIMATOR DoBunnyEat;

STATE s_BunnyStand[5][3] =
{
    {
        {BUNNY_STAND_R0 + 0, BUNNY_STAND_RATE, DoBunnyEat, &s_BunnyStand[0][1]},
        {BUNNY_STAND_R0 + 4, SF_QUICK_CALL, DoBunnyGrowUp, &s_BunnyStand[0][2]},
        {BUNNY_STAND_R0 + 4, BUNNY_STAND_RATE, DoBunnyEat, &s_BunnyStand[0][0]},
    },
    {
        {BUNNY_STAND_R1 + 0, BUNNY_STAND_RATE, DoBunnyEat, &s_BunnyStand[1][1]},
        {BUNNY_STAND_R1 + 4, SF_QUICK_CALL, DoBunnyGrowUp, &s_BunnyStand[1][2]},
        {BUNNY_STAND_R1 + 4, BUNNY_STAND_RATE, DoBunnyEat, &s_BunnyStand[1][0]},
    },
    {
        {BUNNY_STAND_R2 + 0, BUNNY_STAND_RATE, DoBunnyEat, &s_BunnyStand[2][1]},
        {BUNNY_STAND_R2 + 4, SF_QUICK_CALL, DoBunnyGrowUp, &s_BunnyStand[2][2]},
        {BUNNY_STAND_R2 + 4, BUNNY_STAND_RATE, DoBunnyEat, &s_BunnyStand[2][0]},
    },
    {
        {BUNNY_STAND_R3 + 0, BUNNY_STAND_RATE, DoBunnyEat, &s_BunnyStand[3][1]},
        {BUNNY_STAND_R3 + 4, SF_QUICK_CALL, DoBunnyGrowUp, &s_BunnyStand[3][2]},
        {BUNNY_STAND_R3 + 4, BUNNY_STAND_RATE, DoBunnyEat, &s_BunnyStand[3][0]},
    },
    {
        {BUNNY_STAND_R4 + 0, BUNNY_STAND_RATE, DoBunnyEat, &s_BunnyStand[4][1]},
        {BUNNY_STAND_R4 + 4, SF_QUICK_CALL, DoBunnyGrowUp, &s_BunnyStand[4][2]},
        {BUNNY_STAND_R4 + 4, BUNNY_STAND_RATE, DoBunnyEat, &s_BunnyStand[4][0]},
    },
};


STATEp sg_BunnyStand[] =
{
    s_BunnyStand[0],
    s_BunnyStand[1],
    s_BunnyStand[2],
    s_BunnyStand[3],
    s_BunnyStand[4]
};

//////////////////////
//
// BUNNY GET LAYED
//
//////////////////////

#define BUNNY_SCREW_RATE 16
ANIMATOR DoBunnyScrew;

STATE s_BunnyScrew[5][2] =
{
    {
        {BUNNY_STAND_R0 + 0, BUNNY_SCREW_RATE, DoBunnyScrew, &s_BunnyScrew[0][1]},
        {BUNNY_STAND_R0 + 2, BUNNY_SCREW_RATE, DoBunnyScrew, &s_BunnyScrew[0][0]},
    },
    {
        {BUNNY_STAND_R1 + 0, BUNNY_SCREW_RATE, DoBunnyScrew, &s_BunnyScrew[1][1]},
        {BUNNY_STAND_R1 + 2, BUNNY_SCREW_RATE, DoBunnyScrew, &s_BunnyScrew[1][0]},
    },
    {
        {BUNNY_STAND_R2 + 0, BUNNY_SCREW_RATE, DoBunnyScrew, &s_BunnyScrew[2][1]},
        {BUNNY_STAND_R2 + 2, BUNNY_SCREW_RATE, DoBunnyScrew, &s_BunnyScrew[2][0]},
    },
    {
        {BUNNY_STAND_R3 + 0, BUNNY_SCREW_RATE, DoBunnyScrew, &s_BunnyScrew[3][1]},
        {BUNNY_STAND_R3 + 2, BUNNY_SCREW_RATE, DoBunnyScrew, &s_BunnyScrew[3][0]},
    },
    {
        {BUNNY_STAND_R4 + 0, BUNNY_SCREW_RATE, DoBunnyScrew, &s_BunnyScrew[4][1]},
        {BUNNY_STAND_R4 + 2, BUNNY_SCREW_RATE, DoBunnyScrew, &s_BunnyScrew[4][0]},
    },
};


STATEp sg_BunnyScrew[] =
{
    s_BunnyScrew[0],
    s_BunnyScrew[1],
    s_BunnyScrew[2],
    s_BunnyScrew[3],
    s_BunnyScrew[4]
};

//////////////////////
//
// BUNNY SWIPE
//
//////////////////////

#define BUNNY_SWIPE_RATE 8
ANIMATOR InitActorDecide;
ANIMATOR InitBunnySlash;

STATE s_BunnySwipe[5][8] =
{
    {
        {BUNNY_SWIPE_R0 + 0, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[0][1]},
        {BUNNY_SWIPE_R0 + 1, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[0][2]},
        {BUNNY_SWIPE_R0 + 1, 0 | SF_QUICK_CALL, InitBunnySlash, &s_BunnySwipe[0][3]},
        {BUNNY_SWIPE_R0 + 2, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[0][4]},
        {BUNNY_SWIPE_R0 + 3, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[0][5]},
        {BUNNY_SWIPE_R0 + 3, 0 | SF_QUICK_CALL, InitBunnySlash, &s_BunnySwipe[0][6]},
        {BUNNY_SWIPE_R0 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_BunnySwipe[0][7]},
        {BUNNY_SWIPE_R0 + 3, BUNNY_SWIPE_RATE, DoBunnyMove, &s_BunnySwipe[0][7]},
    },
    {
        {BUNNY_SWIPE_R1 + 0, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[1][1]},
        {BUNNY_SWIPE_R1 + 1, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[1][2]},
        {BUNNY_SWIPE_R1 + 1, 0 | SF_QUICK_CALL, InitBunnySlash, &s_BunnySwipe[1][3]},
        {BUNNY_SWIPE_R1 + 2, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[1][4]},
        {BUNNY_SWIPE_R1 + 3, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[1][5]},
        {BUNNY_SWIPE_R1 + 3, 0 | SF_QUICK_CALL, InitBunnySlash, &s_BunnySwipe[1][6]},
        {BUNNY_SWIPE_R1 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_BunnySwipe[1][7]},
        {BUNNY_SWIPE_R1 + 3, BUNNY_SWIPE_RATE, DoBunnyMove, &s_BunnySwipe[1][7]},
    },
    {
        {BUNNY_SWIPE_R2 + 0, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[2][1]},
        {BUNNY_SWIPE_R2 + 1, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[2][2]},
        {BUNNY_SWIPE_R2 + 1, 0 | SF_QUICK_CALL, InitBunnySlash, &s_BunnySwipe[2][3]},
        {BUNNY_SWIPE_R2 + 2, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[2][4]},
        {BUNNY_SWIPE_R2 + 3, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[2][5]},
        {BUNNY_SWIPE_R2 + 3, 0 | SF_QUICK_CALL, InitBunnySlash, &s_BunnySwipe[2][6]},
        {BUNNY_SWIPE_R2 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_BunnySwipe[2][7]},
        {BUNNY_SWIPE_R2 + 3, BUNNY_SWIPE_RATE, DoBunnyMove, &s_BunnySwipe[2][7]},
    },
    {
        {BUNNY_SWIPE_R3 + 0, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[3][1]},
        {BUNNY_SWIPE_R3 + 1, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[3][2]},
        {BUNNY_SWIPE_R3 + 1, 0 | SF_QUICK_CALL, InitBunnySlash, &s_BunnySwipe[3][3]},
        {BUNNY_SWIPE_R3 + 2, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[3][4]},
        {BUNNY_SWIPE_R3 + 3, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[3][5]},
        {BUNNY_SWIPE_R3 + 3, 0 | SF_QUICK_CALL, InitBunnySlash, &s_BunnySwipe[3][6]},
        {BUNNY_SWIPE_R3 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_BunnySwipe[3][7]},
        {BUNNY_SWIPE_R3 + 3, BUNNY_SWIPE_RATE, DoBunnyMove, &s_BunnySwipe[3][7]},
    },
    {
        {BUNNY_SWIPE_R4 + 0, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[4][1]},
        {BUNNY_SWIPE_R4 + 1, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[4][2]},
        {BUNNY_SWIPE_R4 + 1, 0 | SF_QUICK_CALL, InitBunnySlash, &s_BunnySwipe[4][3]},
        {BUNNY_SWIPE_R4 + 2, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[4][4]},
        {BUNNY_SWIPE_R4 + 3, BUNNY_SWIPE_RATE, NullBunny, &s_BunnySwipe[4][5]},
        {BUNNY_SWIPE_R4 + 3, 0 | SF_QUICK_CALL, InitBunnySlash, &s_BunnySwipe[4][6]},
        {BUNNY_SWIPE_R4 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_BunnySwipe[4][7]},
        {BUNNY_SWIPE_R4 + 3, BUNNY_SWIPE_RATE, DoBunnyMove, &s_BunnySwipe[4][7]},
    }
};


STATEp sg_BunnySwipe[] =
{
    &s_BunnySwipe[0][0],
    &s_BunnySwipe[1][0],
    &s_BunnySwipe[2][0],
    &s_BunnySwipe[3][0],
    &s_BunnySwipe[4][0]
};


//////////////////////
//
// BUNNY HEART - show players heart
//
//////////////////////

#define BUNNY_HEART_RATE 14
ANIMATOR DoBunnyStandKill;

STATE s_BunnyHeart[5][4] =
{
    {
        {BUNNY_SWIPE_R0 + 0, BUNNY_HEART_RATE, DoBunnyStandKill, &s_BunnyHeart[0][0]},
    },
    {
        {BUNNY_SWIPE_R1 + 0, BUNNY_HEART_RATE, DoBunnyStandKill, &s_BunnyHeart[1][0]},
    },
    {
        {BUNNY_SWIPE_R2 + 0, BUNNY_HEART_RATE, DoBunnyStandKill, &s_BunnyHeart[2][0]},
    },
    {
        {BUNNY_SWIPE_R3 + 0, BUNNY_HEART_RATE, DoBunnyStandKill, &s_BunnyHeart[3][0]},
    },
    {
        {BUNNY_SWIPE_R4 + 0, BUNNY_HEART_RATE, DoBunnyStandKill, &s_BunnyHeart[4][0]},
    }
};


STATEp sg_BunnyHeart[] =
{
    &s_BunnyHeart[0][0],
    &s_BunnyHeart[1][0],
    &s_BunnyHeart[2][0],
    &s_BunnyHeart[3][0],
    &s_BunnyHeart[4][0]
};

//////////////////////
//
// BUNNY PAIN
//
//////////////////////

#define BUNNY_PAIN_RATE 38
ANIMATOR DoBunnyPain;

STATE s_BunnyPain[5][1] =
{
    {
        {BUNNY_SWIPE_R0 + 0, BUNNY_PAIN_RATE, DoBunnyPain, &s_BunnyPain[0][0]},
    },
    {
        {BUNNY_SWIPE_R0 + 0, BUNNY_PAIN_RATE, DoBunnyPain, &s_BunnyPain[1][0]},
    },
    {
        {BUNNY_SWIPE_R0 + 0, BUNNY_PAIN_RATE, DoBunnyPain, &s_BunnyPain[2][0]},
    },
    {
        {BUNNY_SWIPE_R0 + 0, BUNNY_PAIN_RATE, DoBunnyPain, &s_BunnyPain[3][0]},
    },
    {
        {BUNNY_SWIPE_R0 + 0, BUNNY_PAIN_RATE, DoBunnyPain, &s_BunnyPain[4][0]},
    }
};

STATEp sg_BunnyPain[] =
{
    &s_BunnyPain[0][0],
    &s_BunnyPain[1][0],
    &s_BunnyPain[2][0],
    &s_BunnyPain[3][0],
    &s_BunnyPain[4][0]
};

//////////////////////
//
// BUNNY JUMP
//
//////////////////////

#define BUNNY_JUMP_RATE 25

STATE s_BunnyJump[5][6] =
{
    {
        {BUNNY_RUN_R0 + 1, BUNNY_JUMP_RATE, DoBunnyMoveJump, &s_BunnyJump[0][1]},
        {BUNNY_RUN_R0 + 2, BUNNY_JUMP_RATE, DoBunnyMoveJump, &s_BunnyJump[0][1]},
    },
    {
        {BUNNY_RUN_R1 + 1, BUNNY_JUMP_RATE, DoBunnyMoveJump, &s_BunnyJump[1][1]},
        {BUNNY_RUN_R1 + 2, BUNNY_JUMP_RATE, DoBunnyMoveJump, &s_BunnyJump[1][1]},
    },
    {
        {BUNNY_RUN_R2 + 1, BUNNY_JUMP_RATE, DoBunnyMoveJump, &s_BunnyJump[2][1]},
        {BUNNY_RUN_R2 + 2, BUNNY_JUMP_RATE, DoBunnyMoveJump, &s_BunnyJump[2][1]},
    },
    {
        {BUNNY_RUN_R3 + 1, BUNNY_JUMP_RATE, DoBunnyMoveJump, &s_BunnyJump[3][1]},
        {BUNNY_RUN_R3 + 2, BUNNY_JUMP_RATE, DoBunnyMoveJump, &s_BunnyJump[3][1]},
    },
    {
        {BUNNY_RUN_R4 + 1, BUNNY_JUMP_RATE, DoBunnyMoveJump, &s_BunnyJump[4][1]},
        {BUNNY_RUN_R4 + 2, BUNNY_JUMP_RATE, DoBunnyMoveJump, &s_BunnyJump[4][1]},
    }
};


STATEp sg_BunnyJump[] =
{
    &s_BunnyJump[0][0],
    &s_BunnyJump[1][0],
    &s_BunnyJump[2][0],
    &s_BunnyJump[3][0],
    &s_BunnyJump[4][0]
};


//////////////////////
//
// BUNNY FALL
//
//////////////////////

#define BUNNY_FALL_RATE 25

STATE s_BunnyFall[5][6] =
{
    {
        {BUNNY_RUN_R0 + 3, BUNNY_FALL_RATE, DoBunnyMoveJump, &s_BunnyFall[0][0]},
    },
    {
        {BUNNY_RUN_R1 + 3, BUNNY_FALL_RATE, DoBunnyMoveJump, &s_BunnyFall[1][0]},
    },
    {
        {BUNNY_RUN_R2 + 3, BUNNY_FALL_RATE, DoBunnyMoveJump, &s_BunnyFall[2][0]},
    },
    {
        {BUNNY_RUN_R3 + 3, BUNNY_FALL_RATE, DoBunnyMoveJump, &s_BunnyFall[3][0]},
    },
    {
        {BUNNY_RUN_R4 + 3, BUNNY_FALL_RATE, DoBunnyMoveJump, &s_BunnyFall[4][0]},
    }
};


STATEp sg_BunnyFall[] =
{
    &s_BunnyFall[0][0],
    &s_BunnyFall[1][0],
    &s_BunnyFall[2][0],
    &s_BunnyFall[3][0],
    &s_BunnyFall[4][0]
};


//////////////////////
//
// BUNNY JUMP ATTACK
//
//////////////////////

#define BUNNY_JUMP_ATTACK_RATE 35
int DoBunnyBeginJumpAttack(short SpriteNum);

STATE s_BunnyJumpAttack[5][6] =
{
    {
        {BUNNY_RUN_R0 + 1, BUNNY_JUMP_ATTACK_RATE, NullBunny, &s_BunnyJumpAttack[0][1]},
        {BUNNY_RUN_R0 + 1, 0 | SF_QUICK_CALL, DoBunnyBeginJumpAttack, &s_BunnyJumpAttack[0][2]},
        {BUNNY_RUN_R0 + 2, BUNNY_JUMP_ATTACK_RATE, DoBunnyMoveJump, &s_BunnyJumpAttack[0][2]},
    },
    {
        {BUNNY_RUN_R1 + 1, BUNNY_JUMP_ATTACK_RATE, NullBunny, &s_BunnyJumpAttack[1][1]},
        {BUNNY_RUN_R1 + 1, 0 | SF_QUICK_CALL, DoBunnyBeginJumpAttack, &s_BunnyJumpAttack[1][2]},
        {BUNNY_RUN_R1 + 2, BUNNY_JUMP_ATTACK_RATE, DoBunnyMoveJump, &s_BunnyJumpAttack[1][2]},
    },
    {
        {BUNNY_RUN_R2 + 1, BUNNY_JUMP_ATTACK_RATE, NullBunny, &s_BunnyJumpAttack[2][1]},
        {BUNNY_RUN_R2 + 1, 0 | SF_QUICK_CALL, DoBunnyBeginJumpAttack, &s_BunnyJumpAttack[2][2]},
        {BUNNY_RUN_R2 + 2, BUNNY_JUMP_ATTACK_RATE, DoBunnyMoveJump, &s_BunnyJumpAttack[2][2]},
    },
    {
        {BUNNY_RUN_R3 + 1, BUNNY_JUMP_ATTACK_RATE, NullBunny, &s_BunnyJumpAttack[3][1]},
        {BUNNY_RUN_R3 + 1, 0 | SF_QUICK_CALL, DoBunnyBeginJumpAttack, &s_BunnyJumpAttack[3][2]},
        {BUNNY_RUN_R3 + 2, BUNNY_JUMP_ATTACK_RATE, DoBunnyMoveJump, &s_BunnyJumpAttack[3][2]},
    },
    {
        {BUNNY_RUN_R4 + 1, BUNNY_JUMP_ATTACK_RATE, NullBunny, &s_BunnyJumpAttack[4][1]},
        {BUNNY_RUN_R4 + 1, 0 | SF_QUICK_CALL, DoBunnyBeginJumpAttack, &s_BunnyJumpAttack[4][2]},
        {BUNNY_RUN_R4 + 2, BUNNY_JUMP_ATTACK_RATE, DoBunnyMoveJump, &s_BunnyJumpAttack[4][2]},
    }
};


STATEp sg_BunnyJumpAttack[] =
{
    &s_BunnyJumpAttack[0][0],
    &s_BunnyJumpAttack[1][0],
    &s_BunnyJumpAttack[2][0],
    &s_BunnyJumpAttack[3][0],
    &s_BunnyJumpAttack[4][0]
};


//////////////////////
//
// BUNNY DIE
//
//////////////////////

#define BUNNY_DIE_RATE 16
ANIMATOR BunnySpew;

STATE s_BunnyDie[] =
{
    {BUNNY_DIE + 0, BUNNY_DIE_RATE, NullBunny, &s_BunnyDie[1]},
    {BUNNY_DIE + 0, SF_QUICK_CALL,  BunnySpew, &s_BunnyDie[2]},
    {BUNNY_DIE + 1, BUNNY_DIE_RATE, NullBunny, &s_BunnyDie[3]},
    {BUNNY_DIE + 2, BUNNY_DIE_RATE, NullBunny, &s_BunnyDie[4]},
    {BUNNY_DIE + 2, BUNNY_DIE_RATE, NullBunny, &s_BunnyDie[5]},
    {BUNNY_DEAD, BUNNY_DIE_RATE, DoActorDebris, &s_BunnyDie[5]},
};

#define BUNNY_DEAD_RATE 8

STATE s_BunnyDead[] =
{
    {BUNNY_DIE + 0, BUNNY_DEAD_RATE, NullAnimator, &s_BunnyDie[1]},
    {BUNNY_DIE + 0, SF_QUICK_CALL,  BunnySpew, &s_BunnyDie[2]},
    {BUNNY_DIE + 1, BUNNY_DEAD_RATE, NullAnimator, &s_BunnyDead[3]},
    {BUNNY_DIE + 2, BUNNY_DEAD_RATE, NullAnimator, &s_BunnyDead[4]},
    {BUNNY_DEAD, SF_QUICK_CALL, QueueFloorBlood, &s_BunnyDead[5]},
    {BUNNY_DEAD, BUNNY_DEAD_RATE, DoActorDebris, &s_BunnyDead[5]},
};

STATEp sg_BunnyDie[] =
{
    s_BunnyDie
};

STATEp sg_BunnyDead[] =
{
    s_BunnyDead
};

STATE s_BunnyDeathJump[] =
{
    {BUNNY_DIE + 0, BUNNY_DIE_RATE, DoActorDeathMove, &s_BunnyDeathJump[0]}
};

STATE s_BunnyDeathFall[] =
{
    {BUNNY_DIE + 1, BUNNY_DIE_RATE, DoActorDeathMove, &s_BunnyDeathFall[0]}
};

STATEp sg_BunnyDeathJump[] =
{
    s_BunnyDeathJump
};

STATEp sg_BunnyDeathFall[] =
{
    s_BunnyDeathFall
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

ACTOR_ACTION_SET BunnyActionSet =
{
    sg_BunnyStand,
    sg_BunnyRun,
    sg_BunnyJump,
    sg_BunnyFall,
    NULL,                               // sg_BunnyCrawl,
    NULL,                               // sg_BunnySwim,
    NULL,                               // sg_BunnyFly,
    NULL,                               // sg_BunnyRise,
    NULL,                               // sg_BunnySit,
    NULL,                               // sg_BunnyLook,
    NULL,                               // climb
    sg_BunnyPain,
    sg_BunnyDie,
    NULL,
    sg_BunnyDead,
    sg_BunnyDeathJump,
    sg_BunnyDeathFall,
    {NULL},
    {1024},
    {NULL},
    {1024},
    {sg_BunnyHeart, sg_BunnyRun},
    NULL,
    NULL
};

ACTOR_ACTION_SET BunnyWhiteActionSet =
{
    sg_BunnyStand,
    sg_BunnyRun,
    sg_BunnyJump,
    sg_BunnyFall,
    NULL,                               // sg_BunnyCrawl,
    NULL,                               // sg_BunnySwim,
    NULL,                               // sg_BunnyFly,
    NULL,                               // sg_BunnyRise,
    NULL,                               // sg_BunnySit,
    NULL,                               // sg_BunnyLook,
    NULL,                               // climb
    sg_BunnyPain,                       // pain
    sg_BunnyDie,
    NULL,
    sg_BunnyDead,
    sg_BunnyDeathJump,
    sg_BunnyDeathFall,
    {sg_BunnySwipe},
    {1024},
//    {sg_BunnyJumpAttack, sg_BunnySwipe},
//    {800, 1024},
    {sg_BunnySwipe},
    {1024},
    {sg_BunnyHeart, sg_BunnySwipe},
    NULL,
    NULL
};

int
SetupBunny(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u;
    ANIMATOR DoActorDecide;

    if (TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
    {
        u = User[SpriteNum];
        ASSERT(u);
    }
    else
    {
        User[SpriteNum] = u = SpawnUser(SpriteNum, BUNNY_RUN_R0, s_BunnyRun[0]);
        u->Health = 10;
    }

    Bunny_Count++;
    //if(Bunny_Count > 20)
    //    {
    //    KillSprite(SpriteNum);
    //    Bunny_Count--;
    //    return(0);
    //    }

    ChangeState(SpriteNum, s_BunnyRun[0]);
    u->StateEnd = s_BunnyDie;
    u->Rot = sg_BunnyRun;
    //sp->xrepeat = 64;
    //sp->yrepeat = 64;
    u->ShellNum = 0; // Not Pregnant right now
    u->FlagOwner = 0;

    sp->clipdist = (150) >> 2;

    if (sp->pal == PALETTE_PLAYER1)
    {
        EnemyDefaults(SpriteNum, &BunnyWhiteActionSet, &WhiteBunnyPersonality);
        u->Attrib = &WhiteBunnyAttrib;
        sp->xrepeat = 96;
        sp->yrepeat = 90;

        sp->clipdist = 200>>2;

        if (!TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
            u->Health = 60;
    }
    else if (sp->pal == PALETTE_PLAYER8) // Male Rabbit
    {
        EnemyDefaults(SpriteNum, &BunnyActionSet, &BunnyPersonality);
        u->Attrib = &BunnyAttrib;
        //sp->xrepeat = 76;
        //sp->yrepeat = 70;

        //sp->shade = 0; // darker
        if (!TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
            u->Health = 20;
        u->Flag1 = 0;
    }
    else
    {
        // Female Rabbit
        EnemyDefaults(SpriteNum, &BunnyActionSet, &BunnyPersonality);
        u->Attrib = &BunnyAttrib;
        u->spal = sp->pal = PALETTE_PLAYER0;
        u->Flag1 = SEC(5);
        //sp->shade = 0; // darker
    }

    DoActorSetSpeed(SpriteNum, FAST_SPEED);

    SET(u->Flags, SPR_XFLIP_TOGGLE);


    u->zclip = Z(16);
    u->floor_dist = Z(8);
    u->ceiling_dist = Z(8);
    u->lo_step = Z(16);

    return 0;
}

int
GetBunnyJumpHeight(short jump_speed, short jump_grav)
{
    int jump_iterations;
    int height;

    jump_speed = labs(jump_speed);

    jump_iterations = jump_speed / (jump_grav * ACTORMOVETICS);

    height = jump_speed * jump_iterations * ACTORMOVETICS;

    height = DIV256(height);

    return DIV2(height);
}

int
PickBunnyJumpSpeed(short SpriteNum, int pix_height)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    ASSERT(pix_height < 128);

    u->jump_speed = -600;
    u->jump_grav = 8;

    while (TRUE)
    {
        if (GetBunnyJumpHeight(u->jump_speed, u->jump_grav) > pix_height + 20)
            break;

        u->jump_speed -= 100;

        ASSERT(u->jump_speed > -3000);
    }

    return u->jump_speed;
}

//
// JUMP ATTACK
//

int
DoBunnyBeginJumpAttack(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp psp = User[SpriteNum]->tgt_sp;
    int dist;
    int CanSeePlayer(short SpriteNum);
    short tang;


#define RANDOM_NEG(x) (RANDOM_P2((x)<<1) - (x))

    tang = getangle(psp->x - sp->x, psp->y - sp->y);

    if (move_sprite(SpriteNum, sintable[NORM_ANGLE(tang+512)] >> 7, sintable[tang] >> 7,
                    0L, u->ceiling_dist, u->floor_dist, CLIPMASK_ACTOR, ACTORMOVETICS))
        sp->ang = NORM_ANGLE(sp->ang + 1024) + (RANDOM_NEG(256 << 6) >> 6);
    else
        sp->ang = NORM_ANGLE(tang + (RANDOM_NEG(256 << 6) >> 6));

    DoActorSetSpeed(SpriteNum, FAST_SPEED);

    //u->jump_speed = -800;
    PickJumpMaxSpeed(SpriteNum, -400); // was -800

    SET(u->Flags, SPR_JUMPING);
    RESET(u->Flags, SPR_FALLING);

    // set up individual actor jump gravity
    u->jump_grav = 17; // was 8

    // if I didn't do this here they get stuck in the air sometimes
    DoActorZrange(SpriteNum);

    DoJump(SpriteNum);

    return 0;
}

int
DoBunnyMoveJump(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        int nx, ny;

        // Move while jumping
        nx = sp->xvel * (int) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
        ny = sp->xvel * (int) sintable[sp->ang] >> 14;

        move_actor(SpriteNum, nx, ny, 0L);

        if (TEST(u->Flags, SPR_JUMPING))
            DoActorJump(SpriteNum);
        else
            DoActorFall(SpriteNum);
    }

    DoActorZrange(SpriteNum);

    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
//        if (DoBunnyQuickJump(SpriteNum))
//            return (0);

        InitActorDecide(SpriteNum);
    }

    return 0;
}

int
DoPickCloseBunny(short SpriteNum)
{
    USERp u = User[SpriteNum],tu;
    SPRITEp sp = &sprite[SpriteNum],tsp;
    int dist, near_dist = 1000, a,b,c;
    short i, nexti;
    //short BunnyCount=0, Bunny_Result = -1;

    // if actor can still see the player
    int look_height = SPRITEp_TOS(sp);
    SWBOOL ICanSee = FALSE;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], i, nexti)
    {
        tsp = &sprite[i];
        tu = User[i];

        if (sp == tsp) continue;

        if (tu->ID != BUNNY_RUN_R0) continue;

        DISTANCE(tsp->x, tsp->y, sp->x, sp->y, dist, a, b, c);

        if (dist > near_dist) continue;

        ICanSee = FAFcansee(sp->x, sp->y, look_height, sp->sectnum, tsp->x, tsp->y, SPRITEp_UPPER(tsp), tsp->sectnum);

        if (ICanSee && dist < near_dist && tu->ID == BUNNY_RUN_R0)
        {
            near_dist = dist;
            u->tgt_sp = u->lo_sp = tsp;
            //Bunny_Result = i;
            return i;
        }
    }
    return -1;
}

int
DoBunnyQuickJump(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (u->spal != PALETTE_PLAYER8) return FALSE;

    if (!u->lo_sp && u->spal == PALETTE_PLAYER8 && MoveSkip4)
        DoPickCloseBunny(SpriteNum);

    // Random Chance of like sexes fighting
    if (u->lo_sp)
    {
        short hit_sprite = u->lo_sp - sprite;
        SPRITEp tsp = u->lo_sp;
        USERp tu = User[hit_sprite];

        if (!tu || tu->ID != BUNNY_RUN_R0) return FALSE;


        // Not mature enough yet
        if (sp->xrepeat != 64 || sp->yrepeat != 64) return FALSE;
        if (tsp->xrepeat != 64 || tsp->yrepeat != 64) return FALSE;

        // Kill a rival
        // Only males fight
        if (tu->spal == sp->pal && RANDOM_RANGE(1000) > 995)
        {
            if (u->spal == PALETTE_PLAYER8 && tu->spal == PALETTE_PLAYER8)
            {
                PlaySound(DIGI_BUNNYATTACK, &sp->x, &sp->y, &sp->z, v3df_follow);
                PlaySound(DIGI_BUNNYDIE2, &tsp->x, &tsp->y, &tsp->z, v3df_follow);
                tu->Health = 0;

                // Blood fountains
                InitBloodSpray(hit_sprite,TRUE,-1);

                if (SpawnShrap(hit_sprite, SpriteNum))
                {
                    SetSuicide(hit_sprite);
                }
                else
                    DoActorDie(hit_sprite, SpriteNum);

                Bunny_Count--; // Bunny died

                u->lo_sp = NULL;
                return TRUE;
            }
        }
    }

    // Get layed!
    if (u->lo_sp && u->spal == PALETTE_PLAYER8) // Only males check this
    {
        short hit_sprite = u->lo_sp - sprite;
        SPRITEp tsp = u->lo_sp;
        USERp tu = User[hit_sprite];


        if (!tu || tu->ID != BUNNY_RUN_R0) return FALSE;

        // Not mature enough to mate yet
        if (sp->xrepeat != 64 || sp->yrepeat != 64) return FALSE;
        if (tsp->xrepeat != 64 || tsp->yrepeat != 64) return FALSE;

        if (tu->ShellNum <= 0 && tu->WaitTics <= 0 && u->WaitTics <= 0)
        {
            if (TEST(tsp->extra, SPRX_PLAYER_OR_ENEMY))
            {
                PLAYERp pp = NULL;

                if (RANDOM_RANGE(1000) < 995 && tu->spal != PALETTE_PLAYER0) return FALSE;

                DoActorPickClosePlayer(SpriteNum);

                if (User[u->tgt_sp-sprite]->PlayerP)
                    pp = User[u->tgt_sp-sprite]->PlayerP;

                if (tu->spal != PALETTE_PLAYER0)
                {
                    if (tu->Flag1 > 0) return FALSE;
                    tu->FlagOwner = 1; // FAG!
                    tu->Flag1 = SEC(10);
                    if (pp)
                    {
                        short choose_snd;
                        int fagsnds[] = {DIGI_FAGRABBIT1,DIGI_FAGRABBIT2,DIGI_FAGRABBIT3};

                        if (pp == Player+myconnectindex)
                        {
                            choose_snd = STD_RANDOM_RANGE(2<<8)>>8;
                            if (FAFcansee(sp->x,sp->y,SPRITEp_TOS(sp),sp->sectnum,pp->posx, pp->posy, pp->posz, pp->cursectnum) && FACING(sp, u->tgt_sp))
                                PlayerSound(fagsnds[choose_snd],&pp->posx,&pp->posy,&pp->posz,v3df_doppler|v3df_follow|v3df_dontpan,pp);
                        }
                    }
                }
                else
                {
                    if (pp && RANDOM_RANGE(1000) > 200)
                    {
                        short choose_snd;
                        int straightsnds[] = {DIGI_RABBITHUMP1,DIGI_RABBITHUMP2,
                                              DIGI_RABBITHUMP3,DIGI_RABBITHUMP4};

                        if (pp == Player+myconnectindex)
                        {
                            choose_snd = STD_RANDOM_RANGE(3<<8)>>8;
                            if (FAFcansee(sp->x,sp->y,SPRITEp_TOS(sp),sp->sectnum,pp->posx, pp->posy, pp->posz, pp->cursectnum) && FACING(sp, u->tgt_sp))
                                PlayerSound(straightsnds[choose_snd],&pp->posx,&pp->posy,&pp->posz,v3df_doppler|v3df_follow|v3df_dontpan,pp);
                        }
                    }
                }

                sp->x = tsp->x; // Mount up little bunny
                sp->y = tsp->y;
                sp->ang = tsp->ang;
                sp->ang = NORM_ANGLE(sp->ang + 1024);
                HelpMissileLateral(SpriteNum, 2000L);
                sp->ang = tsp->ang;
                u->Vis = sp->ang;  // Remember angles for later
                tu->Vis = tsp->ang;

                NewStateGroup(SpriteNum, sg_BunnyScrew);
                NewStateGroup(hit_sprite, sg_BunnyScrew);
                if (gs.ParentalLock || Global_PLock)
                {
                    SET(sp->cstat, CSTAT_SPRITE_INVISIBLE); // Turn em' invisible
                    SET(tsp->cstat, CSTAT_SPRITE_INVISIBLE); // Turn em' invisible
                }
                u->WaitTics = tu->WaitTics = SEC(10);  // Mate for this long
                return TRUE;
            }
        }
    }

    return FALSE;
}


int
NullBunny(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];


    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (TEST(u->Flags, SPR_JUMPING))
            DoActorJump(SpriteNum);
        else
            DoActorFall(SpriteNum);
    }

    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
        KeepActorOnFloor(SpriteNum);

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    return 0;
}


int DoBunnyPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullBunny(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(SpriteNum);
    return 0;
}

int DoBunnyRipHeart(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    SPRITEp tsp = u->tgt_sp;

    NewStateGroup(SpriteNum, sg_BunnyHeart);
    u->WaitTics = 6 * 120;

    // player face bunny
    tsp->ang = getangle(sp->x - tsp->x, sp->y - tsp->y);
    return 0;
}

int DoBunnyStandKill(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullBunny(SpriteNum);

    // Growl like the bad ass bunny you are!
    if (RANDOM_RANGE(1000) > 800)
        PlaySound(DIGI_BUNNYATTACK,&sp->x,&sp->y,&sp->z,v3df_none);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        NewStateGroup(SpriteNum, sg_BunnyRun);
    return 0;
}

void BunnyHatch(short Weapon)
{
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];

    short New,i;
    SPRITEp np;
    USERp nu;
#define MAX_BUNNYS 1
    short rip_ang[MAX_BUNNYS];

    rip_ang[0] = RANDOM_P2(2048);

    for (i = 0; i < MAX_BUNNYS; i++)
    {
        New = COVERinsertsprite(wp->sectnum, STAT_DEFAULT);
        np = &sprite[New];
        memset(np,0,sizeof(SPRITE));
        np->sectnum = wp->sectnum;
        np->statnum = STAT_DEFAULT;
        np->x = wp->x;
        np->y = wp->y;
        np->z = wp->z;
        np->owner = -1;
        np->xrepeat = 30;  // Baby size
        np->yrepeat = 24;
        np->ang = rip_ang[i];
        np->pal = 0;
        SetupBunny(New);
        nu = User[New];
        np->shade = wp->shade;

        // make immediately active
        SET(nu->Flags, SPR_ACTIVE);
        if (RANDOM_RANGE(1000) > 500) // Boy or Girl?
            nu->spal = np->pal = PALETTE_PLAYER0; // Girl
        else
        {
            nu->spal = np->pal = PALETTE_PLAYER8; // Boy
            // Oops, mommy died giving birth to a boy
            if (RANDOM_RANGE(1000) > 500)
            {
                wu->Health = 0;
                Bunny_Count--; // Bunny died

                // Blood fountains
                InitBloodSpray(Weapon,TRUE,-1);

                if (SpawnShrap(Weapon, New))
                {
                    SetSuicide(Weapon);
                }
                else
                    DoActorDie(Weapon, New);
            }
        }

        nu->ShellNum = 0; // Not Pregnant right now

        NewStateGroup(New, nu->ActorActionSet->Jump);
        nu->ActorActionFunc = DoActorMoveJump;
        DoActorSetSpeed(New, FAST_SPEED);
        PickJumpMaxSpeed(New, -600);

        SET(nu->Flags, SPR_JUMPING);
        RESET(nu->Flags, SPR_FALLING);

        nu->jump_grav = 8;

        // if I didn't do this here they get stuck in the air sometimes
        DoActorZrange(New);

        DoActorJump(New);
    }
}

int BunnyHatch2(short Weapon)
{
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];

    short New,i;
    SPRITEp np;
    USERp nu;

    New = COVERinsertsprite(wp->sectnum, STAT_DEFAULT);
    np = &sprite[New];
    memset(np,0,sizeof(SPRITE));
    np->sectnum = wp->sectnum;
    np->statnum = STAT_DEFAULT;
    np->x = wp->x;
    np->y = wp->y;
    np->z = wp->z;
    np->owner = -1;
    np->xrepeat = 30;  // Baby size
    np->yrepeat = 24;
    np->ang = RANDOM_P2(2048);
    np->pal = 0;
    SetupBunny(New);
    nu = User[New];
    np->shade = wp->shade;

    // make immediately active
    SET(nu->Flags, SPR_ACTIVE);
    if (RANDOM_RANGE(1000) > 500) // Boy or Girl?
    {
        nu->spal = np->pal = PALETTE_PLAYER0; // Girl
        nu->Flag1 = SEC(5);
    }
    else
    {
        nu->spal = np->pal = PALETTE_PLAYER8; // Boy
        nu->Flag1 = 0;
    }

    nu->ShellNum = 0; // Not Pregnant right now

    NewStateGroup(New, nu->ActorActionSet->Jump);
    nu->ActorActionFunc = DoActorMoveJump;
    DoActorSetSpeed(New, FAST_SPEED);
    if (TEST_BOOL3(wp))
    {
        PickJumpMaxSpeed(New, -600-RANDOM_RANGE(600));
        np->xrepeat = np->yrepeat = 64;
        np->xvel = 150 + RANDOM_RANGE(1000);
        nu->Health = 1; // Easy to pop. Like shootn' skeet.
        np->ang -= RANDOM_RANGE(128);
        np->ang += RANDOM_RANGE(128);
    }
    else
        PickJumpMaxSpeed(New, -600);

    SET(nu->Flags, SPR_JUMPING);
    RESET(nu->Flags, SPR_FALLING);

    nu->jump_grav = 8;
    nu->FlagOwner = 0;

    nu->active_range = 75000; // Set it far

    // if I didn't do this here they get stuck in the air sometimes
    DoActorZrange(New);

    DoActorJump(New);

    return New;
}

int
DoBunnyMove(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    // Parental lock crap
    if (TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
        RESET(sp->cstat, CSTAT_SPRITE_INVISIBLE); // Turn em' back on

    // Sometimes they just won't die!
    if (u->Health <= 0)
        SetSuicide(SpriteNum);

    if (u->scale_speed)
    {
        DoScaleSprite(SpriteNum);
    }

    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (TEST(u->Flags, SPR_JUMPING))
            DoActorJump(SpriteNum);
        else
            DoActorFall(SpriteNum);
    }

    // if on a player/enemy sprite jump quickly
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        DoBunnyQuickJump(SpriteNum);
    }

    if (TEST(u->Flags, SPR_SLIDING))
        DoActorSlide(SpriteNum);

    if (u->track >= 0)
        ActorFollowTrack(SpriteNum, ACTORMOVETICS);
    else
        (*u->ActorActionFunc)(SpriteNum);

    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
        KeepActorOnFloor(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    if (RANDOM_RANGE(1000) > 985 && sp->pal != PALETTE_PLAYER1 && u->track < 0)
    {
        switch (sector[sp->sectnum].floorpicnum)
        {
        case 153:
        case 154:
        case 193:
        case 219:
        case 2636:
        case 2689:
        case 3561:
        case 3562:
        case 3563:
        case 3564:
            NewStateGroup(SpriteNum,sg_BunnyStand);
            break;
        default:
            sp->ang = NORM_ANGLE(RANDOM_RANGE(2048 << 6) >> 6);
            u->jump_speed = -350;
            DoActorBeginJump(SpriteNum);
            u->ActorActionFunc = DoActorMoveJump;
            break;
        }
    }

    return 0;
}

int
BunnySpew(short SpriteNum)
{
    //InitBloodSpray(SpriteNum,TRUE,-1);
    InitBloodSpray(SpriteNum,TRUE,-1);
    return 0;
}

int
DoBunnyEat(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];


    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (TEST(u->Flags, SPR_JUMPING))
            DoActorJump(SpriteNum);
        else
            DoActorFall(SpriteNum);
    }

    // if on a player/enemy sprite jump quickly
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        DoBunnyQuickJump(SpriteNum);
    }

    if (TEST(u->Flags, SPR_SLIDING))
        DoActorSlide(SpriteNum);

    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
        KeepActorOnFloor(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    switch (sector[sp->sectnum].floorpicnum)
    {
    case 153:
    case 154:
    case 193:
    case 219:
    case 2636:
    case 2689:
    case 3561:
    case 3562:
    case 3563:
    case 3564:
        if (RANDOM_RANGE(1000) > 970)
            NewStateGroup(SpriteNum,sg_BunnyRun);
        break;
    default:
        NewStateGroup(SpriteNum,sg_BunnyRun);
        break;
    }
    return 0;
}

int
DoBunnyScrew(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int dist;

    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (TEST(u->Flags, SPR_JUMPING))
            DoActorJump(SpriteNum);
        else
            DoActorFall(SpriteNum);
    }

    if (TEST(u->Flags, SPR_SLIDING))
        DoActorSlide(SpriteNum);

    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
        KeepActorOnFloor(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    if (RANDOM_RANGE(1000) > 990) // Bunny sex sounds
    {
        if (!gs.ParentalLock && !Global_PLock)
            PlaySound(DIGI_BUNNYATTACK, &sp->x, &sp->y, &sp->z, v3df_follow);
    }

    u->WaitTics -= ACTORMOVETICS;

    if ((u->FlagOwner || u->spal == PALETTE_PLAYER0) && u->WaitTics > 0) // Keep Girl still
        NewStateGroup(SpriteNum,sg_BunnyScrew);

    if (u->spal == PALETTE_PLAYER0 && u->WaitTics <= 0) // Female has baby
    {
        u->Flag1 = SEC(5); // Count down to babies
        u->ShellNum = 1; // She's pregnant now
    }

    if (u->WaitTics <= 0)
    {
        RESET(sp->cstat, CSTAT_SPRITE_INVISIBLE); // Turn em' back on
        u->FlagOwner = 0;
        NewStateGroup(SpriteNum,sg_BunnyRun);
    }

    return 0;
}

int
DoBunnyGrowUp(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (sp->pal == PALETTE_PLAYER1) return 0;   // Don't bother white bunnies

    if ((u->Counter -= ACTORMOVETICS) <= 0)
    {
        if ((++sp->xrepeat) > 64) sp->xrepeat = 64;
        if ((++sp->yrepeat) > 64) sp->yrepeat = 64;
        u->Counter = 60;
    }

    // Don't go homo too much!
    if (sp->pal != PALETTE_PLAYER0 && u->Flag1 > 0)
        u->Flag1 -= ACTORMOVETICS;

    // Gestation period for female rabbits
    if (sp->pal == PALETTE_PLAYER0 && u->ShellNum > 0)
    {
        if ((u->Flag1 -= ACTORMOVETICS) <= 0)
        {
            if (Bunny_Count < 20)
            {
                PlaySound(DIGI_BUNNYDIE2, &sp->x, &sp->y, &sp->z, v3df_follow);
                BunnyHatch(SpriteNum); // Baby time
            }
            u->ShellNum = 0; // Not pregnent anymore
        }
    }

    return 0;
}


#include "saveable.h"

static saveable_code saveable_bunny_code[] =
{
    SAVE_CODE(SetupBunny),
    SAVE_CODE(GetBunnyJumpHeight),
    SAVE_CODE(PickBunnyJumpSpeed),
    SAVE_CODE(DoBunnyBeginJumpAttack),
    SAVE_CODE(DoBunnyMoveJump),
    SAVE_CODE(DoPickCloseBunny),
    SAVE_CODE(DoBunnyQuickJump),
    SAVE_CODE(NullBunny),
    SAVE_CODE(DoBunnyPain),
    SAVE_CODE(DoBunnyRipHeart),
    SAVE_CODE(DoBunnyStandKill),
    SAVE_CODE(BunnyHatch),
    SAVE_CODE(BunnyHatch2),
    SAVE_CODE(DoBunnyMove),
    SAVE_CODE(BunnySpew),
    SAVE_CODE(DoBunnyEat),
    SAVE_CODE(DoBunnyScrew),
    SAVE_CODE(DoBunnyGrowUp),
};

static saveable_data saveable_bunny_data[] =
{
    SAVE_DATA(BunnyBattle),
    SAVE_DATA(BunnyOffense),
    SAVE_DATA(BunnyBroadcast),
    SAVE_DATA(BunnySurprised),
    SAVE_DATA(BunnyEvasive),
    SAVE_DATA(BunnyLostTarget),
    SAVE_DATA(BunnyCloseRange),
    SAVE_DATA(BunnyWander),

    SAVE_DATA(WhiteBunnyPersonality),
    SAVE_DATA(BunnyPersonality),

    SAVE_DATA(WhiteBunnyAttrib),
    SAVE_DATA(BunnyAttrib),

    SAVE_DATA(s_BunnyRun),
    SAVE_DATA(sg_BunnyRun),
    SAVE_DATA(s_BunnyStand),
    SAVE_DATA(sg_BunnyStand),
    SAVE_DATA(s_BunnyScrew),
    SAVE_DATA(sg_BunnyScrew),
    SAVE_DATA(s_BunnySwipe),
    SAVE_DATA(sg_BunnySwipe),
    SAVE_DATA(s_BunnyHeart),
    SAVE_DATA(sg_BunnyHeart),
    SAVE_DATA(s_BunnyPain),
    SAVE_DATA(sg_BunnyPain),
    SAVE_DATA(s_BunnyJump),
    SAVE_DATA(sg_BunnyJump),
    SAVE_DATA(s_BunnyFall),
    SAVE_DATA(sg_BunnyFall),
    SAVE_DATA(s_BunnyJumpAttack),
    SAVE_DATA(sg_BunnyJumpAttack),
    SAVE_DATA(s_BunnyDie),
    SAVE_DATA(sg_BunnyDie),
    SAVE_DATA(s_BunnyDead),
    SAVE_DATA(sg_BunnyDead),
    SAVE_DATA(s_BunnyDeathJump),
    SAVE_DATA(sg_BunnyDeathJump),
    SAVE_DATA(s_BunnyDeathFall),
    SAVE_DATA(sg_BunnyDeathFall),

    SAVE_DATA(BunnyActionSet),
    SAVE_DATA(BunnyWhiteActionSet),
};

saveable_module saveable_bunny =
{
    // code
    saveable_bunny_code,
    SIZ(saveable_bunny_code),

    // data
    saveable_bunny_data,
    SIZ(saveable_bunny_data)
};
