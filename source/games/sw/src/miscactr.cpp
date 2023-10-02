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
#include "misc.h"
#include "tags.h"
#include "ai.h"
#include "sector.h"
#include "sprite.h"

BEGIN_SW_NS

ATTRIBUTE ToiletGirlAttrib =
{
    {0, 0, 0, 0},                     // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    3,                               // MaxWeapons;
    {
        0, 0, DIGI_TOILETGIRLSCREAM,
        DIGI_TOILETGIRLPAIN, DIGI_TOILETGIRLSCREAM, 0,0,0,0,0
    }
};

//////////////////////
//
// TOILETGIRL STAND
//
//////////////////////
#define TOILETGIRL_RATE 60

STATE s_ToiletGirlStand[2] =
{
    {TOILETGIRL_R0 + 0, TOILETGIRL_RATE, &AF(DoToiletGirl), &s_ToiletGirlStand[1]},
    {TOILETGIRL_R0 + 1, TOILETGIRL_RATE, &AF(DoToiletGirl), &s_ToiletGirlStand[0]}
};

//////////////////////
//
// TOILETGIRL PAIN
//
//////////////////////

#define TOILETGIRL_PAIN_RATE 32
#define TOILETGIRL_PAIN_R0 TOILETGIRL_R0

STATE s_ToiletGirlPain[2] =
{
    {TOILETGIRL_PAIN_R0 + 0, TOILETGIRL_PAIN_RATE, &AF(ToiletGirlPain), &s_ToiletGirlPain[1]},
    {TOILETGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_ToiletGirlPain[0]}
};

//////////////////////
//
// TOILETGIRL UZI
//
//////////////////////

#define TOILETGIRL_UZI_RATE 8
#define TOILETGIRL_FIRE_R0 TOILETGIRL_R0 + 2

STATE s_ToiletGirlUzi[16] =
{
    {TOILETGIRL_FIRE_R0 + 0, TOILETGIRL_UZI_RATE, &AF(ToiletGirlUzi), &s_ToiletGirlUzi[1]},
    {TOILETGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ToiletGirlUzi[2]},
    {TOILETGIRL_FIRE_R0 + 1, TOILETGIRL_UZI_RATE, &AF(ToiletGirlUzi), &s_ToiletGirlUzi[3]},
    {TOILETGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ToiletGirlUzi[4]},
    {TOILETGIRL_FIRE_R0 + 0, TOILETGIRL_UZI_RATE, &AF(ToiletGirlUzi), &s_ToiletGirlUzi[5]},
    {TOILETGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ToiletGirlUzi[6]},
    {TOILETGIRL_FIRE_R0 + 1, TOILETGIRL_UZI_RATE, &AF(ToiletGirlUzi), &s_ToiletGirlUzi[7]},
    {TOILETGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ToiletGirlUzi[8]},
    {TOILETGIRL_FIRE_R0 + 0, TOILETGIRL_UZI_RATE, &AF(ToiletGirlUzi), &s_ToiletGirlUzi[9]},
    {TOILETGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ToiletGirlUzi[10]},
    {TOILETGIRL_FIRE_R0 + 1, TOILETGIRL_UZI_RATE, &AF(ToiletGirlUzi), &s_ToiletGirlUzi[11]},
    {TOILETGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ToiletGirlUzi[12]},
    {TOILETGIRL_FIRE_R0 + 0, TOILETGIRL_UZI_RATE, &AF(ToiletGirlUzi), &s_ToiletGirlUzi[13]},
    {TOILETGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ToiletGirlUzi[14]},
    {TOILETGIRL_FIRE_R0 + 1, TOILETGIRL_UZI_RATE, &AF(ToiletGirlUzi), &s_ToiletGirlUzi[15]},
    {TOILETGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_ToiletGirlUzi[0]},
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int  SetupToiletGirl(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, TOILETGIRL_R0, s_ToiletGirlStand);
        actor->user.Health = 60;
    }


    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_ToiletGirlStand);
    actor->user.__legacyState.Attrib = &ToiletGirlAttrib;
    actor->user.__legacyState.StateEnd = s_ToiletGirlStand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;

    actor->spr.scale = DVector2(0.5, 0.5);
    actor->vel.X = 0;
    actor->vel.Z = 0;
    actor->spr.lotag = TOILETGIRL_R0;
    actor->user.FlagOwner = 0;
    actor->user.ID = TOILETGIRL_R0;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWToiletGirl, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupToiletGirl(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoToiletGirl(DSWActor* actor)
{
    bool ICanSee = false;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor), actor->sector(), ActorVectOfMiddle(actor->user.targetActor), actor->user.targetActor->sector());

    if (actor->user.FlagOwner != 1)
    {
        if (RandomRange(1000) > 980)
        {
            short choose_snd;

            choose_snd = RANDOM_P2(1024<<4)>>4;

            if (!SoundValidAndActive(actor, CHAN_ToiletFart))
            {
                if (choose_snd > 750)
                    PlaySound(DIGI_TOILETGIRLFART1, actor, v3df_dontpan, CHAN_ToiletFart);
                else if (choose_snd > 350)
                    PlaySound(DIGI_TOILETGIRLFART2, actor, v3df_dontpan, CHAN_ToiletFart);
                else
                    PlaySound(DIGI_TOILETGIRLFART3, actor, v3df_dontpan, CHAN_ToiletFart);
            }
        }
    }
    else if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        if (!SoundValidAndActive(actor, CHAN_AnimeMad))
        {
            if (RandomRange(1000<<8)>>8 > 500)
                PlaySound(DIGI_ANIMEMAD1, actor, v3df_dontpan, CHAN_AnimeMad);
            else
                PlaySound(DIGI_ANIMEMAD2, actor, v3df_dontpan, CHAN_AnimeMad);
        }
        ChangeState(actor,s_ToiletGirlUzi);
        actor->user.WaitTics = SEC(1)+SEC(RandomRange(3<<8)>>8);
        actor->user.FlagOwner = 0;
    }

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING | SPR_CLIMBING)))
    {
        KeepActorOnFloor(actor);
    }

    // take damage from environment
    DoActorSectorDamage(actor);
    actor->vel.X = 0;
    actor->vel.Z = 0;

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullToiletGirl(DSWActor* actor)
{
    bool ICanSee = false;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor),actor->sector(),actor->user.targetActor->spr.pos, actor->user.targetActor->sector());

    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if (actor->user.FlagOwner != 1)
    {
    }
    else if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        if (!SoundValidAndActive(actor, CHAN_AnimeMad))
        {
            if (RandomRange(1000<<8)>>8 > 500)
                PlaySound(DIGI_ANIMEMAD1, actor, v3df_dontpan, CHAN_AnimeMad);
            else
                PlaySound(DIGI_ANIMEMAD2, actor, v3df_dontpan, CHAN_AnimeMad);
        }
        ChangeState(actor,s_ToiletGirlUzi);
        actor->user.WaitTics = SEC(1)+SEC(RandomRange(3<<8)>>8);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int ToiletGirlUzi(DSWActor* actor)
{
    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
    {
        actor->user.WaitTics = RandomRange(240)+120;
        ChangeState(actor,s_ToiletGirlStand);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int ToiletGirlPain(DSWActor* actor)
{
    NullToiletGirl(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(actor,s_ToiletGirlStand);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

ATTRIBUTE WashGirlAttrib =
{
    {0, 0, 0, 0},                     // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    3,                               // MaxWeapons;
    {
        0, 0, DIGI_TOILETGIRLSCREAM,
        DIGI_TOILETGIRLPAIN, DIGI_TOILETGIRLSCREAM, 0,0,0,0,0
    }
};

//////////////////////
//
// WASHGIRL STAND
//
//////////////////////
#define WASHGIRL_RATE 60

STATE s_WashGirlStand[2] =
{
    {WASHGIRL_R0 + 0, WASHGIRL_RATE, &AF(DoWashGirl), &s_WashGirlStand[1]},
    {WASHGIRL_R0 + 1, WASHGIRL_RATE, &AF(DoWashGirl), &s_WashGirlStand[0]}
};

#define WASHGIRL_RATE2 20
STATE s_WashGirlStandScrub[2] =
{
    {WASHGIRL_R0 + 0, WASHGIRL_RATE2, &AF(DoWashGirl), &s_WashGirlStandScrub[1]},
    {WASHGIRL_R0 + 1, WASHGIRL_RATE2, &AF(DoWashGirl), &s_WashGirlStandScrub[0]}
};

//////////////////////
//
// WASHGIRL PAIN
//
//////////////////////

#define WASHGIRL_PAIN_RATE 30
#define WASHGIRL_PAIN_R0 WASHGIRL_R0

STATE s_WashGirlPain[2] =
{
    {WASHGIRL_PAIN_R0 + 0, WASHGIRL_PAIN_RATE, &AF(WashGirlPain), &s_WashGirlPain[1]},
    {WASHGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_WashGirlPain[0]}
};


//////////////////////
//
// WASHGIRL UZI
//
//////////////////////

#define WASHGIRL_UZI_RATE 8
#define WASHGIRL_FIRE_R0 WASHGIRL_R0 + 2

STATE s_WashGirlUzi[16] =
{
    {WASHGIRL_FIRE_R0 + 0, WASHGIRL_UZI_RATE, &AF(WashGirlUzi), &s_WashGirlUzi[1]},
    {WASHGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_WashGirlUzi[2]},
    {WASHGIRL_FIRE_R0 + 1, WASHGIRL_UZI_RATE, &AF(WashGirlUzi), &s_WashGirlUzi[3]},
    {WASHGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_WashGirlUzi[4]},
    {WASHGIRL_FIRE_R0 + 0, WASHGIRL_UZI_RATE, &AF(WashGirlUzi), &s_WashGirlUzi[5]},
    {WASHGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_WashGirlUzi[6]},
    {WASHGIRL_FIRE_R0 + 1, WASHGIRL_UZI_RATE, &AF(WashGirlUzi), &s_WashGirlUzi[7]},
    {WASHGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_WashGirlUzi[8]},
    {WASHGIRL_FIRE_R0 + 0, WASHGIRL_UZI_RATE, &AF(WashGirlUzi), &s_WashGirlUzi[9]},
    {WASHGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_WashGirlUzi[10]},
    {WASHGIRL_FIRE_R0 + 1, WASHGIRL_UZI_RATE, &AF(WashGirlUzi), &s_WashGirlUzi[11]},
    {WASHGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_WashGirlUzi[12]},
    {WASHGIRL_FIRE_R0 + 0, WASHGIRL_UZI_RATE, &AF(WashGirlUzi), &s_WashGirlUzi[13]},
    {WASHGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_WashGirlUzi[14]},
    {WASHGIRL_FIRE_R0 + 1, WASHGIRL_UZI_RATE, &AF(WashGirlUzi), &s_WashGirlUzi[15]},
    {WASHGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_WashGirlUzi[0]},
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupWashGirl(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, WASHGIRL_R0,s_WashGirlStand);
        actor->user.Health = 60;
    }

    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_WashGirlStand);
    actor->user.__legacyState.Attrib = &WashGirlAttrib;
    actor->user.__legacyState.StateEnd = s_WashGirlStand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;

	actor->spr.scale = DVector2(0.4374, 0.375);
    actor->vel.X = 0;
    actor->vel.Z = 0;
    actor->spr.lotag = WASHGIRL_R0;
    actor->user.FlagOwner = 0;
    actor->user.ID = WASHGIRL_R0;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWWashGirl, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupWashGirl(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoWashGirl(DSWActor* actor)
{
    bool ICanSee = false;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor),actor->sector(),ActorVectOfMiddle(actor->user.targetActor),actor->user.targetActor->sector());

    if (RandomRange(1000) > 980 && actor->user.ShellNum <= 0)
    {
        if (!SoundValidAndActive(actor, CHAN_AnimeSing))
        {
            if (RANDOM_P2(1024<<4)>>4 > 500)
                PlaySound(DIGI_ANIMESING1, actor, v3df_dontpan, CHAN_AnimeSing);
            else
                PlaySound(DIGI_ANIMESING2, actor, v3df_dontpan, CHAN_AnimeSing);
        }

        ChangeState(actor,s_WashGirlStandScrub);
        actor->user.ShellNum = RandomRange(2*120)+240;
    }
    else
    {
        if (actor->user.ShellNum > 0)
        {
            if ((actor->user.ShellNum -= ACTORMOVETICS) < 0)
            {
                ChangeState(actor,s_WashGirlStand);
                actor->user.ShellNum = 0;
            }
        }
    }

    if (actor->user.FlagOwner != 1)
    {
    }
    else if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        if (!SoundValidAndActive(actor, CHAN_AnimeMad))
        {
            if (RandomRange(1000<<8)>>8 > 500)
                PlaySound(DIGI_ANIMEMAD1, actor, v3df_dontpan, CHAN_AnimeMad);
            else
                PlaySound(DIGI_ANIMEMAD2, actor, v3df_dontpan, CHAN_AnimeMad);
        }
        ChangeState(actor,s_WashGirlUzi);
        actor->user.WaitTics = SEC(1)+SEC(RandomRange(3<<8)>>8);
        actor->user.FlagOwner = 0;
    }

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING | SPR_CLIMBING)))
    {
        KeepActorOnFloor(actor);
    }

    // take damage from environment
    DoActorSectorDamage(actor);
    actor->vel.X = 0;
    actor->vel.Z = 0;

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullWashGirl(DSWActor* actor)
{
    bool ICanSee = false;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor),actor->sector(),actor->user.targetActor->spr.pos, actor->user.targetActor->sector());

    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if (actor->user.FlagOwner != 1)
    {
    }
    else if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        if (!SoundValidAndActive(actor, CHAN_AnimeMad))
        {
            if (RandomRange(1000<<8)>>8 > 500)
                PlaySound(DIGI_ANIMEMAD1, actor, v3df_dontpan, CHAN_AnimeMad);
            else
                PlaySound(DIGI_ANIMEMAD2, actor, v3df_dontpan, CHAN_AnimeMad);
        }
        ChangeState(actor,s_WashGirlUzi);
        actor->user.WaitTics = SEC(1)+SEC(RandomRange(3<<8)>>8);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int WashGirlUzi(DSWActor* actor)
{
    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
    {
        actor->user.WaitTics = RandomRange(240)+120;
        ChangeState(actor,s_WashGirlStand);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int WashGirlPain(DSWActor* actor)
{
    NullWashGirl(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(actor,s_WashGirlStand);

    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------


ATTRIBUTE TrashCanAttrib =
{
    {0, 0, 0, 0},                     // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    0,                               // MaxWeapons;
    {0, 0, DIGI_TRASHLID, DIGI_TRASHLID, 0, 0,0,0,0,0}
};

//////////////////////
//
// TRASHCAN STAND
//
//////////////////////
#define TRASHCAN_RATE 120
#define TRASHCAN_R0 TRASHCAN

STATE s_TrashCanStand[1] =
{
    {TRASHCAN_R0 + 0, TRASHCAN_RATE, &AF(DoTrashCan), &s_TrashCanStand[0]}
};

//////////////////////
//
// TRASHCAN PAIN
//
//////////////////////

#define TRASHCAN_PAIN_RATE 8
#define TRASHCAN_PAIN_R0 TRASHCAN

STATE s_TrashCanPain[7] =
{
    {TRASHCAN_PAIN_R0 + 0, TRASHCAN_PAIN_RATE, &AF(TrashCanPain), &s_TrashCanPain[1]},
    {TRASHCAN_PAIN_R0 + 1, TRASHCAN_PAIN_RATE, &AF(TrashCanPain), &s_TrashCanPain[2]},
    {TRASHCAN_PAIN_R0 + 2, TRASHCAN_PAIN_RATE, &AF(TrashCanPain), &s_TrashCanPain[3]},
    {TRASHCAN_PAIN_R0 + 3, TRASHCAN_PAIN_RATE, &AF(TrashCanPain), &s_TrashCanPain[4]},
    {TRASHCAN_PAIN_R0 + 4, TRASHCAN_PAIN_RATE, &AF(TrashCanPain), &s_TrashCanPain[5]},
    {TRASHCAN_PAIN_R0 + 5, TRASHCAN_PAIN_RATE, &AF(TrashCanPain), &s_TrashCanPain[6]},
    {TRASHCAN_PAIN_R0 + 6, TRASHCAN_PAIN_RATE, &AF(TrashCanPain), &s_TrashCanPain[0]}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupTrashCan(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, TRASHCAN,s_TrashCanStand);
        actor->user.Health = 60;
    }

    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_TrashCanStand);
    actor->user.__legacyState.Attrib = &TrashCanAttrib;
    actor->user.__legacyState.StateEnd = s_TrashCanStand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;


	actor->spr.scale = DVector2(0.71875, 0.65625);
    actor->vel.X = 0;
    actor->vel.Z = 0;
    actor->user.ID = TRASHCAN;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);
    actor->spr.extra &= ~(SPRX_PLAYER_OR_ENEMY);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoTrashCan(DSWActor* actor)
{
    // stay on floor unless doing certain things
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING | SPR_CLIMBING)))
    {
        KeepActorOnFloor(actor);
    }

    actor->vel.X = 0;
    actor->vel.Z = 0;

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int TrashCanPain(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(actor,s_TrashCanStand);

    return 0;
}


//---------------------------------------------------------------------------
//
// PACHINKO WIN LIGHT
//
//---------------------------------------------------------------------------

ATTRIBUTE PachinkoLightAttrib =
{
    {0, 0, 0, 0},                     // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    0,                               // MaxWeapons;
    {0, 0, DIGI_TRASHLID, DIGI_TRASHLID, 0, 0,0,0,0,0}
};

#define PACHINKOLIGHT_RATE 120
#define PACHINKOLIGHT_R0 623

STATE s_PachinkoLightStand[] =
{
    {PACHINKOLIGHT_R0 + 0, PACHINKOLIGHT_RATE, nullptr,  &s_PachinkoLightStand[0]}
};

STATE s_PachinkoLightOperate[] =
{
    {PACHINKOLIGHT_R0 - 0, 12, &AF(PachinkoLightOperate), &s_PachinkoLightOperate[1]},
    {PACHINKOLIGHT_R0 - 1, 12, &AF(PachinkoLightOperate), &s_PachinkoLightOperate[2]},
    {PACHINKOLIGHT_R0 - 2, 12, &AF(PachinkoLightOperate), &s_PachinkoLightOperate[3]},
    {PACHINKOLIGHT_R0 - 3, 12, &AF(PachinkoLightOperate), &s_PachinkoLightOperate[4]},
    {PACHINKOLIGHT_R0 - 4, 12, &AF(PachinkoLightOperate), &s_PachinkoLightOperate[5]},
    {PACHINKOLIGHT_R0 - 5, 12, &AF(PachinkoLightOperate), &s_PachinkoLightOperate[0]},
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupPachinkoLight(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, PACHINKOLIGHT_R0,s_PachinkoLightStand);
        actor->user.Health = 1;
    }

    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_PachinkoLightStand);
    actor->user.__legacyState.Attrib = &PachinkoLightAttrib;
    actor->user.__legacyState.StateEnd = s_PachinkoLightStand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;
    actor->user.ID = PACHINKOLIGHT_R0;

    actor->vel.X = 0;
    actor->vel.Z = 0;
    actor->spr.lotag = TAG_PACHINKOLIGHT;
    actor->spr.shade = -2;
    actor->user.spal = actor->spr.pal;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);
    actor->spr.extra &= ~(SPRX_PLAYER_OR_ENEMY);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWPachinkoWinLight, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupPachinkoLight(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int PachinkoLightOperate(DSWActor* actor)
{
    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
    {
        actor->spr.shade = -2;
        ChangeState(actor,s_PachinkoLightStand);
    }
    return 0;
}

//---------------------------------------------------------------------------
//
// PACHINKO MACHINE #1
//
//---------------------------------------------------------------------------

CVAR(Bool, Pachinko_Win_Cheat, false, 0)

ATTRIBUTE Pachinko1Attrib =
{
    {0, 0, 0, 0},                     // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    0,                               // MaxWeapons;
    {0, 0, DIGI_TRASHLID, DIGI_TRASHLID, 0, 0,0,0,0,0}
};

#define PACHINKO1_RATE 120
#define PACHINKO1_R0 PACHINKO1

STATE s_Pachinko1Stand[] =
{
    {PACHINKO1_R0 + 0, PACHINKO1_RATE, nullptr,  &s_Pachinko1Stand[0]}
};

STATE s_Pachinko1Operate[] =
{
    {PACHINKO1_R0 + 0, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[1]},
    {PACHINKO1_R0 + 1, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[2]},
    {PACHINKO1_R0 + 2, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[3]},
    {PACHINKO1_R0 + 3, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[4]},
    {PACHINKO1_R0 + 4, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[5]},
    {PACHINKO1_R0 + 5, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[6]},
    {PACHINKO1_R0 + 6, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[7]},
    {PACHINKO1_R0 + 7, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[8]},
    {PACHINKO1_R0 + 8, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[9]},
    {PACHINKO1_R0 + 9, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[10]},
    {PACHINKO1_R0 + 10, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[11]},
    {PACHINKO1_R0 + 11, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[12]},
    {PACHINKO1_R0 + 12, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[13]},
    {PACHINKO1_R0 + 13, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[14]},
    {PACHINKO1_R0 + 14, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[15]},
    {PACHINKO1_R0 + 15, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[16]},
    {PACHINKO1_R0 + 16, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[17]},
    {PACHINKO1_R0 + 17, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[18]},
    {PACHINKO1_R0 + 18, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[19]},
    {PACHINKO1_R0 + 19, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[20]},
    {PACHINKO1_R0 + 20, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[21]},
    {PACHINKO1_R0 + 21, 12, &AF(Pachinko1Operate), &s_Pachinko1Operate[22]},
    {PACHINKO1_R0 + 22, SF_QUICK_CALL, &AF(PachinkoCheckWin), &s_Pachinko1Stand[0]}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupPachinko1(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, PACHINKO1,s_Pachinko1Stand);
        actor->user.Health = 1;
    }

    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_Pachinko1Stand);
    actor->user.__legacyState.Attrib = &Pachinko1Attrib;
    actor->user.__legacyState.StateEnd = s_Pachinko1Stand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;
    actor->user.ID = PACHINKO1;

    actor->vel.Z = 0;
    actor->spr.lotag = PACHINKO1;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);
    actor->spr.extra &= ~(SPRX_PLAYER_OR_ENEMY);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWPachinko1, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupPachinko1(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int PachinkoCheckWin(DSWActor* actor)
{
    actor->user.WaitTics = 0;  // Can operate it again now

    // You already won, no more from this machine!
    if (TEST_BOOL1(actor)) return 0;

    // Well? Did I win????!
    /*short rnd = */RandomRange(1000);
    if (RandomRange(1000) > 900 || Pachinko_Win_Cheat)
    {
        int i;

        // Do a possible combo switch
        if (ComboSwitchTest(TAG_COMBO_SWITCH_EVERYTHING, actor->spr.hitag))
        {
            DoMatchEverything(&Player[myconnectindex], actor->spr.hitag, 1);
        }

        ActorCoughItem(actor); // I WON! I WON!
        PlaySound(DIGI_PALARM, actor, v3df_none);

        // Can't win any more now!
        SET_BOOL1(actor);

        // Turn on the pachinko lights
        SWStatIterator it(STAT_ENEMY);
        while (auto itActor = it.Next())
        {
            if (itActor->spr.lotag == TAG_PACHINKOLIGHT)
            {
                if (itActor->spr.hitag == SP_TAG5(actor))
                {
                    itActor->spr.shade = -90; // Full brightness
                    itActor->user.WaitTics = SEC(3); // Flash
                    ChangeState(itActor,s_PachinkoLightOperate);
                }
            }
        }

    }

    //{
    //if(rnd > 950)
    //    PlayerSound(DIGI_SHISEISI, pp, v3df_follow|v3df_dontpan,pp);
    //else
    //if(rnd > 900)
    //    PlayerSound(DIGI_YOULOOKSTUPID, pp, v3df_follow|v3df_dontpan,pp);
    //else
    //if(rnd > 850)
    //    PlayerSound(DIGI_HURTBAD5, pp, v3df_follow|v3df_dontpan,pp);
    //}

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int Pachinko1Operate(DSWActor* actor)
{
    short rnd;

    rnd = RandomRange(1000);
    if (rnd > 900)
    {
        rnd = RandomRange(1000);  // TEMP SOUNDS: Need pachinko sounds!
        if (rnd > 700)
            PlaySound(DIGI_PROLL1, actor, v3df_none);
        else if (rnd > 400)
            PlaySound(DIGI_PROLL2, actor, v3df_none);
        else
            PlaySound(DIGI_PROLL3, actor, v3df_none);
    }

    return 0;
}


//---------------------------------------------------------------------------
//
// PACHINKO MACHINE #2
//
//---------------------------------------------------------------------------


ATTRIBUTE Pachinko2Attrib =
{
    {0, 0, 0, 0},                     // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    0,                               // MaxWeapons;
    {0, 0, DIGI_TRASHLID, DIGI_TRASHLID, 0, 0,0,0,0,0}
};

#define PACHINKO2_RATE 120
#define PACHINKO2_R0 PACHINKO2

STATE s_Pachinko2Stand[] =
{
    {PACHINKO2_R0 + 0, PACHINKO2_RATE, nullptr,  &s_Pachinko2Stand[0]}
};

STATE s_Pachinko2Operate[] =
{
    {PACHINKO2_R0 + 0, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[1]},
    {PACHINKO2_R0 + 1, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[2]},
    {PACHINKO2_R0 + 2, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[3]},
    {PACHINKO2_R0 + 3, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[4]},
    {PACHINKO2_R0 + 4, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[5]},
    {PACHINKO2_R0 + 5, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[6]},
    {PACHINKO2_R0 + 6, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[7]},
    {PACHINKO2_R0 + 7, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[8]},
    {PACHINKO2_R0 + 8, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[9]},
    {PACHINKO2_R0 + 9, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[10]},
    {PACHINKO2_R0 + 10, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[11]},
    {PACHINKO2_R0 + 11, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[12]},
    {PACHINKO2_R0 + 12, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[13]},
    {PACHINKO2_R0 + 13, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[14]},
    {PACHINKO2_R0 + 14, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[15]},
    {PACHINKO2_R0 + 15, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[16]},
    {PACHINKO2_R0 + 16, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[17]},
    {PACHINKO2_R0 + 17, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[18]},
    {PACHINKO2_R0 + 18, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[19]},
    {PACHINKO2_R0 + 19, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[20]},
    {PACHINKO2_R0 + 20, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[21]},
    {PACHINKO2_R0 + 21, 12, &AF(Pachinko1Operate), &s_Pachinko2Operate[22]},
    {PACHINKO2_R0 + 22, SF_QUICK_CALL, &AF(PachinkoCheckWin), &s_Pachinko2Stand[0]}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupPachinko2(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, PACHINKO2,s_Pachinko2Stand);
        actor->user.Health = 1;
    }

    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_Pachinko2Stand);
    actor->user.__legacyState.Attrib = &Pachinko2Attrib;
    actor->user.__legacyState.StateEnd = s_Pachinko2Stand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;
    actor->user.ID = PACHINKO2;

    actor->vel.Z = 0;
    actor->spr.lotag = PACHINKO2;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);
    actor->spr.extra &= ~(SPRX_PLAYER_OR_ENEMY);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWPachinko2, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupPachinko2(self);
    return 0;
}

//---------------------------------------------------------------------------
//
// PACHINKO MACHINE #3
//
//---------------------------------------------------------------------------


ATTRIBUTE Pachinko3Attrib =
{
    {0, 0, 0, 0},                     // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    0,                               // MaxWeapons;
    {0, 0, DIGI_TRASHLID, DIGI_TRASHLID, 0, 0,0,0,0,0}
};

#define PACHINKO3_RATE 120
#define PACHINKO3_R0 PACHINKO3

STATE s_Pachinko3Stand[] =
{
    {PACHINKO3_R0 + 0, PACHINKO3_RATE, nullptr,  &s_Pachinko3Stand[0]}
};

STATE s_Pachinko3Operate[] =
{
    {PACHINKO3_R0 + 0, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[1]},
    {PACHINKO3_R0 + 1, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[2]},
    {PACHINKO3_R0 + 2, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[3]},
    {PACHINKO3_R0 + 3, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[4]},
    {PACHINKO3_R0 + 4, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[5]},
    {PACHINKO3_R0 + 5, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[6]},
    {PACHINKO3_R0 + 6, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[7]},
    {PACHINKO3_R0 + 7, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[8]},
    {PACHINKO3_R0 + 8, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[9]},
    {PACHINKO3_R0 + 9, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[10]},
    {PACHINKO3_R0 + 10, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[11]},
    {PACHINKO3_R0 + 11, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[12]},
    {PACHINKO3_R0 + 12, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[13]},
    {PACHINKO3_R0 + 13, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[14]},
    {PACHINKO3_R0 + 14, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[15]},
    {PACHINKO3_R0 + 15, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[16]},
    {PACHINKO3_R0 + 16, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[17]},
    {PACHINKO3_R0 + 17, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[18]},
    {PACHINKO3_R0 + 18, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[19]},
    {PACHINKO3_R0 + 19, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[20]},
    {PACHINKO3_R0 + 20, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[21]},
    {PACHINKO3_R0 + 21, 12, &AF(Pachinko1Operate), &s_Pachinko3Operate[22]},
    {PACHINKO3_R0 + 22, SF_QUICK_CALL, &AF(PachinkoCheckWin), &s_Pachinko3Stand[0]}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupPachinko3(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, PACHINKO3,s_Pachinko3Stand);
        actor->user.Health = 1;
    }

    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_Pachinko3Stand);
    actor->user.__legacyState.Attrib = &Pachinko3Attrib;
    actor->user.__legacyState.StateEnd = s_Pachinko3Stand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;
    actor->user.ID = PACHINKO3;

    actor->vel.Z = 0;
    actor->spr.lotag = PACHINKO3;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);
    actor->spr.extra &= ~(SPRX_PLAYER_OR_ENEMY);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWPachinko3, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupPachinko3(self);
    return 0;
}

//---------------------------------------------------------------------------
//
// PACHINKO MACHINE #4
//
//---------------------------------------------------------------------------


ATTRIBUTE Pachinko4Attrib =
{
    {0, 0, 0, 0},                     // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    0,                               // MaxWeapons;
    {0, 0, DIGI_TRASHLID, DIGI_TRASHLID, 0, 0,0,0,0,0}
};

#define PACHINKO4_RATE 120
#define PACHINKO4_R0 PACHINKO4

STATE s_Pachinko4Stand[] =
{
    {PACHINKO4_R0 + 0, PACHINKO4_RATE, nullptr,  &s_Pachinko4Stand[0]}
};

STATE s_Pachinko4Operate[] =
{
    {PACHINKO4_R0 + 0, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[1]},
    {PACHINKO4_R0 + 1, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[2]},
    {PACHINKO4_R0 + 2, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[3]},
    {PACHINKO4_R0 + 3, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[4]},
    {PACHINKO4_R0 + 4, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[5]},
    {PACHINKO4_R0 + 5, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[6]},
    {PACHINKO4_R0 + 6, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[7]},
    {PACHINKO4_R0 + 7, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[8]},
    {PACHINKO4_R0 + 8, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[9]},
    {PACHINKO4_R0 + 9, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[10]},
    {PACHINKO4_R0 + 10, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[11]},
    {PACHINKO4_R0 + 11, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[12]},
    {PACHINKO4_R0 + 12, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[13]},
    {PACHINKO4_R0 + 13, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[14]},
    {PACHINKO4_R0 + 14, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[15]},
    {PACHINKO4_R0 + 15, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[16]},
    {PACHINKO4_R0 + 16, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[17]},
    {PACHINKO4_R0 + 17, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[18]},
    {PACHINKO4_R0 + 18, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[19]},
    {PACHINKO4_R0 + 19, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[20]},
    {PACHINKO4_R0 + 20, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[21]},
    {PACHINKO4_R0 + 21, 12, &AF(Pachinko1Operate), &s_Pachinko4Operate[22]},
    {PACHINKO4_R0 + 22, SF_QUICK_CALL, &AF(PachinkoCheckWin), &s_Pachinko4Stand[0]}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupPachinko4(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, PACHINKO4,s_Pachinko4Stand);
        actor->user.Health = 1;
    }

    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_Pachinko4Stand);
    actor->user.__legacyState.Attrib = &Pachinko4Attrib;
    actor->user.__legacyState.StateEnd = s_Pachinko4Stand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;
    actor->user.ID = PACHINKO4;

    actor->vel.Z = 0;
    actor->spr.lotag = PACHINKO4;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);
    actor->spr.extra &= ~(SPRX_PLAYER_OR_ENEMY);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWPachinko4, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupPachinko4(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

ATTRIBUTE CarGirlAttrib =
{
    {0, 0, 0, 0},                     // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    3,                               // MaxWeapons;
    {
        0, 0, DIGI_TOILETGIRLSCREAM,
        DIGI_TOILETGIRLPAIN, DIGI_TOILETGIRLSCREAM, 0,0,0,0,0
    }
};

//////////////////////
//
// CARGIRL STAND
//
//////////////////////
#define CARGIRL_RATE 60

STATE s_CarGirlStand[2] =
{
    {CARGIRL_R0 + 0, CARGIRL_RATE, &AF(DoCarGirl), &s_CarGirlStand[1]},
    {CARGIRL_R0 + 1, CARGIRL_RATE, &AF(DoCarGirl), &s_CarGirlStand[0]}
};

//////////////////////
//
// CARGIRL PAIN
//
//////////////////////

#define CARGIRL_PAIN_RATE 32
#define CARGIRL_PAIN_R0 CARGIRL_R0

STATE s_CarGirlPain[2] =
{
    {CARGIRL_PAIN_R0 + 0, CARGIRL_PAIN_RATE, &AF(CarGirlPain), &s_CarGirlPain[1]},
    {CARGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_CarGirlPain[0]}
};

//////////////////////
//
// CARGIRL UZI
//
//////////////////////

#define CARGIRL_UZI_RATE 8
#define CARGIRL_FIRE_R0 CARGIRL_R0 + 2

STATE s_CarGirlUzi[16] =
{
    {CARGIRL_FIRE_R0 + 0, 240, &AF(CarGirlUzi), &s_CarGirlUzi[1]},
    {CARGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_CarGirlUzi[2]},
    {CARGIRL_FIRE_R0 + 1, CARGIRL_UZI_RATE, &AF(CarGirlUzi), &s_CarGirlUzi[3]},
    {CARGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_CarGirlUzi[4]},
    {CARGIRL_FIRE_R0 + 0, CARGIRL_UZI_RATE, &AF(CarGirlUzi), &s_CarGirlUzi[5]},
    {CARGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_CarGirlUzi[6]},
    {CARGIRL_FIRE_R0 + 1, CARGIRL_UZI_RATE, &AF(CarGirlUzi), &s_CarGirlUzi[7]},
    {CARGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_CarGirlUzi[8]},
    {CARGIRL_FIRE_R0 + 0, CARGIRL_UZI_RATE, &AF(CarGirlUzi), &s_CarGirlUzi[9]},
    {CARGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_CarGirlUzi[10]},
    {CARGIRL_FIRE_R0 + 1, CARGIRL_UZI_RATE, &AF(CarGirlUzi), &s_CarGirlUzi[11]},
    {CARGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_CarGirlUzi[12]},
    {CARGIRL_FIRE_R0 + 0, CARGIRL_UZI_RATE, &AF(CarGirlUzi), &s_CarGirlUzi[13]},
    {CARGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_CarGirlUzi[14]},
    {CARGIRL_FIRE_R0 + 1, CARGIRL_UZI_RATE, &AF(CarGirlUzi), &s_CarGirlUzi[15]},
    {CARGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, &AF(InitEnemyUzi), &s_CarGirlUzi[0]},
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupCarGirl(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, CARGIRL_R0,s_CarGirlStand);
        actor->user.Health = 60;
    }


    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_CarGirlStand);
    actor->user.__legacyState.Attrib = &CarGirlAttrib;
    actor->user.__legacyState.StateEnd = s_CarGirlStand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;

	actor->spr.scale = DVector2(0.453125, 0.390625);
    actor->vel.X = 0;
    actor->vel.Z = 0;
    actor->spr.lotag = CARGIRL_R0;
    actor->user.FlagOwner = 0;
    actor->user.ID = CARGIRL_R0;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);
    actor->spr.cstat |= (CSTAT_SPRITE_XFLIP);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWCarGirl, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupCarGirl(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoCarGirl(DSWActor* actor)
{
    bool ICanSee = false;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor),actor->sector(),ActorVectOfMiddle(actor->user.targetActor),actor->user.targetActor->sector());

    if (actor->user.FlagOwner == 1)
    {
        if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
        {
            if (!SoundValidAndActive(actor, CHAN_AnimeMad))
            {
                short choose;
                choose = RandomRange(1000);

                if (choose > 750)
                    PlaySound(DIGI_LANI049, actor, v3df_dontpan, CHAN_AnimeMad);
                else if (choose > 500)
                    PlaySound(DIGI_LANI051, actor, v3df_dontpan, CHAN_AnimeMad);
                else if (choose > 250)
                    PlaySound(DIGI_LANI052, actor, v3df_dontpan, CHAN_AnimeMad);
                else
                    PlaySound(DIGI_LANI054, actor, v3df_dontpan, CHAN_AnimeMad);
            }
            ChangeState(actor,s_CarGirlUzi);
            actor->user.WaitTics = SEC(3)+SEC(RandomRange(2<<8)>>8);
            actor->user.FlagOwner = 0;
        }
    }

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING | SPR_CLIMBING)))
    {
        KeepActorOnFloor(actor);
    }

    // take damage from environment
    DoActorSectorDamage(actor);
    actor->vel.X = 0;
    actor->vel.Z = 0;

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullCarGirl(DSWActor* actor)
{
    bool ICanSee = false;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor),actor->sector(),actor->user.targetActor->spr.pos, actor->user.targetActor->sector());

    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if (actor->user.FlagOwner != 1)
    {
    }
    else if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        if (!SoundValidAndActive(actor, CHAN_AnimeMad))
        {
            short choose;
            choose = RandomRange(1000);

            if (choose > 750)
                PlaySound(DIGI_LANI049, actor, v3df_dontpan, CHAN_AnimeMad);
            else if (choose > 500)
                PlaySound(DIGI_LANI051, actor, v3df_dontpan, CHAN_AnimeMad);
            else if (choose > 250)
                PlaySound(DIGI_LANI052, actor, v3df_dontpan, CHAN_AnimeMad);
            else
                PlaySound(DIGI_LANI054, actor, v3df_dontpan, CHAN_AnimeMad);
        }
        ChangeState(actor,s_CarGirlUzi);
        actor->user.WaitTics = SEC(3)+SEC(RandomRange(2<<8)>>8);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int CarGirlUzi(DSWActor* actor)
{
    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
    {
        actor->user.WaitTics = RandomRange(240)+120;
        ChangeState(actor,s_CarGirlStand);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int CarGirlPain(DSWActor* actor)
{
    NullCarGirl(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(actor,s_CarGirlStand);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

ATTRIBUTE MechanicGirlAttrib =
{
    {0, 0, 0, 0},                     // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    3,                               // MaxWeapons;
    {
        0, 0, DIGI_TOILETGIRLSCREAM,
        DIGI_TOILETGIRLPAIN, DIGI_TOILETGIRLSCREAM, 0,0,0,0,0
    }
};

//////////////////////
//
// MECHANICGIRL STAND
//
//////////////////////
#define MECHANICGIRL_RATE 60

STATE s_MechanicGirlStand[2] =
{
    {MECHANICGIRL_R0 + 0, MECHANICGIRL_RATE, &AF(DoMechanicGirl), &s_MechanicGirlStand[1]},
    {MECHANICGIRL_R0 + 1, MECHANICGIRL_RATE, &AF(DoMechanicGirl), &s_MechanicGirlStand[0]}
};

//////////////////////
//
// MECHANICGIRL PAIN
//
//////////////////////

#define MECHANICGIRL_PAIN_RATE 32
#define MECHANICGIRL_PAIN_R0 MECHANICGIRL_R0

STATE s_MechanicGirlPain[2] =
{
    {MECHANICGIRL_PAIN_R0 + 0, MECHANICGIRL_PAIN_RATE, &AF(MechanicGirlPain), &s_MechanicGirlPain[1]},
    {MECHANICGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_MechanicGirlPain[0]}
};

//////////////////////
//
// MECHANICGIRL DRILL
//
//////////////////////

#define MECHANICGIRL_DRILL_RATE 32
#define MECHANICGIRL_DRILL_R0 MECHANICGIRL_R0 + 2

STATE s_MechanicGirlDrill[2] =
{
    {MECHANICGIRL_DRILL_R0 + 0, MECHANICGIRL_DRILL_RATE, &AF(MechanicGirlDrill), &s_MechanicGirlDrill[1]},
    {MECHANICGIRL_DRILL_R0 + 1, MECHANICGIRL_DRILL_RATE, &AF(MechanicGirlDrill), &s_MechanicGirlDrill[0]},
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupMechanicGirl(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, MECHANICGIRL_R0,s_MechanicGirlStand);
        actor->user.Health = 60;
    }


    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_MechanicGirlStand);
    actor->user.__legacyState.Attrib = &MechanicGirlAttrib;
    actor->user.__legacyState.StateEnd = s_MechanicGirlStand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;

	actor->spr.scale = DVector2(0.421875, 0.40625);
    actor->vel.X = 0;
    actor->vel.Z = 0;
    actor->spr.lotag = MECHANICGIRL_R0;
    actor->user.FlagOwner = 0;
    actor->user.ID = MECHANICGIRL_R0;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWMechanicGirl, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupMechanicGirl(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoMechanicGirl(DSWActor* actor)
{
    bool ICanSee = false;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor),actor->sector(),ActorVectOfMiddle(actor->user.targetActor),actor->user.targetActor->sector());

    if (actor->user.FlagOwner == 1)
    {
        if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
        {
            if (!SoundValidAndActive(actor, CHAN_AnimeMad))
            {
                short choose;
                choose = RandomRange(1000);

                if (choose > 750)
                    PlaySound(DIGI_LANI073, actor, v3df_dontpan, CHAN_AnimeMad);
                else if (choose > 500)
                    PlaySound(DIGI_LANI075, actor, v3df_dontpan, CHAN_AnimeMad);
                else if (choose > 250)
                    PlaySound(DIGI_LANI077, actor, v3df_dontpan, CHAN_AnimeMad);
                else
                    PlaySound(DIGI_LANI079, actor, v3df_dontpan, CHAN_AnimeMad);
            }
            ChangeState(actor,s_MechanicGirlDrill);
            actor->user.WaitTics = SEC(1)+SEC(RandomRange(2<<8)>>8);
            actor->user.FlagOwner = 0;
        }
    }

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING | SPR_CLIMBING)))
    {
        KeepActorOnFloor(actor);
    }

    // take damage from environment
    DoActorSectorDamage(actor);
    actor->vel.X = 0;
    actor->vel.Z = 0;

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullMechanicGirl(DSWActor* actor)
{
    bool ICanSee = false;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor),actor->sector(),actor->user.targetActor->spr.pos, actor->user.targetActor->sector());

    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if (actor->user.FlagOwner != 1)
    {
    }
    else if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        if (!SoundValidAndActive(actor, CHAN_AnimeMad))
        {
            short choose;
            choose = RandomRange(1000);

            if (choose > 750)
                PlaySound(DIGI_LANI073, actor, v3df_dontpan, CHAN_AnimeMad);
            else if (choose > 500)
                PlaySound(DIGI_LANI075, actor, v3df_dontpan, CHAN_AnimeMad);
            else if (choose > 250)
                PlaySound(DIGI_LANI077, actor, v3df_dontpan, CHAN_AnimeMad);
            else
                PlaySound(DIGI_LANI079, actor, v3df_dontpan, CHAN_AnimeMad);
        }
        ChangeState(actor,s_MechanicGirlDrill);
        actor->user.WaitTics = SEC(1)+SEC(RandomRange(2<<8)>>8);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int MechanicGirlDrill(DSWActor* actor)
{
    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
    {
        actor->user.WaitTics = RandomRange(240)+120;
        ChangeState(actor,s_MechanicGirlStand);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int MechanicGirlPain(DSWActor* actor)
{
    NullMechanicGirl(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(actor,s_MechanicGirlStand);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

ATTRIBUTE SailorGirlAttrib =
{
    {0, 0, 0, 0},                     // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    3,                               // MaxWeapons;
    {
        0, 0, DIGI_TOILETGIRLSCREAM,
        DIGI_TOILETGIRLPAIN, DIGI_TOILETGIRLSCREAM, 0,0,0,0,0
    }
};

//////////////////////
//
// SAILORGIRL STAND
//
//////////////////////
#define SAILORGIRL_RATE 60

STATE s_SailorGirlStand[2] =
{
    {SAILORGIRL_R0 + 0, SAILORGIRL_RATE, &AF(DoSailorGirl), &s_SailorGirlStand[1]},
    {SAILORGIRL_R0 + 1, SAILORGIRL_RATE, &AF(DoSailorGirl), &s_SailorGirlStand[0]}
};

//////////////////////
//
// SAILORGIRL PAIN
//
//////////////////////

#define SAILORGIRL_PAIN_RATE 32
#define SAILORGIRL_PAIN_R0 SAILORGIRL_R0

STATE s_SailorGirlPain[2] =
{
    {SAILORGIRL_PAIN_R0 + 0, SAILORGIRL_PAIN_RATE, &AF(SailorGirlPain), &s_SailorGirlPain[1]},
    {SAILORGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SailorGirlPain[0]}
};

//////////////////////
//
// SAILORGIRL UZI
//
//////////////////////

#define SAILORGIRL_UZI_RATE 128
#define SAILORGIRL_FIRE_R0 SAILORGIRL_R0 + 2

STATE s_SailorGirlThrow[] =
{
    {SAILORGIRL_FIRE_R0 + 0, SAILORGIRL_UZI_RATE, &AF(SailorGirlThrow), &s_SailorGirlThrow[0]},
};

short alreadythrew;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupSailorGirl(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, SAILORGIRL_R0,s_SailorGirlStand);
        actor->user.Health = 60;
    }


    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_SailorGirlStand);
    actor->user.__legacyState.Attrib = &SailorGirlAttrib;
    actor->user.__legacyState.StateEnd = s_SailorGirlStand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;

	actor->spr.scale = DVector2(0.4375, 0.40625);
    actor->vel.X = 0;
    actor->vel.Z = 0;
    actor->spr.lotag = SAILORGIRL_R0;
    actor->user.FlagOwner = 0;
    actor->user.ID = SAILORGIRL_R0;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);
    alreadythrew = 0;

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWSailorGirl, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupEel(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSailorGirl(DSWActor* actor)
{
    bool ICanSee = false;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor),actor->sector(),ActorVectOfMiddle(actor->user.targetActor),actor->user.targetActor->sector());

    if (actor->user.FlagOwner == 1)
    {
        if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
        {
            if (!SoundValidAndActive(actor, CHAN_AnimeMad))
            {
                short choose;
                choose = RandomRange(1000);

                if (choose > 750 && alreadythrew < 3)
                {
                    ActorCoughItem(actor);
                    alreadythrew++;
                    PlaySound(DIGI_LANI060, actor, v3df_dontpan, CHAN_AnimeMad);
                }
                else if (choose > 500)
                    PlaySound(DIGI_LANI063, actor, v3df_dontpan, CHAN_AnimeMad);
                else if (choose > 250)
                    PlaySound(DIGI_LANI065, actor, v3df_dontpan, CHAN_AnimeMad);
                else
                    PlaySound(DIGI_LANI066, actor, v3df_dontpan, CHAN_AnimeMad);
            }
            ChangeState(actor,s_SailorGirlThrow);
            actor->user.WaitTics = SEC(1)+SEC(RandomRange(3<<8)>>8);
            actor->user.FlagOwner = 0;
        }
    }

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING | SPR_CLIMBING)))
    {
        KeepActorOnFloor(actor);
    }

    // take damage from environment
    DoActorSectorDamage(actor);
    actor->vel.X = 0;
    actor->vel.Z = 0;

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullSailorGirl(DSWActor* actor)
{
    bool ICanSee = false;
    static short alreadythrew = 0;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor),actor->sector(),actor->user.targetActor->spr.pos, actor->user.targetActor->sector());

    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if (actor->user.FlagOwner != 1)
    {
    }
    else if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        if (!SoundValidAndActive(actor, CHAN_AnimeMad))
        {
            short choose;
            choose = RandomRange(1000);

            if (choose > 750 && alreadythrew < 3)
            {
                ActorCoughItem(actor);
                alreadythrew++;
                PlaySound(DIGI_LANI060, actor, v3df_dontpan, CHAN_AnimeMad);
            }
            else if (choose > 500)
                PlaySound(DIGI_LANI063, actor, v3df_dontpan, CHAN_AnimeMad);
            else if (choose > 250)
                PlaySound(DIGI_LANI065, actor, v3df_dontpan, CHAN_AnimeMad);
            else
                PlaySound(DIGI_LANI066, actor, v3df_dontpan, CHAN_AnimeMad);
        }
        ChangeState(actor,s_SailorGirlThrow);
        actor->user.WaitTics = SEC(1)+SEC(RandomRange(3<<8)>>8);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SailorGirlThrow(DSWActor* actor)
{
    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
    {
        actor->user.WaitTics = RandomRange(240)+120;
        ChangeState(actor,s_SailorGirlStand);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SailorGirlPain(DSWActor* actor)
{
    NullSailorGirl(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(actor,s_SailorGirlStand);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------


ATTRIBUTE PruneGirlAttrib =
{
    {450, 450, 450, 450},             // Speeds
    {0, 0, 0, 0},                     // Tic Adjusts
    3,                               // MaxWeapons;
    //{DIGI_PRUNECACKLE3, DIGI_PRUNECACKLE, DIGI_TOILETGIRLSCREAM,
    {
        0,0, DIGI_TOILETGIRLSCREAM,
        DIGI_TOILETGIRLPAIN, DIGI_TOILETGIRLSCREAM, 0,0,0,0,0
    }
};

//////////////////////
//
// PRUNEGIRL STAND
//
//////////////////////
#define PRUNEGIRL_RATE 60

STATE s_PruneGirlStand[2] =
{
    {PRUNEGIRL_R0 + 0, PRUNEGIRL_RATE, &AF(DoPruneGirl), &s_PruneGirlStand[1]},
    {PRUNEGIRL_R0 + 1, PRUNEGIRL_RATE, &AF(DoPruneGirl), &s_PruneGirlStand[0]}
};

//////////////////////
//
// PRUNEGIRL PAIN
//
//////////////////////

#define PRUNEGIRL_PAIN_RATE 32
#define PRUNEGIRL_PAIN_R0 PRUNEGIRL_R0

STATE s_PruneGirlPain[2] =
{
    {PRUNEGIRL_PAIN_R0 + 0, PRUNEGIRL_PAIN_RATE, &AF(PruneGirlPain), &s_PruneGirlPain[1]},
    {PRUNEGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_PruneGirlPain[0]}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupPruneGirl(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, PRUNEGIRL_R0,s_PruneGirlStand);
        actor->user.Health = 60;
    }


    EnemyDefaults(actor, nullptr, nullptr);

    ChangeState(actor,s_PruneGirlStand);
    actor->user.__legacyState.Attrib = &PruneGirlAttrib;
    actor->user.__legacyState.StateEnd = s_PruneGirlStand;
    actor->user.__legacyState.Rot = 0;
    actor->user.__legacyState.RotNum = 0;

	actor->spr.scale = DVector2(0.515625, 0.4375);
    actor->vel.X = 0;
    actor->vel.Z = 0;
    actor->spr.lotag = PRUNEGIRL_R0;
    actor->user.FlagOwner = 0;
    actor->user.ID = PRUNEGIRL_R0;

    actor->user.Flags &= ~(SPR_XFLIP_TOGGLE);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWPruneGirl, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupPruneGirl(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoPruneGirl(DSWActor* actor)
{
    bool ICanSee = false;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor),actor->sector(),ActorVectOfMiddle(actor->user.targetActor),actor->user.targetActor->sector());

    if (actor->user.FlagOwner == 1)
    {
        if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
        {
            if (!SoundValidAndActive(actor, CHAN_AnimeMad))
            {
                short choose;
                choose = StdRandomRange(1000);

                if (choose > 750)
                    PlaySound(DIGI_LANI089, actor, v3df_dontpan, CHAN_AnimeMad);
                else if (choose > 500)
                    PlaySound(DIGI_LANI091, actor, v3df_dontpan, CHAN_AnimeMad);
                else if (choose > 250)
                    PlaySound(DIGI_LANI093, actor, v3df_dontpan, CHAN_AnimeMad);
                else
                    PlaySound(DIGI_LANI095, actor, v3df_dontpan, CHAN_AnimeMad);
            }
            actor->user.WaitTics = SEC(1)+SEC(RandomRange(3<<8)>>8);
            actor->user.FlagOwner = 0;
        }
    }
    else
    {
        if (!SoundValidAndActive(actor, CHAN_CoyHandle))
        {
            short choose;
            choose = StdRandomRange(1000);

            if (choose > 990)
                PlaySound(DIGI_PRUNECACKLE, actor, v3df_dontpan, CHAN_CoyHandle);
            else if (choose > 985)
                PlaySound(DIGI_PRUNECACKLE2, actor, v3df_dontpan, CHAN_CoyHandle);
            else if (choose > 980)
                PlaySound(DIGI_PRUNECACKLE3, actor, v3df_dontpan, CHAN_CoyHandle);
            else if (choose > 975)
                PlaySound(DIGI_LANI091, actor, v3df_dontpan, CHAN_CoyHandle);
        }
    }

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING | SPR_CLIMBING)))
    {
        KeepActorOnFloor(actor);
    }

    // take damage from environment
    DoActorSectorDamage(actor);
    actor->vel.X = 0;
    actor->vel.Z = 0;

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullPruneGirl(DSWActor* actor)
{
    bool ICanSee = false;

    DoActorPickClosePlayer(actor);
    ICanSee = FAFcansee(ActorVectOfMiddle(actor),actor->sector(),actor->user.targetActor->spr.pos, actor->user.targetActor->sector());

    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if (actor->user.FlagOwner != 1)
    {
    }
    else if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        if (!SoundValidAndActive(actor, CHAN_AnimeMad))
        {
            short choose;
            choose = RandomRange(1000);

            if (choose > 750)
                PlaySound(DIGI_LANI089, actor, v3df_dontpan, CHAN_AnimeMad);
            else if (choose > 500)
                PlaySound(DIGI_LANI091, actor, v3df_dontpan, CHAN_AnimeMad);
            else if (choose > 250)
                PlaySound(DIGI_LANI093, actor, v3df_dontpan, CHAN_AnimeMad);
            else
                PlaySound(DIGI_LANI095, actor, v3df_dontpan, CHAN_AnimeMad);
        }
        actor->user.WaitTics = SEC(1)+SEC(RandomRange(3<<8)>>8);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int PruneGirlUzi(DSWActor* actor)
{
    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
    {
        actor->user.WaitTics = RandomRange(240)+120;
        ChangeState(actor,s_PruneGirlStand);
        actor->user.FlagOwner = 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int PruneGirlPain(DSWActor* actor)
{
    NullPruneGirl(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(actor,s_PruneGirlStand);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------


#include "saveable.h"

static saveable_data saveable_miscactr_data[] =
{
    SAVE_DATA(ToiletGirlAttrib),
    SAVE_DATA(WashGirlAttrib),
    SAVE_DATA(TrashCanAttrib),
    SAVE_DATA(PachinkoLightAttrib),
    SAVE_DATA(Pachinko1Attrib),
    SAVE_DATA(Pachinko2Attrib),
    SAVE_DATA(Pachinko3Attrib),
    SAVE_DATA(Pachinko4Attrib),
    SAVE_DATA(CarGirlAttrib),
    SAVE_DATA(MechanicGirlAttrib),
    SAVE_DATA(SailorGirlAttrib),
    SAVE_DATA(PruneGirlAttrib),

    SAVE_DATA(s_ToiletGirlStand),
    SAVE_DATA(s_ToiletGirlPain),
    SAVE_DATA(s_ToiletGirlUzi),
    SAVE_DATA(s_WashGirlStand),
    SAVE_DATA(s_WashGirlStandScrub),
    SAVE_DATA(s_WashGirlPain),
    SAVE_DATA(s_WashGirlUzi),
    SAVE_DATA(s_TrashCanStand),
    SAVE_DATA(s_TrashCanPain),
    SAVE_DATA(s_PachinkoLightStand),
    SAVE_DATA(s_PachinkoLightOperate),
    SAVE_DATA(s_Pachinko1Stand),
    SAVE_DATA(s_Pachinko1Operate),
    SAVE_DATA(s_Pachinko2Stand),
    SAVE_DATA(s_Pachinko2Operate),
    SAVE_DATA(s_Pachinko3Stand),
    SAVE_DATA(s_Pachinko3Operate),
    SAVE_DATA(s_Pachinko4Stand),
    SAVE_DATA(s_Pachinko4Operate),
    SAVE_DATA(s_CarGirlStand),
    SAVE_DATA(s_CarGirlPain),
    SAVE_DATA(s_CarGirlUzi),
    SAVE_DATA(s_MechanicGirlStand),
    SAVE_DATA(s_MechanicGirlPain),
    SAVE_DATA(s_MechanicGirlDrill),
    SAVE_DATA(s_SailorGirlStand),
    SAVE_DATA(s_SailorGirlPain),
    SAVE_DATA(s_SailorGirlThrow),
    SAVE_DATA(s_PruneGirlStand),
    SAVE_DATA(s_PruneGirlPain)
};

saveable_module saveable_miscactr =
{
    // code
    nullptr, 0,

    // data
    saveable_miscactr_data,
    SIZ(saveable_miscactr_data)
};
END_SW_NS
