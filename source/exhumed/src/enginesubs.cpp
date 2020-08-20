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

// static int globhiz, globloz, globhihit, globlohit;

BEGIN_PS_NS


void overwritesprite(int thex, int they, short tilenum, signed char shade, char stat, char dapalnum, int basepal)
{
    // no animation
    uint8_t animbak = picanm[tilenum].sf;
    picanm[tilenum].sf = 0;
    int offx = 0, offy = 0;
    if (stat & 1)
    {
        offx -= tilesiz[tilenum].x>>1;
        if (stat & 8)
            offx += tileLeftOffset(tilenum);
        else
            offx -= tileLeftOffset(tilenum);
        offy -= (tilesiz[tilenum].y>>1)+tileTopOffset(tilenum);
    }
    if (stat&8)
        offx += tilesiz[tilenum].x;
    if (stat&16)
        offy += tilesiz[tilenum].y;
    thex += offx;
    they += offy;
    rotatesprite(thex << 16, they << 16, 65536L, (stat & 8) << 7, tilenum, shade, dapalnum,
        16 + (stat & 2) + ((stat & 4) >> 2) + (((stat & 16) >> 2) ^ ((stat & 8) >> 1)),
        windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y, nullptr, basepal);
    picanm[tilenum].sf = animbak;
}

void resettiming()
{
    numframes = 0L;
    totalclock = 0L;
// TODO	totalclocklock = 0L;
}

void doTileLoad(int i)
{
	tileLoad(i);

#ifdef USE_OPENGL
	if (r_precache) PrecacheHardwareTextures(i);
#endif

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
