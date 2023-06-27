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
#include "misc.h"

BEGIN_SW_NS

DECISION LavaBattle[] =
{
    {600,   &AF(InitActorMoveCloser)         },
    {700,   &AF(InitActorSetDecide)         },
    {710,   &AF(InitActorRunAway   )         },
    {1024,  &AF(InitActorAttack    )         }
};

DECISION LavaOffense[] =
{
    {700,   &AF(InitActorMoveCloser)         },
    {800,   &AF(InitActorSetDecide)         },
    {1024,  &AF(InitActorAttack    )         }
};

DECISIONB LavaBroadcast[] =
{
    {21,    attr_alert      },
    {51,    attr_ambient      },
    {1024,  0    }
};

DECISION LavaSurprised[] =
{
    {701,   &AF(InitActorMoveCloser)        },
    {1024,  &AF(InitActorDecide    )        }
};

DECISION LavaEvasive[] =
{
    {10,   &AF(InitActorEvade)  },
    {1024, nullptr            }
};

DECISION LavaLostTarget[] =
{
    {900,   &AF(InitActorFindPlayer  )       },
    {1024,  &AF(InitActorWanderAround)       }
};

DECISION LavaCloseRange[] =
{
    {700,   &AF(InitActorAttack    )         },
    {1024,  &AF(InitActorReposition)         }
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
FState s_LavaStand[] =
{
        {SPR_LAVA_RUN, 'A', LAVA_STAND_RATE, &AF(DoLavaMove), &s_LavaStand[0]},
};


//////////////////////
//
// LAVA RUN
//
//////////////////////

#define LAVA_RUN_RATE 24


FState s_LavaRun[] =
{
        {SPR_LAVA_RUN, 'A', LAVA_RUN_RATE, &AF(DoLavaMove), &s_LavaRun[1]},
        {SPR_LAVA_RUN, 'A', LAVA_RUN_RATE, &AF(DoLavaMove), &s_LavaRun[2]},
        {SPR_LAVA_RUN, 'A', LAVA_RUN_RATE, &AF(DoLavaMove), &s_LavaRun[3]},
        {SPR_LAVA_RUN, 'A', LAVA_RUN_RATE, &AF(DoLavaMove), &s_LavaRun[0]},
};


//////////////////////
//
// LAVA THROW
//
//////////////////////

#define LAVA_THROW_RATE 9

FState s_LavaThrow[] =
{
        {SPR_LAVA_THROW, 'A', LAVA_THROW_RATE, &AF(NullLava), &s_LavaThrow[1]},
        {SPR_LAVA_THROW, 'A', LAVA_THROW_RATE, &AF(NullLava), &s_LavaThrow[2]},
        {SPR_LAVA_THROW, 'A', LAVA_THROW_RATE*2, &AF(NullLava), &s_LavaThrow[3]},
        {SPR_LAVA_THROW, 'A', LAVA_THROW_RATE, &AF(NullLava), &s_LavaThrow[4]},
        {SPR_LAVA_THROW, 'A', LAVA_THROW_RATE, &AF(NullLava), &s_LavaThrow[5]},
        {SPR_LAVA_THROW, 'A', SF_QUICK_CALL, &AF(InitLavaThrow), &s_LavaThrow[6]},
        {SPR_LAVA_THROW, 'A', LAVA_THROW_RATE, &AF(NullLava), &s_LavaThrow[7]},
        {SPR_LAVA_THROW, 'A', LAVA_THROW_RATE, &AF(NullLava), &s_LavaThrow[8]},
        {SPR_LAVA_THROW, 'A', 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_LavaThrow[9]},
        {SPR_LAVA_THROW, 'A', LAVA_THROW_RATE, &AF(DoLavaMove), &s_LavaThrow[9]},
};



//////////////////////
//
// LAVA FLAME
//
//////////////////////

#define LAVA_FLAME_RATE 18

FState s_LavaFlame[] =
{
        {SPR_LAVA_FLAME, 'A', LAVA_FLAME_RATE*2, &AF(NullLava), &s_LavaFlame[1]},
        {SPR_LAVA_FLAME, 'A', LAVA_FLAME_RATE, &AF(NullLava), &s_LavaFlame[2]},
        {SPR_LAVA_FLAME, 'A', LAVA_FLAME_RATE*2, &AF(NullLava), &s_LavaFlame[3]},
        {SPR_LAVA_FLAME, 'A', SF_QUICK_CALL, &AF(InitLavaFlame), &s_LavaFlame[4]},
        {SPR_LAVA_FLAME, 'A', LAVA_FLAME_RATE, &AF(NullLava), &s_LavaFlame[5]},
        {SPR_LAVA_FLAME, 'A', LAVA_FLAME_RATE, &AF(NullLava), &s_LavaFlame[6]},
        {SPR_LAVA_FLAME, 'A', SF_QUICK_CALL,   &AF(InitActorDecide), &s_LavaFlame[7]},
        {SPR_LAVA_FLAME, 'A', LAVA_FLAME_RATE, &AF(DoLavaMove), &s_LavaFlame[7]},
};


//////////////////////
//
// LAVA DIE
//
//////////////////////

#define LAVA_DIE_RATE 20

FState s_LavaDie[] =
{
    {SPR_LAVA_DIE, 'A', LAVA_DIE_RATE, &AF(NullLava), &s_LavaDie[1]},
    {SPR_LAVA_DEAD, 'A', LAVA_DIE_RATE, &AF(DoActorDebris), &s_LavaDie[1]}
};

FState s_LavaDead[] =
{
    {SPR_LAVA_DEAD, 'A', LAVA_DIE_RATE, &AF(DoActorDebris), &s_LavaDead[0]},
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


ACTOR_ACTION_SET LavaActionSet =
{
    s_LavaStand,
    s_LavaRun,
    nullptr, //s_LavaJump,
    nullptr, //s_LavaFall,
    nullptr, //s_LavaCrawl,
    nullptr, //s_LavaSwim,
    nullptr, //s_LavaFly,
    nullptr, //s_LavaRise,
    nullptr, //s_LavaSit,
    nullptr, //s_LavaLook,
    nullptr, //climb
    nullptr, //pain
    s_LavaDie,
    nullptr, //s_LavaHariKari,
    s_LavaDead,
    nullptr, //s_LavaDeathJump,
    nullptr, //s_LavaDeathFall,
    {s_LavaFlame},
    {1024},
    {s_LavaFlame, s_LavaThrow, s_LavaThrow, s_LavaThrow},
    {256, 512, 768, 1024},
    {nullptr},
    nullptr,
    nullptr
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupLava(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, LAVA_RUN_R0, s_LavaRun[0]);
        actor->user.Health = 100;
    }

    actor->user.__legacyState.Attrib = &LavaAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.__legacyState.StateEnd = s_LavaDie;

    EnemyDefaults(actor, &LavaActionSet, &LavaPersonality);
    actor->setStateGroup(NAME_Run);
    actor->setPicFromState();
    actor->spr.scale = DVector2(1.71875, 1.71875);
    actor->clipdist = 32;
    actor->user.Flags |= (SPR_XFLIP_TOGGLE|SPR_ELECTRO_TOLERANT);

    actor->user.loz = actor->spr.pos.Z;

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWLava, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupLava(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullLava(DSWActor* actor)
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

int DoLavaMove(DSWActor* actor)
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

static saveable_data saveable_lava_data[] =
{
    SAVE_DATA(LavaPersonality),

    SAVE_DATA(LavaAttrib),
    SAVE_DATA(LavaPissedAttrib),

    SAVE_DATA(s_LavaStand),
    SAVE_DATA(s_LavaRun),
    SAVE_DATA(s_LavaThrow),
    SAVE_DATA(s_LavaFlame),
    SAVE_DATA(s_LavaDie),
    SAVE_DATA(s_LavaDead),

    SAVE_DATA(LavaActionSet),
};

saveable_module saveable_lava =
{
    // code
    nullptr, 0,

    // data
    saveable_lava_data,
    SIZ(saveable_lava_data)
};
END_SW_NS
