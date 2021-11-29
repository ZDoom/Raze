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
DSWActor* WarpToArea(DSWActor* sp_from, int32_t* x, int32_t* y, int32_t* z, int* sectnum);

bool WarpPlaneSectorInfo(short sectnum, DSWActor** sp_ceiling, DSWActor** sp_floor)
{
    *sp_floor = nullptr;
    *sp_ceiling = nullptr;

    if (Prediction)
        return false;

    if (sectnum < 0 || !TEST(sector[sectnum].extra, SECTFX_WARP_SECTOR))
        return false;

    SWStatIterator it(STAT_WARP);
    while (auto actor = it.Next())
    {
        auto sp = &actor->s();

        if (sp->sectnum == sectnum)
        {
            // skip - don't teleport
            if (SP_TAG10(sp) == 1)
                continue;

            if (sp->hitag == WARP_CEILING_PLANE)
            {
                *sp_ceiling = actor;
            }
            else if (sp->hitag == WARP_FLOOR_PLANE)
            {
                *sp_floor = actor;
            }
        }
    }

    return true;
}

DSWActor* WarpPlane(int32_t* x, int32_t* y, int32_t* z, int* sectnum)
{
    DSWActor* sp_floor,* sp_ceiling;

    if (Prediction)
        return nullptr;

    if (!WarpPlaneSectorInfo(*sectnum, &sp_ceiling, &sp_floor))
        return nullptr;

    if (sp_ceiling)
    {
        if (*z <= sp_ceiling->s().z)
        {
            return WarpToArea(sp_ceiling, x, y, z, sectnum);
        }
    }

    if (sp_floor)
    {
        if (*z >= sp_floor->s().z)
        {
            return WarpToArea(sp_floor, x, y, z, sectnum);
        }
    }

    return nullptr;
}

DSWActor* WarpToArea(DSWActor* sp_from, int32_t* x, int32_t* y, int32_t* z, int* sectnum)
{
    int xoff;
    int yoff;
    int zoff;
    SPRITEp const sp = &sp_from->s();
    short match;
    short to_tag = 0;
    short match_rand[16];
    int z_adj = 0;

    xoff = *x - sp->x;
    yoff = *y - sp->y;
    zoff = *z - sp->z;
    match = sp->lotag;

#if 0
    TAG 2 = match
            TAG 3 = Type
                    Sprite - 0,32 always teleports you to the center at the angle the sprite is facing
    Offset - 1 always teleports you by the offset.Does not touch the angle
    TAG 4 = angle
            TAG 5 to 8 = random match locations
#endif

    memset(match_rand,0,sizeof(match_rand));

    switch (sp->hitag)
    {
    case WARP_TELEPORTER:
        to_tag = WARP_TELEPORTER;

        // if tag 5 has something this is a random teleporter
        if (SP_TAG5(sp))
        {
            short ndx = 0;
            match_rand[ndx++] = SP_TAG2(sp);
            match_rand[ndx++] = SP_TAG5(sp);
            if (SP_TAG6(sp))
                match_rand[ndx++] = SP_TAG6(sp);
            if (SP_TAG7(sp))
                match_rand[ndx++] = SP_TAG7(sp);
            if (SP_TAG8(sp))
                match_rand[ndx++] = SP_TAG8(sp);

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
        auto spi = &actor->s();

        if (spi->lotag == match && actor != sp_from)
        {
            // exp: WARP_CEILING or WARP_CEILING_PLANE
            if (spi->hitag == to_tag)
            {
                if (!validSectorIndex(spi->sectnum))
                    return nullptr;

                // determine new x,y,z position
                *x = spi->x + xoff;
                *y = spi->y + yoff;
                *z = spi->z + zoff;

                // make sure you warp outside of warp plane
                *z += z_adj;

                // get new sector
                *sectnum = spi->sectnum;
                updatesector(*x, *y, sectnum);

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

bool WarpSectorInfo(short sectnum, DSWActor** sp_warp)
{
    *sp_warp = nullptr;

    if (!TEST(sector[sectnum].extra, SECTFX_WARP_SECTOR))
        return false;

    SWStatIterator it(STAT_WARP);
    while (auto actor = it.Next())
    {
        auto sp = &actor->s();

        if (sp->sectnum == sectnum)
        {
            // skip - don't teleport
            if (SP_TAG10(sp) == 1)
                continue;

            if (sp->hitag == WARP_TELEPORTER)
            {
                *sp_warp = actor;
            }
        }
    }

    return true;
}

DSWActor* Warp(int32_t* x, int32_t* y, int32_t* z, int* sectnum)
{
    DSWActor* sp_warp;

    if (Prediction)
        return nullptr;

    if (!WarpSectorInfo(*sectnum, &sp_warp))
        return nullptr;

    if (sp_warp)
    {
        return WarpToArea(sp_warp, x, y, z, sectnum);
    }

    return nullptr;
}
END_SW_NS
