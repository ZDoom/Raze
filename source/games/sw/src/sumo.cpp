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
#include "weapon.h"
#include "sector.h"
#include "gamecontrol.h"
#include "mapinfo.h"
#include "v_draw.h"

BEGIN_SW_NS

extern int InitSumoNapalm(DSWActor*);
extern int InitSumoStompAttack(DSWActor*);
extern int InitMiniSumoClap(DSWActor*);
extern int InitSumoSkull(DSWActor*);

extern uint8_t playTrack;
bool bosswasseen[3];

DSWActor* BossSpriteNum[3];


DECISION SumoBattle[] =
{
    {690,   &AF(InitActorMoveCloser)   },
    {692,   &AF(InitActorSetDecide)   },
    {1024,  &AF(InitActorAttack    )   }
};

DECISION SumoOffense[] =
{
    {690,   &AF(InitActorMoveCloser)   },
    {692,   &AF(InitActorSetDecide)   },
    {1024,  &AF(InitActorAttack    )   }
};

DECISIONB SumoBroadcast[] =
{
    {2,    attr_alert },
    {4,    attr_ambient  },
    {1024, 0 }
};

DECISION SumoSurprised[] =
{
    {700,   &AF(InitActorMoveCloser)   },
    {703,   &AF(InitActorSetDecide)   },
    {1024,  &AF(InitActorDecide    )   }
};

DECISION SumoEvasive[] =
{
    {1024, &AF(InitActorAttack) }
};

DECISION SumoLostTarget[] =
{
    {900,   &AF(InitActorFindPlayer  )       },
    {1024,  &AF(InitActorWanderAround)       }
};

DECISION SumoCloseRange[] =
{
    {1024,  &AF(InitActorAttack)         }
};

PERSONALITY SumoPersonality =
{
    SumoBattle,
    SumoOffense,
    SumoBroadcast,
    SumoSurprised,
    SumoEvasive,
    SumoLostTarget,
    SumoCloseRange,
    SumoCloseRange
};

ATTRIBUTE SumoAttrib =
{
    {160, 180, 180, 180},             // Speeds
    {3, 0, 0, 0},                     // Tic Adjusts
    3,                               // MaxWeapons;
    {
        DIGI_SUMOAMBIENT, DIGI_SUMOALERT, DIGI_SUMOSCREAM,
        DIGI_SUMOPAIN, DIGI_SUMOSCREAM, 0,0,0,0,0
    }
};


//////////////////////
//
// SUMO RUN
//
//////////////////////

#define SUMO_RATE 24

FState s_SumoRun[] =
{
        {SPR_SUMO_RUN, 'A', SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[1]},
        {SPR_SUMO_RUN, 'B', SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[2]},
        {SPR_SUMO_RUN, 'C', SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[3]},
        {SPR_SUMO_RUN, 'D', SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[0]}
};

//////////////////////
//
// SUMO STAND
//
//////////////////////

FState s_SumoStand[] =
{
        {SPR_SUMO_RUN, 'A', SUMO_RATE, &AF(DoSumoMove), &s_SumoStand[0]}
};

//////////////////////
//
// SUMO PAIN
//
//////////////////////

#define SUMO_PAIN_RATE 30

FState s_SumoPain[] =
{
        {SPR_SUMO_PAIN, 'A', SUMO_PAIN_RATE, &AF(NullSumo), &s_SumoPain[1]},
        {SPR_SUMO_PAIN, 'A', 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoPain[0]}
};

//////////////////////
//
// SUMO FART
//
//////////////////////

#define SUMO_FART_RATE 12

FState s_SumoFart[] =
{
        {SPR_SUMO_FART, 'A', SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[1]},
        {SPR_SUMO_FART, 'A', SF_QUICK_CALL, &AF(InitSumoFart), &s_SumoFart[2]},
        {SPR_SUMO_FART, 'B', SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[3]},
        {SPR_SUMO_FART, 'C', SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[4]},
        {SPR_SUMO_FART, 'D', SUMO_FART_RATE*10, &AF(NullSumo), &s_SumoFart[5]},
        {SPR_SUMO_FART, 'D', SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoFart[0]}
};

//////////////////////
//
// SUMO CLAP
//
//////////////////////

#define SUMO_CLAP_RATE 12

FState s_SumoClap[] =
{
        {SPR_SUMO_CLAP, 'A', SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[1]},
        {SPR_SUMO_CLAP, 'B', SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[2]},
        {SPR_SUMO_CLAP, 'C', SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[3]},
        {SPR_SUMO_CLAP, 'C', SF_QUICK_CALL, &AF(InitSumoClap), &s_SumoClap[4]},
        {SPR_SUMO_CLAP, 'D', SUMO_CLAP_RATE*10, &AF(NullSumo), &s_SumoClap[5]},
        {SPR_SUMO_CLAP, 'D', SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoClap[5]}
};

//////////////////////
//
// SUMO STOMP
//
//////////////////////

#define SUMO_STOMP_RATE 30

FState s_SumoStomp[] =
{
        {SPR_SUMO_STOMP, 'A', SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[1]},
        {SPR_SUMO_STOMP, 'B', SUMO_STOMP_RATE*3, &AF(NullSumo), &s_SumoStomp[2]},
        {SPR_SUMO_STOMP, 'C', SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[3]},
        {SPR_SUMO_STOMP, 'C', 0|SF_QUICK_CALL, &AF(InitSumoStomp), &s_SumoStomp[4]},
        {SPR_SUMO_STOMP, 'C', 8, &AF(NullSumo), &s_SumoStomp[5]},
        {SPR_SUMO_STOMP, 'C', 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoStomp[5]}
};

//////////////////////
//
// SUMO DIE
//
//////////////////////

#define SUMO_DIE_RATE 30

FState s_SumoDie[] =
{
    {SPR_SUMO_DIE, 'A', SUMO_DIE_RATE*2, &AF(NullSumo), &s_SumoDie[1]},
    {SPR_SUMO_DIE, 'B', SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[2]},
    {SPR_SUMO_DIE, 'C', SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[3]},
    {SPR_SUMO_DIE, 'D', SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[4]},
    {SPR_SUMO_DIE, 'E', SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[5]},
    {SPR_SUMO_DIE, 'F', SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[6]},
    {SPR_SUMO_DIE, 'G', SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[7]},
    {SPR_SUMO_DIE, 'G', SUMO_DIE_RATE*3, &AF(NullSumo), &s_SumoDie[8]},
    {SPR_SUMO_DIE, 'H', SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[9]},
    {SPR_SUMO_DIE, 'G', SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[10]},
    {SPR_SUMO_DIE, 'H', SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[11]},
    {SPR_SUMO_DIE, 'G', SUMO_DIE_RATE-8, &AF(NullSumo), &s_SumoDie[12]},
    {SPR_SUMO_DIE, 'H', SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[13]},
    {SPR_SUMO_DIE, 'H', SF_QUICK_CALL, &AF(DoSumoDeathMelt), &s_SumoDie[14]},
    {SPR_SUMO_DIE, 'G', SUMO_DIE_RATE-15, &AF(NullSumo), &s_SumoDie[15]},
    {SPR_SUMO_DEAD, 'A', SF_QUICK_CALL, &AF(QueueFloorBlood), &s_SumoDie[16]},
    {SPR_SUMO_DEAD, 'A', SUMO_DIE_RATE, &AF(DoActorDebris), &s_SumoDie[16]}
};

FState s_SumoDead[] =
{
    {SPR_SUMO_DEAD, 'A', SUMO_DIE_RATE, &AF(DoActorDebris), &s_SumoDead[0]},
};

ACTOR_ACTION_SET SumoActionSet =
{
    s_SumoStand,
    s_SumoRun,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr, //climb
    s_SumoPain, //pain
    s_SumoDie,
    nullptr,
    s_SumoDead,
    nullptr,
    nullptr,
    {s_SumoStomp,s_SumoFart},
    {800,1024},
    {s_SumoClap,s_SumoStomp,s_SumoFart},
    {400,750,1024},
    {nullptr},
    nullptr,
    nullptr
};

ACTOR_ACTION_SET MiniSumoActionSet =
{
    s_SumoStand,
    s_SumoRun,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr, //climb
    s_SumoPain, //pain
    s_SumoDie,
    nullptr,
    s_SumoDead,
    nullptr,
    nullptr,
    {s_SumoClap},
    {1024},
    {s_SumoClap},
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

int SetupSumo(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,SUMO_RUN_R0,s_SumoRun[0]);
        actor->user.Health = 6000;
    }

    if (Skill == 0) actor->user.Health = 2000;
    if (Skill == 1) actor->user.Health = 4000;

    ChangeState(actor,s_SumoRun[0]);
    actor->user.__legacyState.Attrib = &SumoAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.__legacyState.StateEnd = s_SumoDie;
    actor->user.__legacyState.Rot = s_SumoRun;

    EnemyDefaults(actor, &SumoActionSet, &SumoPersonality);

    actor->clipdist = 32;
    if (actor->spr.pal == 16)
    {
        // Mini Sumo
        actor->spr.scale = DVector2(0.671875, 0.453125);
        actor->user.__legacyState.ActorActionSet = &MiniSumoActionSet;
        actor->user.Health = 500;
    }
    else
    {
        actor->spr.scale = DVector2(1.796875, 1.171875);
    }

    //actor->user.Flags |= (SPR_XFLIP_TOGGLE);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWSumo, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupSumo(self);
    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullSumo(DSWActor* actor)
{
    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSumoMove(DSWActor* actor)
{
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

int DoSumoRumble(DSWActor* actor)
{
    SetSumoQuake(actor);
    return 0;
}

int InitSumoFart(DSWActor* actor)
{
    PlaySound(DIGI_SUMOFART, actor, v3df_follow);
    InitChemBomb(actor);
    SetSumoFartQuake(actor);
    InitSumoNapalm(actor);
    return 0;
}

int InitSumoStomp(DSWActor* actor)
{
    PlaySound(DIGI_SUMOSTOMP, actor, v3df_none);
    SetSumoQuake(actor);
    InitSumoStompAttack(actor);
    return 0;
}

int InitSumoClap(DSWActor* actor)
{
    if (actor->spr.pal == 16 && RandomRange(1000) <= 800)
        InitMiniSumoClap(actor);
    else
        InitSumoSkull(actor);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSumoDeathMelt(DSWActor* actor)
{
    PlaySound(DIGI_SUMOFART, actor, v3df_follow);

    actor->user.ID = SUMO_RUN_R0;
    InitChemBomb(actor);
    actor->user.ID = 0;

    DoMatchEverything(nullptr, actor->spr.lotag, 1);
    if (!SW_SHAREWARE)
    {
        // Resume the regular music - in a hack-free fashion.
        PlaySong(currentLevel->music.GetChars(), currentLevel->cdSongId);
    }

    BossSpriteNum[1] = nullptr; // Sprite is gone, set it back to keep it valid!

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BossHealthMeter(void)
{
    DSWPlayer* pp = getPlayer(myconnectindex);
    short color=0,metertics,meterunit;
    int y;
    extern bool NoMeters;
    short health;
    static bool triedplay = false;

    if (NoMeters) return;

    if (!(currentLevel->gameflags & (LEVEL_SW_BOSSMETER_SERPENT | LEVEL_SW_BOSSMETER_SUMO | LEVEL_SW_BOSSMETER_ZILLA))) return;

    // Don't draw bar for other players
    if (pp != getPlayer(myconnectindex))
        return;

    // all enemys
    if (currentLevel->gameflags & (LEVEL_SW_BOSSMETER_SERPENT|LEVEL_SW_BOSSMETER_SUMO|LEVEL_SW_BOSSMETER_ZILLA) &&
        BossSpriteNum[0] == nullptr && BossSpriteNum[1] == nullptr && BossSpriteNum[2] == nullptr)
    {
        SWStatIterator it(STAT_ENEMY);
        while (auto itActor = it.Next())
        {
            if ((itActor->user.ID == SERP_RUN_R0 || itActor->user.ID == SUMO_RUN_R0 || itActor->user.ID == ZILLA_RUN_R0) && itActor->spr.pal != 16)
            {
                if (itActor->user.ID == SERP_RUN_R0 && (currentLevel->gameflags & LEVEL_SW_BOSSMETER_SERPENT))
                    BossSpriteNum[0] = itActor;
                else if (itActor->user.ID == SUMO_RUN_R0 && (currentLevel->gameflags & LEVEL_SW_BOSSMETER_SUMO))
                    BossSpriteNum[1] = itActor;
                else if (itActor->user.ID == ZILLA_RUN_R0 && (currentLevel->gameflags & LEVEL_SW_BOSSMETER_ZILLA))
                    BossSpriteNum[2] = itActor;
            }
        }
    }

    if (BossSpriteNum[0] == nullptr && BossSpriteNum[1] == nullptr && BossSpriteNum[2] == nullptr)
        return;


    // Only show the meter when you can see the boss
    for (int i=0; i<3; i++)
    {
        DSWActor* actor = BossSpriteNum[i];
        if (actor != nullptr && !bosswasseen[i])
        {
            if (cansee(ActorVectOfTop(actor), actor->sector(), pp->GetActor()->getPosWithOffsetZ().plusZ(-40), pp->cursector))
            {
                if (i == 0 && !bosswasseen[0])
                {
                    bosswasseen[0] = true;
                    if (!SW_SHAREWARE)
                    {
                        PlaySong(ThemeSongs[2].GetChars(), ThemeTrack[2], true);
                    }
                }
                else if (i == 1 && !bosswasseen[1])
                {
                    bosswasseen[1] = true;
                    if (!SW_SHAREWARE)
                    {
                        PlaySong(ThemeSongs[3].GetChars(), ThemeTrack[3], true);
                    }
                }
                else if (i == 2 && !bosswasseen[2])
                {
                    bosswasseen[2] = true;
                    if (!SW_SHAREWARE)
                    {
                        PlaySong(ThemeSongs[4].GetChars(), ThemeTrack[4], true);
                    }
                }
            }
        }
    }


    for (int i=0; i<3; i++)
    {
        DSWActor* actor = BossSpriteNum[i];
        if ((!bosswasseen[i] || actor == nullptr))
            continue;

        if (actor->user.ID == SERP_RUN_R0 && bosswasseen[0])
        {
            if (Skill == 0) health = 1100;
            else if (Skill == 1) health = 2200;
            else
                health = HEALTH_SERP_GOD;
            meterunit = health / 30;
        }
        else if (actor->user.ID == SUMO_RUN_R0 && bosswasseen[1])
        {
            if (Skill == 0) health = 2000;
            else if (Skill == 1) health = 4000;
            else
                health = 6000;
            meterunit = health / 30;
        }
        else if (actor->user.ID == ZILLA_RUN_R0 && bosswasseen[2])
        {
            if (Skill == 0) health = 2000;
            else if (Skill == 1) health = 4000;
            else
                health = 6000;
            meterunit = health / 30;
        }
        else
            continue;

        if (meterunit > 0)
        {
            if (actor->user.Health < meterunit && actor->user.Health > 0)
                metertics = 1;
            else
                metertics = actor->user.Health / meterunit;
        }
        else
            continue;

        if (metertics <= 0)
        {
            continue;
        }

        if (numplayers < 2) y = 10;
        else if (numplayers >=2 && numplayers <= 4) y = 20;
        else
            y = 30;

        if ((currentLevel->gameflags & (LEVEL_SW_BOSSMETER_SUMO|LEVEL_SW_BOSSMETER_ZILLA)) == (LEVEL_SW_BOSSMETER_SUMO | LEVEL_SW_BOSSMETER_ZILLA) && numplayers >= 2)
        {
            if (actor->user.ID == SUMO_RUN_R0 && bosswasseen[1]) y += 10;
            else if (actor->user.ID == ZILLA_RUN_R0 && bosswasseen[2]) y += 20;
        }

        if (metertics <= 12 && metertics > 6)
            color = 20;
        else if (metertics <= 6)
            color = 25;
        else
            color = 22;

        DrawTexture(twod, tileGetTexture(5407, true), 85, y, DTA_FullscreenScale, FSMode_Fit320x200,
            DTA_CenterOffsetRel, 2, DTA_TranslationIndex, TRANSLATION(Translation_Remap, 1), TAG_DONE);

        DrawTexture(twod, tileGetTexture(5406 - metertics, true), 147, y, DTA_FullscreenScale, FSMode_Fit320x200,
            DTA_CenterOffsetRel, 2, DTA_TranslationIndex, TRANSLATION(Translation_Remap, color), TAG_DONE);
    }

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------


#include "saveable.h"

static saveable_data saveable_sumo_data[] =
{
    SAVE_DATA(SumoPersonality),

    SAVE_DATA(SumoAttrib),

    SAVE_DATA(s_SumoRun),
    SAVE_DATA(s_SumoStand),
    SAVE_DATA(s_SumoPain),
    SAVE_DATA(s_SumoFart),
    SAVE_DATA(s_SumoClap),
    SAVE_DATA(s_SumoStomp),
    SAVE_DATA(s_SumoDie),
    SAVE_DATA(s_SumoDead),

    SAVE_DATA(SumoActionSet),
    SAVE_DATA(MiniSumoActionSet),
};

saveable_module saveable_sumo =
{
    // code
    nullptr, 0,

    // data
    saveable_sumo_data,
    SIZ(saveable_sumo_data)
};
END_SW_NS
