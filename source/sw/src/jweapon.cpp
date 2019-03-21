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
#include "common.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "common_game.h"
#include "break.h"
#include "quake.h"
#include "network.h"
#include "pal.h"

#include "ai.h"
#include "weapon.h"

#include "sprite.h"
#include "sector.h"
#include "actor.h"

ANIMATOR NullAnimator,DoSuicide;
ANIMATOR DoBloodSpray;
int SpawnFlashBombOnActor(int16_t enemy);

ANIMATOR DoPuff, BloodSprayFall;
extern STATE s_Puff[];
extern STATE s_FireballFlames[];
extern STATE s_GoreFloorSplash[];
extern STATE s_GoreSplash[];
extern SWBOOL GlobalSkipZrange;

#define CHEMTICS SEC(40)

#define GOREDrip 1562 //2430
#define BLOODSPRAY_RATE 20

STATE s_BloodSpray[] =
{
    {GOREDrip + 0, BLOODSPRAY_RATE, BloodSprayFall, &s_BloodSpray[1]},
    {GOREDrip + 1, BLOODSPRAY_RATE, BloodSprayFall, &s_BloodSpray[2]},
    {GOREDrip + 2, BLOODSPRAY_RATE, BloodSprayFall, &s_BloodSpray[3]},
    {GOREDrip + 3, BLOODSPRAY_RATE, BloodSprayFall, &s_BloodSpray[4]},
    {GOREDrip + 3, 100, DoSuicide, &s_BloodSpray[0]}
};


#define EXP_RATE 2
STATE s_PhosphorExp[] =
{
    {EXP + 0, EXP_RATE, NullAnimator, &s_PhosphorExp[1]},
    {EXP + 1, EXP_RATE, NullAnimator, &s_PhosphorExp[2]},
    {EXP + 2, EXP_RATE, NullAnimator, &s_PhosphorExp[3]},
    {EXP + 3, EXP_RATE, NullAnimator, &s_PhosphorExp[4]},
    {EXP + 4, EXP_RATE, NullAnimator, &s_PhosphorExp[5]},
    {EXP + 5, EXP_RATE, NullAnimator, &s_PhosphorExp[6]},
    {EXP + 6, EXP_RATE, NullAnimator, &s_PhosphorExp[7]},
    {EXP + 7, EXP_RATE, NullAnimator, &s_PhosphorExp[8]},
    {EXP + 8, EXP_RATE, NullAnimator, &s_PhosphorExp[9]},
    {EXP + 9, EXP_RATE, NullAnimator, &s_PhosphorExp[10]},
    {EXP + 10, EXP_RATE, NullAnimator, &s_PhosphorExp[11]},
    {EXP + 11, EXP_RATE, NullAnimator, &s_PhosphorExp[12]},
    {EXP + 12, EXP_RATE, NullAnimator, &s_PhosphorExp[13]},
    {EXP + 13, EXP_RATE, NullAnimator, &s_PhosphorExp[14]},
    {EXP + 14, EXP_RATE, NullAnimator, &s_PhosphorExp[15]},
    {EXP + 15, EXP_RATE, NullAnimator, &s_PhosphorExp[16]},
    {EXP + 16, EXP_RATE, NullAnimator, &s_PhosphorExp[17]},
    {EXP + 17, EXP_RATE, NullAnimator, &s_PhosphorExp[18]},
    {EXP + 18, EXP_RATE, NullAnimator, &s_PhosphorExp[19]},
    {EXP + 19, EXP_RATE, NullAnimator, &s_PhosphorExp[20]},
    {EXP + 20, 100, DoSuicide, &s_PhosphorExp[0]}
};

#define MUSHROOM_RATE 25

STATE s_NukeMushroom[] =
{
    {MUSHROOM_CLOUD + 0, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[1]},
    {MUSHROOM_CLOUD + 1, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[2]},
    {MUSHROOM_CLOUD + 2, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[3]},
    {MUSHROOM_CLOUD + 3, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[4]},
    {MUSHROOM_CLOUD + 4, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[5]},
    {MUSHROOM_CLOUD + 5, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[6]},
    {MUSHROOM_CLOUD + 6, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[7]},
    {MUSHROOM_CLOUD + 7, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[8]},
    {MUSHROOM_CLOUD + 8, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[9]},
    {MUSHROOM_CLOUD + 9, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[10]},
    {MUSHROOM_CLOUD + 10, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[11]},
    {MUSHROOM_CLOUD + 11, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[12]},
    {MUSHROOM_CLOUD + 12, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[13]},
    {MUSHROOM_CLOUD + 13, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[14]},
    {MUSHROOM_CLOUD + 14, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[15]},
    {MUSHROOM_CLOUD + 15, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[16]},
    {MUSHROOM_CLOUD + 16, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[17]},
    {MUSHROOM_CLOUD + 17, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[18]},
    {MUSHROOM_CLOUD + 18, MUSHROOM_RATE, NullAnimator, &s_NukeMushroom[19]},
    {MUSHROOM_CLOUD + 19, 100, DoSuicide, &s_NukeMushroom[0]},
};

ANIMATOR DoRadiationCloud;

#define RADIATION_RATE 16

STATE s_RadiationCloud[] =
{
    {RADIATION_CLOUD + 0, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[1]},
    {RADIATION_CLOUD + 1, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[2]},
    {RADIATION_CLOUD + 2, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[3]},
    {RADIATION_CLOUD + 3, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[4]},
    {RADIATION_CLOUD + 4, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[5]},
    {RADIATION_CLOUD + 5, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[6]},
    {RADIATION_CLOUD + 6, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[7]},
    {RADIATION_CLOUD + 7, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[8]},
    {RADIATION_CLOUD + 8, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[9]},
    {RADIATION_CLOUD + 9, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[10]},
    {RADIATION_CLOUD + 10, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[11]},
    {RADIATION_CLOUD + 11, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[12]},
    {RADIATION_CLOUD + 12, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[13]},
    {RADIATION_CLOUD + 13, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[14]},
    {RADIATION_CLOUD + 14, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[15]},
    {RADIATION_CLOUD + 15, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[16]},
    {RADIATION_CLOUD + 16, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[17]},
    {RADIATION_CLOUD + 17, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[18]},
    {RADIATION_CLOUD + 18, RADIATION_RATE, DoRadiationCloud, &s_RadiationCloud[19]},
    {RADIATION_CLOUD + 19, 100, DoSuicide, &s_RadiationCloud[0]},
};

#define CHEMBOMB_FRAMES 1
#define CHEMBOMB_R0 3038
#define CHEMBOMB_R1 CHEMBOMB_R0 + (CHEMBOMB_FRAMES * 1)
#define CHEMBOMB_R2 CHEMBOMB_R0 + (CHEMBOMB_FRAMES * 2)
#define CHEMBOMB_R3 CHEMBOMB_R0 + (CHEMBOMB_FRAMES * 3)
#define CHEMBOMB_R4 CHEMBOMB_R0 + (CHEMBOMB_FRAMES * 4)

#define CHEMBOMB CHEMBOMB_R0
#define CHEMBOMB_RATE 8
ANIMATOR DoChemBomb;

STATE s_ChemBomb[5] =
{
    {CHEMBOMB_R0 + 0, CHEMBOMB_RATE, DoChemBomb, &s_ChemBomb[1]},
    {CHEMBOMB_R1 + 0, CHEMBOMB_RATE, DoChemBomb, &s_ChemBomb[2]},
    {CHEMBOMB_R2 + 0, CHEMBOMB_RATE, DoChemBomb, &s_ChemBomb[3]},
    {CHEMBOMB_R3 + 0, CHEMBOMB_RATE, DoChemBomb, &s_ChemBomb[4]},
    {CHEMBOMB_R4 + 0, CHEMBOMB_RATE, DoChemBomb, &s_ChemBomb[0]},
};


#define CALTROPS_FRAMES 1
#define CALTROPS_R0 CALTROPS-1

#define CALTROPS_RATE 8

ANIMATOR DoCaltrops, DoCaltropsStick;

STATE s_Caltrops[] =
{
    {CALTROPS_R0 + 0, CALTROPS_RATE, DoCaltrops, &s_Caltrops[1]},
    {CALTROPS_R0 + 1, CALTROPS_RATE, DoCaltrops, &s_Caltrops[2]},
    {CALTROPS_R0 + 2, CALTROPS_RATE, DoCaltrops, &s_Caltrops[0]},
};

STATE s_CaltropsStick[] =
{
    {CALTROPS_R0 + 2, CALTROPS_RATE, DoCaltropsStick, &s_CaltropsStick[0]},
};

//////////////////////
//
// CAPTURE FLAG
//
//////////////////////

ANIMATOR DoFlag, DoCarryFlag, DoCarryFlagNoDet;

#undef FLAG
#define FLAG 2520
#define FLAG_RATE 16

STATE s_CarryFlag[] =
{
    {FLAG + 0, FLAG_RATE, DoCarryFlag, &s_CarryFlag[1]},
    {FLAG + 1, FLAG_RATE, DoCarryFlag, &s_CarryFlag[2]},
    {FLAG + 2, FLAG_RATE, DoCarryFlag, &s_CarryFlag[0]}
};

STATE s_CarryFlagNoDet[] =
{
    {FLAG + 0, FLAG_RATE, DoCarryFlagNoDet, &s_CarryFlagNoDet[1]},
    {FLAG + 1, FLAG_RATE, DoCarryFlagNoDet, &s_CarryFlagNoDet[2]},
    {FLAG + 2, FLAG_RATE, DoCarryFlagNoDet, &s_CarryFlagNoDet[0]}
};

STATE s_Flag[] =
{
    {FLAG + 0, FLAG_RATE, DoFlag, &s_Flag[1]},
    {FLAG + 1, FLAG_RATE, DoFlag, &s_Flag[2]},
    {FLAG + 2, FLAG_RATE, DoFlag, &s_Flag[0]}
};

#define PHOSPHORUS_RATE 8
ANIMATOR DoPhosphorus;

STATE s_Phosphorus[] =
{
    {PHOSPHORUS + 0, PHOSPHORUS_RATE, DoPhosphorus, &s_Phosphorus[1]},
    {PHOSPHORUS + 1, PHOSPHORUS_RATE, DoPhosphorus, &s_Phosphorus[0]},
};

ANIMATOR DoBloodSpray;

#define CHUNK1 1685
STATE s_BloodSprayChunk[] =
{
    {CHUNK1 + 0, 8, DoBloodSpray, &s_BloodSprayChunk[1]},
    {CHUNK1 + 1, 8, DoBloodSpray, &s_BloodSprayChunk[2]},
    {CHUNK1 + 2, 8, DoBloodSpray, &s_BloodSprayChunk[3]},
    {CHUNK1 + 3, 8, DoBloodSpray, &s_BloodSprayChunk[4]},
    {CHUNK1 + 4, 8, DoBloodSpray, &s_BloodSprayChunk[5]},
    {CHUNK1 + 5, 8, DoBloodSpray, &s_BloodSprayChunk[0]},
};

ANIMATOR DoWallBloodDrip;

#define DRIP 1566
STATE s_BloodSprayDrip[] =
{
    {DRIP + 0, PHOSPHORUS_RATE, DoWallBloodDrip, &s_BloodSprayDrip[1]},
    {DRIP + 1, PHOSPHORUS_RATE, DoWallBloodDrip, &s_BloodSprayDrip[2]},
    {DRIP + 2, PHOSPHORUS_RATE, DoWallBloodDrip, &s_BloodSprayDrip[0]},
};

/////////////////////////////////////////////////////////////////////////////////////////////

int
DoWallBloodDrip(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    //sp->z += (300+RANDOM_RANGE(2300)) >> 1;

    // sy & sz are the ceiling and floor of the sector you are sliding down
    if (u->sz != u->sy)
    {
        // if you are between the ceiling and floor fall fast
        if (sp->z > u->sy && sp->z < u->sz)
        {
            sp->zvel += 300;
            sp->z += sp->zvel;
        }
        else
        {
            sp->zvel = (300+RANDOM_RANGE(2300)) >> 1;
            sp->z += sp->zvel;
        }
    }
    else
    {
        sp->zvel = (300+RANDOM_RANGE(2300)) >> 1;
        sp->z += sp->zvel;
    }

    if (sp->z >= u->loz)
    {
        sp->z = u->loz;
        SpawnFloorSplash(SpriteNum);
        KillSprite(SpriteNum);
        return 0;
    }

    return 0;
}

void
SpawnMidSplash(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp np;
    USERp nu;
    short New;

    New = SpawnSprite(STAT_MISSILE, GOREDrip, s_GoreSplash, sp->sectnum,
                      sp->x, sp->y, SPRITEp_MID(sp), sp->ang, 0);

    np = &sprite[New];
    nu = User[New];

    //SetOwner(Weapon, New);
    np->shade = -12;
    np->xrepeat = 70-RANDOM_RANGE(20);
    np->yrepeat = 70-RANDOM_RANGE(20);
    nu->ox = u->ox;
    nu->oy = u->oy;
    nu->oz = u->oz;
    SET(np->cstat, CSTAT_SPRITE_YCENTER);
    RESET(np->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) < 512)
        SET(np->cstat, CSTAT_SPRITE_XFLIP);

    nu->xchange = 0;
    nu->ychange = 0;
    nu->zchange = 0;

    if (TEST(u->Flags, SPR_UNDERWATER))
        SET(nu->Flags, SPR_UNDERWATER);
}

void
SpawnFloorSplash(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp np;
    USERp nu;
    short New;

    New = SpawnSprite(STAT_MISSILE, GOREDrip, s_GoreFloorSplash, sp->sectnum,
                      sp->x, sp->y, sp->z, sp->ang, 0);

    np = &sprite[New];
    nu = User[New];

    //SetOwner(Weapon, New);
    np->shade = -12;
    np->xrepeat = 70-RANDOM_RANGE(20);
    np->yrepeat = 70-RANDOM_RANGE(20);
    nu->ox = u->ox;
    nu->oy = u->oy;
    nu->oz = u->oz;
    SET(np->cstat, CSTAT_SPRITE_YCENTER);
    RESET(np->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) < 512)
        SET(np->cstat, CSTAT_SPRITE_XFLIP);

    nu->xchange = 0;
    nu->ychange = 0;
    nu->zchange = 0;

    if (TEST(u->Flags, SPR_UNDERWATER))
        SET(nu->Flags, SPR_UNDERWATER);
}


int
DoBloodSpray(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    int cz,fz;

    if (TEST(u->Flags, SPR_UNDERWATER))
    {
        ScaleSpriteVector(Weapon, 50000);

        u->Counter += 20;  // These are STAT_SKIIP4 now, so * 2
        u->zchange += u->Counter;
    }
    else
    {
        u->Counter += 20;
        u->zchange += u->Counter;
    }

    if (sp->xvel <= 2)
    {
        // special stuff for blood worm
        sp->z += (u->zchange >> 1);

        getzsofslope(sp->sectnum, sp->x, sp->y, &cz, &fz);
        // pretend like we hit a sector
        if (sp->z >= fz)
        {
            sp->z = fz;
            SpawnFloorSplash(Weapon);
            KillSprite((short) Weapon);
            return TRUE;
        }
    }
    else
    {
        u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange,
                              u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);
    }


    MissileHitDiveArea(Weapon);

    if (u->ret)
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_PLAX_WALL:
            KillSprite(Weapon);
            return TRUE;
        case HIT_SPRITE:
        {
            short wall_ang, dang;
            short hit_sprite = NORM_SPRITE(u->ret);
            SPRITEp hsp = &sprite[hit_sprite];
            USERp hu = User[hit_sprite];

            if (TEST(hsp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                wall_ang = NORM_ANGLE(hsp->ang);
                SpawnMidSplash(Weapon);
                QueueWallBlood(Weapon, hsp->ang);
                WallBounce(Weapon, wall_ang);
                ScaleSpriteVector(Weapon, 32000);
            }
            else
            {
                u->xchange = u->ychange = 0;
                SpawnMidSplash(Weapon);
                QueueWallBlood(Weapon, hsp->ang);
                KillSprite((short) Weapon);
                return TRUE;
            }


            break;
        }

        case HIT_WALL:
        {
            short hit_wall, nw, wall_ang, dang;
            WALLp wph;
            short wb;

            hit_wall = NORM_WALL(u->ret);
            wph = &wall[hit_wall];

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, sp->x, sp->y, sp->z, sp->ang, u->ID);
                u->ret = 0;
                break;
            }


            nw = wall[hit_wall].point2;
            wall_ang = NORM_ANGLE(getangle(wall[nw].x - wph->x, wall[nw].y - wph->y) + 512);

            SpawnMidSplash(Weapon);
            wb = QueueWallBlood(Weapon, NORM_ANGLE(wall_ang+1024));

            if (wb < 0)
            {
                KillSprite(Weapon);
                return 0;
            }
            else
            {
                if (FAF_Sector(sprite[wb].sectnum) || FAF_ConnectArea(sprite[wb].sectnum))
                {
                    KillSprite(Weapon);
                    return 0;
                }

                sp->xvel = sp->yvel = u->xchange = u->ychange = 0;
                sp->xrepeat = sp->yrepeat = 70 - RANDOM_RANGE(25);
                sp->x = sprite[wb].x;
                sp->y = sprite[wb].y;

                // !FRANK! bit of a hack
                // yvel is the hit_wall
                if (sprite[wb].yvel >= 0)
                {
                    short wallnum = sprite[wb].yvel;

                    // sy & sz are the ceiling and floor of the sector you are sliding down
                    if (wall[wallnum].nextsector >= 0)
                        getzsofslope(wall[wallnum].nextsector, sp->x, sp->y, &u->sy, &u->sz);
                    else
                        u->sy = u->sz; // ceiling and floor are equal - white wall
                }

                RESET(sp->cstat,CSTAT_SPRITE_INVISIBLE);
                ChangeState(Weapon, s_BloodSprayDrip);
            }

            //WallBounce(Weapon, wall_ang);
            //ScaleSpriteVector(Weapon, 32000);
            break;
        }

        case HIT_SECTOR:
        {
            // hit floor
            if (sp->z > DIV2(u->hiz + u->loz))
            {
                if (TEST(u->Flags, SPR_UNDERWATER))
                    SET(u->Flags, SPR_BOUNCE);  // no bouncing
                // underwater

                if (u->lo_sectp && SectUser[sp->sectnum] && SectUser[sp->sectnum]->depth)
                    SET(u->Flags, SPR_BOUNCE);  // no bouncing on
                // shallow water

#if 0
                if (!TEST(u->Flags, SPR_BOUNCE))
                {
                    SpawnFloorSplash(Weapon);
                    SET(u->Flags, SPR_BOUNCE);
                    u->ret = 0;
                    u->Counter = 0;
                    u->zchange = -u->zchange;
                    ScaleSpriteVector(Weapon, 32000);   // Was 18000
                    u->zchange /= 6;
                }
                else
#endif
                {
                    u->xchange = u->ychange = 0;
                    SpawnFloorSplash(Weapon);
                    KillSprite((short) Weapon);
                    return TRUE;
                }
            }
            else
            // hit something above
            {
                u->zchange = -u->zchange;
                ScaleSpriteVector(Weapon, 32000);       // was 22000
            }
            break;
        }
        }
    }



    // if you haven't bounced or your going slow do some puffs
    if (!TEST(u->Flags, SPR_BOUNCE | SPR_UNDERWATER))
    {
        SPRITEp np;
        USERp nu;
        short New;

        New = SpawnSprite(STAT_MISSILE, GOREDrip, s_BloodSpray, sp->sectnum,
                          sp->x, sp->y, sp->z, sp->ang, 100);

        np = &sprite[New];
        nu = User[New];

        SetOwner(Weapon, New);
        np->shade = -12;
        np->xrepeat = 40-RANDOM_RANGE(30);
        np->yrepeat = 40-RANDOM_RANGE(30);
        nu->ox = u->ox;
        nu->oy = u->oy;
        nu->oz = u->oz;
        SET(np->cstat, CSTAT_SPRITE_YCENTER);
        RESET(np->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

        if (RANDOM_P2(1024) < 512)
            SET(np->cstat, CSTAT_SPRITE_XFLIP);
        if (RANDOM_P2(1024) < 512)
            SET(np->cstat, CSTAT_SPRITE_YFLIP);

        nu->xchange = u->xchange;
        nu->ychange = u->ychange;
        nu->zchange = u->zchange;

        ScaleSpriteVector(New, 20000);

        if (TEST(u->Flags, SPR_UNDERWATER))
            SET(nu->Flags, SPR_UNDERWATER);
    }

    return FALSE;
}


int
DoPhosphorus(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;

    if (TEST(u->Flags, SPR_UNDERWATER))
    {
        ScaleSpriteVector(Weapon, 50000);

        u->Counter += 20*2;
        u->zchange += u->Counter;
    }
    else
    {
        u->Counter += 20*2;
        u->zchange += u->Counter;
    }

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS*2);

    MissileHitDiveArea(Weapon);

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    if (u->ret)
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_PLAX_WALL:
            KillSprite(Weapon);
            return TRUE;
        case HIT_SPRITE:
        {
            short wall_ang, dang;
            short hit_sprite = -2;
            SPRITEp hsp;
            USERp hu;


            hit_sprite = NORM_SPRITE(u->ret);
            hsp = &sprite[hit_sprite];
            hu = User[hit_sprite];

            if (TEST(hsp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                wall_ang = NORM_ANGLE(hsp->ang);
                WallBounce(Weapon, wall_ang);
                ScaleSpriteVector(Weapon, 32000);
            }
            else
            {
                if (TEST(hsp->extra, SPRX_BURNABLE))
                {
                    if (!hu)
                        hu = SpawnUser(hit_sprite, hsp->picnum, NULL);
                    SpawnFireballExp(Weapon);
                    if (hu)
                        SpawnFireballFlames(Weapon, hit_sprite);
                    DoFlamesDamageTest(Weapon);
                }
                u->xchange = u->ychange = 0;
                KillSprite((short) Weapon);
                return TRUE;
            }


            break;
        }

        case HIT_WALL:
        {
            short hit_wall, nw, wall_ang, dang;
            WALLp wph;

            hit_wall = NORM_WALL(u->ret);
            wph = &wall[hit_wall];

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, sp->x, sp->y, sp->z, sp->ang, u->ID);
                u->ret = 0;
                break;
            }


            nw = wall[hit_wall].point2;
            wall_ang = NORM_ANGLE(getangle(wall[nw].x - wph->x, wall[nw].y - wph->y) + 512);

            WallBounce(Weapon, wall_ang);
            ScaleSpriteVector(Weapon, 32000);
            break;
        }

        case HIT_SECTOR:
        {
            SWBOOL did_hit_wall;

            if (SlopeBounce(Weapon, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a wall
                    ScaleSpriteVector(Weapon, 28000);
                    u->ret = 0;
                    u->Counter = 0;
                }
                else
                {
                    // hit a sector
                    if (sp->z > DIV2(u->hiz + u->loz))
                    {
                        // hit a floor
                        if (!TEST(u->Flags, SPR_BOUNCE))
                        {
                            SET(u->Flags, SPR_BOUNCE);
                            ScaleSpriteVector(Weapon, 32000);       // was 18000
                            u->zchange /= 6;
                            u->ret = 0;
                            u->Counter = 0;
                        }
                        else
                        {
                            u->xchange = u->ychange = 0;
                            SpawnFireballExp(Weapon);
                            KillSprite((short) Weapon);
                            return TRUE;
                        }
                    }
                    else
                    {
                        // hit a ceiling
                        ScaleSpriteVector(Weapon, 32000);   // was 22000
                    }
                }
            }
            else
            {
                // hit floor
                if (sp->z > DIV2(u->hiz + u->loz))
                {
                    if (TEST(u->Flags, SPR_UNDERWATER))
                        SET(u->Flags, SPR_BOUNCE);  // no bouncing
                    // underwater

                    if (u->lo_sectp && SectUser[sp->sectnum] && SectUser[sp->sectnum]->depth)
                        SET(u->Flags, SPR_BOUNCE);  // no bouncing on
                    // shallow water

                    if (!TEST(u->Flags, SPR_BOUNCE))
                    {
                        SET(u->Flags, SPR_BOUNCE);
                        u->ret = 0;
                        u->Counter = 0;
                        u->zchange = -u->zchange;
                        ScaleSpriteVector(Weapon, 32000);   // Was 18000
                        u->zchange /= 6;
                    }
                    else
                    {
                        u->xchange = u->ychange = 0;
                        SpawnFireballExp(Weapon);
                        KillSprite((short) Weapon);
                        return TRUE;
                    }
                }
                else
                // hit something above
                {
                    u->zchange = -u->zchange;
                    ScaleSpriteVector(Weapon, 32000);       // was 22000
                }
            }
            break;
        }
        }
    }



    // if you haven't bounced or your going slow do some puffs
    if (!TEST(u->Flags, SPR_BOUNCE | SPR_UNDERWATER) && !TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
    {
        SPRITEp np;
        USERp nu;
        short New;

        New = SpawnSprite(STAT_SKIP4, PUFF, s_PhosphorExp, sp->sectnum,
                          sp->x, sp->y, sp->z, sp->ang, 100);

        np = &sprite[New];
        nu = User[New];

        np->hitag = LUMINOUS;           // Always full brightness
        SetOwner(Weapon, New);
        np->shade = -40;
        np->xrepeat = 12 + RANDOM_RANGE(10);
        np->yrepeat = 12 + RANDOM_RANGE(10);
        nu->ox = u->ox;
        nu->oy = u->oy;
        nu->oz = u->oz;
        SET(np->cstat, CSTAT_SPRITE_YCENTER);
        RESET(np->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

        if (RANDOM_P2(1024) < 512)
            SET(np->cstat, CSTAT_SPRITE_XFLIP);
        if (RANDOM_P2(1024) < 512)
            SET(np->cstat, CSTAT_SPRITE_YFLIP);

        nu->xchange = u->xchange;
        nu->ychange = u->ychange;
        nu->zchange = u->zchange;

        nu->spal = np->pal = PALETTE_PLAYER3;   // RED

        ScaleSpriteVector(New, 20000);

        if (TEST(u->Flags, SPR_UNDERWATER))
            SET(nu->Flags, SPR_UNDERWATER);
    }

    return FALSE;
}

int
DoChemBomb(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;

    if (TEST(u->Flags, SPR_UNDERWATER))
    {
        ScaleSpriteVector(Weapon, 50000);

        u->Counter += 20;
        u->zchange += u->Counter;
    }
    else
    {
        u->Counter += 20;
        u->zchange += u->Counter;
    }

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    if (u->ret)
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_PLAX_WALL:
            KillSprite(Weapon);
            return TRUE;
        case HIT_SPRITE:
        {
            short wall_ang, dang;
            short hit_sprite;
            SPRITEp hsp;

            if (!TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                PlaySound(DIGI_CHEMBOUNCE, &sp->x, &sp->y, &sp->z, v3df_dontpan);

            hit_sprite = NORM_SPRITE(u->ret);
            hsp = &sprite[hit_sprite];

            if (TEST(hsp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                wall_ang = NORM_ANGLE(hsp->ang);
                WallBounce(Weapon, wall_ang);
                ScaleSpriteVector(Weapon, 32000);
            }
            else
            {
                // Canister pops when first smoke starts out
                if (u->WaitTics == CHEMTICS && !TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                {
                    PlaySound(DIGI_GASPOP, &sp->x, &sp->y, &sp->z, v3df_dontpan | v3df_doppler);
                    PlaySound(DIGI_CHEMGAS, &sp->x, &sp->y, &sp->z, v3df_dontpan | v3df_doppler);
                    Set3DSoundOwner(Weapon);
                }
                u->xchange = u->ychange = 0;
                u->WaitTics -= (MISSILEMOVETICS * 2);
                if (u->WaitTics <= 0)
                    KillSprite((short) Weapon);
                return TRUE;
            }


            break;
        }

        case HIT_WALL:
        {
            short hit_wall, nw, wall_ang, dang;
            WALLp wph;

            hit_wall = NORM_WALL(u->ret);
            wph = &wall[hit_wall];

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, sp->x, sp->y, sp->z, sp->ang, u->ID);
                u->ret = 0;
                break;
            }

            if (!TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                PlaySound(DIGI_CHEMBOUNCE, &sp->x, &sp->y, &sp->z, v3df_dontpan);

            nw = wall[hit_wall].point2;
            wall_ang = NORM_ANGLE(getangle(wall[nw].x - wph->x, wall[nw].y - wph->y) + 512);

            WallBounce(Weapon, wall_ang);
            ScaleSpriteVector(Weapon, 32000);
            break;
        }

        case HIT_SECTOR:
        {
            SWBOOL did_hit_wall;

            if (SlopeBounce(Weapon, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a wall
                    ScaleSpriteVector(Weapon, 28000);
                    u->ret = 0;
                    u->Counter = 0;
                }
                else
                {
                    // hit a sector
                    if (sp->z > DIV2(u->hiz + u->loz))
                    {
                        // hit a floor
                        if (!TEST(u->Flags, SPR_BOUNCE))
                        {
                            if (!TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                                PlaySound(DIGI_CHEMBOUNCE, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                            SET(u->Flags, SPR_BOUNCE);
                            ScaleSpriteVector(Weapon, 32000);       // was 18000
                            u->zchange /= 6;
                            u->ret = 0;
                            u->Counter = 0;
                        }
                        else
                        {
                            // Canister pops when first smoke starts out
                            if (u->WaitTics == CHEMTICS && !TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                            {
                                PlaySound(DIGI_GASPOP, &sp->x, &sp->y, &sp->z, v3df_dontpan | v3df_doppler);
                                PlaySound(DIGI_CHEMGAS, &sp->x, &sp->y, &sp->z, v3df_dontpan | v3df_doppler);
                                Set3DSoundOwner(Weapon);
                            }
                            SpawnRadiationCloud(Weapon);
                            u->xchange = u->ychange = 0;
                            u->WaitTics -= (MISSILEMOVETICS * 2);
                            if (u->WaitTics <= 0)
                                KillSprite((short) Weapon);
                            return TRUE;
                        }
                    }
                    else
                    {
                        // hit a ceiling
                        ScaleSpriteVector(Weapon, 32000);   // was 22000
                    }
                }
            }
            else
            {
                // hit floor
                if (sp->z > DIV2(u->hiz + u->loz))
                {
                    if (TEST(u->Flags, SPR_UNDERWATER))
                        SET(u->Flags, SPR_BOUNCE);  // no bouncing
                    // underwater

                    if (u->lo_sectp && SectUser[sp->sectnum] && SectUser[sp->sectnum]->depth)
                        SET(u->Flags, SPR_BOUNCE);  // no bouncing on
                    // shallow water

                    if (!TEST(u->Flags, SPR_BOUNCE))
                    {
                        if (!TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                            PlaySound(DIGI_CHEMBOUNCE, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                        SET(u->Flags, SPR_BOUNCE);
                        u->ret = 0;
                        u->Counter = 0;
                        u->zchange = -u->zchange;
                        ScaleSpriteVector(Weapon, 32000);   // Was 18000
                        u->zchange /= 6;
                    }
                    else
                    {
                        // Canister pops when first smoke starts out
                        if (u->WaitTics == CHEMTICS && !TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                        {
                            PlaySound(DIGI_GASPOP, &sp->x, &sp->y, &sp->z, v3df_dontpan | v3df_doppler);
                            PlaySound(DIGI_CHEMGAS, &sp->x, &sp->y, &sp->z, v3df_dontpan | v3df_doppler);
                            Set3DSoundOwner(Weapon);
                        }
                        // WeaponMoveHit(Weapon);
                        SpawnRadiationCloud(Weapon);
                        u->xchange = u->ychange = 0;
                        u->WaitTics -= (MISSILEMOVETICS * 2);
                        if (u->WaitTics <= 0)
                            KillSprite((short) Weapon);
                        return TRUE;
                    }
                }
                else
                // hit something above
                {
                    u->zchange = -u->zchange;
                    ScaleSpriteVector(Weapon, 32000);       // was 22000
                }
            }
            break;
        }
        }
    }

    //if(TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
    //SpawnRadiationCloud(Weapon);

    // if you haven't bounced or your going slow do some puffs
    if (!TEST(u->Flags, SPR_BOUNCE | SPR_UNDERWATER) && !TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
    {
        SPRITEp np;
        USERp nu;
        short New;

        New = SpawnSprite(STAT_MISSILE, PUFF, s_Puff, sp->sectnum,
                          sp->x, sp->y, sp->z, sp->ang, 100);

        np = &sprite[New];
        nu = User[New];

        SetOwner(Weapon, New);
        np->shade = -40;
        np->xrepeat = 40;
        np->yrepeat = 40;
        nu->ox = u->ox;
        nu->oy = u->oy;
        nu->oz = u->oz;
        // !Frank - dont do translucent
        SET(np->cstat, CSTAT_SPRITE_YCENTER);
        // SET(np->cstat, CSTAT_SPRITE_YCENTER|CSTAT_SPRITE_TRANSLUCENT);
        RESET(np->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

        nu->xchange = u->xchange;
        nu->ychange = u->ychange;
        nu->zchange = u->zchange;

        nu->spal = np->pal = PALETTE_PLAYER6;

        ScaleSpriteVector(New, 20000);

        if (TEST(u->Flags, SPR_UNDERWATER))
            SET(nu->Flags, SPR_UNDERWATER);
    }

    return FALSE;
}

int
DoCaltropsStick(int16_t Weapon)
{
    USERp u = User[Weapon];

    u->Counter = !u->Counter;

    if (u->Counter)
        DoFlamesDamageTest(Weapon);

    return 0;
}

int
DoCaltrops(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;

    if (TEST(u->Flags, SPR_UNDERWATER))
    {
        ScaleSpriteVector(Weapon, 50000);

        u->Counter += 20;
        u->zchange += u->Counter;
    }
    else
    {
        u->Counter += 70;
        u->zchange += u->Counter;
    }

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (u->ret)
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_PLAX_WALL:
            KillSprite(Weapon);
            return TRUE;
        case HIT_SPRITE:
        {
            short wall_ang, dang;
            short hit_sprite;
            SPRITEp hsp;

            PlaySound(DIGI_CALTROPS, &sp->x, &sp->y, &sp->z, v3df_dontpan);

            hit_sprite = NORM_SPRITE(u->ret);
            hsp = &sprite[hit_sprite];

            if (TEST(hsp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                wall_ang = NORM_ANGLE(hsp->ang);
                WallBounce(Weapon, wall_ang);
                ScaleSpriteVector(Weapon, 10000);
            }
            else
            {
                // fall to the ground
                u->xchange = u->ychange = 0;
            }


            break;
        }

        case HIT_WALL:
        {
            short hit_wall, nw, wall_ang, dang;
            WALLp wph;

            hit_wall = NORM_WALL(u->ret);
            wph = &wall[hit_wall];

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, sp->x, sp->y, sp->z, sp->ang, u->ID);
                u->ret = 0;
                break;
            }

            PlaySound(DIGI_CALTROPS, &sp->x, &sp->y, &sp->z, v3df_dontpan);

            nw = wall[hit_wall].point2;
            wall_ang = NORM_ANGLE(getangle(wall[nw].x - wph->x, wall[nw].y - wph->y) + 512);

            WallBounce(Weapon, wall_ang);
            ScaleSpriteVector(Weapon, 1000);
            break;
        }

        case HIT_SECTOR:
        {
            SWBOOL did_hit_wall;

            if (SlopeBounce(Weapon, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a wall
                    ScaleSpriteVector(Weapon, 1000);
                    u->ret = 0;
                    u->Counter = 0;
                }
                else
                {
                    // hit a sector
                    if (sp->z > DIV2(u->hiz + u->loz))
                    {
                        // hit a floor
                        if (!TEST(u->Flags, SPR_BOUNCE))
                        {
                            PlaySound(DIGI_CALTROPS, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                            SET(u->Flags, SPR_BOUNCE);
                            ScaleSpriteVector(Weapon, 1000);        // was 18000
                            u->ret = 0;
                            u->Counter = 0;
                        }
                        else
                        {
                            u->xchange = u->ychange = 0;
                            SET(sp->extra, SPRX_BREAKABLE);
                            SET(sp->cstat,CSTAT_SPRITE_BREAKABLE);
                            ChangeState(Weapon, s_CaltropsStick);
                            return TRUE;
                        }
                    }
                    else
                    {
                        // hit a ceiling
                        ScaleSpriteVector(Weapon, 1000);    // was 22000
                    }
                }
            }
            else
            {
                // hit floor
                if (sp->z > DIV2(u->hiz + u->loz))
                {
                    if (TEST(u->Flags, SPR_UNDERWATER))
                        SET(u->Flags, SPR_BOUNCE);  // no bouncing
                    // underwater

                    if (u->lo_sectp && SectUser[sp->sectnum] && SectUser[sp->sectnum]->depth)
                        SET(u->Flags, SPR_BOUNCE);  // no bouncing on
                    // shallow water

                    if (!TEST(u->Flags, SPR_BOUNCE))
                    {
                        PlaySound(DIGI_CALTROPS, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                        SET(u->Flags, SPR_BOUNCE);
                        u->ret = 0;
                        u->Counter = 0;
                        u->zchange = -u->zchange;
                        ScaleSpriteVector(Weapon, 1000);    // Was 18000
                    }
                    else
                    {
                        u->xchange = u->ychange = 0;
                        SET(sp->extra, SPRX_BREAKABLE);
                        SET(sp->cstat,CSTAT_SPRITE_BREAKABLE);
                        ChangeState(Weapon, s_CaltropsStick);
                        return TRUE;
                    }
                }
                else
                // hit something above
                {
                    u->zchange = -u->zchange;
                    ScaleSpriteVector(Weapon, 1000);        // was 22000
                }
            }
            break;
        }
        }
    }


    return FALSE;
}

/////////////////////////////
//
// Deadly green gas clouds
//
/////////////////////////////
int
SpawnRadiationCloud(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], np;
    USERp u = User[SpriteNum], nu;
    short New;


    if (!MoveSkip4)
        return FALSE;

    // This basically works like a MoveSkip8, if one existed
//  u->Counter2 = !u->Counter2;
    if (u->ID == MUSHROOM_CLOUD || u->ID == 3121)
    {
        if ((u->Counter2++) > 16)
            u->Counter2 = 0;
        if (u->Counter2)
            return FALSE;
    }
    else
    {
        if ((u->Counter2++) > 2)
            u->Counter2 = 0;
        if (u->Counter2)
            return FALSE;
    }

    if (TEST(u->Flags, SPR_UNDERWATER))
        return -1;

    New = SpawnSprite(STAT_MISSILE, RADIATION_CLOUD, s_RadiationCloud, sp->sectnum,
                      sp->x, sp->y, sp->z - RANDOM_P2(Z(8)), sp->ang, 0);

    np = &sprite[New];
    nu = User[New];

    SetOwner(sp->owner, New);
    nu->WaitTics = 1 * 120;
    np->shade = -40;
    np->xrepeat = 32;
    np->yrepeat = 32;
    np->clipdist = sp->clipdist;
    SET(np->cstat, CSTAT_SPRITE_YCENTER);
    RESET(np->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    nu->spal = np->pal = PALETTE_PLAYER6;
    // Won't take floor palettes
    np->hitag = SECTFU_DONT_COPY_PALETTE;

    if (RANDOM_P2(1024) < 512)
        SET(np->cstat, CSTAT_SPRITE_XFLIP);
    //if (RANDOM_P2(1024) < 512)
    //SET(np->cstat, CSTAT_SPRITE_YFLIP);

    np->ang = RANDOM_P2(2048);
    np->xvel = RANDOM_P2(32);

    nu->Counter = 0;
    nu->Counter2 = 0;

    if (u->ID == MUSHROOM_CLOUD || u->ID == 3121)
    {
        nu->Radius = 2000;
        nu->xchange = (MOVEx(np->xvel>>2, np->ang));
        nu->ychange = (MOVEy(np->xvel>>2, np->ang));
        np->zvel = Z(1) + RANDOM_P2(Z(2));
    }
    else
    {
        nu->xchange = MOVEx(np->xvel, np->ang);
        nu->ychange = MOVEy(np->xvel, np->ang);
        np->zvel = Z(4) + RANDOM_P2(Z(4));
        nu->Radius = 4000;
    }

    return FALSE;
}

int
DoRadiationCloud(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    sp->z -= sp->zvel;

    sp->x += u->xchange;
    sp->y += u->ychange;

    if (u->ID)
    {
        DoFlamesDamageTest(SpriteNum);
    }

    return FALSE;
}

//////////////////////////////////////////////
//
// Inventory Chemical Bombs
//
//////////////////////////////////////////////
int
PlayerInitChemBomb(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w, hit_sprite;
    short oclipdist;
    int dist;


    PlaySound(DIGI_THROW, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan | v3df_doppler);

    nx = pp->posx;
    ny = pp->posy;
    nz = pp->posz + pp->bob_z + Z(8);

    // Spawn a shot
    // Inserting and setting up variables
    w = SpawnSprite(STAT_MISSILE, CHEMBOMB, s_ChemBomb, pp->cursectnum,
                    nx, ny, nz, pp->pang, CHEMBOMB_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    // don't throw it as far if crawling
    if (TEST(pp->Flags, PF_CRAWLING))
    {
        wp->xvel -= DIV4(wp->xvel);
    }

//    wu->RotNum = 5;
//    NewStateGroup(w, &sg_ChemBomb);
    SET(wu->Flags, SPR_XFLIP_TOGGLE);

    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 32;
    wp->xrepeat = 32;
    wp->shade = -15;
    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 200;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    wu->Counter = 0;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK);

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    wp->zvel = ((100 - pp->horiz) * HORIZ_MULT);

    // //DSPRINTF(ds,"horiz %d, ho %d, ho+ho %d",pp->horiz, pp->horizoff,
    // pp->horizoff + pp->horiz);
    // MONO_PRINT(ds);

    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 0;
    wp->clipdist = 0;

//    wp->ang = NORM_ANGLE(wp->ang - 512);
//    HelpMissileLateral(w, 800);
//    wp->ang = NORM_ANGLE(wp->ang + 512);

    MissileSetPos(w, DoChemBomb, 1000);

    pp->SpriteP->clipdist = oclipdist;
    wp->clipdist = 80L >> 2;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel >> 1;

    // adjust xvel according to player velocity
    wu->xchange += pp->xvect >> 14;
    wu->ychange += pp->yvect >> 14;

    // Smoke will come out for this many seconds
    wu->WaitTics = CHEMTICS;

    return 0;
}

int
InitSpriteChemBomb(int16_t SpriteNum)
{
    USERp u = User[SpriteNum];
    USERp wu;
    SPRITEp sp = &sprite[SpriteNum], wp;
    int nx, ny, nz;
    short w, hit_sprite;
    short oclipdist;
    int dist;


    PlaySound(DIGI_THROW, &sp->x, &sp->y, &sp->z, v3df_dontpan | v3df_doppler);

    nx = sp->x;
    ny = sp->y;
    nz = sp->z;

    // Spawn a shot
    // Inserting and setting up variables
    w = SpawnSprite(STAT_MISSILE, CHEMBOMB, s_ChemBomb, sp->sectnum,
                    nx, ny, nz, sp->ang, CHEMBOMB_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    SET(wu->Flags, SPR_XFLIP_TOGGLE);

    SetOwner(SpriteNum, w);
    wp->yrepeat = 32;
    wp->xrepeat = 32;
    wp->shade = -15;
    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 200;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    wu->Counter = 0;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK);

    wp->zvel = ((-100 - RANDOM_RANGE(100)) * HORIZ_MULT);

    wp->clipdist = 80L >> 2;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel >> 1;

    // Smoke will come out for this many seconds
    wu->WaitTics = CHEMTICS;

    return 0;
}


int
InitChemBomb(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w, hit_sprite;
    short oclipdist;
    int dist;


// Need to make it take away from inventory weapon list
//    PlayerUpdateAmmo(pp, u->WeaponNum, -1);

    nx = sp->x;
    ny = sp->y;
    nz = sp->z;

    // Spawn a shot
    // Inserting and setting up variables
    w = SpawnSprite(STAT_MISSILE, MUSHROOM_CLOUD, s_ChemBomb, sp->sectnum,
                    nx, ny, nz, sp->ang, CHEMBOMB_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

//    wu->RotNum = 5;
//    NewStateGroup(w, &sg_ChemBomb);
    SET(wu->Flags, SPR_XFLIP_TOGGLE);

//    SetOwner(SpriteNum, w);
//    SetOwner(-1, w);
    SetOwner(sp->owner, w); // !FRANK
    wp->yrepeat = 32;
    wp->xrepeat = 32;
    wp->shade = -15;
    wu->Radius = 200;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    wu->Counter = 0;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_INVISIBLE);      // Make nuke radiation
    // invis.
    RESET(wp->cstat, CSTAT_SPRITE_BLOCK);

    if (SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    wp->zvel = ((-100 - RANDOM_RANGE(100)) * HORIZ_MULT);
    wp->clipdist = 0;

    if (u->ID == MUSHROOM_CLOUD || u->ID == 3121 || u->ID == SUMO_RUN_R0) // 3121 == GRENADE_EXP
    {
        wu->xchange = 0;
        wu->ychange = 0;
        wu->zchange = 0;
        wp->xvel = wp->yvel = wp->zvel = 0;
        // Smoke will come out for this many seconds
        wu->WaitTics = 40*120;
    }
    else
    {
        wu->xchange = MOVEx(wp->xvel, wp->ang);
        wu->ychange = MOVEy(wp->xvel, wp->ang);
        wu->zchange = wp->zvel >> 1;
        // Smoke will come out for this many seconds
        wu->WaitTics = 3*120;
    }


    return 0;
}

//////////////////////////////////////////////
//
// Inventory Flash Bombs
//
//////////////////////////////////////////////
int
PlayerInitFlashBomb(PLAYERp pp)
{
    short pnum, i, nexti;
    unsigned int stat;
    int dist, tx, ty, tmin;
    short damage;
    SPRITEp sp = pp->SpriteP, hp;
    USERp u = User[pp->PlayerSprite], hu;

    PlaySound(DIGI_GASPOP, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan | v3df_doppler);

    // Set it just a little to let player know what he just did
    SetFadeAmt(pp, -30, 1);             // White flash

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];
            hu = User[i];

            if (i == pp->PlayerSprite)
                break;

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, tx, ty, tmin);
            if (dist > 16384)           // Flash radius
                continue;

            if (!TEST(sp->cstat, CSTAT_SPRITE_BLOCK))
                continue;

            if (!FAFcansee(hp->x, hp->y, hp->z, hp->sectnum, sp->x, sp->y, sp->z - SPRITEp_SIZE_Z(sp), sp->sectnum))
                continue;

            damage = GetDamage(i, pp->PlayerSprite, DMG_FLASHBOMB);

            if (hu->sop_parent)
            {
                break;
            }
            else if (hu->PlayerP)
            {
//              if(hu->PlayerP->NightVision)
//              {
//                  SetFadeAmt(hu->PlayerP, -200, 1); // Got him with night vision on!
//                  PlayerUpdateHealth(hu->PlayerP, -15); // Hurt eyes
//              }else
                if (damage < -70)
                {
                    int choosesnd = 0;

                    choosesnd = RANDOM_RANGE(MAX_PAIN);

                    PlayerSound(PlayerLowHealthPainVocs[choosesnd],&pp->posx,
                                &pp->posy,&pp->posy,v3df_dontpan|v3df_doppler|v3df_follow,pp);
                }
                SetFadeAmt(hu->PlayerP, damage, 1);     // White flash
            }
            else
            {
                ActorPain(i);
                SpawnFlashBombOnActor(i);
            }
        }
    }

    return 0;
}

int
InitFlashBomb(int16_t SpriteNum)
{
    short pnum, i, nexti;
    unsigned int stat;
    int dist, tx, ty, tmin;
    short damage;
    SPRITEp sp = &sprite[SpriteNum], hp;
    USERp u = User[SpriteNum], hu;
    PLAYERp pp = Player + screenpeek;

    PlaySound(DIGI_GASPOP, &sp->x, &sp->y, &sp->z, v3df_dontpan | v3df_doppler);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];
            hu = User[i];

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, tx, ty, tmin);
            if (dist > 16384)           // Flash radius
                continue;

            if (!TEST(sp->cstat, CSTAT_SPRITE_BLOCK))
                continue;

            if (!FAFcansee(hp->x, hp->y, hp->z, hp->sectnum, sp->x, sp->y, sp->z - SPRITEp_SIZE_Z(sp), sp->sectnum))
                continue;

            damage = GetDamage(i, SpriteNum, DMG_FLASHBOMB);

            if (hu->sop_parent)
            {
                break;
            }
            else if (hu->PlayerP)
            {
                if (damage < -70)
                {
                    int choosesnd = 0;

                    choosesnd = RANDOM_RANGE(MAX_PAIN);

                    PlayerSound(PlayerLowHealthPainVocs[choosesnd],&pp->posx,
                                &pp->posy,&pp->posy,v3df_dontpan|v3df_doppler|v3df_follow,pp);
                }
                SetFadeAmt(hu->PlayerP, damage, 1);     // White flash
            }
            else
            {
                if (i != SpriteNum)
                {
                    ActorPain(i);
                    SpawnFlashBombOnActor(i);
                }
            }
        }
    }

    return 0;
}


// This is a sneaky function to make actors look blinded by flashbomb while using flaming code
int
SpawnFlashBombOnActor(int16_t enemy)
{
    SPRITEp ep = &sprite[enemy];
    USERp eu = User[enemy];
    SPRITEp np;
    USERp nu;
    short New;


    // Forget about burnable sprites
    if (TEST(ep->extra, SPRX_BURNABLE))
        return eu->flame;


    if (enemy >= 0)
    {
        if (!eu)
        {
            ASSERT(TRUE == FALSE);
        }

        if (eu->flame >= 0)
        {
            int sizez = SPRITEp_SIZE_Z(ep) + DIV4(SPRITEp_SIZE_Z(ep));

            np = &sprite[eu->flame];
            nu = User[eu->flame];


            if (nu->Counter >= SPRITEp_SIZE_Z_2_YREPEAT(np, sizez))
            {
                // keep flame only slightly bigger than the enemy itself
                nu->Counter = SPRITEp_SIZE_Z_2_YREPEAT(np, sizez) * 2;
            }
            else
            {
                // increase max size
                nu->Counter += SPRITEp_SIZE_Z_2_YREPEAT(np, 8 << 8) * 2;
            }

            // Counter is max size
            if (nu->Counter >= 230)
            {
                // this is far too big
                nu->Counter = 230;
            }

            if (nu->WaitTics < 2 * 120)
                nu->WaitTics = 2 * 120; // allow it to grow again

            return eu->flame;
        }
    }

    New = SpawnSprite(STAT_MISSILE, FIREBALL_FLAMES, s_FireballFlames, ep->sectnum,
                      ep->x, ep->y, ep->z, ep->ang, 0);
    np = &sprite[New];
    nu = User[New];

    if (enemy >= 0)
        eu->flame = New;

    np->xrepeat = 16;
    np->yrepeat = 16;

    if (enemy >= 0)
    {
        nu->Counter = SPRITEp_SIZE_Z_2_YREPEAT(np, SPRITEp_SIZE_Z(ep) >> 1) * 4;
    }
    else
        nu->Counter = 0;                // max flame size

    np->shade = -40;
    SET(np->cstat, CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_INVISIBLE);
    RESET(np->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    nu->Radius = 200;

    if (enemy >= 0)
    {
        SetAttach(enemy, New);
    }

    return New;
}

//////////////////////////////////////////////
//
// Inventory Caltrops
//
//////////////////////////////////////////////
int
PlayerInitCaltrops(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w, hit_sprite;
    short oclipdist, i;
    int dist;


    PlaySound(DIGI_THROW, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan | v3df_doppler);

    nx = pp->posx;
    ny = pp->posy;
    nz = pp->posz + pp->bob_z + Z(8);

    // Throw out several caltrops
//  for(i=0;i<3;i++)
//  {
    // Spawn a shot
    // Inserting and setting up variables
    w = SpawnSprite(STAT_DEAD_ACTOR, CALTROPS, s_Caltrops, pp->cursectnum,
                    nx, ny, nz, pp->pang, (CHEMBOMB_VELOCITY + RANDOM_RANGE(CHEMBOMB_VELOCITY)) / 2);

    wp = &sprite[w];
    wu = User[w];

    // don't throw it as far if crawling
    if (TEST(pp->Flags, PF_CRAWLING))
    {
        wp->xvel -= DIV4(wp->xvel);
    }

    SET(wu->Flags, SPR_XFLIP_TOGGLE);

    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 64;
    wp->xrepeat = 64;
    wp->shade = -15;
    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 200;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    wu->Counter = 0;
//      SET(wp->cstat, CSTAT_SPRITE_BLOCK);

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    // They go out at different angles
//        wp->ang = NORM_ANGLE(pp->pang + (RANDOM_RANGE(50) - 25));

    wp->zvel = ((100 - pp->horiz) * HORIZ_MULT);

    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 0;
    wp->clipdist = 0;

    MissileSetPos(w, DoCaltrops, 1000);

    pp->SpriteP->clipdist = oclipdist;
    wp->clipdist = 80L >> 2;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel >> 1;

    // adjust xvel according to player velocity
    wu->xchange += pp->xvect >> 14;
    wu->ychange += pp->yvect >> 14;

    // Caltrops stay around for this many seconds
//      wu->WaitTics = CHEMTICS*5;
//  }

    SetupSpriteForBreak(wp);            // Put Caltrops in the break queue
    return 0;
}

int
InitCaltrops(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w, hit_sprite;
    short oclipdist, i;
    int dist;


    PlaySound(DIGI_THROW, &sp->x, &sp->y, &sp->z, v3df_dontpan | v3df_doppler);

    nx = sp->x;
    ny = sp->y;
    nz = sp->z;

    // Spawn a shot
    // Inserting and setting up variables
    w = SpawnSprite(STAT_DEAD_ACTOR, CALTROPS, s_Caltrops, sp->sectnum,
                    nx, ny, nz, sp->ang, CHEMBOMB_VELOCITY / 2);

    wp = &sprite[w];
    wu = User[w];

    SET(wu->Flags, SPR_XFLIP_TOGGLE);

    SetOwner(SpriteNum, w);
    wp->yrepeat = 64;
    wp->xrepeat = 64;
    wp->shade = -15;
    // !FRANK - clipbox must be <= weapon otherwise can clip thru walls
    wp->clipdist = sp->clipdist;
    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 200;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    wu->Counter = 0;

    wp->zvel = ((-100 - RANDOM_RANGE(100)) * HORIZ_MULT);

    // wp->clipdist = 80L>>2;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel >> 1;

    SetupSpriteForBreak(wp);            // Put Caltrops in the break queue
    return 0;
}

int
InitPhosphorus(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w, hit_sprite;
    short oclipdist, i, daang;
    int dist;


    PlaySound(DIGI_FIREBALL1, &sp->x, &sp->y, &sp->z, v3df_follow);

    nx = sp->x;
    ny = sp->y;
    nz = sp->z;

    daang = NORM_ANGLE(RANDOM_RANGE(2048));

    // Spawn a shot
    // Inserting and setting up variables
    w = SpawnSprite(STAT_SKIP4, FIREBALL1, s_Phosphorus, sp->sectnum,
                    nx, ny, nz, daang, CHEMBOMB_VELOCITY/3);

    wp = &sprite[w];
    wu = User[w];

    wp->hitag = LUMINOUS;               // Always full brightness
    SET(wu->Flags, SPR_XFLIP_TOGGLE);
    // !Frank - don't do translucent
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    // SET(wp->cstat, CSTAT_SPRITE_TRANSLUCENT|CSTAT_SPRITE_YCENTER);
    wp->shade = -128;

    //SetOwner(SpriteNum, w);
    wp->yrepeat = 64;
    wp->xrepeat = 64;
    wp->shade = -15;
    // !FRANK - clipbox must be <= weapon otherwise can clip thru walls
    if (sp->clipdist > 0)
        wp->clipdist = sp->clipdist-1;
    else
        wp->clipdist = sp->clipdist;
    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 600;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    wu->Counter = 0;

    wp->zvel = ((-100 - RANDOM_RANGE(100)) * HORIZ_MULT);

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = (wp->zvel >> 1);

    return 0;
}

int
InitBloodSpray(int16_t SpriteNum, SWBOOL dogib, short velocity)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w, hit_sprite;
    short oclipdist, i, cnt, ang, vel, rnd;
    int dist;


    if (dogib)
        cnt = RANDOM_RANGE(3)+1;
    else
        cnt = 1;

    //if(dogib)
    //    {
    rnd = RANDOM_RANGE(1000);
    if (rnd > 650)
        PlaySound(DIGI_GIBS1, &sp->x, &sp->y, &sp->z, v3df_none);
    else if (rnd > 350)
        PlaySound(DIGI_GIBS2, &sp->x, &sp->y, &sp->z, v3df_none);
    else
        PlaySound(DIGI_GIBS3, &sp->x, &sp->y, &sp->z, v3df_none);
    //    }

    ang = sp->ang;
    vel = velocity;

    for (i=0; i<cnt; i++)
    {

        if (velocity == -1)
            vel = 105+RANDOM_RANGE(320);
        else if (velocity == -2)
            vel = 105+RANDOM_RANGE(100);

        if (dogib)
            ang = NORM_ANGLE(ang + 512 + RANDOM_RANGE(200));
        else
            ang = NORM_ANGLE(ang+1024+256 - RANDOM_RANGE(256));

        nx = sp->x;
        ny = sp->y;
        nz = SPRITEp_TOS(sp)-20;

        //RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

        // Spawn a shot
        // Inserting and setting up variables
        w = SpawnSprite(STAT_MISSILE, GOREDrip, s_BloodSprayChunk, sp->sectnum,
                        nx, ny, nz, ang, vel*2);

        wp = &sprite[w];
        wu = User[w];

        SET(wu->Flags, SPR_XFLIP_TOGGLE);
        if (dogib)
            SET(wp->cstat, CSTAT_SPRITE_YCENTER);
        else
            SET(wp->cstat, CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_INVISIBLE);
        wp->shade = -12;

        SetOwner(SpriteNum, w);
        wp->yrepeat = 64-RANDOM_RANGE(35);
        wp->xrepeat = 64-RANDOM_RANGE(35);
        wp->shade = -15;
        wp->clipdist = sp->clipdist;
        wu->WeaponNum = u->WeaponNum;
        wu->Radius = 600;
        wu->ceiling_dist = Z(3);
        wu->floor_dist = Z(3);
        wu->Counter = 0;

        wp->zvel = ((-10 - RANDOM_RANGE(50)) * HORIZ_MULT);

        wu->xchange = MOVEx(wp->xvel, wp->ang);
        wu->ychange = MOVEy(wp->xvel, wp->ang);
        wu->zchange = wp->zvel >> 1;

        if (!GlobalSkipZrange)
            DoActorZrange(w);
    }

    return 0;
}

int
BloodSprayFall(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    sp->z += 1500;

    return 0;
}

////////////////// DEATHFLAG! ////////////////////////////////////////////////////////////////
// Rules: Run to an enemy flag, run over it an it will stick to you.
// The goal is to run the enemy's flag back to your startpoint.
// If an enemy flag touches a friendly start sector, then the opposing team explodes and
// your team wins and the level restarts.
// Once you pick up a flag, you have 30 seconds to score, otherwise, the flag detonates
// an explosion, killing you and anyone in the vicinity, and you don't score.
//////////////////////////////////////////////////////////////////////////////////////////////

// Update the scoreboard for team color that just scored.
void
DoFlagScore(int16_t pal)
{
    SPRITEp sp;
    int SpriteNum = 0, NextSprite = 0;

    TRAVERSE_SPRITE_STAT(headspritestat[0], SpriteNum, NextSprite)
    {
        sp = &sprite[SpriteNum];

        if (sp->picnum < 1900 || sp->picnum > 1999)
            continue;

        if (sp->pal == pal)
            sp->picnum++;               // Increment the counter

        if (sp->picnum > 1999)
            sp->picnum = 1900;          // Roll it over if you must

    }
}

int
DoFlagRangeTest(short Weapon, short range)
{
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];

    USERp u;
    SPRITEp sp;
    short i, nexti;
    unsigned int stat;
    int dist, tx, ty;
    int tmin;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            sp = &sprite[i];
            u = User[i];


            DISTANCE(sp->x, sp->y, wp->x, wp->y, dist, tx, ty, tmin);
            if (dist > range)
                continue;

            if (sp == wp)
                continue;

            if (!TEST(sp->cstat, CSTAT_SPRITE_BLOCK))
                continue;

            if (!TEST(sp->extra, SPRX_PLAYER_OR_ENEMY))
                continue;

            if (!FAFcansee(sp->x, sp->y, sp->z, sp->sectnum, wp->x, wp->y, wp->z, wp->sectnum))
                continue;

            dist = FindDistance3D(wp->x - sp->x, wp->y - sp->y, (wp->z - sp->z) >> 4);
            if (dist > range)
                continue;

            return i;                   // Return the spritenum
        }
    }

    return -1;                          // -1 for no sprite index.  Not
    // found.
}

int
DoCarryFlag(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

#define FLAG_DETONATE_STATE 99
    SPRITEp fp = &sprite[u->FlagOwner];
    USERp fu = User[u->FlagOwner];


    // if no owner then die
    if (u->Attach >= 0)
    {
        SPRITEp ap = &sprite[u->Attach];

        setspritez_old(Weapon, ap->x, ap->y, SPRITEp_MID(ap));
        sp->ang = NORM_ANGLE(ap->ang + 1536);
    }

    // not activated yet
    if (!TEST(u->Flags, SPR_ACTIVE))
    {
        if ((u->WaitTics -= (MISSILEMOVETICS * 2)) > 0)
            return FALSE;

        // activate it
        u->WaitTics = SEC(30);          // You have 30 seconds to get it to
        // scorebox
        u->Counter2 = 0;
        SET(u->Flags, SPR_ACTIVE);
    }

    // limit the number of times DoFlagRangeTest is called
    u->Counter++;
    if (u->Counter > 1)
        u->Counter = 0;

    if (!u->Counter)
    {
        // not already in detonate state
        if (u->Counter2 < FLAG_DETONATE_STATE)
        {
            SPRITEp ap = &sprite[u->Attach];
            USERp au = User[u->Attach];

            if (!au || au->Health <= 0)
            {
                u->Counter2 = FLAG_DETONATE_STATE;
                u->WaitTics = SEC(1) / 2;
            }
            // if in score box, score.
            if (sector[ap->sectnum].hitag == 9000 && sector[ap->sectnum].lotag == ap->pal
                && ap->pal != sp->pal)
            {
                if (u->FlagOwner >= 0)
                {
                    if (fp->lotag)      // Trigger everything if there is a
                        // lotag
                        DoMatchEverything(NULL, fp->lotag, ON);
                }
                if (!TEST_BOOL1(fp))
                {
                    PlaySound(DIGI_BIGITEM, &ap->x, &ap->y, &ap->z, v3df_none);
                    DoFlagScore(ap->pal);
                    if (SP_TAG5(fp) > 0)
                    {
                        fu->filler++;
                        if (fu->filler >= SP_TAG5(fp))
                        {
                            fu->filler = 0;
                            DoMatchEverything(NULL, SP_TAG6(fp), ON);
                        }
                    }
                }
                SetSuicide(Weapon);     // Kill the flag, you scored!
            }
        }
        else
        {
            // Time's up! Move directly to detonate state
            u->Counter2 = FLAG_DETONATE_STATE;
            u->WaitTics = SEC(1) / 2;
        }

    }

    u->WaitTics -= (MISSILEMOVETICS * 2);

    switch (u->Counter2)
    {
    case 0:
        if (u->WaitTics < SEC(30))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 1:
        if (u->WaitTics < SEC(20))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 2:
        if (u->WaitTics < SEC(10))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 3:
        if (u->WaitTics < SEC(5))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 4:
        if (u->WaitTics < SEC(4))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 5:
        if (u->WaitTics < SEC(3))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 6:
        if (u->WaitTics < SEC(2))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2 = FLAG_DETONATE_STATE;
        }
        break;
    case FLAG_DETONATE_STATE:
        // start frantic beeping
        PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
        u->Counter2++;
        break;
    case FLAG_DETONATE_STATE + 1:
        SpawnGrenadeExp(Weapon);
        SetSuicide(Weapon);
        return FALSE;
        break;
    }

    return FALSE;
}

int
DoCarryFlagNoDet(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp ap = &sprite[u->Attach];
    USERp au = User[u->Attach];
    SPRITEp fp = &sprite[u->FlagOwner];
    USERp fu = User[u->FlagOwner];


    if (u->FlagOwner >= 0)
        fu->WaitTics = 30 * 120;        // Keep setting respawn tics so it
    // won't respawn

    // if no owner then die
    if (u->Attach >= 0)
    {
        SPRITEp ap = &sprite[u->Attach];

        setspritez_old(Weapon, ap->x, ap->y, SPRITEp_MID(ap));
        sp->ang = NORM_ANGLE(ap->ang + 1536);
        sp->z = ap->z - DIV2(SPRITEp_SIZE_Z(ap));
    }


    if (!au || au->Health <= 0)
    {
        if (u->FlagOwner >= 0)
            fu->WaitTics = 0;           // Tell it to respawn
        SetSuicide(Weapon);
        return FALSE;
    }

    // if in score box, score.
    if (sector[ap->sectnum].hitag == 9000 && sector[ap->sectnum].lotag == ap->pal
        && ap->pal != sp->pal)
    {
        if (u->FlagOwner >= 0)
        {
            //DSPRINTF(ds, "Flag has owner %d, fp->lotag = %d", u->FlagOwner, fp->lotag);
            //MONO_PRINT(ds);
            if (fp->lotag)              // Trigger everything if there is a
                // lotag
                DoMatchEverything(NULL, fp->lotag, ON);
            fu->WaitTics = 0;           // Tell it to respawn
        }
        if (!TEST_BOOL1(fp))
        {
            PlaySound(DIGI_BIGITEM, &ap->x, &ap->y, &ap->z, v3df_none);
            DoFlagScore(ap->pal);
            if (SP_TAG5(fp) > 0)
            {
                fu->filler++;
                if (fu->filler >= SP_TAG5(fp))
                {
                    fu->filler = 0;
                    DoMatchEverything(NULL, SP_TAG6(fp), ON);
                }
            }
        }
        SetSuicide(Weapon);             // Kill the flag, you scored!
    }

    return FALSE;
}


int
SetCarryFlag(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

    // stuck
    SET(u->Flags, SPR_BOUNCE);
    // not yet active for 1 sec
//    RESET(u->Flags, SPR_ACTIVE);
//    u->WaitTics = SEC(3);
    SET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    u->Counter = 0;
    change_sprite_stat(Weapon, STAT_ITEM);
    if (sp->hitag == 1)
        ChangeState(Weapon, s_CarryFlagNoDet);
    else
        ChangeState(Weapon, s_CarryFlag);

    return FALSE;
}

int
DoFlag(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int16_t hit_sprite = -1;

    hit_sprite = DoFlagRangeTest(Weapon, 1000);

    if (hit_sprite != -1)
    {
        SPRITEp hsp = &sprite[hit_sprite];

        SetCarryFlag(Weapon);

        // check to see if sprite is player or enemy
        if (TEST(hsp->extra, SPRX_PLAYER_OR_ENEMY))
        {
            // attach weapon to sprite
            RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            SetAttach(hit_sprite, Weapon);
            u->sz = hsp->z - DIV2(SPRITEp_SIZE_Z(hsp));
            //u->sz = hsp->z - SPRITEp_MID(hsp);   // Set mid way up who it hit
        }
    }

    return FALSE;
}


int
InitShell(int16_t SpriteNum, int16_t ShellNum)
{
    USERp u = User[SpriteNum];
    USERp wu;
    SPRITEp sp = &sprite[SpriteNum], wp;
    int nx, ny, nz;
    short w;
    short oclipdist,id=0,velocity=0;
    int dist;
    STATEp p=NULL;
    int zvel;
    extern STATE s_UziShellShrap[];
    extern STATE s_ShotgunShellShrap[];

#define UZI_SHELL 2152
#define SHOT_SHELL 2180

    nx = sp->x;
    ny = sp->y;
    nz = DIV2(SPRITEp_TOS(sp)+ SPRITEp_BOS(sp));

    switch (ShellNum)
    {
    case -2:
    case -3:
        id = UZI_SHELL;
        p = s_UziShellShrap;
        velocity = 1500 + RANDOM_RANGE(1000);
        break;
    case -4:
        id = SHOT_SHELL;
        p = s_ShotgunShellShrap;
        velocity = 2000 + RANDOM_RANGE(1000);
        break;
    }

    w = SpawnSprite(STAT_SKIP4, id, p, sp->sectnum,
                    nx, ny, nz, sp->ang, 64);

    wp = &sprite[w];
    wu = User[w];

    wp->zvel = -(velocity);

    if (u->PlayerP)
    {
        wp->z += ((100 - u->PlayerP->horiz) * (HORIZ_MULT/3));
    }

    switch (wu->ID)
    {
    case UZI_SHELL:
        wp->z -= Z(13);

        if (ShellNum == -3)
        {
            wp->ang = sp->ang;
            HelpMissileLateral(w,2500);
            wp->ang = NORM_ANGLE(wp->ang-512);
            HelpMissileLateral(w,1000); // Was 1500
            wp->ang = NORM_ANGLE(wp->ang+712);
        }
        else
        {
            wp->ang = sp->ang;
            HelpMissileLateral(w,2500);
            wp->ang = NORM_ANGLE(wp->ang+512);
            HelpMissileLateral(w,1500);
            wp->ang = NORM_ANGLE(wp->ang-128);
        }
        wp->ang += (RANDOM_P2(128<<5)>>5) - DIV2(128);
        wp->ang = NORM_ANGLE(wp->ang);

        // Set the shell number
        wu->ShellNum = ShellCount;
        wp->yrepeat = wp->xrepeat = 13;
        break;
    case SHOT_SHELL:
        wp->z -= Z(13);
        wp->ang = sp->ang;
        HelpMissileLateral(w,2500);
        wp->ang = NORM_ANGLE(wp->ang+512);
        HelpMissileLateral(w,1300);
        wp->ang = NORM_ANGLE(wp->ang-128-64);
        wp->ang += (RANDOM_P2(128<<5)>>5) - DIV2(128);
        wp->ang = NORM_ANGLE(wp->ang);

        // Set the shell number
        wu->ShellNum = ShellCount;
        wp->yrepeat = wp->xrepeat = 18;
        break;
    }

    SetOwner(SpriteNum, w);
    wp->shade = -15;
    wu->ceiling_dist = Z(1);
    wu->floor_dist = Z(1);
    wu->Counter = 0;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(wp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    RESET(wu->Flags, SPR_BOUNCE|SPR_UNDERWATER); // Make em' bounce

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;
    //if (TEST(u->PlayerP->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
    //    SET(wu->Flags, SPR_UNDERWATER);
    wu->jump_speed = 200;
    wu->jump_speed += RANDOM_RANGE(400);
    wu->jump_speed = -wu->jump_speed;

    DoBeginJump(w);
    wu->jump_grav = ACTOR_GRAVITY;

    return 0;
}


#include "saveable.h"

static saveable_data saveable_jweapon_data[] =
{
    SAVE_DATA(s_BloodSpray),
    SAVE_DATA(s_PhosphorExp),
    SAVE_DATA(s_NukeMushroom),
    SAVE_DATA(s_RadiationCloud),
    SAVE_DATA(s_ChemBomb),
    SAVE_DATA(s_Caltrops),
    SAVE_DATA(s_CaltropsStick),
    SAVE_DATA(s_CarryFlag),
    SAVE_DATA(s_CarryFlagNoDet),
    SAVE_DATA(s_Flag),
    SAVE_DATA(s_Phosphorus),
    SAVE_DATA(s_BloodSprayChunk),
    SAVE_DATA(s_BloodSprayDrip),
};

saveable_module saveable_jweapon =
{
    // code
    NULL,0,

    // data
    saveable_jweapon_data,
    SIZ(saveable_jweapon_data)
};
