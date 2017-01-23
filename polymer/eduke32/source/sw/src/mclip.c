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

#include "mytypes.h"
#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "player.h"
#include "mclip.h"

#if 0
int MultiClipMove(PLAYERp pp, int z, int floor_dist)
{
    int i;
    int ox[MAX_CLIPBOX],oy[MAX_CLIPBOX];
    SPRITEp sp = pp->sop->sp_child;
    USERp u = User[sp - sprite];
    SECTOR_OBJECTp sop = pp->sop;
    short ang;
    short min_ndx;
    int min_dist = 999999;
    int dist;

    int ret_start;
    int ret[MAX_CLIPBOX];
    int x[MAX_CLIPBOX],y[MAX_CLIPBOX];

    for (i = 0; i < sop->clipbox_num; i++)
    {
        ang = NORM_ANGLE(pp->pang + sop->clipbox_ang[i]);
        ox[i] = x[i] = pp->posx + (sop->clipbox_vdist[i] * sintable[NORM_ANGLE(ang + 512)] >> 14);
        oy[i] = y[i] = pp->posy + (sop->clipbox_vdist[i] * sintable[ang] >> 14);

        // move the box
        ret[i] = clipmove_old(&x[i], &y[i], &z, &pp->cursectnum, pp->xvect, pp->yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);

        // save the dist moved
        dist = FindDistance2D(x[i] - ox[i], y[i] - oy[i]);

        if (dist < min_dist)
        {
            min_dist = dist;
            min_ndx = i;
        }
    }

    // put posx and y off from offset
    pp->posx -= ox[min_ndx] - x[min_ndx];
    pp->posy -= oy[min_ndx] - y[min_ndx];

    return ret[min_ndx];
}
#endif

#if 0
int MultiClipMove(PLAYERp pp, int z, int floor_dist)
{
    int i;
    int ox[MAX_CLIPBOX],oy[MAX_CLIPBOX];
    SPRITEp sp = pp->sop->sp_child;
    USERp u = User[sp - sprite];
    SECTOR_OBJECTp sop = pp->sop;
    short ang;
    short min_ndx;
    int min_dist = 999999;
    int dist;

    int ret_start;
    int ret[MAX_CLIPBOX];
    int x[MAX_CLIPBOX],y[MAX_CLIPBOX];

    for (i = 0; i < sop->clipbox_num; i++)
    {
        ang = NORM_ANGLE(pp->pang + sop->clipbox_ang[i]);
        ox[i] = x[i] = pp->posx + (sop->clipbox_vdist[i] * sintable[NORM_ANGLE(ang + 512)] >> 14);
        oy[i] = y[i] = pp->posy + (sop->clipbox_vdist[i] * sintable[ang] >> 14);

        // move the box
        //pushmove_old(&x[i], &y[i], &z, &pp->cursectnum, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);
        ret[i] = clipmove_old(&x[i], &y[i], &z, &pp->cursectnum, pp->xvect, pp->yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);
        //pushmove_old(&x[i], &y[i], &z, &pp->cursectnum, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);

        // save the dist moved
        dist = FindDistance2D(x[i] - ox[i], y[i] - oy[i]);

        if (dist < min_dist)
        {
            min_dist = dist;
            min_ndx = i;
        }
    }

    // put posx and y off from offset
    pp->posx -= ox[min_ndx] - x[min_ndx];
    pp->posy -= oy[min_ndx] - y[min_ndx];

    return ret[min_ndx];
}
#endif

#if 0
int MultiClipMove(PLAYERp pp, int z, int floor_dist)
{
    int i;
    int ox[MAX_CLIPBOX],oy[MAX_CLIPBOX];
    SPRITEp sp = pp->sop->sp_child;
    USERp u = User[sp - sprite];
    SECTOR_OBJECTp sop = pp->sop;
    short ang;
    short min_ndx;
    int min_dist = 999999;
    int dist;

    int ret_start;
    int ret[MAX_CLIPBOX];
    int x[MAX_CLIPBOX],y[MAX_CLIPBOX];

    int xvect,yvect;
    int xs,ys;

    for (i = 0; i < sop->clipbox_num; i++)
    {
        // move the box to position instead of using offset- this prevents small rounding errors
        // allowing you to move through wall
        ang = NORM_ANGLE(pp->pang + sop->clipbox_ang[i]);

        xs = pp->posx;
        ys = pp->posy;

        xvect = (sop->clipbox_vdist[i] * sintable[NORM_ANGLE(ang + 512)]);
        yvect = (sop->clipbox_vdist[i] * sintable[ang]);
        ret_start = clipmove_old(&xs, &ys, &z, &pp->cursectnum, xvect, yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);

        // save off the start position
        ox[i] = x[i] = xs;
        oy[i] = y[i] = ys;

        // move the box
        ret[i] = clipmove_old(&x[i], &y[i], &z, &pp->cursectnum, pp->xvect, pp->yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);

        // save the dist moved
        dist = ksqrt(SQ(x[i] - ox[i]) + SQ(y[i] - oy[i]));
        //dist = FindDistance2D(x[i] - ox[i], y[i] - oy[i]);

        if (ret[i])
        {
            //DSPRINTF(ds,"i %d, ret %d, dist %d",i, ret[i], dist);
            //printf("%s\n",ds);
            MONO_PRINT(ds);
        }

        if (dist < min_dist)
        {
            min_dist = dist;
            min_ndx = i;
        }
    }

    // put posx and y off from offset
    pp->posx -= ox[min_ndx] - x[min_ndx];
    pp->posy -= oy[min_ndx] - y[min_ndx];

    return ret[min_ndx];
}
#endif

#if 0
int MultiClipMove(PLAYERp pp, int z, int floor_dist)
{
    int i;
    int ox[MAX_CLIPBOX],oy[MAX_CLIPBOX];
    SPRITEp sp = pp->sop->sp_child;
    USERp u = User[sp - sprite];
    SECTOR_OBJECTp sop = pp->sop;
    short ang;
    short min_ndx;
    int min_dist = 999999;
    int dist;

    int ret_start;
    int ret[MAX_CLIPBOX];
    int x[MAX_CLIPBOX],y[MAX_CLIPBOX];

    int xvect,yvect;
    int xs,ys;

    for (i = 0; i < sop->clipbox_num; i++)
    {
        // move the box to position instead of using offset- this prevents small rounding errors
        // allowing you to move through wall
        ang = NORM_ANGLE(pp->pang + sop->clipbox_ang[i]);

        xs = pp->posx;
        ys = pp->posy;

        xvect = (sop->clipbox_vdist[i] * sintable[NORM_ANGLE(ang + 512)]);
        yvect = (sop->clipbox_vdist[i] * sintable[ang]);
        ret_start = clipmove_old(&xs, &ys, &z, &pp->cursectnum, xvect, yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);

        if (ret_start)
        {
            // hit something moving into position
            min_dist = 0;
            min_ndx = i;
            // ox is where it should be
            ox[i] = x[i] = pp->posx + (sop->clipbox_vdist[i] * sintable[NORM_ANGLE(ang + 512)] >> 14);
            oy[i] = y[i] = pp->posy + (sop->clipbox_vdist[i] * sintable[ang] >> 14);

            x[i] = xs;
            y[i] = ys;

            dist = ksqrt(SQ(x[i] - ox[i]) + SQ(y[i] - oy[i]));
            //ox[i] = x[i] = oy[i] = y[i] = 0;
        }
        else
        {
            // save off the start position
            ox[i] = x[i] = xs;
            oy[i] = y[i] = ys;

            // move the box
            ret[i] = clipmove_old(&x[i], &y[i], &z, &pp->cursectnum, pp->xvect, pp->yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);

            // save the dist moved
            dist = ksqrt(SQ(x[i] - ox[i]) + SQ(y[i] - oy[i]));
            //dist = FindDistance2D(x[i] - ox[i], y[i] - oy[i]);

            if (ret[i])
            {
                //DSPRINTF(ds,"i %d, ret %d, dist %d",i, ret[i], dist);
                MONO_PRINT(ds);
            }

            if (dist < min_dist)
            {
                min_dist = dist;
                min_ndx = i;
            }
        }
    }

    // put posx and y off from offset
    pp->posx -= ox[min_ndx] - x[min_ndx];
    pp->posy -= oy[min_ndx] - y[min_ndx];

    return ret[min_ndx];
}
#endif

int MultiClipMove(PLAYERp pp, int z, int floor_dist)
{
    int i;
    int ox[MAX_CLIPBOX],oy[MAX_CLIPBOX];
    SPRITEp sp = pp->sop->sp_child;
    USERp u = User[sp - sprite];
    SECTOR_OBJECTp sop = pp->sop;
    short ang;
    short min_ndx = 0;
    int min_dist = 999999;
    int dist;

    int ret_start;
    int ret;
    int min_ret=0;
    int x[MAX_CLIPBOX],y[MAX_CLIPBOX];

    int xvect,yvect;
    int xs,ys;

    for (i = 0; i < sop->clipbox_num; i++)
    {
        // move the box to position instead of using offset- this prevents small rounding errors
        // allowing you to move through wall
        ang = NORM_ANGLE(pp->pang + sop->clipbox_ang[i]);

        xs = pp->posx;
        ys = pp->posy;

        xvect = (sop->clipbox_vdist[i] * sintable[NORM_ANGLE(ang + 512)]);
        yvect = (sop->clipbox_vdist[i] * sintable[ang]);
        clipmoveboxtracenum = 1;
        ret_start = clipmove_old(&xs, &ys, &z, &pp->cursectnum, xvect, yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);
        clipmoveboxtracenum = 3;

        if (ret_start)
        {
            // hit something moving into start position
            min_dist = 0;
            min_ndx = i;
            // ox is where it should be
            ox[i] = x[i] = pp->posx + (sop->clipbox_vdist[i] * sintable[NORM_ANGLE(ang + 512)] >> 14);
            oy[i] = y[i] = pp->posy + (sop->clipbox_vdist[i] * sintable[ang] >> 14);

            // xs is where it hit
            x[i] = xs;
            y[i] = ys;

            // see the dist moved
            dist = ksqrt(SQ(x[i] - ox[i]) + SQ(y[i] - oy[i]));

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
            ox[i] = x[i] = xs;
            oy[i] = y[i] = ys;

            // move the box
            ret = clipmove_old(&x[i], &y[i], &z, &pp->cursectnum, pp->xvect, pp->yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);

            // save the dist moved
            dist = ksqrt(SQ(x[i] - ox[i]) + SQ(y[i] - oy[i]));

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
    pp->posx += x[min_ndx] - ox[min_ndx];
    pp->posy += y[min_ndx] - oy[min_ndx];

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

        x = pp->posx;
        y = pp->posy;

        xvect = (sop->clipbox_vdist[i] * sintable[NORM_ANGLE(ang + 512)]);
        yvect = (sop->clipbox_vdist[i] * sintable[ang]);

        // move the box
        ret = clipmove_old(&x, &y, &z, &cursectnum, xvect, yvect, (int)sop->clipbox_dist[i], Z(4), floor_dist, CLIPMASK_PLAYER);

        ASSERT(cursectnum >= 0);

        if (ret)
        {
            // attempt to move a bit when turning against a wall
            //ang = NORM_ANGLE(ang + 1024);
            //pp->xvect += 20 * sintable[NORM_ANGLE(ang + 512)];
            //pp->yvect += 20 * sintable[ang];
            return FALSE;
        }
    }

    return TRUE;
}

int testquadinsect(int *point_num, vec2_t const * q, short sectnum)
{
    int i,next_i;

    *point_num = -1;

    for (i=0; i < 4; i++)
    {
        if (!inside(q[i].x, q[i].y, sectnum))
        {
            ////DSPRINTF(ds,"inside %ld failed",i);
            //MONO_PRINT(ds);

            *point_num = i;

            return FALSE;
        }
    }

    for (i=0; i<4; i++)
    {
        next_i = MOD4(i+1);
        if (!cansee(q[i].x, q[i].y,0x3fffffff, sectnum,
                    q[next_i].x, q[next_i].y,0x3fffffff, sectnum))
        {
            DSPRINTF(ds,"cansee %ld failed, x1 %d, y1 %d, x2 %d, y2 %d, sectnum %d",i, q[i].x, q[i].y, q[next_i].x, q[next_i].y, sectnum);
            MONO_PRINT(ds);

            return FALSE;
        }
    }

    return TRUE;
}


//Ken gives the tank clippin' a try...
int RectClipMove(PLAYERp pp, int *qx, int *qy)
{
    SECTORp *sectp;
    SECTOR_OBJECTp sop = pp->sop;
    WALLp wp;
    int count=0;
    int i;
    vec2_t xy[4];
    short startwall,endwall;
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
        return TRUE;
    }

    if (point_num < 0)
        return FALSE;

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

        return FALSE;
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

        return FALSE;
    }

    return FALSE;
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
    int ret;
    short ang;
    short rot_ang;
    int point_num;

    rot_ang = NORM_ANGLE(new_ang + sop->spin_ang - sop->ang_orig);
    for (i = 0; i < 4; i++)
    {
        vec2_t const p = { ox[i], oy[i] };
        rotatepoint(*(vec2_t *)&pp->posx, p, rot_ang, &xy[i]);
        // cannot use sop->xmid and ymid because the SO is off the map at this point
        //rotatepoint(*(vec2_t *)&sop->xmid, p, rot_ang, &xy[i]);
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
        return TRUE;
    }

    if (point_num < 0)
        return FALSE;

    return FALSE;
}
