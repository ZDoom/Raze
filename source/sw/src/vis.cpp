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
#include "game.h"
#include "tags.h"
#include "common_game.h"
#include "break.h"
#include "quake.h"
#include "pal.h"
#include "sprite.h"

extern short NormalVisibility;  // player.c
extern SWBOOL GamePaused;

#define VIS_VisCur(sp) (SP_TAG2(sp))
#define VIS_VisDir(sp) (SP_TAG3(sp))
#define VIS_VisGoal(sp) (SP_TAG4(sp))

void ProcessVisOn(void)
{
    short i, nexti;
    SPRITEp sp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_VIS_ON], i, nexti)
    {
        sp = &sprite[i];

        if (VIS_VisDir(sp))
        {
            // get brighter
            VIS_VisCur(sp) >>= 1;
            //VIS_VisCur(sp) -= 16;
            if (VIS_VisCur(sp) <= VIS_VisGoal(sp))
            {
                VIS_VisCur(sp) = VIS_VisGoal(sp);
                VIS_VisDir(sp) ^= 1;
            }
        }
        else
        {
            // get darker
            VIS_VisCur(sp) <<= 1;
            VIS_VisCur(sp) += 1;
            //VIS_VisCur(sp) += 16;
            if (VIS_VisCur(sp) >= NormalVisibility)
            {
                VIS_VisCur(sp) = NormalVisibility;
                if (sp->owner >= 0)
                {
                    ASSERT(User[sp->owner]);
                    RESET(User[sp->owner]->Flags2, SPR2_VIS_SHADING);
                }
                KillSprite(i);
            }
        }
    }
}

void VisViewChange(PLAYERp pp, int *vis)
{
    short i, nexti;
    SPRITEp sp;
    short BrightestVis = NormalVisibility;
    int x,y,z;
    short sectnum;

    if (GamePaused)
        return;

    // find the closest quake - should be a strength value
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_VIS_ON], i, nexti)
    {
        sp = &sprite[i];

        if (sp->owner >= 0)
        {
            x = sprite[sp->owner].x;
            y = sprite[sp->owner].y;
            z = sprite[sp->owner].z;
            sectnum = sprite[sp->owner].sectnum;
        }
        else
        {
            x = sp->x;
            y = sp->y;
            z = sp->z;
            sectnum = sp->sectnum;
        }

        // save off the brightest vis that you can see
        if (FAFcansee(pp->posx, pp->posy, pp->posz, pp->cursectnum, x, y, z, sectnum))
        {
            if (VIS_VisCur(sp) < BrightestVis)
                BrightestVis = VIS_VisCur(sp);
        }
    }

    *vis = BrightestVis;
}

int SpawnVis(short Parent, short sectnum, int x, int y, int z, int amt)
{
    short SpriteNum;
    SPRITEp sp;
    short i,nexti;

    if (Parent >= 0)
    {
        if (sector[sprite[Parent].sectnum].floorpal == PALETTE_FOG)
            return -1;

        if (sector[sprite[Parent].sectnum].floorpal == PALETTE_DIVE_LAVA)
            return -1;

        // kill any others with the same parent
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_VIS_ON], i, nexti)
        {
            sp = &sprite[i];
            if (sp->owner == Parent)
            {
                KillSprite(i);
            }
        }

        SpriteNum = COVERinsertsprite(sprite[Parent].sectnum, STAT_VIS_ON);
        sp = &sprite[SpriteNum];

        sp->owner = Parent;

        ASSERT(User[Parent]);
        SET(User[Parent]->Flags2, SPR2_CHILDREN);

        sp->x = sprite[Parent].x;
        sp->y = sprite[Parent].y;
        sp->z = sprite[Parent].z;

        SET(User[Parent]->Flags2, SPR2_VIS_SHADING);
    }
    else
    {
        if (sector[sectnum].floorpal == PALETTE_FOG)
            return -1;

        SpriteNum = COVERinsertsprite(sectnum, STAT_VIS_ON);
        sp = &sprite[SpriteNum];

        sp->x = x;
        sp->y = y;
        sp->z = z - Z(20);
        sp->owner = -1;
    }

    sp->cstat = 0;
    sp->extra = 0;

    VIS_VisDir(sp) = 1;
    VIS_VisCur(sp) = NormalVisibility;
    VIS_VisGoal(sp) = amt;

    return SpriteNum;
}

