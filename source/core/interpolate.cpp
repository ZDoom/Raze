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

#include "interp.h"
#include "m_fixed.h"

#define (MAXINTERPOLATIONS 16384 + 256)
static int recursions = 0;
static int numinterpolations = 0;

// Not used yet. The entire interpolation feature as-is is highly serialization unfriendly because it only stores pointers without context, meaning there is no safe way to store them in a savegame without constantly risking breakage.

// Todo: This really needs to be made serialization friendly
struct Interpolation
{
	int oldipos;
	int bakipos;
	void *curipos;
	bool isshort;
};

static Interpolation interpolations[MAXINTERPOLATIONS];


void setinterpolation(void *posptr, bool isshort)
{
    if (numinterpolations >= MAXINTERPOLATIONS)
        return;

    for (int i = numinterpolations - 1; i >= 0; i--)
    {
        if (interpolations[i].curipos == posptr)
            return;
    }

    interpolations[numinterpolations].curipos = posptr;
    interpolations[numinterpolations].oldipos = *posptr;
    numinterpolations++;
}

void setinterpolation(int *posptr)
{
	setinterpolation(posptr, false);
}

// only used by SW to interpolate floorheinum and ceilingheinum
void setinterpolation(short *posptr)
{
	setinterpolation(posptr, true);
}

void stopinterpolation(void *posptr)
{
    for (int i = numinterpolations - 1; i >= 0; i--)
    {
        if (curipos[i] == posptr)
        {
            numinterpolations--;
			interpolations[i] = interpolations[numinterpolations];
        }
    }
}

void updateinterpolations(void)                  // Stick at beginning of domovethings
{
    int i;

    for (int i = numinterpolations - 1; i >= 0; i--)
        interpolations[i].oldipos = interpolations[i].isshort? *(short*)interpolations[i].curipos : *(int*)interpolations[i].curipos;
}

// must call restore for every do interpolations
// make sure you don't exit
void dointerpolations(int smoothratio)                      // Stick at beginning of drawscreen
{
    if (recursions++)
        return;
	
    int i, j, odelta, ndelta;

    ndelta = 0;
    j = 0;

    for (i = numinterpolations - 1; i >= 0; i--)
    {
        bakipos[i] = *curipos[i];

        odelta = ndelta;
        ndelta = (*curipos[i]) - oldipos[i];

        if (odelta != ndelta)
            j = FixedMul(ndelta, smoothratio);

        *curipos[i] = oldipos[i] + j;
    }
}

void restoreinterpolations(bool force)                 // Stick at end of drawscreen
{
    int i;

    if (!force && --recursions)
        return;

	recursions = 0;	// if interpolations are forcibly restored, the recursion counter must also be reset.
    for (i = numinterpolations - 1; i >= 0; i--)
        *curipos[i] = bakipos[i];
}

void togglespriteinterpolation(spritetype *sp, int set)
{
    auto func = set ? setinterpolation : stopinterpolation;
    func(&sp->x);
    func(&sp->y);
    func(&sp->z);
}

