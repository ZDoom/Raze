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

void ScaleSectorObject(SECTOR_OBJECTp);

short
DoSectorObjectSetScale(short match)
{
    SECTOR_OBJECTp sop;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (sop->xmid == MAXLONG)
            continue;

        if (sop->match_event == match)
        {
            SET(sop->flags, SOBJ_DYNAMIC);
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

short
DoSOevent(short match, short state)
{
    SECTOR_OBJECTp sop;
    SPRITEp me_sp;
    short vel_adj=0, spin_adj=0;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (sop->xmid == MAXLONG)
            continue;

        if (sop->match_event == match)
        {
            if (TEST(sop->flags, SOBJ_WAIT_FOR_EVENT))
            {
                if (sop->save_vel > 0 || sop->save_spin_speed > 0)
                {
                    RESET(sop->flags, SOBJ_WAIT_FOR_EVENT);
                    sop->vel = sop->save_vel;
                    sop->spin_speed = sop->save_spin_speed;
                }
            }

            if (sop->match_event_sprite == -1)
                continue;

            me_sp = &sprite[sop->match_event_sprite];

            // toggle
            if (state == -1)
            {
                if (TEST_BOOL3(me_sp))
                {
                    RESET_BOOL3(me_sp);
                    state = OFF;
                }
                else
                {
                    SET_BOOL3(me_sp);
                    state = ON;
                }
            }

            if (state == ON)
            {
                spin_adj = (int)SP_TAG3(me_sp);
                vel_adj = SP_TAG7(me_sp);
            }
            else if (state == OFF)
            {
                spin_adj = -(int)SP_TAG3(me_sp);
                vel_adj = -SP_TAG7(me_sp);
            }

            sop->spin_speed += spin_adj;

            if (TEST_BOOL1(me_sp))
                sop->vel_tgt += vel_adj;
            else
                sop->vel += vel_adj;

            if (TEST_BOOL2(me_sp))
            {
                sop->dir *= -1;
            }
        }
    }
    return 0;
}


//
// SCALING - PreAnimator
//

void ScaleSectorObject(SECTOR_OBJECTp sop)
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

void ScaleRandomPoint(SECTOR_OBJECTp sop, short k, short ang, int x, int y, int *dx, int *dy)
{
    int xmul,ymul;

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
    if (RANDOM_P2(1024) < (sop->scale_point_rand_freq/2))
    {
        //sop->scale_point_speed[k] = SCALE_POINT_SPEED;
        short half = sop->scale_point_base_speed/2;
        short quart = sop->scale_point_base_speed/4;
        sop->scale_point_speed[k] = sop->scale_point_base_speed + (RANDOM_RANGE(half) - quart);
    }

    xmul = (sop->scale_point_dist[k] * sop->scale_x_mult)>>8;
    ymul = (sop->scale_point_dist[k] * sop->scale_y_mult)>>8;

    *dx = x + ((xmul * sintable[NORM_ANGLE(ang+512)]) >> 14);
    *dy = y + ((ymul * sintable[ang]) >> 14);
}

//
// Morph point - move point around
//

void
MorphTornado(SECTOR_OBJECTp sop)
{
    int mx, my;
    int ceilingz;
    int floorz;
    SECTORp *sectp;
    int j;
    int x,y,sx,sy;

    // z direction
    ASSERT(sop->op_main_sector >= 0);
    sop->morph_z += Z(sop->morph_z_speed);

    // move vector
    if (sop->morph_wall_point < 0)
        return;

    // place at correct x,y offset from center
    x = sop->xmid - sop->morph_xoff;
    y = sop->ymid - sop->morph_yoff;

    sx = x;
    sy = y;

    // move it from last x,y
    mx = x + (((sop->morph_speed) * sintable[NORM_ANGLE(sop->morph_ang+512)]) >> 14);
    my = y + (((sop->morph_speed) * sintable[sop->morph_ang]) >> 14);

    // bound check radius
    if (ksqrt(SQ(sop->xmid - mx) + SQ(sop->ymid - my)) > sop->morph_dist_max + sop->scale_dist)
    {
        // find angle
        sop->morph_ang = NORM_ANGLE(getangle(mx - sop->xmid, my - sop->ymid));
        // reverse angle
        sop->morph_ang = NORM_ANGLE(sop->morph_ang + 1024);

        // move back some from last point
        mx = sx + (((sop->morph_speed*2) * sintable[NORM_ANGLE(sop->morph_ang+512)]) >> 14);
        my = sy + (((sop->morph_speed*2) * sintable[sop->morph_ang]) >> 14);

        sop->morph_xoff = sop->xmid - mx;
        sop->morph_yoff = sop->ymid - my;
    }

    // save x,y back as offset info
    sop->morph_xoff = sop->xmid - mx;
    sop->morph_yoff = sop->ymid - my;

    if ((RANDOM_P2(1024<<4)>>4) < sop->morph_rand_freq)
        sop->morph_ang = RANDOM_P2(2048);

    // move it x,y
    dragpoint(sop->morph_wall_point, mx, my, 0);

    // bound the Z
    ceilingz = sector[sop->op_main_sector].ceilingz;
    floorz = sector[sop->op_main_sector].floorz;

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if (SectUser[*sectp - sector] &&
            TEST(SectUser[*sectp - sector]->flags, SECTFU_SO_SLOPE_CEILING_TO_POINT))
        {
#define TOR_LOW (floorz)
            if (sop->morph_z > TOR_LOW)
            {
                sop->morph_z_speed *= -1;
                sop->morph_z = TOR_LOW;
            }
            else if (sop->morph_z < ceilingz)
            {
                sop->morph_z_speed *= -1;
                sop->morph_z = ceilingz;
            }

            alignceilslope(*sectp - sector, mx, my, sop->morph_z);
        }
    }
}

// moves center point around and aligns slope
void
MorphFloor(SECTOR_OBJECTp sop)
{
    int mx, my;
    int floorz;
    SECTORp *sectp;
    int j;
    int x,y;

    // z direction
    ASSERT(sop->op_main_sector >= 0);
    sop->morph_z -= Z(sop->morph_z_speed);

    // move vector
    if (sop->morph_wall_point < 0)
        return;

    // place at correct x,y offset from center
    x = sop->xmid - sop->morph_xoff;
    y = sop->ymid - sop->morph_yoff;

    // move it from last x,y
    mx = x + (((sop->morph_speed) * sintable[NORM_ANGLE(sop->morph_ang+512)]) >> 14);
    my = y + (((sop->morph_speed) * sintable[sop->morph_ang]) >> 14);

    // save x,y back as offset info
    sop->morph_xoff = sop->xmid - mx;
    sop->morph_yoff = sop->ymid - my;

    // bound check radius
    if (Distance(sop->xmid, sop->ymid, mx, my) > sop->morph_dist_max)
    {
        // go in the other direction
        //sop->morph_speed *= -1;
        sop->morph_ang = NORM_ANGLE(sop->morph_ang + 1024);

        // back it up and save it off
        mx = x + (((sop->morph_speed) * sintable[NORM_ANGLE(sop->morph_ang+512)]) >> 14);
        my = y + (((sop->morph_speed) * sintable[sop->morph_ang]) >> 14);
        sop->morph_xoff = sop->xmid - mx;
        sop->morph_yoff = sop->ymid - my;

        // turn it all the way around and then do a random -512 to 512 from there
        //sop->morph_ang = NORM_ANGLE(sop->morph_ang + 1024 + (RANDOM_P2(1024) - 512));
    }

    if ((RANDOM_P2(1024<<4)>>4) < sop->morph_rand_freq)
        sop->morph_ang = RANDOM_P2(2048);

    // move x,y point "just like in build"
    dragpoint(sop->morph_wall_point, mx, my, 0);

    // bound the Z
    floorz = sector[sop->op_main_sector].floorz;

#define MORPH_FLOOR_ZRANGE Z(300)

    if (sop->morph_z > MORPH_FLOOR_ZRANGE)
    {
        sop->morph_z = MORPH_FLOOR_ZRANGE;
        //sop->morph_ang = NORM_ANGLE(sop->morph_ang + 1024);
        sop->morph_z_speed *= -1;
    }
    else if (sop->morph_z < -MORPH_FLOOR_ZRANGE)
    {
        sop->morph_z = -MORPH_FLOOR_ZRANGE;
        //sop->morph_ang = NORM_ANGLE(sop->morph_ang + 1024);
        sop->morph_z_speed *= -1;
    }

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if (SectUser[*sectp - sector] &&
            TEST(SectUser[*sectp - sector]->flags, SECTFU_SO_SLOPE_CEILING_TO_POINT))
        {
            alignflorslope(*sectp - sector, mx, my, floorz + sop->morph_z);
        }
    }
}

void
SOBJ_AlignFloorToPoint(SECTOR_OBJECTp sop, int x, int y, int z)
{
    SECTORp *sectp;
    int j;

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if (SectUser[*sectp - sector] &&
            TEST(SectUser[*sectp - sector]->flags, SECTFU_SO_SLOPE_CEILING_TO_POINT))
        {
            alignflorslope(*sectp - sector, x, y, z);
        }
    }
}

void
SOBJ_AlignCeilingToPoint(SECTOR_OBJECTp sop, int x, int y, int z)
{
    SECTORp *sectp;
    int j;

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if (SectUser[*sectp - sector] &&
            TEST(SectUser[*sectp - sector]->flags, SECTFU_SO_SLOPE_CEILING_TO_POINT))
        {
            alignceilslope(*sectp - sector, x, y, z);
        }
    }
}

void
SOBJ_AlignFloorCeilingToPoint(SECTOR_OBJECTp sop, int x, int y, int z)
{
    SECTORp *sectp;
    int j;

    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if (SectUser[*sectp - sector] &&
            TEST(SectUser[*sectp - sector]->flags, SECTFU_SO_SLOPE_CEILING_TO_POINT))
        {
            alignflorslope(*sectp - sector, x, y, z);
            alignceilslope(*sectp - sector, x, y, z);
        }
    }
}

// moves center point around and aligns slope
void
SpikeFloor(SECTOR_OBJECTp sop)
{
    int mx, my;
    int floorz;
    SECTORp *sectp;
    int j;
    int x,y;

    // z direction
    ASSERT(sop->op_main_sector >= 0);
    sop->morph_z -= Z(sop->morph_z_speed);

    // move vector
    if (sop->morph_wall_point < 0)
        return;

    // place at correct x,y offset from center
    x = sop->xmid - sop->morph_xoff;
    y = sop->ymid - sop->morph_yoff;

    // move it from last x,y
    mx = x;
    my = y;

    // bound the Z
    floorz = sector[sop->op_main_sector].floorz;

#define MORPH_FLOOR_ZRANGE Z(300)

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

    SOBJ_AlignFloorToPoint(sop, mx, my, floorz + sop->morph_z);

#if 0
    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        if (SectUser[*sectp - sector] &&
            TEST(SectUser[*sectp - sector]->flags, SECTFU_SO_SLOPE_CEILING_TO_POINT))
        {
            alignflorslope(*sectp - sector, mx, my, floorz + sop->morph_z);
        }
    }
#endif
}


#include "saveable.h"

static saveable_code saveable_morph_code[] =
{
    SAVE_CODE(DoSectorObjectSetScale),
    SAVE_CODE(DoSOevent),
    SAVE_CODE(ScaleSectorObject),
    SAVE_CODE(ScaleRandomPoint),
    SAVE_CODE(MorphTornado),
    SAVE_CODE(MorphFloor),
    SAVE_CODE(SOBJ_AlignFloorToPoint),
    SAVE_CODE(SOBJ_AlignCeilingToPoint),
    SAVE_CODE(SOBJ_AlignFloorCeilingToPoint),
    SAVE_CODE(SpikeFloor),
};

saveable_module saveable_morph =
{
    // code
    saveable_morph_code,
    SIZ(saveable_morph_code),

    // data
    NULL,0
};
