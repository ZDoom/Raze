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
#include "sector.h"
#include "gamecontrol.h"
#include "mapinfo.h"

BEGIN_SW_NS

DECISION SerpBattle[] =
{
    {670,   &AF(InitActorMoveCloser  )       },
    {700,   &AF(InitActorSetDecide)         },
    {710,   &AF(InitActorRunAway     )       },
    {1024,  &AF(InitActorAttack      )       }
};

DECISION SerpOffense[] =
{
    {775,   &AF(InitActorMoveCloser  )       },
    {800,   &AF(InitActorSetDecide)         },
    {1024,  &AF(InitActorAttack      )       }
};

DECISIONB SerpBroadcast[] =
{
    {10,    attr_ambient       },
    {1024,  0   }
};

DECISION SerpSurprised[] =
{
    {701,   &AF(InitActorMoveCloser)        },
    {1024,  &AF(InitActorDecide    )        }
};

DECISION SerpEvasive[] =
{
    {10,   &AF(InitActorEvade)  },
    {1024, nullptr            }
};

DECISION SerpLostTarget[] =
{
    {900,   &AF(InitActorFindPlayer  )       },
    {921,   &AF(InitActorSetDecide)       },
    {1024,  &AF(InitActorWanderAround)       }
};

DECISION SerpCloseRange[] =
{
    {700,   &AF(InitActorAttack    )         },
    {1024,  &AF(InitActorReposition)         }
};

PERSONALITY SerpPersonality =
{
    SerpBattle,
    SerpOffense,
    SerpBroadcast,
    SerpSurprised,
    SerpEvasive,
    SerpLostTarget,
    SerpCloseRange,
    SerpCloseRange
};

ATTRIBUTE SerpAttrib =
{
    {200, 220, 240, 270},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_SERPAMBIENT, DIGI_SERPALERT, DIGI_SERPSWORDATTACK,
        DIGI_SERPPAIN, DIGI_SERPSCREAM, DIGI_SERPMAGICLAUNCH,
        DIGI_SERPSUMMONHEADS,DIGI_SERPTAUNTYOU,DIGI_SERPDEATHEXPLODE,0
    }
};

ATTRIBUTE SerpPissedAttrib =
{
    {200, 220, 240, 270},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_SERPAMBIENT, DIGI_SERPALERT, DIGI_SERPSWORDATTACK,
        DIGI_SERPPAIN, DIGI_SERPSCREAM, DIGI_SERPMAGICLAUNCH,
        DIGI_SERPSUMMONHEADS,DIGI_SERPTAUNTYOU,DIGI_SERPDEATHEXPLODE,0
    }
};


//////////////////////
//
// SERP RUN
//
//////////////////////

#define SERP_RUN_RATE 24

FState s_SerpRun[] =
{
        {SPR_SERP_RUN, 'A', SERP_RUN_RATE, &AF(DoSerpMove), &s_SerpRun[1]},
        {SPR_SERP_RUN, 'B', SERP_RUN_RATE, &AF(DoSerpMove), &s_SerpRun[2]},
        {SPR_SERP_RUN, 'C', SERP_RUN_RATE, &AF(DoSerpMove), &s_SerpRun[3]},
        {SPR_SERP_RUN, 'B', SERP_RUN_RATE, &AF(DoSerpMove), &s_SerpRun[0]},
};


//////////////////////
//
// SERP SLASH
//
//////////////////////

#define SERP_SLASH_RATE 9

FState s_SerpSlash[] =
{
        {SPR_SERP_SLASH, 'C', SERP_SLASH_RATE, &AF(NullSerp), &s_SerpSlash[1]},
        {SPR_SERP_SLASH, 'B', SERP_SLASH_RATE, &AF(NullSerp), &s_SerpSlash[2]},
        {SPR_SERP_SLASH, 'A', SERP_SLASH_RATE*2, &AF(NullSerp), &s_SerpSlash[3]},
        {SPR_SERP_SLASH, 'B', SERP_SLASH_RATE, &AF(NullSerp), &s_SerpSlash[4]},
        {SPR_SERP_SLASH, 'C', SERP_SLASH_RATE, &AF(NullSerp), &s_SerpSlash[5]},
        {SPR_SERP_SLASH, 'D', SF_QUICK_CALL, &AF(InitSerpSlash), &s_SerpSlash[6]},
        {SPR_SERP_SLASH, 'D', SERP_SLASH_RATE, &AF(NullSerp), &s_SerpSlash[7]},
        {SPR_SERP_SLASH, 'E', SERP_SLASH_RATE, &AF(NullSerp), &s_SerpSlash[8]},
        {SPR_SERP_SLASH, 'E', 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SerpSlash[9]},
        {SPR_SERP_SLASH, 'E', SERP_SLASH_RATE, &AF(DoSerpMove), &s_SerpSlash[9]},
};


//////////////////////
//
// SERP SKULL SPELL
//
//////////////////////

#define SERP_SKULL_SPELL_RATE 18

FState s_SerpSkullSpell[] =
{
        {SPR_SERP_SPELL, 'C', SERP_SKULL_SPELL_RATE*2, &AF(NullSerp), &s_SerpSkullSpell[1]},
        {SPR_SERP_SPELL, 'B', SERP_SKULL_SPELL_RATE, &AF(NullSerp), &s_SerpSkullSpell[2]},
        {SPR_SERP_SPELL, 'A', SERP_SKULL_SPELL_RATE*2, &AF(NullSerp), &s_SerpSkullSpell[3]},
        {SPR_SERP_SPELL, 'A', SF_QUICK_CALL, &AF(InitSerpRing), &s_SerpSkullSpell[4]},
        {SPR_SERP_SPELL, 'A', SERP_SKULL_SPELL_RATE, &AF(NullSerp), &s_SerpSkullSpell[5]},
        {SPR_SERP_SPELL, 'B', SERP_SKULL_SPELL_RATE, &AF(NullSerp), &s_SerpSkullSpell[6]},
        {SPR_SERP_SPELL, 'B', SF_QUICK_CALL,   &AF(InitActorDecide), &s_SerpSkullSpell[7]},
        {SPR_SERP_SPELL, 'B', SERP_SKULL_SPELL_RATE, &AF(DoSerpMove), &s_SerpSkullSpell[7]},
};


//////////////////////
//
// SERP SPELL
//
//////////////////////

#define SERP_SPELL_RATE 18

FState s_SerpSpell[] =
{
        {SPR_SERP_SPELL, 'C', SERP_SPELL_RATE*2, &AF(NullSerp), &s_SerpSpell[1]},
        {SPR_SERP_SPELL, 'B', SERP_SPELL_RATE, &AF(NullSerp), &s_SerpSpell[2]},
        {SPR_SERP_SPELL, 'A', SERP_SPELL_RATE*2, &AF(NullSerp), &s_SerpSpell[3]},
        {SPR_SERP_SPELL, 'A', SF_QUICK_CALL, &AF(InitSerpSpell), &s_SerpSpell[4]},
        {SPR_SERP_SPELL, 'A', SERP_SPELL_RATE, &AF(NullSerp), &s_SerpSpell[5]},
        {SPR_SERP_SPELL, 'B', SERP_SPELL_RATE, &AF(NullSerp), &s_SerpSpell[6]},
        {SPR_SERP_SPELL, 'B', SF_QUICK_CALL,   &AF(InitActorDecide), &s_SerpSpell[7]},
        {SPR_SERP_SPELL, 'B', SERP_SPELL_RATE, &AF(DoSerpMove), &s_SerpSpell[7]},
};


//////////////////////
//
// SERP SPELL MONSTER
//
//////////////////////

FState s_SerpMonstSpell[] =
{
        {SPR_SERP_SPELL, 'C', SERP_SPELL_RATE*2, &AF(NullSerp), &s_SerpMonstSpell[1]},
        {SPR_SERP_SPELL, 'B', SERP_SPELL_RATE, &AF(NullSerp), &s_SerpMonstSpell[2]},
        {SPR_SERP_SPELL, 'A', SERP_SPELL_RATE*2, &AF(NullSerp), &s_SerpMonstSpell[3]},
        {SPR_SERP_SPELL, 'A', SF_QUICK_CALL, &AF(InitSerpMonstSpell), &s_SerpMonstSpell[4]},
        {SPR_SERP_SPELL, 'A', SERP_SPELL_RATE, &AF(NullSerp), &s_SerpMonstSpell[5]},
        {SPR_SERP_SPELL, 'B', SERP_SPELL_RATE, &AF(NullSerp), &s_SerpMonstSpell[6]},
        {SPR_SERP_SPELL, 'B', SF_QUICK_CALL,   &AF(InitActorDecide), &s_SerpMonstSpell[7]},
        {SPR_SERP_SPELL, 'B', SERP_SPELL_RATE, &AF(DoSerpMove), &s_SerpMonstSpell[7]},
};


//////////////////////
//
// SERP RAPID SPELL
//
//////////////////////

#define SERP_SPELL_RATE 18

FState s_SerpRapidSpell[] =
{
        {SPR_SERP_SPELL, 'C', SERP_SPELL_RATE*2, &AF(NullSerp), &s_SerpRapidSpell[1]},
        {SPR_SERP_SPELL, 'B', SERP_SPELL_RATE, &AF(NullSerp), &s_SerpRapidSpell[2]},
        {SPR_SERP_SPELL, 'A', SERP_SPELL_RATE*2, &AF(NullSerp), &s_SerpRapidSpell[3]},
        {SPR_SERP_SPELL, 'A', SF_QUICK_CALL, &AF(InitSerpSpell), &s_SerpRapidSpell[4]},
        {SPR_SERP_SPELL, 'A', SERP_SPELL_RATE*2, &AF(NullSerp), &s_SerpRapidSpell[5]},
        {SPR_SERP_SPELL, 'A', SF_QUICK_CALL, &AF(InitSerpSpell), &s_SerpRapidSpell[6]},
        {SPR_SERP_SPELL, 'A', SERP_SPELL_RATE, &AF(NullSerp), &s_SerpRapidSpell[7]},
        {SPR_SERP_SPELL, 'B', SERP_SPELL_RATE, &AF(NullSerp), &s_SerpRapidSpell[8]},
        {SPR_SERP_SPELL, 'B', SF_QUICK_CALL,   &AF(InitActorDecide), &s_SerpRapidSpell[9]},
        {SPR_SERP_SPELL, 'B', SERP_SPELL_RATE, &AF(DoSerpMove), &s_SerpRapidSpell[9]},
};


//////////////////////
//
// SERP STAND
//
//////////////////////

#define SERP_STAND_RATE 12

FState s_SerpStand[] =
{
        {SPR_SERP_RUN, 'A', SERP_STAND_RATE, &AF(DoSerpMove), &s_SerpStand[0]},
};


//////////////////////
//
// SERP DIE
//
//////////////////////

#define SERP_DIE_RATE 20

FState s_SerpDie[] =
{
    {SPR_SERP_DIE, 'A', SERP_DIE_RATE, &AF(NullSerp), &s_SerpDie[1]},
    {SPR_SERP_DIE, 'B', SERP_DIE_RATE, &AF(NullSerp), &s_SerpDie[2]},
    {SPR_SERP_DIE, 'C', SERP_DIE_RATE, &AF(NullSerp), &s_SerpDie[3]},
    {SPR_SERP_DIE, 'D', SERP_DIE_RATE, &AF(NullSerp), &s_SerpDie[4]},
    {SPR_SERP_DIE, 'E', SERP_DIE_RATE, &AF(NullSerp), &s_SerpDie[5]},
    {SPR_SERP_DIE, 'F', SERP_DIE_RATE, &AF(NullSerp), &s_SerpDie[6]},
    {SPR_SERP_DIE, 'G', SERP_DIE_RATE, &AF(NullSerp), &s_SerpDie[7]},
    {SPR_SERP_DIE, 'H', SERP_DIE_RATE, &AF(NullSerp), &s_SerpDie[8]},
    {SPR_SERP_DIE, 'I', SERP_DIE_RATE, &AF(NullSerp), &s_SerpDie[9]},
    {SPR_SERP_DIE, 'I', SF_QUICK_CALL, &AF(DoDeathSpecial), &s_SerpDie[10]},
    {SPR_SERP_DEAD, 'A',    SERP_DIE_RATE, &AF(DoActorDebris), &s_SerpDie[10]}
};

FState s_SerpDead[] =
{
    {SPR_SERP_DEAD, 'A', SERP_DIE_RATE, &AF(DoActorDebris), &s_SerpDead[0]},
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


ACTOR_ACTION_SET SerpActionSet =
{
    s_SerpStand,
    s_SerpRun,
    nullptr, //s_SerpJump,
    nullptr, //s_SerpFall,
    nullptr, //s_SerpCrawl,
    nullptr, //s_SerpSwim,
    nullptr, //s_SerpFly,
    nullptr, //s_SerpRise,
    nullptr, //s_SerpSit,
    nullptr, //s_SerpLook,
    nullptr, //climb
    nullptr, //pain
    s_SerpDie,
    nullptr, //s_SerpHariKari,
    s_SerpDead,
    nullptr, //s_SerpDeathJump,
    nullptr, //s_SerpDeathFall,
    {s_SerpSlash},
    {1024},
    {s_SerpSlash, s_SerpSpell, s_SerpRapidSpell, s_SerpRapidSpell},
    {256, 724, 900, 1024},
    {nullptr},
    nullptr,
    nullptr
};

int SetupSerp(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,SERP_RUN_R0,s_SerpRun[0]);
        actor->user.Health = HEALTH_SERP_GOD;
    }

    if (Skill == 0) actor->user.Health = 1100;
    if (Skill == 1) actor->user.Health = 2200;

    actor->user.__legacyState.Attrib = &SerpAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.__legacyState.StateEnd = s_SerpDie;

    EnemyDefaults(actor, &SerpActionSet, &SerpPersonality);
    actor->setStateGroup(NAME_Run);
    actor->setPicFromState();

    // Mini-Boss Serp
    if (actor->spr.pal == 16)
    {
        actor->user.Health = 1000;
        actor->spr.scale = DVector2(1.15625, 1.15625);
    }
    else
    {
        actor->spr.scale = DVector2(1.5625, 2);
    }

    actor->clipdist = 32;
    actor->user.Flags |= (SPR_XFLIP_TOGGLE|SPR_ELECTRO_TOLERANT);

    actor->user.loz = actor->spr.pos.Z;

    // amount to move up for clipmove
    actor->user.zclip = 80;
    // size of step can walk off of
    actor->user.lo_step = 40;

    actor->user.floor_dist = actor->user.zclip - actor->user.lo_step;
    actor->user.ceiling_dist = ActorSizeZ(actor) - actor->user.zclip;

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWSerpent, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupSerp(self);
    return 0;
}

int NullSerp(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    KeepActorOnFloor(actor);

    //DoActorSectorDamage(actor);
    return 0;
}

int DoSerpMove(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        actor->callAction();

    // serp ring
    if (actor->spr.pal != 16)
    {
        switch (actor->user.Counter2)
        {
        case 0:
            if (actor->user.Health != actor->user.MaxHealth)
            {
                NewStateGroup(actor, s_SerpSkullSpell);
                actor->user.Counter2++;
            }
            break;

        case 1:
        {
            if (actor->user.Counter <= 0)
                NewStateGroup(actor, s_SerpSkullSpell);
        }
        break;
        }
    }

    KeepActorOnFloor(actor);

    //DoActorSectorDamage(actor);
    return 0;
}

int DoDeathSpecial(DSWActor* actor)
{
    DoMatchEverything(nullptr, actor->spr.lotag, 1);

    if (!SW_SHAREWARE)
    {
        // Resume the regular music - in a hack-free fashion.
        PlaySong(currentLevel->music.GetChars(), currentLevel->cdSongId);
    }

    BossSpriteNum[0] = nullptr;
    return 0;
}


#include "saveable.h"

static saveable_data saveable_serp_data[] =
{
    SAVE_DATA(SerpPersonality),

    SAVE_DATA(SerpAttrib),
    SAVE_DATA(SerpPissedAttrib),

    SAVE_DATA(s_SerpRun),
    SAVE_DATA(s_SerpSlash),
    SAVE_DATA(s_SerpSkullSpell),
    SAVE_DATA(s_SerpSpell),
    SAVE_DATA(s_SerpMonstSpell),
    SAVE_DATA(s_SerpRapidSpell),
    SAVE_DATA(s_SerpStand),
    SAVE_DATA(s_SerpDie),
    SAVE_DATA(s_SerpDead),

    SAVE_DATA(SerpActionSet),
};

saveable_module saveable_serp =
{
    // code
    nullptr, 0,

    // data
    saveable_serp_data,
    SIZ(saveable_serp_data)
};

END_SW_NS
