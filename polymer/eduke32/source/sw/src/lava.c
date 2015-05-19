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
#include "actor.h"
#include "track.h"

DECISION LavaBattle[] =
{
    {600,   InitActorMoveCloser         },
    {700,   InitActorAlertNoise         },
    {710,   InitActorRunAway            },
    {1024,  InitActorAttack             }
};

DECISION LavaOffense[] =
{
    {700,   InitActorMoveCloser         },
    {800,   InitActorAlertNoise         },
    {1024,  InitActorAttack             }
};

DECISION LavaBroadcast[] =
{
    {21,    InitActorAlertNoise        },
    {51,    InitActorAmbientNoise      },
    {1024,  InitActorDecide            }
};

DECISION LavaSurprised[] =
{
    {701,   InitActorMoveCloser        },
    {1024,  InitActorDecide            }
};

DECISION LavaEvasive[] =
{
    {10,   InitActorEvade  },
    {1024, NULL            }
};

DECISION LavaLostTarget[] =
{
    {900,   InitActorFindPlayer         },
    {1024,  InitActorWanderAround       }
};

DECISION LavaCloseRange[] =
{
    {700,   InitActorAttack             },
    {1024,  InitActorReposition         }
};

PERSONALITY LavaPersonality =
{
    LavaBattle,
    LavaOffense,
    LavaBroadcast,
    LavaSurprised,
    LavaEvasive,
    LavaLostTarget,
    LavaCloseRange,
    LavaCloseRange
};

ATTRIBUTE LavaAttrib =
{
    {200, 220, 240, 270},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_LAVABOSSAMBIENT,DIGI_LAVABOSSALERT,DIGI_LAVABOSSMETEOR,
        DIGI_LAVABOSSPAIN,DIGI_LAVABOSSEXPLODE,DIGI_LAVABOSSSWIM,
        DIGI_LAVABOSSRISE,DIGI_LAVABOSSFLAME,DIGI_LAVABOSSMETEXP,
        DIGI_LAVABOSSSIZZLE
    }
};

ATTRIBUTE LavaPissedAttrib =
{
    {200, 220, 240, 270},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_LAVABOSSAMBIENT,DIGI_LAVABOSSALERT,DIGI_LAVABOSSMETEOR,
        DIGI_LAVABOSSPAIN,DIGI_LAVABOSSEXPLODE,DIGI_LAVABOSSSWIM,
        DIGI_LAVABOSSRISE,DIGI_LAVABOSSFLAME,DIGI_LAVABOSSMETEXP,
        DIGI_LAVABOSSSIZZLE
    }
};

//////////////////////
//
// LAVA STAND
//
//////////////////////

#define LAVA_STAND_RATE 12
ANIMATOR DoLavaMove,NullLava,DoActorDebris,NullLava;

STATE s_LavaStand[5][1] =
{
    {
        {LAVA_RUN_R0 + 0, LAVA_STAND_RATE, DoLavaMove, &s_LavaStand[0][0]},
    },
    {
        {LAVA_RUN_R1 + 0, LAVA_STAND_RATE, DoLavaMove, &s_LavaStand[1][0]},
    },
    {
        {LAVA_RUN_R2 + 0, LAVA_STAND_RATE, DoLavaMove, &s_LavaStand[2][0]},
    },
    {
        {LAVA_RUN_R3 + 0, LAVA_STAND_RATE, DoLavaMove, &s_LavaStand[3][0]},
    },
    {
        {LAVA_RUN_R4 + 0, LAVA_STAND_RATE, DoLavaMove, &s_LavaStand[4][0]},
    },
};


STATEp sg_LavaStand[] =
{
    s_LavaStand[0],
    s_LavaStand[1],
    s_LavaStand[2],
    s_LavaStand[3],
    s_LavaStand[4]
};


//////////////////////
//
// LAVA RUN
//
//////////////////////

#define LAVA_RUN_RATE 24


STATE s_LavaRun[5][4] =
{
    {
        {LAVA_RUN_R0 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[0][1]},
        {LAVA_RUN_R0 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[0][2]},
        {LAVA_RUN_R0 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[0][3]},
        {LAVA_RUN_R0 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[0][0]},
    },
    {
        {LAVA_RUN_R1 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[1][1]},
        {LAVA_RUN_R1 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[1][2]},
        {LAVA_RUN_R1 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[1][3]},
        {LAVA_RUN_R1 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[1][0]},
    },
    {
        {LAVA_RUN_R2 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[2][1]},
        {LAVA_RUN_R2 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[2][2]},
        {LAVA_RUN_R2 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[2][3]},
        {LAVA_RUN_R2 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[2][0]},
    },
    {
        {LAVA_RUN_R3 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[3][1]},
        {LAVA_RUN_R3 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[3][2]},
        {LAVA_RUN_R3 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[3][3]},
        {LAVA_RUN_R3 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[3][0]},
    },
    {
        {LAVA_RUN_R4 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[4][1]},
        {LAVA_RUN_R4 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[4][2]},
        {LAVA_RUN_R4 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[4][3]},
        {LAVA_RUN_R4 + 0, LAVA_RUN_RATE, DoLavaMove, &s_LavaRun[4][0]},
    }
};


STATEp sg_LavaRun[] =
{
    &s_LavaRun[0][0],
    &s_LavaRun[1][0],
    &s_LavaRun[2][0],
    &s_LavaRun[3][0],
    &s_LavaRun[4][0]
};

//////////////////////
//
// LAVA THROW
//
//////////////////////

#define LAVA_THROW_RATE 9
ANIMATOR InitActorDecide;
ANIMATOR InitLavaThrow;

STATE s_LavaThrow[5][10] =
{
    {
        {LAVA_THROW_R0 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[0][1]},
        {LAVA_THROW_R0 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[0][2]},
        {LAVA_THROW_R0 + 0, LAVA_THROW_RATE*2, NullLava, &s_LavaThrow[0][3]},
        {LAVA_THROW_R0 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[0][4]},
        {LAVA_THROW_R0 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[0][5]},
        {LAVA_THROW_R0 + 0, SF_QUICK_CALL, InitLavaThrow, &s_LavaThrow[0][6]},
        {LAVA_THROW_R0 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[0][7]},
        {LAVA_THROW_R0 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[0][8]},
        {LAVA_THROW_R0 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_LavaThrow[0][9]},
        {LAVA_THROW_R0 + 0, LAVA_THROW_RATE, DoLavaMove, &s_LavaThrow[0][9]},
    },
    {
        {LAVA_THROW_R1 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[1][1]},
        {LAVA_THROW_R1 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[1][2]},
        {LAVA_THROW_R1 + 0, LAVA_THROW_RATE*2, NullLava, &s_LavaThrow[1][3]},
        {LAVA_THROW_R1 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[1][4]},
        {LAVA_THROW_R1 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[1][5]},
        {LAVA_THROW_R1 + 0, SF_QUICK_CALL, InitLavaThrow, &s_LavaThrow[1][6]},
        {LAVA_THROW_R1 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[1][7]},
        {LAVA_THROW_R1 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[1][8]},
        {LAVA_THROW_R1 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_LavaThrow[1][9]},
        {LAVA_THROW_R1 + 0, LAVA_THROW_RATE, DoLavaMove, &s_LavaThrow[1][9]},
    },
    {
        {LAVA_THROW_R2 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[2][1]},
        {LAVA_THROW_R2 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[2][2]},
        {LAVA_THROW_R2 + 0, LAVA_THROW_RATE*2, NullLava, &s_LavaThrow[2][3]},
        {LAVA_THROW_R2 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[2][4]},
        {LAVA_THROW_R2 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[2][5]},
        {LAVA_THROW_R2 + 0, SF_QUICK_CALL, InitLavaThrow, &s_LavaThrow[2][6]},
        {LAVA_THROW_R2 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[2][7]},
        {LAVA_THROW_R2 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[2][8]},
        {LAVA_THROW_R2 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_LavaThrow[2][9]},
        {LAVA_THROW_R2 + 0, LAVA_THROW_RATE, DoLavaMove, &s_LavaThrow[2][9]},
    },
    {
        {LAVA_THROW_R3 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[3][1]},
        {LAVA_THROW_R3 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[3][2]},
        {LAVA_THROW_R3 + 0, LAVA_THROW_RATE*2, NullLava, &s_LavaThrow[3][3]},
        {LAVA_THROW_R3 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[3][4]},
        {LAVA_THROW_R3 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[3][5]},
        {LAVA_THROW_R3 + 0, SF_QUICK_CALL, InitLavaThrow, &s_LavaThrow[3][6]},
        {LAVA_THROW_R3 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[3][7]},
        {LAVA_THROW_R3 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[3][8]},
        {LAVA_THROW_R3 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_LavaThrow[3][9]},
        {LAVA_THROW_R3 + 0, LAVA_THROW_RATE, DoLavaMove, &s_LavaThrow[3][9]},
    },
    {
        {LAVA_THROW_R4 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[4][1]},
        {LAVA_THROW_R4 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[4][2]},
        {LAVA_THROW_R4 + 0, LAVA_THROW_RATE*2, NullLava, &s_LavaThrow[4][3]},
        {LAVA_THROW_R4 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[4][4]},
        {LAVA_THROW_R4 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[4][5]},
        {LAVA_THROW_R4 + 0, SF_QUICK_CALL, InitLavaThrow, &s_LavaThrow[4][6]},
        {LAVA_THROW_R4 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[4][7]},
        {LAVA_THROW_R4 + 0, LAVA_THROW_RATE, NullLava, &s_LavaThrow[4][8]},
        {LAVA_THROW_R4 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_LavaThrow[4][9]},
        {LAVA_THROW_R4 + 0, LAVA_THROW_RATE, DoLavaMove, &s_LavaThrow[4][9]},
    }
};


STATEp sg_LavaThrow[] =
{
    &s_LavaThrow[0][0],
    &s_LavaThrow[1][0],
    &s_LavaThrow[2][0],
    &s_LavaThrow[3][0],
    &s_LavaThrow[4][0]
};


//////////////////////
//
// LAVA FLAME
//
//////////////////////

#define LAVA_FLAME_RATE 18
ANIMATOR InitLavaFlame;

STATE s_LavaFlame[5][8] =
{
    {
        {LAVA_FLAME_R0 + 0, LAVA_FLAME_RATE*2, NullLava, &s_LavaFlame[0][1]},
        {LAVA_FLAME_R0 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[0][2]},
        {LAVA_FLAME_R0 + 0, LAVA_FLAME_RATE*2, NullLava, &s_LavaFlame[0][3]},
        {LAVA_FLAME_R0 + 0, SF_QUICK_CALL, InitLavaFlame, &s_LavaFlame[0][4]},
        {LAVA_FLAME_R0 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[0][5]},
        {LAVA_FLAME_R0 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[0][6]},
        {LAVA_FLAME_R0 + 0, SF_QUICK_CALL,   InitActorDecide, &s_LavaFlame[0][7]},
        {LAVA_FLAME_R0 + 0, LAVA_FLAME_RATE, DoLavaMove, &s_LavaFlame[0][7]},
    },
    {
        {LAVA_FLAME_R1 + 0, LAVA_FLAME_RATE*2, NullLava, &s_LavaFlame[1][1]},
        {LAVA_FLAME_R1 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[1][2]},
        {LAVA_FLAME_R1 + 0, LAVA_FLAME_RATE*2, NullLava, &s_LavaFlame[1][3]},
        {LAVA_FLAME_R1 + 0, SF_QUICK_CALL, InitLavaFlame, &s_LavaFlame[1][4]},
        {LAVA_FLAME_R1 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[1][5]},
        {LAVA_FLAME_R1 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[1][6]},
        {LAVA_FLAME_R1 + 0, SF_QUICK_CALL, InitActorDecide, &s_LavaFlame[1][7]},
        {LAVA_FLAME_R1 + 0, LAVA_FLAME_RATE, DoLavaMove, &s_LavaFlame[1][7]},
    },
    {
        {LAVA_FLAME_R2 + 0, LAVA_FLAME_RATE*2, NullLava, &s_LavaFlame[2][1]},
        {LAVA_FLAME_R2 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[2][2]},
        {LAVA_FLAME_R2 + 0, LAVA_FLAME_RATE*2, NullLava, &s_LavaFlame[2][3]},
        {LAVA_FLAME_R2 + 0, SF_QUICK_CALL, InitLavaFlame, &s_LavaFlame[2][4]},
        {LAVA_FLAME_R2 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[2][5]},
        {LAVA_FLAME_R2 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[2][6]},
        {LAVA_FLAME_R2 + 0, SF_QUICK_CALL, InitActorDecide, &s_LavaFlame[2][7]},
        {LAVA_FLAME_R2 + 0, LAVA_FLAME_RATE, DoLavaMove, &s_LavaFlame[2][7]},
    },
    {
        {LAVA_FLAME_R3 + 0, LAVA_FLAME_RATE*2, NullLava, &s_LavaFlame[3][1]},
        {LAVA_FLAME_R3 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[3][2]},
        {LAVA_FLAME_R3 + 0, LAVA_FLAME_RATE*2, NullLava, &s_LavaFlame[3][3]},
        {LAVA_FLAME_R3 + 0, SF_QUICK_CALL, InitLavaFlame, &s_LavaFlame[3][4]},
        {LAVA_FLAME_R3 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[3][5]},
        {LAVA_FLAME_R3 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[3][6]},
        {LAVA_FLAME_R3 + 0, SF_QUICK_CALL, InitActorDecide, &s_LavaFlame[3][7]},
        {LAVA_FLAME_R3 + 0, LAVA_FLAME_RATE, DoLavaMove, &s_LavaFlame[3][7]},
    },
    {
        {LAVA_FLAME_R4 + 0, LAVA_FLAME_RATE*2, NullLava, &s_LavaFlame[4][1]},
        {LAVA_FLAME_R4 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[4][2]},
        {LAVA_FLAME_R4 + 0, LAVA_FLAME_RATE*2, NullLava, &s_LavaFlame[4][3]},
        {LAVA_FLAME_R4 + 0, SF_QUICK_CALL, InitLavaFlame, &s_LavaFlame[4][4]},
        {LAVA_FLAME_R4 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[4][5]},
        {LAVA_FLAME_R4 + 0, LAVA_FLAME_RATE, NullLava, &s_LavaFlame[4][6]},
        {LAVA_FLAME_R4 + 0, SF_QUICK_CALL, InitActorDecide, &s_LavaFlame[4][7]},
        {LAVA_FLAME_R4 + 0, LAVA_FLAME_RATE, DoLavaMove, &s_LavaFlame[4][7]},
    }
};


STATEp sg_LavaFlame[] =
{
    &s_LavaFlame[0][0],
    &s_LavaFlame[1][0],
    &s_LavaFlame[2][0],
    &s_LavaFlame[3][0],
    &s_LavaFlame[4][0]
};

//////////////////////
//
// LAVA DIE
//
//////////////////////

#define LAVA_DIE_RATE 20

STATE s_LavaDie[] =
{
    {LAVA_DIE + 0, LAVA_DIE_RATE, NullLava, &s_LavaDie[1]},
    {LAVA_DEAD,    LAVA_DIE_RATE, DoActorDebris, &s_LavaDie[1]}
};

STATE s_LavaDead[] =
{
    {LAVA_DEAD, LAVA_DIE_RATE, DoActorDebris, &s_LavaDead[0]},
};

STATEp sg_LavaDie[] =
{
    s_LavaDie
};

STATEp sg_LavaDead[] =
{
    s_LavaDead
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


ACTOR_ACTION_SET LavaActionSet =
{
    sg_LavaStand,
    sg_LavaRun,
    NULL, //sg_LavaJump,
    NULL, //sg_LavaFall,
    NULL, //sg_LavaCrawl,
    NULL, //sg_LavaSwim,
    NULL, //sg_LavaFly,
    NULL, //sg_LavaRise,
    NULL, //sg_LavaSit,
    NULL, //sg_LavaLook,
    NULL, //climb
    NULL, //pain
    sg_LavaDie,
    NULL, //sg_LavaHariKari,
    sg_LavaDead,
    NULL, //sg_LavaDeathJump,
    NULL, //sg_LavaDeathFall,
    {sg_LavaFlame},
    {1024},
    {sg_LavaFlame, sg_LavaThrow, sg_LavaThrow, sg_LavaThrow},
    {256, 512, 768, 1024},
    {NULL},
    NULL,
    NULL
};

int
SetupLava(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,LAVA_RUN_R0,s_LavaRun[0]);
        u->Health = 100;
    }

    ChangeState(SpriteNum, s_LavaRun[0]);
    u->Attrib = &LavaAttrib;
    DoActorSetSpeed(SpriteNum, NORM_SPEED);
    u->StateEnd = s_LavaDie;
    u->Rot = sg_LavaRun;

    EnemyDefaults(SpriteNum, &LavaActionSet, &LavaPersonality);
    sp->xrepeat = sp->yrepeat = 110;
    sp->clipdist = (512) >> 2;
    SET(u->Flags, SPR_XFLIP_TOGGLE|SPR_ELECTRO_TOLERANT);

    u->loz = sp->z;

    return 0;
}

int NullLava(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    KeepActorOnFloor(SpriteNum);

    DoActorSectorDamage(SpriteNum);
    return 0;
}

int DoLavaMove(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    if (u->track >= 0)
        ActorFollowTrack(SpriteNum, ACTORMOVETICS);
    else
        (*u->ActorActionFunc)(SpriteNum);

    KeepActorOnFloor(SpriteNum);

    DoActorSectorDamage(SpriteNum);
    return 0;
}


#include "saveable.h"

static saveable_code saveable_lava_code[] =
{
    SAVE_CODE(SetupLava),
    SAVE_CODE(NullLava),
    SAVE_CODE(DoLavaMove),
};

static saveable_data saveable_lava_data[] =
{
    SAVE_DATA(LavaBattle),
    SAVE_DATA(LavaOffense),
    SAVE_DATA(LavaBroadcast),
    SAVE_DATA(LavaSurprised),
    SAVE_DATA(LavaEvasive),
    SAVE_DATA(LavaLostTarget),
    SAVE_DATA(LavaCloseRange),

    SAVE_DATA(LavaPersonality),

    SAVE_DATA(LavaAttrib),
    SAVE_DATA(LavaPissedAttrib),

    SAVE_DATA(s_LavaStand),
    SAVE_DATA(sg_LavaStand),
    SAVE_DATA(s_LavaRun),
    SAVE_DATA(sg_LavaRun),
    SAVE_DATA(s_LavaThrow),
    SAVE_DATA(sg_LavaThrow),
    SAVE_DATA(s_LavaFlame),
    SAVE_DATA(sg_LavaFlame),
    SAVE_DATA(s_LavaDie),
    SAVE_DATA(s_LavaDead),
    SAVE_DATA(sg_LavaDie),
    SAVE_DATA(sg_LavaDead),

    SAVE_DATA(LavaActionSet),
};

saveable_module saveable_lava =
{
    // code
    saveable_lava_code,
    SIZ(saveable_lava_code),

    // data
    saveable_lava_data,
    SIZ(saveable_lava_data)
};
