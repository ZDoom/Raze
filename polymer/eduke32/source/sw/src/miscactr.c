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
#include "quake.h"
#include "fx_man.h"
#include "actor.h"
#include "sector.h"
#include "sprite.h"


ANIMATOR NullToiletGirl;

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
ANIMATOR NullToiletGirl,DoToiletGirl;

STATE s_ToiletGirlStand[2] =
{
    {TOILETGIRL_R0 + 0, TOILETGIRL_RATE, DoToiletGirl, &s_ToiletGirlStand[1]},
    {TOILETGIRL_R0 + 1, TOILETGIRL_RATE, DoToiletGirl, &s_ToiletGirlStand[0]}
};

//////////////////////
//
// TOILETGIRL PAIN
//
//////////////////////

#define TOILETGIRL_PAIN_RATE 32
#define TOILETGIRL_PAIN_R0 TOILETGIRL_R0
ANIMATOR ToiletGirlPain;

STATE s_ToiletGirlPain[2] =
{
    {TOILETGIRL_PAIN_R0 + 0, TOILETGIRL_PAIN_RATE, ToiletGirlPain, &s_ToiletGirlPain[1]},
    {TOILETGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_ToiletGirlPain[0]}
};

//////////////////////
//
// TOILETGIRL UZI
//
//////////////////////

#define TOILETGIRL_UZI_RATE 8
#define TOILETGIRL_FIRE_R0 TOILETGIRL_R0 + 2
ANIMATOR InitEnemyUzi,ToiletGirlUzi;

STATE s_ToiletGirlUzi[16] =
{
    {TOILETGIRL_FIRE_R0 + 0, TOILETGIRL_UZI_RATE, ToiletGirlUzi, &s_ToiletGirlUzi[1]},
    {TOILETGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ToiletGirlUzi[2]},
    {TOILETGIRL_FIRE_R0 + 1, TOILETGIRL_UZI_RATE, ToiletGirlUzi, &s_ToiletGirlUzi[3]},
    {TOILETGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ToiletGirlUzi[4]},
    {TOILETGIRL_FIRE_R0 + 0, TOILETGIRL_UZI_RATE, ToiletGirlUzi, &s_ToiletGirlUzi[5]},
    {TOILETGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ToiletGirlUzi[6]},
    {TOILETGIRL_FIRE_R0 + 1, TOILETGIRL_UZI_RATE, ToiletGirlUzi, &s_ToiletGirlUzi[7]},
    {TOILETGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ToiletGirlUzi[8]},
    {TOILETGIRL_FIRE_R0 + 0, TOILETGIRL_UZI_RATE, ToiletGirlUzi, &s_ToiletGirlUzi[9]},
    {TOILETGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ToiletGirlUzi[10]},
    {TOILETGIRL_FIRE_R0 + 1, TOILETGIRL_UZI_RATE, ToiletGirlUzi, &s_ToiletGirlUzi[11]},
    {TOILETGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ToiletGirlUzi[12]},
    {TOILETGIRL_FIRE_R0 + 0, TOILETGIRL_UZI_RATE, ToiletGirlUzi, &s_ToiletGirlUzi[13]},
    {TOILETGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ToiletGirlUzi[14]},
    {TOILETGIRL_FIRE_R0 + 1, TOILETGIRL_UZI_RATE, ToiletGirlUzi, &s_ToiletGirlUzi[15]},
    {TOILETGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_ToiletGirlUzi[0]},
};

int
SetupToiletGirl(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,TOILETGIRL_R0,s_ToiletGirlStand);
        u->Health = 60;
    }


    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_ToiletGirlStand);
    u->Attrib = &ToiletGirlAttrib;
    u->StateEnd = s_ToiletGirlStand;
    u->Rot = 0;
    u->RotNum = 0;

    sp->xrepeat = 38;
    sp->yrepeat = 32;
    sp->xvel = sp->yvel = sp->zvel = 0;
    sp->lotag = TOILETGIRL_R0;
    u->FlagOwner = 0;
    u->ID = TOILETGIRL_R0;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);

    return 0;
}

int DoToiletGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short rnd_range = 0;
    static int handle=0;
    SWBOOL ICanSee = FALSE;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,SPRITEp_MID(u->tgt_sp),u->tgt_sp->sectnum);

    if (u->FlagOwner != 1)
    {
        if (RANDOM_RANGE(1000) > 980)
        {
            static int handle;
            short choose_snd;

            choose_snd = RANDOM_P2(1024<<4)>>4;

            if (!FX_SoundActive(handle))
            {
                if (choose_snd > 750)
                    handle = PlaySound(DIGI_TOILETGIRLFART1,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else if (choose_snd > 350)
                    handle = PlaySound(DIGI_TOILETGIRLFART2,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else
                    handle = PlaySound(DIGI_TOILETGIRLFART3,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            }
        }
    }
    else if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        static int madhandle;

        if (!FX_SoundActive(madhandle))
        {
            if (RANDOM_RANGE(1000<<8)>>8 > 500)
                madhandle = PlaySound(DIGI_ANIMEMAD1,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else
                madhandle = PlaySound(DIGI_ANIMEMAD2,&sp->x,&sp->y,&sp->z,v3df_dontpan);
        }
        ChangeState(SpriteNum,s_ToiletGirlUzi);
        u->WaitTics = SEC(1)+SEC(RANDOM_RANGE(3<<8)>>8);
        u->FlagOwner = 0;
    }

    //(*u->ActorActionFunc) (SpriteNum);


    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING | SPR_CLIMBING))
    {
        KeepActorOnFloor(SpriteNum);
    }

    // take damage from environment
    DoActorSectorDamage(SpriteNum);
    sp->xvel = sp->yvel = sp->zvel = 0;

    return 0;
}

int NullToiletGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SWBOOL ICanSee = FALSE;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,u->tgt_sp->z,u->tgt_sp->sectnum);

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if (u->FlagOwner != 1)
    {
    }
    else if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        static int madhandle;

        if (!FX_SoundActive(madhandle))
        {
            if (RANDOM_RANGE(1000<<8)>>8 > 500)
                madhandle = PlaySound(DIGI_ANIMEMAD1,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else
                madhandle = PlaySound(DIGI_ANIMEMAD2,&sp->x,&sp->y,&sp->z,v3df_dontpan);
        }
        ChangeState(SpriteNum,s_ToiletGirlUzi);
        u->WaitTics = SEC(1)+SEC(RANDOM_RANGE(3<<8)>>8);
        u->FlagOwner = 0;
    }

    return 0;
}

int ToiletGirlUzi(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
    {
        u->WaitTics = RANDOM_RANGE(240)+120;
        ChangeState(SpriteNum,s_ToiletGirlStand);
        u->FlagOwner = 0;
    }

    return 0;
}

int ToiletGirlPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullToiletGirl(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(SpriteNum,s_ToiletGirlStand);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

ANIMATOR NullWashGirl;

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
ANIMATOR NullWashGirl,DoWashGirl;

STATE s_WashGirlStand[2] =
{
    {WASHGIRL_R0 + 0, WASHGIRL_RATE, DoWashGirl, &s_WashGirlStand[1]},
    {WASHGIRL_R0 + 1, WASHGIRL_RATE, DoWashGirl, &s_WashGirlStand[0]}
};

#define WASHGIRL_RATE2 20
STATE s_WashGirlStandScrub[2] =
{
    {WASHGIRL_R0 + 0, WASHGIRL_RATE2, DoWashGirl, &s_WashGirlStandScrub[1]},
    {WASHGIRL_R0 + 1, WASHGIRL_RATE2, DoWashGirl, &s_WashGirlStandScrub[0]}
};

//////////////////////
//
// WASHGIRL PAIN
//
//////////////////////

#define WASHGIRL_PAIN_RATE 30
#define WASHGIRL_PAIN_R0 WASHGIRL_R0
ANIMATOR WashGirlPain;

STATE s_WashGirlPain[2] =
{
    {WASHGIRL_PAIN_R0 + 0, WASHGIRL_PAIN_RATE, WashGirlPain, &s_WashGirlPain[1]},
    {WASHGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_WashGirlPain[0]}
};

//////////////////////
//
// WASHGIRL UZI
//
//////////////////////

#define WASHGIRL_UZI_RATE 8
#define WASHGIRL_FIRE_R0 WASHGIRL_R0 + 2
ANIMATOR InitEnemyUzi,WashGirlUzi;

STATE s_WashGirlUzi[16] =
{
    {WASHGIRL_FIRE_R0 + 0, WASHGIRL_UZI_RATE, WashGirlUzi, &s_WashGirlUzi[1]},
    {WASHGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_WashGirlUzi[2]},
    {WASHGIRL_FIRE_R0 + 1, WASHGIRL_UZI_RATE, WashGirlUzi, &s_WashGirlUzi[3]},
    {WASHGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_WashGirlUzi[4]},
    {WASHGIRL_FIRE_R0 + 0, WASHGIRL_UZI_RATE, WashGirlUzi, &s_WashGirlUzi[5]},
    {WASHGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_WashGirlUzi[6]},
    {WASHGIRL_FIRE_R0 + 1, WASHGIRL_UZI_RATE, WashGirlUzi, &s_WashGirlUzi[7]},
    {WASHGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_WashGirlUzi[8]},
    {WASHGIRL_FIRE_R0 + 0, WASHGIRL_UZI_RATE, WashGirlUzi, &s_WashGirlUzi[9]},
    {WASHGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_WashGirlUzi[10]},
    {WASHGIRL_FIRE_R0 + 1, WASHGIRL_UZI_RATE, WashGirlUzi, &s_WashGirlUzi[11]},
    {WASHGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_WashGirlUzi[12]},
    {WASHGIRL_FIRE_R0 + 0, WASHGIRL_UZI_RATE, WashGirlUzi, &s_WashGirlUzi[13]},
    {WASHGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_WashGirlUzi[14]},
    {WASHGIRL_FIRE_R0 + 1, WASHGIRL_UZI_RATE, WashGirlUzi, &s_WashGirlUzi[15]},
    {WASHGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_WashGirlUzi[0]},
};

int
SetupWashGirl(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,WASHGIRL_R0,s_WashGirlStand);
        u->Health = 60;
    }

    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_WashGirlStand);
    u->Attrib = &WashGirlAttrib;
    u->StateEnd = s_WashGirlStand;
    u->Rot = 0;
    u->RotNum = 0;

    sp->xrepeat = 28;
    sp->yrepeat = 24;
    sp->xvel = sp->yvel = sp->zvel = 0;
    sp->lotag = WASHGIRL_R0;
    u->FlagOwner = 0;
    u->ID = WASHGIRL_R0;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);

    return 0;
}

int DoWashGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short rnd_range = 0;
    static int handle=0;
    SWBOOL ICanSee = FALSE;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,SPRITEp_MID(u->tgt_sp),u->tgt_sp->sectnum);

    if (RANDOM_RANGE(1000) > 980 && u->ShellNum <= 0)
    {
        static int handle;

        if (!FX_SoundActive(handle))
        {
            if (RANDOM_P2(1024<<4)>>4 > 500)
                handle = PlaySound(DIGI_ANIMESING1,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else
                handle = PlaySound(DIGI_ANIMESING2,&sp->x,&sp->y,&sp->z,v3df_dontpan);
        }

        ChangeState(SpriteNum,s_WashGirlStandScrub);
        u->ShellNum = RANDOM_RANGE(2*120)+240;
    }
    else
    {
        if (u->ShellNum > 0)
        {
            if ((u->ShellNum -= ACTORMOVETICS) < 0)
            {
                ChangeState(SpriteNum,s_WashGirlStand);
                u->ShellNum = 0;
            }
        }
    }

    if (u->FlagOwner != 1)
    {
    }
    else if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        static int madhandle;

        if (!FX_SoundActive(madhandle))
        {
            if (RANDOM_RANGE(1000<<8)>>8 > 500)
                madhandle = PlaySound(DIGI_ANIMEMAD1,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else
                madhandle = PlaySound(DIGI_ANIMEMAD2,&sp->x,&sp->y,&sp->z,v3df_dontpan);
        }
        ChangeState(SpriteNum,s_WashGirlUzi);
        u->WaitTics = SEC(1)+SEC(RANDOM_RANGE(3<<8)>>8);
        u->FlagOwner = 0;
    }

    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING | SPR_CLIMBING))
    {
        KeepActorOnFloor(SpriteNum);
    }

    // take damage from environment
    DoActorSectorDamage(SpriteNum);
    sp->xvel = sp->yvel = sp->zvel = 0;

    return 0;
}

int NullWashGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SWBOOL ICanSee = FALSE;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,u->tgt_sp->z,u->tgt_sp->sectnum);

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if (u->FlagOwner != 1)
    {
    }
    else if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        static int madhandle;

        if (!FX_SoundActive(madhandle))
        {
            if (RANDOM_RANGE(1000<<8)>>8 > 500)
                madhandle = PlaySound(DIGI_ANIMEMAD1,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else
                madhandle = PlaySound(DIGI_ANIMEMAD2,&sp->x,&sp->y,&sp->z,v3df_dontpan);
        }
        ChangeState(SpriteNum,s_WashGirlUzi);
        u->WaitTics = SEC(1)+SEC(RANDOM_RANGE(3<<8)>>8);
        u->FlagOwner = 0;
    }

    return 0;
}

int WashGirlUzi(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
    {
        u->WaitTics = RANDOM_RANGE(240)+120;
        ChangeState(SpriteNum,s_WashGirlStand);
        u->FlagOwner = 0;
    }

    return 0;
}

int WashGirlPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullWashGirl(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(SpriteNum,s_WashGirlStand);

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////

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
ANIMATOR NullTrashCan,DoTrashCan;

STATE s_TrashCanStand[1] =
{
    {TRASHCAN_R0 + 0, TRASHCAN_RATE, DoTrashCan, &s_TrashCanStand[0]}
};

//////////////////////
//
// TRASHCAN PAIN
//
//////////////////////

#define TRASHCAN_PAIN_RATE 8
#define TRASHCAN_PAIN_R0 TRASHCAN
ANIMATOR TrashCanPain, TrashCanStand;

STATE s_TrashCanPain[7] =
{
    {TRASHCAN_PAIN_R0 + 0, TRASHCAN_PAIN_RATE, TrashCanPain, &s_TrashCanPain[1]},
    {TRASHCAN_PAIN_R0 + 1, TRASHCAN_PAIN_RATE, TrashCanPain, &s_TrashCanPain[2]},
    {TRASHCAN_PAIN_R0 + 2, TRASHCAN_PAIN_RATE, TrashCanPain, &s_TrashCanPain[3]},
    {TRASHCAN_PAIN_R0 + 3, TRASHCAN_PAIN_RATE, TrashCanPain, &s_TrashCanPain[4]},
    {TRASHCAN_PAIN_R0 + 4, TRASHCAN_PAIN_RATE, TrashCanPain, &s_TrashCanPain[5]},
    {TRASHCAN_PAIN_R0 + 5, TRASHCAN_PAIN_RATE, TrashCanPain, &s_TrashCanPain[6]},
    {TRASHCAN_PAIN_R0 + 6, TRASHCAN_PAIN_RATE, TrashCanPain, &s_TrashCanPain[0]}
};

int
SetupTrashCan(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,TRASHCAN,s_TrashCanStand);
        u->Health = 60;
    }

    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_TrashCanStand);
    u->Attrib = &TrashCanAttrib;
    u->StateEnd = s_TrashCanStand;
    u->Rot = 0;
    u->RotNum = 0;


    sp->xrepeat = 46;
    sp->yrepeat = 42;
    sp->xvel = sp->yvel = sp->zvel = 0;
    u->ID = TRASHCAN;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);
    RESET(sp->extra, SPRX_PLAYER_OR_ENEMY);

    return 0;
}

int DoTrashCan(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short rnd_range = 0;
    static int handle=0;

    //(*u->ActorActionFunc) (SpriteNum);

    // stay on floor unless doing certain things
    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING | SPR_CLIMBING))
    {
        KeepActorOnFloor(SpriteNum);
    }

    sp->xvel = sp->yvel = sp->zvel = 0;

    return 0;
}

int TrashCanPain(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(SpriteNum,s_TrashCanStand);

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// PACHINKO WIN LIGHT
////////////////////////////////////////////////////////////////////
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
    {PACHINKOLIGHT_R0 + 0, PACHINKOLIGHT_RATE, NullAnimator, &s_PachinkoLightStand[0]}
};

ANIMATOR PachinkoLightOperate;
STATE s_PachinkoLightOperate[] =
{
    {PACHINKOLIGHT_R0 - 0, 12, PachinkoLightOperate, &s_PachinkoLightOperate[1]},
    {PACHINKOLIGHT_R0 - 1, 12, PachinkoLightOperate, &s_PachinkoLightOperate[2]},
    {PACHINKOLIGHT_R0 - 2, 12, PachinkoLightOperate, &s_PachinkoLightOperate[3]},
    {PACHINKOLIGHT_R0 - 3, 12, PachinkoLightOperate, &s_PachinkoLightOperate[4]},
    {PACHINKOLIGHT_R0 - 4, 12, PachinkoLightOperate, &s_PachinkoLightOperate[5]},
    {PACHINKOLIGHT_R0 - 5, 12, PachinkoLightOperate, &s_PachinkoLightOperate[0]},
};

int
SetupPachinkoLight(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,PACHINKOLIGHT_R0,s_PachinkoLightStand);
        u->Health = 1;
    }

    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_PachinkoLightStand);
    u->Attrib = &PachinkoLightAttrib;
    u->StateEnd = s_PachinkoLightStand;
    u->Rot = 0;
    u->RotNum = 0;
    u->ID = PACHINKOLIGHT_R0;

    sp->xvel = sp->yvel = sp->zvel = 0;
    sp->lotag = TAG_PACHINKOLIGHT;
    sp->shade = -2;
    u->spal = sp->pal;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);
    RESET(sp->extra, SPRX_PLAYER_OR_ENEMY);

    return 0;
}

int PachinkoLightOperate(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
    {
        sp->shade = -2;
        ChangeState(SpriteNum,s_PachinkoLightStand);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////
// PACHINKO MACHINE #1
////////////////////////////////////////////////////////////////////

SWBOOL Pachinko_Win_Cheat = FALSE;

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
    {PACHINKO1_R0 + 0, PACHINKO1_RATE, NullAnimator, &s_Pachinko1Stand[0]}
};

ANIMATOR Pachinko1Operate,PachinkoCheckWin;
STATE s_Pachinko1Operate[] =
{
    {PACHINKO1_R0 + 0, 12, Pachinko1Operate, &s_Pachinko1Operate[1]},
    {PACHINKO1_R0 + 1, 12, Pachinko1Operate, &s_Pachinko1Operate[2]},
    {PACHINKO1_R0 + 2, 12, Pachinko1Operate, &s_Pachinko1Operate[3]},
    {PACHINKO1_R0 + 3, 12, Pachinko1Operate, &s_Pachinko1Operate[4]},
    {PACHINKO1_R0 + 4, 12, Pachinko1Operate, &s_Pachinko1Operate[5]},
    {PACHINKO1_R0 + 5, 12, Pachinko1Operate, &s_Pachinko1Operate[6]},
    {PACHINKO1_R0 + 6, 12, Pachinko1Operate, &s_Pachinko1Operate[7]},
    {PACHINKO1_R0 + 7, 12, Pachinko1Operate, &s_Pachinko1Operate[8]},
    {PACHINKO1_R0 + 8, 12, Pachinko1Operate, &s_Pachinko1Operate[9]},
    {PACHINKO1_R0 + 9, 12, Pachinko1Operate, &s_Pachinko1Operate[10]},
    {PACHINKO1_R0 + 10, 12, Pachinko1Operate, &s_Pachinko1Operate[11]},
    {PACHINKO1_R0 + 11, 12, Pachinko1Operate, &s_Pachinko1Operate[12]},
    {PACHINKO1_R0 + 12, 12, Pachinko1Operate, &s_Pachinko1Operate[13]},
    {PACHINKO1_R0 + 13, 12, Pachinko1Operate, &s_Pachinko1Operate[14]},
    {PACHINKO1_R0 + 14, 12, Pachinko1Operate, &s_Pachinko1Operate[15]},
    {PACHINKO1_R0 + 15, 12, Pachinko1Operate, &s_Pachinko1Operate[16]},
    {PACHINKO1_R0 + 16, 12, Pachinko1Operate, &s_Pachinko1Operate[17]},
    {PACHINKO1_R0 + 17, 12, Pachinko1Operate, &s_Pachinko1Operate[18]},
    {PACHINKO1_R0 + 18, 12, Pachinko1Operate, &s_Pachinko1Operate[19]},
    {PACHINKO1_R0 + 19, 12, Pachinko1Operate, &s_Pachinko1Operate[20]},
    {PACHINKO1_R0 + 20, 12, Pachinko1Operate, &s_Pachinko1Operate[21]},
    {PACHINKO1_R0 + 21, 12, Pachinko1Operate, &s_Pachinko1Operate[22]},
    {PACHINKO1_R0 + 22, SF_QUICK_CALL, PachinkoCheckWin, &s_Pachinko1Stand[0]}
};

int
SetupPachinko1(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,PACHINKO1,s_Pachinko1Stand);
        u->Health = 1;
    }

    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_Pachinko1Stand);
    u->Attrib = &Pachinko1Attrib;
    u->StateEnd = s_Pachinko1Stand;
    u->Rot = 0;
    u->RotNum = 0;
    u->ID = PACHINKO1;

    sp->yvel = sp->zvel = 0;
    sp->lotag = PACHINKO1;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);
    RESET(sp->extra, SPRX_PLAYER_OR_ENEMY);

    return 0;
}

int PachinkoCheckWin(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    short rnd;

    u->WaitTics = 0;  // Can operate it again now

    //CON_ConMessage("bool1 = %d",TEST_BOOL1(sp));
    // You already won, no more from this machine!
    if (TEST_BOOL1(sp)) return 0;

    // Well? Did I win????!
    rnd = RANDOM_RANGE(1000);
    if (RANDOM_RANGE(1000) > 900 || Pachinko_Win_Cheat)
    {
        short i, nexti;
        SPRITEp tsp;
        USERp tu;

        // Do a possible combo switch
        if (ComboSwitchTest(TAG_COMBO_SWITCH_EVERYTHING, sp->hitag))
        {
            DoMatchEverything(Player+myconnectindex, sp->hitag, ON);
        }

        ActorCoughItem(SpriteNum); // I WON! I WON!
        PlaySound(DIGI_PALARM,&sp->x,&sp->y,&sp->z,v3df_none);

        // Can't win any more now!
        SET_BOOL1(sp);

        // Turn on the pachinko lights
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], i, nexti)
        {
            tsp = &sprite[i];
            tu = User[i];

            if (tsp->lotag == TAG_PACHINKOLIGHT)
            {
                if (tsp->hitag == SP_TAG5(sp))
                {
                    tsp->shade = -90; // Full brightness
                    tu->WaitTics = SEC(3); // Flash
                    ChangeState(i,s_PachinkoLightOperate);
                }
            }
        }

    }

    //{
    //if(rnd > 950)
    //    PlayerSound(DIGI_SHISEISI,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
    //else
    //if(rnd > 900)
    //    PlayerSound(DIGI_YOULOOKSTUPID,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
    //else
    //if(rnd > 850)
    //    PlayerSound(DIGI_HURTBAD5,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
    //}

    return 0;
}

int Pachinko1Operate(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    short rnd;
    PLAYERp pp = Player + myconnectindex;

    rnd = RANDOM_RANGE(1000);
    if (rnd > 900)
    {
        rnd = RANDOM_RANGE(1000);  // TEMP SOUNDS: Need pachinko sounds!
        if (rnd > 700)
            PlaySound(DIGI_PROLL1,&sp->x,&sp->y,&sp->z,v3df_none);
        else if (rnd > 400)
            PlaySound(DIGI_PROLL2,&sp->x,&sp->y,&sp->z,v3df_none);
        else
            PlaySound(DIGI_PROLL3,&sp->x,&sp->y,&sp->z,v3df_none);
    }

    return 0;
}


////////////////////////////////////////////////////////////////////
// PACHINKO MACHINE #2
////////////////////////////////////////////////////////////////////

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
    {PACHINKO2_R0 + 0, PACHINKO2_RATE, NullAnimator, &s_Pachinko2Stand[0]}
};

ANIMATOR Pachinko2Operate;
STATE s_Pachinko2Operate[] =
{
    {PACHINKO2_R0 + 0, 12, Pachinko1Operate, &s_Pachinko2Operate[1]},
    {PACHINKO2_R0 + 1, 12, Pachinko1Operate, &s_Pachinko2Operate[2]},
    {PACHINKO2_R0 + 2, 12, Pachinko1Operate, &s_Pachinko2Operate[3]},
    {PACHINKO2_R0 + 3, 12, Pachinko1Operate, &s_Pachinko2Operate[4]},
    {PACHINKO2_R0 + 4, 12, Pachinko1Operate, &s_Pachinko2Operate[5]},
    {PACHINKO2_R0 + 5, 12, Pachinko1Operate, &s_Pachinko2Operate[6]},
    {PACHINKO2_R0 + 6, 12, Pachinko1Operate, &s_Pachinko2Operate[7]},
    {PACHINKO2_R0 + 7, 12, Pachinko1Operate, &s_Pachinko2Operate[8]},
    {PACHINKO2_R0 + 8, 12, Pachinko1Operate, &s_Pachinko2Operate[9]},
    {PACHINKO2_R0 + 9, 12, Pachinko1Operate, &s_Pachinko2Operate[10]},
    {PACHINKO2_R0 + 10, 12, Pachinko1Operate, &s_Pachinko2Operate[11]},
    {PACHINKO2_R0 + 11, 12, Pachinko1Operate, &s_Pachinko2Operate[12]},
    {PACHINKO2_R0 + 12, 12, Pachinko1Operate, &s_Pachinko2Operate[13]},
    {PACHINKO2_R0 + 13, 12, Pachinko1Operate, &s_Pachinko2Operate[14]},
    {PACHINKO2_R0 + 14, 12, Pachinko1Operate, &s_Pachinko2Operate[15]},
    {PACHINKO2_R0 + 15, 12, Pachinko1Operate, &s_Pachinko2Operate[16]},
    {PACHINKO2_R0 + 16, 12, Pachinko1Operate, &s_Pachinko2Operate[17]},
    {PACHINKO2_R0 + 17, 12, Pachinko1Operate, &s_Pachinko2Operate[18]},
    {PACHINKO2_R0 + 18, 12, Pachinko1Operate, &s_Pachinko2Operate[19]},
    {PACHINKO2_R0 + 19, 12, Pachinko1Operate, &s_Pachinko2Operate[20]},
    {PACHINKO2_R0 + 20, 12, Pachinko1Operate, &s_Pachinko2Operate[21]},
    {PACHINKO2_R0 + 21, 12, Pachinko1Operate, &s_Pachinko2Operate[22]},
    {PACHINKO2_R0 + 22, SF_QUICK_CALL, PachinkoCheckWin, &s_Pachinko2Stand[0]}
};

int
SetupPachinko2(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,PACHINKO2,s_Pachinko2Stand);
        u->Health = 1;
    }

    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_Pachinko2Stand);
    u->Attrib = &Pachinko2Attrib;
    u->StateEnd = s_Pachinko2Stand;
    u->Rot = 0;
    u->RotNum = 0;
    u->ID = PACHINKO2;

    sp->yvel = sp->zvel = 0;
    sp->lotag = PACHINKO2;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);
    RESET(sp->extra, SPRX_PLAYER_OR_ENEMY);

    return 0;
}

////////////////////////////////////////////////////////////////////
// PACHINKO MACHINE #3
////////////////////////////////////////////////////////////////////

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
    {PACHINKO3_R0 + 0, PACHINKO3_RATE, NullAnimator, &s_Pachinko3Stand[0]}
};

ANIMATOR Pachinko3Operate;
STATE s_Pachinko3Operate[] =
{
    {PACHINKO3_R0 + 0, 12, Pachinko1Operate, &s_Pachinko3Operate[1]},
    {PACHINKO3_R0 + 1, 12, Pachinko1Operate, &s_Pachinko3Operate[2]},
    {PACHINKO3_R0 + 2, 12, Pachinko1Operate, &s_Pachinko3Operate[3]},
    {PACHINKO3_R0 + 3, 12, Pachinko1Operate, &s_Pachinko3Operate[4]},
    {PACHINKO3_R0 + 4, 12, Pachinko1Operate, &s_Pachinko3Operate[5]},
    {PACHINKO3_R0 + 5, 12, Pachinko1Operate, &s_Pachinko3Operate[6]},
    {PACHINKO3_R0 + 6, 12, Pachinko1Operate, &s_Pachinko3Operate[7]},
    {PACHINKO3_R0 + 7, 12, Pachinko1Operate, &s_Pachinko3Operate[8]},
    {PACHINKO3_R0 + 8, 12, Pachinko1Operate, &s_Pachinko3Operate[9]},
    {PACHINKO3_R0 + 9, 12, Pachinko1Operate, &s_Pachinko3Operate[10]},
    {PACHINKO3_R0 + 10, 12, Pachinko1Operate, &s_Pachinko3Operate[11]},
    {PACHINKO3_R0 + 11, 12, Pachinko1Operate, &s_Pachinko3Operate[12]},
    {PACHINKO3_R0 + 12, 12, Pachinko1Operate, &s_Pachinko3Operate[13]},
    {PACHINKO3_R0 + 13, 12, Pachinko1Operate, &s_Pachinko3Operate[14]},
    {PACHINKO3_R0 + 14, 12, Pachinko1Operate, &s_Pachinko3Operate[15]},
    {PACHINKO3_R0 + 15, 12, Pachinko1Operate, &s_Pachinko3Operate[16]},
    {PACHINKO3_R0 + 16, 12, Pachinko1Operate, &s_Pachinko3Operate[17]},
    {PACHINKO3_R0 + 17, 12, Pachinko1Operate, &s_Pachinko3Operate[18]},
    {PACHINKO3_R0 + 18, 12, Pachinko1Operate, &s_Pachinko3Operate[19]},
    {PACHINKO3_R0 + 19, 12, Pachinko1Operate, &s_Pachinko3Operate[20]},
    {PACHINKO3_R0 + 20, 12, Pachinko1Operate, &s_Pachinko3Operate[21]},
    {PACHINKO3_R0 + 21, 12, Pachinko1Operate, &s_Pachinko3Operate[22]},
    {PACHINKO3_R0 + 22, SF_QUICK_CALL, PachinkoCheckWin, &s_Pachinko3Stand[0]}
};

int
SetupPachinko3(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,PACHINKO3,s_Pachinko3Stand);
        u->Health = 1;
    }

    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_Pachinko3Stand);
    u->Attrib = &Pachinko3Attrib;
    u->StateEnd = s_Pachinko3Stand;
    u->Rot = 0;
    u->RotNum = 0;
    u->ID = PACHINKO3;

    sp->yvel = sp->zvel = 0;
    sp->lotag = PACHINKO3;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);
    RESET(sp->extra, SPRX_PLAYER_OR_ENEMY);

    return 0;
}


////////////////////////////////////////////////////////////////////
// PACHINKO MACHINE #4
////////////////////////////////////////////////////////////////////

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
    {PACHINKO4_R0 + 0, PACHINKO4_RATE, NullAnimator, &s_Pachinko4Stand[0]}
};

ANIMATOR Pachinko4Operate;
STATE s_Pachinko4Operate[] =
{
    {PACHINKO4_R0 + 0, 12, Pachinko1Operate, &s_Pachinko4Operate[1]},
    {PACHINKO4_R0 + 1, 12, Pachinko1Operate, &s_Pachinko4Operate[2]},
    {PACHINKO4_R0 + 2, 12, Pachinko1Operate, &s_Pachinko4Operate[3]},
    {PACHINKO4_R0 + 3, 12, Pachinko1Operate, &s_Pachinko4Operate[4]},
    {PACHINKO4_R0 + 4, 12, Pachinko1Operate, &s_Pachinko4Operate[5]},
    {PACHINKO4_R0 + 5, 12, Pachinko1Operate, &s_Pachinko4Operate[6]},
    {PACHINKO4_R0 + 6, 12, Pachinko1Operate, &s_Pachinko4Operate[7]},
    {PACHINKO4_R0 + 7, 12, Pachinko1Operate, &s_Pachinko4Operate[8]},
    {PACHINKO4_R0 + 8, 12, Pachinko1Operate, &s_Pachinko4Operate[9]},
    {PACHINKO4_R0 + 9, 12, Pachinko1Operate, &s_Pachinko4Operate[10]},
    {PACHINKO4_R0 + 10, 12, Pachinko1Operate, &s_Pachinko4Operate[11]},
    {PACHINKO4_R0 + 11, 12, Pachinko1Operate, &s_Pachinko4Operate[12]},
    {PACHINKO4_R0 + 12, 12, Pachinko1Operate, &s_Pachinko4Operate[13]},
    {PACHINKO4_R0 + 13, 12, Pachinko1Operate, &s_Pachinko4Operate[14]},
    {PACHINKO4_R0 + 14, 12, Pachinko1Operate, &s_Pachinko4Operate[15]},
    {PACHINKO4_R0 + 15, 12, Pachinko1Operate, &s_Pachinko4Operate[16]},
    {PACHINKO4_R0 + 16, 12, Pachinko1Operate, &s_Pachinko4Operate[17]},
    {PACHINKO4_R0 + 17, 12, Pachinko1Operate, &s_Pachinko4Operate[18]},
    {PACHINKO4_R0 + 18, 12, Pachinko1Operate, &s_Pachinko4Operate[19]},
    {PACHINKO4_R0 + 19, 12, Pachinko1Operate, &s_Pachinko4Operate[20]},
    {PACHINKO4_R0 + 20, 12, Pachinko1Operate, &s_Pachinko4Operate[21]},
    {PACHINKO4_R0 + 21, 12, Pachinko1Operate, &s_Pachinko4Operate[22]},
    {PACHINKO4_R0 + 22, SF_QUICK_CALL, PachinkoCheckWin, &s_Pachinko4Stand[0]}
};

int
SetupPachinko4(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,PACHINKO4,s_Pachinko4Stand);
        u->Health = 1;
    }

    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_Pachinko4Stand);
    u->Attrib = &Pachinko4Attrib;
    u->StateEnd = s_Pachinko4Stand;
    u->Rot = 0;
    u->RotNum = 0;
    u->ID = PACHINKO4;

    sp->yvel = sp->zvel = 0;
    sp->lotag = PACHINKO4;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);
    RESET(sp->extra, SPRX_PLAYER_OR_ENEMY);

    return 0;
}


////////////////////////////////////////////////////////////////////////////////


ANIMATOR NullCarGirl;

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
ANIMATOR NullCarGirl,DoCarGirl;

STATE s_CarGirlStand[2] =
{
    {CARGIRL_R0 + 0, CARGIRL_RATE, DoCarGirl, &s_CarGirlStand[1]},
    {CARGIRL_R0 + 1, CARGIRL_RATE, DoCarGirl, &s_CarGirlStand[0]}
};

//////////////////////
//
// CARGIRL PAIN
//
//////////////////////

#define CARGIRL_PAIN_RATE 32
#define CARGIRL_PAIN_R0 CARGIRL_R0
ANIMATOR CarGirlPain;

STATE s_CarGirlPain[2] =
{
    {CARGIRL_PAIN_R0 + 0, CARGIRL_PAIN_RATE, CarGirlPain, &s_CarGirlPain[1]},
    {CARGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_CarGirlPain[0]}
};

//////////////////////
//
// CARGIRL UZI
//
//////////////////////

#define CARGIRL_UZI_RATE 8
#define CARGIRL_FIRE_R0 CARGIRL_R0 + 2
ANIMATOR InitEnemyUzi,CarGirlUzi;

STATE s_CarGirlUzi[16] =
{
    {CARGIRL_FIRE_R0 + 0, 240, CarGirlUzi, &s_CarGirlUzi[1]},
    {CARGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_CarGirlUzi[2]},
    {CARGIRL_FIRE_R0 + 1, CARGIRL_UZI_RATE, CarGirlUzi, &s_CarGirlUzi[3]},
    {CARGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_CarGirlUzi[4]},
    {CARGIRL_FIRE_R0 + 0, CARGIRL_UZI_RATE, CarGirlUzi, &s_CarGirlUzi[5]},
    {CARGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_CarGirlUzi[6]},
    {CARGIRL_FIRE_R0 + 1, CARGIRL_UZI_RATE, CarGirlUzi, &s_CarGirlUzi[7]},
    {CARGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_CarGirlUzi[8]},
    {CARGIRL_FIRE_R0 + 0, CARGIRL_UZI_RATE, CarGirlUzi, &s_CarGirlUzi[9]},
    {CARGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_CarGirlUzi[10]},
    {CARGIRL_FIRE_R0 + 1, CARGIRL_UZI_RATE, CarGirlUzi, &s_CarGirlUzi[11]},
    {CARGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_CarGirlUzi[12]},
    {CARGIRL_FIRE_R0 + 0, CARGIRL_UZI_RATE, CarGirlUzi, &s_CarGirlUzi[13]},
    {CARGIRL_FIRE_R0 + 0, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_CarGirlUzi[14]},
    {CARGIRL_FIRE_R0 + 1, CARGIRL_UZI_RATE, CarGirlUzi, &s_CarGirlUzi[15]},
    {CARGIRL_FIRE_R0 + 1, 0 | SF_QUICK_CALL, InitEnemyUzi, &s_CarGirlUzi[0]},
};

int
SetupCarGirl(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,CARGIRL_R0,s_CarGirlStand);
        u->Health = 60;
    }


    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_CarGirlStand);
    u->Attrib = &CarGirlAttrib;
    u->StateEnd = s_CarGirlStand;
    u->Rot = 0;
    u->RotNum = 0;

    sp->xrepeat = 29;
    sp->yrepeat = 25;
    sp->xvel = sp->yvel = sp->zvel = 0;
    sp->lotag = CARGIRL_R0;
    u->FlagOwner = 0;
    u->ID = CARGIRL_R0;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);
    SET(sp->cstat, CSTAT_SPRITE_XFLIP);

    return 0;
}

int DoCarGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short rnd_range = 0;
    static int handle=0;
    SWBOOL ICanSee = FALSE;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,SPRITEp_MID(u->tgt_sp),u->tgt_sp->sectnum);

    if (u->FlagOwner == 1)
    {
        if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
        {
            static int madhandle;

            if (!FX_SoundActive(madhandle))
            {
                short choose;
                choose = RANDOM_RANGE(1000);

                if (choose > 750)
                    madhandle = PlaySound(DIGI_LANI049,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else if (choose > 500)
                    madhandle = PlaySound(DIGI_LANI051,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else if (choose > 250)
                    madhandle = PlaySound(DIGI_LANI052,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else
                    madhandle = PlaySound(DIGI_LANI054,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            }
            ChangeState(SpriteNum,s_CarGirlUzi);
            u->WaitTics = SEC(3)+SEC(RANDOM_RANGE(2<<8)>>8);
            u->FlagOwner = 0;
        }
    }

    //(*u->ActorActionFunc) (SpriteNum);


    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING | SPR_CLIMBING))
    {
        KeepActorOnFloor(SpriteNum);
    }

    // take damage from environment
    DoActorSectorDamage(SpriteNum);
    sp->xvel = sp->yvel = sp->zvel = 0;

    return 0;
}

int NullCarGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SWBOOL ICanSee = FALSE;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,u->tgt_sp->z,u->tgt_sp->sectnum);

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if (u->FlagOwner != 1)
    {
    }
    else if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        static int madhandle;

        if (!FX_SoundActive(madhandle))
        {
            short choose;
            choose = RANDOM_RANGE(1000);

            if (choose > 750)
                madhandle = PlaySound(DIGI_LANI049,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else if (choose > 500)
                madhandle = PlaySound(DIGI_LANI051,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else if (choose > 250)
                madhandle = PlaySound(DIGI_LANI052,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else
                madhandle = PlaySound(DIGI_LANI054,&sp->x,&sp->y,&sp->z,v3df_dontpan);
        }
        ChangeState(SpriteNum,s_CarGirlUzi);
        u->WaitTics = SEC(3)+SEC(RANDOM_RANGE(2<<8)>>8);
        u->FlagOwner = 0;
    }

    return 0;
}

int CarGirlUzi(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
    {
        u->WaitTics = RANDOM_RANGE(240)+120;
        ChangeState(SpriteNum,s_CarGirlStand);
        u->FlagOwner = 0;
    }

    return 0;
}

int CarGirlPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullCarGirl(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(SpriteNum,s_CarGirlStand);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////


ANIMATOR NullMechanicGirl;

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
ANIMATOR NullMechanicGirl,DoMechanicGirl;

STATE s_MechanicGirlStand[2] =
{
    {MECHANICGIRL_R0 + 0, MECHANICGIRL_RATE, DoMechanicGirl, &s_MechanicGirlStand[1]},
    {MECHANICGIRL_R0 + 1, MECHANICGIRL_RATE, DoMechanicGirl, &s_MechanicGirlStand[0]}
};

//////////////////////
//
// MECHANICGIRL PAIN
//
//////////////////////

#define MECHANICGIRL_PAIN_RATE 32
#define MECHANICGIRL_PAIN_R0 MECHANICGIRL_R0
ANIMATOR MechanicGirlPain;

STATE s_MechanicGirlPain[2] =
{
    {MECHANICGIRL_PAIN_R0 + 0, MECHANICGIRL_PAIN_RATE, MechanicGirlPain, &s_MechanicGirlPain[1]},
    {MECHANICGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_MechanicGirlPain[0]}
};

//////////////////////
//
// MECHANICGIRL DRILL
//
//////////////////////

#define MECHANICGIRL_DRILL_RATE 32
#define MECHANICGIRL_DRILL_R0 MECHANICGIRL_R0 + 2
ANIMATOR MechanicGirlDrill;

STATE s_MechanicGirlDrill[2] =
{
    {MECHANICGIRL_DRILL_R0 + 0, MECHANICGIRL_DRILL_RATE, MechanicGirlDrill, &s_MechanicGirlDrill[1]},
    {MECHANICGIRL_DRILL_R0 + 1, MECHANICGIRL_DRILL_RATE, MechanicGirlDrill, &s_MechanicGirlDrill[0]},
};


int
SetupMechanicGirl(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,MECHANICGIRL_R0,s_MechanicGirlStand);
        u->Health = 60;
    }


    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_MechanicGirlStand);
    u->Attrib = &MechanicGirlAttrib;
    u->StateEnd = s_MechanicGirlStand;
    u->Rot = 0;
    u->RotNum = 0;

    sp->xrepeat = 27;
    sp->yrepeat = 26;
    sp->xvel = sp->yvel = sp->zvel = 0;
    sp->lotag = MECHANICGIRL_R0;
    u->FlagOwner = 0;
    u->ID = MECHANICGIRL_R0;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);

    return 0;
}

int DoMechanicGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short rnd_range = 0;
    static int handle=0;
    SWBOOL ICanSee = FALSE;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,SPRITEp_MID(u->tgt_sp),u->tgt_sp->sectnum);

    if (u->FlagOwner == 1)
    {
        if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
        {
            static int madhandle;

            if (!FX_SoundActive(madhandle))
            {
                short choose;
                choose = RANDOM_RANGE(1000);

                if (choose > 750)
                    madhandle = PlaySound(DIGI_LANI073,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else if (choose > 500)
                    madhandle = PlaySound(DIGI_LANI075,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else if (choose > 250)
                    madhandle = PlaySound(DIGI_LANI077,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else
                    madhandle = PlaySound(DIGI_LANI079,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            }
            ChangeState(SpriteNum,s_MechanicGirlDrill);
            u->WaitTics = SEC(1)+SEC(RANDOM_RANGE(2<<8)>>8);
            u->FlagOwner = 0;
        }
    }

    //(*u->ActorActionFunc) (SpriteNum);


    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING | SPR_CLIMBING))
    {
        KeepActorOnFloor(SpriteNum);
    }

    // take damage from environment
    DoActorSectorDamage(SpriteNum);
    sp->xvel = sp->yvel = sp->zvel = 0;

    return 0;
}

int NullMechanicGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SWBOOL ICanSee = FALSE;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,u->tgt_sp->z,u->tgt_sp->sectnum);

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if (u->FlagOwner != 1)
    {
    }
    else if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        static int madhandle;

        if (!FX_SoundActive(madhandle))
        {
            short choose;
            choose = RANDOM_RANGE(1000);

            if (choose > 750)
                madhandle = PlaySound(DIGI_LANI073,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else if (choose > 500)
                madhandle = PlaySound(DIGI_LANI075,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else if (choose > 250)
                madhandle = PlaySound(DIGI_LANI077,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else
                madhandle = PlaySound(DIGI_LANI079,&sp->x,&sp->y,&sp->z,v3df_dontpan);
        }
        ChangeState(SpriteNum,s_MechanicGirlDrill);
        u->WaitTics = SEC(1)+SEC(RANDOM_RANGE(2<<8)>>8);
        u->FlagOwner = 0;
    }

    return 0;
}

int MechanicGirlDrill(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
    {
        u->WaitTics = RANDOM_RANGE(240)+120;
        ChangeState(SpriteNum,s_MechanicGirlStand);
        u->FlagOwner = 0;
    }

    return 0;
}

int MechanicGirlPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullMechanicGirl(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(SpriteNum,s_MechanicGirlStand);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////


ANIMATOR NullSailorGirl;

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
ANIMATOR NullSailorGirl,DoSailorGirl;

STATE s_SailorGirlStand[2] =
{
    {SAILORGIRL_R0 + 0, SAILORGIRL_RATE, DoSailorGirl, &s_SailorGirlStand[1]},
    {SAILORGIRL_R0 + 1, SAILORGIRL_RATE, DoSailorGirl, &s_SailorGirlStand[0]}
};

//////////////////////
//
// SAILORGIRL PAIN
//
//////////////////////

#define SAILORGIRL_PAIN_RATE 32
#define SAILORGIRL_PAIN_R0 SAILORGIRL_R0
ANIMATOR SailorGirlPain;

STATE s_SailorGirlPain[2] =
{
    {SAILORGIRL_PAIN_R0 + 0, SAILORGIRL_PAIN_RATE, SailorGirlPain, &s_SailorGirlPain[1]},
    {SAILORGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_SailorGirlPain[0]}
};

//////////////////////
//
// SAILORGIRL UZI
//
//////////////////////

#define SAILORGIRL_UZI_RATE 128
#define SAILORGIRL_FIRE_R0 SAILORGIRL_R0 + 2
ANIMATOR InitEnemyUzi,SailorGirlThrow;

STATE s_SailorGirlThrow[] =
{
    {SAILORGIRL_FIRE_R0 + 0, SAILORGIRL_UZI_RATE, SailorGirlThrow, &s_SailorGirlThrow[0]},
};

short alreadythrew;

int
SetupSailorGirl(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,SAILORGIRL_R0,s_SailorGirlStand);
        u->Health = 60;
    }


    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_SailorGirlStand);
    u->Attrib = &SailorGirlAttrib;
    u->StateEnd = s_SailorGirlStand;
    u->Rot = 0;
    u->RotNum = 0;

    sp->xrepeat = 28;
    sp->yrepeat = 26;
    sp->xvel = sp->yvel = sp->zvel = 0;
    sp->lotag = SAILORGIRL_R0;
    u->FlagOwner = 0;
    u->ID = SAILORGIRL_R0;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);
    alreadythrew = 0;

    return 0;
}

int DoSailorGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short rnd_range = 0;
    static int handle=0;
    SWBOOL ICanSee = FALSE;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,SPRITEp_MID(u->tgt_sp),u->tgt_sp->sectnum);

    if (u->FlagOwner == 1)
    {
        if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
        {
            static int madhandle;

            if (!FX_SoundActive(madhandle))
            {
                short choose;
                choose = RANDOM_RANGE(1000);

                if (choose > 750 && alreadythrew < 3)
                {
                    ActorCoughItem(SpriteNum);
                    alreadythrew++;
                    madhandle = PlaySound(DIGI_LANI060,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                }
                else if (choose > 500)
                    madhandle = PlaySound(DIGI_LANI063,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else if (choose > 250)
                    madhandle = PlaySound(DIGI_LANI065,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else
                    madhandle = PlaySound(DIGI_LANI066,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            }
            ChangeState(SpriteNum,s_SailorGirlThrow);
            u->WaitTics = SEC(1)+SEC(RANDOM_RANGE(3<<8)>>8);
            u->FlagOwner = 0;
        }
    }

    //(*u->ActorActionFunc) (SpriteNum);


    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING | SPR_CLIMBING))
    {
        KeepActorOnFloor(SpriteNum);
    }

    // take damage from environment
    DoActorSectorDamage(SpriteNum);
    sp->xvel = sp->yvel = sp->zvel = 0;

    return 0;
}

int NullSailorGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SWBOOL ICanSee = FALSE;
    static short alreadythrew = 0;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,u->tgt_sp->z,u->tgt_sp->sectnum);

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if (u->FlagOwner != 1)
    {
    }
    else if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        static int madhandle;

        if (!FX_SoundActive(madhandle))
        {
            short choose;
            choose = RANDOM_RANGE(1000);

            if (choose > 750 && alreadythrew < 3)
            {
                ActorCoughItem(SpriteNum);
                alreadythrew++;
                madhandle = PlaySound(DIGI_LANI060,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            }
            else if (choose > 500)
                madhandle = PlaySound(DIGI_LANI063,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else if (choose > 250)
                madhandle = PlaySound(DIGI_LANI065,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else
                madhandle = PlaySound(DIGI_LANI066,&sp->x,&sp->y,&sp->z,v3df_dontpan);
        }
        ChangeState(SpriteNum,s_SailorGirlThrow);
        u->WaitTics = SEC(1)+SEC(RANDOM_RANGE(3<<8)>>8);
        u->FlagOwner = 0;
    }

    return 0;
}

int SailorGirlThrow(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
    {
        u->WaitTics = RANDOM_RANGE(240)+120;
        ChangeState(SpriteNum,s_SailorGirlStand);
        u->FlagOwner = 0;
    }

    return 0;
}

int SailorGirlPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullSailorGirl(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(SpriteNum,s_SailorGirlStand);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////


ANIMATOR NullPruneGirl;

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
ANIMATOR NullPruneGirl,DoPruneGirl;

STATE s_PruneGirlStand[2] =
{
    {PRUNEGIRL_R0 + 0, PRUNEGIRL_RATE, DoPruneGirl, &s_PruneGirlStand[1]},
    {PRUNEGIRL_R0 + 1, PRUNEGIRL_RATE, DoPruneGirl, &s_PruneGirlStand[0]}
};

//////////////////////
//
// PRUNEGIRL PAIN
//
//////////////////////

#define PRUNEGIRL_PAIN_RATE 32
#define PRUNEGIRL_PAIN_R0 PRUNEGIRL_R0
ANIMATOR PruneGirlPain;

STATE s_PruneGirlPain[2] =
{
    {PRUNEGIRL_PAIN_R0 + 0, PRUNEGIRL_PAIN_RATE, PruneGirlPain, &s_PruneGirlPain[1]},
    {PRUNEGIRL_PAIN_R0 + 0, 0|SF_QUICK_CALL, InitActorDecide, &s_PruneGirlPain[0]}
};

int
SetupPruneGirl(short SpriteNum)
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
        User[SpriteNum] = u = SpawnUser(SpriteNum,PRUNEGIRL_R0,s_PruneGirlStand);
        u->Health = 60;
    }


    EnemyDefaults(SpriteNum, NULL, NULL);

    ChangeState(SpriteNum,s_PruneGirlStand);
    u->Attrib = &PruneGirlAttrib;
    u->StateEnd = s_PruneGirlStand;
    u->Rot = 0;
    u->RotNum = 0;

    sp->xrepeat = 33;
    sp->yrepeat = 28;
    sp->xvel = sp->yvel = sp->zvel = 0;
    sp->lotag = PRUNEGIRL_R0;
    u->FlagOwner = 0;
    u->ID = PRUNEGIRL_R0;

    RESET(u->Flags, SPR_XFLIP_TOGGLE);

    return 0;
}

int DoPruneGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short rnd_range = 0;
    static int madhandle=0,coyhandle=0;
    SWBOOL ICanSee = FALSE;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,SPRITEp_MID(u->tgt_sp),u->tgt_sp->sectnum);

    if (u->FlagOwner == 1)
    {
        if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
        {
            if (!FX_SoundActive(madhandle))
            {
                short choose;
                choose = STD_RANDOM_RANGE(1000);

                if (choose > 750)
                    madhandle = PlaySound(DIGI_LANI089,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else if (choose > 500)
                    madhandle = PlaySound(DIGI_LANI091,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else if (choose > 250)
                    madhandle = PlaySound(DIGI_LANI093,&sp->x,&sp->y,&sp->z,v3df_dontpan);
                else
                    madhandle = PlaySound(DIGI_LANI095,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            }
            u->WaitTics = SEC(1)+SEC(RANDOM_RANGE(3<<8)>>8);
            u->FlagOwner = 0;
        }
    }
    else
    {
        if (!FX_SoundActive(coyhandle))
        {
            short choose;
            choose = STD_RANDOM_RANGE(1000);

            if (choose > 990)
                coyhandle = PlaySound(DIGI_PRUNECACKLE,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else if (choose > 985)
                coyhandle = PlaySound(DIGI_PRUNECACKLE2,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else if (choose > 980)
                coyhandle = PlaySound(DIGI_PRUNECACKLE3,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else if (choose > 975)
                coyhandle = PlaySound(DIGI_LANI091,&sp->x,&sp->y,&sp->z,v3df_dontpan);
        }
    }

    //(*u->ActorActionFunc) (SpriteNum);


    // stay on floor unless doing certain things
    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING | SPR_CLIMBING))
    {
        KeepActorOnFloor(SpriteNum);
    }

    // take damage from environment
    DoActorSectorDamage(SpriteNum);
    sp->xvel = sp->yvel = sp->zvel = 0;

    return 0;
}

int NullPruneGirl(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SWBOOL ICanSee = FALSE;

    DoActorPickClosePlayer(SpriteNum);
    ICanSee = FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,u->tgt_sp->x,u->tgt_sp->y,u->tgt_sp->z,u->tgt_sp->sectnum);

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if (u->FlagOwner != 1)
    {
    }
    else if ((u->WaitTics -= ACTORMOVETICS) <= 0 && ICanSee)
    {
        static int madhandle;

        if (!FX_SoundActive(madhandle))
        {
            short choose;
            choose = RANDOM_RANGE(1000);

            if (choose > 750)
                madhandle = PlaySound(DIGI_LANI089,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else if (choose > 500)
                madhandle = PlaySound(DIGI_LANI091,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else if (choose > 250)
                madhandle = PlaySound(DIGI_LANI093,&sp->x,&sp->y,&sp->z,v3df_dontpan);
            else
                madhandle = PlaySound(DIGI_LANI095,&sp->x,&sp->y,&sp->z,v3df_dontpan);
        }
        u->WaitTics = SEC(1)+SEC(RANDOM_RANGE(3<<8)>>8);
        u->FlagOwner = 0;
    }

    return 0;
}

int PruneGirlUzi(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (!TEST(u->Flags,SPR_CLIMBING))
        KeepActorOnFloor(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
    {
        u->WaitTics = RANDOM_RANGE(240)+120;
        ChangeState(SpriteNum,s_PruneGirlStand);
        u->FlagOwner = 0;
    }

    return 0;
}

int PruneGirlPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    NullPruneGirl(SpriteNum);

    if ((u->WaitTics -= ACTORMOVETICS) <= 0)
        ChangeState(SpriteNum,s_PruneGirlStand);

    return 0;
}


#include "saveable.h"

static saveable_code saveable_miscactr_code[] =
{
    SAVE_CODE(SetupToiletGirl),
    SAVE_CODE(DoToiletGirl),
    SAVE_CODE(NullToiletGirl),
    SAVE_CODE(ToiletGirlUzi),
    SAVE_CODE(ToiletGirlPain),

    SAVE_CODE(SetupWashGirl),
    SAVE_CODE(DoWashGirl),
    SAVE_CODE(NullWashGirl),
    SAVE_CODE(WashGirlUzi),
    SAVE_CODE(WashGirlPain),

    SAVE_CODE(SetupTrashCan),
    SAVE_CODE(DoTrashCan),
    SAVE_CODE(TrashCanPain),

    SAVE_CODE(SetupPachinkoLight),
    SAVE_CODE(PachinkoLightOperate),

    SAVE_CODE(SetupPachinko1),
    SAVE_CODE(SetupPachinko2),
    SAVE_CODE(SetupPachinko3),
    SAVE_CODE(SetupPachinko4),
    SAVE_CODE(PachinkoCheckWin),
    SAVE_CODE(Pachinko1Operate),

    SAVE_CODE(SetupCarGirl),
    SAVE_CODE(DoCarGirl),
    SAVE_CODE(NullCarGirl),
    SAVE_CODE(CarGirlUzi),
    SAVE_CODE(CarGirlPain),

    SAVE_CODE(SetupMechanicGirl),
    SAVE_CODE(DoMechanicGirl),
    SAVE_CODE(NullMechanicGirl),
    SAVE_CODE(MechanicGirlDrill),
    SAVE_CODE(MechanicGirlPain),

    SAVE_CODE(SetupSailorGirl),
    SAVE_CODE(DoSailorGirl),
    SAVE_CODE(NullSailorGirl),
    SAVE_CODE(SailorGirlThrow),
    SAVE_CODE(SailorGirlPain),

    SAVE_CODE(SetupPruneGirl),
    SAVE_CODE(DoPruneGirl),
    SAVE_CODE(NullPruneGirl),
    SAVE_CODE(PruneGirlUzi),
    SAVE_CODE(PruneGirlPain),
};

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
    saveable_miscactr_code,
    SIZ(saveable_miscactr_code),

    // data
    saveable_miscactr_data,
    SIZ(saveable_miscactr_data)
};
