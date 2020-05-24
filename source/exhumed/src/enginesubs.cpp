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
#if 0
    rotatesprite(thex << 16, they << 16, 0x10000, (short)((flags & 8) << 7), tilenum, shade, dapalnum,
        (char)(((flags & 1 ^ 1) << 4) + (flags & 2) + ((flags & 4) >> 2) + ((flags & 16) >> 2) ^ ((flags & 8) >> 1)),
        windowx1, windowy1, windowx2, windowy2);
#endif
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

void permanentwritesprite(int thex, int they, short tilenum, signed char shade, int cx1, int cy1, int cx2, int cy2, char dapalnum)
{
    rotatesprite(thex << 16, they << 16, 65536L, 0, tilenum, shade, dapalnum, 8 + 16, cx1, cy1, cx2, cy2);
}

void resettiming()
{
    numframes = 0L;
    totalclock = 0L;
// TODO	totalclocklock = 0L;
}


static int32_t xdim_to_320_16(int32_t x)
{
    const int32_t screenwidth = scale(240<<16, xdim, ydim);
    return scale(x, screenwidth, xdim) + (160<<16) - (screenwidth>>1);
}

static int32_t ydim_to_200_16(int32_t y)
{
    y = scale(y, 200<<16, ydim);
    return divscale16(y - (200<<15), rotatesprite_yxaspect) - rotatesprite_y_offset + (200<<15);
}

static int32_t xdim_from_320_16(int32_t x)
{
    const int32_t screenwidth = scale(240<<16, xdim, ydim);
    return scale(x + (screenwidth>>1) - (160<<16), xdim, screenwidth);
}

static int32_t ydim_from_200_16(int32_t y)
{
    y = mulscale16(y + rotatesprite_y_offset - (200<<15), rotatesprite_yxaspect) + (200<<15);
    return scale(y, ydim, 200<<16);
}

void printext(int x, int y, const char *buffer, short tilenum, char UNUSED(invisiblecol))
{
    int i;
    unsigned char ch;
//    const int32_t screenwidth = scale(240<<16, xdim, ydim);

    x = xdim_to_320_16(x);
    y = ydim_to_200_16(y);

    for (i = 0; buffer[i] != 0; i++)
    {
        ch = (unsigned char)buffer[i];
        rotatesprite(x - ((ch & 15) << (3+16)), y - ((ch >> 4) << (3+16)), 65536L, 0, tilenum, 0, 0, 2 + 8 + 16 + 128, xdim_from_320_16(x), ydim_from_200_16(y),
            xdim_from_320_16(x + (8<<16))-1, ydim_from_200_16(y + (8<<16))-1);
        x += (8<<16);
    }
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
