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
#include "actor.h"
#include "sprite.h"
#include "track.h"

ANIMATOR InitRipperHang;
ANIMATOR DoActorMoveJump;
ANIMATOR DoRipperMoveJump;
ANIMATOR DoRipperHangJF;
ANIMATOR DoRipperQuickJump;

DECISION RipperBattle[] =
{
    {748, InitActorMoveCloser},
    {750, InitActorAlertNoise},
//    {900, InitRipperHang},
    {755, InitActorAttackNoise},
    {1024, InitActorAttack}
};

DECISION RipperOffense[] =
{
    {700, InitActorMoveCloser},
    {710, InitActorAlertNoise},
    {1024, InitActorAttack}
};

DECISION RipperBroadcast[] =
{
    {3, InitActorAlertNoise},
    {6, InitActorAmbientNoise},
    {1024, InitActorDecide}
};

DECISION RipperSurprised[] =
{
    {30, InitRipperHang},
    {701, InitActorMoveCloser},
    {1024, InitActorDecide}
};

DECISION RipperEvasive[] =
{
    {6, InitRipperHang},
    {1024, NULL}
};

DECISION RipperLostTarget[] =
{
    {980, InitActorFindPlayer},
    {1024, InitActorWanderAround}
};

DECISION RipperCloseRange[] =
{
    {900,   InitActorAttack             },
    {1024,  InitActorReposition         }
};

PERSONALITY RipperPersonality =
{
    RipperBattle,
    RipperOffense,
    RipperBroadcast,
    RipperSurprised,
    RipperEvasive,
    RipperLostTarget,
    RipperCloseRange,
    RipperCloseRange
};

ATTRIBUTE RipperAttrib =
{
    {200, 220, 240, 280},               // Speeds
    {5, 0, -2, -4},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_RIPPERAMBIENT, DIGI_RIPPERALERT, DIGI_RIPPERATTACK,
        DIGI_RIPPERPAIN, DIGI_RIPPERSCREAM, DIGI_RIPPERHEARTOUT,
        0,0,0,0
    }
};

//////////////////////
//
// RIPPER RUN
//
//////////////////////

#define RIPPER_RUN_RATE 16

ANIMATOR DoRipperMove, NullRipper, DoActorDebris;

STATE s_RipperRun[5][4] =
{
    {
        {RIPPER_RUN_R0 + 0, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[0][1]},
        {RIPPER_RUN_R0 + 1, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[0][2]},
        {RIPPER_RUN_R0 + 2, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[0][3]},
        {RIPPER_RUN_R0 + 3, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[0][0]},
    },
    {
        {RIPPER_RUN_R1 + 0, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[1][1]},
        {RIPPER_RUN_R1 + 1, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[1][2]},
        {RIPPER_RUN_R1 + 2, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[1][3]},
        {RIPPER_RUN_R1 + 3, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[1][0]},
    },
    {
        {RIPPER_RUN_R2 + 0, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[2][1]},
        {RIPPER_RUN_R2 + 1, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[2][2]},
        {RIPPER_RUN_R2 + 2, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[2][3]},
        {RIPPER_RUN_R2 + 3, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[2][0]},
    },
    {
        {RIPPER_RUN_R3 + 0, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[3][1]},
        {RIPPER_RUN_R3 + 1, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[3][2]},
        {RIPPER_RUN_R3 + 2, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[3][3]},
        {RIPPER_RUN_R3 + 3, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[3][0]},
    },
    {
        {RIPPER_RUN_R4 + 0, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[4][1]},
        {RIPPER_RUN_R4 + 1, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[4][2]},
        {RIPPER_RUN_R4 + 2, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[4][3]},
        {RIPPER_RUN_R4 + 3, RIPPER_RUN_RATE | SF_TIC_ADJUST, DoRipperMove, &s_RipperRun[4][0]},
    }
};


STATEp sg_RipperRun[] =
{
    &s_RipperRun[0][0],
    &s_RipperRun[1][0],
    &s_RipperRun[2][0],
    &s_RipperRun[3][0],
    &s_RipperRun[4][0]
};

//////////////////////
//
// RIPPER STAND
//
//////////////////////

#define RIPPER_STAND_RATE 12

STATE s_RipperStand[5][1] =
{
    {
        {RIPPER_STAND_R0 + 0, RIPPER_STAND_RATE, DoRipperMove, &s_RipperStand[0][0]},
    },
    {
        {RIPPER_STAND_R1 + 0, RIPPER_STAND_RATE, DoRipperMove, &s_RipperStand[1][0]},
    },
    {
        {RIPPER_STAND_R2 + 0, RIPPER_STAND_RATE, DoRipperMove, &s_RipperStand[2][0]},
    },
    {
        {RIPPER_STAND_R3 + 0, RIPPER_STAND_RATE, DoRipperMove, &s_RipperStand[3][0]},
    },
    {
        {RIPPER_STAND_R4 + 0, RIPPER_STAND_RATE, DoRipperMove, &s_RipperStand[4][0]},
    },
};


STATEp sg_RipperStand[] =
{
    s_RipperStand[0],
    s_RipperStand[1],
    s_RipperStand[2],
    s_RipperStand[3],
    s_RipperStand[4]
};

//////////////////////
//
// RIPPER SWIPE
//
//////////////////////

#define RIPPER_SWIPE_RATE 8
ANIMATOR InitActorDecide;
ANIMATOR InitRipperSlash;

STATE s_RipperSwipe[5][8] =
{
    {
        {RIPPER_SWIPE_R0 + 0, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[0][1]},
        {RIPPER_SWIPE_R0 + 1, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[0][2]},
        {RIPPER_SWIPE_R0 + 1, 0 | SF_QUICK_CALL, InitRipperSlash, &s_RipperSwipe[0][3]},
        {RIPPER_SWIPE_R0 + 2, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[0][4]},
        {RIPPER_SWIPE_R0 + 3, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[0][5]},
        {RIPPER_SWIPE_R0 + 3, 0 | SF_QUICK_CALL, InitRipperSlash, &s_RipperSwipe[0][6]},
        {RIPPER_SWIPE_R0 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_RipperSwipe[0][7]},
        {RIPPER_SWIPE_R0 + 3, RIPPER_SWIPE_RATE, DoRipperMove, &s_RipperSwipe[0][7]},
    },
    {
        {RIPPER_SWIPE_R1 + 0, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[1][1]},
        {RIPPER_SWIPE_R1 + 1, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[1][2]},
        {RIPPER_SWIPE_R1 + 1, 0 | SF_QUICK_CALL, InitRipperSlash, &s_RipperSwipe[1][3]},
        {RIPPER_SWIPE_R1 + 2, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[1][4]},
        {RIPPER_SWIPE_R1 + 3, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[1][5]},
        {RIPPER_SWIPE_R1 + 3, 0 | SF_QUICK_CALL, InitRipperSlash, &s_RipperSwipe[1][6]},
        {RIPPER_SWIPE_R1 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_RipperSwipe[1][7]},
        {RIPPER_SWIPE_R1 + 3, RIPPER_SWIPE_RATE, DoRipperMove, &s_RipperSwipe[1][7]},
    },
    {
        {RIPPER_SWIPE_R2 + 0, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[2][1]},
        {RIPPER_SWIPE_R2 + 1, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[2][2]},
        {RIPPER_SWIPE_R2 + 1, 0 | SF_QUICK_CALL, InitRipperSlash, &s_RipperSwipe[2][3]},
        {RIPPER_SWIPE_R2 + 2, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[2][4]},
        {RIPPER_SWIPE_R2 + 3, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[2][5]},
        {RIPPER_SWIPE_R2 + 3, 0 | SF_QUICK_CALL, InitRipperSlash, &s_RipperSwipe[2][6]},
        {RIPPER_SWIPE_R2 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_RipperSwipe[2][7]},
        {RIPPER_SWIPE_R2 + 3, RIPPER_SWIPE_RATE, DoRipperMove, &s_RipperSwipe[2][7]},
    },
    {
        {RIPPER_SWIPE_R3 + 0, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[3][1]},
        {RIPPER_SWIPE_R3 + 1, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[3][2]},
        {RIPPER_SWIPE_R3 + 1, 0 | SF_QUICK_CALL, InitRipperSlash, &s_RipperSwipe[3][3]},
        {RIPPER_SWIPE_R3 + 2, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[3][4]},
        {RIPPER_SWIPE_R3 + 3, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[3][5]},
        {RIPPER_SWIPE_R3 + 3, 0 | SF_QUICK_CALL, InitRipperSlash, &s_RipperSwipe[3][6]},
        {RIPPER_SWIPE_R3 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_RipperSwipe[3][7]},
        {RIPPER_SWIPE_R3 + 3, RIPPER_SWIPE_RATE, DoRipperMove, &s_RipperSwipe[3][7]},
    },
    {
        {RIPPER_SWIPE_R4 + 0, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[4][1]},
        {RIPPER_SWIPE_R4 + 1, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[4][2]},
        {RIPPER_SWIPE_R4 + 1, 0 | SF_QUICK_CALL, InitRipperSlash, &s_RipperSwipe[4][3]},
        {RIPPER_SWIPE_R4 + 2, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[4][4]},
        {RIPPER_SWIPE_R4 + 3, RIPPER_SWIPE_RATE, NullRipper, &s_RipperSwipe[4][5]},
        {RIPPER_SWIPE_R4 + 3, 0 | SF_QUICK_CALL, InitRipperSlash, &s_RipperSwipe[4][6]},
        {RIPPER_SWIPE_R4 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_RipperSwipe[4][7]},
        {RIPPER_SWIPE_R4 + 3, RIPPER_SWIPE_RATE, DoRipperMove, &s_RipperSwipe[4][7]},
    }
};


STATEp sg_RipperSwipe[] =
{
    &s_RipperSwipe[0][0],
    &s_RipperSwipe[1][0],
    &s_RipperSwipe[2][0],
    &s_RipperSwipe[3][0],
    &s_RipperSwipe[4][0]
};


//////////////////////
//
// RIPPER SPEW
//
//////////////////////

#define RIPPER_SPEW_RATE 8
ANIMATOR InitActorDecide;
ANIMATOR InitCoolgFire;

STATE s_RipperSpew[5][7] =
{
    {
        {RIPPER_SWIPE_R0 + 0, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[0][1]},
        {RIPPER_SWIPE_R0 + 1, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[0][2]},
        {RIPPER_SWIPE_R0 + 1, 0 | SF_QUICK_CALL, InitCoolgFire, &s_RipperSpew[0][3]},
        {RIPPER_SWIPE_R0 + 2, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[0][4]},
        {RIPPER_SWIPE_R0 + 3, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[0][5]},
        {RIPPER_SWIPE_R0 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_RipperSpew[0][6]},
        {RIPPER_SWIPE_R0 + 3, RIPPER_SPEW_RATE, DoRipperMove, &s_RipperSpew[0][6]},
    },
    {
        {RIPPER_SWIPE_R1 + 0, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[1][1]},
        {RIPPER_SWIPE_R1 + 1, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[1][2]},
        {RIPPER_SWIPE_R1 + 1, 0 | SF_QUICK_CALL, InitCoolgFire, &s_RipperSpew[1][3]},
        {RIPPER_SWIPE_R1 + 2, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[1][4]},
        {RIPPER_SWIPE_R1 + 3, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[1][5]},
        {RIPPER_SWIPE_R1 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_RipperSpew[1][6]},
        {RIPPER_SWIPE_R1 + 3, RIPPER_SPEW_RATE, DoRipperMove, &s_RipperSpew[1][6]},
    },
    {
        {RIPPER_SWIPE_R2 + 0, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[2][1]},
        {RIPPER_SWIPE_R2 + 1, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[2][2]},
        {RIPPER_SWIPE_R2 + 1, 0 | SF_QUICK_CALL, InitCoolgFire, &s_RipperSpew[2][3]},
        {RIPPER_SWIPE_R2 + 2, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[2][4]},
        {RIPPER_SWIPE_R2 + 3, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[2][5]},
        {RIPPER_SWIPE_R2 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_RipperSpew[2][6]},
        {RIPPER_SWIPE_R2 + 3, RIPPER_SPEW_RATE, DoRipperMove, &s_RipperSpew[2][6]},
    },
    {
        {RIPPER_SWIPE_R3 + 0, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[3][1]},
        {RIPPER_SWIPE_R3 + 1, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[3][2]},
        {RIPPER_SWIPE_R3 + 1, 0 | SF_QUICK_CALL, InitCoolgFire, &s_RipperSpew[3][3]},
        {RIPPER_SWIPE_R3 + 2, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[3][4]},
        {RIPPER_SWIPE_R3 + 3, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[3][5]},
        {RIPPER_SWIPE_R3 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_RipperSpew[3][6]},
        {RIPPER_SWIPE_R3 + 3, RIPPER_SPEW_RATE, DoRipperMove, &s_RipperSpew[3][6]},
    },
    {
        {RIPPER_SWIPE_R4 + 0, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[4][1]},
        {RIPPER_SWIPE_R4 + 1, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[4][2]},
        {RIPPER_SWIPE_R4 + 1, 0 | SF_QUICK_CALL, InitCoolgFire, &s_RipperSpew[4][3]},
        {RIPPER_SWIPE_R4 + 2, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[4][4]},
        {RIPPER_SWIPE_R4 + 3, RIPPER_SPEW_RATE, NullRipper, &s_RipperSpew[4][5]},
        {RIPPER_SWIPE_R4 + 3, 0 | SF_QUICK_CALL, InitActorDecide, &s_RipperSpew[4][6]},
        {RIPPER_SWIPE_R4 + 3, RIPPER_SPEW_RATE, DoRipperMove, &s_RipperSpew[4][6]},
    }
};


STATEp sg_RipperSpew[] =
{
    &s_RipperSpew[0][0],
    &s_RipperSpew[1][0],
    &s_RipperSpew[2][0],
    &s_RipperSpew[3][0],
    &s_RipperSpew[4][0]
};


//////////////////////
//
// RIPPER HEART - show players heart
//
//////////////////////

#define RIPPER_HEART_RATE 14
ANIMATOR DoRipperStandHeart;

STATE s_RipperHeart[5][4] =
{
    {
        {RIPPER_HEART_R0 + 0, RIPPER_HEART_RATE, DoRipperStandHeart, &s_RipperHeart[0][0]},
    },
    {
        {RIPPER_HEART_R1 + 0, RIPPER_HEART_RATE, DoRipperStandHeart, &s_RipperHeart[1][0]},
    },
    {
        {RIPPER_HEART_R2 + 0, RIPPER_HEART_RATE, DoRipperStandHeart, &s_RipperHeart[2][0]},
    },
    {
        {RIPPER_HEART_R3 + 0, RIPPER_HEART_RATE, DoRipperStandHeart, &s_RipperHeart[3][0]},
    },
    {
        {RIPPER_HEART_R4 + 0, RIPPER_HEART_RATE, DoRipperStandHeart, &s_RipperHeart[4][0]},
    }
};

STATEp sg_RipperHeart[] =
{
    &s_RipperHeart[0][0],
    &s_RipperHeart[1][0],
    &s_RipperHeart[2][0],
    &s_RipperHeart[3][0],
    &s_RipperHeart[4][0]
};

//////////////////////
//
// RIPPER HANG
//
//////////////////////

#define RIPPER_HANG_RATE 14
ANIMATOR DoRipperHang;

STATE s_RipperHang[5][4] =
{
    {
        {RIPPER_HANG_R0 + 0, RIPPER_HANG_RATE, DoRipperHang, &s_RipperHang[0][0]},
    },
    {
        {RIPPER_HANG_R1 + 0, RIPPER_HANG_RATE, DoRipperHang, &s_RipperHang[1][0]},
    },
    {
        {RIPPER_HANG_R2 + 0, RIPPER_HANG_RATE, DoRipperHang, &s_RipperHang[2][0]},
    },
    {
        {RIPPER_HANG_R3 + 0, RIPPER_HANG_RATE, DoRipperHang, &s_RipperHang[3][0]},
    },
    {
        {RIPPER_HANG_R4 + 0, RIPPER_HANG_RATE, DoRipperHang, &s_RipperHang[4][0]},
    }
};


STATEp sg_RipperHang[] =
{
    &s_RipperHang[0][0],
    &s_RipperHang[1][0],
    &s_RipperHang[2][0],
    &s_RipperHang[3][0],
    &s_RipperHang[4][0]
};


//////////////////////
//
// RIPPER PAIN
//
//////////////////////

#define RIPPER_PAIN_RATE 38
ANIMATOR DoRipperPain;

STATE s_RipperPain[5][1] =
{
    {
        {RIPPER_JUMP_R0 + 0, RIPPER_PAIN_RATE, DoRipperPain, &s_RipperPain[0][0]},
    },
    {
        {RIPPER_JUMP_R0 + 0, RIPPER_PAIN_RATE, DoRipperPain, &s_RipperPain[1][0]},
    },
    {
        {RIPPER_JUMP_R0 + 0, RIPPER_PAIN_RATE, DoRipperPain, &s_RipperPain[2][0]},
    },
    {
        {RIPPER_JUMP_R0 + 0, RIPPER_PAIN_RATE, DoRipperPain, &s_RipperPain[3][0]},
    },
    {
        {RIPPER_JUMP_R0 + 0, RIPPER_PAIN_RATE, DoRipperPain, &s_RipperPain[4][0]},
    }
};

STATEp sg_RipperPain[] =
{
    &s_RipperPain[0][0],
    &s_RipperPain[1][0],
    &s_RipperPain[2][0],
    &s_RipperPain[3][0],
    &s_RipperPain[4][0]
};

//////////////////////
//
// RIPPER JUMP
//
//////////////////////

#define RIPPER_JUMP_RATE 25

STATE s_RipperJump[5][6] =
{
    {
        {RIPPER_JUMP_R0 + 0, RIPPER_JUMP_RATE, NullRipper, &s_RipperJump[0][1]},
        {RIPPER_JUMP_R0 + 1, RIPPER_JUMP_RATE, DoRipperMoveJump, &s_RipperJump[0][1]},
    },
    {
        {RIPPER_JUMP_R1 + 0, RIPPER_JUMP_RATE, NullRipper, &s_RipperJump[1][1]},
        {RIPPER_JUMP_R1 + 1, RIPPER_JUMP_RATE, DoRipperMoveJump, &s_RipperJump[1][1]},
    },
    {
        {RIPPER_JUMP_R2 + 0, RIPPER_JUMP_RATE, NullRipper, &s_RipperJump[2][1]},
        {RIPPER_JUMP_R2 + 1, RIPPER_JUMP_RATE, DoRipperMoveJump, &s_RipperJump[2][1]},
    },
    {
        {RIPPER_JUMP_R3 + 0, RIPPER_JUMP_RATE, NullRipper, &s_RipperJump[3][1]},
        {RIPPER_JUMP_R3 + 1, RIPPER_JUMP_RATE, DoRipperMoveJump, &s_RipperJump[3][1]},
    },
    {
        {RIPPER_JUMP_R4 + 0, RIPPER_JUMP_RATE, NullRipper, &s_RipperJump[4][1]},
        {RIPPER_JUMP_R4 + 1, RIPPER_JUMP_RATE, DoRipperMoveJump, &s_RipperJump[4][1]},
    }
};


STATEp sg_RipperJump[] =
{
    &s_RipperJump[0][0],
    &s_RipperJump[1][0],
    &s_RipperJump[2][0],
    &s_RipperJump[3][0],
    &s_RipperJump[4][0]
};


//////////////////////
//
// RIPPER FALL
//
//////////////////////

#define RIPPER_FALL_RATE 25

STATE s_RipperFall[5][6] =
{
    {
        {RIPPER_FALL_R0 + 0, RIPPER_FALL_RATE, DoRipperMoveJump, &s_RipperFall[0][0]},
    },
    {
        {RIPPER_FALL_R1 + 0, RIPPER_FALL_RATE, DoRipperMoveJump, &s_RipperFall[1][0]},
    },
    {
        {RIPPER_FALL_R2 + 0, RIPPER_FALL_RATE, DoRipperMoveJump, &s_RipperFall[2][0]},
    },
    {
        {RIPPER_FALL_R3 + 0, RIPPER_FALL_RATE, DoRipperMoveJump, &s_RipperFall[3][0]},
    },
    {
        {RIPPER_FALL_R4 + 0, RIPPER_FALL_RATE, DoRipperMoveJump, &s_RipperFall[4][0]},
    }
};


STATEp sg_RipperFall[] =
{
    &s_RipperFall[0][0],
    &s_RipperFall[1][0],
    &s_RipperFall[2][0],
    &s_RipperFall[3][0],
    &s_RipperFall[4][0]
};


//////////////////////
//
// RIPPER JUMP ATTACK
//
//////////////////////

#define RIPPER_JUMP_ATTACK_RATE 35
int DoRipperBeginJumpAttack(short SpriteNum);

STATE s_RipperJumpAttack[5][6] =
{
    {
        {RIPPER_JUMP_R0 + 0, RIPPER_JUMP_ATTACK_RATE, NullRipper, &s_RipperJumpAttack[0][1]},
        {RIPPER_JUMP_R0 + 0, 0 | SF_QUICK_CALL, DoRipperBeginJumpAttack, &s_RipperJumpAttack[0][2]},
        {RIPPER_JUMP_R0 + 1, RIPPER_JUMP_ATTACK_RATE, DoRipperMoveJump, &s_RipperJumpAttack[0][2]},
    },
    {
        {RIPPER_JUMP_R1 + 0, RIPPER_JUMP_ATTACK_RATE, NullRipper, &s_RipperJumpAttack[1][1]},
        {RIPPER_JUMP_R1 + 0, 0 | SF_QUICK_CALL, DoRipperBeginJumpAttack, &s_RipperJumpAttack[1][2]},
        {RIPPER_JUMP_R1 + 1, RIPPER_JUMP_ATTACK_RATE, DoRipperMoveJump, &s_RipperJumpAttack[1][2]},
    },
    {
        {RIPPER_JUMP_R2 + 0, RIPPER_JUMP_ATTACK_RATE, NullRipper, &s_RipperJumpAttack[2][1]},
        {RIPPER_JUMP_R2 + 0, 0 | SF_QUICK_CALL, DoRipperBeginJumpAttack, &s_RipperJumpAttack[2][2]},
        {RIPPER_JUMP_R2 + 1, RIPPER_JUMP_ATTACK_RATE, DoRipperMoveJump, &s_RipperJumpAttack[2][2]},
    },
    {
        {RIPPER_JUMP_R3 + 0, RIPPER_JUMP_ATTACK_RATE, NullRipper, &s_RipperJumpAttack[3][1]},
        {RIPPER_JUMP_R3 + 0, 0 | SF_QUICK_CALL, DoRipperBeginJumpAttack, &s_RipperJumpAttack[3][2]},
        {RIPPER_JUMP_R3 + 1, RIPPER_JUMP_ATTACK_RATE, DoRipperMoveJump, &s_RipperJumpAttack[3][2]},
    },
    {
        {RIPPER_JUMP_R4 + 0, RIPPER_JUMP_ATTACK_RATE, NullRipper, &s_RipperJumpAttack[4][1]},
        {RIPPER_JUMP_R4 + 0, 0 | SF_QUICK_CALL, DoRipperBeginJumpAttack, &s_RipperJumpAttack[4][2]},
        {RIPPER_JUMP_R4 + 1, RIPPER_JUMP_ATTACK_RATE, DoRipperMoveJump, &s_RipperJumpAttack[4][2]},
    }
};


STATEp sg_RipperJumpAttack[] =
{
    &s_RipperJumpAttack[0][0],
    &s_RipperJumpAttack[1][0],
    &s_RipperJumpAttack[2][0],
    &s_RipperJumpAttack[3][0],
    &s_RipperJumpAttack[4][0]
};


//////////////////////
//
// RIPPER HANG_JUMP
//
//////////////////////

#define RIPPER_HANG_JUMP_RATE 20

STATE s_RipperHangJump[5][6] =
{
    {
        {RIPPER_JUMP_R0 + 0, RIPPER_HANG_JUMP_RATE, NullRipper, &s_RipperHangJump[0][1]},
        {RIPPER_JUMP_R0 + 1, RIPPER_HANG_JUMP_RATE, DoRipperHangJF, &s_RipperHangJump[0][1]},
    },
    {
        {RIPPER_JUMP_R1 + 0, RIPPER_HANG_JUMP_RATE, NullRipper, &s_RipperHangJump[1][1]},
        {RIPPER_JUMP_R1 + 1, RIPPER_HANG_JUMP_RATE, DoRipperHangJF, &s_RipperHangJump[1][1]},
    },
    {
        {RIPPER_JUMP_R2 + 0, RIPPER_HANG_JUMP_RATE, NullRipper, &s_RipperHangJump[2][1]},
        {RIPPER_JUMP_R2 + 1, RIPPER_HANG_JUMP_RATE, DoRipperHangJF, &s_RipperHangJump[2][1]},
    },
    {
        {RIPPER_JUMP_R3 + 0, RIPPER_HANG_JUMP_RATE, NullRipper, &s_RipperHangJump[3][1]},
        {RIPPER_JUMP_R3 + 1, RIPPER_HANG_JUMP_RATE, DoRipperHangJF, &s_RipperHangJump[3][1]},
    },
    {
        {RIPPER_JUMP_R4 + 0, RIPPER_HANG_JUMP_RATE, NullRipper, &s_RipperHangJump[4][1]},
        {RIPPER_JUMP_R4 + 1, RIPPER_HANG_JUMP_RATE, DoRipperHangJF, &s_RipperHangJump[4][1]},
    }
};


STATEp sg_RipperHangJump[] =
{
    &s_RipperHangJump[0][0],
    &s_RipperHangJump[1][0],
    &s_RipperHangJump[2][0],
    &s_RipperHangJump[3][0],
    &s_RipperHangJump[4][0]
};

//////////////////////
//
// RIPPER HANG_FALL
//
//////////////////////

#define RIPPER_FALL_RATE 25

STATE s_RipperHangFall[5][6] =
{
    {
        {RIPPER_FALL_R0 + 0, RIPPER_FALL_RATE, DoRipperHangJF, &s_RipperHangFall[0][0]},
    },
    {
        {RIPPER_FALL_R1 + 0, RIPPER_FALL_RATE, DoRipperHangJF, &s_RipperHangFall[1][0]},
    },
    {
        {RIPPER_FALL_R2 + 0, RIPPER_FALL_RATE, DoRipperHangJF, &s_RipperHangFall[2][0]},
    },
    {
        {RIPPER_FALL_R3 + 0, RIPPER_FALL_RATE, DoRipperHangJF, &s_RipperHangFall[3][0]},
    },
    {
        {RIPPER_FALL_R4 + 0, RIPPER_FALL_RATE, DoRipperHangJF, &s_RipperHangFall[4][0]},
    }
};


STATEp sg_RipperHangFall[] =
{
    &s_RipperHangFall[0][0],
    &s_RipperHangFall[1][0],
    &s_RipperHangFall[2][0],
    &s_RipperHangFall[3][0],
    &s_RipperHangFall[4][0]
};



//////////////////////
//
// RIPPER DIE
//
//////////////////////

#define RIPPER_DIE_RATE 16

STATE s_RipperDie[] =
{
    {RIPPER_DIE + 0, RIPPER_DIE_RATE, NullRipper, &s_RipperDie[1]},
    {RIPPER_DIE + 1, RIPPER_DIE_RATE, NullRipper, &s_RipperDie[2]},
    {RIPPER_DIE + 2, RIPPER_DIE_RATE, NullRipper, &s_RipperDie[3]},
    {RIPPER_DIE + 3, RIPPER_DIE_RATE, NullRipper, &s_RipperDie[4]},
    {RIPPER_DEAD, RIPPER_DIE_RATE, DoActorDebris, &s_RipperDie[4]},
};

#define RIPPER_DEAD_RATE 8

STATE s_RipperDead[] =
{
    {RIPPER_DIE + 2, RIPPER_DEAD_RATE, NullAnimator, &s_RipperDead[1]},
    {RIPPER_DIE + 3, RIPPER_DEAD_RATE, NullAnimator, &s_RipperDead[2]},
    {RIPPER_DEAD, SF_QUICK_CALL, QueueFloorBlood, &s_RipperDead[3]},
    {RIPPER_DEAD, RIPPER_DEAD_RATE, DoActorDebris, &s_RipperDead[3]},
};

STATEp sg_RipperDie[] =
{
    s_RipperDie
};

STATEp sg_RipperDead[] =
{
    s_RipperDead
};

STATE s_RipperDeathJump[] =
{
    {RIPPER_DIE + 0, RIPPER_DIE_RATE, DoActorDeathMove, &s_RipperDeathJump[0]}
};

STATE s_RipperDeathFall[] =
{
    {RIPPER_DIE + 1, RIPPER_DIE_RATE, DoActorDeathMove, &s_RipperDeathFall[0]}
};


STATEp sg_RipperDeathJump[] =
{
    s_RipperDeathJump
};

STATEp sg_RipperDeathFall[] =
{
    s_RipperDeathFall
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


ACTOR_ACTION_SET RipperActionSet =
{
    sg_RipperStand,
    sg_RipperRun,
    sg_RipperJump,
    sg_RipperFall,
    NULL,                               // sg_RipperCrawl,
    NULL,                               // sg_RipperSwim,
    NULL,                               // sg_RipperFly,
    NULL,                               // sg_RipperRise,
    NULL,                               // sg_RipperSit,
    NULL,                               // sg_RipperLook,
    NULL,                               // climb
    sg_RipperPain,
    sg_RipperDie,
    NULL,                               // sg_RipperHariKari,
    sg_RipperDead,
    sg_RipperDeathJump,
    sg_RipperDeathFall,
    {sg_RipperSwipe,sg_RipperSpew},
    {800,1024},
    {sg_RipperJumpAttack, sg_RipperSpew},
    {400, 1024},
    {sg_RipperHeart, sg_RipperHang},
    NULL,
    NULL
};

ACTOR_ACTION_SET RipperBrownActionSet =
{
    sg_RipperStand,
    sg_RipperRun,
    sg_RipperJump,
    sg_RipperFall,
    NULL,                               // sg_RipperCrawl,
    NULL,                               // sg_RipperSwim,
    NULL,                               // sg_RipperFly,
    NULL,                               // sg_RipperRise,
    NULL,                               // sg_RipperSit,
    NULL,                               // sg_RipperLook,
    NULL,                               // climb
    sg_RipperPain,                      // pain
    sg_RipperDie,
    NULL,                               // sg_RipperHariKari,
    sg_RipperDead,
    sg_RipperDeathJump,
    sg_RipperDeathFall,
    {sg_RipperSwipe},
    {1024},
    {sg_RipperJumpAttack, sg_RipperSwipe},
    {800, 1024},
    {sg_RipperHeart, sg_RipperHang},
    NULL,
    NULL
};

int
SetupRipper(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum, RIPPER_RUN_R0, s_RipperRun[0]);
        u->Health = HEALTH_RIPPER/2; // Baby rippers are weaker
    }

    ChangeState(SpriteNum, s_RipperRun[0]);
    u->Attrib = &RipperAttrib;
    DoActorSetSpeed(SpriteNum, FAST_SPEED);
    u->StateEnd = s_RipperDie;
    u->Rot = sg_RipperRun;
    sp->xrepeat = 64;
    sp->yrepeat = 64;

    if (sp->pal == PALETTE_BROWN_RIPPER)
    {
        EnemyDefaults(SpriteNum, &RipperBrownActionSet, &RipperPersonality);
        sp->xrepeat = 106;
        sp->yrepeat = 90;

        if (!TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
            u->Health = HEALTH_MOMMA_RIPPER;

        sp->clipdist += 128 >> 2;
    }
    else
    {
        EnemyDefaults(SpriteNum, &RipperActionSet, &RipperPersonality);
    }

    SET(u->Flags, SPR_XFLIP_TOGGLE);

    return 0;
}

int
GetJumpHeight(short jump_speed, short jump_grav)
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
PickJumpSpeed(short SpriteNum, int pix_height)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    //ASSERT(pix_height < 128);

    u->jump_speed = -600;
    u->jump_grav = 8;

    while (TRUE)
    {
        if (GetJumpHeight(u->jump_speed, u->jump_grav) > pix_height + 20)
            break;

        u->jump_speed -= 100;

        ASSERT(u->jump_speed > -3000);
    }

    return u->jump_speed;
}


int
PickJumpMaxSpeed(short SpriteNum, short max_speed)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int zh;

    ASSERT(max_speed < 0);

    u->jump_speed = max_speed;
    u->jump_grav = 8;

    zh = SPRITEp_TOS(sp);

    while (TRUE)
    {
        if (zh - Z(GetJumpHeight(u->jump_speed, u->jump_grav)) - Z(16) > u->hiz)
            break;

        u->jump_speed += 100;

        if (u->jump_speed > -200)
            break;
    }

    return u->jump_speed;
}


//
// HANGING - Jumping/Falling/Stationary
//

int
InitRipperHang(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int dist;
    short ang2;

    hitdata_t hitinfo = { { 0, 0, 0 }, -2, 0, -2 };

    SWBOOL Found = FALSE;
    short dang, tang;

    for (dang = 0; dang < 2048; dang += 128)
    {
        tang = NORM_ANGLE(sp->ang + dang);

        FAFhitscan(sp->x, sp->y, sp->z - SPRITEp_SIZE_Z(sp), sp->sectnum,  // Start position
                   sintable[NORM_ANGLE(tang + 512)],   // X vector of 3D ang
                   sintable[tang],             // Y vector of 3D ang
                   0,                          // Z vector of 3D ang
                   &hitinfo, CLIPMASK_MISSILE);

        //ASSERT(hitinfo.sect >= 0);
        if (hitinfo.sect < 0)
            continue;

        dist = Distance(sp->x, sp->y, hitinfo.pos.x, hitinfo.pos.y);

        if (hitinfo.wall < 0 || dist < 2000 || dist > 7000)
        {
            continue;
        }

        Found = TRUE;
        sp->ang = tang;
        break;
    }

    if (!Found)
    {
        InitActorDecide(SpriteNum);
        return 0;
    }

    NewStateGroup(SpriteNum, sg_RipperHangJump);
    u->StateFallOverride = sg_RipperHangFall;
    DoActorSetSpeed(SpriteNum, FAST_SPEED);

    //u->jump_speed = -800;
    PickJumpMaxSpeed(SpriteNum, -800);

    SET(u->Flags, SPR_JUMPING);
    RESET(u->Flags, SPR_FALLING);

    // set up individual actor jump gravity
    u->jump_grav = 8;

    DoJump(SpriteNum);

    return 0;
}

int
DoRipperHang(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if ((u->WaitTics -= ACTORMOVETICS) > 0)
        return 0;

    NewStateGroup(SpriteNum, sg_RipperJumpAttack);
    // move to the 2nd frame - past the pause frame
    u->Tics += u->State->Tics;
    return 0;
}

int
DoRipperMoveHang(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int nx, ny;

    // Move while jumping
    nx = sp->xvel * (int) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
    ny = sp->xvel * (int) sintable[sp->ang] >> 14;

    // if cannot move the sprite
    if (!move_actor(SpriteNum, nx, ny, 0L))
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_WALL:
        {
            short hit_wall;
            short w, nw;

            hit_wall = NORM_WALL(u->ret);

            NewStateGroup(SpriteNum, u->ActorActionSet->Special[1]);
            u->WaitTics = 2 + ((RANDOM_P2(4 << 8) >> 8) * 120);

            // hang flush with the wall
            w = hit_wall;
            nw = wall[w].point2;
            sp->ang = NORM_ANGLE(getangle(wall[nw].x - wall[w].x, wall[nw].y - wall[w].y) - 512);

            return 0;
        }
        }
    }

    return 0;
}


int
DoRipperHangJF(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int nx, ny;

    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (TEST(u->Flags, SPR_JUMPING))
            DoJump(SpriteNum);
        else
            DoFall(SpriteNum);
    }

    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (DoRipperQuickJump(SpriteNum))
            return 0;

        InitActorDecide(SpriteNum);
    }

    DoRipperMoveHang(SpriteNum);

    return 0;

}

//
// JUMP ATTACK
//

int
DoRipperBeginJumpAttack(short SpriteNum)
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
        sp->ang = NORM_ANGLE((sp->ang + 1024) + (RANDOM_NEG(256 << 6) >> 6));
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
DoRipperMoveJump(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (TEST(u->Flags, SPR_JUMPING))
            DoJump(SpriteNum);
        else
            DoFall(SpriteNum);
    }

    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (DoRipperQuickJump(SpriteNum))
            return 0;

        InitActorDecide(SpriteNum);
    }

    DoRipperMoveHang(SpriteNum);
    return 0;
}

//
// STD MOVEMENT
//

int
DoRipperQuickJump(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    // Tests to see if ripper is on top of a player/enemy and then immediatly
    // does another jump

    if (u->lo_sp)
    {
        SPRITEp tsp = u->lo_sp;

        if (TEST(tsp->extra, SPRX_PLAYER_OR_ENEMY))
        {
            NewStateGroup(SpriteNum, sg_RipperJumpAttack);
            // move past the first state
            u->Tics = 30;
            return TRUE;
        }
    }

    return FALSE;
}


int
NullRipper(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    return 0;
}


int DoRipperPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullRipper(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(SpriteNum);
    return 0;
}


// CTW MODIFICATION
//int DoRipperRipHeart(SpriteNum)
int DoRipperRipHeart(short SpriteNum)
// CTW MODIFICATION END
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    SPRITEp tsp = u->tgt_sp;

    NewStateGroup(SpriteNum, sg_RipperHeart);
    u->WaitTics = 6 * 120;

    // player face ripper
    tsp->ang = getangle(sp->x - tsp->x, sp->y - tsp->y);
    return 0;
}

// CTW MODIFICATION
//int DoRipperStandHeart(SpriteNum)
int DoRipperStandHeart(short SpriteNum)
// CTW MODIFICATION END
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullRipper(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        NewStateGroup(SpriteNum, sg_RipperRun);
    return 0;
}

void RipperHatch(short Weapon)
{
    SPRITEp wp = &sprite[Weapon];

    short New,i;
    SPRITEp np;
    USERp nu;
#define MAX_RIPPERS 1
    short rip_ang[MAX_RIPPERS];

    rip_ang[0] = RANDOM_P2(2048);
    // rip_ang[1] = NORM_ANGLE(rip_ang[0] + 1024 + (RANDOM_P2(512) - 256));

    for (i = 0; i < MAX_RIPPERS; i++)
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
        //np->xrepeat = np->yrepeat = 36;
        np->xrepeat = np->yrepeat = 64;
        np->ang = rip_ang[i];
        np->pal = 0;
        SetupRipper(New);
        nu = User[New];

        // make immediately active
        SET(nu->Flags, SPR_ACTIVE);

        NewStateGroup(New, nu->ActorActionSet->Jump);
        nu->ActorActionFunc = DoActorMoveJump;
        DoActorSetSpeed(New, FAST_SPEED);
        PickJumpMaxSpeed(New, -600);

        SET(nu->Flags, SPR_JUMPING);
        RESET(nu->Flags, SPR_FALLING);

        nu->jump_grav = 8;

        // if I didn't do this here they get stuck in the air sometimes
        DoActorZrange(New);

        DoJump(New);
    }
}

int
DoRipperMove(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (u->scale_speed)
    {
        DoScaleSprite(SpriteNum);
    }

    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (TEST(u->Flags, SPR_JUMPING))
            DoJump(SpriteNum);
        else
            DoFall(SpriteNum);
    }

    // if on a player/enemy sprite jump quickly
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (DoRipperQuickJump(SpriteNum))
            return 0;

        KeepActorOnFloor(SpriteNum);
    }

    if (TEST(u->Flags, SPR_SLIDING))
        DoActorSlide(SpriteNum);

    if (u->track >= 0)
        ActorFollowTrack(SpriteNum, ACTORMOVETICS);
    else
        (*u->ActorActionFunc)(SpriteNum);

    DoActorSectorDamage(SpriteNum);
    return 0;
}


#include "saveable.h"

static saveable_code saveable_ripper_code[] =
{
    SAVE_CODE(SetupRipper),
    SAVE_CODE(GetJumpHeight),
    SAVE_CODE(PickJumpSpeed),
    SAVE_CODE(PickJumpMaxSpeed),

    SAVE_CODE(InitRipperHang),
    SAVE_CODE(DoRipperHang),
    SAVE_CODE(DoRipperMoveHang),
    SAVE_CODE(DoRipperHangJF),

    SAVE_CODE(DoRipperBeginJumpAttack),
    SAVE_CODE(DoRipperMoveJump),

    SAVE_CODE(DoRipperQuickJump),
    SAVE_CODE(NullRipper),
    SAVE_CODE(DoRipperPain),
    SAVE_CODE(DoRipperRipHeart),
    SAVE_CODE(DoRipperStandHeart),
    SAVE_CODE(RipperHatch),
    SAVE_CODE(DoRipperMove),
};

static saveable_data saveable_ripper_data[] =
{
    SAVE_DATA(RipperBattle),
    SAVE_DATA(RipperOffense),
    SAVE_DATA(RipperBroadcast),
    SAVE_DATA(RipperSurprised),
    SAVE_DATA(RipperEvasive),
    SAVE_DATA(RipperLostTarget),
    SAVE_DATA(RipperCloseRange),

    SAVE_DATA(RipperPersonality),

    SAVE_DATA(RipperAttrib),

    SAVE_DATA(s_RipperRun),
    SAVE_DATA(sg_RipperRun),
    SAVE_DATA(s_RipperStand),
    SAVE_DATA(sg_RipperStand),
    SAVE_DATA(s_RipperSwipe),
    SAVE_DATA(sg_RipperSwipe),
    SAVE_DATA(s_RipperSpew),
    SAVE_DATA(sg_RipperSpew),
    SAVE_DATA(s_RipperHeart),
    SAVE_DATA(sg_RipperHeart),
    SAVE_DATA(s_RipperHang),
    SAVE_DATA(sg_RipperHang),
    SAVE_DATA(s_RipperPain),
    SAVE_DATA(sg_RipperPain),
    SAVE_DATA(s_RipperJump),
    SAVE_DATA(sg_RipperJump),
    SAVE_DATA(s_RipperFall),
    SAVE_DATA(sg_RipperFall),
    SAVE_DATA(s_RipperJumpAttack),
    SAVE_DATA(sg_RipperJumpAttack),
    SAVE_DATA(s_RipperHangJump),
    SAVE_DATA(sg_RipperHangJump),
    SAVE_DATA(s_RipperHangFall),
    SAVE_DATA(sg_RipperHangFall),
    SAVE_DATA(s_RipperDie),
    SAVE_DATA(s_RipperDead),
    SAVE_DATA(sg_RipperDie),
    SAVE_DATA(sg_RipperDead),
    SAVE_DATA(s_RipperDeathJump),
    SAVE_DATA(s_RipperDeathFall),
    SAVE_DATA(sg_RipperDeathJump),
    SAVE_DATA(sg_RipperDeathFall),

    SAVE_DATA(RipperActionSet),
    SAVE_DATA(RipperBrownActionSet),
};

saveable_module saveable_ripper =
{
    // code
    saveable_ripper_code,
    SIZ(saveable_ripper_code),

    // data
    saveable_ripper_data,
    SIZ(saveable_ripper_data)
};
