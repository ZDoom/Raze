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

STATE s_ZillaRun[1][6] =
{
    {
        {SPR_ZILLA_RUN, 'A', ZILLA_RATE, &AF(DoZillaMove), &s_ZillaRun[0][1]},
        {SPR_ZILLA_RUN, 'B', SF_QUICK_CALL, &AF(DoZillaStomp), &s_ZillaRun[0][2]},
        {SPR_ZILLA_RUN, 'B', ZILLA_RATE, &AF(DoZillaMove), &s_ZillaRun[0][3]},
        {SPR_ZILLA_RUN, 'C', ZILLA_RATE, &AF(DoZillaMove), &s_ZillaRun[0][4]},
        {SPR_ZILLA_RUN, 'D', SF_QUICK_CALL, &AF(DoZillaStomp), &s_ZillaRun[0][5]},
        {SPR_ZILLA_RUN, 'D', ZILLA_RATE, &AF(DoZillaMove), &s_ZillaRun[0][0]}
    },
};

STATE* sg_ZillaRun[] =
{
    &s_ZillaRun[0][0],
};


//////////////////////
//
// ZILLA STAND
//
//////////////////////

STATE s_ZillaStand[1][1] =
{
    {
        {SPR_ZILLA_RUN, 'A', ZILLA_RATE, &AF(DoZillaMove), &s_ZillaStand[0][0]}
    },
};

STATE* sg_ZillaStand[] =
{
    &s_ZillaStand[0][0],
};

//////////////////////
//
// ZILLA PAIN
//
//////////////////////

#define ZILLA_PAIN_RATE 30

STATE s_ZillaPain[1][2] =
{
    {
        {SPR_ZILLA_PAIN, 'A', ZILLA_PAIN_RATE, &AF(NullZilla), &s_ZillaPain[0][1]},
        {SPR_ZILLA_PAIN, 'A', 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_ZillaPain[0][0]}
    },
};

STATE* sg_ZillaPain[] =
{
    &s_ZillaPain[0][0],
};

//////////////////////
//
// ZILLA RAIL
//
//////////////////////

#define ZILLA_RAIL_RATE 12

STATE s_ZillaRail[1][14] =
{
    {
        {SPR_ZILLA_RAIL, 'A', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[0][1]},
        {SPR_ZILLA_RAIL, 'B', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[0][2]},
        {SPR_ZILLA_RAIL, 'C', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[0][3]},
        {SPR_ZILLA_RAIL, 'D', SF_QUICK_CALL, &AF(InitZillaRail), &s_ZillaRail[0][4]},
        {SPR_ZILLA_RAIL, 'A', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[0][5]},
        {SPR_ZILLA_RAIL, 'B', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[0][6]},
        {SPR_ZILLA_RAIL, 'C', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[0][7]},
        {SPR_ZILLA_RAIL, 'D', SF_QUICK_CALL, &AF(InitZillaRail), &s_ZillaRail[0][8]},
        {SPR_ZILLA_RAIL, 'A', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[0][9]},
        {SPR_ZILLA_RAIL, 'B', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[0][10]},
        {SPR_ZILLA_RAIL, 'C', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[0][11]},
        {SPR_ZILLA_RAIL, 'D', SF_QUICK_CALL, &AF(InitZillaRail), &s_ZillaRail[0][12]},
        {SPR_ZILLA_RAIL, 'D', ZILLA_RAIL_RATE, &AF(NullZilla), &s_ZillaRail[0][13]},
        {SPR_ZILLA_RAIL, 'D', SF_QUICK_CALL, &AF(InitActorDecide), &s_ZillaRail[0][0]}
    },
};

STATE* sg_ZillaRail[] =
{
    &s_ZillaRail[0][0],
};

//////////////////////
//
// ZILLA ROCKET
//
//////////////////////

#define ZILLA_ROCKET_RATE 12

STATE s_ZillaRocket[1][7] =
{
    {
        {SPR_ZILLA_ROCKET, 'A', ZILLA_ROCKET_RATE, &AF(NullZilla), &s_ZillaRocket[0][1]},
        {SPR_ZILLA_ROCKET, 'B', ZILLA_ROCKET_RATE, &AF(NullZilla), &s_ZillaRocket[0][2]},
        {SPR_ZILLA_ROCKET, 'C', ZILLA_ROCKET_RATE*4, &AF(NullZilla), &s_ZillaRocket[0][3]},
        {SPR_ZILLA_ROCKET, 'C', SF_QUICK_CALL, &AF(InitZillaRocket), &s_ZillaRocket[0][4]},
        {SPR_ZILLA_ROCKET, 'C', ZILLA_ROCKET_RATE*4, &AF(NullZilla), &s_ZillaRocket[0][5]},
        {SPR_ZILLA_ROCKET, 'D', SF_QUICK_CALL, &AF(InitActorDecide), &s_ZillaRocket[0][6]},
        {SPR_ZILLA_ROCKET, 'D', ZILLA_ROCKET_RATE*10, &AF(NullZilla), &s_ZillaRocket[0][5]}
    },
};

STATE* sg_ZillaRocket[] =
{
    &s_ZillaRocket[0][0],
};

//////////////////////
//
// ZILLA UZI
//
//////////////////////

#define ZILLA_UZI_RATE 8

STATE s_ZillaUzi[1][17] =
{
    {
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[0][1]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[0][2]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[0][3]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[0][4]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[0][5]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[0][6]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[0][7]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[0][8]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[0][9]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[0][10]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[0][11]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[0][12]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[0][13]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[0][14]},
        {SPR_ZILLA_SHOOT, 'A', ZILLA_UZI_RATE, &AF(NullZilla), &s_ZillaUzi[0][15]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ZillaUzi[0][16]},
        {SPR_ZILLA_SHOOT, 'A', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_ZillaUzi[0][16]},
    },
};


STATE* sg_ZillaUzi[] =
{
    s_ZillaUzi[0],
    s_ZillaUzi[1],
    s_ZillaUzi[2],
    s_ZillaUzi[3],
    s_ZillaUzi[4]
};


//////////////////////
//
// ZILLA DIE
//
//////////////////////

#define ZILLA_DIE_RATE 30

STATE s_ZillaDie[] =
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

STATE* sg_ZillaDie[] =
{
    s_ZillaDie
};

STATE s_ZillaDead[] =
{
    {SPR_ZILLA_DEAD, 'A', ZILLA_DIE_RATE, &AF(DoActorDebris), &s_ZillaDead[0]},
};

STATE* sg_ZillaDead[] =
{
    s_ZillaDead
};

ACTOR_ACTION_SET ZillaActionSet =
{
    sg_ZillaStand,
    sg_ZillaRun,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr, //climb
    sg_ZillaPain, //pain
    sg_ZillaDie,
    nullptr,
    sg_ZillaDead,
    nullptr,
    nullptr,
    {sg_ZillaUzi,sg_ZillaRail},
    {950,1024},
    {sg_ZillaUzi,sg_ZillaRocket,sg_ZillaRail},
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

    ChangeState(actor,s_ZillaRun[0]);
    actor->user.__legacyState.Attrib = &ZillaAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.__legacyState.StateEnd = s_ZillaDie;
    actor->user.__legacyState.Rot = sg_ZillaRun;

    EnemyDefaults(actor, &ZillaActionSet, &ZillaPersonality);

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

    SAVE_DATA(s_ZillaRun),
    SAVE_DATA(sg_ZillaRun),
    SAVE_DATA(s_ZillaStand),
    SAVE_DATA(sg_ZillaStand),
    SAVE_DATA(s_ZillaPain),
    SAVE_DATA(sg_ZillaPain),
    SAVE_DATA(s_ZillaRail),
    SAVE_DATA(sg_ZillaRail),
    SAVE_DATA(s_ZillaRocket),
    SAVE_DATA(sg_ZillaRocket),
    SAVE_DATA(s_ZillaUzi),
    SAVE_DATA(sg_ZillaUzi),
    SAVE_DATA(s_ZillaDie),
    SAVE_DATA(sg_ZillaDie),
    SAVE_DATA(s_ZillaDead),
    SAVE_DATA(sg_ZillaDead),

    SAVE_DATA(ZillaActionSet),
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
