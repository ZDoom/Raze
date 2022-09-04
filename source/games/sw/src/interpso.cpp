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
	soi_sprang = 0xb000000,
};

static struct so_interp
{
    struct interp_data
    {
        int curelement;
        double oldipos;
        double bakipos;
        double lastipos;
        double lastoldipos;
        DAngle lastangdiff;
        TObjPtr<DSWActor*> actorofang;
    } data[SO_MAXINTERPOLATIONS];

    int32_t numinterpolations;
    int32_t tic, lasttic;
    bool hasvator;
} so_interpdata[MAX_SECTOR_OBJECTS];

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void MarkSOInterp()
{
    int32_t i;
    SECTOR_OBJECT* sop;
    so_interp* interp;
    so_interp::interp_data* data;

    for (sop = SectorObject, interp = so_interpdata;
        sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++, interp++)
    {
        if (SO_EMPTY(sop))
            continue;
        for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
        {
            GC::Mark(data->actorofang);
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static double getvalue(so_interp::interp_data& element)
{
    int index = element.curelement & soi_base;
    int type = element.curelement & ~soi_base;
    switch (type)
    {
    case soi_wallx:
        return wall[index].pos.X;
    case soi_wally:
        return wall[index].pos.Y;
    case soi_ceil:
        return sector[index].ceilingz;
    case soi_floor:
        return sector[index].floorz;
    case soi_sox:
        return SectorObject[index].pmid.X;
    case soi_soy:
        return SectorObject[index].pmid.Y;
    case soi_soz:
        return SectorObject[index].pmid.Z;
    case soi_sprx:
		if (element.actorofang)
			return element.actorofang->spr.pos.X;
        break;
    case soi_spry:
		if (element.actorofang)
			return element.actorofang->spr.pos.Y;
        break;
    case soi_sprz:
		if (element.actorofang)
			return element.actorofang->spr.pos.Z;
        break;
    default:
		break;
    }
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void setvalue(so_interp::interp_data& element, double value)
{
    int index = element.curelement & soi_base;
    int type = element.curelement & ~soi_base;
    switch (type)
    {
    case soi_wallx:
        wall[index].pos.X = value;
        wall[index].moved();
        break;
    case soi_wally:
        wall[index].pos.Y = value;
        wall[index].moved();
        break;
    case soi_ceil:
        sector[index].setceilingz(value);
        break;
    case soi_floor:
        sector[index].setfloorz(value);
        break;
    case soi_sox:
        SectorObject[index].pmid.X = value;
        break;
    case soi_soy:
        SectorObject[index].pmid.Y = value;
        break;
    case soi_soz:
        SectorObject[index].pmid.Z = value;
        break;
    case soi_sprx:
        if (element.actorofang)
            element.actorofang->spr.pos.X = value;
        break;
    case soi_spry:
        if (element.actorofang)
            element.actorofang->spr.pos.Y = value;
        break;
    case soi_sprz:
        if (element.actorofang)
            element.actorofang->spr.pos.Z = value;
        break;
    default:
        break;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void so_setpointinterpolation(so_interp *interp, int element, DSWActor* actor = nullptr)
{
    int32_t i;
    if (interp->numinterpolations >= SO_MAXINTERPOLATIONS)
        return;

    for (i = 0; i < interp->numinterpolations; i++)
    {
        if (interp->data[i].curelement == element && interp->data[i].actorofang == actor)
            return;
    }

    so_interp::interp_data *data = &interp->data[interp->numinterpolations++];

    data->curelement = element;
	data->actorofang = actor;
    data->oldipos =
        data->lastipos =
        data->lastoldipos = getvalue(*data);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void so_setspriteanginterpolation(so_interp *interp, DSWActor* actor)
{
    int32_t i;
    if (interp->numinterpolations >= SO_MAXINTERPOLATIONS)
        return;

    for (i = 0; i < interp->numinterpolations; i++)
        if (interp->data[i].curelement == soi_sprang && interp->data[i].actorofang == actor)
            return;

    so_interp::interp_data *data = &interp->data[interp->numinterpolations++];

    data->curelement = soi_sprang;
    data->oldipos =
        data->lastipos =
        data->lastoldipos = actor->spr.angle.Degrees();
    data->lastangdiff = nullAngle;
    data->actorofang = actor;
}

//---------------------------------------------------------------------------
//
// Covers points and angles altogether
//
//---------------------------------------------------------------------------

static void so_stopdatainterpolation(so_interp *interp, int element, DSWActor* actor)
{
	for (int i = 0; i < interp->numinterpolations; i++)
	{
		if (interp->data[i].curelement == element && interp->data[i].actorofang == actor)
		{
			interp->data[i] = interp->data[--(interp->numinterpolations)];
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void so_addinterpolation(SECTOR_OBJECT* sop)
{
    sectortype* *sectp;
    int32_t startwall, endwall;
    int32_t i;

    so_interp *interp = &so_interpdata[sop - SectorObject];
    interp->numinterpolations = 0;
    interp->hasvator = false;

    for (sectp = sop->sectp; *sectp; sectp++)
    {
        for (auto& wal : wallsofsector(*sectp))
        {
            so_setpointinterpolation(interp, wallnum(&wal) | soi_wallx);
            so_setpointinterpolation(interp, wallnum(&wal) | soi_wally);

            if (wal.twoSided())
            {
                auto nextWall = wal.nextWall()->point2Wall();
                so_setpointinterpolation(interp, wallnum(nextWall) | soi_wallx);
                so_setpointinterpolation(interp, wallnum(nextWall) | soi_wally);
            }
        }


        SWSectIterator it(*sectp);
        while (auto actor = it.Next())
            if (actor->spr.statnum == STAT_VATOR && SP_TAG1(actor) == SECT_VATOR)
            {
                interp->hasvator = true;
                break;
            }
    }

    if (!interp->hasvator)
        for (sectp = sop->sectp; *sectp; sectp++)
        {
            so_setpointinterpolation(interp, sectnum(*sectp) | soi_floor);
            so_setpointinterpolation(interp, sectnum(*sectp) | soi_ceil);
        }

    // interpolate midpoint, for aiming at a remote controlled SO
    so_setpointinterpolation(interp, int(sop - SectorObject) | soi_sox);
    so_setpointinterpolation(interp, int(sop - SectorObject) | soi_soy);
    so_setpointinterpolation(interp, int(sop - SectorObject) | soi_soz);

    interp->tic = 0;
    interp->lasttic = synctics;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void so_setspriteinterpolation(SECTOR_OBJECT* sop, DSWActor* actor)
{
    so_interp *interp = &so_interpdata[sop - SectorObject];

    so_setpointinterpolation(interp, soi_sprx, actor);
    so_setpointinterpolation(interp, soi_spry, actor);
    if (!interp->hasvator)
        so_setpointinterpolation(interp, soi_sprz, actor);
    so_setspriteanginterpolation(interp, actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void so_stopspriteinterpolation(SECTOR_OBJECT* sop, DSWActor *actor)
{
    so_interp *interp = &so_interpdata[sop - SectorObject];

    so_stopdatainterpolation(interp, soi_sprx, actor);
    so_stopdatainterpolation(interp, soi_spry, actor);
    if (!interp->hasvator)
        so_stopdatainterpolation(interp, soi_sprz, actor);
    so_stopdatainterpolation(interp, soi_sprang, actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void so_setinterpolationtics(SECTOR_OBJECT* sop, int16_t locktics)
{
    so_interp *interp = &so_interpdata[sop - SectorObject];

    interp->tic = 0;
    interp->lasttic = locktics;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void so_updateinterpolations(void) // Stick at beginning of domovethings
{
    int32_t i;
    SECTOR_OBJECT* sop;
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
            if (data->curelement == soi_sprang)
            {
                auto actorofang = data->actorofang;
                if (actorofang)
                {
                    if (actorofang->hasU())
                        actorofang->user.oangdiff = nullAngle;
                    if (!interpolating)
                        data->lastangdiff = nullAngle;
                    data->oldipos = actorofang->spr.angle.Degrees();
                }
            }
            else
            {
                data->oldipos = getvalue(*data);
            }

            if (!interpolating)
                data->lastipos = data->lastoldipos = data->oldipos;
        }
    }
}

//---------------------------------------------------------------------------
//
// must call restore for every do interpolations
// make sure you don't exit
//
//---------------------------------------------------------------------------

void so_dointerpolations(double interpfrac)                      // Stick at beginning of drawscreen
{
    int32_t i;
    SECTOR_OBJECT* sop;
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
        {
            auto actorofang = interp->data[i].actorofang;
            if (interp->data[i].curelement >= soi_sprx && actorofang == nullptr)
                continue; // target went poof.

            interp->data[i].bakipos = (interp->data[i].curelement == soi_sprang) ?
                                      actorofang->spr.angle.Degrees() :
                                      getvalue(interp->data[i]);
        }
        if (interp->tic == 0) // Only if the SO has just moved
        {
            for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
            {
                data->lastipos = data->bakipos;
                data->lastoldipos = data->oldipos;
                if (data->curelement == soi_sprang)
                {
                    auto actorofang = data->actorofang;
                    if (actorofang)
                    {
                        data->lastangdiff = actorofang->hasU() ? actorofang->user.oangdiff : nullAngle;
                    }
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

        double ratio = interpfrac * synctics + interp->tic;
        ratio /= interp->lasttic;
        ratio = (interp->tic == interp->lasttic) ? 1. : ratio;

        for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
        {
            // Hack for jittery coolies in level 1's train.
            // Based in idea on code from draw.cpp:analyzesprites.
            // TODO: It could be better. In particular, it could be better
            // to conditionally disable the interpolation from analyzesprites
            // instead, using TSPRITE info if possible.
            if (data->curelement >= soi_sprx && data->curelement <= soi_sprz)
            {
				DSWActor* actor = data->actorofang;
                if (!actor) continue;
                if (actor->hasU() && (actor->spr.statnum != STAT_DEFAULT) &&
                    ((actor->user.Flags & (SPR_SKIP4) && (actor->spr.statnum <= STAT_SKIP4_INTERP_END)) ||
                     (actor->user.Flags & (SPR_SKIP2) && (actor->spr.statnum <= STAT_SKIP2_INTERP_END))))
                    continue;
            }

            if (data->curelement == soi_sprang)
            {
                DSWActor* actor = data->actorofang;
                if (!actor) continue;
                actor->spr.angle = (DAngle::fromDeg(data->lastoldipos) + data->lastangdiff * ratio).Normalized360();
            }
            else
            {
                double delta = data->lastipos - data->lastoldipos;
                setvalue(*data, data->lastoldipos + delta * ratio);
            }
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void so_restoreinterpolations(void)                 // Stick at end of drawscreen
{
    int32_t i;
    SECTOR_OBJECT* sop;
    so_interp *interp;
    so_interp::interp_data *data;

    for (sop = SectorObject, interp = so_interpdata;
         sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++, interp++)
    {
        bool skip = !SyncInput() && (sop->track == SO_TURRET);
        if (SO_EMPTY(sop) || skip)
            continue;

        for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
            if (data->curelement == soi_sprang)
            {
                auto actorofang = interp->data[i].actorofang;
                if (actorofang) actorofang->spr.angle = DAngle::fromDeg(data->bakipos);
            }
            else
            {
                setvalue(*data, data->bakipos);
            }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void so_serializeinterpolations(FSerializer& arc)
{
    SECTOR_OBJECT* sop;
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
                                ("spriteofang", data->actorofang)
                                .EndObject();
                            if (arc.isReading())
                            {
                                data->lastipos = data->lastoldipos = data->oldipos;
                                data->lastangdiff = nullAngle;
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
