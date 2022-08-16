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
#include "ai.h"
#include "pal.h"
#include "sprite.h"
#include "misc.h"

BEGIN_SW_NS

ANIMATOR InitRipper2Hang;
ANIMATOR DoRipper2MoveJump;
ANIMATOR DoRipper2HangJF;
ANIMATOR DoRipper2QuickJump;
ANIMATOR InitRipper2Charge;

DECISION Ripper2Battle[] =
{
    {879, InitRipper2Charge},
    {883, InitActorAttackNoise},
    {900, InitRipper2Hang},
    {1024, InitActorAttack}
};

DECISION Ripper2Offense[] =
{
    {789, InitActorMoveCloser},
    {790, InitActorAttackNoise},
    {800, InitRipper2Hang},
    {1024, InitActorAttack}
};

DECISION Ripper2Broadcast[] =
{
    {3, InitActorAmbientNoise},
    {1024, InitActorDecide}
};

DECISION Ripper2Surprised[] =
{
    {40, InitRipper2Hang},
    {701, InitActorMoveCloser},
    {1024, InitActorDecide}
};

DECISION Ripper2Evasive[] =
{
    {10, InitActorMoveCloser},
    {1024, InitRipper2Charge}
};

DECISION Ripper2LostTarget[] =
{
    {900, InitActorFindPlayer},
    {1024, InitActorWanderAround}
};

DECISION Ripper2CloseRange[] =
{
    {1024,  InitActorAttack         }
};

PERSONALITY Ripper2Personality =
{
    Ripper2Battle,
    Ripper2Offense,
    Ripper2Broadcast,
    Ripper2Surprised,
    Ripper2Evasive,
    Ripper2LostTarget,
    Ripper2CloseRange,
    Ripper2CloseRange
};

ATTRIBUTE Ripper2Attrib =
{
    {100, 120, 300, 380},               // Speeds
    {5, 0, -2, -4},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_RIPPER2AMBIENT, DIGI_RIPPER2ALERT, DIGI_RIPPER2ATTACK,
        DIGI_RIPPER2PAIN, DIGI_RIPPER2SCREAM, DIGI_RIPPER2HEARTOUT,
        0,0,0,0
    }
};

//////////////////////
//
// RIPPER2 RUN
//
//////////////////////

#define RIPPER2_RUN_RATE 16

ANIMATOR DoRipper2Move, NullRipper2, DoActorDebris;

STATE s_Ripper2Run[5][4] =
{
    {
        {RIPPER2_RUN_R0 + 0, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[0][1]},
        {RIPPER2_RUN_R0 + 1, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[0][2]},
        {RIPPER2_RUN_R0 + 2, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[0][3]},
        {RIPPER2_RUN_R0 + 3, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[0][0]},
    },
    {
        {RIPPER2_RUN_R1 + 0, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[1][1]},
        {RIPPER2_RUN_R1 + 1, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[1][2]},
        {RIPPER2_RUN_R1 + 2, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[1][3]},
        {RIPPER2_RUN_R1 + 3, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[1][0]},
    },
    {
        {RIPPER2_RUN_R2 + 0, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[2][1]},
        {RIPPER2_RUN_R2 + 1, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[2][2]},
        {RIPPER2_RUN_R2 + 2, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[2][3]},
        {RIPPER2_RUN_R2 + 3, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[2][0]},
    },
    {
        {RIPPER2_RUN_R3 + 0, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[3][1]},
        {RIPPER2_RUN_R3 + 1, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[3][2]},
        {RIPPER2_RUN_R3 + 2, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[3][3]},
        {RIPPER2_RUN_R3 + 3, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[3][0]},
    },
    {
        {RIPPER2_RUN_R4 + 0, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[4][1]},
        {RIPPER2_RUN_R4 + 1, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[4][2]},
        {RIPPER2_RUN_R4 + 2, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[4][3]},
        {RIPPER2_RUN_R4 + 3, RIPPER2_RUN_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2Run[4][0]},
    }
};


STATE* sg_Ripper2Run[] =
{
    &s_Ripper2Run[0][0],
    &s_Ripper2Run[1][0],
    &s_Ripper2Run[2][0],
    &s_Ripper2Run[3][0],
    &s_Ripper2Run[4][0]
};

//////////////////////
//
// RIPPER2 RUNFAST
//
//////////////////////

#define RIPPER2_RUNFAST_RATE 14

ANIMATOR NullRipper2, DoActorDebris;

STATE s_Ripper2RunFast[5][4] =
{
    {
        {RIPPER2_RUNFAST_R0 + 0, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[0][1]},
        {RIPPER2_RUNFAST_R0 + 1, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[0][2]},
        {RIPPER2_RUNFAST_R0 + 2, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[0][3]},
        {RIPPER2_RUNFAST_R0 + 3, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[0][0]},
    },
    {
        {RIPPER2_RUNFAST_R1 + 0, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[1][1]},
        {RIPPER2_RUNFAST_R1 + 1, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[1][2]},
        {RIPPER2_RUNFAST_R1 + 2, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[1][3]},
        {RIPPER2_RUNFAST_R1 + 3, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[1][0]},
    },
    {
        {RIPPER2_RUNFAST_R2 + 0, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[2][1]},
        {RIPPER2_RUNFAST_R2 + 1, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[2][2]},
        {RIPPER2_RUNFAST_R2 + 2, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[2][3]},
        {RIPPER2_RUNFAST_R2 + 3, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[2][0]},
    },
    {
        {RIPPER2_RUNFAST_R3 + 0, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[3][1]},
        {RIPPER2_RUNFAST_R3 + 1, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[3][2]},
        {RIPPER2_RUNFAST_R3 + 2, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[3][3]},
        {RIPPER2_RUNFAST_R3 + 3, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[3][0]},
    },
    {
        {RIPPER2_RUNFAST_R4 + 0, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[4][1]},
        {RIPPER2_RUNFAST_R4 + 1, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[4][2]},
        {RIPPER2_RUNFAST_R4 + 2, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[4][3]},
        {RIPPER2_RUNFAST_R4 + 3, RIPPER2_RUNFAST_RATE | SF_TIC_ADJUST, DoRipper2Move, &s_Ripper2RunFast[4][0]},
    }
};


STATE* sg_Ripper2RunFast[] =
{
    &s_Ripper2RunFast[0][0],
    &s_Ripper2RunFast[1][0],
    &s_Ripper2RunFast[2][0],
    &s_Ripper2RunFast[3][0],
    &s_Ripper2RunFast[4][0]
};

//////////////////////
//
// RIPPER2 STAND
//
//////////////////////

#define RIPPER2_STAND_RATE 12

STATE s_Ripper2Stand[5][1] =
{
    {
        {RIPPER2_STAND_R0 + 0, RIPPER2_STAND_RATE, DoRipper2Move, &s_Ripper2Stand[0][0]},
    },
    {
        {RIPPER2_STAND_R1 + 0, RIPPER2_STAND_RATE, DoRipper2Move, &s_Ripper2Stand[1][0]},
    },
    {
        {RIPPER2_STAND_R2 + 0, RIPPER2_STAND_RATE, DoRipper2Move, &s_Ripper2Stand[2][0]},
    },
    {
        {RIPPER2_STAND_R3 + 0, RIPPER2_STAND_RATE, DoRipper2Move, &s_Ripper2Stand[3][0]},
    },
    {
        {RIPPER2_STAND_R4 + 0, RIPPER2_STAND_RATE, DoRipper2Move, &s_Ripper2Stand[4][0]},
    },
};

STATE* sg_Ripper2Stand[] =
{
    s_Ripper2Stand[0],
    s_Ripper2Stand[1],
    s_Ripper2Stand[2],
    s_Ripper2Stand[3],
    s_Ripper2Stand[4]
};

//////////////////////
//
// RIPPER2 SWIPE
//
//////////////////////

#define RIPPER2_SWIPE_RATE 14
ANIMATOR InitActorDecide;
ANIMATOR InitRipperSlash;

STATE s_Ripper2Swipe[5][8] =
{
    {
        {RIPPER2_SWIPE_R0 + 0, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[0][1]},
        {RIPPER2_SWIPE_R0 + 1, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[0][2]},
        {RIPPER2_SWIPE_R0 + 1, 0 | SF_QUICK_CALL, InitRipperSlash, &s_Ripper2Swipe[0][3]},
        {RIPPER2_SWIPE_R0 + 2, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[0][4]},
        {RIPPER2_SWIPE_R0 + 3, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[0][5]},
        {RIPPER2_SWIPE_R0 + 3, 0 | SF_QUICK_CALL, InitRipperSlash, &s_Ripper2Swipe[0][6]},
        {RIPPER2_SWIPE_R0 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_Ripper2Swipe[0][7]},
        {RIPPER2_SWIPE_R0 + 3, RIPPER2_SWIPE_RATE, DoRipper2Move, &s_Ripper2Swipe[0][7]},
    },
    {
        {RIPPER2_SWIPE_R1 + 0, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[1][1]},
        {RIPPER2_SWIPE_R1 + 1, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[1][2]},
        {RIPPER2_SWIPE_R1 + 1, 0 | SF_QUICK_CALL, InitRipperSlash, &s_Ripper2Swipe[1][3]},
        {RIPPER2_SWIPE_R1 + 2, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[1][4]},
        {RIPPER2_SWIPE_R1 + 3, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[1][5]},
        {RIPPER2_SWIPE_R1 + 3, 0 | SF_QUICK_CALL, InitRipperSlash, &s_Ripper2Swipe[1][6]},
        {RIPPER2_SWIPE_R1 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_Ripper2Swipe[1][7]},
        {RIPPER2_SWIPE_R1 + 3, RIPPER2_SWIPE_RATE, DoRipper2Move, &s_Ripper2Swipe[1][7]},
    },
    {
        {RIPPER2_SWIPE_R2 + 0, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[2][1]},
        {RIPPER2_SWIPE_R2 + 1, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[2][2]},
        {RIPPER2_SWIPE_R2 + 1, 0 | SF_QUICK_CALL, InitRipperSlash, &s_Ripper2Swipe[2][3]},
        {RIPPER2_SWIPE_R2 + 2, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[2][4]},
        {RIPPER2_SWIPE_R2 + 3, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[2][5]},
        {RIPPER2_SWIPE_R2 + 3, 0 | SF_QUICK_CALL, InitRipperSlash, &s_Ripper2Swipe[2][6]},
        {RIPPER2_SWIPE_R2 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_Ripper2Swipe[2][7]},
        {RIPPER2_SWIPE_R2 + 3, RIPPER2_SWIPE_RATE, DoRipper2Move, &s_Ripper2Swipe[2][7]},
    },
    {
        {RIPPER2_SWIPE_R3 + 0, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[3][1]},
        {RIPPER2_SWIPE_R3 + 1, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[3][2]},
        {RIPPER2_SWIPE_R3 + 1, 0 | SF_QUICK_CALL, InitRipperSlash, &s_Ripper2Swipe[3][3]},
        {RIPPER2_SWIPE_R3 + 2, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[3][4]},
        {RIPPER2_SWIPE_R3 + 3, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[3][5]},
        {RIPPER2_SWIPE_R3 + 3, 0 | SF_QUICK_CALL, InitRipperSlash, &s_Ripper2Swipe[3][6]},
        {RIPPER2_SWIPE_R3 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_Ripper2Swipe[3][7]},
        {RIPPER2_SWIPE_R3 + 3, RIPPER2_SWIPE_RATE, DoRipper2Move, &s_Ripper2Swipe[3][7]},
    },
    {
        {RIPPER2_SWIPE_R4 + 0, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[4][1]},
        {RIPPER2_SWIPE_R4 + 1, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[4][2]},
        {RIPPER2_SWIPE_R4 + 1, 0 | SF_QUICK_CALL, InitRipperSlash, &s_Ripper2Swipe[4][3]},
        {RIPPER2_SWIPE_R4 + 2, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[4][4]},
        {RIPPER2_SWIPE_R4 + 3, RIPPER2_SWIPE_RATE, NullRipper2, &s_Ripper2Swipe[4][5]},
        {RIPPER2_SWIPE_R4 + 3, 0 | SF_QUICK_CALL, InitRipperSlash, &s_Ripper2Swipe[4][6]},
        {RIPPER2_SWIPE_R4 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_Ripper2Swipe[4][7]},
        {RIPPER2_SWIPE_R4 + 3, RIPPER2_SWIPE_RATE, DoRipper2Move, &s_Ripper2Swipe[4][7]},
    }
};


STATE* sg_Ripper2Swipe[] =
{
    &s_Ripper2Swipe[0][0],
    &s_Ripper2Swipe[1][0],
    &s_Ripper2Swipe[2][0],
    &s_Ripper2Swipe[3][0],
    &s_Ripper2Swipe[4][0]
};

//////////////////////
//
// RIPPER2 KONG
//
//////////////////////

#define RIPPER2_MEKONG_RATE 18
ANIMATOR InitActorDecide, ChestRipper2;

STATE s_Ripper2Kong[5][7] =
{
    {
        {RIPPER2_MEKONG_R0 + 0, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[0][1]},
        {RIPPER2_MEKONG_R0 + 0, SF_QUICK_CALL,       ChestRipper2, &s_Ripper2Kong[0][2]},
        {RIPPER2_MEKONG_R0 + 1, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[0][3]},
        {RIPPER2_MEKONG_R0 + 2, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[0][4]},
        {RIPPER2_MEKONG_R0 + 3, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[0][5]},
        {RIPPER2_MEKONG_R0 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_Ripper2Kong[0][6]},
        {RIPPER2_MEKONG_R0 + 0, RIPPER2_MEKONG_RATE, DoRipper2Move, &s_Ripper2Kong[0][6]},
    },
    {
        {RIPPER2_MEKONG_R1 + 0, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[1][1]},
        {RIPPER2_MEKONG_R0 + 0, SF_QUICK_CALL,       ChestRipper2, &s_Ripper2Kong[1][2]},
        {RIPPER2_MEKONG_R1 + 1, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[1][3]},
        {RIPPER2_MEKONG_R1 + 2, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[1][4]},
        {RIPPER2_MEKONG_R1 + 3, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[1][5]},
        {RIPPER2_MEKONG_R1 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_Ripper2Kong[1][6]},
        {RIPPER2_MEKONG_R1 + 0, RIPPER2_MEKONG_RATE, DoRipper2Move, &s_Ripper2Kong[1][6]},
    },
    {
        {RIPPER2_MEKONG_R2 + 0, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[2][1]},
        {RIPPER2_MEKONG_R0 + 0, SF_QUICK_CALL,       ChestRipper2, &s_Ripper2Kong[2][2]},
        {RIPPER2_MEKONG_R2 + 1, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[2][3]},
        {RIPPER2_MEKONG_R2 + 2, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[2][4]},
        {RIPPER2_MEKONG_R2 + 3, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[2][5]},
        {RIPPER2_MEKONG_R2 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_Ripper2Kong[2][6]},
        {RIPPER2_MEKONG_R2 + 0, RIPPER2_MEKONG_RATE, DoRipper2Move, &s_Ripper2Kong[2][6]},
    },
    {
        {RIPPER2_MEKONG_R3 + 0, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[3][1]},
        {RIPPER2_MEKONG_R0 + 0, SF_QUICK_CALL,       ChestRipper2, &s_Ripper2Kong[3][2]},
        {RIPPER2_MEKONG_R3 + 1, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[3][3]},
        {RIPPER2_MEKONG_R3 + 2, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[3][4]},
        {RIPPER2_MEKONG_R3 + 3, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[3][5]},
        {RIPPER2_MEKONG_R3 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_Ripper2Kong[3][6]},
        {RIPPER2_MEKONG_R3 + 0, RIPPER2_MEKONG_RATE, DoRipper2Move, &s_Ripper2Kong[3][6]},
    },
    {
        {RIPPER2_MEKONG_R4 + 0, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[4][1]},
        {RIPPER2_MEKONG_R0 + 0, SF_QUICK_CALL,       ChestRipper2, &s_Ripper2Kong[4][2]},
        {RIPPER2_MEKONG_R4 + 1, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[4][3]},
        {RIPPER2_MEKONG_R4 + 2, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[4][4]},
        {RIPPER2_MEKONG_R4 + 3, RIPPER2_MEKONG_RATE, NullRipper2, &s_Ripper2Kong[4][5]},
        {RIPPER2_MEKONG_R4 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_Ripper2Kong[4][6]},
        {RIPPER2_MEKONG_R4 + 0, RIPPER2_MEKONG_RATE, DoRipper2Move, &s_Ripper2Kong[4][6]},
    }
};


STATE* sg_Ripper2Kong[] =
{
    &s_Ripper2Kong[0][0],
    &s_Ripper2Kong[1][0],
    &s_Ripper2Kong[2][0],
    &s_Ripper2Kong[3][0],
    &s_Ripper2Kong[4][0]
};


//////////////////////
//
// RIPPER2 HEART - show players heart
//
//////////////////////

#define RIPPER2_HEART_RATE 20
ANIMATOR DoRipper2StandHeart;

STATE s_Ripper2Heart[5][4] =
{
    {
        {RIPPER2_HEART_R0 + 0, RIPPER2_HEART_RATE, DoRipper2StandHeart, &s_Ripper2Heart[0][1]},
        {RIPPER2_HEART_R0 + 1, RIPPER2_HEART_RATE, DoRipper2StandHeart, &s_Ripper2Heart[0][0]},
    },
    {
        {RIPPER2_HEART_R1 + 0, RIPPER2_HEART_RATE, DoRipper2StandHeart, &s_Ripper2Heart[1][1]},
        {RIPPER2_HEART_R1 + 1, RIPPER2_HEART_RATE, DoRipper2StandHeart, &s_Ripper2Heart[1][0]},
    },
    {
        {RIPPER2_HEART_R2 + 0, RIPPER2_HEART_RATE, DoRipper2StandHeart, &s_Ripper2Heart[2][1]},
        {RIPPER2_HEART_R2 + 1, RIPPER2_HEART_RATE, DoRipper2StandHeart, &s_Ripper2Heart[2][0]},
    },
    {
        {RIPPER2_HEART_R3 + 0, RIPPER2_HEART_RATE, DoRipper2StandHeart, &s_Ripper2Heart[3][1]},
        {RIPPER2_HEART_R3 + 1, RIPPER2_HEART_RATE, DoRipper2StandHeart, &s_Ripper2Heart[3][0]},
    },
    {
        {RIPPER2_HEART_R4 + 0, RIPPER2_HEART_RATE, DoRipper2StandHeart, &s_Ripper2Heart[4][1]},
        {RIPPER2_HEART_R4 + 1, RIPPER2_HEART_RATE, DoRipper2StandHeart, &s_Ripper2Heart[4][0]},
    }
};


STATE* sg_Ripper2Heart[] =
{
    &s_Ripper2Heart[0][0],
    &s_Ripper2Heart[1][0],
    &s_Ripper2Heart[2][0],
    &s_Ripper2Heart[3][0],
    &s_Ripper2Heart[4][0]
};

//////////////////////
//
// RIPPER2 HANG
//
//////////////////////

#define RIPPER2_HANG_RATE 14
ANIMATOR DoRipper2Hang;

STATE s_Ripper2Hang[5][4] =
{
    {
        {RIPPER2_HANG_R0, RIPPER2_HANG_RATE, DoRipper2Hang, &s_Ripper2Hang[0][0]},
    },
    {
        {RIPPER2_HANG_R1, RIPPER2_HANG_RATE, DoRipper2Hang, &s_Ripper2Hang[1][0]},
    },
    {
        {RIPPER2_HANG_R2, RIPPER2_HANG_RATE, DoRipper2Hang, &s_Ripper2Hang[2][0]},
    },
    {
        {RIPPER2_HANG_R3, RIPPER2_HANG_RATE, DoRipper2Hang, &s_Ripper2Hang[3][0]},
    },
    {
        {RIPPER2_HANG_R4, RIPPER2_HANG_RATE, DoRipper2Hang, &s_Ripper2Hang[4][0]},
    }
};


STATE* sg_Ripper2Hang[] =
{
    &s_Ripper2Hang[0][0],
    &s_Ripper2Hang[1][0],
    &s_Ripper2Hang[2][0],
    &s_Ripper2Hang[3][0],
    &s_Ripper2Hang[4][0]
};


//////////////////////
//
// RIPPER2 PAIN
//
//////////////////////

#define RIPPER2_PAIN_RATE 38
ANIMATOR DoRipper2Pain;

STATE s_Ripper2Pain[5][1] =
{
    {
        {4414 + 0, RIPPER2_PAIN_RATE, DoRipper2Pain, &s_Ripper2Pain[0][0]},
    },
    {
        {4414 + 0, RIPPER2_PAIN_RATE, DoRipper2Pain, &s_Ripper2Pain[1][0]},
    },
    {
        {4414 + 0, RIPPER2_PAIN_RATE, DoRipper2Pain, &s_Ripper2Pain[2][0]},
    },
    {
        {4414 + 0, RIPPER2_PAIN_RATE, DoRipper2Pain, &s_Ripper2Pain[3][0]},
    },
    {
        {4414 + 0, RIPPER2_PAIN_RATE, DoRipper2Pain, &s_Ripper2Pain[4][0]},
    }
};

STATE* sg_Ripper2Pain[] =
{
    &s_Ripper2Pain[0][0],
    &s_Ripper2Pain[1][0],
    &s_Ripper2Pain[2][0],
    &s_Ripper2Pain[3][0],
    &s_Ripper2Pain[4][0]
};

//////////////////////
//
// RIPPER2 JUMP
//
//////////////////////

#define RIPPER2_JUMP_RATE 25

STATE s_Ripper2Jump[5][6] =
{
    {
        {RIPPER2_JUMP_R0 + 0, RIPPER2_JUMP_RATE, NullRipper2, &s_Ripper2Jump[0][1]},
        {RIPPER2_JUMP_R0 + 1, RIPPER2_JUMP_RATE, DoRipper2MoveJump, &s_Ripper2Jump[0][1]},
    },
    {
        {RIPPER2_JUMP_R1 + 0, RIPPER2_JUMP_RATE, NullRipper2, &s_Ripper2Jump[1][1]},
        {RIPPER2_JUMP_R1 + 1, RIPPER2_JUMP_RATE, DoRipper2MoveJump, &s_Ripper2Jump[1][1]},
    },
    {
        {RIPPER2_JUMP_R2 + 0, RIPPER2_JUMP_RATE, NullRipper2, &s_Ripper2Jump[2][1]},
        {RIPPER2_JUMP_R2 + 1, RIPPER2_JUMP_RATE, DoRipper2MoveJump, &s_Ripper2Jump[2][1]},
    },
    {
        {RIPPER2_JUMP_R3 + 0, RIPPER2_JUMP_RATE, NullRipper2, &s_Ripper2Jump[3][1]},
        {RIPPER2_JUMP_R3 + 1, RIPPER2_JUMP_RATE, DoRipper2MoveJump, &s_Ripper2Jump[3][1]},
    },
    {
        {RIPPER2_JUMP_R4 + 0, RIPPER2_JUMP_RATE, NullRipper2, &s_Ripper2Jump[4][1]},
        {RIPPER2_JUMP_R4 + 1, RIPPER2_JUMP_RATE, DoRipper2MoveJump, &s_Ripper2Jump[4][1]},
    }
};


STATE* sg_Ripper2Jump[] =
{
    &s_Ripper2Jump[0][0],
    &s_Ripper2Jump[1][0],
    &s_Ripper2Jump[2][0],
    &s_Ripper2Jump[3][0],
    &s_Ripper2Jump[4][0]
};


//////////////////////
//
// RIPPER2 FALL
//
//////////////////////

#define RIPPER2_FALL_RATE 25

STATE s_Ripper2Fall[5][6] =
{
    {
        {RIPPER2_FALL_R0 + 0, RIPPER2_FALL_RATE, DoRipper2MoveJump, &s_Ripper2Fall[0][0]},
    },
    {
        {RIPPER2_FALL_R1 + 0, RIPPER2_FALL_RATE, DoRipper2MoveJump, &s_Ripper2Fall[1][0]},
    },
    {
        {RIPPER2_FALL_R2 + 0, RIPPER2_FALL_RATE, DoRipper2MoveJump, &s_Ripper2Fall[2][0]},
    },
    {
        {RIPPER2_FALL_R3 + 0, RIPPER2_FALL_RATE, DoRipper2MoveJump, &s_Ripper2Fall[3][0]},
    },
    {
        {RIPPER2_FALL_R4 + 0, RIPPER2_FALL_RATE, DoRipper2MoveJump, &s_Ripper2Fall[4][0]},
    }
};


STATE* sg_Ripper2Fall[] =
{
    &s_Ripper2Fall[0][0],
    &s_Ripper2Fall[1][0],
    &s_Ripper2Fall[2][0],
    &s_Ripper2Fall[3][0],
    &s_Ripper2Fall[4][0]
};


//////////////////////
//
// RIPPER2 JUMP ATTACK
//
//////////////////////

#define RIPPER2_JUMP_ATTACK_RATE 35
int DoRipper2BeginJumpAttack(DSWActor* actor);

STATE s_Ripper2JumpAttack[5][6] =
{
    {
        {RIPPER2_JUMP_R0 + 0, RIPPER2_JUMP_ATTACK_RATE, NullRipper2, &s_Ripper2JumpAttack[0][1]},
        {RIPPER2_JUMP_R0 + 0, 0 | SF_QUICK_CALL, DoRipper2BeginJumpAttack, &s_Ripper2JumpAttack[0][2]},
        {RIPPER2_JUMP_R0 + 2, RIPPER2_JUMP_ATTACK_RATE, DoRipper2MoveJump, &s_Ripper2JumpAttack[0][3]},
        {RIPPER2_JUMP_R0 + 1, RIPPER2_JUMP_ATTACK_RATE, DoRipper2MoveJump, &s_Ripper2JumpAttack[0][3]},
    },
    {
        {RIPPER2_JUMP_R1 + 0, RIPPER2_JUMP_ATTACK_RATE, NullRipper2, &s_Ripper2JumpAttack[1][1]},
        {RIPPER2_JUMP_R1 + 0, 0 | SF_QUICK_CALL, DoRipper2BeginJumpAttack, &s_Ripper2JumpAttack[1][2]},
        {RIPPER2_JUMP_R1 + 2, RIPPER2_JUMP_ATTACK_RATE, DoRipper2MoveJump, &s_Ripper2JumpAttack[1][3]},
        {RIPPER2_JUMP_R1 + 1, RIPPER2_JUMP_ATTACK_RATE, DoRipper2MoveJump, &s_Ripper2JumpAttack[1][3]},
    },
    {
        {RIPPER2_JUMP_R2 + 0, RIPPER2_JUMP_ATTACK_RATE, NullRipper2, &s_Ripper2JumpAttack[2][1]},
        {RIPPER2_JUMP_R2 + 0, 0 | SF_QUICK_CALL, DoRipper2BeginJumpAttack, &s_Ripper2JumpAttack[2][2]},
        {RIPPER2_JUMP_R2 + 2, RIPPER2_JUMP_ATTACK_RATE, DoRipper2MoveJump, &s_Ripper2JumpAttack[2][3]},
        {RIPPER2_JUMP_R1 + 1, RIPPER2_JUMP_ATTACK_RATE, DoRipper2MoveJump, &s_Ripper2JumpAttack[2][3]},
    },
    {
        {RIPPER2_JUMP_R3 + 0, RIPPER2_JUMP_ATTACK_RATE, NullRipper2, &s_Ripper2JumpAttack[3][1]},
        {RIPPER2_JUMP_R3 + 0, 0 | SF_QUICK_CALL, DoRipper2BeginJumpAttack, &s_Ripper2JumpAttack[3][2]},
        {RIPPER2_JUMP_R3 + 2, RIPPER2_JUMP_ATTACK_RATE, DoRipper2MoveJump, &s_Ripper2JumpAttack[3][3]},
        {RIPPER2_JUMP_R1 + 1, RIPPER2_JUMP_ATTACK_RATE, DoRipper2MoveJump, &s_Ripper2JumpAttack[3][3]},
    },
    {
        {RIPPER2_JUMP_R4 + 0, RIPPER2_JUMP_ATTACK_RATE, NullRipper2, &s_Ripper2JumpAttack[4][1]},
        {RIPPER2_JUMP_R4 + 0, 0 | SF_QUICK_CALL, DoRipper2BeginJumpAttack, &s_Ripper2JumpAttack[4][2]},
        {RIPPER2_JUMP_R4 + 2, RIPPER2_JUMP_ATTACK_RATE, DoRipper2MoveJump, &s_Ripper2JumpAttack[4][3]},
        {RIPPER2_JUMP_R1 + 1, RIPPER2_JUMP_ATTACK_RATE, DoRipper2MoveJump, &s_Ripper2JumpAttack[4][3]},
    }
};


STATE* sg_Ripper2JumpAttack[] =
{
    &s_Ripper2JumpAttack[0][0],
    &s_Ripper2JumpAttack[1][0],
    &s_Ripper2JumpAttack[2][0],
    &s_Ripper2JumpAttack[3][0],
    &s_Ripper2JumpAttack[4][0]
};


//////////////////////
//
// RIPPER2 HANG_JUMP
//
//////////////////////

#define RIPPER2_HANG_JUMP_RATE 20

STATE s_Ripper2HangJump[5][6] =
{
    {
        {RIPPER2_JUMP_R0 + 0, RIPPER2_HANG_JUMP_RATE, NullRipper2, &s_Ripper2HangJump[0][1]},
        {RIPPER2_JUMP_R0 + 1, RIPPER2_HANG_JUMP_RATE, DoRipper2HangJF, &s_Ripper2HangJump[0][1]},
    },
    {
        {RIPPER2_JUMP_R1 + 0, RIPPER2_HANG_JUMP_RATE, NullRipper2, &s_Ripper2HangJump[1][1]},
        {RIPPER2_JUMP_R1 + 1, RIPPER2_HANG_JUMP_RATE, DoRipper2HangJF, &s_Ripper2HangJump[1][1]},
    },
    {
        {RIPPER2_JUMP_R2 + 0, RIPPER2_HANG_JUMP_RATE, NullRipper2, &s_Ripper2HangJump[2][1]},
        {RIPPER2_JUMP_R2 + 1, RIPPER2_HANG_JUMP_RATE, DoRipper2HangJF, &s_Ripper2HangJump[2][1]},
    },
    {
        {RIPPER2_JUMP_R3 + 0, RIPPER2_HANG_JUMP_RATE, NullRipper2, &s_Ripper2HangJump[3][1]},
        {RIPPER2_JUMP_R3 + 1, RIPPER2_HANG_JUMP_RATE, DoRipper2HangJF, &s_Ripper2HangJump[3][1]},
    },
    {
        {RIPPER2_JUMP_R4 + 0, RIPPER2_HANG_JUMP_RATE, NullRipper2, &s_Ripper2HangJump[4][1]},
        {RIPPER2_JUMP_R4 + 1, RIPPER2_HANG_JUMP_RATE, DoRipper2HangJF, &s_Ripper2HangJump[4][1]},
    }
};


STATE* sg_Ripper2HangJump[] =
{
    &s_Ripper2HangJump[0][0],
    &s_Ripper2HangJump[1][0],
    &s_Ripper2HangJump[2][0],
    &s_Ripper2HangJump[3][0],
    &s_Ripper2HangJump[4][0]
};

//////////////////////
//
// RIPPER2 HANG_FALL
//
//////////////////////

#define RIPPER2_FALL_RATE 25

STATE s_Ripper2HangFall[5][6] =
{
    {
        {RIPPER2_FALL_R0 + 0, RIPPER2_FALL_RATE, DoRipper2HangJF, &s_Ripper2HangFall[0][0]},
    },
    {
        {RIPPER2_FALL_R1 + 0, RIPPER2_FALL_RATE, DoRipper2HangJF, &s_Ripper2HangFall[1][0]},
    },
    {
        {RIPPER2_FALL_R2 + 0, RIPPER2_FALL_RATE, DoRipper2HangJF, &s_Ripper2HangFall[2][0]},
    },
    {
        {RIPPER2_FALL_R3 + 0, RIPPER2_FALL_RATE, DoRipper2HangJF, &s_Ripper2HangFall[3][0]},
    },
    {
        {RIPPER2_FALL_R4 + 0, RIPPER2_FALL_RATE, DoRipper2HangJF, &s_Ripper2HangFall[4][0]},
    }
};


STATE* sg_Ripper2HangFall[] =
{
    &s_Ripper2HangFall[0][0],
    &s_Ripper2HangFall[1][0],
    &s_Ripper2HangFall[2][0],
    &s_Ripper2HangFall[3][0],
    &s_Ripper2HangFall[4][0]
};



//////////////////////
//
// RIPPER2 DIE
//
//////////////////////

#define RIPPER2_DIE_RATE 18

STATE s_Ripper2Die[] =
{
    {RIPPER2_DIE + 0, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Die[1]},
    {RIPPER2_DIE + 1, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Die[2]},
    {RIPPER2_DIE + 2, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Die[3]},
    {RIPPER2_DIE + 3, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Die[4]},
    {RIPPER2_DIE + 4, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Die[5]},
    {RIPPER2_DIE + 5, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Die[6]},
    {RIPPER2_DIE + 6, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Die[7]},
    {RIPPER2_DEAD, RIPPER2_DIE_RATE, DoActorDebris, &s_Ripper2Die[7]},
};

#define RIPPER2_DEAD_RATE 8

STATE s_Ripper2Dead[] =
{
    {RIPPER2_DIE + 0, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Dead[1]},
    {RIPPER2_DIE + 1, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Dead[2]},
    {RIPPER2_DIE + 2, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Dead[3]},
    {RIPPER2_DIE + 3, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Dead[4]},
    {RIPPER2_DIE + 4, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Dead[5]},
    {RIPPER2_DIE + 5, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Dead[6]},
    {RIPPER2_DIE + 6, RIPPER2_DIE_RATE, NullRipper2, &s_Ripper2Dead[7]},
    {RIPPER2_DEAD, SF_QUICK_CALL, QueueFloorBlood, &s_Ripper2Dead[8]},
    {RIPPER2_DEAD, RIPPER2_DEAD_RATE, DoActorDebris, &s_Ripper2Dead[8]},
};

STATE* sg_Ripper2Die[] =
{
    s_Ripper2Die
};

STATE* sg_Ripper2Dead[] =
{
    s_Ripper2Dead
};

STATE s_Ripper2DeathJump[] =
{
    {RIPPER2_DIE + 0, RIPPER2_DIE_RATE, DoActorDeathMove, &s_Ripper2DeathJump[0]}
};

STATE s_Ripper2DeathFall[] =
{
    {RIPPER2_DIE + 1, RIPPER2_DIE_RATE, DoActorDeathMove, &s_Ripper2DeathFall[0]}
};


STATE* sg_Ripper2DeathJump[] =
{
    s_Ripper2DeathJump
};

STATE* sg_Ripper2DeathFall[] =
{
    s_Ripper2DeathFall
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


ACTOR_ACTION_SET Ripper2ActionSet =
{
    sg_Ripper2Stand,
    sg_Ripper2Run,
    sg_Ripper2Jump,
    sg_Ripper2Fall,
    nullptr,                               // sg_Ripper2Crawl,
    nullptr,                               // sg_Ripper2Swim,
    nullptr,                               // sg_Ripper2Fly,
    nullptr,                               // sg_Ripper2Rise,
    nullptr,                               // sg_Ripper2Sit,
    nullptr,                               // sg_Ripper2Look,
    nullptr,                               // climb
    sg_Ripper2Pain,
    sg_Ripper2Die,
    nullptr,                               // sg_Ripper2HariKari,
    sg_Ripper2Dead,
    sg_Ripper2DeathJump,
    sg_Ripper2DeathFall,
    {sg_Ripper2Swipe},
    {1024},
    {sg_Ripper2JumpAttack, sg_Ripper2Kong},
    {500, 1024},
    {sg_Ripper2Heart, sg_Ripper2Hang},
    nullptr,
    nullptr
};

ACTOR_ACTION_SET Ripper2BrownActionSet =
{
    sg_Ripper2Stand,
    sg_Ripper2Run,
    sg_Ripper2Jump,
    sg_Ripper2Fall,
    nullptr,                               // sg_Ripper2Crawl,
    nullptr,                               // sg_Ripper2Swim,
    nullptr,                               // sg_Ripper2Fly,
    nullptr,                               // sg_Ripper2Rise,
    nullptr,                               // sg_Ripper2Sit,
    nullptr,                               // sg_Ripper2Look,
    nullptr,                               // climb
    sg_Ripper2Pain,                      // pain
    sg_Ripper2Die,
    nullptr,                               // sg_Ripper2HariKari,
    sg_Ripper2Dead,
    sg_Ripper2DeathJump,
    sg_Ripper2DeathFall,
    {sg_Ripper2Swipe},
    {1024},
    {sg_Ripper2JumpAttack, sg_Ripper2Kong},
    {400, 1024},
    {sg_Ripper2Heart, sg_Ripper2Hang},
    nullptr,
    nullptr
};

int SetupRipper2(DSWActor* actor)
{
    ANIMATOR DoActorDecide;

    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, RIPPER2_RUN_R0, s_Ripper2Run[0]);
        actor->user.Health = HEALTH_RIPPER2;
    }

    ChangeState(actor, s_Ripper2Run[0]);
    actor->user.Attrib = &Ripper2Attrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.StateEnd = s_Ripper2Die;
    actor->user.Rot = sg_Ripper2Run;
    actor->spr.clipdist = 512 >> 2;  // This actor is bigger, needs bigger box.
    actor->spr.xrepeat = actor->spr.yrepeat = 55;

    if (actor->spr.pal == PALETTE_BROWN_RIPPER)
    {
        EnemyDefaults(actor, &Ripper2BrownActionSet, &Ripper2Personality);
        actor->spr.xrepeat += 40;
        actor->spr.yrepeat += 40;

        if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
            actor->user.Health = HEALTH_MOMMA_RIPPER;

        actor->spr.clipdist += 128 >> 2;
    }
    else
    {
        EnemyDefaults(actor, &Ripper2ActionSet, &Ripper2Personality);
    }

    actor->user.Flags |= (SPR_XFLIP_TOGGLE);

    return 0;
}

//
// HANGING - Jumping/Falling/Stationary
//

int InitRipper2Hang(DSWActor* actor)
{
    int dist;

    HitInfo hit{};

    bool Found = false;
    short dang, tang;

    for (dang = 0; dang < 2048; dang += 128)
    {
        tang = NORM_ANGLE(actor->int_ang() + dang);

        FAFhitscan(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - ActorSizeZ(actor), actor->sector(),  // Start position
                   bcos(tang),   // X vector of 3D ang
                   bsin(tang),   // Y vector of 3D ang
                   0,            // Z vector of 3D ang
                   hit, CLIPMASK_MISSILE);

        if (hit.hitSector == nullptr)
            continue;

        dist = Distance(actor->int_pos().X, actor->int_pos().Y, hit.hitpos.X, hit.hitpos.Y);

        if (hit.hitWall == nullptr || dist < 2000 || dist > 7000)
        {
            continue;
        }

        Found = true;
        actor->set_int_ang(tang);
        break;
    }

    if (!Found)
    {
        InitActorDecide(actor);
        return 0;
    }

    NewStateGroup(actor, sg_Ripper2HangJump);
    actor->user.StateFallOverride = sg_Ripper2HangFall;
    DoActorSetSpeed(actor, FAST_SPEED);

    //actor->user.jump_speed = -800;
    PickJumpMaxSpeed(actor, -(RandomRange(400)+100));

    actor->user.Flags |= (SPR_JUMPING);
    actor->user.Flags &= ~(SPR_FALLING);

    // set up individual actor jump gravity
    actor->user.jump_grav = 8;

    DoJump(actor);

    return 0;
}

int DoRipper2Hang(DSWActor* actor)
{
    if ((actor->user.WaitTics -= ACTORMOVETICS) > 0)
        return 0;

    NewStateGroup(actor, sg_Ripper2JumpAttack);
    // move to the 2nd frame - past the pause frame
    actor->user.Tics += actor->user.State->Tics;

    return 0;
}

int DoRipper2MoveHang(DSWActor* actor)
{
    int nx, ny;

    // Move while jumping
    nx = MulScale(actor->spr.xvel, bcos(actor->int_ang()), 14);
    ny = MulScale(actor->spr.xvel, bsin(actor->int_ang()), 14);

    // if cannot move the sprite
    if (!move_actor(actor, nx, ny, 0L))
    {
        if (actor->user.coll.type == kHitWall)
        {
            short hit_wall;
            short w, nw;

            // Don't keep clinging and going ever higher!
            if (abs(actor->int_pos().Z - actor->user.targetActor->int_pos().Z) > (4000<<4))
                return 0;

            NewStateGroup(actor, actor->user.ActorActionSet->Special[1]);
            if (RANDOM_P2(1024<<8)>>8 > 500)
                actor->user.WaitTics = ((RANDOM_P2(2 << 8) >> 8) * 120);
            else
                actor->user.WaitTics = 0; // Double jump

            // hang flush with the wall
            actor->set_int_ang(NORM_ANGLE(getangle(actor->user.coll.hitWall->delta()) - 512));

            return 0;
        }
    }

    return 0;
}


int DoRipper2HangJF(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoJump(actor);
        else
            DoFall(actor);
    }

    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        if (DoRipper2QuickJump(actor))
            return 0;

        InitActorDecide(actor);
    }

    DoRipper2MoveHang(actor);

    return 0;

}

//
// JUMP ATTACK
//

int DoRipper2BeginJumpAttack(DSWActor* actor)
{
    DSWActor* target = actor->user.targetActor;
    short tang;

    tang = getangle(target->int_pos().X - actor->int_pos().X, target->int_pos().Y - actor->int_pos().Y);

    // Always jump at player if mad.

	Collision coll = move_sprite(actor, bcos(tang, -7), bsin(tang, -7),
							   0, actor->user.ceiling_dist, actor->user.floor_dist, CLIPMASK_ACTOR, ACTORMOVETICS);
    if (coll.type != kHitNone)
        actor->set_int_ang(NORM_ANGLE((actor->int_ang() + 1024) + (RANDOM_NEG(256, 6) >> 6)));
    else
        actor->set_int_ang(NORM_ANGLE(tang));


    DoActorSetSpeed(actor, FAST_SPEED);

    //actor->user.jump_speed = -800;
    PickJumpMaxSpeed(actor, -(RandomRange(400)+100));

    actor->user.Flags |= (SPR_JUMPING);
    actor->user.Flags &= ~(SPR_FALLING);

    // set up individual actor jump gravity
    actor->user.jump_grav = 8;

    // if I didn't do this here they get stuck in the air sometimes
    DoActorZrange(actor);

    DoJump(actor);

    return 0;
}

int DoRipper2MoveJump(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoJump(actor);
        else
            DoFall(actor);
    }

    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        if (DoRipper2QuickJump(actor))
            return 0;

        InitActorDecide(actor);
    }

    DoRipper2MoveHang(actor);
    return 0;
}

//
// STD MOVEMENT
//

int DoRipper2QuickJump(DSWActor* actor)
{
    // Tests to see if ripper2 is on top of a player/enemy and then immediatly
    // does another jump

    DSWActor* low = actor->user.lowActor;
    if (low)
    {
        if ((low->spr.extra & SPRX_PLAYER_OR_ENEMY))
        {
            NewStateGroup(actor, sg_Ripper2JumpAttack);
            // move past the first state
            actor->user.Tics = 30;
            return true;
        }
    }

    return false;
}


int NullRipper2(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    DoActorSectorDamage(actor);

    return 0;
}


int DoRipper2Pain(DSWActor* actor)
{
    NullRipper2(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(actor);
    return 0;
}


int DoRipper2RipHeart(DSWActor* actor)
{
    DSWActor* target = actor->user.targetActor;
    NewStateGroup(actor, sg_Ripper2Heart);
    actor->user.WaitTics = 6 * 120;

    // player face ripper2
    target->set_int_ang(getangle(actor->int_pos().X - target->int_pos().X, actor->int_pos().Y - target->int_pos().Y));
    return 0;
}

int DoRipper2StandHeart(DSWActor* actor)
{
    NullRipper2(actor);

    if (!SoundValidAndActive(actor, CHAN_RipHeart))
        PlaySound(DIGI_RIPPER2HEARTOUT, actor, v3df_none, CHAN_RipHeart);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        NewStateGroup(actor, sg_Ripper2Run);
    return 0;
}

void Ripper2Hatch(DSWActor* actor)
{
	const int MAX_RIPPER2S = 1;
    short rip_ang[MAX_RIPPER2S];

    rip_ang[0] = RANDOM_P2(2048);
    //rip_ang[1] = NORM_ANGLE(rip_ang[0] + 1024 + (RANDOM_P2(512) - 256));

    for (int i = 0; i < MAX_RIPPER2S; i++)
    {
        auto actorNew = insertActor(actor->sector(), STAT_DEFAULT);
        ClearOwner(actorNew);
        actorNew->spr.pos = actor->spr.pos;

        actorNew->spr.xrepeat = actorNew->spr.yrepeat = 64;
        actorNew->set_int_ang(rip_ang[i]);
        actorNew->spr.pal = 0;
        actorNew->spr.shade = -10;
        SetupRipper2(actorNew);

        // make immediately active
        actorNew->user.Flags |= (SPR_ACTIVE);

        NewStateGroup(actorNew, actorNew->user.ActorActionSet->Jump);
        actorNew->user.ActorActionFunc = DoActorMoveJump;
        DoActorSetSpeed(actorNew, FAST_SPEED);
        PickJumpMaxSpeed(actorNew, -600);

        actorNew->user.Flags |= (SPR_JUMPING);
        actorNew->user.Flags &= ~(SPR_FALLING);

        actorNew->user.jump_grav = 8;

        // if I didn't do this here they get stuck in the air sometimes
        DoActorZrange(actorNew);

        DoJump(actorNew);
    }
}

int DoRipper2Move(DSWActor* actor)
{
    if (actor->spr.hitag == TAG_SWARMSPOT && actor->spr.lotag == 1)
        DoCheckSwarm(actor);

    if (actor->user.scale_speed)
    {
        DoScaleSprite(actor);
    }

    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoJump(actor);
        else
            DoFall(actor);
    }

    // if on a player/enemy sprite jump quickly
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        if (DoRipper2QuickJump(actor))
            return 0;

        KeepActorOnFloor(actor);
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        (*actor->user.ActorActionFunc)(actor);

    DoActorSectorDamage(actor);

    return 0;
}


int InitRipper2Charge(DSWActor* actor)
{
    DoActorSetSpeed(actor, FAST_SPEED);

    InitActorMoveCloser(actor);

    NewStateGroup(actor, sg_Ripper2RunFast);

    return 0;
}

int ChestRipper2(DSWActor* actor)
{
    PlaySound(DIGI_RIPPER2CHEST, actor, v3df_follow);
    return 0;
}


#include "saveable.h"

static saveable_code saveable_ripper2_code[] =
{
    SAVE_CODE(InitRipper2Hang),
    SAVE_CODE(DoRipper2Hang),
    SAVE_CODE(DoRipper2MoveHang),
    SAVE_CODE(DoRipper2HangJF),

    SAVE_CODE(DoRipper2BeginJumpAttack),
    SAVE_CODE(DoRipper2MoveJump),
    SAVE_CODE(DoRipper2QuickJump),
    SAVE_CODE(NullRipper2),
    SAVE_CODE(DoRipper2Pain),
    SAVE_CODE(DoRipper2RipHeart),
    SAVE_CODE(DoRipper2StandHeart),
    SAVE_CODE(Ripper2Hatch),
    SAVE_CODE(DoRipper2Move),
    SAVE_CODE(InitRipper2Charge),
    SAVE_CODE(ChestRipper2),
};

static saveable_data saveable_ripper2_data[] =
{
    SAVE_DATA(Ripper2Battle),
    SAVE_DATA(Ripper2Offense),
    SAVE_DATA(Ripper2Broadcast),
    SAVE_DATA(Ripper2Surprised),
    SAVE_DATA(Ripper2Evasive),
    SAVE_DATA(Ripper2LostTarget),
    SAVE_DATA(Ripper2CloseRange),

    SAVE_DATA(Ripper2Personality),

    SAVE_DATA(Ripper2Attrib),

    SAVE_DATA(s_Ripper2Run),
    SAVE_DATA(sg_Ripper2Run),
    SAVE_DATA(s_Ripper2RunFast),
    SAVE_DATA(sg_Ripper2RunFast),
    SAVE_DATA(s_Ripper2Stand),
    SAVE_DATA(sg_Ripper2Stand),
    SAVE_DATA(s_Ripper2Swipe),
    SAVE_DATA(sg_Ripper2Swipe),
    SAVE_DATA(s_Ripper2Kong),
    SAVE_DATA(sg_Ripper2Kong),
    SAVE_DATA(s_Ripper2Heart),
    SAVE_DATA(sg_Ripper2Heart),
    SAVE_DATA(s_Ripper2Hang),
    SAVE_DATA(sg_Ripper2Hang),
    SAVE_DATA(s_Ripper2Pain),
    SAVE_DATA(sg_Ripper2Pain),
    SAVE_DATA(s_Ripper2Jump),
    SAVE_DATA(sg_Ripper2Jump),
    SAVE_DATA(s_Ripper2Fall),
    SAVE_DATA(sg_Ripper2Fall),
    SAVE_DATA(s_Ripper2JumpAttack),
    SAVE_DATA(sg_Ripper2JumpAttack),
    SAVE_DATA(s_Ripper2HangJump),
    SAVE_DATA(sg_Ripper2HangJump),
    SAVE_DATA(s_Ripper2HangFall),
    SAVE_DATA(sg_Ripper2HangFall),
    SAVE_DATA(s_Ripper2Die),
    SAVE_DATA(s_Ripper2Dead),
    SAVE_DATA(sg_Ripper2Die),
    SAVE_DATA(sg_Ripper2Dead),
    SAVE_DATA(s_Ripper2DeathJump),
    SAVE_DATA(s_Ripper2DeathFall),
    SAVE_DATA(sg_Ripper2DeathJump),
    SAVE_DATA(sg_Ripper2DeathFall),

    SAVE_DATA(Ripper2ActionSet),
    SAVE_DATA(Ripper2BrownActionSet),
};

saveable_module saveable_ripper2 =
{
    // code
    saveable_ripper2_code,
    SIZ(saveable_ripper2_code),

    // data
    saveable_ripper2_data,
    SIZ(saveable_ripper2_data)
};
END_SW_NS
