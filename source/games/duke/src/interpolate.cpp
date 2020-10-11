//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
aint with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------

#include "ns.h"	// Must come before everything else!
#include "global.h"

BEGIN_DUKE_NS

int32_t numinterpolations;
int32_t g_interpolationLock;
int32_t oldipos[MAXINTERPOLATIONS];
int32_t *curipos[MAXINTERPOLATIONS];
int32_t bakipos[MAXINTERPOLATIONS];


void updateinterpolations()  //Stick at beginning of domovethings
{
	int i;

	for(i=numinterpolations-1;i>=0;i--) oldipos[i] = *curipos[i];
}


void setinterpolation(int *posptr)
{
	int i;

	if (numinterpolations >= MAXINTERPOLATIONS) return;
	for(i=numinterpolations-1;i>=0;i--)
		if (curipos[i] == posptr) return;
	curipos[numinterpolations] = posptr;
	oldipos[numinterpolations] = *posptr;
	numinterpolations++;
}

void stopinterpolation(int *posptr)
{
	int i;

	for(i=numinterpolations-1;i>=0;i--)
		if (curipos[i] == posptr)
		{
			numinterpolations--;
			oldipos[i] = oldipos[numinterpolations];
			bakipos[i] = bakipos[numinterpolations];
			curipos[i] = curipos[numinterpolations];
		}
}

void dointerpolations(int smoothratio)       //Stick at beginning of drawscreen
{
	int i, j, odelta, ndelta;

	ndelta = 0; j = 0;
	for(i=numinterpolations-1;i>=0;i--)
	{
		bakipos[i] = *curipos[i];
		odelta = ndelta; ndelta = (*curipos[i])-oldipos[i];
		if (odelta != ndelta) j = mulscale16(ndelta,smoothratio);
		*curipos[i] = oldipos[i]+j;
	}
}

void restoreinterpolations()  //Stick at end of drawscreen
{
	int i;

	for(i=numinterpolations-1;i>=0;i--) *curipos[i] = bakipos[i];
}


void setsectinterpolate(int i)
{
	int j, k, startwall,endwall;
	auto sect = &sector[sprite[i].sectnum];

	startwall = sect->wallptr;
	endwall = startwall+sect->wallnum;

	for(j=startwall;j<endwall;j++)
	{
		setinterpolation(&wall[j].x);
		setinterpolation(&wall[j].y);
		k = wall[j].nextwall;
		if(k >= 0)
		{
			setinterpolation(&wall[k].x);
			setinterpolation(&wall[k].y);
			k = wall[k].point2;
			setinterpolation(&wall[k].x);
			setinterpolation(&wall[k].y);
		}
	}
}

void clearsectinterpolate(short i)
{
	short j,startwall,endwall;
	auto sect = &sector[sprite[i].sectnum];

	startwall = sect->wallptr;
	endwall = startwall + sect->wallnum;
	for(j=startwall;j<endwall;j++)
	{
		stopinterpolation(&wall[j].x);
		stopinterpolation(&wall[j].y);
		if(wall[j].nextwall >= 0)
		{
			stopinterpolation(&wall[wall[j].nextwall].x);
			stopinterpolation(&wall[wall[j].nextwall].y);
		}
	}
}

END_DUKE_NS

