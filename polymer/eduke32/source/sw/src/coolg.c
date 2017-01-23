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
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "sprite.h"
#include "actor.h"
#include "weapon.h"
#include "track.h"

ANIMATOR DoCoolgCircle,InitCoolgCircle;

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

ANIMATOR DoCoolgMove,NullAnimator,DoStayOnFloor, DoActorDebris, NullCoolg, DoCoolgBirth;

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

STATEp sg_CoolgRun[] =
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

STATEp sg_CoolgStand[] =
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

STATEp sg_CoolgClub[] =
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

STATEp sg_CoolgAttack[] =
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

STATEp sg_CoolgPain[] =
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

STATEp sg_CoolgDie[] =
{
    s_CoolgDie
};

STATE s_CoolgDead[] =
{
    {COOLG_DEAD, SF_QUICK_CALL, QueueFloorBlood, &s_CoolgDead[1]},
    {COOLG_DEAD, COOLG_DIE_RATE, DoActorDebris, &s_CoolgDead[1]},
};

STATEp sg_CoolgDead[] =
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

STATEp sg_CoolgBirth[] =
{
    s_CoolgBirth
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


ACTOR_ACTION_SET CoolgActionSet =
{
    sg_CoolgStand,
    sg_CoolgRun,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, //climb
    sg_CoolgPain, //pain
    sg_CoolgDie,
    NULL,
    sg_CoolgDead,
    NULL,
    NULL,
//  {sg_CoolgClub},
    {sg_CoolgAttack},
    {1024},
    {sg_CoolgAttack},
    {1024},
    {NULL,NULL},
    NULL,
    NULL
};

int DoCoolgMatchPlayerZ(short SpriteNum);

void
CoolgCommon(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    sp->clipdist = (200) >> 2;
    //u->floor_dist = Z(5);
    u->floor_dist = Z(16);
    u->ceiling_dist = Z(20);

    u->sz = sp->z;

    sp->xrepeat = 42;
    sp->yrepeat = 42;
    SET(sp->extra, SPRX_PLAYER_OR_ENEMY);
}

int
SetupCoolg(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,COOLG_RUN_R0,s_CoolgRun[0]);
        u->Health = HEALTH_COOLIE_GHOST;
    }

    ChangeState(SpriteNum, s_CoolgRun[0]);
    u->Attrib = &CoolgAttrib;
    DoActorSetSpeed(SpriteNum, NORM_SPEED);
    u->StateEnd = s_CoolgDie;
    u->Rot = sg_CoolgRun;

    EnemyDefaults(SpriteNum, &CoolgActionSet, &CoolgPersonality);

    SET(u->Flags, SPR_NO_SCAREDZ|SPR_XFLIP_TOGGLE);

    CoolgCommon(SpriteNum);

    return 0;
}

extern short TotalKillable;

int
NewCoolg(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    USERp nu;
    SPRITEp np;
    ANIMATOR DoActorDecide;
    short New;

    New = SpawnSprite(STAT_ENEMY, COOLG_RUN_R0, &s_CoolgBirth[0], sp->sectnum, sp->x, sp->y, sp->z, sp->ang, 50);

    nu = User[New];
    np = &sprite[New];

    ChangeState(New, &s_CoolgBirth[0]);
    nu->StateEnd = s_CoolgDie;
    nu->Rot = sg_CoolgRun;
    np->pal = nu->spal = u->spal;

    nu->ActorActionSet = &CoolgActionSet;

    np->shade = sp->shade;
    nu->Personality = &CoolgPersonality;
    nu->Attrib = &CoolgAttrib;

    // special case
    TotalKillable++;
    CoolgCommon(New);

    return 0;
}


int
DoCoolgBirth(short New)
{
    SPRITEp sp;
    USERp u;
    ANIMATOR DoActorDecide;

    u = User[New];
    sp = &sprite[New];

    u->Health = HEALTH_COOLIE_GHOST;
    u->Attrib = &CoolgAttrib;
    DoActorSetSpeed(New, NORM_SPEED);

    ChangeState(New, s_CoolgRun[0]);
    u->StateEnd = s_CoolgDie;
    u->Rot = sg_CoolgRun;

    EnemyDefaults(New, &CoolgActionSet, &CoolgPersonality);
    // special case
    TotalKillable--;

    SET(u->Flags, SPR_NO_SCAREDZ|SPR_XFLIP_TOGGLE);
    CoolgCommon(New);

    return 0;
}

int NullCoolg(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    u->ShellNum -= ACTORMOVETICS;

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    DoCoolgMatchPlayerZ(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    return 0;
}


int DoCoolgMatchPlayerZ(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp tsp = User[SpriteNum]->tgt_sp;
    int zdiff,zdist;
    int loz,hiz;

    int bound;

    // If blocking bits get unset, just die
    if (!TEST(sp->cstat,CSTAT_SPRITE_BLOCK) || !TEST(sp->cstat,CSTAT_SPRITE_BLOCK_HITSCAN))
    {
        InitBloodSpray(SpriteNum, TRUE, 105);
        InitBloodSpray(SpriteNum, TRUE, 105);
        UpdateSinglePlayKills(SpriteNum);
        SetSuicide(SpriteNum);
    }

    // actor does a sine wave about u->sz - this is the z mid point

    zdiff = (SPRITEp_MID(tsp)) - u->sz;

    // check z diff of the player and the sprite
    zdist = Z(20 + RANDOM_RANGE(100)); // put a random amount
    //zdist = Z(20);
    if (labs(zdiff) > zdist)
    {
        if (zdiff > 0)
            u->sz += 170 * ACTORMOVETICS;
        else
            u->sz -= 170 * ACTORMOVETICS;
    }

#define COOLG_BOB_AMT (Z(8))

    // save off lo and hi z
    loz = u->loz;
    hiz = u->hiz;

    // adjust loz/hiz for water depth
    if (u->lo_sectp && SectUser[u->lo_sectp - sector] && SectUser[u->lo_sectp - sector]->depth)
        loz -= Z(SectUser[u->lo_sectp - sector]->depth) - Z(8);

    // lower bound
    if (u->lo_sp)
        bound = loz - u->floor_dist;
    else
        bound = loz - u->floor_dist - COOLG_BOB_AMT;

    if (u->sz > bound)
    {
        u->sz = bound;
    }

    // upper bound
    if (u->hi_sp)
        bound = hiz + u->ceiling_dist;
    else
        bound = hiz + u->ceiling_dist + COOLG_BOB_AMT;

    if (u->sz < bound)
    {
        u->sz = bound;
    }

    u->sz = min(u->sz, loz - u->floor_dist);
    u->sz = max(u->sz, hiz + u->ceiling_dist);

    u->Counter = (u->Counter + (ACTORMOVETICS<<3)) & 2047;
    sp->z = u->sz + ((COOLG_BOB_AMT * (int)sintable[u->Counter]) >> 14);

    bound = u->hiz + u->ceiling_dist + COOLG_BOB_AMT;
    if (sp->z < bound)
    {
        // bumped something
        sp->z = u->sz = bound + COOLG_BOB_AMT;
    }

    return 0;
}

int InitCoolgCircle(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];


    u->ActorActionFunc = DoCoolgCircle;

    NewStateGroup(SpriteNum, u->ActorActionSet->Run);

    // set it close
    DoActorSetSpeed(SpriteNum, FAST_SPEED);

    // set to really fast
    sp->xvel = 400;
    // angle adjuster
    u->Counter2 = sp->xvel/3;
    // random angle direction
    if (RANDOM_P2(1024) < 512)
        u->Counter2 = -u->Counter2;

    // z velocity
    u->jump_speed = 400 + RANDOM_P2(256);
    if (labs(u->sz - u->hiz) < labs(u->sz - u->loz))
        u->jump_speed = -u->jump_speed;

    u->WaitTics = (RANDOM_RANGE(3)+1) * 120;

    (*u->ActorActionFunc)(SpriteNum);

    return 0;
}

int DoCoolgCircle(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp tsp = User[SpriteNum]->tgt_sp;
    int nx,ny,bound;


    sp->ang = NORM_ANGLE(sp->ang + u->Counter2);

    nx = sp->xvel * (int) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
    ny = sp->xvel * (int) sintable[sp->ang] >> 14;

    if (!move_actor(SpriteNum, nx, ny, 0L))
    {
        InitActorReposition(SpriteNum);
        return 0;
    }

    // move in the z direction
    u->sz -= u->jump_speed * ACTORMOVETICS;

    bound = u->hiz + u->ceiling_dist + COOLG_BOB_AMT;
    if (u->sz < bound)
    {
        // bumped something
        u->sz = bound;
        InitActorReposition(SpriteNum);
        return 0;
    }

    // time out
    if ((u->WaitTics -= ACTORMOVETICS) < 0)
    {
        InitActorReposition(SpriteNum);
        u->WaitTics = 0;
        return 0;
    }

    return 0;
}


int
DoCoolgDeath(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int nx, ny;


    RESET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
    RESET(sp->cstat, CSTAT_SPRITE_INVISIBLE);
    sp->xrepeat = 42;
    sp->shade = -10;

    if (TEST(u->Flags, SPR_FALLING))
    {
        DoFall(SpriteNum);
    }
    else
    {
        DoFindGroundPoint(SpriteNum);
        u->floor_dist = 0;
        DoBeginFall(SpriteNum);
    }

    if (TEST(u->Flags, SPR_SLIDING))
        DoActorSlide(SpriteNum);

    // slide while falling
    nx = sp->xvel * (int) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
    ny = sp->xvel * (int) sintable[sp->ang] >> 14;

    u->ret = move_sprite(SpriteNum, nx, ny, 0L, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, ACTORMOVETICS);
    DoFindGroundPoint(SpriteNum);

    // on the ground
    if (sp->z >= u->loz)
    {
        RESET(u->Flags, SPR_FALLING|SPR_SLIDING);
        RESET(sp->cstat, CSTAT_SPRITE_YFLIP); // If upside down, reset it
        NewStateGroup(SpriteNum, u->ActorActionSet->Dead);
        return 0;
    }

    return 0;
}

int DoCoolgMove(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if ((u->ShellNum -= ACTORMOVETICS) <= 0)
    {
        switch (u->FlagOwner)
        {
        case 0:
            SET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
            u->ShellNum = SEC(2);
            break;
        case 1:
            PlaySound(DIGI_VOID3, &sp->x, &sp->y, &sp->z, v3df_follow);
            RESET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
            SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);
            u->ShellNum = SEC(1) + SEC(RANDOM_RANGE(2));
            break;
        case 2:
            SET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
            RESET(sp->cstat, CSTAT_SPRITE_INVISIBLE);
            u->ShellNum = SEC(2);
            break;
        case 3:
            PlaySound(DIGI_VOID3, &sp->x, &sp->y, &sp->z, v3df_follow);
            RESET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
            RESET(sp->cstat, CSTAT_SPRITE_INVISIBLE);
            u->ShellNum = SEC(2) + SEC(RANDOM_RANGE(3));
            break;
        default:
            u->FlagOwner = 0;
            break;
        }
        u->FlagOwner++;
        if (u->FlagOwner > 3) u->FlagOwner = 0;
    }

    if (u->FlagOwner-1 == 0)
    {
        sp->xrepeat--;
        sp->shade++;
        if (sp->xrepeat < 4) sp->xrepeat = 4;
        if (sp->shade > 126)
        {
            sp->shade = 127;
            sp->hitag = 9998;
        }
    }
    else if (u->FlagOwner-1 == 2)
    {
        sp->hitag = 0;
        sp->xrepeat++;
        sp->shade--;
        if (sp->xrepeat > 42) sp->xrepeat = 42;
        if (sp->shade < -10) sp->shade = -10;
    }
    else if (u->FlagOwner == 0)
    {
        sp->xrepeat = 42;
        sp->shade = -10;
        sp->hitag = 0;
    }

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    if (u->track >= 0)
        ActorFollowTrack(SpriteNum, ACTORMOVETICS);
    else
    {
        (*u->ActorActionFunc)(SpriteNum);
    }

    if (RANDOM_P2(1024) < 32 && !TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
        InitCoolgDrip(SpriteNum);

    DoCoolgMatchPlayerZ(SpriteNum);

    DoActorSectorDamage(SpriteNum);


    return 0;

}

int DoCoolgPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullCoolg(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(SpriteNum);

    return 0;
}


#include "saveable.h"

static saveable_code saveable_coolg_code[] =
{
    SAVE_CODE(CoolgCommon),
    SAVE_CODE(SetupCoolg),
    SAVE_CODE(NewCoolg),
    SAVE_CODE(DoCoolgBirth),
    SAVE_CODE(NullCoolg),
    SAVE_CODE(DoCoolgMatchPlayerZ),
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
