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
    sectortype* sectp;

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
            x = own->spr.pos.X;
            y = own->spr.pos.Y;
            z = own->spr.pos.Z;
            sectp = own->spr.sector();
        }
        else
        {
            x = sp->pos.X;
            y = sp->pos.Y;
            z = sp->pos.Z;
            sectp = sp->sector();
        }

        // save off the brightest vis that you can see
        if (FAFcansee(pp->pos.X, pp->pos.Y, pp->pos.Z, pp->cursector, x, y, z, sectp))
        {
            if (VIS_VisCur(sp) < BrightestVis)
                BrightestVis = VIS_VisCur(sp);
        }
    }

    *vis = BrightestVis;
}

void SpawnVis(DSWActor* parentActor, sectortype* sect, int x, int y, int z, int amt)
{
    SPRITEp sp;
    if (parentActor != nullptr)
    {
        auto psp = &parentActor->s();
        auto pu = parentActor->u();

        if (psp->sector()->floorpal == PALETTE_FOG)
            return;

        if (psp->sector()->floorpal == PALETTE_DIVE_LAVA)
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

        auto actorNew = insertActor(psp->sector(), STAT_VIS_ON);
        sp = &actorNew->s();
        SetOwner(parentActor, actorNew);


        ASSERT(parentActor->hasU());
        SET(pu->Flags2, SPR2_CHILDREN);

        sp->pos.X = psp->pos.X;
        sp->pos.Y = psp->pos.Y;
        sp->pos.Z = psp->pos.Z;

        SET(pu->Flags2, SPR2_VIS_SHADING);
    }
    else
    {
        if (sect->floorpal == PALETTE_FOG)
            return;

        auto actorNew = insertActor(sect, STAT_VIS_ON);
        sp = &actorNew->s();

        sp->pos.X = x;
        sp->pos.Y = y;
        sp->pos.Z = z - Z(20);
    }

    sp->cstat = 0;
    sp->extra = 0;

    VIS_VisDir(sp) = 1;
    VIS_VisCur(sp) = NormalVisibility;
    VIS_VisGoal(sp) = amt;
}

END_SW_NS
