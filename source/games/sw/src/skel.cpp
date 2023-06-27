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
#include "misc.h"

BEGIN_SW_NS

DECISION SkelBattle[] =
{
    {600,   &AF(InitActorMoveCloser)         },
    {602,   &AF(InitActorSetDecide)         },
    {700,   &AF(InitActorRunAway   )         },
    {1024,  &AF(InitActorAttack    )         }
};

DECISION SkelOffense[] =
{
    {700,   &AF(InitActorMoveCloser)         },
    {702,   &AF(InitActorSetDecide)         },
    {1024,  &AF(InitActorAttack    )         }
};

DECISIONB SkelBroadcast[] =
{
    {3,    attr_alert      },
    {6,    attr_ambient      },
    {1024, 0  }
};

DECISION SkelSurprised[] =
{
    {701,   &AF(InitActorMoveCloser)         },
    {1024,  &AF(InitActorDecide    )        }
};

DECISION SkelEvasive[] =
{
    {22,     &AF(InitActorDuck )            },
    {30,     &AF(InitActorEvade)            },
    {1024,   nullptr                      },
};

DECISION SkelLostTarget[] =
{
    {900,   &AF(InitActorFindPlayer  )       },
    {1024,  &AF(InitActorWanderAround)       }
};

DECISION SkelCloseRange[] =
{
    {800,  &AF(InitActorAttack    )         },
    {1024, &AF(InitActorReposition)            }
};

PERSONALITY SkelPersonality =
{
    SkelBattle,
    SkelOffense,
    SkelBroadcast,
    SkelSurprised,
    SkelEvasive,
    SkelLostTarget,
    SkelCloseRange,
    SkelCloseRange
};

ATTRIBUTE SkelAttrib =
{
    {60, 80, 100, 130},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        DIGI_SPAMBIENT, DIGI_SPALERT, 0,
        DIGI_SPPAIN, DIGI_SPSCREAM, DIGI_SPBLADE,
        DIGI_SPELEC,DIGI_SPTELEPORT,0,0
    }
};

//////////////////////
//
// SKEL RUN
//
//////////////////////

#define SKEL_RUN_RATE 12

// +4 on frame #3 to add character

FState s_SkelRun[] =
{
        {SPR_SKEL_RUN, 'A', SKEL_RUN_RATE+4, &AF(DoSkelMove), &s_SkelRun[1]},
        {SPR_SKEL_RUN, 'B', SKEL_RUN_RATE, &AF(DoSkelMove), &s_SkelRun[2]},
        {SPR_SKEL_RUN, 'C', SKEL_RUN_RATE, &AF(DoSkelMove), &s_SkelRun[3]},
        {SPR_SKEL_RUN, 'D', SKEL_RUN_RATE+4, &AF(DoSkelMove), &s_SkelRun[4]},
        {SPR_SKEL_RUN, 'E', SKEL_RUN_RATE, &AF(DoSkelMove), &s_SkelRun[5]},
        {SPR_SKEL_RUN, 'F', SKEL_RUN_RATE, &AF(DoSkelMove), &s_SkelRun[0]},
};

//////////////////////
//
// SKEL SLASH
//
//////////////////////

#define SKEL_SLASH_RATE 20

FState s_SkelSlash[] =
{
        {SPR_SKEL_SLASH, 'A', SKEL_SLASH_RATE, &AF(NullSkel), &s_SkelSlash[1]},
        {SPR_SKEL_SLASH, 'B', SKEL_SLASH_RATE, &AF(NullSkel), &s_SkelSlash[2]},
        {SPR_SKEL_SLASH, 'C', 0|SF_QUICK_CALL, &AF(InitSkelSlash), &s_SkelSlash[3]},
        {SPR_SKEL_SLASH, 'C', SKEL_SLASH_RATE*2, &AF(NullSkel), &s_SkelSlash[4]},
        {SPR_SKEL_SLASH, 'B', SKEL_SLASH_RATE, &AF(NullSkel), &s_SkelSlash[5]},
        {SPR_SKEL_SLASH, 'B', 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SkelSlash[6]},
        {SPR_SKEL_SLASH, 'B', SKEL_SLASH_RATE, &AF(DoSkelMove), &s_SkelSlash[6]},
};


//////////////////////
//
// SKEL SPELL
//
//////////////////////

#define SKEL_SPELL_RATE 20

FState s_SkelSpell[] =
{
        {SPR_SKEL_SPELL, 'A', SKEL_SPELL_RATE+9, &AF(NullSkel), &s_SkelSpell[1]},
        {SPR_SKEL_SPELL, 'B', SKEL_SPELL_RATE, &AF(NullSkel), &s_SkelSpell[2]},
        {SPR_SKEL_SPELL, 'C', SKEL_SPELL_RATE, &AF(NullSkel), &s_SkelSpell[3]},
        {SPR_SKEL_SPELL, 'D', SKEL_SPELL_RATE*2, &AF(NullSkel), &s_SkelSpell[4]},
        {SPR_SKEL_SPELL, 'D', 0|SF_QUICK_CALL, &AF(InitSkelSpell), &s_SkelSpell[5]},
        {SPR_SKEL_SPELL, 'D', 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SkelSpell[6]},
        {SPR_SKEL_SPELL, 'D', SKEL_SPELL_RATE, &AF(DoSkelMove), &s_SkelSpell[6]},
};


//////////////////////
//
// SKEL PAIN
//
//////////////////////

#define SKEL_PAIN_RATE 38

FState s_SkelPain[] =
{
        {SPR_SKEL_PAIN, 'A', SKEL_PAIN_RATE, &AF(DoSkelPain), &s_SkelPain[0]},
};

//////////////////////
//
// SKEL TELEPORT
//
//////////////////////

#define SKEL_TELEPORT_RATE 20

FState s_SkelTeleport[] =
{
    {SPR_SKEL_TELEPORT, 'A',  1,                  nullptr,  &s_SkelTeleport[1]},
    {SPR_SKEL_TELEPORT, 'A',  0|SF_QUICK_CALL,    &AF(DoSkelInitTeleport), &s_SkelTeleport[2]},
    {SPR_SKEL_TELEPORT, 'A',  SKEL_TELEPORT_RATE, nullptr,  &s_SkelTeleport[3]},
    {SPR_SKEL_TELEPORT, 'B',  SKEL_TELEPORT_RATE, nullptr,  &s_SkelTeleport[4]},
    {SPR_SKEL_TELEPORT, 'C',  SKEL_TELEPORT_RATE, nullptr,  &s_SkelTeleport[5]},
    {SPR_SKEL_TELEPORT, 'D',  SKEL_TELEPORT_RATE, nullptr,  &s_SkelTeleport[6]},
    {SPR_SKEL_TELEPORT, 'E',  SKEL_TELEPORT_RATE, nullptr,  &s_SkelTeleport[7]},
    {SPR_SKEL_TELEPORT, 'F',  SKEL_TELEPORT_RATE, nullptr,  &s_SkelTeleport[8]},
    {SPR_SKEL_TELEPORT, 'F',  0|SF_QUICK_CALL,    &AF(DoSkelTeleport), &s_SkelTeleport[9]},
    {SPR_SKEL_TELEPORT, 'F',  SKEL_TELEPORT_RATE, nullptr,  &s_SkelTeleport[10]},
    {SPR_SKEL_TELEPORT, 'E',  SKEL_TELEPORT_RATE, nullptr,  &s_SkelTeleport[11]},
    {SPR_SKEL_TELEPORT, 'D',  SKEL_TELEPORT_RATE, nullptr,  &s_SkelTeleport[12]},
    {SPR_SKEL_TELEPORT, 'C',  SKEL_TELEPORT_RATE, nullptr,  &s_SkelTeleport[13]},
    {SPR_SKEL_TELEPORT, 'B',  SKEL_TELEPORT_RATE, nullptr,  &s_SkelTeleport[14]},
    {SPR_SKEL_TELEPORT, 'A',  SKEL_TELEPORT_RATE, &AF(DoSkelTermTeleport), &s_SkelTeleport[15]},
    {SPR_SKEL_TELEPORT, 'A',  0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SkelTeleport[16]},
    {SPR_SKEL_TELEPORT, 'A',  SKEL_TELEPORT_RATE, &AF(DoSkelMove), &s_SkelTeleport[16]},
};

//////////////////////
//
// SKEL STAND
//
//////////////////////

#define SKEL_STAND_RATE 12

FState s_SkelStand[] =
{
        {SPR_SKEL_RUN, 'A', SKEL_STAND_RATE, &AF(DoSkelMove), &s_SkelStand[0]},
};


//////////////////////
//
// SKEL DIE
//
//////////////////////

#define SKEL_DIE_RATE 25

FState s_SkelDie[] =
{
    {SPR_SKEL_DIE, 'A', SKEL_DIE_RATE, nullptr,  &s_SkelDie[1]},
    {SPR_SKEL_DIE, 'B', SKEL_DIE_RATE, nullptr,  &s_SkelDie[2]},
    {SPR_SKEL_DIE, 'C', SKEL_DIE_RATE, nullptr,  &s_SkelDie[3]},
    {SPR_SKEL_DIE, 'D', SKEL_DIE_RATE, nullptr,  &s_SkelDie[4]},
    {SPR_SKEL_DIE, 'E', SKEL_DIE_RATE, nullptr,  &s_SkelDie[5]},
    {SPR_SKEL_DIE, 'F', SKEL_DIE_RATE, &AF(DoSuicide),   &s_SkelDie[5]},
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

ACTOR_ACTION_SET SkelActionSet =
{
    s_SkelStand,
    s_SkelRun,
    nullptr, //s_SkelJump,
    nullptr, //s_SkelFall,
    nullptr, //s_SkelCrawl,
    nullptr, //s_SkelSwim,
    nullptr, //s_SkelFly,
    nullptr, //s_SkelRise,
    nullptr, //s_SkelSit,
    nullptr, //s_SkelLook,
    nullptr, //climb
    s_SkelPain, //pain
    s_SkelDie,
    nullptr, //s_SkelHariKari,
    nullptr, //s_SkelDead,
    nullptr, //s_SkelDeathJump,
    nullptr, //s_SkelDeathFall,
    {s_SkelSlash},
    {1024},
    {s_SkelSpell},
    {1024},
    {nullptr},
    s_SkelTeleport,
    nullptr
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupSkel(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,SKEL_RUN_R0,s_SkelRun[0]);
        actor->user.Health = HEALTH_SKEL_PRIEST;
    }

    actor->user.__legacyState.Attrib = &SkelAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.__legacyState.StateEnd = s_SkelDie;

    EnemyDefaults(actor, &SkelActionSet, &SkelPersonality);
    actor->setStateGroup(NAME_Run);
    actor->setPicFromState();

    // 256 is default
    //actor->clipdist = 16;
    actor->user.Flags |= (SPR_XFLIP_TOGGLE);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWSkeleton, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupSkel(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSkelInitTeleport(DSWActor* actor)
{
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    PlaySpriteSound(actor,attr_extra3,v3df_follow);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSkelTeleport(DSWActor* actor)
{
    auto pos = actor->spr.pos;

    while (true)
    {
        pos.XY() = actor->spr.pos.XY();

        if (RANDOM_P2(1024) < 512)
            pos.X += 32 + RANDOM_P2F(64, 4);
        else
            pos.X -= 32 + RANDOM_P2F(64, 4);

        if (RANDOM_P2(1024) < 512)
            pos.Y += 32 + RANDOM_P2F(64, 4);
        else
            pos.Y -= 32 + RANDOM_P2F(64, 4);

        SetActorZ(actor, pos);

        if (actor->insector())
            break;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSkelTermTeleport(DSWActor* actor)
{
    actor->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullSkel(DSWActor* actor)
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

int DoSkelPain(DSWActor* actor)
{
    NullSkel(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSkelMove(DSWActor* actor)
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

static saveable_data saveable_skel_data[] =
{
    SAVE_DATA(SkelPersonality),

    SAVE_DATA(SkelAttrib),

    SAVE_DATA(s_SkelRun),
    SAVE_DATA(s_SkelSlash),
    SAVE_DATA(s_SkelSpell),
    SAVE_DATA(s_SkelPain),
    SAVE_DATA(s_SkelTeleport),
    SAVE_DATA(s_SkelStand),
    SAVE_DATA(s_SkelDie),

    SAVE_DATA(SkelActionSet),
};

saveable_module saveable_skel =
{
    // code
    nullptr, 0,

    // data
    saveable_skel_data,
    SIZ(saveable_skel_data)
};
END_SW_NS
