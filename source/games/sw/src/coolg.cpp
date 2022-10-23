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
#include "sprite.h"
#include "weapon.h"
#include "misc.h"

BEGIN_SW_NS

const int COOLG_BOB_AMT = 8;

DECISION CoolgBattle[] =
{
    {50,    &AF(InitCoolgCircle    )         },
    {450,   &AF(InitActorMoveCloser)         },
    {1024,  &AF(InitActorAttack    )         }
};

DECISION CoolgOffense[] =
{
    {449,   &AF(InitActorMoveCloser)         },
    {1024,  &AF(InitActorAttack    )         }
};

DECISIONB CoolgBroadcast[] =
{
    {1,    attr_ambient       },
    {1024, 0            }
};

DECISION CoolgSurprised[] =
{
    {100,   &AF(InitCoolgCircle    )        },
    {701,   &AF(InitActorMoveCloser)        },
    {1024,  &AF(InitActorDecide    )        }
};

DECISION CoolgEvasive[] =
{
    {20,     &AF(InitCoolgCircle)           },
    {1024,   &AF(InitActorRunAway)          },
};

DECISION CoolgLostTarget[] =
{
    {900,   &AF(InitActorFindPlayer   )      },
    {1024,  &AF(InitActorWanderAround)       }
};

DECISION CoolgCloseRange[] =
{
    {800,   &AF(InitActorAttack    )         },
    {1024,  &AF(InitActorReposition)         }
};

DECISION CoolgTouchTarget[] =
{
    //{50,   InitCoolgCircle            },
    {1024,  &AF(InitActorAttack)            },
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

FState s_CoolgRun[] =
{
        {SPR_COOLG_RUN, 'A', COOLG_RUN_RATE, &AF(DoCoolgMove), &s_CoolgRun[1]},
        {SPR_COOLG_RUN, 'B', COOLG_RUN_RATE, &AF(DoCoolgMove), &s_CoolgRun[2]},
        {SPR_COOLG_RUN, 'C', COOLG_RUN_RATE, &AF(DoCoolgMove), &s_CoolgRun[3]},
        {SPR_COOLG_RUN, 'D', COOLG_RUN_RATE, &AF(DoCoolgMove), &s_CoolgRun[0]},
};

//////////////////////
//
// COOLG STAND
//
//////////////////////


FState s_CoolgStand[] =
{
        {SPR_COOLG_RUN, 'A', COOLG_RUN_RATE, &AF(DoCoolgMove), &s_CoolgStand[0]},
};

//////////////////////
//
// COOLG CLUB
//
//////////////////////

#define COOLG_RATE 16

FState s_CoolgClub[] =
{
        {SPR_COOLG_CLUB, 'A', COOLG_RATE, &AF(NullCoolg), &s_CoolgClub[1]},
        {SPR_COOLG_RUN, 'A', COOLG_RATE, &AF(NullCoolg), &s_CoolgClub[2]},
        {SPR_COOLG_CLUB, 'B', 0|SF_QUICK_CALL, &AF(InitCoolgBash), &s_CoolgClub[3]},
        {SPR_COOLG_CLUB, 'B', COOLG_RATE, &AF(NullCoolg), &s_CoolgClub[4]},
        {SPR_COOLG_CLUB, 'B', 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_CoolgClub[5]},
        {SPR_COOLG_CLUB, 'B', COOLG_RATE, &AF(DoCoolgMove), &s_CoolgClub[5]}
};

//////////////////////
//
// COOLG FIRE
//
//////////////////////

#define COOLG_FIRE_RATE 12

FState s_CoolgAttack[] =
{
        {SPR_COOLG_FIRE, 'A', COOLG_FIRE_RATE*2,  &AF(NullCoolg),              &s_CoolgAttack[1]},
        {SPR_COOLG_FIRE, 'B', COOLG_FIRE_RATE*2,  &AF(NullCoolg),              &s_CoolgAttack[2]},
        {SPR_COOLG_FIRE, 'C', COOLG_FIRE_RATE*2,  &AF(NullCoolg),              &s_CoolgAttack[3]},
        {SPR_COOLG_FIRE, 'C', 0|SF_QUICK_CALL,    &AF(InitCoolgFire),          &s_CoolgAttack[4]},
        {SPR_COOLG_FIRE, 'C', COOLG_FIRE_RATE,    &AF(NullCoolg),              &s_CoolgAttack[5]},
        {SPR_COOLG_FIRE, 'C', 0|SF_QUICK_CALL,    &AF(InitActorDecide),        &s_CoolgAttack[6]},
        {SPR_COOLG_RUN, 'C', COOLG_FIRE_RATE,    &AF(DoCoolgMove),            &s_CoolgAttack[6]}
};

//////////////////////
//
// COOLG PAIN
//
//////////////////////

#define COOLG_PAIN_RATE 15

FState s_CoolgPain[] =
{
        {SPR_COOLG_PAIN, 'A', COOLG_PAIN_RATE, &AF(DoCoolgPain), &s_CoolgPain[1]},
        {SPR_COOLG_PAIN, 'A', COOLG_PAIN_RATE, &AF(DoCoolgPain), &s_CoolgPain[1]},
};

//////////////////////
//
// COOLG DIE
//
//////////////////////

#define COOLG_DIE_RATE 20

FState s_CoolgDie[] =
{
    {SPR_COOLG_DIE, 'A', COOLG_DIE_RATE, &AF(DoCoolgDeath), &s_CoolgDie[1]},
    {SPR_COOLG_DIE, 'B', COOLG_DIE_RATE, &AF(DoCoolgDeath), &s_CoolgDie[2]},
    {SPR_COOLG_DIE, 'C', COOLG_DIE_RATE, &AF(DoCoolgDeath), &s_CoolgDie[3]},
    {SPR_COOLG_DIE, 'D', COOLG_DIE_RATE, &AF(DoCoolgDeath), &s_CoolgDie[4]},
    {SPR_COOLG_DIE, 'E', COOLG_DIE_RATE, &AF(DoCoolgDeath), &s_CoolgDie[5]},
    {SPR_COOLG_DIE, 'F', COOLG_DIE_RATE, &AF(DoCoolgDeath), &s_CoolgDie[5]},
};

FState s_CoolgDead[] =
{
    {SPR_COOLG_DEAD, 'A', SF_QUICK_CALL, &AF(QueueFloorBlood), &s_CoolgDead[1]},
    {SPR_COOLG_DEAD, 'A', COOLG_DIE_RATE, &AF(DoActorDebris), &s_CoolgDead[1]},
};

//////////////////////
//
// COOLG BIRTH
//
//////////////////////

#define COOLG_BIRTH_RATE 20

FState s_CoolgBirth[] =
{
    {SPR_COOLG_BIRTH, 'A', COOLG_BIRTH_RATE, nullptr,  &s_CoolgBirth[1]},
    {SPR_COOLG_BIRTH, 'B', COOLG_BIRTH_RATE, nullptr,  &s_CoolgBirth[2]},
    {SPR_COOLG_BIRTH, 'C', COOLG_BIRTH_RATE, nullptr,  &s_CoolgBirth[3]},
    {SPR_COOLG_BIRTH, 'D', COOLG_BIRTH_RATE, nullptr,  &s_CoolgBirth[4]},
    {SPR_COOLG_BIRTH, 'E', COOLG_BIRTH_RATE, nullptr,  &s_CoolgBirth[5]},
    {SPR_COOLG_BIRTH, 'F', COOLG_BIRTH_RATE, nullptr,  &s_CoolgBirth[6]},
    {SPR_COOLG_BIRTH, 'G', COOLG_BIRTH_RATE, nullptr,  &s_CoolgBirth[7]},
    {SPR_COOLG_BIRTH, 'H', COOLG_BIRTH_RATE, nullptr,  &s_CoolgBirth[8]},
    {SPR_COOLG_BIRTH, 'I', COOLG_BIRTH_RATE, nullptr,  &s_CoolgBirth[9]},
    {SPR_COOLG_BIRTH, 'I', COOLG_BIRTH_RATE, nullptr,  &s_CoolgBirth[10]},
    {SPR_COOLG_BIRTH, 'I', 0 | SF_QUICK_CALL, &AF(DoCoolgBirth), &s_CoolgBirth[10]}
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


ACTOR_ACTION_SET CoolgActionSet =
{
    s_CoolgStand,
    s_CoolgRun,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr, //climb
    s_CoolgPain, //pain
    s_CoolgDie,
    nullptr,
    s_CoolgDead,
    nullptr,
    nullptr,
//  {s_CoolgClub},
    {s_CoolgAttack},
    {1024},
    {s_CoolgAttack},
    {1024},
    {nullptr,nullptr},
    nullptr,
    nullptr
};

int DoCoolgMatchPlayerZ(DSWActor* actor);

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void CoolgCommon(DSWActor* actor)
{
    actor->clipdist = 12.5;
    actor->user.floor_dist = (16);
    actor->user.ceiling_dist = (20);

    actor->user.pos.Z = actor->spr.pos.Z;

	actor->spr.scale = DVector2(0.65625, 0.65625);
    actor->spr.extra |= (SPRX_PLAYER_OR_ENEMY);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupCoolg(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,COOLG_RUN_R0,s_CoolgRun[0]);
        actor->user.Health = HEALTH_COOLIE_GHOST;
    }

    actor->user.__legacyState.Attrib = &CoolgAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);

    EnemyDefaults(actor, &CoolgActionSet, &CoolgPersonality);

    actor->setStateGroup(NAME_Run);
    actor->setPicFromState();
    actor->user.__legacyState.StateEnd = s_CoolgDie;

    actor->user.Flags |= (SPR_NO_SCAREDZ|SPR_XFLIP_TOGGLE);

    CoolgCommon(actor);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWCoolg, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupCoolg(self);
    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NewCoolg(DSWActor* actor)
{
    auto actorNew = SpawnActor(STAT_ENEMY, COOLG_RUN_R0, &s_CoolgBirth[0], actor->sector(), actor->spr.pos, actor->spr.Angles.Yaw, 50/16.);

    ChangeState(actorNew, &s_CoolgBirth[0]);
    actorNew->user.__legacyState.StateEnd = s_CoolgDie;
    actorNew->setStateGroup(NAME_Run, 0, false);
    actorNew->spr.pal = actorNew->user.spal = actor->user.spal;

    actorNew->user.__legacyState.ActorActionSet = &CoolgActionSet;

    actorNew->spr.shade = actor->spr.shade;
    actorNew->user.Personality = &CoolgPersonality;
    actorNew->user.__legacyState.Attrib = &CoolgAttrib;

    // special case
    Level.addKillCount();
    CoolgCommon(actorNew);

    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoCoolgBirth(DSWActor* actor)
{
    actor->user.Health = HEALTH_COOLIE_GHOST;
    actor->user.__legacyState.Attrib = &CoolgAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);

    actor->setStateGroup(NAME_Run);
    actor->user.__legacyState.StateEnd = s_CoolgDie;

    EnemyDefaults(actor, &CoolgActionSet, &CoolgPersonality);
    // special case
    Level.addKillCount(-1);

    actor->user.Flags |= (SPR_NO_SCAREDZ|SPR_XFLIP_TOGGLE);
    CoolgCommon(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullCoolg(DSWActor* actor)
{
    actor->user.ShellNum -= ACTORMOVETICS;

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    DoCoolgMatchPlayerZ(actor);
    DoActorSectorDamage(actor);
    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoCoolgMatchPlayerZ(DSWActor* actor)
{
    // If blocking bits get unset, just die
    if (!(actor->spr.cstat & CSTAT_SPRITE_BLOCK) || !(actor->spr.cstat & CSTAT_SPRITE_BLOCK_HITSCAN))
    {
        InitBloodSpray(actor, true, 105);
        InitBloodSpray(actor, true, 105);
        UpdateSinglePlayKills(actor);
        SetSuicide(actor);
    }

    // actor does a sine wave about sz - this is the z mid point

    double zdiff = (ActorZOfMiddle(actor->user.targetActor)) - actor->user.pos.Z;

    // check z diff of the player and the sprite
    double zdist = 20 + RandomRange(100); // put a random amount
    //zdist = Z(20);
    if (abs(zdiff) > zdist)
    {
        if (zdiff > 0)
            actor->user.pos.Z += 170 * ACTORMOVETICS * zmaptoworld;
        else
            actor->user.pos.Z -= 170 * ACTORMOVETICS * zmaptoworld;
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
    actor->spr.pos.Z = actor->user.pos.Z + COOLG_BOB_AMT * BobVal(actor->user.Counter);

    bound = actor->user.hiz + actor->user.ceiling_dist + COOLG_BOB_AMT;
    if (actor->spr.pos.Z < bound)
    {
        // bumped something
        actor->spr.pos.Z = actor->user.pos.Z = bound + COOLG_BOB_AMT;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int InitCoolgCircle(DSWActor* actor)
{
    actor->user.ActorActionFunc = AF(DoCoolgCircle);

    actor->setStateGroup(NAME_Run);

    // set it close
    DoActorSetSpeed(actor, FAST_SPEED);

    // set to really fast
    actor->vel.Z = 400 / 256.;
    // angle adjuster
    actor->user.Counter2 = int(actor->vel.X * (16. / 3));
    // random angle direction
    if (RANDOM_P2(1024) < 512)
        actor->user.Counter2 = -actor->user.Counter2;

    // z velocity
    actor->user.jump_speed = 400 + RANDOM_P2(256);
    if (abs(actor->user.pos.Z - actor->user.hiz) < abs(actor->user.pos.Z - actor->user.loz))
        actor->user.jump_speed = -actor->user.jump_speed;

    actor->user.WaitTics = (RandomRange(3)+1) * 120;

    actor->callAction();

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoCoolgCircle(DSWActor* actor)
{
    double bound;

    actor->spr.Angles.Yaw += mapangle(actor->user.Counter2);

    if (!move_actor(actor, DVector3(actor->spr.Angles.Yaw.ToVector() * actor->vel.X, 0)))
    {
        InitActorReposition(actor);
        return 0;
    }

    // move in the z direction
    actor->user.pos.Z -= actor->user.jump_speed * ACTORMOVETICS * JUMP_FACTOR;

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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoCoolgDeath(DSWActor* actor)
{
    actor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);
    actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE);
    actor->spr.scale.X = (0.65625);
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
    auto vec = actor->spr.Angles.Yaw.ToVector() * actor->vel.X;

    actor->user.coll = move_sprite(actor, DVector3(vec, 0), actor->user.ceiling_dist, actor->user.floor_dist, CLIPMASK_MISSILE, ACTORMOVETICS);
    DoFindGroundPoint(actor);

    // on the ground
    if (actor->spr.pos.Z >= actor->user.loz)
    {
        actor->user.Flags &= ~(SPR_FALLING|SPR_SLIDING);
        actor->spr.cstat &= ~(CSTAT_SPRITE_YFLIP); // If upside down, reset it
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
		actor->spr.scale.X -= REPEAT_SCALE;
        actor->spr.shade++;
		if (actor->spr.scale.X < 0.0625) actor->spr.scale.X = (0.0625);
        if (actor->spr.shade > 126)
        {
            actor->spr.shade = 127;
            actor->spr.hitag = 9998;
        }
    }
    else if (actor->user.FlagOwner-1 == 2)
    {
        actor->spr.hitag = 0;
        actor->spr.scale.X += (REPEAT_SCALE);
        actor->spr.shade--;
        if (actor->spr.scale.X > 0.65625) actor->spr.scale.X = (0.65625);
        if (actor->spr.shade < -10) actor->spr.shade = -10;
    }
    else if (actor->user.FlagOwner == 0)
    {
        actor->spr.scale.X = (0.65625);
        actor->spr.shade = -10;
        actor->spr.hitag = 0;
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
    {
        actor->callAction();
    }

    if (RANDOM_P2(1024) < 32 && !(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
        InitCoolgDrip(actor);

    DoCoolgMatchPlayerZ(actor);

    DoActorSectorDamage(actor);


    return 0;

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoCoolgPain(DSWActor* actor)
{
    NullCoolg(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(actor);

    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

#include "saveable.h"

static saveable_data saveable_coolg_data[] =
{
    SAVE_DATA(CoolgPersonality),

    SAVE_DATA(CoolgAttrib),

    SAVE_DATA(s_CoolgRun),
    SAVE_DATA(s_CoolgStand),
    SAVE_DATA(s_CoolgClub),
    SAVE_DATA(s_CoolgAttack),
    SAVE_DATA(s_CoolgPain),
    SAVE_DATA(s_CoolgDie),
    SAVE_DATA(s_CoolgDead),
    SAVE_DATA(s_CoolgBirth),

    SAVE_DATA(CoolgActionSet),
};

saveable_module saveable_coolg =
{
    // code
    nullptr, 0,

    // data
    saveable_coolg_data,
    SIZ(saveable_coolg_data)
};
END_SW_NS
