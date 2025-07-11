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
#include "sector.h"
#include "ai.h"
#include "player.h"
#include "game.h"
#include "interpso.h"
#include "network.h"
#include "sprite.h"
#include "misc.h"
#include "weapon.h"

BEGIN_SW_NS

DVector2 DoTrack(SECTOR_OBJECT* sop, short locktics);
void DoAutoTurretObject(SECTOR_OBJECT* sop);
void DoTornadoObject(SECTOR_OBJECT* sop);
int PickJumpSpeed(DSWActor*, int pix_height);
DSWActor* FindNearSprite(DSWActor, short);
ANIMATOR NinjaJumpActionFunc;

#define ACTOR_STD_JUMP (-384)
DAngle GlobSpeedSO;

double Distance(DVector2 p1, DVector2 p2)
{
    double min;

    if ((p2.X = p2.X - p1.X) < 0)
        p2.X = -p2.X;

    if ((p2.Y = p2.Y - p1.Y) < 0)
        p2.Y = -p2.Y;

    if (p2.X > p2.Y)
        min = p2.Y;
    else
        min = p2.X;

    return p2.X + p2.Y - (min * 0.5);
}


// determine if moving down the track will get you closer to the player
short TrackTowardPlayer(DSWActor* actor, TRACK* t, TRACK_POINT* start_point)
{
    TRACK_POINT* end_point;

    // determine which end of the Track we are starting from
    if (start_point == t->TrackPoint)
    {
        end_point = t->TrackPoint + t->NumPoints - 1;
    }
    else
    {
        end_point = t->TrackPoint;
    }

    auto end_dist = Distance(end_point->pos.XY(), actor->spr.pos.XY());
    auto start_dist = Distance(start_point->pos.XY(), actor->spr.pos.XY());

    return (end_dist < start_dist);
}

short TrackStartCloserThanEnd(DSWActor* actor, TRACK* t, TRACK_POINT* start_point)
{
    TRACK_POINT* end_point;

    // determine which end of the Track we are starting from
    if (start_point == t->TrackPoint)
    {
        end_point = t->TrackPoint + t->NumPoints - 1;
    }
    else
    {
        end_point = t->TrackPoint;
    }

    auto end_dist = Distance(end_point->pos.XY(), actor->spr.pos.XY());
    auto start_dist = Distance(start_point->pos.XY(), actor->spr.pos.XY());

    return (start_dist < end_dist);
}

/*

!AIC - Looks at endpoints to figure direction of the track and the closest
point to the sprite.

*/

short ActorFindTrack(DSWActor* actor, int8_t player_dir, int track_type, int* track_point_num, int* track_dir)
{
    const double threshold = 15000 * maptoworld;
    double near_dist = 999999;

    int i;
    short end_point[2] = { 0,0 };

    TRACK* t, *near_track = nullptr;
    TRACK_POINT* tp, *near_tp = nullptr;

    enum
    {
        TOWARD_PLAYER = 1,
        AWAY_FROM_PLAYER = -1
    };

    // look at all tracks finding the closest endpoint
    for (t = &Track[0]; t < &Track[MAX_TRACKS]; t++)
    {
        tp = t->TrackPoint;

        // Skip if high tag is not ONE of the track type we are looking for
        if (!(t->ttflags & track_type))
            continue;

        // Skip if already someone on this track
        if ((t->flags & TF_TRACK_OCCUPIED))
        {
            continue;
        }

        switch (track_type)
        {
        case BIT(TT_DUCK_N_SHOOT):
        {
            if (!actor->user.ActorActionSet->Duck)
                return -1;

            end_point[1] = 0;
            break;
        }

        // for ladders only look at first track point
        case BIT(TT_LADDER):
        {
            if (!actor->user.ActorActionSet->Climb)
                return -1;

            end_point[1] = 0;
            break;
        }

        case BIT(TT_JUMP_UP):
        case BIT(TT_JUMP_DOWN):
        {
            if (!actor->user.ActorActionSet->Jump)
                return -1;

            end_point[1] = 0;
            break;
        }

        case BIT(TT_TRAVERSE):
        {
            if (!actor->user.ActorActionSet->Crawl || !actor->user.ActorActionSet->Jump)
                return -1;

            break;
        }

        // look at end point also
        default:
            end_point[1] = t->NumPoints - 1;
            break;
        }

        double zdiff = 16;

        // Look at both track end points to see wich is closer
        for (i = 0; i < 2; i++)
        {
            tp = t->TrackPoint + end_point[i];

            double dist = Distance(tp->pos.XY(), actor->spr.pos.XY());

            if (dist < threshold && dist < near_dist)
            {
                // make sure track start is on approximate z level - skip if
                // not
                if (abs(actor->spr.pos.Z - tp->pos.Z) > zdiff)
                {
                    continue;
                }

                // determine if the track leads in the direction we want it
                // to
                if (player_dir == TOWARD_PLAYER)
                {
                    if (!TrackTowardPlayer(actor->user.targetActor, t, tp))
                    {
                        continue;
                    }
                }
                else if (player_dir == AWAY_FROM_PLAYER)
                {
                    if (TrackTowardPlayer(actor->user.targetActor, t, tp))
                    {
                        continue;
                    }
                }

                // make sure the start distance is closer than the end
                // distance
                if (!TrackStartCloserThanEnd(actor, t, tp))
                {
                    continue;
                }

                near_dist = dist;
                near_track = t;
                near_tp = tp;

                *track_point_num = end_point[i];
                *track_dir = i ? -1 : 1;
            }
        }

    }

    auto track_sect = &sector[0];
    if (near_dist < threshold)
    {
        // get the sector number of the point
        updatesector(near_tp->pos, &track_sect);

        // if can see the point, return the track number
        if (track_sect && FAFcansee(actor->spr.pos.plusZ(-16), actor->sector(), DVector3(near_tp->pos.XY(), track_sect->floorz - 32), track_sect))
        {
            return short(near_track - &Track[0]);
        }
    }
    return -1;
}


void NextTrackPoint(SECTOR_OBJECT* sop)
{
    sop->point += sop->dir;

    if (sop->point > Track[sop->track].NumPoints - 1)
        sop->point = 0;

    if (sop->point < 0)
        sop->point = Track[sop->track].NumPoints - 1;
}


void NextActorTrackPoint(DSWActor* actor)
{
    actor->user.point += actor->user.track_dir;

    if (actor->user.point > Track[actor->user.track].NumPoints - 1)
        actor->user.point = 0;

    if (actor->user.point < 0)
        actor->user.point = Track[actor->user.track].NumPoints - 1;
}

void TrackAddPoint(TRACK* t, TRACK_POINT* tp, DSWActor* actor)
{
    TRACK_POINT* tpoint = (tp + t->NumPoints);

    tpoint->pos = actor->spr.pos;
    tpoint->angle = actor->spr.Angles.Yaw;
    tpoint->tag_low = actor->spr.lotag;
    tpoint->tag_high = actor->spr.hitag;

    t->NumPoints++;

    KillActor(actor);
}

DSWActor* TrackClonePoint(DSWActor* actor)
{
    auto actorNew = insertActor(actor->sector(), actor->spr.statnum);

    actorNew->spr.cstat = 0;
    actorNew->spr.extra = 0;
    actorNew->spr.pos = actor->spr.pos;
    actorNew->spr.Angles.Yaw = actor->spr.Angles.Yaw;
    actorNew->spr.lotag = actor->spr.lotag;
    actorNew->spr.hitag = actor->spr.hitag;

    return actorNew;
}

void QuickJumpSetup(short stat, short lotag, short type)
{
    int ndx;
    TRACK_POINT* tp;
    TRACK* t;
    DSWActor* start_sprite,* end_sprite;

    // make short quick jump tracks
    SWStatIterator it(stat);
    while (auto actor = it.Next())
    {
        // find an open track
        for (ndx = 0; ndx < MAX_TRACKS; ndx++)
        {
            if (Track[ndx].NumPoints == 0)
                break;
        }

        ASSERT(ndx < MAX_TRACKS);

        Track[ndx].SetTrackSize(4);

        tp = Track[ndx].TrackPoint;
        t = &Track[ndx];

        // set track type
        t->ttflags |= (BIT(type));
        t->flags = 0;

        // clone point
        end_sprite = TrackClonePoint(actor);
        start_sprite = TrackClonePoint(actor);

        // add start point
        start_sprite->spr.lotag = TRACK_START;
        start_sprite->spr.hitag = 0;
        TrackAddPoint(t, tp, start_sprite);

        // add jump point
        actor->spr.pos += actor->spr.Angles.Yaw.ToVector() * 4;
        actor->spr.lotag = lotag;
        TrackAddPoint(t, tp, actor);

        // add end point
        end_sprite->spr.pos += end_sprite->spr.Angles.Yaw.ToVector() * 128;
        end_sprite->spr.lotag = TRACK_END;
        end_sprite->spr.hitag = 0;
        TrackAddPoint(t, tp, end_sprite);
    }
}


void QuickScanSetup(short stat, short lotag, short type)
{
    int ndx;
    TRACK_POINT* tp;
    TRACK* t;
    DSWActor* start_sprite,* end_sprite;

    // make short quick jump tracks
    SWStatIterator it(stat);
    while (auto actor = it.Next())
    {

        // find an open track
        for (ndx = 0; ndx < MAX_TRACKS; ndx++)
        {
            if (Track[ndx].NumPoints == 0)
                break;
        }

        ASSERT(ndx < MAX_TRACKS);

        // save space for 3 points
        Track[ndx].SetTrackSize(4);

        ASSERT(Track[ndx].TrackPoint != nullptr);

        tp = Track[ndx].TrackPoint;
        t = &Track[ndx];

        // set track type
        t->ttflags |= (BIT(type));
        t->flags = 0;

        // clone point
        end_sprite = TrackClonePoint(actor);
        start_sprite = TrackClonePoint(actor);

        // add start point
        start_sprite->spr.lotag = TRACK_START;
        start_sprite->spr.hitag = 0;
        start_sprite->spr.pos += start_sprite->spr.Angles.Yaw.ToVector() * 4;
        TrackAddPoint(t, tp, start_sprite);

        // add jump point
        actor->spr.lotag = lotag;
        TrackAddPoint(t, tp, actor);

        // add end point
        end_sprite->spr.pos += end_sprite->spr.Angles.Yaw.ToVector() * 4;
        end_sprite->spr.lotag = TRACK_END;
        end_sprite->spr.hitag = 0;
        TrackAddPoint(t, tp, end_sprite);
    }
}

void QuickExitSetup(short stat, short type)
{
    int ndx;
    TRACK_POINT* tp;
    TRACK* t;
    DSWActor* start_sprite,* end_sprite;

    SWStatIterator it(stat);
    while (auto actor = it.Next())
    {
        // find an open track
        for (ndx = 0; ndx < MAX_TRACKS; ndx++)
        {
            if (Track[ndx].NumPoints == 0)
                break;
        }

        ASSERT(ndx < MAX_TRACKS);

        // save space for 3 points
        Track[ndx].SetTrackSize(4);

        ASSERT(Track[ndx].TrackPoint != nullptr);

        tp = Track[ndx].TrackPoint;
        t = &Track[ndx];

        // set track type
        t->ttflags |= (BIT(type));
        t->flags = 0;

        // clone point
        end_sprite = TrackClonePoint(actor);
        start_sprite = TrackClonePoint(actor);

        // add start point
        start_sprite->spr.lotag = TRACK_START;
        start_sprite->spr.hitag = 0;
        TrackAddPoint(t, tp, start_sprite);

        KillActor(actor);

        // add end point
        end_sprite->spr.pos += end_sprite->spr.Angles.Yaw.ToVector() * 64;
        end_sprite->spr.lotag = TRACK_END;
        end_sprite->spr.hitag = 0;
        TrackAddPoint(t, tp, end_sprite);
    }
}

void QuickLadderSetup(short stat, short lotag, short type)
{
    int ndx;
    TRACK_POINT* tp;
    TRACK* t;
    DSWActor* start_sprite,* end_sprite;

    SWStatIterator it(stat);
    while (auto actor = it.Next())
    {
        // find an open track
        for (ndx = 0; ndx < MAX_TRACKS; ndx++)
        {
            if (Track[ndx].NumPoints == 0)
                break;
        }

        ASSERT(ndx < MAX_TRACKS);

        // save space for 3 points
        Track[ndx].SetTrackSize(4);

        ASSERT(Track[ndx].TrackPoint != nullptr);

        tp = Track[ndx].TrackPoint;
        t = &Track[ndx];

        // set track type
        t->ttflags |= (BIT(type));
        t->flags = 0;

        // clone point
        end_sprite = TrackClonePoint(actor);
        start_sprite = TrackClonePoint(actor);

        // add start point
        start_sprite->spr.lotag = TRACK_START;
        start_sprite->spr.hitag = 0;
        start_sprite->spr.pos += (start_sprite->spr.Angles.Yaw + DAngle180).ToVector() * 16;
        TrackAddPoint(t, tp, start_sprite);

        // add climb point
        actor->spr.lotag = lotag;
        TrackAddPoint(t, tp, actor);

        // add end point
        end_sprite->spr.pos += end_sprite->spr.Angles.Yaw.ToVector() * 32;
        end_sprite->spr.lotag = TRACK_END;
        end_sprite->spr.hitag = 0;
        TrackAddPoint(t, tp, end_sprite);
    }
}


void TrackSetup(void)
{
    int ndx;
    TRACK_POINT* tp;
    TRACK* t;
    TRACK_POINT* New;
    int size;

    // put points on track
    for (ndx = 0; ndx < MAX_TRACKS; ndx++)
    {
        SWStatIterator it(STAT_TRACK + ndx);

        if (!it.Next())
        {
            // for some reason I need at least one record allocated
            // can't remember why at this point
            Track[ndx].TrackPoint = (TRACK_POINT*)CallocMem(sizeof(TRACK_POINT) * 1, 1);
            continue;
        }

        ASSERT(Track[ndx].TrackPoint == nullptr);

        // make the track array rather large.  I'll resize it to correct size
        // later.
        Track[ndx].TrackPoint = (TRACK_POINT*)CallocMem(sizeof(TRACK_POINT) * 500, 1);

        ASSERT(Track[ndx].TrackPoint != nullptr);

        tp = Track[ndx].TrackPoint;
        t = &Track[ndx];

        // find the first point and save it
        it.Reset(STAT_TRACK + ndx);
        while (auto actor = it.Next())
        {
            if (actor->spr.lotag == TRACK_START)
            {
                ASSERT(t->NumPoints == 0);

                TrackAddPoint(t, tp, actor);
                break;
            }
        }

        // didn't find the start point of the track
        if (t->NumPoints == 0)
        {
            int i;
            it.Reset(STAT_TRACK + ndx);
            auto itActor = it.Next();
            Printf("WARNING: Did not find first point of Track Number %d, x %d, y %d\n", ndx, int(itActor->spr.pos.X), int(itActor->spr.pos.Y));
            it.Reset(STAT_TRACK + ndx);
            while (auto actor = it.Next())
            {
                // neuter the track's sprite list
                KillActor(actor);
            }
            continue;
        }

        // set up flags for track types
        if (tp->tag_low == TRACK_START && tp->tag_high)
            t->ttflags |= (BIT(tp->tag_high));

        // while there are still sprites on this status list

        while (it.Reset(STAT_TRACK + ndx), it.Next())
        {
            DSWActor* next_actor = nullptr;
            double dist, low_dist = 999999;

            // find the closest point to the last point
            it.Reset(STAT_TRACK + ndx);
            while (auto actor = it.Next())
            {
                dist = Distance((tp + t->NumPoints - 1)->pos.XY(), actor->spr.pos.XY());

                if (dist < low_dist)
                {
                    next_actor = actor;
                    low_dist = dist;
                }

            }

            // save the closest one off and kill it
            if (next_actor != nullptr)
            {
                TrackAddPoint(t, tp, next_actor);
            }

        }

        size = (Track[ndx].NumPoints + 1) * sizeof(TRACK_POINT);
        New = (TRACK_POINT*)CallocMem(size, 1);
        memcpy(New, Track[ndx].TrackPoint, size);
        FreeMem(Track[ndx].TrackPoint);
        Track[ndx].TrackPoint = New;

        ASSERT(Track[ndx].TrackPoint != nullptr);
    }

    QuickJumpSetup(STAT_QUICK_JUMP, TRACK_ACTOR_QUICK_JUMP, TT_JUMP_UP);
    QuickJumpSetup(STAT_QUICK_JUMP_DOWN, TRACK_ACTOR_QUICK_JUMP_DOWN, TT_JUMP_DOWN);
    QuickJumpSetup(STAT_QUICK_SUPER_JUMP, TRACK_ACTOR_QUICK_SUPER_JUMP, TT_SUPER_JUMP_UP);
    QuickScanSetup(STAT_QUICK_SCAN, TRACK_ACTOR_QUICK_SCAN, TT_SCAN);
    QuickLadderSetup(STAT_QUICK_LADDER, TRACK_ACTOR_CLIMB_LADDER, TT_LADDER);
    QuickExitSetup(STAT_QUICK_EXIT, TT_EXIT);
    QuickJumpSetup(STAT_QUICK_OPERATE, TRACK_ACTOR_QUICK_OPERATE, TT_OPERATE);
    QuickJumpSetup(STAT_QUICK_DUCK, TRACK_ACTOR_QUICK_DUCK, TT_DUCK_N_SHOOT);
    QuickJumpSetup(STAT_QUICK_DEFEND, TRACK_ACTOR_QUICK_DEFEND, TT_HIDE_N_SHOOT);

}

DSWActor* FindBoundSprite(int tag)
{
    SWStatIterator it(STAT_ST1);
    while (auto actor = it.Next())
    {
        if (actor->spr.hitag == tag)
        {
            return actor;
        }
    }

    return nullptr;
}


void SectorObjectSetupBounds(SECTOR_OBJECT* sop)
{
    int startwall, endwall;
    int i, k, j;
    DSWActor* BoundActor = nullptr;
    bool FoundOutsideLoop = false;
    bool SectorInBounds;
    sectortype* *sectp;
    DSWActor* child = sop->sp_child;

    static const uint8_t StatList[] =
    {
        STAT_DEFAULT, STAT_MISC, STAT_ITEM, STAT_TRAP,
        STAT_SPAWN_SPOT, STAT_SOUND_SPOT, STAT_WALL_MOVE,
        STAT_WALLBLOOD_QUEUE,
        STAT_SPRITE_HIT_MATCH,
        STAT_AMBIENT,
        STAT_DELETE_SPRITE,
        STAT_SPAWN_TRIGGER, // spawing monster trigger - for Randy's bullet train.
        //STAT_FLOOR_PAN, STAT_CEILING_PAN
    };

    // search for 2 sprite bounding tags

    BoundActor = FindBoundSprite(500 + (int(sop - SectorObject) * 5));
    if (BoundActor == nullptr)
    {
        I_Error("SOP bound sprite with hitag %d not found", 500 + (int(sop - SectorObject) * 5));
    }

    DVector2 vlow = BoundActor->spr.pos.XY();

    KillActor(BoundActor);

    BoundActor = FindBoundSprite(501 + (int(sop - SectorObject) * 5));
    if (BoundActor == nullptr)
    {
        I_Error("SOP bound sprite with hitag %d not found", 501 + (int(sop - SectorObject) * 5));
    }
    DVector2 vhigh = BoundActor->spr.pos.XY();

    KillActor(BoundActor);

    // set radius for explosion checking - based on bounding box
    child->user.Radius = int(((vhigh.X - vlow.X) + (vhigh.Y - vlow.Y)) * (0.75 * 0.25) * worldtoint); // trying to get it a good size

    // search for center sprite if it exists

    BoundActor = FindBoundSprite(SECT_SO_CENTER);
    if (BoundActor)
    {
        sop->pmid = BoundActor->spr.pos;
        KillActor(BoundActor);
    }

    // look through all sectors for whole sectors that are IN bounds
    for (auto&sec: sector)
    {
        auto sect = &sec;

        SectorInBounds = true;

        for(auto& wal : sect->walls)
        {
            // all walls have to be in bounds to be in sector object
            if (!(wal.pos.X > vlow.X && wal.pos.X < vhigh.X && wal.pos.Y > vlow.Y && wal.pos.Y < vhigh.Y))
            {
                SectorInBounds = false;
                break;
            }
        }

        if (SectorInBounds)
        {
            sop->sectp[sop->num_sectors] = sect;
            sop->sectp[sop->num_sectors+1] = nullptr;

            // all sectors in sector object have this flag set - for colision
            // detection and recognition
            sect->extra |= SECTFX_SECTOR_OBJECT;

            sop->zorig_floor[sop->num_sectors] = sect->floorz;

            if ((sect->extra & SECTFX_SINK))
                sop->zorig_floor[sop->num_sectors] += FixedToInt(sect->depth_fixed);

            // lowest and highest floor z's
            if (sect->floorz > sop->floor_loz)
                sop->floor_loz = sect->floorz;

            if (sect->floorz < sop->floor_hiz)
                sop->floor_hiz = sect->floorz;

            sop->num_sectors++;
        }

        ASSERT((uint16_t)sop->num_sectors < SIZ(SectorObject[0].sectp));
    }

    //
    // Make sure every sector object has an outer loop tagged - important
    //

    FoundOutsideLoop = false;

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        // move all walls in sectors
        for(auto& wal : (*sectp)->walls)
        {
            // for morph point - tornado style
            if (wal.lotag == TAG_WALL_ALIGN_SLOPE_TO_POINT)
                sop->morph_wall_point = &wal;

            if (wal.extra && (wal.extra & WALLFX_LOOP_OUTER))
                FoundOutsideLoop = true;

            // each wall has this set - for collision detection
            wal.extra |= WALLFX_SECTOR_OBJECT|WALLFX_DONT_STICK;
            if (wal.twoSided())
                wal.nextWall()->extra |= WALLFX_SECTOR_OBJECT|WALLFX_DONT_STICK;
        }
    }

    if (!FoundOutsideLoop)
    {
        I_Error("Forgot to tag outer loop for Sector Object #%d", (int)(sop - SectorObject));
    }

    so_addinterpolation(sop);

    for (i = 0; i < (int)SIZ(StatList); i++)
    {
        SWStatIterator it(StatList[i]);
        while (auto itActor = it.Next())
        {
            if (itActor->spr.pos.X > vlow.X && itActor->spr.pos.X < vhigh.X && itActor->spr.pos.Y > vlow.Y && itActor->spr.pos.Y < vhigh.Y)
            {
                // some delete sprites ride others don't
                if (itActor->spr.statnum == STAT_DELETE_SPRITE)
                {
                    if (!TEST_BOOL2(itActor))
                        continue;
                }

                if (!itActor->hasU())
                    SpawnUser(itActor, 0, nullptr);

                itActor->user.RotNum = 0;

                itActor->backuppos();
                itActor->user.oz = itActor->opos.Z;

                switch (itActor->spr.statnum)
                {
                case STAT_WALL_MOVE:
                    break;
                case STAT_DEFAULT:
                    switch (itActor->spr.hitag)
                    {
                    case SO_CLIP_BOX:
                    {
                        sop->clipdist = 0;
                        sop->clipbox_dist[sop->clipbox_num] = itActor->spr.lotag * maptoworld;

                        sop->clipbox_vdist[sop->clipbox_num] = (sop->pmid.XY() - itActor->spr.pos.XY()).Length();

                        auto ang2 = (itActor->spr.pos.XY() - sop->pmid.XY()).Angle();
                        sop->clipbox_ang[sop->clipbox_num] = deltaangle(ang2, sop->ang);

                        sop->clipbox_num++;
                        KillActor(itActor);


                        goto cont;
                    }
                    case SO_SHOOT_POINT:
                        ClearOwner(itActor);
                        change_actor_stat(itActor, STAT_SO_SHOOT_POINT);
                        itActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
                        break;
                    default:
                        break;
                    }
                    break;
                }


                itActor->user.pos.SetXY(sop->pmid.XY() - itActor->spr.pos.XY());
                itActor->user.pos.Z = sop->mid_sector->floorz - itActor->spr.pos.Z;

                itActor->user.Flags |= (SPR_SO_ATTACHED);

                itActor->user.sang = itActor->spr.Angles.Yaw;
                itActor->user.spal = itActor->spr.pal;

                // search SO's sectors to make sure that it is not on a
                // sector

                // place all sprites on list
                int sn;
                for (sn = 0; sn < (int)SIZ(sop->so_actors); sn++)
                {
                    if (sop->so_actors[sn] == nullptr)
                        break;
                }

				sop->so_actors[sn] = itActor;
                so_setspriteinterpolation(sop, itActor);


                if (!(sop->flags & SOBJ_SPRITE_OBJ))
                {
                    // determine if sprite is on a SO sector - set flag if
                    // true
                    for (j = 0; j < sop->num_sectors; j++)
                    {
                        if (sop->sectp[j] == itActor->sector())
                        {
                            itActor->user.Flags |= (SPR_ON_SO_SECTOR);
                            itActor->user.pos.Z = itActor->sector()->floorz - itActor->spr.pos.Z;
                            break;
                        }
                    }
                }
            }

cont:
            continue;
        }
    }

    // for SPRITE OBJECT sprites, set the actor->user.sz value to the difference
    // between the zmid and the actor->spr.z
    if ((sop->flags & SOBJ_SPRITE_OBJ))
    {
        double zmid = -9999999;

        // choose the lowest sprite for the zmid
        for (i = 0; sop->so_actors[i] != nullptr; i++)
        {
            auto actor = sop->so_actors[i];

            if (actor->spr.pos.Z > zmid)
                zmid = actor->spr.pos.Z;
        }

        ASSERT(zmid != -9999999);

        sop->pmid.Z = zmid;

		for (i = 0; sop->so_actors[i] != nullptr; i++)
		{
            auto actor = sop->so_actors[i];
            actor->user.pos.Z = sop->pmid.Z - actor->spr.pos.Z;
        }
    }
}


void SetupSectorObject(sectortype* sectp, short tag)
{
    SECTOR_OBJECT* sop;
    int object_num;
    short j;

    tag -= (TAG_OBJECT_CENTER - 1);

    object_num = tag / 5;
    sop = &SectorObject[object_num];

    // initialize stuff first time through
    if (sop->num_sectors == -1)
    {
        void DoTornadoObject(SECTOR_OBJECT* sop);
        void MorphTornado(SECTOR_OBJECT* sop);
        void MorphFloor(SECTOR_OBJECT* sop);
        void ScaleSectorObject(SECTOR_OBJECT* sop);
        void DoAutoTurretObject(SECTOR_OBJECT* sop);

        memset(sop->sectp, 0, sizeof(sop->sectp));
        memset(sop->so_actors, 0, sizeof(sop->so_actors));
        sop->morph_wall_point = nullptr;
        sop->op_main_sector = nullptr;
        sop->scratch = nullptr; // this is a guard field for sectp, because several loops do not test the end properly.
        sop->match_event_actor = nullptr;
        sop->crush_z = 0;
        sop->drive_angspeed = 0;
        sop->drive_angslide = 0;
        sop->drive_slide = 0;
        sop->drive_speed = 0;
        sop->num_sectors = 0;
        sop->update = 15000;
        sop->flags = 0;
        sop->clipbox_num = 0;
        sop->bob_amt = 0;
        sop->vel_rate = 6;
        sop->z_rate = 256;
        sop->z_tgt = 0;
        sop->zdelta = 0;
        sop->wait_tics = 0;
        sop->spin_speed = nullAngle;
        sop->spin_ang = nullAngle;
        sop->ang_orig = nullAngle;
        sop->clipdist = 64;
        sop->target_dist = 0;
        sop->turn_speed = 4;
        sop->floor_loz = -9999999;
        sop->floor_hiz = 9999999;
        sop->ang_tgt = sop->ang = sop->ang_moving = nullAngle;
        sop->op_main_sector = nullptr;
        sop->ram_damage = 0;
        sop->max_damage = -9999;

        sop->scale_type = SO_SCALE_NONE;
        sop->scale_dist = 0;
        sop->scale_speed = 1.25;
        sop->scale_dist_min = -64;
        sop->scale_dist_max = 64;
        sop->scale_rand_freq = 64>>3;

        sop->scale_x_mult = 256;
        sop->scale_y_mult = 256;

        sop->morph_ang = RandomAngle();
        sop->morph_z_speed = 20;
        sop->morph_speed = 2;
        sop->morph_dist_max = 64;
        sop->morph_rand_freq = 64;
        sop->morph_dist = 0;
        sop->morph_off = { 0,0 };

        sop->PreMoveAnimator = nullptr;
        sop->PostMoveAnimator = nullptr;
        sop->Animator = nullptr;
    }

    switch (tag % 5)
    {
    case TAG_OBJECT_CENTER - 500:

        sop->mid_sector = sectp;
        sop->pmid = SectorMidPoint(sectp);

        sop->dir = 1;
        sop->track = sectp->hitag;

        // spawn a sprite to make it easier to integrate with sprite routines
        auto actorNew = SpawnActor(STAT_SO_SP_CHILD, 0, nullptr, sectp, sop->pmid, nullAngle);
        sop->sp_child = actorNew;
        actorNew->user.sop_parent = sop;
        actorNew->user.Flags2 |= (SPR2_SPRITE_FAKE_BLOCK); // for damage test

        // check for any ST1 sprites laying on the center sector
        SWSectIterator it(sectp);
        while (auto actor = it.Next())
        {
            if (actor->spr.statnum == STAT_ST1)
            {
                switch (actor->spr.hitag)
                {
                case SO_SCALE_XY_MULT:
                    if (SP_TAG5(actor))
                        sop->scale_x_mult = SP_TAG5(actor);
                    if (SP_TAG6(actor))
                        sop->scale_y_mult = SP_TAG6(actor);
                    KillActor(actor);
                    break;

                case SO_SCALE_POINT_INFO:

                    memset(sop->scale_point_dist,0,sizeof(sop->scale_point_dist));
                    sop->scale_point_base_speed = SP_TAG2(actor) * maptoworld;
                    for (j = 0; j < (int)SIZ(sop->scale_point_speed); j++)
                    {
                        sop->scale_point_speed[j] = SP_TAG2(actor) * maptoworld;
                    }

                    if (SP_TAG4(actor))
                        sop->scale_point_rand_freq = (uint8_t)SP_TAG4(actor);
                    else
                        sop->scale_point_rand_freq = 64;

                    sop->scale_point_dist_min = -SP_TAG5(actor) * maptoworld;
                    sop->scale_point_dist_max = SP_TAG6(actor) * maptoworld;
                    KillActor(actor);
                    break;

                case SO_SCALE_INFO:
                    sop->flags |= (SOBJ_DYNAMIC);
                    sop->scale_speed = SP_TAG2(actor) * maptoworld;
                    sop->scale_dist_min = -SP_TAG5(actor) * maptoworld;
                    sop->scale_dist_max = SP_TAG6(actor) * maptoworld;

                    sop->scale_type = SP_TAG4(actor);
                    sop->scale_active_type = SP_TAG7(actor);

                    if (SP_TAG8(actor))
                        sop->scale_rand_freq = (uint8_t)SP_TAG8(actor);
                    else
                        sop->scale_rand_freq = 64>>3;

                    if (SP_TAG3(actor) == 0)
                        sop->scale_dist = sop->scale_dist_min;
                    else if (SP_TAG3(actor) == 1)
                        sop->scale_dist = sop->scale_dist_max;

                    KillActor(actor);
                    break;

                case SPAWN_SPOT:
					if (actor->spr.clipdist == 3) // notreallyclipdist
                    {
                        change_actor_stat(actor, STAT_NO_STATE);
                        SpawnUser(actor, 0, nullptr);
                        actor->user.ActorActionFunc = nullptr;
                    }
                    break;

                case SO_AUTO_TURRET:
                    sop->Animator = DoAutoTurretObject;
                    KillActor(actor);
                    break;

                case SO_TORNADO:
                    if (SW_SHAREWARE) break;
                    sop->vel = 120;
                    sop->flags |= (SOBJ_DYNAMIC);
                    sop->scale_type = SO_SCALE_CYCLE;
                    // spin stuff
                    sop->spin_speed = DAngle22_5 * (1. / 16);
                    sop->last_ang = sop->ang;
                    // animators
                    sop->Animator = DoTornadoObject;
                    sop->PreMoveAnimator = ScaleSectorObject;
                    sop->PostMoveAnimator = MorphTornado;
                    // clip
                    sop->clipdist = 156.25;
                    // morph point
                    sop->morph_speed = 1;
                    sop->morph_z_speed = 6;
                    sop->morph_dist_max = 64;
                    sop->morph_rand_freq = 8;
                    sop->scale_dist_min = -48;
                    KillActor(actor);
                    break;
                case SO_FLOOR_MORPH:
                    if (SW_SHAREWARE) break;
                    sop->flags |= (SOBJ_DYNAMIC);
                    sop->scale_type = SO_SCALE_NONE;
                    sop->morph_speed = 7.5;
                    sop->morph_z_speed = 7;
                    sop->PostMoveAnimator = MorphFloor;
                    sop->morph_dist_max = 250;
                    sop->morph_rand_freq = 8;
                    KillActor(actor);
                    break;

                case SO_AMOEBA:
                    sop->flags |= (SOBJ_DYNAMIC);
                    //sop->scale_type = SO_SCALE_CYCLE;
                    sop->scale_type = SO_SCALE_RANDOM_POINT;
                    sop->PreMoveAnimator = ScaleSectorObject;

                    memset(sop->scale_point_dist,0,sizeof(sop->scale_point_dist));;
                    sop->scale_point_base_speed = 0.25 + RandomRangeF(0.5);
                    for (j = 0; j < (int)SIZ(sop->scale_point_speed); j++)
                        sop->scale_point_speed[j] = 0.25 + RandomRangeF(0.5);

                    sop->scale_point_dist_min = -16;
                    sop->scale_point_dist_max = 16;
                    sop->scale_point_rand_freq = 32;
                    KillActor(actor);
                    break;
                case SO_MAX_DAMAGE:
                    actorNew->user.MaxHealth = SP_TAG2(actor);
                    if (SP_TAG5(actor) != 0)
                        sop->max_damage = SP_TAG5(actor);
                    else
                        sop->max_damage = actorNew->user.MaxHealth;

                    switch (actor->spr.clipdist) // notreallyclipdist
                    {
                    case 0:
                        break;
                    case 1:
                        sop->flags |= (SOBJ_DIE_HARD);
                        break;
                    }
                    KillActor(actor);
                    break;

                case SO_DRIVABLE_ATTRIB:

                    sop->drive_angspeed = FixedToFloat<11>(SP_TAG2(actor));
                    sop->drive_angslide = SP_TAG3(actor);
                    if (sop->drive_angslide <= 0 || sop->drive_angslide == 32)
                        sop->drive_angslide = 1;

                    sop->drive_speed = SP_TAG6(actor);
                    sop->drive_speed <<= 5;
                    sop->drive_slide = SP_TAG7(actor);
                    if (sop->drive_slide <= 0)
                        sop->drive_slide = 1;

                    if (TEST_BOOL1(actor))
                        sop->flags |= (SOBJ_NO_QUAKE);

                    if (TEST_BOOL3(actor))
                        sop->flags |= (SOBJ_REMOTE_ONLY);

                    if (TEST_BOOL4(actor))
                    {
                        sop->crush_z = actor->spr.pos.Z;
                        sop->flags |= (SOBJ_RECT_CLIP);
                    }

                    //KillActor(actor);
                    break;

                case SO_RAM_DAMAGE:
                    sop->ram_damage = actor->spr.lotag;
                    KillActor(actor);
                    break;
                case SECT_SO_CLIP_DIST:
                    sop->clipdist = actor->spr.lotag * maptoworld;
                    KillActor(actor);
                    break;
                case SECT_SO_SPRITE_OBJ:
                    sop->flags |= (SOBJ_SPRITE_OBJ);
                    KillActor(actor);
                    break;
                case SECT_SO_DONT_ROTATE:
                    sop->flags |= (SOBJ_DONT_ROTATE);
                    KillActor(actor);
                    break;
                case SO_LIMIT_TURN:
                    sop->limit_ang_center = actor->spr.Angles.Yaw;
                    sop->limit_ang_delta = mapangle(actor->spr.lotag);
                    KillActor(actor);
                    break;
                case SO_MATCH_EVENT:
                    sop->match_event = actor->spr.lotag;
                    sop->match_event_actor = actor;
                    break;
                case SO_SET_SPEED:
                    sop->vel = actor->spr.lotag * 256;
                    sop->vel_tgt = sop->vel;
                    KillActor(actor);
                    break;
                case SO_SPIN:
                    if (sop->spin_speed != nullAngle)
                        break;
                    sop->spin_speed = mapangle(actor->spr.lotag);
                    sop->last_ang = sop->ang;
                    KillActor(actor);
                    break;
                case SO_ANGLE:
                    sop->ang = sop->ang_moving = actor->spr.Angles.Yaw;
                    sop->last_ang = sop->ang_orig = sop->ang;
                    sop->spin_ang = nullAngle;
                    KillActor(actor);
                    break;
                case SO_SPIN_REVERSE:

                    sop->spin_speed = mapangle(actor->spr.lotag);
                    sop->last_ang = sop->ang;

                    if (sop->spin_speed >= nullAngle)
                        sop->spin_speed = -sop->spin_speed;

                    KillActor(actor);
                    break;
                case SO_BOB_START:
                    sop->bob_amt = actor->spr.lotag;
                    sop->bob_sine_ndx = 0;
                    sop->bob_speed = 4;
                    KillActor(actor);
                    break;
                case SO_TURN_SPEED:
                    sop->turn_speed = actor->spr.lotag;
                    KillActor(actor);
                    break;
                case SO_SYNC1:
                    sop->flags |= (SOBJ_SYNC1);
                    KillActor(actor);
                    break;
                case SO_SYNC2:
                    sop->flags |= (SOBJ_SYNC2);
                    KillActor(actor);
                    break;
                case SO_KILLABLE:
                    sop->flags |= (SOBJ_KILLABLE);
                    KillActor(actor);
                    break;
                }
            }
        }

        if (sop->vel == -1)
            sop->vel = sop->vel_tgt = 8 * 256;

        SectorObjectSetupBounds(sop);

        if (sop->track >= SO_OPERATE_TRACK_START)
        {
            switch (sop->track)
            {
            case SO_TURRET_MGUN:
            case SO_TURRET:
            case SO_VEHICLE:
                sop->vel = 0;
                sop->flags |= (SOBJ_OPERATIONAL);
                break;
#if 0
            case SO_SPEED_BOAT:
                sop->vel = 0;
                sop->bob_amt = 2;
                sop->bob_speed = 4;
                sop->flags |= (SOBJ_OPERATIONAL);
                break;
#endif
            default:
                sop->flags |= (SOBJ_OPERATIONAL);
                break;
            }
        }

        sectp->lotag = 0;
        sectp->hitag = 0;

        if (sop->max_damage <= 0)
            VehicleSetSmoke(sop, SpawnVehicleSmoke);

        break;
    }

}

void PostSetupSectorObject(void)
{
    SECTOR_OBJECT* sop;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (SO_EMPTY(sop))
            continue;
        FindMainSector(sop);
    }
}


SECTOR_OBJECT* PlayerOnObject(sectortype* match)
{
    short i, j;
    SECTOR_OBJECT* sop;

    // place each sector object on the track
    //for (i = 0; !SO_EMPTY(&SectorObject[i]) && (i < MAX_SECTOR_OBJECTS); i++)
    for (i = 0; (i < MAX_SECTOR_OBJECTS); i++)
    {
        sop = &SectorObject[i];

        if (sop->track < SO_OPERATE_TRACK_START)
            continue;

        for (j = 0; j < sop->num_sectors; j++)
        {
            if (sop->sectp[j] == match && (match->extra & SECTFX_OPERATIONAL))
            {
                return sop;
            }
        }
    }

    return nullptr;
}


void PlaceSectorObjectsOnTracks(void)
{
    short i, j, k, startwall, endwall;
    bool found;

    // place each sector object on the track
    for (i = 0; i < MAX_SECTOR_OBJECTS; i++)
    {
        double low_dist = 999999, dist;
        SECTOR_OBJECT* sop = &SectorObject[i];
        TRACK_POINT* tpoint = nullptr;

        if (SO_EMPTY(sop))
            continue;


        // save off the original x and y locations of the walls AND sprites
        sop->num_walls = 0;
        for (j = 0; sop->sectp[j] != nullptr; j++)
        {

            // move all walls in sectors
            for (auto& wal : sop->sectp[j]->walls)
            {
                sop->orig[sop->num_walls] = sop->pmid.XY() - wal.pos;
                sop->num_walls++;
            }
        }

        ASSERT((uint16_t)sop->num_walls < SIZ(sop->orig));

        if (sop->track <= -1)
            continue;

        if (sop->track >= SO_OPERATE_TRACK_START)
            continue;

        found = false;
        // find the closest point on the track and put SOBJ on it
        for (j = 0; j < Track[sop->track].NumPoints; j++)
        {
            tpoint = Track[sop->track].TrackPoint;

            dist = Distance((tpoint + j)->pos.XY(), sop->pmid.XY());

            if (dist < low_dist)
            {
                low_dist = dist;
                sop->point = j;
                found = true;
            }
        }

        if (!found)
        {
            sop->track = -1;
            continue;
        }

        NextTrackPoint(sop);

        sop->ang = ((tpoint + sop->point)->pos - sop->pmid).Angle();
        sop->ang_moving = sop->ang_tgt = sop->ang;
    }

}


void PlaceActorsOnTracks(void)
{
    short j, tag;
    TRACK_POINT* tpoint = nullptr;

    // place each actor on the track
    SWStatIterator it(STAT_ENEMY);
    while (auto actor = it.Next())
    {
        double low_dist = 999999, dist;

        tag = actor->spr.lotag;

        if (tag < TAG_ACTOR_TRACK_BEGIN || tag > TAG_ACTOR_TRACK_END)
            continue;

        // setup sprite track defaults
        actor->user.track = tag - TAG_ACTOR_TRACK_BEGIN;
        actor->norm_ang();
        // if facing left go backward
        if (actor->spr.Angles.Yaw > DAngle90  && actor->spr.Angles.Yaw < DAngle270)
        {
            actor->user.track_dir = -1;
        }
        else
        {
            actor->user.track_dir = 1;
        }

        actor->user.track_vel = int(actor->vel.X * 4096);
        actor->user.vel_tgt = actor->user.track_vel;
        actor->user.vel_rate = 6;

        // find the closest point on the track and put SOBJ on it
        for (j = 0; j < Track[actor->user.track].NumPoints; j++)
        {
            tpoint = Track[actor->user.track].TrackPoint;

            dist = Distance((tpoint + j)->pos.XY(), actor->spr.pos.XY());

            if (dist < low_dist)
            {
                low_dist = dist;
                actor->user.point = j;
            }
        }

        NextActorTrackPoint(actor);

        if (Track[actor->user.track].NumPoints == 0)
        {
            Printf("WARNING: Sprite %d (%2.2f, %2.2f) placed on track %d with no points!\n", actor->GetIndex(), actor->spr.pos.X, actor->spr.pos.Y, actor->user.track);
            continue;
        }

        // check angle in the "forward" direction
        actor->spr.Angles.Yaw = ((tpoint + actor->user.point)->pos - actor->spr.pos).Angle();
    }
}


void MovePlayer(DSWPlayer* pp, SECTOR_OBJECT* sop, const DVector2& move)
{
    void DoPlayerZrange(DSWPlayer* pp);

    // make sure your standing on the so
    if (pp->Flags & (PF_JUMPING | PF_FALLING | PF_FLYING))
        return;

    pp->sop_riding = sop;

    // if player has NOT moved and player is NOT riding
    // set up the player for riding
    if (!(pp->Flags & PF_PLAYER_MOVED) && !(pp->Flags & PF_PLAYER_RIDING))
    {
        pp->Flags |= (PF_PLAYER_RIDING);

        pp->RevolveAng = pp->GetActor()->spr.Angles.Yaw;
        pp->Revolve.SetXY(pp->GetActor()->spr.pos.XY());

        // set the delta angle to 0 when moving
        pp->RevolveDeltaAng = nullAngle;
    }

    pp->GetActor()->spr.pos.XY() += move;

    if ((sop->flags & SOBJ_DONT_ROTATE))
    {
        UpdatePlayerSprite(pp);
        return;
    }

    if (pp->Flags & (PF_PLAYER_MOVED))
    {
        // Player is moving

        // save the current information so when Player stops
        // moving then you
        // know where he was last
        pp->RevolveAng = pp->GetActor()->spr.Angles.Yaw;
        pp->Revolve.SetXY(pp->GetActor()->spr.pos.XY());

        // set the delta angle to 0 when moving
        pp->RevolveDeltaAng = nullAngle;
    }
    else
    {
        // Player is NOT moving

        // Move saved x&y variables
        pp->Revolve += move;

        // Last known angle is now adjusted by the delta angle
        pp->RevolveAng = deltaangle(pp->RevolveDeltaAng, pp->GetActor()->spr.Angles.Yaw);
    }

    // increment Players delta angle
    pp->RevolveDeltaAng += GlobSpeedSO;

    pp->GetActor()->spr.pos.SetXY(rotatepoint(sop->pmid.XY(), pp->Revolve.XY(), pp->RevolveDeltaAng));

    // THIS WAS CAUSING PROLEMS!!!!
    // Sectors are still being manipulated so you can end up in a void (-1) sector

    // New angle is formed by taking last known angle and
    // adjusting by the delta angle
    pp->GetActor()->spr.Angles.Yaw = (pp->RevolveAng + pp->RevolveDeltaAng).Normalized360();

    UpdatePlayerSprite(pp);
}

void MovePoints(SECTOR_OBJECT* sop, DAngle deltaangle, const DVector2& move)
{
    int j;
    int pnum;
    DSWPlayer* pp;
    sectortype** sectp;
    int i;
    DAngle rot_ang;
    bool PlayerMove = true;

    if (SO_EMPTY(sop))
        PlayerMove = false;

    // move along little midpoint
    sop->pmid += move;

    if (SO_EMPTY(sop))
        PlayerMove = false;

    // move child sprite along also
    sop->sp_child->spr.pos.SetXY(sop->pmid.XY());

    // setting floor z if need be
    if ((sop->flags & SOBJ_ZMID_FLOOR))
        sop->pmid.Z = sop->mid_sector->floorz;

    DVector2 pivot = sop->pmid.XY();
    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if ((sop->flags & (SOBJ_SPRITE_OBJ | SOBJ_DONT_ROTATE)))
            goto PlayerPart;

        // move all walls in sectors
        for(auto& wal : (*sectp)->walls)
        {
            if ((wal.extra & (WALLFX_LOOP_DONT_SPIN | WALLFX_DONT_MOVE)))
                continue;

            if (wal.extra && (wal.extra & WALLFX_LOOP_OUTER))
            {
                dragpoint(&wal, wal.pos + move);
            }
            else
            {
                wal.move(wal.pos + move);
            }

            rot_ang = deltaangle;

            if ((wal.extra & WALLFX_LOOP_REVERSE_SPIN))
                rot_ang = -deltaangle;

            if ((wal.extra & WALLFX_LOOP_SPIN_2X))
                rot_ang = (rot_ang * 2).Normalized360();

            if ((wal.extra & WALLFX_LOOP_SPIN_4X))
                rot_ang = (rot_ang * 4).Normalized360();

            auto vec = rotatepoint(pivot, wal.pos, rot_ang);

            if (wal.extra && (wal.extra & WALLFX_LOOP_OUTER))
            {
                dragpoint(&wal, vec);
            }
            else
            {
                wal.move(vec);
            }
        }

PlayerPart:

        TRAVERSE_CONNECT(pnum)
        {
            pp = getPlayer(pnum);

            // if controlling a sector object
            if (pp->sop)
                continue;

            if (!pp->lo_sectp)
                continue;

            if ((pp->lo_sectp->extra & SECTFX_NO_RIDE))
            {
                continue;
            }

            // move the player
            if (pp->lo_sectp == sop->sectp[j])
            {
                if (PlayerMove)
                    MovePlayer(pp, sop, move);
            }
        }
    }

	for (i = 0; sop->so_actors[i] != nullptr; i++)
	{
        DSWActor* actor = sop->so_actors[i];
        if (!actor) continue;

        // if its a player sprite || NOT attached
        if (!actor->hasU() || actor->user.PlayerP || !(actor->user.Flags & SPR_SO_ATTACHED))
            continue;

        // move the player
        TRAVERSE_CONNECT(pnum)
        {
            pp = getPlayer(pnum);

            if (pp->lowActor && pp->lowActor == actor)
            {
                if (PlayerMove)
                    MovePlayer(pp, sop, move);
            }
        }

        actor->spr.pos.SetXY(sop->pmid.XY() - actor->user.pos.XY());

        // sprites z update
        if ((sop->flags & SOBJ_SPRITE_OBJ))
        {
            // Sprite Objects follow zmid
            actor->spr.pos.Z = sop->pmid.Z - actor->user.pos.Z;
        }
        else
        {
            // Sector Objects can either have sprites ON or OFF of the sector
            if (actor->user.Flags & (SPR_ON_SO_SECTOR))
            {
                // move with sector its on
                actor->spr.pos.Z = actor->sector()->floorz - actor->user.pos.Z;
            }
            else
            {
                // move with the mid sector
                actor->spr.pos.Z = sop->mid_sector->floorz - actor->user.pos.Z;
            }
        }

        auto oldang = actor->spr.Angles.Yaw;
        actor->spr.Angles.Yaw = actor->user.sang;

        if (actor->user.Flags & (SPR_ON_SO_SECTOR))
        {
            if ((sop->flags & SOBJ_DONT_ROTATE))
                continue;

            // IS part of a sector, sprite can do things based on the
            // current sector it is in
            if ((actor->sector()->walls[0].extra & WALLFX_LOOP_DONT_SPIN))
                continue;

            if ((actor->sector()->walls[0].extra & WALLFX_LOOP_REVERSE_SPIN))
            {
                actor->spr.pos.SetXY(rotatepoint(sop->pmid.XY(), actor->spr.pos.XY(), -deltaangle));
                actor->spr.Angles.Yaw -= deltaangle;
            }
            else
            {
                actor->spr.pos.SetXY(rotatepoint(sop->pmid.XY(), actor->spr.pos.XY(), deltaangle));
                actor->spr.Angles.Yaw += deltaangle;
            }
			actor->norm_ang();
        }
        else
        {
            if (!(sop->flags & SOBJ_DONT_ROTATE))
            {
                // NOT part of a sector - independant of any sector
                actor->spr.pos.SetXY(rotatepoint(sop->pmid.XY(), actor->spr.pos.XY(), deltaangle));
                actor->spr.Angles.Yaw += deltaangle;
				actor->norm_ang();
            }

            // Does not necessarily move with the sector so must accout for
            // moving across sectors
            if (!SO_EMPTY(sop)) // special case for operating SO's
                SetActorZ(sop->so_actors[i], actor->spr.pos);
        }

        actor->user.oangdiff += ::deltaangle(oldang, actor->spr.Angles.Yaw);

        if ((actor->spr.extra & SPRX_BLADE))
        {
            DoBladeDamage(sop->so_actors[i]);
        }
    }

    TRAVERSE_CONNECT(pnum)
    {
        pp = getPlayer(pnum);

        // if player was on a sector object
        if (pp->sop_riding)
        {
            // update here AFTER sectors/player has been manipulated
            // prevents you from falling into map HOLEs created by moving
            // Sectors and sprites around.
            //if (!SO_EMPTY(sop))
            updatesector(pp->GetActor()->getPosWithOffsetZ(), &pp->cursector);

            // in case you are in a whirlpool
            // move perfectly with the ride in the z direction
            if (pp->Flags & PF_CRAWLING)
            {
                // move up some for really fast moving plats
                DoPlayerZrange(pp);
                pp->posZset(pp->loz - PLAYER_CRAWL_HEIGHTF);
            }
            else
            {
                // move up some for really fast moving plats
                DoPlayerZrange(pp);

                if (!(pp->Flags & (PF_JUMPING | PF_FALLING | PF_FLYING)))
                {
                    pp->posZset(pp->loz - PLAYER_HEIGHTF);
                }
            }
        }
        else
        {
            // if player was not on any sector object set Riding flag to false
            pp->Flags &= ~(PF_PLAYER_RIDING);
        }
    }
}

void RefreshPoints(SECTOR_OBJECT* sop, const DVector2& move, bool dynamic)
{
    int wallcount = 0;
    DAngle delta_ang_from_orig;

    // do scaling
    if (dynamic && sop->PreMoveAnimator)
        (*sop->PreMoveAnimator)(sop);

    sectortype** sectp;
    int j;
    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if (!(sop->flags & SOBJ_SPRITE_OBJ))
        {
            // move all walls in sectors back to the original position
            for (auto& wal : (*sectp)->walls)
            {
                if (!(wal.extra && (wal.extra & WALLFX_DONT_MOVE)))
                {
                    DVector2 pos = sop->pmid.XY() - sop->orig[wallcount];
                    DVector2 dpos = pos;

                    if (dynamic && sop->scale_type)
                    {
                        if (!(wal.extra & WALLFX_DONT_SCALE))
                        {
                            auto ang = (pos - sop->pmid).Angle();

                            if (sop->scale_type == SO_SCALE_RANDOM_POINT)
                            {
                                // was causing memory overwrites
                                dpos = ScaleRandomPoint(sop, wallcount, ang, pos);
                            }
                            else
                            {
                                double xmul = (sop->scale_dist * sop->scale_x_mult) * (1 / 256.);
                                double ymul = (sop->scale_dist * sop->scale_y_mult) * (1 / 256.);

                                dpos.X = pos.X + xmul * ang.Cos();
                                dpos.Y = pos.Y + ymul * ang.Sin();
                            }
                        }
                    }

                    if (wal.extra & WALLFX_LOOP_OUTER)
                    {
                        dragpoint(&wal, dpos);
                    }
                    else
                    {
                        wal.move(dpos);
                    }
                }

                wallcount++;
            }
        }
    }

    if (sop->spin_speed != nullAngle)
    {
        // same as below - ignore the objects angle
        // last_ang is the last true angle before SO started spinning
        delta_ang_from_orig = (sop->last_ang + sop->spin_ang - sop->ang_orig).Normalized360();
    }
    else
    {
        // angle traveling + the new spin angle all offset from the original
        // angle
        delta_ang_from_orig = (sop->ang + sop->spin_ang - sop->ang_orig).Normalized360();
    }

    // Note that this delta angle is from the original angle
    // nx,ny are 0 so the points are not moved, just rotated

    MovePoints(sop, delta_ang_from_orig, move);

    // do morphing - angle independent
    if (dynamic && sop->PostMoveAnimator)
        (*sop->PostMoveAnimator)(sop);
}

void KillSectorObjectSprites(SECTOR_OBJECT* sop)
{
    int i;

    for (i = 0; sop->so_actors[i] != nullptr; i++)
    {
		DSWActor* actor = sop->so_actors[i];
        if (!actor) continue;

        // not a part of the so anymore
        actor->user.Flags &= ~(SPR_SO_ATTACHED);

        if (actor->spr.picnum == ST1 && actor->spr.hitag == SPAWN_SPOT)
            continue;

        so_stopspriteinterpolation(sop, actor);
        KillActor(actor);
    }

    // clear the list
    sop->so_actors[0] = nullptr;
}

void UpdateSectorObjectSprites(SECTOR_OBJECT* sop)
{
    int i;

	for (i = 0; sop->so_actors[i] != nullptr; i++)
	{
        DSWActor* actor = sop->so_actors[i];
        if (!actor) continue;

        SetActorZ(actor, actor->spr.pos);
    }
}

SECTOR_OBJECT* DetectSectorObject(sectortype* sectph)
{
    short j;
    sectortype* *sectp;
    SECTOR_OBJECT* sop;


    // collapse the SO to a single point
    // move all points to nx,ny
    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (SO_EMPTY(sop))
            continue;

        for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
        {
            if (sectph == *sectp)
                return sop;
        }
    }

    return nullptr;
}

SECTOR_OBJECT* DetectSectorObjectByWall(walltype* wph)
{
    SECTOR_OBJECT* sop;

    // collapse the SO to a single point
    // move all points to nx,ny
    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (SO_EMPTY(sop))
            continue;

        sectortype** sectp;
        int j;
        for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
        {
            for (auto& wal : (*sectp)->walls)
            {
                // if outer wall check the NEXTWALL also
                if ((wal.extra & WALLFX_LOOP_OUTER))
                {
                    if (wal.twoSided() && wph == wal.nextWall())
                        return sop;
                }

                if (wph == &wal)
                    return sop;
            }
        }
    }

    return nullptr;
}


void CollapseSectorObject(SECTOR_OBJECT* sop, const DVector2& pos)
{
    int j;
    sectortype* *sectp;

    // collapse the SO to a single point
    // move all points to nx,ny
    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if (!(sop->flags & SOBJ_SPRITE_OBJ))
        {
            // move all walls in sectors back to the original position
            for (auto& wal : (*sectp)->walls)
            {
                if ((wal.extra & WALLFX_DONT_MOVE))
                    continue;

                if (wal.extra && (wal.extra & WALLFX_LOOP_OUTER))
                {
                    dragpoint(&wal, pos);
                }
                else
                {
                    wal.move(pos);
                }
            }
        }
    }
}


void MoveZ(SECTOR_OBJECT* sop)
{
    short i;
    sectortype* *sectp;

    if (sop->bob_amt)
    {
        sop->bob_sine_ndx = (PlayClock << sop->bob_speed) & 2047;
        sop->bob_diff = sop->bob_amt * BobVal(sop->bob_sine_ndx);

        // for all sectors
        for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
        {
            if (sop->sectp[i]->hasU() && (sop->sectp[i]->flags & SECTFU_SO_DONT_BOB))
                continue;

            (*sectp)->setfloorz(sop->zorig_floor[i] + sop->bob_diff);
        }
    }

    if ((sop->flags & SOBJ_MOVE_VERTICAL))
    {
        i = AnimGetGoal (ANIM_SopZ, int(sop - SectorObject), nullptr);
        if (i < 0)
            sop->flags &= ~(SOBJ_MOVE_VERTICAL);
    }

    if ((sop->flags & SOBJ_ZDIFF_MODE))
    {
        return;
    }

    // move all floors
    if ((sop->flags & SOBJ_ZDOWN))
    {
        for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
        {
            AnimSet(ANIM_Floorz, *sectp, sop->zorig_floor[i] + sop->z_tgt, sop->z_rate);
        }

        sop->flags &= ~(SOBJ_ZDOWN);
    }
    else if ((sop->flags & SOBJ_ZUP))
    {
        for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
        {
            AnimSet(ANIM_Floorz, *sectp, sop->zorig_floor[i] + sop->z_tgt, sop->z_rate);
        }

        sop->flags &= ~(SOBJ_ZUP);
    }
}

void CallbackSOsink(ANIM* ap, void *data)
{
    SECTOR_OBJECT* sop;
    int i, ndx;
    bool found = false;
    int tgt_depth;
    sectortype* srcsect = nullptr;
    sectortype* destsect = nullptr;

    sop = (SECTOR_OBJECT*)data;

    for (i = 0; sop->sectp[i] != nullptr; i++)
    {
        if (sop->sectp[i]->hasU() && (sop->sectp[i]->flags & SECTFU_SO_SINK_DEST))
        {
            srcsect = sop->sectp[i];
            break;
        }
    }

    ASSERT(srcsect != nullptr);

    for (i = 0; sop->sectp[i] != nullptr; i++)
    {
        if (ap->animtype == ANIM_Floorz && ap->animindex == sectindex(sop->sectp[i]))
        {
            destsect = sop->sectp[i];
            break;
        }
    }

    ASSERT(destsect != nullptr);

    destsect->setfloortexture(srcsect->floortexture);
    destsect->floorshade = srcsect->floorshade;

    destsect->floorstat &= ~(CSTAT_SECTOR_ALIGN);
    destsect->u_defined = true;
    ASSERT(srcsect->hasU());

    tgt_depth = FixedToInt(srcsect->depth_fixed);

    for(auto& sect: sector)
    {
        if (&sect == destsect)
        {
            ndx = AnimSet(ANIM_SUdepth, destsect, IntToFixed(tgt_depth), ap->vel);
            AnimSetVelAdj(ndx, ap->vel_adj);
            found = true;
            break;
        }
    }

    ASSERT(found);

    SWSectIterator it(destsect);
    while (auto actor = it.Next())
    {
        if (!actor->hasU() || actor->user.PlayerP || !(actor->user.Flags & SPR_SO_ATTACHED))
            continue;

        // move sprite WAY down in water
        ndx = AnimSet(ANIM_Userz, 0, actor, -actor->user.pos.Z - ActorSizeZ(actor) - 100, ap->vel / 256);
        AnimSetVelAdj(ndx, ap->vel_adj);
    }


    // Take out any blocking walls
    for(auto& wal : destsect->walls)
    {
        wal.cstat &= ~(CSTAT_WALL_BLOCK);
    }

    return;
}


void MoveSectorObjects(SECTOR_OBJECT* sop, short locktics)
{
    so_setinterpolationtics(sop, locktics);

    if (sop->track >= SO_OPERATE_TRACK_START)
    {
        if ((sop->flags & SOBJ_UPDATE_ONCE))
        {
            sop->flags &= ~(SOBJ_UPDATE_ONCE);
            RefreshPoints(sop, DVector2(0, 0), false);
        }
        return;
    }


    // if pausing the return
    if (sop->wait_tics)
    {
        sop->wait_tics -= locktics;
        if (sop->wait_tics <= 0)
            sop->wait_tics = 0;

        return;
    }

    auto delta_ang = nullAngle;

    DVector2 npos(0, 0);
    if (sop->track > -1)
        npos = DoTrack(sop, locktics);

    // get delta to target angle
    delta_ang = deltaangle(sop->ang, sop->ang_tgt);
    // If we do not truncate precision here, some SOs will misbehave, most notably the boat in MAP 5.
    delta_ang = DAngle::fromBuild((delta_ang.Buildang() >> sop->turn_speed));

    sop->ang = (sop->ang + delta_ang).Normalized360();


    // move z values
    MoveZ(sop);

    // calculate the spin speed
    auto speed = sop->spin_speed * locktics;
    // spin_ang is incremented by the spin_speed
    sop->spin_ang = (sop->spin_ang + speed).Normalized360();

    if (sop->spin_speed != nullAngle)
    {
        // ignore delta angle if spinning
        GlobSpeedSO = speed;
    }
    else
    {
        // The actual delta from the last frame
        GlobSpeedSO = speed + delta_ang;
    }

    if ((sop->flags & SOBJ_DYNAMIC))
    {
        // trick tricks
        RefreshPoints(sop, npos, true);
    }
    else
    {
        // Update the points so there will be no warping
        if ((sop->flags & (SOBJ_UPDATE|SOBJ_UPDATE_ONCE)) ||
            sop->vel ||
            (sop->ang != sop->ang_tgt) ||
            GlobSpeedSO.Degrees())
        {
            sop->flags &= ~(SOBJ_UPDATE_ONCE);
            RefreshPoints(sop, npos, false);
        }
    }
}

DVector2 DoTrack(SECTOR_OBJECT* sop, short locktics)
{
    TRACK_POINT* tpoint;

    tpoint = Track[sop->track].TrackPoint + sop->point;

    // calculate an angle to the target

    if (sop->vel)
        sop->ang_moving = sop->ang_tgt = (tpoint->pos - sop->pmid).Angle();

    // NOTE: Jittery ride - try new value out here
    // NOTE: Put a loop around this (locktics) to make it more acuruate
    const int TRACK_POINT_SIZE = 200;
    if (sop->target_dist < 100 * maptoworld)
    {
        switch (tpoint->tag_low)
        {
        case TRACK_MATCH_EVERYTHING:
            DoMatchEverything(nullptr, tpoint->tag_high, -1);
            break;

        case TRACK_MATCH_EVERYTHING_ONCE:
            DoMatchEverything(nullptr, tpoint->tag_high, -1);
            tpoint->tag_low = 0;
            tpoint->tag_high = 0;
            break;

        case TRACK_SPIN:
            if (sop->spin_speed != nullAngle)
                break;

            sop->spin_speed = mapangle(tpoint->tag_high);
            sop->last_ang = sop->ang;
            break;

        case TRACK_SPIN_REVERSE:
        {
            if (sop->spin_speed == nullAngle)
                break;

            if (sop->spin_speed >= nullAngle)
            {
                sop->spin_speed = -sop->spin_speed;
            }
        }
        break;

        case TRACK_SPIN_STOP:
            if (sop->spin_speed == nullAngle)
                break;

            sop->spin_speed = nullAngle;
            break;

        case TRACK_BOB_START:
            sop->flags |= (SOBJ_ZMID_FLOOR);
            sop->bob_amt = tpoint->tag_high;
            sop->bob_sine_ndx = 0;
            sop->bob_speed = 4;
            break;

        case TRACK_BOB_STOP:
            sop->bob_speed = 0;
            sop->bob_sine_ndx = 0;
            sop->bob_amt = 0;
            break;

        case TRACK_BOB_SPEED:
            sop->bob_speed = tpoint->tag_high;
            break;

        case TRACK_REVERSE:
            sop->dir *= -1;
            break;
        case TRACK_STOP:
            sop->vel = 0;
            sop->wait_tics = tpoint->tag_high * 128;
            break;
        case TRACK_SET_SPEED:
            sop->vel = tpoint->tag_high * 256;
            sop->vel_tgt = sop->vel;
            break;

        //
        // Controls the velocity
        //

        case TRACK_VEL_RATE:
            sop->vel_rate = tpoint->tag_high;
            break;
        case TRACK_SPEED_UP:
            sop->flags &= ~(SOBJ_SLOW_DOWN | SOBJ_SPEED_UP);
            if (sop->dir < 0)
            {
                // set target to new slower target
                sop->vel_tgt = sop->vel_tgt - (tpoint->tag_high * 256);
                sop->flags |= (SOBJ_SLOW_DOWN);
            }
            else
            {
                sop->vel_tgt = sop->vel_tgt + (tpoint->tag_high * 256);
                sop->flags |= (SOBJ_SPEED_UP);
            }

            break;

        case TRACK_SLOW_DOWN:
            sop->flags &= ~(SOBJ_SLOW_DOWN | SOBJ_SPEED_UP);
            if (sop->dir > 0)
            {
                sop->vel_tgt = sop->vel_tgt - (tpoint->tag_high * 256);
                sop->flags |= (SOBJ_SLOW_DOWN);
            }
            else
            {
                sop->vel_tgt = sop->vel_tgt + (tpoint->tag_high * 256);
                sop->flags |= (SOBJ_SPEED_UP);
            }
            break;

        //
        // Controls z
        //

        case TRACK_SO_SINK:
        {
            sectortype* *sectp;
            sectortype* dest_sector = nullptr;
            short i,ndx;

            for (i = 0; sop->sectp[i] != nullptr; i++)
            {
                if (sop->sectp[i]->hasU() && (sop->sectp[i]->flags & SECTFU_SO_SINK_DEST))
                {
                    dest_sector = sop->sectp[i];
                    break;
                }
            }

            ASSERT(dest_sector != nullptr);

            sop->bob_speed = 0;
            sop->bob_sine_ndx = 0;
            sop->bob_amt = 0;

            for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
            {
                if (sop->sectp[i]->hasU() && (sop->sectp[i]->flags & SECTFU_SO_DONT_SINK))
                    continue;

                ndx = AnimSet(ANIM_Floorz, *sectp, dest_sector->floorz, tpoint->tag_high);
                AnimSetCallback(ndx, CallbackSOsink, sop);
                AnimSetVelAdj(ndx, 6 * zmaptoworld);
            }

            break;
        }

        case TRACK_SO_FORM_WHIRLPOOL:
        {
            // for lowering the whirlpool in level 1
            sectortype* *sectp;
            int i;

            for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
            {
                if ((*sectp)->hasU())
                {
                    if ((*sectp) && (*sectp)->stag == SECT_SO_FORM_WHIRLPOOL)
                    {
                        AnimSet(ANIM_Floorz, *sectp, (*sectp)->floorz + (*sectp)->height, 128);
                        (*sectp)->floorshade += (*sectp)->height / 6;

                        (*sectp)->extra &= ~(SECTFX_NO_RIDE);
                    }
                }
            }

            break;
        }

        case TRACK_MOVE_VERTICAL:
        {
            int zr;
            sop->flags |= (SOBJ_MOVE_VERTICAL);

            if (tpoint->tag_high > 0)
                zr = tpoint->tag_high;
            else
                zr = 256;

            // look at the next point
            NextTrackPoint(sop);
            tpoint = Track[sop->track].TrackPoint + sop->point;

            // set anim
            AnimSet(ANIM_SopZ, int(sop-SectorObject), nullptr, tpoint->pos.Z, zr);

            // move back to current point by reversing direction
            sop->dir *= -1;
            NextTrackPoint(sop);
            tpoint = Track[sop->track].TrackPoint + sop->point;
            sop->dir *= -1;

            break;
        }

        case TRACK_WAIT_FOR_EVENT:
        {
            if (tpoint->tag_high == -1)
                break;

            sop->flags |= (SOBJ_WAIT_FOR_EVENT);
            sop->save_vel = sop->vel;
            sop->save_spin_speed = sop->spin_speed;

            sop->vel = 0;
            sop->spin_speed = nullAngle;
            // only set event if non-zero
            if (tpoint->tag_high)
                sop->match_event = tpoint->tag_high;
            tpoint->tag_high = -1;
            break;
        }

        case TRACK_ZDIFF_MODE:
            sop->flags |= (SOBJ_ZDIFF_MODE);
            sop->zdelta = tpoint->tag_high;
            break;
        case TRACK_ZRATE:
            sop->z_rate = Z(tpoint->tag_high); // looks like a bug. z_rate is pixel based.
            break;
        case TRACK_ZUP:
            sop->flags &= ~(SOBJ_ZDOWN | SOBJ_ZUP);
            if (sop->dir < 0)
            {
                sop->z_tgt += tpoint->tag_high;
                sop->flags |= (SOBJ_ZDOWN);
            }
            else
            {
                sop->z_tgt -= tpoint->tag_high;
                sop->flags |= (SOBJ_ZUP);
            }
            break;
        case TRACK_ZDOWN:
            sop->flags &= ~(SOBJ_ZDOWN | SOBJ_ZUP);
            if (sop->dir > 0)
            {
                sop->z_tgt += tpoint->tag_high;
                sop->flags |= (SOBJ_ZDOWN);
            }
            else
            {
                sop->z_tgt -= tpoint->tag_high;
                sop->flags |= (SOBJ_ZUP);
            }
            break;
        }

        // get the next point
        NextTrackPoint(sop);
        tpoint = Track[sop->track].TrackPoint + sop->point;

        // calculate distance to target point
        sop->target_dist = (sop->pmid.XY() - tpoint->pos.XY()).Length();

        // calculate a new angle to the target
        sop->ang_moving = sop->ang_tgt = (tpoint->pos - sop->pmid).Angle();

        if ((sop->flags & SOBJ_ZDIFF_MODE))
        {

            // set dx,dy,dz up for finding the z magnitude
            auto pos = tpoint->pos.plusZ(-sop->zdelta);

            // find the distance to the target (player)
            double dist = (pos.XY() - sop->pmid.XY()).Length();

            // (velocity * difference between the target and the object)
            // take absolute value
            sop->z_rate = (int)abs((sop->vel * maptoworld * (sop->pmid.Z - pos.Z)) / dist);

            if ((sop->flags & SOBJ_SPRITE_OBJ))
            {
                // only modify zmid for sprite_objects
                AnimSet(ANIM_SopZ, int(sop - SectorObject), nullptr, pos.Z, sop->z_rate);
            }
            else
            {
                // churn through sectors setting their new z values
                for (int i = 0; sop->sectp[i] != nullptr; i++)
                {
                    AnimSet(ANIM_Floorz, sop->sectp[i], pos.Z - (sop->mid_sector->floorz - sop->sectp[i]->floorz), sop->z_rate);
                }
            }
        }
    }
    else
    {

        // make velocity approach the target velocity
        if ((sop->flags & SOBJ_SPEED_UP))
        {
            if ((sop->vel += (locktics << sop->vel_rate)) >= sop->vel_tgt)
            {
                sop->vel = sop->vel_tgt;
                sop->flags &= ~(SOBJ_SPEED_UP);
            }
        }
        else if ((sop->flags & SOBJ_SLOW_DOWN))
        {
            if ((sop->vel -= (locktics << sop->vel_rate)) <= sop->vel_tgt)
            {
                sop->vel = sop->vel_tgt;
                sop->flags &= ~(SOBJ_SLOW_DOWN);
            }
        }
    }

    // calculate a new x and y
    if (sop->vel && !(sop->flags & SOBJ_MOVE_VERTICAL))
    {
        DVector2 n;
		// The truncation here is needed. There still seems to be code that depends on integers for these
        n.X = int(((sop->vel) >> 8) * locktics * sop->ang_moving.Cos()) * inttoworld;
        n.Y = int(((sop->vel) >> 8) * locktics * sop->ang_moving.Sin()) * inttoworld;

        sop->target_dist -= n.Length();
        return n;
    }
    return { 0,0 };
}


void OperateSectorObjectForTics(SECTOR_OBJECT* sop, DAngle newang, const DVector2& pos, int locktics)
{
    int i;
    sectortype* *sectp;

    if (Prediction)
        return;

    if (sop->track < SO_OPERATE_TRACK_START)
        return;

    so_setinterpolationtics(sop, locktics);

    if (sop->bob_amt)
    {
        sop->bob_sine_ndx = (PlayClock << sop->bob_speed) & 2047;
        sop->bob_diff = sop->bob_amt * BobVal(sop->bob_sine_ndx);

        // for all sectors
        for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
        {
            if (sop->sectp[i]->hasU() && (sop->sectp[i]->flags & SECTFU_SO_DONT_BOB))
                continue;

            (*sectp)->setfloorz(sop->zorig_floor[i] + sop->bob_diff);
        }
    }

    GlobSpeedSO = nullAngle;

    //sop->ang_tgt = newang;
    sop->ang_moving = newang;

    sop->spin_ang = nullAngle;
    sop->ang = newang;

    RefreshPoints(sop, pos - sop->pmid.XY(), false);
}

void OperateSectorObject(SECTOR_OBJECT* sop, DAngle newang, const DVector2& pos)
{
    OperateSectorObjectForTics(sop, newang, pos, synctics);
}

void PlaceSectorObject(SECTOR_OBJECT* sop, const DVector2& pos)
{
    so_setinterpolationtics(sop, synctics);
    RefreshPoints(sop, pos - sop->pmid.XY(), false);
}

void VehicleSetSmoke(SECTOR_OBJECT* sop, ANIMATOR* animator)
{
    sectortype* *sectp;

    for (sectp = sop->sectp; *sectp; sectp++)
    {
        SWSectIterator it(*sectp);
        while (auto actor = it.Next())
        {
            switch (actor->spr.hitag)
            {

            case SPAWN_SPOT:
                if (actor->spr.clipdist == 3) // notreallyclipdist
                {
                    if (animator)
                    {
                        if (actor->spr.statnum == STAT_NO_STATE)
                            break;

                        change_actor_stat(actor, STAT_NO_STATE);
                        DoSoundSpotMatch(actor->spr.lotag, 1, 0);
                        DoSpawnSpotsForDamage(actor->spr.lotag);
                    }
                    else
                    {
                        change_actor_stat(actor, STAT_SPAWN_SPOT);
                        DoSoundSpotStopSound(actor->spr.lotag);
                    }

                    actor->user.ActorActionFunc = animator;
                }
                break;
            }
        }
    }
}


void TornadoSpin(SECTOR_OBJECT* sop)
{
    DAngle delta_ang, speed;
    short locktics = synctics;

    // get delta to target angle
    delta_ang = deltaangle(sop->ang, sop->ang_tgt);

    sop->ang = (sop->ang + (delta_ang / (1 << sop->turn_speed))).Normalized360();
    delta_ang /= 1 << sop->turn_speed;

    // move z values
    MoveZ(sop);

    // calculate the spin speed
    speed = sop->spin_speed * locktics;
    // spin_ang is incremented by the spin_speed
    sop->spin_ang = sop->spin_ang + speed;

    if (sop->spin_speed != nullAngle)
    {
        // ignore delta angle if spinning
        GlobSpeedSO = speed;
    }
    else
    {
        // The actual delta from the last frame
        GlobSpeedSO = speed + delta_ang;
    }
}

void DoTornadoObject(SECTOR_OBJECT* sop)
{
    int ret;
    DAngle &ang = sop->ang_moving;

    auto cursect = sop->op_main_sector; // for sop->vel
    double floor_dist = (abs(cursect->ceilingz - cursect->floorz)) * 0.25;
    DVector3 pos(sop->pmid.XY(), floor_dist);

    PlaceSectorObject(sop, {MAXSO, MAXSO});
    Collision coll;

    auto vect = ang.ToVector() * sop->vel * inttoworld; // vel is still in Build coordinates.
    clipmove(pos, &cursect, vect, sop->clipdist, 0., floor_dist, CLIPMASK_ACTOR, coll);

    if (coll.type != kHitNone)
    {
		ang = ang + DAngle180 - DAngle45 + RandomAngle(90);
    }

    TornadoSpin(sop);
    RefreshPoints(sop, pos.XY() - sop->pmid.XY(), true);
}

void DoAutoTurretObject(SECTOR_OBJECT* sop)
{
    DSWActor* actor = sop->sp_child;
    if (!actor) return;

    DAngle delta_ang;
    DAngle diff;

    if ((sop->max_damage != -9999 && sop->max_damage <= 0) || !actor->hasU())
        return;


    actor->user.WaitTics -= synctics;

    // check for new player if doesn't have a target or time limit expired
    if (!actor->user.targetActor || actor->user.WaitTics < 0)
    {
        // 4 seconds
        actor->user.WaitTics = 4*120;
        DoActorPickClosePlayer(actor);
    }

    if (MoveSkip2 == 0)
    {
		for (int i = 0; sop->so_actors[i] != nullptr; i++)
        {
            DSWActor* sActor = sop->so_actors[i];
            if (!sActor) continue;

			if (sActor->spr.statnum == STAT_SO_SHOOT_POINT)
            {
                if (!FAFcansee(sActor->spr.pos.plusZ(-4), sActor->sector(), ActorUpperVect(actor->user.targetActor), actor->user.targetActor->sector()))
                {
                    return;
                }
            }
        }


        // FirePausing
        if (actor->user.Counter > 0)
        {
            actor->user.Counter -= synctics*2;
            if (actor->user.Counter <= 0)
                actor->user.Counter = 0;
        }

        if (actor->user.Counter == 0)
        {
			for (int i = 0; sop->so_actors[i] != nullptr; i++)
			{
                DSWActor* sActor = sop->so_actors[i];
                if (!sActor) continue;

				if (sActor->spr.statnum == STAT_SO_SHOOT_POINT)
				{
                    if (SP_TAG5(sActor))
                        actor->user.Counter = SP_TAG5(sActor);
                    else
                        actor->user.Counter = 12;
                    InitTurretMgun(sop);
                }
            }
        }

        sop->ang_tgt = (actor->user.targetActor->spr.pos - sop->pmid).Angle();

        // get delta to target angle
        delta_ang = deltaangle(sop->ang, sop->ang_tgt);

        sop->ang += delta_ang * 0.125;

        if (sop->limit_ang_center >= nullAngle)
        {
            diff = deltaangle(sop->limit_ang_center, sop->ang);

            if (abs(diff) >= sop->limit_ang_delta)
            {
                if (diff < nullAngle)
                    sop->ang = (sop->limit_ang_center - sop->limit_ang_delta);
                else
                    sop->ang = (sop->limit_ang_center + sop->limit_ang_delta);

            }
        }

        OperateSectorObjectForTics(sop, sop->ang, sop->pmid.XY(), 2*synctics);
    }
}


void DoActorHitTrackEndPoint(DSWActor* actor)
{
    Track[actor->user.track].flags &= ~(TF_TRACK_OCCUPIED);

    // jump the current track & determine if you should go to another
    if (actor->user.Flags & (SPR_RUN_AWAY))
    {
        // look for another track leading away from the player
        actor->user.track = FindTrackAwayFromPlayer(actor);

        if (actor->user.track >= 0)
        {
            auto tp = Track[actor->user.track].TrackPoint + actor->user.point;
            actor->spr.Angles.Yaw = (tp->pos - actor->spr.pos).Angle();
        }
        else
        {
            actor->user.Flags &= ~(SPR_RUN_AWAY);
            DoActorSetSpeed(actor, NORM_SPEED);
            actor->user.track = -1;
        }
    }
    else if (actor->user.Flags & (SPR_FIND_PLAYER))
    {
        // look for another track leading away from the player
        actor->user.track = FindTrackToPlayer(actor);

        if (actor->user.track >= 0)
        {
            auto tp = Track[actor->user.track].TrackPoint + actor->user.point;
            actor->spr.Angles.Yaw = (tp->pos - actor->spr.pos).Angle();
        }
        else
        {
            actor->user.Flags &= ~(SPR_FIND_PLAYER);
            DoActorSetSpeed(actor, NORM_SPEED);
            actor->user.track = -1;
        }
    }
    else
    {
         actor->user.track = -1;
    }
}


void ActorLeaveTrack(DSWActor* actor)
{
    if (actor->user.track == -1)
        return;

    actor->user.Flags &= ~(SPR_FIND_PLAYER|SPR_RUN_AWAY|SPR_CLIMBING);
    Track[actor->user.track].flags &= ~(TF_TRACK_OCCUPIED);
    actor->user.track = -1;
}

bool ActorTrackDecide(TRACK_POINT* tpoint, DSWActor* actor)
{
    switch (tpoint->tag_low)
    {
    case TRACK_START:

        // if track has a type and actor is going the right direction jump
        // the track
        if (Track[actor->user.track].ttflags)
        {
            if (actor->user.track_dir == -1)
            {
                DoActorHitTrackEndPoint(actor);
                return false;
            }
        }

        break;

    case TRACK_END:
        // if track has a type and actor is going to right direction jump the
        // track
        if (Track[actor->user.track].ttflags)
        {
            if (actor->user.track_dir == 1)
            {
                DoActorHitTrackEndPoint(actor);
                return false;
            }
        }

        break;


    case TRACK_ACTOR_WAIT_FOR_PLAYER:
    {
        actor->user.Flags |= (SPR_WAIT_FOR_PLAYER);
        actor->user.Dist = tpoint->tag_high * maptoworld;
        break;
    }

    case TRACK_ACTOR_WAIT_FOR_TRIGGER:
    {
        actor->user.Flags |= (SPR_WAIT_FOR_TRIGGER);
        actor->user.Dist = tpoint->tag_high * maptoworld;
        break;
    }

    //
    // Controls the velocity
    //

    case TRACK_ACTOR_VEL_RATE:
        actor->user.vel_rate = tpoint->tag_high;
        break;
    case TRACK_ACTOR_SPEED_UP:
        actor->user.Flags &= ~(SPR_SLOW_DOWN | SPR_SPEED_UP);
        if (actor->user.track_dir < 0)
        {
            // set target to new slower target
            actor->user.vel_tgt = actor->user.vel_tgt - (tpoint->tag_high * 256);
            actor->user.Flags |= (SPR_SLOW_DOWN);
        }
        else
        {
            actor->user.vel_tgt = actor->user.vel_tgt + (tpoint->tag_high * 256);
            actor->user.Flags |= (SPR_SPEED_UP);
        }

        break;

    case TRACK_ACTOR_SLOW_DOWN:
        actor->user.Flags &= ~(SPR_SLOW_DOWN | SPR_SPEED_UP);
        if (actor->user.track_dir > 0)
        {
            actor->user.vel_tgt = actor->user.vel_tgt - (tpoint->tag_high * 256);
            actor->user.Flags |= (SPR_SLOW_DOWN);
        }
        else
        {
            actor->user.vel_tgt = actor->user.vel_tgt + (tpoint->tag_high * 256);
            actor->user.Flags |= (SPR_SPEED_UP);
        }
        break;

    // Reverse it
    case TRACK_ACTOR_REVERSE:
        actor->user.track_dir *= -1;
        break;

    case TRACK_ACTOR_STAND:
        NewStateGroup(actor, actor->user.ActorActionSet->Stand);
        break;

    case TRACK_ACTOR_JUMP:
        if (actor->user.ActorActionSet->Jump)
        {
            actor->spr.Angles.Yaw = tpoint->angle;

            if (!tpoint->tag_high)
                actor->user.jump_speed = ACTOR_STD_JUMP;
            else
                actor->user.jump_speed = -tpoint->tag_high;

            DoActorBeginJump(actor);
            actor->user.ActorActionFunc = DoActorMoveJump;
        }

        break;

    case TRACK_ACTOR_QUICK_JUMP:
    case TRACK_ACTOR_QUICK_SUPER_JUMP:
        if (actor->user.ActorActionSet->Jump)
        {
            int zdiff;
            HitInfo hit{};

            actor->spr.Angles.Yaw = tpoint->angle;


            ActorLeaveTrack(actor);

            if (tpoint->tag_high)
            {
                actor->user.jump_speed = -tpoint->tag_high;
            }
            else
            {
                actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);

                FAFhitscan(actor->spr.pos.plusZ(-24), actor->sector(), DVector3(actor->spr.Angles.Yaw.ToVector() * 1024, 0), hit, CLIPMASK_MISSILE);

                actor->spr.cstat |= (CSTAT_SPRITE_BLOCK);

                ASSERT(hit.hitSector != nullptr);

                if (hit.actor() != nullptr)
                    return false;

                if (hit.hitWall == nullptr)
                    return false;

                if (!hit.hitWall->twoSided())
                    return false;

                zdiff = (int)abs(actor->spr.pos.Z - hit.hitWall->nextSector()->floorz);

                actor->user.jump_speed = PickJumpSpeed(actor, zdiff);
            }

            DoActorBeginJump(actor);
            actor->user.ActorActionFunc = DoActorMoveJump;

            return false;
        }

        break;

    case TRACK_ACTOR_QUICK_JUMP_DOWN:

        if (actor->user.ActorActionSet->Jump)
        {
            actor->spr.Angles.Yaw = tpoint->angle;

            ActorLeaveTrack(actor);

            if (tpoint->tag_high)
            {
                actor->user.jump_speed = -tpoint->tag_high;
            }
            else
            {
                actor->user.jump_speed = -350;
            }

            DoActorBeginJump(actor);
            actor->user.ActorActionFunc = DoActorMoveJump;
            return false;
        }

        break;

    case TRACK_ACTOR_QUICK_SCAN:

        if (actor->user.ActorActionSet->Jump)
        {
            ActorLeaveTrack(actor);
            return false;
        }

        break;

    case TRACK_ACTOR_QUICK_DUCK:

        if (actor->user.Rot != actor->user.ActorActionSet->Duck)
        {
            actor->spr.Angles.Yaw = tpoint->angle;

            ActorLeaveTrack(actor);

            if (!tpoint->tag_high)
                actor->user.WaitTics = 4 * 120;
            else
                actor->user.WaitTics = tpoint->tag_high * 128;

            InitActorDuck(actor);
            actor->user.ActorActionFunc = DoActorDuck;
            return false;
        }

        break;

    case TRACK_ACTOR_OPERATE:
    case TRACK_ACTOR_QUICK_OPERATE:
    {
        HitInfo near{};
        double z[2];

        if (actor->user.Rot == actor->user.ActorActionSet->Sit || actor->user.Rot == actor->user.ActorActionSet->Stand)
            return false;

        actor->spr.Angles.Yaw = tpoint->angle;

        z[0] = actor->spr.pos.Z - ActorSizeZ(actor) + 5;
        z[1] = actor->spr.pos.Z - (ActorSizeZ(actor) * 0.5);

        for (auto& zz : z)
        {
            neartag(DVector3(actor->spr.pos.XY(), zz), actor->sector(), actor->spr.Angles.Yaw, near, 64., NT_Lotag | NT_Hitag);

            if (near.actor() != nullptr)
            {
                if (OperateSprite(near.actor(), false))
                {
                    if (!tpoint->tag_high)
                        actor->user.WaitTics = 2 * 120;
                    else
                        actor->user.WaitTics = tpoint->tag_high * 128;

                    NewStateGroup(actor, actor->user.ActorActionSet->Stand);
                }
            }
        }

        if (near.hitSector != nullptr)
        {
            if (OperateSector(near.hitSector, false))
            {
                if (!tpoint->tag_high)
                    actor->user.WaitTics = 2 * 120;
                else
                    actor->user.WaitTics = tpoint->tag_high * 128;

                NewStateGroup(actor, actor->user.ActorActionSet->Sit);
            }
        }

        break;
    }

    case TRACK_ACTOR_JUMP_IF_FORWARD:
        if (actor->user.ActorActionSet->Jump && actor->user.track_dir == 1)
        {
            if (!tpoint->tag_high)
                actor->user.jump_speed = ACTOR_STD_JUMP;
            else
                actor->user.jump_speed = -tpoint->tag_high;

            DoActorBeginJump(actor);
        }

        break;

    case TRACK_ACTOR_JUMP_IF_REVERSE:
        if (actor->user.ActorActionSet->Jump && actor->user.track_dir == -1)
        {
            if (!tpoint->tag_high)
                actor->user.jump_speed = ACTOR_STD_JUMP;
            else
                actor->user.jump_speed = -tpoint->tag_high;

            DoActorBeginJump(actor);
        }

        break;

    case TRACK_ACTOR_CRAWL:
        if (actor->user.Rot != actor->user.ActorActionSet->Crawl)
            NewStateGroup(actor, actor->user.ActorActionSet->Crawl);
        else
            NewStateGroup(actor, actor->user.ActorActionSet->Rise);
        break;

    case TRACK_ACTOR_SWIM:
        if (actor->user.Rot != actor->user.ActorActionSet->Swim)
            NewStateGroup(actor, actor->user.ActorActionSet->Swim);
        else
            NewStateGroup(actor, actor->user.ActorActionSet->Rise);
        break;

    case TRACK_ACTOR_FLY:
        NewStateGroup(actor, actor->user.ActorActionSet->Fly);
        break;

    case TRACK_ACTOR_SIT:

        if (actor->user.ActorActionSet->Sit)
        {
            if (!tpoint->tag_high)
                actor->user.WaitTics = 3 * 120;
            else
                actor->user.WaitTics = tpoint->tag_high * 128;

            NewStateGroup(actor, actor->user.ActorActionSet->Sit);
        }

        break;

    case TRACK_ACTOR_DEATH1:
        if (actor->user.ActorActionSet->Death2)
        {
            actor->user.WaitTics = 4 * 120;
            NewStateGroup(actor, actor->user.ActorActionSet->Death1);
        }
        break;

    case TRACK_ACTOR_DEATH2:

        if (actor->user.ActorActionSet->Death2)
        {
            actor->user.WaitTics = 4 * 120;
            NewStateGroup(actor, actor->user.ActorActionSet->Death2);
        }

        break;

    case TRACK_ACTOR_DEATH_JUMP:

        if (actor->user.ActorActionSet->DeathJump)
        {
            actor->user.Flags |= (SPR_DEAD);
            actor->vel.X *= 2;
            actor->user.jump_speed = -495;
            DoActorBeginJump(actor);
            NewStateGroup(actor, actor->user.ActorActionSet->DeathJump);
        }

        break;

    case TRACK_ACTOR_CLOSE_ATTACK1:

        if (actor->user.ActorActionSet->CloseAttack[0])
        {
            if (!tpoint->tag_high)
                actor->user.WaitTics = 2 * 120;
            else
                actor->user.WaitTics = tpoint->tag_high * 128;

            NewStateGroup(actor, actor->user.ActorActionSet->CloseAttack[0]);
        }

        break;

    case TRACK_ACTOR_CLOSE_ATTACK2:

        if (actor->user.ActorActionSet->CloseAttack[1])
        {
            if (!tpoint->tag_high)
                actor->user.WaitTics = 4 * 120;
            else
                actor->user.WaitTics = tpoint->tag_high * 128;

            NewStateGroup(actor, actor->user.ActorActionSet->CloseAttack[1]);
        }

        break;

    case TRACK_ACTOR_ATTACK1:
    case TRACK_ACTOR_ATTACK2:
    case TRACK_ACTOR_ATTACK3:
    case TRACK_ACTOR_ATTACK4:
    case TRACK_ACTOR_ATTACK5:
    case TRACK_ACTOR_ATTACK6:
    {
        STATE* **ap = &actor->user.ActorActionSet->Attack[0] + (tpoint->tag_low - TRACK_ACTOR_ATTACK1);


        if (*ap)
        {
            if (!tpoint->tag_high)
                actor->user.WaitTics = 4 * 120;
            else
                actor->user.WaitTics = tpoint->tag_high * 128;

            NewStateGroup(actor, *ap);
        }

        break;
    }

    case TRACK_ACTOR_ZDIFF_MODE:
        if (actor->user.Flags & (SPR_ZDIFF_MODE))
        {
            actor->user.Flags &= ~(SPR_ZDIFF_MODE);
            actor->spr.pos.Z = actor->sector()->floorz;
            actor->vel.Z = 0;
        }
        else
        {
            actor->user.Flags |= (SPR_ZDIFF_MODE);
        }
        break;

    case TRACK_ACTOR_CLIMB_LADDER:

        if (actor->user.ActorActionSet->Jump)
        {
            HitInfo near;

            //
            // Get angle and x,y pos from CLIMB_MARKER
            //
            auto lActor = FindNearSprite(actor, STAT_CLIMB_MARKER);

            if (!lActor)
            {
                ActorLeaveTrack(actor);
                return false;
            }

            // determine where the player is supposed to be in relation to the ladder
            // move out in front of the ladder
            auto vec = lActor->spr.Angles.Yaw.ToVector() * 6.25;

			actor->spr.pos.SetXY(lActor->spr.pos.XY() + vec);

			actor->spr.Angles.Yaw += DAngle180;

            //
            // Get the z height to climb
            //

			double z = ActorZOfTop(actor) - (ActorSizeZ(actor) * 0.5);
            neartag(DVector3(actor->spr.pos.XY(), z), actor->sector(), actor->spr.Angles.Yaw, near, 37.5, NT_Lotag | NT_Hitag | NT_NoSpriteCheck);

            if (near.hitWall == nullptr)
            {
                ActorLeaveTrack(actor);
                return false;
            }
            auto wal = near.hitWall;

#if 0
            if (!wal->twoSided())
            {
                I_Error("Take out white wall ladder x = %d, y = %d",wal->x, wal->y);
            }
#endif

            // destination z for climbing
            if (wal->twoSided())
                actor->user.pos.Z = wal->nextSector()->floorz;
            else
                actor->user.pos.Z = wal->sectorp()->ceilingz; // don't crash on bad setups.

            DoActorZrange(actor);

            //
            // Adjust for YCENTERING
            //

            actor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
            double bos_z = ActorZOfBottom(actor);
            if (bos_z > actor->user.loz)
            {
                actor->user.pos.Y = (bos_z - actor->spr.pos.Z);
                actor->spr.pos.Z -= actor->user.pos.Y;
            }

            //
            // Misc climb setup
            //

            actor->user.Flags |= (SPR_CLIMBING);
            NewStateGroup(actor, actor->user.ActorActionSet->Climb);

            actor->vel.Z -= 1;
        }

        break;

    case TRACK_ACTOR_SET_JUMP:
        actor->user.jump_speed = -tpoint->tag_high;
        break;
    }

    return true;
}

/*

!AIC - This is where actors follow tracks.  Its massy, hard to read, and more
complex than it needs to be.  It was taken from sector object track movement
code.  The routine above ActorTrackDecide() is where a track tag is recognized
and acted upon.  There are quite a few of these that are not useful to us at
present time.

*/

int ActorFollowTrack(DSWActor* actor, short locktics)
{
    DSWPlayer* pp;

    TRACK_POINT* tpoint;
    short pnum;
	DVector3 vec(0,0,0);

    // if not on a track then better not go here
    if (actor->user.track == -1)
        return true;

    // if lying in wait for player
    if (actor->user.Flags & (SPR_WAIT_FOR_PLAYER | SPR_WAIT_FOR_TRIGGER))
    {
        if (actor->user.Flags & (SPR_WAIT_FOR_PLAYER))
        {
            TRAVERSE_CONNECT(pnum)
            {
                pp = getPlayer(pnum);

                if ((actor->spr.pos.XY() - pp->GetActor()->spr.pos.XY()).Length() < actor->user.Dist)
                {
                    actor->user.targetActor = pp->GetActor();
                    actor->user.Flags &= ~(SPR_WAIT_FOR_PLAYER);
                    return true;
                }
            }
        }

        actor->user.Tics = 0;
        return true;
    }

    // if pausing the return
    if (actor->user.WaitTics)
    {
        actor->user.WaitTics -= locktics;
        if (actor->user.WaitTics <= 0)
        {
            actor->user.Flags &= ~(SPR_DONT_UPDATE_ANG);
            NewStateGroup(actor, actor->user.ActorActionSet->Run);
            actor->user.WaitTics = 0;
        }

        return true;
    }

    tpoint = Track[actor->user.track].TrackPoint + actor->user.point;

    if (!(actor->user.Flags & (SPR_CLIMBING | SPR_DONT_UPDATE_ANG)))
    {
        actor->spr.Angles.Yaw = (tpoint->pos - actor->spr.pos).Angle();
    }

    double dist = (actor->spr.pos.XY() - tpoint->pos.XY()).Length();
    if (dist < 200 * maptoworld) // 64
    {
        if (!ActorTrackDecide(tpoint, actor))
            return true;

        // get the next point
        NextActorTrackPoint(actor);
        tpoint = Track[actor->user.track].TrackPoint + actor->user.point;

        if (!(actor->user.Flags & (SPR_CLIMBING | SPR_DONT_UPDATE_ANG)))
        {
            // calculate a new angle to the target
            actor->spr.Angles.Yaw = (tpoint->pos - actor->spr.pos).Angle();
        }

        if (actor->user.Flags & (SPR_ZDIFF_MODE))
        {
            // find the distance to the target (player)
            dist = (tpoint->pos.XY() - actor->spr.pos.XY()).Length();

            // (velocity * difference between the target and the object) /
            // distance
            actor->vel.Z = -((actor->vel.Z * (actor->spr.pos.Z - tpoint->pos.Z)) / dist);
        }
    }
    else
    {
        // make velocity approach the target velocity
        if (actor->user.Flags & (SPR_SPEED_UP))
        {
            if ((actor->user.track_vel += (locktics << actor->user.vel_rate)) >= actor->user.vel_tgt)
            {
                actor->user.track_vel = actor->user.vel_tgt;
                actor->user.Flags &= ~(SPR_SPEED_UP);
            }

            // update the real velocity
            actor->vel.X = actor->user.track_vel / 4096.;
        }
        else if (actor->user.Flags & (SPR_SLOW_DOWN))
        {
            if ((actor->user.track_vel -= (locktics << actor->user.vel_rate)) <= actor->user.vel_tgt)
            {
                actor->user.track_vel = actor->user.vel_tgt;
                actor->user.Flags &= ~(SOBJ_SLOW_DOWN);
            }
            actor->vel.X = actor->user.track_vel / 4096.;
        }


        if (actor->user.Flags & (SPR_CLIMBING))
        {
            if (ActorZOfTop(actor) + (ActorSizeZ(actor) * 0.25) < actor->user.pos.Z)
            {
                actor->user.Flags &= ~(SPR_CLIMBING);

                actor->vel.Z = 0;

                actor->spr.Angles.Yaw = (tpoint->pos - actor->spr.pos).Angle();

                ActorLeaveTrack(actor);
                actor->spr.cstat &= ~(CSTAT_SPRITE_YCENTER);
                actor->spr.pos.Z += actor->user.pos.Y;

                DoActorSetSpeed(actor, SLOW_SPEED);
                actor->user.ActorActionFunc = NinjaJumpActionFunc;
                actor->user.jump_speed = -650;
                DoActorBeginJump(actor);

                return true;
            }
        }
        else
        {
            // calculate a new x and y
			vec.SetXY(actor->spr.Angles.Yaw.ToVector() * actor->vel.X);
        }

        if (actor->vel.Z != 0)
            vec.Z = actor->vel.Z * locktics;
    }

    actor->user.coll = move_sprite(actor, vec, actor->user.ceiling_dist, actor->user.floor_dist, 0, locktics);


    if (actor->user.coll.type != kHitNone)
    {
        if (!(actor->user.Flags & (SPR_JUMPING|SPR_FALLING)))
            ActorLeaveTrack(actor);
    }


    return true;
}


#include "saveable.h"

static saveable_code saveable_track_code[] =
{
    SAVE_CODE(DoTornadoObject),
    SAVE_CODE(DoAutoTurretObject),
};

saveable_module saveable_track =
{
    // code
    saveable_track_code,
    SIZ(saveable_track_code),

    // data
    nullptr,0
};

END_SW_NS
