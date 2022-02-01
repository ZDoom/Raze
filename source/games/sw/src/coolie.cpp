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
#include "tags.h"
#include "ai.h"
#include "sprite.h"
#include "misc.h"
#include "weapon.h"

BEGIN_SW_NS

ANIMATOR InitCoolieCharge;

DECISION CoolieBattle[] =
{
    {700,   InitCoolieCharge         },
    {990,   InitActorMoveCloser         },
    {1000,  InitActorAttackNoise        },
    {1024,  InitActorRunAway           }
};

DECISION CoolieOffense[] =
{
    {700,   InitCoolieCharge         },
    {1015,  InitActorMoveCloser         },
    {1024,  InitActorAttackNoise        }
};

DECISION CoolieBroadcast[] =
{
    //{1,     InitActorAlertNoise        },
    {16,    InitActorAmbientNoise          },
    {1024,  InitActorDecide            }
};

DECISION CoolieSurprised[] =
{
    {700,   InitActorMoveCloser         },
    {703,   InitActorAmbientNoise        },
    {1024,  InitActorDecide            }
};

DECISION CoolieEvasive[] =
{
    {10,   InitActorEvade  },
    {1024, nullptr            }
};

DECISION CoolieLostTarget[] =
{
    {900,   InitActorFindPlayer         },
    {1024,  InitActorWanderAround       }
};

DECISION CoolieCloseRange[] =
{
    {400,   InitCoolieCharge         },
    {1024,  InitActorReposition            }
};

PERSONALITY CooliePersonality =
{
    CoolieBattle,
    CoolieOffense,
    CoolieBroadcast,
    CoolieSurprised,
    CoolieEvasive,
    CoolieLostTarget,
    CoolieCloseRange,
    CoolieCloseRange
};

ATTRIBUTE CoolieAttrib =
{
    {60, 80, 100, 200},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        DIGI_COOLIEAMBIENT, DIGI_COOLIEALERT, DIGI_COOLIEALERT,
        DIGI_COOLIEPAIN, 0, DIGI_CGMATERIALIZE,
        DIGI_COOLIEEXPLODE,0,0,0
    }
};


//////////////////////
//
// COOLIE RUN
//
//////////////////////

#define COOLIE_RATE 12

ANIMATOR DoCoolieMove,NullCoolie,DoStayOnFloor,
         DoActorDebris, SpawnCoolieExp,
         SpawnCoolg;

STATE s_CoolieRun[5][4] =
{
    {
        {COOLIE_RUN_R0 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[0][1]},
        {COOLIE_RUN_R0 + 1, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[0][2]},
        {COOLIE_RUN_R0 + 2, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[0][3]},
        {COOLIE_RUN_R0 + 3, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[0][0]}
    },
    {
        {COOLIE_RUN_R1 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[1][1]},
        {COOLIE_RUN_R1 + 1, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[1][2]},
        {COOLIE_RUN_R1 + 2, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[1][3]},
        {COOLIE_RUN_R1 + 3, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[1][0]}
    },
    {
        {COOLIE_RUN_R2 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[2][1]},
        {COOLIE_RUN_R2 + 1, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[2][2]},
        {COOLIE_RUN_R2 + 2, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[2][3]},
        {COOLIE_RUN_R2 + 3, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[2][0]}
    },
    {
        {COOLIE_RUN_R3 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[3][1]},
        {COOLIE_RUN_R3 + 1, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[3][2]},
        {COOLIE_RUN_R3 + 2, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[3][3]},
        {COOLIE_RUN_R3 + 3, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[3][0]}
    },
    {
        {COOLIE_RUN_R4 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[4][1]},
        {COOLIE_RUN_R4 + 1, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[4][2]},
        {COOLIE_RUN_R4 + 2, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[4][3]},
        {COOLIE_RUN_R4 + 3, COOLIE_RATE, DoCoolieMove, &s_CoolieRun[4][0]},
    }
};

STATE* sg_CoolieRun[] =
{
    &s_CoolieRun[0][0],
    &s_CoolieRun[1][0],
    &s_CoolieRun[2][0],
    &s_CoolieRun[3][0],
    &s_CoolieRun[4][0]
};

//////////////////////
//
// COOLIE CHARGE
//
//////////////////////

#define COOLIE_RATE 12

ANIMATOR DoCoolieMove,NullCoolie,DoStayOnFloor,
         DoActorDebris, SpawnCoolieExp,
         SpawnCoolg;

STATE s_CoolieCharge[5][4] =
{
    {
        {COOLIE_CHARGE_R0 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[0][1]},
        {COOLIE_CHARGE_R0 + 1, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[0][2]},
        {COOLIE_CHARGE_R0 + 2, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[0][3]},
        {COOLIE_CHARGE_R0 + 3, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[0][0]}
    },
    {
        {COOLIE_CHARGE_R1 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[1][1]},
        {COOLIE_CHARGE_R1 + 1, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[1][2]},
        {COOLIE_CHARGE_R1 + 2, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[1][3]},
        {COOLIE_CHARGE_R1 + 3, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[1][0]}
    },
    {
        {COOLIE_CHARGE_R2 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[2][1]},
        {COOLIE_CHARGE_R2 + 1, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[2][2]},
        {COOLIE_CHARGE_R2 + 2, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[2][3]},
        {COOLIE_CHARGE_R2 + 3, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[2][0]}
    },
    {
        {COOLIE_CHARGE_R3 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[3][1]},
        {COOLIE_CHARGE_R3 + 1, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[3][2]},
        {COOLIE_CHARGE_R3 + 2, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[3][3]},
        {COOLIE_CHARGE_R3 + 3, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[3][0]}
    },
    {
        {COOLIE_CHARGE_R4 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[4][1]},
        {COOLIE_CHARGE_R4 + 1, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[4][2]},
        {COOLIE_CHARGE_R4 + 2, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[4][3]},
        {COOLIE_CHARGE_R4 + 3, COOLIE_RATE, DoCoolieMove, &s_CoolieCharge[4][0]},
    }
};

STATE* sg_CoolieCharge[] =
{
    &s_CoolieCharge[0][0],
    &s_CoolieCharge[1][0],
    &s_CoolieCharge[2][0],
    &s_CoolieCharge[3][0],
    &s_CoolieCharge[4][0]
};

//////////////////////
//
// COOLIE STAND
//
//////////////////////

STATE s_CoolieStand[5][6] =
{
    {
        {COOLIE_RUN_R0 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieStand[0][0]}
    },
    {
        {COOLIE_RUN_R1 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieStand[1][0]}
    },
    {
        {COOLIE_RUN_R2 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieStand[2][0]}
    },
    {
        {COOLIE_RUN_R3 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieStand[3][0]}
    },
    {
        {COOLIE_RUN_R4 + 0, COOLIE_RATE, DoCoolieMove, &s_CoolieStand[4][0]}
    }
};

STATE* sg_CoolieStand[] =
{
    &s_CoolieStand[0][0],
    &s_CoolieStand[1][0],
    &s_CoolieStand[2][0],
    &s_CoolieStand[3][0],
    &s_CoolieStand[4][0]
};

//////////////////////
//
// COOLIE PAIN
//
//////////////////////

#define COOLIE_PAIN_RATE 60
ANIMATOR CooliePain;

STATE s_CooliePain[5][1] =
{
    {
        {COOLIE_PAIN_R0 + 0, COOLIE_PAIN_RATE, CooliePain, &s_CooliePain[0][0]},
        //{COOLIE_PAIN_R0 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_CooliePain[0][0]}
    },
    {
        {COOLIE_PAIN_R1 + 0, COOLIE_PAIN_RATE, CooliePain, &s_CooliePain[1][0]},
        //{COOLIE_PAIN_R1 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_CooliePain[1][0]}
    },
    {
        {COOLIE_PAIN_R2 + 0, COOLIE_PAIN_RATE, CooliePain, &s_CooliePain[2][0]},
        //{COOLIE_PAIN_R2 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_CooliePain[2][0]}
    },
    {
        {COOLIE_PAIN_R3 + 0, COOLIE_PAIN_RATE, CooliePain, &s_CooliePain[3][0]},
        //{COOLIE_PAIN_R3 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_CooliePain[3][0]}
    },
    {
        {COOLIE_PAIN_R4 + 0, COOLIE_PAIN_RATE, CooliePain, &s_CooliePain[4][0]},
        //{COOLIE_PAIN_R4 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_CooliePain[4][0]}
    }
};

STATE* sg_CooliePain[] =
{
    &s_CooliePain[0][0],
    &s_CooliePain[1][0],
    &s_CooliePain[2][0],
    &s_CooliePain[3][0],
    &s_CooliePain[4][0]
};

//////////////////////
//
// COOLIE DIE
//
//////////////////////

#define COOLIE_DIE_RATE 30
ANIMATOR DoCoolieWaitBirth;

STATE s_CoolieDie[] =
{
    {COOLIE_DIE + 0, COOLIE_DIE_RATE, NullCoolie, &s_CoolieDie[1]},

    {COOLIE_DIE + 0, 0|SF_QUICK_CALL, SpawnCoolieExp, &s_CoolieDie[2]},

    {COOLIE_DIE + 1, COOLIE_DIE_RATE, NullCoolie, &s_CoolieDie[3]},
    {COOLIE_DIE + 2, COOLIE_DIE_RATE, NullCoolie, &s_CoolieDie[4]},
    {COOLIE_DIE + 3, COOLIE_DIE_RATE, NullCoolie, &s_CoolieDie[5]},
    {COOLIE_DIE + 4, COOLIE_DIE_RATE, NullCoolie, &s_CoolieDie[6]},
    {COOLIE_DIE + 5, COOLIE_DIE_RATE, NullCoolie, &s_CoolieDie[7]},
    {COOLIE_DIE + 6, COOLIE_DIE_RATE, NullCoolie, &s_CoolieDie[8]},
    {COOLIE_DIE + 7, COOLIE_DIE_RATE, DoCoolieWaitBirth, &s_CoolieDie[8]},

    {COOLIE_DIE + 7, COOLIE_DIE_RATE*5, DoActorDebris, &s_CoolieDie[10]},
    {COOLIE_DIE + 7, 0|SF_QUICK_CALL, SpawnCoolg, &s_CoolieDie[11]},
    {COOLIE_DEAD_NOHEAD, SF_QUICK_CALL, QueueFloorBlood, &s_CoolieDie[12]},
    {COOLIE_DEAD_NOHEAD, COOLIE_DIE_RATE, DoActorDebris, &s_CoolieDie[12]}
};

STATE* sg_CoolieDie[] =
{
    s_CoolieDie
};

STATE s_CoolieDead[] =
{
    {COOLIE_DEAD, COOLIE_DIE_RATE, DoActorDebris, &s_CoolieDead[0]},
};

STATE* sg_CoolieDead[] =
{
    s_CoolieDead
};

ACTOR_ACTION_SET CoolieActionSet =
{
    sg_CoolieStand,
    sg_CoolieRun,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr, //climb
    sg_CooliePain, //pain
    sg_CoolieDie,
    nullptr,
    sg_CoolieDead,
    nullptr,
    nullptr,
    {sg_CoolieCharge},
    {1024},
    {sg_CoolieCharge},
    {1024},
    {nullptr},
    nullptr,
    nullptr
};

// later. This is used by multiple enemies.
void EnemyDefaults(DSWActor* actor, ACTOR_ACTION_SET* action, PERSONALITY* person)
{
    unsigned int wpn;
    int wpn_cnt;
    int depth = 0;

    switch (actor->user.ID)
    {
    case PACHINKO1:
    case PACHINKO2:
    case PACHINKO3:
    case PACHINKO4:
    case 623:
    case TOILETGIRL_R0:
    case WASHGIRL_R0:
    case CARGIRL_R0:
    case MECHANICGIRL_R0:
    case SAILORGIRL_R0:
    case PRUNEGIRL_R0:
    case TRASHCAN:
    case BUNNY_RUN_R0:
        break;
    default:
    {
        TotalKillable++;
    }

    break;
    }

    actor->spr.cstat &= ~(CSTAT_SPRITE_RESTORE);

    actor->user.spal = actor->spr.pal;

    actor->user.RotNum = 5;
    actor->spr.clipdist = (256) >> 2;

    actor->user.zclip = Z(48);
    actor->user.lo_step = Z(32);

    actor->user.floor_dist = actor->user.zclip - actor->user.lo_step;
    actor->user.ceiling_dist = ActorSizeZ(actor) - actor->user.zclip;

    actor->user.Radius = 400;

    actor->user.MaxHealth = actor->user.Health;

    actor->user.PainThreshold = (actor->user.Health >> 4) - 1;

    actor->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    actor->spr.extra |= (SPRX_PLAYER_OR_ENEMY);

    actor->spr.picnum = actor->user.State->Pic;
    change_actor_stat(actor, STAT_ENEMY);

    actor->user.Personality = person;
    actor->user.ActorActionSet = action;

    DoActorZrange(actor);

    //KeepActorOnFloor(actor); // for swimming actors

    // make sure we start in the water if thats where we are
    if (actor->user.lo_sectp)
    {

        if (actor->user.lo_sectp->hasU() && (actor->user.lo_sectp->extra & SECTFX_SINK))
        {
            depth = FixedToInt(actor->user.lo_sectp->depth_fixed);
        }
        else
        {
            SWSectIterator it(actor->user.lo_sectp);
            while (auto itActor = it.Next())
            {
                if (itActor->spr.picnum == ST1 && itActor->spr.hitag == SECT_SINK)
                {
                    depth = itActor->spr.lotag;
                }
            }
        }
    }

    if (depth && labs(actor->int_pos().Z - actor->user.loz) < Z(8))
    {
        actor->add_int_z(Z(depth));
        actor->user.loz = actor->int_pos().Z;
        actor->backupz();
    }

    if (!action)
        return;

    NewStateGroup(actor, actor->user.ActorActionSet->Run);

    actor->user.ActorActionFunc = DoActorDecide;

    // find the number of long range attacks
    for (wpn = wpn_cnt = 0; wpn < SIZ(actor->user.ActorActionSet->Attack); wpn++)
    {
        if (actor->user.ActorActionSet->Attack[wpn])
            wpn_cnt++;
        else
            break;
    }

    // for actors this tells the number of weapons available
    // for player it tells the current weapon
    actor->user.WeaponNum = int8_t(wpn_cnt);
}

int SetupCoolie(DSWActor* actor)
{
    ANIMATOR DoActorDecide;

    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,COOLIE_RUN_R0,s_CoolieRun[0]);
        actor->user.Health = HEALTH_COOLIE;
    }

    ChangeState(actor,s_CoolieRun[0]);
    actor->user.Attrib = &CoolieAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.StateEnd = s_CoolieDie;
    actor->user.Rot = sg_CoolieRun;

    EnemyDefaults(actor, &CoolieActionSet, &CooliePersonality);

    actor->spr.xrepeat = 42;
    actor->spr.yrepeat = 42;

    actor->user.Flags |= (SPR_XFLIP_TOGGLE);

    return 0;
}


int NewCoolg(DSWActor*);
int SpawnCoolg(DSWActor* actor)
{
    // Don't do a ghost every time
    if (RandomRange(1000) > 700 || Skill < MinEnemySkill - 1)
    {
        return(0);
    }

    NewCoolg(actor);
    PlaySpriteSound(actor,attr_extra1,v3df_follow);

    return 0;
}

int CooliePain(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(actor);

    return 0;
}

int NullCoolie(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    return 0;
}

int DoCoolieMove(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        (*actor->user.ActorActionFunc)(actor);

    KeepActorOnFloor(actor);

    if (DoActorSectorDamage(actor))
    {
        return 0;
    }

    if (Distance(actor->int_pos().X, actor->int_pos().Y, actor->user.targetActor->int_pos().X, actor->user.targetActor->int_pos().Y) < 1200)
    {
        UpdateSinglePlayKills(actor);
        DoActorDie(actor, actor, 0);
        return 0;
    }

    return 0;
}

int InitCoolieCharge(DSWActor* actor)
{
    if (RANDOM_P2(1024) > 950)
        PlaySound(DIGI_COOLIESCREAM, actor, v3df_follow);

    DoActorSetSpeed(actor, FAST_SPEED);

    InitActorMoveCloser(actor);

    NewStateGroup(actor, sg_CoolieCharge);

    return 0;
}


int DoCoolieWaitBirth(DSWActor* actor)
{
    if ((actor->user.Counter -= ACTORMOVETICS) <= 0)
    {
        ChangeState(actor,&s_CoolieDie[9]);
    }

    return 0;
}


#include "saveable.h"

static saveable_code saveable_coolie_code[] =
{
    SAVE_CODE(SpawnCoolg),
    SAVE_CODE(CooliePain),
    SAVE_CODE(NullCoolie),
    SAVE_CODE(DoCoolieMove),
    SAVE_CODE(InitCoolieCharge),
    SAVE_CODE(DoCoolieWaitBirth),
};

static saveable_data saveable_coolie_data[] =
{
    SAVE_DATA(CoolieBattle),
    SAVE_DATA(CoolieOffense),
    SAVE_DATA(CoolieBroadcast),
    SAVE_DATA(CoolieSurprised),
    SAVE_DATA(CoolieEvasive),
    SAVE_DATA(CoolieLostTarget),
    SAVE_DATA(CoolieCloseRange),

    SAVE_DATA(CooliePersonality),

    SAVE_DATA(CoolieAttrib),

    SAVE_DATA(s_CoolieRun),
    SAVE_DATA(sg_CoolieRun),
    SAVE_DATA(s_CoolieCharge),
    SAVE_DATA(sg_CoolieCharge),
    SAVE_DATA(s_CoolieStand),
    SAVE_DATA(sg_CoolieStand),
    SAVE_DATA(s_CooliePain),
    SAVE_DATA(sg_CooliePain),
    SAVE_DATA(s_CoolieDie),
    SAVE_DATA(sg_CoolieDie),
    SAVE_DATA(s_CoolieDead),
    SAVE_DATA(sg_CoolieDead),

    SAVE_DATA(CoolieActionSet),
};

saveable_module saveable_coolie =
{
    // code
    saveable_coolie_code,
    SIZ(saveable_coolie_code),

    // data
    saveable_coolie_data,
    SIZ(saveable_coolie_data)
};
END_SW_NS
