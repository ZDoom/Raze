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
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "pal.h"
#include "player.h"
#include "network.h"
#include "sprite.h"
#include "weapon.h"
#include "misc.h"

BEGIN_SW_NS

#define ZOMBIE_TIME_LIMIT ((120*20)/ACTORMOVETICS)

DECISION ZombieBattle[] =
{
    {399, &AF(InitActorMoveCloser)},
    {1024, &AF(InitActorAttack)}
};

DECISION ZombieOffense[] =
{
    {399, &AF(InitActorMoveCloser)},
    {1024, &AF(InitActorAttack)}
};

DECISIONB ZombieBroadcast[] =
{
    {6, attr_ambient},
    {1024, 0}
};

DECISION ZombieSurprised[] =
{
    {701, &AF(InitActorMoveCloser)},
    {1024, &AF(InitActorDecide)}
};

DECISION ZombieEvasive[] =
{
    {400,   &AF(InitActorDuck)},
    {1024,  nullptr}
};

DECISION ZombieLostTarget[] =
{
    {900, &AF(InitActorFindPlayer)},
    {1024, &AF(InitActorWanderAround)}
};

DECISION ZombieCloseRange[] =
{
    {800,   &AF(InitActorAttack    )         },
    {1024,  &AF(InitActorReposition)         }
};

PERSONALITY ZombiePersonality =
{
    ZombieBattle,
    ZombieOffense,
    ZombieBroadcast,
    ZombieSurprised,
    ZombieEvasive,
    ZombieLostTarget,
    ZombieCloseRange,
    ZombieCloseRange
};

ATTRIBUTE ZombieAttrib =
{
    //{50, 60, 70, 80},                   // Speeds
    {120, 140, 170, 200},                 // Speeds
    {4, 0, 0, -2},                      // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        DIGI_NINJAAMBIENT, DIGI_NINJAALERT, DIGI_STAR,
        DIGI_NINJAPAIN, DIGI_NINJASCREAM,0,0,0,0,0
    }
};


//////////////////////
//
// ZOMBIE RUN
//
//////////////////////

#define ZOMBIE_RATE 32

FState s_ZombieRun[] =
{
        {SPR_PLAYER_NINJA_RUN, 'A', ZOMBIE_RATE | SF_TIC_ADJUST, &AF(DoZombieMove), &s_ZombieRun[1]},
        {SPR_PLAYER_NINJA_RUN, 'B', ZOMBIE_RATE | SF_TIC_ADJUST, &AF(DoZombieMove), &s_ZombieRun[2]},
        {SPR_PLAYER_NINJA_RUN, 'C', ZOMBIE_RATE | SF_TIC_ADJUST, &AF(DoZombieMove), &s_ZombieRun[3]},
        {SPR_PLAYER_NINJA_RUN, 'D', ZOMBIE_RATE | SF_TIC_ADJUST, &AF(DoZombieMove), &s_ZombieRun[0]},
};

//////////////////////
//
// PLAYER_ZOMBIE STAND
//
//////////////////////

#define ZOMBIE_STAND_RATE 10

FState s_ZombieStand[] =
{
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_STAND_RATE, &AF(NullZombie), &s_ZombieStand[0]},
};

//////////////////////
//
// ZOMBIE PAIN
//
//////////////////////

#define ZOMBIE_PAIN_RATE 15

FState s_ZombiePain[] =
{
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_PAIN_RATE, &AF(DoZombiePain), &s_ZombiePain[1]},
        {SPR_PLAYER_NINJA_STAND, 'B', ZOMBIE_PAIN_RATE, &AF(DoZombiePain), &s_ZombiePain[1]},
};

//////////////////////
//
// ZOMBIE NUKE
//
//////////////////////

#define ZOMBIE_NUKE_RATE 18

FState s_ZombieNuke[] =
{
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_NUKE_RATE * 2, &AF(NullZombie), &s_ZombieNuke[1]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_NUKE_RATE, &AF(NullZombie), &s_ZombieNuke[2]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyNuke), &s_ZombieNuke[3]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_NUKE_RATE * 2, &AF(NullZombie), &s_ZombieNuke[4]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_ZombieNuke[5]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_NUKE_RATE, &AF(DoZombieMove), &s_ZombieNuke[5]},
};

//////////////////////
//
// ZOMBIE ROCKET
//
//////////////////////

#define ZOMBIE_ROCKET_RATE 14

FState s_ZombieRocket[] =
{
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_ROCKET_RATE * 2, &AF(NullZombie), &s_ZombieRocket[1]},
        {SPR_PLAYER_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyRocket), &s_ZombieRocket[2]},
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_ROCKET_RATE, &AF(NullZombie), &s_ZombieRocket[3]},
        {SPR_PLAYER_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_ZombieRocket[4]},
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_ROCKET_RATE, &AF(DoZombieMove), &s_ZombieRocket[4]},
};


//////////////////////
//
// ZOMBIE ROCKET
//
//////////////////////

#define ZOMBIE_RAIL_RATE 14

FState s_ZombieRail[] =
{
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_RAIL_RATE * 2, &AF(NullZombie), &s_ZombieRail[1]},
        {SPR_PLAYER_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyRail), &s_ZombieRail[2]},
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_RAIL_RATE, &AF(NullZombie), &s_ZombieRail[3]},
        {SPR_PLAYER_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_ZombieRail[4]},
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_RAIL_RATE, &AF(DoZombieMove), &s_ZombieRail[4]},
};


//////////////////////
//
// ZOMBIE GRENADE
//
//////////////////////

#define ZOMBIE_ROCKET_RATE 14

FState s_ZombieGrenade[] =
{
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_ROCKET_RATE * 2, &AF(NullZombie), &s_ZombieGrenade[1]},
        {SPR_PLAYER_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitSpriteGrenade), &s_ZombieGrenade[2]},
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_ROCKET_RATE, &AF(NullZombie), &s_ZombieGrenade[3]},
        {SPR_PLAYER_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_ZombieGrenade[4]},
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_ROCKET_RATE, &AF(DoZombieMove), &s_ZombieGrenade[4]},
};



//////////////////////
//
// ZOMBIE FLASHBOMB
//
//////////////////////

#define ZOMBIE_FLASHBOMB_RATE 14

FState s_ZombieFlashBomb[] =
{
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_FLASHBOMB_RATE * 2, &AF(NullZombie), &s_ZombieFlashBomb[1]},
        {SPR_PLAYER_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitFlashBomb), &s_ZombieFlashBomb[2]},
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_FLASHBOMB_RATE, &AF(NullZombie), &s_ZombieFlashBomb[3]},
        {SPR_PLAYER_NINJA_STAND, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_ZombieFlashBomb[4]},
        {SPR_PLAYER_NINJA_STAND, 'A', ZOMBIE_FLASHBOMB_RATE, &AF(DoZombieMove), &s_ZombieFlashBomb[4]},
};


//////////////////////
//
// ZOMBIE UZI
//
//////////////////////

#define ZOMBIE_UZI_RATE 8

FState s_ZombieUzi[] =
{
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_UZI_RATE, &AF(NullZombie), &s_ZombieUzi[1]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(CheckFire), &s_ZombieUzi[2]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_UZI_RATE, &AF(NullZombie), &s_ZombieUzi[3]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZombieUzi[4]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_UZI_RATE, &AF(NullZombie), &s_ZombieUzi[5]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZombieUzi[6]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_UZI_RATE, &AF(NullZombie), &s_ZombieUzi[7]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZombieUzi[8]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_UZI_RATE, &AF(NullZombie), &s_ZombieUzi[9]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZombieUzi[10]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_UZI_RATE, &AF(NullZombie), &s_ZombieUzi[11]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZombieUzi[12]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_UZI_RATE, &AF(NullZombie), &s_ZombieUzi[13]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZombieUzi[14]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', ZOMBIE_UZI_RATE, &AF(NullZombie), &s_ZombieUzi[15]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZombieUzi[16]},
        {SPR_PLAYER_NINJA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_ZombieUzi[16]},
};


//////////////////////
//
// ZOMBIE FALL
//
//////////////////////

#define ZOMBIE_FALL_RATE 25

FState s_ZombieFall[] =
{
        {SPR_PLAYER_NINJA_JUMP, 'D', ZOMBIE_FALL_RATE, &AF(DoZombieMove), &s_ZombieFall[0]},
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

ACTOR_ACTION_SET ZombieActionSet =
{
    s_ZombieStand,
    s_ZombieRun,
    nullptr,
    s_ZombieFall,
    nullptr,
    nullptr,
    nullptr,
    s_ZombieRun,
    s_ZombieRun,
    nullptr,
    nullptr,
    s_ZombiePain,
    s_ZombieRun,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
#if 0
    {s_ZombieUzi},
    {1024},
    {s_ZombieUzi, s_ZombieRocket, s_ZombieGrenade, s_ZombieNuke},
    {400, 750, 1000, 1024},
#endif
    {s_ZombieRail},
    {1024},
    {s_ZombieRail},
    {1024},
    {nullptr},
    nullptr,
    nullptr
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupZombie(DSWActor* actor)
{

    actor->user.Health = 100;
    actor->user.__legacyState.StateEnd = &s_ZombiePain[0];
	actor->spr.scale = DVector2(PLAYER_NINJA_XREPEAT, PLAYER_NINJA_YREPEAT);

    actor->user.__legacyState.Attrib = &ZombieAttrib;
    EnemyDefaults(actor, &ZombieActionSet, &ZombiePersonality);

    actor->setStateGroup(NAME_Run);
    actor->setPicFromState();
    DoActorSetSpeed(actor, NORM_SPEED);

    actor->user.Radius = 280;
    actor->user.Flags |= (SPR_XFLIP_TOGGLE);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SpawnZombie(DSWPlayer* pp, DSWActor* weaponActor)
{
    auto ownerActor = GetOwner(weaponActor);

    if (ownerActor == nullptr)
        return;

    auto actorNew = SpawnActor(STAT_ENEMY, ZOMBIE_RUN_R0, &s_ZombieRun[0], pp->cursector, pp->GetActor()->getPosWithOffsetZ(), pp->GetActor()->spr.Angles.Yaw, 0);
    SetOwner(actorNew, ownerActor);
    actorNew->spr.pal = actorNew->user.spal = ownerActor->user.spal;
    actorNew->spr.Angles.Yaw = RandomAngle();
    SetupZombie(actorNew);
    actorNew->spr.shade = -10;
    actorNew->user.Flags2 |= (SPR2_DONT_TARGET_OWNER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT);

    DoActorPickClosePlayer(actorNew);

    // make immediately active
    actorNew->user.Flags |= (SPR_ACTIVE);

    actorNew->user.Flags &= ~(SPR_JUMPING);
    actorNew->user.Flags &= ~(SPR_FALLING);

    // if I didn't do this here they get stuck in the air sometimes
    DoActorZrange(actorNew);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SpawnZombie2(DSWActor* actor)
{
    auto sectu = actor->sector();
    sectortype* sectp = actor->sector();

    auto ownerActor = GetOwner(actor);

    if (ownerActor == nullptr)
        return;

    if (sectu && ((sectp->extra & SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE))
        return;

    if (SectorIsUnderwaterArea(actor->sector()))
        return;

    if (FAF_ConnectArea(actor->sector()))
    {
        auto newsect = actor->sector();
        updatesectorz(actor->spr.pos.plusZ(10), &newsect);
        if (SectorIsUnderwaterArea(newsect))
            return;
    }


    auto actorNew = SpawnActor(STAT_ENEMY, ZOMBIE_RUN_R0, &s_ZombieRun[0], actor->sector(), actor->spr.pos, actor->spr.Angles.Yaw, 0);
    actorNew->user.Counter3 = 0;
    SetOwner(ownerActor, actorNew);
    actorNew->spr.pal = actorNew->user.spal = ownerActor->user.spal;
    actorNew->spr.Angles.Yaw = RandomAngle();
    SetupZombie(actorNew);
    actorNew->spr.shade = -10;
    actorNew->user.Flags2 |= (SPR2_DONT_TARGET_OWNER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT);

    DoActorPickClosePlayer(actorNew);

    // make immediately active
    actorNew->user.Flags |= (SPR_ACTIVE);

    actorNew->user.Flags &= ~(SPR_JUMPING);
    actorNew->user.Flags &= ~(SPR_FALLING);

    // if I didn't do this here they get stuck in the air sometimes
    DoActorZrange(actorNew);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoZombieMove(DSWActor* actor)
{
    if (actor->user.Counter3++ >= ZOMBIE_TIME_LIMIT)
    {
        InitBloodSpray(actor,true,105);
        InitBloodSpray(actor,true,105);
        InitBloodSpray(actor,true,105);
        SetSuicide(actor);
        return 0;
    }

    DSWActor* tActor = actor->user.targetActor;
    if (tActor && tActor->hasU())
    {
        if ((tActor->user.Flags & PF_DEAD))
            DoActorPickClosePlayer(actor);
    }

    // jumping and falling
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else if (actor->user.Flags & (SPR_FALLING))
            DoActorFall(actor);
    }

    // sliding
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    // Do track or call current action function - such as DoActorMoveCloser()
    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
    {
        actor->callAction();
    }

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
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

int NullZombie(DSWActor* actor)
{
    if (actor->user.Counter3++ >= ZOMBIE_TIME_LIMIT)
    {
        InitBloodSpray(actor,true,105);
        InitBloodSpray(actor,true,105);
        InitBloodSpray(actor,true,105);
        SetSuicide(actor);
        return 0;
    }

    DSWActor* tActor = actor->user.targetActor;
    if (tActor && tActor->hasU())
    {
        if ((tActor->user.Flags & PF_DEAD))
            DoActorPickClosePlayer(actor);
    }

    if (actor->user.WaitTics > 0)
        actor->user.WaitTics -= ACTORMOVETICS;

    if (actor->user.Flags & (SPR_SLIDING) && !(actor->user.Flags & (SPR_JUMPING|SPR_FALLING)))
        DoActorSlide(actor);

    if (!(actor->user.Flags & (SPR_JUMPING|SPR_FALLING)))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoZombiePain(DSWActor* actor)
{
    NullZombie(actor);

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

static saveable_data saveable_zombie_data[] =
{
    SAVE_DATA(ZombiePersonality),

    SAVE_DATA(ZombieAttrib),

    SAVE_DATA(ZombieActionSet),
};

saveable_module saveable_zombie =
{
    // code
    nullptr, 0,

    // data
    saveable_zombie_data,
    SIZ(saveable_zombie_data)
};
END_SW_NS
