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
#include "tags.h"
#include "ai.h"
#include "pal.h"
#include "sprite.h"
#include "weapon.h"
#include "misc.h"

BEGIN_SW_NS

int Bunny_Count = 0;
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


STATE* sg_BunnyRun[] =
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


STATE* sg_BunnyStand[] =
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


STATE* sg_BunnyScrew[] =
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


STATE* sg_BunnySwipe[] =
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


STATE* sg_BunnyHeart[] =
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

STATE* sg_BunnyPain[] =
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


STATE* sg_BunnyJump[] =
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


STATE* sg_BunnyFall[] =
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
int DoBunnyBeginJumpAttack(DSWActor* actor);

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


STATE* sg_BunnyJumpAttack[] =
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

STATE* sg_BunnyDie[] =
{
    s_BunnyDie
};

STATE* sg_BunnyDead[] =
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

STATE* sg_BunnyDeathJump[] =
{
    s_BunnyDeathJump
};

STATE* sg_BunnyDeathFall[] =
{
    s_BunnyDeathFall
};


/*
STATE* *Stand[MAX_WEAPONS];
STATE* *Run;
STATE* *Jump;
STATE* *Fall;
STATE* *Crawl;
STATE* *Swim;
STATE* *Fly;
STATE* *Rise;
STATE* *Sit;
STATE* *Look;
STATE* *Climb;
STATE* *Pain;
STATE* *Death1;
STATE* *Death2;
STATE* *Dead;
STATE* *DeathJump;
STATE* *DeathFall;
STATE* *CloseAttack[2];
STATE* *Attack[6];
STATE* *Special[2];
*/

ACTOR_ACTION_SET BunnyActionSet =
{
    sg_BunnyStand,
    sg_BunnyRun,
    sg_BunnyJump,
    sg_BunnyFall,
    nullptr,                               // sg_BunnyCrawl,
    nullptr,                               // sg_BunnySwim,
    nullptr,                               // sg_BunnyFly,
    nullptr,                               // sg_BunnyRise,
    nullptr,                               // sg_BunnySit,
    nullptr,                               // sg_BunnyLook,
    nullptr,                               // climb
    sg_BunnyPain,
    sg_BunnyDie,
    nullptr,
    sg_BunnyDead,
    sg_BunnyDeathJump,
    sg_BunnyDeathFall,
    {nullptr},
    {1024},
    {nullptr},
    {1024},
    {sg_BunnyHeart, sg_BunnyRun},
    nullptr,
    nullptr
};

ACTOR_ACTION_SET BunnyWhiteActionSet =
{
    sg_BunnyStand,
    sg_BunnyRun,
    sg_BunnyJump,
    sg_BunnyFall,
    nullptr,                               // sg_BunnyCrawl,
    nullptr,                               // sg_BunnySwim,
    nullptr,                               // sg_BunnyFly,
    nullptr,                               // sg_BunnyRise,
    nullptr,                               // sg_BunnySit,
    nullptr,                               // sg_BunnyLook,
    nullptr,                               // climb
    sg_BunnyPain,                       // pain
    sg_BunnyDie,
    nullptr,
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
    nullptr,
    nullptr
};

int SetupBunny(DSWActor* actor)
{
    ANIMATOR DoActorDecide;

    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, BUNNY_RUN_R0, s_BunnyRun[0]);
        actor->user.Health = 10;
    }

    Bunny_Count++;

    ChangeState(actor, s_BunnyRun[0]);
    actor->user.StateEnd = s_BunnyDie;
    actor->user.Rot = sg_BunnyRun;
    actor->user.ShellNum = 0; // Not Pregnant right now
    actor->user.FlagOwner = 0;

    actor->spr.clipdist = (150) >> 2;

    if (actor->spr.pal == PALETTE_PLAYER1)
    {
        EnemyDefaults(actor, &BunnyWhiteActionSet, &WhiteBunnyPersonality);
        actor->user.Attrib = &WhiteBunnyAttrib;
        actor->spr.xrepeat = 96;
        actor->spr.yrepeat = 90;

        actor->spr.clipdist = 200>>2;

        if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
            actor->user.Health = 60;
    }
    else if (actor->spr.pal == PALETTE_PLAYER8) // Male Rabbit
    {
        EnemyDefaults(actor, &BunnyActionSet, &BunnyPersonality);
        actor->user.Attrib = &BunnyAttrib;
        //actor->spr.xrepeat = 76;
        //actor->spr.yrepeat = 70;

        //actor->spr.shade = 0; // darker
        if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
            actor->user.Health = 20;
        actor->user.Flag1 = 0;
    }
    else
    {
        // Female Rabbit
        EnemyDefaults(actor, &BunnyActionSet, &BunnyPersonality);
        actor->user.Attrib = &BunnyAttrib;
        actor->user.spal = actor->spr.pal = PALETTE_PLAYER0;
        actor->user.Flag1 = SEC(5);
        //actor->spr.shade = 0; // darker
    }

    DoActorSetSpeed(actor, FAST_SPEED);

    actor->user.Flags |= (SPR_XFLIP_TOGGLE);


    actor->user.zclip = (16);
    actor->user.floor_dist = (8);
    actor->user.ceiling_dist = (8);
    actor->user.lo_step = Z(16);

    return 0;
}

int GetBunnyJumpHeight(int jump_speed, int jump_grav)
{
    int jump_iterations;
    int height;

    jump_speed = abs(jump_speed);

    jump_iterations = jump_speed / (jump_grav * ACTORMOVETICS);

    height = jump_speed * jump_iterations * ACTORMOVETICS;

    return height >> 9;
}

int PickBunnyJumpSpeed(DSWActor* actor, int pix_height)
{
    ASSERT(pix_height < 128);

    actor->user.jump_speed = -600;
    actor->user.jump_grav = 8;

    while (true)
    {
        if (GetBunnyJumpHeight(actor->user.jump_speed, actor->user.jump_grav) > pix_height + 20)
            break;

        actor->user.jump_speed -= 100;

        ASSERT(actor->user.jump_speed > -3000);
    }

    return actor->user.jump_speed;
}

//
// JUMP ATTACK
//

int DoBunnyBeginJumpAttack(DSWActor* actor)
{
    DSWActor* target = actor->user.targetActor;
    int tang;

    tang = getangle(target->int_pos().X - actor->int_pos().X, target->int_pos().Y - actor->int_pos().Y);

    Collision coll = move_sprite(actor, bcos(tang, -7), bsin(tang, -7),
        0L, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_ACTOR, ACTORMOVETICS);

    if (coll.type != kHitNone)
        actor->set_int_ang(NORM_ANGLE(actor->int_ang() + 1024) + (RANDOM_NEG(256, 6) >> 6));
    else
        actor->set_int_ang(NORM_ANGLE(tang + (RANDOM_NEG(256, 6) >> 6)));

    DoActorSetSpeed(actor, FAST_SPEED);

    //actor->user.jump_speed = -800;
    PickJumpMaxSpeed(actor, -400); // was -800

    actor->user.Flags |= (SPR_JUMPING);
    actor->user.Flags &= ~(SPR_FALLING);

    // set up individual actor jump gravity
    actor->user.jump_grav = 17; // was 8

    // if I didn't do this here they get stuck in the air sometimes
    DoActorZrange(actor);

    DoJump(actor);

    return 0;
}

int DoBunnyMoveJump(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        int nx, ny;

        // Move while jumping
        nx = MulScale(actor->spr.xvel, bcos(actor->int_ang()), 14);
        ny = MulScale(actor->spr.xvel, bsin(actor->int_ang()), 14);

        move_actor(actor, nx, ny, 0L);

        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else
            DoActorFall(actor);
    }

    DoActorZrange(actor);

    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        InitActorDecide(actor);
    }

    return 0;
}

void DoPickCloseBunny(DSWActor* actor)
{
    int dist, near_dist = 1000, a,b,c;

    // if actor can still see the player
    int look_height = ActorZOfTop(actor);
    bool ICanSee = false;

    SWStatIterator it(STAT_ENEMY);
    while (auto itActor = it.Next())
    {
        if (actor == itActor) continue;

        if (itActor->user.ID != BUNNY_RUN_R0) continue;

        DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, a, b, c);

        if (dist > near_dist) continue;

        ICanSee = FAFcansee(actor->int_pos().X, actor->int_pos().Y, look_height, actor->sector(), itActor->int_pos().X, itActor->int_pos().Y, ActorUpperZ(itActor), itActor->sector());

        if (ICanSee && dist < near_dist && itActor->user.ID == BUNNY_RUN_R0)
        {
            near_dist = dist;
            actor->user.targetActor = itActor;
            actor->user.lowActor = itActor;
            //Bunny_Result = i;
            return;
        }
    }
}

int DoBunnyQuickJump(DSWActor* actor)
{
    if (actor->user.spal != PALETTE_PLAYER8) return false;

    if (!actor->user.lowActor&& actor->user.spal == PALETTE_PLAYER8 && MoveSkip4)
        DoPickCloseBunny(actor);

    // Random Chance of like sexes fighting
    DSWActor* hitActor = actor->user.lowActor;
    if (hitActor)
    {
        if (!hitActor->hasU() || hitActor->user.ID != BUNNY_RUN_R0) return false;


        // Not mature enough yet
        if (actor->spr.xrepeat != 64 || actor->spr.yrepeat != 64) return false;
        if (hitActor->spr.xrepeat != 64 || hitActor->spr.yrepeat != 64) return false;

        // Kill a rival
        // Only males fight
        if (hitActor->user.spal == actor->spr.pal && RandomRange(1000) > 995)
        {
            if (actor->user.spal == PALETTE_PLAYER8 && hitActor->user.spal == PALETTE_PLAYER8)
            {
                PlaySound(DIGI_BUNNYATTACK, actor, v3df_follow);
                PlaySound(DIGI_BUNNYDIE2, hitActor, v3df_follow);
                hitActor->user.Health = 0;

                // Blood fountains
                InitBloodSpray(hitActor, true,-1);

                if (SpawnShrap(hitActor, actor))
                {
                    SetSuicide(hitActor);
                }
                else
                    DoActorDie(hitActor, actor, 0);

                Bunny_Count--; // Bunny died

                actor->user.lowActor = nullptr;
                return true;
            }
        }
    }

    // Get layed!
    hitActor = actor->user.lowActor;
    if (hitActor && actor->user.spal == PALETTE_PLAYER8) // Only males check this
    {
        if (!hitActor->hasU() || hitActor->user.ID != BUNNY_RUN_R0) return false;

        // Not mature enough to mate yet
        if (actor->spr.xrepeat != 64 || actor->spr.yrepeat != 64) return false;
        if (hitActor->spr.xrepeat != 64 || hitActor->spr.yrepeat != 64) return false;

        if (hitActor->user.ShellNum <= 0 && hitActor->user.WaitTics <= 0 && actor->user.WaitTics <= 0)
        {
            if ((hitActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
            {
                PLAYER* pp = nullptr;

                if (RandomRange(1000) < 995 && hitActor->user.spal != PALETTE_PLAYER0) return false;

                DoActorPickClosePlayer(actor);

                if (actor->user.targetActor->user.PlayerP)
                    pp = actor->user.targetActor->user.PlayerP;

                if (hitActor->user.spal != PALETTE_PLAYER0)
                {
                    if (hitActor->user.Flag1 > 0) return false;
                    hitActor->user.FlagOwner = 1; // FAG!
                    hitActor->user.Flag1 = SEC(10);
                    if (pp)
                    {
                        int choose_snd;
                        static const int fagsnds[] = {DIGI_FAGRABBIT1,DIGI_FAGRABBIT2,DIGI_FAGRABBIT3};

                        if (pp == Player+myconnectindex)
                        {
                            choose_snd = StdRandomRange(2<<8)>>8;
                            if (FAFcansee(actor->int_pos().X,actor->int_pos().Y,ActorZOfTop(actor),actor->sector(),pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, pp->cursector) && Facing(actor, actor->user.targetActor))
                                PlayerSound(fagsnds[choose_snd], v3df_doppler|v3df_follow|v3df_dontpan,pp);
                        }
                    }
                }
                else
                {
                    if (pp && RandomRange(1000) > 200)
                    {
                        int choose_snd;
                        static const int straightsnds[] = {DIGI_RABBITHUMP1,DIGI_RABBITHUMP2, DIGI_RABBITHUMP3,DIGI_RABBITHUMP4};

                        if (pp == Player+myconnectindex)
                        {
                            choose_snd = StdRandomRange(3<<8)>>8;
                            if (FAFcansee(actor->int_pos().X,actor->int_pos().Y,ActorZOfTop(actor),actor->sector(),pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, pp->cursector) && Facing(actor, actor->user.targetActor))
                                PlayerSound(straightsnds[choose_snd], v3df_doppler|v3df_follow|v3df_dontpan,pp);
                        }
                    }
                }

                actor->copyXY(hitActor);
                actor->spr.angle = hitActor->spr.angle;
                actor->set_int_ang(NORM_ANGLE(actor->int_ang() + 1024));
                HelpMissileLateral(actor, 2000);
                actor->spr.angle = hitActor->spr.angle;
                actor->user.Vis = actor->int_ang();  // Remember angles for later
                hitActor->user.Vis = hitActor->int_ang();

                NewStateGroup(actor, sg_BunnyScrew);
                NewStateGroup(hitActor, sg_BunnyScrew);
                actor->user.WaitTics = hitActor->user.WaitTics = SEC(10);  // Mate for this long
                return true;
            }
        }
    }

    return false;
}


int NullBunny(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else
            DoActorFall(actor);
    }

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
        KeepActorOnFloor(actor);

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    DoActorSectorDamage(actor);

    return 0;
}


int DoBunnyPain(DSWActor* actor)
{
    NullBunny(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(actor);
    return 0;
}

int DoBunnyRipHeart(DSWActor* actor)
{
    DSWActor* target = actor->user.targetActor;

    NewStateGroup(actor, sg_BunnyHeart);
    actor->user.WaitTics = 6 * 120;

    // player face bunny
    target->spr.angle = VecToAngle(actor->spr.pos - target->spr.pos);
    return 0;
}

int DoBunnyStandKill(DSWActor* actor)
{
    NullBunny(actor);

    // Growl like the bad ass bunny you are!
    if (RandomRange(1000) > 800)
        PlaySound(DIGI_BUNNYATTACK, actor, v3df_none);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        NewStateGroup(actor, sg_BunnyRun);
    return 0;
}


void BunnyHatch(DSWActor* actor)
{
    const int MAX_BUNNYS = 1;
    int16_t rip_ang[MAX_BUNNYS];

    rip_ang[0] = RANDOM_P2(2048);

    for (int i = 0; i < MAX_BUNNYS; i++)
    {
        auto actorNew = insertActor(actor->sector(), STAT_DEFAULT);
        actorNew->spr.pos = actor->spr.pos;
        actorNew->spr.xrepeat = 30;  // Baby size
        actorNew->spr.yrepeat = 24;
        actorNew->set_int_ang(rip_ang[i]);
        actorNew->spr.pal = 0;
        SetupBunny(actorNew);
        actorNew->spr.shade = actor->spr.shade;

        // make immediately active
        actorNew->user.Flags |= (SPR_ACTIVE);
        if (RandomRange(1000) > 500) // Boy or Girl?
            actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER0; // Girl
        else
        {
            actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER8; // Boy
            // Oops, mommy died giving birth to a boy
            if (RandomRange(1000) > 500)
            {
                actor->user.Health = 0;
                Bunny_Count--; // Bunny died

                // Blood fountains
                InitBloodSpray(actor, true, -1);

                if (SpawnShrap(actor, actorNew))
                {
                    SetSuicide(actor);
                }
                else
                    DoActorDie(actor, actorNew, 0);
            }
        }

        actorNew->user.ShellNum = 0; // Not Pregnant right now

        NewStateGroup(actorNew, actorNew->user.ActorActionSet->Jump);
        actorNew->user.ActorActionFunc = DoActorMoveJump;
        DoActorSetSpeed(actorNew, FAST_SPEED);
        PickJumpMaxSpeed(actorNew, -600);

        actorNew->user.Flags |= (SPR_JUMPING);
        actorNew->user.Flags &= ~(SPR_FALLING);

        actorNew->user.jump_grav = 8;

        // if I didn't do this here they get stuck in the air sometimes
        DoActorZrange(actorNew);

        DoActorJump(actorNew);
    }
}

DSWActor* BunnyHatch2(DSWActor* actor)
{

    auto actorNew = insertActor(actor->sector(), STAT_DEFAULT);
    actorNew->spr.pos = actor->spr.pos;
    actorNew->spr.xrepeat = 30;  // Baby size
    actorNew->spr.yrepeat = 24;
    actorNew->set_int_ang(RANDOM_P2(2048));
    actorNew->spr.pal = 0;
    SetupBunny(actorNew);
    actorNew->spr.shade = actor->spr.shade;

    // make immediately active
    actorNew->user.Flags |= (SPR_ACTIVE);
    if (RandomRange(1000) > 500) // Boy or Girl?
    {
        actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER0; // Girl
        actorNew->user.Flag1 = SEC(5);
    }
    else
    {
        actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER8; // Boy
        actorNew->user.Flag1 = 0;
    }

    actorNew->user.ShellNum = 0; // Not Pregnant right now

    NewStateGroup(actorNew, actorNew->user.ActorActionSet->Jump);
    actorNew->user.ActorActionFunc = DoActorMoveJump;
    DoActorSetSpeed(actorNew, FAST_SPEED);
    if (TEST_BOOL3(actor))
    {
        PickJumpMaxSpeed(actorNew, -600-RandomRange(600));
        actorNew->spr.xrepeat = actorNew->spr.yrepeat = 64;
        actorNew->spr.xvel = 150 + RandomRange(1000);
        actorNew->user.Health = 1; // Easy to pop. Like shootn' skeet.
        actorNew->add_int_ang(-RandomRange(128));
        actorNew->add_int_ang(RandomRange(128));
    }
    else
        PickJumpMaxSpeed(actorNew, -600);

    actorNew->user.Flags |= (SPR_JUMPING);
    actorNew->user.Flags &= ~(SPR_FALLING);

    actorNew->user.jump_grav = 8;
    actorNew->user.FlagOwner = 0;

    actorNew->user.active_range = 75000; // Set it far

    // if I didn't do this here they get stuck in the air sometimes
    DoActorZrange(actorNew);

    DoActorJump(actorNew);

    return actorNew;
}

int DoBunnyMove(DSWActor* actor)
{
    // Parental lock crap
    if (actor->spr.cstat & (CSTAT_SPRITE_INVISIBLE))
        actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE); // Turn em' back on

    // Sometimes they just won't die!
    if (actor->user.Health <= 0)
        SetSuicide(actor);

    if (actor->user.scale_speed)
    {
        DoScaleSprite(actor);
    }

    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else
            DoActorFall(actor);
    }

    // if on a player/enemy sprite jump quickly
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        DoBunnyQuickJump(actor);
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        (*actor->user.ActorActionFunc)(actor);

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    if (RandomRange(1000) > 985 && actor->spr.pal != PALETTE_PLAYER1 && actor->user.track < 0)
    {
        switch (actor->sector()->floorpicnum)
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
            NewStateGroup(actor,sg_BunnyStand);
            break;
        default:
            actor->set_int_ang(NORM_ANGLE(RandomRange(2048 << 6) >> 6));
            actor->user.jump_speed = -350;
            DoActorBeginJump(actor);
            actor->user.ActorActionFunc = DoActorMoveJump;
            break;
        }
    }

    return 0;
}

int BunnySpew(DSWActor* actor)
{
    InitBloodSpray(actor, true, -1);
    return 0;
}

int DoBunnyEat(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else
            DoActorFall(actor);
    }

    // if on a player/enemy sprite jump quickly
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        DoBunnyQuickJump(actor);
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    switch (actor->sector()->floorpicnum)
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
        if (RandomRange(1000) > 970)
            NewStateGroup(actor,sg_BunnyRun);
        break;
    default:
        NewStateGroup(actor,sg_BunnyRun);
        break;
    }
    return 0;
}

int DoBunnyScrew(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else
            DoActorFall(actor);
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    if (RandomRange(1000) > 990) // Bunny sex sounds
    {
         PlaySound(DIGI_BUNNYATTACK, actor, v3df_follow);
    }

    actor->user.WaitTics -= ACTORMOVETICS;

    if ((actor->user.FlagOwner || actor->user.spal == PALETTE_PLAYER0) && actor->user.WaitTics > 0) // Keep Girl still
        NewStateGroup(actor,sg_BunnyScrew);

    if (actor->user.spal == PALETTE_PLAYER0 && actor->user.WaitTics <= 0) // Female has baby
    {
        actor->user.Flag1 = SEC(5); // Count down to babies
        actor->user.ShellNum = 1; // She's pregnant now
    }

    if (actor->user.WaitTics <= 0)
    {
        actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE); // Turn em' back on
        actor->user.FlagOwner = 0;
        NewStateGroup(actor,sg_BunnyRun);
    }

    return 0;
}

int DoBunnyGrowUp(DSWActor* actor)
{
    if (actor->spr.pal == PALETTE_PLAYER1) return 0;   // Don't bother white bunnies

    if ((actor->user.Counter -= ACTORMOVETICS) <= 0)
    {
        if ((++actor->spr.xrepeat) > 64) actor->spr.xrepeat = 64;
        if ((++actor->spr.yrepeat) > 64) actor->spr.yrepeat = 64;
        actor->user.Counter = 60;
    }

    // Don't go homo too much!
    if (actor->spr.pal != PALETTE_PLAYER0 && actor->user.Flag1 > 0)
        actor->user.Flag1 -= ACTORMOVETICS;

    // Gestation period for female rabbits
    if (actor->spr.pal == PALETTE_PLAYER0 && actor->user.ShellNum > 0)
    {
        if ((actor->user.Flag1 -= ACTORMOVETICS) <= 0)
        {
            if (Bunny_Count < 20)
            {
                PlaySound(DIGI_BUNNYDIE2, actor, v3df_follow);
                BunnyHatch(actor); // Baby time
            }
            actor->user.ShellNum = 0; // Not pregnent anymore
        }
    }

    return 0;
}


#include "saveable.h"

static saveable_code saveable_bunny_code[] =
{
    SAVE_CODE(DoBunnyBeginJumpAttack),
    SAVE_CODE(DoBunnyMoveJump),
    SAVE_CODE(DoPickCloseBunny),
    SAVE_CODE(DoBunnyQuickJump),
    SAVE_CODE(NullBunny),
    SAVE_CODE(DoBunnyPain),
    SAVE_CODE(DoBunnyRipHeart),
    SAVE_CODE(DoBunnyStandKill),
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
END_SW_NS
