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

#include "gamecontrol.h"

#include "names2.h"
#include "game.h"
#include "tags.h"
#include "break.h"
#include "pal.h"
#include "sprite.h"

BEGIN_SW_NS

extern short NormalVisibility;  // player.c

#define VIS_VisCur(sp) (SP_TAG2(sp))
#define VIS_VisDir(sp) (SP_TAG3(sp))
#define VIS_VisGoal(sp) (SP_TAG4(sp))

void ProcessVisOn(void)
{
    SPRITEp sp;

    SWStatIterator it(STAT_VIS_ON);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

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
                auto own = GetOwner(actor);
                if (own != nullptr)
                {
                    ASSERT(own->hasU());
                    RESET(own->u()->Flags2, SPR2_VIS_SHADING);
                }
                KillActor(actor);
            }
        }
    }
}

void VisViewChange(PLAYERp pp, int *vis)
{
    SPRITEp sp;
    short BrightestVis = NormalVisibility;
    int x,y,z;
    short sectnum;

    if (paused)
        return;

    // find the closest quake - should be a strength value
    SWStatIterator it(STAT_VIS_ON);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        auto own = GetOwner(actor);
        if (own != nullptr)
        {
            x = own->s().x;
            y = own->s().y;
            z = own->s().z;
            sectnum = own->s().sectnum;
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

void SpawnVis(DSWActor* parentActor, short sectnum, int x, int y, int z, int amt)
{
    SPRITEp sp;
    if (parentActor != nullptr)
    {
        auto psp = &parentActor->s();
        auto pu = parentActor->u();

        if (sector[psp->sectnum].floorpal == PALETTE_FOG)
            return;

        if (sector[psp->sectnum].floorpal == PALETTE_DIVE_LAVA)
            return;

        // kill any others with the same parent
        SWStatIterator it(STAT_VIS_ON);
        while (auto itActor = it.Next())
        {
            if (GetOwner(itActor) == parentActor)
            {
                KillActor(itActor);
            }
        }

        auto actorNew = InsertActor(psp->sectnum, STAT_VIS_ON);
        sp = &actorNew->s();
        SetOwner(parentActor, actorNew);


        ASSERT(parentActor->hasU());
        SET(pu->Flags2, SPR2_CHILDREN);

        sp->x = psp->x;
        sp->y = psp->y;
        sp->z = psp->z;

        SET(pu->Flags2, SPR2_VIS_SHADING);
    }
    else
    {
        if (sector[sectnum].floorpal == PALETTE_FOG)
            return;

        auto actorNew = InsertActor(sectnum, STAT_VIS_ON);
        sp = &actorNew->s();

        sp->x = x;
        sp->y = y;
        sp->z = z - Z(20);
    }

    sp->cstat = 0;
    sp->extra = 0;

    VIS_VisDir(sp) = 1;
    VIS_VisCur(sp) = NormalVisibility;
    VIS_VisGoal(sp) = amt;
}

END_SW_NS
