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

BEGIN_SW_NS

void ScaleSectorObject(SECTOR_OBJECT*);

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

short DoSectorObjectSetScale(short match)
{
    SECTOR_OBJECT* sop;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (SO_EMPTY(sop))
            continue;

        if (sop->match_event == match)
        {
            sop->flags |= (SOBJ_DYNAMIC);
            sop->PreMoveAnimator = ScaleSectorObject;

            switch (sop->scale_active_type)
            {
            case SO_SCALE_RANDOM_POINT:
                if (sop->scale_type == SO_SCALE_HOLD || sop->scale_type == SO_SCALE_NONE)
                {
                    // if holding start it up
                    sop->scale_type = sop->scale_active_type;
                }
                else
                {
                    // if moving set to hold
                    sop->scale_type = SO_SCALE_HOLD;
                }
                break;

            case SO_SCALE_DEST:

                sop->scale_type = sop->scale_active_type;

                if (sop->scale_dist == sop->scale_dist_max)
                {
                    // make it negative
                    if (sop->scale_speed > 0)
                        sop->scale_speed = -sop->scale_speed;
                }
                else if (sop->scale_dist == sop->scale_dist_min)
                {
                    // make it positive
                    if (sop->scale_speed < 0)
                        sop->scale_speed = -sop->scale_speed;
                }
                else
                {
                    // make it positive
                    if (sop->scale_speed < 0)
                        sop->scale_speed = -sop->scale_speed;
                }
                break;

            case SO_SCALE_RANDOM:
            case SO_SCALE_CYCLE:
                if (sop->scale_type == SO_SCALE_HOLD)
                {
                    // if holding start it up
                    sop->scale_type = sop->scale_active_type;
                }
                else
                {
                    // if moving set to hold
                    sop->scale_type = SO_SCALE_HOLD;
                }
                break;
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

short DoSOevent(short match, short state)
{
    SECTOR_OBJECT* sop;
    short vel_adj=0, spin_adj=0;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (SO_EMPTY(sop))
            continue;

        if (sop->match_event == match)
        {
            if ((sop->flags & SOBJ_WAIT_FOR_EVENT))
            {
                if (sop->save_vel > 0 || sop->save_spin_speed > nullAngle)
                {
                    sop->flags &= ~(SOBJ_WAIT_FOR_EVENT);
                    sop->vel = sop->save_vel;
                    sop->spin_speed = sop->save_spin_speed;
                }
            }

            auto me_act = sop->match_event_actor;
            if (me_act == nullptr)
                continue;

            // toggle
            if (state == -1)
            {
                if (TEST_BOOL3(me_act))
                {
                    RESET_BOOL3(me_act);
                    state = 0;
                }
                else
                {
                    SET_BOOL3(me_act);
                    state = 1;
                }
            }

            if (state == 1)
            {
                spin_adj = (int)SP_TAG3(me_act);
                vel_adj = SP_TAG7(me_act);
            }
            else if (state == 0)
            {
                spin_adj = -(int)SP_TAG3(me_act);
                vel_adj = -SP_TAG7(me_act);
            }

            sop->spin_speed += DAngle::fromBuild(spin_adj);

            if (TEST_BOOL1(me_act))
                sop->vel_tgt += vel_adj;
            else
                sop->vel += vel_adj;

            if (TEST_BOOL2(me_act))
            {
                sop->dir *= -1;
            }
        }
    }
    return 0;
}


//---------------------------------------------------------------------------
//
// SCALING - PreAnimator
//
//---------------------------------------------------------------------------

void ScaleSectorObject(SECTOR_OBJECT* sop)
{
    switch (sop->scale_type)
    {
    case SO_SCALE_NONE:
        break;

    case SO_SCALE_HOLD:
        break;

    // to dest
    case SO_SCALE_DEST:
        sop->scale_dist += sop->scale_speed;

        if (sop->scale_dist > sop->scale_dist_max)
        {
            sop->scale_dist = sop->scale_dist_max;
            sop->scale_type = SO_SCALE_HOLD;
        }
        else if (sop->scale_dist < sop->scale_dist_min)
        {
            sop->scale_dist = sop->scale_dist_min;
            sop->scale_type = SO_SCALE_HOLD;
        }

        break;

    // random direction change
    case SO_SCALE_RANDOM:

        sop->scale_dist += sop->scale_speed;
        if (sop->scale_dist > sop->scale_dist_max)
        {
            sop->scale_speed *= -1;
            sop->scale_dist = sop->scale_dist_max;
        }
        else if (sop->scale_dist < sop->scale_dist_min)
        {
            sop->scale_speed *= -1;
            sop->scale_dist = sop->scale_dist_min;
        }

        if (RANDOM_P2(1024) < sop->scale_rand_freq<<3)
        {
            sop->scale_speed *= -1;
        }

        break;

    // cycle through max and min
    case SO_SCALE_CYCLE:
        sop->scale_dist += sop->scale_speed;

        if (sop->scale_dist > sop->scale_dist_max)
        {
            sop->scale_speed *= -1;
            sop->scale_dist = sop->scale_dist_max;
        }
        else if (sop->scale_dist < sop->scale_dist_min)
        {
            sop->scale_speed *= -1;
            sop->scale_dist = sop->scale_dist_min;
        }
        break;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DVector2 ScaleRandomPoint(SECTOR_OBJECT* sop, int k, DAngle ang, const DVector2& pos)
{
    sop->scale_point_dist[k] += sop->scale_point_speed[k];
    if (sop->scale_point_dist[k] > sop->scale_point_dist_max)
    {
        sop->scale_point_speed[k] *= -1;
        sop->scale_point_dist[k] = sop->scale_point_dist_max;
    }
    else if (sop->scale_point_dist[k] < sop->scale_point_dist_min)
    {
        sop->scale_point_speed[k] *= -1;
        sop->scale_point_dist[k] = sop->scale_point_dist_min;
    }

    if (RANDOM_P2(1024) < sop->scale_point_rand_freq)
    {
        sop->scale_point_speed[k] *= -1;
    }

    // change up speed at random
    if (RANDOM_P2(1024) < (sop->scale_point_rand_freq / 2))
    {
        double half = sop->scale_point_base_speed / 2;
        double quart = sop->scale_point_base_speed / 4;
        sop->scale_point_speed[k] = sop->scale_point_base_speed + (RandomRangeF(half) - quart);
    }

    double xmul = (sop->scale_point_dist[k] * sop->scale_x_mult) * (1 / 256.);
    double ymul = (sop->scale_point_dist[k] * sop->scale_y_mult) * (1 / 256.);

    DVector2 ret;

    ret.X = pos.X + xmul * ang.Cos();
    ret.Y = pos.Y + ymul * ang.Sin();
    return ret;
}

//---------------------------------------------------------------------------
//
// Morph point - move point around
//
//---------------------------------------------------------------------------

void MorphTornado(SECTOR_OBJECT* sop)
{
    sectortype* *sectp;
    int j;

    // z direction
    ASSERT(sop->op_main_sector != nullptr);
    sop->morph_z += sop->morph_z_speed;

    // move vector
    if (sop->morph_wall_point == nullptr)
        return;

    // place at correct x,y offset from center
    DVector2 pos = sop->pmid - sop->morph_off;
    DVector2 spos = pos;

    // move it from last x,y
    DVector2 mpos = pos + sop->morph_ang.ToVector() * sop->morph_speed;

    // bound check radius
    if ((sop->pmid - mpos).Length() > sop->morph_dist_max + sop->scale_dist)
    {
        // find and reverse angle
        sop->morph_ang = VecToAngle(mpos - sop->pmid) + DAngle180;

        // move back some from last point
        mpos = spos + sop->morph_ang.ToVector() * sop->morph_speed * 2;
    }

    // save x,y back as offset info
    sop->morph_off = sop->pmid - mpos;

    if ((RANDOM_P2(1024<<4)>>4) < sop->morph_rand_freq)
        sop->morph_ang = RandomAngle();

    // move it x,y
    dragpoint(sop->morph_wall_point, mpos);

    // bound the Z
    double ceilz = sop->op_main_sector->ceilingz;
    double florz = sop->op_main_sector->floorz;

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if ((*sectp)->hasU() &&
            ((*sectp)->flags & SECTFU_SO_SLOPE_CEILING_TO_POINT))
        {
            if (sop->morph_z > florz)
            {
                sop->morph_z_speed *= -1;
                sop->morph_z = florz;
            }
            else if (sop->morph_z < ceilz)
            {
                sop->morph_z_speed *= -1;
                sop->morph_z = ceilz;
            }

            alignceilslope(*sectp, DVector3(mpos, sop->morph_z));
        }
    }
}

//---------------------------------------------------------------------------
//
// moves center point around and aligns slope
//
//---------------------------------------------------------------------------

void MorphFloor(SECTOR_OBJECT* sop)
{
    sectortype* *sectp;
    int j;

    // z direction
    ASSERT(sop->op_main_sector != nullptr);
    sop->morph_z -= sop->morph_z_speed;

    // move vector
    if (sop->morph_wall_point == nullptr)
        return;

    // place at correct x,y offset from center
    auto pos = sop->pmid - sop->morph_off;

    // move it from last x,y
    DVector2 mpos = pos + sop->morph_ang.ToVector() * sop->morph_speed;

    // save x,y back as offset info
    sop->morph_off = sop->pmid - mpos;

    // bound check radius
    if ((sop->pmid - mpos).Length() > sop->morph_dist_max)
    {
        // go in the other direction
        sop->morph_ang = sop->morph_ang + DAngle180;

        // back it up and save it off
        mpos = pos + sop->morph_ang.ToVector() * sop->morph_speed;
        sop->morph_off = sop->pmid - mpos;

        // turn it all the way around and then do a random -512 to 512 from there
        //sop->morph_ang = NORM_ANGLE(sop->morph_ang + 1024 + (RANDOM_P2(1024) - 512));
    }

    if ((RANDOM_P2(1024<<4)>>4) < sop->morph_rand_freq)
        sop->morph_ang = RandomAngle();

    // move x,y point "just like in build"
    dragpoint(sop->morph_wall_point, mpos);

    // bound the Z
    double florz = sop->op_main_sector->floorz;

    const double MORPH_FLOOR_ZRANGE = 300;

    if (sop->morph_z > MORPH_FLOOR_ZRANGE)
    {
        sop->morph_z = MORPH_FLOOR_ZRANGE;
        sop->morph_z_speed *= -1;
    }
    else if (sop->morph_z < -MORPH_FLOOR_ZRANGE)
    {
        sop->morph_z = -MORPH_FLOOR_ZRANGE;
        sop->morph_z_speed *= -1;
    }

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if ((*sectp)->hasU() &&
            ((*sectp)->flags & SECTFU_SO_SLOPE_CEILING_TO_POINT))
        {
            alignflorslope(*sectp, DVector3(mpos, florz + sop->morph_z));
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SOBJ_AlignFloorToPoint(SECTOR_OBJECT* sop, const DVector3& pos)
{
    sectortype* *sectp;
    int j;

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if ((*sectp)->hasU() &&
            ((*sectp)->flags & SECTFU_SO_SLOPE_CEILING_TO_POINT))
        {
            alignflorslope(*sectp, pos);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SOBJ_AlignCeilingToPoint(SECTOR_OBJECT* sop, const DVector3& pos)
{
    sectortype* *sectp;
    int j;

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if ((*sectp)->hasU() &&
            ((*sectp)->flags & SECTFU_SO_SLOPE_CEILING_TO_POINT))
        {
            alignceilslope(*sectp, pos);
        }
    }
}

//---------------------------------------------------------------------------
//
// moves center point around and aligns slope
//
//---------------------------------------------------------------------------

void SpikeFloor(SECTOR_OBJECT* sop)
{
    // z direction
    ASSERT(sop->op_main_sector != nullptr);
    sop->morph_z -= sop->morph_z_speed;

    // move vector
    if (sop->morph_wall_point == nullptr)
        return;

    // place at correct x,y offset from center
    auto pos = sop->pmid - sop->morph_off;

    // bound the Z

    const double MORPH_FLOOR_ZRANGE = 300;

    if (sop->morph_z > MORPH_FLOOR_ZRANGE)
    {
        sop->morph_z = MORPH_FLOOR_ZRANGE;
        sop->morph_z_speed *= -1;
    }
    else if (sop->morph_z < -MORPH_FLOOR_ZRANGE)
    {
        sop->morph_z = -MORPH_FLOOR_ZRANGE;
        sop->morph_z_speed *= -1;
    }
    pos.Z = sop->op_main_sector->floorz + sop->morph_z;

    SOBJ_AlignFloorToPoint(sop, pos);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

#include "saveable.h"

static saveable_code saveable_morph_code[] =
{
    SAVE_CODE(ScaleSectorObject),
    SAVE_CODE(MorphTornado),
    SAVE_CODE(MorphFloor),
    SAVE_CODE(SpikeFloor),
};

saveable_module saveable_morph =
{
    // code
    saveable_morph_code,
    SIZ(saveable_morph_code),

    // data
    nullptr,0
};
END_SW_NS
