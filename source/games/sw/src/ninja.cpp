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
// Added Ninja Sliced fix
// Fixed Ninja sliced dead and rotation
// Added s_NinjaDieSlicedHack[]
// Fixed Saved Game
// Added GrabThroat Hack
//

#include "build.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "pal.h"
#include "player.h"
#include "network.h"
#include "weapon.h"
#include "misc.h"
#include "sprite.h"

BEGIN_SW_NS


/*

 !AIC - Decision tables used in mostly ai.c DoActorActionDecide().

*/

DECISION NinjaBattle[] =
{
    {499, &AF(InitActorMoveCloser)},
    {1024, &AF(InitActorAttack)}
};

DECISION NinjaOffense[] =
{
    {499, &AF(InitActorMoveCloser)},
    {1024, &AF(InitActorAttack)}
};

DECISIONB NinjaBroadcast[] =
{
    {6, attr_ambient},
    {1024, 0}
};

DECISION NinjaSurprised[] =
{
    {701, &AF(InitActorMoveCloser)},
    {1024, &AF(InitActorDecide)}
};

DECISION NinjaEvasive[] =
{
    {400,   &AF(InitActorDuck)}, // 100
    {1024,  nullptr}
};

DECISION NinjaLostTarget[] =
{
    {900, &AF(InitActorFindPlayer)},
    {1024, &AF(InitActorWanderAround)}
};

DECISION NinjaCloseRange[] =
{
    {700,   &AF(InitActorAttack    )         },
    {1024,  &AF(InitActorReposition)         }
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
    {1023, &AF(InitActorDuck)},
    {1024, &AF(InitActorSetDecide)},
};

DECISIONB NinjaSniperBroadcast2[] =
{
    {1023, 0},
    {1024, attr_ambient},
};

DECISION NinjaSniperBattle[] =
{
    {499, &AF(InitActorDuck)},
    {500, &AF(InitActorSetDecide)},
    {1024, &AF(InitActorAttack)}
};

PERSONALITY NinjaSniperPersonality =
{
    NinjaSniperBattle,
    NinjaSniperBattle,
    NinjaSniperBroadcast2,
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

//////////////////////
//
// NINJA RUN
//
//////////////////////

#define NINJA_RATE 18

FState s_NinjaRun[] =
{

        {SPR_NINJA_RUN, 'A', NINJA_RATE | SF_TIC_ADJUST, &AF(DoNinjaMove), &s_NinjaRun[1]},
        {SPR_NINJA_RUN, 'B', NINJA_RATE | SF_TIC_ADJUST, &AF(DoNinjaMove), &s_NinjaRun[2]},
        {SPR_NINJA_RUN, 'C', NINJA_RATE | SF_TIC_ADJUST, &AF(DoNinjaMove), &s_NinjaRun[3]},
        {SPR_NINJA_RUN, 'D', NINJA_RATE | SF_TIC_ADJUST, &AF(DoNinjaMove), &s_NinjaRun[0]},
};


//////////////////////
//
// NINJA STAND
//
//////////////////////

#define NINJA_STAND_RATE 10

FState s_NinjaStand[] =
{
        {SPR_NINJA_STAND, 'A', NINJA_STAND_RATE, &AF(DoNinjaMove), &s_NinjaStand[0]},
};


//////////////////////
//
// NINJA RISE
//
//////////////////////

#define NINJA_RISE_RATE 10

FState s_NinjaRise[] =
{
        {SPR_NINJA_CRAWL, 'A', NINJA_RISE_RATE, &AF(NullNinja), &s_NinjaRise[1]},
        {SPR_NINJA_STAND, 'A', NINJA_STAND_RATE, &AF(NullNinja), &s_NinjaRise[2]},
        {SPR_NULL, 0, 0, nullptr, &s_NinjaRun[0]},
};



//////////////////////
//
// NINJA CRAWL
//
//////////////////////


#define NINJA_CRAWL_RATE 14
FState s_NinjaCrawl[] =
{
        {SPR_NINJA_CRAWL, 'A', NINJA_CRAWL_RATE, &AF(DoNinjaMove), &s_NinjaCrawl[1]},
        {SPR_NINJA_CRAWL, 'B', NINJA_CRAWL_RATE, &AF(DoNinjaMove), &s_NinjaCrawl[2]},
        {SPR_NINJA_CRAWL, 'C', NINJA_CRAWL_RATE, &AF(DoNinjaMove), &s_NinjaCrawl[3]},
        {SPR_NINJA_CRAWL, 'B', NINJA_CRAWL_RATE, &AF(DoNinjaMove), &s_NinjaCrawl[0]},
};


//////////////////////
//
// NINJA KNEEL_CRAWL
//
//////////////////////

#define NINJA_KNEEL_CRAWL_RATE 20

FState s_NinjaKneelCrawl[] =
{
        {SPR_NINJA_CRAWL, 'A', NINJA_KNEEL_CRAWL_RATE, &AF(NullNinja), &s_NinjaKneelCrawl[1]},
        {SPR_NINJA_CRAWL, 'A', NINJA_CRAWL_RATE, &AF(DoNinjaMove), &s_NinjaKneelCrawl[2]},
        {SPR_NINJA_CRAWL, 'B', NINJA_CRAWL_RATE, &AF(DoNinjaMove), &s_NinjaKneelCrawl[3]},
        {SPR_NINJA_CRAWL, 'C', NINJA_CRAWL_RATE, &AF(DoNinjaMove), &s_NinjaKneelCrawl[4]},
        {SPR_NINJA_CRAWL, 'B', NINJA_CRAWL_RATE, &AF(DoNinjaMove), &s_NinjaKneelCrawl[1]},
};


//////////////////////
//
// NINJA DUCK
//
//////////////////////

#define NINJA_DUCK_RATE 10

FState s_NinjaDuck[] =
{
        {SPR_NINJA_CRAWL, 'A', NINJA_DUCK_RATE, &AF(NullNinja), &s_NinjaDuck[1]},
        {SPR_NINJA_CRAWL, 'A', NINJA_CRAWL_RATE, &AF(DoNinjaMove), &s_NinjaDuck[1]},
};


//////////////////////
//
// NINJA SIT
//
//////////////////////

FState s_NinjaSit[] =
{
        {SPR_NINJA_CRAWL, 'A', NINJA_RISE_RATE, &AF(DoNinjaMove), &s_NinjaSit[0]},
};


//////////////////////
//
// NINJA CEILING
//
//////////////////////

FState s_NinjaCeiling[] =
{
        {SPR_NINJA_CRAWL, 'A', NINJA_RISE_RATE, &AF(DoNinjaCeiling), &s_NinjaCeiling[0]},
};


//////////////////////
//
// NINJA JUMP
//
//////////////////////

#define NINJA_JUMP_RATE 24

FState s_NinjaJump[] =
{
        {SPR_NINJA_JUMP, 'A', NINJA_JUMP_RATE, &AF(DoNinjaMove), &s_NinjaJump[1]},
        {SPR_NINJA_JUMP, 'B', NINJA_JUMP_RATE, &AF(DoNinjaMove), &s_NinjaJump[1]},
};


//////////////////////
//
// NINJA FALL
//
//////////////////////

#define NINJA_FALL_RATE 16

FState s_NinjaFall[] =
{
        {SPR_NINJA_JUMP, 'B', NINJA_FALL_RATE, &AF(DoNinjaMove), &s_NinjaFall[1]},
        {SPR_NINJA_JUMP, 'C', NINJA_FALL_RATE, &AF(DoNinjaMove), &s_NinjaFall[1]},
};


//////////////////////
//
// NINJA SWIM
//
//////////////////////


#define NINJA_SWIM_RATE 18
FState s_NinjaSwim[] =
{
        {SPR_NINJA_SWIM, 'B', NINJA_SWIM_RATE, &AF(DoNinjaMove), &s_NinjaSwim[1]},
        {SPR_NINJA_SWIM, 'C', NINJA_SWIM_RATE, &AF(DoNinjaMove), &s_NinjaSwim[2]},
        {SPR_NINJA_SWIM, 'D', NINJA_SWIM_RATE, &AF(DoNinjaMove), &s_NinjaSwim[0]},
};


//////////////////////
//
// NINJA DIVE
//
//////////////////////


#define NINJA_DIVE_RATE 23

FState s_NinjaDive[] =
{
        {SPR_NINJA_SWIM, 'A', NINJA_DIVE_RATE, &AF(DoNinjaMove), &s_NinjaDive[1]},
        {SPR_NINJA_SWIM, 'B', NINJA_DIVE_RATE, &AF(DoNinjaMove), &s_NinjaDive[2]},
        {SPR_NINJA_SWIM, 'C', NINJA_DIVE_RATE, &AF(DoNinjaMove), &s_NinjaDive[3]},
        {SPR_NINJA_SWIM, 'D', NINJA_DIVE_RATE, &AF(DoNinjaMove), &s_NinjaDive[0]},
};

//////////////////////
//
// NINJA CLIMB
//
//////////////////////


#define NINJA_CLIMB_RATE 20
FState s_NinjaClimb[] =
{
        {SPR_NINJA_CLIMB, 'A', NINJA_CLIMB_RATE, &AF(DoNinjaMove), &s_NinjaClimb[1]},
        {SPR_NINJA_CLIMB, 'B', NINJA_CLIMB_RATE, &AF(DoNinjaMove), &s_NinjaClimb[2]},
        {SPR_NINJA_CLIMB, 'C', NINJA_CLIMB_RATE, &AF(DoNinjaMove), &s_NinjaClimb[3]},
        {SPR_NINJA_CLIMB, 'D', NINJA_CLIMB_RATE, &AF(DoNinjaMove), &s_NinjaClimb[0]},
};

//////////////////////
//
// NINJA FLY
//
//////////////////////

#define NINJA_FLY_RATE 12

FState s_NinjaFly[] =
{
        {SPR_NINJA_FLY, 'A', NINJA_FLY_RATE, &AF(DoNinjaMove), &s_NinjaFly[0]},
};

//////////////////////
//
// NINJA PAIN
//
//////////////////////

#define NINJA_PAIN_RATE 15

FState s_NinjaPain[] =
{
        {SPR_NINJA_PAIN, 'A', NINJA_PAIN_RATE, &AF(DoNinjaPain), &s_NinjaPain[1]},
        {SPR_NINJA_PAIN, 'B', NINJA_PAIN_RATE, &AF(DoNinjaPain), &s_NinjaPain[1]},
};

//////////////////////
//
// NINJA STAR
//
//////////////////////

#define NINJA_STAR_RATE 18

FState s_NinjaStar[] =
{
        {SPR_NINJA_THROW, 'A', NINJA_STAR_RATE * 2,     &AF(NullNinja),            &s_NinjaStar[1]},
        {SPR_NINJA_THROW, 'A', NINJA_STAR_RATE,       &AF(NullNinja),          &s_NinjaStar[2]},
        {SPR_NINJA_THROW, 'B', 0 | SF_QUICK_CALL,         &AF(InitEnemyStar),      &s_NinjaStar[3]},
        {SPR_NINJA_THROW, 'B', NINJA_STAR_RATE * 2,     &AF(NullNinja),            &s_NinjaStar[4]},
        {SPR_NINJA_THROW, 'C', 0 | SF_QUICK_CALL,         &AF(InitActorDecide),    &s_NinjaStar[5]},
        {SPR_NINJA_THROW, 'C', NINJA_STAR_RATE,       &AF(DoNinjaMove),        &s_NinjaStar[5]},
};

//////////////////////
//
// NINJA MIRV
//
//////////////////////

#define NINJA_MIRV_RATE 18

FState s_NinjaMirv[] =
{
        {SPR_NINJA_THROW, 'A', NINJA_MIRV_RATE * 2, &AF(NullNinja), &s_NinjaMirv[1]},
        {SPR_NINJA_THROW, 'B', NINJA_MIRV_RATE, &AF(NullNinja), &s_NinjaMirv[2]},
        {SPR_NINJA_THROW, 'C', 0 | SF_QUICK_CALL, &AF(InitEnemyMirv), &s_NinjaMirv[3]},
        {SPR_NINJA_THROW, 'C', NINJA_MIRV_RATE * 2, &AF(NullNinja), &s_NinjaMirv[4]},
        {SPR_NINJA_THROW, 'C', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_NinjaMirv[5]},
        {SPR_NINJA_THROW, 'C', NINJA_MIRV_RATE, &AF(DoNinjaMove), &s_NinjaMirv[5]},
};


//////////////////////
//
// NINJA NAPALM
//
//////////////////////

#define NINJA_NAPALM_RATE 18

FState s_NinjaNapalm[] =
{
        {SPR_NINJA_THROW, 'A', NINJA_NAPALM_RATE * 2, &AF(NullNinja), &s_NinjaNapalm[1]},
        {SPR_NINJA_THROW, 'B', NINJA_NAPALM_RATE, &AF(NullNinja), &s_NinjaNapalm[2]},
        {SPR_NINJA_THROW, 'C', 0 | SF_QUICK_CALL, &AF(InitEnemyNapalm), &s_NinjaNapalm[3]},
        {SPR_NINJA_THROW, 'C', NINJA_NAPALM_RATE * 2, &AF(NullNinja), &s_NinjaNapalm[4]},
        {SPR_NINJA_THROW, 'C', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_NinjaNapalm[5]},
        {SPR_NINJA_THROW, 'C', NINJA_NAPALM_RATE, &AF(DoNinjaMove), &s_NinjaNapalm[5]},
};


//////////////////////
//
// NINJA ROCKET
//
//////////////////////

#define NINJA_ROCKET_RATE 14

FState s_NinjaRocket[] =
{
        {SPR_NINJA_STAND, 'A', NINJA_ROCKET_RATE * 2, &AF(NullNinja), &s_NinjaRocket[1]},
        {SPR_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyRocket), &s_NinjaRocket[2]},
        {SPR_NINJA_STAND, 'A', NINJA_ROCKET_RATE, &AF(NullNinja), &s_NinjaRocket[3]},
        {SPR_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_NinjaRocket[4]},
        {SPR_NINJA_STAND, 'A', NINJA_ROCKET_RATE, &AF(DoNinjaMove), &s_NinjaRocket[4]},
};


//////////////////////
//
// NINJA ROCKET
//
//////////////////////

#define NINJA_ROCKET_RATE 14

FState s_NinjaGrenade[] =
{
        {SPR_NINJA_STAND, 'A', NINJA_ROCKET_RATE * 2, &AF(NullNinja), &s_NinjaGrenade[1]},
        {SPR_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitSpriteGrenade), &s_NinjaGrenade[2]},
        {SPR_NINJA_STAND, 'A', NINJA_ROCKET_RATE, &AF(NullNinja), &s_NinjaGrenade[3]},
        {SPR_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_NinjaGrenade[4]},
        {SPR_NINJA_STAND, 'A', NINJA_ROCKET_RATE, &AF(DoNinjaMove), &s_NinjaGrenade[4]},
};


//////////////////////
//
// NINJA FLASHBOMB
//
//////////////////////

#define NINJA_FLASHBOMB_RATE 14

FState s_NinjaFlashBomb[] =
{
        {SPR_NINJA_STAND, 'A', NINJA_FLASHBOMB_RATE * 2, &AF(NullNinja), &s_NinjaFlashBomb[1]},
        {SPR_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitFlashBomb), &s_NinjaFlashBomb[2]},
        {SPR_NINJA_STAND, 'A', NINJA_FLASHBOMB_RATE, &AF(NullNinja), &s_NinjaFlashBomb[3]},
        {SPR_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_NinjaFlashBomb[4]},
        {SPR_NINJA_STAND, 'A', NINJA_FLASHBOMB_RATE, &AF(DoNinjaMove), &s_NinjaFlashBomb[4]},
};


//////////////////////
//
// NINJA UZI
//
//////////////////////

#define NINJA_UZI_RATE 8

FState s_NinjaUzi[] =
{
        {SPR_NINJA_FIRE, 'A', NINJA_UZI_RATE, &AF(NullNinja), &s_NinjaUzi[1]},
        {SPR_NINJA_FIRE, 'A', 0 | SF_QUICK_CALL, &AF(CheckFire), &s_NinjaUzi[2]},
        {SPR_NINJA_FIRE, 'B', NINJA_UZI_RATE, &AF(NullNinja), &s_NinjaUzi[3]},
        {SPR_NINJA_FIRE, 'B', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_NinjaUzi[4]},
        {SPR_NINJA_FIRE, 'A', NINJA_UZI_RATE, &AF(NullNinja), &s_NinjaUzi[5]},
        {SPR_NINJA_FIRE, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_NinjaUzi[6]},
        {SPR_NINJA_FIRE, 'B', NINJA_UZI_RATE, &AF(NullNinja), &s_NinjaUzi[7]},
        {SPR_NINJA_FIRE, 'B', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_NinjaUzi[8]},
        {SPR_NINJA_FIRE, 'A', NINJA_UZI_RATE, &AF(NullNinja), &s_NinjaUzi[9]},
        {SPR_NINJA_FIRE, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_NinjaUzi[10]},
        {SPR_NINJA_FIRE, 'B', NINJA_UZI_RATE, &AF(NullNinja), &s_NinjaUzi[11]},
        {SPR_NINJA_FIRE, 'B', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_NinjaUzi[12]},
        {SPR_NINJA_FIRE, 'A', NINJA_UZI_RATE, &AF(NullNinja), &s_NinjaUzi[13]},
        {SPR_NINJA_FIRE, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_NinjaUzi[14]},
        {SPR_NINJA_FIRE, 'B', NINJA_UZI_RATE, &AF(NullNinja), &s_NinjaUzi[15]},
        {SPR_NINJA_FIRE, 'B', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_NinjaUzi[16]},
        {SPR_NINJA_FIRE, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_NinjaUzi[16]},
};


//////////////////////
//
// NINJA HARI KARI
//
//////////////////////

#define NINJA_HARI_KARI_WAIT_RATE 200
#define NINJA_HARI_KARI_FALL_RATE 16

FState s_NinjaHariKari[] =
{
    {SPR_NINJA_HARI_KARI, 'A',   NINJA_HARI_KARI_FALL_RATE,      &AF(NullNinja),       &s_NinjaHariKari[1]},
    {SPR_NINJA_HARI_KARI, 'A',   SF_QUICK_CALL,                  &AF(DoNinjaSpecial),       &s_NinjaHariKari[2]},
    {SPR_NINJA_HARI_KARI, 'B',   NINJA_HARI_KARI_WAIT_RATE,      &AF(NullNinja),       &s_NinjaHariKari[3]},
    {SPR_NINJA_HARI_KARI, 'C',   SF_QUICK_CALL,                  &AF(DoNinjaHariKari), &s_NinjaHariKari[4]},
    {SPR_NINJA_HARI_KARI, 'C',   NINJA_HARI_KARI_FALL_RATE,      nullptr,        &s_NinjaHariKari[5]},
    {SPR_NINJA_HARI_KARI, 'D',   NINJA_HARI_KARI_FALL_RATE,      nullptr,        &s_NinjaHariKari[6]},
    {SPR_NINJA_HARI_KARI, 'E',   NINJA_HARI_KARI_FALL_RATE,      nullptr,        &s_NinjaHariKari[7]},
    {SPR_NINJA_HARI_KARI, 'F',   NINJA_HARI_KARI_FALL_RATE,      nullptr,        &s_NinjaHariKari[8]},
    {SPR_NINJA_HARI_KARI, 'G',   NINJA_HARI_KARI_FALL_RATE,      nullptr,        &s_NinjaHariKari[9]},
    {SPR_NINJA_HARI_KARI, 'H',   NINJA_HARI_KARI_FALL_RATE,      nullptr,        &s_NinjaHariKari[10]},
    {SPR_NINJA_HARI_KARI, 'H',   NINJA_HARI_KARI_FALL_RATE,      nullptr,        &s_NinjaHariKari[10]},
};

//////////////////////
//
// NINJA GRAB THROAT
//
//////////////////////

#define NINJA_GRAB_THROAT_RATE 32

FState s_NinjaGrabThroat[] =
{
    {SPR_NINJA_GRAB_THROAT, 'A',   NINJA_GRAB_THROAT_RATE,      &AF(NullNinja),       &s_NinjaGrabThroat[1]},
    {SPR_NINJA_GRAB_THROAT, 'A',   SF_QUICK_CALL,               &AF(DoNinjaSpecial),  &s_NinjaGrabThroat[2]},
    {SPR_NINJA_GRAB_THROAT, 'B',   NINJA_GRAB_THROAT_RATE,      &AF(NullNinja),       &s_NinjaGrabThroat[3]},
    {SPR_NINJA_GRAB_THROAT, 'C',   SF_QUICK_CALL,               &AF(DoNinjaGrabThroat), &s_NinjaGrabThroat[4]},
    {SPR_NINJA_GRAB_THROAT, 'C',   NINJA_GRAB_THROAT_RATE,      &AF(NullNinja),       &s_NinjaGrabThroat[5]},
    {SPR_NINJA_GRAB_THROAT, 'B',   NINJA_GRAB_THROAT_RATE,      &AF(NullNinja),       &s_NinjaGrabThroat[0]},
};

//////////////////////
//
// NINJA DIE
//
//////////////////////

#define NINJA_DIE_RATE 14

FState s_NinjaDie[] =
{
    {SPR_NINJA_DIE, 'A', NINJA_DIE_RATE, &AF(NullNinja), &s_NinjaDie[1]},
    {SPR_NINJA_DIE, 'B', NINJA_DIE_RATE, &AF(NullNinja), &s_NinjaDie[2]},
    {SPR_NINJA_DIE, 'C', NINJA_DIE_RATE, &AF(NullNinja), &s_NinjaDie[3]},
    {SPR_NINJA_DIE, 'D', NINJA_DIE_RATE, &AF(NullNinja), &s_NinjaDie[4]},
    {SPR_NINJA_DIE, 'E', NINJA_DIE_RATE, &AF(NullNinja), &s_NinjaDie[5]},
    {SPR_NINJA_DIE, 'F', NINJA_DIE_RATE-4, &AF(NullNinja), &s_NinjaDie[6]},
    {SPR_NINJA_DIE, 'G', NINJA_DIE_RATE-6, &AF(NullNinja), &s_NinjaDie[7]},
    {SPR_NINJA_DIE, 'G', SF_QUICK_CALL, &AF(DoNinjaSpecial), &s_NinjaDie[8]},
    {SPR_NINJA_DIE, 'G', NINJA_DIE_RATE-10, &AF(NullNinja), &s_NinjaDie[9]},
    {SPR_NINJA_DIE, 'H', SF_QUICK_CALL, &AF(QueueFloorBlood), &s_NinjaDie[10]},
    {SPR_NINJA_DIE, 'H', NINJA_DIE_RATE-12, &AF(DoActorDebris), &s_NinjaDie[10]},
};


#define NINJA_DIESLICED_RATE 20

FState s_NinjaDieSliced[] =
{
    {SPR_NINJA_SLICED, 'A', NINJA_DIESLICED_RATE*6, &AF(NullNinja), &s_NinjaDieSliced[1]},
    {SPR_NINJA_SLICED, 'B', NINJA_DIESLICED_RATE,   &AF(NullNinja), &s_NinjaDieSliced[2]},
    {SPR_NINJA_SLICED, 'C', NINJA_DIESLICED_RATE,   &AF(NullNinja), &s_NinjaDieSliced[3]},
    {SPR_NINJA_SLICED, 'D', NINJA_DIESLICED_RATE,   &AF(NullNinja), &s_NinjaDieSliced[4]},
    {SPR_NINJA_SLICED, 'E', NINJA_DIESLICED_RATE-1, &AF(NullNinja), &s_NinjaDieSliced[5]},
    {SPR_NINJA_SLICED, 'F', NINJA_DIESLICED_RATE-2, &AF(NullNinja), &s_NinjaDieSliced[6]},
    {SPR_NINJA_SLICED, 'G', NINJA_DIESLICED_RATE-3, &AF(NullNinja), &s_NinjaDieSliced[7]},
    {SPR_NINJA_SLICED, 'H', NINJA_DIESLICED_RATE-4, &AF(NullNinja), &s_NinjaDieSliced[8]},
    {SPR_NINJA_SLICED, 'H', SF_QUICK_CALL, &AF(DoNinjaSpecial), &s_NinjaDieSliced[9]},
    {SPR_NINJA_SLICED, 'I', NINJA_DIESLICED_RATE-5, &AF(NullNinja), &s_NinjaDieSliced[10]},
    {SPR_NINJA_SLICED, 'J', SF_QUICK_CALL, &AF(QueueFloorBlood), &s_NinjaDieSliced[11]},
    {SPR_NINJA_SLICED, 'J', NINJA_DIESLICED_RATE, &AF(DoActorDebris), &s_NinjaDieSliced[11]},
    };

FState s_NinjaDieSlicedHack[] =
    {
    {SPR_NINJA_SLICED_HACK, 'A', NINJA_DIESLICED_RATE*6, &AF(NullNinja), &s_NinjaDieSlicedHack[1]},
    {SPR_NINJA_SLICED_HACK, 'B', NINJA_DIESLICED_RATE,   &AF(NullNinja), &s_NinjaDieSlicedHack[2]},
    {SPR_NINJA_SLICED_HACK, 'C', NINJA_DIESLICED_RATE,   &AF(NullNinja), &s_NinjaDieSlicedHack[3]},
    {SPR_NINJA_SLICED_HACK, 'D', NINJA_DIESLICED_RATE,   &AF(NullNinja), &s_NinjaDieSlicedHack[4]},
    {SPR_NINJA_SLICED_HACK, 'E', NINJA_DIESLICED_RATE-1, &AF(NullNinja), &s_NinjaDieSlicedHack[5]},
    {SPR_NINJA_SLICED_HACK, 'E', NINJA_DIESLICED_RATE-2, &AF(NullNinja), &s_NinjaDieSlicedHack[6]},
    {SPR_NINJA_SLICED_HACK, 'F', NINJA_DIESLICED_RATE-3, &AF(NullNinja), &s_NinjaDieSlicedHack[7]},
    {SPR_NINJA_SLICED_HACK, 'F', NINJA_DIESLICED_RATE-4, &AF(NullNinja), &s_NinjaDieSlicedHack[8]},
    {SPR_NINJA_SLICED_HACK, 'G', SF_QUICK_CALL        , &AF(DoNinjaSpecial), &s_NinjaDieSlicedHack[9]},
    {SPR_NINJA_SLICED_HACK, 'G', NINJA_DIESLICED_RATE-5, &AF(NullNinja), &s_NinjaDieSlicedHack[10]},
    {SPR_NINJA_SLICED_HACK, 'H', SF_QUICK_CALL         , &AF(QueueFloorBlood), &s_NinjaDieSlicedHack[11]},
    {SPR_NINJA_SLICED_HACK, 'H', NINJA_DIESLICED_RATE-6, &AF(DoActorDebris), &s_NinjaDieSlicedHack[11]},
    };

FState s_NinjaDead[] =
{
    {SPR_NINJA_DIE, 'F', NINJA_DIE_RATE, &AF(DoActorDebris), &s_NinjaDead[1]},
    {SPR_NINJA_DIE, 'G', SF_QUICK_CALL, &AF(DoNinjaSpecial), &s_NinjaDead[2]},
    {SPR_NINJA_DIE, 'G', NINJA_DIE_RATE, &AF(DoActorDebris), &s_NinjaDead[3]},
    {SPR_NINJA_DIE, 'H', SF_QUICK_CALL, &AF(QueueFloorBlood),&s_NinjaDead[4]},
    {SPR_NINJA_DIE, 'H', NINJA_DIE_RATE, &AF(DoActorDebris), &s_NinjaDead[4]},
};


FState s_NinjaDeathJump[] =
{
    {SPR_NINJA_DIE, 'A', NINJA_DIE_RATE, &AF(DoActorDeathMove), &s_NinjaDeathJump[1]},
    {SPR_NINJA_DIE, 'B', NINJA_DIE_RATE, &AF(DoActorDeathMove), &s_NinjaDeathJump[2]},
    {SPR_NINJA_DIE, 'C', NINJA_DIE_RATE, &AF(DoActorDeathMove), &s_NinjaDeathJump[2]},
};

FState s_NinjaDeathFall[] =
{
    {SPR_NINJA_DIE, 'D', NINJA_DIE_RATE, &AF(DoActorDeathMove), &s_NinjaDeathFall[1]},
    {SPR_NINJA_DIE, 'E', NINJA_DIE_RATE, &AF(DoActorDeathMove), &s_NinjaDeathFall[1]},
};

/*
FState* *Stand[MAX_WEAPONS];
FState* *Run;
FState* *Jump;
FState* *Fall;
FState* *Crawl;
FState* *Swim;
FState* *Fly;
FState* *Rise;
FState* *Sit;
FState* *Look;
FState* *Climb;
FState* *Pain;
FState* *Death1;
FState* *Death2;
FState* *Dead;
FState* *DeathJump;
FState* *DeathFall;
FState* *CloseAttack[2];
FState* *Attack[6];
FState* *Special[2];
*/

/*

 !AIC - Collection of states that connect action to states

*/

ACTOR_ACTION_SET NinjaSniperActionSet =
{
    s_NinjaDuck,
    s_NinjaCrawl,
    s_NinjaJump,
    s_NinjaFall,
    s_NinjaKneelCrawl,
    s_NinjaSwim,
    s_NinjaFly,
    s_NinjaUzi,
    s_NinjaDuck,
    nullptr,
    s_NinjaClimb,
    s_NinjaPain,
    s_NinjaDie,
    s_NinjaHariKari,
    s_NinjaDead,
    s_NinjaDeathJump,
    s_NinjaDeathFall,
    {s_NinjaUzi},
    {1024},
    {s_NinjaUzi},
    {1024},
    {nullptr},
    s_NinjaDuck,
    s_NinjaDive
};

ACTOR_ACTION_SET NinjaActionSet =
{
    s_NinjaStand,
    s_NinjaRun,
    s_NinjaJump,
    s_NinjaFall,
    s_NinjaKneelCrawl,
    s_NinjaSwim,
    s_NinjaFly,
    s_NinjaRise,
    s_NinjaSit,
    nullptr,
    s_NinjaClimb,
    s_NinjaPain,
    s_NinjaDie,
    s_NinjaHariKari,
    s_NinjaDead,
    s_NinjaDeathJump,
    s_NinjaDeathFall,
    {s_NinjaUzi, s_NinjaStar},
    {1000, 1024},
    {s_NinjaUzi, s_NinjaStar},
    {800, 1024},
    {nullptr},
    s_NinjaDuck,
    s_NinjaDive
};

ACTOR_ACTION_SET NinjaRedActionSet =
{
    s_NinjaStand,
    s_NinjaRun,
    s_NinjaJump,
    s_NinjaFall,
    s_NinjaKneelCrawl,
    s_NinjaSwim,
    s_NinjaFly,
    s_NinjaRise,
    s_NinjaSit,
    nullptr,
    s_NinjaClimb,
    s_NinjaPain,
    s_NinjaDie,
    s_NinjaHariKari,
    s_NinjaDead,
    s_NinjaDeathJump,
    s_NinjaDeathFall,
    {s_NinjaUzi, s_NinjaUzi},
    {812, 1024},
    {s_NinjaUzi, s_NinjaRocket},
    {812, 1024},
    {nullptr},
    s_NinjaDuck,
    s_NinjaDive
};

ACTOR_ACTION_SET NinjaSeekerActionSet =
{
    s_NinjaStand,
    s_NinjaRun,
    s_NinjaJump,
    s_NinjaFall,
    s_NinjaKneelCrawl,
    s_NinjaSwim,
    s_NinjaFly,
    s_NinjaRise,
    s_NinjaSit,
    nullptr,
    s_NinjaClimb,
    s_NinjaPain,
    s_NinjaDie,
    s_NinjaHariKari,
    s_NinjaDead,
    s_NinjaDeathJump,
    s_NinjaDeathFall,
    {s_NinjaUzi, s_NinjaStar},
    {812, 1024},
    {s_NinjaUzi, s_NinjaRocket},
    {812, 1024},
    {nullptr},
    s_NinjaDuck,
    s_NinjaDive
};

ACTOR_ACTION_SET NinjaGrenadeActionSet =
{
    s_NinjaStand,
    s_NinjaRun,
    s_NinjaJump,
    s_NinjaFall,
    s_NinjaKneelCrawl,
    s_NinjaSwim,
    s_NinjaFly,
    s_NinjaRise,
    s_NinjaSit,
    nullptr,
    s_NinjaClimb,
    s_NinjaPain,
    s_NinjaDie,
    s_NinjaHariKari,
    s_NinjaDead,
    s_NinjaDeathJump,
    s_NinjaDeathFall,
    {s_NinjaUzi, s_NinjaUzi},
    {812, 1024},
    {s_NinjaUzi, s_NinjaGrenade},
    {812, 1024},
    {nullptr},
    s_NinjaDuck,
    s_NinjaDive
};

ACTOR_ACTION_SET NinjaGreenActionSet =
{
    s_NinjaStand,
    s_NinjaRun,
    s_NinjaJump,
    s_NinjaFall,
    s_NinjaKneelCrawl,
    s_NinjaSwim,
    s_NinjaFly,
    s_NinjaRise,
    s_NinjaSit,
    nullptr,
    s_NinjaClimb,
    s_NinjaPain,
    s_NinjaDie,
    s_NinjaHariKari,
    s_NinjaDead,
    s_NinjaDeathJump,
    s_NinjaDeathFall,
    {s_NinjaUzi, s_NinjaFlashBomb},
    {912, 1024},
    {s_NinjaFlashBomb, s_NinjaUzi, s_NinjaMirv, s_NinjaNapalm},
    {150, 500, 712, 1024},
    {nullptr},
    s_NinjaDuck,
    s_NinjaDive
};



/*

 !AIC - Every actor has a setup where they are initialized

*/

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupNinja(DSWActor* actor)
{
    short pic = actor->spr.picnum;

    // Fake some skill settings in case the lower skills are empty.
    int RedNinjaHealth = MinEnemySkill <= Skill ? HEALTH_RED_NINJA : HEALTH_NINJA;
    if (Skill < MinEnemySkill - 1) actor->spr.pal = 0;

    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, NINJA_RUN_R0, s_NinjaRun[0]);
        actor->user.Health = HEALTH_NINJA;
    }

	actor->spr.scale = DVector2(0.71875, 0.71875);

    if (actor->spr.pal == PALETTE_PLAYER5)
    {
        actor->user.__legacyState.Attrib = &InvisibleNinjaAttrib;
        EnemyDefaults(actor, &NinjaGreenActionSet, &NinjaPersonality);
        if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
            actor->user.Health = RedNinjaHealth;
        actor->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT);
        actor->spr.shade = 127;
        actor->spr.pal = actor->user.spal = PALETTE_PLAYER5;
        actor->spr.hitag = 9998;
        if (pic == NINJA_CRAWL_R0)
        {
            if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
            {
                actor->user.__legacyState.Attrib = &NinjaAttrib;
                actor->user.__legacyState.ActorActionSet = &NinjaActionSet;
                actor->user.Personality = &NinjaPersonality;
                ChangeState(actor, s_NinjaCeiling[0]);
            }
            else
            {
                actor->user.__legacyState.Attrib = &NinjaAttrib;
                actor->user.__legacyState.ActorActionSet = &NinjaSniperActionSet;
                actor->user.Personality = &NinjaSniperPersonality;
                ChangeState(actor, s_NinjaDuck[0]);
            }
        }
    }
    else if (actor->spr.pal == PALETTE_PLAYER3)
    {
        actor->user.__legacyState.Attrib = &NinjaAttrib;
        EnemyDefaults(actor, &NinjaRedActionSet, &NinjaPersonality);
        if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
            actor->user.Health = RedNinjaHealth;
        actor->spr.pal = actor->user.spal = PALETTE_PLAYER3;
        if (pic == NINJA_CRAWL_R0)
        {
            if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
            {
                actor->user.__legacyState.Attrib = &NinjaAttrib;
                actor->user.__legacyState.ActorActionSet = &NinjaActionSet;
                actor->user.Personality = &NinjaPersonality;
                ChangeState(actor, s_NinjaCeiling[0]);
            }
            else
            {
                actor->user.__legacyState.Attrib = &NinjaAttrib;
                actor->user.__legacyState.ActorActionSet = &NinjaSniperActionSet;
                actor->user.Personality = &NinjaSniperPersonality;
                ChangeState(actor, s_NinjaDuck[0]);
            }
        }
    }
    else if (actor->spr.pal == PAL_XLAT_LT_TAN)
    {
        actor->user.__legacyState.Attrib = &NinjaAttrib;
        EnemyDefaults(actor, &NinjaSeekerActionSet, &NinjaPersonality);
        if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
            actor->user.Health = RedNinjaHealth;
        actor->spr.pal = actor->user.spal = PAL_XLAT_LT_TAN;
        actor->user.__legacyState.Attrib = &NinjaAttrib;
    }
    else if (actor->spr.pal == PAL_XLAT_LT_GREY)
    {
        actor->user.__legacyState.Attrib = &NinjaAttrib;
        EnemyDefaults(actor, &NinjaGrenadeActionSet, &NinjaPersonality);
        if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
            actor->user.Health = RedNinjaHealth;
        actor->spr.pal = actor->user.spal = PAL_XLAT_LT_GREY;
        actor->user.__legacyState.Attrib = &NinjaAttrib;
    }
    else
    {
        actor->user.__legacyState.Attrib = &NinjaAttrib;
        actor->spr.pal = actor->user.spal = PALETTE_PLAYER0;
        EnemyDefaults(actor, &NinjaActionSet, &NinjaPersonality);
        if (pic == NINJA_CRAWL_R0)
        {
            actor->user.__legacyState.Attrib = &NinjaAttrib;
            actor->user.__legacyState.ActorActionSet = &NinjaSniperActionSet;
            actor->user.Personality = &NinjaSniperPersonality;
            ChangeState(actor, s_NinjaDuck[0]);
        }
    }

    //actor->setStateGroup()
    actor->user.__legacyState.StateEnd = s_NinjaDie;
    actor->setStateGroup(NAME_Run, 0, true); // Something wrong with the sniper ninja...
    ChangeState(actor, s_NinjaRun[0]);
    DoActorSetSpeed(actor, NORM_SPEED);

    actor->user.Radius = 280;
    actor->user.Flags |= (SPR_XFLIP_TOGGLE);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWNinja, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupNinja(self);
    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoNinjaHariKari(DSWActor* actor)
{
    UpdateSinglePlayKills(actor);
    change_actor_stat(actor, STAT_DEAD_ACTOR);
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    actor->user.Flags |= (SPR_DEAD);
    actor->user.Flags &= ~(SPR_FALLING | SPR_JUMPING);
    actor->user.floor_dist = (40);
    
    actor->clearActionFunc();

    actor->spr.extra |= (SPRX_BREAKABLE);
    actor->spr.cstat |= (CSTAT_SPRITE_BREAKABLE);

    PlaySound(DIGI_NINJAUZIATTACK, actor, v3df_follow);

    SpawnBlood(actor, actor);

    int cnt = RandomRange(4)+1;
    for (int i=0; i<=cnt; i++)
        InitBloodSpray(actor,true,-2);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoNinjaGrabThroat(DSWActor* actor)
{
    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
    {
        UpdateSinglePlayKills(actor);
        actor->user.Flags2 &= ~(SPR2_DYING);
        actor->spr.cstat &= ~(CSTAT_SPRITE_YFLIP);
        change_actor_stat(actor, STAT_DEAD_ACTOR);
        actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        actor->user.Flags |= (SPR_DEAD);
        actor->user.Flags &= ~(SPR_FALLING | SPR_JUMPING);
        actor->user.floor_dist = (40);
        
        actor->clearActionFunc();

        actor->spr.extra |= (SPRX_BREAKABLE);
        actor->spr.cstat |= (CSTAT_SPRITE_BREAKABLE);


        actor->ChangeStateEnd();
        actor->vel.X = 0;
        PlaySound(DIGI_NINJASCREAM, actor, v3df_follow);
    }

    return 0;
}

/*

 !AIC - Most actors have one of these and the all look similar

*/

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoNinjaMove(DSWActor* actor)
{
    if (actor->user.Flags2 & (SPR2_DYING))
    {
        if (sw_ninjahack)
            NewStateGroup(actor, s_NinjaHariKari);
        else
            NewStateGroup(actor, s_NinjaGrabThroat);
        return 0;
    }

    // jumping and falling
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING) && !(actor->user.Flags & SPR_CLIMBING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else if (actor->user.Flags & (SPR_FALLING))
            DoActorFall(actor);
    }

    // sliding
    if (actor->user.Flags & (SPR_SLIDING) && !(actor->user.Flags & SPR_CLIMBING))
        DoActorSlide(actor);

    // !AIC - do track or call current action function - such as DoActorMoveCloser()
    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
    {
        actor->callAction();
    }

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING | SPR_CLIMBING)))
    {
        KeepActorOnFloor(actor);
    }

    // take damage from environment
    DoActorSectorDamage(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NinjaJumpActionFunc(DSWActor* actor)
{
    // if cannot move the sprite
    if (!move_actor(actor, DVector3(actor->spr.Angles.Yaw.ToVector() * actor->vel.X, 0)))
    {
        return 0;
    }

    if (!(actor->user.Flags & (SPR_JUMPING|SPR_FALLING)))
    {
        InitActorDecide(actor);
    }

    return 0;
}

/*

 !AIC - Short version of DoNinjaMove without the movement code.  For times when
 the actor is doing something but not moving.

*/

int NullNinja(DSWActor* actor)
{
    if (actor->user.WaitTics > 0) actor->user.WaitTics -= ACTORMOVETICS;

    if (actor->user.Flags & (SPR_SLIDING) && !(actor->user.Flags & SPR_CLIMBING) && !(actor->user.Flags & (SPR_JUMPING|SPR_FALLING)))
        DoActorSlide(actor);

    if (!(actor->user.Flags & SPR_CLIMBING) && !(actor->user.Flags & (SPR_JUMPING|SPR_FALLING)))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoNinjaPain(DSWActor* actor)
{
    NullNinja(actor);

    if (actor->user.Flags2 & (SPR2_DYING))
    {
        if (sw_ninjahack)
            NewStateGroup(actor, s_NinjaHariKari);
        else
            NewStateGroup(actor, s_NinjaGrabThroat);
        return 0;
    }

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoNinjaSpecial(DSWActor* actor)
{
    if (actor->user.spal == PALETTE_PLAYER5)
    {
        actor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);
        actor->spr.hitag = 0;
        actor->spr.shade = -10;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int CheckFire(DSWActor* actor)
{
    if (!CanSeePlayer(actor))
        InitActorDuck(actor);
    return 0;
}

int DoNinjaCeiling(DSWActor* actor)
{
    DoActorSectorDamage(actor);
    return 0;
}


//
// !AIC - Stuff from here down is really Player related.  Should be moved but it was
// too convienent to put it here.
//

void InitAllPlayerSprites(const DVector3& spawnpos, const DAngle startang)
{
    short i;

    TRAVERSE_CONNECT(i)
    {
        InitPlayerSprite(getPlayer(i), spawnpos, startang);
    }
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerLevelReset(DSWPlayer* pp)
{
    DSWActor* actor = pp->GetActor();

    if (gNet.MultiGameType == MULTI_GAME_COMMBAT)
    {
        PlayerDeathReset(pp);
        return;
    }

    if (pp->Flags & (PF_DIVING))
        DoPlayerStopDiveNoWarp(pp);

    COVER_SetReverb(0); // Turn off any echoing that may have been going before
    pp->Reverb = 0;
    pp->WpnFirstType = WPN_SWORD;
    //PlayerUpdateHealth(pp, 500);
    //pp->Armor = 0;
    //PlayerUpdateArmor(pp, 0);
    pp->KillerActor = nullptr;;
    pp->NightVision = false;
    pp->StartColor = 0;
    pp->FadeAmt = 0;
    pp->DeathType = 0;
    actor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
    actor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);
    pp->Flags &= ~(PF_WEAPON_DOWN|PF_WEAPON_RETRACT);
    pp->Flags &= ~(PF_DEAD);

    pp->sop_control = nullptr;
    pp->sop_riding = nullptr;
    pp->sop_remote = nullptr;
    pp->sop = nullptr;
    DoPlayerResetMovement(pp);
    DamageData[actor->user.WeaponNum].Init(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerDeathReset(DSWPlayer* pp)
{
    DSWActor* actor = pp->GetActor();

    if (pp->Flags & (PF_DIVING))
        DoPlayerStopDiveNoWarp(pp);

    COVER_SetReverb(0); // Turn off any echoing that may have been going before
    pp->Reverb = 0;
    // second weapon - whatever it is
    actor->user.WeaponNum = WPN_SWORD;
    pp->WpnFirstType = actor->user.WeaponNum;
    pp->WpnRocketType = 0;
    pp->WpnRocketHeat = 0;             // 5 to 0 range
    pp->WpnRocketNuke = 0;             // 1, you have it, or you don't
    pp->WpnFlameType = 0;              // Guardian weapons fire
    pp->WpnUziType = 2;
    pp->WpnShotgunType = 0;            // Shotgun has normal or fully automatic fire
    pp->WpnShotgunAuto = 0;            // 50-0 automatic shotgun rounds
    pp->WpnShotgunLastShell = 0;       // Number of last shell fired
    pp->Bloody = false;
    pp->TestNukeInit = false;
    pp->InitingNuke = false;
    pp->NukeInitialized = false;
    pp->WpnReloadState = 2;

    memset(pp->WpnAmmo,0,sizeof(pp->WpnAmmo));
    memset(pp->InventoryTics,0,sizeof(pp->InventoryTics));
    memset(pp->InventoryPercent,0,sizeof(pp->InventoryPercent));
    memset(pp->InventoryAmount,0,sizeof(pp->InventoryAmount));
    memset(pp->InventoryActive,0,sizeof(pp->InventoryActive));
    pp->WpnAmmo[WPN_STAR] = 30;
    pp->WpnAmmo[WPN_SWORD] = pp->WpnAmmo[WPN_FIST] = 30;
    pp->WpnFlags = 0;
    pp->WpnGotOnceFlags = 0;
    pp->WpnFlags |= (BIT(WPN_SWORD));
    pp->WpnFlags |= (BIT(WPN_FIST) | BIT(actor->user.WeaponNum));
    pp->WpnFlags |= (BIT(WPN_STAR) | BIT(actor->user.WeaponNum));
    pp->Flags &= ~(PF_PICKED_UP_AN_UZI);
    pp->Flags &= ~(PF_TWO_UZI);

    actor->user.Health = 100;
    pp->MaxHealth = 100;
    //PlayerUpdateHealth(pp, 500);
    puser[pp->pnum].Health = actor->user.Health;
    pp->Armor = 0;
    PlayerUpdateArmor(pp, 0);
    pp->KillerActor = nullptr;;
    pp->NightVision = false;
    pp->StartColor = 0;
    pp->FadeAmt = 0;
    pp->DeathType = 0;
    actor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);
    pp->Flags &= ~(PF_WEAPON_DOWN|PF_WEAPON_RETRACT);
    pp->Flags &= ~(PF_DEAD);

    pp->sop_control = nullptr;
    pp->sop_riding = nullptr;
    pp->sop_remote = nullptr;
    pp->sop = nullptr;
    DoPlayerResetMovement(pp);
    DamageData[actor->user.WeaponNum].Init(pp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerPanelSetup(void)
{
    short pnum;

    // For every player setup the panel weapon stuff
    //for (pp = Player; pp < &Player[numplayers]; pp++)
    TRAVERSE_CONNECT(pnum)
    {
        auto pp = getPlayer(pnum);

        ASSERT(pp->GetActor()->hasU());

        PlayerUpdateWeapon(pp, pp->GetActor()->user.WeaponNum);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerGameReset(DSWPlayer* pp)
{
    DSWActor* actor = pp->GetActor();

    COVER_SetReverb(0); // Turn off any echoing that may have been going before
    pp->Reverb = 0;
    actor->user.WeaponNum = WPN_SWORD;
    pp->WpnFirstType = actor->user.WeaponNum;
    pp->WpnRocketType = 0;
    pp->WpnRocketHeat = 0; // 5 to 0 range
    pp->WpnRocketNuke = 0; // 1, you have it, or you don't
    pp->WpnFlameType = 0; // Guardian weapons fire
    pp->WpnUziType = 2;
    pp->WpnShotgunType = 0;            // Shotgun has normal or fully automatic fire
    pp->WpnShotgunAuto = 0;            // 50-0 automatic shotgun rounds
    pp->WpnShotgunLastShell = 0;       // Number of last shell fired
    pp->Bloody = false;
    pp->TestNukeInit = false;
    pp->InitingNuke = false;
    pp->NukeInitialized = false;
    pp->WpnReloadState = 2;

    pp->WpnAmmo[WPN_STAR] = 30;
    pp->WpnAmmo[WPN_SWORD] = pp->WpnAmmo[WPN_FIST] = 30;
    pp->WpnFlags = 0;
    pp->WpnGotOnceFlags = 0;
    pp->WpnFlags |= (BIT(WPN_SWORD));
    pp->WpnFlags |= (BIT(WPN_FIST) | BIT(actor->user.WeaponNum));
    pp->WpnFlags |= (BIT(WPN_STAR) | BIT(actor->user.WeaponNum));
    pp->Flags &= ~(PF_PICKED_UP_AN_UZI);
    pp->Flags &= ~(PF_TWO_UZI);
    pp->MaxHealth = 100;
    PlayerUpdateHealth(pp, 500);
    pp->Armor = 0;
    PlayerUpdateArmor(pp, 0);
    pp->KillerActor = nullptr;;

    if (pp == getPlayer(screenpeek))
    {
        videoFadePalette(0,0,0,0);
    }
    pp->NightVision = false;
    pp->StartColor = 0;
    pp->FadeAmt = 0;
    pp->DeathType = 0;

    actor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);

    pp->sop_control = nullptr;
    pp->sop_riding = nullptr;
    pp->sop_remote = nullptr;
    pp->sop = nullptr;
    DoPlayerResetMovement(pp);
    DamageData[actor->user.WeaponNum].Init(pp);
}

extern ACTOR_ACTION_SET PlayerNinjaActionSet;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitPlayerSprite(DSWPlayer* pp, const DVector3& spawnpos, const DAngle startang)
{
    int pnum = int(pp->pnum);
    double fz,cz;
    extern bool NewGame;

    COVER_SetReverb(0); // Turn off any echoing that may have been going before
    pp->Reverb = 0;
    auto actor = SpawnActor(STAT_PLAYER0 + pnum, NINJA_RUN_R0, nullptr, pp->cursector, spawnpos.plusZ(PLAYER_HEIGHTF), startang);
    actor->viewzoffset = -PLAYER_HEIGHTF;

    // if too close to the floor - stand up
    calcSlope(pp->cursector, actor->getPosWithOffsetZ(), &cz, &fz);
    if (actor->spr.pos.Z > fz)
    {
        actor->spr.pos.Z = fz;
    }
    actor->backuploc();

    pp->actor = actor;
    pp->pnum = pnum;

    pp->InitAngles();

    actor->spr.cstat |= (CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    actor->spr.extra |= (SPRX_PLAYER_OR_ENEMY);
    actor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);

    // Grouping items that need to be reset after a LoadLevel
    actor->user.__legacyState.ActorActionSet = &PlayerNinjaActionSet;

    actor->user.Radius = 400;
    actor->user.PlayerP = pp;
    //actor->user.Health = pp->MaxHealth;
    actor->user.Flags |= (SPR_XFLIP_TOGGLE);


    actor->spr.shade = -60; // was 15
    actor->clipdist = 16;

    actor->spr.scale = DVector2(PLAYER_NINJA_XREPEAT, PLAYER_NINJA_YREPEAT);
    actor->spr.pal = PALETTE_PLAYER0 + pp->pnum;
    actor->user.spal = actor->spr.pal;

    pp->GetActor()->setStateGroup(NAME_Run);
    actor->setPicFromState();

    pp->PlayerUnderActor = nullptr;

    DoPlayerZrange(pp);

    if (NewGame)
    {
        PlayerGameReset(pp);
    }
    else
    {
        // restore stuff from last level
        puser[pnum].CopyToUser(actor);
        PlayerLevelReset(pp);
    }

    memset(pp->InventoryTics,0,sizeof(pp->InventoryTics));

    if (pp == getPlayer(screenpeek))
    {
        videoFadePalette(0,0,0,0);
    }

    pp->NightVision = false;
    pp->StartColor = 0;
    pp->FadeAmt = 0;
    pp->DeathType = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SpawnPlayerUnderSprite(DSWPlayer* pp)
{
    DSWActor* plActor = pp->GetActor();

    int pnum = int(pp->pnum);

    pp->PlayerUnderActor = SpawnActor(STAT_PLAYER_UNDER0 + pnum,
                                                 NINJA_RUN_R0, nullptr, pp->cursector, pp->GetActor()->getPosWithOffsetZ(), pp->GetActor()->spr.Angles.Yaw);

    DSWActor* actor = pp->PlayerUnderActor;

    actor->spr.cstat |= (CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    actor->spr.extra |= (SPRX_PLAYER_OR_ENEMY);

    actor->user.State = plActor->user.State;
    NewStateGroup(pp->PlayerUnderActor, plActor->user.__legacyState.Rot);

    actor->user.Radius = plActor->user.Radius;
    actor->user.PlayerP = pp;
    actor->user.Health = pp->MaxHealth;
    actor->user.Flags |= (SPR_XFLIP_TOGGLE);

    actor->user.__legacyState.ActorActionSet = plActor->user.__legacyState.ActorActionSet;

    actor->spr.picnum = plActor->spr.picnum;
    actor->copy_clipdist(plActor);
    actor->spr.scale = plActor->spr.scale;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------


#include "saveable.h"

static saveable_data saveable_ninja_data[] =
{
    SAVE_DATA(NinjaPersonality),
    SAVE_DATA(NinjaSniperPersonality),

    SAVE_DATA(NinjaAttrib),
    SAVE_DATA(InvisibleNinjaAttrib),

    SAVE_DATA(s_NinjaRun),
    SAVE_DATA(s_NinjaStand),
    SAVE_DATA(s_NinjaRise),
    SAVE_DATA(s_NinjaCrawl),
    SAVE_DATA(s_NinjaKneelCrawl),
    SAVE_DATA(s_NinjaDuck),
    SAVE_DATA(s_NinjaSit),
    SAVE_DATA(s_NinjaCeiling),
    SAVE_DATA(s_NinjaJump),
    SAVE_DATA(s_NinjaFall),
    SAVE_DATA(s_NinjaSwim),
    SAVE_DATA(s_NinjaDive),
    SAVE_DATA(s_NinjaClimb),
    SAVE_DATA(s_NinjaFly),
    SAVE_DATA(s_NinjaPain),
    SAVE_DATA(s_NinjaStar),
    SAVE_DATA(s_NinjaMirv),
    SAVE_DATA(s_NinjaNapalm),
    SAVE_DATA(s_NinjaRocket),
    SAVE_DATA(s_NinjaGrenade),
    SAVE_DATA(s_NinjaFlashBomb),
    SAVE_DATA(s_NinjaUzi),
    SAVE_DATA(s_NinjaHariKari),
    SAVE_DATA(s_NinjaGrabThroat),
    SAVE_DATA(s_NinjaDie),
    SAVE_DATA(s_NinjaDieSliced),
    SAVE_DATA(s_NinjaDieSlicedHack),
    SAVE_DATA(s_NinjaDead),
    SAVE_DATA(s_NinjaDeathJump),
    SAVE_DATA(s_NinjaDeathFall),

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
    nullptr, 0,

    // data
    saveable_ninja_data,
    SIZ(saveable_ninja_data)
};

END_SW_NS
