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

#include "compat.h"

#include "game.h"
#include "interpso.h"
#include "serializer.h"
#include "names2.h"

BEGIN_SW_NS

#define SO_MAXINTERPOLATIONS 1024

enum
{
    soi_base = 0xffffff,
    soi_wallx = 0x1000000,
    soi_wally = 0x2000000,
    soi_ceil = 0x3000000,
    soi_floor = 0x4000000,
    soi_sox = 0x5000000,
    soi_soy = 0x6000000,
    soi_soz = 0x7000000,
    soi_sprx = 0x8000000,
    soi_spry = 0x9000000,
    soi_sprz = 0xa000000,
};

static struct so_interp
{
    struct interp_data
    {
        int curelement;
        int32_t oldipos;
        int32_t bakipos;
        int32_t lastipos;
        int32_t lastoldipos;
        int32_t lastangdiff;
        int32_t spriteofang;
    } data[SO_MAXINTERPOLATIONS];

    int32_t numinterpolations;
    int32_t tic, lasttic;
    bool hasvator;
} so_interpdata[MAX_SECTOR_OBJECTS];

static int &getvalue(int element)
{
    static int scratch;
    int index = element & soi_base;
    int type = element & ~soi_base;
    switch (type)
    {
    case soi_wallx:
        return wall[index].x;
    case soi_wally:
        return wall[index].y;
    case soi_ceil:
        return sector[index].ceilingz;
    case soi_floor:
        return sector[index].floorz;
    case soi_sox:
        return SectorObject[index].xmid;
    case soi_soy:
        return SectorObject[index].ymid;
    case soi_soz:
        return SectorObject[index].zmid;
    case soi_sprx:
        return sprite[index].x;
    case soi_spry:
        return sprite[index].y;
    case soi_sprz:
        return sprite[index].z;
    default:
        return scratch;
    }
}

static void so_setpointinterpolation(so_interp *interp, int element)
{
    int32_t i;
    if (interp->numinterpolations >= SO_MAXINTERPOLATIONS)
        return;

    for (i = 0; i < interp->numinterpolations; i++)
    {
        if (interp->data[i].curelement == element)
            return;
    }

    so_interp::interp_data *data = &interp->data[interp->numinterpolations++];

    data->curelement = element;
    data->oldipos =
        data->lastipos =
        data->lastoldipos = getvalue(element);
    data->spriteofang = -1;
}

static void so_setspriteanginterpolation(so_interp *interp, int32_t spritenum)
{
    int32_t i;
    if (interp->numinterpolations >= SO_MAXINTERPOLATIONS)
        return;

    for (i = 0; i < interp->numinterpolations; i++)
        if (interp->data[i].curelement == -1 && interp->data[i].spriteofang == spritenum)
            return;

    so_interp::interp_data *data = &interp->data[interp->numinterpolations++];

    data->curelement = -1;
    data->oldipos =
        data->lastipos =
        data->lastoldipos = sprite[spritenum].ang;
    data->lastangdiff = 0;
    data->spriteofang = spritenum;
}

// Covers points and angles altogether
static void so_stopdatainterpolation(so_interp *interp, int element)
{
    int32_t i;

    for (i = 0; i < interp->numinterpolations; i++)
    {
        if (interp->data[i].curelement == -1)
        {
            if (interp->data[i].spriteofang == element) break;
        }
        else if (interp->data[i].curelement == element)
            break;

    }

    if (i == interp->numinterpolations)
        return;

    interp->data[i] = interp->data[--(interp->numinterpolations)];
}

void so_addinterpolation(SECTOR_OBJECTp sop)
{
    SECTORp *sectp;
    int32_t startwall, endwall;
    int32_t i;

    so_interp *interp = &so_interpdata[sop - SectorObject];
    interp->numinterpolations = 0;
    interp->hasvator = false;

    for (sectp = sop->sectp; *sectp; sectp++)
    {
        startwall = (*sectp)->wallptr;
        endwall = startwall + (*sectp)->wallnum - 1;

        for (i = startwall; i <= endwall; i++)
        {
            int32_t nextwall = wall[i].nextwall;

            so_setpointinterpolation(interp, i|soi_wallx);
            so_setpointinterpolation(interp, i|soi_wally);

            if (nextwall >= 0)
            {
                so_setpointinterpolation(interp, wall[nextwall].point2|soi_wallx);
                so_setpointinterpolation(interp, wall[nextwall].point2|soi_wally);
            }
        }

        int i;
        SectIterator it(*sectp - sector);
        while ((i = it.NextIndex()) >= 0)
            if (sprite[i].statnum == STAT_VATOR && SP_TAG1(sprite+i) == SECT_VATOR)
                break;
        interp->hasvator |= (i >= 0);
    }

    if (!interp->hasvator)
        for (sectp = sop->sectp; *sectp; sectp++)
        {
            so_setpointinterpolation(interp, int(*sectp - sector) | soi_floor);
            so_setpointinterpolation(interp, int(*sectp - sector) | soi_ceil);
        }

    // interpolate midpoint, for aiming at a remote controlled SO
    so_setpointinterpolation(interp, int(sop - SectorObject) | soi_sox);
    so_setpointinterpolation(interp, int(sop - SectorObject) | soi_soy);
    so_setpointinterpolation(interp, int(sop - SectorObject) | soi_soz);

    interp->tic = 0;
    interp->lasttic = synctics;
}

void so_setspriteinterpolation(SECTOR_OBJECTp sop, spritetype *sp)
{
    so_interp *interp = &so_interpdata[sop - SectorObject];
    int snum = int(sp - sprite);

    so_setpointinterpolation(interp, snum | soi_sprx);
    so_setpointinterpolation(interp, snum | soi_spry);
    if (!interp->hasvator)
        so_setpointinterpolation(interp, snum | soi_sprz);
    so_setspriteanginterpolation(interp, snum);
}

void so_stopspriteinterpolation(SECTOR_OBJECTp sop, spritetype *sp)
{
    so_interp *interp = &so_interpdata[sop - SectorObject];
    int snum = int(sp - sprite);

    so_stopdatainterpolation(interp, snum | soi_sprx);
    so_stopdatainterpolation(interp, snum | soi_spry);
    if (!interp->hasvator)
        so_stopdatainterpolation(interp, snum | soi_sprz);
    so_stopdatainterpolation(interp, -1);
}

void so_setinterpolationtics(SECTOR_OBJECTp sop, int16_t locktics)
{
    so_interp *interp = &so_interpdata[sop - SectorObject];

    interp->tic = 0;
    interp->lasttic = locktics;
}

void so_updateinterpolations(void) // Stick at beginning of domovethings
{
    int32_t i;
    SECTOR_OBJECTp sop;
    so_interp *interp;
    so_interp::interp_data *data;
    bool interpolating = cl_sointerpolation && !CommEnabled; // If changing from menu

    for (sop = SectorObject, interp = so_interpdata;
         sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++, interp++)
    {
        bool skip = !SyncInput() && (sop->track == SO_TURRET);
        if (SO_EMPTY(sop) || skip)
            continue;
        if (interp->tic < interp->lasttic)
            interp->tic += synctics;
        for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
        {
            if (data->spriteofang >= 0)
            {
                USERp u = User[data->spriteofang].Data();
                if (u)
                    u->oangdiff = 0;
                if (!interpolating)
                    data->lastangdiff = 0;
                data->oldipos = sprite[data->spriteofang].ang;
            }
            else
                data->oldipos = getvalue(data->curelement);

            if (!interpolating)
                data->lastipos = data->lastoldipos = data->oldipos;
        }
    }
}

// must call restore for every do interpolations
// make sure you don't exit
void so_dointerpolations(int32_t smoothratio)                      // Stick at beginning of drawscreen
{
    int32_t i, delta;
    SECTOR_OBJECTp sop;
    so_interp *interp;
    so_interp::interp_data *data;

    // Set the bakipos values separately, in case a point is shared.
    // Also set lastipos if there's been an actual change in a point.
    for (sop = SectorObject, interp = so_interpdata;
         sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++, interp++)
    {
        bool skip = !SyncInput() && (sop->track == SO_TURRET);
        if (SO_EMPTY(sop) || skip)
            continue;

        for (i = 0; i < interp->numinterpolations; i++)
            interp->data[i].bakipos = (interp->data[i].spriteofang >= 0) ?
                                      sprite[interp->data[i].spriteofang].ang :
                                      getvalue(interp->data[i].curelement);

        if (interp->tic == 0) // Only if the SO has just moved
        {
            for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
            {
                data->lastipos = data->bakipos;
                data->lastoldipos = data->oldipos;
                if (data->spriteofang >= 0)
                {
                    USERp u = User[data->spriteofang].Data();
                    data->lastangdiff = u ? u->oangdiff : 0;
                }
            }
        }
    }

    for (sop = SectorObject, interp = so_interpdata;
         sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++, interp++)
    {
        bool skip = !SyncInput() && (sop->track == SO_TURRET);
        if (SO_EMPTY(sop) || skip)
            continue;

        // Check if interpolation has been explicitly disabled
        if (interp->lasttic == 0)
            continue;

        // Unfortunately, interpolating over less samples doesn't work well
        // in multiplayer. We also skip any sector object not
        // remotely controlled by some player.
        if (CommEnabled &&
            ((interp->lasttic != synctics) ||
             !(sop->controller) ||
             ((Player[screenpeek].sop_control == sop) &&
               !Player[screenpeek].sop_remote)))
            continue;

        int32_t ratio = smoothratio * synctics + 65536 * interp->tic;
        ratio /= interp->lasttic;
        ratio = (interp->tic == interp->lasttic) ? 65536 : ratio;

        for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
        {
            // Hack for jittery coolies in level 1's train.
            // Based in idea on code from draw.cpp:analyzesprites.
            // TODO: It could be better. In particular, it could be better
            // to conditionally disable the interpolation from analyzesprites
            // instead, using TSPRITE info if possible.
            if (data->curelement >= soi_sprx)
            {
                int32_t sprnum = data->curelement & soi_base;
                USERp u = User[sprnum].Data();
                if (u && (sprite[sprnum].statnum != STAT_DEFAULT) &&
                    ((TEST(u->Flags, SPR_SKIP4) && (sprite[sprnum].statnum <= STAT_SKIP4_INTERP_END)) ||
                     (TEST(u->Flags, SPR_SKIP2) && (sprite[sprnum].statnum <= STAT_SKIP2_INTERP_END))))
                    continue;
            }

            if (data->spriteofang >= 0)
                sprite[data->spriteofang].ang = NORM_ANGLE(data->lastoldipos + MulScale(data->lastangdiff, ratio, 16));
            else
            {
                delta = data->lastipos - data->lastoldipos;
                getvalue(data->curelement) = data->lastoldipos + MulScale(delta, ratio, 16);
            }
        }
    }
}

void so_restoreinterpolations(void)                 // Stick at end of drawscreen
{
    int32_t i;
    SECTOR_OBJECTp sop;
    so_interp *interp;
    so_interp::interp_data *data;

    for (sop = SectorObject, interp = so_interpdata;
         sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++, interp++)
    {
        bool skip = !SyncInput() && (sop->track == SO_TURRET);
        if (SO_EMPTY(sop) || skip)
            continue;

        for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
            if (data->spriteofang >= 0)
                sprite[data->spriteofang].ang = data->bakipos;
            else
                getvalue(data->curelement) = data->bakipos;
    }
}

void so_serializeinterpolations(FSerializer& arc)
{
    SECTOR_OBJECTp sop;
    so_interp* interp;

    if (arc.BeginArray("sop_interp"))
    {
        for (sop = SectorObject, interp = so_interpdata; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++, interp++)
        {
            if (arc.BeginObject(nullptr))
            {
                so_interp::interp_data* data = interp->data;
                arc("numinterp", interp->numinterpolations)
                    ("hasvator", interp->hasvator);
                if (arc.BeginArray("data"))
                {
                    for (int i = 0; i < interp->numinterpolations; i++, data++)
                    {
                        if (arc.BeginObject(nullptr))
                        {
                            arc("curelement", data->curelement)
                                ("oldipos", data->oldipos)
                                ("spriteofang", data->spriteofang)
                                .EndObject();
                            if (arc.isReading())
                            {
                                data->lastipos = data->lastoldipos = data->oldipos;
                                data->lastangdiff = 0;
                            }
                        }
                    }
                    arc.EndArray();
                }
                arc.EndObject();
                interp->tic = 0;
                interp->lasttic = synctics;
            }
        }
        arc.EndArray();
    }
}

END_SW_NS
