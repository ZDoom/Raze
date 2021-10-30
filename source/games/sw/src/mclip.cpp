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

#include "mytypes.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "player.h"
#include "mclip.h"

BEGIN_SW_NS


int MultiClipMove(PLAYERp pp, int z, int floor_dist)
{
    int i;
    vec3_t opos[MAX_CLIPBOX], pos[MAX_CLIPBOX];
    SECTOR_OBJECTp sop = pp->sop;
    short ang;
    short min_ndx = 0;
    int min_dist = 999999;
    int dist;

    int ret_start;
    int ret;
    int min_ret=0;

    int xvect,yvect;

    for (i = 0; i < sop->clipbox_num; i++)
    {
        // move the box to position instead of using offset- this prevents small rounding errors
        // allowing you to move through wall
        ang = NORM_ANGLE(pp->angle.ang.asbuild() + sop->clipbox_ang[i]);

        vec3_t spos = { pp->posx, pp->posy, z };

        xvect = sop->clipbox_vdist[i] * bcos(ang);
        yvect = sop->clipbox_vdist[i] * bsin(ang);
        clipmoveboxtracenum = 1;
        ret_start = clipmove(&spos, &pp->cursectnum, xvect, yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);
        clipmoveboxtracenum = 3;

        if (ret_start)
        {
            // hit something moving into start position
            min_dist = 0;
            min_ndx = i;
            // ox is where it should be
            opos[i].x = pos[i].x = pp->posx + MulScale(sop->clipbox_vdist[i], bcos(ang), 14);
            opos[i].y = pos[i].y = pp->posy + MulScale(sop->clipbox_vdist[i], bsin(ang), 14);

            // spos.x is where it hit
            pos[i].x = spos.x;
            pos[i].y = spos.y;

            // see the dist moved
            dist = ksqrt(SQ(pos[i].x - opos[i].x) + SQ(pos[i].y - opos[i].y));

            // save it off
            if (dist < min_dist)
            {
                min_dist = dist;
                min_ndx = i;
                min_ret = ret_start;
            }
        }
        else
        {
            // save off the start position
            opos[i] = pos[i] = spos;
            pos[i].z = z;

            // move the box
            ret = clipmove(&pos[i], &pp->cursectnum, pp->xvect, pp->yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);

            // save the dist moved
            dist = ksqrt(SQ(pos[i].x - opos[i].x) + SQ(pos[i].y - opos[i].y));

            if (ret)
            {
            }

            if (dist < min_dist)
            {
                min_dist = dist;
                min_ndx = i;
                min_ret = ret;
            }
        }
    }

    // put posx and y off from offset
    pp->posx += pos[min_ndx].x - opos[min_ndx].x;
    pp->posy += pos[min_ndx].y - opos[min_ndx].y;

    return min_ret;
}

short MultiClipTurn(PLAYERp pp, short new_ang, int z, int floor_dist)
{
    int i;
    SECTOR_OBJECTp sop = pp->sop;
    int ret;
    int x,y;
    short ang;
    int xvect, yvect;
    short cursectnum = pp->cursectnum;

    for (i = 0; i < sop->clipbox_num; i++)
    {
        ang = NORM_ANGLE(new_ang + sop->clipbox_ang[i]);

        vec3_t pos = { pp->posx, pp->posy, z };

        xvect = sop->clipbox_vdist[i] * bcos(ang);
        yvect = sop->clipbox_vdist[i] * bsin(ang);

        // move the box
        ret = clipmove(&pos, &cursectnum, xvect, yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);

        ASSERT(cursectnum >= 0);

        if (ret)
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

int testquadinsect(int *point_num, vec2_t const * q, short sectnum)
{
    int i,next_i;

    *point_num = -1;

    for (i=0; i < 4; i++)
    {
        if (!inside(q[i].x, q[i].y, sectnum))
        {
            *point_num = i;

            return false;
        }
    }

    for (i=0; i<4; i++)
    {
        next_i = MOD4(i+1);
        if (!cansee(q[i].x, q[i].y,0x3fffffff, sectnum,
                    q[next_i].x, q[next_i].y,0x3fffffff, sectnum))
        {
            return false;
        }
    }

    return true;
}


//Ken gives the tank clippin' a try...
int RectClipMove(PLAYERp pp, int *qx, int *qy)
{
    int i;
    vec2_t xy[4];
    int point_num;

    for (i = 0; i < 4; i++)
    {
        xy[i].x = qx[i] + (pp->xvect>>14);
        xy[i].y = qy[i] + (pp->yvect>>14);
    }

    //Given the 4 points: x[4], y[4]
    if (testquadinsect(&point_num, xy, pp->cursectnum))
    {
        pp->posx += (pp->xvect>>14);
        pp->posy += (pp->yvect>>14);
        return true;
    }

    if (point_num < 0)
        return false;

    if ((point_num == 0) || (point_num == 3))   //Left side bad - strafe right
    {
        for (i = 0; i < 4; i++)
        {
            xy[i].x = qx[i] - (pp->yvect>>15);
            xy[i].y = qy[i] + (pp->xvect>>15);
        }
        if (testquadinsect(&point_num, xy, pp->cursectnum))
        {
            pp->posx -= (pp->yvect>>15);
            pp->posy += (pp->xvect>>15);
        }

        return false;
    }

    if ((point_num == 1) || (point_num == 2))   //Right side bad - strafe left
    {
        for (i = 0; i < 4; i++)
        {
            xy[i].x = qx[i] + (pp->yvect>>15);
            xy[i].y = qy[i] - (pp->xvect>>15);
        }
        if (testquadinsect(&point_num, xy, pp->cursectnum))
        {
            pp->posx += (pp->yvect>>15);
            pp->posy -= (pp->xvect>>15);
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

short RectClipTurn(PLAYERp pp, short new_ang, int *qx, int *qy, int *ox, int *oy)
{
    int i;
    vec2_t xy[4];
    SECTOR_OBJECTp sop = pp->sop;
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
    if (testquadinsect(&point_num, xy, pp->cursectnum))
    {
        // move to new pos
        for (i = 0; i < 4; i++)
        {
            qx[i] = xy[i].x;
            qy[i] = xy[i].y;
        }
        return true;
    }

    if (point_num < 0)
        return false;

    return false;
}
END_SW_NS
