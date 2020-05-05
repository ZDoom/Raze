//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2017-2019 - Nuke.YKT
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
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

This file is a combination of code from the following sources:
- EDuke 2 by Matt Saettler
- JFDuke by Jonathon Fowler (jf@jonof.id.au),
- DukeGDX and RedneckGDX by Alexander Makarov-[M210] (m210-2007@mail.ru)
- Redneck Rampage reconstructed source by Nuke.YKT

Note:
 Most of this code follows DukeGDX and RedneckGDX because for Java it had
 to undo all the macro hackery that make the Duke source extremely hard to read.
 The other code bases were mainly used to add missing feature support (e.g. WW2GI)
 and verify correctness.
 
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names.h"

BEGIN_DUKE_NS

bool ifsquished(int i, int p) 
{
	if (g_gameType & GAMEFLAG_RRALL) return false;	// this function is a no-op in RR's source.
	
	bool squishme = false;
	if (sprite[i].picnum == TILE_APLAYER && ud.clipping)
		return false;

	auto &sc = sector[sprite[i].sectnum];
	int floorceildist = sc.floorz - sc.ceilingz;

	if (sc.lotag != ST_23_SWINGING_DOOR)
	{
		if (sprite[i].pal == 1)
			squishme = floorceildist < (32 << 8) && (sc.lotag & 32768) == 0;
		else
			squishme = floorceildist < (12 << 8);
	}

	if (squishme) 
	{
		FTA(QUOTE_SQUISHED, ps[p]);

		if (badguy(&sprite[i]))
			sprite[i].xvel = 0;

		if (sprite[i].pal == 1) 
		{
			hittype[i].picnum = SHOTSPARK1;
			hittype[i].extra = 1;
			return false;
		}

		return true;
	}
	return false;
}


END_DUKE_NS
