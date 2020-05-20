//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include "ns.h"	// Must come before everything else!
#include "global.h"

BEGIN_DUKE_NS

// No need to revert this to the original state - to be robust this needs to be redone entirely, so this code will disappear anyway.

#define MAXINTERPOLATIONS MAXSPRITES

int32_t numinterpolations;
int32_t g_interpolationLock;
int32_t oldipos[MAXINTERPOLATIONS];
int32_t *curipos[MAXINTERPOLATIONS];
int32_t bakipos[MAXINTERPOLATIONS];

int G_SetInterpolation(int32_t *const posptr)
{
    if (numinterpolations >= MAXINTERPOLATIONS)
        return 1;

    for (bssize_t i = 0; i < numinterpolations; ++i)
        if (curipos[i] == posptr)
            return 0;

    curipos[numinterpolations] = posptr;
    oldipos[numinterpolations] = *posptr;
    numinterpolations++;
    return 0;
}

void G_StopInterpolation(const int32_t * const posptr)
{
    for (bssize_t i = 0; i < numinterpolations; ++i)
        if (curipos[i] == posptr)
        {
            numinterpolations--;
            oldipos[i] = oldipos[numinterpolations];
            bakipos[i] = bakipos[numinterpolations];
            curipos[i] = curipos[numinterpolations];
        }
}

void G_DoInterpolations(int smoothRatio)
{
    if (g_interpolationLock++)
        return;

    int32_t ndelta = 0;

    for (bssize_t i = 0, j = 0; i < numinterpolations; ++i)
    {
        int32_t const odelta = ndelta;
        bakipos[i] = *curipos[i];
        ndelta = (*curipos[i]) - oldipos[i];
        if (odelta != ndelta)
            j = mulscale16(ndelta, smoothRatio);
        *curipos[i] = oldipos[i] + j;
    }
}


void G_UpdateInterpolations(void)  //Stick at beginning of G_DoMoveThings
{
    for (bssize_t i=numinterpolations-1; i>=0; i--) oldipos[i] = *curipos[i];
}

void G_RestoreInterpolations(void)  //Stick at end of drawscreen
{
    int32_t i=numinterpolations-1;

    if (--g_interpolationLock)
        return;

    for (; i>=0; i--) *curipos[i] = bakipos[i];
}


END_DUKE_NS
