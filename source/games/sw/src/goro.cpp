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
#include "tags.h"
#include "game.h"
#include "ai.h"
#include "misc.h"

BEGIN_SW_NS

DECISION GoroBattle[] =
{
    {697,   &AF(InitActorMoveCloser  )       },
    {700,   &AF(InitActorSetDecide)        },
    {1024,  &AF(InitActorAttack      )       }
};

DECISION GoroOffense[] =
{
    {797,   &AF(InitActorMoveCloser )        },
    {800,   &AF(InitActorSetDecide)       },
    {1024,  &AF(InitActorAttack     )        }
};

DECISIONB GoroBroadcast[] =
{
    {3,    attr_ambient          },
    {1024, 0  }
};

DECISION GoroSurprised[] =
{
    {701,   &AF(InitActorMoveCloser)         },
    {1024,  &AF(InitActorDecide    )        }
};

DECISION GoroEvasive[] =
{
    {10,    &AF(InitActorEvade     )       },
    {1024,  &AF(InitActorMoveCloser)       }
};

DECISION GoroLostTarget[] =
{
    {900,   &AF(InitActorFindPlayer  )       },
    {1024,  &AF(InitActorWanderAround)       }
};

DECISION GoroCloseRange[] =
{
    {700,   &AF(InitActorAttack    )         },
    {1024,  &AF(InitActorReposition)         }
};

PERSONALITY GoroPersonality =
{
    GoroBattle,
    GoroOffense,
    GoroBroadcast,
    GoroSurprised,
    GoroEvasive,
    GoroLostTarget,
    GoroCloseRange,
    GoroCloseRange
};

ATTRIBUTE GoroAttrib =
{
    {160, 180, 200, 230},               // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        DIGI_GRDAMBIENT, DIGI_GRDALERT, 0,
        DIGI_GRDPAIN, DIGI_GRDSCREAM, DIGI_GRDSWINGAXE,
        DIGI_GRDFIREBALL,0,0,0
    }
};


//////////////////////
//
// GORO RUN
//
//////////////////////

#define GORO_RUN_RATE 18

FState s_GoroRun[] =
{
        {SPR_GORO_RUN, 'A', GORO_RUN_RATE|SF_TIC_ADJUST, &AF(DoGoroMove), &s_GoroRun[1]},
        {SPR_GORO_RUN, 'B', GORO_RUN_RATE|SF_TIC_ADJUST, &AF(DoGoroMove), &s_GoroRun[2]},
        {SPR_GORO_RUN, 'C', GORO_RUN_RATE|SF_TIC_ADJUST, &AF(DoGoroMove), &s_GoroRun[3]},
        {SPR_GORO_RUN, 'D', GORO_RUN_RATE|SF_TIC_ADJUST, &AF(DoGoroMove), &s_GoroRun[0]},
};


//////////////////////
//
// GORO CHOP
//
//////////////////////

#define GORO_CHOP_RATE 14

FState s_GoroChop[] =
{
        {SPR_GORO_CHOP, 'A', GORO_CHOP_RATE, &AF(NullGoro), &s_GoroChop[1]},
        {SPR_GORO_CHOP, 'B', GORO_CHOP_RATE, &AF(NullGoro), &s_GoroChop[2]},
        {SPR_GORO_CHOP, 'C', GORO_CHOP_RATE, &AF(NullGoro), &s_GoroChop[3]},
        {SPR_GORO_CHOP, 'C', 0|SF_QUICK_CALL, &AF(InitGoroChop), &s_GoroChop[4]},
        {SPR_GORO_CHOP, 'C', GORO_CHOP_RATE, &AF(NullGoro), &s_GoroChop[5]},
        {SPR_GORO_CHOP, 'C', 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_GoroChop[6]},
        {SPR_GORO_CHOP, 'C', GORO_CHOP_RATE, &AF(DoGoroMove), &s_GoroChop[6]},
};



//////////////////////
//
// GORO SPELL
//
//////////////////////

#define GORO_SPELL_RATE 6
#define GORO_SPELL_PAUSE 30

FState s_GoroSpell[] =
{
        {SPR_GORO_SPELL, 'A', GORO_SPELL_PAUSE, &AF(NullGoro),      &s_GoroSpell[1]},
        {SPR_GORO_SPELL, 'B', GORO_SPELL_PAUSE, &AF(NullGoro),      &s_GoroSpell[2]},
        {SPR_GORO_SPELL, 'B', GORO_SPELL_RATE, &AF(InitEnemyFireball), &s_GoroSpell[3]},
        {SPR_GORO_SPELL, 'B', GORO_SPELL_RATE, &AF(NullGoro),      &s_GoroSpell[4]},
        {SPR_GORO_SPELL, 'B', GORO_SPELL_RATE, &AF(InitEnemyFireball), &s_GoroSpell[5]},
        {SPR_GORO_SPELL, 'B', GORO_SPELL_RATE, &AF(NullGoro),      &s_GoroSpell[6]},
        {SPR_GORO_SPELL, 'B', GORO_SPELL_RATE, &AF(InitEnemyFireball), &s_GoroSpell[7]},
        {SPR_GORO_SPELL, 'B', GORO_SPELL_PAUSE, &AF(NullGoro),      &s_GoroSpell[8]},
        {SPR_GORO_SPELL, 'B', 0|SF_QUICK_CALL, &AF(InitActorDecide),   &s_GoroSpell[9]},
        {SPR_GORO_SPELL, 'B', GORO_SPELL_RATE, &AF(DoGoroMove),        &s_GoroSpell[9]},
};


//////////////////////
//
// GORO STAND
//
//////////////////////

#define GORO_STAND_RATE 12

FState s_GoroStand[] =
{
        {SPR_GORO_CHOP, 'A', GORO_STAND_RATE, &AF(DoGoroMove), &s_GoroStand[0]},
};


//////////////////////
//
// GORO PAIN
//
//////////////////////

#define GORO_PAIN_RATE 12

FState s_GoroPain[] =
{
        {SPR_GORO_CHOP, 'A', GORO_PAIN_RATE, &AF(DoGoroPain), &s_GoroPain[0]},
};


//////////////////////
//
// GORO DIE
//
//////////////////////

#define GORO_DIE_RATE 16

FState s_GoroDie[] =
{
    {SPR_GORO_DIE, 'A', GORO_DIE_RATE, &AF(NullGoro), &s_GoroDie[1]},
    {SPR_GORO_DIE, 'B', GORO_DIE_RATE, &AF(NullGoro), &s_GoroDie[2]},
    {SPR_GORO_DIE, 'C', GORO_DIE_RATE, &AF(NullGoro), &s_GoroDie[3]},
    {SPR_GORO_DIE, 'D', GORO_DIE_RATE, &AF(NullGoro), &s_GoroDie[4]},
    {SPR_GORO_DIE, 'E', GORO_DIE_RATE, &AF(NullGoro), &s_GoroDie[5]},
    {SPR_GORO_DIE, 'F', GORO_DIE_RATE, &AF(NullGoro), &s_GoroDie[6]},
    {SPR_GORO_DIE, 'G', GORO_DIE_RATE, &AF(NullGoro), &s_GoroDie[7]},
    {SPR_GORO_DIE, 'H', GORO_DIE_RATE, &AF(NullGoro), &s_GoroDie[8]},
    {SPR_GORO_DIE, 'I', GORO_DIE_RATE, &AF(NullGoro), &s_GoroDie[9]},
    {SPR_GORO_DEAD, 'A', SF_QUICK_CALL, &AF(QueueFloorBlood), &s_GoroDie[10]},
    {SPR_GORO_DEAD, 'A', GORO_DIE_RATE, &AF(DoActorDebris), &s_GoroDie[10]},
};

FState s_GoroDead[] =
{
    {SPR_GORO_DEAD, 'A', GORO_DIE_RATE, &AF(DoActorDebris), &s_GoroDead[0]},
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

ACTOR_ACTION_SET GoroActionSet =
{
    s_GoroStand,
    s_GoroRun,
    nullptr, //s_GoroJump,
    nullptr, //s_GoroFall,
    nullptr, //s_GoroCrawl,
    nullptr, //s_GoroSwim,
    nullptr, //s_GoroFly,
    nullptr, //s_GoroRise,
    nullptr, //s_GoroSit,
    nullptr, //s_GoroLook,
    nullptr, //climb
    s_GoroPain,
    s_GoroDie,
    nullptr, //s_GoroHariKari,
    s_GoroDead,
    nullptr, //s_GoroDeathJump,
    nullptr, //s_GoroDeathFall,
    {s_GoroChop},
    {1024},
    {s_GoroSpell},
    {1024},
    {nullptr,nullptr},
    nullptr,
    nullptr
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int SetupGoro(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, GORO_RUN_R0,s_GoroRun[0]);
        actor->user.Health = HEALTH_GORO;
    }

    actor->user.__legacyState.Attrib = &GoroAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.__legacyState.StateEnd = s_GoroDie;


    EnemyDefaults(actor, &GoroActionSet, &GoroPersonality);
    actor->setStateGroup(NAME_Run);
    actor->setPicFromState();
    actor->clipdist = 32;
    actor->user.Flags |= (SPR_XFLIP_TOGGLE);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWGoro, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupGoro(self);
    return 0;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int NullGoro(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoGoroPain(DSWActor* actor)
{
    NullGoro(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(actor);
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoGoroMove(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        actor->callAction();

    KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------


#include "saveable.h"

static saveable_data saveable_goro_data[] =
{
    SAVE_DATA(GoroPersonality),

    SAVE_DATA(GoroAttrib),

    SAVE_DATA(s_GoroRun),
    SAVE_DATA(s_GoroChop),
    SAVE_DATA(s_GoroSpell),
    SAVE_DATA(s_GoroStand),
    SAVE_DATA(s_GoroPain),
    SAVE_DATA(s_GoroDie),
    SAVE_DATA(s_GoroDead),

    SAVE_DATA(GoroActionSet),
};

saveable_module saveable_goro =
{
    // code
    nullptr, 0,

    // data
    saveable_goro_data,
    SIZ(saveable_goro_data)
};
END_SW_NS
