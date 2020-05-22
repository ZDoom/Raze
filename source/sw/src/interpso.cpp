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
#include "pragmas.h"

#include "game.h"
#include "interp.h"
#include "interpso.h"

BEGIN_SW_NS

#define SO_MAXINTERPOLATIONS MAXINTERPOLATIONS

static struct so_interp
{
    struct interp_data
    {
        void *curipos;
        int32_t oldipos;
        int32_t bakipos;
        int32_t lastipos;
        int32_t lastoldipos;
        int32_t lastangdiff;
        int32_t spriteofang;
    } data[SO_MAXINTERPOLATIONS];

    int32_t numinterpolations;
    int32_t tic, lasttic;
} so_interpdata[MAX_SECTOR_OBJECTS];

static void so_setpointinterpolation(so_interp *interp, int32_t *posptr)
{
    int32_t i;
    if (interp->numinterpolations >= SO_MAXINTERPOLATIONS)
        return;

    for (i = 0; i < interp->numinterpolations; i++)
        if (interp->data[i].curipos == posptr)
            return;

    so_interp::interp_data *data = &interp->data[interp->numinterpolations++];

    data->curipos = posptr;
    data->oldipos = *posptr;
    data->lastipos = *posptr;
    data->lastoldipos = *posptr;
    data->spriteofang = -1;
}

static void so_setspriteanginterpolation(so_interp *interp, int16_t *posptr, int32_t spritenum)
{
    int32_t i;
    if (interp->numinterpolations >= SO_MAXINTERPOLATIONS)
        return;

    for (i = 0; i < interp->numinterpolations; i++)
        if (interp->data[i].curipos == posptr)
            return;

    so_interp::interp_data *data = &interp->data[interp->numinterpolations++];

    data->curipos = posptr;
    data->oldipos = *posptr;
    data->lastipos = *posptr;
    data->lastoldipos = *posptr;
    data->lastangdiff = 0;
    data->spriteofang = spritenum;
}

// Covers points and angles altogether
static void so_stopdatainterpolation(so_interp *interp, void *posptr)
{
    int32_t i;

    for (i = 0; i < interp->numinterpolations; i++)
        if (interp->data[i].curipos == posptr)
            break;

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

    for (sectp = sop->sectp; *sectp; sectp++)
    {
        startwall = (*sectp)->wallptr;
        endwall = startwall + (*sectp)->wallnum - 1;

        for (i = startwall; i <= endwall; i++)
        {
            int32_t nextwall = wall[i].nextwall;

            so_setpointinterpolation(interp, &wall[i].x);
            so_setpointinterpolation(interp, &wall[i].y);

            if (nextwall >= 0)
            {
                so_setpointinterpolation(interp, &wall[wall[nextwall].point2].x);
                so_setpointinterpolation(interp, &wall[wall[nextwall].point2].y);
            }
        }

        so_setpointinterpolation(interp, &(*sectp)->ceilingz);
        so_setpointinterpolation(interp, &(*sectp)->floorz);
    }

    // interpolate midpoint, for aiming at a remote controlled SO
    so_setpointinterpolation(interp, &sop->xmid);
    so_setpointinterpolation(interp, &sop->ymid);
    so_setpointinterpolation(interp, &sop->zmid);

    interp->tic = 0;
    interp->lasttic = synctics;
}

void so_setspriteinterpolation(SECTOR_OBJECTp sop, spritetype *sp)
{
    so_interp *interp = &so_interpdata[sop - SectorObject];

    so_setpointinterpolation(interp, &sp->x);
    so_setpointinterpolation(interp, &sp->y);
    so_setpointinterpolation(interp, &sp->z);
    so_setspriteanginterpolation(interp, &sp->ang, sp - sprite);
}

void so_stopspriteinterpolation(SECTOR_OBJECTp sop, spritetype *sp)
{
    so_interp *interp = &so_interpdata[sop - SectorObject];

    so_stopdatainterpolation(interp, &sp->x);
    so_stopdatainterpolation(interp, &sp->y);
    so_stopdatainterpolation(interp, &sp->z);
    so_stopdatainterpolation(interp, &sp->ang);
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
    SWBOOL interpolating = cl_sointerpolation && !CommEnabled; // If changing from menu

    for (sop = SectorObject, interp = so_interpdata;
         sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++, interp++)
    {
        if (sop->xmid == INT32_MAX /*|| sop->xmid == MAXSO*/)
            continue;
        if (interp->tic < interp->lasttic)
            interp->tic += synctics;
        for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
        {
            if (data->spriteofang >= 0)
            {
                USERp u = User[data->spriteofang];
                if (u)
                    u->oangdiff = 0;
                if (!interpolating)
                    data->lastangdiff = 0;
                data->oldipos = *(int16_t *)(data->curipos);
            }
            else
                data->oldipos = *(int32_t *)(data->curipos);

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
        if (sop->xmid == INT32_MAX /*|| sop->xmid == MAXSO*/)
            continue;

        for (i = 0; i < interp->numinterpolations; i++)
            interp->data[i].bakipos = (interp->data[i].spriteofang >= 0) ?
                                      *(int16_t *)(interp->data[i].curipos) :
                                      *(int32_t *)(interp->data[i].curipos);

        if (interp->tic == 0) // Only if the SO has just moved
        {
            for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
            {
                data->lastipos = data->bakipos;
                data->lastoldipos = data->oldipos;
                if (data->spriteofang >= 0)
                {
                    USERp u = User[data->spriteofang];
                    data->lastangdiff = u ? u->oangdiff : 0;
                }
            }
        }
    }

    for (sop = SectorObject, interp = so_interpdata;
         sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++, interp++)
    {
        if (sop->xmid == INT32_MAX /*|| sop->xmid == MAXSO*/)
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

        for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
        {
            int32_t ratio = smoothratio * synctics + 65536 * interp->tic;
            ratio /= interp->lasttic;
            ratio = (interp->tic == interp->lasttic) ? 65536 : ratio;
            if (data->spriteofang >= 0)
                *(int16_t *)(data->curipos) = NORM_ANGLE(data->lastoldipos + mulscale16(data->lastangdiff, ratio));
            else
            {
                delta = data->lastipos - data->lastoldipos;
                *(int32_t *)(data->curipos) = data->lastoldipos + mulscale16(delta, ratio);
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
        if (sop->xmid == INT32_MAX /*|| sop->xmid == MAXSO*/)
            continue;

        for (i = 0, data = interp->data; i < interp->numinterpolations; i++, data++)
            if (data->spriteofang >= 0)
                *(int16_t *)(data->curipos) = data->bakipos;
            else
                *(int32_t *)(data->curipos) = data->bakipos;
    }
}

int SaveSymDataInfo(MFILE_WRITE fil, void *ptr);

SWBOOL so_writeinterpolations(MFILE_WRITE fil)
{
    int32_t i;
    SECTOR_OBJECTp sop;
    so_interp *interp;
    SWBOOL saveisshot = FALSE;

    for (sop = SectorObject, interp = so_interpdata;
         sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++, interp++)
    {
        so_interp::interp_data *data = interp->data;
        MWRITE(&interp->numinterpolations,sizeof(interp->numinterpolations),1,fil);
        for (i = 0; i < interp->numinterpolations; i++, data++)
        {
            saveisshot |= SaveSymDataInfo(fil, data->curipos);
            MWRITE(&data->oldipos,sizeof(data->oldipos),1,fil);
            MWRITE(&data->spriteofang,sizeof(data->spriteofang),1,fil);
        }
    }
    return saveisshot;
}

int LoadSymDataInfo(MFILE_READ fil, void **ptr);

SWBOOL so_readinterpolations(MFILE_READ fil)
{
    int32_t i;
    SECTOR_OBJECTp sop;
    so_interp *interp;
    SWBOOL saveisshot = FALSE;

    for (sop = SectorObject, interp = so_interpdata;
         sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++, interp++)
    {
        so_interp::interp_data *data = interp->data;
        MREAD(&interp->numinterpolations,sizeof(interp->numinterpolations),1,fil);
        for (i = 0; i < interp->numinterpolations; i++, data++)
        {
            saveisshot |= LoadSymDataInfo(fil, (void **)&data->curipos);
            MREAD(&data->oldipos,sizeof(data->oldipos),1,fil);
            MREAD(&data->spriteofang,sizeof(data->spriteofang),1,fil);
            data->lastipos = data->lastoldipos = data->oldipos;
            data->lastangdiff = 0;
        }
        interp->tic = 0;
        interp->lasttic = synctics;
    }
    return saveisshot;
}

END_SW_NS
