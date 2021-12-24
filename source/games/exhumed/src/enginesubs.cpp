//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "engine.h"
#include "precache.h"

//#include <io.h>
//#include <fcntl.h>
#include "gamecvars.h"
#include "gamecontrol.h"

// static int globhiz, globloz, globhihit, globlohit;

BEGIN_PS_NS


void resettiming()
{
    lastTic = -1;
}

void precache()
{
    if (!r_precache) return;

    for (auto& sect: sector)
    {
        int j = sect.ceilingpicnum;
        markTileForPrecache(j, sect.ceilingpal);
        if (picanm[j].sf & PICANM_ANIMTYPE_MASK)
            for (int k = 1; k <= picanm[j].num; k++)  markTileForPrecache(j + k, sect.ceilingpal);

        j = sect.floorpicnum;
        markTileForPrecache(j, sect.floorpal);
        if (picanm[j].sf & PICANM_ANIMTYPE_MASK)
            for (int k = 1; k <= picanm[j].num; k++)  markTileForPrecache(j + k, sect.floorpal);
    }

    for(auto& wal : wall)
    {
        int j = wal.picnum;
        markTileForPrecache(j, wal.pal);
        if (picanm[j].sf & PICANM_ANIMTYPE_MASK)
            for (int k = 1; k <= picanm[j].num; k++)  markTileForPrecache(j + k, wal.pal);

        if (wal.twoSided())
        {
            j = wal.overpicnum;
            markTileForPrecache(j, wal.pal);
            if (picanm[j].sf & PICANM_ANIMTYPE_MASK)
                for (int k = 1; k <= picanm[j].num; k++)  markTileForPrecache(j + k, wal.pal);

        }
    }

	ExhumedSpriteIterator it;
	while (auto ac = it.Next())
    {
        int j = ac->spr.picnum;
        markTileForPrecache(j, ac->spr.pal);
        if (picanm[j].sf & PICANM_ANIMTYPE_MASK)
            for (int k = 1; k <= picanm[j].num; k++)  markTileForPrecache(j + k, ac->spr.pal);
    }
    precacheMarkedTiles();
}
END_PS_NS
