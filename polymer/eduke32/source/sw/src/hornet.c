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
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "actor.h"
#include "weapon.h"
#include "track.h"
#include "sprite.h"

ANIMATOR DoHornetCircle, InitHornetCircle;


DECISION HornetBattle[] =
{
    {50,    InitHornetCircle          },
    {798,   InitActorMoveCloser         },
    {800,   InitActorAlertNoise        },
    {1024,  InitActorRunAway            }
};

DECISION HornetOffense[] =
{
    {1022,  InitActorMoveCloser        },
    {1024,  InitActorAlertNoise        }
};

DECISION HornetBroadcast[] =
{
    {3,    InitActorAlertNoise        },
    {6,    InitActorAmbientNoise          },
    {1024,  InitActorDecide             }
};

DECISION HornetSurprised[] =
{
    {100,   InitHornetCircle           },
    {701,   InitActorMoveCloser         },
    {1024,  InitActorDecide             }
};

DECISION HornetEvasive[] =
{
    {20,     InitHornetCircle          },
    {1024,   NULL                      },
};

DECISION HornetLostTarget[] =
{
    {900,   InitActorFindPlayer         },
    {1024,  InitActorWanderAround       }
};

DECISION HornetCloseRange[] =
{
    {900,   InitActorMoveCloser         },
    {1024,  InitActorReposition         }
};

ANIMATOR InitHornetSting;

DECISION HornetTouchTarget[] =
{
    {500,   InitHornetCircle            },
    {1024,  InitHornetSting             }
};

PERSONALITY HornetPersonality =
{
    HornetBattle,
    HornetOffense,
    HornetBroadcast,
    HornetSurprised,
    HornetEvasive,
    HornetLostTarget,
    HornetCloseRange,
    HornetTouchTarget
};

ATTRIBUTE HornetAttrib =
{
    {300, 350, 375, 400}, // Speeds
    {0,  0, 0,  0}, // Tic Adjusts
    0,      //MaxWeapons;
    {
        0, 0, DIGI_HORNETSTING, DIGI_HORNETSTING, DIGI_HORNETDEATH,
        0,0,0,0,0
    }
};

//////////////////////
//
// HORNET RUN
//////////////////////

#define HORNET_RUN_RATE 7

ANIMATOR DoHornetMove,NullHornet,DoStayOnFloor, DoActorDebris, NullHornet, DoHornetBirth;

STATE s_HornetRun[5][2] =
{
    {
        {HORNET_RUN_R0 + 0, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[0][1]},
        {HORNET_RUN_R0 + 1, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[0][0]},
    },
    {
        {HORNET_RUN_R1 + 0, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[1][1]},
        {HORNET_RUN_R1 + 1, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[1][0]},
    },
    {
        {HORNET_RUN_R2 + 0, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[2][1]},
        {HORNET_RUN_R2 + 1, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[2][0]},
    },
    {
        {HORNET_RUN_R3 + 0, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[3][1]},
        {HORNET_RUN_R3 + 1, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[3][0]},
    },
    {
        {HORNET_RUN_R4 + 0, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[4][1]},
        {HORNET_RUN_R4 + 1, HORNET_RUN_RATE, DoHornetMove, &s_HornetRun[4][0]},
    }
};

STATEp sg_HornetRun[] =
{
    &s_HornetRun[0][0],
    &s_HornetRun[1][0],
    &s_HornetRun[2][0],
    &s_HornetRun[3][0],
    &s_HornetRun[4][0]
};

//////////////////////
//
// HORNET STAND
//
//////////////////////

#define HORNET_STAND_RATE (HORNET_RUN_RATE + 5)

STATE s_HornetStand[5][2] =
{
    {
        {HORNET_RUN_R0 + 0, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[0][1]},
        {HORNET_RUN_R0 + 1, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[0][0]}
    },
    {
        {HORNET_RUN_R1 + 0, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[1][1]},
        {HORNET_RUN_R1 + 1, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[1][0]}
    },
    {
        {HORNET_RUN_R2 + 0, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[2][1]},
        {HORNET_RUN_R2 + 1, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[2][0]}
    },
    {
        {HORNET_RUN_R3 + 0, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[3][1]},
        {HORNET_RUN_R3 + 1, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[3][0]}
    },
    {
        {HORNET_RUN_R4 + 0, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[4][1]},
        {HORNET_RUN_R4 + 1, HORNET_STAND_RATE, DoHornetMove, &s_HornetStand[4][0]}
    }
};

STATEp sg_HornetStand[] =
{
    &s_HornetStand[0][0],
    &s_HornetStand[1][0],
    &s_HornetStand[2][0],
    &s_HornetStand[3][0],
    &s_HornetStand[4][0]
};

//////////////////////
//
// HORNET DIE
//
//////////////////////

#define HORNET_DIE_RATE 20
ANIMATOR DoHornetDeath;
STATE s_HornetDie[] =
{
#if 0
    {HORNET_DIE + 0, HORNET_DIE_RATE, NullHornet, &s_HornetDie[1]},
    {HORNET_DEAD,    HORNET_DIE_RATE, DoActorDebris, &s_HornetDie[1]},
#else
    {HORNET_DIE + 0, HORNET_DIE_RATE, DoHornetDeath, &s_HornetDie[0]},
#endif
};

STATEp sg_HornetDie[] =
{
    s_HornetDie
};

STATE s_HornetDead[] =
{
    {HORNET_DEAD, HORNET_DIE_RATE, DoActorDebris, &s_HornetDead[0]},
};

STATEp sg_HornetDead[] =
{
    s_HornetDead
};

/*
STATEp *Stand[MAX_WEAPONS];
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
STATEp *CloseAttack[2];
STATEp *Attack[6];
STATEp *Special[2];
*/

ACTOR_ACTION_SET HornetActionSet =
{
    sg_HornetStand,
    sg_HornetRun,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, //climb
    NULL, //pain
    sg_HornetDie,
    NULL,
    sg_HornetDead,
    NULL,
    NULL,
    {NULL},
    {0},
    {NULL},
    {0},
    {NULL},
    NULL,
    NULL
};

int DoHornetMatchPlayerZ(short SpriteNum);


int
SetupHornet(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,HORNET_RUN_R0,s_HornetRun[0]);
        u->Health = HEALTH_HORNET;
    }

    ChangeState(SpriteNum, s_HornetRun[0]);
    u->Attrib = &HornetAttrib;
    DoActorSetSpeed(SpriteNum, NORM_SPEED);
    u->StateEnd = s_HornetDie;
    u->Rot = sg_HornetRun;

    EnemyDefaults(SpriteNum, &HornetActionSet, &HornetPersonality);

    SET(u->Flags, SPR_NO_SCAREDZ|SPR_XFLIP_TOGGLE);
    SET(sp->cstat, CSTAT_SPRITE_YCENTER);

    sp->clipdist = (100) >> 2;
    u->floor_dist = Z(16);
    u->ceiling_dist = Z(16);

    u->sz = sp->z;

    sp->xrepeat = 37;
    sp->yrepeat = 32;

    // Special looping buzz sound attached to each hornet spawned
    PlaySound(DIGI_HORNETBUZZ,&sp->x,&sp->y,&sp->z,v3df_follow|v3df_init);
    Set3DSoundOwner(SpriteNum);

    return 0;
}

int NullHornet(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    DoHornetMatchPlayerZ(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    return 0;
}

int DoHornetMatchPlayerZ(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp tsp = User[SpriteNum]->tgt_sp;
    long zdiff,zdist;
    long loz,hiz;

    long bound;

    // actor does a sine wave about u->sz - this is the z mid point

    //zdiff = (SPRITEp_LOWER(tsp) - Z(8)) - u->sz;
    zdiff = (SPRITEp_MID(tsp)) - u->sz;

    // check z diff of the player and the sprite
    zdist = Z(20 + RANDOM_RANGE(200)); // put a random amount
    if (labs(zdiff) > zdist)
    {
        if (zdiff > 0)
            // manipulate the z midpoint
            //u->sz += 256 * ACTORMOVETICS;
            u->sz += 1024 * ACTORMOVETICS;
        else
            u->sz -= 256 * ACTORMOVETICS;
    }

#define HORNET_BOB_AMT (Z(16))

    // save off lo and hi z
    loz = u->loz;
    hiz = u->hiz;

    // adjust loz/hiz for water depth
    if (u->lo_sectp && SectUser[u->lo_sectp - sector] && SectUser[u->lo_sectp - sector]->depth)
        loz -= Z(SectUser[u->lo_sectp - sector]->depth) - Z(8);

    // lower bound
    if (u->lo_sp)
        bound = loz - u->floor_dist;
    else
        bound = loz - u->floor_dist - HORNET_BOB_AMT;

    if (u->sz > bound)
    {
        u->sz = bound;
    }

    // upper bound
    if (u->hi_sp)
        bound = hiz + u->ceiling_dist;
    else
        bound = hiz + u->ceiling_dist + HORNET_BOB_AMT;

    if (u->sz < bound)
    {
        u->sz = bound;
    }

    u->sz = min(u->sz, loz - u->floor_dist);
    u->sz = max(u->sz, hiz + u->ceiling_dist);

    u->Counter = (u->Counter + (ACTORMOVETICS << 3) + (ACTORMOVETICS << 1)) & 2047;
    sp->z = u->sz + ((HORNET_BOB_AMT * (long)sintable[u->Counter]) >> 14);

    bound = u->hiz + u->ceiling_dist + HORNET_BOB_AMT;
    if (sp->z < bound)
    {
        // bumped something
        sp->z = u->sz = bound + HORNET_BOB_AMT;
    }

    return 0;
}

int InitHornetCircle(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    u->ActorActionFunc = DoHornetCircle;

    NewStateGroup(SpriteNum, u->ActorActionSet->Run);

    // set it close
    DoActorSetSpeed(SpriteNum, FAST_SPEED);

    // set to really fast
    sp->xvel = 400;
    // angle adjuster
    u->Counter2 = sp->xvel/3;
    // random angle direction
    if (RANDOM_P2(1024) < 512)
        u->Counter2 = -u->Counter2;

    // z velocity
    u->jump_speed = 200 + RANDOM_P2(128);
    if (labs(u->sz - u->hiz) < labs(u->sz - u->loz))
        u->jump_speed = -u->jump_speed;

    u->WaitTics = (RANDOM_RANGE(3)+1) * 60;

    (*u->ActorActionFunc)(SpriteNum);

    return 0;
}

int DoHornetCircle(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp tsp = User[SpriteNum]->tgt_sp;
    long nx,ny,bound;

    sp->ang = NORM_ANGLE(sp->ang + u->Counter2);

    nx = sp->xvel * (long) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
    ny = sp->xvel * (long) sintable[sp->ang] >> 14;

    if (!move_actor(SpriteNum, nx, ny, 0L))
    {
        //ActorMoveHitReact(SpriteNum);

        // try moving in the opposite direction
        u->Counter2 = -u->Counter2;
        sp->ang = NORM_ANGLE(sp->ang + 1024);
        nx = sp->xvel * (long) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
        ny = sp->xvel * (long) sintable[sp->ang] >> 14;

        if (!move_actor(SpriteNum, nx, ny, 0L))
        {
            InitActorReposition(SpriteNum);
            return 0;
        }
    }

    // move in the z direction
    u->sz -= u->jump_speed * ACTORMOVETICS;

    bound = u->hiz + u->ceiling_dist + HORNET_BOB_AMT;
    if (u->sz < bound)
    {
        // bumped something
        u->sz = bound;
        InitActorReposition(SpriteNum);
        return 0;
    }

    // time out
    if ((u->WaitTics -= ACTORMOVETICS) < 0)
    {
        InitActorReposition(SpriteNum);
        u->WaitTics = 0;
        return 0;
    }

    return 0;
}


int
DoHornetDeath(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    long nx, ny;

    if (TEST(u->Flags, SPR_FALLING))
    {
        u->loz = u->zclip;
        DoFall(SpriteNum);
    }
    else
    {
        RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
        u->jump_speed = 0;
        u->floor_dist = 0;
        DoBeginFall(SpriteNum);
        DoFindGroundPoint(SpriteNum);
        u->zclip = u->loz;
    }

    if (TEST(u->Flags, SPR_SLIDING))
        DoActorSlide(SpriteNum);

    // slide while falling
    nx = sp->xvel * (long) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
    ny = sp->xvel * (long) sintable[sp->ang] >> 14;

    u->ret = move_sprite(SpriteNum, nx, ny, 0L, u->ceiling_dist, u->floor_dist, 1, ACTORMOVETICS);

    // on the ground
    if (sp->z >= u->loz)
    {
        RESET(u->Flags, SPR_FALLING|SPR_SLIDING);
        RESET(sp->cstat, CSTAT_SPRITE_YFLIP); // If upside down, reset it
        NewStateGroup(SpriteNum, u->ActorActionSet->Dead);
        DeleteNoSoundOwner(SpriteNum);
        return 0;
    }

    return 0;
}

// Hornets can swarm around other hornets or whatever is tagged as swarm target
int DoCheckSwarm(short SpriteNum)
{
    short i,nexti;
    SPRITEp sp = &sprite[SpriteNum], tsp;
    USERp u = User[SpriteNum], tu;
    long dist, pdist, a,b,c;
    PLAYERp pp;

    if (!MoveSkip8) return 0;     // Don't over check

    if (!u->tgt_sp) return 0;

    // Who's the closest meat!?
    DoActorPickClosePlayer(SpriteNum);

    if (User[u->tgt_sp - sprite]->PlayerP)
    {
        pp = User[u->tgt_sp - sprite]->PlayerP;
        DISTANCE(sp->x, sp->y, pp->posx, pp->posy, pdist, a, b, c);
    }
    else
        return 0;

    // all enemys
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], i, nexti)
    {
        tsp = &sprite[i];
        tu = User[i];

        if (!tu) continue;

        if (tsp->hitag != TAG_SWARMSPOT || tsp->lotag != 2) continue;

        DISTANCE(sp->x, sp->y, tsp->x, tsp->y, dist, a, b, c);

        if (dist < pdist && u->ID == tu->ID) // Only flock to your own kind
        {
            u->tgt_sp = tsp; // Set target to swarm center
        }
    }

    return TRUE;

}

int DoHornetMove(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    // Check for swarming
    // lotag of 1 = Swarm around lotags of 2
    // lotag of 0 is normal
    if (sp->hitag == TAG_SWARMSPOT && sp->lotag == 1)
        DoCheckSwarm(SpriteNum);

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    if (u->track >= 0)
        ActorFollowTrack(SpriteNum, ACTORMOVETICS);
    else
        (*u->ActorActionFunc)(SpriteNum);

    DoHornetMatchPlayerZ(SpriteNum);

    DoActorSectorDamage(SpriteNum);

    return 0;
}


#include "saveable.h"

static saveable_code saveable_hornet_code[] =
{
    SAVE_CODE(SetupHornet),
    SAVE_CODE(NullHornet),
    SAVE_CODE(DoHornetMatchPlayerZ),
    SAVE_CODE(InitHornetCircle),
    SAVE_CODE(DoHornetCircle),
    SAVE_CODE(DoHornetDeath),
    SAVE_CODE(DoCheckSwarm),
    SAVE_CODE(DoHornetMove),
};

static saveable_data saveable_hornet_data[] =
{
    SAVE_DATA(HornetBattle),
    SAVE_DATA(HornetOffense),
    SAVE_DATA(HornetBroadcast),
    SAVE_DATA(HornetSurprised),
    SAVE_DATA(HornetEvasive),
    SAVE_DATA(HornetLostTarget),
    SAVE_DATA(HornetCloseRange),
    SAVE_DATA(HornetTouchTarget),

    SAVE_DATA(HornetPersonality),

    SAVE_DATA(HornetAttrib),

    SAVE_DATA(s_HornetRun),
    SAVE_DATA(sg_HornetRun),
    SAVE_DATA(s_HornetStand),
    SAVE_DATA(sg_HornetStand),
    SAVE_DATA(s_HornetDie),
    SAVE_DATA(sg_HornetDie),
    SAVE_DATA(s_HornetDead),
    SAVE_DATA(sg_HornetDead),

    SAVE_DATA(HornetActionSet),
};

saveable_module saveable_hornet =
{
    // code
    saveable_hornet_code,
    SIZ(saveable_hornet_code),

    // data
    saveable_hornet_data,
    SIZ(saveable_hornet_data)
};
