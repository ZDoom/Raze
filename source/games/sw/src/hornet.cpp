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
#include "sprite.h"

BEGIN_SW_NS

ANIMATOR DoHornetCircle, InitHornetCircle;


DECISION HornetBattle[] =
{
    {50,    InitHornetCircle          },
    {798,   InitActorMoveCloser         },
    {800,   InitActorAlertNoise        },
    {1024,  InitActorRunAway            }
};

DECISION HornetOffense[] =
{
    {1022,  InitActorMoveCloser        },
    {1024,  InitActorAlertNoise        }
};

DECISION HornetBroadcast[] =
{
    {3,    InitActorAlertNoise        },
    {6,    InitActorAmbientNoise          },
    {1024,  InitActorDecide             }
};

DECISION HornetSurprised[] =
{
    {100,   InitHornetCircle           },
    {701,   InitActorMoveCloser         },
    {1024,  InitActorDecide             }
};

DECISION HornetEvasive[] =
{
    {20,     InitHornetCircle          },
    {1024,   nullptr                      },
};

DECISION HornetLostTarget[] =
{
    {900,   InitActorFindPlayer         },
    {1024,  InitActorWanderAround       }
};

DECISION HornetCloseRange[] =
{
    {900,   InitActorMoveCloser         },
    {1024,  InitActorReposition         }
};

ANIMATOR InitHornetSting;

DECISION HornetTouchTarget[] =
{
    {500,   InitHornetCircle            },
    {1024,  InitHornetSting             }
};

PERSONALITY HornetPersonality =
{
    HornetBattle,
    HornetOffense,
    HornetBroadcast,
    HornetSurprised,
    HornetEvasive,
    HornetLostTarget,
    HornetCloseRange,
    HornetTouchTarget
};

ATTRIBUTE HornetAttrib =
{
    {300, 350, 375, 400}, // Speeds
    {0,  0, 0,  0}, // Tic Adjusts
    0,      //MaxWeapons;
    {
        0, 0, DIGI_HORNETSTING, DIGI_HORNETSTING, DIGI_HORNETDEATH,
        0,0,0,0,0
    }
};

//////////////////////
//
// HORNET RUN
//////////////////////

#define HORNET_RUN_RATE 7

ANIMATOR DoHornetMove,NullHornet,DoStayOnFloor, DoActorDebris, NullHornet, DoHornetBirth;

STATE s_HornetRun[5][2] =
{
    {
        {HORNET_RUN_R0 + 0, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[0][1]},
        {HORNET_RUN_R0 + 1, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[0][0]},
    },
    {
        {HORNET_RUN_R1 + 0, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[1][1]},
        {HORNET_RUN_R1 + 1, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[1][0]},
    },
    {
        {HORNET_RUN_R2 + 0, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[2][1]},
        {HORNET_RUN_R2 + 1, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[2][0]},
    },
    {
        {HORNET_RUN_R3 + 0, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[3][1]},
        {HORNET_RUN_R3 + 1, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[3][0]},
    },
    {
        {HORNET_RUN_R4 + 0, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[4][1]},
        {HORNET_RUN_R4 + 1, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[4][0]},
    }
};

STATE* sg_HornetRun[] =
{
    &s_HornetRun[0][0],
    &s_HornetRun[1][0],
    &s_HornetRun[2][0],
    &s_HornetRun[3][0],
    &s_HornetRun[4][0]
};

//////////////////////
//
// HORNET STAND
//
//////////////////////

#define HORNET_STAND_RATE (HORNET_RUN_RATE + 5)

STATE s_HornetStand[5][2] =
{
    {
        {HORNET_RUN_R0 + 0, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[0][1]},
        {HORNET_RUN_R0 + 1, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[0][0]}
    },
    {
        {HORNET_RUN_R1 + 0, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[1][1]},
        {HORNET_RUN_R1 + 1, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[1][0]}
    },
    {
        {HORNET_RUN_R2 + 0, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[2][1]},
        {HORNET_RUN_R2 + 1, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[2][0]}
    },
    {
        {HORNET_RUN_R3 + 0, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[3][1]},
        {HORNET_RUN_R3 + 1, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[3][0]}
    },
    {
        {HORNET_RUN_R4 + 0, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[4][1]},
        {HORNET_RUN_R4 + 1, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[4][0]}
    }
};

STATE* sg_HornetStand[] =
{
    &s_HornetStand[0][0],
    &s_HornetStand[1][0],
    &s_HornetStand[2][0],
    &s_HornetStand[3][0],
    &s_HornetStand[4][0]
};

//////////////////////
//
// HORNET DIE
//
//////////////////////

#define HORNET_DIE_RATE 20
ANIMATOR DoHornetDeath;
STATE s_HornetDie[] =
{
#if 0
    {HORNET_DIE + 0, HORNET_DIE_RATE, NullHornet, &s_HornetDie[1]},
    {HORNET_DEAD,    HORNET_DIE_RATE, DoActorDebris, &s_HornetDie[1]},
#else
    {HORNET_DIE + 0, HORNET_DIE_RATE, DoHornetDeath, &s_HornetDie[0]},
#endif
};

STATE* sg_HornetDie[] =
{
    s_HornetDie
};

STATE s_HornetDead[] =
{
    {HORNET_DEAD, HORNET_DIE_RATE, DoActorDebris, &s_HornetDead[0]},
};

STATE* sg_HornetDead[] =
{
    s_HornetDead
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

ACTOR_ACTION_SET HornetActionSet =
{
    sg_HornetStand,
    sg_HornetRun,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr, //climb
    nullptr, //pain
    sg_HornetDie,
    nullptr,
    sg_HornetDead,
    nullptr,
    nullptr,
    {nullptr},
    {0},
    {nullptr},
    {0},
    {nullptr},
    nullptr,
    nullptr
};

int DoHornetMatchPlayerZ(DSWActor* actor);


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int SetupHornet(DSWActor* actor)
{
    ANIMATOR DoActorDecide;

    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,HORNET_RUN_R0,s_HornetRun[0]);
        actor->user.Health = HEALTH_HORNET;
    }

    ChangeState(actor, s_HornetRun[0]);
    actor->user.Attrib = &HornetAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.StateEnd = s_HornetDie;
    actor->user.Rot = sg_HornetRun;

    EnemyDefaults(actor, &HornetActionSet, &HornetPersonality);

    actor->user.Flags |= (SPR_NO_SCAREDZ|SPR_XFLIP_TOGGLE);
    actor->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actor->spr.clipdist = (100) >> 2;
    actor->user.floor_dist = (16);
    actor->user.ceiling_dist = (16);

    actor->user.pos.Z = actor->spr.pos.Z;

    actor->spr.xrepeat = 37;
    actor->spr.yrepeat = 32;

    // Special looping buzz sound attached to each hornet spawned
    PlaySound(DIGI_HORNETBUZZ, actor, v3df_follow|v3df_init);

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int NullHornet(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    DoHornetMatchPlayerZ(actor);
    DoActorSectorDamage(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static const int HORNET_BOB_AMT = 16;

int DoHornetMatchPlayerZ(DSWActor* actor)
{

    // actor does a sine wave about actor->user.sz - this is the z mid point

    double zdiff = (ActorZOfMiddle(actor->user.targetActor)) - actor->user.pos.Z;

    // check z diff of the player and the sprite
    double zdist = 20 + RandomRange(200); // put a random amount
    if (abs(zdiff) > zdist)
    {
        if (zdiff > 0)
            // manipulate the z midpoint
            //actor->user.sz += 256 * ACTORMOVETICS;
            actor->user.pos.Z += 1024 * ACTORMOVETICS * zmaptoworld;
        else
            actor->user.pos.Z -= 256 * ACTORMOVETICS * zmaptoworld;
    }

    // save off lo and hi z
    double loz = actor->user.loz;
    double hiz = actor->user.hiz;

    // adjust loz/hiz for water depth
    if (actor->user.lo_sectp && actor->user.lo_sectp->hasU() && FixedToInt(actor->user.lo_sectp->depth_fixed))
        loz -= FixedToInt(actor->user.lo_sectp->depth_fixed) - 8;

    double bound;
    // lower bound
    if (actor->user.lowActor)
        bound = loz - actor->user.floor_dist;
    else
        bound = loz - actor->user.floor_dist - HORNET_BOB_AMT;

    if (actor->user.pos.Z > bound)
    {
        actor->user.pos.Z = bound;
    }

    // upper bound
    if (actor->user.highActor)
        bound = hiz + actor->user.ceiling_dist;
    else
        bound = hiz + actor->user.ceiling_dist + HORNET_BOB_AMT;

    if (actor->user.pos.Z < bound)
    {
        actor->user.pos.Z = bound;
    }

    actor->user.pos.Z = min(actor->user.pos.Z, loz - actor->user.floor_dist);
    actor->user.pos.Z = max(actor->user.pos.Z, hiz + actor->user.ceiling_dist);

    actor->user.Counter = (actor->user.Counter + (ACTORMOVETICS << 3) + (ACTORMOVETICS << 1)) & 2047;
    actor->spr.pos.Z = actor->user.pos.Z + HORNET_BOB_AMT * BobVal(actor->user.Counter);

    bound = actor->user.hiz + actor->user.ceiling_dist + HORNET_BOB_AMT;
    if (actor->spr.pos.Z < bound)
    {
        // bumped something
        actor->spr.pos.Z = bound + HORNET_BOB_AMT;
        actor->user.pos.Z = actor->spr.pos.Z;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int InitHornetCircle(DSWActor* actor)
{
    actor->user.ActorActionFunc = DoHornetCircle;

    NewStateGroup(actor, actor->user.ActorActionSet->Run);

    // set it close
    DoActorSetSpeed(actor, FAST_SPEED);

    // set to really fast
    actor->vel.X = 25;
    // angle adjuster
    actor->user.Counter2 = 400 / 3;
    // random angle direction
    if (RANDOM_P2(1024) < 512)
        actor->user.Counter2 = -actor->user.Counter2;

    // z velocity
    actor->user.jump_speed = 200 + RANDOM_P2(128);
    if (abs(actor->user.pos.Z - actor->user.hiz) < abs(actor->user.pos.Z - actor->user.loz))
        actor->user.jump_speed = -actor->user.jump_speed;

    actor->user.WaitTics = (RandomRange(3)+1) * 60;

    (*actor->user.ActorActionFunc)(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoHornetCircle(DSWActor* actor)
{
    double bound;

    actor->spr.angle += DAngle::fromBuild(actor->user.Counter2);

    if (!move_actor(actor, DVector3(actor->spr.angle.ToVector() * actor->vel.X, 0)))
    {
        //ActorMoveHitReact(actor);

        // try moving in the opposite direction
        actor->user.Counter2 = -actor->user.Counter2;
        actor->spr.angle += DAngle180;

        if (!move_actor(actor, DVector3(actor->spr.angle.ToVector() * actor->vel.X, 0)))
        {
            InitActorReposition(actor);
            return 0;
        }
    }

    // move in the z direction
    actor->user.pos.Z -= actor->user.jump_speed * ACTORMOVETICS * JUMP_FACTOR;

    bound = actor->user.hiz + actor->user.ceiling_dist + HORNET_BOB_AMT;
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


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoHornetDeath(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_FALLING))
    {
        actor->user.loz = actor->user.zclip;
        DoFall(actor);
    }
    else
    {
        actor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
        actor->user.jump_speed = 0;
        actor->user.floor_dist = 0;
        DoBeginFall(actor);
        DoFindGroundPoint(actor);
        actor->user.zclip = actor->user.loz;
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    // slide while falling
	auto vec = actor->spr.angle.ToVector() * actor->vel.X;

    actor->user.coll = move_sprite(actor, DVector3(vec, 0), actor->user.ceiling_dist, actor->user.floor_dist, 1, ACTORMOVETICS);

    // on the ground
    if (actor->spr.pos.Z >= actor->user.loz)
    {
        actor->user.Flags &= ~(SPR_FALLING|SPR_SLIDING);
        actor->spr.cstat &= ~(CSTAT_SPRITE_YFLIP); // If upside down, reset it
        NewStateGroup(actor, actor->user.ActorActionSet->Dead);
        DeleteNoSoundOwner(actor);
        return 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// Hornets can swarm around other hornets or whatever is tagged as swarm target
//
//---------------------------------------------------------------------------

int DoCheckSwarm(DSWActor* actor)
{
    double dist, pdist;
    PLAYER* pp;

    if (!MoveSkip8) return 0;     // Don't over check

    if (!actor->user.targetActor) return 0;

    // Who's the closest meat!?
    DoActorPickClosePlayer(actor);

    if (actor->user.targetActor->user.PlayerP)
    {
        pp = actor->user.targetActor->user.PlayerP;
        pdist = (actor->spr.pos.XY() - pp->pos.XY()).LengthSquared();
    }
    else
        return 0;

    // all enemys
    SWStatIterator it(STAT_ENEMY);
    while (auto itActor = it.Next())
    {
        if (!itActor->hasU()) continue;

        if (itActor->spr.hitag != TAG_SWARMSPOT || itActor->spr.lotag != 2) continue;

        dist = (actor->spr.pos.XY() - itActor->spr.pos.XY()).LengthSquared();

        if (dist < pdist && actor->user.ID == itActor->user.ID) // Only flock to your own kind
        {
            actor->user.targetActor = itActor; // Set target to swarm center
        }
    }

    return true;

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoHornetMove(DSWActor* actor)
{
    // Check for swarming
    // lotag of 1 = Swarm around lotags of 2
    // lotag of 0 is normal
    if (actor->spr.hitag == TAG_SWARMSPOT && actor->spr.lotag == 1)
        DoCheckSwarm(actor);

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        (*actor->user.ActorActionFunc)(actor);

    DoHornetMatchPlayerZ(actor);

    DoActorSectorDamage(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------


#include "saveable.h"

static saveable_code saveable_hornet_code[] =
{
    SAVE_CODE(NullHornet),
    SAVE_CODE(DoHornetMatchPlayerZ),
    SAVE_CODE(InitHornetCircle),
    SAVE_CODE(DoHornetCircle),
    SAVE_CODE(DoHornetDeath),
    SAVE_CODE(DoCheckSwarm),
    SAVE_CODE(DoHornetMove),
};

static saveable_data saveable_hornet_data[] =
{
    SAVE_DATA(HornetBattle),
    SAVE_DATA(HornetOffense),
    SAVE_DATA(HornetBroadcast),
    SAVE_DATA(HornetSurprised),
    SAVE_DATA(HornetEvasive),
    SAVE_DATA(HornetLostTarget),
    SAVE_DATA(HornetCloseRange),
    SAVE_DATA(HornetTouchTarget),

    SAVE_DATA(HornetPersonality),

    SAVE_DATA(HornetAttrib),

    SAVE_DATA(s_HornetRun),
    SAVE_DATA(sg_HornetRun),
    SAVE_DATA(s_HornetStand),
    SAVE_DATA(sg_HornetStand),
    SAVE_DATA(s_HornetDie),
    SAVE_DATA(sg_HornetDie),
    SAVE_DATA(s_HornetDead),
    SAVE_DATA(sg_HornetDead),

    SAVE_DATA(HornetActionSet),
};

saveable_module saveable_hornet =
{
    // code
    saveable_hornet_code,
    SIZ(saveable_hornet_code),

    // data
    saveable_hornet_data,
    SIZ(saveable_hornet_data)
};
END_SW_NS
