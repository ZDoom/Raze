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

void doTileLoad(int i)
{
	if (r_precache) PrecacheHardwareTextures(i);
}

void precache()
{
    int i;

    for (i = 0; i < numsectors; i++)
    {
        short j = sector[i].ceilingpicnum;
        doTileLoad(j);
        j = sector[i].floorpicnum;
		doTileLoad(j);
    }

    for (i = 0; i < numwalls; i++)
    {
        short j = wall[i].picnum;
		doTileLoad(j);
    }

    for (i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            short j = sprite[i].picnum;
			doTileLoad(j);
        }
    }
}
END_PS_NS
