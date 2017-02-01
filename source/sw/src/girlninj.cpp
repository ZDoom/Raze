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
#include "actor.h"
#include "track.h"
#include "sprite.h"

int DoHariKariBlood(short SpriteNum);
//int InitActorMoveCloser(short SpriteNum);

DECISION GirlNinjaBattle[] =
{
    {499, InitActorMoveCloser},
    //{509, InitActorAmbientNoise},
    //{710, InitActorRunAway},
    {1024, InitActorAttack}
};

DECISION GirlNinjaOffense[] =
{
    {499, InitActorMoveCloser},
    //{509, InitActorAmbientNoise},
    {1024, InitActorAttack}
};

DECISION GirlNinjaBroadcast[] =
{
    //{1, InitActorAlertNoise},
    {6, InitActorAmbientNoise},
    {1024, InitActorDecide}
};

DECISION GirlNinjaSurprised[] =
{
    {701, InitActorMoveCloser},
    {1024, InitActorDecide}
};

DECISION GirlNinjaEvasive[] =
{
    {400,   InitActorDuck}, // 100
//    {300,   InitActorEvade},
//    {800,   InitActorRunAway},
    {1024,  NULL}
};

DECISION GirlNinjaLostTarget[] =
{
    {900, InitActorFindPlayer},
    {1024, InitActorWanderAround}
};

DECISION GirlNinjaCloseRange[] =
{
    {900,   InitActorAttack             },
    {1024,  InitActorReposition         }
};

/*

 !AIC - Collection of decision tables

*/

PERSONALITY GirlNinjaPersonality =
{
    GirlNinjaBattle,
    GirlNinjaOffense,
    GirlNinjaBroadcast,
    GirlNinjaSurprised,
    GirlNinjaEvasive,
    GirlNinjaLostTarget,
    GirlNinjaCloseRange,
    GirlNinjaCloseRange
};


ATTRIBUTE GirlNinjaAttrib =
{
    {120, 140, 160, 190},               // Speeds
    {4, 0, 0, -2},                      // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        DIGI_GIRLNINJAALERT, DIGI_GIRLNINJAALERT, DIGI_STAR,
        DIGI_GIRLNINJAALERTT, DIGI_GIRLNINJASCREAM,0,0,0,0,0
    }
};

//////////////////////
//
// GIRLNINJA RUN
//
//////////////////////

ANIMATOR DoGirlNinjaMove, DoGirlNinjaCrawl, DoStayOnFloor, NullGirlNinja, DoActorJump, DoActorFall, DoActorDebris, DoGirlNinjaHariKari, DoActorSlide;
ANIMATOR InitActorDecide;

#define GIRLNINJA_RATE 18

STATE s_GirlNinjaRun[5][4] =
{

    {
        {GIRLNINJA_RUN_R0 + 0, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[0][1]},
        {GIRLNINJA_RUN_R0 + 1, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[0][2]},
        {GIRLNINJA_RUN_R0 + 2, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[0][3]},
        {GIRLNINJA_RUN_R0 + 3, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[0][0]},
    },
    {
        {GIRLNINJA_RUN_R1 + 0, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[1][1]},
        {GIRLNINJA_RUN_R1 + 1, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[1][2]},
        {GIRLNINJA_RUN_R1 + 2, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[1][3]},
        {GIRLNINJA_RUN_R1 + 3, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[1][0]},
    },
    {
        {GIRLNINJA_RUN_R2 + 0, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[2][1]},
        {GIRLNINJA_RUN_R2 + 1, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[2][2]},
        {GIRLNINJA_RUN_R2 + 2, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[2][3]},
        {GIRLNINJA_RUN_R2 + 3, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[2][0]},
    },
    {
        {GIRLNINJA_RUN_R3 + 0, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[3][1]},
        {GIRLNINJA_RUN_R3 + 1, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[3][2]},
        {GIRLNINJA_RUN_R3 + 2, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[3][3]},
        {GIRLNINJA_RUN_R3 + 3, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[3][0]},
    },
    {
        {GIRLNINJA_RUN_R4 + 0, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[4][1]},
        {GIRLNINJA_RUN_R4 + 1, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[4][2]},
        {GIRLNINJA_RUN_R4 + 2, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[4][3]},
        {GIRLNINJA_RUN_R4 + 3, GIRLNINJA_RATE | SF_TIC_ADJUST, DoGirlNinjaMove, &s_GirlNinjaRun[4][0]},
    },

};


STATEp sg_GirlNinjaRun[] =
{
    s_GirlNinjaRun[0],
    s_GirlNinjaRun[1],
    s_GirlNinjaRun[2],
    s_GirlNinjaRun[3],
    s_GirlNinjaRun[4]
};

//////////////////////
//
// GIRLNINJA STAND
//
//////////////////////

#define GIRLNINJA_STAND_RATE 10

STATE s_GirlNinjaStand[5][1] =
{
    {
        {GIRLNINJA_STAND_R0 + 0, GIRLNINJA_STAND_RATE, DoGirlNinjaMove, &s_GirlNinjaStand[0][0]},
    },
    {
        {GIRLNINJA_STAND_R1 + 0, GIRLNINJA_STAND_RATE, DoGirlNinjaMove, &s_GirlNinjaStand[1][0]},
    },
    {
        {GIRLNINJA_STAND_R2 + 0, GIRLNINJA_STAND_RATE, DoGirlNinjaMove, &s_GirlNinjaStand[2][0]},
    },
    {
        {GIRLNINJA_STAND_R3 + 0, GIRLNINJA_STAND_RATE, DoGirlNinjaMove, &s_GirlNinjaStand[3][0]},
    },
    {
        {GIRLNINJA_STAND_R4 + 0, GIRLNINJA_STAND_RATE, DoGirlNinjaMove, &s_GirlNinjaStand[4][0]},
    },
};


STATEp sg_GirlNinjaStand[] =
{
    s_GirlNinjaStand[0],
    s_GirlNinjaStand[1],
    s_GirlNinjaStand[2],
    s_GirlNinjaStand[3],
    s_GirlNinjaStand[4]
};

//////////////////////
//
// GIRLNINJA RISE
//
//////////////////////

#define GIRLNINJA_RISE_RATE 10

STATE s_GirlNinjaRise[5][3] =
{
    {
        {GIRLNINJA_KNEEL_R0 + 0, GIRLNINJA_RISE_RATE, NullGirlNinja, &s_GirlNinjaRise[0][1]},
        {GIRLNINJA_STAND_R0 + 0, GIRLNINJA_STAND_RATE, NullGirlNinja, &s_GirlNinjaRise[0][2]},
        {0, 0, NULL, (STATEp)sg_GirlNinjaRun},  // JBF: sg_GirlNinjaRun really is supposed to be the
        // pointer to the state group. See StateControl() where
        // it says "if (!u->State->Pic)".
    },
    {
        {GIRLNINJA_KNEEL_R1 + 0, GIRLNINJA_RISE_RATE, NullGirlNinja, &s_GirlNinjaRise[1][1]},
        {GIRLNINJA_STAND_R1 + 0, GIRLNINJA_STAND_RATE, NullGirlNinja, &s_GirlNinjaRise[1][2]},
        {0, 0, NULL, (STATEp)sg_GirlNinjaRun},
    },
    {
        {GIRLNINJA_KNEEL_R2 + 0, GIRLNINJA_RISE_RATE, NullGirlNinja, &s_GirlNinjaRise[2][1]},
        {GIRLNINJA_STAND_R2 + 0, GIRLNINJA_STAND_RATE, NullGirlNinja, &s_GirlNinjaRise[2][2]},
        {0, 0, NULL, (STATEp)sg_GirlNinjaRun},
    },
    {
        {GIRLNINJA_KNEEL_R3 + 0, GIRLNINJA_RISE_RATE, NullGirlNinja, &s_GirlNinjaRise[3][1]},
        {GIRLNINJA_STAND_R3 + 0, GIRLNINJA_STAND_RATE, NullGirlNinja, &s_GirlNinjaRise[3][2]},
        {0, 0, NULL, (STATEp)sg_GirlNinjaRun},
    },
    {
        {GIRLNINJA_KNEEL_R4 + 0, GIRLNINJA_RISE_RATE, NullGirlNinja, &s_GirlNinjaRise[4][1]},
        {GIRLNINJA_STAND_R4 + 0, GIRLNINJA_STAND_RATE, NullGirlNinja, &s_GirlNinjaRise[4][2]},
        {0, 0, NULL, (STATEp)sg_GirlNinjaRun},
    },
};


STATEp sg_GirlNinjaRise[] =
{
    s_GirlNinjaRise[0],
    s_GirlNinjaRise[1],
    s_GirlNinjaRise[2],
    s_GirlNinjaRise[3],
    s_GirlNinjaRise[4]
};

//////////////////////
//
// GIRLNINJA DUCK
//
//////////////////////

#define GIRLNINJA_DUCK_RATE 10
#define GIRLNINJA_CRAWL_RATE 14

STATE s_GirlNinjaDuck[5][2] =
{
    {
        {GIRLNINJA_KNEEL_R0 + 0, GIRLNINJA_DUCK_RATE, NullGirlNinja, &s_GirlNinjaDuck[0][1]},
        {GIRLNINJA_CRAWL_R0 + 0, GIRLNINJA_CRAWL_RATE, DoGirlNinjaMove, &s_GirlNinjaDuck[0][1]},
    },
    {
        {GIRLNINJA_KNEEL_R1 + 0, GIRLNINJA_DUCK_RATE, NullGirlNinja, &s_GirlNinjaDuck[1][1]},
        {GIRLNINJA_CRAWL_R1 + 0, GIRLNINJA_CRAWL_RATE, DoGirlNinjaMove, &s_GirlNinjaDuck[1][1]},
    },
    {
        {GIRLNINJA_KNEEL_R2 + 0, GIRLNINJA_DUCK_RATE, NullGirlNinja, &s_GirlNinjaDuck[2][1]},
        {GIRLNINJA_CRAWL_R2 + 0, GIRLNINJA_CRAWL_RATE, DoGirlNinjaMove, &s_GirlNinjaDuck[2][1]},
    },
    {
        {GIRLNINJA_KNEEL_R3 + 0, GIRLNINJA_DUCK_RATE, NullGirlNinja, &s_GirlNinjaDuck[3][1]},
        {GIRLNINJA_CRAWL_R3 + 0, GIRLNINJA_CRAWL_RATE, DoGirlNinjaMove, &s_GirlNinjaDuck[3][1]},
    },
    {
        {GIRLNINJA_KNEEL_R4 + 0, GIRLNINJA_DUCK_RATE, NullGirlNinja, &s_GirlNinjaDuck[4][1]},
        {GIRLNINJA_CRAWL_R4 + 0, GIRLNINJA_CRAWL_RATE, DoGirlNinjaMove, &s_GirlNinjaDuck[4][1]},
    },
};


STATEp sg_GirlNinjaDuck[] =
{
    s_GirlNinjaDuck[0],
    s_GirlNinjaDuck[1],
    s_GirlNinjaDuck[2],
    s_GirlNinjaDuck[3],
    s_GirlNinjaDuck[4]
};


//////////////////////
//
// GIRLNINJA SIT
//
//////////////////////

STATE s_GirlNinjaSit[5][1] =
{
    {
        {GIRLNINJA_KNEEL_R0 + 0, GIRLNINJA_RISE_RATE, DoGirlNinjaMove, &s_GirlNinjaSit[0][0]},
    },
    {
        {GIRLNINJA_KNEEL_R1 + 0, GIRLNINJA_RISE_RATE, DoGirlNinjaMove, &s_GirlNinjaSit[1][0]},
    },
    {
        {GIRLNINJA_KNEEL_R2 + 0, GIRLNINJA_RISE_RATE, DoGirlNinjaMove, &s_GirlNinjaSit[2][0]},
    },
    {
        {GIRLNINJA_KNEEL_R3 + 0, GIRLNINJA_RISE_RATE, DoGirlNinjaMove, &s_GirlNinjaSit[3][0]},
    },
    {
        {GIRLNINJA_KNEEL_R4 + 0, GIRLNINJA_RISE_RATE, DoGirlNinjaMove, &s_GirlNinjaSit[4][0]},
    },
};


STATEp sg_GirlNinjaSit[] =
{
    s_GirlNinjaSit[0],
    s_GirlNinjaSit[1],
    s_GirlNinjaSit[2],
    s_GirlNinjaSit[3],
    s_GirlNinjaSit[4]
};


//////////////////////
//
// GIRLNINJA JUMP
//
//////////////////////

#define GIRLNINJA_JUMP_RATE 24

STATE s_GirlNinjaJump[5][2] =
{
    {
        {GIRLNINJA_JUMP_R0 + 0, GIRLNINJA_JUMP_RATE, DoGirlNinjaMove, &s_GirlNinjaJump[0][1]},
        {GIRLNINJA_JUMP_R0 + 1, GIRLNINJA_JUMP_RATE, DoGirlNinjaMove, &s_GirlNinjaJump[0][1]},
    },
    {
        {GIRLNINJA_JUMP_R1 + 0, GIRLNINJA_JUMP_RATE, DoGirlNinjaMove, &s_GirlNinjaJump[1][1]},
        {GIRLNINJA_JUMP_R1 + 1, GIRLNINJA_JUMP_RATE, DoGirlNinjaMove, &s_GirlNinjaJump[1][1]},
    },
    {
        {GIRLNINJA_JUMP_R2 + 0, GIRLNINJA_JUMP_RATE, DoGirlNinjaMove, &s_GirlNinjaJump[2][1]},
        {GIRLNINJA_JUMP_R2 + 1, GIRLNINJA_JUMP_RATE, DoGirlNinjaMove, &s_GirlNinjaJump[2][1]},
    },
    {
        {GIRLNINJA_JUMP_R3 + 0, GIRLNINJA_JUMP_RATE, DoGirlNinjaMove, &s_GirlNinjaJump[3][1]},
        {GIRLNINJA_JUMP_R3 + 1, GIRLNINJA_JUMP_RATE, DoGirlNinjaMove, &s_GirlNinjaJump[3][1]},
    },
    {
        {GIRLNINJA_JUMP_R4 + 0, GIRLNINJA_JUMP_RATE, DoGirlNinjaMove, &s_GirlNinjaJump[4][1]},
        {GIRLNINJA_JUMP_R4 + 1, GIRLNINJA_JUMP_RATE, DoGirlNinjaMove, &s_GirlNinjaJump[4][1]},
    },
};


STATEp sg_GirlNinjaJump[] =
{
    s_GirlNinjaJump[0],
    s_GirlNinjaJump[1],
    s_GirlNinjaJump[2],
    s_GirlNinjaJump[3],
    s_GirlNinjaJump[4]
};


//////////////////////
//
// GIRLNINJA FALL
//
//////////////////////

#define GIRLNINJA_FALL_RATE 16

STATE s_GirlNinjaFall[5][2] =
{
    {
        {GIRLNINJA_JUMP_R0 + 1, GIRLNINJA_FALL_RATE, DoGirlNinjaMove, &s_GirlNinjaFall[0][1]},
        {GIRLNINJA_JUMP_R0 + 2, GIRLNINJA_FALL_RATE, DoGirlNinjaMove, &s_GirlNinjaFall[0][1]},
    },
    {
        {GIRLNINJA_JUMP_R1 + 1, GIRLNINJA_FALL_RATE, DoGirlNinjaMove, &s_GirlNinjaFall[1][1]},
        {GIRLNINJA_JUMP_R1 + 2, GIRLNINJA_FALL_RATE, DoGirlNinjaMove, &s_GirlNinjaFall[1][1]},
    },
    {
        {GIRLNINJA_JUMP_R2 + 1, GIRLNINJA_FALL_RATE, DoGirlNinjaMove, &s_GirlNinjaFall[2][1]},
        {GIRLNINJA_JUMP_R2 + 2, GIRLNINJA_FALL_RATE, DoGirlNinjaMove, &s_GirlNinjaFall[2][1]},
    },
    {
        {GIRLNINJA_JUMP_R3 + 1, GIRLNINJA_FALL_RATE, DoGirlNinjaMove, &s_GirlNinjaFall[3][1]},
        {GIRLNINJA_JUMP_R3 + 2, GIRLNINJA_FALL_RATE, DoGirlNinjaMove, &s_GirlNinjaFall[3][1]},
    },
    {
        {GIRLNINJA_JUMP_R4 + 1, GIRLNINJA_FALL_RATE, DoGirlNinjaMove, &s_GirlNinjaFall[4][1]},
        {GIRLNINJA_JUMP_R4 + 2, GIRLNINJA_FALL_RATE, DoGirlNinjaMove, &s_GirlNinjaFall[4][1]},
    },
};


STATEp sg_GirlNinjaFall[] =
{
    s_GirlNinjaFall[0],
    s_GirlNinjaFall[1],
    s_GirlNinjaFall[2],
    s_GirlNinjaFall[3],
    s_GirlNinjaFall[4]
};

//////////////////////
//
// GIRLNINJA PAIN
//
//////////////////////

#define GIRLNINJA_PAIN_RATE 15
ANIMATOR DoGirlNinjaPain;

STATE s_GirlNinjaPain[5][1] =
{
    {
        {GIRLNINJA_PAIN_R0 + 0, GIRLNINJA_PAIN_RATE, DoGirlNinjaPain, &s_GirlNinjaPain[0][0]},
    },
    {
        {GIRLNINJA_PAIN_R1 + 0, GIRLNINJA_PAIN_RATE, DoGirlNinjaPain, &s_GirlNinjaPain[1][0]},
    },
    {
        {GIRLNINJA_PAIN_R2 + 0, GIRLNINJA_PAIN_RATE, DoGirlNinjaPain, &s_GirlNinjaPain[2][0]},
    },
    {
        {GIRLNINJA_PAIN_R3 + 0, GIRLNINJA_PAIN_RATE, DoGirlNinjaPain, &s_GirlNinjaPain[3][0]},
    },
    {
        {GIRLNINJA_PAIN_R4 + 0, GIRLNINJA_PAIN_RATE, DoGirlNinjaPain, &s_GirlNinjaPain[4][0]},
    },
};

STATEp sg_GirlNinjaPain[] =
{
    s_GirlNinjaPain[0],
    s_GirlNinjaPain[1],
    s_GirlNinjaPain[2],
    s_GirlNinjaPain[3],
    s_GirlNinjaPain[4]
};

//////////////////////
//
// GIRLNINJA STICKY
//
//////////////////////

#define GIRLNINJA_STICKY_RATE 32
ANIMATOR InitEnemyMine;

STATE s_GirlNinjaSticky[5][6] =
{
    {
        {GIRLNINJA_THROW_R0 + 0, GIRLNINJA_STICKY_RATE * 2,     NullGirlNinja,            &s_GirlNinjaSticky[0][1]},
        {GIRLNINJA_THROW_R0 + 0, GIRLNINJA_STICKY_RATE,       NullGirlNinja,          &s_GirlNinjaSticky[0][2]},
        {GIRLNINJA_THROW_R0 + 1, 0 | SF_QUICK_CALL,         InitEnemyMine,      &s_GirlNinjaSticky[0][3]},
        {GIRLNINJA_THROW_R0 + 1, GIRLNINJA_STICKY_RATE * 2,     NullGirlNinja,            &s_GirlNinjaSticky[0][4]},
        {GIRLNINJA_THROW_R0 + 2, 0 | SF_QUICK_CALL,         InitActorDecide,    &s_GirlNinjaSticky[0][5]},
        {GIRLNINJA_THROW_R0 + 2, GIRLNINJA_STICKY_RATE,       DoGirlNinjaMove,        &s_GirlNinjaSticky[0][5]},
    },
    {
        {GIRLNINJA_THROW_R1 + 0, GIRLNINJA_STICKY_RATE * 2,     NullGirlNinja,            &s_GirlNinjaSticky[1][1]},
        {GIRLNINJA_THROW_R1 + 0, GIRLNINJA_STICKY_RATE,       NullGirlNinja,          &s_GirlNinjaSticky[1][2]},
        {GIRLNINJA_THROW_R1 + 1, 0 | SF_QUICK_CALL,         InitEnemyMine,      &s_GirlNinjaSticky[1][3]},
        {GIRLNINJA_THROW_R1 + 1, GIRLNINJA_STICKY_RATE * 2,     NullGirlNinja,            &s_GirlNinjaSticky[1][4]},
        {GIRLNINJA_THROW_R1 + 2, 0 | SF_QUICK_CALL,         InitActorDecide,    &s_GirlNinjaSticky[1][5]},
        {GIRLNINJA_THROW_R1 + 2, GIRLNINJA_STICKY_RATE,       DoGirlNinjaMove,        &s_GirlNinjaSticky[1][5]},
    },
    {
        {GIRLNINJA_THROW_R2 + 0, GIRLNINJA_STICKY_RATE * 2,     NullGirlNinja,            &s_GirlNinjaSticky[2][1]},
        {GIRLNINJA_THROW_R2 + 0, GIRLNINJA_STICKY_RATE,       NullGirlNinja,          &s_GirlNinjaSticky[2][2]},
        {GIRLNINJA_THROW_R2 + 1, 0 | SF_QUICK_CALL,         InitEnemyMine,      &s_GirlNinjaSticky[2][3]},
        {GIRLNINJA_THROW_R2 + 1, GIRLNINJA_STICKY_RATE * 2,     NullGirlNinja,            &s_GirlNinjaSticky[2][4]},
        {GIRLNINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL,         InitActorDecide,    &s_GirlNinjaSticky[2][5]},
        {GIRLNINJA_THROW_R2 + 2, GIRLNINJA_STICKY_RATE,       DoGirlNinjaMove,        &s_GirlNinjaSticky[2][5]},
    },
    {
        {GIRLNINJA_THROW_R2 + 0, GIRLNINJA_STICKY_RATE * 2,     NullGirlNinja,            &s_GirlNinjaSticky[3][1]},
        {GIRLNINJA_THROW_R2 + 0, GIRLNINJA_STICKY_RATE,       NullGirlNinja,          &s_GirlNinjaSticky[3][2]},
        {GIRLNINJA_THROW_R2 + 1, 0 | SF_QUICK_CALL,         InitEnemyMine,      &s_GirlNinjaSticky[3][3]},
        {GIRLNINJA_THROW_R2 + 1, GIRLNINJA_STICKY_RATE * 2,     NullGirlNinja,            &s_GirlNinjaSticky[3][4]},
        {GIRLNINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL,         InitActorDecide,    &s_GirlNinjaSticky[3][5]},
        {GIRLNINJA_THROW_R2 + 2, GIRLNINJA_STICKY_RATE,       DoGirlNinjaMove,        &s_GirlNinjaSticky[3][5]},
    },
    {
        {GIRLNINJA_THROW_R2 + 0, GIRLNINJA_STICKY_RATE * 2,     NullGirlNinja,            &s_GirlNinjaSticky[4][1]},
        {GIRLNINJA_THROW_R2 + 0, GIRLNINJA_STICKY_RATE,       NullGirlNinja,          &s_GirlNinjaSticky[4][2]},
        {GIRLNINJA_THROW_R2 + 1, 0 | SF_QUICK_CALL,         InitEnemyMine,      &s_GirlNinjaSticky[4][3]},
        {GIRLNINJA_THROW_R2 + 1, GIRLNINJA_STICKY_RATE * 2,     NullGirlNinja,            &s_GirlNinjaSticky[4][4]},
        {GIRLNINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL,         InitActorDecide,    &s_GirlNinjaSticky[4][5]},
        {GIRLNINJA_THROW_R2 + 2, GIRLNINJA_STICKY_RATE,       DoGirlNinjaMove,        &s_GirlNinjaSticky[4][5]},
    },
};

STATEp sg_GirlNinjaSticky[] =
{
    s_GirlNinjaSticky[0],
    s_GirlNinjaSticky[1],
    s_GirlNinjaSticky[2],
    s_GirlNinjaSticky[3],
    s_GirlNinjaSticky[4]
};


//////////////////////
//
// GIRLNINJA CROSSBOW
//
//////////////////////

#define GIRLNINJA_CROSSBOW_RATE 14
ANIMATOR InitEnemyCrossbow;

STATE s_GirlNinjaCrossbow[5][5] =
{
    {
        {GIRLNINJA_FIRE_R0 + 0, GIRLNINJA_CROSSBOW_RATE * 2, NullGirlNinja, &s_GirlNinjaCrossbow[0][1]},
        {GIRLNINJA_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyCrossbow, &s_GirlNinjaCrossbow[0][2]},
        {GIRLNINJA_FIRE_R0 + 1, GIRLNINJA_CROSSBOW_RATE, NullGirlNinja, &s_GirlNinjaCrossbow[0][3]},
        {GIRLNINJA_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitActorDecide, &s_GirlNinjaCrossbow[0][4]},
        {GIRLNINJA_FIRE_R0 + 1, GIRLNINJA_CROSSBOW_RATE, DoGirlNinjaMove, &s_GirlNinjaCrossbow[0][4]},
    },
    {
        {GIRLNINJA_FIRE_R1 + 0, GIRLNINJA_CROSSBOW_RATE * 2, NullGirlNinja, &s_GirlNinjaCrossbow[1][1]},
        {GIRLNINJA_FIRE_R1 + 1, 0 | SF_QUICK_CALL, InitEnemyCrossbow, &s_GirlNinjaCrossbow[1][2]},
        {GIRLNINJA_FIRE_R1 + 1, GIRLNINJA_CROSSBOW_RATE, NullGirlNinja, &s_GirlNinjaCrossbow[1][3]},
        {GIRLNINJA_FIRE_R1 + 1, 0 | SF_QUICK_CALL, InitActorDecide, &s_GirlNinjaCrossbow[1][4]},
        {GIRLNINJA_FIRE_R1 + 1, GIRLNINJA_CROSSBOW_RATE, DoGirlNinjaMove, &s_GirlNinjaCrossbow[1][4]},
    },
    {
        {GIRLNINJA_FIRE_R2 + 0, GIRLNINJA_CROSSBOW_RATE * 2, NullGirlNinja, &s_GirlNinjaCrossbow[2][1]},
        {GIRLNINJA_FIRE_R2 + 1, 0 | SF_QUICK_CALL, InitEnemyCrossbow, &s_GirlNinjaCrossbow[2][2]},
        {GIRLNINJA_FIRE_R2 + 1, GIRLNINJA_CROSSBOW_RATE, NullGirlNinja, &s_GirlNinjaCrossbow[2][3]},
        {GIRLNINJA_FIRE_R2 + 1, 0 | SF_QUICK_CALL, InitActorDecide, &s_GirlNinjaCrossbow[2][4]},
        {GIRLNINJA_FIRE_R2 + 1, GIRLNINJA_CROSSBOW_RATE, DoGirlNinjaMove, &s_GirlNinjaCrossbow[2][4]},
    },
    {
        {GIRLNINJA_FIRE_R3 + 0, GIRLNINJA_CROSSBOW_RATE * 2, NullGirlNinja, &s_GirlNinjaCrossbow[3][1]},
        {GIRLNINJA_FIRE_R3 + 1, 0 | SF_QUICK_CALL, InitEnemyCrossbow, &s_GirlNinjaCrossbow[3][2]},
        {GIRLNINJA_FIRE_R3 + 1, GIRLNINJA_CROSSBOW_RATE, NullGirlNinja, &s_GirlNinjaCrossbow[3][3]},
        {GIRLNINJA_FIRE_R3 + 1, 0 | SF_QUICK_CALL, InitActorDecide, &s_GirlNinjaCrossbow[3][4]},
        {GIRLNINJA_FIRE_R3 + 1, GIRLNINJA_CROSSBOW_RATE, DoGirlNinjaMove, &s_GirlNinjaCrossbow[3][4]},
    },
    {
        {GIRLNINJA_FIRE_R4 + 0, GIRLNINJA_CROSSBOW_RATE * 2, NullGirlNinja, &s_GirlNinjaCrossbow[4][1]},
        {GIRLNINJA_FIRE_R4 + 1, 0 | SF_QUICK_CALL, InitEnemyCrossbow, &s_GirlNinjaCrossbow[4][2]},
        {GIRLNINJA_FIRE_R4 + 1, GIRLNINJA_CROSSBOW_RATE, NullGirlNinja, &s_GirlNinjaCrossbow[4][3]},
        {GIRLNINJA_FIRE_R4 + 1, 0 | SF_QUICK_CALL, InitActorDecide, &s_GirlNinjaCrossbow[4][4]},
        {GIRLNINJA_FIRE_R4 + 1, GIRLNINJA_CROSSBOW_RATE, DoGirlNinjaMove, &s_GirlNinjaCrossbow[4][4]},
    },
};


STATEp sg_GirlNinjaCrossbow[] =
{
    s_GirlNinjaCrossbow[0],
    s_GirlNinjaCrossbow[1],
    s_GirlNinjaCrossbow[2],
    s_GirlNinjaCrossbow[3],
    s_GirlNinjaCrossbow[4]
};



//////////////////////
//
// GIRLNINJA DIE
//
//////////////////////

#define GIRLNINJA_DIE_RATE 30
ANIMATOR DoGirlNinjaSpecial;

STATE s_GirlNinjaDie[] =
{
    {GIRLNINJA_DIE + 0, GIRLNINJA_DIE_RATE*2, NullGirlNinja, &s_GirlNinjaDie[1]},
    {GIRLNINJA_DIE + 1, GIRLNINJA_DIE_RATE, NullGirlNinja, &s_GirlNinjaDie[2]},
    {GIRLNINJA_DIE + 2, GIRLNINJA_DIE_RATE, NullGirlNinja, &s_GirlNinjaDie[3]},
    {GIRLNINJA_DIE + 3, GIRLNINJA_DIE_RATE, NullGirlNinja, &s_GirlNinjaDie[4]},
    {GIRLNINJA_DIE + 4, GIRLNINJA_DIE_RATE, NullGirlNinja, &s_GirlNinjaDie[5]},
    {GIRLNINJA_DIE + 5, GIRLNINJA_DIE_RATE, NullGirlNinja, &s_GirlNinjaDie[6]},
    {GIRLNINJA_DIE + 6, GIRLNINJA_DIE_RATE, NullGirlNinja, &s_GirlNinjaDie[7]},
    {GIRLNINJA_DIE + 6, SF_QUICK_CALL, DoGirlNinjaSpecial, &s_GirlNinjaDie[8]},
    {GIRLNINJA_DIE + 7, GIRLNINJA_DIE_RATE, NullGirlNinja, &s_GirlNinjaDie[9]},
    {GIRLNINJA_DIE + 8, SF_QUICK_CALL, QueueFloorBlood, &s_GirlNinjaDie[10]},
    {GIRLNINJA_DIE + 8, GIRLNINJA_DIE_RATE, DoActorDebris, &s_GirlNinjaDie[10]},
};

STATE s_GirlNinjaDead[] =
{
    {GIRLNINJA_DIE + 6, GIRLNINJA_DIE_RATE, DoActorDebris, &s_GirlNinjaDead[1]},
    {GIRLNINJA_DIE + 7, SF_QUICK_CALL, DoGirlNinjaSpecial, &s_GirlNinjaDead[2]},
    {GIRLNINJA_DIE + 7, GIRLNINJA_DIE_RATE, DoActorDebris, &s_GirlNinjaDead[3]},
    {GIRLNINJA_DIE + 8, SF_QUICK_CALL, QueueFloorBlood,&s_GirlNinjaDead[4]},
    {GIRLNINJA_DIE + 8, GIRLNINJA_DIE_RATE, DoActorDebris, &s_GirlNinjaDead[4]},
};


STATE s_GirlNinjaDeathJump[] =
{
    {GIRLNINJA_DIE + 0, GIRLNINJA_DIE_RATE, DoActorDeathMove, &s_GirlNinjaDeathJump[1]},
    {GIRLNINJA_DIE + 1, GIRLNINJA_DIE_RATE, DoActorDeathMove, &s_GirlNinjaDeathJump[2]},
    {GIRLNINJA_DIE + 2, GIRLNINJA_DIE_RATE, DoActorDeathMove, &s_GirlNinjaDeathJump[2]},
};

STATE s_GirlNinjaDeathFall[] =
{
    {GIRLNINJA_DIE + 3, GIRLNINJA_DIE_RATE, DoActorDeathMove, &s_GirlNinjaDeathFall[1]},
    {GIRLNINJA_DIE + 4, GIRLNINJA_DIE_RATE, DoActorDeathMove, &s_GirlNinjaDeathFall[1]},
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

STATEp sg_GirlNinjaDie[] =
{
    s_GirlNinjaDie
};

STATEp sg_GirlNinjaDead[] =
{
    s_GirlNinjaDead
};

STATEp sg_GirlNinjaDeathJump[] =
{
    s_GirlNinjaDeathJump
};

STATEp sg_GirlNinjaDeathFall[] =
{
    s_GirlNinjaDeathFall
};

/*

 !AIC - Collection of states that connect action to states

*/

ACTOR_ACTION_SET GirlNinjaActionSet =
{
    sg_GirlNinjaStand,
    sg_GirlNinjaRun,
    sg_GirlNinjaJump,
    sg_GirlNinjaFall,
    NULL,
    NULL,
    NULL,
    sg_GirlNinjaRise,
    sg_GirlNinjaSit,
    NULL,
    NULL,
    sg_GirlNinjaPain,
    sg_GirlNinjaDie,
    NULL,
    sg_GirlNinjaDead,
    sg_GirlNinjaDeathJump,
    sg_GirlNinjaDeathFall,
    {sg_GirlNinjaCrossbow, sg_GirlNinjaSticky},
    {800, 1024},
    {sg_GirlNinjaCrossbow, sg_GirlNinjaSticky},
    {800, 1024},
    {NULL},
    sg_GirlNinjaDuck,
    NULL
};

int
SetupGirlNinja(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u;
    ANIMATOR DoActorDecide;
    short pic = sp->picnum;

    if (TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
    {
        u = User[SpriteNum];
        ASSERT(u);
    }
    else
    {
        User[SpriteNum] = u = SpawnUser(SpriteNum, GIRLNINJA_RUN_R0, s_GirlNinjaRun[0]);
        u->Health = 100;
    }

    u->StateEnd = s_GirlNinjaDie;
    u->Rot = sg_GirlNinjaRun;
    sp->xrepeat = 51;
    sp->yrepeat = 43;

    u->Attrib = &GirlNinjaAttrib;
    sp->pal = u->spal = 26;
    EnemyDefaults(SpriteNum, &GirlNinjaActionSet, &GirlNinjaPersonality);

    ChangeState(SpriteNum, s_GirlNinjaRun[0]);
    DoActorSetSpeed(SpriteNum, NORM_SPEED);

    u->Radius = 280;
    RESET(u->Flags, SPR_XFLIP_TOGGLE);

    return 0;
}


int
DoGirlNinjaMove(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    // jumping and falling
    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING) && !TEST(u->Flags, SPR_CLIMBING))
    {
        if (TEST(u->Flags, SPR_JUMPING))
            DoActorJump(SpriteNum);
        else if (TEST(u->Flags, SPR_FALLING))
            DoActorFall(SpriteNum);
    }

    // sliding
    if (TEST(u->Flags, SPR_SLIDING) && !TEST(u->Flags, SPR_CLIMBING))
        DoActorSlide(SpriteNum);

    // !AIC - do track or call current action function - such as DoActorMoveCloser()
    if (u->track >= 0)
        ActorFollowTrack(SpriteNum, ACTORMOVETICS);
    else
    {
        (*u->ActorActionFunc)(SpriteNum);
    }

    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING | SPR_CLIMBING))
    {
        KeepActorOnFloor(SpriteNum);
    }

    // take damage from environment
    DoActorSectorDamage(SpriteNum);

    return 0;
}

int
GirlNinjaJumpActionFunc(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    int nx, ny;

    // Move while jumping
    nx = sp->xvel * (int) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
    ny = sp->xvel * (int) sintable[sp->ang] >> 14;

    // if cannot move the sprite
    if (!move_actor(SpriteNum, nx, ny, 0L))
    {
        return 0;
    }

    if (!TEST(u->Flags, SPR_JUMPING|SPR_FALLING))
    {
        InitActorDecide(SpriteNum);
    }

    return 0;
}

int
NullGirlNinja(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (u->WaitTics > 0) u->WaitTics -= ACTORMOVETICS;

    if (TEST(u->Flags, SPR_SLIDING) && !TEST(u->Flags, SPR_CLIMBING) && !TEST(u->Flags, SPR_JUMPING|SPR_FALLING))
        DoActorSlide(SpriteNum);

    if (!TEST(u->Flags, SPR_CLIMBING) && !TEST(u->Flags, SPR_JUMPING|SPR_FALLING))
        KeepActorOnFloor(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    return 0;
}


int DoGirlNinjaPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullGirlNinja(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(SpriteNum);

    return 0;
}

int DoGirlNinjaSpecial(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (u->spal == PALETTE_PLAYER5)
    {
        RESET(sp->cstat,CSTAT_SPRITE_TRANSLUCENT);
        sp->hitag = 0;
        sp->shade = -10;
    }

    return 0;
}


#include "saveable.h"

static saveable_code saveable_girlninj_code[] =
{
    SAVE_CODE(SetupGirlNinja),
    SAVE_CODE(DoGirlNinjaMove),
    SAVE_CODE(GirlNinjaJumpActionFunc),
    SAVE_CODE(NullGirlNinja),
    SAVE_CODE(DoGirlNinjaPain),
    SAVE_CODE(DoGirlNinjaSpecial),
};

static saveable_data saveable_girlninj_data[] =
{
    SAVE_DATA(GirlNinjaBattle),
    SAVE_DATA(GirlNinjaOffense),
    SAVE_DATA(GirlNinjaBroadcast),
    SAVE_DATA(GirlNinjaSurprised),
    SAVE_DATA(GirlNinjaEvasive),
    SAVE_DATA(GirlNinjaLostTarget),
    SAVE_DATA(GirlNinjaCloseRange),

    SAVE_DATA(GirlNinjaPersonality),

    SAVE_DATA(GirlNinjaAttrib),

    SAVE_DATA(s_GirlNinjaRun),
    SAVE_DATA(sg_GirlNinjaRun),
    SAVE_DATA(s_GirlNinjaStand),
    SAVE_DATA(sg_GirlNinjaStand),
    SAVE_DATA(s_GirlNinjaRise),
    SAVE_DATA(sg_GirlNinjaRise),
    SAVE_DATA(s_GirlNinjaDuck),
    SAVE_DATA(sg_GirlNinjaDuck),
    SAVE_DATA(s_GirlNinjaSit),
    SAVE_DATA(sg_GirlNinjaSit),
    SAVE_DATA(s_GirlNinjaJump),
    SAVE_DATA(sg_GirlNinjaJump),
    SAVE_DATA(s_GirlNinjaFall),
    SAVE_DATA(sg_GirlNinjaFall),
    SAVE_DATA(s_GirlNinjaPain),
    SAVE_DATA(sg_GirlNinjaPain),
    SAVE_DATA(s_GirlNinjaSticky),
    SAVE_DATA(sg_GirlNinjaSticky),
    SAVE_DATA(s_GirlNinjaCrossbow),
    SAVE_DATA(sg_GirlNinjaCrossbow),
    SAVE_DATA(s_GirlNinjaDie),
    SAVE_DATA(s_GirlNinjaDead),
    SAVE_DATA(s_GirlNinjaDeathJump),
    SAVE_DATA(s_GirlNinjaDeathFall),
    SAVE_DATA(sg_GirlNinjaDie),
    SAVE_DATA(sg_GirlNinjaDead),
    SAVE_DATA(sg_GirlNinjaDeathJump),
    SAVE_DATA(sg_GirlNinjaDeathFall),

    SAVE_DATA(GirlNinjaActionSet),
};

saveable_module saveable_girlninj =
{
    // code
    saveable_girlninj_code,
    SIZ(saveable_girlninj_code),

    // data
    saveable_girlninj_data,
    SIZ(saveable_girlninj_data)
};
