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


Collision MultiClipMove(PLAYER* pp, int z, int floor_dist)
{
    int i;
    vec3_t opos[MAX_CLIPBOX], pos[MAX_CLIPBOX];
    SECTOR_OBJECT* sop = pp->sop;
    short ang;
    short min_ndx = 0;
    int min_dist = 999999;
    int dist;

    int ret;
    Collision min_ret{};

    int xvect,yvect;

    for (i = 0; i < sop->clipbox_num; i++)
    {
        // move the box to position instead of using offset- this prevents small rounding errors
        // allowing you to move through wall
        ang = NORM_ANGLE(pp->angle.ang.asbuild() + sop->clipbox_ang[i]);

        vec3_t spos = { pp->pos.X, pp->pos.Y, z };

        xvect = sop->clipbox_vdist[i] * bcos(ang);
        yvect = sop->clipbox_vdist[i] * bsin(ang);
        Collision coll;
        clipmove(spos, &pp->cursector, xvect, yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER, coll, 1);

        if (coll.type != kHitNone)
        {
            // hit something moving into start position
            min_dist = 0;
            min_ndx = i;
            // ox is where it should be
            opos[i].X = pos[i].X = pp->pos.X + MulScale(sop->clipbox_vdist[i], bcos(ang), 14);
            opos[i].Y = pos[i].Y = pp->pos.Y + MulScale(sop->clipbox_vdist[i], bsin(ang), 14);

            // spos.x is where it hit
            pos[i].X = spos.X;
            pos[i].Y = spos.Y;

            // see the dist moved
            dist = ksqrt(SQ(pos[i].X - opos[i].X) + SQ(pos[i].Y - opos[i].Y));

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
            pos[i].Z = z;

            // move the box
            clipmove(pos[i], &pp->cursector, pp->vect.X, pp->vect.Y, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER, coll);

            // save the dist moved
            dist = ksqrt(SQ(pos[i].X - opos[i].X) + SQ(pos[i].Y - opos[i].Y));

            if (dist < min_dist)
            {
                min_dist = dist;
                min_ndx = i;
                min_ret = coll;
            }
        }
    }

    // put posx and y off from offset
    pp->pos.X += pos[min_ndx].X - opos[min_ndx].X;
    pp->pos.Y += pos[min_ndx].Y - opos[min_ndx].Y;

    return min_ret;
}

short MultiClipTurn(PLAYER* pp, short new_ang, int z, int floor_dist)
{
    int i;
    SECTOR_OBJECT* sop = pp->sop;
    int ret;
    int x,y;
    short ang;
    int xvect, yvect;
    auto cursect = pp->cursector;

    for (i = 0; i < sop->clipbox_num; i++)
    {
        ang = NORM_ANGLE(new_ang + sop->clipbox_ang[i]);

        vec3_t pos = { pp->pos.X, pp->pos.Y, z };

        xvect = sop->clipbox_vdist[i] * bcos(ang);
        yvect = sop->clipbox_vdist[i] * bsin(ang);

        // move the box
        Collision coll;
        clipmove(pos, &cursect, xvect, yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER, coll);

        ASSERT(cursect);

        if (coll.type != kHitNone)
        {
            // attempt to move a bit when turning against a wall
            //ang = NORM_ANGLE(ang + 1024);
            //pp->xvect += 20 * bcos(ang);
            //pp->yvect += 20 * bsin(ang);
            return false;
        }
    }

    return true;
}

int testquadinsect(int *point_num, vec2_t const * q, sectortype* sect)
{
    int i,next_i;

    *point_num = -1;

    for (i=0; i < 4; i++)
    {
        if (!inside(q[i].X, q[i].Y, sect))
        {
            *point_num = i;

            return false;
        }
    }

    for (i=0; i<4; i++)
    {
        next_i = (i+1) & 3;
        if (!cansee(q[i].X, q[i].Y,0x3fffffff, sect,
                    q[next_i].X, q[next_i].Y,0x3fffffff, sect))
        {
            return false;
        }
    }

    return true;
}


//Ken gives the tank clippin' a try...
int RectClipMove(PLAYER* pp, int *qx, int *qy)
{
    int i;
    vec2_t xy[4];
    int point_num;

    for (i = 0; i < 4; i++)
    {
        xy[i].X = qx[i] + (pp->vect.X>>14);
        xy[i].Y = qy[i] + (pp->vect.Y>>14);
    }

    //Given the 4 points: x[4], y[4]
    if (testquadinsect(&point_num, xy, pp->cursector))
    {
        pp->pos.X += (pp->vect.X>>14);
        pp->pos.Y += (pp->vect.Y>>14);
        return true;
    }

    if (point_num < 0)
        return false;

    if ((point_num == 0) || (point_num == 3))   //Left side bad - strafe right
    {
        for (i = 0; i < 4; i++)
        {
            xy[i].X = qx[i] - (pp->vect.Y>>15);
            xy[i].Y = qy[i] + (pp->vect.X>>15);
        }
        if (testquadinsect(&point_num, xy, pp->cursector))
        {
            pp->pos.X -= (pp->vect.Y>>15);
            pp->pos.Y += (pp->vect.X>>15);
        }

        return false;
    }

    if ((point_num == 1) || (point_num == 2))   //Right side bad - strafe left
    {
        for (i = 0; i < 4; i++)
        {
            xy[i].X = qx[i] + (pp->vect.Y>>15);
            xy[i].Y = qy[i] - (pp->vect.X>>15);
        }
        if (testquadinsect(&point_num, xy, pp->cursector))
        {
            pp->pos.X += (pp->vect.Y>>15);
            pp->pos.Y -= (pp->vect.X>>15);
        }

        return false;
    }

    return false;
}

int testpointinquad(int x, int y, int *qx, int *qy)
{
    int i, cnt, x1, y1, x2, y2;

    cnt = 0;
    for (i=0; i<4; i++)
    {
        y1 = qy[i]-y;
        y2 = qy[(i+1)&3]-y;
        if ((y1^y2) >= 0) continue;

        x1 = qx[i]-x;
        x2 = qx[(i+1)&3]-x;
        if ((x1^x2) >= 0)
            cnt ^= x1;
        else
            cnt ^= (x1*y2-x2*y1)^y2;
    }
    return cnt>>31;
}

short RectClipTurn(PLAYER* pp, short new_ang, int *qx, int *qy, int *ox, int *oy)
{
    int i;
    vec2_t xy[4];
    SECTOR_OBJECT* sop = pp->sop;
    short rot_ang;
    int point_num;

    rot_ang = NORM_ANGLE(new_ang + sop->spin_ang - sop->ang_orig);
    for (i = 0; i < 4; i++)
    {
        vec2_t const p = { ox[i], oy[i] };
        rotatepoint(pp->pos.vec2, p, rot_ang, &xy[i]);
        // cannot use sop->xmid and ymid because the SO is off the map at this point
        //rotatepoint(&sop->xmid, p, rot_ang, &xy[i]);
    }

    //Given the 4 points: x[4], y[4]
    if (testquadinsect(&point_num, xy, pp->cursector))
    {
        // move to new pos
        for (i = 0; i < 4; i++)
        {
            qx[i] = xy[i].X;
            qy[i] = xy[i].Y;
        }
        return true;
    }

    if (point_num < 0)
        return false;

    return false;
}
END_SW_NS
