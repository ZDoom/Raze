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
#include "break.h"
#include "network.h"
#include "pal.h"

#include "ai.h"
#include "weapon.h"

#include "sprite.h"
#include "sector.h"

BEGIN_SW_NS

ANIMATOR DoSuicide;
ANIMATOR DoBloodSpray;
void SpawnFlashBombOnActor(DSWActor* actor);

ANIMATOR DoPuff, BloodSprayFall;
extern STATE s_Puff[];
extern STATE s_FireballFlames[];
extern STATE s_GoreFloorSplash[];
extern STATE s_GoreSplash[];
extern bool GlobalSkipZrange;

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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoWallBloodDrip(DSWActor* actor)
{
    // sy & sz are the ceiling and floor of the sector you are sliding down
    if (actor->user.pos.Z != actor->user.pos.Y)
    {
        // if you are between the ceiling and floor fall fast
        if (actor->spr.pos.Z > actor->user.pos.Y && actor->spr.pos.Z < actor->user.pos.Z)
        {
            actor->vel.Z += 18.75;
            actor->spr.pos.Z += actor->vel.Z;
        }
        else
        {
            actor->vel.Z = 9.375 + RandomRangeF(71.875);
            actor->spr.pos.Z += actor->vel.Z;
        }
    }
    else
    {
		actor->vel.Z = 9.375 + RandomRangeF(71.875);
        actor->spr.pos.Z += actor->vel.Z;
    }

    if (actor->spr.pos.Z >= actor->user.loz)
    {
        actor->spr.pos.Z = actor->user.loz;
        SpawnFloorSplash(actor);
        KillActor(actor);
        return 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SpawnMidSplash(DSWActor* actor)
{
    auto actorNew = SpawnActor(STAT_MISSILE, GOREDrip, s_GoreSplash, actor->sector(),
                      DVector3(actor->spr.pos.XY(), ActorZOfMiddle(actor)), actor->spr.angle);

    actorNew->spr.shade = -12;
    actorNew->spr.xrepeat = 70-RandomRange(20);
    actorNew->spr.yrepeat = 70-RandomRange(20);
    actorNew->opos = actor->opos;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) < 512)
        actorNew->spr.cstat |= (CSTAT_SPRITE_XFLIP);

	actor->user.change.Zero();

    if (actor->user.Flags & (SPR_UNDERWATER))
        actorNew->user.Flags |= (SPR_UNDERWATER);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SpawnFloorSplash(DSWActor* actor)
{
    auto actorNew = SpawnActor(STAT_MISSILE, GOREDrip, s_GoreFloorSplash, actor->sector(), actor->spr.pos, actor->spr.angle);

    actorNew->spr.shade = -12;
    actorNew->spr.xrepeat = 70-RandomRange(20);
    actorNew->spr.yrepeat = 70-RandomRange(20);
    actorNew->opos = actor->opos;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) < 512)
        actorNew->spr.cstat |= (CSTAT_SPRITE_XFLIP);

	actor->user.change.Zero();

    if (actor->user.Flags & (SPR_UNDERWATER))
        actorNew->user.Flags |= (SPR_UNDERWATER);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoBloodSpray(DSWActor* actor)
{
    double cz,fz;

    if (actor->user.Flags & (SPR_UNDERWATER))
    {
        ScaleSpriteVector(actor, 50000);

        actor->user.Counter += 20;  // These are STAT_SKIIP4 now, so * 2
        actor->user.addCounterToChange();
    }
    else
    {
        actor->user.Counter += 20;
        actor->user.addCounterToChange();
    }

    if (actor->vel.X <= 0.125)
    {
        // special stuff for blood worm
        actor->spr.pos.Z += actor->user.change.Z * 0.5;

        getzsofslopeptr(actor->sector(), actor->spr.pos, &cz, &fz);
        // pretend like we hit a sector
        if (actor->spr.pos.Z >= fz)
        {
            actor->spr.pos.Z = fz;
            SpawnFloorSplash(actor);
            KillActor(actor);
            return true;
        }
    }
    else
    {
        actor->user.coll = move_missile(actor, actor->user.change, actor->user.ceiling_dist, actor->user.floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);
    }


    MissileHitDiveArea(actor);

    {
        switch (actor->user.coll.type)
        {
        case kHitVoid:
            KillActor(actor);
            return true;
        case kHitSprite:
        {
            auto hitActor = actor->user.coll.actor();

            if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                SpawnMidSplash(actor);
                QueueWallBlood(actor, hitActor->spr.angle);
                WallBounce(actor, hitActor->spr.angle);
                ScaleSpriteVector(actor, 32000);
            }
            else
            {
                actor->user.change.X = actor->user.change.Y = 0;
                SpawnMidSplash(actor);
                QueueWallBlood(actor, hitActor->spr.angle);
                KillActor(actor);
                return true;
            }


            break;
        }

        case kHitWall:
        {
            int hit_wall, nw;
            walltype* wph;
            int wb;

            wph = actor->user.coll.hitWall;

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, actor->spr.pos, actor->spr.angle, actor->user.ID);
                actor->user.coll.setNone();
                break;
            }

            auto wall_ang = VecToAngle(wph->delta()) - DAngle90;

            SpawnMidSplash(actor);
            auto bldActor = QueueWallBlood(actor, wall_ang);

            if (bldActor== nullptr)
            {
                KillActor(actor);
                return 0;
            }
            else
            {
                if (FAF_Sector(bldActor->sector()) || FAF_ConnectArea(bldActor->sector()))
                {
                    KillActor(actor);
                    return 0;
                }

                actor->vel.X = 0;
                actor->user.change.X = actor->user.change.Y = 0;
                actor->spr.xrepeat = actor->spr.yrepeat = 70 - RandomRange(25);
                actor->spr.pos.XY() = bldActor->spr.pos.XY();

                // !FRANK! bit of a hack
                // yvel is the hit_wall
                if (bldActor->tempwall)
                {
                    // sy & sz are the ceiling and floor of the sector you are sliding down
                    if (bldActor->tempwall->twoSided())
                        getzsofslopeptr(bldActor->tempwall->nextSector(), actor->spr.pos.X, actor->spr.pos.Y, &actor->user.pos.Y, &actor->user.pos.Z);
                    else
                        actor->user.pos.Y = actor->user.pos.Z; // ceiling and floor are equal - white wall
                }

                actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE);
                ChangeState(actor, s_BloodSprayDrip);
            }

            break;
        }

        case kHitSector:
        {
            // hit floor
            if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
            {
                if (actor->user.Flags & (SPR_UNDERWATER))
                    actor->user.Flags |= (SPR_BOUNCE);  // no bouncing
                // underwater

                if (actor->user.lo_sectp && actor->sector()->hasU() && FixedToInt(actor->sector()->depth_fixed))
                    actor->user.Flags |= (SPR_BOUNCE);  // no bouncing on
                // shallow water

#if 0
                if (!(actor->user.Flags & SPR_BOUNCE))
                {
                    SpawnFloorSplash(actor);
                    actor->user.Flags |= (SPR_BOUNCE);
                    actor->user.coll.setNone();
                    actor->user.Counter = 0;
                    actor->user.zchange = -actor->user.zchange;
                    ScaleSpriteVector(actor, 32000);   // Was 18000
                    actor->user.zchange /= 6;
                }
                else
#endif
                {
                    actor->user.change.X = actor->user.change.Y = 0;
                    SpawnFloorSplash(actor);
                    KillActor(actor);
                    return true;
                }
            }
            else
            // hit something above
            {
                actor->user.change.Z = -actor->user.change.Z;
                ScaleSpriteVector(actor, 32000);       // was 22000
            }
            break;
        }
        }
    }



    // if you haven't bounced or your going slow do some puffs
    if (!(actor->user.Flags & (SPR_BOUNCE | SPR_UNDERWATER)))
    {

        auto actorNew = SpawnActor(STAT_MISSILE, GOREDrip, s_BloodSpray, actor->sector(), actor->spr.pos, actor->spr.angle, 6.25);

        SetOwner(actor, actorNew);
        actorNew->spr.shade = -12;
        actorNew->spr.xrepeat = 40-RandomRange(30);
        actorNew->spr.yrepeat = 40-RandomRange(30);
        actorNew->opos = actor->opos;
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

        if (RANDOM_P2(1024) < 512)
            actorNew->spr.cstat |= (CSTAT_SPRITE_XFLIP);
        if (RANDOM_P2(1024) < 512)
            actorNew->spr.cstat |= (CSTAT_SPRITE_YFLIP);

		actorNew->user.change = actor->user.change;

        ScaleSpriteVector(actorNew, 20000);

        if (actor->user.Flags & (SPR_UNDERWATER))
            actorNew->user.Flags |= (SPR_UNDERWATER);
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoPhosphorus(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_UNDERWATER))
    {
        ScaleSpriteVector(actor, 50000);

        actor->user.Counter += 20*2;
        actor->user.addCounterToChange();
    }
    else
    {
        actor->user.Counter += 20*2;
        actor->user.addCounterToChange();
    }

    actor->user.coll = move_missile(actor, actor->user.change, actor->user.ceiling_dist, actor->user.floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS*2);

    MissileHitDiveArea(actor);

    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    {
        switch (actor->user.coll.type)
        {
        case kHitVoid:
            KillActor(actor);
            return true;
        case kHitSprite:
        {
            short wall_ang;

            auto hitActor = actor->user.coll.actor();

            if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                WallBounce(actor, hitActor->spr.angle);
                ScaleSpriteVector(actor, 32000);
            }
            else
            {
                if ((hitActor->spr.extra & SPRX_BURNABLE))
                {
                    if (!hitActor->hasU())
                        SpawnUser(hitActor, hitActor->spr.picnum, nullptr);
                    SpawnFireballExp(actor);
                    if (hitActor->hasU())
                        SpawnFireballFlames(actor, hitActor);
                    DoFlamesDamageTest(actor);
                }
                actor->user.change.X = actor->user.change.Y = 0;
                KillActor(actor);
                return true;
            }


            break;
        }

        case kHitWall:
        {
            walltype* wph = actor->user.coll.hitWall;

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, actor->spr.pos, actor->spr.angle, actor->user.ID);
                actor->user.coll.setNone();
                break;
            }

            WallBounce(actor, wph->delta().Angle() + DAngle90);
            ScaleSpriteVector(actor, 32000);
            break;
        }

        case kHitSector:
        {
            bool did_hit_wall;

            if (SlopeBounce(actor, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a wall
                    ScaleSpriteVector(actor, 28000);
                    actor->user.coll.setNone();
                    actor->user.Counter = 0;
                }
                else
                {
                    // hit a sector
                    if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                    {
                        // hit a floor
                        if (!(actor->user.Flags & SPR_BOUNCE))
                        {
                            actor->user.Flags |= (SPR_BOUNCE);
                            ScaleSpriteVector(actor, 32000);       // was 18000
                            actor->user.change.Z /= 6;
                            actor->user.coll.setNone();
                            actor->user.Counter = 0;
                        }
                        else
                        {
                            actor->user.change.X = actor->user.change.Y = 0;
                            SpawnFireballExp(actor);
                            KillActor(actor);
                            return true;
                        }
                    }
                    else
                    {
                        // hit a ceiling
                        ScaleSpriteVector(actor, 32000);   // was 22000
                    }
                }
            }
            else
            {
                // hit floor
                if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                {
                    if (actor->user.Flags & (SPR_UNDERWATER))
                        actor->user.Flags |= (SPR_BOUNCE);  // no bouncing
                    // underwater

                    if (actor->user.lo_sectp && actor->sector()->hasU() && FixedToInt(actor->sector()->depth_fixed))
                        actor->user.Flags |= (SPR_BOUNCE);  // no bouncing on
                    // shallow water

                    if (!(actor->user.Flags & SPR_BOUNCE))
                    {
                        actor->user.Flags |= (SPR_BOUNCE);
                        actor->user.coll.setNone();
                        actor->user.Counter = 0;
                        actor->user.change.Z = -actor->user.change.Z;
                        ScaleSpriteVector(actor, 32000);   // Was 18000
                        actor->user.change.Z /= 6;
                    }
                    else
                    {
                        actor->user.change.X = actor->user.change.Y = 0;
                        SpawnFireballExp(actor);
                        KillActor(actor);
                        return true;
                    }
                }
                else
                // hit something above
                {
                    actor->user.change.Z = -actor->user.change.Z;
                    ScaleSpriteVector(actor, 32000);       // was 22000
                }
            }
            break;
        }
        }
    }



    // if you haven't bounced or your going slow do some puffs
    if (!(actor->user.Flags & (SPR_BOUNCE | SPR_UNDERWATER)) && !(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
    {

        auto actorNew = SpawnActor(STAT_SKIP4, PUFF, s_PhosphorExp, actor->sector(), actor->spr.pos, actor->spr.angle, 6.25);

        actorNew->spr.hitag = LUMINOUS;           // Always full brightness
        SetOwner(actor, actorNew);
        actorNew->spr.shade = -40;
        actorNew->spr.xrepeat = 12 + RandomRange(10);
        actorNew->spr.yrepeat = 12 + RandomRange(10);
        actorNew->opos = actor->opos;
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

        if (RANDOM_P2(1024) < 512)
            actorNew->spr.cstat |= (CSTAT_SPRITE_XFLIP);
        if (RANDOM_P2(1024) < 512)
            actorNew->spr.cstat |= (CSTAT_SPRITE_YFLIP);

		actorNew->user.change = actor->user.change;

        actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER3;   // RED

        ScaleSpriteVector(actorNew, 20000);

        if (actor->user.Flags & (SPR_UNDERWATER))
            actorNew->user.Flags |= (SPR_UNDERWATER);
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoChemBomb(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_UNDERWATER))
    {
        ScaleSpriteVector(actor, 50000);

        actor->user.Counter += 20;
        actor->user.addCounterToChange();
    }
    else
    {
        actor->user.Counter += 20;
        actor->user.addCounterToChange();
    }

    actor->user.coll = move_missile(actor, actor->user.change, actor->user.ceiling_dist, actor->user.floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    {
        switch (actor->user.coll.type)
        {
        case kHitVoid:
            KillActor(actor);
            return true;
        case kHitSprite:
        {
            short wall_ang;

            if (!(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                PlaySound(DIGI_CHEMBOUNCE, actor, v3df_dontpan);

            auto hitActor = actor->user.coll.actor();

            if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                WallBounce(actor, hitActor->spr.angle);
                ScaleSpriteVector(actor, 32000);
            }
            else
            {
                // Canister pops when first smoke starts out
                if (actor->user.WaitTics == CHEMTICS && !(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                {
                    PlaySound(DIGI_GASPOP, actor, v3df_dontpan | v3df_doppler);
                    PlaySound(DIGI_CHEMGAS, actor, v3df_dontpan | v3df_doppler);
                }
                actor->user.change.X = actor->user.change.Y = 0;
                actor->user.WaitTics -= (MISSILEMOVETICS * 2);
                if (actor->user.WaitTics <= 0)
                    KillActor(actor);
                return true;
            }


            break;
        }

        case kHitWall:
        {
            auto wph = actor->user.coll.hitWall;

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, actor->spr.pos, actor->spr.angle, actor->user.ID);
                actor->user.coll.setNone();
                break;
            }

            if (!(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                PlaySound(DIGI_CHEMBOUNCE, actor, v3df_dontpan);

			WallBounce(actor, wph->delta().Angle() + DAngle90);
            ScaleSpriteVector(actor, 32000);
            break;
        }

        case kHitSector:
        {
            bool did_hit_wall;

            if (SlopeBounce(actor, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a wall
                    ScaleSpriteVector(actor, 28000);
                    actor->user.coll.setNone();
                    actor->user.Counter = 0;
                }
                else
                {
                    // hit a sector
                    if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                    {
                        // hit a floor
                        if (!(actor->user.Flags & SPR_BOUNCE))
                        {
                            if (!(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                                PlaySound(DIGI_CHEMBOUNCE, actor, v3df_dontpan);
                            actor->user.Flags |= (SPR_BOUNCE);
                            ScaleSpriteVector(actor, 32000);       // was 18000
                            actor->user.change.Z /= 6;
                            actor->user.coll.setNone();
                            actor->user.Counter = 0;
                        }
                        else
                        {
                            // Canister pops when first smoke starts out
                            if (actor->user.WaitTics == CHEMTICS && !(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                            {
                                PlaySound(DIGI_GASPOP, actor, v3df_dontpan | v3df_doppler);
                                PlaySound(DIGI_CHEMGAS, actor, v3df_dontpan | v3df_doppler);
                            }
                            SpawnRadiationCloud(actor);
                            actor->user.change.X = actor->user.change.Y = 0;
                            actor->user.WaitTics -= (MISSILEMOVETICS * 2);
                            if (actor->user.WaitTics <= 0)
                                KillActor(actor);
                            return true;
                        }
                    }
                    else
                    {
                        // hit a ceiling
                        ScaleSpriteVector(actor, 32000);   // was 22000
                    }
                }
            }
            else
            {
                // hit floor
                if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                {
                    if (actor->user.Flags & (SPR_UNDERWATER))
                        actor->user.Flags |= (SPR_BOUNCE);  // no bouncing
                    // underwater

                    if (actor->user.lo_sectp && actor->sector()->hasU() && FixedToInt(actor->sector()->depth_fixed))
                        actor->user.Flags |= (SPR_BOUNCE);  // no bouncing on
                    // shallow water

                    if (!(actor->user.Flags & SPR_BOUNCE))
                    {
                        if (!(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                            PlaySound(DIGI_CHEMBOUNCE, actor, v3df_dontpan);
                        actor->user.Flags |= (SPR_BOUNCE);
                        actor->user.coll.setNone();
                        actor->user.Counter = 0;
                        actor->user.change.Z = -actor->user.change.Z;
                        ScaleSpriteVector(actor, 32000);   // Was 18000
                        actor->user.change.Z /= 6;
                    }
                    else
                    {
                        // Canister pops when first smoke starts out
                        if (actor->user.WaitTics == CHEMTICS && !(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                        {
                            PlaySound(DIGI_GASPOP, actor, v3df_dontpan | v3df_doppler);
                            PlaySound(DIGI_CHEMGAS, actor, v3df_dontpan | v3df_doppler);
                        }
                        SpawnRadiationCloud(actor);
                        actor->user.change.X = actor->user.change.Y = 0;
                        actor->user.WaitTics -= (MISSILEMOVETICS * 2);
                        if (actor->user.WaitTics <= 0)
                            KillActor(actor);
                        return true;
                    }
                }
                else
                // hit something above
                {
                    actor->user.change.Z = -actor->user.change.Z;
                    ScaleSpriteVector(actor, 32000);       // was 22000
                }
            }
            break;
        }
        }
    }

    // if you haven't bounced or your going slow do some puffs
    if (!(actor->user.Flags & (SPR_BOUNCE | SPR_UNDERWATER)) && !(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
    {
        auto actorNew = SpawnActor(STAT_MISSILE, PUFF, s_Puff, actor->sector(), actor->spr.pos, actor->spr.angle, 6.25);

        SetOwner(actor, actorNew);
        actorNew->spr.shade = -40;
        actorNew->spr.xrepeat = 40;
        actorNew->spr.yrepeat = 40;
        actorNew->opos = actor->opos;
        // !Frank - dont do translucent
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        // actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER|CSTAT_SPRITE_TRANSLUCENT);
        actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

		actorNew->user.change = actor->user.change;
		
        actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER6;

        ScaleSpriteVector(actorNew, 20000);

        if (actor->user.Flags & (SPR_UNDERWATER))
            actorNew->user.Flags |= (SPR_UNDERWATER);
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoCaltropsStick(DSWActor* actor)
{
    actor->user.Counter = !actor->user.Counter;

    if (actor->user.Counter)
        DoFlamesDamageTest(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoCaltrops(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_UNDERWATER))
    {
        ScaleSpriteVector(actor, 50000);

        actor->user.Counter += 20;
        actor->user.addCounterToChange();
    }
    else
    {
        actor->user.Counter += 70;
        actor->user.addCounterToChange();
    }

    actor->user.coll = move_missile(actor, actor->user.change, actor->user.ceiling_dist, actor->user.floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    {
        switch (actor->user.coll.type)
        {
        case kHitVoid:
            KillActor(actor);
            return true;
        case kHitSprite:
        {
            short wall_ang;

            PlaySound(DIGI_CALTROPS, actor, v3df_dontpan);

            auto hitActor = actor->user.coll.actor();

            if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                WallBounce(actor, hitActor->spr.angle);
                ScaleSpriteVector(actor, 10000);
            }
            else
            {
                // fall to the ground
                actor->user.change.X = actor->user.change.Y = 0;
            }


            break;
        }

        case kHitWall:
        {
            auto wph = actor->user.coll.hitWall;

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, actor->spr.pos, actor->spr.angle, actor->user.ID);
                actor->user.coll.setNone();
                break;
            }

            PlaySound(DIGI_CALTROPS, actor, v3df_dontpan);

			WallBounce(actor, wph->delta().Angle() + DAngle90);
            ScaleSpriteVector(actor, 1000);
            break;
        }

        case kHitSector:
        {
            bool did_hit_wall;

            if (SlopeBounce(actor, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a wall
                    ScaleSpriteVector(actor, 1000);
                    actor->user.coll.setNone();
                    actor->user.Counter = 0;
                }
                else
                {
                    // hit a sector
                    if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                    {
                        // hit a floor
                        if (!(actor->user.Flags & SPR_BOUNCE))
                        {
                            PlaySound(DIGI_CALTROPS, actor, v3df_dontpan);
                            actor->user.Flags |= (SPR_BOUNCE);
                            ScaleSpriteVector(actor, 1000);        // was 18000
                            actor->user.coll.setNone();
                            actor->user.Counter = 0;
                        }
                        else
                        {
                            actor->user.change.X = actor->user.change.Y = 0;
                            actor->spr.extra |= (SPRX_BREAKABLE);
                            actor->spr.cstat |= (CSTAT_SPRITE_BREAKABLE);
                            ChangeState(actor, s_CaltropsStick);
                            return true;
                        }
                    }
                    else
                    {
                        // hit a ceiling
                        ScaleSpriteVector(actor, 1000);    // was 22000
                    }
                }
            }
            else
            {
                // hit floor
                if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                {
                    if (actor->user.Flags & (SPR_UNDERWATER))
                        actor->user.Flags |= (SPR_BOUNCE);  // no bouncing
                    // underwater

                    if (actor->user.lo_sectp && actor->sector()->hasU() && FixedToInt(actor->sector()->depth_fixed))
                        actor->user.Flags |= (SPR_BOUNCE);  // no bouncing on
                    // shallow water

                    if (!(actor->user.Flags & SPR_BOUNCE))
                    {
                        PlaySound(DIGI_CALTROPS, actor, v3df_dontpan);
                        actor->user.Flags |= (SPR_BOUNCE);
                        actor->user.coll.setNone();
                        actor->user.Counter = 0;
                        actor->user.change.Z = -actor->user.change.Z;
                        ScaleSpriteVector(actor, 1000);    // Was 18000
                    }
                    else
                    {
                        actor->user.change.X = actor->user.change.Y = 0;
                        actor->spr.extra |= (SPRX_BREAKABLE);
                        actor->spr.cstat |= (CSTAT_SPRITE_BREAKABLE);
                        ChangeState(actor, s_CaltropsStick);
                        return true;
                    }
                }
                else
                // hit something above
                {
                    actor->user.change.Z = -actor->user.change.Z;
                    ScaleSpriteVector(actor, 1000);        // was 22000
                }
            }
            break;
        }
        }
    }


    return false;
}

/////////////////////////////
//
// Deadly green gas clouds
//
/////////////////////////////

int SpawnRadiationCloud(DSWActor* actor)
{
    if (!MoveSkip4)
        return false;

    // This basically works like a MoveSkip8, if one existed
//  actor->user.Counter2 = !actor->user.Counter2;
    if (actor->user.ID == MUSHROOM_CLOUD || actor->user.ID == 3121)
    {
        if ((actor->user.Counter2++) > 16)
            actor->user.Counter2 = 0;
        if (actor->user.Counter2)
            return false;
    }
    else
    {
        if ((actor->user.Counter2++) > 2)
            actor->user.Counter2 = 0;
        if (actor->user.Counter2)
            return false;
    }

    if (actor->user.Flags & (SPR_UNDERWATER))
        return -1;

    auto actorNew = SpawnActor(STAT_MISSILE, RADIATION_CLOUD, s_RadiationCloud, actor->sector(),
							   actor->spr.pos.plusZ(-RANDOM_P2F(8, 8)), actor->spr.angle);

    SetOwner(GetOwner(actor), actorNew);
    actorNew->user.WaitTics = 1 * 120;
    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = 32;
    actorNew->spr.yrepeat = 32;
    actorNew->spr.clipdist = actor->spr.clipdist;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER6;
    // Won't take floor palettes
    actorNew->spr.hitag = SECTFU_DONT_COPY_PALETTE;

    if (RANDOM_P2(1024) < 512)
        actorNew->spr.cstat |= (CSTAT_SPRITE_XFLIP);
    //if (RANDOM_P2(1024) < 512)
    //actorNew->spr.cstat |= (CSTAT_SPRITE_YFLIP);

    actorNew->spr.angle = RandomAngle();
    actorNew->vel.X = RandomRangeF(2);

    actorNew->user.Counter = 0;
    actorNew->user.Counter2 = 0;

    if (actor->user.ID == MUSHROOM_CLOUD || actor->user.ID == 3121)
    {
        actorNew->user.Radius = 2000;
        actorNew->user.change.XY() = actorNew->spr.angle.ToVector() * actorNew->vel.X * 0.25;
		actorNew->vel.Z = 1 + RandomRangeF(2);
    }
    else
    {
		UpdateChangeXY(actorNew);
        actorNew->vel.Z = 4 + RandomRangeF(4);
        actorNew->user.Radius = 4000;
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoRadiationCloud(DSWActor* actor)
{
	actor->spr.pos.XY() += actor->user.change;
    actor->spr.pos.Z -= actor->vel.Z;

    if (actor->user.ID)
    {
        DoFlamesDamageTest(actor);
    }

    return false;
}

//////////////////////////////////////////////
//
// Inventory Chemical Bombs
//
//////////////////////////////////////////////
int PlayerInitChemBomb(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    short oclipdist;


    PlaySound(DIGI_THROW, pp, v3df_dontpan | v3df_doppler);

    if (!pp->insector())
        return 0;

    auto pos = pp->pos.plusZ(pp->bob_z + 8);

    // Spawn a shot
    // Inserting and setting up variables
    auto actorNew = SpawnActor(STAT_MISSILE, CHEMBOMB, s_ChemBomb, pp->cursector, pos, pp->angle.ang, CHEMBOMB_VELOCITY);

    // don't throw it as far if crawling
    if (pp->Flags & (PF_CRAWLING))
    {
        actorNew->vel.X *= 0.75;
    }

//    actorNew->user.RotNum = 5;
    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 32;
    actorNew->spr.xrepeat = 32;
    actorNew->spr.shade = -15;
    actorNew->user.WeaponNum = plActor->user.WeaponNum;
    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = 3;
    actorNew->user.floor_dist = 3;
    actorNew->user.Counter = 0;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK);

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    actorNew->vel.Z -= pp->horizon.horiz.asbuildf() * HORIZ_MULTF;

    oclipdist = plActor->spr.clipdist;
    plActor->spr.clipdist = 0;
    actorNew->spr.clipdist = 0;

    MissileSetPos(actorNew, DoChemBomb, 1000);

    plActor->spr.clipdist = uint8_t(oclipdist);
    actorNew->spr.clipdist = 80 >> 2;

	UpdateChange(actorNew, 0.5);

    // adjust xvel according to player velocity
	actorNew->user.change += pp->vect;

    // Smoke will come out for this many seconds
    actorNew->user.WaitTics = CHEMTICS;

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int InitSpriteChemBomb(DSWActor* actor)
{
    PlaySound(DIGI_THROW, actor, v3df_dontpan | v3df_doppler);

    // Spawn a shot
    // Inserting and setting up variables
    auto actorNew = SpawnActor(STAT_MISSILE, CHEMBOMB, s_ChemBomb, actor->sector(),
                   actor->spr.pos, actor->spr.angle, CHEMBOMB_VELOCITY);

    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);

    SetOwner(actor, actorNew);
    actorNew->spr.yrepeat = 32;
    actorNew->spr.xrepeat = 32;
    actorNew->spr.shade = -15;
    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = 3;
    actorNew->user.floor_dist = 3;
    actorNew->user.Counter = 0;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK);

	actorNew->vel.Z = (-100 - RandomRange(100)) * HORIZ_MULTF;

    actorNew->spr.clipdist = 80 >> 2;

	UpdateChange(actorNew, 0.5);

    // Smoke will come out for this many seconds
    actorNew->user.WaitTics = CHEMTICS;

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int InitChemBomb(DSWActor* actor)
{
    // Spawn a shot
    // Inserting and setting up variables
    auto actorNew = SpawnActor(STAT_MISSILE, MUSHROOM_CLOUD, s_ChemBomb, actor->sector(), actor->spr.pos, actor->spr.angle, CHEMBOMB_VELOCITY);

    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);

    SetOwner(GetOwner(actor), actorNew);
    actorNew->spr.yrepeat = 32;
    actorNew->spr.xrepeat = 32;
    actorNew->spr.shade = -15;
    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = 3;
    actorNew->user.floor_dist = 3;
    actorNew->user.Counter = 0;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_INVISIBLE);      // Make nuke radiation
    // invis.
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);

    if (SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

	actorNew->vel.Z = (-100 - RandomRange(100)) * HORIZ_MULTF;
    actorNew->spr.clipdist = 0;

    if (actor->user.ID == MUSHROOM_CLOUD || actor->user.ID == 3121 || actor->user.ID == SUMO_RUN_R0) // 3121 == GRENADE_EXP
    {
		actor->user.change.Zero();
        actorNew->vel.X = 0;
        actorNew->vel.Z = 0;
        // Smoke will come out for this many seconds
        actorNew->user.WaitTics = 40*120;
    }
    else
    {
		UpdateChange(actorNew, 0.5);
        // Smoke will come out for this many seconds
        actorNew->user.WaitTics = 3*120;
    }


    return 0;
}

//---------------------------------------------------------------------------
//
// Inventory Flash Bombs
//
//---------------------------------------------------------------------------

int PlayerInitFlashBomb(PLAYER* pp)
{
    unsigned int stat;

    short damage;
    DSWActor* actor = pp->actor;

    PlaySound(DIGI_GASPOP, pp, v3df_dontpan | v3df_doppler);

    // Set it just a little to let player know what he just did
    SetFadeAmt(pp, -30, 1);             // White flash

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == pp->actor)
                break;

			double dist = (itActor->spr.pos.XY() - actor->spr.pos.XY()).Length();
            if (dist > 1024)           // Flash radius
                continue;

            if (!(actor->spr.cstat & CSTAT_SPRITE_BLOCK))
                continue;

            if (!FAFcansee(itActor->spr.pos, itActor->sector(), actor->spr.pos.plusZ(-ActorSizeZ(actor)), actor->sector()))
                continue;

            damage = GetDamage(itActor, pp->actor, DMG_FLASHBOMB);

            if (itActor->user.sop_parent)
            {
                break;
            }
            else if (itActor->user.PlayerP)
            {
//              if(itActor->user.PlayerP->NightVision)
//              {
//                  SetFadeAmt(itActor->user.PlayerP, -200, 1); // Got him with night vision on!
//                  PlayerUpdateHealth(itActor->user.PlayerP, -15); // Hurt eyes
//              }else
                if (damage < -70)
                {
                    int choosesnd = 0;

                    choosesnd = RandomRange(MAX_PAIN);

                    PlayerSound(PlayerLowHealthPainVocs[choosesnd],v3df_dontpan|v3df_doppler|v3df_follow,pp);
                }
                SetFadeAmt(itActor->user.PlayerP, damage, 1);     // White flash
            }
            else
            {
                ActorPain(itActor);
                SpawnFlashBombOnActor(itActor);
            }
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int InitFlashBomb(DSWActor* actor)
{
    int i;
    unsigned int stat;
    short damage;
    PLAYER* pp = Player + screenpeek;

    PlaySound(DIGI_GASPOP, actor, v3df_dontpan | v3df_doppler);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            double dist = (itActor->spr.pos.XY() - actor->spr.pos.XY()).Length();
            if (dist > 1024)           // Flash radius
                continue;

            if (!(actor->spr.cstat & CSTAT_SPRITE_BLOCK))
                continue;

            if (!FAFcansee(itActor->spr.pos, itActor->sector(), actor->spr.pos.plusZ(-ActorSizeZ(actor)), actor->sector()))
                continue;

            damage = GetDamage(itActor, actor, DMG_FLASHBOMB);

            if (itActor->user.sop_parent)
            {
                break;
            }
            else if (itActor->user.PlayerP)
            {
                if (damage < -70)
                {
                    int choosesnd = 0;

                    choosesnd = RandomRange(MAX_PAIN);

                    PlayerSound(PlayerLowHealthPainVocs[choosesnd],v3df_dontpan|v3df_doppler|v3df_follow,pp);
                }
                SetFadeAmt(itActor->user.PlayerP, damage, 1);     // White flash
            }
            else
            {
                if (itActor != actor)
                {
                    ActorPain(itActor);
                    SpawnFlashBombOnActor(itActor);
                }
            }
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// This is a sneaky function to make actors look blinded by flashbomb while using flaming code
//
//---------------------------------------------------------------------------

void SpawnFlashBombOnActor(DSWActor* actor)
{
    if (!actor->hasU()) return;

    // Forget about burnable sprites
    if ((actor->spr.extra & SPRX_BURNABLE))
        return;

    if (actor != nullptr)
    {
        DSWActor* flameActor = actor->user.flameActor;
        if (flameActor != nullptr)
        {
            double sizez = ActorSizeZ(actor) * 1.25;

            if (flameActor->user.Counter >= GetRepeatFromHeight(flameActor, sizez))
            {
                // keep flame only slightly bigger than the enemy itself
                flameActor->user.Counter = GetRepeatFromHeight(flameActor, sizez) * 2;
            }
            else
            {
                // increase max size
                flameActor->user.Counter += GetRepeatFromHeight(flameActor, 8) * 2;
            }

            // Counter is max size
            if (flameActor->user.Counter >= 230)
            {
                // this is far too big
                flameActor->user.Counter = 230;
            }

            if (flameActor->user.WaitTics < 2 * 120)
                flameActor->user.WaitTics = 2 * 120; // allow it to grow again

            return;
        }
    }

    auto actorNew = SpawnActor(STAT_MISSILE, FIREBALL_FLAMES, s_FireballFlames, actor->sector(),
							   actor->spr.pos, actor->spr.angle);

    if (actor != nullptr)
        actor->user.flameActor = actorNew;

    actorNew->spr.xrepeat = 16;
    actorNew->spr.yrepeat = 16;

    if (actor->user.flameActor != nullptr)
    {
        actorNew->user.Counter = GetRepeatFromHeight(actorNew, ActorSizeZ(actor) * 0.5) * 4;
    }
    else
        actorNew->user.Counter = 0;                // max flame size

    actorNew->spr.shade = -40;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_INVISIBLE);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    actorNew->user.Radius = 200;

    if (actor->user.flameActor != nullptr)
    {
        SetAttach(actor, actorNew);
    }

    return;
}

//---------------------------------------------------------------------------
//
// Inventory Caltrops
//
//---------------------------------------------------------------------------

int PlayerInitCaltrops(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    short oclipdist;

    PlaySound(DIGI_THROW, pp, v3df_dontpan | v3df_doppler);

    if (!pp->insector())
        return 0;

    auto pos = pp->pos.plusZ(pp->bob_z + 8);

    auto actorNew = SpawnActor(STAT_DEAD_ACTOR, CALTROPS, s_Caltrops, pp->cursector, pos, pp->angle.ang, (CHEMBOMB_VELOCITY + RandomRangeF(CHEMBOMB_VELOCITY)) / 2);

    // don't throw it as far if crawling
    if (pp->Flags & (PF_CRAWLING))
    {
		actorNew->vel.X *= 0.75;
    }

    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 64;
    actorNew->spr.xrepeat = 64;
    actorNew->spr.shade = -15;
    actorNew->user.WeaponNum = plActor->user.WeaponNum;
    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = 3;
    actorNew->user.floor_dist = 3;
    actorNew->user.Counter = 0;
//      spawnedActor->spr.cstat |= (CSTAT_SPRITE_BLOCK);

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    actorNew->vel.Z -= pp->horizon.horiz.asbuildf() * 0.5;

    oclipdist = plActor->spr.clipdist;
    plActor->spr.clipdist = 0;
    actorNew->spr.clipdist = 0;

    MissileSetPos(actorNew, DoCaltrops, 1000);

    plActor->spr.clipdist = uint8_t(oclipdist);
    actorNew->spr.clipdist = 80L >> 2;

	UpdateChange(actorNew, 0.5);

    // adjust xvel according to player velocity
    actorNew->user.change += pp->vect;

    SetupSpriteForBreak(actorNew);            // Put Caltrops in the break queue
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int InitCaltrops(DSWActor* actor)
{
    PlaySound(DIGI_THROW, actor, v3df_dontpan | v3df_doppler);

    // Spawn a shot
    // Inserting and setting up variables
    auto actorNew = SpawnActor(STAT_DEAD_ACTOR, CALTROPS, s_Caltrops, actor->sector(),
							   actor->spr.pos, actor->spr.angle, CHEMBOMB_VELOCITY / 2);

    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);

    SetOwner(actor, actorNew);
    actorNew->spr.yrepeat = 64;
    actorNew->spr.xrepeat = 64;
    actorNew->spr.shade = -15;
    // !FRANK - clipbox must be <= weapon otherwise can clip thru walls
    actorNew->spr.clipdist = actor->spr.clipdist;
    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = 3;
    actorNew->user.floor_dist = 3;
    actorNew->user.Counter = 0;
	actorNew->vel.Z = (-100 - RandomRange(100)) * HORIZ_MULTF;
	
	UpdateChange(actorNew, 0.5);

    SetupSpriteForBreak(actorNew);            // Put Caltrops in the break queue
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int InitPhosphorus(DSWActor* actor)
{
    PlaySound(DIGI_FIREBALL1, actor, v3df_follow);

    // Spawn a shot
    // Inserting and setting up variables
    auto actorNew = SpawnActor(STAT_SKIP4, FIREBALL1, s_Phosphorus, actor->sector(),
							   actor->spr.pos, RandomAngle(), CHEMBOMB_VELOCITY/3);

    actorNew->spr.hitag = LUMINOUS;               // Always full brightness
    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);
    // !Frank - don't do translucent
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    // actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT|CSTAT_SPRITE_YCENTER);
    actorNew->spr.shade = -128;

    actorNew->spr.yrepeat = 64;
    actorNew->spr.xrepeat = 64;
    actorNew->spr.shade = -15;
    // !FRANK - clipbox must be <= weapon otherwise can clip thru walls
    if (actor->spr.clipdist > 0)
        actorNew->spr.clipdist = actor->spr.clipdist-1;
    else
        actorNew->spr.clipdist = actor->spr.clipdist;
    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 600;
    actorNew->user.ceiling_dist = 3;
    actorNew->user.floor_dist = 3;
    actorNew->user.Counter = 0;
	actorNew->vel.Z = (-100 - RandomRange(100)) * HORIZ_MULTF;

	UpdateChange(actorNew, 0.5);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int InitBloodSpray(DSWActor* actor, bool dogib, short velocity)
{
    short cnt, vel, rnd;


    if (dogib)
        cnt = RandomRange(3)+1;
    else
        cnt = 1;

    //if(dogib)
    //    {
    rnd = RandomRange(1000);
    if (rnd > 650)
        PlaySound(DIGI_GIBS1, actor, v3df_none);
    else if (rnd > 350)
        PlaySound(DIGI_GIBS2, actor, v3df_none);
    else
        PlaySound(DIGI_GIBS3, actor, v3df_none);
    //    }

    vel = velocity;
	DAngle ang = actor->spr.angle;
    for (int i=0; i<cnt; i++)
    {

        if (velocity == -1)
            vel = 105+RandomRange(320);
        else if (velocity == -2)
            vel = 105+RandomRange(100);

        if (dogib)
            ang = ang + DAngle90 + DAngle::fromBuild(RandomRange(200));
        else
            ang = ang + DAngle180 + DAngle45 - RandomAngle(45);

        // Spawn a shot
        auto actorNew = SpawnActor(STAT_MISSILE, GOREDrip, s_BloodSprayChunk, actor->sector(),
                        ActorVectOfTop(actor).plusZ(- 20/256.), ang, vel / 8.);

        actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);
        if (dogib)
            actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        else
            actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_INVISIBLE);
        actorNew->spr.shade = -12;

        SetOwner(actor, actorNew);
        actorNew->spr.yrepeat = 64-RandomRange(35);
        actorNew->spr.xrepeat = 64-RandomRange(35);
        actorNew->spr.shade = -15;
        actorNew->spr.clipdist = actor->spr.clipdist;
        actorNew->user.WeaponNum = actor->user.WeaponNum;
        actorNew->user.Radius = 600;
        actorNew->user.ceiling_dist = 3;
        actorNew->user.floor_dist = 3;
        actorNew->user.Counter = 0;

        actorNew->vel.Z = (-10 - RandomRange(50)) * HORIZ_MULTF;

		UpdateChange(actorNew, 0.5);

        if (!GlobalSkipZrange)
            DoActorZrange(actorNew);
    }

    return 0;
}

int BloodSprayFall(DSWActor* actor)
{
	actor->spr.pos.Z += 5.86;
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
void DoFlagScore(int16_t pal)
{
    SWStatIterator it(STAT_DEFAULT);
    while (auto actor = it.Next())
    {
        if (actor->spr.picnum < 1900 || actor->spr.picnum > 1999)
            continue;

        if (actor->spr.pal == pal)
            actor->spr.picnum++;               // Increment the counter

        if (actor->spr.picnum > 1999)
            actor->spr.picnum = 1900;          // Roll it over if you must

    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DSWActor* DoFlagRangeTest(DSWActor* actor, double range)
{
    unsigned int stat;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            double dist = (itActor->spr.pos - actor->spr.pos).LengthSquared();
            if (dist > range * range)
                continue;

            if (actor == itActor)
                continue;

            if (!(itActor->spr.cstat & CSTAT_SPRITE_BLOCK))
                continue;

            if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                continue;

            if (!FAFcansee(itActor->spr.pos, itActor->sector(), actor->spr.pos, actor->sector()))
                continue;

            return itActor;
        }
    }

    return nullptr;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoCarryFlag(DSWActor* actor)
{
    const int FLAG_DETONATE_STATE = 99;
    DSWActor* fown = actor->user.flagOwnerActor;
    if (!fown) return 0;

    DSWActor* attached = actor->user.attachActor;

    // if no Owner then die
    if (attached != nullptr)
    {
        DVector3 pos(attached->spr.pos.XY(), ActorZOfMiddle(attached));
        SetActorZ(actor, pos);
		actor->spr.angle = attached->spr.angle + DAngle270;
    }

    // not activated yet
    if (!(actor->user.Flags & SPR_ACTIVE))
    {
        if ((actor->user.WaitTics -= (MISSILEMOVETICS * 2)) > 0)
            return false;

        // activate it
        actor->user.WaitTics = SEC(30);          // You have 30 seconds to get it to
        // scorebox
        actor->user.Counter2 = 0;
        actor->user.Flags |= (SPR_ACTIVE);
    }

    // limit the number of times DoFlagRangeTest is called
    actor->user.Counter++;
    if (actor->user.Counter > 1)
        actor->user.Counter = 0;

    if (!actor->user.Counter)
    {
        // not already in detonate state
        if (actor->user.Counter2 < FLAG_DETONATE_STATE)
        {
            if (!attached->hasU() || attached->user.Health <= 0)
            {
                actor->user.Counter2 = FLAG_DETONATE_STATE;
                actor->user.WaitTics = SEC(1) / 2;
            }
            // if in score box, score.
            if (attached->sector()->hitag == 9000 && attached->sector()->lotag == attached->spr.pal
                && attached->spr.pal != actor->spr.pal)
            {
                if (fown != nullptr)
                {
                    if (fown->spr.lotag)      // Trigger everything if there is a lotag
                        DoMatchEverything(nullptr, fown->spr.lotag, 1);
                }
                if (!TEST_BOOL1(fown))
                {
                    PlaySound(DIGI_BIGITEM, actor->user.attachActor, v3df_none);
                    DoFlagScore(attached->spr.pal);
                    if (SP_TAG5(fown) > 0)
                    {
                        fown->user.filler++;
                        if (fown->user.filler >= SP_TAG5(fown))
                        {
                            fown->user.filler = 0;
                            DoMatchEverything(nullptr, SP_TAG6(fown), 1);
                        }
                    }
                }
                SetSuicide(actor);     // Kill the flag, you scored!
            }
        }
        else
        {
            // Time's up! Move directly to detonate state
            actor->user.Counter2 = FLAG_DETONATE_STATE;
            actor->user.WaitTics = SEC(1) / 2;
        }

    }

    actor->user.WaitTics -= (MISSILEMOVETICS * 2);

    switch (actor->user.Counter2)
    {
    case 0:
        if (actor->user.WaitTics < SEC(30))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 1:
        if (actor->user.WaitTics < SEC(20))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 2:
        if (actor->user.WaitTics < SEC(10))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 3:
        if (actor->user.WaitTics < SEC(5))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 4:
        if (actor->user.WaitTics < SEC(4))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 5:
        if (actor->user.WaitTics < SEC(3))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 6:
        if (actor->user.WaitTics < SEC(2))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2 = FLAG_DETONATE_STATE;
        }
        break;
    case FLAG_DETONATE_STATE:
        // start frantic beeping
        PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
        actor->user.Counter2++;
        break;
    case FLAG_DETONATE_STATE + 1:
        SpawnGrenadeExp(actor);
        SetSuicide(actor);
        return false;
        break;
    }

    return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoCarryFlagNoDet(DSWActor* actor)
{
    DSWActor* attached = actor->user.attachActor;
    DSWActor* fown = actor->user.flagOwnerActor;
    if (!fown) return 0;


    if (actor->user.flagOwnerActor != nullptr)
        fown->user.WaitTics = 30 * 120;        // Keep setting respawn tics so it won't respawn

    // if no Owner then die
    if (attached != nullptr)
    {
        SetActorZ(actor, DVector3(attached->spr.pos.XY(), ActorZOfMiddle(attached)));
		actor->spr.angle = attached->spr.angle + DAngle270;
        actor->spr.pos.Z = attached->spr.pos.Z - (ActorSizeZ(attached) * 0.5);
    }

    if (!attached->hasU() || attached->user.Health <= 0)
    {
        if (actor->user.flagOwnerActor != nullptr)
            fown->user.WaitTics = 0;           // Tell it to respawn
        SetSuicide(actor);
        return false;
    }

    // if in score box, score.
    if (attached->sector()->hitag == 9000 && attached->sector()->lotag == attached->spr.pal
        && attached->spr.pal != actor->spr.pal)
    {
        if (actor->user.flagOwnerActor != nullptr)
        {
            if (fown->spr.lotag)              // Trigger everything if there is a lotag
                DoMatchEverything(nullptr, fown->spr.lotag, 1);
            fown->user.WaitTics = 0;           // Tell it to respawn
        }
        if (!TEST_BOOL1(fown))
        {
            PlaySound(DIGI_BIGITEM, actor->user.attachActor, v3df_none);
            DoFlagScore(attached->spr.pal);
            if (SP_TAG5(fown) > 0)
            {
                fown->user.filler++;
                if (fown->user.filler >= SP_TAG5(fown))
                {
                    fown->user.filler = 0;
                    DoMatchEverything(nullptr, SP_TAG6(fown), 1);
                }
            }
        }
        SetSuicide(actor);             // Kill the flag, you scored!
    }

    return false;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetCarryFlag(DSWActor* actor)
{
    // stuck
    actor->user.Flags |= (SPR_BOUNCE);
    // not yet active for 1 sec

    actor->spr.cstat |= (CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    actor->user.Counter = 0;
    change_actor_stat(actor, STAT_ITEM);
    if (actor->spr.hitag == 1)
        ChangeState(actor, s_CarryFlagNoDet);
    else
        ChangeState(actor, s_CarryFlag);

    return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoFlag(DSWActor* actor)
{
    auto hitActor = DoFlagRangeTest(actor, 62.5);

    if (hitActor)
    {
        SetCarryFlag(actor);

        // check to see if sprite is player or enemy
        if ((hitActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
        {
            // attach weapon to sprite
            actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            SetAttach(hitActor, actor);
            actor->user.pos.Z = hitActor->spr.pos.Z - ActorSizeZ(hitActor) * 0.5;
        }
    }

    return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SpawnShell(DSWActor* actor, int ShellNum)
{
    short id = 0;
    double velocity = 0;
    STATE* p=nullptr;
    extern STATE s_UziShellShrap[];
    extern STATE s_ShotgunShellShrap[];

    switch (ShellNum)
    {
    case -2:
    case -3:
        id = UZI_SHELL;
        p = s_UziShellShrap;
        velocity = 5.85 + RandomRangeF(3.9);
        break;
    case -4:
        id = SHOT_SHELL;
        p = s_ShotgunShellShrap;
        velocity = 7.8 + RandomRangeF(3.9);
        break;
    }

    auto actorNew = SpawnActor(STAT_SKIP4, id, p, actor->sector(), ActorVectOfMiddle(actor), actor->spr.angle, 4);

    actorNew->vel.Z = -velocity;

    if (actor->user.PlayerP)
    {
		actorNew->vel.Z = -actor->user.PlayerP->horizon.horiz.asbuildf() * HORIZ_MULTF * (1. / 3.);
    }

    switch (actorNew->user.ID)
    {
    case UZI_SHELL:
		actorNew->spr.pos.Z -= 13;

        if (ShellNum == -3)
        {
            actorNew->spr.angle = actor->spr.angle;
            HelpMissileLateral(actorNew,2500);
            actorNew->spr.angle -= DAngle90;
            HelpMissileLateral(actorNew,1000); // Was 1500
			actorNew->spr.angle += DAngle::fromBuild(712); // somewhat weird number...
        }
        else
        {
            actorNew->spr.angle = actor->spr.angle;
            HelpMissileLateral(actorNew,2500);
            actorNew->spr.angle += DAngle90;
            HelpMissileLateral(actorNew,1500);
			actorNew->spr.angle -= DAngle22_5;
        }
		actorNew->spr.angle += RandomAngle(22.5) - DAngle22_5/2;
        actorNew->norm_ang();

        // Set the shell number
        actorNew->user.ShellNum = ShellCount;
        actorNew->spr.yrepeat = actorNew->spr.xrepeat = 13;
        break;
    case SHOT_SHELL:
		actorNew->spr.pos.Z -= 13;
        actorNew->spr.angle = actor->spr.angle;
        HelpMissileLateral(actorNew,2500);
        actorNew->spr.angle += DAngle90;
        HelpMissileLateral(actorNew,1300);
        actorNew->spr.angle -= DAngle22_5 * 1.5;
		actorNew->spr.angle += RandomAngle(22.5) - DAngle22_5/2;
        actorNew->norm_ang();

        // Set the shell number
        actorNew->user.ShellNum = ShellCount;
        actorNew->spr.yrepeat = actorNew->spr.xrepeat = 18;
        break;
    }

    SetOwner(actor, actorNew);
    actorNew->spr.shade = -15;
    actorNew->user.ceiling_dist = 1;
    actorNew->user.floor_dist = 1;
    actorNew->user.Counter = 0;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    actorNew->user.Flags &= ~(SPR_BOUNCE|SPR_UNDERWATER); // Make em' bounce

	UpdateChange(actorNew);

    actorNew->user.jump_speed = 200;
    actorNew->user.jump_speed += RandomRange(400);
    actorNew->user.jump_speed = -actorNew->user.jump_speed;

    DoBeginJump(actorNew);
    actorNew->user.jump_grav = ACTOR_GRAVITY;

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
    nullptr,0,

    // data
    saveable_jweapon_data,
    SIZ(saveable_jweapon_data)
};
END_SW_NS
