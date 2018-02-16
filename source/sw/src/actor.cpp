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
#include "weapon.h"
#include "sprite.h"
#include "actor.h"

extern int jump_grav;

extern STATE s_DebrisNinja[];
extern STATE s_DebrisRat[];
extern STATE s_DebrisCrab[];
extern STATE s_DebrisStarFish[];
extern STATE s_NinjaDieSliced[];

extern STATEp sg_NinjaGrabThroat[];

int DoActorStopFall(short SpriteNum);


int
DoScaleSprite(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int scale_value;

    if (u->scale_speed)
    {
        u->scale_value += u->scale_speed * ACTORMOVETICS;

        scale_value = u->scale_value >> 8;

        if (u->scale_speed > 0)
        {
            if (scale_value > u->scale_tgt)
                u->scale_speed = 0;
            else
                sp->xrepeat = sp->yrepeat = scale_value;
        }
        else
        {
            if (scale_value < u->scale_tgt)
                u->scale_speed = 0;
            else
                sp->xrepeat = sp->yrepeat = scale_value;
        }

    }

    return 0;
}

int
DoActorDie(short SpriteNum, short weapon)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = &sprite[SpriteNum];


    change_sprite_stat(SpriteNum, STAT_DEAD_ACTOR);
    SET(u->Flags, SPR_DEAD);
    RESET(u->Flags, SPR_FALLING | SPR_JUMPING);
    u->floor_dist = Z(40);

    // test for gibable dead bodies
    SET(sp->extra, SPRX_BREAKABLE);
    SET(sp->cstat, CSTAT_SPRITE_BREAKABLE);

    if (weapon < 0)
    {
        // killed by one of these non-sprites
        switch (weapon)
        {
        case WPN_NM_LAVA:
            ChangeState(SpriteNum, u->StateEnd);
            u->RotNum = 0;
            break;

        case WPN_NM_SECTOR_SQUISH:
            ChangeState(SpriteNum, u->StateEnd);
            u->RotNum = 0;
            break;
        }

        return 0;
    }


    // killed by one of these sprites
    switch (User[weapon]->ID)
    {
    // Coolie actually explodes himself
    // he is the Sprite AND Weapon
    case COOLIE_RUN_R0:
        ChangeState(SpriteNum, u->StateEnd);
        u->RotNum = 0;
        sp->xvel <<= 1;
        u->ActorActionFunc = NULL;
        sprite[SpriteNum].ang = NORM_ANGLE(sprite[SpriteNum].ang + 1024);
        break;

    case NINJA_RUN_R0:
        if (u->ID == NINJA_RUN_R0) // Cut in half!
        {
            SPRITEp wp = &sprite[weapon];

            if (User[weapon]->WeaponNum != WPN_FIST)
            {
                //SpawnBlood(SpriteNum, SpriteNum, -1, -1, -1, -1);
                InitPlasmaFountain(wp, sp);
                InitPlasmaFountain(wp, sp);
                PlaySound(DIGI_NINJAINHALF,&sp->x,&sp->y,&sp->z,v3df_none);
                ChangeState(SpriteNum, &s_NinjaDieSliced[0]);
            }
            else
            {
                if (RANDOM_RANGE(1000) > 500)
                {
                    SPRITEp wp = &sprite[weapon];
                    InitPlasmaFountain(wp, sp);
                }

                ChangeState(SpriteNum, u->StateEnd);
                u->RotNum = 0;
                u->ActorActionFunc = NULL;
                sp->xvel = 200 + RANDOM_RANGE(200);
                u->jump_speed = -200 - RANDOM_RANGE(250);
                DoActorBeginJump(SpriteNum);
                sprite[SpriteNum].ang = sprite[weapon].ang;
            }
        }
        else
        {
            // test for gibable dead bodies
            if (RANDOM_RANGE(1000) > 500)
                SET(sp->cstat, CSTAT_SPRITE_YFLIP);
            ChangeState(SpriteNum, u->StateEnd);
            sp->xvel = 0;
            u->jump_speed = 0;
            DoActorBeginJump(SpriteNum);
        }

        u->RotNum = 0;

        u->ActorActionFunc = NULL;
        //u->ActorActionFunc = NullAnimator;
        sprite[SpriteNum].ang = sprite[weapon].ang;
        break;

    case COOLG_RUN_R0:
    case SKEL_RUN_R0:
    case RIPPER_RUN_R0:
    case RIPPER2_RUN_R0:
    case EEL_RUN_R0:
    case STAR1:
    case SUMO_RUN_R0:
        ChangeState(SpriteNum, u->StateEnd);
        u->RotNum = 0;
        break;

    case UZI_SMOKE:
        if (RANDOM_RANGE(1000) > 500)
            SET(sp->cstat, CSTAT_SPRITE_YFLIP);
        ChangeState(SpriteNum, u->StateEnd);
        u->RotNum = 0;
        // Rippers still gotta jump or they fall off walls weird
        if (u->ID == RIPPER_RUN_R0 || u->ID == RIPPER2_RUN_R0)
        {
            sp->xvel <<= 1;
            u->jump_speed = -100 - RANDOM_RANGE(250);
            DoActorBeginJump(SpriteNum);
        }
        else
        {
            sp->xvel = 0;
            u->jump_speed = -10 - RANDOM_RANGE(25);
            DoActorBeginJump(SpriteNum);
        }
        u->ActorActionFunc = NULL;
        // Get angle to player
        sp->ang = NORM_ANGLE(getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y) + 1024);
        break;

    case UZI_SMOKE+1: // Shotgun
        if (RANDOM_RANGE(1000) > 500)
            SET(sp->cstat, CSTAT_SPRITE_YFLIP);
        ChangeState(SpriteNum, u->StateEnd);
        u->RotNum = 0;

        // Rippers still gotta jump or they fall off walls weird
        if (u->ID == RIPPER_RUN_R0 || u->ID == RIPPER2_RUN_R0)
        {
            sp->xvel = 75 + RANDOM_RANGE(100);
            u->jump_speed = -100 - RANDOM_RANGE(150);
        }
        else
        {
            sp->xvel = 100 + RANDOM_RANGE(200);
            u->jump_speed = -100 - RANDOM_RANGE(250);
        }
        DoActorBeginJump(SpriteNum);
        u->ActorActionFunc = NULL;
        // Get angle to player
        sp->ang = NORM_ANGLE(getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y) + 1024);
        break;

    default:
        ASSERT(weapon >= 0);
        switch (u->ID)
        {
        case SKULL_R0:
        case BETTY_R0:
            ChangeState(SpriteNum, u->StateEnd);
            break;

        default:
            if (RANDOM_RANGE(1000) > 700)
            {
                SPRITEp wp = &sprite[weapon];
                InitPlasmaFountain(wp, sp);
            }

            if (RANDOM_RANGE(1000) > 500)
                SET(sp->cstat, CSTAT_SPRITE_YFLIP);
            ChangeState(SpriteNum, u->StateEnd);
            u->RotNum = 0;
            u->ActorActionFunc = NULL;
            sp->xvel = 300 + RANDOM_RANGE(400);
            u->jump_speed = -300 - RANDOM_RANGE(350);
            DoActorBeginJump(SpriteNum);
            sprite[SpriteNum].ang = sprite[weapon].ang;
            break;
        }
        break;
    }

    // These are too big to flip upside down
    switch (u->ID)
    {
    case RIPPER2_RUN_R0:
    case COOLIE_RUN_R0:
    case SUMO_RUN_R0:
    case ZILLA_RUN_R0:
        RESET(sp->cstat, CSTAT_SPRITE_YFLIP);
        break;
    }

    u->ID = 0;

    return 0;
}

void
DoDebrisCurrent(SPRITEp sp)
{
    int xvect, yvect;
    int nx, ny;
    int ret=0;
    USERp u = User[sp - sprite];
    SECT_USERp sectu = SectUser[sp->sectnum];

    //sp->clipdist = (256+128)>>2;

    nx = DIV4(sectu->speed) * (int) sintable[NORM_ANGLE(sectu->ang + 512)] >> 14;
    ny = DIV4(sectu->speed) * (int) sintable[sectu->ang] >> 14;

    // faster than move_sprite
    //move_missile(sp-sprite, nx, ny, 0, Z(2), Z(0), 0, ACTORMOVETICS);
    ret = move_sprite(sp-sprite, nx, ny, 0, u->ceiling_dist, u->floor_dist, 0, ACTORMOVETICS);

    // attempt to move away from wall
    if (ret)
    {
        short rang = RANDOM_P2(2048);

        nx = DIV4(sectu->speed) * (int) sintable[NORM_ANGLE(sectu->ang + rang + 512)] >> 14;
        ny = DIV4(sectu->speed) * (int) sintable[NORM_ANGLE(sectu->ang + rang)] >> 14;

        move_sprite(sp-sprite, nx, ny, 0, u->ceiling_dist, u->floor_dist, 0, ACTORMOVETICS);
    }

    sp->z = u->loz;
}

int
DoActorSectorDamage(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SECT_USERp sectu = SectUser[sp->sectnum];
    SECTORp sectp = &sector[sp->sectnum];

    if (u->Health <= 0)
        return FALSE;

    if (sectu && sectu->damage)
    {
        if (TEST(sectu->flags, SECTFU_DAMAGE_ABOVE_SECTOR))
        {
            if ((u->DamageTics -= synctics) < 0)
            {
                u->DamageTics = 60;
                u->Health -= sectu->damage;

                if (u->Health <= 0)
                {
                    UpdateSinglePlayKills(SpriteNum);
                    DoActorDie(SpriteNum, WPN_NM_LAVA);
                    return TRUE;
                }
            }
        }
        else if (SPRITEp_BOS(sp) >= sectp->floorz)
        {
            if ((u->DamageTics -= synctics) < 0)
            {
                u->DamageTics = 60;
                u->Health -= sectu->damage;

                if (u->Health <= 0)
                {
                    UpdateSinglePlayKills(SpriteNum);
                    DoActorDie(SpriteNum, WPN_NM_LAVA);
                    return TRUE;
                }
            }
        }
    }

    // note that most squishing is done in vator.c
    if (u->lo_sectp && u->hi_sectp && labs(u->loz - u->hiz) < DIV2(SPRITEp_SIZE_Z(sp)))
    //if (u->lo_sectp && u->hi_sectp && labs(u->loz - u->hiz) < SPRITEp_SIZE_Z(sp))
    {
        u->Health = 0;
        if (SpawnShrap(SpriteNum, WPN_NM_SECTOR_SQUISH))
        {
            UpdateSinglePlayKills(SpriteNum);
            SetSuicide(SpriteNum);
        }
        else
        {
            ASSERT(TRUE == FALSE);
            //DoActorDie(SpriteNum, WPN_NM_SECTOR_SQUISH);
        }

        return TRUE;
    }

    return FALSE;
}


int
move_debris(short SpriteNum, int xchange, int ychange, int zchange)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int nx, ny;

    u->ret = move_sprite(SpriteNum, xchange, ychange, zchange,
                         u->ceiling_dist, u->floor_dist, 0, ACTORMOVETICS);

    return !u->ret;
}

// !AIC - Supposed to allow floating of DEBRIS (dead bodies, flotsam, jetsam).  Or if water has
// current move with the current.

int
DoActorDebris(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SECTORp sectp = &sector[sp->sectnum];
    int nx, ny;

    // This was move from DoActorDie so actor's can't be walked through until they are on the floor
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // Don't let some actors float
    switch (u->ID)
    {
    case HORNET_RUN_R0:
    case BUNNY_RUN_R0:
        KillSprite(SpriteNum);
        return 0;
    case ZILLA_RUN_R0:
        getzsofslope(sp->sectnum, sp->x, sp->y, &u->hiz, &u->loz);
        u->lo_sectp = &sector[sp->sectnum];
        u->hi_sectp = &sector[sp->sectnum];
        u->lo_sp = NULL;
        u->hi_sp = NULL;
        break;
    }

    if (TEST(sectp->extra, SECTFX_SINK))
    {
        if (TEST(sectp->extra, SECTFX_CURRENT))
        {
            DoDebrisCurrent(sp);
        }
        else
        {
            //nx = sp->xvel * ACTORMOVETICS * (int) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
            //ny = sp->xvel * ACTORMOVETICS * (int) sintable[sp->ang] >> 14;
            nx = ACTORMOVETICS * (int) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
            ny = ACTORMOVETICS * (int) sintable[sp->ang] >> 14;

            //sp->clipdist = (256+128)>>2;

            if (!move_debris(SpriteNum, nx, ny, 0L))
            {
                sp->ang = RANDOM_P2(2048);
            }
        }

        if (SectUser[sp->sectnum] && SectUser[sp->sectnum]->depth > 10) // JBF: added null check
        {
            u->WaitTics = (u->WaitTics + (ACTORMOVETICS << 3)) & 1023;
            //sp->z = Z(2) + u->loz + ((Z(4) * (int) sintable[u->WaitTics]) >> 14);
            sp->z = u->loz - ((Z(2) * (int) sintable[u->WaitTics]) >> 14);
        }
    }
    else
    {
        sp->z = u->loz;
    }

    return 0;
}


int
DoFireFly(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int nx, ny;

    nx = 4 * ACTORMOVETICS * (int) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
    ny = 4 * ACTORMOVETICS * (int) sintable[sp->ang] >> 14;

    sp->clipdist = 256>>2;
    if (!move_actor(SpriteNum, nx, ny, 0L))
    {
        sp->ang = NORM_ANGLE(sp->ang + 1024);
    }

    u->WaitTics = (u->WaitTics + (ACTORMOVETICS << 1)) & 2047;

    sp->z = u->sz + ((Z(32) * (int) sintable[u->WaitTics]) >> 14);
    return 0;
}

int
DoGenerateSewerDebris(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], np;
    USERp u = User[SpriteNum], nu;
    short n;

    static STATEp Debris[] =
    {
        s_DebrisNinja,
        s_DebrisRat,
        s_DebrisCrab,
        s_DebrisStarFish
    };

    u->Tics -= ACTORMOVETICS;

    if (u->Tics <= 0)
    {
        u->Tics = u->WaitTics;

        n = SpawnSprite(STAT_DEAD_ACTOR, 0, Debris[RANDOM_P2(4<<8)>>8], sp->sectnum, sp->x, sp->y, sp->z, sp->ang, 200);
        np = &sprite[n];
        nu = User[n];

        SetOwner(SpriteNum, n);
    }

    return 0;
}

// !AIC - Tries to keep actors correctly on the floor.  More that a bit messy.

void
KeepActorOnFloor(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SECTORp sectp;
    int depth;

    sectp = &sector[sp->sectnum];

    RESET(sp->cstat, CSTAT_SPRITE_YFLIP); // If upside down, reset it

    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
        return;

    if (u->lo_sectp && SectUser[u->lo_sectp - sector])
        depth = SectUser[u->lo_sectp - sector]->depth;
    else
        depth = 0;

    if (TEST(sectp->extra, SECTFX_SINK) &&
        depth > 35 &&
        u->ActorActionSet && u->ActorActionSet->Swim)
    {
        if (TEST(u->Flags, SPR_SWIMMING))
        {
            if (u->Rot != u->ActorActionSet->Run && u->Rot != u->ActorActionSet->Swim && u->Rot != u->ActorActionSet->Stand)
            {
                // was swimming but have now stopped
                RESET(u->Flags, SPR_SWIMMING);
                RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
                u->oz = sp->z = u->loz;
                return;
            }

            if (u->Rot == u->ActorActionSet->Run)
            {
                NewStateGroup(SpriteNum, u->ActorActionSet->Swim);
            }

            // are swimming
            u->oz = sp->z = u->loz - Z(depth);
        }
        else
        {
            // only start swimming if you are running
            if (u->Rot == u->ActorActionSet->Run || u->Rot == u->ActorActionSet->Swim)
            {
                NewStateGroup(SpriteNum, u->ActorActionSet->Swim);
                u->oz = sp->z = u->loz - Z(depth);
                SET(u->Flags, SPR_SWIMMING);
                SET(sp->cstat, CSTAT_SPRITE_YCENTER);
            }
            else
            {
                RESET(u->Flags, SPR_SWIMMING);
                RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
                u->oz = sp->z = u->loz;
            }
        }

        return;
    }

    // NOT in a swimming situation
    RESET(u->Flags, SPR_SWIMMING);
    RESET(sp->cstat, CSTAT_SPRITE_YCENTER);

#if 1
    if (TEST(u->Flags, SPR_MOVED))
    {
        u->oz = sp->z = u->loz;
    }
    else
    {
        int ceilz,ceilhit,florz,florhit;
        FAFgetzrangepoint(sp->x, sp->y, sp->z, sp->sectnum,
                          &ceilz, &ceilhit, &florz, &florhit);

        u->oz = sp->z = florz;
    }
#endif


}

int
DoActorBeginSlide(short SpriteNum, short ang, short vel, short dec)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    SET(u->Flags, SPR_SLIDING);

    u->slide_ang = ang;
    u->slide_vel = vel;
    u->slide_dec = dec;

    //DoActorSlide(SpriteNum);

    return 0;
}

// !AIC - Sliding can occur in different directions from movement of the actor.
// Has its own set of variables

int
DoActorSlide(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    int nx, ny;

    nx = u->slide_vel * (int) sintable[NORM_ANGLE(u->slide_ang + 512)] >> 14;
    ny = u->slide_vel * (int) sintable[u->slide_ang] >> 14;

    if (!move_actor(SpriteNum, nx, ny, 0L))
    {
        RESET(u->Flags, SPR_SLIDING);
        return FALSE;
    }

    u->slide_vel -= u->slide_dec * ACTORMOVETICS;

    if (u->slide_vel < 20)
    {
        RESET(u->Flags, SPR_SLIDING);
    }

    return TRUE;
}

// !AIC - Actor jumping and falling

int
DoActorBeginJump(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    SET(u->Flags, SPR_JUMPING);
    RESET(u->Flags, SPR_FALLING);

    // u->jump_speed = should be set before calling

    // set up individual actor jump gravity
    u->jump_grav = ACTOR_GRAVITY;

    // Change sprites state to jumping
    if (u->ActorActionSet)
    {
        if (TEST(u->Flags, SPR_DEAD))
            NewStateGroup(SpriteNum, u->ActorActionSet->DeathJump);
        else
            NewStateGroup(SpriteNum, u->ActorActionSet->Jump);
    }
    u->StateFallOverride = NULL;

    //DO NOT CALL DoActorJump! DoActorStopFall can cause an infinite loop and
    //stack overflow if it is called.
    //DoActorJump(SpriteNum);

    return 0;
}

int
DoActorJump(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    int jump_adj;

    // precalculate jump value to adjust jump speed by
    jump_adj = u->jump_grav * ACTORMOVETICS;

    // adjust jump speed by gravity - if jump speed greater than 0 player
    // have started falling
    if ((u->jump_speed += jump_adj) > 0)
    {
        //DSPRINTF(ds,"Actor Jump Height %d", labs(sp->z - sector[sp->sectnum].floorz)>>8 );
        MONO_PRINT(ds);

        // Start falling
        DoActorBeginFall(SpriteNum);
        return 0;
    }

    // adjust height by jump speed
    sp->z += u->jump_speed * ACTORMOVETICS;

    // if player gets to close the ceiling while jumping
    if (sp->z < u->hiz + Z(PIC_SIZY(SpriteNum)))
    {
        // put player at the ceiling
        sp->z = u->hiz + Z(PIC_SIZY(SpriteNum));

        // reverse your speed to falling
        u->jump_speed = -u->jump_speed;

        //DSPRINTF(ds,"Jump: sp_num %d, hi_num %d, hi_sect %d",SpriteNum, u->hi_sp - sprite, u->hi_sectp - sector);
        MONO_PRINT(ds);

        // Change sprites state to falling
        DoActorBeginFall(SpriteNum);
    }

    return 0;
}


int
DoActorBeginFall(short SpriteNum)
{
    USERp u = User[SpriteNum];

    SET(u->Flags, SPR_FALLING);
    RESET(u->Flags, SPR_JUMPING);

    u->jump_grav = ACTOR_GRAVITY;

    // Change sprites state to falling
    if (u->ActorActionSet)
    {
        if (TEST(u->Flags, SPR_DEAD))
        {
            NewStateGroup(SpriteNum, u->ActorActionSet->DeathFall);
        }
        else
            NewStateGroup(SpriteNum, u->ActorActionSet->Fall);

        if (u->StateFallOverride)
        {
            NewStateGroup(SpriteNum, u->StateFallOverride);
        }
    }

    DoActorFall(SpriteNum);

    return 0;
}


int
DoActorFall(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    // adjust jump speed by gravity
    u->jump_speed += u->jump_grav * ACTORMOVETICS;

    // adjust player height by jump speed
    sp->z += u->jump_speed * ACTORMOVETICS;

    // Stick like glue when you hit the ground
    if (sp->z > u->loz)
    {
        DoActorStopFall(SpriteNum);
    }

    return 0;
}

int
DoActorStopFall(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    sp->z = u->loz;

    RESET(u->Flags, SPR_FALLING | SPR_JUMPING);
    RESET(sp->cstat, CSTAT_SPRITE_YFLIP);


    // don't stand on face or wall sprites - jump again
    if (u->lo_sp && !TEST(u->lo_sp->cstat, CSTAT_SPRITE_ALIGNMENT_FLOOR))
    {
        USERp tu = User[u->lo_sp - sprite];

        //sp->ang = NORM_ANGLE(sp->ang + (RANDOM_P2(64<<8)>>8) - 32);
        sp->ang = NORM_ANGLE(sp->ang + 1024 + (RANDOM_P2(512<<8)>>8));
        u->jump_speed = -350;

        //DSPRINTF(ds,"StopFall: sp_num %d, sp->picnum %d, lo_num %d, lo_sp->picnum %d",SpriteNum, sp->picnum, u->lo_sp - sprite, u->lo_sp->picnum);
        MONO_PRINT(ds);

        DoActorBeginJump(SpriteNum);
        return 0;
    }

    // Change sprites state to running
    if (u->ActorActionSet)
    {
        if (TEST(u->Flags, SPR_DEAD))
        {
            NewStateGroup(SpriteNum, u->ActorActionSet->Dead);
            PlaySound(DIGI_ACTORBODYFALL1,&sp->x,&sp->y,&sp->z,v3df_none);
        }
        else
        {
            PlaySound(DIGI_ACTORHITGROUND,&sp->x,&sp->y,&sp->z,v3df_none);

            NewStateGroup(SpriteNum, u->ActorActionSet->Run);

            if ((u->track >= 0) && (u->jump_speed) > 800 && (u->ActorActionSet->Sit))
            {
                u->WaitTics = 80;
                NewStateGroup(SpriteNum, u->ActorActionSet->Sit);
            }
        }
    }

    return 0;
}

int
DoActorDeathMove(short SpriteNum)
{
    ANIMATOR DoFindGround;

    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    int nx, ny;

    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (TEST(u->Flags, SPR_JUMPING))
            DoActorJump(SpriteNum);
        else
            DoActorFall(SpriteNum);
    }

    nx = sp->xvel * (int) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
    ny = sp->xvel * (int) sintable[sp->ang] >> 14;

    sp->clipdist = (128+64)>>2;
    move_actor(SpriteNum, nx, ny, 0);

    // only fall on top of floor sprite or sector
    DoFindGroundPoint(SpriteNum);

    return 0;
}

// !AIC - Jumping a falling for shrapnel and other stuff, not actors.

int
DoBeginJump(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    SET(u->Flags, SPR_JUMPING);
    RESET(u->Flags, SPR_FALLING);

    // set up individual actor jump gravity
    u->jump_grav = ACTOR_GRAVITY;

    DoJump(SpriteNum);

    return 0;
}

int
DoJump(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    int jump_adj;

    // precalculate jump value to adjust jump speed by
    jump_adj = u->jump_grav * ACTORMOVETICS;

    // adjust jump speed by gravity - if jump speed greater than 0 player
    // have started falling
    if ((u->jump_speed += jump_adj) > 0)
    {
        // Start falling
        DoBeginFall(SpriteNum);
        return 0;
    }

    // adjust height by jump speed
    sp->z += u->jump_speed * ACTORMOVETICS;

    // if player gets to close the ceiling while jumping
    if (sp->z < u->hiz + Z(PIC_SIZY(SpriteNum)))
    {
        // put player at the ceiling
        sp->z = u->hiz + Z(PIC_SIZY(SpriteNum));

        // reverse your speed to falling
        u->jump_speed = -u->jump_speed;

        // Change sprites state to falling
        DoBeginFall(SpriteNum);
    }

    return 0;
}


int
DoBeginFall(short SpriteNum)
{
    USERp u = User[SpriteNum];

    SET(u->Flags, SPR_FALLING);
    RESET(u->Flags, SPR_JUMPING);

    u->jump_grav = ACTOR_GRAVITY;

    DoFall(SpriteNum);

    return 0;
}

#if 0
int
DoFall(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    // adjust jump speed by gravity
    u->jump_speed += u->jump_grav * ACTORMOVETICS;

    // adjust player height by jump speed
    sp->z += u->jump_speed * ACTORMOVETICS;

    // Stick like glue when you hit the ground
    if (sp->z > u->loz)
    {
        sp->z = u->loz;
        RESET(u->Flags, SPR_FALLING);
    }

    return 0;
}
#else
int
DoFall(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    // adjust jump speed by gravity
    u->jump_speed += u->jump_grav * ACTORMOVETICS;

    // adjust player height by jump speed
    sp->z += u->jump_speed * ACTORMOVETICS;

    // Stick like glue when you hit the ground
    if (sp->z > u->loz - u->floor_dist)
    {
        sp->z = u->loz - u->floor_dist;
        RESET(u->Flags, SPR_FALLING);
    }

    return 0;
}
#endif


#include "saveable.h"

static saveable_code saveable_actor_code[] =
{
    SAVE_CODE(DoScaleSprite),
    SAVE_CODE(DoActorDie),
    SAVE_CODE(DoDebrisCurrent),
    SAVE_CODE(DoActorSectorDamage),
    SAVE_CODE(DoActorDebris),
    SAVE_CODE(DoFireFly),
    SAVE_CODE(DoGenerateSewerDebris),
    SAVE_CODE(KeepActorOnFloor),
    SAVE_CODE(DoActorBeginSlide),
    SAVE_CODE(DoActorSlide),
    SAVE_CODE(DoActorBeginJump),
    SAVE_CODE(DoActorJump),
    SAVE_CODE(DoActorBeginFall),
    SAVE_CODE(DoActorFall),
    SAVE_CODE(DoActorStopFall),
    SAVE_CODE(DoActorDeathMove),
    SAVE_CODE(DoBeginJump),
    SAVE_CODE(DoJump),
    SAVE_CODE(DoBeginFall),
    SAVE_CODE(DoFall)
};

saveable_module saveable_actor =
{
    // code
    saveable_actor_code,
    SIZ(saveable_actor_code),

    // data
    NULL,0
};
