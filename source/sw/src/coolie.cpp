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
#include "build.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "sprite.h"
#include "actor.h"
#include "track.h"
#include "weapon.h"

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
    {1024, NULL            }
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

STATEp sg_CoolieRun[] =
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

STATEp sg_CoolieCharge[] =
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

STATEp sg_CoolieStand[] =
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

STATEp sg_CooliePain[] =
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

STATEp sg_CoolieDie[] =
{
    s_CoolieDie
};

STATE s_CoolieDead[] =
{
    {COOLIE_DEAD, COOLIE_DIE_RATE, DoActorDebris, &s_CoolieDead[0]},
};

STATEp sg_CoolieDead[] =
{
    s_CoolieDead
};

/*
typedef struct
{
#define MAX_ACTOR_CLOSE_ATTACK 2
#define MAX_ACTOR_ATTACK 6
STATEp *Stand;
STATEp *Run;
STATEp *Jump;
STATEp *Fall;
STATEp *Crawl;
STATEp *Swim;
STATEp *Fly;
STATEp *Rise;
STATEp *Sit;
STATEp *Look;
STATEp *Climb;
STATEp *Pain;
STATEp *Death1;
STATEp *Death2;
STATEp *Dead;
STATEp *DeathJump;
STATEp *DeathFall;

STATEp *CloseAttack[MAX_ACTOR_CLOSE_ATTACK];
short  CloseAttackPercent[MAX_ACTOR_CLOSE_ATTACK];

STATEp *Attack[MAX_ACTOR_ATTACK];
short  AttackPercent[MAX_ACTOR_ATTACK];

STATEp *Special[2];
STATEp *Duck;
STATEp *Dive;
}ACTOR_ACTION_SET,*ACTOR_ACTION_SETp;
*/

ACTOR_ACTION_SET CoolieActionSet =
{
    sg_CoolieStand,
    sg_CoolieRun,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, //climb
    sg_CooliePain, //pain
    sg_CoolieDie,
    NULL,
    sg_CoolieDead,
    NULL,
    NULL,
    {sg_CoolieCharge},
    {1024},
    {sg_CoolieCharge},
    {1024},
    {NULL},
    NULL,
    NULL
};

void EnemyDefaults(short SpriteNum, ACTOR_ACTION_SETp action, PERSONALITYp person)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = &sprite[SpriteNum];
    unsigned int wpn;
    short wpn_cnt;
    short depth = 0;
    extern short TotalKillable;
    extern SWBOOL DebugSecret;

    switch (u->ID)
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
#if DEBUG
        if (DebugSecret)
        {
            sprintf(ds,"COUNTED: spnum %d, pic %d, x %d, y %d",SpriteNum,sp->picnum,sp->x,sp->y);
            DebugWriteString(ds);
        }
#endif
    }

    break;
    }

    RESET(sp->cstat, CSTAT_SPRITE_RESTORE);

    u->spal = sp->pal;

    u->RotNum = 5;
    sp->clipdist = (256) >> 2;

    u->zclip = Z(48);
    u->lo_step = Z(32);

    u->floor_dist = u->zclip - u->lo_step;
    u->ceiling_dist = SPRITEp_SIZE_Z(sp) - u->zclip;

    u->Radius = 400;

    u->MaxHealth = u->Health;

    u->PainThreshold = DIV16(u->Health) - 1;
    //u->PainThreshold = DIV4(u->Health) - 1;

    SET(sp->cstat,CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    SET(sp->extra,SPRX_PLAYER_OR_ENEMY);

    sprite[SpriteNum].picnum = u->State->Pic;
    change_sprite_stat(SpriteNum, STAT_ENEMY);

    u->Personality = person;
    u->ActorActionSet = action;

    DoActorZrange(SpriteNum);

    //KeepActorOnFloor(SpriteNum); // for swimming actors

    // make sure we start in the water if thats where we are
    if (u->lo_sectp) // && SectUser[u->lo_sectp - sector])
    {
        short i,nexti;
        short sectnum = u->lo_sectp - sector;

        if (SectUser[sectnum] && TEST(u->lo_sectp->extra, SECTFX_SINK))
        {
            depth = SectUser[sectnum]->depth;
        }
        else
        {
            TRAVERSE_SPRITE_SECT(headspritesect[sectnum],i,nexti)
            {
                SPRITEp np = &sprite[i];
                if (np->picnum == ST1 && np->hitag == SECT_SINK)
                {
                    depth = np->lotag;
                }
            }
        }
    }

    if (depth && labs(sp->z - u->loz) < Z(8))
    {
        sp->z += Z(depth);
        u->loz = sp->z;
        u->oz = sp->z;
    }

    if (!action)
        return;

    NewStateGroup(SpriteNum, u->ActorActionSet->Run);

    u->ActorActionFunc = DoActorDecide;

    // find the number of long range attacks
    for (wpn = wpn_cnt = 0; wpn < SIZ(u->ActorActionSet->Attack); wpn++)
    {
        if (u->ActorActionSet->Attack[wpn])
            wpn_cnt++;
        else
            break;
    }

    // for actors this tells the number of weapons available
    // for player it tells the current weapon
    u->WeaponNum = wpn_cnt;
}

int
SetupCoolie(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u;
    ANIMATOR DoActorDecide;

    if (TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
    {
        u = User[SpriteNum];
        ASSERT(u);
    }
    else
    {
        User[SpriteNum] = u = SpawnUser(SpriteNum,COOLIE_RUN_R0,s_CoolieRun[0]);
        u->Health = HEALTH_COOLIE;
    }

    ChangeState(SpriteNum,s_CoolieRun[0]);
    u->Attrib = &CoolieAttrib;
    DoActorSetSpeed(SpriteNum, NORM_SPEED);
    u->StateEnd = s_CoolieDie;
    u->Rot = sg_CoolieRun;

    EnemyDefaults(SpriteNum, &CoolieActionSet, &CooliePersonality);

    sp->xrepeat = 42;
    sp->yrepeat = 42;

    SET(u->Flags, SPR_XFLIP_TOGGLE);

    return 0;
}


int SpawnCoolg(short SpriteNum)
{
    int NewCoolg(short);
    USERp u = User[SpriteNum];

    // Don't do a ghost every time
    if (RANDOM_RANGE(1000) > 700) return 0;

    NewCoolg(SpriteNum);

    PlaySpriteSound(SpriteNum,attr_extra1,v3df_follow);

    return 0;
}

int CooliePain(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(SpriteNum);

    return 0;
}

int NullCoolie(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    return 0;
}

int DoCoolieMove(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    if (u->track >= 0)
        ActorFollowTrack(SpriteNum, ACTORMOVETICS);
    else
        (*u->ActorActionFunc)(SpriteNum);

    KeepActorOnFloor(SpriteNum);

    if (DoActorSectorDamage(SpriteNum))
    {
        return 0;
    }

    if (Distance(sp->x, sp->y, u->tgt_sp->x, u->tgt_sp->y) < 1200)
    {
        //DoActorDie(SpriteNum, -3);
        UpdateSinglePlayKills(SpriteNum);
        DoActorDie(SpriteNum, SpriteNum);
        return 0;
    }

    return 0;
}

int InitCoolieCharge(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (RANDOM_P2(1024) > 950)
        PlaySound(DIGI_COOLIESCREAM, &sp->x, &sp->y, &sp->z, v3df_follow);

    DoActorSetSpeed(SpriteNum, FAST_SPEED);

    InitActorMoveCloser(SpriteNum);

    NewStateGroup(SpriteNum, sg_CoolieCharge);

    return 0;
}


int
DoCoolieWaitBirth(short SpriteNum)
{
    SPRITEp sp;
    USERp u;

    u = User[SpriteNum];
    sp = &sprite[SpriteNum];

    if ((u->Counter -= ACTORMOVETICS) <= 0)
    {
        ChangeState(SpriteNum,&s_CoolieDie[9]);
    }

    return 0;
}


#include "saveable.h"

static saveable_code saveable_coolie_code[] =
{
    SAVE_CODE(EnemyDefaults),
    SAVE_CODE(SetupCoolie),
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
