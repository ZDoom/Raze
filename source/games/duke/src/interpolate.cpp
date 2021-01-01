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
#include "interpolate.h"

BEGIN_DUKE_NS

void setsectinterpolate(int sectnum)
{
	int j, k, startwall,endwall;
	auto sect = &sector[sectnum];

	startwall = sect->wallptr;
	endwall = startwall+sect->wallnum;

	for(j=startwall;j<endwall;j++)
	{
		StartInterpolation(j, Interp_Wall_X);
		StartInterpolation(j, Interp_Wall_Y);
		k = wall[j].nextwall;
		if(k >= 0)
		{
			StartInterpolation(k, Interp_Wall_X);
			StartInterpolation(k, Interp_Wall_Y);
			k = wall[k].point2;
			StartInterpolation(k, Interp_Wall_X);
			StartInterpolation(k, Interp_Wall_Y);
		}
	}
}

void clearsectinterpolate(int sectnum)
{
	short j,startwall,endwall;
	auto sect = &sector[sectnum];

	startwall = sect->wallptr;
	endwall = startwall + sect->wallnum;
	for(j=startwall;j<endwall;j++)
	{
		StopInterpolation(j, Interp_Wall_X);
		StopInterpolation(j, Interp_Wall_Y);
		if(wall[j].nextwall >= 0)
		{
			StopInterpolation(wall[j].nextwall, Interp_Wall_X);
			StopInterpolation(wall[j].nextwall, Interp_Wall_Y);
		}
	}
}

END_DUKE_NS

