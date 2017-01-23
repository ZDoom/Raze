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

#include "names2.h"
#include "panel.h"
//#include "keys.h"
#include "tags.h"
#include "sector.h"
#include "ai.h"
#include "player.h"
#include "game.h"
#include "net.h"
#include "sprite.h"
#include "track.h"
#include "weapon.h"


void DoTrack(SECTOR_OBJECTp sop, short locktics, int *nx, int *ny);
void DoAutoTurretObject(SECTOR_OBJECTp sop);
void DoTornadoObject(SECTOR_OBJECTp sop);

#define ACTOR_STD_JUMP (-384)
int GlobSpeedSO;

#undef BOUND_4PIX
//#define BOUND_4PIX(x) ( TRUNC4((x) + MOD4(x)) )
#define BOUND_4PIX(x) (x)

// determine if moving down the track will get you closer to the player
short
TrackTowardPlayer(SPRITEp sp, TRACKp t, TRACK_POINTp start_point)
{
    TRACK_POINTp end_point;
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

    end_dist = Distance(end_point->x, end_point->y, sp->x, sp->y);
    start_dist = Distance(start_point->x, start_point->y, sp->x, sp->y);

    if (end_dist < start_dist)
    {
        return TRUE;
    }

    return FALSE;

}

short
TrackStartCloserThanEnd(short SpriteNum, TRACKp t, TRACK_POINTp start_point)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    TRACK_POINTp end_point;
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

    end_dist = Distance(end_point->x, end_point->y, sp->x, sp->y);
    start_dist = Distance(start_point->x, start_point->y, sp->x, sp->y);

    if (start_dist < end_dist)
    {
        return TRUE;
    }

    return FALSE;

}

/*

!AIC - Looks at endpoints to figure direction of the track and the closest
point to the sprite.

*/

short
ActorFindTrack(short SpriteNum, int8_t player_dir, int track_type, short *track_point_num, short *track_dir)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    int dist, near_dist = 999999, zdiff;
    short track_sect=0;

    short i;
    short end_point[2] = {0,0};

    TRACKp t, near_track = NULL;
    TRACK_POINTp tp, near_tp = NULL;

#define TOWARD_PLAYER 1
#define AWAY_FROM_PLAYER -1

    // look at all tracks finding the closest endpoint
    for (t = &Track[0]; t < &Track[MAX_TRACKS]; t++)
    {
        tp = t->TrackPoint;

        // Skip if high tag is not ONE of the track type we are looking for
        if (!TEST(t->ttflags, track_type))
            continue;

        // Skip if already someone on this track
        if (TEST(t->flags, TF_TRACK_OCCUPIED))
        {
            //DSPRINTF(ds,"occupied!");
            MONO_PRINT(ds);
            continue;
        }

        switch (track_type)
        {
        case BIT(TT_DUCK_N_SHOOT):
        {
            if (!u->ActorActionSet->Duck)
                return -1;

            end_point[1] = 0;
            break;
        }

        // for ladders only look at first track point
        case BIT(TT_LADDER):
        {
            if (!u->ActorActionSet->Climb)
                return -1;

            end_point[1] = 0;
            break;
        }

        case BIT(TT_JUMP_UP):
        case BIT(TT_JUMP_DOWN):
        {
            if (!u->ActorActionSet->Jump)
                return -1;

            end_point[1] = 0;
            break;
        }

        case BIT(TT_TRAVERSE):
        {
            if (!u->ActorActionSet->Crawl || !u->ActorActionSet->Jump)
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

            dist = Distance(tp->x, tp->y, sp->x, sp->y);

            if (dist < 15000 && dist < near_dist)
            {
                // make sure track start is on approximate z level - skip if
                // not
                if (labs(sp->z - tp->z) > zdiff)
                {
                    continue;
                }

                // determine if the track leads in the direction we want it
                // to
                if (player_dir == TOWARD_PLAYER)
                {
                    if (!TrackTowardPlayer(u->tgt_sp, t, tp))
                    {
                        continue;
                    }
                }
                else if (player_dir == AWAY_FROM_PLAYER)
                {
                    if (TrackTowardPlayer(u->tgt_sp, t, tp))
                    {
                        continue;
                    }
                }

                // make sure the start distance is closer than the end
                // distance
                if (!TrackStartCloserThanEnd(SpriteNum, t, tp))
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

    if (near_dist < 15000)
    {
        // get the sector number of the point
        COVERupdatesector(near_tp->x, near_tp->y, &track_sect);

        // if can see the point, return the track number
        if (FAFcansee(sp->x, sp->y, sp->z - Z(16), sp->sectnum, near_tp->x, near_tp->y, sector[track_sect].floorz - Z(32), track_sect))
        {
            //DSPRINTF(ds,"Found track point in sector %d\n",track_sect);
            MONO_PRINT(ds);
            return near_track - &Track[0];
        }

        return -1;
    }
    else
    {
        return -1;
    }
}


void
NextTrackPoint(SECTOR_OBJECTp sop)
{
    sop->point += sop->dir;

    if (sop->point > Track[sop->track].NumPoints - 1)
        sop->point = 0;

    if (sop->point < 0)
        sop->point = Track[sop->track].NumPoints - 1;
}


void
NextActorTrackPoint(short SpriteNum)
{
    USERp u = User[SpriteNum];

    u->point += u->track_dir;

    if (u->point > Track[u->track].NumPoints - 1)
        u->point = 0;

    if (u->point < 0)
        u->point = Track[u->track].NumPoints - 1;
}

void
TrackAddPoint(TRACKp t, TRACK_POINTp tp, short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    TRACK_POINTp tpoint = (tp + t->NumPoints);

    //    //DSPRINTF(ds,"3 ndx = %d, numpoints = %d", t - Track, t->NumPoints);
    //    MONO_PRINT(ds);

    tpoint->x = sp->x;
    tpoint->y = sp->y;
    tpoint->z = sp->z;
    tpoint->ang = sp->ang;
    tpoint->tag_low = sp->lotag;
    tpoint->tag_high = sp->hitag;

    t->NumPoints++;

    KillSprite(SpriteNum);
}

int
TrackClonePoint(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], np;
    short New;

    New = COVERinsertsprite(sp->sectnum, sp->statnum);

    ASSERT(New != -1);

    np = &sprite[New];

    np->cstat = np->extra = 0;
    np->x = sp->x;
    np->y = sp->y;
    np->z = sp->z;
    np->ang = sp->ang;
    np->lotag = sp->lotag;
    np->hitag = sp->hitag;

    return New;
}

void QuickJumpSetup(short stat, short lotag, short type)
{
    short SpriteNum = 0, NextSprite, ndx;
    TRACK_POINTp tp;
    TRACKp t;
    SPRITEp nsp;
    short start_sprite, end_sprite;

    // make short quick jump tracks
    TRAVERSE_SPRITE_STAT(headspritestat[stat], SpriteNum, NextSprite)
    {

        // find an open track
        for (ndx = 0; ndx < MAX_TRACKS; ndx++)
        {
            if (Track[ndx].NumPoints == 0)
                break;
        }

        ASSERT(ndx < MAX_TRACKS);

        ////DSPRINTF(ds,"1 ndx = %d, numpoints = %d\n", ndx, Track[ndx].NumPoints);
        //MONO_PRINT(ds);

        FreeMem(Track[ndx].TrackPoint);
        Track[ndx].TrackPoint = CallocMem((4 * sizeof(TRACK_POINT)), 1);

        tp = Track[ndx].TrackPoint;
        t = &Track[ndx];

        // set track type
        SET(t->ttflags, BIT(type));
        t->flags = 0;

        // clone point
        end_sprite = TrackClonePoint(SpriteNum);
        start_sprite = TrackClonePoint(SpriteNum);

        // add start point
        nsp = &sprite[start_sprite];
        nsp->lotag = TRACK_START;
        nsp->hitag = 0;
        TrackAddPoint(t, tp, start_sprite);

        ////DSPRINTF(ds,"2 ndx = %d, numpoints = %d\n", ndx, Track[ndx].NumPoints);
        //MONO_PRINT(ds);

        // add jump point
        nsp = &sprite[SpriteNum];
        nsp->x += 64 * (int) sintable[NORM_ANGLE(nsp->ang + 512)] >> 14;
        nsp->y += 64 * (int) sintable[nsp->ang] >> 14;
        nsp->lotag = lotag;
        TrackAddPoint(t, tp, SpriteNum);

        // add end point
        nsp = &sprite[end_sprite];
        nsp->x += 2048 * (int) sintable[NORM_ANGLE(nsp->ang + 512)] >> 14;
        nsp->y += 2048 * (int) sintable[nsp->ang] >> 14;
        nsp->lotag = TRACK_END;
        nsp->hitag = 0;
        TrackAddPoint(t, tp, end_sprite);
    }
}


void QuickScanSetup(short stat, short lotag, short type)
{
    short SpriteNum = 0, NextSprite, ndx;
    TRACK_POINTp tp;
    TRACKp t;
    SPRITEp nsp;
    short start_sprite, end_sprite;

    // make short quick jump tracks
    TRAVERSE_SPRITE_STAT(headspritestat[stat], SpriteNum, NextSprite)
    {

        // find an open track
        for (ndx = 0; ndx < MAX_TRACKS; ndx++)
        {
            if (Track[ndx].NumPoints == 0)
                break;
        }

        ASSERT(ndx < MAX_TRACKS);

        // save space for 3 points
        FreeMem(Track[ndx].TrackPoint);
        Track[ndx].TrackPoint = CallocMem((4 * sizeof(TRACK_POINT)), 1);

        ASSERT(Track[ndx].TrackPoint != NULL);

        tp = Track[ndx].TrackPoint;
        t = &Track[ndx];

        // set track type
        SET(t->ttflags, BIT(type));
        t->flags = 0;

        // clone point
        end_sprite = TrackClonePoint(SpriteNum);
        start_sprite = TrackClonePoint(SpriteNum);

        // add start point
        nsp = &sprite[start_sprite];
        nsp->lotag = TRACK_START;
        nsp->hitag = 0;
        nsp->x += 64 * (int) sintable[NORM_ANGLE(nsp->ang + 1024 + 512)] >> 14;
        nsp->y += 64 * (int) sintable[NORM_ANGLE(nsp->ang + 1024)] >> 14;
        TrackAddPoint(t, tp, start_sprite);

        // add jump point
        nsp = &sprite[SpriteNum];
        nsp->lotag = lotag;
        TrackAddPoint(t, tp, SpriteNum);

        // add end point
        nsp = &sprite[end_sprite];
        nsp->x += 64 * (int) sintable[NORM_ANGLE(nsp->ang + 512)] >> 14;
        nsp->y += 64 * (int) sintable[nsp->ang] >> 14;
        nsp->lotag = TRACK_END;
        nsp->hitag = 0;
        TrackAddPoint(t, tp, end_sprite);
    }
}

void QuickExitSetup(short stat, short type)
{
    short SpriteNum = 0, NextSprite, ndx;
    TRACK_POINTp tp;
    TRACKp t;
    SPRITEp nsp;
    short start_sprite, end_sprite;

    TRAVERSE_SPRITE_STAT(headspritestat[stat], SpriteNum, NextSprite)
    {

        // find an open track
        for (ndx = 0; ndx < MAX_TRACKS; ndx++)
        {
            if (Track[ndx].NumPoints == 0)
                break;
        }

        ASSERT(ndx < MAX_TRACKS);

        // save space for 3 points
        FreeMem(Track[ndx].TrackPoint);
        Track[ndx].TrackPoint = CallocMem((4 * sizeof(TRACK_POINT)), 1);

        ASSERT(Track[ndx].TrackPoint != NULL);

        tp = Track[ndx].TrackPoint;
        t = &Track[ndx];

        // set track type
        SET(t->ttflags, BIT(type));
        t->flags = 0;

        // clone point
        end_sprite = TrackClonePoint(SpriteNum);
        start_sprite = TrackClonePoint(SpriteNum);

        // add start point
        nsp = &sprite[start_sprite];
        nsp->lotag = TRACK_START;
        nsp->hitag = 0;
        TrackAddPoint(t, tp, start_sprite);

        KillSprite(SpriteNum);

        // add end point
        nsp = &sprite[end_sprite];
        nsp->x += 1024 * (int) sintable[NORM_ANGLE(nsp->ang + 512)] >> 14;
        nsp->y += 1024 * (int) sintable[nsp->ang] >> 14;
        nsp->lotag = TRACK_END;
        nsp->hitag = 0;
        TrackAddPoint(t, tp, end_sprite);
    }
}

void QuickLadderSetup(short stat, short lotag, short type)
{
    short SpriteNum = 0, NextSprite, ndx;
    TRACK_POINTp tp;
    TRACKp t;
    SPRITEp nsp;
    short start_sprite, end_sprite;

    TRAVERSE_SPRITE_STAT(headspritestat[stat], SpriteNum, NextSprite)
    {

        // find an open track
        for (ndx = 0; ndx < MAX_TRACKS; ndx++)
        {
            if (Track[ndx].NumPoints == 0)
                break;
        }

        ASSERT(ndx < MAX_TRACKS);

        // save space for 3 points
        FreeMem(Track[ndx].TrackPoint);
        Track[ndx].TrackPoint = CallocMem((4 * sizeof(TRACK_POINT)), 1);

        ASSERT(Track[ndx].TrackPoint != NULL);

        tp = Track[ndx].TrackPoint;
        t = &Track[ndx];

        // set track type
        SET(t->ttflags, BIT(type));
        t->flags = 0;

        // clone point
        end_sprite = TrackClonePoint(SpriteNum);
        start_sprite = TrackClonePoint(SpriteNum);

        // add start point
        nsp = &sprite[start_sprite];
        nsp->lotag = TRACK_START;
        nsp->hitag = 0;
        nsp->x += MOVEx(256,nsp->ang + 1024);
        nsp->y += MOVEy(256,nsp->ang + 1024);
        TrackAddPoint(t, tp, start_sprite);

        // add climb point
        nsp = &sprite[SpriteNum];
        nsp->lotag = lotag;
        TrackAddPoint(t, tp, SpriteNum);

        // add end point
        nsp = &sprite[end_sprite];
        nsp->x += MOVEx(512,nsp->ang);
        nsp->y += MOVEy(512,nsp->ang);
        nsp->lotag = TRACK_END;
        nsp->hitag = 0;
        TrackAddPoint(t, tp, end_sprite);
    }
}


void
TrackSetup(void)
{
    short SpriteNum = 0, NextSprite, ndx;
    TRACK_POINTp tp;
    TRACKp t;
    SPRITEp nsp;
    short new_sprite1, new_sprite2;
    TRACK_POINTp New;
    int size;

    // put points on track
    for (ndx = 0; ndx < MAX_TRACKS; ndx++)
    {
        if (headspritestat[STAT_TRACK + ndx] == -1)
        {
            // for some reason I need at least one record allocated
            // can't remember why at this point
            Track[ndx].TrackPoint = CallocMem(sizeof(TRACK_POINT) * 1, 1);
            continue;
        }

        ASSERT(Track[ndx].TrackPoint == NULL);

        // make the track array rather large.  I'll resize it to correct size
        // later.
        Track[ndx].TrackPoint = CallocMem(sizeof(TRACK_POINT) * 500, 1);

        ASSERT(Track[ndx].TrackPoint != NULL);

        tp = Track[ndx].TrackPoint;
        t = &Track[ndx];

        // find the first point and save it
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_TRACK + ndx], SpriteNum, NextSprite)
        {
            if (LOW_TAG_SPRITE(SpriteNum) == TRACK_START)
            {
                ASSERT(t->NumPoints == 0);

                TrackAddPoint(t, tp, SpriteNum);
                break;
            }
        }

        // didn't find the start point of the track
        if (t->NumPoints == 0)
        {
            int i, nexti;
            SPRITEp sp = &sprite[headspritestat[STAT_TRACK+ndx]];
            buildprintf("WARNING: Did not find first point of Track Number %d, x %d, y %d", ndx, sp->x, sp->y);
            for (i=headspritestat[STAT_TRACK+ndx]; i>=0; i=nexti)
            {
                // neuter the track's sprite list
                nexti = nextspritestat[i];
                deletesprite(i);
            }
            continue;
        }

        // set up flags for track types
        if (tp->tag_low == TRACK_START && tp->tag_high)
            SET(t->ttflags, BIT(tp->tag_high));

        // while there are still sprites on this status list
        while (headspritestat[STAT_TRACK + ndx] != -1)
        {
            short next_sprite = -1;
            int dist, low_dist = 999999;

            // find the closest point to the last point
            TRAVERSE_SPRITE_STAT(headspritestat[STAT_TRACK + ndx], SpriteNum, NextSprite)
            {
                dist = Distance((tp + t->NumPoints - 1)->x, (tp + t->NumPoints - 1)->y, sprite[SpriteNum].x, sprite[SpriteNum].y);

                if (dist < low_dist)
                {
                    next_sprite = SpriteNum;
                    low_dist = dist;
                }

            }

            // save the closest one off and kill it
            if (next_sprite != -1)
            {
                ASSERT(low_dist < 20000);
                TrackAddPoint(t, tp, next_sprite);
            }

        }

        size = (Track[ndx].NumPoints + 1) * sizeof(TRACK_POINT);
        New = CallocMem(size, 1);
        memcpy(New, Track[ndx].TrackPoint, size);
        FreeMem(Track[ndx].TrackPoint);
        Track[ndx].TrackPoint = New;

        ASSERT(Track[ndx].TrackPoint != NULL);
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

SPRITEp
FindBoundSprite(short tag)
{
    short sn, next_sn;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ST1], sn, next_sn)
    {
        if (sprite[sn].hitag == tag)
        {
            return &sprite[sn];
        }
    }

    return NULL;
}


void
SectorObjectSetupBounds(SECTOR_OBJECTp sop)
{
    int xlow, ylow, xhigh, yhigh;
    short sp_num, next_sp_num, sn, startwall, endwall;
    int i, k, j;
    SPRITEp BoundSprite;
    SWBOOL FoundOutsideLoop = FALSE, FoundSector = FALSE;
    SWBOOL SectorInBounds;
    SECTORp *sectp;
    PLAYERp pp;
    short pnum;
    USERp u = User[sop->sp_child - sprite];

    static unsigned char StatList[] =
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

    BoundSprite = FindBoundSprite(500 + ((sop - SectorObject) * 5));

    //DSPRINTF(ds,"tagnum %d, so num %d",500 + ((sop - SectorObject) * 5), sop - SectorObject);
    MONO_PRINT(ds);

    ASSERT(BoundSprite != NULL);
    xlow = BoundSprite->x;
    ylow = BoundSprite->y;

    KillSprite(BoundSprite - sprite);

    BoundSprite = FindBoundSprite(501 + ((sop - SectorObject) * 5));
    ASSERT(BoundSprite != NULL);
    xhigh = BoundSprite->x;
    yhigh = BoundSprite->y;

    KillSprite(BoundSprite - sprite);

    // set radius for explosion checking - based on bounding box
    u->Radius = DIV4((xhigh - xlow) + (yhigh - ylow));
    u->Radius -= DIV4(u->Radius); // trying to get it a good size

    // search for center sprite if it exists

    BoundSprite = FindBoundSprite(SECT_SO_CENTER);
    if (BoundSprite)
    {
        sop->xmid = BoundSprite->x;
        sop->ymid = BoundSprite->y;
        sop->zmid = BoundSprite->z;
        KillSprite(BoundSprite - sprite);
    }

#if 0
    // look for players on sector object
    TRAVERSE_CONNECT(pnum)
    {
        pp = &Player[pnum];

        if (pp->posx > xlow && pp->posx < xhigh && pp->posy > ylow && pp->posy < yhigh)
        {
            pp->RevolveAng = pp->pang;
            pp->RevolveX = pp->posx;
            pp->RevolveY = pp->posy;
            pp->RevolveDeltaAng = 0;
            SET(pp->Flags, PF_PLAYER_RIDING);

            pp->sop_riding = sop;
        }
    }
#endif


    // look through all sectors for whole sectors that are IN bounds
    for (k = 0; k < numsectors; k++)
    {
        startwall = sector[k].wallptr;
        endwall = startwall + sector[k].wallnum - 1;

        SectorInBounds = TRUE;

        for (j = startwall; j <= endwall; j++)
        {
            // all walls have to be in bounds to be in sector object
            if (!(wall[j].x > xlow && wall[j].x < xhigh && wall[j].y > ylow && wall[j].y < yhigh))
            {
                SectorInBounds = FALSE;
                break;
            }
        }

        if (SectorInBounds)
        {
            sop->sector[sop->num_sectors] = k;
            sop->sectp[sop->num_sectors] = &sector[k];

            // all sectors in sector object have this flag set - for colision
            // detection and recognition
            SET(sector[k].extra, SECTFX_SECTOR_OBJECT);

            sop->zorig_floor[sop->num_sectors] = sector[k].floorz;
            sop->zorig_ceiling[sop->num_sectors] = sector[k].ceilingz;

            if (TEST(sector[k].extra, SECTFX_SINK))
                sop->zorig_floor[sop->num_sectors] += Z(SectUser[k]->depth);

            // lowest and highest floorz's
            if (sector[k].floorz > sop->floor_loz)
                sop->floor_loz = sector[k].floorz;

            if (sector[k].floorz < sop->floor_hiz)
                sop->floor_hiz = sector[k].floorz;

            sop->num_sectors++;
        }

        ASSERT(sop->num_sectors < SIZ(SectorObject[0].sector));
    }

    //
    // Make sure every sector object has an outer loop tagged - important
    //

    FoundOutsideLoop = FALSE;

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        startwall = (*sectp)->wallptr;
        endwall = startwall + (*sectp)->wallnum - 1;

        // move all walls in sectors
        for (k = startwall; k <= endwall; k++)
        {
            // for morph point - tornado style
            if (wall[k].lotag == TAG_WALL_ALIGN_SLOPE_TO_POINT)
                sop->morph_wall_point = k;

            if (wall[k].extra && TEST(wall[k].extra, WALLFX_LOOP_OUTER))
                FoundOutsideLoop = TRUE;

            // each wall has this set - for collision detection
            SET(wall[k].extra, WALLFX_SECTOR_OBJECT|WALLFX_DONT_STICK);
            if (wall[k].nextwall >= 0)
                SET(wall[wall[k].nextwall].extra, WALLFX_SECTOR_OBJECT|WALLFX_DONT_STICK);
        }
    }

    if (!FoundOutsideLoop)
    {
        TerminateGame();
        printf("Forgot to tag outer loop for Sector Object #%d", (int)(sop - SectorObject));
        exit(1);
    }

    for (i = 0; i < (int)SIZ(StatList); i++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatList[i]], sp_num, next_sp_num)
        {
            SPRITEp sp = &sprite[sp_num];
            USERp u;

            ASSERT(sp_num != -1);

            if (sp->x > xlow && sp->x < xhigh && sp->y > ylow && sp->y < yhigh)
            {
                // some delete sprites ride others don't
                if (sp->statnum == STAT_DELETE_SPRITE)
                {
                    if (!TEST_BOOL2(sp))
                        continue;
                }

                if (User[sp_num] == NULL)
                    u = SpawnUser(sp_num, 0, NULL);
                else
                    u = User[sp_num];

                u->RotNum = 0;

                u->ox = sp->x;
                u->oy = sp->y;
                u->oz = sp->z;

                switch (sp->statnum)
                {
                case STAT_WALL_MOVE:
                    ////DSPRINTF(ds,"Damage Wall attached ");
                    //MONO_PRINT(ds);
                    break;
                case STAT_DEFAULT:
                    switch (sp->hitag)
                    {
                    case SO_CLIP_BOX:
                    {
                        short ang2;
                        sop->clipdist = 0;
                        sop->clipbox_dist[sop->clipbox_num] = sp->lotag;
                        sop->clipbox_xoff[sop->clipbox_num] = sop->xmid - sp->x;
                        sop->clipbox_yoff[sop->clipbox_num] = sop->ymid - sp->y;

                        sop->clipbox_vdist[sop->clipbox_num] = ksqrt(SQ(sop->xmid - sp->x) + SQ(sop->ymid - sp->y));

                        ang2 = getangle(sp->x - sop->xmid, sp->y - sop->ymid);
                        sop->clipbox_ang[sop->clipbox_num] = GetDeltaAngle(sop->ang, ang2);

                        sop->clipbox_num++;
                        KillSprite(sp_num);


                        goto cont;
                    }
                    case SO_SHOOT_POINT:
                        sp->owner = -1;
                        change_sprite_stat(sp_num, STAT_SO_SHOOT_POINT);
                        RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
                        break;
                    default:
                        break;
                    }
                    break;
                }


                u->sx = sop->xmid - sp->x;
                u->sy = sop->ymid - sp->y;
                u->sz = sector[sop->mid_sector].floorz - sp->z;

                SET(u->Flags, SPR_SO_ATTACHED);

                u->sang = sp->ang;
                u->spal = sp->pal;

                // search SO's sectors to make sure that it is not on a
                // sector

                // place all sprites on list
                for (sn = 0; sn < (int)SIZ(sop->sp_num); sn++)
                {
                    if (sop->sp_num[sn] == -1)
                        break;
                }

                ASSERT(sn < SIZ(sop->sp_num) - 1);

                sop->sp_num[sn] = sp_num;


                if (!TEST(sop->flags, SOBJ_SPRITE_OBJ))
                {
                    // determine if sprite is on a SO sector - set flag if
                    // true
                    for (j = 0; j < sop->num_sectors; j++)
                    {
                        if (sop->sector[j] == sp->sectnum)
                        {
                            SET(u->Flags, SPR_ON_SO_SECTOR);
                            u->sz = sector[sp->sectnum].floorz - sp->z;
                            break;
                        }
                    }
                }
            }

cont:
            continue;
        }
    }

    // for SPRITE OBJECT sprites, set the u->sz value to the difference
    // between the zmid and the sp->z
    if (TEST(sop->flags, SOBJ_SPRITE_OBJ))
    {
        SPRITEp sp;
        USERp u;
        int zmid = -9999999;

        // choose the lowest sprite for the zmid
        for (i = 0; sop->sp_num[i] != -1; i++)
        {
            sp = &sprite[sop->sp_num[i]];
            u = User[sop->sp_num[i]];

            if (sp->z > zmid)
                zmid = sp->z;
        }

        ASSERT(zmid != -9999999);

        sop->zmid = zmid;

        for (i = 0; sop->sp_num[i] != -1; i++)
        {
            sp = &sprite[sop->sp_num[i]];
            u = User[sop->sp_num[i]];

            u->sz = sop->zmid - sp->z;
        }

    }
}


void
SetupSectorObject(short sectnum, short tag)
{
    SPRITEp sp;
    SECTOR_OBJECTp sop;
    short object_num, ndx = 0, startwall, endwall, SpriteNum, NextSprite;
    int trash;
    short j, k;
    short New;
    USERp u;

    tag -= (TAG_OBJECT_CENTER - 1);
    // sector[sectnum].lotag = tag;

    object_num = tag / 5;
    sop = &SectorObject[object_num];

    // initialize stuff first time through
    if (sop->num_sectors == -1)
    {
        void DoTornadoObject(SECTOR_OBJECTp sop);
        void MorphTornado(SECTOR_OBJECTp sop);
        void MorphFloor(SECTOR_OBJECTp sop);
        void ScaleSectorObject(SECTOR_OBJECTp sop);
        void DoAutoTurretObject(SECTOR_OBJECTp sop);

        memset(sop->sectp, 0, sizeof(sop->sectp));
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
        sop->op_main_sector = -1;
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

        sop->PreMoveAnimator = NULL;
        sop->PostMoveAnimator = NULL;
        sop->Animator = NULL;
    }

    switch (tag % 5)
    {
    case TAG_OBJECT_CENTER - 500:

        sop->mid_sector = sectnum;
        SectorMidPoint(sectnum, &sop->xmid, &sop->ymid, &sop->zmid);
        //sop->zmid = sector[sectnum].floorz;
        //sop->zmid = DIV2(sector[sectnum].floorz + sector[sectnum].ceilingz);

        sop->dir = 1;
        sop->track = HIGH_TAG(sectnum);

        // spawn a sprite to make it easier to integrate with sprite routines
        New = SpawnSprite(STAT_SO_SP_CHILD, 0, NULL, sectnum,
                          sop->xmid, sop->ymid, sop->zmid, 0, 0);
        sop->sp_child = &sprite[New];
        u = User[New];
        u->sop_parent = sop;
        SET(u->Flags2, SPR2_SPRITE_FAKE_BLOCK); // for damage test

        // check for any ST1 sprites laying on the center sector
        TRAVERSE_SPRITE_SECT(headspritesect[sectnum], SpriteNum, NextSprite)
        {
            sp = &sprite[SpriteNum];

            if (sp->statnum == STAT_ST1)
            {
                switch (sp->hitag)
                {
                case SO_SCALE_XY_MULT:
                    if (SP_TAG5(sp))
                        sop->scale_x_mult = SP_TAG5(sp);
                    if (SP_TAG6(sp))
                        sop->scale_y_mult = SP_TAG6(sp);
                    KillSprite(SpriteNum);
                    break;

                case SO_SCALE_POINT_INFO:

                    memset(sop->scale_point_dist,0,sizeof(sop->scale_point_dist));
                    sop->scale_point_base_speed = SP_TAG2(sp);
                    for (j = 0; j < (int)SIZ(sop->scale_point_speed); j++)
                    {
                        sop->scale_point_speed[j] = SP_TAG2(sp);
                    }

                    if (SP_TAG4(sp))
                        sop->scale_point_rand_freq = (uint8_t)SP_TAG4(sp);
                    else
                        sop->scale_point_rand_freq = 64;

                    sop->scale_point_dist_min = -SP_TAG5(sp);
                    sop->scale_point_dist_max = SP_TAG6(sp);
                    KillSprite(SpriteNum);
                    break;

                case SO_SCALE_INFO:
                    SET(sop->flags, SOBJ_DYNAMIC);
                    sop->scale_speed = SP_TAG2(sp);
                    sop->scale_dist_min = -SP_TAG5(sp);
                    sop->scale_dist_max = SP_TAG6(sp);

                    sop->scale_type = SP_TAG4(sp);
                    sop->scale_active_type = SP_TAG7(sp);

                    if (SP_TAG8(sp))
                        sop->scale_rand_freq = (uint8_t)SP_TAG8(sp);
                    else
                        sop->scale_rand_freq = 64>>3;

                    if (SP_TAG3(sp) == 0)
                        sop->scale_dist = sop->scale_dist_min;
                    else if (SP_TAG3(sp) == 1)
                        sop->scale_dist = sop->scale_dist_max;

                    KillSprite(SpriteNum);
                    break;

                case SPAWN_SPOT:
                    if (sp->clipdist == 3)
                    {
                        USERp u;
                        change_sprite_stat(SpriteNum, STAT_NO_STATE);
                        u = SpawnUser(SpriteNum, 0, NULL);
                        u->ActorActionFunc = NULL;
                    }
                    break;

                case SO_AUTO_TURRET:
                    sop->Animator = DoAutoTurretObject;
                    KillSprite(SpriteNum);
                    break;

                case SO_TORNADO:
                    if (SW_SHAREWARE) break;
                    sop->vel = 120;
                    SET(sop->flags, SOBJ_DYNAMIC);
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
                    KillSprite(SpriteNum);
                    break;
                case SO_FLOOR_MORPH:
                    if (SW_SHAREWARE) break;
                    SET(sop->flags, SOBJ_DYNAMIC);
                    sop->scale_type = SO_SCALE_NONE;
                    sop->morph_speed = 120;
                    sop->morph_z_speed = 7;
                    sop->PostMoveAnimator = MorphFloor;
                    sop->morph_dist_max = 4000;
                    sop->morph_rand_freq = 8;
                    KillSprite(SpriteNum);
                    break;

                case SO_AMOEBA:
                    SET(sop->flags, SOBJ_DYNAMIC);
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
                    KillSprite(SpriteNum);
                    break;
                case SO_MAX_DAMAGE:
                    u->MaxHealth = SP_TAG2(sp);
                    if (SP_TAG5(sp) != 0)
                        sop->max_damage = SP_TAG5(sp);
                    else
                        sop->max_damage = u->MaxHealth;

                    switch (sp->clipdist)
                    {
                    case 0:
                        break;
                    case 1:
                        SET(sop->flags, SOBJ_DIE_HARD);
                        break;
                    }
                    KillSprite(SpriteNum);
                    break;

                case SO_DRIVABLE_ATTRIB:

                    sop->drive_angspeed = SP_TAG2(sp);
                    sop->drive_angspeed <<= 5;
                    sop->drive_angslide = SP_TAG3(sp);
                    if (sop->drive_angslide <= 0 || sop->drive_angslide == 32)
                        sop->drive_angslide = 1;

                    sop->drive_speed = SP_TAG6(sp);
                    sop->drive_speed <<= 5;
                    sop->drive_slide = SP_TAG7(sp);
                    if (sop->drive_slide <= 0)
                        sop->drive_slide = 1;

                    if (TEST_BOOL1(sp))
                        SET(sop->flags, SOBJ_NO_QUAKE);

                    if (TEST_BOOL3(sp))
                        SET(sop->flags, SOBJ_REMOTE_ONLY);

                    if (TEST_BOOL4(sp))
                    {
                        sop->crush_z = sp->z;
                        SET(sop->flags, SOBJ_RECT_CLIP);
                    }

                    //KillSprite(SpriteNum);
                    break;

                case SO_RAM_DAMAGE:
                    sop->ram_damage = sp->lotag;
                    KillSprite(SpriteNum);
                    break;
                case SECT_SO_CLIP_DIST:
                    sop->clipdist = sp->lotag;
                    KillSprite(SpriteNum);
                    break;
                case SECT_SO_SPRITE_OBJ:
                    SET(sop->flags, SOBJ_SPRITE_OBJ);
                    KillSprite(SpriteNum);
                    break;
                case SECT_SO_DONT_ROTATE:
                    SET(sop->flags, SOBJ_DONT_ROTATE);
                    KillSprite(SpriteNum);
                    break;
                case SO_LIMIT_TURN:
                    sop->limit_ang_center = sp->ang;
                    sop->limit_ang_delta = sp->lotag;
                    KillSprite(SpriteNum);
                    break;
                case SO_MATCH_EVENT:
                    sop->match_event = sp->lotag;
                    sop->match_event_sprite = SpriteNum;
                    break;
                case SO_SET_SPEED:
                    sop->vel = sp->lotag * 256;
                    sop->vel_tgt = sop->vel;
                    KillSprite(SpriteNum);
                    break;
                case SO_SPIN:
                    if (sop->spin_speed)
                        break;
                    sop->spin_speed = sp->lotag;
                    sop->last_ang = sop->ang;
                    KillSprite(SpriteNum);
                    break;
                case SO_ANGLE:
                    sop->ang = sop->ang_moving = sp->ang;
                    sop->last_ang = sop->ang_orig = sop->ang;
                    sop->spin_ang = 0;
                    KillSprite(SpriteNum);
                    break;
                case SO_SPIN_REVERSE:

                    sop->spin_speed = sp->lotag;
                    sop->last_ang = sop->ang;

                    if (sop->spin_speed >= 0)
                        sop->spin_speed = -sop->spin_speed;

                    KillSprite(SpriteNum);
                    break;
                case SO_BOB_START:
                    sop->bob_amt = Z(sp->lotag);
                    sop->bob_sine_ndx = 0;
                    sop->bob_speed = 4;
                    KillSprite(SpriteNum);
                    break;
                case SO_TURN_SPEED:
                    sop->turn_speed = sp->lotag;
                    KillSprite(SpriteNum);
                    break;
                case SO_SYNC1:
                    SET(sop->flags, SOBJ_SYNC1);
                    KillSprite(SpriteNum);
                    break;
                case SO_SYNC2:
                    SET(sop->flags, SOBJ_SYNC2);
                    KillSprite(SpriteNum);
                    break;
                case SO_KILLABLE:
                    SET(sop->flags, SOBJ_KILLABLE);
                    KillSprite(SpriteNum);
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
            case SO_TANK:
                sop->vel = 0;
                SET(sop->flags, SOBJ_OPERATIONAL);
                break;
            case SO_SPEED_BOAT:
                sop->vel = 0;
                sop->bob_amt = Z(2);
                sop->bob_speed = 4;
                SET(sop->flags, SOBJ_OPERATIONAL);
                break;
            default:
                SET(sop->flags, SOBJ_OPERATIONAL);
                break;
            }
        }

        sector[sectnum].lotag = 0;
        sector[sectnum].hitag = 0;

        if (sop->max_damage <= 0)
            VehicleSetSmoke(sop, SpawnVehicleSmoke);

        // find radius
        //u->Radius = sop->

        break;
    }

}

void
PostSetupSectorObject(void)
{
    SECTOR_OBJECTp sop;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (sop->xmid == MAXLONG)
            continue;
        FindMainSector(sop);
    }
}


SECTOR_OBJECTp
PlayerOnObject(short sectnum_match)
{
    short i, j;
    SECTOR_OBJECTp sop;

    // place each sector object on the track
    //for (i = 0; (SectorObject[i].xmid != MAXLONG) && (i < MAX_SECTOR_OBJECTS); i++)
    for (i = 0; (i < MAX_SECTOR_OBJECTS); i++)
    {
        sop = &SectorObject[i];

        if (sop->track < SO_OPERATE_TRACK_START)
            continue;

        for (j = 0; j < sop->num_sectors; j++)
        {
            if (sop->sector[j] == sectnum_match && TEST(sector[sectnum_match].extra, SECTFX_OPERATIONAL))
            {
                return sop;
            }
        }
    }

    return NULL;
}


void
PlaceSectorObjectsOnTracks(void)
{
    short i, j, k, startwall, endwall;
    SWBOOL found;

    // place each sector object on the track
    for (i = 0; i < MAX_SECTOR_OBJECTS; i++)
    {
        int low_dist = 999999, dist;
        SECTOR_OBJECTp sop = &SectorObject[i];
        TRACK_POINTp tpoint = NULL;
        short spnum, next_spnum;

        if (sop->xmid == MAXLONG)
            continue;


        // save off the original x and y locations of the walls AND sprites
        sop->num_walls = 0;
        for (j = 0; sop->sector[j] != -1; j++)
        {
            startwall = sector[sop->sector[j]].wallptr;
            endwall = startwall + sector[sop->sector[j]].wallnum - 1;

            // move all walls in sectors
            for (k = startwall; k <= endwall; k++)
            {
                sop->xorig[sop->num_walls] = sop->xmid - wall[k].x;
                sop->yorig[sop->num_walls] = sop->ymid - wall[k].y;
                sop->num_walls++;
            }
        }

        ASSERT(sop->num_walls < SIZ(sop->xorig));

        if (sop->track <= -1)
            continue;

        if (sop->track >= SO_OPERATE_TRACK_START)
            continue;

        found = FALSE;
        // find the closest point on the track and put SOBJ on it
        for (j = 0; j < Track[sop->track].NumPoints; j++)
        {
            tpoint = Track[sop->track].TrackPoint;

            dist = Distance((tpoint + j)->x, (tpoint + j)->y, sop->xmid, sop->ymid);

            if (dist < low_dist)
            {
                low_dist = dist;
                sop->point = j;
                found = TRUE;
                ////DSPRINTF(ds,"point = %d, dist = %d, x1=%d, y1=%d",j,low_dist,(tpoint +j)->x,(tpoint+j)->y);
                //MONO_PRINT(ds);
            }
        }

        if (!found)
        {
            //DSPRINTF(ds,"track not found");
            MONO_PRINT(ds);
            sop->track = -1;
            continue;
        }

        NextTrackPoint(sop);

        sop->ang = getangle((tpoint + sop->point)->x - sop->xmid, (tpoint + sop->point)->y - sop->ymid);

        sop->ang_moving = sop->ang_tgt = sop->ang;
    }

}


void
PlaceActorsOnTracks(void)
{
    short i, nexti, j, tag, htag, new_ang;
    SPRITEp sp;
    USERp u;
    TRACK_POINTp tpoint = NULL;

    // place each actor on the track
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], i, nexti)
    {
        int low_dist = 999999, dist;

        sp = User[i]->SpriteP;
        u = User[i];

        tag = LOW_TAG_SPRITE(i);
        htag = HIGH_TAG_SPRITE(i);

        if (tag < TAG_ACTOR_TRACK_BEGIN || tag > TAG_ACTOR_TRACK_END)
            continue;

        // setup sprite track defaults
        u->track = tag - TAG_ACTOR_TRACK_BEGIN;

        // if facing left go backward
        if (BETWEEN(sp->ang, 513, 1535))
        {
            u->track_dir = -1;
        }
        else
        {
            u->track_dir = 1;
        }

        u->track_vel = sp->xvel * 256;
        u->vel_tgt = u->track_vel;
        u->vel_rate = 6;

        // find the closest point on the track and put SOBJ on it
        for (j = 0; j < Track[u->track].NumPoints; j++)
        {
            tpoint = Track[u->track].TrackPoint;

            dist = Distance((tpoint + j)->x, (tpoint + j)->y, sp->x, sp->y);

            if (dist < low_dist)
            {
                low_dist = dist;
                u->point = j;
            }
        }

        NextActorTrackPoint(i);

        // check angle in the "forward" direction
        sp->ang = getangle((tpoint + u->point)->x - sp->x, (tpoint + u->point)->y - sp->y);
    }
}


void
MovePlayer(PLAYERp pp, SECTOR_OBJECTp sop, int nx, int ny)
{
    void DoPlayerZrange(PLAYERp pp);

    // make sure your standing on the so
    if (TEST(pp->Flags, PF_JUMPING | PF_FALLING | PF_FLYING))
        return;

    pp->sop_riding = sop;

    // if player has NOT moved and player is NOT riding
    // set up the player for riding
    if (!TEST(pp->Flags, PF_PLAYER_MOVED) && !TEST(pp->Flags, PF_PLAYER_RIDING))
    {
        SET(pp->Flags, PF_PLAYER_RIDING);

        pp->RevolveAng = pp->pang;
        pp->RevolveX = pp->posx;
        pp->RevolveY = pp->posy;

        // set the delta angle to 0 when moving
        pp->RevolveDeltaAng = 0;
    }

    pp->posx += BOUND_4PIX(nx);
    pp->posy += BOUND_4PIX(ny);


    if (TEST(sop->flags, SOBJ_DONT_ROTATE))
    {
        UpdatePlayerSprite(pp);
        return;
    }

    if (TEST(pp->Flags, PF_PLAYER_MOVED))
    {
        // Player is moving

        // save the current information so when Player stops
        // moving then you
        // know where he was last
        pp->RevolveAng = pp->pang;
        pp->RevolveX = pp->posx;
        pp->RevolveY = pp->posy;

        // set the delta angle to 0 when moving
        pp->RevolveDeltaAng = 0;
    }
    else
    {
        // Player is NOT moving

        // Move saved x&y variables
        pp->RevolveX += BOUND_4PIX(nx);
        pp->RevolveY += BOUND_4PIX(ny);

        // Last known angle is now adjusted by the delta angle
        pp->RevolveAng = NORM_ANGLE(pp->pang - pp->RevolveDeltaAng);
    }

    // increment Players delta angle
    pp->RevolveDeltaAng = NORM_ANGLE(pp->RevolveDeltaAng + GlobSpeedSO);

    rotatepoint(*(vec2_t *)&sop->xmid, *(vec2_t *)&pp->RevolveX, pp->RevolveDeltaAng, (vec2_t *)&pp->posx);

    // THIS WAS CAUSING PROLEMS!!!!
    // Sectors are still being manipulated so you can end up in a void (-1) sector
    //COVERupdatesector(pp->posx, pp->posy, &pp->cursectnum);

    // New angle is formed by taking last known angle and
    // adjusting by the delta angle
    pp->pang = NORM_ANGLE(pp->RevolveAng + pp->RevolveDeltaAng);

    UpdatePlayerSprite(pp);
}

void
MovePoints(SECTOR_OBJECTp sop, short delta_ang, int nx, int ny)
{
    int j, k, c;
    vec2_t rxy;
    short startwall, endwall, save_ang, pnum;
    PLAYERp pp;
    SECTORp *sectp;
    SPRITEp sp;
    WALLp wp;
    USERp u;
    short i, nexti, rot_ang;
    SWBOOL PlayerMove = TRUE;

    if (sop->xmid >= (int)MAXSO)
        PlayerMove = FALSE;

    // move along little midpoint
    sop->xmid += BOUND_4PIX(nx);
    sop->ymid += BOUND_4PIX(ny);

    if (sop->xmid >= (int)MAXSO)
        PlayerMove = FALSE;

    // move child sprite along also
    sop->sp_child->x = sop->xmid;
    sop->sp_child->y = sop->ymid;

    //COVERupdatesector(sop->xmid, sop->ymid, &sop->sectnum);

    // setting floorz if need be
    //if (!TEST(sop->flags, SOBJ_SPRITE_OBJ))
    if (TEST(sop->flags, SOBJ_ZMID_FLOOR))
        sop->zmid = sector[sop->mid_sector].floorz;

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if (TEST(sop->flags, SOBJ_SPRITE_OBJ | SOBJ_DONT_ROTATE))
            goto PlayerPart;

        startwall = (*sectp)->wallptr;
        endwall = startwall + (*sectp)->wallnum - 1;

        // move all walls in sectors
        for (wp = &wall[startwall], k = startwall; k <= endwall; wp++, k++)
        {
            if (TEST(wp->extra, WALLFX_LOOP_DONT_SPIN | WALLFX_DONT_MOVE))
                continue;

            if (wp->extra && TEST(wp->extra, WALLFX_LOOP_OUTER))
            {
                dragpoint(k, wp->x += BOUND_4PIX(nx), wp->y += BOUND_4PIX(ny), 0);
            }
            else
            {
                wp->x += BOUND_4PIX(nx);
                wp->y += BOUND_4PIX(ny);
            }

            rot_ang = delta_ang;

            if (TEST(wp->extra, WALLFX_LOOP_REVERSE_SPIN))
                rot_ang = -delta_ang;

            if (TEST(wp->extra, WALLFX_LOOP_SPIN_2X))
                rot_ang = NORM_ANGLE(rot_ang * 2);

            if (TEST(wp->extra, WALLFX_LOOP_SPIN_4X))
                rot_ang = NORM_ANGLE(rot_ang * 4);

            rotatepoint(*(vec2_t *)&sop->xmid, *(vec2_t *)&wp->x, rot_ang, &rxy);

            if (wp->extra && TEST(wp->extra, WALLFX_LOOP_OUTER))
            {
                dragpoint(k, rxy.x, rxy.y, 0);
            }
            else
            {
                wp->x = rxy.x;
                wp->y = rxy.y;
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

            if (TEST(sector[pp->lo_sectp - sector].extra, SECTFX_NO_RIDE))
            {
#if 0
                short nr, nextnr;
                SWBOOL skip = TRUE;
                TRAVERSE_SPRITE_STAT(headspritestat[STAT_NO_RIDE], nr, nextnr)
                {
                    if (sprite[nr].lotag == sop - SectorObject)
                        skip = TRUE;
                    else
                        skip = FALSE;
                }

                if (skip)
#endif
                continue;
            }

            // move the player
            if (pp->lo_sectp - sector == sop->sector[j])
            {
                if (PlayerMove)
                    MovePlayer(pp, sop, nx, ny);
            }
        }
    }

    for (i = 0; sop->sp_num[i] != -1; i++)
    {
        sp = &sprite[sop->sp_num[i]];
        u = User[sop->sp_num[i]];

        // if its a player sprite || NOT attached
        if (!u || u->PlayerP || !TEST(u->Flags, SPR_SO_ATTACHED))
            continue;

        // move the player
        TRAVERSE_CONNECT(pnum)
        {
            pp = Player + pnum;

            if (pp->lo_sp && pp->lo_sp == sp)
            {
                if (PlayerMove)
                    MovePlayer(pp, sop, nx, ny);
            }
        }

        sp->x = sop->xmid - u->sx;
        sp->y = sop->ymid - u->sy;

        // sprites z update
        if (TEST(sop->flags, SOBJ_SPRITE_OBJ))
        {
            // Sprite Objects follow zmid
            sp->z = sop->zmid - u->sz;
        }
        else
        {
            // Sector Objects can either have sprites ON or OFF of the sector
            if (TEST(u->Flags, SPR_ON_SO_SECTOR))
            {
                // move with sector its on
                sp->z = sector[sp->sectnum].floorz - u->sz;
            }
            else
            {
                // move with the mid sector
                sp->z = sector[sop->mid_sector].floorz - u->sz;
            }
        }

        sp->ang = u->sang;

        if (TEST(u->Flags, SPR_ON_SO_SECTOR))
        {
            if (TEST(sop->flags, SOBJ_DONT_ROTATE))
                continue;

            // IS part of a sector - sprite can do things based on the
            // current sector it is in
            if (TEST(wall[sector[sp->sectnum].wallptr].extra, WALLFX_LOOP_DONT_SPIN))
                continue;

            if (TEST(wall[sector[sp->sectnum].wallptr].extra, WALLFX_LOOP_REVERSE_SPIN))
            {
                rotatepoint(*(vec2_t *)&sop->xmid, *(vec2_t *)&sp->x, -delta_ang, (vec2_t *)&sp->x);
                sp->ang = NORM_ANGLE(sp->ang - delta_ang);
            }
            else
            {
                rotatepoint(*(vec2_t *)&sop->xmid, *(vec2_t *)&sp->x, delta_ang, (vec2_t *)&sp->x);
                sp->ang = NORM_ANGLE(sp->ang + delta_ang);
            }

        }
        else
        {
            if (!TEST(sop->flags, SOBJ_DONT_ROTATE))
            {
                // NOT part of a sector - independant of any sector
                rotatepoint(*(vec2_t *)&sop->xmid, *(vec2_t *)&sp->x, delta_ang, (vec2_t *)&sp->x);
                sp->ang = NORM_ANGLE(sp->ang + delta_ang);
            }

            // Does not necessarily move with the sector so must accout for
            // moving across sectors
            if (sop->xmid < (int)MAXSO) // special case for operating SO's
                setspritez(sop->sp_num[i], (vec3_t *)sp);
        }

        if (TEST(sp->extra, SPRX_BLADE))
        {
            DoBladeDamage(sop->sp_num[i]);
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
            //if (sop->xmid < (int)MAXSO)
            COVERupdatesector(pp->posx, pp->posy, &pp->cursectnum);

            // in case you are in a whirlpool
            // move perfectly with the ride in the z direction
            if TEST(pp->Flags, PF_CRAWLING)
            {
                // move up some for really fast moving plats
                //pp->posz -= PLAYER_HEIGHT + Z(12);
                DoPlayerZrange(pp);
                pp->posz = pp->loz - PLAYER_CRAWL_HEIGHT;
                pp->SpriteP->z = pp->loz;
            }
            else
            {
                // move up some for really fast moving plats
                //pp->posz -= Z(24);
                DoPlayerZrange(pp);

                if (!TEST(pp->Flags, PF_JUMPING | PF_FALLING | PF_FLYING))
                {
                    pp->posz = pp->loz - PLAYER_HEIGHT;
                    pp->SpriteP->z = pp->loz;
                }
            }
        }
        else
        {
            // if player was not on any sector object set Riding flag to false
            RESET(pp->Flags, PF_PLAYER_RIDING);
        }
    }
}

void
RefreshPoints(SECTOR_OBJECTp sop, int nx, int ny, SWBOOL dynamic)
{
    short wallcount = 0, j, k, startwall, endwall, delta_ang_from_orig;
    SECTORp *sectp;
    WALLp wp;
    short ang;
    short new_ang;
    int dx,dy,x,y;

    // do scaling
    if (dynamic && sop->PreMoveAnimator)
        (*sop->PreMoveAnimator)(sop);

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if (!TEST(sop->flags, SOBJ_SPRITE_OBJ))
        {
            startwall = (*sectp)->wallptr;
            endwall = startwall + (*sectp)->wallnum - 1;

            // move all walls in sectors back to the original position
            for (wp = &wall[startwall], k = startwall; k <= endwall; wp++, k++)
            {
                if (!(wp->extra && TEST(wp->extra, WALLFX_DONT_MOVE)))
                {
                    dx = x = sop->xmid - sop->xorig[wallcount];
                    dy = y = sop->ymid - sop->yorig[wallcount];

                    if (dynamic && sop->scale_type)
                    {
                        if (!TEST(wp->extra, WALLFX_DONT_SCALE))
                        {
                            ang = NORM_ANGLE(getangle(x - sop->xmid, y - sop->ymid));

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

                                dx = x + ((xmul * sintable[NORM_ANGLE(ang+512)]) >> 14);
                                dy = y + ((ymul * sintable[ang]) >> 14);
                            }
                        }
                    }

                    if (wp->extra && TEST(wp->extra, WALLFX_LOOP_OUTER))
                    {
                        dragpoint(k, dx, dy, 0);
                    }
                    else
                    {
                        wp->x = dx;
                        wp->y = dy;
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

void KillSectorObjectSprites(SECTOR_OBJECTp sop)
{
    SPRITEp sp;
    USERp u;
    int i;

    for (i = 0; sop->sp_num[i] != -1; i++)
    {
        sp = &sprite[sop->sp_num[i]];
        u = User[sop->sp_num[i]];

        // not a part of the so anymore
        RESET(u->Flags, SPR_SO_ATTACHED);

        if (sp->picnum == ST1 && sp->hitag == SPAWN_SPOT)
            continue;

        KillSprite(sop->sp_num[i]);
    }

    // clear the list
    sop->sp_num[0] = -1;
}

void UpdateSectorObjectSprites(SECTOR_OBJECTp sop)
{
    SPRITEp sp;
    USERp u;
    int i;

    for (i = 0; sop->sp_num[i] != -1; i++)
    {
        sp = &sprite[sop->sp_num[i]];
        u = User[sop->sp_num[i]];

        setspritez(sop->sp_num[i], (vec3_t *)sp);
    }
}

SECTOR_OBJECTp
DetectSectorObject(SECTORp sectph)
{
    short j;
    SECTORp *sectp;
    SECTOR_OBJECTp sop;


    // collapse the SO to a single point
    // move all points to nx,ny
    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (sop->xmid == MAXLONG || sop->xmid == MAXSO)
            continue;

        for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
        {
            if (sectph == *sectp)
                return sop;
        }
    }

    return NULL;
}

SECTOR_OBJECTp
DetectSectorObjectByWall(WALLp wph)
{
    short j, k, startwall, endwall;
    SECTORp *sectp;
    WALLp wp;
    SECTOR_OBJECTp sop;

//    if (wph->nextsector >= 0)
//        return(DetectSectorObject(&sector[wph->nextsector]));

    // collapse the SO to a single point
    // move all points to nx,ny
    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (sop->xmid == MAXLONG || sop->xmid == MAXSO)
            continue;

        for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
        {
            startwall = (*sectp)->wallptr;
            endwall = startwall + (*sectp)->wallnum - 1;

            for (wp = &wall[startwall], k = startwall; k <= endwall; wp++, k++)
            {
                // if outer wall check the NEXTWALL also
                if (TEST(wp->extra, WALLFX_LOOP_OUTER))
                {
                    if (wph == &wall[wp->nextwall])
                        return sop;
                }

                if (wph == wp)
                    return sop;
            }
        }
    }

    return NULL;
}


void
CollapseSectorObject(SECTOR_OBJECTp sop, int nx, int ny)
{
    short j, k, startwall, endwall;
    SECTORp *sectp;
    WALLp wp;

    // collapse the SO to a single point
    // move all points to nx,ny
    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if (!TEST(sop->flags, SOBJ_SPRITE_OBJ))
        {
            startwall = (*sectp)->wallptr;
            endwall = startwall + (*sectp)->wallnum - 1;

            // move all walls in sectors back to the original position
            for (wp = &wall[startwall], k = startwall; k <= endwall; wp++, k++)
            {
                if (TEST(wp->extra, WALLFX_DONT_MOVE))
                    continue;

                if (wp->extra && TEST(wp->extra, WALLFX_LOOP_OUTER))
                {
                    dragpoint(k, nx, ny, 0);
                }
                else
                {
                    wp->x = nx;
                    wp->y = ny;
                }
            }
        }
    }
}


void
MoveZ(SECTOR_OBJECTp sop)
{
    short i;
    SECTORp *sectp;

    if (sop->bob_amt)
    {
        SPRITEp sp;
        USERp u;

        sop->bob_sine_ndx = (totalsynctics << sop->bob_speed) & 2047;
        sop->bob_diff = ((sop->bob_amt * (int) sintable[sop->bob_sine_ndx]) >> 14);

        // for all sectors
        for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
        {
            if (SectUser[sop->sector[i]] && TEST(SectUser[sop->sector[i]]->flags, SECTFU_SO_DONT_BOB))
                continue;

            (*sectp)->floorz = sop->zorig_floor[i] + sop->bob_diff;
        }
    }

    if (TEST(sop->flags, SOBJ_MOVE_VERTICAL))
    {
        i = AnimGetGoal(&sop->zmid);
        if (i < 0)
            RESET(sop->flags, SOBJ_MOVE_VERTICAL);
    }

    if (TEST(sop->flags, SOBJ_ZDIFF_MODE))
    {
        return;
    }

    // move all floors
    if (TEST(sop->flags, SOBJ_ZDOWN))
    {
        for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
        {
            AnimSet(&(*sectp)->floorz, sop->zorig_floor[i] + sop->z_tgt, sop->z_rate);
        }

        RESET(sop->flags, SOBJ_ZDOWN);
    }
    else if (TEST(sop->flags, SOBJ_ZUP))
    {
        for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
        {
            AnimSet(&(*sectp)->floorz, sop->zorig_floor[i] + sop->z_tgt, sop->z_rate);
        }

        RESET(sop->flags, SOBJ_ZUP);
    }
}

void CallbackSOsink(ANIMp ap, void *data)
{
    SECTOR_OBJECTp sop;
    SECTORp *sectp;
    SPRITEp sp;
    USERp u;
    SECT_USERp su;
    short startwall, endwall, j;
    short dest_sector = -1;
    short src_sector = -1;
    short i, nexti, ndx;
    char found = FALSE;
    int tgt_depth;

    sop = data;

    for (i = 0; sop->sector[i] != -1; i++)
    {
        if (SectUser[sop->sector[i]] && TEST(SectUser[sop->sector[i]]->flags, SECTFU_SO_SINK_DEST))
        {
            src_sector = sop->sector[i];
            break;
        }
    }

    ASSERT(src_sector != -1);

    for (i = 0; sop->sector[i] != -1; i++)
    {
        if (ap->ptr == &sector[sop->sector[i]].floorz)
        {
            dest_sector = sop->sector[i];
            break;
        }
    }

    ASSERT(dest_sector != -1);


    sector[dest_sector].floorpicnum = sector[src_sector].floorpicnum;
    sector[dest_sector].floorshade = sector[src_sector].floorshade;
//    sector[dest_sector].floorz = sector[src_sector].floorz;

    RESET(sector[dest_sector].floorstat, FLOOR_STAT_RELATIVE);

    su = GetSectUser(dest_sector);

    ASSERT(su != NULL);

    ASSERT(GetSectUser(src_sector));
    tgt_depth = (GetSectUser(src_sector))->depth;

#if 0
    for (w = &Water[0]; w < &Water[MAX_WATER]; w++)
    {
        if (w->sector == dest_sector)
        {
            ndx = AnimSet(&w->depth, Z(tgt_depth), ap->vel>>8);
            AnimSetVelAdj(ndx, ap->vel_adj);

            // This is interesting
            // Added a depth_fract to the struct so I could do a
            // 16.16 Fixed point representation to change the depth
            // in a more precise way
            ndx = AnimSet((int *)&su->depth_fract, tgt_depth<<16, (ap->vel<<8)>>8);
            AnimSetVelAdj(ndx, ap->vel_adj);

            found = TRUE;
            break;
        }
    }
#else
    {
        short sectnum;
        for (sectnum = 0; sectnum < numsectors; sectnum++)
        {
            if (sectnum == dest_sector)
            {
                // This is interesting
                // Added a depth_fract to the struct so I could do a
                // 16.16 Fixed point representation to change the depth
                // in a more precise way
                ndx = AnimSet((int *)&su->depth_fract, tgt_depth<<16, (ap->vel<<8)>>8);
                AnimSetVelAdj(ndx, ap->vel_adj);
                found = TRUE;
                break;
            }
        }
    }
#endif

    ASSERT(found);

    TRAVERSE_SPRITE_SECT(headspritesect[dest_sector], i, nexti)
    {
        sp = &sprite[i];
        u = User[i];

        if (!u || u->PlayerP || !TEST(u->Flags, SPR_SO_ATTACHED))
            continue;

        // move sprite WAY down in water
        ndx = AnimSet(&u->sz, -u->sz - SPRITEp_SIZE_Z(sp) - Z(100), ap->vel>>8);
        AnimSetVelAdj(ndx, ap->vel_adj);
    }


    // Take out any blocking walls
    startwall = sector[dest_sector].wallptr;
    endwall = startwall + sector[dest_sector].wallnum - 1;
    for (j = startwall; j <= endwall; j++)
    {
        RESET(wall[j].cstat, CSTAT_WALL_BLOCK);
    }

    return;
}


void
MoveSectorObjects(SECTOR_OBJECTp sop, short locktics)
{
    int j, k, c, nx, ny, nz, rx, ry, dx, dy, dz;
    short speed;
    int dist;
    short startwall, endwall;
    short delta_ang;
    short pnum;
    PLAYERp pp;
    short sp, next_sp;

    if (sop->track >= SO_OPERATE_TRACK_START)
    {
        if (TEST(sop->flags, SOBJ_UPDATE_ONCE))
        {
            RESET(sop->flags, SOBJ_UPDATE_ONCE);
            RefreshPoints(sop, 0, 0, FALSE);
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
    delta_ang = GetDeltaAngle(sop->ang_tgt, sop->ang);

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

    if (TEST(sop->flags, SOBJ_DYNAMIC))
    {
        // trick tricks
        RefreshPoints(sop, nx, ny, TRUE);
    }
    else
    {
        // Update the points so there will be no warping
        if (TEST(sop->flags, SOBJ_UPDATE|SOBJ_UPDATE_ONCE) ||
            sop->vel ||
            (sop->ang != sop->ang_tgt) ||
            GlobSpeedSO)
        {
            RESET(sop->flags, SOBJ_UPDATE_ONCE);
            RefreshPoints(sop, nx, ny, FALSE);
        }
    }
}

void DoTrack(SECTOR_OBJECTp sop, short locktics, int *nx, int *ny)
{
    TRACK_POINTp tpoint;
    int dx, dy, dz;
    int dist;

    tpoint = Track[sop->track].TrackPoint + sop->point;

    // calculate an angle to the target

    if (sop->vel)
        sop->ang_moving = sop->ang_tgt = getangle(tpoint->x - sop->xmid, tpoint->y - sop->ymid);

    // NOTE: Jittery ride - try new value out here
    // NOTE: Put a loop around this (locktics) to make it more acuruate
#define TRACK_POINT_SIZE 200
    //dist = Distance(sop->xmid, sop->ymid, tpoint->x, tpoint->y);
    //if (dist < TRACK_POINT_SIZE)
    if (sop->target_dist < 100)
    {
        switch (tpoint->tag_low)
        {
        case TRACK_MATCH_EVERYTHING:
            DoMatchEverything(NULL, tpoint->tag_high, -1);
            break;

        case TRACK_MATCH_EVERYTHING_ONCE:
            DoMatchEverything(NULL, tpoint->tag_high, -1);
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
            SET(sop->flags, SOBJ_ZMID_FLOOR);
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
            RESET(sop->flags, SOBJ_SLOW_DOWN | SOBJ_SPEED_UP);
            if (sop->dir < 0)
            {
                // set target to new slower target
                sop->vel_tgt = sop->vel_tgt - (tpoint->tag_high * 256);
                SET(sop->flags, SOBJ_SLOW_DOWN);
            }
            else
            {
                sop->vel_tgt = sop->vel_tgt + (tpoint->tag_high * 256);
                SET(sop->flags, SOBJ_SPEED_UP);
            }

            break;

        case TRACK_SLOW_DOWN:
            RESET(sop->flags, SOBJ_SLOW_DOWN | SOBJ_SPEED_UP);
            if (sop->dir > 0)
            {
                sop->vel_tgt = sop->vel_tgt - (tpoint->tag_high * 256);
                SET(sop->flags, SOBJ_SLOW_DOWN);
            }
            else
            {
                sop->vel_tgt = sop->vel_tgt + (tpoint->tag_high * 256);
                SET(sop->flags, SOBJ_SPEED_UP);
            }
            break;

        //
        // Controls z
        //

        case TRACK_SO_SINK:
        {
            SECTORp *sectp;
            short dest_sector = -1;
            short i,ndx;

            for (i = 0; sop->sector[i] != -1; i++)
            {
                if (SectUser[sop->sector[i]] && TEST(SectUser[sop->sector[i]]->flags, SECTFU_SO_SINK_DEST))
                {
                    dest_sector = sop->sector[i];
                    break;
                }
            }

            ASSERT(dest_sector != -1);

            sop->bob_speed = 0;
            sop->bob_sine_ndx = 0;
            sop->bob_amt = 0;

            //DSPRINTF(ds,"dest sector %d",dest_sector);
            MONO_PRINT(ds);

            for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
            {
                if (SectUser[sop->sector[i]] && TEST(SectUser[sop->sector[i]]->flags, SECTFU_SO_DONT_SINK))
                    continue;

                ndx = AnimSet(&(*sectp)->floorz, sector[dest_sector].floorz, tpoint->tag_high);
                AnimSetCallback(ndx, CallbackSOsink, sop);
                AnimSetVelAdj(ndx, 6);
            }

            break;
        }

        case TRACK_SO_FORM_WHIRLPOOL:
        {
            // for lowering the whirlpool in level 1
            SECTORp *sectp;
            short i,ndx;
            SECT_USERp sectu;

            for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
            {
                sectu = SectUser[*sectp - sector];

                if (sectu && sectu->stag == SECT_SO_FORM_WHIRLPOOL)
                {
                    AnimSet(&(*sectp)->floorz, (*sectp)->floorz + Z(sectu->height), 128);
                    (*sectp)->floorshade += sectu->height/6;

                    RESET((*sectp)->extra, SECTFX_NO_RIDE);
                }
            }

            break;
        }

        case TRACK_MOVE_VERTICAL:
        {
            int zr;
            SET(sop->flags, SOBJ_MOVE_VERTICAL);

            if (tpoint->tag_high > 0)
                zr = tpoint->tag_high;
            else
                zr = 256;

            // look at the next point
            NextTrackPoint(sop);
            tpoint = Track[sop->track].TrackPoint + sop->point;

            // set anim
            AnimSet(&sop->zmid, tpoint->z, zr);

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

            SET(sop->flags, SOBJ_WAIT_FOR_EVENT);
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
            SET(sop->flags, SOBJ_ZDIFF_MODE);
            sop->zdelta = Z(tpoint->tag_high);
            break;
        case TRACK_ZRATE:
            sop->z_rate = Z(tpoint->tag_high);
            break;
        case TRACK_ZUP:
            RESET(sop->flags, SOBJ_ZDOWN | SOBJ_ZUP);
            if (sop->dir < 0)
            {
                sop->z_tgt = sop->z_tgt + Z(tpoint->tag_high);
                SET(sop->flags, SOBJ_ZDOWN);
            }
            else
            {
                sop->z_tgt = sop->z_tgt - Z(tpoint->tag_high);
                SET(sop->flags, SOBJ_ZUP);
            }
            break;
        case TRACK_ZDOWN:
            RESET(sop->flags, SOBJ_ZDOWN | SOBJ_ZUP);
            if (sop->dir > 0)
            {
                sop->z_tgt = sop->z_tgt + Z(tpoint->tag_high);
                SET(sop->flags, SOBJ_ZDOWN);
            }
            else
            {
                sop->z_tgt = sop->z_tgt - Z(tpoint->tag_high);
                SET(sop->flags, SOBJ_ZUP);
            }
            break;
        }

        // get the next point
        NextTrackPoint(sop);
        tpoint = Track[sop->track].TrackPoint + sop->point;

        // calculate distance to target poing
        sop->target_dist = Distance(sop->xmid, sop->ymid, tpoint->x, tpoint->y);

        // calculate a new angle to the target
        sop->ang_moving = sop->ang_tgt = getangle(tpoint->x - sop->xmid, tpoint->y - sop->ymid);

        if (TEST(sop->flags, SOBJ_ZDIFF_MODE))
        {
            int dist;
            short i;

            // set dx,dy,dz up for finding the z magnitude
            dx = tpoint->x;
            dy = tpoint->y;
            dz = tpoint->z - sop->zdelta;

            // find the distance to the target (player)
            dist = DIST(dx, dy, sop->xmid, sop->ymid);

            // (velocity * difference between the target and the object)
            // / distance
            sop->z_rate = (sop->vel * (sop->zmid - dz)) / dist;

            // take absolute value and convert to pixels (divide by 256)
            sop->z_rate = PIXZ(labs(sop->z_rate));

            if (TEST(sop->flags, SOBJ_SPRITE_OBJ))
            {
                // only modify zmid for sprite_objects
                AnimSet(&sop->zmid, dz, sop->z_rate);
            }
            else
            {
                // churn through sectors setting their new z values
                for (i = 0; sop->sector[i] != -1; i++)
                {
                    AnimSet(&sector[sop->sector[i]].floorz, dz - (sector[sop->mid_sector].floorz - sector[sop->sector[i]].floorz), sop->z_rate);
                }
            }
        }
    }
    else
    {

        // make velocity approach the target velocity
        if (TEST(sop->flags, SOBJ_SPEED_UP))
        {
            if ((sop->vel += (locktics << sop->vel_rate)) >= sop->vel_tgt)
            {
                sop->vel = sop->vel_tgt;
                RESET(sop->flags, SOBJ_SPEED_UP);
            }
        }
        else if (TEST(sop->flags, SOBJ_SLOW_DOWN))
        {
            if ((sop->vel -= (locktics << sop->vel_rate)) <= sop->vel_tgt)
            {
                sop->vel = sop->vel_tgt;
                RESET(sop->flags, SOBJ_SLOW_DOWN);
            }
        }
    }

    // calculate a new x and y
    if (sop->vel && !TEST(sop->flags,SOBJ_MOVE_VERTICAL))
    {
        *nx = (DIV256(sop->vel)) * locktics * (int) sintable[NORM_ANGLE(sop->ang_moving + 512)] >> 14;
        *ny = (DIV256(sop->vel)) * locktics * (int) sintable[sop->ang_moving] >> 14;

        dist = Distance(sop->xmid, sop->ymid, sop->xmid + *nx, sop->ymid + *ny);
        sop->target_dist -= dist;
    }
}


void
OperateSectorObject(SECTOR_OBJECTp sop, short newang, int newx, int newy)
{
    int i, nx, ny;
    short speed;
    short delta_ang;
    SECTORp *sectp;

    if (Prediction)
        return;

    if (sop->track < SO_OPERATE_TRACK_START)
        return;

    if (sop->bob_amt)
    {
        sop->bob_sine_ndx = (totalsynctics << sop->bob_speed) & 2047;
        sop->bob_diff = ((sop->bob_amt * (int) sintable[sop->bob_sine_ndx]) >> 14);

        // for all sectors
        for (i = 0, sectp = &sop->sectp[0]; *sectp; sectp++, i++)
        {
            if (SectUser[sop->sector[i]] && TEST(SectUser[sop->sector[i]]->flags, SECTFU_SO_DONT_BOB))
                continue;

            (*sectp)->floorz = sop->zorig_floor[i] + sop->bob_diff;
        }
    }

    nx = 0;
    ny = 0;
    GlobSpeedSO = 0;

    delta_ang = 0;

    //sop->ang_tgt = newang;
    sop->ang_moving = newang;

    // get delta to target angle
    delta_ang = GetDeltaAngle(sop->ang_tgt, sop->ang);

    sop->spin_ang = 0;
    sop->ang = newang;

    RefreshPoints(sop, newx - sop->xmid, newy - sop->ymid, FALSE);
}

void
PlaceSectorObject(SECTOR_OBJECTp sop, short newang, int newx, int newy)
{
    RefreshPoints(sop, newx - sop->xmid, newy - sop->ymid, FALSE);
}

void VehicleSetSmoke(SECTOR_OBJECTp sop, ANIMATORp animator)
{
    short SpriteNum, NextSprite;
    SECTORp *sectp;
    SPRITEp sp;
    USERp u;

    for (sectp = sop->sectp; *sectp; sectp++)
    {
        TRAVERSE_SPRITE_SECT(headspritesect[*sectp - sector], SpriteNum, NextSprite)
        {
            sp = &sprite[SpriteNum];
            u = User[SpriteNum];

            switch (sp->hitag)
            {

            case SPAWN_SPOT:
                if (sp->clipdist == 3)
                {
                    if (animator)
                    {
                        if (sp->statnum == STAT_NO_STATE)
                            break;

                        change_sprite_stat(SpriteNum, STAT_NO_STATE);
                        DoSoundSpotMatch(sp->lotag, 1, 0);
                        DoSpawnSpotsForDamage(sp->lotag);
                    }
                    else
                    {
                        change_sprite_stat(SpriteNum, STAT_SPAWN_SPOT);
                        DoSoundSpotStopSound(sp->lotag);
                    }

                    u->ActorActionFunc = animator;
                }
                break;
            }
        }
    }
}


void
KillSectorObject(SECTOR_OBJECTp sop)
{
    int nx, ny, nz;
    short speed;
    short delta_ang;
    SECTORp *sectp;
    int newx = MAXSO;
    int newy = MAXSO;
    short newang = 0;

    if (sop->track < SO_OPERATE_TRACK_START)
        return;

    nx = 0;
    ny = 0;
    delta_ang = 0;

    sop->ang_tgt = sop->ang_moving = newang;

    // get delta to target angle
    delta_ang = GetDeltaAngle(sop->ang_tgt, sop->ang);

    sop->spin_ang = 0;
    sop->ang = sop->ang_tgt;

    RefreshPoints(sop, newx - sop->xmid, newy - sop->ymid, FALSE);
}


void TornadoSpin(SECTOR_OBJECTp sop)
{
    short delta_ang, speed;
    short locktics = synctics;

    // get delta to target angle
    delta_ang = GetDeltaAngle(sop->ang_tgt, sop->ang);

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

void
DoTornadoObject(SECTOR_OBJECTp sop)
{
    short delta_ang;
    SECTORp *sectp;
    int xvect,yvect;
    short cursect;
    // this made them move together more or less - cool!
    //static short ang = 1024;
    int floor_dist;
    vec3_t pos;
    int ret;
    short *ang = &sop->ang_moving;

    xvect = (sop->vel * sintable[NORM_ANGLE(*ang + 512)]);
    yvect = (sop->vel * sintable[NORM_ANGLE(*ang)]);

    cursect = sop->op_main_sector; // for sop->vel
    floor_dist = DIV4(labs(sector[cursect].ceilingz - sector[cursect].floorz));
    pos.x = sop->xmid;
    pos.y = sop->ymid;
    pos.z = floor_dist;

    PlaceSectorObject(sop, *ang, MAXSO, MAXSO);
    ret = clipmove(&pos, &cursect, xvect, yvect, (int)sop->clipdist, Z(0), floor_dist, CLIPMASK_ACTOR);

    if (ret)
    {
        *ang = NORM_ANGLE(*ang + 1024 + RANDOM_P2(512) - 256);
    }

    TornadoSpin(sop);
    RefreshPoints(sop, pos.x - sop->xmid, pos.y - sop->ymid, TRUE);
}

void
DoAutoTurretObject(SECTOR_OBJECTp sop)
{
    short SpriteNum = sop->sp_child - sprite;
    SPRITEp shootp;
    USERp u = User[SpriteNum];
    short new_ang;
    short delta_ang;
    int diff;
    int dist;
    short i;

    if (sop->max_damage != -9999 && sop->max_damage <= 0)
        return;


    u->WaitTics -= synctics;

    // check for new player if doesn't have a target or time limit expired
    if (!u->tgt_sp || u->WaitTics < 0)
    {
        // 4 seconds
        u->WaitTics = 4*120;
        DoActorPickClosePlayer(SpriteNum);
    }

    if (MoveSkip2 == 0)
    {
        for (i = 0; sop->sp_num[i] != -1; i++)
        {
            if (sprite[sop->sp_num[i]].statnum == STAT_SO_SHOOT_POINT)
            {
                shootp = &sprite[sop->sp_num[i]];

                if (!FAFcansee(shootp->x, shootp->y, shootp->z-Z(4), shootp->sectnum,
                               u->tgt_sp->x, u->tgt_sp->y, SPRITEp_UPPER(u->tgt_sp), u->tgt_sp->sectnum))
                {
                    return;
                }
            }
        }


        // FirePausing
        if (u->Counter > 0)
        {
            u->Counter -= synctics*2;
            if (u->Counter <= 0)
                u->Counter = 0;
        }

        if (u->Counter == 0)
        {
            shootp = NULL;
            for (i = 0; sop->sp_num[i] != -1; i++)
            {
                if (sprite[sop->sp_num[i]].statnum == STAT_SO_SHOOT_POINT)
                {
                    shootp = &sprite[sop->sp_num[i]];

                    if (SP_TAG5(shootp))
                        u->Counter = SP_TAG5(shootp);
                    else
                        u->Counter = 12;
                    InitTurretMgun(sop);
                }
            }
        }

        //sop->ang_tgt = getangle(sop->xmid - u->tgt_sp->x, sop->ymid - u->tgt_sp->y);
        sop->ang_tgt = getangle(u->tgt_sp->x - sop->xmid,  u->tgt_sp->y - sop->ymid);

        // get delta to target angle
        delta_ang = GetDeltaAngle(sop->ang_tgt, sop->ang);

        //sop->ang += delta_ang >> 4;
        sop->ang = NORM_ANGLE(sop->ang + (delta_ang >> 3));
        //sop->ang += delta_ang >> 2;

        if (sop->limit_ang_center >= 0)
        {
            diff = GetDeltaAngle(sop->ang, sop->limit_ang_center);

            if (labs(diff) >= sop->limit_ang_delta)
            {
                if (diff < 0)
                    sop->ang = sop->limit_ang_center - sop->limit_ang_delta;
                else
                    sop->ang = sop->limit_ang_center + sop->limit_ang_delta;

            }
        }

        OperateSectorObject(sop, sop->ang, sop->xmid, sop->ymid);
    }
}


void
DoActorHitTrackEndPoint(USERp u)
{
    SPRITEp sp = u->SpriteP;

    RESET(Track[u->track].flags, TF_TRACK_OCCUPIED);

    // jump the current track & determine if you should go to another
    if (TEST(u->Flags, SPR_RUN_AWAY))
    {
        short FindTrackAwayFromPlayer(USERp);

        //DSPRINTF(ds, "End Of Track - Looking for another!\n");
        MONO_PRINT(ds);

        // look for another track leading away from the player
        u->track = FindTrackAwayFromPlayer(u);

        if (u->track >= 0)
        {
            sp->ang = NORM_ANGLE(getangle((Track[u->track].TrackPoint + u->point)->x - sp->x, (Track[u->track].TrackPoint + u->point)->y - sp->y));
            //DSPRINTF(ds, "Track Away From Player!\n");
            MONO_PRINT(ds);
        }
        else
        {
            RESET(u->Flags, SPR_RUN_AWAY);
            DoActorSetSpeed(sp - sprite, NORM_SPEED);
            u->track = -1;
        }
    }
    else if (TEST(u->Flags, SPR_FIND_PLAYER))
    {
        short FindTrackToPlayer(USERp);

        //DSPRINTF(ds, "End Of Track - Looking for another!\n");
        MONO_PRINT(ds);

        // look for another track leading away from the player
        u->track = FindTrackToPlayer(u);

        if (u->track >= 0)
        {
            sp->ang = NORM_ANGLE(getangle((Track[u->track].TrackPoint + u->point)->x - sp->x, (Track[u->track].TrackPoint + u->point)->y - sp->y));
        }
        else
        {
            RESET(u->Flags, SPR_FIND_PLAYER);
            DoActorSetSpeed(sp - sprite, NORM_SPEED);
            u->track = -1;
        }
    }
    else
    {
        //DSPRINTF(ds, "End Of Track - DONT Look for another!\n");
        MONO_PRINT(ds);

        u->track = -1;
    }
}


void
ActorLeaveTrack(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (u->track == -1)
        return;

    RESET(u->Flags, SPR_FIND_PLAYER|SPR_RUN_AWAY|SPR_CLIMBING);
    RESET(Track[u->track].flags, TF_TRACK_OCCUPIED);
    u->track = -1;
}

/*
ScanToWall
(lsp->x, lsp->y, SPRITEp_TOS(sp) - DIV2(SPRITEp_SIZE_Z(sp)), lsp->sectnum,
    sintable[NORM_ANGLE(lsp->ang + 1024 + 512)],
    sintable[lsp->ang + 1024],
    0,
    &hitinfo);
*/


SWBOOL
ActorTrackDecide(TRACK_POINTp tpoint, short SpriteNum)
{
    SPRITEp sp;
    USERp u = User[SpriteNum];

    sp = u->SpriteP;

    //DSPRINTF(ds,"tpoint->tag_low = %d, u->ID = %d\n",tpoint->tag_low,u->ID);
    MONO_PRINT(ds);

    switch (tpoint->tag_low)
    {
    case TRACK_START:

        // if track has a type and actor is going the right direction jump
        // the track
        if (Track[u->track].ttflags)
        {
            if (u->track_dir == -1)
            {
                DoActorHitTrackEndPoint(u);
                return FALSE;
            }
        }

        break;

    case TRACK_END:
        // if track has a type and actor is going to right direction jump the
        // track
        if (Track[u->track].ttflags)
        {
            if (u->track_dir == 1)
            {
                DoActorHitTrackEndPoint(u);
                return FALSE;
            }
        }

        break;


    case TRACK_ACTOR_WAIT_FOR_PLAYER:
    {
        SET(u->Flags, SPR_WAIT_FOR_PLAYER);
        u->Dist = tpoint->tag_high;
        break;
    }

    case TRACK_ACTOR_WAIT_FOR_TRIGGER:
    {
        SET(u->Flags, SPR_WAIT_FOR_TRIGGER);
        u->Dist = tpoint->tag_high;
        break;
    }

    //
    // Controls the velocity
    //

    case TRACK_ACTOR_VEL_RATE:
        u->vel_rate = tpoint->tag_high;
        break;
    case TRACK_ACTOR_SPEED_UP:
        RESET(u->Flags, SPR_SLOW_DOWN | SPR_SPEED_UP);
        if (u->track_dir < 0)
        {
            // set target to new slower target
            u->vel_tgt = u->vel_tgt - (tpoint->tag_high * 256);
            SET(u->Flags, SPR_SLOW_DOWN);
        }
        else
        {
            u->vel_tgt = u->vel_tgt + (tpoint->tag_high * 256);
            SET(u->Flags, SPR_SPEED_UP);
        }

        break;

    case TRACK_ACTOR_SLOW_DOWN:
        RESET(u->Flags, SPR_SLOW_DOWN | SPR_SPEED_UP);
        if (u->track_dir > 0)
        {
            u->vel_tgt = u->vel_tgt - (tpoint->tag_high * 256);
            SET(u->Flags, SPR_SLOW_DOWN);
        }
        else
        {
            u->vel_tgt = u->vel_tgt + (tpoint->tag_high * 256);
            SET(u->Flags, SPR_SPEED_UP);
        }
        break;

    // Reverse it
    case TRACK_ACTOR_REVERSE:
        u->track_dir *= -1;
        break;

    case TRACK_ACTOR_STAND:
        NewStateGroup(SpriteNum, u->ActorActionSet->Stand);
        break;

    case TRACK_ACTOR_JUMP:
        if (u->ActorActionSet->Jump)
        {
            sp->ang = tpoint->ang;

            if (!tpoint->tag_high)
                u->jump_speed = ACTOR_STD_JUMP;
            else
                u->jump_speed = -tpoint->tag_high;

            DoActorBeginJump(SpriteNum);
            u->ActorActionFunc = DoActorMoveJump;
        }

        break;

    case TRACK_ACTOR_QUICK_JUMP:
    case TRACK_ACTOR_QUICK_SUPER_JUMP:
        if (u->ActorActionSet->Jump)
        {
            int DoActorMoveJump(short SpriteNum);
            int PickJumpSpeed(short SpriteNum, int pix_height);
            int zdiff;
            hitdata_t hitinfo;

            sp->ang = tpoint->ang;


            ActorLeaveTrack(SpriteNum);

            if (tpoint->tag_high)
            {
                u->jump_speed = -tpoint->tag_high;
            }
            else
            {
                RESET(sp->cstat, CSTAT_SPRITE_BLOCK);

                FAFhitscan(sp->x, sp->y, sp->z - Z(24), sp->sectnum,      // Start position
                           sintable[NORM_ANGLE(sp->ang + 512)],      // X vector of 3D ang
                           sintable[sp->ang],    // Y vector of 3D ang
                           0,                            // Z vector of 3D ang
                           &hitinfo, CLIPMASK_MISSILE);

                SET(sp->cstat, CSTAT_SPRITE_BLOCK);

                ASSERT(hitinfo.sect >= 0);

                if (hitinfo.sprite >= 0)
                    return FALSE;

                if (hitinfo.wall < 0)
                    return FALSE;

                zdiff = labs(sp->z - sector[wall[hitinfo.wall].nextsector].floorz) >> 8;

                u->jump_speed = PickJumpSpeed(SpriteNum, zdiff);
            }

            DoActorBeginJump(SpriteNum);
            u->ActorActionFunc = DoActorMoveJump;

            return FALSE;
        }

        break;

    case TRACK_ACTOR_QUICK_JUMP_DOWN:

        if (u->ActorActionSet->Jump)
        {
            int DoActorMoveJump(short SpriteNum);

            sp->ang = tpoint->ang;

            ActorLeaveTrack(SpriteNum);

            if (tpoint->tag_high)
            {
                u->jump_speed = -tpoint->tag_high;
            }
            else
            {
                u->jump_speed = -350;
            }

            DoActorBeginJump(SpriteNum);
            u->ActorActionFunc = DoActorMoveJump;
            return FALSE;
        }

        break;

    case TRACK_ACTOR_QUICK_SCAN:

        if (u->ActorActionSet->Jump)
        {
            ActorLeaveTrack(SpriteNum);
            return FALSE;
        }

        break;

    case TRACK_ACTOR_QUICK_DUCK:

        if (u->Rot != u->ActorActionSet->Duck)
        {
            int DoActorDuck(short SpriteNum);

            sp->ang = tpoint->ang;

            ActorLeaveTrack(SpriteNum);

            if (!tpoint->tag_high)
                u->WaitTics = 4 * 120;
            else
                u->WaitTics = tpoint->tag_high * 128;

            InitActorDuck(SpriteNum);
            u->ActorActionFunc = DoActorDuck;
            return FALSE;
        }

        break;

    case TRACK_ACTOR_OPERATE:
    case TRACK_ACTOR_QUICK_OPERATE:
    {
        short nearsector, nearwall, nearsprite;
        int nearhitdist;
        int z[2];
        int i;

        if (u->Rot == u->ActorActionSet->Sit || u->Rot == u->ActorActionSet->Stand)
            return FALSE;

        sp->ang = tpoint->ang;

//          //DSPRINTF(ds,"sp->x = %ld, sp->y = %ld, sp->sector = %d, tp->x = %ld, tp->y = %ld, tp->ang = %d\n",sp->x,sp->y,sp->sectnum,tpoint->x,tpoint->y,tpoint->ang);
//          MONO_PRINT(ds);

        z[0] = sp->z - SPRITEp_SIZE_Z(sp) + Z(5);
        z[1] = sp->z - DIV2(SPRITEp_SIZE_Z(sp));

        for (i = 0; i < (int)SIZ(z); i++)
        {
            neartag(sp->x, sp->y, z[i], sp->sectnum, sp->ang,
                    &nearsector, &nearwall, &nearsprite,
                    &nearhitdist, 1024L, NTAG_SEARCH_LO_HI, NULL);

//              //DSPRINTF(ds,"nearsector = %d, nearwall = %d, nearsprite = %d hitdist == %ld\n",nearsector,nearwall,nearsprite,nearhitdist);
//              MONO_PRINT(ds);

            if (nearsprite >= 0 && nearhitdist < 1024)
            {
                if (OperateSprite(nearsprite, FALSE))
                {
                    if (!tpoint->tag_high)
                        u->WaitTics = 2 * 120;
                    else
                        u->WaitTics = tpoint->tag_high * 128;

                    NewStateGroup(SpriteNum, u->ActorActionSet->Stand);
                }
            }
        }

        if (nearsector >= 0 && nearhitdist < 1024)
        {
            if (OperateSector(nearsector, FALSE))
            {
                if (!tpoint->tag_high)
                    u->WaitTics = 2 * 120;
                else
                    u->WaitTics = tpoint->tag_high * 128;

                NewStateGroup(SpriteNum, u->ActorActionSet->Sit);
            }
        }

        if (nearwall >= 0 && nearhitdist < 1024)
        {
            if (OperateWall(nearwall, FALSE))
            {
                if (!tpoint->tag_high)
                    u->WaitTics = 2 * 120;
                else
                    u->WaitTics = tpoint->tag_high * 128;

                NewStateGroup(SpriteNum, u->ActorActionSet->Stand);
            }
        }

        break;
    }

    case TRACK_ACTOR_JUMP_IF_FORWARD:
        if (u->ActorActionSet->Jump && u->track_dir == 1)
        {
            if (!tpoint->tag_high)
                u->jump_speed = ACTOR_STD_JUMP;
            else
                u->jump_speed = -tpoint->tag_high;

            DoActorBeginJump(SpriteNum);
        }

        break;

    case TRACK_ACTOR_JUMP_IF_REVERSE:
        if (u->ActorActionSet->Jump && u->track_dir == -1)
        {
            if (!tpoint->tag_high)
                u->jump_speed = ACTOR_STD_JUMP;
            else
                u->jump_speed = -tpoint->tag_high;

            DoActorBeginJump(SpriteNum);
        }

        break;

    case TRACK_ACTOR_CRAWL:
        if (u->Rot != u->ActorActionSet->Crawl)
            NewStateGroup(SpriteNum, u->ActorActionSet->Crawl);
        else
            NewStateGroup(SpriteNum, u->ActorActionSet->Rise);
        break;

    case TRACK_ACTOR_SWIM:
        if (u->Rot != u->ActorActionSet->Swim)
            NewStateGroup(SpriteNum, u->ActorActionSet->Swim);
        else
            NewStateGroup(SpriteNum, u->ActorActionSet->Rise);
        break;

    case TRACK_ACTOR_FLY:
        NewStateGroup(SpriteNum, u->ActorActionSet->Fly);
        break;

    case TRACK_ACTOR_SIT:

        if (u->ActorActionSet->Sit)
        {
            if (!tpoint->tag_high)
                u->WaitTics = 3 * 120;
            else
                u->WaitTics = tpoint->tag_high * 128;

            NewStateGroup(SpriteNum, u->ActorActionSet->Sit);
        }

        break;

    case TRACK_ACTOR_DEATH1:
        if (u->ActorActionSet->Death2)
        {
            u->WaitTics = 4 * 120;
            NewStateGroup(SpriteNum, u->ActorActionSet->Death1);
        }
        break;

    case TRACK_ACTOR_DEATH2:

        if (u->ActorActionSet->Death2)
        {
            u->WaitTics = 4 * 120;
            NewStateGroup(SpriteNum, u->ActorActionSet->Death2);
        }

        break;

    case TRACK_ACTOR_DEATH_JUMP:

        if (u->ActorActionSet->DeathJump)
        {
            SET(u->Flags, SPR_DEAD);
            sp->xvel <<= 1;
            u->jump_speed = -495;
            DoActorBeginJump(SpriteNum);
            NewStateGroup(SpriteNum, u->ActorActionSet->DeathJump);
        }

        break;

    case TRACK_ACTOR_CLOSE_ATTACK1:

        if (u->ActorActionSet->CloseAttack[0])
        {
            if (!tpoint->tag_high)
                u->WaitTics = 2 * 120;
            else
                u->WaitTics = tpoint->tag_high * 128;

            NewStateGroup(SpriteNum, u->ActorActionSet->CloseAttack[0]);
        }

        break;

    case TRACK_ACTOR_CLOSE_ATTACK2:

        if (u->ActorActionSet->CloseAttack[1])
        {
            if (!tpoint->tag_high)
                u->WaitTics = 4 * 120;
            else
                u->WaitTics = tpoint->tag_high * 128;

            NewStateGroup(SpriteNum, u->ActorActionSet->CloseAttack[1]);
        }

        break;

    case TRACK_ACTOR_ATTACK1:
    case TRACK_ACTOR_ATTACK2:
    case TRACK_ACTOR_ATTACK3:
    case TRACK_ACTOR_ATTACK4:
    case TRACK_ACTOR_ATTACK5:
    case TRACK_ACTOR_ATTACK6:
    {
        STATEp **ap = &u->ActorActionSet->Attack[0] + (tpoint->tag_low - TRACK_ACTOR_ATTACK1);


        if (*ap)
        {
            if (!tpoint->tag_high)
                u->WaitTics = 4 * 120;
            else
                u->WaitTics = tpoint->tag_high * 128;

            NewStateGroup(SpriteNum, *ap);
        }

        break;
    }

    case TRACK_ACTOR_ZDIFF_MODE:
        if (TEST(u->Flags, SPR_ZDIFF_MODE))
        {
            RESET(u->Flags, SPR_ZDIFF_MODE);
            sp->z = sector[sp->sectnum].floorz;
            sp->zvel = 0;
        }
        else
        {
            SET(u->Flags, SPR_ZDIFF_MODE);
        }
        break;

    case TRACK_ACTOR_CLIMB_LADDER:

        if (u->ActorActionSet->Jump)
        {
            short hit_sect, hit_wall, hit_sprite;
            int bos_z,nx,ny;
            int dist;
            SPRITEp lsp;
            SPRITEp FindNearSprite(SPRITEp, short);

            //
            // Get angle and x,y pos from CLIMB_MARKER
            //
            lsp = FindNearSprite(sp, STAT_CLIMB_MARKER);

            if (!lsp)
            {
                ActorLeaveTrack(SpriteNum);
                return FALSE;
            }

            // determine where the player is supposed to be in relation to the ladder
            // move out in front of the ladder
            nx = MOVEx(100, lsp->ang);
            ny = MOVEy(100, lsp->ang);

            sp->x = lsp->x + nx;
            sp->y = lsp->y + ny;

            sp->ang = NORM_ANGLE(lsp->ang + 1024);

            //
            // Get the z height to climb
            //

            neartag(sp->x, sp->y, SPRITEp_TOS(sp) - DIV2(SPRITEp_SIZE_Z(sp)), sp->sectnum,
                    sp->ang,
                    &hit_sect, &hit_wall, &hit_sprite,
                    &dist, 600L, NTAG_SEARCH_LO_HI, NULL);

            if (hit_wall < 0)
            {
                ActorLeaveTrack(SpriteNum);
                return FALSE;
            }

#if DEBUG
            if (wall[hit_wall].nextsector < 0)
            {
                TerminateGame();
                printf("Take out white wall ladder x = %d, y = %d",wall[hit_wall].x, wall[hit_wall].y);
                exit(0);
            }
#endif

            // destination z for climbing
            u->sz = sector[wall[hit_wall].nextsector].floorz;

            DoActorZrange(SpriteNum);

            //
            // Adjust for YCENTERING
            //

            SET(sp->cstat, CSTAT_SPRITE_YCENTER);
            bos_z = SPRITEp_BOS(sp);
            if (bos_z > u->loz)
            {
                u->sy = (bos_z - sp->z);
                sp->z -= u->sy;
            }

            //
            // Misc climb setup
            //

            SET(u->Flags, SPR_CLIMBING);
            NewStateGroup(SpriteNum, u->ActorActionSet->Climb);

            sp->zvel = -Z(1);
        }

        break;

    case TRACK_ACTOR_SET_JUMP:
        u->jump_speed = -tpoint->tag_high;
        break;
    }

    return TRUE;
}

/*

!AIC - This is where actors follow tracks.  Its massy, hard to read, and more
complex than it needs to be.  It was taken from sector object track movement
code.  The routine above ActorTrackDecide() is where a track tag is recognized
and acted upon.  There are quite a few of these that are not useful to us at
present time.

*/

int
ActorFollowTrack(short SpriteNum, short locktics)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    PLAYERp pp;

    int move_actor(short SpriteNum, int xchange, int ychange, int zchange);

    TRACK_POINTp tpoint;
    short pnum;
    int nx = 0, ny = 0, nz = 0, dx, dy, dz;
    int dist;

    // if not on a track then better not go here
    if (u->track == -1)
        return TRUE;

    // if lying in wait for player
    if (TEST(u->Flags, SPR_WAIT_FOR_PLAYER | SPR_WAIT_FOR_TRIGGER))
    {
        if (TEST(u->Flags, SPR_WAIT_FOR_PLAYER))
        {
            TRAVERSE_CONNECT(pnum)
            {
                pp = &Player[pnum];

                if (Distance(sp->x, sp->y, pp->posx, pp->posy) < u->Dist)
                {
                    u->tgt_sp = pp->SpriteP;
                    RESET(u->Flags, SPR_WAIT_FOR_PLAYER);
                    return TRUE;
                }
            }
        }

        u->Tics = 0;
        return TRUE;
    }

    // if pausing the return
    if (u->WaitTics)
    {
        u->WaitTics -= locktics;
        if (u->WaitTics <= 0)
        {
            RESET(u->Flags, SPR_DONT_UPDATE_ANG);
            NewStateGroup(SpriteNum, u->ActorActionSet->Run);
            u->WaitTics = 0;
        }

        return TRUE;
    }

    tpoint = Track[u->track].TrackPoint + u->point;

    if (!(TEST(u->Flags, SPR_CLIMBING | SPR_DONT_UPDATE_ANG)))
    {
        sp->ang = getangle(tpoint->x - sp->x, tpoint->y - sp->y);
    }

    if ((dist = Distance(sp->x, sp->y, tpoint->x, tpoint->y)) < 200) // 64
    {
        if (!ActorTrackDecide(tpoint, SpriteNum))
            return TRUE;

        // get the next point
        NextActorTrackPoint(SpriteNum);
        tpoint = Track[u->track].TrackPoint + u->point;

        if (!(TEST(u->Flags, SPR_CLIMBING | SPR_DONT_UPDATE_ANG)))
        {
            // calculate a new angle to the target
            sp->ang = getangle(tpoint->x - sp->x, tpoint->y - sp->y);
        }

        if (TEST(u->Flags, SPR_ZDIFF_MODE))
        {
            int dist;

            // set dx,dy,dz up for finding the z magnitude
            dx = tpoint->x;
            dy = tpoint->y;
            dz = tpoint->z;

            // find the distance to the target (player)
            dist = DIST(dx, dy, sp->x, sp->y);

            // (velocity * difference between the target and the object) /
            // distance
            sp->zvel = -((sp->xvel * (sp->z - dz)) / dist);
        }
    }
    else
    {
        // make velocity approach the target velocity
        if (TEST(u->Flags, SPR_SPEED_UP))
        {
            if ((u->track_vel += (locktics << u->vel_rate)) >= u->vel_tgt)
            {
                u->track_vel = u->vel_tgt;
                RESET(u->Flags, SPR_SPEED_UP);
            }

            // update the real velocity
            sp->xvel = DIV256(u->track_vel);
        }
        else if (TEST(u->Flags, SPR_SLOW_DOWN))
        {
            if ((u->track_vel -= (locktics << u->vel_rate)) <= u->vel_tgt)
            {
                u->track_vel = u->vel_tgt;
                RESET(u->Flags, SOBJ_SLOW_DOWN);
            }

            sp->xvel = DIV256(u->track_vel);
        }

        nx = 0;
        ny = 0;

        if (TEST(u->Flags, SPR_CLIMBING))
        {
            if (SPRITEp_TOS(sp) + DIV4(SPRITEp_SIZE_Z(sp)) < u->sz)
            {
                ANIMATOR DoActorMoveJump;
                ANIMATOR NinjaJumpActionFunc;
                RESET(u->Flags, SPR_CLIMBING);

                sp->zvel = 0;

                sp->ang = getangle(tpoint->x - sp->x, tpoint->y - sp->y);

                ActorLeaveTrack(SpriteNum);
                RESET(sp->cstat, CSTAT_SPRITE_YCENTER);
                sp->z += u->sy;

                DoActorSetSpeed(SpriteNum, SLOW_SPEED);
                u->ActorActionFunc = NinjaJumpActionFunc;
                u->jump_speed = -650;
                DoActorBeginJump(SpriteNum);

                return TRUE;
            }
        }
        else
        {
            // calculate a new x and y
            nx = sp->xvel * (int) sintable[NORM_ANGLE(sp->ang + 512)] >> 14;
            ny = sp->xvel * (int) sintable[sp->ang] >> 14;
        }

        nz = 0;

        if (sp->zvel)
            nz = sp->zvel * locktics;
    }

    u->ret = move_sprite(SpriteNum, nx, ny, nz, u->ceiling_dist, u->floor_dist, 0, locktics);


    if (u->ret)
    {
        if (!TEST(u->Flags, SPR_JUMPING|SPR_FALLING))
            ActorLeaveTrack(SpriteNum);
    }


    return TRUE;
}


#include "saveable.h"

static saveable_code saveable_track_code[] =
{
    SAVE_CODE(DoTrack),
    SAVE_CODE(DoTornadoObject),
    SAVE_CODE(DoAutoTurretObject),
    SAVE_CODE(DoActorHitTrackEndPoint),
    SAVE_CODE(CallbackSOsink),
};

saveable_module saveable_track =
{
    // code
    saveable_track_code,
    SIZ(saveable_track_code),

    // data
    NULL,0
};

