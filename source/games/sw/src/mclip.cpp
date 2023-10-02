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
#include "game.h"
#include "tags.h"
#include "player.h"
#include "mclip.h"

BEGIN_SW_NS


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

Collision MultiClipMove(DSWPlayer* pp, double zz, double floordist)
{
    int i;
    DVector3 opos[MAX_CLIPBOX], pos[MAX_CLIPBOX];
    SECTOR_OBJECT* sop = pp->sop;
    short min_ndx = 0;
    double min_dist = 999999;
    double dist;

    int ret;
    Collision min_ret{};

    for (i = 0; i < sop->clipbox_num; i++)
    {
        // move the box to position instead of using offset- this prevents small rounding errors
        // allowing you to move through wall
        DAngle ang = (pp->GetActor()->spr.Angles.Yaw + sop->clipbox_ang[i]);
        DVector3 spos(pp->GetActor()->getPosWithOffsetZ(), zz);

        DVector2 vect = ang.ToVector() * sop->clipbox_vdist[i];
        Collision coll;

        clipmove(spos, &pp->cursector, vect, sop->clipbox_dist[i], 4., floordist, CLIPMASK_PLAYER, coll, 1);

        if (coll.type != kHitNone)
        {
            // hit something moving into start position
            min_dist = 0;
            min_ndx = i;
            // ox is where it should be
            opos[i].XY() = pp->GetActor()->getPosWithOffsetZ() + ang.ToVector() * sop->clipbox_vdist[i];

            // spos.x is where it hit
            pos[i].XY() = spos.XY();

            // see the dist moved
            dist = (pos[i].XY() - opos[i].XY()).Length();

            // save it off
            if (dist < min_dist)
            {
                min_dist = dist;
                min_ndx = i;
                min_ret = coll;
            }
        }
        else
        {
            // save off the start position
            opos[i] = pos[i] = spos;
            pos[i].Z = zz;

            // move the box
            clipmove(pos[i], &pp->cursector, pp->vect, sop->clipbox_dist[i], 4., floordist, CLIPMASK_PLAYER, coll);

            // save the dist moved
            dist = (pos[i].XY() - opos[i].XY()).Length();

            if (dist < min_dist)
            {
                min_dist = dist;
                min_ndx = i;
                min_ret = coll;
            }
        }
    }

    // put posx and y off from offset
    pp->GetActor()->spr.pos.XY() += pos[min_ndx].XY() - opos[min_ndx].XY();

    return min_ret;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int MultiClipTurn(DSWPlayer* pp, DAngle new_ang, double zz, double floordist)
{
    int i;
    SECTOR_OBJECT* sop = pp->sop;
    int ret;
    auto cursect = pp->cursector;

    for (i = 0; i < sop->clipbox_num; i++)
    {
        DAngle ang = new_ang + sop->clipbox_ang[i];

        DVector3 spos(pp->GetActor()->getPosWithOffsetZ(), zz);

        DVector2 vect = ang.ToVector() * sop->clipbox_vdist[i];
        Collision coll;

        clipmove(spos, &cursect, vect, sop->clipbox_dist[i], 4., floordist, CLIPMASK_PLAYER, coll);

        ASSERT(cursect);

        if (coll.type != kHitNone)
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int testquadinsect(int *point_num, DVector2 const * qp, sectortype* sect)
{
    int i,next_i;

    *point_num = -1;

    for (i=0; i < 4; i++)
    {
        if (!inside(qp[i].X, qp[i].Y, sect))
        {
            *point_num = i;

            return false;
        }
    }

    for (i=0; i<4; i++)
    {
        next_i = (i+1) & 3;
        if (!cansee(DVector3(qp[i], 0x3fffff), sect, DVector3(qp[next_i], 0x3fffff), sect))
        {
            return false;
        }
    }

    return true;
}


//---------------------------------------------------------------------------
//
//Ken gives the tank clippin' a try...
//
//---------------------------------------------------------------------------

int RectClipMove(DSWPlayer* pp, DVector2* qpos)
{
    int i;
    DVector2 xy[4];
    int point_num;
	DVector2 pvect = pp->vect;

    for (i = 0; i < 4; i++)
    {
        xy[i] = qpos[i] + pvect;
    }

    //Given the 4 points: x[4], y[4]
    if (testquadinsect(&point_num, xy, pp->cursector))
    {
        pp->GetActor()->spr.pos.XY() += pvect;
        return true;
    }

    if (point_num < 0)
        return false;

    if ((point_num == 0) || (point_num == 3))   //Left side bad - strafe right
    {
        for (i = 0; i < 4; i++)
        {
            xy[i].X = qpos[i].X - pvect.Y * 0.5;
            xy[i].Y = qpos[i].Y + pvect.X * 0.5;
        }
        if (testquadinsect(&point_num, xy, pp->cursector))
        {
            pp->GetActor()->spr.pos.XY() += { -pvect.X * 0.5, pvect.X * 0.5 };
        }

        return false;
    }

    if ((point_num == 1) || (point_num == 2))   //Right side bad - strafe left
    {
        for (i = 0; i < 4; i++)
        {
            xy[i].X = qpos[i].X + pvect.Y * 0.5;
            xy[i].Y = qpos[i].Y - pvect.X * 0.5;
        }
        if (testquadinsect(&point_num, xy, pp->cursector))
        {
            pp->GetActor()->spr.pos.XY() += { pvect.X * 0.5, -pvect.X * 0.5 };
        }

        return false;
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

short RectClipTurn(DSWPlayer* pp, DAngle new_angl, DVector2* qpos, DVector2* opos)
{
    int i;
    DVector2 xy[4];
    SECTOR_OBJECT* sop = pp->sop;
    DAngle rot_angl;
    int point_num;

    rot_angl = new_angl + sop->spin_ang - sop->ang_orig;
    for (i = 0; i < 4; i++)
    {
        xy[i] = rotatepoint(pp->GetActor()->spr.pos.XY(), opos[i], rot_angl);
        // cannot use sop->xmid and ymid because the SO is off the map at this point
    }

    //Given the 4 points: x[4], y[4]
    if (testquadinsect(&point_num, xy, pp->cursector))
    {
        // move to new pos
        for (i = 0; i < 4; i++)
        {
            qpos[i] = xy[i];
        }
        return true;
    }

    if (point_num < 0)
        return false;

    return false;
}
END_SW_NS
