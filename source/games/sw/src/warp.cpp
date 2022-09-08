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
#include "misc.h"

BEGIN_SW_NS

////////////////////////////////////////////////////////////////////////////////
//
// WARPING - PLANE STYLE
//
////////////////////////////////////////////////////////////////////////////////

extern bool Prediction;
DSWActor* WarpToArea(DSWActor* sp_from, int32_t* x, int32_t* y, int32_t* z, sectortype** sect);

bool WarpPlaneSectorInfo(sectortype* sect, DSWActor** sp_ceiling, DSWActor** sp_floor)
{
    *sp_floor = nullptr;
    *sp_ceiling = nullptr;

    if (Prediction)
        return false;

    if (sect== nullptr || !(sect->extra & SECTFX_WARP_SECTOR))
        return false;

    SWStatIterator it(STAT_WARP);
    while (auto actor = it.Next())
    {
        if (actor->sector() == sect)
        {
            // skip - don't teleport
            if (SP_TAG10(actor) == 1)
                continue;

            if (actor->spr.hitag == WARP_CEILING_PLANE)
            {
                *sp_ceiling = actor;
            }
            else if (actor->spr.hitag == WARP_FLOOR_PLANE)
            {
                *sp_floor = actor;
            }
        }
    }

    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DSWActor* WarpPlane(int32_t* x, int32_t* y, int32_t* z, sectortype** sect)
{
    DSWActor* sp_floor,* sp_ceiling;

    if (Prediction)
        return nullptr;

    if (!WarpPlaneSectorInfo(*sect, &sp_ceiling, &sp_floor))
        return nullptr;

    if (sp_ceiling)
    {
        if (*z <= sp_ceiling->int_pos().Z)
        {
            return WarpToArea(sp_ceiling, x, y, z, sect);
        }
    }

    if (sp_floor)
    {
        if (*z >= sp_floor->int_pos().Z)
        {
            return WarpToArea(sp_floor, x, y, z, sect);
        }
    }

    return nullptr;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DSWActor* WarpToArea(DSWActor* sp_from, int32_t* x, int32_t* y, int32_t* z, sectortype** sect)
{
    int xoff;
    int yoff;
    int zoff;
    short match;
    short to_tag = 0;
    short match_rand[16];
    int z_adj = 0;

    xoff = *x - sp_from->int_pos().X;
    yoff = *y - sp_from->int_pos().Y;
    zoff = *z - sp_from->int_pos().Z;
    match = sp_from->spr.lotag;

#if 0
    TAG 2 = match
            TAG 3 = Type
                    Sprite - 0,32 always teleports you to the center at the angle the sprite is facing
    Offset - 1 always teleports you by the offset.Does not touch the angle
    TAG 4 = angle
            TAG 5 to 8 = random match locations
#endif

    memset(match_rand,0,sizeof(match_rand));

    switch (sp_from->spr.hitag)
    {
    case WARP_TELEPORTER:
        to_tag = WARP_TELEPORTER;

        // if tag 5 has something this is a random teleporter
        if (SP_TAG5(sp_from))
        {
            short ndx = 0;
            match_rand[ndx++] = SP_TAG2(sp_from);
            match_rand[ndx++] = SP_TAG5(sp_from);
            if (SP_TAG6(sp_from))
                match_rand[ndx++] = SP_TAG6(sp_from);
            if (SP_TAG7(sp_from))
                match_rand[ndx++] = SP_TAG7(sp_from);
            if (SP_TAG8(sp_from))
                match_rand[ndx++] = SP_TAG8(sp_from);

            // reset the match you are looking for
            match = match_rand[RandomRange(ndx)];
        }
        break;
    case WARP_CEILING_PLANE:
        to_tag = WARP_FLOOR_PLANE;
        // make sure you warp outside of warp plane
        z_adj = -Z(2);
        break;
    case WARP_FLOOR_PLANE:
        to_tag = WARP_CEILING_PLANE;
        // make sure you warp outside of warp plane
        z_adj = Z(2);
        break;
    }

    SWStatIterator it(STAT_WARP);
    while (auto actor = it.Next())
    {
        if (actor->spr.lotag == match && actor != sp_from)
        {
            // exp: WARP_CEILING or WARP_CEILING_PLANE
            if (actor->spr.hitag == to_tag)
            {
                if (!actor->insector())
                    return nullptr;

                // determine new x,y,z position
                *x = actor->int_pos().X + xoff;
                *y = actor->int_pos().Y + yoff;
                *z = actor->int_pos().Z + zoff;

                // make sure you warp outside of warp plane
                *z += z_adj;

                // get new sector
                *sect = actor->sector();
                updatesector(*x, *y, sect);

                return actor;
            }
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
//
// Warp - Teleporter style
//
////////////////////////////////////////////////////////////////////////////////

bool WarpSectorInfo(sectortype* sect, DSWActor** sp_warp)
{
    *sp_warp = nullptr;

    if (!sect || !(sect->extra & SECTFX_WARP_SECTOR))
        return false;

    SWStatIterator it(STAT_WARP);
    while (auto actor = it.Next())
    {
        if (actor->sector() == sect)
        {
            // skip - don't teleport
            if (SP_TAG10(actor) == 1)
                continue;

            if (actor->spr.hitag == WARP_TELEPORTER)
            {
                *sp_warp = actor;
            }
        }
    }

    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DSWActor* Warp(int32_t* x, int32_t* y, int32_t* z, sectortype** sect)
{
    DSWActor* sp_warp;

    if (Prediction)
        return nullptr;

    if (!WarpSectorInfo(*sect, &sp_warp))
        return nullptr;

    if (sp_warp)
    {
        return WarpToArea(sp_warp, x, y, z, sect);
    }

    return nullptr;
}
END_SW_NS
