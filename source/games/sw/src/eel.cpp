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



#include "names2.h"
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "weapon.h"
#include "misc.h"

BEGIN_SW_NS

DECISION EelBattle[] =
{
    {649,   &AF(InitActorMoveCloser)         },
    {650,   &AF(InitActorSetDecide)      },
    {1024,  &AF(InitActorMoveCloser)         }
};

DECISION EelOffense[] =
{
    {649,   &AF(InitActorMoveCloser)         },
    {750,   &AF(InitActorSetDecide)     },
    {1024,  &AF(InitActorMoveCloser)         }
};

DECISIONB EelBroadcast[] =
{
    {3,    attr_alert       },
    {6,    attr_ambient  },
    {1024, 0   }
};

DECISION EelSurprised[] =
{
    {701,   &AF(InitActorMoveCloser)        },
    {1024,  &AF(InitActorDecide    )        }
};

DECISION EelEvasive[] =
{
    { 790,  &AF(InitActorRunAway   )        },
    {1024,  &AF(InitActorMoveCloser)        },
};

DECISION EelLostTarget[] =
{
    {900,   &AF(InitActorFindPlayer  )       },
    {1024,  &AF(InitActorWanderAround)       }
};

DECISION EelCloseRange[] =
{
    {950,   &AF(InitActorAttack    )        },
    {1024,  &AF(InitActorReposition)            }
};

DECISION EelTouchTarget[] =
{
    {1024,  &AF(InitActorAttack)            },
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

FState s_EelRun[] =
{
        {SPR_EEL_RUN, 'A', EEL_RUN_RATE, &AF(DoEelMove), &s_EelRun[1]},
        {SPR_EEL_RUN, 'B', EEL_RUN_RATE, &AF(DoEelMove), &s_EelRun[2]},
        {SPR_EEL_RUN, 'C', EEL_RUN_RATE, &AF(DoEelMove), &s_EelRun[3]},
        {SPR_EEL_RUN, 'B', EEL_RUN_RATE, &AF(DoEelMove), &s_EelRun[0]},
};

//////////////////////
//
// EEL STAND
//
//////////////////////


FState s_EelStand[] =
{
        {SPR_EEL_RUN, 'A', EEL_RUN_RATE, &AF(DoEelMove), &s_EelStand[0]},
};

//////////////////////
//
// EEL FIRE
//
//////////////////////

#define EEL_FIRE_RATE 12

FState s_EelAttack[] =
{
        {SPR_EEL_FIRE, 'A', EEL_FIRE_RATE*2,  &AF(NullEel),              &s_EelAttack[1]},
        {SPR_EEL_FIRE, 'B', EEL_FIRE_RATE*2,  &AF(NullEel),              &s_EelAttack[2]},
        {SPR_EEL_FIRE, 'C', EEL_FIRE_RATE*2,  &AF(NullEel),              &s_EelAttack[3]},
        {SPR_EEL_FIRE, 'C', 0|SF_QUICK_CALL,  &AF(InitEelFire),          &s_EelAttack[4]},
        {SPR_EEL_FIRE, 'C', EEL_FIRE_RATE,    &AF(NullEel),              &s_EelAttack[5]},
        {SPR_EEL_FIRE, 'D', 0|SF_QUICK_CALL,  &AF(InitActorDecide),      &s_EelAttack[6]},
        {SPR_EEL_RUN, 'D', EEL_FIRE_RATE,    &AF(DoEelMove),            &s_EelAttack[6]}
};

//////////////////////
//
// EEL DIE
//
//////////////////////

#define EEL_DIE_RATE 20

FState s_EelDie[] =
{
    {SPR_EEL_DIE, 'A', EEL_DIE_RATE, &AF(DoEelDeath), &s_EelDie[1]},
    {SPR_EEL_DIE, 'A', EEL_DIE_RATE, &AF(DoEelDeath), &s_EelDie[2]},
    {SPR_EEL_DIE, 'A', EEL_DIE_RATE, &AF(DoEelDeath), &s_EelDie[3]},
    {SPR_EEL_DIE, 'A', EEL_DIE_RATE, &AF(DoEelDeath), &s_EelDie[4]},
    {SPR_EEL_DIE, 'A', EEL_DIE_RATE, &AF(DoEelDeath), &s_EelDie[5]},
    {SPR_EEL_DIE, 'A', EEL_DIE_RATE, &AF(DoEelDeath), &s_EelDie[5]},
};

FState s_EelDead[] =
{
    {SPR_EEL_DEAD, 'A', EEL_DIE_RATE, &AF(DoActorDebris), &s_EelDead[0]},
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


ACTOR_ACTION_SET EelActionSet =
{
    s_EelStand,
    s_EelRun,
    nullptr,
    nullptr,
    nullptr,
    s_EelRun,
    nullptr,
    nullptr,
    s_EelStand,
    nullptr,
    nullptr, //climb
    s_EelStand, //pain
    s_EelDie,
    nullptr,
    s_EelDead,
    nullptr,
    nullptr,
    {s_EelAttack},
    {1024},
    {s_EelAttack},
    {1024},
    {nullptr,nullptr},
    nullptr,
    nullptr
};

int DoEelMatchPlayerZ(DSWActor* actor);


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void EelCommon(DSWActor* actor)
{
    actor->clipdist = 6.25;
    actor->user.floor_dist = (16);
    actor->user.floor_dist = (16);
    actor->user.ceiling_dist = (20);

    actor->user.pos.Z = actor->spr.pos.Z;

	actor->spr.scale = DVector2(0.546875, 0.421875);
    actor->user.Radius = 400;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int SetupEel(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,EEL_RUN_R0,s_EelRun[0]);
        actor->user.Health = 40;
    }

    actor->user.__legacyState.Attrib = &EelAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.__legacyState.StateEnd = s_EelDie;

    EnemyDefaults(actor, &EelActionSet, &EelPersonality);
    actor->setStateGroup(NAME_Run);
    actor->setPicFromState();

    actor->user.Flags |= (SPR_NO_SCAREDZ|SPR_XFLIP_TOGGLE);

    EelCommon(actor);

    actor->user.Flags &= ~(SPR_SHADOW); // Turn off shadows
    actor->user.zclip = (8);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWEel, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupEel(self);
    return 0;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int NullEel(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    DoEelMatchPlayerZ(actor);

    DoActorSectorDamage(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoEelMatchPlayerZ(DSWActor* actor)
{
    if (FAF_ConnectArea(actor->sector()))
    {
        if (actor->user.hi_sectp)
        {
            actor->user.hiz = actor->sector()->ceilingz + 16;
            actor->user.hi_sectp = actor->sector();
        }
        else
        {
            if (actor->user.hiz < actor->sector()->ceilingz + 16)
                actor->user.hiz = actor->sector()->ceilingz + 16;
        }
    }

    // actor does a sine wave about actor->user.sz - this is the z mid point

    double zdiff = ActorZOfBottom(actor->user.targetActor) - 8 - actor->user.pos.Z;

    // check z diff of the player and the sprite
    double zdist = 20 + RandomRange(64); // put a random amount
    if (abs(zdiff) > zdist)
    {
        if (zdiff > 0)
            // manipulate the z midpoint
            actor->user.pos.Z += 160 * ACTORMOVETICS * zmaptoworld;
        else
            actor->user.pos.Z -= 160 * ACTORMOVETICS * zmaptoworld;
    }

    const int EEL_BOB_AMT = 4;

    // save off lo and hi z
    double loz = actor->user.loz;
    double hiz = actor->user.hiz;

    // adjust loz/hiz for water depth
    if (actor->user.lo_sectp && actor->user.lo_sectp->hasU() && FixedToInt(actor->user.lo_sectp->depth_fixed))
        loz -= FixedToInt(actor->user.lo_sectp->depth_fixed) - 8;

    // lower bound
    double bound;
    if (actor->user.lowActor && actor->user.targetActor == actor->user.highActor) // this doesn't look right...
    {
        double dist = (actor->spr.pos.XY() - actor->user.lowActor->spr.pos.XY()).Length();
        if (dist <= 18.75)
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
        double dist = (actor->spr.pos.XY() - actor->user.highActor->spr.pos.XY()).Length();
        if (dist <= 18.75)
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
    actor->spr.pos.Z = actor->user.pos.Z + EEL_BOB_AMT * BobVal(actor->user.Counter);

    bound = actor->user.hiz + actor->user.ceiling_dist + EEL_BOB_AMT;
    if (actor->spr.pos.Z < bound)
    {
        // bumped something
        actor->spr.pos.Z = bound + EEL_BOB_AMT;
        actor->user.pos.Z = actor->spr.pos.Z;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoEelDeath(DSWActor* actor)
{
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
    auto vec = actor->spr.Angles.Yaw.ToVector() * actor->vel.X;

    actor->user.coll = move_sprite(actor, DVector3(vec, 0), actor->user.ceiling_dist, actor->user.floor_dist, CLIPMASK_MISSILE, ACTORMOVETICS);
    DoFindGroundPoint(actor);

    // on the ground
    if (actor->spr.pos.Z >= actor->user.loz)
    {
        actor->user.Flags &= ~(SPR_FALLING|SPR_SLIDING);
        if (RandomRange(1000) > 500)
            actor->spr.cstat |= (CSTAT_SPRITE_XFLIP);
        if (RandomRange(1000) > 500)
            actor->spr.cstat |= (CSTAT_SPRITE_YFLIP);
        actor->setStateGroup(NAME_Dead);
        return 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoEelMove(DSWActor* actor)
{
    ASSERT(actor->user.__legacyState.Rot != nullptr);

    if (SpriteOverlap(actor, actor->user.targetActor))
        actor->setStateGroup(NAME_CloseAttack, 0);

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        actor->callAction();

    DoEelMatchPlayerZ(actor);

    DoActorSectorDamage(actor);

    return 0;

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------


#include "saveable.h"

static saveable_data saveable_eel_data[] =
{
    SAVE_DATA(EelPersonality),

    SAVE_DATA(EelAttrib),

    SAVE_DATA(s_EelRun),
    SAVE_DATA(s_EelStand),
    SAVE_DATA(s_EelAttack),
    SAVE_DATA(s_EelDie),
    SAVE_DATA(s_EelDead),

    SAVE_DATA(EelActionSet)
};

saveable_module saveable_eel =
{
    // code
    nullptr, 0,

    // data
    saveable_eel_data,
    SIZ(saveable_eel_data)
};
END_SW_NS
