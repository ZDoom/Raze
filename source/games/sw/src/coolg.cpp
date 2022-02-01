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
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "sprite.h"
#include "weapon.h"
#include "misc.h"

BEGIN_SW_NS

ANIMATOR DoCoolgCircle,InitCoolgCircle;

enum { COOLG_BOB_AMT = (Z(8)) };

DECISION CoolgBattle[] =
{
    {50,    InitCoolgCircle             },
    {450,   InitActorMoveCloser         },
    //{456,   InitActorAmbientNoise        },
    //{760,   InitActorRunAway            },
    {1024,  InitActorAttack             }
};

DECISION CoolgOffense[] =
{
    {449,   InitActorMoveCloser         },
    //{554,   InitActorAmbientNoise       },
    {1024,  InitActorAttack             }
};

DECISION CoolgBroadcast[] =
{
    //{1,    InitActorAlertNoise         },
    {1,    InitActorAmbientNoise       },
    {1024, InitActorDecide             }
};

DECISION CoolgSurprised[] =
{
    {100,   InitCoolgCircle            },
    {701,   InitActorMoveCloser        },
    {1024,  InitActorDecide            }
};

DECISION CoolgEvasive[] =
{
    {20,     InitCoolgCircle           },
    {1024,   InitActorRunAway          },
};

DECISION CoolgLostTarget[] =
{
    {900,   InitActorFindPlayer         },
    {1024,  InitActorWanderAround       }
};

DECISION CoolgCloseRange[] =
{
    {800,   InitActorAttack             },
    {1024,  InitActorReposition         }
};

DECISION CoolgTouchTarget[] =
{
    //{50,   InitCoolgCircle            },
    {1024,  InitActorAttack            },
};

PERSONALITY CoolgPersonality =
{
    CoolgBattle,
    CoolgOffense,
    CoolgBroadcast,
    CoolgSurprised,
    CoolgEvasive,
    CoolgLostTarget,
    CoolgCloseRange,
    CoolgTouchTarget
};

ATTRIBUTE CoolgAttrib =
{
    {60, 80, 150, 190},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        DIGI_CGAMBIENT, DIGI_CGALERT, 0,
        DIGI_CGPAIN, DIGI_CGSCREAM, DIGI_CGMATERIALIZE,
        DIGI_CGTHIGHBONE,DIGI_CGMAGIC,DIGI_CGMAGICHIT,0
    }
};

//////////////////////
//
// COOLG RUN
//////////////////////

#define COOLG_RUN_RATE 40

ANIMATOR DoCoolgMove,DoStayOnFloor, DoActorDebris, NullCoolg, DoCoolgBirth;

STATE s_CoolgRun[5][4] =
{
    {
        {COOLG_RUN_R0 + 0, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[0][1]},
        {COOLG_RUN_R0 + 1, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[0][2]},
        {COOLG_RUN_R0 + 2, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[0][3]},
        {COOLG_RUN_R0 + 3, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[0][0]},
    },
    {
        {COOLG_RUN_R1 + 0, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[1][1]},
        {COOLG_RUN_R1 + 1, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[1][2]},
        {COOLG_RUN_R1 + 2, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[1][3]},
        {COOLG_RUN_R1 + 3, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[1][0]},
    },
    {
        {COOLG_RUN_R2 + 0, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[2][1]},
        {COOLG_RUN_R2 + 1, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[2][2]},
        {COOLG_RUN_R2 + 2, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[2][3]},
        {COOLG_RUN_R2 + 3, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[2][0]},
    },
    {
        {COOLG_RUN_R3 + 0, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[3][1]},
        {COOLG_RUN_R3 + 1, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[3][2]},
        {COOLG_RUN_R3 + 2, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[3][3]},
        {COOLG_RUN_R3 + 3, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[3][0]},
    },
    {
        {COOLG_RUN_R4 + 0, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[4][1]},
        {COOLG_RUN_R4 + 1, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[4][2]},
        {COOLG_RUN_R4 + 2, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[4][3]},
        {COOLG_RUN_R4 + 3, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgRun[4][0]},
    }
};

STATE* sg_CoolgRun[] =
{
    &s_CoolgRun[0][0],
    &s_CoolgRun[1][0],
    &s_CoolgRun[2][0],
    &s_CoolgRun[3][0],
    &s_CoolgRun[4][0]
};

//////////////////////
//
// COOLG STAND
//
//////////////////////


STATE s_CoolgStand[5][1] =
{
    {
        {COOLG_RUN_R0 + 0, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgStand[0][0]},
    },
    {
        {COOLG_RUN_R1 + 0, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgStand[1][0]},
    },
    {
        {COOLG_RUN_R2 + 0, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgStand[2][0]},
    },
    {
        {COOLG_RUN_R3 + 0, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgStand[3][0]},
    },
    {
        {COOLG_RUN_R4 + 0, COOLG_RUN_RATE, DoCoolgMove, &s_CoolgStand[4][0]},
    }
};

STATE* sg_CoolgStand[] =
{
    &s_CoolgStand[0][0],
    &s_CoolgStand[1][0],
    &s_CoolgStand[2][0],
    &s_CoolgStand[3][0],
    &s_CoolgStand[4][0]
};

//////////////////////
//
// COOLG CLUB
//
//////////////////////

#define COOLG_RATE 16
ANIMATOR InitCoolgBash;

STATE s_CoolgClub[5][6] =
{
    {
        {COOLG_CLUB_R0 + 0, COOLG_RATE, NullCoolg, &s_CoolgClub[0][1]},
        {COOLG_RUN_R0  + 0, COOLG_RATE, NullCoolg, &s_CoolgClub[0][2]},
        {COOLG_CLUB_R0 + 1, 0|SF_QUICK_CALL, InitCoolgBash, &s_CoolgClub[0][3]},
        {COOLG_CLUB_R0 + 1, COOLG_RATE, NullCoolg, &s_CoolgClub[0][4]},
        {COOLG_CLUB_R0 + 1, 0|SF_QUICK_CALL, InitActorDecide, &s_CoolgClub[0][5]},
        {COOLG_CLUB_R0 + 1, COOLG_RATE, DoCoolgMove, &s_CoolgClub[0][5]}
    },
    {
        {COOLG_CLUB_R1 + 0, COOLG_RATE, NullCoolg, &s_CoolgClub[1][1]},
        {COOLG_RUN_R1  + 0, COOLG_RATE, NullCoolg, &s_CoolgClub[1][2]},
        {COOLG_CLUB_R1 + 1, 0|SF_QUICK_CALL, InitCoolgBash, &s_CoolgClub[1][3]},
        {COOLG_CLUB_R1 + 1, COOLG_RATE, NullCoolg, &s_CoolgClub[1][4]},
        {COOLG_CLUB_R1 + 1, 0|SF_QUICK_CALL, InitActorDecide, &s_CoolgClub[1][5]},
        {COOLG_CLUB_R1 + 1, COOLG_RATE, DoCoolgMove, &s_CoolgClub[1][5]}
    },
    {
        {COOLG_CLUB_R2 + 0, COOLG_RATE, NullCoolg, &s_CoolgClub[2][1]},
        {COOLG_RUN_R2  + 0, COOLG_RATE, NullCoolg, &s_CoolgClub[2][2]},
        {COOLG_CLUB_R2 + 1, 0|SF_QUICK_CALL, InitCoolgBash, &s_CoolgClub[2][3]},
        {COOLG_CLUB_R2 + 1, COOLG_RATE, NullCoolg, &s_CoolgClub[2][4]},
        {COOLG_CLUB_R2 + 1, 0|SF_QUICK_CALL, InitActorDecide, &s_CoolgClub[2][5]},
        {COOLG_CLUB_R2 + 1, COOLG_RATE, DoCoolgMove, &s_CoolgClub[2][5]}
    },
    {
        {COOLG_CLUB_R3 + 0, COOLG_RATE, NullCoolg, &s_CoolgClub[3][1]},
        {COOLG_RUN_R3  + 0, COOLG_RATE, NullCoolg, &s_CoolgClub[3][2]},
        {COOLG_CLUB_R3 + 1, 0|SF_QUICK_CALL, InitCoolgBash, &s_CoolgClub[3][3]},
        {COOLG_CLUB_R3 + 1, COOLG_RATE, NullCoolg, &s_CoolgClub[3][4]},
        {COOLG_CLUB_R3 + 1, 0|SF_QUICK_CALL, InitActorDecide, &s_CoolgClub[3][5]},
        {COOLG_CLUB_R3 + 1, COOLG_RATE, DoCoolgMove, &s_CoolgClub[3][5]}
    },
    {
        {COOLG_CLUB_R4 + 0, COOLG_RATE, NullCoolg, &s_CoolgClub[4][1]},
        {COOLG_RUN_R4  + 0, COOLG_RATE, NullCoolg, &s_CoolgClub[4][2]},
        {COOLG_CLUB_R4 + 1, 0|SF_QUICK_CALL, InitCoolgBash, &s_CoolgClub[4][3]},
        {COOLG_CLUB_R4 + 1, COOLG_RATE, NullCoolg, &s_CoolgClub[4][4]},
        {COOLG_CLUB_R4 + 1, 0|SF_QUICK_CALL, InitActorDecide, &s_CoolgClub[4][5]},
        {COOLG_CLUB_R4 + 1, COOLG_RATE, DoCoolgMove, &s_CoolgClub[4][5]}
    }
};

STATE* sg_CoolgClub[] =
{
    &s_CoolgClub[0][0],
    &s_CoolgClub[1][0],
    &s_CoolgClub[2][0],
    &s_CoolgClub[3][0],
    &s_CoolgClub[4][0]
};

//////////////////////
//
// COOLG FIRE
//
//////////////////////

ANIMATOR InitCoolgFire;
#define COOLG_FIRE_RATE 12

STATE s_CoolgAttack[5][7] =
{
    {
        {COOLG_FIRE_R0 + 0, COOLG_FIRE_RATE*2,  NullCoolg,              &s_CoolgAttack[0][1]},
        {COOLG_FIRE_R0 + 1, COOLG_FIRE_RATE*2,  NullCoolg,              &s_CoolgAttack[0][2]},
        {COOLG_FIRE_R0 + 2, COOLG_FIRE_RATE*2,  NullCoolg,              &s_CoolgAttack[0][3]},
        {COOLG_FIRE_R0 + 2, 0|SF_QUICK_CALL,    InitCoolgFire,          &s_CoolgAttack[0][4]},
        {COOLG_FIRE_R0 + 2, COOLG_FIRE_RATE,    NullCoolg,              &s_CoolgAttack[0][5]},
        {COOLG_FIRE_R0 + 2, 0|SF_QUICK_CALL,    InitActorDecide,        &s_CoolgAttack[0][6]},
        {COOLG_RUN_R0  + 2, COOLG_FIRE_RATE,    DoCoolgMove,            &s_CoolgAttack[0][6]}
    },
    {
        {COOLG_FIRE_R1 + 0, COOLG_FIRE_RATE*2,  NullCoolg,              &s_CoolgAttack[1][1]},
        {COOLG_FIRE_R1 + 1, COOLG_FIRE_RATE*2,  NullCoolg,              &s_CoolgAttack[1][2]},
        {COOLG_FIRE_R1 + 2, COOLG_FIRE_RATE*2,  NullCoolg,              &s_CoolgAttack[1][3]},
        {COOLG_FIRE_R1 + 2, 0|SF_QUICK_CALL,    InitCoolgFire,          &s_CoolgAttack[1][4]},
        {COOLG_FIRE_R1 + 2, COOLG_FIRE_RATE,    NullCoolg,              &s_CoolgAttack[1][5]},
        {COOLG_FIRE_R1 + 2, 0|SF_QUICK_CALL,    InitActorDecide,        &s_CoolgAttack[1][6]},
        {COOLG_RUN_R0  + 2, COOLG_FIRE_RATE,    DoCoolgMove,            &s_CoolgAttack[1][6]}
    },
    {
        {COOLG_FIRE_R2 + 0, COOLG_FIRE_RATE*2,  NullCoolg,              &s_CoolgAttack[2][1]},
        {COOLG_FIRE_R2 + 1, COOLG_FIRE_RATE*2,  NullCoolg,              &s_CoolgAttack[2][2]},
        {COOLG_FIRE_R2 + 2, COOLG_FIRE_RATE*2,  NullCoolg,              &s_CoolgAttack[2][3]},
        {COOLG_FIRE_R2 + 2, 0|SF_QUICK_CALL,    InitCoolgFire,          &s_CoolgAttack[2][4]},
        {COOLG_FIRE_R2 + 2, COOLG_FIRE_RATE,    NullCoolg,              &s_CoolgAttack[2][5]},
        {COOLG_FIRE_R2 + 2, 0|SF_QUICK_CALL,    InitActorDecide,        &s_CoolgAttack[2][6]},
        {COOLG_RUN_R0  + 2, COOLG_FIRE_RATE,    DoCoolgMove,            &s_CoolgAttack[2][6]}
    },
    {
        {COOLG_RUN_R3 + 0, COOLG_FIRE_RATE*2,  NullCoolg,               &s_CoolgAttack[3][1]},
        {COOLG_RUN_R3 + 1, COOLG_FIRE_RATE*2,  NullCoolg,               &s_CoolgAttack[3][2]},
        {COOLG_RUN_R3 + 2, COOLG_FIRE_RATE*2,  NullCoolg,               &s_CoolgAttack[3][3]},
        {COOLG_RUN_R3 + 2, 0|SF_QUICK_CALL,    InitCoolgFire,           &s_CoolgAttack[3][4]},
        {COOLG_RUN_R3 + 2, COOLG_FIRE_RATE,    NullCoolg,               &s_CoolgAttack[3][5]},
        {COOLG_RUN_R3 + 2, 0|SF_QUICK_CALL,    InitActorDecide,         &s_CoolgAttack[3][6]},
        {COOLG_RUN_R0 + 2, COOLG_FIRE_RATE,    DoCoolgMove,            &s_CoolgAttack[3][6]}
    },
    {
        {COOLG_RUN_R4 + 0, COOLG_FIRE_RATE*2,  NullCoolg,               &s_CoolgAttack[4][1]},
        {COOLG_RUN_R4 + 1, COOLG_FIRE_RATE*2,  NullCoolg,               &s_CoolgAttack[4][2]},
        {COOLG_RUN_R4 + 2, COOLG_FIRE_RATE*2,  NullCoolg,               &s_CoolgAttack[4][3]},
        {COOLG_RUN_R4 + 2, 0|SF_QUICK_CALL,    InitCoolgFire,           &s_CoolgAttack[4][4]},
        {COOLG_RUN_R4 + 2, COOLG_FIRE_RATE,    NullCoolg,               &s_CoolgAttack[4][5]},
        {COOLG_RUN_R4 + 2, 0|SF_QUICK_CALL,    InitActorDecide,         &s_CoolgAttack[4][6]},
        {COOLG_RUN_R0 + 2, COOLG_FIRE_RATE,    DoCoolgMove,            &s_CoolgAttack[4][6]}
    }
};

STATE* sg_CoolgAttack[] =
{
    &s_CoolgAttack[0][0],
    &s_CoolgAttack[1][0],
    &s_CoolgAttack[2][0],
    &s_CoolgAttack[3][0],
    &s_CoolgAttack[4][0]
};

//////////////////////
//
// COOLG PAIN
//
//////////////////////

#define COOLG_PAIN_RATE 15
ANIMATOR DoCoolgPain;

STATE s_CoolgPain[5][2] =
{
    {
        {COOLG_PAIN_R0 + 0, COOLG_PAIN_RATE, DoCoolgPain, &s_CoolgPain[0][1]},
        {COOLG_PAIN_R0 + 0, COOLG_PAIN_RATE, DoCoolgPain, &s_CoolgPain[0][1]},
    },
    {
        {COOLG_RUN_R1 + 0, COOLG_PAIN_RATE, DoCoolgPain, &s_CoolgPain[1][1]},
        {COOLG_RUN_R1 + 0, COOLG_PAIN_RATE, DoCoolgPain, &s_CoolgPain[1][1]},
    },
    {
        {COOLG_RUN_R2 + 0, COOLG_PAIN_RATE, DoCoolgPain, &s_CoolgPain[2][1]},
        {COOLG_RUN_R2 + 0, COOLG_PAIN_RATE, DoCoolgPain, &s_CoolgPain[2][1]},
    },
    {
        {COOLG_RUN_R3 + 0, COOLG_PAIN_RATE, DoCoolgPain, &s_CoolgPain[3][1]},
        {COOLG_RUN_R3 + 0, COOLG_PAIN_RATE, DoCoolgPain, &s_CoolgPain[3][1]},
    },
    {
        {COOLG_RUN_R4 + 0, COOLG_PAIN_RATE, DoCoolgPain, &s_CoolgPain[4][1]},
        {COOLG_RUN_R4 + 0, COOLG_PAIN_RATE, DoCoolgPain, &s_CoolgPain[4][1]},
    },
};

STATE* sg_CoolgPain[] =
{
    s_CoolgPain[0],
    s_CoolgPain[1],
    s_CoolgPain[2],
    s_CoolgPain[3],
    s_CoolgPain[4]
};


//////////////////////
//
// COOLG DIE
//
//////////////////////

#define COOLG_DIE_RATE 20

#define COOLG_DIE 4307
#define COOLG_DEAD 4307+5
ANIMATOR DoCoolgDeath;
STATE s_CoolgDie[] =
{
    {COOLG_DIE +    0, COOLG_DIE_RATE, DoCoolgDeath, &s_CoolgDie[1]},
    {COOLG_DIE +    1, COOLG_DIE_RATE, DoCoolgDeath, &s_CoolgDie[2]},
    {COOLG_DIE +    2, COOLG_DIE_RATE, DoCoolgDeath, &s_CoolgDie[3]},
    {COOLG_DIE +    3, COOLG_DIE_RATE, DoCoolgDeath, &s_CoolgDie[4]},
    {COOLG_DIE +    4, COOLG_DIE_RATE, DoCoolgDeath, &s_CoolgDie[5]},
    {COOLG_DIE +    5, COOLG_DIE_RATE, DoCoolgDeath, &s_CoolgDie[5]},
};

STATE* sg_CoolgDie[] =
{
    s_CoolgDie
};

STATE s_CoolgDead[] =
{
    {COOLG_DEAD, SF_QUICK_CALL, QueueFloorBlood, &s_CoolgDead[1]},
    {COOLG_DEAD, COOLG_DIE_RATE, DoActorDebris, &s_CoolgDead[1]},
};

STATE* sg_CoolgDead[] =
{
    s_CoolgDead
};

//////////////////////
//
// COOLG BIRTH
//
//////////////////////

#define COOLG_BIRTH_RATE 20
#define COOLG_BIRTH 4268

STATE s_CoolgBirth[] =
{
    {COOLG_BIRTH + 0, COOLG_BIRTH_RATE, NullAnimator, &s_CoolgBirth[1]},
    {COOLG_BIRTH + 1, COOLG_BIRTH_RATE, NullAnimator, &s_CoolgBirth[2]},
    {COOLG_BIRTH + 2, COOLG_BIRTH_RATE, NullAnimator, &s_CoolgBirth[3]},
    {COOLG_BIRTH + 3, COOLG_BIRTH_RATE, NullAnimator, &s_CoolgBirth[4]},
    {COOLG_BIRTH + 4, COOLG_BIRTH_RATE, NullAnimator, &s_CoolgBirth[5]},
    {COOLG_BIRTH + 5, COOLG_BIRTH_RATE, NullAnimator, &s_CoolgBirth[6]},
    {COOLG_BIRTH + 6, COOLG_BIRTH_RATE, NullAnimator, &s_CoolgBirth[7]},
    {COOLG_BIRTH + 7, COOLG_BIRTH_RATE, NullAnimator, &s_CoolgBirth[8]},
    {COOLG_BIRTH + 8, COOLG_BIRTH_RATE, NullAnimator, &s_CoolgBirth[9]},
    {COOLG_BIRTH + 8, COOLG_BIRTH_RATE, NullAnimator, &s_CoolgBirth[10]},
    {COOLG_BIRTH + 8, 0|SF_QUICK_CALL, DoCoolgBirth, &s_CoolgBirth[10]}
};

STATE* sg_CoolgBirth[] =
{
    s_CoolgBirth
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


ACTOR_ACTION_SET CoolgActionSet =
{
    sg_CoolgStand,
    sg_CoolgRun,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr, //climb
    sg_CoolgPain, //pain
    sg_CoolgDie,
    nullptr,
    sg_CoolgDead,
    nullptr,
    nullptr,
//  {sg_CoolgClub},
    {sg_CoolgAttack},
    {1024},
    {sg_CoolgAttack},
    {1024},
    {nullptr,nullptr},
    nullptr,
    nullptr
};

int DoCoolgMatchPlayerZ(DSWActor* actor);

void CoolgCommon(DSWActor* actor)
{
    actor->spr.clipdist = (200) >> 2;
    actor->user.floor_dist = Z(16);
    actor->user.ceiling_dist = Z(20);

    actor->user.pos.Z = actor->int_pos().Z;

    actor->spr.xrepeat = 42;
    actor->spr.yrepeat = 42;
    actor->spr.extra |= (SPRX_PLAYER_OR_ENEMY);
}

int SetupCoolg(DSWActor* actor)
{
    ANIMATOR DoActorDecide;

    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,COOLG_RUN_R0,s_CoolgRun[0]);
        actor->user.Health = HEALTH_COOLIE_GHOST;
    }

    ChangeState(actor, s_CoolgRun[0]);
    actor->user.Attrib = &CoolgAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.StateEnd = s_CoolgDie;
    actor->user.Rot = sg_CoolgRun;

    EnemyDefaults(actor, &CoolgActionSet, &CoolgPersonality);

    actor->user.Flags |= (SPR_NO_SCAREDZ|SPR_XFLIP_TOGGLE);

    CoolgCommon(actor);

    return 0;
}

int NewCoolg(DSWActor* actor)
{
    ANIMATOR DoActorDecide;

    auto actorNew = SpawnActor(STAT_ENEMY, COOLG_RUN_R0, &s_CoolgBirth[0], actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->spr.ang, 50);

    ChangeState(actorNew, &s_CoolgBirth[0]);
    actorNew->user.StateEnd = s_CoolgDie;
    actorNew->user.Rot = sg_CoolgRun;
    actorNew->spr.pal = actorNew->user.spal = actor->user.spal;

    actorNew->user.ActorActionSet = &CoolgActionSet;

    actorNew->spr.shade = actor->spr.shade;
    actorNew->user.Personality = &CoolgPersonality;
    actorNew->user.Attrib = &CoolgAttrib;

    // special case
    TotalKillable++;
    CoolgCommon(actorNew);

    return 0;
}


int DoCoolgBirth(DSWActor* actor)
{
    ANIMATOR DoActorDecide;

    actor->user.Health = HEALTH_COOLIE_GHOST;
    actor->user.Attrib = &CoolgAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);

    ChangeState(actor, s_CoolgRun[0]);
    actor->user.StateEnd = s_CoolgDie;
    actor->user.Rot = sg_CoolgRun;

    EnemyDefaults(actor, &CoolgActionSet, &CoolgPersonality);
    // special case
    TotalKillable--;

    actor->user.Flags |= (SPR_NO_SCAREDZ|SPR_XFLIP_TOGGLE);
    CoolgCommon(actor);

    return 0;
}

int NullCoolg(DSWActor* actor)
{
    actor->user.ShellNum -= ACTORMOVETICS;

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    DoCoolgMatchPlayerZ(actor);
    DoActorSectorDamage(actor);
    return 0;
}


int DoCoolgMatchPlayerZ(DSWActor* actor)
{
    int zdiff,zdist;
    int loz,hiz;

    int bound;

    // If blocking bits get unset, just die
    if (!(actor->spr.cstat & CSTAT_SPRITE_BLOCK) || !(actor->spr.cstat & CSTAT_SPRITE_BLOCK_HITSCAN))
    {
        InitBloodSpray(actor, true, 105);
        InitBloodSpray(actor, true, 105);
        UpdateSinglePlayKills(actor);
        SetSuicide(actor);
    }

    // actor does a sine wave about sz - this is the z mid point

    zdiff = (ActorZOfMiddle(actor->user.targetActor)) - actor->user.pos.Z;

    // check z diff of the player and the sprite
    zdist = Z(20 + RandomRange(100)); // put a random amount
    //zdist = Z(20);
    if (labs(zdiff) > zdist)
    {
        if (zdiff > 0)
            actor->user.pos.Z += 170 * ACTORMOVETICS;
        else
            actor->user.pos.Z -= 170 * ACTORMOVETICS;
    }

    // save off lo and hi z
    loz = actor->user.loz;
    hiz = actor->user.hiz;

    // adjust loz/hiz for water depth
    if (actor->user.lo_sectp && actor->user.lo_sectp->hasU() && FixedToInt(actor->user.lo_sectp->depth_fixed))
        loz -= Z(FixedToInt(actor->user.lo_sectp->depth_fixed)) - Z(8);

    // lower bound
    if (actor->user.lowActor)
        bound = loz - actor->user.floor_dist;
    else
        bound = loz - actor->user.floor_dist - COOLG_BOB_AMT;

    if (actor->user.pos.Z > bound)
    {
        actor->user.pos.Z = bound;
    }

    // upper bound
    if (actor->user.highActor)
        bound = hiz + actor->user.ceiling_dist;
    else
        bound = hiz + actor->user.ceiling_dist + COOLG_BOB_AMT;

    if (actor->user.pos.Z < bound)
    {
        actor->user.pos.Z = bound;
    }

    actor->user.pos.Z = min(actor->user.pos.Z, loz - actor->user.floor_dist);
    actor->user.pos.Z = max(actor->user.pos.Z, hiz + actor->user.ceiling_dist);

    actor->user.Counter = (actor->user.Counter + (ACTORMOVETICS<<3)) & 2047;
    actor->set_int_z(actor->user.pos.Z + MulScale(COOLG_BOB_AMT, bsin(actor->user.Counter), 14));

    bound = actor->user.hiz + actor->user.ceiling_dist + COOLG_BOB_AMT;
    if (actor->int_pos().Z < bound)
    {
        // bumped something
        actor->set_int_z(actor->user.pos.Z = bound + COOLG_BOB_AMT);
    }

    return 0;
}

int InitCoolgCircle(DSWActor* actor)
{
    actor->user.ActorActionFunc = DoCoolgCircle;

    NewStateGroup(actor, actor->user.ActorActionSet->Run);

    // set it close
    DoActorSetSpeed(actor, FAST_SPEED);

    // set to really fast
    actor->spr.xvel = 400;
    // angle adjuster
    actor->user.Counter2 = actor->spr.xvel/3;
    // random angle direction
    if (RANDOM_P2(1024) < 512)
        actor->user.Counter2 = -actor->user.Counter2;

    // z velocity
    actor->user.jump_speed = 400 + RANDOM_P2(256);
    if (labs(actor->user.pos.Z - actor->user.hiz) < labs(actor->user.pos.Z - actor->user.loz))
        actor->user.jump_speed = -actor->user.jump_speed;

    actor->user.WaitTics = (RandomRange(3)+1) * 120;

    (*actor->user.ActorActionFunc)(actor);

    return 0;
}

int DoCoolgCircle(DSWActor* actor)
{
    int nx,ny,bound;

    actor->spr.ang = NORM_ANGLE(actor->spr.ang + actor->user.Counter2);

    nx = MulScale(actor->spr.xvel, bcos(actor->spr.ang), 14);
    ny = MulScale(actor->spr.xvel, bsin(actor->spr.ang), 14);

    if (!move_actor(actor, nx, ny, 0L))
    {
        InitActorReposition(actor);
        return 0;
    }

    // move in the z direction
    actor->user.pos.Z -= actor->user.jump_speed * ACTORMOVETICS;

    bound = actor->user.hiz + actor->user.ceiling_dist + COOLG_BOB_AMT;
    if (actor->user.pos.Z < bound)
    {
        // bumped something
        actor->user.pos.Z = bound;
        InitActorReposition(actor);
        return 0;
    }

    // time out
    if ((actor->user.WaitTics -= ACTORMOVETICS) < 0)
    {
        InitActorReposition(actor);
        actor->user.WaitTics = 0;
        return 0;
    }

    return 0;
}


int DoCoolgDeath(DSWActor* actor)
{
    int nx, ny;

    actor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);
    actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE);
    actor->spr.xrepeat = 42;
    actor->spr.shade = -10;

    if (actor->user.Flags & (SPR_FALLING))
    {
        DoFall(actor);
    }
    else
    {
        DoFindGroundPoint(actor);
        actor->user.floor_dist = 0;
        DoBeginFall(actor);
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    // slide while falling
    nx = MulScale(actor->spr.xvel, bcos(actor->spr.ang), 14);
    ny = MulScale(actor->spr.xvel, bsin(actor->spr.ang), 14);

    actor->user.coll = move_sprite(actor, nx, ny, 0L, actor->user.ceiling_dist, actor->user.floor_dist, CLIPMASK_MISSILE, ACTORMOVETICS);
    DoFindGroundPoint(actor);

    // on the ground
    if (actor->int_pos().Z >= actor->user.loz)
    {
        actor->user.Flags &= ~(SPR_FALLING|SPR_SLIDING);
        actor->spr.cstat &= ~(CSTAT_SPRITE_YFLIP); // If upside down, reset it
        NewStateGroup(actor, actor->user.ActorActionSet->Dead);
        return 0;
    }

    return 0;
}


int DoCoolgMove(DSWActor* actor)
{
    if ((actor->user.ShellNum -= ACTORMOVETICS) <= 0)
    {
        switch (actor->user.FlagOwner)
        {
        case 0:
            actor->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT);
            actor->user.ShellNum = SEC(2);
            break;
        case 1:
            PlaySound(DIGI_VOID3, actor, v3df_follow);
            actor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);
            actor->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);
            actor->user.ShellNum = SEC(1) + SEC(RandomRange(2));
            break;
        case 2:
            actor->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT);
            actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE);
            actor->user.ShellNum = SEC(2);
            break;
        case 3:
            PlaySound(DIGI_VOID3, actor, v3df_follow);
            actor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);
            actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE);
            actor->user.ShellNum = SEC(2) + SEC(RandomRange(3));
            break;
        default:
            actor->user.FlagOwner = 0;
            break;
        }
        actor->user.FlagOwner++;
        if (actor->user.FlagOwner > 3) actor->user.FlagOwner = 0;
    }

    if (actor->user.FlagOwner-1 == 0)
    {
        actor->spr.xrepeat--;
        actor->spr.shade++;
        if (actor->spr.xrepeat < 4) actor->spr.xrepeat = 4;
        if (actor->spr.shade > 126)
        {
            actor->spr.shade = 127;
            actor->spr.hitag = 9998;
        }
    }
    else if (actor->user.FlagOwner-1 == 2)
    {
        actor->spr.hitag = 0;
        actor->spr.xrepeat++;
        actor->spr.shade--;
        if (actor->spr.xrepeat > 42) actor->spr.xrepeat = 42;
        if (actor->spr.shade < -10) actor->spr.shade = -10;
    }
    else if (actor->user.FlagOwner == 0)
    {
        actor->spr.xrepeat = 42;
        actor->spr.shade = -10;
        actor->spr.hitag = 0;
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
    {
        (*actor->user.ActorActionFunc)(actor);
    }

    if (RANDOM_P2(1024) < 32 && !(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
        InitCoolgDrip(actor);

    DoCoolgMatchPlayerZ(actor);

    DoActorSectorDamage(actor);


    return 0;

}

int DoCoolgPain(DSWActor* actor)
{
    NullCoolg(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(actor);

    return 0;
}


#include "saveable.h"

static saveable_code saveable_coolg_code[] =
{
    SAVE_CODE(DoCoolgBirth),
    SAVE_CODE(NullCoolg),
    SAVE_CODE(InitCoolgCircle),
    SAVE_CODE(DoCoolgCircle),
    SAVE_CODE(DoCoolgDeath),
    SAVE_CODE(DoCoolgMove),
    SAVE_CODE(DoCoolgPain),
};

static saveable_data saveable_coolg_data[] =
{
    SAVE_DATA(CoolgBattle),
    SAVE_DATA(CoolgOffense),
    SAVE_DATA(CoolgBroadcast),
    SAVE_DATA(CoolgSurprised),
    SAVE_DATA(CoolgEvasive),
    SAVE_DATA(CoolgLostTarget),
    SAVE_DATA(CoolgCloseRange),
    SAVE_DATA(CoolgTouchTarget),

    SAVE_DATA(CoolgPersonality),

    SAVE_DATA(CoolgAttrib),

    SAVE_DATA(s_CoolgRun),
    SAVE_DATA(sg_CoolgRun),
    SAVE_DATA(s_CoolgStand),
    SAVE_DATA(sg_CoolgStand),
    SAVE_DATA(s_CoolgClub),
    SAVE_DATA(sg_CoolgClub),
    SAVE_DATA(s_CoolgAttack),
    SAVE_DATA(sg_CoolgAttack),
    SAVE_DATA(s_CoolgPain),
    SAVE_DATA(sg_CoolgPain),
    SAVE_DATA(s_CoolgDie),
    SAVE_DATA(sg_CoolgDie),
    SAVE_DATA(s_CoolgDead),
    SAVE_DATA(sg_CoolgDead),
    SAVE_DATA(s_CoolgBirth),
    SAVE_DATA(sg_CoolgBirth),

    SAVE_DATA(CoolgActionSet),
};

saveable_module saveable_coolg =
{
    // code
    saveable_coolg_code,
    SIZ(saveable_coolg_code),

    // data
    saveable_coolg_data,
    SIZ(saveable_coolg_data)
};
END_SW_NS
