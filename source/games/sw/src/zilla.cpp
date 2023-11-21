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
#include "gamecontrol.h"
#include "mapinfo.h"

BEGIN_SW_NS


DECISION ZillaBattle[] =
{
    {100,   &AF(InitActorRunAway   )   },
    {690,   &AF(InitActorMoveCloser)   },
    {692,   &AF(InitActorSetDecide)   },
    {1024,  &AF(InitActorAttack    )   }
};

DECISION ZillaOffense[] =
{
    {100,   &AF(InitActorRunAway   )   },
    {690,   &AF(InitActorMoveCloser)   },
    {692,   &AF(InitActorSetDecide)   },
    {1024,  &AF(InitActorAttack    )   }
};

DECISIONB ZillaBroadcast[] =
{
    {2,     attr_alert },
    {4,     attr_ambient  },
    {1024,  0 }
};

DECISION ZillaSurprised[] =
{
    {700,   &AF(InitActorMoveCloser)   },
    {703,   &AF(InitActorSetDecide)   },
    {1024,  &AF(InitActorDecide    )   }
};

DECISION ZillaEvasive[] =
{
    {1024, &AF(InitActorWanderAround) }
};

DECISION ZillaLostTarget[] =
{
    {900,   &AF(InitActorFindPlayer  )       },
    {1024,  &AF(InitActorWanderAround)       }
};

DECISION ZillaCloseRange[] =
{
    {1024,  &AF(InitActorAttack)         }
};

PERSONALITY ZillaPersonality =
{
    ZillaBattle,
    ZillaOffense,
    ZillaBroadcast,
    ZillaSurprised,
    ZillaEvasive,
    ZillaLostTarget,
    ZillaCloseRange,
    ZillaCloseRange
};

ATTRIBUTE ZillaAttrib =
{
    {100, 100, 100, 100},                 // Speeds
    {3, 0, 0, 0},                     // Tic Adjusts
    3,                               // MaxWeapons;
    {
        DIGI_Z17010, DIGI_Z17010, DIGI_Z17025,
        DIGI_Z17052, DIGI_Z17025, 0,0,0,0,0
    }
};


//////////////////////
//
// ZILLA RUN
//
//////////////////////

#define ZILLA_RATE 48

FState s_ZillaRun[] =
{
        {SPR_ZILLA_RUN, 'A', ZILLA_RATE, &AF(DoZillaMove), &s_ZillaRun[1]},
        {SPR_ZILLA_RUN, 'B', SF_QUICK_CALL, &AF(DoZillaStomp), &s_ZillaRun[2]},
        {SPR_ZILLA_RUN, 'B', ZILLA_RATE, &AF(DoZillaMove), &s_ZillaRun[3]},
        {SPR_ZILLA_RUN, 'C', ZILLA_RATE, &AF(DoZillaMove), &s_ZillaRun[4]},
        {SPR_ZILLA_RUN, 'D', SF_QUICK_CALL, &AF(DoZillaStomp), &s_ZillaRun[5]},
        {SPR_ZILLA_RUN, 'D', ZILLA_RATE, &AF(DoZillaMove), &s_ZillaRun[0]}
};

//////////////////////
//
// ZILLA STAND
//
//////////////////////

FState s_ZillaStand[] =
{
        {SPR_ZILLA_RUN, 'A', ZILLA_RATE, &AF(DoZillaMove), &s_ZillaStand[0]}
};

//////////////////////
//
// ZILLA PAIN
//
//////////////////////

#define ZILLA_PAIN_RATE 30

FState s_ZillaPain[] =
{
        {SPR_ZILLA_PAIN, 'A', ZILLA_PAIN_RATE, &AF(NullZilla), &s_ZillaPain[1]},
        {SPR_ZILLA_PAIN, 'A', 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_ZillaPain[0]}
};

//////////////////////
//
// ZILLA RAIL
//
//////////////////////

#define ZILLA_RAIL_RATE 12

FState s_ZillaRail[] =
{
        {SPR_ZILLA_RAIL, 'A', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[1]},
        {SPR_ZILLA_RAIL, 'B', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[2]},
        {SPR_ZILLA_RAIL, 'C', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[3]},
        {SPR_ZILLA_RAIL, 'D', SF_QUICK_CALL, &AF(InitZillaRail), &s_ZillaRail[4]},
        {SPR_ZILLA_RAIL, 'A', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[5]},
        {SPR_ZILLA_RAIL, 'B', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[6]},
        {SPR_ZILLA_RAIL, 'C', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[7]},
        {SPR_ZILLA_RAIL, 'D', SF_QUICK_CALL, &AF(InitZillaRail), &s_ZillaRail[8]},
        {SPR_ZILLA_RAIL, 'A', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[9]},
        {SPR_ZILLA_RAIL, 'B', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[10]},
        {SPR_ZILLA_RAIL, 'C', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[11]},
        {SPR_ZILLA_RAIL, 'D', SF_QUICK_CALL, &AF(InitZillaRail), &s_ZillaRail[12]},
        {SPR_ZILLA_RAIL, 'D', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[13]},
        {SPR_ZILLA_RAIL, 'D', SF_QUICK_CALL, &AF(InitActorDecide), &s_ZillaRail[0]}
};

//////////////////////
//
// ZILLA ROCKET
//
//////////////////////

#define ZILLA_ROCKET_RATE 12

FState s_ZillaRocket[] =
{
        {SPR_ZILLA_ROCKET, 'A', ZILLA_ROCKET_RATE, &AF(NullZilla), &s_ZillaRocket[1]},
        {SPR_ZILLA_ROCKET, 'B', ZILLA_ROCKET_RATE, &AF(NullZilla), &s_ZillaRocket[2]},
        {SPR_ZILLA_ROCKET, 'C', ZILLA_ROCKET_RATE*4, &AF(NullZilla), &s_ZillaRocket[3]},
        {SPR_ZILLA_ROCKET, 'C', SF_QUICK_CALL, &AF(InitZillaRocket), &s_ZillaRocket[4]},
        {SPR_ZILLA_ROCKET, 'C', ZILLA_ROCKET_RATE*4, &AF(NullZilla), &s_ZillaRocket[5]},
        {SPR_ZILLA_ROCKET, 'D', SF_QUICK_CALL, &AF(InitActorDecide), &s_ZillaRocket[6]},
        {SPR_ZILLA_ROCKET, 'D', ZILLA_ROCKET_RATE*10, &AF(NullZilla), &s_ZillaRocket[5]}
};

//////////////////////
//
// ZILLA UZI
//
//////////////////////

#define ZILLA_UZI_RATE 8

FState s_ZillaUzi[] =
{
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[1]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[2]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[3]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[4]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[5]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[6]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[7]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[8]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[9]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[10]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[11]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[12]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[13]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[14]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[15]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[16]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_ZillaUzi[16]},
};


//////////////////////
//
// ZILLA DIE
//
//////////////////////

#define ZILLA_DIE_RATE 30

FState s_ZillaDie[] =
{
    {SPR_ZILLA_DIE, 'A', ZILLA_DIE_RATE*15, &AF(DoZillaDeathMelt), &s_ZillaDie[1]},
    {SPR_ZILLA_DIE, 'B', ZILLA_DIE_RATE, &AF(NullZilla), &s_ZillaDie[2]},
    {SPR_ZILLA_DIE, 'C', ZILLA_DIE_RATE, &AF(NullZilla), &s_ZillaDie[3]},
    {SPR_ZILLA_DIE, 'D', ZILLA_DIE_RATE, &AF(NullZilla), &s_ZillaDie[4]},
    {SPR_ZILLA_DIE, 'E', ZILLA_DIE_RATE, &AF(NullZilla), &s_ZillaDie[5]},
    {SPR_ZILLA_DIE, 'F', ZILLA_DIE_RATE, &AF(NullZilla), &s_ZillaDie[6]},
    {SPR_ZILLA_DIE, 'G', ZILLA_DIE_RATE, &AF(NullZilla), &s_ZillaDie[7]},
    {SPR_ZILLA_DIE, 'H', ZILLA_DIE_RATE*3, &AF(NullZilla), &s_ZillaDie[8]},
    {SPR_ZILLA_DEAD, 'A', ZILLA_DIE_RATE, &AF(DoActorDebris), &s_ZillaDie[8]}
};

FState s_ZillaDead[] =
{
    {SPR_ZILLA_DEAD, 'A', ZILLA_DIE_RATE, &AF(DoActorDebris), &s_ZillaDead[0]},
};

ACTOR_ACTION_SET ZillaActionSet =
{
    s_ZillaStand,
    s_ZillaRun,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr, //climb
    s_ZillaPain, //pain
    s_ZillaDie,
    nullptr,
    s_ZillaDead,
    nullptr,
    nullptr,
    {s_ZillaUzi,s_ZillaRail},
    {950,1024},
    {s_ZillaUzi,s_ZillaRocket,s_ZillaRail},
    {400,950,1024},
    {nullptr},
    nullptr,
    nullptr
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupZilla(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, ZILLA_RUN_R0, s_ZillaRun[0]);
        actor->user.Health = 6000;
    }

    if (Skill == 0) actor->user.Health = 2000;
    if (Skill == 1) actor->user.Health = 4000;

    actor->user.__legacyState.Attrib = &ZillaAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.__legacyState.StateEnd = s_ZillaDie;

    EnemyDefaults(actor, &ZillaActionSet, &ZillaPersonality);
    actor->setStateGroup(NAME_Run);
    actor->setPicFromState();

    actor->clipdist = 32;
    actor->spr.scale = DVector2(1.515625, 1.23475);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullZilla(DSWActor* actor)
{
    calcSlope(actor->sector(), actor->spr.pos, &actor->user.hiz, &actor->user.loz);
    actor->user.lo_sectp = actor->sector();
    actor->user.hi_sectp = actor->sector();
    actor->user.lowActor = nullptr;
    actor->user.highActor = nullptr;
    actor->spr.pos.Z = actor->user.loz;

    DoActorSectorDamage(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoZillaMove(DSWActor* actor)
{
    short choose;

    // Random Zilla taunts
    if (!SoundValidAndActive(actor, CHAN_AnimeMad))
    {
        choose = StdRandomRange(1000);
        if (choose > 990)
            PlaySound(DIGI_Z16004, actor, v3df_none, CHAN_AnimeMad);
        else if (choose > 985)
            PlaySound(DIGI_Z16004, actor, v3df_none, CHAN_AnimeMad);
        else if (choose > 980)
            PlaySound(DIGI_Z16004, actor, v3df_none, CHAN_AnimeMad);
        else if (choose > 975)
            PlaySound(DIGI_Z16004, actor, v3df_none, CHAN_AnimeMad);
    }


    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        actor->callAction();

    KeepActorOnFloor(actor);

    if (DoActorSectorDamage(actor))
    {
        return 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoZillaStomp(DSWActor* actor)
{
    PlaySound(DIGI_ZILLASTOMP, actor, v3df_follow);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoZillaDeathMelt(DSWActor* actor)
{
    if (RandomRange(1000) > 800)
        SpawnGrenadeExp(actor);

    actor->user.ID = ZILLA_RUN_R0;
    actor->user.Flags &= ~(SPR_JUMPING|SPR_FALLING|SPR_MOVED);

    //DoMatchEverything(nullptr, actor->spr.lotag, ON);
    if (!SW_SHAREWARE)
    {
        // Resume the regular music - in a hack-free fashion.
        PlaySong(currentLevel->music.GetChars(), currentLevel->cdSongId);
    }

    //KeepActorOnFloor(actor);
    calcSlope(actor->sector(), actor->spr.pos, &actor->user.hiz, &actor->user.loz);
    actor->user.lo_sectp = actor->sector();
    actor->user.hi_sectp = actor->sector();
    actor->user.lowActor = nullptr;
    actor->user.highActor = nullptr;
    actor->spr.pos.Z = actor->user.loz;

    BossSpriteNum[2] = nullptr;
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------


#include "saveable.h"

static saveable_data saveable_zilla_data[] =
{
    SAVE_DATA(ZillaPersonality),

    SAVE_DATA(ZillaAttrib),

    SAVE_DATA(ZillaActionSet),

    SAVE_DATA(s_ZillaRun),
    SAVE_DATA(s_ZillaStand),
    SAVE_DATA(s_ZillaPain),
    SAVE_DATA(s_ZillaRail),
    SAVE_DATA(s_ZillaRocket),
    SAVE_DATA(s_ZillaUzi),
    SAVE_DATA(s_ZillaDie)

};

saveable_module saveable_zilla =
{
    // code
    nullptr, 0,

    // data
    saveable_zilla_data,
    SIZ(saveable_zilla_data)
};

END_SW_NS
