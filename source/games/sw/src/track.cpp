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

void DoTrack(SECTOR_OBJECT* sop, short locktics, int *nx, int *ny);
void DoAutoTurretObject(SECTOR_OBJECT* sop);
void DoTornadoObject(SECTOR_OBJECT* sop);
int PickJumpSpeed(DSWActor*, int pix_height);
DSWActor* FindNearSprite(DSWActor, short);
ANIMATOR NinjaJumpActionFunc;

#define ACTOR_STD_JUMP (-384)
int GlobSpeedSO;

// determine if moving down the track will get you closer to the player
short TrackTowardPlayer(DSWActor* actor, TRACK* t, TRACK_POINT* start_point)
{
    TRACK_POINT* end_point;
    int end_dist, start_dist;

    // determine which end of the Track we are starting from
    if (start_point == t->TrackPoint)
    {
        end_point = t->TrackPoint + t->NumPoints - 1;
    }
    else
    {
        end_point = t->TrackPoint;
    }

    end_dist = Distance(end_point->x, end_point->y, actor->spr.pos.X, actor->spr.pos.Y);
    start_dist = Distance(start_point->x, start_point->y, actor->spr.pos.X, actor->spr.pos.Y);

    if (end_dist < start_dist)
    {
        return true;
    }

    return false;

}

short TrackStartCloserThanEnd(DSWActor* actor, TRACK* t, TRACK_POINT* start_point)
{
    TRACK_POINT* end_point;
    int end_dist, start_dist;

    // determine which end of the Track we are starting from
    if (start_point == t->TrackPoint)
    {
        end_point = t->TrackPoint + t->NumPoints - 1;
    }
    else
    {
        end_point = t->TrackPoint;
    }

    end_dist = Distance(end_point->x, end_point->y, actor->spr.pos.X, actor->spr.pos.Y);
    start_dist = Distance(start_point->x, start_point->y, actor->spr.pos.X, actor->spr.pos.Y);

    if (start_dist < end_dist)
    {
        return true;
    }

    return false;

}

/*

!AIC - Looks at endpoints to figure direction of the track and the closest
point to the sprite.

*/

short ActorFindTrack(DSWActor* actor, int8_t player_dir, int track_type, int* track_point_num, int* track_dir)
{
    int dist, near_dist = 999999, zdiff;

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

        zdiff = Z(16);

        // Look at both track end points to see wich is closer
        for (i = 0; i < 2; i++)
        {
            tp = t->TrackPoint + end_point[i];

            dist = Distance(tp->x, tp->y, actor->spr.pos.X, actor->spr.pos.Y);

            if (dist < 15000 && dist < near_dist)
            {
                // make sure track start is on approximate z level - skip if
                // not
                if (labs(actor->spr.pos.Z - tp->z) > zdiff)
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
    if (near_dist < 15000)
    {
        // get the sector number of the point
        updatesector(near_tp->x, near_tp->y, &track_sect);

        // if can see the point, return the track number
        if (track_sect && FAFcansee(actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z - Z(16), actor->sector(), near_tp->x, near_tp->y, track_sect->floorz - Z(32), track_sect))
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

    tpoint->x = actor->spr.pos.X;
    tpoint->y = actor->spr.pos.Y;
    tpoint->z = actor->spr.pos.Z;
    tpoint->ang = actor->spr.ang;
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
    actorNew->spr.pos.X = actor->spr.pos.X;
    actorNew->spr.pos.Y = actor->spr.pos.Y;
    actorNew->spr.pos.Z = actor->spr.pos.Z;
    actorNew->spr.ang = actor->spr.ang;
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
        actor->spr.pos.X += MulScale(64, bcos(actor->spr.ang), 14);
        actor->spr.pos.Y += MulScale(64, bsin(actor->spr.ang), 14);
        actor->spr.lotag = lotag;
        TrackAddPoint(t, tp, actor);

        // add end point
        end_sprite->spr.pos.X += MulScale(2048, bcos(end_sprite->spr.ang), 14);
        end_sprite->spr.pos.Y += MulScale(2048, bsin(end_sprite->spr.ang), 14);
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
        start_sprite->spr.pos.X += MulScale(64, -bcos(start_sprite->spr.ang), 14);
        start_sprite->spr.pos.Y += MulScale(64, -bsin(start_sprite->spr.ang), 14);
        TrackAddPoint(t, tp, start_sprite);

        // add jump point
        actor->spr.lotag = lotag;
        TrackAddPoint(t, tp, actor);

        // add end point
        end_sprite->spr.pos.X += MulScale(64, bcos(end_sprite->spr.ang), 14);
        end_sprite->spr.pos.Y += MulScale(64, bsin(end_sprite->spr.ang), 14);
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
        end_sprite->spr.pos.X += MulScale(1024, bcos(end_sprite->spr.ang), 14);
        end_sprite->spr.pos.Y += MulScale(1024, bsin(end_sprite->spr.ang), 14);
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
        start_sprite->spr.pos.X += MOVEx(256,start_sprite->spr.ang + 1024);
        start_sprite->spr.pos.Y += MOVEy(256,start_sprite->spr.ang + 1024);
        TrackAddPoint(t, tp, start_sprite);

        // add climb point
        actor->spr.lotag = lotag;
        TrackAddPoint(t, tp, actor);

        // add end point
        end_sprite->spr.pos.X += MOVEx(512,end_sprite->spr.ang);
        end_sprite->spr.pos.Y += MOVEy(512,end_sprite->spr.ang);
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
            Printf("WARNING: Did not find first point of Track Number %d, x %d, y %d\n", ndx, itActor->spr.pos.X, itActor->spr.pos.Y);
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
            int dist, low_dist = 999999;

            // find the closest point to the last point
            it.Reset(STAT_TRACK + ndx);
            while (auto actor = it.Next())
            {
                dist = Distance((tp + t->NumPoints - 1)->x, (tp + t->NumPoints - 1)->y, actor->spr.pos.X, actor->spr.pos.Y);

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
    int xlow, ylow, xhigh, yhigh;
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

    xlow = BoundActor->spr.pos.X;
    ylow = BoundActor->spr.pos.Y;

    KillActor(BoundActor);

    BoundActor = FindBoundSprite(501 + (int(sop - SectorObject) * 5));
    if (BoundActor == nullptr)
    {
        I_Error("SOP bound sprite with hitag %d not found", 501 + (int(sop - SectorObject) * 5));
    }
    xhigh = BoundActor->spr.pos.X;
    yhigh = BoundActor->spr.pos.Y;

    KillActor(BoundActor);

    // set radius for explosion checking - based on bounding box
    child->user.Radius = ((xhigh - xlow) + (yhigh - ylow)) >> 2;
    child->user.Radius -= (child->user.Radius >> 2); // trying to get it a good size

    // search for center sprite if it exists

    BoundActor = FindBoundSprite(SECT_SO_CENTER);
    if (BoundActor)
    {
        sop->pmid.X = BoundActor->spr.pos.X;
        sop->pmid.Y = BoundActor->spr.pos.Y;
        sop->pmid.Z = BoundActor->spr.pos.Z;
        KillActor(BoundActor);
    }

#if 0
    // look for players on sector object
    PLAYER* pp;
    short pnum;
    TRAVERSE_CONNECT(pnum)
    {
        pp = &Player[pnum];

        if (pp->posx > xlow && pp->posx < xhigh && pp->posy > ylow && pp->posy < yhigh)
        {
            pp->RevolveAng = pp->angle.ang;
            pp->Revolve.X = pp->pos.X;
            pp->Revolve.Y = pp->pos.Y;
            pp->RevolveDeltaAng = 0;
            pp->Flags |= (PF_PLAYER_RIDING);

            pp->sop_riding = sop;
        }
    }
#endif


    // look through all sectors for whole sectors that are IN bounds
    for (auto&sec: sector)
    {
        auto sect = &sec;

        SectorInBounds = true;

        for(auto& wal : wallsofsector(sect))
        {
            // all walls have to be in bounds to be in sector object
            if (!(wal.pos.X > xlow && wal.pos.X < xhigh && wal.pos.Y > ylow && wal.pos.Y < yhigh))
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
            sop->zorig_ceiling[sop->num_sectors] = sect->ceilingz;

            if ((sect->extra & SECTFX_SINK))
                sop->zorig_floor[sop->num_sectors] += Z(FixedToInt(sect->depth_fixed));

            // lowest and highest floorz's
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
        for(auto& wal : wallsofsector(*sectp))
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
            if (itActor->spr.pos.X > xlow && itActor->spr.pos.X < xhigh && itActor->spr.pos.Y > ylow && itActor->spr.pos.Y < yhigh)
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
                        short ang2;
                        sop->clipdist = 0;
                        sop->clipbox_dist[sop->clipbox_num] = itActor->spr.lotag;
                        sop->clipbox_xoff[sop->clipbox_num] = sop->pmid.X - itActor->spr.pos.X;
                        sop->clipbox_yoff[sop->clipbox_num] = sop->pmid.Y - itActor->spr.pos.Y;

                        sop->clipbox_vdist[sop->clipbox_num] = ksqrt(SQ(sop->pmid.X - itActor->spr.pos.X) + SQ(sop->pmid.Y - itActor->spr.pos.Y));

                        ang2 = getangle(itActor->spr.pos.X - sop->pmid.X, itActor->spr.pos.Y - sop->pmid.Y);
                        sop->clipbox_ang[sop->clipbox_num] = getincangle(ang2, sop->ang);

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


                itActor->user.pos.X = sop->pmid.X - itActor->spr.pos.X;
                itActor->user.pos.Y = sop->pmid.Y - itActor->spr.pos.Y;
                itActor->user.pos.Z = sop->mid_sector->floorz - itActor->spr.pos.Z;

                itActor->user.Flags |= (SPR_SO_ATTACHED);

                itActor->user.sang = itActor->spr.ang;
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
        int zmid = -9999999;

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
        sop->zdelta = sop->z_tgt = 0;
        sop->wait_tics = 0;
        sop->spin_speed = 0;
        sop->spin_ang = 0;
        sop->ang_orig = 0;
        sop->clipdist = 1024;
        sop->target_dist = 0;
        sop->turn_speed = 4;
        sop->floor_loz = -9999999;
        sop->floor_hiz = 9999999;
        sop->player_xoff = sop->player_yoff = 0;
        sop->ang_tgt = sop->ang = sop->ang_moving = 0;
        sop->op_main_sector = nullptr;
        sop->ram_damage = 0;
        sop->max_damage = -9999;

        sop->scale_type = SO_SCALE_NONE;
        sop->scale_dist = 0;
        sop->scale_speed = 20;
        sop->scale_dist_min = -1024;
        sop->scale_dist_max = 1024;
        sop->scale_rand_freq = 64>>3;

        sop->scale_x_mult = 256;
        sop->scale_y_mult = 256;

        sop->morph_ang = RANDOM_P2(2048);
        sop->morph_z_speed = 20;
        sop->morph_speed = 32;
        sop->morph_dist_max = 1024;
        sop->morph_rand_freq = 64;
        sop->morph_dist = 0;
        sop->morph_xoff = 0;
        sop->morph_yoff = 0;

        sop->PreMoveAnimator = nullptr;
        sop->PostMoveAnimator = nullptr;
        sop->Animator = nullptr;
    }

    switch (tag % 5)
    {
    case TAG_OBJECT_CENTER - 500:

        sop->mid_sector = sectp;
        SectorMidPoint(sectp, &sop->pmid.X, &sop->pmid.Y, &sop->pmid.Z);

        sop->dir = 1;
        sop->track = sectp->hitag;

        // spawn a sprite to make it easier to integrate with sprite routines
        auto actorNew = SpawnActor(STAT_SO_SP_CHILD, 0, nullptr, sectp,
                          sop->pmid.X, sop->pmid.Y, sop->pmid.Z, 0, 0);
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
                    sop->scale_point_base_speed = SP_TAG2(actor);
                    for (j = 0; j < (int)SIZ(sop->scale_point_speed); j++)
                    {
                        sop->scale_point_speed[j] = SP_TAG2(actor);
                    }

                    if (SP_TAG4(actor))
                        sop->scale_point_rand_freq = (uint8_t)SP_TAG4(actor);
                    else
                        sop->scale_point_rand_freq = 64;

                    sop->scale_point_dist_min = -SP_TAG5(actor);
                    sop->scale_point_dist_max = SP_TAG6(actor);
                    KillActor(actor);
                    break;

                case SO_SCALE_INFO:
                    sop->flags |= (SOBJ_DYNAMIC);
                    sop->scale_speed = SP_TAG2(actor);
                    sop->scale_dist_min = -SP_TAG5(actor);
                    sop->scale_dist_max = SP_TAG6(actor);

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
                    if (actor->spr.clipdist == 3)
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
                    sop->spin_speed = 16;
                    sop->last_ang = sop->ang;
                    // animators
                    sop->Animator = DoTornadoObject;
                    sop->PreMoveAnimator = ScaleSectorObject;
                    sop->PostMoveAnimator = MorphTornado;
                    // clip
                    sop->clipdist = 2500;
                    // morph point
                    sop->morph_speed = 16;
                    sop->morph_z_speed = 6;
                    sop->morph_dist_max = 1024;
                    sop->morph_rand_freq = 8;
                    sop->scale_dist_min = -768;
                    KillActor(actor);
                    break;
                case SO_FLOOR_MORPH:
                    if (SW_SHAREWARE) break;
                    sop->flags |= (SOBJ_DYNAMIC);
                    sop->scale_type = SO_SCALE_NONE;
                    sop->morph_speed = 120;
                    sop->morph_z_speed = 7;
                    sop->PostMoveAnimator = MorphFloor;
                    sop->morph_dist_max = 4000;
                    sop->morph_rand_freq = 8;
                    KillActor(actor);
                    break;

                case SO_AMOEBA:
                    sop->flags |= (SOBJ_DYNAMIC);
                    //sop->scale_type = SO_SCALE_CYCLE;
                    sop->scale_type = SO_SCALE_RANDOM_POINT;
                    sop->PreMoveAnimator = ScaleSectorObject;

                    memset(sop->scale_point_dist,0,sizeof(sop->scale_point_dist));;
                    sop->scale_point_base_speed = SCALE_POINT_SPEED;
                    for (j = 0; j < (int)SIZ(sop->scale_point_speed); j++)
                        sop->scale_point_speed[j] = SCALE_POINT_SPEED;

                    sop->scale_point_dist_min = -256;
                    sop->scale_point_dist_max = 256;
                    sop->scale_point_rand_freq = 32;
                    KillActor(actor);
                    break;
                case SO_MAX_DAMAGE:
                    actorNew->user.MaxHealth = SP_TAG2(actor);
                    if (SP_TAG5(actor) != 0)
                        sop->max_damage = SP_TAG5(actor);
                    else
                        sop->max_damage = actorNew->user.MaxHealth;

                    switch (actor->spr.clipdist)
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

                    sop->drive_angspeed = SP_TAG2(actor);
                    sop->drive_angspeed <<= 5;
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
                    sop->clipdist = actor->spr.lotag;
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
                    sop->limit_ang_center = actor->spr.ang;
                    sop->limit_ang_delta = actor->spr.lotag;
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
                    if (sop->spin_speed)
                        break;
                    sop->spin_speed = actor->spr.lotag;
                    sop->last_ang = sop->ang;
                    KillActor(actor);
                    break;
                case SO_ANGLE:
                    sop->ang = sop->ang_moving = actor->spr.ang;
                    sop->last_ang = sop->ang_orig = sop->ang;
                    sop->spin_ang = 0;
                    KillActor(actor);
                    break;
                case SO_SPIN_REVERSE:

                    sop->spin_speed = actor->spr.lotag;
                    sop->last_ang = sop->ang;

                    if (sop->spin_speed >= 0)
                        sop->spin_speed = -sop->spin_speed;

                    KillActor(actor);
                    break;
                case SO_BOB_START:
                    sop->bob_amt = Z(actor->spr.lotag);
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
                sop->bob_amt = Z(2);
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
        int low_dist = 999999, dist;
        SECTOR_OBJECT* sop = &SectorObject[i];
        TRACK_POINT* tpoint = nullptr;

        if (SO_EMPTY(sop))
            continue;


        // save off the original x and y locations of the walls AND sprites
        sop->num_walls = 0;
        for (j = 0; sop->sectp[j] != nullptr; j++)
        {

            // move all walls in sectors
            for (auto& wal : wallsofsector(sop->sectp[j]))
            {
                sop->xorig[sop->num_walls] = sop->pmid.X - wal.pos.X;
                sop->yorig[sop->num_walls] = sop->pmid.Y - wal.pos.Y;
                sop->num_walls++;
            }
        }

        ASSERT((uint16_t)sop->num_walls < SIZ(sop->xorig));

        if (sop->track <= -1)
            continue;

        if (sop->track >= SO_OPERATE_TRACK_START)
            continue;

        found = false;
        // find the closest point on the track and put SOBJ on it
        for (j = 0; j < Track[sop->track].NumPoints; j++)
        {
            tpoint = Track[sop->track].TrackPoint;

            dist = Distance((tpoint + j)->x, (tpoint + j)->y, sop->pmid.X, sop->pmid.Y);

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

        sop->ang = getangle((tpoint + sop->point)->x - sop->pmid.X, (tpoint + sop->point)->y - sop->pmid.Y);

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
        int low_dist = 999999, dist;

        tag = actor->spr.lotag;

        if (tag < TAG_ACTOR_TRACK_BEGIN || tag > TAG_ACTOR_TRACK_END)
            continue;

        // setup sprite track defaults
        actor->user.track = tag - TAG_ACTOR_TRACK_BEGIN;

        // if facing left go backward
        if (actor->spr.ang >= 513 && actor->spr.ang <= 1535)
        {
            actor->user.track_dir = -1;
        }
        else
        {
            actor->user.track_dir = 1;
        }

        actor->user.track_vel = actor->spr.xvel * 256;
        actor->user.vel_tgt = actor->user.track_vel;
        actor->user.vel_rate = 6;

        // find the closest point on the track and put SOBJ on it
        for (j = 0; j < Track[actor->user.track].NumPoints; j++)
        {
            tpoint = Track[actor->user.track].TrackPoint;

            dist = Distance((tpoint + j)->x, (tpoint + j)->y, actor->spr.pos.X, actor->spr.pos.Y);

            if (dist < low_dist)
            {
                low_dist = dist;
                actor->user.point = j;
            }
        }

        NextActorTrackPoint(actor);

        if (Track[actor->user.track].NumPoints == 0)
        {
            Printf("WARNING: Sprite %d (%d, %d) placed on track %d with no points!\n", actor->GetIndex(), actor->spr.pos.X, actor->spr.pos.Y, actor->user.track);
            continue;
        }

        // check angle in the "forward" direction
        actor->spr.ang = getangle((tpoint + actor->user.point)->x - actor->spr.pos.X, (tpoint + actor->user.point)->y - actor->spr.pos.Y);
    }
}


void MovePlayer(PLAYER* pp, SECTOR_OBJECT* sop, int nx, int ny)
{
    void DoPlayerZrange(PLAYER* pp);

    // make sure your standing on the so
    if (pp->Flags & (PF_JUMPING | PF_FALLING | PF_FLYING))
        return;

    pp->sop_riding = sop;

    // if player has NOT moved and player is NOT riding
    // set up the player for riding
    if (!(pp->Flags & PF_PLAYER_MOVED) && !(pp->Flags & PF_PLAYER_RIDING))
    {
        pp->Flags |= (PF_PLAYER_RIDING);

        pp->RevolveAng = pp->angle.ang;
        pp->Revolve.X = pp->pos.X;
        pp->Revolve.Y = pp->pos.Y;

        // set the delta angle to 0 when moving
        pp->RevolveDeltaAng = 0;
    }

    pp->pos.X += nx;
    pp->pos.Y += ny;

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
        pp->RevolveAng = pp->angle.ang;
        pp->Revolve.X = pp->pos.X;
        pp->Revolve.Y = pp->pos.Y;

        // set the delta angle to 0 when moving
        pp->RevolveDeltaAng = 0;
    }
    else
    {
        // Player is NOT moving

        // Move saved x&y variables
        pp->Revolve.X += nx;
        pp->Revolve.Y += ny;

        // Last known angle is now adjusted by the delta angle
        pp->RevolveAng = pp->angle.ang - buildang(pp->RevolveDeltaAng);
    }

    // increment Players delta angle
    pp->RevolveDeltaAng = NORM_ANGLE(pp->RevolveDeltaAng + GlobSpeedSO);

    rotatepoint(sop->pmid.vec2, *(vec2_t *)&pp->Revolve.X, pp->RevolveDeltaAng, &pp->pos.vec2);

    // THIS WAS CAUSING PROLEMS!!!!
    // Sectors are still being manipulated so you can end up in a void (-1) sector

    // New angle is formed by taking last known angle and
    // adjusting by the delta angle
    pp->angle.addadjustment(pp->angle.ang - (pp->RevolveAng + buildang(pp->RevolveDeltaAng)));

    UpdatePlayerSprite(pp);
}

void MovePoints(SECTOR_OBJECT* sop, short delta_ang, int nx, int ny)
{
    int j;
    vec2_t rxy;
    int pnum;
    PLAYER* pp;
    sectortype* *sectp;
    int i, rot_ang;
    bool PlayerMove = true;

    if (sop->pmid.X >= MAXSO)
        PlayerMove = false;

    // move along little midpoint
    sop->pmid.X += nx;
    sop->pmid.Y += ny;

    if (sop->pmid.X >= MAXSO)
        PlayerMove = false;

    // move child sprite along also
    sop->sp_child->spr.pos.X = sop->pmid.X;
    sop->sp_child->spr.pos.Y = sop->pmid.Y;


    // setting floorz if need be
    if ((sop->flags & SOBJ_ZMID_FLOOR))
        sop->pmid.Z = sop->mid_sector->floorz;

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if ((sop->flags & (SOBJ_SPRITE_OBJ | SOBJ_DONT_ROTATE)))
            goto PlayerPart;

        // move all walls in sectors
        for(auto& wal : wallsofsector(*sectp))
        {
            if ((wal.extra & (WALLFX_LOOP_DONT_SPIN | WALLFX_DONT_MOVE)))
                continue;

            if (wal.extra && (wal.extra & WALLFX_LOOP_OUTER))
            {
                dragpoint(&wal, wal.pos.X + nx, wal.pos.Y + ny);
            }
            else
            {
                wal.move(wal.pos.X + nx, wal.pos.Y + ny);
            }

            rot_ang = delta_ang;

            if ((wal.extra & WALLFX_LOOP_REVERSE_SPIN))
                rot_ang = -delta_ang;

            if ((wal.extra & WALLFX_LOOP_SPIN_2X))
                rot_ang = NORM_ANGLE(rot_ang * 2);

            if ((wal.extra & WALLFX_LOOP_SPIN_4X))
                rot_ang = NORM_ANGLE(rot_ang * 4);

            rotatepoint(sop->pmid.vec2, wal.pos, rot_ang, &rxy);

            if (wal.extra && (wal.extra & WALLFX_LOOP_OUTER))
            {
                dragpoint(&wal, rxy.X, rxy.Y);
            }
            else
            {
                wal.move(rxy.X, rxy.Y);
            }
        }

PlayerPart:

        TRAVERSE_CONNECT(pnum)
        {
            pp = Player + pnum;

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
                    MovePlayer(pp, sop, nx, ny);
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
            pp = Player + pnum;

            if (pp->lowActor && pp->lowActor == actor)
            {
                if (PlayerMove)
                    MovePlayer(pp, sop, nx, ny);
            }
        }

        actor->spr.pos.X = sop->pmid.X - actor->user.pos.X;
        actor->spr.pos.Y = sop->pmid.Y - actor->user.pos.Y;

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

        int16_t oldang = actor->spr.ang;
        actor->spr.ang = actor->user.sang;

        if (actor->user.Flags & (SPR_ON_SO_SECTOR))
        {
            if ((sop->flags & SOBJ_DONT_ROTATE))
                continue;

            // IS part of a sector, sprite can do things based on the
            // current sector it is in
            if ((actor->sector()->firstWall()->extra & WALLFX_LOOP_DONT_SPIN))
                continue;

            if ((actor->sector()->firstWall()->extra & WALLFX_LOOP_REVERSE_SPIN))
            {
                rotatepoint(sop->pmid.vec2, actor->spr.pos.vec2, -delta_ang, &actor->spr.pos.vec2);
                actor->spr.ang = NORM_ANGLE(actor->spr.ang - delta_ang);
            }
            else
            {
                rotatepoint(sop->pmid.vec2, actor->spr.pos.vec2, delta_ang, &actor->spr.pos.vec2);
                actor->spr.ang = NORM_ANGLE(actor->spr.ang + delta_ang);
            }

        }
        else
        {
            if (!(sop->flags & SOBJ_DONT_ROTATE))
            {
                // NOT part of a sector - independant of any sector
                rotatepoint(sop->pmid.vec2, actor->spr.pos.vec2, delta_ang, &actor->spr.pos.vec2);
                actor->spr.ang = NORM_ANGLE(actor->spr.ang + delta_ang);
            }

            // Does not necessarily move with the sector so must accout for
            // moving across sectors
            if (sop->pmid.X < MAXSO) // special case for operating SO's
                SetActorZ(sop->so_actors[i], &actor->spr.pos);
        }

        actor->user.oangdiff += getincangle(oldang, actor->spr.ang);

        if ((actor->spr.extra & SPRX_BLADE))
        {
            DoBladeDamage(sop->so_actors[i]);
        }
    }

    TRAVERSE_CONNECT(pnum)
    {
        pp = Player + pnum;

        // if player was on a sector object
        if (pp->sop_riding)
        {
            // update here AFTER sectors/player has been manipulated
            // prevents you from falling into map HOLEs created by moving
            // Sectors and sprites around.
            //if (sop->xmid < MAXSO)
            updatesector(pp->pos.X, pp->pos.Y, &pp->cursector);

            // in case you are in a whirlpool
            // move perfectly with the ride in the z direction
            if (pp->Flags & PF_CRAWLING)
            {
                // move up some for really fast moving plats
                //pp->posz -= PLAYER_HEIGHT + Z(12);
                DoPlayerZrange(pp);
                pp->pos.Z = pp->loz - PLAYER_CRAWL_HEIGHT;
                pp->actor->spr.pos.Z = pp->loz;
            }
            else
            {
                // move up some for really fast moving plats
                //pp->posz -= Z(24);
                DoPlayerZrange(pp);

                if (!(pp->Flags & (PF_JUMPING | PF_FALLING | PF_FLYING)))
                {
                    pp->pos.Z = pp->loz - PLAYER_HEIGHT;
                    pp->actor->spr.pos.Z = pp->loz;
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

void RefreshPoints(SECTOR_OBJECT* sop, int nx, int ny, bool dynamic)
{
    short wallcount = 0, delta_ang_from_orig;
    short ang;
    int dx,dy,x,y;

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
            for (auto& wal : wallsofsector(*sectp))
            {
                if (!(wal.extra && (wal.extra & WALLFX_DONT_MOVE)))
                {
                    dx = x = sop->pmid.X - sop->xorig[wallcount];
                    dy = y = sop->pmid.Y - sop->yorig[wallcount];

                    if (dynamic && sop->scale_type)
                    {
                        if (!(wal.extra & WALLFX_DONT_SCALE))
                        {
                            ang = NORM_ANGLE(getangle(x - sop->pmid.X, y - sop->pmid.Y));

                            if (sop->scale_type == SO_SCALE_RANDOM_POINT)
                            {
                                // was causing memory overwrites
                                //ScaleRandomPoint(sop, k, ang, x, y, &dx, &dy);
                                ScaleRandomPoint(sop, wallcount, ang, x, y, &dx, &dy);
                            }
                            else
                            {
                                int xmul = (sop->scale_dist * sop->scale_x_mult)>>8;
                                int ymul = (sop->scale_dist * sop->scale_y_mult)>>8;

                                dx = x + MulScale(xmul, bcos(ang), 14);
                                dy = y + MulScale(ymul, bsin(ang), 14);
                            }
                        }
                    }

                    if (wal.extra && (wal.extra & WALLFX_LOOP_OUTER))
                    {
                        dragpoint(&wal, dx, dy);
                    }
                    else
                    {
                        wal.move(dx, dy);
                    }
                }

                wallcount++;
            }
        }
    }

    if (sop->spin_speed)
    {
        // same as below - ignore the objects angle
        // last_ang is the last true angle before SO started spinning
        delta_ang_from_orig = NORM_ANGLE(sop->last_ang + sop->spin_ang - sop->ang_orig);
    }
    else
    {
        // angle traveling + the new spin angle all offset from the original
        // angle
        delta_ang_from_orig = NORM_ANGLE(sop->ang + sop->spin_ang - sop->ang_orig);
    }

    // Note that this delta angle is from the original angle
    // nx,ny are 0 so the points are not moved, just rotated
    MovePoints(sop, delta_ang_from_orig, nx, ny);

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

        SetActorZ(actor, &actor->spr.pos);
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
            for (auto& wal : wallsofsector(*sectp))
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


void CollapseSectorObject(SECTOR_OBJECT* sop, int nx, int ny)
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
            for (auto& wal : wallsofsector(*sectp))
            {
                if ((wal.extra & WALLFX_DONT_MOVE))
                    continue;

                if (wal.extra && (wal.extra & WALLFX_LOOP_OUTER))
                {
                    dragpoint(&wal, nx, ny);
                }
                else
                {
                    wal.move(nx, ny);
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
        sop->bob_diff = MulScale(sop->bob_amt, bsin(sop->bob_sine_ndx), 14);

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
        if (ap->animtype == ANIM_Floorz && ap->animindex == sectnum(sop->sectp[i]))
        {
            destsect = sop->sectp[i];
            break;
        }
    }

    ASSERT(destsect != nullptr);

    destsect->floorpicnum = srcsect->floorpicnum;
    destsect->floorshade = srcsect->floorshade;
//    destsect->floorz = srcsect->floorz;

    destsect->floorstat &= ~(CSTAT_SECTOR_ALIGN);
    destsect->u_defined = true;
    ASSERT(srcsect->hasU());

    tgt_depth = FixedToInt(srcsect->depth_fixed);

    for(auto& sect: sector)
    {
        if (&sect == destsect)
        {
            ndx = AnimSet(ANIM_SUdepth, destsect, IntToFixed(tgt_depth), (ap->vel << 8) >> 8);
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
        ndx = AnimSet(ANIM_Userz, 0, actor, -actor->user.pos.Z - ActorSizeZ(actor) - Z(100), ap->vel>>8);
        AnimSetVelAdj(ndx, ap->vel_adj);
    }


    // Take out any blocking walls
    for(auto& wal : wallsofsector(destsect))
    {
        wal.cstat &= ~(CSTAT_WALL_BLOCK);
    }

    return;
}


void MoveSectorObjects(SECTOR_OBJECT* sop, short locktics)
{
    int nx, ny;
    short speed;
    short delta_ang;

    so_setinterpolationtics(sop, locktics);

    if (sop->track >= SO_OPERATE_TRACK_START)
    {
        if ((sop->flags & SOBJ_UPDATE_ONCE))
        {
            sop->flags &= ~(SOBJ_UPDATE_ONCE);
            RefreshPoints(sop, 0, 0, false);
        }
        return;
    }

    nx = 0;
    ny = 0;

    // if pausing the return
    if (sop->wait_tics)
    {
        sop->wait_tics -= locktics;
        if (sop->wait_tics <= 0)
            sop->wait_tics = 0;

        return;
    }

    delta_ang = 0;

    if (sop->track > -1)
        DoTrack(sop, locktics, &nx, &ny);

    // get delta to target angle
    delta_ang = getincangle(sop->ang, sop->ang_tgt);

    sop->ang = NORM_ANGLE(sop->ang + (delta_ang >> sop->turn_speed));
    delta_ang = delta_ang >> sop->turn_speed;

    // move z values
    MoveZ(sop);

    // calculate the spin speed
    speed = sop->spin_speed * locktics;
    // spin_ang is incremented by the spin_speed
    sop->spin_ang = NORM_ANGLE(sop->spin_ang + speed);

    if (sop->spin_speed)
    {
        // ignore delta angle if spinning
        GlobSpeedSO = speed;
    }
    else
    {
        // The actual delta from the last frame
        GlobSpeedSO = speed;
        GlobSpeedSO += delta_ang;
    }

    if ((sop->flags & SOBJ_DYNAMIC))
    {
        // trick tricks
        RefreshPoints(sop, nx, ny, true);
    }
    else
    {
        // Update the points so there will be no warping
        if ((sop->flags & (SOBJ_UPDATE|SOBJ_UPDATE_ONCE)) ||
            sop->vel ||
            (sop->ang != sop->ang_tgt) ||
            GlobSpeedSO)
        {
            sop->flags &= ~(SOBJ_UPDATE_ONCE);
            RefreshPoints(sop, nx, ny, false);
        }
    }
}

void DoTrack(SECTOR_OBJECT* sop, short locktics, int *nx, int *ny)
{
    TRACK_POINT* tpoint;
    int dx, dy, dz;
    int dist;

    tpoint = Track[sop->track].TrackPoint + sop->point;

    // calculate an angle to the target

    if (sop->vel)
        sop->ang_moving = sop->ang_tgt = getangle(tpoint->x - sop->pmid.X, tpoint->y - sop->pmid.Y);

    // NOTE: Jittery ride - try new value out here
    // NOTE: Put a loop around this (locktics) to make it more acuruate
    const int TRACK_POINT_SIZE = 200;
    if (sop->target_dist < 100)
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
            if (sop->spin_speed)
                break;

            sop->spin_speed = tpoint->tag_high;
            sop->last_ang = sop->ang;
            break;

        case TRACK_SPIN_REVERSE:
        {
            if (!sop->spin_speed)
                break;

            if (sop->spin_speed >= 0)
            {
                sop->spin_speed = -sop->spin_speed;
            }
        }
        break;

        case TRACK_SPIN_STOP:
            if (!sop->spin_speed)
                break;

            sop->spin_speed = 0;
            break;

        case TRACK_BOB_START:
            sop->flags |= (SOBJ_ZMID_FLOOR);
            sop->bob_amt = Z(tpoint->tag_high);
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
                AnimSetVelAdj(ndx, 6);
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
                        AnimSet(ANIM_Floorz, *sectp, (*sectp)->floorz + Z((*sectp)->height), 128);
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
            AnimSet(ANIM_SopZ, int(sop-SectorObject), nullptr, tpoint->z, zr);

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

            sop->vel = sop->spin_speed = 0;
            // only set event if non-zero
            if (tpoint->tag_high)
                sop->match_event = tpoint->tag_high;
            tpoint->tag_high = -1;
            break;
        }

        case TRACK_ZDIFF_MODE:
            sop->flags |= (SOBJ_ZDIFF_MODE);
            sop->zdelta = Z(tpoint->tag_high);
            break;
        case TRACK_ZRATE:
            sop->z_rate = Z(tpoint->tag_high);
            break;
        case TRACK_ZUP:
            sop->flags &= ~(SOBJ_ZDOWN | SOBJ_ZUP);
            if (sop->dir < 0)
            {
                sop->z_tgt = sop->z_tgt + Z(tpoint->tag_high);
                sop->flags |= (SOBJ_ZDOWN);
            }
            else
            {
                sop->z_tgt = sop->z_tgt - Z(tpoint->tag_high);
                sop->flags |= (SOBJ_ZUP);
            }
            break;
        case TRACK_ZDOWN:
            sop->flags &= ~(SOBJ_ZDOWN | SOBJ_ZUP);
            if (sop->dir > 0)
            {
                sop->z_tgt = sop->z_tgt + Z(tpoint->tag_high);
                sop->flags |= (SOBJ_ZDOWN);
            }
            else
            {
                sop->z_tgt = sop->z_tgt - Z(tpoint->tag_high);
                sop->flags |= (SOBJ_ZUP);
            }
            break;
        }

        // get the next point
        NextTrackPoint(sop);
        tpoint = Track[sop->track].TrackPoint + sop->point;

        // calculate distance to target poing
        sop->target_dist = Distance(sop->pmid.X, sop->pmid.Y, tpoint->x, tpoint->y);

        // calculate a new angle to the target
        sop->ang_moving = sop->ang_tgt = getangle(tpoint->x - sop->pmid.X, tpoint->y - sop->pmid.Y);

        if ((sop->flags & SOBJ_ZDIFF_MODE))
        {
            short i;

            // set dx,dy,dz up for finding the z magnitude
            dx = tpoint->x;
            dy = tpoint->y;
            dz = tpoint->z - sop->zdelta;

            // find the distance to the target (player)
            dist = DIST(dx, dy, sop->pmid.X, sop->pmid.Y);

            // (velocity * difference between the target and the object)
            // / distance
            sop->z_rate = (sop->vel * (sop->pmid.Z - dz)) / dist;

            // take absolute value and convert to pixels (divide by 256)
            sop->z_rate = PIXZ(labs(sop->z_rate));

            if ((sop->flags & SOBJ_SPRITE_OBJ))
            {
                // only modify zmid for sprite_objects
                AnimSet(ANIM_SopZ, int(sop - SectorObject), nullptr, dz, sop->z_rate);
            }
            else
            {
                // churn through sectors setting their new z values
                for (i = 0; sop->sectp[i] != nullptr; i++)
                {
                    AnimSet(ANIM_Floorz, sop->sectp[i], dz - (sop->mid_sector->floorz - sop->sectp[i]->floorz), sop->z_rate);
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
        *nx = ((sop->vel) >> 8) * locktics * bcos(sop->ang_moving) >> 14;
        *ny = ((sop->vel) >> 8) * locktics * bsin(sop->ang_moving) >> 14;

        dist = Distance(sop->pmid.X, sop->pmid.Y, sop->pmid.X + *nx, sop->pmid.Y + *ny);
        sop->target_dist -= dist;
    }
}


void OperateSectorObjectForTics(SECTOR_OBJECT* sop, short newang, int newx, int newy, short locktics)
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
        sop->bob_diff = MulScale(sop->bob_amt, bsin(sop->bob_sine_ndx), 14);

        // for all sectors
        for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
        {
            if (sop->sectp[i]->hasU() && (sop->sectp[i]->flags & SECTFU_SO_DONT_BOB))
                continue;

            (*sectp)->setfloorz(sop->zorig_floor[i] + sop->bob_diff);
        }
    }

    GlobSpeedSO = 0;

    //sop->ang_tgt = newang;
    sop->ang_moving = newang;

    sop->spin_ang = 0;
    sop->ang = newang;

    RefreshPoints(sop, newx - sop->pmid.X, newy - sop->pmid.Y, false);
}

void OperateSectorObject(SECTOR_OBJECT* sop, short newang, int newx, int newy)
{
    OperateSectorObjectForTics(sop, newang, newx, newy, synctics);
}

void PlaceSectorObject(SECTOR_OBJECT* sop, int newx, int newy)
{
    so_setinterpolationtics(sop, synctics);
    RefreshPoints(sop, newx - sop->pmid.X, newy - sop->pmid.Y, false);
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
                if (actor->spr.clipdist == 3)
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


void KillSectorObject(SECTOR_OBJECT* sop)
{
    int newx = MAXSO;
    int newy = MAXSO;
    short newang = 0;

    if (sop->track < SO_OPERATE_TRACK_START)
        return;

    sop->ang_tgt = sop->ang_moving = newang;

    sop->spin_ang = 0;
    sop->ang = sop->ang_tgt;

    RefreshPoints(sop, newx - sop->pmid.X, newy - sop->pmid.Y, false);
}


void TornadoSpin(SECTOR_OBJECT* sop)
{
    short delta_ang, speed;
    short locktics = synctics;

    // get delta to target angle
    delta_ang = getincangle(sop->ang, sop->ang_tgt);

    sop->ang = NORM_ANGLE(sop->ang + (delta_ang >> sop->turn_speed));
    delta_ang = delta_ang >> sop->turn_speed;

    // move z values
    MoveZ(sop);

    // calculate the spin speed
    speed = sop->spin_speed * locktics;
    // spin_ang is incremented by the spin_speed
    sop->spin_ang = NORM_ANGLE(sop->spin_ang + speed);

    if (sop->spin_speed)
    {
        // ignore delta angle if spinning
        GlobSpeedSO = speed;
    }
    else
    {
        // The actual delta from the last frame
        GlobSpeedSO = speed;
        GlobSpeedSO += delta_ang;
    }
}

void DoTornadoObject(SECTOR_OBJECT* sop)
{
    int xvect,yvect;
    // this made them move together more or less - cool!
    //static short ang = 1024;
    int floor_dist;
    vec3_t pos;
    int ret;
    short *ang = &sop->ang_moving;

    xvect = sop->vel * bcos(*ang);
    yvect = sop->vel * bcos(*ang);

    auto cursect = sop->op_main_sector; // for sop->vel
    floor_dist = (abs(cursect->ceilingz - cursect->floorz)) >> 2;
    pos.X = sop->pmid.X;
    pos.Y = sop->pmid.Y;
    pos.Z = floor_dist;

    PlaceSectorObject(sop, MAXSO, MAXSO);
    Collision coll;
    clipmove(pos, &cursect, xvect, yvect, (int)sop->clipdist, Z(0), floor_dist, CLIPMASK_ACTOR, coll);

    if (coll.type != kHitNone)
    {
        *ang = NORM_ANGLE(*ang + 1024 + RANDOM_P2(512) - 256);
    }

    TornadoSpin(sop);
    RefreshPoints(sop, pos.X - sop->pmid.X, pos.Y - sop->pmid.Y, true);
}

void DoAutoTurretObject(SECTOR_OBJECT* sop)
{
    DSWActor* actor = sop->sp_child;
    if (!actor) return;

    short delta_ang;
    int diff;
    short i;

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
		for (i = 0; sop->so_actors[i] != nullptr; i++)
        {
            DSWActor* sActor = sop->so_actors[i];
            if (!sActor) continue;

			if (sActor->spr.statnum == STAT_SO_SHOOT_POINT)
            {
                if (!FAFcansee(sActor->spr.pos.X, sActor->spr.pos.Y, sActor->spr.pos.Z-Z(4), sActor->sector(),
                               actor->user.targetActor->spr.pos.X, actor->user.targetActor->spr.pos.Y, ActorUpperZ(actor->user.targetActor), actor->user.targetActor->sector()))
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
			for (i = 0; sop->so_actors[i] != nullptr; i++)
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

        sop->ang_tgt = getangle(actor->user.targetActor->spr.pos.X - sop->pmid.X,  actor->user.targetActor->spr.pos.Y - sop->pmid.Y);

        // get delta to target angle
        delta_ang = getincangle(sop->ang, sop->ang_tgt);

        //sop->ang += delta_ang >> 4;
        sop->ang = NORM_ANGLE(sop->ang + (delta_ang >> 3));
        //sop->ang += delta_ang >> 2;

        if (sop->limit_ang_center >= 0)
        {
            diff = getincangle(sop->limit_ang_center, sop->ang);

            if (labs(diff) >= sop->limit_ang_delta)
            {
                if (diff < 0)
                    sop->ang = sop->limit_ang_center - sop->limit_ang_delta;
                else
                    sop->ang = sop->limit_ang_center + sop->limit_ang_delta;

            }
        }

        OperateSectorObjectForTics(sop, sop->ang, sop->pmid.X, sop->pmid.Y, 2*synctics);
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
            actor->spr.ang = NORM_ANGLE(getangle((Track[actor->user.track].TrackPoint + actor->user.point)->x - actor->spr.pos.X, (Track[actor->user.track].TrackPoint + actor->user.point)->y - actor->spr.pos.Y));
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
            actor->spr.ang = NORM_ANGLE(getangle((Track[actor->user.track].TrackPoint + actor->user.point)->x - actor->spr.pos.X, (Track[actor->user.track].TrackPoint + actor->user.point)->y - actor->spr.pos.Y));
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
        actor->user.Dist = tpoint->tag_high;
        break;
    }

    case TRACK_ACTOR_WAIT_FOR_TRIGGER:
    {
        actor->user.Flags |= (SPR_WAIT_FOR_TRIGGER);
        actor->user.Dist = tpoint->tag_high;
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
            actor->spr.ang = tpoint->ang;

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

            actor->spr.ang = tpoint->ang;


            ActorLeaveTrack(actor);

            if (tpoint->tag_high)
            {
                actor->user.jump_speed = -tpoint->tag_high;
            }
            else
            {
                actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);

                FAFhitscan(actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z - Z(24), actor->sector(),      // Start position
                           bcos(actor->spr.ang),    // X vector of 3D ang
                           bsin(actor->spr.ang),    // Y vector of 3D ang
                           0,                // Z vector of 3D ang
                           hit, CLIPMASK_MISSILE);

                actor->spr.cstat |= (CSTAT_SPRITE_BLOCK);

                ASSERT(hit.hitSector != nullptr);

                if (hit.actor() != nullptr)
                    return false;

                if (hit.hitWall == nullptr)
                    return false;

                if (!hit.hitWall->twoSided())
                    return false;

                zdiff = labs(actor->spr.pos.Z - hit.hitWall->nextSector()->floorz) >> 8;

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
            actor->spr.ang = tpoint->ang;

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
            actor->spr.ang = tpoint->ang;

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
        HitInfo near;
        int z[2];
        int i;

        if (actor->user.Rot == actor->user.ActorActionSet->Sit || actor->user.Rot == actor->user.ActorActionSet->Stand)
            return false;

        actor->spr.ang = tpoint->ang;

        z[0] = actor->spr.pos.Z - ActorSizeZ(actor) + Z(5);
        z[1] = actor->spr.pos.Z - (ActorSizeZ(actor) >> 1);

        for (i = 0; i < (int)SIZ(z); i++)
        {
            neartag({ actor->spr.pos.X, actor->spr.pos.Y, z[i] }, actor->sector(), actor->spr.ang, near, 1024, NTAG_SEARCH_LO_HI);

            if (near.actor() != nullptr && near.hitpos.X < 1024)
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

        if (near.hitSector != nullptr && near.hitpos.X < 1024)
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
            actor->spr.xvel <<= 1;
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
            actor->spr.zvel = 0;
        }
        else
        {
            actor->user.Flags |= (SPR_ZDIFF_MODE);
        }
        break;

    case TRACK_ACTOR_CLIMB_LADDER:

        if (actor->user.ActorActionSet->Jump)
        {
            int bos_z,nx,ny;
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
            nx = MOVEx(100, lActor->spr.ang);
            ny = MOVEy(100, lActor->spr.ang);

            actor->spr.pos.X = lActor->spr.pos.X + nx;
            actor->spr.pos.Y = lActor->spr.pos.Y + ny;

            actor->spr.ang = NORM_ANGLE(lActor->spr.ang + 1024);

            //
            // Get the z height to climb
            //

            neartag({ actor->spr.pos.X, actor->spr.pos.Y, ActorZOfTop(actor) - (ActorSizeZ(actor) >> 1) }, actor->sector(), actor->spr.ang, near, 600, NTAG_SEARCH_LO_HI);

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
            bos_z = ActorZOfBottom(actor);
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

            actor->spr.zvel = -Z(1);
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
    PLAYER* pp;

    TRACK_POINT* tpoint;
    short pnum;
    int nx = 0, ny = 0, nz = 0, dx, dy, dz;
    int dist;

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
                pp = &Player[pnum];

                if (Distance(actor->spr.pos.X, actor->spr.pos.Y, pp->pos.X, pp->pos.Y) < actor->user.Dist)
                {
                    actor->user.targetActor = pp->actor;
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
        actor->spr.ang = getangle(tpoint->x - actor->spr.pos.X, tpoint->y - actor->spr.pos.Y);
    }

    if ((dist = Distance(actor->spr.pos.X, actor->spr.pos.Y, tpoint->x, tpoint->y)) < 200) // 64
    {
        if (!ActorTrackDecide(tpoint, actor))
            return true;

        // get the next point
        NextActorTrackPoint(actor);
        tpoint = Track[actor->user.track].TrackPoint + actor->user.point;

        if (!(actor->user.Flags & (SPR_CLIMBING | SPR_DONT_UPDATE_ANG)))
        {
            // calculate a new angle to the target
            actor->spr.ang = getangle(tpoint->x - actor->spr.pos.X, tpoint->y - actor->spr.pos.Y);
        }

        if (actor->user.Flags & (SPR_ZDIFF_MODE))
        {
            // set dx,dy,dz up for finding the z magnitude
            dx = tpoint->x;
            dy = tpoint->y;
            dz = tpoint->z;

            // find the distance to the target (player)
            dist = DIST(dx, dy, actor->spr.pos.X, actor->spr.pos.Y);

            // (velocity * difference between the target and the object) /
            // distance
            actor->spr.zvel = -((actor->spr.xvel * (actor->spr.pos.Z - dz)) / dist);
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
            actor->spr.xvel = (actor->user.track_vel) >> 8;
        }
        else if (actor->user.Flags & (SPR_SLOW_DOWN))
        {
            if ((actor->user.track_vel -= (locktics << actor->user.vel_rate)) <= actor->user.vel_tgt)
            {
                actor->user.track_vel = actor->user.vel_tgt;
                actor->user.Flags &= ~(SOBJ_SLOW_DOWN);
            }

            actor->spr.xvel = (actor->user.track_vel) >> 8;
        }

        nx = 0;
        ny = 0;

        if (actor->user.Flags & (SPR_CLIMBING))
        {
            if (ActorZOfTop(actor) + (ActorSizeZ(actor) >> 2) < actor->user.pos.Z)
            {
                actor->user.Flags &= ~(SPR_CLIMBING);

                actor->spr.zvel = 0;

                actor->spr.ang = getangle(tpoint->x - actor->spr.pos.X, tpoint->y - actor->spr.pos.Y);

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
            nx = MulScale(actor->spr.xvel, bcos(actor->spr.ang), 14);
            ny = MulScale(actor->spr.xvel, bsin(actor->spr.ang), 14);
        }

        nz = 0;

        if (actor->spr.zvel)
            nz = actor->spr.zvel * locktics;
    }

    actor->user.coll = move_sprite(actor, nx, ny, nz, actor->user.ceiling_dist, actor->user.floor_dist, 0, locktics);


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
