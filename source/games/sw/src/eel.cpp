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
#include "weapon.h"
#include "misc.h"

BEGIN_SW_NS

DECISION EelBattle[] =
{
    {649,   InitActorMoveCloser         },
    {650,   InitActorAlertNoise         },
    {1024,  InitActorMoveCloser         }
};

DECISION EelOffense[] =
{
    {649,   InitActorMoveCloser         },
    {750,   InitActorAlertNoise         },
    {1024,  InitActorMoveCloser         }
};

DECISION EelBroadcast[] =
{
    {3,    InitActorAlertNoise         },
    {6,    InitActorAmbientNoise       },
    {1024,  InitActorDecide            }
};

DECISION EelSurprised[] =
{
    {701,   InitActorMoveCloser        },
    {1024,  InitActorDecide            }
};

DECISION EelEvasive[] =
{
    { 790,  InitActorRunAway           },
    {1024,  InitActorMoveCloser        },
};

DECISION EelLostTarget[] =
{
    {900,   InitActorFindPlayer         },
    {1024,  InitActorWanderAround       }
};

DECISION EelCloseRange[] =
{
    {950,   InitActorAttack            },
    {1024,  InitActorReposition            }
};

ANIMATOR InitEelFire;

DECISION EelTouchTarget[] =
{
    {1024,  InitActorAttack            },
};

PERSONALITY EelPersonality =
{
    EelBattle,
    EelOffense,
    EelBroadcast,
    EelSurprised,
    EelEvasive,
    EelLostTarget,
    EelCloseRange,
    EelTouchTarget
};

ATTRIBUTE EelAttrib =
{
    {100, 110, 120, 130},               // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        0, 0, 0,
        0, 0, 0,
        0,0,0,0
    }
};

//////////////////////
// EEL RUN
//////////////////////

#define EEL_RUN_RATE 20

ANIMATOR DoEelMove,DoStayOnFloor, DoActorDebris, NullEel;

STATE s_EelRun[5][4] =
{
    {
        {EEL_RUN_R0 + 0, EEL_RUN_RATE, DoEelMove, &s_EelRun[0][1]},
        {EEL_RUN_R0 + 1, EEL_RUN_RATE, DoEelMove, &s_EelRun[0][2]},
        {EEL_RUN_R0 + 2, EEL_RUN_RATE, DoEelMove, &s_EelRun[0][3]},
        {EEL_RUN_R0 + 1, EEL_RUN_RATE, DoEelMove, &s_EelRun[0][0]},
    },
    {
        {EEL_RUN_R1 + 0, EEL_RUN_RATE, DoEelMove, &s_EelRun[1][1]},
        {EEL_RUN_R1 + 1, EEL_RUN_RATE, DoEelMove, &s_EelRun[1][2]},
        {EEL_RUN_R1 + 2, EEL_RUN_RATE, DoEelMove, &s_EelRun[1][3]},
        {EEL_RUN_R1 + 1, EEL_RUN_RATE, DoEelMove, &s_EelRun[1][0]},
    },
    {
        {EEL_RUN_R2 + 0, EEL_RUN_RATE, DoEelMove, &s_EelRun[2][1]},
        {EEL_RUN_R2 + 1, EEL_RUN_RATE, DoEelMove, &s_EelRun[2][2]},
        {EEL_RUN_R2 + 2, EEL_RUN_RATE, DoEelMove, &s_EelRun[2][3]},
        {EEL_RUN_R2 + 1, EEL_RUN_RATE, DoEelMove, &s_EelRun[2][0]},
    },
    {
        {EEL_RUN_R3 + 0, EEL_RUN_RATE, DoEelMove, &s_EelRun[3][1]},
        {EEL_RUN_R3 + 1, EEL_RUN_RATE, DoEelMove, &s_EelRun[3][2]},
        {EEL_RUN_R3 + 2, EEL_RUN_RATE, DoEelMove, &s_EelRun[3][3]},
        {EEL_RUN_R3 + 1, EEL_RUN_RATE, DoEelMove, &s_EelRun[3][0]},
    },
    {
        {EEL_RUN_R4 + 0, EEL_RUN_RATE, DoEelMove, &s_EelRun[4][1]},
        {EEL_RUN_R4 + 1, EEL_RUN_RATE, DoEelMove, &s_EelRun[4][2]},
        {EEL_RUN_R4 + 2, EEL_RUN_RATE, DoEelMove, &s_EelRun[4][3]},
        {EEL_RUN_R4 + 1, EEL_RUN_RATE, DoEelMove, &s_EelRun[4][0]},
    }
};

STATE* sg_EelRun[] =
{
    &s_EelRun[0][0],
    &s_EelRun[1][0],
    &s_EelRun[2][0],
    &s_EelRun[3][0],
    &s_EelRun[4][0]
};

//////////////////////
//
// EEL STAND
//
//////////////////////


STATE s_EelStand[5][1] =
{
    {
        {EEL_RUN_R0 + 0, EEL_RUN_RATE, DoEelMove, &s_EelStand[0][0]},
    },
    {
        {EEL_RUN_R1 + 0, EEL_RUN_RATE, DoEelMove, &s_EelStand[1][0]},
    },
    {
        {EEL_RUN_R2 + 0, EEL_RUN_RATE, DoEelMove, &s_EelStand[2][0]},
    },
    {
        {EEL_RUN_R3 + 0, EEL_RUN_RATE, DoEelMove, &s_EelStand[3][0]},
    },
    {
        {EEL_RUN_R4 + 0, EEL_RUN_RATE, DoEelMove, &s_EelStand[4][0]},
    }
};

STATE* sg_EelStand[] =
{
    &s_EelStand[0][0],
    &s_EelStand[1][0],
    &s_EelStand[2][0],
    &s_EelStand[3][0],
    &s_EelStand[4][0]
};

//////////////////////
//
// EEL FIRE
//
//////////////////////

ANIMATOR InitEelFire, EelShock;
#define EEL_FIRE_RATE 12

STATE s_EelAttack[5][7] =
{
    {
        {EEL_FIRE_R0 + 0, EEL_FIRE_RATE*2,  NullEel,              &s_EelAttack[0][1]},
        {EEL_FIRE_R0 + 1, EEL_FIRE_RATE*2,  NullEel,              &s_EelAttack[0][2]},
        {EEL_FIRE_R0 + 2, EEL_FIRE_RATE*2,  NullEel,              &s_EelAttack[0][3]},
        {EEL_FIRE_R0 + 2, 0|SF_QUICK_CALL,  InitEelFire,          &s_EelAttack[0][4]},
        {EEL_FIRE_R0 + 2, EEL_FIRE_RATE,    NullEel,              &s_EelAttack[0][5]},
        {EEL_FIRE_R0 + 3, 0|SF_QUICK_CALL,  InitActorDecide,      &s_EelAttack[0][6]},
        {EEL_RUN_R0  + 3, EEL_FIRE_RATE,    DoEelMove,            &s_EelAttack[0][6]}
    },
    {
        {EEL_FIRE_R1 + 0, EEL_FIRE_RATE*2,  NullEel,              &s_EelAttack[1][1]},
        {EEL_FIRE_R1 + 1, EEL_FIRE_RATE*2,  NullEel,              &s_EelAttack[1][2]},
        {EEL_FIRE_R1 + 2, EEL_FIRE_RATE*2,  NullEel,              &s_EelAttack[1][3]},
        {EEL_FIRE_R1 + 2, 0|SF_QUICK_CALL,  InitEelFire,          &s_EelAttack[1][5]},
        {EEL_FIRE_R1 + 2, EEL_FIRE_RATE,    NullEel,              &s_EelAttack[1][6]},
        {EEL_FIRE_R1 + 3, 0|SF_QUICK_CALL,  InitActorDecide,      &s_EelAttack[1][7]},
        {EEL_RUN_R0  + 3, EEL_FIRE_RATE,    DoEelMove,            &s_EelAttack[1][7]}
    },
    {
        {EEL_FIRE_R2 + 0, EEL_FIRE_RATE*2,  NullEel,              &s_EelAttack[2][1]},
        {EEL_FIRE_R2 + 1, EEL_FIRE_RATE*2,  NullEel,              &s_EelAttack[2][2]},
        {EEL_FIRE_R2 + 2, EEL_FIRE_RATE*2,  NullEel,              &s_EelAttack[2][3]},
        {EEL_FIRE_R2 + 2, 0|SF_QUICK_CALL,  InitEelFire,          &s_EelAttack[2][4]},
        {EEL_FIRE_R2 + 2, EEL_FIRE_RATE,    NullEel,              &s_EelAttack[2][5]},
        {EEL_FIRE_R2 + 3, 0|SF_QUICK_CALL,  InitActorDecide,      &s_EelAttack[2][6]},
        {EEL_RUN_R0  + 3, EEL_FIRE_RATE,    DoEelMove,            &s_EelAttack[2][6]}
    },
    {
        {EEL_RUN_R3 + 0, EEL_FIRE_RATE*2,  NullEel,               &s_EelAttack[3][1]},
        {EEL_RUN_R3 + 1, EEL_FIRE_RATE*2,  NullEel,               &s_EelAttack[3][2]},
        {EEL_RUN_R3 + 2, EEL_FIRE_RATE*2,  NullEel,               &s_EelAttack[3][3]},
        {EEL_RUN_R3 + 2, 0|SF_QUICK_CALL,  InitEelFire,           &s_EelAttack[3][4]},
        {EEL_RUN_R3 + 2, EEL_FIRE_RATE,    NullEel,               &s_EelAttack[3][5]},
        {EEL_RUN_R3 + 3, 0|SF_QUICK_CALL,  InitActorDecide,       &s_EelAttack[3][6]},
        {EEL_RUN_R0 + 3, EEL_FIRE_RATE,    DoEelMove,             &s_EelAttack[3][6]}
    },
    {
        {EEL_RUN_R4 + 0, EEL_FIRE_RATE*2,  NullEel,               &s_EelAttack[4][1]},
        {EEL_RUN_R4 + 1, EEL_FIRE_RATE*2,  NullEel,               &s_EelAttack[4][2]},
        {EEL_RUN_R4 + 2, EEL_FIRE_RATE*2,  NullEel,               &s_EelAttack[4][3]},
        {EEL_RUN_R4 + 2, 0|SF_QUICK_CALL,  InitEelFire,           &s_EelAttack[4][4]},
        {EEL_RUN_R4 + 2, EEL_FIRE_RATE,    NullEel,               &s_EelAttack[4][5]},
        {EEL_RUN_R4 + 3, 0|SF_QUICK_CALL,  InitActorDecide,       &s_EelAttack[4][6]},
        {EEL_RUN_R0 + 3, EEL_FIRE_RATE,    DoEelMove,             &s_EelAttack[4][6]}
    }
};

STATE* sg_EelAttack[] =
{
    &s_EelAttack[0][0],
    &s_EelAttack[1][0],
    &s_EelAttack[2][0],
    &s_EelAttack[3][0],
    &s_EelAttack[4][0]
};


//////////////////////
//
// EEL DIE
//
//////////////////////

#define EEL_DIE_RATE 20

ANIMATOR DoEelDeath;
STATE s_EelDie[] =
{
    {EEL_DIE +    0, EEL_DIE_RATE, DoEelDeath, &s_EelDie[1]},
    {EEL_DIE +    0, EEL_DIE_RATE, DoEelDeath, &s_EelDie[2]},
    {EEL_DIE +    0, EEL_DIE_RATE, DoEelDeath, &s_EelDie[3]},
    {EEL_DIE +    0, EEL_DIE_RATE, DoEelDeath, &s_EelDie[4]},
    {EEL_DIE +    0, EEL_DIE_RATE, DoEelDeath, &s_EelDie[5]},
    {EEL_DIE +    0, EEL_DIE_RATE, DoEelDeath, &s_EelDie[5]},
};

STATE* sg_EelDie[] =
{
    s_EelDie
};

STATE s_EelDead[] =
{
//    {EEL_DEAD, SF_QUICK_CALL , QueueFloorBlood, &s_EelDead[1]},
    {EEL_DEAD, EEL_DIE_RATE, DoActorDebris, &s_EelDead[0]},
};

STATE* sg_EelDead[] =
{
    s_EelDead
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


ACTOR_ACTION_SET EelActionSet =
{
    sg_EelStand,
    sg_EelRun,
    nullptr,
    nullptr,
    nullptr,
    sg_EelRun,
    nullptr,
    nullptr,
    sg_EelStand,
    nullptr,
    nullptr, //climb
    sg_EelStand, //pain
    sg_EelDie,
    nullptr,
    sg_EelDead,
    nullptr,
    nullptr,
    {sg_EelAttack},
    {1024},
    {sg_EelAttack},
    {1024},
    {nullptr,nullptr},
    nullptr,
    nullptr
};

int DoEelMatchPlayerZ(DSWActor* actor);


void EelCommon(DSWActor* actor)
{
    actor->spr.clipdist = (100) >> 2;
    actor->user.floor_dist = Z(16);
    actor->user.floor_dist = Z(16);
    actor->user.ceiling_dist = Z(20);

    actor->user.pos.Z = actor->int_pos().Z;

    actor->spr.xrepeat = 35;
    actor->spr.yrepeat = 27;
    actor->user.Radius = 400;
}

int SetupEel(DSWActor* actor)
{
    ANIMATOR DoActorDecide;

    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,EEL_RUN_R0,s_EelRun[0]);
        actor->user.Health = 40;
    }

    ChangeState(actor, s_EelRun[0]);
    actor->user.Attrib = &EelAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.StateEnd = s_EelDie;
    actor->user.Rot = sg_EelRun;

    EnemyDefaults(actor, &EelActionSet, &EelPersonality);

    actor->user.Flags |= (SPR_NO_SCAREDZ|SPR_XFLIP_TOGGLE);

    EelCommon(actor);

    actor->user.Flags &= ~(SPR_SHADOW); // Turn off shadows
    actor->user.zclip = Z(8);

    return 0;
}


int NullEel(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    DoEelMatchPlayerZ(actor);

    DoActorSectorDamage(actor);

    return 0;
}

int DoEelMatchPlayerZ(DSWActor* actor)
{
    int zdiff,zdist;
    int loz,hiz;
    int dist,a,b,c;

    int bound;

    if (FAF_ConnectArea(actor->sector()))
    {
        if (actor->user.hi_sectp)
        {
            actor->user.hiz = actor->sector()->int_ceilingz() + Z(16);
            actor->user.hi_sectp = actor->sector();
        }
        else
        {
            if (actor->user.hiz < actor->sector()->int_ceilingz() + Z(16))
                actor->user.hiz = actor->sector()->int_ceilingz() + Z(16);
        }
    }

    // actor does a sine wave about actor->user.sz - this is the z mid point

    zdiff = (ActorZOfBottom(actor->user.targetActor) - Z(8)) - actor->user.pos.Z;

    // check z diff of the player and the sprite
    zdist = Z(20 + RandomRange(64)); // put a random amount
    if (labs(zdiff) > zdist)
    {
        if (zdiff > 0)
            // manipulate the z midpoint
            actor->user.pos.Z += 160 * ACTORMOVETICS;
        else
            actor->user.pos.Z -= 160 * ACTORMOVETICS;
    }

    const int EEL_BOB_AMT = (Z(4));

    // save off lo and hi z
    loz = actor->user.loz;
    hiz = actor->user.hiz;

    // adjust loz/hiz for water depth
    if (actor->user.lo_sectp && actor->user.lo_sectp->hasU() && FixedToInt(actor->user.lo_sectp->depth_fixed))
        loz -= Z(FixedToInt(actor->user.lo_sectp->depth_fixed)) - Z(8);

    // lower bound
    if (actor->user.lowActor && actor->user.targetActor == actor->user.highActor) // this doesn't look right...
    {
        DISTANCE(actor->int_pos().X, actor->int_pos().Y, actor->user.lowActor->int_pos().X, actor->user.lowActor->int_pos().Y, dist, a, b, c);
        if (dist <= 300)
            bound = actor->user.pos.Z;
        else
            bound = loz - actor->user.floor_dist;
    }
    else
        bound = loz - actor->user.floor_dist - EEL_BOB_AMT;

    if (actor->user.pos.Z > bound)
    {
        actor->user.pos.Z = bound;
    }

    // upper bound
    if (actor->user.highActor && actor->user.targetActor == actor->user.highActor)
    {
        DISTANCE(actor->int_pos().X, actor->int_pos().Y, actor->user.highActor->int_pos().X, actor->user.highActor->int_pos().Y, dist, a, b, c);
        if (dist <= 300)
            bound = actor->user.pos.Z;
        else
            bound = hiz + actor->user.ceiling_dist;
    }
    else
        bound = hiz + actor->user.ceiling_dist + EEL_BOB_AMT;

    if (actor->user.pos.Z < bound)
    {
        actor->user.pos.Z = bound;
    }

    actor->user.pos.Z = min(actor->user.pos.Z, loz - actor->user.floor_dist);
    actor->user.pos.Z = max(actor->user.pos.Z, hiz + actor->user.ceiling_dist);

    actor->user.Counter = (actor->user.Counter + (ACTORMOVETICS << 3) + (ACTORMOVETICS << 1)) & 2047;
    actor->set_int_z(actor->user.pos.Z + MulScale(EEL_BOB_AMT, bsin(actor->user.Counter), 14));

    bound = actor->user.hiz + actor->user.ceiling_dist + EEL_BOB_AMT;
    if (actor->int_pos().Z < bound)
    {
        // bumped something
        actor->set_int_z(actor->user.pos.Z = bound + EEL_BOB_AMT);
    }

    return 0;
}

int DoEelDeath(DSWActor* actor)
{
    int nx, ny;
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
    nx = MulScale(actor->spr.xvel, bcos(actor->int_ang()), 14);
    ny = MulScale(actor->spr.xvel, bsin(actor->int_ang()), 14);

    actor->user.coll = move_sprite(actor, nx, ny, 0L, actor->user.ceiling_dist, actor->user.floor_dist, CLIPMASK_MISSILE, ACTORMOVETICS);
    DoFindGroundPoint(actor);

    // on the ground
    if (actor->int_pos().Z >= actor->user.loz)
    {
        actor->user.Flags &= ~(SPR_FALLING|SPR_SLIDING);
        if (RandomRange(1000) > 500)
            actor->spr.cstat |= (CSTAT_SPRITE_XFLIP);
        if (RandomRange(1000) > 500)
            actor->spr.cstat |= (CSTAT_SPRITE_YFLIP);
        NewStateGroup(actor, actor->user.ActorActionSet->Dead);
        return 0;
    }

    return 0;
}

int DoEelMove(DSWActor* actor)
{
    ASSERT(actor->user.Rot != nullptr);

    if (SpriteOverlap(actor, actor->user.targetActor))
        NewStateGroup(actor, actor->user.ActorActionSet->CloseAttack[0]);

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        (*actor->user.ActorActionFunc)(actor);

    DoEelMatchPlayerZ(actor);

    DoActorSectorDamage(actor);

    return 0;

}


#include "saveable.h"

static saveable_code saveable_eel_code[] =
{
    SAVE_CODE(DoEelMatchPlayerZ),
    SAVE_CODE(DoEelDeath),
    SAVE_CODE(DoEelMove)
};

static saveable_data saveable_eel_data[] =
{
    SAVE_DATA(EelBattle),
    SAVE_DATA(EelOffense),
    SAVE_DATA(EelBroadcast),
    SAVE_DATA(EelSurprised),
    SAVE_DATA(EelEvasive),
    SAVE_DATA(EelLostTarget),
    SAVE_DATA(EelCloseRange),
    SAVE_DATA(EelTouchTarget),

    SAVE_DATA(EelPersonality),

    SAVE_DATA(EelAttrib),

    SAVE_DATA(s_EelRun),
    SAVE_DATA(sg_EelRun),
    SAVE_DATA(s_EelStand),
    SAVE_DATA(sg_EelStand),
    SAVE_DATA(s_EelAttack),
    SAVE_DATA(sg_EelAttack),
    SAVE_DATA(s_EelDie),
    SAVE_DATA(sg_EelDie),
    SAVE_DATA(s_EelDead),
    SAVE_DATA(sg_EelDead),

    SAVE_DATA(EelActionSet)
};

saveable_module saveable_eel =
{
    // code
    saveable_eel_code,
    SIZ(saveable_eel_code),

    // data
    saveable_eel_data,
    SIZ(saveable_eel_data)
};
END_SW_NS
