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
#include "weapon.h"
#include "track.h"
#include "actor.h"
#include "ninja.h"
#include "sprite.h"

/*

 !AIC - Decision tables used in mostly ai.c DoActorActionDecide().

*/

DECISION NinjaBattle[] =
{
    {499, InitActorMoveCloser},
    //{509, InitActorAmbientNoise},
    //{710, InitActorRunAway},
    {1024, InitActorAttack}
};

DECISION NinjaOffense[] =
{
    {499, InitActorMoveCloser},
    //{509, InitActorAmbientNoise},
    {1024, InitActorAttack}
};

DECISION NinjaBroadcast[] =
{
    //{1, InitActorAlertNoise},
    {6, InitActorAmbientNoise},
    {1024, InitActorDecide}
};

DECISION NinjaSurprised[] =
{
    {701, InitActorMoveCloser},
    {1024, InitActorDecide}
};

DECISION NinjaEvasive[] =
{
    {400,   InitActorDuck}, // 100
//    {300,   InitActorEvade},
//    {800,   InitActorRunAway},
    {1024,  NULL}
};

DECISION NinjaLostTarget[] =
{
    {900, InitActorFindPlayer},
    {1024, InitActorWanderAround}
};

DECISION NinjaCloseRange[] =
{
    {700,   InitActorAttack             },
    {1024,  InitActorReposition         }
};

/*

 !AIC - Collection of decision tables

*/

PERSONALITY NinjaPersonality =
{
    NinjaBattle,
    NinjaOffense,
    NinjaBroadcast,
    NinjaSurprised,
    NinjaEvasive,
    NinjaLostTarget,
    NinjaCloseRange,
    NinjaCloseRange
};

// Sniper Ninjas
DECISION NinjaSniperRoam[] =
{
    {1023, InitActorDuck},
    {1024, InitActorAmbientNoise},
};

DECISION NinjaSniperBattle[] =
{
    {499, InitActorDuck},
    {500, InitActorAmbientNoise},
    {1024, InitActorAttack}
};

PERSONALITY NinjaSniperPersonality =
{
    NinjaSniperBattle,
    NinjaSniperBattle,
    NinjaSniperRoam,
    NinjaSniperRoam,
    NinjaSniperRoam,
    NinjaSniperRoam,
    NinjaSniperBattle,
    NinjaSniperBattle
};


/*

 !AIC - Extra attributes - speeds for running, animation tic adjustments for speeeds, etc

*/

ATTRIBUTE NinjaAttrib =
{
    {110, 130, 150, 180},                 // Speeds
    //{90, 110, 130, 160},                 // Speeds
    //{60, 80, 100, 130},                 // Speeds
    {4, 0, 0, -2},                     // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        DIGI_NINJAAMBIENT, DIGI_NINJAALERT, DIGI_STAR,
        DIGI_NINJAPAIN, DIGI_NINJASCREAM,0,0,0,0,0
    }
};

ATTRIBUTE InvisibleNinjaAttrib =
{
    {220, 240, 300, 360},                 // Speeds
    {4, 0, 0, -2},                     // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        DIGI_NINJAAMBIENT, DIGI_NINJAALERT, DIGI_STAR,
        DIGI_NINJAPAIN, DIGI_NINJASCREAM,0,0,0,0,0
    }
};

ATTRIBUTE DefaultAttrib =
{
    {60, 80, 100, 130},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {0, 0, 0}
};

//////////////////////
//
// NINJA RUN
//
//////////////////////

ANIMATOR DoNinjaMove, DoNinjaCrawl, DoStayOnFloor, NullNinja, DoActorJump, DoActorFall, DoActorDebris, DoNinjaHariKari, DoActorSlide;
ANIMATOR InitActorDecide;

#define NINJA_RATE 18

STATE s_NinjaRun[5][4] =
{

    {
        {NINJA_RUN_R0 + 0, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[0][1]},
        {NINJA_RUN_R0 + 1, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[0][2]},
        {NINJA_RUN_R0 + 2, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[0][3]},
        {NINJA_RUN_R0 + 3, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[0][0]},
    },
    {
        {NINJA_RUN_R1 + 0, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[1][1]},
        {NINJA_RUN_R1 + 1, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[1][2]},
        {NINJA_RUN_R1 + 2, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[1][3]},
        {NINJA_RUN_R1 + 3, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[1][0]},
    },
    {
        {NINJA_RUN_R2 + 0, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[2][1]},
        {NINJA_RUN_R2 + 1, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[2][2]},
        {NINJA_RUN_R2 + 2, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[2][3]},
        {NINJA_RUN_R2 + 3, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[2][0]},
    },
    {
        {NINJA_RUN_R3 + 0, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[3][1]},
        {NINJA_RUN_R3 + 1, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[3][2]},
        {NINJA_RUN_R3 + 2, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[3][3]},
        {NINJA_RUN_R3 + 3, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[3][0]},
    },
    {
        {NINJA_RUN_R4 + 0, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[4][1]},
        {NINJA_RUN_R4 + 1, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[4][2]},
        {NINJA_RUN_R4 + 2, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[4][3]},
        {NINJA_RUN_R4 + 3, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[4][0]},
    },

};

#if 0
STATE s_NinjaRun[5][6] =
{

    {
        {NINJA_RUN_R0 + 0, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[0][1]},
        {NINJA_RUN_R0 + 1, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[0][2]},
        {NINJA_RUN_R0 + 2, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[0][3]},
        {NINJA_RUN_R0 + 3, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[0][4]},
        {NINJA_RUN_R0 + 4, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[0][5]},
        {NINJA_RUN_R0 + 5, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[0][0]},
    },
    {
        {NINJA_RUN_R1 + 0, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[1][1]},
        {NINJA_RUN_R1 + 1, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[1][2]},
        {NINJA_RUN_R1 + 2, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[1][3]},
        {NINJA_RUN_R1 + 3, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[1][4]},
        {NINJA_RUN_R1 + 4, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[1][5]},
        {NINJA_RUN_R1 + 5, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[1][0]},
    },
    {
        {NINJA_RUN_R2 + 0, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[2][1]},
        {NINJA_RUN_R2 + 1, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[2][2]},
        {NINJA_RUN_R2 + 2, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[2][3]},
        {NINJA_RUN_R2 + 3, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[2][4]},
        {NINJA_RUN_R2 + 4, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[2][5]},
        {NINJA_RUN_R2 + 5, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[2][0]},
    },
    {
        {NINJA_RUN_R3 + 0, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[3][1]},
        {NINJA_RUN_R3 + 1, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[3][2]},
        {NINJA_RUN_R3 + 2, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[3][3]},
        {NINJA_RUN_R3 + 3, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[3][4]},
        {NINJA_RUN_R3 + 4, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[3][5]},
        {NINJA_RUN_R3 + 5, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[3][0]},
    },
    {
        {NINJA_RUN_R4 + 0, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[4][1]},
        {NINJA_RUN_R4 + 1, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[4][2]},
        {NINJA_RUN_R4 + 2, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[4][3]},
        {NINJA_RUN_R4 + 3, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[4][4]},
        {NINJA_RUN_R4 + 4, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[4][5]},
        {NINJA_RUN_R4 + 5, NINJA_RATE | SF_TIC_ADJUST, DoNinjaMove, &s_NinjaRun[4][0]},
    },

};
#endif

STATEp sg_NinjaRun[] =
{
    s_NinjaRun[0],
    s_NinjaRun[1],
    s_NinjaRun[2],
    s_NinjaRun[3],
    s_NinjaRun[4]
};

//////////////////////
//
// NINJA STAND
//
//////////////////////

#define NINJA_STAND_RATE 10

STATE s_NinjaStand[5][1] =
{
    {
        {NINJA_STAND_R0 + 0, NINJA_STAND_RATE, DoNinjaMove, &s_NinjaStand[0][0]},
    },
    {
        {NINJA_STAND_R1 + 0, NINJA_STAND_RATE, DoNinjaMove, &s_NinjaStand[1][0]},
    },
    {
        {NINJA_STAND_R2 + 0, NINJA_STAND_RATE, DoNinjaMove, &s_NinjaStand[2][0]},
    },
    {
        {NINJA_STAND_R3 + 0, NINJA_STAND_RATE, DoNinjaMove, &s_NinjaStand[3][0]},
    },
    {
        {NINJA_STAND_R4 + 0, NINJA_STAND_RATE, DoNinjaMove, &s_NinjaStand[4][0]},
    },
};


STATEp sg_NinjaStand[] =
{
    s_NinjaStand[0],
    s_NinjaStand[1],
    s_NinjaStand[2],
    s_NinjaStand[3],
    s_NinjaStand[4]
};

//////////////////////
//
// NINJA RISE
//
//////////////////////

#define NINJA_RISE_RATE 10

STATE s_NinjaRise[5][3] =
{
    {
        {NINJA_KNEEL_R0 + 0, NINJA_RISE_RATE, NullNinja, &s_NinjaRise[0][1]},
        {NINJA_STAND_R0 + 0, NINJA_STAND_RATE, NullNinja, &s_NinjaRise[0][2]},
        {0, 0, NULL, (STATEp)sg_NinjaRun},
    },
    {
        {NINJA_KNEEL_R1 + 0, NINJA_RISE_RATE, NullNinja, &s_NinjaRise[1][1]},
        {NINJA_STAND_R1 + 0, NINJA_STAND_RATE, NullNinja, &s_NinjaRise[1][2]},
        {0, 0, NULL, (STATEp)sg_NinjaRun},
    },
    {
        {NINJA_KNEEL_R2 + 0, NINJA_RISE_RATE, NullNinja, &s_NinjaRise[2][1]},
        {NINJA_STAND_R2 + 0, NINJA_STAND_RATE, NullNinja, &s_NinjaRise[2][2]},
        {0, 0, NULL, (STATEp)sg_NinjaRun},
    },
    {
        {NINJA_KNEEL_R3 + 0, NINJA_RISE_RATE, NullNinja, &s_NinjaRise[3][1]},
        {NINJA_STAND_R3 + 0, NINJA_STAND_RATE, NullNinja, &s_NinjaRise[3][2]},
        {0, 0, NULL, (STATEp)sg_NinjaRun},
    },
    {
        {NINJA_KNEEL_R4 + 0, NINJA_RISE_RATE, NullNinja, &s_NinjaRise[4][1]},
        {NINJA_STAND_R4 + 0, NINJA_STAND_RATE, NullNinja, &s_NinjaRise[4][2]},
        {0, 0, NULL, (STATEp)sg_NinjaRun},
    },
};


STATEp sg_NinjaRise[] =
{
    s_NinjaRise[0],
    s_NinjaRise[1],
    s_NinjaRise[2],
    s_NinjaRise[3],
    s_NinjaRise[4]
};


//////////////////////
//
// NINJA CRAWL
//
//////////////////////


#define NINJA_CRAWL_RATE 14
STATE s_NinjaCrawl[5][4] =
{
    {
        {NINJA_CRAWL_R0 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[0][1]},
        {NINJA_CRAWL_R0 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[0][2]},
        {NINJA_CRAWL_R0 + 2, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[0][3]},
        {NINJA_CRAWL_R0 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[0][0]},
    },
    {
        {NINJA_CRAWL_R1 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[1][1]},
        {NINJA_CRAWL_R1 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[1][2]},
        {NINJA_CRAWL_R1 + 2, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[1][3]},
        {NINJA_CRAWL_R1 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[1][0]},
    },
    {
        {NINJA_CRAWL_R2 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[2][1]},
        {NINJA_CRAWL_R2 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[2][2]},
        {NINJA_CRAWL_R2 + 2, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[2][3]},
        {NINJA_CRAWL_R2 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[2][0]},
    },
    {
        {NINJA_CRAWL_R3 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[3][1]},
        {NINJA_CRAWL_R3 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[3][2]},
        {NINJA_CRAWL_R3 + 2, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[3][3]},
        {NINJA_CRAWL_R3 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[3][0]},
    },
    {
        {NINJA_CRAWL_R4 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[4][1]},
        {NINJA_CRAWL_R4 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[4][2]},
        {NINJA_CRAWL_R4 + 2, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[4][3]},
        {NINJA_CRAWL_R4 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaCrawl[4][0]},
    },
};


STATEp sg_NinjaCrawl[] =
{
    s_NinjaCrawl[0],
    s_NinjaCrawl[1],
    s_NinjaCrawl[2],
    s_NinjaCrawl[3],
    s_NinjaCrawl[4]
};

//////////////////////
//
// NINJA KNEEL_CRAWL
//
//////////////////////

#define NINJA_KNEEL_CRAWL_RATE 20

STATE s_NinjaKneelCrawl[5][5] =
{
    {
        {NINJA_KNEEL_R0 + 0, NINJA_KNEEL_CRAWL_RATE, NullNinja, &s_NinjaKneelCrawl[0][1]},
        {NINJA_CRAWL_R0 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[0][2]},
        {NINJA_CRAWL_R0 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[0][3]},
        {NINJA_CRAWL_R0 + 2, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[0][4]},
        {NINJA_CRAWL_R0 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[0][1]},
    },
    {
        {NINJA_KNEEL_R1 + 0, NINJA_KNEEL_CRAWL_RATE, NullNinja, &s_NinjaKneelCrawl[1][1]},
        {NINJA_CRAWL_R1 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[1][2]},
        {NINJA_CRAWL_R1 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[1][3]},
        {NINJA_CRAWL_R1 + 2, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[1][4]},
        {NINJA_CRAWL_R1 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[1][1]},
    },
    {
        {NINJA_KNEEL_R2 + 0, NINJA_KNEEL_CRAWL_RATE, NullNinja, &s_NinjaKneelCrawl[2][1]},
        {NINJA_CRAWL_R2 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[2][2]},
        {NINJA_CRAWL_R2 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[2][3]},
        {NINJA_CRAWL_R2 + 2, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[2][4]},
        {NINJA_CRAWL_R2 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[2][1]},
    },
    {
        {NINJA_KNEEL_R3 + 0, NINJA_KNEEL_CRAWL_RATE, NullNinja, &s_NinjaKneelCrawl[3][1]},
        {NINJA_CRAWL_R3 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[3][2]},
        {NINJA_CRAWL_R3 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[3][3]},
        {NINJA_CRAWL_R3 + 2, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[3][4]},
        {NINJA_CRAWL_R3 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[3][1]},
    },
    {
        {NINJA_KNEEL_R4 + 0, NINJA_KNEEL_CRAWL_RATE, NullNinja, &s_NinjaKneelCrawl[4][1]},
        {NINJA_CRAWL_R4 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[4][2]},
        {NINJA_CRAWL_R4 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[4][3]},
        {NINJA_CRAWL_R4 + 2, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[4][4]},
        {NINJA_CRAWL_R4 + 1, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaKneelCrawl[4][1]},
    },
};


STATEp sg_NinjaKneelCrawl[] =
{
    s_NinjaKneelCrawl[0],
    s_NinjaKneelCrawl[1],
    s_NinjaKneelCrawl[2],
    s_NinjaKneelCrawl[3],
    s_NinjaKneelCrawl[4]
};


//////////////////////
//
// NINJA DUCK
//
//////////////////////

#define NINJA_DUCK_RATE 10

STATE s_NinjaDuck[5][2] =
{
    {
        {NINJA_KNEEL_R0 + 0, NINJA_DUCK_RATE, NullNinja, &s_NinjaDuck[0][1]},
        {NINJA_CRAWL_R0 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaDuck[0][1]},
    },
    {
        {NINJA_KNEEL_R1 + 0, NINJA_DUCK_RATE, NullNinja, &s_NinjaDuck[1][1]},
        {NINJA_CRAWL_R1 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaDuck[1][1]},
    },
    {
        {NINJA_KNEEL_R2 + 0, NINJA_DUCK_RATE, NullNinja, &s_NinjaDuck[2][1]},
        {NINJA_CRAWL_R2 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaDuck[2][1]},
    },
    {
        {NINJA_KNEEL_R3 + 0, NINJA_DUCK_RATE, NullNinja, &s_NinjaDuck[3][1]},
        {NINJA_CRAWL_R3 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaDuck[3][1]},
    },
    {
        {NINJA_KNEEL_R4 + 0, NINJA_DUCK_RATE, NullNinja, &s_NinjaDuck[4][1]},
        {NINJA_CRAWL_R4 + 0, NINJA_CRAWL_RATE, DoNinjaMove, &s_NinjaDuck[4][1]},
    },
};


STATEp sg_NinjaDuck[] =
{
    s_NinjaDuck[0],
    s_NinjaDuck[1],
    s_NinjaDuck[2],
    s_NinjaDuck[3],
    s_NinjaDuck[4]
};


//////////////////////
//
// NINJA SIT
//
//////////////////////

STATE s_NinjaSit[5][1] =
{
    {
        {NINJA_KNEEL_R0 + 0, NINJA_RISE_RATE, DoNinjaMove, &s_NinjaSit[0][0]},
    },
    {
        {NINJA_KNEEL_R1 + 0, NINJA_RISE_RATE, DoNinjaMove, &s_NinjaSit[1][0]},
    },
    {
        {NINJA_KNEEL_R2 + 0, NINJA_RISE_RATE, DoNinjaMove, &s_NinjaSit[2][0]},
    },
    {
        {NINJA_KNEEL_R3 + 0, NINJA_RISE_RATE, DoNinjaMove, &s_NinjaSit[3][0]},
    },
    {
        {NINJA_KNEEL_R4 + 0, NINJA_RISE_RATE, DoNinjaMove, &s_NinjaSit[4][0]},
    },
};


STATEp sg_NinjaSit[] =
{
    s_NinjaSit[0],
    s_NinjaSit[1],
    s_NinjaSit[2],
    s_NinjaSit[3],
    s_NinjaSit[4]
};

//////////////////////
//
// NINJA CEILING
//
//////////////////////

ANIMATOR DoNinjaCeiling;
STATE s_NinjaCeiling[5][1] =
{
    {
        {NINJA_KNEEL_R0 + 0, NINJA_RISE_RATE, DoNinjaCeiling, &s_NinjaCeiling[0][0]},
    },
    {
        {NINJA_KNEEL_R1 + 0, NINJA_RISE_RATE, DoNinjaCeiling, &s_NinjaCeiling[1][0]},
    },
    {
        {NINJA_KNEEL_R2 + 0, NINJA_RISE_RATE, DoNinjaCeiling, &s_NinjaCeiling[2][0]},
    },
    {
        {NINJA_KNEEL_R3 + 0, NINJA_RISE_RATE, DoNinjaCeiling, &s_NinjaCeiling[3][0]},
    },
    {
        {NINJA_KNEEL_R4 + 0, NINJA_RISE_RATE, DoNinjaCeiling, &s_NinjaCeiling[4][0]},
    },
};


STATEp sg_NinjaCeiling[] =
{
    s_NinjaCeiling[0],
    s_NinjaCeiling[1],
    s_NinjaCeiling[2],
    s_NinjaCeiling[3],
    s_NinjaCeiling[4]
};


//////////////////////
//
// NINJA JUMP
//
//////////////////////

#define NINJA_JUMP_RATE 24

STATE s_NinjaJump[5][2] =
{
    {
        {NINJA_JUMP_R0 + 0, NINJA_JUMP_RATE, DoNinjaMove, &s_NinjaJump[0][1]},
        {NINJA_JUMP_R0 + 1, NINJA_JUMP_RATE, DoNinjaMove, &s_NinjaJump[0][1]},
    },
    {
        {NINJA_JUMP_R1 + 0, NINJA_JUMP_RATE, DoNinjaMove, &s_NinjaJump[1][1]},
        {NINJA_JUMP_R1 + 1, NINJA_JUMP_RATE, DoNinjaMove, &s_NinjaJump[1][1]},
    },
    {
        {NINJA_JUMP_R2 + 0, NINJA_JUMP_RATE, DoNinjaMove, &s_NinjaJump[2][1]},
        {NINJA_JUMP_R2 + 1, NINJA_JUMP_RATE, DoNinjaMove, &s_NinjaJump[2][1]},
    },
    {
        {NINJA_JUMP_R3 + 0, NINJA_JUMP_RATE, DoNinjaMove, &s_NinjaJump[3][1]},
        {NINJA_JUMP_R3 + 1, NINJA_JUMP_RATE, DoNinjaMove, &s_NinjaJump[3][1]},
    },
    {
        {NINJA_JUMP_R4 + 0, NINJA_JUMP_RATE, DoNinjaMove, &s_NinjaJump[4][1]},
        {NINJA_JUMP_R4 + 1, NINJA_JUMP_RATE, DoNinjaMove, &s_NinjaJump[4][1]},
    },
};


STATEp sg_NinjaJump[] =
{
    s_NinjaJump[0],
    s_NinjaJump[1],
    s_NinjaJump[2],
    s_NinjaJump[3],
    s_NinjaJump[4]
};


//////////////////////
//
// NINJA FALL
//
//////////////////////

#define NINJA_FALL_RATE 16

STATE s_NinjaFall[5][2] =
{
    {
        {NINJA_JUMP_R0 + 1, NINJA_FALL_RATE, DoNinjaMove, &s_NinjaFall[0][1]},
        {NINJA_JUMP_R0 + 2, NINJA_FALL_RATE, DoNinjaMove, &s_NinjaFall[0][1]},
    },
    {
        {NINJA_JUMP_R1 + 1, NINJA_FALL_RATE, DoNinjaMove, &s_NinjaFall[1][1]},
        {NINJA_JUMP_R1 + 2, NINJA_FALL_RATE, DoNinjaMove, &s_NinjaFall[1][1]},
    },
    {
        {NINJA_JUMP_R2 + 1, NINJA_FALL_RATE, DoNinjaMove, &s_NinjaFall[2][1]},
        {NINJA_JUMP_R2 + 2, NINJA_FALL_RATE, DoNinjaMove, &s_NinjaFall[2][1]},
    },
    {
        {NINJA_JUMP_R3 + 1, NINJA_FALL_RATE, DoNinjaMove, &s_NinjaFall[3][1]},
        {NINJA_JUMP_R3 + 2, NINJA_FALL_RATE, DoNinjaMove, &s_NinjaFall[3][1]},
    },
    {
        {NINJA_JUMP_R4 + 1, NINJA_FALL_RATE, DoNinjaMove, &s_NinjaFall[4][1]},
        {NINJA_JUMP_R4 + 2, NINJA_FALL_RATE, DoNinjaMove, &s_NinjaFall[4][1]},
    },
};


STATEp sg_NinjaFall[] =
{
    s_NinjaFall[0],
    s_NinjaFall[1],
    s_NinjaFall[2],
    s_NinjaFall[3],
    s_NinjaFall[4]
};

//////////////////////
//
// NINJA SWIM
//
//////////////////////


#define NINJA_SWIM_RATE 18
STATE s_NinjaSwim[5][3] =
{
    {
        {NINJA_SWIM_R0 + 1, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[0][1]},
        {NINJA_SWIM_R0 + 2, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[0][2]},
        {NINJA_SWIM_R0 + 3, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[0][0]},
    },
    {
        {NINJA_SWIM_R1 + 1, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[1][1]},
        {NINJA_SWIM_R1 + 2, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[1][2]},
        {NINJA_SWIM_R1 + 3, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[1][0]},
    },
    {
        {NINJA_SWIM_R2 + 1, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[2][1]},
        {NINJA_SWIM_R2 + 2, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[2][2]},
        {NINJA_SWIM_R2 + 3, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[2][0]},
    },
    {
        {NINJA_SWIM_R3 + 1, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[3][1]},
        {NINJA_SWIM_R3 + 2, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[3][2]},
        {NINJA_SWIM_R3 + 3, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[3][0]},
    },
    {
        {NINJA_SWIM_R4 + 1, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[4][1]},
        {NINJA_SWIM_R4 + 2, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[4][2]},
        {NINJA_SWIM_R4 + 3, NINJA_SWIM_RATE, DoNinjaMove, &s_NinjaSwim[4][0]},
    },
};


STATEp sg_NinjaSwim[] =
{
    s_NinjaSwim[0],
    s_NinjaSwim[1],
    s_NinjaSwim[2],
    s_NinjaSwim[3],
    s_NinjaSwim[4]
};

//////////////////////
//
// NINJA DIVE
//
//////////////////////


#define NINJA_DIVE_RATE 23

STATE s_NinjaDive[5][4] =
{
    {
        {NINJA_SWIM_R0 + 0, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[0][1]},
        {NINJA_SWIM_R0 + 1, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[0][2]},
        {NINJA_SWIM_R0 + 2, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[0][3]},
        {NINJA_SWIM_R0 + 3, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[0][0]},
    },
    {
        {NINJA_SWIM_R1 + 0, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[1][1]},
        {NINJA_SWIM_R1 + 1, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[1][2]},
        {NINJA_SWIM_R1 + 2, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[1][3]},
        {NINJA_SWIM_R1 + 3, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[1][0]},
    },
    {
        {NINJA_SWIM_R2 + 0, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[2][1]},
        {NINJA_SWIM_R2 + 1, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[2][2]},
        {NINJA_SWIM_R2 + 2, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[2][3]},
        {NINJA_SWIM_R2 + 3, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[2][0]},
    },
    {
        {NINJA_SWIM_R3 + 0, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[3][1]},
        {NINJA_SWIM_R3 + 1, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[3][2]},
        {NINJA_SWIM_R3 + 2, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[3][3]},
        {NINJA_SWIM_R3 + 3, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[3][0]},
    },
    {
        {NINJA_SWIM_R4 + 0, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[4][1]},
        {NINJA_SWIM_R4 + 1, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[4][2]},
        {NINJA_SWIM_R4 + 2, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[4][3]},
        {NINJA_SWIM_R4 + 3, NINJA_DIVE_RATE, DoNinjaMove, &s_NinjaDive[4][0]},
    },
};

STATEp sg_NinjaDive[] =
{
    s_NinjaDive[0],
    s_NinjaDive[1],
    s_NinjaDive[2],
    s_NinjaDive[3],
    s_NinjaDive[4]
};

//////////////////////
//
// NINJA CLIMB
//
//////////////////////


#define NINJA_CLIMB_RATE 20
STATE s_NinjaClimb[5][4] =
{
    {
        {NINJA_CLIMB_R0 + 0, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[0][1]},
        {NINJA_CLIMB_R0 + 1, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[0][2]},
        {NINJA_CLIMB_R0 + 2, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[0][3]},
        {NINJA_CLIMB_R0 + 3, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[0][0]},
    },
    {
        {NINJA_CLIMB_R1 + 0, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[1][1]},
        {NINJA_CLIMB_R1 + 1, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[1][2]},
        {NINJA_CLIMB_R1 + 2, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[1][3]},
        {NINJA_CLIMB_R1 + 3, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[1][0]},
    },
    {
        {NINJA_CLIMB_R4 + 0, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[2][1]},
        {NINJA_CLIMB_R4 + 1, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[2][2]},
        {NINJA_CLIMB_R4 + 2, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[2][3]},
        {NINJA_CLIMB_R4 + 3, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[2][0]},
    },
    {
        {NINJA_CLIMB_R3 + 0, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[3][1]},
        {NINJA_CLIMB_R3 + 1, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[3][2]},
        {NINJA_CLIMB_R3 + 2, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[3][3]},
        {NINJA_CLIMB_R3 + 3, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[3][0]},
    },
    {
        {NINJA_CLIMB_R2 + 0, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[4][1]},
        {NINJA_CLIMB_R2 + 1, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[4][2]},
        {NINJA_CLIMB_R2 + 2, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[4][3]},
        {NINJA_CLIMB_R2 + 3, NINJA_CLIMB_RATE, DoNinjaMove, &s_NinjaClimb[4][0]},
    },
};

STATEp sg_NinjaClimb[] =
{
    s_NinjaClimb[0],
    s_NinjaClimb[1],
    s_NinjaClimb[2],
    s_NinjaClimb[3],
    s_NinjaClimb[4]
};


//////////////////////
//
// NINJA FLY
//
//////////////////////

#define NINJA_FLY_RATE 12

STATE s_NinjaFly[5][1] =
{
    {
        {NINJA_FLY_R0 + 0, NINJA_FLY_RATE, DoNinjaMove, &s_NinjaFly[0][0]},
    },
    {
        {NINJA_FLY_R1 + 0, NINJA_FLY_RATE, DoNinjaMove, &s_NinjaFly[1][0]},
    },
    {
        {NINJA_FLY_R2 + 0, NINJA_FLY_RATE, DoNinjaMove, &s_NinjaFly[2][0]},
    },
    {
        {NINJA_FLY_R3 + 0, NINJA_FLY_RATE, DoNinjaMove, &s_NinjaFly[3][0]},
    },
    {
        {NINJA_FLY_R4 + 0, NINJA_FLY_RATE, DoNinjaMove, &s_NinjaFly[4][0]},
    },
};

STATEp sg_NinjaFly[] =
{
    s_NinjaFly[0],
    s_NinjaFly[1],
    s_NinjaFly[2],
    s_NinjaFly[3],
    s_NinjaFly[4]
};

//////////////////////
//
// NINJA PAIN
//
//////////////////////

#define NINJA_PAIN_RATE 15
ANIMATOR DoNinjaPain;

STATE s_NinjaPain[5][2] =
{
    {
        {NINJA_PAIN_R0 + 0, NINJA_PAIN_RATE, DoNinjaPain, &s_NinjaPain[0][1]},
        {NINJA_PAIN_R0 + 1, NINJA_PAIN_RATE, DoNinjaPain, &s_NinjaPain[0][1]},
    },
    {
        {NINJA_STAND_R1 + 0, NINJA_PAIN_RATE, DoNinjaPain, &s_NinjaPain[1][1]},
        {NINJA_STAND_R1 + 0, NINJA_PAIN_RATE, DoNinjaPain, &s_NinjaPain[1][1]},
    },
    {
        {NINJA_STAND_R2 + 0, NINJA_PAIN_RATE, DoNinjaPain, &s_NinjaPain[2][1]},
        {NINJA_STAND_R2 + 0, NINJA_PAIN_RATE, DoNinjaPain, &s_NinjaPain[2][1]},
    },
    {
        {NINJA_STAND_R3 + 0, NINJA_PAIN_RATE, DoNinjaPain, &s_NinjaPain[3][1]},
        {NINJA_STAND_R3 + 0, NINJA_PAIN_RATE, DoNinjaPain, &s_NinjaPain[3][1]},
    },
    {
        {NINJA_STAND_R4 + 0, NINJA_PAIN_RATE, DoNinjaPain, &s_NinjaPain[4][1]},
        {NINJA_STAND_R4 + 0, NINJA_PAIN_RATE, DoNinjaPain, &s_NinjaPain[4][1]},
    },
};

STATEp sg_NinjaPain[] =
{
    s_NinjaPain[0],
    s_NinjaPain[1],
    s_NinjaPain[2],
    s_NinjaPain[3],
    s_NinjaPain[4]
};

//////////////////////
//
// NINJA STAR
//
//////////////////////

#define NINJA_STAR_RATE 18
ANIMATOR InitEnemyStar;

STATE s_NinjaStar[5][6] =
{
    {
        {NINJA_THROW_R0 + 0, NINJA_STAR_RATE * 2,     NullNinja,            &s_NinjaStar[0][1]},
        {NINJA_THROW_R0 + 0, NINJA_STAR_RATE,       NullNinja,          &s_NinjaStar[0][2]},
        {NINJA_THROW_R0 + 1, 0 | SF_QUICK_CALL,         InitEnemyStar,      &s_NinjaStar[0][3]},
        {NINJA_THROW_R0 + 1, NINJA_STAR_RATE * 2,     NullNinja,            &s_NinjaStar[0][4]},
        {NINJA_THROW_R0 + 2, 0 | SF_QUICK_CALL,         InitActorDecide,    &s_NinjaStar[0][5]},
        {NINJA_THROW_R0 + 2, NINJA_STAR_RATE,       DoNinjaMove,        &s_NinjaStar[0][5]},
    },
    {
        {NINJA_THROW_R1 + 0, NINJA_STAR_RATE * 2,     NullNinja,            &s_NinjaStar[1][1]},
        {NINJA_THROW_R1 + 0, NINJA_STAR_RATE,       NullNinja,          &s_NinjaStar[1][2]},
        {NINJA_THROW_R1 + 1, 0 | SF_QUICK_CALL,         InitEnemyStar,      &s_NinjaStar[1][3]},
        {NINJA_THROW_R1 + 1, NINJA_STAR_RATE * 2,     NullNinja,            &s_NinjaStar[1][4]},
        {NINJA_THROW_R1 + 2, 0 | SF_QUICK_CALL,         InitActorDecide,    &s_NinjaStar[1][5]},
        {NINJA_THROW_R1 + 2, NINJA_STAR_RATE,       DoNinjaMove,        &s_NinjaStar[1][5]},
    },
    {
        {NINJA_THROW_R2 + 0, NINJA_STAR_RATE * 2,     NullNinja,            &s_NinjaStar[2][1]},
        {NINJA_THROW_R2 + 0, NINJA_STAR_RATE,       NullNinja,          &s_NinjaStar[2][2]},
        {NINJA_THROW_R2 + 1, 0 | SF_QUICK_CALL,         InitEnemyStar,      &s_NinjaStar[2][3]},
        {NINJA_THROW_R2 + 1, NINJA_STAR_RATE * 2,     NullNinja,            &s_NinjaStar[2][4]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL,         InitActorDecide,    &s_NinjaStar[2][5]},
        {NINJA_THROW_R2 + 2, NINJA_STAR_RATE,       DoNinjaMove,        &s_NinjaStar[2][5]},
    },
    {
        {NINJA_THROW_R2 + 0, NINJA_STAR_RATE * 2,     NullNinja,            &s_NinjaStar[3][1]},
        {NINJA_THROW_R2 + 0, NINJA_STAR_RATE,       NullNinja,          &s_NinjaStar[3][2]},
        {NINJA_THROW_R2 + 1, 0 | SF_QUICK_CALL,         InitEnemyStar,      &s_NinjaStar[3][3]},
        {NINJA_THROW_R2 + 1, NINJA_STAR_RATE * 2,     NullNinja,            &s_NinjaStar[3][4]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL,         InitActorDecide,    &s_NinjaStar[3][5]},
        {NINJA_THROW_R2 + 2, NINJA_STAR_RATE,       DoNinjaMove,        &s_NinjaStar[3][5]},
    },
    {
        {NINJA_THROW_R2 + 0, NINJA_STAR_RATE * 2,     NullNinja,            &s_NinjaStar[4][1]},
        {NINJA_THROW_R2 + 0, NINJA_STAR_RATE,       NullNinja,          &s_NinjaStar[4][2]},
        {NINJA_THROW_R2 + 1, 0 | SF_QUICK_CALL,         InitEnemyStar,      &s_NinjaStar[4][3]},
        {NINJA_THROW_R2 + 1, NINJA_STAR_RATE * 2,     NullNinja,            &s_NinjaStar[4][4]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL,         InitActorDecide,    &s_NinjaStar[4][5]},
        {NINJA_THROW_R2 + 2, NINJA_STAR_RATE,       DoNinjaMove,        &s_NinjaStar[4][5]},
    },
};

STATEp sg_NinjaStar[] =
{
    s_NinjaStar[0],
    s_NinjaStar[1],
    s_NinjaStar[2],
    s_NinjaStar[3],
    s_NinjaStar[4]
};

//////////////////////
//
// NINJA MIRV
//
//////////////////////

#define NINJA_MIRV_RATE 18
ANIMATOR InitEnemyMirv;

STATE s_NinjaMirv[5][6] =
{
    {
        {NINJA_THROW_R0 + 0, NINJA_MIRV_RATE * 2, NullNinja, &s_NinjaMirv[0][1]},
        {NINJA_THROW_R0 + 1, NINJA_MIRV_RATE, NullNinja, &s_NinjaMirv[0][2]},
        {NINJA_THROW_R0 + 2, 0 | SF_QUICK_CALL, InitEnemyMirv, &s_NinjaMirv[0][3]},
        {NINJA_THROW_R0 + 2, NINJA_MIRV_RATE * 2, NullNinja, &s_NinjaMirv[0][4]},
        {NINJA_THROW_R0 + 2, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaMirv[0][5]},
        {NINJA_THROW_R0 + 2, NINJA_MIRV_RATE, DoNinjaMove, &s_NinjaMirv[0][5]},
    },
    {
        {NINJA_THROW_R1 + 0, NINJA_MIRV_RATE * 2, NullNinja, &s_NinjaMirv[1][1]},
        {NINJA_THROW_R1 + 1, NINJA_MIRV_RATE, NullNinja, &s_NinjaMirv[1][2]},
        {NINJA_THROW_R1 + 2, 0 | SF_QUICK_CALL, InitEnemyMirv, &s_NinjaMirv[1][3]},
        {NINJA_THROW_R1 + 2, NINJA_MIRV_RATE * 2, NullNinja, &s_NinjaMirv[1][4]},
        {NINJA_THROW_R1 + 2, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaMirv[1][5]},
        {NINJA_THROW_R1 + 2, NINJA_MIRV_RATE, DoNinjaMove, &s_NinjaMirv[1][5]},
    },
    {
        {NINJA_THROW_R2 + 0, NINJA_MIRV_RATE * 2, NullNinja, &s_NinjaMirv[2][1]},
        {NINJA_THROW_R2 + 1, NINJA_MIRV_RATE, NullNinja, &s_NinjaMirv[2][2]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitEnemyMirv, &s_NinjaMirv[2][3]},
        {NINJA_THROW_R2 + 2, NINJA_MIRV_RATE * 2, NullNinja, &s_NinjaMirv[2][4]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaMirv[2][5]},
        {NINJA_THROW_R2 + 2, NINJA_MIRV_RATE, DoNinjaMove, &s_NinjaMirv[2][5]},
    },
    {
        {NINJA_THROW_R2 + 0, NINJA_MIRV_RATE * 2, NullNinja, &s_NinjaMirv[3][1]},
        {NINJA_THROW_R2 + 1, NINJA_MIRV_RATE, NullNinja, &s_NinjaMirv[3][2]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitEnemyMirv, &s_NinjaMirv[3][3]},
        {NINJA_THROW_R2 + 2, NINJA_MIRV_RATE * 2, NullNinja, &s_NinjaMirv[3][4]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaMirv[3][5]},
        {NINJA_THROW_R2 + 2, NINJA_MIRV_RATE, DoNinjaMove, &s_NinjaMirv[3][5]},
    },
    {
        {NINJA_THROW_R2 + 0, NINJA_MIRV_RATE * 2, NullNinja, &s_NinjaMirv[4][1]},
        {NINJA_THROW_R2 + 1, NINJA_MIRV_RATE, NullNinja, &s_NinjaMirv[4][2]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitEnemyMirv, &s_NinjaMirv[4][3]},
        {NINJA_THROW_R2 + 2, NINJA_MIRV_RATE * 2, NullNinja, &s_NinjaMirv[4][4]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaMirv[4][5]},
        {NINJA_THROW_R2 + 2, NINJA_MIRV_RATE, DoNinjaMove, &s_NinjaStar[4][5]},
    },
};


STATEp sg_NinjaMirv[] =
{
    s_NinjaMirv[0],
    s_NinjaMirv[1],
    s_NinjaMirv[2],
    s_NinjaMirv[3],
    s_NinjaMirv[4]
};

//////////////////////
//
// NINJA NAPALM
//
//////////////////////

#define NINJA_NAPALM_RATE 18
ANIMATOR InitEnemyNapalm;

STATE s_NinjaNapalm[5][6] =
{
    {
        {NINJA_THROW_R0 + 0, NINJA_NAPALM_RATE * 2, NullNinja, &s_NinjaNapalm[0][1]},
        {NINJA_THROW_R0 + 1, NINJA_NAPALM_RATE, NullNinja, &s_NinjaNapalm[0][2]},
        {NINJA_THROW_R0 + 2, 0 | SF_QUICK_CALL, InitEnemyNapalm, &s_NinjaNapalm[0][3]},
        {NINJA_THROW_R0 + 2, NINJA_NAPALM_RATE * 2, NullNinja, &s_NinjaNapalm[0][4]},
        {NINJA_THROW_R0 + 2, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaNapalm[0][5]},
        {NINJA_THROW_R0 + 2, NINJA_NAPALM_RATE, DoNinjaMove, &s_NinjaNapalm[0][5]},
    },
    {
        {NINJA_THROW_R1 + 0, NINJA_NAPALM_RATE * 2, NullNinja, &s_NinjaNapalm[1][1]},
        {NINJA_THROW_R1 + 1, NINJA_NAPALM_RATE, NullNinja, &s_NinjaNapalm[1][2]},
        {NINJA_THROW_R1 + 2, 0 | SF_QUICK_CALL, InitEnemyNapalm, &s_NinjaNapalm[1][3]},
        {NINJA_THROW_R1 + 2, NINJA_NAPALM_RATE * 2, NullNinja, &s_NinjaNapalm[1][4]},
        {NINJA_THROW_R1 + 2, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaNapalm[1][5]},
        {NINJA_THROW_R1 + 2, NINJA_NAPALM_RATE, DoNinjaMove, &s_NinjaNapalm[1][5]},
    },
    {
        {NINJA_THROW_R2 + 0, NINJA_NAPALM_RATE * 2, NullNinja, &s_NinjaNapalm[2][1]},
        {NINJA_THROW_R2 + 1, NINJA_NAPALM_RATE, NullNinja, &s_NinjaNapalm[2][2]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitEnemyNapalm, &s_NinjaNapalm[2][3]},
        {NINJA_THROW_R2 + 2, NINJA_NAPALM_RATE * 2, NullNinja, &s_NinjaNapalm[2][4]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaNapalm[2][5]},
        {NINJA_THROW_R2 + 2, NINJA_NAPALM_RATE, DoNinjaMove, &s_NinjaNapalm[2][5]},
    },
    {
        {NINJA_THROW_R2 + 0, NINJA_NAPALM_RATE * 2, NullNinja, &s_NinjaNapalm[3][1]},
        {NINJA_THROW_R2 + 1, NINJA_NAPALM_RATE, NullNinja, &s_NinjaNapalm[3][2]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitEnemyNapalm, &s_NinjaNapalm[3][3]},
        {NINJA_THROW_R2 + 2, NINJA_NAPALM_RATE * 2, NullNinja, &s_NinjaNapalm[3][4]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaNapalm[3][5]},
        {NINJA_THROW_R2 + 2, NINJA_NAPALM_RATE, DoNinjaMove, &s_NinjaNapalm[3][5]},
    },
    {
        {NINJA_THROW_R2 + 0, NINJA_NAPALM_RATE * 2, NullNinja, &s_NinjaNapalm[4][1]},
        {NINJA_THROW_R2 + 1, NINJA_NAPALM_RATE, NullNinja, &s_NinjaNapalm[4][2]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitEnemyNapalm, &s_NinjaNapalm[4][3]},
        {NINJA_THROW_R2 + 2, NINJA_NAPALM_RATE * 2, NullNinja, &s_NinjaNapalm[4][4]},
        {NINJA_THROW_R2 + 2, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaNapalm[4][5]},
        {NINJA_THROW_R2 + 2, NINJA_NAPALM_RATE, DoNinjaMove, &s_NinjaNapalm[4][5]},
    },
};


STATEp sg_NinjaNapalm[] =
{
    s_NinjaNapalm[0],
    s_NinjaNapalm[1],
    s_NinjaNapalm[2],
    s_NinjaNapalm[3],
    s_NinjaNapalm[4]
};


//////////////////////
//
// NINJA ROCKET
//
//////////////////////

#define NINJA_ROCKET_RATE 14
ANIMATOR InitEnemyRocket;

STATE s_NinjaRocket[5][5] =
{
    {
        {NINJA_STAND_R0 + 0, NINJA_ROCKET_RATE * 2, NullNinja, &s_NinjaRocket[0][1]},
        {NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyRocket, &s_NinjaRocket[0][2]},
        {NINJA_STAND_R0 + 0, NINJA_ROCKET_RATE, NullNinja, &s_NinjaRocket[0][3]},
        {NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaRocket[0][4]},
        {NINJA_STAND_R0 + 0, NINJA_ROCKET_RATE, DoNinjaMove, &s_NinjaRocket[0][4]},
    },
    {
        {NINJA_STAND_R1 + 0, NINJA_ROCKET_RATE * 2, NullNinja, &s_NinjaRocket[1][1]},
        {NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyRocket, &s_NinjaRocket[1][2]},
        {NINJA_STAND_R1 + 0, NINJA_ROCKET_RATE, NullNinja, &s_NinjaRocket[1][3]},
        {NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaRocket[1][4]},
        {NINJA_STAND_R1 + 0, NINJA_ROCKET_RATE, DoNinjaMove, &s_NinjaRocket[1][4]},
    },
    {
        {NINJA_STAND_R2 + 0, NINJA_ROCKET_RATE * 2, NullNinja, &s_NinjaRocket[2][1]},
        {NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyRocket, &s_NinjaRocket[2][2]},
        {NINJA_STAND_R2 + 0, NINJA_ROCKET_RATE, NullNinja, &s_NinjaRocket[2][3]},
        {NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaRocket[2][4]},
        {NINJA_STAND_R2 + 0, NINJA_ROCKET_RATE, DoNinjaMove, &s_NinjaRocket[2][4]},
    },
    {
        {NINJA_STAND_R3 + 0, NINJA_ROCKET_RATE * 2, NullNinja, &s_NinjaRocket[3][1]},
        {NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyRocket, &s_NinjaRocket[3][2]},
        {NINJA_STAND_R3 + 0, NINJA_ROCKET_RATE, NullNinja, &s_NinjaRocket[3][3]},
        {NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaRocket[3][4]},
        {NINJA_STAND_R3 + 0, NINJA_ROCKET_RATE, DoNinjaMove, &s_NinjaRocket[3][4]},
    },
    {
        {NINJA_STAND_R4 + 0, NINJA_ROCKET_RATE * 2, NullNinja, &s_NinjaRocket[4][1]},
        {NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyRocket, &s_NinjaRocket[4][2]},
        {NINJA_STAND_R4 + 0, NINJA_ROCKET_RATE, NullNinja, &s_NinjaRocket[4][3]},
        {NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaRocket[4][4]},
        {NINJA_STAND_R4 + 0, NINJA_ROCKET_RATE, DoNinjaMove, &s_NinjaRocket[4][4]},
    },
};


STATEp sg_NinjaRocket[] =
{
    s_NinjaRocket[0],
    s_NinjaRocket[1],
    s_NinjaRocket[2],
    s_NinjaRocket[3],
    s_NinjaRocket[4]
};

//////////////////////
//
// NINJA ROCKET
//
//////////////////////

#define NINJA_ROCKET_RATE 14
ANIMATOR InitSpriteGrenade;

STATE s_NinjaGrenade[5][5] =
{
    {
        {NINJA_STAND_R0 + 0, NINJA_ROCKET_RATE * 2, NullNinja, &s_NinjaGrenade[0][1]},
        {NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitSpriteGrenade, &s_NinjaGrenade[0][2]},
        {NINJA_STAND_R0 + 0, NINJA_ROCKET_RATE, NullNinja, &s_NinjaGrenade[0][3]},
        {NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaGrenade[0][4]},
        {NINJA_STAND_R0 + 0, NINJA_ROCKET_RATE, DoNinjaMove, &s_NinjaGrenade[0][4]},
    },
    {
        {NINJA_STAND_R1 + 0, NINJA_ROCKET_RATE * 2, NullNinja, &s_NinjaGrenade[1][1]},
        {NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitSpriteGrenade, &s_NinjaGrenade[1][2]},
        {NINJA_STAND_R1 + 0, NINJA_ROCKET_RATE, NullNinja, &s_NinjaGrenade[1][3]},
        {NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaGrenade[1][4]},
        {NINJA_STAND_R1 + 0, NINJA_ROCKET_RATE, DoNinjaMove, &s_NinjaGrenade[1][4]},
    },
    {
        {NINJA_STAND_R2 + 0, NINJA_ROCKET_RATE * 2, NullNinja, &s_NinjaGrenade[2][1]},
        {NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitSpriteGrenade, &s_NinjaGrenade[2][2]},
        {NINJA_STAND_R2 + 0, NINJA_ROCKET_RATE, NullNinja, &s_NinjaGrenade[2][3]},
        {NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaGrenade[2][4]},
        {NINJA_STAND_R2 + 0, NINJA_ROCKET_RATE, DoNinjaMove, &s_NinjaGrenade[2][4]},
    },
    {
        {NINJA_STAND_R3 + 0, NINJA_ROCKET_RATE * 2, NullNinja, &s_NinjaGrenade[3][1]},
        {NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitSpriteGrenade, &s_NinjaGrenade[3][2]},
        {NINJA_STAND_R3 + 0, NINJA_ROCKET_RATE, NullNinja, &s_NinjaGrenade[3][3]},
        {NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaGrenade[3][4]},
        {NINJA_STAND_R3 + 0, NINJA_ROCKET_RATE, DoNinjaMove, &s_NinjaGrenade[3][4]},
    },
    {
        {NINJA_STAND_R4 + 0, NINJA_ROCKET_RATE * 2, NullNinja, &s_NinjaGrenade[4][1]},
        {NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitSpriteGrenade, &s_NinjaGrenade[4][2]},
        {NINJA_STAND_R4 + 0, NINJA_ROCKET_RATE, NullNinja, &s_NinjaGrenade[4][3]},
        {NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaGrenade[4][4]},
        {NINJA_STAND_R4 + 0, NINJA_ROCKET_RATE, DoNinjaMove, &s_NinjaGrenade[4][4]},
    },
};


STATEp sg_NinjaGrenade[] =
{
    s_NinjaGrenade[0],
    s_NinjaGrenade[1],
    s_NinjaGrenade[2],
    s_NinjaGrenade[3],
    s_NinjaGrenade[4]
};


//////////////////////
//
// NINJA FLASHBOMB
//
//////////////////////

#define NINJA_FLASHBOMB_RATE 14
ANIMATOR InitFlashBomb;

STATE s_NinjaFlashBomb[5][5] =
{
    {
        {NINJA_STAND_R0 + 0, NINJA_FLASHBOMB_RATE * 2, NullNinja, &s_NinjaFlashBomb[0][1]},
        {NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitFlashBomb, &s_NinjaFlashBomb[0][2]},
        {NINJA_STAND_R0 + 0, NINJA_FLASHBOMB_RATE, NullNinja, &s_NinjaFlashBomb[0][3]},
        {NINJA_STAND_R0 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaFlashBomb[0][4]},
        {NINJA_STAND_R0 + 0, NINJA_FLASHBOMB_RATE, DoNinjaMove, &s_NinjaFlashBomb[0][4]},
    },
    {
        {NINJA_STAND_R1 + 0, NINJA_FLASHBOMB_RATE * 2, NullNinja, &s_NinjaFlashBomb[1][1]},
        {NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitFlashBomb, &s_NinjaFlashBomb[1][2]},
        {NINJA_STAND_R1 + 0, NINJA_FLASHBOMB_RATE, NullNinja, &s_NinjaFlashBomb[1][3]},
        {NINJA_STAND_R1 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaFlashBomb[1][4]},
        {NINJA_STAND_R1 + 0, NINJA_FLASHBOMB_RATE, DoNinjaMove, &s_NinjaFlashBomb[1][4]},
    },
    {
        {NINJA_STAND_R2 + 0, NINJA_FLASHBOMB_RATE * 2, NullNinja, &s_NinjaFlashBomb[2][1]},
        {NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitFlashBomb, &s_NinjaFlashBomb[2][2]},
        {NINJA_STAND_R2 + 0, NINJA_FLASHBOMB_RATE, NullNinja, &s_NinjaFlashBomb[2][3]},
        {NINJA_STAND_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaFlashBomb[2][4]},
        {NINJA_STAND_R2 + 0, NINJA_FLASHBOMB_RATE, DoNinjaMove, &s_NinjaFlashBomb[2][4]},
    },
    {
        {NINJA_STAND_R3 + 0, NINJA_FLASHBOMB_RATE * 2, NullNinja, &s_NinjaFlashBomb[3][1]},
        {NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitFlashBomb, &s_NinjaFlashBomb[3][2]},
        {NINJA_STAND_R3 + 0, NINJA_FLASHBOMB_RATE, NullNinja, &s_NinjaFlashBomb[3][3]},
        {NINJA_STAND_R3 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaFlashBomb[3][4]},
        {NINJA_STAND_R3 + 0, NINJA_FLASHBOMB_RATE, DoNinjaMove, &s_NinjaFlashBomb[3][4]},
    },
    {
        {NINJA_STAND_R4 + 0, NINJA_FLASHBOMB_RATE * 2, NullNinja, &s_NinjaFlashBomb[4][1]},
        {NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitFlashBomb, &s_NinjaFlashBomb[4][2]},
        {NINJA_STAND_R4 + 0, NINJA_FLASHBOMB_RATE, NullNinja, &s_NinjaFlashBomb[4][3]},
        {NINJA_STAND_R4 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaFlashBomb[4][4]},
        {NINJA_STAND_R4 + 0, NINJA_FLASHBOMB_RATE, DoNinjaMove, &s_NinjaFlashBomb[4][4]},
    },
};


STATEp sg_NinjaFlashBomb[] =
{
    s_NinjaFlashBomb[0],
    s_NinjaFlashBomb[1],
    s_NinjaFlashBomb[2],
    s_NinjaFlashBomb[3],
    s_NinjaFlashBomb[4]
};

//////////////////////
//
// NINJA UZI
//
//////////////////////

#define NINJA_UZI_RATE 8
ANIMATOR InitEnemyUzi,CheckFire;

STATE s_NinjaUzi[5][17] =
{
    {
        {NINJA_FIRE_R0 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[0][1]},
        {NINJA_FIRE_R0 + 0, 0 | SF_QUICK_CALL, CheckFire, &s_NinjaUzi[0][2]},
        {NINJA_FIRE_R0 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[0][3]},
        {NINJA_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[0][4]},
        {NINJA_FIRE_R0 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[0][5]},
        {NINJA_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[0][6]},
        {NINJA_FIRE_R0 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[0][7]},
        {NINJA_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[0][8]},
        {NINJA_FIRE_R0 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[0][9]},
        {NINJA_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[0][10]},
        {NINJA_FIRE_R0 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[0][11]},
        {NINJA_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[0][12]},
        {NINJA_FIRE_R0 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[0][13]},
        {NINJA_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[0][14]},
        {NINJA_FIRE_R0 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[0][15]},
        {NINJA_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[0][16]},
        {NINJA_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaUzi[0][16]},
    },
    {
        {NINJA_FIRE_R1 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[1][1]},
        {NINJA_FIRE_R1 + 0, 0 | SF_QUICK_CALL, CheckFire, &s_NinjaUzi[1][2]},
        {NINJA_FIRE_R1 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[1][3]},
        {NINJA_FIRE_R1 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[1][4]},
        {NINJA_FIRE_R1 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[1][5]},
        {NINJA_FIRE_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[1][6]},
        {NINJA_FIRE_R1 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[1][7]},
        {NINJA_FIRE_R1 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[1][8]},
        {NINJA_FIRE_R1 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[1][9]},
        {NINJA_FIRE_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[1][10]},
        {NINJA_FIRE_R1 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[1][11]},
        {NINJA_FIRE_R1 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[1][12]},
        {NINJA_FIRE_R1 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[1][13]},
        {NINJA_FIRE_R1 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[1][14]},
        {NINJA_FIRE_R1 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[1][15]},
        {NINJA_FIRE_R1 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[1][16]},
        {NINJA_FIRE_R1 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaUzi[1][16]},
    },
    {
        {NINJA_FIRE_R2 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[2][1]},
        {NINJA_FIRE_R2 + 0, 0 | SF_QUICK_CALL, CheckFire, &s_NinjaUzi[2][2]},
        {NINJA_FIRE_R2 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[2][3]},
        {NINJA_FIRE_R2 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[2][4]},
        {NINJA_FIRE_R2 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[2][5]},
        {NINJA_FIRE_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[2][6]},
        {NINJA_FIRE_R2 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[2][7]},
        {NINJA_FIRE_R2 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[2][8]},
        {NINJA_FIRE_R2 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[2][9]},
        {NINJA_FIRE_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[2][10]},
        {NINJA_FIRE_R2 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[2][11]},
        {NINJA_FIRE_R2 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[2][12]},
        {NINJA_FIRE_R2 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[2][13]},
        {NINJA_FIRE_R2 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[2][14]},
        {NINJA_FIRE_R2 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[2][15]},
        {NINJA_FIRE_R2 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[2][16]},
        {NINJA_FIRE_R2 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaUzi[2][16]},
    },
    {
        {NINJA_FIRE_R3 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[3][1]},
        {NINJA_FIRE_R3 + 0, 0 | SF_QUICK_CALL, CheckFire, &s_NinjaUzi[3][2]},
        {NINJA_FIRE_R3 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[3][3]},
        {NINJA_FIRE_R3 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[3][4]},
        {NINJA_FIRE_R3 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[3][5]},
        {NINJA_FIRE_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[3][6]},
        {NINJA_FIRE_R3 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[3][7]},
        {NINJA_FIRE_R3 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[3][8]},
        {NINJA_FIRE_R3 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[3][9]},
        {NINJA_FIRE_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[3][10]},
        {NINJA_FIRE_R3 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[3][11]},
        {NINJA_FIRE_R3 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[3][12]},
        {NINJA_FIRE_R3 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[3][13]},
        {NINJA_FIRE_R3 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[3][14]},
        {NINJA_FIRE_R3 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[3][15]},
        {NINJA_FIRE_R3 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[3][16]},
        {NINJA_FIRE_R3 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaUzi[3][16]},
    },
    {
        {NINJA_FIRE_R4 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[4][1]},
        {NINJA_FIRE_R4 + 0, 0 | SF_QUICK_CALL, CheckFire, &s_NinjaUzi[4][2]},
        {NINJA_FIRE_R4 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[4][3]},
        {NINJA_FIRE_R4 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[4][4]},
        {NINJA_FIRE_R4 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[4][5]},
        {NINJA_FIRE_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[4][6]},
        {NINJA_FIRE_R4 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[4][7]},
        {NINJA_FIRE_R4 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[4][8]},
        {NINJA_FIRE_R4 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[4][9]},
        {NINJA_FIRE_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[4][10]},
        {NINJA_FIRE_R4 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[4][11]},
        {NINJA_FIRE_R4 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[4][12]},
        {NINJA_FIRE_R4 + 0, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[4][13]},
        {NINJA_FIRE_R4 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[4][14]},
        {NINJA_FIRE_R4 + 1, NINJA_UZI_RATE, NullNinja, &s_NinjaUzi[4][15]},
        {NINJA_FIRE_R4 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_NinjaUzi[4][16]},
        {NINJA_FIRE_R4 + 0, 0 | SF_QUICK_CALL, InitActorDecide, &s_NinjaUzi[4][16]},
    },
};


STATEp sg_NinjaUzi[] =
{
    s_NinjaUzi[0],
    s_NinjaUzi[1],
    s_NinjaUzi[2],
    s_NinjaUzi[3],
    s_NinjaUzi[4]
};


//////////////////////
//
// NINJA HARI KARI
//
//////////////////////

#define NINJA_HARI_KARI_WAIT_RATE 200
#define NINJA_HARI_KARI_FALL_RATE 16
ANIMATOR DoHariKariBlood;
ANIMATOR DoNinjaSpecial;

STATE s_NinjaHariKari[] =
{
    {NINJA_HARI_KARI_R0 + 0,   NINJA_HARI_KARI_FALL_RATE,      NullNinja,       &s_NinjaHariKari[1]},
    {NINJA_HARI_KARI_R0 + 0,   SF_QUICK_CALL,                  DoNinjaSpecial,       &s_NinjaHariKari[2]},
    {NINJA_HARI_KARI_R0 + 1,   NINJA_HARI_KARI_WAIT_RATE,      NullNinja,       &s_NinjaHariKari[3]},
    {NINJA_HARI_KARI_R0 + 2,   SF_QUICK_CALL,                  DoNinjaHariKari, &s_NinjaHariKari[4]},
    {NINJA_HARI_KARI_R0 + 2,   NINJA_HARI_KARI_FALL_RATE,      NullAnimator,       &s_NinjaHariKari[5]},
    {NINJA_HARI_KARI_R0 + 3,   NINJA_HARI_KARI_FALL_RATE,      NullAnimator,       &s_NinjaHariKari[6]},
    {NINJA_HARI_KARI_R0 + 4,   NINJA_HARI_KARI_FALL_RATE,      NullAnimator,       &s_NinjaHariKari[7]},
    {NINJA_HARI_KARI_R0 + 5,   NINJA_HARI_KARI_FALL_RATE,      NullAnimator,       &s_NinjaHariKari[8]},
    {NINJA_HARI_KARI_R0 + 6,   NINJA_HARI_KARI_FALL_RATE,      NullAnimator,       &s_NinjaHariKari[9]},
    {NINJA_HARI_KARI_R0 + 7,   NINJA_HARI_KARI_FALL_RATE,      NullAnimator,       &s_NinjaHariKari[10]},
    {NINJA_HARI_KARI_R0 + 7,   NINJA_HARI_KARI_FALL_RATE,      NullAnimator,       &s_NinjaHariKari[10]},
};

STATEp sg_NinjaHariKari[] =
{
    s_NinjaHariKari,
    s_NinjaHariKari,
    s_NinjaHariKari,
    s_NinjaHariKari,
    s_NinjaHariKari
};


//////////////////////
//
// NINJA GRAB THROAT
//
//////////////////////

#define NINJA_GRAB_THROAT_RATE 32
#define NINJA_GRAB_THROAT_R0 4237
ANIMATOR DoNinjaGrabThroat;

STATE s_NinjaGrabThroat[] =
{
    {NINJA_GRAB_THROAT_R0 + 0,   NINJA_GRAB_THROAT_RATE,      NullNinja,       &s_NinjaGrabThroat[1]},
    {NINJA_GRAB_THROAT_R0 + 0,   SF_QUICK_CALL,               DoNinjaSpecial,  &s_NinjaGrabThroat[2]},
    {NINJA_GRAB_THROAT_R0 + 1,   NINJA_GRAB_THROAT_RATE,      NullNinja,       &s_NinjaGrabThroat[3]},
    {NINJA_GRAB_THROAT_R0 + 2,   SF_QUICK_CALL,               DoNinjaGrabThroat, &s_NinjaGrabThroat[4]},
    {NINJA_GRAB_THROAT_R0 + 2,   NINJA_GRAB_THROAT_RATE,      NullNinja,       &s_NinjaGrabThroat[5]},
    {NINJA_GRAB_THROAT_R0 + 1,   NINJA_GRAB_THROAT_RATE,      NullNinja,       &s_NinjaGrabThroat[0]},
};

STATEp sg_NinjaGrabThroat[] =
{
    s_NinjaGrabThroat,
    s_NinjaGrabThroat,
    s_NinjaGrabThroat,
    s_NinjaGrabThroat,
    s_NinjaGrabThroat
};


//////////////////////
//
// NINJA DIE
//
//////////////////////

#define NINJA_DIE_RATE 14

STATE s_NinjaDie[] =
{
    {NINJA_DIE + 0, NINJA_DIE_RATE, NullNinja, &s_NinjaDie[1]},
    {NINJA_DIE + 1, NINJA_DIE_RATE, NullNinja, &s_NinjaDie[2]},
    {NINJA_DIE + 2, NINJA_DIE_RATE, NullNinja, &s_NinjaDie[3]},
    {NINJA_DIE + 3, NINJA_DIE_RATE, NullNinja, &s_NinjaDie[4]},
    {NINJA_DIE + 4, NINJA_DIE_RATE, NullNinja, &s_NinjaDie[5]},
    {NINJA_DIE + 5, NINJA_DIE_RATE-4, NullNinja, &s_NinjaDie[6]},
    {NINJA_DIE + 6, NINJA_DIE_RATE-6, NullNinja, &s_NinjaDie[7]},
    {NINJA_DIE + 6, SF_QUICK_CALL, DoNinjaSpecial, &s_NinjaDie[8]},
    {NINJA_DIE + 6, NINJA_DIE_RATE-10, NullNinja, &s_NinjaDie[9]},
    {NINJA_DIE + 7, SF_QUICK_CALL, QueueFloorBlood, &s_NinjaDie[10]},
    {NINJA_DIE + 7, NINJA_DIE_RATE-12, DoActorDebris, &s_NinjaDie[10]},
};


#define NINJA_DIESLICED_RATE 20
ANIMATOR DoCutInHalf;

STATE s_NinjaDieSliced[] =
{
    {NINJA_SLICED + 0, NINJA_DIESLICED_RATE*6, NullNinja, &s_NinjaDieSliced[1]},
    {NINJA_SLICED + 1, NINJA_DIESLICED_RATE,   NullNinja, &s_NinjaDieSliced[2]},
    {NINJA_SLICED + 2, NINJA_DIESLICED_RATE,   NullNinja, &s_NinjaDieSliced[3]},
    {NINJA_SLICED + 3, NINJA_DIESLICED_RATE,   NullNinja, &s_NinjaDieSliced[4]},
    {NINJA_SLICED + 4, NINJA_DIESLICED_RATE-1, NullNinja, &s_NinjaDieSliced[5]},
    {NINJA_SLICED + 5, NINJA_DIESLICED_RATE-2, NullNinja, &s_NinjaDieSliced[6]},
    {NINJA_SLICED + 6, NINJA_DIESLICED_RATE-3, NullNinja, &s_NinjaDieSliced[7]},
    {NINJA_SLICED + 7, NINJA_DIESLICED_RATE-4, NullNinja, &s_NinjaDieSliced[8]},
    {NINJA_SLICED + 7, SF_QUICK_CALL, DoNinjaSpecial, &s_NinjaDieSliced[9]},
    {NINJA_SLICED + 8, NINJA_DIESLICED_RATE-5, NullNinja, &s_NinjaDieSliced[10]},
    {NINJA_SLICED + 9, SF_QUICK_CALL, QueueFloorBlood, &s_NinjaDieSliced[11]},
    {NINJA_SLICED + 9, NINJA_DIESLICED_RATE, DoActorDebris, &s_NinjaDieSliced[11]},
};

STATE s_NinjaDead[] =
{
    {NINJA_DIE + 5, NINJA_DIE_RATE, DoActorDebris, &s_NinjaDead[1]},
    {NINJA_DIE + 6, SF_QUICK_CALL, DoNinjaSpecial, &s_NinjaDead[2]},
    {NINJA_DIE + 6, NINJA_DIE_RATE, DoActorDebris, &s_NinjaDead[3]},
    {NINJA_DIE + 7, SF_QUICK_CALL, QueueFloorBlood,&s_NinjaDead[4]},
    {NINJA_DIE + 7, NINJA_DIE_RATE, DoActorDebris, &s_NinjaDead[4]},
};


STATE s_NinjaDeathJump[] =
{
    {NINJA_DIE + 0, NINJA_DIE_RATE, DoActorDeathMove, &s_NinjaDeathJump[1]},
    {NINJA_DIE + 1, NINJA_DIE_RATE, DoActorDeathMove, &s_NinjaDeathJump[2]},
    {NINJA_DIE + 2, NINJA_DIE_RATE, DoActorDeathMove, &s_NinjaDeathJump[2]},
};

STATE s_NinjaDeathFall[] =
{
    {NINJA_DIE + 3, NINJA_DIE_RATE, DoActorDeathMove, &s_NinjaDeathFall[1]},
    {NINJA_DIE + 4, NINJA_DIE_RATE, DoActorDeathMove, &s_NinjaDeathFall[1]},
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

STATEp sg_NinjaDie[] =
{
    s_NinjaDie
};

STATEp sg_NinjaDieSliced[] =
{
    s_NinjaDieSliced
};

STATEp sg_NinjaDead[] =
{
    s_NinjaDead
};

STATEp sg_NinjaDeathJump[] =
{
    s_NinjaDeathJump
};

STATEp sg_NinjaDeathFall[] =
{
    s_NinjaDeathFall
};

/*

 !AIC - Collection of states that connect action to states

*/

ACTOR_ACTION_SET NinjaSniperActionSet =
{
    sg_NinjaDuck,
    sg_NinjaCrawl,
    sg_NinjaJump,
    sg_NinjaFall,
    sg_NinjaKneelCrawl,
    sg_NinjaSwim,
    sg_NinjaFly,
    sg_NinjaUzi,
    sg_NinjaDuck,
    NULL,
    sg_NinjaClimb,
    sg_NinjaPain,
    sg_NinjaDie,
    sg_NinjaHariKari,
    sg_NinjaDead,
    sg_NinjaDeathJump,
    sg_NinjaDeathFall,
    {sg_NinjaUzi},
    {1024},
    {sg_NinjaUzi},
    {1024},
    {NULL},
    sg_NinjaDuck,
    sg_NinjaDive
};

ACTOR_ACTION_SET NinjaActionSet =
{
    sg_NinjaStand,
    sg_NinjaRun,
    sg_NinjaJump,
    sg_NinjaFall,
    sg_NinjaKneelCrawl,
    sg_NinjaSwim,
    sg_NinjaFly,
    sg_NinjaRise,
    sg_NinjaSit,
    NULL,
    sg_NinjaClimb,
    sg_NinjaPain,
    sg_NinjaDie,
    sg_NinjaHariKari,
    sg_NinjaDead,
    sg_NinjaDeathJump,
    sg_NinjaDeathFall,
    {sg_NinjaUzi, sg_NinjaStar},
    {1000, 1024},
    {sg_NinjaUzi, sg_NinjaStar},
    {800, 1024},
    {NULL},
    sg_NinjaDuck,
    sg_NinjaDive
};

ACTOR_ACTION_SET NinjaRedActionSet =
{
    sg_NinjaStand,
    sg_NinjaRun,
    sg_NinjaJump,
    sg_NinjaFall,
    sg_NinjaKneelCrawl,
    sg_NinjaSwim,
    sg_NinjaFly,
    sg_NinjaRise,
    sg_NinjaSit,
    NULL,
    sg_NinjaClimb,
    sg_NinjaPain,
    sg_NinjaDie,
    sg_NinjaHariKari,
    sg_NinjaDead,
    sg_NinjaDeathJump,
    sg_NinjaDeathFall,
    {sg_NinjaUzi, sg_NinjaUzi},
    {812, 1024},
    {sg_NinjaUzi, sg_NinjaRocket},
    {812, 1024},
    {NULL},
    sg_NinjaDuck,
    sg_NinjaDive
};

ACTOR_ACTION_SET NinjaSeekerActionSet =
{
    sg_NinjaStand,
    sg_NinjaRun,
    sg_NinjaJump,
    sg_NinjaFall,
    sg_NinjaKneelCrawl,
    sg_NinjaSwim,
    sg_NinjaFly,
    sg_NinjaRise,
    sg_NinjaSit,
    NULL,
    sg_NinjaClimb,
    sg_NinjaPain,
    sg_NinjaDie,
    sg_NinjaHariKari,
    sg_NinjaDead,
    sg_NinjaDeathJump,
    sg_NinjaDeathFall,
    {sg_NinjaUzi, sg_NinjaStar},
    {812, 1024},
    {sg_NinjaUzi, sg_NinjaRocket},
    {812, 1024},
    {NULL},
    sg_NinjaDuck,
    sg_NinjaDive
};

ACTOR_ACTION_SET NinjaGrenadeActionSet =
{
    sg_NinjaStand,
    sg_NinjaRun,
    sg_NinjaJump,
    sg_NinjaFall,
    sg_NinjaKneelCrawl,
    sg_NinjaSwim,
    sg_NinjaFly,
    sg_NinjaRise,
    sg_NinjaSit,
    NULL,
    sg_NinjaClimb,
    sg_NinjaPain,
    sg_NinjaDie,
    sg_NinjaHariKari,
    sg_NinjaDead,
    sg_NinjaDeathJump,
    sg_NinjaDeathFall,
    {sg_NinjaUzi, sg_NinjaUzi},
    {812, 1024},
    {sg_NinjaUzi, sg_NinjaGrenade},
    {812, 1024},
    {NULL},
    sg_NinjaDuck,
    sg_NinjaDive
};

ACTOR_ACTION_SET NinjaGreenActionSet =
{
    sg_NinjaStand,
    sg_NinjaRun,
    sg_NinjaJump,
    sg_NinjaFall,
    sg_NinjaKneelCrawl,
    sg_NinjaSwim,
    sg_NinjaFly,
    sg_NinjaRise,
    sg_NinjaSit,
    NULL,
    sg_NinjaClimb,
    sg_NinjaPain,
    sg_NinjaDie,
    sg_NinjaHariKari,
    sg_NinjaDead,
    sg_NinjaDeathJump,
    sg_NinjaDeathFall,
    {sg_NinjaUzi, sg_NinjaFlashBomb},
    {912, 1024},
    {sg_NinjaFlashBomb, sg_NinjaUzi, sg_NinjaMirv, sg_NinjaNapalm},
    {150, 500, 712, 1024},
    {NULL},
    sg_NinjaDuck,
    sg_NinjaDive
};


extern STATEp sg_PlayerNinjaRun[];
extern STATEp sg_PlayerNinjaStand[];
extern STATEp sg_PlayerNinjaJump[];
extern STATEp sg_PlayerNinjaFall[];
extern STATEp sg_PlayerNinjaClimb[];
extern STATEp sg_PlayerNinjaCrawl[];
extern STATEp sg_PlayerNinjaSwim[];
ACTOR_ACTION_SET PlayerNinjaActionSet =
{
    sg_PlayerNinjaStand,
    sg_PlayerNinjaRun,
    sg_PlayerNinjaJump,
    sg_PlayerNinjaFall,
    //sg_NinjaJump,
    //sg_NinjaFall,
    sg_PlayerNinjaCrawl,
    sg_PlayerNinjaSwim,
    sg_NinjaFly,
    sg_NinjaRise,
    sg_NinjaSit,
    NULL,
    sg_PlayerNinjaClimb,
    sg_NinjaPain,
    sg_NinjaDie,
    sg_NinjaHariKari,
    sg_NinjaDead,
    sg_NinjaDeathJump,
    sg_NinjaDeathFall,
    {sg_NinjaStar, sg_NinjaUzi},
    {1000, 1024},
    {sg_NinjaStar, sg_NinjaUzi},
    {800, 1024},
    {NULL},
    sg_NinjaDuck,
    sg_PlayerNinjaSwim
};

int
DoHariKariBlood(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    return 0;
}

/*

 !AIC - Every actor has a setup where they are initialized

*/

int
SetupNinja(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum, NINJA_RUN_R0, s_NinjaRun[0]);
        u->Health = HEALTH_NINJA;
    }

    u->StateEnd = s_NinjaDie;
    u->Rot = sg_NinjaRun;
    sp->xrepeat = 46;
    sp->yrepeat = 46;

    if (sp->pal == PALETTE_PLAYER5)
    {
        u->Attrib = &InvisibleNinjaAttrib;
        EnemyDefaults(SpriteNum, &NinjaGreenActionSet, &NinjaPersonality);
        if (!TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
            u->Health = HEALTH_RED_NINJA;
        SET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
        sp->shade = 127;
        sp->pal = u->spal = PALETTE_PLAYER5;
        sp->hitag = 9998;
        if (pic == NINJA_CRAWL_R0)
        {
            if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
            {
                u->Attrib = &NinjaAttrib;
                u->ActorActionSet = &NinjaActionSet;
                u->Personality = &NinjaPersonality;
                ChangeState(SpriteNum, s_NinjaCeiling[0]);
            }
            else
            {
                u->Attrib = &NinjaAttrib;
                u->ActorActionSet = &NinjaSniperActionSet;
                u->Personality = &NinjaSniperPersonality;
                ChangeState(SpriteNum, s_NinjaDuck[0]);
            }
        }
    }
    else if (sp->pal == PALETTE_PLAYER3)
    {
        u->Attrib = &NinjaAttrib;
        EnemyDefaults(SpriteNum, &NinjaRedActionSet, &NinjaPersonality);
        if (!TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
            u->Health = HEALTH_RED_NINJA;
        sp->pal = u->spal = PALETTE_PLAYER3;
        if (pic == NINJA_CRAWL_R0)
        {
            if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
            {
                u->Attrib = &NinjaAttrib;
                u->ActorActionSet = &NinjaActionSet;
                u->Personality = &NinjaPersonality;
                ChangeState(SpriteNum, s_NinjaCeiling[0]);
            }
            else
            {
                u->Attrib = &NinjaAttrib;
                u->ActorActionSet = &NinjaSniperActionSet;
                u->Personality = &NinjaSniperPersonality;
                ChangeState(SpriteNum, s_NinjaDuck[0]);
            }
        }
    }
    else if (sp->pal == PAL_XLAT_LT_TAN)
    {
        u->Attrib = &NinjaAttrib;
        EnemyDefaults(SpriteNum, &NinjaSeekerActionSet, &NinjaPersonality);
        if (!TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
            u->Health = HEALTH_RED_NINJA;
        sp->pal = u->spal = PAL_XLAT_LT_TAN;
        u->Attrib = &NinjaAttrib;
    }
    else if (sp->pal == PAL_XLAT_LT_GREY)
    {
        u->Attrib = &NinjaAttrib;
        EnemyDefaults(SpriteNum, &NinjaGrenadeActionSet, &NinjaPersonality);
        if (!TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
            u->Health = HEALTH_RED_NINJA;
        sp->pal = u->spal = PAL_XLAT_LT_GREY;
        u->Attrib = &NinjaAttrib;
    }
    else
    {
        u->Attrib = &NinjaAttrib;
        sp->pal = u->spal = PALETTE_PLAYER0;
        EnemyDefaults(SpriteNum, &NinjaActionSet, &NinjaPersonality);
        if (pic == NINJA_CRAWL_R0)
        {
            u->Attrib = &NinjaAttrib;
            u->ActorActionSet = &NinjaSniperActionSet;
            u->Personality = &NinjaSniperPersonality;
            ChangeState(SpriteNum, s_NinjaDuck[0]);
        }
    }

    ChangeState(SpriteNum, s_NinjaRun[0]);
    DoActorSetSpeed(SpriteNum, NORM_SPEED);

    u->Radius = 280;
    SET(u->Flags, SPR_XFLIP_TOGGLE);

    return 0;
}

int
DoNinjaHariKari(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    int SpawnBlood(short SpriteNum, short Weapon, short hit_ang, int hit_x, int hit_y, int hit_z);
    short cnt,i;

    UpdateSinglePlayKills(SpriteNum);
    change_sprite_stat(SpriteNum, STAT_DEAD_ACTOR);
    RESET(sprite[SpriteNum].cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    SET(u->Flags, SPR_DEAD);
    RESET(u->Flags, SPR_FALLING | SPR_JUMPING);
    u->floor_dist = Z(40);
    u->RotNum = 0;
    u->ActorActionFunc = NULL;

    SET(sp->extra, SPRX_BREAKABLE);
    SET(sp->cstat, CSTAT_SPRITE_BREAKABLE);

    PlaySound(DIGI_NINJAUZIATTACK,&sp->x,&sp->y,&sp->z,v3df_follow);

    SpawnBlood(SpriteNum, SpriteNum, -1, -1, -1, -1);

    cnt = RANDOM_RANGE(4)+1;
    for (i=0; i<=cnt; i++)
        InitBloodSpray(SpriteNum,TRUE,-2);

    return 0;
}

int
DoNinjaGrabThroat(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    int SpawnBlood(short SpriteNum, short Weapon, short hit_ang, int hit_x, int hit_y, int hit_z);
    short cnt,i;

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
    {
        UpdateSinglePlayKills(SpriteNum);
        RESET(u->Flags2, SPR2_DYING);
        RESET(sp->cstat, CSTAT_SPRITE_YFLIP);
        change_sprite_stat(SpriteNum, STAT_DEAD_ACTOR);
        RESET(sprite[SpriteNum].cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        SET(u->Flags, SPR_DEAD);
        RESET(u->Flags, SPR_FALLING | SPR_JUMPING);
        u->floor_dist = Z(40);
        u->RotNum = 0;
        u->ActorActionFunc = NULL;

        SET(sp->extra, SPRX_BREAKABLE);
        SET(sp->cstat, CSTAT_SPRITE_BREAKABLE);

        //SpawnBlood(SpriteNum, SpriteNum, -1, -1, -1, -1);

        ChangeState(SpriteNum, u->StateEnd);
        sp->xvel = 0;
        //u->jump_speed = -300;
        //DoActorBeginJump(SpriteNum);
        PlaySound(DIGI_NINJASCREAM,&sp->x,&sp->y,&sp->z,v3df_follow);
    }

    return 0;
}

/*

 !AIC - Most actors have one of these and the all look similar

*/

int
DoNinjaMove(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (TEST(u->Flags2, SPR2_DYING))
    {
        NewStateGroup(SpriteNum, sg_NinjaGrabThroat);
        return 0;
    }

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
NinjaJumpActionFunc(short SpriteNum)
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

/*

 !AIC - Short version of DoNinjaMove without the movement code.  For times when
 the actor is doing something but not moving.

*/

int
NullNinja(short SpriteNum)
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


int DoNinjaPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullNinja(SpriteNum);

    if (TEST(u->Flags2, SPR2_DYING))
    {
        NewStateGroup(SpriteNum, sg_NinjaGrabThroat);
        return 0;
    }

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(SpriteNum);

    return 0;
}

int DoNinjaSpecial(short SpriteNum)
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

int CheckFire(short SpriteNum)
{
    if (!CanSeePlayer(SpriteNum))
        InitActorDuck(SpriteNum);
    return 0;
}

int
DoNinjaCeiling(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    DoActorSectorDamage(SpriteNum);

    return 0;
}


//
// !AIC - Stuff from here down is really Player related.  Should be moved but it was
// too convienent to put it here.
//

void
InitAllPlayerSprites(void)
{
    short i, sp_num;
    USERp u;

    TRAVERSE_CONNECT(i)
    {
        InitPlayerSprite(Player + i);
    }
}


void
PlayerLevelReset(PLAYERp pp)
{
    SPRITEp sp = &sprite[pp->PlayerSprite];
    USERp u = User[pp->PlayerSprite];

    if (gNet.MultiGameType == MULTI_GAME_COMMBAT)
    {
        PlayerDeathReset(pp);
        return;
    }

    if (TEST(pp->Flags, PF_DIVING))
        DoPlayerStopDiveNoWarp(pp);

    COVER_SetReverb(0); // Turn off any echoing that may have been going before
    pp->Reverb = 0;
    pp->SecretsFound = 0;
    pp->WpnFirstType = WPN_SWORD;
    //PlayerUpdateHealth(pp, 500);
    //pp->Armor = 0;
    //PlayerUpdateArmor(pp, 0);
    pp->Kills = 0;
    pp->Killer = -1;
    pp->NightVision = FALSE;
    pp->StartColor = 0;
    pp->FadeAmt = 0;
    pp->DeathType = 0;
    PlayerUpdatePanelInfo(pp);
    RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
    RESET(pp->Flags, PF_WEAPON_DOWN|PF_WEAPON_RETRACT);
    RESET(pp->Flags, PF_DEAD);

    pp->sop_control = NULL;
    pp->sop_riding = NULL;
    pp->sop_remote = NULL;
    pp->sop = NULL;
    DoPlayerResetMovement(pp);
    DamageData[u->WeaponNum].Init(pp);
}

void
PlayerDeathReset(PLAYERp pp)
{
    SPRITEp sp = &sprite[pp->PlayerSprite];
    USERp u = User[pp->PlayerSprite];

    if (TEST(pp->Flags, PF_DIVING))
        DoPlayerStopDiveNoWarp(pp);

    COVER_SetReverb(0); // Turn off any echoing that may have been going before
    pp->Reverb = 0;
    // second weapon - whatever it is
    u->WeaponNum = WPN_SWORD;
    pp->WpnFirstType = u->WeaponNum;
    pp->WpnRocketType = 0;
    pp->WpnRocketHeat = 0;             // 5 to 0 range
    pp->WpnRocketNuke = 0;             // 1, you have it, or you don't
    pp->WpnFlameType = 0;              // Guardian weapons fire
    pp->WpnUziType = 2;
    pp->WpnShotgunType = 0;            // Shotgun has normal or fully automatic fire
    pp->WpnShotgunAuto = 0;            // 50-0 automatic shotgun rounds
    pp->WpnShotgunLastShell = 0;       // Number of last shell fired
    pp->Bloody = FALSE;
    pp->TestNukeInit = FALSE;
    pp->InitingNuke = FALSE;
    pp->nukevochandle = 0;
    pp->NukeInitialized = FALSE;
    pp->BunnyMode = FALSE;

    memset(pp->WpnAmmo,0,sizeof(pp->WpnAmmo));
    memset(pp->InventoryTics,0,sizeof(pp->InventoryTics));
    memset(pp->InventoryPercent,0,sizeof(pp->InventoryPercent));
    memset(pp->InventoryAmount,0,sizeof(pp->InventoryAmount));
    memset(pp->InventoryActive,0,sizeof(pp->InventoryActive));
    pp->WpnAmmo[WPN_STAR] = 30;
    pp->WpnAmmo[WPN_SWORD] = pp->WpnAmmo[WPN_FIST] = 30;
    pp->WpnFlags = 0;
    pp->WpnGotOnceFlags = 0;
    SET(pp->WpnFlags, BIT(WPN_SWORD));
    SET(pp->WpnFlags, BIT(WPN_FIST) | BIT(u->WeaponNum));
    SET(pp->WpnFlags, BIT(WPN_STAR) | BIT(u->WeaponNum));
    RESET(pp->Flags, PF_PICKED_UP_AN_UZI);
    RESET(pp->Flags, PF_TWO_UZI);

    u->Health = 100;
    pp->MaxHealth = 100;
    //PlayerUpdateHealth(pp, 500);
    puser[pp->pnum].Health = u->Health;
    pp->Armor = 0;
    PlayerUpdateArmor(pp, 0);
    pp->Killer = -1;
    pp->NightVision = FALSE;
    pp->StartColor = 0;
    pp->FadeAmt = 0;
    pp->DeathType = 0;
    PlayerUpdatePanelInfo(pp);
    RESET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
    RESET(pp->Flags, PF_WEAPON_DOWN|PF_WEAPON_RETRACT);
    RESET(pp->Flags, PF_DEAD);

    pp->sop_control = NULL;
    pp->sop_riding = NULL;
    pp->sop_remote = NULL;
    pp->sop = NULL;
    DoPlayerResetMovement(pp);
    //if (pp->CurWpn)
    //    RESET(pp->CurWpn->flags, PANF_DEATH_HIDE);
    DamageData[u->WeaponNum].Init(pp);
}

void
PlayerPanelSetup(void)
{
    short pnum;
    PLAYERp pp;
    USERp u;

    // For every player setup the panel weapon stuff
    //for (pp = Player; pp < &Player[numplayers]; pp++)
    TRAVERSE_CONNECT(pnum)
    {
        pp = Player + pnum;

        u = User[pp->PlayerSprite];

        ASSERT(u != NULL);

        //u->WeaponNum = WPN_STAR;
        //pp->WpnFirstType = WPN_SWORD;

        PlayerUpdateWeapon(pp, u->WeaponNum);
    }
}

void
PlayerGameReset(PLAYERp pp)
{
    SPRITEp sp = &sprite[pp->PlayerSprite];
    USERp u = User[pp->PlayerSprite];

    COVER_SetReverb(0); // Turn off any echoing that may have been going before
    pp->Reverb = 0;
    u->WeaponNum = WPN_SWORD;
    pp->WpnFirstType = u->WeaponNum;
    pp->WpnRocketType = 0;
    pp->WpnRocketHeat = 0; // 5 to 0 range
    pp->WpnRocketNuke = 0; // 1, you have it, or you don't
    pp->WpnFlameType = 0; // Guardian weapons fire
    pp->WpnUziType = 2;
    pp->WpnShotgunType = 0;            // Shotgun has normal or fully automatic fire
    pp->WpnShotgunAuto = 0;            // 50-0 automatic shotgun rounds
    pp->WpnShotgunLastShell = 0;       // Number of last shell fired
    pp->Bloody = FALSE;
    pp->TestNukeInit = FALSE;
    pp->InitingNuke = FALSE;
    pp->nukevochandle = 0;
    pp->NukeInitialized = FALSE;
    pp->BunnyMode = FALSE;
    pp->SecretsFound = 0;

    pp->WpnAmmo[WPN_STAR] = 30;
    pp->WpnAmmo[WPN_SWORD] = pp->WpnAmmo[WPN_FIST] = 30;
    pp->WpnFlags = 0;
    pp->WpnGotOnceFlags = 0;
    SET(pp->WpnFlags, BIT(WPN_SWORD));
    SET(pp->WpnFlags, BIT(WPN_FIST) | BIT(u->WeaponNum));
    SET(pp->WpnFlags, BIT(WPN_STAR) | BIT(u->WeaponNum));
    RESET(pp->Flags, PF_PICKED_UP_AN_UZI);
    RESET(pp->Flags, PF_TWO_UZI);
    pp->MaxHealth = 100;
    PlayerUpdateHealth(pp, 500);
    pp->Armor = 0;
    PlayerUpdateArmor(pp, 0);
    pp->Killer = -1;

    if (pp == Player+screenpeek)
    {
        if (getrendermode() < 3)
            COVERsetbrightness(gs.Brightness,&palette_data[0][0]);
        else
            setpalettefade(0,0,0,0);
        memcpy(pp->temp_pal, palette_data, sizeof(palette_data));
    }
    pp->NightVision = FALSE;
    pp->StartColor = 0;
    pp->FadeAmt = 0;
    pp->DeathType = 0;

    PlayerUpdatePanelInfo(pp);
    RESET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);

    pp->sop_control = NULL;
    pp->sop_riding = NULL;
    pp->sop_remote = NULL;
    pp->sop = NULL;
    DoPlayerResetMovement(pp);
    DamageData[u->WeaponNum].Init(pp);
}

extern ACTOR_ACTION_SET PlayerNinjaActionSet;

void
PlayerSpriteLoadLevel(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    ChangeState(SpriteNum, s_NinjaRun[0]);
    u->Rot = sg_NinjaRun;
    u->ActorActionSet = &PlayerNinjaActionSet;
}

void
InitPlayerSprite(PLAYERp pp)
{
    short i, sp_num;
    SPRITE *sp;
    USERp u;
    int pnum = pp - Player;
    extern SWBOOL NewGame;

    COVER_SetReverb(0); // Turn off any echoing that may have been going before
    pp->Reverb = 0;
    sp_num = pp->PlayerSprite = SpawnSprite(STAT_PLAYER0 + pnum, NINJA_RUN_R0, NULL, pp->cursectnum, pp->posx,
                                            pp->posy, pp->posz, pp->pang, 0);

    pp->SpriteP = sp = &sprite[sp_num];
    pp->pnum = pnum;

    SET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    SET(sp->extra, SPRX_PLAYER_OR_ENEMY);
    RESET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);

    u = User[sp_num];

    // Grouping items that need to be reset after a LoadLevel
    ChangeState(sp_num, s_NinjaRun[0]);
    u->Rot = sg_NinjaRun;
    u->ActorActionSet = &PlayerNinjaActionSet;

    u->RotNum = 5;

    u->Radius = 400;
    u->PlayerP = pp;
    //u->Health = pp->MaxHealth;
    SET(u->Flags, SPR_XFLIP_TOGGLE);


    sp->picnum = u->State->Pic;
    sp->shade = -60; // was 15
    sp->clipdist = (char)(256L >> 2);

    sp->xrepeat = PLAYER_NINJA_XREPEAT;
    sp->yrepeat = PLAYER_NINJA_YREPEAT;
    sp->pal = PALETTE_PLAYER0 + pp->pnum;
    u->spal = sp->pal;

    NewStateGroup(sp_num, u->ActorActionSet->Run);

    pp->PlayerUnderSprite = -1;
    pp->UnderSpriteP = NULL;

    DoPlayerZrange(pp);

    if (NewGame)
    {
        PlayerGameReset(pp);
    }
    else
    {
        // save stuff from last level
        u->WeaponNum = puser[pnum].WeaponNum;
        u->LastWeaponNum = puser[pnum].LastWeaponNum;
        u->Health = puser[pnum].Health;
        PlayerLevelReset(pp);
    }

    memset(pp->InventoryTics,0,sizeof(pp->InventoryTics));

    if (pp == Player+screenpeek)
    {
        if (getrendermode() < 3)
            COVERsetbrightness(gs.Brightness,&palette_data[0][0]);
        else
            setpalettefade(0,0,0,0);
        memcpy(pp->temp_pal, palette_data, sizeof(palette_data));
    }

    pp->NightVision = FALSE;
    pp->StartColor = 0;
    pp->FadeAmt = 0;
    pp->DeathType = 0;
    PlayerUpdatePanelInfo(pp);
}

void
SpawnPlayerUnderSprite(PLAYERp pp)
{
    USERp pu = User[pp->PlayerSprite], u;
    SPRITEp psp = &sprite[pp->PlayerSprite];
    SPRITEp sp;
    int pnum = pp - Player, sp_num;

    sp_num = pp->PlayerUnderSprite = SpawnSprite(STAT_PLAYER_UNDER0 + pnum,
                                                 NINJA_RUN_R0, NULL, pp->cursectnum, pp->posx, pp->posy, pp->posz, pp->pang, 0);

    sp = &sprite[sp_num];
    u = User[sp_num];

    pp->UnderSpriteP = sp;

    SET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    SET(sp->extra, SPRX_PLAYER_OR_ENEMY);

    u->Rot = sg_NinjaRun;
    u->RotNum = pu->RotNum;
    NewStateGroup(sp_num, pu->Rot);

    u->Radius = pu->Radius;
    u->PlayerP = pp;
    u->Health = pp->MaxHealth;
    SET(u->Flags, SPR_XFLIP_TOGGLE);

    u->ActorActionSet = pu->ActorActionSet;

    sp->picnum = psp->picnum;
    sp->clipdist = psp->clipdist;
    sp->xrepeat = psp->xrepeat;
    sp->yrepeat = psp->yrepeat;

    //sp->pal = psp->pal;
    //u->spal = pu->spal;
}

#include "saveable.h"

static saveable_code saveable_ninja_code[] =
{
    SAVE_CODE(DoHariKariBlood),
    SAVE_CODE(SetupNinja),
    SAVE_CODE(DoNinjaHariKari),
    SAVE_CODE(DoNinjaGrabThroat),
    SAVE_CODE(DoNinjaMove),
    SAVE_CODE(NinjaJumpActionFunc),
    SAVE_CODE(NullNinja),
    SAVE_CODE(DoNinjaPain),
    SAVE_CODE(DoNinjaSpecial),
    SAVE_CODE(CheckFire),
    SAVE_CODE(DoNinjaCeiling)
};

static saveable_data saveable_ninja_data[] =
{
    SAVE_DATA(NinjaBattle),
    SAVE_DATA(NinjaOffense),
    SAVE_DATA(NinjaBroadcast),
    SAVE_DATA(NinjaSurprised),
    SAVE_DATA(NinjaEvasive),
    SAVE_DATA(NinjaLostTarget),
    SAVE_DATA(NinjaCloseRange),
    SAVE_DATA(NinjaSniperRoam),
    SAVE_DATA(NinjaSniperBattle),

    SAVE_DATA(NinjaPersonality),
    SAVE_DATA(NinjaSniperPersonality),

    SAVE_DATA(NinjaAttrib),
    SAVE_DATA(InvisibleNinjaAttrib),
    SAVE_DATA(DefaultAttrib),

    SAVE_DATA(s_NinjaRun),
    SAVE_DATA(sg_NinjaRun),
    SAVE_DATA(s_NinjaStand),
    SAVE_DATA(sg_NinjaStand),
    SAVE_DATA(s_NinjaRise),
    SAVE_DATA(sg_NinjaRise),
    SAVE_DATA(s_NinjaCrawl),
    SAVE_DATA(sg_NinjaCrawl),
    SAVE_DATA(s_NinjaKneelCrawl),
    SAVE_DATA(sg_NinjaKneelCrawl),
    SAVE_DATA(s_NinjaDuck),
    SAVE_DATA(sg_NinjaDuck),
    SAVE_DATA(s_NinjaSit),
    SAVE_DATA(sg_NinjaSit),
    SAVE_DATA(s_NinjaCeiling),
    SAVE_DATA(sg_NinjaCeiling),
    SAVE_DATA(s_NinjaJump),
    SAVE_DATA(sg_NinjaJump),
    SAVE_DATA(s_NinjaFall),
    SAVE_DATA(sg_NinjaFall),
    SAVE_DATA(s_NinjaSwim),
    SAVE_DATA(sg_NinjaSwim),
    SAVE_DATA(s_NinjaDive),
    SAVE_DATA(sg_NinjaDive),
    SAVE_DATA(s_NinjaClimb),
    SAVE_DATA(sg_NinjaClimb),
    SAVE_DATA(s_NinjaFly),
    SAVE_DATA(sg_NinjaFly),
    SAVE_DATA(s_NinjaPain),
    SAVE_DATA(sg_NinjaPain),
    SAVE_DATA(s_NinjaStar),
    SAVE_DATA(sg_NinjaStar),
    SAVE_DATA(s_NinjaMirv),
    SAVE_DATA(sg_NinjaMirv),
    SAVE_DATA(s_NinjaNapalm),
    SAVE_DATA(sg_NinjaNapalm),
    SAVE_DATA(s_NinjaRocket),
    SAVE_DATA(sg_NinjaRocket),
    SAVE_DATA(s_NinjaGrenade),
    SAVE_DATA(sg_NinjaGrenade),
    SAVE_DATA(s_NinjaFlashBomb),
    SAVE_DATA(sg_NinjaFlashBomb),
    SAVE_DATA(s_NinjaUzi),
    SAVE_DATA(sg_NinjaUzi),
    SAVE_DATA(s_NinjaHariKari),
    SAVE_DATA(sg_NinjaHariKari),
    SAVE_DATA(s_NinjaGrabThroat),
    SAVE_DATA(sg_NinjaGrabThroat),
    SAVE_DATA(s_NinjaDie),
    SAVE_DATA(s_NinjaDieSliced),
    SAVE_DATA(s_NinjaDead),
    SAVE_DATA(s_NinjaDeathJump),
    SAVE_DATA(s_NinjaDeathFall),
    SAVE_DATA(sg_NinjaDie),
    SAVE_DATA(sg_NinjaDieSliced),
    SAVE_DATA(sg_NinjaDead),
    SAVE_DATA(sg_NinjaDeathJump),
    SAVE_DATA(sg_NinjaDeathFall),

    SAVE_DATA(NinjaSniperActionSet),
    SAVE_DATA(NinjaActionSet),
    SAVE_DATA(NinjaRedActionSet),
    SAVE_DATA(NinjaSeekerActionSet),
    SAVE_DATA(NinjaGrenadeActionSet),
    SAVE_DATA(NinjaGreenActionSet),
    SAVE_DATA(PlayerNinjaActionSet),
};

saveable_module saveable_ninja =
{
    // code
    saveable_ninja_code,
    SIZ(saveable_ninja_code),

    // data
    saveable_ninja_data,
    SIZ(saveable_ninja_data)
};

