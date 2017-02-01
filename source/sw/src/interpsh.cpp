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

#include "compat.h"
#include "pragmas.h"

#include "interp.h"

#define SHORT_MAXINTERPOLATIONS 256
short short_numinterpolations = 0, short_startofdynamicinterpolations = 0;
short short_oldipos[SHORT_MAXINTERPOLATIONS];
short short_bakipos[SHORT_MAXINTERPOLATIONS];
short *short_curipos[SHORT_MAXINTERPOLATIONS];

void short_setinterpolation(short *posptr)
{
    int i;

    if (short_numinterpolations >= SHORT_MAXINTERPOLATIONS)
        return;

    for (i = short_numinterpolations - 1; i >= 0; i--)
    {
        if (short_curipos[i] == posptr)
            return;
    }

    short_curipos[short_numinterpolations] = posptr;
    short_oldipos[short_numinterpolations] = *posptr;
    short_numinterpolations++;
}

void short_stopinterpolation(short *posptr)
{
    int i;

    for (i = short_numinterpolations - 1; i >= short_startofdynamicinterpolations; i--)
    {
        if (short_curipos[i] == posptr)
        {
            short_numinterpolations--;
            short_oldipos[i] = short_oldipos[short_numinterpolations];
            short_bakipos[i] = short_bakipos[short_numinterpolations];
            short_curipos[i] = short_curipos[short_numinterpolations];
        }
    }
}

void short_updateinterpolations(void)                  // Stick at beginning of domovethings
{
    int i;

    for (i = short_numinterpolations - 1; i >= 0; i--)
        short_oldipos[i] = *short_curipos[i];
}

// must call restore for every do interpolations
// make sure you don't exit
void short_dointerpolations(int smoothratio)                      // Stick at beginning of drawscreen
{
    int i, j, odelta, ndelta;

    ndelta = 0;
    j = 0;

    for (i = short_numinterpolations - 1; i >= 0; i--)
    {
        short_bakipos[i] = *short_curipos[i];

        odelta = ndelta;
        ndelta = (*short_curipos[i]) - short_oldipos[i];

        if (odelta != ndelta)
            j = mulscale16(ndelta, smoothratio);

        *short_curipos[i] = short_oldipos[i] + j;
    }
}

void short_restoreinterpolations(void)                 // Stick at end of drawscreen
{
    int i;

    for (i = short_numinterpolations - 1; i >= 0; i--)
        *short_curipos[i] = short_bakipos[i];
}
