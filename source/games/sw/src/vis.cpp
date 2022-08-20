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

inline int16_t& VIS_VisCur(DSWActor* actor) { return SP_TAG2(actor); }
#define VIS_VisDir(actor) (SP_TAG3(actor))
#define VIS_VisGoal(actor) (SP_TAG4(actor))

void ProcessVisOn(void)
{
    SWStatIterator it(STAT_VIS_ON);
    while (auto actor = it.Next())
    {
        if (VIS_VisDir(actor))
        {
            // get brighter
            VIS_VisCur(actor) >>= 1;
            //VIS_VisCur(actor) -= 16;
            if (VIS_VisCur(actor) <= VIS_VisGoal(actor))
            {
                VIS_VisCur(actor) = VIS_VisGoal(actor);
                VIS_VisDir(actor) ^= 1;
            }
        }
        else
        {
            // get darker
            VIS_VisCur(actor) <<= 1;
            VIS_VisCur(actor) += 1;
            //VIS_VisCur(actor) += 16;
            if (VIS_VisCur(actor) >= NormalVisibility)
            {
                VIS_VisCur(actor) = NormalVisibility;
                auto own = GetOwner(actor);
                if (own != nullptr)
                {
                    ASSERT(own->hasU());
                    own->user.Flags2 &= ~(SPR2_VIS_SHADING);
                }
                KillActor(actor);
            }
        }
    }
}

void VisViewChange(PLAYER* pp, int *vis)
{
    short BrightestVis = NormalVisibility;
    int x,y,z;
    sectortype* sectp;

    if (paused)
        return;

    // find the closest quake - should be a strength value
    SWStatIterator it(STAT_VIS_ON);
    while (auto actor = it.Next())
    {
        auto own = GetOwner(actor);
        if (own != nullptr)
        {
            x = own->int_pos().X;
            y = own->int_pos().Y;
            z = own->int_pos().Z;
            sectp = own->sector();
        }
        else
        {
            x = actor->int_pos().X;
            y = actor->int_pos().Y;
            z = actor->int_pos().Z;
            sectp = actor->sector();
        }

        // save off the brightest vis that you can see
        if (FAFcansee(pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, pp->cursector, x, y, z, sectp))
        {
            if (VIS_VisCur(actor) < BrightestVis)
                BrightestVis = VIS_VisCur(actor);
        }
    }

    *vis = BrightestVis;
}

void SpawnVis(DSWActor* parentActor, sectortype* sect, const DVector3& pos, int amt)
{
    DSWActor* actorNew = nullptr;
    if (parentActor != nullptr)
    {
        if (parentActor->sector()->floorpal == PALETTE_FOG)
            return;

        if (parentActor->sector()->floorpal == PALETTE_DIVE_LAVA)
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

        actorNew = insertActor(parentActor->sector(), STAT_VIS_ON);
        SetOwner(parentActor, actorNew);


        ASSERT(parentActor->hasU());
        parentActor->user.Flags2 |= (SPR2_CHILDREN);

        actorNew->spr.pos = parentActor->spr.pos;

        parentActor->user.Flags2 |= (SPR2_VIS_SHADING);
    }
    else
    {
        if (sect->floorpal == PALETTE_FOG)
            return;

        actorNew = insertActor(sect, STAT_VIS_ON);

        actorNew->spr.pos = pos.plusZ(-20);
    }

    actorNew->spr.cstat = 0;
    actorNew->spr.extra = 0;

    VIS_VisDir(actorNew) = 1;
    VIS_VisCur(actorNew) = NormalVisibility;
    VIS_VisGoal(actorNew) = amt;
}

END_SW_NS
