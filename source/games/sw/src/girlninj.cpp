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
#include "misc.h"
#include "sprite.h"

BEGIN_SW_NS

DECISION GirlNinjaBattle[] =
{
    {499, &AF(InitActorMoveCloser)},
    {1024, &AF(InitActorAttack)}
};

DECISION GirlNinjaOffense[] =
{
    {499, &AF(InitActorMoveCloser)},
    {1024, &AF(InitActorAttack)}
};

DECISIONB GirlNinjaBroadcast[] =
{
    {6, attr_ambient},
    {1024, 0}
};

DECISION GirlNinjaSurprised[] =
{
    {701, &AF(InitActorMoveCloser)},
    {1024, &AF(InitActorDecide)}
};

DECISION GirlNinjaEvasive[] =
{
    {400,   &AF(InitActorDuck)}, // 100
    {1024,  nullptr}
};

DECISION GirlNinjaLostTarget[] =
{
    {900, &AF(InitActorFindPlayer)},
    {1024, &AF(InitActorWanderAround)}
};

DECISION GirlNinjaCloseRange[] =
{
    {900,   &AF(InitActorAttack    )         },
    {1024,  &AF(InitActorReposition)         }
};

/*

 !AIC - Collection of decision tables

*/

PERSONALITY GirlNinjaPersonality =
{
    GirlNinjaBattle,
    GirlNinjaOffense,
    GirlNinjaBroadcast,
    GirlNinjaSurprised,
    GirlNinjaEvasive,
    GirlNinjaLostTarget,
    GirlNinjaCloseRange,
    GirlNinjaCloseRange
};


ATTRIBUTE GirlNinjaAttrib =
{
    {120, 140, 160, 190},               // Speeds
    {4, 0, 0, -2},                      // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        DIGI_GIRLNINJAALERT, DIGI_GIRLNINJAALERT, DIGI_STAR,
        DIGI_GIRLNINJAALERTT, DIGI_GIRLNINJASCREAM,0,0,0,0,0
    }
};

//////////////////////
//
// GIRLNINJA RUN
//
//////////////////////

#define GIRLNINJA_RATE 18

FState s_GirlNinjaRun[] = 
{
        {SPR_GIRLNINJA_RUN, 'A', GIRLNINJA_RATE | SF_TIC_ADJUST, &AF(DoGirlNinjaMove), &s_GirlNinjaRun[1]},
        {SPR_GIRLNINJA_RUN, 'B', GIRLNINJA_RATE | SF_TIC_ADJUST, &AF(DoGirlNinjaMove), &s_GirlNinjaRun[2]},
        {SPR_GIRLNINJA_RUN, 'C', GIRLNINJA_RATE | SF_TIC_ADJUST, &AF(DoGirlNinjaMove), &s_GirlNinjaRun[3]},
        {SPR_GIRLNINJA_RUN, 'D', GIRLNINJA_RATE | SF_TIC_ADJUST, &AF(DoGirlNinjaMove), &s_GirlNinjaRun[0]},
};

//////////////////////
//
// GIRLNINJA STAND
//
//////////////////////

#define GIRLNINJA_STAND_RATE 10

FState s_GirlNinjaStand[] = 
{
        {SPR_GIRLNINJA_STAND, 'A', GIRLNINJA_STAND_RATE, &AF(DoGirlNinjaMove), &s_GirlNinjaStand[0]},
};


//////////////////////
//
// GIRLNINJA RISE
//
//////////////////////

#define GIRLNINJA_RISE_RATE 10

FState s_GirlNinjaRise[] = 
{
        {SPR_GIRLNINJA_KNEEL, 'A', GIRLNINJA_RISE_RATE, &AF(NullGirlNinja), &s_GirlNinjaRise[1]},
        {SPR_GIRLNINJA_STAND, 'A', GIRLNINJA_STAND_RATE, &AF(NullGirlNinja), &s_GirlNinjaRise[2]},
        {SPR_NULL, 0, 0, nullptr, &s_GirlNinjaRun[0]},  // JBF: s_GirlNinjaRun really is supposed to be the
        // pointer to the state group. See StateControl() where
        // it says "if (!actor->user.State->Pic)".
};


//////////////////////
//
// GIRLNINJA DUCK
//
//////////////////////

#define GIRLNINJA_DUCK_RATE 10
#define GIRLNINJA_CRAWL_RATE 14

FState s_GirlNinjaDuck[] = 
{
        {SPR_GIRLNINJA_KNEEL, 'A', GIRLNINJA_DUCK_RATE, &AF(NullGirlNinja), &s_GirlNinjaDuck[1]},
        {SPR_GIRLNINJA_CRAWL, 'A', GIRLNINJA_CRAWL_RATE, &AF(DoGirlNinjaMove), &s_GirlNinjaDuck[1]},
};


//////////////////////
//
// GIRLNINJA SIT
//
//////////////////////

FState s_GirlNinjaSit[] = 
{
        {SPR_GIRLNINJA_KNEEL, 'A', GIRLNINJA_RISE_RATE, &AF(DoGirlNinjaMove), &s_GirlNinjaSit[0]},
};


//////////////////////
//
// GIRLNINJA JUMP
//
//////////////////////

#define GIRLNINJA_JUMP_RATE 24

FState s_GirlNinjaJump[] = 
{
        {SPR_GIRLNINJA_JUMP, 'A', GIRLNINJA_JUMP_RATE, &AF(DoGirlNinjaMove), &s_GirlNinjaJump[1]},
        {SPR_GIRLNINJA_JUMP, 'B', GIRLNINJA_JUMP_RATE, &AF(DoGirlNinjaMove), &s_GirlNinjaJump[1]},
};


//////////////////////
//
// GIRLNINJA FALL
//
//////////////////////

#define GIRLNINJA_FALL_RATE 16

FState s_GirlNinjaFall[] = 
{
        {SPR_GIRLNINJA_JUMP, 'B', GIRLNINJA_FALL_RATE, &AF(DoGirlNinjaMove), &s_GirlNinjaFall[1]},
        {SPR_GIRLNINJA_JUMP, 'C', GIRLNINJA_FALL_RATE, &AF(DoGirlNinjaMove), &s_GirlNinjaFall[1]},
};


//////////////////////
//
// GIRLNINJA PAIN
//
//////////////////////

#define GIRLNINJA_PAIN_RATE 15

FState s_GirlNinjaPain[] = 
{
        {SPR_GIRLNINJA_PAIN, 'A', GIRLNINJA_PAIN_RATE, &AF(DoGirlNinjaPain), &s_GirlNinjaPain[0]},
};

//////////////////////
//
// GIRLNINJA STICKY
//
//////////////////////

#define GIRLNINJA_STICKY_RATE 32

FState s_GirlNinjaSticky[] = 
{
        {SPR_GIRLNINJA_THROW, 'A', GIRLNINJA_STICKY_RATE * 2,     &AF(NullGirlNinja),            &s_GirlNinjaSticky[1]},
        {SPR_GIRLNINJA_THROW, 'A', GIRLNINJA_STICKY_RATE,       &AF(NullGirlNinja),          &s_GirlNinjaSticky[2]},
        {SPR_GIRLNINJA_THROW, 'B', 0 | SF_QUICK_CALL,         &AF(InitEnemyMine),      &s_GirlNinjaSticky[3]},
        {SPR_GIRLNINJA_THROW, 'B', GIRLNINJA_STICKY_RATE * 2,     &AF(NullGirlNinja),            &s_GirlNinjaSticky[4]},
        {SPR_GIRLNINJA_THROW, 'C', 0 | SF_QUICK_CALL,         &AF(InitActorDecide),    &s_GirlNinjaSticky[5]},
        {SPR_GIRLNINJA_THROW, 'C', GIRLNINJA_STICKY_RATE,       &AF(DoGirlNinjaMove),        &s_GirlNinjaSticky[5]},
};

//////////////////////
//
// GIRLNINJA CROSSBOW
//
//////////////////////

#define GIRLNINJA_CROSSBOW_RATE 14

FState s_GirlNinjaCrossbow[] = 
{
        {SPR_GIRLNINJA_FIRE, 'A', GIRLNINJA_CROSSBOW_RATE * 2, &AF(NullGirlNinja), &s_GirlNinjaCrossbow[1]},
        {SPR_GIRLNINJA_FIRE, 'B', 0 | SF_QUICK_CALL, &AF(InitEnemyCrossbow), &s_GirlNinjaCrossbow[2]},
        {SPR_GIRLNINJA_FIRE, 'B', GIRLNINJA_CROSSBOW_RATE, &AF(NullGirlNinja), &s_GirlNinjaCrossbow[3]},
        {SPR_GIRLNINJA_FIRE, 'B', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_GirlNinjaCrossbow[4]},
        {SPR_GIRLNINJA_FIRE, 'B', GIRLNINJA_CROSSBOW_RATE, &AF(DoGirlNinjaMove), &s_GirlNinjaCrossbow[4]},
};


//////////////////////
//
// GIRLNINJA DIE
//
//////////////////////

#define GIRLNINJA_DIE_RATE 30

FState s_GirlNinjaDie[] =
{
    {SPR_GIRLNINJA_DIE, 'A', GIRLNINJA_DIE_RATE*2, &AF(NullGirlNinja), &s_GirlNinjaDie[1]},
    {SPR_GIRLNINJA_DIE, 'B', GIRLNINJA_DIE_RATE, &AF(NullGirlNinja), &s_GirlNinjaDie[2]},
    {SPR_GIRLNINJA_DIE, 'C', GIRLNINJA_DIE_RATE, &AF(NullGirlNinja), &s_GirlNinjaDie[3]},
    {SPR_GIRLNINJA_DIE, 'D', GIRLNINJA_DIE_RATE, &AF(NullGirlNinja), &s_GirlNinjaDie[4]},
    {SPR_GIRLNINJA_DIE, 'E', GIRLNINJA_DIE_RATE, &AF(NullGirlNinja), &s_GirlNinjaDie[5]},
    {SPR_GIRLNINJA_DIE, 'F', GIRLNINJA_DIE_RATE, &AF(NullGirlNinja), &s_GirlNinjaDie[6]},
    {SPR_GIRLNINJA_DIE, 'G', GIRLNINJA_DIE_RATE, &AF(NullGirlNinja), &s_GirlNinjaDie[7]},
    {SPR_GIRLNINJA_DIE, 'G', SF_QUICK_CALL, &AF(DoGirlNinjaSpecial), &s_GirlNinjaDie[8]},
    {SPR_GIRLNINJA_DIE, 'H', GIRLNINJA_DIE_RATE, &AF(NullGirlNinja), &s_GirlNinjaDie[9]},
    {SPR_GIRLNINJA_DIE, 'I', SF_QUICK_CALL, &AF(QueueFloorBlood), &s_GirlNinjaDie[10]},
    {SPR_GIRLNINJA_DIE, 'I', GIRLNINJA_DIE_RATE, &AF(DoActorDebris), &s_GirlNinjaDie[10]},
};

FState s_GirlNinjaDead[] =
{
    {SPR_GIRLNINJA_DIE, 'G', GIRLNINJA_DIE_RATE, &AF(DoActorDebris), &s_GirlNinjaDead[1]},
    {SPR_GIRLNINJA_DIE, 'H', SF_QUICK_CALL, &AF(DoGirlNinjaSpecial), &s_GirlNinjaDead[2]},
    {SPR_GIRLNINJA_DIE, 'H', GIRLNINJA_DIE_RATE, &AF(DoActorDebris), &s_GirlNinjaDead[3]},
    {SPR_GIRLNINJA_DIE, 'I', SF_QUICK_CALL, &AF(QueueFloorBlood),&s_GirlNinjaDead[4]},
    {SPR_GIRLNINJA_DIE, 'I', GIRLNINJA_DIE_RATE, &AF(DoActorDebris), &s_GirlNinjaDead[4]},
};


FState s_GirlNinjaDeathJump[] =
{
    {SPR_GIRLNINJA_DIE, 'A', GIRLNINJA_DIE_RATE, &AF(DoActorDeathMove), &s_GirlNinjaDeathJump[1]},
    {SPR_GIRLNINJA_DIE, 'B', GIRLNINJA_DIE_RATE, &AF(DoActorDeathMove), &s_GirlNinjaDeathJump[2]},
    {SPR_GIRLNINJA_DIE, 'C', GIRLNINJA_DIE_RATE, &AF(DoActorDeathMove), &s_GirlNinjaDeathJump[2]},
};

FState s_GirlNinjaDeathFall[] =
{
    {SPR_GIRLNINJA_DIE, 'D', GIRLNINJA_DIE_RATE, &AF(DoActorDeathMove), &s_GirlNinjaDeathFall[1]},
    {SPR_GIRLNINJA_DIE, 'E', GIRLNINJA_DIE_RATE, &AF(DoActorDeathMove), &s_GirlNinjaDeathFall[1]},
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

ACTOR_ACTION_SET GirlNinjaActionSet =
{
    s_GirlNinjaStand,
    s_GirlNinjaRun,
    s_GirlNinjaJump,
    s_GirlNinjaFall,
    nullptr,
    nullptr,
    nullptr,
    s_GirlNinjaRise,
    s_GirlNinjaSit,
    nullptr,
    nullptr,
    s_GirlNinjaPain,
    s_GirlNinjaDie,
    nullptr,
    s_GirlNinjaDead,
    s_GirlNinjaDeathJump,
    s_GirlNinjaDeathFall,
    {s_GirlNinjaCrossbow, s_GirlNinjaSticky},
    {800, 1024},
    {s_GirlNinjaCrossbow, s_GirlNinjaSticky},
    {800, 1024},
    {nullptr},
    s_GirlNinjaDuck,
    nullptr
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int SetupGirlNinja(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, GIRLNINJA_RUN_R0, s_GirlNinjaRun[0]);
        actor->user.Health = (Skill < MinEnemySkill - 1) ? 50 : 100;
    }

    actor->user.__legacyState.StateEnd = s_GirlNinjaDie;
	actor->spr.scale = DVector2(0.796875, 0.671875);

    actor->user.__legacyState.Attrib = &GirlNinjaAttrib;
    actor->spr.pal = actor->user.spal = 26;
    EnemyDefaults(actor, &GirlNinjaActionSet, &GirlNinjaPersonality);

    actor->setStateGroup(NAME_Run);
    actor->setPicFromState();
    DoActorSetSpeed(actor, NORM_SPEED);

    actor->user.Radius = 280;
    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWGirlNinja, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupGirlNinja(self);
    return 0;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoGirlNinjaMove(DSWActor* actor)
{
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

int GirlNinjaJumpActionFunc(DSWActor* actor)
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int NullGirlNinja(DSWActor* actor)
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

int DoGirlNinjaPain(DSWActor* actor)
{
    NullGirlNinja(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoGirlNinjaSpecial(DSWActor* actor)
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

#include "saveable.h"

static saveable_data saveable_girlninj_data[] =
{
    SAVE_DATA(GirlNinjaPersonality),

    SAVE_DATA(GirlNinjaAttrib),

    SAVE_DATA(s_GirlNinjaRun),
    SAVE_DATA(s_GirlNinjaStand),
    SAVE_DATA(s_GirlNinjaRise),
    SAVE_DATA(s_GirlNinjaDuck),
    SAVE_DATA(s_GirlNinjaSit),
    SAVE_DATA(s_GirlNinjaJump),
    SAVE_DATA(s_GirlNinjaFall),
    SAVE_DATA(s_GirlNinjaPain),
    SAVE_DATA(s_GirlNinjaSticky),
    SAVE_DATA(s_GirlNinjaCrossbow),
    SAVE_DATA(s_GirlNinjaDie),
    SAVE_DATA(s_GirlNinjaDead),
    SAVE_DATA(s_GirlNinjaDeathJump),
    SAVE_DATA(s_GirlNinjaDeathFall),

    SAVE_DATA(GirlNinjaActionSet),
};

saveable_module saveable_girlninj =
{
    // code
    nullptr, 0,
    // data
    saveable_girlninj_data,
    SIZ(saveable_girlninj_data)
};
END_SW_NS
