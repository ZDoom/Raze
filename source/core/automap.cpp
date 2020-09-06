//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

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
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//------------------------------------------------------------------------- 

#include "automap.h"
#include "cstat.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "gstrings.h"
#include "printf.h"
#include "serializer.h"


bool automapping;
bool gFullMap;
FixedBitArray<MAXSECTORS> show2dsector;
FixedBitArray<MAXWALLS> show2dwall;
FixedBitArray<MAXSPRITES> show2dsprite;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SerializeAutomap(FSerializer& arc)
{
    if (arc.BeginObject("automap"))
    {
        arc("automapping", automapping)
            ("fullmap", gFullMap)
            // Only store what's needed. Unfortunately for sprites it is not that easy
            .SerializeMemory("mappedsectors", show2dsector.Storage(), (numsectors + 7) / 8)
            .SerializeMemory("mappedwalls", show2dwall.Storage(), (numwalls + 7) / 8)
            .SerializeMemory("mappedsprites", show2dsprite.Storage(), MAXSPRITES / 8)
            .EndObject();
    }
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(allmap)
{
	if (!CheckCheatmode(true, false))
	{
		gFullMap = !gFullMap;
		Printf("%s\n", GStrings(gFullMap ? "SHOW MAP: ON" : "SHOW MAP: OFF"));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ClearAutomap()
{
    show2dsector.Zero();
    show2dwall.Zero();
    show2dsprite.Zero();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void MarkSectorSeen(int i)
{
	if (i >= 0) 
	{
		show2dsector.Set(i);
		auto wal = &wall[sector[i].wallptr];
		for (int j = sector[i].wallnum; j > 0; j--, wal++)
		{
			i = wal->nextsector;
			if (i < 0) continue;
			if (wal->cstat & 0x0071) continue;
			if (wall[wal->nextwall].cstat & 0x0071) continue;
			if (sector[i].lotag == 32767) continue;
			if (sector[i].ceilingz >= sector[i].floorz) continue;
			show2dsector.Set(i);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawOverheadMap(int pl_x, int pl_y, int pl_angle)
{
}

