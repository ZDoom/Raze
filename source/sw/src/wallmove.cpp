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

//#include "keys.h"
#include "names2.h"
//#include "panel.h"
#include "game.h"
#include "tags.h"
#include "common_game.h"
#include "weapon.h"
#include "sprite.h"

//#include "ai.h"

SECTOR_OBJECTp DetectSectorObjectByWall(WALLp);

void SOwallmove(SECTOR_OBJECTp sop, SPRITEp sp, WALLp find_wallp, int dist, int *nx, int *ny)
{
    int j,k,wallcount;
    WALLp wp;
    short startwall,endwall;
    SECTORp *sectp;

    if (TEST(sop->flags, SOBJ_SPRITE_OBJ))
        return;

    wallcount = 0;
    for (sectp = sop->sectp, j = 0; *sectp; sectp++, j++)
    {
        startwall = (*sectp)->wallptr;
        endwall = startwall + (*sectp)->wallnum - 1;

        // move all walls in sectors back to the original position
        for (wp = &wall[startwall], k = startwall; k <= endwall; wp++, k++)
        {
            // find the one wall we want to adjust
            if (wp == find_wallp)
            {
                short ang;
                // move orig x and y in saved angle
                ASSERT(User[sp - sprite]);
                ang = User[sp - sprite]->sang;

                *nx = ((dist * sintable[NORM_ANGLE(ang + 512)])>>14);
                *ny = ((dist * sintable[ang])>>14);

                sop->xorig[wallcount] -= *nx;
                sop->yorig[wallcount] -= *ny;

                SET(sop->flags, SOBJ_UPDATE_ONCE);
                return;
            }

            wallcount++;
        }
    }
}

int DoWallMove(SPRITEp sp)
{
    int dist,nx,ny;
    short shade1,shade2,ang,picnum1,picnum2;
    WALLp wallp;
    short prev_wall;
    SWBOOL found = FALSE;
    short dang;
    SWBOOL SOsprite = FALSE;

    dist = SP_TAG13(sp);
    ang = SP_TAG4(sp);
    picnum1 = SP_TAG5(sp);
    picnum2 = SP_TAG6(sp);
    shade1 = SP_TAG7(sp);
    shade2 = SP_TAG8(sp);
    dang = ((int)SP_TAG10(sp)) << 3;

    if (dang)
        ang = NORM_ANGLE(ang + (RANDOM_RANGE(dang) - dang/2));

    nx = (dist * sintable[NORM_ANGLE(ang + 512)])>>14;
    ny = (dist * sintable[ang])>>14;

    for (wallp = wall; wallp < &wall[numwalls]; wallp++)
    {
        if (wallp->x == sp->x && wallp->y == sp->y)
        {
            found = TRUE;

            if (TEST(wallp->extra, WALLFX_SECTOR_OBJECT))
            {
                SECTOR_OBJECTp sop;
                sop = DetectSectorObjectByWall(wallp);
                ASSERT(sop);
                SOwallmove(sop, sp, wallp, dist, &nx, &ny);

                SOsprite = TRUE;
            }
            else
            {
                wallp->x = sp->x + nx;
                wallp->y = sp->y + ny;
            }

            if (shade1)
                wallp->shade = shade1;
            if (picnum1)
                wallp->picnum = picnum1;

            // find the previous wall
            prev_wall = PrevWall(wallp - wall);
            if (shade2)
                wall[prev_wall].shade = shade2;
            if (picnum2)
                wall[prev_wall].picnum = picnum2;
        }
    }

    SP_TAG9(sp)--;
    if ((signed char)SP_TAG9(sp) <= 0)
    {
        KillSprite(sp - sprite);
    }
    else
    {
        if (SOsprite)
        {
            // move the sprite offset from center
            User[sp - sprite]->sx -= nx;
            User[sp - sprite]->sy -= ny;
        }
        else
        {
            sp->x += nx;
            sp->y += ny;
        }
    }

    return found;
}

SWBOOL CanSeeWallMove(SPRITEp wp, short match)
{
    short i,nexti;
    SWBOOL found = FALSE;
    SPRITEp sp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_WALL_MOVE_CANSEE], i, nexti)
    {
        sp = &sprite[i];

        if (SP_TAG2(sp) == match)
        {
            found = TRUE;

            if (cansee(wp->x,wp->y,wp->z,wp->sectnum,sp->x,sp->y,sp->z,sp->sectnum))
            {
                return TRUE;
            }
        }
    }

    if (found)
        return FALSE;
    else
        return TRUE;
}

int DoWallMoveMatch(short match)
{
    SPRITEp sp;
    short i,nexti;
    SWBOOL found = FALSE;

    // just all with the same matching tags
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_WALL_MOVE], i, nexti)
    {
        sp = &sprite[i];

        if (SP_TAG2(sp) == match)
        {
            found = TRUE;
            DoWallMove(sp);
        }
    }

    return found;
}


#include "saveable.h"

static saveable_code saveable_wallmove_code[] =
{
    SAVE_CODE(DoWallMove),
    SAVE_CODE(CanSeeWallMove),
    SAVE_CODE(DoWallMoveMatch),
};

saveable_module saveable_wallmove =
{
    // code
    saveable_wallmove_code,
    SIZ(saveable_wallmove_code),

    // data
    NULL,0
};
