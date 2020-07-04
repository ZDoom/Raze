//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

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

#include "ns.h"
#include "duke3d.h"


BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// Floor Over Floor

// If standing in sector with SE42
// then draw viewing to SE41 and raise all =hi SE43 cielings.

// If standing in sector with SE43
// then draw viewing to SE40 and lower all =hi SE42 floors.

// If standing in sector with SE44
// then draw viewing to SE40.

// If standing in sector with SE45
// then draw viewing to SE41.
//
//---------------------------------------------------------------------------

static int tempsectorz[MAXSECTORS];
static int tempsectorpicnum[MAXSECTORS];
//short tempcursectnum;

void SE40_Draw(int tag, int spnum, int x, int y, int z, int a, int h, int smoothratio)
{
    int i, j = 0, k = 0;
    int floor1, floor2 = 0, ok = 0, fofmode = 0;
    int offx, offy;

    if (sprite[spnum].ang != 512) return;

    i = FOF;    //Effect TILE
    tileDelete(FOF);
    if (!(gotpic[i >> 3] & (1 << (i & 7)))) return;
    gotpic[i >> 3] &= ~(1 << (i & 7));

    floor1 = spnum;

    if (sprite[spnum].lotag == tag + 2) fofmode = tag + 0;
    if (sprite[spnum].lotag == tag + 3) fofmode = tag + 1;
    if (sprite[spnum].lotag == tag + 4) fofmode = tag + 0;
    if (sprite[spnum].lotag == tag + 5) fofmode = tag + 1;

    ok++;

    for (j = 0; j < MAXSPRITES; j++)
    {
        if (
            sprite[j].picnum == 1 &&
            sprite[j].lotag == fofmode &&
            sprite[j].hitag == sprite[floor1].hitag
            ) {
            floor1 = j; fofmode = sprite[j].lotag; ok++; break;
        }
    }
    // if(ok==1) { Message("no floor1",RED); return; }

    if (fofmode == tag + 0) k = tag + 1; else k = tag + 0;

    for (j = 0; j < MAXSPRITES; j++)
    {
        if (
            sprite[j].picnum == 1 &&
            sprite[j].lotag == k &&
            sprite[j].hitag == sprite[floor1].hitag
            ) {
            floor2 = j; ok++; break;
        }
    }

    // if(ok==2) { Message("no floor2",RED); return; }

    for (j = 0; j < MAXSPRITES; j++)  // raise ceiling or floor
    {
        if (sprite[j].picnum == 1 &&
            sprite[j].lotag == k + 2 &&
            sprite[j].hitag == sprite[floor1].hitag
            )
        {
            if (k == tag + 0)
            {
                tempsectorz[sprite[j].sectnum] = sector[sprite[j].sectnum].floorz;
                sector[sprite[j].sectnum].floorz += (((z - sector[sprite[j].sectnum].floorz) / 32768) + 1) * 32768;
                tempsectorpicnum[sprite[j].sectnum] = sector[sprite[j].sectnum].floorpicnum;
                sector[sprite[j].sectnum].floorpicnum = 13;
            }
            if (k == tag + 1)
            {
                tempsectorz[sprite[j].sectnum] = sector[sprite[j].sectnum].ceilingz;
                sector[sprite[j].sectnum].ceilingz += (((z - sector[sprite[j].sectnum].ceilingz) / 32768) - 1) * 32768;
                tempsectorpicnum[sprite[j].sectnum] = sector[sprite[j].sectnum].ceilingpicnum;
                sector[sprite[j].sectnum].ceilingpicnum = 13;
            }
        }
    }

    i = floor1;
    offx = x - sprite[i].x;
    offy = y - sprite[i].y;
    i = floor2;
#if 0
    drawrooms(offx + sprite[i].x, offy + sprite[i].y, z, a, h, sprite[i].sectnum);
#else
    renderDrawRoomsQ16(sprite[i].x + offx, sprite[i].y + offy, z, a, h, sprite[i].sectnum);
#endif

    fi.animatesprites(offx + sprite[i].x, offy + sprite[i].y, fix16_to_int(a), smoothratio);
    renderDrawMasks();

    for (j = 0; j < MAXSPRITES; j++)  // restore ceiling or floor
    {
        if (sprite[j].picnum == 1 &&
            sprite[j].lotag == k + 2 &&
            sprite[j].hitag == sprite[floor1].hitag
            )
        {
            if (k == tag + 0)
            {
                sector[sprite[j].sectnum].floorz = tempsectorz[sprite[j].sectnum];
                sector[sprite[j].sectnum].floorpicnum = tempsectorpicnum[sprite[j].sectnum];
            }
            if (k == tag + 1)
            {
                sector[sprite[j].sectnum].ceilingz = tempsectorz[sprite[j].sectnum];
                sector[sprite[j].sectnum].ceilingpicnum = tempsectorpicnum[sprite[j].sectnum];
            }
        }// end if
    }// end for

} // end SE40


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void se40code(int tag, int x, int y, int z, int a, int h, int smoothratio)
{
    int i;

    i = headspritestat[STAT_RAROR];
    while (i >= 0)
    {
        switch (sprite[i].lotag - tag + 40)
        {
            //            case 40:
            //            case 41:
            //                SE40_Draw(i,x,y,a,smoothratio);
            //                break;
        case 42:
        case 43:
        case 44:
        case 45:
            if (ps[screenpeek].cursectnum == sprite[i].sectnum)
                SE40_Draw(tag, i, x, y, z, a, h, smoothratio);
            break;
        }
        i = nextspritestat[i];
    }
}


END_DUKE_NS
